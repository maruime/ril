/*	see copyright notice in ril.h */

#include "ril_pcheader.h"
#include "ril_vm.h"
#include "ril_var.h"
#include "ril_state.h"
#include "ril_utils.h"

typedef struct
{
  int size;
  int lastindex;
  ril_var_t *returnvar;
} ril_localvar_t;

typedef struct
{
  hashmap_key_t key;
  ril_var_t var;
} ril_varstack_t;

ril_state_t* ril_newstate(RILVM vm)
{
  ril_state_t *state = (ril_state_t*)ril_malloc(sizeof(ril_state_t));
  ril_vmcmd_t *cmd;
  int i;

  state->vm = vm;
  state->lastlabel = 0;
  state->returnvar = NULL;

  state->argc = 0;
  for (i = RIL_ARGUMENT_SIZE - 1; 0 <= i; --i)
  {
    ril_initvar(vm, &state->args[i].temp);
  }

  ril_initvar(vm, &state->rootvar);
  state->varbuffer = buffer_open(sizeof(ril_varstack_t), 64);
  
  state->tag_stack = stack_open(sizeof(ril_tagstack_t));
  stack_resize(state->tag_stack, STACK_BUFFER_SIZE);
  state->ext_buffer = buffer_open(1, EXT_BUFFER_SIZE);

  cmd = &state->tmpcmd[1];
  cmd->tag = ril_getregisteredtag2(vm, RIL_TAG_EXIT);
  cmd->parent = NULL;
  cmd->nextpair = NULL;
  cmd->pair = NULL;
  cmd->arg = NULL;

  state->cmd.cur = NULL;
  state->cmd.prev = NULL;
  state->cmd.next = vm->code.cmd;

  return state;
}

void ril_clearstate(ril_state_t *state)
{
  ril_tagstack_t *tagstack;
  ril_state_t *backupstate = ril_getstate(state->vm);
  int i;

  ril_setstate(state);
  while (NULL != (tagstack = stack_back(state->tag_stack, NULL)))
  {
    if (NULL != tagstack->tag->delete_handler) tagstack->tag->delete_handler(state->vm);
    stack_pop(state->tag_stack, NULL);
  }
  ril_setstate(backupstate);

  ril_cleararray(state->vm, &state->rootvar);
  for (i = buffer_size(state->varbuffer) - 1; 0 <= i; --i)
  {
    ril_clearvar(state->vm, &((ril_varstack_t*)buffer_index(state->varbuffer, i))->var);
  }
  buffer_clear(state->varbuffer);

  if (NULL != state->returnvar) ril_deletevar(state->vm, state->returnvar);
}

void ril_deletestate(ril_state_t *state)
{
  int i;

  if (state == ril_getstate(state->vm))
  {
    ril_setmainstate(state->vm);
  }
  
  ril_clearstate(state);

  for (i = 0; i < RIL_ARGUMENT_SIZE; ++i)
  {
    ril_clearvar(state->vm, &state->args[i].temp);
  }

  buffer_close(state->varbuffer);
  stack_close(state->tag_stack);
  buffer_close(state->ext_buffer);
  
  ril_free(state);
}

void ril_setstate(ril_state_t *state)
{
  state->vm->state = state;
}

ril_state_t* ril_getstate(RILVM vm)
{
  return vm->state;
}

void ril_setmainstate(RILVM vm)
{
  vm->state = vm->mainstate;
}

RILRESULT ril_savestate(RILVM vm, buffer_t *dest)
{
  int32_t i, *size;
  void *buf;
  
  buffer_write(dest, vm->loadfile, sizeof(vm->loadfile));
  buffer_write(dest, &vm->hash, sizeof(vm->hash));
  
  buf = buffer_malloc(dest, sizeof(ril_cmdid_t));
  (*(ril_cmdid_t*)buf) = _cmd2cmdid(vm, vm->state->cmd.cur);
  buf = buffer_malloc(dest, sizeof(ril_cmdid_t));
  (*(ril_cmdid_t*)buf) = _cmd2cmdid(vm, vm->state->cmd.next);
  buf = buffer_malloc(dest, sizeof(ril_crc_t));
  (*(ril_crc_t*)buf) = vm->state->lastlabel;
  //buf = buffer_malloc(dest, sizeof(ril_cmdid_t));
  //(*(ril_cmdid_t*)buf) = _cmd2cmdid(vm, vm->cmd.prev);
  
  //buffer_write(dest, &vm->isfirst, sizeof(vm->isfirst));
  
  size = (int32_t*)buffer_malloc(dest, sizeof(int32_t));
  *size = stack_count(vm->state->tag_stack);
  for (i = 0; i < *size; ++i)
  {
    ril_tagstack_t *tagstack = (ril_tagstack_t*)stack_index(vm->state->tag_stack, i, NULL);
    *(uint32_t*)buffer_malloc(dest, sizeof(ril_signature_t)) = ril_signature(tagstack->tag);
    if (RIL_FAILED(tagstack->tag->savestate_handler(vm, dest))) return RIL_ERROR;
  }

  size = (int32_t*)buffer_malloc(dest, sizeof(int32_t));
  *size = buffer_size(vm->state->varbuffer);
  for (i = 0; i < *size; ++i)
  {
    ril_varstack_t *varstack = (ril_varstack_t*)buffer_index(vm->state->varbuffer, i);
    buffer_write(dest, &varstack->key, sizeof(varstack->key));
    ril_writevar(dest, &varstack->var);
  }

  return RIL_OK;
}

int ril_loadstate(RILVM vm, const void *src)
{
  int32_t result, i, size;
  ril_md5_t hash;
  const void *cur = src;
  
  if ('\0' != *(char*)cur)
  {
    ril_cleararguments(vm->state);
    ril_setstring(vm, ril_getargument(vm, 0), (char*)cur);
    
    result = ril_getexecutehandler(ril_getregisteredtag(vm, "goto", "file"))(vm);
    if (RIL_FAILED(result))
    {
      return RIL_ERROR;
    }
  }
  cur = (int8_t*)cur + sizeof(vm->loadfile);
  
  cur = ril_read(&hash, cur, sizeof(hash));
  
  vm->state->cmd.cur = _cmdid2cmd(vm, (*(ril_cmdid_t*)cur));
  cur = (int8_t*)cur + sizeof(ril_cmdid_t);
  vm->state->cmd.next = _cmdid2cmd(vm, (*(ril_cmdid_t*)cur));
  cur = (int8_t*)cur + sizeof(ril_cmdid_t);
  cur = ril_read(&vm->state->lastlabel, cur, sizeof(ril_crc_t));
  //vm->cmd.prev = _cmdid2cmd(vm, (*(ril_cmdid_t*)cur));
  //cur += sizeof(ril_cmdid_t);
  
  //cur = ril_read(&vm->isfirst, cur, sizeof(vm->isfirst));
  
  cur = ril_read(&size, cur, sizeof(size));
  for (i = 0; i < size; ++i)
  {
    ril_tagstack_t *tagstack = (ril_tagstack_t*)stack_push(vm->state->tag_stack, NULL);
    tagstack->buffer_offset = buffer_size(vm->state->ext_buffer);
    tagstack->tag = ril_getregisteredtag2(vm, *(ril_signature_t*)cur);
    cur = (int8_t*)cur + sizeof(ril_signature_t);
    result = tagstack->tag->loadstate_handler(vm, cur);
    if (RIL_FAILED(result)) return RIL_ERROR;
    cur = (int8_t*)cur + result;
  }

  cur = ril_read(&size, cur, sizeof(size));
  for (i = 0; i < size; ++i)
  {
    ril_varstack_t *varstack = (ril_varstack_t*)buffer_malloc(vm->state->varbuffer, 1);
    cur = ril_read(&varstack->key, cur, sizeof(varstack->key));
    ril_initvar(vm, &varstack->var);
    cur = (uint8_t*)cur + ril_readvar(vm, &varstack->var, cur);
  }

  if (ril_md5cmp(hash, vm->hash))
  {
    vm->hash = hash;
    if (vm->state->lastlabel)
    {
      ril_gotolabelbyhash(vm, vm->state->lastlabel);
    }
    else
    {
      ril_cmdid_t cmdid;
      cmdid.id = 0;
      ril_goto(vm, cmdid);
    }
  }
  
  return (intptr_t)cur - (intptr_t)src;
}

void ril_pushlocalvar(RILVM vm)
{
  ril_localvar_t *workarea;
  ril_state_t *state = vm->state;

  /* backup vars */
  workarea = (ril_localvar_t*)ril_mallocworkarea(vm, sizeof(ril_localvar_t));
  workarea->size = ril_countvar(&state->rootvar);
  workarea->lastindex = buffer_size(state->varbuffer);
  workarea->returnvar = state->returnvar;
  state->returnvar= NULL;

  if (0 < workarea->size)
  {
    hashmap_clear(((ril_array_t*)state->rootvar.variant.ptr_value)->map);
  }
}

ril_var_t* ril_addlocalvar(RILVM vm, hashmap_key_t key)
{
  ril_varstack_t *varstack = (ril_varstack_t*)buffer_malloc(vm->state->varbuffer, 1);

  varstack->key = key;
  ril_initvar(vm, &varstack->var);
  ril_set2arraybyhash(vm, &vm->state->rootvar, &varstack->var, NULL, key);

  return &varstack->var;
}

void ril_poplocalvar(RILVM vm)
{
  int i;
  ril_localvar_t *workarea = (ril_localvar_t*)ril_workarea(vm);
  ril_varstack_t *varstack;
  ril_state_t *state = vm->state;

  /* restore return var */
  if (NULL != state->returnvar) ril_deletevar(vm, state->returnvar);
  state->returnvar = workarea->returnvar;
  
  /* clear array */
  ril_cleararray(vm, &vm->state->rootvar);

  varstack = (ril_varstack_t*)buffer_back(state->varbuffer);
  for (i = buffer_size(state->varbuffer) - 1; workarea->lastindex <= i; --i, --varstack)
  {
    ril_clearvar(state->vm, &varstack->var);
  }
  buffer_resize(state->varbuffer, workarea->lastindex);

  /* restore vars */
  varstack = (ril_varstack_t*)buffer_back(state->varbuffer);
  for (i = workarea->size - 1; 0 <= i; --i, --varstack)
  {
    ril_set2arraybyhash(state->vm, &state->rootvar, &varstack->var, NULL, varstack->key);
  }
}

void ril_savelocalvar(RILVM vm, buffer_t *buffer)
{
  ril_localvar_t *workarea = (ril_localvar_t*)ril_workarea(vm);

  ril_cleararray(vm, &vm->state->rootvar);

  buffer_write(buffer, &workarea->size, sizeof(workarea->size));
}

int ril_loadlocalvar(RILVM vm, const void *src)
{
  const void *cur = src;
  ril_localvar_t *workarea = (ril_localvar_t*)ril_mallocworkarea(vm, sizeof(ril_localvar_t));

  cur = ril_read(&workarea->size, cur, sizeof(workarea->size));

  return (uintptr_t)cur - (uintptr_t)src;
}

RILRESULT ril_copyarguments(RILVM vm, ril_state_t *dest)
{
  int i;
  ril_state_t *src = vm->state;

  ril_cleararguments(dest);
  for (i = src->argc - 1; 0 <= i; --i)
  {
    dest->args[i].var = &dest->args[i].temp;
    ril_copyvar(vm, dest->args[i].var, src->args[i].var);
  }
  dest->argc = src->argc;

  return RIL_OK;
}

RILRESULT ril_cleararguments(ril_state_t *state)
{
  state->argc = 0;

  return RIL_OK;
}

int ril_countarguments(RILVM vm)
{
  return vm->state->argc;
}

RILRESULT ril_setargumentsbycmd(RILVM vm, ril_vmcmd_t *cmd)
{
  int i, argc = buffer_size(cmd->tag->param_buffer);
  ril_vmarg_t *arg = cmd->arg;
  ril_register_t *argreg = vm->state->args, *reg;
  
  ril_cleararguments(vm->state);
  for (i = argc - 1; 0 <= i; --i, ++arg, ++argreg)
  {
    reg = calc_execute(vm, arg->data);
    /* copy with temporary */
    if (reg->var == &reg->temp)
    {
      ril_clearvar(vm, &argreg->temp);
      argreg->temp = *reg->var;
      argreg->var = &argreg->temp;
      reg->var->variant.type = VARIANT_NULL;
      continue;
    }
    argreg->parent = reg->parent;
    argreg->hashkey = reg->hashkey;
    argreg->var  = reg->var;
  }
  
  vm->state->argc = argc;
  
  return RIL_OK;
}

RILRESULT ril_setarguments(RILVM vm, ril_cmdid_t cmdid)
{
  return ril_setargumentsbycmd(vm, _cmdid2cmd(vm, cmdid));
}

ril_var_t* ril_getargument(RILVM vm, int index)
{
  ril_var_t *var;
  int i;

  if (index + 1 > vm->state->argc)
  {
    for (i = vm->state->argc; i <= index; ++i)
    {
      var = &vm->state->args[index].temp;
      ril_clearvar(vm, var);
      vm->state->args[index].var = var;
    }
    vm->state->argc = index + 1;
  }

  return vm->state->args[index].var;
}

const void* ril_getptr(RILVM vm, int index)
{
  variant_t v = ril_getargument(vm, index)->variant;
  calc_cast(vm->calc, &v, VARIANT_PTR);
  return v.ptr_value;
}

const char* ril_getstring(RILVM vm, int index)
{
  return ril_var2string(vm, ril_getargument(vm, index));
}

bool ril_getbool(RILVM vm, int index)
{
  return 0 != ril_getinteger(vm, index);
}

int ril_getinteger(RILVM vm, int index)
{
  return ril_var2integer(vm, ril_getargument(vm, index));
}

float ril_getfloat(RILVM vm, int index)
{
  return ril_var2float(vm, ril_getargument(vm, index));
}

int ril_has(RILVM vm, int index)
{
  return vm->state->argc > index;
}

void ril_set(RILVM vm, ril_var_t *var)
{
  if (NULL != vm->state->returnvar) ril_deletevar(vm, vm->state->returnvar);
  ril_retainvar(var);
  vm->state->returnvar = var;
}

void ril_return(RILVM vm, ril_var_t *var)
{
  if (NULL == vm->state->returnvar) return;
  ril_copyvar(vm, vm->state->returnvar, var);
  ril_deletevar(vm, vm->state->returnvar);
  vm->state->returnvar = NULL;
}

void ril_returninteger(RILVM vm, int value)
{
  if (NULL == vm->state->returnvar) return;
  ril_setinteger(vm, vm->state->returnvar, value);
  ril_deletevar(vm, vm->state->returnvar);
  vm->state->returnvar = NULL;
}

void ril_returnfloat(RILVM vm, float value)
{
  if (NULL == vm->state->returnvar) return;
  ril_setfloat(vm, vm->state->returnvar, value);
  ril_deletevar(vm, vm->state->returnvar);
  vm->state->returnvar = NULL;
}

void ril_returnstring(RILVM vm, const char *value)
{
  if (NULL == vm->state->returnvar) return;
  ril_setstring(vm, vm->state->returnvar, value);
  ril_deletevar(vm, vm->state->returnvar);
  vm->state->returnvar = NULL;
}

RILVM ril_state2vm(ril_state_t *state)
{
  return state->vm;
}
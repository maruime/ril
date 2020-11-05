/*	see copyright notice in ril.h */

#include <stdio.h>

#include "ril_pcheader.h"
#include "ril_vm.h"
#include "ril_state.h"
#include "ril_var.h"
#include "ril_compiler.h"
#include "ril_api.h"
#include "ril_utils.h"

typedef struct
{
  bool hasfile;
  union
  {
    struct
    {
      ril_vmcmd_t *cmd;
    };
    struct
    {
      ril_cmdid_t cmdid;
      char file[512];
    };
  };
} ril_return_t;

typedef struct
{
  ril_tag_t *tag;
  RILFUNCTION executehandler;
  RILSAVEFUNCTION savehandler;
  RILLOADFUNCTION loadhandler;
  RILDELETEFUNCTION deletehandler;
  void *shareddata;
} ril_function_t;

typedef struct
{
  ril_cmdid_t cmdid;
  struct
  {
    RILFUNCTION executehandler;
    void *shareddata;
  } ch, r;
  uint8_t toarray;
  ril_var_t var;
} ril_stream_t;

typedef struct
{
  ril_var_t *from, *item, *key;
  hashmap_entry_t *entry;
} ril_foreach_t;

RIL_FUNC(std_ch, vm)
{
  printf("%s", ril_getstring(vm, 0));
  
  return RIL_NEXT;
}

RIL_FUNC(std_r, vm)
{
  ril_var_t *var = ril_getargument(vm, 0);

  ril_setstring(vm, var, "\n");
  ril_ch(vm, var);
  
  return RIL_NEXT;
}

RIL_FUNC(label, vm)
{
  vm->state->lastlabel = *(ril_crc_t*)ril_getptr(vm, 0);
  
  return RIL_NEXT;
}

RIL_FUNC(set, vm)
{
  ril_set(vm, ril_getargument(vm, 0));
  return RIL_NEXT;
}

RIL_FUNC(unset, vm)
{
  ril_register_t *reg = vm->state->args;
  ril_unset2array(vm, reg->parent, reg->hashkey);
  
  return RIL_NEXT;
}

RIL_FUNC(isnull, vm)
{
  ril_var_t *var = ril_getargument(vm, 0);

  ril_returninteger(vm, ril_isnull(var));
  
  return RIL_NEXT;
}

RIL_FUNC(isint, vm)
{
  ril_var_t *var = ril_getargument(vm, 0);

  ril_returninteger(vm, ril_isint(var));
  
  return RIL_NEXT;
}

RIL_FUNC(isreal, vm)
{
  ril_var_t *var = ril_getargument(vm, 0);

  ril_returninteger(vm, ril_isreal(var));
  
  return RIL_NEXT;
}

RIL_FUNC(isarray, vm)
{
  ril_var_t *var = ril_getargument(vm, 0);

  ril_returninteger(vm, ril_isarray(var));
  
  return RIL_NEXT;
}

RIL_FUNC(isstring, vm)
{
  ril_var_t *var = ril_getargument(vm, 0);

  ril_returninteger(vm, ril_isstring(var));
  
  return RIL_NEXT;
}

RIL_FUNC(goto, vm)
{
  if (ril_has(vm, 0))
  {
    vm->state->cmd.next = &vm->code.cmd[ril_getinteger(vm, 0)];
    return RIL_NULL;
  }
  
  return RIL_NEXT;
}

RIL_FUNC(gotofile, vm)
{
  const char *file = ril_getstring(vm, 0);
  ril_code_t code;
  ril_label_t *label;
  uint32_t label_size;
  buffer_t *buffer = buffer_open(1, 512);
  int nexttagid = 0;
  
  if (RIL_FAILED(ril_compilefile(vm, file, buffer)))
  {
    return RIL_ERROR;
  }
  
  if (ril_has(vm, 1))
  {
    ril_parsecode(&code, buffer_front(buffer));
    
    label = vm->code.label;
    label_size = vm->code.common->label_size;
    
    vm->code.label = (void*)code.label;
    vm->code.common->label_size = code.common->label_size;
    
    nexttagid = ril_getinteger(vm, 1);
    
    vm->code.label = label;
    vm->code.common->label_size = label_size;
    
    if (0 > nexttagid)
    {
      return RIL_ERROR;
    }
  }

  if (RIL_FAILED(ril_load(vm, buffer_front(buffer), buffer_size(buffer))))
  {
    return RIL_ERROR;
  }
  vm->state->cmd.next = vm->code.cmd + nexttagid;
  
  buffer_close(buffer);
  
  ril_setfilename(vm, file);
  
  return RIL_NULL;
}

RIL_FUNC(gosub, vm)
{
  ril_return_t *rtn;
  
  ril_addstack(vm, ril_getregisteredtag2(vm, RIL_TAG_RETURN));
  rtn = ril_mallocworkarea(vm, sizeof(ril_return_t));
  rtn->cmd = vm->state->cmd.cur;
  rtn->hasfile = false;
  
  return ril_getexecutehandler(ril_getregisteredtag2(vm, RIL_TAG_GOTO))(vm);
}

RIL_FUNC(gosubfile, vm)
{
  ril_return_t *rtn;
  
  ril_addstack(vm, ril_getregisteredtag2(vm, RIL_TAG_RETURN));
  rtn = ril_mallocworkarea(vm, sizeof(ril_return_t));
  rtn->cmdid = _cmd2cmdid(vm, vm->state->cmd.cur);
  rtn->hasfile = true;
  strncpy(rtn->file, vm->loadfile, sizeof(rtn->file));
  
  return ril_getexecutehandler(ril_getregisteredtag2(vm, RIL_TAG_GOTOFILE))(vm);
}

RIL_FUNC(if, vm)
{
  int *result = ril_mallocworkarea(vm, sizeof(int));
  *result = ril_getbool(vm, 0);
  
  return *result ? RIL_NEXT : RIL_NEXTPAIR;
}

RIL_FUNC(else, vm)
{
  return *(int*)ril_workarea(vm) ? RIL_BREAKPAIR : RIL_NEXT;
}

RIL_FUNC(elseif, vm)
{
  int *result = (int*)ril_workarea(vm);
  
  if (!*result && ril_getbool(vm, 0))
  {
    *result = true;
    return RIL_NEXT;
  }
  
  return RIL_NEXTPAIR;
}

RIL_FUNC(endif, vm)
{
  return RIL_BREAKPAIR;
}

RIL_FUNC(let, vm)
{
  return RIL_NEXT;
}

RIL_COMPILEFUNC(macro, context)
{
  ril_tag_t *t;
  char name[128], args[256], localvars[256];
  ril_crc_t namehash;
  buffer_t *buffer;
  int i;
  
  /* name */
  strcpy(name, rilc_getstring(context, 0));
  
  /* args */
  strcpy(args, rilc_getstring(context, 1));
  
  /* local variables */
  strcpy(localvars, rilc_getstring(context, 2));
  
  if (NULL == ril_getregisteredtag(context->vm, name, args))
  {
    t = ril_registertag(context->vm, name, args, RIL_CALLFUNC(callmacro));
    RIL_SETSTORAGE(t, callmacro);
  }
  
  rilc_eraselastcmd(context);
  rilc_newcmd(context, ril_signature(context->tag));
  rilc_addstring(context, name);
  rilc_addstring(context, args);
  
  /* local variables */
  buffer = buffer_open(1, 512);
  ril_parseparameters(buffer, args);
  ril_parseparameters(buffer, localvars);
 
  rilc_addarg(context);
  i = sizeof(int32_t) + buffer_size(buffer) * sizeof(ril_crc_t);
  calc_writevalue2buffer(context->data_buffer, VARIANT_LITERAL | VARIANT_BYTES, NULL, sizeof(uint32_t) + i);
  *(uint32_t*)buffer_malloc(context->data_buffer, sizeof(uint32_t)) = i; /* byte size */
  *(int32_t*)buffer_malloc(context->data_buffer, sizeof(int32_t)) = buffer_size(buffer); /* var size */
  for (i = 0; i < buffer_size(buffer); ++i)
  {
    namehash = ril_makecrc(ril_getparametername(buffer, i));
    buffer_write(context->data_buffer, &namehash, sizeof(namehash));
  }
  calc_writeoperator(context->data_buffer, CALC_END);
  buffer_close(buffer);
  
  return RIL_OK;
}

RIL_FUNC(macro, vm)
{
  ril_tag_t *tag = ril_registertag(vm, ril_getstring(vm, 0), ril_getstring(vm, 1), RIL_CALLFUNC(callmacro));
  RIL_SETSTORAGE(tag, callmacro);
  ril_setshareddata(tag, vm->state->cmd.cur);

  return RIL_BREAKPAIR;
}

RIL_FUNC(callmacro, vm)
{
  int i, varsize;
  ril_tag_t *tag = ril_currenttag(vm);
  ril_var_t *var, *var2;
  ril_return_t *rtn;
  const ril_crc_t *localvars;

  var = ((ril_register_t*)calc_execute(vm, ((ril_vmcmd_t*)ril_getshareddata(tag))->arg[2].data))->var;
  localvars = (ril_crc_t*)variant_getbytes(&var->variant);
  
  varsize = *localvars;
  ++localvars;

  /* set local variables */
  ril_pushlocalvar(vm);
  ril_fetchlocalvar(vm);
  for (i = 0; i < varsize; ++i)
  {
    var = ril_addlocalvar(vm, *localvars);
    ++localvars;

    /* set arguemnt value */
    if (!ril_has(vm, i)) continue;
    var2 = ril_getargument(vm, i);
    ril_copyvar(vm, var, var2);
  }
  ril_fetchglobalvar(vm);
  
  vm->state->cmd.next = (ril_vmcmd_t*)ril_getshareddata(tag);
  
  ril_addstack(vm, ril_getregisteredtag2(vm, RIL_TAG_RETURN));
  rtn = (ril_return_t*)ril_mallocworkarea(vm, sizeof(ril_return_t));
  rtn->cmd = vm->state->cmd.cur;
  rtn->hasfile = false;
  
  return RIL_NEXT;
}

RIL_SAVEFUNC(callmacro, vm, dest)
{
  ril_savelocalvar(vm, dest);
  return RIL_OK;
}

RIL_LOADFUNC(callmacro, vm, src)
{
  return ril_loadlocalvar(vm, src);
}

RIL_DELETEFUNC(callmacro, vm)
{
  ril_poplocalvar(vm);

  return RIL_OK;
}

RIL_FUNC(endmacro, vm)
{
  return RIL_CALLFUNC(return)(vm);
}

RIL_FUNC(return, vm)
{
  ril_tagstack_t *stack;
  ril_return_t *workarea;
  ril_cmdid_t cmdid;
  ril_var_t var;
  
  ril_initvar(vm, &var);
  ril_copyvar(vm, &var, ril_getargument(vm, 0));
  
  for (;;)
  {
    stack = stack_back(vm->state->tag_stack, NULL);
    if (NULL == stack) break;
    if (RIL_TAG_RETURN == ril_signature(stack->tag)) break;
    stack->tag->delete_handler(vm);
    buffer_resize(vm->state->ext_buffer, stack->buffer_offset);
    stack_pop(vm->state->tag_stack, NULL);
  }
  
  if (NULL == stack)
  {
    return ril_getexecutehandler(ril_getregisteredtag2(vm, RIL_TAG_EXIT))(vm);
  }
  
  workarea = ril_workarea(vm);
  
  if (workarea->hasfile)
  {
    cmdid = workarea->cmdid;
    ril_cleararguments(ril_getstate(vm));
    ril_setstring(vm, ril_getargument(vm, 0), workarea->file);
    
    ril_releaseworkarea(vm, ril_currenttag(vm));
    
    if (RIL_FAILED(ril_getexecutehandler(ril_getregisteredtag2(vm, RIL_TAG_GOTOFILE))(vm)))
    {
      return RIL_ERROR;
    }
    
    ril_setnextcmd(vm, _cmdid2cmd(vm, cmdid));
  }
  else
  {
    ril_setnextcmd(vm, workarea->cmd);
    ril_releaseworkarea(vm, workarea->cmd->tag);
  }

  if (!ril_isnull(&var)) ril_return(vm, &var);
  ril_clearvar(vm, &var);
  
  return RIL_NEXT;
}

RIL_SAVEFUNC(return, vm, dest)
{
  ril_return_t *workarea = ril_workarea(vm);
  ril_cmdid_t cmdid;
  int32_t size;
  
  *(uint8_t*)buffer_malloc(dest, 1) = workarea->hasfile;
  if (workarea->hasfile)
  {
    cmdid = workarea->cmdid;
    buffer_write(dest, &cmdid, sizeof(cmdid));
    
    size = strlen(workarea->file) + 1;
    *(int32_t*)buffer_malloc(dest, sizeof(size)) = size;
    buffer_write(dest, workarea->file, size);
  }
  else
  {
    cmdid = _cmd2cmdid(vm, workarea->cmd);
    buffer_write(dest, &cmdid, sizeof(cmdid));
  }
  
  return RIL_OK;
}

RIL_LOADFUNC(return, vm, src)
{
  const char *cur = src;
  ril_return_t *workarea = (ril_return_t*)ril_mallocworkarea(vm, sizeof(ril_return_t));
  
  workarea->hasfile = *(uint8_t*)cur++;
  if (workarea->hasfile)
  {
    workarea->cmdid = *(ril_cmdid_t*)cur;
    cur += sizeof(ril_cmdid_t);
    strcpy(workarea->file, cur);
    cur += strlen(workarea->file) + 1;
  }
  else
  {
    workarea->cmd = _cmdid2cmd(vm, *(ril_cmdid_t*)cur);
    cur += sizeof(ril_cmdid_t);
  }
  
  return (intptr_t)cur - (intptr_t)src;
}

RIL_DELETEFUNC(return, vm)
{
  return RIL_OK;
}

RIL_FUNC(break, vm)
{
  RIL_CALLFUNC(continue)(vm);
  
  return RIL_BREAKPAIR;
}

RIL_FUNC(continue, vm)
{
  ril_tagstack_t *stack;
  
  for (;;)
  {
    stack = stack_back(vm->state->tag_stack, NULL);
    if (NULL == stack) break;
    if (vm->state->cmd.cur->parent->tag == stack->tag) break;
    stack->tag->delete_handler(vm);
    buffer_resize(vm->state->ext_buffer, stack->buffer_offset);
    stack_pop(vm->state->tag_stack, NULL);
  }
  
  vm->state->cmd.cur->pair = vm->state->cmd.cur->parent->pair;
  
  return RIL_FIRSTPAIR;
}

RIL_FUNC(while, vm)
{
  return ril_getbool(vm, 0) ? RIL_NEXT : RIL_BREAKPAIR;
}

RIL_FUNC(endwhile, vm)
{
  return RIL_FIRSTPAIR;
}

RIL_FUNC(do, vm)
{
  return RIL_NEXT;
}

RIL_FUNC(dowhile, vm)
{
  return ril_getbool(vm, 0) ? RIL_FIRSTPAIR : RIL_BREAKPAIR;
}

RIL_FUNC(foreach, vm)
{
  ril_foreach_t *workarea;
  
  if (ril_isfirst(vm))
  {
    workarea = (ril_foreach_t*)ril_mallocworkarea(vm, sizeof(ril_foreach_t));
    workarea->from = ril_getargument(vm, 0);
    if (!ril_isarray(workarea->from)) return RIL_BREAKPAIR;
    workarea->item = ril_getargument(vm, 1);
    if (ril_has(vm, 2)) workarea->key = ril_getargument(vm, 2);
    workarea->entry = hashmap_firstentry(((ril_array_t*)workarea->from->variant.ptr_value)->map);
  }
  else
  {
    workarea = (ril_foreach_t*)ril_workarea(vm);
  }
  
  if (NULL == workarea->entry)
  {
    return RIL_BREAKPAIR;
  }

  ril_copyvar(vm, workarea->item, (ril_var_t*)hashmap_getdatabyentry(workarea->entry));
  if (ril_has(vm, 2)) ril_setstring(vm, workarea->key, (char*)hashmap_getrawkeybyentry(workarea->entry));

  workarea->entry = hashmap_nextentry(workarea->entry);
  
  return RIL_NEXT;
}

RIL_FUNC(endforeach, vm)
{
  return RIL_FIRSTPAIR;
}

RIL_SAVEFUNC(foreach, vm, dest)
{
  return RIL_OK;
}

RIL_LOADFUNC(foreach, vm, src)
{
  return RIL_OK;
}

RIL_DELETEFUNC(foreach, vm)
{
  return RIL_OK;
}

RIL_FUNC(exit, vm)
{
  return RIL_EXIT;
}

RIL_FUNC(next, vm)
{
  return RIL_NEXT;
}

RIL_FUNC(const, vm)
{
  ril_getargument(vm, 0)->isconst = true;
  
  return RIL_NEXT;
}

RIL_COMPILEFUNC(include, context)
{
  RILRESULT result;
  char *buf;
  const char *file;

  file = ril_getpath(context->vm, rilc_getstring(context, 0));
  
  buf = ril_readfile(file);
  
  if (NULL == buf)
  {
    return ril_error(context->vm, "Fatal error: Cannot open %s", file);
  }
  
  rilc_eraselastcmd(context);
  
  result = rilc_compile(buf, context);
  
  ril_free(buf);
  
  return result;
}

static RIL_FUNC(stream2var, vm)
{
  const char *str, *str2;
  int length, length2;
  char *buf;
  ril_stream_t *workarea = ril_getshareddata(ril_getregisteredtag2(vm, RIL_TAG_CH));
  ril_var_t *var = (workarea->toarray) ? ril_createvar(vm, &workarea->var, NULL) : &workarea->var;
  
  if (VARIANT_STRING & var->variant.type)
  {
    str = ril_var2string(vm, var);
    length = strlen(str);
    str2 = ril_getstring(vm, 0);
    length2 = strlen(str2);
    
    buf = ril_mallocworkarea(vm, length + length2 + 1);
    memcpy(buf, str, length);
    memcpy(buf + length, str2, length2);
    buf[length + length2] = '\0';
    
    ril_setstring(vm, var, buf);
    ril_eraseworkarea(vm, length + length2 + 1);
  }
  else
  {
    ril_copyvar(vm, var, ril_getargument(vm, 0));
  }

  return RIL_NEXT;
}

static RIL_FUNC(stream2var_r, vm)
{
  ril_stream_t *workarea = ril_getshareddata(ril_getregisteredtag2(vm, RIL_TAG_CH));
  
  if (workarea->toarray) return RIL_NEXT;
  
  RIL_CALLFUNC(std_r)(vm);
  
  return RIL_NEXT;
}

void ril_pushfunction(RILVM vm, ril_tag_t *tag, RILFUNCTION executefunc, RILSAVEFUNCTION savefunc, RILLOADFUNCTION loadfunc, RILDELETEFUNCTION deletefunc, void *userdata)
{
  ril_function_t *workarea = (ril_function_t*)ril_mallocworkarea(vm, sizeof(ril_function_t));
  
  workarea->tag = tag;
  workarea->executehandler = tag->execute_handler;
  workarea->savehandler = tag->savestate_handler;
  workarea->loadhandler = tag->loadstate_handler;
  workarea->deletehandler = tag->delete_handler;
  workarea->shareddata = ril_getshareddata(tag);

  ril_setshareddata(tag, userdata);
  ril_setexecutehandler(tag, executefunc);
  ril_setstoragehandler(tag, savefunc, loadfunc, deletefunc);
  if (NULL == savefunc) ril_useworkarea(tag, false);
}

void* ril_popfunction(RILVM vm, void *src)
{
  ril_function_t *workarea = (ril_function_t*)src;

  ril_setshareddata(workarea->tag, workarea->shareddata);
  ril_setexecutehandler(workarea->tag, workarea->executehandler);
  ril_setstoragehandler(workarea->tag, workarea->savehandler, workarea->loadhandler, workarea->deletehandler);

  return workarea + 1;
}

RIL_FUNC(stream, vm)
{
  ril_tag_t *tag;
  ril_stream_t *workarea = (ril_stream_t*)ril_mallocworkarea(vm, sizeof(ril_stream_t));
  
  workarea->cmdid = ril_cmd(vm);
  workarea->toarray = ril_getbool(vm, 0);
  ril_initvar(vm, &workarea->var);

  tag = ril_getregisteredtag2(vm, RIL_TAG_CH);
  ril_pushfunction(vm, tag, RIL_CALLFUNC(stream2var), NULL, NULL, NULL, workarea);
  tag = ril_getregisteredtag2(vm, RIL_TAG_R);
  ril_pushfunction(vm, tag, RIL_CALLFUNC(stream2var_r), NULL, NULL, NULL, workarea);
  
  return RIL_NEXT;
}

RIL_FUNC(endstream, vm)
{
  ril_stream_t *workarea = (ril_stream_t*)ril_workarea(vm);

  ril_return(vm, &workarea->var);
  ril_clearvar(vm, &workarea->var);

  return RIL_BREAKPAIR;
}

RIL_SAVEFUNC(stream, vm, dest)
{
  ril_stream_t *workarea = ril_workarea(vm);
  buffer_write(dest, &workarea->cmdid, sizeof(workarea->cmdid));
  buffer_write(dest, &workarea->toarray, sizeof(workarea->toarray));
  ril_writevar(dest, &workarea->var);
  
  return RIL_OK;
}

RIL_LOADFUNC(stream, vm, src)
{
  const void* cur = src;
  ril_tag_t *tag;
  ril_stream_t *workarea = (ril_stream_t*)ril_mallocworkarea(vm, sizeof(ril_stream_t));
  cur = ril_read(&workarea->cmdid, cur, sizeof(workarea->cmdid));
  cur = ril_read(&workarea->toarray, cur, sizeof(workarea->toarray));
  
  ril_setarguments(vm, workarea->cmdid);
  
  tag = ril_getregisteredtag2(vm, RIL_TAG_CH);
  ril_pushfunction(vm, tag, RIL_CALLFUNC(stream2var), NULL, NULL, NULL, workarea);
  tag = ril_getregisteredtag2(vm, RIL_TAG_R);
  ril_pushfunction(vm, tag, RIL_CALLFUNC(stream2var_r), NULL, NULL, NULL, workarea);
  
  ril_initvar(vm, &workarea->var);
  cur = (int8_t*)cur + ril_readvar(vm, &workarea->var, cur);
  
  return (intptr_t)cur - (intptr_t)src;
}

RIL_DELETEFUNC(stream, vm)
{
  void *src =  (ril_stream_t*)ril_workarea(vm) + 1;

  ril_popfunction(vm, ril_popfunction(vm, src));

  return RIL_OK;
}

RIL_SAVEFUNC(null, vm, dest)
{
  return RIL_OK;
}

RIL_LOADFUNC(null, vm, src)
{
  return 0;
}

RIL_DELETEFUNC(null, vm)
{
  return RIL_OK;
}

RIL_SAVEFUNC(int, vm, dest)
{
  buffer_write(dest, ril_workarea(vm), sizeof(int));
  
  return RIL_OK;
}

RIL_LOADFUNC(int, vm, src)
{
  *(int*)ril_mallocworkarea(vm, sizeof(int)) = *(int*)src;
  
  return sizeof(int);
}

RIL_DELETEFUNC(int, vm)
{
  return RIL_OK;
}

RIL_COMPILEFUNC(literal, context)
{
  int valueindex;
  calc_value_t *value;
  ril_cmd_t *textcmd = NULL;
  
  if ('\r' == *context->cur) ++context->cur;
  if ('\n' == *context->cur) ++context->cur;
    
  for (;'\0' != *context->cur; ++context->cur)
  {
    if ('\n' == *context->cur)
    {
      ++context->line;
      if (NULL == rilc_addcmd(context, RIL_TAG_R)) return RIL_ERROR;
      continue;
    }
    if ('\r' == *context->cur)
    {
      if ('\n' != context->cur[1])
      {
        ++context->line;
        if (NULL == rilc_addcmd(context, RIL_TAG_R)) return RIL_ERROR;
      }
      continue;
    }
    if (ril_isleftdelimiter(context->vm, context->cur))
    {
      if (!strncmp(context->cur + context->vm->delimiter.left.length, "endliteral", 10))
      {
        break;
      }
    }
    
    if (NULL == context->cmd || textcmd != context->cmd)
    {
      textcmd = rilc_addcmd(context, RIL_TAG_CH);
      if (NULL == textcmd) return RIL_ERROR;
      rilc_addarg(context);
      calc_writeoperator(context->data_buffer, CALC_PUSH);
      valueindex = buffer_size(context->data_buffer);
      value = buffer_malloc(context->data_buffer, sizeof(calc_value_t));
      value->type = VARIANT_LITERAL | VARIANT_STRING;
      value->size = 1;
    }
    else buffer_erase(context->data_buffer, 1 + sizeof(calc_opcode_t));
    *(char*)buffer_malloc(context->data_buffer, 1) = *context->cur;
    *(char*)buffer_malloc(context->data_buffer, 1) = '\0';
    calc_writeoperator(context->data_buffer, CALC_END);
    
    value = (calc_value_t*)buffer_index(context->data_buffer, valueindex);
    ++value->size;
  }
  
  if (context->cmd->signature == RIL_TAG_R) rilc_eraselastcmd(context);
    
  return RIL_OK;
}

RIL_FUNC(count, vm)
{
  ril_var_t *var = ril_getargument(vm, 0);
  
  ril_returninteger(vm, ril_countvar(var));
  
  return RIL_NEXT;
}

RIL_FUNC(file, vm)
{
  ril_var_t var;
  const char *path = ril_getpath(vm, ril_getstring(vm, 0));
  char *buf = ril_readfile(path), *cur = buf, *begin = cur;
  bool toarray = ril_getbool(vm, 1);

  ril_initvar(vm, &var);

  if (NULL == buf)
  {
    /* error */
    return RIL_NEXT;
  }
  
  if (toarray)
  {
    for (; '\0' != *cur; ++cur)
    {
      if ('\n' == *cur)
      {
        *cur = '\0';
        ril_setstring(vm, ril_createvar(vm, &var, NULL), begin);
        begin = cur + 1;
        continue;
      }
      if ('\r' == *cur)
      {
        *cur = '\0';
        ril_createvar(vm, &var, NULL);
        if ('\n' == cur[1]) ++cur;
        begin = cur + 1;
        continue;
      }
    }
    ril_setstring(vm, ril_createvar(vm, &var, NULL), begin);
  }
  else
  {
    ril_setstring(vm, &var, buf);
  }
  
  ril_free(buf);

  ril_return(vm, &var);
  ril_clearvar(vm, &var);
  
  return RIL_NEXT;
}

RIL_FUNC(substr, vm)
{
  const char *str = ril_getstring(vm, 0), *begin;
  char *buf;
  int offset = ril_getinteger(vm, 1);
  int length = ril_getinteger(vm, 2);

  if (0 > offset)
  {
    offset += ril_mbstrlen(str);
  }
  if (0 >= length)
  {
    length += ril_mbstrlen(str);
  }

  while ('\0' != *str)
  {
    if (0 >= offset) break;
    str += mblen(str, MB_CUR_MAX);
    --offset;
  }

  begin = str;
  while ('\0' != *str)
  {
    if (0 >= length) break;
    str += mblen(str, MB_CUR_MAX);
    --length;
  }

  length = str - begin;
  buf = ril_mallocworkarea(vm, length + 1);
  memcpy(buf, begin, length);
  buf[length] = '\0';
  ril_returnstring(vm, buf);
  ril_eraseworkarea(vm, length + 1);

  return RIL_NEXT;
}

RIL_FUNC(strlen, vm)
{
  const char *str = ril_getstring(vm, 0);

  ril_returninteger(vm, ril_mbstrlen(str));

  return RIL_NEXT;
}

RIL_FUNC(strtok, vm)
{
  const char *str = ril_getstring(vm, 0);
  size_t size = strlen(str) + 1;
  char *buf = (char*)ril_malloc(size);
  const char *delimiter = ril_getstring(vm, 1);
  const char *token;
  ril_var_t var;

  ril_initvar(vm, &var);

  memcpy(buf, str, size);
  token = strtok(buf, delimiter);
  while (NULL != token)
  {
    ril_setstring(vm, ril_createvar(vm, &var, NULL), token);
    token = strtok(NULL, delimiter);
  }

  ril_return(vm, &var);
  ril_clearvar(vm, &var);
  ril_free(buf);

  return RIL_NEXT;
}

RIL_FUNC(ini, vm)
{
  ril_var_t parent, *section = &parent, *var;
  const char *file = ril_getpath(vm, ril_getstring(vm, 0));
  bool tolower = ril_getbool(vm, 1);
  char *buf = ril_readfile(file), *src = buf, *begin;
  char word[128];

  ril_initvar(vm, &parent);

  if (NULL == src)
  {
    /* error */
    return RIL_NEXT;
  }

  while ('\0' != *src)
  {
    src = ril_trimspace(src);
    /* section */
    if ('[' == *src)
    {
      src = ril_getword(word, src + 1, true);
      src = ril_trimspace(src);
      if (']' != *src)
      {
        /* error */
        ril_free(buf);
        return RIL_NEXT;
      }
      if (tolower) ril_str2lower(word, word);
      section = ril_createvar(vm, &parent, word);
      ++src;
      continue;
    }
    /* key */
    src = ril_getword(word, src, true);
    if ('\0' == *word)
    {
      src = ril_nextline(src);
      continue;
    }
    if (tolower) ril_str2lower(word, word);
    var = ril_createvar(vm, section, word);
    src = ril_trimspace(src);
    src = ril_moveto(src, '=');
    if ('=' != *src)
    {
      /* error */
      ril_free(buf);
      return RIL_NEXT;
    }
    src = ril_trimspace(src + 1);
    /* value */
    begin = src;
    src = ril_movetoeol(begin);
    word[0] = *src;
    *src = '\0';
    ril_setstring(vm, var, begin);
    *src = word[0];
    src = ril_nextline(src);
  }

  ril_free(buf);

  ril_return(vm, &parent);
  ril_clearvar(vm, &parent);

  return RIL_NEXT;
}

RIL_FUNC(readvar, vm)
{
  ril_var_t var;
  char *buf;
  const char *file = ril_getpath(vm, ril_getstring(vm, 0));

  ril_initvar(vm, &var);

  buf = ril_readfile(file);
  ril_readvar(vm, &var, buf);
  ril_free(buf);
  ril_return(vm, &var);

  ril_clearvar(vm, &var);

   return RIL_NEXT;
}

RIL_FUNC(writevar, vm)
{
  buffer_t *buffer = buffer_open(1, 1024);
  const char *file = ril_getpath(vm, ril_getstring(vm, 0));
  ril_var_t *var = ril_getargument(vm, 1);

  ril_writevar(buffer, var);
  ril_writefile(file, buffer_front(buffer), buffer_size(buffer));

  buffer_close(buffer);

   return RIL_NEXT;
}
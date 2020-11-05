/*	see copyright notice in ril.h */

#include "ril_pcheader.h"
#include "ril_vm.h"
#include "ril_state.h"
#include "ril_var.h"
#include "ril_api.h"
#include "ril_compiler.h"
#include "ril_utils.h"
#include "stack.h"

#define CALC_BUFFER_SIZE 1024

static buffer_t* _parameter_open(int size)
{
  return buffer_open(sizeof(ril_parameter_t), size);
}

static void _parameter_close(buffer_t *buffer)
{
  int i;
  ril_parameter_t *param;

  for (i = 0; i < buffer_size(buffer); ++i)
  {
    param = (ril_parameter_t*)buffer_index(buffer, i);
    buffer_close(param->valuebuffer);
  }
  buffer_close(buffer);
}

static ril_parameter_t* _parameter_add(buffer_t *buffer)
{
  ril_parameter_t *param;

  param = (ril_parameter_t*)buffer_malloc(buffer, 1);
  param->valuebuffer = buffer_open(1, 128);

  return param;
}

static void ril_errorhandler(RILVM vm, const char *msg)
{
  printf("Ril: %s\n", msg);
}

void* ril_malloc(int size)
{
  return malloc(size);
}

void* ril_realloc(void *ptr, int size)
{
  return realloc(ptr, size);
}

void ril_free(void *ptr)
{
  free(ptr);
}

RILVM ril_open(void)
{
  RILVM vm = (RILVM)ril_malloc(sizeof(ril_vm_t));
  ril_tag_t *t, *t2, *t3, *t4;
  ril_var_t *var;
  
  ril_seterrorhandler(vm, ril_errorhandler);
  
  vm->path[0] = '\0';
  vm->loadfile[0] = '\0';
  
  vm->code.hascode = false;
  vm->paircmds = NULL;

  ril_initvar(vm, &vm->globalvar);
  ril_fetchglobalvar(vm);

  vm->tagmap = hashmap_open();
  
  vm->calc = calc_open(CALC_BUFFER_SIZE);

  /* static tags */
  ril_registertag(vm, "ch", "value", RIL_CALLFUNC(std_ch));
  ril_registertag(vm, "r", NULL, RIL_CALLFUNC(std_r));
  RIL_REGISTERTAG(vm, goto, "label");
  ril_registertag(vm, "goto", "file", RIL_CALLFUNC(gotofile));
  RIL_REGISTERTAG(vm, gosub, "label");
  ril_registertag(vm, "gosub", "file", RIL_CALLFUNC(gosubfile));
  RIL_REGISTERTAG(vm, exit, NULL);
  RIL_REGISTERTAG(vm, label, "value");
  t = RIL_REGISTERTAG(vm, return, "value = null");
  RIL_SETSTORAGE(t, return);
  ril_useworkarea(t, false);
   /* static tags */
  
  ril_registertag(vm, "goto", "file, label", RIL_CALLFUNC(gotofile));
  ril_registertag(vm, "gosub", "file, label", RIL_CALLFUNC(gosubfile));
  RIL_REGISTERTAG(vm, set, "&var");
  RIL_REGISTERTAG(vm, unset, "&var");
  RIL_REGISTERTAG(vm, isnull, "&var");
  RIL_REGISTERTAG(vm, isint, "&var");
  ril_registertag(vm, "isinteger", "&var", RIL_CALLFUNC(isint));
  RIL_REGISTERTAG(vm, isreal, "&var");
  RIL_REGISTERTAG(vm, isarray, "&var");
  RIL_REGISTERTAG(vm, isstring, "&var");
  RIL_REGISTERTAG(vm, let, "value");
  RIL_REGISTERTAG(vm, const, "value");
  t = RIL_REGISTERTAG(vm, if, "value");
  t2 = RIL_REGISTERTAG(vm, elseif, "value");
  t3 = RIL_REGISTERTAG(vm, else, NULL);
  t4 = RIL_REGISTERTAG(vm, endif, NULL);
  ril_setpairtag(t, t2);
  ril_setpairtag(t, t3);
  ril_setpairtag(t, t4);
  ril_setpairtag(t2, t2);
  ril_setpairtag(t2, t3);
  ril_setpairtag(t2, t4);
  ril_setpairtag(t3, t4);
  RIL_SETSTORAGE(t, int);
  
  t2 = RIL_REGISTERTAG(vm, endmacro, NULL);
  t = RIL_REGISTERTAG(vm, macro, "name, params = \"\", vars = \"\"");
  RIL_SETCOMPILEHANDLER(t, macro);
  RIL_SETSTORAGE(t, null);
  ril_setpairtag(t, t2);
  
  t3 = RIL_REGISTERTAG(vm, break, NULL);
  t4 = RIL_REGISTERTAG(vm, continue, NULL);
  
  t = RIL_REGISTERTAG(vm, while, "value");
  t2 = RIL_REGISTERTAG(vm, endwhile, NULL);
  ril_setpairtag(t, t2);
  ril_setchildtag(t, t3);
  ril_setchildtag(t, t4);
  RIL_SETSTORAGE(t, null);
  
  t = RIL_REGISTERTAG(vm, do, NULL);
  t2 = RIL_REGISTERTAG(vm, dowhile, "value");
  ril_setpairtag(t, t2);
  ril_setchildtag(t, t3);
  ril_setchildtag(t, t4);
  RIL_SETSTORAGE(t, null);
  
  t = RIL_REGISTERTAG(vm, foreach, "&from, &item, &key");
  t2 = RIL_REGISTERTAG(vm, endforeach, NULL);
  ril_setpairtag(t, t2);
  ril_setchildtag(t, t3);
  ril_setchildtag(t, t4);
  RIL_SETSTORAGE(t, foreach);
  t = RIL_REGISTERTAG(vm, foreach, "&from, &item");
  ril_setpairtag(t, t2);
  ril_setchildtag(t, t3);
  ril_setchildtag(t, t4);
  RIL_SETSTORAGE(t, foreach);
  
  t = ril_registertag(vm, "include", "file", NULL);
  RIL_SETCOMPILEHANDLER(t, include);
  
  t = RIL_REGISTERTAG(vm, stream, "toarray = FALSE");
  t2 = RIL_REGISTERTAG(vm, endstream, NULL);
  ril_setpairtag(t, t2);
  RIL_SETSTORAGE(t, stream);
  
  t = ril_registertag(vm, "literal", NULL, NULL);
  t2 = ril_registertag(vm, "endliteral", NULL, NULL);
  RIL_SETCOMPILEHANDLER(t, literal);
  ril_setpairtag(t, t2);
  
  RIL_REGISTERTAG(vm, count, "var");
  RIL_REGISTERTAG(vm, file, "file, toarray = FALSE");
  RIL_REGISTERTAG(vm, substr, "src, offset = 0, length = 0");
  RIL_REGISTERTAG(vm, strlen, "src");
  RIL_REGISTERTAG(vm, strtok, "src, delimiter");
  RIL_REGISTERTAG(vm, ini, "file, tolower = true");
  RIL_REGISTERTAG(vm, writevar, "file, var");
  RIL_REGISTERTAG(vm, readvar, "file");

  vm->mainstate = ril_newstate(vm);
  ril_setmainstate(vm);

  var = ril_createvar(vm, NULL, "TRUE");
  ril_setinteger(vm, var, 1);
  ril_setconst(var);
  var = ril_createvar(vm, NULL, "true");
  ril_setinteger(vm, var, 1);
  ril_setconst(var);
  
  var = ril_createvar(vm, NULL, "YES");
  ril_setinteger(vm, var, 1);
  ril_setconst(var);
  var = ril_createvar(vm, NULL, "yes");
  ril_setinteger(vm, var, 1);
  ril_setconst(var);
  
  var = ril_createvar(vm, NULL, "FALSE");
  ril_setinteger(vm, var, 0);
  ril_setconst(var);
  var = ril_createvar(vm, NULL, "false");
  ril_setinteger(vm, var, 0);
  ril_setconst(var);
  
  var = ril_createvar(vm, NULL, "NO");
  ril_setinteger(vm, var, 0);
  ril_setconst(var);
  var = ril_createvar(vm, NULL, "no");
  ril_setinteger(vm, var, 0);
  ril_setconst(var);
  
  ril_setdelimiter(vm, "[", "]");
  
  return vm;
}

void ril_close(RILVM vm)
{
  ril_deletestate(vm->mainstate);
  ril_freecode(vm);
  ril_free(vm->paircmds);
  calc_close(vm->calc);
  ril_clearvar(vm, &vm->globalvar);
  ril_deletetags(vm);
  ril_free(vm);
}

void ril_setfilename(RILVM vm, const char *file)
{
  strcpy(vm->loadfile, file);
}

void ril_cleartags(RILVM vm)
{
  hashmap_entry_t *entry = hashmap_firstentry(vm->tagmap);
  for (; NULL != entry; entry = hashmap_nextentry(entry))
  {
    ril_deletetag((ril_tag_t*)hashmap_getdatabyentry(entry));
  }
  hashmap_clear(vm->tagmap);
}

void ril_deletetags(RILVM vm)
{
  ril_cleartags(vm);
  hashmap_close(vm->tagmap);
}

void ril_deletemacros(RILVM vm)
{
  hashmap_entry_t *entry = hashmap_firstentry(vm->tagmap);
  for (; NULL != entry; entry = hashmap_nextentry(entry))
  {
    ril_tag_t *tag = (ril_tag_t*)hashmap_getdatabyentry(entry);
    if (tag->execute_handler != RIL_CALLFUNC(callmacro)) continue;
    hashmap_delete(vm->tagmap, hashmap_getkeybyentry(entry));
    ril_deletetag(tag);
  }
}

ril_tag_t* ril_createtag(RILVM vm, ril_signature_t signature)
{
  ril_tag_t *tag = ril_getregisteredtag2(vm, signature);

  if (NULL == tag)
  {
    tag = (ril_tag_t*)ril_malloc(sizeof(ril_tag_t));
    ril_inittag(tag);
    tag->signature = signature;
    hashmap_add(vm->tagmap, tag->signature, NULL, tag);
  }

  return tag;
}

void ril_inittag(ril_tag_t *t)
{
  t->name[0] = '\0';
  t->hasparent = false;
  t->refcount = 0;
  t->param_buffer = _parameter_open(5);
  t->pair_buffer = buffer_open(sizeof(ril_pairtag_t), 5);
  t->child_buffer = buffer_open(sizeof(ril_childtag_t), 5);
  ril_useworkarea(t, false);
  ril_setexecutehandler(t, NULL);
  ril_setcompilehandler(t, NULL);
  ril_setstoragehandler(t, NULL, NULL, NULL);
}

void ril_deletetag(ril_tag_t *t)
{
  _parameter_close(t->param_buffer);
  buffer_close(t->pair_buffer);
  buffer_close(t->child_buffer);

  ril_free(t);
}

const char* ril_tagname(ril_tag_t *tag)
{
  return tag->name;
}

ril_signature_t ril_signature(ril_tag_t *tag)
{
  return tag->signature;
}

void ril_setpath(RILVM vm, const char *path)
{
  int length = strlen(path);
  
  strcpy(vm->path, path);
  
  if ('/' != path[length - 1])
  {
    vm->path[length] = '/';
    vm->path[length + 1] = '\0';
  }
}

RILRESULT ril_setdelimiter(RILVM vm, const char *left, const char *right)
{
  int leftlegnth = strlen(left), rightlength = strlen(right);
  
  if (!leftlegnth) return RIL_ERROR;
  if (RIL_DELIMITER_LENGTH <= leftlegnth) return RIL_ERROR;
  
  if (!rightlength) return RIL_ERROR;
  if (RIL_DELIMITER_LENGTH <= rightlength) return RIL_ERROR;
  
  memcpy(vm->delimiter.left.string, left, leftlegnth + 1);
  vm->delimiter.left.length = leftlegnth;
  
  memcpy(vm->delimiter.right.string, right, rightlength + 1);
  vm->delimiter.right.length = rightlength;
  
  return RIL_OK;
}

bool ril_isleftdelimiter(RILVM vm, const char *src)
{
  return !strncmp(src, vm->delimiter.left.string, vm->delimiter.left.length);
}

bool ril_isrightdelimiter(RILVM vm, const char *src)
{
  return !strncmp(src, vm->delimiter.right.string, vm->delimiter.right.length);
}

int parameter_sort(const void *a , const void *b)
{
  const ril_parameter_t *ap = (ril_parameter_t*)a;
  const ril_parameter_t *bp = (ril_parameter_t*)b;

  if (ap->namehash < bp->namehash) return -1;
  if (ap->namehash == bp->namehash) return 0;
  return 1;
}

const char* ril_getparametername(const buffer_t *buffer, int index)
{
  ril_parameter_t *param = (ril_parameter_t*)buffer_index(buffer, index);
  
  return param->name;
}

const char* ril_getparametervalue(const buffer_t *buffer, int index)
{
  ril_parameter_t *param = (ril_parameter_t*)buffer_index(buffer, index);

  if (buffer_empty(param->valuebuffer)) return "";
  
  return (char*)buffer_front(param->valuebuffer);
}

RILRESULT ril_parseparameters(buffer_t *buffer, const char *args)
{
  ril_parameter_t *param;
  const char *begin;
  
  if (NULL == args) return RIL_OK;
  
  buffer_setblocksize(buffer, sizeof(ril_parameter_t));
  
  for (; '\0' != *args; ++args)
  {
    param = _parameter_add(buffer);
    param->isrefvar = false;
    // name
    args = ril_trimspace(args);
    if ('&' == *args)
    {
      param->isrefvar = true;
      ++args;
    }
    args = ril_getword(param->name, args, false);
    if ('\0' == param->name[0]) return RIL_ERROR;
    param->namehash = ril_makecrc(param->name);

    args = ril_trimspace(args);
    if ('\0' == *args) break;
    if (',' == *args) continue;
    if ('=' != *args) return RIL_ERROR;
    ++args;

    // value
    begin = args = ril_trimspace(args);
    do
    {
      args = ril_moveto(args, ',');
      if ('\0' == *args) break;
    } while('\\' == args[-1]);
    buffer_write(param->valuebuffer, begin, args - begin);
    *(char*)buffer_malloc(param->valuebuffer, 1) = '\0';

    if (buffer_empty(param->valuebuffer)) return RIL_ERROR;
    if ('\0' == *args) break;
    if (',' != *args) return RIL_ERROR;
  }
  
  return RIL_OK;
}

static __inline RILRESULT _addparameters(RILVM vm, ril_tag_t *tag, buffer_t *buffer)
{
  ril_parameter_t *src, *dest;
  int i;

  for (i = 0; i < buffer_size(buffer); ++i)
  {
    src = (ril_parameter_t*)buffer_index(buffer, i);
    dest =  _parameter_add(tag->param_buffer);
    dest->isrefvar = src->isrefvar;
    dest->namehash = src->namehash;
    strcpy(dest->name, src->name);
    buffer_write(dest->valuebuffer, buffer_front(src->valuebuffer), buffer_size(src->valuebuffer));
  }

  return RIL_OK;
}

ril_signature_t ril_makesignature(const char *name, const char *args)
{
  ril_signature_t signature;
  buffer_t *parameter = _parameter_open(16);

  if (RIL_FAILED(ril_parseparameters(parameter, args)))
  {
    buffer_close(parameter);
    return 0;
  }
  signature = ril_makesignature2(name, parameter);
  _parameter_close(parameter);

  return signature;
}

ril_signature_t ril_makesignature2(const char *name, const buffer_t *parameter)
{
  ril_signature_t signature;
  buffer_t *buffer = buffer_open(1, 512);

  ril_makesignaturestring(buffer, name, parameter);
  signature = ril_makecrc((char*)buffer_front(buffer));

  buffer_close(buffer);

  return signature;
}

void ril_makesignaturestring(buffer_t *dest, const char *name, const buffer_t *parameter)
{
  int i;

  buffer_write(dest, name, strlen(name));
  if (0 < buffer_size(parameter)) *(char*)buffer_malloc(dest, 1) = ' ';

  qsort(buffer_front(parameter), buffer_size(parameter), sizeof(ril_parameter_t), parameter_sort);
  for (i = 0; i < buffer_size(parameter); ++i)
  {
    const char *name = ril_getparametername(parameter, i);
    buffer_write(dest, name, strlen(name));
    *(char*)buffer_malloc(dest, 1) = ':';
  }
  *(char*)buffer_malloc(dest, 1) = '\0';
}

ril_tag_t* ril_registertag(RILVM vm, const char *name, const char *args, RILFUNCTION func)
{
  ril_tag_t *t;
  buffer_t *parameter = _parameter_open(16);
  buffer_t *signature = buffer_open(1, 512);

  ril_parseparameters(parameter, args);
  ril_makesignaturestring(signature, name, parameter);

  t = ril_createtag(vm, ril_makecrc((char*)buffer_front(signature)));

  if ('\0' == t->name[0])
  {
    strcpy(t->name, name);
    buffer_clear(parameter);
    ril_parseparameters(parameter, args);
    if (RIL_FAILED(_addparameters(vm, t, parameter)))
    {
        ril_deletetag(t);
        _parameter_close(parameter);
        buffer_close(signature);
        return NULL;
    }
  }

  ril_setexecutehandler(t, func);

  _parameter_close(parameter);
  buffer_close(signature);

  return t;
}

ril_tag_t* ril_getregisteredtag(RILVM vm, const char *name, const char *args)
{
  return ril_getregisteredtag2(vm, ril_makesignature(name, args));
}

ril_tag_t* ril_getregisteredtag2(RILVM vm, ril_signature_t signature)
{
  return (ril_tag_t*)hashmap_getdata(vm->tagmap, signature);
}

ril_tag_t* ril_getregisteredtag3(RILVM vm, const char *name, const char *args, bool hasnonamearg, bool iscmp)
{
  int j, k, hitnum;
  buffer_t *parameter;
  hashmap_entry_t *entry;
  
  parameter = _parameter_open(16);
  ril_parseparameters(parameter, args);
  
  entry = hashmap_firstentry(vm->tagmap);
  for (; NULL != entry; entry = hashmap_nextentry(entry))
  {
    ril_tag_t *tag = (ril_tag_t*)hashmap_getdatabyentry(entry);
    int argsize = buffer_size(tag->param_buffer);
    
    if (iscmp)
    {
      if (buffer_size(parameter) + hasnonamearg != argsize) continue;
    }
    else
    {
      if (buffer_size(parameter) + hasnonamearg > argsize) continue;
    }
    if (strcmp(tag->name, name)) continue;
    
    hitnum = 0;
    for (j = hasnonamearg; j < argsize; ++j)
    {
      ril_parameter_t *arg = (ril_parameter_t*)buffer_index(tag->param_buffer, j);
      for (k = buffer_size(parameter) - 1; 0 <= k; --k)
      {
        if (arg->namehash == ((ril_parameter_t*)buffer_index(parameter, k))->namehash) break;
      }
      if (0 > k)
      {
        if (buffer_empty(arg->valuebuffer)) break;
        continue;
      }
      else ++hitnum;
    }
    if (j == argsize && hitnum == buffer_size(parameter))
    {
      _parameter_close(parameter);
      return tag;
    }
  }

  _parameter_close(parameter);
  
  return NULL;
}

RILFUNCTION ril_getexecutehandler(ril_tag_t *tag)
{
  return tag->execute_handler;
}

void ril_setexecutehandler(ril_tag_t* t, RILFUNCTION handler)
{
  t->execute_handler = NULL == handler ? RIL_CALLFUNC(next) : handler;
}

void ril_setcompilehandler(ril_tag_t* t, RILCOMPILEFUNCTION handler)
{
  t->compile_handler = handler;
}

void ril_setstoragehandler(ril_tag_t* t, RILSAVEFUNCTION savehandler, RILLOADFUNCTION loadhandler, RILDELETEFUNCTION deletehandler)
{
  t->savestate_handler = savehandler;
  t->loadstate_handler = loadhandler;
  t->delete_handler = deletehandler;

  if (NULL != savehandler)
  {
    ril_useworkarea(t, true);
  }
}

void ril_useworkarea(ril_tag_t *t, bool value)
{
  t->addstack = value;
}

ril_tag_t* ril_setpairtag(ril_tag_t *t, ril_tag_t *pair_t)
{
  ril_pairtag_t *pairtag = buffer_malloc(t->pair_buffer, 1);
  pairtag->tag = pair_t;
  ++pair_t->refcount;
  
  return t;
}

ril_tag_t* ril_setchildtag(ril_tag_t *parent, ril_tag_t *child)
{
  ril_childtag_t *childtag = (ril_childtag_t*)buffer_malloc(parent->child_buffer, 1);
  childtag->tag = child;
  child->hasparent = true;
  
  return parent;
}

void ril_ch(RILVM vm, ril_var_t *var)
{
  ril_state_t *state = ril_getstate(vm);
  ril_setstate(ril_newstate(vm));
  ril_copyvar(vm, ril_getargument(vm, 0), var);
  ril_calltag(vm, (ril_getregisteredtag2(vm, RIL_TAG_CH)));
  ril_deletestate(ril_getstate(vm));
  ril_setstate(state);
}

const ril_label_t* ril_getlabel(RILVM vm, ril_crc_t namehash)
{
  int i;
  const ril_label_t *label_cur;
  
  for (i = vm->code.common->label_size - 1; 0 <= i; --i)
  {
    label_cur = &vm->code.label[i];
    if (namehash == label_cur->namehash)
    {
      return label_cur;
    }
  }
  
  return NULL;
}

RILRESULT ril_gotolabel(RILVM vm, const char *name)
{
  return ril_gotolabelbyhash(vm, ril_makecrc(name));
}

RILRESULT ril_gotolabelbyhash(RILVM vm, ril_crc_t hash)
{
  const ril_label_t *label = ril_getlabel(vm, hash);
  
  if (NULL == label) return RIL_ERROR;
  
  ril_cleararguments(vm->state);
  ril_setinteger(vm, ril_getargument(vm, 0), label->cmdid);
  ril_getregisteredtag2(vm, RIL_TAG_GOTO)->execute_handler(vm);
  
  return RIL_OK;
}

RILRESULT ril_goto(RILVM vm, ril_cmdid_t cmdid)
{
  vm->state->cmd.next = _cmdid2cmd(vm, cmdid);
  
  return RIL_OK;
}

void ril_setuserdata(RILVM vm, void *userdata)
{
  vm->userdata = userdata;
}

void* ril_userdata(RILVM vm)
{
  return vm->userdata;
}

bool ril_isfirst(RILVM vm)
{
  return vm->state->isfirst;
}

ril_cmdid_t ril_cmd(RILVM vm)
{
  return _cmd2cmdid(vm, vm->state->cmd.cur);
}

void ril_nextcmd(RILVM vm)
{
  vm->state->cmd.next = ++vm->state->cmd.cur;
}

void ril_prevcmd(RILVM vm)
{
  vm->state->cmd.next = --vm->state->cmd.cur;
}

void ril_setnextcmd(RILVM vm, ril_vmcmd_t *cmd)
{
  vm->state->cmd.next = cmd;
}

ril_tag_t* ril_currenttag(RILVM vm)
{
  return vm->state->cmd.cur->tag;
}

void ril_setnextcmdbyid(RILVM vm, ril_cmdid_t cmdid)
{
  ril_setnextcmd(vm, _cmdid2cmd(vm, cmdid));
}

ril_tag_t* ril_cmdid2tag(RILVM vm, ril_cmdid_t cmdid)
{
  return _cmdid2cmd(vm, cmdid)->tag;
}

ril_signature_t ril_cmdid2signature(RILVM vm, ril_cmdid_t cmdid)
{
  return _cmdid2cmd(vm, cmdid)->signature;
}

static __inline int _docmd(RILVM vm, ril_vmcmd_t *cmd)
{
  int result;

  vm->state->cmd.cur = cmd;

  if (NULL != vm->state->cmd.prev)
  {
    vm->state->isfirst = cmd->pair != vm->state->cmd.prev->pair;
  }
  else
  {
    vm->state->isfirst = true;
  }

  ril_setargumentsbycmd(vm, cmd);
  
  if (vm->state->isfirst)
  {
    if (cmd->tag->addstack)
    {
      ril_addstack(vm, cmd->tag);
    }
  }
  
  result = cmd->tag->execute_handler(vm);

  if (RIL_BREAKPAIR == result && cmd->pair->first->tag->addstack)
  {
    ril_releaseworkarea(vm, cmd->pair->first->tag);
  }
  
  vm->state->cmd.prev = cmd;
  
  return result;
}

static __inline bool _update2result(RILVM vm, int result)
{
  switch (result)
  {
  case RIL_STOP:
    return true;
  case RIL_EXIT:
    return true;
  case RIL_NEXT:
    ++vm->state->cmd.next;
    break;
  case RIL_NEXTPAIR:
    vm->state->cmd.next = vm->state->cmd.cur->nextpair;
    break;
  case RIL_BREAKPAIR:
    vm->state->cmd.next = vm->state->cmd.cur->pair->last + 1;
    break;
  case RIL_FIRSTPAIR:
    vm->state->cmd.next = vm->state->cmd.cur->pair->first;
    break;
  case RIL_LASTPAIR:
    vm->state->cmd.next = vm->state->cmd.cur->pair->last;
    break;
  }
  
  return false;
}

int ril_docmd(RILVM vm, ril_cmdid_t cmd)
{
  return _docmd(vm, _cmdid2cmd(vm, cmd));
}

int ril_execute(RILVM vm)
{
  int result;
  
  for (;;)
  {
    result = _docmd(vm, vm->state->cmd.next);
    if (RIL_FAILED(result)) return result;
    if (_update2result(vm, result)) return result;
  }
  
  return result;
}

int ril_calltag(RILVM vm, ril_tag_t *tag)
{
  ril_vmcmd_t *cmd = vm->state->tmpcmd;
  
  ril_clearstate(ril_getstate(vm));

  cmd->tag = tag;
  cmd->nextpair = NULL;
  cmd->parent = vm->state->cmd.cur;
  cmd->arg = NULL;
  
  vm->state->cmd.cur = cmd;
  
  return ril_getexecutehandler(tag)(vm);
}

int ril_callmacro(RILVM vm, ril_tag_t *tag)
{
  int result = ril_calltag(vm, tag);
  
  if (RIL_FAILED(result)) return result;
  if (_update2result(vm, result)) return result;
  
  return ril_execute(vm);
}

void ril_seterrorhandler(RILVM vm, RILERROR func)
{
  vm->error_hander = func;
}

void* ril_mallocworkarea(RILVM vm, int size)
{
  return buffer_malloc(vm->state->ext_buffer, size);
}

void ril_eraseworkarea(RILVM vm, int size)
{
  buffer_erase(vm->state->ext_buffer, size);
}

void* ril_workarea(RILVM vm)
{
  ril_tagstack_t *stack = (ril_tagstack_t*)stack_back(vm->state->tag_stack, NULL);
  return buffer_index(vm->state->ext_buffer, stack->buffer_offset);
}

void ril_releaseworkarea(RILVM vm, ril_tag_t *tag)
{
  ril_tagstack_t *stack;
  
  for (;;)
  {
    stack = (ril_tagstack_t*)stack_back(vm->state->tag_stack, NULL);
    if (NULL == stack) break;
    if (NULL != stack->tag->delete_handler) stack->tag->delete_handler(vm);
    buffer_resize(vm->state->ext_buffer, stack->buffer_offset);
    stack_pop(vm->state->tag_stack, NULL);
    if (tag == stack->tag) break;
  }
}

void ril_setshareddata(ril_tag_t *tag, void *data)
{
  tag->userdata = data;
}

void* ril_getshareddata(ril_tag_t *tag)
{
  return tag->userdata;
}

void ril_addstack(RILVM vm, ril_tag_t *tag)
{
  ril_tagstack_t *stack = (ril_tagstack_t*)stack_push(vm->state->tag_stack, NULL);
  
  stack->tag = tag;
  stack->buffer_offset = buffer_size(vm->state->ext_buffer);
}

RILRESULT ril_fetchlocalvar(RILVM vm)
{
  vm->rootvar = &vm->state->rootvar;

  return RIL_OK;
}

void ril_fetchglobalvar(RILVM vm)
{
  vm->rootvar = &vm->globalvar;
}

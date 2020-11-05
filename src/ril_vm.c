/*	see copyright notice in ril.h */

#include "ril_pcheader.h"
#include "ril_vm.h"
#include "ril_state.h"
#include "ril_var.h"
#include "ril_api.h"
#include "ril_utils.h"
#include "md5.h"

void ril_parsecode(ril_code_t *code, const void *src)
{
  code->common = (ril_common_header_t*)src;
  code->label = (ril_label_t*)((int8_t*)src + code->common->label_offset);
  code->cmd = (ril_cmd_t*)((int8_t*)src + code->common->cmd_offset);
  code->arg = (ril_arg_t*)((int8_t*)src + code->common->arg_offset);
  code->data = (void*)((int8_t*)src + code->common->data_offset);
}

void ril_freecode(RILVM vm)
{
  if (!vm->code.hascode) return;
  
  ril_free(vm->code.common);
  ril_free(vm->code.label);
  ril_free(vm->code.cmd);
  ril_free(vm->code.arg);
  ril_free(vm->code.data);
  
  vm->code.hascode = false;

  ril_deletemacros(vm);
}

static __inline RILRESULT _setpaircmd(RILVM vm)
{
  int i, k = 0;
  ril_vmcmd_t *cmd;
  
  vm->paircmds = ril_realloc(vm->paircmds, sizeof(ril_paircmd_t) * vm->code.common->cmd_size);

  for (i = 0, cmd = vm->code.cmd; i < vm->code.common->cmd_size; ++i, ++cmd)
  {
    if (NULL != cmd->pair) continue;
    cmd->pair = &vm->paircmds[k];
    cmd->pair->first = cmd;
    cmd->pair->last = cmd;
    while (cmd != cmd->pair->last->nextpair)
    {
      cmd->pair->last = cmd->pair->last->nextpair;
      cmd->pair->last->pair = cmd->pair;
    }
    ++k;
  }
  
  return RIL_OK;
}

static __inline RILRESULT _copycode(RILVM vm, ril_code_t *code, int codesize)
{
  int i;
  ril_vmcmd_t *cmd;
  ril_vmarg_t *arg;
  
  vm->code.common = ril_malloc(sizeof(ril_common_header_t));
  memcpy(vm->code.common, code->common, sizeof(ril_common_header_t));
  
  vm->code.label = ril_malloc(sizeof(ril_label_t) * code->common->label_size);
  memcpy(vm->code.label, code->label, sizeof(ril_label_t) * code->common->label_size);
  
  vm->code.data = ril_malloc(codesize - code->common->data_offset);
  memcpy(vm->code.data, code->data, codesize - code->common->data_offset);
  
  vm->code.arg = ril_malloc(sizeof(ril_vmarg_t) * code->common->arg_size);
  for (i = 0; i < code->common->arg_size; ++i, ++code->arg)
  {
    arg = &vm->code.arg[i];
    arg->data = (int8_t*)vm->code.data + code->arg->data_offset;
  }
  
  cmd = vm->code.cmd = ril_malloc(sizeof(ril_vmcmd_t) * code->common->cmd_size);
  for (i = 0; i < code->common->cmd_size; ++i, ++cmd)
  {
    cmd->signature = code->cmd[i].signature;
    cmd->tag = ril_createtag(vm, cmd->signature);
    cmd->arg = &vm->code.arg[code->cmd[i].arg_offset];
    cmd->nextpair = &vm->code.cmd[code->cmd[i].pair_cmdid.id];
    cmd->parent = &vm->code.cmd[code->cmd[i].parent_cmdid.id];
    cmd->pair = NULL;
  }
  
  vm->code.hascode = true;
  
  return RIL_OK;
}

RILRESULT ril_load(RILVM vm, const void *src, int size)
{
  int i;
  ril_code_t code;
  md5_state_t md5state;
  ril_crc_t *md5tags;
  
  ril_freecode(vm);
  ril_parsecode(&code, src);
  
  if (code.common->endian != ril_endian())
  {
    return ril_error(vm, "bad endian");
  }

  if (RIL_FAILED(_copycode(vm, &code, size))) return RIL_ERROR;
  
  vm->loadfile[0] = '\0';
  vm->state->cmd.next = vm->code.cmd;
  
  if (RIL_FAILED(_setpaircmd(vm))) return RIL_ERROR;
  
  // md5
  md5tags = ril_malloc(vm->code.common->cmd_size * sizeof(ril_crc_t));
  for (i = vm->code.common->cmd_size - 1; 0 <= i; --i) md5tags[i] = vm->code.cmd[i].signature;
  md5_init(&md5state);
  md5_append(&md5state, (uint8_t*)md5tags, vm->code.common->cmd_size * sizeof(ril_crc_t));
  md5_finish(&md5state, vm->hash.buf);
  ril_free(md5tags);
  
  return RIL_OK;
}

RILRESULT ril_loadfile(RILVM vm, const char *file)
{
  int result;
  
  ril_cleararguments(vm->state);
  ril_setstring(vm, ril_getargument(vm, 0), file);
  
  result = ril_getexecutehandler(ril_getregisteredtag(vm, "goto", "file"))(vm);
  
  return RIL_FAILED(result) ? RIL_ERROR : RIL_OK;
}

RILRESULT ril_loadbytefile(RILVM vm, const char *file)
{
  RILRESULT result;
  char *buf;
  const char *path = ril_getpath(vm, file);
  
  buf = ril_readfile(path);
  if (NULL == buf)
  {
    return ril_error(vm, "Fatal error: Cannot open %s", path);
  }
  
  result = ril_load(vm, buf, strlen(buf));
  
  ril_free(buf);
  
  if (RIL_SUCCEEDED((result)))
  {
    ril_setfilename(vm, file);
  }
  
  return result;
}

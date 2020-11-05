/*	see copyright notice in ril.h */

#ifndef _RIL_COMPILER_H_
#define _RIL_COMPILER_H_

#define TAG_TYPE_BUFF_SIZE 64
#define LABEL_BUFF_SIZE 128
#define TAG_BUFF_SIZE 1024
#define ARG_BUFF_SIZE TAG_BUFF_SIZE * 4
#define DATA_BUFF_SIZE ARG_BUFF_SIZE * 256

#define RIL_COMPILE_ERROR(context, s, ...) \
ril_error(context->vm, s " on line %d", ##__VA_ARGS__, context->line);

struct _ril_compile
{
  uint32_t line;
  const char *cur;
  RILVM vm;
  char calc_error[256];
  char tagname[128];
  ril_tag_t *tag;
  ril_cmd_t *cmd;
  ril_cmdid_t cmdid;
  stack_t *pair_stack;
  buffer_t *cmd_buffer;
  buffer_t *arg_buffer;
  buffer_t *data_buffer;
  buffer_t *label_buffer;
  buffer_t *var_buffer;
};

#ifdef __cplusplus
extern "C" {
#endif

void ril_calcerrorhandler(calc_t *calc, const char *msg);
const void* rilc_getargument(ril_compile_t *context, uint32_t argid);
ril_cmd_t* rilc_newcmd(ril_compile_t *context, ril_signature_t signature);
ril_cmd_t* rilc_addcmd(ril_compile_t *context, ril_signature_t signature);
RILRESULT rilc_checkpair(ril_compile_t *context, ril_cmd_t *cmd);
RILRESULT rilc_checkchild(ril_compile_t *context, ril_cmd_t *cmd);

RILRESULT calc_cb_compile(calc_compile_t *context, ril_compile_t *c_context);

#ifdef __cplusplus
}
#endif

#endif

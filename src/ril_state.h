/*	see copyright notice in ril.h */

#ifndef _RIL_STATE_H_
#define _RIL_STATE_H_

#define RIL_ARGUMENT_SIZE 32
#define STACK_BUFFER_SIZE 1024
#define EXT_BUFFER_SIZE 1024

struct _ril_state
{
  RILVM vm;
  struct
  {
    ril_vmcmd_t *cur;
    ril_vmcmd_t *prev;
    ril_vmcmd_t *next;
  } cmd;
  uint8_t isfirst;
  ril_crc_t lastlabel;

  int argc;
  ril_register_t args[RIL_ARGUMENT_SIZE];

  ril_var_t rootvar, *returnvar;
  buffer_t *varbuffer;
  buffer_t *ext_buffer;
  stack_t *tag_stack;
  ril_vmcmd_t tmpcmd[2];
};

#endif

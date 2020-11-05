/*	see copyright notice in ril.h */

#ifndef _RIL_VM_H_
#define _RIL_VM_H_

#define RIL_DELIMITER_LENGTH 128

#define LABEL_NULL 0x80000000

enum
{
  VAR_CALC,
  VAR_HASH,
  VAR_ADD,
  VAR_END
};

typedef struct
{
  ril_crc_t namehash;
  char name[128];
  buffer_t *valuebuffer;
  bool isrefvar;
} ril_parameter_t;

typedef struct
{
  ril_tag_t *tag;
} ril_pairtag_t;

typedef struct
{
  ril_tag_t *tag;
} ril_childtag_t;

struct _ril_tag
{
  char name[128];
  ril_signature_t signature;
  buffer_t *param_buffer;
  buffer_t *pair_buffer;
  buffer_t *child_buffer;
  RILFUNCTION execute_handler;
  RILRESULT (*volatile compile_handler)(ril_compile_t*);
  RILSAVEFUNCTION savestate_handler;
  RILLOADFUNCTION loadstate_handler;
  RILDELETEFUNCTION delete_handler;
  void *userdata;
  int refcount;
  bool hasparent;
  bool addstack;
};

struct _ril_label
{
  uint32_t namehash;
  union
  {
    int32_t cmdid;
    int32_t id;
  };
};

typedef struct
{
  ril_signature_t signature;
  ril_cmdid_t pair_cmdid;
  ril_cmdid_t parent_cmdid;
  uint32_t arg_offset;
} ril_cmd_t;

typedef struct
{
  uint32_t data_offset;
} ril_arg_t;

typedef struct
{
  uint8_t endian;
  int32_t cmd_size;
  int32_t arg_size;
  int32_t label_size;
  
  uint32_t label_offset;
  uint32_t cmd_offset;
  uint32_t arg_offset;
  uint32_t data_offset;
} ril_common_header_t;

typedef struct
{
  void *data;
} ril_vmarg_t;

struct _ril_vmcmd;
typedef struct _ril_vmcmd ril_vmcmd_t;

typedef struct
{
  ril_vmcmd_t *first;
  ril_vmcmd_t *last;
} ril_paircmd_t;

struct _ril_vmcmd {
  ril_tag_t *tag;
  ril_signature_t signature;
  ril_vmarg_t *arg;
  ril_vmcmd_t *nextpair;
  ril_paircmd_t *pair;
  ril_vmcmd_t *parent;
};

struct _ril_code
{
  const ril_common_header_t *common;
  const ril_label_t  *label;
  const ril_cmd_t *cmd;
  const ril_arg_t  *arg;
  const void *data;
};

struct _ril_var
{
  int isconst;
  int refcount;
  variant_t variant;
};

typedef struct
{
  hashmap_t *map;
  int refcount;
  int nextnum;
} ril_array_t;

typedef struct
{
  char *ptr;
  uint32_t size;
  int refcount;
} ril_string_t;

typedef struct
{
  ril_tag_t *tag;
  int buffer_offset;
} ril_tagstack_t;

struct _ril_register
{
  ril_var_t *parent;
  ril_var_t *var;
  hashmap_key_t  hashkey;
  ril_var_t temp;
};

struct _ril_vm
{
  char path[256];
  char loadfile[512];
  calc_t *calc;
  hashmap_t *tagmap;
  ril_md5_t hash;
  
  void *userdata;
  
  struct
  {
    struct
    {
      int length;
      char string[RIL_DELIMITER_LENGTH];
    } left, right;
  } delimiter;
  
  struct
  {
    bool hascode;
    ril_common_header_t *common;
    ril_label_t         *label;
    ril_vmcmd_t         *cmd;
    ril_vmarg_t         *arg;
    void *data;
  } code;

  ril_var_t globalvar;
  ril_var_t *rootvar;

  ril_paircmd_t *paircmds;
  
  ril_state_t *state;
  ril_state_t *mainstate;
  
  RILERROR error_hander;
};

#ifdef __cplusplus
extern "C" {
#endif

void ril_parsecode(ril_code_t *code, const void *src);
void ril_freecode(RILVM vm);

#ifdef __cplusplus
}
#endif

#endif

/*	see copyright notice in ril.h */

#ifndef _RIL_CALC_H_
#define _RIL_CALC_H_

#include "variant.h"

#define VARIANT_MAX 20

enum
{
  CALC_PUSH,
  CALC_MOVE,
  CALC_ADD,
  CALC_SUB,
  CALC_STRCAT,
  CALC_MULTI,
  CALC_DIV,
  CALC_MOD,
  CALC_BITAND,
  CALC_BITOR,
  CALC_XOR,
  CALC_RSHIFT,
  CALC_LSHIFT,
  CALC_NOT,
  CALC_NEG,
  CALC_LESS,
  CALC_GREATER,
  CALC_LESSEQ,
  CALC_GREATEREQ,
  CALC_AND,
  CALC_OR,
  CALC_EQUAL,
  CALC_NOTEQUAL,
  CALC_INCFRONT,
  CALC_INCBACK,
  CALC_DECFRONT,
  CALC_DECBACK,
  CALC_END
};

struct _calc;
struct _ril_register;
typedef struct _ril_register ril_register_t;

typedef uint32_t calc_opcode_t;

typedef struct
{
  calc_opcode_t type;
  uint32_t size;
} calc_value_t;

typedef struct
{
  const char *front;
  const char *cur;
  const char *begin;
  buffer_t *lastdest_buffer;
  buffer_t *dest_buffer;
  int dest_begin;
  stack_t *stack;
  int class_num;
  int plus_priority;
  int prev_is_operator;
  int valueindex;
  bool hasminus;
  bool isref;
} calc_compile_t;

typedef struct
{
  const void *src;
} calc_execute_t;

typedef struct _calc
{
  stack_t *stack;
  buffer_t *temp_buffer;
  
  void (*error_hander)(struct _calc*, const char*);
} calc_t;

#ifdef __cplusplus
extern "C" {
#endif

calc_t* calc_open(int);
void calc_close(calc_t*);

RILRESULT calc_compile(ril_compile_t*, buffer_t*, const char*);
void calc_writevalue(calc_compile_t*, int, const void*, uint32_t);
void calc_writevalue2buffer(buffer_t* dest, int type, const void *src, uint32_t size);
int calc_countvalue(const void *src);
calc_value_t* calc_lastvalue(calc_compile_t *context);

ril_register_t* calc_execute(RILVM vm, const void *src);
const char* calc_tostring(RILVM vm, const void *src);

int calc_cast(calc_t *calc, variant_t *v, int casttype);
void calc_writeoperator(buffer_t *buffer, calc_opcode_t op);
bool calc_isvar(const void *src);

#ifdef __cplusplus
}
#endif

#endif

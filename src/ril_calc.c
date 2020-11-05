/*	see copyright notice in ril.h */

#include "ril_pcheader.h"
#include "variant.h"
#include "ril_vm.h"
#include "ril_utils.h"
#include "ril_var.h"
#include "ril_compiler.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>

#define STACK_SIZE 128

#ifdef _WIN32
#pragma warning(disable:4996)
#endif

#define OPERATOR_ADD_PRIORITY 50

typedef struct
{
  int priority;
  calc_opcode_t type;
} operator_t;

typedef struct
{
  const char *str;
  calc_opcode_t type;
} operator_type_t;

int calc_cast(calc_t *calc, variant_t *variant, int casttype)
{
  int type = variant->type;
  char *buf;

  variant->type = casttype;

  if (type == variant->type) return RIL_OK;

  switch (type)
  {
  case VARIANT_INTEGER:
    switch (casttype)
    {
    case VARIANT_INTEGER: break;
    case VARIANT_REAL: variant->real_value = (float)variant->int_value; break;
    case VARIANT_STRING:
      buf = (char*)buffer_malloc(calc->temp_buffer, 100);
      sprintf(buf, "%d", variant->int_value);
      variant->string_value = buf;
      break;
    }
    break;
  case VARIANT_REAL:
    switch (casttype)
    {
    case VARIANT_REAL: break;
    case VARIANT_INTEGER: variant->int_value = (int32_t)variant->real_value; break;
    case VARIANT_STRING:
      buf = (char*)buffer_malloc(calc->temp_buffer, 100);
      sprintf(buf, "%f", variant->real_value);
      variant->string_value = buf;
      break;
    }
    break;
  case VARIANT_STRING:
    switch (casttype)
    {
    case VARIANT_STRING: break;
    case VARIANT_INTEGER: variant->int_value = atoi(variant->string_value); break;
    case VARIANT_REAL: variant->real_value = (float)atof(variant->string_value); break;
    }
    break;
  case VARIANT_ARRAY:
    switch (casttype)
    {
    case VARIANT_STRING:
      variant->string_value = "";
      break;
    case VARIANT_INTEGER:
    case VARIANT_REAL:
      variant->int_value = 0;
      break;
    }
    break;
  case VARIANT_STRINGOBJ: {
    ril_string_t *string = (ril_string_t*)variant->ptr_value;
    switch (casttype)
    {
    case VARIANT_STRING: variant->string_value = string->ptr; break;
    case VARIANT_STRINGOBJ: break;
    case VARIANT_INTEGER: variant->int_value = atoi(string->ptr); break;
    case VARIANT_REAL: variant->real_value = (float)atof(string->ptr); break;
    }
    break; }
  case VARIANT_BYTES:
    switch (casttype)
    {
    case VARIANT_PTR:
      variant->ptr_value = (int8_t*)variant->ptr_value + sizeof(uint32_t);
      break;
    }
  case VARIANT_NULL:
    switch (casttype)
    {
    case VARIANT_INTEGER:
    case VARIANT_REAL:
      variant->int_value = 0;
      break;
    case VARIANT_STRING:
      variant->string_value = "";
      break;
    }
    break;
    break;
  }

  return RIL_OK;
}

static __inline void _getvar(RILVM vm, const void *src, ril_register_t *reg)
{
  uint32_t size;
  calc_opcode_t op;
  const char *name = NULL;
  bool isfirst = true;

  reg->var = &vm->globalvar;
  for (;;)
  {
    op = *(calc_opcode_t*)src;
    src = (calc_opcode_t*)src + 1;

    if (VAR_END == op) break;
    reg->parent = reg->var;
    switch (op)
    {
    case VAR_HASH:
      name = (char*)ril_read(&reg->hashkey, src, sizeof(reg->hashkey));
      reg->var = NULL;
      if (isfirst && RIL_SUCCEEDED(ril_fetchlocalvar(vm)))
      {
        reg->var = ril_getvarbyhash(vm, NULL, reg->hashkey);
        if (NULL != reg->var) reg->parent = vm->rootvar;
        ril_fetchglobalvar(vm);
      }
      if (NULL == reg->var)
      {
        reg->var = ril_createvarbyhash(vm, reg->parent, name, reg->hashkey);
      }
      isfirst = false;
      src = name + strlen(name) + 1;
      continue;
    case VAR_CALC:
      src = ril_read(&size, src, sizeof(uint32_t));
      name = calc_tostring(vm, src);
      reg->hashkey = hashmap_makekey(name);
      reg->var = ril_createvarbyhash(vm, reg->parent, name, reg->hashkey);
      src = (uint8_t*)src + size;
      continue;
    case VAR_ADD:
      name = NULL;
      reg->var = ril_createvar(vm, reg->parent, NULL);
      continue;
    }
  }
}

static __inline calc_value_t* _value(calc_execute_t *context)
{
  calc_value_t *value = (calc_value_t*)context->src;
  context->src = (int8_t*)context->src + sizeof(calc_value_t) + value->size;

  return value;
}

static __inline void _push(RILVM vm, calc_execute_t *context)
{
  ril_var_t *var;
  calc_value_t *value = _value(context);
  void *src = value + 1;

  ril_register_t *reg = (ril_register_t*)stack_push(vm->calc->stack, NULL);
  var = reg->var = &reg->temp;

  switch (value->type)
  {
  case VARIANT_INTEGER:
    ril_setinteger(vm, var, *(int*)src);
    break;
  case VARIANT_REAL:
    ril_setfloat(vm, var, *(float*)src);
    break;
  case VARIANT_REFVAR:
  case VARIANT_VAR:
    _getvar(vm, src, reg);
    break;
  case (VARIANT_LITERAL | VARIANT_STRING):
    var->variant.type = value->type;
    var->variant.string_value = (char*)src;
    break;
  case (VARIANT_LITERAL | VARIANT_BYTES):
    var->variant.type = value->type;
    var->variant.ptr_value = src;
    break;
  case VARIANT_LABEL: {
    int result;
    const ril_label_t *label = (ril_label_t*)src;
    const ril_label_t *label_cur;

    result = -1;
    if (label->id < vm->code.common->label_size)
    {
      label_cur = &vm->code.label[label->id];
      if (label->namehash == label_cur->namehash)
      {
        result = label_cur->cmdid;
      }
    }

    // has another code
    if (0 > result)
    {
      label_cur = ril_getlabel(vm, label->namehash);
      if (NULL != label_cur) result = label_cur->cmdid;
    }

    ril_setinteger(vm, var, result);
    break; }
  }
}

static __inline void _strcat(RILVM vm)
{
  ril_register_t *rv, *lv;
  const char *buf;
  int size, index;

  rv = (ril_register_t*)stack_pop(vm->calc->stack, NULL);
  lv = (ril_register_t*)stack_back(vm->calc->stack, NULL);

  index = buffer_size(vm->calc->temp_buffer);

  buf = ril_var2string(vm, lv->var); size = strlen(buf);
  buffer_write(vm->calc->temp_buffer, buf, size);

  buf = ril_var2string(vm, rv->var); size = strlen(buf);
  buffer_write(vm->calc->temp_buffer, buf, size);

  *(char*)buffer_malloc(vm->calc->temp_buffer, 1) = '\0';

  lv->var = &lv->temp;
  ril_setstring(vm,  lv->var, (char*)buffer_index(vm->calc->temp_buffer, index));
}

static __inline void _move(RILVM vm)
{
  ril_register_t *lv, *rv;
  
  rv = (ril_register_t*)stack_pop(vm->calc->stack, NULL);
  lv = (ril_register_t*)stack_back(vm->calc->stack, NULL);
  
  if (lv->var->isconst)
  {
    ril_error(vm, "Fatal error: Assignment of read-only variable");
  }

  ril_copyvar(vm, lv->var, rv->var);
}

static __inline void _strop(RILVM vm, int op, ril_var_t *dest, ril_var_t *lv, ril_var_t *rv)
{
  int result;
  switch (op)
  {
  case '=':
    result = !strcmp(ril_var2string(vm, lv), ril_var2string(vm, rv));
    ril_setinteger(vm, dest, result);
    break;
  case '!':
    result = 0 != strcmp(ril_var2string(vm, lv), ril_var2string(vm, rv));
    ril_setinteger(vm, dest, result);
    break;
  }
}

#define BW_OP(op, vm, lv, rv) \
rv = (ril_register_t*)stack_pop(vm->calc->stack, NULL); \
lv = (ril_register_t*)stack_back(vm->calc->stack, NULL); \
ril_setinteger(vm, &lv->temp, ril_var2integer(vm, lv->var) op ril_var2integer(vm, rv->var)); \
lv->var = &lv->temp;

#define OP(op, vm, lv, rv) \
rv = (ril_register_t*)stack_pop(vm->calc->stack, NULL); \
lv = (ril_register_t*)stack_back(vm->calc->stack, NULL); \
switch ((lv->var->variant.type | rv->var->variant.type) & (VARIANT_INTEGER | VARIANT_REAL | VARIANT_STRING)) \
{ \
case VARIANT_INTEGER: \
  ril_setinteger(vm, &lv->temp, ril_var2integer(vm, lv->var) op ril_var2integer(vm, rv->var)); \
  break; \
case (VARIANT_INTEGER|VARIANT_REAL): \
case VARIANT_REAL: \
  ril_setfloat(vm, &lv->temp, (float)(ril_var2float(vm, lv->var) op ril_var2float(vm, rv->var))); \
  break; \
case (VARIANT_STRING|VARIANT_INTEGER): \
case (VARIANT_STRING|VARIANT_REAL): \
case VARIANT_STRING: \
  _strop(vm, (#op)[0], &lv->temp, lv->var, rv->var); \
  break; \
} \
lv->var = &lv->temp;

#define INC_FRONT(op, vm, context) \
{ \
  context->src = (calc_opcode_t*)context->src + 1; \
  _push(vm, context); \
  lv = (ril_register_t*)stack_back(vm->calc->stack, NULL); \
  switch (lv->var->variant.type) \
  { \
  case VARIANT_INTEGER: \
    op lv->var->variant.int_value; \
    break; \
  case VARIANT_REAL: \
    op lv->var->variant.real_value; \
    break; \
  default: \
    ril_setinteger(vm, lv->var, 1); \
    break; \
  } \
}

#define INC_BACK(op, vm) \
{ \
  lv = (ril_register_t*)stack_pop(vm->calc->stack, NULL); \
\
  switch (lv->var->variant.type) \
  { \
  case VARIANT_INTEGER: \
    op lv->var->variant.int_value; \
    break; \
  case VARIANT_REAL: \
    op lv->var->variant.real_value; \
    break; \
  default: \
    ril_setinteger(vm, lv->var, 1); \
    break; \
  } \
}

calc_t* calc_open(int buffer_size)
{
  calc_t *calc = (calc_t*)malloc(sizeof(calc_t));
  ril_register_t *reg;
  int i;
  
  calc->error_hander = NULL;

  calc->stack = stack_open(sizeof(ril_register_t));
  stack_resize(calc->stack, STACK_SIZE);
  reg = (ril_register_t*)calc->stack->pool;
  for (i = 0; i < STACK_SIZE; ++i, ++reg)
  {
    ril_initvar(NULL, &reg->temp);
  }

  calc->temp_buffer = buffer_open(1, buffer_size);
  
  return calc;
}

void calc_close(calc_t *calc)
{
  ril_register_t *reg;
  int i;

  reg = (ril_register_t*)calc->stack->pool;
  for (i = 0; i < STACK_SIZE; ++i, ++reg)
  {
    ril_clearvar(NULL, &reg->temp);
  }
  stack_close(calc->stack);

  buffer_close(calc->temp_buffer);
  free(calc);
}

int __inline _execute(RILVM vm, calc_execute_t *context, const calc_opcode_t type)
{
  ril_register_t *lv, *rv;

  switch (type)
  {
  case CALC_PUSH: _push(vm, context); break;
  case CALC_MOVE: _move(vm); break;
  case CALC_ADD: OP(+, vm, lv, rv) break;
  case CALC_SUB: OP(-, vm, lv, rv) break;
  case CALC_STRCAT: _strcat(vm); break;
  case CALC_MULTI: OP(*, vm, lv, rv) break;
  case CALC_DIV: OP(/, vm, lv, rv) break;
  case CALC_INCFRONT: INC_FRONT(++, vm, context) break;
  case CALC_INCBACK: INC_BACK(++, vm) break;
  case CALC_DECFRONT: INC_FRONT(--, vm, context) break;
  case CALC_DECBACK: INC_BACK(--, vm) break;
  case CALC_GREATER:
    OP(>, vm, lv, rv)
    ril_setinteger(vm, lv->var, ril_var2integer(vm, lv->var));
    break;
  case CALC_GREATEREQ:
    OP(>=, vm, lv, rv)
    ril_setinteger(vm, lv->var, ril_var2integer(vm, lv->var));
    break;
  case CALC_LESS:
    OP(<, vm, lv, rv)
    ril_setinteger(vm, lv->var, ril_var2integer(vm, lv->var));
    break;
  case CALC_LESSEQ:
    OP(<=, vm, lv, rv)
    ril_setinteger(vm, lv->var, ril_var2integer(vm, lv->var));
    break;
  case CALC_EQUAL:
    OP(==, vm, lv, rv)
    ril_setinteger(vm, lv->var, ril_var2integer(vm, lv->var));
    break;
  case CALC_NOTEQUAL:
    OP(!=, vm, lv, rv)
    ril_setinteger(vm, lv->var, ril_var2integer(vm, lv->var));
    break;
  case CALC_MOD: BW_OP(%, vm, lv, rv) break;
  case CALC_BITAND: BW_OP(&, vm, lv, rv) break;
  case CALC_BITOR: BW_OP(|, vm, lv, rv) break;
  case CALC_AND: BW_OP(&&, vm, lv, rv) break;
  case CALC_OR: BW_OP(||, vm, lv, rv) break;
  case CALC_XOR: BW_OP(^, vm, lv, rv) break;
  case CALC_NOT:
    lv = (ril_register_t*)stack_back(vm->calc->stack, NULL);
    ril_setinteger(vm, &lv->temp, !ril_var2integer(vm, lv->var));
    lv->var = &lv->temp;
    break;
  case CALC_NEG:
    lv = (ril_register_t*)stack_back(vm->calc->stack, NULL);
    ril_setinteger(vm, &lv->temp, ~ril_var2integer(vm, lv->var));
    lv->var = &lv->temp;
    break;
  case CALC_RSHIFT: BW_OP(>>, vm, lv, rv) break;
  case CALC_LSHIFT: BW_OP(<<, vm, lv, rv) break;
  }

  return RIL_OK;
}

ril_register_t* calc_execute(RILVM vm, const void *src)
{
  calc_execute_t context;
  
  context.src = src;
  
  buffer_clear(vm->calc->temp_buffer);
  
  for (;;)
  {
    calc_opcode_t type = *(calc_opcode_t*)context.src;
    if (CALC_END == type) break;
    context.src = (calc_opcode_t*)context.src + 1;
    _execute(vm, &context, type);
  }

  return (ril_register_t*)stack_pop(vm->calc->stack, NULL);
}

const char* calc_tostring(RILVM vm, const void *src)
{
  return ril_var2string(vm, ((ril_register_t*)calc_execute(vm, src))->var);
}

static __inline void calc_optimize(RILVM vm, calc_compile_t *e_context)
{
  int size;
  ril_register_t *reg;
  calc_value_t *value;
  calc_execute_t context;
  
  context.src = buffer_index(e_context->dest_buffer, e_context->dest_begin);
  buffer_clear(vm->calc->temp_buffer);
  
  for (;;)
  {
    calc_opcode_t type = *(calc_opcode_t*)context.src;
    if (CALC_END == type) break;
    if (CALC_INCFRONT == type || CALC_INCBACK == type) break;
    if (CALC_PUSH == type)
    {
      calc_value_t *value = (calc_value_t*)((calc_opcode_t*)context.src + 1);
      if (!(value->type & (VARIANT_INTEGER | VARIANT_REAL | VARIANT_STRING))) break;
    }
    context.src = (calc_opcode_t*)context.src + 1;
    _execute(vm, &context, type);
  }
  
  size = (intptr_t)buffer_malloc(e_context->dest_buffer, 0) - (intptr_t)context.src;
  buffer_resize(e_context->dest_buffer, e_context->dest_begin);
  for (; NULL != (reg = (ril_register_t*)stack_back(vm->calc->stack, NULL)); stack_pop(vm->calc->stack, NULL))
  {
    switch (reg->var->variant.type)
    {
    case VARIANT_INTEGER:
      calc_writevalue(e_context, VARIANT_INTEGER, &reg->var->variant.ptr_value, sizeof(int));
      break;
    case VARIANT_REAL:
      calc_writevalue(e_context, VARIANT_REAL, &reg->var->variant.ptr_value, sizeof(float));
      break;
    case VARIANT_LITERAL | VARIANT_STRING:
    case VARIANT_STRING:
    case VARIANT_STRINGOBJ:
      calc_writevalue(e_context, VARIANT_LITERAL | VARIANT_STRING, ril_var2string(vm, reg->var), strlen(ril_var2string(vm, reg->var)) + 1);
      break;
    case VARIANT_NULL:
      calc_writevalue(e_context, VARIANT_NULL, 0, 0);
      break;
    default:
      value = (calc_value_t*)((uintptr_t)reg->var->variant.ptr_value - sizeof(calc_value_t));
      calc_writevalue(e_context, reg->var->variant.type, reg->var->variant.ptr_value, value->size);
      break;
    }
  }
  buffer_write(e_context->dest_buffer, context.src, size);
  
  stack_clear(vm->calc->stack);
}

static __inline RILRESULT push_operator(RILVM vm, calc_compile_t *context, operator_t operator_cur)
{
  operator_t *operator;
  
  operator_cur.priority += context->plus_priority;

  if (context->prev_is_operator)
  {
    switch (operator_cur.type)
    {
    case CALC_SUB:
      context->hasminus = true;
      return RIL_OK;
    case CALC_BITAND:
      context->isref = true;
      return RIL_OK;
    case CALC_NEG:
    case CALC_NOT:
      break;
    case CALC_INCFRONT:
    case CALC_DECFRONT:
      calc_writeoperator(context->dest_buffer, operator_cur.type);
      return RIL_OK;
    default:
      return ril_error(vm, "unexpected '%c'", context->begin[0]);
    }
    context->prev_is_operator = 0;
  }
  else
  {
    switch (operator_cur.type)
    {
    case CALC_NEG:
    case CALC_NOT:
      return ril_error(vm, "unexpected '%c'", context->begin[0]);
      break;
    case CALC_INCFRONT:
      *(calc_opcode_t*)buffer_malloc(context->lastdest_buffer, sizeof(calc_opcode_t)) = CALC_PUSH;
      buffer_write(context->lastdest_buffer, calc_lastvalue(context), sizeof(calc_value_t));
      buffer_write(context->lastdest_buffer, calc_lastvalue(context) + 1, calc_lastvalue(context)->size);
      calc_writeoperator(context->lastdest_buffer, CALC_INCBACK);
      return RIL_OK;
    case CALC_DECFRONT:
      *(calc_opcode_t*)buffer_malloc(context->lastdest_buffer, sizeof(calc_opcode_t)) = CALC_PUSH;
      buffer_write(context->lastdest_buffer, calc_lastvalue(context), sizeof(calc_value_t));
      buffer_write(context->lastdest_buffer, calc_lastvalue(context) + 1, calc_lastvalue(context)->size);
      calc_writeoperator(context->lastdest_buffer, CALC_DECBACK);
      return RIL_OK;
    }
  }
  context->prev_is_operator = !context->prev_is_operator;
  while (NULL != (operator = (operator_t*)stack_back(context->stack, NULL)))
  {
    if (operator_cur.priority <= operator->priority)
    {
      calc_writeoperator(context->dest_buffer, operator->type);
      stack_pop(context->stack, NULL);
      continue;
    }
    break;
  }
  stack_push(context->stack, &operator_cur);
  
  return RIL_OK;
}

static __inline operator_t check_operator(RILVM vm, calc_compile_t *context, const char **src)
{
  static operator_type_t operator_list[] = {
    {"=", CALC_MOVE},
    
    {"||", CALC_OR},
    {"or", CALC_OR},
    {"&&", CALC_AND},
    {"and", CALC_AND},
    
    {"|", CALC_BITOR},
    {"^", CALC_XOR},
    {"&", CALC_BITAND},
    
    {"==", CALC_EQUAL},
    {"!=", CALC_NOTEQUAL},
    
    {">=", CALC_GREATEREQ},
    {"<=", CALC_LESSEQ},
    {">", CALC_GREATER},
    {"<", CALC_LESS},
  
    {">>", CALC_RSHIFT},
    {"<<", CALC_LSHIFT},

    {".", CALC_STRCAT},
    {"-", CALC_SUB},
    {"+", CALC_ADD},

    {"*", CALC_MULTI},
    {"/", CALC_DIV},
    {"%", CALC_MOD},
  
    {"!", CALC_NOT},
    {"~", CALC_NEG},
    {"++", CALC_INCFRONT},
    {"--", CALC_DECFRONT},
  };
  static int list_size = sizeof(operator_list) / sizeof(operator_type_t);
  operator_t operator = {0};
  int i, shift_size = 0;

  for (i = 0; i < list_size; ++i)
  {
    operator_type_t *operator_type = &operator_list[i];
    int cnt = 0;
    int cmp_size = strlen(operator_type->str);
    while ((*src)[cnt] == operator_type->str[cnt])
    {
      if (cnt + 1 >= cmp_size && shift_size < cmp_size)
      {
        operator.type = operator_type->type;
        operator.priority = i + 1;
        shift_size = cmp_size;
      }
      ++cnt;
    }
  }
  
  *src += shift_size;
  if ('=' == **src)
  {
    calc_value_t v = *calc_lastvalue(context);
    operator_t operator_equal = check_operator(vm, context, src);
    
    if (context->prev_is_operator)
    {
      operator.type = CALC_ADD;
      return operator;
    }
    context->plus_priority += OPERATOR_ADD_PRIORITY;
    push_operator(vm, context, operator_equal);
    context->prev_is_operator = 0;

    calc_writevalue(context, v.type, calc_lastvalue(context) + 1, v.size);
    operator.priority -= OPERATOR_ADD_PRIORITY;
    context->plus_priority += OPERATOR_ADD_PRIORITY;
  }

  return operator;
}

static __inline void _compile_close(calc_compile_t *context)
{
  buffer_close(context->lastdest_buffer);
  stack_close(context->stack);
}

static __inline RILRESULT _compile_string(calc_compile_t *context)
{
  int size = 0, len;
  
  calc_writevalue(context, VARIANT_LITERAL | VARIANT_STRING, NULL, 0);
  
  ++context->cur;
  while ('"' != *context->cur)
  {
    if ('\\' == *context->cur)
    {
      switch (context->cur[1])
      {
      case '"':
      case '\\':
        ++context->cur;
        break;
      }
    }
    len = mblen(context->cur, MB_CUR_MAX);
    buffer_write(context->dest_buffer, context->cur, len);
    size += len;
    context->cur += len;
  }
  *(uint8_t*)buffer_malloc(context->dest_buffer, 1) = '\0';
  ++size;
  
  calc_lastvalue(context)->size = size;
  
  ++context->cur;

  return RIL_OK;
}

static __inline RILRESULT _compile_hex(calc_compile_t *context)
{
  int value = 0, column;
  
  context->cur += 2; // skip 0x
  for (;; ++context->cur, value = value * 16 + column)
  {
    column = *context->cur;
    if (isdigit(column))
    {
      column -= '0';
      continue;
    }
    if ('a' <= column && 'f' >= column)
    {
      column -= 'a' - 10;
      continue;
    }
    if ('A' <= column && 'F' >= column)
    {
      column -= 'A' - 10;
      continue;
    }
    break;
  }
  if (context->hasminus) value *= -1;
  calc_writevalue(context, VARIANT_INTEGER, &value, sizeof(int));
  
  return RIL_OK;
}

static __inline RILRESULT _compile_number(RILVM vm, calc_compile_t *context)
{
  int isfloat = 0, length;
  const char *start = context->cur;
  char *buf;

  if ('0' == context->cur[0] && 'x' == context->cur[1])
  {
    return _compile_hex(context);
  }

  do
  {
    ++context->cur;
    if ('.' == *context->cur && isdigit(context->cur[1]))
    {
      if (isfloat)
      {
        _compile_close(context);
        return ril_error(vm, "unexpected '.'");
      }
      context->cur += 2;
      isfloat = 1;
    }
  }
  while (isdigit(*context->cur));
  
  length = context->cur - start;
  buf = (char*)malloc(length + 1);
  memcpy(buf, start, length);
  buf[length] = '\0';
  
  if (isfloat)
  {
    float value = (float)atof(buf);
    if (context->hasminus) value *= -1;
    calc_writevalue(context, VARIANT_REAL, &value, sizeof(float));
  }
  else
  {
    int value = atoi(buf);
    if (context->hasminus) value *= -1;
    calc_writevalue(context, VARIANT_INTEGER, &value, sizeof(int));
  }
  
  free(buf);
  
  return RIL_OK;
}

RILRESULT calc_compile(ril_compile_t *c_context, buffer_t *dest_buffer, const char *src)
{
  calc_compile_t context;
  operator_t operator, *poperator;
  const char *current_text;
  int result;

  context.hasminus = false;
  context.isref = false;
  context.class_num = 0;
  context.plus_priority = 0;
  context.prev_is_operator = true;
  context.front = src;
  context.cur = context.front;
  context.dest_begin = buffer_size(dest_buffer);
  context.dest_buffer = dest_buffer;
  context.lastdest_buffer = buffer_open(1, 1024);
  context.stack = stack_open(sizeof(operator_t));
  stack_resize(context.stack, 100);

  for (;;)
  {
    context.cur = ril_trimspace(context.cur);
    if ('\0' == *context.cur) break;

    context.begin = context.cur;

    /* user function */
    current_text = context.cur;
    result = calc_cb_compile(&context, c_context);
    if (RIL_FAILED(result))
    {
      _compile_close(&context);
      return RIL_ERROR;
    }
    if (CALC_END == result) break;

    if (current_text != context.cur)
    {
      context.prev_is_operator = false;
      context.hasminus = false;
      context.isref = false;
      continue;
    }
    
    /* operator */
    operator = check_operator(c_context->vm, &context, &context.cur);
    if (operator.type)
    {
      if (RIL_FAILED(push_operator(c_context->vm, &context, operator)))
      {
        _compile_close(&context);
        return RIL_ERROR;
      }
      if ('\0' == *context.cur) break;
      continue;
    }
    
    if ('(' == *context.cur)
    {
      ++context.class_num;
      context.prev_is_operator = 1;
      context.plus_priority += OPERATOR_ADD_PRIORITY;
      ++context.cur;
      continue;
    }
    
    if (')' == *context.cur)
    {
      if (!context.class_num)
      {
        _compile_close(&context);
        return ril_error(c_context->vm, "unexpected )");
      }
      --context.class_num;
      context.plus_priority -= OPERATOR_ADD_PRIORITY;
      if (context.plus_priority < 0) break;
      ++context.cur;
      continue;
    }

    if (!context.prev_is_operator)
    {
      _compile_close(&context);
      return ril_error(c_context->vm, "no operator");
    }
    context.prev_is_operator = false;
    
    /* string */
    if ('\"' == *context.cur)
    {
      if (RIL_FAILED(_compile_string(&context))) return RIL_ERROR;
      context.hasminus = false;
      continue;
    }

    /* numeric */
    if (isdigit(*context.cur))
    {
      if (RIL_FAILED(_compile_number(c_context->vm, &context))) return RIL_ERROR;
      context.hasminus = false;
      continue;
    }
    
    _compile_close(&context);
    return ril_error(c_context->vm, "unexpected '%c'", context.begin[0]);
  }
  
  if (context.prev_is_operator)
  {
    _compile_close(&context);
    return ril_error(c_context->vm, "unexpected '%c'", context.begin[0]);
  }
  
  if (context.class_num)
  {
    _compile_close(&context);
    return ril_error(c_context->vm, "'(' not closed");
  }
  
  while (NULL != (poperator = (operator_t*)stack_pop(context.stack, NULL)))
  {
    calc_writeoperator(dest_buffer, poperator->type);
  }
  buffer_write(dest_buffer, buffer_front(context.lastdest_buffer), buffer_bytesize(context.lastdest_buffer));
  calc_writeoperator(dest_buffer, CALC_END);

  if (context.isref && !calc_isvar(buffer_front(dest_buffer)))
  {
    return ril_error(c_context->vm, "Calculation of a reference type can not be");
  }
  
  calc_optimize(c_context->vm, &context);
  _compile_close(&context);
  
  return RIL_OK;
}

void calc_writevalue(calc_compile_t* context, int type, const void *src, uint32_t size)
{
  context->valueindex = buffer_size(context->dest_buffer) + sizeof(calc_opcode_t);
  calc_writevalue2buffer(context->dest_buffer, type, src, size);
}

void calc_writevalue2buffer(buffer_t* dest, int type, const void *src, uint32_t size)
{
  calc_value_t *value;

  calc_writeoperator(dest, CALC_PUSH);
  value = (calc_value_t*)buffer_malloc(dest, sizeof(calc_value_t));
  value->type = type;
  value->size = size;
  if (NULL != src) buffer_write(dest, src, size);
}

calc_value_t* calc_lastvalue(calc_compile_t *context)
{
  return (calc_value_t*)buffer_index(context->dest_buffer, context->valueindex);
}

int calc_countvalue(const void *src)
{
  int counter = 0, type;
  const calc_value_t *value;
  
  for(;;)
  {
    type = *(calc_opcode_t*)src;
    src = (calc_opcode_t*)src + 1;
    if (CALC_END == type) break;
    if (CALC_PUSH == type)
    {
      value = (calc_value_t*)src;
      src = (int8_t*)src + sizeof(calc_value_t) + value->size;
      ++counter;
    }
  }
  
  return counter;
}

void calc_writeoperator(buffer_t *buffer, calc_opcode_t op)
{
  buffer_write(buffer, &op, sizeof(op));
}

bool calc_isvar(const void *src)
{
  if (1 != calc_countvalue(src)) return false;
      
  return VARIANT_VAR & ((calc_value_t*)((uint8_t*)src + sizeof(calc_opcode_t)))->type;
}
/*	see copyright notice in ril.h */

#include "ril_pcheader.h"
#include "ril_vm.h"
#include "ril_var.h"
#include "ril_compiler.h"
#include "ril_utils.h"

ril_var_t* ril_getvar(RILVM vm, ril_var_t *parent, const char *name)
{
  return ril_getvarbyhash(vm, parent, hashmap_makekey(name));
}

ril_var_t* ril_getvarbyhash(RILVM vm, ril_var_t *parent, hashmap_key_t namehash)
{
  if (NULL == parent) parent = vm->rootvar;
  if (!ril_isarray(parent)) return NULL;
  
  return (ril_var_t*)hashmap_getdata(((ril_array_t*)parent->variant.ptr_value)->map, namehash);
}

ril_var_t* ril_newvar(RILVM vm)
{
  ril_var_t *var = (ril_var_t*)ril_malloc(sizeof(ril_var_t));
  
  ril_initvar(vm, var);
  
  return var;
}

ril_var_t* ril_createvarbyhash(RILVM vm, ril_var_t *parent, const char *name, hashmap_key_t hashkey)
{
  ril_var_t *var = ril_getvarbyhash(vm, parent, hashkey);
  
  if (NULL == var)
  {
    var = ril_newvar(vm);
    ril_set2arraybyhash(vm, parent, var, name, hashkey);
    ril_deletevar(vm, var);
  }
  
  return var;
}

ril_var_t* ril_createvar(RILVM vm, ril_var_t *parent, const char *name)
{
  if (NULL == name)
  {
    ril_var_t *var = ril_newvar(vm);
    ril_set2arraybyhash(vm, parent, var, NULL, 0);
    ril_deletevar(vm, var);
    return var;
  }
  return ril_createvarbyhash(vm, parent, name, hashmap_makekey(name));
}

static __inline ril_array_t* ril_newarray(RILVM vm)
{
  ril_array_t *array = (ril_array_t*)ril_malloc(sizeof(ril_array_t));

  array->refcount = 1;
  array->nextnum = 0;
  array->map = hashmap_open();

  return array;
}

void ril_cleararray(RILVM vm, ril_var_t *var)
{
  ril_array_t *array;
  hashmap_entry_t *mapentry;

  if (!ril_isarray(var)) return;

  array = (ril_array_t*)var->variant.ptr_value;
  if (1 < array->refcount)
  {
    --array->refcount;
    var->variant.ptr_value = ril_newarray(vm);
    return;
  }

  while (NULL != (mapentry = hashmap_firstentry(array->map)))
  {
    ril_unset2array(vm, var, hashmap_getkeybyentry(mapentry));
  }

  array->nextnum = 0;
  hashmap_clear(array->map);
}

ril_var_t* ril_set2arraybyhash(RILVM vm, ril_var_t *parent, ril_var_t *var, const char *name, hashmap_key_t hashkey)
{
  bool isnum;
  const char *cur;
  int num;
  ril_var_t *var2, *var3;
  ril_array_t *array;
  hashmap_entry_t *mapentry;
  
  if (NULL == parent) parent = vm->rootvar;
  
  if (!(NULL == name && 0 == hashkey))
  {
    var2 = ril_getvarbyhash(vm, parent, hashkey);
    if (NULL != var2) ril_deletevar(vm, var2);
  }

  if (!ril_isarray(parent))
  {
    ril_clearvar(vm, parent);
    
    array = ril_newarray(vm);
    
    parent->variant.type = VARIANT_ARRAY;
    parent->variant.ptr_value = array;
  }
  else
  {
    array = parent->variant.ptr_value;
  }
  
  // copy array
  if (1 < array->refcount)
  {
    --array->refcount;
    mapentry = hashmap_firstentry(array->map);
    
    array = ril_newarray(vm);
    
    parent->variant.ptr_value = array;
    
    for (; NULL != mapentry; mapentry = hashmap_nextentry(mapentry))
    {
      var2 = hashmap_getdatabyentry(mapentry);
      var3 = ril_createvarbyhash(vm, parent, (char*)hashmap_getrawkeybyentry(mapentry), hashmap_getkeybyentry(mapentry));
      ril_copyvar(vm, var3, var2);
    }
  }
  
  /* copy name */
  if (NULL != name)
  {
    num = strlen(name) + 1;
    cur = (char*)ril_malloc(num);
    memcpy((char*)cur, name, num);
    name = cur;
  }
  else
  {
    /* not specify a name */
    if (0 == hashkey)
    {
      name = (char*)ril_malloc(16);
      sprintf((char*)name, "%d", array->nextnum);
      hashkey = hashmap_makekey(name);
    }
  }

  ril_retainvar(var);
  hashmap_add(array->map, hashkey, name, var);

  /* is numeric */
  if (NULL == name) return var;

  isnum = '\0' != *name;
  for (cur = name; '\0' != *cur; ++cur)
  {
    if (!isdigit((unsigned char)*cur))
    {
      isnum = false;
      break;
    }
  }
  if (isnum)
  {
    num = atoi(name);
    if (array->nextnum <= num) array->nextnum = num + 1;
  }
  
  return var;
}

ril_var_t* ril_set2array(RILVM vm, ril_var_t *parent, ril_var_t *var, const char *name)
{
  return ril_set2arraybyhash(vm, parent, var, name, NULL != name ? hashmap_makekey(name) : 0);
}

void ril_unset2array(RILVM vm, ril_var_t *parent, hashmap_key_t hashkey)
{
  hashmap_entry_t *entry;
  ril_array_t *array = (ril_array_t*)parent->variant.ptr_value;
  ril_var_t *var = ril_getvarbyhash(vm, parent, hashkey);

  if (NULL == var) return;
  ril_deletevar(vm, var);

  entry = hashmap_getentry(array->map, hashkey);
  ril_free(hashmap_getrawkeybyentry(entry));
  hashmap_delete(array->map, hashkey);
}

void ril_initvar(RILVM vm, ril_var_t *var)
{
  var->isconst = false;
  var->refcount = 1;
  var->variant.type = VARIANT_NULL;
}

void ril_retainvar(ril_var_t *var)
{
  ++var->refcount;
}

void ril_deletevar(RILVM vm, ril_var_t *var)
{
  --var->refcount;
  if (0 != var->refcount) return;

  ril_clearvar(vm, var);
  ril_free(var);
}

void ril_clearvar(RILVM vm, ril_var_t *var)
{  
  var->isconst = false;
  
  switch (var->variant.type)
  {
  case VARIANT_STRINGOBJ: {
    ril_string_t *string = (ril_string_t*)var->variant.ptr_value;
    if (1 < string->refcount)
    {
      --string->refcount;
      break;
    }
    free(string->ptr);
    ril_free(var->variant.ptr_value);
    break; }
  case VARIANT_ARRAY: {
    ril_cleararray(vm, var);
    hashmap_close(((ril_array_t*)var->variant.ptr_value)->map);
    ril_free(((ril_array_t*)var->variant.ptr_value));
    break; }
  case VARIANT_NULL:
    return;
  default:
    variant_clear(&var->variant);
  }
  
  var->variant.type = VARIANT_NULL;
}

void ril_setinteger(RILVM vm, ril_var_t *var, const int value)
{
  ril_clearvar(vm, var);
  var->variant.type = VARIANT_INTEGER;
  var->variant.int_value = value;
}

void ril_setfloat(RILVM vm, ril_var_t *var, const float value)
{
  ril_clearvar(vm, var);
  var->variant.type = VARIANT_REAL;
  var->variant.real_value = value;
}

void ril_setstring(RILVM vm, ril_var_t *var, const char *value)
{
  ril_setstringbysize(vm, var, value, strlen(value) + 1);
}

void ril_setstringbysize(RILVM vm, ril_var_t *var, const char *value, int size)
{
  ril_string_t *string = (ril_string_t*)ril_malloc(sizeof(ril_string_t));
  
  ril_clearvar(vm, var);
  
  string->refcount = 1;
  string->size = size+ 1;
  string->ptr = (char*)malloc(string->size);
 
  var->variant.type = VARIANT_STRINGOBJ;
  var->variant.ptr_value = string;
  
  memcpy(string->ptr, value, string->size);
}

void ril_copyvar(RILVM vm, ril_var_t *dest, ril_var_t *src)
{
  ril_setvariant(vm, dest, &src->variant);
}

void ril_setvariant(RILVM vm, ril_var_t *dest, variant_t *variant)
{
  ril_clearvar(vm, dest);
  
  switch (variant->type)
  {
  case VARIANT_STRING:
    ril_setstring(vm, dest, variant->string_value);
    break;
  case VARIANT_STRINGOBJ:
    ++((ril_string_t*)variant->ptr_value)->refcount;
    dest->variant = *variant;
    break;
  case VARIANT_ARRAY:
    ++((ril_array_t*)variant->ptr_value)->refcount;
    dest->variant = *variant;
    break;
  case VARIANT_NULL:
    dest->variant.type = VARIANT_NULL;
    break;
  default:
    variant_copy(&dest->variant, variant);
  }
}

void ril_setconst(ril_var_t *var)
{
  var->isconst = true;
}

static __inline variant_t* _variantcast(ril_var_t *var)
{
  static variant_t variant;
  
  if (VARIANT_STRINGOBJ != var->variant.type) return &var->variant;
  
  variant.type = VARIANT_STRING;
  variant.string_value = ((ril_string_t*)var->variant.ptr_value)->ptr;
  
  return &variant;
}

const char* ril_var2string(RILVM vm, ril_var_t *var)
{
  variant_t variant;
  
  if (NULL == vm)
  {
    return variant_getstring(_variantcast(var));
  }
  
  variant = var->variant;
  calc_cast(vm->calc, &variant, VARIANT_STRING);
  
  return variant.string_value;
}

int ril_var2integer(RILVM vm, ril_var_t *var)
{
  variant_t variant;
  
  if (NULL == vm)
  {
    return variant_getinteger(_variantcast(var));
  }
  
  variant = var->variant;
  calc_cast(vm->calc, &variant, VARIANT_INTEGER);
  
  return variant.int_value;
}

float ril_var2float(RILVM vm, ril_var_t *var)
{
  variant_t variant;
  
  if (NULL == vm)
  {
    return variant_getfloat(_variantcast(var));
  }
  
  variant = var->variant;
  calc_cast(vm->calc, &variant, VARIANT_REAL);
  
  return variant.real_value;
}

bool ril_var2bool(RILVM vm, ril_var_t *var)
{
  if (NULL == vm)
  {
    return 0 != variant_getinteger(_variantcast(var));
  }
  
  return 0 != ril_var2integer(vm, var);
}

RILRESULT ril_writevar(buffer_t *dest, ril_var_t *var)
{
  hashmap_entry_t *mapentry;
  ril_array_t *array;
  uint32_t size;
  const char *str;
  calc_opcode_t type;

  if (ril_isnull(var))
  {
    type = VARIANT_NULL;
    buffer_write(dest, &type, sizeof(calc_opcode_t));
    return RIL_OK;
  }
  if (ril_isint(var))
  {
    type = VARIANT_INTEGER;
    buffer_write(dest, &type, sizeof(calc_opcode_t));

    buffer_write(dest, &var->variant.int_value, sizeof(var->variant.int_value));

    return RIL_OK;
  }
  if (ril_isreal(var))
  {
    type = VARIANT_REAL;
    buffer_write(dest, &type, sizeof(calc_opcode_t));

    buffer_write(dest, &var->variant.real_value, sizeof(var->variant.real_value));

    return RIL_OK;
  }
  if (ril_isstring(var))
  {
    type = VARIANT_STRING;
    buffer_write(dest, &type, sizeof(calc_opcode_t));

    str = ril_var2string(NULL, var);
    size = strlen(str) + 1;
    buffer_write(dest, &size, sizeof(size));
    buffer_write(dest, str, size);

    return RIL_OK;
  }
  if (ril_isarray(var))
  {
    type = VARIANT_ARRAY;
    buffer_write(dest, &type, sizeof(calc_opcode_t));

    array = (ril_array_t*)var->variant.ptr_value;
    size = hashmap_count(array->map);
    buffer_write(dest, &size, sizeof(size));
    mapentry = hashmap_firstentry(((ril_array_t*)var->variant.ptr_value)->map);
    for (; NULL != mapentry; mapentry = hashmap_nextentry(mapentry))
    {
      var = (ril_var_t*)hashmap_getdatabyentry(mapentry);
      str = (char*)hashmap_getrawkeybyentry(mapentry);
      buffer_write(dest, str, strlen(str) + 1);
      ril_writevar(dest, var);
    }

    return RIL_OK;
  }
  
  return RIL_ERROR;
}

int ril_readvar(RILVM vm, ril_var_t *var, const void *src)
{
  const void *src_begin = src;
  uint32_t i, size;
  ril_var_t *childvar;
  calc_opcode_t type;
  
  src = ril_read(&type, src, sizeof(type));
  
  ril_clearvar(vm, var);
  
  switch (type)
  {
  case VARIANT_NULL:
    break;
  case VARIANT_INTEGER:
    ril_setinteger(vm, var, *(int*)src);
    src = (int8_t*)src + sizeof(var->variant.int_value);
    break;
  case VARIANT_REAL:
    ril_setfloat(vm, var, *(float*)src);
    src = (int8_t*)src + sizeof(var->variant.real_value);
    break;
  case VARIANT_STRING:
    src = ril_read(&size, src, sizeof(size));
    ril_setstring(vm, var, (const char*)src);
    src = (int8_t*)src + size;
    break;
  case VARIANT_ARRAY:
    src = ril_read(&size, src, sizeof(size));
    for (i = 0; i < size; ++i)
    {
      childvar = ril_createvar(vm, var, (char*)src);
      src = (int8_t*)src + strlen((char*)src) + 1;
      src = (int8_t*)src + ril_readvar(vm, childvar, src);
    }
    break;
  }
  
  return (intptr_t)src - (intptr_t)src_begin;
}

int ril_countvar(ril_var_t *var)
{
  if (!ril_isarray(var)) return 0;
  
  return hashmap_count(((ril_array_t*)var->variant.ptr_value)->map);
}

bool ril_isnull(ril_var_t *var)
{
  return VARIANT_NULL == var->variant.type;
}

bool ril_isint(ril_var_t *var)
{
  return VARIANT_INTEGER == var->variant.type;
}

bool ril_isreal(ril_var_t *var)
{
  return VARIANT_REAL == var->variant.type;
}

bool ril_isarray(ril_var_t *var)
{
  return VARIANT_ARRAY == var->variant.type;
}

bool ril_isstring(ril_var_t *var)
{
  return 0 < (VARIANT_STRING & var->variant.type);
}

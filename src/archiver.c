#include "archiver.h"
#include "variant.h"
#include <stdint.h>
#include <stdlib.h>

enum
{
  VARIANT_ARCHIVER = VARIANT_STD_SIZE,
};

struct _archiver
{
  hashmap_t *map;
};

archiver_t* archiver_open(void)
{
  archiver_t *archiver = (archiver_t*)malloc(sizeof(archiver_t));
  
  archiver->map = hashmap_open();
  
  return archiver;
}

static __inline void _variant_clear(variant_t *v)
{
  variant_clear(v);
  if (variant_istype(v, VARIANT_ARCHIVER))
  {
    archiver_close((archiver_t*)v->ptr_value);
    v->type = VARIANT_NULL;
  }
}

void archiver_clear(archiver_t *archiver)
{
  hashmap_entry_t *mapentry = hashmap_firstentry(archiver->map);
  variant_t *v;
  
  for (; NULL != mapentry; mapentry = hashmap_nextentry(mapentry))
  {
    v = (variant_t*)hashmap_getdatabyentry(mapentry);
    _variant_clear(v);
    variant_delete(v);
  }
  
  hashmap_clear(archiver->map);
}

void archiver_close(archiver_t *archiver)
{
  archiver_clear(archiver);
  hashmap_close(archiver->map);
  free(archiver);
}

static __inline variant_t* archiver_createvariantbyhash(archiver_t *archiver, hashmap_key_t key)
{
  variant_t *v = (variant_t*)hashmap_getdata(archiver->map, key);
  
  return NULL != v ? v : variant_new();
}

void archiver_setinteger(archiver_t *archiver, hashmap_key_t key, int value)
{
  variant_t *v = archiver_createvariantbyhash(archiver, key);
  _variant_clear(v);
  variant_setinteger(v, value);
}

void archiver_setreal(archiver_t *archiver, hashmap_key_t key, double value)
{
  variant_t *v = archiver_createvariantbyhash(archiver, key);
  _variant_clear(v);
  variant_setfloat(v, (float)value);
}

void archiver_setstring(archiver_t *archiver, hashmap_key_t key, const char *value)
{
  variant_t *v = archiver_createvariantbyhash(archiver, key);
  _variant_clear(v);
  variant_setstring(v, value);
}

void archiver_setarchiver(archiver_t *archiver, hashmap_key_t key, archiver_t *value)
{
  variant_t *v = archiver_createvariantbyhash(archiver, key);
  _variant_clear(v);
  v->type = VARIANT_ARCHIVER;
  v->ptr_value = value;
}

static __inline variant_t* archiver_getvariant(archiver_t *archiver, hashmap_key_t key)
{
  return (variant_t*)hashmap_getdata(archiver->map, key);
}

int archiver_hasdata(archiver_t *archiver, hashmap_key_t key)
{
  return NULL != archiver_getvariant(archiver, key);
}

int archiver_getinteger(archiver_t *archiver, hashmap_key_t key)
{
  variant_t *v = archiver_getvariant(archiver, key);

  return NULL != v ? variant_getinteger(v) : 0;
}

double archiver_getreal(archiver_t *archiver, hashmap_key_t key)
{
  variant_t *v = archiver_getvariant(archiver, key);
  
  return NULL != v ? variant_getfloat(v) : 0;
}

const char* archiver_getstring(archiver_t *archiver, hashmap_key_t key)
{
  variant_t *v = archiver_getvariant(archiver, key);
  
  return NULL != v ? variant_getstring(v) : NULL;
}

archiver_t* archiver_getarchiver(archiver_t *archiver, hashmap_key_t key)
{
  variant_t *v = archiver_getvariant(archiver, key);
  
  return NULL != v ? (archiver_t*)v->ptr_value : NULL;
}

int archiver_read(archiver_t *archiver, const void *src)
{
  const void *cur = src;
  uint32_t i, size = *(uint32_t*)src;
  hashmap_key_t key;
  variant_t *variant;
  
  cur = (uint8_t*)cur + sizeof(uint32_t);
  
  for (i = 0; i < size; ++i)
  {
    key = *(hashmap_key_t*)cur;
    cur = (uint8_t*)cur + sizeof(key);
    
    variant = archiver_createvariantbyhash(archiver, key);
    _variant_clear(variant);
    
    cur = (uint8_t*)cur + variant_unselialize(variant, cur);
    if (variant_istype(variant, VARIANT_ARCHIVER))
    {
      variant->ptr_value = archiver_open();
      cur = (uint8_t*)cur + archiver_read((archiver_t*)variant->ptr_value, cur);
    }
  }
  
  return (uintptr_t)cur - (uintptr_t)src;
}

void archiver_write(archiver_t *archiver, buffer_t *buffer)
{
  uint32_t size = hashmap_count(archiver->map);
  hashmap_entry_t *mapentry = hashmap_firstentry(archiver->map);
  hashmap_key_t key;
  variant_t *variant;
  
  buffer_write(buffer, &size, sizeof(size));
  
  for (; NULL != mapentry; mapentry = hashmap_nextentry(mapentry))
  {
    variant = (variant_t*)hashmap_getdatabyentry(mapentry);
    key = hashmap_getkeybyentry(mapentry);
    buffer_write(buffer, &key, sizeof(key));
    variant_selialize(variant, buffer);
    if (variant_istype(variant, VARIANT_ARCHIVER))
    {
      archiver_write((archiver_t*)variant->ptr_value, buffer);
    }
  }
}
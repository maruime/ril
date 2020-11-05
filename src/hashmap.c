#include "hashmap.h"
#include "crc.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define TABLESIZE 503

struct _hashmap_entry
{
  hashmap_key_t hashkey;
  const void *rawkey;
  const void *data;
  hashmap_entry_t *next, *prev;
};

struct _hashmap
{
  hashmap_entry_t *table, *first, *last;
  unsigned int size, count;
};

static __inline unsigned int _primenumber(unsigned int start)
{
  int i, j, isnot;

  for (i = start + 1; i < 0xFFFFFFFF; i += 2)
  {
    isnot = 0;
    for (j = 3; j <= sqrt(i); j += 2)
    {
      if (i % j == 0)
      {
        isnot = 1;
        break;
      }
    }

    if(isnot == 0) return i;
  }

  return 0;
}

hashmap_key_t hashmap_makekey(const char *str)
{
  return crc(str, strlen(str), 0);
}

static __inline int hashmap_extend(hashmap_t *hashmap)
{
  int size;
  void *buf;
  
  size = _primenumber(hashmap->size * 2);
  
  buf = (hashmap_entry_t*)realloc(hashmap->table, sizeof(hashmap_entry_t) * size);
  if (NULL == buf) return -1;
  
  hashmap->size = size;
  hashmap->table = buf;
  
  return 0;
}

static __inline hashmap_key_t hashmap_rehash(hashmap_t *hashmap, const hashmap_key_t firsthash)
{
  hashmap_key_t tablehash = firsthash % hashmap->size;
  unsigned int k, size = hashmap->size / 2;
  
  for (k = 1; k <= size; ++k)
  {
    tablehash += k + (k - 1);
    if (hashmap->size <= tablehash) tablehash %= hashmap->size;
    
    if (NULL == hashmap->table[tablehash].data)
    {
      return tablehash;
    }
  }
  
  return -1;
}

void hashmap_add(hashmap_t *hashmap, const hashmap_key_t hash, const void *rawkey, const void *data)
{
  hashmap_key_t tablehash = hash % hashmap->size;
  hashmap_entry_t *entry;
  
  if (NULL != hashmap->table[tablehash].data)
  {
    tablehash = hashmap_rehash(hashmap, tablehash);
    
    if (-1 == tablehash) 
    {
      if (hashmap_extend(hashmap)) return;
      hashmap_add(hashmap, hash, rawkey, data);
      return;
    }
  }
  entry = &hashmap->table[tablehash];
  entry->hashkey = hash;
  entry->rawkey = rawkey;
  entry->data = data;
  entry->prev = hashmap->last;
  entry->next = NULL;
  if (NULL == hashmap->first) hashmap->first = entry;
  if (NULL != entry->prev) entry->prev->next = entry;
  hashmap->last = entry;
  ++hashmap->count;
}

void* hashmap_getdata(hashmap_t *hashmap, const hashmap_key_t hash)
{
  hashmap_entry_t *entry = hashmap_getentry(hashmap, hash);
  return NULL != entry ? (void*)entry->data : NULL;
}

void* hashmap_delete(hashmap_t *hashmap, const hashmap_key_t hash)
{
  hashmap_key_t tablehash = hash % hashmap->size;
  unsigned int k = 0, size = hashmap->size / 2;
  hashmap_entry_t *entry;
  const void *data;
  
  while (k <= size)
  {
    entry = &hashmap->table[tablehash];
    if (hash != entry->hashkey)
    {
      ++k;
      if (k) tablehash += k + (k - 1);
      if (hashmap->size <= tablehash) tablehash %= hashmap->size;
      continue;
    }

    --hashmap->count;
    data = entry->data;
    entry->data = NULL;
    if (NULL != entry->prev)
    {
      entry->prev->next = entry->next;
    }
    if (NULL != entry->next)
    {
      entry->next->prev = entry->prev;
    }
    if (hashmap->first == entry)
    {
      hashmap->first = entry->next;
    }
    if (hashmap->last == entry)
    {
      hashmap->last = entry->prev;
    }

    return (void*)data;
  }
  
  return NULL;
}

void hashmap_clear(hashmap_t *hashmap)
{
  hashmap_entry_t *entry = hashmap_firstentry(hashmap);

  for (; NULL != entry; entry = hashmap_nextentry(entry))
  {
    entry->data = NULL;
  }
  
  hashmap->count = 0;
  hashmap->first = NULL;
  hashmap->last = NULL;
}

hashmap_t* hashmap_open(void)
{
  hashmap_t *hashmap = (hashmap_t*)malloc(sizeof(hashmap_t));
  hashmap_entry_t *entry;
  unsigned int i;

  hashmap->size = TABLESIZE;
  hashmap->count = 0;
  hashmap->first = NULL;
  hashmap->last = NULL;
  hashmap->table = (hashmap_entry_t*)malloc(sizeof(hashmap_entry_t) * hashmap->size);

  entry = hashmap->table;
  for (i = 0; i < hashmap->size; ++i, ++entry) entry->data = NULL;
  
  return hashmap;
}

void hashmap_close(hashmap_t *hashmap)
{
  free(hashmap->table);
  free(hashmap);
}

unsigned int hashmap_count(hashmap_t *hashmap)
{
  return hashmap->count;
}

hashmap_entry_t* hashmap_firstentry(hashmap_t *hashmap)
{
  return hashmap->first;
}

hashmap_entry_t* hashmap_lastentry(hashmap_t *hashmap)
{
  return hashmap->last;
}

hashmap_entry_t* hashmap_getentry(hashmap_t *hashmap, hashmap_key_t hash)
{
  hashmap_key_t tablehash = hash % hashmap->size;
  unsigned int k = 0, size = hashmap->size / 2;
  hashmap_entry_t *entry;
  
  while (k <= size)
  {
    entry = &hashmap->table[tablehash];
    if (hash == entry->hashkey) return entry;
    ++k;
    if (k) tablehash += k + (k - 1);
    if (hashmap->size <= tablehash) tablehash %= hashmap->size;
  }
  
  return NULL;
}

hashmap_entry_t* hashmap_nextentry(hashmap_entry_t *entry)
{
  return entry->next;
}

hashmap_entry_t* hashmap_preventry(hashmap_entry_t *entry)
{
  return entry->prev;
}

void* hashmap_getdatabyentry(hashmap_entry_t *entry)
{
  return (void*)entry->data;
}

hashmap_key_t hashmap_getkeybyentry(hashmap_entry_t *entry)
{
  return entry->hashkey;
}

void* hashmap_getrawkeybyentry(hashmap_entry_t *entry)
{
  return (void*)entry->rawkey;
}

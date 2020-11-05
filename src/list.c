/**
 * list - The list list structure.
 *
 * MIT License
 * Copyright (C) 2011 Nothan
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Nothan
 * nothan@t-denrai.net
 *
 * Tsuioku Denrai
 * http://t-denrai.net/
 */

#include "list.h"
#include <stdlib.h>

#define ITEM_NULL -1

struct _list
{
  int size;
  int block_size;
  int autoresize;
  list_entry_t *pool, *free, *front, *back;
};

struct _list_entry
{
  list_entry_t *next, *prev;
};

static __inline list_entry_t* id2entry(list_t *list, int id)
{
  return (list_entry_t*)(list->pool + id * list->block_size);
}

list_entry_t* list_data2entry(void *src)
{
  return (list_entry_t*)((uintptr_t)src - sizeof(list_entry_t));
}

list_t* list_open(int block_size, int size)
{
  list_t *list = (list_t*)malloc(sizeof(list_t));
  
  list->size = 0;
  list->block_size = sizeof(list_entry_t) + block_size;
  list->pool = NULL;
  list_clear(list);
  list_setautoresize(list, size);
  
  return list;
}

void list_close(list_t *list)
{
  if (list->pool != NULL) free(list->pool);
}

int list_size(list_t *list)
{
  return list->size;
}

void list_setautoresize(list_t *list, int size)
{
  list->autoresize = size;
}

void list_resize(list_t *list, int size)
{
  int i;
  list_entry_t *pool = (list_entry_t*)realloc(list->pool, list->block_size * size);

  if (NULL == pool) return;
  if (size <= list->size) return;
  
  list->pool = pool;
  pool = id2entry(list, list->size);
  for (i = list->size; i < size; ++i)
  {
    pool->next = (list_entry_t*)((uintptr_t)pool + list->block_size);
    pool = pool->next;
  }
  pool = (list_entry_t*)((uintptr_t)pool - list->block_size);
  pool->next = list->free;
  list->free = id2entry(list, list->size);
  list->size = size;
}

void list_clear(list_t *list)
{
  int i;
  list_entry_t *pool = list->pool;
  
  list->free = list->pool;
  list->front = NULL;
  list->back = NULL;
  
  if (0 == list->size) return;
  
  for (i = 0; i < list->size; ++i)
  {
    pool->next = (list_entry_t*)((uintptr_t)pool + list->block_size);
    pool = pool->next;
  }
  pool->next = NULL;
}

static __inline list_entry_t* list_new(list_t *list)
{
  list_entry_t *free = list->free;
  
  if (NULL == free)
  {
    list_resize(list, list->size + list->autoresize);
    free = list->free;
    if (NULL == free) return NULL;
  }
  list->free = free->next;
  
  return free;
}

void* list_pushfront(list_t *list)
{
  list_entry_t *entry = list_frontentry(list);
  
  if (NULL == list->front)
  {
    entry = list->back = list->front = list_new(list);
    entry->prev = NULL;
    entry->next = NULL;
    
    return entry + 1;
  }
  
  return list_pushprev(list, entry);
}

void* list_pushback(list_t *list)
{
  list_entry_t *entry = list_backentry(list);
  
  if (NULL == list->back)
  {
    return list_pushfront(list);
  }
  
  return list_pushnext(list, entry);
}

void list_delete(list_t *list, void *ptr)
{
  list_eraseentry(list, list_data2entry(ptr));
}

void* list_front(list_t *list)
{
  return list->front + 1;
}

void* list_back(list_t *list)
{
  return list->back + 1;
}

list_entry_t* list_frontentry(list_t *list)
{
  return list->front;
}

list_entry_t* list_backentry(list_t *list)
{
  return list->back;
}

void* list_getdata(list_entry_t *entry)
{
  return entry + 1;
}

void* list_pushprev(list_t *list, list_entry_t *entry)
{
  list_entry_t *newentry = list_new(list);
  
  if (NULL == newentry) return NULL;

  if (entry == list->front) list->front = newentry;
  if (NULL != entry->prev)
  {
    entry->prev->next = newentry;
  }
  newentry->prev = entry->prev;
  newentry->next = entry;
  entry->prev = newentry;
  
  return newentry + 1;
}

void* list_pushnext(list_t *list, list_entry_t *entry)
{
  list_entry_t *newentry = list_new(list);
  
  if (NULL == newentry) return NULL;
  
  if (entry == list->back) list->back = newentry;
  if (NULL != entry->next)
  {
    entry->next->prev = newentry;
  }
  newentry->next = entry->next;
  newentry->prev = entry;
  entry->next = newentry;
  
  return newentry + 1;
}

void list_eraseentry(list_t *list, list_entry_t *entry)
{
  if (entry == list->front) list->front = entry->next;
  if (entry == list->back) list->back = entry->prev;
  if (NULL != entry->prev) entry->prev->next = entry->next;
  if (NULL != entry->next) entry->next->prev = entry->prev;
  
  entry->next = list->free;
  list->free = entry;
}

list_entry_t* list_preventry(list_entry_t *entry)
{
  return entry->prev;
}

list_entry_t* list_nextentry(list_entry_t *entry)
{
  return entry->next;
}

int list_count(list_t *list)
{
  int counter = 0;
  list_entry_t *entry;
  
  for (entry = list->front; NULL != entry; entry = list_nextentry(entry))
  {
    ++counter;
  }
  
  return counter;
}

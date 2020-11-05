/**
 * stack - The stack data structure.
 *
 * MIT License
 * Copyright (C) 2010-2011 Nothan
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

#ifndef _STACK_T
#define _STACK_T

#include "stack.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct _stack
{
  int size;
  int blocksize;
  int8_t *pool;
  int8_t *current;
};
typedef struct _stack stack_t;

#ifdef __cplusplus
extern "C" {
#endif

static __inline stack_t* stack_open(int blocksize)
{
  stack_t *stack = malloc(sizeof(stack_t));

  if (NULL == stack) return NULL;
  stack->size = 0;
  stack->blocksize = blocksize;
  stack->current = NULL;
  stack->pool = NULL;
  
  return stack;
}

static __inline void stack_close(stack_t *stack)
{
  if (stack->pool != NULL) free(stack->pool);
}

static __inline void stack_clear(stack_t *stack)
{
  stack->current = stack->pool;
}

static __inline void stack_setblocksize(stack_t *stack, int blocksize)
{
  stack_clear(stack);
  stack->blocksize = blocksize;
}

static __inline void stack_resize(stack_t *stack, int size)
{
  int8_t* pool = (int8_t*)realloc(stack->pool, stack->blocksize * size);
  int used_size = stack->current - stack->pool;
  if (NULL == pool) return;
  
  stack->pool = pool;
  stack->current = stack->pool + used_size;
  stack->size = stack->blocksize * size;
}
  
static __inline int stack_count(stack_t *stack)
{
  return (stack->current - stack->pool) / stack->blocksize;
}

static __inline void* stack_push(stack_t *stack, void *src)
{
  void *dest = stack->current;

  if (stack->current - stack->pool >= stack->size) return NULL;
  stack->current += stack->blocksize;

  if (NULL != src)
  {
    memcpy(dest, src, stack->blocksize);
  }
  
  return dest;
}

static __inline void* stack_pop(stack_t *stack, void *dest)
{
  if (stack->current == stack->pool) return NULL;
  stack->current -= stack->blocksize;

  if (NULL != dest)
  {
    memcpy(dest, stack->current, stack->blocksize);
  }
  
  return stack->current;
}

static __inline void* stack_index(stack_t *stack, int index, void *dest)
{
  int8_t *ptr;
  
  if (index < 0)
  {
    ptr = stack->current + (index * stack->blocksize);
    return ptr >= stack->pool ? ptr : NULL;
  }
  
  ptr = stack->pool + (index * stack->blocksize);

  if (ptr >= stack->current) return NULL;
  if (dest != NULL)
  {
    memcpy(dest, ptr, stack->blocksize);
  }
  
  return ptr;
}

static __inline void* stack_front(stack_t *stack, void *dest)
{
  if (stack->pool == stack->current) return NULL;

  if (NULL != dest)
  {
    memcpy(dest, stack->pool, stack->blocksize);
  }

  return stack->pool;
}

static __inline void* stack_back(stack_t *stack, void *dest)
{
  void *src = stack->current - stack->blocksize;

  if (stack->pool == stack->current) return NULL;

  if (NULL != dest)
  {
    memcpy(dest, src, stack->blocksize);
  }

  return src;
}

#ifdef __cplusplus
}
#endif

#endif

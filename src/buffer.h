/**
 * buffer
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

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <stdlib.h>
#include <string.h>

struct _buffer
{
  int blocksize;
  int resizesize;
  int size;
  int writesize;
  void *pool;
  void *current;
};
typedef struct _buffer buffer_t;

#ifdef __cplusplus
extern "C" {
#endif

static __inline int blocktobyte(const buffer_t *buffer, int size)
{
  return buffer->blocksize * size;
}

static __inline void buffer_autoresize(buffer_t *buffer, int size)
{
  buffer->resizesize = size;
}

static __inline buffer_t* buffer_open(int blocksize, int size)
{
  buffer_t *buffer = malloc(sizeof(buffer_t));

  if (NULL == buffer) return NULL;
  buffer->size = 0;
  buffer->blocksize = blocksize;
  buffer->writesize = 0;
  buffer->resizesize = 0;
  buffer->pool = NULL;
  buffer->current = NULL;

  buffer_autoresize(buffer, size);

  return buffer;
}

static __inline void buffer_close(buffer_t *buffer)
{
  if (buffer->pool != NULL) free(buffer->pool);
  free(buffer);
}

static __inline void buffer_resize(buffer_t *buffer, int size)
{
  if (size <= buffer->size)
  {
    if (size < buffer->writesize)
    {
      buffer->writesize = size;
      buffer->current = (char*)buffer->pool + blocktobyte(buffer, size);
    }
    return;
  }
  buffer->pool = realloc(buffer->pool, blocktobyte(buffer, size));
  buffer->current = (char*)buffer->pool + blocktobyte(buffer, buffer->writesize);
  buffer->size = size;
}

static __inline void buffer_setblocksize(buffer_t *buffer, int blocksize)
{
  float ratio;
  
  if (buffer->blocksize == blocksize) return;
  if (buffer->writesize)
  {
    ratio = (float)buffer->blocksize / (float)blocksize;
    buffer->size *= (int)ratio;
  }
  buffer->blocksize = blocksize;
}

static __inline void buffer_clear(buffer_t *buffer)
{
  buffer->writesize = 0;
  buffer->current = buffer->pool;
}

static __inline void checksize(buffer_t *buffer, int size)
{
  int resizesize = size + buffer->writesize;

  if (resizesize >= buffer->size)
  {
    if (size < buffer->resizesize)
    {
      resizesize = buffer->resizesize + buffer->writesize;
    }
    buffer_resize(buffer, resizesize);
  }
}

static __inline void* buffer_front(const buffer_t *buffer)
{
  return buffer->writesize ? buffer->pool : NULL;
}

static __inline void* buffer_back(const buffer_t *buffer)
{
  return buffer->writesize ? (char*)buffer->current - buffer->blocksize : NULL;
}

static __inline void* buffer_index(const buffer_t *buffer, int index)
{
  void *ptr;
  
  if (index < 0)
  {
    ptr = (char*)buffer->current + blocktobyte(buffer, index);
    return ptr >= buffer->pool ? ptr : NULL;
  }
  
  ptr = (char*)buffer->pool + blocktobyte(buffer, index);
  
  return ptr < buffer->current ? ptr : NULL;
}

static __inline void* buffer_malloc(buffer_t *buffer, int size)
{
  void *buff;
  
  checksize(buffer, size);
  buff = buffer->current;
  buffer->writesize += size;
  buffer->current = (char*)buffer->current + blocktobyte(buffer, size);
  
  return buff;
}

static __inline void buffer_write(buffer_t *buffer, const void *src, int size)
{
  void *buff = buffer_malloc(buffer, size);
  memcpy(buff, src, blocktobyte(buffer, size));
}

static __inline void buffer_erase(buffer_t *buffer, int size)
{
  buffer->writesize -= size;
  if (buffer->writesize <= 0)
  {
    buffer_clear(buffer);
  }
  else
  {
    buffer->current = (char*)buffer->current - blocktobyte(buffer, size);
  }
}

static __inline int buffer_bytesize(const buffer_t *buffer)
{
  return (char*)buffer->current - (char*)buffer->pool;
}

static __inline void* buffer_memcpy(const buffer_t *buffer, void *dest)
{
  return memcpy(dest, buffer->pool, buffer_bytesize(buffer));
}

static __inline int buffer_size(const buffer_t *buffer)
{
  return buffer->writesize;
}

static __inline int buffer_freesize(const buffer_t *buffer)
{
  return buffer->size - buffer->writesize;
}

static __inline int buffer_empty(const buffer_t *buffer)
{
  return !buffer_size(buffer);
}

#ifdef __cplusplus
}
#endif

#endif

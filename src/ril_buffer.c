/*	see copyright notice in ril.h */

#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

buffer_t* ril_buffer_open(int blocksize, int size)
{
  return buffer_open(blocksize, size);
}

void ril_buffer_close(buffer_t *buffer)
{
  buffer_close(buffer);
}

void ril_buffer_resize(buffer_t *buffer, int size)
{
  buffer_resize(buffer, size);
}

void ril_buffer_setblocksize(buffer_t *buffer, int size)
{
  buffer_setblocksize(buffer, size);
}

void ril_buffer_autoresize(buffer_t *buffer, int size)
{
  buffer_autoresize(buffer, size);
}

void ril_buffer_clear(buffer_t *buffer)
{
  buffer_clear(buffer);
}

void* ril_buffer_front(const buffer_t *buffer)
{
  return buffer_front(buffer);
}

void* ril_buffer_back(const buffer_t *buffer)
{
  return buffer_back(buffer);
}

void* ril_buffer_index(const buffer_t *buffer, int index)
{
  return buffer_index(buffer, index);
}

void ril_buffer_write(buffer_t *buffer, const void *src, int size)
{
  buffer_write(buffer, src, size);
}

void* ril_buffer_malloc(buffer_t *buffer, int size)
{
  return buffer_malloc(buffer, size);
}

void ril_buffer_erase(buffer_t *buffer, int size)
{
  buffer_erase(buffer, size);
}

void* ril_buffer_memcpy(const buffer_t *buffer, void *dest)
{
  return buffer_memcpy(buffer, dest);
}

int ril_buffer_size(const buffer_t *buffer)
{
  return buffer_size(buffer);
}

int ril_buffer_bytesize(const buffer_t *buffer)
{
  return buffer_bytesize(buffer);
}

int ril_buffer_freesize(const buffer_t *buffer)
{
  return buffer_freesize(buffer);
}

#ifdef __cplusplus
}
#endif

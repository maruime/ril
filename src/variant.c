/**
 * variant
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

#include "variant.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

variant_t* variant_new(void)
{
  variant_t *v = (variant_t*)malloc(sizeof(variant_t));
  variant_init(v);
  
  return v;
}

void variant_delete(variant_t *v)
{
  variant_clear(v);
  free(v);
}

void variant_init(variant_t *v)
{
  v->type = VARIANT_NULL;
}

void variant_setstring(variant_t *v, const char *value)
{
  int size = strlen(value) + 1;
  
  variant_clear(v);
  
  v->type = VARIANT_STRING;
  v->ptr_value = malloc(size);
  memcpy(v->ptr_value, value, size);
}

void variant_setinteger(variant_t *v, int value)
{
  variant_clear(v);
  
  v->type = VARIANT_INTEGER;
  v->int_value = value;
}

void variant_setfloat(variant_t *v, float value)
{
  variant_clear(v);
  
  v->type = VARIANT_REAL;
  v->real_value = value;
}

void variant_setbytes(variant_t *v, const void *value, int size)
{
  variant_clear(v);
  
  v->type = VARIANT_BYTES;
  v->ptr_value = (char*)malloc(sizeof(uint32_t) + size);
  memcpy(v->ptr_value, &size, sizeof(size));
  memcpy((int8_t*)v->ptr_value + sizeof(size), value, size);
}

void variant_copy(variant_t *dest, const variant_t *src)
{
  switch ((src->type | VARIANT_LITERAL) ^ VARIANT_LITERAL)
  {
  case VARIANT_STRING:
    variant_setstring(dest, src->string_value);
    break;
  case VARIANT_BYTES:
    variant_setbytes(dest, (int8_t*)src->ptr_value + sizeof(uint32_t), *(uint32_t*)src->ptr_value);
    break;
  default:
    variant_clear(dest);
    dest->ptr_value = src->ptr_value;
    dest->type = src->type;
    break;
  }
}

void variant_clear(variant_t *v)
{
  switch (v->type)
  {
  case VARIANT_STRING:
    v->type = VARIANT_NULL;
    free(v->string_value);
    break;
  case VARIANT_BYTES:
    v->type = VARIANT_NULL;
    free(v->ptr_value);
    break;
  case VARIANT_REAL:
  case VARIANT_INTEGER:
    v->type = VARIANT_NULL;
    break;
  }
}

const char* variant_getstring(variant_t *v)
{
  static char buf[128];
  
  switch ((v->type | VARIANT_LITERAL) ^ VARIANT_LITERAL)
  {
  case VARIANT_STRING:
    return v->string_value;
  case VARIANT_INTEGER:
    sprintf(buf, "%d", v->int_value);
    return buf;
  case VARIANT_REAL:
    sprintf(buf, "%f", v->real_value);
    return buf;
  }
  
  return "";
}

int variant_getinteger(variant_t *v)
{
  switch ((v->type | VARIANT_LITERAL) ^ VARIANT_LITERAL)
  {
  case VARIANT_INTEGER: return v->int_value;
  case VARIANT_REAL: return (int)v->real_value;
  case VARIANT_STRING: return atoi(v->string_value);
  }
  
  return 0;
}

float variant_getfloat(variant_t *v)
{
  switch ((v->type | VARIANT_LITERAL) ^ VARIANT_LITERAL)
  {
  case VARIANT_REAL: return v->real_value;
  case VARIANT_INTEGER: return (float)v->int_value;
  case VARIANT_STRING: return (float)atof(v->string_value);
  }
  
  return 0;
}

void* variant_getbytes(variant_t *v)
{
  switch ((v->type | VARIANT_LITERAL) ^ VARIANT_LITERAL)
  {
  case VARIANT_BYTES: return (int8_t*)v->ptr_value + sizeof(uint32_t);
  case VARIANT_REAL:
  case VARIANT_INTEGER:
    return &v->ptr_value;
  case VARIANT_STRING: return v->string_value;
  }
  
  return 0;
}

int variant_getsize(variant_t *v)
{
  switch (v->type)
  {
  case VARIANT_BYTES: return *(uint32_t*)v->ptr_value;
  case VARIANT_REAL: return sizeof(v->real_value);
  case VARIANT_INTEGER: return sizeof(v->int_value);
  case VARIANT_STRING: return strlen(v->string_value) + 1;
  }
  
  return 0;
}

int variant_unselialize(variant_t *v, const void *src)
{
  const void *cur = src;
  int type = *(uint8_t*)cur, size;
  
  cur = (uint8_t*)cur + 1;
  
  switch (type)
  {
  case VARIANT_BYTES:
    size = *(uint32_t*)cur;
    cur = (uint8_t*)cur + 4;
    variant_setbytes(v, cur, size);
    cur = (uint8_t*)cur + size;
    break;
  case VARIANT_REAL:
    variant_setfloat(v, *(float*)cur);
    cur = (uint8_t*)cur + sizeof(float);
    break;
  case VARIANT_INTEGER:
    variant_setinteger(v, *(int*)cur);
    cur = (uint8_t*)cur + sizeof(int);
    break;
  case VARIANT_STRING:
    size = *(uint32_t*)cur;
    cur = (uint8_t*)cur + 4;
    variant_setstring(v, cur);
    cur = (uint8_t*)cur + size;
    break;
  default:
    variant_clear(v);
    v->type = type;
  }
  
  return (uintptr_t)cur - (uintptr_t)src;
}

void variant_selialize(variant_t *v, buffer_t *buffer)
{
  uint32_t size;
  uint8_t type = v->type;
  
  buffer_write(buffer, &type, sizeof(type));
  
  switch (v->type)
  {
  case VARIANT_BYTES:
    size = *(uint32_t*)v->ptr_value;
    buffer_write(buffer, &size, sizeof(size));
    buffer_write(buffer, (int8_t*)v->ptr_value + sizeof(size), size);
    break;
  case VARIANT_REAL:
    buffer_write(buffer, &v->real_value, sizeof(float));
    break;
  case VARIANT_INTEGER:
    buffer_write(buffer, &v->int_value, sizeof(int));
    break;
  case VARIANT_STRING:
    buffer_write(buffer, v->string_value, variant_getsize(v));
    break;
  }
}

int variant_type(variant_t *v)
{
  return v->type;
}

int variant_istype(variant_t *v, int type)
{
  return v->type == type;
}

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

#ifndef _VARIANT_H_
#define _VARIANT_H_

#include "buffer.h"

enum
{
  VARIANT_NULL            = 0x00000000,
  VARIANT_LITERAL        = 0x10000000,
  VARIANT_NUMBER        = 0x01000000,
  VARIANT_STRING         = 0x02000000,
  VARIANT_VAR               = 0x04000000,
  VARIANT_INTEGER       = VARIANT_NUMBER | 0x00000001,
  VARIANT_REAL             = VARIANT_NUMBER | 0x00000002,
  VARIANT_STRINGOBJ    = VARIANT_STRING | 0x00000002,
  VARIANT_PTR                = 0x00000001,
  VARIANT_BYTES            = 0x00000002,
  VARIANT_LABEL            = 0x00000004,
  VARIANT_REFVAR          = VARIANT_VAR | 0x00000001,
  VARIANT_ARRAY            = 0x00000010,
};

typedef struct _variant
{
  int type;
  union {
    int int_value;
    float real_value;
    char* string_value;
    void* ptr_value;
  };
} variant_t;

#ifdef __cplusplus
extern "C" {
#endif
  
variant_t* variant_new(void);
void variant_delete(variant_t *v);
void variant_init(variant_t *v);
void variant_setstring(variant_t *v, const char *value);
void variant_setinteger(variant_t *v, int value);
void variant_setfloat(variant_t *v, float value);
void variant_setbytes(variant_t *v, const void *value, int size);
void variant_copy(variant_t *dest, const variant_t *src);
void variant_clear(variant_t *v);

const char* variant_getstring(variant_t *v);
int variant_getinteger(variant_t *v);
float variant_getfloat(variant_t *v);
void* variant_getbytes(variant_t *v);
int variant_getsize(variant_t *v);
  
int variant_unselialize(variant_t *v, const void *src);
void variant_selialize(variant_t *v, buffer_t *buffer);
  
int variant_type(variant_t *v);
int variant_istype(variant_t *v, int type);
  
#ifdef __cplusplus
}
#endif

#endif

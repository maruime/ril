/**
 * list - The list data structure.
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

#ifndef _LIST_H_
#define _LIST_H_

struct _list;
struct _list_entry;
typedef struct _list list_t;
typedef struct _list_entry list_entry_t;

#ifdef __cplusplus
extern "C" {
#endif

list_t* list_open(int, int size);
void list_close(list_t*);
int list_size(list_t*);
void list_setautoresize(list_t *list, int size);
void list_resize(list_t*, int);
void list_clear(list_t*);
void* list_pushfront(list_t *list);
void* list_pushback(list_t *list);
void list_delete(list_t*, void*);
int list_count(list_t *list);
void* list_front(list_t *list);
void* list_back(list_t *list);
list_entry_t* list_frontentry(list_t *list);
list_entry_t* list_backentry(list_t *list);
  
list_entry_t* list_data2entry(void *src);
void* list_pushprev(list_t *list, list_entry_t *entry);
void* list_pushnext(list_t *list, list_entry_t *entry);
void list_eraseentry(list_t *list, list_entry_t *entry);
void* list_getdata(list_entry_t *entry);
list_entry_t* list_preventry(list_entry_t *entry);
list_entry_t* list_nextentry(list_entry_t *entry);

#ifdef __cplusplus
}
#endif

#endif

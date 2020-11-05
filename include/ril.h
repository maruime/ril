/**
 * ril - Ril Scripting language
 *
 * MIT License
 * Copyright (C) 2011 Nothan
 * All rights reserved.
 *
 * Permission is hereby granted, free of charile, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merile, publish, distribute, sublicense, and/or sell
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

#ifndef _RIL_H_
#define _RIL_H_
#define _BUFFER_H_

#include <stdint.h>

#ifndef __cplusplus
#ifdef _WIN32
#pragma warning(disable:4996)
#define true 1
#define false 0
typedef int bool;
#else
#include <stdbool.h>
#endif
#endif

#ifndef RIL_API
#define RIL_API extern
#endif

#define RIL_OK (0)
#define RIL_ERROR (-1)

#define RIL_FAILED(res) (res < 0)
#define RIL_SUCCEEDED(res) (res >= 0)

#define RIL_ISTAG(vm, cmdid, signature) (signature == ril_cmdid2signature(vm, cmdid))

#define RIL_CALLFUNC(x) ril_func_##x
#define RIL_CALLCOMPILEFUNC(name) ril_compilefunc_##name
#define RIL_CALLSAVEFUNC(x) ril_savefunc_##x
#define RIL_CALLLOADFUNC(x) ril_loadfunc_##x
#define RIL_CALLDELETEFUNC(x) ril_deletefunc_##x

#define RIL_FUNC(name, vm) int RIL_CALLFUNC(name)(RILVM vm)
#define RIL_COMPILEFUNC(name, context) RILRESULT RIL_CALLCOMPILEFUNC(name)(ril_compile_t *context)
#define RIL_SAVEFUNC(name, vm, dest) RILRESULT RIL_CALLSAVEFUNC(name)(RILVM vm, ril_buffer_t *dest)
#define RIL_LOADFUNC(name, vm, src) RILRESULT RIL_CALLLOADFUNC(name)(RILVM vm, const void *src)
#define RIL_DELETEFUNC(name, vm) RILRESULT RIL_CALLDELETEFUNC(name)(RILVM vm)

#define RIL_REGISTERTAG(vm, name, args) ril_registertag(vm, #name, args, RIL_CALLFUNC(name))
#define RIL_SETCOMPILEHANDLER(tag, name) ril_setcompilehandler(tag, RIL_CALLCOMPILEFUNC(name))
#define RIL_SETSTORAGE(tag, hname) ril_setstoragehandler(tag, RIL_CALLSAVEFUNC(hname), RIL_CALLLOADFUNC(hname), RIL_CALLDELETEFUNC(hname))

enum
{
  RIL_NULL = 0,
  RIL_STOP,
  RIL_EXIT,
  RIL_NEXT,
  RIL_NEXTPAIR,
  RIL_BREAKPAIR,
  RIL_FIRSTPAIR,
  RIL_LASTPAIR,
};

enum
{
  RIL_TAG_CH = 0x819B9DD9,
  RIL_TAG_R = 0xC90C2086,
  RIL_TAG_GOTO = 0x1A0818D6,
  RIL_TAG_GOTOFILE = 0x3A9C2995,
  RIL_TAG_GOSUB = 0x6BC6D625,
  RIL_TAG_GOSUBFILE = 0x832E2D6C,
  RIL_TAG_EXIT = 0x3A2FCC45,
  RIL_TAG_LABEL = 0xB672AD4F,
  RIL_TAG_RETURN = 0x2302A933,
};

struct _ril_compile;
struct _ril_vm;
struct _ril_state;
struct _ril_var;
struct _ril_tagarg;
struct _ril_tag;
struct _ril_label;
struct _ril_code;
struct _buffer;
struct _ril_vmcmd;

typedef int ril_int_t;
typedef unsigned int ril_uint_t;
typedef float ril_float_t;
typedef struct _buffer ril_buffer_t;
typedef int RILRESULT;
typedef uint32_t ril_crc_t;
typedef ril_crc_t ril_signature_t;
typedef struct {uint8_t buf[16];} ril_md5_t;
typedef struct _ril_compile ril_compile_t;
typedef struct _ril_vm ril_vm_t;
typedef struct _ril_state ril_state_t;
typedef ril_vm_t* RILVM;
typedef struct _ril_var ril_var_t;
typedef struct _ril_tagarg ril_tagarg_t;
typedef struct _ril_tag ril_tag_t;
typedef struct _ril_label ril_label_t;
typedef struct _ril_code ril_code_t;
typedef void (*RILERROR)(RILVM, const char*);
typedef int (*RILFUNCTION)(RILVM);
typedef RILRESULT (*RILCOMPILEFUNCTION)(ril_compile_t*);
typedef RILRESULT (*RILSAVEFUNCTION)(RILVM vm, ril_buffer_t *dest);
typedef int (*RILLOADFUNCTION)(RILVM vm, const void *src);
typedef int (*RILDELETEFUNCTION)(RILVM);

typedef struct
{
  int id;
} ril_cmdid_t;

#ifdef __cplusplus
extern "C" {
#endif

/* vm */
RIL_API RILVM ril_open(void);
RIL_API void ril_close(RILVM vm);
RIL_API void ril_setpath(RILVM vm, const char *path);
RIL_API RILRESULT ril_setdelimiter(RILVM vm, const char *left, const char *right);
RIL_API bool ril_isleftdelimiter(RILVM vm, const char *src);
RIL_API bool ril_isrightdelimiter(RILVM vm, const char *src);
RIL_API RILRESULT ril_load(RILVM vm, const void *src, int size);
RIL_API RILRESULT ril_loadfile(RILVM vm, const char *file);
RIL_API RILRESULT ril_loadbytefile(RILVM vm, const char *file);
RIL_API int ril_docmd(RILVM vm, ril_cmdid_t cmd);
RIL_API int ril_execute(RILVM vm);
RIL_API int ril_calltag(RILVM vm, ril_tag_t *tag);
RIL_API int ril_callmacro(RILVM vm, ril_tag_t *tag);
RIL_API const char* ril_tagname(ril_tag_t *tag);
RIL_API ril_signature_t ril_signature(ril_tag_t *tag);
RIL_API const char* ril_getparametername(const ril_buffer_t *buffer, int index);
RIL_API const char* ril_getparametervalue(const ril_buffer_t *buffer, int index);
RIL_API RILRESULT ril_parseparameters(ril_buffer_t *buffer, const char *args);
RIL_API ril_signature_t ril_makesignature(const char *name, const char *args);
RIL_API ril_signature_t ril_makesignature2(const char *name, const ril_buffer_t *parameterbuffer);
RIL_API void ril_makesignaturestring(ril_buffer_t *dest, const char *name, const ril_buffer_t *parameter);
RIL_API ril_tag_t* ril_registertag(RILVM vm, const char *name, const char *args, RILFUNCTION func);
RIL_API ril_tag_t* ril_setpairtag(ril_tag_t *fc, ril_tag_t *pair_fc);
RIL_API ril_tag_t* ril_setchildtag(ril_tag_t *parent, ril_tag_t *child);
RIL_API RILFUNCTION ril_getexecutehandler(ril_tag_t *tag);
RIL_API void ril_setexecutehandler(ril_tag_t* t, RILFUNCTION handler);
RIL_API void ril_setcompilehandler(ril_tag_t* t, RILCOMPILEFUNCTION handler);
RIL_API void ril_setstoragehandler(ril_tag_t* t, RILSAVEFUNCTION savehandler, RILLOADFUNCTION loadhandler, RILDELETEFUNCTION deletehandler);
RIL_API void ril_useworkarea(ril_tag_t *t, bool value);
RIL_API void ril_seterrorhandler(RILVM vm, RILERROR func);
RIL_API void ril_setuserdata(RILVM vm, void *userdata);
RIL_API void* ril_userdata(RILVM vm);
RIL_API bool ril_isfirst(RILVM vm);
RIL_API ril_tag_t* ril_getregisteredtag(RILVM vm, const char *name, const char *args);
RIL_API ril_tag_t* ril_getregisteredtag2(RILVM vm, ril_signature_t signature);
RIL_API ril_tag_t* ril_getregisteredtag3(RILVM vm, const char *name, const char *args, bool hasnonamearg, bool iscmp);
RIL_API void ril_setnextcmdbyid(RILVM vm, ril_cmdid_t cmdid);
RIL_API void* ril_malloc(int size);
RIL_API void* ril_realloc(void *ptr, int size);
RIL_API void ril_free(void *ptr);
RIL_API void ril_setfilename(RILVM vm, const char *file);
  
RIL_API void ril_ch(RILVM vm, ril_var_t *var);
RIL_API const ril_label_t* ril_getlabel(RILVM vm, ril_crc_t name_hash);
RIL_API RILRESULT ril_gotolabel(RILVM vm, const char *name);
RIL_API RILRESULT ril_gotolabelbyhash(RILVM vm, ril_crc_t hash);
RIL_API RILRESULT ril_goto(RILVM vm, ril_cmdid_t cmd);
RIL_API ril_cmdid_t ril_cmd(RILVM vm);
RIL_API void ril_nextcmd(RILVM vm);
RIL_API void ril_prevcmd(RILVM vm);
RIL_API ril_tag_t* ril_cmdid2tag(RILVM vm, ril_cmdid_t cmdid);
RIL_API ril_signature_t ril_cmdid2signature(RILVM vm, ril_cmdid_t cmdid);
RIL_API ril_tag_t* ril_currenttag(RILVM vm);
RIL_API void* ril_mallocworkarea(RILVM vm, int size);
RIL_API void ril_eraseworkarea(RILVM vm, int size);
RIL_API void* ril_workarea(RILVM vm);
RIL_API void ril_releaseworkarea(RILVM vm, ril_tag_t *tag);
RIL_API void ril_setshareddata(ril_tag_t *tag, void *data);
RIL_API void* ril_getshareddata(ril_tag_t *tag);

/* state */
RIL_API ril_state_t* ril_newstate(RILVM vm);
RIL_API void ril_deletestate(ril_state_t *state);
RIL_API void ril_clearstate(ril_state_t *state);
RIL_API void ril_setstate(ril_state_t *state);
RIL_API ril_state_t* ril_getstate(RILVM vm);
RIL_API void ril_setmainstate(RILVM vm);
RIL_API RILRESULT ril_savestate(RILVM vm, ril_buffer_t *dest);
RIL_API int ril_loadstate(RILVM vm, const void *src);
RIL_API void ril_pushlocalvar(RILVM vm);
RIL_API ril_var_t* ril_addlocalvar(RILVM vm, ril_crc_t key);
RIL_API void ril_poplocalvar(RILVM vm);
RIL_API void ril_savelocalvar(RILVM vm, ril_buffer_t *buffer);
RIL_API int ril_loadlocalvar(RILVM vm, const void *src);
RIL_API RILRESULT ril_cleararguments(ril_state_t *state);
RIL_API RILRESULT ril_copyarguments(RILVM vm, ril_state_t *dest);
RIL_API int ril_countarguments(RILVM vm);
RIL_API RILRESULT ril_setarguments(RILVM vm, ril_cmdid_t cmd);
RIL_API ril_var_t* ril_getargument(RILVM vm, int index);
RIL_API const void* ril_getptr(RILVM vm, int index);
RIL_API const char* ril_getstring(RILVM vm, int index);
RIL_API bool ril_getbool(RILVM vm, int index);
RIL_API int ril_getinteger(RILVM vm, int index);
RIL_API float ril_getfloat(RILVM vm, int index);
RIL_API int ril_has(RILVM vm, int index);
RIL_API void ril_set(RILVM vm, ril_var_t *var);
RIL_API void ril_return(RILVM vm, ril_var_t *var);
RIL_API void ril_returninteger(RILVM vm, int value);
RIL_API void ril_returnfloat(RILVM vm, float value);
RIL_API void ril_returnstring(RILVM vm, const char *value);
RIL_API RILVM ril_state2vm(ril_state_t *state);

/* standard tags*/
RIL_API void ril_pushfunction(RILVM vm, ril_tag_t *tag, RILFUNCTION executefunc, RILSAVEFUNCTION savefunc, RILLOADFUNCTION loadfunc, RILDELETEFUNCTION deletefunc, void *userdata);
RIL_API void* ril_popfunction(RILVM vm, void *src);
RIL_API RIL_FUNC(label, vm);
RIL_API RIL_FUNC(set, vm);
RIL_API RIL_FUNC(unset, vm);
RIL_API RIL_FUNC(isnull, vm);
RIL_API RIL_FUNC(isint, vm);
RIL_API RIL_FUNC(isreal, vm);
RIL_API RIL_FUNC(isarray, vm);
RIL_API RIL_FUNC(isstring, vm);
RIL_API RIL_FUNC(std_ch, vm);
RIL_API RIL_FUNC(std_r, vm);
RIL_API RIL_FUNC(exit, vm);
RIL_API RIL_FUNC(next, vm);
RIL_API RIL_FUNC(goto, vm);
RIL_API RIL_FUNC(gotofile, vm);
RIL_API RIL_FUNC(gosub, vm);
RIL_API RIL_FUNC(gosubfile, vm);
RIL_API RIL_FUNC(if, vm);
RIL_API RIL_FUNC(else, vm);
RIL_API RIL_FUNC(elseif, vm);
RIL_API RIL_FUNC(endif, vm);
RIL_API RIL_FUNC(let, vm);
RIL_API RIL_COMPILEFUNC(macro, context);
RIL_API RIL_FUNC(macro, vm);
RIL_API RIL_FUNC(endmacro, vm);
RIL_API RIL_FUNC(callmacro, vm);
RIL_API RIL_SAVEFUNC(callmacro, dest, src);
RIL_API RIL_LOADFUNC(callmacro, dest, src);
RIL_API RIL_DELETEFUNC(callmacro, vm);
RIL_API RIL_FUNC(return, vm);
RIL_API RIL_SAVEFUNC(return, dest, src);
RIL_API RIL_LOADFUNC(return, dest, src);
RIL_API RIL_DELETEFUNC(return, vm);
RIL_API RIL_FUNC(break, vm);
RIL_API RIL_FUNC(continue, vm);
RIL_API RIL_COMPILEFUNC(while, context);
RIL_API RIL_FUNC(while, vm);
RIL_API RIL_FUNC(endwhile, vm);
RIL_API RIL_FUNC(do, vm);
RIL_API RIL_FUNC(dowhile, vm);
RIL_API RIL_FUNC(foreach, vm);
RIL_API RIL_FUNC(endforeach, vm); 
RIL_API RIL_SAVEFUNC(foreach, dest, src);
RIL_API RIL_LOADFUNC(foreach, dest, src);
RIL_API RIL_DELETEFUNC(foreach, vm);
RIL_API RIL_FUNC(const, vm);
RIL_API RIL_COMPILEFUNC(include, context);
RIL_API RIL_FUNC(stream, vm);
RIL_API RIL_FUNC(endstream, vm);
RIL_API RIL_SAVEFUNC(stream, dest, src);
RIL_API RIL_LOADFUNC(stream, dest, src);
RIL_API RIL_DELETEFUNC(stream, vm);
RIL_API RIL_SAVEFUNC(null, dest, src);
RIL_API RIL_LOADFUNC(null, dest, src);
RIL_API RIL_DELETEFUNC(null, vm);
RIL_API RIL_SAVEFUNC(int, dest, src);
RIL_API RIL_LOADFUNC(int, dest, src);
RIL_API RIL_DELETEFUNC(int, vm);
RIL_API RIL_COMPILEFUNC(literal, context);
RIL_API RIL_FUNC(count, vm);
RIL_API RIL_FUNC(file, vm);
RIL_FUNC(substr, vm);
RIL_FUNC(strlen, vm);
RIL_FUNC(strtok, vm);
RIL_FUNC(ini, vm);
RIL_FUNC(writevar, vm);
RIL_FUNC(readvar, vm);

/* compiler */
RIL_API RILRESULT ril_compile(RILVM vm, const char *text, ril_buffer_t *dest);
RIL_API RILRESULT ril_compilefile(RILVM vm, const char *file, ril_buffer_t *dest);
RIL_API const char* rilc_getstring(ril_compile_t *context, uint32_t argid);
RIL_API void rilc_eraselastcmd(ril_compile_t *context);;
RIL_API void rilc_addarg(ril_compile_t *context);
RIL_API void rilc_addbytes(ril_compile_t *context, const void *value, int size);
RIL_API void rilc_addstring(ril_compile_t *context, const char *value);
RIL_API RILRESULT rilc_compile(const char *src, ril_compile_t *context);

/* variable */
RIL_API ril_var_t* ril_getvar(RILVM vm, ril_var_t *parent, const char *name);
RIL_API ril_var_t* ril_getvarbyhash(RILVM vm, ril_var_t *parent, ril_crc_t name_hash);
RIL_API ril_var_t* ril_newvar(RILVM vm);
RIL_API ril_var_t* ril_createvarbyhash(RILVM vm, ril_var_t *parent, const char *name, ril_crc_t hashkey);
RIL_API ril_var_t* ril_createvar(RILVM vm, ril_var_t *parent, const char *name);
RIL_API ril_var_t* ril_set2arraybyhash(RILVM vm, ril_var_t *parent, ril_var_t *var, const char *name, ril_crc_t hashkey);
RIL_API ril_var_t* ril_set2array(RILVM vm, ril_var_t *parent, ril_var_t *var, const char *name);
RIL_API void ril_unset2array(RILVM vm, ril_var_t *parent, ril_crc_t hashkey);
RIL_API void ril_initvar(RILVM vm, ril_var_t *var);
RIL_API void ril_retainvar(ril_var_t *var);
RIL_API void ril_deletevar(RILVM vm, ril_var_t *var);
RIL_API void ril_clearvar(RILVM vm, ril_var_t *var);
RIL_API void ril_cleararray(RILVM vm, ril_var_t *var);
RIL_API void ril_setinteger(RILVM vm, ril_var_t *var, const int value);
RIL_API void ril_setfloat(RILVM vm, ril_var_t *var, const float value);
RIL_API void ril_setstring(RILVM vm, ril_var_t *var, const char *value);
RIL_API void ril_setstringbysize(RILVM vm, ril_var_t *var, const char *value, int size);
RIL_API void ril_copyvar(RILVM vm, ril_var_t *dest, ril_var_t *src);
RIL_API void ril_setconst(ril_var_t *var);
RIL_API const char* ril_var2string(RILVM vm, ril_var_t *var);
RIL_API int ril_var2integer(RILVM vm, ril_var_t *var);
RIL_API float ril_var2float(RILVM vm, ril_var_t *var);
RIL_API bool ril_var2bool(RILVM vm, ril_var_t *var);
RIL_API RILRESULT ril_writevar(ril_buffer_t*, ril_var_t *var);
RIL_API int ril_readvar(RILVM vm, ril_var_t *var, const void *src);
RIL_API int ril_countvar(ril_var_t *var);
RIL_API RILRESULT ril_fetchlocalvar(RILVM vm);
RIL_API void ril_fetchglobalvar(RILVM vm);
RIL_API bool ril_isnull(ril_var_t *var);
RIL_API bool ril_isint(ril_var_t *var);
RIL_API bool ril_isreal(ril_var_t *var);
RIL_API bool ril_isarray(ril_var_t *var);
RIL_API bool ril_isstring(ril_var_t *var);
RIL_API void ril_retain(ril_var_t *var);

/* utils */
RIL_API ril_crc_t ril_makecrc(const char *str);

/* buffer */
RIL_API ril_buffer_t* ril_buffer_open(int blocksize, int size);
RIL_API void ril_buffer_close(ril_buffer_t*);
RIL_API void ril_buffer_resize(ril_buffer_t*, int);
RIL_API void ril_buffer_setblocksize(ril_buffer_t*, int);
RIL_API void ril_buffer_autoresize(ril_buffer_t *buffer, int size);
RIL_API void ril_buffer_clear(ril_buffer_t *buffer);
RIL_API void* ril_buffer_front(const ril_buffer_t *buffer);
RIL_API void* ril_buffer_back(const ril_buffer_t *buffer);
RIL_API void* ril_buffer_index(const ril_buffer_t *buffer, int index);
RIL_API void ril_buffer_write(ril_buffer_t *buffer, const void *src, int size);
RIL_API void* ril_buffer_malloc(ril_buffer_t *buffer, int size);
RIL_API void ril_buffer_erase(ril_buffer_t *buffer, int size);
RIL_API void* ril_buffer_memcpy(ril_buffer_t *buffer, void*);
RIL_API int ril_buffer_size(const ril_buffer_t *buffer);
RIL_API int ril_buffer_bytesize(const ril_buffer_t *buffer);
RIL_API int ril_buffer_freesize(const ril_buffer_t *buffer);
  
#ifdef __cplusplus
}
#endif

#endif

/*	see copyright notice in ril.h */

#ifndef _RIL_UTILS_H_
#define _RIL_UTILS_H_

enum {
  RIL_LITTLE_ENDIAN = 0,
  RIL_BIG_ENDIAN,
};

#ifdef __cplusplus
extern "C" {
#endif

void* ril_write(void *dest, const void *src, uint32_t size);
const void* ril_read(void *dest, const void *src, uint32_t size);
ril_crc_t ril_makecrc(const char *str);
bool ril_md5cmp(ril_md5_t a, ril_md5_t b);
char* ril_getword(char *dest, const char *src, bool trimspace);
char* ril_moveto(const char *src, const char code);
char* ril_trimspace(const char *src);
char* ril_movetoeol(const char *src);
char* ril_nextline(const char *src);
uint8_t ril_endian(void);
RILRESULT ril_error(RILVM vm, const char *s, ...);
char* ril_readfile(const char *file);
size_t ril_writefile(const char *file, const void *src, int size);
const char* ril_getpath(RILVM vm, const char *file);
void ril_str2lower(char *dest, const char *src);
void ril_str2upper(char *dest, const char *src);

static __inline ril_vmcmd_t* _cmdid2cmd(RILVM vm, ril_cmdid_t cmdid)
{
  return vm->code.cmd + cmdid.id;
}

static __inline ril_cmdid_t _cmd2cmdid(RILVM vm, const ril_vmcmd_t *cmd)
{
  ril_cmdid_t cmdid;
  cmdid.id = ((uintptr_t)cmd - (uintptr_t)vm->code.cmd) / sizeof(ril_vmcmd_t);
  
  return cmdid;
}

int ril_mbstrlen(const char*str);

#ifdef __cplusplus
}
#endif
  
#endif

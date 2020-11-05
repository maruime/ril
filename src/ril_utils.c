/*	see copyright notice in ril.h */

#include "ril_pcheader.h"
#include "ril_vm.h"
#include "ril_utils.h"
#include "crc.h"
#include <stdarg.h>
#include <locale.h>

void* ril_write(void *dest, const void *src, uint32_t size)
{
  memcpy(dest, src, size);
  return (int8_t*)dest + size;
}

const void* ril_read(void *dest, const void *src, uint32_t size)
{
  memcpy(dest, src, size);
  return (int8_t*)src + size;
}

ril_crc_t ril_makecrc(const char *str)
{
  return crc(str, strlen(str), 0);
}

bool ril_md5cmp(ril_md5_t a, ril_md5_t b)
{
  return memcmp(a.buf, b.buf, 16);
}

char* ril_getword(char *dest, const char *src, bool trimspace)
{
  char *dest_cur = dest;
  int readbyte;

  if (trimspace) src = ril_trimspace(src);

  for (; '\0' != *src; src += readbyte)
  {
    readbyte = mblen(src, MB_CUR_MAX);

    if (readbyte > 1 || isalpha((unsigned char)*src) ||
        '_' == *src ||
        (dest_cur != dest && isdigit((unsigned char)*src)))
    {
      memcpy(dest_cur, src, readbyte);
      dest_cur += readbyte;
      continue;
    }
    if (dest_cur != dest) break;
    break;
  }
  
  *dest_cur = '\0';
  
  return (char*)src;
}

char* ril_moveto(const char *src, const char code)
{
  int readbyte;

  for (; '\0' != *src; src += readbyte)
  {
    if (*src == code) return (char*)src;
    readbyte = mblen(src, MB_CUR_MAX);
  }
  return (char*)src;
}

char* ril_trimspace(const char *src)
{
  while (' ' == *src || '\f' == *src || '\t' == *src || '\v' == *src) ++src;

  return (char*)src;
}

char* ril_movetoeol(const char *src)
{
  while ('\0' != *src && '\n' != *src && '\r' != *src) ++src;

  return (char*)src;
}

char* ril_nextline(const char *src)
{
  src = ril_movetoeol(src);
  if ('\r' == *src) ++src;
  if ('\n' == *src) ++src;

  return (char*)src;
}

uint8_t ril_endian(void)
{
#if defined(__LITTLE_ENDIAN__)
  return RIL_LITTLE_ENDIAN;
#elif defined(__BIG_ENDIAN__)
  return RIL_BIG_ENDIAN;
#else
  int i = 1;
  return (*(char*)&i) ? RIL_LITTLE_ENDIAN : RIL_BIG_ENDIAN;
#endif
}

RILRESULT ril_error(RILVM vm, const char *s, ...)
{
  static char temp[1024];
  va_list vl;
  
  if (NULL == vm->error_hander) return RIL_ERROR;
  
  va_start(vl, s);
  vsprintf(temp, s, vl);
  va_end(vl);
  
  vm->error_hander(vm, temp);
  
  return RIL_ERROR;
}

char* ril_readfile(const char *file)
{
  FILE *fp = fopen(file, "rb");
  char *buf;
  int size;
  
  if (NULL == fp) return NULL;
  
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  rewind(fp);
  
  buf = (char*)ril_malloc(size + 1);
  fread(buf, 1, size, fp);
  buf[size] = '\0';
  fclose(fp);
  
  return buf;
}

size_t ril_writefile(const char *file, const void *src, int size)
{
  FILE *fp = fopen(file, "wb");
  size_t result;

  if (NULL == fp) return 0;

  result = fwrite(src, 1, size, fp);
  fclose(fp);

  return result;
}

const char* ril_getpath(RILVM vm, const char *file)
{
  static char path[1024];
  int length = strlen(vm->path);
  
#ifdef _WIN32
  if (NULL != strstr(file, ":\\")) return file;
#else
  if ('/' == file[0]) return file;
  if (NULL != strstr(file, ":/")) return file;
#endif
  
  memcpy(path, vm->path, length);
  strcpy(path + length, file);
  
  return path;
}

int ril_mbstrlen(const char*str)
{
  int count = 0;

  while('\0' != *str)
  {
    str += mblen(str, MB_CUR_MAX);
    count++;
  }

  return count;
}

void ril_str2lower(char *dest, const char *src)
{
  int length;

  while ('\0' != *src)
  {
    length = mblen(src, MB_CUR_MAX);
    if (1 == length) *dest = tolower(*src);
    src += length;
    dest += length;
  }
  *dest = '\0';
}

void ril_str2upper(char *dest, const char *src)
{
  int length;

  while ('\0' != *src)
  {
    length = mblen(src, MB_CUR_MAX);
    if (1 == length) *dest = toupper(*src);
    src += length;
    dest += length;
  }
  *dest = '\0';
}
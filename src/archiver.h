#ifndef _ARCHIVER_H_
#define _ARCHIVER_H_

#include "hashmap.h"
#include "buffer.h"

struct _archiver;
typedef struct _archiver archiver_t;

#ifdef __cplusplus
extern "C" {
#endif

archiver_t* archiver_open(void);
void archiver_clear(archiver_t *archiver);
void archiver_close(archiver_t *archiver);

void archiver_setinteger(archiver_t *archiver, hashmap_key_t key, int value);
void archiver_setreal(archiver_t *archiver, hashmap_key_t key, double value);
void archiver_setstring(archiver_t *archiver, hashmap_key_t key, const char *value);
void archiver_setarchiver(archiver_t *archiver, hashmap_key_t key, archiver_t *value);

int archiver_hasdata(archiver_t *archiver, hashmap_key_t key);
int archiver_getinteger(archiver_t *archiver, hashmap_key_t key);
double archiver_getreal(archiver_t *archiver, hashmap_key_t key);
const char* archiver_getstring(archiver_t *archiver, hashmap_key_t key);
archiver_t* archiver_getarchiver(archiver_t *archiver, hashmap_key_t key);
  
int archiver_read(archiver_t *archiver, const void *src);
void archiver_write(archiver_t *archiver, buffer_t *buffer);

#ifdef __cplusplus
}
#endif

#endif

#ifndef _HASHMAP_H_
#define _HASHMAP_H_

struct _hashmap;
struct _hashmap_entry;

typedef unsigned int  hashmap_key_t;
typedef struct _hashmap hashmap_t;
typedef struct _hashmap_entry hashmap_entry_t;

#ifdef __cplusplus
extern "C" {
#endif

hashmap_key_t hashmap_makekey(const char *str);
void hashmap_add(hashmap_t *hashtable, const hashmap_key_t hash, const void *rawkey, const void *data);
void* hashmap_getdata(hashmap_t *hashtable, const hashmap_key_t hash);
void* hashmap_getrawkey(hashmap_t *hashtable, const hashmap_key_t hash);
void* hashmap_delete(hashmap_t *hashtable, const hashmap_key_t hash);
void hashmap_clear(hashmap_t *hashtable);
hashmap_t* hashmap_open(void);
void hashmap_close(hashmap_t *hashmap);
unsigned int hashmap_count(hashmap_t *hashmap);
  
hashmap_entry_t* hashmap_firstentry(hashmap_t *hashmap);
hashmap_entry_t* hashmap_lastentry(hashmap_t *hashmap);
hashmap_entry_t* hashmap_getentry(hashmap_t *hashmap, hashmap_key_t hash);
hashmap_entry_t* hashmap_nextentry(hashmap_entry_t *entry);
hashmap_entry_t* hashmap_preventry(hashmap_entry_t *entry);
void* hashmap_getdatabyentry(hashmap_entry_t *entry);
hashmap_key_t hashmap_getkeybyentry(hashmap_entry_t *entry);
void* hashmap_getrawkeybyentry(hashmap_entry_t *entry);

#ifdef __cplusplus
}
#endif

#endif
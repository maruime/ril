#ifndef _CRC_H_
#define _CRC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t crc(const void *key, uint32_t len, uint32_t hash);
  
#ifdef __cplusplus
}
#endif

#endif

#ifndef HASH_HPP_
#define HASH_HPP_

#include <stdint.h>

uint32_t djb_hash(const char* data);
uint32_t murmur_hash2(const void* key, int len, uint32_t seed = 0);

#endif

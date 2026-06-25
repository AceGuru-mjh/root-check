#ifndef SELF_VERIFY_H
#define SELF_VERIFY_H

#include <stdint.h>
#include <stddef.h>

#define SELF_VERIFY_SEGMENTS 8

int  self_verify_code_crc(void);
int  self_verify_integrity(void);
int  self_verify_segment_hash(int seg_idx, uint8_t hash_out[64]);
int  self_verify_full_hash(uint8_t hash_out[64]);

#endif

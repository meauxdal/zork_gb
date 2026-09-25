#ifndef Z_STRING_DECODER_H
#define Z_STRING_DECODER_H

#include <stdint.h>

uint32_t decode_z_string(uint32_t addr, char* out_buf, int* out_idx, int max_len, int depth);

#endif
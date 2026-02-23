#ifndef Z_STRING_DECODER_H
#define Z_STRING_DECODER_H

#include <stdint.h>

void     decode_zstring(uint32_t address);
uint32_t zstring_end_addr(uint32_t address);

#endif

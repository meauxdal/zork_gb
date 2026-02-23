#ifndef Z_STRING_DECODER_H
#define Z_STRING_DECODER_H

#include <stdint.h>

/**
 * Z-String Decoder
 * Converts Z-Machine encoded text into ASCII for the VWF renderer.
 */
void decode_zstring(uint32_t address);

#endif

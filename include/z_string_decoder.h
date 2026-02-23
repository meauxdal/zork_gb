#ifndef Z_STRING_DECODER_H
#define Z_STRING_DECODER_H

#include <stdint.h>

/*
 * z_string_decoder.h
 *
 * Decodes a Z-machine encoded string starting at `address`, printing each
 * character via z_render_put_char(). Returns the address of the first byte
 * *after* the string so the caller can advance the PC correctly.
 */

uint32_t decode_zstring(uint32_t address);

#endif /* Z_STRING_DECODER_H */

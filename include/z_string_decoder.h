#ifndef Z_STRING_DECODER_H
#define Z_STRING_DECODER_H

#include <stdint.h>

// Decodes and prints a Z-string starting at the given address
void decode_zstring(uint16_t address);

// Decodes and prints the Z-string at the current PC, then updates the PC
void decode_zstring_at_pc(void);

// Calculates the length of a Z-string in memory without printing it
uint16_t get_zstring_length(uint16_t address);

#endif

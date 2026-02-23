#include <stdint.h>
#include "z_memory.h"

/**
 * External reference to the raw story file binary.
 * This symbol must match the name used in your linker/data object.
 */
extern const uint8_t zork_data[];

/**
 * Read a single byte from the story data.
 */
uint8_t z_read_byte(uint32_t address) {
    return zork_data[address];
}

/**
 * Read a 16-bit word (Big Endian) from the story data.
 */
uint16_t z_read_word(uint32_t address) {
    return ((uint16_t)zork_data[address] << 8) | zork_data[address + 1];
}

/**
 * Write a byte to the story data.
 * NOTE: On a Game Boy, this only works if the data is in WRAM.
 * If zork_data is in ROM, this will fail silently.
 */
void z_write_byte(uint32_t address, uint8_t value) {
    ((uint8_t*)zork_data)[address] = value;
}

/**
 * Write a 16-bit word (Big Endian) to the story data.
 */
void z_write_word(uint32_t address, uint16_t value) {
    ((uint8_t*)zork_data)[address] = (uint8_t)(value >> 8);
    ((uint8_t*)zork_data)[address + 1] = (uint8_t)(value & 0xFF);
}

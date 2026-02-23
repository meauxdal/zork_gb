/**
 * src/z_memory.c
 * -------------------------------------------------------------------------
 * Banked memory accessor for Zork data using bin2banks.py output.
 */

#include <stdint.h>
#include <gb/gb.h>
#include "z_memory.h"
#include "zork_data/zork_data.h"

static const unsigned char* const banks[ZORK_DATA_NUM_BANKS] = {
    zork_bank1_data,
    zork_bank2_data,
    zork_bank3_data,
    zork_bank4_data,
    zork_bank5_data,
    zork_bank6_data,
    zork_bank7_data,
    zork_bank8_data,
    zork_bank9_data,
    zork_bank10_data
};

uint8_t z_read_byte(uint32_t address) {
    uint8_t bank_index = (address >> 14); // 16 KB per bank
    if (bank_index >= ZORK_DATA_NUM_BANKS) return 0;

    uint16_t offset = address & 0x3FFF; // offset inside bank
    SWITCH_ROM(bank_index + ZORK_DATA_BANK_START);
    uint8_t value = banks[bank_index][offset];
    SWITCH_ROM(1); // restore default bank
    return value;
}

uint16_t z_read_word(uint32_t address) {
    return ((uint16_t)z_read_byte(address) << 8) | z_read_byte(address + 1);
}

void z_write_byte(uint32_t address, uint8_t value) {
    // ROM data is immutable; implement save RAM mapping here if needed.
    (void)address; (void)value;
}

void z_write_word(uint32_t address, uint16_t value) {
    (void)address; (void)value;
}

void z_init_memory(void) {
    // Any initialization logic for banked memory can go here
}

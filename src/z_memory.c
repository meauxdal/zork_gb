/*
 * z_memory.c
 *
 * Two-region writable memory:
 *
 *   z_wram[0x1000]      : shadow of z-file 0x0000..0x0FFF
 *                         covers header + entire object table
 *
 *   z_globals[240*2]    : shadow of z-file 0x2008..0x21E7
 *                         covers all 240 global variables
 *
 * Everything else is read-only from banked ROM.
 *
 * Read priority:
 *   1. If address is in 0x0000..0x0FFF  -> z_wram
 *   2. If address is in globals range   -> z_globals
 *   3. Otherwise                        -> banked ROM
 */

#include <gb/gb.h>
#include <stdint.h>
#include "z_memory.h"
#include "zork_data.h"

static uint8_t z_wram[Z_DYNAMIC_SIZE];           /* 4096 bytes */
static uint8_t z_globals[Z_GLOBALS_COUNT * 2u];  /* 480 bytes  */

/* The globals base address is read from the z-file header at 0x0C.
 * We cache it after init so we don't re-read the header constantly. */
static uint16_t z_globals_base = 0u;

static uint8_t rom_read_byte(uint32_t address) {
    uint8_t  bank_index = (uint8_t)(address >> 14u);
    uint16_t offset     = (uint16_t)(address & 0x3FFFu);
    uint8_t  val        = 0u;

    if (bank_index >= ZORK_DATA_NUM_BANKS) return 0u;

    SWITCH_ROM((uint8_t)(ZORK_DATA_BANK_START + bank_index));
    switch (bank_index) {
        case 0: val = zork_bank2_data[offset]; break;
        case 1: val = zork_bank3_data[offset]; break;
        case 2: val = zork_bank4_data[offset]; break;
        case 3: val = zork_bank5_data[offset]; break;
        case 4: val = zork_bank6_data[offset]; break;
        case 5: val = zork_bank7_data[offset]; break;
        default: break;
    }
    SWITCH_ROM(1u);
    return val;
}

void z_mem_init(void) {
    uint16_t i;

    /* Copy header + object table region */
    for (i = 0u; i < Z_DYNAMIC_SIZE; i++) {
        z_wram[i] = rom_read_byte((uint32_t)i);
    }

    /* Read the actual globals table base from the header (offset 0x0C) */
    z_globals_base = (uint16_t)((uint16_t)z_wram[0x0Cu] << 8)
                   | (uint16_t)z_wram[0x0Du];

    /* Copy globals */
    for (i = 0u; i < Z_GLOBALS_COUNT * 2u; i++) {
        z_globals[i] = rom_read_byte((uint32_t)z_globals_base + i);
    }
}

uint8_t z_read_byte(uint32_t address) {
    if (address < (uint32_t)Z_DYNAMIC_SIZE) {
        return z_wram[(uint16_t)address];
    }
    if (z_globals_base != 0u &&
        address >= (uint32_t)z_globals_base &&
        address <  (uint32_t)z_globals_base + (uint32_t)(Z_GLOBALS_COUNT * 2u)) {
        return z_globals[(uint16_t)(address - z_globals_base)];
    }
    return rom_read_byte(address);
}

uint16_t z_read_word(uint32_t address) {
    return ((uint16_t)z_read_byte(address) << 8)
         |  (uint16_t)z_read_byte(address + 1u);
}

void z_write_byte(uint32_t address, uint8_t value) {
    if (address < (uint32_t)Z_DYNAMIC_SIZE) {
        z_wram[(uint16_t)address] = value;
        return;
    }
    if (z_globals_base != 0u &&
        address >= (uint32_t)z_globals_base &&
        address <  (uint32_t)z_globals_base + (uint32_t)(Z_GLOBALS_COUNT * 2u)) {
        z_globals[(uint16_t)(address - z_globals_base)] = value;
    }
    /* Writes to other static addresses are silently ignored */
}

void z_write_word(uint32_t address, uint16_t value) {
    z_write_byte(address,      (uint8_t)(value >> 8));
    z_write_byte(address + 1u, (uint8_t)(value & 0xFFu));
}

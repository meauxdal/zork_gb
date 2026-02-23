/*
 * z_memory.c
 *
 * Banked ROM access + WRAM shadow for the Z-machine dynamic segment.
 *
 * ROM layout (produced by bin2banks.py):
 *   Bank 1 = z-file bytes 0x0000 .. 0x3FFF
 *   Bank 2 = z-file bytes 0x4000 .. 0x7FFF
 *   ... etc.
 * Bank 1 is always mapped at 0x4000-0x7FFF by GBDK's startup code, so
 * SWITCH_ROM(1) is "home". We bump the bank number for each 16 KB window.
 *
 * Dynamic segment shadow:
 *   z_wram[0 .. Z_DYNAMIC_SIZE-1] mirrors z-file[0 .. Z_DYNAMIC_SIZE-1].
 *   Copied from ROM at z_mem_init(). All writes go here. Reads of dynamic
 *   addresses come from here. Reads above Z_DYNAMIC_SIZE go to ROM.
 */

#include <gb/gb.h>
#include <stdint.h>
#include <string.h>
#include "z_memory.h"
#include "zork_data.h"

/* -----------------------------------------------------------------------
 * WRAM shadow for the dynamic (writable) segment
 * ----------------------------------------------------------------------- */
static uint8_t z_wram[Z_DYNAMIC_SIZE];

/* -----------------------------------------------------------------------
 * Banked ROM pointer table — populated by bin2banks.py / zork_data.h
 * ----------------------------------------------------------------------- */
static const uint8_t * const z_banks[ZORK_DATA_NUM_BANKS] = {
    zork_bank1_data,
    zork_bank2_data,
    zork_bank3_data,
    zork_bank4_data,
    zork_bank5_data,
    zork_bank6_data,
#if ZORK_DATA_NUM_BANKS > 6
    zork_bank7_data,
    zork_bank8_data,
    zork_bank9_data,
    zork_bank10_data,
#endif
};

/* -----------------------------------------------------------------------
 * Raw ROM read — no WRAM check, used only during init copy
 * ----------------------------------------------------------------------- */
static uint8_t rom_read_byte(uint32_t address) {
    uint8_t bank_index = (uint8_t)(address >> 14);
    if (bank_index >= ZORK_DATA_NUM_BANKS) return 0;
    uint16_t offset = (uint16_t)(address & 0x3FFFu);
    SWITCH_ROM(bank_index + ZORK_DATA_BANK_START);
    uint8_t val = z_banks[bank_index][offset];
    SWITCH_ROM(ZORK_DATA_BANK_START); /* restore bank 1 as default */
    return val;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

void z_mem_init(void) {
    /* Copy the dynamic segment out of ROM into WRAM so writes work. */
    uint16_t i;
    for (i = 0; i < Z_DYNAMIC_SIZE; i++) {
        z_wram[i] = rom_read_byte((uint32_t)i);
    }
}

uint8_t z_read_byte(uint32_t address) {
    if (address < Z_DYNAMIC_SIZE) {
        return z_wram[(uint16_t)address];
    }
    return rom_read_byte(address);
}

uint16_t z_read_word(uint32_t address) {
    return ((uint16_t)z_read_byte(address) << 8) | z_read_byte(address + 1u);
}

void z_write_byte(uint32_t address, uint8_t value) {
    if (address < Z_DYNAMIC_SIZE) {
        z_wram[(uint16_t)address] = value;
    }
    /* Writes above the shadow are silently ignored — ROM is immutable. */
}

void z_write_word(uint32_t address, uint16_t value) {
    z_write_byte(address,     (uint8_t)(value >> 8));
    z_write_byte(address + 1u, (uint8_t)(value & 0xFFu));
}

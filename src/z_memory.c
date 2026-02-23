/*
 * z_memory.c
 *
 * Banked ROM access + WRAM shadow for the Z-machine dynamic segment.
 *
 * For the init copy and all ROM reads, we use the zork_bankN_data arrays
 * directly. GBDK's linker places them in the correct banks via #pragma bank,
 * and SWITCH_ROM maps that bank into 0x4000-0x7FFF before we dereference.
 * The arrays are declared extern so the compiler knows they exist, and
 * SWITCH_ROM ensures the right bank is mapped when we index into them.
 *
 * This avoids the pointer-table-after-bank-switch hazard: we never store
 * a pointer to banked data; we always switch first, then index.
 *
 * Z-file bank layout (--start-bank 2):
 *   Bank 2 = z-file bytes 0x0000..0x3FFF   (bank_index 0)
 *   Bank 3 = z-file bytes 0x4000..0x7FFF   (bank_index 1)
 *   Bank 4 = z-file bytes 0x8000..0xBFFF   (bank_index 2)
 *   Bank 5 = z-file bytes 0xC000..0xFFFF   (bank_index 3)
 *   Bank 6 = z-file bytes 0x10000..0x13FFF (bank_index 4)
 *   Bank 7 = z-file bytes 0x14000..0x17FFF (bank_index 5)
 */

#include <gb/gb.h>
#include <stdint.h>
#include "z_memory.h"
#include "zork_data.h"

/* The dynamic segment shadow lives in WRAM */
static uint8_t z_wram[Z_DYNAMIC_SIZE];

/*
 * Read from banked ROM. We switch the bank, read, switch back.
 * Using a switch statement so the compiler sees concrete array accesses
 * rather than a pointer dereference after a bank switch — lcc handles
 * this correctly because each case is a direct array index with the
 * bank already mapped.
 */
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
    for (i = 0u; i < Z_DYNAMIC_SIZE; i++) {
        z_wram[i] = rom_read_byte((uint32_t)i);
    }
}

uint8_t z_read_byte(uint32_t address) {
    if (address < (uint32_t)Z_DYNAMIC_SIZE) {
        return z_wram[(uint16_t)address];
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
    }
}

void z_write_word(uint32_t address, uint16_t value) {
    z_write_byte(address,      (uint8_t)(value >> 8));
    z_write_byte(address + 1u, (uint8_t)(value & 0xFFu));
}

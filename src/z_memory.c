/*
 * z_memory.c
 *
 * Banked ROM access + WRAM shadow for the Z-machine dynamic segment.
 *
 * The key insight for Game Boy banked ROM:
 *   SWITCH_ROM(n) maps bank n into the hardware window at 0x4000-0x7FFF.
 *   We can then read directly from that window using a volatile pointer.
 *   We must NOT go through a pointer table that lives in a different bank,
 *   because switching banks unmaps the table itself.
 *
 * Z-file layout in ROM banks (--start-bank 2):
 *   Bank 2: z-file bytes 0x0000..0x3FFF
 *   Bank 3: z-file bytes 0x4000..0x7FFF
 *   ... etc.
 *
 * WRAM shadow:
 *   The first Z_DYNAMIC_SIZE bytes of the z-file are copied into z_wram[]
 *   at boot. All writes go here. Reads of dynamic addresses come from here.
 *   Reads of static addresses (>= Z_DYNAMIC_SIZE) go to banked ROM.
 */

#include <gb/gb.h>
#include <stdint.h>
#include "z_memory.h"
#include "zork_data.h"

/* WRAM shadow for the writable dynamic segment */
static uint8_t z_wram[Z_DYNAMIC_SIZE];

/*
 * The Game Boy banked ROM window is always at 0x4000-0x7FFF.
 * After SWITCH_ROM(n), bank n's bytes are readable at that address.
 * We read through a volatile pointer so the compiler can't cache or
 * reorder the access across the bank switch.
 */
#define BANKED_WIN ((volatile uint8_t *)0x4000u)

static uint8_t rom_read_byte(uint32_t address) {
    uint8_t  bank_index = (uint8_t)(address >> 14u);
    uint16_t offset     = (uint16_t)(address & 0x3FFFu);

    if (bank_index >= ZORK_DATA_NUM_BANKS) return 0u;

    SWITCH_ROM((uint8_t)(ZORK_DATA_BANK_START + bank_index));
    uint8_t val = BANKED_WIN[offset];
    SWITCH_ROM(1u); /* restore: our code lives in bank 1 */
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

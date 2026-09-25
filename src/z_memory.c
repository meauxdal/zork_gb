#include "z_memory.h"
#include "zork_data.h"
#include <stdint.h>
#include <stdbool.h>

static uint8_t current_rom_bank = 1;

/**
 * Sets the active ROM bank and updates tracking state.
 */
void z_set_bank(uint8_t bank) {
    current_rom_bank = bank;
    *(volatile uint8_t*)0x2000 = bank;
}

/**
 * Reads a single byte from Z-machine memory given a 32-bit absolute address.
 * Preserves active MBC ROM bank context to prevent execution code swaps.
 */
uint8_t z_read_byte(uint32_t addr) {
    if (addr < 0x4000) {
        // Fixed Bank 0 (0x0000 - 0x3FFF)
        return zork_rom[addr];
    } else {
        // High Banks: Bank-swapped region (0x4000 - 0x7FFF)
        uint8_t target_bank = (uint8_t)(addr >> 14);
        uint16_t offset = (uint16_t)((addr & 0x3FFF) + 0x4000);

        uint8_t prev_bank = current_rom_bank;
        if (target_bank != prev_bank) {
            *(volatile uint8_t*)0x2000 = target_bank;
        }

        uint8_t val = *(volatile uint8_t*)offset;

        // Restore prior bank context after byte fetch
        if (target_bank != prev_bank) {
            *(volatile uint8_t*)0x2000 = prev_bank;
        }

        return val;
    }
}

/**
 * Reads a 16-bit big-endian word from Z-machine memory.
 */
uint16_t z_read_word(uint32_t addr) {
    uint8_t hi = z_read_byte(addr);
    uint8_t lo = z_read_byte(addr + 1);
    return (uint16_t)((hi << 8) | lo);
}
#include "z_memory.h"
#include <gb/gb.h>
#include <string.h>

/**
 * Z-Machine Memory Atom
 * Total Story Size: ~85KB
 * Dynamic Memory: 0x0000 to Static Memory Address (Header 0x0E)
 * Static Memory: Follows Dynamic, stays in ROM.
 */

 // We reserve 4KB for Dynamic RAM. Most V3 games use less than this for writable data.
static uint8_t z_dynamic_ram[4096];
static uint16_t static_memory_base;

void z_init_memory(void) {
    // 1. Read the static memory base from the header (offset 0x0E)
    // Initially, we must read this from ROM Bank 1 where the data is mapped
    SWITCH_ROM_MBC1(1);

    // The header is at the very start of the story file data
    // We assume the story file starts at 0x4000 (Bank 1)
    uint8_t high = *(uint8_t*)(0x400E);
    uint8_t low = *(uint8_t*)(0x400F);
    static_memory_base = (uint16_t)(high << 8) | low;

    // 2. Copy Dynamic RAM from ROM to our internal buffer
    // This allows the engine to write to the header and global variables
    for (uint16_t i = 0; i < 4096 && i < static_memory_base; i++) {
        z_dynamic_ram[i] = *(uint8_t*)(0x4000 + i);
    }
}

uint8_t z_read_byte(uint32_t address) {
    if (address < 4096) {
        // Dynamic RAM access
        extern uint8_t z_dynamic_ram[];
        return z_dynamic_ram[address];
    }

    // MBC1 Banking logic
    uint8_t bank = (uint8_t)(address / 16384) + 1;
    uint16_t offset = (uint16_t)(address % 16384);

    SWITCH_ROM_MBC1(bank);
    return *(uint8_t*)(0x4000 + offset);
}

void z_write_byte(uint32_t address, uint8_t value) {
    if (address < 4096) {
        extern uint8_t z_dynamic_ram[];
        z_dynamic_ram[address] = value;
    }
}

uint16_t z_read_word(uint32_t address) {
    uint8_t h = z_read_byte(address);
    uint8_t l = z_read_byte(address + 1);
    return (uint16_t)((h << 8) | l);
}

void z_write_word(uint32_t address, uint16_t value) {
    z_write_byte(address, (uint8_t)(value >> 8));
    z_write_byte(address + 1, (uint8_t)(value & 0xFF));
}

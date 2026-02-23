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
    // If the address is within the Dynamic range, read from RAM
    if (address < static_memory_base && address < 4096) {
        return z_dynamic_ram[address];
    }

    // Otherwise, we must calculate which ROM bank the data lives in
    // Each Game Boy bank is 16KB (16384 bytes)
    // Story file is mapped starting at Bank 1
    uint8_t bank = (uint8_t)(address / 16384) + 1;
    uint16_t offset = (uint16_t)(address % 16384);

    SWITCH_ROM_MBC1(bank);
    return *(uint8_t*)(0x4000 + offset);
}

uint16_t z_read_word(uint32_t address) {
    uint8_t h = z_read_byte(address);
    uint8_t l = z_read_byte(address + 1);
    return (uint16_t)((h << 8) | l);
}

void z_write_byte(uint32_t address, uint8_t value) {
    // The Z-machine is only allowed to write to Dynamic Memory
    if (address < static_memory_base && address < 4096) {
        z_dynamic_ram[address] = value;
    }
    // Writes to Static/High memory are ignored per Z-spec
}

void z_write_word(uint32_t address, uint16_t value) {
    z_write_byte(address, (uint8_t)(value >> 8));
    z_write_byte(address + 1, (uint8_t)(value & 0xFF));
}

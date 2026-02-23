/*
 * memory_core.c
 * Purpose: MBC5 Bank Switching and Big-Endian Memory Access
 */

#include <gb/gb.h>
#include <stdint.h>
#include "memory_core.h"

// 4KB buffer for the Z-Machine's dynamic memory (writable)
uint8_t  dynamic_memory_buffer[DYNAMIC_MEM_SIZE];
uint32_t z_machine_pc = 0;

static uint8_t active_rom_bank = 0xFF;

/* Reads a single byte from the Z-Machine address space */
uint8_t z_read_byte(uint32_t address) {
    if (address < DYNAMIC_MEM_SIZE) {
        return dynamic_memory_buffer[address];
    }

    // V3: ROM starts at bank 1 (0x4000). Calculate target bank based on address.
    uint8_t target_bank = (uint8_t)(address >> 14) + ZORK_START_BANK;

    if (active_rom_bank != target_bank) {
        SWITCH_MBC5_BANK(target_bank);
        active_rom_bank = target_bank;
    }

    // Access ROM at the 0x4000-0x7FFF window
    return *(volatile uint8_t*)(0x4000 + (address & 0x3FFF));
}

/* Writes a single byte (only allowed within the dynamic memory buffer) */
void z_write_byte(uint32_t address, uint8_t value) {
    if (address < DYNAMIC_MEM_SIZE) {
        dynamic_memory_buffer[address] = value;
    }
}

/* Reads a 16-bit word, correcting for Z-Machine Big-Endianness */
uint16_t z_read_word(uint32_t address) {
    uint8_t hi = z_read_byte(address);
    uint8_t lo = z_read_byte(address + 1);
    return ((uint16_t)hi << 8) | lo;
}

/* Writes a 16-bit word in Big-Endian format */
void z_write_word(uint32_t address, uint16_t value) {
    z_write_byte(address, (uint8_t)(value >> 8));
    z_write_byte(address + 1, (uint8_t)(value & 0xFF));
}

/* Fetches the next byte from the Program Counter and increments it */
uint8_t z_fetch_byte(void) {
    return z_read_byte(z_machine_pc++);
}

/* Fetches the next 16-bit word from the Program Counter and increments it */
uint16_t z_fetch_word(void) {
    uint16_t val = z_read_word(z_machine_pc);
    z_machine_pc += 2;
    return val;
}

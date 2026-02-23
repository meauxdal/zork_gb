#### FILE: include/z_memory.h
#ifndef Z_MEMORY_H
#define Z_MEMORY_H

#include <gb/gb.h>
#include <stdint.h>

#define DYNAMIC_MEM_SIZE 0x1000  // 4KB
#define Z_ROM_BANK       1       // Zork .z3 sits in Bank 1

// Core Memory Functions
void z_init_memory(void);
uint8_t z_read_byte(uint16_t address);
void z_write_byte(uint16_t address, uint8_t value);
uint16_t z_read_word(uint16_t address);

// SRAM / Save Logic
void z_save_game(void);
void z_restore_game(void);

#endif

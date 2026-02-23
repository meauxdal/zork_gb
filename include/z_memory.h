#ifndef Z_MEMORY_H
#define Z_MEMORY_H

#include <gb/gb.h>
#include <stdint.h>

#ifndef SWITCH_ROM_BANK
#define SWITCH_ROM_BANK(b) SWITCH_ROM_MBC5(b)
#endif

#ifndef SWITCH_RAM_BANK
#define SWITCH_RAM_BANK(b) SWITCH_RAM_MBC5(b)
#endif

#define DYNAMIC_MEM_SIZE 0x1000

// Core Memory Functions
void z_init_memory(void);
uint8_t z_read_byte(uint16_t address);
void z_write_byte(uint16_t address, uint8_t value);
uint16_t z_read_word(uint16_t address);
uint8_t z_fetch_byte(void);
uint16_t z_fetch_word(void);

// SRAM / Save Logic
void z_save_game(void);
void z_restore_game(void);

#endif

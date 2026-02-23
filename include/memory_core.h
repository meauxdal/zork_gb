#ifndef MEMORY_CORE_H
#define MEMORY_CORE_H

#include <stdint.h>

#define DYNAMIC_MEM_SIZE 0x1000  // 4KB WRAM buffer
#define ZORK_START_BANK  1       // .z3 data begins at ROM Bank 1

extern uint32_t z_machine_pc;

uint8_t  z_read_byte(uint32_t address);
void     z_write_byte(uint32_t address, uint8_t value);
uint16_t z_read_word(uint32_t address);
void     z_write_word(uint32_t address, uint16_t value);
uint8_t  z_fetch_byte(void);
uint16_t z_fetch_word(void);

#endif

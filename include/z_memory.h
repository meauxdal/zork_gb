#ifndef Z_MEMORY_H
#define Z_MEMORY_H

#include <stdint.h>

void z_init_memory(void);

uint8_t  z_read_byte(uint32_t address);
uint16_t z_read_word(uint32_t address);
void     z_write_byte(uint32_t address, uint8_t value);
void     z_write_word(uint32_t address, uint16_t value);

#endif

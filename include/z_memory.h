#ifndef Z_MEMORY_H
#define Z_MEMORY_H

#include <stdint.h>

/*
 * Z-machine dynamic memory for Zork I:
 *   0x0000..0x0FFF -> Game Boy WRAM
 *   0x1000..0x2E52 -> cartridge SRAM bank 0
 *   0x2E53+ -> story ROM
 *
 * SRAM banks 1 and 2 hold the save state.
 */

#define Z_DYNAMIC_WRAM_SIZE 0x1000u
#define Z_DYNAMIC_SIZE      0x2E53u
#define Z_GLOBALS_BASE      0x2271u
#define Z_GLOBALS_COUNT     240u

void     z_mem_init(void);

uint8_t  z_read_byte(uint32_t address);
uint16_t z_read_word(uint32_t address);
void     z_write_byte(uint32_t address, uint8_t value);
void     z_write_word(uint32_t address, uint16_t value);

uint8_t  z_save_state(void);
uint8_t  z_restore_state(void);

#endif /* Z_MEMORY_H */

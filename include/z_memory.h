#ifndef Z_MEMORY_H
#define Z_MEMORY_H

#include <stdint.h>

/*
 * z_memory.h
 *
 * Z-Machine memory model for Game Boy.
 *
 * The Z-file is split across banked ROM (read-only). The dynamic segment
 * (addresses 0x0000 .. Z_DYNAMIC_SIZE-1) is also shadowed in WRAM so
 * writes work. Reads check the WRAM shadow first for dynamic addresses,
 * then fall through to banked ROM for everything else.
 *
 * Zork I dynamic segment ends at 0x2E53 (~11.8 KB). We shadow the first
 * Z_DYNAMIC_SIZE bytes. Writes above that limit are silently dropped —
 * acceptable for now because Zork I doesn't write above the object/globals
 * area in practice during normal play.
 */

#define Z_DYNAMIC_SIZE 0x1E00u   /* 7680 bytes — fits in 8 KB WRAM */

void     z_mem_init(void);

uint8_t  z_read_byte(uint32_t address);
uint16_t z_read_word(uint32_t address);
void     z_write_byte(uint32_t address, uint8_t value);
void     z_write_word(uint32_t address, uint16_t value);

#endif /* Z_MEMORY_H */

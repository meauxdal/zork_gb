#ifndef Z_MEMORY_H
#define Z_MEMORY_H

#include <stdint.h>

/*
 * z_memory.h
 *
 * Z_DYNAMIC_SIZE must fit in WRAM alongside the call stack, eval stack,
 * and other globals. GBDK gives us 8KB WRAM (0xC000-0xDFFF) with the
 * hardware stack growing down from 0xDFFF.
 *
 * Zork I writes to:
 *   0x0000..0x0037 : header (flags we set)
 *   0x02B4..0x0E5A : object table (~250 objects x 9 bytes)
 *   0x2008..0x21EF : globals table (240 words)
 *
 * The globals table at 0x2008 is the furthest write. But fitting 0x2200
 * bytes of shadow plus stack plus call frames exceeds 8KB. So we shadow
 * only the header + object table region, and handle globals differently:
 * globals are read/written via get_variable/set_variable which go through
 * z_read_word/z_write_word — we just need those addresses to be writable.
 *
 * Practical split that fits in 8KB WRAM:
 *   z_wram[0x1000]  = 4096 bytes  (header + object table, safely covers writes)
 *   z_stack[128b]   + call_stack[~312b] + other globals ~= 512 bytes
 *   Stack headroom  = 8192 - 4096 - 512 = 3584 bytes  ← plenty
 *
 * Globals (0x2008+) are above the shadow. z_write_byte silently drops them.
 * We handle globals by keeping a separate small globals mirror.
 */

#define Z_DYNAMIC_SIZE  0x1000u   /* 4096 bytes — header + object table */
#define Z_GLOBALS_BASE  0x2008u   /* where Zork I's global table starts */
#define Z_GLOBALS_COUNT 240u      /* 240 globals x 2 bytes = 480 bytes */

void     z_mem_init(void);

uint8_t  z_read_byte(uint32_t address);
uint16_t z_read_word(uint32_t address);
void     z_write_byte(uint32_t address, uint8_t value);
void     z_write_word(uint32_t address, uint16_t value);

#endif /* Z_MEMORY_H */

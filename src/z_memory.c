#include <stdint.h>
#include <gb/gb.h>
#include "z_memory.h"
#include "zork_data.h"

/* Current ROM bank for bank switching */
extern uint8_t _current_bank;

/* Helper: read a byte from a banked Zork ROM */
uint8_t z_read_byte(uint32_t address) {
    uint32_t bank_index = address / ZORK_BANK_SIZE;  // bank number starting at 0
    uint16_t offset = address % ZORK_BANK_SIZE;      // offset within the bank
    uint8_t value = 0;

    /* Only access banks that exist */
    switch (ZORK_DATA_BANK_START + bank_index) {
#if ZORK_DATA_NUM_BANKS >= 1
    case 1: value = zork_bank1_data[offset]; break;
#endif
#if ZORK_DATA_NUM_BANKS >= 2
    case 2: value = zork_bank2_data[offset]; break;
#endif
#if ZORK_DATA_NUM_BANKS >= 3
    case 3: value = zork_bank3_data[offset]; break;
#endif
#if ZORK_DATA_NUM_BANKS >= 4
    case 4: value = zork_bank4_data[offset]; break;
#endif
#if ZORK_DATA_NUM_BANKS >= 5
    case 5: value = zork_bank5_data[offset]; break;
#endif
#if ZORK_DATA_NUM_BANKS >= 6
    case 6: value = zork_bank6_data[offset]; break;
#endif
#if ZORK_DATA_NUM_BANKS >= 7
    case 7: value = zork_bank7_data[offset]; break;
#endif
#if ZORK_DATA_NUM_BANKS >= 8
    case 8: value = zork_bank8_data[offset]; break;
#endif
    default: value = 0; break;
    }

    return value;
}

/* Read two bytes as a big-endian word */
uint16_t z_read_word(uint32_t address) {
    return ((uint16_t)z_read_byte(address) << 8) | z_read_byte(address + 1);
}

/* Writes are currently NOP (story ROM is immutable) */
void z_write_byte(uint32_t address, uint8_t value) {
    (void)address; (void)value;
}

void z_write_word(uint32_t address, uint16_t value) {
    (void)address; (void)value;
}

/* Stub for initialization (linkage target for main.c) */
void z_init_memory(void) {}

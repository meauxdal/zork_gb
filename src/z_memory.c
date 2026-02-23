#include <stdint.h>
#include "z_memory.h"
#include "zork_data.h"

/* Read a byte from the banked Zork ROM */
uint8_t z_read_byte(uint32_t address) {
    uint32_t bank_index = address / ZORK_BANK_SIZE;
    uint32_t offset = address % ZORK_BANK_SIZE;

    if (bank_index >= ZORK_DATA_NUM_BANKS) return 0; // bounds check

    switch (ZORK_DATA_BANK_START + bank_index) {
#define BANK_CASE(N) case N: return zork_bank##N##_data[offset];
        BANK_CASE(1)
            BANK_CASE(2)
            BANK_CASE(3)
            BANK_CASE(4)
            BANK_CASE(5)
            BANK_CASE(6)
            BANK_CASE(7)
            BANK_CASE(8)
            // add more if ZORK_DATA_NUM_BANKS > 8
#undef BANK_CASE
    default: return 0;
    }
}

/* Read a word (big-endian) from banked ROM */
uint16_t z_read_word(uint32_t address) {
    uint16_t hi = z_read_byte(address);
    uint16_t lo = z_read_byte(address + 1);
    return (hi << 8) | lo;
}

/* Write stubs: story ROM is read-only */
void z_write_byte(uint32_t address, uint8_t value) { (void)address; (void)value; }
void z_write_word(uint32_t address, uint16_t value) { (void)address; (void)value; }

void z_init_memory(void) {
    /* Any RAM initialization goes here */
}

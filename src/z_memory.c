#include <stdint.h>
#include <gb/gb.h>
#include "z_memory.h"
#include "zork_data.h"

/* Helper: fetch byte from banked ROM */
uint8_t z_read_byte(uint32_t address) {
    uint8_t value;
    uint8_t save_bank = _current_bank;

    /* Determine bank number and offset */
    uint8_t bank = ZORK_DATA_BANK_START + (address / ZORK_BANK_SIZE);
    uint16_t offset = address % ZORK_BANK_SIZE;

    /* Switch to the correct bank */
    SWITCH_ROM(bank);

    /* Use pointer arithmetic on banked array */
    switch (bank) {
#define BANK_CASE(n) case n: value = zork_bank##n##_data[offset]; break;
#if ZORK_DATA_NUM_BANKS >= 1
        BANK_CASE(7)
#endif
#if ZORK_DATA_NUM_BANKS >= 2
            BANK_CASE(8)
#endif
#if ZORK_DATA_NUM_BANKS >= 3
            BANK_CASE(9)
#endif
#if ZORK_DATA_NUM_BANKS >= 4
            BANK_CASE(10)
#endif
    default: value = 0; break;
    }

    /* Restore previous bank */
    SWITCH_ROM(save_bank);
    return value;
}

uint16_t z_read_word(uint32_t address) {
    return ((uint16_t)z_read_byte(address) << 8) | z_read_byte(address + 1);
}

/* Writes are no-ops for ROM story data */
void z_write_byte(uint32_t address, uint8_t value) { (void)address; (void)value; }
void z_write_word(uint32_t address, uint16_t value) { (void)address; (void)value; }

void z_init_memory(void) {
    /* Nothing to do for ROM-only story data */
}

#include <stdint.h>
#include <gb/gb.h>
#include "z_memory.h"
#include "zork_data/zork_data.h"

uint8_t z_read_byte(uint32_t address) {
    uint8_t value;
    uint8_t save_bank = _current_bank;

    uint8_t bank = (uint8_t)(address >> 14) + ZORK_DATA_BANK_START;
    uint16_t offset = (uint16_t)(address & 0x3FFF);

    SWITCH_ROM(bank);
    switch (bank) {
    case 1: value = zork_bank1_data[offset]; break;
    case 2: value = zork_bank2_data[offset]; break;
    case 3: value = zork_bank3_data[offset]; break;
    case 4: value = zork_bank4_data[offset]; break;
    case 5: value = zork_bank5_data[offset]; break;
    case 6: value = zork_bank6_data[offset]; break;
    default: value = 0; break;
    }
    SWITCH_ROM(save_bank);
    return value;
}

uint16_t z_read_word(uint32_t address) {
    return ((uint16_t)z_read_byte(address) << 8) | z_read_byte(address + 1);
}

void z_write_byte(uint32_t address, uint8_t value) {
    (void)address; (void)value;
}

void z_write_word(uint32_t address, uint16_t value) {
    (void)address; (void)value;
}

void z_init_memory(void) {
}

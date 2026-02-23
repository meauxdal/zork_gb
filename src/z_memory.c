#include <gb/gb.h>
#include "z_memory.h"
#include <string.h>

// The 4KB buffer for the "Dynamic" part of the Z-Machine
uint8_t dynamic_ram[DYNAMIC_MEM_SIZE];

void z_init_memory(void) {
    // Copy the first 4KB from ROM Bank 1 into our working RAM
    SWITCH_ROM_BANK(Z_ROM_BANK);
    memcpy(dynamic_ram, (uint8_t *)0x4000, DYNAMIC_MEM_SIZE);
}

uint8_t z_read_byte(uint16_t address) {
    if (address < DYNAMIC_MEM_SIZE) {
        return dynamic_ram[address];
    } else {
        // High/Static memory stays in ROM Bank 1
        SWITCH_ROM_BANK(Z_ROM_BANK);
        return *(uint8_t *)(0x4000 + address);
    }
}

void z_write_byte(uint16_t address, uint8_t value) {
    if (address < DYNAMIC_MEM_SIZE) {
        dynamic_ram[address] = value;
    }
    // Static memory (ROM) is read-only; writes are ignored
}

uint16_t z_read_word(uint16_t address) {
    return (uint16_t)((z_read_byte(address) << 8) | z_read_byte(address + 1));
}

// SRAM logic for MBC5 (Using 0xA000 - 0xBFFF)
void z_save_game(void) {
    ENABLE_RAM;
    SWITCH_RAM_BANK(0);
    memcpy((uint8_t *)0xA000, dynamic_ram, DYNAMIC_MEM_SIZE);
    DISABLE_RAM;
}

void z_restore_game(void) {
    ENABLE_RAM;
    SWITCH_RAM_BANK(0);
    memcpy(dynamic_ram, (uint8_t *)0xA000, DYNAMIC_MEM_SIZE);
    DISABLE_RAM;
}

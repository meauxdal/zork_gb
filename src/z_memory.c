#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "z_memory.h"
#include "zork_data.h"

/* The 4KB buffer for the "Dynamic" (read/write) part of the Z-Machine.
   Z-Machine V3 dynamic memory is everything below the static base pointer
   stored at header offset 0x0E. In practice for Zork I it's about 0x2B4 bytes,
   but we conservatively reserve the full DYNAMIC_MEM_SIZE (0x1000 = 4KB). */
uint8_t dynamic_ram[DYNAMIC_MEM_SIZE];

/* Map a Z-machine byte address to the ROM bank number and the byte offset
   within the 0x4000-byte bank window. */
static uint8_t z_addr_to_bank(uint16_t address) {
    return (uint8_t)(ZORK_DATA_BANK_START + (address / ZORK_BANK_SIZE));
}

static uint16_t z_addr_to_offset(uint16_t address) {
    return (uint16_t)(address % ZORK_BANK_SIZE);
}

void z_init_memory(void) {
    /* Copy the first DYNAMIC_MEM_SIZE bytes from bank 1 into RAM */
    SWITCH_ROM_BANK(ZORK_DATA_BANK_START);
    memcpy(dynamic_ram, (uint8_t *)0x4000, DYNAMIC_MEM_SIZE);
}

uint8_t z_read_byte(uint16_t address) {
    if (address < DYNAMIC_MEM_SIZE) {
        return dynamic_ram[address];
    }
    /* Static/high memory: switch to the correct bank and read */
    SWITCH_ROM_BANK(z_addr_to_bank(address));
    return *(uint8_t *)(0x4000 + z_addr_to_offset(address));
}

void z_write_byte(uint16_t address, uint8_t value) {
    if (address < DYNAMIC_MEM_SIZE) {
        dynamic_ram[address] = value;
    }
    /* Writes to static/ROM are silently ignored */
}

uint16_t z_read_word(uint16_t address) {
    /* Note: Z-machine words are big-endian */
    return (uint16_t)(((uint16_t)z_read_byte(address) << 8) | z_read_byte(address + 1));
}

/* SRAM save/restore using MBC5 external RAM */
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

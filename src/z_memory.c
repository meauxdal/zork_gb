/*
 * z_memory.c
 *
 * Z-machine memory map for the Zork I V3 story:
 *
 *   0x0000..0x0FFF : dynamic memory in Game Boy WRAM
 *   0x1000..0x2E52 : dynamic memory in cartridge SRAM bank 0
 *   0x2E53+        : read-only story data in banked ROM
 *
 * SRAM banks 1 and 2 hold the save state.
 */

#include <gb/gb.h>
#include <stdint.h>
#include <string.h>
#include "z_memory.h"
#include "zork_data.h"
#include "z_variable_stack.h"
#include "z_dispatcher.h"

static uint8_t z_wram[Z_DYNAMIC_WRAM_SIZE];

#define SRAM_BASE ((uint8_t *)0xA000u)
#define SRAM_BANK_SIZE 0x2000u

#define Z_DYNAMIC_SRAM_BANK 0u
#define Z_SAVE_SRAM_FIRST   1u

static uint16_t z_static_base = Z_DYNAMIC_SIZE;
static const char SAVE_MAGIC[8] = "ZORKGB01";

/* Temporary buffer for copying between SRAM banks. */
static uint8_t sram_copy_buf[128];

static uint8_t rom_read_byte(uint32_t address) {
    uint8_t  bank_index = (uint8_t)(address >> 14u);
    uint16_t offset     = (uint16_t)(address & 0x3FFFu);
    uint8_t  val        = 0u;

    if (bank_index >= ZORK_DATA_NUM_BANKS) return 0u;

    SWITCH_ROM((uint8_t)(ZORK_DATA_BANK_START + bank_index));
    switch (bank_index) {
        case 0: val = zork_bank2_data[offset]; break;
        case 1: val = zork_bank3_data[offset]; break;
        case 2: val = zork_bank4_data[offset]; break;
        case 3: val = zork_bank5_data[offset]; break;
        case 4: val = zork_bank6_data[offset]; break;
        case 5: val = zork_bank7_data[offset]; break;
        default: break;
    }
    SWITCH_ROM(1u);
    return val;
}

static void sram_save_write(uint16_t offset, const uint8_t *src, uint16_t len) {
    while (len != 0u) {
        uint8_t bank = (uint8_t)(Z_SAVE_SRAM_FIRST + (offset / SRAM_BANK_SIZE));
        uint16_t bank_offset = offset & (SRAM_BANK_SIZE - 1u);
        uint16_t chunk = (uint16_t)(SRAM_BANK_SIZE - bank_offset);
        if (chunk > len) chunk = len;

        SWITCH_RAM(bank);
        memcpy(SRAM_BASE + bank_offset, src, chunk);

        offset = (uint16_t)(offset + chunk);
        src += chunk;
        len = (uint16_t)(len - chunk);
    }
}

static void sram_save_read(uint16_t offset, uint8_t *dst, uint16_t len) {
    while (len != 0u) {
        uint8_t bank = (uint8_t)(Z_SAVE_SRAM_FIRST + (offset / SRAM_BANK_SIZE));
        uint16_t bank_offset = offset & (SRAM_BANK_SIZE - 1u);
        uint16_t chunk = (uint16_t)(SRAM_BANK_SIZE - bank_offset);
        if (chunk > len) chunk = len;

        SWITCH_RAM(bank);
        memcpy(dst, SRAM_BASE + bank_offset, chunk);

        offset = (uint16_t)(offset + chunk);
        dst += chunk;
        len = (uint16_t)(len - chunk);
    }
}

static void save_dynamic_sram(uint16_t *offset) {
    uint16_t src_offset = Z_DYNAMIC_WRAM_SIZE;
    uint16_t remaining = (uint16_t)(z_static_base - Z_DYNAMIC_WRAM_SIZE);

    while (remaining != 0u) {
        uint16_t chunk = remaining;
        if (chunk > sizeof(sram_copy_buf))
            chunk = sizeof(sram_copy_buf);

        SWITCH_RAM(Z_DYNAMIC_SRAM_BANK);
        memcpy(sram_copy_buf, SRAM_BASE + (src_offset - Z_DYNAMIC_WRAM_SIZE), chunk);
        sram_save_write(*offset, sram_copy_buf, chunk);

        *offset = (uint16_t)(*offset + chunk);
        src_offset = (uint16_t)(src_offset + chunk);
        remaining = (uint16_t)(remaining - chunk);
    }
}

static void restore_dynamic_sram(uint16_t *offset) {
    uint16_t dst_offset = Z_DYNAMIC_WRAM_SIZE;
    uint16_t remaining = (uint16_t)(z_static_base - Z_DYNAMIC_WRAM_SIZE);

    while (remaining != 0u) {
        uint16_t chunk = remaining;
        if (chunk > sizeof(sram_copy_buf))
            chunk = sizeof(sram_copy_buf);

        sram_save_read(*offset, sram_copy_buf, chunk);

        SWITCH_RAM(Z_DYNAMIC_SRAM_BANK);
        memcpy(SRAM_BASE + (dst_offset - Z_DYNAMIC_WRAM_SIZE), sram_copy_buf, chunk);

        *offset = (uint16_t)(*offset + chunk);
        dst_offset = (uint16_t)(dst_offset + chunk);
        remaining = (uint16_t)(remaining - chunk);
    }
}

void z_mem_init(void) {
    uint16_t i;

    /* Header + object table live in WRAM. */
    for (i = 0u; i < Z_DYNAMIC_WRAM_SIZE; i++) {
        z_wram[i] = rom_read_byte((uint32_t)i);
    }

    z_static_base = z_read_word(0x0Eu);
    if (z_static_base < Z_DYNAMIC_WRAM_SIZE || z_static_base > Z_DYNAMIC_SIZE)
        z_static_base = Z_DYNAMIC_SIZE;

    /* Remaining dynamic memory lives in SRAM bank 0. */
    ENABLE_RAM;
    SWITCH_RAM(Z_DYNAMIC_SRAM_BANK);

    for (i = Z_DYNAMIC_WRAM_SIZE; i < z_static_base; i++) {
        SRAM_BASE[i - Z_DYNAMIC_WRAM_SIZE] = rom_read_byte((uint32_t)i);
    }
}

uint8_t z_read_byte(uint32_t address) {
    if (address < (uint32_t)Z_DYNAMIC_WRAM_SIZE) {
        return z_wram[(uint16_t)address];
    }

    if (address < (uint32_t)z_static_base) {
        return SRAM_BASE[(uint16_t)(address - Z_DYNAMIC_WRAM_SIZE)];
    }

    return rom_read_byte(address);
}

uint16_t z_read_word(uint32_t address) {
    return ((uint16_t)z_read_byte(address) << 8)
         |  (uint16_t)z_read_byte(address + 1u);
}

void z_write_byte(uint32_t address, uint8_t value) {
    if (address < (uint32_t)Z_DYNAMIC_WRAM_SIZE) {
        z_wram[(uint16_t)address] = value;
        return;
    }

    if (address < (uint32_t)z_static_base) {
        SRAM_BASE[(uint16_t)(address - Z_DYNAMIC_WRAM_SIZE)] = value;
    }
}

void z_write_word(uint32_t address, uint16_t value) {
    z_write_byte(address,      (uint8_t)(value >> 8));
    z_write_byte(address + 1u, (uint8_t)(value & 0xFFu));
}

uint8_t z_save_state(void) {
    uint16_t offset = 0u;

    ENABLE_RAM;

    sram_save_write(offset, (const uint8_t *)SAVE_MAGIC, 8u);
    offset = (uint16_t)(offset + 8u);

    sram_save_write(offset, (const uint8_t *)&z_machine_pc, sizeof(z_machine_pc));
    offset = (uint16_t)(offset + sizeof(z_machine_pc));

    sram_save_write(offset, z_wram, Z_DYNAMIC_WRAM_SIZE);
    offset = (uint16_t)(offset + Z_DYNAMIC_WRAM_SIZE);

    save_dynamic_sram(&offset);

    sram_save_write(offset, &sp, sizeof(sp));
    offset = (uint16_t)(offset + sizeof(sp));

    sram_save_write(offset, (const uint8_t *)z_stack, sizeof(z_stack));
    offset = (uint16_t)(offset + sizeof(z_stack));

    sram_save_write(offset, (const uint8_t *)&fp, sizeof(fp));
    offset = (uint16_t)(offset + sizeof(fp));

    sram_save_write(offset, (const uint8_t *)call_stack, sizeof(call_stack));

    SWITCH_RAM(Z_DYNAMIC_SRAM_BANK);
    return 1u;
}

uint8_t z_restore_state(void) {
    uint16_t offset = 0u;
    char magic[8];

    ENABLE_RAM;

    sram_save_read(offset, (uint8_t *)magic, sizeof(magic));
    if (memcmp(magic, SAVE_MAGIC, sizeof(magic)) != 0) {
        SWITCH_RAM(Z_DYNAMIC_SRAM_BANK);
        return 0u;
    }
    offset = (uint16_t)(offset + sizeof(magic));

    sram_save_read(offset, (uint8_t *)&z_machine_pc, sizeof(z_machine_pc));
    offset = (uint16_t)(offset + sizeof(z_machine_pc));

    sram_save_read(offset, z_wram, Z_DYNAMIC_WRAM_SIZE);
    offset = (uint16_t)(offset + Z_DYNAMIC_WRAM_SIZE);

    restore_dynamic_sram(&offset);

    sram_save_read(offset, &sp, sizeof(sp));
    offset = (uint16_t)(offset + sizeof(sp));

    sram_save_read(offset, (uint8_t *)z_stack, sizeof(z_stack));
    offset = (uint16_t)(offset + sizeof(z_stack));

    sram_save_read(offset, (uint8_t *)&fp, sizeof(fp));
    offset = (uint16_t)(offset + sizeof(fp));

    sram_save_read(offset, (uint8_t *)call_stack, sizeof(call_stack));

    SWITCH_RAM(Z_DYNAMIC_SRAM_BANK);
    return 1u;
}

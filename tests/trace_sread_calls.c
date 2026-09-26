#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MOCK_GB_H

#define J_A      0x01
#define J_B      0x02
#define J_SELECT 0x04
#define J_START  0x08
#define J_RIGHT  0x10
#define J_LEFT   0x20
#define J_UP     0x40
#define J_DOWN   0x80

uint8_t mock_vram[18][20];
void set_bkg_tile_xy(uint8_t x, uint8_t y, uint8_t tile) {}
void wait_vbl_done(void) {}

static uint8_t mock_sram_banks[4][8192];
static uint8_t mock_ram_bank = 0;
static uint8_t mock_rom_bank = 1;
static uint8_t ram_enabled = 0;

uint8_t zork_b2[16384];
uint8_t zork_b3[16384];
uint8_t zork_b4[16384];
uint8_t zork_b5[16384];
uint8_t zork_b6[16384];
uint8_t zork_b7[16384];

#define zork_bank2_data zork_b2
#define zork_bank3_data zork_b3
#define zork_bank4_data zork_b4
#define zork_bank5_data zork_b5
#define zork_bank6_data zork_b6
#define zork_bank7_data zork_b7

#define ZORK_DATA_H
#define ZORK_DATA_BANK_START 2
#define ZORK_DATA_NUM_BANKS 6

#define ENABLE_RAM (ram_enabled = 1)
#define DISABLE_RAM (ram_enabled = 0)
#define SWITCH_RAM(b) (mock_ram_bank = (b))
#define SWITCH_ROM(b) (mock_rom_bank = (b))

#define SRAM_BASE (mock_sram_banks[mock_ram_bank])

#include "z_memory.h"
#include "../src/z_memory.c"
#include "../src/z_variable_stack.c"

void z_render_put_char(char c) { putchar(c); }
void z_render_reset_line_count(void) {}
void z_render_show_candidate(char c) {}
void z_render_clear_candidate(void) {}
void z_render_status_begin(void) {}
void z_render_status_end(void) {}

#include "../src/z_status_bar.c"
#include "../src/z_object_engine.c"
#include "../src/z_string_decoder.c"

uint8_t joypad(void) { return 0; }

static int call_no = 0;

char workboy_get_char(void) {
    static int polls = 0;
    polls++;
    if (polls < 3) return 0;
    polls = 0;
    return '\n'; // submit empty line instantly
}

#include "../src/z_dispatcher.c"

int main(void) {
    FILE *f = fopen("data/zork1.z3", "rb");
    uint8_t full_story[100000];
    size_t len = fread(full_story, 1, sizeof(full_story), f);
    fclose(f);

    memcpy(zork_b2, full_story + 0 * 16384, 16384);
    memcpy(zork_b3, full_story + 1 * 16384, 16384);
    memcpy(zork_b4, full_story + 2 * 16384, 16384);
    memcpy(zork_b5, full_story + 3 * 16384, 16384);
    memcpy(zork_b6, full_story + 4 * 16384, 16384);
    if (len > 5 * 16384)
        memcpy(zork_b7, full_story + 5 * 16384, len - 5 * 16384);

    z_mem_init();
    z_stack_init();
    z_dispatcher_init();

    uint8_t flags = z_read_byte(0x01u);
    flags &= ~0x10u; flags &= ~0x20u; flags &= ~0x40u;
    z_write_byte(0x01u, flags);
    z_write_byte(0x20u, 18u); z_write_byte(0x21u, 20u);

    for (int i = 0; i < 50000; i++) {
        uint32_t pc = z_machine_pc;
        if (z_read_byte(pc) == 0xE4) { // SREAD opcode
            call_no++;
            printf("\n[SREAD Call #%d]\n", call_no);
            if (call_no >= 5) break;
        }
        execute_next_instruction();
    }
    return 0;
}

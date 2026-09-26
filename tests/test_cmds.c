#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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
uint8_t joypad(void) { return 0; }

static uint8_t mock_sb_val = 0xFF;
static uint8_t mock_sc_reg = 0;
#define SB_REG mock_sb_val
#define SC_REG mock_sc_reg

static uint8_t story_data[100000];
static size_t story_len = 0;

uint8_t z_read_byte(uint32_t address) {
    if (address < story_len) return story_data[address];
    return 0;
}
uint16_t z_read_word(uint32_t address) {
    return ((uint16_t)z_read_byte(address) << 8) | z_read_byte(address + 1);
}
void z_write_byte(uint32_t address, uint8_t value) {
    if (address < story_len) story_data[address] = value;
}
void z_write_word(uint32_t address, uint16_t value) {
    z_write_byte(address, (uint8_t)(value >> 8));
    z_write_byte(address + 1, (uint8_t)(value & 0xFF));
}

#include "z_memory.h"
#include "z_variable_stack.h"
#include "z_dispatcher.h"
#include "z_render.h"
#include "z_status_bar.h"
#include "z_object_engine.h"

uint8_t z_save_state(void) { return 1; }
uint8_t z_restore_state(void) { return 1; }

#include "../src/z_variable_stack.c"

void z_render_put_char(char c) {
    putchar(c);
}
void z_render_reset_line_count(void) {}
void z_render_show_candidate(char c) {}
void z_render_clear_candidate(void) {}
void z_render_status_begin(void) {}
void z_render_status_end(void) {}

#include "../src/z_status_bar.c"
#include "../src/z_object_engine.c"
#include "../src/z_string_decoder.c"
#include "../src/workboy.c"

/* Instead of redefining op_sread, we hack op_sread in z_dispatcher.c by modifying z_dispatcher.c or defining a hook */

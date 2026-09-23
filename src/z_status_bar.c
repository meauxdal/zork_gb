/*
 * z_status_bar.c
 *
 * V3 status bar: "location name" on left, "Score: N  Turns: N" on right.
 *
 * V3 spec §8.2: global variable 1 (0x11) = current room object,
 * global 2 (0x12) = score, global 3 (0x13) = turns.
 */

#include <gb/gb.h>
#include <stdint.h>
#include "z_status_bar.h"
#include "z_render.h"
#include "z_object_engine.h"
#include "z_variable_stack.h"
#include "z_memory.h"

/* Clear status row */
static void clear_status(void) {
    uint8_t x;
    for (x = 0; x < 20u; x++) set_bkg_tile_xy(x, 0u, 0x20u);
}

static void print_num_to_buf(int16_t val, char *buf, uint8_t *idx) {
    if (val < 0) {
        buf[(*idx)++] = '-';
        val = -val;
    }
    char tmp[6];
    uint8_t tlen = 0;
    do {
        tmp[tlen++] = '0' + (char)(val % 10);
        val /= 10;
    } while (val > 0);
    while (tlen > 0) {
        buf[(*idx)++] = tmp[--tlen];
    }
}

void update_status_bar(void) {
    clear_status();

    z_render_status_begin(); /* redirect z_render_put_char to row 0 */

    /* Format score and turns string: " S:score T:turns" */
    int16_t score = (int16_t)get_variable(0x12u);
    int16_t turns = (int16_t)get_variable(0x13u);

    char right_buf[12];
    uint8_t rlen = 0;
    right_buf[rlen++] = ' ';
    right_buf[rlen++] = 'S';
    right_buf[rlen++] = ':';
    print_num_to_buf(score, right_buf, &rlen);
    right_buf[rlen++] = ' ';
    right_buf[rlen++] = 'T';
    right_buf[rlen++] = ':';
    print_num_to_buf(turns, right_buf, &rlen);
    right_buf[rlen] = '\0';

    uint8_t right_start = (rlen < 20u) ? (20u - rlen) : 0u;

    /* Location name = object name of global 1 */
    uint8_t location = (uint8_t)get_variable(0x11u);
    if (location != 0u) {
        get_object_name(location);
    }

    /* Print right side (Score/Turns) aligned to right, overwriting tail if needed */
    uint8_t i;
    for (i = 0u; i < rlen; i++) {
        set_bkg_tile_xy(right_start + i, 0u, (uint8_t)right_buf[i]);
    }

    z_render_status_end();
}

/*
 * z_render.c
 *
 * Fixed-width 8x8 tile renderer.
 * Tile index == ASCII value; font must be pre-loaded into VRAM tile block 0.
 *
 * Layout:
 *   Row 0          : status bar (written by z_status_bar.c via cursor API)
 *   Rows 1 .. 17   : scrolling text area
 *
 * Scrolling: when the cursor advances past row 17, we shift all text rows
 * up by one and clear the bottom row. Simple and correct for a text game.
 */

#include <gb/gb.h>
#include <stdint.h>
#include "z_render.h"

#define COLS        20u
#define TEXT_TOP     1u
#define TEXT_BOTTOM 17u
#define TEXT_ROWS   (TEXT_BOTTOM - TEXT_TOP + 1u)  /* 17 */

static uint8_t cur_x;
static uint8_t cur_y;

/* Saved cursor for status bar rendering */
static uint8_t saved_x;
static uint8_t saved_y;

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */
static void clear_row(uint8_t row) {
    uint8_t x;
    for (x = 0; x < COLS; x++) {
        set_bkg_tile_xy(x, row, 0x20u); /* 0x20 = space tile */
    }
}

static void scroll_up(void) {
    uint8_t row, x;
    /* Copy each row's tiles up one row */
    for (row = TEXT_TOP; row < TEXT_BOTTOM; row++) {
        for (x = 0; x < COLS; x++) {
            /*
             * GBDK doesn't provide get_bkg_tile_xy, so we'd need our own
             * tile shadow to do this properly. For now: clear-on-wrap is
             * simpler and good enough for MVP. Real scroll comes later.
             */
            (void)row; (void)x;
        }
    }
    /* MVP: just clear bottom row and stay there rather than true scroll */
    cur_y = TEXT_BOTTOM;
    clear_row(cur_y);
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

void z_render_init(void) {
    uint8_t row;
    cur_x = 0;
    cur_y = TEXT_TOP;
    for (row = 0; row < 18u; row++) {
        clear_row(row);
    }
}

void z_render_put_char(char c) {
    if (c == '\n' || c == '\r') {
        cur_x = 0;
        cur_y++;
        if (cur_y > TEXT_BOTTOM) scroll_up();
        return;
    }

    if (c == '\b') {
        if (cur_x > 0) {
            cur_x--;
            set_bkg_tile_xy(cur_x, cur_y, 0x20u);
        }
        return;
    }

    /* Auto-wrap: status bar (y=0) never wraps */
    if (cur_x >= COLS) {
        if (cur_y == 0) return;
        cur_x = 0;
        cur_y++;
        if (cur_y > TEXT_BOTTOM) scroll_up();
    }

    set_bkg_tile_xy(cur_x, cur_y, (uint8_t)c);
    cur_x++;
}

void z_render_status_begin(void) {
    saved_x = cur_x;
    saved_y = cur_y;
    cur_x = 0;
    cur_y = 0;
}

void z_render_status_end(void) {
    cur_x = saved_x;
    cur_y = saved_y;
}

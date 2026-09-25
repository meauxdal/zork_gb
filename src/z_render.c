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
#include "workboy.h"

#define COLS        20u
#define TEXT_TOP     1u
#define TEXT_BOTTOM 17u
#define TEXT_ROWS   (TEXT_BOTTOM - TEXT_TOP + 1u)  /* 17 */

static uint8_t cur_x;
static uint8_t cur_y;

/* Screen tile buffer for rows 1..17 */
static uint8_t text_screen[TEXT_ROWS][COLS];

/* Lines printed since last prompt / page pause */
static uint8_t lines_printed;

/* Saved cursor for status bar rendering */
static uint8_t saved_x;
static uint8_t saved_y;

static void show_more_prompt_and_wait(void) {
    uint8_t saved_tiles[6];
    uint8_t x;
    const char *more_str = "[MORE]";

    /* Save 6 tiles at bottom right of screen (cols 14..19, row TEXT_BOTTOM) */
    for (x = 0; x < 6u; x++) {
        saved_tiles[x] = text_screen[TEXT_ROWS - 1u][14u + x];
        set_bkg_tile_xy(14u + x, TEXT_BOTTOM, (uint8_t)more_str[x]);
    }

    /* Wait for any currently held keys to be released first */
    while (joypad() != 0 || workboy_get_char() != 0) {
        wait_vbl_done();
    }

    /* Wait for a new key/button press */
    while (1) {
        uint8_t pad = joypad();
        char wb_char = workboy_get_char();
        if (pad != 0 || wb_char != 0) break;
        wait_vbl_done();
    }

    /* Wait for key release so button press doesn't bleed into input */
    while (joypad() != 0 || workboy_get_char() != 0) {
        wait_vbl_done();
    }

    /* Restore saved tiles */
    for (x = 0; x < 6u; x++) {
        set_bkg_tile_xy(14u + x, TEXT_BOTTOM, saved_tiles[x]);
    }

    lines_printed = 0;
}

/* -----------------------------------------------------------------------
 * Internal helpers
 * ----------------------------------------------------------------------- */
static void clear_row(uint8_t row) {
    uint8_t x;
    for (x = 0; x < COLS; x++) {
        set_bkg_tile_xy(x, row, 0x20u); /* 0x20 = space tile */
        if (row >= TEXT_TOP && row <= TEXT_BOTTOM) {
            text_screen[row - TEXT_TOP][x] = 0x20u;
        }
    }
}

static void scroll_up(void) {
    uint8_t row, x;
    /* Copy each row's tiles up one row in text_screen buffer and update VRAM */
    for (row = 0u; row < TEXT_ROWS - 1u; row++) {
        for (x = 0u; x < COLS; x++) {
            uint8_t tile = text_screen[row + 1u][x];
            if (text_screen[row][x] != tile) {
                text_screen[row][x] = tile;
                set_bkg_tile_xy(x, row + TEXT_TOP, tile);
            }
        }
    }
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
    lines_printed = 0;
    for (row = 0; row < 18u; row++) {
        clear_row(row);
    }
}

void z_render_reset_line_count(void) {
    lines_printed = 0;
}

void z_render_put_char(char c) {
    if (c == '\n' || c == '\r') {
        cur_x = 0;
        cur_y++;
        lines_printed++;
        if (cur_y > TEXT_BOTTOM) {
            if (lines_printed >= TEXT_ROWS - 1u) {
                show_more_prompt_and_wait();
            }
            scroll_up();
        }
        return;
    }

    if (c == '\b') {
        if (cur_x > 0) {
            cur_x--;
        } else if (cur_y > TEXT_TOP) {
            cur_y--;
            cur_x = COLS - 1u;
        } else {
            return;
        }
        set_bkg_tile_xy(cur_x, cur_y, 0x20u);
        if (cur_y >= TEXT_TOP && cur_y <= TEXT_BOTTOM) {
            text_screen[cur_y - TEXT_TOP][cur_x] = 0x20u;
        }
        return;
    }

    /* Auto-wrap: status bar (y=0) never wraps */
    if (cur_x >= COLS) {
        if (cur_y == 0) return;
        cur_x = 0;
        cur_y++;
        lines_printed++;
        if (cur_y > TEXT_BOTTOM) {
            if (lines_printed >= TEXT_ROWS - 1u) {
                show_more_prompt_and_wait();
            }
            scroll_up();
        }
    }

    set_bkg_tile_xy(cur_x, cur_y, (uint8_t)c);
    if (cur_y >= TEXT_TOP && cur_y <= TEXT_BOTTOM) {
        text_screen[cur_y - TEXT_TOP][cur_x] = (uint8_t)c;
    }
    cur_x++;
}

void z_render_show_candidate(char c) {
    if (cur_x >= COLS) {
        if (cur_y == 0) return;
        cur_x = 0;
        cur_y++;
        if (cur_y > TEXT_BOTTOM) scroll_up();
    }
    set_bkg_tile_xy(cur_x, cur_y, (uint8_t)c);
}

void z_render_clear_candidate(void) {
    if (cur_x < COLS) {
        set_bkg_tile_xy(cur_x, cur_y, 0x20u);
    }
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

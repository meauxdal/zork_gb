#include <gb/gb.h>
#include <stdint.h>
#include "z_vwf_render.h"

/* * MVP Fixed-Width Renderer
 * Maps characters directly to the 8x8 hardware tile grid.
 */

static uint8_t cursor_x = 0; // Tile column (0-19)
static uint8_t cursor_y = 1; // Tile row (0-17)

void vwf_init(void) {
    cursor_x = 0;
    cursor_y = 1;

    // Clear the visible background area (20x18 tiles)
    for (uint8_t y = 0; y < 18; y++) {
        for (uint8_t x = 0; x < 20; x++) {
            set_bkg_tile_xy(x, y, 0x20); // 0x20 is the ASCII space tile
        }
    }
}

void vwf_put_char(char c) {
    // Handle newline character
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }

    // Wrap to next line if we exceed the screen width
    if (cursor_x >= 20) {
        cursor_x = 0;
        cursor_y++;
    }

    // Basic vertical wrap (scroll logic can be implemented later)
    if (cursor_y >= 18) {
        cursor_y = 1;
    }

    /* * In the fixed-width model, we assume the VRAM contains the font
     * starting at tile 0. ASCII 'A' (65) will simply use tile 65.
     */
    set_bkg_tile_xy(cursor_x, cursor_y, (uint8_t)c);

    cursor_x++;
}

void vwf_flush_buffer(void) {
    // No-op in fixed-width mode as tiles are written immediately to VRAM.
}

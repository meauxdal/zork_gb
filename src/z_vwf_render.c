#### FILE: src / vwf_render.c
#include "vwf_render.h"
#include <gb/gb.h>
#include <string.h>

// Font metrics and buffers
extern const uint8_t zork_font[];        // Reference to external font data
extern const uint8_t zork_font_widths[]; // Width map for VWF

static uint8_t tile_buf[16];  // Current 8x8 tile being composed
static uint8_t cursor_x = 0;   // Pixel-based X position
static uint8_t cursor_y = 0;   // Tile-based Y position (row)
static uint8_t current_tile = 1;

void vwf_init(void) {
    cursor_x = 0;
    cursor_y = 1; // Row 0 is reserved for the Status Bar
    memset(tile_buf, 0, sizeof(tile_buf));
}

void vwf_put_char(char c) {
    if (c == '\n') {
        vwf_flush_buffer();
        cursor_x = 0;
        cursor_y++;
        if (cursor_y > 17) cursor_y = 1; // Simple wrap for now
        return;
    }

    uint8_t width = zork_font_widths[(uint8_t)c];
    const uint8_t* char_ptr = &zork_font[(uint8_t)c * 8];

    // Check if char fits on current line; if not, wrap
    if (cursor_x + width > 160) {
        vwf_put_char('\n');
    }

    // Render char into tile_buf via bit-shifting
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t line = char_ptr[i];
        // Shift bits into tile_buf (Simplified 1-bpp logic)
        tile_buf[i * 2] |= (line >> (cursor_x % 8));
    }

    cursor_x += width;

    // If we've crossed a tile boundary (8 pixels), flush and move to next tile
    if (cursor_x / 8 >= current_tile) {
        vwf_flush_buffer();
        current_tile++;
    }
}

void vwf_flush_buffer(void) {
    set_bkg_data(current_tile, 1, tile_buf);
    set_bkg_tiles(cursor_x / 8, cursor_y, 1, 1, &current_tile);
    memset(tile_buf, 0, sizeof(tile_buf));
}

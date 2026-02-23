// #### FILE: src/z_vwf_render.c
#include <gb/gb.h>
#include <stdint.h>
#include "z_vwf_render.h"
#include "z_memory.h"

// External font arrays
extern const uint8_t zork_font[256*8];
extern const uint8_t zork_font_widths[256];

// Cursor position for rendering
static uint8_t cursor_x;
static uint8_t cursor_y;

void vwf_init(void) {
    cursor_x = 0;
    cursor_y = 0;
    // Load font tiles into background tiles (assume bank 0)
    set_bkg_data(0, 256, zork_font);
}

void vwf_putc(char c) {
    uint8_t width = zork_font_widths[(uint8_t)c];
    // Draw each column as a background tile (simplified)
    set_bkg_tiles(cursor_x, cursor_y, width, 1, &zork_font[c*8]);
    cursor_x += width;
    if (cursor_x >= 20) { // Wrap at 20 tiles
        cursor_x = 0;
        cursor_y++;
    }
}

void vwf_puts(const char* s) {
    while (*s) {
        vwf_putc(*s++);
    }
}

void vwf_set_cursor(uint8_t x, uint8_t y) {
    cursor_x = x;
    cursor_y = y;
}

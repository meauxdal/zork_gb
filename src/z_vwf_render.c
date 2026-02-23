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
    set_bkg_tiles(cursor_x, cursor_y, width, 1, &zork_font[(uint8_t)c * 8]);
    cursor_x += width;
    if (cursor_x >= 20) { // Wrap at 20 tiles
        cursor_x = 0;
        cursor_y++;
    }
}

/* Alias — callers in z_string_decoder and workboy use this name */
void vwf_put_char(char c) {
    vwf_putc(c);
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

/* Position cursor at column `col` on the status bar row (row 0) */
void vwf_seek_status(uint8_t col) {
    cursor_x = col;
    cursor_y = 0;
}

/* Print "Score/Moves" or "HH:MM" stats right-aligned on the status row.
   Uses a simple right-edge print; a full implementation would pad to 20 cols. */
void vwf_print_status_stats(int16_t val1, int16_t val2) {
    /* Minimal: seek near the right edge and print the two values separated by '/'.
       A real implementation would use itoa and pad to fixed width. */
    vwf_set_cursor(14, 0);
    /* Print val1 */
    if (val1 < 0) { vwf_putc('-'); val1 = -val1; }
    if (val1 >= 100) vwf_putc('0' + (char)(val1 / 100));
    if (val1 >= 10)  vwf_putc('0' + (char)((val1 / 10) % 10));
    vwf_putc('0' + (char)(val1 % 10));
    vwf_putc('/');
    /* Print val2 */
    if (val2 < 0) { vwf_putc('-'); val2 = -val2; }
    if (val2 >= 100) vwf_putc('0' + (char)(val2 / 100));
    if (val2 >= 10)  vwf_putc('0' + (char)((val2 / 10) % 10));
    vwf_putc('0' + (char)(val2 % 10));
}

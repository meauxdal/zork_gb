#include <gb/gb.h>
#include <stdint.h>
#include "z_vwf_render.h"

/*
 * z_vwf_render.c -- Game Boy background-tile text renderer.
 *
 * Model:
 *   - zork_font_2bpp holds 256 glyphs in GB 2bpp format (16 bytes each).
 *   - vwf_init() uploads all 256 tiles to VRAM with set_bkg_data().
 *   - Tile index in VRAM == ASCII value of the character, so drawing a
 *     character is just writing its ASCII value as a tile index to the BG map.
 *   - The BG tilemap is 32x32 tiles. We use the top row for the status bar
 *     and rows 1-17 for the main text window (160x136 px visible area).
 *   - Cursor is tracked in tile units (x: 0-19, y: 0-17).
 *
 * "Variable width" at this stage means we advance the cursor by the glyph's
 * pixel width in tile units (rounded up). True sub-tile VWF needs a pixel
 * framebuffer approach and can be added later.
 */

extern const unsigned char zork_font_2bpp[256*16];
extern unsigned char zork_font_widths[256];
extern void zork_font_init(void);

/* Tile-unit dimensions of the visible screen */
#define SCREEN_TILES_W  20
#define SCREEN_TILES_H  18
#define STATUS_ROW       0
#define TEXT_ROW_START   1

static uint8_t cursor_x;
static uint8_t cursor_y;

/* Blank tile index -- ASCII space (0x20) has an all-zero glyph */
#define BLANK_TILE 0x20

void vwf_init(void) {
    cursor_x = 0;
    cursor_y = TEXT_ROW_START;

    zork_font_init();

    /* Upload all 256 glyphs (2bpp, 16 bytes each) to VRAM tile block 0.
       set_bkg_data(first_tile, num_tiles, data) */
    set_bkg_data(0, 256, zork_font_2bpp);

    /* Clear the visible tilemap to spaces */
    {
        uint8_t row, col;
        uint8_t blank_row[20];
        for (col = 0; col < 20; col++) blank_row[col] = BLANK_TILE;
        for (row = 0; row < SCREEN_TILES_H; row++) {
            set_bkg_tiles(0, row, SCREEN_TILES_W, 1, blank_row);
        }
    }
}

static void scroll_up(void) {
    /* Shift every text row up by one, then blank the bottom row */
    uint8_t row, col;
    uint8_t buf[20];
    for (row = TEXT_ROW_START; row < SCREEN_TILES_H - 1; row++) {
        /* BGB note: get_bkg_tiles is not in all GBDK versions;
           we track a shadow tilemap instead in a full impl.
           For now, a simple upward blit suffices for first-boot testing. */
        get_bkg_tiles(0, row + 1, SCREEN_TILES_W, 1, buf);
        set_bkg_tiles(0, row,     SCREEN_TILES_W, 1, buf);
    }
    /* blank last row */
    for (col = 0; col < 20; col++) buf[col] = BLANK_TILE;
    set_bkg_tiles(0, SCREEN_TILES_H - 1, SCREEN_TILES_W, 1, buf);
}

void vwf_putc(char c) {
    uint8_t tile = (uint8_t)c;

    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= SCREEN_TILES_H) {
            scroll_up();
            cursor_y = SCREEN_TILES_H - 1;
        }
        return;
    }

    /* Write the tile index (== ASCII value) to the BG map */
    set_bkg_tiles(cursor_x, cursor_y, 1, 1, &tile);
    cursor_x++;

    if (cursor_x >= SCREEN_TILES_W) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= SCREEN_TILES_H) {
            scroll_up();
            cursor_y = SCREEN_TILES_H - 1;
        }
    }
}

void vwf_put_char(char c) {
    vwf_putc(c);
}

void vwf_puts(const char *s) {
    while (*s) vwf_putc(*s++);
}

void vwf_set_cursor(uint8_t x, uint8_t y) {
    cursor_x = x;
    cursor_y = y;
}

void vwf_seek_status(uint8_t col) {
    cursor_x = col;
    cursor_y = STATUS_ROW;
}

void vwf_print_status_stats(int16_t val1, int16_t val2) {
    vwf_set_cursor(14, STATUS_ROW);
    if (val1 < 0) { vwf_putc('-'); val1 = -val1; }
    if (val1 >= 100) vwf_putc((char)('0' + val1 / 100));
    if (val1 >=  10) vwf_putc((char)('0' + (val1 / 10) % 10));
    vwf_putc((char)('0' + val1 % 10));
    vwf_putc('/');
    if (val2 < 0) { vwf_putc('-'); val2 = -val2; }
    if (val2 >= 100) vwf_putc((char)('0' + val2 / 100));
    if (val2 >=  10) vwf_putc((char)('0' + (val2 / 10) % 10));
    vwf_putc((char)('0' + val2 % 10));
}

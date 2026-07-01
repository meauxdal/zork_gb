/*
 * main.c
 *
 * Zork I for Game Boy — entry point.
 *
 * Init order matters:
 *   1. Load font into VRAM (must happen before display on)
 *   2. Init renderer (clears screen tiles)
 *   3. Display on
 *   4. z_mem_init() — copies dynamic segment from ROM to WRAM
 *   5. z_stack_init() — zeroes stacks
 *   6. z_dispatcher_init() — reads PC from story file header
 *   7. Set header flags to advertise our capabilities to the story
 *   8. Run
 */

#include <gb/gb.h>
#include <stdint.h>
#include "z_memory.h"
#include "z_dispatcher.h"
#include "z_variable_stack.h"
#include "z_render.h"
#include "z_status_bar.h"

extern const uint8_t zork_font_2bpp[]; /* defined in zork_font.c */

void main(void) {
    /* 1. Upload font — 256 tiles, 16 bytes each, starting at tile 0 */
    set_bkg_data(0u, 256u, zork_font_2bpp);

    /* 2. Renderer init (clears screen) */
    z_render_init();

    /* 3. Display on */
    SHOW_BKG;
    DISPLAY_ON;

    /* 4. Memory — copy dynamic segment to WRAM */
    z_mem_init();

    /* 5. Stack */
    z_stack_init();

    /* 6. Dispatcher — sets PC from story header */
    z_dispatcher_init();

    /*
     * 7. Header flags (byte 0x01):
     *    Bit 4 = status bar available (0=yes, 1=no in V3)
     *    Bit 5 = screen-splitting not available (we don't implement it)
     *    Bit 6 = variable-pitch font is default (we use fixed, so clear it)
     */
    uint8_t flags = z_read_byte(0x01u);
    flags &= ~0x10u; /* clear bit 4: status line IS available */
    flags &= ~0x20u; /* clear bit 5: no split */
    flags &= ~0x40u; /* clear bit 6: fixed-pitch */
    z_write_byte(0x01u, flags);

    /* 7a. Screen dimensions (V3 spec §11.1.2) */
    z_write_byte(0x20u, 18u); /* Height in rows */
    z_write_byte(0x21u, 20u); /* Width in chars */

    /* 8. Execute */
    while (1) {
        execute_next_instruction();
    }
}

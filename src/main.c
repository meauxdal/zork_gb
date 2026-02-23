#include <gb/gb.h>
#include <stdint.h>

// Standardized Z-Machine Core Includes
#include "z_memory.h"
#include "z_dispatcher.h"
#include "z_variable_stack.h"
#include "z_vwf_render.h"
#include "z_status_bar.h"
#include "workboy.h"

// External reference to the font data in zork_font.c
extern const uint8_t zork_font_2bpp[];

void main(void) {
    // 1. Hardware & Display Initialization
    // Upload the fixed-width font to VRAM (Tiles 0-255)
    // Each tile is 16 bytes in 2bpp format.
    set_bkg_data(0, 256, zork_font_2bpp);

    vwf_init();
    SHOW_BKG;
    DISPLAY_ON;

    // 2. Core Engine Initialization
    z_init_memory();      // Map ROM Bank 1 and copy Dynamic RAM
    z_stack_init();       // Zero out the evaluation and call frame stacks
    z_dispatcher_init();  // Set PC to the start address found in the header

    // 3. Set Header Flags
    // Set Bit 4 of byte 0x01 to inform the story file we support a status bar.
    uint8_t flags = z_read_byte(0x01);
    z_write_byte(0x01, flags | 0x10);

    // 4. The Execution Loop
    while (1) {
        execute_next_instruction();

        // Yield to the hardware at the end of each cycle 
        // to prevent display tearing and manage power consumption.
        wait_vbl_done();
    }
}

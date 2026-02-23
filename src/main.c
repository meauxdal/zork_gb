#### FILE: src / main.c
#include <gb/gb.h>
#include <stdint.h>
#include "z_memory.h"
#include "z_dispatcher.h"
#include "z_variable_stack.h"
#include "vwf_render.h"
#include "workboy.h"

void main(void) {
    // 1. Hardware Initialization
    DISPLAY_ON;
    SHOW_BKG;
    vwf_init();

    // 2. Z-Machine Initialization
    z_init_memory();      // Map ROM Bank 1 and copy Dynamic RAM
    z_dispatcher_init();  // Set PC to the start address in header

    // 3. Set Header Flags
    // Set Bit 4 of byte 0x01 to tell the game we support a status bar
    uint8_t flags = z_read_byte(0x01);
    z_write_byte(0x01, flags | 0x10);

    // 4. The Infinite Execution Loop
    while (1) {
        execute_next_instruction();

        // Safety: If the Z-Machine hits a HALT or error, 
        // we yield to the hardware to prevent battery drain.
        wait_vbl_done();
    }
}

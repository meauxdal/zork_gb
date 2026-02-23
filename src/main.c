/*
 * main.c
 * Purpose: Game Boy Hardware Initialization and Main Loop.
 */

#include <gb/gb.h>
#include <stdint.h>
#include "memory_core.h"
#include "vwf_render.h"
#include "z_dispatcher.h"
#include "z_status_bar.h"
#include "workboy.h"

void main(void) {
    // 1. Hardware Setup
    DISPLAY_OFF;
    vwf_init_display();
    workboy_init(); // Initialize keyboard interrupt vectors
    
    // 2. Z-Machine Initialization
    // Copy the first 4KB of Bank 1 into our WRAM dynamic buffer
    // This allows the engine to write to the 'header' and 'globals'
    for (uint16_t i = 0; i < DYNAMIC_MEM_SIZE; i++) {
        z_write_byte(i, z_read_byte(i));
    }

    // Set initial PC from Header (Offset 0x06)
    z_machine_pc = (uint32_t)z_read_word(0x06);

    // 3. Initial UI Draw
    update_status_bar();
    DISPLAY_ON;

    // 4. The Dispatch Loop
    // The interpreter will run until the game executes a 'QUIT' or 'RESTART'
    while (1) {
        execute_next_instruction();
    }
}

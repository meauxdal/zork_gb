/*
 * workboy.c
 * Purpose: Workboy Keyboard Driver for Game Boy / MiSTer.
 */

#include <gb/gb.h>
#include <stdint.h>
#include <stdio.h>
#include "workboy.h"
#include "z_vwf_render.h"

#define BUFFER_SIZE 128
static uint8_t input_buffer[BUFFER_SIZE];
static uint8_t buf_ptr = 0;

/* Forward declaration — defined at the bottom of this file */
void noworkboy_int_handler(void);

void workboy_init(void) {
    // Initialize Serial I/O registers for Workboy protocol
    // MiSTer's Workboy implementation expects standard serial clocking
    add_SIO(noworkboy_int_handler); // Placeholder for actual SIO ISR if needed
    buf_ptr = 0;
}

/* * Mimics a standard line-input function. 
 * This blocks until 'Enter' is pressed, echoing chars to the VWF layer.
 */
uint8_t workboy_read_line(uint8_t *buffer, uint8_t max_len) {
    uint8_t count = 0;
    uint8_t key;

    while (count < max_len) {
        // Poll for input (MiSTer / GBDK standard input)
        // In a physical Workboy, this would be an SIO interrupt.
        // For MiSTer compatibility, we use the standard getchar() mapping.
        key = getchar();

        if (key == '\r' || key == '\n') {
            buffer[count] = '\0';
            vwf_put_char('\n');
            break;
        } 
        else if (key == 8 || key == 127) { // Backspace
            if (count > 0) {
                count--;
                // Handle visual backspace (move cursor back and overwrite)
                // Note: vwf_render needs a backspace helper for perfection
            }
        }
        else if (key >= 32 && key <= 126) {
            buffer[count++] = key;
            vwf_put_char(key); // Echo to screen
        }
    }
    return count;
}

// Internal ISR placeholder
void noworkboy_int_handler(void) {
    // Handle raw SIO bytes from physical hardware here
}

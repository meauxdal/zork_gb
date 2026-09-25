#include "workboy.h"
#include <stdint.h>

extern uint8_t joypad_state;

/**
 * Flushes hardware joypad line states and software input buffers.
 * Prevents key mashing during [MORE] screen paging from leaking into
 * the game's command input buffer.
 */
void workboy_flush_input(void) {
    // Reset GB P1 joypad register
    *(volatile uint8_t*)0xFF00 = 0x30;
    
    // Clear software joypad state tracking
    joypad_state = 0;

    // Drain driver hardware buffer ring
    while (workboy_get_char() != 0) {
        // Discard stale character inputs
    }
}
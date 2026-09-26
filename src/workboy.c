#include "workboy.h"
#include <stdint.h>

#ifndef SB_REG
#include <gb/gb.h>
#endif

#ifndef SC_REG_WRITE
#define SC_REG_WRITE(val) (SC_REG = (val))
#endif

static char workboy_scancode_to_ascii(uint8_t code) {
    if (code == 0x0A) return '\n';
    if (code == 0x08) return '\b';
    if (code == 0x20) return ' ';
    if (code >= 0x31 && code <= 0x4A) return (char)('A' + (code - 0x31));
    if (code >= 'a' && code <= 'z') return (char)code;
    if (code >= 'A' && code <= 'Z') return (char)code;
    if (code >= '0' && code <= '9') return (char)code;
    if (code == ',' || code == '.' || code == '\'' || code == '-' || code == '?') return (char)code;
    return 0;
}

static uint8_t s_last_scancode = 0xFF;
static uint8_t s_sample_count = 0;
static uint8_t s_key_held = 0xFF;

char workboy_get_char(void) {
    SC_REG_WRITE(0x81);
    uint8_t raw = SB_REG;

    if (raw == 0xFF) {
        s_sample_count = 0;
        s_last_scancode = 0xFF;
        s_key_held = 0xFF;
        return 0;
    }

    char ch = workboy_scancode_to_ascii(raw);
    if (ch == 0) {
        s_sample_count = 0;
        s_last_scancode = 0xFF;
        s_key_held = 0xFF;
        return 0;
    }

    if (raw == s_key_held) {
        return 0;
    }

    if (raw == s_last_scancode) {
        s_sample_count++;
        if (s_sample_count == 3) {
            s_key_held = raw;
            return ch;
        }
    } else {
        s_last_scancode = raw;
        s_sample_count = 1;
    }

    return 0;
}

uint8_t workboy_is_key_down(void) {
    SC_REG_WRITE(0x81);
    uint8_t raw = SB_REG;
    if (raw == 0xFF) return 0;
    return (workboy_scancode_to_ascii(raw) != 0) ? 1 : 0;
}

/**
 * Flushes hardware joypad line states and software input buffers.
 * Prevents key mashing during [MORE] screen paging from leaking into
 * the game's command input buffer.
 */
void workboy_flush_input(void) {
    // Reset GB P1 joypad register
    *(volatile uint8_t*)0xFF00 = 0x30;

    s_sample_count = 0;
    s_last_scancode = 0xFF;
    s_key_held = 0xFF;

    // Drain driver hardware buffer ring
    while (workboy_get_char() != 0) {
        // Discard stale character inputs
    }
}

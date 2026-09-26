#include "workboy.h"
#include <stdint.h>

#ifndef SC_REG
#include <gb/gb.h>
#endif

extern uint8_t joypad_state;

static uint8_t prev_scancode = 0xFF;
static uint8_t consecutive_samples = 0;
static uint8_t last_confirmed_key = 0xFF;

static char scancode_to_ascii(uint8_t code) {
    if (code == 0x0A || code == 0x0D) return '\n';
    if (code == 0x08) return '\b';
    if (code == 0x20) return ' ';
    if (code >= 0x31 && code <= 0x4A) {
        return (char)('A' + (code - 0x31));
    }
    if (code >= 0x4B && code <= 0x54) {
        return (char)('0' + (code - 0x4B));
    }
    if (code >= 0x20 && code <= 0x7E) {
        return (char)code;
    }
    return 0;
}

uint8_t workboy_is_key_down(void) {
#ifdef SC_REG_WRITE
    SC_REG_WRITE(0x81);
#else
    SC_REG = 0x81;
#endif
    uint8_t raw = SB_REG;
    if (raw == 0xFF || raw == 0x00) return 0;
    return (scancode_to_ascii(raw) != 0) ? 1 : 0;
}

char workboy_get_char(void) {
#ifdef SC_REG_WRITE
    SC_REG_WRITE(0x81);
#else
    SC_REG = 0x81;
#endif
    uint8_t raw = SB_REG;
    char ascii = scancode_to_ascii(raw);

    if (raw == 0xFF || raw == 0x00 || ascii == 0) {
        prev_scancode = 0xFF;
        consecutive_samples = 0;
        last_confirmed_key = 0xFF;
        return 0;
    }

    if (raw == prev_scancode) {
        if (consecutive_samples < 255) {
            consecutive_samples++;
        }
        if (consecutive_samples >= 3) {
            if (raw != last_confirmed_key) {
                last_confirmed_key = raw;
                return ascii;
            }
        }
    } else {
        prev_scancode = raw;
        consecutive_samples = 1;
    }

    return 0;
}

void workboy_flush_input(void) {
#ifndef MOCK_GB_H
    *(volatile uint8_t*)0xFF00 = 0x30;
#endif
    joypad_state = 0;
    while (workboy_get_char() != 0) {
    }
}

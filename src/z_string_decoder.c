/*
 * z_string_decoder.c
 * Purpose: Z-Machine Version 3 string decoding (A0, A1, A2 alphabets).
 * Platform: Game Boy (Optimized for small-stack recursion)
 */

#include <stdint.h>
#include "memory_core.h"
#include "vwf_render.h"
#include "z_string_decoder.h"

// Alphabet tables as defined in the Z-Spec
static const char alpha_a0[] = "abcdefghijklmnopqrstuvwxyz";
static const char alpha_a1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char alpha_a2[] = " \n0123456789.,!?_#'\"/\\-:()";

typedef enum {
    ST_NORMAL,
    ST_ABBREV,    // Awaiting abbreviation index
    ST_ZSCII_HI,  // 10-bit ZSCII high 5 bits
    ST_ZSCII_LO   // 10-bit ZSCII low 5 bits
} DecodeState;

/* * Internal decoder logic. 
 * 'depth' prevents infinite abbreviation loops (Spec limit: 1)
 */
static void decode_zstring_inner(uint32_t address, uint8_t depth) {
    DecodeState state = ST_NORMAL;
    uint8_t active_alpha = 0;
    uint8_t abbrev_table = 0;
    uint8_t zscii_high = 0;

    for (;;) {
        uint16_t word = z_read_word(address);
        address += 2;

        uint8_t zchars[3];
        zchars[0] = (word >> 10) & 0x1F;
        zchars[1] = (word >> 5)  & 0x1F;
        zchars[2] = word & 0x1F;

        for (uint8_t i = 0; i < 3; i++) {
            uint8_t c = zchars[i];

            switch (state) {
                case ST_ABBREV: {
                    if (depth < 1) { // Guard against nested abbreviations
                        uint16_t abbrev_base = z_read_word(0x18);
                        // Abbrev entries are word-pointers (Address = Pointer * 2)
                        uint16_t entry_addr = z_read_word(abbrev_base + (((abbrev_table - 1) << 6) + (c << 1)));
                        decode_zstring_inner((uint32_t)entry_addr << 1, depth + 1);
                    }
                    state = ST_NORMAL;
                    break;
                }

                case ST_ZSCII_HI:
                    zscii_high = c;
                    state = ST_ZSCII_LO;
                    break;

                case ST_ZSCII_LO: {
                    uint8_t final_char = (zscii_high << 5) | c;
                    vwf_put_char(final_char);
                    state = ST_NORMAL;
                    break;
                }

                case ST_NORMAL:
                default:
                    if (c == 0) {
                        vwf_put_char(' ');
                    } else if (c >= 1 && c <= 3) {
                        abbrev_table = c;
                        state = ST_ABBREV;
                    } else if (c == 4) {
                        active_alpha = 1; // Shift to A1
                    } else if (c == 5) {
                        active_alpha = 2; // Shift to A2
                    } else {
                        // Character lookup [6..31]
                        if (active_alpha == 2 && c == 6) {
                            state = ST_ZSCII_HI;
                        } else {
                            char ch;
                            if (active_alpha == 0)      ch = alpha_a0[c - 6];
                            else if (active_alpha == 1) ch = alpha_a1[c - 6];
                            else                        ch = alpha_a2[c - 6];
                            
                            vwf_put_char((uint8_t)ch);
                            active_alpha = 0; // Return to A0
                        }
                    }
                    break;
            }
        }

        if (word & 0x8000) break; // Bit 15 signals end of string
    }
}

/* Public interface to decode at current address */
void decode_zstring(uint32_t address) {
    decode_zstring_inner(address, 0);
}

/* Helper to find where a string ends without printing (used by dispatcher) */
uint32_t zstring_end_addr(uint32_t address) {
    for (;;) {
        uint16_t word = z_read_word(address);
        address += 2;
        if (word & 0x8000) return address;
    }
}

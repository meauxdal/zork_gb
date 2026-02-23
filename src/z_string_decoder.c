/*
 * z_string_decoder.c
 *
 * Z-Machine Version 3 Z-string decoder.
 *
 * Z-strings are sequences of 16-bit words. The high bit of each word is
 * the end-of-string flag. Each word packs three 5-bit Z-characters.
 *
 * Alphabet tables (V3):
 *   A0 (default): a-z
 *   A1           : A-Z
 *   A2           : punctuation / digits
 *
 * Shift characters:
 *   4 = shift to A1 for next char only
 *   5 = shift to A2 for next char only
 *
 * Abbreviations:
 *   1, 2, 3 = next z-char N selects abbreviation (32*(abbrev-1)+N)
 *   Abbreviation strings live in the abbreviation table pointed to by
 *   header word at 0x18.
 *
 * A2[0] is a space (z-char 6), A2[1] is newline (z-char 7).
 * 10-bit escape: A2 + z-char 6 means the next two z-chars form a 10-bit
 * ZSCII code (high 5 bits first).
 */

#include "z_string_decoder.h"
#include "z_memory.h"
#include "z_render.h"

/* V3 alphabet tables, indexed [set][zchar - 6] */
static const char z_alpha[3][27] = {
    /* A0 */ "abcdefghijklmnopqrstuvwxyz",
    /* A1 */ "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    /* A2 */ " \n0123456789.,!?_#'\"/\\-:()"
};

/* -----------------------------------------------------------------------
 * Forward declaration for recursion (abbreviation lookup calls us back)
 * ----------------------------------------------------------------------- */
static uint32_t decode_zstring_inner(uint32_t address);

static void print_abbreviation(uint8_t abbrev_index) {
    /* Abbreviation table base is at header 0x18 as a word address */
    uint16_t abbrev_table = z_read_word(0x18u);
    /* Each entry is a word address (must be multiplied by 2) */
    uint16_t entry_addr = abbrev_table + (uint16_t)abbrev_index * 2u;
    uint32_t str_addr   = (uint32_t)z_read_word(entry_addr) * 2u;
    decode_zstring_inner(str_addr);
}

static uint32_t decode_zstring_inner(uint32_t address) {
    uint16_t word;
    uint8_t  zchars[3];
    uint8_t  current_set  = 0;
    uint8_t  abbrev_mode  = 0; /* 0 = normal, 1/2/3 = pending abbrev base */
    uint8_t  escape_hi    = 0; /* non-zero when we have the high 5 bits of escape */
    uint8_t  in_escape    = 0;

    do {
        word = z_read_word(address);
        address += 2u;

        zchars[0] = (uint8_t)((word >> 10) & 0x1Fu);
        zchars[1] = (uint8_t)((word >>  5) & 0x1Fu);
        zchars[2] = (uint8_t)( word        & 0x1Fu);

        uint8_t i;
        for (i = 0; i < 3u; i++) {
            uint8_t c = zchars[i];

            /* --- 10-bit ZSCII escape (A2 shift + zchar 6) --- */
            if (in_escape == 1u) {
                escape_hi = c;
                in_escape = 2u;
                continue;
            }
            if (in_escape == 2u) {
                /* Combine: high 5 bits + low 5 bits => ZSCII code */
                uint8_t zscii = (uint8_t)((escape_hi << 5) | c);
                z_render_put_char((char)zscii);
                in_escape    = 0;
                current_set  = 0;
                continue;
            }

            /* --- Abbreviation pending --- */
            if (abbrev_mode != 0u) {
                uint8_t idx = (uint8_t)((abbrev_mode - 1u) * 32u + c);
                print_abbreviation(idx);
                abbrev_mode  = 0;
                current_set  = 0;
                continue;
            }

            /* --- Normal z-character --- */
            if (c == 0u) {
                z_render_put_char(' ');
            } else if (c == 1u || c == 2u || c == 3u) {
                abbrev_mode = c;
            } else if (c == 4u) {
                current_set = 1u;
            } else if (c == 5u) {
                current_set = 2u;
            } else {
                /* c is 6..31 */
                if (current_set == 2u && c == 6u) {
                    /* Start 10-bit escape */
                    in_escape   = 1u;
                    current_set = 0u;
                } else {
                    z_render_put_char(z_alpha[current_set][c - 6u]);
                    current_set = 0u;
                }
            }
        }
    } while (!(word & 0x8000u));

    return address;
}

uint32_t decode_zstring(uint32_t address) {
    return decode_zstring_inner(address);
}

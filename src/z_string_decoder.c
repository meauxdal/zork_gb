#include "z_string_decoder.h"
#include "z_memory.h"
#include <stdint.h>
#include <stdbool.h>

static const char ALPHABET_A0[] = "abcdefghijklmnopqrstuvwxyz";
static const char ALPHABET_A1[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char ALPHABET_A2[] = " \n0123456789.,!"  "?_#\'\"/\\-:()";

/**
 * Calculates the absolute 32-bit byte address for a given Z-code abbreviation.
 * Performs 32-bit arithmetic to prevent 16-bit table pointer overflow.
 */
static uint32_t get_abbrev_address(uint8_t table, uint8_t index) {
    uint32_t abbrev_table_base = z_read_word(0x18);
    uint32_t entry_ptr = abbrev_table_base + (uint32_t)(2 * (32 * (table - 1) + index));
    uint16_t word_addr = z_read_word(entry_ptr);
    
    return (uint32_t)word_addr * 2;
}

/**
 * Recursively decodes Z-encoded string bytes into standard ASCII.
 * Uses 32-bit absolute addresses to retain proper bank alignment when returning
 * from nested abbreviation expansions.
 */
uint32_t decode_z_string(uint32_t addr, char* out_buf, int* out_idx, int max_len, int depth) {
    if (depth > 3) return addr; // Prevent recursion stack overflow

    uint8_t current_alphabet = 0;
    uint8_t shift_alphabet = 0;
    bool is_shifted = false;

    bool abbrev_mode = false;
    uint8_t abbrev_table = 0;

    bool zscii_mode = false;
    uint16_t zscii_char = 0;
    uint8_t zscii_stage = 0;

    bool end_of_string = false;

    while (!end_of_string && (*out_idx < max_len - 1)) {
        uint16_t word = z_read_word(addr);
        addr += 2;

        end_of_string = (word & 0x8000) != 0;

        uint8_t zchars[3] = {
            (uint8_t)((word >> 10) & 0x1F),
            (uint8_t)((word >> 5) & 0x1F),
            (uint8_t)(word & 0x1F)
        };

        for (int i = 0; i < 3; i++) {
            uint8_t zc = zchars[i];

            // Processing 10-bit raw ZSCII literal character
            if (zscii_mode) {
                if (zscii_stage == 0) {
                    zscii_char = (zc & 0x1F) << 5;
                    zscii_stage = 1;
                } else {
                    zscii_char |= (zc & 0x1F);
                    if (*out_idx < max_len - 1) {
                        out_buf[(*out_idx)++] = (char)zscii_char;
                    }
                    zscii_mode = false;
                    zscii_stage = 0;
                }
                continue;
            }

            // Expanding abbreviation Z-character (1, 2, 3)
            if (abbrev_mode) {
                uint32_t abbrev_addr = get_abbrev_address(abbrev_table, zc);
                decode_z_string(abbrev_addr, out_buf, out_idx, max_len, depth + 1);
                abbrev_mode = false;
                continue;
            }

            if (zc == 0) {
                if (*out_idx < max_len - 1) {
                    out_buf[(*out_idx)++] = ' ';
                }
            } else if (zc >= 1 && zc <= 3) {
                abbrev_mode = true;
                abbrev_table = zc;
                is_shifted = false; // Reset shift state prior to expansion
            } else if (zc == 4) {
                shift_alphabet = 1;
                is_shifted = true;
            } else if (zc == 5) {
                shift_alphabet = 2;
                is_shifted = true;
            } else {
                uint8_t active_alpha = is_shifted ? shift_alphabet : current_alphabet;
                char ch = ' ';

                if (active_alpha == 0) {
                    ch = ALPHABET_A0[zc - 6];
                } else if (active_alpha == 1) {
                    ch = ALPHABET_A1[zc - 6];
                } else if (active_alpha == 2) {
                    if (zc == 6) {
                        zscii_mode = true;
                        zscii_stage = 0;
                        is_shifted = false;
                        continue;
                    } else {
                        ch = ALPHABET_A2[zc - 6];
                    }
                }

                if (*out_idx < max_len - 1) {
                    out_buf[(*out_idx)++] = ch;
                }

                is_shifted = false;
            }
        }
    }

    out_buf[*out_idx] = '\0';
    return addr;
}
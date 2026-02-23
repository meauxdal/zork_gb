#include "z_string_decoder.h"
#include "z_memory.h"
#include "z_dispatcher.h"
#include "z_vwf_render.h"

static const char alphabet_fixed[] =
"abcdefghijklmnopqrstuvwxyz" // A0
"ABCDEFGHIJKLMNOPQRSTUVWXYZ" // A1
" \n0123456789.,!?_#'\"/\\-:()"; // A2

void decode_abbreviation(uint8_t abbr_num, uint8_t zchar) {
    // Header offset 0x18 contains the word address of the abbreviation table
    uint16_t abbr_table_base = z_read_word(0x18);

    // Each entry is a 2-byte word address. 
    // Formulas: (32 * (set-1)) + index
    uint16_t entry_addr = abbr_table_base + (uint16_t)(32 * (zchar - 1) + abbr_num) * 2;
    uint16_t word_addr = z_read_word(entry_addr);

    // Abbreviations are stored as Z-strings at word_addr * 2
    decode_zstring(word_addr * 2);
}

void decode_zstring(uint16_t address) {
    uint8_t current_alphabet = 0;
    uint8_t shift = 0;
    uint16_t word;
    uint8_t done = 0;

    while (!done) {
        word = z_read_word(address);
        address += 2;

        // Bit 15 is the "end of string" marker
        if (word & 0x8000) done = 1;

        // Extract three 5-bit Z-characters
        uint8_t zchars[3];
        zchars[0] = (word >> 10) & 0x1F;
        zchars[1] = (word >> 5) & 0x1F;
        zchars[2] = word & 0x1F;

        for (uint8_t i = 0; i < 3; i++) {
            uint8_t c = zchars[i];

            // Handle temporary alphabet shifts (zchars 4 and 5)
            if (shift) {
                current_alphabet = shift;
                shift = 0;
            }

            if (c == 0) {
                vwf_put_char(' ');
            }
            else if (c >= 1 && c <= 3) {
                // The NEXT zchar in the stream is the index into the abbreviation table
                // Since zchars are packed in 3s, we must fetch the next one carefully
                uint8_t next_zchar;
                i++;
                if (i < 3) {
                    next_zchar = zchars[i];
                }
                else {
                    // Index is the first character of the next word
                    uint16_t next_word = z_read_word(address);
                    next_zchar = (next_word >> 10) & 0x1F;
                    // We do NOT increment address here; the outer loop will handle it
                }
                decode_abbreviation(next_zchar, c);
            }
            else if (c == 4) {
                shift = 1; // Shift to Alphabet 1 (Uppercase)
            }
            else if (c == 5) {
                shift = 2; // Shift to Alphabet 2 (Punctuation/Numbers)
            }
            else {
                // Standard character lookup
                uint16_t lookup_idx = (uint16_t)(current_alphabet * 26) + (c - 6);
                vwf_put_char(alphabet_fixed[lookup_idx]);

                // Reset alphabet to A0 after a single character if it was shifted
                current_alphabet = 0;
            }
        }
    }
}

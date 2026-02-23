#include "z_string_decoder.h"
#include "z_memory.h"
#include "z_dispatcher.h"
#include "z_vwf_render.h"

static const char alphabet[] =
"abcdefghijklmnopqrstuvwxyz" // A0
"ABCDEFGHIJKLMNOPQRSTUVWXYZ" // A1
" \n0123456789.,!?_#'\"/\\-:()"; // A2

void decode_zstring(uint16_t address) {
    uint8_t current_alphabet = 0;
    uint8_t shift = 0;
    uint16_t word;
    uint8_t zchars[3];
    uint8_t done = 0;

    while (!done) {
        word = z_read_word(address);
        address += 2;

        if (word & 0x8000) done = 1; // End of string bit

        zchars[0] = (word >> 10) & 0x1F;
        zchars[1] = (word >> 5) & 0x1F;
        zchars[2] = word & 0x1F;

        for (uint8_t i = 0; i < 3; i++) {
            uint8_t c = zchars[i];

            if (shift) {
                current_alphabet = shift;
                shift = 0;
            }

            if (c == 0) {
                vwf_put_char(' ');
            }
            else if (c >= 1 && c <= 3) {
                // Abbreviations - For Zork I V3, usually ignored or simplified
            }
            else if (c == 4) {
                shift = 1; // Shift to A1
            }
            else if (c == 5) {
                shift = 2; // Shift to A2
            }
            else {
                // Standard character mapping
                uint8_t index = (current_alphabet * 26) + (c - 6);
                vwf_put_char(alphabet[index]);
                current_alphabet = 0; // Reset alphabet after one char
            }
        }
    }
}

void decode_zstring_at_pc(void) {
    uint16_t start_addr = z_machine_pc;
    decode_zstring(start_addr);

    // Advance PC past the end of the encoded string
    while (!(z_read_word(z_machine_pc) & 0x8000)) {
        z_machine_pc += 2;
    }
    z_machine_pc += 2; // Step over the final word
}

/* Returns the number of bytes occupied by the Z-string at `address` */
uint16_t get_zstring_length(uint16_t address) {
    uint16_t start = address;
    while (!(z_read_word(address) & 0x8000)) {
        address += 2;
    }
    return (address - start) + 2; /* include the final terminating word */
}

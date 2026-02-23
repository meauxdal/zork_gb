#include "z_string_decoder.h"
#include "z_memory.h"
#include "z_vwf_render.h"

/* * Fixed: Array bounds set to 27 to accommodate null terminator.
 * This resolves the Warning 147 and prevents stack corruption.
 */
static const char charset[3][27] = {
    "abcdefghijklmnopqrstuvwxyz",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    " \n0123456789.,!?_#'\"/\\-:()"
};

void decode_zstring(uint32_t address) {
    uint16_t word;
    uint8_t zchars[3];
    uint8_t current_set = 0;
    uint8_t abbreviation_mode = 0;

    do {
        word = z_read_word(address);
        address += 2;

        zchars[0] = (word >> 10) & 0x1F;
        zchars[1] = (word >> 5) & 0x1F;
        zchars[2] = word & 0x1F;

        for (uint8_t i = 0; i < 3; i++) {
            uint8_t c = zchars[i];
            if (abbreviation_mode) { abbreviation_mode = 0; continue; }

            if (c == 0) {
                vwf_put_char(' ');
            }
            else if (c >= 1 && c <= 3) {
                abbreviation_mode = c;
            }
            else if (c == 4) {
                current_set = 1;
            }
            else if (c == 5) {
                current_set = 2;
            }
            else if (c >= 6 && c <= 31) {
                vwf_put_char(charset[current_set][c - 6]);
                current_set = 0;
            }
        }
    } while (!(word & 0x8000));
}

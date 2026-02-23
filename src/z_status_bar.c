/*
 * z_status_bar.c
 * Purpose: Professional status line rendering for Z-Machine V3.
 */

#include <stdint.h>
#include "z_memory.h"
#include "z_variable_stack.h"
#include "z_object_engine.h"
#include "z_string_decoder.h"
#include "z_vwf_render.h"

void update_status_bar(void) {
    // V3 Convention:
    // Global 0: Current Room ID
    // Global 1: Score (or Hours)
    // Global 2: Moves (or Minutes)
    uint16_t room_id = get_variable(0x10);
    int16_t  val1    = (int16_t)get_variable(0x11);
    int16_t  val2    = (int16_t)get_variable(0x12);

    // 1. Move cursor to top-left
    vwf_seek_status(0);

    // 2. Decode the Room's short name
    // The name is a Z-string located at the start of the object's property table
    uint32_t prop_table_addr = get_object_property_table((uint8_t)room_id);
    uint8_t  name_len_in_words = z_read_byte(prop_table_addr);
    
    if (name_len_in_words > 0) {
        decode_zstring(prop_table_addr + 1);
    }

    // 3. Render the Score/Moves on the far right
    vwf_print_status_stats(val1, val2);
}

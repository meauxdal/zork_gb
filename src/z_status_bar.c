#include "z_status_bar.h"
#include "z_object_engine.h"  // CRITICAL: Resolves implicit declaration
#include "z_variable_stack.h"
#include "z_vwf_render.h"
#include "z_memory.h"
#include <gb/gb.h>

void update_status_bar(void) {
    // Clear top line
    for (uint8_t i = 0; i < 20; i++) {
        set_bkg_tile_xy(i, 0, 0x20);
    }

    // Get Player's location
    uint8_t player_parent = get_parent(1);

    // Set temporary cursor to row 0 for name output
    if (player_parent != 0) {
        // This will now find the prototype in z_object_engine.h
        get_object_name(player_parent);
    }
}

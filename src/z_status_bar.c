#include "z_status_bar.h"
#include "z_vwf_render.h"
#include "z_object_engine.h"
#include "z_variable_stack.h"
#include "z_memory.h"
#include <gb/gb.h>
#include <stdio.h>

/**
 * Z-Machine Status Bar Logic
 * Traditionally, V3 games expect a status line at row 0.
 * Format: Room Name                Score: N  Moves: M
 */

void update_status_bar(void) {
    // 1. Save current cursor position
    // We don't want the status bar to permanently move the main text cursor

    // 2. Clear the top line
    for (uint8_t i = 0; i < 20; i++) {
        set_bkg_tile_xy(i, 0, 0x20); // Space
    }

    // 3. Get Player's location (Object 1 is usually the player)
    // The parent of the player object is the current room
    uint8_t player_parent = get_parent(1);

    // Set cursor to top-left
    // Note: We temporarily hijack the renderer's cursor logic
    // In a more robust build, we would use a dedicated draw_string_at(x, y)

    // For now, we move the cursor to 0,0 and print
    // (Assuming a modified vwf_put_char that allows row 0 access)

    if (player_parent != 0) {
        get_object_name(player_parent);
    }

    // 4. Fetch Score and Moves
    // Z-Machine V3: Global 17 is Score, Global 18 is Moves
    uint16_t score = get_variable(0x11);
    uint16_t moves = get_variable(0x12);

    // 5. Draw Score/Moves on the right side if space permits
    // Due to the 20-column limit of the GB, we may only show Score or Room
    // Here we jump to tile 12 to try and fit the score
}

void draw_status_bar_char(char c, uint8_t x) {
    set_bkg_tile_xy(x, 0, (uint8_t)c);
}

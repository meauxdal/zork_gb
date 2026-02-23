/*
 * z_status_bar.c
 *
 * V3 status bar: "location name" on left, "Score: N  Turns: N" on right.
 *
 * V3 spec §8.2: global variable 1 (0x11) = current room object,
 * global 2 (0x12) = score, global 3 (0x13) = turns.
 */

#include <gb/gb.h>
#include <stdint.h>
#include "z_status_bar.h"
#include "z_render.h"
#include "z_object_engine.h"
#include "z_variable_stack.h"
#include "z_memory.h"

/* Clear status row */
static void clear_status(void) {
    uint8_t x;
    for (x = 0; x < 20u; x++) set_bkg_tile_xy(x, 0u, 0x20u);
}

void update_status_bar(void) {
    clear_status();

    z_render_status_begin(); /* redirect z_render_put_char to row 0 */

    /* Location name = object name of global 1 */
    uint8_t location = (uint8_t)get_variable(0x11u);
    if (location != 0u) get_object_name(location);

    z_render_status_end();
}

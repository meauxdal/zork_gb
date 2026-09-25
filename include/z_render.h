#ifndef Z_RENDER_H
#define Z_RENDER_H

#include <stdint.h>

/*
 * z_render.h
 *
 * Fixed-width 8x8 tile renderer for the Game Boy background layer.
 * One ASCII character == one tile. Tile index == ASCII value, so the
 * font in VRAM must be loaded with that layout (see zork_font.c).
 *
 * Screen is 20 columns x 18 rows. Row 0 is reserved for the status bar.
 * Text output starts at row 1.
 */

void z_render_init(void);
void z_render_put_char(char c);
void z_render_reset_line_count(void);

/* Candidate character display for hi-score style D-Pad entry */
void z_render_show_candidate(char c);
void z_render_clear_candidate(void);

/* Cursor control for the status bar — saves/restores main cursor. */
void z_render_status_begin(void);
void z_render_status_end(void);

#endif /* Z_RENDER_H */

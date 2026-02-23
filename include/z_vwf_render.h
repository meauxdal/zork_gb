// #### FILE: src/z_vwf_render.h
#ifndef Z_VWF_RENDER_H
#define Z_VWF_RENDER_H

#include <stdint.h>

void vwf_init(void);
void vwf_putc(char c);
void vwf_put_char(char c);   /* alias for vwf_putc; used by z_string_decoder & workboy */
void vwf_puts(const char* s);
void vwf_set_cursor(uint8_t x, uint8_t y);

/* Status bar helpers */
void vwf_seek_status(uint8_t col);
void vwf_print_status_stats(int16_t score, int16_t moves);

#endif

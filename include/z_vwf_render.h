// #### FILE: src/z_vwf_render.h
#ifndef Z_VWF_RENDER_H
#define Z_VWF_RENDER_H

#include <stdint.h>

void vwf_init(void);
void vwf_putc(char c);
void vwf_puts(const char* s);
void vwf_set_cursor(uint8_t x, uint8_t y);

#endif

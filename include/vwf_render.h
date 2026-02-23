#### FILE: include / vwf_render.h
#ifndef VWF_RENDER_H
#define VWF_RENDER_H

#include <stdint.h>

void vwf_init(void);
void vwf_put_char(char c);
void vwf_flush_buffer(void);
void vwf_clear_screen(void);

#endif

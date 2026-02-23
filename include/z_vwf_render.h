#ifndef Z_VWF_RENDER_H
#define Z_VWF_RENDER_H

#include <stdint.h>

void vwf_init(void);
void vwf_put_char(char c);
void vwf_flush_buffer(void);

#endif

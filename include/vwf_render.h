#ifndef VWF_RENDER_H
#define VWF_RENDER_H

#include <stdint.h>

#define TEXT_FIRST_ROW 2
#define STATUS_ROW     0

void vwf_init_display(void);
void vwf_clear_text(void);
void vwf_put_char(uint8_t chr);
void vwf_seek_status(uint8_t col);
void vwf_print_status_stats(int16_t val1, int16_t val2);

#endif

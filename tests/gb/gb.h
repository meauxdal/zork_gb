#ifndef MOCK_GB_H
#define MOCK_GB_H

#include <stdint.h>

void set_bkg_tile_xy(uint8_t x, uint8_t y, uint8_t tile);
void wait_vbl_done(void);

#define SWITCH_ROM(b) ((void)(b))

#endif /* MOCK_GB_H */

#ifndef WORKBOY_H
#define WORKBOY_H

#include <stdint.h>

char workboy_get_char(void);
uint8_t workboy_is_key_down(void);
void workboy_flush_input(void);

#endif

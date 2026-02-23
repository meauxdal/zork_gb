#ifndef WORKBOY_H
#define WORKBOY_H

#include <stdint.h>

void    workboy_init(void);
uint8_t workboy_read_line(uint8_t *buffer, uint8_t max_len);

#endif

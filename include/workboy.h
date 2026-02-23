#ifndef WORKBOY_H
#define WORKBOY_H

#include <stdint.h>

/**
 * Initializes the Workboy keyboard hardware.
 */
void workboy_init(void);

/**
 * Polls the keyboard for a single character.
 * Returns 0 if no key is pressed.
 */
char workboy_get_char(void);

#endif

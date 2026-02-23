#ifndef Z_VARIABLE_STACK_H
#define Z_VARIABLE_STACK_H

#include <stdint.h>

void z_stack_init(void);
void push_stack(uint16_t value);
uint16_t pop_stack(void);
uint16_t get_variable(uint8_t var);
void set_variable(uint8_t var, uint16_t value);
void call_routine(uint16_t packed_addr, uint8_t num_args, uint16_t* args, uint8_t store_var);
void return_from_routine(uint16_t value);

#endif

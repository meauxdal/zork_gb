#### FILE: include / z_variable_stack.h
#ifndef Z_VARIABLE_STACK_H
#define Z_VARIABLE_STACK_H

#include <stdint.h>

#define STACK_DEPTH 256

// Variable Access
uint16_t get_variable(uint8_t var);
void set_variable(uint8_t var, uint16_t value);

// Stack Operations
void push_stack(uint16_t value);
uint16_t pop_stack(void);

// Routine Management
void call_routine(uint16_t packed_addr, uint16_t* args, uint8_t arg_count, uint8_t store_var);
void return_from_routine(uint16_t return_value);

#endif

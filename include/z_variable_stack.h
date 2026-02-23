#ifndef Z_VARIABLE_STACK_H
#define Z_VARIABLE_STACK_H

#include <stdint.h>

#define MAX_EVAL_STACK 128
#define MAX_CALL_DEPTH 32

typedef struct {
    uint32_t return_pc;
    uint16_t locals[15];
    uint16_t sp_at_entry;
    uint8_t  num_locals;
    uint8_t  store_var;
    uint8_t  discard_result;
} RoutineFrame;

uint16_t get_variable(uint8_t var_id);
void     set_variable(uint8_t var_id, uint16_t value);
void     call_routine(uint16_t packed_addr, uint16_t *args, uint8_t count, uint8_t store_var, uint8_t discard);
void     return_from_routine(uint16_t return_value);

#endif

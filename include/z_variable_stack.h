#ifndef Z_VARIABLE_STACK_H
#define Z_VARIABLE_STACK_H

#include <stdint.h>

/*
 * z_variable_stack.h
 *
 * Z-Machine evaluation stack and call frame management.
 *
 * Variable numbering (V3 spec §4.2):
 *   0x00        : top of evaluation stack (pop on read, push on write)
 *   0x01..0x0F  : local variables of current routine
 *   0x10..0xFF  : global variables (stored in z-file at globals table)
 */

void     z_stack_init(void);

/* Evaluation stack */
void     push_stack(uint16_t value);
uint16_t pop_stack(void);
uint16_t peek_stack(void);

#define MAX_STACK   64
#define MAX_FRAMES   8

typedef struct {
    uint32_t return_pc;     /* PC to restore on return */
    uint8_t  store_var;     /* variable to receive return value */
    uint8_t  num_locals;    /* number of locals in this frame */
    uint16_t locals[15];    /* V3: up to 15 locals */
    uint16_t stack_base;    /* sp value at frame entry (for unwinding) */
} z_frame;

extern uint16_t z_stack[MAX_STACK];
extern uint8_t  sp;
extern z_frame  call_stack[MAX_FRAMES];
extern int8_t   fp;

/* Variable access */
uint16_t get_variable(uint8_t var);
void     set_variable(uint8_t var, uint16_t value);

/* Call frames */
void     call_routine(uint16_t packed_addr, uint16_t *args, uint8_t num_args, uint8_t store_var);
void     return_from_routine(uint16_t value);

#endif /* Z_VARIABLE_STACK_H */

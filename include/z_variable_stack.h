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

/* Variable access */
uint16_t get_variable(uint8_t var);
void     set_variable(uint8_t var, uint16_t value);

/* Call frames */
void     call_routine(uint16_t packed_addr, uint16_t *args, uint8_t num_args, uint8_t store_var);
void     return_from_routine(uint16_t value);

#endif /* Z_VARIABLE_STACK_H */

#ifndef Z_DISPATCHER_H
#define Z_DISPATCHER_H

#include <stdint.h>

/*
 * z_dispatcher.h
 *
 * Z-Machine fetch-decode-execute cycle.
 * z_machine_pc is global so call frame save/restore in z_variable_stack.c
 * can access it directly without an accessor function.
 */

extern uint32_t z_machine_pc;

void z_dispatcher_init(void);
void execute_next_instruction(void);
void z_tokenize(uint16_t text_buf, uint16_t parse_buf);

#endif /* Z_DISPATCHER_H */

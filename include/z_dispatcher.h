#ifndef Z_DISPATCHER_H
#define Z_DISPATCHER_H

#include <stdint.h>

/**
 * Z-Machine Instruction Dispatcher
 * Controls the fetch-decode-execute cycle and PC management.
 */

 // Global Program Counter (PC) - 32-bit to support banked ROM access
extern uint32_t z_machine_pc;

/**
 * Reads the entry address from the story file header and sets the PC.
 */
void z_dispatcher_init(void);

/**
 * The core execution step. Decodes the opcode at the current PC,
 * performs the operation, and advances the PC.
 */
void execute_next_instruction(void);

#endif

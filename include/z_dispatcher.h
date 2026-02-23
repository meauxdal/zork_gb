#ifndef Z_DISPATCHER_H
#define Z_DISPATCHER_H

#include <stdint.h>

extern uint32_t z_machine_pc;

void z_dispatcher_init(void);
void execute_next_instruction(void);

#endif

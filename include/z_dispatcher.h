#### FILE: include/z_dispatcher.h
#ifndef Z_DISPATCHER_H
#define Z_DISPATCHER_H

#include <stdint.h>

// The Program Counter (Global)
extern uint16_t z_machine_pc;

// Core Dispatcher Functions
void z_dispatcher_init(void);
void z_execute_cycle(void);

#endif

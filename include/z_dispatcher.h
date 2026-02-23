#ifndef Z_DISPATCHER_H
#define Z_DISPATCHER_H

void execute_next_instruction(void);
void handle_branch(uint8_t condition);
void handle_store(uint16_t value);

#endif

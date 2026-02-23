#### FILE: src / z_variable_stack.c
#include "z_variable_stack.h"
#include "z_memory.h"
#include "z_dispatcher.h"
#include <string.h>

// Evaluation Stack
static uint16_t z_stack[STACK_DEPTH];
static uint8_t  sp = 0;

// Current Routine State
static uint16_t local_vars[15];
static uint8_t  current_store_var;
static uint8_t  current_num_locals = 0;

// Call Frame Structure
typedef struct {
    uint32_t return_pc;
    uint8_t  store_variable;
    uint8_t  num_locals;
    uint16_t locals[15];
    uint8_t  previous_sp;
} CallFrame;

static CallFrame call_stack[MAX_CALL_DEPTH];
static uint8_t call_sp = 0;

void z_stack_init(void) {
    sp = 0;
    call_sp = 0;
    current_num_locals = 0;
    memset(local_vars, 0, sizeof(local_vars));
}

uint16_t get_variable(uint8_t var) {
    if (var == 0) return pop_stack();
    if (var < 0x10) return local_vars[var - 1];

    uint16_t global_table = z_read_word(0x0C);
    return z_read_word(global_table + (var - 0x10) * 2);
}

void set_variable(uint8_t var, uint16_t value) {
    if (var == 0) {
        push_stack(value);
    }
    else if (var < 0x10) {
        local_vars[var - 1] = value;
    }
    else {
        uint16_t global_table = z_read_word(0x0C);
        z_write_byte(global_table + (var - 0x10) * 2, (uint8_t)(value >> 8));
        z_write_byte(global_table + (var - 0x10) * 2 + 1, (uint8_t)(value & 0xFF));
    }
}

void push_stack(uint16_t value) {
    if (sp < STACK_DEPTH) z_stack[sp++] = value;
}

uint16_t pop_stack(void) {
    return (sp > 0) ? z_stack[--sp] : 0;
}

void call_routine(uint16_t packed_addr, uint16_t* args, uint8_t arg_count, uint8_t store_var) {
    if (packed_addr == 0) {
        // Calling address 0 simply returns 0 immediately
        set_variable(store_var, 0);
        return;
    }

    // Protect against stack overflow
    if (call_sp >= MAX_CALL_DEPTH) {
        // In a full build, we would trigger a fatal error screen here
        return;
    }

    // 1. Snapshot the current state into the call frame
    CallFrame* frame = &call_stack[call_sp++];
    frame->return_pc = z_machine_pc;
    frame->store_variable = store_var;
    frame->num_locals = current_num_locals;
    frame->previous_sp = sp;
    for (uint8_t i = 0; i < 15; i++) {
        frame->locals[i] = local_vars[i];
    }

    // 2. Set up the new routine context
    uint32_t real_addr = (uint32_t)packed_addr * 2;
    z_machine_pc = (uint16_t)real_addr; // Note: For a strictly V3 mapping, this fits in 16 bits if ROM is banked

    current_num_locals = z_read_byte(z_machine_pc++);

    // Read initial local values
    for (uint8_t i = 0; i < current_num_locals; i++) {
        local_vars[i] = z_read_word(z_machine_pc);
        z_machine_pc += 2;
    }

    // Override locals with passed arguments
    for (uint8_t i = 0; i < arg_count && i < current_num_locals; i++) {
        local_vars[i] = args[i];
    }

    current_store_var = store_var;
}

void return_from_routine(uint16_t return_value) {
    if (call_sp == 0) {
        // Stack underflow; the game is exiting
        return;
    }

    // 1. Pop the previous state
    CallFrame* frame = &call_stack[--call_sp];

    // 2. Restore state
    z_machine_pc = frame->return_pc;
    current_num_locals = frame->num_locals;
    sp = frame->previous_sp; // Truncates the evaluation stack

    for (uint8_t i = 0; i < 15; i++) {
        local_vars[i] = frame->locals[i];
    }

    // 3. Store the return value using the newly restored context
    set_variable(frame->store_variable, return_value);
}

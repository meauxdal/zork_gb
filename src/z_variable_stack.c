/*
 * z_variable_stack.c
 * Purpose: Local/Global variable management and Call Stack state.
 * Platform: Game Boy (Big-Endian aware)
 */

#include <stdint.h>
#include "memory_core.h"
#include "z_variable_stack.h"

// The evaluation stack for 2OP/1OP results
uint16_t z_eval_stack[MAX_EVAL_STACK];
uint16_t sp = 0;

// The call stack for tracking routine nesting
RoutineFrame call_stack[MAX_CALL_DEPTH];
uint8_t fp = 0;

/* Resolves a Z-Machine variable ID to its current value */
uint16_t get_variable(uint8_t var_id) {
    // 0x00: The Evaluation Stack (Pop)
    if (var_id == 0) {
        if (sp == 0) return 0; // Underflow safety
        return z_eval_stack[--sp];
    }

    // 0x01 - 0x0F: Local Variables for the current frame
    if (var_id < 0x10) {
        return call_stack[fp].locals[var_id - 1];
    }

    // 0x10 - 0xFF: Global Variables (stored in ROM/RAM header)
    // Global table address is found at word-offset 0x0C in the header
    uint16_t global_base = z_read_word(0x0C);
    return z_read_word(global_base + ((uint32_t)(var_id - 0x10) << 1));
}

/* Updates a Z-Machine variable ID with a new value */
void set_variable(uint8_t var_id, uint16_t value) {
    // 0x00: The Evaluation Stack (Push)
    if (var_id == 0) {
        if (sp < MAX_EVAL_STACK) {
            z_eval_stack[sp++] = value;
        }
        return;
    }

    // 0x01 - 0x0F: Local Variables
    if (var_id < 0x10) {
        call_stack[fp].locals[var_id - 1] = value;
        return;
    }

    // 0x10 - 0xFF: Global Variables
    uint16_t global_base = z_read_word(0x0C);
    z_write_word(global_base + ((uint32_t)(var_id - 0x10) << 1), value);
}

/* Prepares a new stack frame and jumps to a routine address */
void call_routine(uint16_t packed_addr, uint16_t *args, uint8_t arg_count, uint8_t store_var, uint8_t discard_result) {
    // Calling address 0 is a special case in V3: returns False.
    if (packed_addr == 0) {
        if (!discard_result) set_variable(store_var, 0);
        return;
    }

    // Commercial Grade Guard: Prevent stack overflow
    if (fp >= MAX_CALL_DEPTH - 1) return;

    // V3 Packed addresses are Byte Address / 2.
    uint32_t routine_addr = (uint32_t)packed_addr << 1;

    // Push new frame
    fp++;
    call_stack[fp].return_pc      = z_machine_pc;
    call_stack[fp].sp_at_entry    = sp;
    call_stack[fp].store_var      = store_var;
    call_stack[fp].discard_result = discard_result;

    // Read routine header: first byte is the number of local variables
    uint8_t num_locals = z_read_byte(routine_addr++);
    call_stack[fp].num_locals = num_locals;

    // Load initial local values from the routine definition
    for (uint8_t i = 0; i < num_locals; i++) {
        call_stack[fp].locals[i] = z_read_word(routine_addr);
        routine_addr += 2;
    }

    // Overwrite locals with the arguments actually passed by the caller
    for (uint8_t i = 0; i < arg_count && i < num_locals; i++) {
        call_stack[fp].locals[i] = args[i];
    }

    // Move Program Counter to the first instruction of the routine
    z_machine_pc = routine_addr;
}

/* Tears down the current frame and returns to the caller */
void return_from_routine(uint16_t return_value) {
    if (fp == 0) return; // Cannot return from the top-level main loop

    uint32_t resume_pc = call_stack[fp].return_pc;
    uint8_t  dest_var  = call_stack[fp].store_var;
    uint8_t  discard   = call_stack[fp].discard_result;

    // Reset the evaluation stack to its state before the call
    sp = call_stack[fp].sp_at_entry;
    fp--;

    z_machine_pc = resume_pc;

    if (!discard) {
        set_variable(dest_var, return_value);
    }
}

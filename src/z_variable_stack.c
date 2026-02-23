#### FILE: src / z_variable_stack.c
#include "z_variable_stack.h"
#include "z_memory.h"
#include "z_dispatcher.h"

static uint16_t z_stack[STACK_DEPTH];
static uint8_t  sp = 0; // Stack Pointer

// Local variables for the current routine
static uint16_t local_vars[15];
static uint8_t  current_store_var;

uint16_t get_variable(uint8_t var) {
    if (var == 0) {
        return pop_stack();
    }
    else if (var < 0x10) {
        return local_vars[var - 1];
    }
    else {
        // Global variables are stored in the header's global table
        uint16_t global_table = z_read_word(0x0C);
        return z_read_word(global_table + (var - 0x10) * 2);
    }
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
        // Global table writes must be byte-swapped back to Big-Endian
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
        set_variable(store_var, 0);
        return;
    }

    // In V3, packed addresses are multiplied by 2
    uint32_t real_addr = (uint32_t)packed_addr * 2;

    // Save current state (Minimal implementation for now)
    // Professional version would push current PC and locals to a Meta-Stack
    current_store_var = store_var;

    // Set PC to the start of the routine
    z_machine_pc = (uint16_t)real_addr;

    // Read local variable count and initial values from the routine header
    uint8_t num_locals = z_read_byte(z_machine_pc++);
    for (uint8_t i = 0; i < num_locals; i++) {
        local_vars[i] = z_read_word(z_machine_pc);
        z_machine_pc += 2;
    }

    // Override locals with passed arguments
    for (uint8_t i = 0; i < arg_count; i++) {
        local_vars[i] = args[i];
    }
}

void return_from_routine(uint16_t return_value) {
    // Note: A full implementation requires tracking the return PC
    // For now, we set the result in the expected variable
    set_variable(current_store_var, return_value);
}

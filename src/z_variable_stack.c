#include "z_variable_stack.h"
#include "z_memory.h"
#include "z_dispatcher.h"
#include <string.h>

/* * Z-Machine Variable and Stack Atom
 * Manages the evaluation stack and nested routine call frames.
 */

#define MAX_STACK 128
#define MAX_FRAMES 16

typedef struct {
    uint32_t return_pc;      // Where to go after return
    uint8_t  store_var;      // Which variable to store the result in
    uint8_t  num_locals;     // Number of local variables in this frame
    uint16_t locals[15];     // V3 allows up to 15 locals
    uint16_t stack_base;     // The SP value when this frame started
} z_frame;

static uint16_t z_stack[MAX_STACK];
static uint8_t  sp = 0;

static z_frame  call_stack[MAX_FRAMES];
static int8_t   fp = -1; // Frame Pointer

void z_stack_init(void) {
    sp = 0;
    fp = -1;
    memset(z_stack, 0, sizeof(z_stack));
    memset(call_stack, 0, sizeof(call_stack));
}

void push_stack(uint16_t value) {
    if (sp < MAX_STACK) {
        z_stack[sp++] = value;
    }
}

uint16_t pop_stack(void) {
    if (sp > 0) {
        return z_stack[--sp];
    }
    return 0;
}

uint16_t get_variable(uint8_t var) {
    // 0x00: Top of Stack
    if (var == 0) {
        return pop_stack();
    }

    // 0x01 - 0x0F: Local Variables
    if (var < 0x10) {
        if (fp >= 0 && (var - 1) < call_stack[fp].num_locals) {
            return call_stack[fp].locals[var - 1];
        }
        return 0;
    }

    // 0x10 - 0xFF: Global Variables
    // The Global Table address is found at header offset 0x0C
    uint16_t global_table = z_read_word(0x0C);
    return z_read_word(global_table + (uint16_t)(var - 0x10) * 2);
}

void set_variable(uint8_t var, uint16_t value) {
    if (var == 0) {
        push_stack(value);
    }
    else if (var < 0x10) {
        if (fp >= 0 && (var - 1) < call_stack[fp].num_locals) {
            call_stack[fp].locals[var - 1] = value;
        }
    }
    else {
        uint16_t global_table = z_read_word(0x0C);
        z_write_word(global_table + (uint16_t)(var - 0x10) * 2, value);
    }
}

void call_routine(uint16_t packed_addr, uint8_t num_args, uint16_t* args, uint8_t store_var) {
    if (fp >= MAX_FRAMES - 1) return; // Stack Overflow

    // In V3, packed addresses are multiplied by 2
    uint32_t target_addr = (uint32_t)packed_addr * 2;
    if (target_addr == 0) {
        set_variable(store_var, 0);
        return;
    }

    fp++;
    z_frame* frame = &call_stack[fp];
    frame->return_pc = z_machine_pc;
    frame->store_var = store_var;
    frame->stack_base = sp;

    // Read initial locals from the routine header
    uint8_t num_locals = z_read_byte(target_addr++);
    frame->num_locals = num_locals;

    for (uint8_t i = 0; i < num_locals; i++) {
        uint16_t default_val = z_read_word(target_addr);
        target_addr += 2;
        // Use passed argument if available, otherwise use default
        frame->locals[i] = (i < num_args) ? args[i] : default_val;
    }

    z_machine_pc = target_addr;
}

void return_from_routine(uint16_t value) {
    if (fp < 0) return;

    z_frame* frame = &call_stack[fp];
    z_machine_pc = frame->return_pc;
    uint8_t store_var = frame->store_var;
    sp = frame->stack_base; // Unwind the evaluation stack

    fp--;
    set_variable(store_var, value);
}

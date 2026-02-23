/*
 * z_variable_stack.c
 *
 * Z-Machine evaluation stack and call frame management for V3.
 *
 * Stack layout in WRAM:
 *   z_stack[]    — 16-bit evaluation stack, grows up from index 0
 *   call_stack[] — fixed array of call frames, indexed by fp
 *
 * Memory budget on GB:
 *   MAX_STACK 64  = 128 bytes
 *   MAX_FRAMES 8  = 8 * sizeof(z_frame) = 8 * (4+1+1+30+2) = 304 bytes
 *   Total ~432 bytes of WRAM, acceptable.
 */

#include "z_variable_stack.h"
#include "z_memory.h"
#include "z_dispatcher.h"
#include <string.h>

#define MAX_STACK   64
#define MAX_FRAMES   8

typedef struct {
    uint32_t return_pc;     /* PC to restore on return */
    uint8_t  store_var;     /* variable to receive return value */
    uint8_t  num_locals;    /* number of locals in this frame */
    uint16_t locals[15];    /* V3: up to 15 locals */
    uint16_t stack_base;    /* sp value at frame entry (for unwinding) */
} z_frame;

static uint16_t z_stack[MAX_STACK];
static uint8_t  sp = 0;

static z_frame  call_stack[MAX_FRAMES];
static int8_t   fp = -1;

/* -----------------------------------------------------------------------
 * Init
 * ----------------------------------------------------------------------- */
void z_stack_init(void) {
    sp = 0;
    fp = -1;
    memset(z_stack,    0, sizeof(z_stack));
    memset(call_stack, 0, sizeof(call_stack));
}

/* -----------------------------------------------------------------------
 * Evaluation stack
 * ----------------------------------------------------------------------- */
void push_stack(uint16_t value) {
    if (sp < MAX_STACK) z_stack[sp++] = value;
}

uint16_t pop_stack(void) {
    if (sp > 0) return z_stack[--sp];
    return 0;
}

/* -----------------------------------------------------------------------
 * Variable access (spec §4.2)
 * ----------------------------------------------------------------------- */
uint16_t get_variable(uint8_t var) {
    if (var == 0x00u) {
        return pop_stack();
    }
    if (var <= 0x0Fu) {
        if (fp >= 0 && (var - 1u) < call_stack[fp].num_locals)
            return call_stack[fp].locals[var - 1u];
        return 0;
    }
    /* Global: table base at header 0x0C */
    uint16_t base = z_read_word(0x0Cu);
    return z_read_word(base + (uint16_t)(var - 0x10u) * 2u);
}

void set_variable(uint8_t var, uint16_t value) {
    if (var == 0x00u) {
        push_stack(value);
        return;
    }
    if (var <= 0x0Fu) {
        if (fp >= 0 && (var - 1u) < call_stack[fp].num_locals)
            call_stack[fp].locals[var - 1u] = value;
        return;
    }
    uint16_t base = z_read_word(0x0Cu);
    z_write_word(base + (uint16_t)(var - 0x10u) * 2u, value);
}

/* -----------------------------------------------------------------------
 * Call frames
 *
 * Signature: args first, then num_args — matches dispatcher call site.
 * packed_addr * 2 = byte address of routine in z-file (V3 spec §5).
 * ----------------------------------------------------------------------- */
void call_routine(uint16_t packed_addr, uint16_t *args, uint8_t num_args, uint8_t store_var) {
    uint32_t target = (uint32_t)packed_addr * 2u;

    /* Calling address 0 is legal in Z-machine; it's a no-op that stores 0 */
    if (target == 0u) {
        set_variable(store_var, 0u);
        return;
    }

    if (fp >= MAX_FRAMES - 1) return; /* call stack overflow — ignore */

    fp++;
    z_frame *frame    = &call_stack[fp];
    frame->return_pc  = z_machine_pc;
    frame->store_var  = store_var;
    frame->stack_base = sp;

    uint8_t num_locals = z_read_byte(target++);
    if (num_locals > 15u) num_locals = 15u;
    frame->num_locals = num_locals;

    uint8_t i;
    for (i = 0; i < num_locals; i++) {
        uint16_t def = z_read_word(target);
        target += 2u;
        frame->locals[i] = (i < num_args) ? args[i] : def;
    }

    z_machine_pc = target;
}

void return_from_routine(uint16_t value) {
    if (fp < 0) return;

    z_frame *frame  = &call_stack[fp];
    z_machine_pc    = frame->return_pc;
    uint8_t sv      = frame->store_var;
    sp              = frame->stack_base;
    fp--;

    set_variable(sv, value);
}

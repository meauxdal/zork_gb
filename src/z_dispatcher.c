#### FILE: src/z_dispatcher.c
#include "z_dispatcher.h"
#include "z_memory.h"
#include "z_variable_stack.h"
#include "z_string_decoder.h"
#include "z_object_engine.h"
#include "workboy.h"
#include <stdio.h>

uint16_t z_machine_pc = 0;

/* Helper: Branching Logic */
void handle_branch(uint8_t condition) {
    uint8_t branch_byte = z_fetch_byte();
    uint8_t reverse = !(branch_byte & 0x80);
    uint16_t offset;

    if (branch_byte & 0x40) {
        offset = branch_byte & 0x3F;
    } else {
        offset = ((branch_byte & 0x3F) << 8) | z_fetch_byte();
        if (offset & 0x2000) offset |= 0xC000; // Sign extend 14-bit
    }

    if (condition == !reverse) {
        if (offset == 0) return_from_routine(0);
        else if (offset == 1) return_from_routine(1);
        else z_machine_pc += (int16_t)offset - 2;
    }
}

/* Helper: Storage Logic */
void handle_store(uint16_t value) {
    uint8_t variable = z_fetch_byte();
    set_variable(variable, value);
}

void execute_next_instruction(void) {
    uint8_t opcode = z_fetch_byte();
    uint16_t operands[4];
    
    // --- 2OP Opcodes (Long Form) ---
    if (opcode < 0x80) {
        operands[0] = (opcode & 0x40) ? get_variable(z_fetch_byte()) : z_fetch_byte();
        operands[1] = (opcode & 0x20) ? get_variable(z_fetch_byte()) : z_fetch_byte();
        
        switch (opcode & 0x1F) {
            case 0x01: handle_branch(operands[0] == operands[1]); break; // JE
            case 0x05: handle_branch((int16_t)operands[0] < (int16_t)operands[1]); break; // JL
            case 0x06: handle_branch((int16_t)operands[0] > (int16_t)operands[1]); break; // JG
            case 0x0D: handle_store(operands[0] | operands[1]); break; // OR
            case 0x0E: handle_store(operands[0] & operands[1]); break; // AND
            case 0x14: handle_store(operands[0] + operands[1]); break; // ADD
            case 0x15: handle_store(operands[0] - operands[1]); break; // SUB
            case 0x16: handle_store(operands[0] * operands[1]); break; // MUL
            case 0x17: handle_store((int16_t)operands[0] / (int16_t)operands[1]); break; // DIV
            case 0x18: handle_store((int16_t)operands[0] % (int16_t)operands[1]); break; // MOD
        }
    } 
    // --- 1OP & 0OP Opcodes (Short Form) ---
    else if (opcode < 0xC0) {
        uint8_t type = (opcode >> 4) & 0x03;
        if (type != 3) { // 1OP
            uint16_t val = (type == 0) ? z_fetch_word() : (type == 1) ? z_fetch_byte() : get_variable(z_fetch_byte());
            switch (opcode & 0x0F) {
                case 0x00: handle_branch(val == 0); break; // JZ
                case 0x05: handle_store(val + 1); break; // INC
                case 0x06: handle_store(val - 1); break; // DEC
                case 0x0B: return_from_routine(val); break; // RET
                case 0x0C: z_machine_pc = val; break; // JUMP
            }
        } else { // 0OP
            switch (opcode & 0x0F) {
                case 0x00: return_from_routine(1); break; // RTRUE
                case 0x01: return_from_routine(0); break; // RFALSE
                case 0x02: decode_zstring_at_pc(); break; // PRINT
                case 0x08: return_from_routine(pop_stack()); break; // RET_POPPED
                case 0x09: pop_stack(); break; // POP
            }
        }
    }
    // --- VAR Opcodes (Variable Form) ---
    else {
        // Opcode 181/182: SAVE and RESTORE
        if ((opcode & 0x1F) == 0x15) { z_save_game(); handle_branch(1); }
        if ((opcode & 0x1F) == 0x16) { z_restore_game(); handle_branch(1); }
        
        // Note: Standard CALL and SREAD logic should go here as well.
    }
}

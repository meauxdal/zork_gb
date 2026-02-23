#include "z_dispatcher.h"
#include "z_memory.h"
#include "z_variable_stack.h"
#include "z_string_decoder.h"
#include "z_vwf_render.h"
#include "z_status_bar.h"

uint32_t z_machine_pc;

void z_dispatcher_init(void) {
    // Header offset 0x06 contains the initial PC (word address)
    uint16_t start_addr = z_read_word(0x06);
    z_machine_pc = (uint32_t)start_addr;
}

void execute_next_instruction(void) {
    uint8_t opcode = z_read_byte(z_machine_pc++);

    // 0OP - Zero Operand Instructions
    if (opcode >= 0xB0 && opcode <= 0xBF) {
        switch (opcode) {
        case 0xB2: // PRINT: Print literal Z-string following opcode
            decode_zstring(z_machine_pc);
            // The decoder doesn't know how long the string was, 
            // we need to skip the PC past the encoded bytes.
            while (!(z_read_word(z_machine_pc) & 0x8000)) {
                z_machine_pc += 2;
            }
            z_machine_pc += 2; // Skip the final word
            break;

        case 0xB1: // RET_TRUE
            return_from_routine(1);
            break;

        case 0xB0: // RTRUE
            return_from_routine(1);
            break;

        case 0xB8: // RET_POPPED
            return_from_routine(pop_stack());
            break;

        case 0xBA: // SHOW_STATUS (V3 Only)
            update_status_bar();
            break;
        }
    }
    // 1OP - One Operand Instructions
    else if (opcode >= 0x80 && opcode <= 0x8F) {
        // Logic for 1OP...
    }
    // VAR - Variable Operand Instructions
    else if (opcode >= 0xE0 && opcode <= 0xFF) {
        switch (opcode) {
        case 0xE0: // CALL
        {
            uint16_t target = get_variable(z_read_byte(z_machine_pc++));
            uint8_t store_var = z_read_byte(z_machine_pc++);
            // Simplified: No arguments for MVP dispatch
            call_routine(target, 0, 0, store_var);
        }
        break;
        }
    }
}

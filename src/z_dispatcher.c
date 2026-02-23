#include "z_dispatcher.h"
#include "z_memory.h"
#include "z_variable_stack.h"
#include "z_string_decoder.h"
#include "z_vwf_render.h"
#include "z_status_bar.h"

uint32_t z_machine_pc;

void z_dispatcher_init(void) {
    uint16_t start_addr = z_read_word(0x06);
    z_machine_pc = (uint32_t)start_addr;
}

void execute_next_instruction(void) {
    uint8_t opcode = z_read_byte(z_machine_pc++);

    // 0OP Instructions
    if (opcode >= 0xB0 && opcode <= 0xBF) {
        switch (opcode) {
        case 0xB2: // PRINT
            decode_zstring(z_machine_pc);
            while (!(z_read_word(z_machine_pc) & 0x8000)) {
                z_machine_pc += 2;
            }
            z_machine_pc += 2;
            break;
        case 0xB1: return_from_routine(1); break;
        case 0xB0: return_from_routine(1); break;
        case 0xB8: return_from_routine(pop_stack()); break;
        case 0xBA: update_status_bar(); break;
        }
    }
    // 1OP Instructions
    else if (opcode >= 0x80 && opcode <= 0x8F) {
        // Implement 1OP logic here
    }
    // VAR Instructions - Removed redundant 'opcode <= 0xFF'
    else if (opcode >= 0xE0) {
        switch (opcode) {
        case 0xE0: // CALL
        {
            uint16_t target = get_variable(z_read_byte(z_machine_pc++));
            uint8_t store_var = z_read_byte(z_machine_pc++);
            call_routine(target, 0, 0, store_var);
        }
        break;
        }
    }
}

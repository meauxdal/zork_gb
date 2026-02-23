#include "z_dispatcher.h"
#include "z_memory.h"
#include "z_variable_stack.h"
#include "z_vwf_render.h"
#include "z_status_bar.h"
#include "workboy.h"

uint32_t z_machine_pc;

/**
 * Handle Branching logic for 2OP and 1OP instructions.
 */
void handle_branch(uint8_t condition) {
    uint8_t b1 = z_read_byte(z_machine_pc++);
    uint8_t branch_on_true = (b1 & 0x80) >> 7;
    uint16_t offset;

    if (b1 & 0x40) {
        offset = b1 & 0x3F;
    }
    else {
        uint8_t b2 = z_read_byte(z_machine_pc++);
        offset = ((uint16_t)(b1 & 0x3F) << 8) | b2;
        if (offset & 0x2000) offset |= 0xC000;
    }

    if (condition == (branch_on_true != 0)) {
        if (offset == 0) return_from_routine(0);
        else if (offset == 1) return_from_routine(1);
        else z_machine_pc = (uint32_t)((int32_t)z_machine_pc + (int16_t)offset - 2);
    }
}

/**
 * SREAD Implementation: Bridge between Game Boy hardware and Z-Machine buffers.
 */
void op_sread(uint16_t text_buf, uint16_t parse_buf) {
    uint8_t max_chars = z_read_byte(text_buf);
    uint8_t char_count = 0;
    char input_char;

    while (char_count < max_chars - 1) {
        input_char = workboy_get_char();
        if (input_char == 0) { wait_vbl_done(); continue; }
        if (input_char == 0x0D) break; // Enter

        if (input_char == 0x08 && char_count > 0) { // Backspace
            char_count--;
            vwf_put_char(input_char);
            continue;
        }

        vwf_put_char(input_char);
        if (input_char >= 'A' && input_char <= 'Z') input_char += 32;
        z_write_byte(text_buf + 1 + char_count, (uint8_t)input_char);
        char_count++;
    }
    z_write_byte(text_buf + 1 + char_count, 0);
    // Note: z_tokenize(text_buf, parse_buf) should be called here once implemented.
}

void execute_next_instruction(void) {
    uint8_t opcode = z_read_byte(z_machine_pc++);

    // 2OP Instructions (Long Form)
    if (opcode >= 0x01 && opcode <= 0x1F) {
        uint16_t op1 = get_variable(z_read_byte(z_machine_pc++));
        uint16_t op2 = get_variable(z_read_byte(z_machine_pc++));
        switch (opcode) {
        case 0x01: handle_branch(op1 == op2); break;
        case 0x14: set_variable(z_read_byte(z_machine_pc++), (int16_t)op1 + (int16_t)op2); break;
        case 0x15: set_variable(z_read_byte(z_machine_pc++), (int16_t)op1 - (int16_t)op2); break;
        }
    }
    // 0OP Instructions
    else if (opcode >= 0xB0 && opcode <= 0xBF) {
        switch (opcode) {
        case 0xB0: return_from_routine(1); break;
        case 0xB1: return_from_routine(0); break;
        case 0xB2: // PRINT
            decode_zstring(z_machine_pc);
            while (!(z_read_word(z_machine_pc) & 0x8000)) z_machine_pc += 2;
            z_machine_pc += 2;
            break;
        case 0xBA: update_status_bar(); break;
        }
    }
    // VAR Instructions
    else if (opcode >= 0xE0) {
        switch (opcode) {
        case 0xE0: // CALL
        {
            uint16_t addr = get_variable(z_read_byte(z_machine_pc++));
            uint8_t store = z_read_byte(z_machine_pc++);
            call_routine(addr, 0, 0, store);
        }
        break;
        case 0xE4: // SREAD
        {
            uint16_t t_buf = get_variable(z_read_byte(z_machine_pc++));
            uint16_t p_buf = get_variable(z_read_byte(z_machine_pc++));
            op_sread(t_buf, p_buf);
        }
        break;
        }
    }
}

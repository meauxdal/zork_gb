#include <gb/gb.h>
#include <stdint.h>
#include "z_dispatcher.h"
#include "z_memory.h"
#include "z_variable_stack.h"
#include "z_vwf_render.h"
#include "z_status_bar.h"
#include "z_string_decoder.h"
#include "workboy.h"

uint32_t z_machine_pc;

void z_dispatcher_init(void) {
    /* The Z-Machine Version 3 start PC is stored as a 16-bit word at header offset 0x06 */
    z_machine_pc = (uint32_t)z_read_word(0x06);
}

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

void op_sread(uint16_t text_buf, uint16_t parse_buf) {
    (void)parse_buf;
    uint8_t max_chars = z_read_byte(text_buf);
    uint8_t char_count = 0;
    char input_char;

    while (char_count < max_chars - 1) {
        input_char = workboy_get_char();
        if (input_char == 0) {
            wait_vbl_done();
            continue;
        }
        if (input_char == 0x0D) break;

        if (input_char == 0x08 && char_count > 0) {
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
}

void execute_next_instruction(void) {
    uint8_t opcode = z_read_byte(z_machine_pc++);

    if (opcode >= 0x01 && opcode <= 0x1F) {
        uint16_t op1 = get_variable(z_read_byte(z_machine_pc++));
        uint16_t op2 = get_variable(z_read_byte(z_machine_pc++));
        switch (opcode) {
        case 0x01: handle_branch(op1 == op2); break;
        case 0x14: set_variable(z_read_byte(z_machine_pc++), (int16_t)op1 + (int16_t)op2); break;
        case 0x15: set_variable(z_read_byte(z_machine_pc++), (int16_t)op1 - (int16_t)op2); break;
        }
        return;
    }
    if ((opcode & 0xF0) == 0xB0) {
        switch (opcode) {

        case 0xB0: /* RTRUE */
            return_from_routine(1);
            return;

        case 0xB1: /* RFALSE */
            return_from_routine(0);
            return;

        case 0xB2: /* PRINT */
            z_machine_pc = decode_zstring(z_machine_pc);
            return;

        case 0xB3: /* PRINT_RET */
            z_machine_pc = decode_zstring(z_machine_pc);
            return_from_routine(1);
            return;

        case 0xBB: /* NEW_LINE */
            vwf_put_char('\n');
            return;

        case 0xBC: /* SHOW_STATUS */
            update_status_bar();
            return;

        case 0xBA: /* QUIT */
            while (1) { wait_vbl_done(); }
            return;

        default:
            return;
        }
    }
    else if (opcode >= 0xE0) {
        switch (opcode) {
        case 0xE0: { /* CALL (VAR) */
            uint8_t types = z_fetch_byte();

            uint16_t ops[4];
            uint8_t opcount = 0;

            for (uint8_t i = 0; i < 4; i++) {
                uint8_t type = (types >> (6 - 2 * i)) & 0x03;
                if (type == 3) break; /* omitted */

                if (type == 0) { /* large const */
                    ops[opcount++] = z_fetch_word();
                }
                else if (type == 1) { /* small const */
                    ops[opcount++] = z_fetch_byte();
                }
                else { /* variable */
                    uint8_t var = z_fetch_byte();
                    ops[opcount++] = z_read_variable(var);
                }
            }

            uint8_t store_var = z_fetch_byte();

            if (opcount == 0) return;

            uint16_t routine = ops[0];
            call_routine(routine, &ops[1], opcount - 1, store_var);
            return;
        }
        case 0xE4: {
            uint16_t t_buf = get_variable(z_read_byte(z_machine_pc++));
            uint16_t p_buf = get_variable(z_read_byte(z_machine_pc++));
            op_sread(t_buf, p_buf);
            return;
        }
    }
}

/*
 * z_dispatcher.c
 * Purpose: Central Instruction Dispatcher and Input Processor
 * Platform: Game Boy (MiSTer / Workboy Support)
 */

#include <stdint.h>
#include <string.h>
#include "memory_core.h"
#include "z_variable_stack.h"
#include "z_string_decoder.h"
#include "z_object_engine.h"
#include "vwf_render.h"
#include "workboy.h"
#include "z_dispatcher.h"
#include "z_status_bar.h"

/* --- Helper: Operand Fetching --- */

static uint16_t read_operand(uint8_t type) {
    switch (type) {
        case 0: return z_fetch_word();               // Large constant (2 bytes)
        case 1: return z_fetch_byte();               // Small constant (1 byte)
        case 2: return get_variable(z_fetch_byte()); // Variable reference
        default: return 0;
    }
}

/* --- Helper: Dictionary Tokenization & Binary Search --- */

static void do_sread(uint16_t text_addr, uint16_t parse_addr) {
    uint8_t max_chars = z_read_byte(text_addr);
    uint8_t buf[128];
    uint8_t len;

    vwf_put_char('\n');
    vwf_put_char('>');
    vwf_put_char(' ');

    // Block for Workboy Keyboard Input
    len = workboy_read_line(buf, (max_chars < 127) ? max_chars : 127);
    z_write_byte(text_addr + 1, len);

    for (uint8_t i = 0; i < len; i++) {
        uint8_t ch = buf[i];
        if (ch >= 'A' && ch <= 'Z') ch += 32; // Lowercase for dictionary matching
        z_write_byte(text_addr + 2 + i, ch);
    }
    z_write_byte(text_addr + 2 + len, 0);

    if (parse_addr == 0) return;

    // Dictionary Search Parameters
    uint16_t dict_addr   = z_read_word(0x08);
    uint8_t  sep_count   = z_read_byte(dict_addr);
    uint32_t entry_start = dict_addr + 1 + sep_count;
    uint8_t  entry_size  = z_read_byte(entry_start);
    uint16_t entry_count = z_read_word(entry_start + 1);
    uint32_t entries     = entry_start + 3;

    uint8_t word_count = 0;
    uint8_t max_words  = z_read_byte(parse_addr);
    uint8_t i = 0;

    while (i < len && word_count < max_words) {
        if (buf[i] == ' ') { i++; continue; }
        
        uint8_t word_start = i;
        while (i < len && buf[i] != ' ') i++;
        uint8_t word_len = i - word_start;

        // Z-Machine V3: Encode first 6 chars into two 16-bit words (4 bytes)
        uint8_t zchars[6] = {5,5,5,5,5,5};
        for (uint8_t w = 0; w < word_len && w < 6; w++) {
            zchars[w] = (buf[word_start + w] >= 'a' && buf[word_start + w] <= 'z') 
                        ? (buf[word_start + w] - 'a' + 6) : 5;
        }

        uint8_t enc[4];
        enc[0] = (zchars[0] << 2) | (zchars[1] >> 3);
        enc[1] = ((zchars[1] & 7) << 5) | zchars[2];
        enc[2] = (zchars[3] << 2) | (zchars[4] >> 3) | 0x80; // End bit set
        enc[3] = ((zchars[4] & 7) << 5) | zchars[5];

        // Binary Search
        uint16_t found = 0;
        int16_t lo = 0, hi = (int16_t)entry_count - 1;
        while (lo <= hi) {
            int16_t mid = lo + (hi - lo) / 2;
            uint32_t mid_addr = entries + (uint32_t)mid * entry_size;
            int8_t cmp = 0;
            for (uint8_t b = 0; b < 4; b++) {
                uint8_t d = z_read_byte(mid_addr + b);
                if (d < enc[b]) { cmp = -1; break; }
                if (d > enc[b]) { cmp = 1; break; }
            }
            if (cmp == 0) { found = (uint16_t)mid_addr; break; }
            if (cmp < 0) lo = mid + 1; else hi = mid - 1;
        }

        uint32_t p_entry = parse_addr + 2 + (uint32_t)word_count * 4;
        z_write_word(p_entry, found);
        z_write_byte(p_entry + 2, word_len);
        z_write_byte(p_entry + 3, word_start + 2);
        word_count++;
    }
    z_write_byte(parse_addr + 1, word_count);
}

/* --- Main Logic: The Dispatcher --- */

void execute_next_instruction(void) {
    uint8_t opcode = z_fetch_byte();
    uint16_t operands[4];
    uint8_t count = 0;

    // --- LONG FORM (2OP) ---
    if (opcode < 0x80) {
        operands[0] = read_operand((opcode & 0x40) ? 2 : 1);
        operands[1] = read_operand((opcode & 0x20) ? 2 : 1);
        
        switch (opcode & 0x1F) {
            case 0x01: handle_branch(operands[0] == operands[1]); break;
            case 0x06: handle_branch(get_object_parent((uint8_t)operands[0]) == (uint8_t)operands[1]); break;
            case 0x0A: handle_branch(get_object_attr((uint8_t)operands[0], (uint8_t)operands[1])); break;
            case 0x0E: insert_object((uint8_t)operands[0], (uint8_t)operands[1]); break;
            case 0x14: handle_store((uint16_t)((int16_t)operands[0] + (int16_t)operands[1])); break;
            case 0x15: handle_store((uint16_t)((int16_t)operands[0] - (int16_t)operands[1])); break;
        }
    } 

    // --- SHORT FORM (1OP or 0OP) ---
    else if (opcode < 0xC0) {
        uint8_t type = (opcode >> 4) & 0x03;
        if (type != 3) { // 1OP
            operands[0] = read_operand(type);
            switch (opcode & 0x0F) {
                case 0x00: handle_branch(operands[0] == 0); break;
                case 0x01: {
                    uint8_t sib = get_object_sibling((uint8_t)operands[0]);
                    handle_store(sib);
                    handle_branch(sib != 0);
                    break;
                }
                case 0x02: {
                    uint8_t child = get_object_child((uint8_t)operands[0]);
                    handle_store(child);
                    handle_branch(child != 0);
                    break;
                }
                case 0x03: handle_store(get_object_parent((uint8_t)operands[0])); break;
                case 0x0B: return_from_routine(operands[0]); break;
            }
        } else { // 0OP
            switch (opcode & 0x0F) {
                case 0x00: return_from_routine(1); break;
                case 0x01: return_from_routine(0); break;
                case 0x02: 
                    decode_zstring(z_machine_pc); 
                    z_machine_pc = zstring_end_addr(z_machine_pc); 
                    break;
                case 0x08: return_from_routine(get_variable(0)); break;
            }
        }
    }

    // --- VARIABLE FORM ---
    else {
        uint8_t types = z_fetch_byte();
        for (uint8_t i = 0; i < 4; i++) {
            uint8_t t = (types >> (6 - (i * 2))) & 0x03;
            if (t == 3) break;
            operands[count++] = read_operand(t);
        }

        switch (opcode & 0x1F) {
            case 0x00: // CALL
                call_routine(operands[0], &operands[1], count - 1, z_fetch_byte(), 0);
                break;
            case 0x01: // STORE_PROP
                put_prop_value((uint8_t)operands[0], (uint8_t)operands[1], operands[2]);
                break;
            case 0x04: // SREAD
                update_status_bar(); // Refresh UI before the Game Boy blocks for user input
                do_sread(operands[0], operands[1]);
                break;
        }
    }
}

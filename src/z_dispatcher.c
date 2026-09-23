/*
 * z_dispatcher.c
 *
 * Z-Machine Version 3 instruction dispatcher.
 *
 * Opcode form detection (spec §4.3):
 *   0x00..0x1F : 2OP, both operands are variables (LONG form, var/var)
 *   0x20..0x3F : 2OP, op1 small const, op2 variable
 *   0x40..0x5F : 2OP, op1 variable, op2 small const
 *   0x60..0x7F : 2OP, both small consts
 *   0x80..0x8F : 1OP, large const
 *   0x90..0x9F : 1OP, small const
 *   0xA0..0xAF : 1OP, variable
 *   0xB0..0xBF : 0OP
 *   0xC0..0xDF : 2OP VAR (operand types in following byte)
 *   0xE0..0xFF : VAR (operand types in following byte)
 *
 * We implement what Zork I actually exercises on its boot path plus
 * a handful more to avoid silent PC corruption on unknown opcodes.
 */

#include <gb/gb.h>
#include <stdint.h>
#include "z_dispatcher.h"
#include "z_memory.h"
#include "z_variable_stack.h"
#include "z_render.h"
#include "z_status_bar.h"
#include "z_string_decoder.h"
#include "z_object_engine.h"
#include "workboy.h"

/* -----------------------------------------------------------------------
 * Global PC
 * ----------------------------------------------------------------------- */
uint32_t z_machine_pc;

/* -----------------------------------------------------------------------
 * Fetch helpers — advance PC and return the consumed byte/word
 * ----------------------------------------------------------------------- */
#define FETCH_BYTE() (z_read_byte(z_machine_pc++))
#define FETCH_WORD() (z_machine_pc += 2u, z_read_word(z_machine_pc - 2u))

/* -----------------------------------------------------------------------
 * Read a VAR-form operand type byte, fill ops[], return operand count.
 * type bits: 00=large const 01=small const 10=variable 11=omitted
 * ----------------------------------------------------------------------- */
static uint8_t fetch_var_operands(uint16_t ops[4]) {
    uint8_t types = FETCH_BYTE();
    uint8_t count = 0;
    uint8_t i;
    for (i = 0; i < 4u; i++) {
        uint8_t t = (types >> (6u - 2u * i)) & 0x03u;
        if (t == 3u) break;
        if      (t == 0u) ops[count++] = FETCH_WORD();
        else if (t == 1u) ops[count++] = FETCH_BYTE();
        else              ops[count++] = get_variable(FETCH_BYTE());
    }
    return count;
}

/* -----------------------------------------------------------------------
 * Branch handler (spec §4.7)
 * condition: 1 if the branch condition is true.
 * ----------------------------------------------------------------------- */
static void handle_branch(uint8_t condition) {
    uint8_t b1          = FETCH_BYTE();
    uint8_t branch_true = (b1 & 0x80u) ? 1u : 0u;
    int16_t offset;

    if (b1 & 0x40u) {
        /* Short form: 6-bit unsigned offset in b1 */
        offset = (int16_t)(b1 & 0x3Fu);
    } else {
        /* Long form: 14-bit signed offset across b1 and b2 */
        uint8_t  b2  = FETCH_BYTE();
        uint16_t raw = ((uint16_t)(b1 & 0x3Fu) << 8) | b2;
        /* Sign-extend from 14 bits */
        if (raw & 0x2000u) raw |= 0xC000u;
        offset = (int16_t)raw;
    }

    if (condition == branch_true) {
        if      (offset == 0) return_from_routine(0u);
        else if (offset == 1) return_from_routine(1u);
        else z_machine_pc = (uint32_t)((int32_t)z_machine_pc + offset - 2);
    }
}

/* -----------------------------------------------------------------------
 * Dictionary tokenization (lexical analysis) for sread / parse
 * ----------------------------------------------------------------------- */

static uint8_t char_to_zchar(char c, uint8_t *out) {
    if (c >= 'a' && c <= 'z') {
        out[0] = (uint8_t)(c - 'a' + 6);
        return 1u;
    }
    if (c >= 'A' && c <= 'Z') {
        out[0] = 4u; /* shift to A1 */
        out[1] = (uint8_t)(c - 'A' + 6);
        return 2u;
    }
    if (c >= '0' && c <= '9') {
        out[0] = 5u; /* shift to A2 */
        out[1] = (uint8_t)(c - '0' + 8);
        return 2u;
    }
    /* Punctuation / special characters in A2 */
    static const char a2_chars[] = " \n0123456789.,!?_#'\"/\\-:()";
    uint8_t idx;
    for (idx = 0u; idx < 25u; idx++) {
        if (a2_chars[idx] == c) {
            out[0] = 5u; /* shift to A2 */
            out[1] = (uint8_t)(idx + 6u);
            return 2u;
        }
    }
    /* Unknown char -> pad value 5 */
    out[0] = 5u;
    out[1] = 5u;
    return 2u;
}

static uint16_t search_dictionary(uint16_t dict_addr, uint16_t w1, uint16_t w2) {
    uint8_t num_sep = z_read_byte(dict_addr);
    uint8_t entry_len = z_read_byte(dict_addr + 1u + num_sep);
    uint16_t num_entries = z_read_word(dict_addr + 2u + num_sep);
    uint16_t entries_start = dict_addr + 4u + num_sep;

    int16_t low = 0;
    int16_t high = (int16_t)num_entries - 1;
    uint32_t target_key = ((uint32_t)w1 << 16) | w2;

    while (low <= high) {
        int16_t mid = (low + high) / 2;
        uint16_t eaddr = entries_start + (uint16_t)mid * entry_len;
        uint16_t ew1 = z_read_word(eaddr);
        uint16_t ew2 = z_read_word(eaddr + 2u);
        uint32_t ekey = ((uint32_t)ew1 << 16) | ew2;

        if (ekey == target_key) return eaddr;
        if (ekey < target_key) low = mid + 1;
        else high = mid - 1;
    }
    return 0u;
}

void z_tokenize(uint16_t text_buf, uint16_t parse_buf) {
    if (parse_buf == 0u) return;

    uint16_t dict_addr = z_read_word(0x08u);
    if (dict_addr == 0u) return;

    uint8_t num_sep = z_read_byte(dict_addr);
    uint8_t max_words = z_read_byte(parse_buf);
    uint8_t tok_count = 0u;
    uint16_t ptr = 1u; /* 1-based index in text_buf */

    while (tok_count < max_words) {
        /* Skip leading spaces */
        char c = (char)z_read_byte(text_buf + ptr);
        if (c == '\0') break;

        if (c == ' ') {
            ptr++;
            continue;
        }

        /* Check if c is a separator */
        uint8_t is_sep = 0u;
        uint8_t i;
        for (i = 0u; i < num_sep; i++) {
            if ((char)z_read_byte(dict_addr + 1u + i) == c) {
                is_sep = 1u;
                break;
            }
        }

        uint16_t word_start = ptr;
        uint8_t word_len = 0u;

        if (is_sep) {
            word_len = 1u;
            ptr++;
        } else {
            /* Read until space, separator, or null */
            while (1) {
                char ch = (char)z_read_byte(text_buf + ptr);
                if (ch == '\0' || ch == ' ') break;

                uint8_t sep = 0u;
                for (i = 0u; i < num_sep; i++) {
                    if ((char)z_read_byte(dict_addr + 1u + i) == ch) {
                        sep = 1u;
                        break;
                    }
                }
                if (sep) break;

                word_len++;
                ptr++;
            }
        }

        if (word_len == 0u) continue;

        /* Encode up to 6 Z-chars */
        uint8_t zchars[12]; /* max possible from 6 chars * 2 */
        uint8_t zcount = 0u;
        for (i = 0u; i < word_len && zcount < 6u; i++) {
            char ch = (char)z_read_byte(text_buf + word_start + i);
            uint8_t tmp[2];
            uint8_t n = char_to_zchar(ch, tmp);
            if (zcount < 6u) zchars[zcount++] = tmp[0];
            if (n > 1u && zcount < 6u) zchars[zcount++] = tmp[1];
        }
        while (zcount < 6u) {
            zchars[zcount++] = 5u; /* pad with 5 */
        }

        uint16_t w1 = ((uint16_t)zchars[0] << 10) | ((uint16_t)zchars[1] << 5) | (uint16_t)zchars[2];
        uint16_t w2 = 0x8000u | ((uint16_t)zchars[3] << 10) | ((uint16_t)zchars[4] << 5) | (uint16_t)zchars[5];

        uint16_t match_addr = search_dictionary(dict_addr, w1, w2);

        /* Write to parse_buf: 4 bytes per word entry starting at parse_buf + 2 */
        uint16_t entry_addr = parse_buf + 2u + (uint16_t)tok_count * 4u;
        z_write_word(entry_addr, match_addr);
        z_write_byte(entry_addr + 2u, word_len);
        z_write_byte(entry_addr + 3u, (uint8_t)word_start);

        tok_count++;
    }

    /* Store total number of parsed words at parse_buf + 1 */
    z_write_byte(parse_buf + 1u, tok_count);
}

/* -----------------------------------------------------------------------
 * sread (opcode 0xE4) — blocking input from Workboy
 *
 * text_buf layout: [max_chars][char0][char1]...[0x00 terminator]
 * parse_buf: lexical analysis
 * ----------------------------------------------------------------------- */
static void op_sread(uint16_t text_buf, uint16_t parse_buf) {
    uint8_t max_chars = z_read_byte(text_buf);
    uint8_t count     = 0;
    char    c;

    while (count < max_chars - 1u) {
        c = workboy_get_char();
        if (c == 0) {
            wait_vbl_done();
            continue;
        }

        if (c == '\r' || c == '\n') break;

        if (c == '\b' && count > 0u) {
            count--;
            z_render_put_char('\b');
            continue;
        }

        z_render_put_char(c);
        /* Z-machine expects lowercase in the text buffer */
        if (c >= 'A' && c <= 'Z') c += 32;
        z_write_byte(text_buf + 1u + count, (uint8_t)c);
        count++;
    }
    z_write_byte(text_buf + 1u + count, 0u);

    if (parse_buf) {
        z_tokenize(text_buf, parse_buf);
    }
}

/* -----------------------------------------------------------------------
 * Object property helpers (used by GET_PROP etc.)
 * ----------------------------------------------------------------------- */

/* Returns address of property data for obj/prop, or 0 if not found.
 * On return, *size_out holds the property data size in bytes. */
static uint16_t find_property(uint8_t obj, uint8_t prop, uint8_t *size_out) {
    uint16_t addr = get_object_address(obj);
    if (addr == 0u) return 0u;

    /* Properties pointer is at offset 7 in the 9-byte object entry */
    uint16_t pptr = z_read_word(addr + 7u);

    /* Skip object name: first byte is name length in words */
    uint8_t name_len = z_read_byte(pptr);
    pptr += 1u + (uint16_t)name_len * 2u;

    /* Walk property list */
    while (1) {
        uint8_t sz_byte = z_read_byte(pptr);
        if (sz_byte == 0u) break; /* end of list */
        uint8_t p_num  = sz_byte & 0x1Fu;
        uint8_t p_size = (sz_byte >> 5u) + 1u;
        pptr++;
        if (p_num == prop) {
            *size_out = p_size;
            return pptr;
        }
        pptr += p_size;
    }
    return 0u;
}

/* Default property value from table at start of object table */
static uint16_t default_property(uint8_t prop) {
    uint16_t base = z_read_word(0x0Au); /* object table address */
    /* Default props are 31 words before the object entries */
    return z_read_word(base + (uint16_t)(prop - 1u) * 2u);
}

/* -----------------------------------------------------------------------
 * Main dispatch
 * ----------------------------------------------------------------------- */
void z_dispatcher_init(void) {
    /* V3 start PC is a 16-bit word address at header offset 0x06 */
    z_machine_pc = (uint32_t)z_read_word(0x06u);
}

void execute_next_instruction(void) {
    uint8_t  opcode = FETCH_BYTE();
    uint16_t op1, op2;
    uint8_t  var;

    /* ===== LONG form: 2OP 0x00..0x7F ===== */
    if (opcode < 0x80u) {
        /* Bit 6: op1 type (0=small const, 1=variable)
         * Bit 5: op2 type (0=small const, 1=variable) */
        uint8_t raw1 = FETCH_BYTE();
        uint8_t raw2 = FETCH_BYTE();
        op1 = (opcode & 0x40u) ? get_variable(raw1) : (uint16_t)raw1;
        op2 = (opcode & 0x20u) ? get_variable(raw2) : (uint16_t)raw2;
        uint8_t base_op = opcode & 0x1Fu; /* strip type bits */

        switch (base_op) {
        case 0x01: /* JE: branch if op1 == op2 */
            handle_branch(op1 == op2);
            break;
        case 0x02: /* JL: branch if op1 < op2 (signed) */
            handle_branch((int16_t)op1 < (int16_t)op2);
            break;
        case 0x03: /* JG: branch if op1 > op2 (signed) */
            handle_branch((int16_t)op1 > (int16_t)op2);
            break;
        case 0x04: /* DEC_CHK: decrement var, branch if < op2 */
            var = (uint8_t)op1;
            {
                int16_t v;
                if (var == 0u) {
                    v = (int16_t)pop_stack() - 1;
                    push_stack((uint16_t)v);
                } else {
                    v = (int16_t)get_variable(var) - 1;
                    set_variable(var, (uint16_t)v);
                }
                handle_branch(v < (int16_t)op2);
            }
            break;
        case 0x05: /* INC_CHK: increment var, branch if > op2 */
            var = (uint8_t)op1;
            {
                int16_t v;
                if (var == 0u) {
                    v = (int16_t)pop_stack() + 1;
                    push_stack((uint16_t)v);
                } else {
                    v = (int16_t)get_variable(var) + 1;
                    set_variable(var, (uint16_t)v);
                }
                handle_branch(v > (int16_t)op2);
            }
            break;
        case 0x06: /* JIN: branch if obj op1 is child of op2 */
            handle_branch(get_parent((uint8_t)op1) == (uint8_t)op2);
            break;
        case 0x07: /* TEST: branch if op1 & op2 == op2 */
            handle_branch((op1 & op2) == op2);
            break;
        case 0x08: /* OR */
            set_variable(FETCH_BYTE(), op1 | op2);
            break;
        case 0x09: /* AND */
            set_variable(FETCH_BYTE(), op1 & op2);
            break;
        case 0x0A: /* TEST_ATTR: branch if object op1 has attr op2 */
            {
                uint16_t addr = get_object_address((uint8_t)op1);
                uint8_t  byte_idx = op2 / 8u;
                uint8_t  bit_mask = 0x80u >> (op2 & 7u);
                uint8_t  attr_byte = (addr && byte_idx < 4u)
                                     ? z_read_byte(addr + byte_idx) : 0u;
                handle_branch(attr_byte & bit_mask);
            }
            break;
        case 0x0B: /* SET_ATTR */
            {
                uint16_t addr = get_object_address((uint8_t)op1);
                if (addr) {
                    uint8_t byte_idx  = op2 / 8u;
                    uint8_t bit_mask  = 0x80u >> (op2 & 7u);
                    z_write_byte(addr + byte_idx,
                                 z_read_byte(addr + byte_idx) | bit_mask);
                }
            }
            break;
        case 0x0C: /* CLEAR_ATTR */
            {
                uint16_t addr = get_object_address((uint8_t)op1);
                if (addr) {
                    uint8_t byte_idx = op2 / 8u;
                    uint8_t bit_mask = 0x80u >> (op2 & 7u);
                    z_write_byte(addr + byte_idx,
                                 z_read_byte(addr + byte_idx) & ~bit_mask);
                }
            }
            break;
        case 0x0D: /* STORE: store op2 into variable op1 */
            if ((uint8_t)op1 == 0u) {
                pop_stack();
                push_stack(op2);
            } else {
                set_variable((uint8_t)op1, op2);
            }
            break;
        case 0x0E: /* INSERT_OBJ: make op1 a child of op2 */
            {
                /* Unlink op1 from current parent first */
                uint8_t obj   = (uint8_t)op1;
                uint8_t dest  = (uint8_t)op2;
                uint8_t par   = get_parent(obj);
                if (par != 0u) {
                    /* Remove from parent's child list */
                    if (get_child(par) == obj) {
                        set_child(par, get_sibling(obj));
                    } else {
                        uint8_t sib = get_child(par);
                        while (sib && get_sibling(sib) != obj)
                            sib = get_sibling(sib);
                        if (sib) set_sibling(sib, get_sibling(obj));
                    }
                }
                /* Link into dest */
                set_sibling(obj, get_child(dest));
                set_child(dest, obj);
                set_parent(obj, dest);
            }
            break;
        case 0x0F: /* LOADW: load word from array */
            set_variable(FETCH_BYTE(), z_read_word(op1 + 2u * op2));
            break;
        case 0x10: /* LOADB: load byte from array */
            set_variable(FETCH_BYTE(), z_read_byte(op1 + op2));
            break;
        case 0x11: /* GET_PROP */
            {
                uint8_t sz;
                uint16_t paddr = find_property((uint8_t)op1, (uint8_t)op2, &sz);
                uint16_t val;
                if (paddr == 0u) {
                    val = default_property((uint8_t)op2);
                } else if (sz == 1u) {
                    val = z_read_byte(paddr);
                } else {
                    val = z_read_word(paddr);
                }
                set_variable(FETCH_BYTE(), val);
            }
            break;
        case 0x12: /* GET_PROP_ADDR */
            {
                uint8_t sz;
                uint16_t paddr = find_property((uint8_t)op1, (uint8_t)op2, &sz);
                set_variable(FETCH_BYTE(), paddr);
            }
            break;
        case 0x13: /* GET_NEXT_PROP */
            {
                uint16_t addr = get_object_address((uint8_t)op1);
                uint16_t pptr = addr ? z_read_word(addr + 7u) : 0u;
                uint8_t  next = 0u;
                if (pptr) {
                    uint8_t name_len = z_read_byte(pptr);
                    pptr += 1u + (uint16_t)name_len * 2u;
                    uint8_t target = (uint8_t)op2;
                    if (target == 0u) {
                        /* Return first property number */
                        uint8_t sb = z_read_byte(pptr);
                        next = sb ? (sb & 0x1Fu) : 0u;
                    } else {
                        /* Skip to target, then return next */
                        while (1) {
                            uint8_t sb = z_read_byte(pptr);
                            if (sb == 0u) break;
                            uint8_t pnum  = sb & 0x1Fu;
                            uint8_t psize = (sb >> 5u) + 1u;
                            pptr += 1u + psize;
                            if (pnum == target) {
                                uint8_t nsb = z_read_byte(pptr);
                                next = nsb ? (nsb & 0x1Fu) : 0u;
                                break;
                            }
                        }
                    }
                }
                set_variable(FETCH_BYTE(), next);
            }
            break;
        case 0x14: /* ADD */
            set_variable(FETCH_BYTE(), (uint16_t)((int16_t)op1 + (int16_t)op2));
            break;
        case 0x15: /* SUB */
            set_variable(FETCH_BYTE(), (uint16_t)((int16_t)op1 - (int16_t)op2));
            break;
        case 0x16: /* MUL */
            set_variable(FETCH_BYTE(), (uint16_t)((int16_t)op1 * (int16_t)op2));
            break;
        case 0x17: /* DIV */
            if (op2 != 0u)
                set_variable(FETCH_BYTE(), (uint16_t)((int16_t)op1 / (int16_t)op2));
            break;
        case 0x18: /* MOD */
            if (op2 != 0u)
                set_variable(FETCH_BYTE(), (uint16_t)((int16_t)op1 % (int16_t)op2));
            break;
        default:
            /* Unknown 2OP: skip store/branch byte if needed — safest is no-op */
            break;
        }
        return;
    }

    /* ===== SHORT form: 1OP 0x80..0xAF and 0OP 0xB0..0xBF ===== */
    if (opcode < 0xC0u) {
        if (opcode >= 0xB0u) {
            /* 0OP */
            switch (opcode) {
            case 0xB0: /* RTRUE */
                return_from_routine(1u);
                break;
            case 0xB1: /* RFALSE */
                return_from_routine(0u);
                break;
            case 0xB2: /* PRINT (inline z-string) */
                z_machine_pc = decode_zstring(z_machine_pc);
                break;
            case 0xB3: /* PRINT_RET */
                z_machine_pc = decode_zstring(z_machine_pc);
                z_render_put_char('\n');
                return_from_routine(1u);
                break;
            case 0xB4: /* NOP */
                break;
            case 0xB5: /* SAVE — save state to battery SRAM */
                handle_branch(z_save_state());
                break;
            case 0xB6: /* RESTORE — restore state from battery SRAM */
                handle_branch(z_restore_state());
                break;
            case 0xB7: /* RESTART — reinit and jump to start */
                z_dispatcher_init();
                break;
            case 0xB8: /* RET_POPPED */
                return_from_routine(pop_stack());
                break;
            case 0xB9: /* POP */
                pop_stack();
                break;
            case 0xBA: /* QUIT */
                while (1) wait_vbl_done();
                break;
            case 0xBB: /* NEW_LINE */
                z_render_put_char('\n');
                break;
            case 0xBC: /* SHOW_STATUS */
                update_status_bar();
                break;
            case 0xBD: /* VERIFY — always succeed, branch true */
                handle_branch(1u);
                break;
            default:
                break;
            }
            return;
        }

        /* 1OP: bits 5:4 of opcode encode operand type */
        {
            uint8_t op_type = (opcode >> 4u) & 0x03u;
            if      (op_type == 0u) op1 = FETCH_WORD();          /* large const */
            else if (op_type == 1u) op1 = FETCH_BYTE();          /* small const */
            else                    op1 = get_variable(FETCH_BYTE()); /* variable */

            uint8_t base_op = opcode & 0x0Fu;
            switch (base_op) {
            case 0x00: /* JZ: branch if op1 == 0 */
                handle_branch(op1 == 0u);
                break;
            case 0x01: /* GET_SIBLING */
                {
                    uint8_t sib = get_sibling((uint8_t)op1);
                    set_variable(FETCH_BYTE(), sib);
                    handle_branch(sib != 0u);
                }
                break;
            case 0x02: /* GET_CHILD */
                {
                    uint8_t ch = get_child((uint8_t)op1);
                    set_variable(FETCH_BYTE(), ch);
                    handle_branch(ch != 0u);
                }
                break;
            case 0x03: /* GET_PARENT */
                set_variable(FETCH_BYTE(), get_parent((uint8_t)op1));
                break;
            case 0x04: /* GET_PROP_LEN */
                {
                    /* Address passed is the data address; size byte is one before */
                    uint16_t paddr = op1;
                    uint8_t sz = 0u;
                    if (paddr > 0u) {
                        uint8_t sz_byte = z_read_byte(paddr - 1u);
                        sz = (sz_byte >> 5u) + 1u;
                    }
                    set_variable(FETCH_BYTE(), sz);
                }
                break;
            case 0x05: /* INC */
                var = (uint8_t)op1;
                if (var == 0u) {
                    uint16_t v = pop_stack() + 1u;
                    push_stack(v);
                } else {
                    set_variable(var, (uint16_t)((int16_t)get_variable(var) + 1));
                }
                break;
            case 0x06: /* DEC */
                var = (uint8_t)op1;
                if (var == 0u) {
                    uint16_t v = pop_stack() - 1u;
                    push_stack(v);
                } else {
                    set_variable(var, (uint16_t)((int16_t)get_variable(var) - 1));
                }
                break;
            case 0x07: /* PRINT_ADDR: print z-string at byte address */
                decode_zstring((uint32_t)op1);
                break;
            case 0x09: /* REMOVE_OBJ */
                {
                    uint8_t obj = (uint8_t)op1;
                    uint8_t par = get_parent(obj);
                    if (par != 0u) {
                        if (get_child(par) == obj) {
                            set_child(par, get_sibling(obj));
                        } else {
                            uint8_t sib = get_child(par);
                            while (sib && get_sibling(sib) != obj)
                                sib = get_sibling(sib);
                            if (sib) set_sibling(sib, get_sibling(obj));
                        }
                        set_parent(obj, 0u);
                        set_sibling(obj, 0u);
                    }
                }
                break;
            case 0x0A: /* PRINT_OBJ: print name of object */
                get_object_name((uint8_t)op1);
                break;
            case 0x0B: /* RET */
                return_from_routine(op1);
                break;
            case 0x0C: /* JUMP: unconditional */
                z_machine_pc = (uint32_t)((int32_t)z_machine_pc + (int16_t)op1 - 2);
                break;
            case 0x0D: /* PRINT_PADDR: print z-string at packed address */
                decode_zstring((uint32_t)op1 * 2u);
                break;
            case 0x0E: /* LOAD: load variable */
                {
                    uint8_t v = (uint8_t)op1;
                    uint16_t val = (v == 0u) ? peek_stack() : get_variable(v);
                    set_variable(FETCH_BYTE(), val);
                }
                break;
            case 0x0F: /* NOT */
                set_variable(FETCH_BYTE(), ~op1);
                break;
            default:
                break;
            }
        }
        return;
    }

    /* ===== VAR form: 0xC0..0xFF ===== */
    {
        uint16_t ops[4];
        uint8_t  nops;

        if (opcode < 0xE0u) {
            /* 0xC0..0xDF: 2OP in VAR form (used for CALL with many args) */
            nops = fetch_var_operands(ops);
            uint8_t base_op = opcode & 0x1Fu;
            /* These mirror the 2OP table — most commonly it's just CALL */
            switch (base_op) {
            case 0x01:
                {
                    uint8_t cond = 0u;
                    if (nops >= 2u && ops[0] == ops[1]) cond = 1u;
                    if (nops >= 3u && ops[0] == ops[2]) cond = 1u;
                    if (nops >= 4u && ops[0] == ops[3]) cond = 1u;
                    handle_branch(cond);
                }
                break;
            case 0x14: if (nops >= 2u) set_variable(FETCH_BYTE(), (uint16_t)((int16_t)ops[0] + (int16_t)ops[1])); break;
            case 0x15: if (nops >= 2u) set_variable(FETCH_BYTE(), (uint16_t)((int16_t)ops[0] - (int16_t)ops[1])); break;
            default: break;
            }
            return;
        }

        /* 0xE0..0xFF: VAR opcodes */
        nops = fetch_var_operands(ops);

        switch (opcode) {
        case 0xE0: /* CALL */
            if (nops == 0u) break;
            call_routine(ops[0], &ops[1], nops - 1u, FETCH_BYTE());
            break;
        case 0xE1: /* STOREW: array word store */
            if (nops >= 3u)
                z_write_word(ops[0] + 2u * ops[1], ops[2]);
            break;
        case 0xE2: /* STOREB: array byte store */
            if (nops >= 3u)
                z_write_byte(ops[0] + ops[1], (uint8_t)ops[2]);
            break;
        case 0xE3: /* PUT_PROP */
            if (nops >= 3u) {
                uint8_t sz;
                uint16_t paddr = find_property((uint8_t)ops[0], (uint8_t)ops[1], &sz);
                if (paddr) {
                    if (sz == 1u) z_write_byte(paddr, (uint8_t)ops[2]);
                    else          z_write_word(paddr, ops[2]);
                }
            }
            break;
        case 0xE4: /* SREAD (input) */
            if (nops >= 2u) op_sread(ops[0], ops[1]);
            break;
        case 0xE5: /* PRINT_CHAR */
            if (nops >= 1u) z_render_put_char((char)ops[0]);
            break;
        case 0xE6: /* PRINT_NUM */
            if (nops >= 1u) {
                /* Print signed decimal — hand-roll to avoid printf */
                int16_t n = (int16_t)ops[0];
                char buf[7]; /* -32768\0 */
                uint8_t idx = 6u;
                buf[idx] = '\0';
                uint8_t neg = 0u;
                if (n < 0) { neg = 1u; n = -n; }
                do {
                    buf[--idx] = '0' + (char)(n % 10);
                    n /= 10;
                } while (n);
                if (neg) buf[--idx] = '-';
                {
                    char *p = &buf[idx];
                    while (*p) z_render_put_char(*p++);
                }
            }
            break;
        case 0xE7: /* RANDOM */
            if (nops >= 1u) {
                /* Minimal LCG — good enough for Zork's use */
                static uint16_t rng = 12345u;
                int16_t range = (int16_t)ops[0];
                uint16_t result = 0u;
                if (range > 0) {
                    rng = (uint16_t)(rng * 1103u + 12345u);
                    result = (rng % (uint16_t)range) + 1u;
                } else {
                    rng = (range == 0) ? 12345u : (uint16_t)(-range);
                }
                set_variable(FETCH_BYTE(), result);
            }
            break;
        case 0xE8: /* PUSH */
            if (nops >= 1u) push_stack(ops[0]);
            break;
        case 0xE9: /* PULL */
            if (nops >= 1u) {
                uint8_t v = (uint8_t)ops[0];
                if (v == 0u) {
                    uint8_t dest = (uint8_t)pop_stack();
                    set_variable(dest, pop_stack());
                } else {
                    set_variable(v, pop_stack());
                }
            }
            break;
        case 0xEA: /* SPLIT_WINDOW — no-op on GB */
            break;
        case 0xEB: /* SET_WINDOW — no-op on GB */
            break;
        case 0xF3: /* OUTPUT_STREAM — no-op (we always output to screen) */
            break;
        case 0xF4: /* INPUT_STREAM — no-op */
            break;
        case 0xF5: /* SOUND_EFFECT — no-op */
            break;
        default:
            break;
        }
    }
}

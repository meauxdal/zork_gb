/*
 * tests/test_zork.c
 *
 * Unit test suite for Zork Game Boy implementation.
 * Compiles on host with gcc to test logic and regression safety.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

/* Mock Game Boy hardware / GBDK functions */
uint8_t mock_vram[18][20];
static uint8_t mock_joypad_state = 0;
static uint8_t mock_sb_reg = 0xFF;

void set_bkg_tile_xy(uint8_t x, uint8_t y, uint8_t tile) {
    if (x < 20 && y < 18) {
        mock_vram[y][x] = tile;
    }
}

void wait_vbl_done(void) {}

uint8_t joypad(void) {
    return mock_joypad_state;
}

static uint8_t mock_sb_val = 0xFF;
static uint8_t mock_workboy_rx = 0xFF;

static uint8_t mock_sc_reg = 0;
static void handle_sc_write(uint8_t val) {
    mock_sc_reg = val;
    if (val & 0x80) {
        /* Simulate hardware serial transfer: incoming byte shifted into SB_REG */
        mock_sb_val = mock_workboy_rx;
        mock_sc_reg &= 0x7F; /* Transfer complete */
    }
}

#define SB_REG mock_sb_val
#define SC_REG mock_sc_reg
/* Intercept writes to SC_REG in test code */
#define SC_REG_WRITE(val) handle_sc_write(val)

#define J_A      0x01
#define J_B      0x02
#define J_SELECT 0x04
#define J_START  0x08
#define J_RIGHT  0x10
#define J_LEFT   0x20
#define J_UP     0x40
#define J_DOWN   0x80

#define SWITCH_ROM(b) ((void)(b))

/* Load zork1.z3 binary into memory for testing */
static uint8_t story_data[100000];
static size_t story_len = 0;

/* Mock memory read/write for testing */
uint8_t z_read_byte(uint32_t address) {
    if (address < story_len) {
        return story_data[address];
    }
    return 0;
}

uint16_t z_read_word(uint32_t address) {
    return ((uint16_t)z_read_byte(address) << 8) | z_read_byte(address + 1);
}

void z_write_byte(uint32_t address, uint8_t value) {
    if (address < story_len) {
        story_data[address] = value;
    }
}

void z_write_word(uint32_t address, uint16_t value) {
    z_write_byte(address, (uint8_t)(value >> 8));
    z_write_byte(address + 1, (uint8_t)(value & 0xFF));
}

/* Include source files directly or implement tests against test copies */
#include "../include/z_memory.h"
#include "../include/z_variable_stack.h"
#include "../include/z_dispatcher.h"
#include "../include/z_render.h"
#include "../include/z_status_bar.h"
#include "../include/z_object_engine.h"

/* Mock SRAM for host test suite */
static uint8_t mock_sram[16384];

uint8_t z_save_state(void) {
    uint8_t *ptr = mock_sram;
    memcpy(ptr, "ZORKGB01", 8); ptr += 8;
    memcpy(ptr, &z_machine_pc, sizeof(z_machine_pc)); ptr += sizeof(z_machine_pc);
    memcpy(ptr, story_data, Z_DYNAMIC_SIZE); ptr += Z_DYNAMIC_SIZE;
    uint16_t gbase = z_read_word(0x0C);
    memcpy(ptr, story_data + gbase, Z_GLOBALS_COUNT * 2u); ptr += Z_GLOBALS_COUNT * 2u;
    *ptr++ = sp;
    memcpy(ptr, z_stack, sizeof(z_stack)); ptr += sizeof(z_stack);
    memcpy(ptr, &fp, sizeof(fp)); ptr += sizeof(fp);
    memcpy(ptr, call_stack, sizeof(call_stack)); ptr += sizeof(call_stack);
    return 1u;
}

uint8_t z_restore_state(void) {
    uint8_t *ptr = mock_sram;
    if (memcmp(ptr, "ZORKGB01", 8) != 0) return 0u;
    ptr += 8;
    memcpy(&z_machine_pc, ptr, sizeof(z_machine_pc)); ptr += sizeof(z_machine_pc);
    memcpy(story_data, ptr, Z_DYNAMIC_SIZE); ptr += Z_DYNAMIC_SIZE;
    uint16_t gbase = z_read_word(0x0C);
    memcpy(story_data + gbase, ptr, Z_GLOBALS_COUNT * 2u); ptr += Z_GLOBALS_COUNT * 2u;
    sp = *ptr++;
    memcpy(z_stack, ptr, sizeof(z_stack)); ptr += sizeof(z_stack);
    memcpy(&fp, ptr, sizeof(fp)); ptr += sizeof(fp);
    memcpy(call_stack, ptr, sizeof(call_stack)); ptr += sizeof(call_stack);
    return 1u;
}

/* Include test implementations of stack and renderer */
#include "../src/z_variable_stack.c"
#include "../src/z_render.c"
#include "../src/z_status_bar.c"
#include "../src/z_object_engine.c"
#include "../src/z_string_decoder.c"

/* Workboy implementation */
#include "../src/workboy.c"

#include "../src/z_dispatcher.c"

void test_stack_operations(void) {
    printf("[TEST] Testing stack operations...\n");
    z_stack_init();

    push_stack(42);
    push_stack(100);
    assert(peek_stack() == 100);
    assert(pop_stack() == 100);
    assert(peek_stack() == 42);
    assert(pop_stack() == 42);
    assert(pop_stack() == 0);
    assert(peek_stack() == 0);

    printf("  Stack tests passed!\n");
}

void test_dictionary_tokenization(void) {
    printf("[TEST] Testing dictionary tokenization (z_tokenize)...\n");

    /* Buffer addresses in dynamic RAM area (e.g. 0x0200) */
    uint16_t text_buf = 0x0200;
    uint16_t parse_buf = 0x0250;

    /* Write "open mailbox" into text_buf */
    z_write_byte(text_buf, 30); /* max chars */
    const char *cmd = "open mailbox";
    uint8_t len = strlen(cmd);
    for (uint8_t i = 0; i < len; i++) {
        z_write_byte(text_buf + 1 + i, cmd[i]);
    }
    z_write_byte(text_buf + 1 + len, 0);

    /* Setup parse_buf */
    z_write_byte(parse_buf, 6); /* max 6 words */
    z_write_byte(parse_buf + 1, 0); /* num words */

    z_tokenize(text_buf, parse_buf);

    uint8_t parsed_count = z_read_byte(parse_buf + 1);
    assert(parsed_count == 2);

    /* Token 0: "open" */
    uint16_t addr1 = z_read_word(parse_buf + 2);
    uint8_t len1 = z_read_byte(parse_buf + 4);
    uint8_t off1 = z_read_byte(parse_buf + 5);
    assert(addr1 != 0); /* "open" is in zork1 dictionary */
    assert(len1 == 4);
    assert(off1 == 1);

    /* Token 1: "mailbox" */
    uint16_t addr2 = z_read_word(parse_buf + 6);
    uint8_t len2 = z_read_byte(parse_buf + 8);
    uint8_t off2 = z_read_byte(parse_buf + 9);
    assert(addr2 != 0); /* "mailbox" is in zork1 dictionary */
    assert(len2 == 7);
    assert(off2 == 6);

    printf("  Dictionary tokenization tests passed! Match addr 'open': %04X, 'mailbox': %04X\n", addr1, addr2);
}

void test_screen_scrolling(void) {
    printf("[TEST] Testing screen renderer and scrolling...\n");
    memset(mock_vram, 0, sizeof(mock_vram));
    z_render_init();

    /* Fill text area lines 1..17 */
    for (int line = 1; line <= 20; line++) {
        char msg[20];
        snprintf(msg, sizeof(msg), "Line %d", line);
        for (char *p = msg; *p; p++) {
            z_render_put_char(*p);
        }
        z_render_put_char('\n');
    }

    /* Verify bottom row (row 17, index 16) holds Line 20 */
    char expected[20];
    snprintf(expected, sizeof(expected), "Line 20");
    for (size_t i = 0; i < strlen(expected); i++) {
        assert(mock_vram[16][i] == (uint8_t)expected[i]);
    }

    printf("  Screen renderer and scrolling tests passed!\n");
}

void test_status_bar(void) {
    printf("[TEST] Testing status bar display...\n");
    memset(mock_vram, 0, sizeof(mock_vram));
    z_render_init();

    /* Set room, score, turns globals */
    /* Globals table base address is at header offset 0x0C */
    uint16_t gbase = z_read_word(0x0C);

    /* Global 1 (room object) = 0x11, offset = (0x11 - 0x10) * 2 = 2 */
    /* Set room to object 1 */
    z_write_word(gbase + 2, 1);
    /* Global 2 (score) = 0x12 -> offset 4 -> score = 10 */
    z_write_word(gbase + 4, 10);
    /* Global 3 (turns) = 0x13 -> offset 6 -> turns = 5 */
    z_write_word(gbase + 6, 5);

    update_status_bar();

    /* Verify line 0 of mock_vram */
    char status_line[21];
    for (int x = 0; x < 20; x++) {
        status_line[x] = mock_vram[0][x] ? mock_vram[0][x] : ' ';
    }
    status_line[20] = '\0';
    printf("  Status line output: '%s'\n", status_line);

    /* Check right side contains " S:10 T:5" */
    assert(strstr(status_line, "S:10 T:5") != NULL);

    printf("  Status bar tests passed!\n");
}

void test_2op_var_form_and_array_ops(void) {
    printf("[TEST] Testing 2OP instructions in VAR form and array address wrapping...\n");

    /* Test 2OP in VAR form: ADD (0xC0 | 0x14 = 0xD4) */
    /* Byte sequence: 0xD4 0x5F 0x10 0x20 0x01
     * 0xD4 = 2OP VAR form, opcode 0x14 (ADD)
     * 0x5F = type byte: 01 (small const), 01 (small const), 11 (omitted), 11 (omitted)
     * 0x10 = op1 (small const 0x10)
     * 0x20 = op2 (small const 0x20)
     * 0x01 = result store variable (variable 1)
     */
    uint32_t code_addr = 0x0300;
    z_write_byte(code_addr, 0xD4);
    z_write_byte(code_addr + 1, 0x5F);
    z_write_byte(code_addr + 2, 0x10);
    z_write_byte(code_addr + 3, 0x20);
    z_write_byte(code_addr + 4, 0x01); /* store in local 1 */

    z_machine_pc = code_addr;

    /* Setup call frame so local 1 exists */
    fp = 0;
    call_stack[0].num_locals = 2;
    call_stack[0].locals[0] = 0;

    execute_next_instruction();

    /* Local 1 (var 1) should hold 0x10 + 0x20 = 0x30 = 48 */
    assert(get_variable(1) == 48);

    /* Test STOREW / LOADW address wrapping */
    /* STOREW at array 0xFFFE with word index 2 => 0xFFFE + 4 = 0x10002 -> wrapped to 0x0002 */
    /* Byte sequence for STOREW (0xE1): 0xE1 0x03 0xFF 0xFE 0x00 0x02 0x12 0x34
     * 0xE1 = VAR op 0x01 (STOREW)
     * 0x03 = operand types: large const (00), large const (00), large const (00), omitted (11) -> 0x03
     */
    z_write_byte(code_addr, 0xE1);
    z_write_byte(code_addr + 1, 0x03);
    z_write_word(code_addr + 2, 0xFFFE);
    z_write_word(code_addr + 4, 0x0002);
    z_write_word(code_addr + 6, 0x1234);

    z_machine_pc = code_addr;
    execute_next_instruction();

    /* Check that address 0x0002 in dynamic RAM received 0x1234 */
    assert(z_read_word(0x0002) == 0x1234);

    printf("  2OP VAR form and array address wrapping tests passed!\n");
}

void test_save_restore(void) {
    printf("[TEST] Testing save and restore state...\n");
    memset(mock_sram, 0, sizeof(mock_sram));

    /* Check restore on empty SRAM returns failure (0) */
    assert(z_restore_state() == 0u);

    /* Set up test state */
    z_stack_init();
    push_stack(1234);
    push_stack(5678);

    uint16_t gbase = z_read_word(0x0C);
    z_write_word(gbase + 2, 99); /* Global 1 */
    z_machine_pc = 0x12345;

    /* Save state */
    assert(z_save_state() == 1u);

    /* Mutate state */
    pop_stack();
    push_stack(9999);
    z_write_word(gbase + 2, 1);
    z_machine_pc = 0x54321;

    /* Restore state */
    assert(z_restore_state() == 1u);

    /* Verify state restored */
    assert(z_machine_pc == 0x12345);
    assert(z_read_word(gbase + 2) == 99);
    assert(pop_stack() == 5678);
    assert(pop_stack() == 1234);

    printf("  Save and restore tests passed!\n");
}

void test_print_num(void) {
    printf("[TEST] Testing PRINT_NUM (0xE6) opcode...\n");
    memset(mock_vram, 0, sizeof(mock_vram));
    z_render_init();

    /* Test 1: PRINT_NUM -32768 (0x8000)
     * Opcode sequence: 0xE6 0x3F 0x80 0x00
     * 0xE6 = PRINT_NUM
     * 0x3F = operand types: large const (00), omitted (11), omitted (11), omitted (11) -> 0x3F
     * 0x80 0x00 = large const 0x8000 (-32768)
     */
    uint32_t code_addr = 0x0400;
    z_write_byte(code_addr, 0xE6);
    z_write_byte(code_addr + 1, 0x3F);
    z_write_word(code_addr + 2, 0x8000);

    z_machine_pc = code_addr;
    execute_next_instruction();

    /* Verify output on line 1 in mock_vram */
    char buf[20];
    for (int i = 0; i < 6; i++) {
        buf[i] = (char)mock_vram[1][i];
    }
    buf[6] = '\0';
    assert(strcmp(buf, "-32768") == 0);

    /* Test 2: PRINT_NUM 0
     * Opcode sequence: 0xE6 0x3F 0x00 0x00
     */
    z_render_init();
    z_write_byte(code_addr, 0xE6);
    z_write_byte(code_addr + 1, 0x3F);
    z_write_word(code_addr + 2, 0x0000);

    z_machine_pc = code_addr;
    execute_next_instruction();

    buf[0] = (char)mock_vram[1][0];
    buf[1] = '\0';
    assert(strcmp(buf, "0") == 0);

    /* Test 3: PRINT_NUM 12345
     * Opcode sequence: 0xE6 0x3F 0x30 0x39
     */
    z_render_init();
    z_write_byte(code_addr, 0xE6);
    z_write_byte(code_addr + 1, 0x3F);
    z_write_word(code_addr + 2, 12345);

    z_machine_pc = code_addr;
    execute_next_instruction();

    for (int i = 0; i < 5; i++) {
        buf[i] = (char)mock_vram[1][i];
    }
    buf[5] = '\0';
    assert(strcmp(buf, "12345") == 0);

    printf("  PRINT_NUM tests passed!\n");
}

int main(void) {
    printf("Starting Zork GB unit tests...\n");

    FILE *f = fopen("data/zork1.z3", "rb");
    if (!f) {
        fprintf(stderr, "Error opening data/zork1.z3\n");
        return 1;
    }
    story_len = fread(story_data, 1, sizeof(story_data), f);
    fclose(f);
    printf("Loaded story binary (%zu bytes)\n", story_len);

    test_stack_operations();
    test_dictionary_tokenization();
    test_screen_scrolling();
    test_status_bar();
    test_2op_var_form_and_array_ops();
    test_save_restore();
    test_print_num();

    /* Test serial noise handling and debouncing in workboy_get_char */
    printf("[TEST] Testing Workboy serial link noise handling...\n");
    mock_workboy_rx = 0xFF;
    assert(workboy_get_char() == 0);

    /* 1. Transient noise spike (e.g. 0x0A enter key spike for 1 or 2 polls) must be rejected */
    mock_workboy_rx = 0x0A;
    assert(workboy_get_char() == 0); /* 1st poll */
    assert(workboy_get_char() == 0); /* 2nd poll */
    mock_workboy_rx = 0x00;           /* noise stops / idle */
    assert(workboy_get_char() == 0);

    /* 2. Sustained valid key press ('A' = 0x31) across 3 consecutive polls */
    mock_workboy_rx = 0x31;
    assert(workboy_get_char() == 0); /* Poll 1 */
    assert(workboy_get_char() == 0); /* Poll 2 */
    char fetched = workboy_get_char(); /* Poll 3: confirmed! */
    assert(fetched == 'A');

    /* 3. Key held down does not re-trigger */
    assert(workboy_get_char() == 0); /* Poll 4 */

    /* 4. Release key and press 'Enter' (0x0A) stably */
    mock_workboy_rx = 0xFF; /* release */
    assert(workboy_get_char() == 0);

    mock_workboy_rx = 0x0A; /* Enter key */
    assert(workboy_get_char() == 0); /* Poll 1 */
    assert(workboy_get_char() == 0); /* Poll 2 */
    assert(workboy_get_char() == '\n'); /* Poll 3: confirmed Enter */

    mock_workboy_rx = 0xFF;
    assert(workboy_get_char() == 0);
    printf("  Workboy serial noise tests passed!\n");

    /* Test Game Pad input in op_sread */
    printf("[TEST] Testing Game Pad controls in op_sread...\n");
    uint16_t text_buf = 0x0300;
    uint16_t parse_buf = 0x0350;
    z_write_byte(text_buf, 20);
    z_write_byte(parse_buf, 6);

    /* We test candidate character selection array bounds & logic */
    assert(NUM_CANDIDATES > 0);
    assert(candidate_chars[0] == 'a');

    printf("  Game Pad controls tests passed!\n");

    printf("\nAll unit tests completed successfully!\n");
    return 0;
}

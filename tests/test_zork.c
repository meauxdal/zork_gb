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

void set_bkg_tile_xy(uint8_t x, uint8_t y, uint8_t tile) {
    if (x < 20 && y < 18) {
        mock_vram[y][x] = tile;
    }
}

void wait_vbl_done(void) {}

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
#include "../include/z_variable_stack.h"
#include "../include/z_dispatcher.h"
#include "../include/z_render.h"
#include "../include/z_status_bar.h"
#include "../include/z_object_engine.h"

/* Include test implementations of stack and renderer */
#include "../src/z_variable_stack.c"
#include "../src/z_render.c"
#include "../src/z_status_bar.c"
#include "../src/z_object_engine.c"
#include "../src/z_string_decoder.c"

/* Workboy mock */
char mock_input_char = 0;
char workboy_get_char(void) {
    char c = mock_input_char;
    mock_input_char = 0;
    return c;
}

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

    printf("\nAll unit tests completed successfully!\n");
    return 0;
}

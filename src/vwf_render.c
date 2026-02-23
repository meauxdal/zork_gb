/*
 * vwf_render.c
 * Purpose: MVP Display Driver with Status Bar and Scrolling support.
 * Platform: Game Boy (GBDK)
 */

#include <gb/gb.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "vwf_render.h"
#include "z_string_decoder.h"

static uint8_t cursor_col = 0;
static uint8_t cursor_row = TEXT_FIRST_ROW;
static uint8_t lines_since_clear = 0;

/* Initializes the LCD and draws the Status Bar separator */
void vwf_init_display(void) {
    DISPLAY_OFF;
    BGP_REG = 0xE4; // Standard palette: 3=Black, 0=White
    cls();
    
    // Draw a visual separator line at Row 1
    gotoxy(0, 1);
    for (uint8_t i = 0; i < 20; i++) putchar('-');
    
    cursor_col = 0;
    cursor_row = TEXT_FIRST_ROW;
    DISPLAY_ON;
}

/* Forces the cursor to a specific column in the Status Bar (Row 0) */
void vwf_seek_status(uint8_t col) {
    cursor_col = col;
    cursor_row = 0;
    gotoxy(cursor_col, cursor_row);
}

/* Simple MORE prompt to wait for user button press */
void vwf_more_prompt(void) {
    gotoxy(0, 17);
    printf("-- MORE --");
    waitpad(J_A | J_B | J_START);
    waitpadup();
    
    // Clear the bottom area and reset scroll counter
    vwf_clear_text();
}

/* Clears the main text area but leaves the Status Bar intact */
void vwf_clear_text(void) {
    for (uint8_t r = TEXT_FIRST_ROW; r < 18; r++) {
        gotoxy(0, r);
        printf("                    ");
    }
    cursor_row = TEXT_FIRST_ROW;
    cursor_col = 0;
    lines_since_clear = 0;
}

/* Core printing function with scrolling logic */
void vwf_put_char(uint8_t chr) {
    if (chr == '\n') {
        cursor_col = 0;
        cursor_row++;
        lines_since_clear++;
    } else {
        if (cursor_col >= 20) {
            cursor_col = 0;
            cursor_row++;
            lines_since_clear++;
        }
        
        gotoxy(cursor_col, cursor_row);
        putchar(chr);
        cursor_col++;
    }

    // Trigger MORE prompt if we fill the screen
    if (lines_since_clear >= 14 && cursor_row >= 16) {
        vwf_more_prompt();
    }
}

/* Special helper to render score/moves in the status line */
void vwf_print_status_stats(int16_t val1, int16_t val2) {
    gotoxy(12, 0);
    printf("%d/%d", val1, val2);
}

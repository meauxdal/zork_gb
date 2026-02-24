#include <gb/gb.h>
#include <stdint.h>

/* Workboy Scancode to ASCII Mapping (Simplified for Zork) */
const char workboy_map[] = {
    0, 0, 0, 0, 0, 0, 0, 0,         // 0x00-0x07
    '\b', '\t', '\n', 0, 0, 0, 0, 0, // 0x08-0x0F (Backsp, Tab, Enter)
    ' ', '!', '\"', '#', '$', '%', '&', '\'',
    '(', ')', '*', '+', ',', '-', '.', '/',
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', ':', ';', '<', '=', '>', '?',
    '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
    'X', 'Y', 'Z', '[', '\\', ']', '^', '_'
};

char workboy_get_char(void) {
    uint8_t scancode;

    /* 1. Initiate Serial Transfer */
    /* SB is irrelevant on send for Workboy, but we must trigger the clock */
    SB_REG = 0x00; 
    SC_REG = 0x81; /* Start transfer, Internal Clock */

    /* 2. Wait for transfer completion */
    /* On a real GB, this takes ~1ms. In Z-Machine loop, we poll the SC bit 7. */
    while (SC_REG & 0x80);

    scancode = SB_REG;

    /* 3. Validation */
    if (scancode == 0xFF || scancode > 0x4F) {
        return 0;
    }

    /* 4. Basic Debouncing / Edge Detection */
    /* To prevent a single press from filling the buffer, the dispatcher 
       should only accept this char if the previous poll was 0xFF. */
    static uint8_t last_scan = 0xFF;
    if (scancode == last_scan) return 0;
    
    last_scan = scancode;
    return workboy_map[scancode];
}

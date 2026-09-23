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
#ifdef SC_REG_WRITE
    SC_REG_WRITE(0x81);
#else
    SC_REG = 0x81; /* Start transfer, Internal Clock */
#endif

    /* 2. Wait for transfer completion */
    /* On a real GB, this takes ~1ms. In Z-Machine loop, we poll the SC bit 7. */
    while (SC_REG & 0x80);

    scancode = SB_REG;

    /* 3. Validation & Multi-Sample Debouncing */
    static uint8_t pending_scan = 0xFFu;
    static uint8_t pending_count = 0u;
    static uint8_t last_reported_scan = 0xFFu;

    if (scancode == 0xFFu || scancode >= sizeof(workboy_map) || workboy_map[scancode] == 0) {
        /* Idle / invalid scancode: reset sample counters and key-held state */
        pending_scan = 0xFFu;
        pending_count = 0u;
        last_reported_scan = 0xFFu;
        return 0;
    }

    /* Require 3 consecutive identical valid reads (~3 frames) to reject noise */
    if (scancode == pending_scan) {
        if (pending_count < 255u) pending_count++;
    } else {
        pending_scan = scancode;
        pending_count = 1u;
    }

    if (pending_count >= 3u) {
        if (scancode != last_reported_scan) {
            last_reported_scan = scancode;
            return workboy_map[scancode];
        }
    }

    return 0;
}

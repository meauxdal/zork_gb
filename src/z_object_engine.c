/*
 * z_object_engine.c
 * Purpose: Z-Machine Version 3 Object Tree and Property Management.
 * Platform: Game Boy (Optimized for MBC5/Banked ROM)
 */

#include <stdint.h>
#include "memory_core.h"
#include "z_string_decoder.h"
#include "z_object_engine.h"

// V3 Constants: 31 default property words (62 bytes), each entry is 9 bytes.
#define DEFAULT_PROP_TABLE_SIZE 62
#define OBJECT_ENTRY_SIZE        9

/* Calculates the absolute ROM address for a given Object ID */
uint32_t get_object_address(uint8_t object_id) {
    uint16_t table_base = z_read_word(0x0A); // Header address 0x0A
    return (uint32_t)table_base + DEFAULT_PROP_TABLE_SIZE + ((uint32_t)(object_id - 1) * OBJECT_ENTRY_SIZE);
}

/* Hierarchy Getters */

uint8_t get_object_parent(uint8_t object_id) {
    if (object_id == 0) return 0;
    return z_read_byte(get_object_address(object_id) + 4);
}

uint8_t get_object_sibling(uint8_t object_id) {
    if (object_id == 0) return 0;
    return z_read_byte(get_object_address(object_id) + 5);
}

uint8_t get_object_child(uint8_t object_id) {
    if (object_id == 0) return 0;
    return z_read_byte(get_object_address(object_id) + 6);
}

/* Hierarchy Setters (Required for picking up/dropping items) */

void set_object_parent(uint8_t object_id, uint8_t parent_id) {
    z_write_byte(get_object_address(object_id) + 4, parent_id);
}

void set_object_sibling(uint8_t object_id, uint8_t sibling_id) {
    z_write_byte(get_object_address(object_id) + 5, sibling_id);
}

void set_object_child(uint8_t object_id, uint8_t child_id) {
    z_write_byte(get_object_address(object_id) + 6, child_id);
}

/* Property Table Logic */

uint32_t get_object_property_table(uint8_t object_id) {
    return (uint32_t)z_read_word(get_object_address(object_id) + 7);
}

/* Attribute Management (32-bit bitmask) */

uint8_t get_object_attr(uint8_t object_id, uint8_t attr_num) {
    uint32_t addr = get_object_address(object_id) + (attr_num >> 3);
    uint8_t  bit  = 0x80 >> (attr_num & 7);
    return (z_read_byte(addr) & bit) != 0;
}

/* The Core Movement Logic: Moving an object into a new parent */
void insert_object(uint8_t obj, uint8_t dest) {
    // 1. Remove from current position in the tree
    remove_object(obj);

    // 2. Make 'obj' the first child of 'dest'
    uint8_t old_first_child = get_object_child(dest);
    set_object_parent(obj, dest);
    set_object_sibling(obj, old_first_child);
    set_object_child(dest, obj);
}

void remove_object(uint8_t obj) {
    uint8_t parent = get_object_parent(obj);
    if (parent == 0) return;

    uint8_t current = get_object_child(parent);
    if (current == obj) {
        // Object was the first child; simple replacement
        set_object_child(parent, get_object_sibling(obj));
    } else {
        // Walk the sibling chain to find 'obj' and bridge the gap
        while (current != 0) {
            uint8_t next = get_object_sibling(current);
            if (next == obj) {
                set_object_sibling(current, get_object_sibling(obj));
                break;
            }
            current = next;
        }
    }
    set_object_parent(obj, 0);
    set_object_sibling(obj, 0);
}

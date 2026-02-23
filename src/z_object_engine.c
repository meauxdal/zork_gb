#include "z_object_engine.h"
#include "z_memory.h"
#include "z_string_decoder.h"

/**
 * Z-Machine Version 3 Object Table Logic
 * Objects are 1-indexed. Object 0 is null.
 * Each entry is 9 bytes: 4 bytes attributes, 1 byte parent,
 * 1 byte sibling, 1 byte child, 2 bytes properties pointer.
 */

uint16_t get_object_address(uint8_t obj_id) {
    if (obj_id == 0) return 0;

    // Header 0x0A contains the address of the object table
    uint16_t object_table_base = z_read_word(0x0A);

    // The first 31 entries in the table are the default property values (31 words)
    uint16_t object_entries_start = object_table_base + (31 * 2);

    // Each object entry is 9 bytes long
    return object_entries_start + (uint16_t)(obj_id - 1) * 9;
}

void get_object_name(uint8_t obj_id) {
    uint16_t addr = get_object_address(obj_id);
    if (addr == 0) return;

    // The properties pointer is at the end of the 9-byte object entry
    uint16_t prop_ptr = z_read_word(addr + 7);

    // The first byte of the properties description is the length of the name in words
    // The name (a Z-string) follows immediately after
    decode_zstring(prop_ptr + 1);
}

uint8_t get_parent(uint8_t obj_id) {
    uint16_t addr = get_object_address(obj_id);
    return (addr == 0) ? 0 : z_read_byte(addr + 4);
}

uint8_t get_sibling(uint8_t obj_id) {
    uint16_t addr = get_object_address(obj_id);
    return (addr == 0) ? 0 : z_read_byte(addr + 5);
}

uint8_t get_child(uint8_t obj_id) {
    uint16_t addr = get_object_address(obj_id);
    return (addr == 0) ? 0 : z_read_byte(addr + 6);
}

void set_parent(uint8_t obj_id, uint8_t new_parent) {
    uint16_t addr = get_object_address(obj_id);
    if (addr != 0) z_write_byte(addr + 4, new_parent);
}

void set_sibling(uint8_t obj_id, uint8_t new_sibling) {
    uint16_t addr = get_object_address(obj_id);
    if (addr != 0) z_write_byte(addr + 5, new_sibling);
}

void set_child(uint8_t obj_id, uint8_t new_child) {
    uint16_t addr = get_object_address(obj_id);
    if (addr != 0) z_write_byte(addr + 6, new_child);
}

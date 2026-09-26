/*
 * z_object_engine.c
 *
 * Z-Machine Version 3 object tree.
 */

#include "z_object_engine.h"
#include "z_memory.h"
#include "z_string_decoder.h"
#include "z_render.h"

uint16_t get_object_address(uint8_t obj_id) {
    if (obj_id == 0u) return 0u;
    uint16_t base  = z_read_word(0x0Au); /* object table pointer in header */
    uint16_t start = base + 31u * 2u;   /* skip 31 default property words */
    return start + (uint16_t)(obj_id - 1u) * 9u;
}

void get_object_name(uint8_t obj_id) {
    uint16_t addr = get_object_address(obj_id);
    if (addr == 0u) return;
    uint16_t prop_ptr = z_read_word(addr + 7u);
    if (prop_ptr == 0u) return;
    /* prop_ptr[0] = name length in words; name z-string follows */
    char buf[128];
    int out_idx = 0;
    decode_z_string((uint32_t)prop_ptr + 1u, buf, &out_idx, (int)sizeof(buf), 0);
    for (int i = 0; i < out_idx; i++) {
        z_render_put_char(buf[i]);
    }
}

uint8_t get_parent(uint8_t obj_id) {
    uint16_t addr = get_object_address(obj_id);
    return addr ? z_read_byte(addr + 4u) : 0u;
}

uint8_t get_sibling(uint8_t obj_id) {
    uint16_t addr = get_object_address(obj_id);
    return addr ? z_read_byte(addr + 5u) : 0u;
}

uint8_t get_child(uint8_t obj_id) {
    uint16_t addr = get_object_address(obj_id);
    return addr ? z_read_byte(addr + 6u) : 0u;
}

void set_parent(uint8_t obj_id, uint8_t new_parent) {
    uint16_t addr = get_object_address(obj_id);
    if (addr) z_write_byte(addr + 4u, new_parent);
}

void set_sibling(uint8_t obj_id, uint8_t new_sibling) {
    uint16_t addr = get_object_address(obj_id);
    if (addr) z_write_byte(addr + 5u, new_sibling);
}

void set_child(uint8_t obj_id, uint8_t new_child) {
    uint16_t addr = get_object_address(obj_id);
    if (addr) z_write_byte(addr + 6u, new_child);
}

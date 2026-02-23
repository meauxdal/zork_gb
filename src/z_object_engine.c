#### FILE: src / z_object_engine.c
#include "z_object_engine.h"
#include "z_memory.h"

uint16_t get_object_address(uint8_t obj_id) {
    uint16_t object_table = z_read_word(0x0A);
    // Skip the default property table (31 words/62 bytes in V3)
    uint16_t object_entries = object_table + 62;
    // Each object entry is 9 bytes in V3
    return object_entries + (uint16_t)((obj_id - 1) * 9);
}

uint8_t get_object_parent(uint8_t obj_id) {
    if (obj_id == 0) return 0;
    return z_read_byte(get_object_address(obj_id) + 4);
}

uint8_t get_object_sibling(uint8_t obj_id) {
    if (obj_id == 0) return 0;
    return z_read_byte(get_object_address(obj_id) + 5);
}

uint8_t get_object_child(uint8_t obj_id) {
    if (obj_id == 0) return 0;
    return z_read_byte(get_object_address(obj_id) + 6);
}

uint8_t get_object_attr(uint8_t obj_id, uint8_t attr_id) {
    uint16_t addr = get_object_address(obj_id);
    uint8_t byte_offset = attr_id >> 3; // attr / 8
    uint8_t bit_mask = 0x80 >> (attr_id & 7); // bit within the byte

    return (z_read_byte(addr + byte_offset) & bit_mask) ? 1 : 0;
}

void set_object_attr(uint8_t obj_id, uint8_t attr_id, uint8_t value) {
    uint16_t addr = get_object_address(obj_id);
    uint8_t byte_offset = attr_id >> 3;
    uint8_t bit_mask = 0x80 >> (attr_id & 7);
    uint8_t current_byte = z_read_byte(addr + byte_offset);

    if (value) {
        z_write_byte(addr + byte_offset, current_byte | bit_mask);
    }
    else {
        z_write_byte(addr + byte_offset, current_byte & ~bit_mask);
    }
}

void insert_object(uint8_t obj_id, uint8_t dest_id) {
    // 1. Remove obj_id from current parent
    uint8_t old_parent = get_object_parent(obj_id);
    if (old_parent != 0) {
        uint16_t parent_addr = get_object_address(old_parent);
        uint8_t prev = 0;
        uint8_t curr = get_object_child(old_parent);

        while (curr != 0 && curr != obj_id) {
            prev = curr;
            curr = get_object_sibling(curr);
        }

        if (curr == obj_id) {
            if (prev == 0) {
                // Was first child
                z_write_byte(parent_addr + 6, get_object_sibling(obj_id));
            }
            else {
                // Patch sibling chain
                z_write_byte(get_object_address(prev) + 5, get_object_sibling(obj_id));
            }
        }
    }

    // 2. Set new parent and make obj_id the first child of dest_id
    uint16_t obj_addr = get_object_address(obj_id);
    uint16_t dest_addr = get_object_address(dest_id);

    z_write_byte(obj_addr + 4, dest_id); // Set parent
    z_write_byte(obj_addr + 5, z_read_byte(dest_addr + 6)); // Obj sibling = dest's old first child
    z_write_byte(dest_addr + 6, obj_id); // Dest first child = obj
}

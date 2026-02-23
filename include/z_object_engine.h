#### FILE: include / z_object_engine.h
#ifndef Z_OBJECT_ENGINE_H
#define Z_OBJECT_ENGINE_H

#include <stdint.h>

// Z-Machine V3 Object Structure:
// 31 bytes per object:
// - Attributes: 4 bytes (32 bits)
// - Parent: 1 byte
// - Sibling: 1 byte
// - Child: 1 byte
// - Property Pointer: 2 bytes

uint16_t get_object_address(uint8_t obj_id);
uint8_t  get_object_parent(uint8_t obj_id);
uint8_t  get_object_sibling(uint8_t obj_id);
uint8_t  get_object_child(uint8_t obj_id);

void     insert_object(uint8_t obj_id, uint8_t destination_id);
uint8_t  get_object_attr(uint8_t obj_id, uint8_t attr_id);
void     set_object_attr(uint8_t obj_id, uint8_t attr_id, uint8_t value);

#endif

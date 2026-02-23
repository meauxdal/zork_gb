#ifndef Z_OBJECT_ENGINE_H
#define Z_OBJECT_ENGINE_H

#include <stdint.h>

/**
 * Z-Machine Object Engine Prototypes
 * Standardized for V3 (Zork I).
 */

uint16_t get_object_address(uint8_t obj_id);
void get_object_name(uint8_t obj_id);

uint8_t get_parent(uint8_t obj_id);
uint8_t get_sibling(uint8_t obj_id);
uint8_t get_child(uint8_t obj_id);

void set_parent(uint8_t obj_id, uint8_t new_parent);
void set_sibling(uint8_t obj_id, uint8_t new_sibling);
void set_child(uint8_t obj_id, uint8_t new_child);

#endif

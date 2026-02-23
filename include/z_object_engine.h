#ifndef Z_OBJECT_ENGINE_H
#define Z_OBJECT_ENGINE_H

#include <stdint.h>

/*
 * z_object_engine.h
 *
 * Z-Machine Version 3 object tree operations.
 *
 * Object table layout (spec §12):
 *   [object_table_base + 0 .. 61] : 31 default property words
 *   [+ 62 onward]                 : object entries, 9 bytes each:
 *       bytes 0-3 : 32 attribute bits
 *       byte  4   : parent object number (1-indexed, 0=none)
 *       byte  5   : sibling object number
 *       byte  6   : child object number
 *       bytes 7-8 : properties table pointer
 */

uint16_t get_object_address(uint8_t obj_id);
void     get_object_name(uint8_t obj_id);  /* prints via z_render */

uint8_t  get_parent(uint8_t obj_id);
uint8_t  get_sibling(uint8_t obj_id);
uint8_t  get_child(uint8_t obj_id);
void     set_parent(uint8_t obj_id, uint8_t new_parent);
void     set_sibling(uint8_t obj_id, uint8_t new_sibling);
void     set_child(uint8_t obj_id, uint8_t new_child);

#endif /* Z_OBJECT_ENGINE_H */

#ifndef Z_OBJECT_ENGINE_H
#define Z_OBJECT_ENGINE_H

#include <stdint.h>

uint32_t get_object_address(uint8_t object_id);
uint8_t  get_object_parent(uint8_t object_id);
uint8_t  get_object_sibling(uint8_t object_id);
uint8_t  get_object_child(uint8_t object_id);
uint32_t get_object_property_table(uint8_t object_id);
uint8_t  get_object_attr(uint8_t object_id, uint8_t attr_num);
void     insert_object(uint8_t obj, uint8_t dest);
void     remove_object(uint8_t obj);
void     put_prop_value(uint8_t obj, uint8_t prop_id, uint16_t value);

#endif

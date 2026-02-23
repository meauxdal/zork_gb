#ifndef ZORK_DATA_H
#define ZORK_DATA_H

#include <stdint.h>

#define ZORK_DATA_BANK_START  1
#define ZORK_DATA_NUM_BANKS   6
#define ZORK_BANK_SIZE        0x4000u

extern const unsigned char zork_bank1_data[16384];
extern const unsigned char zork_bank2_data[16384];
extern const unsigned char zork_bank3_data[16384];
extern const unsigned char zork_bank4_data[16384];
extern const unsigned char zork_bank5_data[16384];
extern const unsigned char zork_bank6_data[16384];

#endif

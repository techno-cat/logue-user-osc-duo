/*
Copyright 2019 Tomoaki Itoh
This software is released under the MIT License, see LICENSE.txt.
//*/

#include <stdint.h>

#define LCW_AA_TABLE_BITS (10)
#define LCW_AA_TABLE_SIZE (1024)
#define LCW_AA_TABLE_MASK (0x3FF)

#define LCW_AA_VALUE_BITS (14)
#define LCW_AA_VALUE_MAX (1 << (LCW_AA_VALUE_BITS))
#define LCW_AA_VALUE_MASK ((LCW_AA_VALUE_MAX) - 1)

extern const int16_t gLcwAntiAiliasingTable[LCW_AA_TABLE_SIZE];

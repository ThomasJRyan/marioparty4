#include "dolphin/types.h"

u32 randomGeneratorState;

void InitializeRandomGenerator (u32 seed) {
    randomGeneratorState = seed;
}

s32 GetRandomInteger(void) {
    return (randomGeneratorState = (randomGeneratorState * 0x41C64E6D) + 0x3039) / 65536 & 0x7FFF;
}
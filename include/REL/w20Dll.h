#ifndef REL_W20DLL_H
#define REL_W20DLL_H

#include "dolphin/types.h"

typedef struct W20BoardData {
    /* 0x00 */ s32 boardState;
    /* 0x04 */ s16 boardSubstate;
    /* 0x06 */ s16 boardEventState;
} W20BoardData;

#endif

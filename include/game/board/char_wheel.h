#ifndef _BOARD_CHAR_WHEEL_H
#define _BOARD_CHAR_WHEEL_H

#include "dolphin/types.h"

s32 BoardCharWheelInit(s32 player, s32 mode);
BOOL BoardCharWheelCheck(void);
void BoardCharWheelWait(void);
s32 BoardCharWheelResultGet(void);
void BoardCharWheelSpeedSet(float speed);
void BoardCharWheelTargetSet(s16 player);

#endif

#ifndef _GAME_PERF_H
#define _GAME_PERF_H

#include "dolphin.h"

void HuPerfInit(void);
s32 HuPerfCreate(char *name, u8 red, u8 green, u8 blue, u8 alpha);
void HuPerfZero(void);
void HuPerfBegin(s32 timer_slot);
void HuPerfEnd(s32 timer_slot);

#endif

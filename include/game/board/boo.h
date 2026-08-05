#ifndef _BOARD_BOO_H
#define _BOARD_BOO_H

#include "dolphin.h"
#include "game/object.h"

s16 BoardBooCreate(s32 owner_player, Vec *input_position);
void BoardBooKill(void);
s32 BoardBooStealTypeSet(s32 theft_type);
s32 BoardBooStealMain(void);
s32 BoardBooStealValueGet(s16 *stolen_coins, s16 *punch_reduction);
BOOL BoardBooStealLightCheck(void);

#endif

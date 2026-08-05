#ifndef _BOARD_STAR_H
#define _BOARD_STAR_H

#include "game/board/space.h"

#include "dolphin.h"

void BoardStarHostSet(s16 host_model);
s16 BoardStarHostMdlGet(void);
void BoardStarExec(s32 player, s32 space);
void BoardStarGive(s32 player, Vec *start_position);
void BoardStarShowNext(s32 player);

extern s32 boardStarSndTbl[];

#endif

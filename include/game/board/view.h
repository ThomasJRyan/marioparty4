#ifndef _BOARD_VIEW_H
#define _BOARD_VIEW_H

#include "dolphin.h"

void BoardViewOverheadExec(s32 player);
void BoardViewFocusGetPos(Vec *position);
void BoardViewFocusSet(Vec *target_position, u16 duration);
void BoardViewMoveStart(Vec *start_position, Vec *target_position, u16 duration);
BOOL BoardViewMoveCheck(void);
void BoardViewMoveEnd(void);
void BoardViewWait(void);
void BoardViewMapExec(s32 player);

#endif

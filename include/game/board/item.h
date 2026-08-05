#ifndef _BOARD_ITEM_H
#define _BOARD_ITEM_H

#include "dolphin/types.h"

void BoardItemStart(s32 player, s32 item);
BOOL BoardItemDoneCheck(void);
void BoardItemBagItemSet(s16 *items);
#ifndef __MWERKS__
void BoardItemPlayerRestore(s32 player, s32 item);
#endif

#endif

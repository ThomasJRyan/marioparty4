#ifndef _BOARD_UI_H
#define _BOARD_UI_H

#include "game/sprite.h"

s32 BoardItemModelGet(s32 item);
s32 BoardItemNameGet(s32 item);
BOOL BoardStatusStopCheck(s32 player);
s32 BoardStatusVisibleGet(s32 player);
void BoardStatusShowSetAll(s32 show);
void BoardStatusItemSet(s32 show);
void BoardStatusShowSetForce(s32 player);
void BoardStatusGraySet(s32 player, s32 gray);
void BoardStatusShowSet(s32 player, s32 show);
void BoardStatusItemHideSet(s32 player, s32 show_items);
void BoardStatusTargetPosSet(s32 player, Vec *position);
void BoardStatusPosGet(s32 player, Vec *position);
void BoardStatusPosSet(s32 player, Vec *position);
void BoardStatusHammerCreate(s32 player);
void BoardStatusHammerShowSet(s32 player, s32 animate);
void BoardStatusHammerKill(s32 player);
void BoardStatusKill(void);
void BoardStatusCreate(void);
void BoardSpriteCreate(s32 file, s32 prio, ANIMDATA **anim, s16 *sprite);
void BoardSpriteDigitUpdate(s16 sprite_group, s16 ones_sprite, s32 value);
void BoardPickerCreate(s32 player, s8 choice_count, void *positions, s8 target_choice);
void BoardPickerBackFlagSet(s32 allow_back);
BOOL BoardPickerDoneCheck(void);
s32 BoardPickerPosGet(Vec *position);
s32 BoardPickerChoiceGet(void);
void BoardYourTurnExec(s32 player);
s32 BoardItemUseExec(s32 player);
void BoardMakeRandomItem(void);
void BoardItemGetDestPos(s32 player, Vec *positions);
void BoardItemStatusKill(s32 player);

extern s8 boardItemUser;

#endif

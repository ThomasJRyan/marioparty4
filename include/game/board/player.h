#ifndef _BOARD_PLAYER_H
#define _BOARD_PLAYER_H

#include "math.h"
#include "dolphin.h"
#include "game/gamework_data.h"
#include "game/gamework.h"
#include "game/window.h"
#include "game/object.h"
#include "game/hu3d.h"
#include "game/board/main.h"
#include "game/board/space.h"
#include "game/board/ui.h"

enum {
	BOARD_ITEM_NONE = -1,
	BOARD_ITEM_MINI_MUSHROOM = 0,
	BOARD_ITEM_MEGA_MUSHROOM = 1,
	BOARD_ITEM_SUPER_MINI_MUSHROOM = 2,
	BOARD_ITEM_SUPER_MEGA_MUSHROOM = 3,
	BOARD_ITEM_MINI_MEGA_HAMMER = 4,
	BOARD_ITEM_WARP_PIPE = 5,
	BOARD_ITEM_SWAP_CARD = 6,
	BOARD_ITEM_SPARKY_STICKER = 7,
	BOARD_ITEM_GADDLIGHT = 8,
	BOARD_ITEM_CHOMP_CALL = 9,
	BOARD_ITEM_BOWSER_SUIT = 10,
	BOARD_ITEM_BOOS_CRYSTAL_BALL = 11,
	BOARD_ITEM_MAGIC_LAMP = 12,
	SHOP_ITEMS_END = BOARD_ITEM_MAGIC_LAMP,
	BOARD_ITEM_ITEM_BAG = 13,
	BOARD_ITEMS_END
};

extern s16 boardPlayerMdl[4];

static inline s32 BoardPlayerHandicapGet(s32 player)
{
	return GWPlayer[player].handicap;
}

static inline PlayerState *BoardPlayerGet(s32 player)
{
	return &GWPlayer[player];
}

static inline s16 BoardPlayerModelGet(s32 player)
{
	PlayerState *player_ptr = BoardPlayerGet(player);
	return boardPlayerMdl[player_ptr->player_idx];
}

s32 BoardItemPrevGet(void);
void BoardItemPrevSet(s32 item);
s32 BoardPlayerGetCharMess(s32 player);
char* BoardPlayerGetCharName(s32 player);
s32 BoardPlayerIsAllCom(void);
void BoardPlayerInit(void);
void BoardPlayerModelInit(void);
void BoardPlayerModelKill(void);
void BoardPlayerLayerSet(s32 player, s32 layer);
void BoardPlayerCameraSet(s32 player, u16 camera_mask);
void fn_80062A40(s32 player, f32 value);
void BoardPlayerExistCheck(s32 player, s32 check_flag);
s32 BoardPlayerItemAdd(s32 player, s32 item);
s32 BoardPlayerItemRemove(s32 player, s32 item_slot);
s32 BoardPlayerItemFind(s32 player, s32 item);
s32 BoardPlayerItemCount(s32 player);
void BoardPlayerCornerPosSet(s32 player);
void BoardPlayerCornerPosGet(s32 player, Vec* position);
void BoardPlayerAmbSet(s32 player, f32 red, f32 green, f32 blue);
void BoardPlayerMtxSet(s32 player, Mtx* matrix);
void BoardPlayerPosSetV(s32 player, Vec* position);
void BoardPlayerPosSet(s32 player, f32 x, f32 y, f32 z);
void BoardPlayerPosGet(s32 player, Vec* position);
void BoardPlayerRotSetV(s32 player, Vec* rotation);
void BoardPlayerRotSet(s32 player, f32 x, f32 y, f32 z);
void BoardPlayerRotGet(s32 player, Vec* rotation);
void BoardPlayerRotYSet(s32 player, f32 yaw);
f32 BoardPlayerRotYGet(s32 player);
void BoardPlayerScaleSetV(s32 player, Vec* scale);
void BoardPlayerScaleSet(s32 player, f32 x, f32 y, f32 z);
void BoardPlayerScaleGet(s32 player, Vec* scale);
s16 BoardPlayerCurrMotionGet(s32 player);
s32 BoardPlayerMotionCreate(s32 player, s32 data_num);
s32 BoardPlayerMotionKill(s32 player, s32 motion);
s32 BoardPlayerMotionEndCheck(s32 player);
void BoardPlayerMotionEndWait(s32 player);
void BoardPlayerMotionStart(s32 player, s32 motion, s32 attributes);
void BoardPlayerMotionShiftSet(s32 player, s32 motion, f32 start_time, f32 blend_time, u32 attributes);
void BoardPlayerMotionSpeedSet(s32 player, f32 speed);
void BoardPlayerMotionTimeSet(s32 player, f32 time);
f32 BoardPlayerMotionTimeGet(s32 player);
f32 BoardPlayerMotionMaxTimeGet(s32 player);
void BoardPlayerMotionStartEndSet(s32 player, f32 start_time, f32 end_time);
void BoardPlayerModelAttrSet(s32 player, s32 attributes);
void BoardPlayerModelAttrReset(s32 player, s32 attributes);
void BoardPlayerCoinsSet(s32 player, s32 coins);
s32 BoardPlayerCoinsGet(s32 player);
void BoardPlayerCoinsAdd(s32 player, s32 delta);
void BoardPlayerStarsAdd(s32 player, s32 delta);
void BoardPlayerSizeSet(s32 player, s32 size);
s32 BoardPlayerSizeGet(s32 player);
s32 BoardPlayerSameTeamFind(s32 player);
s32 BoardPlayerTeamFind(s32 player);
s32 BoardPlayerRankCalc(s32 player);
void BoardPlayerPreTurnHookSet(s32 player, s32 (*hook)());
void BoardPlayerPostTurnHookSet(s32 player, s32 (*hook)());
void BoardPlayerTurnExec(s32 player);
void BoardPlayerTurnRollExec(s32 player);
void BoardPlayerTurnMoveExec(s32 player);
void BoardPlayerPostTurnHookExec(s32 player);
void BoardPlayerSizeRestore(s32 player);
void BoardPlayerZoomRestore(s32 player);
void BoardJunctionMaskSet(s32 flags);
void BoardJunctionMaskReset(s32 flags);
void BoardJunctionMaskZero(void);
void BoardPlayerVoiceEnableSet(s32 player, s32 motion, s32 enabled);

#ifndef __MWERKS__
void BoardPlayerMoveBetween(s32 player, s32 start_space, s32 target_space);
#endif

void BoardPlayerMoveTo(s32 player, s32 target_space);
void BoardPlayerMoveToAsync(s32 player, s32 target_space);
void BoardPlayerPosLerpStart(s32 player, Vec* start_pos, Vec* target_pos, s16 duration);
void BoardPlayerDiceJumpStart(s32 player);
s32 BoardPlayerDiceJumpCheck(s32 player);
void BoardPlayerIdleSet(s32 player);
void BoardPlayerMotBlendSet(s32 player, s16 target_yaw, s16 duration);

void BoardPlayerBtnDownWait(s32 player, u32 button_mask);
void BoardPlayerAutoSizeSet(s32 player, s32 size);
s32 BoardPlayerAutoSizeGet(s32 player);
u32 BoardPlayerMoveAwayIsDone(void);
void BoardPlayerMoveAwayStart(s32 ordering_player, s32 space, s32 immediate);
void BoardPlayerMoveAwayStartCurr(s32 space, s32 immediate);
void BoardPlayerCopyMat(s32 player);
void BoardPlayerCopyEyeMat(s32 player, s32 blacken);
void BoardPlayerSparkSet(s32 player);
void BoardBowserSuitMotionSetJump(void);
//...
s32 BoardPlayerAutoSizeGet(s32 player);
void BoardPlayerAutoSizeSet(s32 player, s32 size);
void BoardPlayerCopyMat(s32 player);
void BoardBowserSuitInit(s32 player);
void BoardBowserSuitKill(s32 player);
void BoardRollCreate(s32 player, s32 animate_reveal);
void BoardRollUpdateSet(s32 unused_player);
s32 BoardMegaDoubleDiceCheck(void);
void BoardPlayerResizeAnimExec(s32 player, s32 size);
s16 BoardBowserSuitPlayerModelGet(void);
void BoardBowserSuitMotionSetWait(void);
void BoardBowserSuitPlayerModelKill(void);
void BoardRollDispSet(s32 visible);
s32 BoardPlayerMotBlendCheck(s32 player);
void BoardBowserSuitMotionSetWalk(void);
s16 BoardBowserSuitModelGet(void);

#endif

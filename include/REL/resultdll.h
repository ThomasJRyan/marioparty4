#ifndef REL_RESULTDLL_H
#define REL_RESULTDLL_H

#include "game/data.h"
#include "game/hu3d.h"

#include "game/sprite.h"

typedef struct ResultStageModel {
	s32 dataNumber;
	u32 attributes;
	s16 modelType;
	s16 linkedModelIndex;
	s16 motionLinkIndex;
	Vec pos;
	Vec rot;
	Vec scale;
} ResultStageModel;

typedef struct ResultStageSprite {
	u32 dataNumber;
	s16 attributes;
	s16 priority;
	float x;
	float y;
	GXColor color;
} ResultStageSprite;

void ResultBoardDataRead(void);
void ResultCoinAdd(void);
void ResultCoinNumGet(s16 *coinNum);

void ResultBattleMain(void);

void StageModelCreate(ResultStageModel *modelEntries);
void StageSpriteCreate(ResultStageSprite *spriteEntries);
void StageSpriteKill(void);

extern s16 resultStageModelIds[32];
extern s16 resultStageMotionIds[32];
extern s16 resultStageSpriteIds[33];
extern s16 unreferencedResultDatalistValue;
extern s32 resultReadEndF;
extern s32 resultFastF;
extern s16 resultBonusPlayer;

#endif

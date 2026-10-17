#ifndef REL_MODESELDLL_H
#define REL_MODESELDLL_H

#include "game/data.h"
#include "game/hu3d.h"

#include "game/sprite.h"

typedef struct DatalistModel {
	s32 datanum;
	u32 attr;
	s16 type;
	s16 link;
	s16 motLink;
	Vec pos;
	Vec rot;
	Vec scale;
} DataListModel;

typedef struct DatalistSprite {
	u32 datanum;
	s16 attr;
	s16 prio;
	float x;
	float y;
	GXColor color;
} DataListSprite;

#ifndef __MWERKS__
void SetModeSelectionCamera(s16 view);
#endif

s32 SelectSpecialMode(void);
s32 RunFileSelection(void);

void LoadModeSelectionModels(DataListModel *modelList);
void LoadModeSelectionSprites(DataListSprite *spriteList);
void UnloadModeSelectionSprites(void);

extern s16 modeSelectModelIds[24];
extern s16 modeSelectMotionIds[24];
extern s16 modeSelectSpriteIds[12];
extern s16 lastLoadedModelId;
extern s16 introSoundEffectId;
extern DataListModel modeSelectModelData[];
extern DataListSprite modeSelectSpriteData[];

#endif

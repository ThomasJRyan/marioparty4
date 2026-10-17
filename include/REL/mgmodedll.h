#ifndef REL_MGMODEDLL_H
#define REL_MGMODEDLL_H

#include "game/data.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"

#include "game/pad.h"
#include "game/sprite.h"
#include "game/window.h"

typedef struct DataListModel {
    s32 datanum;
    u32 attr;
    s16 type;
    s16 link;
    s16 motLink;
    Vec pos;
    Vec rot;
    Vec scale;
} DataListModel; // Size 0x34

typedef struct DataListSprite {
    u32 datanum;
    s16 attr;
    s16 prio;
    float x;
    float y;
    GXColor color;
} DataListSprite; // Size 0x14

typedef struct MinigamePlayerState {
    OMOBJ *object;
    void (*updateCallback)(OMOBJ *, ...);
    u8 unreferencedPadding[0x50];
    s32 playerIndex;
    s32 team;
    s32 isComputer;
    s32 difficulty;
    s32 character;
    s32 controllerPort;
    s32 selectionFlags[4];
    s32 selectionValues[4];
} MinigamePlayerState; // Size 0x90

typedef struct MinigameSharedState {
    s32 boardModelId;
    s32 boardMotionId;
    s32 selectionModelMotionId;
    s32 modeModelId;
    s32 modeModelMotionId;
    s32 victoryMotionId;
    s32 alternateModeMotionId;
    s32 modeSelectionType;
    s32 battleStarMaximum;
    MinigamePlayerState playerStates[4];
} MinigameSharedState; // Size 0x264

extern s16 mgModeModelIds[13];
extern s16 mgModeMotionIds[13];
extern s16 mgModeSpriteIds[29];
extern s16 playerCharacterIdsByGroup[4];
extern s16 playerOrderByGroup[4];
extern s16 playerModelIdsByGroup[4];
extern s16 playerMotionIdsByGroup[4][10];

extern s16 minigameIdsByType[10][70];
extern s16 minigameCountByType[10];
extern s16 availableMinigameIdsByType[10][70];
extern s16 availableMinigameCountByType[10];
extern s16 characterAssetsLoaded;

extern HUPROCESS *objman;

extern s16 freePlayWindowState;

extern s16 freePlayModeType;
extern s16 freePlayModeSelection;

extern MinigameSharedState minigameSharedState;
extern s16 mgModeEvtNo;

extern s16 freePlaySelectionOrder[];
extern DataListModel mgModeModelDataList[];
extern DataListSprite mgModeSpriteDataList[];

s32 runFreePlaySelection(void);
s32 checkFreePlaySelection(s32 value);
void showMinigameRecords(void);
s32 runBattleMinigameSelection(void);
s32 runTicTacToeMinigame(void);

void initializeMinigameMenuObjects(HUPROCESS *processManager, MinigameSharedState *sharedState);

void playMinigameModeOpening(void);
s32 runMinigameModeSelection(s32 modeType);

s32 createStandardMessageWindow(s32 windowId, s32 messageId, s32 choiceIndex);
void closeStandardMessageWindow(s32 windowId);
void showMessageAndWait(s32 windowId, s32 messageId, s32 choiceIndex, s32 waitFrames);
s32 showMessageChoiceWindow(s32 windowId, s32 messageId, s32 choiceIndex);
s32 createCenteredMessageWindow(s32 windowId);
void closeCenteredMessageWindow(s32 windowId);

void createDataListModels(DataListModel *modelList);
void createDataListSprites(DataListSprite *spriteList);
void installMinigameBackgroundDrawHook(void);
void loadMinigameCharacterAssets(void);
s16 setMinigamePreviewSprite(s16 spriteGroupId, s16 spriteIndex, s16 x);
void createPlayerCharacterModels(void);
void buildMinigameTypeLists(s32 modeType);

#endif

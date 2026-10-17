#ifndef REL_W10DLL_H
#define REL_W10DLL_H

#include "game/gamework_data.h"
#include "dolphin.h"
#include "REL/executor.h"
#include "game/process.h"
#include "game/window.h"

enum {
    MAPOBJ_MELON = 0,
    MAPOBJ_MAX
};

typedef struct W10TutorialBoardWork {
/* 0x00 */ s32 sceneIndex;
/* 0x04 */ u16 rollIndex;
/* 0x06 */ u16 messageIndex;
/* 0x08 */ s16 focusModel;
} W10TutorialBoardWork;

typedef void (*TutorialSceneFunc)(void);

typedef struct TutorialSceneEntry {
    s16 eventCue;
    TutorialSceneFunc execute;
} TutorialSceneEntry;

extern s32 boardTutorialData[4];

void HostPosSet(s16 positionIndex);
void TutorialEventInit(void);
void TutorialEventReset(void);
void TutorialSpaceHide(void);
void TutorialSpaceShow(s16 spriteIndex, s16 screenX, s16 screenY);
void HostWinExec(s32 messageId);
void TutorialLandExec(void);
void TutorialNullExec(void);
void TutorialSprCreate(void);
void TutorialSprKill(void);
void HostCreate(void);
int TutorialHookExec(s16 eventCue, s32 eventParameter);

void TutorialStartExec(void);
void TutorialPreRollExec(void);
void TutorialOrderExec(void);
void TutorialP1StartExec(void);
void TutorialP2StartExec(void);
void TutorialP3StartExec(void);
void TutorialP4StartExec(void);
void TutorialGameStartExec(void);
void TutorialRollExec(void);
void TutorialMoveExec(void);
void TutorialLandExec(void);
void TutorialLandBlueExec(void);
void TutorialLandRedExec(void);
void TutorialLandHappenExec(void);
void TutorialLandFortuneExec(void);
void TutorialLandBowserExec(void);
void TutorialLandMushroomExec(void);
void TutorialMushroomMegaExec(void);
void TutorialMushroomMiniExec(void);
void TutorialLandBattleExec(void);
void TutorialLandWarpExec(void);
void TutorialWarpDecideExec(void);
void TutorialBlockStarExec(void);
void TutorialBlockCoinExec(void);
void TutorialMiniMegaExec(void);
void TutorialMiniUseExec(void);
void TutorialMiniStopExec(void);
void TutorialMegaUseExec(void);
void TutorialMegaDiceExec(void);
void TutorialMegaStompExec(void);
void TutorialMegaEventExec(void);
void TutorialMegaStopExec(void);
void TutorialEndExec(void);
void TutorialMGStatusExec(void);
void TutorialMG4PExec(void);
void TutorialMG2Vs2Exec(void);
void TutorialMG1Vs3Exec(void);
void TutorialMG1Vs3AltExec(void);
void TutorialShopExec(void);
void TutorialLotteryExec(void);
void TutorialBooHouseExec(void);

extern W10TutorialBoardWork *tutorialBoardWork;
extern s16 boardMapObjMdl[MAPOBJ_MAX];
extern s16 tutorialDoneF;
extern s16 tutorialExitWin;

extern s16 boardStarHostMdl;
extern s16 boardShopHostMdl;
extern s16 boardLotteryHostMdl;
extern s16 boardBooHouseHostMdl;

#endif

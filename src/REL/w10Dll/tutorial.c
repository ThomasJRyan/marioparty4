#include "REL/w10Dll.h"
#include "game/esprite.h"
#include "game/pad.h"
#include "game/data.h"
#include "game/wipe.h"
#include "game/board/ui.h"

#ifndef __MWERKS__
#include "game/board/audio.h"
#include "game/board/main.h"
#include "game/board/roll.h"
#include "game/board/tutorial.h"
#endif

static s16 sprList[11];
static s16 sprCurr;

static void CloseTutorial(void) {
    s32 port;
    s32 i;

    if (tutorialDoneF == 0) {
        if (WipeStatGet() != 0) {
            HuWinDispOff(tutorialExitWin);
            return;
        }
        HuWinDispOn(tutorialExitWin);
        if (BoardIsKill() != 0) {
            return;
        }
        for (i = 0; i < 4; i++) {
            port = GWPlayer[i].port;
            if (port == -1) {
                continue;
            }
            if (HuPadStatGet(port) == 0 && HuPadBtnDown[port] & PAD_BUTTON_START) {
                break;
            }
        }
        if (i != 4) {
            tutorialDoneF = 1;
            OSReport("@@@@@@@@@@@@@@ Tutorial Exit @@@@@@@@@@@@@@\n");
            BoardAudSeqFadeOut(0, 0x1F3);
            BoardKill();
            BoardTutorialHookSet(NULL);
        }
    }
}

static s16 rollDataTbl[11][3] = {
	1, -1, -1,
	2, -1, -1,
	4, -1, -1,
	5, -1, -1,
	5, -1, -1,
	4, -1, -1,
	1, -1, -1,
	3, -1, -1,
	2, -1, -1,
	4, -1, -1,
	5, 6, -1,
};

static TutorialSceneEntry tutorialSceneTable[42] = {
	{0, TutorialStartExec},
	{1, TutorialPreRollExec},
	{2, TutorialOrderExec},
	{3, TutorialP1StartExec},
	{3, TutorialP2StartExec},
	{3, TutorialP3StartExec},
	{3, TutorialP4StartExec},
	{4, TutorialGameStartExec},
	{5, TutorialRollExec},
	{6, TutorialMoveExec},
	{7, TutorialLandExec},
	{10, TutorialLandBlueExec},
	{11, TutorialLandRedExec},
	{16, TutorialLandHappenExec},
	{17, TutorialLandFortuneExec},
	{12, TutorialLandBowserExec},
	{13, TutorialLandMushroomExec},
	{14, TutorialMushroomMegaExec},
	{14, TutorialMushroomMiniExec},
	{15, TutorialLandBattleExec},
	{18, TutorialLandWarpExec},
	{20, TutorialWarpDecideExec},
	{21, TutorialBlockStarExec},
	{22, TutorialBlockCoinExec},
	{5, TutorialMiniMegaExec},
	{23, TutorialMiniUseExec},
	{7, TutorialMiniStopExec},
	{5, TutorialMegaUseExec},
	{23, TutorialMegaDiceExec},
	{24, TutorialMegaStompExec},
	{25, TutorialMegaEventExec},
	{8, TutorialMegaStopExec},
	{9, TutorialEndExec},
	{26, TutorialMGStatusExec},
	{27, TutorialMG4PExec},
	{27, TutorialMG2Vs2Exec},
	{27, TutorialMG1Vs3Exec},
	{27, TutorialMG1Vs3AltExec},
	{28, TutorialShopExec},
	{28, TutorialLotteryExec},
	{28, TutorialBooHouseExec},
	{-1, NULL },
};

int TutorialHookExec(s16 eventCue, s32 eventParameter) {
    if (eventCue == 30) {
        CloseTutorial();
        return;
    }
    if (eventCue != 29) {
        OSReport("Tutorial Hook P0:%d P1:%d P2:%d P3:%d  SCEN:%d  CUE:%d  PRM:%d DICE:%d  MSG:%d\n",
            GWPlayer[0].com, GWPlayer[1].com,
            GWPlayer[2].com, GWPlayer[3].com,
            tutorialBoardWork->sceneIndex, eventCue, eventParameter, tutorialBoardWork->rollIndex, tutorialBoardWork->messageIndex);
        if (tutorialSceneTable[tutorialBoardWork->sceneIndex].eventCue != -1) {
            if (eventCue == 5) {
                BoardRollTutorialSet(&rollDataTbl[tutorialBoardWork->rollIndex++][0]);
            }
            if (eventCue == tutorialSceneTable[tutorialBoardWork->sceneIndex].eventCue) {
                TutorialSceneFunc sceneFunction = tutorialSceneTable[tutorialBoardWork->sceneIndex++].execute;
                sceneFunction();
            }
        }
    }
}

static s32 tutorialSprTbl[11] = {
    DATA_MAKE_NUM(DATADIR_W10, 4),
    DATA_MAKE_NUM(DATADIR_W10, 5),
    DATA_MAKE_NUM(DATADIR_W10, 6),
    DATA_MAKE_NUM(DATADIR_W10, 7),
    DATA_MAKE_NUM(DATADIR_W10, 8),
    DATA_MAKE_NUM(DATADIR_W10, 9),
    DATA_MAKE_NUM(DATADIR_W10, 10),
    DATA_MAKE_NUM(DATADIR_W10, 11),
    DATA_MAKE_NUM(DATADIR_W10, 12),
    DATA_MAKE_NUM(DATADIR_W10, 13),
    DATA_MAKE_NUM(DATADIR_W10, 14)
};

void TutorialSprCreate(void) {
    s32 i;

    for (i = 0; i < 11; i++) {
        sprList[i] = espEntry(tutorialSprTbl[i], 0, 0);
        espDispOff(sprList[i]);
    }
}

void TutorialSprKill(void) {
    s32 i;

    for (i = 0; i < 11; i++) {
        espKill(sprList[i]);
    }
}

void TutorialSpaceShow(s16 spriteIndex, s16 screenX, s16 screenY) {
    f32 scaleX;
    f32 scaleY;
    u32 i;

    sprCurr = sprList[spriteIndex];
    espPosSet(sprCurr, screenX, screenY);
    scaleX = 0.0f;
    scaleY = 0.0f;
    espDispOn(sprCurr);
    for (i = 0; i < 15; i++) {
        scaleX += 0.06666667f;
        scaleY += 0.06666667f;
        espScaleSet(sprCurr, scaleX, scaleY);
        HuPrcVSleep();        
    }
    espScaleSet(sprCurr, 1.0f, 1.0f);
}

void TutorialSpaceHide(void) {
    f32 scaleX;
    f32 scaleY;
    u32 i;

    scaleX = 1.0f;
    scaleY = 1.0f;
    for (i = 0; i < 15; i++) {
        scaleX -= 0.06666667f;
        scaleY -= 0.06666667f;
        espScaleSet(sprCurr, scaleX, scaleY);
        HuPrcVSleep();        
    }
    espDispOff(sprCurr);
}

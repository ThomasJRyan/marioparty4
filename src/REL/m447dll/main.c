#include "REL/m447dll.h"
#include "REL/executor.h"
#include "game/esprite.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/window.h"
#include "game/wipe.h"

#include "math.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/audio.h"
#endif

typedef struct {
    /* 0x00 */ s16 spriteId;
    /* 0x02 */ char padding02[2];
    /* 0x04 */ float scaleProgress;
    /* 0x08 */ float scaleStep;
} M447TransitionState; // Size 0xC

void M447ExitProcess(void);
M447GameState* M447GameCreate(void);
void M447GameDestroy(M447GameState* gameState);
BOOL M447ExitReadyCheck(M447GameState* gameState);
void M447GamePhaseSet(M447GameState* gameState, s32 phaseIndex);
void M447GameProcess(void);
void M447IntroPhaseUpdate(M447GameState* gameState);
void M447RoundSetupUpdate(M447GameState* gameState);
void M447RoundPlayUpdate(M447GameState* gameState);
void M447BoardCleanupUpdate(M447GameState* gameState);
void M447GameResultUpdate(M447GameState* gameState);
void M447MessageWait(M447GameState* gameState, s16 windowId, s32 waitFrames);
s32 M447ChoiceGet(M447GameState* gameState, s16 windowId, s32 choiceFlags, s32 computerChoice, s32 waitFrames);
void M447CharacterIntroDialog(void);
void M447HudCreate(M447GameState* gameState);
void M447HudDestroy(M447GameState* gameState);
void M447ScoreboardShow(M447GameState* gameState);
void M447ScoreIconAnimate(M447GameState* gameState, s32 playerIndex, s32 scoreIndex);
void M447ScoreIconAnimationProcess(void);
void M447HudHide(M447GameState* gameState);
void M447ComputerPlayerCountGet(M447GameState* gameState);

M447StageState* stageState;
char alignmentPadding[4];
M447GameState* gameState;
HUPROCESS* objectManager;
HUPROCESS* exitProcess;

const s32 hudAssetIds[] = {
    DATA_MAKE_NUM(DATADIR_M447, 53),
    DATA_MAKE_NUM(DATADIR_M447, 53),
    DATA_MAKE_NUM(DATADIR_M447, 54),
    DATA_MAKE_NUM(DATADIR_M447, 54),
    DATA_MAKE_NUM(DATADIR_M447, 55),
    DATA_MAKE_NUM(DATADIR_M447, 55),
    DATA_MAKE_NUM(DATADIR_M447, 55),
    DATA_MAKE_NUM(DATADIR_M447, 56),
    DATA_MAKE_NUM(DATADIR_M447, 56),
    DATA_MAKE_NUM(DATADIR_M447, 56),
    DATA_MAKE_NUM(DATADIR_M447, 55),
    DATA_MAKE_NUM(DATADIR_M447, 55),
    DATA_MAKE_NUM(DATADIR_M447, 55),
    DATA_MAKE_NUM(DATADIR_M447, 56),
    DATA_MAKE_NUM(DATADIR_M447, 56),
    DATA_MAKE_NUM(DATADIR_M447, 56),
    DATA_MAKE_NUM(DATADIR_M447, 44),
    DATA_MAKE_NUM(DATADIR_M447, 45),
    DATA_MAKE_NUM(DATADIR_M447, 46),
    DATA_MAKE_NUM(DATADIR_M447, 47),
    DATA_MAKE_NUM(DATADIR_M447, 48),
    DATA_MAKE_NUM(DATADIR_M447, 49),
    DATA_MAKE_NUM(DATADIR_M447, 50),
    DATA_MAKE_NUM(DATADIR_M447, 51),
    DATA_MAKE_NUM(DATADIR_M447, 52)
};

void ObjectSetup(void) {
    objectManager = omInitObjMan(10, 0x2000);
    omGameSysInit(objectManager);
    HuWinInit(1);
    exitProcess = HuPrcChildCreate(M447ExitProcess, 1000, 0x2000, 0, objectManager);
    gameState = M447GameCreate();
}

void M447ExitProcess(void) {
    while (omSysExitReq == 0) {
        HuPrcVSleep();
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    HuAudFadeOut(100);
    M447GameDestroy(gameState);
    HuPrcVSleep();
    omOvlReturnEx(1, 1);
    HuPrcEnd();
}

M447GameState* M447GameCreate(void) {
    Vec blockPosition;
    Vec velocity = { 0.0f, 3000.0f, 1.0f };
    Vec rotation = { 0.0f, 1.0f, 0.0f };
    Vec position = { 0.0f, 0.0f, 0.0f };
    M447GameState* object;
    s32 i;

    object = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*object), HU_MEMNUM_OVL);
    if (!object) {
        OSReport("M447FlowCreate ERROR...\n");
        return NULL;
    }
    Hu3DShadowCreate(30.0f, 20.0f, 5000.0f);
    Hu3DShadowTPLvlSet(0.45f);
    Hu3DShadowPosSet(&velocity, &rotation, &position);
    M447RandomSeedSet(frand());
    object->shutdownComplete = 0;
    M447CameraInit();
    stageState = M447StageCreate();
    blockPosition.x = 0.0f;
    blockPosition.y = -1000.0f;
    blockPosition.z = 0.0f;
    for (i = 0; i < 5; i++) {
        blockStates[i] = M447BlockCreate();
        blockStates[i]->position = blockPosition;
    }
    M447PlayerRegistryInit();
    for (i = 0; i < 2; i++) {
        playerStates[i] = M447PlayerCreate(i);
    }
    M447ComputerPlayerCountGet(object);
    M447HudCreate(object);
    object->gamePhase = 0;
    object->phaseStep = 0;
    object->roundTimer = 0;
    object->roundState = 0;
    object->roundNumber = 0;
    object->bonusRoundActive = 0;
    object->musicSequenceId = -1;
    object->resultWindowId = -1;
    object->countdownWindowId = -1;
    object->mainProcess = HuPrcChildCreate(M447GameProcess, 100, 0x4000, 0, objectManager);
    object->mainProcess->user_data = object;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    return object;
}

void M447GameDestroy(M447GameState* gameState) {
    M447HudDestroy(gameState);
    M447StageDestroy(stageState);
    M447CameraShutdown();
    HuPrcKill(gameState->mainProcess);
    HuMemDirectFree(gameState);
}

BOOL M447ExitReadyCheck(M447GameState* gameState) {
    return gameState->shutdownComplete == 0;
}

void M447GamePhaseSet(M447GameState* gameState, s32 phaseIndex) {
    gameState->gamePhase = phaseIndex;
    gameState->phaseStep = 0;
}

void (*gamePhaseHandlers[])(M447GameState*) = {
	M447IntroPhaseUpdate,
	M447RoundSetupUpdate,
	M447RoundPlayUpdate,
	M447BoardCleanupUpdate,
	M447GameResultUpdate
};

void M447GameProcess(void) {
    M447GameState* objectWork;

    objectWork = HuPrcCurrentGet()->user_data;
    while (omSysExitReq == 0 || !M447ExitReadyCheck(objectWork)) {
        gamePhaseHandlers[objectWork->gamePhase](objectWork);
        HuPrcVSleep();
    }
    omSysExitReq = 1;
    objectWork->shutdownComplete = 1;
    HuPrcEnd();
}

const Vec introCameraPositions[2] = {
    {   0.0f, 300.0f, 0.0f },
    { -0.12f, 176.0f, 0.0f }
};

void M447IntroPhaseUpdate(M447GameState* gameState) {
    s32 i;

    switch (gameState->phaseStep) {
        case 0:
            gameState->phaseTimer = 0;
            gameState->phaseStep = 1;
            break;
        case 1:
            WipeStatGet();
            gameState->phaseStep = 2;
            break;
        case 2:
            if (gameState->phaseTimer++ >= 60) {
                gameState->phaseStep = 3;
            }
            break;
        case 3:
            M447StageActionSet(stageState, 1);
            gameState->phaseTimer = 0;
            gameState->phaseStep = 4;
            break;
        case 4:
            if (!M447StageActionActiveCheck(stageState)) {
                gameState->phaseStep = 5;
            }
            break;
        case 5:
            if (playerStates[1]->characterIndex == 8) {
                gameState->musicSequenceId = HuAudSeqPlay(0x3D);
            }
            M447CameraZoomTargetSet(860.0f, 60);
            M447CameraPositionTargetSet(&introCameraPositions[1], 60);
            gameState->phaseTimer = 0;
            gameState->phaseStep = 6;
            break;
        case 6:
            if (gameState->phaseTimer++ >= 60) {
                if (playerStates[1]->characterIndex == 8) {
                    gameState->phaseStep = 7;
                } else {
                    gameState->phaseStep = 9;
                }
            }
            break;
        case 7:
            gameState->dialogProcess = HuPrcChildCreate(M447CharacterIntroDialog, 100, 0x2000, 0, objectManager);
            gameState->dialogProcess->user_data = gameState;
            gameState->dialogComplete = 1;
            gameState->phaseStep = 8;
            break;
        case 8:
            if (gameState->dialogComplete != 0) {
                break;
            }
            gameState->dialogProcess = NULL;
            gameState->phaseStep = 9;
            /* fallthrough */
        case 9:
            if (gameState->musicSequenceId != -1) {
                HuAudSeqFadeOut(gameState->musicSequenceId, 1000);
            }
            M447CameraZoomTargetSet(1150.0f, 60);
            M447CameraPositionTargetSet(&introCameraPositions[0], 60);
            for (i = 0; i < 2; i++) {
                M447PlayerStateSet(playerStates[i], 6);
            }
            M447GamePhaseSet(gameState, 1);
            break;
    }
}

void M447RoundSetupUpdate(M447GameState* gameState) {
    Vec rotation;
    Vec position;
    s32 cardIndex;
    s32 cardWork;
    s32 i;

    switch (gameState->phaseStep) {
        case 0:
            gameState->phaseStep = 1;
            /* fallthrough */
        case 1:
            M447StageActionSet(stageState, 3);
            gameState->phaseTimer = 0;
            gameState->phaseStep = 2;
            /* fallthrough */
        case 2:
            if (gameState->phaseTimer++ < 110) {
                break;
            }
            gameState->phaseStep = 3;
            /* fallthrough */
        case 3:
            M447BlockLayoutSet(M447RandomValueGet() % 10 > 5);
            cardIndex = gameState->roundNumber * 2 + M447RandomValueGet() % 2;
            cardWork = M447RandomValueGet() % 5;
            for (i = 0; i < 5; i++) {
                M447BlockCardSet(blockStates[i], cardIndex, cardWork == i);
                M447PlayerSpawnPositionGet(i, &position);
                blockStates[i]->position = position;
                rotation.x = M447RandomValueGet() % 360;
                rotation.y = 0.0f;
                rotation.z = M447RandomValueGet() % 360;
                blockStates[i]->rotation = rotation;
                M447BlockActionSet(blockStates[i], 1);
            }
            gameState->phaseTimer = 0;
            gameState->phaseStep = 4;
            /* fallthrough */
        case 4:
            if (gameState->phaseTimer++ < 60) {
                break;
            }
            for (i = 0; i < 5; i++) {
                M447BlockActionSet(blockStates[i], 2);
            }
            /* fallthrough */
            gameState->phaseStep = 5;
        case 5:
            M447StageActionSet(stageState, 4);
            gameState->phaseTimer = 0;
            gameState->phaseStep = 6;
            /* fallthrough */
        case 6:
            if (!M447StageActionActiveCheck(stageState)) {
                if (gameState->bonusRoundActive != 0) {
                    gameState->phaseStep = 9;
                } else {
                    gameState->phaseStep = 7;
                }
            }
            break;
        case 7:
            gameState->musicSequenceId = HuAudSeqPlay(0x3E);
            gameState->resultWindowId = MGSeqStartCreate();
            gameState->phaseStep = 8;
            /* fallthrough */
        case 8:
            if (MGSeqStatGet(gameState->resultWindowId) != 0) {
                break;
            }
            gameState->bonusRoundActive = 1;
            gameState->phaseStep = 9;
            /* fallthrough */
        case 9:
            M447GamePhaseSet(gameState, 2);
            break;
    }
}

void M447RoundPlayUpdate(M447GameState* gameState) {
    s32 cardValue;
    s32 stateIndex;
    s32 i;

    switch (gameState->phaseStep) {
        case 0:
            M447ScoreboardShow(gameState);
            gameState->roundTimer = REFRESH_RATE * 15;
            gameState->countdownWindowId = MGSeqCreate(1, gameState->roundTimer / REFRESH_RATE, -1, -1);
            for (i = 0; i < 2; i++) {
                M447PlayerStateSet(playerStates[i], 1);
                playerStates[i]->inputEnabled = 1;
            }
            gameState->phaseStep = 1;
            /* fallthrough */
        case 1:
            cardValue = M447UnrevealedPlayerCountGet(stageState);
            if (gameState->roundTimer > 0 && cardValue > 0 && --gameState->roundTimer % REFRESH_RATE == 0) {
                MGSeqParamSet(gameState->countdownWindowId, 1, gameState->roundTimer / REFRESH_RATE);
            }
            if (gameState->roundTimer > 0 && cardValue > 0) {
                break;
            }
            gameState->phaseStep = 2;
            /* fallthrough */
        case 2:
            for (i = 0; i < 2; i++) {
                playerStates[i]->inputEnabled = 0;
                M447PlayerDisplayOff(playerStates[i]);
            }
            gameState->phaseStep = 3;
            break;
        case 3:
            if (gameState->countdownWindowId != -1) {
                MGSeqParamSet(gameState->countdownWindowId, 2, -1);
                gameState->countdownWindowId = -1;
            }
            for (i = 0; i < 2; i++) {
                if (playerStates[i]->state != 1 && playerStates[i]->state != 0) {
                    return;
                }
            }
            for (i = 0; i < 5; i++) {
                if (blockStates[i]->actionState != 0 && blockStates[i]->actionState != 2) {
                    return;
                }
            }
            gameState->phaseStep = 4;
            /* fallthrough */
        case 4:
            for (i = 0; i < 2; i++) {
                if (playerStates[i]->roundWinFlag != 0) {
                    M447ScoreIconAnimate(gameState, i, playerStates[i]->scoreCount - 1);
                }
            }
            gameState->roundNumber++;
            for (i = 0, stateIndex = 0; i < 2; i++) {
                if (playerStates[i]->scoreCount > stateIndex) {
                    stateIndex = playerStates[i]->scoreCount;
                }
            }
            if (stateIndex >= 3 || gameState->roundNumber >= 5) {
                M447GamePhaseSet(gameState, 4);
            } else {
                M447GamePhaseSet(gameState, 3);
            }
            break;
    }
}

void M447BoardCleanupUpdate(M447GameState* gameState) {
    s32 i;

    switch (gameState->phaseStep) {
        case 0:
            for (i = 0; i < 2; i++) {
                M447PlayerStateSet(playerStates[i], 6);
            }
            gameState->phaseStep = 1;
            /* fallthrough */
        case 1:
            for (i = 0; i < 5; i++) {
                if (blockStates[i]->actionState != 0) {
                    M447BlockActionSet(blockStates[i], 4);
                }
            }
            gameState->phaseTimer = 0;
            gameState->phaseStep = 2;
            /* fallthrough */
        case 2:
            if (gameState->phaseTimer++ < 160) {
                break;
            }
            gameState->phaseStep = 3;
            /* fallthrough */
        case 3:
            M447GamePhaseSet(gameState, 1);
            break;
    }
}

const Vec resultCameraPosition = { 0.0f, 150.0f, 0.0f };
const Vec resultCameraLookDirection = { 6.0f,   0.0f, 0.0f };

void M447GameResultUpdate(M447GameState* gameState) {
    s32 cardIndex;
    s32 cardWork;
    s32 i;

    switch (gameState->phaseStep) {
        case 0:
            for (i = 0; i < 5; i++) {
                if (blockStates[i]->actionState != 0) {
                    M447BlockActionSet(blockStates[i], 4);
                }
            }
            gameState->phaseStep = 1;
            /* fallthrough */
        case 1:
            // Bug: HuAudSeqFadeOut takes two arguments.
            HuAudSeqFadeOut(gameState->musicSequenceId);
            gameState->resultWindowId = MGSeqCreate(3, 1);
            gameState->phaseStep = 2;
            /* fallthrough */
        case 2:
            if (MGSeqStatGet(gameState->resultWindowId) != 0) {
                break;
            }
            gameState->phaseStep = 3;
            /* fallthrough */
        case 3:
            for (i = 0; i < 2; i++) {
                M447PlayerStateSet(playerStates[i], 7);
            }
            M447CameraPositionTargetSet(&resultCameraPosition, 180);
            M447CameraZoomTargetSet(300.0f, 180);
            M447CameraLookTargetSet(&resultCameraLookDirection, 180);
            gameState->phaseTimer = 0;
            gameState->phaseStep = 4;
            /* fallthrough */
        case 4:
            if (gameState->phaseTimer++ < 180) {
                break;
            }
            gameState->phaseStep = 5;
            /* fallthrough */
        case 5:
            cardIndex = playerStates[0]->scoreCount;
            cardWork = playerStates[1]->scoreCount;
            if (playerStates[1]->characterIndex != 8 && cardIndex == cardWork) {
                M447PlayerStateSet(playerStates[0], 10);
                M447PlayerStateSet(playerStates[1], 10);
                HuAudSStreamPlay(4);
            } else if (cardIndex > cardWork) {
                M447PlayerStateSet(playerStates[0], 9);
                M447PlayerStateSet(playerStates[1], 10);
                GWPlayerCoinWinAdd(0, 10);
                HuAudSStreamPlay(1);
            } else {
                M447PlayerStateSet(playerStates[0], 10);
                M447PlayerStateSet(playerStates[1], 9);
                GWPlayerCoinWinAdd(1, 10);
                if (playerStates[1]->characterIndex == 8) {
                    HuAudSStreamPlay(3);
                } else {
                    HuAudSStreamPlay(1);
                }
            }
            gameState->phaseTimer = 0;
            gameState->phaseStep = 6;
            /* fallthrough */
        case 6:
            if (gameState->phaseTimer++ < 210) {
                break;
            }
            gameState->phaseStep = 7;
            /* fallthrough */
        case 7:
            omSysExitReq = 1;
            gameState->shutdownComplete = 1;
            break;
    }
}

void M447MessageWait(M447GameState* gameState, s16 windowId, s32 waitFrames) {
    HuWinComKeyReset();
    if (gameState->computerPlayerCount == 2) {
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, waitFrames);
    }
    winData[windowId].active_pad = 1;
    HuWinMesWait(windowId);
}

s32 M447ChoiceGet(M447GameState* gameState, s16 windowId, s32 choiceFlags, s32 computerChoice, s32 waitFrames) {
    if (gameState->computerPlayerCount == 2) {
        HuPrcSleep(waitFrames);
        return computerChoice;
    }
    return HuWinChoiceGet(windowId, choiceFlags);
}

const s32 introDialogueMessages[] = {
    MAKE_MESSID(42, 3),
    MAKE_MESSID(42, 4),
    MAKE_MESSID(42, 5),
    MAKE_MESSID(42, 6),
    MAKE_MESSID(42, 7),
    0
};

void M447CharacterIntroDialog(void) {
    M447GameState* gameState;
    s16 object;
    s16 cardIndex;
    s32 modelIndex;
    u32 cardValue;

    gameState = HuPrcCurrentGet()->user_data;
    object = HuWinExCreateStyled(-10000.0f, 40.0f, 280, 120, -1, 1);
    HuWinExAnimIn(object);
    HuWinBGTPLvlSet(object, 0.8f);
    HuWinMesSpeedSet(object, 1);
    HuWinMesColSet(object, 0);
    HuWinMesSet(object, MAKE_MESSID(42, 1));
    M447MessageWait(gameState, object, 60);
    if (gameState->computerPlayerCount < 2) {
        HuWinMesColSet(object, 0);
        HuWinMesSet(object, MAKE_MESSID(42, 2));
        cardIndex = M447ChoiceGet(gameState, object, 0, 1, 60);
    } else {
        cardIndex = 1;
    }
    if (cardIndex == 0) {
        modelIndex = 0;
        while ((cardValue = introDialogueMessages[modelIndex++]) != 0) {
            HuWinMesColSet(object, 0);
            HuWinMesSet(object, cardValue);
            M447MessageWait(gameState, object, 60);
        }
        HuWinMesColSet(object, 0);
        HuWinMesSet(object, MAKE_MESSID(42, 8));
        M447MessageWait(gameState, object, 60);
    } else {
        HuWinMesColSet(object, 0);
        HuWinMesSet(object, MAKE_MESSID(42, 9));
        M447MessageWait(gameState, object, 60);
    }
    HuWinExAnimOut(object);
    HuWinExCleanup(object);
    gameState->dialogComplete = 0;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

void M447HudCreate(M447GameState* gameState) {
    s32 i;

    for (i = 0; i < 25; i++) {
        gameState->hudSprites[i] = espEntry(hudAssetIds[i], 0, 0);
        espDispOff(gameState->hudSprites[i]);
        espPriSet(gameState->hudSprites[i], 100);
    }
    espPriSet(gameState->hudSprites[7], 99);
    espPriSet(gameState->hudSprites[8], 99);
    espPriSet(gameState->hudSprites[9], 99);
    espPriSet(gameState->hudSprites[13], 99);
    espPriSet(gameState->hudSprites[14], 99);
    espPriSet(gameState->hudSprites[15], 99);
}

void M447HudDestroy(M447GameState* gameState) {
    s32 i;

    for (i = 0; i < 25; i++) {
        espKill(gameState->hudSprites[i]);
    }
}

void M447ScoreboardShow(M447GameState* gameState) {
    s16 cardWork;
    s32 position;
    s32 i;
    s32 j;

    espDispOn(gameState->hudSprites[0]);
    espPosSet(gameState->hudSprites[0], 96.0f, 64.0f);
    espDispOn(gameState->hudSprites[2]);
    espPosSet(gameState->hudSprites[2], 96.0f, 63.0f);
    espTPLvlSet(gameState->hudSprites[2], 0.5f);
    espColorSet(gameState->hudSprites[2], 0, 0, 0);
    espDispOn(gameState->hudSprites[1]);
    espPosSet(gameState->hudSprites[1], 480.0f, 64.0f);
    espDispOn(gameState->hudSprites[3]);
    espPosSet(gameState->hudSprites[3], 480.0f, 63.0f);
    espTPLvlSet(gameState->hudSprites[3], 0.5f);
    espColorSet(gameState->hudSprites[3], 0, 0, 0);
    for (i = 0; i < 2; i++) {
        cardWork = playerStates[i]->characterIndex + 16;
        espDispOn(gameState->hudSprites[cardWork]);
        espPosSet(gameState->hudSprites[cardWork], i * 384 + 46, 63.0f);
    }
    for (j = 0; j < 2; j++) {
        position = playerStates[j]->scoreCount;
        for (i = 0; i < 3; i++) {
            cardWork = i + 4 + j * 6;
            espDispOn(gameState->hudSprites[cardWork]);
            espPosSet(gameState->hudSprites[cardWork], j * 384 + 80 + i * 32, 63.0f);
        }
    }
}

void M447ScoreIconAnimate(M447GameState* gameState, s32 playerIndex, s32 scoreIndex) {
    M447TransitionState* object;
    HUPROCESS* playerIndex;

    object = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*object), HU_MEMNUM_OVL);
    object->spriteId = gameState->hudSprites[scoreIndex + 7 + playerIndex * 6];
    espPosSet(object->spriteId, playerIndex * 384 + 80 + scoreIndex * 32, 63.0f);
    object->scaleProgress = 0.0f;
    object->scaleStep = 0.05f;
    playerIndex = HuPrcChildCreate(M447ScoreIconAnimationProcess, 100, 0x1000, 0, objectManager);
    playerIndex->user_data = object;
}

void M447ScoreIconAnimationProcess(void) {
    float distance;
    M447TransitionState* objectWork;

    objectWork = HuPrcCurrentGet()->user_data;
    espDispOn(objectWork->spriteId);
    while (TRUE) {
        if ((objectWork->scaleProgress += objectWork->scaleStep) > 1.0f) {
            objectWork->scaleProgress = 1.0f;
            break;
        }
        distance = 1.0f + 1.5f * (1.0f - objectWork->scaleProgress);
        espScaleSet(objectWork->spriteId, distance, distance);
        HuPrcVSleep();
    }
    espScaleSet(objectWork->spriteId, 1.0f, 1.0f);
    HuAudFXPlay(9);
    HuMemDirectFree(objectWork);
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

void M447HudHide(M447GameState* gameState) {
    s32 i;

    for (i = 0; i <= 25; i++) {
        espDispOff(gameState->hudSprites[i]);
    }
}

void M447ComputerPlayerCountGet(M447GameState* gameState) {
    s32 i;

    gameState->computerPlayerCount = 0;
    for (i = 0; i < 2; i++) {
        if (playerStates[i]->isComputer != 0) {
            gameState->computerPlayerCount++;
        }
    }
}

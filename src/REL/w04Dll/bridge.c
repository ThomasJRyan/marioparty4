#include "REL/w04Dll.h"
#include "game/audio.h"
#include "game/data.h"
#include "game/gamework_data.h"
#include "game/memory.h"
#include "game/objsub.h"
#include "game/process.h"
#include "game/window.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/window.h"

#include "dolphin.h"
#include "ext_math.h"

static void FadeW04BridgeRail(s16 reveal, s16 durationFrames);
static void AnimateW04BridgeToggle(void);
static void SetW04BridgeSpaceFlags(s16 bridgeIsOpen);
static void UpdateW04BridgeModels(void);
static void AnimateW04BridgeModel(void);
static void CleanupW04BridgeModel(void);
static void FinishW04BridgeEvent(void);

static HUPROCESS *bridgeVisualProcesses[3];
static HUPROCESS *bridgeEventProcess;

static s32 bridgeModelResources[] = {
    DATA_MAKE_NUM(DATADIR_W04, 32),
    -1
};

void W04BridgeSetup(void) {
    Vec bridgePosition;
    Vec modelScale;
    s16 *bridgeProcessData;
    s32 spaceFlagMask;
    s32 i;

    modelScale.x = modelScale.y = modelScale.z = 1.5f;
    for (i = 0; i < 3; i++) {
        bridgeVisualProcesses[i] = HuPrcChildCreate(AnimateW04BridgeModel, 0x2003, 0x1000, 0, boardMainProc);
        HuPrcDestructorSet2(bridgeVisualProcesses[i], CleanupW04BridgeModel);
        bridgeProcessData = HuMemDirectMallocNum(HEAP_HEAP, 5 * sizeof(s16), HU_MEMNUM_OVL);
        bridgeVisualProcesses[i]->user_data = bridgeProcessData;
        bridgeProcessData[0] = i;
        bridgeProcessData[1] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 31), bridgeModelResources, 1);
        BoardModelScaleSetV(bridgeProcessData[1], &modelScale);
        bridgeProcessData[2] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 33), NULL, 1);
        BoardModelAttrSet(bridgeProcessData[2], 0x40000001);
        BoardModelHookSet(bridgeProcessData[1], "itemhook_R", bridgeProcessData[2]);
        BoardModelMotionStart(bridgeProcessData[1], 1, 0x40000001);
        spaceFlagMask = (0x8000 >> i);
        spaceFlagMask |= 0x1000;
        bridgeProcessData[3] = BoardSpaceFlagSearch(0, spaceFlagMask);
        spaceFlagMask = (0x8000 >> i);
        spaceFlagMask |= 0x800;
        bridgeProcessData[4] = BoardSpaceFlagSearch(0, spaceFlagMask);
        BoardSpacePosGet(0, bridgeProcessData[3], &bridgePosition);
        BoardModelPosSetV(bridgeProcessData[1], &bridgePosition);
    }
    modelScale.x = modelScale.y = modelScale.z = 2.0f;
    BoardModelScaleSetV(w04BoardModelIds[11], &modelScale);
    UpdateW04BridgeModels();
    if (w04BoardEventState->bridgeState & 1) {
        BoardModelMotionStartEndSet(w04BoardModelIds[5], 0, 180);
        BoardModelMotionTimeSet(w04BoardModelIds[5], 108.0f);
        FadeW04BridgeRail(0, 1);
        BoardModelAttrSet(w04BoardModelIds[5], 0x40000002);
        return;
    }
    BoardModelAttrReset(w04BoardModelIds[5], 0x40000002);
    BoardModelMotionStartEndSet(w04BoardModelIds[5], 0, 60);
    BoardModelMotionTimeSet(w04BoardModelIds[5], 0.0f);
    FadeW04BridgeRail(1, 1);
}

void W04BridgeCleanup(void) {
    s32 i;

    for (i = 0; i < 3; i++) {
        HuPrcKill(bridgeVisualProcesses[i]);
    }
}

static s32 bridgeToggleMotionIds[8] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 28),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 28),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 28),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 28),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 28),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 28),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 28),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 28)
};

void W04BridgeToggleEvent(void) {
    s32 currentPlayer;
    s16 playerMotionId;

    currentPlayer = GWSystem.player_curr;
    playerMotionId = BoardPlayerMotionCreate(currentPlayer, bridgeToggleMotionIds[GWPlayer[currentPlayer].character]);
    BoardRollDispSet(0);
    BoardPlayerMotBlendSet(currentPlayer, 180, 15);
    BoardCameraViewSet(2);
    while (!BoardPlayerMotBlendCheck(currentPlayer)) {
        HuPrcVSleep();
    }
    BoardCameraMotionWait();
    w04BoardEventState->bridgeState ^= 1;
    AnimateW04BridgeToggle();
    UpdateW04BridgeModels();
    PlayerFXPlay(currentPlayer, 0x128);
    BoardPlayerMotionStart(currentPlayer, playerMotionId, 0);
    while (!BoardPlayerMotionEndCheck(currentPlayer)) {
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    if (!(w04BoardEventState->bridgeState & 1)) {
        BoardWinCreate(2, MAKE_MESSID(32, 24), 11);
        BoardWinWait();
        BoardWinKill();
    }
    BoardPlayerMotionShiftSet(currentPlayer, 1, 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
    BoardCameraViewSet(1);
    BoardCameraMotionWait();
    BoardRollDispSet(1);
    BoardPlayerMotionKill(currentPlayer, playerMotionId);
    HuPrcKill(NULL);
    while (TRUE) {
        HuPrcVSleep();
    }
}

static void FadeW04BridgeRail(s16 reveal, s16 durationFrames) {
    float alphaStep;
    float railAlpha;
    s32 i;

    if (reveal != 0) {
        railAlpha = 0.0f;
        alphaStep = 255.0f / durationFrames;
    } else {
        railAlpha = 255.0f;
        alphaStep = -255.0f / durationFrames;
    }
    for (i = 0; i < durationFrames; i++) {
        BoardModelAlphaSet(w04BoardModelIds[11], railAlpha);
        railAlpha += alphaStep;
        HuPrcVSleep();
    }
    if (reveal != 0) {
        BoardModelAlphaSet(w04BoardModelIds[11], 0xFF);
    } else {
        BoardModelAlphaSet(w04BoardModelIds[11], 0);
    }
}

static void AnimateW04BridgeToggle(void) {
    float closeRailAlpha;
    float closeAlphaStep;
    float openRailAlpha;
    float openAlphaStep;
    s32 frame;

    HuAudFXPlay(0x4B);
    if (w04BoardEventState->bridgeState & 1) {
        BoardWinCreate(2, MAKE_MESSID(32, 23), 11);
        BoardWinWait();
        BoardWinKill();
        omVibrate(GWSystem.player_curr, 12, 6, 6);
        HuAudFXPlay(0x45E);
        BoardModelMotionStartEndSet(w04BoardModelIds[5], 0, 180);
        BoardModelMotionTimeSet(w04BoardModelIds[5], 59.0f);
        closeRailAlpha = 255.0f;
        closeAlphaStep = -5.1f;
        for (frame = 0; frame < 50; frame++) {
            BoardModelAlphaSet(w04BoardModelIds[11], closeRailAlpha);
            closeRailAlpha += closeAlphaStep;
            HuPrcVSleep();
        }
        BoardModelAlphaSet(w04BoardModelIds[11], 0);
        BoardModelAttrSet(w04BoardModelIds[5], 0x40000002);
    } else {
        omVibrate(GWSystem.player_curr, 12, 6, 6);
        HuAudFXPlay(0x45D);
        BoardModelAttrReset(w04BoardModelIds[5], 0x40000002);
        BoardModelMotionStartEndSet(w04BoardModelIds[5], 0, 180);
        BoardModelMotionTimeSet(w04BoardModelIds[5], 110.0f);
        openRailAlpha = 0.0f;
        openAlphaStep = 3.642857f;
        for (frame = 0; frame < 70; frame++) {
            BoardModelAlphaSet(w04BoardModelIds[11], openRailAlpha);
            openRailAlpha += openAlphaStep;
            HuPrcVSleep();
        }
        BoardModelAlphaSet(w04BoardModelIds[11], 0xFF);
        BoardModelMotionStartEndSet(w04BoardModelIds[5], 0, 60);
    }
}

static void SetW04BridgeSpaceFlags(s16 bridgeIsOpen) {
    BoardSpace *boardSpace;
    s32 i;

    for (i = 0; i < BoardSpaceCountGet(0); i++) {
        boardSpace = BoardSpaceGet(0, i + 1);
        if (boardSpace->flag & 0x20) {
            if (bridgeIsOpen != 0) {
                boardSpace->flag |= 0x04000000;
            } else {
                boardSpace->flag &= ~0x04000000;
            }
        }
    }
}

static void UpdateW04BridgeModels(void) {
    s16 *shortValues;
    s32 i;

    if (w04BoardEventState->bridgeState == 0) {
        BoardModelMotionStartEndSet(w04BoardModelIds[3], 0, 80);
        BoardModelMotionTimeSet(w04BoardModelIds[3], 0.0f);
        BoardModelVisibilitySet(w04BoardModelIds[0], 1);
        BoardModelVisibilitySet(w04BoardModelIds[1], 1);
        BoardModelVisibilitySet(w04BoardModelIds[2], 1);
        SetW04BridgeSpaceFlags(0);
        for (i = 0; i < 3; i++) {
            shortValues = bridgeVisualProcesses[i]->user_data;
            BoardModelMotionStartEndSet(shortValues[2], 0, 20);
        }
    } else {
        BoardModelMotionStartEndSet(w04BoardModelIds[3], 100, 100);
        BoardModelMotionTimeSet(w04BoardModelIds[3], 100.0f);
        BoardModelVisibilitySet(w04BoardModelIds[0], 0);
        BoardModelVisibilitySet(w04BoardModelIds[1], 0);
        BoardModelVisibilitySet(w04BoardModelIds[2], 0);
        SetW04BridgeSpaceFlags(1);
        for (i = 0; i < 3; i++) {
            shortValues = bridgeVisualProcesses[i]->user_data;
            BoardModelMotionStartEndSet(shortValues[2], 25, 25);
        }
    }
}

static void AnimateW04BridgeModel(void) {
    Vec movementStep;
    Vec bridgePosition;
    Vec nextSpacePosition;
    float bridgeAngle;
    s16 *shortValues;
    s16 startSpace;
    s16 endSpace;
    s16 i;

    shortValues = HuPrcCurrentGet()->user_data;
    startSpace = shortValues[3];
    endSpace = shortValues[4];
    while (TRUE) {
        BoardSpacePosGet(0, startSpace, &bridgePosition);
        BoardSpacePosGet(0, endSpace, &nextSpacePosition);
        VECSubtract(&nextSpacePosition, &bridgePosition, &movementStep);
        VECScale(&movementStep, &movementStep, 0.008333334f);
        bridgeAngle = Atan2d(nextSpacePosition.x - bridgePosition.x, nextSpacePosition.z - bridgePosition.z);
        BoardModelPosGet(shortValues[1], &bridgePosition);
        while (W04ModelRotateYToAngle(shortValues[1], bridgeAngle, 2.5f) == 0) {
            if (w04BoardEventState->bridgeState == 0) {
                BoardModelAlphaSet(shortValues[1], 0xFF);
            } else {
                BoardModelAlphaSet(shortValues[1], 0);
            }
            HuPrcVSleep();
        }
        for (i = 0; i < 120; i++) {
            if (w04BoardEventState->bridgeState == 0) {
                BoardModelAlphaSet(shortValues[1], 0xFF);
            } else {
                BoardModelAlphaSet(shortValues[1], 0);
            }
            VECAdd(&bridgePosition, &movementStep, &bridgePosition);
            BoardModelPosSetV(shortValues[1], &bridgePosition);
            HuPrcVSleep();
        }
        i = startSpace;
        startSpace = endSpace;
        endSpace = i;
    }
}

static void CleanupW04BridgeModel(void) {
    s16 *shortValues;

    shortValues = HuPrcCurrentGet()->user_data;
    BoardModelKill(shortValues[1]);
    HuMemDirectFree(shortValues);
}

static void FinishW04BridgeEvent(void) {
    bridgeEventProcess = NULL;
}

void W04BridgeWalkEvent(void) {
    s32 currentPlayerIndex;

    currentPlayerIndex = GWSystem.player_curr;
    bridgeEventProcess = HuPrcChildCreate(W04BridgeToggleEvent, 0x2003, 0x2000, 0, boardMainProc);
    HuPrcDestructorSet2(bridgeEventProcess, FinishW04BridgeEvent);
    while (bridgeEventProcess) {
        HuPrcVSleep();
    }
}

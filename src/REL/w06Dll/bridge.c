#include "REL/w06Dll.h"
#include "game/audio.h"
#include "game/data.h"
#include "game/gamework_data.h"
#include "game/object.h"
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

typedef struct {
    struct {
        u8 isActive : 1;
        u8 isCompleted : 1;
        u8 isMoving : 1;
        u8 isVisible : 1;
        u8 isInitialized : 1;
    };
    u8 delayFrames;
    s8 bridgeState;
    s8 crossingMode;
    s16 motionId;
    char padding[2];
    float bridgeMotionTime;
} W06BridgeWork;

static void W06BridgeCrossingProcess(void);
static void W06BridgeCrossingCleanup(void);
static s32 W06BridgePlayerCross(s32 playerIndex, s8 crossingMode);
static void W06BridgeObjectUpdate(OMOBJ *bridgeObject);

static s8 bridgeCrossingType;
static s16 bridgeTargetSpace;
static s16 bridgeLandingSpace;
static s16 bridgeSpaceLinks[3][2];
static s16 bridgeModels[3];
static OMOBJ *bridgeObject;
static HUPROCESS *bridgeProcess;

static s16 bridgePlayerMotion = -1;

static float bridgeRotations[][2] = {
    {  90.0f, 180.0f },
    { -90.0f,   0.0f },
    { 180.0f,   0.0f }
};

static s32 bridgeSpaceFlags[][2] = {
    { 0x00000200, 0x00000100 },
    { 0x00000080, 0x00000040 },
    { 0x00000800, 0x00000400 }
};

static s32 bridgeModelData[] = {
    DATA_MAKE_NUM(DATADIR_W06, 3),
    DATA_MAKE_NUM(DATADIR_W06, 3),
    DATA_MAKE_NUM(DATADIR_W06, 4)
};

static s32 bridgeWalkMotions[8] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 26),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 26),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 26),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 26),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 26),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 26),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 26),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 26),
};

static s32 bridgeRunMotions[8] = {
    DATA_MAKE_NUM(DATADIR_W06, 30),
    DATA_MAKE_NUM(DATADIR_W06, 31),
    DATA_MAKE_NUM(DATADIR_W06, 32),
    DATA_MAKE_NUM(DATADIR_W06, 33),
    DATA_MAKE_NUM(DATADIR_W06, 34),
    DATA_MAKE_NUM(DATADIR_W06, 35),
    DATA_MAKE_NUM(DATADIR_W06, 36),
    DATA_MAKE_NUM(DATADIR_W06, 37)
};

void W06BridgeCreate(void) {
    Vec bridgePosition;
    float bridgeMotionFrame;
    BoardSpace *linkedSpace;
    s16 flaggedSpaceIndex;
    s32 bridgeSideIndex;
    s32 i;

    if (GWSystem.star_pos < 4) {
        w06BoardData->isMoving = 0;
    } else {
        w06BoardData->isMoving = 1;
    }
    for (i = 0; i < 3; i++) {
        bridgeModels[i] = BoardModelCreate(bridgeModelData[i], NULL, 0);
        switch (i) {
            case 0:
                bridgeSideIndex = (w06BoardData->isActive != 0) ? 1 : 0;
                OSs8tof32(&w06BoardData->playerIndex, &bridgeMotionFrame);
                break;
            case 1:
                bridgeSideIndex = (w06BoardData->isCompleted != 0) ? 1 : 0;
                OSs8tof32(&w06BoardData->bridgeState, &bridgeMotionFrame);
                break;
            case 2:
                bridgeSideIndex = (w06BoardData->isMoving != 0) ? 1 : 0;
                bridgeMotionFrame = 0.0f;
                break;
        }
        flaggedSpaceIndex = BoardSpaceFlagPosGet(0, bridgeSpaceFlags[i][bridgeSideIndex], &bridgePosition);
        BoardModelPosSetV(bridgeModels[i], &bridgePosition);
        BoardModelRotYSet(bridgeModels[i], bridgeRotations[i][bridgeSideIndex]);
        BoardModelMotionShiftSet(bridgeModels[i], 0, 60.0f * bridgeMotionFrame, 0.0f, HU3D_MOTATTR_NONE);
        HuPrcSleep(2);
        BoardModelMotionSpeedSet(bridgeModels[i], 0.0f);
        bridgeSpaceLinks[i][bridgeSideIndex] = flaggedSpaceIndex;
        flaggedSpaceIndex = BoardSpaceFlagSearch(0, bridgeSpaceFlags[i][bridgeSideIndex ^ 1]);
        linkedSpace = BoardSpaceGet(0, flaggedSpaceIndex);
        linkedSpace->flag |= 0x04000000;
        bridgeSpaceLinks[i][bridgeSideIndex ^ 1] = flaggedSpaceIndex;
    }
}

void W06BridgeDestroy(void) {
    s32 i;

    for (i = 0; i < 3; i++) {
        BoardModelKill(bridgeModels[i]);
        bridgeModels[i] = -1;
    }
}

void W06BridgeStateUpdate(void) {
    Vec bridgePosition;
    float bridgeMotionFrame;
    BoardSpace *linkedSpace;
    s16 bridgeSideIndex;
    s16 flaggedSpaceIndex;
    s16 bridgeIndex;

    if (GWSystem.star_pos < 4) {
        w06BoardData->isMoving = 0;
    } else {
        w06BoardData->isMoving = 1;
    }
    bridgeIndex = 2;
    bridgeSideIndex = (w06BoardData->isMoving != 0) ? 1 : 0;
    bridgeMotionFrame = 0.0f;
    flaggedSpaceIndex = BoardSpaceFlagPosGet(0, bridgeSpaceFlags[bridgeIndex][bridgeSideIndex], &bridgePosition);
    BoardModelPosSetV(bridgeModels[bridgeIndex], &bridgePosition);
    BoardModelRotYSet(bridgeModels[bridgeIndex], bridgeRotations[bridgeIndex][bridgeSideIndex]);
    BoardModelMotionShiftSet(bridgeModels[bridgeIndex], 0, 60.0f * bridgeMotionFrame, 0.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(2);
    BoardModelMotionSpeedSet(bridgeModels[bridgeIndex], 0.0f);
    bridgeSpaceLinks[bridgeIndex][bridgeSideIndex] = flaggedSpaceIndex;
    flaggedSpaceIndex = BoardSpaceFlagSearch(0, bridgeSpaceFlags[bridgeIndex][bridgeSideIndex]);
    linkedSpace = BoardSpaceGet(0, flaggedSpaceIndex);
    linkedSpace->flag &= ~0x04000000;
    flaggedSpaceIndex = BoardSpaceFlagSearch(0, bridgeSpaceFlags[bridgeIndex][bridgeSideIndex ^ 1]);
    linkedSpace = BoardSpaceGet(0, flaggedSpaceIndex);
    linkedSpace->flag |= 0x04000000;
    bridgeSpaceLinks[bridgeIndex][bridgeSideIndex ^ 1] = flaggedSpaceIndex;
}

void W06BridgeCrossingStart(s32 crossingType, s16 spaceIndex) {
    BoardRollDispSet(0);
    bridgeCrossingType = crossingType;
    bridgeTargetSpace = spaceIndex;
    bridgeProcess = HuPrcChildCreate(W06BridgeCrossingProcess, 0x2003, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(bridgeProcess, W06BridgeCrossingCleanup);
    while (bridgeProcess) {
        HuPrcVSleep();
    }
    BoardRollDispSet(1);
}

static void W06BridgeCrossingProcess(void) {
    s32 currentPlayerIndex;
    s8 *crossingCounter;
    BoardSpace *targetSpace;

    currentPlayerIndex = GWSystem.player_curr;
    if (bridgeCrossingType != 0) {
        crossingCounter = &w06BoardData->bridgeState;
    } else {
        crossingCounter = &w06BoardData->playerIndex;
    }
    targetSpace = BoardSpaceGet(0, bridgeTargetSpace);
    bridgeLandingSpace = targetSpace->link[0];
    GWPlayer[currentPlayerIndex].space_curr = bridgeLandingSpace;
    W06BridgePlayerCross(currentPlayerIndex, *crossingCounter);
    (*crossingCounter)++;
    if (*crossingCounter >= 3) {
        *crossingCounter = 0;
    }
    HuPrcEnd();
}

static void W06BridgeCrossingCleanup(void) {
    if (bridgePlayerMotion != -1) {
        BoardPlayerMotionKill(GWSystem.player_curr, bridgePlayerMotion);
        bridgePlayerMotion = -1;
    }
    bridgeProcess = NULL;
}

static s32 W06BridgePlayerCross(s32 playerIndex, s8 crossingMode) {
    Vec landingPosition;
    Vec playerPosition;
    Vec crossingDirection;
    float facingAngle;
    float movementDuration;
    float movementSpeed;
    s32 characterId;
    OMOBJ *crossingObject;
    W06BridgeWork *crossingWork;
    s8 vibrationFrame;

    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardSpacePosGet(0, bridgeLandingSpace, &landingPosition);
    VECSubtract(&landingPosition, &playerPosition, &crossingDirection);
    facingAngle = atan2d(crossingDirection.x, crossingDirection.z);
    BoardPlayerMotBlendSet(playerIndex, facingAngle, 0xF);
    while (!BoardPlayerMotBlendCheck(playerIndex)) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(playerIndex);
    HuPrcSleep(11);
    characterId = GWPlayer[playerIndex].character;
    switch (crossingMode) {
        case 0:
            bridgePlayerMotion = -1;
            movementDuration = 60.0f;
            movementSpeed = 1.0f;
            break;
        case 1:
            bridgePlayerMotion = BoardPlayerMotionCreate(playerIndex, bridgeRunMotions[characterId]);
            movementDuration = 50.0f;
            movementSpeed = 2.5f;
            if (GWPlayer[playerIndex].bowser_suit) {
                BoardPlayerIdleSet(playerIndex);
                HuPrcSleep(30);
            } else {
                BoardPlayerMotionShiftSet(playerIndex, bridgePlayerMotion, 0.0f, 6.0f, HU3D_MOTATTR_NONE);
                BoardPlayerMotionSpeedSet(playerIndex, 1.5f);
                HuPrcSleep(60);
            }
            break;
        case 2:
            bridgePlayerMotion = BoardPlayerMotionCreate(playerIndex, bridgeWalkMotions[characterId]);
            movementDuration = 40.0f;
            movementSpeed = 2.0f;
            if (GWPlayer[playerIndex].bowser_suit) {
                BoardPlayerIdleSet(playerIndex);
                HuPrcSleep(30);
            } else {
                BoardPlayerMotionShiftSet(playerIndex, bridgePlayerMotion, 0.0f, 6.0f, HU3D_MOTATTR_LOOP);
                BoardPlayerMotionSpeedSet(playerIndex, 3.0f);
                HuPrcSleep(60);
            }
            break;
    }
    crossingObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W06BridgeObjectUpdate);
    bridgeObject = crossingObject;
    crossingWork = omObjGetWork(crossingObject, W06BridgeWork);
    crossingWork->isActive = 0;
    crossingWork->isCompleted = 0;
    crossingWork->isMoving = 0;
    crossingWork->isVisible = 0;
    crossingWork->isInitialized = 0;
    crossingWork->delayFrames = 0;
    crossingWork->crossingMode = crossingMode;
    crossingWork->bridgeState = 0;
    crossingWork->motionId = bridgeModels[bridgeCrossingType];
    OSs8tof32(&crossingMode, &crossingWork->bridgeMotionTime);
    crossingWork->bridgeMotionTime = 60.0f * (crossingWork->bridgeMotionTime + 1.0f);
    BoardPlayerPosLerpStart(playerIndex, &playerPosition, &landingPosition, movementDuration);
    BoardPlayerMotionSpeedSet(playerIndex, movementSpeed);
    for (vibrationFrame = 0; GWPlayer[playerIndex].moving; vibrationFrame++) {
        if (!(vibrationFrame & 0xF)) {
            omVibrate(playerIndex, 12, 6, 6);
        }
        HuPrcVSleep();
    }
    facingAngle -= 180.0f;
    BoardPlayerMotBlendSet(playerIndex, facingAngle, 0xF);
    while (!BoardPlayerMotBlendCheck(playerIndex)) {
        HuPrcVSleep();
    }
    while (bridgeObject) {
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    if (crossingMode == 2) {
        BoardWinCreate(0, MAKE_MESSID(48, 40), -1);
        BoardWinWait();
        BoardWinKill();
    }
    BoardCameraTargetPlayerSet(playerIndex);
    BoardCameraViewSet(1);
    facingAngle += 180.0f;
    BoardPlayerMotBlendSet(playerIndex, facingAngle, 0xF);
    while (!BoardPlayerMotBlendCheck(playerIndex)) {
        HuPrcVSleep();
    }
    BoardCameraMotionWait();
    if (crossingMode == 1) {
        BoardWinCreate(0, MAKE_MESSID(48, 15), -1);
        BoardWinWait();
        BoardWinKill();
    }
    return 0;
}

static void W06BridgeObjectUpdate(OMOBJ *bridgeObject) {
    Vec currentBridgePosition;
    u8 bridgeFrame;
    BoardSpace *currentSpace;
    W06BridgeWork *bridgeWork;

    bridgeWork = omObjGetWork(bridgeObject, W06BridgeWork);
    if (bridgeWork->isActive != 0 || BoardIsKill()) {
        bridgeObject = NULL;
        omDelObjEx(HuPrcCurrentGet(), bridgeObject);
        return;
    }
    if (bridgeWork->delayFrames != 0) {
        bridgeWork->delayFrames--;
        return;
    }
    if (bridgeWork->isCompleted != 0 && bridgeWork->isInitialized == 0) {
        BoardModelMotionStart(bridgeWork->motionId, 0, 0);
        BoardModelMotionTimeSet(bridgeWork->motionId, bridgeWork->bridgeMotionTime - 60.0f);
        BoardModelMotionSpeedSet(bridgeWork->motionId, 1.0f);
        bridgeWork->isInitialized = 1;
    }
    if (bridgeWork->isMoving != 0) {
        HuAudFXPlay(0x49C);
        bridgeWork->isMoving = 0;
    }
    if (bridgeWork->isVisible != 0) {
        HuAudFXPlay(0x49D);
        bridgeWork->isVisible = 0;
    }
    switch (bridgeWork->bridgeState) {
        case 0:
            if (bridgeWork->isCompleted == 0) {
                if (GWPlayer[GWSystem.player_curr].bowser_suit != 0) {
                    bridgeWork->delayFrames = 72;
                } else {
                    bridgeWork->delayFrames = 30;
                }
                bridgeWork->isMoving = 1;
                bridgeWork->isCompleted = 1;
                break;
            }
            if (BoardModelMotionTimeGet(bridgeWork->motionId) >= bridgeWork->bridgeMotionTime) {
                BoardModelMotionTimeSet(bridgeWork->motionId, bridgeWork->bridgeMotionTime);
                BoardModelMotionSpeedSet(bridgeWork->motionId, 0.0f);
                if (bridgeWork->crossingMode == 2) {
                    bridgeWork->delayFrames = 30;
                    bridgeWork->bridgeState = 1;
                    bridgeWork->isVisible = 1;
                } else {
                    bridgeWork->isActive = 1;
                }
            }
            break;
        case 1:
            if (bridgeCrossingType == 0) {
                bridgeSideIndex = w06BoardData->isActive;
                w06BoardData->isActive ^= 1;
            } else {
                bridgeSideIndex = w06BoardData->isCompleted;
                w06BoardData->isCompleted ^= 1;
            }
            currentSpace = BoardSpaceGet(0, bridgeSpaceLinks[bridgeCrossingType][bridgeSideIndex]);
            currentSpace->flag |= 0x04000000;
            currentSpace = BoardSpaceGet(0, bridgeSpaceLinks[bridgeCrossingType][bridgeSideIndex ^ 1]);
            currentSpace->flag &= ~0x04000000;
            BoardSpacePosGet(0, bridgeSpaceLinks[bridgeCrossingType][bridgeSideIndex ^ 1], &currentBridgePosition);
            BoardModelMotionStart(bridgeWork->motionId, 0, 0);
            BoardModelMotionTimeSet(bridgeWork->motionId, 180.0f);
            BoardModelPosSetV(bridgeWork->motionId, &currentBridgePosition);
            BoardModelRotYSet(bridgeWork->motionId, bridgeRotations[bridgeCrossingType][bridgeSideIndex ^ 1]);
            BoardCameraTargetModelSet(bridgeWork->motionId);
            bridgeWork->bridgeState = 2;
            break;
        case 2:
            if (BoardModelMotionEndCheck(bridgeWork->motionId) && BoardCameraMotionIsDone()) {
                bridgeWork->isActive = 1;
            }
            break;
    }
}

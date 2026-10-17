#include "REL/m424Dll.h"

#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/EnvelopeExec.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/pad.h"
#include "version.h"

#include "math.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/frand.h"
#endif

// STRUCT
typedef struct M424ClawState {
    s16 state;
    s16 characterId;
    s16 elapsedFrames;
    s16 animationState;
    s16 velocity;
    s16 actionTimer;
    s16 position;
    s16 motionIndex;
    s16 motionTimer;
    s16 effectState;
    s16 targetPosition;
    s16 collisionState;
    s16 targetRotation;
    f32 positionX;
    f32 positionY;
    f32 targetScale;
    f32 motionFrame;
    f32 velocityY;
    Vec velocityZ;
    Vec playerPositions[12];
    Vec targetPositions[12];
    s32 clawModel;
    s32 shadowModel;
    s32 isAnimating;
    s32 actionState;
    s32 carriedBallIndex;
    s32 ballControllerState;
    Vec collisionTimer;
    f32 motionState;
    s32 soundEffectId;
    s32 effectTimer;
    s32 animationFrame;
    s32 animationFrameIndex;
    s32 animationDuration;
    s32 turnState;
    s32 turnTimer;
    s32 playerIndex;
    s32 motionCharacterId;
    f32 clawFlags;
    s32 animationFrameCounter;
} M424ClawState; // sizeof 0x1B0

typedef struct M424ClawMotionState {
    s32 state;
    f32 elapsedFrames;
    f32 velocity;
    f32 position;
} M424ClawMotionState; // sizeof 0x10

typedef struct M424ClawPlayerState {
    s32 state;
    s32 elapsedFrames;
} M424ClawPlayerState; // sizeof 0x8

// BSS
HUPROCESS* clawObjectManager;
OMOBJ* clawObject;
s32 unusedClawValues[2];
s32 clawPlayerCount;
s32 clawJointFound;
s32 useCurrentModelMotion;
s32 clawJointDebugEnabled;
M424ClawPlayerState clawPlayerSoundTimers[0x10];

// DATA
Vec clawPositionConfigs[4] = {
    { 950.0f, 275.0f, 0.0f },
    { 900.0f, 360.0f, 320.0f },
    { 494.99996f, 220.0f, 494.99996f },
    { 0.0f, 225.0f, 0.0f },
};
f32 clawMovementSpeeds[3] = { 12.0f, 11.0f, 17.5f };
char clawJointNames[12][0x10] = {
    "tsume10",
    "tsume11",
    "tsume12",
    "tsume20",
    "tsume21",
    "tsume22",
    "tsume30",
    "tsume31",
    "tsume32",
    "tsume40",
    "tsume41",
    "tsume42",
};
u8 clawCollisionFlags[12] = { 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0 };
s32 clawMotionIds[9] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x05), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x06), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x42),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x08), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18) };
Vec zeroVector = { 0.0f, 0.0f, 0.0f };

// PROTO
void BallPositionSet(u8, f32, f32, f32);
void BallStateSet(u8, s32);
u8 BallStateGet(u8);
u8 BallControllerStateGet(u8);
u8 BallPlayerIndexGet(u8);
Vec *BallPositionPointerGet(u8);
void ClawObjectInit(OMOBJ *);
void ClawObjectUpdate(OMOBJ *);
void ClawObjectStop(OMOBJ *);
void ClawAnimationUpdate(OMOBJ *);
u16 ClawFallDurationFramesGet(f32, f32, f32);
s32 ClawPlayerCollisionResolve(Vec *playerPosition, Vec *movementDelta, f32 clawRadius, Vec *resolvedPosition, s32 recursionDepth);
void ClawBallCollisionApply(s32, s32);
s32 ModelJointMatrixGet(s16, const char *, Mtx, s32);
void ModelJointMatrixFindRecursive(HSFOBJECT *, Mtx, const char *, Mtx);
void ModelAnimationUpdate(s32);
void ClawSoundTimersInit(HUPROCESS *);
void ClawSoundTimersUpdate(OMOBJ *);
s32 ClawSoundTimerStart(s32, s32);

OMOBJ *ClawObjectCreate(HUPROCESS *processManager)
{
    clawObjectManager = processManager;
    clawObject = omAddObjEx(processManager, 0x40, 5, 9, -1, ClawObjectInit);
    clawPlayerCount = 0;
    clawJointFound = 0;
    useCurrentModelMotion = 0;
    clawJointDebugEnabled = 0;
    ClawSoundTimersInit(processManager);
    return clawObject;
}

void ClawObjectDestroy(void)
{
    ClawObjectStop(clawObject);
}

void ClawObjectInit(OMOBJ *clawObject)
{
    s32 playerIndex;
    M424ClawState *clawState;

    clawState = clawObject->data = HuMemDirectMallocNum(HEAP_HEAP, 0x1B0, 0x10000000);
    memset(clawState, 0, 0x1B0);
    clawState->state = playerOrder[3];
    clawState->characterId = GWPlayerCfg[clawState->state].character;
    clawState->elapsedFrames = GWPlayerCfg[clawState->state].pad_idx;
    clawState->animationState = GWPlayerCfg[clawState->state].iscom;
    clawState->velocity = GWPlayerCfg[clawState->state].diff;
    clawState->actionTimer = clawState->position = 0;
    clawState->motionTimer = 0;
    clawState->effectState = 0;
    clawState->targetPosition = 0;
    clawState->collisionState = 0;
    clawState->targetRotation = 0;
    clawState->positionX = 0.0f;
    clawState->positionY = 1.0f;
    clawState->targetScale = 0.0f;
    clawState->motionFrame = 0.0f;
    clawState->velocityY = 1.0f;
    clawState->clawModel = 0;
    clawState->shadowModel = 0;
    clawState->isAnimating = 0;
    clawState->actionState = -1;
    clawState->carriedBallIndex = -1;
    clawState->ballControllerState = -1;
    clawState->collisionTimer = zeroVector;
    // @todo how did this happen
    clawState->motionState = atan2d(-102.864990234375, 424.559997558593750);
    if (clawState->motionState < 0.0f) {
        clawState->motionState += 360.0f;
    }
    clawState->soundEffectId = -1;
    clawState->velocityZ = zeroVector;
  
    for (playerIndex = 0; playerIndex < 0xC; playerIndex++) {
        clawState->playerPositions[playerIndex] = zeroVector;
        clawState->targetPositions[playerIndex] = zeroVector;
    }
    clawState->effectTimer = 0;
    clawState->animationFrame = -1;
    clawState->motionIndex = 0;
    clawState->motionTimer = 0;
    clawState->turnState = 0;
    clawState->turnTimer = 0;
    clawState->playerIndex = 0;
    clawState->motionCharacterId = 0;
    clawState->clawFlags = 0.0f;
    clawState->animationFrameCounter = 0;
    clawObject->mdlId[0] = CharModelCreate(clawState->characterId, 1);
    Hu3DModelShadowSet(clawObject->mdlId[0]);
    CharModelStepFxSet(clawState->characterId, 1);
    for (playerIndex = 0; playerIndex < 9; playerIndex++) {
        clawObject->mtnId[playerIndex] = CharMotionCreate(clawState->characterId, clawMotionIds[playerIndex]);
    }
    clawState->motionIndex = 5;
    CharMotionSet(clawState->characterId, clawObject->mtnId[clawState->motionIndex]);
    clawObject->mdlId[1] = Hu3DModelCreateFile(0x37001D);
    Hu3DModelShadowSet(clawObject->mdlId[1]);
    clawObject->mdlId[2] = Hu3DModelCreateFile(0x37001E);
    Hu3DModelShadowSet(clawObject->mdlId[2]);
    clawObject->mdlId[3] = Hu3DModelCreateFile(0x37001F);
    Hu3DModelShadowSet(clawObject->mdlId[3]);
    clawObject->mdlId[4] = Hu3DModelCreateFile(0x370020);
    Hu3DModelShadowSet(clawObject->mdlId[4]);
    Hu3DModelScaleSet(clawObject->mdlId[4], 0.9f, 0.9f, 0.9f);
    Hu3DModelHookSet(clawObject->mdlId[1], "hook1", clawObject->mdlId[2]);
    Hu3DModelHookSet(clawObject->mdlId[2], "hook2", clawObject->mdlId[3]);
    Hu3DModelHookSet(clawObject->mdlId[3], "hook3", clawObject->mdlId[4]);
    Hu3DModelPosSet(clawObject->mdlId[1], 950.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(clawObject->mdlId[2], 0x40000002);
    Hu3DModelAttrSet(clawObject->mdlId[4], 0x40000002);
    clawObject->trans.x = 950.0f;
    clawObject->trans.y = 275.0f;
    clawObject->trans.z = 0.0f;
    clawObject->rot.y = 270.0f;
    clawState->motionTimer = 3;
    clawState->targetScale = 0.45f;
    clawState->effectState = 0;
    clawObject->objFunc = ClawObjectUpdate;
}

void ClawObjectUpdate(OMOBJ *clawObject)
{
    Mtx jointMatrix;
    Mtx baseJointMatrix;
    Vec clawPosition;
    Vec collisionCorrection;
    Vec collisionOffset;
    Vec pathStart;
    Vec pathEnd;
    f32 motionValue;
    f32 modelScale;
    M424ClawState *clawState;
    s32 pathIndex;
    s32 modelId;
    s32 ballControllerState;
    s32 playerIndex;
    s32 motionState;

    clawState = clawObject->data;
    if (M424SceneStateGet() == 4) {
        clawState->position = HuPadBtn[clawState->elapsedFrames];
        clawState->actionTimer = HuPadBtnDown[clawState->elapsedFrames];
        if (clawState->animationState != 0) {
            ClawAnimationUpdate(clawObject);
        }
    }
    else {
        clawState->position = 0;
        clawState->actionTimer = 0;
    }
    if (M424SceneStateGet() <= 1) {
        motionState = -1;
    }
    else {
        motionState = clawState->motionTimer;
    }
    switch (motionState) {
        case 0:
            if (M424SceneStateGet() >= 4) {
                clawState->motionTimer = 1;
                clawState->effectState = 0;
            }
            break;
        case 1:
            if (M424SceneStateGet() > 4) {
                clawState->motionTimer = 10;
                clawState->effectState = 0;
            }
            else if ((clawState->actionTimer & 0x100) != 0) {
                HuAudFXPlay(0x62A);
                clawState->soundEffectId = HuAudFXPlay(0x621);
                clawState->motionTimer = 2;
                clawState->effectState = 0;
            }
            break;
        case 2:
            clawState->effectState++;
            clawState->targetScale += (REFRESH_FREQ * clawState->positionY);
            if (clawState->targetScale >= 1.0f) {
                clawState->targetScale = 1.0f;
                clawState->positionY = -1.0f;
            }
            else {
                if (clawState->targetScale <= 0.0f) {
                    clawState->targetScale = 0.0f;
                    clawState->positionY = 1.0f;
                }
            }
            if (M424SceneStateGet() > 4) {
                if (clawState->soundEffectId != -1) {
                    HuAudFXStop(clawState->soundEffectId);
                    clawState->soundEffectId = -1;
                }
                HuAudFXPlay(0x622);
                clawState->motionTimer = 10;
                clawState->effectState = 0;
            }
            else if (((clawState->position & 0x100) == 0) || (clawState->effectState >= REFRESH_RATE_F * 6)) {
                if (clawState->soundEffectId != -1) {
                    HuAudFXStop(clawState->soundEffectId);
                    clawState->soundEffectId = -1;
                }
                HuAudFXPlay(0x622);
                clawState->soundEffectId = HuAudFXPlay(0x62C);
                clawState->actionState = -1;
                clawState->actionTimer = -1;
                clawState->motionTimer = 3;
                clawState->effectState = 0;
            }
            break;
        case 3:
            clawState->effectState++;
            if (clawState->clawModel == 0) {
                clawState->motionFrame += REFRESH_FREQ;
            }
            if (clawState->motionFrame > 1.0f) {
                clawState->motionFrame = 1.0f;
            }
            if (clawState->effectState >= REFRESH_RATE) {
                if (clawState->soundEffectId != -1) {
                    HuAudFXStop(clawState->soundEffectId);
                    clawState->soundEffectId = -1;
                }
                HuAudFXPlay(0x62D);
                HuAudFXPlay(0x625);
                clawState->motionTimer = 4;
                clawState->effectState = 0;
            }
            clawState->clawModel = 0;
            break;
        case 4:
            clawState->effectState++;
            if (clawState->shadowModel == 0) {
                clawState->velocityY -= REFRESH_FREQ;
            }
            if (clawState->velocityY < 0.0f) {
                clawState->velocityY = 0.0f;
            }
            if (clawState->effectState >= REFRESH_RATE) {
                clawState->motionTimer = 5;
                clawState->effectState = 0;
            }
            clawState->shadowModel = 0;
            break;
        case 5:
            clawState->effectState++;
            if (clawState->effectState >= REFRESH_RATE_F / 10) {
                clawState->soundEffectId = HuAudFXPlay(0x62C);
                if ((clawState->actionState != -1)) {
                    ballControllerState = BallControllerStateGet(clawState->actionState);
                    if (ballControllerState != 0) {
                        playerIndex = BallPlayerIndexGet(clawState->actionState);
                        CharFXPlay(playerIndex, 0x128);
                    }
                }
                clawState->motionTimer = 6;
                clawState->effectState = 0;
            }
            break;
        case 6:
            clawState->effectState++;
            clawState->motionFrame -= 2.0f / (REFRESH_RATE_F * 1.5f); // thanks GPT
            if (clawState->motionFrame <= 0.0f) {
                if (clawState->soundEffectId != -1) {
                    HuAudFXStop(clawState->soundEffectId);
                    clawState->soundEffectId = -1;
                }
                HuAudFXPlay(0x62D);
                clawState->motionFrame = 0.0f;
                if (clawState->actionState != -1) {
                    ballControllerState = BallControllerStateGet(clawState->actionState);
                }
                else {
                    ballControllerState = -1;
                }
                if (clawState->actionState == -1) {
                    if (M424SceneStateGet() > 4) {
                        clawState->motionTimer = 10;
                        clawState->effectState = 0;
                    }
                    else {
                        clawState->motionTimer = 8;
                        clawState->effectState = 0;
                        HuAudFXPlay(0x625);
                    }
                }
                else if (ballControllerState == 0) {
                    clawState->motionTimer = 8;
                    clawState->effectState = 0;
                    HuAudFXPlay(0x625);
                }
                else {
                    if (clawState->actionState != -1) {
                        MapCameraAnimationStart(-6.0f / (REFRESH_RATE_F * 10.0f), REFRESH_RATE);
                        MapEffectStart(REFRESH_RATE * 2 - 3, 0);
                    }
                    HuAudFXPlay(0x62B);
                    clawState->soundEffectId = HuAudFXPlay(0x623);
                    clawState->motionTimer = 7;
                    clawState->effectState = 0;
                }
            }
            break;
        case 7:
            if (clawObject->rot.y < clawState->motionState) {
                clawObject->rot.y += 1.25f;
            }
            else if (clawObject->rot.y > clawState->motionState) {
                clawObject->rot.y = clawState->motionState;
            }
            motionValue = 0.35f - clawState->targetScale;
            if (motionValue > 0.0125f) {
                clawState->targetScale += 0.0125f;
            }
            else if (motionValue < -0.0125f) {
                clawState->targetScale -= 0.0125f;
            }
            else {
                clawState->targetScale = 0.35f;
            }
            if ((clawObject->rot.y == clawState->motionState) && (0.35f == clawState->targetScale)) {
                if (clawState->soundEffectId != -1) {
                    HuAudFXStop(clawState->soundEffectId);
                    clawState->soundEffectId = -1;
                }
                HuAudFXPlay(0x624);
                HuAudFXPlay(0x625);
                clawState->motionTimer = 8;
                clawState->effectState = 0;
            }
            break;
        case 8:
            if ((clawState->effectState == 0) && (clawState->actionState != -1)) {
                if (BallControllerStateGet(clawState->actionState) != 0) {
                    BallStateSet(clawState->actionState, 4);
                    playerIndex = BallPlayerIndexGet(clawState->actionState);
                    CharFXPlay(playerIndex, 0x123);
                    ClawSoundTimerStart(0x628, REFRESH_RATE / 2);
                }
                else {
                    BallStateSet(clawState->actionState, 2);
                }
                clawState->carriedBallIndex = clawState->actionState;
                clawState->actionState = -1;
            }
            clawState->effectState++;
            clawState->velocityY += 2.0f * REFRESH_FREQ;
            if (clawState->velocityY > 1.0f) {
                clawState->velocityY = 1.0f;
            }
            if (clawState->velocityY >= 1.0f) {
                if (clawObject->rot.y == clawState->motionState) {
                    clawState->soundEffectId = HuAudFXPlay(0x623);
                }
                if (clawState->carriedBallIndex != -1) {
                    if (BallControllerStateGet(clawState->carriedBallIndex) != 0) {
                        clawPlayerCount++;
                    }
                }
                clawState->motionTimer = 9;
                clawState->effectState = 0;
            }
            break;
        case 9:
            clawState->effectState++;
            if (clawObject->rot.y > 270.0f) {
                clawObject->rot.y -= 1.25f;
            }
            else if (clawObject->rot.y < 270.0f) {
                clawObject->rot.y = 270.0f;
            }
            if (clawState->targetScale > 0.025f) {
                clawState->targetScale -= 0.025f;
            }
            else if (clawState->targetScale < 0.05f) {
                clawState->targetScale = 0.0f;
            }
            if ((270.0f == clawObject->rot.y) && (0.0f == clawState->targetScale)) {
                if (clawState->soundEffectId != -1) {
                    HuAudFXStop(clawState->soundEffectId);
                    clawState->soundEffectId = -1;
                    HuAudFXPlay(0x624);
                }
                clawState->positionY = 1.0f;
                clawState->motionTimer = 1;
                clawState->effectState = 0;
            }
            break;
        case 10:
            if (ClawPlayerThresholdReached() != 0) {
                clawState->motionIndex = 5;
                CharMotionSet(clawState->characterId, clawObject->mtnId[clawState->motionIndex]);
                clawState->motionIndex = 3;
                CharMotionShiftSet(clawState->characterId, clawObject->mtnId[clawState->motionIndex], 0.0f, 8.0f, 0U);
                clawState->collisionState = 0;
                clawState->targetPosition = 0;
                clawState->positionX = clawMovementSpeeds[0];
                clawState->targetRotation = ClawFallDurationFramesGet(clawPositionConfigs[0].y, clawPositionConfigs[1].y, clawState->positionX);
                clawState->motionTimer = 11;
                clawState->effectState = 0;
            }
            else {
                clawState->motionTimer = 13;
                clawState->effectState = 0;
            }
            break;
        case 11:
            pathIndex = clawState->targetPosition;
            if (pathIndex > 2) {
                pathIndex = 2;
            }
            if (clawState->collisionState == 0) {
                clawState->effectState++;
                pathStart = clawPositionConfigs[pathIndex];
                pathEnd = clawPositionConfigs[pathIndex + 1];
                clawObject->rot.y = AngleApproachDegrees(atan2d((pathEnd.x - clawObject->trans.x), (pathEnd.z - clawObject->trans.z)), clawObject->rot.y, 10.0f);
                motionValue = (f32)clawState->effectState / clawState->targetRotation;
                clawObject->trans.x = pathStart.x + (motionValue * (pathEnd.x - pathStart.x));
                clawObject->trans.z = pathStart.z + (motionValue * (pathEnd.z - pathStart.z));
                clawObject->trans.y += clawState->positionX;
                clawState->positionX -= 39.2f / REFRESH_RATE_F;
                if (clawState->targetPosition == 2) {
                    if ((clawState->effectState >= (0.4f * clawState->targetRotation)) && (clawState->motionIndex != 6) && (CharMotionShiftIDGet(clawState->characterId) == -1)) {
                        clawState->motionIndex = 6;
                        CharMotionShiftSet(clawState->characterId, clawObject->mtnId[clawState->motionIndex], 0.0f, 8.0f, 0);
                    }
                }
                if (clawState->effectState >= clawState->targetRotation) {
                    clawObject->trans.x = pathEnd.x;
                    clawObject->trans.y = pathEnd.y;
                    clawObject->trans.z = pathEnd.z;
                    clawState->motionIndex = 4;
                    CharMotionShiftSet(clawState->characterId, clawObject->mtnId[clawState->motionIndex], 0.0f, 8.0f, 0U);
                    clawState->effectState = 0;
                    clawState->collisionState ^= 1;
                    if (++clawState->targetPosition >= 3) {
                        clawState->motionTimer = 0xC;
                    }
                }
            }
            else {
                pathStart = clawPositionConfigs[pathIndex];
                pathEnd = clawPositionConfigs[pathIndex + 1];
                motionValue = atan2d((pathEnd.x - clawObject->trans.x), (pathEnd.z - clawObject->trans.z));
                clawObject->rot.y = AngleApproachDegrees(motionValue, clawObject->rot.y, 10.0f);
                if ((AngleDeltaDegrees(motionValue, clawObject->rot.y) < 1.0f) && (CharMotionShiftIDGet(clawState->characterId) == -1)) {
                    clawState->motionIndex = 3;
                    CharMotionShiftSet(clawState->characterId, clawObject->mtnId[clawState->motionIndex], 0.0f, 8.0f, 0);
                    clawState->positionX = clawMovementSpeeds[pathIndex];
                    clawState->targetRotation = ClawFallDurationFramesGet(pathStart.y, pathEnd.y, clawState->positionX);
                    clawState->effectState = 0;
                    clawState->collisionState ^= 1;
                }
            }
            break;
        case 12:
            if ((clawState->motionIndex != 0) && (CharMotionShiftIDGet(clawState->characterId) == -1)) {
                clawState->motionIndex = 0;
                CharMotionShiftSet(clawState->characterId, clawObject->mtnId[clawState->motionIndex], 0.0f, 8.0f, 0x40000008);
            }
            clawObject->rot.y = AngleApproachDegrees(0.0f, clawObject->rot.y, 10.0f);
            if ((AngleDeltaDegrees(0.0f, clawObject->rot.y) < 1.0f) && (clawState->motionIndex == 0)) {
                clawState->motionTimer = 0xD;
                clawState->effectState = 0;
            }
            break;
        case 13:
            if (M424SceneStateGet() >= 7) {
                if (ClawPlayerThresholdReached() != 0) {
                    clawState->motionTimer = 14;
                    clawState->effectState = 0;
                }
                else {
                    clawState->motionTimer = 15;
                    clawState->effectState = 0;
                }
            }
            break;
        case 14:
            if ((clawState->motionIndex != 7) && (CharMotionShiftIDGet(clawState->characterId) == -1)) {
                clawState->motionIndex = 7;
                CharMotionShiftSet(clawState->characterId, clawObject->mtnId[clawState->motionIndex], 0.0f, 8.0f, 0);
            }
            break;
        case 15:
            break;
    }
    if (clawState->motionTimer <= 9) {
        Hu3DModelRotSet(clawObject->mdlId[1], 0.0f, 90.0f + clawObject->rot.y, 0.0f);
    }
    modelId = clawObject->mdlId[2];
    Hu3DMotionTimeSet(modelId, clawState->targetScale * Hu3DMotionMaxTimeGet(modelId));
    ModelAnimationUpdate(modelId);
    modelScale = 0.2f + (0.8f * clawState->motionFrame);
    Hu3DModelScaleSet(clawObject->mdlId[3], 1.0f, modelScale, 1.0f);
    Hu3DModelScaleSet(clawObject->mdlId[4], 1.0f, 1.0f / modelScale, 1.0f);
    modelId = clawObject->mdlId[4];
    Hu3DMotionTimeSet(modelId, clawState->velocityY * Hu3DMotionMaxTimeGet(modelId));
    ModelAnimationUpdate(modelId);
    ModelJointMatrixGet(clawObject->mdlId[1], "hook1", jointMatrix, 0);
    ModelJointMatrixGet(clawObject->mdlId[2], "hook2", jointMatrix, 1);
    ModelJointMatrixGet(clawObject->mdlId[3], "hook3", jointMatrix, 1);
    MTXCopy(jointMatrix, baseJointMatrix);
    clawState->velocityZ.x = jointMatrix[0][3];
    clawState->velocityZ.y = jointMatrix[1][3];
    clawState->velocityZ.z = jointMatrix[2][3];

    for (pathIndex = 0; pathIndex < 0xC; pathIndex++) {
        MTXCopy(baseJointMatrix, jointMatrix);
        ModelJointMatrixGet(clawObject->mdlId[4], clawJointNames[pathIndex], jointMatrix, 1);
        clawState->targetPositions[pathIndex] = clawState->playerPositions[pathIndex];
        clawState->playerPositions[pathIndex].x = jointMatrix[0][3];
        clawState->playerPositions[pathIndex].y = jointMatrix[1][3];
        clawState->playerPositions[pathIndex].z = jointMatrix[2][3];
    }
    if ((clawState->actionState != -1) && (clawState->motionTimer >= 6)) {
        collisionCorrection.x = 0.0f;
        collisionCorrection.y = clawState->collisionTimer.y;
        collisionCorrection.z = 0.0f;
        VECSubtract(&clawState->collisionTimer, &collisionCorrection, &collisionOffset);
        motionValue = VECMag(&collisionOffset);
        if (motionValue != 0.0f) {
            VECNormalize(&collisionOffset, &collisionOffset);
            VECScale(&collisionOffset, &collisionOffset, 0.9f * motionValue);
            VECAdd(&collisionCorrection, &collisionOffset, &clawState->collisionTimer);
        }
        VECAdd(&clawState->velocityZ, &clawState->collisionTimer, &clawPosition);
        BallPositionSet(clawState->actionState, clawPosition.x, clawPosition.y, clawPosition.z);
    }
    clawState->isAnimating = 1;
}

void ClawObjectStop(OMOBJ *clawObject)
{
    clawObject->objFunc = NULL;
}

f32 clawMotionFrameTimings[4][2] = {
    REFRESH_RATE_F * 0.5f, REFRESH_RATE_F * 0.4f,
    REFRESH_RATE_F * 0.4f, REFRESH_RATE_F * 0.3f,
    REFRESH_RATE_F * 0.2f, REFRESH_RATE_F * 0.1f,
    REFRESH_RATE_F * 0.1f, 0.0f
};

f32 clawMotionScales[4][2] = { 0.2f, 0.15f, 0.15f, 0.1f, 0.1f, 0.05f, 0.05f, 0.0f };

void ClawAnimationUpdate(OMOBJ *clawObject)
{
    M424ClawMotionState candidatePlayers[8];
    M424ClawMotionState candidateSwap;
    s32 candidateCount;
    f32 playerAngle;
    f32 normalizedDistance;
    f32 distanceSquared;
    f32 motionTimeStart;
    f32 motionTimeEnd;
    f32 animationDuration;
    f32 scaleAdjustment;
    s32 randomFrameOffset;
    s32 randomScaleOffset;
    M424ClawState *clawState;
    Vec *ballPositionPointer;
    s32 ballIndex;
    s32 candidateIndex;
    s32 comparisonIndex;
    M424BallPhysicsState *ballPhysicsState;
    s32 animationActive;

    candidateCount = 0;
    clawState = clawObject->data;
    ballPhysicsState = &ballPhysicsStates[0];
    if (ballPhysicsState) {
        clawState->position = 0;
        clawState->actionTimer = 0;
        switch (clawState->motionTimer) {
            case 1:
                if (clawState->effectTimer == 0) {
                    clawState->effectTimer = 1;
                    clawState->animationFrame = -1;
                    clawState->animationFrameIndex = 0;
                    clawState->motionTimer = 0;
                    clawState->turnState = 0;
                    clawState->turnTimer = 0;
                    clawState->playerIndex = 0;
                    clawState->motionCharacterId = 0;
                    clawState->clawFlags = 0.0f;
                }
                if (clawState->animationFrame == -1) {
                    for (ballIndex = 0, candidateIndex = 0; ballIndex < ballCount; ballIndex++, ballPhysicsState++) {
                        if ((BallControllerStateGet(ballIndex) != 0) && (BallStateGet(ballIndex) <= 2)) {
                            ballPosition = BallPositionPointerGet(ballIndex);
                            playerAngle = atan2d(ballPosition->x, ballPosition->z);
                            if (playerAngle > 180.0f) {
                                playerAngle -= 360.0f;
                            }
                            if (candidateIndex < 8) {
                                if (playerAngle >= -90.0f) {
                                    if (playerAngle <= 45.0f) {
                                        candidatePlayers[candidateIndex].state = ballIndex;
                                        candidatePlayers[candidateIndex].position = playerAngle;
                                        candidatePlayers[candidateIndex].elapsedFrames = ballPosition->x;
                                        candidatePlayers[candidateIndex].velocity = ballPosition->z;
                                        candidateIndex++;
                                    }
                                }
                            }
                        }
                    }
                    if (candidateIndex != 0) {
                        for (ballIndex = 0; ballIndex < (candidateIndex - 1); ballIndex++) {
                            comparisonIndex = ballIndex + 1;
                            for (; comparisonIndex < candidateIndex; comparisonIndex++) {
                                if (candidatePlayers[ballIndex].position < candidatePlayers[comparisonIndex].position) {
                                    candidateSwap = candidatePlayers[ballIndex];
                                    candidatePlayers[ballIndex] = candidatePlayers[comparisonIndex];
                                    candidatePlayers[comparisonIndex] = candidateSwap;
                                }
                            }
                        }
                        clawState->animationFrame = candidatePlayers[0].state;
                        clawState->clawFlags = candidatePlayers[0].position;
                        clawState->turnTimer = 0;
                        clawState->playerIndex = 0;
                        clawState->motionCharacterId = 0;
                        clawState->animationFrameCounter = 0;
                    }
                }
                if (clawState->animationFrame != -1) {
                    ballPosition = BallPositionPointerGet(clawState->animationFrame);
                    playerAngle = atan2d(ballPosition->x, ballPosition->z);
                    if (playerAngle > 180.0f) {
                        playerAngle -= 360.0f;
                    }
                    if (playerAngle >= 30.0f) {
                        if (clawState->turnTimer != 0) {
                            motionTimeStart = clawMotionFrameTimings[clawState->velocity][0];
                            motionTimeEnd = clawMotionFrameTimings[clawState->velocity][1];
                            randomFrameOffset = ((motionTimeEnd - motionTimeStart) * (motionTimeStart * (0.007874016f * (frand() & 0x7F))));
                            motionTimeStart = clawMotionScales[clawState->velocity][0];
                            motionTimeEnd = clawMotionScales[clawState->velocity][1];
                            randomScaleOffset = ((motionTimeEnd - motionTimeStart) * (motionTimeStart * (0.007874016f * (frand() & 0x7F))));
                            clawState->animationFrameIndex = ((3.0f * clawState->motionCharacterId) + randomFrameOffset);
                            distanceSquared = (ballPosition->x * ballPosition->x) + (ballPosition->z * ballPosition->z);
                            normalizedDistance = sqrtf(distanceSquared);
                            normalizedDistance = (normalizedDistance - 350.0f) / 250.0f + randomScaleOffset;
                            if (normalizedDistance < 0.0f) {
                                normalizedDistance = 0.0f;
                            }
                            else if (normalizedDistance > 1.0f) {
                                normalizedDistance = 1.0f;
                            }
                            clawState->animationDuration = (REFRESH_RATE_F * (1.0f - normalizedDistance));
                            animationDuration = (1.5f + (-1.5f * normalizedDistance));
                            clawState->animationFrameIndex -= (s32)(REFRESH_RATE_F * animationDuration);
                            animationDuration = (1.0f + (-1.1f * normalizedDistance));
                            clawState->turnState = (REFRESH_RATE_F * animationDuration);
                            clawState->turnTimer = 0;
                            clawState->playerIndex = 0;
                            clawState->animationFrameCounter = 0;
                            clawState->animationFrameCounter = (clawState->animationFrameCounter + (REFRESH_RATE_F * (1.0f - normalizedDistance)));
                        }
                        if (++clawState->animationFrameCounter >= (clawState->animationFrameIndex - clawState->turnState)) {
                            clawState->actionTimer |= 0x100;
                            clawState->position |= 0x100;
                            clawState->animationFrameCounter = 0;
                            return;
                        }
                    }
                    else {
                        if (playerAngle >= (15.0f + clawState->clawFlags)) {
                            clawState->playerIndex++;
                            clawState->clawFlags = playerAngle;
                            clawState->motionCharacterId = (clawState->turnTimer / clawState->playerIndex);
                            return;
                        }
                        clawState->turnTimer++;
                        return;
                    }
                }
                break;
            case 2:
                if (clawState->animationFrame != -1) {
                    animationActive = 1;
                    if (++clawState->animationFrameCounter >= clawState->animationDuration) {
                        animationActive = 0;
                    }
                }
                else {
                    animationActive = 0;
                }
                if (animationActive != 0) {
                    clawState->position |= 0x100;
                }
                clawState->effectTimer = 0;
                break;
        }
    }
}

u16 ClawFallDurationFramesGet(f32 initialVelocity, f32 targetHeight, f32 gravityPerFrame)
{
    f32 height;
    u16 frameCount;

    frameCount = 0;
    for (height = initialVelocity; gravityPerFrame > 0.0f || height > targetHeight; frameCount++) {
        height += gravityPerFrame;
        gravityPerFrame -= 39.2f / REFRESH_RATE_F;
    }
    return frameCount;
}

s32 ClawMotionActive(void)
{
    M424ClawState *clawState;

    if (!clawObject) {
        return 0;
    }
    if (!clawObject->data) {
        return 0;
    }
    clawState = clawObject->data;
    if ((clawState->motionTimer >= 1) && (clawState->motionTimer < 6)) {
        return 1;
    }
    return 0;
}

s32 ClawPlayerThresholdReached(void)
{
    if (clawPlayerCount >= 3) {
        return 1;
    }
    else {
        return 0;
    }
}

s32 ClawMotionPhase10Reached(void)
{
    M424ClawState *clawState;

    if (!clawObject) {
        return 0;
    }
    if (!clawObject->data) {
        return 0;
    }

    clawState = clawObject->data;
    if (clawState->motionTimer >= 10) {
        return 1;
    }
    return 0;
}

s32 ClawMotionPhase13Reached(void)
{
    M424ClawState *clawState;

    if (!clawObject) {
        return 0;
    }
    if (!clawObject->data) {
        return 0;
    }

    clawState = clawObject->data;
    if (clawState->motionTimer >= 13) {
        return 1;
    }
    return 0;
}

void ClawPlayerPositionResolve(Vec *playerPosition, Vec *movement, f32 clawRadius, Vec *resolvedPosition)
{
    ClawPlayerCollisionResolve(playerPosition, movement, clawRadius, resolvedPosition, 0x10);
}

s32 ClawPlayerCollisionResolve(Vec *playerPosition, Vec *movementDelta, f32 clawRadius, Vec *resolvedPosition, s32 recursionDepth)
{
    Vec playerJointPosition;
    Vec translatedTargetPosition;
    Vec relativeMovement;
    Vec projectedTargetPosition;
    Vec separationVector;
    Vec desiredPosition;
    Vec collisionNormal;
    Vec movementOffset;
    f32 minimumDistance;
    f32 distance;
    f32 unusedFloat28;
    f32 projectionFactor;
    f32 segmentLengthSquared;
    f32 unusedFloat25;
    f32 dotProduct;
    f32 horizontalDistance;
    M424ClawState *clawState;
    s32 jointIndex;
    s32 collisionResolved;
    s32 segmentCollisionResolved;
    OMOBJ *clawGameObject;
    s32 collisionAllowed;

    collisionAllowed = 1;
    collisionResolved = 0;
    clawObject = clawObject;
    clawState = clawObject->data;
    VECAdd(playerPosition, movementDelta, &desiredPosition);
    movementOffset = *movementDelta;
    *resolvedPosition = *playerPosition;
    minimumDistance = 5.0f + clawRadius;
        if (clawState->isAnimating == 0) {
        return 0;
    }
    switch (clawState->motionTimer) {
        case 3:
            for (jointIndex = 0; jointIndex < 0xC; jointIndex++) {
                if (collisionResolved != 0) {
                    desiredPosition = *resolvedPosition;
                    VECSubtract(&desiredPosition, playerPosition, &movementOffset);
                }
                VECSubtract(&desiredPosition, &clawState->playerPositions[jointIndex], &collisionNormal);
                distance = VECMag(&collisionNormal);
                if (distance < minimumDistance) {
                    if (0.0f == distance) {
                        distance = minimumDistance;
                        collisionNormal.x = 0.0f;
                        collisionNormal.y = 0.0f;
                        collisionNormal.z = 1.0f;
                    }
                    horizontalDistance = sqrtf((collisionNormal.x * collisionNormal.x) + (collisionNormal.z * collisionNormal.z));
                    if ((clawState->playerPositions[jointIndex].y > desiredPosition.y) && (horizontalDistance < 17.599999999999998)) {
                        clawState->clawModel = 1;
                    }
                    else {
                        VECNormalize(&collisionNormal, &collisionNormal);
                        VECScale(&collisionNormal, &collisionNormal, minimumDistance);
                        VECAdd(&clawState->playerPositions[jointIndex], &collisionNormal, resolvedPosition);
                    }
                    collisionResolved = 1;
                }
            }
            break;
        default:
            for (jointIndex = 0; jointIndex < 0xC; jointIndex++) {
                if (collisionResolved != 0) {
                    desiredPosition = *resolvedPosition;
                    VECSubtract(&desiredPosition, playerPosition, &movementOffset);
                }
                segmentCollisionResolved = 0;
                playerJointPosition = clawState->playerPositions[jointIndex];
                VECAdd(&clawState->targetPositions[jointIndex], &movementOffset, &translatedTargetPosition);
                VECSubtract(&playerJointPosition, &translatedTargetPosition, &collisionNormal);
                VECSubtract(&desiredPosition, &translatedTargetPosition, &relativeMovement);
                segmentLengthSquared = (collisionNormal.z * collisionNormal.z) + ((collisionNormal.x * collisionNormal.x) + (collisionNormal.y * collisionNormal.y));
                if (segmentLengthSquared != 0.0f) {
                    dotProduct = (collisionNormal.x * relativeMovement.x) + (collisionNormal.y * relativeMovement.y) + (collisionNormal.z * relativeMovement.z);
                    projectionFactor = dotProduct / segmentLengthSquared;
                    if (projectionFactor >= 0.0f) {
                        if (projectionFactor <= 1.0f) {
                            VECScale(&collisionNormal, &collisionNormal, projectionFactor);
                            VECAdd(&translatedTargetPosition, &collisionNormal, &projectedTargetPosition);
                            VECSubtract(&desiredPosition, &projectedTargetPosition, &separationVector);
                            distance = VECMag(&separationVector);
                            if (distance == 0.0f) {
                                if (VECMag(&collisionNormal) <= 0.0f) {
                                    collisionNormal.x = collisionNormal.y = 0.0f;
                                    collisionNormal.z = 1.0f;
                                }
                                VECNormalize(&collisionNormal, &separationVector);
                                VECScale(&separationVector, &separationVector, minimumDistance);
                                VECAdd(&playerJointPosition, &separationVector, resolvedPosition);
                                collisionResolved = 1;
                                segmentCollisionResolved = 1;
                            }
                            else if (distance < minimumDistance) {
                                if (VECMag(&separationVector) <= 0.0f) {

                                    separationVector.x = separationVector.y = 0.0f;
                                    separationVector.z = 1.0f;
                                }
                                VECNormalize(&separationVector, &separationVector);
                                VECScale(&separationVector, &separationVector, minimumDistance);
                                VECAdd(&projectedTargetPosition, &separationVector, resolvedPosition);
                                collisionResolved = 1;
                                segmentCollisionResolved = 1;
                            }
                        }
                    }
                }
                if (segmentCollisionResolved == 0) {
                    VECSubtract(&desiredPosition, &playerJointPosition, &collisionNormal);
                    distance = VECMag(&collisionNormal);
                    if (distance < minimumDistance) {
                        if (distance <= 0.0f) {
                            collisionNormal.x = collisionNormal.y = 0.0f;
                            collisionNormal.z = 1.0f;
                        }
                        VECNormalize(&collisionNormal, &collisionNormal);
                        VECScale(&collisionNormal, &collisionNormal, minimumDistance);
                        VECAdd(&clawState->playerPositions[jointIndex], &collisionNormal, resolvedPosition);
                        collisionResolved = 1;
                        segmentCollisionResolved = 1;
                    }
                }
                if (segmentCollisionResolved == 0) {
                    VECSubtract(&desiredPosition, &translatedTargetPosition, &collisionNormal);
                    distance = VECMag(&collisionNormal);
                    if (distance < minimumDistance) {
                        if (distance <= 0.0f) {
                            collisionNormal.x = collisionNormal.y = 0.0f;
                            collisionNormal.z = 1.0f;
                        }
                        VECNormalize(&collisionNormal, &collisionNormal);
                        VECScale(&collisionNormal, &collisionNormal, minimumDistance);
                        VECAdd(&clawState->playerPositions[jointIndex], &collisionNormal, resolvedPosition);
                        collisionResolved = 1;
                        segmentCollisionResolved = 1;
                    }
                }
                if ((clawCollisionFlags[jointIndex] != 0) && (segmentCollisionResolved == 0)) {
                    collisionAllowed = 0;
                }
            }
            if ((clawState->motionTimer == 4) && (collisionAllowed != 0)) {
                resolvedPosition->x = clawState->velocityZ.x;
                resolvedPosition->y = playerPosition->y;
                resolvedPosition->z = clawState->velocityZ.z;
                recursionDepth = 0;
                clawState->shadowModel = 1;
            }
            break;
        case 0:
        case 1:
        case 2:
        case 9:
            break;
    }
    if (collisionResolved != 0) {
        if (--recursionDepth > 0) {
            desiredPosition = *resolvedPosition;
            VECSubtract(&desiredPosition, playerPosition, &movementOffset);
            if (ClawPlayerCollisionResolve(playerPosition, &movementOffset, clawRadius, &desiredPosition, recursionDepth) != 0) {
                *resolvedPosition = desiredPosition;
            }
        }
    }

    (void)minimumDistance; // @todo fix
    (void)minimumDistance;
    return collisionResolved;
}

s32 ClawBallCollisionCheck(Vec *ballPosition)
{
    Vec clawTipPosition;
    Vec positionDelta;
    f32 distance;
    M424ClawState *clawState;
    OMOBJ *clawObject;
    s32 collisionDetected;

    collisionDetected = 0;
    clawObject = clawObject;
    clawState = clawObject->data;
    if (clawState->isAnimating == 0) {
        return 0;
    }
    switch (clawState->motionTimer) {
        case 5:
            clawTipPosition.x = clawState->velocityZ.x;
            clawTipPosition.y = -120.00001f + clawState->velocityZ.y;
            clawTipPosition.z = clawState->velocityZ.z;
            VECSubtract(ballPosition, &clawTipPosition, &positionDelta);
            if ((distance = VECMag(&positionDelta)) < 50.0f) {
                collisionDetected = 1;
            }
    }
    return collisionDetected;
}

void ClawBallCollisionsUpdate(s32 unusedFlags)
{
    Vec ballPosition;
    Vec positionDelta;
    Vec resolvedPosition;
    Vec previousPosition;
    Vec *ballVelocity;
    M424BallPhysicsState *ballPhysicsState;
    Vec *ballPosition;
    M424ClawState *clawState;
    s32 ballIndex;
    s32 collisionRetryCount;
    OMOBJ *clawObject;

    if ((u8)omPauseChk() != 0U)
        return;

    clawGameObject = clawObject;
    if (clawGameObject) {
        clawState = clawGameObject->data;
        if (clawState && (clawState->isAnimating != 0)) {
            ballPhysicsState = &ballPhysicsStates[0];
            if (ballPhysicsState) {
                collisionRetryCount = 0x10;

                for (ballIndex = 0; ballIndex < ballCount; ballIndex++) {
                    ballPhysicsState = &ballPhysicsStates[ballIndex];
                    if ((BallStateGet(ballIndex) == 3) || (BallStateGet(ballIndex) == 4))
                        continue;
                    ballPositionPointer = BallPositionPointerGet(ballIndex);
                    ballVelocity = BallVelocityPointerGet(ballIndex);
                    previousPosition = *ballPositionPointer;
                    ballPosition = ballPhysicsState->currentPosition;
                    VECSubtract(ballPositionPointer, &ballPosition, &positionDelta);
                    if (ClawPlayerCollisionResolve(&ballPosition, &positionDelta, 100.0f, &resolvedPosition, 0x10) != 0) {
                        *ballPositionPointer = resolvedPosition;
                        ballPosition = resolvedPosition;
                        if ((ballPhysicsState->controllerState < 3) && (BallPositionClamp(&ballPosition, &resolvedPosition) != 0)) {
                            *ballPositionPointer = resolvedPosition;
                            ballPosition = resolvedPosition;
                        }
                        ClawBallCollisionApply(ballIndex, 0x20);
                        if ((previousPosition.y != ballPosition.y) && (ballIndex != clawState->actionState) && (ballIndex != clawState->carriedBallIndex)) {
                            ballPositionPointer->y = previousPosition.y;
                            ballPosition.y = previousPosition.y;
                            if (collisionRetryCount > 0) {
                                ballIndex--;
                                collisionRetryCount--;
                                continue;
                            }
                        }
                    }
                    if (clawState->actionState == -1) {
                        if (ClawBallCollisionCheck(&ballPosition) != 0) {
                            clawState->actionState = ballIndex;
                            clawState->ballControllerState = BallControllerStateGet(ballIndex);
                            VECSubtract(&ballPosition, &clawState->velocityZ, &clawState->collisionTimer);
                            BallPositionSet(clawState->actionState, ballPosition.x, ballPosition.y, ballPosition.z);
                        }
                    }
                    collisionRetryCount = 0x10;
                }
            }
        }
    }
}

void ClawBallCollisionApply(s32 ballIndex, s32 recursionDepth)
{
    Vec ballPosition;
    Vec otherBallPosition;
    Vec separationVector;
    Vec clampedPosition;
    Vec *currentBallPositionPointer;
    f32 separationScale;
    f32 separationDistance;
    s32 otherBallIndex; // ! - uninitialized
    M424BallPhysicsState *otherBallPhysicsState;
    Vec *otherBallPositionPointer;

#ifdef __MWERKS__
    M424BallPhysicsState *unusedPhysicsState;
    unusedPhysicsState = &ballPhysicsStates[otherBallIndex];
#endif
    currentBallPositionPointer = BallPositionPointerGet(ballIndex);
    ballPosition = *currentBallPositionPointer;
    for (otherBallIndex = 0; otherBallIndex < ballCount; otherBallIndex++) {
        if ((otherBallIndex != ballIndex) && (BallStateGet(otherBallIndex) < 3)) {
            otherBallPhysicsState = &ballPhysicsStates[otherBallIndex];
            otherBallPositionPointer = BallPositionPointerGet(otherBallIndex);
            otherBallPosition = *otherBallPositionPointer;
            VECSubtract(&otherBallPosition, &ballPosition, &separationVector);
            separationDistance = VECMag(&separationVector);
            if (separationDistance < 200.0f) {
                if (separationDistance > 0.0f) {
                    separationScale = 1.0f / separationDistance;
                    separationVector.x *= separationScale;
                    separationVector.y *= separationScale;
                    separationVector.z *= separationScale;
                }
                else {
                    separationVector.x = 0.0f;
                    separationVector.y = 0.0f;
                    separationVector.z = 1.0f;
                }
                separationScale = 1.0f + (200.0f - separationDistance);
                otherBallPosition.x += (separationVector.x * separationScale);
                otherBallPosition.z += (separationVector.z * separationScale);
                otherBallPositionPointer->x = otherBallPosition.x;
                otherBallPositionPointer->z = otherBallPosition.z;
                if ((otherBallPhysicsState->controllerState < 3) && (BallPositionClamp(&otherBallPosition, &clampedPosition) != 0)) {
                    otherBallPositionPointer->x = otherBallPosition.x = clampedPosition.x;
                    otherBallPositionPointer->z = otherBallPosition.z = clampedPosition.z;
                }
                if (recursionDepth > 0) {
                    recursionDepth--;
                    ClawBallCollisionApply(otherBallIndex, recursionDepth);
                }
            }
        }
    }
}

void ClawBallCollisionCallback(void)
{
    ClawBallCollisionsUpdate(0);
}

s32 ModelJointMatrixGet(s16 modelId, const char *jointName, Mtx jointMatrix, s32 includeModelTransform)
{
    Mtx modelMatrix;
    Mtx rotationMatrix;
    HSFDATA *modelData;
    HU3DMODEL *model;

    model = &Hu3DData[modelId];
    modelData = model->hsf;
    clawJointFound = 0;
    if (model->motId != -1) {
        useCurrentModelMotion = 1;
    }
    else {
        useCurrentModelMotion = 0;
    }
    mtxRot(rotationMatrix, model->rot.x, model->rot.y, model->rot.z);
    MTXScale(modelMatrix, model->scale.x, model->scale.y, model->scale.z);
    MTXConcat(rotationMatrix, modelMatrix, modelMatrix);
    mtxTransCat(modelMatrix, model->pos.x, model->pos.y, model->pos.z);
    if (includeModelTransform != 0) {
        MTXConcat(jointMatrix, modelMatrix, modelMatrix);
    }
    ModelJointMatrixFindRecursive(modelData->root, modelMatrix, jointName, jointMatrix);
    return clawJointFound;
}

void ModelJointMatrixFindRecursive(HSFOBJECT *jointObject, Mtx parentMatrix, const char *targetJointName, Mtx resultMatrix)
{
    Mtx jointWorldMatrix;
    Mtx jointRotationMatrix;
    Mtx childRotationMatrix;
    Mtx childWorldMatrix;
    Mtx grandchildRotationMatrix;
    Mtx grandchildWorldMatrix;
    HSFTRANSFORM *jointTransform;
    HSFTRANSFORM *childTransform;
    HSFTRANSFORM *grandchildTransform;
    HSFOBJECT *childObject;
    HSFOBJECT *grandchildObject;
    u32 childIndex;
    u32 grandchildIndex;
    u32 descendantIndex;

    if (clawJointFound == 0) {
        if (useCurrentModelMotion != 0) {
            jointTransform = &jointObject->mesh.curr;
        }
        else {
            jointTransform = &jointObject->mesh.base;
        }
        if ((jointTransform->scale.x <= 0.0f) && (jointTransform->scale.y <= 0.0f) && (jointTransform->scale.z <= 0.0f)) {
            MTXCopy(parentMatrix, jointWorldMatrix);
            OSReport("OBJ MTX CHECK ERROR %s \n", jointObject->name);
        }
        else {
            mtxRot(jointRotationMatrix, jointTransform->rot.x, jointTransform->rot.y, jointTransform->rot.z);
            MTXScale(jointWorldMatrix, jointTransform->scale.x, jointTransform->scale.y, jointTransform->scale.z);
            MTXConcat(jointRotationMatrix, jointWorldMatrix, jointWorldMatrix);
            mtxTransCat(jointWorldMatrix, jointTransform->pos.x, jointTransform->pos.y, jointTransform->pos.z);
            MTXConcat(parentMatrix, jointWorldMatrix, jointWorldMatrix);
        }
        if (clawJointDebugEnabled != 0) {
            OSReport("%s \n", jointObject->name);
        }
        if (strcmp(targetJointName, jointObject->name) == 0) {
            MTXCopy(jointWorldMatrix, resultMatrix);
            clawJointFound = 1;
        }
        for (childIndex = 0; childIndex < jointObject->mesh.childrenCount; childIndex++) {
            childObject = jointObject->mesh.children[childIndex];
            if (clawJointFound == 0) {
                if (useCurrentModelMotion != 0) {
                    childTransform = &childObject->mesh.curr;
                }
                else {
                    childTransform = &childObject->mesh.base;
                }
                if ((childTransform->scale.x <= 0.0f) && (childTransform->scale.y <= 0.0f) && (childTransform->scale.z <= 0.0f)) {
                    MTXCopy(jointWorldMatrix, childWorldMatrix);
                    OSReport("OBJ MTX CHECK ERROR %s \n", childObject->name);
                }
                else {
                    mtxRot(childRotationMatrix, childTransform->rot.x, childTransform->rot.y, childTransform->rot.z);
                    MTXScale(childWorldMatrix, childTransform->scale.x, childTransform->scale.y, childTransform->scale.z);
                    MTXConcat(childRotationMatrix, childWorldMatrix, childWorldMatrix);
                    mtxTransCat(childWorldMatrix, childTransform->pos.x, childTransform->pos.y, childTransform->pos.z);
                    MTXConcat(jointWorldMatrix, childWorldMatrix, childWorldMatrix);
                }
                if (clawJointDebugEnabled != 0) {
                    OSReport("%s \n", childObject->name);
                }
                if (strcmp(targetJointName, childObject->name) == 0) {
                    MTXCopy(childWorldMatrix, resultMatrix);
                    clawJointFound = 1;
                }
                for (grandchildIndex = 0; grandchildIndex < childObject->mesh.childrenCount; grandchildIndex++) {
                    grandchildObject = childObject->mesh.children[grandchildIndex];
                    if (clawJointFound == 0) {
                        if (useCurrentModelMotion != 0) {
                            grandchildTransform = &grandchildObject->mesh.curr;
                        }
                        else {
                            grandchildTransform = &grandchildObject->mesh.base;
                        }
                        if ((grandchildTransform->scale.x <= 0.0f) && (grandchildTransform->scale.y <= 0.0f) && (grandchildTransform->scale.z <= 0.0f)) {
                            MTXCopy(childWorldMatrix, grandchildWorldMatrix);
                            OSReport("OBJ MTX CHECK ERROR %s \n", grandchildObject->name);
                        }
                        else {
                            mtxRot(grandchildRotationMatrix, grandchildTransform->rot.x, grandchildTransform->rot.y, grandchildTransform->rot.z);
                            MTXScale(grandchildWorldMatrix, grandchildTransform->scale.x, grandchildTransform->scale.y, grandchildTransform->scale.z);
                            MTXConcat(grandchildRotationMatrix, grandchildWorldMatrix, grandchildWorldMatrix);
                            mtxTransCat(grandchildWorldMatrix, grandchildTransform->pos.x, grandchildTransform->pos.y, grandchildTransform->pos.z);
                            MTXConcat(childWorldMatrix, grandchildWorldMatrix, grandchildWorldMatrix);
                        }
                        if (clawJointDebugEnabled != 0) {
                            OSReport("%s \n", grandchildObject->name);
                        }
                        if (strcmp(targetJointName, grandchildObject->name) == 0) {
                            MTXCopy(grandchildWorldMatrix, resultMatrix);
                            clawJointFound = 1;
                        }
                        for (descendantIndex = 0; descendantIndex < grandchildObject->mesh.childrenCount; descendantIndex++) {
                            ModelJointMatrixFindRecursive(grandchildObject->mesh.children[descendantIndex], grandchildWorldMatrix, targetJointName, resultMatrix);
                        }
                    }
                }
            }
        }
    }
}

void ModelAnimationUpdate(s32 modelId)
{
    HU3DMODEL *model;

    model = &Hu3DData[modelId];
    if (model->motId != -1) {
        Hu3DMotionExec(modelId, model->motId, model->motWork.time, 0);
    }
    if (model->motIdShift != -1) {
        Hu3DSubMotionExec(modelId);
    }
    if (model->hsf->cenvNum != 0) {
        EnvelopeProc(model->hsf);
    }
}

void ClawSoundTimersInit(HUPROCESS *processManager)
{
    M424ClawPlayerState *soundTimer;
    s32 playerIndex;

    omAddObjEx(processManager, 0x40, 0, 0, -1, ClawSoundTimersUpdate);
    soundTimer = &clawPlayerSoundTimers[0];

    for (playerIndex = 0; playerIndex < 0x10; playerIndex++, soundTimer++) {
        soundTimer->state = -1;
        soundTimer->elapsedFrames = -1;
    }
}

void ClawSoundTimersUpdate(OMOBJ *timerObject)
{
    M424ClawPlayerState *soundTimer;
    s32 playerIndex;

    soundTimer = &clawPlayerSoundTimers[0];
    for (playerIndex = 0; playerIndex < 0x10; playerIndex++, soundTimer++) {
        if ((soundTimer->state >= 0) && (soundTimer->elapsedFrames > 0)) {
            if (--soundTimer->elapsedFrames <= 0) {
                HuAudFXPlay(soundTimer->state);
                soundTimer->state = -1;
                soundTimer->elapsedFrames = -1;
            }
        }
    }
}

s32 ClawSoundTimerStart(s32 soundId, s32 delayFrames)
{
    M424ClawPlayerState *soundTimer;
    s32 timerIndex;

    soundTimer = &clawPlayerSoundTimers[0];
    for (timerIndex = 0; timerIndex < 0x10; timerIndex++, soundTimer++) {
        if (soundTimer->state == -1)
            break;
    }
    if (timerIndex >= 0x10) {
        return -1;
    }
    soundTimer->state = soundId;
    soundTimer->elapsedFrames = delayFrames;
    return timerIndex;
}

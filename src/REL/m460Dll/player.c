#include "datadir_enum.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/object.h"
#include "game/pad.h"
#include "string.h"

#include "REL/m460Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

typedef struct M460PlayerMotionConfig {
    /* 0x00 */ u16 motionIndex;
    /* 0x04 */ float blendDuration;
    /* 0x08 */ float motionStartFrame;
    /* 0x0C */ float motionEndFrame;
    /* 0x10 */ u32 motionFlags;
} M460PlayerMotionConfig; /* size = 0x14 */

typedef struct M460PlayerWork {
    s32 characterModelId;
    s32 motionId;
    s32 playerSlot;
    s32 motionFlags;
    s32 actionFlags;
    u32 buttonPressed;
    u32 buttonReleased;
    float movementAngle;
    float movementSpeed;
    s32 isComputerPlayer;
    s32 isRunning;
    u16 actionState;
    s32 actionTimer;
    char unreferencedPlayerData34[0x4];
    s16 motionFrame;
    char unreferencedMotionData[6];
    s32 motionStatus;
    float currentYaw;
    float targetYaw;
    float turnProgress;
    float turnSpeed;
    float movementScale;
    s32 soundState;
    float jumpHeight;
    Vec position;
    Vec velocity;
    Vec targetPosition;
    s32 soundEmitterId;
} M460PlayerWork; /* size = 0x88 */

void M460InitializePlayer(OMOBJ *object);
void M460UpdatePlayerSelection(OMOBJ *object);
void M460UpdatePlayerRound(OMOBJ *object);
void M460UpdatePlayerResults(OMOBJ *object);
void M460UpdatePlayerTurning(OMOBJ *object);
s32 M460UpdatePlayerMovement(OMOBJ *object, float maxSpeed, float movementAcceleration);
void M460UpdatePlayerAnimation(OMOBJ *object);
void M460BeginPlayerAction(OMOBJ *object);
void M460SetPlayerAction(OMOBJ *object, u32 motionAction);
s32 M460IsPlayerActionDone(OMOBJ *object);
void M460InitializeSpectator(OMOBJ *object);
void M460UpdateSpectator(OMOBJ *object);

u32 playerMotionFileIds[8] = { 0, 2, 3, 0x590015, 0x59000D, 0x17, 0x18, 0x1A };
struct M460PlayerMotionConfig playerMotionConfigs[8] = {
    { 0, 0.2f, 0.0f, -1.0f, 0x40000001 },
    { 1, 0.2f, 0.0f, -1.0f, 0x40000001 },
    { 2, 0.2f, 0.0f, -1.0f, 0x40000001 },
    { 3, 0.2f, 0.0f, -1.0f, 0x40000001 },
    { 4, 0.2f, 0.0f, -1.0f, 0 },
    { 5, 0.2f, 0.0f, -1.0f, 0 },
    { 6, 0.2f, 0.0f, -1.0f, 0 },
    { 7, 0.2f, 0.0f, -1.0f, 0x40000001 },
};

OMOBJ *playerObjects[1];
OMOBJ *spectatorObject;
s32 playerSlotByObject[1];

void M460CreatePlayerObjects(HUPROCESS *process)
{
    u32 playerGroupIds[1];
    u32 playerSlotOrder[1];

    s16 playerIndex;
    s16 nextPlayerIndex;
    u32 swapValue;
    OMOBJ *playerObject;

    for (playerIndex = 0; playerIndex < 1; playerIndex++) {
        playerGroupIds[playerIndex] = GWPlayerCfg[playerIndex].group;
        playerSlotOrder[playerIndex] = playerIndex;
    }
    for (playerIndex = 0; playerIndex < 0; playerIndex++) {
        for (nextPlayerIndex = playerIndex + 1; nextPlayerIndex < 1; nextPlayerIndex++) {
            if (playerGroupIds[playerIndex] > playerGroupIds[nextPlayerIndex]) {
                swapValue = playerGroupIds[playerIndex];
                playerGroupIds[playerIndex] = playerGroupIds[nextPlayerIndex];
                playerGroupIds[nextPlayerIndex] = swapValue;
                swapValue = playerSlotOrder[playerIndex];
                playerSlotOrder[playerIndex] = playerSlotOrder[nextPlayerIndex];
                playerSlotOrder[nextPlayerIndex] = swapValue;
            }
        }
    }
    for (playerIndex = 0; playerIndex < 1; playerIndex++) {
        playerSlotByObject[playerSlotOrder[playerIndex]] = playerIndex;
    }
    omMakeGroupEx(process, 0, 1);
    omGetGroupMemberListEx(process, 0);
    for (playerIndex = 0; playerIndex < 1; playerIndex++) {
        playerObject = playerObjects[playerIndex] = omAddObjEx(process, 0x28, 1, 8, 0, M460InitializePlayer);
        playerObject->work[0] = playerIndex;
    }
    spectatorObject = omAddObjEx(process, 0x28, 1, 5, -1, M460InitializeSpectator);
}

void M460DestroyPlayerObjects(void)
{
    CharModelKill(-1);
}

void M460InitializePlayer(OMOBJ *object)
{
    M460PlayerWork *playerWork;
    s32 motionIndex;

    float movementSpeed[4] = { 0.25f, 0.5f, 0.75f, 1.0f };
    Vec position2[4] = {
        { 0.0f, 0.0f, 300.0f },
        { -450.0f, 0.0f, -50.0f },
        { 0.0f, 0.0f, -350.0f },
        { 450.0f, 0.0f, -50.0f },
    };
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M460PlayerWork), HU_MEMNUM_OVL);
    playerWork = object->data;
    memset(playerWork, 0, sizeof(M460PlayerWork));
    object->stat |= 0x100;
    playerWork->characterModelId = GWPlayerCfg[object->work[0]].character;
    playerWork->playerSlot = playerSlotByObject[object->work[0]];
    M460SetPlayerResult(object->work[0], playerWork->playerSlot, playerWork->characterModelId);
    playerWork->movementScale = movementSpeed[GWPlayerCfg[object->work[0]].diff & 3];
    playerWork->isComputerPlayer = GWPlayerCfg[object->work[0]].iscom;
    object->mdlId[0] = CharModelCreate(playerWork->characterModelId, 2);
    Hu3DModelLayerSet(object->mdlId[0], 1);
    CharEffectLayerSet(2);
    Hu3DModelShadowSet(object->mdlId[0]);
    for (motionIndex = 0; motionIndex < 8; motionIndex++) {
        if (playerMotionFileIds[motionIndex] < 0x10000) {
            object->mtnId[motionIndex] = CharMotionCreate(playerWork->characterModelId, playerMotionFileIds[motionIndex]);
        }
        else {
            object->mtnId[motionIndex] = Hu3DJointMotionFile(object->mdlId[0], playerWork->characterModelId + playerMotionFileIds[motionIndex]);
        }
    }
    CharMotionDataClose(playerWork->characterModelId);
    CharNpcDustVoiceOffSet(object->mdlId[0], object->mtnId[3], 1);
    object->trans.x = playerWork->position.x = position2[playerWork->playerSlot].x;
    object->trans.y = playerWork->position.y = position2[playerWork->playerSlot].y;
    object->trans.z = playerWork->position.z = position2[playerWork->playerSlot].z;
    playerWork->currentYaw = playerWork->targetYaw = 0.0f;
    playerWork->turnProgress = 0.0f;
    playerWork->turnSpeed = 0.02f;
    playerWork->jumpHeight = 50.0f;
    playerWork->soundState = 0;
    playerWork->actionState = 0;
    playerWork->motionId = -1;
    M460SetPlayerAction(object, 0);
    playerWork->soundEmitterId = -1;
    object->objFunc = M460UpdatePlayerSelection;
}

void M460UpdatePlayerSelection(OMOBJ *object)
{
    M460PlayerWork *playerWork;
    s32 reachedTarget;

    playerWork = object->data;
    playerWork->buttonPressed = playerWork->buttonReleased = 0;
    playerWork->motionFlags = playerWork->actionFlags = 0;
    playerWork->movementAngle = 0.0f;
    playerWork->movementSpeed = 0.0f;
    switch (playerWork->actionState) {
        case 0:
            playerWork->currentYaw = playerWork->targetYaw = 180.0f;
            playerWork->position.x = 0.0f;
            playerWork->position.y = 0.0f;
            playerWork->position.z = -450.0f;
            M460SetPlayerAction(object, 7);
            playerWork->actionState = 1;
            playerWork->actionTimer = 0;
            break;
        case 1:
            if (M460GetResultState() == 1) {
                playerWork->position.x = 0.0f;
                playerWork->position.y = 0.0f;
                playerWork->position.z = -100.0f;
                playerWork->targetPosition.x = 0.0f;
                playerWork->targetPosition.y = 0.0f;
                playerWork->targetPosition.z = -450.0f;
                playerWork->actionState = 2;
                playerWork->actionTimer = 0;
            }
            break;
        case 2:
            reachedTarget = M460UpdatePlayerMovement(object, 0.5f, 20.0f);
            M460UpdatePlayerAnimation(object);
            if (reachedTarget != 0) {
                playerWork->actionState = 3;
                playerWork->actionTimer = 0;
            }
            break;
    }
    VECAdd(&playerWork->position, &playerWork->velocity, &playerWork->position);
    M460UpdatePlayerTurning(object);
    if (M460GetResultState() == 2) {
        playerWork->actionState = 0;
        object->objFunc = M460UpdatePlayerRound;
    }
}

void M460UpdatePlayerRound(OMOBJ *object)
{
    M460PlayerWork *playerWork;
    s32 reachedTarget;
    u32 selectionState;
    s32 playerReadyForResults;

    playerWork = object->data;
    playerReadyForResults = 0;
    {
        Vec selectionPositions[2] = {
            { -250.0f, 0.0f, -900.0f },
            { 250.0f, 0.0f, -900.0f },
        };

        Vec resultPositions[2] = {
            { -250.0f, 0.0f, -1400.0f },
            { 250.0f, 0.0f, -1400.0f },
        };
        playerWork->buttonPressed = playerWork->buttonReleased = 0;
        playerWork->motionFlags = playerWork->actionFlags = 0;
        playerWork->movementAngle = 0.0f;
        playerWork->movementSpeed = 0.0f;
        if (M460GetRoundState() == 2) {
            if (playerWork->isComputerPlayer != 0) {
                M460BeginPlayerAction(object);
            }
            else {
                if (HuPadTrigL[GWPlayerCfg[object->work[0]].pad_idx] > 0x28) {
                    playerWork->buttonPressed = 1;
                }
                if (HuPadTrigR[GWPlayerCfg[object->work[0]].pad_idx] > 0x28) {
                    playerWork->buttonReleased = 1;
                }
            }
        }
        switch (playerWork->actionState) {
            case 0:
                if (M460GetCurrentFrameCount() != 0) {
                    playerWork->position.x = -140.0f;
                    playerWork->position.y = 0.0f;
                    playerWork->position.z = -150.0f;
                }
                playerWork->targetPosition.x = 0.0f;
                playerWork->targetPosition.y = 0.0f;
                playerWork->targetPosition.z = -450.0f;
                playerWork->motionStatus = 3;
                playerWork->actionState = 4;
            case 4:
                reachedTarget = M460UpdatePlayerMovement(object, 0.5f, 20.0f);
                M460UpdatePlayerAnimation(object);
                if ((reachedTarget != 0) && (M460GetRoundState() == 2)) {
                    playerWork->targetYaw = 180.0f;
                    playerWork->velocity.x = playerWork->velocity.y = playerWork->velocity.z = 0.0f;
                    playerWork->actionState = 5;
                }
                break;
            case 5:
                reachedTarget = 0;
                M460UpdatePlayerAnimation(object);
                if (CharMotionShiftIDGet(playerWork->characterModelId) < 0) {
                    reachedTarget = 1;
                }
                selectionState = M460GetSelectionState();
                if (selectionState == 3) {
                    if (reachedTarget != 0) {
                        if (playerWork->buttonPressed != 0) {
                            selectionState = 0;
                        }
                        else if (playerWork->buttonReleased != 0) {
                            selectionState = 1;
                        }
                        if (selectionState != 3) {
                            M460SetSelectionState(selectionState);
                        }
                    }
                }
                else {
                    playerWork->motionStatus = selectionState;
                    playerWork->targetPosition = selectionPositions[playerWork->motionStatus];
                    playerWork->actionState = 6;
                }
                break;
            case 6:
                reachedTarget = M460UpdatePlayerMovement(object, 1.0f, 20.0f);
                M460UpdatePlayerAnimation(object);
                if (reachedTarget != 0) {
                    M460ConfirmSelection();
                    playerWork->targetYaw = 180.0f;
                    playerWork->velocity.x = playerWork->velocity.y = playerWork->velocity.z = 0.0f;
                    playerWork->actionState = 7;
                }
                break;
            case 7:
                M460UpdatePlayerAnimation(object);
                if (M460IsSelectionReady() != 0) {
                    if (M460IsGameComplete() == 0) {
                        playerWork->targetPosition = resultPositions[playerWork->motionStatus];
                        playerWork->actionState = 8;
                    }
                    else {
                        playerReadyForResults = 1;
                    }
                }
                break;
            case 8:
                reachedTarget = M460UpdatePlayerMovement(object, 0.5f, 20.0f);
                M460UpdatePlayerAnimation(object);

            default:
                if (M460GetRoundState() == 1) {
                    playerWork->actionState = 0;
                }
                playerReadyForResults = 1;
                break;
        }
    }
    VECAdd(&playerWork->position, &playerWork->velocity, &playerWork->position);
    M460UpdatePlayerTurning(object);
    if ((M460GetRoundState() == 5) && (playerReadyForResults != 0)) {
        playerWork->actionState = 0;
        object->objFunc = M460UpdatePlayerResults;
    }
}

void M460UpdatePlayerResults(OMOBJ *object)
{
    Vec emitterPosition;
    M460PlayerWork *playerWork;
    s32 reachedTarget;

    playerWork = object->data;
    reachedTarget = 0;
    switch (playerWork->actionState) {
        case 0:
            M460SetPlayerStateFlag(object->work[0]);
            M460SetPlayerAction(object, 4);
            emitterPosition = playerWork->position;
            emitterPosition.y += 160.0f;
            playerWork->actionState = 9;
            break;
        case 9:
            if (M460IsPlayerActionDone(object) != 0) {
                playerWork->targetPosition.x = 0.0f;
                playerWork->targetPosition.y = 0.0f;
                playerWork->targetPosition.z = 500.0f;
                M460SetCameraStateFlag(object->work[0]);
                CharFXPlay(playerWork->characterModelId, 0x128);
                playerWork->isRunning = 1;
                playerWork->actionState = 0xA;
            }
            break;
        case 10:
            reachedTarget = M460UpdatePlayerMovement(object, 1.0f, 20.0f);
            M460UpdatePlayerAnimation(object);
            if (reachedTarget != 0) {
                playerWork->velocity.x = playerWork->velocity.y = playerWork->velocity.z = 0.0f;
                playerWork->actionState = 0xB;
            }
            break;
        case 11:
            M460SetPlayerAction(object, 0);
            break;
    }
    VECAdd(&playerWork->position, &playerWork->velocity, &playerWork->position);
    M460UpdatePlayerTurning(object);
    if (playerWork->soundEmitterId >= 0) {
        emitterPosition = playerWork->position;
        emitterPosition.y += 160.0f;
        HuAudFXEmiterUpDate(playerWork->soundEmitterId, &emitterPosition);
    }
}

void M460UpdatePlayerTurning(OMOBJ *object)
{
    M460PlayerWork *playerWork = object->data;
    playerWork->motionFrame++;
    playerWork->actionTimer++;
    object->trans.x = playerWork->position.x;
    object->trans.y = playerWork->position.y;
    object->trans.z = playerWork->position.z;
    playerWork->turnProgress = playerWork->turnProgress + playerWork->turnSpeed;
    if (playerWork->turnProgress > 0.45f) {
        playerWork->turnProgress = 0.45f;
    }
    playerWork->currentYaw = M460ApproachAngle(playerWork->currentYaw, playerWork->targetYaw, playerWork->turnProgress);
    object->rot.y = playerWork->currentYaw;
    Hu3DModelPosSetV(object->mdlId[0], &playerWork->position);
    Hu3DModelRotSet(object->mdlId[0], 0.0f, playerWork->currentYaw, 0.0f);
}

s32 M460UpdatePlayerMovement(OMOBJ *object, float maximumSpeed, float acceleration)
{
    Vec targetOffset;
    float movementSpeed;
    M460PlayerWork *playerWork;
    s32 reachedTarget;

    playerWork = object->data;
    reachedTarget = 0;
    VECSubtract(&playerWork->targetPosition, &playerWork->position, &targetOffset);
    targetOffset.y = 0.0f;
    movementSpeed = VECMag(&targetOffset);
    if (movementSpeed < acceleration) {
        movementSpeed = 0.0f;
        reachedTarget = 1;
    }
    else {
        if (VECMag(&targetOffset) >= 100.0f) {
            movementSpeed = 100.0f;
        }
        movementSpeed = 0.2f + (0.008f * movementSpeed);
    }
    playerWork->movementAngle = atan2d(targetOffset.x, targetOffset.z);
    playerWork->movementSpeed = movementSpeed;
    if (playerWork->movementSpeed > maximumSpeed) {
        playerWork->movementSpeed = maximumSpeed;
    }
    return reachedTarget;
}

void M460UpdatePlayerAnimation(OMOBJ *object)
{
    float movementSpeed;
    float speedScale;

    M460PlayerWork *playerWork = object->data;
    if (playerWork->movementSpeed > 0.0f) {
        playerWork->targetYaw = playerWork->movementAngle;
    }
    if (playerWork->movementSpeed > 0.05f) {
        // 1 / REFRESH_RATE?
        playerWork->velocity.x = REFRESH_FREQ * (550.0 * (playerWork->movementSpeed * sind(playerWork->movementAngle)));
        playerWork->velocity.z = REFRESH_FREQ * (550.0 * (playerWork->movementSpeed * cosd(playerWork->movementAngle)));
        if (playerWork->isRunning != 0) {
            playerWork->velocity.x *= 1.5f;
            playerWork->velocity.z *= 1.5f;
        }
        playerWork->turnProgress = 0.2f;
        playerWork->turnSpeed = 0.0f;
        if (CharMotionShiftIDGet(playerWork->characterModelId) < 0) {
            movementSpeed = 0.5f;
            if (playerWork->motionId == 2) {
                movementSpeed = 0.45f;
            }
            if (playerWork->movementSpeed > movementSpeed) {
                if (playerWork->isRunning != 0) {
                    M460SetPlayerAction(object, 3);
                }
                else {
                    M460SetPlayerAction(object, 2);
                }
                speedScale = playerWork->movementSpeed;
            }
            else {
                M460SetPlayerAction(object, 1);
                speedScale = playerWork->movementSpeed / movementSpeed;
            }
            CharMotionSpeedSet(playerWork->characterModelId, speedScale);
        }
    }
    else {
        playerWork->velocity.x = playerWork->velocity.y = playerWork->velocity.z = 0.0f;
        if (CharMotionShiftIDGet(playerWork->characterModelId) < 0) {
            M460SetPlayerAction(object, 0);
        }
    }
}

void M460BeginPlayerAction(OMOBJ *object)
{
    M460PlayerWork *playerWork = object->data;
    if (frandmod(0x3E8) < 0x1F4) {
        playerWork->buttonPressed = 1;
        return;
    }
    playerWork->buttonReleased = 1;
}

void M460SetPlayerAction(OMOBJ *object, u32 motionAction)
{
    M460PlayerWork *playerWork = object->data;
    if ((playerWork->motionId != motionAction) && (motionAction < 8)) {
        float movementSpeed = REFRESH_RATE_F * playerMotionConfigs[motionAction].blendDuration;
        if (playerWork->motionId < 0) {
            movementSpeed = 0.0f;
        }
        playerWork->motionId = motionAction;
        CharMotionShiftSet(
            playerWork->characterModelId, object->mtnId[playerMotionConfigs[motionAction].motionIndex], REFRESH_RATE_F * playerMotionConfigs[motionAction].motionStartFrame, movementSpeed, playerMotionConfigs[motionAction].motionFlags);
        if (playerMotionConfigs[motionAction].motionEndFrame >= 0.0f) {
            Hu3DMotionShiftStartEndSet(object->mdlId[0], REFRESH_RATE_F * playerMotionConfigs[motionAction].motionStartFrame, REFRESH_RATE_F * playerMotionConfigs[motionAction].motionEndFrame);
        }
    }
}

s32 M460IsPlayerActionDone(OMOBJ *object)
{
    M460PlayerWork *playerWork = object->data;
    s32 actionIsDone = 0;
    if ((CharMotionEndCheck(playerWork->characterModelId) != 0) && (CharMotionShiftIDGet(playerWork->characterModelId) < 0)) {
        actionIsDone = 1;
    }
    return actionIsDone;
}

s32 M460SetPlayerActionIfDone(OMOBJ *object, u32 motionAction)
{
    s32 actionChanged = 0;
    if (M460IsPlayerActionDone(object)) {
        M460SetPlayerAction(object, motionAction);
        actionChanged = 1;
    }
    return actionChanged;
}

s32 spectatorMotionFileIds[5] = {
    DATA_MAKE_NUM(DATADIR_M460, 0x1E),
    DATA_MAKE_NUM(DATADIR_M460, 0x1F),
    DATA_MAKE_NUM(DATADIR_M460, 0x20),
    DATA_MAKE_NUM(DATADIR_M460, 0x21),
    DATA_MAKE_NUM(DATADIR_M460, 0x22),
};

void M460InitializeSpectator(OMOBJ *object)
{
    s32 modelId;
    s32 motionIndex;

    omSetTra(object, 0.0f, 0.0f, 0.0f);
    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M460, 0x1D));
    object->mdlId[0] = modelId;
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelShadowSet(modelId);
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    for (motionIndex = 0; motionIndex < 5; motionIndex++) {
        object->mtnId[motionIndex] = Hu3DJointMotionFile(modelId, spectatorMotionFileIds[motionIndex]);
    }
    CharNpcDustVoiceOffSet(modelId, object->mtnId[1], 0);
    CharNpcDustVoiceOffSet(modelId, object->mtnId[2], 1);
    CharNpcDustVoiceOffSet(modelId, object->mtnId[3], 1);
    object->work[1] = 0;
    object->objFunc = M460UpdateSpectator;
}

void M460UpdateSpectator(OMOBJ *object)
{
    float interpolation;
    float movementSpeed;
    s32 spectatorSide;
    s32 motionFrame;

    Vec spectatorStartPositions[2] = {
        { -250.0f, 0.0f, -2200.0f },
        { 250.0f, 0.0f, -2200.0f },
    };
    Vec playerOffset;
    s32 footstepMotionFrames[2] = { 6, 21 };
    switch (object->work[1]) {
        case 0:
            if ((M460GetRoundState() == 3) && (M460IsGameComplete() != 0) && (M460IsSelectionConfirmed() != 0)) {
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 0.0f, HU3D_MOTATTR_LOOP);
                spectatorSide = M460GetSelectionState();
                object->trans.x = spectatorStartPositions[spectatorSide].x;
                object->trans.y = spectatorStartPositions[spectatorSide].y;
                object->trans.z = spectatorStartPositions[spectatorSide].z;
                object->rot.y = 0.0f;
                object->work[1] = 1;
                object->work[2] = 0;
            }
            break;
        case 1:
            object->work[2]++;
            if (object->work[2] > 0.3f * REFRESH_RATE_F) {
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[2], 0.0f, REFRESH_RATE_F / 2, HU3D_MOTATTR_LOOP);
                object->work[1] = 2;
            }
            break;
        case 2:
            playerOffset.x = (*playerObjects)->trans.x - object->trans.x;
            playerOffset.y = 0.0f;
            playerOffset.z = (*playerObjects)->trans.z - object->trans.z;
            object->rot.y = M460ApproachAngle(object->rot.y, atan2d(playerOffset.x, playerOffset.z), 0.1f);
            object->trans.z += 400.0f * REFRESH_FREQ;
            if (M460IsSelectionReady() != 0) {
                HuAudFXPlay(0x3C);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[3], 0.0f, REFRESH_RATE_F / 2, HU3D_MOTATTR_LOOP);
                object->work[1] = 3;
                object->work[3] = REFRESH_RATE;
            }
            break;
        case 3:
            Hu3DMotionSpeedSet(object->mdlId[0], 0.9f);
            motionFrame = Hu3DMotionTimeGet(object->mdlId[0]);
            for (spectatorSide = 0; spectatorSide < 2; spectatorSide++) {
                if (motionFrame == footstepMotionFrames[spectatorSide]) {
                    movementSpeed = 127.0f / REFRESH_RATE_F * object->work[3];
                    HuAudFXPlayVol(0x6B, movementSpeed);
                }
            }
            playerOffset.x = (*playerObjects)->trans.x - object->trans.x;
            playerOffset.y = 0.0f;
            playerOffset.z = (*playerObjects)->trans.z - object->trans.z;
            object->rot.y = M460ApproachAngle(object->rot.y, atan2d(playerOffset.x, playerOffset.z), 0.1f);
            if (object->trans.z < -900.0f) {
                object->trans.z += 700.0f / REFRESH_RATE_F;
            }
            else {
                interpolation = VECMag(&playerOffset);
                if (interpolation < 20.0f) {
                    interpolation = 0.0f;
                }
                else {
                    if (VECMag(&playerOffset) >= 100.0f) {
                        interpolation = 100.0f;
                    }
                    interpolation = 0.2f + (0.008f * interpolation);
                    interpolation *= 700.0f / REFRESH_RATE_F;
                }
                object->trans.x = object->trans.x + (interpolation * sind(object->rot.y));
                object->trans.z = object->trans.z + (interpolation * cosd(object->rot.y));
            }
            if ((object->trans.z > 450.0f) && (object->work[3] != 0)) {
                object->work[3]--;
            }
            break;
    }
}

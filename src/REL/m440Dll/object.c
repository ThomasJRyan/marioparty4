#include "REL/m440Dll.h"

#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/flag.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/pad.h"
#include "math.h"
#include "string.h"
#include "version.h"

#ifdef __MWERKS__
#include "game/audio.h"
#endif

#undef REFRESH_RATE_F
#if VERSION_NTSC
#define REFRESH_RATE_F 60.0f
#else
#define REFRESH_RATE_F 49.998f
#endif

// bss
OMOBJ *playerObjects[4];
s16 playerOrder[4];
Vec playerStartingPositions[4];
f32 playerMotionProgress;
Vec playerMotionOrigin;
s16 playerOrbitState;
u8 playerTurnStarted;
s16 playerRewardState;

// data
s16 remainingRewardCount = 2;
s16 currentRewardIndex = -1;
u32 playerMotionAssets[8][2] = { { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00), 0 }, { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02), 0 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03), 0 }, { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x38), 0 }, { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x48), 0 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17), 0 }, { 0x47000C, 1 }, { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x36), 0 } };
f32 playerTurnAngles[4] = { 210.0f, 190.0f, 170.0f, 0.0f };

// prototypes
void InitializePlayerPositions(OMOBJ *);

void CreatePlayerObjects(HUPROCESS *objectManager)
{
    s16 playerIndex;

    omMakeGroupEx(objectManager, 0, 4);
    omGetGroupMemberListEx(objectManager, 0);

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObjects[playerIndex] = omAddObjEx(objectManager, 0x64, 9, 8, 0, InitializePlayerObject);
        playerObjects[playerIndex]->work[0] = playerIndex;
    }
    omAddObjEx(objectManager, 0x65, 0, 0, -1, InitializePlayerPositions);
}

void InitializePlayerObject(OMOBJ *object)
{
    f32 characterRadius;
    s32 playerIndex;
    s16 motionIndex;
    PlayerWork *playerWork;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, 0x78, HU_MEMNUM_OVL);
    playerWork = (PlayerWork *)object->data;
    memset(playerWork, 0, 0x78);
    object->stat |= 0x100;
    playerWork->playerIndex = object->work[0];
    playerWork->padIndex = GWPlayerCfg[playerWork->playerIndex].pad_idx;
    playerWork->characterId = GWPlayerCfg[playerWork->playerIndex].character;
    object->mdlId[0] = CharModelCreate(playerWork->characterId, 4);
    Hu3DModelLayerSet(object->mdlId[0], 1);
    Hu3DModelShadowSet(object->mdlId[0]);

    for (motionIndex = 0; motionIndex < 8; motionIndex++) {
        if (playerMotionAssets[motionIndex][1] == 0) {
            object->mtnId[motionIndex] = CharMotionCreate(playerWork->characterId, playerMotionAssets[motionIndex][0]);
        }
        else {
            object->mtnId[motionIndex] = CharMotionCreate(playerWork->characterId, playerWork->characterId + playerMotionAssets[motionIndex][0]);
        }
    }
    CharMotionVoiceOnSet(playerWork->characterId, object->mtnId[4], 0);
    CharMotionDataClose(playerWork->characterId);
    Hu3DMotionSet(object->mdlId[0], object->mtnId[0]);
    playerWork->position.x = playerWork->position.y = playerWork->position.z = 0.0f;
    playerWork->rotation.x = playerWork->rotation.y = playerWork->rotation.z = 0.0f;
    playerWork->entryRotation.y = 90.0f;
    playerWork->stateFrame = 0;
    playerWork->stateDuration = 2 * REFRESH_RATE_F;
    playerWork->inputFrameCount = playerWork->turnFrameCount = playerWork->movementFrameCount = 0;
    if (playerWork->characterId == 5) {
        characterRadius = 350.0f;
    }
    else {
        characterRadius = 410.0f;
    }
    playerWork->characterRadius = characterRadius;
    playerIndex = playerWork->playerIndex;
    if (_CheckFlag(FLAG_ID_MAKE(1, 12)) == 0) {
        GWPlayer[playerIndex].coin_win = 0;
    }
    object->objFunc = IdlePlayerObject;
}

void IdlePlayerObject(OMOBJ *object)
{
    return;
}

void InitializePlayerPositions(OMOBJ *object)
{
    s16 playerIndex;
    s16 playerSlot;
    s32 firstPlayerIndex;
    u32 unusedInteger0;
    u32 unusedInteger3;
    u32 unusedInteger3Alternate;
    PlayerWork *playerWork;

    firstPlayerIndex = GWSystem.player_curr;
    if (firstPlayerIndex < 0) {
        firstPlayerIndex = 0;
    }
    playerIndex = firstPlayerIndex;

    for (playerSlot = 0; playerSlot < 4; playerSlot++) {
        playerOrder[playerSlot] = playerIndex;
        playerIndex = (playerIndex + 1) % 4;
    }
    playerStartingPositions->x = (700.0f * cos(M_PI / 2 - M_PI / 9));
    playerStartingPositions->z = (700.0f * sin(M_PI / 2 - M_PI / 9));

    for (playerSlot = 1; playerSlot < 4; playerSlot++) {
        playerStartingPositions[playerSlot].x = playerStartingPositions[playerSlot - 1].x - 150.0f;
        playerStartingPositions[playerSlot].z = playerStartingPositions->z;
    }

    for (playerSlot = 0; playerSlot < 4; playerSlot++) {
        playerWork = playerObjects[playerOrder[playerSlot]]->data;
        playerWork->playerSlot = playerSlot;
        playerWork->targetPosition.x = (playerStartingPositions[playerSlot].x - 1000.0f);
        playerWork->targetPosition.y = 0.0f;
        playerWork->targetPosition.z = playerStartingPositions->z;
    }
    object->objFunc = UpdatePlayerObjects;
}

void UpdatePlayerObjects(OMOBJ *object)
{
    f32 motionProgress;
    HU3DMODEL *playerModel;
    f32 stateProgress;
    s16 targetPositionIndex;
    s16 playerSlot;
    OMOBJ *playerObject;
    PlayerWork *playerWork;

    for (playerSlot = 0; playerSlot < 4; playerSlot++) {
        if (playerOrder[playerSlot] != -1) {
            playerObject = playerObjects[playerOrder[playerSlot]];
            playerModel = &Hu3DData[playerObject->mdlId[0]];
            playerWork = (PlayerWork *)playerObject->data;
            switch (playerWork->stateFlags & 0xF) {
                case 1:
                    playerWork->stateFrame++;
                    stateProgress = (f32)playerWork->stateFrame / playerWork->stateDuration;
                    if (stateProgress >= 1.0f) {
                        stateProgress = 1.0f;
                        playerWork->stateFrame = 0;
                        if (playerTurnStarted == 0) {
                            playerTurnStarted = 1;
                            ChoosePlayerRewards();
                        }
                        else {
                            SetPlayerState(playerWork, 15, 7);
                            Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[1], 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
                        }
                    }
                    targetPositionIndex = (playerWork->playerSlot == -1) ? 0 : playerWork->playerSlot;
                    playerWork->position.x = playerWork->targetPosition.x + (playerStartingPositions[targetPositionIndex].x - playerWork->targetPosition.x) * stateProgress;
                    playerWork->position.y = 0.0f;
                    playerWork->position.z = playerWork->targetPosition.z + (playerStartingPositions[targetPositionIndex].z - playerWork->targetPosition.z) * stateProgress;
                    playerWork->movementFrameCount++;
                    stateProgress = playerWork->movementFrameCount / (REFRESH_RATE_F / 3.0f);
                    if (stateProgress >= 1.0f) {
                        stateProgress = 1.0f;
                    }
                    playerWork->rotation.y = playerWork->entryRotation.y + (90.0f - playerWork->entryRotation.y) * stateProgress;
                    break;
                case 7:
                    playerWork->stateFrame++;
                    stateProgress = (f32)playerWork->stateFrame / (2.0f * REFRESH_RATE_F / 3);
                    if (stateProgress >= 1.0f) {
                        stateProgress = 1.0f;
                        playerWork->stateFrame = 0;
                        SetPlayerState(playerWork, 15, 8);
                        Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[0], 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
                    }
                    playerWork->rotation.y = 90.0f + (playerTurnAngles[playerWork->playerSlot] - 90.0f) * stateProgress;
                    playerWork->entryRotation = playerWork->rotation;
                    break;
                case 2:
                    AnimatePlayerOrbit(playerObject, playerWork);
                    break;
                case 3:
                    if (isRewardSequenceReady() != 0) {
                        StartPlayerReveal();
                        UpdatePlayerApproach(playerObject, playerWork);
                        UpdatePlayerReward(playerObject, playerWork);
                        UpdatePlayerMotion(playerObject, playerWork);
                    }
                    break;
                case 4:
                    UpdatePlayerCelebration(playerObject, playerWork);
                    break;
                case 5:
                    UpdatePlayerExit(playerObject, playerWork);
                    break;
                case 6:
                    if (UpdatePlayerCoinReward(playerObject, playerWork) != 0) {
                        playerOrder[playerSlot] = -1;
                    }
                    break;
                case 0:
                case 8:
                    break;
            }
            Hu3DModelPosSet(playerObject->mdlId[0], playerWork->position.x, playerWork->position.y, playerWork->position.z);
            Hu3DModelRotSet(playerObject->mdlId[0], playerWork->rotation.x, playerWork->rotation.y, playerWork->rotation.z);
        }
    }
}

f32 playerOrbitAngle = 70.0f;

void AnimatePlayerOrbit(OMOBJ *object, PlayerWork *playerWork)
{
    s16 availableModelOrder[5] = { 3, 1, 0, 2, 4 };
    f32 positionDeltaX;
    f32 positionDeltaZ;
    f32 stateProgress;
    f32 targetRotation;
    s16 unusedIndex0;
    s16 unusedIndex0Alternate;
    s16 unusedIndex3;
    s16 modelIndex;

    switch (playerOrbitState) {
        case 0:
            playerOrbitAngle -= (REFRESH_RATE_F / 120);
            if (playerOrbitAngle <= 45.0f) {
                playerOrbitAngle = 45.0f;
                playerOrbitState++;
            }
            playerWork->targetPosition = playerWork->position;
            playerWork->position.x = (700.0 * cosd(playerOrbitAngle));
            playerWork->position.z = (700.0 * sind(playerOrbitAngle));
            positionDeltaX = (playerWork->position.x - playerWork->targetPosition.x);
            positionDeltaZ = (playerWork->position.z - playerWork->targetPosition.z);
            targetRotation = atan2d(positionDeltaX, positionDeltaZ);
            if (targetRotation < 0.0f) {
                targetRotation += 360.0f;
            }
            playerWork->rotationStartAngle = targetRotation;
            playerWork->rotation.y = targetRotation;
            return;
        case 1:
            playerWork->stateDuration++;
            stateProgress = playerWork->stateDuration / (REFRESH_RATE_F / 6.0f);
            if (stateProgress >= 1.0f) {
                stateProgress = 1.0f;
            }
            targetRotation = 225.0f;
            playerWork->rotation.y = playerWork->rotationStartAngle + (stateProgress * (targetRotation - playerWork->rotationStartAngle));
            playerWork->stateFrame++;
            stateProgress = playerWork->stateFrame / (5.0f * REFRESH_RATE_F / 6.0f);
            if (stateProgress >= 1.0f) {
                stateProgress = 1.0f;
                playerWork->stateFrame = playerWork->stateDuration = 0;
                playerWork->rotationStartAngle = playerWork->rotation.y;

                for (modelIndex = 0; modelIndex < 5; modelIndex++) {
                    if (rewardModelVisibility[availableModelOrder[modelIndex]] != 0) {
                        playerWork->selectedModelId = playerWork->rewardIndex = availableModelOrder[modelIndex];
                        break;
                    }
                }
                playerOrbitState++;
            }
            positionDeltaX = 700.0f + ((playerWork->characterRadius - 700.0f) * stateProgress);
            playerWork->position.x = (positionDeltaX * cos(M_PI / 4));
            playerWork->position.z = (positionDeltaX * sin(M_PI / 4));
            positionDeltaX = 0.1f;
            positionDeltaZ = 0.6f;
            if (stateProgress >= positionDeltaX) {
                stateProgress -= positionDeltaX;
                stateProgress *= (1.0f / positionDeltaZ);
                if (stateProgress > 1.0f) {
                    stateProgress = 1.0f;
                }
                playerWork->position.y = 150.0f * stateProgress;
            }
            break;
        case 2:
            playerOrbitAngle += REFRESH_RATE_F / 60.0f;
            if (playerOrbitAngle >= playerTurnAngles[playerWork->rewardIndex]) {
                playerOrbitAngle = playerTurnAngles[playerWork->rewardIndex];
                playerOrbitState++;
            }
            playerWork->targetPosition = playerWork->position;
            playerWork->position.x = (playerWork->characterRadius * cosd(playerOrbitAngle));
            playerWork->position.z = (playerWork->characterRadius * sind(playerOrbitAngle));
            positionDeltaX = (playerWork->position.x - playerWork->targetPosition.x);
            positionDeltaZ = (playerWork->position.z - playerWork->targetPosition.z);
            targetRotation = atan2d(positionDeltaX, positionDeltaZ);
            if (targetRotation < 0.0f) {
                targetRotation += 360.0f;
            }
            playerWork->stateFrame++;
            stateProgress = playerWork->stateFrame / (REFRESH_RATE_F / 6.0f);
            if (stateProgress < 1.0f) {
                playerWork->rotation.y = playerWork->rotationStartAngle + (stateProgress * (targetRotation - playerWork->rotationStartAngle));
            }
            else {
                playerWork->rotation.y = targetRotation;
            }
            playerWork->rotationTargetAngle = playerWork->rotation.y;
            return;
        case 3:
            playerWork->stateDuration++;
            stateProgress = playerWork->stateDuration / (REFRESH_RATE_F / 6.0f);
            if (stateProgress >= 1.0f) {
                stateProgress = 1.0f;
                playerWork->stateFrame = playerWork->stateDuration = 0;
                playerOrbitState++;
                Hu3DMotionShiftSet(*object->mdlId, *object->mtnId, 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
            }

            targetRotation = 90.0f - playerTurnAngles[playerWork->rewardIndex];
            playerWork->rotation.y = interpolateAngle(playerWork->rotationTargetAngle, targetRotation, stateProgress);
            return;
        case 4:
            if (++playerWork->stateFrame > (REFRESH_RATE / 4)) {
                playerOrbitState++;
            }
            break;
        case 5:
            if (currentRewardIndex < 0) {
                ResetPlayerSequenceState();
                currentRewardIndex++;
                return;
            }
            if (currentRewardIndex == 0) {
                if (isPlayerSequenceComplete() == 0) {
                    currentRewardIndex++;
                    return;
                }
            }
            else {
                playerOrbitState = 0;
                playerOrbitAngle = 70.0f;
                playerWork->rewardStateFlags = 0;
                playerWork->stateFrame = playerWork->stateDuration = 0;
                playerWork->rotationStartAngle = playerTurnAngles[playerWork->rewardIndex];
                playerWork->rotationTargetAngle = 0.0f;
                playerWork->rewardIndex = 0;
                SetPlayerMotionTiming(playerWork, playerWork->rotation.y, playerWork->rotation.y);
                SetPlayerState(playerWork, 15, 3);
                StartPlayerExit();
            }
            break;
    }
}

void UpdatePlayerApproach(OMOBJ *object, PlayerWork *playerWork)
{
    s16 availableModelChoices[5];
    s16 availableModelOrder[5];
    f32 triggerProgress;
    f32 triggerDuration;
    f32 angleDifference;
    f32 stateProgress;
    s16 unusedIndex0;
    s16 unusedIndex0Alternate;
    s16 unusedIndex3;
    s16 availableModelCount;
    s16 choiceCount;
    s16 choiceIndex;

    if (GWPlayerCfg[playerWork->playerIndex].iscom != 0) {
        HuPadStkX[playerWork->padIndex] = 0;
        HuPadStkY[playerWork->padIndex] = 0;
        HuPadTrigR[playerWork->padIndex] = 0;
        HuPadBtn[playerWork->padIndex] = 0;
        switch (GWPlayerCfg[playerWork->playerIndex].diff) {
            case 0:
                stateProgress = 0.3f;
                break;
            case 1:
                stateProgress = 0.3f;
                break;
            case 2:
                stateProgress = 0.3f;
                break;
            case 3:
                stateProgress = 0.3f;
                break;
        }

        stateProgress += (0.1f * playerRewardState);
        for (choiceIndex = 0, availableModelCount = 0; choiceIndex < 5; choiceIndex++) {
            if (rewardModelVisibility[choiceIndex] != 0) {
                availableModelOrder[availableModelCount++] = choiceIndex;
            }
        }
        availableModelCount--;
        if (playerWork->selectedRewardId == -1) {
            if (playerWork->selectionThreshold < stateProgress) {
                playerWork->selectedRewardId = getEffectModelId(-1);
            }
            else {
                for (choiceIndex = 0, choiceCount = 0; choiceIndex < 5; choiceIndex++) {
                    if ((choiceIndex != getEffectModelId(-1)) && (rewardModelVisibility[choiceIndex] != 0)) {
                        availableModelChoices[choiceCount++] = choiceIndex;
                    }
                }
                choiceCount--;
                choiceIndex = (((rand8() << 8) | rand8()) % (s16)(choiceCount + 1));
                playerWork->selectedRewardId = availableModelChoices[choiceIndex];
            }
            choiceIndex = (((rand8() << 8) | rand8()) % (s16)(availableModelCount + 1));
            playerWork->selectedRewardOrder = availableModelOrder[choiceIndex];
        }
        if (playerWork->remainingRewardChoices == 0) {
            angleDifference = ABS(playerWork->rotationStartAngle - playerTurnAngles[playerWork->selectedRewardId]);
            if (angleDifference < 5.0f) {
                HuPadTrigR[playerWork->padIndex] = 0x96;
                HuPadBtn[playerWork->padIndex] = 0x20;
                return;
            }
            if (playerWork->rotationStartAngle < playerTurnAngles[playerWork->selectedRewardId]) {
                HuPadStkX[playerWork->padIndex] = -0x2D;
                return;
            }
            if (playerWork->rotationStartAngle > playerTurnAngles[playerWork->selectedRewardId]) {
                HuPadStkX[playerWork->padIndex] = 0x2D;
            }
        }
        else {
            angleDifference = ABS(playerWork->rotationStartAngle - playerTurnAngles[playerWork->selectedRewardOrder]);
            if (angleDifference < 5.0f) {
                if (playerWork->buttonRepeatActive != 0) {
                    playerWork->selectionFrameCount++;
                    stateProgress = playerWork->selectionFrameCount / (2.0f * REFRESH_RATE_F / 3.0f);
                    if (stateProgress > 1.0f) {
                        stateProgress = 1.0f;
                        playerWork->buttonRepeatActive = 0;
                        playerWork->remainingRewardChoices--;
                        choiceIndex = (((rand8() << 8) | rand8()) % (s16)(availableModelCount + 1));
                        playerWork->selectedRewardOrder = availableModelOrder[choiceIndex];
                    }
                    HuPadTrigR[playerWork->padIndex] = (u8)(playerWork->buttonRepeatDelay * stateProgress);
                    return;
                }
                if ((((rand8() << 8) | rand8()) % 101) < 0x32) {
                    playerWork->selectionFrameCount = 0;
                    playerWork->buttonRepeatDelay = ((((rand8() << 8) | rand8()) % 81) + 0x32);
                    playerWork->buttonRepeatActive = 1;
                    return;
                }
                playerWork->remainingRewardChoices--;
                choiceIndex = (((rand8() << 8) | rand8()) % (s16)(availableModelCount + 1));
                playerWork->selectedRewardOrder = availableModelOrder[choiceIndex];
                return;
            }
            if (playerWork->rotationStartAngle < playerTurnAngles[playerWork->selectedRewardOrder]) {
                HuPadStkX[playerWork->padIndex] = -0x2D;
                return;
            }
            if (playerWork->rotationStartAngle > playerTurnAngles[playerWork->selectedRewardOrder]) {
                HuPadStkX[playerWork->padIndex] = 0x2D;
            }
        }
    }
}

void UpdatePlayerReward(OMOBJ *object, PlayerWork *playerWork)
{
    s16 stickY;
    f32 targetX;
    f32 targetZ;
    f32 turnProgress;
    f32 targetRotation;
    f32 targetHeading;
    f32 interpolatedHeading;
    f32 startHeading;
    s16 directionFlags;
    s16 nextTargetIndex;
    s16 stickX;

    if (getCurrentSequencePlayer() < -1) {
        StartPlayerCelebration();
        HuPadStkX[playerWork->padIndex] = 0;
        HuPadStkY[playerWork->padIndex] = 0;
        HuPadTrigR[playerWork->padIndex] = 0x96;
        HuPadBtn[playerWork->padIndex] = 0x20;
    }
    else {
        stickX = HuPadStkX[playerWork->padIndex];
        stickY = HuPadStkY[playerWork->padIndex];
    }
    if ((playerWork->rewardStateFlags & 8) == 0) {
        nextTargetIndex = -1;
        if (ABS(stickX) < 7) {
            nextTargetIndex = GetPlayerStateFlags(playerWork, 0);
        }
        else {
            if (stickX > 0) {
                playerWork->rewardStateFlags = playerWork->rewardStateFlags & 0xFFFFFFFD;
                playerWork->rewardStateFlags = playerWork->rewardStateFlags | 1;
            }
            else {
                playerWork->rewardStateFlags = playerWork->rewardStateFlags & 0xFFFFFFFE;
                playerWork->rewardStateFlags = playerWork->rewardStateFlags | 2;
            }
            nextTargetIndex = GetPlayerStateFlags(playerWork, 1);
            if (nextTargetIndex >= 0x64) {
                nextTargetIndex -= 0x64;
                playerWork->selectedModelId = nextTargetIndex;
                playerWork->rewardStateFlags = playerWork->rewardStateFlags & 0xFFFFFFFC;
            }
        }
        if ((playerWork->rewardStateFlags & 1) != 0) {
            playerWork->rotationStartAngle -= REFRESH_RATE_F / 60.0f;
            if ((nextTargetIndex != -1) && (playerTurnAngles[nextTargetIndex] > playerWork->rotationStartAngle)) {
                playerWork->rotationStartAngle = playerTurnAngles[nextTargetIndex];
                playerWork->selectedModelId = nextTargetIndex;
                playerWork->rewardStateFlags = playerWork->rewardStateFlags & 0xFFFFFFFE;
            }
        }
        else if ((playerWork->rewardStateFlags & 2) != 0) {
            playerWork->rotationStartAngle += REFRESH_RATE_F / 60.0f;
            if ((nextTargetIndex != -1) && (playerTurnAngles[nextTargetIndex] < playerWork->rotationStartAngle)) {
                playerWork->rotationStartAngle = playerTurnAngles[nextTargetIndex];
                playerWork->selectedModelId = nextTargetIndex;
                playerWork->rewardStateFlags = playerWork->rewardStateFlags & 0xFFFFFFFD;
            }
        }
        playerWork->targetPosition = playerWork->position;
        playerWork->position.x = (playerWork->characterRadius * cosd(playerWork->rotationStartAngle));
        playerWork->position.z = (playerWork->characterRadius * sind(playerWork->rotationStartAngle));
        if ((playerWork->rewardStateFlags & 3) != 0) {
            playerWork->rewardStateFlags = playerWork->rewardStateFlags & ~4;
            targetX = (playerWork->position.x - playerWork->targetPosition.x);
            targetZ = (playerWork->position.z - playerWork->targetPosition.z);
            targetHeading = atan2d(targetX, targetZ);
            if (targetHeading < 0.0f) {
                targetHeading += 360.0f;
            }
            SetPlayerMotionTiming(playerWork, playerWork->rotation.y, targetHeading);
        }
        else {
            playerWork->rewardStateFlags |= 4;
            targetHeading = 90.0f - playerTurnAngles[playerWork->selectedModelId];
            SetPlayerMotionTiming(playerWork, playerWork->rotation.y, targetHeading);
        }
        playerWork->stateFrame++;
        turnProgress = (f32)playerWork->stateFrame / playerWork->stateDuration;
        if (playerWork->stateFrame >= playerWork->stateDuration) {
            playerWork->stateFrame = playerWork->stateDuration;
            turnProgress = 1.0f;
            if ((playerWork->rewardStateFlags & 4) != 0) {
                SetPlayerMotion(object, 0, HU3D_MOTATTR_LOOP);
            }
            else {
                SetPlayerMotion(object, 2, HU3D_MOTATTR_LOOP);
            }
        }
        else {
            SetPlayerMotion(object, 2, HU3D_MOTATTR_LOOP);
        }
        if ((playerWork->rewardStateFlags & 4) != 0) {
            playerWork->rotation.y = interpolateAngle(playerWork->rotationTargetAngle, playerWork->reserved70, turnProgress);
            return;
        }
        startHeading = playerWork->rotationTargetAngle;
        targetRotation = playerWork->reserved70;
        if ((playerWork->rewardStateFlags & 1) != 0) {
            if (startHeading > targetRotation) {
                startHeading -= 360.0f;
            }
        }
        else if (((playerWork->rewardStateFlags & 2) != 0) && (startHeading < targetRotation)) {
            startHeading += 360.0f;
        }
        interpolatedHeading = startHeading + (turnProgress * (targetRotation - startHeading));
        if (interpolatedHeading < 0.0f) {
            interpolatedHeading += 360.0f;
        }
        else {
            if (interpolatedHeading >= 360.0f) {
                interpolatedHeading -= 360.0f;
            }
        }
        playerWork->rotation.y = interpolatedHeading;
    }
}

s16 GetPlayerStateFlags(PlayerWork *playerWork, u8 includeCurrentPlayer)
{
    f32 nearestTargetDistance;
    s16 nearestTargetIndex;
    s16 targetIndex;

    nearestTargetDistance = 999.0f;
    nearestTargetIndex = -1;
    if ((playerWork->rewardStateFlags & 1) != 0) {
        for (targetIndex = 0; targetIndex < 5; targetIndex++) {
            if (rewardModelVisibility[targetIndex] != 0) {
                if (includeCurrentPlayer == 0) {
                    if (playerTurnAngles[targetIndex] > playerWork->rotationStartAngle) {
                        continue;
                    }
                }
                else if (playerTurnAngles[targetIndex] >= playerWork->rotationStartAngle) {
                    continue;
                }
                if (nearestTargetDistance > (playerWork->rotationStartAngle - playerTurnAngles[targetIndex])) {
                    nearestTargetIndex = targetIndex;
                    nearestTargetDistance = playerWork->rotationStartAngle - playerTurnAngles[targetIndex];
                }
            }
        }
    }
    else if ((playerWork->rewardStateFlags & 2) != 0) {
        for (targetIndex = 0; targetIndex < 5; targetIndex++) {
            if (rewardModelVisibility[targetIndex] != 0) {
                if (includeCurrentPlayer == 0) {
                    if (playerTurnAngles[targetIndex] < playerWork->rotationStartAngle) {
                        continue;
                    }
                }
                else if (playerTurnAngles[targetIndex] <= playerWork->rotationStartAngle) {
                    continue;
                }
                if (nearestTargetDistance > (playerTurnAngles[targetIndex] - playerWork->rotationStartAngle)) {
                    nearestTargetIndex = targetIndex;
                    nearestTargetDistance = playerTurnAngles[targetIndex] - playerWork->rotationStartAngle;
                }
            }
        }
    }
    if ((nearestTargetIndex == -1) && (includeCurrentPlayer != 0)) {
        for (targetIndex = 0; targetIndex < 5; targetIndex++) {
            if ((rewardModelVisibility[targetIndex] != 0) && (playerWork->rotationStartAngle == playerTurnAngles[targetIndex])) {
                nearestTargetIndex = targetIndex + 0x64;
            }
        }
    }
    return nearestTargetIndex;
}

void SetPlayerMotionTiming(PlayerWork *playerWork, f32 startAngle, f32 targetAngle)
{
    f32 rotationDuration;
    f32 rotationStart;
    f32 rotationEnd;
    f32 rotationDistance;

    rotationDuration = REFRESH_RATE_F / 6.0f;
    playerWork->rotationTargetAngle = startAngle;
    playerWork->reserved70 = targetAngle;
    rotationDistance = ABS(startAngle - targetAngle);
    if (rotationDistance > 180.0f) {
        rotationDistance -= 360.0f;
    }
    rotationDistance = ABS(rotationDistance);
    playerWork->stateFrame = 0;
    playerWork->stateDuration = ((rotationDuration / 90.0f) * rotationDistance);
}

void UpdatePlayerMotion(OMOBJ *object, PlayerWork *playerWork)
{
    HU3DMODEL *playerModel;
    f32 motionDuration;
    f32 motionStartTime;
    f32 triggerValue;
    f32 motionProgress;
    f32 motionTimeDifference;

    playerModel = &Hu3DData[object->mdlId[0]];
    motionStartTime = 7.0f;
    motionDuration = Hu3DMotionMaxTimeGet(object->mdlId[0]) - motionStartTime;
    triggerValue = HuPadTrigR[playerWork->padIndex];
    if ((playerModel->motIdShift == -1) && (playerModel->motId == object->mtnId[0]) && ((playerWork->rewardStateFlags & 8) == 0) && (triggerValue > 10.0f)) {
        playerWork->rewardStateFlags |= 8;
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[3], 0.0f, 7.0f, HU3D_MOTATTR_NONE);
    }
    if ((playerWork->rewardStateFlags & 8) != 0) {
        if (triggerValue <= 10.0f) {
            SetEffectParameter(playerWork->selectedModelId, -1.0f);
            playerWork->rewardStateFlags &= ~8;
            playerModel->motWork.speed = 1.0f;
            Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
        }
        else if (triggerValue > 10.0f) {
            if ((HuPadBtn[playerWork->padIndex] & 0x20) != 0) {
                playerWork->rewardIndex = 0;
                playerWork->rotationStartAngle = triggerValue;
                playerWork->rotationTargetAngle = 0.0f;
                playerWork->rewardStateFlags = 0;
                rewardModelVisibility[playerWork->selectedModelId] = 0;
                if (getCurrentSequencePlayer() < (4.5f * REFRESH_RATE)) {
                    StartPlayerCelebration();
                }
                SetPlayerState(playerWork, 15, 4);
            }
            if (playerModel->motIdShift == -1) {
                playerModel->motWork.speed = 0.0f;
                motionProgress = 0.006666667f * (triggerValue - 10.0f);
                if (motionProgress > 0.8f) {
                    motionProgress = 0.8f;
                }
                motionProgress = motionStartTime + (motionProgress * motionDuration);
                motionTimeDifference = ABS(playerModel->motWork.time - motionProgress);
                if (motionTimeDifference < 1.0f) {
                    playerModel->motWork.time = motionProgress;
                }
                if (playerModel->motWork.time < motionProgress) {
                    playerModel->motWork.time += 1.0f;
                }
                else if (playerModel->motWork.time > motionProgress) {
                    playerModel->motWork.time -= 1.0f;
                }
                motionProgress = (playerModel->motWork.time - motionStartTime) / motionDuration;
                SetEffectParameter(playerWork->selectedModelId, motionProgress);
            }
        }
        playerWork->position.x = (410.0 * cosd(playerTurnAngles[playerWork->selectedModelId]));
        playerWork->position.z = (410.0 * sind(playerTurnAngles[playerWork->selectedModelId]));
    }
}

void UpdatePlayerCelebration(OMOBJ *object, PlayerWork *playerWork)
{
    HU3DMODEL *playerModel;
    f32 motionDuration;
    f32 motionStartTime;
    f32 motionTimeDifference;
    f32 motionTimeBlend;
    f32 motionProgress;

    playerModel = &Hu3DData[object->mdlId[0]];
    motionStartTime = 7.0f;
    motionDuration = Hu3DMotionMaxTimeGet(object->mdlId[0]) - motionStartTime;
    switch (playerWork->rewardIndex) {
        case 0:
            if (playerModel->motIdShift == -1) {
                playerWork->rotationStartAngle += 40.0f;
                playerModel->motWork.speed = 0.0f;
                motionProgress = 0.006666667f * (playerWork->rotationStartAngle - 10.0f);
                if (motionProgress > 0.8f) {
                    motionProgress = 0.8f;
                }
                motionTimeBlend = 0.01f * (playerWork->rotationStartAngle - 130.0f);
                if (motionTimeBlend > 0.19999999f) {
                    motionTimeBlend = 0.19999999f;
                }
                else if (motionTimeBlend < 0.0f) {
                    motionTimeBlend = 0.0f;
                }
                motionProgress += motionTimeBlend;
                motionProgress = motionTimeBlend = motionStartTime + (motionProgress * motionDuration);
                motionTimeDifference = ABS(playerModel->motWork.time - motionProgress);
                if (motionTimeDifference < 1.0f) {
                    playerModel->motWork.time = motionProgress;
                }
                if (playerModel->motWork.time < motionProgress) {
                    playerModel->motWork.time += 3.0f;
                }
                else if (playerModel->motWork.time > motionProgress) {
                    playerModel->motWork.time -= 3.0f;
                }
                motionProgress = (playerModel->motWork.time - motionStartTime) / motionDuration;
                SetEffectParameter(playerWork->selectedModelId, motionProgress);
                playerWork->position.x = (410.0 * cosd(playerTurnAngles[playerWork->selectedModelId]));
                playerWork->position.z = (410.0 * sind(playerTurnAngles[playerWork->selectedModelId]));
                if (playerWork->rotationStartAngle >= 150.0f) {
                    playerWork->rotationStartAngle = 150.0f;
                    if (playerModel->motWork.time == motionTimeBlend) {
                        playerWork->stateFrame = 0;
                        playerWork->rotationStartAngle = playerWork->rotation.y;
                        playerWork->rotationTargetAngle = 180.0f + playerWork->rotation.y;
                        if (playerWork->rotationTargetAngle >= 360.0f) {
                            playerWork->rotationTargetAngle -= 360.0f;
                        }
                        playerWork->rewardIndex++;
                        HuAudFXPlay(0x70B);
                    }
                }
            }
            break;
        case 1:
            if (++playerWork->stateFrame > (REFRESH_RATE / 6)) {
                playerWork->stateFrame = 0;
                Hu3DMotionSpeedSet(object->mdlId[0], 2.0f);
                if ((playerWork->rotationTargetAngle - playerWork->rotationStartAngle) > 0.0f) {
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[7], 64.0f, 7.0f, HU3D_MOTATTR_NONE);
                }
                else {
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[7], 4.0f, 7.0f, HU3D_MOTATTR_NONE);
                }
                Hu3DMotionShiftSpeedSet(object->mdlId[0], 2.0f);
                playerWork->rewardIndex++;
            }
            break;
        case 2:
            playerWork->stateFrame++;
            motionProgress = playerWork->stateFrame / (0.5f * REFRESH_RATE_F);
            playerWork->position.x = ((410.0f + (motionProgress * (playerWork->characterRadius - 410.0f))) * cosd(playerTurnAngles[playerWork->selectedModelId]));
            playerWork->position.z = ((410.0f + (motionProgress * (playerWork->characterRadius - 410.0f))) * sind(playerTurnAngles[playerWork->selectedModelId]));
            if (motionProgress >= 1.0f) {
                playerWork->stateFrame = 0;
                motionProgress = 1.0f;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
                playerWork->rewardIndex++;
            }
            else if (((playerWork->rotationTargetAngle - playerWork->rotationStartAngle) < 0.0f) && (Hu3DMotionShiftIDGet(object->mdlId[0]) < 0)) {
                if (Hu3DMotionTimeGet(object->mdlId[0]) >= 60.0f) {
                    Hu3DMotionSpeedSet(object->mdlId[0], 0.0f);
                }
            }
            playerWork->rotation.y = interpolateAngle(playerWork->rotationStartAngle, playerWork->rotationTargetAngle, motionProgress);
            break;
        case 3:
            if (++playerWork->stateFrame > (s32)REFRESH_RATE_F) {
                if (getEffectModelId(playerWork->selectedModelId) == 0) {
                    SetEffectFlags(7U, 3U);
                }
                else {
                    SetEffectFlags(7U, 4U);
                }
                playerWork->stateFrame = 0;
                playerWork->rewardIndex++;
            }
            break;
        case 4:
            if (getEffectFlags(0x100U) != 0) {
                SetEffectFlags(0x100U, 0U);
                if (getEffectModelId(playerWork->selectedModelId) == 0) {
                    SetPlayerState(playerWork, 15, 5);
                }
                else {
                    SetPlayerState(playerWork, 15, 6);
                }
                playerWork->stateFrame = playerWork->stateDuration = 0;
                playerWork->rewardIndex = playerWork->reserved62 = 0;
                playerWork->rotationStartAngle = playerWork->rotationTargetAngle = 0.0f;
            }
            break;
    }
    if (getNextSequencePlayer() >= 0) {
        if (getCurrentSequencePlayer() < (4.5f * REFRESH_RATE)) {
            StartPlayerCelebration();
            return;
        }
        StartPlayerReveal();
    }
}

void UpdatePlayerExit(OMOBJ *object, PlayerWork *playerWork)
{
    HU3DMODEL *playerModel;
    f32 positionDeltaZ;
    f32 positionDeltaX;
    f32 targetRotation;
    f32 stateProgress;
    s16 availableModelCount;
    s16 modelIndex;

    playerModel = &Hu3DData[object->mdlId[0]];
    switch (playerWork->rewardIndex) {
        case 0:
            playerMotionProgress = playerTurnAngles[playerWork->selectedModelId];
#if VERSION_NTSC
            Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[4], 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
#else
            Hu3DMotionSet(object->mdlId[0], object->mtnId[4]);
#endif
            CharFXPlay(playerWork->characterId, 0x125);
            playerWork->rewardIndex++;
            break;
        case 1:
            if (playerModel->motWork.time >= (Hu3DMotionMaxTimeGet(object->mdlId[0]) - 1.0f)) {
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[2], 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
                playerWork->stateFrame = playerWork->stateDuration = 0;
                playerWork->rotationStartAngle = playerWork->rotation.y;

                for (modelIndex = 0, availableModelCount = -1; modelIndex < 5; modelIndex++) {
                    if (rewardModelVisibility[modelIndex] != 0) {
                        availableModelCount++;
                    }
                }
                if (availableModelCount == 0) {
                    playerRewardState++;
                    SetEffectFlags(0x18, 0x18);
                    HuAudFXPlay(0x70E);
                }
                playerWork->rewardIndex++;
            }
            break;
        case 2:
            playerMotionProgress += REFRESH_RATE_F / 60.0f;
            if (playerMotionProgress >= 135.0f) {
                playerMotionProgress = 135.0f;
                playerWork->stateDuration = playerWork->inputFrameCount = 0;
                ChoosePlayerRewards();
                playerWork->rewardIndex++;
            }
            playerWork->targetPosition = playerWork->position;
            playerWork->position.x = (playerWork->characterRadius * cosd(playerMotionProgress));
            playerWork->position.z = (playerWork->characterRadius * sind(playerMotionProgress));
            positionDeltaX = (playerWork->position.x - playerWork->targetPosition.x);
            positionDeltaZ = (playerWork->position.z - playerWork->targetPosition.z);
            targetRotation = atan2d(positionDeltaX, positionDeltaZ);
            if (targetRotation < 0.0f) {
                targetRotation += 360.0f;
            }
            playerWork->stateFrame++;
            stateProgress = playerWork->stateFrame / (REFRESH_RATE_F / 6.0f);
            if (stateProgress >= 1.0f) {
                stateProgress = 1.0f;
            }
            playerWork->rotation.y = interpolateAngle(playerWork->rotationStartAngle, targetRotation, stateProgress);
            playerWork->rotationTargetAngle = playerWork->rotation.y;
            break;
        case 3:
            playerWork->stateDuration++;
            stateProgress = playerWork->stateDuration / (REFRESH_RATE_F / 6.0f);
            if (stateProgress >= 1.0f) {
                stateProgress = 1.0f;
            }
            targetRotation = 315.0f;
            playerWork->rotation.y = playerWork->rotationTargetAngle + (stateProgress * (targetRotation - playerWork->rotationTargetAngle));
            playerWork->inputFrameCount++;
            stateProgress = playerWork->inputFrameCount / (5.0f * REFRESH_RATE_F / 6.0f);
            if (stateProgress >= 1.0f) {
                stateProgress = 1.0f;
                playerWork->stateFrame = playerWork->stateDuration = playerWork->inputFrameCount = 0;
                playerWork->rotationStartAngle = playerWork->rotation.y;
                playerWork->rewardIndex++;
            }
            positionDeltaX = playerWork->characterRadius + ((700.0f - playerWork->characterRadius) * stateProgress);
            playerWork->position.x = positionDeltaX * cosd(135);
            playerWork->position.z = positionDeltaX * sind(135);
            positionDeltaX = 0.3f;
            positionDeltaZ = 0.6f;
            if (stateProgress >= positionDeltaX) {
                stateProgress -= positionDeltaX;
                stateProgress *= (1.0f / positionDeltaZ);
                if (stateProgress > 1.0f) {
                    stateProgress = 1.0f;
                }
                playerWork->position.y = 150.0f * (1.0f - stateProgress);
            }
            break;
        case 4:
            playerMotionProgress -= REFRESH_RATE_F / 120;
            if (playerMotionProgress <= 110.0f) {
                playerMotionProgress = 110.0f;
                playerWork->rewardIndex = 0;
                playerWork->stateFrame = 0;
                playerWork->stateDuration = (3 - remainingRewardCount) * (s32)(0.5f * REFRESH_RATE_F);
                playerWork->movementFrameCount = 0;
                playerWork->entryRotation.y = 90.0f;
                SetPlayerState(playerWork, 15, 1);
            }
            playerWork->targetPosition = playerWork->position;
            playerWork->position.x = (700.0 * cosd(playerMotionProgress));
            playerWork->position.z = (700.0 * sind(playerMotionProgress));
            positionDeltaX = (playerWork->position.x - playerWork->targetPosition.x);
            positionDeltaZ = (playerWork->position.z - playerWork->targetPosition.z);
            targetRotation = atan2d(positionDeltaX, positionDeltaZ);
            if (targetRotation < 0.0f) {
                targetRotation += 360.0f;
            }
            playerWork->inputFrameCount++;
            stateProgress = playerWork->inputFrameCount / (REFRESH_RATE_F / 6.0f);
            if (stateProgress >= 1.0f) {
                stateProgress = 1.0f;
            }
            playerWork->rotation.y = interpolateAngle(playerWork->rotationStartAngle, targetRotation, stateProgress);
            break;
    }
}

u8 UpdatePlayerCoinReward(OMOBJ *object, PlayerWork *playerWork)
{
    HU3DMODEL *playerModel;
    f32 motionProgress;
    s32 playerIndex;
    s16 coinReward;

    playerModel = &Hu3DData[object->mdlId[0]];
    switch (playerWork->rewardIndex) {
        case 0:
            playerRewardState = 0;
            Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[6], 0.0f, 7.0f, HU3D_MOTATTR_NONE);
            CharFXPlay(playerWork->characterId, 0x12E);
            playerWork->rewardIndex++;
            break;
        case 1:
            if (getEffectFlags(7U) == 5) {
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
                playerWork->stateFrame = 0;
                playerWork->rotationStartAngle = playerWork->rotation.y;
                motionProgress = playerTurnAngles[playerWork->selectedModelId];
                playerMotionOrigin = playerWork->position;
                if ((playerWork->selectedModelId == 4) || (playerWork->selectedModelId == 3)) {
                    playerWork->targetPosition.x = (840.0000610351563 * cosd(motionProgress));
                    playerWork->targetPosition.y = 0.0f;
                    playerWork->targetPosition.z = (840.0000610351563 * sind(motionProgress));
                }
                else {
                    playerWork->targetPosition.x = (1050.0 * cosd(motionProgress));
                    playerWork->targetPosition.y = 0.0f;
                    playerWork->targetPosition.z = (1050.0 * sind(motionProgress));
                }
                playerWork->rotation.y = 0.0f;
                MTXRotRad(playerModel->mtx, 0x59, 0.017453292f * playerWork->rotationStartAngle);
                playerWork->rewardIndex++;
                if (remainingRewardCount == 0) {
                    StartRewardSequence();
                }
            }
            break;
        case 2:
            playerWork->stateFrame++;
            playerWork->rotationStartAngle += 40.0f;
            motionProgress = playerWork->stateFrame / (1.5f * REFRESH_RATE_F);
            if (motionProgress >= 1.0f) {
                motionProgress = 1.0f;
                playerWork->rewardIndex++;
            }
            playerWork->position.x = playerMotionOrigin.x + (motionProgress * (playerWork->targetPosition.x - playerMotionOrigin.x));
            playerWork->position.y = quadraticInterpolate(playerMotionOrigin.y, 1400.0f + playerMotionOrigin.y, 0.0f, motionProgress);
            playerWork->position.z = playerMotionOrigin.z + (motionProgress * (playerWork->targetPosition.z - playerMotionOrigin.x));
            playerWork->rotation.x = 180.0f * motionProgress;
            MTXRotRad(playerModel->mtx, 0x59, 0.017453292f * playerWork->rotationStartAngle);
            break;
        case 3:
            SetPlayerState(playerWork, 15, 8);
            Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
            coinReward = remainingRewardCount + 1;
            playerIndex = playerWork->playerIndex;
            GWPlayerCoinWinSet(playerIndex, coinReward);
            remainingRewardCount -= 1;
            return 1;
    }
    return 0;
}

s32 SetPlayerMotion(OMOBJ *object, s16 motionIndex, u32 motionAttributes)
{
    s16 currentMotionId;
    s16 transitionMotionId;

    currentMotionId = Hu3DData[object->mdlId[0]].motId;
    transitionMotionId = Hu3DData[object->mdlId[0]].motIdShift;
    if ((transitionMotionId != object->mtnId[motionIndex]) && (currentMotionId != object->mtnId[motionIndex])) {
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[motionIndex], 0.0f, 7.0f, motionAttributes);
    }
}

void RestorePlayerIdleMotions(void)
{
    s16 playerIndex;
    PlayerWork *playerWork;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerWork = (PlayerWork *)playerObjects[playerIndex]->data;
        if (GetPlayerState(playerWork, 15) == 8) {
            Hu3DMotionShiftSet(playerObjects[playerIndex]->mdlId[0], playerObjects[playerIndex]->mtnId[6], 0.0f, 7.0f, HU3D_MOTATTR_NONE);
        }
    }
}

void SetPlayersIdleMotions(void)
{
    PlayerWork *playerWork;
    s16 currentMotionId;
    s16 transitionMotionId;
    s16 playerIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerWork = (PlayerWork *)playerObjects[playerIndex]->data;
        if (GetPlayerState(playerWork, 15) == 8) {
            currentMotionId = Hu3DData[playerObjects[playerIndex]->mdlId[0]].motId;
            transitionMotionId = Hu3DData[playerObjects[playerIndex]->mdlId[0]].motIdShift;
            if ((transitionMotionId != playerObjects[playerIndex]->mtnId[0]) && (currentMotionId != playerObjects[playerIndex]->mtnId[0])) {
                Hu3DMotionShiftSet(playerObjects[playerIndex]->mdlId[0], playerObjects[playerIndex]->mtnId[0], 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
            }
        }
    }
}

s16 GetFirstActivePlayer(void)
{
    s16 playerSlot;

    for (playerSlot = 0; playerSlot < 4; playerSlot++) {
        if (playerOrder[playerSlot] != -1) {
            return playerOrder[playerSlot];
        }
    }
    return 0;
}

void StartPlayerCelebrations(void)
{
    OMOBJ *playerObject;
    s16 playerSlot;
    PlayerWork *playerWork;

    for (playerSlot = 0; playerSlot < 4; playerSlot++) {
        playerObject = playerObjects[playerOrder[playerSlot]];
        playerWork = (PlayerWork *)playerObject->data;
        SetPlayerState(playerWork, 15, 1);
        Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[2], 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
    }
}

void ChoosePlayerRewards(void)
{
    OMOBJ *playerObject;
    s16 unusedIndex0;
    s16 playerSlot;
    s32 aiDifficultyRank;
    s32 aiDifficultyRankAlternate;
    s32 maxTargetChoices;
    u32 unusedIndex0Alternate;
    u32 unusedIndex3;
    u32 unusedIndex3Alternate;
    PlayerWork *playerWork;

    for (playerSlot = 0; playerSlot < 4; playerSlot++) {
        if (playerOrder[playerSlot] != -1) {
            playerObject = playerObjects[playerOrder[playerSlot]];
            playerWork = (PlayerWork *)playerObject->data;
            if (playerWork->playerSlot == 0) {
                switch (GWPlayerCfg[playerWork->playerIndex].diff) {
                    case 0:
                        maxTargetChoices = 3;
                        break;
                    case 1:
                        maxTargetChoices = 2;
                        break;
                    case 2:
                        maxTargetChoices = 1;
                        break;
                    case 3:
                        maxTargetChoices = 0;
                        break;
                }
                playerWork->selectionThreshold = ((((rand8() << 8) | rand8()) % 256) / 255.0f);
                playerWork->remainingRewardChoices = (((rand8() << 8) | rand8()) % (s16)(maxTargetChoices + 1));
                playerWork->selectedRewardId = -1;
                playerWork->selectionFrameCount = playerWork->buttonRepeatActive = 0;
                playerWork->playerSlot = -1;
                playerWork->stateFrame = playerWork->stateDuration = 0;
                SetPlayerState(playerWork, 15, 2);
            }
            else if (playerWork->playerSlot == -1) {
                playerWork->playerSlot = remainingRewardCount;
            }
            else {
                playerWork->playerSlot--;
                playerWork->targetPosition = playerWork->position;
                playerWork->stateFrame = 0;
                playerWork->stateDuration = (s32)(0.5f * REFRESH_RATE_F);
                playerWork->movementFrameCount = 0;
                SetPlayerState(playerWork, 15, 1);
            }
            if (playerObject->mtnId[2] != Hu3DData[playerObject->mdlId[0]].motId) {
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[2], 0.0f, 7.0f, HU3D_MOTATTR_LOOP);
            }
        }
    }
}

u16 GetPlayerState(PlayerWork *playerWork, u16 stateMask)
{
    return playerWork->stateFlags & stateMask;
}

void SetPlayerState(PlayerWork *playerWork, u16 stateMask, u16 newState)
{
    playerWork->stateFlags &= ~stateMask;
    playerWork->stateFlags |= newState;
}

s32 GetPlayerSlot(s32 playerSlot)
{
    return playerOrder[playerSlot];
}

#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "string.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/frand.h"
#endif

#include "REL/m443Dll.h"

typedef struct M443DllOverlayWork {
    s32 isVisible;
    s32 blinkFrameOffset;
    s32 animationFrame;
    Vec2f iconPosition;
    s16 spriteIds[11];
} M443DllOverlayWork;

typedef struct M443DllPlayerWork {
    s32 characterId;
    s32 stateTimer;
    s32 currentMotionIndex;
    s32 playerDisplayIndex;
    u32 pressedButtons;
    s32 heldButtons;
    f32 triggerStrength;
    s32 modelHookEnabled;
    s32 previousModelHookState;
    s32 isComputer;
    char reservedPlayerWorkPadding[0xC];
    u16 animationState;
    s32 animationFrameCount;
    char reservedAnimationPadding[0x4];
    s16 stateFrameCount;
    f32 unusedWorkFloat;
    f32 currentYaw;
    f32 targetYaw;
    f32 yawBlend;
    f32 yawBlendStep;
    f32 secondaryModelYaw;
    f32 difficultyScale;
    s32 computerInputState;
    f32 targetEngineSpeed;
    u32 previousGear;
    f32 engineSpeedThreshold;
    f32 gearShiftDelay;
    u32 exhaustCycle;
    f32 distanceTravelled;
    f32 throttle;
    u32 gear;
    u32 gearShiftCooldown;
    f32 engineSpeed;
    char reservedEnginePadding[0x4];
    f32 targetEngineSpeed;
    f32 wheelRotationSpeed;
    f32 unusedEngineState;
    f32 engineResponse;
    f32 engineSpeedBlend;
    f32 unusedEngineValue;
    s32 engineSoundId;
    s32 unusedSoundId;
    s32 startSoundId;
    s32 engineLoopSoundId;
    s32 gearShiftSoundId;
    f32 startingLineZ;
    f32 collisionRadius;
    Vec position;
    Vec velocity;
    Vec secondaryModelPosition;
    Vec particlePosition;
    char reservedEffectPadding[0xC];
    M443DllCameraState *mainCamera;
    M443DllCameraState *resultCamera;
    union {
        M443DllOverlayWork speedOverlay;
        struct {
            s32 playerInputEnabled;
            s32 gearShiftEventPending;
        };
    };
} M443DllPlayerWork; /* size = 0x134 */

typedef struct M443DllMotionConfig {
    u16 motionIndex;
    f32 blendDuration;
    f32 motionStartTime;
    f32 motionEndTime;
    u32 motionFlags;
} M443DllMotionConfig;

void CreatePlayerRacer(OMOBJ *object);
void UpdatePlayerRacer(OMOBJ *object);
void UpdatePlayerInput(OMOBJ *object);
void StartPlayerRacer(OMOBJ *object);
void UpdatePlayerRacerState(OMOBJ *object);
void UpdatePlayerSteering(OMOBJ *object);
void UpdatePlayerCameraVisibility(OMOBJ *object);
void UpdatePlayerControls(OMOBJ *object);
void UpdatePlayerOrientation(OMOBJ *object);
void UpdatePlayerEngine(OMOBJ *object);
void SetPlayerMotion(OMOBJ *object, u32 motionIndex);
s32 IsPlayerMotionComplete(OMOBJ *object);
void CreateSpeedOverlay(OMOBJ *object);
void UpdateSpeedOverlay(OMOBJ *object);
f32 GetEngineAcceleration(u32 gear, f32 engineSpeed);
f32 ScaleEngineResponse(f32 engineResponse);
f32 GetEngineSpeedRatio(s32 gear, f32 engineSpeed);
f32 GetEngineSpeedForGear(u32 gear, f32 engineSpeedRatio);
f32 SquareEngineValue(f32 value);
void UpdateExhaustEffect(s32 modelId);
void EmitExhaustParticles(u32 playerIndex, Vec *position, f32 throttle, f32 engineSpeed);
void ClearExhaustParticles(u32 playerIndex);

OMOBJ *playerObjectsByIndex[4];
s32 playerDisplayOrderByIndex[6];

char *itemHookNames[0x8] = { "itemhook-r", "itemhook-r", "itemhook-r", "itemhook-r", "itemhook-r", "itemhook-r", "itemhook-r", "itemhook-r" };
u32 characterModelResources[8][2] = {
    { 0x4A0005, 0x4A0006 },
    { 0x4A0007, 0x4A0008 },
    { 0x4A0009, 0x4A000A },
    { 0x4A000B, 0x4A000C },
    { 0x4A000D, 0x4A000E },
    { 0x4A000F, 0x4A0010 },
    { 0x4A0011, 0x4A0012 },
    { 0x4A0013, 0x4A0014 },
};
u32 characterMotionResources[4] = { 0, 0x4A0020, 0x17, 0x72 };

M443DllMotionConfig playerMotionConfigs[4] = { { 0x0, 0.2f, 0.0f, -1.0f, 0x40000001 }, { 0x1, 0.2f, 0.0f, -1.0f, 0x40000001 }, { 0x2, 0.2f, 0.0f, -1.0f, 0x0 },
    { 0x3, 0.2f, 0.0f, -1.0f, 0x0 } };

s32 playerStartSounds[4] = {
    0x727,
    0x728,
    0x729,
    0x72A,
};
s32 playerEngineLoopSounds[4] = {
    0x72B,
    0x72C,
    0x72D,
    0x72E,
};
s32 playerEngineSounds[4] = {
    0x72F,
    0x730,
    0x731,
    0x732,
};
s32 playerGearShiftSounds[4] = {
    0x736,
    0x737,
    0x738,
    0x739,
};
Vec playerRankPositions[4] = { { 0.0f, 180.0f, 500.0f }, { -150.0f, 110.0f, 500.0f }, { 170.0f, 80.0f, 500.0f }, { 350.0f, 0.0f, 500.0f } };
Vec playerGridPositions[4] = { { -225.0f, 0.0f, 1000.0f }, { -75.0f, 0.0f, 1000.0f }, { 75.0f, 0.0f, 1000.0f }, { 225.0f, 0.0f, 1000.0f } };
Vec playerRankModelPositions[4] = { { 250.0f, 0.0f, -1000.0f }, { -350.0f, 0.0f, -1700.0f }, { 450.0f, 0.0f, -2200.0f }, { 0.0f, 0.0f, -3000.0f } };

void InitializePlayerRacers(HUPROCESS *process)
{
    u32 playerGroups[4];
    u32 playerOrder[4];
    OMOBJ *playerObject;
    u32 groupSwapTemp;
    s16 playerIndex;
    s16 sortIndex;

    for (sortIndex = 0; sortIndex < 4; sortIndex++) {
        playerGroups[sortIndex] = GWPlayerCfg[sortIndex].group;
        playerOrder[sortIndex] = sortIndex;
    }

    for (sortIndex = 0; sortIndex < 3; sortIndex++) {
        for (playerIndex = sortIndex + 1; playerIndex < 4; playerIndex++) {
            if (playerGroups[sortIndex] > playerGroups[playerIndex]) {
                groupSwapTemp = playerGroups[sortIndex];
                playerGroups[sortIndex] = playerGroups[playerIndex];
                playerGroups[playerIndex] = groupSwapTemp;
                groupSwapTemp = playerOrder[sortIndex];
                playerOrder[sortIndex] = playerOrder[playerIndex];
                playerOrder[playerIndex] = groupSwapTemp;
            }
        }
    }

    for (sortIndex = 0; sortIndex < 4; sortIndex++) {
        playerDisplayOrderByIndex[playerOrder[sortIndex]] = (s16)sortIndex;
    }
    omMakeGroupEx(process, 0, 4);
    omGetGroupMemberListEx(process, 0);

    for (sortIndex = 0; sortIndex < 4; sortIndex++) {
        playerObject = playerObjectsByIndex[sortIndex] = omAddObjEx(process, 0x28, 5, 4, 0, CreatePlayerRacer);
        playerObject->work[0] = sortIndex;
    }
}

void EmptyPlayerCallback(void)
{
    CharModelKill(-1);
}

void CreatePlayerRacer(OMOBJ *object)
{
    f32 difficultyScales[4] = { 0.25f, 0.5f, 0.75f, 1.0f };
    s32 initializationCounter;
    f32 initializationScale;
    s32 characterId;
    s32 particleModelId;
    s32 motionIndex;
    u32 playerIndex;
    M443DllPlayerWork *playerWork;
    HU3DPARTICLEDATA *particleData;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, 0x134, HU_MEMNUM_OVL);
    playerWork = object->data;
    memset(playerWork, 0, 0x134);
    object->stat |= 0x100;
    playerIndex = object->work[0];
    characterId = GWPlayerCfg[playerIndex].character;
    playerWork->characterId = characterId;
    initializationCounter = playerWork->stateTimer = 0;
    playerWork->playerDisplayIndex = playerDisplayOrderByIndex[object->work[0]];
    playerWork->difficultyScale = difficultyScales[GWPlayerCfg[playerIndex].diff & 3];
    playerWork->isComputer = GWPlayerCfg[object->work[0]].iscom;
    object->mdlId[0] = CharModelCreate(characterId, 8);
    Hu3DModelLayerSet(object->mdlId[0], 4);
    object->mdlId[1] = Hu3DModelCreateFile(characterModelResources[characterId][0]);
    Hu3DModelLayerSet(object->mdlId[1], 4);
    Hu3DMotionSpeedSet(object->mdlId[1], 0.0f);
    object->mdlId[2] = Hu3DModelCreateFile(characterModelResources[characterId][1]);
    Hu3DModelLayerSet(object->mdlId[2], 4);
    Hu3DMotionSpeedSet(object->mdlId[2], 0.0f);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);

    for (motionIndex = 0; motionIndex < 4; motionIndex++) {
        if (characterMotionResources[motionIndex] < 0x10000) {
            object->mtnId[motionIndex] = CharMotionCreate(characterId, characterMotionResources[motionIndex]);
        }
        else {
            object->mtnId[motionIndex]
                = Hu3DJointMotion(object->mdlId[0], HuDataSelHeapReadNum(characterId + characterMotionResources[motionIndex], 0x10000000, HEAP_MODEL));
        }
    }
    CharMotionDataClose(characterId);
    particleModelId = Hu3DParticleCreate(HuSprAnimReadFile(0x4A001D), 0xC8);
    object->mdlId[3] = particleModelId;
    Hu3DModelLayerSet(particleModelId, 4);
    particleData = ((HU3DPARTICLE *)Hu3DData[particleModelId].hookData)->data;

    for (motionIndex = 0; motionIndex < 0xC8; motionIndex++, particleData++) {
        particleData->scale = 0.0f;
        particleData->color.a = 0;
        particleData->time = 0;
    }

    object->trans.x = playerWork->position.x = playerWork->secondaryModelPosition.x = (100.0f * (4.0f * object->work[0])) - 600.0f;
    object->trans.y = playerWork->position.y = playerWork->secondaryModelPosition.y = 0.0f;
    object->trans.z = playerWork->position.z = playerWork->secondaryModelPosition.z = 0.0f;

    playerWork->velocity.x = playerWork->velocity.y = playerWork->velocity.z = 0.0f;
    playerWork->particlePosition.x = playerWork->particlePosition.y = playerWork->particlePosition.z = 0.0f;
    playerWork->currentYaw = playerWork->targetYaw = 0.0f;
    playerWork->yawBlend = 0.0f;
    playerWork->yawBlendStep = 0.02f;
    playerWork->secondaryModelYaw = 180.0f;
    playerWork->collisionRadius = 50.0f;
    playerWork->startingLineZ = 350.0f;
    playerWork->currentMotionIndex = -1;
    playerWork->modelHookEnabled = 1;
    playerWork->previousModelHookState = 0;
    playerWork->engineResponse = 1.0f;
    playerWork->unusedEngineState = 0.0f;
    playerWork->engineSpeedBlend = 0.0f;
    playerWork->unusedEngineValue = 0.0f;
    playerWork->engineSpeed = 1000.0f;
    playerWork->targetEngineSpeed = GetEngineSpeedRatio(playerWork->gear, playerWork->engineSpeed);
    playerWork->engineSoundId = playerWork->unusedSoundId = -1;
    playerWork->startSoundId = playerWork->engineLoopSoundId = playerWork->gearShiftSoundId = -1;
    playerWork->engineLoopSoundId = HuAudFXPlay(playerEngineLoopSounds[object->work[0]]);
    playerWork->previousGear = -1;
    playerWork->computerInputState = 0;
    playerWork->mainCamera = GetMainCamera(object->work[0] + 1);
    playerWork->resultCamera = GetResultCamera(object->work[0] + 1);
    playerWork->mainCamera->isEnabled = 0;
    CreateSpeedOverlay(object);
    playerWork->animationState = 0;
    object->objFunc = UpdatePlayerRacer;
}

void UpdatePlayerRacer(OMOBJ *object)
{
    Vec animationPosition;
    f32 controllerTriggerValue;
    f32 animationEase;
    f32 animationProgress;
    M443DllPlayerWork *playerWork;

    playerWork = object->data;
    playerWork->pressedButtons = 0;
    playerWork->heldButtons = 0;
    playerWork->triggerStrength = 0.0f;
    if (playerWork->playerInputEnabled != 0) {
        if (playerWork->isComputer != 0) {
            UpdatePlayerControls(object);
        }
        else {
            playerWork->heldButtons = HuPadBtn[GWPlayerCfg[object->work[0]].pad_idx];
            playerWork->pressedButtons = HuPadBtnDown[GWPlayerCfg[object->work[0]].pad_idx];
            controllerTriggerValue = HuPadTrigR[GWPlayerCfg[object->work[0]].pad_idx];
            if (controllerTriggerValue > 140.0f) {
                controllerTriggerValue = 140.0f;
            }
            playerWork->triggerStrength = (1.0f / 140.0f) * controllerTriggerValue;
        }
    }
    switch (playerWork->animationState) {
        case 0:
            playerWork->mainCamera->targetPosition = playerWork->secondaryModelPosition;
            playerWork->mainCamera->targetOffset.y = 150.0f;
            playerWork->mainCamera->targetPosition.y += playerWork->mainCamera->targetOffset.y;
            playerWork->mainCamera->rotation.x = -30.0f;
            playerWork->mainCamera->rotation.y = 90.0f;
            playerWork->mainCamera->zoomDistance = 500.0f;
            playerWork->mainCamera->cameraPanX = -20.0f - frandmod(10);
            playerWork->mainCamera->cameraPanY = 160.0f + frandmod(0x28);
            playerWork->mainCamera->savedViewportCenterX = playerWork->mainCamera->viewportCenterX;
            playerWork->mainCamera->savedViewportCenterY = playerWork->mainCamera->viewportCenterY;
            playerWork->mainCamera->savedViewportScale = playerWork->mainCamera->viewportScale;
            if ((playerWork->stateFrameCount < REFRESH_RATE_F * 2) && ((playerWork->stateFrameCount % 60) > 10)) {
                playerWork->triggerStrength = 1.0f;
            }
            if (getCameraTransitionState() == 2) {
                playerWork->animationState++;
                playerWork->stateFrameCount = 0;
                playerWork->startSoundId = HuAudFXPlay(playerStartSounds[object->work[0]]);
            }
            break;
        case 1:
            playerWork->mainCamera->isEnabled = 1;
            animationProgress = REFRESH_FREQ / 2 * playerWork->stateFrameCount;
            if (animationProgress > 1.0f) {
                animationProgress = 1.0f;
            }
            animationEase = 2.0f * (animationProgress - 0.5f);
            if (animationEase < 0.0f) {
                animationEase = 0.0f;
            }
            animationEase = animationEase * animationEase;
            animationPosition.x = 320.0 + 160.0 * (animationProgress * cosd(450.0f * animationProgress + 90.0f * object->work[0]));
            animationPosition.y = 240.0 + 120.0 * (animationProgress * sind(450.0f * animationProgress + 90.0f * object->work[0]));
            playerWork->mainCamera->viewportCenterX = animationPosition.x + (animationEase * (playerWork->mainCamera->savedViewportCenterX - animationPosition.x));
            playerWork->mainCamera->viewportCenterY = animationPosition.y + (animationEase * (playerWork->mainCamera->savedViewportCenterY - animationPosition.y));
            playerWork->mainCamera->viewportScale = 0.01f + ((playerWork->mainCamera->savedViewportScale - 0.01f) * animationProgress);
            playerWork->mainCamera->rotation.y = 90.0f + ((playerWork->mainCamera->cameraPanY - 90.0f) * animationProgress);
            playerWork->mainCamera->zoomDistance = 500.0f + (-200.0f * animationEase);
            if (animationProgress >= 1.0f) {
                playerWork->playerInputEnabled = 1;
                if (playerWork->stateFrameCount >= REFRESH_RATE_F * 2.5f) {
                    SetCameraTransitionState(4);
                    playerWork->animationState++;
                    playerWork->stateFrameCount = 0;
                }
            }
            break;
        case 2:
            animationProgress = REFRESH_FREQ / 2 * playerWork->stateFrameCount;
            if (animationProgress > 0.2f) {
                animationProgress = 0.2f;
            }
            playerWork->mainCamera->rotation.x = interpolateAngle(playerWork->mainCamera->rotation.x, -12.0f, animationProgress);
            playerWork->mainCamera->rotation.y = interpolateAngle(playerWork->mainCamera->rotation.y, 0.0f, animationProgress);
            playerWork->mainCamera->zoomDistance += ((1000.0f - playerWork->mainCamera->zoomDistance) * (0.5f * animationProgress));
            break;
    }
    UpdatePlayerEngine(object);
    VECAdd(&playerWork->position, &playerWork->velocity, &playerWork->position);
    UpdatePlayerOrientation(object);
    UpdatePlayerSteering(object);

    if (getCameraTransitionStage() == 2) {
        playerWork->mainCamera->targetPosition = playerWork->secondaryModelPosition;
        playerWork->mainCamera->targetPosition.y += 150.0f;
        playerWork->mainCamera->targetOffset.y = 150.0f;
        playerWork->mainCamera->rotation.x = -12.0f;
        playerWork->mainCamera->rotation.y = 0.0f;
        playerWork->mainCamera->zoomDistance = 1000.0f;
        playerWork->animationState = 0;
        object->objFunc = &UpdatePlayerInput;
    }
}

void UpdatePlayerInput(OMOBJ *object)
{
    f32 controllerTriggerValue;
    s32 motionStarted;
    M443DllPlayerWork *playerWork;
    void *unusedModelPointer;

    playerWork = object->data;
    motionStarted = 0;
    playerWork->pressedButtons = 0;
    playerWork->heldButtons = 0;
    playerWork->triggerStrength = 0.0f;
    if (playerWork->isComputer != 0) {
        UpdatePlayerControls(object);
    }
    else {
        playerWork->heldButtons = HuPadBtn[GWPlayerCfg[object->work[0]].pad_idx];
        playerWork->pressedButtons = HuPadBtnDown[GWPlayerCfg[object->work[0]].pad_idx];
        controllerTriggerValue = HuPadTrigR[GWPlayerCfg[object->work[0]].pad_idx];
        if (controllerTriggerValue > 140.0f) {
            controllerTriggerValue = 140.0f;
        }
        playerWork->triggerStrength = (1.0f / 140.0f) * controllerTriggerValue;
    }
    switch (playerWork->animationState) {
        case 0:
            SetPlayerMotion(object, 1);
            motionStarted = 1;
    }
    UpdatePlayerEngine(object);
    VECAdd(&playerWork->position, &playerWork->velocity, &playerWork->position);
    UpdatePlayerOrientation(object);
    UpdatePlayerSteering(object);
    playerWork->mainCamera->targetPosition = playerWork->secondaryModelPosition;
    VECAdd(&playerWork->mainCamera->targetPosition, &playerWork->mainCamera->targetOffset, &playerWork->mainCamera->targetPosition);
    if ((getResultStage() == 3) && (motionStarted != 0)) {
        SetPlayerFinishEvent(object->work[0]);
        playerWork->playerInputEnabled = 0;
        playerWork->animationState = 0;
        object->objFunc = StartPlayerRacer;
    }
}

void StartPlayerRacer(OMOBJ *object)
{
    M443DllPlayerWork *playerWork;
    void *unusedModelPointer;

    playerWork = object->data;
    playerWork->pressedButtons = 0;
    playerWork->heldButtons = 0;
    playerWork->triggerStrength = 0.0f;
    UpdatePlayerEngine(object);
    VECAdd(&playerWork->position, &playerWork->velocity, &playerWork->position);
    UpdatePlayerOrientation(object);
    UpdatePlayerSteering(object);
    playerWork->mainCamera->targetPosition = playerWork->secondaryModelPosition;
    VECAdd(&playerWork->mainCamera->targetPosition, &playerWork->mainCamera->targetOffset, &playerWork->mainCamera->targetPosition);
    if (getResultStage() == 5) {
        playerWork->modelHookEnabled = 0;
        playerWork->animationState = 0;
        object->objFunc = UpdatePlayerRacerState;
    }
}

void UpdatePlayerRacerState(OMOBJ *object)
{
    M443DllPlayerWork *playerWork;

    playerWork = object->data;
    VECAdd(&playerWork->position, &playerWork->velocity, &playerWork->position);
    UpdatePlayerOrientation(object);
    UpdatePlayerSteering(object);
    switch (playerWork->animationState) {
        case 0:
            Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
            ClearExhaustParticles(object->work[0]);
            Hu3DModelShadowSet(object->mdlId[0]);
            playerWork->currentMotionIndex = -1;
            SetPlayerMotion(object, 0);
            if (getPlayerRank(object->work[0]) >= 0) {
                playerWork->position = playerRankPositions[getPlayerRank(object->work[0])];
                playerWork->secondaryModelPosition = playerRankModelPositions[getPlayerRank(object->work[0])];
                playerWork->secondaryModelYaw = 30.0f;
            }
            else {
                Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
                playerWork->position = playerGridPositions[object->work[0]];
            }
            playerWork->animationState++;
            break;
        case 1:
            playerWork->targetYaw = 0.0f;
            playerWork->yawBlend = 0.15f;
            playerWork->yawBlendStep = 0.0f;
            playerWork->velocity.x = playerWork->velocity.y = playerWork->velocity.z = 0.0f;
            SetPlayerFinishEvent(object->work[0]);
            playerWork->animationState++;
            break;
        case 2:
            if (getRewardSequenceState() == 1) {
                if (getPlayerRank(object->work[0]) == 0) {
                    SetPlayerMotion(object, 2);
                }
                else {
                    SetPlayerMotion(object, 3);
                }
                playerWork->animationState++;
            }
            break;
        case 3:
            if (IsPlayerMotionComplete(object) != 0) {
                SetPlayerEvent(object->work[0]);
                playerWork->animationState++;
            }
            break;
    }
}

void UpdatePlayerSteering(OMOBJ *object)
{
    M443DllPlayerWork *playerWork;

    playerWork = object->data;
    playerWork->stateFrameCount++;
    playerWork->animationFrameCount++;
    object->trans.x = playerWork->position.x;
    object->trans.y = playerWork->position.y;
    object->trans.z = playerWork->position.z;
    playerWork->yawBlend += playerWork->yawBlendStep;
    if (playerWork->yawBlend > 0.45f) {
        playerWork->yawBlend = 0.45f;
    }
    playerWork->currentYaw = interpolateAngle(playerWork->currentYaw, playerWork->targetYaw, playerWork->yawBlend);
    object->rot.y = playerWork->currentYaw;
    if (playerWork->modelHookEnabled != 0) {
        if (playerWork->previousModelHookState == 0) {
            Hu3DModelHookSet(object->mdlId[1], "hook", object->mdlId[0]);
        }

        playerWork->secondaryModelPosition.z = playerWork->position.z = -(playerWork->distanceTravelled - playerWork->startingLineZ);
        SetPlayerMotion(object, 1);
    }
    else {
        if (playerWork->previousModelHookState != 0) {
            Hu3DModelHookReset(object->mdlId[1]);
        }
        Hu3DModelPosSetV(object->mdlId[0], &playerWork->position);
        Hu3DModelRotSet(object->mdlId[0], 0.0f, playerWork->currentYaw, 0.0f);
    }
    playerWork->previousModelHookState = playerWork->modelHookEnabled;
    Hu3DModelPosSetV(object->mdlId[1], &playerWork->secondaryModelPosition);
    Hu3DModelRotSet(object->mdlId[1], 0.0f, playerWork->secondaryModelYaw, 0.0f);
    Hu3DModelPosSetV(object->mdlId[2], &playerWork->secondaryModelPosition);
    Hu3DModelRotSet(object->mdlId[2], 0.0f, playerWork->secondaryModelYaw, 0.0f);
}

void UpdatePlayerCameraVisibility(OMOBJ *object)
{
    M443DllPlayerWork *otherPlayerWork;
    M443DllPlayerWork *playerWork = object->data;
    s32 playerDistanceWeights[4][4] = {
        { 0, 1, 2, 3 },
        { 1, 0, 1, 2 },
        { 2, 1, 0, 1 },
        { 3, 2, 1, 0 },
    };
    f32 distanceThresholds[4] = { 0.0f, -700.0f, 600.0f, 1200.0f };
    s32 unusedCollisionStateA;
    s32 unusedCollisionStateB;
    s32 unusedCollisionStateC;
    s32 cameraGroupMasks[4] = { 2, 4, 8, 16 };
    f32 relativePlayerDistance;
    s32 nearbyCameraMask;
    u32 otherPlayerIndex;

    nearbyCameraMask = cameraGroupMasks[object->work[0]] | 1;

    for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
        if (otherPlayerIndex != object->work[0]) {
            otherPlayerWork = playerObjectsByIndex[otherPlayerIndex]->data;
            relativePlayerDistance = otherPlayerWork->distanceTravelled;
            if (otherPlayerWork->exhaustCycle != playerWork->exhaustCycle) {
                relativePlayerDistance += REFRESH_FREQ * 100.0f * otherPlayerWork->throttle;
            }
            relativePlayerDistance = playerWork->distanceTravelled - relativePlayerDistance;
            if (relativePlayerDistance > distanceThresholds[playerDistanceWeights[object->work[0]][otherPlayerIndex]]) {
                nearbyCameraMask |= cameraGroupMasks[otherPlayerIndex];
            }
        }
    }
    SetModelCameraMask(object->mdlId[0], nearbyCameraMask);
    SetModelCameraMask(object->mdlId[1], nearbyCameraMask);
    SetModelCameraMask(object->mdlId[2], nearbyCameraMask);
    SetModelCameraMask(object->mdlId[3], nearbyCameraMask);
}

void UpdatePlayerControls(OMOBJ *object)
{
    f32 aiInputAdjustment;
    M443DllPlayerWork *playerWork;

    playerWork = object->data;
    switch (playerWork->computerInputState) {
        case 0:
            aiInputAdjustment = 1.25f - playerWork->difficultyScale;
            aiInputAdjustment *= 0.6f + (0.00040000002f * frandmod(0x3E8));
            if (frandmod(0x3E8) < 0x1F4) {
                aiInputAdjustment *= -1.0f;
            }
            playerWork->targetEngineSpeed = (3000.0f + (2000.0f * aiInputAdjustment));
            playerWork->computerInputState++;
            /* fallthrough */
        case 1:
            if (playerWork->engineSpeed < playerWork->targetEngineSpeed) {
                playerWork->triggerStrength = 1.0f;
            }
            if (playerWork->engineSpeedBlend >= 1.0f) {
                playerWork->computerInputState = (s32)(playerWork->computerInputState + 1);
            }
            break;
        case 2:
            playerWork->triggerStrength = 1.0f;
            if (playerWork->gear != playerWork->previousGear) {
                playerWork->engineSpeedThreshold = playerWork->gearShiftDelay = 0.0f;
                if (frandmod(0x3E8) < 0x1F4) {
                    aiInputAdjustment = 1.2f - playerWork->difficultyScale;
                    aiInputAdjustment *= (0.5f + 0.0005f * frandmod(0x3E8));
                    playerWork->engineSpeedThreshold = 8000.0f - 1500.0f * aiInputAdjustment;
                }
                else {
                    aiInputAdjustment = 1.2f - playerWork->difficultyScale;
                    aiInputAdjustment *= (0.5f + 0.0005f * frandmod(0x3E8));
                    playerWork->gearShiftDelay = REFRESH_FREQ + aiInputAdjustment;
                }
            }
            playerWork->previousGear = playerWork->gear;
            if (playerWork->engineSpeedThreshold > 0.0f) {
                if (playerWork->engineSpeed > playerWork->engineSpeedThreshold) {
                    playerWork->pressedButtons = (s32)(playerWork->pressedButtons | 0x100);
                    playerWork->engineSpeedThreshold = 0.0f;
                    break;
                }
            }
            else if (playerWork->gearShiftDelay > 0.0f) {
                if (playerWork->engineSpeed >= 8000.0f) {
                    playerWork->gearShiftDelay = (playerWork->gearShiftDelay - REFRESH_FREQ);
                    if (playerWork->gearShiftDelay <= 0.0f) {
                        playerWork->pressedButtons = (s32)(playerWork->pressedButtons | 0x100);
                        playerWork->gearShiftDelay = 0.0f;
                    }
                }
            }
            break;
    }
}

void UpdatePlayerOrientation(OMOBJ *object)
{
    Vec separationVector;
    f32 separationDistance;
    s32 collisionPass;
    s32 playerIndex;
    s32 otherPlayerIndex;
    M443DllPlayerWork *otherPlayerWork;
    M443DllPlayerWork *playerWork;

    for (collisionPass = 0; collisionPass < 2; collisionPass++) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerWork = playerObjectsByIndex[playerIndex]->data;

            for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                if (playerIndex != otherPlayerIndex) {
                    otherPlayerWork = playerObjectsByIndex[otherPlayerIndex]->data;
                    VECSubtract(&playerWork->position, &otherPlayerWork->position, &separationVector);
                    separationVector.y = 0.0f;
                    separationDistance = VECMag(&separationVector);
                    if (separationDistance < (playerWork->collisionRadius + otherPlayerWork->collisionRadius)) {
                        if (separationDistance > 0.0f) {
                            separationDistance = 0.5f * ((playerWork->collisionRadius + otherPlayerWork->collisionRadius) - separationDistance);
                        }
                        else {
                            separationDistance = 0.5f * (playerWork->collisionRadius + otherPlayerWork->collisionRadius);
                            separationVector.x = (frandmod(0x3E8) - 0x1F4);
                            separationVector.z = (frandmod(0x3E8) - 0x1F4);
                        }
                        VECNormalize(&separationVector, &separationVector);
                        VECScale(&separationVector, &separationVector, separationDistance);
                        VECAdd(&playerWork->position, &separationVector, &playerWork->position);
                        VECSubtract(&otherPlayerWork->position, &separationVector, &otherPlayerWork->position);
                    }
                }
            }
            if (playerWork->position.y < 0.0f) {
                playerWork->position.y = 0.0f;
            }
        }
    }
}

void UpdatePlayerEngine(OMOBJ *object)
{
    M443DllPlayerWork *playerWork = object->data;
    HSFOBJECT *modelObject;
    f32 sp28[8] = { 0.0f, 40.0f, 0.0f, 0.0f, -16.0f, 40.0f, -16.0f, 0.0f };
    f32 engineAdjustment;
    f32 engineResponseScale;
    u32 previousGear;

    previousGear = playerWork->gear;
    if (playerWork->engineSpeed > 3300.0f) {
        if (getResultStage() == 2) {
            playerWork->gearShiftEventPending = 0;
            if (playerWork->gearShiftCooldown == 0) {
                if ((playerWork->pressedButtons & 0x100) != 0) {
                    if (playerWork->gear < 3) {
                        playerWork->gear++;
                        playerWork->gearShiftCooldown = REFRESH_RATE / 2;
                    }
                }
                else {
                    if (playerWork->engineSpeed >= 8000.0f) {
                        playerWork->gearShiftEventPending = 1;
                    }
                }
            }
            else {
                playerWork->gearShiftCooldown--;
            }
        }
    }
    else if ((playerWork->engineSpeed < 1500.0f) && (playerWork->gear != 0)) {
        playerWork->gear--;
        playerWork->gearShiftCooldown = REFRESH_RATE / 2;
    }
    if (playerWork->gear != previousGear) {
        playerWork->engineSpeed = GetEngineSpeedForGear(playerWork->gear, playerWork->targetEngineSpeed);
        omVibrate(object->work[0], 0xC, 4, 2);
    }
    engineAdjustment = (1.5f * playerWork->triggerStrength) - 0.5f;
    if (engineAdjustment < -0.4f) {
        engineAdjustment = -1.0f;
    }
    engineResponseScale = 0.75f;
    if ((engineAdjustment > -0.5f) && (playerWork->engineSpeedBlend < 1.0f)) {
        engineResponseScale += 0.25f * (1.0f - playerWork->engineSpeedBlend);
    }
    engineAdjustment = engineAdjustment * GetEngineAcceleration(playerWork->gear, playerWork->engineSpeed);
    engineAdjustment -= playerWork->engineSpeedBlend * SquareEngineValue(GetEngineSpeedRatio(playerWork->gear, playerWork->engineSpeed));
    engineAdjustment = engineResponseScale * ScaleEngineResponse(engineAdjustment);
    playerWork->targetEngineSpeed += REFRESH_FREQ * engineAdjustment;
    playerWork->throttle += playerWork->engineSpeedBlend * (playerWork->targetEngineSpeed - playerWork->throttle);
    engineAdjustment = playerWork->targetEngineSpeed + playerWork->engineSpeedBlend * (playerWork->throttle - playerWork->targetEngineSpeed);
    playerWork->engineSpeed = GetEngineSpeedForGear(playerWork->gear, engineAdjustment);
    if (getResultStage() <= 2) {
        if (getResultStage() == 2) {
            if (playerWork->triggerStrength <= 0.0f) {
                playerWork->targetEngineSpeed *= 0.98f;
            }
        }
        if ((playerWork->engineSpeed > 8000.0f) && ((playerWork->stateFrameCount & 0xF) == 0)) {
            omVibrate(object->work[0], 0xC, 6, 6);
        }
    }
    if (playerWork->engineSpeed > 9500.0f) {
        playerWork->engineSpeed = 9000.0f;
        playerWork->targetEngineSpeed = GetEngineSpeedRatio(playerWork->gear, playerWork->engineSpeed);
    }
    else if (playerWork->engineSpeed < 1000.0f) {
        playerWork->engineSpeed = 1000.0f;
        playerWork->targetEngineSpeed = GetEngineSpeedRatio(playerWork->gear, playerWork->engineSpeed);
    }
    playerWork->wheelRotationSpeed = (0.31206876f * playerWork->targetEngineSpeed);
    if (playerWork->engineSpeedBlend < 1.0f) {
        engineAdjustment = playerWork->targetEngineSpeed - playerWork->throttle;
        if (engineAdjustment > 2.0f) {
            engineAdjustment = 1.0f - (0.02f * engineAdjustment);
            if (engineAdjustment > 1.0f) {
                engineAdjustment = 1.0f;
            }
            if (engineAdjustment < 0.0f) {
                engineAdjustment = 0.0f;
            }
            engineAdjustment = (1.0f - engineAdjustment * engineAdjustment);
            playerWork->engineSpeedBlend += (REFRESH_FREQ * 0.02f) * (1.0f - 1.5f * engineAdjustment);
            if (playerWork->engineSpeedBlend > 0.9f) {
                playerWork->engineSpeedBlend = 1.0f;
            }
            if (playerWork->engineSpeedBlend < 0.0001f) {
                playerWork->engineSpeedBlend = 0.0001f;
            }
        }
        else {
            playerWork->engineSpeedBlend = 1.0f;
        }
    }
    playerWork->distanceTravelled += REFRESH_FREQ * 100.0f * playerWork->throttle;
    if (getResultStage() < 2) {
        playerWork->throttle = 0.0f;
        playerWork->distanceTravelled = 0.0f;
        playerWork->engineSpeedBlend = 0.0f;
    }
    if (playerWork->distanceTravelled >= 40000.0f) {
        if (playerWork->throttle >= 30.0f) {
            Hu3DMotionSpeedSet(object->mdlId[2], 1.0f);
            Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
            if (getResultStage() == 2) {
                HuAudFXPlay(0x733);
            }
        }
    }
    UpdateSpeedOverlay(object);
    playerWork->exhaustCycle = 1 - playerWork->exhaustCycle;
    engineAdjustment = 22.5f * (playerWork->stateFrameCount & 0xF);
    modelObject = Hu3DModelObjPtrGet(object->mdlId[2], "ballon");
    modelObject->mesh.base.rot.z = engineAdjustment;
    modelObject = Hu3DModelObjPtrGet(object->mdlId[1], "r_tire");
    if ((playerWork->throttle <= 0.0f) && (playerWork->wheelRotationSpeed < 1.5f)) {
        playerWork->wheelRotationSpeed = 0.0f;
    }
    modelObject->mesh.base.rot.x += 360.0f * REFRESH_FREQ * playerWork->wheelRotationSpeed;
    modelObject->mesh.base.rot.x = fmod(modelObject->mesh.base.rot.x, 360.0);
    modelObject = Hu3DModelObjPtrGet(object->mdlId[1], "f_tire");
    modelObject->mesh.base.rot.x += 360.0f * REFRESH_FREQ * (0.003978877f * (100.0f * playerWork->throttle));
    modelObject->mesh.base.rot.x = fmod(modelObject->mesh.base.rot.x, 360.0);
    engineAdjustment = 0.110999994f * playerWork->wheelRotationSpeed;
    if (engineAdjustment > 1.0f) {
        engineAdjustment = 1.0f;
    }
    Hu3DMotionTimeSet(object->mdlId[1], 30.0f * engineAdjustment);
    SetPlayerScore(object->work[0], playerWork->distanceTravelled);
    UpdatePlayerCameraVisibility(object);
    if (playerWork->engineSpeedBlend < 1.0f) {
        engineAdjustment = (320.4422f * playerWork->wheelRotationSpeed) - (100.0f * playerWork->throttle);
        if (engineAdjustment > 0.0f) {
            engineAdjustment *= 0.0002f;
            if (engineAdjustment > 1.0f) {
                engineAdjustment = 1.0f;
            }
            EmitExhaustParticles(object->work[0], &playerWork->secondaryModelPosition, engineAdjustment, 100.0f * playerWork->throttle);
        }
    }
    UpdateExhaustEffect(object->mdlId[3]);
    if (playerWork->engineLoopSoundId >= 0) {
        HuAudFXPitchSet(playerWork->engineLoopSoundId, (8191.0f * (0.0001f * playerWork->engineSpeed)));
        if (getResultStage() >= 5) {
            HuAudFXFadeOut(playerWork->engineLoopSoundId, REFRESH_RATE);
            playerWork->engineLoopSoundId = -1;
        }
    }
    if (getResultStage() < 2) {
        if (playerWork->engineSoundId < 0) {
            playerWork->engineSoundId = HuAudFXPlay(playerEngineSounds[object->work[0]]);
        }
        if (playerWork->engineSoundId >= 0) {
            engineAdjustment = GetEngineSpeedRatio(playerWork->gear, 10000.0f);
            engineAdjustment = (1.0f / (0.31206876f * engineAdjustment));
            HuAudFXPitchSet(playerWork->engineSoundId, (8191.0f * (playerWork->wheelRotationSpeed * engineAdjustment)));
            engineAdjustment = 127.0f;
            if (playerWork->wheelRotationSpeed < 0.1f) {
                engineAdjustment = 0.0f;
            }
            HuAudFXVolSet(playerWork->engineSoundId, engineAdjustment);
        }
    }
    else if (getResultStage() <= 2) {
        if (playerWork->engineSoundId < 0) {
            playerWork->engineSoundId = HuAudFXPlay(playerEngineSounds[object->work[0]]);
        }
        if (playerWork->engineSoundId >= 0) {
            HuAudFXPitchSet(playerWork->engineSoundId, (8191.0f * (0.0001f * playerWork->engineSpeed) * (1.0f - playerWork->engineSpeedBlend)));
            engineAdjustment = 127.0f;
            if (playerWork->engineSpeedBlend >= 1.0f) {
                engineAdjustment = 0.0f;
            }
            HuAudFXVolSet(playerWork->engineSoundId, engineAdjustment);
        }
    }
    else if (playerWork->engineSoundId >= 0) {
        HuAudFXStop(playerWork->engineSoundId);
        playerWork->engineSoundId = -1;
    }
    if ((getResultStage() >= 1) && (playerWork->startSoundId >= 0)) {
        HuAudFXFadeOut(playerWork->startSoundId, REFRESH_RATE / 2);
        playerWork->startSoundId = -1;
    }
    if ((getResultStage() == 2) && (playerWork->gearShiftSoundId < 0)) {
        playerWork->gearShiftSoundId = HuAudFXPlay(playerGearShiftSounds[object->work[0]]);
    }
    if (playerWork->gearShiftSoundId >= 0) {
        engineAdjustment = 0.012048192f * playerWork->throttle;
        HuAudFXPitchSet(playerWork->gearShiftSoundId, ((2.0f * (8191.0f * engineAdjustment)) - 8191.0f));
        HuAudFXVolSet(playerWork->gearShiftSoundId, (127.0f * engineAdjustment));
        if (getResultStage() >= 5U) {
            HuAudFXFadeOut(playerWork->gearShiftSoundId, REFRESH_RATE);
            playerWork->gearShiftSoundId = -1;
        }
    }
}

void SetPlayerMotion(OMOBJ *object, u32 motionIndex)
{
    f32 motionBlendDuration;
    M443DllPlayerWork *playerWork;

    playerWork = object->data;
    if ((playerWork->currentMotionIndex != motionIndex) && (motionIndex < 4)) {
        motionBlendDuration = REFRESH_RATE_F * playerMotionConfigs[motionIndex].blendDuration;
        if (playerWork->currentMotionIndex < 0) {
            motionBlendDuration = 0.0f;
        }
        playerWork->currentMotionIndex = motionIndex;
        CharMotionShiftSet(
            playerWork->characterId, object->mtnId[playerMotionConfigs[motionIndex].motionIndex], REFRESH_RATE_F * playerMotionConfigs[motionIndex].motionStartTime, motionBlendDuration, playerMotionConfigs[motionIndex].motionFlags);
        if (playerMotionConfigs[motionIndex].motionEndTime >= 0.0f) {
            Hu3DMotionShiftStartEndSet(object->mdlId[0], REFRESH_RATE_F * playerMotionConfigs[motionIndex].motionStartTime, REFRESH_RATE_F * playerMotionConfigs[motionIndex].motionEndTime);
        }
    }
}

s32 IsPlayerMotionComplete(OMOBJ *object)
{
    s32 motionFinished;
    M443DllPlayerWork *playerWork;

    playerWork = object->data;
    motionFinished = 0;
    if ((CharMotionEndCheck(playerWork->characterId) != 0) && (CharMotionShiftIDGet(playerWork->characterId) < 0)) {
        motionFinished = 1;
    }
    return motionFinished;
}

s32 SetPlayerMotionIfNeeded(OMOBJ *object, u32 motionIndex)
{
    s32 motionStarted;
    s32 motionFinished;
    M443DllPlayerWork *playerWork;

    motionStarted = 0;
    playerWork = object->data;
    motionFinished = 0;
    if ((CharMotionEndCheck(playerWork->characterId) != 0) && (CharMotionShiftIDGet(playerWork->characterId) < 0)) {
        motionFinished = 1;
    }

    if (motionFinished != 0) {
        SetPlayerMotion(object, motionIndex);
        motionStarted = 1;
    }
    return motionStarted;
}

u32 speedOverlaySpriteResources[11][2] = {
    { 0x4A0015, 0xFA },
    { 0x4A0016, 0xF0 },
    { 0x4A0017, 0xE6 },
    { 0x4A0018, 0xF0 },
    { 0x4A0019, 0xF0 },
    { 0x4A001A, 0xF0 },
    { 0x4A001B, 0xF0 },
    { 0x61002E, 0xF0 },
    { 0x61002E, 0xF0 },
    { 0x61002E, 0xF0 },
    { 0x4A001C, 0xF0 },
};

Vec2f speedOverlayPositions[4] = {
    { 64.0f, 54.0f },
    { 46.0f, 54.0f },
    { 64.0f, 44.0f },
    { 46.0f, 44.0f },
};

void CreateSpeedOverlay(OMOBJ *object)
{
    s32 spriteIndex;
    M443DllPlayerWork *playerWork;
    M443DllOverlayWork *speedOverlay;
    f32 *unusedOverlayValuePointer;

    playerWork = object->data;
    speedOverlay = &playerWork->speedOverlay;

    for (spriteIndex = 0; spriteIndex < 11; spriteIndex++) {
        speedOverlay->spriteIds[spriteIndex] = -1;
        if (speedOverlaySpriteResources[spriteIndex][0] != 0) {
            speedOverlay->spriteIds[spriteIndex] = espEntry(speedOverlaySpriteResources[spriteIndex][0], speedOverlaySpriteResources[spriteIndex][1], 0);
            espAttrSet(speedOverlay->spriteIds[spriteIndex], HUSPR_ATTR_LINEAR);
            espDispOff(speedOverlay->spriteIds[spriteIndex]);
        }
    }
    espColorSet(speedOverlay->spriteIds[0], 0, 0, 0);
    espTPLvlSet(speedOverlay->spriteIds[0], 0.5);
    espScaleSet(speedOverlay->spriteIds[1], 0.75, 0.75);
    espScaleSet(speedOverlay->spriteIds[2], 0.75, 0.75);
    speedOverlay->iconPosition = speedOverlayPositions[object->work[0]];
}

f32 speedDigitOffsets[][2] = {
    { 20.0f, 4.0f },
    { 18.0f, 6.0f },
    { 46.0f, 33.0f },
    { 72.0f, 32.0f },
    { 72.0f, 14.0f },
    { 72.0f, -4.0f },
    { 72.0f, -22.0f },
    { -140.0f, 32.0f },
    { -158.0f, 32.0f },
    { -176.0f, 32.0f },
    { -96.0f, 32.0f },
};

void UpdateSpeedOverlay(OMOBJ *object)
{
    f32 engineNeedleJitter;
    s32 gearIndicatorBank;
    s32 spriteIndex;
    u32 throttleDisplay;
    M443DllPlayerWork *playerWork;
    M443DllOverlayWork *speedOverlay;

    playerWork = object->data;
    speedOverlay = &playerWork->speedOverlay;
    speedOverlay->animationFrame++;

    for (spriteIndex = 0; spriteIndex < 11; spriteIndex++) {
        if (speedOverlay->spriteIds[spriteIndex] >= 0) {
            if (speedOverlay->isVisible == 0) {
                espDispOff(speedOverlay->spriteIds[spriteIndex]);
            }
            else {
                espDispOn(speedOverlay->spriteIds[spriteIndex]);
                espPosSet(speedOverlay->spriteIds[spriteIndex], 0.9f * (playerWork->resultCamera->viewportCenterX + speedOverlay->iconPosition.x + speedDigitOffsets[spriteIndex][0]),
                    playerWork->resultCamera->viewportCenterY + speedOverlay->iconPosition.y + speedDigitOffsets[spriteIndex][1]);
            }
        }
    }
    engineNeedleJitter = 0.0f;
    if (playerWork->engineSpeed > 8000.0f) {
        engineNeedleJitter = (0.004f * frandmod(0x3E8)) - 2.0f;
    }
    espZRotSet(speedOverlay->spriteIds[2], (90.0f * (0.0001f * playerWork->engineSpeed)) + engineNeedleJitter);

    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        gearIndicatorBank = 0;
        if (playerWork->gear == spriteIndex) {
            gearIndicatorBank = 2;
        }
        else if (((playerWork->gear + 1) == spriteIndex) && (speedOverlay->blinkFrameOffset != 0)) {
            gearIndicatorBank += (speedOverlay->animationFrame >> 1) & 1;
        }
        espBankSet(speedOverlay->spriteIds[spriteIndex + 3], gearIndicatorBank);
    }
    throttleDisplay = (3.6f * playerWork->throttle);
    spriteIndex = throttleDisplay % 10;
    espBankSet(speedOverlay->spriteIds[7], spriteIndex);
    throttleDisplay = (throttleDisplay - spriteIndex) / 10;
    spriteIndex = throttleDisplay % 10;
    espBankSet(speedOverlay->spriteIds[8], spriteIndex);
    throttleDisplay = (throttleDisplay - spriteIndex) / 10;
    spriteIndex = throttleDisplay % 10;
    espBankSet(speedOverlay->spriteIds[9], spriteIndex);
}

f32 gearEngineSpeeds[4] = { 18.5f, 9.25f, 6.66f, 5.55f };

f32 GetGearEngineSpeed(u32 gear)
{
    if (gear >= 4) {
        if (gear >= 4) { }
    }
    return gearEngineSpeeds[gear];
}

f32 GetEngineAudioResponse(f32 engineSpeed)
{
    f32 engineAudioResponse;
    f32 highRpmAudioModifier;

    engineAudioResponse = 0.8f * (0.00016666666f * (6000.0f - engineSpeed));
    engineAudioResponse = 0.6f * (1.0f - (engineAudioResponse * engineAudioResponse));
    highRpmAudioModifier = 0.000400000002f * (6500.0f - engineSpeed);
    highRpmAudioModifier = 0.4f * (1.0f - (highRpmAudioModifier * highRpmAudioModifier));
    if (highRpmAudioModifier > 0.0f) {
        engineAudioResponse += highRpmAudioModifier;
    }
    highRpmAudioModifier = engineSpeed - 8000.0f;
    if (highRpmAudioModifier > 0.0f) {
        highRpmAudioModifier = 1.0f - (0.00033333333f * highRpmAudioModifier);
        engineAudioResponse = highRpmAudioModifier * (engineAudioResponse * highRpmAudioModifier);
    }
    return 55.0f * engineAudioResponse;
}

f32 GetEngineAcceleration(u32 gear, f32 engineSpeed)
{
    return 1.7647059f * (GetEngineAudioResponse(engineSpeed) * GetGearEngineSpeed(gear));
}

f32 ScaleEngineResponse(f32 engineResponse)
{
    return 9.8f * (0.0012820513f * engineResponse);
}

f32 GetEngineSpeedRatio(s32 gear, f32 engineSpeed)
{
    return 3.204422f * ((0.016666668f * engineSpeed) / GetGearEngineSpeed(gear));
}

f32 GetEngineSpeedForGear(u32 gear, f32 engineSpeedRatio)
{
    return 0.31206876f * (60.0f * engineSpeedRatio) * GetGearEngineSpeed(gear);
}

f32 SquareEngineValue(f32 value)
{
    return 0.024899999f * (value * value);
}

f32 GetNormalizedEngineSpeed(u32 gear, f32 engineSpeed)
{
    return engineSpeed / GetGearEngineSpeed(gear);
}
f32 GetEngineSpeedFromRatio(u32 gear, f32 engineSpeedRatio)
{
    return engineSpeedRatio * GetGearEngineSpeed(gear);
}

void UpdateExhaustEffect(s32 modelId)
{
    s32 particleIndex;
    HU3DPARTICLE *particleSystem;
    HU3DPARTICLEDATA *particle;

    particleSystem = Hu3DData[modelId].hookData;
    particle = particleSystem->data;

    for (particleIndex = 0; particleIndex < particleSystem->maxCnt; particleIndex++, particle++) {
        if (particle->time != 0) {
            VECAdd(&particle->pos, &particle->vel, &particle->pos);
            particle->vel.z -= 18.0f / REFRESH_RATE_F;
            particle->scale += 2.0f;
            particle->color.a *= 0.98f;
            if (particle->time < REFRESH_RATE_F / 2.5f) {
                particle->color.a = particle->color.a * 0.9f;
            }
            if (--particle->time == 0) {
                particle->time = 0;
                particle->scale = 0.0f;
                particle->color.a = 0;
            }
        }
    }
    DCStoreRange(particleSystem->data, particleSystem->maxCnt * 0x44);
}

static inline f32 GetParticleSideDirection(u32 value)
{
    if ((value & 1) != 0) {
        return 1.0f;
    }
    else {
        return -1.0f;
    }
}

void EmitExhaustParticles(u32 playerIndex, Vec *position, f32 throttle, f32 engineSpeed)
{
    f32 particleAppearanceJitter;
    f32 particleSideDirection;
    HU3DPARTICLE *particleSystem;
    s32 particleIndex;
    u32 remainingParticlesToEmit;
    HU3DPARTICLEDATA *particle;

    particleSystem = Hu3DData[playerObjectsByIndex[playerIndex]->mdlId[3]].hookData;
    if (frandmod(0x3E8) > 1000.0f * throttle)
        return;

    remainingParticlesToEmit = 2;
    particle = particleSystem->data;

    for (particleIndex = 0; particleIndex < particleSystem->maxCnt; particleIndex++, particle++) {
        if (particle->time != 0)
            continue;

        particle->time = (REFRESH_RATE_F * (0.6f + (0.0005f * frandmod(0x3E8))));
        particleSideDirection = GetParticleSideDirection(remainingParticlesToEmit);
        particleAppearanceJitter = 0.030000001f * frandmod(0x3E8);
        particle->pos.x = position->x + particleSideDirection * (70.0f + (0.07f * frandmod(0x3E8)));
        particle->pos.y = 10.0f + (position->y + (0.02f * frandmod(0x3E8)));
        particle->pos.z = (250.0f + position->z);
        particleAppearanceJitter = (50.0f * REFRESH_FREQ) + (VERSION_PAL ? 0.002f : REFRESH_FREQ / 10.0f) * frandmod(0x3E8);
        particleAppearanceJitter *= 1.0f + throttle;
        particle->vel.x = ((VERSION_PAL ? 0.003f : 0.0025f) * frandmod(0x3E8) - (75.0f / REFRESH_RATE_F));
        particle->vel.y = (200.0f * REFRESH_FREQ + (VERSION_PAL ? 0.002f : 0.0016666667f) * frandmod(0x3E8));
        particle->vel.z = ((250.0f * REFRESH_FREQ + ((VERSION_PAL ? 0.002f : 0.0016666667f) * frandmod(0x3E8))) - (REFRESH_FREQ * (0.5f * engineSpeed)));
        particle->zRot = frandmod(0x168);
        particleAppearanceJitter = 0.001f * frandmod(0x3E8);
        particle->scale = (50.0f + (100.0f * particleAppearanceJitter));
        particle->color.a = (70.0f + (80.0f * (1.0f - particleAppearanceJitter)));
        particle->color.r = (50.0f + (145.0f * particleAppearanceJitter));
        particle->color.g = (55.0f + (140.0f * particleAppearanceJitter));
        particle->color.b = (70.0f + (125.0f * particleAppearanceJitter));
        if (--remainingParticlesToEmit < 1)
            break;
    }
}

void ClearExhaustParticles(u32 playerIndex)
{
    s32 particleIndex;
    HU3DPARTICLE *particleSystem;
    HU3DPARTICLEDATA *particle;

    particleSystem = Hu3DData[playerObjectsByIndex[playerIndex]->mdlId[3]].hookData;
    particle = particleSystem->data;

    for (particleIndex = 0; particleIndex < particleSystem->maxCnt; particleIndex++, particle++) {
        particle->time = 0;
        particle->scale = 0.0f;
        particle->color.a = 0;
    }
}

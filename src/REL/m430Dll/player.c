#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXStruct.h"
#include "dolphin/gx/GXVert.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/flag.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "string.h"

#include "REL/m430Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/frand.h"
#endif

inline double Fabs2(double x)
{
    return (double)fabs(x);
}

typedef struct M430PlayerMotionResource {
    /* 0x00 */ s32 motionDataId;
    /* 0x04 */ s32 motionSlot;
} M430PlayerMotionResource; /* size = 0x08 */

typedef struct M430PlayerSpriteLayout {
    s32 spriteOriginX;
    s32 spriteOriginY;
    s32 spriteOffsetX;
    s32 spriteOffsetY;
    s32 spriteIds[6];
    s32 spritePositions[6][3];
    s32 spriteTargetPositions[6][3];
    s32 spriteCount;
    s32 spriteIndex;
} M430PlayerSpriteLayout; /* size = 0xC0 */

typedef struct M430PlayerParticleEffect {
    s32 effectIndex;
    s32 particleModelId;
    s32 cameraIndex;
    s32 effectState;
    s32 effectFrame;
    ANIMDATA *particleAnimation;
    Vec position;
    Vec rotation;
    Vec scale;
} M430PlayerParticleEffect; /* size = 0x3C */

typedef struct M430PlayerModelEffect {
    s32 effectIndex;
    s32 modelId;
    s32 playerIndex;
    s32 effectState;
    s32 effectFrame;
    Vec position;
    Vec rotation;
    Vec scale;
} M430PlayerModelEffect; /* size = 0x38 */

typedef struct M430PlayerAnimatedEffect {
    s32 effectIndex;
    s32 modelId;
    s32 particleModelId;
    s32 playerIndex;
    s32 effectState;
    s32 effectFrame;
    float elapsedTime;
    Vec position;
    float rotX;
    float rotY;
    float rotZ;
    float scaleX;
    float scaleY;
    float scaleZ;
} M430PlayerAnimatedEffect; /* size = 0x40 */

typedef struct M430PlayerWaterHook {
    s32 cameraIndex;
    s32 hookModelId;
    s32 isActive;
    float interpolationProgress;
    float startX;
    float startY;
    float startZ;
    float currentX;
    float currentY;
    float currentZ;
    float targetX;
    float targetY;
    float targetZ;
    float originX;
    float originY;
    float originZ;
    float deltaX;
    float deltaY;
    float deltaZ;
    Vec *interpolationPoints;
    Vec *renderPoints;
    Vec2f *textureCoordinates;
    s32 displayListCapacity;
    u32 displayListLength;
    void *displayList;
} M430PlayerWaterHook; /* size = 0x64 */

typedef struct M430PlayerPairState {
    s32 playerIds[2];
    s32 playerFlags;
    float playerX;
    float playerY;
    float playerZ;
    float previousPlayerZ;
    float scrollSpeed;
    s32 collectedCoins;
} M430PlayerPairState; /* size = 0x24 */

typedef struct M430PlayerWork {
    u32 playerIndex;
    u16 playerSide;
    u16 padIndex;
    u16 isComputer;
    u16 difficulty;
    u16 playerMode;
    u16 playerState;
    u16 stateFrame;
    u16 tiltFrame;
    u16 roundState;
    u16 randomTiltPhase;
    float homePositionX;
    float rotationY;
    float positionY;
    float forwardSpeed;
    float horizontalVelocity;
    float verticalImpulse;
    float forwardAcceleration;
    s32 lifeCount;
    s32 characterId;
    char reservedPlayerMetadata[4];
    s32 buttonsHeld;
    u32 buttonsPressed;
    float stickX;
    float stickY;
    float nearbySpawnDistance;
    float farSpawnDistance;
    s32 motionIndex;
    s32 inputFlags;
    s32 startSoundId;
    s32 landingSoundId;
    s32 soundVolume;
    s32 particleEmitterId;
    float movementSpeed;
    s32 randomMotionTimerFrames;
    s32 otherPlayerIndex;
    float randomMotionOffset;
} M430PlayerWork; /* size = 0x80 */

void InitializePlayerWaterHooks(void);
void ReleasePlayerWaterHooks(void);
void EmptyWaterHookCallback(void);
void SetWaterHookPoint(s32 cameraIndex, s32 isTargetPosition, float positionX, float positionY, float positionZ);
void SetWaterHookProgress(s32 cameraIndex, float interpolationProgress);
void DisableWaterHook(s32 cameraIndex);
void UpdateWaterHookGeometry(M430PlayerWaterHook *cameraHook);
void DrawWaterHook(HU3DMODEL *model, Mtx matrix);
void CreateWaterHooks(void);
void DestroyWaterHooks(void);
void EmptyPlayerEffectCallback(void);
s32 FindPlayerEffectAtPosition(s32 playerIndex, float positionX, float positionY, float positionZ);
s32 SpawnPlayerEffectAtPosition(s32 playerIndex, float positionX, float positionY, float positionZ, s32 consumeEffect);
s32 FindNearestPlayerEffect(s32 playerIndex, float maxPositionZ, Vec *nearestEffectPosition);
void RemovePlayerEffect(s32 playerIndex);
void InitializePlayerEffects(void);
void UpdatePlayerEffects(void);
void EmptySecondaryEffectCallback(void);
s32 FindSecondaryEffectAtPosition(s32 playerIndex, float positionX, float positionY, float positionZ);
s32 SpawnSecondaryEffectAtPosition(s32 playerIndex, float positionX, float positionY, float positionZ, s32 consumeEffect);
s32 FindNearestSecondaryEffect(s32 playerIndex, float maxPositionZ, Vec *nearestEffectPosition);
M430PlayerModelEffect *GetPlayerModelEffect(s32 effectIndex);
void RemovePlayerModelEffect(s32 effectIndex);
void InitializePlayerModelEffects(void);
void UpdatePlayerModelEffects(void);
void InitializePlayerSplashEffects(void);
s32 FindPlayerSplashEffect(s32 cameraIndex, float positionX, float positionY, float positionZ);
void RemovePlayerSplashEffect(s32 playerIndex);
void UpdatePlayerSplashEffects(void);
void InitializePlayerSprites(void);
void ReleasePlayerSprites(void);
void SetPlayerSpritesVisible(s32 isVisible);
void InterpolateBezierVector(float progress, float *startValues, float *controlValues, float *endValues, float *outputValues);
float GetShortestAngleDifference(float angleA, float angleB);
float ApproachAngle(float targetAngle, float currentAngle, float maxStep);
void ReleasePlayerAnimation(ANIMDATA *animation);
void RotateVectorByMatrix(Mtx rotationMatrix, Vec *vector);
void CreatePlayer(OMOBJ *object);
void InitializePlayerAssets(void);
void UpdatePlayer(OMOBJ *object);
void UpdatePlayerActions(OMOBJ *object);
void UpdatePlayerMotion(OMOBJ *object);
void UpdatePlayerEffects(OMOBJ *object);
void FinishPlayerRendering(OMOBJ *object);
void UpdatePlayerSprites(OMOBJ *object);
s32 IsPlayerCleanupComplete(OMOBJ *object);
void CleanupPlayer(OMOBJ *object);
void UpdatePlayerDisplay(OMOBJ *object);

M430PlayerMotionResource playerMotionResources[7] = {
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x42), -1 },
    { 0, 1 },
    { 0, 2 },
    { 0, 3 },
    { 0, -1 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17), -1 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18), -1 },
};
M430PlayerMotionResource playerEffectMotionResources[4] = {
    { 0, 4 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00), -1 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17), -1 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18), -1 },
};
s32 playerEffectModelAssets[5] = { 0, DATA_MAKE_NUM(DATADIR_M430, 0x31), DATA_MAKE_NUM(DATADIR_M430, 0x39), DATA_MAKE_NUM(DATADIR_M430, 0x41),
    DATA_MAKE_NUM(DATADIR_M430, 0x49) };

float playerLanePositions[] = { -262.5f, -173.25f, -86.625f, 0.0f, 86.625f, 173.25f, 262.5f,
#ifdef NON_MATCHING
    0.0f,
#endif
};

Vec playerSpawnOffsets[8] = {
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 160.0f },
    { 60.000004f, 0.0f, 0.0f },
    { -60.000004f, 0.0f, 0.0f },
    { 50.0f, 0.0f, 100.0f },
    { -50.0f, 0.0f, 100.0f },
    { 60.000004f, 0.0f, -120.00001f },
    { -60.000004f, 0.0f, -120.00001f },
};
float playerSpawnHeightOffsets[8] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 25.0f, 0.0f, 25.0f };
s32 playerEffectAssetsA[2] = { DATA_MAKE_NUM(DATADIR_M430, 0x1C), DATA_MAKE_NUM(DATADIR_M430, 0x1D) };
s32 playerEffectAssetsB[2] = { DATA_MAKE_NUM(DATADIR_M430, 0x1E), DATA_MAKE_NUM(DATADIR_M430, 0x1F) };
s32 playerEffectAssetsC[2] = { DATA_MAKE_NUM(DATADIR_M430, 0x20), DATA_MAKE_NUM(DATADIR_M430, 0x21) };

s32 useSharedCamera;
HUPROCESS *playerObjectProcess;
M430PlayerPairState *playerPairStates;
M430PlayerWaterHook *playerWaterHooks;
M430PlayerAnimatedEffect *playerAnimatedEffects;
M430PlayerModelEffect *playerModelEffects;
M430PlayerParticleEffect *playerParticleEffects;
M430PlayerSpriteLayout *playerSpriteLayouts;
s32 playerDisplayEnabled;
s32 playerPairStateActive;

void InitializePlayers(HUPROCESS *process)
{
    Vec listenerPosition;
    Vec listenerDirection;

    playerObjectProcess = process;
    InitializePlayerAssets();
    InitializePlayerWaterHooks();
    CreateWaterHooks();
    InitializePlayerEffects();
    InitializePlayerModelEffects();
    UpdatePlayerSplashEffects();
    omAddObjEx(process, 0x2D, 0, 0, -1, CreatePlayer);
    listenerPosition.x = CenterM[0].x + (CZoomM[0] * (sind(CRotM[0].y) * cosd(CRotM[0].x)));
    listenerPosition.y = CenterM[0].y + (CZoomM[0] * -sind(CRotM[0].x));
    listenerPosition.z = CenterM[0].z + (CZoomM[0] * (cosd(CRotM[0].y) * cosd(CRotM[0].x)));
    VECSubtract(CenterM, &listenerPosition, &listenerDirection);
    VECNormalize(&listenerDirection, &listenerDirection);
    HuAudFXListnerSetEX(&listenerPosition, &listenerDirection, 100000.0f, 34000.0f / REFRESH_RATE_F, 0.0f, 300.0f, 300.0f);
}

void CreatePlayer(OMOBJ *object)
{
    Vec listenerPosition;
    Vec listenerDirection;

    ReleasePlayerWaterHooks();
    DestroyWaterHooks();
    UpdatePlayerEffects();
    UpdatePlayerModelEffects();
    InitializePlayerSprites();
    listenerPosition.x = CenterM[0].x + (CZoomM[0] * (sind(CRotM[0].y) * cosd(CRotM[0].x)));
    listenerPosition.y = CenterM[0].y + (CZoomM[0] * -sind(CRotM[0].x));
    listenerPosition.z = CenterM[0].z + (CZoomM[0] * (cosd(CRotM[0].y) * cosd(CRotM[0].x)));
    VECSubtract(CenterM, &listenerPosition, &listenerDirection);
    VECNormalize(&listenerDirection, &listenerDirection);
    HuAudFXListnerUpdate(&listenerPosition, &listenerDirection);
}

void ReleasePlayerAssets(void)
{
    HuAudFXListnerKill();
    EmptyWaterHookCallback();
    EmptyPlayerEffectCallback();
    EmptySecondaryEffectCallback();
    InitializePlayerSplashEffects();
    ReleasePlayerSprites();
}

void InitializePlayerAssets(void)
{
    s8 firstPlayerByGroup[4];
    s32 groupSearchComplete;
    s32 playerSlot;
    OMOBJ *playerObject;
    s32 configuredGroup;
    s32 playerIndex;

    groupSearchComplete = 0;
    useSharedCamera = 1;
    omMakeGroupEx(playerObjectProcess, 0, 4);
    omGetGroupMemberListEx(playerObjectProcess, 0);
    playerPairStates = HuMemDirectMallocNum(HEAP_HEAP, 2 * sizeof(M430PlayerPairState), HU_MEMNUM_OVL);
    memset(playerPairStates, 0, 2 * sizeof(M430PlayerPairState));
    for (playerSlot = 0; playerSlot < 4; playerSlot++) {
        firstPlayerByGroup[playerSlot] = -1;
    }
    for (playerSlot = 0; playerSlot < 4; playerSlot++) {
        configuredGroup = GWPlayerCfg[playerSlot].group;
        if (firstPlayerByGroup[configuredGroup] == -1) {
            firstPlayerByGroup[configuredGroup] = playerSlot;
        }
        else {
            break;
        }
    }
    if (playerSlot < 4) {
        firstPlayerByGroup[0] = 0;
        firstPlayerByGroup[1] = 1;
        firstPlayerByGroup[2] = 2;
        firstPlayerByGroup[3] = 3;
    }
    for (playerSlot = 0; playerSlot < 4; playerSlot++) {
        playerObject = omAddObjEx(playerObjectProcess, 0x28, 3, 7, 0, UpdatePlayer);
        playerIndex = firstPlayerByGroup[playerSlot];
        playerObject->work[0] = playerIndex;
        playerObject->work[1] = playerSlot >> 1;
        playerObject->work[2] = playerSlot & 1;
    }
}

void UpdatePlayer(OMOBJ *object)
{
    M430PlayerWork *work;
    s32 motionSlot;
    M430PlayerPairState *playerSideData;
    u32 playerIndex;
    HSFDATA *modelData;
    HSFMATERIAL *material;
    HU3DMODEL *model;
    s32 characterId;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M430PlayerWork), HU_MEMNUM_OVL);
    work = object->data;
    memset(work, 0, sizeof(M430PlayerWork));
    playerIndex = object->work[0];
    characterId = GWPlayerCfg[playerIndex].character;
    work->characterId = characterId;
    work->playerIndex = playerIndex;
    work->playerSide = object->work[1];
    work->padIndex = GWPlayerCfg[playerIndex].pad_idx;
    work->isComputer = GWPlayerCfg[playerIndex].iscom;
    work->difficulty = GWPlayerCfg[playerIndex].diff;
    work->playerMode = object->work[2];
    work->playerState = 0;
    work->stateFrame = 0;
    work->tiltFrame = 0;
    work->roundState = 0;
    work->randomTiltPhase = 0;
    if (work->playerSide == 0) {
        work->homePositionX = 450.0f;
    }
    else {
        work->homePositionX = -450.0f;
    }
    work->lifeCount = 1;
    work->forwardSpeed = 750.0f * REFRESH_FREQ;
    work->horizontalVelocity = 0.0f;
    work->rotationY = 180.0f;
    work->verticalImpulse = 0.0f;
    work->forwardAcceleration = 0.0f;
    work->positionY = 0.0f;
    work->buttonsHeld = work->buttonsPressed = 0;
    work->stickX = work->stickY = 0.0f;
    work->nearbySpawnDistance = 0.0f;
    work->farSpawnDistance = 0.0f;
    work->randomMotionTimerFrames = 0;
    work->otherPlayerIndex = -1;
    work->randomMotionOffset = 0.0f;
    work->startSoundId = -1;
    work->landingSoundId = -1;
    work->soundVolume = -1;
    work->particleEmitterId = -1;
    work->movementSpeed = playerSpawnHeightOffsets[characterId];
    object->mdlId[0] = CharModelCreate(work->characterId, 4);
    Hu3DModelCameraSet(object->mdlId[0], 3);
    Hu3DModelLayerSet(object->mdlId[0], 6);
    CharModelStepFxSet(work->characterId, 3);
    if (work->playerMode == 0) {
        for (motionSlot = 0; motionSlot < 7; motionSlot++) {
            if ((playerMotionResources[motionSlot].motionSlot) == 0xFFFFFFFF) {
                object->mtnId[motionSlot] = CharMotionCreate(work->characterId, playerMotionResources[motionSlot].motionDataId);
            }
            else {
                object->mtnId[motionSlot] = Hu3DJointMotionFile(object->mdlId[0], work->characterId + playerEffectModelAssets[playerMotionResources[motionSlot].motionSlot]);
            }
            if (motionSlot == 5) {
                CharMotionVoiceOnSet(work->characterId, object->mtnId[motionSlot], 0);
            }
        }
        work->motionIndex = 0;
        CharMotionSet(work->characterId, object->mtnId[work->motionIndex]);
        object->mdlId[1] = Hu3DModelCreateFile(playerEffectAssetsA[work->playerSide]);
        Hu3DModelLayerSet(object->mdlId[1], 2);
        Hu3DModelCameraSet(object->mdlId[1], 3);
        Hu3DModelScaleSet(object->mdlId[1], 1.2f, 1.2f, 1.2f);
        object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M430, 0x25));
        Hu3DModelLayerSet(object->mdlId[2], 1);
        Hu3DModelCameraSet(object->mdlId[2], 3);
        work->startSoundId = HuAudFXPlay(0x67A);
        work->soundVolume = 0x50;
        if (work->startSoundId != -1) {
            HuAudFXVolSet(work->startSoundId, work->soundVolume);
        }
        if (work->playerSide == 0) {
            work->landingSoundId = HuAudFXPlay(0x675);
        }
        else {
            work->landingSoundId = HuAudFXPlay(0x676);
        }
        if (work->landingSoundId != -1) {
            HuAudFXPitchSet(work->landingSoundId, 0);
        }
    }
    else {
        for (motionSlot = 0; motionSlot < 4; motionSlot++) {
            if (playerEffectMotionResources[motionSlot].motionSlot == 0xFFFFFFFF) {
                object->mtnId[motionSlot] = CharMotionCreate(work->characterId, playerEffectMotionResources[motionSlot].motionDataId);
            }
            else {
                object->mtnId[motionSlot] = Hu3DJointMotionFile(object->mdlId[0], work->characterId + playerEffectModelAssets[playerEffectMotionResources[motionSlot].motionSlot]);
            }
            if (motionSlot == 2) {
                CharMotionVoiceOnSet(work->characterId, object->mtnId[motionSlot], 0);
            }
        }
        work->motionIndex = 0;
        CharMotionSet(work->characterId, object->mtnId[work->motionIndex]);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
        object->mdlId[1] = Hu3DModelCreateFile(playerEffectAssetsB[work->playerSide]);
        Hu3DModelLayerSet(object->mdlId[1], 7);
        Hu3DModelCameraSet(object->mdlId[1], 3);
        Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_LOOP);
        Hu3DMotionSpeedSet(object->mdlId[1], 2.0f);
        object->mdlId[2] = Hu3DModelCreateFile(playerEffectAssetsC[work->playerSide]);
        Hu3DModelLayerSet(object->mdlId[2], 6);
        Hu3DModelCameraSet(object->mdlId[2], 3);
    }
    CharMotionDataClose(work->characterId);
    object->trans.x = work->homePositionX;
    if (work->playerMode == 0) {
        object->trans.y = 0.0f;
    }
    else {
        object->trans.y = 200.0f;
    }
    if (work->playerSide == 0) {
        object->trans.z = -100000.0f;
    }
    else {
        object->trans.z = -101000.0f;
    }
    object->rot.y = 180.0f;
    work->positionY = object->trans.y;
    playerSideData = &playerPairStates[work->playerSide];
    if (work->playerMode == 0) {
        playerSideData->playerIds[work->playerMode] = work->playerIndex;
        if (work->isComputer != 0) {
            characterId = playerSideData->playerFlags | 1;
            playerSideData->playerFlags = characterId;
        }
        playerSideData->playerX = object->trans.x;
        playerSideData->playerY = 0.0f;
        playerSideData->playerZ = object->trans.z;
        playerSideData->previousPlayerZ = object->trans.z;
        playerSideData->scrollSpeed = work->forwardSpeed;
        playerSideData->collectedCoins = 0;
        playerPairStateActive = 1;
    }
    else {
        playerSideData->playerIds[work->playerMode] = work->playerIndex;
        if (work->isComputer != 0) {
            playerSideData->playerFlags = playerSideData->playerFlags | 2;
        }
    }
    if (work->playerMode != 0) {
        model = &Hu3DData[object->mdlId[1]];
        modelData = model->hsf;
        for (material = modelData->material, motionSlot = 0; motionSlot < modelData->materialNum; motionSlot++, material++) {
            material->flags |= 2;
        }
    }
    object->objFunc = UpdatePlayerActions;
    (void)model;
    (void)model;
}

void UpdatePlayerActions(OMOBJ *object)
{
    Vec particlePosition;
    float transitionProgress;
    float modelOffsetX;
    float modelOffsetZ;
    M430PlayerWork *work;
    s32 modelIndex;
    M430PlayerPairState *playerSideData;
    HSFDATA *modelData;
    HSFMATERIAL *material;
    HU3DMODEL *model;

    work = object->data;
    playerSideData = &playerPairStates[work->playerSide];
    work->buttonsHeld = 0;
    work->buttonsPressed = 0;
    work->stickX = 0.0f;
    work->stickY = 0.0f;
    if (work->playerMode == 0) {
        work->lifeCount = 3;
        work->forwardSpeed = -2460.0f * REFRESH_FREQ;
        object->rot.y = 0.0f;
        object->rot.z = 0.0f;
        object->trans.z -= work->forwardAcceleration;
        UpdatePlayerEffects(object);
        if (work->startSoundId != -1) {
            HuAudFXVolSet(work->startSoundId, work->soundVolume);
        }
        if (work->landingSoundId != -1) {
            HuAudFXPitchSet(work->landingSoundId, (s32)(8192.0f * (work->soundVolume / 127.0f)));
        }
        object->rot.y = object->rot.z = 0.0f;
        if (++work->stateFrame > (REFRESH_RATE_F * (1.5f + (0.5f * work->playerSide)))) {
            work->forwardAcceleration += 30.000002f;
            playerSideData->previousPlayerZ = object->trans.z = object->trans.z + work->forwardAcceleration;
            work->soundVolume += 3;
            if (work->soundVolume > 0x7F) {
                work->soundVolume = 0x7F;
            }
        }
        else {
            playerSideData->previousPlayerZ = object->trans.z;
        }
        modelOffsetX = work->movementSpeed * sind(object->rot.y);
        modelOffsetZ = work->movementSpeed * cosd(object->rot.y);
        Hu3DModelPosSet(object->mdlId[1], object->trans.x + modelOffsetX, object->trans.y, object->trans.z + modelOffsetZ);
        Hu3DModelRotSet(object->mdlId[1], object->rot.x, object->rot.y, object->rot.z);
        Hu3DModelPosSet(object->mdlId[2], object->trans.x + modelOffsetX, object->trans.y - 150.0f, object->trans.z + modelOffsetZ);
        Hu3DModelRotSet(object->mdlId[2], object->rot.x, 180.0f + object->rot.y, object->rot.z);
        SetWaterHookPoint(work->playerSide, 0, object->trans.x, 25.0f + object->trans.y, object->trans.z - 100.0f);
        if (work->stateFrame > REFRESH_RATE_F / 2) {
            transitionProgress = REFRESH_FREQ * (work->stateFrame - REFRESH_RATE_F / 2);
            if (transitionProgress > 1.0f) {
                transitionProgress = 1.0f;
            }
            transitionProgress = sind((90.0f * transitionProgress));
            transitionProgress *= transitionProgress;
        }
        else {
            transitionProgress = 0.0f;
        }
        CenterM[work->playerSide].x = work->homePositionX - (transitionProgress * work->homePositionX);
        CenterM[work->playerSide].z = (950.0f * (0.5f + (0.5f * transitionProgress))) + getWaterCameraX(work->playerSide);
    }
    else {
        work->rotationY = 0.0f;
        object->rot.y = 0.0f;
        object->rot.z = 0.0f;
        FinishPlayerRendering(object);
        work->rotationY = 0.0f;
        object->rot.y = 0.0f;
        object->rot.z = 0.0f;
        object->trans.y = 200.0f;
        object->trans.z = playerSideData->previousPlayerZ - 300.0f;
        object->rot.z = 0.0f;
        for (modelIndex = 0; modelIndex < 2; modelIndex++) {
            Hu3DModelPosSet(object->mdlId[modelIndex + 1], object->trans.x, 150.0f + object->trans.y, 50.0f + object->trans.z);
            Hu3DModelRotSet(object->mdlId[modelIndex + 1], object->rot.x, object->rot.y, object->rot.z);
        }
        SetWaterHookPoint(work->playerSide, 1, object->trans.x, 150.0f + object->trans.y, 50.0f + object->trans.z);
        if (work->particleEmitterId == -1) {
            particlePosition.x = object->trans.x;
            particlePosition.y = object->trans.y;
            particlePosition.z = object->trans.z;
            work->particleEmitterId = HuAudFXEmiterPlay(0x67B, &particlePosition);
        }
        else {
            particlePosition.x = object->trans.x;
            particlePosition.y = object->trans.y;
            particlePosition.z = object->trans.z;
            HuAudFXEmiterUpDate(work->particleEmitterId, &particlePosition);
        }
    }
    if (getIntroPhase() == 2) {
        if (work->playerMode == 0) {
            work->lifeCount = 1;
            work->forwardSpeed = 750.0f * REFRESH_FREQ;
            work->horizontalVelocity = 0.0f;
            work->soundVolume = 0x50;
            object->trans.y = 0.0f;
        }
        else {
            model = &Hu3DData[object->mdlId[1]];
            modelData = model->hsf;
            for (material = modelData->material, modelIndex = 0; modelIndex < modelData->materialNum; modelIndex++, material++) {
                material->flags &= 0xFFFFFFFD;
            }
            if (work->particleEmitterId != -1) {
                HuAudFXStop(work->particleEmitterId);
            }
            work->particleEmitterId = -1;
            if (work->playerSide == 0) {
                work->particleEmitterId = HuAudFXPlay(0x67C);
            }
            else {
                work->particleEmitterId = HuAudFXPlay(0x67D);
            }
            if (work->particleEmitterId != -1) {
                HuAudFXPitchSet(work->particleEmitterId, -0x2000);
            }
            work->horizontalVelocity = 0.0f;
            object->trans.y = 200.0f;
            object->rot.z = 180.0f;
        }
        object->trans.x *= -1.0f;
        work->homePositionX *= -1.0f;
        object->trans.x = work->homePositionX;
        object->trans.z = 0.0f;
        object->rot.x = 0.0f;
        object->rot.y = 180.0f;
        work->rotationY = 180.0f;
        work->positionY = object->trans.y;
        playerPairStateActive = 0;
        object->objFunc = UpdatePlayerMotion;
    }
}

void UpdatePlayerMotion(OMOBJ *object)
{
    s32 unusedInputFlags;
    s32 unusedInputValue;
    float speedRatio;
    M430PlayerWork *work;
    s32 cameraIndex;
    M430PlayerPairState *playerSideData;

    work = object->data;
    unusedInputFlags = 0;
    unusedInputValue = -1;
    playerSideData = &playerPairStates[work->playerSide];
    if ((playerDisplayEnabled == 0) && (getRoundState() >= 2)) {
        SetPlayerSpritesVisible(1);
    }
    if (useSharedCamera != 0) {
        Hu3DModelCameraSet(object->mdlId[0], 3);
        Hu3DModelCameraSet(object->mdlId[1], 3);
        if (object->mdlId[2] != -1) {
            Hu3DModelCameraSet(object->mdlId[2], 3);
        }
    }
    else {
        if (work->playerSide == 0) {
            cameraIndex = 1;
        }
        else {
            cameraIndex = 2;
        }
        Hu3DModelCameraSet(object->mdlId[0], cameraIndex);
        Hu3DModelCameraSet(object->mdlId[1], cameraIndex);
        if (object->mdlId[2] != -1) {
            Hu3DModelCameraSet(object->mdlId[2], cameraIndex);
        }
    }
    if (getRoundState() == 2) {
        work->buttonsHeld = HuPadBtn[work->padIndex];
        work->buttonsPressed = HuPadBtnDown[work->padIndex];
        work->stickX = HuPadStkX[work->padIndex];
        work->stickY = HuPadStkY[work->padIndex];
    }
    else {
        work->buttonsHeld = 0;
        work->buttonsPressed = 0;
        work->stickX = 0.0f;
        work->stickY = 0.0f;
    }
    if (getRoundState() < 2) {
        work->rotationY = 180.0f;
        work->horizontalVelocity = 0.0f;
        object->trans.x = work->homePositionX;
        object->rot.y = 180.0f;
    }
    if (work->playerMode == 0) {
        UpdatePlayerEffects(object);
        speedRatio = (work->forwardSpeed - (750.0f * REFRESH_FREQ)) / (1710.0001f * REFRESH_FREQ);
        work->soundVolume = 80.0f + (47.0f * speedRatio);
        if (work->startSoundId != -1) {
            HuAudFXVolSet(work->startSoundId, work->soundVolume);
        }
        if (work->landingSoundId != -1) {
            HuAudFXPitchSet(work->landingSoundId, (s32)(8192.0f * (work->soundVolume / 127.0f)));
        }
    }
    else {
        FinishPlayerRendering(object);
        speedRatio = (playerSideData->scrollSpeed - (750.0f * REFRESH_FREQ)) / (1710.0001f * REFRESH_FREQ);
        speedRatio = 8192.0f + (-16384.0f * (1.0f - speedRatio));
        if (work->particleEmitterId != -1) {
            HuAudFXPitchSet(work->particleEmitterId, speedRatio);
        }
    }
    if (getRoundState() == 3) {
        SetPlayerInputFlag(work->playerIndex);
        work->inputFlags = 0;
        return;
    }
    if (getRoundState() == 4) {
        work->playerState = 2;
        work->stateFrame = 0;
        if (work->playerMode == 0) {
            work->motionIndex = 4;
        }
        else {
            work->motionIndex = 1;
        }
        CharMotionSet(work->characterId, object->mtnId[work->motionIndex]);
        Hu3DModelCameraSet(object->mdlId[0], 1);
        Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
        if ((object->mdlId[1] != -1) && (work->playerMode != 0)) {
            Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
        }
        if (work->startSoundId != -1) {
            HuAudFXStop(work->startSoundId);
        }
        if (work->landingSoundId != -1) {
            HuAudFXStop(work->landingSoundId);
        }
        work->startSoundId = work->landingSoundId = -1;
        if (work->particleEmitterId != -1) {
            HuAudFXStop(work->particleEmitterId);
        }
        if (object->mdlId[2] != -1) {
            Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
        }
        DisableWaterHook(work->playerSide);
        RemovePlayerEffect(work->playerSide);
        RemovePlayerModelEffect(work->playerSide);
        RemovePlayerSplashEffect(work->playerSide);
        object->objFunc = UpdatePlayerSprites;
    }
}

void UpdatePlayerEffects(OMOBJ *object)
{
    Mtx rotationMatrix;
    Vec transformedOffset;
    Vec velocity;
    float spawnPositionX;
    float targetForwardSpeed;
    float forwardOffset;
    float speedMagnitude;
    float speedTiltOffset;
    float steeringRotation;
    float knockbackTilt;
    float steeringInput;
    float steeringInputMagnitude;
    float unusedVerticalOffset;
    float unusedRotationOffset;
    float unusedForwardOffset;
    float unusedLateralOffset;
    float unusedScaleOffset;
    M430PlayerWork *work;
    M430PlayerPairState *playerSideData;
    s32 collisionPointIndex;
    s32 motionIndex;

    work = object->data;
    motionIndex = -1;
    playerSideData = &playerPairStates[work->playerSide];
    if ((getRoundState() == 2) && (work->isComputer != 0)) {
        CleanupPlayer(object);
    }
    mtxRot(rotationMatrix, object->rot.x, object->rot.y, object->rot.z);
    rotationMatrix[1][0] *= object->scale.y;
    rotationMatrix[1][1] *= object->scale.y;
    rotationMatrix[1][2] *= object->scale.z;
    object->trans.y = 0.3f * (100.0f * (work->forwardSpeed / (2460.0f * REFRESH_FREQ)));
    if (object->trans.y < 0.0f) {
        object->trans.y *= -0.5f;
    }
    speedTiltOffset = 15.0f * (work->forwardSpeed / (2460.0f * REFRESH_FREQ));
    if (work->forwardSpeed < 0.0f) {
        speedTiltOffset *= -0.5f;
    }
    switch (work->playerState) {
        case 0:
            work->tiltFrame = work->lifeCount + (frand() & 7) + work->tiltFrame;
            work->roundState = work->lifeCount + (frand() & 7) + work->roundState;
            object->rot.x = (2.5 * sind(work->tiltFrame)) - speedTiltOffset;
            object->rot.z = 2.0 * cosd(work->roundState);
            for (collisionPointIndex = 0; collisionPointIndex < 8; collisionPointIndex++) {
                transformedOffset.x = playerSpawnOffsets[collisionPointIndex].x;
                transformedOffset.y = playerSpawnOffsets[collisionPointIndex].y;
                transformedOffset.z = playerSpawnOffsets[collisionPointIndex].z;
                RotateVectorByMatrix(&rotationMatrix[0], &transformedOffset);
                spawnPositionX = work->movementSpeed * sind(object->rot.y);
                forwardOffset = work->movementSpeed * cosd(object->rot.y);
                if (SpawnPlayerEffectAtPosition(
                        work->playerSide, spawnPositionX + (transformedOffset.x + object->trans.x), transformedOffset.y + object->trans.y, forwardOffset + (transformedOffset.z + object->trans.z), 1)
                    != 0) {
                    break;
                }
            }
            if (collisionPointIndex < 8) {
                work->motionIndex = 3;
                CharMotionShiftSet(work->characterId, object->mtnId[work->motionIndex], 0.0f, 10.0f, 0);
                work->playerState = 1;
                work->stateFrame = 0;
                if (work->playerSide == 0) {
                    HuAudFXPlay(0x678);
                }
                else {
                    HuAudFXPlay(0x67E);
                }
                CharFXPlay(work->characterId, 0x123);
                omVibrate(work->playerIndex, 0xC, 4, 2);
                work->verticalImpulse = 5.0f;
                work->randomTiltPhase = frand() & 3;
            }
            break;
        case 1:
            work->lifeCount = 1;
            work->buttonsPressed = 0;
            work->stickX = 0.0f;
            work->tiltFrame = work->lifeCount + (frand() & 7) + work->tiltFrame;
            work->roundState = work->lifeCount + (frand() & 7) + work->roundState;
            object->rot.x = (2.5 * sind(work->tiltFrame)) - speedTiltOffset;
            object->rot.z = 2.0 * cosd(work->roundState);
            work->verticalImpulse = 5.0f - (5.0f * ((0.66666666f / REFRESH_RATE_F) * work->stateFrame));
            knockbackTilt = 7.0 * (work->verticalImpulse * sind((10.0f * work->stateFrame)));
            if ((work->randomTiltPhase & 1) != 0) {
                object->rot.x += knockbackTilt;
            }
            else {
                object->rot.x -= knockbackTilt;
            }
            if ((work->randomTiltPhase & 2) != 0) {
                object->rot.z += knockbackTilt;
            }
            else {
                object->rot.z -= knockbackTilt;
            }
            if ((work->stateFrame & 1) != 0) {
                Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            else {
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            if (++work->stateFrame >= REFRESH_RATE_F * 1.5f) {
                work->playerState = 0;
                work->stateFrame = 0;
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            break;
    }
    steeringInput = work->stickX;
    steeringInputMagnitude = steeringInput;
    if (Fabs2(steeringInputMagnitude) < 8.0) {
        steeringInput = 0.0f;
    }
    work->horizontalVelocity *= 0.98f;
    work->horizontalVelocity += 0.105f / REFRESH_RATE_F * steeringInput;
    if (work->horizontalVelocity < -525.0f / REFRESH_RATE_F) {
        work->horizontalVelocity = -525.0f / REFRESH_RATE_F;
    }
    else if (work->horizontalVelocity > 525.0f / REFRESH_RATE_F) {
        work->horizontalVelocity = 525.0f / REFRESH_RATE_F;
    }
    object->trans.x += work->horizontalVelocity;
    steeringRotation = 180.0f - (30.0f * (0.015625f * steeringInput));
    work->rotationY = ApproachAngle(steeringRotation, work->rotationY, 1.0f);
    object->rot.y = work->rotationY;
    if (object->trans.x > (350.0f + work->homePositionX)) {
        object->trans.x = 350.0f + work->homePositionX;
    }
    else if (object->trans.x < (work->homePositionX - 350.0f)) {
        object->trans.x = work->homePositionX - 350.0f;
    }
    if (work->playerState == 0) {
        if ((work->stickX > 8.0f) && (object->rot.y < 170.0f)) {
            motionIndex = 1;
        }
        else if ((work->stickX < -8.0f) && (object->rot.y > 190.0f)) {
            motionIndex = 2;
        }
        else {
            motionIndex = 0;
        }
        if ((motionIndex != -1) && (work->motionIndex != motionIndex) && (CharMotionShiftIDGet(work->characterId) < 0)) {
            work->motionIndex = motionIndex;
            CharMotionShiftSet(work->characterId, object->mtnId[work->motionIndex], 0.0f, 10.0f, 0);
        }
    }
    if ((work->buttonsPressed & 0x100) != 0) {
        work->lifeCount++;
    }
    else if ((work->buttonsPressed & 0x200) != 0) {
        work->lifeCount--;
    }
    if (work->lifeCount < 1) {
        work->lifeCount = 1;
    }
    else if (work->lifeCount > 3) {
        work->lifeCount = 3;
    }
    switch (work->lifeCount) {
        case 1:
            targetForwardSpeed = 750.0f * REFRESH_FREQ;
            (void)targetForwardSpeed;
            break;
        case 2:
            targetForwardSpeed = 1675.0f * REFRESH_FREQ;
            (void)targetForwardSpeed;
            break;
        default:
            targetForwardSpeed = 2460.0f * REFRESH_FREQ;
            break;
    }
    if (fabs(targetForwardSpeed - work->forwardSpeed) < REFRESH_FREQ * 100.0f) {
        work->forwardSpeed = targetForwardSpeed;
    }
    else if (targetForwardSpeed > work->forwardSpeed) {
        work->forwardSpeed += REFRESH_FREQ * 100.0f;
    }
    else {
        work->forwardSpeed -= REFRESH_FREQ * 100.0f;
    }
    object->trans.z -= work->forwardSpeed;
    if (getRoundState() >= 1) {
        if ((work->nearbySpawnDistance -= work->forwardSpeed) < 0.0f) {
            work->nearbySpawnDistance = 4500.0f - (500.0f * ((frand() & 0x8F) / 128.0f));
            spawnPositionX = work->homePositionX + playerLanePositions[frand() & 7];
            FindPlayerEffectAtPosition(work->playerSide, spawnPositionX, object->trans.y - 100.0f, -6000.0f + object->trans.z);
        }
        if ((work->farSpawnDistance -= work->forwardSpeed) < 0.0f) {
            work->farSpawnDistance = 2000.0f;
            spawnPositionX = work->homePositionX + playerLanePositions[frand() & 7];
            if ((frand() & 1) != 0) {
                steeringRotation = 350.0f;
            }
            else {
                steeringRotation = 500.0f;
            }
            if (getRoundState() == 2) {
                FindSecondaryEffectAtPosition(work->playerSide, spawnPositionX, 50.0f + steeringRotation, -6000.0f + object->trans.z);
            }
        }
    }
    playerSideData->playerY = object->trans.x - playerSideData->playerX;
    playerSideData->playerX = object->trans.x;
    playerSideData->playerZ = object->trans.z;
    if (playerPairStateActive == 0) {
        playerSideData->previousPlayerZ = object->trans.z;
    }
    playerSideData->scrollSpeed = work->forwardSpeed;
    SetWaterCameraY(work->playerSide, object->trans.z);
    CenterM[work->playerSide].x = work->homePositionX + (0.5f * (object->trans.x - work->homePositionX));
    CenterM[work->playerSide].z = 950.0f + getWaterCameraX(work->playerSide);
    spawnPositionX = work->movementSpeed * sind(object->rot.y);
    spawnPositionX = spawnPositionX;
    forwardOffset = work->movementSpeed * cosd(object->rot.y);
    forwardOffset = forwardOffset;
    if (work->playerIndex == 0) {
        OSReport("X>%3.3f Z>%3.3f \n", spawnPositionX, forwardOffset);
    }
    Hu3DModelPosSet(object->mdlId[1], object->trans.x + spawnPositionX, object->trans.y, object->trans.z + forwardOffset);
    Hu3DModelRotSet(object->mdlId[1], object->rot.x, object->rot.y, object->rot.z);
    Hu3DModelPosSet(object->mdlId[2], object->trans.x + spawnPositionX, object->trans.y - 150.0f, object->trans.z + forwardOffset);
    Hu3DModelRotSet(object->mdlId[2], object->rot.x, 180.0f + object->rot.y, object->rot.z);
    transformedOffset.x = 0.0f;
    transformedOffset.y = 25.0f;
    transformedOffset.z = -100.0f;
    RotateVectorByMatrix(rotationMatrix, &transformedOffset);
    transformedOffset.x += object->trans.x;
    transformedOffset.y += object->trans.y;
    transformedOffset.z += playerSideData->previousPlayerZ;
    SetWaterHookPoint(work->playerSide, 0, transformedOffset.x, transformedOffset.y, transformedOffset.z);
    speedMagnitude = work->forwardSpeed / (2460.0f * REFRESH_FREQ);
    if (speedMagnitude < 0.0) {
        speedMagnitude *= -1.0f;
    }
    EmitSurfaceSplashParticles(work->playerSide, &transformedOffset, 180.0f + object->rot.y, speedMagnitude);
    velocity.x = work->horizontalVelocity;
    velocity.y = 0.0f;
    velocity.z = -work->forwardSpeed;
    transformedOffset.x = 0.0f;
    transformedOffset.y = 0.0f;
    transformedOffset.z = 160.0f;
    RotateVectorByMatrix(&rotationMatrix[0], &transformedOffset);
    transformedOffset.x += object->trans.x;
    transformedOffset.y += object->trans.y;
    transformedOffset.z += playerSideData->previousPlayerZ;
    EmitUnderwaterSplashParticles(work->playerSide, &transformedOffset, object->rot.y, speedMagnitude, &velocity);
}

s32 playerSideCameraIds[2] = { 1, 2 };
Vec playerWaterHookPath[7] = {
    { -300.0f, 30.000002f, -150.0f },
    { -150.0f, 30.000002f, 0.0f },
    { 150.0f, 30.000002f, 0.0f },
    { 300.0f, 30.000002f, -150.0f },
    { -400.0f, 0.0f, 250.0f },
    { 380.0f, 0.0f, 300.0f },
    { 0.0f, 1.0f, 0.0f },
};
Vec2f zeroTextureCoordinate = { 0.0f, 0.0f };
s32 playerAccentColor = 0xFFA040FF;

void FinishPlayerRendering(OMOBJ *object)
{
    Mtx rotationMatrix;
    Vec modelOffset;
    float followDistance;
    float verticalError;
    float targetHeight;
    float steeringInput;
    float targetRotation;
    float previousX;
    M430PlayerWork *work;
    M430PlayerPairState *playerSideData;
    s32 modelIndex;

    work = object->data;
    targetRotation = 180.0f;
    followDistance = 0.0f;
    targetHeight = 0.0f;
    playerSideData = &playerPairStates[work->playerSide];
    if ((getRoundState() == 2) && (work->isComputer != 0)) {
        UpdatePlayerDisplay(object);
    }
    if ((s32)work->playerState != 0) {
        (void)object;
    }
    else if ((getRoundState() == 2) && (SpawnSecondaryEffectAtPosition(work->playerSide, object->trans.x, 50.0f + object->trans.y, object->trans.z, 1) != -1)) {
        HuAudFXPlay(7);
        playerSideData->collectedCoins++;
    }
    if (fabs(steeringInput = work->stickX) < 8.0) {
        steeringInput = 0.0f;
    }
    work->horizontalVelocity *= 0.98f;
    work->horizontalVelocity += (0.1575f / REFRESH_RATE_F) * steeringInput;
    if (work->horizontalVelocity < -525.0f / REFRESH_RATE_F) {
        work->horizontalVelocity = -525.0f / REFRESH_RATE_F;
    }
    else if (work->horizontalVelocity > 525.0f / REFRESH_RATE_F) {
        work->horizontalVelocity = 525.0f / REFRESH_RATE_F;
    }
    previousX = object->trans.x;
    object->trans.x += work->horizontalVelocity;
    if (object->trans.x > (350.0f + work->homePositionX)) {
        object->trans.x = 350.0f + work->homePositionX;
    }
    else if (object->trans.x < (work->homePositionX - 350.0f)) {
        object->trans.x = work->homePositionX - 350.0f;
    }
    verticalError = fabs(0.0033333334f * (object->trans.y - 200.0f));
    followDistance = 245.0f + (-105.0f * verticalError);
    if (object->trans.x > (playerSideData->playerX + followDistance)) {
        object->trans.x = playerSideData->playerX + followDistance;
    }
    else if (object->trans.x < (playerSideData->playerX - followDistance)) {
        object->trans.x = playerSideData->playerX - followDistance;
    }
    targetRotation = 180.0f;
    work->rotationY = targetRotation - (30.0f * (REFRESH_RATE_F / 525.0f * (object->trans.x - previousX)));
    object->rot.y = ApproachAngle(work->rotationY, object->rot.y, 2.5f);
    if (fabs(GetShortestAngleDifference(object->rot.y, 180.0f)) < 90.0) {
        object->rot.z = 0.5f * (object->rot.y - 180.0f);
    }
    else {
        object->rot.z = 0.5f * object->rot.y;
    }
    object->trans.z = 300.0f + playerSideData->previousPlayerZ;
    if (playerSideData->scrollSpeed < 1125.0f / REFRESH_RATE_F) {
        targetHeight = 200.0f;
    }
    else if (playerSideData->scrollSpeed < 2000.0f * REFRESH_FREQ) {
        targetHeight = 350.0f;
    }
    else {
        targetHeight = 500.0f;
    }
    verticalError = work->positionY - targetHeight;
    if (fabs(verticalError) < REFRESH_FREQ * 100.0f) {
        work->positionY = targetHeight;
    }
    else if (verticalError > 0.0f) {
        work->positionY -= REFRESH_FREQ * 100.0f;
    }
    else if (verticalError < 0.0f) {
        work->positionY += REFRESH_FREQ * 100.0f;
    }
    work->stateFrame += (frand() & 3) + 1;
    object->trans.y = work->positionY + (0.10000000149011612 * (100.0 * sind(work->stateFrame)));
    mtxRot(&rotationMatrix[0], object->rot.x, object->rot.y, object->rot.z);
    rotationMatrix[1][0] *= object->scale.y;
    rotationMatrix[1][1] *= object->scale.y;
    rotationMatrix[1][2] *= object->scale.z;
    modelOffset.x = 0.0f;
    modelOffset.y = 150.0f;
    modelOffset.z = 50.0f;
    RotateVectorByMatrix(&rotationMatrix[0], &modelOffset);
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        Hu3DModelPosSet(object->mdlId[playerSideCameraIds[modelIndex]], object->trans.x + modelOffset.x, object->trans.y + modelOffset.y, object->trans.z + modelOffset.z);
        Hu3DModelRotSet(object->mdlId[playerSideCameraIds[modelIndex]], object->rot.x, object->rot.y, object->rot.z);
    }
    modelOffset.x = 0.0f;
    modelOffset.y = 150.0f;
    modelOffset.z = 50.0f;
    RotateVectorByMatrix(&rotationMatrix[0], &modelOffset);
    SetWaterHookPoint(work->playerSide, 1, object->trans.x + modelOffset.x, object->trans.y + modelOffset.y, object->trans.z + modelOffset.z);
    SetWaterHookProgress(work->playerSide, 1.0 - fabs((object->trans.x - playerSideData->playerX) / followDistance));
}

void UpdatePlayerSprites(OMOBJ *object)
{
    float modelRotationX;
    float modelRotationZ;
    M430PlayerWork *work;
    s32 formationModelIndex;
    M430PlayerPairState *playerSideData;
    s32 formationIndex;
    M430PlayerPairState *opposingSideData;

    work = object->data;
    playerSideData = &playerPairStates[work->playerSide];
    opposingSideData = &playerPairStates[work->playerSide ^ 1];
    switch (work->inputFlags) {
        case 0:
            formationIndex = work->playerMode + (work->playerSide * 2);
            object->trans.x = playerWaterHookPath[formationIndex].x;
            object->trans.y = playerWaterHookPath[formationIndex].y;
            object->trans.z = playerWaterHookPath[formationIndex].z + getWaterCameraX(0);
            object->rot.x = object->rot.y = object->rot.z = 0.0f;
            object->scale.x = object->scale.y = object->scale.z = 1.0f;
            CenterM[0].x = 0.0f;
            CenterM[0].z = 750.0f + object->trans.z;
            if (work->playerMode == 0) {
                work->motionIndex = 4;
            }
            else {
                work->motionIndex = 1;
            }
            CharMotionSet(work->characterId, object->mtnId[work->motionIndex]);
            Hu3DModelShadowSet(object->mdlId[0]);
            if (work->playerMode == 0) {
                Hu3DModelCameraSet(object->mdlId[1], 3);
                Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
                Hu3DModelCameraSet(object->mdlId[2], 3);
                Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
                work->rotationY = (180.0f + (40.0f * (0.007874016f * (frand() & 0x7F)))) - 20.0f;
                if (work->playerSide == 0) {
                    formationModelIndex = 4;
                }
                else {
                    formationModelIndex = 5;
                }
                Hu3DModelPosSet(object->mdlId[1], playerWaterHookPath[formationModelIndex].x, playerWaterHookPath[formationModelIndex].y, playerWaterHookPath[formationModelIndex].z + getWaterCameraX(0));
                Hu3DModelPosSet(
                    object->mdlId[2], playerWaterHookPath[formationModelIndex].x, playerWaterHookPath[formationModelIndex].y - 150.0f, playerWaterHookPath[formationModelIndex].z + getWaterCameraX(0));
                Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
                Hu3DModelShadowSet(object->mdlId[1]);
            }
            if (playerSideData->collectedCoins > 0) {
                work->playerState = 3;
            }
            else {
                work->playerState = 4;
            }
            GWPlayerCoinCollectAdd(work->playerIndex, playerSideData->collectedCoins);
            work->inputFlags++;
            break;
        case 1:
            if (getResultState() == 1) {
                if (work->playerIndex == 0) {
                    if ((playerSideData->collectedCoins == 0) && (opposingSideData->collectedCoins == 0)) {
                        HuAudSStreamPlay(4);
                    }
                    else {
                        HuAudSStreamPlay(1);
                    }
                }
                if (work->playerMode == 0) {
                    if (work->playerState == 3) {
                        work->motionIndex = 5;
                    }
                    else {
                        work->motionIndex = 6;
                    }
                }
                else if (work->playerState == 3) {
                    work->motionIndex = 2;
                }
                else {
                    work->motionIndex = 3;
                }
                CharMotionShiftSet(work->characterId, object->mtnId[work->motionIndex], 0.0f, 8.0f, 0);
                if (work->playerState == 3) {
                    CharFXPlay(work->characterId, 0x122);
                }
                work->stateFrame = 0;
                work->inputFlags++;
            }
            break;
        case 2:
            if (IsPlayerCleanupComplete(object) != 0) {
                SetPlayerActionFlag(work->playerIndex);
                work->inputFlags++;
            }
            break;
    }
    if (work->playerMode == 0) {
        modelRotationX = -10.0f;
        modelRotationZ = 0.0f;
        Hu3DModelRotSet(object->mdlId[1], modelRotationX, work->rotationY, modelRotationZ);
        Hu3DModelRotSet(object->mdlId[2], modelRotationX, work->rotationY, modelRotationZ);
    }
}

s32 IsPlayerCleanupComplete(OMOBJ *object)
{
    M430PlayerWork *work = object->data;
    s32 motionComplete = 0;
    if ((CharMotionEndCheck(work->characterId) != 0) && (CharMotionShiftIDGet(work->characterId) < 0)) {
        motionComplete = 1;
    }
    return motionComplete;
}

void CleanupPlayer(OMOBJ *object)
{
    Vec nearbyPlayerPosition;
    float unusedFloat30;
    float unusedFloat28;
    float unusedFloat20;
    float unusedFloat18;
    float unusedFloat10;
    float unusedFloat8;
    float targetX;
    float nearbyPlayerX;
    float targetPlayerX;
    float targetPlayerZ;
    float safeDistance;
    float alternatePlayerX;
    float nearbyPlayerZ;
    float unusedFloat24;
    float unusedFloat23;
    float unusedFloat22;
    float unusedFloat21;
    float unusedFloat20;
    float unusedFloat19;
    float unusedFloat18;
    M430PlayerWork *work;
    s32 targetPlayerIndex;
    s32 targetLifeCount;
    M430PlayerModelEffect *targetPlayerEffect;
    M430PlayerPairState *playerSideData;
    s32 nearbyPlayerIndex;
    s32 alternatePlayerIndex;

    work = object->data;
    targetLifeCount = 3;
    playerSideData = &playerPairStates[work->playerSide];
    if (playerSideData->playerFlags != 3) {
        work->difficulty = 3;
    }
    if (--work->randomMotionTimerFrames < 0) {
        work->randomMotionOffset = 150.0f * (3 - work->difficulty) * ((frand() & 0x7F) / 127.0f);
        if ((frand() & 1) != 0) {
            work->randomMotionOffset *= -1.0f;
        }
        work->randomMotionTimerFrames = 0x1E + (frand() % 30) + (4 - work->difficulty) * 0x3C;
    }
    switch (work->lifeCount) {
        case 1:
            safeDistance = 500.0f;
            break;
        case 2:
            safeDistance = 1500.0f;
            break;
        default:
            safeDistance = 3000.0f;
            break;
    }
    nearbyPlayerIndex = FindNearestPlayerEffect(work->playerSide, object->trans.z, &nearbyPlayerPosition);
    nearbyPlayerX = nearbyPlayerPosition.x;
    nearbyPlayerZ = nearbyPlayerPosition.z;
    targetPlayerIndex = FindNearestSecondaryEffect(work->playerSide, object->trans.z, &nearbyPlayerPosition);
    targetPlayerX = nearbyPlayerPosition.x;
    targetPlayerZ = nearbyPlayerPosition.z;
    if (targetPlayerIndex == work->otherPlayerIndex) {
        targetPlayerIndex = FindNearestSecondaryEffect(work->playerSide, 100.0f + targetPlayerZ, &nearbyPlayerPosition);
        targetPlayerX = nearbyPlayerPosition.x;
        targetPlayerZ = nearbyPlayerPosition.z;
    }
    else if (work->otherPlayerIndex == -1) {
        alternatePlayerIndex = FindNearestSecondaryEffect(work->playerSide, 100.0f + targetPlayerZ, &nearbyPlayerPosition);
        alternatePlayerX = nearbyPlayerPosition.x;
        if (work->difficulty >= 2) {
            if (fabs(targetPlayerX - object->trans.x) > 300.0) {
                if (fabs(alternatePlayerX - object->trans.x) < 300.0) {
                    if ((fabs(targetPlayerZ - object->trans.z) < 1800.0) && (work->lifeCount == 3)) {
                        work->otherPlayerIndex = targetPlayerIndex;
                        targetPlayerIndex = alternatePlayerIndex;
                        targetPlayerX = alternatePlayerX;
                    }
                }
            }
        }
    }
    else {
        work->otherPlayerIndex = -1;
    }
    if (fabs(nearbyPlayerZ - object->trans.z) < safeDistance) {
        if (nearbyPlayerX > object->trans.x) {
            targetX = nearbyPlayerX - 200.0f;
        }
        else {
            targetX = 200.0f + nearbyPlayerX;
        }
    }
    else {
        if ((fabs(nearbyPlayerX - targetPlayerX) > 200.0) || (nearbyPlayerIndex == -1)) {
            targetX = targetPlayerX;
        }
        else if ((nearbyPlayerX > targetPlayerX) || (nearbyPlayerX > (250.0f + work->homePositionX))) {
            targetX = nearbyPlayerX - 200.0f;
        }
        else {
            targetX = 200.0f + nearbyPlayerX;
        }
    }
    targetX += work->randomMotionOffset;
    if (targetX > (350.0f + work->homePositionX)) {
        targetX = 350.0f + work->homePositionX;
    }
    else if (targetX < (work->homePositionX - 350.0f)) {
        targetX = work->homePositionX - 350.0f;
    }
    if (targetPlayerIndex != -1) {
        targetX -= object->trans.x;
        if (targetX > 30.000002f) {
            work->stickX = 64.0f;
        }
        else if (targetX < -30.000002f) {
            work->stickX = -64.0f;
        }
        targetPlayerEffect = GetPlayerModelEffect(targetPlayerIndex);
        if (targetPlayerEffect != NULL) {
            if (targetPlayerEffect->position.y > 450.0f) {
                targetLifeCount = 3;
            }
            else {
                targetLifeCount = 2;
            }
        }
        if (targetLifeCount > work->lifeCount) {
            work->buttonsPressed |= 0x100;
            return;
        }
        if (targetLifeCount < work->lifeCount) {
            work->buttonsPressed |= 0x200;
        }
    }
    else if (targetLifeCount > work->lifeCount) {
        work->buttonsPressed |= 0x100;
    }
}

void UpdatePlayerDisplay(OMOBJ *object)
{
    Vec targetPlayerPosition;
    float targetX;
    float targetPlayerZ;
    float alternatePlayerX;
    M430PlayerWork *work;
    s32 targetPlayerIndex;
    M430PlayerPairState *playerSideData;
    s32 alternatePlayerIndex;

    work = object->data;
    playerSideData = &playerPairStates[work->playerSide];
    if (playerSideData->playerFlags != 3) {
        work->difficulty = 3;
    }
    if (--work->randomMotionTimerFrames < 0) {
        work->randomMotionOffset = 75.0f * (3 - work->difficulty) * ((frand() & 0x7F) / 127.0f);
        if ((frand() & 1) != 0) {
            work->randomMotionOffset *= -1.0f;
        }
        work->randomMotionTimerFrames = 0x1E + (frand() % 60) + (4 - work->difficulty) * 0x3C;
    }
    targetPlayerIndex = FindNearestSecondaryEffect(work->playerSide, object->trans.z, &targetPlayerPosition);
    targetX = targetPlayerPosition.x;
    targetPlayerZ = targetPlayerPosition.z;
    if (targetPlayerIndex == work->otherPlayerIndex) {
        targetPlayerIndex = FindNearestSecondaryEffect(work->playerSide, 100.0f + targetPlayerZ, &targetPlayerPosition);
        targetX = targetPlayerPosition.x;
        targetPlayerZ = targetPlayerPosition.z;
    }
    else if (work->otherPlayerIndex == -1) {
        alternatePlayerIndex = FindNearestSecondaryEffect(work->playerSide, 100.0f + targetPlayerZ, &targetPlayerPosition);
        alternatePlayerX = targetPlayerPosition.x;
        if (work->difficulty >= 2) {
            if (fabs(targetX - object->trans.x) > 300.0) {
                if (fabs(alternatePlayerX - object->trans.x) < 300.0) {
                    if ((fabs(targetPlayerZ - object->trans.z) < 1800.0) && (work->lifeCount == 3)) {
                        work->otherPlayerIndex = targetPlayerIndex;
                        targetPlayerIndex = alternatePlayerIndex;
                        targetX = alternatePlayerX;
                    }
                }
            }
        }
    }
    else {
        work->otherPlayerIndex = -1;
    }
    targetX += work->randomMotionOffset;
    if (targetX > (350.0f + work->homePositionX)) {
        targetX = 350.0f + work->homePositionX;
    }
    else if (targetX < (work->homePositionX - 350.0f)) {
        targetX = work->homePositionX - 350.0f;
    }
    if (targetPlayerIndex != -1) {
        targetX -= object->trans.x;
        if (targetX > 30.000002f) {
            work->stickX = 64.0f;
            return;
        }
        if (targetX < -30.000002f) {
            work->stickX = -64.0f;
        }
    }
}

void InitializePlayerWaterHooks(void)
{
    M430PlayerWaterHook *cameraHook;
    HU3DMODEL *hookModel;
    s32 vertexIndex;
    s32 cameraIndex;
    void *displayListSource;
    void *displayListBuildBuffer;

    cameraHook = playerWaterHooks = HuMemDirectMallocNum(HEAP_HEAP, 2 * sizeof(M430PlayerWaterHook), HU_MEMNUM_OVL);
    memset(cameraHook, 0, 2 * sizeof(M430PlayerWaterHook));
    for (cameraIndex = 0; cameraIndex < 2; cameraHook++) {
        cameraHook->cameraIndex = cameraIndex;
        cameraHook->hookModelId = Hu3DHookFuncCreate(DrawWaterHook);
        Hu3DModelLayerSet(cameraHook->hookModelId, 6);
        Hu3DModelCameraSet(cameraHook->hookModelId, 3);
        hookModel = &Hu3DData[cameraHook->hookModelId];
        hookModel->hookData = cameraHook;
        cameraHook->isActive = 1;
        cameraHook->interpolationProgress = 0.0f;
        cameraHook->startX = cameraHook->startY = cameraHook->startZ = 0.0f;
        cameraHook->currentX = cameraHook->currentY = cameraHook->currentZ = 500.0f;
        cameraHook->targetX = cameraHook->targetY = cameraHook->targetZ = 1000.0f;
        cameraHook->originX = cameraHook->originY = cameraHook->originZ = 0.0f;
        cameraHook->deltaX = cameraHook->deltaY = cameraHook->deltaZ = 1000.0f;
        cameraHook->displayListCapacity = 0;
        cameraHook->interpolationPoints = HuMemDirectMallocNum(HEAP_HEAP, 0xC0, HU_MEMNUM_OVL);
        memset(cameraHook->interpolationPoints, 0, 0xC0);
        cameraHook->renderPoints = HuMemDirectMallocNum(HEAP_HEAP, 0xC0, HU_MEMNUM_OVL);
        memset(cameraHook->renderPoints, 0, 0xC0);
        cameraHook->textureCoordinates = HuMemDirectMallocNum(HEAP_HEAP, 0x80, HU_MEMNUM_OVL);
        memset(cameraHook->textureCoordinates, 0, 0x80);
        hookModel = &Hu3DData[cameraHook->hookModelId];
        displayListSource = HuMemDirectMallocNum(HEAP_MODEL, 0x200, hookModel->mallocNo);
        displayListBuildBuffer = displayListSource;
        DCFlushRange(displayListSource, 0x200);
        GXBeginDisplayList(displayListBuildBuffer, 0x200);
        GXBegin(GX_LINESTRIP, GX_VTXFMT0, 0x10);
        for (vertexIndex = 0; vertexIndex < 0x10; vertexIndex++) {
            GXUnknownu16(vertexIndex);
            GXUnknownu16(0);
            GXUnknownu16(vertexIndex);
        }
        cameraHook->displayListLength = GXEndDisplayList();
        cameraHook->displayList = HuMemDirectMallocNum(HEAP_MODEL, cameraHook->displayListLength, hookModel->mallocNo);
        memcpy(cameraHook->displayList, displayListSource, cameraHook->displayListLength);
        DCFlushRange(cameraHook->displayList, cameraHook->displayListLength);
        HuMemDirectFree(displayListSource);
        UpdateWaterHookGeometry(cameraHook);
        cameraIndex++;
    }
}

void ReleasePlayerWaterHooks(void)
{
    float interpolationFraction;
    M430PlayerWaterHook *cameraHook;
    s32 pointIndex;
    s32 cameraIndex;

    for (cameraHook = playerWaterHooks, cameraIndex = 0; cameraIndex < 2; cameraIndex++, cameraHook++) {
        if (cameraHook->isActive != 0) {
            cameraHook->originX = cameraHook->originY = cameraHook->originZ = 0.0f;
            cameraHook->deltaX = cameraHook->targetX - cameraHook->startX;
            cameraHook->deltaY = cameraHook->targetY - cameraHook->startY;
            cameraHook->deltaZ = cameraHook->targetZ - cameraHook->startZ;
            interpolationFraction = 0.5f * (cameraHook->originX + cameraHook->deltaX);
            cameraHook->currentX = interpolationFraction + (cameraHook->interpolationProgress * (cameraHook->deltaX - interpolationFraction));
            interpolationFraction = 0.5f * (cameraHook->originY + cameraHook->deltaY);
            cameraHook->currentY = interpolationFraction + (cameraHook->interpolationProgress * (cameraHook->originY - interpolationFraction));
            interpolationFraction = 0.5f * (cameraHook->originZ + cameraHook->deltaZ);
            cameraHook->currentZ = interpolationFraction + (cameraHook->interpolationProgress * (cameraHook->originZ - interpolationFraction));
            for (pointIndex = 0; pointIndex < 0x10; pointIndex++) {
                interpolationFraction = pointIndex / 15.0f;
                if (interpolationFraction > 1.0f) {
                    interpolationFraction = 1.0f;
                }
                InterpolateBezierVector(interpolationFraction, &cameraHook->originX, &cameraHook->currentX, &cameraHook->deltaX, (float *)&cameraHook->interpolationPoints[pointIndex]);
            }
            UpdateWaterHookGeometry(cameraHook);
            Hu3DModelPosSet(cameraHook->hookModelId, cameraHook->startX, cameraHook->startY, cameraHook->startZ);
        }
    }
}

void EmptyWaterHookCallback(void) { }

void SetWaterHookPoint(s32 cameraIndex, s32 isTargetPosition, float positionX, float positionY, float positionZ)
{
    M430PlayerWaterHook *cameraHook = &playerWaterHooks[cameraIndex];
    if (isTargetPosition == 0) {
        cameraHook->startX = positionX;
        cameraHook->startY = positionY;
        cameraHook->startZ = positionZ;
        return;
    }
    cameraHook->targetX = positionX;
    cameraHook->targetY = positionY;
    cameraHook->targetZ = positionZ;
}

void SetWaterHookProgress(s32 cameraIndex, float interpolationProgress)
{
    M430PlayerWaterHook *cameraHook = &playerWaterHooks[cameraIndex];
    cameraHook->interpolationProgress = interpolationProgress;
}

void DisableWaterHook(s32 cameraIndex)
{
    M430PlayerWaterHook *cameraHook = &playerWaterHooks[cameraIndex];
    if (cameraHook->isActive != 0) {
        Hu3DModelAttrSet(cameraHook->hookModelId, HU3D_ATTR_DISPOFF);
    }
    cameraHook->isActive = 0;
}

void UpdateWaterHookGeometry(M430PlayerWaterHook *cameraHook)
{
    float segmentX;
    float segmentY;
    float segmentZ;
    float textureV;
    float segmentLength;
    s32 pointIndex;

    textureV = 0.0f;
    for (pointIndex = 0xF; pointIndex >= 0; pointIndex--) {
        cameraHook->renderPoints[pointIndex] = cameraHook->interpolationPoints[pointIndex];
        cameraHook->textureCoordinates[pointIndex] = zeroTextureCoordinate;
        if (pointIndex == 0) {
            textureV = 0.0f;
            cameraHook->textureCoordinates[pointIndex].y = textureV;
        }
        else {
            segmentX = cameraHook->interpolationPoints[pointIndex - 1].x - cameraHook->interpolationPoints[pointIndex].x;
            segmentY = cameraHook->interpolationPoints[pointIndex - 1].y - cameraHook->interpolationPoints[pointIndex].y;
            segmentZ = cameraHook->interpolationPoints[pointIndex - 1].z - cameraHook->interpolationPoints[pointIndex].z;
            segmentLength = sqrtf((segmentZ * segmentZ) + ((segmentX * segmentX) + (segmentY * segmentY)));
            textureV -= 0.01f * segmentLength;
            cameraHook->textureCoordinates[pointIndex].y = textureV;
        }
    }
    DCFlushRangeNoSync(cameraHook->renderPoints, 0xC0);
    DCFlushRangeNoSync(cameraHook->textureCoordinates, 0x80);
    PPCSync();
}

void DrawWaterHook(HU3DMODEL *hookModel, Mtx modelMatrix)
{
    Mtx cameraMatrix;
    M430PlayerWaterHook *cameraHook = hookModel->hookData;
    GXLoadPosMtxImm(modelMatrix, 0);
    MTXInvXpose(modelMatrix, cameraMatrix);
    GXLoadNrmMtxImm(cameraMatrix, 0);
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetNumChans(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, 0, 0x7D);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVREG0);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVREG0);
    GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_VTX, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_SPEC);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXSetZCompLoc(0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
    GXSetLineWidth(0x10, GX_TO_ZERO);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0);
    GXSetArray(GX_VA_POS, cameraHook->renderPoints, 0xC);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_TEX_ST, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, &playerAccentColor, 4);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_RGBA6, 0);
    GXSetArray(GX_VA_TEX0, cameraHook->textureCoordinates, 8);
    GXCallDisplayList(cameraHook->displayList, cameraHook->displayListLength);
}

void CreateWaterHooks(void)
{
    M430PlayerAnimatedEffect *effect;
    s32 effectIndex;

    effect = playerAnimatedEffects = HuMemDirectMallocNum(HEAP_HEAP, 0x10 * sizeof(M430PlayerAnimatedEffect), HU_MEMNUM_OVL);
    memset(effect, 0, 0x10 * sizeof(M430PlayerAnimatedEffect));
    for (effectIndex = 0; effectIndex < 0x10; effectIndex++, effect++) {
        effect->effectIndex = effectIndex;
        if (effectIndex == 0) {
            effect->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M430, 0x23));
        }
        else {
            effect->modelId = Hu3DModelLink(playerAnimatedEffects[0].modelId);
        }
        Hu3DModelLayerSet(effect->modelId, 2);
        Hu3DModelAttrSet(effect->modelId, HU3D_ATTR_DISPOFF);
        effect->particleModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M430, 0x24));
        Hu3DModelLayerSet(effect->particleModelId, 2);
        Hu3DModelAttrSet(effect->particleModelId, HU3D_ATTR_DISPOFF);
        effect->playerIndex = 0;
        effect->effectState = 0;
        effect->effectFrame = 0;
        effect->elapsedTime = 0.0f;
        effect->position.x = effect->position.y = effect->position.z = 0.0f;
        effect->rotX = effect->rotY = effect->rotZ = 0.0f;
        effect->scaleX = effect->scaleY = effect->scaleZ = 1.0f;
    }
}

void DestroyWaterHooks(void)
{
    M430PlayerAnimatedEffect *effect;
    s32 effectIndex;
    M430PlayerPairState *playerSideData;

    for (effect = playerAnimatedEffects, effectIndex = 0; effectIndex < 0x10; effectIndex++, effect++) {
        if (effect->effectState != 0) {
            playerSideData = &playerPairStates[effect->playerIndex];
            switch (effect->effectState) {
                case 1:
                    if (effect->position.y < 0.0f) {
                        effect->position.y += 1.0f;
                    }
                    break;
                case 2:
                    Hu3DModelAttrSet(effect->modelId, HU3D_ATTR_DISPOFF);
                    Hu3DModelAttrReset(effect->particleModelId, HU3D_ATTR_DISPOFF);
                    Hu3DMotionTimeSet(effect->particleModelId, 0.0f);
                    Hu3DMotionSpeedSet(effect->particleModelId, 1.0f);
                    effect->effectState = 3;
                    break;
                case 3:
                    break;
            }
            if (effect->position.z > (1000.0f + playerSideData->playerZ)) {
                Hu3DModelAttrSet(effect->modelId, HU3D_ATTR_DISPOFF);
                Hu3DModelAttrSet(effect->particleModelId, HU3D_ATTR_DISPOFF);
                effect->effectState = 0;
            }
            Hu3DModelPosSet(effect->modelId, effect->position.x, effect->position.y, effect->position.z);
            Hu3DModelRotSet(effect->modelId, effect->rotX, effect->rotY, effect->rotZ);
            Hu3DModelScaleSet(effect->modelId, effect->scaleX, effect->scaleY, effect->scaleZ);
            Hu3DModelPosSet(effect->particleModelId, effect->position.x, effect->position.y, effect->position.z);
            Hu3DModelRotSet(effect->particleModelId, effect->rotX, effect->rotY, effect->rotZ);
            Hu3DModelScaleSet(effect->particleModelId, effect->scaleX, effect->scaleY, effect->scaleZ);
        }
    }
}

void EmptyPlayerEffectCallback(void) { }

s32 FindPlayerEffectAtPosition(s32 playerIndex, float positionX, float positionY, float positionZ)
{
    M430PlayerAnimatedEffect *effect;
    s32 effectIndex;

    for (effect = playerAnimatedEffects, effectIndex = 0; effectIndex < 0x10; effectIndex++, effect++) {
        if (effect->effectState == 0) {
            break;
        }
    }
    if (effectIndex >= 0x10) {
        return -1;
    }
    if (playerIndex == 0) {
        Hu3DModelCameraSet(effect->modelId, 1);
        Hu3DModelCameraSet(effect->particleModelId, 1);
    }
    else {
        Hu3DModelCameraSet(effect->modelId, 2);
        Hu3DModelCameraSet(effect->particleModelId, 2);
    }
    Hu3DModelAttrReset(effect->modelId, HU3D_ATTR_DISPOFF);
    effect->playerIndex = playerIndex;
    effect->effectState = 1;
    effect->effectFrame = 0;
    effect->position.x = positionX;
    effect->position.y = positionY;
    effect->position.z = positionZ;
    effect->rotX = effect->rotY = effect->rotZ = 0.0f;
    effect->scaleX = effect->scaleY = effect->scaleZ = 1.0f;
    Hu3DModelPosSet(effect->modelId, effect->position.x, effect->position.y, effect->position.z);
    Hu3DModelRotSet(effect->modelId, effect->rotX, effect->rotY, effect->rotZ);
    Hu3DModelScaleSet(effect->modelId, effect->scaleX, effect->scaleY, effect->scaleZ);
}

s32 SpawnPlayerEffectAtPosition(s32 playerIndex, float positionX, float positionY, float positionZ, s32 consumeEffect)
{
#ifdef NON_MATCHING
#define BSS_16C_LEN 16
#else
#define BSS_16C_LEN 32
#endif
    float deltaX;
    float deltaZ;
    M430PlayerAnimatedEffect *effect;
    s32 effectIndex;

    for (effect = playerAnimatedEffects, effectIndex = 0; effectIndex < BSS_16C_LEN; effectIndex++, effect++) {
        if ((effect->effectState == 1) && (effect->playerIndex == playerIndex)) {
            deltaX = positionX - effect->position.x;
            deltaZ = positionZ - effect->position.z;
            if ((fabs(deltaX) < 50.0) && (fabs(deltaZ) < 50.0)) {
                break;
            }
        }
    }
    if (effectIndex >= BSS_16C_LEN) {
        return 0;
    }
    if (consumeEffect != 0) {
        effect->elapsedTime = 50.0f;
        effect->effectState = 2;
        effect->effectFrame = 0;
    }
    return 1;
#undef BSS_16C_LEN
}

s32 FindNearestPlayerEffect(s32 playerIndex, float maxPositionZ, Vec *nearestEffectPosition)
{
    float nearestDistance;
    float effectDistance;
    M430PlayerAnimatedEffect *effect;
    s32 effectIndex;
    s32 nearestEffectIndex;

    effect = playerAnimatedEffects;
    nearestEffectIndex = -1;
    nearestDistance = 10000.0f;
    nearestEffectPosition->x = nearestEffectPosition->y = nearestEffectPosition->z = 0.0f;
    for (effectIndex = 0; effectIndex < 0x10; effectIndex++, effect++) {
        if ((effect->effectState == 1) && (effect->playerIndex == playerIndex) && !(effect->position.z > maxPositionZ)) {
            effectDistance = fabs(effect->position.z - (maxPositionZ - 100.0f));
            if (effectDistance < nearestDistance) {
                nearestDistance = effectDistance;
                nearestEffectIndex = effectIndex;
            }
        }
    }
    if (nearestEffectIndex == -1) {
        return -1;
    }
    effect = &playerAnimatedEffects[nearestEffectIndex];
    *nearestEffectPosition = effect->position;
    return nearestEffectIndex;
}

void RemovePlayerEffect(s32 playerIndex)
{
    M430PlayerAnimatedEffect *effect;
    s32 effectIndex;

    for (effect = playerAnimatedEffects, effectIndex = 0; effectIndex < 0x10; effectIndex++, effect++) {
        if ((effect->effectState != 0) && (effect->playerIndex == playerIndex)) {
            effect->effectState = 0;
            Hu3DModelAttrSet(effect->modelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(effect->particleModelId, HU3D_ATTR_DISPOFF);
        }
    }
}

void InitializePlayerEffects(void)
{
    M430PlayerModelEffect *effect;
    s32 effectIndex;

    effect = playerModelEffects = HuMemDirectMallocNum(HEAP_HEAP, 0x20 * sizeof(M430PlayerModelEffect), HU_MEMNUM_OVL);
    memset(effect, 0, 0x20 * sizeof(M430PlayerModelEffect));
    for (effectIndex = 0; effectIndex < 0x20; effect++) {
        effect->effectIndex = effectIndex;
        if (effectIndex == 0) {
            effect->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M430, 0x22));
        }
        else {
            effect->modelId = Hu3DModelLink(playerModelEffects[0].modelId);
        }
        Hu3DModelLayerSet(effect->modelId, 7);
        Hu3DModelAttrSet(effect->modelId, HU3D_ATTR_DISPOFF);
        effect->playerIndex = 0;
        effect->effectState = 0;
        effect->effectFrame = 0;
        effect->position.x = effect->position.y = effect->position.z = 0.0f;
        effect->rotation.x = effect->rotation.y = effect->rotation.z = 0.0f;
        effect->scale.x = effect->scale.y = effect->scale.z = 1.0f;
        effectIndex++;
    }
}

void UpdatePlayerEffects(void)
{
    M430PlayerModelEffect *effect;
    s32 effectIndex;
    M430PlayerPairState *playerSideData;

    for (effect = playerModelEffects, effectIndex = 0; effectIndex < 0x20; effectIndex++, effect++) {
        if (effect->effectState != 0) {
            switch (effect->effectState) {
                case 1:
                    effect->rotation.y += 7.5;
                    break;
                case 2:
                    Hu3DModelAttrSet(effect->modelId, HU3D_ATTR_DISPOFF);
                    effect->effectState = 0;
                    break;
            }
            playerSideData = &playerPairStates[effect->playerIndex];
            if (effect->position.z > (1000.0f + playerSideData->playerZ)) {
                Hu3DModelAttrSet(effect->modelId, HU3D_ATTR_DISPOFF);
                effect->effectState = 0;
            }
            else {
                Hu3DModelPosSet(effect->modelId, effect->position.x, effect->position.y, effect->position.z);
                Hu3DModelRotSet(effect->modelId, effect->rotation.x, effect->rotation.y, effect->rotation.z);
                Hu3DModelScaleSet(effect->modelId, effect->scale.x, effect->scale.y, effect->scale.z);
            }
        }
    }
}

void EmptySecondaryEffectCallback(void) { }

s32 FindSecondaryEffectAtPosition(s32 playerIndex, float positionX, float positionY, float positionZ)
{
    M430PlayerModelEffect *effect;
    s32 effectIndex;

    for (effect = playerModelEffects, effectIndex = 0; effectIndex < 0x20; effectIndex++, effect++) {
        if (effect->effectState == 0) {
            break;
        }
    }
    if (effectIndex >= 0x20) {
        return -1;
    }
    if (playerIndex == 0) {
        Hu3DModelCameraSet(effect->modelId, 1);
    }
    else {
        Hu3DModelCameraSet(effect->modelId, 2);
    }
    Hu3DModelAttrReset(effect->modelId, HU3D_ATTR_DISPOFF);
    effect->playerIndex = playerIndex;
    effect->effectState = 1;
    effect->position.x = positionX;
    effect->position.y = positionY;
    effect->position.z = positionZ;
    effect->rotation.x = effect->rotation.y = effect->rotation.z = 0.0f;
    effect->scale.x = effect->scale.y = effect->scale.z = 1.0f;
    Hu3DModelPosSet(effect->modelId, effect->position.x, effect->position.y, effect->position.z);
    Hu3DModelRotSet(effect->modelId, effect->rotation.x, effect->rotation.y, effect->rotation.z);
    Hu3DModelScaleSet(effect->modelId, effect->scale.x, effect->scale.y, effect->scale.z);
}

s32 SpawnSecondaryEffectAtPosition(s32 playerIndex, float positionX, float positionY, float positionZ, s32 consumeEffect)
{
    float deltaX;
    float deltaZ;
    float deltaY;
    M430PlayerModelEffect *effect;
    s32 effectIndex;

    for (effect = playerModelEffects, effectIndex = 0; effectIndex < 0x20; effectIndex++, effect++) {
        if ((effect->effectState == 1) && (effect->playerIndex == playerIndex)) {
            deltaX = effect->position.x - positionX;
            deltaY = effect->position.y - positionY;
            deltaZ = effect->position.z - positionZ;
            if ((fabs(deltaY) < 100.0) && (sqrtf((deltaX * deltaX) + (deltaZ * deltaZ)) < 100.0f)) {
                break;
            }
        }
    }
    if (effectIndex >= 0x20) {
        return -1;
    }
    if (consumeEffect != 0) {
        FindPlayerSplashEffect(effect->playerIndex, effect->position.x, effect->position.y, effect->position.z);
        effect->effectState = 2;
        effect->effectFrame = 0;
    }
    return 1;
}

s32 FindNearestSecondaryEffect(s32 playerIndex, float maxPositionZ, Vec *nearestEffectPosition)
{
    float nearestDistance;
    float effectDistance;
    M430PlayerModelEffect *effect;
    s32 effectIndex;
    s32 nearestEffectIndex;

    effect = playerModelEffects;
    nearestEffectIndex = -1;
    nearestDistance = 10000.0f;
    nearestEffectPosition->x = nearestEffectPosition->y = nearestEffectPosition->z = 0.0f;
    for (effectIndex = 0; effectIndex < 0x20; effectIndex++, effect++) {
        if ((effect->effectState == 1) && (effect->playerIndex == playerIndex) && !(effect->position.z > maxPositionZ)) {
            effectDistance = fabs(effect->position.z - maxPositionZ);
            if (effectDistance < nearestDistance) {
                nearestDistance = effectDistance;
                nearestEffectIndex = effectIndex;
            }
        }
    }
    if (nearestEffectIndex == -1) {
        return -1;
    }
    effect = &playerModelEffects[nearestEffectIndex];
    *nearestEffectPosition = effect->position;
    return nearestEffectIndex;
}

M430PlayerModelEffect *GetPlayerModelEffect(s32 effectIndex)
{
    if ((effectIndex < 0) || (effectIndex >= 0x20)) {
        return NULL;
    }
    return &playerModelEffects[effectIndex];
}

void RemovePlayerModelEffect(s32 playerIndex)
{
    M430PlayerModelEffect *effect;
    s32 effectIndex;

    for (effect = playerModelEffects, effectIndex = 0; effectIndex < 0x20; effectIndex++, effect++) {
        if ((effect->effectState != 0) && (effect->playerIndex == playerIndex)) {
            effect->effectState = 0;
            Hu3DModelAttrSet(effect->modelId, HU3D_ATTR_DISPOFF);
        }
    }
}

void InitializePlayerModelEffects(void)
{
    M430PlayerParticleEffect *effect;
    s32 effectIndex;

    effect = playerParticleEffects = HuMemDirectMallocNum(HEAP_HEAP, 8 * sizeof(M430PlayerParticleEffect), HU_MEMNUM_OVL);
    memset(effect, 0, 8 * sizeof(M430PlayerParticleEffect));
    for (effectIndex = 0; effectIndex < 8; effectIndex++, effect++) {
        if (effectIndex == 0) {
            effect->particleAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M430, 0x26), HU_MEMNUM_OVL));
        }
        else {
            effect->particleAnimation = playerParticleEffects[0].particleAnimation;
        }
        effect->effectIndex = effectIndex;
        effect->particleModelId = Hu3DParticleCreate(effect[0].particleAnimation, 0x20);
        Hu3DModelLayerSet(effect->particleModelId, 7);
        Hu3DModelAttrSet(effect->particleModelId, HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(effect->particleModelId, HU3D_ATTR_PARTICLE_KILL);
        effect->cameraIndex = 0;
        effect->effectState = 0;
        effect->effectFrame = 0;

        effect->position.x = effect->position.y = effect->position.z = 0.0f;
        effect->rotation.x = effect->rotation.y = effect->rotation.z = 0.0f;
        effect->scale.x = effect->scale.y = effect->scale.z = 1.0f;
    }
}

void UpdatePlayerModelEffects(void)
{
    M430PlayerParticleEffect *effect;
    HU3DPARTICLEDATA *particle;
    HU3DPARTICLE *particleSystem;
    s32 effectIndex;
    s32 particleIndex;
    M430PlayerPairState *playerSideData;
    HU3DMODEL *particleModel;
    s32 particleAlpha;

    for (effect = playerParticleEffects, effectIndex = 0; effectIndex < 8; effectIndex++, effect++) {
        if (effect->effectState != 0) {
            if (effect->effectFrame > 1) {
                particleModel = &Hu3DData[effect->particleModelId];
                particleSystem = particleModel->hookData;
                for (particle = particleSystem->data, particleIndex = 0; particleIndex < particleSystem->maxCnt; particleIndex++, particle++) {
                    if (++particle->time > particle->parManId) {
                        particleAlpha = particle->color.a - 9;
                        if (particleAlpha < 0) {
                            particle->scale = 0.0f;
                        }
                        else {
                            particle->color.a = particleAlpha;
                        }
                    }
                    particle->scale *= 0.975;
                    particle->pos.x += particle->vel.x;
                    particle->pos.y += particle->vel.y;
                    particle->pos.z += particle->vel.z;
                    particle->vel.y -= 0.3103333379576603;
                }
            }
            playerSideData = &playerPairStates[effect->cameraIndex];
            effect->position.z -= playerSideData->scrollSpeed;
            if (++effect->effectFrame > 0x78) {
                Hu3DModelAttrSet(effect->particleModelId, HU3D_ATTR_DISPOFF);
                effect->effectState = 0;
            }
            else {
                Hu3DModelPosSet(effect->particleModelId, effect->position.x, effect->position.y, effect->position.z);
                Hu3DModelRotSet(effect->particleModelId, effect->rotation.x, effect->rotation.y, effect->rotation.z);
                Hu3DModelScaleSet(effect->particleModelId, effect->scale.x, effect->scale.y, effect->scale.z);
            }
        }
    }
}

void InitializePlayerSplashEffects(void)
{
    ReleasePlayerAnimation(playerParticleEffects[0].particleAnimation);
}

s32 FindPlayerSplashEffect(s32 cameraIndex, float positionX, float positionY, float positionZ)
{
    float particleElevation;
    float particleAngle;
    float particleAzimuth;
    float directionZ;
    float directionY;
    float particleScale;
    M430PlayerParticleEffect *effect;
    HU3DPARTICLEDATA *particle;
    s32 effectIndex;
    HU3DPARTICLE *particleSystem;
    HU3DMODEL *particleModel;

    for (effect = playerParticleEffects, effectIndex = 0; effectIndex < 8; effectIndex++, effect++) {
        if (effect->effectState == 0) {
            break;
        }
    }
    if (effectIndex >= 8) {
        return -1;
    }
    if (cameraIndex == 0) {
        Hu3DModelCameraSet(effect->particleModelId, 1);
    }
    else {
        Hu3DModelCameraSet(effect->particleModelId, 2);
    }
    Hu3DModelAttrReset(effect->particleModelId, HU3D_ATTR_DISPOFF);
    effect->cameraIndex = cameraIndex;
    effect->effectState = 1;
    effect->effectFrame = 0;
    particleModel = &Hu3DData[effect->particleModelId];
    particleSystem = particleModel->hookData;
    particleSystem->blendMode = 1;
    for (particle = particleSystem->data, effectIndex = 0; effectIndex < particleSystem->maxCnt; effectIndex++, particle++) {
        particleAngle = (frand() % 30) + 0x4B;
        particleAzimuth = frand() % 360;
        directionY = sind(particleAngle);
        directionZ = cosd(particleAngle);
        particleScale = 0.8f + (0.3f * (0.007874016f * (frand() % 255)));
        particleElevation = 0.35f;
        particle->vel.x = particleElevation * (0.6499999761581421 * (50.0 * (directionZ * sind(particleAzimuth))));
        particle->vel.y = particleElevation * (50.0f * directionY * particleScale);
        particle->vel.z = particleElevation * (0.6499999761581421 * (50.0 * (directionZ * cosd(particleAzimuth))));
        particle->time = 0;
        particle->parManId = (frand() % 7) + 0xA;
        particle->scale = 45.0f;
        particle->pos.x = 0.25f * ((frand() & 0x7F) - 0x40);
        particle->pos.y = 0.25f * ((frand() & 0x7F) - 0x40);
        particle->pos.z = 0.25f * ((frand() & 0x7F) - 0x40);
        particle->color.r = (frand() % 64) + 0xC0;
        particle->color.g = (frand() % 64) + 0xC0;
        particle->color.b = (frand() % 64) + 0xC0;
        particle->color.a = 0xFF;
    }
    effect->position.x = positionX;
    effect->position.y = positionY;
    effect->position.z = positionZ;
    effect->rotation.x = effect->rotation.y = effect->rotation.z = 0.0f;
    effect->scale.x = effect->scale.y = effect->scale.z = 1.0f;
    Hu3DModelPosSet(effect->particleModelId, effect->position.x, effect->position.y, effect->position.z);
    Hu3DModelRotSet(effect->particleModelId, effect->rotation.x, effect->rotation.y, effect->rotation.z);
    Hu3DModelScaleSet(effect->particleModelId, effect->scale.x, effect->scale.y, effect->scale.z);
}

void RemovePlayerSplashEffect(s32 playerIndex)
{
    M430PlayerParticleEffect *effect;
    s32 effectIndex;

    for (effect = playerParticleEffects, effectIndex = 0; effectIndex < 8; effectIndex++, effect++) {
        if (effect->effectState != 0) {
            effect->effectState = 0;
            Hu3DModelAttrSet(effect->particleModelId, HU3D_ATTR_DISPOFF);
        }
    }
}

void UpdatePlayerSplashEffects(void)
{
    M430PlayerSpriteLayout *playerDisplay;
    s32 spriteIndex;
    s32 playerSide;

    playerDisplay = HuMemDirectMallocNum(HEAP_HEAP, 2 * sizeof(M430PlayerSpriteLayout), HU_MEMNUM_OVL);
    playerSpriteLayouts = playerDisplay;
    for (playerSide = 0; playerSide < 2; playerSide++, playerDisplay++) {
        if (playerSide == 0) {
            playerDisplay->spriteOriginX = 0x90;
        }
        else {
            playerDisplay->spriteOriginX = 0x1B0;
        }
        playerDisplay->spriteOriginY = 0x198;
        playerDisplay->spriteOffsetX = 0;
        playerDisplay->spriteOffsetY = 0;
        playerDisplay->spriteCount = 0x15;
        playerDisplay->spriteIndex = 0;
        for (spriteIndex = 0; spriteIndex < 6; spriteIndex++) {
            switch (spriteIndex) {
                case 0:
                case 1:
                    playerDisplay->spriteIds[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x33), 1, 0);
                    espPriSet(playerDisplay->spriteIds[spriteIndex], 0x40);
                    playerDisplay->spritePositions[spriteIndex][0] = (spriteIndex * 0x14) + 0xB;
                    playerDisplay->spritePositions[spriteIndex][1] = 0;
                    playerDisplay->spriteTargetPositions[spriteIndex][0] = playerDisplay->spritePositions[spriteIndex][0] - playerDisplay->spriteCount;
                    playerDisplay->spriteTargetPositions[spriteIndex][1] = playerDisplay->spritePositions[spriteIndex][1] - playerDisplay->spriteIndex;
                    break;
                case 2:
                    if (playerSide == 0) {
                        playerDisplay->spriteIds[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x1B), 1, 0);
                    }
                    else {
                        playerDisplay->spriteIds[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x1C), 1, 0);
                    }
                    espPriSet(playerDisplay->spriteIds[spriteIndex], 0xFF);
                    playerDisplay->spritePositions[spriteIndex][0] = 0;
                    playerDisplay->spritePositions[spriteIndex][1] = 0;
                    break;
                case 3:
                    playerDisplay->spriteIds[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x1D), 1, 0);
                    espPriSet(playerDisplay->spriteIds[spriteIndex], 0x100);
                    espTPLvlSet(playerDisplay->spriteIds[spriteIndex], 0.6000000238418579);
                    espColorSet(playerDisplay->spriteIds[spriteIndex], 0, 0, 0);
                    playerDisplay->spritePositions[spriteIndex][0] = 0;
                    playerDisplay->spritePositions[spriteIndex][1] = 0;
                    break;
                case 4:
                    playerDisplay->spriteIds[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x08), 1, 0);
                    espPriSet(playerDisplay->spriteIds[spriteIndex], 0x80);
                    playerDisplay->spritePositions[spriteIndex][0] = -0x21;
                    playerDisplay->spritePositions[spriteIndex][1] = 0;
                    break;
                case 5:
                    playerDisplay->spriteIds[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_M430, 0x28), 1, 0);
                    espPriSet(playerDisplay->spriteIds[spriteIndex], 0x80);
                    espZRotSet(playerDisplay->spriteIds[spriteIndex], 45.0);
                    playerDisplay->spritePositions[spriteIndex][0] = -0xA;
                    playerDisplay->spritePositions[spriteIndex][1] = 0;
                    break;
            }
            espDispOn(playerDisplay->spriteIds[spriteIndex]);
            espDrawNoSet(playerDisplay->spriteIds[spriteIndex], 0);
            espAttrSet(playerDisplay->spriteIds[spriteIndex], HUSPR_ATTR_NOANIM);
            espPosSet(playerDisplay->spriteIds[spriteIndex], playerDisplay->spriteOriginX + playerDisplay->spritePositions[spriteIndex][0], playerDisplay->spriteOriginY + playerDisplay->spritePositions[spriteIndex][1]);
            espBankSet(playerDisplay->spriteIds[spriteIndex], 0);
        }
    }
    playerDisplayEnabled = 1;
    SetPlayerSpritesVisible(0);
}

void InitializePlayerSprites(void)
{
#ifdef NON_MATCHING
    s32 coinDigits[6];
#else
    s32 coinDigits[4];
#endif
    float coinScale;
    float spriteX;
    float spriteY;
    M430PlayerSpriteLayout *playerDisplay;
    s32 spriteIndex;
    s32 coinCount;
    s32 playerSide;

    for (playerDisplay = playerSpriteLayouts, playerSide = 0; playerSide < 2; playerSide++, playerDisplay++) {
        coinCount = playerPairStates[playerSide].collectedCoins;
        if (playerDisplayEnabled != 0) {
            if (coinCount != playerDisplay->spriteOffsetX) {
                playerDisplay->spriteOffsetX = coinCount;
                playerDisplay->spriteOffsetY = 0x14;
            }
            if (playerDisplay->spriteOffsetY != 0) {
                coinScale = 1.0 + (0.800000011920929 * sind((180.0f * (playerDisplay->spriteOffsetY / 20.0f))));
                playerDisplay->spriteOffsetY--;
            }
            else {
                coinScale = 1.0f;
            }
            if ((coinDigits[0] = coinCount / 10) > 9) {
                coinDigits[0] = 9;
            }
            coinCount -= coinDigits[0] * 0xA;
            coinDigits[1] = coinCount;
            if (coinCount > 9) {
                coinDigits[1] = 9;
            }
            for (spriteIndex = 0; spriteIndex < 6; spriteIndex++) {
                if (spriteIndex < 2) {
                    spriteX = (playerDisplay->spriteOriginX + playerDisplay->spriteCount) + (playerDisplay->spriteTargetPositions[spriteIndex][0] * coinScale);
                    spriteY = (playerDisplay->spriteOriginY + playerDisplay->spriteIndex) + (playerDisplay->spriteTargetPositions[spriteIndex][1] * coinScale);
                    espBankSet(playerDisplay->spriteIds[spriteIndex], coinDigits[spriteIndex]);
                    espScaleSet(playerDisplay->spriteIds[spriteIndex], coinScale, coinScale);
                }
                else {
                    spriteX = playerDisplay->spriteOriginX + playerDisplay->spritePositions[spriteIndex][0];
                    spriteY = playerDisplay->spriteOriginY + playerDisplay->spritePositions[spriteIndex][1];
                }
                if (coinDigits[spriteIndex] < 0) {
                    coinDigits[spriteIndex] = 0;
                }
                espPosSet(playerDisplay->spriteIds[spriteIndex], spriteX, spriteY);
            }
        }
    }
}

void ReleasePlayerSprites(void)
{
    M430PlayerSpriteLayout *playerDisplay;
    s32 spriteIndex;
    s32 playerSide;

    for (playerDisplay = playerSpriteLayouts, playerSide = 0; playerSide < 2; playerSide++, playerDisplay++) {
        for (spriteIndex = 0; spriteIndex < 6; spriteIndex++) {
            espKill(playerDisplay->spriteIds[spriteIndex]);
        }
    }
}

void SetPlayerSpritesVisible(s32 isVisible)
{
    M430PlayerSpriteLayout *playerDisplay;
    s32 spriteIndex;
    s32 playerSide;

    playerDisplay = playerSpriteLayouts;
    playerDisplayEnabled = isVisible;
    for (playerSide = 0; playerSide < 2; playerSide++, playerDisplay++) {
        for (spriteIndex = 0; spriteIndex < 6; spriteIndex++) {
            if (isVisible != 0) {
                espAttrReset(playerDisplay->spriteIds[spriteIndex], HUSPR_ATTR_DISPOFF);
            }
            else {
                espAttrSet(playerDisplay->spriteIds[spriteIndex], HUSPR_ATTR_DISPOFF);
            }
        }
    }
}

float InterpolateBezierValue(float progress, float startValue, float controlValue, float endValue)
{
    float remainingProgress;
    float interpolatedValue;

    remainingProgress = 1.0 - progress;
    interpolatedValue = (endValue * (progress * progress)) + ((startValue * (remainingProgress * remainingProgress)) + (2.0 * (controlValue * (remainingProgress * progress))));
    return interpolatedValue;
}

void InterpolateBezierVector(float progress, float *startValues, float *controlValues, float *endValues, float *outputValues)
{
    s32 componentIndex;

    for (componentIndex = 0; componentIndex < 3; componentIndex++) {
        *outputValues++ = InterpolateBezierValue(progress, *startValues++, *controlValues++, *endValues++);
    }
}

float GetShortestAngleDifference(float angleA, float angleB)
{
    float angleDifference;

    if (angleA >= 360.0f) {
        angleA -= 360.0f;
    }
    else if (angleA < 0.0f) {
        angleA += 360.0f;
    }
    if (angleB >= 360.0f) {
        angleB -= 360.0f;
    }
    else if (angleB < 0.0f) {
        angleB += 360.0f;
    }
    angleDifference = angleA - angleB;
    if (angleDifference <= -180.0f) {
        angleDifference += 360.0f;
    }
    else if (angleDifference >= 180.0f) {
        angleDifference -= 360.0f;
    }
    return angleDifference;
}

float ApproachAngle(float targetAngle, float currentAngle, float maxStep)
{
    float angleStep;
    float angleDifference;
    float unusedAngle;

    if (targetAngle >= 360.0) {
        targetAngle -= 360.0;
    }
    else if (targetAngle < 0.0) {
        targetAngle += 360.0;
    }
    if (currentAngle >= 360.0) {
        currentAngle -= 360.0;
    }
    else if (currentAngle < 0.0) {
        currentAngle += 360.0;
    }
    angleDifference = 360.0 + (targetAngle - currentAngle);
    if (fabs(angleDifference) >= 360.0) {
        angleDifference = fmod(angleDifference, 360.0);
    }
    if (angleDifference < 180.0) {
        if (angleDifference <= maxStep) {
            angleStep = angleDifference;
        }
        else {
            angleStep = maxStep;
        }
    }
    else if ((360.0 - angleDifference) <= maxStep) {
        angleStep = -(360.0 - angleDifference);
    }
    else {
        angleStep = -maxStep;
    }
    angleStep += currentAngle;
    if (angleStep >= 360.0) {
        angleStep -= 360.0;
    }
    else if (angleStep < 0.0) {
        angleStep += 360.0;
    }
    return angleStep;
}

void ReleasePlayerAnimation(ANIMDATA *animation)
{
    animation->useNum = 0;
    HuSprAnimKill(animation);
}

void RotateVectorByMatrix(Mtx rotationMatrix, Vec *vector)
{
    Vec rotatedVector;

    rotatedVector.x = (rotationMatrix[0][2] * vector->z) + ((rotationMatrix[0][0] * vector->x) + (rotationMatrix[0][1] * vector->y));
    rotatedVector.y = (rotationMatrix[1][2] * vector->z) + ((rotationMatrix[1][0] * vector->x) + (rotationMatrix[1][1] * vector->y));
    rotatedVector.z = (rotationMatrix[2][2] * vector->z) + ((rotationMatrix[2][0] * vector->x) + (rotationMatrix[2][1] * vector->y));
    *vector = rotatedVector;
}

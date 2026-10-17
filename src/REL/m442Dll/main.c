#include "game/disp.h"
#include "version.h"
#define HUSPR_USE_OLD_DEFS
#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework.h"

#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "REL/m442Dll.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/frand.h"
#endif

typedef struct M442BackgroundParticle {
    /* 0x00 */ Float x; // probably Vec
    /* 0x04 */ Float y;
    /* 0x08 */ Float z;
    /* 0x0C */ Float originX;
    /* 0x10 */ Float riseSpeed;
    /* 0x14 */ char unreferencedParticlePadding[4];
    /* 0x18 */ s16 remainingFrames;
    /* 0x1A */ s16 initialFrames;
    /* 0x1C */ Float opacity;
    /* 0x20 */ Float angle;
    /* 0x24 */ Float angularSpeed;
    /* 0x28 */ Float horizontalSpeed;
    /* 0x2C */ Float particleScale;
} M442BackgroundParticle; /* size = 0x30 */

typedef struct M442FramebufferTexture {
    /* 0x00 */ void *textureBuffer;
    /* 0x04 */ u16 sourceWidth;
    /* 0x06 */ u16 sourceHeight;
    /* 0x08 */ u16 textureWidth;
    /* 0x0A */ u16 textureHeight;
    /* 0x0C */ u32 textureBufferSize;
    /* 0x10 */ s32 copyFlags;
} M442FramebufferTexture; /* size = 0x14 */

typedef struct M442NavigationCell {
    /* 0x00 */ Vec worldPosition;
    /* 0x0C */ u8 pathStepCount;
    /* 0x0E */ s16 unusedGridPadding;
    /* 0x10 */ Float obstacleWeight;
} M442NavigationCell; /* size = 0x14 */

typedef struct M442CollectibleRecord {
    /* 0x00 */ Vec worldPosition;
    /* 0x0C */ u8 collectedCount;
    /* 0x0E */ s16 respawnTimer;
    /* 0x10 */ OMOBJ *ownerObject;
} M442CollectibleRecord; /* size = 0x14 */

typedef struct M442PlayerWork {
    /* 0x00 */ struct {
        u8 isActive : 1;
        u8 isInitialized : 1;
        u8 needsAction : 1;
        u8 isCelebrating : 1;
    };
    /* 0x02 */ s16 playerIndex;
    /* 0x04 */ s16 controllerPort;
    /* 0x06 */ s8 stickX;
    /* 0x07 */ s8 stickY;
    /* 0x08 */ u16 buttonsPressed;
    /* 0x0A */ u16 buttonsDown;
    /* 0x0C */ s32 regionMode;
    /* 0x10 */ s16 characterId;
    /* 0x12 */ s16 difficulty;
    /* 0x14 */ s16 playerGroup;
    /* 0x16 */ s16 playerState;
    /* 0x18 */ Float facingAngle;
    /* 0x1C */ Float turnRate;
    /* 0x20 */ char unreferencedPlayerPadding[4];
    /* 0x24 */ Vec movementVelocity;
    /* 0x30 */ s32 motionState;
    /* 0x34 */ s32 overlayMotion;
    /* 0x38 */ Float movementSpeed;
    /* 0x3C */ s16 animationTimer;
    /* 0x3E */ s16 actionTimer;
    /* 0x40 */ s16 targetIndex;
    /* 0x42 */ s16 actionState;
    /* 0x44 */ Float actionProgress;
    /* 0x48 */ s16 collectibleIndex;
    /* 0x4A */ char unreferencedTargetPadding[2];
    /* 0x4C */ Float targetX;
    /* 0x50 */ Float targetY;
    /* 0x54 */ Float targetZ;
    /* 0x58 */ Float targetDistance;
    /* 0x5C */ s32 targetRouteIndex;
    /* 0x60 */ char unreferencedPlayerWorkPadding[0xC];
    /* 0x6C */ s16 actionRequested;
    /* 0x70 */ Vec targetPosition;
    /* 0x7C */ s32 targetWaitTimer;
} M442PlayerWork; /* size = 0x80 */

typedef struct M442CameraSettings {
    /* 0x00 */ Float cameraZoom;
    /* 0x04 */ Float centerX;
    /* 0x08 */ Float centerY;
    /* 0x0C */ Float centerZ;
    /* 0x10 */ Float rotationX;
    /* 0x14 */ Float rotationY;
    /* 0x18 */ Float rotationZ;
} M442CameraSettings; /* size = 0x1C */

typedef struct M442CollectibleWork {
    /* 0x00 */ struct {
        u8 isActive : 1;
    };
    /* 0x02 */ s16 animationState;
    /* 0x04 */ s16 gridColumn;
    /* 0x06 */ s16 gridRow;
    /* 0x08 */ Float baseHeight;
    /* 0x0C */ s16 animationTimer;
    /* 0x0E */ s16 animationDuration;
    /* 0x10 */ s32 soundHandle;
    /* 0x14 */ s16 itemType;
    /* 0x16 */ s16 spawnIndex;
    /* 0x18 */ Float verticalScale;
} M442CollectibleWork; /* size = 0x1C */

void ExitMinigame(OMOBJ *object);
void InitializePlayerObjects(HUPROCESS *process);
void FreePlayerObjects(void);
s32 ArePlayersInitialized(void);
void CreatePlayerObject(OMOBJ *object);
void UpdatePlayerObject(OMOBJ *object);
s16 FindFacingCollectible(OMOBJ *object);
void ClampPlayerToArena(OMOBJ *object);
void SeparatePlayersFromGroup(OMOBJ *object);
void SeparatePlayerFromHuman(OMOBJ *object);
void InitializeMinigameState(OMOBJ *object);
s32 IsInteractionPathClear(OMOBJ *object, Vec *targetPosition);
void BeginPlayerFinish(OMOBJ *object);
void UpdatePlayerFinish(OMOBJ *object);
void FreePlayerObject(OMOBJ *object);
void InitializeArenaObjects(HUPROCESS *process);
void NoopArenaCallback(void);
void CreateArenaObject(OMOBJ *object);
void NoopArenaObjectCallback(OMOBJ *object);
void ResetBackgroundParticle(s32 particleIndex);
void UpdateMinigameSequence(OMOBJ *object);
void DrawBackgroundParticles(HU3DMODEL *model, Mtx mtx);
void DrawArenaOverlay(HU3DMODEL *model, Mtx mtx);
s16 CreateCollectibleRecord(Vec *position);
void DrawCollectibles(HU3DMODEL *model, Mtx mtx);
void DrawPlayerIndicator(HU3DMODEL *model, Mtx mtx);
void UpdateArenaParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void InitializeCollectibleObjects(HUPROCESS *process);
void FreeCollectibleObjects(void);
void CreateCollectibleObject(OMOBJ *object);
void UpdateCollectibleObject(OMOBJ *object);
s32 FindNearbyCollectibleObject(Vec *position);
void CollectObjectiveItem(s32 itemIndex, s32 collectibleIndex);
void FreeCollectibleObject(OMOBJ *object);
void UpdateCollectibleParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void UpdateCollectibleParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void CopyTextureFromFramebuffer(M442FramebufferTexture *texture, u8 clearFramebuffer);
Float InterpolateAngle(Float currentAngle, Float targetAngle, Float interpolationFactor);
void InitializeCamera(OMOBJ *object);
void InterpolateCameraSettings(s32 startIndex, s32 endIndex, Float interpolationFactor);
void UpdateCamera(OMOBJ *object);
s32 CheckPlayerRumbleStatus(void);

M442CameraSettings cameraSettings[3] = {
    { 500.0f, 0.0f, 500.0f, 350.0f, 84.0f, 0.0f, 0.0f },
    { 600.0f, 0.0f, 350.0f, 400.0f, -30.0f, 0.0f, 0.0f },
    { 3300.0f, 0.0f, 150.0f, 150.0f, -37.0f, 0.0f, 0.0f },
};

Vec keyLightPosition = { 700.0f, 2500.0f, -700.0f };

Vec worldOrigin = { 0.0f, 0.0f, 0.0f };

GXColor keyLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };

Vec shadowCameraPosition = { 200.0f, 15000.0f, -200.0f };

Vec shadowCameraUp = { 0.0f, 1.0f, 0.0f };

s32 unusedSequenceCounter;
OMOBJ *sequenceObject;
OMOBJ *cameraObject;
OMOBJ *arenaObject;
s32 unusedPlayerState;
u32 minigameState;
s32 playerObjectStates[1];
s32 playerObjectIds[1];
s32 sequenceFrameTimer;
s32 nextPlayerIndex;
s16 countdownValue;
s16 countdownFrameTimer;
s16 winCoinCounter;
s16 winningPlayerCount;
s16 winningPlayerIndices[1];
ANIMDATA *backgroundParticleTexture;
ANIMDATA *collectibleTextures[3];
ANIMDATA *playerParticleAnimation;
ANIMDATA *collectibleParticleAnimation;
ANIMDATA *collectibleAnimationData[4];
s16 collectedScore;
s32 remainingCollectibleCount;
s32 collectibleSpawnIndex;
u8 collectibleSpawnGrid[0x271];
char unusedGridBuffer[0x271];
s32 collectibleRecordCount;
M442CollectibleRecord collectibleRecords[0x400];
M442NavigationCell navigationGrid[11][11];
M442FramebufferTexture framebufferTextures[2];
M442BackgroundParticle backgroundParticles[0x400];
Float collectibleAnimationPhases[0xF];
#if VERSION_PAL
s32 palRumbleCheck;
#endif
Vec keyLightDirection;
s16 sequenceId;
s16 resultsSequenceId;
char alignmentPadding[0x10];

void ObjectSetup(void)
{
    HUPROCESS *playerSlot;
    HU3DCAMERA *playerObjects;
    s16 playerWork;

    playerObjects = Hu3DCamera;
    OSReport("******* M442ObjectSetup *********\n");
    playerSlot = omInitObjMan(0x32, 0x2000);
    omGameSysInit(playerSlot);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 30.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    arenaObject = omAddObjEx(playerSlot, 0x7FDA, 0, 0, -1, omOutView);
    keyLightDirection.x = worldOrigin.x - keyLightPosition.x;
    keyLightDirection.y = worldOrigin.y - keyLightPosition.y;
    keyLightDirection.z = worldOrigin.z - keyLightPosition.z;
    VECNormalize(&keyLightDirection, &keyLightDirection);
    playerWork = Hu3DGLightCreateV(&keyLightPosition, &keyLightDirection, &keyLightColor);
    Hu3DGLightInfinitytSet(playerWork);
    Hu3DGLightStaticSet(playerWork, 1);
    Hu3DShadowCreate(10.0f, playerObjects->near, playerObjects->far);
    Hu3DShadowTPLvlSet(0.625f);
    Hu3DShadowPosSet(&shadowCameraPosition, &shadowCameraUp, &worldOrigin);
    HuAudSndGrpSet(0x44);
    sequenceObject = omAddObjEx(playerSlot, 0, 0, 0, -1, InitializeMinigameState);
    cameraObject = omAddObjEx(playerSlot, 0xA, 0, 0, -1, InitializeCamera);
    InitializePlayerObjects(playerSlot);
    InitializeArenaObjects(playerSlot);
    InitializeCollectibleObjects(playerSlot);
    if (GWMGRecordGet(4) == 0) {
        GWMGRecordSet(4, 0);
    }
    InitializeScoreDisplay(playerSlot, GWMGRecordGet(4));
    SetScoreDisplayMode(0);
}

void SetMinigameState(u32 state)
{
    minigameState = state;
}

u32 GetMinigameState(void)
{
    return minigameState;
}

void InitializeMinigameState(OMOBJ *object)
{
    minigameState = 0;
    sequenceFrameTimer = REFRESH_RATE;
    countdownValue = 60;
    countdownFrameTimer = REFRESH_RATE;
    winCoinCounter = -1;
    winningPlayerCount = 0;
    collectedScore = 0;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    object->objFunc = UpdateMinigameSequence;
}

s32 endSequenceAudioHandle = -1;

void UpdateMinigameSequence(OMOBJ *object)
{
    s32 playerSlot;

    switch (minigameState) {
        case 0:
            if (sequenceFrameTimer == REFRESH_RATE) {
                HuAudFXPlay(0x71B);
                HuAudFXPlay(0x71F);
            }
            if (--sequenceFrameTimer == 0) {
                minigameState = 1;
                sequenceFrameTimer = 2 * REFRESH_RATE;
            }
            break;
        case 1:
            if (--sequenceFrameTimer == 0) {
                minigameState = 2;
                sequenceFrameTimer = 2 * REFRESH_RATE;
            }
            break;
        case 2:
            if (--sequenceFrameTimer == 0) {
                minigameState = 3;
                sequenceId = MGSeqCreate(3, 0);
            }
            break;
        case 3:
            if (MGSeqStatGet(sequenceId) == 0) {
                minigameState = 4;
                resultsSequenceId = MGSeqCreate(1, 60, -1, -1);
            }
            if (endSequenceAudioHandle < 0 && (MGSeqStatGet(sequenceId) & 0x10)) {
                endSequenceAudioHandle = HuAudSeqPlay(0x43);
            }
            break;
        case 4:
            if (--countdownFrameTimer == 0) {
                countdownValue -= 1;
                MGSeqParamSet(resultsSequenceId, 1, countdownValue);
                countdownFrameTimer = REFRESH_RATE;
            }
            if (countdownValue == 0 || remainingCollectibleCount == 0) {
                minigameState = 5;
                sequenceId = MGSeqCreate(3, 1);
                HuAudSeqFadeOut(endSequenceAudioHandle, 100);
            }
            break;
        case 5:
            if (resultsSequenceId >= 0) {
                MGSeqParamSet(resultsSequenceId, 2, -1);
                resultsSequenceId = -1;
            }
            if (MGSeqStatGet(sequenceId) == 0 && ArePlayersInitialized()) {
                if (GWMGRecordGet(4) < collectedScore) {
                    GWMGRecordSet(4, collectedScore);
                    minigameState = 6;
                    sequenceFrameTimer = REFRESH_RATE;
                    sequenceId = MGSeqCreate(0xE, collectedScore);
                    SetScoreDisplayMode(1);
                }
                else {
                    minigameState = 9;
                    sequenceFrameTimer = REFRESH_RATE;
                }
                mgRecordExtra = collectedScore;
            }
            break;
        case 6:
            if (MGSeqStatGet(sequenceId) == 0) {
                minigameState = 9;
                sequenceFrameTimer = REFRESH_RATE;
            }
            break;
        case 9:
            if (--sequenceFrameTimer == 0) {
                minigameState = 11;
                if (collectedScore) {
                    HuAudSStreamPlay(1);
                }
                else {
                    HuAudSStreamPlay(4);
                }
                sequenceFrameTimer = 3.5f * REFRESH_RATE;
            }
            break;
        case 11:
            if (--sequenceFrameTimer == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = ExitMinigame;
            }
            break;
    }
    if (winningPlayerCount != 0) {
        winCoinCounter++;
        for (playerSlot = 0; playerSlot < winningPlayerCount; playerSlot++) {
            GWPlayerCoinWinSet(winningPlayerIndices[playerSlot], winCoinCounter);
        }
        winningPlayerCount = 0;
    }
    if ((omSysExitReq != 0) && (WipeStatGet() == 0)) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = ExitMinigame;
    }
}

void ExitMinigame(OMOBJ *object)
{
    if (WipeStatGet() == 0) {
        FreePlayerObjects();
        NoopArenaCallback();
        FreeCollectibleObjects();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
        OSReport("******* M442Exit *********\n");
    }
}

void InitializePlayerObjects(HUPROCESS *process)
{
    s32 setupIndex;
    s32 gridColumn;

    s32 spriteAnimationDataIds[4] = { 0x00490003, 0x00490006, 0x00490005, 0x00490004 };

    playerObjectStates[0] = playerObjectStates[1] = playerObjectStates[2] = playerObjectStates[3] = -1;
    playerObjectIds[0] = playerObjectIds[1] = playerObjectIds[2] = playerObjectIds[3] = -1;
    nextPlayerIndex = 0;

#if VERSION_PAL
    palRumbleCheck = CheckPlayerRumbleStatus();
#endif

    for (setupIndex = 0; setupIndex < 4; setupIndex++) {
        if (GWPlayerCfg[setupIndex].iscom != 1) {
            break;
        }
        nextPlayerIndex++;
    }
    omMakeGroupEx(process, 0, 1);
    for (setupIndex = 0; setupIndex < 1; setupIndex++) {
        omAddObjEx(process, 5, 0xA, 0x32, 0, CreatePlayerObject);
    }
    playerParticleAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M442, 0x0A), HU_MEMNUM_OVL));
    for (setupIndex = 0; setupIndex < 4; setupIndex++) {
        collectibleAnimationData[setupIndex] = HuSprAnimRead(HuDataReadNum(spriteAnimationDataIds[setupIndex], HU_MEMNUM_OVL));
    }
    memset(navigationGrid, 0, 0x974);
    for (setupIndex = 0; setupIndex < 9; setupIndex++) {
        for (gridColumn = 0; gridColumn < 9; gridColumn++) {
            navigationGrid[setupIndex + 1][gridColumn + 1].worldPosition.x = 1600.0f * (gridColumn - 4) / 8;
            navigationGrid[setupIndex + 1][gridColumn + 1].worldPosition.y = 0.0f;
            navigationGrid[setupIndex + 1][gridColumn + 1].worldPosition.z = 1600.0f * (setupIndex - 4) / 8;
        }
    }
}

void FreePlayerObjects(void)
{
    s32 playerIndex;
    OMOBJ **playerObjects;

    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    if (playerObjects != NULL) {
        for (playerIndex = 0; playerIndex < 1; playerIndex++) {
            FreePlayerObject(playerObjects[playerIndex]);
        }
    }
}

s32 ArePlayersInitialized(void)
{
    s32 playerIndex;
    OMOBJ **playerObjects;
    M442PlayerWork *playerWork;

    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    for (playerIndex = 0; playerIndex < 1; playerIndex++) {
        playerWork = playerObjects[playerIndex]->data;
        if (!playerWork->isInitialized) {
            return 0;
        }
    }
    return 1;
}

s32 playerMotionIds[7] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x05),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x2B),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x48),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18),
};

s32 playerOverlayMotionIds[2] = { 0x490010, 0x490018 };

Vec playerStartingPositions[4] = {
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
};

void CreatePlayerObject(OMOBJ *object)
{
    M442PlayerWork *playerWork;
    u32 motionIndex;
    u32 overlayIndex;
    HU3DMODEL *modelData;
    s32 characterId;

    playerWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M442PlayerWork), HU_MEMNUM_OVL);
    object->data = playerWork;
    memset(playerWork, 0, sizeof(M442PlayerWork));
    playerWork->playerIndex = nextPlayerIndex++;
    playerWork->controllerPort = GWPlayerCfg[playerWork->playerIndex].pad_idx;
#if VERSION_NTSC
    playerWork->regionMode = CheckPlayerRumbleStatus();
#else
    playerWork->regionMode = palRumbleCheck;
#endif
    characterId = GWPlayerCfg[playerWork->playerIndex].character;
    playerWork->characterId = characterId;
    playerWork->difficulty = GWPlayerCfg[playerWork->playerIndex].diff;
    playerWork->playerGroup = GWPlayerCfg[playerWork->playerIndex].group;
    object->mdlId[0] = CharModelCreate(characterId, 8);
    Hu3DModelLayerSet(object->mdlId[0], 1);
    Hu3DModelShadowSet(object->mdlId[0]);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M442, 0x01));
    Hu3DModelHookSet(object->mdlId[0], "test11_tex_we-itemhook-r", object->mdlId[1]);
    playerWork->motionState = 0;
    for (motionIndex = 0; motionIndex < 7; motionIndex++) {
        object->mtnId[motionIndex] = CharMotionCreate(playerWork->characterId, playerMotionIds[motionIndex]);
    }
    for (overlayIndex = 0; overlayIndex < 2; overlayIndex++) {
        object->mtnId[motionIndex + overlayIndex] = CharMotionCreate(playerWork->characterId, playerWork->characterId + playerOverlayMotionIds[overlayIndex]);
    }
    Hu3DMotionSet(object->mdlId[0], object->mtnId[0]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DMotionOverlaySet(object->mdlId[0], object->mtnId[7]);
    CharMotionVoiceOnSet(playerWork->characterId, object->mtnId[5], 0);
    object->trans.x = playerStartingPositions[playerWork->playerIndex].x;
    object->trans.y = playerStartingPositions[playerWork->playerIndex].y;
    object->trans.z = playerStartingPositions[playerWork->playerIndex].z;
    object->mdlId[2] = Hu3DParticleCreate(playerParticleAnimation, 0x32);
    Hu3DParticleHookSet(object->mdlId[2], UpdateCollectibleParticles);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(object->mdlId[2], 1);
    playerWork->targetRouteIndex = -1;
    playerWork->targetIndex = -1;
    object->mdlId[3] = Hu3DHookFuncCreate(DrawPlayerIndicator);
    Hu3DModelLayerSet(object->mdlId[3], 7);
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    modelData = &Hu3DData[object->mdlId[3]];
    modelData->hookData = playerWork;
    CharModelDataClose(playerWork->characterId);
    playerWork->needsAction = 1;
    playerWork->isCelebrating = 1;
    object->objFunc = UpdatePlayerObject;
}

void NoopObjectCallback(OMOBJ *object)
{
    void *objectWorkData;
    s32 gameState;

    objectWorkData = object->data;
    gameState = minigameState;
    if (gameState >= 2) {
        (void)gameState;
    }
}

typedef struct Vec2s16 {
    s16 column;
    s16 row;
} Vec2s16;

s16 navigationDirections[8][2] = {
    { 0xFFFF, 0xFFFF },
    { 0, 0xFFFF },
    { 1, 0xFFFF },
    { 1, 0 },
    { 1, 1 },
    { 0, 1 },
    { 0xFFFF, 1 },
    { 0xFFFF, 0 },
};

void UpdateAiNavigation(OMOBJ *object)
{
    Vec2s16 routeQueue[0x51];
    Vec2s16 candidateRoute[0x51];
    s16 bestRouteNodes[0xA2];
    Vec directionToTarget;
    Vec2s16 playerGridPosition;
    Vec2s16 neighborGridPosition;
    Vec2s16 routePosition;
    Vec2s16 targetGridPosition;

    Float obstacleRisk;
    Float distance;
    Float movementSpeed;
    Float bestRouteRisk;

    s32 gridIndex;
    M442PlayerWork *playerWork;
    s32 gridColumn;
    s32 objectIndex;
    OMOBJ **groupObjects;
    u32 neighborDirection;
    s32 targetPathLength;
    s16 queueReadIndex;
    s16 routeStackSize;
    s32 routeDirectionMask;
    s16 queueWriteIndex;
    M442PlayerWork *otherPlayerWork;

    playerWork = object->data;
    bestRouteRisk = 1.0f;
    VECSubtract(&playerWork->targetPosition, &object->trans, &directionToTarget);
    distance = VECMag(&directionToTarget);
    if (distance < 300.0f) {
        if (distance < 140.0f) {
            if ((frand() & 1) == 0) {
                playerWork->actionRequested = 1;
            }
            else {
                playerWork->needsAction = 1;
            }
        }
        else {
            movementSpeed = 32.0f;
            VECNormalize(&directionToTarget, &directionToTarget);
            playerWork->stickX = directionToTarget.x * movementSpeed;
            playerWork->stickY = -directionToTarget.z * movementSpeed;
        }
        return;
    }
    playerGridPosition.column = 4.0f + ((8.0f * object->trans.x) / 1600.0f);
    playerGridPosition.row = 4.0f + ((8.0f * object->trans.z) / 1600.0f);
    targetGridPosition.column = 4.0f + ((8.0f * playerWork->targetPosition.x) / 1600.0f);
    targetGridPosition.row = 4.0f + ((8.0f * playerWork->targetPosition.z) / 1600.0f);
    for (gridIndex = 0; gridIndex < 9; gridIndex++) {
        for (gridColumn = 0; gridColumn < 9; gridColumn++) {
            navigationGrid[gridIndex + 1][gridColumn + 1].pathStepCount = 0;
            navigationGrid[gridIndex + 1][gridColumn + 1].obstacleWeight = 0.0f;
            groupObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
            for (objectIndex = 0; objectIndex < 1; objectIndex++) {
                if (groupObjects[objectIndex] != object) {
                    VECSubtract(&navigationGrid[gridIndex + 1][gridColumn + 1].worldPosition, &groupObjects[objectIndex]->trans, &directionToTarget);
                    distance = VECMag(&directionToTarget);
                    if (distance < 800.0f) {
                        obstacleRisk = cosd(((90.0f * distance) / 800.0f));
                        obstacleRisk *= obstacleRisk;
                        if (obstacleRisk > navigationGrid[gridIndex + 1][gridColumn + 1].obstacleWeight) {
                            navigationGrid[gridIndex + 1][gridColumn + 1].obstacleWeight = obstacleRisk;
                        }
                    }
                }
            }
            groupObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 2);
            for (objectIndex = 0; objectIndex < 15; objectIndex++) {
                otherPlayerWork = groupObjects[objectIndex]->data;
                if (otherPlayerWork->isActive) {
                    VECSubtract(&navigationGrid[gridIndex + 1][gridColumn + 1].worldPosition, &groupObjects[objectIndex]->trans, &directionToTarget);
                    distance = VECMag(&directionToTarget);
                    if (distance < 800.0f) {
                        obstacleRisk = cosd(((90.0f * distance) / 800.0f));
                        obstacleRisk *= obstacleRisk;
                        if (obstacleRisk > navigationGrid[gridIndex + 1][gridColumn + 1].obstacleWeight) {
                            navigationGrid[gridIndex + 1][gridColumn + 1].obstacleWeight = obstacleRisk;
                        }
                    }
                }
            }
        }
    }
    queueReadIndex = queueWriteIndex = 0;
    navigationGrid[playerGridPosition.row + 1][playerGridPosition.column + 1].pathStepCount = 1;
    routeQueue[queueWriteIndex++] = playerGridPosition;
    do {
        for (gridIndex = 0; gridIndex < 8U; gridIndex++) {
            neighborGridPosition.column = routeQueue[queueReadIndex].column + navigationDirections[gridIndex][0];
            neighborGridPosition.row = routeQueue[queueReadIndex].row + navigationDirections[gridIndex][1];
            if ((neighborGridPosition.column >= 0) && (neighborGridPosition.column < 9) && (neighborGridPosition.row >= 0) && (neighborGridPosition.row < 9)
                && (navigationGrid[neighborGridPosition.row + 1][neighborGridPosition.column + 1].pathStepCount == 0)
                && (navigationGrid[neighborGridPosition.row + 1][neighborGridPosition.column + 1].obstacleWeight >= 0.0f)) {
                navigationGrid[neighborGridPosition.row + 1][neighborGridPosition.column + 1].pathStepCount
                    = navigationGrid[routeQueue[queueReadIndex].row + 1][routeQueue[queueReadIndex].column + 1].pathStepCount + 1;
                routeQueue[queueWriteIndex++] = neighborGridPosition;
            }
        }
        queueReadIndex++;
    } while (queueReadIndex != queueWriteIndex);
    targetPathLength = navigationGrid[targetGridPosition.row + 1][targetGridPosition.column + 1].pathStepCount;
    for (gridIndex = 0; gridIndex < 9; gridIndex++) {
        for (gridColumn = 0; gridColumn < 9; gridColumn++) {
            if (navigationGrid[gridIndex + 1][gridColumn + 1].pathStepCount >= targetPathLength) {
                navigationGrid[gridIndex + 1][gridColumn + 1].pathStepCount = 0;
            }
        }
    }
    navigationGrid[targetGridPosition.row + 1][targetGridPosition.column + 1].pathStepCount = targetPathLength;
    if (targetPathLength > 1) {
        for (gridIndex = targetPathLength - 1; gridIndex > 1; gridIndex--) {
            for (gridColumn = 0; gridColumn < 9; gridColumn++) {
                for (objectIndex = 0; objectIndex < 9; objectIndex++) {
                    if (gridIndex == navigationGrid[gridColumn + 1][objectIndex + 1].pathStepCount) {
                        routeDirectionMask = 0;
                        for (neighborDirection = 0; neighborDirection < 8; neighborDirection++) {
                            if ((gridIndex - 1)
                                == navigationGrid[navigationDirections[neighborDirection][1] + gridColumn + 1][navigationDirections[neighborDirection][0] + objectIndex + 1].pathStepCount) {
                                routeDirectionMask |= 1;
                            }
                            else if ((gridIndex + 1)
                                == navigationGrid[navigationDirections[neighborDirection][1] + gridColumn + 1][navigationDirections[neighborDirection][0] + objectIndex + 1].pathStepCount) {
                                routeDirectionMask |= 2;
                            }
                        }
                        if (routeDirectionMask != 3) {
                            navigationGrid[gridColumn + 1][objectIndex + 1].pathStepCount = 0;
                        }
                    }
                }
            }
        }
    }
    routeStackSize = 0;
    routeQueue[routeStackSize++] = playerGridPosition;
    bestRouteRisk = 1.0f;
    do {
        routeStackSize--;
        routePosition = routeQueue[routeStackSize];
        candidateRoute[navigationGrid[routePosition.row + 1][routePosition.column + 1].pathStepCount - 1] = routePosition;

        if (navigationGrid[routePosition.row + 1][routePosition.column + 1].pathStepCount >= targetPathLength) {
            obstacleRisk = 0.0f;
            for (gridIndex = 1; gridIndex < targetPathLength; gridIndex++) {
                if (navigationGrid[candidateRoute[gridIndex].row + 1][candidateRoute[gridIndex].column + 1].obstacleWeight > obstacleRisk) {
                    obstacleRisk = navigationGrid[candidateRoute[gridIndex].row + 1][candidateRoute[gridIndex].column + 1].obstacleWeight;
                }
            }
            if (obstacleRisk < bestRouteRisk) {
                bestRouteRisk = obstacleRisk;
                memcpy(&bestRouteNodes, &candidateRoute[0], targetPathLength * 4);
            }
        }
        else {
            for (gridIndex = 0; gridIndex < 8U; gridIndex++) {
                neighborGridPosition.column = routePosition.column + navigationDirections[gridIndex][0];
                neighborGridPosition.row = routePosition.row + navigationDirections[gridIndex][1];
                if (navigationGrid[neighborGridPosition.row + 1][neighborGridPosition.column + 1].pathStepCount == (navigationGrid[routePosition.row + 1][routePosition.column + 1].pathStepCount + 1)) {
                    routeQueue[routeStackSize++] = neighborGridPosition;
                }
            }
        }
    } while (routeStackSize != 0);
    VECSubtract(&navigationGrid[bestRouteNodes[3] + 1][bestRouteNodes[2] + 1].worldPosition, &object->trans, &directionToTarget);
    distance = VECMag(&directionToTarget);
    movementSpeed = 64.0f + (0.2f * (32.0f * playerWork->difficulty));
    playerWork->facingAngle = InterpolateAngle(playerWork->facingAngle, atan2d(directionToTarget.x, directionToTarget.z), 0.1f);
    playerWork->stickX = movementSpeed * sind(playerWork->facingAngle);
    playerWork->stickY = movementSpeed * -cosd(playerWork->facingAngle);

    if (++playerWork->targetWaitTimer > 5 * REFRESH_RATE) {
        playerWork->needsAction = 1;
        playerWork->targetWaitTimer = 0;
    }
    (void)object;
    (void)neighborDirection;
    (void)targetPathLength;
    (void)queueReadIndex;
}

void ChooseAiTarget(OMOBJ *object)
{
    OMOBJ *candidatePlayers[15];

    M442PlayerWork *playerWork;
    s32 candidateIndex;
    OMOBJ **playerObjects;
    s32 candidateCount;
    M442PlayerWork *candidateWork;

    playerWork = object->data;
    if (playerWork->needsAction) {
        playerWork->needsAction = 0;
        playerWork->actionRequested = 0;
        playerWork->targetWaitTimer = 0;
        if (((frand() & 0x1F) < (playerWork->difficulty + 4)) && !playerWork->isCelebrating) {
            playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 2);
            candidateCount = 0;
            for (candidateIndex = 0; candidateIndex < 15; candidateIndex++) {
                candidateWork = playerObjects[candidateIndex]->data;
                if (!candidateWork->isActive) {
                    candidatePlayers[candidateCount++] = playerObjects[candidateIndex];
                }
            }
            if (candidateCount == 0) {
                return;
            }
            candidateIndex = (candidateCount * (frand() & 0xFF)) >> 8;
            playerWork->targetPosition.x = candidatePlayers[candidateIndex]->trans.x;
            playerWork->targetPosition.z = candidatePlayers[candidateIndex]->trans.z;
        }
        else {
            playerWork->isCelebrating = 0;
            playerWork->targetPosition.x = 800.0f * ((frand() & 0x1FF) - 256.0f) / 256;
            playerWork->targetPosition.z = 800.0f * ((frand() & 0x1FF) - 256.0f) / 256;
        }
    }
    switch (playerWork->actionRequested) {
        case 0:
            UpdateAiNavigation(object);
            break;
        case 1:
            playerWork->buttonsDown |= 0x100;
            break;
        default:
            break;
    }
}

void UpdatePlayerObject(OMOBJ *object)
{
    Vec interactionPosition;
    Float movementSpeed;

    Float stickX;
    Float stickY;
    double motionProgress;
    Float animationSpeed;
    Float stickMagnitude;
    Float scaledMovementSpeed;

    M442PlayerWork *playerWork;
    s32 motionIndex;
    u32 motionAttributes;
    s32 activeItemType;
    void *particleWork;
    OMOBJ **playerObjects;
    M442PlayerWork *otherPlayerWork;
    s32 gameState;
    s32 gameStateBeforeUpdate;

    playerWork = object->data;
    motionIndex = playerWork->motionState;
    motionAttributes = playerWork->overlayMotion;
    animationSpeed = playerWork->movementSpeed;

    if (!playerWork->isInitialized) {
        gameState = minigameState;
        if (gameState == 5 && (playerWork->playerState == 0 || playerWork->playerState == 5)) {
            playerWork->isInitialized = 1;
            object->objFunc = BeginPlayerFinish;
            return;
        }
    }
    else {
        return;
    }
    gameStateBeforeUpdate = minigameState;
    if (gameStateBeforeUpdate < 4) {
        return;
    }
    switch (playerWork->playerState) {
        case 0:
            if (GWPlayerCfg[playerWork->playerIndex].iscom == 0) {
                playerWork->stickX = HuPadStkX[playerWork->controllerPort];
                playerWork->stickY = HuPadStkY[playerWork->controllerPort];
                playerWork->buttonsPressed = HuPadBtn[playerWork->controllerPort];
                playerWork->buttonsDown = HuPadBtnDown[playerWork->controllerPort];
            }
            else {
                playerWork->buttonsPressed = playerWork->buttonsDown = 0;
                playerWork->stickX = playerWork->stickY = 0;
                ChooseAiTarget(object);
            }
            stickX = playerWork->stickX;
            stickY = -(Float)playerWork->stickY;
            stickMagnitude = sqrtf((stickX * stickX) + (stickY * stickY));
            if (stickMagnitude > 0.0f) {
                stickX /= stickMagnitude;
                stickY /= stickMagnitude;
                if (stickMagnitude > 72.0f) {
                    stickMagnitude = 72.0f;
                }
                scaledMovementSpeed = ((600.0f/REFRESH_RATE) * stickMagnitude) / 72.0f;
                playerWork->movementVelocity.x = stickX * scaledMovementSpeed;
                playerWork->movementVelocity.z = stickY * scaledMovementSpeed;
                playerWork->movementSpeed = scaledMovementSpeed;
                object->rot.y = InterpolateAngle(object->rot.y, atan2d(playerWork->movementVelocity.x, playerWork->movementVelocity.z), 0.5f);
                if (playerWork->movementSpeed < (450.0f / REFRESH_RATE)) {
                    motionIndex = 1;
                    animationSpeed = playerWork->movementSpeed / (450.0f / REFRESH_RATE);
                }
                else {
                    motionIndex = 2;
                    animationSpeed = playerWork->movementSpeed / (600.0f/REFRESH_RATE);
                }
            }
            else {
                playerWork->movementVelocity.x = playerWork->movementVelocity.z = 0.0f;
                motionIndex = 0;
                animationSpeed = 1.0f;
            }
            motionAttributes = HU3D_MOTATTR_LOOP;
            object->trans.x += playerWork->movementVelocity.x;
            object->trans.z += playerWork->movementVelocity.z;
            SeparatePlayerFromHuman(object);
            SeparatePlayersFromGroup(object);
            ClampPlayerToArena(object);
            playerWork->collectibleIndex = FindFacingCollectible(object);
            if (playerWork->buttonsDown & 0x100) {
                if (playerWork->collectibleIndex < 0) {
                    interactionPosition.x = object->trans.x + (140.0 * sind(object->rot.y));
                    interactionPosition.y = object->trans.y;
                    interactionPosition.z = object->trans.z + (140.0 * cosd(object->rot.y));
                }
                else {
                    interactionPosition = collectibleRecords[playerWork->collectibleIndex].worldPosition;
                }
                Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
                if ((IsInteractionPathClear(object, &interactionPosition) == 1)
                    && ((playerWork->collectibleIndex >= 0
                            && ((collectibleRecords[playerWork->collectibleIndex].ownerObject == object) || (collectibleRecords[playerWork->collectibleIndex].ownerObject == NULL)))
                        || (playerWork->collectibleIndex < 0))) {
                    playerWork->playerState = 1;
                    playerWork->animationTimer = playerWork->actionTimer = 60;
                    if (playerWork->collectibleIndex >= 0) {
                        collectibleRecords[playerWork->collectibleIndex].ownerObject = object;
                    }
                }
                else {
                    playerWork->playerState = 2;
                    playerWork->animationTimer = playerWork->actionTimer = 24;
                    playerWork->needsAction = 1;
                }
            }
            break;
        case 1:
            if (playerWork->animationTimer == (playerWork->actionTimer - 8)) {
                Hu3DMotionOverlayReset(object->mdlId[0]);
            }
            if (playerWork->animationTimer == (playerWork->actionTimer / 2)) {
                if (playerWork->collectibleIndex < 0) {
                    if (playerWork->isActive) {
                        interactionPosition.x = object->trans.x + (140.0 * sind(object->rot.y));
                        interactionPosition.y = object->trans.y;
                        interactionPosition.z = object->trans.z + 140.0 * cosd(object->rot.y);
                        playerWork->collectibleIndex = CreateCollectibleRecord(&interactionPosition);
                        collectibleRecords[playerWork->collectibleIndex].ownerObject = object;
                    }
                    else {
                        playerWork->needsAction = 1;
                    }
                }
                if ((playerWork->collectibleIndex >= 0) && (collectibleRecords[playerWork->collectibleIndex].collectedCount < 3)) {
                    collectibleRecords[playerWork->collectibleIndex].collectedCount += 1;
                    collectibleRecords[playerWork->collectibleIndex].respawnTimer = 2 * REFRESH_RATE;
                    HuAudFXPlay(0x71C);
                    if (collectibleRecords[playerWork->collectibleIndex].collectedCount == 3) {
                        playerWork->needsAction = 1;
                        if (playerWork->targetRouteIndex >= 0) {
                            CollectObjectiveItem(playerWork->targetRouteIndex, playerWork->collectibleIndex);
                        }
                    }
                    particleWork = Hu3DData[object->mdlId[2]].hookData;
                    *(s16 *)particleWork = 0;
                    (void)particleWork;
                    Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
                    Hu3DModelPosSet(object->mdlId[2], object->trans.x + (140.0 * sind(object->rot.y)), object->trans.y,
                        object->trans.z + (140.0 * cosd(object->rot.y)));
                }
            }
            motionIndex = 8;
            animationSpeed = 1.0f;
            motionAttributes = HU3D_MOTATTR_NONE;
            object->trans.x += 0.2f * (playerWork->targetX - object->trans.x);
            object->trans.z += 0.2f * (playerWork->targetZ - object->trans.z);
            object->rot.y = InterpolateAngle(object->rot.y, playerWork->targetDistance, 0.2f);
            if (--playerWork->animationTimer == 0) {
                playerWork->playerState = 0;
                Hu3DMotionOverlaySet(object->mdlId[0], object->mtnId[7]);
            }
            break;
        case 2:
            if (playerWork->animationTimer == (playerWork->actionTimer - 8)) {
                Hu3DMotionOverlayReset(object->mdlId[0]);
            }
            motionIndex = 8;
            animationSpeed = playerWork->animationTimer < (playerWork->actionTimer / 2) ? 0.0f : 1.0f;
            motionAttributes = HU3D_MOTATTR_NONE;
            object->trans.x += 0.2f * (playerWork->targetX - object->trans.x);
            object->trans.z += 0.2f * (playerWork->targetZ - object->trans.z);
            object->rot.y = InterpolateAngle(object->rot.y, playerWork->targetDistance, 0.2f);
            if (--playerWork->animationTimer == 0) {
                playerWork->playerState = 0;
                Hu3DMotionOverlaySet(object->mdlId[0], object->mtnId[7]);
            }
            break;
    }
    if (playerWork->targetRouteIndex >= 0 && (playerWork->collectibleIndex < 0 || (playerWork->collectibleIndex >= 0 && collectibleRecords[playerWork->collectibleIndex].collectedCount < 3))) {
        playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 2);
        otherPlayerWork = playerObjects[playerWork->targetRouteIndex]->data;
        activeItemType = otherPlayerWork->playerGroup;
    }
    else {
        activeItemType = -1;
    }
    if (activeItemType != playerWork->targetIndex) {
        playerWork->targetIndex = activeItemType;
        if (activeItemType < 0) {
            playerWork->actionState = 0;
        }
    }
    if (playerWork->targetIndex >= 0) {
        if (playerWork->regionMode == 0) {
            Hu3DModelAttrReset(object->mdlId[3], HU3D_ATTR_DISPOFF);
        }
        switch (playerWork->targetIndex) {
            case 0:
                playerWork->actionProgress = 0.75 + sind(((90.0f * (playerWork->actionState % (REFRESH_RATE / 4))) / (Float)(REFRESH_RATE / 4)));
                if (VERSION_NTSC ? 1 : playerWork->regionMode == 1) {
                    if ((playerWork->actionState % (REFRESH_RATE / 3)) == 0) {
                        omVibrate(playerWork->playerIndex, 0xC, 0xC, 0);
                    }
                }
                break;
            case 1:
                playerWork->actionProgress = 0.75 + (0.75 * sind(((90.0f * (playerWork->actionState % (REFRESH_RATE / 2))) / (0.5f * REFRESH_RATE))));
                if (VERSION_NTSC ? 1 : playerWork->regionMode == 1) {
                    if ((playerWork->actionState % (REFRESH_RATE / 3)) == 0) {
                        omVibrate(playerWork->playerIndex, 0xC, 4, 2);
                    }
                }
                break;
            case 2:
                playerWork->actionProgress = 0.75 + (0.5 * sind(((90.0f * (playerWork->actionState % REFRESH_RATE)) / (1.0f * REFRESH_RATE))));
                if (VERSION_NTSC ? 1 : playerWork->regionMode == 1) {
                    if ((playerWork->actionState % (REFRESH_RATE / 3)) == 0) {
                        omVibrate(playerWork->playerIndex, 12, 6, 6);
                    }
                }
                break;
        }
        playerWork->actionState++;
    }
    else {
        Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    }
    Hu3DModelPosSet(object->mdlId[3], object->trans.x, 300.0f + object->trans.y, object->trans.z);
    if ((motionIndex >= 0 && motionIndex != playerWork->motionState) || motionAttributes != playerWork->overlayMotion) {
        playerWork->motionState = motionIndex;
        playerWork->overlayMotion = motionAttributes;
        CharMotionShiftSet(playerWork->characterId, object->mtnId[playerWork->motionState], 0.0f, 8.0f, motionAttributes);
    }
    playerWork->movementSpeed = animationSpeed;
    CharMotionSpeedSet(playerWork->characterId, animationSpeed);
}

s16 FindFacingCollectible(OMOBJ *object)
{
    Vec interactionPosition;
    Vec directionToItem;
    Vec facingDirection;

    Float itemDistance;
    Float nearestDistance;
    Float facingDotProduct;

    M442PlayerWork *playerWork;
    s32 nearestItemIndex;
    s32 itemIndex;
    M442CollectibleRecord *itemEntry;
    s32 isOutsideBoard;

    nearestItemIndex = -1;
    isOutsideBoard = 0;
    nearestDistance = 50.0f;
    playerWork = object->data;
    facingDirection.x = sind(object->rot.y);
    facingDirection.y = 0.0f;
    facingDirection.z = cosd(object->rot.y);
    interactionPosition.x = object->trans.x + (140.0f * facingDirection.x);
    interactionPosition.y = object->trans.y;
    interactionPosition.z = object->trans.z + (140.0f * facingDirection.z);
    if (interactionPosition.x >= -800.0f && interactionPosition.x < 800.0f && interactionPosition.z >= -800.0f && interactionPosition.z < 800.0f) {
        for (itemEntry = collectibleRecords, itemIndex = 0; itemIndex < collectibleRecordCount; itemIndex++, itemEntry++) {
            VECSubtract(&itemEntry->worldPosition, &interactionPosition, &directionToItem);
            itemDistance = VECMag(&directionToItem);
            VECSubtract(&itemEntry->worldPosition, &object->trans, &directionToItem);
            VECNormalize(&directionToItem, &directionToItem);
            facingDotProduct = VECDotProduct(&directionToItem, &facingDirection);
            if (itemDistance < nearestDistance && facingDotProduct >= 0.0f) {
                nearestDistance = itemDistance;
                nearestItemIndex = itemIndex;
            }
        }
    }
    else {
        isOutsideBoard = 1;
    }
    if (nearestItemIndex >= 0 && collectibleRecords[nearestItemIndex].collectedCount < 3) {
        VECSubtract(&object->trans, &collectibleRecords[nearestItemIndex].worldPosition, &directionToItem);
        VECNormalize(&directionToItem, &directionToItem);
        playerWork->targetX = (140.0f * directionToItem.x) + collectibleRecords[nearestItemIndex].worldPosition.x;
        playerWork->targetY = 0.0f;
        playerWork->targetZ = (140.0f * directionToItem.z) + collectibleRecords[nearestItemIndex].worldPosition.z;
        playerWork->targetDistance = atan2d(-directionToItem.x, -directionToItem.z);
        playerWork->targetRouteIndex = FindNearbyCollectibleObject(&collectibleRecords[nearestItemIndex].worldPosition);
    }
    else {
        playerWork->targetX = object->trans.x;
        playerWork->targetY = object->trans.y;
        playerWork->targetZ = object->trans.z;
        playerWork->targetDistance = object->rot.y;
        playerWork->targetRouteIndex = FindNearbyCollectibleObject(&interactionPosition);
        playerWork->isActive = 0;
        if (nearestItemIndex < 0 && isOutsideBoard == 0) {
            playerWork->isActive = 1;
        }
    }
    return nearestItemIndex;
}

void ClampPlayerToArena(OMOBJ *object)
{
    if (object->trans.x < -800.0f) {
        object->trans.x = -800.0f;
    }
    else if (object->trans.x > 800.0f) {
        object->trans.x = 800.0f;
    }
    if (object->trans.z < -800.0f) {
        object->trans.z = -800.0f;
        return;
    }
    if (object->trans.z > 800.0f) {
        object->trans.z = 800.0f;
    }
}

void SeparatePlayersFromGroup(OMOBJ *object)
{
    Vec separationVector;
    M442PlayerWork *playerWork;

    Float playerDistance;
    Float separationDistance;

    s32 playerIndex;
    OMOBJ **playerObjects;
    M442PlayerWork *otherPlayerWork;

    playerWork = object->data;
    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 2);
    for (playerIndex = 0; playerIndex < 15; playerIndex++) {
        otherPlayerWork = playerObjects[playerIndex]->data;
        if (otherPlayerWork->isActive) {
            VECSubtract(&object->trans, &playerObjects[playerIndex]->trans, &separationVector);
            playerDistance = VECMag(&separationVector);
            if (playerDistance < 70.0f) {
                separationDistance = 70.0f - playerDistance;
                if (playerDistance != 0.0f) {
                    VECNormalize(&separationVector, &separationVector);
                }
                else {
                    separationVector.x = 0.0f;
                    separationVector.z = 1.0f;
                }
                object->trans.x += separationVector.x * separationDistance;
                object->trans.z += separationVector.z * separationDistance;
            }
        }
    }
}

void SeparatePlayerFromHuman(OMOBJ *object)
{
    Vec separationVector;
    void *playerWork;
    void *otherPlayerWork;

    Float separationDistance;
    Float playerDistance;

    s32 playerIndex;
    OMOBJ *otherPlayer;
    OMOBJ **playerObjects;

    playerWork = object->data;
    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    for (playerIndex = 0; playerIndex < 1; playerIndex++) {
        otherPlayer = playerObjects[playerIndex];
        if (object != otherPlayer) {
            otherPlayerWork = otherPlayer->data;
            VECSubtract(&object->trans, &otherPlayer->trans, &separationVector);
            playerDistance = VECMag(&separationVector);
            if (playerDistance < 100.0f) {
                separationDistance = 100.0f - playerDistance;
                if (playerDistance != 0.0f) {
                    VECNormalize(&separationVector, &separationVector);
                }
                else {
                    separationVector.x = 0.0f;
                    separationVector.z = 1.0f;
                }
                object->trans.x += separationVector.x * separationDistance;
                object->trans.z += separationVector.z * separationDistance;
            }
        }
    }
}

void ResolvePlayerCollision(OMOBJ *object)
{
    Vec separationVector;

    Float separationForce;
    Float playerShare;
    Float playerSpeed;
    Float otherPlayerShare;
    Float playerDistance;
    Float otherPlayerSpeed;

    OMOBJ *otherPlayer;
    s32 playerIndex;
    OMOBJ **playerObjects;
    M442PlayerWork *playerWork;
    M442PlayerWork *otherPlayerWork;

    playerWork = object->data;
    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    for (playerIndex = 0; playerIndex < 1; playerIndex++) {
        otherPlayer = playerObjects[playerIndex];
        if (object != otherPlayer) {
            otherPlayerWork = otherPlayer->data;
            VECSubtract(&object->trans, &otherPlayer->trans, &separationVector);
            playerDistance = VECMag(&separationVector);
            if (playerDistance < 100.0f) {
                separationForce = 100.0f - playerDistance;
                playerSpeed = VECMag(&playerWork->movementVelocity);
                otherPlayerSpeed = VECMag(&otherPlayerWork->movementVelocity);
                if (playerSpeed + otherPlayerSpeed == 0.0f) {
                    playerShare = otherPlayerShare = 0.5f;
                }
                else {
                    playerShare = playerSpeed / (playerSpeed + otherPlayerSpeed);
                    otherPlayerShare = 1.0f - playerShare;
                }
                VECNormalize(&separationVector, &separationVector);
                object->trans.x += otherPlayerShare * (separationVector.x * separationForce);
                object->trans.z += otherPlayerShare * (separationVector.z * separationForce);
                otherPlayer->trans.x += playerShare * (-separationVector.x * separationForce);
                otherPlayer->trans.z += playerShare * (-separationVector.z * separationForce);
            }
        }
    }
}

s32 IsInteractionPathClear(OMOBJ *object, Vec *targetPosition)
{
    Vec otherPlayerOffset;
    Vec targetDirection;
    void *playerWork;

    Float otherPlayerDistance;
    Float targetDirectionDot;

    s32 playerIndex;
    OMOBJ **playerObjects;
    s32 targetIsClear;

    playerWork = object->data;
    VECSubtract(targetPosition, &object->trans, &targetDirection);
    VECNormalize(&targetDirection, &targetDirection);
    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    targetIsClear = 1;
    for (playerIndex = 0; playerIndex < 1; playerIndex++) {
        if (object != playerObjects[playerIndex]) {
            VECSubtract(&playerObjects[playerIndex]->trans, targetPosition, &otherPlayerOffset);
            otherPlayerDistance = VECMag(&otherPlayerOffset);
            VECSubtract(&playerObjects[playerIndex]->trans, &object->trans, &otherPlayerOffset);
            VECNormalize(&otherPlayerOffset, &otherPlayerOffset);
            targetDirectionDot = VECDotProduct(&otherPlayerOffset, &targetDirection);
            if (targetDirectionDot >= 0.0f && otherPlayerDistance < 80.0f) {
                targetIsClear = 0;
            }
        }
    }
    (void)targetDirectionDot;
    return targetIsClear;
}

void BeginPlayerFinish(OMOBJ *object)
{
    M442PlayerWork *playerWork;

    playerWork = object->data;
    Hu3DMotionOverlayReset(object->mdlId[0]);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    playerWork->animationTimer = REFRESH_RATE / 5.5;
    object->objFunc = UpdatePlayerFinish;
}

void UpdatePlayerFinish(OMOBJ *object)
{
    Float animationSpeed;

    M442PlayerWork *playerWork;
    s32 motionIndex;
    u32 motionAttributes;
    s32 gameState;

    playerWork = object->data;
    motionIndex = playerWork->motionState;
    motionAttributes = playerWork->overlayMotion;
    animationSpeed = playerWork->movementSpeed;
    gameState = minigameState;
    if (gameState < 11) {
        motionIndex = 0;
        animationSpeed = 1.0f;
        motionAttributes = HU3D_MOTATTR_LOOP;
    }
    else {
        object->rot.y = InterpolateAngle(object->rot.y, 0.0f, 0.2f);
        if (collectedScore != 0) {
            motionIndex = 5;
            animationSpeed = 1.0f;
            motionAttributes = HU3D_MOTATTR_NONE;
            if (playerWork->animationTimer != 0) {
                if (--playerWork->animationTimer == 0) {
                    CharFXPlay(playerWork->characterId, 0x125);
                }
            }
        }
        else {
            motionIndex = 6;
            animationSpeed = 1.0f;
            motionAttributes = HU3D_MOTATTR_NONE;
        }
    }
    if ((motionIndex >= 0 && motionIndex != playerWork->motionState) || motionAttributes != playerWork->overlayMotion) {
        playerWork->motionState = motionIndex;
        playerWork->overlayMotion = motionAttributes;
        CharMotionShiftSet(playerWork->characterId, object->mtnId[playerWork->motionState], 0.0f, 8.0f, motionAttributes);
    }
    playerWork->movementSpeed = animationSpeed;
    CharMotionSpeedSet(playerWork->characterId, animationSpeed);
}

void FreePlayerObject(OMOBJ *object)
{
    void *playerWork;

    playerWork = object->data;
    if (object->data != NULL) {
        HuMemDirectFree(object->data);
        object->data = NULL;
    }
}

void InitializeArenaObjects(HUPROCESS *process)
{
    omMakeGroupEx(process, 1, 1);
    omAddObjEx(process, 10, 6, 0, 1, CreateArenaObject);
}

void NoopArenaCallback(void) { }

u16 framebufferTextureDimensions[2][5] = {
    { 0x280, 0x1E0, 0x280, 0x1E0, 0 },
    { 0x280, 0x1E0, 0x280, 0x1E0, 0 },
};

s32 collectibleAnimationDataIds[3] = { 0x490007, 0x490008, 0x490009 };

void CreateArenaObject(OMOBJ *object)
{
    s32 setupIndex;
    s16 particleSprite;
    HU3DPARTICLE *particleData;

    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M442, 0));
    Hu3DModelLayerSet(object->mdlId[0], 1);
    Hu3DModelShadowMapObjSet(object->mdlId[0], "oasis-grid4");
    object->mdlId[1] = Hu3DParticleCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M442, 12), HU_MEMNUM_OVL)), 1);
    Hu3DModelLayerSet(object->mdlId[1], 7);
    Hu3DParticleHookSet(object->mdlId[1], UpdateArenaParticles);
    particleData = Hu3DData[object->mdlId[1]].hookData;
    object->mdlId[2] = Hu3DParticleCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M442, 13), HU_MEMNUM_OVL)), 20);
    Hu3DModelLayerSet(object->mdlId[2], 7);
    particleData->work = Hu3DData[object->mdlId[2]].hookData;
    particleSprite = espEntry(0x49000E, 0x8000, 0);
    espPosSet(particleSprite, 288.0, 240.0);
    espScaleSet(particleSprite, 80.0, 60.0);
    espColorSet(particleSprite, 0xFF, 0xFF, 0xFF);
    particleData->emitCnt = particleSprite;
    for (setupIndex = 0; setupIndex < 2; setupIndex++) {
        framebufferTextures[setupIndex].sourceWidth = framebufferTextureDimensions[setupIndex][0];
        framebufferTextures[setupIndex].sourceHeight = framebufferTextureDimensions[setupIndex][1];
        framebufferTextures[setupIndex].textureWidth = framebufferTextureDimensions[setupIndex][2];
        framebufferTextures[setupIndex].textureHeight = framebufferTextureDimensions[setupIndex][3];
        framebufferTextures[setupIndex].copyFlags = framebufferTextureDimensions[setupIndex][4];
        framebufferTextures[setupIndex].textureBufferSize = GXGetTexBufferSize(framebufferTextures[setupIndex].textureWidth, framebufferTextures[setupIndex].textureHeight, 4, GX_FALSE, 0);
        framebufferTextures[setupIndex].textureBuffer = HuMemDirectMallocNum(HEAP_HEAP, framebufferTextures[setupIndex].textureBufferSize, HU_MEMNUM_OVL);
        memset(framebufferTextures[setupIndex].textureBuffer, 0, framebufferTextures[setupIndex].textureBufferSize);
    }
    for (setupIndex = 0; setupIndex < 1024; setupIndex++) {
        ResetBackgroundParticle(setupIndex);
    }
    object->mdlId[3] = Hu3DHookFuncCreate(DrawBackgroundParticles);
    backgroundParticleTexture = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M442, 0x0B));
    object->mdlId[4] = Hu3DHookFuncCreate(DrawArenaOverlay);
    Hu3DModelLayerSet(object->mdlId[4], 6);
    for (setupIndex = 0; setupIndex < 15; setupIndex++) {
        collectibleAnimationPhases[setupIndex] = ((Float)(frand() & 0xFF)) / 256;
    }
    memset(unusedGridBuffer, 0, 0x271);
    for (setupIndex = 0; setupIndex < 3; setupIndex++) {
        collectibleTextures[setupIndex] = HuSprAnimReadFile(collectibleAnimationDataIds[setupIndex]);
    }
    object->mdlId[5] = Hu3DHookFuncCreate(DrawCollectibles);
    Hu3DModelLayerSet(object->mdlId[5], 2);
    object->objFunc = NoopArenaObjectCallback;
}

void NoopArenaObjectCallback(OMOBJ *object) { }

void GridToWorldPosition(u8 gridColumn, u8 gridRow, Vec *worldPosition)
{
    worldPosition->x = (66.666664f * (gridColumn - 12)) + (33.333332f * (gridRow & 1));
    worldPosition->y = 0.0f;
    worldPosition->z = 66.666664f * (gridRow - 12);
}

void ResetBackgroundParticle(s32 particleIndex)
{
    backgroundParticles[particleIndex].x = 100.0f * (18.0f * ((frand() & 0x1FF) - 256.0f)) / 256;
    backgroundParticles[particleIndex].y = 0.0f;
    backgroundParticles[particleIndex].z = 100.0f * (18.0f * ((frand() & 0x1FF) - 256.0f)) / 256;
    backgroundParticles[particleIndex].originX = backgroundParticles[particleIndex].x;
    backgroundParticles[particleIndex].riseSpeed = (25.0f / REFRESH_RATE) + ((100.0f / REFRESH_RATE) * (frand() & 0xFF) / 256);
    backgroundParticles[particleIndex].remainingFrames = backgroundParticles[particleIndex].initialFrames = (frand() & 0x3F) + 0x20;
    backgroundParticles[particleIndex].opacity = 0.1f + (0.9f * (frand() & 0xFF) / 256);
    backgroundParticles[particleIndex].angle = 360.0f * (frand() & 0xFF) / 256;
    backgroundParticles[particleIndex].angularSpeed = 4.0f * (frand() & 0xFF) / 256;
    backgroundParticles[particleIndex].horizontalSpeed = 100.0f * (frand() & 0xFF) / 256;
    backgroundParticles[particleIndex].particleScale = 1.0f + (3.0f * (frand() & 0xFF) / 256);
}

GXColor backgroundParticleColor = { 0xFF, 0xFF, 0xFF, 0x20 };

GXColor arenaOverlayColor = { 0x00, 0x7E, 0x7E, 0xFF };

Vec backgroundParticleQuad[4] = {
    { -1.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f },
    { 1.0f, -1.0f, 0.0f },
    { -1.0f, -1.0f, 0.0f },
};

void DrawBackgroundParticles(HU3DMODEL *model, Mtx mtx)
{
    Mtx orthographicProjection;
    Mtx identityMatrix;
    Mtx inverseModelMatrix;
    ROMtx cameraRotationMatrix;
    Mtx perspectiveProjection;
    Vec quadCornerBottomLeft; // ! - uninitialized
    Vec quadCornerBottomRight; // ! - uninitialized
    Vec quadCornerTopRight; // ! - uninitialized
    Vec quadCornerTopLeft;

    Float particleRadius;
    Float particleAlpha;
    Float particleFrameRatio;
    Float particleDepth;
    Float particleWidth;
    Float particleHeight;
    Float textureCoordinateX;
    Float textureCoordinateY;
    Float cameraDepth;
    Float cameraWidth;
    Float cameraHeight;
    Float screenScale;
    Float particleScale;

    M442BackgroundParticle *particle;
    s32 particleIndex;
    HU3DCAMERA *camera;
    u16 textureWidth;
    u16 textureHeight;
    u16 vertexIndex;
    u16 textureIndex;

    camera = Hu3DCamera;
    C_MTXOrtho(orthographicProjection, 0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 100.0f);
    GXSetProjection(orthographicProjection, GX_ORTHOGRAPHIC);
    MTXIdentity(identityMatrix);
    GXLoadPosMtxImm(identityMatrix, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanMatColor(GX_COLOR0A0, arenaOverlayColor);
    GXSetNumTexGens(0);
    HuSprTexLoad(backgroundParticleTexture, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetCullMode(GX_CULL_BACK);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetScissor(0, 0, framebufferTextures[0].textureWidth, framebufferTextures[0].textureHeight);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition2u16(0, 0);
    GXPosition2u16(framebufferTextures[0].textureWidth, 0);
    GXPosition2u16(framebufferTextures[0].textureWidth, framebufferTextures[0].textureHeight);
    GXPosition2u16(0, framebufferTextures[0].textureHeight);
    GXDrawDone();
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 60, GX_FALSE, 125);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    C_MTXPerspective(perspectiveProjection, camera->fov, camera->aspect, camera->near, camera->far);
    GXSetProjection(perspectiveProjection, GX_PERSPECTIVE);
    MTXInverse(mtx, inverseModelMatrix);
    inverseModelMatrix[0][3] = inverseModelMatrix[1][3] = inverseModelMatrix[2][3] = 0.0f;
    PSMTXReorder(inverseModelMatrix, cameraRotationMatrix);
    PSMTXROMultVecArray(cameraRotationMatrix, &backgroundParticleQuad[0], &quadCornerTopLeft, 4);
    GXLoadPosMtxImm(mtx, 0);
    particle = backgroundParticles;
    for (particleIndex = 0; particleIndex < 1024; particleIndex++, particle++) {
        particleRadius = 50.0f * particle->opacity;
        backgroundParticleColor.a = 255.0 * sind(((180.0f * particle->remainingFrames) / particle->initialFrames));
        GXSetChanMatColor(GX_COLOR0A0, backgroundParticleColor);
        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        GXPosition3f32(
            particle->x + (particle->particleScale * (quadCornerTopLeft.x * particleRadius)), particle->y + (quadCornerTopLeft.y * particleRadius), particle->z + (quadCornerTopLeft.z * particleRadius));
        GXTexCoord2f32(0.0f, 0.0f);
        GXPosition3f32(
            particle->x + (particle->particleScale * (quadCornerTopRight.x * particleRadius)), particle->y + (quadCornerTopRight.y * particleRadius), particle->z + (quadCornerTopRight.z * particleRadius));
        GXTexCoord2f32(1.0f, 0.0f);
        GXPosition3f32(
            particle->x + (particle->particleScale * (quadCornerBottomRight.x * particleRadius)), particle->y + (quadCornerBottomRight.y * particleRadius), particle->z + (quadCornerBottomRight.z * particleRadius));
        GXTexCoord2f32(1.0f, 1.0f);
        GXPosition3f32(
            particle->x + (particle->particleScale * (quadCornerBottomLeft.x * particleRadius)), particle->y + (quadCornerBottomLeft.y * particleRadius), particle->z + (quadCornerBottomLeft.z * particleRadius));
        GXTexCoord2f32(0.0f, 1.0f);
        particle->angle += particle->angularSpeed;
        particle->x = particle->originX + (particle->horizontalSpeed * sind(particle->angle));
        particle->y += particle->riseSpeed;
        if (--particle->remainingFrames == 0) {
            ResetBackgroundParticle(particleIndex);
        }
    }
    GXDrawDone();
    CopyTextureFromFramebuffer(&framebufferTextures[0], 1);
}

GXColor arenaOverlayTint = { 0xFF, 0xFF, 0xFF, 0x40 };

Float arenaOverlayDistortionMatrix[2][3] = {
    { 0.0f, 0.0f, 0.025f },
    { 0.0f, 0.025f, 0.0f },
};

GXColor collectibleColors[3] = {
    { 0xFF, 0x00, 0x00, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0x40 },
    { 0xFF, 0x00, 0x00, 0xFF },
};

void DrawArenaOverlay(HU3DMODEL *model, Mtx mtx)
{
    Mtx screenProjection;
    Mtx identityMatrix;
    GXTexObj textureObject;

    CopyTextureFromFramebuffer(&framebufferTextures[1], 0);
    C_MTXOrtho(screenProjection, 0.0f, 480.0f, 0.0f, 640.0f, 0.0f, 100.0f);
    GXSetProjection(screenProjection, GX_ORTHOGRAPHIC);
    MTXIdentity(identityMatrix);
    GXLoadPosMtxImm(identityMatrix, 0);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GXSetNumTexGens(2);
    GXInitTexObj(&textureObject, framebufferTextures[1].textureBuffer, framebufferTextures[1].textureWidth, framebufferTextures[1].textureHeight, 4, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&textureObject, GX_TEXMAP0);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 60, GX_FALSE, 125);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevColor(GX_TEVREG0, arenaOverlayTint);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXInitTexObj(&textureObject, framebufferTextures->textureBuffer, framebufferTextures->textureWidth, framebufferTextures->textureHeight, 4, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&textureObject, GX_TEXMAP1);
    GXSetNumIndStages(1);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, 60, GX_FALSE, 125);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD1, GX_TEXMAP1);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
    GXSetIndTexMtx(GX_ITM_0, arenaOverlayDistortionMatrix, 0);
    GXSetTevIndWarp(GX_TEVSTAGE0, GX_INDTEXSTAGE0, GX_TRUE, GX_FALSE, GX_ITM_0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetCullMode(GX_CULL_BACK);

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition2u16(0, 0);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition2u16(640, 0);
    GXTexCoord2f32(1.0f, 0.0f);
    GXPosition2u16(640, 480);
    GXTexCoord2f32(1.0f, 1.0f);
    GXPosition2u16(0, 480);
    GXTexCoord2f32(0.0f, 1.0f);

    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevDirect(GX_TEVSTAGE1);
}

s16 CreateCollectibleRecord(Vec *position)
{
    s32 cacheIndex;
    M442CollectibleRecord *cacheEntry;

    cacheIndex = collectibleRecordCount;
    cacheEntry = &collectibleRecords[collectibleRecordCount];
    cacheEntry->worldPosition.x = position->x;
    cacheEntry->worldPosition.y = position->y;
    cacheEntry->worldPosition.z = position->z;
    cacheEntry->collectedCount = 0;
    cacheEntry->respawnTimer = 0;
    cacheEntry->ownerObject = NULL;
    collectibleRecordCount = (collectibleRecordCount + 1) % 1024;
    return cacheIndex;
}

s32 FindCollectibleRecord(Vec *position)
{
    Vec positionDelta;
    s32 cacheIndex;

    for (cacheIndex = 0; cacheIndex < collectibleRecordCount; cacheIndex++) {
        VECSubtract(&collectibleRecords[cacheIndex].worldPosition, position, &positionDelta);
        if (VECMag(&positionDelta) < 50.0f) {
            return cacheIndex;
        }
    }
}

void DrawCollectibles(HU3DMODEL *model, Mtx matrix)
{
    Float animationProgress;
    Float spriteScale;
    Float spriteAngle;
    Float spriteOpacity;
    Float spriteWidth;
    Float spriteHeight;
    Float spriteX;
    Float spriteY;

    M442CollectibleRecord *cacheEntry;
    s32 cacheIndex;

    GXLoadPosMtxImm(matrix, 0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetNumTexGens(1);
    for (cacheIndex = 0; cacheIndex < 3; cacheIndex++) {
        HuSprTexLoad(collectibleTextures[cacheIndex], 0, cacheIndex, GX_REPEAT, GX_REPEAT, GX_LINEAR); // TODO
    }
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 60, GX_FALSE, 125);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetCullMode(GX_CULL_BACK);
    cacheEntry = collectibleRecords;
    for (cacheIndex = 0; cacheIndex < collectibleRecordCount; cacheIndex++, cacheEntry++) {
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, cacheEntry->collectedCount - 1, GX_COLOR0A0);
        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        GXPosition3f32(cacheEntry->worldPosition.x - 50.0f, 1.0f, cacheEntry->worldPosition.z - 50.0f);
        GXTexCoord2f32(0.0f, 0.0f);

        GXPosition3f32(50.0f + cacheEntry->worldPosition.x, 1.0f, cacheEntry->worldPosition.z - 50.0f);
        GXTexCoord2f32(1.0f, 0.0f);

        GXPosition3f32(50.0f + cacheEntry->worldPosition.x, 1.0f, 50.0f + cacheEntry->worldPosition.z);
        GXTexCoord2f32(1.0f, 1.0f);

        GXPosition3f32(cacheEntry->worldPosition.x - 50.0f, 1.0f, 50.0f + cacheEntry->worldPosition.z);
        GXTexCoord2f32(0.0f, 1.0f);

        if (cacheEntry->respawnTimer != 0) {
            if (--cacheEntry->respawnTimer == 0) {
                cacheEntry->ownerObject = NULL;
            }
        }
    }
}

GXColor playerIndicatorColor = { 0xFF, 0xFF, 0xFF, 0x80 };

void DrawPlayerIndicator(HU3DMODEL *model, Float (*mtx)[4])
{
    Float verticalOffset;
    Float modelScale;
    Mtx modelMatrix;

    M442PlayerWork *playerWork;
    s32 playerIndex;

    playerWork = model->hookData;
    MTXInverse(mtx, modelMatrix);
    modelMatrix[0][3] = modelMatrix[1][3] = modelMatrix[2][3] = 0.0f;
    MTXConcat(mtx, &modelMatrix[0], mtx);
    GXLoadPosMtxImm(mtx, 0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetNumTexGens(1);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        HuSprTexLoad(collectibleAnimationData[playerIndex], 0, playerIndex, GX_CLAMP, GX_CLAMP, GX_LINEAR); // TODO
    }
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_RASA, GX_CA_TEXA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 60, GX_FALSE, 125);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanMatColor(GX_COLOR0A0, playerIndicatorColor);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetCullMode(GX_CULL_BACK);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    GXPosition3f32(-80.0f, 80.0f, 0.0f);
    GXTexCoord2f32(0.0f, 0.0f);

    GXPosition3f32(80.0f, 80.0f, 0.0f);
    GXTexCoord2f32(1.0f, 0.0f);

    GXPosition3f32(80.0f, -80.0f, 0.0f);
    GXTexCoord2f32(1.0f, 1.0f);

    GXPosition3f32(-80.0f, -80.0f, 0.0f);
    GXTexCoord2f32(0.0f, 1.0f);

    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, playerWork->targetIndex + 1, GX_COLOR0A0);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
    mtxScaleCat(mtx, playerWork->actionProgress, playerWork->actionProgress, playerWork->actionProgress);
    GXLoadPosMtxImm(mtx, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    GXPosition3f32(-80.0f, 80.0f, 0.0f);
    GXTexCoord2f32(0.0f, 0.0f);

    GXPosition3f32(80.0f, 80.0f, 0.0f);
    GXTexCoord2f32(1.0f, 0.0f);

    GXPosition3f32(80.0f, -80.0f, 0.0f);
    GXTexCoord2f32(1.0f, 1.0f);

    GXPosition3f32(-80.0f, -80.0f, 0.0f);
    GXTexCoord2f32(0.0f, 1.0f);
}

void UpdateArenaParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    Vec cameraOrigin;
    Vec incomingParticlePosition;
    Vec reflectedParticlePosition;
    Vec cameraDirection;
    Vec reflectedDirection;

    Float facingDotProduct;
    Float particleProgress;
    Float reflectionProjection;

    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleEmitter;
    s32 particleIndex;

    particleEmitter = particle->work;
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    if (particle->dataCnt == 0) {
        particle->dataCnt = 1;
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->scale = 2000.0f;
            particleData->vel.x = 20.0f;
            particleData->vel.y = 100.0f;
            particleData->vel.z = 0.0f;
            VECNormalize(&particleData->vel, &particleData->vel);
            particleData->color.r = particleData->color.g = particleData->color.b = particleData->color.a = 0xFF;
        }
        particleData = particleEmitter->data;
        for (particleIndex = 0; particleIndex < particleEmitter->maxCnt; particleIndex++, particleData++) {
            particleData->scale = 50.0f + 500.0f * (frand() & 0xFF) / 256;
            particleData->color.r = (frand() & 0x3F) + 0xC0;
            particleData->color.g = (frand() & 0x3F) + 0xC0;
            particleData->color.b = (frand() & 0x3F) + 0xC0;
            particleData->color.a = 0x40;
            particleData->vel.x = (frand() & 0xFF) / 4096.0f;
        }
    }
    cameraDirection.x = sind(CRot.y) * cosd(CRot.x);
    cameraDirection.y = -sind(CRot.x);
    cameraDirection.z = cosd(CRot.y) * cosd(CRot.x);
    cameraOrigin.x = Center.x + (cameraDirection.x * CZoom);
    cameraOrigin.y = Center.y + (cameraDirection.y * CZoom);
    cameraOrigin.z = Center.z + (cameraDirection.z * CZoom);

    particleData = particle->data;
    incomingParticlePosition.x = cameraOrigin.x + (2500.0f * particleData->vel.x);
    incomingParticlePosition.y = cameraOrigin.y + (2500.0f * particleData->vel.y);
    incomingParticlePosition.z = cameraOrigin.z + (2500.0f * particleData->vel.z);

    particleData->zRot += 0.0034906585560035243;
    particleData->pos = incomingParticlePosition;
    DCStoreRangeNoSync(particle->data, particle->maxCnt * 0x44);
    reflectionProjection = (particleData->vel.z * -cameraDirection.z) + ((particleData->vel.x * -cameraDirection.x) + (particleData->vel.y * -cameraDirection.y));

    reflectedDirection.x = -particleData->vel.x + (2.0f * -cameraDirection.x * reflectionProjection);
    reflectedDirection.y = -particleData->vel.y + (2.0f * -cameraDirection.y * reflectionProjection);
    reflectedDirection.z = -particleData->vel.z + (2.0f * -cameraDirection.z * reflectionProjection);
    reflectedParticlePosition.x = cameraOrigin.x + (2500.0f * reflectedDirection.x);
    reflectedParticlePosition.y = cameraOrigin.y + (2500.0f * reflectedDirection.y);
    reflectedParticlePosition.z = cameraOrigin.z + (2500.0f * reflectedDirection.z);
    facingDotProduct = -VECDotProduct(&particleData->vel, &cameraDirection);
    if (facingDotProduct >= 0.0f) {
        espTPLvlSet(particle->emitCnt, 0.5f * facingDotProduct * facingDotProduct);
    }
    else {
        espTPLvlSet(particle->emitCnt, 0.0);
    }
    particleData = particleEmitter->data;
    for (particleIndex = 0; particleIndex < particleEmitter->maxCnt; particleIndex++, particleData++) {
        particleProgress = particleData->vel.x + ((Float)(particleIndex - 2) / (particleEmitter->maxCnt - 2));
        particleData->pos.x = incomingParticlePosition.x + (particleProgress * (reflectedParticlePosition.x - incomingParticlePosition.x));
        particleData->pos.y = incomingParticlePosition.y + (particleProgress * (reflectedParticlePosition.y - incomingParticlePosition.y));
        particleData->pos.z = incomingParticlePosition.z + (particleProgress * (reflectedParticlePosition.z - incomingParticlePosition.z));
        if (facingDotProduct >= 0.0f) {
            particleData->color.a = 64.0f * (facingDotProduct * facingDotProduct);
        }
    }
    DCStoreRangeNoSync(particleEmitter->data, particleEmitter->maxCnt * 0x44);
}

void InitializeCollectibleObjects(HUPROCESS *process)
{
    s32 objectIndex;
    s32 gridColumn;

    remainingCollectibleCount = 15;
    collectibleSpawnIndex = 0;
    omMakeGroupEx(process, 2, 15);
    for (objectIndex = 0; objectIndex < 15; objectIndex++) {
        omAddObjEx(process, 10, 2, 0, 2, CreateCollectibleObject);
    }
    memset(collectibleSpawnGrid, 0, 0x271);
    for (objectIndex = 9; objectIndex < 16; objectIndex++) {
        for (gridColumn = 9; gridColumn < 16; gridColumn++) {
            *(collectibleSpawnGrid + (objectIndex * 0x19) + gridColumn) = 1;
        }
    }
    collectibleParticleAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M442, 15), HU_MEMNUM_OVL));
}

void FreeCollectibleObjects(void)
{
    s32 objectIndex;
    OMOBJ **objectList;

    objectList = omGetGroupMemberListEx(HuPrcCurrentGet(), 2);
    for (objectIndex = 0; objectIndex < 15; objectIndex++) {
        FreeCollectibleObject(objectList[objectIndex]);
    }
}

s16 collectibleTypesBySpawnIndex[16] = { 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 0 };

void CreateCollectibleObject(OMOBJ *object)
{
    s16 availableGridCells[625][2];
    Vec cellWorldPosition;

    Float worldDeltaX;
    Float worldDeltaZ;

    s32 cellIndex;
    M442CollectibleWork *itemWork;
    s32 gridColumn;
    s32 availableCellCount;
    u8 selectedGridRow;
    u8 selectedGridColumn;

    itemWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M442CollectibleWork), HU_MEMNUM_OVL);
    object->data = itemWork;
    memset(itemWork, 0, sizeof(M442CollectibleWork));
    itemWork->spawnIndex = collectibleSpawnIndex++;
    itemWork->verticalScale = 1.0f;
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M442, 0x02));
    Hu3DModelLayerSet(object->mdlId[0], 7);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    object->mdlId[1] = Hu3DParticleCreate(collectibleParticleAnimation, 0x64);
    Hu3DParticleHookSet(object->mdlId[1], UpdateCollectibleParticleEffect);
    Hu3DModelLayerSet(object->mdlId[1], 7);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    for (availableCellCount = 0, cellIndex = 1; cellIndex < 24; cellIndex++) {
        for (gridColumn = 1; gridColumn < 24; gridColumn++) {
            if (*(collectibleSpawnGrid + (cellIndex * 0x19) + gridColumn) == 0) {
                availableGridCells[availableCellCount][0] = gridColumn;
                availableGridCells[availableCellCount][1] = cellIndex;
                availableCellCount++;
            }
        }
    }
    cellIndex = (availableCellCount * (frand() & 0xFF)) >> 8;
    selectedGridRow = availableGridCells[cellIndex][1];
    selectedGridColumn = availableGridCells[cellIndex][0];
    GridToWorldPosition(selectedGridColumn, selectedGridRow, &object->trans);
    itemWork->gridColumn = availableGridCells[cellIndex][0];
    itemWork->gridRow = availableGridCells[cellIndex][1];
    for (cellIndex = 0; cellIndex < 25; cellIndex++) {
        for (gridColumn = 0; gridColumn < 25; gridColumn++) {
            GridToWorldPosition(gridColumn, cellIndex, &cellWorldPosition);
            worldDeltaX = cellWorldPosition.x - object->trans.x;
            worldDeltaZ = cellWorldPosition.z - object->trans.z;

            if (sqrtf((worldDeltaX * worldDeltaX) + (worldDeltaZ * worldDeltaZ)) < 250.0f) {
                *(collectibleSpawnGrid + (cellIndex * 0x19) + gridColumn) = 1;
            }
        }
    }
    itemWork->itemType = collectibleTypesBySpawnIndex[itemWork->spawnIndex];
    itemWork->baseHeight = 0.0f;
    Hu3DModelPosSet(object->mdlId[1], object->trans.x, object->trans.y, object->trans.z);
    object->objFunc = UpdateCollectibleObject;
}

s32 collectibleSoundIds[3] = { 0x721, 0x71E, 0x720 };

void UpdateCollectibleObject(OMOBJ *object)
{
    M442CollectibleWork *itemWork = object->data;
    Float itemTypeScale[3] = { 1.0f, 0.6f, 0.3f };

    switch (itemWork->animationState) {
        case 0:
            if (itemWork->isActive) {
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
                itemWork->animationState = 1;
                itemWork->verticalScale = 0.0f;
                itemWork->animationTimer = REFRESH_RATE;
            }
            break;
        case 1:
            if (itemWork->verticalScale < 0.3f) {
                itemWork->verticalScale += 0.05f;
            }
            itemWork->animationState = 2;
            itemWork->animationTimer = itemWork->animationDuration = 0.75 * REFRESH_RATE;
            itemWork->soundHandle = HuAudFXPlay(collectibleSoundIds[itemWork->itemType]);
            break;
        case 2:
            itemWork->verticalScale = 0.30000001192092896 + (0.800000011920929 * cosd(((90.0f * itemWork->animationTimer) / itemWork->animationDuration)));
            if (--itemWork->animationTimer == 0) {
                itemWork->animationState = 3;
                itemWork->animationTimer = 90;
            }
            break;
        case 3:
            itemWork->verticalScale = 1.0 + (0.10000000149011612 * sind(itemWork->animationTimer));
            itemWork->animationTimer += 3;
            break;
    }
    object->scale.x = object->scale.z = itemTypeScale[itemWork->itemType];
    object->scale.y = itemWork->verticalScale * itemTypeScale[itemWork->itemType];
    itemWork->baseHeight = 400.0f * object->scale.y;
    Hu3DModelPosSet(object->mdlId[1], object->trans.x, object->trans.y, object->trans.z);
    Hu3DModelScaleSet(object->mdlId[1], object->scale.x, object->scale.y, object->scale.z);
}

s32 FindNearbyCollectibleObject(Vec *position)
{
    Vec itemPositionDelta;
    Float nearestItemDistance;

    Float itemDistance;

    s32 itemIndex;
    OMOBJ **itemObjects;
    M442PlayerWork *itemWork;

    nearestItemDistance = 10000.0f;
    if ((position->x < -800.0f) || (position->x >= 800.0f) || (position->z < -800.0f) || (position->z >= 800.0f)) {
        return -1;
    }
    itemObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 2);
    for (itemIndex = 0; itemIndex < 15; itemIndex++) {
        itemWork = itemObjects[itemIndex]->data;
        if (itemWork->isActive == 0) {
            VECSubtract(&itemObjects[itemIndex]->trans, position, &itemPositionDelta);
            itemDistance = VECMag(&itemPositionDelta);
            if (itemDistance < 100.0f) {
                return itemIndex;
            }
        }
    }
    return -1;
}

s16 collectibleScoreValues[4] = { 0x64, 0xA, 1, 0 };

void CollectObjectiveItem(s32 objectIndex, s32 collectibleIndex)
{
    OMOBJ *itemObject;
    M442PlayerWork *itemWork;
    OMOBJ **itemObjects;

    itemObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 2);
    itemObject = itemObjects[objectIndex];
    itemWork = itemObject->data;
    itemWork->isActive = 1;
    itemObject->trans.x = collectibleRecords[collectibleIndex].worldPosition.x;
    itemObject->trans.y = collectibleRecords[collectibleIndex].worldPosition.y;
    itemObject->trans.z = collectibleRecords[collectibleIndex].worldPosition.z;
    HuAudFXPlay(0x71D);
    collectedScore += collectibleScoreValues[itemWork->itemType];
    remainingCollectibleCount--;
}

void FreeCollectibleObject(OMOBJ *object)
{
    if (object->data != NULL) {
        HuMemDirectFree(object->data);
        object->data = NULL;
    }
}

void UpdateCollectibleParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    Float particleAngle;
    Float particleSpeed;

    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    s32 particlesBelowFloor;

    if (particle->dataCnt == 0) {
        particle->dataCnt = 1;
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleData++, particleIndex++) {
            particleData->scale = (50.0f + (20.0f * (frand() & 0xFF))) / 256;
            particleAngle = 360.0f * (frand() & 0xFF) / 256;
            particleData->pos.x = 100.0 * (0.20000000298023224 * sind(particleAngle));
            particleData->pos.y = 0.0f;
            particleData->pos.z = 100.0 * (0.20000000298023224 * cosd(particleAngle));
            particleSpeed = (100.0f / REFRESH_RATE) + ((150.0f / REFRESH_RATE) * (frand() & 0xFF) / 256);
            particleAngle = 360.0f * (frand() & 0xFF) / 256;
            particleData->vel.x = particleSpeed * sind(particleAngle);
            particleData->vel.z = particleSpeed * cosd(particleAngle);
            particleData->vel.y = (1000.0f / REFRESH_RATE) * (frand() & 0xFF) / 256;
            particleData->color.r = particleData->color.g = particleData->color.b = particleData->color.a = 0xFF;
        }
    }
    particlesBelowFloor = 0;
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->pos.y < -50.0f) {
            particlesBelowFloor++;
        }
        else {
            particleData->vel.y += -(1.2f/REFRESH_RATE)*25;
            particleData->pos.x += particleData->vel.x;
            particleData->pos.y += particleData->vel.y;
            particleData->pos.z += particleData->vel.z;
        }
    }
    if (particlesBelowFloor == particle->maxCnt) {
        model->attr |= 1;
    }
    DCStoreRangeNoSync(particle->data, particle->maxCnt * 0x44);
}

void InitializeCollectibleParticle(HU3DPARTICLEDATA *particleData)
{
    Float particleAngle;

    particleData->scale = 30.0f + (10.0f * (frand() & 0xFF) / 256);
    particleAngle = 360.0f * (frand() & 0xFF) / 256;
    particleData->pos.x = 0.0f;
    particleData->pos.y = 0.0f;
    particleData->pos.z = 0.0f;
    particleData->vel.x = 0.0f;
    particleData->vel.y = 0.2f + (0.8f * (frand() & 0xFF) / 256);
    particleData->vel.z = ((frand() & 0xFF) * 0x168) >> 8;
    particleData->accel.x = 0.5f + (0.5f * (frand() & 0xFF) / 256);
    particleData->color.r = 0x19;
    particleData->color.g = 0x13;
    particleData->color.b = 0x1E;
    particleData->color.a = 0xFF;
}

void UpdateCollectibleParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    s32 emitterAngle;
    Float particleAngle;
    Float particleScale;
    Float particleHeight;

    Float particleAge;

    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;

    if (particle->dataCnt == 0) {
        particle->dataCnt = 1;
        particleData = particle->data;
        particleAngle = atan2d(particle->pos.x, particle->pos.z);
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            InitializeCollectibleParticle(particleData);
            particleData->vel.x = 30.0f * (frand() & 0xFF) / 256;
            particleData->scale *= model->scale.x;
        }
    }
    emitterAngle = 0;
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        particleAge = particleData->vel.x - 20.0f;
        particleData->pos.y = 400.0f + (particleAge * (-particleData->vel.y * particleAge));
        particleData->pos.x = 2.0 * (particleData->vel.x * sind(particleData->vel.z));
        particleData->pos.z = 2.0 * (particleData->vel.x * cosd(particleData->vel.z));
        particleData->vel.x += particleData->accel.x;
        if (particleData->vel.x > 30.0f) {
            InitializeCollectibleParticle(particleData);
            particleData->scale *= model->scale.x;
        }
    }
    DCStoreRangeNoSync(particle->data, particle->maxCnt * 0x44);
}

void CopyTextureFromFramebuffer(M442FramebufferTexture *texture, u8 clearFramebuffer)
{
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_TRUE);
    GXSetTexCopySrc(0, 0, texture->sourceWidth, texture->sourceHeight);
    GXSetTexCopyDst(texture->textureWidth, texture->textureHeight, GX_TF_RGB565, texture->copyFlags);
    GXSetCopyClear(BGColor, 0xFFFFFF);
    GXCopyTex(texture->textureBuffer, clearFramebuffer);
    GXPixModeSync();
    DCFlushRange(texture->textureBuffer, texture->textureBufferSize);
}

Float InterpolateAngle(Float currentAngle, Float targetAngle, Float interpolationFactor)
{
    Float angleDifference;
    Float interpolatedAngle;

    angleDifference = fmod(targetAngle - currentAngle, 360.0);
    if (angleDifference < 0.0f) {
        angleDifference += 360.0f;
    }
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    interpolatedAngle = fmod(currentAngle + (interpolationFactor * angleDifference), 360.0);
    if (interpolatedAngle < 0.0f) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

void InitializeCamera(OMOBJ *object)
{
    InterpolateCameraSettings(0, 0, 0.0f);
    object->objFunc = UpdateCamera;
}

void InterpolateCameraSettings(s32 startIndex, s32 endIndex, Float interpolationFactor)
{
    CZoom = cameraSettings[startIndex].cameraZoom + (interpolationFactor * (cameraSettings[endIndex].cameraZoom - cameraSettings[startIndex].cameraZoom));
    Center.x = cameraSettings[startIndex].centerX + (interpolationFactor * (cameraSettings[endIndex].centerX - cameraSettings[startIndex].centerX));
    Center.y = cameraSettings[startIndex].centerY + (interpolationFactor * (cameraSettings[endIndex].centerY - cameraSettings[startIndex].centerY));
    Center.z = cameraSettings[startIndex].centerZ + (interpolationFactor * (cameraSettings[endIndex].centerZ - cameraSettings[startIndex].centerZ));
    CRot.x = cameraSettings[startIndex].rotationX + (interpolationFactor * (cameraSettings[endIndex].rotationX - cameraSettings[startIndex].rotationX));
    CRot.y = cameraSettings[startIndex].rotationY + (interpolationFactor * (cameraSettings[endIndex].rotationY - cameraSettings[startIndex].rotationY));
    CRot.z = cameraSettings[startIndex].rotationZ + (interpolationFactor * (cameraSettings[endIndex].rotationZ - cameraSettings[startIndex].rotationZ));
}

Float cameraTransitionProgress[2] = { 0.0f, 0.0f };

void UpdateCamera(OMOBJ *object)
{
    HU3DCAMERA *camera = Hu3DCamera;
    switch (minigameState) {
        case 1:
            InterpolateCameraSettings(0, 1, sind((90.0f * cameraTransitionProgress[0])));
            if (cameraTransitionProgress[0] < 1.0f) {
                cameraTransitionProgress[0] += 1.0f / REFRESH_RATE;
                if (cameraTransitionProgress[0] > 1.0f) {
                    cameraTransitionProgress[0] = 1.0f;
                    return;
                }
            }
        case 3:
        case 4:
            break;
        case 2:
            InterpolateCameraSettings(1, 2, 1.0 - cosd((90.0f * cameraTransitionProgress[1])));
            if (cameraTransitionProgress[1] < 1.0f) {
                cameraTransitionProgress[1] += 1.0f / REFRESH_RATE;
                if (cameraTransitionProgress[1] > 1.0f) {
                    cameraTransitionProgress[1] = 1.0f;
                    return;
                }
            }
            break;
        case 8:
            InterpolateCameraSettings(1, 1, 0.0f);
            break;
        case 9:
            break;
        default:
            break;
    }
}

s16 GetFinalScore(void)
{
    return collectedScore;
}

s32 controllerRumbleMasks[4] = { 0x80000000, 0x40000000, 0x20000000, 0x10000000 };

s32 CheckPlayerRumbleStatus(void)
{
    s32 playerIndex;
    u32 rumbleStatus;

    rumbleStatus = HuPadRumbleGet();
    for (playerIndex = 0; playerIndex < 1; playerIndex++) {
        if (GWPlayerCfg[playerIndex].iscom == 0 && (rumbleStatus & controllerRumbleMasks[GWPlayerCfg[playerIndex].pad_idx]) == 0) {
            return 0;
        }
    }
    return GWGameStat.rumble;
}

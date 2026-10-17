#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "string.h"

#include "REL/m421Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/frand.h"
#endif

typedef struct M421DllPlayerWork {
    s32 characterId;
    s32 entryTimer;
    s32 motionId;
    u32 playerOrder;
    u32 pressedButtons;
    s32 heldButtons;
    float stickAngle;
    float stickMagnitude;
    s32 isControllable;
    s32 isJumping;
    s32 jumpTimer;
    s32 isComputer;
    char reserved30[0x4];
    s32 resultState;
    s32 isOutOfPlay;
    s32 isInitialized;
    char reserved40[4];
    u16 actionState;
    u32 stateTimer;
    char reserved4C[0x4];
    s16 frameCounter;
    char reserved52[6];
    float currentYaw;
    float targetYaw;
    float yawProgress;
    float yawStep;
    float difficulty;
    s32 effectFlags;
    u32 effectTimer;
    u32 collisionTimer;
    s32 audioEmitterId;
    char reserved7C[4];
    float collisionRadius;
    float playerHeight;
    float collisionHeight;
    float modelHeight;
    float modelScale;
    float modelOpacity;
    float upperModelY;
    float groundY;
    float verticalOffset;
    float modelYOffset;
    float effectScale;
    float effectPhase;
    float effectVelocityX;
    float effectVelocityY;
    u32 effectState;
    u32 celebrationTimer;
    u32 resultFlags;
    u32 statusFlags;
    s32 promptSpriteId;
    Vec position;
    Vec velocity;
    Vec savedPosition;
    Vec effectPosition;
    Vec previousPosition;
    float jumpCoefficients[4];
    Mtx rotationMatrix;
    float *arenaInfluenceGrid;
} M421DllPlayerWork; /* size = 0x14C */

typedef struct M421DllPlayerDLWork {
    Vec playerPositions[4];
    u32 playerStates[4];
    GXColor playerColors[4];
    Vec fanVertices[4][0xE];
    u32 shadowTextureSize;
    void *shadowTextureBuffer;
} M421DllPlayerDLWork; /* size = 0x2F8 */

typedef struct M421DllMotionConfig {
    /* 0x00 */ u16 motionId;
    /* 0x04 */ float blendDuration;
    /* 0x08 */ float motionStartFrame;
    /* 0x0C */ float motionEndFrame;
    /* 0x10 */ u32 motionFlags;
} M421DllMotionConfig; /* size = 0x14 */

void InitializePlayer(OMOBJ *object);
void UpdatePlayerIntro(OMOBJ *object);
void UpdatePlayerIdle(OMOBJ *object);
void UpdatePlayerMovement(OMOBJ *object);
void UpdatePlayerJump(OMOBJ *object);
void UpdatePlayerState(OMOBJ *object);
void UpdatePlayerFall(OMOBJ *object);
void UpdatePlayerResult(OMOBJ *object);
void UpdatePlayerFacing(OMOBJ *object);
void UpdatePlayerControls(OMOBJ *object);
void UpdatePlayerResults(OMOBJ *object);
void UpdatePlayerAnimationState(OMOBJ *object);
void UpdatePlayerEffects(OMOBJ *object);
void UpdatePlayerShadow(OMOBJ *object);
void UpdatePlayerProximity(OMOBJ *object);
void UpdatePlayerCollisions(OMOBJ *object);
void ClampPlayerPosition(OMOBJ *object, Vec *desiredPosition, Vec *outputPosition)
{
    Vec clampedPosition;
    float distanceFromCenter;
    float boundaryScale;

    M421DllPlayerWork *work = object->data;

    clampedPosition = *desiredPosition;
    if (work->isOutOfPlay == 0) {
        distanceFromCenter = VECMag(&clampedPosition);
        if (distanceFromCenter > (750.0f - work->collisionHeight)) {
            boundaryScale = (750.0f - work->collisionHeight) / distanceFromCenter;
            VECScale(&clampedPosition, &clampedPosition, boundaryScale);
        }
    }
    *outputPosition = clampedPosition;
}

void UpdatePlayerPairCollisions(OMOBJ *object);
void SetPlayerMotion(OMOBJ *object, u32 motionId);
s32 IsPlayerMotionFinished(OMOBJ *object);
void CalculateJumpCoefficients(float jumpHeight, float jumpDuration, float *coefficients)
{
    coefficients[0] = jumpDuration;
    coefficients[1] = jumpHeight;
    coefficients[3] = (2.0f * -jumpHeight) / (jumpDuration * jumpDuration);
    coefficients[2] = -coefficients[3] * jumpDuration;
}

void UpdateParticleTransform(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void EmitPlayerEffect(OMOBJ *object, Vec *effectPosition);
void CreatePlayerDisplayList(OMOBJ *object);
void PlayerModelCallback(OMOBJ *object);
void DrawPlayerShadow(HU3DMODEL *model, Mtx matrix);
void DrawPlayerFan(HU3DMODEL *model, Mtx matrix);

char *playerItemHookNames[8] = {
    "itemhook-r",
    "itemhook-r",
    "itemhook-r",
    "itemhook-r",
    "itemhook-r",
    "itemhook-r",
    "itemhook-r",
    "itemhook-r",
};

s32 playerModelAssetIds[8][3] = {
    { DATA_MAKE_NUM(DATADIR_M421, 0x06), DATA_MAKE_NUM(DATADIR_M421, 0x0E), DATA_MAKE_NUM(DATADIR_M421, 0x16) },
    { DATA_MAKE_NUM(DATADIR_M421, 0x07), DATA_MAKE_NUM(DATADIR_M421, 0x0F), DATA_MAKE_NUM(DATADIR_M421, 0x17) },
    { DATA_MAKE_NUM(DATADIR_M421, 0x08), DATA_MAKE_NUM(DATADIR_M421, 0x10), DATA_MAKE_NUM(DATADIR_M421, 0x18) },
    { DATA_MAKE_NUM(DATADIR_M421, 0x09), DATA_MAKE_NUM(DATADIR_M421, 0x11), DATA_MAKE_NUM(DATADIR_M421, 0x19) },
    { DATA_MAKE_NUM(DATADIR_M421, 0x0A), DATA_MAKE_NUM(DATADIR_M421, 0x12), DATA_MAKE_NUM(DATADIR_M421, 0x1A) },
    { DATA_MAKE_NUM(DATADIR_M421, 0x0B), DATA_MAKE_NUM(DATADIR_M421, 0x13), DATA_MAKE_NUM(DATADIR_M421, 0x1B) },
    { DATA_MAKE_NUM(DATADIR_M421, 0x0C), DATA_MAKE_NUM(DATADIR_M421, 0x14), DATA_MAKE_NUM(DATADIR_M421, 0x1C) },
    { DATA_MAKE_NUM(DATADIR_M421, 0x0D), DATA_MAKE_NUM(DATADIR_M421, 0x15), DATA_MAKE_NUM(DATADIR_M421, 0x1D) },
};
s32 playerAccessoryAssetIds[8] = {
    DATA_MAKE_NUM(DATADIR_M421, 0x1E),
    DATA_MAKE_NUM(DATADIR_M421, 0x1F),
    DATA_MAKE_NUM(DATADIR_M421, 0x20),
    DATA_MAKE_NUM(DATADIR_M421, 0x21),
    DATA_MAKE_NUM(DATADIR_M421, 0x22),
    DATA_MAKE_NUM(DATADIR_M421, 0x23),
    DATA_MAKE_NUM(DATADIR_M421, 0x24),
    DATA_MAKE_NUM(DATADIR_M421, 0x25),
};
float playerModelScales[8] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.8f };
s32 playerMotionIds[6] = { 0, 2, 3, 0x14, 0x17, 0x18 };
M421DllMotionConfig playerMotionConfigs[6] = {
    { 0, 0.2f, 0.0f, -1.0f, 0x40000001 },
    { 1, 0.2f, 0.0f, -1.0f, 0x40000001 },
    { 2, 0.2f, 0.0f, -1.0f, 0x40000001 },
    { 3, 0.2f, 0.0f, -1.0f, 0 },
    { 4, 0.2f, 0.0f, -1.0f, 0 },
    { 5, 0.2f, 0.0f, -1.0f, 0 },
};
Vec playerApproachPositions[4] = {
    { 0.0f, 0.0f, 100.0f },
    { -200.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f },
    { 200.0f, 0.0f, 0.0f },
};
Vec playerResultPositions[4] = {
    { -80.0f, 0.0f, -300.0f },
    { -200.0f, 0.0f, -50.0f },
    { 0.0f, 0.0f, -150.0f },
    { 200.0f, 0.0f, -50.0f },
};
Vec2f resultJumpParameters[3] = { { 300.0f, 0.4f }, { 450.0f, 0.7f }, { 600.0f, 1.0f } };
u32 resultCelebrationDurations[3] = { REFRESH_RATE / 10, REFRESH_RATE / 5, REFRESH_RATE / 5 };
float resultLaunchSpeeds[3] = {

    (VERSION_PAL) ? 480.0f / REFRESH_RATE_F : 480.0f / REFRESH_RATE_OFF_BY_1,
    510.0f / REFRESH_RATE,
    550.0f / REFRESH_RATE
};
float resultBounceScales[3] = { 0.9f, 0.8f, 0.5f };
float resultEffectVelocitiesX[3] = { 1188.0f * REFRESH_FREQ, 1224.0f * REFRESH_FREQ, 1224.0f * REFRESH_FREQ };
float resultEffectVelocitiesY[3] = { -70.0f, -60.000004f, -70.0f };

float playerApproachRanges[3] = { VERSION_PAL ? 192.0f : 192.00003f, 357.0f, 550.0f };

// M421DllPlayerWork
OMOBJ *playerObjects[4];
// M421DllPlayerDLWork
OMOBJ *playerDisplayObject;
u32 playerOrderById[4];
u32 collisionCheckPlayerIndex;

void CreatePlayers(HUPROCESS *process)
{
    u32 playerGroups[4];
    u32 playerIds[4];
    s16 playerIndex;
    s16 comparePlayerIndex;
    u32 swapValue;
    OMOBJ *playerObject;

    collisionCheckPlayerIndex = -1;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerGroups[playerIndex] = GWPlayerCfg[playerIndex].group;
        playerIds[playerIndex] = playerIndex;
    }
    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        for (comparePlayerIndex = playerIndex + 1; comparePlayerIndex < 4; comparePlayerIndex++) {
            if (playerGroups[playerIndex] > playerGroups[comparePlayerIndex]) {
                swapValue = playerGroups[playerIndex];
                playerGroups[playerIndex] = playerGroups[comparePlayerIndex];
                playerGroups[comparePlayerIndex] = swapValue;
                swapValue = playerIds[playerIndex];
                playerIds[playerIndex] = playerIds[comparePlayerIndex];
                playerIds[comparePlayerIndex] = swapValue;
            }
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerOrderById[playerIds[playerIndex]] = playerIndex;
    }
    omMakeGroupEx(process, 0, 4);
    omGetGroupMemberListEx(process, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObject = playerObjects[playerIndex] = omAddObjEx(process, 0x28, 5, 6, 0, InitializePlayer);
        playerObject->work[0] = playerIndex;
    }
    playerDisplayObject = omAddObjEx(process, 0x1E, 2, 0, -1, CreatePlayerDisplayList);
    playerDisplayObject->work[0] = 0;
}

void ResetPlayerMotion(void)
{
    CharModelKill(-1);
}

void InitializePlayer(OMOBJ *object)
{
    s32 entryFlag;

    M421DllPlayerWork *work;
    s32 motionIndex;
    HU3DPARTICLEDATA *particle;
    u32 playerType;
    s32 characterId;
    s32 particleModelId;
    u32 playerIndex;

    float difficultyScaleByLevel[4] = { 0.25f, 0.5f, 0.75f, 1.0f };
    float playerDimensions[2][2] = { { 100.0f, 143.0f }, { 100.0f, 100.0f } };
    Vec spawnPositions[4] = { 0.0f, 1500.0f, 300.0f, -450.0f, 0.0f, -50.0f, 0.0f, 0.0f, -350.0f, 450.0f, 0.0f, -50.0f };

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M421DllPlayerWork), HU_MEMNUM_OVL);
    work = object->data;
    memset(work, 0, sizeof(M421DllPlayerWork));
    object->stat |= 0x100;
    playerIndex = object->work[0];
    characterId = GWPlayerCfg[playerIndex].character;
    work->characterId = characterId;
    entryFlag = work->entryTimer = 0;
    work->playerOrder = playerOrderById[object->work[0]];
    playerType = 0;
    if (work->playerOrder != 0) {
        playerType++;
    }
    SetPlayerStatusAndOrder(object->work[0], work->playerOrder, work->characterId);
    work->difficulty = difficultyScaleByLevel[GWPlayerCfg[playerIndex].diff & 3];
    work->isComputer = GWPlayerCfg[object->work[0]].iscom;
    object->mdlId[0] = CharModelCreate(characterId, 4);
    Hu3DModelLayerSet(object->mdlId[0], 1);
    for (motionIndex = 0; motionIndex < 6; motionIndex++) {
        object->mtnId[motionIndex] = CharMotionCreate(characterId, playerMotionIds[motionIndex]);
    }
    CharMotionDataClose(characterId);
    if (playerType == 1) {
        object->mdlId[1] = Hu3DModelCreateFile(playerAccessoryAssetIds[characterId]);
        Hu3DModelLayerSet(object->mdlId[1], 1);
        SetPlayerMotion(object, 0);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    }
    object->mdlId[2] = Hu3DModelCreateFile(playerModelAssetIds[characterId][playerType]);
    Hu3DModelLayerSet(object->mdlId[2], 1);
    Hu3DModelShadowSet(object->mdlId[2]);
    if (playerType != 0) {
        object->mdlId[3] = Hu3DModelCreateFile(playerModelAssetIds[characterId][2]);
        Hu3DModelLayerSet(object->mdlId[3], 1);
        Hu3DModelShadowSet(object->mdlId[3]);
        Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
        work->promptSpriteId = espEntry(0x120006, 1, 0);
        espDispOff(work->promptSpriteId);
        espAttrSet(work->promptSpriteId, HUSPR_ATTR_LINEAR);
    }
    particleModelId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M421, 0x27)), 0x80);
    object->mdlId[4] = particleModelId;
    Hu3DModelLayerSet(particleModelId, 1);
    Hu3DParticleHookSet(particleModelId, UpdateParticleTransform);
    for (particle = ((HU3DPARTICLE *)Hu3DData[particleModelId].hookData)->data, motionIndex = 0; motionIndex < 0x80; motionIndex++, particle++) {
        particle->scale = 0.0f;
        particle->color.a = 0;
        particle->time = 0;
    }
    if (work->playerOrder == 0) {
        work->arenaInfluenceGrid = HuMemDirectMallocNum(HEAP_HEAP, 441 * sizeof(float), HU_MEMNUM_OVL);
        memset(work->arenaInfluenceGrid, 0, 441 * sizeof(float));
    }
    object->trans.x = work->position.x = spawnPositions[work->playerOrder].x;
    object->trans.y = work->position.y = spawnPositions[work->playerOrder].y;
    object->trans.z = work->position.z = spawnPositions[work->playerOrder].z;
    work->isControllable = 1;
    work->modelOpacity = 1.0f;
    work->playerHeight = playerDimensions[playerType][0];
    work->collisionHeight = playerDimensions[playerType][1];
    work->modelHeight = 110.0f;
    work->modelScale = 1.0f;
    work->upperModelY = work->position.y + (2.0f * work->collisionHeight);
    work->groundY = work->position.y;
    work->verticalOffset = 0.0f;
    work->modelYOffset = 0.0f;
    work->audioEmitterId = -1;
    work->currentYaw = work->targetYaw = 0.0f;
    work->yawProgress = 0.0f;
    work->yawStep = 0.02f;
    work->collisionRadius = 50.0f;
    work->effectScale = 0.0f;
    MTXIdentity(work->rotationMatrix);
    work->motionId = -1;
    work->effectFlags = 0;
    work->isInitialized = 1;
    work->actionState = 0;
    if (playerType == 0) {
        Hu3DModelShadowSet(object->mdlId[0]);
        object->objFunc = UpdatePlayerIntro;
        Hu3DModelReflectTypeSet(object->mdlId[2], 4);
        work->modelScale = playerModelScales[work->characterId];
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
        return;
    }
    object->objFunc = UpdatePlayerIdle;
}

void UpdatePlayerIntro(OMOBJ *object)
{
    M421DllPlayerWork *work = object->data;
    work->pressedButtons = 0;
    work->heldButtons = 0;
    work->stickAngle = 0.0f;
    work->stickMagnitude = 0.0f;
    switch (work->actionState) {
        case 0:
            if (work->stateTimer > REFRESH_RATE_F * 1.8f) {
                HuAudFXPlay(0x601);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
                work->pressedButtons = 0x100;
                work->actionState++;
            }
            break;
        case 1:
            if (work->isJumping == 0) {
                work->pressedButtons = 0x100;
                work->actionState++;
            }
            break;
    }
    VECAdd(&work->position, &work->velocity, &work->position);
    UpdatePlayerControls(object);
    UpdatePlayerFacing(object);
    UpdatePlayerEffects(object);
    if (getCameraMode() == 2) {
        work->actionState = 0;
        object->objFunc = UpdatePlayerMovement;
    }
}

void UpdatePlayerIdle(OMOBJ *object)
{
    M421DllPlayerWork *work = object->data;
    {
        Vec playerPromptPositions[4] = {
            { 0.0f, 0.0f, 0.0f },
            { 80.0f, 190.0f, 0.0f },
            { 290.0f, 120.0f, 0.0f },
            { 500.0f, 190.0f, 0.0f },
        };
        work->pressedButtons = 0;
        work->heldButtons = 0;
        work->stickAngle = 0.0f;
        work->stickMagnitude = 0.0f;
        switch (work->actionState) {
            case 0:
                if ((work->stateTimer > REFRESH_RATE_F / 2) && ((work->stateTimer & 0xF) == 0) && (frandmod(0x3E8) < 0x12C)) {
                    work->pressedButtons = 0x100;
                    work->actionState++;
                }
                else if (work->stateTimer > REFRESH_RATE_F * 2) {
                    work->actionState++;
                }
                break;
            case 1:
                if (work->stateTimer > REFRESH_RATE_F * 2) {
                    work->stickAngle = atan2d(-work->position.x, 300.0f - work->position.z);
                    work->stickMagnitude = 0.01f;
                    if (work->stateTimer < REFRESH_RATE_F * 2.6f) {
                        espPosSet(work->promptSpriteId, playerPromptPositions[work->playerOrder].x, playerPromptPositions[work->playerOrder].y);
                        espScaleSet(work->promptSpriteId, 0.6000000238418579, 0.6000000238418579);
                        espDispOn(work->promptSpriteId);
                    }
                    else {
                        espDispOff(work->promptSpriteId);
                    }
                }
                if (work->stateTimer >= REFRESH_RATE_F * 3) {
                    work->pressedButtons = 0x100;
                    work->resultFlags = 1;
                    work->actionState++;
                }
                break;
        }
    }
    VECAdd(&work->position, &work->velocity, &work->position);
    UpdatePlayerResults(object);
    UpdatePlayerFacing(object);
    UpdatePlayerEffects(object);
    if (getCameraMode() == 2) {
        work->actionState = 0;
        object->objFunc = UpdatePlayerJump;
    }
}

void UpdatePlayerMovement(OMOBJ *object)
{
    float stickX;
    float stickY;

    M421DllPlayerWork *work = object->data;
    s32 canConfirm = 0;
    work->pressedButtons = 0;
    work->heldButtons = 0;
    work->stickAngle = 0.0f;
    work->stickMagnitude = 0.0f;
    if (getResultState() == 2) {
        if (work->isComputer != 0) {
            UpdatePlayerProximity(object);
        }
        else {
            work->heldButtons = HuPadBtn[GWPlayerCfg[object->work[0]].pad_idx];
            work->pressedButtons = HuPadBtnDown[GWPlayerCfg[object->work[0]].pad_idx];
            stickX = HuPadStkX[GWPlayerCfg[object->work[0]].pad_idx];
            stickY = -HuPadStkY[GWPlayerCfg[object->work[0]].pad_idx];
            work->stickAngle = atan2d(stickX, stickY);
            work->stickMagnitude = 0.013888889f * sqrtf((stickX * stickX) + (stickY * stickY));
            if (work->stickMagnitude > 1.0f) {
                work->stickMagnitude = 1.0f;
            }
        }
    }
    if ((s32)work->actionState != 0) {
        (void)canConfirm;
    }
    else {
        canConfirm = 1;
    }
    VECAdd(&work->position, &work->velocity, &work->position);
    UpdatePlayerCollisions(object);
    UpdatePlayerFacing(object);
    UpdatePlayerControls(object);
    UpdatePlayerEffects(object);
    if ((getResultState() == 3) && (canConfirm != 0) && (isResultsSequenceFinished() != 0)) {
        work->actionState = 0;
        object->objFunc = UpdatePlayerFall;
    }
}

void UpdatePlayerJump(OMOBJ *object)
{
    float stickX;
    float stickY;

    M421DllPlayerWork *work = object->data;
    s32 canConfirm = 0;
    work->pressedButtons = 0;
    work->heldButtons = 0;
    work->stickAngle = 0.0f;
    work->stickMagnitude = 0.0f;
    if (getResultState() == 2) {
        if (work->isComputer != 0) {
            UpdatePlayerProximity(object);
        }
        else {
            work->heldButtons = HuPadBtn[GWPlayerCfg[object->work[0]].pad_idx];
            work->pressedButtons = HuPadBtnDown[GWPlayerCfg[object->work[0]].pad_idx];
            stickX = HuPadStkX[GWPlayerCfg[object->work[0]].pad_idx];
            stickY = -HuPadStkY[GWPlayerCfg[object->work[0]].pad_idx];
            work->stickAngle = atan2d(stickX, stickY);
            work->stickMagnitude = 0.013888889f * sqrtf((stickX * stickX) + (stickY * stickY));
            if (work->stickMagnitude > 1.0f) {
                work->stickMagnitude = 1.0f;
            }
        }
    }
    if ((s32)work->actionState != 0) {
        (void)canConfirm;
    }
    else if (work->isJumping == 0) {
        canConfirm = 1;
    }
    VECAdd(&work->position, &work->velocity, &work->position);
    UpdatePlayerCollisions(object);
    UpdatePlayerResults(object);
    UpdatePlayerFacing(object);
    UpdatePlayerEffects(object);
    if (work->isOutOfPlay != 0) {
        if (setPlayerResultFlag(work->playerOrder) != 0) {
            object->objFunc = UpdatePlayerState;
        }
        else {
            work->isOutOfPlay = 0;
        }
    }
    if ((work->isOutOfPlay == 0) && (getResultState() == 3) && (canConfirm != 0) && (isResultsSequenceFinished() != 0)) {
        work->actionState = 0;
        object->objFunc = UpdatePlayerFall;
    }
}

void UpdatePlayerState(OMOBJ *object)
{
    Mtx rotationMatrix;
    Vec targetPosition;
    Vec launchOffset;
    s32 state;
    float launchAngle;
    float horizontalSpeed;

    M421DllPlayerWork *work;
    M421DllCameraStruct *cameraWork;

    work = object->data;
    state = 0;
    {
        Vec launchDirection = { 0.0f, 0.0f, 0.0f };
        switch (work->actionState) {
            case 7:
                break;
            case 0:
                ResetHostMotion();
                work->isControllable = 0;
                work->currentYaw = work->targetYaw = 0.0f;
                MTXIdentity(work->rotationMatrix);
                Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(object->mdlId[3], HU3D_ATTR_DISPOFF);
                Hu3DModelPosSetV(object->mdlId[3], &work->effectPosition);
                Hu3DMotionTimeSet(object->mdlId[3], 0.0f);
                Hu3DMotionSpeedSet(object->mdlId[3], 3.0f);
                HuAudFXEmiterPlay(0x5FC, &work->effectPosition);
                work->audioEmitterId = CharFXPlayPos(work->characterId, 0x123, &work->position);
                omVibrate(object->work[0], 0xC, 4, 2);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
                SetPlayerMotion(object, 0);
                EmitPlayerEffect(object, &work->effectPosition);
                cameraWork = GetRenderCameraForViewFlag(1);
                mtxRot(rotationMatrix, cameraWork->rotation.x, cameraWork->rotation.y, 0.0f);
                launchDirection.z = cameraWork->zoomDistance - 350.0f;
                MTXMultVec(rotationMatrix, &launchDirection, &launchOffset);
                VECAdd(&cameraWork->lookAtTarget, &launchOffset, &targetPosition);
                CalculateJumpCoefficients(targetPosition.y - work->position.y, 0.9f, work->jumpCoefficients);
                VECSubtract(&targetPosition, &work->position, &launchOffset);
                work->velocity.y = REFRESH_FREQ * work->jumpCoefficients[2];
                work->velocity.x = (10.0f / 9.0f) * REFRESH_FREQ * launchOffset.x;
                work->velocity.z = (10.0f / 9.0f) * REFRESH_FREQ * launchOffset.z;
                work->actionState = 1;
                work->stateTimer = 0;
                break;
            case 1:
                work->velocity.y += REFRESH_FREQ * (REFRESH_FREQ * work->jumpCoefficients[3]);
                horizontalSpeed = sqrtf((work->velocity.x * work->velocity.x) + (work->velocity.z * work->velocity.z));
                horizontalSpeed = atan2d(horizontalSpeed, work->velocity.y);
                horizontalSpeed = horizontalSpeed;
                launchOffset.x = work->velocity.z;
                launchOffset.y = 0.0f;
                launchOffset.z = -work->velocity.x;
                VECNormalize(&launchOffset, &launchOffset);
                MTXRotAxisRad(work->rotationMatrix, &launchOffset, MTXDegToRad(horizontalSpeed));
                launchOffset.x = launchOffset.z = 0.0f;
                launchOffset.y = 1.0f;
                MTXRotAxisRad(rotationMatrix, &launchOffset, MTXDegToRad(22.5f * work->stateTimer));
                MTXConcat(work->rotationMatrix, rotationMatrix, work->rotationMatrix);
                HuAudFXEmiterUpDate(work->audioEmitterId, &work->position);
                if (work->stateTimer >= REFRESH_RATE_F * 1.2f) {
                    MTXIdentity(work->rotationMatrix);
                    work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                    work->actionState = 2;
                    work->stateTimer = 0;
                    SetPlayerReadyFlag(object->work[0]);
                    SetPlayerConfirmFlag(object->work[0]);
                    SetPlayerExitFlag(object->work[0]);
                }
                break;
            case 2:
                work->velocity.y += -980.0f * REFRESH_FREQ * REFRESH_FREQ;
                if (work->stateTimer > REFRESH_RATE_F * 2) {
                    work->actionState = 7;
                    work->stateTimer = 0;
                    work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                }
                break;
        }
    }
    VECAdd(&work->position, &work->velocity, &work->position);
    UpdatePlayerFacing(object);
    UpdatePlayerEffects(object);
}

void UpdatePlayerFall(OMOBJ *object)
{
    M421DllPlayerWork *work = object->data;
    work->pressedButtons = 0;
    work->heldButtons = 0;
    work->stickAngle = 0.0f;
    work->stickMagnitude = 0.0f;
    switch (work->actionState) {
        case 0:
            if (work->playerOrder == 0) {
                work->isControllable = 0;
                work->currentYaw = work->targetYaw = 0.0f;
                MTXIdentity(work->rotationMatrix);
                Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
                SetPlayerMotion(object, 0);
                work->position = work->savedPosition;
            }
            else {
                work->isControllable = 0;
                work->currentYaw = work->targetYaw = 0.0f;
                MTXIdentity(work->rotationMatrix);
                Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
                SetPlayerMotion(object, 0);
                work->position = work->savedPosition;
            }
            EmitPlayerEffect(object, &work->effectPosition);
            HuAudFXEmiterPlay(0x600, &work->effectPosition);
            work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
            work->actionState = 3;
            break;
        case 3:
            Hu3DModelShadowSet(object->mdlId[0]);
            work->velocity.y += -980.0f * REFRESH_FREQ * REFRESH_FREQ;
            if (work->position.y <= 0.0f) {
                work->position.y = 0.0f;
                work->velocity.y = 0.0f;
                work->actionState = 4;
            }
            break;
        case 4:
            work->stateTimer = 0;
            SetPlayerReadyFlag(object->work[0]);
            break;
    }
    VECAdd(&work->position, &work->velocity, &work->position);
    work->savedPosition = work->position;
    UpdatePlayerPairCollisions(object);
    UpdatePlayerFacing(object);
    UpdatePlayerEffects(object);
    if (getResultState() == 5) {
        object->objFunc = UpdatePlayerResult;
    }
}

void UpdatePlayerResult(OMOBJ *object)
{
    Vec targetOffset;
    float targetDistance;
    M421DllPlayerWork *work = object->data;

    VECAdd(&work->position, &work->velocity, &work->position);
    work->savedPosition = work->position;
    UpdatePlayerPairCollisions(object);
    UpdatePlayerFacing(object);
    UpdatePlayerEffects(object);
    switch (work->actionState) {
        case 4:
            if (getWinnerPlayerId(object->work[0]) >= 0) {
                VECSubtract(&playerApproachPositions[work->playerOrder], &work->position, &targetOffset);
            }
            else {
                VECSubtract(&playerResultPositions[work->playerOrder], &work->position, &targetOffset);
            }
            targetOffset.y = 0.0f;
            targetDistance = VECMag(&targetOffset);
            if (targetDistance < 10.0f) {
                targetDistance = 0.0f;
            }
            else {
                if (VECMag(&targetOffset) >= 100.0f) {
                    targetDistance = 100.0f;
                }
                targetDistance = 0.2f + (0.008f * targetDistance);
            }
            work->stickAngle = atan2d(targetOffset.x, targetOffset.z);
            work->stickMagnitude = targetDistance;
            UpdatePlayerAnimationState(object);
            if ((work->stickMagnitude == 0.0f) || (work->stateTimer > REFRESH_RATE * 5)) {
                work->targetYaw = 0.0f;
                work->yawProgress = 0.15f;
                work->yawStep = 0.0f;
                work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                SetPlayerConfirmFlag(object->work[0]);
            }
            if (getExitState() == 1) {
                work->actionState = 5;
            }
            break;
        case 5:
            if (getExitState() == 1) {
                if (getWinnerPlayerId(object->work[0]) >= 0) {
                    SetPlayerMotion(object, 4);
                }
                else {
                    SetPlayerMotion(object, 5);
                }
                work->actionState = 6;
                return;
            }
            break;
        case 6:
            if (IsPlayerMotionFinished(object) != 0) {
                SetPlayerExitFlag(object->work[0]);
            }
            break;
    }
}

void UpdatePlayerFacing(OMOBJ *object)
{
    M421DllPlayerWork *work = object->data;
    work->frameCounter++;
    work->stateTimer++;
    object->trans.x = work->position.x;
    object->trans.y = work->position.y;
    object->trans.z = work->position.z;
    work->yawProgress += work->yawStep;
    if (work->yawProgress > 0.45f) {
        work->yawProgress = 0.45f;
    }
    work->currentYaw = interpolateAngle(work->currentYaw, work->targetYaw, work->yawProgress);
    object->rot.y = work->currentYaw;
}

void UpdatePlayerControls(OMOBJ *object)
{
    Mtx jumpRotation;
    Mtx rotationAxis;
    Vec movementDirection;
    float jumpSpeed;
    float verticalSpeed;
    float launchAngle;
    float horizontalSpeed;

    M421DllPlayerWork *work = object->data;
    if (work->isControllable != 0) {
        if (work->stickMagnitude > 0.0f) {
            work->targetYaw = work->stickAngle;
            work->yawProgress = 0.15f;
            work->yawStep = 0.0f;
        }
        if (work->stickMagnitude < 0.05f) {
            SetPlayerMotion(object, 0);
            CharMotionSpeedSet(work->characterId, 1.0f);
        }
        else {
            verticalSpeed = 0.5f;
            if (work->motionId == 2) {
                verticalSpeed = 0.45f;
            }
            if (work->stickMagnitude > verticalSpeed) {
                SetPlayerMotion(object, 2);
                launchAngle = work->stickMagnitude;
            }
            else {
                SetPlayerMotion(object, 1);
                launchAngle = work->stickMagnitude / verticalSpeed;
            }
            CharMotionSpeedSet(work->characterId, launchAngle);
        }
        movementDirection.x = 1.0 * REFRESH_FREQ * (REFRESH_FREQ * 100.0f * (2.0 * (work->stickMagnitude * sind(work->stickAngle))));
        movementDirection.y = 0.0f;
        movementDirection.z = 1.0 * REFRESH_FREQ * (REFRESH_FREQ * 100.0f * (2.0 * (work->stickMagnitude * cosd(work->stickAngle))));
        VECAdd(&movementDirection, &work->velocity, &work->velocity);
        if (work->isJumping == 0) {
            if ((work->pressedButtons & 0x100) != 0) {
                work->isJumping = 1;
                CalculateJumpCoefficients(400.0f - (2.0f * work->collisionHeight), 0.25f, work->jumpCoefficients);
                work->velocity.y = REFRESH_FREQ * work->jumpCoefficients[2];
                if (getCameraMode() == 2) {
                    HuAudFXEmiterPlay(0x5FE, &work->effectPosition);
                }
            }
        }
        else {
            work->velocity.y += REFRESH_FREQ * (REFRESH_FREQ * work->jumpCoefficients[3]);
            if ((work->velocity.y < 0.0f) && (work->position.y <= 0.0f)) {
                work->velocity.y = 0.0f;
                work->isJumping = 0;
                if (getCameraMode() != 2) {
                    HuAudFXEmiterPlay(0x5FD, &work->effectPosition);
                    omVibrate(object->work[0], 0xC, 4, 2);
                }
                else {
                    HuAudFXEmiterPlay(0x5FF, &work->effectPosition);
                }
            }
        }
        MTXIdentity(jumpRotation);
        MTXIdentity(rotationAxis);
        work->effectScale = 1.0f - work->stickMagnitude;
        movementDirection = work->velocity;
        movementDirection.y = 0.0f;
        jumpSpeed = VECMag(&movementDirection);
        if (jumpSpeed > 0.0f) {
            VECNormalize(&movementDirection, &movementDirection);
            horizontalSpeed = movementDirection.z;
            movementDirection.z = -movementDirection.x;
            movementDirection.x = horizontalSpeed;
            jumpSpeed *= 360.0f / (3.14159f * (2.0f * work->collisionHeight));
            MTXRotAxisRad(jumpRotation, &movementDirection, 0.017453292f * jumpSpeed);
        }
        movementDirection.x = sind((90.0f + work->stickAngle));
        movementDirection.y = 0.0f;
        movementDirection.z = cosd((90.0f + work->stickAngle));
        if (work->stickMagnitude > 0.0f) {
            jumpSpeed = REFRESH_FREQ * 100.0f * work->stickMagnitude * (360.0f / (3.14159f * (2.0f * work->collisionHeight)));
            if (work->isJumping != 0) {
                jumpSpeed *= 2.0f;
            }
            MTXRotAxisRad(rotationAxis, &movementDirection, 0.017453292f * jumpSpeed);
        }
        MTXConcat(jumpRotation, rotationAxis, jumpRotation);
        MTXConcat(jumpRotation, work->rotationMatrix, work->rotationMatrix);
        UpdatePlayerShadow(object);
    }
}

void UpdatePlayerResults(OMOBJ *object)
{
    M421DllPlayerWork *work = object->data;
    if (work->isControllable != 0) {
        if (work->velocity.y > 0.0f) {
            work->groundY = work->position.y - (0.03f / REFRESH_FREQ * work->velocity.y);
            if (work->groundY < 0.0f) {
                work->groundY = 0.0f;
            }
            work->upperModelY = (2.0f * (work->position.y + work->collisionHeight)) - work->groundY;
            work->verticalOffset = 0.5f * work->velocity.y;
            work->modelYOffset = -0.2f * work->verticalOffset;
        }
        else if (work->velocity.y < 0.0f) {
            work->upperModelY = work->position.y + (2.0f * work->collisionHeight);
            work->groundY = work->position.y;
            work->verticalOffset = 0.8f * work->velocity.y;
            work->modelYOffset = work->verticalOffset * (-2.0f / work->effectState);
        }
        else if (work->isJumping == 0) {
            work->groundY = work->position.y;
            if ((work->effectState != 0) || (work->celebrationTimer != 0)) {
                work->upperModelY += work->verticalOffset;
                work->verticalOffset += work->modelYOffset;
            }
            else {
                work->verticalOffset = work->modelYOffset = 0.0f;
                work->upperModelY = work->position.y + (2.0f * work->collisionHeight);
            }
            if (work->celebrationTimer != 0) {
                work->effectPhase = work->effectPhase + work->effectVelocityX;
                work->upperModelY = work->position.y + (2.0f * work->collisionHeight);
                work->upperModelY = work->upperModelY + ((work->celebrationTimer / (0.3f / REFRESH_FREQ)) * (work->effectVelocityY * sind(work->effectPhase)));
            }
        }
        work->effectPosition = work->position;
        work->effectPosition.y = 0.5f * (work->upperModelY + work->groundY);
        work->modelOpacity = (work->effectPosition.y - work->groundY) / work->collisionHeight;
        if ((work->stickMagnitude > 0.0f) && (work->isJumping == 0)) {
            work->targetYaw = work->stickAngle;
            work->yawProgress = 0.15f;
            work->yawStep = 0.0f;
        }
        if ((work->pressedButtons & 0x100) != 0) {
            work->statusFlags = REFRESH_RATE / 5;
        }
        if (work->statusFlags != 0) {
            work->statusFlags--;
        }
        if (work->isJumping == 0) {
            if ((work->jumpTimer == 0) && (work->resultFlags < 3) && ((work->resultFlags != 0) || (work->celebrationTimer == 0)) && (work->statusFlags != 0)) {
                work->jumpTimer = 1;
            }
            if (work->effectState != 0) {
                work->effectState--;
                if (work->effectState == 0) {
                    work->verticalOffset *= resultBounceScales[work->resultFlags - 1];
                    work->modelYOffset = work->verticalOffset * (-2.0f / (work->celebrationTimer + 1));
                }
            }
            else {
                if (work->celebrationTimer != 0) {
                    work->celebrationTimer--;
                }
                if ((work->resultFlags >= 3) || (work->jumpTimer == 0)) {
                    work->resultFlags = 0;
                }
                if (work->jumpTimer != 0) {
                    work->jumpTimer = 0;
                    work->isJumping = 1;
                    work->velocity.x = resultLaunchSpeeds[work->resultFlags] * (work->stickMagnitude * sind(work->stickAngle));
                    work->velocity.z = resultLaunchSpeeds[work->resultFlags] * (work->stickMagnitude * cosd(work->stickAngle));
                    if (work->resultFlags < 2) {
                        HuAudFXEmiterPlay(0x5F9, &work->effectPosition);
                    }
                    else {
                        HuAudFXEmiterPlay(0x5FA, &work->effectPosition);
                    }
                    CalculateJumpCoefficients(resultJumpParameters[work->resultFlags].x - (2.0f * work->collisionHeight), 0.5f * resultJumpParameters[work->resultFlags].y, work->jumpCoefficients);
                    work->velocity.y = REFRESH_FREQ * work->jumpCoefficients[2];
                    work->effectState = (VERSION_PAL) ? 7 : 9;
                    work->celebrationTimer = resultCelebrationDurations[work->resultFlags];
                    work->resultFlags++;
                }
            }
        }
        else {
            work->effectPhase = 0.0f;
            work->effectVelocityX = resultEffectVelocitiesX[work->resultFlags - 1];
            work->effectVelocityY = resultEffectVelocitiesY[work->resultFlags - 1];
            work->velocity.y += REFRESH_FREQ * (REFRESH_FREQ * work->jumpCoefficients[3]);
            if ((work->velocity.y < 0.0f) && (work->position.y <= 0.0f)) {
                work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                work->isJumping = 0;
                HuAudFXEmiterPlay(0x5FB, &work->effectPosition);
            }
        }
    }
}

void UpdatePlayerAnimationState(OMOBJ *object)
{
    float runThreshold;
    float motionSpeed;

    M421DllPlayerWork *work = object->data;
    if (work->stickMagnitude > 0.0f) {
        work->targetYaw = work->stickAngle;
    }
    if ((work->stickMagnitude > 0.05f) && (work->resultState == 0)) {
        work->velocity.x = REFRESH_FREQ * (550.0 * (work->stickMagnitude * sind(work->stickAngle)));
        work->velocity.z = REFRESH_FREQ * (550.0 * (work->stickMagnitude * cosd(work->stickAngle)));
        work->yawProgress = 0.2f;
        work->yawStep = 0.0f;
        runThreshold = 0.5f;
        if (work->motionId == 2) {
            runThreshold = 0.45f;
        }
        if (work->stickMagnitude > runThreshold) {
            SetPlayerMotion(object, 2);
            motionSpeed = work->stickMagnitude;
        }
        else {
            SetPlayerMotion(object, 1);
            motionSpeed = work->stickMagnitude / runThreshold;
        }
        CharMotionSpeedSet(work->characterId, motionSpeed);
        return;
    }
    work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
    SetPlayerMotion(object, 0);
}

void UpdatePlayerEffects(OMOBJ *object)
{
    float modelScale;

    M421DllPlayerWork *work = object->data;
    work->savedPosition = work->position;
    if (work->isControllable != 0) {
        if (work->playerOrder == 0) {
            work->effectPosition = work->position;
            work->effectPosition.y += work->collisionHeight * work->modelOpacity;
            work->savedPosition.y += work->modelOpacity * (work->collisionHeight - work->playerHeight);
            Hu3DModelPosSetV(object->mdlId[0], &work->savedPosition);
            Hu3DModelRotSet(object->mdlId[0], 0.0f, work->currentYaw, 0.0f);
            Hu3DModelScaleSet(object->mdlId[0], work->modelScale, work->modelScale, work->modelScale);
            Hu3DModelPosSetV(object->mdlId[2], &work->effectPosition);
            MTXCopy(work->rotationMatrix, Hu3DData[object->mdlId[2]].mtx);
            Hu3DModelScaleSet(object->mdlId[2], 1.0f, work->modelOpacity, 1.0f);
            return;
        }
        work->savedPosition.y += (2.0f * work->collisionHeight * work->modelOpacity) - work->modelHeight;
        Hu3DModelPosSetV(object->mdlId[0], &work->savedPosition);
        Hu3DModelRotSet(object->mdlId[0], 0.0f, work->currentYaw, 0.0f);
        Hu3DModelPosSetV(object->mdlId[2], &work->effectPosition);
        Hu3DModelRotSet(object->mdlId[2], 0.0f, work->currentYaw, 0.0f);
        modelScale = sqrtf(1.0f / work->modelOpacity);
        Hu3DModelScaleSet(object->mdlId[2], modelScale, work->modelOpacity, modelScale);
        Hu3DModelPosSet(object->mdlId[1], work->effectPosition.x, 15.000001f + (work->effectPosition.y + (work->collisionHeight * work->modelOpacity)), work->effectPosition.z);
        Hu3DModelRotSet(object->mdlId[1], 0.0f, work->currentYaw, 0.0f);
        return;
    }
    work->modelScale += 0.1f * (1.0f - work->modelScale);
    Hu3DModelPosSetV(object->mdlId[0], &work->savedPosition);
    Hu3DModelRotSet(object->mdlId[0], 0.0f, work->currentYaw, 0.0f);
    Hu3DModelScaleSet(object->mdlId[0], work->modelScale, work->modelScale, work->modelScale);
    MTXCopy(work->rotationMatrix, Hu3DData[object->mdlId[0]].mtx);
}

void UpdatePlayerShadow(OMOBJ *object)
{
    Vec currentPosition;
    Vec projectedPosition;
    Vec arenaSample;
    Vec stepVector;
    Vec displacement;
    float falloff;
    float dotFalloff;
    float influenceStrength;
    float travelDistance;

    s32 stepIndex;
    s32 gridIndex;
    s32 sampleIndex;

    M421DllPlayerWork *work = object->data;
    currentPosition = work->position;
    currentPosition.y = 0.0f;
    VECScale(&work->velocity, &stepVector, (VERSION_PAL) ? 6.25f : 7.5f);
    projectedPosition = currentPosition;
    for (stepIndex = 0; stepIndex < 0x10; stepIndex++) {
        VECAdd(&projectedPosition, &stepVector, &projectedPosition);
        ClampPlayerPosition(object, &projectedPosition, &projectedPosition);
    }
    projectedPosition.y = 0.0f;
    VECSubtract(&currentPosition, &projectedPosition, &displacement);
    displacement.y = 0.0f;
    travelDistance = VECMag((Vec *)&displacement.y);
    influenceStrength = 0.0025f * travelDistance;
    if (influenceStrength > 1.0f) {
        influenceStrength = 1.0f;
    }
    if (travelDistance > 0.0f) {
        VECNormalize(&displacement, &displacement);
    }
    arenaSample.y = 0.0f;
    arenaSample.z = -750.0f;
    gridIndex = 0;
    for (stepIndex = 0; stepIndex < 0x15; stepIndex++) {
        arenaSample.x = -750.0f;
        for (sampleIndex = 0; sampleIndex < 0x15; sampleIndex++, gridIndex++) {
            if (VECMag(&arenaSample) >= 700.0f) {
                work->arenaInfluenceGrid[gridIndex] = 10.0f;
            }
            else {
                VECSubtract(&projectedPosition, &arenaSample, &stepVector);
                falloff = 1.0f - (0.001f * VECMag(&stepVector));
                if (falloff < 0.0f) {
                    falloff = 0.0f;
                }
                work->arenaInfluenceGrid[gridIndex] = 3.0f * falloff;
                VECSubtract(&currentPosition, &arenaSample, &stepVector);
                falloff = 1.0f - (0.001f * VECMag(&stepVector));
                if (falloff < 0.0f) {
                    falloff = 0.0f;
                }
                work->arenaInfluenceGrid[gridIndex] += 3.0f * falloff;
                VECSubtract(&arenaSample, &currentPosition, &stepVector);
                falloff = VECMag(&stepVector);
                dotFalloff = 1.0f - (0.001f * falloff);
                if (dotFalloff < 0.0f) {
                    dotFalloff = 0.0f;
                }
                if (falloff > 0.0f) {
                    VECNormalize(&stepVector, &stepVector);
                }
                falloff = VECDotProduct(&stepVector, &displacement);
                if (falloff > 0.0f) {
                    falloff = 4.0f * (influenceStrength * (dotFalloff * (falloff * falloff)));
                    work->arenaInfluenceGrid[gridIndex] += falloff;
                }
            }
            arenaSample.x += 75.0f;
        }
        arenaSample.z += 75.0f;
    }
}

void FindNearestPlayerPosition(Vec *targetPosition, float searchRadius, Vec *bestPosition)
{
    Vec flatTargetPosition;
    Vec arenaSample;
    Vec sampleOffset;
    float sampleDistance;
    float bestScore;

    s32 rowIndex;
    M421DllPlayerWork *work;
    s32 columnIndex;
    s32 gridIndex;

    for (rowIndex = 0; rowIndex < 4; rowIndex++) {
        work = playerObjects[rowIndex]->data;
        if ((work->playerOrder == 0) && work->arenaInfluenceGrid) {
            break;
        }
    }
    flatTargetPosition = *targetPosition;
    flatTargetPosition.y = 0.0f;
    *bestPosition = flatTargetPosition;
    bestScore = 100.0f;
    arenaSample.y = 0.0f;
    arenaSample.z = -750.0f;
    gridIndex = 0;
    for (rowIndex = 0; rowIndex < 0x15; rowIndex++) {
        arenaSample.x = -750.0f;
        for (columnIndex = 0; columnIndex < 0x15; columnIndex++, gridIndex++) {
            VECSubtract(&arenaSample, &flatTargetPosition, &sampleOffset);
            sampleDistance = VECMag(&sampleOffset);
            if (sampleDistance <= searchRadius) {
                sampleDistance = 1.0f + (0.5f - (0.0005f * sampleDistance));
                sampleDistance *= work->arenaInfluenceGrid[gridIndex];
                if (sampleDistance < bestScore) {
                    bestScore = sampleDistance;
                    *bestPosition = arenaSample;
                }
            }
            arenaSample.x += 75.0f;
        }
        arenaSample.z += 75.0f;
    }
}

void UpdatePlayerProximity(OMOBJ *object)
{
    Vec targetPosition;
    Vec targetOffset;
    Vec otherPosition;
    Vec direction;
    Vec velocityDirection;
    float distance;
    float nearestDistance;
    float targetRange;

    M421DllPlayerWork *otherPlayer;
    s32 playerIndex;

    M421DllPlayerWork *player = object->data;
    if (player->playerOrder != 0) {
        if (player->isJumping == 0) {
            switch (player->effectFlags) {
                case 0:
                    if (player->resultFlags == 3) {
                        player->effectFlags = 3;
                        return;
                    }
                    player->effectFlags = 2;
                    player->effectTimer = REFRESH_RATE_F * (0.3f + (0.00040000002f * frandmod(0x3E8)));
                    if ((frandmod(0x410) < (1000.0f * (0.5f + (0.5f * player->difficulty)))) || (player->isInitialized != 0)) {
                        player->isInitialized = 0;
                        player->effectFlags = 1;
                        return;
                    }
                    break;
                case 1:
                    targetRange = playerApproachRanges[player->resultFlags];
                    FindNearestPlayerPosition(&player->position, 1000.0f, &targetPosition);
                    VECSubtract(&targetPosition, &player->position, &targetOffset);
                    targetOffset.y = 0.0f;
                    distance = VECMag(&targetOffset);
                    if (distance < targetRange) {
                        player->stickMagnitude = VECMag(&targetOffset) / targetRange;
                    }
                    else {
                        player->stickMagnitude = 1.0f;
                    }
                    player->stickAngle = atan2d(targetOffset.x, targetOffset.z);
                    player->pressedButtons = 0x100;
                    return;
                case 2:
                    if (player->effectTimer != 0) {
                        player->effectTimer--;
                        return;
                    }
                    player->effectFlags = 0;
                    return;
                case 3:
                    if (player->resultFlags != 3) {
                        player->effectFlags = 2;
                        player->effectTimer = REFRESH_RATE_F * (0.0003f * frandmod(0x3E8));
                        return;
                    }
                    break;
            }
        }
        else {
            player->effectFlags = 0;
        }
    }
    else {
        nearestDistance = 10000.0f;
        if (player->collisionTimer == 0) {
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                if (object->work[0] != playerObjects[playerIndex]->work[0]) {
                    otherPlayer = playerObjects[playerIndex]->data;
                    if (otherPlayer->isOutOfPlay == 0) {
                        if (otherPlayer->isJumping != 0) {
                            VECScale(&otherPlayer->velocity, &otherPosition, REFRESH_RATE_F);
                            VECAdd(&otherPlayer->position, &otherPosition, &otherPosition);
                        }
                        else {
                            otherPosition = otherPlayer->position;
                        }
                        otherPosition.y = 0.0f;
                        otherPosition.x += 100.0f * (0.5f * ((((2.0f * player->difficulty) / 1000.0f) * frandmod(0x3E8)) - player->difficulty));
                        otherPosition.z += 100.0f * (0.5f * ((((2.0f * player->difficulty) / 1000.0f) * frandmod(0x3E8)) - player->difficulty));
                        distance = VECMag(&otherPosition);
                        if (distance > (700.0f - (100.0f * (2.3f * player->difficulty)))) {
                            distance = (700.0f - (100.0f * (2.3f * player->difficulty))) / distance;
                            VECScale(&otherPosition, &otherPosition, distance);
                        }
                        VECSubtract(&otherPosition, &player->position, &direction);
                        direction.y = 0.0f;
                        distance = VECMag(&direction);
                        if (nearestDistance > distance) {
                            player->previousPosition = otherPosition;
                            nearestDistance = distance;
                        }
                    }
                }
            }
            player->collisionTimer = REFRESH_RATE_F * ((1.5f - player->difficulty) + (0.0015f * frandmod(0x3E8)));
        }
        VECSubtract(&player->previousPosition, &player->position, &direction);
        velocityDirection = player->velocity;
        if (VECMag(&direction)) {
            VECNormalize(&direction, &direction);
        }
        if (VECMag(&velocityDirection)) {
            VECNormalize(&velocityDirection, &velocityDirection);
        }
        distance = (0.5f + (0.5f * player->difficulty)) * (1.0f - VECDotProduct(&direction, &velocityDirection));
        VECScale(&player->velocity, &direction, REFRESH_RATE_F * (2.0f * distance));
        VECAdd(&player->position, &direction, &direction);
        VECSubtract(&player->previousPosition, &direction, &otherPosition);
        nearestDistance = VECMag(&otherPosition);
        player->stickAngle = interpolateAngle(player->currentYaw, atan2d(otherPosition.x, otherPosition.z), 0.5f);
        player->stickMagnitude = 0.95f + (0.05f * player->difficulty);
        if ((nearestDistance < 300.0f) && ((player->frameCounter & 7) == 0) && (frandmod(0x514) < (1000.0f * player->difficulty))) {
            player->pressedButtons = 0x100;
        }
        player->collisionTimer--;
        if (nearestDistance < 130.0f) {
            player->collisionTimer = 0;
        }
    }
}

void UpdatePlayerCollisions(OMOBJ *object)
{
    Vec collisionOffset;
    Vec collisionNormal;
    Vec nextPosition;
    Vec separation;
    Vec positionDelta;
    float distance;
    float overlap;

    M421DllPlayerWork *player;
    M421DllPlayerWork *otherPlayer;
    s32 otherPlayerIndex;
    s32 playerIndex;
    M421DllPlayerWork *firstPlayer;
    M421DllPlayerWork *secondPlayer;
    s32 collisionPass;
    s32 playerIndex;

    if (collisionCheckPlayerIndex >= object->work[0]) {
        for (collisionPass = 0; collisionPass < 4; collisionPass++) {
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                    player = playerObjects[otherPlayerIndex]->data;
                    if ((player->isOutOfPlay == 0) && (player->isControllable != 0)) {
                        player->effectPosition = player->position;
                        player->effectPosition.y += player->collisionHeight;
                    }
                }
                player = playerObjects[playerIndex]->data;
                if ((player->isOutOfPlay == 0) && (player->isControllable != 0)) {
                    for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                        if (playerIndex != otherPlayerIndex) {
                            otherPlayer = playerObjects[otherPlayerIndex]->data;
                            if ((otherPlayer->isOutOfPlay == 0) && (otherPlayer->isControllable != 0)) {
                                VECSubtract(&player->effectPosition, &otherPlayer->effectPosition, &collisionOffset);
                                distance = VECMag(&collisionOffset);
                                if (distance < (player->collisionHeight + otherPlayer->collisionHeight)) {
                                    if (distance > 0.0f) {
                                        distance = 0.5f * ((player->collisionHeight + otherPlayer->collisionHeight) - distance);
                                    }
                                    else {
                                        distance = 0.5f * (player->collisionHeight + otherPlayer->collisionHeight);
                                        collisionOffset.x = frandmod(0x3E8) - 0.0005f;
                                        collisionOffset.y = 0.0f;
                                        collisionOffset.z = frandmod(0x3E8) - 0.0005f;
                                    }
                                    VECNormalize(&collisionOffset, &collisionOffset);
                                    VECScale(&collisionOffset, &collisionOffset, distance);
                                    VECAdd(&player->effectPosition, &collisionOffset, &player->effectPosition);
                                    VECSubtract(&otherPlayer->effectPosition, &collisionOffset, &otherPlayer->effectPosition);
                                    if ((player->playerOrder == 0) && (otherPlayer->playerOrder != 0)) {
                                        otherPlayer->isOutOfPlay = 1;
                                    }
                                    else if ((player->playerOrder != 0) && (otherPlayer->playerOrder == 0)) {
                                        player->isOutOfPlay = 1;
                                    }
                                }
                            }
                        }
                    }
                    for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                        player = playerObjects[otherPlayerIndex]->data;
                        if ((player->isOutOfPlay == 0) && (player->isControllable != 0)) {
                            player->position = player->effectPosition;
                            player->position.y -= player->collisionHeight;
                        }
                    }
                    player = playerObjects[playerIndex]->data;
                    if (player->isJumping != 0) {
                        if (player->position.y < 0.0f) {
                            player->position.y = 0.0f;
                        }
                    }
                    else {
                        player->position.y = 0.0f;
                    }
                    collisionOffset = player->position;
                    collisionOffset.y = 0.0f;
                    distance = VECMag(&collisionOffset);
                    if (distance > (750.0f - player->collisionHeight)) {
                        overlap = (750.0f - player->collisionHeight) / distance;
                        VECScale(&collisionOffset, &collisionOffset, overlap);
                        player->position.x = collisionOffset.x;
                        player->position.z = collisionOffset.z;
                    }
                }
            }
        }
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            player = playerObjects[playerIndex]->data;
            if ((player->isOutOfPlay == 0) && (player->isControllable != 0) && (player->playerOrder == 0)) {
                collisionOffset = player->position;
                collisionOffset.y = 0.0f;
                distance = VECMag(&collisionOffset);
                if (distance >= (750.0f - player->collisionHeight)) {
                    distance = VECDotProduct(&player->velocity, &collisionOffset);
                    if (distance > 0.0f) {
                        VECNormalize(&collisionOffset, &collisionOffset);
                        distance = VECDotProduct(&player->velocity, &collisionOffset);
                        player->velocity.x = 0.92f * (player->velocity.x - (1.1f * (distance * collisionOffset.x)));
                        player->velocity.z = 0.92f * (player->velocity.z - (1.1f * (distance * collisionOffset.z)));
                    }
                }
            }
        }
        for (playerIndex = 0; playerIndex < 3; playerIndex++) {
            for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                player = playerObjects[otherPlayerIndex]->data;
                if ((player->isOutOfPlay == 0) && (player->isControllable != 0)) {
                    player->effectPosition = player->position;
                    player->effectPosition.y += player->collisionHeight;
                }
            }
            player = playerObjects[playerIndex]->data;
            if ((player->isOutOfPlay == 0) && (player->isControllable != 0) && (player->playerOrder != 0)) {
                for (otherPlayerIndex = playerIndex; otherPlayerIndex < 4; otherPlayerIndex++) {
                    if (playerIndex != otherPlayerIndex) {
                        otherPlayer = playerObjects[otherPlayerIndex]->data;
                        if ((otherPlayer->isOutOfPlay == 0) && (otherPlayer->isControllable != 0) && (otherPlayer->playerOrder != 0)) {
                            playerIndex = 0;
                            distance = VECMag(&player->velocity);
                            overlap = VECMag(&otherPlayer->velocity);
                            if ((!(distance <= 0.0f) && (player->isJumping != 0)) || (!(overlap <= 0.0f) && (otherPlayer->isJumping != 0))) {
                                if (player->isJumping == 0) {
                                    firstPlayer = otherPlayer;
                                    secondPlayer = player;
                                }
                                else if (otherPlayer->isJumping == 0) {
                                    firstPlayer = player;
                                    secondPlayer = otherPlayer;
                                }
                                else {
                                    playerIndex = 1;
                                    firstPlayer = player;
                                    secondPlayer = otherPlayer;
                                }
                                VECAdd(&firstPlayer->effectPosition, &firstPlayer->velocity, &separation);
                                VECAdd(&secondPlayer->effectPosition, &secondPlayer->velocity, &positionDelta);
                                VECSubtract(&separation, &positionDelta, &collisionOffset);
                                distance = VECMag(&collisionOffset);
                                if ((distance < (firstPlayer->collisionHeight + secondPlayer->collisionHeight)) && (distance > 0.0f)) {
                                    VECNormalize(&collisionOffset, &collisionOffset);
                                    if (playerIndex != 0) {
                                        VECNormalize(&firstPlayer->velocity, &collisionNormal);
                                        VECNormalize(&secondPlayer->velocity, &nextPosition);
                                        distance = -VECDotProduct(&collisionNormal, &collisionOffset);
                                        overlap = VECDotProduct(&nextPosition, &collisionOffset);
                                    }
                                    else {
                                        VECNormalize(&firstPlayer->velocity, &collisionNormal);
                                        nextPosition.x = nextPosition.y = nextPosition.z = 0.0f;
                                        distance = -VECDotProduct(&collisionNormal, &collisionOffset);
                                        overlap = 0.0f;
                                    }
                                    if ((distance > 0.01f) || (distance > 0.01f)) {
                                        distance = VECDotProduct(&firstPlayer->velocity, &collisionOffset);
                                        overlap = VECDotProduct(&secondPlayer->velocity, &collisionOffset);
                                        separation.x = firstPlayer->velocity.x - (distance * collisionOffset.x);
                                        separation.y = firstPlayer->velocity.y - (distance * collisionOffset.y);
                                        separation.z = firstPlayer->velocity.z - (distance * collisionOffset.z);
                                        positionDelta.x = secondPlayer->velocity.x - (overlap * collisionOffset.x);
                                        positionDelta.y = secondPlayer->velocity.y - (overlap * collisionOffset.y);
                                        positionDelta.z = secondPlayer->velocity.z - (overlap * collisionOffset.z);
                                        if (playerIndex != 0) {
                                            separation.x += 0.5f * (overlap * collisionOffset.x);
                                            separation.y += overlap * collisionOffset.y;
                                            separation.z += 0.5f * (overlap * collisionOffset.z);
                                            positionDelta.x += 0.5f * (distance * collisionOffset.x);
                                            positionDelta.y += distance * collisionOffset.y;
                                            positionDelta.z += 0.5f * (distance * collisionOffset.z);
                                        }
                                        else {
                                            separation.x -= 0.5f * (distance * collisionOffset.x);
                                            separation.y -= distance * collisionOffset.y;
                                            separation.z -= 0.5f * (distance * collisionOffset.z);
                                        }
                                        firstPlayer->velocity = separation;
                                        secondPlayer->velocity = positionDelta;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else {
        player = object->data;
        if (player->isOutOfPlay == 0) {
            if (player->position.y < 0.0f) {
                player->position.y = 0.0f;
            }
            collisionOffset = player->position;
            collisionOffset.y = 0.0f;
            distance = VECMag(&collisionOffset);
            if (distance > (750.0f - player->collisionHeight)) {
                overlap = (750.0f - player->collisionHeight) / distance;
                VECScale(&collisionOffset, &collisionOffset, overlap);
                player->position.x = collisionOffset.x;
                player->position.z = collisionOffset.z;
            }
        }
    }
    collisionCheckPlayerIndex = object->work[0];
}

void ClampPlayerPosition(OMOBJ *object, Vec *desiredPosition, Vec *clampedPosition
{
    Vec clampedPosition;
    float distanceFromCenter;
    float boundaryScale;

    M421DllPlayerWork *work = object->data;

    clampedPosition = *desiredPosition;
    if (work->isOutOfPlay == 0) {
        distanceFromCenter = VECMag(&clampedPosition);
        if (distanceFromCenter > (750.0f - work->collisionHeight)) {
            boundaryScale = (750.0f - work->collisionHeight) / distanceFromCenter;
            VECScale(&clampedPosition, &clampedPosition, boundaryScale);
        }
    }
    *outputPosition = clampedPosition;
}

void UpdatePlayerPairCollisions(OMOBJ *object)
{
    Vec separation;
    float playerDistance;
    float boundaryScale;

    M421DllPlayerWork *player;
    M421DllPlayerWork *otherPlayer;
    s32 playerIndex;
    s32 otherPlayerIndex;
    s32 collisionPass;

    for (collisionPass = 0; collisionPass < 2; collisionPass++) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            player = playerObjects[playerIndex]->data;
            if (player->isOutOfPlay == 0) {
                for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                    if (playerIndex != otherPlayerIndex) {
                        otherPlayer = playerObjects[otherPlayerIndex]->data;
                        if (otherPlayer->isOutOfPlay == 0) {
                            VECSubtract(&player->position, &otherPlayer->position, &separation);
                            separation.y = 0.0f;
                            playerDistance = VECMag(&separation);
                            if (playerDistance < (player->collisionRadius + otherPlayer->collisionRadius)) {
                                if (playerDistance > 0.0f) {
                                    playerDistance = 0.5f * ((player->collisionRadius + otherPlayer->collisionRadius) - playerDistance);
                                }
                                else {
                                    playerDistance = 0.5f * (player->collisionRadius + otherPlayer->collisionRadius);
                                    separation.x = frandmod(0x3E8) - 0x1F4;
                                    separation.z = frandmod(0x3E8) - 0x1F4;
                                }
                                VECNormalize(&separation, &separation);
                                VECScale(&separation, &separation, playerDistance);
                                VECAdd(&player->position, &separation, &player->position);
                                VECSubtract(&otherPlayer->position, &separation, &otherPlayer->position);
                            }
                        }
                    }
                }
                if (player->position.y < 0.0f) {
                    player->position.y = 0.0f;
                }
                separation = player->position;
                separation.y = 0.0f;
                playerDistance = VECMag(&separation);
                if (playerDistance > (750.0f - player->collisionRadius)) {
                    boundaryScale = (750.0f - player->collisionRadius) / playerDistance;
                    VECScale(&separation, &separation, boundaryScale);
                    player->position.x = separation.x;
                    player->position.z = separation.z;
                }
            }
        }
    }
}

void SetPlayerMotion(OMOBJ *object, u32 motionId)
{

    M421DllPlayerWork *work = object->data;
    if ((work->motionId != motionId) && (motionId < 6)) {
        float motionBlendDuration = REFRESH_RATE_F * playerMotionConfigs[motionId].blendDuration;
        if (work->motionId < 0) {
            motionBlendDuration = 0.0f;
        }
        work->motionId = motionId;
        CharMotionShiftSet(
            work->characterId, object->mtnId[playerMotionConfigs[motionId].motionId], REFRESH_RATE_F * playerMotionConfigs[motionId].motionStartFrame, motionBlendDuration, playerMotionConfigs[motionId].motionFlags);
        if (playerMotionConfigs[motionId].motionEndFrame >= 0.0f) {
            Hu3DMotionShiftStartEndSet(object->mdlId[0], REFRESH_RATE_F * playerMotionConfigs[motionId].motionStartFrame, REFRESH_RATE_F * playerMotionConfigs[motionId].motionEndFrame);
        }
    }
}

s32 IsPlayerMotionFinished(OMOBJ *object)
{

    M421DllPlayerWork *work = object->data;
    s32 motionHasEnded = 0;
    if ((CharMotionEndCheck(work->characterId) != 0) && (CharMotionShiftIDGet(work->characterId) < 0)) {
        motionHasEnded = 1;
    }
    return motionHasEnded;
}

s32 SetPlayerMotionIfFinished(OMOBJ *object, u32 motionId)
{
    s32 motionWasShifted;

    motionWasShifted = 0;
    if (IsPlayerMotionFinished(object)) {
        SetPlayerMotion(object, motionId);
        motionWasShifted = 1;
    }
    return motionWasShifted;
}

void CalculateJumpCoefficients(float jumpHeight, float jumpDuration, float *coefficients)
{
    cameraIndex[0] = targetAngle;
    cameraIndex[1] = currentAngle;
    cameraIndex[3] = (2.0f * -currentAngle) / (targetAngle * targetAngle);
    cameraIndex[2] = -cameraIndex[3] * targetAngle;
}

void UpdateParticleTransform(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;

    for (particleData = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->time != 0) {
            VECAdd(&particleData->pos, &particleData->vel, &particleData->pos);
            particleData->scale += 2.0f;
            particleData->color.a *= 0.98f;
            if (particleData->time < REFRESH_RATE_F / 2.5f) {
                particleData->color.a = 0.9f * particleData->color.a;
            }
            if (--particleData->time == 0) {
                particleData->time = 0;
                particleData->scale = 0.0f;
                particleData->color.a = 0;
            }
        }
    }
    DCStoreRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void EmitPlayerEffect(OMOBJ *object, Vec *effectPosition)
{
    Vec particleDirection;
    GXColor characterColor;
    u8 baseColor[4];
    u8 colorRange[4];
    float azimuth;
    float elevation;

    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleSystem;
    s32 particleIndex;
    M421DllPlayerWork *work;
    u32 particlesToEmit;

    work = object->data;
    particleSystem = Hu3DData[object->mdlId[4]].hookData;
    GWCharColorGet(work->characterId, &characterColor);
    baseColor[0] = 100.0f + (0.47058824f * characterColor.r);
    baseColor[1] = 100.0f + (0.47058824f * characterColor.g);
    baseColor[2] = 100.0f + (0.47058824f * characterColor.b);
    colorRange[0] = (220.0f + (0.13725491f * characterColor.r)) - baseColor[0];
    colorRange[1] = (220.0f + (0.13725491f * characterColor.g)) - baseColor[1];
    colorRange[2] = (220.0f + (0.13725491f * characterColor.b)) - baseColor[2];
    particlesToEmit = 0x64;
    for (particleData = particleSystem->data, particleIndex = 0; particleIndex < particleSystem->maxCnt; particleIndex++, particleData++) {
        if (particleData->time != 0) {
            continue;
        }
        particleData->time = REFRESH_RATE_F * (0.6f + (0.0005f * frandmod(0x3E8)));
        azimuth = frandmod(0x168);
        elevation = frandmod(0x168);
        particleDirection.y = sind(azimuth);
        azimuth = cosd(azimuth);
        particleDirection.x = azimuth * sind(elevation);
        particleDirection.z = azimuth * cosd(elevation);
        azimuth = 120.00001f;
        VECScale(&particleDirection, &particleDirection, azimuth);
        VECAdd(&particleDirection, effectPosition, &particleData->pos);
        particleData->vel.x = REFRESH_FREQ * (0.5f * particleDirection.x);
        particleData->vel.y = ((VERSION_PAL) ? 6.0f : 5.0f) + (REFRESH_FREQ * (0.5f * particleDirection.y));
        particleData->vel.z = REFRESH_FREQ * (0.5f * particleDirection.z);
        particleData->zRot = 0.0031415902f * frandmod(0x3E8);
        azimuth = 0.001f * frandmod(0x3E8);
        particleData->scale = 80.0f + (220.0f * azimuth);
        particleData->color.a = 100.0f + (100.0f * (1.0f - azimuth));
        particleData->color.r = baseColor[0] + (colorRange[0] * azimuth);
        particleData->color.g = baseColor[1] + (colorRange[1] * azimuth);
        particleData->color.b = baseColor[2] + (colorRange[2] * azimuth);
        if (--particlesToEmit < 1) {
            break;
        }
    }
}

void CreatePlayerDisplayList(OMOBJ *object)
{
    M421DllPlayerDLWork *work;
    s32 i;

    object->stat |= 0x100;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M421DllPlayerDLWork), HU_MEMNUM_OVL);
    work = object->data;
    memset(work, 0, sizeof(M421DllPlayerDLWork));
    work->shadowTextureSize = GXGetTexBufferSize(0x280, 0x1E0, 5, 0, 0);
    work->shadowTextureBuffer = HuMemDirectMallocNum(HEAP_HEAP, work->shadowTextureSize, HU_MEMNUM_OVL);
    DCFlushRange(work->shadowTextureBuffer, work->shadowTextureSize);
    for (i = 0; i < 4; i++) {
        work->playerColors[i].r = work->playerColors[i].g = work->playerColors[i].b = 0xFF;
    }
    work->playerColors[0].a = 0x40;
    work->playerColors[1].a = 0;
    work->playerColors[2].a = 0xFF;
    work->playerColors[3].a = 0;
    for (i = 0; i < 4; i++) {
        work->fanVertices[i][0].x = work->fanVertices[i][0].y = work->fanVertices[i][0].z = 0.0f;
    }
    for (i = 1; i < 0xE; i++) {
        work->fanVertices[0][i].x = 100.0 * (1.7999999523162842 * sind((30.0f * i)));
        work->fanVertices[0][i].y = 100.0 * (1.7999999523162842 * cosd((30.0f * i)));
        work->fanVertices[0][i].z = 0.0f;
        work->fanVertices[1][i].x = 100.0 * (2.0 * cosd((30.0f * i)));
        work->fanVertices[1][i].y = 0.0f;
        work->fanVertices[1][i].z = 100.0 * (2.0 * sind((30.0f * i)));
        work->fanVertices[2][i].x = 100.0 * sind((30.0f * i));
        work->fanVertices[2][i].y = 100.0 * (1.2000000476837158 * cosd((30.0f * i)));
        work->fanVertices[2][i].z = 0.0f;
        work->fanVertices[3][i].x = 100.0 * sind((30.0f * i));
        work->fanVertices[3][i].y = 0.0f;
        work->fanVertices[3][i].z = 100.0 * cosd((30.0f * i));
    }
    DCStoreRangeNoSync(work->playerColors, sizeof(work->playerColors));
    DCStoreRangeNoSync(work->fanVertices, sizeof(work->fanVertices));
    PPCSync();
    object->mdlId[0] = Hu3DHookFuncCreate(DrawPlayerShadow);
    Hu3DModelLayerSet(object->mdlId[0], 4);
    object->mdlId[1] = Hu3DHookFuncCreate(DrawPlayerFan);
    Hu3DModelLayerSet(object->mdlId[1], 6);
    object->objFunc = PlayerModelCallback;
}

void PlayerModelCallback(OMOBJ *object) { }

void DrawPlayerShadow(HU3DMODEL *model, Mtx matrix)
{
    M421DllPlayerDLWork *work = playerDisplayObject->data;
    GXPixModeSync();
    GXSetTexCopySrc(0, 0, 0x280, 0x1E0);
    GXSetTexCopyDst(0x280, 0x1E0, GX_TF_RGB5A3, GX_FALSE);
    GXCopyTex(work->shadowTextureBuffer, GX_FALSE);
    GXPixModeSync();
}

void DrawPlayerFan(HU3DMODEL *model, Mtx matrix)
{
    Mtx textureProjectionMatrix;
    Mtx modelViewMatrix;
    Mtx lightProjectionMatrix;
    Mtx cameraModelMatrix;
    Mtx inverseCameraMatrix;
    Mtx cameraRelativeMatrix;
    GXTexObj shadowTexture;

    M421DllPlayerDLWork *work;
    s32 playerIndex;
    s32 vertexIndex;
    s32 vertexSetIndex;
    s32 colorIndex;

    work = playerDisplayObject->data;
    MTXTrans(modelViewMatrix, -400.0f, 400.0f, -700.0f);
    MTXConcat(Hu3DCameraMtx, modelViewMatrix, cameraModelMatrix);
    GXLoadPosMtxImm(cameraModelMatrix, 0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXInitTexObj(&shadowTexture, work->shadowTextureBuffer, 0x280, 0x1E0, 5, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObjLOD(&shadowTexture, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXLoadTexObj(&shadowTexture, GX_TEXMAP0);
    GXSetNumTexGens(1);
    C_MTXLightPerspective(lightProjectionMatrix, 41.5f, 1.2f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXConcat(lightProjectionMatrix, Hu3DCameraMtx, lightProjectionMatrix);
    MTXInverse(Hu3DCameraMtx, inverseCameraMatrix);
    MTXConcat(inverseCameraMatrix, cameraModelMatrix, textureProjectionMatrix);
    MTXConcat(lightProjectionMatrix, textureProjectionMatrix, modelViewMatrix);
    GXLoadTexMtxImm(modelViewMatrix, 0x1E, GX_MTX3x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, 0x1E, 0, 0x7D);
    MTXInverse(Hu3DCameraMtx, cameraRelativeMatrix);
    cameraRelativeMatrix[0][3] = cameraRelativeMatrix[1][3] = cameraRelativeMatrix[2][3] = 0.0f;
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX8);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        work->playerPositions[playerIndex] = ((M421DllPlayerWork *)playerObjects[playerIndex]->data)->position;
        if (work->playerStates[playerIndex] == 2) {
            colorIndex = 2;
        }
        else {
            colorIndex = 0;
        }
        vertexSetIndex = colorIndex;
        MTXTrans(modelViewMatrix, work->playerPositions[playerIndex].x, work->playerPositions[playerIndex].y, work->playerPositions[playerIndex].z);
        MTXConcat(modelViewMatrix, cameraRelativeMatrix, modelViewMatrix);
        MTXConcat(Hu3DCameraMtx, modelViewMatrix, cameraModelMatrix);
        GXLoadPosMtxImm(cameraModelMatrix, 0);
        MTXConcat(inverseCameraMatrix, cameraModelMatrix, textureProjectionMatrix);
        MTXConcat(lightProjectionMatrix, textureProjectionMatrix, modelViewMatrix);
        GXLoadTexMtxImm(modelViewMatrix, 0x1E, GX_MTX3x4);
        GXSetArray(GX_VA_POS, work->fanVertices[vertexSetIndex], 0xC);
        GXSetArray(GX_VA_CLR0, &work->playerColors[vertexSetIndex], 4);
        GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, 0xE);
        GXPosition1x8(0);
        GXPosition1x8(0);
        for (vertexIndex = 1; vertexIndex < 0xE; vertexIndex++) {
            GXColor1x8(vertexIndex);
            GXColor1x8(1);
        }
        MTXTrans(modelViewMatrix, work->playerPositions[playerIndex].x, 0.0f, work->playerPositions[playerIndex].z);
        MTXConcat(Hu3DCameraMtx, modelViewMatrix, cameraModelMatrix);
        GXLoadPosMtxImm(cameraModelMatrix, 0);
        MTXConcat(inverseCameraMatrix, cameraModelMatrix, textureProjectionMatrix);
        MTXConcat(lightProjectionMatrix, textureProjectionMatrix, modelViewMatrix);
        GXLoadTexMtxImm(modelViewMatrix, 0x1E, GX_MTX3x4);
        GXSetArray(GX_VA_POS, work->fanVertices[vertexSetIndex + 1], 0xC);
        GXSetArray(GX_VA_CLR0, &work->playerColors[vertexSetIndex], 4);
        GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, 0xE);
        GXPosition1x8(0);
        GXPosition1x8(0);
        for (vertexIndex = 1; vertexIndex < 0xE; vertexIndex++) {
            GXColor1x8(vertexIndex);
            GXColor1x8(1);
        }
    }
}

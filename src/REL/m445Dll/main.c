#include "REL/executor.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/mapspace.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#include "ext_math.h"

typedef struct {
    /* 0x00 */ s16 modelId;
    /* 0x02 */ s16 motions[3];
    /* 0x08 */ char reservedModelMetadata[4];
    /* 0x0C */ s16 unusedMotionId;
    /* 0x0E */ s16 alternateMotionId;
    /* 0x10 */ s16 secondAlternateMotionId;
    /* 0x12 */ char reservedMotionMetadata[2];
    /* 0x14 */ s32 furnitureIndex;
    /* 0x18 */ s32 mapObjectIndex;
    /* 0x1C */ s32 interactionPhase;
    /* 0x20 */ s32 itemInteractionStates[2];
    /* 0x28 */ s32 firstItemPlayerIndex;
    /* 0x2C */ s32 secondItemPlayerIndex;
    /* 0x30 */ s32 interactionState;
    /* 0x34 */ s32 isAvailable;
    /* 0x38 */ s32 isRewardRound;
    /* 0x3C */ s32 itemOwnerIndex;
    /* 0x40 */ s32 secondItemOwnerIndex;
    /* 0x44 */ s32 interactionFrameCount;
    /* 0x48 */ float worldX;
    /* 0x4C */ float worldZ;
    /* 0x50 */ float interactionX[2];
    /* 0x58 */ float interactionZ[2];
} FurnitureData; // Size 0x60

typedef struct {
    /* 0x00 */ s16 primaryModelId;
    /* 0x02 */ s16 tertiaryModelId;
    /* 0x04 */ s16 secondaryModelId;
    /* 0x06 */ s16 quaternaryModelId;
    /* 0x08 */ s16 motions[16];
    /* 0x28 */ s16 selectedModelId;
    /* 0x2A */ char reservedModelMetadata[2];
    /* 0x2C */ s32 furnitureIndex;
    /* 0x30 */ s32 selectedPlayerIndex;
    /* 0x34 */ s32 interactionState;
} FurnitureItemData; // Size 0x38

typedef struct {
    /* 0x00 */ s16 modelId;
    /* 0x02 */ s16 itemModelId;
    /* 0x04 */ s16 spriteGroupId;
    /* 0x06 */ s16 heldItemModelId;
    /* 0x08 */ s16 motions[8];
    /* 0x18 */ s32 playerIndex;
    /* 0x1C */ s32 interactionState;
    /* 0x20 */ s32 padIndex;
    /* 0x24 */ s32 movementState;
    /* 0x28 */ s32 collisionEnabled;
    /* 0x2C */ s32 collisionIndex;
    /* 0x30 */ s32 itemCount;
    /* 0x34 */ float posX;
    /* 0x38 */ float posY;
    /* 0x3C */ float posZ;
    /* 0x40 */ float previousPosX;
    /* 0x44 */ float previousPosZ;
    /* 0x48 */ float rotX;
    /* 0x4C */ float rotY;
    /* 0x50 */ float rotZ;
    /* 0x54 */ float collisionRadius;
    /* 0x58 */ float previousSpeed;
    /* 0x5C */ float moveSpeed;
    /* 0x60 */ float initialRotY;
    /* 0x64 */ char reservedMovementMetadata[0xC];
    /* 0x70 */ Vec movementVelocity;
    /* 0x7C */ s8 stickX;
    /* 0x7D */ s8 stickY;
    /* 0x7E */ s8 triggerR;
    /* 0x7F */ s8 triggerL;
    /* 0x80 */ u16 buttonsHeld;
    /* 0x82 */ u16 buttonsPressed;
    /* 0x84 */ float targetPosX;
    /* 0x88 */ float targetPosZ;
    /* 0x8C */ char reservedTargetMetadata[4];
    /* 0x90 */ s32 animationState;
    /* 0x94 */ char reservedCollisionMetadata[4];
    /* 0x98 */ s32 targetPlayerIndex;
} PlayerData; // Size 0x9C

typedef struct {
    /* 0x00 */ char reservedEffectMetadata[0x18];
    /* 0x18 */ float particleSize;
    /* 0x1C */ char reservedParticleMetadata[4];
    /* 0x20 */ float particleAlpha;
} ParticleEffectData; // Size unknown

typedef struct {
    /* 0x00 */ ParticleEffectData *particleData;
    /* 0x04 */ s32 particleCount;
    /* 0x08 */ s32 particleIndex;
    /* 0x0C */ s32 particleFlags;
} ParticleAnimationData; // Size unknown

void ObjectSetup(void);
void PlaySoundAtPosition(float worldX, float worldZ, s32 soundEffectId);
void MapObjectNoop(OMOBJ *mapObject);
void MapObjectUpdate(OMOBJ *mapObject);
BOOL IsPlayerCharacterModel(s32 playerIndex);
void InitializeSceneModels(void);
float NormalizeAngle(float angle);
FurnitureItemData *FindFreeFurnitureItem(void);
void FreeParticleAnimationData(void);
void AnimateParticleFade(void);
s32 GetFurnitureTargetCount(s32 elapsedSeconds);
void SpawnFurnitureItems(void);
void FurnitureItemProcess(void);
s8 GetPlayerStickValue(s32 playerIndex, s32 verticalAxis);
void ScaleVectorXZ(Vec *vector, float angle, float magnitudeScale);
s8 GetMovementStickValue(s32 playerIndex, s32 verticalAxis);
u8 GetTriggerValue(s32 playerIndex, s32 rightTrigger);
u16 GetPressedButtons(s32 playerIndex);
u16 GetHeldButtons(s32 playerIndex);
s32 FindOverlappingPlayerAtPosition(PlayerData *playerData, float positionX, float positionY, float positionZ);
s32 FindOverlappingPlayer(PlayerData *playerData, s32 *overlappingPlayerCount);
void ResolvePlayerCollision(PlayerData *playerData);
float InterpolateAngle(float startAngle, float endAngle, float interpolation);
BOOL IsPlayerNearPosition(PlayerData *playerData, float targetX, float targetZ);
void UpdateComputerPlayerInput(PlayerData *playerData);
void ShowPlayerSprites(void);
void HidePlayerSprites(void);
void SetPlayerItemCountDisplay(PlayerData *playerData, s32 itemCount);
void UpdatePlayerExitMovement(PlayerData *playerData);
void PlayerMovementProcess(void);
void UpdatePlayerParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void PlayEndingSequence(void);
void AnimateEndingCamera(void);
void WaitForSceneExit(void);
void RunMinigameSequence(void);

s16 sceneLightId;
char sceneLightAlignmentPadding[2]; // Unused.
s16 clockMotionId;
s16 endingModelId;
s32 sceneState;
s32 elapsedFrames;
s32 furnitureSpawnTimes[3];
s32 furnitureSpawnCount;
s32 winningPlayerIndex;
char sceneStateAlignmentPadding[4]; // Unused.
PlayerData players[2];
FurnitureData furniture[8];
FurnitureItemData furnitureItems[10];
HUPROCESS *objectManager;
char objectManagerAlignmentPadding[0x24]; // Unused.
OMOBJ *cameraObject;
Vec shadowPosition;
Vec shadowDirection;
Vec shadowTarget;
s32 exitingPlayerIndex;
float exitDirection;

s32 characterModelIndices[] = { 0, 1, 2, 3, 4, 5, 6, 7 };

// Unused.
s32 characterMotionDirectories[] = { DATADIR_MARIOMOT, DATADIR_LUIGIMOT, DATADIR_PEACHMOT, DATADIR_YOSHIMOT, DATADIR_WARIOMOT, DATADIR_DONKEYMOT, DATADIR_DAISYMOT,
    DATADIR_WALUIGIMOT };

s32 playerModelDataNumbers[]
    = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), DATA_MAKE_NUM(DATADIR_MARIOMOT, 2), DATA_MAKE_NUM(DATADIR_MARIOMOT, 3), DATA_MAKE_NUM(DATADIR_M445, 0),
          DATA_MAKE_NUM(DATADIR_M445, 8), DATA_MAKE_NUM(DATADIR_M445, 16), DATA_MAKE_NUM(DATADIR_MARIOMOT, 75), DATA_MAKE_NUM(DATADIR_MARIOMOT, 24) };

Vec playerStartPositions[] = { { -100.0f, 75.0f, 60.0f }, { 100.0f, 75.0f, -60.0f } };

Vec playerStartRotations[] = { { 0.0f, 0.0f, -79.5f }, { 180.0f, 0.0f, -100.5f } };

float furnitureInteractionPositions[][2] = { { 471.0f, 280.0f }, { -461.5f, 299.5f }, { 271.7f, -482.6f }, { -373.1f, -457.1f }, { 462.8f, -128.5f },
    { 0.0f, -495.4f }, { -40.8f, 632.3f }, { -477.7f, -110.0f } };

float furnitureItemPositions[][2] = { { 467.0f, 393.0f }, { -461.5f, 395.5f }, { 367.7f, -492.1f }, { -268.9f, -467.0f }, { 462.8f, -128.5f },
    { 0.0f, -495.4f }, { -40.8f, 632.3f }, { -477.7f, -110.0f } };

s32 furniturePositions[][2] = { { 360, 306 }, { -360, 341 }, { 326, -360 }, { -326, -360 }, { 321, -122 }, { 0, -332 }, { -50, 500 }, { -334, -110 } };

char *furnitureItemHookNames[][2] = { { "tansu_w_itemhook1", "tansu_w_itemhook2" }, { "sidebord_itemhook1", "sidebord_itemhook2" },
    { "pinkshelf_itemhook1", "pinkshelf_itemhook2" }, { "green_itemhook1", "green_itemhook2" }, { "bookshelf_itemhook", "bookshelf_itemhook" },
    { "clock_itemhook", "clock_itemhook" }, { "book_itemhook", "book_itemhook" }, { "tv_itemhook", "tv_itemhook" } };

s32 furnitureItemCounts[] = { 2, 2, 2, 2, 1, 1, 1, 1 };

void ObjectSetup(void)
{
    objectManager = omInitObjMan(50, 0x2000);
    omGameSysInit(objectManager);
    HuWinInit(1);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 40.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 640, 480);
    cameraObject = omAddObjEx(objectManager, 32730, 0, 0, -1, omOutViewMulti);
    cameraObject->work[0] = 1;
    CRotM[0].x = -12.5f;
    CRotM[0].y = 0.0f;
    CRotM[0].z = 0.0f;
    CenterM[0].x = 0.0f;
    CenterM[0].y = 345.0f;
    CenterM[0].z = -250.0f;
    CZoomM[0] = 520.0f;
    Hu3DLighInit();
    sceneLightId = Hu3DGLightCreate(10000.0f, 0.0f, 10000.0f, -1.0f, 0.0f, -1.0f, 0xFF, 0xFF, 0xFF);
    Hu3DGLightPosAimSet(sceneLightId, 0.0f, 4330.13f, 2500.0f, 0.0f, 0.0f, 0.0f);
    Hu3DGLightInfinitytSet(sceneLightId);
    shadowPosition.x = -200.0f;
    shadowPosition.y = 4000.0f;
    shadowPosition.z = 0.0f;
    shadowDirection.x = shadowDirection.z = 0.0f;
    shadowDirection.y = 1.0f;
    shadowTarget.x = 0.0f;
    shadowTarget.y = 0.0f;
    shadowTarget.z = 0.0f;
    Hu3DShadowCreate(30.0f, 20.0f, 10000.0f);
    Hu3DShadowTPLvlSet(0.3f);
    Hu3DShadowPosSet(&shadowPosition, &shadowDirection, &shadowTarget);
    Hu3DBGColorSet(0, 0, 0);
    InitializeSceneModels();
    HuPrcChildCreate(RunMinigameSequence, 0x1000, 0x2000, 0, HuPrcCurrentGet());
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
}

void PlaySoundAtPosition(float worldX, float worldZ, s32 soundEffectId)
{
    Vec soundPosition;

    soundPosition.x = worldX;
    soundPosition.y = 0.0f;
    soundPosition.z = worldZ;
    HuAudFXEmiterPlay(soundEffectId, &soundPosition);
}

void MapObjectNoop(OMOBJ *mapObject) { }

void MapObjectUpdate(OMOBJ *mapObject)
{
    mapObject->objFunc = MapObjectNoop;
}

BOOL IsPlayerCharacterModel(s32 playerIndex)
{
    return (GWPlayerCfg[playerIndex].character == 9);
}

void InitializeSceneModels(void)
{
    float playerRotationY;
    float playerPosX;
    float playerPosZ;
    FurnitureData *furnitureData;
    FurnitureItemData *itemData;
    PlayerData *playerData;
    OMOBJ *mapObject;
    HUPROCESS *animationProcess;
    ANIMDATA *particleAnimation;
    ANIMDATA *statusSpriteAnimations[4];
    void *modelData;
    s16 modelId;
    s32 dataNumber;
    s32 motionDataNumber;
    Vec listenerPosition;
    Vec listenerDirection;
    s32 index;
    s32 motionIndex;

    nMap = 0;
    HuAudSndGrpSet(0x47);
    listenerPosition.x = 0.0f;
    listenerPosition.y = 998.0f;
    listenerPosition.z = 1551.0f;
    listenerDirection.x = 0.0f;
    listenerDirection.y = -0.5f;
    listenerDirection.z = -1.0f;
    HuAudFXListnerSetEX(&listenerPosition, &listenerDirection, 10000.0f, 34.0f*(1.0f/REFRESH_RATE), 0.0f, 300.0f, 300.0f);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 24), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    Hu3DModelLayerSet(modelId, 0);
    Hu3DModelShadowMapSet(modelId);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 25), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    mapObject = omAddObjEx(objectManager, 0x100, 1, 0, -1, MapObjectUpdate);
    MapObject[nMap++] = mapObject;
    mapObject->mdlId[0] = modelId;
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 26), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    Hu3DModelLayerSet(modelId, 0);
    Hu3DModelShadowSet(modelId);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 27), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    Hu3DModelLayerSet(modelId, 0);
    Hu3DModelShadowMapSet(modelId);
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 28), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(modelData);
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    mapObject = omAddObjEx(objectManager, 0x100, 1, 0, -1, MapObjectUpdate);
    MapObject[nMap++] = mapObject;
    mapObject->mdlId[0] = modelId;
    modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 100), HU_MEMNUM_OVL, HEAP_MODEL);
    endingModelId = Hu3DModelCreate(modelData);
    Hu3DModelAttrSet(endingModelId, HU3D_ATTR_DISPOFF);
    for (index = 0; index < 8; index++) {
        furnitureData = &furniture[index];
        furnitureData->furnitureIndex = index;
        furnitureData->interactionState = 0;
        dataNumber = DATA_MAKE_NUM(DATADIR_M445, 38 + index * 5);
        modelData = HuDataSelHeapReadNum(dataNumber, HU_MEMNUM_OVL, HEAP_MODEL);
        modelId = Hu3DModelCreate(modelData);
        furnitureData->modelId = modelId;
        for (motionIndex = 0, dataNumber++; motionIndex < 3; motionIndex++) {
            modelData = HuDataSelHeapReadNum(dataNumber++, HU_MEMNUM_OVL, HEAP_MODEL);
            furnitureData->motions[motionIndex] = Hu3DJointMotion(modelId, modelData);
        }
        Hu3DModelShadowSet(modelId);
        Hu3DMotionSet(modelId, furnitureData->motions[0]);
        Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
        furnitureData->worldX = furniturePositions[index][0];
        furnitureData->worldZ = furniturePositions[index][1];
        furnitureData->interactionX[0] = furnitureInteractionPositions[index][0];
        furnitureData->interactionZ[0] = furnitureInteractionPositions[index][1];
        furnitureData->interactionX[1] = furnitureItemPositions[index][0];
        furnitureData->interactionZ[1] = furnitureItemPositions[index][1];
        if (index == 5) {
            modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 81), HU_MEMNUM_OVL, HEAP_MODEL);
            clockMotionId = Hu3DJointMotion(modelId, modelData);
            Hu3DMotionSet(modelId, clockMotionId);
            Hu3DMotionTimeSet(modelId, 61.0f);
            Hu3DModelAttrSet(modelId, HU3D_MOTATTR_PAUSE);
        }
        if (index == 6) {
            modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 78), HU_MEMNUM_OVL, HEAP_MODEL);
            furnitureData->alternateMotionId = Hu3DJointMotion(modelId, modelData);
        }
        if (index == 7) {
            modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 79), HU_MEMNUM_OVL, HEAP_MODEL);
            furnitureData->alternateMotionId = Hu3DJointMotion(modelId, modelData);
            modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 80), HU_MEMNUM_OVL, HEAP_MODEL);
            furnitureData->secondAlternateMotionId = Hu3DJointMotion(modelId, modelData);
        }
        furnitureData->mapObjectIndex = nMap;
        modelData = HuDataSelHeapReadNum(dataNumber, HU_MEMNUM_OVL, HEAP_MODEL);
        modelId = Hu3DModelCreate(modelData);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
        mapObject = MapObject[nMap++] = omAddObjEx(objectManager, 0x100, 1, 0, -1, MapObjectUpdate);
        mapObject->mdlId[0] = modelId;
        mapObject->trans.x = furnitureData->worldX;
        mapObject->trans.y = 0.0f;
        mapObject->trans.z = furnitureData->worldZ;
        animationProcess = HuPrcChildCreate(FurnitureItemProcess, 0x2000, 0x2000, 0, HuPrcCurrentGet());
        animationProcess->user_data = furnitureData;
    }
    HuPrcChildCreate(SpawnFurnitureItems, 0x2000, 0x2000, 0, HuPrcCurrentGet());
    for (index = 0; index < 10; index++) {
        itemData = &furnitureItems[index];
        itemData->furnitureIndex = index;
        itemData->selectedPlayerIndex = -1;
        modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 82), HU_MEMNUM_OVL, HEAP_MODEL);
        modelId = Hu3DModelCreate(modelData);
        itemData->primaryModelId = modelId;
        for (motionIndex = 0; motionIndex < 4; motionIndex++) {
            modelData = HuDataSelHeapReadNum(motionIndex + DATA_MAKE_NUM(DATADIR_M445, 83), HU_MEMNUM_OVL, HEAP_MODEL);
            itemData->motions[motionIndex] = Hu3DJointMotion(modelId, modelData);
        }
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
        modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 82), HU_MEMNUM_OVL, HEAP_MODEL);
        modelId = Hu3DModelCreate(modelData);
        itemData->secondaryModelId = modelId;
        for (motionIndex = 0; motionIndex < 4; motionIndex++) {
            modelData = HuDataSelHeapReadNum(motionIndex + DATA_MAKE_NUM(DATADIR_M445, 87), HU_MEMNUM_OVL, HEAP_MODEL);
            itemData->motions[motionIndex + 4] = Hu3DJointMotion(modelId, modelData);
        }
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
        modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 91), HU_MEMNUM_OVL, HEAP_MODEL);
        modelId = Hu3DModelCreate(modelData);
        itemData->tertiaryModelId = modelId;
        for (motionIndex = 0; motionIndex < 4; motionIndex++) {
            modelData = HuDataSelHeapReadNum(motionIndex + DATA_MAKE_NUM(DATADIR_M445, 92), HU_MEMNUM_OVL, HEAP_MODEL);
            itemData->motions[motionIndex + 8] = Hu3DJointMotion(modelId, modelData);
        }
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
        modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 91), HU_MEMNUM_OVL, HEAP_MODEL);
        modelId = Hu3DModelCreate(modelData);
        itemData->quaternaryModelId = modelId;
        for (motionIndex = 0; motionIndex < 4; motionIndex++) {
            modelData = HuDataSelHeapReadNum(motionIndex + DATA_MAKE_NUM(DATADIR_M445, 96), HU_MEMNUM_OVL, HEAP_MODEL);
            itemData->motions[motionIndex + 12] = Hu3DJointMotion(modelId, modelData);
        }
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    }
    for (index = 0; index < 2; index++) {
        playerData = &players[index];
        playerData->playerIndex = index;
        playerData->interactionState = index;
        playerData->padIndex = GWPlayerCfg[index].pad_idx;
        if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
            playerData->modelId = CharModelCreate(characterModelIndices[GWPlayerCfg[index].character], 2);
        }
        else {
            modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 29), HU_MEMNUM_OVL, HEAP_MODEL);
            playerData->modelId = Hu3DModelCreate(modelData);
        }
        Hu3DModelLayerSet(playerData->modelId, 0);
        CharModelStepFxSet(characterModelIndices[GWPlayerCfg[index].character], 1);
        CharEffectLayerSet(3);
        playerData->rotX = playerData->rotZ = 0.0f;
        if (index != 0) {
            playerRotationY = playerData->rotY = NormalizeAngle(-90.0f);
        }
        else {
            playerData->rotY = playerRotationY = 90.0f;
        }
        playerData->initialRotY = playerData->rotY;
        if (index != 0) {
            playerPosX = playerData->posX = 160.0f;
        }
        else {
            playerPosX = playerData->posX = -160.0f;
        }
        playerData->posY = 0.0f;
        if (index != 0) {
            playerPosZ = playerData->posZ = -45.0f;
        }
        else {
            playerPosZ = playerData->posZ = 45.0f;
        }
        playerData->collisionRadius = 50.0f;
        playerData->moveSpeed = playerData->previousSpeed = 0.0f;
        HuSetVecF(&playerData->movementVelocity, 0.0, 0.0, 0.0);
        Hu3DModelPosSet(playerData->modelId, playerData->posX, playerData->posY, playerData->posZ);
        Hu3DModelRotSet(playerData->modelId, playerData->rotX, playerData->rotY, playerData->rotZ);
        Hu3DModelShadowSet(playerData->modelId);
        modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 37), HU_MEMNUM_OVL, HEAP_MODEL);
        modelId = Hu3DModelCreate(modelData);
        playerData->heldItemModelId = modelId;
        Hu3DModelLayerSet(modelId, 0);
        Hu3DModelShadowSet(modelId);
        Hu3DModelPosSet(modelId, playerStartPositions[index].x, playerStartPositions[index].y, playerStartPositions[index].z);
        Hu3DModelRotSet(modelId, playerStartRotations[index].x, playerStartRotations[index].y, playerStartRotations[index].z);
        modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 101), HU_MEMNUM_OVL, HEAP_MODEL);
        modelId = Hu3DModelCreate(modelData);
        playerData->itemModelId = modelId;
        Hu3DMotionSpeedSet(modelId, 1.4f);
        Hu3DModelLayerSet(modelId, 1);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
        if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
            for (motionIndex = 0; motionIndex < 8; motionIndex++) {
                motionDataNumber = playerModelDataNumbers[motionIndex];
                if ((motionDataNumber & ~0xFFFF) == DATADIR_M445) {
                    motionDataNumber += GWPlayerCfg[index].character;
                }
                playerData->motions[motionIndex] = CharMotionCreate(characterModelIndices[GWPlayerCfg[index].character], motionDataNumber);
            }
            CharMotionSet(characterModelIndices[GWPlayerCfg[index].character], playerData->motions[0]);
        }
        else {
            for (motionIndex = 0; motionIndex < 7; motionIndex++) {
                modelData = HuDataSelHeapReadNum(motionIndex + DATA_MAKE_NUM(DATADIR_M445, 30), HU_MEMNUM_OVL, HEAP_MODEL);
                playerData->motions[motionIndex] = Hu3DJointMotion(playerData->modelId, modelData);
            }
            Hu3DMotionSet(playerData->modelId, playerData->motions[0]);
            CharNpcDustSet(playerData->modelId, playerData->motions[1], 0, 9);
            CharNpcDustSet(playerData->modelId, playerData->motions[2], 1, 9);
        }
        Hu3DModelAttrSet(playerData->modelId, HU3D_MOTATTR_LOOP);
        if (index == 0) {
            modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 22), HU_MEMNUM_OVL, HEAP_MODEL);
            statusSpriteAnimations[0] = HuSprAnimRead(modelData);
            modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 23), HU_MEMNUM_OVL, HEAP_MODEL);
            statusSpriteAnimations[1] = HuSprAnimRead(modelData);
            modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 51), HU_MEMNUM_OVL, HEAP_MODEL);
            statusSpriteAnimations[3] = HuSprAnimRead(modelData);
        }
        if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
            modelData = HuDataSelHeapReadNum(GWPlayerCfg[index].character + DATADIR_MGCONST, HU_MEMNUM_OVL, HEAP_MODEL);
        }
        else {
            modelData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M445, 102), HU_MEMNUM_OVL, HEAP_MODEL);
        }
        statusSpriteAnimations[2] = HuSprAnimRead(modelData);
        playerData->spriteGroupId = HuSprGrpCreate(5);
        modelId = HuSprCreate(statusSpriteAnimations[0], 0x1000, 0);
        HuSprGrpMemberSet(playerData->spriteGroupId, 0, modelId);
        HuSprPosSet(playerData->spriteGroupId, 0, 0.0f, 0.0f);
        modelId = HuSprCreate(statusSpriteAnimations[1], 0x1001, 0);
        HuSprGrpMemberSet(playerData->spriteGroupId, 1, modelId);
        HuSprPosSet(playerData->spriteGroupId, 1, 0.0f, 0.0f);
        HuSprTPLvlSet(playerData->spriteGroupId, 1, 0.5f);
        HuSprColorSet(playerData->spriteGroupId, 1, 0, 0, 0);
        modelId = HuSprCreate(statusSpriteAnimations[2], 0x1000, 0);
        HuSprGrpMemberSet(playerData->spriteGroupId, 2, modelId);
        HuSprPosSet(playerData->spriteGroupId, 2, -36.0f, -2.0f);
        modelId = HuSprCreate(statusSpriteAnimations[3], 0x1000, 0);
        HuSprGrpMemberSet(playerData->spriteGroupId, 3, modelId);
        HuSprPosSet(playerData->spriteGroupId, 3, 16.0f, 2.0f);
        modelId = HuSprCreate(statusSpriteAnimations[3], 0x1000, 0);
        HuSprGrpMemberSet(playerData->spriteGroupId, 4, modelId);
        HuSprPosSet(playerData->spriteGroupId, 4, -4.0f, 2.0f);
        HuSprAttrSet(playerData->spriteGroupId, 4, 4);
        HuSprGrpPosSet(playerData->spriteGroupId, index * 438 + 73, 61.0f);
        playerData->itemCount = 0;
        SetPlayerItemCountDisplay(playerData, playerData->itemCount);
        HidePlayerSprites();
        CharMotionDataClose(characterModelIndices[GWPlayerCfg[index].character]);
        animationProcess = HuPrcChildCreate(PlayerMovementProcess, 0x2000, 0x3000, 0, HuPrcCurrentGet());
        animationProcess->user_data = playerData;
    }
    particleAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 3), HU_MEMNUM_OVL));
    modelId = Hu3DParticleCreate(particleAnimation, 3);
    Hu3DModelPosSet(modelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(modelId, 1.0f, 1.0f, 1.0f);
    Hu3DModelLayerSet(modelId, 4);
    Hu3DParticleColSet(modelId, 0xFF, 0xFF, 0xFF);
    Hu3DParticleScaleSet(modelId, 0.0f);
    Hu3DParticleHookSet(modelId, UpdatePlayerParticles);
    HuDataDirClose(DATADIR_EFFECT);
    furnitureSpawnCount = 0;
    furnitureSpawnTimes[0] = frandmod(25);
    furnitureSpawnTimes[1] = frandmod(25) + 30;
    furnitureSpawnTimes[2] = 1000;
    HuPrcChildCreate(WaitForSceneExit, 0x1000, 0x1000, 0, HuPrcCurrentGet());
}

float NormalizeAngle(float angle)
{
    while (angle >= 360.0f || angle < 0.0f) {
        if (angle >= 360.0f) {
            angle -= 360.0f;
        }
        if (angle < 0.0f) {
            angle += 360.0f;
        }
    }
    return angle;
}

FurnitureItemData *FindFreeFurnitureItem(void)
{
    FurnitureItemData *freeFurnitureItem;
    s32 furnitureItemIndex;

    for (furnitureItemIndex = 0; furnitureItemIndex < 10; furnitureItemIndex++) {
        freeFurnitureItem = &furnitureItems[furnitureItemIndex];
        if (freeFurnitureItem->selectedPlayerIndex == -1) {
            break;
        }
    }
    if (furnitureItemIndex >= 10) {
        freeFurnitureItem = NULL;
    }
    return freeFurnitureItem;
}

void FreeParticleAnimationData(void)
{
    ParticleAnimationData *particleProcessData;

    particleProcessData = HuPrcCurrentGet()->user_data;
    HuMemDirectFree(particleProcessData);
}

void AnimateParticleFade(void)
{
    float horizontalStep;
    float verticalStep;
    ParticleAnimationData *particleProcessData;
    s32 remainingFrames;

    particleProcessData = HuPrcCurrentGet()->user_data;
    HuPrcDestructorSet(FreeParticleAnimationData);
    switch (particleProcessData->particleCount) {
        case 0:
            horizontalStep = -140.0f / particleProcessData->particleFlags;
            verticalStep = 0.0f / particleProcessData->particleFlags;
            break;
        default:
            horizontalStep = 0.0f;
            verticalStep = 0.0f;
            break;
    }
    for (remainingFrames = particleProcessData->particleFlags; remainingFrames != 0; remainingFrames--) {
        particleProcessData->particleData->particleSize += horizontalStep;
        particleProcessData->particleData->particleAlpha += verticalStep;
        HuPrcVSleep();
    }
    while (particleProcessData->particleIndex == 0) {
        HuPrcVSleep();
    }
    for (remainingFrames = particleProcessData->particleFlags; remainingFrames != 0; remainingFrames--) {
        particleProcessData->particleData->particleSize -= horizontalStep;
        particleProcessData->particleData->particleAlpha -= verticalStep;
        HuPrcVSleep();
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

s32 GetFurnitureTargetCount(s32 elapsedSeconds)
{
    s32 targetOccupantCount;

    if (elapsedSeconds < 20) {
        targetOccupantCount = 3;
    }
    else if (elapsedSeconds < 40) {
        targetOccupantCount = 4;
    }
    else {
        targetOccupantCount = 5;
    }
    return targetOccupantCount;
}

void SpawnFurnitureItems(void)
{
    FurnitureData *furnitureData;
    s32 previousFurnitureIndex;
    s32 candidateFurnitureIndex;
    s32 selectionCountdown;
    s32 activeFurnitureCount;
    s32 furnitureIndex;

    previousFurnitureIndex = -1;
    candidateFurnitureIndex = 0;
    while (sceneState < 3) {
        HuPrcVSleep();
    }
    while (TRUE) {
        if (sceneState >= 4) {
            break;
        }
        for (furnitureIndex = 0, activeFurnitureCount = 0; furnitureIndex < 8; furnitureIndex++) {
            furnitureData = &furniture[furnitureIndex];
            if (furnitureData->isAvailable != 0) {
                activeFurnitureCount++;
            }
        }
        if (activeFurnitureCount < GetFurnitureTargetCount(elapsedFrames / REFRESH_RATE)) {
            selectionCountdown = frandmod(8) + 1;
            furnitureIndex = 0;
            while (TRUE) {
                furnitureData = &furniture[furnitureIndex];
                if (furnitureData->isAvailable == 0) {
                    selectionCountdown--;
                }
                if (selectionCountdown == 0) {
                    break;
                }
                furnitureIndex++;
                if (furnitureIndex >= 8) {
                    furnitureIndex = 0;
                }
            }
            furnitureData->isAvailable = 1;
            HuPrcSleep(60);
        }
        HuPrcVSleep();
    }
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

void FurnitureItemProcess(void)
{
    FurnitureData *furnitureData;
    FurnitureItemData *firstItemData;
    FurnitureItemData *secondItemData;
    s32 itemIndex;
    s32 spawnedItems;

    furnitureData = HuPrcCurrentGet()->user_data;
    furnitureData->interactionPhase = 0;
    furnitureData->isAvailable = 0;
    while (TRUE) {
        while (furnitureData->isAvailable == 0) {
            HuPrcVSleep();
        }
        furnitureData->itemInteractionStates[0] = furnitureData->itemInteractionStates[1] = 0;
        furnitureData->unusedMotionId = 0;
        if (furnitureData->furnitureIndex == 5 || furnitureData->furnitureIndex == 7) {
            furnitureData->unusedMotionId = 4;
        }
        firstItemData = FindFreeFurnitureItem();
        firstItemData->selectedPlayerIndex = furnitureData->furnitureIndex;
        firstItemData->interactionState = 0;
        furnitureData->itemOwnerIndex = firstItemData->furnitureIndex;
        furnitureData->secondItemOwnerIndex = -1;
        if (furnitureSpawnTimes[furnitureSpawnCount] < elapsedFrames / REFRESH_RATE) {
            furnitureData->isRewardRound = 1;
            if (furnitureItemCounts[furnitureData->furnitureIndex] == 1) {
                furnitureSpawnCount++;
            }
            if (furnitureData->furnitureIndex == 5 || furnitureData->furnitureIndex == 7) {
                firstItemData->selectedModelId = firstItemData->secondaryModelId;
            }
            else {
                firstItemData->selectedModelId = firstItemData->primaryModelId;
            }
        }
        else {
            furnitureData->isRewardRound = 0;
            if (furnitureData->furnitureIndex == 5 || furnitureData->furnitureIndex == 7) {
                firstItemData->selectedModelId = firstItemData->quaternaryModelId;
            }
            else {
                firstItemData->selectedModelId = firstItemData->tertiaryModelId;
            }
            furnitureData->unusedMotionId += 8;
        }
        Hu3DModelAttrReset(firstItemData->selectedModelId, HU3D_ATTR_DISPOFF);
        Hu3DModelHookSet(furnitureData->modelId, furnitureItemHookNames[furnitureData->furnitureIndex][0], firstItemData->selectedModelId);
        Hu3DMotionSet(firstItemData->selectedModelId, firstItemData->motions[furnitureData->unusedMotionId]);
        if (furnitureItemCounts[furnitureData->furnitureIndex] > 1) {
            secondItemData = FindFreeFurnitureItem();
            secondItemData->selectedPlayerIndex = furnitureData->furnitureIndex;
            secondItemData->interactionState = 0;
            furnitureData->secondItemOwnerIndex = secondItemData->furnitureIndex;
            if (furnitureSpawnTimes[furnitureSpawnCount] < elapsedFrames / REFRESH_RATE) {
                furnitureSpawnCount++;
                if (furnitureData->furnitureIndex == 5 || furnitureData->furnitureIndex == 7) {
                    secondItemData->selectedModelId = secondItemData->secondaryModelId;
                }
                else {
                    secondItemData->selectedModelId = secondItemData->primaryModelId;
                }
            }
            else {
                if (furnitureData->furnitureIndex == 5 || furnitureData->furnitureIndex == 7) {
                    secondItemData->selectedModelId = secondItemData->quaternaryModelId;
                }
                else {
                    secondItemData->selectedModelId = secondItemData->tertiaryModelId;
                }
            }
            Hu3DModelAttrReset(secondItemData->selectedModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelHookSet(furnitureData->modelId, furnitureItemHookNames[furnitureData->furnitureIndex][1], secondItemData->selectedModelId);
            Hu3DMotionSet(secondItemData->selectedModelId, secondItemData->motions[furnitureData->unusedMotionId]);
        }
        furnitureData->interactionPhase = 1;
        furnitureData->interactionFrameCount = 0;
        switch (furnitureData->furnitureIndex) {
            case 0:
            case 1:
            case 3:
                PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x751);
                break;
            case 6:
                PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x753);
                break;
            default:
                PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x752);
                break;
        }
        if (furnitureData->furnitureIndex == 7 && furnitureData->isRewardRound == 0) {
            Hu3DMotionSet(furnitureData->modelId, furnitureData->alternateMotionId);
        }
        else {
            Hu3DMotionSet(furnitureData->modelId, furnitureData->motions[1]);
        }
        Hu3DModelAttrReset(furnitureData->modelId, HU3D_MOTATTR_LOOP | HU3D_MOTATTR_REV);
        while (!Hu3DMotionEndCheck(furnitureData->modelId)) {
            furnitureData->interactionFrameCount++;
            HuPrcVSleep();
        }
        if (sceneState >= 4) {
            break;
        }
        if (furnitureData->furnitureIndex == 7 && furnitureData->isRewardRound == 0) {
            Hu3DMotionSet(furnitureData->modelId, furnitureData->secondAlternateMotionId);
        }
        else if (furnitureData->furnitureIndex == 6 && furnitureData->isRewardRound == 0) {
            Hu3DMotionSet(furnitureData->modelId, furnitureData->alternateMotionId);
        }
        else {
            Hu3DMotionSet(furnitureData->modelId, furnitureData->motions[2]);
        }
        itemIndex = REFRESH_RATE*2.5f;
        Hu3DMotionSet(firstItemData->selectedModelId, firstItemData->motions[furnitureData->unusedMotionId + 1]);
        Hu3DModelAttrSet(firstItemData->selectedModelId, HU3D_MOTATTR_PAUSE);
        if (furnitureData->furnitureIndex == 6) {
            HuPrcSleep(12);
            Hu3DModelAttrReset(firstItemData->selectedModelId, HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
            if (furnitureData->isRewardRound != 0) {
                PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x758);
            }
            else {
                PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x757);
            }
        }
        if (furnitureData->furnitureIndex == 7) {
            HuPrcSleep(11);
            Hu3DModelAttrReset(firstItemData->selectedModelId, HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
            PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x754);
            Hu3DModelAttrReset(endingModelId, HU3D_ATTR_DISPOFF);
            Hu3DMotionTimeSet(endingModelId, 0.0f);
        }
        while (!Hu3DMotionEndCheck(furnitureData->modelId)) {
            furnitureData->interactionFrameCount++;
            HuPrcVSleep();
        }
        if (furnitureData->furnitureIndex != 6) {
            if (furnitureData->isRewardRound != 0) {
                PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x758);
            }
            else {
                PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x757);
            }
            Hu3DModelAttrReset(firstItemData->selectedModelId, HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
            if (furnitureItemCounts[furnitureData->furnitureIndex] > 1) {
                Hu3DMotionSet(secondItemData->selectedModelId, secondItemData->motions[furnitureData->unusedMotionId + 1]);
                Hu3DModelAttrReset(secondItemData->selectedModelId, HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
            }
        }
        while (!Hu3DMotionEndCheck(firstItemData->selectedModelId)) {
            HuPrcVSleep();
        }
        firstItemData->interactionState = 1;
        Hu3DMotionSet(firstItemData->selectedModelId, firstItemData->motions[furnitureData->unusedMotionId + 2]);
        Hu3DModelAttrSet(firstItemData->selectedModelId, HU3D_MOTATTR_LOOP);
        if (furnitureItemCounts[furnitureData->furnitureIndex] > 1) {
            secondItemData->interactionState = 1;
            Hu3DMotionSet(secondItemData->selectedModelId, secondItemData->motions[furnitureData->unusedMotionId + 2]);
            Hu3DModelAttrSet(secondItemData->selectedModelId, HU3D_MOTATTR_LOOP);
        }
        furnitureData->interactionPhase = 2;
        spawnedItems = itemIndex;
        while (TRUE) {
            if (furnitureData->itemInteractionStates[0] == 1) {
                furnitureData->itemInteractionStates[0] = 2;
                Hu3DMotionSet(firstItemData->selectedModelId, firstItemData->motions[furnitureData->unusedMotionId + 3]);
                Hu3DModelAttrReset(firstItemData->selectedModelId, HU3D_MOTATTR_LOOP);
                if (furnitureData->isRewardRound != 0) {
                    if (furnitureData->firstItemPlayerIndex != 0) {
                        PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x75C);
                    }
                    else {
                        PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x75A);
                    }
                }
                else {
                    if (furnitureData->firstItemPlayerIndex != 0) {
                        PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x75B);
                    }
                    else {
                        PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x759);
                    }
                }
            }
            if (furnitureData->itemInteractionStates[1] == 1) {
                furnitureData->itemInteractionStates[1] = 2;
                Hu3DMotionSet(secondItemData->selectedModelId, secondItemData->motions[furnitureData->unusedMotionId + 3]);
                Hu3DModelAttrReset(secondItemData->selectedModelId, HU3D_MOTATTR_LOOP);
                if (furnitureData->isRewardRound != 0) {
                    if (furnitureData->secondItemPlayerIndex != 0) {
                        PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x75C);
                    }
                    else {
                        PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x75A);
                    }
                }
                else {
                    if (furnitureData->secondItemPlayerIndex != 0) {
                        PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x75B);
                    }
                    else {
                        PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x759);
                    }
                }
            }
            if (furnitureData->furnitureIndex == 6 && furnitureData->itemInteractionStates[0] == 2 && Hu3DMotionTimeGet(firstItemData->selectedModelId) == 12.0f) {
                break;
            }
            if (furnitureItemCounts[furnitureData->furnitureIndex] > 1) {
                if (spawnedItems <= 0) {
                    break;
                }
                if (Hu3DMotionEndCheck(firstItemData->selectedModelId) && Hu3DMotionEndCheck(secondItemData->selectedModelId)) {
                    break;
                }
            }
            else {
                if (spawnedItems <= 0) {
                    break;
                }
                if (Hu3DMotionEndCheck(firstItemData->selectedModelId)) {
                    break;
                }
            }
            HuPrcVSleep();
            spawnedItems--;
        }
        furnitureData->interactionPhase = 0;
        firstItemData->interactionState = 0;
        if (furnitureData->furnitureIndex != 6) {
            if (furnitureData->itemInteractionStates[0] == 0) {
                Hu3DMotionSet(firstItemData->selectedModelId, firstItemData->motions[furnitureData->unusedMotionId + 1]);
                Hu3DMotionTimeSet(firstItemData->selectedModelId, Hu3DMotionMaxTimeGet(firstItemData->selectedModelId));
                Hu3DMotionSpeedSet(firstItemData->selectedModelId, 2.0f);
                Hu3DModelAttrSet(firstItemData->selectedModelId, HU3D_MOTATTR_REV);
                Hu3DModelAttrReset(firstItemData->selectedModelId, HU3D_MOTATTR_LOOP);
            }
            if (furnitureItemCounts[furnitureData->furnitureIndex] > 1 && furnitureData->itemInteractionStates[1] == 0) {
                secondItemData->interactionState = 0;
                Hu3DMotionSet(secondItemData->selectedModelId, secondItemData->motions[furnitureData->unusedMotionId + 1]);
                Hu3DMotionTimeSet(secondItemData->selectedModelId, Hu3DMotionMaxTimeGet(secondItemData->selectedModelId));
                Hu3DMotionSpeedSet(secondItemData->selectedModelId, 2.0f);
                Hu3DModelAttrSet(secondItemData->selectedModelId, HU3D_MOTATTR_REV);
                Hu3DModelAttrReset(secondItemData->selectedModelId, HU3D_MOTATTR_LOOP);
            }
            if (furnitureItemCounts[furnitureData->furnitureIndex] > 1) {
                while (!Hu3DMotionEndCheck(firstItemData->selectedModelId) || !Hu3DMotionEndCheck(secondItemData->selectedModelId)) {
                    HuPrcVSleep();
                }
            }
            else {
                while (!Hu3DMotionEndCheck(firstItemData->selectedModelId)) {
                    HuPrcVSleep();
                }
            }
        }
        Hu3DMotionSpeedSet(firstItemData->selectedModelId, 1.0f);
        if (furnitureItemCounts[furnitureData->furnitureIndex] > 1) {
            Hu3DMotionSpeedSet(secondItemData->selectedModelId, 1.0f);
        }
        Hu3DModelAttrSet(furnitureData->modelId, HU3D_MOTATTR_REV);
        if (furnitureData->furnitureIndex == 6 && furnitureData->itemInteractionStates[0] == 0) {
            HuPrcSleep(26);
            Hu3DMotionSet(firstItemData->selectedModelId, firstItemData->motions[furnitureData->unusedMotionId + 1]);
            Hu3DMotionTimeSet(firstItemData->selectedModelId, Hu3DMotionMaxTimeGet(firstItemData->selectedModelId));
            Hu3DMotionSpeedSet(firstItemData->selectedModelId, 2.0f);
            Hu3DModelAttrSet(firstItemData->selectedModelId, HU3D_MOTATTR_REV);
            Hu3DModelAttrReset(firstItemData->selectedModelId, HU3D_MOTATTR_LOOP);
        }
        while (!Hu3DMotionEndCheck(furnitureData->modelId)) {
            HuPrcVSleep();
        }
        firstItemData->selectedPlayerIndex = -1;
        Hu3DModelAttrSet(firstItemData->selectedModelId, HU3D_ATTR_DISPOFF);
        if (furnitureItemCounts[furnitureData->furnitureIndex] > 1) {
            secondItemData->selectedPlayerIndex = -1;
            Hu3DModelAttrSet(secondItemData->selectedModelId, HU3D_ATTR_DISPOFF);
        }
        Hu3DModelHookReset(furnitureData->modelId);
        Hu3DMotionSet(furnitureData->modelId, furnitureData->motions[0]);
        Hu3DModelAttrSet(furnitureData->modelId, HU3D_MOTATTR_LOOP);
        furnitureData->isAvailable = 0;
        HuPrcVSleep();
    }
}

// identical to m411Dll/main.c::fn_1_5D58
s8 GetPlayerStickValue(s32 playerIndex, s32 verticalAxis)
{
    PlayerData *furnitureData;
    s8 stickValue;

    furnitureData = &players[playerIndex];
    if (!GWPlayerCfg[playerIndex].iscom) {
        (verticalAxis != 0) ? (stickValue = HuPadStkY[furnitureData->padIndex]) : (stickValue = HuPadStkX[furnitureData->padIndex]);
    }
    else {
        (verticalAxis != 0) ? (stickValue = furnitureData->stickY) : (stickValue = furnitureData->stickX);
    }
    return stickValue;
}

void ScaleVectorXZ(Vec *vector, float angle, float magnitudeScale)
{
    float scaledMagnitude;

    scaledMagnitude = sqrtf(vector->x * vector->x + vector->z * vector->z);
    scaledMagnitude *= magnitudeScale;
    vector->x = scaledMagnitude * sind(angle);
    vector->z = scaledMagnitude * cosd(angle);
}

// similar to m414Dll/main.c::fn_1_5554
// TODO: update other files (m414 and m411).
s8 GetMovementStickValue(s32 playerIndex, s32 verticalAxis)
{
    PlayerData *playerData;
    s8 stickValue;

    playerData = &players[playerIndex];
    stickValue = 0;
    if (!GWPlayerCfg[playerIndex].iscom && sceneState < 4) {
        (verticalAxis != 0) ? (stickValue = HuPadStkY[playerData->padIndex]) : (stickValue = HuPadStkX[playerData->padIndex]);
    }
    else {
        (verticalAxis != 0) ? (stickValue = playerData->stickY) : (stickValue = playerData->stickX);
    }
    return stickValue;
}

u8 GetTriggerValue(s32 playerIndex, s32 rightTrigger)
{
    PlayerData *playerData;
    s8 triggerValue;

    playerData = &players[playerIndex];
    triggerValue = 0;
    if (sceneState < 4) {
        if (!GWPlayerCfg[playerIndex].iscom) {
            (rightTrigger != 0) ? (triggerValue = HuPadTrigR[playerData->padIndex]) : (triggerValue = HuPadTrigL[playerData->padIndex]);
        }
        else {
            (rightTrigger != 0) ? (triggerValue = playerData->triggerR) : (triggerValue = playerData->triggerL);
        }
    }
    return triggerValue;
}

u16 GetPressedButtons(s32 playerIndex)
{
    PlayerData *playerData;
    u16 pressedButtons;

    playerData = &players[playerIndex];
    pressedButtons = 0;
    if (sceneState < 4) {
        if (!GWPlayerCfg[playerIndex].iscom) {
            pressedButtons = HuPadBtnDown[playerData->padIndex];
        }
        else {
            pressedButtons = playerData->buttonsPressed;
        }
    }
    return pressedButtons;
}

u16 GetHeldButtons(s32 playerIndex)
{
    PlayerData *playerData;
    u16 heldButtons;

    playerData = &players[playerIndex];
    if (sceneState < 4) {
        if (!GWPlayerCfg[playerIndex].iscom) {
            heldButtons = HuPadBtn[playerData->padIndex];
        }
        else {
            heldButtons = playerData->buttonsHeld;
        }
    }
    return heldButtons;
}

s32 FindOverlappingPlayerAtPosition(PlayerData *playerData, float positionX, float positionY, float positionZ)
{
    Vec candidatePosition;
    Vec otherPlayerPosition;
    Vec separation;
    s32 nearestPlayerIndex;
    float distance;
    float nearestDistance;
    PlayerData *otherPlayer;
    s32 playerIndex;

    nearestPlayerIndex = -1;
    for (playerIndex = 0, nearestDistance = 10000.0f; playerIndex < 2; playerIndex++) {
        if (playerIndex == playerData->playerIndex) {
            continue;
        }
        otherPlayer = &players[playerIndex];
        HuSetVecF(&candidatePosition, positionX, positionY, positionZ);
        HuSetVecF(&otherPlayerPosition, otherPlayer->posX, otherPlayer->posY, otherPlayer->posZ);
        HuSubVecF(&separation, &candidatePosition, &otherPlayerPosition);
        if ((distance = sqrtf(separation.x * separation.x + separation.y * separation.y + separation.z * separation.z)) < playerData->collisionRadius + otherPlayer->collisionRadius && distance < nearestDistance) {
            nearestDistance = distance;
            nearestPlayerIndex = playerIndex;
        }
    }
    return nearestPlayerIndex;
}

s32 FindOverlappingPlayer(PlayerData *playerData, s32 *overlappingPlayerCount)
{
    Vec playerPosition;
    Vec otherPlayerPosition;
    Vec separation;
    float distance;
    float nearestDistance;
    PlayerData *otherPlayer;
    s32 nearestPlayerIndex;
    s32 overlappingPlayerCount;
    s32 playerIndex;

    nearestPlayerIndex = -1;
    for (playerIndex = 0, nearestDistance = 10000.0f, overlappingPlayerCount = 0; playerIndex < 2; playerIndex++) {
        if (playerIndex == playerData->playerIndex) {
            continue;
        }
        otherPlayer = &players[playerIndex];
        HuSetVecF(&playerPosition, playerData->posX, playerData->posY, playerData->posZ);
        HuSetVecF(&otherPlayerPosition, otherPlayer->posX, otherPlayer->posY, otherPlayer->posZ);
        HuSubVecF(&separation, &playerPosition, &otherPlayerPosition);
        if ((distance = sqrtf(separation.x * separation.x + separation.y * separation.y + separation.z * separation.z)) < playerData->collisionRadius + otherPlayer->collisionRadius) {
            overlappingPlayerCount++;
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearestPlayerIndex = playerIndex;
            }
        }
    }
    *overlappingPlayerCount = overlappingPlayerCount;
    return nearestPlayerIndex;
}

void ResolvePlayerCollision(PlayerData *playerData)
{
    Vec candidatePosition;
    Vec otherPlayerPosition;
    Vec separation;
    s32 unusedFlag;
    s32 searchSteps;
    s32 otherPlayerIndex;
    float separationRadius;
    float angleDifference;
    float turnDirection;
    float resolvedPosX;
    float resolvedPosZ;
    float maxDisplacement;
    float stepSize;
    float playerDirection;
    float candidateX;
    float candidateZ;
    float otherPlayerDirection;
    PlayerData *otherPlayer;
    PlayerData *thirdPlayer;
    s32 i;

    otherPlayerIndex = FindOverlappingPlayer(playerData, &unusedFlag);
    if (otherPlayerIndex == -1) {
        return;
    }
    playerDirection = NormalizeAngle(atan2d(angle->posX - angle->previousPosX, angle->posZ - angle->previousPosZ));
    resolvedPosX = candidateX = playerData->previousPosX;
    resolvedPosZ = candidateZ = playerData->previousPosZ;
    stepSize = 1.0f;
    searchSteps = 1.0f + 10.0f / stepSize;
    while (searchSteps != 0) {
        candidateX += stepSize * sind(playerDirection);
        candidateZ += stepSize * cosd(playerDirection);
        HuSetVecF(&candidatePosition, candidateX, playerData->posY, candidateZ);
        for (i = 0; i < 2; i++) {
            if (i != playerData->playerIndex) {
                otherPlayer = &players[i];
                HuSetVecF(&otherPlayerPosition, otherPlayer->posX, otherPlayer->posY, otherPlayer->posZ);
                HuSubVecF(&separation, &candidatePosition, &otherPlayerPosition);
                if (sqrtf(separation.x * separation.x + separation.y * separation.y + separation.z * separation.z) < playerData->collisionRadius + otherPlayer->collisionRadius) {
                    break;
                }
            }
        }
        if (i < 2) {
            break;
        }
        AddX = 0.0f;
        AddZ = 0.0f;
        MapWall(playerData->collisionRadius, candidateX, playerData->posY + playerData->collisionRadius, candidateZ);
        if (AddX + AddZ != 0.0f) {
            goto blockEnd;
        }
        resolvedPosX = candidateX;
        resolvedPosZ = candidateZ;
        searchSteps--;
    }
    playerDirection = NormalizeAngle(atan2d(angle->posX - angle->previousPosX, angle->posZ - angle->previousPosZ));
    otherPlayerDirection = NormalizeAngle(atan2d(otherPlayer->posX - angle->previousPosX, otherPlayer->posZ - angle->previousPosZ));
    angleDifference = playerDirection - otherPlayerDirection;
    if (angleDifference > 0.0f) {
        turnDirection = -0.5f;
    }
    else {
        turnDirection = 0.5f;
    }
    if (ABS_INV(playerDirection - otherPlayerDirection) > 180.0f) {
        turnDirection = -turnDirection;
    }
    otherPlayerDirection = NormalizeAngle(atan2d(angle->previousPosX - otherPlayer->posX, angle->previousPosZ - otherPlayer->posZ));
    maxDisplacement = 0.8f * playerData->previousSpeed;
    otherPlayerIndex = otherPlayer->playerIndex;
    separationRadius = playerData->collisionRadius + otherPlayer->collisionRadius + 1.0f;
    while (TRUE) {
        candidateX = otherPlayer->posX + separationRadius * sind(otherPlayerDirection);
        candidateZ = otherPlayer->posZ + separationRadius * cosd(otherPlayerDirection);
        HuSetVecF(&candidatePosition, candidateX, 0.0, candidateZ);
        for (i = 0; i < 2; i++) {
            if (i != playerData->playerIndex && i != otherPlayerIndex) {
                thirdPlayer = &players[i];
                HuSetVecF(&otherPlayerPosition, thirdPlayer->posX, 0.0, thirdPlayer->posZ);
                HuSubVecF(&separation, &candidatePosition, &otherPlayerPosition);
                if (sqrtf(separation.x * separation.x + separation.z * separation.z) < playerData->collisionRadius + thirdPlayer->collisionRadius) {
                    break;
                }
            }
        }
        if (i < 2) {
            break;
        }
        AddX = 0.0f;
        AddZ = 0.0f;
        MapWall(playerData->collisionRadius, candidateX, playerData->posY + playerData->collisionRadius, candidateZ);
        if (AddX + AddZ >= 5.0f) {
            break;
        }
        resolvedPosX = candidateX;
        resolvedPosZ = candidateZ;
        HuSetVecF(&candidatePosition, playerData->previousPosX, 0.0, playerData->previousPosZ);
        HuSetVecF(&otherPlayerPosition, candidateX, 0.0, candidateZ);
        HuSubVecF(&separation, &candidatePosition, &otherPlayerPosition);
        if (sqrtf(separation.x * separation.x + separation.z * separation.z) > maxDisplacement) {
            break;
        }
        otherPlayerDirection += turnDirection;
    }
blockEnd:
    playerData->posX = resolvedPosX;
    playerData->posZ = resolvedPosZ;
}

float InterpolateAngle(float startAngle, float endAngle, float interpolation)
{
    float angleDifference;
    float interpolatedAngle;

    angleDifference = fmod(endAngle - startAngle, 360.0);
    if (0.0f > angleDifference) {
        angleDifference += 360.0f;
    }
    if (180.0f < angleDifference) {
        angleDifference -= 360.0f;
    }
    interpolatedAngle = fmod(startAngle + angleDifference * interpolation, 360.0);
    if (0.0f > interpolatedAngle) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

BOOL IsPlayerNearPosition(PlayerData *playerData, float targetX, float targetZ)
{
    Vec itemPosition;
    Vec targetPosition;
    Vec separation;
    BOOL isInRange;

    isInRange = FALSE;
    HuSetVecF(&itemPosition, playerData->posX + 120.0 * sind(playerData->rotY), 0.0, playerData->posZ + 120.0 * cosd(playerData->rotY));
    HuSetVecF(&targetPosition, targetX, 0.0, targetZ);
    HuSubVecF(&separation, &itemPosition, &targetPosition);
    if (sqrtf(separation.x * separation.x + separation.z * separation.z) < 80.0f) {
        isInRange = TRUE;
    }
    return isInRange;
}

void UpdateComputerPlayerInput(PlayerData *playerData)
{
    Vec playerPosition;
    Vec furniturePosition;
    Vec separation;
    float targetAngle;
    float playerDistanceFromCenter;
    float closestFurnitureDistance;
    float minimumDistance;
    float approachSpeed;
    float distance;
    FurnitureData *furnitureData;
    PlayerData *otherPlayer;
    u32 randomThreshold;
    s32 furnitureIndex;

    playerData->stickX = playerData->stickY = 0;
    playerData->triggerL = playerData->triggerR = 0;
    playerData->buttonsHeld = playerData->buttonsPressed = 0;
    if (playerData->playerIndex != 0) {
        otherPlayer = &players[0];
    }
    else {
        otherPlayer = &players[1];
    }
    switch (GWPlayerCfg[playerData->playerIndex].diff) {
        case 0:
            randomThreshold = 90;
            minimumDistance = 90.0f;
            approachSpeed = 0.4f;
            break;
        case 1:
            randomThreshold = 95;
            minimumDistance = 60.0f;
            approachSpeed = 0.6f;
            break;
        case 2:
            randomThreshold = 99;
            minimumDistance = 30.0f;
            approachSpeed = 0.8f;
            break;
        default:
            randomThreshold = 100;
            minimumDistance = 10.0f;
            approachSpeed = 1.0f;
            break;
    }
    for (furnitureIndex = 0, closestFurnitureDistance = 10000.0f; furnitureIndex < 8; furnitureIndex++) {
        furnitureData = &furniture[furnitureIndex];
        if (furnitureData->interactionPhase >= 1 && !(furnitureData->interactionFrameCount < minimumDistance)) {
            HuSetVecF(&playerPosition, playerData->posX, 0.0, playerData->posZ);
            HuSetVecF(&furniturePosition, furnitureData->worldX, 0.0, furnitureData->worldZ);
            HuSubVecF(&separation, &playerPosition, &furniturePosition);
            distance = sqrtf(separation.x * separation.x + separation.z * separation.z);
            playerDistanceFromCenter = sqrtf(playerData->posX * playerData->posX + playerData->posZ * playerData->posZ);
            if (playerDistanceFromCenter < 200.0f) {
                playerDistanceFromCenter = 400.0f;
            }
            else {
                playerDistanceFromCenter = 600.0f;
            }
            if (closestFurnitureDistance > distance && distance < playerDistanceFromCenter) {
                HuSetVecF(&playerPosition, otherPlayer->posX, 0.0, otherPlayer->posZ);
                HuSetVecF(&furniturePosition, furnitureData->worldX, 0.0, furnitureData->worldZ);
                HuSubVecF(&separation, &playerPosition, &furniturePosition);
                if (sqrtf(separation.x * separation.x + separation.z * separation.z) > 50.0f) {
                    playerData->targetPlayerIndex = furnitureIndex;
                    closestFurnitureDistance = distance;
                }
            }
        }
    }
    if (playerData->targetPlayerIndex == -1) {
        for (furnitureIndex = 0, closestFurnitureDistance = 10000.0f; furnitureIndex < 8; furnitureIndex++) {
            furnitureData = &furniture[furnitureIndex];
            HuSetVecF(&playerPosition, playerData->posX, 0.0, playerData->posZ);
            HuSetVecF(&furniturePosition, furnitureData->worldX, 0.0, furnitureData->worldZ);
            HuSubVecF(&separation, &playerPosition, &furniturePosition);
            distance = sqrtf(separation.x * separation.x + separation.z * separation.z);
            if (closestFurnitureDistance > distance && distance > 200.0f) {
                playerData->targetPlayerIndex = furnitureIndex;
                closestFurnitureDistance = distance;
                approachSpeed = 0.3f;
            }
        }
    }
    if (playerData->targetPlayerIndex != -1) {
        furnitureData = &furniture[playerData->targetPlayerIndex];
        playerData->targetPosX = furnitureData->worldX;
        playerData->targetPosZ = furnitureData->worldZ;
        HuSetVecF(&playerPosition, playerData->posX, 0.0, playerData->posZ);
        HuSetVecF(&furniturePosition, playerData->targetPosX, 0.0, playerData->targetPosZ);
        HuSubVecF(&separation, &playerPosition, &furniturePosition);
        distance = sqrtf(separation.x * separation.x + separation.z * separation.z);
        if (distance > 20.0f) {
            targetAngle = atan2d(playerData->targetPosX - playerData->posX, playerData->targetPosZ - playerData->posZ);
            playerData->stickX = 50.0 * sind(targetAngle) * approachSpeed;
            playerData->stickY = -50.0 * cosd(targetAngle) * approachSpeed;
        }
        else {
            if (furnitureData->interactionPhase == 2) {
                for (furnitureIndex = 0; furnitureIndex < furnitureItemCounts[furnitureData->furnitureIndex]; furnitureIndex++) {
                    if (furnitureData->itemInteractionStates[furnitureIndex] == 0) {
                        targetAngle = atan2d(furnitureData->interactionX[furnitureIndex] - playerData->posX, furnitureData->interactionZ[furnitureIndex] - playerData->posZ);
                        playerData->stickX = 15.0 * sind(targetAngle);
                        playerData->stickY = -15.0 * cosd(targetAngle);
                    }
                }
            }
            if (frandmod(100) > randomThreshold) {
                playerData->buttonsPressed |= 0x100;
            }
            playerData->targetPlayerIndex = -1;
        }
        if (furnitureData->interactionPhase == 2) {
            if (IsPlayerNearPosition(playerData, furnitureData->interactionX[0], furnitureData->interactionZ[0]) && furnitureData->itemInteractionStates[0] == 0) {
                playerData->buttonsPressed |= 0x100;
            }
            if (furnitureItemCounts[furnitureData->furnitureIndex] > 1 && IsPlayerNearPosition(playerData, furnitureData->interactionX[1], furnitureData->interactionZ[1]) && furnitureData->itemInteractionStates[1] == 0) {
                playerData->buttonsPressed |= 0x100;
            }
        }
    }
}

void ShowPlayerSprites(void)
{
    s32 j;
    s32 i;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 5; j++) {
            HuSprAttrReset(players[i].spriteGroupId, j, HUSPR_ATTR_DISPOFF);
        }
    }
}

void HidePlayerSprites(void)
{
    s32 j;
    s32 i;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 5; j++) {
            HuSprAttrSet(players[i].spriteGroupId, j, HUSPR_ATTR_DISPOFF);
        }
    }
}

void SetPlayerItemCountDisplay(PlayerData *playerData, s32 itemCount)
{
    HuSprBankSet(playerData->spriteGroupId, 3, itemCount % 10);
    HuSprBankSet(playerData->spriteGroupId, 4, itemCount / 10);
    HuSprAttrReset(playerData->spriteGroupId, 4, HUSPR_ATTR_DISPOFF);
}

void UpdatePlayerExitMovement(PlayerData *playerData)
{
    Vec playerPosition;
    Vec targetPosition;
    Vec separation;
    float targetAngle;
    float playerDistance;
    float scale;
    float otherPlayerDistance;
    PlayerData *otherPlayer;
    s32 scaleFrame;
    s32 crossingBoundary;

    if (sceneState == 4) {
        playerData->collisionEnabled = 1;
        for (scaleFrame = 10, scale = 1.0f; scaleFrame != 0; scaleFrame--, scale -= 0.1f) {
            Hu3DModelScaleSet(playerData->heldItemModelId, scale, scale, scale);
            HuPrcVSleep();
        }
        Hu3DModelAttrSet(playerData->heldItemModelId, HU3D_ATTR_DISPOFF);
        playerData->heldItemModelId = -1;
        Hu3DMotionOverlayReset(playerData->modelId);
        if (players[0].heldItemModelId == -1 && players[1].heldItemModelId == -1) {
            sceneState = 5;
        }
        return;
    }
    Hu3DMotionOverlayReset(playerData->modelId);
    playerData->stickX = playerData->stickY = 0;
    playerData->triggerL = playerData->triggerR = 0;
    playerData->buttonsHeld = playerData->buttonsPressed = 0;
    crossingBoundary = 0;
    if (playerData->playerIndex == 0) {
        if (playerData->posX <= -152.0f) {
            playerData->targetPosX = -230.0f;
            playerData->targetPosZ = 45.0f;
            crossingBoundary = 1;
        }
        else if (playerData->posZ > 203.0f || playerData->posZ < -203.0f) {
            playerData->targetPosX = -170.0f;
            playerData->targetPosZ = playerData->posZ;
        }
        else {
            playerData->targetPosX = playerData->posX;
            if (playerData->posZ > 0.0f) {
                playerData->targetPosZ = 210.0f;
            }
            else {
                playerData->targetPosZ = -210.0f;
            }
        }
    }
    else {
        if (playerData->posX >= 152.0f) {
            playerData->targetPosX = 230.0f;
            playerData->targetPosZ = -45.0f;
            crossingBoundary = 1;
        }
        else if (playerData->posZ > 203.0f || playerData->posZ < -203.0f) {
            playerData->targetPosX = 170.0f;
            playerData->targetPosZ = playerData->posZ;
        }
        else {
            playerData->targetPosX = playerData->posX;
            if (playerData->posZ > 0.0f) {
                playerData->targetPosZ = 210.0f;
            }
            else {
                playerData->targetPosZ = -210.0f;
            }
        }
    }
    otherPlayer = (playerData->playerIndex != 0) ? &players[0] : &players[1];
    HuSetVecF(&playerPosition, playerData->posX, 0.0, playerData->posZ);
    HuSetVecF(&targetPosition, otherPlayer->posX, 0.0, otherPlayer->posZ);
    HuSubVecF(&separation, &playerPosition, &targetPosition);
    playerDistance = sqrtf(separation.x * separation.x + separation.z * separation.z);
    if (playerDistance <= 110.0f) {
        playerDistance = sqrtf(playerData->posX * playerData->posX + playerData->posZ * playerData->posZ);
        otherPlayerDistance = sqrtf(otherPlayer->posX * otherPlayer->posX + otherPlayer->posZ * otherPlayer->posZ);
        if (exitingPlayerIndex == -1) {
            targetAngle = atan2d(playerData->posX, playerData->posZ);
            if (playerDistance < 400.0f) {
                if (playerDistance > otherPlayerDistance) {
                    exitingPlayerIndex = playerData->playerIndex;
                    exitDirection = targetAngle;
                    playerData->targetPosX = playerData->posX + 100.0 * sind(targetAngle);
                    playerData->targetPosZ = playerData->posZ + 100.0 * cosd(targetAngle);
                }
            }
            else {
                if (playerDistance < otherPlayerDistance) {
                    targetAngle += 180.0f;
                    exitingPlayerIndex = playerData->playerIndex;
                    exitDirection = targetAngle;
                    playerData->targetPosX = playerData->posX + 100.0 * sind(targetAngle);
                    playerData->targetPosZ = playerData->posZ + 100.0 * cosd(targetAngle);
                }
            }
        }
        else if (playerData->playerIndex == exitingPlayerIndex) {
            OSReport("pno %d dir %f\n", playerData->playerIndex, exitDirection);
            playerData->targetPosX = playerData->posX + 100.0 * sind(exitDirection);
            playerData->targetPosZ = playerData->posZ + 100.0 * cosd(exitDirection);
        }
    }
    HuSetVecF(&playerPosition, playerData->posX, 0.0, playerData->posZ);
    HuSetVecF(&targetPosition, playerData->targetPosX, 0.0, playerData->targetPosZ);
    HuSubVecF(&separation, &playerPosition, &targetPosition);
    playerDistance = sqrtf(separation.x * separation.x + separation.z * separation.z);
    if (playerDistance < 20.0f && crossingBoundary != 0) {
        playerData->movementState = 4;
    }
    targetAngle = atan2d(playerData->targetPosX - playerData->posX, playerData->targetPosZ - playerData->posZ);
    playerData->stickX = 50.0 * sind(targetAngle);
    playerData->stickY = -50.0 * cosd(targetAngle);
}

void PlayerMovementProcess(void)
{
    Vec movementVector;
    s32 animationWaitFrames;
    float rotationStep;
    float rotationFrames;
    PlayerData *playerData;
    FurnitureData *furnitureData;
    u32 motionAttributes;
    s32 targetFurnitureIndex;
    s32 motionIndex;
    s32 frame;

    playerData = HuPrcCurrentGet()->user_data;
    playerData->movementState = 0;
    playerData->animationState = 0;
    playerData->targetPlayerIndex = -1;
    playerData->collisionEnabled = 1;
    exitingPlayerIndex = -1;
    while (sceneState == 0) {
        HuPrcVSleep();
    }
    sceneState = 2;
    while (sceneState == 2) {
        HuPrcVSleep();
    }
    while (TRUE) {
        playerData->previousPosX = playerData->posX;
        playerData->previousPosZ = playerData->posZ;
        if (GWPlayerCfg[playerData->playerIndex].iscom == 1) {
            UpdateComputerPlayerInput(playerData);
        }
        if (sceneState >= 4) {
            UpdatePlayerExitMovement(playerData);
        }
        switch (playerData->movementState) {
            case 4:
                goto blockExitLoop;
        }
        if (GetPressedButtons(playerData->playerIndex) & 0x100) {
            if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
                Hu3DMotionOverlayReset(playerData->modelId);
                CharMotionSet(characterModelIndices[GWPlayerCfg[playerData->playerIndex].character], playerData->motions[5]);
            }
            else {
                Hu3DMotionSet(playerData->modelId, playerData->motions[4]);
                Hu3DMotionSpeedSet(playerData->modelId, 1.0f);
            }
            Hu3DModelAttrReset(playerData->modelId, HU3D_MOTATTR_LOOP);
            HuPrcSleep(15);
            if (playerData->playerIndex != 0) {
                PlaySoundAtPosition(playerData->posX, playerData->posZ, 0x75F);
            }
            else {
                PlaySoundAtPosition(playerData->posX, playerData->posZ, 0x75E);
            }
            HuPrcSleep(15);
            HuSetVecF(&movementVector, playerData->posX + 120.0 * sind(playerData->rotY), 0.0, playerData->posZ + 120.0 * cosd(playerData->rotY));
            for (frame = 0; frame < 8; frame++) {
                furnitureData = &furniture[frame];
                if (furnitureData->interactionPhase == 2) {
                    animationWaitFrames = 0;
                    if (IsPlayerNearPosition(playerData, furnitureData->interactionX[0], furnitureData->interactionZ[0]) && furnitureData->itemInteractionStates[0] == 0) {
                        animationWaitFrames = 1;
                        furnitureData->itemInteractionStates[0] = 1;
                        furnitureData->firstItemPlayerIndex = playerData->playerIndex;
                        if (playerData->playerIndex != 0) {
                            PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x75D);
                        }
                        else {
                            PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x750);
                        }
                        Hu3DModelAttrReset(playerData->itemModelId, HU3D_ATTR_DISPOFF);
                        Hu3DMotionTimeSet(playerData->itemModelId, 0.0f);
                        Hu3DModelPosSet(playerData->itemModelId, movementVector.x, 160.0f, movementVector.z + 100.0f);
                        if (furnitureData->isRewardRound != 0) {
                            playerData->itemCount += 2;
                        }
                        else {
                            playerData->itemCount++;
                        }
                        SetPlayerItemCountDisplay(playerData, playerData->itemCount);
                    }
                    if (furnitureItemCounts[furnitureData->furnitureIndex] > 1 && IsPlayerNearPosition(playerData, furnitureData->interactionX[1], furnitureData->interactionZ[1])
                        && furnitureData->itemInteractionStates[1] == 0) {
                        furnitureData->itemInteractionStates[1] = 1;
                        furnitureData->secondItemPlayerIndex = playerData->playerIndex;
                        if (playerData->playerIndex != 0) {
                            PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x75D);
                        }
                        else {
                            PlaySoundAtPosition(furnitureData->worldX, furnitureData->worldZ, 0x750);
                        }
                        Hu3DModelAttrReset(playerData->itemModelId, HU3D_ATTR_DISPOFF);
                        Hu3DMotionTimeSet(playerData->itemModelId, 0.0f);
                        Hu3DModelPosSet(playerData->itemModelId, movementVector.x, 160.0f, movementVector.z + 100.0f);
                        if (furnitureData->isRewardRound != 0) {
                            playerData->itemCount += 2;
                        }
                        else {
                            playerData->itemCount++;
                        }
                        SetPlayerItemCountDisplay(playerData, playerData->itemCount);
                    }
                }
            }
            while (!Hu3DMotionEndCheck(playerData->modelId)) {
                HuPrcVSleep();
            }
            playerData->movementState = 0;
            if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
                CharMotionSet(characterModelIndices[GWPlayerCfg[playerData->playerIndex].character], playerData->motions[0]);
                Hu3DModelAttrSet(playerData->modelId, HU3D_MOTATTR_OVL_LOOP);
                if (sceneState < 5) {
                    Hu3DMotionOverlaySet(playerData->modelId, playerData->motions[4]);
                    Hu3DModelAttrSet(playerData->modelId, HU3D_MOTATTR_OVL_LOOP);
                }
            }
            else {
                Hu3DMotionShiftSet(playerData->modelId, playerData->motions[0], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                Hu3DMotionShiftSpeedSet(playerData->modelId, 1.0f);
            }
        }
        else {
            HuSetVecF(&movementVector, GetMovementStickValue(playerData->playerIndex, 0) / 4.0f, 0.0, -GetMovementStickValue(playerData->playerIndex, 1) / 4.0f);
            playerData->moveSpeed = sqrtf(movementVector.x * movementVector.x + movementVector.y * movementVector.y + movementVector.z * movementVector.z);
            if (playerData->moveSpeed >= 0.5f) {
                if (playerData->moveSpeed >= 10.0f) {
                    movementVector.x /= playerData->moveSpeed;
                    movementVector.z /= playerData->moveSpeed;
                    playerData->moveSpeed = 10.0f;
                    movementVector.x = 10.0f * movementVector.x;
                    movementVector.z = 10.0f * movementVector.z;
                }
                playerData->rotY = NormalizeAngle(InterpolateAngle(playerData->rotY, atan2d(movementVector.x, movementVector.z), 0.4f));
                if (playerData->moveSpeed >= 8.0f) {
                    if (playerData->movementState != 2) {
                        playerData->movementState = 2;
                        if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
                            CharMotionShiftSet(
                                characterModelIndices[GWPlayerCfg[playerData->playerIndex].character], playerData->motions[2], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                            if (sceneState < 5) {
                                Hu3DMotionOverlaySet(playerData->modelId, playerData->motions[4]);
                                Hu3DModelAttrSet(playerData->modelId, HU3D_MOTATTR_OVL_LOOP);
                            }
                        }
                        else {
                            Hu3DMotionShiftSet(playerData->modelId, playerData->motions[2], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                            Hu3DMotionShiftSpeedSet(playerData->modelId, 2.0f);
                        }
                    }
                }
                else if (playerData->movementState != 1) {
                    playerData->movementState = 1;
                    if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
                        CharMotionShiftSet(
                            characterModelIndices[GWPlayerCfg[playerData->playerIndex].character], playerData->motions[1], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                        if (sceneState < 5) {
                            Hu3DMotionOverlaySet(playerData->modelId, playerData->motions[4]);
                            Hu3DModelAttrSet(playerData->modelId, HU3D_MOTATTR_OVL_LOOP);
                        }
                    }
                    else {
                        Hu3DMotionShiftSet(playerData->modelId, playerData->motions[1], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                        Hu3DMotionShiftSpeedSet(playerData->modelId, 2.0f);
                    }
                }
            }
            else if (playerData->movementState != 0) {
                playerData->movementState = 0;
                if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
                    CharMotionShiftSet(characterModelIndices[GWPlayerCfg[playerData->playerIndex].character], playerData->motions[0], 0.0f, 0.0f, HU3D_MOTATTR_LOOP);
                    if (sceneState < 5) {
                        Hu3DMotionOverlaySet(playerData->modelId, playerData->motions[4]);
                        Hu3DModelAttrSet(playerData->modelId, HU3D_MOTATTR_OVL_LOOP);
                    }
                }
                else {
                    Hu3DMotionShiftSet(playerData->modelId, playerData->motions[0], 0.0f, 0.0f, HU3D_MOTATTR_LOOP);
                    Hu3DMotionShiftSpeedSet(playerData->modelId, 1.0f);
                }
            }
            playerData->movementVelocity.x = movementVector.x;
            playerData->movementVelocity.z = movementVector.z;
            playerData->previousSpeed = playerData->moveSpeed;
            playerData->posX += playerData->movementVelocity.x;
            playerData->posZ += playerData->movementVelocity.z;
        }
        if ((playerData->collisionIndex = FindOverlappingPlayerAtPosition(playerData, playerData->posX, playerData->posY, playerData->posZ)) != -1) {
            ResolvePlayerCollision(playerData);
        }
        AddX = 0.0f;
        AddZ = 0.0f;
        MapWall(playerData->collisionRadius, playerData->posX, playerData->posY + playerData->collisionRadius, playerData->posZ);
        playerData->posX += AddX;
        playerData->posZ += AddZ;
        Hu3DModelPosSet(playerData->modelId, playerData->posX, playerData->posY, playerData->posZ);
        Hu3DModelRotSet(playerData->modelId, playerData->rotX, playerData->rotY, playerData->rotZ);
        HuPrcVSleep();
    }
blockExitLoop:
    sceneState = 6;
    if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
        CharMotionShiftSet(characterModelIndices[GWPlayerCfg[playerData->playerIndex].character], playerData->motions[0], 0.0f, 2.0f, HU3D_MOTATTR_LOOP);
    }
    else {
        Hu3DMotionShiftSet(playerData->modelId, playerData->motions[0], 0.0f, 0.0f, HU3D_MOTATTR_LOOP);
        Hu3DMotionShiftSpeedSet(playerData->modelId, 1.0f);
    }
    rotationFrames = 10.0f;
    frame = (360.0f - NormalizeAngle(playerData->rotY)) / rotationFrames;
    if (playerData->rotY < 180.0f) {
        frame = NormalizeAngle(playerData->rotY) / rotationFrames;
        rotationFrames = -rotationFrames;
    }
    for (; frame != 0; frame--) {
        playerData->rotY += rotationFrames;
        Hu3DModelRotSet(playerData->modelId, playerData->rotX, playerData->rotY, playerData->rotZ);
        HuPrcVSleep();
    }
    Hu3DModelRotSet(playerData->modelId, playerData->rotX, 0.0f, playerData->rotZ);
    while (sceneState < 7) {
        HuPrcVSleep();
    }
    if (players->itemCount == players[1].itemCount) {
        targetFurnitureIndex = -1;
    }
    if (players->itemCount < players[1].itemCount) {
        targetFurnitureIndex = 1;
    }
    if (players->itemCount > players[1].itemCount) {
        targetFurnitureIndex = 0;
    }
    motionAttributes = HU3D_MOTATTR_NONE;
    if (playerData->playerIndex != 0) {
        if (IsPlayerCharacterModel(1) == FALSE) {
            if (targetFurnitureIndex != -1) {
                HuAudSStreamPlay(1);
            }
            else {
                HuAudSStreamPlay(4);
            }
        }
        else {
            if (targetFurnitureIndex == 0) {
                HuAudSStreamPlay(1);
            }
            else {
                HuAudSStreamPlay(3);
            }
        }
    }
    if (targetFurnitureIndex == -1) {
        if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
            motionIndex = 7;
        }
        else {
            motionIndex = 6;
        }
    }
    else {
        GWPlayerCoinWinSet(targetFurnitureIndex, 10);
        if (targetFurnitureIndex == playerData->playerIndex) {
            motionIndex = 6;
        }
        else {
            motionIndex = 7;
        }
    }
    if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
        CharMotionShiftSet(characterModelIndices[GWPlayerCfg[playerData->playerIndex].character], playerData->motions[motionIndex], 0.0f, 8.0f, motionAttributes);
    }
    else {
        if (motionIndex == 6) {
            motionIndex = 5;
            motionAttributes = HU3D_MOTATTR_LOOP;
        }
        else {
            motionIndex = 6;
            motionAttributes = HU3D_MOTATTR_NONE;
        }
        Hu3DMotionShiftSet(playerData->modelId, playerData->motions[motionIndex], 0.0f, 8.0f, motionAttributes);
        if (motionIndex == 6) {
            for (frame = 10, rotationStep = 30.0f / frame; frame != 0; frame--) {
                playerData->rotY += rotationStep;
                Hu3DModelRotSet(playerData->modelId, 0.0f, playerData->rotY, 0.0f);
                HuPrcVSleep();
            }
        }
    }
    while (TRUE) {
        HuPrcVSleep();
    }
}

void UpdatePlayerParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    PlayerData *playerData;
    float particleAlpha;
    s16 playerIndex;
    s16 particleIndex;

    if (particle->count == 0) {
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->accel.x = particleData->scale = 0.0f;
        }
    }
    particleData = particle->data;
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        playerData = &players[playerIndex];
        if (playerData->collisionEnabled != 0) {
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                if (particleData->accel.x == 0.0f) {
                    break;
                }
            }
            if (particleIndex == particle->maxCnt) {
                break;
            }
            particleData->pos.x = playerData->posX + 50.0 * sind(playerData->rotY - 90.0f);
            particleData->pos.y = playerData->posY + 98.0f;
            particleData->pos.z = playerData->posZ + 50.0 * cosd(playerData->rotY - 90.0f);
            particleData->accel.x = 30.0f;
            particleData->scale = 100.0f;
            particleData->accel.y = 10.0f;
            particleData->color.a = 200;
            particleData->accel.z = 17.0f;
            playerData->collisionEnabled = 0;
        }
    }
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (0.0f != particleData->accel.x) {
            particleData->scale += particleData->accel.y;
            if (particleData->accel.y > 2.0f) {
                particleData->accel.y -= 0.2f;
            }
            particleAlpha = particleData->color.a;
            particleAlpha -= particleData->accel.z;
            if (particleData->accel.z > 3.0f) {
                particleData->accel.z -= 1.0f;
            }
            if (particleAlpha < 0.0f) {
                particleAlpha = 0.0f;
                particleData->accel.x = 0.0f;
            }
            particleData->color.a = particleAlpha;
            if (0.0f == particleData->accel.x) {
                particleData->scale = 0.0f;
            }
        }
    }
}

void PlayEndingSequence(void)
{
    float messageWindowSize[2];
    GXColor messageColor;
    WindowData *windowData;
    PlayerData *playerData;
    s16 musicSequenceId;
    s16 windowId;
    u32 messageId;
    s32 choice;
    s32 selectedOption;
    s32 frame;

    HuPrcSleep(30);
    HuAudFXPlay(0x755);
    choice = furniture[5].modelId;
    Hu3DModelAttrReset(choice, HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
    while (!Hu3DMotionEndCheck(choice)) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x756);
    Hu3DMotionSet(choice, furniture[5].motions[0]);
    Hu3DModelAttrSet(choice, HU3D_MOTATTR_LOOP);
    HuPrcSleep(30);
    for (frame = 30; frame != 0; frame--) {
        CRotM[0].x += -0.55f;
        CenterM[0].y += -11.5f;
        CZoomM[0] += 51.333332f;
        HuPrcVSleep();
    }
    if (IsPlayerCharacterModel(1) == TRUE) {
        musicSequenceId = HuAudSeqPlay(0x3D);
        HuWinMesMaxSizeGet(8, messageWindowSize, MAKE_MESSID(41, 1), MAKE_MESSID(41, 2), MAKE_MESSID(41, 3), MAKE_MESSID(41, 4), MAKE_MESSID(41, 5),
            MAKE_MESSID(41, 6), MAKE_MESSID(41, 7), MAKE_MESSID(41, 8));
        windowId = HuWinCreate(-10000.0f, 40.0f, messageWindowSize[0], messageWindowSize[1], 1);
        HuWinPriSet(windowId, 0x100);
        messageColor.r = messageColor.g = messageColor.b = 0xFF;
        HuWinBGColSet(windowId, &messageColor);
        HuWinBGTPLvlSet(windowId, 0.9f);
        HuWinMesColSet(windowId, 0);
        windowData = &winData[windowId];
        windowData->mess_shadow_color = 9;
        HuWinMesSpeedSet(windowId, 0);
        HuWinDisablePlayerSet(windowId, (1 << GWPlayerCfg->pad_idx) ^ 0xF);
        HuWinExAnimIn(windowId);
        HuWinMesSet(windowId, MAKE_MESSID(41, 1));
        HuWinMesWait(windowId);
        HuWinMesColSet(windowId, 0);
        HuWinMesSet(windowId, MAKE_MESSID(41, 2));
        selectedOption = HuWinChoiceGet(windowId, 0);
        if (selectedOption == 0) {
            frame = 5;
            messageId = MAKE_MESSID(41, 3);
        }
        else {
            frame = 1;
            messageId = MAKE_MESSID(41, 8);
        }
        for (; frame != 0; frame--) {
            HuWinMesColSet(windowId, 0);
            HuWinMesSet(windowId, messageId++);
            HuWinMesWait(windowId);
        }
        HuAudSeqFadeOut(musicSequenceId, 1000);
        HuWinExAnimOut(windowId);
        HuWinAllKill();
    }
    for (frame = 0; frame < 2; frame++) {
        playerData = &players[frame];
        if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
            CharMotionSet(characterModelIndices[GWPlayerCfg[frame].character], playerData->motions[3]);
        }
        else {
            Hu3DMotionSet(playerData->modelId, playerData->motions[3]);
        }
        Hu3DModelAttrReset(playerData->modelId, HU3D_MOTATTR_LOOP);
    }
    HuPrcSleep(20);
    for (frame = 0; frame < 2; frame++) {
        playerData = &players[frame];
        Hu3DModelPosSet(playerData->heldItemModelId, 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(playerData->heldItemModelId, 0.0f, 0.0f, 0.0f);
        if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
            Hu3DModelHookSet(playerData->modelId, CharModelItemHookGet(GWPlayerCfg[frame].character, 2, 0), playerData->heldItemModelId);
        }
        else {
            Hu3DModelHookSet(playerData->modelId, "itemhook_r", playerData->heldItemModelId);
        }
    }
    HuPrcSleep(20);
    for (frame = 0; frame < 2; frame++) {
        playerData = &players[frame];
        if (IsPlayerCharacterModel(playerData->playerIndex) == FALSE) {
            CharMotionShiftSet(characterModelIndices[GWPlayerCfg[playerData->playerIndex].character], playerData->motions[0], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
            Hu3DMotionOverlaySet(playerData->modelId, playerData->motions[4]);
        }
        else {
            Hu3DMotionShiftSet(playerData->modelId, playerData->motions[0], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
        }
    }
}

void AnimateEndingCamera(void)
{
    s32 i;

    while (sceneState < 6) {
        HuPrcVSleep();
    }
    for (i = 30; i != 0; i--) {
        CZoomM[0] += -27.7f;
        HuPrcVSleep();
    }
}

void WaitForSceneExit(void)
{
    while (TRUE) {
        if (omSysExitReq == 1) {
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
            HuAudFadeOut(1000);
            HuPrcSleep(60);
            MGSeqKillAll();
            omOvlReturnEx(1, 1);
            while (TRUE) {
                HuPrcVSleep();
            }
        }
        HuPrcVSleep();
    }
}

void RunMinigameSequence(void)
{
    s16 sequenceId;
    s16 remainingFrames;
    s16 timerSequenceId;
    s32 musicSequenceId;
    s32 frame;

    sceneState = 0;
    PlayEndingSequence();
    HuPrcSleep(30);
    sceneState = 1;
    while (sceneState == 1) {
        HuPrcVSleep();
    }
    musicSequenceId = HuAudSeqPlay(0x3E);
    sequenceId = MGSeqStartCreate();
    while (MGSeqStatGet(sequenceId) != 0) {
        HuPrcVSleep();
    }
    ShowPlayerSprites();
    remainingFrames = (REFRESH_RATE*61)-1;
    timerSequenceId = MGSeqTimerCreate(remainingFrames / REFRESH_RATE);
    sceneState = 3;
    while (TRUE) {
        if (remainingFrames < (REFRESH_RATE-1)) {
            break;
        }
        MGSeqParamSet(timerSequenceId, 1, remainingFrames-- / REFRESH_RATE);
        elapsedFrames++;
        HuPrcVSleep();
    }
    winningPlayerIndex = -1;
    if (players->itemCount != players[1].itemCount) {
        winningPlayerIndex = 0;
        if (players->itemCount < players[1].itemCount) {
            winningPlayerIndex = 1;
        }
    }
    sceneState = 4;
    HuAudSeqFadeOut(musicSequenceId, 100);
    MGSeqParamSet(timerSequenceId, 2, -1);
    sequenceId = MGSeqFinishCreate();
    while (MGSeqStatGet(sequenceId) != 0) {
        HuPrcVSleep();
    }
    while (players->movementState != 4 || players[1].movementState != 4) {
        HuPrcVSleep();
    }
    while (sceneState < 6) {
        HuPrcVSleep();
    }
    for (frame = 30; frame != 0; frame--) {
        CZoomM[0] += -27.7f;
        HuPrcVSleep();
    }
    sceneState = 7;
    HuPrcSleep(210);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    HuPrcSleep(60);
    HuAudFadeOut(1);
    MGSeqKillAll();
    omOvlReturnEx(1, 1);
    while (TRUE) {
        HuPrcVSleep();
    }
}

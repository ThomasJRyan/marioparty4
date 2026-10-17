#include "REL/m412Dll.h"

#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/hu3d.h"
#include "game/mapspace.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include "math.h"
#include "version.h"

// bss
s16 directionalLightId;
s16 sceneCameraId;
s16 sceneModelId;
s16 roundModelIds[2];
s32 roundState;
s32 roundTimer;
s32 playerCharacterResults[4];
f32 cameraPitch;
f32 cameraYaw;
f32 cameraDistance;
PlayerState playerStates[4];
SceneModelSet sceneModelSets[6];
SceneModelSet bonusModelSet;
DebrisPiece *debrisSlots[0x20];
HUPROCESS *objectManager;
Vec cameraPosition;
Vec cameraRotation;
Vec cameraUpVector;
Vec fragmentOriginalPositions[0x17];
Vec fragmentOriginalScales[0x17];
s32 remainingPlayerCount;
Vec shadowPosition;
Vec shadowUpVector;
Vec shadowTarget;

// data
s32 characterModelIndices[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
s32 characterModelResources[8] = { DATA_MAKE_NUM(DATADIR_MARIOMDL1, 0x01), DATA_MAKE_NUM(DATADIR_LUIGIMDL1, 0x01), DATA_MAKE_NUM(DATADIR_PEACHMDL1, 0x01),
    DATA_MAKE_NUM(DATADIR_YOSHIMDL1, 0x01), DATA_MAKE_NUM(DATADIR_WARIOMDL1, 0x01), DATA_MAKE_NUM(DATADIR_DONKEYMDL1, 0x01),
    DATA_MAKE_NUM(DATADIR_DAISYMDL1, 0x01), DATA_MAKE_NUM(DATADIR_WALUIGIMDL1, 0x01) };
s32 characterMotionResources[8] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00), DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00), DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00) };
s32 playerMotionIndices[6] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x1A), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x1B), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x4B) };
Vec fragmentSpawnPositions[7] = { { 474.0f, 30.0f, -497.0f }, { 330.0f, 30.0f, 562.0f }, { -694.0f, 30.0f, 43.0f }, { -375.0f, 30.0f, -483.0f },
    { 715.0f, 30.0f, 62.0f }, { -334.0f, 30.0f, 570.0f }, { 100.0f, 30.0f, -630.0f } };
char *fragmentObjectNames[0x17] = { "sb_hahen-sb_hahen01", "sb_hahen-sb_hahen02", "sb_hahen-sb_hahen03", "sb_hahen-sb_hahen04", "sb_hahen-sb_hahen05",
    "sb_hahen-sb_hahen06", "sb_hahen-sb_hahen07", "sb_hahen-sb_hahen08", "sb_hahen-sb_hahen09", "sb_hahen-sb_hahen10", "sb_hahen-sb_hahen11",
    "sb_hahen-sb_hahen12", "sb_hahen-sb_hahen13", "sb_hahen-sb_hahen14", "sb_hahen-sb_hahen15", "sb_hahen-sb_hahen16", "sb_hahen-sb_hahen17",
    "sb_hahen-sb_hahen18", "sb_hahen-sb_hahen19", "sb_hahen-sb_hahen20", "sb_hahen-sb_hahen21", "sb_hahen-sb_hahen22", "sb_hahen-sb_hahen23" };

void ObjectSetup(void)
{
    f32 cameraX;
    f32 cameraY;
    f32 cameraZ;

    objectManager = omInitObjMan(0x32, 0x2000);
    omGameSysInit(objectManager);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 40.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0U, 0U, 0x280U, 0x1E0U);
    cameraPosition.x = 1100.0f;
    cameraPosition.y = 1600.0f;
    cameraPosition.z = 760.0f;
    cameraUpVector.x = 0.0f;
    cameraUpVector.y = 1.0f;
    cameraUpVector.z = 0.0f;
    cameraRotation.x = 60.0f;
    cameraRotation.y = 0.0f;
    cameraRotation.z = 0.0f;
    cameraX = cameraPosition.x;
    cameraY = cameraPosition.y;
    cameraZ = cameraPosition.z;
    cameraDistance = sqrtf((cameraZ * cameraZ) + ((cameraX * cameraX) + (cameraY * cameraY)));
    cameraYaw = WrapDegrees(atan2d(cameraX, cameraZ));
    cameraPitch = WrapDegrees(atan2d(cameraY, cameraZ));
    cameraPitch = 50.0f;
    Hu3DLighInit();
    directionalLightId = Hu3DGLightCreate(10000.0f, 0.0f, 10000.0f, -1.0f, 0.0f, -1.0f, 0xFFU, 0xFFU, 0xFFU);
    Hu3DGLightPosAimSet(directionalLightId, 0.0f, 4330.13f, 2500.0f, 0.0f, 0.0f, 0.0f);
    Hu3DGLightInfinitytSet(directionalLightId);
    shadowPosition.x = -500.0f;
    shadowPosition.y = 4000.0f;
    shadowPosition.z = 0.0f;
    shadowUpVector.x = shadowUpVector.z = 0.0f;
    shadowUpVector.y = 1.0f;
    shadowTarget.x = 0.0f;
    shadowTarget.y = 0.0f;
    shadowTarget.z = 0.0f;
    Hu3DShadowCreate(30.0f, 20.0f, 10000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowPosSet(&shadowPosition, &shadowUpVector, &shadowTarget);
    Hu3DBGColorSet(0U, 0U, 0U);
    InitializeMinigameScene();
    HuPrcChildCreate(RunMinigameController, 0x1000U, 0x2000U, 0, HuPrcCurrentGet());
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
}

void *PlayPositionedSoundEffect(f32 xPosition, f32 yPosition, f32 zPosition, s32 soundEffectId)
{
    Vec soundPosition;

    soundPosition.x = xPosition;
    soundPosition.y = yPosition;
    soundPosition.z = zPosition;
    return (void *)HuAudFXEmiterPlay(soundEffectId, &soundPosition);
}

void UpdateAudioListenerProcess(void)
{
    Vec listenerDirection;
    HU3DCAMERA *camera;

    camera = &Hu3DCamera[0];

    listenerDirection.x = 0.0f;
    listenerDirection.y = 0.0f;
    listenerDirection.z = -1.0f;

    while (1) {
        HuAudFXListnerUpdate(&camera->pos, &listenerDirection);
        if (roundState >= 3)
            break;
        HuPrcVSleep();
    }
    HuPrcKill(HuPrcCurrentGet());
    while (1) {
        HuPrcVSleep();
    }
}

void MoveSoundEmitterProcess(void)
{
    Vec emitterPosition;
    Vec emitterStep;
    s32 emitterHandle;
    s32 frame;

    emitterHandle = (s32)HuPrcCurrentGet()->user_data;
    emitterPosition.x = 1465.0f;
    emitterPosition.y = 770.0f;
    emitterPosition.z = -2400.0f;
    emitterStep.x = emitterPosition.x - 474.0f;
    emitterStep.y = emitterPosition.y - 30.0f;
    emitterStep.z = emitterPosition.z - -497.0f;

    for (frame = 0; frame < 0x168; frame++) {
        HuAudFXEmiterUpDate(emitterHandle, &emitterPosition);
        VECAdd(&emitterStep, &emitterPosition, &emitterPosition);
        HuPrcVSleep();
    }
    HuPrcKill(HuPrcCurrentGet());
    while (1) {
        HuPrcVSleep();
    }
}

void InitializeHiddenObject(OMOBJ *object)
{
    return;
}

void InitializeHiddenObjectCallback(OMOBJ *object)
{
    object->mdlId[0] = sceneModelId;
    object->objFunc = &InitializeHiddenObject;
}

void InitializeMinigameScene(void)
{
    Vec listenerPosition;
    Vec listenerDirection;
    s32 shuffleValue;
    volatile u32 shuffleIndexA;
    volatile u32 shuffleIndexB;
    SceneModelSet *sceneModelSet;
    PlayerState *playerState;
    s16 modelId;
    void *resourceData;
    s32 index;
    s32 motionIndex;
    SceneModelSet *otherModelSet;
    HSFTRANSFORM *meshTransform;
    HUPROCESS *process;
    s32 swapValue;
    ANIMDATA *animationData;
    s16 baseModelId;
    s16 effectBaseModelId;
    s32 playerCount;
    HSFOBJECT *meshObject;

    nMap = 0;
    HuAudSndGrpSet(0x25);
    listenerPosition.x = 1100.0f;
    listenerPosition.y = 1600.0f;
    listenerPosition.z = 760.0f;
    listenerDirection.x = 0.0f;
    listenerDirection.y = 0.0f;
    listenerDirection.z = -1.0f;
    HuAudFXListnerSetEX(&listenerPosition, &listenerDirection, 10000.0f, 34 * REFRESH_FREQ, 0.0f, 300.0f, 300.0f);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x00), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(resourceData);
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelShadowMapSet(modelId);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x01), HU_MEMNUM_OVL, HEAP_MODEL);
    sceneModelId = Hu3DModelCreate(resourceData);
    Hu3DModelAttrSet(sceneModelId, HU3D_ATTR_DISPOFF);
    MapObject[nMap++] = omAddObjEx(objectManager, 0x100, 1, 0, -1, InitializeHiddenObjectCallback);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x02), HU_MEMNUM_OVL, HEAP_MODEL);
    modelId = Hu3DModelCreate(resourceData);
    Hu3DModelLayerSet(modelId, 0);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);

    for (index = 0; index < 0x20; index++) {
        debrisSlots[index] = NULL;
    }
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x1F), HU_MEMNUM_OVL, HEAP_MODEL);
    sceneCameraId = modelId = Hu3DModelCameraCreate(Hu3DMotionCreate(resourceData), 1);
    Hu3DCameraMotionStart(modelId, 1);

    for (index = 0; index < 2; index++) {
        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x1D), HU_MEMNUM_OVL, HEAP_MODEL);
        roundModelIds[index] = modelId = Hu3DModelCreate(resourceData);
        Hu3DModelLayerSet(modelId, 2);
        Hu3DModelAttrSet(modelId, HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    }

    for (index = 0; index < 6; index++) {
        sceneModelSet = &sceneModelSets[index];
        sceneModelSet->playerIndex = index;
        sceneModelSet->selectionOrder = index;
        sceneModelSet->activeState = 0;
        if (index == 0) {
            resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x06), HU_MEMNUM_OVL, HEAP_MODEL);
            baseModelId = modelId = Hu3DModelCreate(resourceData);
        }
        else {
            modelId = Hu3DModelLink(baseModelId);
        }
        sceneModelSet->primaryModelId = modelId;
        sceneModelSet->position.x = fragmentSpawnPositions[index].x;
        sceneModelSet->position.y = fragmentSpawnPositions[index].y;
        sceneModelSet->position.z = fragmentSpawnPositions[index].z;
        Hu3DModelPosSet(modelId, sceneModelSet->position.x, sceneModelSet->position.y, sceneModelSet->position.z);
        sceneModelSet->rotation.x = sceneModelSet->rotation.z = 0.0f;
        sceneModelSet->rotation.y = WrapDegrees(180.0 + atan2d(sceneModelSet->position.x, sceneModelSet->position.z));
        Hu3DModelRotSet(modelId, sceneModelSet->rotation.x, sceneModelSet->rotation.y, sceneModelSet->rotation.z);
        Hu3DModelLayerSet(modelId, 1);
        Hu3DModelShadowSet(modelId);

        for (motionIndex = 0; motionIndex < 3; motionIndex++) {
            resourceData = HuDataSelHeapReadNum(motionIndex + DATA_MAKE_NUM(DATADIR_M412, 0x07), HU_MEMNUM_OVL, HEAP_MODEL);
            sceneModelSet->characterMotions[motionIndex] = Hu3DJointMotion(modelId, resourceData);
        }
        resourceData = HuDataSelHeapReadNum(index + DATA_MAKE_NUM(DATADIR_M412, 0x0A), HU_MEMNUM_OVL, HEAP_MODEL);
        sceneModelSet->characterMotions[motionIndex] = Hu3DJointMotion(modelId, resourceData);
        Hu3DMotionSet(modelId, sceneModelSet->characterMotions[3]);
        sceneModelSet->reflectedModelId = modelId = Hu3DModelLink(baseModelId);
        Hu3DModelPosSet(modelId, sceneModelSet->position.x, sceneModelSet->position.y - 10.0f, sceneModelSet->position.z);
        Hu3DModelRotSet(modelId, sceneModelSet->rotation.x, sceneModelSet->rotation.y, sceneModelSet->rotation.z);
        Hu3DModelScaleSet(modelId, 1.0f, -1.0f, 1.0f);
        Hu3DModelLayerSet(modelId, 0);
        Hu3DModelShadowReset(modelId);
        Hu3DMotionSet(modelId, sceneModelSet->characterMotions[3]);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_CULL_FRONT);
        resourceData = HuDataSelHeapReadNum(index + DATA_MAKE_NUM(DATADIR_M412, 0x10), HU_MEMNUM_OVL, HEAP_MODEL);
        sceneModelSet->detailModelId = modelId = Hu3DModelCreate(resourceData);
        Hu3DModelLayerSet(modelId, 1);
        if (index >= 3) {
            Hu3DModelAttrSet(sceneModelSet->primaryModelId, HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrSet(sceneModelSet->reflectedModelId, HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrSet(sceneModelSet->detailModelId, HU3D_MOTATTR_PAUSE);
        }
        if (index == 0) {
            resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x03), HU_MEMNUM_OVL, HEAP_MODEL);
            effectBaseModelId = modelId = Hu3DModelCreate(resourceData);
        }
        else {
            modelId = Hu3DModelLink(effectBaseModelId);
        }
        sceneModelSet->effectModelId = modelId;
        Hu3DModelLayerSet(modelId, 1);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
        Hu3DModelShadowSet(modelId);
        Hu3DModelShadowDispOff(modelId);
        sceneModelSet->linkedModelId = Hu3DModelLink(effectBaseModelId);
        Hu3DModelLayerSet(modelId, 0);
        Hu3DModelAttrSet(sceneModelSet->linkedModelId, HU3D_ATTR_DISPOFF);
        process = HuPrcChildCreate(RunGameplaySequence, 0x2000, 0x3000, 0, HuPrcCurrentGet());
        process->user_data = sceneModelSet;
    }

    for (index = 0, playerCount = 0; index < 50; index++) {
        sceneModelSet = &sceneModelSets[frandmod(3) + playerCount];
        otherModelSet = &sceneModelSets[frandmod(3) + playerCount];
        swapValue = sceneModelSet->selectionOrder;
        sceneModelSet->selectionOrder = otherModelSet->selectionOrder;
        otherModelSet->selectionOrder = swapValue;
    }
    sceneModelSet = &sceneModelSets[3];
    otherModelSet = &sceneModelSets[5];
    swapValue = sceneModelSet->selectionOrder;
    sceneModelSet->selectionOrder = otherModelSet->selectionOrder;
    otherModelSet->selectionOrder = swapValue;
    if (frandmod(2) != 0) {
        sceneModelSet = &sceneModelSets[4];
        otherModelSet = &sceneModelSets[5];
        swapValue = sceneModelSet->selectionOrder;
        sceneModelSet->selectionOrder = otherModelSet->selectionOrder;
        otherModelSet->selectionOrder = swapValue;
    }
    sceneModelSet = &bonusModelSet;
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x16), HU_MEMNUM_OVL, HEAP_MODEL);
    baseModelId = modelId = Hu3DModelCreate(resourceData);
    sceneModelSet->primaryModelId = modelId;
    sceneModelSet->position.x = fragmentSpawnPositions[6].x;
    sceneModelSet->position.y = fragmentSpawnPositions[6].y;
    sceneModelSet->position.z = fragmentSpawnPositions[6].z;
    Hu3DModelPosSet(modelId, sceneModelSet->position.x, sceneModelSet->position.y, sceneModelSet->position.z);
    sceneModelSet->rotation.x = sceneModelSet->rotation.z = 0.0f;
    sceneModelSet->rotation.y = WrapDegrees(180.0 + atan2d(sceneModelSet->position.x, sceneModelSet->position.z));
    Hu3DModelRotSet(modelId, sceneModelSet->rotation.x, sceneModelSet->rotation.y, sceneModelSet->rotation.z);
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelShadowSet(modelId);

    for (motionIndex = 0; motionIndex < 5; motionIndex++) {
        resourceData = HuDataSelHeapReadNum(motionIndex + DATA_MAKE_NUM(DATADIR_M412, 0x17), HU_MEMNUM_OVL, HEAP_MODEL);
        sceneModelSet->characterMotions[motionIndex] = Hu3DJointMotion(modelId, resourceData);
    }
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x16), HU_MEMNUM_OVL, HEAP_MODEL);
    sceneModelSet->reflectedModelId = modelId = Hu3DModelCreate(resourceData);

    for (motionIndex = 0; motionIndex < 5; motionIndex++) {
        resourceData = HuDataSelHeapReadNum(motionIndex + DATA_MAKE_NUM(DATADIR_M412, 0x17), HU_MEMNUM_OVL, HEAP_MODEL);
        sceneModelSet->jointMotions[motionIndex] = Hu3DJointMotion(modelId, resourceData);
    }
    Hu3DModelPosSet(modelId, sceneModelSet->position.x, sceneModelSet->position.y - 10.0f, sceneModelSet->position.z);
    Hu3DModelRotSet(modelId, sceneModelSet->rotation.x, sceneModelSet->rotation.y, sceneModelSet->rotation.z);
    Hu3DModelScaleSet(modelId, 1.0f, -1.0f, 1.0f);
    Hu3DModelLayerSet(modelId, 0);
    Hu3DModelShadowReset(modelId);
    Hu3DModelAttrSet(modelId, HU3D_ATTR_CULL_FRONT);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x04), HU_MEMNUM_OVL, HEAP_MODEL);
    sceneModelSet->effectModelId = modelId = Hu3DModelCreate(resourceData);
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelShadowSet(modelId);
    Hu3DModelShadowDispOff(modelId);
    sceneModelSet->linkedModelId = Hu3DModelLink(sceneModelSet->effectModelId);
    Hu3DModelLayerSet(sceneModelSet->linkedModelId, 0);
    Hu3DModelAttrSet(sceneModelSet->linkedModelId, HU3D_ATTR_DISPOFF);
    resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x05), HU_MEMNUM_OVL, HEAP_MODEL);
    sceneModelSet->auxiliaryModelId = modelId = Hu3DModelCreate(resourceData);
    Hu3DModelShadowSet(modelId);
    Hu3DModelShadowDispOff(modelId);
    Hu3DModelLayerSet(modelId, 1);
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);

    for (index = 0; index < 0x17; index++) {
        meshObject = Hu3DModelObjPtrGet(modelId, fragmentObjectNames[index]);
        if (!meshObject) {
            OSReport("objname %s\n", fragmentObjectNames[index]);
            OSPanic("m412.c", 0x258, "ObjPtr");
        }
        meshTransform = &meshObject->mesh.base;
        fragmentOriginalPositions[index].x = meshTransform->pos.x;
        fragmentOriginalPositions[index].y = meshTransform->pos.y;
        fragmentOriginalPositions[index].z = meshTransform->pos.z;
        fragmentOriginalScales[index].x = meshTransform->scale.x;
        fragmentOriginalScales[index].y = meshTransform->scale.y;
        fragmentOriginalScales[index].z = meshTransform->scale.z;
    }
    process = HuPrcChildCreate(RunPlayerAnimationSequence, 0x2000, 0x3000, 0, HuPrcCurrentGet());
    process->user_data = sceneModelSet;

    for (index = 0; index < 4; index++) {
        playerStates[index].circleSlot = index;
    }

    for (index = 0; index < 50; index++) {
        shuffleIndexA = frandmod(4);
        shuffleIndexB = frandmod(4);
        shuffleValue = playerStates[shuffleIndexB].circleSlot;
        playerStates[shuffleIndexB].circleSlot = playerStates[shuffleIndexA].circleSlot;
        playerStates[shuffleIndexA].circleSlot = shuffleValue;
    }

    for (index = 0; index < 4; index++) {
        playerState = &playerStates[index];
        playerState->playerIndex = index;
        playerState->selectionOrder = index;
        playerState->padIndex = GWPlayerCfg[index].padIdx;
        playerState->characterModelId = CharModelCreate(characterModelIndices[GWPlayerCfg[index].character], 4);
        Hu3DModelLayerSet(playerState->characterModelId, 1);
        resourceData = HuDataSelHeapReadNum(characterModelResources[GWPlayerCfg[index].character], HU_MEMNUM_OVL, HEAP_MODEL);
        playerState->reflectedModelId = Hu3DModelCreate(resourceData);
        Hu3DModelScaleSet(playerState->reflectedModelId, 1.0f, -1.0f, 1.0f);
        Hu3DModelAttrSet(playerState->reflectedModelId, HU3D_ATTR_CULL_FRONT);
        Hu3DModelLayerSet(playerState->reflectedModelId, 0);
        CharEffectLayerSet(3);
        playerState->rotation.x = playerState->rotation.z = playerState->rotation.y = 0.0f;
        playerState->rotation.y = WrapDegrees(45.0f + (90.0f * playerState->circleSlot) + cameraYaw);
        playerState->position.x = (400.0 * sind(playerState->rotation.y));
        playerState->position.z = (400.0 * cosd(playerState->rotation.y));
        playerState->rotation.y = WrapDegrees(180.0f + playerState->rotation.y);
        playerState->headingAngle = playerState->rotation.y;
        playerState->position.y = 0.0f;
        playerState->collisionParameters.x = 50.0f;
        playerState->collisionParameters.z = playerState->collisionParameters.y = 0.0f;
        HuSetVecF(&playerState->velocity, 0.0, 0.0, 0.0);
        Hu3DModelPosSet(playerState->characterModelId, playerState->position.x, playerState->position.y, playerState->position.z);
        Hu3DModelRotSet(playerState->characterModelId, playerState->rotation.x, playerState->rotation.y, playerState->rotation.z);
        Hu3DModelPosSet(playerState->reflectedModelId, playerState->position.x, playerState->position.y, playerState->position.z);
        Hu3DModelRotSet(playerState->reflectedModelId, playerState->rotation.x, playerState->rotation.y, playerState->rotation.z);
        Hu3DModelShadowSet(playerState->characterModelId);

        for (motionIndex = 0; motionIndex < 6; motionIndex++) {
            playerState->characterMotions[motionIndex] = CharMotionCreate(characterModelIndices[GWPlayerCfg[index].character], playerMotionIndices[motionIndex]);
            resourceData
                = HuDataSelHeapReadNum((u16)playerMotionIndices[motionIndex] | characterMotionResources[GWPlayerCfg[index].character], HU_MEMNUM_OVL, HEAP_MODEL);
            playerState->jointMotions[motionIndex] = Hu3DJointMotion(playerState->reflectedModelId, resourceData);
        }
        CharMotionSet(characterModelIndices[GWPlayerCfg[index].character], playerState->characterMotions[0]);
        Hu3DModelAttrSet(playerState->characterModelId, HU3D_MOTATTR_LOOP);
        Hu3DMotionSet(playerState->reflectedModelId, playerState->jointMotions[0]);
        Hu3DModelAttrSet(playerState->reflectedModelId, HU3D_MOTATTR_LOOP);
        CharMotionDataClose(characterModelIndices[GWPlayerCfg[index].character]);
        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x1C), HU_MEMNUM_OVL, HEAP_MODEL);
        playerState->effectModelId = modelId = Hu3DModelCreate(resourceData);
        Hu3DModelLayerSet(modelId, 1);
        Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
        Hu3DModelShadowSet(modelId);
        Hu3DModelShadowDispOff(modelId);
        playerState->auxiliaryModelId = modelId = Hu3DModelLink(modelId);
        Hu3DModelLayerSet(modelId, 0);
        Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF | HU3D_ATTR_CULL_FRONT);
        resourceData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x1D), HU_MEMNUM_OVL, HEAP_MODEL);
        playerState->extraModelId = modelId = Hu3DModelCreate(resourceData);
        Hu3DModelLayerSet(modelId, 3);
        Hu3DModelAttrSet(modelId, HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
        process = HuPrcChildCreate(RunPlayerControlProcess, 0x2000, 0x4000, 0, HuPrcCurrentGet());
        process->user_data = playerState;
    }
    animationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M412, 0x1E), HU_MEMNUM_OVL));
    modelId = Hu3DParticleCreate(animationData, 0x100);
    Hu3DModelPosSet(modelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(modelId, 1.0f, 1.0f, 1.0f);
    Hu3DModelLayerSet(modelId, 4);
    Hu3DParticleColSet(modelId, 0xFF, 0xFF, 0xFF);
    Hu3DParticleScaleSet(modelId, 0.0f);
    Hu3DParticleHookSet(modelId, UpdateDebrisParticles);
    animationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 0x03), HU_MEMNUM_OVL));
    modelId = Hu3DParticleCreate(animationData, 0x40);
    Hu3DModelPosSet(modelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(modelId, 1.0f, 1.0f, 1.0f);
    Hu3DModelLayerSet(modelId, 4);
    Hu3DParticleColSet(modelId, 0xFF, 0xFF, 0xFF);
    Hu3DParticleScaleSet(modelId, 0.0f);
    Hu3DParticleHookSet(modelId, UpdateSparkParticles);
    animationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 0x03), HU_MEMNUM_OVL));
    modelId = Hu3DParticleCreate(animationData, 3);
    Hu3DModelPosSet(modelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(modelId, 1.0f, 1.0f, 1.0f);
    Hu3DModelLayerSet(modelId, 4);
    Hu3DParticleColSet(modelId, 0xFF, 0xFF, 0xFF);
    Hu3DParticleScaleSet(modelId, 0.0f);
    Hu3DParticleHookSet(modelId, UpdateShardParticles);
    HuDataDirClose(DATADIR_EFFECT);
    HuPrcChildCreate(WaitForMinigameExit, 0x1000, 0x1000, 0, HuPrcCurrentGet());
}

f32 WrapDegrees(f32 angleDegrees)
{
    while (angleDegrees >= 360.0f || angleDegrees < 0.0f) {
        if (angleDegrees >= 360.0f) {
            angleDegrees -= 360.0f;
        }
        if (angleDegrees < 0.0f) {
            angleDegrees += 360.0f;
        }
    }
    return angleDegrees;
}

void FreeShardAnimationData(void)
{
    void *animationData;

    animationData = HuPrcCurrentGet()->user_data;
    HuMemDirectFree(animationData);
}

void AnimateShardFragment(void)
{
    Vec scaledPosition;
    HSFOBJECT *meshObject;
    HSFTRANSFORM *meshTransform;
    ShardAnimationData *animationData;
    Vec *originalScale;
    s32 frame;
    f32 verticalAcceleration;
    f32 fadeAlpha;

    animationData = HuPrcCurrentGet()->user_data;
    HuPrcDestructorSet(FreeShardAnimationData);
    meshObject = Hu3DModelObjPtrGet(animationData->modelId, fragmentObjectNames[animationData->objectIndex]);
    if (!meshObject) {
        OSReport("objname %s\n", fragmentObjectNames[animationData->objectIndex]);
        OSPanic("m412.c", 0x32E, "ObjPtr");
    }
    meshTransform = &meshObject->mesh.base;
    originalScale = &fragmentOriginalPositions[animationData->objectIndex];
    meshTransform->pos.x = originalScale->x;
    meshTransform->pos.y = originalScale->y;
    meshTransform->pos.z = originalScale->z;
    HuSetVecF(&scaledPosition, 0.15f * meshTransform->pos.x, 0.15f * meshTransform->pos.y, 0.15f * meshTransform->pos.z);
    originalScale = &fragmentOriginalScales[animationData->objectIndex];
    meshTransform->scale.x = originalScale->x;
    meshTransform->scale.y = originalScale->y;
    meshTransform->scale.z = originalScale->z;
    for (frame = 0, verticalAcceleration = -2.0f, fadeAlpha = 1.0f; frame < 0x1E; frame++) {
        scaledPosition.y -= verticalAcceleration;
        verticalAcceleration += 0.5f;
        meshTransform->pos.x += scaledPosition.x;
        meshTransform->pos.y += scaledPosition.y;
        meshTransform->pos.z += scaledPosition.z;
        meshTransform->scale.x *= 0.95f;
        meshTransform->scale.y *= 0.95f;
        meshTransform->scale.z *= 0.95f;
        if (animationData->objectIndex == 0) {
            fadeAlpha -= 0.033333335f;
            Hu3DModelTPLvlSet(animationData->modelId, fadeAlpha);
        }
        HuPrcVSleep();
    }
    animationData->animationActive = 0;
    HuPrcVSleep();
    HuPrcKill(HuPrcCurrentGet());
    while (1) {
        HuPrcVSleep();
    }
}

void ReleaseDebrisSlot(void)
{
    DebrisPiece *debris;

    debris = (DebrisPiece *)HuPrcCurrentGet()->user_data;
    debrisSlots[debris->slotIndex] = NULL;
    HuMemDirectFree(debris);
}

void RunDebrisPhysics(void)
{
    Vec debrisPosition;
    Vec otherDebrisPosition;
    Vec separationVector;
    f32 verticalPosition;
    f32 horizontalDistance;
    HUPROCESS *animationProcess;
    s32 debrisIndex;
    DebrisPiece *otherDebris;
    PlayerState *playerState;
    DebrisPiece *debris;
    ShardAnimationData *shardAnimation;

    debris = (DebrisPiece *)HuPrcCurrentGet()->user_data;
    debris->hasCollided = 0;
    debris->particleSpawnCount = 0;
    HuPrcDestructorSet(ReleaseDebrisSlot);

    do {
        if ((debris->hasCollided != 0) || (roundState >= 4))
            break;
        debris->position.x += (debris->launchVelocity.y * sind(debris->launchVelocity.x));
        debris->position.z += (debris->launchVelocity.y * cosd(debris->launchVelocity.x));
        debris->position.y += debris->launchVelocity.z;
        debris->launchVelocity.z -= 0.9f;
        if (debris->collisionRadius != 100.0f) {
            HuSetVecF(&debrisPosition, debris->position.x, debris->position.y, debris->position.z);
            for (debrisIndex = 0; debrisIndex < 0x20; debrisIndex++) {
                otherDebris = debrisSlots[debrisIndex];
                if ((debrisIndex != debris->slotIndex) && otherDebris && (otherDebris->slotIndex != -1)) {
                    HuSetVecF(&otherDebrisPosition, otherDebris->position.x, otherDebris->position.y, otherDebris->position.z);
                    HuSubVecF(&separationVector, &debrisPosition, &otherDebrisPosition);
                    if (sqrtf((separationVector.z * separationVector.z) + ((separationVector.x * separationVector.x) + (separationVector.y * separationVector.y))) < (debris->collisionRadius + otherDebris->collisionRadius)) {
                        if (otherDebris->collisionRadius != 100.0f) {
                            otherDebris->hasCollided = 1;
                        }
                        debris->hasCollided = 1;
                    }
                }
            }
        }
        HuSetVecF(&debrisPosition, debris->position.x, 0.0, debris->position.z);

        for (debrisIndex = 0; debrisIndex < 4; debrisIndex++) {
            playerState = &playerStates[debrisIndex];
            HuSetVecF(&otherDebrisPosition, playerState->position.x, 0.0, playerState->position.z);
            HuSubVecF(&separationVector, &debrisPosition, &otherDebrisPosition);
            if ((sqrtf((separationVector.z * separationVector.z) + ((separationVector.x * separationVector.x) + (separationVector.y * separationVector.y))) < (debris->collisionRadius + playerState->collisionParameters.x))
                && ((150.0f + playerState->position.y + debris->collisionRadius) > debris->position.y)) {

                if (playerState->roundStatus != 3) {
                    if (debris->debrisType == 0) {
                        omVibrate(playerState->playerIndex, 0xC, 4, 2);
                    }
                    else {
                        omVibrate(playerState->playerIndex, 0xC, 0xC, 0);
                    }
                }
                HuAudFXPlay(0x585);
                playerState->roundStatus = 3;
                debris->hasCollided = 1;
            }
        }
        if (debris->position.y < debris->collisionRadius) {
            debris->position.y = debris->collisionRadius;
            debris->hasCollided = 1;
        }
        Hu3DModelPosSet(debris->modelId, debris->position.x, debris->position.y, debris->position.z);
        Hu3DModelPosSet(debris->reflectedModelId, debris->position.x, -debris->position.y, debris->position.z);
        if (debris->hasCollided != 0)
            break;
        HuPrcVSleep();
    } while (1);
    if (debris->debrisType == 1) {
        HuAudFXPlay(0x58D);
    }
    else {
        HuAudFXPlay(0x58C);
    }
    Hu3DModelAttrSet(debris->modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(debris->reflectedModelId, HU3D_ATTR_DISPOFF);
    Hu3DModelShadowDispOff(debris->modelId);
    debris->particleSpawnCount = 1;
    if (debris->debrisType == 1) {

        for (debrisIndex = 0; debrisIndex < 4; debrisIndex++) {
            if (playerStates[debrisIndex].roundStatus != 3) {
                omVibrate(playerStates[debrisIndex].playerIndex, 0xC, 6, 6);
            }
        }
        Hu3DModelPosSet(debris->settledModelId, debris->position.x, debris->position.y, debris->position.z);
        Hu3DModelAttrReset(debris->settledModelId, HU3D_ATTR_DISPOFF);
        Hu3DModelShadowDispOn(debris->settledModelId);

        for (debrisIndex = 0; debrisIndex < 0x17; debrisIndex++) {
            animationProcess = HuPrcChildCreate(AnimateShardFragment, 0x2000, 0x1000, 0, HuPrcCurrentGet());
            shardAnimation = HuMemDirectMalloc(HEAP_MODEL, sizeof(ShardAnimationData));
            animationProcess->user_data = shardAnimation;
            shardAnimation->modelId = debris->settledModelId;
            shardAnimation->objectIndex = debrisIndex;
            shardAnimation->animationActive = 1;
            shardAnimation->originPosition.x = debris->position.x;
            shardAnimation->originPosition.y = debris->position.y;
            shardAnimation->originPosition.z = debris->position.z;
        }
        while (shardAnimation->animationActive != 0) {
            HuPrcVSleep();
        }
    }
    HuPrcVSleep();
    HuPrcKill(HuPrcCurrentGet());
    while (1) {
        HuPrcVSleep();
    }
}

s32 AllocateDebrisSlot(void)
{
    s32 slotIndex;

    for (slotIndex = 0; slotIndex < 0x20; slotIndex++) {
        if (!debrisSlots[slotIndex])
            break;
    }
    if (slotIndex >= 0x20) {
        slotIndex = -1;
    }
    debrisSlots[slotIndex] = HuMemDirectMalloc(HEAP_MODEL, sizeof(DebrisPiece));
    debrisSlots[slotIndex]->slotIndex = -1;
    debrisSlots[slotIndex]->particleSpawnCount = 0;
    return slotIndex;
}

f32 CalculateAngularStep(s32 frameCount, f32 targetAngle, f32 currentAngle)
{
    f32 angleDifference;
    f32 unusedAngleValue25;
    f32 unusedAngleValue26;
    f32 unusedAngleValue27;
    f32 unusedAngleValue28;
    f32 angularStep;

    angleDifference = targetAngle - WrapDegrees(currentAngle);
    if ((angleDifference < 0.0f) || (ABS_INV(angleDifference) > 180.0f)) {
        if (ABS_INV(angleDifference) > 180.0f) {
            angularStep = (360.0f - ABS_INV(angleDifference)) / frameCount;
            if (angleDifference > 0.0f) {
                angularStep = -angularStep;
            }
        }
        else {
            angularStep = angleDifference / frameCount;
        }
    }
    else {
        angularStep = angleDifference / frameCount;
    }
    return angularStep;
}

s32 collisionLookupTable[6][2] = { { 0x09249222, 0x22210842 }, { 0x04924888, 0x88842108 }, { 0x12492222, 0x22108420 }, { 0x04444444, 0x44444400 },
    { 0x11111111, 0x11100000 }, { 0x04444444, 0x40000000 } };

s32 collisionStateLookup[1][2] = { { 0x00040008, 0x02000000 } };

s32 collisionRowOffsets[6] = { 0x0, 0x0, 0x0, 0x8, 0x12, 0x1C };

void RunGameplaySequence(void)
{
    Vec scenePosition;
    Vec targetPosition;
    Vec separationVector;
    Vec throwParameters;
    f32 initialVerticalSpeed;
    f32 targetX;
    f32 targetAngle;
    f32 throwDistance;
    f32 predictedHeight;
    f32 verticalSpeed;
    s32 lookupTableRow;
    s32 candidateOffset;
    s32 activationFrame;
    s32 soundFrame;
    s32 targetPlayerIndex;
    s32 motionFrame;
    DebrisPiece *debris;
    PlayerState *targetPlayer;
    SceneModelSet *sceneModelSet;
    s32 debrisSlot;
    HUPROCESS *debrisProcess;

    sceneModelSet = HuPrcCurrentGet()->user_data;
    switch (sceneModelSet->playerIndex) {
        case 0:
            activationFrame = 0x142;
            soundFrame = 0x154;
            break;
        case 1:
            activationFrame = 0x14A;
            soundFrame = 0x15E;
            break;
        case 2:
            activationFrame = 0x148;
            soundFrame = 0x162;
            break;
        default:
            activationFrame = 0x4E;
            soundFrame = 0x5D;
            break;
    }

loop10:
    motionFrame = roundTimer / REFRESH_RATE;
    if (motionFrame >= 0x3C) {
        motionFrame = 0x3C;
    }
    if (motionFrame == collisionRowOffsets[sceneModelSet->selectionOrder]) {
        Hu3DModelAttrReset(sceneModelSet->primaryModelId, HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(sceneModelSet->reflectedModelId, HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(sceneModelSet->detailModelId, HU3D_MOTATTR_PAUSE);
    }
    if (activationFrame == Hu3DMotionTimeGet(sceneModelSet->primaryModelId)) {
        sceneModelSet->activeState = 1;
    }
    if (soundFrame == Hu3DMotionTimeGet(sceneModelSet->primaryModelId)) {
        HuAudFXPlay(0x581);
    }
    if (roundState >= 4) {
        while (1) {
            HuPrcVSleep();
        }
    }
    lookupTableRow = 1;
    if (motionFrame >= 0x20) {
        lookupTableRow = 0;
    }
    if (((1 << (motionFrame % 32)) & collisionLookupTable[sceneModelSet->selectionOrder][lookupTableRow]) != 0) {
        debrisSlot = AllocateDebrisSlot();
    }
    else {
        HuPrcVSleep();
        goto loop10;
    }
    targetPlayerIndex = frandmod(4);
    if (playerStates[targetPlayerIndex].roundStatus == 3) {
        for (candidateOffset = 3; candidateOffset != 0; candidateOffset--) {
            targetPlayerIndex++;
            if (targetPlayerIndex >= 4) {
                targetPlayerIndex = 0;
            }
            if (playerStates[targetPlayerIndex].roundStatus != 3)
                break;
        }
    }
    targetPlayer = &playerStates[targetPlayerIndex];
    Hu3DMotionSet(sceneModelSet->primaryModelId, sceneModelSet->characterMotions[1]);
    Hu3DMotionSet(sceneModelSet->reflectedModelId, sceneModelSet->characterMotions[1]);
    Hu3DModelAttrReset(sceneModelSet->primaryModelId, HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(sceneModelSet->reflectedModelId, HU3D_MOTATTR_LOOP);
    motionFrame = Hu3DMotionMaxTimeGet(sceneModelSet->primaryModelId);
    targetAngle = WrapDegrees(atan2d(targetPlayer->position.x - sceneModelSet->position.x, targetPlayer->position.z - sceneModelSet->position.z));
    targetX = (targetPlayer->position.x + (50.0 * sind(targetAngle)));
    throwParameters.y = (targetPlayer->position.z + (50.0 * cosd(targetAngle)));
    throwParameters.x = 360.0f * frandf();
    throwDistance = 50.0f * frandf();
    targetX += (throwDistance * sind(throwParameters.x));
    throwParameters.y += (throwDistance * cosd(throwParameters.x));
    throwParameters.z = CalculateAngularStep(motionFrame, targetAngle, sceneModelSet->rotation.y);
    HuAudFXPlay(0x583);

    for (; motionFrame != 0; motionFrame--) {
        sceneModelSet->rotation.y += throwParameters.z;
        Hu3DModelRotSet(sceneModelSet->primaryModelId, sceneModelSet->rotation.x, sceneModelSet->rotation.y, sceneModelSet->rotation.z);
        Hu3DModelRotSet(sceneModelSet->reflectedModelId, sceneModelSet->rotation.x, sceneModelSet->rotation.y, sceneModelSet->rotation.z);
        HuPrcVSleep();
    }
    HuPrcSleep(0x1E);
    Hu3DModelAttrReset(sceneModelSet->effectModelId, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(sceneModelSet->linkedModelId, HU3D_ATTR_DISPOFF);
    Hu3DModelShadowDispOn(sceneModelSet->effectModelId);
    Hu3DModelPosSet(sceneModelSet->effectModelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(sceneModelSet->linkedModelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelHookSet(sceneModelSet->primaryModelId, "itemhook", sceneModelSet->effectModelId);
    Hu3DModelHookSet(sceneModelSet->reflectedModelId, "itemhook", sceneModelSet->linkedModelId);
    Hu3DMotionShiftSet(sceneModelSet->primaryModelId, sceneModelSet->characterMotions[2], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    Hu3DMotionShiftSet(sceneModelSet->reflectedModelId, sceneModelSet->characterMotions[2], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(10);
    while (Hu3DMotionTimeGet(sceneModelSet->primaryModelId) < 18.0f) {
        HuPrcVSleep();
    }
    Hu3DModelHookReset(sceneModelSet->primaryModelId);
    Hu3DModelHookReset(sceneModelSet->reflectedModelId);
    HuAudFXPlay(0x582);
    debris = debrisSlots[debrisSlot];
    debris->slotIndex = debrisSlot;
    debris->particleSpawnCount = 0;
    debrisProcess = HuPrcChildCreate(RunDebrisPhysics, 0x1000, 0x3000, 0, HuPrcCurrentGet());
    debrisProcess->user_data = debris;
    debris->debrisType = 0;
    debris->modelId = sceneModelSet->effectModelId;
    debris->reflectedModelId = sceneModelSet->linkedModelId;
    debris->collisionRadius = 25.0f;
    debris->position.x = -89.937f;
    debris->position.y = 125.861f;
    debris->position.z = 43.181f;
    debris->velocity.x = targetX;
    debris->velocity.y = throwParameters.y;
    targetAngle = WrapDegrees(atan2d(debris->position.x, debris->position.z));
    targetAngle += sceneModelSet->rotation.y;
    throwDistance = sqrtf((debris->position.x * debris->position.x) + (debris->position.z * debris->position.z));
    debris->position.x = (sceneModelSet->position.x + (throwDistance * sind(targetAngle)));
    debris->position.y = 125.861f + sceneModelSet->position.y;
    debris->position.z = (sceneModelSet->position.z + (throwDistance * cosd(targetAngle)));
    Hu3DModelPosSet(sceneModelSet->effectModelId, debris->position.x, debris->position.y, debris->position.z);
    Hu3DModelPosSet(sceneModelSet->linkedModelId, debris->position.x, -debris->position.y, debris->position.z);
    HuSetVecF(&scenePosition, sceneModelSet->position.x, 0.0, sceneModelSet->position.z);
    HuSetVecF(&targetPosition, targetX, 0.0, throwParameters.y);
    HuSubVecF(&separationVector, &scenePosition, &targetPosition);
    throwDistance = sqrtf((separationVector.x * separationVector.x) + (separationVector.z * separationVector.z));
    initialVerticalSpeed = 1.0f;
    while (1) {
        for (motionFrame = 0, predictedHeight = debris->position.y, verticalSpeed = initialVerticalSpeed; verticalSpeed > 0.0f; motionFrame++) {
            predictedHeight += verticalSpeed;
            verticalSpeed -= 0.9f;
        }
        if (predictedHeight >= (throwDistance / 2.0f))
            break;
        initialVerticalSpeed++;
    }

    for (; predictedHeight > 0.0f; motionFrame++) {
        predictedHeight += verticalSpeed;
        verticalSpeed -= 0.9f;
    }
    debris->launchVelocity.z = initialVerticalSpeed;
    debris->launchVelocity.x = WrapDegrees(atan2d(targetX - debris->position.x, throwParameters.y - debris->position.z));
    debris->launchVelocity.y = throwDistance / motionFrame;

    while (Hu3DMotionEndCheck(sceneModelSet->primaryModelId) == 0) {
        HuPrcVSleep();
    }
    goto loop10;
}

void RunPlayerAnimationSequence(void)
{
    Vec playerPosition;
    Vec collisionPosition;
    Vec separationVector;
    f32 rotationStep;
    f32 turnDuration;
    f32 motionTime;
    f32 verticalAcceleration;
    f32 horizontalDistance;
    f32 verticalPosition;
    volatile f32 verticalSpeed;
    f32 initialVerticalSpeed;
    f32 targetAngle;
    f32 currentAngle;
    f32 targetX;
    f32 targetZ;
    f32 effectHeight;
    f32 distance;
    f32 rotationDifference;
    f32 targetDistance;
    f32 turnAngle;
    f32 motionAngle;
    f32 effectDistance;
    s32 playerIndex;
    s32 lookupTableColumn;
    s32 playerCount;
    s32 timer;
    s32 motionFrame;
    DebrisPiece *debris;
    PlayerState *playerState;
    SceneModelSet *sceneModelSet;
    s32 debrisSlot;
    HUPROCESS *debrisProcess;

    sceneModelSet = HuPrcCurrentGet()->user_data;
loop1:
    timer = roundTimer / REFRESH_RATE;
    if (timer >= 0x3C) {
        timer = 0x3C;
    }
    if (roundState >= 4) {
        while (1) {
            HuPrcVSleep();
        }
    }
    lookupTableColumn = 1;
    if (timer >= 0x20) {
        lookupTableColumn = 0;
    }
    if (((1 << (timer % 32)) & collisionStateLookup[sceneModelSet->selectionOrder][lookupTableColumn]) != 0) {
        debrisSlot = AllocateDebrisSlot();
    }
    else {
        HuPrcVSleep();
        goto loop1;
    }
    Hu3DModelAttrReset(sceneModelSet->primaryModelId, HU3D_MOTATTR_PAUSE);
    Hu3DModelClusterAttrReset(sceneModelSet->primaryModelId, 0, -0x3FFFFFFE);
    Hu3DModelAttrReset(sceneModelSet->reflectedModelId, HU3D_MOTATTR_PAUSE);
    Hu3DModelClusterAttrReset(sceneModelSet->reflectedModelId, 0, -0x3FFFFFFE);
    Hu3DModelAttrReset(sceneModelSet->effectModelId, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(sceneModelSet->linkedModelId, HU3D_ATTR_DISPOFF);
    Hu3DModelShadowDispOn(sceneModelSet->effectModelId);
    effectHeight = 850.0f;
    targetX = -217.722f;
    targetZ = 0.0f;
    currentAngle = WrapDegrees(atan2d(targetX, targetZ));
    currentAngle += sceneModelSet->rotation.y;
    effectDistance = sqrtf((targetX * targetX) + (targetZ * targetZ));
    targetX = (sceneModelSet->position.x + (effectDistance * sind(currentAngle)));
    targetZ = (sceneModelSet->position.z + (effectDistance * cosd(currentAngle)));
    verticalAcceleration = 5.0f;
    while (1) {
        effectHeight -= verticalAcceleration;
        verticalAcceleration += 0.9f;
        if (verticalAcceleration > 20.0f) {
            verticalAcceleration = 20.0f;
        }
        if (effectHeight < (304.52f + sceneModelSet->position.y)) {
            effectHeight = 304.52f + sceneModelSet->position.y;
        }
        Hu3DModelPosSet(sceneModelSet->effectModelId, targetX, effectHeight, targetZ);
        Hu3DModelPosSet(sceneModelSet->linkedModelId, targetX, -effectHeight, targetZ);
        if (effectHeight == (304.52f + sceneModelSet->position.y))
            break;
        HuPrcVSleep();
    }
    Hu3DModelPosSet(sceneModelSet->effectModelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(sceneModelSet->linkedModelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelHookSet(sceneModelSet->primaryModelId, "itemhook", sceneModelSet->effectModelId);
    Hu3DModelHookSet(sceneModelSet->reflectedModelId, "itemhook", sceneModelSet->linkedModelId);
    Hu3DMotionShiftSet(sceneModelSet->primaryModelId, sceneModelSet->characterMotions[2], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    Hu3DMotionShiftSet(sceneModelSet->reflectedModelId, sceneModelSet->jointMotions[2], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(10);

    while (Hu3DMotionEndCheck(sceneModelSet->primaryModelId) == 0) {
        HuPrcVSleep();
    }
    HuPrcSleep(0xA);
    motionFrame = frandmod(4);
    if (playerStates[motionFrame].roundStatus == 3) {
        for (playerCount = 3; playerCount != 0; playerCount--) {
            motionFrame += 1;
            if (motionFrame >= 4) {
                motionFrame = 0;
            }
            if (playerStates[motionFrame].roundStatus != 3)
                break;
        }
    }
    playerState = &playerStates[motionFrame];
    Hu3DMotionSet(sceneModelSet->primaryModelId, sceneModelSet->characterMotions[3]);
    Hu3DModelAttrReset(sceneModelSet->primaryModelId, HU3D_MOTATTR_LOOP);
    Hu3DMotionSet(sceneModelSet->reflectedModelId, sceneModelSet->jointMotions[3]);
    Hu3DModelAttrReset(sceneModelSet->reflectedModelId, HU3D_MOTATTR_LOOP);
    timer = Hu3DMotionMaxTimeGet(sceneModelSet->primaryModelId);
    currentAngle = WrapDegrees(atan2d(playerState->position.x - sceneModelSet->position.x, playerState->position.z - sceneModelSet->position.z));
    turnDuration = (playerState->position.x + (50.0 * sind(currentAngle)));
    motionTime = (playerState->position.z + (50.0 * cosd(currentAngle)));
    horizontalDistance = 360.0f * frandf();
    effectDistance = 50.0f * frandf();
    turnDuration += (effectDistance * sind(horizontalDistance));
    motionTime += (effectDistance * cosd(horizontalDistance));
    rotationStep = CalculateAngularStep(timer, currentAngle, sceneModelSet->rotation.y);
    HuAudFXPlay(0x587);

    for (; timer != 0; timer--) {
        sceneModelSet->rotation.y += rotationStep;
        Hu3DModelRotSet(sceneModelSet->primaryModelId, sceneModelSet->rotation.x, sceneModelSet->rotation.y, sceneModelSet->rotation.z);
        Hu3DModelRotSet(sceneModelSet->reflectedModelId, sceneModelSet->rotation.x, sceneModelSet->rotation.y, sceneModelSet->rotation.z);
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    Hu3DMotionShiftSet(sceneModelSet->primaryModelId, sceneModelSet->characterMotions[4], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    Hu3DMotionShiftSet(sceneModelSet->reflectedModelId, sceneModelSet->jointMotions[4], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(10);

    while (Hu3DMotionTimeGet(sceneModelSet->primaryModelId) < 24.0f) {
        HuPrcVSleep();
    }
    Hu3DModelHookReset(sceneModelSet->primaryModelId);
    Hu3DModelHookReset(sceneModelSet->reflectedModelId);
    HuAudFXPlay(0x586);
    debris = debrisSlots[debrisSlot];
    debris->slotIndex = debrisSlot;
    debris->particleSpawnCount = 0;
    debrisProcess = HuPrcChildCreate(RunDebrisPhysics, 0x1000, 0x3000, 0, HuPrcCurrentGet());
    debrisProcess->user_data = debris;
    debris->debrisType = 1;
    debris->modelId = sceneModelSet->effectModelId;
    debris->reflectedModelId = sceneModelSet->linkedModelId;
    debris->settledModelId = sceneModelSet->auxiliaryModelId;
    debris->collisionRadius = 100.0f;
    debris->position.x = -217.722f;
    debris->position.y = 404.52f;
    debris->position.z = 0.0f;
    currentAngle = WrapDegrees(atan2d(debris->position.x, debris->position.z));
    currentAngle += sceneModelSet->rotation.y;
    effectDistance = sqrtf((debris->position.x * debris->position.x) + (debris->position.z * debris->position.z));
    debris->position.x = (sceneModelSet->position.x + (effectDistance * sind(currentAngle)));
    debris->position.y = (404.52f + sceneModelSet->position.y) - 100.0f;
    debris->position.z = (sceneModelSet->position.z + (effectDistance * cosd(currentAngle)));
    Hu3DModelPosSet(sceneModelSet->effectModelId, debris->position.x, debris->position.y, debris->position.z);
    Hu3DModelPosSet(sceneModelSet->linkedModelId, debris->position.x, -debris->position.y, debris->position.z);
    HuSetVecF(&playerPosition, sceneModelSet->position.x, 0.0, sceneModelSet->position.z);
    HuSetVecF(&collisionPosition, turnDuration, 0.0, motionTime);
    HuSubVecF(&separationVector, &playerPosition, &collisionPosition);
    effectDistance = sqrtf((separationVector.x * separationVector.x) + (separationVector.z * separationVector.z));
    initialVerticalSpeed = 1.0f;

    while (1) {
        for (timer = 0, verticalPosition = debris->position.y, verticalSpeed = initialVerticalSpeed; verticalSpeed > 0.0f; timer++) {
            verticalPosition += verticalSpeed;
            verticalSpeed -= 0.9f;
        }
        if (verticalPosition >= 600.0f)
            break;
        initialVerticalSpeed++;
    }
    for (; verticalPosition > 0.0f; timer++) {
        verticalPosition += verticalSpeed;
        verticalSpeed -= 0.9f;
    }
    debris->launchVelocity.z = initialVerticalSpeed;
    debris->launchVelocity.x = WrapDegrees(atan2d((turnDuration - debris->position.x), (motionTime - debris->position.z)));
    debris->launchVelocity.y = effectDistance / timer;
    while (Hu3DMotionEndCheck(sceneModelSet->primaryModelId) == 0) {
        HuPrcVSleep();
    }
    goto loop1;
}

s8 CheckPlayerCollision(s32 playerIndex, s32 useVerticalAxis)
{
    s8 stickInput;
    PlayerState *playerState;

    playerState = &playerStates[playerIndex];
    if (GWPlayerCfg[playerIndex].iscom == 0) {
        (useVerticalAxis != 0) ? (stickInput = HuPadStkY[playerState->padIndex]) : (stickInput = HuPadStkX[playerState->padIndex]);
    }
    else {
        (useVerticalAxis != 0) ? (stickInput = playerState->stickY) : (stickInput = playerState->stickX);
    }
    return stickInput;
}

void RotateVectorTowardAngle(Vec *direction, f32 angleDegrees, f32 magnitude)
{
    f32 horizontalMagnitude;

    horizontalMagnitude = sqrtf((direction->x * direction->x) + (direction->z * direction->z));
    horizontalMagnitude *= magnitude;
    direction->x = (horizontalMagnitude * sind(angleDegrees));
    direction->z = (horizontalMagnitude * cosd(angleDegrees));
}

s8 CheckPlayerCollisionAlternate(s32 playerIndex, s32 useVerticalAxis)
{
    Vec transformedStick;
    f32 stickAngle;
    s8 stickInput;
    s8 horizontalInput;
    s8 verticalInput;
    PlayerState *playerState;

    playerState = &playerStates[playerIndex];
    if (GWPlayerCfg[playerIndex].iscom == 0) {
        horizontalInput = HuPadStkX[playerState->padIndex];
        verticalInput = HuPadStkY[playerState->padIndex];
        stickAngle = WrapDegrees(atan2d(horizontalInput, verticalInput));
        stickAngle -= cameraYaw;
        HuSetVecF(&transformedStick, (f32)horizontalInput, 0.0, (f32)verticalInput);
        RotateVectorTowardAngle(&transformedStick, stickAngle, 1.0f);
        (useVerticalAxis != 0) ? (stickInput = transformedStick.z) : (stickInput = transformedStick.x);
    }
    else {
        (useVerticalAxis != 0) ? (stickInput = playerState->stickY) : (stickInput = playerState->stickX);
    }
    return stickInput;
}

u8 GetPlayerCollisionState(s32 playerIndex, s32 useRightTrigger)
{
    s8 triggerInput;
    PlayerState *playerState;

    playerState = &playerStates[playerIndex];
    if (GWPlayerCfg[playerIndex].iscom == 0) {
        (useRightTrigger != 0) ? (triggerInput = HuPadTrigR[playerState->padIndex]) : (triggerInput = HuPadTrigL[playerState->padIndex]);
    }
    else {
        (useRightTrigger != 0) ? (triggerInput = playerState->triggerRight) : (triggerInput = playerState->triggerLeft);
    }
    return triggerInput;
}

u16 GetPlayerCollisionValue(s32 playerIndex)
{
    u16 buttonsPressed;
    PlayerState *playerState;

    playerState = &playerStates[playerIndex];
    if (GWPlayerCfg[playerIndex].iscom == 0) {
        buttonsPressed = HuPadBtnDown[playerState->padIndex];
    }
    else {
        buttonsPressed = playerState->buttonsPressed;
    }
    return buttonsPressed;
}

u16 GetPlayerCollisionFlags(s32 playerIndex)
{
    u16 buttonState;
    PlayerState *playerState;

    playerState = &playerStates[playerIndex];
    if (GWPlayerCfg[playerIndex].iscom == 0) {
        buttonState = HuPadBtn[playerState->padIndex];
    }
    else {
        buttonState = playerState->buttonState;
    }
    return buttonState;
}

s32 FindPlayerCollision(PlayerState *player, f32 xPosition, f32 yPosition, f32 zPosition)
{
    Vec candidatePosition;
    Vec playerPosition;
    Vec separationVector;
    PlayerState *otherPlayer;
    s32 playerIndex;
    s32 nearestPlayerIndex;
    f32 distance;
    f32 nearestDistance;

    nearestPlayerIndex = -1;

    for (playerIndex = 0, nearestDistance = 10000.0f; playerIndex < 4; playerIndex++) {
        if (playerIndex != player->playerIndex) {
            otherPlayer = &playerStates[playerIndex];
            HuSetVecF(&candidatePosition, xPosition, yPosition, zPosition);
            HuSetVecF(&playerPosition, otherPlayer->position.x, otherPlayer->position.y, otherPlayer->position.z);
            HuSubVecF(&separationVector, &candidatePosition, &playerPosition);
            if (((distance = sqrtf((separationVector.z * separationVector.z) + ((separationVector.x * separationVector.x) + (separationVector.y * separationVector.y)))) < (player->collisionParameters.x + otherPlayer->collisionParameters.x))
                && (distance < nearestDistance)) {
                nearestDistance = distance;
                nearestPlayerIndex = playerIndex;
                player->fleeFramesRemaining = 0;
            }
        }
    }
    return nearestPlayerIndex;
}

s32 FindNearestPlayer(PlayerState *player, s32 *nearbyPlayerCount)
{
    Vec sourcePosition;
    Vec candidatePosition;
    Vec separationVector;
    f32 distance;
    f32 nearestDistance;
    s32 nearestPlayerIndex;
    s32 nearbyCount;
    s32 playerIndex;
    PlayerState *otherPlayer;

    nearestPlayerIndex = -1;
    playerIndex = 0;
    nearestDistance = 10000.0f;
    nearbyCount = 0;

    for (; playerIndex < 4; playerIndex++) {
        if (playerIndex != player->playerIndex) {
            otherPlayer = &playerStates[playerIndex];
            HuSetVecF(&sourcePosition, player->position.x, player->position.y, player->position.z);
            HuSetVecF(&candidatePosition, otherPlayer->position.x, otherPlayer->position.y, otherPlayer->position.z);
            HuSubVecF(&separationVector, &sourcePosition, &candidatePosition);
            if ((distance = sqrtf((separationVector.z * separationVector.z) + ((separationVector.x * separationVector.x) + (separationVector.y * separationVector.y)))) < (player->collisionParameters.x + otherPlayer->collisionParameters.x)) {
                nearbyCount += 1;
                if (distance < nearestDistance) {
                    nearestDistance = distance;
                    nearestPlayerIndex = playerIndex;
                    player->fleeFramesRemaining = 0;
                }
            }
        }
    }
    *nearbyPlayerCount = nearbyCount;
    return nearestPlayerIndex;
}

void ResolvePlayerCollisions(PlayerState *player)
{
    Vec candidatePosition;
    Vec otherPlayerPosition;
    Vec separationVector;
    Vec unusedCollisionVector84;
    Vec unusedCollisionVector78;
    Vec unusedCollisionVector6c;
    s32 nearbyPlayerCount;
    f32 collisionSeparation;
    f32 directionDifference;
    f32 turnDirection;
    f32 resolvedX;
    f32 resolvedZ;
    f32 minimumSeparation;
    f32 stepDistance;
    f32 playerDirection;
    f32 opponentDirection;
    f32 candidateX;
    f32 candidateZ;
    s32 resolutionSteps;
    s32 otherPlayerIndex;
    PlayerState *candidatePlayer;
    PlayerState *nearestPlayer;
    s32 nearestPlayerIndex;

    nearestPlayerIndex = FindNearestPlayer(player, &nearbyPlayerCount);
    if (nearestPlayerIndex != -1) {
        playerDirection = WrapDegrees(atan2d((player->position.x - player->previousPosition.x), (player->position.z - player->previousPosition.y)));
        resolvedX = candidateX = player->previousPosition.x;
        resolvedZ = candidateZ = player->previousPosition.y;
        stepDistance = 1.0f;
        resolutionSteps = (1.0f + (10.0f / stepDistance));

        while (resolutionSteps != 0) {
            candidateX += stepDistance * sind(playerDirection);
            candidateZ += stepDistance * cosd(playerDirection);
            HuSetVecF(&candidatePosition, candidateX, player->position.y, candidateZ);

            for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                if (otherPlayerIndex == player->playerIndex)
                    continue;
                nearestPlayer = &playerStates[otherPlayerIndex];
                HuSetVecF(&otherPlayerPosition, nearestPlayer->position.x, nearestPlayer->position.y, nearestPlayer->position.z);
                HuSubVecF(&separationVector, &candidatePosition, &otherPlayerPosition);
                if (sqrtf((separationVector.z * separationVector.z) + ((separationVector.x * separationVector.x) + (separationVector.y * separationVector.y))) < (player->collisionParameters.x + nearestPlayer->collisionParameters.x))
                    break;
            }
            if (otherPlayerIndex < 4)
                break;
            AddX = 0.0f;
            AddZ = 0.0f;
            MapWall(player->collisionParameters.x, candidateX, player->position.y + player->collisionParameters.x, candidateZ);
            if ((AddX + AddZ) != 0.0f)
                goto end;
            resolvedX = candidateX;
            resolvedZ = candidateZ;
            resolutionSteps--;
        }
        playerDirection = WrapDegrees(atan2d((player->position.x - player->previousPosition.x), (player->position.z - player->previousPosition.y)));
        opponentDirection = WrapDegrees(atan2d((nearestPlayer->position.x - player->previousPosition.x), (nearestPlayer->position.z - player->previousPosition.y)));
        directionDifference = playerDirection - opponentDirection;
        if (directionDifference > 0.0f) {
            turnDirection = -0.5f;
        }
        else {
            turnDirection = 0.5f;
        }
        if (ABS_INV(playerDirection - opponentDirection) > 180.0f) {
            turnDirection = -turnDirection;
        }
        opponentDirection = WrapDegrees(atan2d((player->previousPosition.x - nearestPlayer->position.x), (player->previousPosition.y - nearestPlayer->position.z)));
        minimumSeparation = 0.8f * player->collisionParameters.y;
        nearestPlayerIndex = nearestPlayer->playerIndex;
        collisionSeparation = 1.0f + (player->collisionParameters.x + nearestPlayer->collisionParameters.x);
        while (1) {
            candidateX = (nearestPlayer->position.x + (collisionSeparation * sind(opponentDirection)));
            candidateZ = (nearestPlayer->position.z + (collisionSeparation * cosd(opponentDirection)));
            HuSetVecF(&candidatePosition, candidateX, 0.0, candidateZ);

            for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                if ((otherPlayerIndex == player->playerIndex) || (otherPlayerIndex == nearestPlayerIndex))
                    continue;
                candidatePlayer = &playerStates[otherPlayerIndex];
                HuSetVecF(&otherPlayerPosition, candidatePlayer->position.x, 0.0, candidatePlayer->position.z);
                HuSubVecF(&separationVector, &candidatePosition, &otherPlayerPosition);
                if (sqrtf((separationVector.x * separationVector.x) + (separationVector.z * separationVector.z)) < (player->collisionParameters.x + candidatePlayer->collisionParameters.x))
                    break;
            }
            if (otherPlayerIndex < 4)
                break;
            AddX = 0.0f;
            AddZ = 0.0f;
            MapWall(player->collisionParameters.x, candidateX, player->position.y + player->collisionParameters.x, candidateZ);
            if ((AddX + AddZ) != 0.0f)
                break;
            resolvedX = candidateX;
            resolvedZ = candidateZ;
            HuSetVecF(&candidatePosition, player->previousPosition.x, 0.0, player->previousPosition.y);
            HuSetVecF(&otherPlayerPosition, candidateX, 0.0, candidateZ);
            HuSubVecF(&separationVector, &candidatePosition, &otherPlayerPosition);
            if (sqrtf((separationVector.x * separationVector.x) + (separationVector.z * separationVector.z)) > minimumSeparation)
                break;
            opponentDirection += turnDirection;
        }
    end:
        player->position.x = resolvedX;
        player->position.z = resolvedZ;
    }
}

f32 InterpolateAngle(f32 startAngle, f32 endAngle, f32 interpolationRatio)
{
    f32 angleDifference;
    f32 interpolatedAngle;

    angleDifference = fmod((endAngle - startAngle), 360.0);
    if (0.0f > angleDifference) {
        angleDifference += 360.0f;
    }
    if (180.0f < angleDifference) {
        angleDifference -= 360.0f;
    }
    interpolatedAngle = fmod((startAngle + (angleDifference * interpolationRatio)), 360.0);
    if (0.0f > interpolatedAngle) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

f32 CalculatePlayerAngle(f32 xPosition, f32 zPosition, PlayerState *player)
{
    Vec targetPosition;
    Vec playerPosition;
    Vec separationVector;

    HuSetVecF(&targetPosition, xPosition, 0.0, zPosition);
    HuSetVecF(&playerPosition, player->position.x, 0.0, player->position.z);
    HuSubVecF(&separationVector, &targetPosition, &playerPosition);
    return sqrtf((separationVector.x * separationVector.x) + (separationVector.z * separationVector.z));
}

static inline void PreserveCompilerPadding(void)
{
    float paddingValue;
    (void)paddingValue;
    (void)paddingValue;
    (void)paddingValue;
    (void)paddingValue;
    (void)paddingValue;
    (void)paddingValue;
    (void)paddingValue;
    (void)paddingValue;
    (void)paddingValue;
    (void)paddingValue;
    (void)paddingValue;
    (void)paddingValue;
    (void)paddingValue;
    (void)paddingValue;
}

void UpdatePlayerState(PlayerState *player)
{
    Vec candidatePosition;
    Vec nearbyObjectPosition;
    Vec separationOrChoiceVector;
    volatile f32 avoidanceDistance;
    f32 escapeDistance;
    f32 nearestDistance;
    f32 debugValue; // probably debug code
    f32 steeringAngle;
    f32 candidateX;
    f32 candidateZ;
    f32 distance;
    s32 difficultyThreshold;
    s32 avoidDebris;
    s32 nearestPlayerIndex;
    DebrisPiece *nearestDebris;
    s32 index;
    PlayerState *otherPlayer;
    DebrisPiece *debris;

    player->stickX = player->stickY = 0;
    player->triggerLeft = player->triggerRight = 0;
    player->buttonState = player->buttonsPressed = 0;

    switch (GWPlayerCfg[player->playerIndex].diff) {
        case 0:
            difficultyThreshold = 0x3C;
            break;
        case 1:
            difficultyThreshold = 0x50;
            break;
        case 2:
            difficultyThreshold = 0x5A;
            break;
        default:
            difficultyThreshold = 0x5F;
            break;
    }
    switch (GWPlayerCfg[player->playerIndex].diff) {
        case 0:
            avoidanceDistance = 200.0f;
            break;
        case 1:
            avoidanceDistance = 250.0f;
            break;
        case 2:
            avoidanceDistance = 300.0f;
            break;
        default:
            avoidanceDistance = 350.0f;
            break;
    }

    for (index = 0, nearestDistance = 10000.0f; index < 0x20; index++) {
        nearestDebris = debrisSlots[index];
        if (nearestDebris && (nearestDebris->slotIndex != -1)) {
            HuSetVecF(&candidatePosition, player->position.x, 0.0, player->position.z);
            HuSetVecF(&nearbyObjectPosition, nearestDebris->position.x, 0.0, nearestDebris->position.z);
            HuSubVecF(&separationOrChoiceVector, &candidatePosition, &nearbyObjectPosition);
            distance = sqrtf((separationOrChoiceVector.z * separationOrChoiceVector.z) + ((separationOrChoiceVector.x * separationOrChoiceVector.x) + (separationOrChoiceVector.y * separationOrChoiceVector.y)));
            if ((distance < (avoidanceDistance + (player->collisionParameters.x + nearestDebris->collisionRadius))) && (nearestDistance > distance)) {
                debris = nearestDebris;
                nearestDistance = distance;
            }
        }
    }
    if (nearestDistance < 10000.0f) {
        player->playerStateFlags = 0;
        avoidDebris = 1;
        if (player->targetDebrisSlot != debris->slotIndex) {
            if (frandmod(0x64) < 0) {
                (void)avoidDebris; // probably debug code
                PreserveCompilerPadding();
            }
            else {
                (void)avoidDebris;
            }
        }
        if (avoidDebris != 0) {
            escapeDistance = 200.0f + (debris->collisionRadius + player->collisionParameters.x) + (100.0f * frandf());
            candidatePosition.x = candidateX = (debris->position.x + (escapeDistance * sind(debris->launchVelocity.x + 90.0f)));
            candidatePosition.z = candidateZ = (debris->position.z + (escapeDistance * cosd(debris->launchVelocity.x + 90.0f)));
            distance = sqrtf((candidateX * candidateX) + (candidateZ * candidateZ));
            nearbyObjectPosition.x = candidateX = (debris->position.x + (escapeDistance * sind(debris->launchVelocity.x - 90.0f)));
            nearbyObjectPosition.z = candidateZ = (debris->position.z + (escapeDistance * cosd(debris->launchVelocity.x - 90.0f)));
            avoidanceDistance = sqrtf((candidateX * candidateX) + (candidateZ * candidateZ));
            if (sqrtf((player->position.x * player->position.x) + (player->position.z * player->position.z)) < 400.0f) {
                separationOrChoiceVector.x = distance;
                distance = avoidanceDistance;
                avoidanceDistance = separationOrChoiceVector.x;
            }
            if (distance > avoidanceDistance) {
                separationOrChoiceVector.x = nearbyObjectPosition.x;
                separationOrChoiceVector.z = nearbyObjectPosition.z;
            }
            else {
                separationOrChoiceVector.x = candidatePosition.x;
                separationOrChoiceVector.z = candidatePosition.z;
            }
            if (player->targetDebrisSlot != debris->slotIndex) {
                player->targetDebrisSlot = debris->slotIndex;
                player->analogStick.x = separationOrChoiceVector.x;
                player->analogStick.y = separationOrChoiceVector.z;
            }

            steeringAngle = atan2d((player->analogStick.x - player->position.x), (player->analogStick.y - player->position.z));

            if (FindPlayerCollision(player, player->position.x + player->velocity.x, player->position.y, player->position.z + player->velocity.z) != -1) {
                steeringAngle += 90.0f;
            }
            player->stickX = 50.0 * sind(steeringAngle);
            player->stickY = -50.0f * cosd(steeringAngle);
        }
    }
    else {
        if (player->playerStateFlags == 0) {
            player->playerStateFlags = 1;
            player->fleeDistanceThreshold = (250.0f + (90.0f * frandf()));
            player->fleeFramesRemaining = 30;
        }
        if ((sqrtf((player->position.x * player->position.x) + (player->position.z * player->position.z)) > player->fleeDistanceThreshold) && (player->fleeFramesRemaining != 0)) {
            player->fleeFramesRemaining--;
            steeringAngle = (180.0 + (atan2d(player->position.x, player->position.z)));
            if (FindPlayerCollision(player, player->position.x + player->velocity.x, player->position.y, player->position.z + player->velocity.z) != -1) {
                player->fleeFramesRemaining = 0;
                steeringAngle += 90.0f;
            }
            player->stickX = 25.0 * sind(steeringAngle);
            player->stickY = -25.0 * cosd(steeringAngle);
            return;
        }
        index = 0;
        nearestDistance = 10000.0f;
        nearestPlayerIndex = -1;

        for (; index < 4; index++) {
            if (index != player->playerIndex) {
                otherPlayer = &playerStates[index];
                HuSetVecF(&candidatePosition, player->position.x, 0.0, player->position.z);
                HuSetVecF(&nearbyObjectPosition, otherPlayer->position.x, 0.0, otherPlayer->position.z);
                HuSubVecF(&separationOrChoiceVector, &candidatePosition, &nearbyObjectPosition);
                distance = sqrtf((separationOrChoiceVector.z * separationOrChoiceVector.z) + ((separationOrChoiceVector.x * separationOrChoiceVector.x) + (separationOrChoiceVector.y * separationOrChoiceVector.y)));
                if (distance < nearestDistance) {
                    nearestPlayerIndex = index;
                    nearestDistance = distance;
                }
            }
        }
        if (nearestPlayerIndex != -1) {
            otherPlayer = &playerStates[nearestPlayerIndex];
            HuSetVecF(&candidatePosition, player->position.x, 0.0, player->position.z);
            HuSetVecF(&nearbyObjectPosition, otherPlayer->position.x, 0.0, otherPlayer->position.z);
            HuSubVecF(&separationOrChoiceVector, &candidatePosition, &nearbyObjectPosition);
            distance = sqrtf((separationOrChoiceVector.x * separationOrChoiceVector.x) + (separationOrChoiceVector.z * separationOrChoiceVector.z));
            if ((distance < 200.0f) && (distance > 150.0f)) {
                steeringAngle = atan2d((player->position.x - otherPlayer->position.x), (player->position.z - otherPlayer->position.z));
                (void)distance;
                player->stickX = 30.0f * sind(steeringAngle);
                player->stickY = -30.0f * cosd(steeringAngle);
            }
        }
    }
}

void RunPlayerControlProcess(void)
{
    Vec movementDirection;
    Vec unusedPositionBuffer188;
    Vec unusedPositionBuffer17c;
    Vec unusedPositionBuffer170;
    Vec unusedPositionBuffer164;
    Vec unusedPositionBuffer158;
    Vec unusedPositionBuffer14c;
    Vec unusedPositionBuffer140;
    f32 velocityAngle;
    f32 collisionAngleWeight;
    f32 effectScaleY;
    f32 effectFadeAlpha;
    f32 turnStep;
    f32 movementAngle;
    f32 movementSpeed;
    f32 collisionAngle;
    s32 playerIndex;
    PlayerState *otherPlayerState;
    PlayerState *playerState;

    playerState = HuPrcCurrentGet()->user_data;
    playerState->roundStatus = 0;
    playerState->playerStateFlags = 0;
    playerState->targetDebrisSlot = -1;

    while (roundState == 0) {
        HuPrcVSleep();
    }

    roundState = 2;
    while (roundState == 2) {
        HuPrcVSleep();
    }
loop6:
    playerState->previousPosition.x = playerState->position.x;
    playerState->previousPosition.y = playerState->position.z;
    if (GWPlayerCfg[playerState->playerIndex].iscom == 1) {
        UpdatePlayerState(playerState);
    }
    switch (playerState->roundStatus) {
        default:
            HuSetVecF(&movementDirection, CheckPlayerCollisionAlternate(playerState->playerIndex, 0) / 4.0f, 0.0, -CheckPlayerCollisionAlternate(playerState->playerIndex, 1) / 4.0f);
            playerState->collisionParameters.z = sqrtf((movementDirection.z * movementDirection.z) + ((movementDirection.x * movementDirection.x) + (movementDirection.y * movementDirection.y)));
            if (playerState->collisionParameters.z >= 0.5f) {
                if (playerState->collisionParameters.z >= 10.0f) {
                    movementDirection.x /= playerState->collisionParameters.z;
                    movementDirection.z /= playerState->collisionParameters.z;
                    playerState->collisionParameters.z = 10.0f;
                    movementDirection.x = 10.0f * movementDirection.x;
                    movementDirection.z = 10.0f * movementDirection.z;
                }
                playerState->rotation.y = InterpolateAngle(playerState->rotation.y, atan2d(movementDirection.x, movementDirection.z), 0.4f);
                if (playerState->collisionParameters.z >= 8.0f) {
                    if (playerState->roundStatus != 2) {
                        playerState->roundStatus = 2;
                        CharMotionShiftSet(
                            characterModelIndices[GWPlayerCfg[playerState->playerIndex].character], playerState->characterMotions[2], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                        Hu3DMotionShiftSet(playerState->reflectedModelId, playerState->jointMotions[2], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                    }
                }
                else if (playerState->roundStatus != 1) {
                    playerState->roundStatus = 1;
                    CharMotionShiftSet(characterModelIndices[GWPlayerCfg[playerState->playerIndex].character], playerState->characterMotions[1], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                    Hu3DMotionShiftSet(playerState->reflectedModelId, playerState->jointMotions[1], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                }
                playerState->velocity.x += 0.1f * movementDirection.x;
                playerState->velocity.z += 0.1f * movementDirection.z;
            }
            else if (playerState->roundStatus != 0) {
                playerState->roundStatus = 0;
                CharMotionSet(characterModelIndices[GWPlayerCfg[playerState->playerIndex].character], playerState->characterMotions[0]);
                Hu3DModelAttrSet(playerState->characterModelId, HU3D_MOTATTR_LOOP);
                Hu3DMotionSet(playerState->reflectedModelId, playerState->jointMotions[0]);
                Hu3DMotionSpeedSet(playerState->reflectedModelId, 2.0f);
                Hu3DModelAttrSet(playerState->reflectedModelId, HU3D_MOTATTR_LOOP);
            }
            movementSpeed = sqrtf((playerState->velocity.x * playerState->velocity.x) + (playerState->velocity.z * playerState->velocity.z));
            if (playerState->collisionParameters.z > 0.5f) {
                if (movementSpeed > 0.25f) {
                    movementSpeed -= 0.25f;
                }
            }
            else {
                movementSpeed -= 0.25f;
            }
            if (movementSpeed < 0.0f) {
                HuSetVecF(&playerState->velocity, 0.0, 0.0, 0.0);
                movementSpeed = 0.0f;
            }
            else {
                if (movementSpeed > 10.0f) {
                    movementSpeed = 10.0f;
                }
                velocityAngle = WrapDegrees(atan2d(playerState->velocity.x, playerState->velocity.z));
                HuSetVecF(&playerState->velocity, (movementSpeed * sind(velocityAngle)), 0.0, (movementSpeed * cosd(velocityAngle)));
            }
            playerState->collisionParameters.y = movementSpeed;
            playerState->position.x += playerState->velocity.x;
            playerState->position.z += playerState->velocity.z;
            if ((playerState->collidingPlayerIndex = FindPlayerCollision(playerState, playerState->position.x, playerState->position.y, playerState->position.z)) != -1) {
                otherPlayerState = &playerStates[playerState->collidingPlayerIndex];
                if (otherPlayerState->roundStatus != 3) {
                    collisionAngle = atan2d((otherPlayerState->position.x - playerState->position.x), (otherPlayerState->position.z - playerState->position.z));
                    movementAngle = atan2d(playerState->velocity.x, playerState->velocity.z);
                    collisionAngle = ABS_INV(collisionAngle - movementAngle);
                    if (collisionAngle > 180.0f) {
                        collisionAngle = 360.0f - collisionAngle;
                    }
                    collisionAngleWeight = collisionAngle / 90.0f;
                    collisionAngle = atan2d((playerState->position.x - otherPlayerState->position.x), (playerState->position.z - otherPlayerState->position.z));
                    movementAngle = atan2d(playerState->velocity.x, playerState->velocity.z);
                    movementAngle = WrapDegrees(180.0f + movementAngle);
                    movementAngle = WrapDegrees(collisionAngle - (movementAngle - collisionAngle));
                    collisionAngle = WrapDegrees(atan2d((otherPlayerState->position.x - playerState->position.x), (otherPlayerState->position.z - playerState->position.z)));
                    HuSetVecF(&movementDirection, playerState->velocity.x, 0.0, playerState->velocity.z);
                    RotateVectorTowardAngle(&movementDirection, collisionAngle, 1.0f - collisionAngleWeight);
                    HuSetVecF(&otherPlayerState->velocity, movementDirection.x, 0.0, movementDirection.z);
                    HuSetVecF(&movementDirection, playerState->velocity.x, 0.0, playerState->velocity.z);
                    RotateVectorTowardAngle(&movementDirection, movementAngle, collisionAngleWeight);
                    HuSetVecF(&playerState->velocity, movementDirection.x, 0.0, movementDirection.z);
                }
                ResolvePlayerCollisions(playerState);
            }
            while (1) {
                AddX = 0.0f;
                AddZ = 0.0f;
                MapWall(playerState->collisionParameters.x, playerState->position.x, playerState->position.y + playerState->collisionParameters.x, playerState->position.z);
                playerState->position.x += AddX;
                playerState->position.z += AddZ;
                if ((playerState->collidingPlayerIndex = FindPlayerCollision(playerState, playerState->position.x, playerState->position.y, playerState->position.z)) == -1)
                    break;
                playerState->position.x = playerState->previousPosition.x;
                playerState->position.z = playerState->previousPosition.y;
            }
            Hu3DModelPosSet(playerState->characterModelId, playerState->position.x, playerState->position.y, playerState->position.z);
            Hu3DModelRotSet(playerState->characterModelId, playerState->rotation.x, playerState->rotation.y, playerState->rotation.z);
            Hu3DModelPosSet(playerState->reflectedModelId, playerState->position.x, playerState->position.y, playerState->position.z);
            Hu3DModelRotSet(playerState->reflectedModelId, playerState->rotation.x, playerState->rotation.y, playerState->rotation.z);
            HuPrcVSleep();
            goto loop6;
        case 3:
            HuAudFXPlay(0x584);
            Hu3DModelAttrSet(playerState->characterModelId, HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrSet(playerState->reflectedModelId, HU3D_MOTATTR_PAUSE);
            Hu3DModelPosSet(playerState->extraModelId, playerState->position.x + (100.0 * sind(cameraYaw)), 200.0f + playerState->position.y,
                playerState->position.z + (100.0 * cosd(cameraYaw)));
            Hu3DModelAttrReset(playerState->extraModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(playerState->extraModelId, HU3D_MOTATTR_PAUSE);
            HuPrcSleep(10);
            Hu3DModelAttrSet(playerState->characterModelId, HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrSet(playerState->reflectedModelId, HU3D_MOTATTR_PAUSE);
            HuPrcSleep(10);
            Hu3DModelPosSet(playerState->effectModelId, playerState->position.x, playerState->position.y, playerState->position.z);
            Hu3DModelPosSet(playerState->auxiliaryModelId, playerState->position.x, playerState->position.y, playerState->position.z);
            Hu3DModelAttrReset(playerState->effectModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelAttrReset(playerState->auxiliaryModelId, HU3D_ATTR_DISPOFF);
            Hu3DModelShadowDispOn(playerState->effectModelId);

            for (effectScaleY = 0.0f, effectFadeAlpha = 1.0f; effectScaleY < 1; effectScaleY += 0.05f, effectFadeAlpha -= 0.1f) {
                Hu3DModelScaleSet(playerState->effectModelId, 1.0f, effectScaleY, 1.0f);
                Hu3DModelScaleSet(playerState->auxiliaryModelId, 1.0f, -effectScaleY, 1.0f);
                if (effectFadeAlpha > 0.0f) {
                    Hu3DModelAmbSet(playerState->characterModelId, effectFadeAlpha, effectFadeAlpha, 1.0f);
                }
                HuPrcVSleep();
            }
            Hu3DModelScaleSet(playerState->effectModelId, 1.0f, 1.0f, 1.0f);
            Hu3DModelScaleSet(playerState->auxiliaryModelId, 1.0f, -1.0f, 1.0f);
            while (1) {
                HuPrcVSleep();
            }
        case 4:
            CharMotionShiftSet(characterModelIndices[GWPlayerCfg[playerState->playerIndex].character], playerState->characterMotions[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
            Hu3DMotionShiftSet(playerState->reflectedModelId, playerState->jointMotions[0], 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
            playerIndex = 30;
            turnStep = CalculateAngularStep(playerIndex, cameraYaw, playerState->rotation.y);

            for (; playerIndex != 0; playerIndex--) {
                playerState->rotation.y += turnStep;
                Hu3DModelRotSet(playerState->characterModelId, playerState->rotation.x, playerState->rotation.y, playerState->rotation.z);
                Hu3DModelRotSet(playerState->reflectedModelId, playerState->rotation.x, playerState->rotation.y, playerState->rotation.z);
                HuPrcVSleep();
            }
            while (roundState < 5) {
                HuPrcVSleep();
            }
            CharMotionShiftSet(characterModelIndices[GWPlayerCfg[playerState->playerIndex].character], playerState->characterMotions[5], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
            Hu3DMotionShiftSet(playerState->reflectedModelId, playerState->jointMotions[5], 0.0f, 5.0f, HU3D_MOTATTR_NONE);

            while (1) {
                HuPrcVSleep();
            }
    }
}

void UpdateDebrisParticles(HU3DMODEL *model, HU3DPARTICLE *particle, f32 (*matrix)[4])
{
    HU3DPARTICLEDATA *particleData;
    f32 particleRadius;
    f32 particleSpeed;
    f32 latitudeAngle;
    f32 longitudeAngle;
    s16 latitudeStep;
    s16 longitudeStep;
    s16 debrisSlotIndex;
    s16 particleIndex;
    s32 particleAlpha;
    DebrisPiece *debris;

    if (particle->count == 0) {
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->accel.x = particleData->scale = 0.0f;
        }
    }
    particleData = particle->data;

    for (debrisSlotIndex = 0; debrisSlotIndex < 0x20; debrisSlotIndex++) {
        debris = debrisSlots[debrisSlotIndex];
        if (debris && (debris->particleSpawnCount != 0)) {
            for (latitudeStep = 0, latitudeAngle = 0.0f; latitudeStep < 4; latitudeStep++, latitudeAngle += 45.0f) {
                for (longitudeStep = 0, longitudeAngle = 0.0f; longitudeStep < 8; longitudeStep++, longitudeAngle += 45.0f) {
                    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                        if (particleData->accel.x == 0.0f)
                            break;
                    }
                    if (particleIndex == particle->maxCnt)
                        break;
                    if (debris->debrisType == 1) {
                        particleRadius = 2.0f + (2.0f * frandf());
                        particleSpeed = 10.0f;
                    }
                    else {
                        particleRadius = 1.0f + frandf();
                        particleSpeed = 3.0f;
                    }
                    particleData->pos.x = debris->position.x + (particleRadius * (sind(latitudeAngle) * cosd(longitudeAngle)));
                    particleData->pos.y = debris->position.y + (particleRadius * sind(longitudeAngle));
                    particleData->pos.z = (debris->position.z + (particleRadius * (cosd(latitudeAngle) * cosd(longitudeAngle))));
                    particleData->vel.x = (particleSpeed * (sind(latitudeAngle) * cosd(longitudeAngle)));
                    particleData->vel.y = (particleSpeed * sind(longitudeAngle));
                    particleData->vel.z = (particleSpeed * (cosd(latitudeAngle) * cosd(longitudeAngle)));
                    particleData->accel.x = 30.0f;
                    if (debris->debrisType == 1) {
                        particleData->scale = 20.0f + (2.0f * frandf());
                        particleData->accel.y = 0.0f;
                    }
                    else {
                        particleData->scale = 15.0f;
                        particleData->accel.y = 0.0f;
                    }
                    particleData->color.a = 0xFF;
                    particleData->accel.z = 20.0f;
                }
            }
            debris->particleSpawnCount++;
        }
    }
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->accel.x != 0.0f) {
            VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
            particleData->vel.x *= 0.95f;
            particleData->vel.z *= 0.95f;
            particleData->vel.y -= 0.2f;
            particleData->scale += particleData->accel.y;
            particleAlpha = particleData->color.a;
            particleAlpha -= particleData->accel.z;
            if (particleData->accel.z > 3.0f) {
                particleData->accel.z -= 1.0f;
            }
            if (particleAlpha < 0) {
                particleAlpha = 0;
                particleData->accel.x = 0.0f;
            }
            particleData->color.a = particleAlpha;
            if (particleData->accel.x == 0.0f) {
                particleData->scale = 0.0f;
            }
        }
    }
}

void UpdateSparkParticles(HU3DMODEL *model, HU3DPARTICLE *particle, f32 (*matrix)[4])
{
    HU3DPARTICLEDATA *particleData;
    f32 particleAngle;
    f32 particleSpeed;
    f32 tangentialSpeed;
    f32 angleStep;
    s16 debrisSlotIndex;
    s16 sparkIndex;
    s16 particleIndex;
    s32 particleAlpha;
    DebrisPiece *debris;

    if (particle->count == 0) {
        particleData = particle->data;

        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->accel.x = particleData->scale = 0.0f;
        }
    }
    particleData = particle->data;

    for (debrisSlotIndex = 0; debrisSlotIndex < 0x20; debrisSlotIndex++) {
        debris = debrisSlots[debrisSlotIndex];
        if (debris && (debris->particleSpawnCount != 0)) {

            for (sparkIndex = 0, angleStep = 0.0f; sparkIndex < 9; sparkIndex++, angleStep += 45.0f) {
                for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                    if (particleData->accel.x == 0.0f)
                        break;
                }
                if (particleIndex == particle->maxCnt)
                    break;
                particleAngle = angleStep;
                if (sparkIndex < 8) {
                    particleSpeed = 1.0f + frandf();
                    tangentialSpeed = 2.0f;
                }
                else {
                    particleSpeed = tangentialSpeed = 0.0f;
                }
                particleData->pos.x = debris->position.x + (particleSpeed * sind(particleAngle));
                particleData->pos.y = debris->position.y + (10.0f * frandf());
                particleData->pos.z = debris->position.z + (particleSpeed * cosd(particleAngle));
                particleData->vel.x = tangentialSpeed * sind(particleAngle);
                particleData->vel.y = -1.0f + frandf();
                particleData->vel.z = tangentialSpeed * cosd(particleAngle);
                particleData->accel.x = 30.0f;
                particleData->scale = 15.0f;
                particleData->accel.y = 1.5f;
                particleData->color.a = 0xC8;
                particleData->accel.z = 17.0f;
            }
            debris->particleSpawnCount = 0;
        }
    }
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (0.0f != particleData->accel.x) {
            VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
            particleData->vel.x *= 0.95f;
            particleData->vel.z *= 0.95f;
            particleData->vel.y -= 0.2f;
            particleData->scale += particleData->accel.y;
            particleAlpha = particleData->color.a;
            particleAlpha -= particleData->accel.z;
            if (particleData->accel.z > 3.0f) {
                particleData->accel.z -= 1.0f;
            }
            if (particleAlpha < 0) {
                particleAlpha = 0;
                particleData->accel.x = 0.0f;
            }
            particleData->color.a = particleAlpha;
            if (0.0f == particleData->accel.x) {
                particleData->scale = 0.0f;
            }
        }
    }
}

void UpdateShardParticles(HU3DMODEL *model, HU3DPARTICLE *particle, f32 (*matrix)[4])
{
    HU3DPARTICLEDATA *particleData;
    f32 particleAlpha;
    s16 modelSetIndex;
    s16 particleIndex;
    SceneModelSet *modelSet;

    if (particle->count == 0) {
        particleData = particle->data;

        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->accel.x = particleData->scale = 0.0f;
        }
    }
    particleData = particle->data;

    for (modelSetIndex = 0; modelSetIndex < 6; modelSetIndex++) {
        modelSet = &sceneModelSets[modelSetIndex];
        if (modelSet->activeState == 0)
            continue;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            if (particleData->accel.x == 0.0f)
                break;
        }
        if (particleIndex == particle->maxCnt)
            break;
        particleData->pos.x = modelSet->position.x + (-100.0 * cosd(modelSet->rotation.y));
        particleData->pos.y = 98.0f + modelSet->position.y;
        particleData->pos.z = modelSet->position.z + (100.0 * sind(modelSet->rotation.y));
        particleData->accel.x = 30.0f;
        particleData->scale = 80.0f;
        particleData->accel.y = 5.0f;
        particleData->color.a = 0xC8;
        particleData->accel.z = 17.0f;
        modelSet->activeState = 0;
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
            if (particleData->accel.x == 0.0f) {
                particleData->scale = 0.0f;
            }
        }
    }
}

void RunRoundSequence(void)
{
    Vec emitterPosition;
    Vec emitterStep;
    f32 cameraMotionTime;
    void *emitterHandle;
    HUPROCESS *emitterProcess;
    s32 frame;
    PlayerState *playerState;

    PlayPositionedSoundEffect(1465.0f, 770.0f, -2400.0f, 0x588);
    HuAudFXPlay(0x58B);

    frame = 0;
    cameraMotionTime = Hu3DMotionMaxTimeGet(sceneCameraId);
    for (; cameraMotionTime; frame++, cameraMotionTime--) {
        if (frame == 0x33) {
            Hu3DModelAttrReset(roundModelIds[0], HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrReset(roundModelIds[0], HU3D_ATTR_DISPOFF);
            Hu3DModelPosSet(roundModelIds[0], 1465.0f, 770.0f, -2400.0f);
        }
        if (frame == 0x47) {
            Hu3DModelAttrReset(roundModelIds[1], HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrReset(roundModelIds[1], HU3D_ATTR_DISPOFF);
            Hu3DModelPosSet(roundModelIds[1], 1531.0f, 773.0f, -2380.0f);
        }
        if (frame == 0x82) {
            emitterHandle = PlayPositionedSoundEffect(1465.0f, 770.0f, -2400.0f, 0x580);
            emitterProcess = HuPrcChildCreate(MoveSoundEmitterProcess, 0x2000, 0x1000, 0, HuPrcCurrentGet());
            emitterProcess->user_data = emitterHandle;
        }
        if (cameraMotionTime == 90.0f) {
            for (frame = 0; frame < 4; frame++) {
                playerState = &playerStates[frame];
                CharMotionShiftSet(characterModelIndices[GWPlayerCfg[frame].character], playerState->characterMotions[3], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
                Hu3DMotionShiftSet(playerState->reflectedModelId, playerState->jointMotions[3], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
            }
        }
        HuPrcVSleep();
    }
    HuPrcSleep(30);

    for (frame = 0; frame < 4; frame++) {
        playerState = &playerStates[frame];
        PlayerFXPlay(playerState->playerIndex, 0x128);
        CharMotionShiftSet(characterModelIndices[GWPlayerCfg[frame].character], playerState->characterMotions[4], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
        Hu3DMotionShiftSet(playerState->reflectedModelId, playerState->jointMotions[4], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    }
}

void NoopObjectCallback(void)
{
    HuPrcSleep(0x1E);
}

void WaitForMinigameExit(void)
{
    while (1) {
        while (1) {
            if (omSysExitReq != 1)
                break;
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
            HuAudFadeOut(0x3E8);
            HuPrcSleep(0x3C);
            MGSeqKillAll();
            omOvlReturnEx(1, 1);
            while (1) {
                HuPrcVSleep();
            }
        }
        HuPrcVSleep();
    }
}

void RunMinigameController(void)
{
    s16 countdownSequence;
    s16 messageSequence;
    s16 countdownFrames;
    s32 musicSequence;
    s32 activePlayerCount;
    s32 playerIndex;

    roundState = 0;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerCharacterResults[playerIndex] = -1;
    }
    HuAudFXPlay(0x57F);
    PlayPositionedSoundEffect(1465.0f, 770.0f, -2400.0f, 0x589);
    HuPrcSleep(1);
    Hu3DModelAttrSet(bonusModelSet.primaryModelId, HU3D_MOTATTR_PAUSE);
    Hu3DModelClusterAttrSet(bonusModelSet.primaryModelId, 0, -0x3FFFFFFE);
    Hu3DModelAttrSet(bonusModelSet.reflectedModelId, HU3D_MOTATTR_PAUSE);
    Hu3DModelClusterAttrSet(bonusModelSet.reflectedModelId, 0, -0x3FFFFFFE);
    HuPrcSleep(1.0f + wipeData.duration);
    RunRoundSequence();
    HuPrcSleep(30);
    roundState = 1;
    while (roundState == 1) {
        HuPrcVSleep();
    }
    musicSequence = HuAudSeqPlay(0x43);
    messageSequence = MGSeqCreate(3, 0);

    while (MGSeqStatGet(messageSequence) != 0) {
        HuPrcVSleep();
    }
    countdownFrames = REFRESH_RATE * 60 + REFRESH_RATE - 1;
    countdownSequence = MGSeqCreate(1, countdownFrames / REFRESH_RATE, -1, -1);
    roundState = 3;

    while (1) {
        for (playerIndex = 0, activePlayerCount = 0; playerIndex < 4; playerIndex++) {
            if (playerStates[playerIndex].roundStatus != 3) {
                activePlayerCount++;
            }
        }
        if ((activePlayerCount <= 1) || (countdownFrames < REFRESH_RATE - 1))
            break;
        MGSeqParamSet(countdownSequence, 1, countdownFrames-- / REFRESH_RATE);
        roundTimer++;
        HuPrcVSleep();
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerStates[playerIndex].roundStatus != 3) {
            playerCharacterResults[playerIndex] = GWPlayerCfg[playerIndex].character;
            if (_CheckFlag(0x1000C) == 0) {
                GWPlayer[playerIndex].coinWin = 10;
            }
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerStates[playerIndex].roundStatus = 4;
    }
    roundState = 4;
    HuAudSeqFadeOut(musicSequence, 0x64);
    MGSeqParamSet(countdownSequence, 2, -1);
    messageSequence = MGSeqCreate(3, 1);

    while (MGSeqStatGet(messageSequence) != 0) {
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    roundState = 5;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerCharacterResults[playerIndex] != -1)
            break;
    }
    if (playerIndex < 4) {
        HuAudSStreamPlay(1);
        messageSequence = MGSeqCreate(5, 3, playerCharacterResults[0], playerCharacterResults[1], playerCharacterResults[2], playerCharacterResults[3]);
    }
    else {
        HuAudSStreamPlay(4);
        messageSequence = MGSeqCreate(3, 2);
    }
    HuPrcSleep(0xD2);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    HuPrcSleep(0x3C);
    HuAudFadeOut(1);
    MGSeqKillAll();
    omOvlReturnEx(1, 1);
    while (1) {
        HuPrcVSleep();
    }
}

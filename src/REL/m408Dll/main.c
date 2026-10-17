#include <version.h>

#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/hu3d.h"
#include "game/gamework_data.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/wipe.h"
#include "game/sprite.h"

#include "REL/m408Dll.h"

#include "ext_math.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/frand.h"
#include "game/esprite.h"
#endif

s32 characterAssetIds[] = {
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x00),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x01),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x02),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x03),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x04),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x05),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x06),
    DATA_MAKE_NUM(DATADIR_MGCONST, 0x07),
};

Vec globalLightPosition = { 150, 1250, 150 };
Vec globalLightDirection = { -0.1, -0.8, 0.1 };
GXColor globalLightColor = { 255, 255, 255, 255 };
Vec shadowLightVectors[] = { { 0, -2000, 0 }, { 0, 1, 0 } };
GXColor shadowLightColor = { 255, 255, 255, 255 };
Vec shadowCameraPosition = { 50, 15000, 50 };
Vec shadowCameraUp = { 0, 1, 0 };
Vec shadowCameraTarget = { 0, 0, 0 };

OMOBJ *mainPlayerObject;
OMOBJ *stageProgressObject;
OMOBJ *viewExitObject;
OMOBJ **characterObjects;
OMOBJ *characterMotionObject;
s16 minigameEntryDelay;
s16 minigameRefreshDuration;
u16 minigameState;
u16 stageProgressState;
s32 stageProgressTimer;
s32 characterObjectCount;
float stageVerticalPosition;
float stageVerticalSpeed;
u32 activeCharacterCountdown;
s32 stageFinishStarted;
s32 minigameExitAllowed;
s32 minigameExitRequested;
u32 maxCharacterAnimationSpeed;
s32 activeCharacterIds[5];
s32 startMessageSequence;
s32 finishMessageSequence;
s32 stageMusicSequence;
s32 stageSoundStreamId;
s32 stageEffectStreamId;
s32 cameraSoundEmitterId;
s32 stageEffectEnabled;
OMOBJ *characterMotionManager;
float cameraFadeValue;
s32 cameraFallStarted;

void InitializeMinigameState(OMOBJ *object);
void InitializeStageProgress(OMOBJ *object);

void CreateCharacterObjects(HUPROCESS *objman);

void ObjectSetup(void)
{
    s32 animationIndex;
    HUPROCESS *objectManager;
    s16 integerValue;
    HU3DLIGHT *light;
    Vec positionVector = { 0, 0, 0 };
    Vec positionVector2 = { 0, 0, -1 };
    HuAudSndGrpSet(33);
#if VERSION_PAL
    HuAudFXListnerSetEX(&positionVector, &positionVector2, 6000, 3668.0f, 0, 300, 300);
#else
    HuAudFXListnerSetEX(&positionVector, &positionVector2, 6000, 3656.6667f, 0, 300, 300);
#endif
    stageMusicSequence = stageSoundStreamId = stageEffectStreamId = cameraSoundEmitterId = -1;
    stageEffectEnabled = 0;
    minigameExitAllowed = 1;
    minigameExitRequested = 0;
    integerValue = Hu3DGLightCreateV(&globalLightPosition, &globalLightDirection, &globalLightColor);
    Hu3DGLightInfinitytSet(integerValue);
    light = &Hu3DGlobalLight[integerValue];
    light->type |= 0x8000;
    Hu3DShadowCreate(30, 100, 25000);
    Hu3DShadowTPLvlSet(0.3f);
    Hu3DShadowPosSet(&shadowCameraPosition, &shadowCameraUp, &shadowCameraTarget);
    objectManager = omInitObjMan(50, 8192);
    omGameSysInit(objectManager);
    for (animationIndex = 0; animationIndex < 4; animationIndex++) {
        CRotM[animationIndex].x = 324.3f;
        CRotM[animationIndex].y = 0;
        CRotM[animationIndex].z = 0;
        CenterM[animationIndex].x = 0;
        CenterM[animationIndex].y = 130;
        CenterM[animationIndex].z = 224;
        CZoomM[animationIndex] = 220;
    }
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, -1, 5, 25000, (4.0f / 3.0f));
    Hu3DCameraViewportSet(1, 0, 0, 640, 480, 0, 1);
    viewExitObject = omAddObjEx(objectManager, 32730, 0, 0, -1, omOutView);
    minigameState = 0;
    stageProgressState = 0;
    stageFinishStarted = 0;
    mainPlayerObject = omAddObjEx(objectManager, 10, 0, 0, -1, InitializeMinigameState);
    maxCharacterAnimationSpeed = 0;
    for (animationIndex = 0; animationIndex < 5; animationIndex++) {
        activeCharacterIds[animationIndex] = -1;
    }
    stageProgressObject = omAddObjEx(objectManager, 50, 0, 0, -1, InitializeStageProgress);
    CreateCharacterObjects(objectManager);
    CreateStageEffects(objectManager);
    CreateEffectObjects(objectManager);
    CreateCameraObjects(objectManager);
}

void UpdateMinigameState(OMOBJ *object);
void FinishMinigameExit(OMOBJ *object);

void InitializeMinigameState(OMOBJ *object)
{
    minigameEntryDelay = 30;
    minigameRefreshDuration = REFRESH_RATE;
    activeCharacterCountdown = 4;
    minigameState = 0;
    object->objFunc = UpdateMinigameState;
}

void UpdateMinigameState(OMOBJ *object)
{
    s32 integerValue = 0;
    Vec scratchValues;
    Vec scratchValues2;
    switch (minigameState) {
        case 0:
            WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
            minigameExitAllowed = 0;
            Hu3DCameraPerspectiveSet(1, 50, 5, 25000, (4.0f / 3.0f));
            minigameState = 1;
            break;

        case 1:
            if (GetCameraSequenceState() == 5) {
                minigameState = 2;
            }
            break;

        case 2:
            if (GetStageEventState() == 5) {
                object->work[0] = 0;
                minigameState = 3;
            }
            break;

        case 3:
            if (activeCharacterCountdown == 0 || WipeStatGet() == 0) {
#if VERSION_PAL
                if (++object->work[0] >= 175.0f) {
#else
                if (++object->work[0] >= 210.0f) {
#endif
                    minigameState = 4;
                }
            }
            break;

        case 4:
            minigameState = 5;
            break;

        case 5:
            if (GetCameraSequenceState() == 5) {
                integerValue = 1;
                minigameState = 5;
            }
            break;
    }
    scratchValues2.x = -(sind(CRot.y) * cosd(CRot.x));
    scratchValues2.y = -(-sind(CRot.x));
    scratchValues2.z = -(cosd(CRot.y) * cosd(CRot.x));
    scratchValues.x = Center.x - (scratchValues2.x * CZoom);
    scratchValues.y = Center.y - (scratchValues2.y * CZoom);
    scratchValues.z = Center.z - (scratchValues2.z * CZoom);
    HuAudFXListnerUpdate(&scratchValues, &scratchValues2);
    if ((omSysExitReq && !WipeStatGet()) && !minigameExitAllowed || integerValue) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        minigameExitAllowed = 1;
        minigameExitRequested = 1;
        if (cameraSoundEmitterId >= 0) {
            HuAudFXFadeOut(cameraSoundEmitterId, REFRESH_RATE / 2);
        }
        object->objFunc = FinishMinigameExit;
    }
}

void FinishMinigameExit(OMOBJ *object)
{
    if (!WipeStatGet()) {
        CharModelKill(-1);
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void UpdateStageProgress(OMOBJ *object);

void InitializeStageProgress(OMOBJ *object)
{
    stageVerticalPosition = 12000;
    stageVerticalSpeed = 0;
    stageProgressState = 0;
    object->objFunc = UpdateStageProgress;
}

void InitializeStageAnimation(OMOBJ *object);

u32 GetStageProgress(void);

void UpdateStageProgress(OMOBJ *object)
{
    if (GetStageProgress() != 2) {
        return;
    }
    switch (stageProgressState) {
        case 0:
            Hu3DCameraPerspectiveSet(1, 35, 5, 25000, (4.0f / 3.0f));
            CRot.x = -80.5f;
            CRot.y = CRot.z = 0;
            Center.x = 0;
            Center.y = stageVerticalPosition;
            Center.z = 0;
            CZoom = 260;
            stageProgressState = 1;
            break;

        case 1:
            startMessageSequence = MGSeqStartCreate();
            MGSeqPosSet(startMessageSequence, 320, 240);
            stageProgressState = 2;

        case 2:
            Center.y = stageVerticalPosition;
            stageVerticalSpeed = -400.0f;
            ;
            stageVerticalPosition += stageVerticalSpeed * REFRESH_FREQ;
            if (stageMusicSequence < 0 && (MGSeqStatGet(startMessageSequence) & 0x10)) {
                stageMusicSequence = HuAudSeqPlay(74);
            }
            if (MGSeqStatGet(startMessageSequence) == 0) {
                stageProgressState = 3;
                object->objFunc = InitializeStageAnimation;
            }

            break;
    }
}

void UpdateStageObjects(void);

void InitializeStageObject(OMOBJ *object);

void UpdateStageObject(OMOBJ *object);

void InitializeStageAnimation(OMOBJ *object)
{
    Center.y = stageVerticalPosition;
    stageVerticalSpeed = -400.0f;
    stageVerticalPosition += stageVerticalSpeed * REFRESH_FREQ;
    UpdateStageObjects();
    if (activeCharacterCountdown == 0) {
        stageFinishStarted = 1;
        finishMessageSequence = MGSeqFinishCreate();
        MGSeqPosSet(finishMessageSequence, 320, 240);
        HuAudSeqFadeOut(stageMusicSequence, 100);
        object->objFunc = UpdateStageObject;
    }
    else {
        if (stageVerticalPosition <= (-stageVerticalSpeed * 0.5f)) {
            finishMessageSequence = MGSeqFinishCreate();
            MGSeqPosSet(finishMessageSequence, 320, 240);
            HuAudSeqFadeOut(stageMusicSequence, 100);
            stageVerticalSpeed = -120.00001f;
            stageVerticalPosition = -stageVerticalSpeed * 2.0f;
            stageProgressState = 4;
            object->objFunc = InitializeStageObject;
        }
    }
}

void UpdateStageObjectAnimation(OMOBJ *object);

void InitializeStageObject(OMOBJ *object)
{
    Vec positionVector;
    Vec positionVector2;
    Vec positionVector3;
    Vec positionVector4;
    float angle;
    if (cameraSoundEmitterId < 0) {
        cameraSoundEmitterId = HuAudFXPlay(1360);
    }
    positionVector.x = 0;
    positionVector.y = 50;
    positionVector.z = 230;
    angle = atan2d(stageVerticalPosition - positionVector.y, positionVector.z);
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    Center.x = 0;
    Center.y = stageVerticalPosition;
    Center.z = 0;
    CRot.x = angle;
    CZoom = sqrtf((positionVector.z * positionVector.z) + ((stageVerticalPosition - positionVector.y) * (stageVerticalPosition - positionVector.y)));
    positionVector4.x = 0;
    positionVector4.z = 0;
    positionVector2.x = 200;
    positionVector2.z = 200;
    positionVector2.y = 800;
    positionVector4.y = 20;
    positionVector3.x = positionVector3.y = 0;
    positionVector3.z = 1;
    Hu3DShadowPosSet(&positionVector2, &positionVector3, &positionVector4);
    Hu3DShadowTPLvlSet(0.5f);
    stageVerticalPosition += stageVerticalSpeed * REFRESH_FREQ;
    if (stageVerticalPosition <= 0.0f) {
        stageVerticalPosition = 0;
        stageProgressState = 5;
        object->objFunc = UpdateStageObjectAnimation;
    }
}

void UpdateStageObjectEffect(OMOBJ *object);

void UpdateStageObject(OMOBJ *object)
{
    if (stageVerticalPosition > (-stageVerticalSpeed * 0.8f)) {
        Center.y = stageVerticalPosition;
        stageVerticalSpeed = -400;
        stageVerticalPosition += stageVerticalSpeed * REFRESH_FREQ;
    }
    if (MGSeqStatGet(finishMessageSequence)) {
        return;
    }
    switch (GetStageEffectState()) {
        case 0:
            OSReport("Map 0\n");
        case 1:
            OSReport("Map 1\n");
            if (minigameExitRequested == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, REFRESH_RATE);
                minigameExitAllowed = 1;
            }
            object->objFunc = UpdateStageObjectEffect;
            break;

        case 2:
            OSReport("Map 2\n");
        case 3:
            OSReport("Map 3\n");
            break;

        case 4:
            OSReport("Map 4\n");
            if (minigameExitRequested == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, REFRESH_RATE / 2);
                minigameExitAllowed = 1;
            }
            object->objFunc = UpdateStageObjectEffect;
            break;
    }
}

void UpdateStageObjectEffect(OMOBJ *object)
{
    Vec positionVector;
    Vec positionVector2;
    Vec positionVector3;
    Vec positionVector4;
    float angle;
    if (!WipeStatGet()) {
        stageVerticalPosition = 0;
        positionVector.x = 0;
        positionVector.y = 50;
        positionVector.z = 230;
        angle = atan2d(stageVerticalPosition - positionVector.y, positionVector.z);
        if (angle < 0.0f) {
            angle += 360.0f;
        }
        Center.x = 0;
        Center.y = stageVerticalPosition;
        Center.z = 0;
        CRot.x = angle;
        CZoom = sqrtf((positionVector.z * positionVector.z) + ((stageVerticalPosition - positionVector.y) * (stageVerticalPosition - positionVector.y)));
        positionVector4.x = 0;
        positionVector4.z = 0;
        positionVector2.x = 200;
        positionVector2.z = 200;
        positionVector2.y = 800;
        positionVector4.y = 20;
        positionVector3.x = positionVector3.y = 0;
        positionVector3.z = 1;
        Hu3DShadowPosSet(&positionVector2, &positionVector3, &positionVector4);
        Hu3DShadowTPLvlSet(0.5f);
        CleanupStageEffects();
        if (cameraSoundEmitterId < 0) {
            cameraSoundEmitterId = HuAudFXPlay(1360);
        }
        if (minigameExitRequested == 0) {
            WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, REFRESH_RATE / 2);
            minigameExitAllowed = 0;
        }
        stageProgressState = 5;
        object->objFunc = UpdateStageObjectAnimation;
    }
    else {
        if (stageVerticalPosition > (-stageVerticalSpeed * 0.8f)) {
            Center.y = stageVerticalPosition;
            stageVerticalSpeed = -400;
            stageVerticalPosition += stageVerticalSpeed * REFRESH_FREQ;
        }
    }
}

void UpdateStageObjectAnimation(OMOBJ *object)
{
    if (!WipeStatGet() && stageEffectStreamId < 0) {
        if (stageEffectEnabled) {
            stageEffectStreamId = HuAudSStreamPlay(1);
        }
        else {
            stageEffectStreamId = HuAudSStreamPlay(4);
        }
    }
}

float characterVerticalOffsets[] = { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 };

void CalculateCharacterModelOffset(CharacterEffectWork *characterWork, Vec *modelOffset)
{
    Mtx transformMatrix;
    Vec positionVector;
    positionVector.x = sind(characterWork->rotation.x + 90.0f);
    positionVector.y = 0;
    positionVector.z = cosd(characterWork->rotation.x + 90.0f);
    MTXRotAxisDeg(transformMatrix, &positionVector, characterWork->rotation.y);
    positionVector.x = positionVector.z = 0;
    positionVector.y = -characterVerticalOffsets[characterWork->characterId];
    MTXMultVec(transformMatrix, &positionVector, modelOffset);
}

void RotateVectorAndTranslate(Vec *translation, Vec *rotation, Vec *inputVector, Vec *outputVector)
{
    Mtx transformMatrix;
    mtxRot(transformMatrix, rotation->x, rotation->y, rotation->z);
    MTXMultVec(transformMatrix, inputVector, outputVector);
    outputVector->x += translation->x;
    outputVector->y += translation->y;
    outputVector->z += translation->z;
}

void CalculateCharacterVelocity(CharacterEffectWork *characterWork, s32 steeringMode, Vec *velocity)
{
    float angle;
    float angle2;
    angle2 = angle = 0;
    velocity->x = (angle * sind(angle2)) + characterWork->velocity.x;
    velocity->y = 15.0f + characterWork->velocity.y + angle;
    velocity->z = (angle * cosd(angle2)) + characterWork->velocity.z;
}

void UpdateCharacterParticleModel(OMOBJ *object, float blendFactor)
{
    Vec positionVector;
    CharacterEffectWork *charactereffectworkPointer = object->data;
    float scale = 1.0f - blendFactor;
    float scale2;
    if (!charactereffectworkPointer->animationSpeed) {
        return;
    }
    CalculateCharacterVelocity(charactereffectworkPointer, 1, &positionVector);
    if (GetStageEventState() >= 4) {
        positionVector.z += -5.0f;
    }
    charactereffectworkPointer->particlePosition.x = (charactereffectworkPointer->particlePosition.x * scale) + (positionVector.x * blendFactor);
    charactereffectworkPointer->particlePosition.y = (charactereffectworkPointer->particlePosition.y * scale) + (positionVector.y * blendFactor);
    charactereffectworkPointer->particlePosition.z = (charactereffectworkPointer->particlePosition.z * scale) + (positionVector.z * blendFactor);
    scale2 = (0.1f * ((charactereffectworkPointer->animationSpeed * 0.5f) + 0.5f)) * blendFactor;
    charactereffectworkPointer->particleScaleVector.x = charactereffectworkPointer->motionScale * (scale2 + (charactereffectworkPointer->particleScaleVector.x * scale));
    charactereffectworkPointer->particleScaleVector.y = charactereffectworkPointer->motionScale * (scale2 + (charactereffectworkPointer->particleScaleVector.y * scale));
    charactereffectworkPointer->particleScaleVector.z = charactereffectworkPointer->motionScale * (scale2 + (charactereffectworkPointer->particleScaleVector.z * scale));
    Hu3DModelPosSetV(object->mdlId[1], &charactereffectworkPointer->particlePosition);
    Hu3DModelScaleSetV(object->mdlId[1], &charactereffectworkPointer->particleScaleVector);
}

void InitializeCharacterEffect(OMOBJ *object)
{
    CharacterEffectWork *work = object->data;
    if (work->animationSpeed) {
#if VERSION_PAL
        work->particlePosition.y += 0.59999999f; // supposedly 0.6f
#else
        work->particlePosition.y += 0.5f;
#endif
        Hu3DModelPosSetV(object->mdlId[1], &work->particlePosition);
    }
}

s32 GetCharacterEffectState(OMOBJ *object)
{
    float interpolationValue;
    float interpolationValue2;
    float interpolationValue3;
    Vec positionVector;
    Vec positionVector2;
    Vec positionVector3;

    CharacterEffectWork *charactereffectworkPointer = object->data;
    interpolationValue = (charactereffectworkPointer->velocity.x * charactereffectworkPointer->velocity.x) + ((10.0f / 7.0f) * ((10.0f / 7.0f) * charactereffectworkPointer->velocity.z * charactereffectworkPointer->velocity.z));
    if (interpolationValue > 6400) {
        interpolationValue2 = 80.0f / sqrtf(interpolationValue);
        positionVector.x = charactereffectworkPointer->velocity.x * interpolationValue2;
        positionVector.z = charactereffectworkPointer->velocity.z * interpolationValue2;
        charactereffectworkPointer->velocityChange.x = positionVector.x - charactereffectworkPointer->velocity.x;
        charactereffectworkPointer->velocityChange.z = positionVector.z - charactereffectworkPointer->velocity.z;
        interpolationValue3 = 80.0f * sqrtf(0.51f);
        positionVector2.x = interpolationValue3 - positionVector.x;
        positionVector2.z = -positionVector.z;
        positionVector3.x = -interpolationValue3 - positionVector.x;
        positionVector3.z = -positionVector.z;
        interpolationValue = 1.0f / VECMagXZ(&positionVector2);
        positionVector2.x *= interpolationValue;
        positionVector2.z *= interpolationValue;
        interpolationValue = 1.0f / VECMagXZ(&positionVector3);
        positionVector3.x *= interpolationValue;
        positionVector3.z *= interpolationValue;
        positionVector2.x += positionVector3.x;
        positionVector2.z += positionVector3.z;
        interpolationValue = 1.0f / VECMagXZ(&positionVector2);
        positionVector2.x *= interpolationValue;
        positionVector2.z *= interpolationValue;
        interpolationValue2 = -((positionVector2.x * charactereffectworkPointer->acceleration.x) + (positionVector2.z * charactereffectworkPointer->acceleration.z));
        charactereffectworkPointer->acceleration.x += (positionVector2.x * 20.0f) + (positionVector2.x * 2.0f * interpolationValue2);
        charactereffectworkPointer->acceleration.z += (positionVector2.z * 20.0f) + (positionVector2.z * 2.0f * interpolationValue2);
        return 1;
    }
    else {
        return 0;
    }
}

void InitializeCharacterObject(OMOBJ *object);
void InitializeCharacterMotions(OMOBJ *object);

void CreateCharacterObjects(HUPROCESS *objman)
{
    s32 i;
    omMakeGroupEx(objman, 0, 4);
    characterObjects = omGetGroupMemberListEx(objman, 0);
    characterObjectCount = 0;
    for (i = 0; i < 4; i++) {
        OMOBJ *object = omAddObjEx(objman, 40, 5, 7, 0, InitializeCharacterObject);
        if (!GWPlayerCfg[i].iscom) {
            characterObjectCount++;
        }
        object->work[0] = i;
    }
    characterMotionManager = omAddObjEx(objman, 10, 0, 0, -1, InitializeCharacterMotions);
}

s32 characterEffectModelIds[] = {
    DATA_MAKE_NUM(DATADIR_M408, 0x02),
    DATA_MAKE_NUM(DATADIR_M408, 0x03),
    DATA_MAKE_NUM(DATADIR_M408, 0x04),
    DATA_MAKE_NUM(DATADIR_M408, 0x05),
    DATA_MAKE_NUM(DATADIR_M408, 0x06),
    DATA_MAKE_NUM(DATADIR_M408, 0x07),
    DATA_MAKE_NUM(DATADIR_M408, 0x08),
    DATA_MAKE_NUM(DATADIR_M408, 0x09),
};

u32 characterMotionIds[] = {
    0,
    23,
    24,
    DATA_MAKE_NUM(DATADIR_M408, 0x2C),
    DATA_MAKE_NUM(DATADIR_M408, 0x3C),
    DATA_MAKE_NUM(DATADIR_M408, 0x34),
    22,
};

float characterStartPositions[][2] = { -40, -30, 40, -30, -40, 30, 40, 30 };

float characterEffectOffsets[][2] = { 0, -56, 0, 56, -80, 0, 80, 0 };

void UpdateCharacterObject(OMOBJ *object);
void UpdateCharacterParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

void InitializeCharacterObject(OMOBJ *object)
{
    CharacterEffectWork *charactereffectworkPointer;
    s32 playerIndex;
    s32 createdObjectId;

    float scratchValue[4] = { 0.25, 0.5, 0.75, 1.0 };
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(CharacterEffectWork), HU_MEMNUM_OVL);
    charactereffectworkPointer = object->data;
    memset(charactereffectworkPointer, 0, sizeof(CharacterEffectWork));
    charactereffectworkPointer->playerIndex = object->work[0];
    charactereffectworkPointer->characterId = GWPlayerCfg[charactereffectworkPointer->playerIndex].character;
    charactereffectworkPointer->characterId &= 0x7;
    charactereffectworkPointer->animationState = 0;
    charactereffectworkPointer->movementSpeed = scratchValue[GWPlayerCfg[object->work[0]].diff & 0x3];
    charactereffectworkPointer->velocity.x = characterStartPositions[charactereffectworkPointer->playerIndex][0];
    charactereffectworkPointer->velocity.y = stageVerticalPosition;
    charactereffectworkPointer->velocity.z = characterStartPositions[charactereffectworkPointer->playerIndex][1];
    charactereffectworkPointer->acceleration.x = charactereffectworkPointer->acceleration.y = charactereffectworkPointer->acceleration.z = 0;
    charactereffectworkPointer->velocityChange.x = charactereffectworkPointer->velocityChange.y = charactereffectworkPointer->velocityChange.z = 0;
    charactereffectworkPointer->rotation.y = 0;
    charactereffectworkPointer->rotation.x = 0;
    charactereffectworkPointer->modelRotation.x = charactereffectworkPointer->modelRotation.y = charactereffectworkPointer->modelRotation.z = 0;
    charactereffectworkPointer->rotation.z = 9.0f;
    charactereffectworkPointer->steeringVector.x = charactereffectworkPointer->steeringVector.y = charactereffectworkPointer->steeringVector.z = 0;
    charactereffectworkPointer->animationSpeed = 4;
    charactereffectworkPointer->motionScale = 0;
    CalculateCharacterModelOffset(charactereffectworkPointer, &charactereffectworkPointer->modelOffset);
    omSetTra(object, charactereffectworkPointer->velocity.x + charactereffectworkPointer->modelOffset.x, charactereffectworkPointer->velocity.y + charactereffectworkPointer->modelOffset.y, charactereffectworkPointer->velocity.z + charactereffectworkPointer->modelOffset.z);
    omSetSca(object, 0.075f, 0.075f, 0.075f);
    object->mdlId[0] = CharModelCreate(charactereffectworkPointer->characterId, 4);
    for (playerIndex = 0; playerIndex < 7; playerIndex++) {
        if (characterMotionIds[playerIndex] < 0x10000) {
            object->mtnId[playerIndex] = CharMotionCreate(charactereffectworkPointer->characterId, characterMotionIds[playerIndex]);
        }
        else {
            object->mtnId[playerIndex] = Hu3DJointMotionFile(object->mdlId[0], charactereffectworkPointer->characterId + characterMotionIds[playerIndex]);
        }
    }
    CharMotionVoiceOnSet(charactereffectworkPointer->characterId, object->mtnId[1], 0);
    CharMotionShiftSet(charactereffectworkPointer->characterId, object->mtnId[0], 0, 0.2, HU3D_MOTATTR_LOOP);
    CharEffectLayerSet(2);
    CharMotionDataClose(charactereffectworkPointer->characterId);
    object->mdlId[1] = Hu3DModelCreateFile(characterEffectModelIds[charactereffectworkPointer->characterId]);
    Hu3DModelLayerSet(object->mdlId[1], 6);
    Hu3DModelScaleSet(object->mdlId[1], 0.1f * charactereffectworkPointer->animationSpeed, 0.1f * charactereffectworkPointer->animationSpeed, 0.1f * charactereffectworkPointer->animationSpeed);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    charactereffectworkPointer->particlePosition.x = charactereffectworkPointer->particlePosition.y = charactereffectworkPointer->particlePosition.z = 0;
    charactereffectworkPointer->particleScaleVector.x = charactereffectworkPointer->particleScaleVector.y = charactereffectworkPointer->particleScaleVector.z = 0;
    UpdateCharacterParticleModel(object, 1.0f);
    createdObjectId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M408, 0x24)), 256);
    object->mdlId[2] = createdObjectId;
    Hu3DModelLayerSet(createdObjectId, 6);
    Hu3DParticleColSet(createdObjectId, 255, 255, 255);
    Hu3DParticleHookSet(createdObjectId, UpdateCharacterParticle);
    Hu3DModelPosSet(createdObjectId, 0, 0, 0);
    ((HU3DPARTICLE *)Hu3DData[createdObjectId].hookData)->dataCnt = 0;
    charactereffectworkPointer->steeringTimer = charactereffectworkPointer->steeringUpdateCounter = 0;
    charactereffectworkPointer->steeringOffset[0] = charactereffectworkPointer->steeringOffset[1] = 0;
    charactereffectworkPointer->targetCharacterIndex = -1;
    charactereffectworkPointer->steeringState = 0;
    charactereffectworkPointer->steeringAngles[0] = charactereffectworkPointer->steeringAngles[1] = 0;
    object->work[1] = 0;
    object->objFunc = UpdateCharacterObject;
}

void SetParticleEmission(s32 particleModelId, Vec *emissionPosition, u32 particleCount);
void UpdateCharacterAnimation(OMOBJ *object);

void UpdateCharacterObject(OMOBJ *object)
{
    CharacterEffectWork *charactereffectworkPointer = object->data;
    Vec positionVector[] = { { 8, -25, -18 }, { 2, -25, -10 }, { 2, -25, -24 }, { -3, -25, -15 } };
    Vec positionVector2;
    Vec positionVector3;
    Vec positionVector4;
    float scratchValue[] = { 45, 135, 225, 315 };
    float interpolationValue;
    switch (object->work[1]) {
        case 0:
            GetCameraTargetTransform(&positionVector2, &positionVector3);
            RotateVectorAndTranslate(&positionVector2, &positionVector3, &positionVector[charactereffectworkPointer->playerIndex], &positionVector4);
            omSetRot(object, 0, 90, 0);
            omSetTra(object, positionVector4.x, positionVector4.y, positionVector4.z);
            if (GetCameraSequenceState() != 1) {
                object->work[1]++;
                charactereffectworkPointer->rotation.y = 60;
                charactereffectworkPointer->rotation.x = scratchValue[charactereffectworkPointer->playerIndex];
                charactereffectworkPointer->acceleration.x = charactereffectworkPointer->acceleration.y = charactereffectworkPointer->acceleration.z = 0;
                object->trans.x -= 10.0 * (2.0 * sind(charactereffectworkPointer->rotation.x));
                object->trans.z -= 10.0 * (2.0 * cosd(charactereffectworkPointer->rotation.x));
                CharMotionShiftSet(charactereffectworkPointer->characterId, object->mtnId[4], 0, 0.2, HU3D_MOTATTR_LOOP);
                charactereffectworkPointer->velocity.x = charactereffectworkPointer->particlePosition.x = object->trans.x;
                charactereffectworkPointer->velocity.y = charactereffectworkPointer->particlePosition.y = object->trans.y;
                charactereffectworkPointer->velocity.z = charactereffectworkPointer->particlePosition.z = object->trans.z;
            }
            break;

        case 1:
            charactereffectworkPointer->acceleration.y += -98.0f;
            if (charactereffectworkPointer->acceleration.y < -400.0f) {
                charactereffectworkPointer->acceleration.y = -400.0f;
            }
            interpolationValue = object->trans.y;
            object->trans.y += charactereffectworkPointer->acceleration.y * REFRESH_FREQ;
#if VERSION_PAL
            charactereffectworkPointer->rotation.x += 1.8f;
#else
            charactereffectworkPointer->rotation.x += 1.5000001f;
#endif
            if (charactereffectworkPointer->rotation.x >= 360.0f) {
                charactereffectworkPointer->rotation.x -= 360.0f;
            }
            object->trans.x += (3.141590118408203f * (10.0 * sind(charactereffectworkPointer->rotation.x - 90.0f))) * REFRESH_FREQ;
            object->trans.z += (3.141590118408203f * (10.0 * cosd(charactereffectworkPointer->rotation.x - 90.0f))) * REFRESH_FREQ;
            if (interpolationValue > 12000 && object->trans.y <= 12000) {
                positionVector2.x = object->trans.x;
                positionVector2.y = object->trans.y;
                positionVector2.z = object->trans.z;
                SetParticleEmission(object->mdlId[2], &positionVector2, 100);
            }
            charactereffectworkPointer->modelRotation.x = charactereffectworkPointer->rotation.y;
            charactereffectworkPointer->modelRotation.y = charactereffectworkPointer->rotation.x;
            charactereffectworkPointer->modelRotation.z = 0;
            charactereffectworkPointer->velocity.x = object->trans.x;
            charactereffectworkPointer->velocity.y = object->trans.y;
            charactereffectworkPointer->velocity.z = object->trans.z;
            omSetRot(object, charactereffectworkPointer->modelRotation.x, charactereffectworkPointer->modelRotation.y, charactereffectworkPointer->modelRotation.z);
            Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
            charactereffectworkPointer->motionScale += 0.2f * (1.0f - charactereffectworkPointer->motionScale);
            if (object->trans.y < 11940) {
                object->work[1]++;
            }
            break;
    }
    UpdateCharacterParticleModel(object, 1.0f);
    if (GetStageEventState() == 1) {
        omSetSca(object, 0.15f, 0.15f, 0.15f);
        object->trans.x = charactereffectworkPointer->velocity.x = charactereffectworkPointer->particlePosition.x = characterStartPositions[charactereffectworkPointer->playerIndex][0];
        object->trans.y = charactereffectworkPointer->velocity.y = charactereffectworkPointer->particlePosition.y = stageVerticalPosition;
        object->trans.z = charactereffectworkPointer->velocity.z = charactereffectworkPointer->particlePosition.z = characterStartPositions[charactereffectworkPointer->playerIndex][1];
        object->rot.y = charactereffectworkPointer->rotation.x = atan2d(object->trans.x, object->trans.z) + 180.0;
        Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
        ((HU3DPARTICLE *)Hu3DData[object->mdlId[2]].hookData)->dataCnt = 0;
        object->objFunc = UpdateCharacterAnimation;
    }
}

void SetCharacterMotion(OMOBJ *object, s8 *motionIds, s8 *motionFlags)
{
    float angle;
    float angle2;
    float angle3;
    CharacterEffectWork *charactereffectworkPointer;
    CharacterEffectWork *charactereffectworkPointer2;
    s8 frameCounter;
    s32 playerIndex;
    s32 integerValue;
    Vec positionVector;
    Vec positionVector2;
    s8 scratchValue[4];

    charactereffectworkPointer = object->data;
    *motionIds = charactereffectworkPointer->steeringOffset[0];
    *motionFlags = charactereffectworkPointer->steeringOffset[1];
    positionVector.x = charactereffectworkPointer->velocity.x;
    positionVector.y = charactereffectworkPointer->velocity.y;
    positionVector.z = charactereffectworkPointer->velocity.z;
    integerValue = findNearestEffectPosition(&positionVector, &positionVector2);
    frameCounter = 0;
    scratchValue[0] = -1;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerIndex != charactereffectworkPointer->playerIndex) {
            charactereffectworkPointer2 = GetCharacterWork(playerIndex);
            if ((charactereffectworkPointer2->animationState & 0x12) == 0) {
                scratchValue[frameCounter++] = playerIndex;
            }
        }
    }
    switch (charactereffectworkPointer->steeringState) {
        case 0:
            if (frameCounter > 0 || integerValue) {
                if (frameCounter > 0) {
                    charactereffectworkPointer->targetCharacterIndex = scratchValue[frandmod(frameCounter)];
                }
#if VERSION_PAL
                charactereffectworkPointer->steeringTimer = frandmod(150) + 100.0f;
#else
                charactereffectworkPointer->steeringTimer = frandmod(180) + 120.0f;
#endif
                charactereffectworkPointer->steeringUpdateCounter = 0;
                charactereffectworkPointer->steeringState = 1;
            }
            else {
                charactereffectworkPointer->steeringUpdateCounter = 0;
                charactereffectworkPointer->steeringState = 2;
            }
            break;

        case 1:
            if ((charactereffectworkPointer->steeringUpdateCounter & 0x3) == 0) {
                if (integerValue) {
                    angle = atan2d(positionVector2.x - charactereffectworkPointer->velocity.x, positionVector2.z - charactereffectworkPointer->velocity.z);
                }
                else {
                    charactereffectworkPointer2 = GetCharacterWork(charactereffectworkPointer->targetCharacterIndex);
                    angle = atan2d(charactereffectworkPointer2->velocity.x - charactereffectworkPointer->velocity.x, charactereffectworkPointer2->velocity.z - charactereffectworkPointer->velocity.z);
                }
                charactereffectworkPointer->steeringAngles[1] = ApproachAngle(charactereffectworkPointer->steeringAngles[0], angle, (0.3f * charactereffectworkPointer->movementSpeed) + 0.15f);
            }
            charactereffectworkPointer->steeringUpdateCounter++;
            charactereffectworkPointer->steeringAngles[0] = ApproachAngle(charactereffectworkPointer->steeringAngles[0], charactereffectworkPointer->steeringAngles[1], 0.2f);
            charactereffectworkPointer->steeringOffset[0] = sind(charactereffectworkPointer->steeringAngles[0]) * 80.0;
            charactereffectworkPointer->steeringOffset[1] = -cosd(charactereffectworkPointer->steeringAngles[0]) * 80.0;
            *motionIds = charactereffectworkPointer->steeringOffset[0];
            *motionFlags = charactereffectworkPointer->steeringOffset[1];
            charactereffectworkPointer->steeringTimer--;
            if (integerValue == 0 && (charactereffectworkPointer2->animationState & 0x12)) {
                charactereffectworkPointer->steeringTimer = 0;
            }
            if (charactereffectworkPointer->steeringTimer == 0) {
                charactereffectworkPointer->steeringState = 0;
            }
            break;

        case 2:
            if ((charactereffectworkPointer->steeringUpdateCounter & 0xF) == 0) {
                angle2 = (0.04f * frandmod(1000)) - 20.0f;
                angle3 = (0.04f * frandmod(1000)) - 20.0f;
                charactereffectworkPointer->steeringAngles[1] = angle = atan2d(angle2 - charactereffectworkPointer->velocity.x, angle3 - charactereffectworkPointer->velocity.z);
            }
            charactereffectworkPointer->steeringUpdateCounter++;
            charactereffectworkPointer->steeringAngles[0] = ApproachAngle(charactereffectworkPointer->steeringAngles[0], charactereffectworkPointer->steeringAngles[1], 0.2f);
            charactereffectworkPointer->steeringOffset[0] = sind(charactereffectworkPointer->steeringAngles[0]) * 80.0;
            charactereffectworkPointer->steeringOffset[1] = -cosd(charactereffectworkPointer->steeringAngles[0]) * 80.0;
            *motionIds = charactereffectworkPointer->steeringOffset[0];
            *motionFlags = charactereffectworkPointer->steeringOffset[1];
            if (frameCounter > 0 || integerValue) {
                charactereffectworkPointer->steeringState = 0;
            }
            break;
    }
}

#define CHECK_IDLE(pos) (((pos) < 0) ? -(pos) : ((pos) > 15))

void UpdateCharacterMovement(OMOBJ *object);
void InitializeCharacterParticleEffect(OMOBJ *object);
void UpdateCharacterEffect(OMOBJ *object);

void UpdateCharacterAnimation(OMOBJ *object)
{
    CharacterEffectWork *charactereffectworkPointer;
    s32 integerValue;

    float interpolationValue;
    float angle;
    float interpolationValue2;
    float angle2;
    float angle3;
    s8 scratchValue;
    s8 scratchValue2;

    charactereffectworkPointer = object->data;
    integerValue = GWPlayerCfg[charactereffectworkPointer->playerIndex].padIdx;
    scratchValue = scratchValue2 = 0;
    if (stageProgressState == 3) {
        if (!GWPlayerCfg[charactereffectworkPointer->playerIndex].iscom) {
            scratchValue = HuPadStkX[integerValue];
            scratchValue2 = HuPadStkY[integerValue];
        }
        else {
            SetCharacterMotion(object, &scratchValue, &scratchValue2);
        }
    }
    charactereffectworkPointer->velocity.y = stageVerticalPosition;
    angle2 = angle3 = 0;
    angle = charactereffectworkPointer->rotation.x;
    interpolationValue2 = -20;
    if (CHECK_IDLE(scratchValue) || CHECK_IDLE(scratchValue2)) {
        angle = atan2d(scratchValue, -scratchValue2);
        if (angle < 0) {
            angle += 360;
        }
        angle2 = 70 * sind(angle);
        angle3 = 70 * cosd(angle);
        interpolationValue2 = 30;
    }
    GetCharacterEffectState(object);
#if VERSION_PAL
#define _VAL_1 (23.0f / 25.0f)
#define _VAL_2 (2.0f / 25.0f)
#else
#define _VAL_1 (28.0f / 30.0f)
#define _VAL_2 (2.0f / 30.0f)
#endif
    charactereffectworkPointer->acceleration.x = (_VAL_1 * charactereffectworkPointer->acceleration.x) + (_VAL_2 * angle2);
    charactereffectworkPointer->acceleration.z = (_VAL_1 * charactereffectworkPointer->acceleration.z) + (_VAL_2 * angle3);
#undef _VAL_1
#undef _VAL_2
    interpolationValue = VECMagXZ(&charactereffectworkPointer->acceleration);
    if (interpolationValue > 200.0f) {
        interpolationValue = 200.0f / interpolationValue;
        charactereffectworkPointer->acceleration.x *= interpolationValue;
        charactereffectworkPointer->acceleration.z *= interpolationValue;
    }
    charactereffectworkPointer->velocityChange.x += (charactereffectworkPointer->acceleration.x * REFRESH_FREQ);
    charactereffectworkPointer->velocityChange.y = 0;
    charactereffectworkPointer->velocityChange.z += (charactereffectworkPointer->acceleration.z * REFRESH_FREQ);
    charactereffectworkPointer->rotation.x = angle;
    charactereffectworkPointer->rotation.y = (0.9f * charactereffectworkPointer->rotation.y) + (0.1f * interpolationValue2);
    charactereffectworkPointer->modelRotation.x = charactereffectworkPointer->rotation.y;
    charactereffectworkPointer->modelRotation.y = ApproachAngle(charactereffectworkPointer->modelRotation.y, charactereffectworkPointer->rotation.x, 0.2f);
    charactereffectworkPointer->modelRotation.z = 0;
    omSetRot(object, charactereffectworkPointer->modelRotation.x, charactereffectworkPointer->modelRotation.y, charactereffectworkPointer->modelRotation.z);
    CalculateCharacterModelOffset(charactereffectworkPointer, &charactereffectworkPointer->modelOffset);
    omSetTra(object, charactereffectworkPointer->velocity.x + charactereffectworkPointer->modelOffset.x, charactereffectworkPointer->velocity.y + charactereffectworkPointer->modelOffset.y, charactereffectworkPointer->velocity.z + charactereffectworkPointer->modelOffset.z);
    charactereffectworkPointer->motionScale += (1.0f - charactereffectworkPointer->motionScale) * 0.2f;
    UpdateCharacterParticleModel(object, 0.3f);
    if (charactereffectworkPointer->animationState & 0x1) {
        charactereffectworkPointer->animationSpeed--;
        charactereffectworkPointer->animationEventTimer = 0;
        charactereffectworkPointer->animationState |= 2;
        interpolationValue = VECMagXZ(&charactereffectworkPointer->steeringVector);
        if (interpolationValue > 0.0f) {
            interpolationValue = 150.0f / interpolationValue;
        }
        charactereffectworkPointer->acceleration.x = charactereffectworkPointer->steeringVector.x * interpolationValue;
        charactereffectworkPointer->acceleration.z = charactereffectworkPointer->steeringVector.z * interpolationValue;
        charactereffectworkPointer->velocityChange.x = charactereffectworkPointer->acceleration.x * REFRESH_FREQ;
        charactereffectworkPointer->velocityChange.z = charactereffectworkPointer->acceleration.z * REFRESH_FREQ;
        CharMotionShiftSet(charactereffectworkPointer->characterId, object->mtnId[5], 0, 0.2f, HU3D_MOTATTR_NONE);
        HuAudFXPlay(1358);
        omVibrate(object->work[0], 12, 4, 2);
        if (charactereffectworkPointer->animationSpeed) {
            CharFXPlay(charactereffectworkPointer->characterId, 296);
            object->objFunc = UpdateCharacterMovement;
        }
        else {
            charactereffectworkPointer->animationState |= 0x30;
            activeCharacterCountdown--;
            CharFXPlay(charactereffectworkPointer->characterId, 291);
            object->objFunc = UpdateCharacterEffect;
        }
    }
    if (GetStageEventState() == 4 && (charactereffectworkPointer->animationState & 0x20) == 0) {
        CharMotionShiftSet(charactereffectworkPointer->characterId, object->mtnId[3], 0, 0.2f, HU3D_MOTATTR_NONE);
        Hu3DModelShadowSet(object->mdlId[0]);
        object->work[1] = 0;
        object->objFunc = InitializeCharacterParticleEffect;
    }
}
#undef CHECK_IDLE

void UpdateCharacterMovement(OMOBJ *object)
{
    CharacterEffectWork *charactereffectworkPointer;
    s32 playerIndex;
    s32 integerValue;
    s32 scratchValue;
    charactereffectworkPointer = object->data;
    scratchValue = charactereffectworkPointer->playerIndex;
    charactereffectworkPointer->animationState |= 2;
    charactereffectworkPointer->velocity.y = stageVerticalPosition;
    GetCharacterEffectState(object);
#if VERSION_PAL
    charactereffectworkPointer->acceleration.x = (24.0f / 25.0f) * charactereffectworkPointer->acceleration.x;
    charactereffectworkPointer->acceleration.z = (24.0f / 25.0f) * charactereffectworkPointer->acceleration.z;
#else
    charactereffectworkPointer->acceleration.x = (29.0f / 30.0f) * charactereffectworkPointer->acceleration.x;
    charactereffectworkPointer->acceleration.z = (29.0f / 30.0f) * charactereffectworkPointer->acceleration.z;
#endif
    charactereffectworkPointer->velocityChange.x += charactereffectworkPointer->acceleration.x * REFRESH_FREQ;
    charactereffectworkPointer->velocityChange.y = 0;
    charactereffectworkPointer->velocityChange.z += charactereffectworkPointer->acceleration.z * REFRESH_FREQ;

#if VERSION_PAL
    charactereffectworkPointer->rotation.x += 10.8f;
#else
    charactereffectworkPointer->rotation.x += 9.0f;
#endif
    if (charactereffectworkPointer->rotation.x >= 360.0f) {
        charactereffectworkPointer->rotation.x -= 360.0f;
    }
    charactereffectworkPointer->rotation.y = 0.9f * charactereffectworkPointer->rotation.y;
    charactereffectworkPointer->modelRotation.x = charactereffectworkPointer->rotation.y;
    charactereffectworkPointer->modelRotation.y = charactereffectworkPointer->rotation.x;
    charactereffectworkPointer->modelRotation.z = 0;
    omSetRot(object, charactereffectworkPointer->modelRotation.x, charactereffectworkPointer->modelRotation.y, charactereffectworkPointer->modelRotation.z);
    CalculateCharacterModelOffset(charactereffectworkPointer, &charactereffectworkPointer->modelOffset);
    omSetTra(object, charactereffectworkPointer->velocity.x + charactereffectworkPointer->modelOffset.x, charactereffectworkPointer->velocity.y + charactereffectworkPointer->modelOffset.y, charactereffectworkPointer->velocity.z + charactereffectworkPointer->modelOffset.z);
    UpdateCharacterParticleModel(object, 0.8f);
    integerValue = charactereffectworkPointer->animationEventTimer & 0x1;
    charactereffectworkPointer->animationEventTimer++;
    if (GetStageEventState() == 4) {
        integerValue = 0;
        CharMotionShiftSet(charactereffectworkPointer->characterId, object->mtnId[3], 0, 0.2f, HU3D_MOTATTR_NONE);
        Hu3DModelShadowSet(object->mdlId[0]);
        object->work[1] = 0;
        object->objFunc = InitializeCharacterParticleEffect;
    }
    else if (charactereffectworkPointer->animationEventTimer > REFRESH_RATE) {
        integerValue = 0;
        charactereffectworkPointer->animationState &= ~0x3;
        CharMotionShiftSet(charactereffectworkPointer->characterId, object->mtnId[4], 0, 0.2f, HU3D_MOTATTR_LOOP);
        object->objFunc = UpdateCharacterAnimation;
    }
    if (integerValue) {
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        for (playerIndex = 0; playerIndex < charactereffectworkPointer->animationSpeed; playerIndex++) {
            Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
        }
    }
    else {
        Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
        for (playerIndex = 0; playerIndex < charactereffectworkPointer->animationSpeed; playerIndex++) {
            Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
        }
    }
}

Vec characterParticleOffsets[] = { { -50, 0, 0 }, { -16, 0, 0 }, { 16, 0, 0 }, { 50, 0, 0 } };

Vec effectLightVectors[] = { { 0, 1500, 2500 }, { 1500, 0, 1 }, { 0, 0, 0 } };

void UpdateCharacterParticleEffect(OMOBJ *object);

void InitializeCharacterParticleEffect(OMOBJ *object)
{
    CharacterEffectWork *charactereffectworkPointer;
    s32 particleIndex;

    charactereffectworkPointer = object->data;
    object->trans.x = characterParticleOffsets[object->work[0]].x;
    object->trans.z = characterParticleOffsets[object->work[0]].z;
    if (object->work[1] == 0) {
        if (maxCharacterAnimationSpeed < charactereffectworkPointer->animationSpeed) {
            maxCharacterAnimationSpeed = charactereffectworkPointer->animationSpeed;
        }
    }
    else if (object->work[1] == 10 && maxCharacterAnimationSpeed == charactereffectworkPointer->animationSpeed) {
        for (particleIndex = 0; activeCharacterIds[particleIndex] >= 0; particleIndex++)
            ;
        activeCharacterIds[particleIndex] = charactereffectworkPointer->characterId;
    }
    object->work[1]++;
    charactereffectworkPointer->velocity.y = stageVerticalPosition;
    charactereffectworkPointer->rotation.y = 0;
    charactereffectworkPointer->rotation.x = 0;
    object->rot.x = object->rot.y = object->rot.z = 0;
    object->trans.y = charactereffectworkPointer->velocity.y;
    charactereffectworkPointer->velocity.x = object->trans.x;
    charactereffectworkPointer->velocity.z = object->trans.z;
    charactereffectworkPointer->velocity.y += characterVerticalOffsets[charactereffectworkPointer->characterId];
    if (object->trans.y < 0) {
        object->trans.y = 0;
    }
    UpdateCharacterParticleModel(object, 1.0f);
    if (GetStageEventState() == 5) {
        object->objFunc = UpdateCharacterParticleEffect;
        if (charactereffectworkPointer->motionDirection) {
            stageEffectEnabled = 1;
        }
    }
}

void UpdateCharacterParticleEffect(OMOBJ *object)
{
    CharacterEffectWork *charactereffectworkPointer = object->data;
    InitializeCharacterEffect(object);
    if (!(charactereffectworkPointer->animationState & 0x8)) {
        charactereffectworkPointer->animationState |= 0x8;
        if (charactereffectworkPointer->motionDirection != 0) {
            CharMotionShiftSet(charactereffectworkPointer->characterId, object->mtnId[1], 0, 0.2f, HU3D_MOTATTR_NONE);
            CharFXPlay(charactereffectworkPointer->characterId, 290);
            GWPlayerCoinCollectAdd(object->work[0], charactereffectworkPointer->motionDirection);
        }
        else {
            CharMotionShiftSet(charactereffectworkPointer->characterId, object->mtnId[2], 0, 0.2f, HU3D_MOTATTR_NONE);
        }
        object->work[1] = 0;
    }
    else if (object->work[1] == 0 && Hu3DMotionEndCheck(object->mdlId[0])) {
        activeCharacterCountdown--;
        object->work[1]++;
    }
}

void InitializeCharacterParticleSystem(OMOBJ *object);

void UpdateCharacterEffect(OMOBJ *object)
{
    CharacterEffectWork *charactereffectworkPointer;
    s32 modelIndex;
    charactereffectworkPointer = object->data;
    if (fabs(object->trans.x) < 160.0 || fabs(object->trans.z) < 160.0) {
        charactereffectworkPointer->velocity.x += charactereffectworkPointer->acceleration.x * REFRESH_FREQ;
        charactereffectworkPointer->velocity.z += charactereffectworkPointer->acceleration.z * REFRESH_FREQ;
#if VERSION_PAL
        charactereffectworkPointer->rotation.x += 10.8f;
#else
        charactereffectworkPointer->rotation.x += 9.0f;
#endif
        if (charactereffectworkPointer->rotation.x >= 360.0f) {
            charactereffectworkPointer->rotation.x -= 360.0f;
        }
        charactereffectworkPointer->rotation.y = 0.9f * charactereffectworkPointer->rotation.y;
        charactereffectworkPointer->modelRotation.x = 90.0f + charactereffectworkPointer->rotation.y;
        charactereffectworkPointer->modelRotation.y = charactereffectworkPointer->rotation.x;
        charactereffectworkPointer->modelRotation.z = 0;
        omSetRot(object, charactereffectworkPointer->modelRotation.x, charactereffectworkPointer->modelRotation.y, charactereffectworkPointer->modelRotation.z);
        CalculateCharacterModelOffset(charactereffectworkPointer, &charactereffectworkPointer->modelOffset);
        omSetTra(object, charactereffectworkPointer->velocity.x + charactereffectworkPointer->modelOffset.x, charactereffectworkPointer->velocity.y + charactereffectworkPointer->modelOffset.y, charactereffectworkPointer->velocity.z + charactereffectworkPointer->modelOffset.z);
    }
    if (GetStageEventState() >= 4) {
        GWPlayerCoinCollectAdd(object->work[0], charactereffectworkPointer->motionDirection);
        if (stageFinishStarted) {
            Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
            CharMotionSet(charactereffectworkPointer->characterId, object->mtnId[6]);
            Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
            object->trans.x = characterParticleOffsets[object->work[0]].x;
            object->trans.y = 0;
            object->trans.z = characterParticleOffsets[object->work[0]].z;
            object->rot.x = object->rot.y = object->rot.z = 0;
        }
        else {
            Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        }

        for (modelIndex = 0; modelIndex < 4; modelIndex++) {
            Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
        }
        object->objFunc = InitializeCharacterParticleSystem;
    }
}

void InitializeCharacterParticleSystem(OMOBJ *object)
{
    CharacterEffectWork *charactereffectworkPointer = object->data;
}

void UpdateCharacterParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    float interpolationValue;
    if (particle->dataCnt == 0) {
        particle->dataCnt++;
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->time = 0;
            particleData->scale = 0;
            particleData->color.a = 0;
        }
        return;
    }
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->time == 0 || particleData->time != 1) {
            continue;
        }
        particleData->parManId++;
#if VERSION_PAL
        interpolationValue = particleData->parManId - 8.5f;
#else
        interpolationValue = particleData->parManId - 10.2f;
#endif
        if (interpolationValue < 0.0f) {
            particleData->pos.x += (particleData->vel.x - particleData->pos.x) * 0.2f;
            particleData->pos.y += (particleData->vel.y - particleData->pos.y) * 0.15f;
            particleData->pos.z += (particleData->vel.z - particleData->pos.z) * 0.2f;
            particleData->scale += (particleData->accel.x - particleData->scale) * 0.2f;
            particleData->accel.y += (particleData->accel.z - particleData->accel.y) * 0.2f;
            particleData->color.a = particleData->accel.y;
        }
        else {
            particleData->pos.x += (particleData->speedDecay * 1.5f) * REFRESH_FREQ;
            particleData->pos.y += (particleData->colorIdx * 1.5f) * REFRESH_FREQ;
            particleData->pos.z += (particleData->scaleBase * 1.5f) * REFRESH_FREQ;
            particleData->scale += (particleData->accel.x * 0.01f);
            particleData->color.a = 0.92f * particleData->color.a;
        }
    }
    DCStoreRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void SetParticleEmission(s32 particleModelId, Vec *emissionPosition, u32 particleCount)
{
    float angle;
    float angle2;

    HU3DPARTICLEDATA *particleData;
    HU3DPARTICLE *particleWork;
    s32 modelIndex;
    particleWork = Hu3DData[particleModelId].hookData;
    if (particleWork->dataCnt != 0) {
        particleData = particleWork->data;
        for (modelIndex = 0; modelIndex < particleWork->maxCnt; modelIndex++, particleData++) {
            if (particleData->time) {
                continue;
            }
            particleData->time = 1;
            particleData->parManId = 0;
            particleData->pos.x = emissionPosition->x;
            particleData->pos.y = 11990;
            particleData->pos.z = emissionPosition->z;
            angle = frandmod(1000) * (1.0f / 1000.0f);
            angle2 = frandmod(1000) * (360.0f / 1000.0f);
            particleData->speedDecay = 10 * (0.8f * (angle * sind(angle2)));
            particleData->colorIdx = 10.0f * (5.0f * (1.0f - (angle * angle)));
            particleData->scaleBase = 10 * (0.8f * (angle * cosd(angle2)));
            particleData->vel.x = particleData->speedDecay + emissionPosition->x;
            particleData->vel.y = particleData->colorIdx + particleData->pos.y;
            particleData->vel.z = particleData->scaleBase + emissionPosition->z;
            particleData->accel.x = (6.0f + (0.012f * frandmod(1000)));
            particleData->scale = 2.0f;
            particleData->accel.y = particleData->color.a = 10;
            particleData->accel.z = frandmod(100) + 64;
            angle = 0.00040000002f * frandmod(1000);
            angle += 0.6f * cosd(angle2);
            particleData->color.r = (55 * angle) + 200;
            particleData->color.g = (50 * angle) + 205;
            particleData->color.b = (5 * angle) + 250;
            if (--particleCount == 0) {
                break;
            }
        }
    }
}

typedef struct CharacterMotionWork {
    s32 playerIndex;
    s32 animationState;
    u32 animationSpeed;
    float motionScale[2];
    s32 motionDirection;
    s32 position;
    s32 velocity[7];
    float cameraAngle[7];
    s16 particleScale[7];
} CharacterMotionWork;

void UpdateCharacterMotions(OMOBJ *object);

void InitializeCharacterMotions(OMOBJ *object)
{
    CharacterMotionWork *charactermotionworkPointer;
    s32 playerIndex;
    s32 playerIndex2;
    s32 integerValue;

    CharacterMotionWork *integerValue2;

    float scratchValue[][2] = { 96, 64, 486, 64, 96, 416, 486, 416 };
    float scratchValue2[][2] = { 0, 0, 0, 0, -54, -1, 2, 1, -20, 0, 42, 2, 22, 2 };

    object->stat |= 0x100;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, 4 * sizeof(CharacterMotionWork), HU_MEMNUM_OVL);
    integerValue2 = object->data;
    memset(integerValue2, 0, 4 * sizeof(CharacterMotionWork));
    charactermotionworkPointer = integerValue2;
    for (playerIndex2 = 0; playerIndex2 < 4; playerIndex2++, charactermotionworkPointer++) {
        for (playerIndex = 0; playerIndex < 7; playerIndex++) {
            charactermotionworkPointer->particleScale[playerIndex] = -1;
            charactermotionworkPointer->cameraAngle[playerIndex] = 1;
        }
        charactermotionworkPointer->motionScale[0] = scratchValue[playerIndex2][0];
        charactermotionworkPointer->motionScale[1] = scratchValue[playerIndex2][1];
        integerValue = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x29), 4, 0);
        charactermotionworkPointer->particleScale[0] = integerValue;
        espColorSet(integerValue, 0, 0, 0);
        espTPLvlSet(integerValue, 0.5);
        charactermotionworkPointer->particleScale[1] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x28), 3, 0);
        charactermotionworkPointer->particleScale[2] = espEntry(characterAssetIds[GWPlayerCfg[playerIndex2].character], 2, 0);
        charactermotionworkPointer->particleScale[3] = espEntry(DATA_MAKE_NUM(DATADIR_M408, 0x2B), 2, 0);
        charactermotionworkPointer->particleScale[4] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x08), 2, 0);
        charactermotionworkPointer->particleScale[6] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x33), 2, 0);
        charactermotionworkPointer->particleScale[5] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x33), 2, 0);
        for (playerIndex = 0; playerIndex < 7; playerIndex++) {
            if (charactermotionworkPointer->particleScale[playerIndex] >= 0) {
                espPosSet(charactermotionworkPointer->particleScale[playerIndex], charactermotionworkPointer->motionScale[0] + scratchValue2[playerIndex][0], charactermotionworkPointer->motionScale[1] + scratchValue2[playerIndex][1]);
                espDispOff(charactermotionworkPointer->particleScale[playerIndex]);
            }
        }
        charactermotionworkPointer->playerIndex = 0;
    }
    HuDataDirClose(DATADIR_MGCONST);
    object->objFunc = UpdateCharacterMotions;
    (void)integerValue2;
    (void)integerValue2;
}

void UpdateCharacterMotions(OMOBJ *object)
{
    float interpolationValue;
    CharacterMotionWork *charactermotionworkPointer;
    s32 playerIndex;
    s32 playerIndex2;
    CharacterMotionWork *integerValue = object->data;
    charactermotionworkPointer = integerValue;
    for (playerIndex2 = 0; playerIndex2 < 4; playerIndex2++, charactermotionworkPointer++) {
        switch (charactermotionworkPointer->motionDirection) {
            case 0:
                if (GetStageEventState() == 3) {
                    charactermotionworkPointer->motionDirection = 1;
                }
                break;

            case 1:
                charactermotionworkPointer->position = 0;
                charactermotionworkPointer->velocity[5] = charactermotionworkPointer->animationSpeed % 10;
                charactermotionworkPointer->velocity[6] = (charactermotionworkPointer->animationSpeed / 10) % 10;
                for (playerIndex = 0; playerIndex < 7; playerIndex++) {
                    if (charactermotionworkPointer->particleScale[playerIndex] >= 0) {
                        espDispOn(charactermotionworkPointer->particleScale[playerIndex]);
                    }
                }
                if (charactermotionworkPointer->playerIndex) {
                    charactermotionworkPointer->cameraAngle[5] = charactermotionworkPointer->cameraAngle[6] = 0.5f;
                    charactermotionworkPointer->motionDirection++;
                }
                else {
                    charactermotionworkPointer->playerIndex = 1;
                    charactermotionworkPointer->cameraAngle[5] = charactermotionworkPointer->cameraAngle[6] = 1.0f;
                    charactermotionworkPointer->motionDirection += 3;
                    break;
                }

            case 2:
                interpolationValue = charactermotionworkPointer->cameraAngle[5];
#if VERSION_PAL
                interpolationValue += 0.12f;
#else
                interpolationValue += 0.10000001f;
#endif
                if (interpolationValue >= 1.5f) {
                    charactermotionworkPointer->motionDirection++;
                }
                charactermotionworkPointer->cameraAngle[5] = charactermotionworkPointer->cameraAngle[6] = interpolationValue;
                break;

            case 3:
                interpolationValue = charactermotionworkPointer->cameraAngle[5];
#if VERSION_PAL
                interpolationValue -= 0.12f;
#else
                interpolationValue -= 0.10000001f;
#endif
                if (interpolationValue <= 1.0f) {
                    interpolationValue = 1.0f;
                    charactermotionworkPointer->motionDirection++;
                }
                charactermotionworkPointer->cameraAngle[5] = charactermotionworkPointer->cameraAngle[6] = interpolationValue;
                break;

            default:
                break;
        }
        charactermotionworkPointer->position++;
        for (playerIndex = 0; playerIndex < 7; playerIndex++) {
            espBankSet(charactermotionworkPointer->particleScale[playerIndex], charactermotionworkPointer->velocity[playerIndex]);
            espScaleSet(charactermotionworkPointer->particleScale[playerIndex], charactermotionworkPointer->cameraAngle[playerIndex], charactermotionworkPointer->cameraAngle[playerIndex]);
        }
    }
}

void SetStageProgress(u16 stageIndex, u16 progressState)
{
    CharacterMotionWork *charactermotionworkPointer;
    charactermotionworkPointer = characterMotionManager->data;
    if (GetStageEventState() == 3) {
        charactermotionworkPointer[stageIndex].animationState += progressState;
        charactermotionworkPointer[stageIndex].animationSpeed += progressState;
        charactermotionworkPointer[stageIndex].motionDirection = 1;
    }
}

u32 GetStageProgress(void)
{
    return minigameState;
}

u32 GetStageEventState(void)
{
    return stageProgressState;
}

float GetStageEventValue(void)
{
    return stageVerticalPosition;
}

void EmptyStageCallback(void) { }

s32 GetStageEventCounter(void)
{
    return 0;
}

void EmptyStageUpdate(void) { }

CharacterEffectWork *GetCharacterWork(s32 playerIndex)
{
    return characterObjects[playerIndex]->data;
}

float InterpolateCurveValue(float startValue, float startTangent, float controlValue1, float controlValue2, float endTangent, float endValue, float controlTangent1, float controlTangent2, float interpolationTime, float curveParameter)
{
    float interpolationValue;
    float scratchValue;
    float interpolationValue2;
    float interpolationValue3;

    float interpolationValue4;
    float interpolationValue5;

    float scratchValue2;

    scratchValue = (endTangent + curveParameter) * (endTangent + curveParameter);
    interpolationValue4 = (interpolationTime * interpolationTime) + (((controlValue2 * controlValue2) + ((controlTangent2 * controlTangent2) + ((controlValue1 * controlValue1) - (2.0f * controlValue1 * controlTangent2)))) - (2.0f * controlValue2 * interpolationTime));
    interpolationValue5 = 2.0f
        * ((controlTangent1 * interpolationTime) + ((((startTangent * controlValue2) + ((endValue * controlTangent2) + (((startValue * controlValue1) - (startValue * controlTangent2)) - (endValue * controlValue1)))) - (startTangent * interpolationTime)) - (controlTangent1 * controlValue2)));
    scratchValue2 = ((controlTangent1 * controlTangent1) + (((startTangent * startTangent) + ((endValue * endValue) + ((startValue * startValue) - (2.0f * startValue * endValue)))) - (2.0f * startTangent * controlTangent1))) - scratchValue;
    if (0.0f == interpolationValue4) {
        return -1;
    }
    interpolationValue = (interpolationValue5 * interpolationValue5) - (4.0f * interpolationValue4 * scratchValue2);
    if (interpolationValue <= 0.0) {
        return -1;
    }
    interpolationValue = sqrtf(interpolationValue);
    interpolationValue2 = (-interpolationValue5 + interpolationValue) / (2.0f * interpolationValue4);
    interpolationValue3 = (-interpolationValue5 - interpolationValue) / (2.0f * interpolationValue4);
    if (interpolationValue2 >= 0.0f && interpolationValue3 >= 0.0f) {
        if (interpolationValue2 < interpolationValue3) {
            if (interpolationValue2 < 1.0f) {
                return interpolationValue2;
            }
        }
        if (interpolationValue3 < 1.0f) {
            return interpolationValue3;
        }
    }
    return -1;
}

void EvaluateCurve(float *coefficients, float parameter, float startValue, float endValue, float startTangent, float endTangent, float scale)
{
    float interpolationValue;
    float interpolationValue2;
    float interpolationValue3;
    float interpolationValue4;
    float scale2;

    float interpolationValue5;

    interpolationValue2 = endValue;
    interpolationValue3 = startTangent;
    interpolationValue = sqrtf((interpolationValue2 * interpolationValue2) + (interpolationValue3 * interpolationValue3));
    if (interpolationValue != 0.0) {
        interpolationValue5 = 1.0f / interpolationValue;
        interpolationValue2 *= interpolationValue5;
        interpolationValue3 *= interpolationValue5;
    }
    interpolationValue4 = endTangent - parameter;
    scale2 = scale - startValue;
    interpolationValue5 = 1.0f / sqrtf((interpolationValue4 * interpolationValue4) + (scale2 * scale2));
    interpolationValue4 *= interpolationValue5;
    scale2 *= interpolationValue5;
    interpolationValue5 = (interpolationValue2 * interpolationValue4) + (interpolationValue3 * scale2);
    coefficients[0] = interpolationValue5 * (interpolationValue4 * interpolationValue);
    coefficients[1] = interpolationValue5 * (scale2 * interpolationValue);
}

void SolveCurveParameters(float startX, float startY, float controlX, float controlY, float endX, float endY, float startTangent, float endTangent, float *playerIndex, float *coefficientB, float *coefficientC,
    float *coefficientD)
{
    float scratchValue[2];
    float scratchValue2[2];
    float scratchValue3;
    float scratchValue4;
    scratchValue3 = startX + (0.5f * (endX - startX));
    scratchValue4 = startY + (0.5f * (endY - startY));
    EvaluateCurve(scratchValue, coefficients, parameter, startValue, endValue, scratchValue3, scratchValue4);
    EvaluateCurve(scratchValue2, coefficients, parameter, startValue, endValue, scratchValue3, scratchValue4);
    *playerIndex = scratchValue2[0] - scratchValue[0];
    *coefficientB = scratchValue2[1] - scratchValue[1];
    *coefficientC = scratchValue[0] - scratchValue2[0];
    *coefficientD = scratchValue[1] - scratchValue2[1];
}

void CalculateCurveCoefficients(float *coefficientA, float *coefficientB, float value0, float value1, float value2, float value3, float value4, float value5)
{
    float interpolationValue = value0 + (0.5f * (value4 - value0));
    float interpolationValue2 = value1 + (0.5f * (value5 - value1));
    float interpolationValue3 = value0 - interpolationValue;
    float interpolationValue4 = value1 - interpolationValue2;
    float interpolationValue5;
    if (interpolationValue3 == 0.0f || interpolationValue4 == 0.0f) {
        *coefficientA = *coefficientB = 0.0f;
        return;
    }
    interpolationValue5 = 1.0f / sqrtf((interpolationValue3 * interpolationValue3) + (interpolationValue4 * interpolationValue4));
    interpolationValue3 *= interpolationValue5;
    interpolationValue4 *= interpolationValue5;
    *coefficientA = (2.0f * interpolationValue3) * ((-value2 * interpolationValue3) - (value3 * interpolationValue4));
    *coefficientB = (2.0f * interpolationValue4) * ((-value2 * interpolationValue3) - (value3 * interpolationValue4));
}

void UpdateCharacterPair(OMOBJ *firstCharacter, OMOBJ *secondCharacter)
{
    CharacterEffectWork *charactereffectworkPointer = firstCharacter->data;
    CharacterEffectWork *charactereffectworkPointer2 = secondCharacter->data;

    float scratchValue;
    float scratchValue2;
    float scratchValue3;
    float scratchValue4;
    SolveCurveParameters(charactereffectworkPointer->velocity.x, charactereffectworkPointer->velocity.z, charactereffectworkPointer->velocityChange.x, charactereffectworkPointer->velocityChange.z, charactereffectworkPointer2->velocity.x, charactereffectworkPointer2->velocity.z, charactereffectworkPointer2->velocityChange.x,
        charactereffectworkPointer2->velocityChange.z, &scratchValue, &scratchValue2, &scratchValue3, &scratchValue4);
    charactereffectworkPointer->velocityChange.x += scratchValue;
    charactereffectworkPointer->velocityChange.z += scratchValue2;
    charactereffectworkPointer2->velocityChange.x += scratchValue3;
    charactereffectworkPointer2->velocityChange.z += scratchValue4;
}

s32 GetCharacterCount(void)
{
    float interpolationValue;
    float interpolationValue2;

    CharacterEffectWork *charactereffectworkPointer;
    s32 playerIndex;
    CharacterEffectWork *charactereffectworkPointer2;
    CharacterEffectWork *charactereffectworkPointer3;
    s32 playerIndex2;
    s32 integerValue;
    OMOBJ *integerValue2;
    OMOBJ *integerValue3;
    OMOBJ *object;

    OMOBJ *candidateCharacterObjects[2];
    integerValue = -1;
    interpolationValue2 = 0;
    interpolationValue = 9999;
    for (playerIndex2 = 0; playerIndex2 < 3; playerIndex2++) {
        integerValue2 = characterObjects[playerIndex2];
        charactereffectworkPointer2 = integerValue2->data;
        if (charactereffectworkPointer2->animationState & 0x10) {
            continue;
        }
        for (playerIndex = playerIndex2 + 1; playerIndex < 4; playerIndex++) {
            integerValue3 = characterObjects[playerIndex];
            charactereffectworkPointer3 = integerValue3->data;
            if (charactereffectworkPointer3->animationState & 0x10) {
                continue;
            }
            interpolationValue2
                = InterpolateCurveValue(charactereffectworkPointer2->velocity.x, charactereffectworkPointer2->velocity.z, charactereffectworkPointer2->velocityChange.x * cameraFadeValue, charactereffectworkPointer2->velocityChange.z * cameraFadeValue, charactereffectworkPointer2->rotation.z,
                    charactereffectworkPointer3->velocity.x, charactereffectworkPointer3->velocity.z, charactereffectworkPointer3->velocityChange.x * cameraFadeValue, charactereffectworkPointer3->velocityChange.z * cameraFadeValue, charactereffectworkPointer3->rotation.z);
            if (interpolationValue2 >= 0.0 && interpolationValue2 <= interpolationValue) {
                interpolationValue = interpolationValue2;
                integerValue = 1;
                candidateCharacterObjects[0] = integerValue2;
                candidateCharacterObjects[1] = integerValue3;
            }
        }
    }
    if (integerValue < 0) {
        return 0;
    }
    if (interpolationValue > 0.0f) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            charactereffectworkPointer = characterObjects[playerIndex]->data;
            charactereffectworkPointer->animationPhaseTimer = 0;
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        object = characterObjects[playerIndex];
        charactereffectworkPointer = object->data;
        if (!(charactereffectworkPointer->animationState & 0x10)) {
            charactereffectworkPointer->velocity.x += cameraFadeValue * (charactereffectworkPointer->velocityChange.x * interpolationValue);
            charactereffectworkPointer->velocity.z += cameraFadeValue * (charactereffectworkPointer->velocityChange.z * interpolationValue);
        }
    }
    if (integerValue != 1) {
        (void)integerValue;
    }
    else {
        UpdateCharacterPair(candidateCharacterObjects[0], candidateCharacterObjects[1]);
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            charactereffectworkPointer = candidateCharacterObjects[playerIndex]->data;
            if (charactereffectworkPointer->animationState & 0x10) {
                continue;
            }
            charactereffectworkPointer->animationPhaseTimer++;
            if (charactereffectworkPointer->motionDirection == 0) {
                cameraFallStarted = 1;
            }
#if VERSION_PAL
            charactereffectworkPointer->motionDirection = 15;
#else
            charactereffectworkPointer->motionDirection = 18;
#endif
            if (charactereffectworkPointer->animationPhaseTimer > 2.0f) {
                charactereffectworkPointer->velocityChange.x = charactereffectworkPointer->velocityChange.z = 0;
            }
        }
    }
    cameraFadeValue *= (1.0f - interpolationValue);
    return 1;
}

void UpdateStageObjects(void)
{
    CharacterEffectWork *charactereffectworkPointer;
    s32 playerIndex;
    OMOBJ *object;
    cameraFadeValue = 1.0f;
    cameraFallStarted = 0;
    while (GetCharacterCount())
        ;
    if (cameraFallStarted) {
        HuAudFXPlay(1359);
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        object = characterObjects[playerIndex];
        charactereffectworkPointer = object->data;
        if (charactereffectworkPointer->position != 0) {
            charactereffectworkPointer->position--;
        }
        if (charactereffectworkPointer->animationState & 0x10) {
            continue;
        }
        charactereffectworkPointer->velocity.x += charactereffectworkPointer->velocityChange.x * cameraFadeValue;
        charactereffectworkPointer->velocity.z += charactereffectworkPointer->velocityChange.z * cameraFadeValue;
        charactereffectworkPointer->acceleration.x = charactereffectworkPointer->velocityChange.x * REFRESH_RATE;
        charactereffectworkPointer->acceleration.z = charactereffectworkPointer->velocityChange.z * REFRESH_RATE;
        charactereffectworkPointer->velocityChange.x = charactereffectworkPointer->velocityChange.z = 0;
    }
}

void GetCameraPositionRotation(Vec *cameraPosition, Vec *cameraRotation)
{
    Vec positionVector;
    positionVector.x = cameraRotation->x - cameraPosition->x;
    positionVector.y = cameraRotation->y - cameraPosition->y;
    positionVector.z = cameraRotation->z - cameraPosition->z;
    Center.x = cameraRotation->x;
    Center.y = cameraRotation->y;
    Center.z = cameraRotation->z;
    CRot.x = atan2d(positionVector.y, VECMagXZ(&positionVector));
    CRot.y = atan2d(-positionVector.x, -positionVector.z);
    CRot.z = 0;
    CZoom = sqrtf(VECMag2Point(&positionVector));
}

void SetCameraParameters(float targetX, float targetY, float targetZ, float positionX, float positionY, float positionZ)
{
    Vec positionVector;
    Vec positionVector2;
    positionVector.x = targetX;
    positionVector.y = targetY;
    positionVector.z = targetZ;
    positionVector2.x = positionX;
    positionVector2.y = positionY;
    positionVector2.z = positionZ;
    GetCameraPositionRotation(&positionVector, &positionVector2);
}

float ApproachAngle(float currentAngle, float targetAngle, float stepScale)
{
    float scale;
    scale = fmodf(targetAngle - currentAngle, 360);
    if (scale < 0.0f) {
        scale += 360.0f;
    }
    if (scale > 180.0f) {
        scale -= 360.0f;
    }
    scale = fmodf((stepScale * scale) + currentAngle, 360);
    if (scale < 0.0f) {
        scale += 360.0f;
    }
    return scale;
}

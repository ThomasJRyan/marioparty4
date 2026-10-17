#include "game/audio.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/hu3d.h"
#include "game/sprite.h"

#include "REL/m408Dll.h"

#include "ext_math.h"

#ifndef __MWERKS__
#include "game/frand.h"
#include "version.h"
#endif
s32 rand8(void);

typedef struct EffectSpawnPattern {
    s32 state;
    float elapsedFrames;
    float positionX;
} EffectSpawnPattern;

typedef struct StageEffectWork {
    s32 state;
    s32 elapsedFrames;
    s32 positionX;
    Vec position;
    Vec targetPosition;
    Vec velocity;
    float rotation;
    Vec scale;
    float motionPhases[2];
    float motionFrame;
    float motionSpeed;
    s32 animationId;
    s32 animationFrame;
    s32 effectType;
    s32 effectState;
    s32 effectTimer;
    float effectScale;
} StageEffectWork;

void UpdateEffectTypeOne(StageEffectWork *effectWork);
void UpdateEffectTypeTwo(StageEffectWork *effectWork);
void UpdateEffectTypeThree(StageEffectWork *effectWork);
void UpdateEffectTypeFour(StageEffectWork *effectWork);
void UpdateEffectTypeFive(StageEffectWork *effectWork);
void UpdateEffectTypeSix(StageEffectWork *effectWork);
void UpdateEffectTypeSeven(StageEffectWork *effectWork);
void UpdateEffectTypeEight(StageEffectWork *effectWork);
void UpdateEffectTypeNine(StageEffectWork *effectWork);

s32 effectModelAssetIds[9] = {
    DATA_MAKE_NUM(DATADIR_M408, 0x12),
    DATA_MAKE_NUM(DATADIR_M408, 0x12),
    DATA_MAKE_NUM(DATADIR_M408, 0x12),
    DATA_MAKE_NUM(DATADIR_M408, 0x12),
    DATA_MAKE_NUM(DATADIR_M408, 0x12),
    DATA_MAKE_NUM(DATADIR_M408, 0x14),
    DATA_MAKE_NUM(DATADIR_M408, 0x14),
    DATA_MAKE_NUM(DATADIR_M408, 0x16),
    DATA_MAKE_NUM(DATADIR_M408, 0x17),
};

s32 effectMotionAssetIds[9] = {
    DATA_MAKE_NUM(DATADIR_M408, 0x13),
    DATA_MAKE_NUM(DATADIR_M408, 0x13),
    DATA_MAKE_NUM(DATADIR_M408, 0x13),
    DATA_MAKE_NUM(DATADIR_M408, 0x13),
    DATA_MAKE_NUM(DATADIR_M408, 0x13),
    DATA_MAKE_NUM(DATADIR_M408, 0x15),
    DATA_MAKE_NUM(DATADIR_M408, 0x15),
    0,
    0,
};

typedef void (*StageEffectUpdateFunc)(StageEffectWork *effectWork);

StageEffectUpdateFunc effectUpdateCallbacks[9] = { UpdateEffectTypeOne, UpdateEffectTypeTwo, UpdateEffectTypeThree, UpdateEffectTypeFour, UpdateEffectTypeFive, UpdateEffectTypeSix, UpdateEffectTypeSeven, UpdateEffectTypeEight, UpdateEffectTypeNine };

EffectSpawnPattern effectSpawnPatterns[] = {
    { 0, 10600, 100 },
    { 0, 10600, 290 },
    { 7, 10500, 0 },
    { 7, 10400, 72 },
    { 7, 10300, 144 },
    { 7, 10200, 216 },
    { 7, 10100, 288 },
    { 5, 9600, 0 },
    { 7, 9000, 0 },
    { 7, 9000, 72 },
    { 7, 9000, 144 },
    { 7, 9000, 216 },
    { 7, 9000, 288 },
    { 1, 8600, 150 },
    { 7, 8500, 216 },
    { 7, 8400, 288 },
    { 7, 8300, 0 },
    { 7, 8200, 72 },
    { 7, 8100, 144 },
    { 6, 7700, 0 },
    { 6, 7700, 180 },
    { 7, 7500, 165 },
    { 7, 7500, 285 },
    { 7, 7500, 45 },
    { 8, 7000, 225 },
    { 8, 7000, 345 },
    { 8, 7000, 105 },
    { 7, 6500, 165 },
    { 7, 6500, 285 },
    { 7, 6500, 45 },
    { 4, 5500, 0 },
    { 4, 5500, 120 },
    { 4, 5500, 240 },
    { 7, 4700, 0 },
    { 7, 4700, 72 },
    { 7, 4700, 144 },
    { 7, 4700, 216 },
    { 7, 4700, 288 },
    { 5, 4500, 120 },
    { 7, 4250, 165 },
    { 7, 4250, 285 },
    { 7, 4250, 45 },
    { 5, 4000, 120 },
    { 7, 3750, 225 },
    { 7, 3750, 345 },
    { 7, 3750, 105 },
    { 5, 3500, 240 },
    { 7, 3250, 165 },
    { 7, 3250, 285 },
    { 7, 3250, 45 },
    { 7, 2100, 36 },
    { 7, 2100, 108 },
    { 7, 2100, 180 },
    { 7, 2100, 252 },
    { 7, 2100, 324 },
    { 3, 1500, 0 },
    { 3, 1500, 120 },
    { 3, 1500, 240 },
    { 8, 1300, 165 },
    { 8, 1300, 285 },
    { 8, 1300, 45 },
    { -1, 10000, 0 },
};

GXColor effectModelColors[4] = { { 255, 255, 255, 255 }, { 255, 255, 0, 255 }, { 0, 255, 255, 255 }, { 255, 200, 200, 255 } };

u8 effectModelOrder[] = { 5, 3, 1, 0 };

float effectMotionFrames[3] = { 72, 120, 0 };

float effectMotionOffsets[3] = { 0, 60, 0 };

float effectMotionAngles[3] = { 30, 60, 90 };

Vec effectLightSettings[3] = { { 1500, 2500, 1500 }, { 0, 1, 0 }, { 0, 0, 0 } };

OMOBJ *effectModelObject;
OMOBJ *effectModelManager;
StageEffectWork stageEffectInstances[256];
s32 activeEffectCount;
s32 nextEffectSlot;
Vec characterTargetPositions[4];
Vec effectSpawnPositions[256];
u32 effectAnimationSeed;

void InitializeEffectModels(OMOBJ *object);
void InitializeEffectModelManager(OMOBJ *object);

void CreateEffectObjects(HUPROCESS *objman)
{
    effectAnimationSeed = 0;
    effectModelObject = omAddObjEx(objman, 40, 256, 1, -1, InitializeEffectModels);
    effectModelManager = omAddObjEx(objman, 41, 4, 0, -1, InitializeEffectModelManager);
}

void SpawnEffectModel(StageEffectWork *effectWork, s32 effectType);
void UpdateEffectModels(OMOBJ *object);
void UpdateEffectAnimation(OMOBJ *object);

void SetEffectModelDataCount(s16 modelId, s16 dataCount);
void UpdateEffectModelParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

void InitializeEffectModels(OMOBJ *object)
{
    StageEffectWork *stageeffectworkPointer;
    s32 modelIndex;
    s16 createdObjectId;
    s16 createdObjectId2;
    s16 createdObjectId3;
    s16 createdObjectId4;
    s16 createdObjectId5;
    s16 stateValue;
    s16 stateValue2;
    stageeffectworkPointer = &stageEffectInstances[0];
    activeEffectCount = 0;
    nextEffectSlot = 0;
    while (effectSpawnPatterns[activeEffectCount].state != -1) {
        if (effectSpawnPatterns[activeEffectCount].state == 7) {
            nextEffectSlot++;
        }
        activeEffectCount++;
    }
    object->stat |= 0x100;
    createdObjectId2 = createdObjectId3 = createdObjectId4 = createdObjectId5 = -1;
    for (modelIndex = 0; modelIndex < activeEffectCount; modelIndex++, stageeffectworkPointer++) {
        SpawnEffectModel(stageeffectworkPointer, modelIndex);
        switch (stageeffectworkPointer->state) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
                if (createdObjectId2 < 0) {
                    createdObjectId2 = Hu3DModelCreateFile(effectModelAssetIds[stageeffectworkPointer->state]);
                    stateValue = Hu3DJointMotionFile(createdObjectId2, effectMotionAssetIds[stageeffectworkPointer->state]);
                    Hu3DModelAttrSet(createdObjectId2, HU3D_MOTATTR_LOOP);
                    Hu3DMotionSet(createdObjectId2, stateValue);
                    createdObjectId = createdObjectId2;
                }
                else {
                    createdObjectId = Hu3DModelLink(createdObjectId2);
                }
                break;

            case 5:
            case 6:
                if (createdObjectId3 < 0) {
                    createdObjectId3 = Hu3DModelCreateFile(effectModelAssetIds[stageeffectworkPointer->state]);
                    stateValue2 = Hu3DJointMotionFile(createdObjectId3, effectMotionAssetIds[stageeffectworkPointer->state]);
                    Hu3DModelAttrSet(createdObjectId3, HU3D_MOTATTR_LOOP);
                    Hu3DMotionSet(createdObjectId3, stateValue2);
                    createdObjectId = createdObjectId3;
                }
                else {
                    createdObjectId = Hu3DModelLink(createdObjectId3);
                }
                break;

            case 7:
                if (createdObjectId4 < 0) {
                    createdObjectId4 = Hu3DModelCreateFile(effectModelAssetIds[stageeffectworkPointer->state]);
                    createdObjectId = createdObjectId4;
                }
                else {
                    createdObjectId = Hu3DModelLink(createdObjectId4);
                }
                break;

            case 8:
                if (createdObjectId5 < 0) {
                    createdObjectId5 = Hu3DModelCreateFile(effectModelAssetIds[stageeffectworkPointer->state]);
                    createdObjectId = createdObjectId5;
                }
                else {
                    createdObjectId = Hu3DModelLink(createdObjectId5);
                }
                break;

            default:
                break;
        }
        object->mdlId[modelIndex] = createdObjectId;
        Hu3DModelPosSetV(object->mdlId[modelIndex], &stageeffectworkPointer->position);
        Hu3DModelRotSetV(object->mdlId[modelIndex], &stageeffectworkPointer->scale);
        Hu3DModelScaleSet(object->mdlId[modelIndex], 0.15f, 0.15f, 0.15f);
        Hu3DModelCameraSet(object->mdlId[modelIndex], 1);
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
        if (stageeffectworkPointer->state < 7) {
            Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_MOTATTR_LOOP);
        }
    }
    for (modelIndex = activeEffectCount; modelIndex < activeEffectCount + nextEffectSlot; modelIndex++) {
        createdObjectId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M408, 0x18)), 32);
        object->mdlId[modelIndex] = createdObjectId;
        Hu3DModelAttrSet(createdObjectId, HU3D_ATTR_DISPOFF);
        Hu3DModelLayerSet(createdObjectId, 6);
        Hu3DParticleColSet(createdObjectId, 255, 255, 255);
        Hu3DParticleHookSet(createdObjectId, UpdateEffectModelParticle);
        Hu3DModelPosSet(createdObjectId, 0, 0, 0);
        SetEffectModelDataCount(createdObjectId, 0);
    }
    object->objFunc = UpdateEffectModels;
}

void SpawnEffectModel(StageEffectWork *effectWork, s32 effectType)
{
    float scale2;
    float scale3;
    s32 frameCounter;
    frameCounter = 0;
    scale3 = 40;
    scale2 = 20;
    effectWork->state = effectSpawnPatterns[effectType].state;
    effectWork->elapsedFrames = effectType;
    effectWork->positionX = -1;
    effectWork->targetPosition.x = effectWork->position.x = 0;
    effectWork->targetPosition.y = effectWork->position.y = effectSpawnPatterns[effectType].elapsedFrames;
    effectWork->targetPosition.z = effectWork->position.z = 0;
    effectWork->velocity.x = effectWork->velocity.y = effectWork->velocity.z = 0;
    effectWork->motionFrame = 180;
    effectWork->scale.x = effectWork->scale.z = effectWork->scale.y = 0;
    effectWork->motionPhases[0] = effectWork->motionPhases[1] = 0;
    effectWork->rotation = 0;
    effectWork->motionSpeed = 1;
    effectWork->animationId = 1;
    effectWork->animationFrame = 0;
    effectWork->effectState = 1;
    effectWork->effectType = 0;
    effectWork->effectTimer = 0;
    effectWork->effectScale = 0;
    switch (effectWork->state) {
        case 0:
            effectWork->effectScale = scale2 + (rand8() % (int)scale3);
            effectWork->rotation = effectSpawnPatterns[effectType].positionX;
            effectWork->scale.y = 180 + (-effectWork->rotation + 90);

            break;

        case 1:
            effectWork->scale.y = effectWork->motionPhases[0] = 180 + (-effectWork->rotation + 90);
            effectWork->rotation = rand8() % 360;
            break;

        case 2:
            effectWork->rotation = effectSpawnPatterns[effectType].positionX;
            effectWork->scale.y = 90 + (180 + (-effectWork->rotation + 90));
            break;

        case 3:
            effectWork->rotation = effectSpawnPatterns[effectType].positionX;
            effectWork->scale.y = (180 + (-effectWork->rotation + 90)) - 90;
            break;

        case 4:
            effectWork->rotation = effectSpawnPatterns[effectType].positionX;
            effectWork->scale.y = 180 + (-effectWork->rotation + 90);
            break;

        case 5:
            effectWork->rotation = rand8() % 360;
            effectWork->scale.y = 180 + (-effectWork->rotation + 90);
            effectWork->motionFrame = 0;
            break;

        case 6:
            effectWork->rotation = effectSpawnPatterns[effectType].positionX;
            effectWork->scale.y = -effectWork->rotation + 90;
            break;

        case 7:
        case 8:
            effectWork->rotation = effectSpawnPatterns[effectType].positionX;
            effectWork->positionX = activeEffectCount + frameCounter;
            frameCounter++;
            break;
    }
    if (effectWork->scale.y > 360) {
        effectWork->scale.y -= 360;
    }
    else {
        if (effectWork->scale.y < 0) {
            effectWork->scale.y += 360;
        }
    }
}

void UpdateEffectModels(OMOBJ *object)
{
    s32 animationIndex;
    u32 integerValue;
    integerValue = GetStageEventState();
    if (integerValue == 2) {
        for (animationIndex = 0; animationIndex < activeEffectCount; animationIndex++)
            ;
        object->objFunc = UpdateEffectAnimation;
    }
}

void EmptyEffectCallback(OMOBJ *object);
float UpdateEffectModel(StageEffectWork *object);
void CleanupEffectModel(StageEffectWork *object);

void UpdateEffectAnimation(OMOBJ *object)
{
    StageEffectWork *stageeffectworkPointer;
    s32 modelIndex;
    u32 integerValue;
    stageeffectworkPointer = &stageEffectInstances[0];
    effectAnimationSeed = 0;
    for (modelIndex = 0; modelIndex < activeEffectCount; modelIndex++, stageeffectworkPointer++) {
        if (stageeffectworkPointer->animationId == 1) {
            effectUpdateCallbacks[stageeffectworkPointer->state](stageeffectworkPointer);
            if (stageeffectworkPointer->effectState == 0) {
                CleanupEffectModel(stageeffectworkPointer);
            }
            Hu3DModelPosSetV(object->mdlId[modelIndex], &stageeffectworkPointer->position);
            Hu3DModelRotSetV(object->mdlId[modelIndex], &stageeffectworkPointer->scale);
            Hu3DModelScaleSet(object->mdlId[modelIndex], 0.15f * stageeffectworkPointer->motionSpeed, 0.15f * stageeffectworkPointer->motionSpeed, 0.15f * stageeffectworkPointer->motionSpeed);
            if (stageeffectworkPointer->position.y > GetStageEventValue() - 1000 && stageeffectworkPointer->position.y < GetStageEventValue() + 200) {
                Hu3DModelAttrReset(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
            }
            else {
                Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
            }
            if (stageeffectworkPointer->state == 7 || stageeffectworkPointer->state == 8) {
                if (stageeffectworkPointer->position.y > GetStageEventValue() - 150 && stageeffectworkPointer->position.y < GetStageEventValue() + 10) {
                    effectSpawnPositions[effectAnimationSeed++] = stageeffectworkPointer->position;
                }
            }
        }
        else {
            Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
        }
    }
    integerValue = GetStageEventState();
    if (integerValue == 5) {
        for (modelIndex = 0; modelIndex < activeEffectCount; modelIndex++, stageeffectworkPointer++) {
            Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
        }
        object->objFunc = EmptyEffectCallback;
    }
}

void EmptyEffectCallback(OMOBJ *object) { }

void SetEffectModelPosition(s16 modelId, float positionX, float positionY, float positionZ);
void SetEffectModelMotion(s16 modelId);

float UpdateEffectModel(StageEffectWork *effectWork)
{
    Vec positionVector;
    float interpolationValue;
    float angle;
    float interpolationValue2;
    CharacterEffectWork *charactereffectworkPointer;
    s32 itemIndex;
    angle = -1;
    interpolationValue2 = 10000;
    for (itemIndex = 0; itemIndex < 4; itemIndex++) {
        charactereffectworkPointer = GetCharacterWork(itemIndex);
        if (charactereffectworkPointer->animationState & 0x10) {
            continue;
        }
        positionVector.x = charactereffectworkPointer->position.x - effectWork->velocity.x;
        positionVector.y = 0;
        positionVector.z = charactereffectworkPointer->position.z - effectWork->velocity.z;
        interpolationValue = VECMag(&positionVector);
        if (interpolationValue < interpolationValue2) {
            angle = atan2d(positionVector.x, positionVector.z);
            interpolationValue2 = interpolationValue;
        }
    }
    return angle;
}

void CleanupEffectModel(StageEffectWork *effectWork)
{
    float distance;
    float distance2;
    float distance3;
    float interpolationValue;
    float interpolationValue2;
    CharacterEffectWork *charactereffectworkPointer;
    s32 animationIndex;
    interpolationValue2 = 14;
    for (animationIndex = 0; animationIndex < 4; animationIndex++) {
        charactereffectworkPointer = GetCharacterWork(animationIndex);
        if (charactereffectworkPointer->animationState & 0x12) {
            continue;
        }
        switch (effectWork->state) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 7:
            case 8:
                distance = charactereffectworkPointer->position.x - effectWork->velocity.x;
                distance3 = charactereffectworkPointer->position.y - effectWork->velocity.y;
                distance2 = charactereffectworkPointer->position.z - effectWork->velocity.z;
                interpolationValue = VECMagPoint(distance, distance3, distance2);
                break;

            case 5:
            case 6:
                distance = charactereffectworkPointer->position.x - effectWork->velocity.x;
                distance2 = charactereffectworkPointer->position.z - effectWork->velocity.z;
                interpolationValue = VECMagPoint(distance, 0, distance2);
                break;
        }
        if (interpolationValue < interpolationValue2 && 0.0f != interpolationValue) {
            if (effectWork->state != 7 && effectWork->state != 8) {
                charactereffectworkPointer->animationState |= 0x1;
                charactereffectworkPointer->steeringVector.x = distance;
                charactereffectworkPointer->steeringVector.z = distance2;
                characterTargetPositions[animationIndex].x = (0.5f * distance) + effectWork->velocity.x;
                characterTargetPositions[animationIndex].z = (0.5f * distance2) + effectWork->velocity.z;
                characterTargetPositions[animationIndex].y = charactereffectworkPointer->position.y;
                effectModelManager->work[0] = animationIndex;
            }
            else {
                effectWork->effectType = 1;
                if (effectWork->state == 8) {
                    charactereffectworkPointer->score += 5;
                    SetStageProgress(animationIndex, 5);
                }
                else {
                    charactereffectworkPointer->score += 1;
                    SetStageProgress(animationIndex, 1);
                }
            }
        }
    }
}

void UpdateEffectTypeOne(StageEffectWork *effectWork)
{
    CharacterEffectWork *charactereffectworkPointer;
    float calculatedFloat;
    float stageHeight;
    float cosineComponent;
    float sineComponent;
    cosineComponent = cosd(effectWork->rotation);
    sineComponent = sind(effectWork->rotation);
    effectWork->position.x = cosineComponent * effectWork->effectScale;
    effectWork->position.z = sineComponent * effectWork->effectScale;
    stageHeight = GetStageEventValue();
    switch (effectWork->animationFrame) {
        case 0:
            if (effectWork->position.y > stageHeight) {
                effectWork->effectTimer = 0;
                effectWork->animationFrame = 1;
            }
            effectWork->scale.x = approachAngle(effectWork->scale.x, -30, 0.1f);
            effectWork->effectState = 1;
            break;

        case 1:
            effectWork->effectState = 0;
            effectWork->position.y = stageHeight;
            effectWork->velocity.x = effectWork->position.x;
            effectWork->velocity.y = effectWork->position.y + 5.0f;
            effectWork->velocity.z = effectWork->position.z;
            calculatedFloat = UpdateEffectModel(effectWork);
            if (calculatedFloat >= 0) {
                effectWork->scale.y = approachAngle(effectWork->scale.y, calculatedFloat, 0.1f);
            }
            effectWork->scale.x = approachAngle(effectWork->scale.x, 0, 0.1f);
#if VERSION_PAL
            if (effectWork->effectTimer < 75.0f) {
#else
            if (effectWork->effectTimer < 90.0f) {
#endif
                effectWork->effectTimer++;
            }
            else {
                effectWork->animationFrame = 3;
            }
            break;

        case 3:
            effectWork->scale.x = approachAngle(effectWork->scale.x, -30, 0.1f);
            effectWork->effectState = 1;
            if (effectWork->position.y - 15.0f > Center.y + CZoom) {
                effectWork->animationFrame = 4;
                effectWork->animationId = 0;
                Hu3DModelAttrSet(effectModelObject->mdlId[effectWork->elapsedFrames], HU3D_ATTR_DISPOFF);
            }
            break;

        case 4:
            break;
    }
}

void UpdateEffectTypeTwo(StageEffectWork *effectWork)
{
    float angle;
    float scale2;
    float angle2;
    float angle3;
    float interpolationValue;
    float interpolationValue2;
    effectWork->motionFrame += 1.5f;
    if (effectWork->motionFrame > 360.0f) {
        effectWork->motionFrame -= 360.0f;
    }
    angle2 = cosd(effectWork->rotation);
    angle3 = sind(effectWork->rotation);
    effectWork->targetPosition.x = angle2 * effectWork->effectScale;
    effectWork->targetPosition.z = angle3 * effectWork->effectScale;
    angle = sind(effectWork->motionFrame);
    effectWork->position.x = angle2 * (60 * angle);
    effectWork->position.z = angle3 * (60 * angle);
    scale2 = -effectWork->rotation - 90;
    if (angle < 0) {
        scale2 += 180;
    }
    effectWork->scale.y = approachAngle(effectWork->scale.y, scale2, 0.05f);
    scale2 = fabs(angle);
    effectWork->scale.x = -30 * scale2;
    interpolationValue2 = GetStageEventValue();
    switch (effectWork->animationFrame) {
        case 0:
            if (effectWork->position.y > interpolationValue2) {
                effectWork->effectTimer = 0;
                effectWork->animationFrame = 1;
            }
            effectWork->effectState = 1;
            if (angle >= sind(75.0f)) {
                interpolationValue = (effectWork->motionFrame - 75) / 30;
                effectWork->motionPhases[1] = 180 + effectWork->motionPhases[0];
            }
            else if (angle <= sind(255.0f)) {
                interpolationValue = (effectWork->motionFrame - 255) / 30;
                effectWork->motionPhases[1] = 180 + effectWork->motionPhases[0];
            }
            else {
                interpolationValue = 0;
                if (effectWork->motionPhases[1] > 360.0f) {
                    effectWork->motionPhases[1] -= 360.0f;
                }
                effectWork->motionPhases[0] = effectWork->motionPhases[1];
            }
            break;

        case 1:
            effectWork->effectState = 0;
            effectWork->position.y = interpolationValue2;
            effectWork->velocity.x = effectWork->position.x;
            effectWork->velocity.y = effectWork->position.y + 5;
            effectWork->velocity.z = effectWork->position.z;
            if (effectWork->effectTimer < REFRESH_RATE_F * 2) {
                effectWork->effectTimer++;
            }
            else {
                effectWork->animationFrame = 3;
            }
            break;

        case 3:
            effectWork->effectState = 1;
            if (effectWork->position.y - 15.0f > Center.y + CZoom) {
                effectWork->animationFrame = 4;
                effectWork->animationId = 0;
                Hu3DModelAttrSet(effectModelObject->mdlId[effectWork->elapsedFrames], HU3D_ATTR_DISPOFF);
            }
            break;

        case 4:
            break;
    }
}

void UpdateEffectTypeThree(StageEffectWork *effectWork)
{
    float interpolationValue;
    float angle;
    float angle2;
    effectWork->rotation += 1.8f;
    if (effectWork->rotation > 360.0f) {
        effectWork->rotation -= 360.0f;
    }
    angle = cosd(effectWork->rotation);
    angle2 = sind(effectWork->rotation);
    effectWork->effectScale = 64;
    effectWork->position.x = angle * effectWork->effectScale;
    effectWork->position.z = angle2 * effectWork->effectScale * 0.7f;
    interpolationValue = GetStageEventValue();
    switch (effectWork->animationFrame) {
        case 0:
            if (effectWork->position.y > interpolationValue) {
                effectWork->effectTimer = 0;
                effectWork->animationFrame = 1;
            }
            effectWork->effectState = 1;
            effectWork->scale.y = -effectWork->rotation;
            if (effectWork->scale.y > 360.0f) {
                effectWork->scale.y -= 360.0f;
            }
            else if (effectWork->scale.y < 0.0f) {
                effectWork->scale.y += 360.0f;
            }
            break;

        case 1:
            effectWork->effectState = 0;
            effectWork->position.y = interpolationValue;
            effectWork->velocity.x = effectWork->position.x;
            effectWork->velocity.y = effectWork->position.y + 5;
            effectWork->velocity.z = effectWork->position.z;
            effectWork->scale.y -= 10.0f;
            if (effectWork->scale.y > 360.0f) {
                effectWork->scale.y -= 360.0f;
            }
            if (effectWork->effectTimer < REFRESH_RATE_F * 2) {
                effectWork->effectTimer++;
            }
            else {
                effectWork->animationFrame = 3;
            }
            break;

        case 3:
            effectWork->effectState = 1;
            if (effectWork->position.y - 15.0f > Center.y + CZoom) {
                effectWork->animationFrame = 4;
                effectWork->animationId = 0;
                Hu3DModelAttrSet(effectModelObject->mdlId[effectWork->elapsedFrames], HU3D_ATTR_DISPOFF);
            }
            break;

        case 4:
            break;
    }
}

void UpdateEffectTypeFour(StageEffectWork *effectWork)
{
    float interpolationValue;
    float interpolationValue2;
    float interpolationValue3;
    float interpolationValue4;
    float interpolationValue5;
    float angle;
    float angle2;
    float angle3;
    float angle4;
    float angle5;
    float angle6;
    float interpolationValue6;
    float angle7;
    float scratchValue;
    effectWork->rotation += 2.0f;
    if (effectWork->rotation > 360.0f) {
        effectWork->rotation -= 360.0f;
    }
    angle = cosd(effectWork->rotation);
    angle2 = sind(effectWork->rotation);
    effectWork->motionPhases[1] = 270.0f;
#ifdef NON_MATCHING
    interpolationValue4 = 0.0f;
    interpolationValue5 = 0.0f;
#endif
    interpolationValue3 = effectWork->targetPosition.y;
    switch (effectWork->animationFrame) {
        case 0:
            interpolationValue = 10;
            interpolationValue4 = angle * interpolationValue;
            interpolationValue5 = angle2 * interpolationValue;
            interpolationValue3 = effectWork->targetPosition.y;
            effectWork->motionPhases[0] = 0;
            effectWork->scale.x = approachAngle(effectWork->scale.x, -30, 0.1);
            effectWork->scale.y = 180.0f + (90.0f + -effectWork->rotation);
            interpolationValue2 = GetStageEventValue();
            if (interpolationValue2 < 5.0f + interpolationValue3) {
                effectWork->motionFrame = 0;
                effectWork->animationFrame = 1;
                effectWork->motionPhases[0] = 8;
            }
            effectWork->effectState = 1;
            break;

        case 1:
            effectWork->motionFrame += 2.0f;
            if (effectWork->motionFrame > 180.0f) {
                effectWork->animationFrame = 3;
            }
            effectWork->motionPhases[0] = 0;
            effectWork->scale.x = approachAngle(effectWork->scale.x, 40, 0.1);
            effectWork->scale.y = approachAngle(effectWork->scale.y, -stepScale->rotation, 0.1);
            angle4 = cosd(effectWork->motionFrame * 0.5f);
            interpolationValue = (-100 * angle4) + 110;
            interpolationValue2 = GetStageEventValue();
            interpolationValue4 = angle * interpolationValue;
            interpolationValue5 = angle2 * interpolationValue;
            interpolationValue3 = interpolationValue2;
            effectWork->velocity.x = interpolationValue4;
            effectWork->velocity.y = interpolationValue2;
            effectWork->velocity.z = interpolationValue5;
            effectWork->effectState = 0;
            break;

        case 3:
            effectWork->effectState = 1;
            effectWork->animationFrame = 4;
            effectWork->animationId = 0;
            Hu3DModelAttrSet(effectModelObject->mdlId[effectWork->elapsedFrames], HU3D_ATTR_DISPOFF);
            break;

        case 4:
            break;
    }
    if (effectWork->scale.y > 360) {
        effectWork->scale.y -= 360;
    }
    else if (effectWork->scale.y < 0) {
        effectWork->scale.y += 360;
    }
    scratchValue = 0;
    angle5 = -cosd(effectWork->scale.x) * 5;
    angle3 = -sind(effectWork->scale.x) * 5;
    angle6 = angle3 * sind(effectWork->scale.y);
    interpolationValue6 = angle5;
    angle7 = angle3 * cosd(effectWork->scale.y);
    effectWork->position.x = angle6 + interpolationValue4;
    effectWork->position.y = interpolationValue6 + interpolationValue3;
    effectWork->position.z = angle7 + interpolationValue5;
}

void UpdateEffectTypeFive(StageEffectWork *effectWork)
{
    float angle;
    float angle2;
    float interpolationValue;
    effectWork->rotation += 3.0f;
    if (effectWork->rotation > 360.0f) {
        effectWork->rotation -= 360.0f;
    }
    angle = cosd(effectWork->rotation);
    angle2 = sind(effectWork->rotation);
    interpolationValue = GetStageEventValue();
    effectWork->scale.y = (90 + (-effectWork->rotation)) + 180 + 90;
    effectWork->position.x = 1.5f * (10.0f * angle) + effectWork->targetPosition.x;
    effectWork->position.z = 1.5f * (10.0f * angle2) + effectWork->targetPosition.z;
    effectWork->velocity.x = effectWork->position.x;
    effectWork->velocity.y = effectWork->position.y + 5;
    effectWork->velocity.z = effectWork->position.z;
    if (effectWork->position.y > 5.0f + interpolationValue) {
        effectWork->effectState = 1;
    }
    else {
        effectWork->effectState = 0;
    }
    if (effectWork->effectState == 1 && effectWork->position.y - 15.0f > Center.y + CZoom) {
        effectWork->animationId = 0;
        Hu3DModelAttrSet(effectModelObject->mdlId[effectWork->elapsedFrames], HU3D_ATTR_DISPOFF);
    }
    if (effectWork->scale.y > 360.0f) {
        effectWork->scale.y -= 360.0f;
    }
    else if (effectWork->scale.y < 0.0f) {
        effectWork->scale.y += 360.0f;
    }
    effectWork->scale.x = approachAngle(effectWork->scale.x, -35, 0.1f);
}

void UpdateEffectTypeSix(StageEffectWork *effectWork)
{
    float angle;
    float interpolationValue;
    float angle2;
    float angle3;
    float interpolationValue2;
    float interpolationValue3;
    float angle4;
    float interpolationValue4;
    float interpolationValue5;
    float interpolationValue6;
    float interpolationValue7;
    interpolationValue = 1;
    interpolationValue3 = GetStageEventValue();
    switch (effectWork->animationFrame) {
        case 0:
            angle2 = cosd(effectWork->rotation);
            angle3 = sind(effectWork->rotation);
            effectWork->position.x = angle2 * (80.0f - (10.0f * interpolationValue));
            effectWork->position.z = angle3 * (80.0f - (10.0f * interpolationValue)) * 0.7f;
            interpolationValue4 = interpolationValue3;
            if (effectWork->position.y > interpolationValue4) {
                effectWork->animationFrame = 1;
                effectWork->targetPosition.x = effectWork->position.x;
                effectWork->targetPosition.z = effectWork->position.z;
            }
            effectWork->effectState = 1;
            effectWork->motionFrame = 0;
            break;

        case 1:
            effectWork->motionFrame += 1.5f;
            if (effectWork->motionFrame > 180.0f) {
                effectWork->animationFrame = 3;
                Hu3DMotionSpeedSet(effectModelObject->mdlId[effectWork->elapsedFrames], 1);
            }

            angle = 180 + effectWork->rotation;
            if (angle > 360.0f) {
                angle -= 360.0f;
            }
            angle2 = cosd(angle);
            angle3 = sind(angle);
            angle4 = cosd(effectWork->motionFrame * 0.5f);
            interpolationValue2 = 180.0f;
            interpolationValue = interpolationValue2 + (angle4 * (-interpolationValue2));
            interpolationValue5 = effectWork->targetPosition.x + (angle2 * interpolationValue);
            interpolationValue7 = effectWork->targetPosition.z + (angle3 * interpolationValue);
            interpolationValue6 = interpolationValue3;
            effectWork->position.x = interpolationValue5;
            effectWork->position.y = interpolationValue6;
            effectWork->position.z = interpolationValue7;
            effectWork->velocity.x = effectWork->position.x;
            effectWork->velocity.y = effectWork->position.y + 5;
            effectWork->velocity.z = effectWork->position.z;
            if (effectWork->scale.x < 30.0f) {
                effectWork->scale.x += 4.0f;
            }
            else {
                effectWork->scale.x = 30.0f;
            }
            Hu3DMotionSpeedSet(effectModelObject->mdlId[effectWork->elapsedFrames], 2);
            effectWork->effectState = 0;
            break;

        case 3:
            Hu3DModelAttrSet(effectModelObject->mdlId[effectWork->elapsedFrames], HU3D_ATTR_DISPOFF);
            effectWork->effectState = 1;
            effectWork->animationId = 0;
            effectWork->animationFrame++;
            break;

        case 4:
            break;
    }
}

void UpdateEffectTypeSeven(StageEffectWork *effectWork)
{
    float cosineComponent3;
    float sineComponent3;
    float calculatedFloat;
    float calculatedFloat2;
    float calculatedFloat3;
    float calculatedFloat4;
    float calculatedFloat5;
    float stageHeight3;
    float calculatedFloat6;
    float calculatedFloat7;
    float calculatedFloat8;
    float calculatedFloat9;
    float calculatedFloat10;
    float calculatedFloat11;
    float scratchValues;
    float scratchValues2;

    effectWork->rotation += 2.0f;
    if (effectWork->rotation > 360.0f) {
        effectWork->rotation -= 360.0f;
    }
    switch (effectWork->animationFrame) {
        case 0:
            effectWork->effectState = 1;
            cosineComponent3 = cosd(effectWork->rotation);
            sineComponent3 = sind(effectWork->rotation);
            effectWork->position.x = 15.0f * cosineComponent3;
            effectWork->position.z = 15.0f * sineComponent3;
            effectWork->scale.y = -effectWork->rotation + 90.0f;
            stageHeight3 = GetStageEventValue();
#if VERSION_PAL
            calculatedFloat8 = -680.0f + stageHeight3;
#else
            calculatedFloat8 = -680.00006f + stageHeight3;
#endif
            if (effectWork->position.y > calculatedFloat8) {
                effectWork->animationFrame = 1;
                effectWork->effectTimer = 0;
            }
            break;

        case 1:
#if VERSION_PAL
            if (effectWork->effectTimer < 85.0f) {
                effectWork->effectTimer++;
                calculatedFloat3 = effectWork->effectTimer / 85.0f;
#else
            if (effectWork->effectTimer < 102.0f) {
                effectWork->effectTimer++;
                calculatedFloat3 = effectWork->effectTimer / 102.0f;
#endif
                calculatedFloat = 15 + (35 * calculatedFloat3);
                calculatedFloat7 = 90 * calculatedFloat3;
                cosineComponent3 = cosd(effectWork->rotation);
                sineComponent3 = sind(effectWork->rotation);
                effectWork->position.x = cosineComponent3 * calculatedFloat;
                effectWork->position.z = sineComponent3 * calculatedFloat;
                effectWork->scale.y = (-effectWork->rotation + 90.0f) - calculatedFloat7;
            }
            else {
                effectWork->animationFrame = 2;
                effectWork->effectTimer = 0;
                Hu3DMotionSpeedSet(effectModelObject->mdlId[effectWork->elapsedFrames], 2);
            }
            break;

        case 2:
            cosineComponent3 = cosd(effectWork->rotation);
            sineComponent3 = sind(effectWork->rotation);
            stageHeight3 = GetStageEventValue();
            calculatedFloat4 = 50 * cosineComponent3;
            calculatedFloat2 = stageHeight3;
            calculatedFloat5 = 50 * sineComponent3;
            effectWork->position.x = calculatedFloat4;
            effectWork->position.y = calculatedFloat2;
            effectWork->position.z = calculatedFloat5;
            if (effectWork->scale.x < 30.0f) {
                effectWork->scale.x += 4.0f;
            }
            else {
                effectWork->scale.x = 30.0f;
            }
            effectWork->scale.y = (-effectWork->rotation + 90.0f) - 90.0f;
            if (effectWork->effectTimer < REFRESH_RATE_F * 3) {
                effectWork->effectTimer++;
            }
            else {
                effectWork->animationFrame = 3;
                effectWork->effectTimer = 0;
                effectWork->targetPosition.y = calculatedFloat2;
                Hu3DMotionSpeedSet(effectModelObject->mdlId[effectWork->elapsedFrames], 1);
            }
            effectWork->velocity.x = calculatedFloat4;
            effectWork->velocity.y = 5.0f + calculatedFloat2;
            effectWork->velocity.z = calculatedFloat5;
            effectWork->effectState = 0;
            break;

        case 3:
#if VERSION_PAL
            if (effectWork->effectTimer < 125.0f) {
#else
            if (effectWork->effectTimer < 150.0f) {
#endif
                effectWork->effectTimer++;
            }
            else {
                effectWork->animationFrame = 4;
                Hu3DModelAttrSet(effectModelObject->mdlId[effectWork->elapsedFrames], HU3D_ATTR_DISPOFF);
            }
            calculatedFloat3 = 0;
            calculatedFloat = 50 + (40 * calculatedFloat3);
            cosineComponent3 = cosd(effectWork->rotation);
            sineComponent3 = sind(effectWork->rotation);
            calculatedFloat4 = cosineComponent3 * calculatedFloat;
            calculatedFloat2 = effectWork->targetPosition.y;
            calculatedFloat5 = sineComponent3 * calculatedFloat;
            scratchValues = 0;
            calculatedFloat9 = -cosd(effectWork->scale.x) * 5;
            calculatedFloat6 = -sind(effectWork->scale.x) * 5;
            calculatedFloat10 = calculatedFloat6 * sind(effectWork->scale.y);
            calculatedFloat11 = calculatedFloat9;
            scratchValues2 = calculatedFloat6 * cosd(effectWork->scale.y);
            effectWork->position.x = calculatedFloat10 + calculatedFloat4;
            effectWork->position.y = calculatedFloat11 + calculatedFloat2;
            effectWork->position.z = scratchValues2 + calculatedFloat5;
            if (effectWork->scale.y > 360.0f) {
                effectWork->scale.y -= 360.0f;
            }
            effectWork->scale.y = (-effectWork->rotation + 90.0f) - 90.0f;
            effectWork->effectState = 1;
            break;

        case 4:
            if (effectWork->effectState == 1 && effectWork->position.y - 15.0f > Center.y + CZoom) {
                effectWork->animationId = 0;
                Hu3DModelAttrSet(effectModelObject->mdlId[effectWork->elapsedFrames], HU3D_ATTR_DISPOFF);
            }
            break;
    }
    if (effectWork->scale.y > 360.0f) {
        effectWork->scale.y -= 360.0f;
    }
    else if (effectWork->scale.y < 0.0f) {
        effectWork->scale.y += 360.0f;
    }
}

void UpdateEffectTypeEight(StageEffectWork *effectWork)
{
    s32 integerValue2;
    float stageHeight2;
    float cosineComponent2;
    float sineComponent2;
    cosineComponent2 = cosd(effectWork->rotation);
    sineComponent2 = sind(effectWork->rotation);
    effectWork->position.x = 32.0f * cosineComponent2;
    effectWork->position.z = 32.0f * sineComponent2;
    stageHeight2 = GetStageEventValue();
    switch (effectWork->animationFrame) {
        case 0:
            if (effectWork->position.y > stageHeight2) {
                effectWork->animationFrame = 1;
                effectWork->effectTimer = 0;
            }
            effectWork->scale.x += 4.0f;
            effectWork->effectState = 1;
            break;

        case 1:
            effectWork->effectState = 0;
            if (effectWork->effectType) {
                effectWork->effectState = 1;
                effectWork->animationFrame = 4;
                effectWork->effectTimer = 0;
                integerValue2 = effectModelObject->mdlId[effectWork->positionX];
                Hu3DModelAttrReset(integerValue2, HU3D_ATTR_DISPOFF);
                SetEffectModelMotion(integerValue2);
                SetEffectModelDataCount(integerValue2, 1);
                SetEffectModelPosition(integerValue2, effectWork->position.x, effectWork->position.y, effectWork->position.z);
                HuAudFXPlay(7);
            }
            else {
#if VERSION_PAL
                if (effectWork->effectTimer < 60.000004f) {
#else
                if (effectWork->effectTimer < 72.0f) {
#endif
                    effectWork->effectTimer++;
                    effectWork->position.y = stageHeight2;
                }
                else {
                    effectWork->animationFrame = 3;
                }
                effectWork->scale.x += 8.0f;
                effectWork->scale.y += 8.0f;
            }
            break;

        case 3:
            effectWork->scale.x += 4.0f;
            effectWork->effectState = 1;
            break;

        case 4:
            if (effectWork->effectTimer < REFRESH_RATE_F / 2) {
                effectWork->scale.x = 90;
                effectWork->scale.y += 32.0f;
                if (effectWork->effectTimer > 5) {
                    SetEffectModelDataCount(effectModelObject->mdlId[effectWork->positionX], 0);
                }
                effectWork->motionSpeed *= 0.8f;
                effectWork->effectTimer++;
            }
            else {
                effectWork->animationId = 0;
                Hu3DModelAttrSet(effectModelObject->mdlId[effectWork->elapsedFrames], HU3D_ATTR_DISPOFF);
            }
            break;
    }
    if (effectWork->scale.x > 360.0f) {
        effectWork->scale.x -= 360.0f;
    }
    if (effectWork->scale.y > 360.0f) {
        effectWork->scale.y -= 360.0f;
    }
    effectWork->velocity.x = effectWork->position.x;
    effectWork->velocity.y = effectWork->position.y;
    effectWork->velocity.z = effectWork->position.z;
}

void UpdateEffectTypeNine(StageEffectWork *effectWork)
{
    s32 integerValue;
    float stageHeight;
    float cosineComponent;
    float sineComponent;

#if VERSION_PAL
    effectWork->motionFrame += 7.2f;
#else
    effectWork->motionFrame += 6.0000005f;
#endif
    if (effectWork->motionFrame >= 360.0f) {
        effectWork->motionFrame -= 360.0f;
    }
    effectWork->scale.z = 30 * sind(effectWork->motionFrame);
    cosineComponent = cosd(effectWork->rotation);
    sineComponent = sind(effectWork->rotation);
    effectWork->position.x = 32.0f * cosineComponent;
    effectWork->position.z = 32.0f * sineComponent;
    stageHeight = GetStageEventValue();
    switch (effectWork->animationFrame) {
        case 0:
            if (effectWork->position.y > stageHeight) {
                effectWork->animationFrame = 1;
                effectWork->effectTimer = 0;
            }
            effectWork->scale.x = -45;
            effectWork->scale.y = 0;
            effectWork->effectState = 1;
            break;

        case 1:
            effectWork->effectState = 0;
            if (effectWork->effectType) {
                effectWork->effectState = 1;
                effectWork->animationFrame = 4;
                effectWork->effectTimer = 0;
                integerValue = effectModelObject->mdlId[effectWork->positionX];
                Hu3DModelAttrReset(integerValue, HU3D_ATTR_DISPOFF);
                SetEffectModelMotion(integerValue);
                SetEffectModelDataCount(integerValue, 1);
                SetEffectModelPosition(integerValue, effectWork->position.x, effectWork->position.y, effectWork->position.z);
                HuAudFXPlay(19);
            }
            else {
#if VERSION_PAL
                if (effectWork->effectTimer < 60.000004f) {
#else
                if (effectWork->effectTimer < 72.0f) {
#endif
                    effectWork->effectTimer++;
                    effectWork->position.y = stageHeight;
                }
                else {
                    effectWork->animationFrame = 3;
                }
            }
            break;

        case 3:
            effectWork->effectState = 1;
            break;

        case 4:
            if (effectWork->effectTimer < REFRESH_RATE_F / 2) {
                effectWork->scale.x = 90;
                effectWork->scale.y += 32.0f;
                if (effectWork->effectTimer > 5) {
                    SetEffectModelDataCount(effectModelObject->mdlId[effectWork->positionX], 0);
                }
                effectWork->motionSpeed *= 0.8f;
                effectWork->effectTimer++;
            }
            else {
                effectWork->animationId = 0;
                Hu3DModelAttrSet(effectModelObject->mdlId[effectWork->elapsedFrames], HU3D_ATTR_DISPOFF);
            }
            break;
    }
    if (effectWork->scale.x > 360.0f) {
        effectWork->scale.x -= 360.0f;
    }
    if (effectWork->scale.y > 360.0f) {
        effectWork->scale.y -= 360.0f;
    }
    effectWork->velocity.x = effectWork->position.x;
    effectWork->velocity.y = effectWork->position.y;
    effectWork->velocity.z = effectWork->position.z;
}

void SetEffectModelDataCount(s16 modelId, s16 dataCount)
{
    HU3DMODEL *model;
    HU3DPARTICLE *integerValue;
    model = &Hu3DData[modelId];
    integerValue = model->hookData;
    integerValue->dataCnt = dataCount;
}

void SetEffectModelPosition(s16 modelId, float positionX, float positionY, float positionZ)
{
    HU3DPARTICLE *particleWork;
    HU3DMODEL *model;
    model = &Hu3DData[modelId];
    particleWork = model->hookData;
    particleWork->pos.x = positionX;
    particleWork->pos.y = positionY;
    particleWork->pos.z = positionZ;
}

void UpdateEffectModelParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float angle;
    float angle2;
    HU3DPARTICLEDATA *particleData;
    u32 integerValue;
    s32 particleIndex;
    s16 updateCounter;
    GXColor *gxcolorPointer;
    if (particle->count == 0) {
        angle = 90;
        particle->emitCnt = 0;
        particle->unk10.y = 3;
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->scale = 0;
            particleData->color.a = 0;
        }
    }
    if (particle->dataCnt) {
        particle->emitCnt--;
        if (particle->emitCnt <= 0.0f) {
            particle->emitCnt = 2;
            updateCounter = 4;
            particleData = particle->data;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                if (particleData->scale != 0) {
                    continue;
                }
                particleData->pos.x = particle->pos.x;
                particleData->pos.y = particle->pos.y;
                particleData->pos.z = particle->pos.z;
                angle2 = frandmod(360);
                integerValue = frandmod(12);
                integerValue += 4.0f;
                particleData->pos.x = particleData->pos.x + (integerValue * sind(angle2));
                particleData->pos.z = particleData->pos.z + (integerValue * cosd(angle2));
                particleData->speedDecay = 0;
                particleData->colorIdx = particle->unk10.y;
                integerValue = 1;
                if (integerValue == 1) {
                    particleData->accel.x = 0.97643f;
                }
                else {
                    particleData->accel.x = 0.96666664f;
                }
                particleData->time = 60;
                particleData->scale = 8;
                integerValue = frand() & 0x1;
                gxcolorPointer = &effectModelColors[integerValue];
                particleData->color.r = gxcolorPointer->r;
                particleData->color.g = gxcolorPointer->g;
                particleData->color.b = gxcolorPointer->b;
                particleData->color.a = 0;
                particleData->parManId = integerValue;
                updateCounter--;
                if (updateCounter == 0) {
                    break;
                }
            }
        }
    }
    updateCounter = 0;
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->scale == 0) {
            continue;
        }
        updateCounter++;
        particleData->pos.y -= 4.0f;
        angle = 1.0 + cosd(particleData->speedDecay);
        angle *= 0.5f;
        particleData->color.a = angle * 255;
        particleData->speedDecay += particleData->colorIdx;
        particleData->zRot = particleData->speedDecay;
        particleData->scale *= particleData->accel.x;
        particleData->parManId ^= 0x3;
        if (particleData->parManId & 0x1) {
            particleData->color.b = 0;
            particleData->color.g = 200;
        }
        else {
            particleData->color.b = 200;
            particleData->color.g = 200;
        }
        if (particleData->parManId & 0x2) {
            integerValue = particleData->color.a + 64;
            if (integerValue > 255) {
                integerValue -= 128;
            }
            particleData->color.a = integerValue;
        }
        particleData->time--;
        if (particleData->time <= 0) {
            particleData->scale = 0;
        }
    }
    DCStoreRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
    if (particle->dataCnt == 0 && updateCounter == 0) {
        model->attr |= 0x1;
    }
}

void SetEffectModelMotion(s16 modelId)
{
    HU3DPARTICLE *particleWork;
    HU3DPARTICLEDATA *particleData;
    s32 modelIndex;
    HU3DMODEL *model = &Hu3DData[modelId];
    particleWork = model->hookData;
    particleWork->count = 0;
    particleData = particleWork->data;
    for (modelIndex = 0; modelIndex < particleWork->maxCnt; modelIndex++, particleData++) {
        particleData->scale = 0;
    }
}

void UpdateEffectModelManager(OMOBJ *object);
void UpdateEffectManagerParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

void InitializeEffectModelManager(OMOBJ *object)
{
    s16 createdObjectId;
    s32 modelIndex;
    omSetStatBit(object, 0x100);
    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
        createdObjectId = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M408, 0x19)), 9);
        object->mdlId[modelIndex] = createdObjectId;
        Hu3DModelAttrSet(createdObjectId, HU3D_ATTR_DISPOFF);
        Hu3DModelLayerSet(createdObjectId, 7);
        Hu3DParticleColSet(createdObjectId, 255, 255, 255);
        Hu3DParticleHookSet(createdObjectId, UpdateEffectManagerParticle);
    }
    object->work[0] = -1;
    object->objFunc = UpdateEffectModelManager;
}

void UpdateEffectModelManager(OMOBJ *object)
{
    s32 integerValue;
    s16 integerValue2;
    HU3DMODEL *model;
    HU3DPARTICLE *particleWork;
    integerValue = object->work[0];
    if (integerValue <= -1) {
        return;
    }
    object->work[0] = -1;
    integerValue2 = object->mdlId[integerValue];
    model = &Hu3DData[integerValue2];
    particleWork = model->hookData;
    Hu3DModelAttrReset(integerValue2, HU3D_ATTR_DISPOFF);
    Hu3DModelPosSet(integerValue2, characterTargetPositions[integerValue].x, characterTargetPositions[integerValue].y, characterTargetPositions[integerValue].z);
    particleWork->count = 0;
}

void UpdateEffectManagerParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float angle;
    float angle2;
    float angle3;
    float interpolationValue;
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;
    u32 integerValue;
    s16 frameCounter;
    s16 frameCounter2;
    if (particle->count == 0) {
        angle = 72;
        particle->dataCnt = 1;
        particle->emitCnt = 0;
        particle->unk10.y = 22.5f;
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->scale = 0;
            particleData->color.a = 0;
        }
    }
    if (particle->dataCnt) {
        particle->dataCnt = 0;
        particle->emitCnt--;
        if (particle->emitCnt <= 0.0f) {
            particle->emitCnt = 30;
            particleData = particle->data;
            frameCounter2 = -1;
            frameCounter = 0;
            for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                if (particleData->scale != 0) {
                    continue;
                }
                if (frameCounter <= 0) {
                    frameCounter2++;
                    if (frameCounter2 >= 3) {
                        break;
                    }
                    angle2 = effectMotionOffsets[frameCounter2];
                    interpolationValue = effectMotionFrames[frameCounter2];
                    angle3 = effectMotionAngles[frameCounter2];
                    frameCounter = effectModelOrder[frameCounter2];
                }
                frameCounter--;
                particleData->pos.x = particleData->pos.y = particleData->pos.z = 0;
                if (90.0f == angle3) {
                    particleData->vel.z = 5;
                    particleData->vel.x = particleData->vel.y = 0;
                }
                else {
                    angle = 5 * cosd(angle3);
                    particleData->vel.z = 5.0 * sind(angle3);
                    particleData->vel.x = angle * sind(angle2);
                    particleData->vel.y = angle * cosd(angle2);
                }
                particleData->speedDecay = 0;
                particleData->colorIdx = particle->unk10.y;
                integerValue = 1;
                particleData->time = 8;
                particleData->scale = 10;
                particleData->accel.x = 1.125f;
                integerValue = frand() & 0x1;
                particleData->parManId = integerValue;
                particleData->color.r = 255;
                particleData->color.g = 255;
                particleData->color.b = 255;
                particleData->color.a = 0;
                if (integerValue) {
                    particleData->color.b = 0;
                    particleData->color.g = 0;
                }
                angle2 += interpolationValue;
            }
        }
    }
    frameCounter = 0;
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->scale == 0) {
            continue;
        }
        frameCounter++;
        VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
        angle = 1.0 + cosd(particleData->speedDecay);
        angle *= 0.45f;
        particleData->color.a = angle * 255;
        particleData->speedDecay += particleData->colorIdx;
        particleData->scale -= particleData->accel.x;
        particleData->parManId ^= 0x3;
        if (particleData->parManId & 0x1) {
            particleData->color.b = 0;
            particleData->color.g = 100;
        }
        else {
            particleData->color.b = 0;
            particleData->color.g = 200;
        }
        if (particleData->parManId & 0x2) {
            integerValue = particleData->color.a + 64;
            if (integerValue > 255) {
                integerValue -= 128;
            }
            particleData->color.a = integerValue;
        }
        particleData->time--;
        if (particleData->time <= 0) {
            particleData->scale = 0;
        }
    }
    DCStoreRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
    if (particle->dataCnt == 0 && updateCounter == 0) {
        model->attr |= 0x1;
    }
}

s32 FindNearestEffectPosition(Vec *referencePosition, Vec *nearestPosition)
{
    Vec positionVector;
    s32 animationIndex;
    float interpolationValue;
    float interpolationValue2;
    interpolationValue2 = 1000;
    if (!effectAnimationSeed) {
        return 0;
    }
    for (animationIndex = 0; animationIndex < effectAnimationSeed; animationIndex++) {
        VECSubtract(&effectSpawnPositions[animationIndex], referencePosition, &positionVector);
        interpolationValue = VECMag(&positionVector);
        if (interpolationValue < interpolationValue2) {
            interpolationValue2 = interpolationValue;
            *nearestPosition = effectSpawnPositions[animationIndex];
        }
    }
    return 1;
}

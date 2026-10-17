#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXVert.h"
#include "dolphin/mtx/GeoTypes.h"
#include "ext_math.h"
#include "game/animdata.h"
#include "game/audio.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/sprite.h"
#include "string.h"

#include "REL/m425Dll.h"

extern u32 GlobalCounter;
extern HU3DLIGHT Hu3DLocalLight[0x20];

typedef struct M425ThwompModelWork {
    s16 animationIndex;
    s16 animationstateAt02;
    s16 frameStart;
    s16 referenceVertexIndex;
    float animationTimeRemaining;
    float animationTimeTotal;
    float frameEnd;
    float motionTimer;
    float motionIntensity;
    float frameLength;
    float movementSpeed;
    float collisionState;
    s32 animationFrameCounter;
    s32 animationFrame;
    float animationCycleProgress;
    s32 frameCount;
    s32 motionSlotCount;
    s32 activeMotionIds[5];
    s32 modelCount;
    float activeMotionTimers[6];
    s32 animationIds[6];
    s32 motionStates[6];
    float motionTimers[6];
    s32 animationTextureIds[6];
    Vec worldPosition;
    float rotationAngle;
    float modelScale;
    float actionTimer;
    float lightElevationAngle;
    float effectTimer;
    float animationBlend;
    s32 activeVertexIndices[3];
    float initialRotation;
    float movementAngle;
    char reservedAnimationData[0x8];
    s16 playerState;
    s16 actionIndex;
    s16 vertexCount;
    s16 vertexIndexCounts[0x19];
    s16 vertexIndexOffsets[0x1A];
    s16 *positionBuffer;
    Vec *positionStart;
    Vec *positionTarget;
    Vec2f *textureCoordinates;
    Vec *positionWork;
    Vec *rotationWork;
    s16 (*displayListIndices)[3];
    Vec *positionOutput;
    Vec *rotationOutput;
    s32 workState;
    u8 *stateBuffer;
    s16 *vertexBuffers[0x19];
    s16 vertexBufferCounts[0x19];
    GXColor effectColor;
    s32 surfaceCount;
    u32 displayListSizes[0x19];
    void *displayLists[0x19];
} M425ThwompModelWork; /* size = 0x30C */

typedef struct M425ThwompAnimationWork {
    char reservedAnimationHeader[4];
    Vec frameStart;
    Vec frameEnd;
    float frameLength;
    char reservedAnimationData[8];
    float lifetimeRemaining;
    float animationFrame;
    Vec worldPosition;
    GXColor motionIds;
    s32 animationFrameIndex;
} M425ThwompAnimationWork; /* size = 0x44 */

typedef void (*ThwompParticleUpdateHook)(HU3DMODEL *model, struct M425ThwompParticleWork *particle, Mtx matrix);

typedef struct M425ThwompParticleWork {
    s16 animationIndex;
    s16 animationstateAt02;
    char reservedParticleData[0x1C];
    u8 movementSpeed;
    u8 thwompFlags;
    char thwompPadding[2];
    s16 collisionState;
    s32 elapsedFrameCount;
    s32 animationFrame;
    u32 frameLimit;
    u32 frameCount;
    ANIMDATA *animationTimer;
    M425ThwompAnimationWork *motionIds;
    Vec *animationData;
    Vec2f *position;
    void *rotation;
    ThwompParticleUpdateHook updateHook;
} M425ThwompParticleWork; /* size = 0x50 */

void InitializeThwompScene(OMOBJ *object);
s32 CalculateThwompMotion(float modelWidth, float modelHeight, s16 verticalSegmentCount, s16 horizontalSegmentCount);
void InitializeThwompModels(s32 modelIndex, float initialRotation);
void SetThwompAnimationState(s32 modelIndex);
void DrawThwompShadow(HU3DMODEL *model, Mtx matrix);
void InitializeThwompModelData(M425ThwompModelWork *thwompModel, HU3DMODEL *model, s32 horizontalSegmentCount, s32 verticalSegmentCount, s32 vertexBufferIndex);
void UpdateThwompModelVertices(M425ThwompModelWork *thwompModel, HU3DMODEL *model);
void BuildThwompModelGeometry(M425ThwompModelWork *thwompModel, HU3DMODEL *model);
void DrawThwompModel(HU3DMODEL *model, Mtx matrix);
void InitializeThwompSprite(void);
void CreateThwompEffect(void);
void CleanupThwompSprite(void);
s32 UpdateThwompEffect(Vec *position, Vec *velocity, float effectLifetime, float effectScale);
s16 ConfigureThwompLight(HU3DMODEL *model, Mtx matrix, Mtx cameraMatrix, float specularStrength);
void InitializeLightParameters(HU3DMODEL *model, HU3DLIGHT *light, s16 lightMask, Mtx modelMatrix, Mtx cameraMatrix, float specularStrength);
s16 LoadThwompAnimation(ANIMDATA *animation, s16 particleCount);
void DrawThwompEffect(HU3DMODEL *model, Mtx matrix);

Vec thwompModelOffsets[2] = {
    { -300.0f, 1.0f, 0.0f },
    { 300.0f, 1.0f, 0.0f },
};

HUPROCESS *thwompProcess;
OMOBJ *thwompObject;
s32 thwompAnimationFrame;
u8 *thwompDataBuffer;
ANIMDATA *thwompIdleAnimation;
ANIMDATA *thwompHitAnimation;
ANIMDATA *thwompEffectAnimation;
// M425ThwompParticleWork
s32 thwompSpriteId;
u8 thwompAlpha;
s32 thwompState;
GXColor thwompEffectColor;
GXLightObj thwompLight;
Vec thwompLightPosition;
Vec thwompLightDirection;

OMOBJ *CreateThwompObjectManager(HUPROCESS *process, s32 objectPriority)
{
    OMOBJ *object;

    thwompProcess = process;
    object = omAddObjEx(process, objectPriority, 6, 0, -1, InitializeThwompScene);
    thwompObject = object;
    object->stat |= 0x100;
    thwompAnimationFrame = 0;
    thwompAlpha = 0x40;
    thwompState = 0;
    object->mdlId[0] = CalculateThwompMotion(350.0f, 350.0f, 0x10, 0x10);
    object->mdlId[1] = CalculateThwompMotion(350.0f, 350.0f, 0x10, 0x10);
    CreateThwompModels(0, &thwompModelOffsets[0]);
    CreateThwompModels(1, &thwompModelOffsets[1]);
    InitializeThwompModels(0, 30.0f);
    InitializeThwompModels(1, -30.0f);
    thwompDataBuffer = HuDataReadNum(0x38000A, HU_MEMNUM_OVL);
    thwompIdleAnimation = HuSprAnimRead(HuDataReadNum(0x38000B, HU_MEMNUM_OVL));
    thwompHitAnimation = HuSprAnimRead(HuDataReadNum(0x38000C, HU_MEMNUM_OVL));
    InitializeThwompSprite();
    return thwompObject;
}

void InitializeThwompScene(OMOBJ *object)
{
    SetThwompAnimationState(0);
    SetThwompAnimationState(1);
    CreateThwompEffect();
}

void CleanupThwompScene(void)
{
    OMOBJ *object = thwompObject;
    if (object) {
        HuMemDirectFree(thwompDataBuffer);
        thwompDataBuffer = NULL;
        HuSprAnimKill(thwompIdleAnimation);
        thwompIdleAnimation = NULL;
        HuSprAnimKill(thwompHitAnimation);
        thwompHitAnimation = NULL;
        CleanupThwompSprite();
        thwompObject = NULL;
        object->objFunc = NULL;
        omDelObjEx(thwompProcess, object);
    }
}

void CreateThwompModels(s32 modelIndex, Vec *targetPosition)
{
    M425ThwompModelWork *thwompModel;
    s32 vibrationIndex;
    HU3DMODEL *model;

    if (thwompObject) {
        model = &Hu3DData[thwompObject->mdlId[modelIndex]];
        thwompModel = model->hookData;
        thwompModel->worldPosition.x = targetPosition->x;
        thwompModel->worldPosition.y = targetPosition->y;
        thwompModel->worldPosition.z = targetPosition->z;
        Hu3DModelPosSet(thwompObject->mdlId[modelIndex], targetPosition->x, targetPosition->y, targetPosition->z);
        if ((thwompState == 0) && (modelIndex == 0) && (targetPosition->y <= 0.0f)) {
            for (vibrationIndex = 0; vibrationIndex < 4; vibrationIndex++) {
                omVibrate(0, 0xC, 6, 6);
            }
            HuAudFXPlay(0x62E);
            thwompState = 1;
        }
    }
}

void InitializeThwompModels(s32 modelIndex, float initialRotation)
{
    HU3DMODEL *model;
    M425ThwompModelWork *thwompModel;

    if (thwompObject) {
        model = &Hu3DData[thwompObject->mdlId[modelIndex]];
        thwompModel = model->hookData;
        thwompModel->rotationAngle = initialRotation;
        Hu3DModelRotSet(thwompObject->mdlId[modelIndex], 0.0f, initialRotation, 0.0f);
    }
}

s32 CalculateThwompMotion(float modelWidth, float modelHeight, s16 verticalSegmentCount, s16 horizontalSegmentCount)
{
    float vectorMagnitude;
    float baseY;
    float baseZ;
    float textureVStart;
    float modelWidthExtent;
    float modelHeightExtent;
    void *displayListBuffer;
    float textureU;
    float positionX;
    float textureV;
    float positionY;
    float angleDegrees;
    float positionZ;
    float rowStartX;
    float positionStepX;
    float secondaryAngleDegrees;
    float textureUStep;
    float textureVStep;
    float textureUStart;
    float positionStepY;

    M425ThwompModelWork *thwompModel;
    s32 lightId;
    Vec *vector;
    Vec2f *textureCoordinate;
    s32 vertexCount;
    Vec *vector2;
    s16 *positionBufferCursor;
    HU3DMODEL *model;
    s32 vertexIndex;
    s16 *vertexIndexBuffer;
    s32 modelId;
    s16 hookId;
    void *displayListAllocation;

    modelId = Hu3DHookFuncCreate(DrawThwompModel);
    model = &Hu3DData[modelId];
    Hu3DModelLayerSet(modelId, 5);
    thwompModel = HuMemDirectMallocNum(HEAP_MODEL, sizeof(M425ThwompModelWork), model->mallocNo);
    model->hookData = thwompModel;
    memset(thwompModel, 0, sizeof(M425ThwompModelWork));
    thwompModel->animationIndex = thwompAnimationFrame;
    thwompAnimationFrame++;
    thwompModel->animationstateAt02 = 0;
    thwompModel->frameStart = Hu3DModelCreateFile(0x38000D);
    Hu3DModelLayerSet(thwompModel->frameStart, 6);
    thwompModel->referenceVertexIndex = 0;
    thwompModel->animationTimeRemaining = 1000.0f;
    thwompModel->animationTimeTotal = 1000.0f;
    thwompModel->frameEnd = 1000.0f;
    thwompModel->motionTimer = 0.0f;
    thwompModel->motionIntensity = 0.0f;
    thwompModel->frameLength = 0.0f;
    thwompModel->movementSpeed = 0.0f;
    thwompModel->collisionState = modelHeight;
    thwompModel->initialRotation = modelWidth;
    thwompModel->movementAngle = modelHeight;
    thwompModel->animationFrameCounter = 0;
    thwompModel->animationFrame = 0;
    thwompModel->animationCycleProgress = 0.0f;
    for (lightId = 0; lightId < 6; lightId++) {
        thwompModel->activeMotionIds[lightId] = -1;
        thwompModel->activeMotionTimers[lightId] = 0.0f;
        thwompModel->motionStates[lightId] = -1;
        thwompModel->motionTimers[lightId] = 0.0f;
    }
    thwompModel->worldPosition.x = thwompModel->worldPosition.y = thwompModel->worldPosition.z = 0.0f;
    thwompModel->rotationAngle = 0.0f;
    thwompModel->modelScale = 1.0f;
    thwompModel->activeVertexIndices[0] = -1;
    thwompModel->activeVertexIndices[1] = -1;
    thwompModel->activeVertexIndices[2] = -1;
    thwompObject->animationIndex[thwompModel->animationIndex + 2] = thwompModel->frameStart;
    if (thwompModel->animationIndex == 0) {
        thwompModel->actionTimer = secondaryAngleDegrees = 302.0f;
        thwompModel->lightElevationAngle = angleDegrees = 193.0f;
    }
    else {
        thwompModel->actionTimer = secondaryAngleDegrees = 302.0f;
        thwompModel->lightElevationAngle = angleDegrees = 150.0f;
    }
    thwompModel->effectTimer = thwompModel->animationBlend = 0.0f;
    positionX = sind(angleDegrees) * cosd(secondaryAngleDegrees);
    positionZ = cosd(angleDegrees) * cosd(secondaryAngleDegrees);
    positionY = sind(secondaryAngleDegrees);
    vectorMagnitude = sqrtf((positionZ * positionZ) + ((positionX * positionX) + (positionY * positionY)));
    if (vectorMagnitude != 0.0f) {
        vectorMagnitude = 1.0f / vectorMagnitude;
    }
    else {
        vectorMagnitude = 0.0f;
    }
    positionX *= vectorMagnitude;
    positionY *= vectorMagnitude;
    positionZ *= vectorMagnitude;
    lightId = Hu3DLLightCreate(modelId, 1000.0f * positionX, 1000.0f * positionY, 1000.0f * positionZ, positionX, positionY, positionZ, 0xFF, 0xFF, 0xFF);
    Hu3DLLightInfinitytSet(modelId, lightId);
    Hu3DLLightStaticSet(modelId, lightId, 1);
    verticalSegmentCount++;
    horizontalSegmentCount++;
    modelWidthExtent = modelWidth - 50.0f;
    modelHeightExtent = modelHeight - 50.0f;
    thwompModel->vertexIndexCounts[0] = horizontalSegmentCount * horizontalSegmentCount;
    thwompModel->vertexIndexOffsets[0] = 0;
    thwompModel->vertexIndexCounts[1] = verticalSegmentCount * horizontalSegmentCount;
    thwompModel->vertexIndexOffsets[1] = thwompModel->vertexIndexOffsets[0] + thwompModel->vertexIndexCounts[0];
    if (thwompModel->animationIndex != 0) {
        thwompModel->vertexIndexCounts[2] = 0;
    }
    else {
        thwompModel->vertexIndexCounts[2] = verticalSegmentCount * horizontalSegmentCount;
    }
    thwompModel->vertexIndexOffsets[2] = thwompModel->vertexIndexOffsets[1] + thwompModel->vertexIndexCounts[1];
    if (thwompModel->animationIndex == 0) {
        thwompModel->vertexIndexCounts[3] = 0;
    }
    else {
        thwompModel->vertexIndexCounts[3] = verticalSegmentCount * horizontalSegmentCount;
    }
    thwompModel->vertexIndexOffsets[3] = thwompModel->vertexIndexOffsets[2] + thwompModel->vertexIndexCounts[2];
    thwompModel->vertexIndexCounts[4] = horizontalSegmentCount * 5;
    thwompModel->vertexIndexOffsets[4] = thwompModel->vertexIndexOffsets[3] + thwompModel->vertexIndexCounts[3];
    thwompModel->vertexIndexCounts[5] = horizontalSegmentCount * 5;
    thwompModel->vertexIndexOffsets[5] = thwompModel->vertexIndexOffsets[4] + thwompModel->vertexIndexCounts[4];
    thwompModel->vertexIndexCounts[6] = verticalSegmentCount * 5;
    thwompModel->vertexIndexOffsets[6] = thwompModel->vertexIndexOffsets[5] + thwompModel->vertexIndexCounts[5];
    thwompModel->vertexIndexCounts[7] = verticalSegmentCount * 5;
    thwompModel->vertexIndexOffsets[7] = thwompModel->vertexIndexOffsets[6] + thwompModel->vertexIndexCounts[6];
    thwompModel->vertexIndexCounts[8] = horizontalSegmentCount * 5;
    thwompModel->vertexIndexOffsets[8] = thwompModel->vertexIndexOffsets[7] + thwompModel->vertexIndexCounts[7];
    thwompModel->vertexIndexCounts[9] = horizontalSegmentCount * 5;
    thwompModel->vertexIndexOffsets[9] = thwompModel->vertexIndexOffsets[8] + thwompModel->vertexIndexCounts[8];
    thwompModel->vertexIndexCounts[10] = horizontalSegmentCount * 5;
    thwompModel->vertexIndexOffsets[10] = thwompModel->vertexIndexOffsets[9] + thwompModel->vertexIndexCounts[9];
    thwompModel->vertexIndexCounts[11] = horizontalSegmentCount * 5;
    thwompModel->vertexIndexOffsets[11] = thwompModel->vertexIndexOffsets[10] + thwompModel->vertexIndexCounts[10];
    thwompModel->vertexIndexCounts[12] = horizontalSegmentCount * 5;
    thwompModel->vertexIndexOffsets[12] = thwompModel->vertexIndexOffsets[11] + thwompModel->vertexIndexCounts[11];
    thwompModel->vertexIndexCounts[13] = horizontalSegmentCount * 5;
    thwompModel->vertexIndexOffsets[13] = thwompModel->vertexIndexOffsets[12] + thwompModel->vertexIndexCounts[12];
    if (thwompModel->animationIndex == 0) {
        thwompModel->vertexIndexCounts[14] = 0;
    }
    else {
        thwompModel->vertexIndexCounts[14] = verticalSegmentCount * 5;
    }
    thwompModel->vertexIndexOffsets[14] = thwompModel->vertexIndexOffsets[13] + thwompModel->vertexIndexCounts[13];
    if (thwompModel->animationIndex != 0) {
        thwompModel->vertexIndexCounts[15] = 0;
    }
    else {
        thwompModel->vertexIndexCounts[15] = verticalSegmentCount * 5;
    }
    thwompModel->vertexIndexOffsets[15] = thwompModel->vertexIndexOffsets[14] + thwompModel->vertexIndexCounts[14];
    thwompModel->vertexIndexCounts[16] = 0x19;
    thwompModel->vertexIndexOffsets[16] = thwompModel->vertexIndexOffsets[15] + thwompModel->vertexIndexCounts[15];
    thwompModel->vertexIndexCounts[17] = 0x19;
    thwompModel->vertexIndexOffsets[17] = thwompModel->vertexIndexOffsets[16] + thwompModel->vertexIndexCounts[16];
    thwompModel->vertexIndexCounts[18] = 0x19;
    thwompModel->vertexIndexOffsets[18] = thwompModel->vertexIndexOffsets[17] + thwompModel->vertexIndexCounts[17];
    thwompModel->vertexIndexCounts[19] = 0x19;
    thwompModel->vertexIndexOffsets[19] = thwompModel->vertexIndexOffsets[18] + thwompModel->vertexIndexCounts[18];
    thwompModel->vertexIndexCounts[20] = 0x19;
    thwompModel->vertexIndexOffsets[20] = thwompModel->vertexIndexOffsets[19] + thwompModel->vertexIndexCounts[19];
    thwompModel->vertexIndexCounts[21] = 0x19;
    thwompModel->vertexIndexOffsets[21] = thwompModel->vertexIndexOffsets[20] + thwompModel->vertexIndexCounts[20];
    thwompModel->vertexIndexCounts[22] = 0x19;
    thwompModel->vertexIndexOffsets[22] = thwompModel->vertexIndexOffsets[21] + thwompModel->vertexIndexCounts[21];
    thwompModel->vertexIndexCounts[23] = 0x19;
    thwompModel->vertexIndexOffsets[23] = thwompModel->vertexIndexOffsets[22] + thwompModel->vertexIndexCounts[22];
    thwompModel->vertexIndexCounts[24] = horizontalSegmentCount * horizontalSegmentCount;
    thwompModel->vertexIndexOffsets[24] = thwompModel->vertexIndexOffsets[23] + thwompModel->vertexIndexCounts[23];
    if (thwompModel->animationIndex == 0) {
        thwompModel->referenceVertexIndex = thwompModel->vertexIndexOffsets[17] + (thwompModel->vertexIndexCounts[17] / 2) + 1;
    }
    else {
        thwompModel->referenceVertexIndex = thwompModel->vertexIndexOffsets[16] + (thwompModel->vertexIndexCounts[16] / 2) - 1;
    }
    thwompModel->playerState = verticalSegmentCount;
    thwompModel->actionIndex = horizontalSegmentCount;
    thwompModel->vertexCount = thwompModel->vertexIndexOffsets[24] + thwompModel->vertexIndexCounts[24];
    thwompModel->surfaceCount = 0x19;
    thwompModel->positionBuffer = HuMemDirectMallocNum(HEAP_MODEL, thwompModel->vertexCount * 2, model->mallocNo);
    memset(thwompModel->positionBuffer, 0, thwompModel->vertexCount * 2);
    thwompModel->positionStart = HuMemDirectMallocNum(HEAP_MODEL, thwompModel->vertexCount * 0xC, model->mallocNo);
    memset(thwompModel->positionStart, 0, thwompModel->vertexCount * 0xC);
    thwompModel->positionTarget = HuMemDirectMallocNum(HEAP_MODEL, thwompModel->vertexCount * 0xC, model->mallocNo);
    memset(thwompModel->positionTarget, 0, thwompModel->vertexCount * 0xC);
    thwompModel->textureCoordinates = HuMemDirectMallocNum(HEAP_MODEL, thwompModel->vertexCount * 8, model->mallocNo);
    memset(thwompModel->textureCoordinates, 0, thwompModel->vertexCount * 8);
    thwompModel->positionWork = HuMemDirectMallocNum(HEAP_MODEL, thwompModel->vertexCount * 0xC, model->mallocNo);
    memset(thwompModel->positionWork, 0, thwompModel->vertexCount * 0xC);
    thwompModel->rotationWork = HuMemDirectMallocNum(HEAP_MODEL, thwompModel->vertexCount * 0xC, model->mallocNo);
    memset(thwompModel->rotationWork, 0, thwompModel->vertexCount * 0xC);
    thwompModel->displayListIndices = HuMemDirectMallocNum(HEAP_MODEL, thwompModel->vertexCount * 6, model->mallocNo);
    memset(thwompModel->displayListIndices, 0, thwompModel->vertexCount * 6);
    thwompModel->positionOutput = HuMemDirectMallocNum(HEAP_MODEL, thwompModel->vertexCount * 0xC, model->mallocNo);
    memset(thwompModel->positionOutput, 0, thwompModel->vertexCount * 0xC);
    thwompModel->rotationOutput = HuMemDirectMallocNum(HEAP_MODEL, thwompModel->vertexIndexCounts[0] * 0xC, model->mallocNo);
    memset(thwompModel->rotationOutput, 0, thwompModel->vertexIndexCounts[0] * 0xC);
    thwompModel->stateBuffer = HuMemDirectMallocNum(HEAP_MODEL, thwompModel->vertexCount, model->mallocNo);
    memset(thwompModel->stateBuffer, 0, thwompModel->vertexCount);
    for (lightId = 0; lightId < thwompModel->vertexCount; lightId++) {
        if (((thwompModel->vertexIndexCounts[0] > 0) && (lightId >= thwompModel->vertexIndexOffsets[0]) && (lightId < (thwompModel->vertexIndexOffsets[0] + thwompModel->vertexIndexCounts[0])))
            || ((thwompModel->vertexIndexCounts[4] > 0) && (lightId >= thwompModel->vertexIndexOffsets[4]) && (lightId < (thwompModel->vertexIndexOffsets[4] + thwompModel->vertexIndexCounts[4])))
            || ((thwompModel->vertexIndexCounts[8] > 0) && (lightId >= thwompModel->vertexIndexOffsets[8]) && (lightId < (thwompModel->vertexIndexOffsets[8] + thwompModel->vertexIndexCounts[8])))
            || ((thwompModel->vertexIndexCounts[10] > 0) && (lightId >= thwompModel->vertexIndexOffsets[10]) && (lightId < (thwompModel->vertexIndexOffsets[10] + thwompModel->vertexIndexCounts[10])))
            || ((thwompModel->vertexIndexCounts[12] > 0) && (lightId >= thwompModel->vertexIndexOffsets[12]) && (lightId < (thwompModel->vertexIndexOffsets[12] + thwompModel->vertexIndexCounts[12])))
            || ((thwompModel->vertexIndexCounts[16] > 0) && (lightId >= thwompModel->vertexIndexOffsets[16]) && (lightId < (thwompModel->vertexIndexOffsets[16] + thwompModel->vertexIndexCounts[16])))
            || ((thwompModel->vertexIndexCounts[17] > 0) && (lightId >= thwompModel->vertexIndexOffsets[17]) && (lightId < (thwompModel->vertexIndexOffsets[17] + thwompModel->vertexIndexCounts[17])))
            || ((thwompModel->vertexIndexCounts[20] > 0) && (lightId >= thwompModel->vertexIndexOffsets[20]) && (lightId < (thwompModel->vertexIndexOffsets[20] + thwompModel->vertexIndexCounts[20])))
            || ((thwompModel->vertexIndexCounts[21] > 0) && (lightId >= thwompModel->vertexIndexOffsets[21]) && (lightId < (thwompModel->vertexIndexOffsets[21] + thwompModel->vertexIndexCounts[21])))) {
            thwompModel->stateBuffer[lightId] = 1;
        }
        else {
            thwompModel->stateBuffer[lightId] = 0;
        }
    }
    thwompModel->effectColor.r = 0;
    thwompModel->effectColor.g = 0;
    thwompModel->effectColor.b = 0xFF;
    thwompModel->effectColor.a = 0x40;
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[0]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[0]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[0]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[0]];
    rowStartX = positionX = -((0.5f * modelWidth) - 25.0f);
    baseY = positionY = modelHeight;
    baseZ = positionZ = -((0.5f * modelWidth) - 25.0f);
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelWidthExtent / (horizontalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[0] != 0) {
        for (lightId = 0; lightId < thwompModel->actionIndex; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                vector->x = positionX;
                vector->y = positionY;
                vector->z = positionZ;
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionZ = positionZ + positionStepX;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0, horizontalSegmentCount - 1, horizontalSegmentCount - 1);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[1]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[1]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[1]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[1]];
    rowStartX = positionX = -((0.5f * modelWidth) - 25.0f);
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = 0.5f * modelWidth;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[1] != 0) {
        for (lightId = 0; lightId < thwompModel->actionIndex; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                vector->x = positionX;
                vector->y = positionY;
                vector->z = positionZ;
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 1, horizontalSegmentCount - 1, verticalSegmentCount - 1);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[2]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[2]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[2]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[2]];
    rowStartX = positionX = -(0.5f * modelWidth);
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = -((0.5f * modelWidth) - 25.0f);
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[2] != 0) {
        for (lightId = 0; lightId < thwompModel->actionIndex; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                vector->x = positionX;
                vector->y = positionY;
                vector->z = positionZ;
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionZ += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionZ = baseZ;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 2, horizontalSegmentCount - 1, verticalSegmentCount - 1);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[3]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[3]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[3]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[3]];
    rowStartX = positionX = 0.5f * modelWidth;
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = (0.5f * modelWidth) - 25.0f;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[3] != 0) {
        for (lightId = 0; lightId < thwompModel->actionIndex; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                vector->x = positionX;
                vector->y = positionY;
                vector->z = positionZ;
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionZ -= positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionZ = baseZ;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 3, horizontalSegmentCount - 1, verticalSegmentCount - 1);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[4]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[4]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[4]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[4]];
    rowStartX = positionX = -((0.5f * modelWidth) - 25.0f);
    baseY = positionY = modelHeight;
    baseZ = positionZ = (0.5f * modelWidth) - 25.0f;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[4] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                angleDegrees = 90.0f * (lightId / 4.0f);
                vector->x = positionX;
                vector->y = baseY + (25.0 * (cosd(angleDegrees) - 1.0));
                vector->z = baseZ + (25.0 * sind(angleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 4, horizontalSegmentCount - 1, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[5]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[5]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[5]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[5]];
    rowStartX = positionX = -((0.5f * modelWidth) - 25.0f);
    baseY = positionY = 25.0f;
    baseZ = positionZ = (0.5f * modelWidth) - 25.0f;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[5] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                angleDegrees = 90.0f + (90.0f * (lightId / 4.0f));
                vector->x = positionX;
                vector->y = baseY + (25.0 * cosd(angleDegrees));
                vector->z = baseZ + (25.0 * sind(angleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 5, horizontalSegmentCount - 1, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[6]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[6]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[6]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[6]];
    rowStartX = positionX = -(0.5f * modelWidth);
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = (0.5f * modelWidth) - 25.0f;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[6] != 0) {
        for (lightId = 0; lightId < thwompModel->playerState; lightId++) {
            for (vertexCount = 0; vertexCount < 5; vertexCount++) {
                angleDegrees = 90.0f * (vertexCount / 4.0f);
                vector->x = rowStartX - (25.0 * (cosd(angleDegrees) - 1.0));
                vector->y = positionY;
                vector->z = baseZ + (25.0 * sind(angleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 6, 4, verticalSegmentCount - 1);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[7]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[7]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[7]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[7]];
    rowStartX = positionX = (0.5f * modelWidth) - 25.0f;
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = 0.5f * modelWidth;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[7] != 0) {
        for (lightId = 0; lightId < thwompModel->playerState; lightId++) {
            for (vertexCount = 0; vertexCount < 5; vertexCount++) {
                angleDegrees = 90.0f - (90.0f * (vertexCount / 4.0f));
                vector->x = rowStartX + (25.0 * cosd(angleDegrees));
                vector->y = positionY;
                vector->z = baseZ + (25.0 * (sind(angleDegrees) - 1.0));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 7, 4, verticalSegmentCount - 1);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[8]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[8]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[8]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[8]];
    rowStartX = positionX = -((0.5f * modelWidth) - 25.0f);
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = -((0.5f * modelWidth) - 25.0f);
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[8] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                angleDegrees = 90.0f + (90.0f * (lightId / 4.0f));
                vector->x = rowStartX + 25.0 * cosd(angleDegrees);
                vector->y = baseY + (25.0 * sind(angleDegrees));
                vector->z = positionZ;
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionZ += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionZ = baseZ;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 8, horizontalSegmentCount - 1, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[9]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[9]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[9]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[9]];
    rowStartX = positionX = -(0.5f * modelWidth - 25.0f);
    baseY = positionY = 25.0f;
    baseZ = positionZ = -((0.5f * modelWidth) - 25.0f);
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[9] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                angleDegrees = 180.0f + (90.0f * (lightId / 4.0f));
                vector->x = rowStartX + (25.0 * cosd(angleDegrees));
                vector->y = baseY + (25.0 * sind(angleDegrees));
                vector->z = positionZ;
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionZ += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionZ = baseZ;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 9, horizontalSegmentCount - 1, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[10]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[10]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[10]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[10]];
    rowStartX = positionX = (0.5f * modelWidth) - 25.0f;
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = (0.5f * modelWidth) - 25.0f;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[10] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                angleDegrees = 90.0f - (90.0f * (lightId / 4.0f));
                vector->x = rowStartX + (25.0 * cosd(angleDegrees));
                vector->y = baseY + (25.0 * sind(angleDegrees));
                vector->z = positionZ;
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionZ -= positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionZ = baseZ;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0xA, horizontalSegmentCount - 1, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[11]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[11]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[11]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[11]];
    rowStartX = positionX = (0.5f * modelWidth) - 25.0f;
    baseY = positionY = 25.0f;
    baseZ = positionZ = (0.5f * modelWidth) - 25.0f;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[11] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                angleDegrees = -(90.0f * (lightId / 4.0f));
                vector->x = rowStartX + (25.0 * cosd(angleDegrees));
                vector->y = baseY + (25.0 * sind(angleDegrees));
                vector->z = positionZ;
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionZ -= positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionZ = baseZ;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0xB, horizontalSegmentCount - 1, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[12]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[12]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[12]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[12]];
    rowStartX = positionX = (0.5f * modelWidth) - 25.0f;
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = -((0.5f * modelWidth) - 25.0f);
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[12] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                angleDegrees = 90.0f + (90.0f * (lightId / 4.0f));
                vector->x = positionX;
                vector->y = baseY + (25.0 * sind(angleDegrees));
                vector->z = baseZ + (25.0 * cosd(angleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX -= positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0xC, horizontalSegmentCount - 1, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[13]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[13]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[13]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[13]];
    rowStartX = positionX = (0.5f * modelWidth) - 25.0f;
    baseY = positionY = 25.0f;
    baseZ = positionZ = -((0.5f * modelWidth) - 25.0f);
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[13] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                angleDegrees = 180.0f + (90.0f * (lightId / 4.0f));
                vector->x = positionX;
                vector->y = baseY + (25.0 * sind(angleDegrees));
                vector->z = baseZ + (25.0 * cosd(angleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX -= positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0xD, horizontalSegmentCount - 1, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[14]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[14]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[14]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[14]];
    rowStartX = positionX = (0.5f * modelWidth) - 25.0f;
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = -((0.5f * modelWidth) - 25.0f);
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[14] != 0) {
        for (lightId = 0; lightId < thwompModel->playerState; lightId++) {
            for (vertexCount = 0; vertexCount < 5; vertexCount++) {
                angleDegrees = -(90.0f * (vertexCount / 4.0f));
                vector->x = rowStartX + (25.0 * cosd(angleDegrees));
                vector->y = positionY;
                vector->z = baseZ + (25.0 * sind(angleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0xE, 4, verticalSegmentCount - 1);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[15]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[15]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[15]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[15]];
    rowStartX = positionX = -((0.5f * modelWidth) - 25.0f);
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = -((0.5f * modelWidth) - 25.0f);
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[15] != 0) {
        for (lightId = 0; lightId < thwompModel->playerState; lightId++) {
            for (vertexCount = 0; vertexCount < 5; vertexCount++) {
                angleDegrees = -90.0f - (90.0f * (vertexCount / 4.0f));
                vector->x = rowStartX + (25.0 * cosd(angleDegrees));
                vector->y = positionY;
                vector->z = baseZ + (25.0 * sind(angleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0xF, 4, verticalSegmentCount - 1);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[16]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[16]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[16]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[16]];
    rowStartX = positionX = -((0.5f * modelWidth) - 25.0f);
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = (0.5f * modelWidth) - 25.0f;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[16] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < 5; vertexCount++) {
                angleDegrees = 90.0f * (vertexCount / 4.0f);
                secondaryAngleDegrees = 90.0f * (lightId / 4.0f);
                vector->x = rowStartX - (25.0 * cosd(angleDegrees) * sind(secondaryAngleDegrees));
                vector->y = baseY + (25.0 * cosd(secondaryAngleDegrees));
                vector->z = baseZ + (25.0 * sind(angleDegrees) * sind(secondaryAngleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0x10, 4, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[17]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[17]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[17]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[17]];
    rowStartX = positionX = (0.5f * modelWidth) - 25.0f;
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = (0.5f * modelWidth) - 25.0f;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[17] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < 5; vertexCount++) {
                angleDegrees = 90.0f - (90.0f * (vertexCount / 4.0f));
                secondaryAngleDegrees = -(90.0f * (lightId / 4.0f));
                vector->x = rowStartX + (25.0 * cosd(angleDegrees) * -sind(secondaryAngleDegrees));
                vector->y = baseY + (25.0 * cosd(secondaryAngleDegrees));
                vector->z = baseZ + (25.0 * sind(angleDegrees) * -sind(secondaryAngleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0x11, 4, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[18]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[18]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[18]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[18]];
    rowStartX = positionX = -((0.5f * modelWidth) - 25.0f);
    baseY = positionY = 25.0f;
    baseZ = positionZ = (0.5f * modelWidth) - 25.0f;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[18] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < 5; vertexCount++) {
                angleDegrees = 90.0f * (vertexCount / 4.0f);
                secondaryAngleDegrees = 90.0f + (90.0f * (lightId / 4.0f));
                vector->x = rowStartX - (25.0 * cosd(angleDegrees) * sind(secondaryAngleDegrees));
                vector->y = baseY + (25.0 * cosd(secondaryAngleDegrees));
                vector->z = baseZ + (25.0 * sind(angleDegrees) * sind(secondaryAngleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0x12, 4, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[19]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[19]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[19]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[19]];
    rowStartX = positionX = (0.5f * modelWidth) - 25.0f;
    baseY = positionY = 25.0f;
    baseZ = positionZ = (0.5f * modelWidth) - 25.0f;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[19] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < 5; vertexCount++) {
                angleDegrees = 90.0f * (vertexCount / 4.0f);
                secondaryAngleDegrees = 90.0f * (lightId / 4.0f);
                vector->x = rowStartX + (25.0 * cosd(angleDegrees) * sind(secondaryAngleDegrees));
                vector->y = baseY - (25.0 * cosd(secondaryAngleDegrees));
                vector->z = baseZ + (25.0 * sind(angleDegrees) * sind(secondaryAngleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0x13, 4, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[20]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[20]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[20]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[20]];
    rowStartX = positionX = (0.5f * modelWidth) - 25.0f;
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = -((0.5f * modelWidth) - 25.0f);
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[20] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < 5; vertexCount++) {
                angleDegrees = 180.0f + (90.0f * (vertexCount / 4.0f));
                secondaryAngleDegrees = 90.0f * (lightId / 4.0f);
                vector->x = rowStartX - (25.0 * cosd(angleDegrees) * sind(secondaryAngleDegrees));
                vector->y = baseY + (25.0 * cosd(secondaryAngleDegrees));
                vector->z = baseZ + (25.0 * sind(angleDegrees) * sind(secondaryAngleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0x14, 4, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[21]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[21]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[21]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[21]];
    rowStartX = positionX = -(0.5f * modelWidth - 25.0f);
    baseY = positionY = modelHeight - 25.0f;
    baseZ = positionZ = -((0.5f * modelWidth) - 25.0f);
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[21] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < 5; vertexCount++) {
                angleDegrees = 270.0f - (90.0f * (vertexCount / 4.0f));
                secondaryAngleDegrees = -(90.0f * (lightId / 4.0f));
                vector->x = rowStartX + (cosd(angleDegrees) * 25.0 * -sind(secondaryAngleDegrees));
                vector->y = baseY + 25.0 * cosd(secondaryAngleDegrees);
                vector->z = baseZ + (25.0 * sind(angleDegrees) * -sind(secondaryAngleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0x15, 4, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[22]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[22]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[22]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[22]];
    rowStartX = positionX = (0.5f * modelWidth) - 25.0f;
    baseY = positionY = 25.0f;
    baseZ = positionZ = -((0.5f * modelWidth) - 25.0f);
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[22] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < 5; vertexCount++) {
                angleDegrees = 180.0f + (90.0f * (vertexCount / 4.0f));
                secondaryAngleDegrees = 90.0f + (90.0f * (lightId / 4.0f));
                vector->x = rowStartX - (25.0 * cosd(angleDegrees) * sind(secondaryAngleDegrees));
                vector->y = baseY + (25.0 * cosd(secondaryAngleDegrees));
                vector->z = baseZ + (25.0 * sind(angleDegrees) * sind(secondaryAngleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0x16, 4, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[23]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[23]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[23]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[23]];
    rowStartX = positionX = -((modelWidth * 0.5f) - 25.0f);
    baseY = positionY = 25.0f;
    baseZ = positionZ = -((modelWidth * 0.5f) - 25.0f);
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelHeightExtent / (verticalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[23] != 0) {
        for (lightId = 0; lightId < 5; lightId++) {
            for (vertexCount = 0; vertexCount < 5; vertexCount++) {
                angleDegrees = 180.0f + (90.0f * (vertexCount / 4.0f));
                secondaryAngleDegrees = 90.0f * (lightId / 4.0f);
                vector->x = rowStartX + (25.0 * cosd(angleDegrees) * sind(secondaryAngleDegrees));
                vector->y = baseY - (25.0 * cosd(secondaryAngleDegrees));
                vector->z = baseZ + (25.0 * sind(angleDegrees) * sind(secondaryAngleDegrees));
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionY -= positionStepY;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0x17, 4, 4);
    }
    vector = &thwompModel->positionStart[thwompModel->vertexIndexOffsets[24]];
    vector2 = &thwompModel->positionWork[thwompModel->vertexIndexOffsets[24]];
    textureCoordinate = &thwompModel->textureCoordinates[thwompModel->vertexIndexOffsets[24]];
    positionBufferCursor = &thwompModel->positionBuffer[thwompModel->vertexIndexOffsets[24]];
    rowStartX = positionX = -((0.5f * modelWidth) - 25.0f);
    baseY = positionY = 0.0f;
    baseZ = positionZ = (0.5f * modelWidth) - 25.0f;
    textureUStart = textureU = 0.0f;
    textureVStart = textureV = 0.0f;
    positionStepX = modelWidthExtent / (horizontalSegmentCount - 1);
    positionStepY = modelWidthExtent / (horizontalSegmentCount - 1);
    textureUStep = 1.0f / (thwompModel->actionIndex - 1);
    textureVStep = 1.0f / (thwompModel->actionIndex - 1);
    if (thwompModel->vertexIndexCounts[0x18] != 0) {
        for (lightId = 0; lightId < thwompModel->actionIndex; lightId++) {
            for (vertexCount = 0; vertexCount < thwompModel->actionIndex; vertexCount++) {
                vector->x = positionX;
                vector->y = positionY;
                vector->z = positionZ;
                textureCoordinate->x = textureU;
                textureCoordinate->y = textureV;
                positionX += positionStepX;
                textureU += textureUStep;
                vector++;
                vector2++;
                textureCoordinate++;
                positionBufferCursor++;
            }
            positionX = rowStartX;
            textureU = textureUStart;
            positionZ = positionZ - positionStepX;
            textureV += textureVStep;
        }
        InitializeThwompModelData(thwompModel, model, 0x18, horizontalSegmentCount - 1, horizontalSegmentCount - 1);
    }
    for (lightId = 0; lightId < thwompModel->vertexCount; lightId++) {
        for (vertexCount = 0; vertexCount < lightId; vertexCount++) {
            positionX = thwompModel->positionStart[lightId].x - thwompModel->positionStart[vertexCount].x;
            positionY = thwompModel->positionStart[lightId].y - thwompModel->positionStart[vertexCount].y;
            positionZ = thwompModel->positionStart[lightId].z - thwompModel->positionStart[vertexCount].z;
            vectorMagnitude = (positionZ * positionZ) + ((positionX * positionX) + (positionY * positionY));
            if (vectorMagnitude < 10.0f) {
                thwompModel->positionBuffer[lightId] = vertexCount;
                if (thwompModel->positionBuffer[vertexCount] != -1) {
                    thwompModel->positionBuffer[lightId] = thwompModel->positionBuffer[vertexCount];
                }
                break;
            }
        }
        if (vertexCount >= lightId) {
            thwompModel->positionBuffer[lightId] = -1;
        }
    }
    memcpy(thwompModel->positionTarget, thwompModel->positionStart, thwompModel->vertexCount * sizeof(Vec));
    memcpy(thwompModel->rotationOutput, &thwompModel->positionTarget[thwompModel->vertexIndexOffsets[0]], thwompModel->vertexIndexCounts[0] * sizeof(Vec));
    thwompModel->workState = 0;
    UpdateThwompModelVertices(thwompModel, model);
    BuildThwompModelGeometry(thwompModel, model);
    textureU = sqrtf((0.5f * modelHeight * (0.5f * modelHeight)) + ((0.5f * modelWidth * (0.5f * modelWidth)) + (0.5f * modelWidth * (0.5f * modelWidth))));
    for (lightId = 0; lightId < thwompModel->vertexCount; lightId++) {
        positionX = thwompModel->positionStart[lightId].x;
        positionY = thwompModel->positionStart[lightId].y - (0.5f * modelHeight);
        positionZ = thwompModel->positionStart[lightId].z;
        vectorMagnitude = sqrtf((positionZ * positionZ) + ((positionX * positionX) + (positionY * positionY)));
        angleDegrees = vectorMagnitude;
        vectorMagnitude = 1.0f - (vectorMagnitude / textureU);
        if (angleDegrees != 0.0f) {
            secondaryAngleDegrees = 1.0f / angleDegrees;
        }
        else {
            secondaryAngleDegrees = 0.0;
        }
        thwompModel->positionOutput[lightId].x = vectorMagnitude * (secondaryAngleDegrees * thwompModel->positionStart[lightId].x);
        thwompModel->positionOutput[lightId].y = vectorMagnitude * (secondaryAngleDegrees * thwompModel->positionStart[lightId].y);
        thwompModel->positionOutput[lightId].z = vectorMagnitude * (secondaryAngleDegrees * thwompModel->positionStart[lightId].z);
    }
    for (lightId = 0; lightId < thwompModel->vertexCount; lightId++) {
        if ((((thwompModel->vertexIndexCounts[6] <= 0) || (lightId < thwompModel->vertexIndexOffsets[6]) || (lightId >= (thwompModel->vertexIndexOffsets[6] + thwompModel->vertexIndexCounts[6])))
                && ((thwompModel->vertexIndexCounts[7] <= 0) || (lightId < thwompModel->vertexIndexOffsets[7]) || (lightId >= (thwompModel->vertexIndexOffsets[7] + thwompModel->vertexIndexCounts[7])))
                && ((thwompModel->vertexIndexCounts[0xE] <= 0) || (lightId < thwompModel->vertexIndexOffsets[0xE]) || (lightId >= (thwompModel->vertexIndexOffsets[0xE] + thwompModel->vertexIndexCounts[0xE])))
                && ((thwompModel->vertexIndexCounts[0xF] <= 0) || (lightId < thwompModel->vertexIndexOffsets[0xF]) || (lightId >= (thwompModel->vertexIndexOffsets[0xF] + thwompModel->vertexIndexCounts[0xF])))
                && ((thwompModel->vertexIndexCounts[4] <= 0) || (lightId < thwompModel->vertexIndexOffsets[4]) || (lightId >= (thwompModel->vertexIndexOffsets[4] + thwompModel->vertexIndexCounts[4])))
                && ((thwompModel->vertexIndexCounts[8] <= 0) || (lightId < thwompModel->vertexIndexOffsets[8]) || (lightId >= (thwompModel->vertexIndexOffsets[8] + thwompModel->vertexIndexCounts[8])))
                && ((thwompModel->vertexIndexCounts[0xA] <= 0) || (lightId < thwompModel->vertexIndexOffsets[0xA]) || (lightId >= (thwompModel->vertexIndexOffsets[0xA] + thwompModel->vertexIndexCounts[0xA])))
                && ((thwompModel->vertexIndexCounts[0xC] <= 0) || (lightId < thwompModel->vertexIndexOffsets[0xC]) || (lightId >= (thwompModel->vertexIndexOffsets[0xC] + thwompModel->vertexIndexCounts[0xC])))
                && ((thwompModel->vertexIndexCounts[0x10] <= 0) || (lightId < thwompModel->vertexIndexOffsets[0x10])
                    || (lightId >= (thwompModel->vertexIndexOffsets[0x10] + thwompModel->vertexIndexCounts[0x10])))
                && ((thwompModel->vertexIndexCounts[0x11] <= 0) || (lightId < thwompModel->vertexIndexOffsets[0x11])
                    || (lightId >= (thwompModel->vertexIndexOffsets[0x11] + thwompModel->vertexIndexCounts[0x11])))
                && ((thwompModel->vertexIndexCounts[0x14] <= 0) || (lightId < thwompModel->vertexIndexOffsets[0x14])
                    || (lightId >= (thwompModel->vertexIndexOffsets[0x14] + thwompModel->vertexIndexCounts[0x14])))
                && ((thwompModel->vertexIndexCounts[0x15] <= 0) || (lightId < thwompModel->vertexIndexOffsets[0x15])
                    || (lightId >= (thwompModel->vertexIndexOffsets[0x15] + thwompModel->vertexIndexCounts[0x15])))
                && (lightId >= thwompModel->vertexIndexOffsets[4]))
            || (thwompModel->positionBuffer[lightId] != -1)) {
            thwompModel->rotationWork[lightId].x = thwompModel->rotationWork[lightId].y = thwompModel->rotationWork[lightId].z = 0.0;
            thwompModel->displayListIndices[lightId][0] = thwompModel->displayListIndices[lightId][1] = 0;
            thwompModel->displayListIndices[lightId][2] = 0;
        }
        else if (((thwompModel->vertexIndexCounts[0] > 0) && (lightId >= thwompModel->vertexIndexOffsets[0]) && (lightId < (thwompModel->vertexIndexOffsets[0] + thwompModel->vertexIndexCounts[0])))
            || ((thwompModel->vertexIndexCounts[4] > 0) && (lightId >= thwompModel->vertexIndexOffsets[4]) && (lightId < (thwompModel->vertexIndexOffsets[4] + thwompModel->vertexIndexCounts[4])))
            || ((thwompModel->vertexIndexCounts[8] > 0) && (lightId >= thwompModel->vertexIndexOffsets[8]) && (lightId < (thwompModel->vertexIndexOffsets[8] + thwompModel->vertexIndexCounts[8])))
            || ((thwompModel->vertexIndexCounts[0xA] > 0) && (lightId >= thwompModel->vertexIndexOffsets[0xA]) && (lightId < (thwompModel->vertexIndexOffsets[0xA] + thwompModel->vertexIndexCounts[0xA])))
            || ((thwompModel->vertexIndexCounts[0xC] > 0) && (lightId >= thwompModel->vertexIndexOffsets[0xC]) && (lightId < (thwompModel->vertexIndexOffsets[0xC] + thwompModel->vertexIndexCounts[0xC])))
            || ((thwompModel->vertexIndexCounts[0x10] > 0) && (lightId >= thwompModel->vertexIndexOffsets[0x10]) && (lightId < (thwompModel->vertexIndexOffsets[0x10] + thwompModel->vertexIndexCounts[0x10])))
            || ((thwompModel->vertexIndexCounts[0x11] > 0) && (lightId >= thwompModel->vertexIndexOffsets[0x11]) && (lightId < (thwompModel->vertexIndexOffsets[0x11] + thwompModel->vertexIndexCounts[0x11])))
            || ((thwompModel->vertexIndexCounts[0x14] > 0) && (lightId >= thwompModel->vertexIndexOffsets[0x14]) && (lightId < (thwompModel->vertexIndexOffsets[0x14] + thwompModel->vertexIndexCounts[0x14])))
            || ((thwompModel->vertexIndexCounts[0x15] > 0) && (lightId >= thwompModel->vertexIndexOffsets[0x15])
                && (lightId < (thwompModel->vertexIndexOffsets[0x15] + thwompModel->vertexIndexCounts[0x15])))) {
            *thwompModel->displayListIndices[lightId] = thwompModel->displayListIndices[lightId][1] = 0;
            vectorMagnitude = sqrtf((thwompModel->positionStart[lightId].x * thwompModel->positionStart[lightId].x) + (thwompModel->positionStart[lightId].z * thwompModel->positionStart[lightId].z));
            angleDegrees = vectorMagnitude;
            vectorMagnitude = 1.0f - (vectorMagnitude / sqrtf(modelWidth * modelWidth));
            thwompModel->rotationWork[lightId].y = -(180.0f * vectorMagnitude);
            angleDegrees = sqrtf(angleDegrees);
            if (angleDegrees != 0.0f) {
                secondaryAngleDegrees = 1.0f / angleDegrees;
            }
            else {
                secondaryAngleDegrees = 0.0;
            }
            thwompModel->rotationWork[lightId].x = sind((180.0f * (secondaryAngleDegrees * thwompModel->positionStart[lightId].x)));
            thwompModel->rotationWork[lightId].z = sind((180.0f * (secondaryAngleDegrees * thwompModel->positionStart[lightId].z)));
            thwompModel->rotationWork[lightId].x *= 20.0f * -(1.0f - vectorMagnitude);
            thwompModel->rotationWork[lightId].z *= (20.0f * -(1.0f - vectorMagnitude));
            thwompModel->displayListIndices[lightId][2] = 1;
        }
        else {
            vectorMagnitude = sqrtf((thwompModel->positionStart[lightId].x * thwompModel->positionStart[lightId].x) + (thwompModel->positionStart[lightId].z * thwompModel->positionStart[lightId].z));
            if (vectorMagnitude != 0.0f) {
                vectorMagnitude = 1.0f / vectorMagnitude;
            }
            else {
                vectorMagnitude = 0.0f;
            }
            thwompModel->rotationWork[lightId].x = -(vectorMagnitude * thwompModel->positionStart[lightId].x);
            thwompModel->rotationWork[lightId].z = -(vectorMagnitude * thwompModel->positionStart[lightId].z);
            thwompModel->rotationWork[lightId].y = 0.0;
            angleDegrees = 180.0 + atan2d(thwompModel->positionStart[lightId].x, thwompModel->positionStart[lightId].z);
            if (angleDegrees > 360.0f) {
                angleDegrees = fmod(angleDegrees, 360.0);
            }
            else if (angleDegrees < 0.0f) {
                angleDegrees += 360.0f;
            }
            angleDegrees = 255.0f * (0.0027777778f * angleDegrees);
            if (angleDegrees >= 255.0f) {
                angleDegrees = 255.0f;
            }
            else if (angleDegrees < 0.0f) {
                angleDegrees = 0.0;
            }
            secondaryAngleDegrees = 64.0f * (thwompModel->positionStart[lightId].y / modelHeight);
            if (secondaryAngleDegrees >= 63.0f) {
                secondaryAngleDegrees = 63.0f;
            }
            else if (secondaryAngleDegrees < 0.0f) {
                secondaryAngleDegrees = 0.0;
            }
            thwompModel->displayListIndices[lightId][0] = angleDegrees;
            thwompModel->displayListIndices[lightId][1] = secondaryAngleDegrees;
            thwompModel->displayListIndices[lightId][2] = 2;
        }
    }
    DCFlushRangeNoSync(thwompModel->positionTarget, thwompModel->vertexCount * sizeof(Vec));
    DCFlushRangeNoSync(thwompModel->positionStart, thwompModel->vertexCount * sizeof(Vec));
    DCFlushRangeNoSync(thwompModel->positionWork, thwompModel->vertexCount * sizeof(Vec));
    DCFlushRangeNoSync(thwompModel->textureCoordinates, thwompModel->vertexCount * sizeof(Vec2f));
    DCFlushRangeNoSync(thwompModel->positionBuffer, thwompModel->vertexCount * 2);
    PPCSync();
    for (vertexIndex = 0; vertexIndex < 0x19; vertexIndex++) {
        if ((thwompModel->vertexBufferCounts[vertexIndex] != 0) && thwompModel->vertexBuffers[vertexIndex] && (thwompModel->vertexIndexCounts[vertexIndex] != 0)) {
            displayListAllocation = HuMemDirectMallocNum(HEAP_MODEL, 0x4000, model->mallocNo);
            displayListBuffer = displayListAllocation;
            (void)hookId;
            DCFlushRange(displayListAllocation, 0x4000);
            GXBeginDisplayList(displayListBuffer, 0x4000);
            GXBegin(GX_QUADS, GX_VTXFMT0, thwompModel->vertexBufferCounts[vertexIndex] * 4);
            vertexIndexBuffer = thwompModel->vertexBuffers[vertexIndex];
            for (lightId = 0; lightId < thwompModel->vertexBufferCounts[vertexIndex]; lightId++) {
                for (vertexCount = 0; vertexCount < 4; vertexCount++) {
                    if (thwompModel->positionBuffer[vertexIndexBuffer[vertexCount]] == -1) {
                        hookId = vertexIndexBuffer[vertexCount];
                    }
                    else {
                        hookId = thwompModel->positionBuffer[vertexIndexBuffer[vertexCount]];
                    }
                    GXPosition1x16(hookId);
                    GXNormal1x16(hookId);
                    GXColor1x16(0);
                    GXTexCoord1x16(vertexIndexBuffer[vertexCount]);
                }
                vertexIndexBuffer += 5;
            }
            thwompModel->displayListSizes[vertexIndex] = GXEndDisplayList();
            if (thwompModel->displayListSizes[vertexIndex] >= 0x4000) {
                OSReport("ERROR! GList test size over! \n");
            }
            thwompModel->displayLists[vertexIndex] = HuMemDirectMallocNum(HEAP_MODEL, thwompModel->displayListSizes[vertexIndex], model->mallocNo);
            memcpy(thwompModel->displayLists[vertexIndex], displayListAllocation, thwompModel->displayListSizes[vertexIndex]);
            DCFlushRange(thwompModel->displayLists[vertexIndex], thwompModel->displayListSizes[vertexIndex]);
            HuMemDirectFree(displayListAllocation);
        }
    }
    return modelId;
}

void InitializeThwompModelData(M425ThwompModelWork *thwompModel, HU3DMODEL *model, s32 surfaceIndex, s32 horizontalSegmentCount, s32 verticalSegmentCount)
{
    s16 *vertexIndices;
    s32 verticalSegment;
    s32 horizontalSegment;

    thwompModel->vertexBufferCounts[surfaceIndex] = horizontalSegmentCount * verticalSegmentCount;
    thwompModel->vertexBuffers[surfaceIndex] = HuMemDirectMallocNum(HEAP_MODEL, thwompModel->vertexBufferCounts[surfaceIndex] * 10, model->mallocNo);
    memset(thwompModel->vertexBuffers[surfaceIndex], 0, thwompModel->vertexBufferCounts[surfaceIndex] * 10);
    vertexIndices = thwompModel->vertexBuffers[surfaceIndex];
    for (verticalSegment = 0; verticalSegment < verticalSegmentCount; verticalSegment++) {
        for (horizontalSegment = 0; horizontalSegment < horizontalSegmentCount; horizontalSegment++) {
            vertexIndices[0] = thwompModel->vertexIndexOffsets[surfaceIndex] + (horizontalSegmentCount + 1) * verticalSegment + horizontalSegment;
            vertexIndices[1] = thwompModel->vertexIndexOffsets[surfaceIndex] + (horizontalSegmentCount + 1) * verticalSegment + (horizontalSegment + 1);
            vertexIndices[2] = thwompModel->vertexIndexOffsets[surfaceIndex] + (horizontalSegmentCount + 1) * (verticalSegment + 1) + (horizontalSegment + 1);
            vertexIndices[3] = thwompModel->vertexIndexOffsets[surfaceIndex] + (horizontalSegmentCount + 1) * (verticalSegment + 1) + horizontalSegment;
            vertexIndices += 5;
        }
    }
    DCFlushRangeNoSync(thwompModel->vertexBuffers[surfaceIndex], thwompModel->vertexBufferCounts[surfaceIndex] * 10);
    PPCSync();
}

void UpdateThwompModelVertices(M425ThwompModelWork *thwompModel, HU3DMODEL *model)
{
    float edgeDeltaX;
    float edgeDeltaY;
    float edgeDeltaZ;
    float edgeLengthSquared;
    s16 *vertexIndices;
    s32 surfaceIndex;
    s32 polygonIndex;
    s32 cornerIndex;

    for (surfaceIndex = 0; surfaceIndex < 0x19; surfaceIndex++) {
        if ((thwompModel->vertexBufferCounts[surfaceIndex] != 0) && thwompModel->vertexBuffers[surfaceIndex]) {
            for (vertexIndices = thwompModel->vertexBuffers[surfaceIndex], polygonIndex = 0; polygonIndex < thwompModel->vertexBufferCounts[surfaceIndex]; polygonIndex++, vertexIndices += 0x5) {
                for (cornerIndex = 0; cornerIndex < 4; cornerIndex++) {
                    if (thwompModel->positionBuffer[vertexIndices[cornerIndex]] != -1) {
                        vertexIndices[cornerIndex] = thwompModel->positionBuffer[vertexIndices[cornerIndex]];
                    }
                }
            }
        }
    }
    for (surfaceIndex = 0; surfaceIndex < 0x19; surfaceIndex++) {
        if ((thwompModel->vertexBufferCounts[surfaceIndex] != 0) && thwompModel->vertexBuffers[surfaceIndex]) {
            for (vertexIndices = thwompModel->vertexBuffers[surfaceIndex], polygonIndex = 0; polygonIndex < thwompModel->vertexBufferCounts[surfaceIndex]; polygonIndex++, vertexIndices += 0x5) {
                edgeDeltaX = thwompModel->positionStart[vertexIndices[0]].x - thwompModel->positionStart[vertexIndices[1]].x;
                edgeDeltaY = thwompModel->positionStart[vertexIndices[0]].y - thwompModel->positionStart[vertexIndices[1]].y;
                edgeDeltaZ = thwompModel->positionStart[vertexIndices[0]].z - thwompModel->positionStart[vertexIndices[1]].z;
                edgeLengthSquared = (edgeDeltaZ * edgeDeltaZ) + ((edgeDeltaX * edgeDeltaX) + (edgeDeltaY * edgeDeltaY));
                if (edgeLengthSquared < 10.0f) {
                    vertexIndices[4] = 1;
                }
                else {
                    edgeDeltaX = thwompModel->positionStart[vertexIndices[0]].x - thwompModel->positionStart[vertexIndices[2]].x;
                    edgeDeltaY = thwompModel->positionStart[vertexIndices[0]].y - thwompModel->positionStart[vertexIndices[2]].y;
                    edgeDeltaZ = thwompModel->positionStart[vertexIndices[0]].z - thwompModel->positionStart[vertexIndices[2]].z;
                    edgeLengthSquared = (edgeDeltaZ * edgeDeltaZ) + ((edgeDeltaX * edgeDeltaX) + (edgeDeltaY * edgeDeltaY));
                    if (edgeLengthSquared < 10.0f) {
                        vertexIndices[4] = 2;
                    }
                    else {
                        vertexIndices[4] = 0;
                    }
                }
            }
        }
    }
}

void BuildThwompModelGeometry(M425ThwompModelWork *thwompModel, HU3DMODEL *model)
{
    Vec vector;
    Vec vector2;
    Vec vector3;
    s32 surfaceIndex;
    s32 triangleIndexB;
    s16 *vertexIndices;
    s32 triangleIndexA;
    s32 triangleIndexC;
    s32 triangleIndexD;
    s32 polygonIndex;

    memset(thwompModel->positionWork, 0, thwompModel->vertexCount * sizeof(Vec));
    for (surfaceIndex = 0; surfaceIndex < thwompModel->surfaceCount; surfaceIndex++) {
        if ((thwompModel->vertexBufferCounts[surfaceIndex] != 0) && thwompModel->vertexBuffers[surfaceIndex]) {
            for (vertexIndices = thwompModel->vertexBuffers[surfaceIndex], polygonIndex = 0; polygonIndex < thwompModel->vertexBufferCounts[surfaceIndex]; polygonIndex++, vertexIndices += 5) {
                switch (vertexIndices[4]) {
                    case 1:
                        triangleIndexA = 0;
                        triangleIndexB = 2;
                        triangleIndexC = 3;
                        triangleIndexD = 1;
                        break;
                    case 2:
                        triangleIndexA = 0;
                        triangleIndexB = 1;
                        triangleIndexC = 3;
                        triangleIndexD = 2;
                        break;
                    default:
                        triangleIndexA = 0;
                        triangleIndexB = 1;
                        triangleIndexC = 2;
                        triangleIndexD = 3;
                        break;
                }
                triangleIndexA = vertexIndices[triangleIndexA];
                triangleIndexB = vertexIndices[triangleIndexB];
                triangleIndexC = vertexIndices[triangleIndexC];
                triangleIndexD = vertexIndices[triangleIndexD];
                vector2.x = thwompModel->positionTarget[triangleIndexB].x - thwompModel->positionTarget[triangleIndexA].x;
                vector2.y = thwompModel->positionTarget[triangleIndexB].y - thwompModel->positionTarget[triangleIndexA].y;
                vector2.z = thwompModel->positionTarget[triangleIndexB].z - thwompModel->positionTarget[triangleIndexA].z;
                vector3.x = thwompModel->positionTarget[triangleIndexC].x - thwompModel->positionTarget[triangleIndexB].x;
                vector3.y = thwompModel->positionTarget[triangleIndexC].y - thwompModel->positionTarget[triangleIndexB].y;
                vector3.z = thwompModel->positionTarget[triangleIndexC].z - thwompModel->positionTarget[triangleIndexB].z;
                VECCrossProduct(&vector2, &vector3, &vector);
                vector.x *= -1.0f;
                vector.y *= -1.0f;
                vector.z *= -1.0f;
                thwompModel->positionWork[triangleIndexA].x += vector.x;
                thwompModel->positionWork[triangleIndexA].y += vector.y;
                thwompModel->positionWork[triangleIndexA].z += vector.z;
                thwompModel->positionWork[triangleIndexB].x += vector.x;
                thwompModel->positionWork[triangleIndexB].y += vector.y;
                thwompModel->positionWork[triangleIndexB].z += vector.z;
                thwompModel->positionWork[triangleIndexC].x += vector.x;
                thwompModel->positionWork[triangleIndexC].y += vector.y;
                thwompModel->positionWork[triangleIndexC].z += vector.z;
                thwompModel->positionWork[triangleIndexD].x += vector.x;
                thwompModel->positionWork[triangleIndexD].y += vector.y;
                thwompModel->positionWork[triangleIndexD].z += vector.z;
            }
        }
    }
    for (surfaceIndex = 0; surfaceIndex < thwompModel->vertexCount; surfaceIndex++) {
        VECNormalize(&thwompModel->positionWork[surfaceIndex], &thwompModel->positionWork[surfaceIndex]);
    }
}

s32 thwompModelResourceIds[3][2] = { { 0x638, 0x63B }, { 0x639, 0x63C }, { 0x63A, 0x63D } };
GXColor thwompModelColor = { 255, 255, 255, 255 };
Mtx thwompModelMatrix = {
    { 0.5f, 0.0f, 0.0f, -0.5f },
    { 0.0f, 0.5f, 0.0f, -0.5f },
    { 0.0f, 0.0f, 0.5f, -0.5f },
};
Vec thwompQuadVertices[4] = {
    { -0.5f, 0.5f, 0.0f },
    { 0.5f, 0.5f, 0.0f },
    { 0.5f, -0.5f, 0.0f },
    { -0.5f, -0.5f, 0.0f },
};
Vec2f thwompTextureCoordinates[4] = {
    { 0.0f, 0.0f },
    { 0.25f, 0.0f },
    { 0.25f, 0.25f },
    { 0.0f, 0.25f },
};

void SetThwompAnimationState(s32 modelIndex)
{
    Vec vector;
    float totalMotionTime;
    float pitchScale;
    float motionWeight;
    M425ThwompModelWork *thwompModel;
    s32 motionSlot;
    s32 animationSlot;
    HU3DLIGHT *light;
    HU3DMODEL *model;
    s32 soundEffectId;

    if (thwompObject) {
        model = &Hu3DData[thwompObject->mdlId[modelIndex]];
        thwompModel = model->hookData;
        for (motionSlot = 0, totalMotionTime = 0.0f; motionSlot < 6; motionSlot++) {
            if ((thwompModel->motionStates[motionSlot] == -1) || (thwompModel->motionTimers[motionSlot] < 0.0f)) {
                thwompModel->motionStates[motionSlot] = -1;
                thwompModel->motionTimers[motionSlot] = 0.0f;
            }
            else {
                totalMotionTime += thwompModel->motionTimers[motionSlot];
                for (animationSlot = 0; animationSlot < 5; animationSlot++) {
                    if (thwompModel->activeMotionIds[animationSlot] == thwompModel->motionStates[motionSlot]) {
                        break;
                    }
                }
                if (animationSlot >= 5) {
                    for (animationSlot = 0; animationSlot < 5; animationSlot++) {
                        if ((thwompModel->activeMotionIds[animationSlot] == -1) && (thwompModel->activeMotionTimers[animationSlot] <= 0.0f)) {
                            break;
                        }
                    }
                }
                if ((animationSlot < 5) && (thwompModel->activeMotionTimers[animationSlot] < thwompModel->motionTimers[motionSlot])) {
                    thwompModel->activeMotionIds[animationSlot] = thwompModel->motionStates[motionSlot];
                    thwompModel->activeMotionTimers[animationSlot] = thwompModel->motionTimers[motionSlot];
                    thwompModel->animationIds[animationSlot] = thwompModel->animationTextureIds[motionSlot];
                }
                thwompModel->motionStates[motionSlot] = -1;
                thwompModel->motionTimers[motionSlot] = 0.0f;
                thwompModel->animationTextureIds[motionSlot] = 0;
            }
        }
        for (motionSlot = 0, animationSlot = 0, thwompModel->motionTimer = 0.0f; motionSlot < 6; motionSlot++) {
            if ((thwompModel->activeMotionIds[motionSlot] != -1) && (thwompModel->activeMotionTimers[motionSlot] > 0.0f)) {
                thwompModel->motionTimer += 0.8f * thwompModel->activeMotionTimers[motionSlot];
                animationSlot++;
            }
        }

        for (motionSlot = 0, animationSlot = 0, thwompModel->movementSpeed = 0.0f; motionSlot < 6; motionSlot++) {
            if ((thwompModel->activeMotionIds[motionSlot] != -1) && (thwompModel->activeMotionTimers[motionSlot] > 0.0f) && (thwompModel->animationIds[motionSlot] != 0)) {
                thwompModel->movementSpeed += 0.8f * thwompModel->activeMotionTimers[motionSlot];
                animationSlot++;
            }
        }
        if (animationSlot != 0) {
            thwompModel->motionIntensity = 20.0f * (thwompModel->movementSpeed * thwompModel->movementSpeed);
            thwompModel->animationTimeRemaining -= thwompModel->motionIntensity;
            if (thwompModel->animationTimeRemaining < 0.0f) {
                thwompModel->animationTimeRemaining = 0.0f;
            }
            if ((thwompModel->motionIntensity > thwompModel->frameLength) && (thwompModel->motionIntensity >= 0.0f)) {
                if (thwompModel->motionIntensity >= 13.200000000000001) {
                    soundEffectId = HuAudFXPlay(thwompModelResourceIds[2][modelIndex]);
                }
                else if (thwompModel->motionIntensity >= 6.6000000000000005) {
                    soundEffectId = HuAudFXPlay(thwompModelResourceIds[1][modelIndex]);
                }
                else {
                    soundEffectId = HuAudFXPlay(thwompModelResourceIds[0][modelIndex]);
                }
                pitchScale = thwompModel->animationTimeRemaining / 1000.0f;
                if (soundEffectId != -1) {
                    HuAudFXPitchSet(soundEffectId, (s32)(8191.0f * (1.0f - pitchScale)));
                }
            }
        }
        else {
            thwompModel->motionIntensity = 0.0f;
        }
        thwompModel->frameLength = thwompModel->motionIntensity;
        if (thwompModel->animationFrame != 0) {
            if (++thwompModel->animationFrameCounter >= thwompModel->animationFrame) {
                thwompModel->animationFrame = thwompModel->animationFrameCounter = 0;
                thwompModel->animationCycleProgress = 0.0f;
            }
        }
        if ((thwompModel->actionTimer != thwompModel->effectTimer) || (thwompModel->lightElevationAngle != thwompModel->animationBlend)) {
            vector.x = sind(thwompModel->lightElevationAngle) * cosd(thwompModel->actionTimer);
            vector.z = cosd(thwompModel->lightElevationAngle) * cosd(thwompModel->actionTimer);
            vector.y = sind(thwompModel->actionTimer);
            VECNormalize(&vector, &vector);
            light = &Hu3DLocalLight[model->lLightId[0]];
            light->pos.x = 1000.0f * vector.x;
            light->pos.y = 1000.0f * vector.y;
            light->pos.z = 1000.0f * vector.z;
            light->dir.x = vector.x;
            light->dir.y = vector.y;
            light->dir.z = vector.z;
            thwompModel->effectTimer = thwompModel->actionTimer;
            thwompModel->animationBlend = thwompModel->lightElevationAngle;
        }
        thwompModel->movementSpeed = 0.0f;
    }
}

void DrawThwompShadow(HU3DMODEL *model, Mtx matrix)
{
    s32 unusedDisplayListBytes;
    s32 unusedVertexCount;
    Mtx matrix2;
    Vec vector;
    Vec vector2;
    float horizontalVelocity;
    float verticalOffset;
    float bounceOffset;
    float shadowScale;
    float motionProgress;
    float rotationX;
    float rotationY;
    float rotationZ;
    float particleScale;

    float worldZ;
    float worldX;
    float worldY;
    float distanceSquared;
    float averageX;
    float averageWeight;
    float averageZ;
    float effectDisplacement;
    float animationProgress;
    float motionStrength;

    M425ThwompModelWork *thwompModel;
    s32 vertexIndex;
    s32 motionSlot;
    s16 activeMotionCount;
    s32 particleCount;

    thwompModel = model->hookData;
    if (thwompModel->animationFrame != 0) {
        effectDisplacement = (float)thwompModel->animationFrameCounter / thwompModel->animationFrame;
        thwompModel->animationCycleProgress = 0.30000001192092896 * cosd((90.0f * effectDisplacement));
        bounceOffset = thwompModel->animationCycleProgress * (0.5 * sind((720.0f * effectDisplacement)));
    }
    else {
        bounceOffset = 0.0f;
    }
    if (thwompModel->animationTimeTotal != 0.0f) {
        animationProgress = thwompModel->animationTimeRemaining / thwompModel->animationTimeTotal;
    }
    else {
        animationProgress = 1.0f;
    }
    model->modelScale.y = thwompModel->modelScale = (0.100000024f + (0.9f * animationProgress)) - bounceOffset;
    if (thwompModel->modelScale > 0.3f) {
        shadowScale = 1.0f - (-0.39999998f * ((thwompModel->modelScale - 0.3f) / 0.7f));
    }
    else {
        shadowScale = 0.6f * (thwompModel->modelScale / 0.3f);
    }
    if (thwompModel->modelScale != 0.0f) {
        shadowScale /= thwompModel->modelScale;
    }
    else {
        shadowScale = 0.0f;
    }
    if (thwompModel->animationTimeRemaining < 0.0f) {
        shadowScale = 0.0f;
    }
    if (animationProgress <= 0.3f) {
        effectDisplacement = animationProgress / 0.3f;
        thwompModel->actionTimer = 222.0f + (80.0f * effectDisplacement);
    }
    motionStrength = 100.0f * thwompModel->motionTimer;
    if (animationProgress > 0.85f) {
        motionStrength += 100.0f * (4.0f * (animationProgress - 0.85f));
    }
    if (motionStrength > 150.0f) {
        motionStrength = 150.0f;
    }
    motionProgress = 1.0f - animationProgress;
    if (motionStrength >= 0.0f) {
        for (vertexIndex = 0; vertexIndex < thwompModel->vertexCount; vertexIndex++) {
            if (thwompModel->positionBuffer[vertexIndex] == -1) {
                thwompModel->positionTarget[vertexIndex].x = thwompModel->positionStart[vertexIndex].x + (motionStrength * thwompModel->positionOutput[vertexIndex].x);
                thwompModel->positionTarget[vertexIndex].y = thwompModel->positionStart[vertexIndex].y + (motionStrength * thwompModel->positionOutput[vertexIndex].y);
                thwompModel->positionTarget[vertexIndex].z = thwompModel->positionStart[vertexIndex].z + (motionStrength * thwompModel->positionOutput[vertexIndex].z);
            }
        }
    }
    else {
        memcpy(thwompModel->positionTarget, thwompModel->positionStart, thwompModel->vertexCount * 0xC);
    }
    thwompModel->motionTimer = 0.0f;
    for (vertexIndex = 0; vertexIndex < thwompModel->vertexCount; vertexIndex++) {
        if (!(animationProgress >= 1.0f) && (thwompModel->displayListIndices[vertexIndex][2] != 0)) {
            switch (thwompModel->displayListIndices[vertexIndex][2]) {
                case 1:
                    thwompModel->positionTarget[vertexIndex].x += motionProgress * thwompModel->rotationWork[vertexIndex].x;
                    thwompModel->positionTarget[vertexIndex].y += motionProgress * thwompModel->rotationWork[vertexIndex].y;
                    thwompModel->positionTarget[vertexIndex].z += motionProgress * thwompModel->rotationWork[vertexIndex].z;
                    break;
                case 2:
                    effectDisplacement = thwompDataBuffer[thwompModel->displayListIndices[vertexIndex][0] + (thwompModel->displayListIndices[vertexIndex][1] << 8)];
                    effectDisplacement = 80.0f * (0.003921569f * (255.0f - effectDisplacement) * motionProgress);
                    thwompModel->positionTarget[vertexIndex].x += effectDisplacement * thwompModel->rotationWork[vertexIndex].x;
                    thwompModel->positionTarget[vertexIndex].y += effectDisplacement * thwompModel->rotationWork[vertexIndex].y;
                    thwompModel->positionTarget[vertexIndex].z += effectDisplacement * thwompModel->rotationWork[vertexIndex].z;
                    break;
            }
        }
    }
    for (motionSlot = 0, activeMotionCount = 0, averageX = averageWeight = averageZ = 0.0f; motionSlot < 6; motionSlot++) {
        if ((thwompModel->activeMotionTimers[motionSlot] > 0.0f) || (thwompModel->activeMotionIds[motionSlot] != -1)) {
            averageX += thwompModel->positionTarget[thwompModel->activeMotionIds[motionSlot]].x;
            averageZ += thwompModel->positionTarget[thwompModel->activeMotionIds[motionSlot]].z;
            averageWeight += thwompModel->activeMotionTimers[motionSlot];
            activeMotionCount++;
        }
    }
    if (activeMotionCount >= 2) {
        averageX /= activeMotionCount;
        averageZ /= activeMotionCount;
        averageWeight /= activeMotionCount;
        mtxRot(matrix2, 0.0f, thwompModel->rotationAngle, 0.0f);
        worldX = thwompModel->worldPosition.x + ((matrix2[0][0] * averageX) + (matrix2[0][2] * averageZ));
        worldZ = thwompModel->worldPosition.z + ((matrix2[2][0] * averageX) + (matrix2[2][2] * averageZ));
        CalculateThwompVertexPosition(thwompModel->animationIndex, 2, worldX, worldZ);
        thwompModel->modelCount = thwompModel->frameCount;
        thwompModel->activeMotionTimers[5] = averageWeight;
        thwompModel->animationIds[5] = 0;
    }
    else {
        thwompModel->modelCount = -1;
        thwompModel->activeMotionTimers[5] = 0.0f;
        thwompModel->animationIds[5] = 0;
    }
    for (motionSlot = 0; motionSlot < 6; motionSlot++) {
        if ((thwompModel->activeMotionTimers[motionSlot] <= 0.0f) || (thwompModel->activeMotionIds[motionSlot] == -1)) {
            thwompModel->activeMotionIds[motionSlot] = -1;
            thwompModel->activeMotionTimers[motionSlot] = 0.0f;
            thwompModel->animationIds[motionSlot] = 0;
        }
        else {
            effectDisplacement = 150.0f * thwompModel->activeMotionTimers[motionSlot] * shadowScale;
            verticalOffset = 50.0f * thwompModel->activeMotionTimers[motionSlot];
            if (verticalOffset > 15.000001f) {
                verticalOffset = 15.000001f;
            }
            averageX = thwompModel->positionTarget[thwompModel->activeMotionIds[motionSlot]].x;
            averageWeight = thwompModel->positionTarget[thwompModel->activeMotionIds[motionSlot]].y;
            averageZ = thwompModel->positionTarget[thwompModel->activeMotionIds[motionSlot]].z;
            if (motionSlot != 5) {
                for (vertexIndex = 0; vertexIndex < thwompModel->vertexCount; vertexIndex++) {
                    if (thwompModel->positionBuffer[vertexIndex] == -1) {
                        worldX = thwompModel->positionTarget[vertexIndex].x - averageX;
                        worldY = thwompModel->modelScale * (thwompModel->positionTarget[vertexIndex].y - averageWeight);
                        worldZ = 0.425f * (thwompModel->positionTarget[vertexIndex].z - averageZ);
                        distanceSquared = (worldZ * worldZ) + ((worldX * worldX) + (worldY * worldY));
                        if (distanceSquared < 10000.0f) {
                            distanceSquared = sqrtf((worldZ * worldZ) + ((worldX * worldX) + (worldY * worldY)));
                            distanceSquared = distanceSquared * 0.01f;
                            thwompModel->positionTarget[vertexIndex].y -= (1.0f - distanceSquared) * effectDisplacement;
                            if (thwompModel->positionTarget[vertexIndex].y < 1.0f) {
                                thwompModel->positionTarget[vertexIndex].y = 1.0f;
                            }
                        }
                        if (thwompModel->stateBuffer[vertexIndex] != 0) {
                            worldZ = thwompModel->positionTarget[vertexIndex].z - averageZ;
                            if (!(fabs(worldZ) > 100.0)) {
                                distanceSquared = sqrtf((worldX * worldX) + (worldZ * worldZ));
                                if (distanceSquared < 200.0f) {
                                    distanceSquared = distanceSquared * 0.005f;
                                    thwompModel->positionTarget[vertexIndex].y += (verticalOffset * sind((90.0f * distanceSquared))); // is this right?
                                }
                                else if (distanceSquared < 400.0f) {
                                    distanceSquared -= 200.0f;
                                    distanceSquared = distanceSquared * 0.005f;
                                    thwompModel->positionTarget[vertexIndex].y += (verticalOffset * cosd((90.0f * distanceSquared)));
                                }
                            }
                        }
                    }
                }
            }
            else {
                for (vertexIndex = 0; vertexIndex < thwompModel->vertexCount; vertexIndex++) {
                    if ((thwompModel->positionBuffer[vertexIndex] == -1) && ((thwompModel->activeVertexIndices[0] != vertexIndex) || (thwompModel->activeVertexIndices[1] != vertexIndex))) {
                        worldX = thwompModel->positionTarget[vertexIndex].x - averageX;
                        worldY = thwompModel->modelScale * (thwompModel->positionTarget[vertexIndex].y - averageWeight);
                        worldZ = 0.425f * (thwompModel->positionTarget[vertexIndex].z - averageZ);
                        distanceSquared = (worldZ * worldZ) + ((worldX * worldX) + (worldY * worldY));
                        if (distanceSquared < 10000.0f) {
                            distanceSquared = sqrtf((worldZ * worldZ) + ((worldX * worldX) + (worldY * worldY)));
                            distanceSquared = distanceSquared * 0.01f;
                            thwompModel->positionTarget[vertexIndex].y -= (1.0f - distanceSquared) * effectDisplacement;
                            if (thwompModel->positionTarget[vertexIndex].y < 1.0f) {
                                thwompModel->positionTarget[vertexIndex].y = 1.0f;
                            }
                        }
                    }
                }
            }
            if (motionSlot < 5) {
                if ((thwompModel->activeMotionTimers[motionSlot] -= 0.075f) <= 0.0f) {
                    thwompModel->activeMotionTimers[motionSlot] = 0.0f;
                    thwompModel->activeMotionIds[motionSlot] = -1;
                    thwompModel->animationIds[motionSlot] = 0;
                }
            }
            else {
                thwompModel->activeMotionTimers[motionSlot] = 0.0f;
                thwompModel->activeMotionIds[motionSlot] = -1;
                thwompModel->animationIds[motionSlot] = 0;
            }
        }
    }
    thwompModel->motionSlotCount = 0;
    memcpy(thwompModel->rotationOutput, &thwompModel->positionTarget[thwompModel->vertexIndexOffsets[0]], thwompModel->vertexIndexCounts[0] * 0xC);
    thwompModel->workState = 0;
    BuildThwompModelGeometry(thwompModel, model);
    if ((thwompModel->frameStart != -1) && (thwompModel->referenceVertexIndex != -1)) {
        mtxRot(matrix2, 0.0f, thwompModel->rotationAngle, 0.0f);
        matrix2[1][0] *= thwompModel->modelScale;
        matrix2[1][1] *= thwompModel->modelScale;
        matrix2[1][2] *= thwompModel->modelScale;
        worldX = thwompModel->positionTarget[thwompModel->referenceVertexIndex].x;
        worldY = thwompModel->positionTarget[thwompModel->referenceVertexIndex].y;
        worldZ = thwompModel->positionTarget[thwompModel->referenceVertexIndex].z;
        averageX = thwompModel->worldPosition.x + ((matrix2[0][2] * worldZ) + ((matrix2[0][0] * worldX) + (matrix2[0][1] * worldY)));
        averageWeight = thwompModel->worldPosition.y + ((matrix2[1][2] * worldZ) + ((matrix2[1][0] * worldX) + (matrix2[1][1] * worldY)));
        averageZ = thwompModel->worldPosition.z + ((matrix2[2][2] * worldZ) + ((matrix2[2][0] * worldX) + (matrix2[2][1] * worldY)));
        Hu3DModelPosSet(thwompModel->frameStart, averageX, averageWeight, averageZ);
        worldX = thwompModel->positionWork[thwompModel->referenceVertexIndex].x;
        worldY = thwompModel->positionWork[thwompModel->referenceVertexIndex].y;
        worldZ = thwompModel->positionWork[thwompModel->referenceVertexIndex].z;
        vector.x = averageX + (20.0f * worldX);
        vector.y = averageWeight + (20.0f * worldY);
        vector.z = averageZ + (20.0f * worldZ);
        horizontalVelocity = sqrtf((worldX * worldX) + (worldZ * worldZ));
        averageX = rotationX = atan2d(horizontalVelocity, worldY);
        averageWeight = rotationY = atan2d(worldX, worldZ);
        averageZ = rotationZ = 0.0f;
        Hu3DModelRotSet(thwompModel->frameStart, averageX, averageWeight, averageZ);
        vertexIndex = (s32)(10.0f * thwompModel->motionIntensity) / 40;
        if (vertexIndex > 0x10) {
            vertexIndex = 0x10;
        }
        for (particleCount = 0; particleCount < vertexIndex; particleCount++) {
            averageX = rotationX;
            averageWeight = rotationY;
            averageZ = rotationZ;
            averageX += 0.5f * ((s32)frand() % 48);
            worldX = sind(averageWeight) * sind(averageX);
            worldZ = cosd(averageWeight) * sind(averageX);
            worldY = cosd(averageX);
            particleScale = 12.0f - ((s32)frand() % 6);
            vector2.x = worldX;
            vector2.y = worldY;
            vector2.z = worldZ;
            UpdateThwompEffect(&vector, &vector2, 50.0f + (5.0f * vertexIndex), particleScale);
        }
    }
    DCFlushRangeNoSync(thwompModel->positionTarget, thwompModel->vertexCount * 0xC);
    DCFlushRangeNoSync(thwompModel->positionWork, thwompModel->vertexCount * 0xC);
    PPCSync();
}

void DrawThwompModel(HU3DMODEL *model, Mtx matrix)
{
    Mtx matrix2;
    ROMtx rotationMatrix;
    GXColor color;
    M425ThwompModelWork *thwompModel;
    s32 vertexIndex;
    s16 drawState;

    thwompModel = model->hookData;
    GXLoadPosMtxImm(matrix, 0);
    MTXInvXpose(matrix, matrix2);
    GXLoadNrmMtxImm(matrix2, 0);
    drawState = ConfigureThwompLight(model, Hu3DCameraMtx, Hu3DCameraMtxXPose, 4.0f);
    if (omPauseChk() == 0) {
        DrawThwompShadow(model, matrix);
    }
    model->modelScale.y = thwompModel->modelScale;
    memcpy(matrix2, thwompModelMatrix, sizeof(thwompModelMatrix));
    mtxRotCat(&matrix2[0], CRot.x, CRot.y + thwompModel->rotationAngle, 180.0f);
    GXLoadTexMtxImm(matrix2, 0x36, GX_MTX2x4);
    color.a = thwompAlpha;
    GXSetTevColor(GX_TEVREG2, color);
    HuSprTexLoad(thwompIdleAnimation, 0, 0, GX_CLAMP, GX_CLAMP, GX_LINEAR);
    HuSprTexLoad(thwompHitAnimation, 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetNumChans(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_NRM, 0x36, GX_FALSE, 0x7D);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP1, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_TEXC, GX_CC_A2, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    thwompEffectColor.r = thwompModel->effectColor.r;
    thwompEffectColor.g = thwompModel->effectColor.g;
    thwompEffectColor.b = thwompModel->effectColor.b;
    thwompEffectColor.a = thwompModel->effectColor.a;
    GXSetChanAmbColor(GX_COLOR0A0, thwompEffectColor);
    GXSetChanMatColor(GX_COLOR0A0, thwompModelColor);
    GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, GX_SRC_REG, drawState, GX_DF_CLAMP, GX_AF_SPEC);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZMode(1, GX_LEQUAL, GX_FALSE);
    GXSetZCompLoc(0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    if (HmfInverseMtxF3X3(matrix, matrix2) == 0) {
        MTXIdentity(matrix2);
    }
    PSMTXReorder(matrix2, rotationMatrix);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, thwompModel->positionTarget, 0xC);
    GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_NRM, thwompModel->positionWork, 0xC);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, &thwompModel->effectColor, 4);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_CLR_RGBA, GX_RGBA6, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetArray(GX_VA_TEX0, thwompModel->textureCoordinates, 8);
    for (vertexIndex = 0; vertexIndex < thwompModel->surfaceCount; vertexIndex++) {
        if (thwompModel->displayLists[vertexIndex] && thwompModel->displayListSizes[vertexIndex] != 0 && vertexIndex != 1) {
            GXCallDisplayList(thwompModel->displayLists[vertexIndex], thwompModel->displayListSizes[vertexIndex]);
        }
    }
    GXSetNumTevStages(2);
    GXSetNumTexGens(2);
    GXSetNumChans(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, GX_FALSE, 0x7D);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_NRM, 0x36, GX_FALSE, 0x7D);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_TEXC, GX_CC_TEXA, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevOp(GX_TEVSTAGE1, GX_REPLACE);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_CPREV, GX_CC_TEXC, GX_CC_A2, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_APREV, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    thwompEffectColor.r = thwompModel->effectColor.r;
    thwompEffectColor.g = thwompModel->effectColor.g;
    thwompEffectColor.b = thwompModel->effectColor.b;
    thwompEffectColor.a = thwompModel->effectColor.a;
    GXSetChanAmbColor(GX_COLOR0A0, thwompEffectColor);
    GXSetChanMatColor(GX_COLOR0A0, thwompModelColor);
    GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, GX_SRC_REG, drawState, GX_DF_CLAMP, GX_AF_SPEC);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    GXSetZCompLoc(0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0);
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetArray(GX_VA_POS, thwompModel->positionTarget, 0xC);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_TEX_S, GX_RGBA6, 0);
    GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
    GXSetArray(GX_VA_NRM, thwompModel->positionWork, 0xC);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_TEX_ST, GX_RGBA8, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetArray(GX_VA_CLR0, &thwompModel->effectColor, 4);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetArray(GX_VA_TEX0, thwompModel->textureCoordinates, 8);
    if (thwompModel->displayLists[1] && (thwompModel->displayListSizes[1] != 0)) {
        GXCallDisplayList(thwompModel->displayLists[1], thwompModel->displayListSizes[1]);
    }
}

s32 CalculateThwompCollision(s32 modelIndex, s32 vertexCacheSlot, Vec *position, Vec *movement, Vec *nextPosition, s32 collisionEffectId)
{
    float impactStrength;
    float animationProgress;
    M425ThwompModelWork *thwompModel;
    s32 motionSlotIndex;
    HU3DMODEL *model;

    if (!thwompObject) {
        return;
    }
    model = &Hu3DData[thwompObject->mdlId[modelIndex]];
    thwompModel = model->hookData;
    if (thwompModel->animationTimeTotal != 0.0f) {
        animationProgress = thwompModel->animationTimeRemaining / thwompModel->animationTimeTotal;
    }
    else {
        animationProgress = 1.0f;
    }
    thwompModel->collisionState = CalculateThwompVertexPosition(modelIndex, vertexCacheSlot, position->x, position->z);
    nextPosition->x = position->x + movement->x;
    nextPosition->y = position->y + movement->y;
    nextPosition->z = position->z + movement->z;
    if ((nextPosition->y < thwompModel->frameCount) && (movement->y < 0.0f) && (thwompModel->frameCount >= 0)) {
        impactStrength = -movement->y;
        if (impactStrength > 50.0f) {
            impactStrength = 50.0f;
        }
        else if (impactStrength < 10.0f) {
            impactStrength = 0.0f;
        }
        if ((impactStrength > 0.0f) && (collisionEffectId != 0)) {
            impactStrength = (impactStrength - 10.0f) / 40.0f;
#if VERSION_PAL
            thwompModel->movementSpeed += impactStrength * 0.8333333f;
#else
            thwompModel->movementSpeed += impactStrength;
#endif
        }
        impactStrength = -movement->y;
        impactStrength = impactStrength * 0.02f;
        if (thwompModel->frameCount >= 0) {
            for (motionSlotIndex = 0; motionSlotIndex < thwompModel->motionSlotCount; motionSlotIndex++) {
                if (thwompModel->motionStates[motionSlotIndex] == thwompModel->frameCount) {
                    break;
                }
            }
            if (motionSlotIndex >= thwompModel->motionSlotCount) {
                motionSlotIndex = thwompModel->motionSlotCount;
            }
            if (motionSlotIndex < 6) {
                thwompModel->motionStates[motionSlotIndex] = thwompModel->frameCount;
                if (thwompModel->motionTimers[motionSlotIndex] < impactStrength) {
                    thwompModel->motionTimers[motionSlotIndex] = impactStrength;
                    thwompModel->animationTextureIds[motionSlotIndex] = collisionEffectId;
                }
                thwompModel->motionSlotCount++;
            }
        }
        nextPosition->y = thwompModel->collisionState - (150.0f * (animationProgress * impactStrength));
        if (nextPosition->y < 1.0f) {
            nextPosition->y = 1.0f;
        }
        return 1;
    }
    if (nextPosition->y < thwompModel->collisionState) {
        nextPosition->y = thwompModel->collisionState;
    }
    return 0;
}

float GetThwompModelId(s32 modelIndex)
{
    HU3DMODEL *model;
    M425ThwompModelWork *thwompModel;

    if (!thwompObject) {
        return;
    }
    model = &Hu3DData[thwompObject->mdlId[modelIndex]];
    thwompModel = model->hookData;
    return thwompModel->animationTimeRemaining;
}

void SetThwompModelState(s32 modelIndex, float animationTime)
{
    HU3DMODEL *model;
    M425ThwompModelWork *thwompModel;

    if (thwompObject) {
        model = &Hu3DData[thwompObject->mdlId[modelIndex]];
        thwompModel = model->hookData;
        thwompModel->animationTimeRemaining = animationTime;
    }
}

void UpdateThwompModelState(s32 modelIndex)
{
    M425ThwompModelWork *thwompModel;
    HU3DMODEL *model;

    if (thwompObject) {
        model = &Hu3DData[thwompObject->mdlId[modelIndex]];
        thwompModel = model->hookData;
        if (thwompModel->animationFrame == 0) {
            thwompModel->animationFrameCounter = 0;
            thwompModel->animationFrame = 0x28;
            thwompModel->animationCycleProgress = 0.3f;
        }
    }
}

void ResetThwompModel(s32 modelVariant)
{
    M425ThwompModelWork *thwompModel;
    s32 modelIndex;
    HU3DMODEL *model;

    if (thwompObject) {
        for (modelIndex = 0; modelIndex < 2; modelIndex++) {
            model = &Hu3DData[thwompObject->mdlId[modelIndex]];
            thwompModel = model->hookData;
            if (modelVariant != 0) {
                thwompModel->vertexCount = thwompModel->vertexIndexOffsets[0x18] + thwompModel->vertexIndexCounts[24];
                thwompModel->surfaceCount = 0x19;
            }
            else {
                thwompModel->vertexCount = thwompModel->vertexIndexOffsets[0x17] + thwompModel->vertexIndexCounts[23];
                thwompModel->surfaceCount = 0x18;
            }
        }
    }
}

float CalculateThwompVertexPosition(s32 modelIndex, s32 vertexCacheSlot, float worldX, float worldZ)
{
    Mtx matrix;
    float transformedX;
    float transformedZ;
    float distanceSquared;
    float nearestDistanceSquared;
    float transformedY;
    M425ThwompModelWork *thwompModel;
    Vec *vector;
    s32 vertexIndex;
    s32 nearestVertexIndex;
    HU3DMODEL *model;

    if (!thwompObject) {
        return 0.0f;
    }
    model = &Hu3DData[thwompObject->mdlId[modelIndex]];
    thwompModel = model->hookData;
    if ((vertexCacheSlot >= 0) && (vertexCacheSlot <= 2) && (thwompModel->activeVertexIndices[vertexCacheSlot] != -1)) {
        nearestVertexIndex = thwompModel->activeVertexIndices[vertexCacheSlot];
        thwompModel->frameCount = nearestVertexIndex;
        if (thwompModel->workState == 0) {
            return thwompModel->modelScale * thwompModel->rotationOutput[nearestVertexIndex].y;
        }
        return thwompModel->rotationOutput[nearestVertexIndex].y;
    }
    if (thwompModel->workState == 0) {
        mtxRot(matrix, 0.0f, thwompModel->rotationAngle, 0.0f);
        matrix[1][0] *= thwompModel->modelScale;
        matrix[1][1] *= thwompModel->modelScale;
        matrix[1][2] *= thwompModel->modelScale;
        for (vertexIndex = 0, vector = thwompModel->rotationOutput; vertexIndex < thwompModel->vertexIndexCounts[0]; vertexIndex++, vector++) {
            transformedX = (matrix[0][2] * vector->z) + ((matrix[0][0] * vector->x) + (matrix[0][1] * vector->y));
            transformedY = (matrix[1][2] * vector->z) + ((matrix[1][0] * vector->x) + (matrix[1][1] * vector->y));
            transformedZ = (matrix[2][2] * vector->z) + ((matrix[2][0] * vector->x) + (matrix[2][1] * vector->y));
            vector->x = transformedX;
            vector->y = transformedY;
            vector->z = transformedZ;
        }
        thwompModel->workState = 1;
    }
    worldX -= thwompModel->worldPosition.x;
    worldZ -= thwompModel->worldPosition.z;
    nearestDistanceSquared = 10000.0f;
    nearestVertexIndex = -1;
    for (vertexIndex = 0, vector = thwompModel->rotationOutput; vertexIndex < thwompModel->vertexIndexCounts[0]; vertexIndex++, vector++) {
        transformedX = worldX - vector->x;
        transformedZ = worldZ - vector->z;
        if (!(fabs(transformedX) > 100.0)) {
            if (!(fabs(transformedZ) > 100.0)) {
                distanceSquared = (transformedX * transformedX) + (transformedZ * transformedZ);
                if (distanceSquared < nearestDistanceSquared) {
                    nearestVertexIndex = vertexIndex;
                    nearestDistanceSquared = distanceSquared;
                }
            }
        }
    }
    (void)vertexIndex;
    thwompModel->frameCount = nearestVertexIndex;
    if ((vertexCacheSlot >= 0) && (vertexCacheSlot <= 2)) {
        thwompModel->activeVertexIndices[vertexCacheSlot] = nearestVertexIndex;
    }
    if (nearestVertexIndex >= 0) {
        return thwompModel->rotationOutput[nearestVertexIndex].y;
    }
    return 0.0f;
}

void InitializeThwompSprite(void)
{
    thwompEffectAnimation = HuSprAnimRead(HuDataReadNum(0x38000E, 0x10000000));
    thwompSpriteId = LoadThwompAnimation(thwompEffectAnimation, 0x40);
    Hu3DModelLayerSet(thwompSpriteId, 6);
}

void CreateThwompEffect(void)
{
    M425ThwompAnimationWork *thwompAnimation;
    M425ThwompParticleWork *thwompParticle;
    s32 particleIndex;
    HU3DMODEL *model;

    model = &Hu3DData[thwompSpriteId];
    thwompParticle = model->hookData;
    for (thwompAnimation = thwompParticle->motionIds, particleIndex = 0; particleIndex < thwompParticle->collisionState; particleIndex++, thwompAnimation++) {
        if (!(thwompAnimation->lifetimeRemaining <= 0.0f)) {
            thwompAnimation->frameStart.x = thwompAnimation->frameEnd.x * thwompAnimation->frameLength;
            thwompAnimation->frameStart.y = thwompAnimation->frameEnd.y * thwompAnimation->frameLength;
            thwompAnimation->frameStart.z = thwompAnimation->frameEnd.z * thwompAnimation->frameLength;
            thwompAnimation->frameLength *= 0.95f;
            VECAdd(&thwompAnimation->worldPosition, &thwompAnimation->frameStart, &thwompAnimation->worldPosition);
            if (++thwompAnimation->animationFrameIndex >= 0xF) {
                thwompAnimation->animationFrameIndex = 0;
                thwompAnimation->lifetimeRemaining = 0.0f;
            }
        }
    }
}

void CleanupThwompSprite(void)
{
    HuSprAnimKill(thwompEffectAnimation);
    thwompEffectAnimation = NULL;
}

s32 UpdateThwompEffect(Vec *vector, Vec *vector2, float effectLifetime, float effectScale)
{
    M425ThwompAnimationWork *thwompAnimation;
    M425ThwompParticleWork *thwompParticle;
    s32 effectSlot;
    HU3DMODEL *model;

    model = &Hu3DData[thwompSpriteId];
    thwompParticle = model->hookData;
    for (thwompAnimation = thwompParticle->motionIds, effectSlot = 0; effectSlot < thwompParticle->collisionState; effectSlot++, thwompAnimation++) {
        if (thwompAnimation->lifetimeRemaining <= 0.0f) {
            break;
        }
    }
    if (effectSlot >= thwompParticle->collisionState) {
        return -1;
    }
    thwompAnimation->worldPosition.x = vector->x;
    thwompAnimation->worldPosition.y = vector->y;
    thwompAnimation->worldPosition.z = vector->z;
    thwompAnimation->frameEnd.x = vector2->x;
    thwompAnimation->frameEnd.y = vector2->y;
    thwompAnimation->frameEnd.z = vector2->z;
    thwompAnimation->frameStart.x = vector2->x * effectScale;
    thwompAnimation->frameStart.y = vector2->y * effectScale;
    thwompAnimation->frameStart.z = vector2->z * effectScale;
    thwompAnimation->frameLength = effectScale;
    thwompAnimation->lifetimeRemaining = effectLifetime;
    thwompAnimation->animationFrame = (s32)frand() % 360;
    thwompAnimation->motionIds.a = 0x40;
    thwompAnimation->animationFrameIndex = 0;
    return effectSlot;
}

s16 ConfigureThwompLight(HU3DMODEL *model, Mtx matrix, Mtx matrix2, float specularStrength)
{
    s16 lightIndex;
    s16 lightMask;
    HU3DLIGHT *light;
    s16 enabledLightMask;

    enabledLightMask = 0;
    lightMask = 1;
    for (light = Hu3DGlobalLight, lightIndex = 0; lightIndex < 8; lightIndex++, light++) {
        if (light->type != -1) {
            lightMask <<= 1;
        }
    }
    if ((model->attr & 0x1000) != 0) {
        for (lightIndex = 0; lightIndex < 8; lightIndex++) {
            if (model->lLightId[lightIndex] != -1) {
                light = &Hu3DLocalLight[model->lLightId[lightIndex]];
                InitializeLightParameters(model, light, lightMask, matrix2, model, specularStrength);
                enabledLightMask |= lightMask;
                lightMask <<= 1;
            }
        }
    }
    return enabledLightMask;
}

void InitializeLightParameters(HU3DMODEL *model, HU3DLIGHT *light, s16 lightMask, Mtx matrix, Mtx matrix2, float specularStrength)
{
    switch ((u8)light->type) {
        case 0:
            GXInitLightAttn(&thwompLight, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
            GXInitLightSpot(&thwompLight, light->cutoff, light->func);
            break;
        case 1:
            GXInitLightAttn(&thwompLight, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
            VECScale(&light->dir, &light->pos, -100000.0f);
            break;
        case 2:
            GXInitLightAttn(&thwompLight, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            GXInitLightDistAttn(&thwompLight, light->cutoff, light->brightness, light->func);
            break;
    }
    if ((light->type & 0x8000) != 0) {
        MTXMultVec(matrix, &light->dir, &thwompLightPosition);
        MTXMultVec(matrix2, &light->pos, &thwompLightDirection);
        GXInitLightPos(&thwompLight, thwompLightDirection.x, thwompLightDirection.y, thwompLightDirection.z);
    }
    else {
        GXInitLightPos(&thwompLight, light->pos.x, light->pos.y, light->pos.z);
        thwompLightPosition = light->dir;
    }
    if (specularStrength == 0.0f) {
        GXInitLightDir(&thwompLight, thwompLightPosition.x, thwompLightPosition.y, thwompLightPosition.z);
    }
    else {
        GXInitSpecularDir(&thwompLight, thwompLightPosition.x, thwompLightPosition.y, thwompLightPosition.z);
        GXInitLightAttn(&thwompLight, 0.0f, 0.0f, 1.0f, specularStrength / 2, 0.0f, 1.0f - (specularStrength / 2));
    }
    GXInitLightColor(&thwompLight, light->color);
    GXLoadLightObjImm(&thwompLight, lightMask);
}

s16 LoadThwompAnimation(ANIMDATA *animation, s16 particleCount)
{
    M425ThwompParticleWork *thwompParticle;
    s16 particleIndex;
    Vec2f *textureCoordinate;
    M425ThwompAnimationWork *thwompAnimation;
    HU3DMODEL *model;
    Vec *vector;
    void *displayListBuffer;
    s16 textureRow;
    void *displayListCommandBuffer;

    textureRow = Hu3DHookFuncCreate(DrawThwompEffect);
    model = &Hu3DData[textureRow];
    thwompParticle = HuMemDirectMallocNum(HEAP_MODEL, sizeof(M425ThwompParticleWork), model->mallocNo);
    model->hookData = thwompParticle;
    thwompParticle->animationTimer = animation;
    animation->useNum++;
    thwompParticle->collisionState = particleCount;
    thwompParticle->movementSpeed = 0;
    thwompParticle->updateHook = 0;
    thwompParticle->elapsedFrameCount = 0;
    thwompParticle->thwompFlags = 0;
    thwompParticle->frameLimit = 0;
    thwompParticle->animationIndex = thwompParticle->animationstateAt02 = 0;
    thwompAnimation = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(M425ThwompAnimationWork), model->mallocNo);
    thwompParticle->motionIds = thwompAnimation;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, thwompAnimation++) {
        thwompAnimation->lifetimeRemaining = 0.0f;
        thwompAnimation->animationFrame = 0.0f;
        thwompAnimation->worldPosition.x = (s32)((frand() & 0x7F) - 0x40) * 0x14;
        thwompAnimation->worldPosition.y = (s32)((frand() & 0x7F) - 0x40) * 0x1E;
        thwompAnimation->worldPosition.z = (s32)((frand() & 0x7F) - 0x40) * 0x14;
        thwompAnimation->motionIds.r = thwompAnimation->motionIds.g = thwompAnimation->motionIds.b = thwompAnimation->motionIds.a = 0xFF;
        thwompAnimation->animationFrameIndex = 0;
    }
    vector = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec) * 4, model->mallocNo);
    thwompParticle->animationData = vector;
    for (particleIndex = 0; particleIndex < (particleCount * 4); particleIndex++, vector++) {
        vector->x = vector->y = vector->z = 0.0f;
    }
    textureCoordinate = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec2f) * 4, model->mallocNo);
    thwompParticle->position = textureCoordinate;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++) {
        textureCoordinate->x = 0.0f;
        textureCoordinate->y = 0.0f;
        textureCoordinate++;
        textureCoordinate->x = 1.0f;
        textureCoordinate->y = 0.0f;
        textureCoordinate++;
        textureCoordinate->x = 1.0f;
        textureCoordinate->y = 1.0f;
        textureCoordinate++;
        textureCoordinate->x = 0.0f;
        textureCoordinate->y = 1.0f;
        textureCoordinate++;
    }
    displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, 0x4000, model->mallocNo);
    displayListCommandBuffer = displayListBuffer;
    DCFlushRange(displayListBuffer, 0x4000);
    GXBeginDisplayList(displayListCommandBuffer, 0x4000);
    GXBegin(GX_QUADS, GX_VTXFMT0, particleCount * 4);
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++) {
        GXPosition1x16(particleIndex * 4);
        GXColor1x16(particleIndex);
        GXTexCoord1x16(particleIndex * 4);

        GXPosition1x16(particleIndex * 4 + 1);
        GXColor1x16(particleIndex);
        GXTexCoord1x16(particleIndex * 4 + 1);

        GXPosition1x16(particleIndex * 4 + 2);
        GXColor1x16(particleIndex);
        GXTexCoord1x16(particleIndex * 4 + 2);

        GXPosition1x16(particleIndex * 4 + 3);
        GXColor1x16(particleIndex);
        GXTexCoord1x16(particleIndex * 4 + 3);
    }
    thwompParticle->frameCount = GXEndDisplayList();
    thwompParticle->rotation = HuMemDirectMallocNum(HEAP_MODEL, thwompParticle->frameCount, model->mallocNo);
    memcpy(thwompParticle->rotation, displayListBuffer, thwompParticle->frameCount);
    DCFlushRange(thwompParticle->rotation, thwompParticle->frameCount);
    HuMemDirectFree(displayListBuffer);
    return textureRow;
}

void DrawThwompEffect(HU3DMODEL *model, Mtx matrix)
{
    Mtx matrix2;
    Mtx matrix3;
    Mtx matrix4;
    Vec vector4[4];
    Vec vector5[4];
    Vec vector6[4];
    ROMtx rotationMatrix;
    M425ThwompParticleWork *thwompParticle;
    Vec *vector;
    M425ThwompAnimationWork *thwompAnimation;
    Vec *vector2;
    Vec *vector3;
    Vec2f *textureCoordinate;
    s16 quadVertexIndex;
    s16 particleIndex;
    s16 textureFormat;
    s16 textureColumn;
    s16 textureRow;
    ThwompParticleUpdateHook particleUpdateHook;

    thwompParticle = model->hookData;
    if ((thwompParticle->animationFrame != GlobalCounter) || (shadowModelDrawF != 0)) {
        GXLoadPosMtxImm(matrix, 0);
        GXSetNumTevStages(1);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, GX_FALSE, 0x7D);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        if (shadowModelDrawF != 0) {
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ONE, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
            GXSetZMode(0, GX_LEQUAL, 0);
        }
        else {
            textureFormat = thwompParticle->animationTimer->bmp->dataFmt & 0xF;
            if ((textureFormat == 7) || (textureFormat == 8)) {
                GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
            }
            else {
                GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
            }
            if ((model->attr & 2) != 0) {
                GXSetZMode(1, GX_LEQUAL, GX_TRUE);
            }
            else {
                GXSetZMode(1, GX_LEQUAL, GX_FALSE);
            }
        }
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
        HuSprTexLoad(thwompParticle->animationTimer, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
        GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
        GXSetZCompLoc(0);
        switch (thwompParticle->movementSpeed) {
            case 0:
                GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
                break;
            case 1:
                GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
                break;
            case 2:
                GXSetBlendMode(GX_BM_BLEND, GX_BL_ZERO, GX_BL_INVDSTCLR, GX_LO_NOOP);
                break;
        }
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0);
        GXSetArray(GX_VA_POS, thwompParticle->animationData, 0xC);
        GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_TEX_ST, GX_RGBA8, 0);
        GXSetArray(GX_VA_CLR0, &thwompParticle->motionIds->motionIds, 0x44);
        GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_RGBA6, 0);
        GXSetArray(GX_VA_TEX0, thwompParticle->position, 8);
        if (HmfInverseMtxF3X3(matrix, matrix2) == 0) {
            MTXIdentity(matrix2);
        }
        PSMTXReorder(matrix2, &rotationMatrix[0]);
        if (thwompParticle->updateHook) {
            particleUpdateHook = thwompParticle->updateHook;
            particleUpdateHook(model, thwompParticle, matrix);
        }
        thwompAnimation = thwompParticle->motionIds;
        vector = thwompParticle->animationData;
        textureCoordinate = thwompParticle->position;
        PSMTXROMultVecArray(rotationMatrix, thwompQuadVertices, &vector6[0], 4);
        for (particleIndex = 0; particleIndex < thwompParticle->collisionState; particleIndex++, thwompAnimation++) {
            if (!thwompAnimation->lifetimeRemaining) {
                vector->x = vector->y = vector->z = 0.0f;
                vector++;
                vector->x = vector->y = vector->z = 0.0f;
                vector++;
                vector->x = vector->y = vector->z = 0.0f;
                vector++;
                vector->x = vector->y = vector->z = 0.0f;
                vector++;
            }
            else {
                if (!thwompAnimation->animationFrame) {
                    vector2 = &vector4[0];
                    vector3 = &vector6[0];
                    VECScale(vector3++, vector2, thwompAnimation->lifetimeRemaining);
                    VECAdd(vector2++, &thwompAnimation->worldPosition, vector++);
                    VECScale(vector3++, vector2, thwompAnimation->lifetimeRemaining);
                    VECAdd(vector2++, &thwompAnimation->worldPosition, vector++);
                    VECScale(vector3++, vector2, thwompAnimation->lifetimeRemaining);
                    VECAdd(vector2++, &thwompAnimation->worldPosition, vector++);
                    VECScale(vector3++, vector2, thwompAnimation->lifetimeRemaining);
                    VECAdd(vector2++, &thwompAnimation->worldPosition, vector++);
                }
                else {
                    VECScale(thwompQuadVertices, &vector4[0], thwompAnimation->lifetimeRemaining);
                    VECScale(&thwompQuadVertices[1], &vector4[1], thwompAnimation->lifetimeRemaining);
                    VECScale(&thwompQuadVertices[2], &vector4[2], thwompAnimation->lifetimeRemaining);
                    VECScale(&thwompQuadVertices[3], &vector4[3], thwompAnimation->lifetimeRemaining);
                    MTXRotRad(matrix4, 0x5A, thwompAnimation->animationFrame);
                    MTXConcat(matrix2, matrix4, matrix3);
                    MTXMultVecArray(matrix3, &vector4[0], &vector5[0], 4);
                    VECAdd(&vector5[0], &thwompAnimation->worldPosition, vector++);
                    VECAdd(&vector5[1], &thwompAnimation->worldPosition, vector++);
                    VECAdd(&vector5[2], &thwompAnimation->worldPosition, vector++);
                    VECAdd(&vector5[3], &thwompAnimation->worldPosition, vector++);
                }
                textureColumn = thwompAnimation->animationFrameIndex & 3;
                textureRow = (thwompAnimation->animationFrameIndex >> 2) & 3;
                for (quadVertexIndex = 0; quadVertexIndex < 4; quadVertexIndex++, textureCoordinate++) {
                    textureCoordinate->x = (0.25f * textureColumn) + thwompTextureCoordinates[quadVertexIndex].x;
                    textureCoordinate->y = (0.25f * textureRow) + thwompTextureCoordinates[quadVertexIndex].y;
                }
            }
        }
        DCFlushRangeNoSync(thwompParticle->animationData, thwompParticle->collisionState * 0xC * 4);
        DCFlushRangeNoSync(thwompParticle->position, thwompParticle->collisionState * 8 * 4);
        PPCSync();
        GXCallDisplayList(thwompParticle->rotation, thwompParticle->frameCount);
        if (shadowModelDrawF == 0) {
            if ((thwompParticle->thwompFlags & 2) == 0) {
                thwompParticle->elapsedFrameCount++;
            }
            if ((thwompParticle->frameLimit != 0) && (thwompParticle->frameLimit <= thwompParticle->elapsedFrameCount)) {
                if ((thwompParticle->thwompFlags & 1) != 0) {
                    thwompParticle->elapsedFrameCount = 0;
                }
                thwompParticle->elapsedFrameCount = thwompParticle->frameLimit;
            }
            thwompParticle->animationFrame = GlobalCounter;
        }
    }
}

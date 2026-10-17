#include "version.h"
#include "game/disp.h"
#define HUSPR_USE_OLD_DEFS
#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXStruct.h"
#include "dolphin/gx/GXVert.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/flag.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include "math.h"
#include "string.h"

#include "REL/m425Dll.h"

extern u8 texMtxTbl[]; // TODO maybe put in hsfdraw.h?

typedef struct M425DllWork {
    u8 objectIndex;
    u8 playerIndex;
    u8 playerGroupIndex;
    u8 objectVariant;
    u8 objectState;
    u8 effectState;
    u8 motionState;
    char reservedPlayerData[2];
    u8 controllerPort;
    u8 difficulty;
    u8 roundIndex;
    u8 objectPosition;
    char reservedActionData;
    s16 motionIndex;
    float targetPosition;
    float objectRotation;
    Vec objectScale;
    Vec objectProgress;
    Vec objectFrame;
    float effectWork[8];
    s32 collisionState;
    float movementSpeed;
    s32 blendFrame;
    s32 blendSpeed;
} M425DllWork; /* size = 0x6C */

typedef struct M425DllWork2 {
    char position[4];
    Vec objectState;
    Vec targetPosition;
    Vec objectFlags;
    Vec objectSpeed;
    float animationAttributes;
    float animationData;
    float effectWork;
    float positionWork;
    float animationMatrix;
    float effectState;
    char reservedAnimationData[0x18];
    float blendFrame;
    float blendSpeed;
    float animationIds;
    char reservedBlendData[4];
    s16 frameStart;
    s16 frameEnd;
} M425DllWork2; /* size = 0x78 */

typedef struct M425ObjectStateWork {
    s16 objectIndex;
    s16 animationState;
    s16 objectState;
    char reservedPlayerData[0x42];
} M425ObjectStateWork; /* size = 0x48 */

typedef struct M425AnimationObjectWork {
    ANIMDATA *objectIndex;
    s16 objectState;
    Vec objectModel;
    Mtx objectRotation;
    Vec2f *animationMatrix;
    s16 effectState;
} M425AnimationObjectWork; /* size = 0x4C */

typedef struct M425ObjectVisualWork {
    char position[0xC];
    s16 objectPosition;
    void *targetPosition;
    Vec *objectRotation;
    GXColor *objectScale;
    Vec *objectFlags;
    GXColor objectTimer;
    GXColor objectProgress;
    s16 objectSpeed;
    s16 animationFrame;
    M425AnimationObjectWork *objectDirection;
} M425ObjectVisualWork; /* size = 0x30 */

typedef struct M425EffectWork {
    /* 0x00 */ u8 objectIndex;
    /* 0x01 */ char pad01[1];
    /* 0x02 */ u8 effectMode;
    /* 0x03 */ u8 effectFrame;
    /* 0x04 */ float objectState;
    /* 0x08 */ Vec objectModel;
    /* 0x14 */ float objectRotation;
    /* 0x18 */ float objectScale;
    /* 0x1C */ float objectFlags;
    /* 0x20 */ float objectTimer;
    /* 0x24 */ float objectProgress;
    /* 0x28 */ float objectSpeed;
    /* 0x2C */ float objectDirection;
    /* 0x30 */ float objectFrame;
    /* 0x34 */ float animationAttributes;
    /* 0x38 */ float animationData;
    /* 0x3C */ float effectWork;
    /* 0x40 */ float positionWork;
    union {
        /* 0x44 */ s16 animationMatrix[0x20];
        struct {
            float effectVelocity;
            float effectState;
            char reservedAnimationData[4];
            float animationFlags;
            char reservedEffectData[0x30];
        };
    };
} M425EffectWork; /* size = 0x84 */

void DrawSceneBackground(OMOBJ *object);
void InitializeCameraObject(OMOBJ *object);
void UpdateCameraObject(OMOBJ *object);
void SetCameraMotion(s16 transitionFrames, Vec *startPosition, Vec *startRotation, float targetZoom);
void UpdatePlayerObject(OMOBJ *object);
void UpdatePlayerAnimation(OMOBJ *object);
void UpdatePlayerEffects(OMOBJ *object);
void UpdatePlayerMotion(OMOBJ *object);
s16 CreateParticleEffect(s32 animationResourceId, s16 particleCount, HU3DPARTICLEHOOK particleUpdateHook);
void UpdateParticleVertices(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void StartCameraIntro(OMOBJ *object);
void UpdateCameraIntro(OMOBJ *object);
void DrawParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void SetVisualEffect(s16 modelId);
void AllocateTextureCoordinateArray(Vec2f **textureCoordinates, s16 frameCount, Vec2f *sourceTextureCoordinates);
void UpdateModelTransform(HU3DMODEL *model, Mtx matrix);
void DrawModelWithEffect(HU3DMODEL *model, Mtx matrix);
s32 GetRandomByte(void);
void WaitForSceneTransition(OMOBJ *object);
void CleanupMinigameScene(OMOBJ *object);
void UpdateParticleEffects(OMOBJ *object);

s16 selectedResult = -1;
Vec cameraStartPosition = { -200.0f, 2500.0f, 1200.0f };
Vec cameraDirection = { 0.2f, -0.8f, -0.2f };
float cameraViewMatrix[9] = { 0.0f, 2500.0f, 1500.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
GXColor whiteLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };

OMOBJ *setupObject;
OMOBJ *outOfViewObject;
// M425DllWork2
OMOBJ *cameraObject;
OMOBJ *particleEffectObject;
// M425DllWork
OMOBJ *playerObjects[4];
u16 cameraIntroState;
s16 resultSequenceId;
s16 minigameSequenceId;
s16 sequenceTimer;
s16 sceneWaitTimer;
M425EffectWork effectWorkPool[0xC];
float cameraVerticalSpeed;
float cameraHeight;
float frameCounter;
char reservedState[4];

void ObjectSetup(void)
{
    s32 playerIndex;
    HUPROCESS *process;
    OMOBJ *object;
    s32 playerSlot;
    HU3DLIGHT *light;
    HU3DCAMERA *camera;
    s32 randomEffectCount;
    s32 lightIndex;
    s32 playerGroupSlots[4] = { 0, 0, 0, 0 };

    OSReport("******* M425ObjectSetup *********\n");
    Hu3DLighInit();
    lightIndex = Hu3DGLightCreateV(&cameraStartPosition, &cameraDirection, &whiteLightColor);
    Hu3DGLightInfinitytSet(lightIndex);
    light = Hu3DGlobalLight;
    light->type |= 0x8000;
    process = omInitObjMan(0x32, 0x2000);
    randomEffectCount = frand() & 0x1F;
    for (playerIndex = 0; playerIndex < randomEffectCount; playerIndex++) {
        GetRandomByte();
    }
    CRot.x = 8.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    Center.x = 0.0f;
    Center.y = 430.0f;
    Center.z = -400.0f;
    CZoom = 1650.0f;
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 45.0f, 100.0f, 50000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    outOfViewObject = omAddObjEx(process, 0x7FDA, 0, 0, -1, omOutView);
    camera = Hu3DCamera;
    camera->fov = -1.0f;
    setupObject = omAddObjEx(process, 0x10, 8, 0, -1, StartCameraIntro);
    particleEffectObject = omAddObjEx(process, 0x20, 0x10, 0, -1, UpdateParticleEffects);
    cameraObject = omAddObjEx(process, 0x50, 0x10, 0, -1, InitializeCameraObject);
    CreateThwompObjectManager(process, 0x40);
    omMakeGroupEx(process, 0, 4);
    omGetGroupMemberListEx(process, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        object = omAddObjEx(process, 0x30, 2, 0xA, 0, UpdatePlayerObject);
        playerObjects[playerIndex] = object;
        object->work[0] = playerIndex;
        object->work[1] = GWPlayerCfg[playerIndex].group;
        for (playerSlot = object->work[1] * 2; playerSlot < 4; playerSlot++) {
            if (playerGroupSlots[playerSlot] == 0) {
                object->work[1] = playerSlot;
                playerGroupSlots[playerSlot]++;
                break;
            }
        }
    }
    omGameSysInit(process);
    HuAudSndGrpSet(0x32);
}

void StartCameraIntro(OMOBJ *object)
{
    cameraIntroState = 0;
    object->objFunc = UpdateCameraIntro;
}

void UpdateCameraIntro(OMOBJ *object)
{
    Vec vector;
    Vec vector2;
    float cameraDistance;
    HU3DCAMERA *camera;

    switch (cameraIntroState) {
        case 0:
            vector.x = -250.0f;
            vector.y = 225.0f;
            vector.z = 200.0f;
            vector2.x = 22.0f;
            vector2.y = 10.0f;
            vector2.z = 0.0f;
            cameraDistance = 500.0f;
            SetCameraMotion(1, &vector, &vector2, cameraDistance);
            cameraVerticalSpeed = -292.0f / REFRESH_RATE_F;
            cameraHeight = 600.0f;
            vector.x = -300.0f;
            vector.y = cameraHeight;
            vector.z = 0.0f;
            CreateThwompModels(0, &vector);
            vector.x = 300.0f;
            CreateThwompModels(1, &vector);
            camera = Hu3DCamera;
            camera->fov = 45.0f;
            WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
            cameraIntroState++;
            break;
        case 1:
            cameraHeight += cameraVerticalSpeed;
            cameraVerticalSpeed += -146.0f / REFRESH_RATE_F;
            if (cameraHeight < 0.0f) {
                cameraHeight = 0.0f;
                cameraIntroState++;
                sceneWaitTimer = REFRESH_RATE;
                UpdateThwompModelState(0);
                UpdateThwompModelState(1);
                ResetThwompModel(0);
            }
            vector.x = -300.0f;
            vector.y = cameraHeight;
            vector.z = 0.0f;
            CreateThwompModels(0, &vector);
            vector.x = 300.0f;
            CreateThwompModels(1, &vector);
            break;
        case 2:
            if (--sceneWaitTimer == 0) {
                vector.x = 0.0f;
                vector.y = 430.0f;
                vector.z = -400.0f;
                vector2.x = 8.0f;
                vector2.y = 0.0f;
                vector2.z = 0.0f;
                cameraDistance = 1650.0f;
                sceneWaitTimer = REFRESH_RATE * 1.2;
                SetCameraMotion(REFRESH_RATE, &vector, &vector2, cameraDistance);
                cameraIntroState++;
            }
            break;
        case 3:
            if (--sceneWaitTimer == 0) {
                cameraIntroState = 4;
                object->objFunc = WaitForSceneTransition;
                resultSequenceId = minigameSequenceId = -1;
            }
            break;
    }
    if ((omSysExitReq != 0) && (WipeStatGet() == 0)) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = CleanupMinigameScene;
    }
}

s32 resultSequenceId = -1;
s32 minigameAnimationIds[0xA] = { 0, 2, 3, 5, 6, 8, 0xA, 9, 0x17, 0x18 };
Vec playerModelPositions[4] = {
    { -350.0f, 800.0f, 100.0f },
    { -200.0f, 1000.0f, 0.0f },
    { 200.0f, 1200.0f, 0.0f },
    { 350.0f, 1400.0f, 100.0f },
};

void WaitForSceneTransition(OMOBJ *object)
{
    s16 winningPlayerIndices[2];
    float thwompModelValue;
    s32 playerIndex;
    s32 resolvedPlayerCount;
    s32 winningPlayerCount;
    s32 remainingSeconds;
    s16 unusedPlayerState;
    s16 unusedSequenceState;

    switch (cameraIntroState) {
        case 4:
            if (minigameSequenceId == -1) {
                sequenceTimer = REFRESH_RATE * 30;
                minigameSequenceId = MGSeqCreate(3, 0);
                MGSeqPosSet(minigameSequenceId, 320.0f, 240.0f);
            }
            else {
                if (MGSeqStatGet(minigameSequenceId) == 0) {
                    minigameSequenceId = -1;
                    cameraIntroState++;
                    resultSequenceId = MGSeqCreate(1, sequenceTimer / REFRESH_RATE, -1, -1);
                }
            }
            if ((resultSequenceId == -1) && ((MGSeqStatGet(minigameSequenceId) & 0x10) != 0)) {
                resultSequenceId = HuAudSeqPlay(0x48);
            }
            break;
        case 5:
            resolvedPlayerCount = 0;
            sequenceTimer--;
            remainingSeconds = (sequenceTimer + REFRESH_RATE - 1) / REFRESH_RATE;
            if (remainingSeconds < 0) {
                remainingSeconds = 0;
            }
            MGSeqParamSet(resultSequenceId, 1, remainingSeconds);
            if (sequenceTimer < -1) {
                resolvedPlayerCount++;
                if ((getThwompModelId(0) == 1000.0f) && (getThwompModelId(1) == 1000.0f)) {
                    selectedResult = -1;
                }
                else {
                    thwompModelValue = getThwompModelId(1);
                    if (getThwompModelId(0) <= thwompModelValue) {
                        selectedResult = 0;
                    }
                    else {
                        selectedResult = 1;
                    }
                }
            }
            for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                if (!(getThwompModelId(playerIndex) > 0.0f)) {
                    selectedResult = playerIndex;
                    resolvedPlayerCount++;
                }
            }
            if (resolvedPlayerCount != 0) {
                cameraIntroState++;
            }
            break;
        case 6:
            if (minigameSequenceId == -1) {
                minigameSequenceId = MGSeqCreate(3, 1);
                MGSeqPosSet(minigameSequenceId, 320.0f, 240.0f);
                MGSeqParamSet(resultSequenceId, 2, -1);
                HuAudSeqFadeOut(resultSequenceId, 0x64);
            }
            else if (MGSeqStatGet(minigameSequenceId) == 0) {
                minigameSequenceId = -1;
                sceneWaitTimer = 1;
                cameraIntroState++;
            }
            break;
        case 7:
            if (--sceneWaitTimer <= 0) {
                minigameSequenceId = -1;
                cameraIntroState++;
            }
            break;
        case 8:
            if (selectedResult != -1) {
                for (playerIndex = 0, winningPlayerCount = 0; playerIndex < 4; playerIndex++) {
                    if (selectedResult == omObjGetDataAs(playerObjects[playerIndex], M425DllWork)->animationState) {
                        winningPlayerIndices[winningPlayerCount] = omObjGetDataAs(playerObjects[playerIndex], M425DllWork)->playerIndex;
                        winningPlayerCount++;
                        GWPlayerCoinWinAdd(playerIndex, 10);
                    }
                }
                minigameSequenceId = MGSeqCreate(5, 3, winningPlayerIndices[0], winningPlayerIndices[1], -1, -1);
                HuAudSStreamPlay(1);
            }
            else {
                minigameSequenceId = MGSeqCreate(3, 2);
                HuAudSStreamPlay(4);
            }
            sceneWaitTimer = REFRESH_RATE * 3.5;
            cameraIntroState++;
            break;
        case 9:
            if (--sceneWaitTimer <= 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = CleanupMinigameScene;
            }
            break;
    }
    if ((omSysExitReq != 0) && (WipeStatGet() == 0)) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = CleanupMinigameScene;
    }
}

void CleanupMinigameScene(OMOBJ *object)
{
    if (WipeStatGet() == 0) {
        CharModelKill(-1);
        MGSeqKill(resultSequenceId);
        MGSeqKillAll();
        CleanupThwompScene();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void UpdateParticleEffects(OMOBJ *object)
{
    M425EffectWork *effectWork;
    s32 modelId;
    s16 *particleWork;
    s32 effectIndex;
    s32 animationIndex;
    Vec vector[] = {
        { 475.0f, 800.0f, -1400.0f },
        { -300.0f, 900.0f, -1400.0f },
        { 300.0f, 875.0f, -1450.0f },
        { -575.0f, 850.0f, -1550.0f },
        { 350.0f, 750.0f, -1500.0f },
        { -450.0f, 775.0f, -1550.0f },
        { 325.0f, 725.0f, -1450.0f },
        { -525.0f, 725.0f, -1500.0f },
        { 575.0f, 850.0f, -1550.0f },
        { -350.0f, 750.0f, -1500.0f },
        { 450.0f, 775.0f, -1550.0f },
        { -325.0f, 725.0f, -1450.0f },
        { 525.0f, 725.0f, -1500.0f },
        { -475.0f, 800.0f, -1400.0f },
        { 300.0f, 900.0f, -1400.0f },
        { -300.0f, 875.0f, -1450.0f },
    };

    modelId = Hu3DModelCreateFile(0x380000);
    object->mdlId[0] = modelId;
    modelId = Hu3DModelCreateFile(0x380001);
    object->mdlId[1] = modelId;
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    modelId = Hu3DHookFuncCreate(UpdateModelTransform);
    object->mdlId[2] = modelId;
    SetVisualEffect(modelId);
    modelId = CreateParticleEffect(0x1E0020, 0xC8, DrawParticleEffect);
    object->mdlId[3] = modelId;
    Hu3DParticleBlendModeSet(modelId, 1);
    modelId = CreateParticleEffect(0x380005, 1, UpdateParticleVertices);
    object->mdlId[5] = modelId;
    Hu3DParticleBlendModeSet(modelId, 1);
    Hu3DModelLayerSet(modelId, 6);
    particleWork = ((HU3DPARTICLE *)Hu3DData[modelId].hookData)->work;
    particleWork[1] = 0;
    modelId = CreateParticleEffect(0x380008, 0xA, UpdateParticleVertices);
    object->mdlId[6] = modelId;
    Hu3DParticleBlendModeSet(modelId, 1);
    Hu3DModelLayerSet(modelId, 6);
    particleWork = ((HU3DPARTICLE *)Hu3DData[modelId].hookData)->work;
    particleWork[1] = 1;
    modelId = CreateParticleEffect(0x380007, 4, UpdateParticleVertices);
    object->mdlId[7] = modelId;
    Hu3DParticleBlendModeSet(modelId, 1);
    Hu3DModelLayerSet(modelId, 6);
    particleWork = ((HU3DPARTICLE *)Hu3DData[modelId].hookData)->work;
    particleWork[1] = 2;
    modelId = CreateParticleEffect(0x380006, 6, UpdateParticleVertices);
    object->mdlId[8] = modelId;
    Hu3DParticleBlendModeSet(modelId, 1);
    Hu3DModelLayerSet(modelId, 6);
    particleWork = ((HU3DPARTICLE *)Hu3DData[modelId].hookData)->work;
    particleWork[1] = 3;
    for (effectIndex = 0; effectIndex < 0xC; effectIndex++) {
        effectWork = &effectWorkPool[effectIndex];
        modelId = Hu3DModelCreateFile(0x380002);
        effectWork->objectIndex = modelId;
        effectWork->objectModel.x = vector[effectIndex].x;
        effectWork->objectModel.y = vector[effectIndex].y;
        effectWork->objectModel.z = vector[effectIndex].z;
        Hu3DModelPosSet(modelId, effectWork->objectModel.x, effectWork->objectModel.y, effectWork->objectModel.z);
        Hu3DMotionSpeedSet(modelId, 6.0f);
        effectWork->effectMode = effectWork->effectFrame = 0;
        effectWork->objectRotation = effectWork->objectScale = effectWork->objectFlags = 0.0f;
        effectWork->objectTimer = effectWork->objectProgress = effectWork->objectSpeed = 1.0f;
        effectWork->animationData = effectWork->effectWork = effectWork->positionWork = 0.0f;
        effectWork->objectDirection = effectWork->objectModel.x;
        effectWork->objectFrame = effectWork->objectModel.y;
        effectWork->animationAttributes = effectWork->objectModel.z;
        if ((effectIndex & 1) != 0) {
            effectWork->objectScale = 90.0f;
            effectWork->objectModel.y += 650.0;
            effectWork->objectModel.x -= 1400.0;
        }
        else {
            effectWork->objectScale = 90.0f;
        }
        effectWork->objectScale += 10.0 * (3.0517578125e-05 * (float)((u16)frand() - 0x8000));
        effectWork->objectModel.z -= 400.0;
        effectWork->objectState = 100.0 * (0.10000000149011612 + (0.019999999552965164 * (1.52587890625e-05 * (float)(s32)(u16)frand())));
        for (animationIndex = 0; animationIndex < 0x20; animationIndex++) {
            effectWork->animationMatrix[animationIndex] = 0;
        }
        effectWork->animationFlags = effectWork->objectState;
    }
    HuAudFXPlay(0x633);
    HuAudFXPlay(0x635);
    object->objFunc = DrawSceneBackground;
}

void RotateMatrixAxes(Mtx matrix, float rotationX, float rotationY, float rotationZ)
{
    Mtx matrix2;
    Mtx matrix3;
    if (rotationZ != 0.0f) {
        MTXRotRad(matrix, 0x5A, MTXDegToRad(rotationZ));
    }
    else {
        MTXIdentity(matrix);
    }
    if (rotationX != 0.0f) {
        MTXRotRad(matrix2, 0x58, MTXDegToRad(rotationX));
        MTXConcat(matrix2, matrix, matrix);
    }
    if (rotationY != 0.0f) {
        MTXRotRad(matrix3, 0x59, MTXDegToRad(rotationY));
        MTXConcat(matrix3, matrix, matrix);
    }
}

float VectorAngleDegrees(float xComponent, float yComponent)
{
    float fullCircleDegrees = 360.0f;
    float angleDegrees = 180.0 * (atan2f(yComponent, xComponent) / M_PI);
    if (angleDegrees < 0.0f) {
        angleDegrees += fullCircleDegrees;
    }
    else if (angleDegrees >= fullCircleDegrees) {
        angleDegrees -= fullCircleDegrees;
    }
    return angleDegrees;
}

void ExtractMatrixRotation(Mtx matrix, Vec *vector)
{
    float sineComponent;
    float cosineComponent;

    vector->x = VectorAngleDegrees(matrix[2][2], matrix[1][2]);
    vector->z = VectorAngleDegrees(matrix[0][0], matrix[0][1]);
    sineComponent = -matrix[0][2];
    cosineComponent = sqrtf((1.0 - (sineComponent * sineComponent)) < 0.0 ? (-1.0 - (sineComponent * sineComponent)) : (1.0 - (sineComponent * sineComponent)));
    if ((vector->x > 90.0f) && (vector->x < 270.0f) && (vector->z > 90.0f) && (vector->z < 270.0f)) {
        vector->x = fmod(180.0f + vector->x, 360.0);
        vector->z = fmod(180.0f + vector->z, 360.0);
        cosineComponent = -cosineComponent;
    }
    vector->y = VectorAngleDegrees(cosineComponent, sineComponent);
}

void DrawSceneBackground(OMOBJ *object)
{
    Mtx matrix;
    Mtx matrix2;
    Mtx matrix3;
    Vec vector;
    M425EffectWork *effectWork;
    s32 effectIndex;

    for (effectIndex = 0; effectIndex < 0xC; effectIndex++) {
        effectWork = &effectWorkPool[effectIndex];
        switch (effectWork->effectFrame) {
            case 0:
                if (effectWork->objectState < effectWork->animationFlags) {
                    effectWork->objectState += 0.4f;
                }
                if ((s32)(64.0 * (1.52587890625e-05 * (float)GetRandomByte())) == 0) {
                    Hu3DMotionTimeSet(effectWork->objectIndex, 0.0f);
                }
                if (Hu3DMotionEndCheck(effectWork->objectIndex) != 0) {
                    effectWork->effectWork = (VERSION_PAL) ? -3.5040002f : -2.92f; // −876.0f / (REFRESH_RATE_F * 5)
                }
                else {
                    effectWork->effectWork = (VERSION_PAL) ? -0.58400005f : -0.48666668f;
                }
                effectWork->effectVelocity += 0.10000000149011612 * (3.0517578125e-05 * (float)((u16)frand() - 0x8000));
                effectWork->effectState += effectWork->effectVelocity;
                if (effectWork->effectState > 180.0f) {
                    effectWork->effectState -= 180.0f;
                }
                if (effectWork->effectState < -180.0f) {
                    effectWork->effectState += 180.0f;
                }
                effectWork->objectRotation = -30.0 * sind(effectWork->effectState) * sind(effectWork->objectScale);
                effectWork->objectFlags = -30.0 * sind(effectWork->effectState) * cosd(effectWork->objectScale);
                if ((effectWork->objectModel.y <= 700.0f) || (fabs(effectWork->objectModel.x) > 2000.0)) {
                    effectWork->effectWork = 0.0f;
                    effectWork->animationMatrix[6] = (s32)(20.0 * (3.0517578125e-05 * (float)(GetRandomByte() - 0x8000))) + 0xB4;
                    effectWork->objectState *= 0.75f;
                    effectWork->effectFrame++;
                }
                break;
            case 1:
                effectWork->objectScale += 1.0f;
                effectWork->objectRotation = 0.0f;
                effectWork->objectFlags = 60.0 * sind(effectWork->animationMatrix[6]) * cosd(effectWork->objectScale);
                if (--effectWork->animationMatrix[6] == 0) {
                    Hu3DModelAttrSet(effectWork->objectIndex, HU3D_MOTATTR_LOOP);
                    Hu3DMotionSpeedSet(effectWork->objectIndex, 6.0f);
                    effectWork->effectWork = 321.2f / REFRESH_RATE_F;
                    effectWork->effectFrame++;
                }
                break;
            case 2:
                if (effectWork->objectState > 4.0f) {
                    effectWork->objectState -= 0.1f;
                    effectWork->effectWork += (VERSION_PAL) ? 0.87600005f : 0.73f;
                }
                effectWork->effectWork += (VERSION_PAL) ? -0.657f : -0.5475f;
                if (effectWork->effectWork < 0.0f) {
                    Hu3DModelAttrReset(effectWork->objectIndex, HU3D_MOTATTR_LOOP);
                    Hu3DMotionSpeedSet(effectWork->objectIndex, 2.0f);
                    effectWork->animationFlags = 100.0 * (0.10000000149011612 + (0.019999999552965164 * (1.52587890625e-05 * (float)(s32)(u16)frand())));
                    effectWork->effectFrame = 0;
                }
                break;
        }
        if (0) {
            s32 unusedEffectWorkValue;
            s32 unusedModelIndex;
            s32 unusedAnimationIndex;
            float unusedAnimationValue;
            float unusedScaleFactor;
            float unusedMotionFactor;
            float unusedEffectFactor;

            (void) object;
            (void) object;
            (void) unusedEffectWorkValue;
            (void) unusedEffectWorkValue;
            (void) unusedEffectWorkValue;
            (void) unusedEffectWorkValue;
            (void) unusedEffectWorkValue;
            (void) unusedEffectWorkValue;
            (void) unusedModelIndex;
            (void) unusedModelIndex;
            (void) unusedModelIndex;
            (void) unusedModelIndex;
            (void) unusedModelIndex;
            (void) unusedModelIndex;
            (void) unusedAnimationIndex;
            (void) unusedAnimationIndex;
            (void) unusedAnimationIndex;
            (void) unusedAnimationIndex;
            (void) unusedAnimationIndex;
            (void) unusedAnimationIndex;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedAnimationValue;
            (void) unusedScaleFactor;
            (void) unusedScaleFactor;
            (void) unusedScaleFactor;
            (void) unusedMotionFactor;
            (void) unusedMotionFactor;
            (void) unusedMotionFactor;
            (void) unusedEffectFactor;
            (void) unusedEffectFactor;
            (void) unusedEffectFactor;
        }
        effectWork->animationData = effectWork->objectState * sind(effectWork->objectScale);
        effectWork->positionWork = effectWork->objectState * cosd(effectWork->objectScale);
        effectWork->objectModel.x += effectWork->animationData;
        effectWork->objectModel.y += effectWork->effectWork;
        effectWork->objectModel.z += effectWork->positionWork;
        Hu3DModelPosSet(effectWork->objectIndex, effectWork->objectModel.x, effectWork->objectModel.y, effectWork->objectModel.z);
        RotateMatrixAxes(matrix, effectWork->objectRotation, -effectWork->objectScale, effectWork->objectFlags);
        ExtractMatrixRotation(matrix, &vector);
        Hu3DModelRotSetV(effectWork->objectIndex, &vector);
    }
}

void InitializeCameraObject(OMOBJ *object)
{
    M425DllWork2 *work = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M425DllWork2), HU_MEMNUM_OVL);
    object->data = work;
    work->frameEnd = work->frameStart = 0;
    work->targetPosition.x = work->objectSpeed.x = 8.0f;
    work->targetPosition.y = work->objectSpeed.y = 0.0f;
    work->targetPosition.z = work->objectSpeed.z = 0.0f;
    work->objectState.x = work->objectFlags.x = 0.0f;
    work->objectState.y = work->objectFlags.y = 430.0f;
    work->objectState.z = work->objectFlags.z = -400.0f;
    work->blendFrame = work->blendSpeed = 1650.0f;
    object->objFunc = UpdateCameraObject;
}

void UpdateCameraObject(OMOBJ *object)
{
    float interpolationProgress;
    M425DllWork2 *work;

    work = object->data;
    if (work->frameEnd != 0) {
        work->frameEnd--;
        interpolationProgress = (float)(work->frameStart - work->frameEnd) / work->frameStart;
        interpolationProgress = sind((90.0f * interpolationProgress));
        work->objectState.x = work->objectFlags.x + (interpolationProgress * (work->animationAttributes - work->objectFlags.x));
        work->objectState.y = work->objectFlags.y + (interpolationProgress * (work->animationData - work->objectFlags.y));
        work->objectState.z = work->objectFlags.z + (interpolationProgress * (work->effectWork - work->objectFlags.z));
        work->targetPosition.x = work->objectSpeed.x + (interpolationProgress * (work->positionWork - work->objectSpeed.x));
        work->targetPosition.y = work->objectSpeed.y + (interpolationProgress * (work->animationMatrix - work->objectSpeed.y));
        work->targetPosition.z = work->objectSpeed.z + (interpolationProgress * (work->effectState - work->objectSpeed.z));
        work->blendFrame = work->blendSpeed + (interpolationProgress * (work->animationIds - work->blendSpeed));
    }
    CRot.x = work->targetPosition.x;
    CRot.y = work->targetPosition.y;
    CRot.z = work->targetPosition.z;
    Center.x = work->objectState.x;
    Center.y = work->objectState.y;
    Center.z = work->objectState.z;
    CZoom = work->blendFrame;
}

void SetCameraMotion(s16 transitionFrames, Vec *vector, Vec *vector2, float targetZoom)
{
    M425DllWork2 *work = cameraObject->data;
    work->frameStart = transitionFrames;
    work->frameEnd = transitionFrames;
    work->objectFlags = work->objectState;
    work->animationAttributes = vector->x;
    work->animationData = vector->y;
    work->effectWork = vector->z;
    work->objectSpeed = work->targetPosition;
    work->positionWork = vector2->x;
    work->animationMatrix = vector2->y;
    work->effectState = vector2->z;
    work->blendSpeed = work->blendFrame;
    work->animationIds = targetZoom;
}

void UpdatePlayerObject(OMOBJ *object)
{
    s32 playerDataDirectory; // ! - uninitialized
    M425DllWork *playerWork;
    s32 playerIndex;
    s32 characterModelId;
    s32 playerSlot;

    playerWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M425DllWork), HU_MEMNUM_OVL);
    object->data = playerWork;
    playerWork->objectIndex = object->work[0];
    playerWork->playerIndex = GWPlayerCfg[playerWork->objectIndex].character;
    playerWork->playerGroupIndex = GWPlayerCfg[playerWork->objectIndex].group;
    playerWork->objectVariant = object->work[1] & 1;
    playerWork->objectState = GWPlayerCfg[playerWork->objectIndex].pad_idx;
    playerWork->effectState = GWPlayerCfg[playerWork->objectIndex].iscom;
    playerWork->motionState = GWPlayerCfg[playerWork->objectIndex].diff;
    playerWork->motionIndex = 0;
    playerWork->roundIndex = 1;
    playerWork->objectPosition = 0;
    memset(playerWork->effectWork, 0, sizeof(playerWork->effectWork));
    playerWork->collisionState = 0;
    playerWork->movementSpeed = 1.0f;
    playerWork->blendFrame = 0;
    playerWork->blendSpeed = 0;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if ((playerIndex != playerWork->objectIndex) && (playerWork->playerGroupIndex == GWPlayerCfg[playerIndex].group)) {
            playerWork->controllerPort = playerIndex;
            break;
        }
    }
    playerSlot = object->work[1];
    playerWork->objectScale.x = playerModelPositions[playerSlot].x;
    playerWork->objectScale.y = playerModelPositions[playerSlot].y;
    playerWork->objectScale.z = playerModelPositions[playerSlot].z;
    playerWork->objectProgress.x = playerWork->objectProgress.y = playerWork->objectProgress.z = 0.0f;
    if (playerWork->playerGroupIndex == 0) {
        playerWork->targetPosition = 30.0f;
    }
    else {
        playerWork->targetPosition = -30.0f;
    }
    characterModelId = CharModelCreate(playerWork->playerIndex, 4);
    object->mdlId[0] = characterModelId;
    Hu3DModelAttrSet(characterModelId, HU3D_MOTATTR_LOOP);
    CharModelStepFxSet(playerWork->playerIndex, 1);
    for (playerIndex = 0; playerIndex < 0xA; playerIndex++) {
        object->playerIndex[playerIndex] = CharMotionCreate(playerWork->playerIndex, minigameAnimationIds[playerIndex]);
        if (playerIndex == 5) {
            CharMotionVoiceOnSet(playerWork->playerIndex, object->mtnId[playerIndex], 0);
        }
    }
    CharMotionDataClose(playerWork->playerIndex);
    CharMotionSet(playerWork->playerIndex, object->motionIndex[playerWork->motionIndex]);
    omSetTra(object, playerWork->objectScale.x, playerWork->objectScale.y, playerWork->objectScale.z);
    Hu3DModelPosSet(characterModelId, playerWork->objectScale.x, playerWork->objectScale.y, playerWork->objectScale.z);
    Hu3DModelRotSet(characterModelId, 0.0f, 30.0f, 0.0f);
#ifndef NON_MATCHING
    HuDataDirClose(playerDataDirectory);
#endif
    object->objFunc = UpdatePlayerAnimation;
}

void UpdatePlayerAnimation(OMOBJ *object)
{
    float recentHeights[8];
    s32 characterModelId;
    M425DllWork *playerWork;
    s32 heightSampleIndex;
    s32 heightHistoryIndex;
    s32 currentHeightIndex;
    s32 collisionDetected;

    collisionDetected = 0;
    characterModelId = object->mdlId[0];
    playerWork = object->data;
    playerWork->objectFrame = playerWork->objectScale;
    if ((cameraIntroState == 5) && (playerWork->effectState != 0)) {
        UpdatePlayerEffects(object);
    }
    UpdatePlayerMotion(object);
    switch (cameraIntroState) {
        case 2:
            if (sceneWaitTimer == 1) {
                playerWork->motionIndex = 5;
                CharMotionShiftSet(playerWork->playerIndex, object->motionIndex[playerWork->motionIndex], 0.0f, 8.0f, 0);
                playerWork->objectProgress.y = 0.0f;
                playerWork->roundIndex = 0;
            }
            playerWork->blendSpeed = 0;
            break;
        case 3:
            if (playerWork->blendFrame != 0) {
                playerWork->objectScale.y = calculateThwompVertexPosition(playerWork->playerGroupIndex, playerWork->objectVariant, playerWork->objectScale.x, playerWork->objectScale.z);
                playerWork->objectScale.y -= 10.0f;
            }
            else if (calculateThwompCollision(playerWork->playerGroupIndex, playerWork->objectVariant, &playerWork->objectFrame, &playerWork->objectProgress, &playerWork->objectScale, 0) != 0) {
                if (playerWork->roundIndex == 0) {
                    HuAudFXPlay(0x62F);
                }
                CharMotionVoiceOnSet(playerWork->playerIndex, object->mtnId[5], 1);
                playerWork->roundIndex = 1;
            }
            break;
        case 5:
            if ((playerWork->motionIndex == 5) && (playerWork->objectPosition == 0)) {
                collisionDetected = 1;
            }

        case 4:
        case 6:
        case 7:
            if (playerWork->blendFrame != 0) {
                playerWork->objectScale.y = calculateThwompVertexPosition(playerWork->playerGroupIndex, playerWork->objectVariant, playerWork->objectScale.x, playerWork->objectScale.z);
                playerWork->objectScale.y -= 10.0f;
            }
            else if (calculateThwompCollision(playerWork->playerGroupIndex, playerWork->objectVariant, &playerWork->objectFrame, &playerWork->objectProgress, &playerWork->objectScale, collisionDetected) != 0) {
                playerWork->roundIndex = 1;
            }
            break;
        case 8:
            if (playerWork->blendFrame != 0) {
                playerWork->objectScale.y = calculateThwompVertexPosition(playerWork->playerGroupIndex, playerWork->objectVariant, playerWork->objectScale.x, playerWork->objectScale.z);
                playerWork->objectScale.y -= 10.0f;
                playerWork->roundIndex = 1;
            }
            else if (calculateThwompCollision(playerWork->playerGroupIndex, playerWork->objectVariant, &playerWork->objectFrame, &playerWork->objectProgress, &playerWork->objectScale, 0) != 0) {
                playerWork->roundIndex = 1;
            }
            if ((playerWork->roundIndex != 0) && (playerWork->motionIndex != 8) && (playerWork->motionIndex != 9)) {
                if (playerWork->playerGroupIndex == selectedResult) {
                    playerWork->motionIndex = 8;
                }
                else {
                    playerWork->motionIndex = 9;
                }
                CharMotionShiftSet(playerWork->playerIndex, object->motionIndex[playerWork->motionIndex], 0.0f, 8.0f, 0);
            }
            break;
        case 9:
            if (playerWork->blendFrame != 0) {
                playerWork->objectScale.y = calculateThwompVertexPosition(playerWork->playerGroupIndex, playerWork->objectVariant, playerWork->objectScale.x, playerWork->objectScale.z);
                playerWork->objectScale.y -= 10.0f;
            }
            else if (calculateThwompCollision(playerWork->playerGroupIndex, playerWork->objectVariant, &playerWork->objectFrame, &playerWork->objectProgress, &playerWork->objectScale, 0) != 0) {
                playerWork->roundIndex = 1;
            }
            break;
    }
    if (playerWork->objectScale.y < 0.0f) {
        playerWork->objectScale.y = 0.0f;
        playerWork->roundIndex = 1;
    }
    currentHeightIndex = playerWork->collisionState % 8;
    playerWork->effectWork[currentHeightIndex] = playerWork->objectScale.y;
    if (++playerWork->collisionState > 8) {
        for (heightSampleIndex = 0; heightSampleIndex < 8; heightSampleIndex++) {
            heightHistoryIndex = currentHeightIndex - heightSampleIndex;
            if (heightHistoryIndex < 0) {
                heightHistoryIndex += 8;
            }
            recentHeights[heightSampleIndex] = playerWork->effectWork[heightHistoryIndex];
        }
        for (heightSampleIndex = 1; heightSampleIndex < 8; heightSampleIndex++) {
            if (fabs(recentHeights[heightSampleIndex] - recentHeights[0]) > 25.0) {
                break;
            }
        }
        if (((playerWork->motionIndex == 0) || (playerWork->motionIndex == 8) || (playerWork->motionIndex == 9)) && (heightSampleIndex >= 8)) {
            playerWork->movementSpeed = playerWork->movementSpeed - 0.05f;
            if (playerWork->movementSpeed < 0.75f) {
                playerWork->movementSpeed = 0.75f;
            }
        }
        else {
            playerWork->movementSpeed = 1.0f;
        }
    }
    omSetTra(object, playerWork->objectScale.x, playerWork->objectScale.y, playerWork->objectScale.z);
}

void UpdatePlayerEffects(OMOBJ *object)
{
    s32 characterModelId;
    s16 motionIndex;
    M425DllWork *playerWork;
    M425DllWork *playerWork2;

    playerWork = object->data;
    playerWork2 = playerObjects[playerWork->controllerPort]->data;
    characterModelId = object->mdlId[0];
    motionIndex = playerWork->motionIndex;
    switch (playerWork->motionIndex) {
        case 4:
        case 7:
        case 0:
            if (playerWork2->motionIndex == 3) {
                if ((playerWork2->effectState == 0) || ((s32)((0x18 - (playerWork->motionState * 3)) * (1.52587890625e-05 * (float)GetRandomByte())) == 0)) {
                    HuPadBtnDown[playerWork->objectState] |= 0x100;
                    break;
                }
            }
            else {
                if ((s32)((0x30 - (playerWork->motionState * 4)) * (1.52587890625e-05 * (float)GetRandomByte())) == 0) {
                    HuPadBtnDown[playerWork->objectState] |= 0x100;
                    break;
                }
                break;
            }
            break;
        case 3:
            if ((s32)(4.0 * (1.52587890625e-05 * (float)GetRandomByte())) != 0) {
                HuPadBtn[playerWork->objectState] |= 0x100;
            }
            if ((playerWork2->effectState == 0) && (playerWork2->motionIndex == 5)) {
                HuPadBtnDown[playerWork->objectState] |= 0x100;
                break;
            }
            if ((playerWork->objectProgress.y < ((VERSION_PAL) ? 6.42400016784668 : 5.353333473205566)) && ((s32)((0xE - (playerWork->motionState * 2)) * (1.52587890625e-05 * (float)GetRandomByte())) == 0)) {
                HuPadBtnDown[playerWork->objectState] |= 0x100;
            }
            break;
    }
}

void UpdatePlayerMotion(OMOBJ *object)
{
    float movementAngle;
    float angleDifference;

    M425DllWork *playerWork;
    s16 currentMotionIndex;
    s16 motionChanged;
    s16 pressedButtons;
    s16 movementX;
    s16 movementZ;
    s16 heldButtons;
    s32 characterModelId;

    playerWork = object->data;
    characterModelId = object->mdlId[0];
    currentMotionIndex = playerWork->motionIndex;
    if (cameraIntroState == 5) {
        movementX = 0;
        movementZ = 0;
        pressedButtons = HuPadBtnDown[playerWork->objectState];
        heldButtons = HuPadBtn[playerWork->objectState];
    }
    else {
        movementX = 0;
        movementZ = 0;
        pressedButtons = 0;
        heldButtons = 0;
    }
    playerWork->blendFrame = 0;
    switch (playerWork->motionIndex) {
        case 4:
        case 7:
            if (CharMotionEndCheck(playerWork->playerIndex) != 0) {
                currentMotionIndex = 0;
                motionChanged = 1;
            }

        case 0:
        case 1:
        case 2:
            playerWork->objectProgress.x = 0.25f * movementX;
            playerWork->objectProgress.z = 0.25f * -movementZ;
            playerWork->objectRotation = sqrtf((playerWork->objectProgress.x * playerWork->objectProgress.x) + (playerWork->objectProgress.z * playerWork->objectProgress.z));
            playerWork->blendFrame = 1;
            if (playerWork->objectRotation > 0.1f) {
                if (playerWork->objectRotation >= 8.0f) {
                    currentMotionIndex = 2;
                    motionChanged = 1;
                }
                else {
                    currentMotionIndex = 1;
                    motionChanged = 1;
                }
                movementAngle = atan2d(playerWork->objectProgress.x, playerWork->objectProgress.z);
                if (movementAngle >= 180.0f) {
                    movementAngle = movementAngle - 360.0f;
                }
                else if (movementAngle < -180.0f) {
                    movementAngle += 360.0f;
                }
                angleDifference = playerWork->targetPosition - movementAngle;
                if (angleDifference >= 180.0f) {
                    angleDifference -= 360.0f;
                }
                else if (angleDifference < -180.0f) {
                    angleDifference += 360.0f;
                }
                playerWork->targetPosition = movementAngle + (0.5f * angleDifference);
                if (playerWork->targetPosition >= 180.0f) {
                    playerWork->targetPosition = playerWork->targetPosition - 360.0f;
                }
                else if (playerWork->targetPosition < -180.0f) {
                    playerWork->targetPosition += 360.0f;
                }
            }
            else {
                currentMotionIndex = 0;
                motionChanged = 1;
            }
            playerWork->objectProgress.y = 0.0f;
            playerWork->objectProgress.y += -146.0f / REFRESH_RATE_F * playerWork->movementSpeed;
            if ((pressedButtons & 0x100) != 0) {
                playerWork->roundIndex = 0;
                playerWork->difficulty = 1;
                playerWork->objectProgress.y = (VERSION_PAL) ? 32.120003f : 26.766666f;
                currentMotionIndex = 3;
                motionChanged = 0;
            }
            break;
        case 3:
            if (playerWork->difficulty != 0) {
                if ((heldButtons & 0x100) != 0) {
                    if (++playerWork->difficulty > 4) {
                        playerWork->difficulty = 0;
                    }
                    else {
                        playerWork->objectProgress.y += 292.0f / REFRESH_RATE_F;
                    }
                }
                else {
                    playerWork->difficulty = 0;
                }
            }
            playerWork->objectProgress.y += -146.0f / REFRESH_RATE_F;
            if (playerWork->roundIndex != 0) {
                playerWork->roundIndex = 1;
                currentMotionIndex = 4;
                motionChanged = 0;
                playerWork->objectProgress.y = 0.0f;
            }
            if (((pressedButtons & 0x100) != 0) || ((pressedButtons & 0x40) != 0)) {
                playerWork->objectProgress.y = 0.0f;
                playerWork->objectPosition = 0;
                currentMotionIndex = 5;
                motionChanged = 0;
            }
            break;
        case 5:
            if ((Hu3DData[characterModelId].motIdShift == -1) && (CharMotionEndCheck(playerWork->playerIndex) != 0)) {
                playerWork->objectProgress.y += -146.0f / REFRESH_RATE_F;
            }
            else {
                playerWork->objectProgress.y = 0.0f;
            }
            if (playerWork->roundIndex != 0) {
                if (playerWork->objectPosition != 0) {
                    playerWork->objectProgress.y = 0.0f;
                    playerWork->objectPosition = 0;
                    currentMotionIndex = 7;
                    motionChanged = 0;
                }
                else {
                    playerWork->objectProgress.y = 584.0f / REFRESH_RATE_F;
                    playerWork->roundIndex = 0;
                    currentMotionIndex = 6;
                    motionChanged = 0;
                }
            }
            break;
        case 6:
            playerWork->objectProgress.y += -146.0f / REFRESH_RATE_F;
            if ((playerWork->roundIndex != 0) && (CharMotionEndCheck(playerWork->playerIndex) != 0)) {
                currentMotionIndex = 7;
                motionChanged = 0;
            }
            break;
        case 8:
        case 9:
            playerWork->objectProgress.y = -146.0f / REFRESH_RATE_F * playerWork->movementSpeed;
            playerWork->blendFrame = 1;
            break;
    }
    playerWork->objectScale.x += playerWork->objectProgress.x;
    if (playerWork->roundIndex == 0) {
        playerWork->objectScale.y += playerWork->objectProgress.y;
    }
    playerWork->objectScale.z += playerWork->objectProgress.z;
    if (currentMotionIndex != playerWork->motionIndex) {
        playerWork->motionIndex = currentMotionIndex;
        CharMotionShiftSet(playerWork->playerIndex, object->motionIndex[playerWork->motionIndex], 0.0f, 8.0f, motionChanged);
    }
    omSetTra(object, playerWork->objectScale.x, playerWork->objectScale.y, playerWork->objectScale.z);
    omSetRot(object, 0.0f, playerWork->targetPosition, 0.0f);
}

s16 CreateParticleEffect(s32 animationResourceId, s16 particleCount, HU3DPARTICLEHOOK particleUpdateHook)
{
    s32 particleModelId;
    M425ObjectStateWork *objectState;
    HU3DPARTICLE *particle;
    ANIMDATA *animation;

    animation = HuSprAnimRead(HuDataReadNum(animationResourceId, HU_MEMNUM_OVL));
    particleModelId = Hu3DParticleCreate(animation, particleCount);
    Hu3DParticleHookSet(particleModelId, particleUpdateHook);
    particle = Hu3DData[particleModelId].hookData;
    objectState = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M425ObjectStateWork), HU_MEMNUM_OVL);
    particle->work = objectState;
    objectState->objectState = 0;
    return particleModelId;
}

void UpdateParticleVertices(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float inverseDistance;
    float distanceSquared;
    float reflectionFactor;
    float particleHeight;
    float particleDepth;

    HU3DPARTICLEDATA *particleData;
    M425ObjectStateWork *objectState;
    s32 particleIndex;

    float particleHeightScales[4] = { 1000.0f, 200.0f, 600.0f, 1200.0f };
    float particleSizeScales[4] = { 1.0f, 0.6f, 0.3f, 0.425f };
    Vec vector;
    Vec vector2;
    Vec vector3;
    Vec vector4;
    s16 activeParticleCount = 0;

    objectState = particle->work;
    switch (objectState->objectState) {
        case 0:
            particleData = particle->data;
            if (objectState->animationState != 0) {
                vector4.x = 0.0f;
                vector4.y = -3600.0f;
                vector4.z = 6250.0f;
                distanceSquared = (vector4.z * vector4.z) + ((vector4.x * vector4.x) + (vector4.y * vector4.y));
                inverseDistance = 1.0f / sqrtf(distanceSquared);
                vector4.x *= inverseDistance;
                vector4.y *= inverseDistance;
                vector4.z *= inverseDistance;
                particleDepth = 6000.0f / particle->maxCnt;
                for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                    reflectionFactor = 500.0 + ((particleDepth * particleIndex) + ((0.25f * particleDepth) * (1.52587890625e-05 * (float)(s32)(u16)frand())));
                    particleHeight = reflectionFactor / 6000.0f;
                    particleData->accel.x = reflectionFactor;
                    particleData->pos.x = reflectionFactor * vector4.x;
                    particleData->pos.y = 4000.0f + (reflectionFactor * vector4.y);
                    particleData->pos.z = -6750.0f + (reflectionFactor * vector4.z);
                    particleData->scale = (0.3f + (0.7f * (particleHeight * particleHeight)))
                        * (particleHeightScales[objectState->animationState] * (0.699999988079071 + (0.30000001192092896 * (1.52587890625e-05 * (float)(s32)(u16)frand()))));
                    particleData->color.r = (s32)(160.0 * (1.52587890625e-05 * (float)GetRandomByte())) + 0x20;
                    particleData->color.g = (s32)(96.0 * (1.52587890625e-05 * (float)GetRandomByte())) + 0x20;
                    particleData->color.b = (s32)(128.0 * (1.52587890625e-05 * (float)GetRandomByte())) + 0x20;
                    particleData->color.a
                        = ((1.0 - (0.4 * particleHeight)) * (255.0f * particleSizeScales[objectState->animationState])) - (s32)(16.0 * (1.52587890625e-05 * (float)GetRandomByte()));
                }
                objectState->objectState++;
            }
            else {
                particleData->pos.x = 0.0f;
                particleData->pos.y = 4000.0f;
                particleData->pos.z = -6750.0f;
                particleData->scale = 1000.0f;
            }
            break;
        case 1:
            vector.x = sind(CRot.y) * cosd(CRot.x);
            vector.y = -sind(CRot.x);
            vector.z = cosd(CRot.y) * cosd(CRot.x);
            vector2.x = -(Center.x + (1.25f * (vector.x * CZoom)));
            vector2.y = 4000.0f - (Center.y + (vector.y * CZoom));
            vector2.z = -6750.0f - (Center.z + (1.25f * (vector.z * CZoom)));
            inverseDistance = 1.0f / sqrtf((vector2.z * vector2.z) + ((vector2.x * vector2.x) + (vector2.y * vector2.y)));
            vector2.x *= inverseDistance;
            vector2.y *= inverseDistance;
            vector2.z *= inverseDistance;
            inverseDistance = -((-vector2.z * -vector.z) + ((-vector2.x * -vector.x) + (-vector2.y * -vector.y)));
            vector3.x = -vector.x + (-2.0f * vector2.x * inverseDistance);
            vector3.y = -vector.y + (-2.0f * vector2.y * inverseDistance);
            vector3.z = -vector.z + (-2.0f * vector2.z * inverseDistance);
            for (particleData = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->pos.x = vector3.x * particleData->accel.x;
                particleData->pos.y = 4000.0f + (vector3.y * particleData->accel.x);
                particleData->pos.z = -6750.0f + (vector3.z * particleData->accel.x);
            }
            break;
    }
    DCFlushRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void DrawParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float spawnAngle;
    HU3DPARTICLEDATA *particleData;
    M425ObjectStateWork *objectState;
    s32 particleIndex;
    s16 activeParticleCount;

    activeParticleCount = 0;
    objectState = particle->work;
    switch (objectState->objectState) {
        case 0:
            for (particleData = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                particleData->time = particleData->parManId = 0;
            }
            objectState->objectState++;
            break;
        case 1:
            for (particleData = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
                if (particleData->time != 0) {
                    particleData->time--;
                }
                else {
                    if (particleData->parManId == 0) {
                        spawnAngle = 180.0 * (1.52587890625e-05 * (float)(s32)(u16)frand());
                        spawnAngle = spawnAngle;
                        if ((s32)(4.0 * (1.52587890625e-05 * (float)GetRandomByte())) != 0) {
                            particleData->pos.x = 100.0 * (4.0 * (1.52587890625e-05 * (float)(s32)(u16)frand())) * cosd(spawnAngle);
                        }
                        else {
                            particleData->pos.x = 100.0 * (2.0 * (1.52587890625e-05 * (float)(s32)(u16)frand())) * cosd(spawnAngle);
                        }
                        particleData->pos.y = -10.0f;
                        particleData->pos.z = -7000.0 + (100.0 * (40.0 * (1.52587890625e-05 * (float)(s32)(u16)frand())) * sind(spawnAngle));
                        particleData->zRot = 1.5707964f;
                        particleData->color.r = particleData->color.g = particleData->color.b = particleData->color.a = 0xC0;
                        particleData->color.b = 0xA0;
                        particleData->scale = 0.0f;
                        particleData->time = (s32)(90.0 * (1.52587890625e-05 * (float)GetRandomByte()));
                        particleData->parManId = 0x3C;
                        particleData->color.r = 0xC0;
                        particleData->color.g = 0xE0;
                        particleData->color.b = 0xE0;
                        particleData->vel.z = 0.01f * (0.05f * (3000.0f + -particleData->pos.z));
                        particleData->vel.z *= particleData->vel.z;
                        particleData->vel.z *= 0.125f;
                    }
                    particleData->color.a = 248.0 * sind((particleData->parManId * 3));
                    particleData->scale = (30.0f * particleData->vel.z) * sind((particleData->parManId * 3));
                    particleData->parManId--;
                }
            }
            if (particle->maxCnt == activeParticleCount) {
                Hu3DModelAttrReset(objectState->objectIndex, HU3D_ATTR_PARTICLE_KILL);
                Hu3DModelKill(objectState->objectIndex);
                return;
            }
    }
    DCFlushRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void SetVisualEffect(s16 modelId)
{
    M425ObjectVisualWork *visualWork;
    s32 displayListFrame;
    s32 vertexIndex;
    s32 normalIndex;
    HU3DMODEL *model;
    void *displayListBuffer;

    Vec2f textureCoordinate[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
    model = &Hu3DData[modelId];
    visualWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M425ObjectVisualWork), HU_MEMNUM_OVL);
    model->hookData = visualWork;
    visualWork->animationFrame = 1;
    visualWork->objectTimer.r = visualWork->objectTimer.g = visualWork->objectTimer.b = 0x40;
    visualWork->objectTimer.a = 0xFF;
    visualWork->objectProgress.a = visualWork->objectProgress.g = visualWork->objectProgress.b = 0xFF;
    visualWork->objectProgress.r = visualWork->objectProgress.b = 0;
    // maybe Mtx?
    visualWork->objectRotation = HuMemDirectMallocNum(HEAP_HEAP, visualWork->animationFrame * (4 * sizeof(Vec)), HU_MEMNUM_OVL);
    visualWork->objectRotation[0].x = -4500.0f;
    visualWork->objectRotation[0].y = 0.0f;
    visualWork->objectRotation[0].z = -7000.0f;
    visualWork->objectRotation[1].x = 4500.0f;
    visualWork->objectRotation[1].y = 0.0f;
    visualWork->objectRotation[1].z = -7000.0f;
    visualWork->objectRotation[2].x = 3000.0f;
    visualWork->objectRotation[2].y = -200.0f;
    visualWork->objectRotation[2].z = 0.0f;
    visualWork->objectRotation[3].x = -3000.0f;
    visualWork->objectRotation[3].y = -200.0f;
    visualWork->objectRotation[3].z = 0.0f;
    visualWork->objectScale = HuMemDirectMallocNum(HEAP_HEAP, visualWork->animationFrame * (4 * sizeof(GXColor)), HU_MEMNUM_OVL);
    visualWork->objectScale[0].r = 0xFF;
    visualWork->objectScale[0].g = 0xFF;
    visualWork->objectScale[0].b = 0xFF;
    visualWork->objectScale[0].a = 0xFF;
    visualWork->objectScale[1].r = 0xFF;
    visualWork->objectScale[1].g = 0xFF;
    visualWork->objectScale[1].b = 0xFF;
    visualWork->objectScale[1].a = 0xFF;
    visualWork->objectScale[2].r = 0xFF;
    visualWork->objectScale[2].g = 0xFF;
    visualWork->objectScale[2].b = 0xFF;
    visualWork->objectScale[2].a = 0xFF;
    visualWork->objectScale[3].r = 0xFF;
    visualWork->objectScale[3].g = 0xFF;
    visualWork->objectScale[3].b = 0xFF;
    visualWork->objectScale[3].a = 0xFF;
    visualWork->objectFlags = HuMemDirectMallocNum(HEAP_HEAP, visualWork->animationFrame * sizeof(Vec), HU_MEMNUM_OVL);
    visualWork->objectFlags->x = 0.0f;
    visualWork->objectFlags->y = 1.0f;
    visualWork->objectFlags->z = 0.0f;
    visualWork->objectSpeed = 2;
    visualWork->objectDirection = HuMemDirectMallocNum(HEAP_HEAP, visualWork->animationFrame * (visualWork->objectSpeed * sizeof(M425AnimationObjectWork)), HU_MEMNUM_OVL);
    visualWork->objectDirection[0].objectIndex = HuSprAnimReadFile(0x380003);
    visualWork->objectDirection[0].objectState = 0;
    visualWork->objectDirection[1].objectIndex = HuSprAnimReadFile(0x380004);
    visualWork->objectDirection[1].objectState = 0;
    visualWork->objectDirection[1].effectState = 0x80;
    for (displayListFrame = 0; displayListFrame < (visualWork->objectSpeed * visualWork->animationFrame); displayListFrame++) {
        MTXIdentity(visualWork->objectDirection[displayListFrame].objectRotation);
        visualWork->objectDirection[displayListFrame].objectModel.x = visualWork->objectDirection[displayListFrame].objectModel.y = visualWork->objectDirection[displayListFrame].objectModel.z = 0.0f;
    }
    for (displayListFrame = 0; displayListFrame < visualWork->objectSpeed; displayListFrame++) {
        AllocateTextureCoordinateArray(&visualWork->objectDirection[displayListFrame].animationMatrix, visualWork->animationFrame, textureCoordinate);
    }
    displayListBuffer = HuMemDirectMallocNum(HEAP_HEAP, 0x20000, HU_MEMNUM_OVL);
    visualWork->targetPosition = displayListBuffer;
    DCFlushRange(visualWork->targetPosition, 0x20000);
    GXBeginDisplayList(displayListBuffer, 0x20000);
    GXBegin(GX_QUADS, GX_VTXFMT0, visualWork->animationFrame * 4);
    for (displayListFrame = 0; displayListFrame < visualWork->animationFrame; displayListFrame++) {
        for (vertexIndex = 0; vertexIndex < 4; vertexIndex++) {
            GXPosition1x16(vertexIndex + (displayListFrame * 4));
            GXColor1x16(displayListFrame);
            GXTexCoord1x16(vertexIndex + (displayListFrame * 4));
            for (normalIndex = 0; normalIndex < visualWork->objectSpeed; normalIndex++) {
                GXNormal1x16(vertexIndex + (displayListFrame * 4));
            }
        }
    }
    visualWork->objectPosition = GXEndDisplayList();
}

void AllocateVectorArray(Vec **vector, s16 frameCount, Vec *sourceVertices)
{
    Vec *frameVertices;
    s32 frameIndex;
    s32 vertexIndex;

    frameVertices = *vector = HuMemDirectMallocNum(HEAP_HEAP, frameCount * (4 * sizeof(Vec)), HU_MEMNUM_OVL);
    for (frameIndex = 0; frameIndex < frameCount; frameIndex++) {
        for (vertexIndex = 0; vertexIndex < 4; frameVertices++, sourceVertices++, vertexIndex++) {
            frameVertices->x = sourceVertices->x;
            frameVertices->y = sourceVertices->y;
            frameVertices->z = sourceVertices->z;
        }
    }
}

void AllocateColorArray(GXColor **color, s16 frameCount, GXColor *sourceColors)
{
    GXColor *frameColors;
    s32 frameIndex;
    s32 colorIndex;

    frameColors = *color = HuMemDirectMallocNum(HEAP_HEAP, frameCount * (4 * sizeof(GXColor)), 0x10000000);
    for (frameIndex = 0; frameIndex < frameCount; frameIndex++) {
        for (colorIndex = 0; colorIndex < 4; frameColors++, sourceColors++, colorIndex++) {
            frameColors->r = sourceColors->r;
            frameColors->g = sourceColors->g;
            frameColors->b = sourceColors->b;
            frameColors->a = sourceColors->a;
        }
    }
}

void AllocatePositionArray(Vec **vector, s16 positionCount, Vec *sourcePositions)
{
    Vec *positions;
    s32 positionIndex;

    positions = *vector = HuMemDirectMallocNum(HEAP_HEAP, positionCount * sizeof(Vec), HU_MEMNUM_OVL);
    for (positionIndex = 0; positionIndex < positionCount; positions++, sourcePositions++, positionIndex++) {
        positions->x = sourcePositions->x;
        positions->y = sourcePositions->y;
        positions->z = sourcePositions->z;
    }
}

void AllocateTextureCoordinateArray(Vec2f **textureCoordinate, s16 frameCount, Vec2f *sourceTextureCoordinates)
{
    Vec2f *frameTextureCoordinates;
    s32 frameIndex;
    s32 textureCoordinateIndex;

    frameTextureCoordinates = *textureCoordinate = HuMemDirectMallocNum(HEAP_HEAP, frameCount * (4 * sizeof(Vec2f)), HU_MEMNUM_OVL);
    for (frameIndex = 0; frameIndex < frameCount; frameIndex++) {
        for (textureCoordinateIndex = 0; textureCoordinateIndex < 4; frameTextureCoordinates++, sourceTextureCoordinates++, textureCoordinateIndex++) {
            frameTextureCoordinates->x = sourceTextureCoordinates->x;
            frameTextureCoordinates->y = sourceTextureCoordinates->y;
        }
    }
}

void UpdateModelTransform(HU3DMODEL *model, Mtx matrix)
{
    float sine;
    float cosine;
    s32 modelIndex;
    M425ObjectVisualWork *visualWork;

    visualWork = model->hookData;
    visualWork->objectDirection[1].objectModel.y -= 0.0005f;
    sine = sind(frameCounter);
    cosine = cosd(frameCounter);
    for (modelIndex = 0; modelIndex < 1; modelIndex++) {
        visualWork->objectDirection[modelIndex].animationMatrix[0].x = 0.005f * sine * (modelIndex != 0 ? 10.0f : -1.0f);
        visualWork->objectDirection[modelIndex].animationMatrix[0].y = 0.005f * cosine * (modelIndex != 0 ? 1.0f : -1.0f);
        visualWork->objectDirection[modelIndex].animationMatrix[1].x = 1.0f + (0.005f * cosine * (modelIndex != 0 ? 10.0f : -1.0f));
        visualWork->objectDirection[modelIndex].animationMatrix[1].y = 0.005f * sine * (modelIndex != 0 ? 1.0f : -1.0f);
        visualWork->objectDirection[modelIndex].animationMatrix[2].x = 1.0f + (0.005f * sine * (modelIndex != 0 ? 10.0f : -1.0f));
        visualWork->objectDirection[modelIndex].animationMatrix[2].y = 1.0f + (0.005f * cosine * (modelIndex != 0 ? 1.0f : -1.0f));
        visualWork->objectDirection[modelIndex].animationMatrix[3].x = 0.005f * cosine * (modelIndex != 0 ? 10.0f : -1.0f);
        visualWork->objectDirection[modelIndex].animationMatrix[3].y = 1.0f + (0.005f * sine * (modelIndex != 0 ? 1.0f : -1.0f));
    }
    frameCounter += 1.0f;
    DrawModelWithEffect(model, matrix);
}

void DrawModelWithEffect(HU3DMODEL *model, Mtx matrix)
{
    Mtx matrix2;
    Mtx matrix3;
    GXColor color;
    s16 activeTextureMap;
    GXTexMapID textureMapIndex;
    M425ObjectVisualWork *visualWork;
    s16 tevStageCount;
    s16 textureGeneratorCount;
    s16 lightChannel;
    s32 textureMatrixIndex;
    s32 textureCoordinateIndex;
    s32 textureMatrix;

    visualWork = model->hookData;
    GXSetChanAmbColor(GX_COLOR0A0, visualWork->objectTimer);
    GXSetChanMatColor(GX_COLOR0A0, visualWork->objectProgress);
    GXLoadPosMtxImm(matrix, 0);
    MTXInverse(matrix, matrix2);
    MTXTranspose(matrix2, matrix3);
    GXLoadNrmMtxImm(matrix3, 0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetZMode(1, GX_LEQUAL, 1);
    tevStageCount = 0;
    textureGeneratorCount = 0;
    activeTextureMap = 0;
    for (textureMapIndex = GX_TEXMAP0; textureMapIndex < visualWork->objectSpeed; textureMapIndex++) {
        MTXRotRad(visualWork->objectDirection[textureMapIndex].objectRotation, 0x5A, 0.0f);
        mtxTransCat(
            visualWork->objectDirection[textureMapIndex].objectRotation, visualWork->objectDirection[textureMapIndex].objectModel.x, visualWork->objectDirection[textureMapIndex].objectModel.y, visualWork->objectDirection[textureMapIndex].objectModel.z);
        switch (visualWork->objectDirection[textureMapIndex].objectState) {
            case 0:
                GXLoadTexMtxImm(visualWork->objectDirection[textureMapIndex].objectRotation, texMtxTbl[textureGeneratorCount], GX_MTX2x4);
                textureMatrixIndex = texMtxTbl[textureGeneratorCount];
                GXSetTexCoordGen2(textureGeneratorCount, GX_TG_MTX2x4, textureMapIndex + 4, textureMatrixIndex, GX_FALSE, 0x7D);
                GXSetTevOrder(tevStageCount, textureGeneratorCount, textureMapIndex, GX_COLOR0A0);
                if (textureMapIndex == GX_TEXMAP0) {
                    GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
                    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
                }
                else {
                    color.a = visualWork->objectDirection[textureMapIndex].effectState;
                    GXSetTevColor(GX_TEVREG2, color);
                    GXSetTevColorIn(tevStageCount, GX_CC_CPREV, GX_CC_TEXC, GX_CC_ZERO, GX_CC_TEXC);
                    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
                }
                GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                break;
            case 1:
                GXLoadTexMtxImm(visualWork->objectDirection[textureMapIndex].objectRotation, texMtxTbl[textureGeneratorCount], GX_MTX2x4);
                textureCoordinateIndex = texMtxTbl[textureGeneratorCount];
                GXSetTexCoordGen2(textureGeneratorCount, GX_TG_MTX2x4, textureMapIndex + 4, textureCoordinateIndex, GX_FALSE, 0x7D);
                GXSetTevOrder(tevStageCount, textureGeneratorCount, textureMapIndex, GX_COLOR0A0);
                GXSetTevColor(GX_TEVREG2, color);
                GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_CPREV, GX_CC_TEXA, GX_CC_ZERO);
                GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_TEXA, GX_CA_APREV, GX_CA_ZERO);
                GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                break;
            case 2:
                color.a = 0xE0;
                GXSetTevColor(GX_TEVREG1, color);
                GXSetTexCoordGen2(textureGeneratorCount + 1, GX_TG_BUMP0, textureGeneratorCount, 0x3C, GX_FALSE, 0x7D);
                GXSetTevOrder(tevStageCount, textureGeneratorCount, textureMapIndex, GX_COLOR0A0);
                GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A1, GX_CC_RASC);
                GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
                GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
                tevStageCount++;
                GXSetTevOrder(tevStageCount, textureGeneratorCount + 1, GX_TEXMAP0, GX_COLOR0A0);
                GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_TEXC, GX_CC_A1, GX_CC_CPREV);
                GXSetTevColorOp(tevStageCount, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
                GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
                break;
            case 3:
                textureGeneratorCount--;
                GXLoadTexMtxImm(visualWork->objectDirection[textureMapIndex].objectRotation, texMtxTbl[textureGeneratorCount], GX_MTX2x4);
                textureMatrix = texMtxTbl[textureGeneratorCount];
                GXSetTexCoordGen2(textureGeneratorCount, GX_TG_MTX2x4, textureMapIndex + 4, textureMatrix, 0, 0x7D);
                GXSetTevOrder(tevStageCount, textureGeneratorCount, textureMapIndex, GX_COLOR0A0);
                GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_CPREV, GX_CC_TEXC, GX_CC_ZERO);
                GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
                GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
                textureGeneratorCount++;
                break;
        }
        tevStageCount++;
        textureGeneratorCount++;
        HuSprTexLoad(visualWork->objectDirection[textureMapIndex].objectIndex, 0, textureMapIndex, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    }
    color.a = visualWork->objectProgress.a;
    GXSetTevColor(GX_TEVREG0, color);
    GXSetTevOrder(tevStageCount, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(tevStageCount, GX_CC_ZERO, GX_CC_CPREV, GX_CC_RASC, GX_CC_ZERO);
    GXSetTevColorOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(tevStageCount, GX_CA_ZERO, GX_CA_APREV, GX_CA_A0, GX_CA_KONST);
    GXSetTevAlphaOp(tevStageCount, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    tevStageCount++;
    GXSetNumTevStages((s32)tevStageCount);
    GXSetNumTexGens((s32)textureGeneratorCount);
    lightChannel = Hu3DLightSet(model, &Hu3DCameraMtx, &Hu3DCameraMtxXPose, 0.0f);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, GX_SRC_VTX, lightChannel, GX_DF_CLAMP, GX_AF_SPOT);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0);
    GXSetArray(GX_VA_POS, visualWork->objectRotation, 0xC);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_TEX_ST, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, visualWork->objectScale, 4);
    for (textureMapIndex = GX_TEXMAP0; textureMapIndex < (visualWork->objectSpeed * visualWork->animationFrame); textureMapIndex++) {
        GXSetVtxDesc(textureMapIndex + 0xD, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, textureMapIndex + 0xD, GX_TEX_ST, GX_RGBA6, 0);
        GXSetArray(textureMapIndex + 0xD, visualWork->objectDirection[textureMapIndex].animationMatrix, 8);
    }
    GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_TEX_S, GX_RGBA6, 0);
    GXSetArray(GX_VA_NRM, visualWork->objectFlags, 0xC);
    GXCallDisplayList(visualWork->targetPosition, visualWork->objectPosition);
}

u32 randomSeed = 0x41C64E6D;

s32 GetRandomByte(void)
{
    randomSeed *= 0x41C64E6D;
    randomSeed += 0x3039;
    return randomSeed >> 0x10;
}

float EvaluateQuadraticBezier(float progress, float startValue, float controlValue, float endValue)
{
    float inverseProgress = 1.0 - progress;
    float interpolatedValue = (endValue * (progress * progress)) + ((startValue * (inverseProgress * inverseProgress)) + (2.0 * (controlValue * (inverseProgress * progress))));
    return interpolatedValue;
}

void EvaluateVectorQuadraticBezier(float *startValues, float *controlValues, float *endValues, float *outputValues, float progress)
{
    s32 componentIndex;

    for (componentIndex = 0; componentIndex < 3; componentIndex++) {
        *outputValues++ = EvaluateQuadraticBezier(progress, *startValues++, *controlValues++, *endValues++);
    }
}

float EvaluateCubicBezier(float progress, float startValue, float controlValue, float endValue)
{
    float derivativeValue = 2.0 * ((progress * endValue) + (((progress - 1.0) * startValue) + ((1.0 - (2.0 * progress)) * controlValue)));
    return derivativeValue;
}

void EvaluateVectorCubicBezier(float *startValues, float *controlValues, float *endValues, float *outputDirection, float progress)
{
    float directionComponents[3];
    float unusedScalar;
    float unusedTangentX;
    float unusedTangentY;
    float directionMagnitude;
    float unusedTangentZ;
    float unusedLength;
    float unusedMagnitude;
    float unusedScale;
    float unusedOutputX;
    float unusedOutputY;
    float unusedOutputZ;
    float unusedProgress;
    s32 componentIndex;

    for (componentIndex = 0; componentIndex < 3; componentIndex++) {
        directionComponents[componentIndex] = EvaluateCubicBezier(progress, *startValues++, *controlValues++, *endValues++);
    }

    directionMagnitude = sqrtf((directionComponents[2] * directionComponents[2]) + ((directionComponents[0] * directionComponents[0]) + (directionComponents[1] * directionComponents[1])));
    if (directionMagnitude) {
        directionMagnitude = 1.0 / directionMagnitude;
        directionMagnitude = directionMagnitude;
        for (componentIndex = 0; componentIndex < 3; componentIndex++) {
            *outputDirection++ = directionMagnitude * directionComponents[componentIndex];
        }
        return;
    }
    *outputDirection++ = 0.0f;
    *outputDirection++ = 0.0f;
    *outputDirection++ = 1.0f;
}

#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/esprite.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "ext_math.h"
#include "version.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/frand.h"
#endif

#define frand7() (frand() & 0x7F)

extern u32 GlobalCounter;

typedef struct {
    /* 0x00 */ s32 textureWidth;
    /* 0x04 */ s32 textureHeight;
    /* 0x08 */ s32 bufferSize;
    /* 0x0C */ s32 textureInitialized;
    /* 0x10 */ void *textureBuffer;
} CameraHookData; // Size 0x14

typedef struct {
    /* 0x00 */ Vec rotation;
    /* 0x0C */ Vec center;
    /* 0x18 */ float zoom;
} CameraPreset; // Size 0x1C

typedef struct {
    /* 0x00 */ CameraHookData *cameraHookData;
    /* 0x04 */ ANIMDATA *spriteAnimation;
} CameraModelHookData; // Size 8

typedef struct {
    /* 0x00 */ s16 objectIndex;
    /* 0x02 */ s16 objectState;
    /* 0x04 */ s16 objectPosition;
    /* 0x06 */ s16 objectFlags;
    /* 0x08 */ Vec position;
} ReflectionVertexData; // Size 0x14

typedef struct {
    /* 0x00 */ s16 objectIndex;
    /* 0x02 */ s16 objectState;
    /* 0x04 */ s16 objectPosition;
    /* 0x06 */ char reservedFlags[2];
    /* 0x08 */ Vec position;
    /* 0x14 */ Vec *targetPosition;
    /* 0x18 */ Vec *animationTimer;
    /* 0x1C */ GXColor *motionData;
    /* 0x20 */ ReflectionVertexData *vertexMetadata;
    /* 0x24 */ float planeParameters[4];
    /* 0x34 */ u32 effectState;
    /* 0x38 */ void *displayList;
} ReflectionDrawData; // Size 0x3C

typedef struct {
    /* 0x00 */ s16 objectIndex;
    /* 0x02 */ s16 objectState;
    /* 0x04 */ ReflectionDrawData *objectPosition;
} ReflectionModelHookData; // Size 8

typedef struct {
    /* 0x00 */ s16 objectIndex;
    /* 0x02 */ s16 objectState;
    /* 0x04 */ Vec objectPosition;
    /* 0x10 */ char positionZ[8];
    /* 0x18 */ float animationTimer;
    /* 0x1C */ char motionData[0xC];
    /* 0x28 */ float objectTimer;
    /* 0x2C */ float actionState;
    /* 0x30 */ float actionTimer;
    /* 0x34 */ float effectState;
    /* 0x38 */ Vec animationData;
    /* 0x44 */ GXColor objectColor;
    /* 0x48 */ s32 frameIndex;
} ModelAnimationState; // Size 0x4C

typedef struct StructM42305 {
    /* 0x00 */ s16 objectIndex;
    /* 0x02 */ s16 objectState;
    /* 0x00 */ char objectPosition[0x1C];
    /* 0x20 */ u8 objectFlags;
    /* 0x21 */ u8 animationState;
    /* 0x22 */ u8 animationFlags;
    /* 0x23 */ s8 activeState;
    /* 0x24 */ char animationData[2];
    /* 0x26 */ s16 animationFrame;
    /* 0x28 */ s32 objectTimer;
    /* 0x2C */ s32 actionState;
    /* 0x30 */ u32 actionTimer;
    /* 0x34 */ s32 effectState;
    /* 0x38 */ ANIMDATA *animationData;
    /* 0x3C */ ModelAnimationState *animationWork;
    /* 0x40 */ Vec *objectData;
    /* 0x44 */ float (*objectColor)[2];
    /* 0x48 */ void *objectState;
    /* 0x4C */ void *objectFlags;
    /* 0x50 */ HU3DMODEL *modelId;
} ModelAnimationHookData; // Size 0x54

typedef struct {
    /* 0x00 */ float objectIndex;
    /* 0x04 */ float objectPosition;
    /* 0x08 */ char position[4];
    /* 0x0C */ float positionX;
    /* 0x10 */ float positionZ;
    /* 0x14 */ char targetPosition[4];
    /* 0x18 */ float animationTimer;
    /* 0x1C */ float motionData;
    /* 0x20 */ char objectFlags[4];
    /* 0x24 */ float animationParameter;
    /* 0x28 */ float objectTimer;
    /* 0x2C */ char actionState[4];
    /* 0x30 */ float actionTimer;
    /* 0x34 */ float effectState;
    /* 0x38 */ char reservedTail[4];
} ModelAnimationRecord; // Size 0x3C

typedef struct {
    /* 0x00 */ float objectIndex;
    /* 0x04 */ float objectPosition;
    /* 0x08 */ float position;
    /* 0x0C */ float positionX;
    /* 0x10 */ float positionZ;
    /* 0x14 */ float targetPosition;
    /* 0x18 */ float animationTimer;
    /* 0x1C */ float motionData;
    /* 0x20 */ float objectFlags;
    /* 0x24 */ float animationData;
} ModelTransformRecord; // Size 0x28

typedef struct {
    /* 0x000 */ u16 objectIndex;
    /* 0x002 */ u16 objectState;
    /* 0x004 */ float objectPosition;
    /* 0x008 */ float position;
    /* 0x00C */ float positionX;
    /* 0x010 */ float positionZ;
    /* 0x014 */ Vec targetPosition;
} OverlayVertexData; // Size 0x20

typedef struct {
    /* 0x000 */ OverlayVertexData objectIndex[16];
    /* 0x200 */ ANIMDATA *animationData;
    /* 0x204 */ s32 animationState;
    /* 0x208 */ void *resourceData;
    /* 0x20C */ ModelAnimationRecord *animationTable;
    /* 0x210 */ ModelTransformRecord *transformTable;
    /* 0x214 */ GXColor *colorTable;
    /* 0x218 */ s32 sequenceId;
    /* 0x21C */ s32 sequenceState;
    /* 0x220 */ void *sequenceData;
} SceneRenderData; // Size 0x224

typedef struct {
    /* 0x000 */ s32 objectIndex;
    /* 0x004 */ s32 objectPosition;
    /* 0x008 */ s32 position;
    /* 0x00C */ s32 positionX;
    /* 0x010 */ s32 positionZ[24];
    /* 0x070 */ s32 rotationZ[24][3];
    /* 0x190 */ s16 playerPositions[10][2];
} OverlayAnimationData; // Size 0x1B8

typedef struct {
    /* 0x00 */ s16 objectIndex;
    /* 0x02 */ s16 effectIndex;
    /* 0x04 */ s16 objectPosition;
    /* 0x06 */ s16 isComputer;
    /* 0x08 */ s16 position;
    /* 0x0A */ s16 animationIndex;
    /* 0x0C */ s16 positionX;
    /* 0x0E */ s16 positionY;
    /* 0x10 */ s16 initialEffectState;
    /* 0x12 */ s16 rotationIndex;
    /* 0x14 */ s32 targetPosition;
    /* 0x18 */ s32 animationTimer;
    /* 0x1C */ s32 motionData;
    /* 0x20 */ Vec startPosition;
    /* 0x2C */ Vec actionState;
    /* 0x38 */ Vec animationData;
    /* 0x44 */ float rotationXAngle;
    /* 0x48 */ float rotationY;
    /* 0x4C */ float rotationZ;
    /* 0x50 */ Vec scale;
    /* 0x5C */ Vec angularVelocity;
    /* 0x68 */ Vec rotationX;
    /* 0x74 */ Vec positionData;
    /* 0x80 */ Vec previousScale;
    /* 0x8C */ float motionTimer;
    /* 0x90 */ float motionFlags;
} UiEffectData; // Size 0x94

typedef struct {
    /* 0x00 */ s16 objectIndex;
    /* 0x02 */ s16 objectState;
    /* 0x04 */ s16 objectPosition;
    /* 0x06 */ s16 objectFlags;
    /* 0x08 */ s16 position;
    /* 0x0A */ s16 animationIndex;
} PlayerSelectionData; // Size 0xC

typedef struct {
    /* 0x00 */ s16 objectIndex;
    /* 0x02 */ s16 characterId;
    /* 0x04 */ s16 objectPosition;
    /* 0x06 */ s16 isComputer;
    /* 0x08 */ s16 position;
    /* 0x0A */ s16 animationIndex;
    /* 0x0C */ s16 playerIndex;
    /* 0x0E */ s16 buttonDown;
    /* 0x10 */ s16 buttonPressed;
    /* 0x12 */ s16 rotationIndex;
    /* 0x14 */ s16 stickX;
    /* 0x16 */ s16 motionSlot;
    /* 0x18 */ s16 animationTimer;
    /* 0x1A */ char reservedState[2];
    /* 0x1C */ float motionData[15];
    /* 0x58 */ s32 motionDataIndex;
    /* 0x5C */ s32 uiEffectIndex;
    /* 0x60 */ float objectTimer;
    /* 0x64 */ s32 currentMotion;
    /* 0x68 */ s32 rotationX;
    /* 0x6C */ s32 rotationY;
    /* 0x70 */ s32 rotationZ;
    /* 0x74 */ s32 positionData;
    /* 0x78 */ float worldX;
    /* 0x7C */ float worldY;
    /* 0x80 */ s32 worldZ;
    /* 0x84 */ s32 motionState;
    /* 0x88 */ float motionSpeed;
    /* 0x8C */ float motionTimer;
    /* 0x90 */ s32 motionFlags;
    /* 0x94 */ s32 animationFlags;
    /* 0x98 */ s32 sequenceState;
    /* 0x9C */ s32 sequenceTimer;
    /* 0xA0 */ s32 sequenceIndex;
    /* 0xA4 */ s32 sequenceFlags;
    /* 0xA8 */ float sequenceProgress;
    /* 0xAC */ s32 effectState;
    /* 0xB0 */ float effectTimer;
    /* 0xB4 */ float effectProgress;
    /* 0xB8 */ float effectScale;
    /* 0xBC */ float effectAngle;
    /* 0xC0 */ float effectFlags;
    /* 0xC4 */ float effectSpeed;
    /* 0xC8 */ Vec startPosition;
    /* 0xD4 */ Vec targetPosition;
    /* 0xE0 */ float animationTime;
    /* 0xE4 */ float animationSpeed;
    /* 0xE8 */ float animationFrame;
    /* 0xEC */ float animationProgress;
    /* 0xF0 */ float frameTimer;
    /* 0xF4 */ float frameProgress;
} PlayerModelData; // Size 0xF8

typedef struct {
    /* 0x000 */ s16 objectIndex;
    /* 0x002 */ s16 objectState;
    /* 0x004 */ Vec objectPosition;
    /* 0x010 */ Vec positionZ;
    /* 0x004 */ char motionData[0xC];
    /* 0x028 */ float objectTimer;
    /* 0x02C */ float actionState;
    /* 0x030 */ float actionTimer;
    /* 0x034 */ float effectState;
    /* 0x038 */ char animationData[4];
    /* 0x03C */ float animationWork;
    /* 0x040 */ s32 objectData;
    /* 0x044 */ float objectColor[12][4];
    /* 0x104 */ Vec controlPoints[12][3];
} CollisionPrismData; // Size 0x2B4

typedef struct {
    /* 0x00 */ s32 objectIndex;
    /* 0x04 */ s32 objectPosition;
    /* 0x08 */ ANIMDATA *position;
} StageModelHookData; // Size 0xC

void InitializeSceneLighting(OMOBJ *object);
void UpdateMinigameScene(OMOBJ *object);
void BeginSceneTransition(OMOBJ *object);
void UpdateSceneFadeOut(OMOBJ *object);
void CreateCameraObject(void);
void UpdateCameraObject(OMOBJ *object);
void ReleaseModelHook(OMOBJ *object);
void ReleaseCameraResources(void);
void UpdateCameraHook(HU3DMODEL *model, Mtx transform);
s32 CreateCameraHook(CameraHookData *resourceData);
void DestroyCameraHook(s32 index);
void UpdateReflectionCameraHook(HU3DMODEL *model, Mtx transform);
s32 CreateReflectionHook(void);
void DrawModelWithReflection(HU3DMODEL *model, ReflectionDrawData *resourceData);
void DrawReflectionModelVariantA(HU3DMODEL *model, ReflectionDrawData *resourceData);
void DrawReflectionModelVariantB(HU3DMODEL *model, ReflectionDrawData *resourceData);
void DrawReflectionModelVariantC(HU3DMODEL *model, ReflectionDrawData *resourceData);
void SetModelHookState(s32 hookId);
void UpdateReflectionModelHook(HU3DMODEL *model, Mtx transform);
void DestroyModelHook(s32 index);
void CreatePlayerObjects(void);
void InitializePlayerObjects(OMOBJ *object);
void UpdatePlayerObjects(OMOBJ *object);
void StopPlayerObjects(void);
void DestroyPlayerObjects(void);
void InitializePlayerSelection(void);
void InitializePlayerModel(OMOBJ *object);
void UpdatePlayerInput(OMOBJ *object);
void UpdatePlayerAnimation(OMOBJ *object);
void StopEffectObjects(void);
s32 ResetEffectObjects(void);
s32 UpdateEffectObjectStates(void);
void SetEffectObjectState(s32 index);
void SetSceneEffect(s32 index, s32 state, s32 mode);
void UpdatePlayerModel(OMOBJ *object);
void UpdatePlayerModelAnimation(OMOBJ *object);
void CreateUiObjects(void);
void UpdateUiObject(OMOBJ *object);
void UpdateUiObjectStates(OMOBJ *object);
void StopUiObjectManager(void);
s32 CreateUiEffect(s32 playerIndex, Vec position, Vec targetPosition);
void SetUiEffectPosition(s32 effectIndex, Vec position);
void SetUiEffectState(s32 effectIndex, Vec state);
s32 GetUiEffectState(s32 object);
void SetUiEffectAnimation(s32 object);
UiEffectData *GetUiEffectObject(s32 object);
void DestroyUiEffects(void);
s32 FindFreeEffectSlot(UiEffectData **object);
void InitializeCollisionData(void);
void UpdateCollisionPrism(Vec *vertices, Vec *position, Vec *normal, float radius, s32 prismIndex, s32 flags);
s32 FindCollisionIntersection(Vec *vertices, Vec *startPosition, Vec *endPosition, Vec *intersection, float radius, s32 flags);
void CreateStageModel(void);
void UpdateStageModel(OMOBJ *object);
void DestroyStageModel(void);
s32 CheckStageCollision(Vec startPosition, Vec endPosition, float collisionRadius, float height, float depth, GXColor color);
s32 CheckStageCollisionRotated(Vec startPosition, Vec endPosition, float collisionRadius, float height, float depth, float rotation, GXColor color);
void CreateBackgroundModels(void);
void UpdateBackgroundModels(OMOBJ *object);
void EmptyBackgroundCallback(void);
s32 CheckBackgroundCollision(Vec startPosition, Vec endPosition, float collisionRadius);
void CreateShadowModel(void);
void UpdateShadowModel(OMOBJ *object);
void DestroyShadowModel(void);
s32 CreateCaptureHook(void);
void CaptureModelTexture(HU3DMODEL *model, Mtx transform);
s32 SetShadowModelPosition(Vec position, float shadowScale, s32 shadowState);
void CreateOverlayObjects(void);
void UpdateOverlayObject(OMOBJ *object);
void UpdateOverlayAnimation(OMOBJ *object);
void DestroyOverlayObjects(void);
void SetOverlayState(s32 object);
void InitializeCameras(void);
void UpdateCameraRotation(OMOBJ *object);
void InterpolateCameraPreset(s32 cameraIndex, s32 startPresetIndex, s32 targetPresetIndex, f32 transitionSpeed);
void SetSceneCamera(s32 presetIndex);
void UpdateCameraMatrices(void);
void InitializeTextureObject(CameraHookData *textureData, void *imageData, s16 textureWidth, GXTexWrapMode wrapMode, s32 textureFormat, s32 textureFlags);
s16 CreateModelAnimationHook(ANIMDATA *animation, s16 animationIndex);
void UpdateModelAnimationCallback(HU3DMODEL *model, Mtx transform);
float GetShortestAngleDifference(float angle, float targetAngle);
float ApproachAngle(float currentAngle, float targetAngle, float maxStep);
float ApproachAngleBySpeed(float angularSpeed, float currentAngle, float targetAngle);
void TransformVectorByMatrix(Mtx matrix, Vec *vector);
s32 CheckSegmentPlaneIntersection(Vec *startPosition, Vec *endPosition, float *plane);
float CheckPointInsidePolygon(Vec *polygonVertices, Vec *point, s32 axis);
void CalculatePlaneFromPoints(Vec *pointA, Vec *pointB, Vec *pointC, float *plane);
void IntersectLineWithPlane(float *plane, Vec *lineStart, Vec *lineEnd, Vec *intersection, Vec *distance);
float GetPointToSegmentDistance(Vec *pointA, Vec *pointB, Vec *point, Vec *closestPoint, f32 *projectionRatio);
s32 CheckSegmentBoxIntersection(Vec *boxCorners, Vec *startPosition, Vec *direction, Vec *collisionPoint, float collisionRadius, s32 faceIndex);

HUPROCESS *objectManager;
s32 minigameSequenceId;
s32 minigameTimerId;
s32 minigameState;
s32 minigameStateFrame;
s32 unusedMinigameState; // unused
s32 minigameCountdown;
s32 playerSelectionTimer;
s32 selectedPlayerIndex;
s32 sceneFlags;
s32 playerCharacterIds[4];
s32 playerScores[4];
s32 winnerAnnouncerSequenceId;
OMOBJ *sceneControllerObject;
OMOBJ *cameraObject;
OMOBJ *playerObjectManager;
OMOBJ *playerEffectObjects[4];
OMOBJ *effectControllerObject;
OMOBJ *playerModelObjects[3];
OMOBJ *uiEffectObjectManager;
OMOBJ *stageModelObject;
OMOBJ *backgroundModelObject;
OMOBJ *shadowModelObject;
OMOBJ *overlayObject;
CollisionPrismData *collisionPrisms;
s32 cameraPresetIndex;
OMOBJ *activePlayerObject;
s32 sceneTimer;
s32 sceneSelection;
s32 sceneFrameCounter;
s32 sceneSubstate;
UiEffectData *uiEffectData[15];
UiEffectData *uiEffectWork[15];
s32 overlayState;

CameraPreset introCameraPresets[]
    = { { { 342.45f, 0.0f, 0.0f }, { 0.0f, 275.0f, 128.0f }, 550.0f }, { { 6.0f, 0.0f, 0.0f }, { 0.0f, 190.0f, 0.0f }, 550.0f } };

CameraPreset playerCameraPresets[]
    = { { { 22.0f, 180.0f, 0.0f }, { -250.0f, 120.00001f, 0.0f }, 160.0f }, { { 22.0f, 180.0f, 0.0f }, { 0.0f, 120.00001f, 0.0f }, 160.0f },
          { { 22.0f, 180.0f, 0.0f }, { 250.0f, 120.00001f, 0.0f }, 160.0f }, { { -45.0f, 180.0f, 0.0f }, { -250.0f, 75.0f, 0.0f }, 180.0f },
          { { -45.0f, 180.0f, 0.0f }, { 0.0f, 75.0f, 0.0f }, 180.0f }, { { -45.0f, 180.0f, 0.0f }, { 250.0f, 75.0f, 0.0f }, 180.0f },
          { { -45.0f, 0.0f, 0.0f }, { -250.0f, 95.0f, 0.0f }, 150.0f }, { { -45.0f, 0.0f, 0.0f }, { 0.0f, 95.0f, 0.0f }, 150.0f },
          { { -45.0f, 0.0f, 0.0f }, { 250.0f, 95.0f, 0.0f }, 150.0f }, { { -45.0f, 0.0f, 0.0f }, { 0.0f, 95.0f, -850.0f }, 150.0f } };

Vec playerCameraOffsets[] = { 0.0f, 0.0f, -850.0f, -250.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 250.0f, 0.0f, 0.0f };

Vec cameraTrackOffsets[] = { 0.0f, 0.0f, -850.0f, 850.0f, 0.0f, 0.0f, 1200.0f, 0.0f, 0.0f, 1650.0f, 0.0f, 0.0f };

Vec playerSpawnPositions[] = { { -250.0f, 45.0f, 500.0f }, { 0.0f, 45.0f, 500.0f }, { 250.0f, 45.0f, 500.0f } };

Vec shadowLightPosition = { 1000.0f, 2000.0f, 1000.0f };
Vec shadowLightDirection = { 0.0f, 1.0f, 0.0f };
Vec shadowTargetPosition = { 0.0f, 0.0f, -450.0f };

void ObjectSetup(void)
{
    HUPROCESS *stateValue;
    HU3DCAMERA *camera;
    s32 i;

    OSReport("******* M426ObjectSetup *********\n");
    stateValue = omInitObjMan(50, 0x2000);
    objectManager = stateValue;
    omGameSysInit(stateValue);
    InitializeCameras();
    Hu3DLightAllKill();
    Hu3DLighInit();
    HuAudSndGrpSet(0x30);
    sceneControllerObject = omAddObjEx(stateValue, 0xFF, 2, 0, -1, InitializeSceneLighting);
    minigameSequenceId = minigameTimerId = -1;
    cameraObject = NULL;
    playerObjectManager = NULL;
    uiEffectObjectManager = NULL;
    effectControllerObject = NULL;
    playerModelObjects[0] = playerModelObjects[1] = playerModelObjects[2] = NULL;
    minigameState = 0;
    minigameStateFrame = 0;
    minigameCountdown = 0;
    playerSelectionTimer = 10;
    selectedPlayerIndex = -1;
    sceneFlags = 0;
    cameraPresetIndex = -1;
    winnerAnnouncerSequenceId = -1;
    for (i = 0; i < 4; i++) {
        playerScores[i] = 0;
        playerCharacterIds[i] = -1;
    }
    Hu3DShadowCreate(30.0f, 1000.0f, 250000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowPosSet(&shadowLightPosition, &shadowLightDirection, &shadowTargetPosition);
    for (i = 0; i < 3; i++) {
        camera = &Hu3DCamera[i];
        camera->fov = -1.0f;
    }
}

Vec sceneLightPosition = { 0.0f, 0.0f, 0.0f };
Vec sceneLightDirection = { -1.0f, -2.0f, -1.0f };
GXColor whiteLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };

void InitializeSceneLighting(OMOBJ *object)
{
    s32 resourceId;

    resourceId = Hu3DGLightCreateV(&sceneLightPosition, &sceneLightDirection, &whiteLightColor);
    Hu3DGLightStaticSet(resourceId, 1);
    Hu3DGLightInfinitytSet(resourceId);
    CreateCameraObject();
    CreatePlayerObjects();
    InitializePlayerSelection();
    CreateUiObjects();
    CreateStageModel();
    CreateBackgroundModels();
    CreateShadowModel();
    CreateOverlayObjects();
    HuAudFXPlay(0x618);
    object->objFunc = UpdateMinigameScene;
}

s32 minigamePlayerOrder[] = { 0, 4, 2, 3, 1, 5 };
s32 minigameSequenceDurations[] = {
    3.1 * REFRESH_RATE, 3.1 * REFRESH_RATE, 3.1 * REFRESH_RATE,
    4 * REFRESH_RATE, 4 * REFRESH_RATE, 4 * REFRESH_RATE
};

void UpdateMinigameScene(OMOBJ *object)
{
    HU3DCAMERA *camera;
    float floatValue;
    s32 index;
    s32 stateFlags;
    s32 stateValue;
    s32 i;

    switch (minigameState) {
        case 0:
            InterpolateCameraPreset(2, 1, 1, 0.0f);
            if (++minigameStateFrame > 3) {
                for (i = 0; i < 3; i++) {
                    camera = &Hu3DCamera[i];
                    camera->fov = 45.0f;
                }
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
                minigameState = 1;
                minigameStateFrame = 0;
            }
            break;
        case 1:
            InterpolateCameraPreset(2, 1, 1, 0.0f);
            if (WipeStatGet() == 0) {
                minigameState = 2;
                minigameStateFrame = 0;
            }
            break;
        case 2:
            if (++minigameStateFrame >= REFRESH_RATE_F / 2) {
                minigameState = 3;
                minigameStateFrame = 0;
            }
            break;
        case 3:
            floatValue = minigameStateFrame / (REFRESH_RATE_F * 1.5f);
            if (floatValue > 1.0f) {
                floatValue = 1.0f;
            }
            floatValue = sind(90.0f * floatValue);
            floatValue *= floatValue;
            InterpolateCameraPreset(2, 1, 0, floatValue);
            if (++minigameStateFrame >= (REFRESH_RATE_F * 1.5f)) {
                minigameState = 4;
                minigameStateFrame = 0;
            }
            break;
        case 4:
            if (ResetEffectObjects() != 0) {
                minigameSequenceId = MGSeqStartCreate();
                minigameState = 5;
                minigameStateFrame = 0;
            }
            break;
        case 5:
            if (minigameSequenceId != -1) {
                stateFlags = MGSeqStatGet(minigameSequenceId);
                if ((stateFlags & 0x10) && winnerAnnouncerSequenceId == -1) {
                    winnerAnnouncerSequenceId = HuAudSeqPlay(0x46);
                }
            }
            else {
                stateFlags = 0;
            }
            if (minigameSequenceId == -1 || stateFlags == 0) {
                minigameSequenceId = -1;
                minigameState = 6;
                minigameStateFrame = 0;
                minigameCountdown = REFRESH_RATE * 30;
                SetOverlayState(1);
                minigameTimerId = MGSeqTimerCreate(minigameCountdown / REFRESH_RATE);
            }
            break;
        case 6:
            index = minigamePlayerOrder[overlayState];
            SetSceneCamera(index);
            if (sceneFrameCounter >= minigameSequenceDurations[index] && ++overlayState > 5) {
                overlayState = 0;
            }
            stateValue = (minigameCountdown + REFRESH_RATE - 1) / REFRESH_RATE;
            if (stateValue < 0) {
                stateValue = 0;
            }
            MGSeqParamSet(minigameTimerId, 1, stateValue);
            if (--minigameCountdown < -1 || playerSelectionTimer <= 0) {
                MGSeqParamSet(minigameTimerId, 2, -1);
                minigameTimerId = -1;
                DestroyUiEffects();
                if (winnerAnnouncerSequenceId != -1) {
                    HuAudSeqFadeOut(winnerAnnouncerSequenceId, 100);
                }
                minigameSequenceId = MGSeqFinishCreate();
                minigameState = 7;
                minigameStateFrame = 0;
            }
            break;
        case 7:
            if (UpdateEffectObjectStates() != 0 && (minigameSequenceId == -1 || MGSeqStatGet(minigameSequenceId) == 0)) {
                minigameState = 9;
                minigameStateFrame = 0;
            }
            break;
        case 9:
            if (playerSelectionTimer <= 0) {
                minigameSequenceId = MGSeqWinCreate(playerCharacterIds[0], playerCharacterIds[1], playerCharacterIds[2], -1);
                SetEffectObjectState(1);
                for (i = 0; i < 4; i++) {
                    if (playerCharacterIds[3] != GWPlayerCfg[i].character) {
                        GWPlayerCoinWinAdd(i, 10);
                    }
                }
                if (playerScores[0] > playerScores[1] && playerScores[0] > playerScores[2]) {
                    i = 0;
                }
                else if (playerScores[1] > playerScores[0] && playerScores[1] > playerScores[2]) {
                    i = 1;
                }
                else if (playerScores[2] > playerScores[0] && playerScores[2] > playerScores[1]) {
                    i = 2;
                }
                else {
                    i = frand() & 3;
                    if (i > 2) {
                        i = 2;
                    }
                    else if (i < 0) {
                        i = 0;
                    }
                }
                SetSceneCamera(i + 6);
                minigameState = 10;
                minigameCountdown = 0;
            }
            else {
                minigameSequenceId = MGSeqWinCreate(playerCharacterIds[3], -1, -1, -1);
                SetEffectObjectState(0);
                for (i = 0; i < 4; i++) {
                    if (playerCharacterIds[3] == GWPlayerCfg[i].character) {
                        GWPlayerCoinWinAdd(i, 10);
                    }
                }
                SetSceneCamera(9);
                minigameState = 10;
                minigameCountdown = 0;
            }
            HuAudSStreamPlay(1);
            HuAudFXPlay(0x618);
            break;
        case 10:
            DestroyPlayerObjects();
            if (++minigameCountdown >= REFRESH_RATE_F * 3.5f) {
                minigameSequenceId = -1;
                minigameState = 11;
                minigameCountdown = 0;
            }
            break;
        case 11:
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
            object->objFunc = BeginSceneTransition;
            break;
    }
    UpdateSceneFadeOut(object);
}

void BeginSceneTransition(OMOBJ *object)
{
    if (WipeStatGet() == 0) {
        ReleaseCameraResources();
        StopPlayerObjects();
        StopEffectObjects();
        StopUiObjectManager();
        DestroyStageModel();
        EmptyBackgroundCallback();
        DestroyShadowModel();
        DestroyOverlayObjects();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void UpdateSceneFadeOut(OMOBJ *object)
{
    s32 stateValue;

    stateValue = 0;
    if (WipeStatGet() != 0 || object->objFunc == BeginSceneTransition) {
        return;
    }
    if (omSysExitReq != 0) {
        stateValue = 1;
    }
    if (stateValue != 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        HuAudStreamFadeOut(30);
        object->objFunc = BeginSceneTransition;
    }
}

s32 GetSceneState(void)
{
    return minigameState;
}

void CreateCameraObject(void)
{
    cameraObject = omAddObjEx(objectManager, 0x7F, 8, 0, -1, UpdateCameraObject);
}

void UpdateCameraObject(OMOBJ *object)
{
    CameraHookData *stateValue;
    HU3DMODEL *model;

    stateValue = object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*stateValue), HU_MEMNUM_OVL);
    memset(stateValue, 0, sizeof(*stateValue));
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M423, 0));
    Hu3DModelCameraSet(object->mdlId[0], 5);
    Hu3DModelShadowMapSet(object->mdlId[0]);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M423, 1));
    Hu3DModelCameraSet(object->mdlId[1], 5);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M423, 2));
    Hu3DModelScaleSet(object->mdlId[2], 1.125f, 1.0f, 1.0f);
    Hu3DModelCameraSet(object->mdlId[2], 5);
    Hu3DModelShadowSet(object->mdlId[2]);
    object->mdlId[3] = Hu3DHookFuncCreate(UpdateCameraHook);
    Hu3DModelCameraSet(object->mdlId[3], 2);
    model = &Hu3DData[object->mdlId[3]];
    model->hookData = stateValue;
    stateValue->textureWidth = 160;
    stateValue->textureHeight = 160;
    stateValue->bufferSize = GXGetTexBufferSize(stateValue->textureWidth, stateValue->textureHeight, 5, 0, 0);
    stateValue->textureBuffer = HuMemDirectMallocNum(HEAP_MODEL, stateValue->bufferSize, (u32)model->mallocNo);
    stateValue->textureInitialized = 0.0f;
    object->mdlId[4] = CreateCameraHook(stateValue);
    Hu3DModelCameraSet(object->mdlId[4], 5);
    Hu3DModelPosSet(object->mdlId[4], 0.0f, 200.0f, -1700.0f);
    object->mdlId[5] = CreateReflectionHook();
    Hu3DModelCameraSet(object->mdlId[5], 4);
    object->objFunc = ReleaseModelHook;
}

void ReleaseModelHook(OMOBJ *object)
{
    DestroyModelHook(object->mdlId[5]);
}

void ReleaseModelResource(OMOBJ *object)
{
    DestroyCameraHook(object->mdlId[4]);
    object->mdlId[4] = -1;
    SetModelHookState(object->mdlId[5]);
    object->mdlId[5] = -1;
    object->objFunc = NULL;
}

void ReleaseCameraResources(void)
{
    ReleaseModelResource(cameraObject);
}

GXColor framebufferClearColor = { 0x00, 0x00, 0x00, 0xFF };

void UpdateCameraHook(HU3DMODEL *model, Mtx transform)
{
    CameraHookData *resourceData;
    HU3DCAMERA *camera;

    resourceData = model->hookData;
    camera = &Hu3DCamera[1];
    if (camera->fov <= 0.0f) {
        return;
    }
    GXSetTexCopySrc(0, 0, 320, 240);
    GXSetTexCopyDst(resourceData->textureWidth, resourceData->textureHeight, GX_TF_RGB5A3, GX_TRUE);
    GXSetCopyClear(framebufferClearColor, 0xFFFFFF);
    GXCopyTex(resourceData->textureBuffer, GX_TRUE);
    DCFlushRange(resourceData->textureBuffer, resourceData->bufferSize);
    GXPixModeSync();
    resourceData->textureInitialized = 1;
    UpdateCameraMatrices();
}

s32 CreateCameraHook(CameraHookData *sourceCameraData)
{
    CameraModelHookData *resourceData;
    HU3DMODEL *model;
    s32 index;

    index = Hu3DHookFuncCreate(UpdateReflectionCameraHook);
    model = &Hu3DData[index];
    resourceData = model->hookData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(*resourceData), model->mallocNo);
    memset(resourceData, 0, sizeof(*resourceData));
    resourceData->cameraHookData = sourceCameraData;
    resourceData->spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M423, 21), HU_MEMNUM_OVL));
    return index;
}

void DestroyCameraHook(s32 index)
{
    CameraModelHookData *resourceData;
    HU3DMODEL *model;

    if (index >= 0) {
        model = &Hu3DData[index];
        resourceData = model->hookData;
        HuSprAnimKill(resourceData->spriteAnimation);
    }
}

f32 reflectionQuadVertices[] = { -400.0f, 400.0f, 0.0f, 400.0f, 400.0f, 0.0f, 400.0f, 0.0f, 0.0f, -400.0f, 0.0f, 0.0f };

f32 reflectionTextureCoordinates[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.75f, 0.0f, 0.75f };

f32 reflectionHighlightCoordinates[] = { 0.0f, 0.0f, 1.2f, 0.0f, 1.2f, 0.75f, 0.0f, 0.75f };

GXColor reflectionVertexColors[] = { { 0x40, 0x40, 0x40, 0xFF } };

GXColor reflectionOverlayColor = { 0x00, 0x00, 0x00, 0x80 };

void UpdateReflectionCameraHook(HU3DMODEL *model, Mtx transform)
{
    HU3DCAMERA *camera;
    CameraModelHookData *resourceData;
    CameraHookData *resourceData2;
    Mtx normalMatrix;
    s16 i;

    camera = &Hu3DCamera[2];
    resourceData = model->hookData;
    resourceData2 = resourceData->cameraHookData;
    if (resourceData2->textureInitialized != 0) {
        if (camera->fov <= 0.0f) {
            return;
        }
        GXLoadPosMtxImm(transform, GX_PNMTX0);
        MTXInvXpose(transform, normalMatrix);
        GXLoadNrmMtxImm(normalMatrix, GX_PNMTX0);
        GXSetTevColor(GX_TEVREG2, reflectionOverlayColor);
        GXSetNumTevStages(2);
        GXSetNumTexGens(2);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX1, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
        GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
        GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        if (Hu3DCameraNo != 0) {
            GXSetTevOp(GX_TEVSTAGE1, GX_REPLACE);
            GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_CPREV, GX_CC_C2, GX_CC_TEXA, GX_CC_ZERO);
            GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        }
        else {
            GXSetTevOp(GX_TEVSTAGE1, GX_REPLACE);
            GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_CPREV, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
            GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        }
        GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_APREV, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
        InitializeTextureObject(resourceData2, resourceData2->textureBuffer, GX_TEXMAP0, GX_CLAMP, 0, 1);
        HuSprTexLoad(resourceData->spriteAnimation, 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
        GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
        GXSetZCompLoc(GX_FALSE);
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetArray(GX_VA_POS, reflectionQuadVertices, 12);
        GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GXSetArray(GX_VA_CLR0, reflectionVertexColors, 4);
        GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
        GXSetArray(GX_VA_TEX0, reflectionTextureCoordinates, 8);
        GXSetVtxDesc(GX_VA_TEX1, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
        GXSetArray(GX_VA_TEX1, reflectionHighlightCoordinates, 8);
        GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        for (i = 0; i < 4; i++) {
            GXPosition1x16(i);
            GXColor1x16(0);
            GXTexCoord1x16(i);
            GXTexCoord1x16(i);
        }
        GXEnd();
    }
}

s32 reflectionViewportSizes[][2] = { { 20, 10 }, { 20, 7 }, { 10, 10 }, { 10, 10 } };

Vec reflectionCameraPositions[] = { { 0.0f, 100.0f, -1100.0f }, { 0.0f, 200.0f, -1000.0f }, { -225.0f, 100.0f, -1000.0f }, { 225.0f, 100.0f, -1000.0f } };

s32 CreateReflectionHook(void)
{
    HU3DMODEL *model;
    ReflectionModelHookData *resourceData;
    ReflectionDrawData *resourceData2;
    s32 index;
    s32 i;

    index = Hu3DHookFuncCreate(UpdateReflectionModelHook);
    model = &Hu3DData[index];
    resourceData = model->hookData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(*resourceData), model->mallocNo);
    memset(resourceData, 0, sizeof(*resourceData));
    resourceData->objectIndex = index;
    resourceData->objectState = 4;
    resourceData2 = resourceData->objectPosition = HuMemDirectMallocNum(HEAP_MODEL, resourceData->objectState * sizeof(*resourceData2), model->mallocNo);
    memset(resourceData2, 0, resourceData->objectState * sizeof(*resourceData2));
    for (i = 0; i < resourceData->objectState; i++) {
        resourceData2 = &resourceData->objectPosition[i];
        resourceData2->objectState = reflectionViewportSizes[i][0];
        resourceData2->objectPosition = reflectionViewportSizes[i][1];
        resourceData2->objectIndex = resourceData2->objectState * resourceData2->objectPosition;
        resourceData2->position = reflectionCameraPositions[i];
        resourceData2->targetPosition = HuMemDirectMallocNum(HEAP_MODEL, resourceData2->objectIndex * sizeof(*resourceData2->targetPosition), model->mallocNo);
        memset(resourceData2->targetPosition, 0, resourceData2->objectIndex * sizeof(*resourceData2->targetPosition));
        resourceData2->animationTimer = HuMemDirectMallocNum(HEAP_MODEL, resourceData2->objectIndex * sizeof(*resourceData2->animationTimer), model->mallocNo);
        memset(resourceData2->animationTimer, 0, resourceData2->objectIndex * sizeof(*resourceData2->animationTimer));
        resourceData2->motionData = HuMemDirectMallocNum(HEAP_MODEL, resourceData2->objectIndex * sizeof(*resourceData2->motionData), model->mallocNo);
        memset(resourceData2->motionData, 0, resourceData2->objectIndex * sizeof(*resourceData2->motionData));
        resourceData2->vertexMetadata = HuMemDirectMallocNum(HEAP_MODEL, resourceData2->objectIndex * sizeof(*resourceData2->vertexMetadata), model->mallocNo);
        memset(resourceData2->vertexMetadata, 0, resourceData2->objectIndex * sizeof(*resourceData2->vertexMetadata));
        switch (i) {
            case 0:
                DrawModelWithReflection(model, resourceData2);
                break;
            case 1:
                DrawReflectionModelVariantA(model, resourceData2);
                break;
            case 2:
                DrawReflectionModelVariantB(model, resourceData2);
                break;
            default:
                DrawReflectionModelVariantC(model, resourceData2);
                break;
        }
    }
    return index;
}

Vec reflectionCameraTargets[] = { { -200.0f, 0.0f, -1100.0f }, { -200.0f, 200.0f, -1050.0f }, { 200.0f, 200.0f, -1050.0f } };

GXColor reflectionModelColor = { 0xFF, 0xFF, 0xFF, 0xFF };

void DrawModelWithReflection(HU3DMODEL *model, ReflectionDrawData *reflectionData)
{
    float floatValue;
    float floatValue2;
    float floatValue3;
    Vec *position;
    GXColor *color;
    ReflectionVertexData *vertexData;
    void *resourceData2;
    void *resourceData3;
    s32 i;
    s32 j;

    position = reflectionData->targetPosition;
    color = reflectionData->motionData;
    vertexData = reflectionData->vertexMetadata;
    for (i = 0; i < 10; i++) {
        floatValue2 = 200.0f * (i / 9.0f);
        floatValue3 = -1100.0f + 50.0f * (i / 9.0f);
        for (j = 0; j < 20; position++, color++, vertexData++, j++) {
            floatValue = -225.0f + 450.0f * (j / 19.0f);
            position->x = floatValue;
            position->y = floatValue2;
            position->z = floatValue3;
            *color = reflectionModelColor;
            vertexData->objectIndex = j;
            vertexData->objectState = i;
            if (i == 0 || i == 9 || j == 0 || j == 19) {
                vertexData->objectPosition = 1;
            }
            else {
                vertexData->objectPosition = 0;
            }
            vertexData->objectFlags = 0;
            vertexData->position.x = vertexData->position.y = vertexData->position.z = 0.0f;
        }
    }
    memcpy(reflectionData->animationTimer, reflectionData->targetPosition, reflectionData->objectIndex * sizeof(*reflectionData->targetPosition));
    DCFlushRangeNoSync(reflectionData->targetPosition, reflectionData->objectIndex * sizeof(*reflectionData->targetPosition));
    DCFlushRangeNoSync(reflectionData->motionData, reflectionData->objectIndex * sizeof(*reflectionData->motionData));
    PPCSync();
    resourceData3 = resourceData2 = HuMemDirectMallocNum(HEAP_MODEL, 0x10000, model->mallocNo);
    DCFlushRange(resourceData2, 0x10000);
    GXBeginDisplayList(resourceData3, 0x10000);
    GXBegin(GX_LINES, GX_VTXFMT0, 9 * 19 * 4);
    for (i = 1; i < 10; i++) {
        for (j = 1; j < 20; j++) {
            GXPosition1x16(i * 20 + j);
            GXColor1x16(i * 20 + j);

            GXPosition1x16(i * 20 + (j - 1));
            GXColor1x16(i * 20 + (j - 1));

            GXPosition1x16(i * 20 + j);
            GXColor1x16(i * 20 + j);

            GXPosition1x16((i - 1) * 20 + j);
            GXColor1x16((i - 1) * 20 + j);
        }
    }
    GXEnd();
    reflectionData->effectState = GXEndDisplayList();
    if (reflectionData->effectState >= 0x10000) {
        OSReport("ERROR! GList test size over! \n");
    }
    reflectionData->displayList = HuMemDirectMallocNum(HEAP_MODEL, reflectionData->effectState, model->mallocNo);
    memcpy(reflectionData->displayList, resourceData2, reflectionData->effectState);
    DCFlushRange(reflectionData->displayList, reflectionData->effectState);
    HuMemDirectFree(resourceData2);
    CalculatePlaneFromPoints(&reflectionCameraTargets[0], &reflectionCameraTargets[1], &reflectionCameraTargets[2], reflectionData->planeParameters);
}

GXColor reflectionVariantAColor = { 0xFF, 0xFF, 0xFF, 0xFF };

void DrawReflectionModelVariantA(HU3DMODEL *model, ReflectionDrawData *reflectionData)
{
    float floatValue;
    float floatValue2;
    float floatValue3;
    Vec *position;
    GXColor *color;
    void *displayListBuffer;
    void *displayListStart;
    s32 i;
    s32 j;

    position = reflectionData->targetPosition;
    color = reflectionData->motionData;
    for (i = 0; i < 7; i++) {
        floatValue2 = 200.0f;
        floatValue3 = -900.0f - 150.0f * (i / 6.0f);
        for (j = 0; j < 20; position++, color++, j++) {
            floatValue = -225.0f + 450.0f * (j / 19.0f);
            position->x = floatValue;
            position->y = floatValue2;
            position->z = floatValue3;
            *color = reflectionVariantAColor;
        }
    }
    memcpy(reflectionData->animationTimer, reflectionData->targetPosition, reflectionData->objectIndex * sizeof(*reflectionData->targetPosition));
    DCFlushRangeNoSync(reflectionData->targetPosition, reflectionData->objectIndex * sizeof(*reflectionData->targetPosition));
    DCFlushRangeNoSync(reflectionData->motionData, reflectionData->objectIndex * sizeof(*reflectionData->motionData));
    PPCSync();
    displayListStart = displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, 0x10000, model->mallocNo);
    DCFlushRange(displayListBuffer, 0x10000);
    GXBeginDisplayList(displayListStart, 0x10000);
    GXBegin(GX_LINES, GX_VTXFMT0, 6 * 19 * 4);
    for (i = 1; i < 7; i++) {
        for (j = 1; j < 20; j++) {
            GXPosition1x16(i * 20 + j);
            GXColor1x16(i * 20 + j);

            GXPosition1x16(i * 20 + (j - 1));
            GXColor1x16(i * 20 + (j - 1));

            GXPosition1x16(i * 20 + j);
            GXColor1x16(i * 20 + j);

            GXPosition1x16((i - 1) * 20 + j);
            GXColor1x16((i - 1) * 20 + j);
        }
    }
    GXEnd();
    reflectionData->effectState = GXEndDisplayList();
    if (reflectionData->effectState >= 0x10000) {
        OSReport("ERROR! GList test size over! \n");
    }
    reflectionData->displayList = HuMemDirectMallocNum(HEAP_MODEL, reflectionData->effectState, model->mallocNo);
    memcpy(reflectionData->displayList, displayListBuffer, reflectionData->effectState);
    DCFlushRange(reflectionData->displayList, reflectionData->effectState);
    HuMemDirectFree(displayListBuffer);
}

GXColor reflectionVariantBColor = { 0xFF, 0xFF, 0xFF, 0xFF };

void DrawReflectionModelVariantB(HU3DMODEL *model, ReflectionDrawData *reflectionData)
{
    float floatValue;
    float floatValue2;
    float floatValue3;
    float floatValue4;
    Vec *position;
    GXColor *color;
    void *displayListBuffer;
    void *displayListStart;
    s32 i;
    s32 j;

    position = reflectionData->targetPosition;
    color = reflectionData->motionData;
    for (i = 0; i < 10; i++) {
        floatValue4 = 200.0f * (i / 9.0f);
        floatValue3 = -225.0f;
        floatValue2 = -1100.0f + 50.0f * (i / 9.0f);
        for (j = 0; j < 10; position++, color++, j++) {
            floatValue = -900.0f - 200.0f * (j / 9.0f);
            if (floatValue < floatValue2) {
                floatValue = floatValue2;
            }
            position->x = floatValue3;
            position->y = floatValue4;
            position->z = floatValue;
            *color = reflectionVariantBColor;
        }
    }
    memcpy(reflectionData->animationTimer, reflectionData->targetPosition, reflectionData->objectIndex * sizeof(*reflectionData->targetPosition));
    DCFlushRangeNoSync(reflectionData->targetPosition, reflectionData->objectIndex * sizeof(*reflectionData->targetPosition));
    DCFlushRangeNoSync(reflectionData->motionData, reflectionData->objectIndex * sizeof(*reflectionData->motionData));
    PPCSync();
    displayListStart = displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, 0x10000, model->mallocNo);
    DCFlushRangeNoSync(displayListBuffer, 0x10000);
    GXBeginDisplayList(displayListStart, 0x10000);
    GXBegin(GX_LINES, GX_VTXFMT0, 9 * 9 * 4);
    for (i = 1; i < 10; i++) {
        for (j = 1; j < 10; j++) {
            GXPosition1x16(i * 10 + j);
            GXColor1x16(i * 10 + j);

            GXPosition1x16(i * 10 + (j - 1));
            GXColor1x16(i * 10 + (j - 1));

            GXPosition1x16(i * 10 + j);
            GXColor1x16(i * 10 + j);

            GXPosition1x16((i - 1) * 10 + j);
            GXColor1x16((i - 1) * 10 + j);
        }
    }
    GXEnd();
    reflectionData->effectState = GXEndDisplayList();
    if (reflectionData->effectState >= 0x10000) {
        OSReport("ERROR! GList test size over! \n");
    }
    reflectionData->displayList = HuMemDirectMallocNum(HEAP_MODEL, reflectionData->effectState, model->mallocNo);
    memcpy(reflectionData->displayList, displayListBuffer, reflectionData->effectState);
    DCFlushRange(reflectionData->displayList, reflectionData->effectState);
    HuMemDirectFree(displayListBuffer);
}

GXColor reflectionVariantCColor = { 0xFF, 0xFF, 0xFF, 0xFF };

void DrawReflectionModelVariantC(HU3DMODEL *model, ReflectionDrawData *reflectionData)
{
    float floatValue;
    float floatValue2;
    float floatValue3;
    float floatValue4;
    Vec *position;
    GXColor *color;
    void *displayListBuffer;
    void *displayListStart;
    s32 i;
    s32 j;

    position = reflectionData->targetPosition;
    color = reflectionData->motionData;
    for (i = 0; i < 10; i++) {
        floatValue4 = 200.0f * (i / 9.0f);
        floatValue3 = 225.0f;
        floatValue2 = -1100.0f + 50.0f * (i / 9.0f);
        for (j = 0; j < 10; position++, color++, j++) {
            floatValue = -900.0f - 200.0f * (j / 9.0f);
            if (floatValue < floatValue2) {
                floatValue = floatValue2;
            }
            position->x = floatValue3;
            position->y = floatValue4;
            position->z = floatValue;
            *color = reflectionVariantCColor;
        }
    }
    memcpy(reflectionData->animationTimer, reflectionData->targetPosition, reflectionData->objectIndex * sizeof(*reflectionData->targetPosition));
    DCFlushRangeNoSync(reflectionData->targetPosition, reflectionData->objectIndex * sizeof(*reflectionData->targetPosition));
    DCFlushRangeNoSync(reflectionData->motionData, reflectionData->objectIndex * sizeof(*reflectionData->motionData));
    PPCSync();
    displayListStart = displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, 0x10000, model->mallocNo);
    DCFlushRange(displayListBuffer, 0x10000);
    GXBeginDisplayList(displayListStart, 0x10000);
    GXBegin(GX_LINES, GX_VTXFMT0, 9 * 9 * 4);
    for (i = 1; i < 10; i++) {
        for (j = 1; j < 10; j++) {
            GXPosition1x16(i * 10 + j);
            GXColor1x16(i * 10 + j);

            GXPosition1x16(i * 10 + (j - 1));
            GXColor1x16(i * 10 + (j - 1));

            GXPosition1x16(i * 10 + j);
            GXColor1x16(i * 10 + j);

            GXPosition1x16((i - 1) * 10 + j);
            GXColor1x16((i - 1) * 10 + j);
        }
    }
    GXEnd();
    reflectionData->effectState = GXEndDisplayList();
    if (reflectionData->effectState >= 0x10000) {
        OSReport("ERROR! GList test size over! \n");
    }
    reflectionData->displayList = HuMemDirectMallocNum(HEAP_MODEL, reflectionData->effectState, model->mallocNo);
    memcpy(reflectionData->displayList, displayListBuffer, reflectionData->effectState);
    DCFlushRange(reflectionData->displayList, reflectionData->effectState);
    HuMemDirectFree(displayListBuffer);
}

void SetModelHookState(s32 hookId) { }

void UpdateReflectionModelHook(HU3DMODEL *model, Mtx transform)
{
    ReflectionModelHookData *resourceData;
    ReflectionDrawData *resourceData2;
    Mtx transformMatrix;
    s16 i;

    resourceData = model->hookData;
    GXLoadPosMtxImm(transform, GX_PNMTX0);
    MTXInvXpose(transform, transformMatrix);
    GXLoadNrmMtxImm(transformMatrix, GX_PNMTX0);
    GXSetLineWidth(6, GX_TO_ZERO);
    GXSetNumTevStages(1);
    GXSetNumTexGens(0);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZCompLoc(GX_FALSE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    for (i = 0; i < resourceData->objectState; i++) {
        resourceData2 = &resourceData->objectPosition[i];
        if (resourceData2->displayList && resourceData2->effectState != 0) {
            DCFlushRangeNoSync(resourceData2->targetPosition, resourceData2->objectIndex * sizeof(*resourceData2->targetPosition));
            DCFlushRangeNoSync(resourceData2->motionData, resourceData2->objectIndex * sizeof(*resourceData2->motionData));
            PPCSync();
            GXSetArray(GX_VA_POS, resourceData2->targetPosition, sizeof(*resourceData2->targetPosition));
            GXSetArray(GX_VA_CLR0, resourceData2->motionData, sizeof(*resourceData2->motionData));
            GXCallDisplayList(resourceData2->displayList, resourceData2->effectState);
        }
    }
}

void DestroyModelHook(s32 modelId)
{
    Vec reflectionPosition;
    Vec positionVector;
    Vec positionVector2;
    Vec *position;
    Vec *position2;
    Vec *position3;
    Vec *position4;
    float floatValue;
    float floatValue2;
    float floatValue3;
    float floatValue4;
    float floatValue5;
    ReflectionDrawData *resourceData;
    ReflectionVertexData *resourceData2;
    UiEffectData *resourceData3;
    ReflectionModelHookData *resourceData4;
    HU3DMODEL *scratchData;
    HU3DMODEL *reflectionModel;
    ReflectionVertexData *scratchData2;
    s32 index;
    s32 stateValue;
    s32 stateValue2;
    s32 i;
    s32 j;
    s32 k;
    s32 index2;
    s32 scratchData3;
    s32 scratchData4;
    s32 collisionFound;
    Mtx transformMatrix;

    scratchData = &Hu3DData[modelId];
    resourceData4 = scratchData->hookData;
    resourceData = resourceData4->objectPosition;
    position = resourceData->targetPosition;
    position2 = resourceData->animationTimer;
    resourceData2 = resourceData->vertexMetadata;
    if (uiEffectObjectManager == NULL) {
        return;
    }
    resourceData3 = uiEffectObjectManager->data;
    for (i = 0; i < resourceData->objectIndex; i++, position++, position2++, resourceData2++) {
        if (resourceData2->objectPosition != 0) {
            continue;
        }
        VECSubtract(position2, position, &positionVector);
        floatValue = VECMag(&positionVector);
        if (floatValue >= 0.0f) {
            VECScale(&positionVector, &positionVector, 0.05f);
            VECAdd(&resourceData2->position, &positionVector, &resourceData2->position);
            floatValue = VECMag(&resourceData2->position);
            if (floatValue > 10.0f) {
                VECNormalize(&resourceData2->position, &resourceData2->position);
                VECScale(&resourceData2->position, &resourceData2->position, 10.0f);
            }
        }
        VECAdd(position, &resourceData2->position, position);
        VECScale(&resourceData2->position, &resourceData2->position, 0.5f);
    }
    for (i = 0; i < 15; i++, resourceData3++) {
        if (resourceData3->isComputer == 0 || resourceData3->positionX != 3) {
            continue;
        }
        collisionFound = 0;
        VECSubtract(&resourceData3->animationData, &resourceData3->positionData, &positionVector);
        if (resourceData3->animationData.x > -225.0f && resourceData3->animationData.x < 225.0f && resourceData3->animationData.y > 0.0f && resourceData3->animationData.y < 200.0f
            && resourceData3->animationData.z > -1150.0f && resourceData3->animationData.z < -950.0f) {
            floatValue5 = 0.005f * resourceData3->animationData.y;
            floatValue4 = -900.0f - (150.0f + floatValue5 * 50.0f);
            if (resourceData3->animationData.z - 20.0 < floatValue4) {
                if (resourceData3->animationData.z < 20.0 + (floatValue4 - 75.0f)) {
                    resourceData3->animationData.z = 20.0 + (floatValue4 - 75.0f);
                }
                collisionFound = 1;
            }
        }
        if (collisionFound != 0) {
            if (resourceData3->animationIndex == 0 && resourceData3->position == 0) {
                SetUiEffectAnimation(i);
            }
            if (resourceData3->initialEffectState == 0) {
                resourceData3->angularVelocity = resourceData3->scale;
                resourceData3->initialEffectState = 1;
            }
            reflectionModel = &Hu3DData[resourceData3->objectIndex];
            positionVector2.x = resourceData3->angularVelocity.z;
            positionVector2.z = -resourceData3->angularVelocity.x;
            positionVector2.y = 0.0f;
            floatValue = 0.05 * VECMag(&positionVector2);
            if (floatValue > 0.00001f) {
                MTXRotAxisRad(transformMatrix, &positionVector2, floatValue);
                MTXConcat(transformMatrix, reflectionModel->mtx, reflectionModel->mtx);
            }
            floatValue = 0.0133333336f * fabs(floatValue4 - (resourceData3->animationData.z - 20.0));
            resourceData3->scale.z += floatValue * 0.4f;
            VECScale(&resourceData3->scale, &resourceData3->scale, 0.95f);
            resourceData3->scale.y *= 0.0f;
            resourceData3->scale.x *= 0.75f;
            scratchData3 = (resourceData3->animationData.x - -225.0f) / 22.5f;
            scratchData4 = resourceData3->animationData.y / 20.0f;
            index = scratchData3 + scratchData4 * resourceData->objectState;
            position = &resourceData->targetPosition[index];
            position2 = &resourceData->animationTimer[index];
            resourceData2 = &resourceData->vertexMetadata[index];
            floatValue2 = position2->z - (resourceData3->animationData.z - 20.0);
            reflectionPosition = resourceData3->animationData;
            if (floatValue2 < 0.0f) {
                continue;
            }
            for (j = -19; j < 20; j++) {
                for (k = -19; k < 20; k++) {
                    stateValue = scratchData3 + k;
                    stateValue2 = scratchData4 + j;
                    index2 = stateValue + stateValue2 * resourceData->objectState;
                    if (stateValue < 0 || stateValue >= resourceData->objectState || stateValue2 < 0 || stateValue2 >= resourceData->objectPosition || index2 >= resourceData->objectIndex) {
                        continue;
                    }
                    scratchData2 = &resourceData->vertexMetadata[index2];
                    if (scratchData2->objectPosition != 0) {
                        continue;
                    }
                    position4 = &resourceData->targetPosition[index2];
                    position3 = &resourceData->animationTimer[index2];
                    VECSubtract(&reflectionPosition, position4, &positionVector);
                    positionVector.z = 0.0f;
                    floatValue = VECMag(&positionVector);
                    if (floatValue > 400.0f) {
                        continue;
                    }
                    if (floatValue > 20.0) {
                        floatValue3 = 0.0025f * (400.0f - floatValue);
                        floatValue3 = floatValue3 * floatValue3;
                        VECNormalize(&positionVector, &positionVector);
                        VECScale(&positionVector, &positionVector, 20.0f * (floatValue3 * floatValue3));
                        position4->x += 0.5f * (position3->x + positionVector.x - position4->x);
                        position4->y += 0.5f * (position3->y + positionVector.y - position4->y);
                        position4->z += 0.5f * (position3->z - floatValue3 * floatValue2 - position4->z);
                    }
                    else {
                        floatValue3 = cosd(0.0025f * floatValue * 90.0f);
                        if (floatValue != 0.0f) {
                            VECNormalize(&positionVector, &positionVector);
                        }
                        else {
                            positionVector.x = positionVector.y = positionVector.z = 0.0f;
                        }
                        VECScale(&positionVector, &positionVector, floatValue3 * floatValue3 * 20.0f);
                        position4->z += 0.5f * (position3->z - floatValue3 * floatValue2 - position4->z);
                    }
                }
            }
        }
        else {
            resourceData3->initialEffectState = 0;
        }
    }
    DCFlushRangeNoSync(resourceData->targetPosition, resourceData->objectIndex * sizeof(*resourceData->targetPosition));
    PPCSync();
}

void CreatePlayerObjects(void)
{
    playerObjectManager = omAddObjEx(objectManager, 0x81, 20, 6, -1, InitializePlayerObjects);
}

Vec playerModelVertices[] = { -1300.0f, 300.0f, -2350.0f, -800.0f, 200.0f, -2250.0f, 800.0f, 200.0f, -2250.0f, 1300.0f, 300.0f, -2350.0f, -1000.0f, 300.0f,
    -2350.0f, 1000.0f, 300.0f, -2350.0f, -1200.0f, 400.0f, -2450.0f, -800.0f, 400.0f, -2450.0f, 800.0f, 400.0f, -2450.0f, 1200.0f, 400.0f, -2450.0f,
    -1400.0f, 200.0f, -2250.0f, 1400.0f, 200.0f, -2250.0f, -1400.0f, 400.0f, -2450.0f, 1400.0f, 400.0f, -2450.0f, -900.0f, 500.0f, -2550.0f, 900.0f,
    500.0f, -2550.0f, -1200.0f, 500.0f, -2550.0f, 1200.0f, 500.0f, -2550.0f, -1100.0f, 200.0f, -2250.0f, 1100.0f, 200.0f, -2250.0f };

s32 playerModelTriangleIndices[] = { 0, 1, 0, 1, 2, 2, 1, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 1, 0 };

s32 playerModelResourceIds[] = { DATA_MAKE_NUM(DATADIR_M423, 12), DATA_MAKE_NUM(DATADIR_M423, 15), DATA_MAKE_NUM(DATADIR_M423, 18) };

s32 playerAnimationResourceIds[] = { DATA_MAKE_NUM(DATADIR_M423, 13), DATA_MAKE_NUM(DATADIR_M423, 14), DATA_MAKE_NUM(DATADIR_M423, 16),
    DATA_MAKE_NUM(DATADIR_M423, 17), DATA_MAKE_NUM(DATADIR_M423, 19), DATA_MAKE_NUM(DATADIR_M423, 20) };

s16 playerAnimationIndices[] = { 0, 2, 4 };

void InitializePlayerObjects(OMOBJ *object)
{
    float floatValue;
    float floatValue2;
    float floatValue3;
    PlayerSelectionData *resourceData;
    s32 index;
    s32 resourceId;
    s32 i;

    resourceData = object->data = HuMemDirectMallocNum(HEAP_HEAP, 20 * sizeof(*resourceData), HU_MEMNUM_OVL);
    memset(resourceData, 0, 20 * sizeof(*resourceData));
    for (i = 0; i < 6; i++) {
        object->mtnId[i] = Hu3DMotionCreateFile(playerAnimationResourceIds[i]);
    }
    for (i = 0; i < 20; i++, resourceData++) {
        index = playerModelTriangleIndices[i];
        resourceData->objectIndex = index;
        resourceData->frameIndex = 0;
        resourceData->objectPosition = 0;
        resourceData->position = 0;
        resourceData->animationIndex = 0;
        resourceData->objectFlags = (REFRESH_RATE_F * 3.5f) + (1.0f / 127) * frand7() * REFRESH_RATE_F;
        floatValue = -playerModelVertices[i].x;
        floatValue2 = -playerModelVertices[i].z;
        floatValue3 = atan2d(floatValue, floatValue2);
        resourceId = Hu3DModelCreateFile(playerModelResourceIds[index]);
        object->mdlId[i] = resourceId;
        Hu3DModelCameraSet(resourceId, 4);
        Hu3DModelPosSet(resourceId, playerModelVertices[i].x, playerModelVertices[i].y, playerModelVertices[i].z);
        if (resourceData->objectIndex != 1) {
            Hu3DModelRotSet(resourceId, 0.0f, floatValue3, 0.0f);
        }
        else {
            Hu3DModelRotSet(resourceId, 10.0f, floatValue3, 0.0f);
        }
        Hu3DModelScaleSet(resourceId, 1.8f, 1.8f, 1.8f);
        Hu3DMotionSet(object->mdlId[i], object->mtnId[playerAnimationIndices[resourceData->objectIndex]]);
        if ((i >= 14 && i <= 17) || i == 11 || i == 10) {
            Hu3DModelAttrSet(resourceId, 1);
        }
    }
    object->stat |= 0x100;
    object->objFunc = UpdatePlayerObjects;
}

s16 playerAnimationSequenceIndices[] = { 0, 2, 4 };
s16 playerAnimationSequenceOrder[] = { 1, 3, 5, 0 };

void UpdatePlayerObjects(OMOBJ *object)
{
    PlayerSelectionData *resourceData;
    s32 stateValue;
    s32 i;

    resourceData = object->data;
    for (i = 0; i < 20; i++, resourceData++) {
        stateValue = 0;
        if (resourceData->objectFlags > 0) {
            stateValue = 1;
            resourceData->objectFlags--;
        }
        else if (resourceData->animationIndex > 0) {
            if (resourceData->position > 0) {
                resourceData->position--;
            }
            else if (resourceData->animationIndex > 0) {
                stateValue = 1;
                resourceData->animationIndex--;
            }
            else {
                resourceData->position = 0;
                resourceData->animationIndex = 0;
            }
        }
        if (stateValue != resourceData->objectState && Hu3DMotionShiftIDGet(object->mdlId[i]) == -1) {
            resourceData->objectState = stateValue;
            if (resourceData->objectState == 0) {
                Hu3DMotionShiftSet(object->mdlId[i], object->mtnId[playerAnimationSequenceIndices[resourceData->objectIndex]], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
            }
            else {
                Hu3DMotionShiftSet(object->mdlId[i], object->mtnId[playerAnimationSequenceOrder[resourceData->objectIndex]], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
            }
        }
    }
}

void StopPlayerObject(OMOBJ *object)
{
    object->objFunc = NULL;
}

void StopPlayerObjects(void)
{
    StopPlayerObject(playerObjectManager);
}

void DestroyPlayerObjects(void)
{
    OMOBJ *object;
    PlayerSelectionData *resourceData;
    s32 i;

    if (playerObjectManager == NULL) {
        return;
    }
    object = playerObjectManager;
    resourceData = object->data;
    for (i = 0; i < 20; i++, resourceData++) {
        if (resourceData->objectFlags > 0 || resourceData->animationIndex > 0) {
            resourceData->position = 0;
        }
        else {
            resourceData->position = (1.0f / 127) * frand7() * 0.25f * REFRESH_RATE_F;
        }
        resourceData->animationIndex = ((1.0f / 127) * frand7() + 2.0f) * REFRESH_RATE_F;
    }
}

void InitializePlayerSelection(void)
{
    s32 scratchData[4];
    s32 index;
    s32 index2;
    s32 stateValue;
    s32 index3;
    s32 index4;
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        scratchData[i] = -1;
    }
    for (i = 0, index4 = 0; i < 4; i++) {
        if (GWPlayerCfgGroupGet(i) == 0) {
            scratchData[3] = i;
        }
        else {
            scratchData[index4] = i;
            index4++;
        }
    }
    for (i = 0; i < 4; i++) {
        if (scratchData[i] == -1) {
            break;
        }
    }
    if (i < 4) {
        scratchData[0] = 1;
        scratchData[1] = 2;
        scratchData[2] = 3;
        scratchData[3] = 0;
    }
    else {
        for (i = 0; i < 32; i++) {
            index = (s16)(frand() & 0xFFF) % 3;
            index2 = (s16)(frand() & 0xFFF) % 3;
            if (index != index2) {
                index4 = scratchData[index2];
                scratchData[index2] = scratchData[index];
                scratchData[index] = index4;
            }
        }
    }
    stateValue = 1;
    for (i = 0, index3 = 0; i < 4; i++) {
        playerEffectObjects[i] = omAddObjEx(objectManager, 0x40, 4, 11, -1, InitializePlayerModel);
        playerEffectObjects[i]->work[0] = i;
        if (i == scratchData[3]) {
            playerCharacterIds[3] = GWPlayerCfg[playerEffectObjects[i]->work[0]].character;
            playerEffectObjects[i]->work[1] = 0;
            playerEffectObjects[i]->work[2] = 0;
            if (GWPlayerCfg[playerEffectObjects[i]->work[0]].iscom) {
                sceneFlags = 1;
            }
        }
        else {
            playerCharacterIds[index3] = GWPlayerCfg[playerEffectObjects[i]->work[0]].character;
            index3++;
            playerEffectObjects[i]->work[1] = 1;
            for (j = 0; j < 3; j++) {
                if (i == scratchData[j]) {
                    break;
                }
            }
            if (j >= 3) {
                j = i - 1;
            }
            playerEffectObjects[i]->work[2] = j;
            playerModelObjects[j] = playerEffectObjects[i];
            if (!GWPlayerCfg[playerEffectObjects[i]->work[0]].iscom) {
                stateValue = 0;
            }
        }
    }
    if (stateValue != 0) {
        sceneFlags = 0;
    }
}

u32 playerModelResourceTable[][2]
    = { { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), -1 }, { DATA_MAKE_NUM(DATADIR_MARIOMOT, 2), -1 }, { DATA_MAKE_NUM(DATADIR_MARIOMOT, 3), -1 }, { 0, 0 },
          { 0, 1 }, { 0, 2 }, { DATA_MAKE_NUM(DATADIR_MARIOMOT, 23), -1 }, { DATA_MAKE_NUM(DATADIR_MARIOMOT, 24), -1 } };

s32 playerResultResourceIds[] = { DATA_MAKE_NUM(DATADIR_M423, 58), DATA_MAKE_NUM(DATADIR_M423, 26), DATA_MAKE_NUM(DATADIR_M423, 34) };

u32 playerMotionResourceTable[][2] = { { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), -1 }, { DATA_MAKE_NUM(DATADIR_MARIOMOT, 2), -1 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 3), -1 }, { 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 }, { DATA_MAKE_NUM(DATADIR_MARIOMOT, 11), -1 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 6), -1 }, { DATA_MAKE_NUM(DATADIR_MARIOMOT, 23), -1 }, { DATA_MAKE_NUM(DATADIR_MARIOMOT, 24), -1 } };

s32 playerAnimationIds[]
    = { DATA_MAKE_NUM(DATADIR_M423, 42), DATA_MAKE_NUM(DATADIR_M423, 66), DATA_MAKE_NUM(DATADIR_M423, 50), DATA_MAKE_NUM(DATADIR_M423, 74) };

void InitializePlayerModel(OMOBJ *object)
{
    float floatValue;
    PlayerModelData *resourceData;
    s32 i;

    resourceData = object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*resourceData), HU_MEMNUM_OVL);
    memset(resourceData, 0, sizeof(*resourceData));
    resourceData->objectIndex = object->work[0];
    resourceData->objectState = GWPlayerCfg[object->work[0]].character;
    resourceData->objectPosition = GWPlayerCfg[object->work[0]].pad_idx;
    resourceData->objectFlags = GWPlayerCfg[object->work[0]].iscom;
    resourceData->position = GWPlayerCfg[object->work[0]].diff;
    resourceData->animationIndex = object->work[1];
    resourceData->positionX = object->work[2];
    memset(&resourceData->motionData, 0, sizeof(resourceData->motionData));
    resourceData->motionDataIndex = 0;
    resourceData->uiEffectIndex = -1;
    resourceData->objectTimer = 0.0f;
    resourceData->currentMotion = -1;
    resourceData->rotationX = 0;
    resourceData->rotationY = 0;
    resourceData->rotationZ = 0;
    resourceData->positionData = 0;
    resourceData->motionSlot = -1;
    resourceData->motionSpeed = 0.0f;
    resourceData->motionTimer = 0.0f;
    resourceData->motionFlags = 0;
    resourceData->animationFlags = 0;
    resourceData->sequenceState = 0;
    resourceData->sequenceTimer = 0;
    resourceData->sequenceIndex = 0;
    resourceData->sequenceFlags = 0;
    resourceData->sequenceProgress = 0.0f;
    resourceData->effectState = 0;
    object->mdlId[0] = CharModelCreate(resourceData->objectState, 4);
    Hu3DModelCameraSet(object->mdlId[0], 5);
    Hu3DModelShadowSet(object->mdlId[0]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_CURVE_MOTOFF);
    CharModelStepFxSet(resourceData->characterId, 2);
    if (resourceData->animationIndex == 0) {
        for (i = 0; i < 11; i++) {
            if (playerMotionResourceTable[i][1] == -1) {
                object->mtnId[i] = CharMotionCreate(resourceData->objectState, playerMotionResourceTable[i][0]);
            }
            else {
                object->mtnId[i] = Hu3DJointMotionFile(object->mdlId[0], playerAnimationIds[playerMotionResourceTable[i][1]] + resourceData->objectState);
            }
        }
        resourceData->motionSlot = 0;
        resourceData->animationTimer = 0;
        CharMotionSet(resourceData->objectState, object->mtnId[resourceData->animationState]);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
        resourceData->effectTimer = object->trans.x = playerCameraOffsets[0].x;
        resourceData->effectProgress = object->trans.y = playerCameraOffsets[0].y;
        resourceData->effectScale = object->trans.z = playerCameraOffsets[0].z;
        resourceData->effectAngle = cameraTrackOffsets[0].x;
        resourceData->effectFlags = cameraTrackOffsets[0].y;
        resourceData->effectSpeed = cameraTrackOffsets[0].z;
        resourceData->worldX = 0.0f;
        resourceData->worldY = 0.0f;
        resourceData->animationTime = resourceData->animationSpeed = resourceData->animationFrame = 0.0f;
        resourceData->animationProgress = resourceData->frameTimer = resourceData->frameProgress = 0.0f;
        resourceData->worldY = 6.5f;
        floatValue = 0.0f;
        for (i = 0; i < 1000; i++) {
            floatValue += resourceData->worldY;
            resourceData->worldY -= (VERSION_PAL) ? 29.4f / REFRESH_RATE_OFF_BY_1 : 29.4f * REFRESH_FREQ;
            if (floatValue <= 0.0f) {
                break;
            }
        }
        resourceData->worldZ = i;
        resourceData->worldY = 10.5f;
        floatValue = 0.0f;
        for (i = 0; i < 1000; i++) {
            floatValue += resourceData->worldY;
            resourceData->worldY -= (VERSION_PAL) ? 39.2f / REFRESH_RATE_F : 0.65333337f;
            if (floatValue <= 0.0f) {
                break;
            }
        }
        resourceData->motionState = i;
        object->objFunc = UpdatePlayerAnimation;
    }
    else {
        for (i = 0; i < 8; i++) {
            if (playerModelResourceTable[i][1] == -1) {
                object->mtnId[i] = CharMotionCreate(resourceData->objectState, playerModelResourceTable[i][0]);
            }
            else {
                object->mtnId[i] = Hu3DJointMotionFile(object->mdlId[0], playerResultResourceIds[playerModelResourceTable[i][1]] + resourceData->objectState);
            }
        }
        resourceData->motionSlot = 0;
        resourceData->animationTimer = 0;
        CharMotionSet(resourceData->objectState, object->mtnId[resourceData->animationState]);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
        resourceData->effectTimer = playerCameraOffsets[resourceData->playerIndex + 1].x;
        resourceData->effectProgress = playerCameraOffsets[resourceData->playerIndex + 1].y;
        resourceData->effectScale = playerCameraOffsets[resourceData->playerIndex + 1].z;
        resourceData->effectAngle = object->trans.x = cameraTrackOffsets[resourceData->positionX + 1].x;
        resourceData->effectFlags = object->trans.y = cameraTrackOffsets[resourceData->positionX + 1].y;
        resourceData->effectSpeed = object->trans.z = cameraTrackOffsets[resourceData->positionX + 1].z;
        resourceData->worldX = atan2d(playerCameraOffsets[0].x - resourceData->effectTimer, playerCameraOffsets[0].z - resourceData->effectScale);
        resourceData->worldY = 0.0f;
        resourceData->sequenceState = 0;
        resourceData->sequenceTimer = (REFRESH_RATE_F * 5) + (1.0f / 127) * frand7() * REFRESH_RATE_F * 15.0f;
        if (sceneFlags != 0) {
            resourceData->position = 3;
        }
        object->objFunc = UpdatePlayerInput;
    }
    CharMotionDataClose(resourceData->characterId);
}

Vec effectOffset = { 0.0f, 20.0f, 50.0f };
GXColor effectStartColor = { 0xFF, 0xFF, 0xFF, 0x10 };
GXColor effectEndColor = { 0xFF, 0xFF, 0xFF, 0x40 };

void UpdatePlayerInput(OMOBJ *object)
{
    Mtx rotationMatrix;
    Vec scratchData;
    Vec scratchData2;
    Vec effectVector;
    float floatValue;
    float floatValue2;
    float floatValue3;
    float floatValue4;
    PlayerModelData *resourceData;
    HU3DMODEL *model;
    s32 resourceId;
    s16 stateValue;
    s16 stateValue2;
    s16 stateValue3;
    s16 stateValue4;

    resourceData = object->data;
    model = &Hu3DData[object->mdlId[0]];
    resourceData->buttonDown = HuPadBtn[resourceData->objectPosition];
    resourceData->buttonPressed = HuPadBtnDown[resourceData->objectPosition];
    resourceData->rotationIndex = HuPadStkX[resourceData->objectPosition];
    resourceData->stickX = HuPadStkY[resourceData->objectPosition];
    if (GetSceneState() == 6) {
        if (resourceData->isComputer != 0) {
            resourceData->buttonDown &= ~PAD_BUTTON_A;
            resourceData->buttonPressed &= ~PAD_BUTTON_A;
            resourceData->rotationIndex = 0;
            resourceData->stickX = 0;
            UpdatePlayerModel(object);
        }
        stateValue = resourceData->buttonDown;
        stateValue2 = resourceData->buttonPressed;
        stateValue3 = resourceData->rotationIndex;
        stateValue4 = resourceData->stickX;
    }
#ifdef NON_MATCHING
    else {
        stateValue = 0;
        stateValue2 = 0;
        stateValue3 = 0;
        stateValue4 = 0;
    }
#endif
    if (fabs(stateValue3) < 8.0) {
        stateValue3 = 0;
    }
    if (fabs(stateValue3) > 80.0) {
        stateValue3 = 80;
    }
    if (fabs(stateValue4) < 8.0) {
        stateValue4 = 0;
    }
    if (fabs(stateValue4) > 80.0) {
        stateValue4 = 80;
    }
    if (GetSceneState() != 6) {
        stateValue3 = stateValue4 = 0;
        stateValue = stateValue2 = 0;
    }
    switch (resourceData->animationTimer) {
        case 0:
            if (GetSceneState() < 3) {
                break;
            }
            effectVector.x = resourceData->effectTimer - object->trans.x;
            effectVector.z = resourceData->effectScale - object->trans.z;
            floatValue2 = VECMagXZ(&effectVector);
            if (floatValue2 > (VERSION_PAL ? 30.000002f : 25.0f)) {
                effectVector.x = resourceData->effectTimer - object->trans.x;
                effectVector.z = resourceData->effectScale - object->trans.z;
                floatValue3 = atan2d(effectVector.x, effectVector.z);
                object->rot.y = ApproachAngle(floatValue3, object->rot.y, 15.0f);
#if VERSION_PAL
                object->trans.x += 25.0 * sind(object->rot.y) * 0.5 * 1.2000000476837158;
                object->trans.z += 25.0 * cosd(object->rot.y) * 0.5 * 1.2000000476837158;
#else
                object->trans.x += 25.0 * sind(object->rot.y) * 0.5;
                object->trans.z += 25.0 * cosd(object->rot.y) * 0.5;
#endif
                if (resourceData->motionSlot != 2) {
                    resourceData->motionSlot = 2;
                    CharMotionSet(resourceData->objectState, object->mtnId[resourceData->animationState]);
                    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
                }
                break;
            }
            object->trans.x = resourceData->effectTimer;
            object->trans.z = resourceData->effectScale;
            resourceData->animationTimer = 1;
            if (resourceData->motionSlot != 0) {
                resourceData->motionSlot = 0;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_LOOP);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f;
#endif
            }
            break;
        case 1:
            object->rot.y = ApproachAngle(0.0f, object->rot.y, VERSION_PAL ? 6.0f : 5.0f);
            if (fabs(GetShortestAngleDifference(object->rot.y, 0.0f)) < 1.0) {
                object->rot.y = 0.0f;
                resourceData->animationTimer = 2;
            }
            break;
        case 2:
            resourceData->animationTimer = 3;
            break;
        case 3:
            object->rot.y = ApproachAngle(0.0f, object->rot.y, VERSION_PAL ? 12.0f : 10.0f);
            if (GetSceneState() >= 7) {
                resourceData->animationTimer = 10;
                resourceData->rotationX = 0;
                break;
            }
            if (fabs(GetShortestAngleDifference(object->rot.y, 0.0f)) < 1.0) {
                object->rot.y = 0.0f;
                scratchData2 = effectOffset;
                mtxRot(rotationMatrix, object->rot.x, object->rot.y, object->rot.z);
                TransformVectorByMatrix(rotationMatrix, &scratchData2);
                scratchData2.x += object->trans.x;
                scratchData2.y += object->trans.y;
                scratchData2.z += object->trans.z;
                if (GetSceneState() <= 6 && (resourceData->uiEffectIndex = CreateUiEffect(resourceData->characterId, playerSpawnPositions[resourceData->playerIndex], scratchData2)) != -1) {
                    resourceData->animationTimer = 4;
                    resourceData->rotationX = 0;
                }
            }
            break;
        case 4:
            resourceId = GetUiEffectState(resourceData->uiEffectIndex);
            if (GetSceneState() >= 7) {
                resourceData->animationTimer = 10;
                resourceData->rotationX = 0;
                break;
            }
            if (resourceId == -1 || resourceData->uiEffectIndex == -1) {
                resourceData->animationTimer = 3;
                break;
            }
            if (resourceId == 2) {
                if (++resourceData->rotationX >= REFRESH_RATE_F / 10.0f) {
                    HuAudFXPlay(0x61A);
                    resourceData->animationTimer = 5;
                    resourceData->rotationX = 0;
                }
                break;
            }
            if (++resourceData->rotationX >= (0.9f * REFRESH_RATE_F) && resourceData->motionSlot != 3) {
                resourceData->motionSlot = 3;
                CharMotionSet(resourceData->objectState, object->mtnId[resourceData->animationState]);
#if VERSION_PAL
                CharMotionSpeedSet(resourceData->characterId, 1.2f);
#endif
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
            }
            break;
        case 5:
            floatValue3 = fabs(GetShortestAngleDifference(object->rot.y, resourceData->positionX));
            if (floatValue3 < 10.0f) {
                object->rot.y += floatValue3;
            }
            else {
                object->rot.y += VERSION_PAL ? 12.0f : 10.0f;
            }
            scratchData2 = effectOffset;
            mtxRot(rotationMatrix, object->rot.x, object->rot.y, object->rot.z);
            TransformVectorByMatrix(rotationMatrix, &scratchData2);
            scratchData2.x += object->trans.x;
            scratchData2.y += object->trans.y;
            scratchData2.z += object->trans.z;
            SetUiEffectPosition(resourceData->uiEffectIndex, scratchData2);
            effectVector.x = 0.0f;
            effectVector.y = 1.0f;
            effectVector.z = 1.0f;
            scratchData2.y = 0.0f;
            CheckStageCollisionRotated(scratchData2, effectVector, 50.0f, -0.5f, 10.0f, 1.0f, effectStartColor);
            if (GetSceneState() >= 7) {
                resourceData->animationTimer = 10;
                resourceData->rotationX = 0;
                break;
            }
            if (fabs(GetShortestAngleDifference(object->rot.y, resourceData->positionX)) < 1.0) {
                resourceData->startPosition = scratchData2;
                resourceData->animationTimer = 6;
                resourceData->rotationX = 0;
            }
            break;
        case 6:
            scratchData.x = 0.0125f * stateValue3 * 300.0f;
            scratchData.y = -900.0f;
            scratchData.z = -900.0f;
            floatValue3 = atan2d(scratchData.x - resourceData->startPosition.x, scratchData.z - resourceData->startPosition.z);
            object->rot.y = ApproachAngle(floatValue3, object->rot.y, 5.0f);
            if (GetSceneState() >= 7) {
                resourceData->animationTimer = 10;
                resourceData->rotationX = 0;
                break;
            }
            if (stateValue2 & PAD_BUTTON_A) {
                resourceData->motionSlot = 4;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_NONE);
                model->motShiftWork.stackValueEED = VERSION_PAL ? 0.6f : 0.5f;
                resourceData->targetPosition = scratchData;
                resourceData->animationTimer = 7;
                resourceData->rotationX = 0;
            }
            break;
        case 7:
            resourceData->rotationX++;
            if (resourceData->rotationX > REFRESH_RATE_F * 1.5f) {
                resourceData->rotationX = REFRESH_RATE * 1.5;
            }
            floatValue2 = (0.6666667f / REFRESH_RATE_F) * resourceData->rotationX;
            scratchData.x = 0.0125f * stateValue3 * 300.0f;
            scratchData.y = -900.0f;
            scratchData.z = -900.0f;
            floatValue3 = atan2d(scratchData.x - resourceData->startPosition.x, scratchData.z - resourceData->startPosition.z);
            object->rot.y = ApproachAngle(floatValue3, object->rot.y, 5.0f);
            resourceData->targetPosition = scratchData;
            if (GetSceneState() >= 7) {
                resourceData->animationTimer = 10;
                resourceData->rotationX = 0;
                break;
            }
            if (!(stateValue & PAD_BUTTON_A)) {
                floatValue4 = 15.000001f + 30.0f * floatValue2;
#if VERSION_PAL
                resourceData->objectTimer = floatValue4 * 1.2f;
#else
                resourceData->objectTimer = floatValue4;
#endif
                resourceData->motionSlot = 5;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_NONE);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f * (0.5f + 1.5f * floatValue2);
#else
                model->motShiftWork.stackValueEED = 0.5f + 1.5f * floatValue2;
#endif
                resourceData->animationTimer = 8;
                resourceData->rotationX = 0;
            }
            break;
        case 8:
            if (GetSceneState() >= 7) {
                resourceData->animationTimer = 10;
                resourceData->rotationX = 0;
                break;
            }
            if (CharMotionTimeGet(resourceData->characterId) >= 10.0f && CharMotionShiftIDGet(resourceData->characterId) == -1) {
                resourceId = GetUiEffectState(resourceData->uiEffectIndex);
                if (resourceId == -1 || resourceData->uiEffectIndex == -1) {
                    resourceData->animationTimer = 3;
                }
                VECSubtract(&resourceData->targetPosition, &resourceData->startPosition, &effectVector);
                VECNormalize(&effectVector, &effectVector);
                floatValue = 15.0f;
                effectVector.x *= resourceData->objectTimer * cosd(floatValue);
                effectVector.z *= resourceData->objectTimer * cosd(floatValue);
                effectVector.y = resourceData->objectTimer * sind(floatValue);
                SetUiEffectState(resourceData->uiEffectIndex, effectVector);
                resourceData->uiEffectIndex = -1;
                HuAudFXPlay(0x61F);
                scratchData2 = resourceData->startPosition;
                scratchData2.y += 75.0f;
                effectVector.x = 0.0f;
                effectVector.y = 1.0f;
                effectVector.z = 1.0f;
                scratchData2.y = 0.0f;
                CheckStageCollision(scratchData2, effectVector, 50.0f, -0.5f, 1.0f, effectEndColor);
                floatValue2 = (resourceData->objectTimer - 15.000001f) / 30.0f;
                if (floatValue2 > 0.66f) {
                    HuAudFXPlay(0x61B);
                }
                else if (floatValue2 > 0.33f) {
                    HuAudFXPlay(0x61C);
                }
                else {
                    HuAudFXPlay(0x61D);
                }
                resourceData->animationTimer = 9;
                resourceData->rotationX = 0;
            }
            break;
        case 9:
            if (GetSceneState() >= 7) {
                resourceData->animationTimer = 10;
                resourceData->rotationX = 0;
                break;
            }
            if (++resourceData->rotationX >= REFRESH_RATE_F / 5) {
                resourceData->motionSlot = 0;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_LOOP);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f;
#endif
                resourceData->animationTimer = 3;
                resourceData->rotationX = 0;
            }
            break;
        case 10:
            object->rot.y = ApproachAngle(0.0f, object->rot.y, 10.0f);
            if (CharMotionShiftIDGet(resourceData->characterId) == -1 && resourceData->motionSlot != 0) {
                resourceData->motionSlot = 0;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_LOOP);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f;
#endif
            }
            break;
        case 11:
            if (CharMotionShiftIDGet(resourceData->characterId) == -1 && resourceData->motionSlot != 6) {
                resourceData->motionSlot = 6;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_NONE);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_SHIFT_LOOP);
            }
            break;
        case 12:
            if (CharMotionShiftIDGet(resourceData->characterId) == -1 && resourceData->motionSlot != 7) {
                resourceData->motionSlot = 7;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_NONE);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_SHIFT_LOOP);
            }
            break;
    }
}

GXColor effectFlashColor = { 0xFF, 0xFF, 0xFF, 0x14 };

void UpdatePlayerAnimation(OMOBJ *object)
{
    Vec positionVector;
    Vec positionVector2;
    Vec positionVector3;
    float animationProgress;
    float animationProgress2;
    float animationProgress3;
    PlayerModelData *resourceData;
    HU3DMODEL *model;
    s16 stateValue;
    s16 stateValue2;
    s16 resourceData2;
    s16 stateValue3;
    s16 i;

    resourceData = object->data;
    model = &Hu3DData[object->mdlId[0]];
    resourceData->buttonDown = HuPadBtn[resourceData->objectPosition];
    resourceData->buttonPressed = HuPadBtnDown[resourceData->objectPosition];
    resourceData->rotationIndex = HuPadStkX[resourceData->objectPosition];
    resourceData->stickX = HuPadStkY[resourceData->objectPosition];
    if (GetSceneState() == 6) {
        if (resourceData->isComputer != 0) {
            resourceData->buttonDown &= ~PAD_BUTTON_A;
            resourceData->buttonPressed &= ~PAD_BUTTON_A;
            resourceData->rotationIndex = 0;
            resourceData->stickX = 0;
            UpdatePlayerModelAnimation(object);
        }
        resourceData2 = resourceData->rotationIndex;
        stateValue3 = resourceData->stickX;
    }
#ifdef NON_MATCHING
    else {
        resourceData2 = 0;
        stateValue3 = 0;
    }
#endif
    if (fabs(resourceData2) < 8.0) {
        resourceData2 = 0;
    }
    if (fabs(resourceData2) > 80.0) {
        resourceData2 = 80;
    }
    if (fabs(stateValue3) < 8.0) {
        stateValue3 = 0;
    }
    if (fabs(stateValue3) > 80.0) {
        stateValue3 = 80;
    }
    switch (resourceData->animationTimer) {
        case 0:
            if (GetSceneState() >= 6) {
                resourceData->animationTimer = 1;
            }
            break;
        case 1:
            if (fabs(resourceData2) > 8.0) {
                animationProgress = 0.125f * resourceData2;
            }
            else {
                animationProgress = 0.0f;
            }
            resourceData->motionData[resourceData->motionDataIndex] = animationProgress;
            if (++resourceData->motionDataIndex >= 15) {
                resourceData->motionDataIndex = 0;
            }
            for (i = 0, animationProgress = 0.0f; i < 15; i++) {
                animationProgress += resourceData->motionData[i];
            }
            animationProgress *= 0.06666667f;
            if (animationProgress > 25.0f) {
                animationProgress = 25.0f;
            }
            else if (animationProgress < -25.0f) {
                animationProgress = -25.0f;
            }
            if (0.0f == animationProgress) {
                stateValue2 = 0;
            }
            else if (fabs(animationProgress) < 10.0) {
                stateValue2 = 1;
            }
            else {
                stateValue2 = 2;
            }
            if (resourceData->motionSlot != stateValue2 && CharMotionShiftIDGet(resourceData->characterId) == -1) {
                resourceData->motionSlot = stateValue2;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_LOOP);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f;
#endif
            }
            if (resourceData->motionSlot == 1 && CharMotionShiftIDGet(resourceData->characterId) == -1) {
                animationProgress2 = fabs(0.1f * animationProgress);
                if (animationProgress2 < 0.25f) {
                    animationProgress2 = 0.25f;
                }
#if VERSION_PAL
                CharMotionSpeedSet(resourceData->characterId, animationProgress2 * 1.2f);
#else
                CharMotionSpeedSet(resourceData->characterId, animationProgress2);
#endif
            }
            if (resourceData2 == 0.0f) {
                animationProgress3 = 0.0f;
            }
            else if (resourceData2 < 0.0f) {
                animationProgress3 = -90.0f;
            }
            else {
                animationProgress3 = 90.0f;
            }
            if (object->rot.y < animationProgress3 && (object->rot.y += 20.0f) > animationProgress3) {
                object->rot.y = animationProgress3;
            }
            if (object->rot.y > animationProgress3 && (object->rot.y -= 20.0f) < animationProgress3) {
                object->rot.y = animationProgress3;
            }
#if VERSION_PAL
            object->trans.x += animationProgress * 1.2f;
#else
            object->trans.x += animationProgress;
#endif
            object->trans.y = 0.0f;
            object->trans.z = resourceData->effectScale;
            if (object->trans.x < -225.0f) {
                object->trans.x = -225.0f;
            }
            if (object->trans.x > 225.0f) {
                object->trans.x = 225.0f;
            }
            if (GetSceneState() >= 7) {
                resourceData->animationTimer = 9;
                resourceData->rotationX = 0;
            }
            else if (resourceData->buttonPressed & PAD_BUTTON_A) {
                if (resourceData2 > 0) {
                    model->motIdShift = -1;
                    resourceData->motionSlot = 3;
                    CharMotionSet(resourceData->objectState, object->mtnId[resourceData->animationState]);
                    CharMotionSpeedSet(resourceData->characterId, VERSION_PAL ? 1.2f : 1.0f);
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_SHIFT_LOOP);
                    resourceData->worldY = 6.5f;
                    resourceData->rotationX = 0;
                    object->rot.y = 90.0f;
                    i = 99;
                    if ((stateValue = FindFreeEffectSlot(uiEffectData)) != 0) {
                        for (i = 0; i < stateValue; i++) {
                            if (uiEffectData[0]->motionData < uiEffectData[0]->targetPosition || uiEffectData[0]->motionData - uiEffectData[0]->targetPosition > REFRESH_RATE / 10.0f) {
                                continue;
                            }
                            if (uiEffectData[0]->motionData < REFRESH_RATE * 0.6f) {
                                animationProgress2 = uiEffectData[0]->rotationX.z - object->trans.z;
                            }
                            else {
                                animationProgress2 = uiEffectData[0]->animationData.z - object->trans.z;
                            }
                            if (animationProgress2 < -100.0f) {
                                continue;
                            }
                            animationProgress2 = fabs(animationProgress2);
                            if (animationProgress2 >= 300.0f) {
                                resourceData->motionTimer = 13.5f;
                            }
                            else if (animationProgress2 <= 100.0f) {
                                resourceData->motionTimer = 6.5f;
                            }
                            else {
                                resourceData->motionTimer = 10.0f;
                            }
                        }
                    }
                    if (i >= stateValue) {
                        resourceData->motionTimer = 10.0f;
                    }
                    resourceData->animationTimer = 2;
                }
                else if (resourceData2 < 0) {
                    model->motIdShift = -1;
                    resourceData->motionSlot = 5;
                    CharMotionSet(resourceData->objectState, object->mtnId[resourceData->animationState]);
                    CharMotionSpeedSet(resourceData->characterId, VERSION_PAL ? 1.2f : 1.0f);
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_SHIFT_LOOP);
                    resourceData->worldY = 6.5f;
                    resourceData->rotationX = 0;
                    object->rot.y = -90.0f;
                    i = 99;
                    if ((stateValue = FindFreeEffectSlot(uiEffectData)) != 0) {
                        for (i = 0; i < stateValue; i++) {
                            if (uiEffectData[0]->motionData < uiEffectData[0]->targetPosition || uiEffectData[0]->motionData - uiEffectData[0]->targetPosition > REFRESH_RATE / 10.0f) {
                                continue;
                            }
                            if (uiEffectData[0]->motionData < REFRESH_RATE * 0.6f) {
                                animationProgress2 = uiEffectData[0]->rotationX.z - object->trans.z;
                            }
                            else {
                                animationProgress2 = uiEffectData[0]->animationData.z - object->trans.z;
                            }
                            if (animationProgress2 > 100.0f) {
                                continue;
                            }
                            animationProgress2 = fabs(animationProgress2);
                            if (animationProgress2 >= 300.0f) {
                                resourceData->motionTimer = 13.5f;
                            }
                            else if (animationProgress2 <= 100.0f) {
                                resourceData->motionTimer = 6.5f;
                            }
                            else {
                                resourceData->motionTimer = 10.0f;
                            }
                        }
                    }
                    if (i >= stateValue) {
                        resourceData->motionTimer = 10.0f;
                    }
                    resourceData->animationTimer = 4;
                }
                else {
                    model->motIdShift = -1;
                    resourceData->motionSlot = 7;
                    CharMotionSet(resourceData->objectState, object->mtnId[resourceData->animationState]);
                    CharMotionSpeedSet(resourceData->characterId, VERSION_PAL ? 1.2f : 1.0f);
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_SHIFT_LOOP);
                    resourceData->worldY = 10.5f;
                    resourceData->rotationX = 0;
                    object->rot.y = 0.0f;
                    resourceData->animationTimer = 7;
                }
            }
            break;
        case 2:
#if VERSION_PAL
            object->trans.x += resourceData->motionTimer * 1.2f;
#else
            object->trans.x += resourceData->motionTimer;
#endif
            if (object->trans.x > 225.0f) {
                object->trans.x = 225.0f;
            }
            object->trans.y += resourceData->positionY;
            resourceData->worldY -= (VERSION_PAL) ? 29.4f / REFRESH_RATE_OFF_BY_1 : 29.4f * REFRESH_FREQ;
            resourceData->rotationX++;
            if (resourceData->rotationX == resourceData->worldZ - REFRESH_RATE_F / 5) {
                resourceData->motionSlot = 4;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_NONE);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f;
#endif
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_SHIFT_LOOP);
            }
            if (object->trans.y <= 0.0f) {
                for (i = 0; i < 6; i++) {
                    positionVector3.x = sind(-90.0f + 30.0f * i);
                    positionVector3.z = cosd(-90.0f + 30.0f * i);
                    positionVector3.y = 0.0f;
                    positionVector2.x = 100.0f * positionVector3.x * 0.01f;
                    positionVector2.z = 100.0f * positionVector3.z * 0.01f;
                    positionVector2.y = 1.0f;
                    positionVector.x = object->trans.x + 100.0f * positionVector3.x * 0.2;
                    positionVector.z = object->trans.z + 100.0f * positionVector3.z * 0.2;
                    positionVector.y = 0.0f;
                    CheckStageCollisionRotated(positionVector, positionVector2, 50.0f, -0.25f, 20.0f, 0.5f, effectFlashColor);
                }
                object->trans.y = 0.0f;
                resourceData->animationTimer = 3;
            }
            break;
        case 3:
            if (CharMotionEndCheck(resourceData->characterId) && CharMotionShiftIDGet(resourceData->characterId) == -1) {
                memset(&resourceData->motionData, 0, sizeof(resourceData->motionData));
                resourceData->motionDataIndex = 0;
                resourceData->motionSlot = 0;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_LOOP);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f;
#endif
                resourceData->animationTimer = 6;
                resourceData->effectState = 0;
            }
            break;
        case 4:
#if VERSION_PAL
            object->trans.x -= resourceData->motionTimer * 1.2f;
#else
            object->trans.x -= resourceData->motionTimer;
#endif
            if (object->trans.x < -225.0f) {
                object->trans.x = -225.0f;
            }
            object->trans.y += resourceData->positionY;
            resourceData->worldY -= (VERSION_PAL) ? 29.4f / REFRESH_RATE_OFF_BY_1 : 29.4f * REFRESH_FREQ;
            resourceData->rotationX++;
            if (resourceData->rotationX == resourceData->worldZ - REFRESH_RATE_F / 5) {
                resourceData->motionSlot = 6;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_NONE);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f;
#endif
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_SHIFT_LOOP);
            }
            if (object->trans.y <= 0.0f) {
                for (i = 0; i < 6; i++) {
                    positionVector3.x = sind(-90.0f + 30.0f * i);
                    positionVector3.z = cosd(-90.0f + 30.0f * i);
                    positionVector3.y = 0.0f;
                    positionVector2.x = 100.0f * positionVector3.x * 0.01f;
                    positionVector2.z = 100.0f * positionVector3.z * 0.01f;
                    positionVector2.y = 1.0f;
                    positionVector.x = object->trans.x + 100.0f * positionVector3.x * 0.2;
                    positionVector.z = object->trans.z + 100.0f * positionVector3.z * 0.2;
                    positionVector.y = 0.0f;
                    CheckStageCollisionRotated(positionVector, positionVector2, 50.0f, -0.25f, 20.0f, 0.5f, effectFlashColor);
                }
                object->trans.y = 0.0f;
                resourceData->animationTimer = 5;
            }
            break;
        case 5:
            if (CharMotionEndCheck(resourceData->characterId) && CharMotionShiftIDGet(resourceData->characterId) == -1) {
                memset(&resourceData->motionData, 0, sizeof(resourceData->motionData));
                resourceData->motionDataIndex = 0;
                resourceData->motionSlot = 0;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_LOOP);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f;
#endif
                resourceData->animationTimer = 6;
                resourceData->effectState = 0;
            }
            break;
        case 6:
            if (++resourceData->effectState > 0.0f) {
                resourceData->effectState = 0;
                resourceData->animationTimer = 1;
            }
            break;
        case 7:
            object->trans.y += resourceData->positionY;
            resourceData->worldY -= (VERSION_PAL) ? 39.2f / REFRESH_RATE_F : 0.65333337f;
            resourceData->rotationX++;
            if (object->trans.y <= 0.0f) {
                resourceData->motionSlot = 0;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_NONE);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f;
#endif
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_SHIFT_LOOP);
                object->trans.y = 0.0f;
                resourceData->rotationX = 0;
                resourceData->animationTimer = 8;
            }
            break;
        case 8:
            if (++resourceData->rotationX > REFRESH_RATE_F / 5) {
                memset(&resourceData->motionData, 0, sizeof(resourceData->motionData));
                resourceData->motionDataIndex = 0;
                resourceData->motionSlot = 0;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_LOOP);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f;
#endif
                resourceData->animationTimer = 1;
            }
            break;
        case 9:
            object->rot.y = ApproachAngle(0.0f, object->rot.y, 10.0f);
            if (CharMotionShiftIDGet(resourceData->characterId) == -1 && resourceData->motionSlot != 1) {
                resourceData->motionSlot = 1;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_LOOP);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f;
#endif
            }
            if (CharMotionShiftIDGet(resourceData->characterId) == -1 && resourceData->motionSlot == 1) {
                object->trans.z += (VERSION_PAL) ? 6.0f : 5.0f;
            }
            if (object->trans.z > playerCameraOffsets->z + 100.0f) {
                resourceData->animationTimer = 10;
            }
            break;
        case 10:
            object->rot.y = ApproachAngle(0.0f, object->rot.y, 10.0f);
            if (CharMotionShiftIDGet(resourceData->characterId) == -1 && resourceData->motionSlot != 0) {
                resourceData->motionSlot = 0;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_LOOP);
#if VERSION_PAL
                model->motShiftWork.stackValueEED = 1.2f;
#endif
            }
            break;
        case 11:
            if (CharMotionShiftIDGet(resourceData->characterId) == -1 && resourceData->motionSlot != 9) {
                resourceData->motionSlot = 9;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_NONE);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_SHIFT_LOOP);
            }
            break;
        case 12:
            if (CharMotionShiftIDGet(resourceData->characterId) == -1 && resourceData->motionSlot != 10) {
                resourceData->motionSlot = 10;
                CharMotionShiftSet(resourceData->objectState, object->mtnId[resourceData->animationState], 0.0f, REFRESH_RATE_F / 7.5f, HU3D_MOTATTR_NONE);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_SHIFT_LOOP);
            }
            break;
    }
    effectControllerObject = object;
    resourceData->animationProgress = object->trans.x - resourceData->animationTime;
    resourceData->frameTimer = object->trans.y - resourceData->animationSpeed;
    resourceData->frameProgress = object->trans.z - resourceData->animationFrame;
    resourceData->animationTime = object->trans.x;
    resourceData->animationSpeed = object->trans.y;
    resourceData->animationFrame = object->trans.z;
}

void StopEffectObject(OMOBJ *object)
{
    object->objFunc = NULL;
}

void StopEffectObjects(void)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        StopEffectObject(playerEffectObjects[i]);
    }
}

s32 ResetEffectObjects(void)
{
    OMOBJ *object;
    PlayerModelData *resourceData;
    s32 i;

    for (i = 0; i < 4; i++) {
        object = playerEffectObjects[i];
        resourceData = object->data;
        if (resourceData->animationIndex == 0) {
            if (resourceData->animationTimer != 0 || fabs(GetShortestAngleDifference(object->rot.y, 0.0f)) > 1.0) {
                break;
            }
        }
        else {
            if (resourceData->animationTimer < 5) {
                break;
            }
        }
    }
    if (i >= 4) {
        return 1;
    }
    return 0;
}

s32 UpdateEffectObjectStates(void)
{
    OMOBJ *object;
    PlayerModelData *resourceData;
    s32 i;

    for (i = 0; i < 4; i++) {
        object = playerEffectObjects[i];
        resourceData = object->data;
        if (resourceData->animationIndex == 0) {
            if (resourceData->animationTimer != 10 || fabs(GetShortestAngleDifference(object->rot.y, 0.0f)) > 1.0 || CharMotionShiftIDGet(resourceData->objectState) != -1) {
                break;
            }
        }
        else {
            if (resourceData->animationTimer != 10 || fabs(GetShortestAngleDifference(object->rot.y, 0.0f)) > 1.0 || CharMotionShiftIDGet(resourceData->objectState) != -1) {
                break;
            }
        }
    }
    if (i >= 4) {
        return 1;
    }
    return 0;
}

void SetEffectObjectState(s32 index)
{
    OMOBJ *object;
    PlayerModelData *resourceData;
    s32 i;

    for (i = 0; i < 4; i++) {
        object = playerEffectObjects[i];
        resourceData = object->data;
        if (resourceData->animationIndex == 0) {
            if (resourceData->animationIndex == index) {
                resourceData->animationTimer = 11;
            }
            else {
                resourceData->animationTimer = 12;
            }
        }
        else {
            if (resourceData->animationIndex == index) {
                resourceData->animationTimer = 11;
            }
            else {
                resourceData->animationTimer = 12;
            }
        }
    }
}

s32 CheckRayIntersection(Vec *vertices, Vec *startPosition, Vec *endPosition, float angle)
{
    Vec prismCorners[4];
    Vec scratchData[4];
    Vec intersectionVector[4];
    Vec scaledDirection;
    Vec intersectionVector2;
    Vec intersectionVector3;
    Vec intersectionVector4;
    Vec planarStartPosition;
    float distanceValue;
    float distanceValue2;
    float distanceValue3;
    float distanceValue4;
    float distanceValue5;
    OMOBJ *object;
    PlayerModelData *resourceData;
    HU3DMODEL *scratchData2;

    if (effectControllerObject == NULL) {
        return 0;
    }
    object = effectControllerObject;
    resourceData = object->data;
    scratchData2 = &Hu3DData[object->mdlId[0]];
    *endPosition = *startPosition;
    switch (resourceData->animationTimer) {
        case 2:
        case 4:
            distanceValue = 150.0f;
            distanceValue2 = 130.0f;
            distanceValue3 = 60.000004f;
            distanceValue4 = 130.0f;
            break;
        default:
            distanceValue = 100.0f;
            distanceValue2 = 130.0f;
            distanceValue3 = 60.000004f;
            distanceValue4 = 0.0f;
            break;
    }
    scratchData[0].x = object->trans.x - 0.5f * distanceValue;
    scratchData[0].y = object->trans.y + distanceValue2;
    scratchData[0].z = object->trans.z + 0.5f * distanceValue3;
    scratchData[1].x = object->trans.x + 0.5f * distanceValue;
    scratchData[1].y = object->trans.y + distanceValue2;
    scratchData[1].z = object->trans.z + 0.5f * distanceValue3;
    scratchData[2].x = object->trans.x + 0.5f * distanceValue;
    scratchData[2].y = object->trans.y - distanceValue4;
    scratchData[2].z = object->trans.z + 0.5f * distanceValue3;
    scratchData[3].x = object->trans.x - 0.5f * distanceValue;
    scratchData[3].y = object->trans.y - distanceValue4;
    scratchData[3].z = object->trans.z + 0.5f * distanceValue3;
    prismCorners[0].x = object->trans.x + 0.5f * distanceValue;
    prismCorners[0].y = object->trans.y + distanceValue2;
    prismCorners[0].z = object->trans.z - 0.5f * distanceValue3;
    prismCorners[1].x = object->trans.x - 0.5f * distanceValue;
    prismCorners[1].y = object->trans.y + distanceValue2;
    prismCorners[1].z = object->trans.z - 0.5f * distanceValue3;
    prismCorners[2].x = object->trans.x - 0.5f * distanceValue;
    prismCorners[2].y = object->trans.y - distanceValue4;
    prismCorners[2].z = object->trans.z - 0.5f * distanceValue3;
    prismCorners[3].x = object->trans.x + 0.5f * distanceValue;
    prismCorners[3].y = object->trans.y - distanceValue4;
    prismCorners[3].z = object->trans.z - 0.5f * distanceValue3;
    VECNormalize(startPosition, &planarStartPosition);
    VECScale(&planarStartPosition, &planarStartPosition, angle);
    VECAdd(vertices, startPosition, &intersectionVector2);
    VECAdd(vertices, startPosition, &intersectionVector3);
    VECAdd(vertices, &planarStartPosition, &scaledDirection);
    VECAdd(&intersectionVector2, &planarStartPosition, &intersectionVector2);
    intersectionVector[0] = scratchData[0];
    intersectionVector[1] = scratchData[1];
    intersectionVector[2] = scratchData[2];
    intersectionVector[3] = scratchData[3];
    if (CheckSegmentBoxIntersection(intersectionVector, vertices, startPosition, &intersectionVector4, angle, 4) != 0) {
        endPosition->x *= 0.5f;
        endPosition->z *= -0.5f;
        SetSceneEffect(15, 1, 0);
        return 1;
    }
    if (intersectionVector3.x >= scratchData[0].x - angle && intersectionVector3.x <= scratchData[2].x + angle && intersectionVector3.y <= scratchData[0].y + angle && intersectionVector3.y >= scratchData[2].y - angle
        && intersectionVector3.z <= scratchData[0].z + angle && intersectionVector3.z >= prismCorners[3].z - angle) {
        if (resourceData->animationProgress > 0.0f && intersectionVector3.x > object->trans.x) {
            planarStartPosition = *startPosition;
            planarStartPosition.y = 0.0f;
            distanceValue5 = VECMag(&planarStartPosition);
            endPosition->x = 0.5f * distanceValue5 + resourceData->animationProgress;
            endPosition->z = 0.0f;
        }
        else if (resourceData->animationProgress < 0.0f && intersectionVector3.x < object->trans.x) {
            planarStartPosition = *startPosition;
            planarStartPosition.y = 0.0f;
            distanceValue5 = VECMag(&planarStartPosition);
            endPosition->x = -(0.5f * distanceValue5) + resourceData->animationProgress;
            endPosition->z = 0.0f;
        }
        else {
            if (fabs(startPosition->x) > fabs(startPosition->z)) {
                endPosition->x *= -1.0f;
            }
            else {
                endPosition->z *= -1.0f;
            }
            endPosition->x *= 0.5f;
            endPosition->z *= 0.5f;
        }
        SetSceneEffect(15, 1, 0);
        return 1;
    }
    return 0;
}

void SetSceneEffect(s32 index, s32 state, s32 mode)
{
    OMOBJ *object;
    PlayerModelData *resourceData;

    if (effectControllerObject != NULL) {
        object = effectControllerObject;
        resourceData = object->data;
        omVibrate(resourceData->objectIndex, 12, 6, 6);
        HuAudFXPlay(0x61A);
    }
}

s32 playerAnimationIds[] = { 3, 4, 5, 6 };
float playerAnimationScales[] = { 0.6f, 0.65f, 0.7f, 0.7f };

s32 playerAnimationParameters[][2] = {
    { REFRESH_RATE / 2, REFRESH_RATE },
    { REFRESH_RATE / 2.5, REFRESH_RATE / 1.25 },
    { REFRESH_RATE / 5, REFRESH_RATE / 2.5 },
    { 0, REFRESH_RATE / 5 }
};

s32 playerAnimationTransitions[][2] = {
    { REFRESH_RATE * 0.6, REFRESH_RATE * 1.4 },
    { REFRESH_RATE * 0.6, REFRESH_RATE * 1.3 },
    { REFRESH_RATE * 0.55, REFRESH_RATE * 1.2 },
    { REFRESH_RATE * 0.55, REFRESH_RATE * 1.1 }
};

void UpdatePlayerModel(OMOBJ *object)
{
    float floatValue;
    float floatValue2;
    PlayerModelData *resourceData;
    HU3DMODEL *scratchData;
    s32 stateValue;

    resourceData = object->data;
    scratchData = &Hu3DData[object->mdlId[0]];
    stateValue = resourceData->animationTimer;
    if (resourceData->sequenceIndex == 0) {
        resourceData->sequenceIndex = 1;
        resourceData->rotationZ = 0;
        stateValue = 5;
    }
    switch (stateValue) {
        case 5:
            if (resourceData->rotationZ != 0) {
                break;
            }
            if (resourceData->sequenceTimer > minigameCountdown && playerAnimationIds[resourceData->position] > resourceData->sequenceState && effectControllerObject != NULL) {
                resourceData->sequenceFlags = 1;
                resourceData->sequenceProgress = (1.0f / 127) * frand7();
                if (effectControllerObject->trans.x >= 270.0f) {
                    resourceData->motionSpeed = -(0.4f + 0.3f * resourceData->sequenceProgress);
                }
                else if (effectControllerObject->trans.x <= -270.0f) {
                    resourceData->motionSpeed = 0.4f + 0.3f * resourceData->sequenceProgress;
                }
                else if (frand() & 1) {
                    resourceData->motionSpeed = -(0.3f + 0.1f * resourceData->sequenceProgress);
                }
                else {
                    resourceData->motionSpeed = 0.3f + 0.1f * resourceData->sequenceProgress;
                }
                floatValue = playerAnimationTransitions[3][0];
                floatValue2 = playerAnimationTransitions[3][1];
                resourceData->rotationZ = floatValue + (1.0f / 127) * frand7() * (floatValue2 - floatValue);
                floatValue = playerAnimationParameters[resourceData->position][0];
                floatValue2 = playerAnimationParameters[resourceData->position][1];
                resourceData->positionData = floatValue + (1.0f / 127) * frand7() * (floatValue2 - floatValue);
                resourceData->sequenceState++;
            }
            else {
                if (frand() & 1) {
                    resourceData->motionSpeed = (1.0f / 127) * frand7() * playerAnimationScales[resourceData->position];
                }
                else {
                    resourceData->motionSpeed = -((1.0f / 127) * frand7()) * playerAnimationScales[resourceData->position];
                }
                floatValue = playerAnimationTransitions[resourceData->position][0];
                floatValue2 = playerAnimationTransitions[resourceData->position][1];
                resourceData->rotationZ = floatValue + (1.0f / 127) * frand7() * (floatValue2 - floatValue);
                floatValue = playerAnimationParameters[resourceData->position][0];
                floatValue2 = playerAnimationParameters[resourceData->position][1];
                resourceData->positionData = floatValue + (1.0f / 127) * frand7() * (floatValue2 - floatValue);
            }
            resourceData->rotationY = 0;
            break;
        case 6:
            if (++resourceData->rotationY >= resourceData->positionData) {
                resourceData->rotationIndex = 80.0f * resourceData->motionSpeed;
                resourceData->buttonPressed |= PAD_BUTTON_A;
                resourceData->buttonDown |= PAD_BUTTON_A;
                resourceData->rotationY = 0;
                return;
            }
            floatValue = sind(180.0f * ((resourceData->positionData - resourceData->rotationY) / REFRESH_RATE_F));
            if (resourceData->positionData & 1) {
                floatValue *= -1.0f;
            }
            resourceData->rotationIndex = 80.0f * (resourceData->motionSpeed + floatValue);
            return;
        case 7:
            if (resourceData->sequenceFlags != 0) {
                if (effectControllerObject->trans.x >= 148.5f) {
                    resourceData->motionSpeed = -(0.4f + 0.3f * resourceData->sequenceProgress);
                }
                else if (effectControllerObject->trans.x <= -148.5f) {
                    resourceData->motionSpeed = 0.4f + 0.3f * resourceData->sequenceProgress;
                }
                else if (effectControllerObject->trans.x > 0.0f) {
                    resourceData->motionSpeed = -(0.5f + 0.2f * resourceData->sequenceProgress);
                }
                else {
                    resourceData->motionSpeed = 0.5f + 0.2f * resourceData->sequenceProgress;
                }
            }
            resourceData->rotationIndex = 80.0f * resourceData->motionSpeed;
            if (++resourceData->rotationY > resourceData->rotationZ) {
                resourceData->motionSpeed = 0.0f;
                resourceData->rotationY = 0;
                resourceData->rotationZ = 0;
                resourceData->positionData = 0;
                resourceData->sequenceFlags = 0;
            }
            else {
                resourceData->buttonDown |= PAD_BUTTON_A;
            }
            break;
        default:
            resourceData->rotationY = 0;
            break;
    }
}

s32 playerMotionResourceIds[] = {
    REFRESH_RATE * 1,
    REFRESH_RATE * 0.75,
    REFRESH_RATE * 0.5,
    REFRESH_RATE * 0.25
};

float playerMotionScale[][2] = { { 200.0f, 150.0f }, { 125.0f, 100.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f } };

s32 playerMotionParameters[][2] = {
    { REFRESH_RATE * 0.8, REFRESH_RATE * 2.5 },
    { REFRESH_RATE * 0.65, REFRESH_RATE * 1.9 },
    { REFRESH_RATE * 0.55, REFRESH_RATE * 1.5 },
    { REFRESH_RATE * 0.35, REFRESH_RATE * 0.7 }
};

s32 playerMotionTransitions[][2] = {
    { REFRESH_RATE * 0.45, REFRESH_RATE * 0.65 },
    { REFRESH_RATE * 0.25, REFRESH_RATE * 0.45 },
    { REFRESH_RATE * 0.15, REFRESH_RATE * 0.25 },
    { 0, REFRESH_RATE * 0.05 }
};

void UpdatePlayerModelAnimation(OMOBJ *object)
{
    Vec effectVelocity;
    float animationProgress;
    float animationProgress2;
    float animationProgress3;
    float animationProgress4;
    float animationProgress5; // ! - uninitialized
    float animationProgress6;
    PlayerModelData *resourceData;
    HU3DMODEL *scratchData;
    UiEffectData *resourceData2;
    s32 stateValue;
    s32 stateValue2;
    s32 i, j;
    s32 stateValue3;

    stateValue3 = 0;
    resourceData = object->data;
    scratchData = &Hu3DData[object->mdlId[0]];
    if (resourceData->currentMotion != -1) {
        resourceData2 = GetUiEffectObject(resourceData->currentMotion);
        if (resourceData2 != NULL) {
            if (resourceData2->positionX != 3 || resourceData2->position != 0 || resourceData2->animationData.z <= -900.0f) {
                stateValue3 = 1;
            }
        }
        else {
            stateValue3 = 1;
        }
    }
    else {
        stateValue3 = 1;
    }
    if (stateValue3 != 0) {
        resourceData->currentMotion = -1;
        resourceData->rotationY = 0;
        animationProgress = playerMotionParameters[resourceData->position][0];
        animationProgress2 = playerMotionParameters[resourceData->position][1];
        resourceData->motionFlags = animationProgress + (1.0f / 127) * frand7() * (animationProgress2 - animationProgress);
        animationProgress = playerMotionTransitions[resourceData->position][0];
        animationProgress2 = playerMotionTransitions[resourceData->position][1];
        resourceData->animationFlags = animationProgress + (1.0f / 127) * frand7() * (animationProgress2 - animationProgress);
        resourceData->targetPosition.x = playerMotionScale[resourceData->position][0] * (0.5f - (1.0f / 127) * frand7());
        resourceData->targetPosition.y = playerMotionScale[resourceData->position][1] * (0.5f - (1.0f / 127) * frand7());
        if (playerSelectionTimer >= 5) {
            resourceData->targetPosition.x *= 1.5f;
        }
        stateValue2 = FindFreeEffectSlot(uiEffectWork);
        if (stateValue2 != 0) {
            for (i = 0; i < stateValue2; i++) {
                resourceData2 = uiEffectWork[i];
                if (resourceData2 == NULL || resourceData2->isComputer == 0 || 0.0f == resourceData2->scale.z || fabs(resourceData2->animationData.x) >= 325.0
                    || resourceData2->targetPosition < resourceData->animationFlags) {
                    continue;
                }
#ifdef NON_MATCHING
                animationProgress5 = 60.000004f + object->trans.z;
#endif
                stateValue = (animationProgress5 - resourceData2->animationData.z) / resourceData2->scale.z;
                if (stateValue > REFRESH_RATE_F * 5) {
                    continue;
                }
                effectVelocity = resourceData2->scale;
                animationProgress3 = resourceData2->animationData.x;
                animationProgress4 = resourceData2->animationData.y;
                for (j = 0; j < stateValue; j++) {
                    animationProgress3 += effectVelocity.x;
                    animationProgress4 += effectVelocity.y;
                    effectVelocity.x *= resourceData2->motionTimer;
                    effectVelocity.z *= resourceData2->motionTimer;
                    effectVelocity.y -= 19.6f / REFRESH_RATE_F;
                    if (animationProgress4 < 20.0) {
                        animationProgress4 = 19.6f / REFRESH_RATE_F;
                        effectVelocity.y *= resourceData2->motionFlags;
                    }
                }
                animationProgress3 += resourceData->targetPosition.x;
                animationProgress4 += resourceData->targetPosition.y;
                if (resourceData->position <= 2 || !(fabs(animationProgress3) >= 225.0 || animationProgress4 >= 200.0f)) {
                    resourceData->currentMotion = resourceData2->effectIndex;
                    break;
                }
            }
        }
    }
    if (resourceData->currentMotion != -1 && (resourceData2 = GetUiEffectObject(resourceData->currentMotion)) != NULL) {
        if (resourceData2->animationData.z > 60.000004f + object->trans.z) {
            animationProgress5 = 60.000004f + object->trans.z;
            stateValue = (animationProgress5 - resourceData2->animationData.z) / resourceData2->scale.z;
            if (stateValue > REFRESH_RATE_F * 5) {
                resourceData->currentMotion = -1;
                return;
            }
            effectVelocity = resourceData2->scale;
            animationProgress3 = resourceData2->animationData.x;
            animationProgress4 = resourceData2->animationData.y;
            for (j = 0; j < stateValue; j++) {
                animationProgress3 += effectVelocity.x;
                animationProgress4 += effectVelocity.y;
                effectVelocity.x *= resourceData2->motionTimer;
                effectVelocity.z *= resourceData2->motionTimer;
                effectVelocity.y -= 19.6f / REFRESH_RATE_F;
                if (animationProgress4 < 20.0) {
                    animationProgress4 = 19.6f / REFRESH_RATE_F;
                    effectVelocity.y *= resourceData2->motionFlags;
                }
            }
            stateValue += resourceData->motionFlags;
            animationProgress3 += resourceData->targetPosition.x;
            animationProgress4 += resourceData->targetPosition.y;
        }
        else {
            stateValue = 0;
            animationProgress3 = resourceData2->animationData.x;
            animationProgress4 = resourceData2->animationData.y;
        }
        if (object->trans.x - 25.0f > animationProgress3) {
            resourceData->rotationIndex = -64;
        }
        if (object->trans.x + 25.0f < animationProgress3) {
            resourceData->rotationIndex = 64;
        }
        animationProgress6 = fabs(animationProgress3 - object->trans.x);
        if (stateValue < 10 && animationProgress6 > 120.00001f) {
            resourceData->buttonPressed |= 0x100;
        }
        else if (stateValue < 5 && animationProgress6 < 25.0f && animationProgress4 > 100.0f) {
            resourceData->buttonPressed |= 0x100;
        }
        else if (stateValue > 20 && animationProgress6 < 50.0) {
            resourceData->buttonPressed /= 2;
        }
        if (++resourceData->rotationY > playerMotionResourceIds[resourceData->position]) {
            resourceData->currentMotion = -1;
        }
    }
}

void CreateUiObjects(void)
{
    uiEffectObjectManager = omAddObjEx(objectManager, 0x50, 0, 0, -1, UpdateUiObject);
}

s32 uiEffectResourceIds[]
    = { DATA_MAKE_NUM(DATADIR_M423, 4), DATA_MAKE_NUM(DATADIR_M423, 5), DATA_MAKE_NUM(DATADIR_M423, 6), DATA_MAKE_NUM(DATADIR_M423, 7),
          DATA_MAKE_NUM(DATADIR_M423, 8), DATA_MAKE_NUM(DATADIR_M423, 9), DATA_MAKE_NUM(DATADIR_M423, 10), DATA_MAKE_NUM(DATADIR_M423, 11) };

void UpdateUiObject(OMOBJ *object)
{
    UiEffectData *resourceData;
    UiEffectData *resourceData2;
    HU3DMODEL *model;
    s32 i;

    if (playerCharacterIds[0] == -1) {
        return;
    }
    resourceData2 = resourceData = object->data = HuMemDirectMallocNum(HEAP_HEAP, 15 * sizeof(*resourceData2), HU_MEMNUM_OVL);
    memset(resourceData2, 0, 15 * sizeof(*resourceData2));
    for (i = 0; i < 15; i++, resourceData2++) {
        resourceData2->objectPosition = i % 3;
        resourceData2->effectIndex = i;
        resourceData2->isComputer = 0;
        resourceData2->animationIndex = 0;
        resourceData2->position = 0;
        resourceData2->positionX = 0;
        resourceData2->positionY = 0;
        resourceData2->initialEffectState = 0;
        resourceData2->rotationIndex = 0;
        resourceData2->targetPosition = 0;
        resourceData2->animationTimer = 0;
        resourceData2->motionData = 0;
        resourceData2->animationData.x = resourceData2->animationData.y = resourceData2->animationData.z = 0.0f;
        resourceData2->rotationX = resourceData2->rotationY = resourceData2->rotationZ = 0.0f;
        resourceData2->scale.x = resourceData2->scale.y = resourceData2->scale.z = 0.0f;
        resourceData2->positionData = resourceData2->animationData;
        resourceData2->previousScale = resourceData2->scale;
        resourceData2->motionTimer = 0.995f;
        resourceData2->motionFlags = -0.5f;
        resourceData2->rotationX.x = resourceData2->rotationX.y = resourceData2->rotationX.z = 0.0f;
        if (i < 3) {
            resourceData2->objectIndex = Hu3DModelCreateFile(uiEffectResourceIds[playerCharacterIds[resourceData2->objectPosition]]);
        }
        else {
            resourceData2->objectIndex = Hu3DModelLink(resourceData[resourceData2->objectPosition].objectIndex);
        }
        Hu3DModelScaleSet(resourceData2->objectIndex, 0.8f, 0.8f, 0.8f);
        Hu3DModelAttrSet(resourceData2->objectIndex, HU3D_ATTR_DISPOFF);
        Hu3DModelCameraSet(resourceData2->objectIndex, 5);
        Hu3DModelShadowSet(resourceData2->objectIndex);
        model = &Hu3DData[resourceData2->objectIndex];
        MTXIdentity(model->mtx);
    }
    InitializeCollisionData();
    object->objFunc = UpdateUiObjectStates;
}

void UpdateUiObjectStates(OMOBJ *object)
{
    Mtx rotationMatrix;
    Vec collisionPosition;
    Vec rotationAxis;
    Vec previousPosition;
    Vec transformMatrix;
    Vec scratchData;
    float floatValue;
    float floatValue2;
    float floatValue3;
    float floatValue4;
    UiEffectData *resourceData;
    UiEffectData *position;
    HU3DMODEL *position2;
    s32 i, j;
    s32 position3;
    s32 model;
    s32 index;

    resourceData = object->data;
    for (i = 0; i < 15; i++, resourceData++) {
        if (resourceData->objectFlags == 0) {
            continue;
        }
        position2 = &Hu3DData[resourceData->objectIndex];
        position3 = 0;
        switch (resourceData->positionX) {
            case 0:
                break;
            case 1:
                floatValue = resourceData->targetPosition / REFRESH_RATE_F;
                resourceData->targetPosition++;
                if (floatValue > 1.0f) {
                    floatValue3 = 1.0f;
                }
                else {
                    floatValue3 = floatValue;
                }
                resourceData->animationData.x = resourceData->objectFlags.x + floatValue3 * (resourceData->actionState.x - resourceData->objectFlags.x);
                resourceData->animationData.z = resourceData->objectFlags.z + floatValue3 * (resourceData->actionState.z - resourceData->objectFlags.z);
                if (floatValue < 0.6f) {
                    floatValue3 = floatValue / 0.6f;
                    floatValue2 = 100.0f;
                }
                else if (floatValue < 1.0f) {
                    floatValue3 = (floatValue - 0.6f) / 0.4f;
                    floatValue2 = 50.0f;
                }
                else {
                    floatValue3 = (floatValue - 1.0f) / 0.25f;
                    floatValue2 = 10.0f;
                }
                resourceData->animationData.y = 20.0 + floatValue2 * sind(180.0f * floatValue3);
                if (resourceData->targetPosition >= REFRESH_RATE_F * 1.25f) {
                    resourceData->positionX = 2;
                    resourceData->targetPosition = 0;
                    resourceData->animationData.y = 20.0f;
                    resourceData->modelId.y = 0.0f;
                }
#if VERSION_PAL
                if (resourceData->targetPosition < 1.22f * REFRESH_RATE_F) {
#else
                if (resourceData->targetPosition < 1.22f * REFRESH_RATE_OFF_BY_1) {
#endif
                    rotationAxis.x = resourceData->modelId.z;
                    rotationAxis.z = -resourceData->modelId.x;
                    rotationAxis.y = 0.0f;
                    floatValue3 = 0.05 * VECMag(&rotationAxis);
                    if (floatValue3 > 0.00001f) {
                        MTXRotAxisRad(rotationMatrix, &rotationAxis, floatValue3);
                        MTXConcat(rotationMatrix, position2->mtx, position2->mtx);
                    }
                }
                Hu3DModelPosSet(resourceData->objectIndex, resourceData->animationData.x, resourceData->animationData.y, resourceData->animationData.z);
                Hu3DModelRotSet(resourceData->objectIndex, resourceData->rotationXAngle, resourceData->rotationY, resourceData->rotationZ);
                break;
            case 2:
                Hu3DModelPosSet(resourceData->objectIndex, resourceData->animationData.x, resourceData->animationData.y, resourceData->animationData.z);
                Hu3DModelRotSet(resourceData->objectIndex, resourceData->rotationXAngle, resourceData->rotationY, resourceData->rotationZ);
                break;
            case 3:
                resourceData->positionData = resourceData->animationData;
                resourceData->positionZ = resourceData->modelId;
                model = 0;
                if (index = FindCollisionIntersection(&resourceData->animationData, &resourceData->modelId, &collisionPosition, &scratchData, 20.0f, 3)) {
                    if (index & 0x8000) {
                        position3 = 1;
                    }
                    if (index & 0x2000) { }
                    if ((index & 0x4000) && resourceData->rotationIndex == 0 && resourceData->position == 0) {
                        floatValue3 = VECMagXZ(&resourceData->modelId);
                        transformMatrix.x = 0.0f;
                        transformMatrix.y = 0.0f;
                        transformMatrix.z = (1.0f / 127) * frand7() * 360.0f;
                        CheckBackgroundCollision(collisionPosition, transformMatrix, 0.5f);
                        HuAudFXPlay(0x620);
                        resourceData->rotationIndex = 1;
                    }
                    resourceData->animationData = collisionPosition;
                    resourceData->modelId = scratchData;
                    resourceData->motionTimer = 0.975f;
                    resourceData->motionFlags = -0.5f;
                }
                else {
                    if (fabs(resourceData->animationData.x) > 1000.0) {
                        Hu3DModelAttrSet(resourceData->objectIndex, HU3D_ATTR_DISPOFF);
                        resourceData->positionX = 0;
                        resourceData->objectFlags = 0;
                        resourceData->targetPosition = 0;
                    }
                    else {
                        resourceData->animationData.x += resourceData->modelId.x;
                        resourceData->animationData.y += resourceData->modelId.y;
                        resourceData->animationData.z += resourceData->modelId.z;
                    }
                }
                if (resourceData->position == 0) {
                    for (j = 0, position = object->data; j < 15; j++, position++) {
                        if (i == j || position->objectFlags == 0 || position->position != 0 || position->positionX != 3) {
                            continue;
                        }
                        VECSubtract(&resourceData->animationData, &position->animationData, &rotationAxis);
                        floatValue3 = VECMag(&rotationAxis);
                        if (floatValue3 < 40.0) {
                            previousPosition = resourceData->animationData;
                            if (floatValue3 != 0.0f) {
                                floatValue = 40.0 - floatValue3;
                                VECNormalize(&rotationAxis, &rotationAxis);
                                VECScale(&rotationAxis, &rotationAxis, floatValue);
                                VECAdd(&resourceData->animationData, &rotationAxis, &resourceData->animationData);
                            }
                            else {
                                resourceData->animationData.x += 40.0;
                            }
                            floatValue = VECMag(&resourceData->modelId);
                            floatValue4 = VECMag(&position->modelId);
                            floatValue3 = 0.5f * (floatValue + floatValue4);
                            if (VECMag(&resourceData->modelId) == 0.0f) {
                                resourceData->modelId.x = 1.0f;
                            }
                            VECNormalize(&resourceData->modelId, &resourceData->modelId);
                            VECSubtract(&resourceData->animationData, &position->animationData, &rotationAxis);
                            if (VECMag(&rotationAxis) == 0.0f) {
                                rotationAxis.x = 1.0f;
                            }
                            VECNormalize(&rotationAxis, &rotationAxis);
                            VECAdd(&resourceData->modelId, &rotationAxis, &resourceData->modelId);
                            if (VECMag(&resourceData->modelId) == 0.0f) {
                                resourceData->modelId.x = 1.0f;
                            }
                            VECNormalize(&resourceData->modelId, &resourceData->modelId);
                            VECScale(&resourceData->modelId, &resourceData->modelId, floatValue3);
                            if (VECMag(&position->modelId) == 0.0f) {
                                position->modelId.x = -1.0f;
                            }
                            VECNormalize(&position->modelId, &position->modelId);
                            VECSubtract(&position->animationData, &resourceData->animationData, &rotationAxis);
                            if (VECMag(&rotationAxis) == 0.0f) {
                                rotationAxis.x = 1.0f;
                            }
                            VECNormalize(&rotationAxis, &rotationAxis);
                            VECAdd(&position->modelId, &rotationAxis, &position->modelId);
                            if (VECMag(&position->modelId) == 0.0f) {
                                position->modelId.x = -1.0f;
                            }
                            VECNormalize(&position->modelId, &position->modelId);
                            VECScale(&position->modelId, &position->modelId, floatValue3);
                            if (index = FindCollisionIntersection(&previousPosition, &rotationAxis, &collisionPosition, &scratchData, 20.0f, 1)) {
                                if (index & 0x8000) {
                                    position3 = 1;
                                }
                                resourceData->animationData = collisionPosition;
                                resourceData->modelId = scratchData;
                            }
                        }
                    }
                }
                if (resourceData->animationData.y < 20.0) {
                    resourceData->animationData.y = 20.0f;
                    resourceData->modelId.y *= resourceData->motionFlags;
                    model = 1;
                    if (resourceData->animationData.x >= -225.0f && resourceData->animationData.x <= 225.0f && resourceData->animationData.z >= -1100.0f && resourceData->animationData.z <= -900.0f) {
                        position3 = 1;
                    }
                    if (fabs(resourceData->modelId.y) < 0.05f) {
                        resourceData->modelId.y = 0.0f;
                    }
                }
                if (resourceData->position == 0) {
                    VECSubtract(&resourceData->animationData, &resourceData->positionData, &rotationAxis);
                    if (CheckRayIntersection(&resourceData->animationData, &resourceData->modelId, &scratchData, 20.0f) != 0) {
                        resourceData->modelId = scratchData;
                        resourceData->position = 1;
                        resourceData->motionTimer = 0.975f;
                        resourceData->motionFlags = -0.5f;
                    }
                }
                if (resourceData->modelId.z > 0.0f) {
                    resourceData->position = 1;
                }
                resourceData->modelId.y -= 19.6f / REFRESH_RATE_F;
                if (model == 0) {
                    resourceData->modelId.x *= resourceData->motionTimer;
                    resourceData->modelId.z *= resourceData->motionTimer;
                }
                else {
                    resourceData->modelId.x *= 0.995f * resourceData->motionTimer;
                    resourceData->modelId.z *= 0.995f * resourceData->motionTimer;
                }
                if (position3 != 0 && resourceData->animationIndex == 0 && resourceData->position == 0) {
                    SetUiEffectAnimation(i);
                }
                if (resourceData->position != 0) {
                    resourceData->animationTimer++;
                    if (resourceData->animationTimer & 1) {
                        Hu3DModelAttrReset(resourceData->objectIndex, HU3D_ATTR_DISPOFF);
                    }
                    else {
                        Hu3DModelAttrSet(resourceData->objectIndex, HU3D_ATTR_DISPOFF);
                    }
                    if (resourceData->animationTimer >= REFRESH_RATE_F * 2) {
                        Hu3DModelAttrSet(resourceData->objectIndex, HU3D_ATTR_DISPOFF);
                        resourceData->positionX = 0;
                        resourceData->objectFlags = 0;
                        resourceData->targetPosition = 0;
                        resourceData->animationTimer = 0;
                    }
                }
                floatValue = VECMagXZ(&resourceData->modelId);
                if (fabs(floatValue) < 0.1f && resourceData->animationData.y <= 20.0 && resourceData->position == 0) {
                    resourceData->positionX = 4;
                    resourceData->targetPosition = 0;
                    resourceData->animationTimer = 0;
                }
                if (fabs(resourceData->animationData.x) > 2000.0 || fabs(resourceData->animationData.y) > 2000.0 || fabs(resourceData->animationData.z) > 2000.0) {
                    resourceData->positionX = 4;
                    resourceData->targetPosition = 0;
                    resourceData->animationTimer = REFRESH_RATE * 2;
                }
                resourceData->targetPosition++;
                rotationAxis.x = resourceData->modelId.z;
                rotationAxis.z = -resourceData->modelId.x;
                rotationAxis.y = 0.0f;
                floatValue3 = 0.05 * VECMag(&rotationAxis);
                if (floatValue3 > 0.00001f) {
                    MTXRotAxisRad(rotationMatrix, &rotationAxis, floatValue3);
                    MTXConcat(rotationMatrix, position2->mtx, position2->mtx);
                }
                Hu3DModelPosSet(resourceData->objectIndex, resourceData->animationData.x, resourceData->animationData.y, resourceData->animationData.z);
                break;
            case 4:
                resourceData->animationTimer++;
                if (resourceData->animationTimer & 1) {
                    Hu3DModelAttrReset(resourceData->objectIndex, HU3D_ATTR_DISPOFF);
                }
                else {
                    Hu3DModelAttrSet(resourceData->objectIndex, HU3D_ATTR_DISPOFF);
                }
                if (resourceData->animationTimer >= REFRESH_RATE_F * 2) {
                    Hu3DModelAttrSet(resourceData->objectIndex, HU3D_ATTR_DISPOFF);
                    resourceData->positionX = 0;
                    resourceData->objectFlags = 0;
                    resourceData->targetPosition = 0;
                    resourceData->animationTimer = 0;
                }
                break;
        }
    }
}

void StopUiObjects(OMOBJ *object)
{
    object->objFunc = NULL;
}

void StopUiObjectManager(void)
{
    StopUiObjects(uiEffectObjectManager);
}

s32 CreateUiEffect(s32 playerIndex, Vec position, Vec targetPosition)
{
    UiEffectData *resourceData;
    HU3DMODEL *resourceData2;
    s32 position2;
    s32 i;

    if (uiEffectObjectManager == NULL) {
        return -1;
    }
    resourceData = uiEffectObjectManager->data;
    for (i = 0, position2 = -1; i < 3; i++) {
        if (playerIndex == playerCharacterIds[i]) {
            position2 = i;
        }
    }
    if (position2 == -1) {
        return -1;
    }
    for (i = 0; i < 15; i++, resourceData++) {
        if (resourceData->objectPosition == position2 && resourceData->isComputer == 0) {
            break;
        }
    }
    if (i >= 15) {
        return -1;
    }
    resourceData2 = &Hu3DData[resourceData->objectIndex];
    resourceData->isComputer = 1;
    resourceData->animationIndex = 0;
    resourceData->position = 0;
    resourceData->initialEffectState = 0;
    resourceData->rotationIndex = 0;
    resourceData->positionX = 1;
    resourceData->targetPosition = 0;
    resourceData->animationTimer = 0;
    resourceData->startPosition = position;
    resourceData->actionState = targetPosition;
    resourceData->animationData = position;
    resourceData->rotationX = resourceData->rotationY = resourceData->rotationZ = 0.0f;
    resourceData->scale.x = 0.016666668f * (targetPosition.x - position.x); //! not updated for PAL
    resourceData->scale.z = 0.016666668f * (targetPosition.z - position.z);
    resourceData->scale.y = 5.0f;
    Hu3DModelPosSet(resourceData->objectIndex, resourceData->animationData.x, resourceData->animationData.y, resourceData->animationData.z);
    Hu3DModelRotSet(resourceData->objectIndex, resourceData->rotationX, resourceData->rotationY, resourceData->rotationZ);
    Hu3DModelAttrReset(resourceData->objectIndex, HU3D_ATTR_DISPOFF);
    MTXIdentity(resourceData2->mtx);
    return i;
}

void SetUiEffectPosition(s32 effectIndex, Vec position)
{
    UiEffectData *resourceData;

    if (uiEffectObjectManager != NULL) {
        resourceData = uiEffectObjectManager->data;
        if (effectIndex < 0 || effectIndex >= 15) {
            return;
        }
        resourceData[effectIndex].animationData = position;
    }
}

void SetUiEffectState(s32 effectIndex, Vec state)
{
    Vec positionVector;
    Vec positionVector2;
    float floatValue3;
    UiEffectData *index;
    UiEffectData *resourceData;

    if (uiEffectObjectManager == NULL) {
        return;
    }
    index = uiEffectObjectManager->data;
    if (effectIndex < 0 || effectIndex >= 15) {
        return;
    }
    resourceData = &index[effectIndex];
    resourceData->positionX = 3;
    resourceData->targetPosition = 0;
    resourceData->animationTimer = 0;
    resourceData->scale = state;
    resourceData->positionData = resourceData->animationData;
    resourceData->motionTimer = 0.995f;
    resourceData->motionFlags = -0.5f;
    if (resourceData->scale.z < 0.0f) {
        positionVector = resourceData->animationData;
        floatValue3 = -800.0f - positionVector.z;
        resourceData->motionData = fabs(floatValue3 / resourceData->scale.z);
        VECScale(&state, &positionVector2, resourceData->motionData);
        VECAdd(&positionVector, &positionVector2, &resourceData->rotationX);
    }
    else {
        resourceData->motionData = 1000;
    }
}

s32 GetUiEffectState(s32 object)
{
    UiEffectData *resourceData;

    if (uiEffectObjectManager == NULL) {
        return -1;
    }
    resourceData = uiEffectObjectManager->data;
    if (object < 0 || object >= 15) {
        return -1;
    }
    if (resourceData[object].objectFlags == 0) {
        return -1;
    }
    return resourceData[object].positionX;
}

void SetUiEffectAnimation(s32 object)
{
    UiEffectData *resourceData2;
    UiEffectData *resourceData;

    if (uiEffectObjectManager == NULL) {
        return;
    }
    resourceData2 = uiEffectObjectManager->data;
    if (object < 0 || object >= 15) {
        return;
    }
    resourceData = &resourceData2[object];
    resourceData->animationIndex = 1;
    resourceData->position = 1;
    resourceData->targetPosition = 0;
    SetShadowModelPosition(resourceData->animationData, 0.75f, playerCharacterIds[resourceData->objectPosition]);
    HuAudFXPlay(9);
    DestroyPlayerObjects();
    playerScores[resourceData->objectPosition]++;
    playerSelectionTimer--;
}

UiEffectData *GetUiEffectObject(s32 object)
{
    UiEffectData *resourceData;
    UiEffectData *positionVector2;

    if (uiEffectObjectManager == NULL) {
        return NULL;
    }
    resourceData = uiEffectObjectManager->data;
    positionVector2 = resourceData;
    if (object < 0 || object >= 15) {
        return NULL;
    }
    return &resourceData[object];
}

GXColor uiEffectColor = { 0xFF, 0xFF, 0xFF, 0xFF };

void DestroyUiEffects(void)
{
    Vec zeroPosition;
    UiEffectData *positionVector3;
    UiEffectData *resourceData;
    s32 activeEffectCount;
    s32 i;

    if (uiEffectObjectManager == NULL) {
        return;
    }
    resourceData = positionVector3 = uiEffectObjectManager->data;
    for (i = 0, activeEffectCount = 0; i < 15; i++, resourceData++) {
        if (resourceData->isComputer == 0) {
            continue;
        }
        zeroPosition.x = zeroPosition.y = zeroPosition.z = 0.0f;
        CheckStageCollision(resourceData->animationData, zeroPosition, 100.0f, 0.0f, 1.0f, uiEffectColor);
        Hu3DModelAttrSet(resourceData->objectIndex, HU3D_ATTR_DISPOFF);
        resourceData->positionX = 0;
        resourceData->isComputer = 0;
        resourceData->targetPosition = 0;
        resourceData->animationTimer = 0;
    }
}

s32 FindFreeEffectSlot(UiEffectData **object)
{
    s32 resourceData3;
    s32 i;
    UiEffectData *position;
    UiEffectData *index;
    UiEffectData *position2;

    if (uiEffectObjectManager == NULL) {
        return;
    }
    position = uiEffectObjectManager->data;
    index = position;
    for (i = 0, resourceData3 = 0; i < 15; i++, position++) {
        if (position->isComputer == 0 || position->positionX != 3 || position->position != 0 || position->animationData.z <= -900.0f) {
            continue;
        }
        object[resourceData3] = position;
        resourceData3++;
    }
    index = position;
    for (i = 0; i < resourceData3 - 1; i++) {
        if (object[i]->animationData.z > object[i + 1]->animationData.z) {
            position2 = object[i];
            object[i] = object[i + 1];
            object[i + 1] = position2;
        }
    }
    return resourceData3;
}

void InitializeCollisionData(void)
{
    Vec prismVertices[8];
    Vec positionVector4;
    Vec positionVector;
    Vec positionVector2;
    CollisionPrismData *resourceData;
    s32 i;

    resourceData = collisionPrisms = HuMemDirectMallocNum(HEAP_HEAP, 16 * sizeof(*collisionPrisms), HU_MEMNUM_OVL);
    memset(resourceData, 0, 16 * sizeof(*resourceData));
    for (i = 0; i < 16; i++, resourceData++) {
        resourceData->objectState = 0;
    }
    positionVector4.x = positionVector4.y = 0.0f;
    positionVector4.z = -1400.0f;
    UpdateCollisionPrism(&positionVector4, NULL, NULL, 0.8f, 0, 0x2003);
    prismVertices[0].x = 1000.0f;
    prismVertices[0].y = 1000.0f;
    prismVertices[0].z = -200.0f;
    prismVertices[1].x = -1000.0f;
    prismVertices[1].y = 1000.0f;
    prismVertices[1].z = -200.0f;
    prismVertices[2].x = -1000.0f;
    prismVertices[2].y = 0.0f;
    prismVertices[2].z = -200.0f;
    prismVertices[3].x = 1000.0f;
    prismVertices[3].y = 0.0f;
    prismVertices[3].z = -200.0f;
    UpdateCollisionPrism(prismVertices, NULL, NULL, 1.0f, 0, 6);
    positionVector4.x = -650.0f;
    positionVector4.y = 75.0f;
    positionVector4.z = -1300.0f;
    positionVector.x = 370.0f;
    positionVector.y = 1095.0f;
    positionVector.z = 120.0f;
    UpdateCollisionPrism(&positionVector4, &positionVector, NULL, 0.8f, 25, 0x2004);
    positionVector4.x = 650.0f;
    positionVector4.y = 75.0f;
    positionVector4.z = -1300.0f;
    positionVector.x = 370.0f;
    positionVector.y = 1095.0f;
    positionVector.z = 120.0f;
    UpdateCollisionPrism(&positionVector4, &positionVector, NULL, 0.8f, 21, 0x2004);
    positionVector4.x = 0.0f;
    positionVector4.y = 100.0f;
    positionVector4.z = -1070.0f;
    positionVector.x = 245.0f;
    positionVector.y = 130.0f;
    positionVector.z = 170.0f;
    UpdateCollisionPrism(&positionVector4, &positionVector, NULL, 1.0f, 45, 4);
    positionVector4.x = -235.0f;
    positionVector4.y = 0.0f;
    positionVector4.z = -900.0f;
    positionVector.x = -235.0f;
    positionVector.y = 200.0f;
    positionVector.z = -900.0f;
    positionVector2.x = 30.000002f;
    positionVector2.y = positionVector2.z = 0.0f;
    UpdateCollisionPrism(&positionVector4, &positionVector, &positionVector2, 0.5f, 0, 0x4007);
    positionVector4.x = 235.0f;
    positionVector4.y = 0.0f;
    positionVector4.z = -900.0f;
    positionVector.x = 235.0f;
    positionVector.y = 200.0f;
    positionVector.z = -900.0f;
    positionVector2.x = 30.000002f;
    positionVector2.y = positionVector2.z = 0.0f;
    UpdateCollisionPrism(&positionVector4, &positionVector, &positionVector2, 0.5f, 0, 0x4007);
    positionVector4.x = 0.0f;
    positionVector4.y = 200.0f;
    positionVector4.z = -900.0f;
    positionVector.x = 245.0f;
    positionVector.y = 30.0f;
    positionVector.z = 30.0f;
    UpdateCollisionPrism(&positionVector4, &positionVector, NULL, 0.5f, 29, 0x4004);
    prismVertices[0].x = -225.0f;
    prismVertices[0].y = 200.0f;
    prismVertices[0].z = -900.0f;
    prismVertices[1].x = -225.0f;
    prismVertices[1].y = 200.0f;
    prismVertices[1].z = -1150.0f;
    prismVertices[2].x = -225.0f;
    prismVertices[2].y = 0.0f;
    prismVertices[2].z = -1150.0f;
    prismVertices[3].x = -225.0f;
    prismVertices[3].y = 0.0f;
    prismVertices[3].z = -900.0f;
    UpdateCollisionPrism(prismVertices, NULL, NULL, 0.5f, 0, 0x8006);
    prismVertices[0].x = 225.0f;
    prismVertices[0].y = 200.0f;
    prismVertices[0].z = -1150.0f;
    prismVertices[1].x = 225.0f;
    prismVertices[1].y = 200.0f;
    prismVertices[1].z = -900.0f;
    prismVertices[2].x = 225.0f;
    prismVertices[2].y = 0.0f;
    prismVertices[2].z = -900.0f;
    prismVertices[3].x = 225.0f;
    prismVertices[3].y = 0.0f;
    prismVertices[3].z = -1150.0f;
    UpdateCollisionPrism(prismVertices, NULL, NULL, 0.5f, 0, 0x8006);
    prismVertices[0].x = -225.0f;
    prismVertices[0].y = 200.0f;
    prismVertices[0].z = -1150.0f;
    prismVertices[1].x = 225.0f;
    prismVertices[1].y = 200.0f;
    prismVertices[1].z = -1150.0f;
    prismVertices[2].x = 225.0f;
    prismVertices[2].y = 0.0f;
    prismVertices[2].z = -1150.0f;
    prismVertices[3].x = -225.0f;
    prismVertices[3].y = 0.0f;
    prismVertices[3].z = -1150.0f;
    UpdateCollisionPrism(prismVertices, NULL, NULL, 0.1f, 0, 0x8006);
    prismVertices[0].x = -225.0f;
    prismVertices[0].y = 200.0f;
    prismVertices[0].z = -900.0f;
    prismVertices[1].x = 225.0f;
    prismVertices[1].y = 200.0f;
    prismVertices[1].z = -900.0f;
    prismVertices[2].x = 225.0f;
    prismVertices[2].y = 200.0f;
    prismVertices[2].z = -1150.0f;
    prismVertices[3].x = -225.0f;
    prismVertices[3].y = 200.0f;
    prismVertices[3].z = -1150.0f;
    UpdateCollisionPrism(prismVertices, NULL, NULL, 0.1f, 0, 0x8006);
}

s16 collisionPrismTriangleIndices[][3] = { { 0, 1, 2 }, { 1, 3, 2 }, { 6, 5, 4 }, { 5, 6, 7 }, { 0, 2, 4 }, { 2, 6, 4 }, { 3, 1, 7 }, { 7, 1, 5 }, { 2, 3, 6 },
    { 3, 7, 6 }, { 1, 0, 4 }, { 1, 4, 5 } };

void UpdateCollisionPrism(Vec *vertices, Vec *sourcePosition, Vec *normal, float radius, s32 prismIndex, s32 flags)
{
    Vec positionVector5[8];
    Vec positionVector6;
    float floatValue5;
    CollisionPrismData *prismData;
    s32 i;
    s32 j;

    prismData = collisionPrisms;
    for (i = 0; i < 16; i++, prismData++) {
        if (prismData->objectState == 0) {
            break;
        }
    }
    if (i >= 16) {
        return;
    }
    prismData->objectIndex = i;
    prismData->objectState = flags;
    prismData->animationWork = radius;
    switch (prismData->objectState & 0xFFF) {
        case 1:
            prismData->effectState = vertices[0].x;
            break;
        case 2:
            prismData->effectState = vertices[0].y;
            break;
        case 3:
            prismData->effectState = vertices[0].z;
            break;
        case 4:
            prismData->objectPosition = vertices[0];
            prismData->positionZ = *position;
            positionVector5[0].x = vertices[0].x - prismData->x;
            positionVector5[0].y = vertices[0].y + prismData->y;
            positionVector5[0].z = vertices[0].z - prismData->z;
            positionVector5[1].x = vertices[0].x + prismData->x;
            positionVector5[1].y = vertices[0].y + prismData->y;
            positionVector5[1].z = vertices[0].z - prismData->z;
            positionVector5[2].x = vertices[0].x - prismData->x;
            positionVector5[2].y = vertices[0].y + prismData->y;
            positionVector5[2].z = vertices[0].z + prismData->z;
            positionVector5[3].x = vertices[0].x + prismData->x;
            positionVector5[3].y = vertices[0].y + prismData->y;
            positionVector5[3].z = vertices[0].z + prismData->z;
            positionVector5[4].x = vertices[0].x - prismData->x;
            positionVector5[4].y = vertices[0].y - prismData->y;
            positionVector5[4].z = vertices[0].z - prismData->z;
            positionVector5[5].x = vertices[0].x + prismData->x;
            positionVector5[5].y = vertices[0].y - prismData->y;
            positionVector5[5].z = vertices[0].z - prismData->z;
            positionVector5[6].x = vertices[0].x - prismData->x;
            positionVector5[6].y = vertices[0].y - prismData->y;
            positionVector5[6].z = vertices[0].z + prismData->z;
            positionVector5[7].x = vertices[0].x + prismData->x;
            positionVector5[7].y = vertices[0].y - prismData->y;
            positionVector5[7].z = vertices[0].z + prismData->z;
            for (j = 0; j < 12; j++) {
                CalculatePlaneFromPoints(&positionVector5[collisionPrismTriangleIndices[j][0]], &positionVector5[collisionPrismTriangleIndices[j][1]], &positionVector5[collisionPrismTriangleIndices[j][2]], prismData->objectColor[j]);
                prismData->controlPoints[j][0] = positionVector5[collisionPrismTriangleIndices[j][0]];
                prismData->controlPoints[j][1] = positionVector5[collisionPrismTriangleIndices[j][1]];
                prismData->controlPoints[j][2] = positionVector5[collisionPrismTriangleIndices[j][2]];
            }
            prismData->objectData = prismIndex;
            break;
        case 5:
            prismData->controlPoints[0][0] = vertices[0];
            prismData->controlPoints[1][0] = vertices[1];
            prismData->controlPoints[2][0] = vertices[2];
            prismData->controlPoints[3][0] = vertices[3];
            CalculatePlaneFromPoints(&vertices[0], &vertices[1], &vertices[3], prismData->objectColor[0]);
            CalculatePlaneFromPoints(&vertices[1], &vertices[2], &vertices[3], prismData->objectColor[1]);
            break;
        case 6:
            prismData->controlPoints[0][0] = vertices[0];
            prismData->controlPoints[1][0] = vertices[1];
            prismData->controlPoints[2][0] = vertices[2];
            prismData->controlPoints[3][0] = vertices[3];
            CalculatePlaneFromPoints(&vertices[0], &vertices[1], &vertices[3], prismData->objectColor[0]);
            CalculatePlaneFromPoints(&vertices[1], &vertices[2], &vertices[3], prismData->objectColor[1]);
            if (prismData->objectColor[0][1] < -0.5f) {
                prismData->objectData = 0;
            }
            if (prismData->objectColor[0][1] > 0.5f) {
                prismData->objectData = 1;
            }
            if (prismData->objectColor[0][0] > 0.5f) {
                prismData->objectData = 2;
            }
            if (prismData->objectColor[0][0] < -0.5f) {
                prismData->objectData = 3;
            }
            if (prismData->objectColor[0][2] < -0.5f) {
                prismData->objectData = 4;
            }
            if (prismData->objectColor[0][2] > 0.5f) {
                prismData->objectData = 5;
            }
            break;
        case 7:
            prismData->objectPosition = vertices[0];
            prismData->positionZ = *position;
            prismData->effectState = normal->x;
            positionVector6.x = prismData->x - vertices[0].x;
            positionVector6.y = prismData->y - vertices[0].y;
            positionVector6.z = prismData->z - vertices[0].z;
            floatValue5 = VECMagXZ(&positionVector6);
            prismData->objectTimer = -atan2d(positionVector6.y, floatValue5);
            prismData->actionState = atan2d(positionVector6.x, positionVector6.z);
            prismData->actionTimer = 0.0f;
            break;
    }
}

s32 collisionSideIndices[] = { 1, 1, 0, 0, 2, 2 };

s32 FindCollisionIntersection(Vec *vertices, Vec *startPosition, Vec *endPosition, Vec *intersection, float radius, s32 flags)
{
    CollisionPrismData *resourceData;
    Vec positionVector7[6];
    Vec scratchData2;
    Vec transformMatrix2;
    Vec segmentStart;
    Vec positionVector8;
    Vec scratchData3;
    Vec intersectionPosition;
    Vec segmentStart2D;
    Vec segmentEnd2D;
    Vec prismPosition2D;
    Vec closestPosition;
    float floatValue;
    float floatValue6;
    float positionVector9;
    float scalarValue;
    float segmentProgress;
    s32 resourceData2;
    s32 position4;
    s32 model;
    s32 i;
    s32 j;

    resourceData = collisionPrisms;
    resourceData2 = -1;
    scratchData2.x = vertices->x + startPosition->x;
    scratchData2.y = vertices->y + startPosition->y;
    scratchData2.z = vertices->z + startPosition->z;
    for (i = 0; i < 16; i++, resourceData++) {
        if (resourceData->objectState == 0) {
            continue;
        }
        switch (resourceData->objectState & 0xFFF) {
            case 1:
                if (startPosition->x < 0.0f) {
                    if (vertices->x > resourceData->effectState + radius && scratchData2.x <= resourceData->effectState + radius + 1.0f) {
                        scratchData2.x = resourceData->effectState + radius;
                        *endPosition = scratchData2;
                        resourceData2 = 0;
                    }
                }
                else {
                    if (vertices->x < resourceData->effectState - radius && scratchData2.x >= resourceData->effectState - radius - 1.0f) {
                        scratchData2.x = resourceData->effectState - radius;
                        *endPosition = scratchData2;
                        resourceData2 = 0;
                    }
                }
                break;
            case 2:
                if (startPosition->y < 0.0f) {
                    if (vertices->y > resourceData->effectState + radius && scratchData2.y <= resourceData->effectState + radius + 1.0f) {
                        scratchData2.y = resourceData->effectState + radius;
                        *endPosition = scratchData2;
                        resourceData2 = 1;
                    }
                }
                else {
                    if (vertices->y < resourceData->effectState - radius && scratchData2.y >= resourceData->effectState - radius - 1.0f) {
                        scratchData2.y = resourceData->effectState - radius;
                        *endPosition = scratchData2;
                        resourceData2 = 1;
                    }
                }
                break;
            case 3:
                if (startPosition->z < 0.0f) {
                    if (vertices->z > resourceData->effectState + radius && scratchData2.z <= resourceData->effectState + radius + 1.0f) {
                        scratchData2.z = resourceData->effectState + radius;
                        *endPosition = scratchData2;
                        resourceData2 = 2;
                    }
                }
                else {
                    if (vertices->z < resourceData->effectState - radius && scratchData2.z >= resourceData->effectState - radius - 1.0f) {
                        scratchData2.z = resourceData->effectState - radius;
                        *endPosition = scratchData2;
                        resourceData2 = 2;
                    }
                }
                break;
            case 4:
                radius = 0.0f;
                VECSubtract(&scratchData2, &resourceData->objectPosition, &transformMatrix2);
                if (fabs(transformMatrix2.x) < resourceData->positionZ.x + radius && fabs(transformMatrix2.y) < resourceData->positionZ.y + radius && fabs(transformMatrix2.z) < resourceData->positionZ.z + radius) {
                    for (j = 0; j < 6; j++) {
                        if (resourceData->objectData & (1 << j)) {
                            position4 = CheckSegmentPlaneIntersection(vertices, &scratchData2, resourceData->objectColor[j * 2]);
                            if (position4 != 0) {
                                break;
                            }
                        }
                    }
                    switch (j) {
                        case 0:
                            scratchData2.y = resourceData->objectPosition.y + resourceData->positionZ.y + radius + 1.0f;
                            break;
                        case 1:
                            scratchData2.y = resourceData->objectPosition.y - resourceData->positionZ.y - radius - 1.0f;
                            break;
                        case 2:
                            scratchData2.x = resourceData->objectPosition.x - resourceData->positionZ.x - radius - 1.0f;
                            break;
                        case 3:
                            scratchData2.x = resourceData->objectPosition.x + resourceData->positionZ.x + radius + 1.0f;
                            break;
                        case 4:
                            scratchData2.z = resourceData->objectPosition.z + resourceData->positionZ.z + radius + 1.0f;
                            break;
                        case 5:
                            scratchData2.z = resourceData->objectPosition.z - resourceData->positionZ.z - radius - 1.0f;
                            break;
                    }
                    if (j < 6) {
                        *endPosition = scratchData2;
                        resourceData2 = collisionSideIndices[j];
                    }
                }
                break;
            case 5:
                if (CheckSegmentPlaneIntersection(vertices, &scratchData2, resourceData->objectColor[0]) != 1) {
                    break;
                }
                IntersectLineWithPlane(resourceData->objectColor[0], vertices, startPosition, resourceData->controlPoints[0], &scratchData3);
                positionVector8.x = fabs(resourceData->objectColor[0][0]);
                positionVector8.y = fabs(resourceData->objectColor[0][1]);
                positionVector8.z = fabs(resourceData->objectColor[0][2]);
                if (positionVector8.x > positionVector8.y && positionVector8.x > positionVector8.z) {
                    position4 = 0;
                }
                else if (positionVector8.y > positionVector8.x && positionVector8.y > positionVector8.z) {
                    position4 = 1;
                }
                else {
                    position4 = 2;
                }
                positionVector7[0] = resourceData->controlPoints[0][0];
                positionVector7[1] = resourceData->controlPoints[1][0];
                positionVector7[2] = resourceData->controlPoints[3][0];
                positionVector7[3] = resourceData->controlPoints[1][0];
                positionVector7[4] = resourceData->controlPoints[2][0];
                positionVector7[5] = resourceData->controlPoints[3][0];
                positionVector9 = CheckPointInsidePolygon(&positionVector7[0], &scratchData3, position4);
                scalarValue = CheckPointInsidePolygon(&positionVector7[3], &scratchData3, position4);
                if (positionVector9 != 0.0f || scalarValue != 0.0f) {
                    endPosition->x = scratchData3.x + resourceData->objectColor[0][0] * (radius + 1.0f);
                    endPosition->y = scratchData3.y + resourceData->objectColor[0][1] * (radius + 1.0f);
                    endPosition->z = scratchData3.z + resourceData->objectColor[0][2] * (radius + 1.0f);
                    resourceData2 = position4;
                }
                break;
            case 6:
                positionVector7[0] = resourceData->controlPoints[0][0];
                positionVector7[1] = resourceData->controlPoints[1][0];
                positionVector7[2] = resourceData->controlPoints[2][0];
                positionVector7[3] = resourceData->controlPoints[3][0];
                if (CheckSegmentBoxIntersection(&positionVector7[0], vertices, startPosition, &scratchData3, radius, resourceData->objectData) != 0) {
                    *endPosition = scratchData3;
                    resourceData2 = collisionSideIndices[resourceData->objectData];
                }
                break;
            case 7:
                segmentStart2D.x = vertices->x;
                segmentStart2D.y = vertices->z;
                segmentStart2D.z = 0.0f;
                segmentEnd2D.x = scratchData2.x;
                segmentEnd2D.y = scratchData2.z;
                segmentEnd2D.z = 0.0f;
                prismPosition2D.x = resourceData->objectPosition.x;
                prismPosition2D.y = resourceData->objectPosition.z;
                prismPosition2D.z = 0.0f;
                floatValue = GetPointToSegmentDistance(&segmentStart2D, &segmentEnd2D, &prismPosition2D, &closestPosition, &segmentProgress);
                if (segmentProgress >= 0.0f && segmentProgress <= 1.0f && floatValue < resourceData->effectState + radius) {
                    floatValue6 = vertices->y + startPosition->y * segmentProgress;
                    if (floatValue6 >= resourceData->objectPosition.y && floatValue6 <= resourceData->positionZ.y) {
                        closestPosition.z = closestPosition.y;
                        closestPosition.y = 0.0f;
                        VECSubtract(&closestPosition, &resourceData->objectPosition, &transformMatrix2);
                        transformMatrix2.y = 0.0f;
                        if (VECMag(&transformMatrix2) == 0.0f) {
                            transformMatrix2.z = 1.0f;
                        }
                        else {
                            VECNormalize(&transformMatrix2, &transformMatrix2);
                        }
                        VECScale(&transformMatrix2, &transformMatrix2, resourceData->effectState + radius + 1.0f);
                        endPosition->x = transformMatrix2.x + resourceData->objectPosition.x;
                        endPosition->z = transformMatrix2.z + resourceData->objectPosition.z;
                        endPosition->y = floatValue6;
                        segmentProgress = floatValue;
                        resourceData2 = 99;
                    }
                }
                else {
                    segmentProgress = floatValue;
                    VECSubtract(&scratchData2, &resourceData->objectPosition, &transformMatrix2);
                    floatValue = VECMagXZ(&transformMatrix2);
                    if (floatValue < resourceData->effectState + radius && scratchData2.y >= resourceData->objectPosition.y && scratchData2.y <= resourceData->positionZ.y) {
                        VECSubtract(&scratchData2, &resourceData->objectPosition, &transformMatrix2);
                        transformMatrix2.y = 0.0f;
                        if (VECMag(&transformMatrix2) == 0.0f) {
                            transformMatrix2.z = 1.0f;
                        }
                        else {
                            VECNormalize(&transformMatrix2, &transformMatrix2);
                        }
                        VECScale(&transformMatrix2, &transformMatrix2, resourceData->effectState + radius + 1.0f);
                        endPosition->x = resourceData->objectPosition.x + transformMatrix2.x;
                        endPosition->z = resourceData->objectPosition.z + transformMatrix2.z;
                        endPosition->y = scratchData2.y;
                        resourceData2 = 99;
                    }
                }
                if (resourceData2 == -1) {
                    break;
                }
                transformMatrix2.x = closestPosition.x - resourceData->objectPosition.x;
                transformMatrix2.z = closestPosition.y - resourceData->objectPosition.z;
                floatValue = VECMagXZ(&transformMatrix2);
                if (floatValue == 0.0f) {
                    floatValue = 1.0f;
                    transformMatrix2.x = 0.0f;
                    transformMatrix2.z = 1.0f;
                }
                else {
                    floatValue = 1.0f / floatValue;
                }
                closestPosition.x = transformMatrix2.x * floatValue;
                closestPosition.z = transformMatrix2.z * floatValue;
                floatValue = VECMagXZ(startPosition);
                if (floatValue != 0.0f) {
                    floatValue = 1.0f / floatValue;
                }
                prismPosition2D.x = -(startPosition->x * floatValue);
                prismPosition2D.z = -(startPosition->z * floatValue);
                segmentProgress = floatValue / (resourceData->effectState + radius);
                transformMatrix2.x = prismPosition2D.x + segmentProgress * (closestPosition.x - prismPosition2D.x);
                transformMatrix2.z = prismPosition2D.z + segmentProgress * (closestPosition.z - prismPosition2D.z);
                floatValue = VECMagXZ(&transformMatrix2);
                if (floatValue == 0.0f) {
                    floatValue = 1.0f;
                    transformMatrix2.x = 0.0f;
                    transformMatrix2.z = 1.0f;
                }
                else {
                    floatValue = 1.0f / floatValue;
                }
                transformMatrix2.x = transformMatrix2.x * floatValue - prismPosition2D.x;
                transformMatrix2.z = transformMatrix2.z * floatValue - prismPosition2D.z;
                transformMatrix2.y = 0.0f;
                if (VECMag(&transformMatrix2) == 0.0f) {
                    transformMatrix2.z = 1.0f;
                }
                else {
                    VECNormalize(&transformMatrix2, &transformMatrix2);
                }
                floatValue = VECMagXZ(startPosition);
                intersection->x = transformMatrix2.x * floatValue * resourceData->animationWork;
                intersection->z = transformMatrix2.z * floatValue * resourceData->animationWork;
                intersection->y = startPosition->y;
                break;
        }
        if (resourceData2 != -1) {
            break;
        }
    }
    if (i < 16) {
        switch (resourceData2) {
            case 0:
                intersection->x = -startPosition->x * resourceData->animationWork;
                intersection->y = startPosition->y;
                intersection->z = startPosition->z * resourceData->animationWork;
                model = resourceData->objectState;
                break;
            case 1:
                intersection->x = startPosition->x * resourceData->animationWork;
                intersection->y = startPosition->y * -0.5f;
                intersection->z = startPosition->z * resourceData->animationWork;
                model = resourceData->objectState;
                break;
            case 2:
                intersection->x = startPosition->x * resourceData->animationWork;
                intersection->y = startPosition->y;
                intersection->z = -startPosition->z * resourceData->animationWork;
                model = resourceData->objectState;
                break;
            default:
                model = resourceData->objectState;
                break;
        }
        flags--;
        if (flags > 0) {
            segmentStart = *vertices;
            positionVector8.x = endPosition->x - vertices->x;
            positionVector8.y = endPosition->y - vertices->y;
            positionVector8.z = endPosition->z - vertices->z;
            resourceData2 = FindCollisionIntersection(&segmentStart, &positionVector8, &scratchData3, &intersectionPosition, angle, index);
            if (resourceData2 != 0) {
                *endPosition = scratchData3;
                *intersection = intersectionPosition;
                if (model & 0x4000) {
                    resourceData2 |= 0x4000;
                }
                if (model & 0x8000) {
                    resourceData2 |= 0x8000;
                }
                model = resourceData2;
            }
        }
        return model;
    }
    return 0;
}

void CreateStageModel(void)
{
    OMOBJ *position;
    StageModelHookData *resourceData;
    ANIMDATA *position2;
    ModelAnimationHookData *resourceData3;
    HU3DMODEL *index;
    s32 positionVector2;

    position = stageModelObject = omAddObjEx(objectManager, 0xBF, 0, 0, -1, UpdateStageModel);
    resourceData = position->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*resourceData), HU_MEMNUM_OVL);
    memset(resourceData, 0, sizeof(*resourceData));
    resourceData->position = position2 = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M423, 22), HU_MEMNUM_OVL));
    resourceData->objectIndex = positionVector2 = CreateModelAnimationHook(position2, 0x100);
    Hu3DModelCameraSet(resourceData->objectIndex, 4);
    Hu3DModelLayerSet(resourceData->objectIndex, 1);
    resourceData->objectPosition = 0;
    index = &Hu3DData[resourceData->objectIndex];
    resourceData3 = index->hookData;
    resourceData3->objectFlags = 0;
}

void UpdateStageModel(OMOBJ *object)
{
    StageModelHookData *position;
    HU3DMODEL *position2;
    ModelAnimationHookData *resourceData3;
    ModelAnimationState *resourceData;
    s32 i;

    position = object->data;
    if (position->objectPosition <= 0) {
        Hu3DModelAttrSet(position->objectIndex, HU3D_ATTR_DISPOFF);
        return;
    }
    Hu3DModelAttrReset(position->objectIndex, HU3D_ATTR_DISPOFF);
    position2 = &Hu3DData[position->objectIndex];
    resourceData3 = position2->hookData;
    resourceData = resourceData3->animationWork;
    resourceData3->activeState = 0;
    for (i = 0; i < resourceData3->animationFrame; i++, resourceData++) {
        if (resourceData->actionTimer <= 0.0f) {
            continue;
        }
        resourceData->animationData.x += resourceData->objectPosition.x;
        resourceData->animationData.y += resourceData->objectPosition.y;
        resourceData->animationData.z += resourceData->objectPosition.z;
        resourceData->effectState += resourceData->animationTimer;
        if (resourceData->effectState >= 360.0f) {
            resourceData->effectState -= 360.0f;
        }
        resourceData->frameIndex = resourceData->objectTimer;
        resourceData->objectTimer += resourceData->actionState;
        if (resourceData->frameIndex >= 16) {
            resourceData->objectState = 0;
            resourceData->objectIndex = 0;
            resourceData->actionTimer = 0.0f;
            position->objectPosition--;
        }
    }
}

void DestroyStageModel(void)
{
    OMOBJ *resourceData;
    StageModelHookData *position;

    resourceData = stageModelObject;
    position = resourceData->data;
    Hu3DModelAttrSet(position->objectIndex, HU3D_ATTR_DISPOFF);
    HuSprAnimKill(position->position);
    resourceData->objFunc = NULL;
}

inline s32 CheckStageCollision(Vec startPosition, Vec endPosition, float collisionRadius, float height, float depth, GXColor color)
{
    OMOBJ *position2;
    StageModelHookData *index;
    HU3DMODEL *resourceData2;
    ModelAnimationHookData *position;
    ModelAnimationState *resourceData;
    s32 i;

    position2 = stageModelObject;
    index = position2->data;
    resourceData2 = &Hu3DData[index->objectIndex];
    position = resourceData2->hookData;
    resourceData = position->animationWork;
    for (i = 0; i < position->animationFrame; i++, resourceData++) {
        if (resourceData->actionTimer <= 0.0f) {
            break;
        }
    }
    if (i >= position->animationFrame) {
        return -1;
    }
    resourceData->objectIndex = resourceData->objectState = 0;
    resourceData->animationData.x = startPosition.x;
    resourceData->animationData.y = startPosition.y;
    resourceData->animationData.z = startPosition.z;
    resourceData->objectPosition.x = endPosition.x;
    resourceData->objectPosition.y = endPosition.y;
    resourceData->objectPosition.z = endPosition.z;
    resourceData->animationTimer = height;
    resourceData->actionTimer = collisionRadius;
    resourceData->objectColor.r = color.r;
    resourceData->objectColor.g = color.g;
    resourceData->objectColor.b = color.b;
    resourceData->objectColor.a = color.a;
    resourceData->effectState = 0.0f;
    resourceData->objectState = 0;
    resourceData->objectIndex = 0;
    resourceData->objectTimer = 0.0f;
    resourceData->actionState = depth;
    index->objectPosition++;
    return i;
}

s32 CheckStageCollisionRotated(Vec startPosition, Vec endPosition, float collisionRadius, float height, float depth, float rotation, GXColor color)
{
    Vec positionVector10;
    Vec positionVector11;
    float floatValue3;
    s32 positionVector4;
    s32 collisionFlags;

    positionVector11.x = endPosition.z;
    positionVector11.z = endPosition.x;
    positionVector11.y = 0.0f;
    VECNormalize(&positionVector11, &positionVector11);
    floatValue3 = 0.5f * depth;
    positionVector10.x = startPosition.x + positionVector11.x * floatValue3;
    positionVector10.y = startPosition.y + positionVector11.y * floatValue3;
    positionVector10.z = startPosition.z + positionVector11.z * floatValue3;
    positionVector4 = CheckStageCollision(positionVector10, endPosition, angle, targetAngle, color, color);
    positionVector10.x = startPosition.x - positionVector11.x * floatValue3;
    positionVector10.y = startPosition.y - positionVector11.y * floatValue3;
    positionVector10.z = startPosition.z - positionVector11.z * floatValue3;
    collisionFlags = CheckStageCollision(positionVector10, endPosition, angle, -targetAngle, color, color);
    return (positionVector4 << 16) | collisionFlags;
}

void CreateBackgroundModels(void)
{
    OMOBJ *resourceData;
    s32 resourceData3;
    s32 i;

    resourceData = backgroundModelObject = omAddObjEx(objectManager, 0xBF, 16, 0, -1, UpdateBackgroundModels);
    for (i = 0; i < resourceData->mdlcnt; i++) {
        resourceData3 = resourceData->mdlId[i] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M423, 3));
        Hu3DModelAttrSet(resourceData3, HU3D_ATTR_DISPOFF);
        Hu3DModelCameraSet(resourceData3, 4);
    }
    resourceData->stat |= 0x100;
}

void UpdateBackgroundModels(OMOBJ *object)
{
    HU3DMODEL *index;
    s32 position;
    s32 i;

    for (i = 0; i < object->mdlcnt; i++) {
        position = object->mdlId[i];
        index = &Hu3DData[position];
        if (!(index->attr & 1) && Hu3DMotionEndCheck(position)) {
            Hu3DModelAttrSet(position, HU3D_ATTR_DISPOFF);
        }
    }
}

void EmptyBackgroundCallback(void) { }

s32 CheckBackgroundCollision(Vec startPosition, Vec endPosition, float collisionRadius)
{
    OMOBJ *resourceData;
    HU3DMODEL *resourceData2;
    s32 resourceData3;
    s32 i;

    resourceData = backgroundModelObject;
    for (i = 0; i < resourceData->mdlcnt; i++) {
        resourceData2 = &Hu3DData[resourceData->mdlId[i]];
        if (resourceData2->attr & 1) {
            break;
        }
    }
    if (i >= resourceData->mdlcnt) {
        return -1;
    }
    resourceData3 = resourceData->mdlId[i];
    Hu3DModelPosSet(resourceData3, startPosition.x, startPosition.y, startPosition.z);
    Hu3DModelRotSet(resourceData3, endPosition.x, endPosition.y, endPosition.z);
    Hu3DModelScaleSet(resourceData3, collisionRadius, collisionRadius, collisionRadius);
    Hu3DMotionTimeSet(resourceData3, 0.0f);
    Hu3DMotionSpeedSet(resourceData3, 2.0f);
    Hu3DModelAttrReset(resourceData3, HU3D_ATTR_DISPOFF);
    return i;
}

void CreateShadowModel(void)
{
    OMOBJ *object;

    object = shadowModelObject = omAddObjEx(objectManager, 0xBF, 1, 0, -1, UpdateShadowModel);
    object->mdlId[0] = CreateCaptureHook();
    Hu3DModelCameraSet(object->mdlId[0], 4);
    Hu3DModelLayerSet(object->mdlId[0], 2);
}

void UpdateShadowModel(OMOBJ *object)
{
    HU3DMODEL *resourceData3;
    SceneRenderData *index;
    OverlayVertexData *resourceData;
    s32 i;

    resourceData3 = &Hu3DData[object->mdlId[0]];
    index = resourceData3->hookData;
    resourceData = index->objectIndex;
    for (i = 0; i < 16; i++, resourceData++) {
        if (resourceData->objectIndex == 0) {
            continue;
        }
        resourceData->objectIndex++;
        if (resourceData->objectIndex < REFRESH_RATE_F / 5) {
            resourceData->objectPosition += 3.5f * REFRESH_FREQ;
        }
        else {
            resourceData->objectPosition += 1.5f * REFRESH_FREQ;
            resourceData->positionZ -= 3.0f * REFRESH_FREQ;
        }
        if (resourceData->positionZ < 0.0f) {
            resourceData->objectIndex = 0;
        }
    }
}

void DestroyShadowModel(void)
{
    OMOBJ *resourceData;
    HU3DMODEL *position;
    SceneRenderData *resourceData3;

    resourceData = shadowModelObject;
    position = &Hu3DData[resourceData->mdlId[0]];
    resourceData3 = position->hookData;
    HuSprAnimKill(resourceData3->animationData);
    resourceData->objFunc = NULL;
}

s32 CreateCaptureHook(void)
{
    float scratchData[32][2];
    float positionVector2[32];
    float floatValue7;
    float floatValue;
    float floatValue5;
    float floatValue3;
    HU3DMODEL *position2;
    SceneRenderData *position;
    ModelTransformRecord *resourceData3;
    ModelAnimationRecord *index;
    s32 resourceData2;
    s32 i;

    resourceData2 = Hu3DHookFuncCreate(CaptureModelTexture);
    position2 = &Hu3DData[resourceData2];
    position = position2->hookData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(*position), position2->mallocNo);
    memset(position, 0, sizeof(*position));
    position->animationTable = HuMemDirectMallocNum(HEAP_MODEL, 32 * sizeof(*position->animationTable), position2->mallocNo);
    memset(position->animationTable, 0, 32 * sizeof(*position->animationTable));
    position->transformTable = HuMemDirectMallocNum(HEAP_MODEL, 32 * sizeof(*position->transformTable), position2->mallocNo);
    memset(position->transformTable, 0, 32 * sizeof(*position->transformTable));
    position->colorTable = HuMemDirectMallocNum(HEAP_MODEL, 5 * sizeof(*position->colorTable), position2->mallocNo);
    memset(position->colorTable, 0, 5 * sizeof(*position->colorTable));
    position->animationState = GXGetTexBufferSize(640, 480, GX_TF_RGB5A3, GX_FALSE, 0);
    position->resourceData = HuMemDirectMallocNum(HEAP_MODEL, position->animationState, position2->mallocNo);
    DCFlushRange(position->resourceData, position->animationState);
    position->animationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M423, 23), HU_MEMNUM_OVL));
    floatValue3 = 0.0f;
    for (i = 0; i < 32; i++) {
        floatValue7 = 1.0f + 0.001f * frandmod(1000);
        positionVector2[i] = floatValue7;
        floatValue3 += floatValue7;
        scratchData[i][1] = 80.0f * floatValue7;
        scratchData[i][0] = 10.0f + 0.02f * frandmod(1000);
    }
    floatValue3 = 360.0f / floatValue3;
    for (i = 0; i < 32; i++) {
        positionVector2[i] *= floatValue3;
    }
    floatValue3 = frandmod(90);
    index = position->animationTable;
    resourceData3 = position->transformTable;
    for (i = 0; i < 32; index++, resourceData3++, i++) {
        floatValue5 = -cosd(floatValue3);
        floatValue = sind(floatValue3);
        index->animationData = floatValue5 * scratchData[i][0];
        index->objectTimer = floatValue * scratchData[i][0];
        index->positionX = floatValue5 * (scratchData[i][0] + scratchData[i][1]);
        index->positionZ = floatValue * (scratchData[i][0] + scratchData[i][1]);
        index->objectIndex = floatValue5 * (scratchData[i][0] + 0.85f * scratchData[i][1]);
        index->objectPosition = floatValue * (scratchData[i][0] + 0.85f * scratchData[i][1]);
        resourceData3->objectIndex = -0.3f * -floatValue5;
        resourceData3->objectPosition = -0.3f * floatValue;
        floatValue5 = -cosd(floatValue3 - 0.4f * positionVector2[i]);
        floatValue = sind(floatValue3 - 0.4f * positionVector2[i]);
        index->actionTimer = floatValue5 * (scratchData[i][0] + 0.7f * scratchData[i][1]);
        index->effectState = floatValue * (scratchData[i][0] + 0.7f * scratchData[i][1]);
        floatValue5 = -cosd(floatValue3 + 0.4f * positionVector2[i]);
        floatValue = sind(floatValue3 + 0.4f * positionVector2[i]);
        index->animationTimer = floatValue5 * (scratchData[i][0] + 0.7f * scratchData[i][1]);
        index->motionData = floatValue * (scratchData[i][0] + 0.7f * scratchData[i][1]);
        resourceData3->position = resourceData3->positionX = resourceData3->positionZ = resourceData3->targetPosition = 0.0f;
        resourceData3->animationTimer = resourceData3->motionData = resourceData3->objectFlags = resourceData3->animationData = 0.0f;
        floatValue3 += positionVector2[i];
    }
    for (i = 0; i < 5; i++) {
        position->colorTable[i].r = position->colorTable[i].g = position->colorTable[i].b = 0xFF;
        position->colorTable[i].a = 0xFF;
    }
    position->sequenceId = 32 * 54;
    position->sequenceData = HuMemDirectMallocNum(HEAP_HEAP, position->sequenceId, HU_MEMNUM_OVL);
    GXBeginDisplayList(position->sequenceData, position->sequenceId);
    for (i = 0; i < 32; i++) {
        GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, 6);

        GXPosition1x16(i * 5);
        GXColor1x16(0);
        GXTexCoord1x16(i * 5);

        GXPosition1x16(i * 5 + 1);
        GXColor1x16(1);
        GXTexCoord1x16(i * 5 + 1);

        GXPosition1x16(i * 5 + 2);
        GXColor1x16(2);
        GXTexCoord1x16(i * 5 + 2);

        GXPosition1x16(i * 5 + 3);
        GXColor1x16(3);
        GXTexCoord1x16(i * 5 + 3);

        GXPosition1x16(i * 5 + 4);
        GXColor1x16(4);
        GXTexCoord1x16(i * 5 + 4);

        GXPosition1x16(i * 5 + 1);
        GXColor1x16(1);
        GXTexCoord1x16(i * 5 + 1);

        GXEnd();
    }
    position->sequenceState = GXEndDisplayList();
    DCFlushRangeNoSync(position->animationTable, 32 * sizeof(*position->animationTable));
    DCFlushRangeNoSync(position->transformTable, 32 * sizeof(*position->transformTable));
    DCFlushRangeNoSync(position->colorTable, 5 * sizeof(*position->colorTable));
    DCFlushRangeNoSync(position->sequenceData, position->sequenceId);
    DCStoreRangeNoSync(position, sizeof(*position));
    PPCSync();
    return resourceData2;
}

GXColor overlayPlayerColors[] = { { 0x66, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0x66, 0xFF }, { 0x66, 0x00, 0x5B, 0xFF }, { 0x00, 0x66, 0x00, 0xFF },
    { 0x42, 0x00, 0x4C, 0xFF }, { 0x47, 0x28, 0x00, 0xFF }, { 0x66, 0x66, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF } };

void CaptureModelTexture(HU3DMODEL *model, Mtx transform)
{
    Mtx transformMatrix3;
    Mtx textureMatrix;
    Mtx reflectionMatrix;
    Mtx transformMatrix;
    Mtx transformMatrix4;
    Mtx positionVector9;
    GXTexObj positionVector;
    GXColor positionVector3 = { 0xCC, 0xCC, 0xCC, 0x7F };
    SceneRenderData *position;
    OverlayVertexData *resourceData;
    s32 i;

    position = model->hookData;
    GXSetTexCopySrc(0, 0, 640, 480);
    GXSetTexCopyDst(640, 480, GX_TF_RGB5A3, GX_FALSE);
    GXCopyTex(position->resourceData, GX_FALSE);
    GXPixModeSync();
    MTXTrans(textureMatrix, -400.0f, 400.0f, -700.0f);
    MTXConcat(Hu3DCameraMtx, textureMatrix, transformMatrix4);
    GXLoadPosMtxImm(transformMatrix4, GX_PNMTX0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    GXInitTexObj(&positionVector, position->resourceData, 640, 480, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObjLOD(&positionVector, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXLoadTexObj(&positionVector, GX_TEXMAP0);
    HuSprTexLoad(position->animationData, 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetNumTexGens(2);
    C_MTXLightPerspective(transformMatrix, 41.5f, 1.2f, 0.5555555f, -0.5f, 0.5555555f, 0.5f);
    MTXConcat(transformMatrix, Hu3DCameraMtx, transformMatrix);
    MTXInverse(Hu3DCameraMtx, positionVector9);
    MTXConcat(positionVector9, transformMatrix4, transformMatrix3);
    MTXConcat(transformMatrix, transformMatrix3, textureMatrix);
    GXLoadTexMtxImm(textureMatrix, GX_TEXMTX0, GX_MTX3x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEXCOORD0, GX_TEXMTX1, GX_FALSE, GX_PTIDENTITY);
    GXSetNumIndStages(1);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD1, GX_TEXMAP1);
    GXSetTevIndWarp(GX_TEVSTAGE0, GX_INDTEXSTAGE0, GX_TRUE, GX_FALSE, GX_ITM_0);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
    MTXScale(textureMatrix, -0.5f, -0.5f, 0.5f);
    GXSetIndTexMtx(GX_ITM_0, (float(*)[3])textureMatrix, -1);
    GXSetNumTevStages(1);
    GXSetTevColor(GX_TEVREG1, positionVector3);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C1, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_RASA, GX_CA_A0, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
    GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_POS, position->animationTable, 12);
    GXSetArray(GX_VA_CLR0, position->colorTable, 4);
    GXSetArray(GX_VA_TEX0, position->transformTable, 8);
    resourceData = position->objectIndex;
    for (i = 0; i < 16; i++, resourceData++) {
        if (resourceData->objectIndex == 0) {
            continue;
        }
        overlayPlayerColors[resourceData->objectState].a = 255.0f * resourceData->positionZ;
        GXSetTevColor(GX_TEVREG0, overlayPlayerColors[resourceData->objectState]);
        MTXTrans(textureMatrix, resourceData->targetPosition.x, resourceData->targetPosition.y, resourceData->targetPosition.z);
        MTXRotRad(transformMatrix3, 'Z', MTXDegToRad(resourceData->positionX));
        MTXConcat(textureMatrix, transformMatrix3, textureMatrix);
        MTXScale(reflectionMatrix, resourceData->objectPosition * resourceData->position, resourceData->objectPosition * resourceData->position, resourceData->objectPosition * resourceData->position);
        MTXConcat(textureMatrix, reflectionMatrix, textureMatrix);
        MTXConcat(Hu3DCameraMtx, textureMatrix, transformMatrix4);
        GXLoadPosMtxImm(transformMatrix4, GX_PNMTX0);
        MTXConcat(positionVector9, transformMatrix4, transformMatrix3);
        MTXConcat(transformMatrix, transformMatrix3, textureMatrix);
        GXLoadTexMtxImm(textureMatrix, GX_TEXMTX0, GX_MTX3x4);
        MTXRotRad(transformMatrix3, 'Z', MTXDegToRad(-resourceData->positionX));
        GXLoadTexMtxImm(transformMatrix3, GX_TEXMTX1, GX_MTX2x4);
        GXCallDisplayList(position->sequenceData, position->sequenceState);
    }
    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
}

s32 SetShadowModelPosition(Vec position, float shadowScale, s32 shadowState)
{
    OMOBJ *resourceData3;
    HU3DMODEL *index;
    SceneRenderData *resourceData2;
    OverlayVertexData *resourceData;
    s32 position2;
    s32 i;

    resourceData3 = shadowModelObject;
    index = &Hu3DData[resourceData3->mdlId[0]];
    resourceData2 = index->hookData;
    resourceData = resourceData2->objectIndex;
    for (i = 0; i < 16; i++, resourceData++) {
        if (resourceData->objectIndex == 0) {
            break;
        }
    }
    if (i < 16) {
        if (shadowState < 0) {
            shadowState = 0;
        }
        resourceData->objectIndex = 1;
        resourceData->objectState = shadowState;
        resourceData->positionZ = 1.0f;
        resourceData->position = shadowScale;
        resourceData->objectPosition = 0.25f;
        resourceData->positionX = frandmod(360);
        resourceData->targetPosition = position;
    }
}

void CreateOverlayObjects(void)
{
    overlayObject = omAddObjEx(objectManager, 0xFE, 0, 0, -1, UpdateOverlayObject);
}

s32 overlayResourceIds[] = { DATA_MAKE_NUM(DATADIR_MGCONST, 12), DATA_MAKE_NUM(DATADIR_MGCONST, 13), DATA_MAKE_NUM(DATADIR_MGCONST, 14),
    DATA_MAKE_NUM(DATADIR_MGCONST, 15), DATA_MAKE_NUM(DATADIR_MGCONST, 16), DATA_MAKE_NUM(DATADIR_MGCONST, 17), DATA_MAKE_NUM(DATADIR_MGCONST, 18),
    DATA_MAKE_NUM(DATADIR_MGCONST, 19) };

s16 overlaySpriteDimensions[] = { 0x7F, 0x7F, 0x7F, 0x100 };

s16 overlaySpriteOffsets[][2] = { { -75, 10 }, { -61, -14 }, { -47, 10 }, { 0, 0 }, { -12, -8 }, { -12, -8 }, { 6, -8 }, { 6, -8 }, { 24, -8 }, { 24, -8 },
    { 42, -8 }, { 42, -8 }, { 60, -8 }, { 60, -8 }, { -12, 10 }, { -12, 10 }, { 6, 10 }, { 6, 10 }, { 24, 10 }, { 24, 10 }, { 42, 10 }, { 42, 10 },
    { 60, 10 }, { 60, 10 } };

void UpdateOverlayObject(OMOBJ *object)
{
    OverlayAnimationData *resourceData;
    s32 positionVector2;
    s32 resourceData3;
    s32 i;

    if ((positionVector2 = playerCharacterIds[3]) != -1) {
        resourceData = object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*resourceData), HU_MEMNUM_OVL);
        memset(resourceData, 0, sizeof(*resourceData));
        resourceData->objectIndex = 480;
        resourceData->objectPosition = 70;
        resourceData->position = 0;
        resourceData->positionX = 0;
        for (i = 0; i < 24; i++) {
            switch (i) {
                case 0:
                case 1:
                case 2:
                    resourceData3 = espEntry(overlayResourceIds[playerCharacterIds[i]], overlaySpriteDimensions[i], 0);
                    resourceData->positionZ[i] = resourceData3;
                    resourceData->rotationZ[i][0] = overlaySpriteOffsets[i][0];
                    resourceData->rotationZ[i][1] = overlaySpriteOffsets[i][1];
                    break;
                case 3:
                    resourceData3 = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 38), overlaySpriteDimensions[i], 0);
                    resourceData->positionZ[i] = resourceData3;
                    resourceData->rotationZ[i][0] = overlaySpriteOffsets[i][0];
                    resourceData->rotationZ[i][1] = overlaySpriteOffsets[i][1];
                    break;
                default:
                    if (!(i & 1)) {
                        resourceData3 = espEntry(DATA_MAKE_NUM(DATADIR_M423, 24), 0x40, 0);
                        resourceData->positionZ[i] = resourceData3;
                    }
                    else {
                        resourceData3 = espEntry(DATA_MAKE_NUM(DATADIR_M423, 25), 0x20, 0);
                        resourceData->positionZ[i] = resourceData3;
                    }
                    resourceData->rotationZ[i][0] = overlaySpriteOffsets[i][0];
                    resourceData->rotationZ[i][1] = overlaySpriteOffsets[i][1];
                    break;
            }
            espDispOn(resourceData3);
            espDrawNoSet(resourceData3, 0);
            espAttrSet(resourceData3, 1);
            espPosSet(resourceData3, resourceData->objectIndex + resourceData->rotationZ[i][0], resourceData->objectPosition + resourceData->rotationZ[i][1]);
            espBankSet(resourceData3, 0);
        }
        for (i = 0; i < 10; i++) {
            resourceData->playerPositions[i][0] = 0;
            resourceData->playerPositions[i][1] = 0;
        }
        SetOverlayState(0);
        object->objFunc = UpdateOverlayAnimation;
    }
}

void UpdateOverlayAnimation(OMOBJ *object)
{
    float floatValue3;
    float floatValue7;
    float floatValue;
    float floatValue5;
    OverlayAnimationData *resourceData;
    s32 index;
    s32 resourceData3;
    s32 i;

    resourceData = object->data;
    if (playerSelectionTimer != resourceData->position) {
        resourceData->position = playerSelectionTimer;
        resourceData->positionX = 20;
    }
    if (resourceData->positionX != 0) {
        floatValue3 = 1.0 + 0.8f * sind(180.0f * (resourceData->positionX / 20.0f));
        resourceData->positionX--;
    }
    else {
        floatValue3 = 1.0f;
    }
    floatValue3 = 1.0f;
    index = 10 - playerSelectionTimer;
    for (i = 0; i < 4; i++) {
        floatValue = resourceData->objectIndex + resourceData->rotationZ[i][0];
        floatValue5 = resourceData->objectPosition + resourceData->rotationZ[i][1];
        espPosSet(resourceData->positionZ[i], floatValue, floatValue5);
        espScaleSet(resourceData->positionZ[i], 1.0f, 1.0f);
    }
    for (i = 4; i < 24; i++) {
        floatValue = resourceData->objectIndex + resourceData->rotationZ[i][0];
        floatValue5 = resourceData->objectPosition + resourceData->rotationZ[i][1];
        if (!(i & 1)) {
            espPosSet(resourceData->positionZ[i], floatValue, floatValue5);
            espScaleSet(resourceData->positionZ[i], 1.0f, 1.0f);
        }
        else {
            resourceData3 = (i - 4) / 2;
            if (index > resourceData3) {
                resourceData->playerPositions[resourceData3][0] = 1;
            }
            else {
                resourceData->playerPositions[resourceData3][0] = 0;
            }
            if (resourceData->playerPositions[resourceData3][0] != 0) {
                if (resourceData->playerPositions[resourceData3][1] < REFRESH_RATE / 4) {
                    resourceData->playerPositions[resourceData3][1]++;
#if VERSION_PAL
                    floatValue3 = 3.0f - 2.0f * (resourceData->playerPositions[resourceData3][1] / 12.0f);
                    floatValue7 = resourceData->playerPositions[resourceData3][1] / 12.0f;
#else
                    floatValue3 = 3.0f - 2.0f * (resourceData->playerPositions[resourceData3][1] / 15.0f);
                    floatValue7 = resourceData->playerPositions[resourceData3][1] / 15.0f;
#endif
                }
                else {
                    floatValue3 = 1.0f;
                    floatValue7 = 1.0f;
                }
                espDispOn(resourceData->positionZ[i]);
                espPosSet(resourceData->positionZ[i], floatValue, floatValue5);
                espTPLvlSet(resourceData->positionZ[i], floatValue7);
                espScaleSet(resourceData->positionZ[i], floatValue3, floatValue3);
            }
            else {
                espDispOff(resourceData->positionZ[i]);
            }
        }
    }
}

void DestroyOverlayObject(OMOBJ *object)
{
    OverlayAnimationData *resourceData3;
    s32 i;

    if (object && object->data) {
        resourceData3 = object->data;
        for (i = 0; i < 24; i++) {
            espKill(resourceData3->positionZ[i]);
        }
        object->data = NULL;
        object->objFunc = NULL;
    }
}

void DestroyOverlayObjects(void)
{
    DestroyOverlayObject(overlayObject);
}

void SetOverlayState(s32 object)
{
    OverlayAnimationData *position;
    s32 i;

    position = overlayObject->data;
    for (i = 0; i < 24; i++) {
        if (object != 0) {
            espDispOn(position->positionZ[i]);
        }
        else {
            espDispOff(position->positionZ[i]);
        }
        if (i >= 4 && (i & 1)) {
            espDispOff(position->positionZ[i]);
        }
    }
}

void InitializeCameras(void)
{
    OMOBJ *object;
    s32 i;

    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 45.0f, 50.0f, 50000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 320.0f, 240.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 320, 240);
    Hu3DCameraCreate(2);
    Hu3DCameraPerspectiveSet(2, 45.0f, 50.0f, 50000.0f, 1.2f);
    Hu3DCameraViewportSet(2, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(2, 0, 0, 640, 480);
    Hu3DCameraCreate(4);
    Hu3DCameraPerspectiveSet(4, 45.0f, 100.0f, 50000.0f, 1.2f);
    Hu3DCameraViewportSet(4, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(4, 0, 0, 640, 480);
    object = activePlayerObject = omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutViewMulti);
    object->work[0] = 3;
    for (i = 0; i < object->work[0]; i++) {
        CRotM[i].x = introCameraPresets[0].rotation.x;
        CRotM[i].y = introCameraPresets[0].rotation.y;
        CRotM[i].z = introCameraPresets[0].rotation.z;
        CenterM[i].x = introCameraPresets[0].center.x;
        CenterM[i].y = introCameraPresets[0].center.y;
        CenterM[i].z = introCameraPresets[0].center.z;
        CZoomM[i] = introCameraPresets[0].zoom;
    }
    sceneSelection = 0;
    sceneTimer = -1;
    omAddObjEx(objectManager, 0xFF, 0, 0, -1, UpdateCameraRotation);
}

void UpdateCameraRotation(OMOBJ *object)
{
    CRotM[1].x = CRotM[0].x;
    CRotM[1].y = CRotM[0].y;
    CRotM[1].z = CRotM[0].z;
    CenterM[1].x = CenterM[0].x;
    CenterM[1].y = CenterM[0].y;
    CenterM[1].z = CenterM[0].z;
    CZoomM[1] = CZoomM[0];
}

void EmptySceneSelectionCallback(void) { }

s32 GetSceneSelection(void)
{
    return sceneSelection;
}

void InterpolateCameraPreset(s32 cameraIndex, s32 startPresetIndex, s32 targetPresetIndex, f32 transitionSpeed)
{
    CameraPreset *startPreset;
    CameraPreset *targetPreset;

    startPreset = &introCameraPresets[startPresetIndex];
    targetPreset = &introCameraPresets[targetPresetIndex];
    CRotM[cameraIndex].x = ApproachAngleBySpeed(transitionSpeed, startPreset->rotation.x, targetPreset->rotation.x);
    CRotM[cameraIndex].y = ApproachAngleBySpeed(transitionSpeed, startPreset->rotation.y, targetPreset->rotation.y);
    CRotM[cameraIndex].z = ApproachAngleBySpeed(transitionSpeed, startPreset->rotation.z, targetPreset->rotation.z);
    CenterM[cameraIndex].x = startPreset->center.x + transitionSpeed * (targetPreset->center.x - startPreset->center.x);
    CenterM[cameraIndex].y = startPreset->center.y + transitionSpeed * (targetPreset->center.y - startPreset->center.y);
    CenterM[cameraIndex].z = startPreset->center.z + transitionSpeed * (targetPreset->center.z - startPreset->center.z);
    CZoomM[cameraIndex] = startPreset->zoom + transitionSpeed * (targetPreset->zoom - startPreset->zoom);
}

float overlayVertexPositions[] = { 95.0f, 95.0f, 135.0f, 95.0f, 95.0f, 125.0f, 135.0f, 125.0f };

void SetSceneCamera(s32 presetIndex)
{
    float floatValue3;
    PlayerModelData *playerModel;
    s32 activePresetIndex;

    if (cameraPresetIndex != presetIndex) {
        CRotM->x = playerCameraPresets[presetIndex].rotation.x;
        CRotM->y = playerCameraPresets[presetIndex].rotation.y;
        CRotM->z = playerCameraPresets[presetIndex].rotation.z;
        CenterM->x = playerCameraPresets[presetIndex].center.x;
        CenterM->y = playerCameraPresets[presetIndex].center.y;
        CenterM->z = playerCameraPresets[presetIndex].center.z;
        CZoomM[0] = playerCameraPresets[presetIndex].zoom;
        sceneFrameCounter = 0;
        if (presetIndex != 0) {
            sceneSubstate = frand();
        }
        else {
            sceneSubstate = 0;
        }
        cameraPresetIndex = presetIndex;
    }
    activePresetIndex = cameraPresetIndex;
    switch (activePresetIndex) {
        case 0:
        case 1:
        case 2:
            sceneFrameCounter++;
            if (sceneSubstate & 1) {
                floatValue3 = cosd((float)sceneFrameCounter);
            }
            else {
                floatValue3 = -cosd((float)sceneFrameCounter);
            }
            CRotM->y = 23.0f * floatValue3 + playerCameraPresets[presetIndex].rotation.y;
            break;
        case 3:
        case 4:
        case 5:
            sceneFrameCounter++;
            if (sceneSubstate & 1) {
                CRotM->y = playerCameraPresets[presetIndex].rotation.y - 45.0f + 0.5f * sceneFrameCounter;
            }
            else {
                CRotM->y = playerCameraPresets[presetIndex].rotation.y + 45.0f - 0.5f * sceneFrameCounter;
            }
            break;
        case 6:
        case 7:
        case 8:
            if (playerModelObjects[activePresetIndex - 6] != NULL) {
                playerModel = playerModelObjects[activePresetIndex - 6]->data;
                CenterM->y = overlayVertexPositions[playerModel->objectState];
            }
            break;
        case 9:
            if (effectControllerObject != NULL) {
                playerModel = effectControllerObject->data;
                CenterM->x = effectControllerObject->trans.x;
                CenterM->z = effectControllerObject->trans.z;
                CenterM->y = overlayVertexPositions[playerModel->objectState];
            }
            break;
    }
}

void UpdateCameraMatrices(void)
{
    Mtx44 transformMatrix4;
    Mtx positionVector9;
    Vec scratchData4;
    Vec cameraLookAtTarget;
    Vec positionVector3;
    GXColor cameraColor;
    HU3DCAMERA *resourceData;

    resourceData = &Hu3DCamera[0];
    if (resourceData->fov <= 0.0f) {
        return;
    }
    C_MTXPerspective(transformMatrix4, resourceData->fov, resourceData->aspect, resourceData->near, resourceData->far);
    GXSetProjection(transformMatrix4, GX_PERSPECTIVE);
    GXSetViewport(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    GXSetScissor(0, 0, 640, 480);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    cameraColor.r = cameraColor.g = cameraColor.b = 0;
    cameraColor.a = 0xFF;
    GXSetTevColor(GX_TEVREG0, cameraColor);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetCullMode(GX_CULL_NONE);
    GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
    scratchData4.x = scratchData4.y = scratchData4.z = 0.0f;
    cameraLookAtTarget.x = cameraLookAtTarget.y = 0.0f;
    cameraLookAtTarget.z = -100.0f;
    positionVector3.x = positionVector3.z = 0.0f;
    positionVector3.y = 1.0f;
    C_MTXLookAt(positionVector9, &scratchData4, &positionVector3, &cameraLookAtTarget);
    GXLoadPosMtxImm(positionVector9, GX_PNMTX0);
    scratchData4.x = 10000.0 * -sin(resourceData->fov / 2.0f) * 1.2f;
    scratchData4.y = 10000.0 * -sin(resourceData->fov / 2.0f) * 0.8333333f;
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-scratchData4.x, -scratchData4.y, -10000.0f);
    GXPosition3f32(scratchData4.x, -scratchData4.y, -10000.0f);
    GXPosition3f32(scratchData4.x, scratchData4.y, -10000.0f);
    GXPosition3f32(-scratchData4.x, scratchData4.y, -10000.0f);
    GXEnd();
}

void InitializeTextureObject(CameraHookData *textureData, void *imageData, s16 textureWidth, GXTexWrapMode wrapMode, s32 textureFormat, s32 textureFlags)
{
    GXTexObj positionVector6;
    CameraHookData *resourceData;

    resourceData = textureData;
    GXInitTexObj(&positionVector6, imageData, resourceData->textureWidth, resourceData->textureHeight, GX_TF_RGB5A3, wrapMode, wrapMode, GX_FALSE);
    if (textureFlags != 0) {
        GXInitTexObjLOD(&positionVector6, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    }
    else {
        GXInitTexObjLOD(&positionVector6, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    }
    GXLoadTexObj(&positionVector6, textureWidth);
}

s16 CreateModelAnimationHook(ANIMDATA *animation, s16 animationIndex)
{
    s16 model;
    s16 i;
    HU3DMODEL *position2;
    ModelAnimationHookData *resourceData;
    ModelAnimationState *resourceData3;
    Vec *position3;
    float(*index)[2];
    void *position4;
    void *index2;

    model = Hu3DHookFuncCreate(UpdateModelAnimationCallback);
    position2 = &Hu3DData[model];
    position2->hookData = resourceData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(*resourceData), position2->mallocNo);
    resourceData->animationData = animation;
    resourceData->animationFrame = animationIndex;
    resourceData->objectFlags = 0;
    resourceData->animationFlags = 0;
    resourceData->objectFlags = NULL;
    resourceData->modelId = NULL;
    resourceData->objectTimer = 0;
    resourceData->animationState = 0;
    resourceData->activeState = 0;
    resourceData->actionTimer = 0;
    resourceData->objectIndex = resourceData->objectState = 0;
    resourceData->animationWork = resourceData3 = HuMemDirectMallocNum(HEAP_MODEL, animationIndex * sizeof(*resourceData3), position2->mallocNo);
    memset(resourceData3, 0, animationIndex * sizeof(*resourceData3));
    for (i = 0; i < animationIndex; i++, resourceData3++) {
        resourceData3->actionTimer = 0.0f;
        resourceData3->effectState = 0.0f;
        resourceData3->objectTimer = 0.0f;
        resourceData3->actionState = 1.0f;
        resourceData3->animationData.x = (frand7() - 64) * 20;
        resourceData3->animationData.y = (frand7() - 64) * 30;
        resourceData3->animationData.z = (frand7() - 64) * 20;
        resourceData3->objectColor.r = resourceData3->objectColor.g = resourceData3->objectColor.b = resourceData3->objectColor.a = 0xFF;
        resourceData3->objectState = 0;
    }
    resourceData->objectData = position3 = HuMemDirectMallocNum(HEAP_MODEL, animationIndex * sizeof(*position3) * 4, position2->mallocNo);
    for (i = 0; i < animationIndex * 4; i++, position3++) {
        position3->x = position3->y = position3->z = 0.0f;
    }
    resourceData->objectColor = index = HuMemDirectMallocNum(HEAP_MODEL, animationIndex * sizeof(*index) * 4, position2->mallocNo);
    for (i = 0; i < animationIndex; i++) {
        (*index)[0] = 0.0f;
        (*index)[1] = 0.0f;
        index++;
        (*index)[0] = 1.0f;
        (*index)[1] = 0.0f;
        index++;
        (*index)[0] = 1.0f;
        (*index)[1] = 1.0f;
        index++;
        (*index)[0] = 0.0f;
        (*index)[1] = 1.0f;
        index++;
    }
    index2 = position4 = HuMemDirectMallocNum(HEAP_MODEL, 0x10000, position2->mallocNo);
    DCFlushRange(position4, 0x10000);
    GXBeginDisplayList(index2, 0x10000);
    GXBegin(GX_QUADS, GX_VTXFMT0, animationIndex * 4);
    for (i = 0; i < animationIndex; i++) {
        GXPosition1x16(i * 4);
        GXColor1x16(i);
        GXTexCoord1x16(i * 4);

        GXPosition1x16(i * 4 + 1);
        GXColor1x16(i);
        GXTexCoord1x16(i * 4 + 1);

        GXPosition1x16(i * 4 + 2);
        GXColor1x16(i);
        GXTexCoord1x16(i * 4 + 2);

        GXPosition1x16(i * 4 + 3);
        GXColor1x16(i);
        GXTexCoord1x16(i * 4 + 3);
    }
    GXEnd();
    resourceData->effectState = GXEndDisplayList();
    resourceData->objectState = HuMemDirectMallocNum(HEAP_MODEL, resourceData->effectState, position2->mallocNo);
    memcpy(resourceData->objectState, position4, resourceData->effectState);
    DCFlushRange(resourceData->objectState, resourceData->effectState);
    HuMemDirectFree(position4);
    return model;
}

s16 UpdateModelAnimationHook(s16 object)
{
    s32 position4;
    s16 index2;
    s16 i;
    HU3DMODEL *model;
    HU3DMODEL *position2;
    ModelAnimationHookData *resourceData2;
    ModelAnimationHookData *resourceData;
    ModelAnimationState *position;
    Vec *position3;
    float(*resourceData3)[2];

    model = &Hu3DData[object];
    resourceData2 = model->hookData;
    index2 = Hu3DHookFuncCreate(model->hookFunc);
    position2 = &Hu3DData[index2];
    position2->hookData = resourceData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(*resourceData), position2->mallocNo);
    resourceData->animationData = resourceData2->animationData;
    resourceData->animationFrame = resourceData2->animationFrame;
    position4 = resourceData->animationFrame;
    resourceData->objectFlags = 0;
    resourceData->animationFlags = resourceData2->animationFlags;
    resourceData->objectFlags = NULL;
    resourceData->modelId = NULL;
    resourceData->objectTimer = 0;
    resourceData->animationState = 0;
    resourceData->actionTimer = 0;
    resourceData->objectIndex = resourceData->objectState = 0;
    resourceData->animationWork = position = HuMemDirectMallocNum(HEAP_MODEL, position4 * sizeof(*position), position2->mallocNo);
    for (i = 0; i < position4; i++, position++) {
        position->actionTimer = 0.0f;
        position->effectState = 0.0f;
        position->objectTimer = 0.0f;
        position->actionState = 1.0f;
        position->animationData.x = (frand7() - 64) * 20;
        position->animationData.y = (frand7() - 64) * 30;
        position->animationData.z = (frand7() - 64) * 20;
        position->objectColor.r = position->objectColor.g = position->objectColor.b = position->objectColor.a = 0xFF;
        position->objectState = 0;
    }
    resourceData->objectData = position3 = HuMemDirectMallocNum(HEAP_MODEL, position4 * sizeof(*position3) * 4, position2->mallocNo);
    for (i = 0; i < (position4 * 4); i++, position3++) {
        position3->x = position3->y = position3->z = 0.0f;
    }
    resourceData->objectColor = resourceData3 = HuMemDirectMallocNum(HEAP_MODEL, position4 * sizeof(*resourceData3) * 4, position2->mallocNo);
    for (i = 0; i < position4; i++) {
        (*resourceData3)[0] = 0.0f;
        (*resourceData3)[1] = 0.0f;
        resourceData3++;
        (*resourceData3)[0] = 1.0f;
        (*resourceData3)[1] = 0.0f;
        resourceData3++;
        (*resourceData3)[0] = 1.0f;
        (*resourceData3)[1] = 1.0f;
        resourceData3++;
        (*resourceData3)[0] = 0.0f;
        (*resourceData3)[1] = 1.0f;
        resourceData3++;
    }
    resourceData->effectState = resourceData2->effectState;
    resourceData->objectState = resourceData2->objectState;
    return index2;
}

Vec overlayQuadVertices[] = { { -0.5f, 0.5f, 0.0f }, { 0.5f, 0.5f, 0.0f }, { 0.5f, -0.5f, 0.0f }, { -0.5f, -0.5f, 0.0f } };

float overlayTextureCoordinates[][2] = { { 0.0f, 0.0f }, { 0.25f, 0.0f }, { 0.25f, 0.25f }, { 0.0f, 0.25f } };

void UpdateModelAnimationCallback(HU3DMODEL *model, Mtx transform)
{
    Mtx transformMatrix2;
    Mtx rotationMatrix;
    Mtx animationMatrix;
    Vec scratchData5[4];
    Vec positionVector12[4];
    Vec scratchData6[4];
    ROMtx positionVector2;
    ModelAnimationHookData *resourceData;
    ModelAnimationState *resourceData3;
    Vec *position;
    float(*index)[2];
    Vec *position2;
    Vec *position4;
    s16 stateValue;
    s16 stateValue2;
    s16 stateValue3;
    void (*resourceData4)(HU3DMODEL *, struct StructM42305 *, Mtx);
    s16 i;
    s16 j;

    resourceData = model->hookData;
    if (resourceData->actionState == GlobalCounter && shadowModelDrawF == 0) {
        return;
    }
    if (resourceData->modelId && resourceData->modelId != model) {
        UpdateModelAnimationCallback(resourceData->modelId, matrix);
    }
    GXLoadPosMtxImm(transform, GX_PNMTX0);
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    if (shadowModelDrawF != 0) {
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ONE, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
        GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    }
    else {
        stateValue = resourceData->animationData->bmp->dataFmt & 0xF;
        if (stateValue == 7 || stateValue == 8) {
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
        }
        else {
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
        }
        if (resourceData->animationFlags & 1) {
            GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
        }
        else if (model->attr & 2) {
            GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        }
        else {
            GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
        }
    }
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_CLAMP, GX_AF_NONE);
    HuSprTexLoad(resourceData->animationData, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZCompLoc(GX_FALSE);
    switch (resourceData->objectFlags) {
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
    if (HmfInverseMtxF3X3(transform, transformMatrix2) == 0) {
        MTXIdentity(transformMatrix2);
    }
    PSMTXReorder(transformMatrix2, positionVector2);
    if (resourceData->objectFlags) {
        resourceData4 = resourceData->objectFlags;
        resourceData4(model, resourceData, transform);
    }
    resourceData3 = resourceData->animationWork;
    position = resourceData->objectData;
    index = resourceData->objectColor;
    PSMTXROMultVecArray(positionVector2, overlayQuadVertices, scratchData6, 4);
    for (i = 0; i < resourceData->animationFrame; i++, resourceData3++) {
        if (!resourceData3->actionTimer) {
            position->x = position->y = position->z = 0.0f;
            position++;
            position->x = position->y = position->z = 0.0f;
            position++;
            position->x = position->y = position->z = 0.0f;
            position++;
            position->x = position->y = position->z = 0.0f;
            position++;
        }
        else if (!resourceData3->effectState) {
            position2 = scratchData5;
            position4 = scratchData6;
            VECScale(position4++, position2, resourceData3->actionTimer);
            VECAdd(position2++, &resourceData3->animationData, position++);
            VECScale(position4++, position2, resourceData3->actionTimer);
            VECAdd(position2++, &resourceData3->animationData, position++);
            VECScale(position4++, position2, resourceData3->actionTimer);
            VECAdd(position2++, &resourceData3->animationData, position++);
            VECScale(position4++, position2, resourceData3->actionTimer);
            VECAdd(position2++, &resourceData3->animationData, position++);
        }
        else {
            VECScale(&overlayQuadVertices[0], &scratchData5[0], resourceData3->actionTimer);
            VECScale(&overlayQuadVertices[1], &scratchData5[1], resourceData3->actionTimer);
            VECScale(&overlayQuadVertices[2], &scratchData5[2], resourceData3->actionTimer);
            VECScale(&overlayQuadVertices[3], &scratchData5[3], resourceData3->actionTimer);
            MTXRotRad(animationMatrix, 'Z', resourceData3->effectState);
            MTXConcat(transformMatrix2, animationMatrix, rotationMatrix);
            MTXMultVecArray(rotationMatrix, scratchData5, positionVector12, 4);
            VECAdd(&positionVector12[0], &resourceData3->animationData, position++);
            VECAdd(&positionVector12[1], &resourceData3->animationData, position++);
            VECAdd(&positionVector12[2], &resourceData3->animationData, position++);
            VECAdd(&positionVector12[3], &resourceData3->animationData, position++);
        }
    }
    resourceData3 = resourceData->animationWork;
    index = resourceData->objectColor;
    if (!(resourceData->animationFlags & 2)) {
        for (i = 0; i < resourceData->animationFrame; i++, resourceData3++) {
            stateValue2 = resourceData3->objectState & 3;
            stateValue3 = (resourceData3->objectState >> 2) & 3;
            for (j = 0; j < 4; j++, index++) {
                (*index)[0] = 0.25f * stateValue2 + overlayTextureCoordinates[j][0];
                (*index)[1] = 0.25f * stateValue3 + overlayTextureCoordinates[j][1];
            }
        }
    }
    else {
        for (i = 0; i < resourceData->animationFrame; i++, resourceData3++) {
            for (j = 0; j < 4; j++, index++) {
                (*index)[0] = 4.0f * overlayTextureCoordinates[j][0];
                (*index)[1] = 4.0f * overlayTextureCoordinates[j][1];
            }
        }
    }
    DCFlushRangeNoSync(resourceData->objectData, resourceData->animationFrame * sizeof(*resourceData->objectData) * 4);
    DCFlushRangeNoSync(resourceData->objectColor, resourceData->animationFrame * sizeof(*resourceData->objectColor) * 4);
    DCFlushRangeNoSync(resourceData->animationWork, resourceData->animationFrame * sizeof(*resourceData->animationWork));
    PPCSync();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, resourceData->objectData, 12);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, &resourceData->animationWork->objectColor, 76);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_TEX0, resourceData->objectColor, 8);
    GXCallDisplayList(resourceData->objectState, resourceData->effectState);
    if (shadowModelDrawF == 0) {
        if (!(resourceData->animationState & 2)) {
            resourceData->objectTimer++;
        }
        if (resourceData->actionTimer != 0 && resourceData->actionTimer <= resourceData->objectTimer) {
            if (resourceData->animationState & 1) {
                resourceData->objectTimer = 0;
            }
            resourceData->objectTimer = resourceData->actionTimer;
        }
        resourceData->actionState = GlobalCounter;
    }
}

float GetShortestAngleDifference(float angle, float targetAngle)
{
    float floatValue3;

    if (angle >= 360.0f) {
        angle -= 360.0f;
    }
    else if (angle < 0.0f) {
        angle += 360.0f;
    }
    if (targetAngle >= 360.0f) {
        targetAngle -= 360.0f;
    }
    else if (targetAngle < 0.0f) {
        targetAngle += 360.0f;
    }
    floatValue3 = angle - targetAngle;
    if (floatValue3 <= -180.0f) {
        floatValue3 += 360.0f;
    }
    else if (floatValue3 >= 180.0f) {
        floatValue3 -= 360.0f;
    }
    return floatValue3;
}

float ApproachAngle(float currentAngle, float targetAngle, float maxStep)
{
    float floatValue;
    float floatValue3;

    if (currentAngle >= 360.0) {
        currentAngle -= 360.0;
    }
    else if (currentAngle < 0.0) {
        currentAngle += 360.0;
    }
    if (targetAngle >= 360.0) {
        targetAngle -= 360.0;
    }
    else if (targetAngle < 0.0) {
        targetAngle += 360.0;
    }
    floatValue = currentAngle - targetAngle + 360.0;
    if (fabs(floatValue) >= 360.0) {
        floatValue = fmod(floatValue, 360.0);
    }
    if (floatValue < 180.0) {
        if (floatValue <= maxStep) {
            floatValue3 = floatValue;
        }
        else {
            floatValue3 = maxStep;
        }
    }
    else {
        if (360.0 - floatValue <= maxStep) {
            floatValue3 = -(360.0 - floatValue);
        }
        else {
            floatValue3 = -maxStep;
        }
    }
    floatValue3 += targetAngle;
    if (floatValue3 >= 360.0) {
        floatValue3 -= 360.0;
    }
    else if (floatValue3 < 0.0) {
        floatValue3 += 360.0;
    }
    return floatValue3;
}

float ApproachAngleBySpeed(float angularSpeed, float currentAngle, float targetAngle)
{
    float floatValue7;

    floatValue7 = GetShortestAngleDifference(targetAngle, targetAngle);
    return ApproachAngle(speed, targetAngle, fabs(floatValue7 * angularSpeed));
}

void TransformVectorByMatrix(Mtx matrix, Vec *vector)
{
    Vec positionVector2;

    positionVector2.x = matrix[0][0] * vector->x + matrix[0][1] * vector->y + matrix[0][2] * vector->z;
    positionVector2.y = matrix[1][0] * vector->x + matrix[1][1] * vector->y + matrix[1][2] * vector->z;
    positionVector2.z = matrix[2][0] * vector->x + matrix[2][1] * vector->y + matrix[2][2] * vector->z;
    *vector = positionVector2;
}

void CalculatePlaneFromPoints(Vec *pointA, Vec *pointB, Vec *pointC, float *plane)
{
    Vec positionVector6;
    Vec edgeVector;
    float floatValue;
    float floatValue5;

    positionVector6.x = pointB->x - pointA->x;
    positionVector6.y = pointB->y - pointA->y;
    positionVector6.z = pointB->z - pointA->z;
    edgeVector.x = pointC->x - pointB->x;
    edgeVector.y = pointC->y - pointB->y;
    edgeVector.z = pointC->z - pointB->z;
    plane[0] = positionVector6.y * edgeVector.z - positionVector6.z * edgeVector.y;
    plane[1] = positionVector6.z * edgeVector.x - positionVector6.x * edgeVector.z;
    plane[2] = positionVector6.x * edgeVector.y - positionVector6.y * edgeVector.x;
    floatValue5 = plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2];
    floatValue = sqrtf(floatValue5);
    if (floatValue != 0.0f) {
        floatValue = 1.0f / floatValue;
    }
    else {
        floatValue = 0.0f;
    }
    plane[0] *= floatValue;
    plane[1] *= floatValue;
    plane[2] *= floatValue;
    plane[3] = -(plane[0] * pointA->x + plane[1] * pointA->y + plane[2] * pointA->z);
}

s32 CheckSegmentPlaneIntersection(Vec *startPosition, Vec *endPosition, float *plane)
{
    float floatValue3;
    float floatValue;

    floatValue3 = startPosition->x * plane[0] + startPosition->y * plane[1] + startPosition->z * plane[2] + plane[3];
    floatValue = endPosition->x * plane[0] + endPosition->y * plane[1] + endPosition->z * plane[2] + plane[3];
    if (floatValue3 <= 0.0f && floatValue > 0.0f) {
        return 1;
    }
    if (floatValue3 >= 0.0f && floatValue < 0.0f) {
        return -1;
    }
    return 0;
}

float GetPolygonEdgeSide(Vec *polygonVertices, Vec *pointA, s32 edgeIndex, float coordinateA, float coordinateB)
{
    float floatValue3;
    float floatValue;
    float floatValue5;
    float floatValue7;
    float floatValue8;

    switch (edgeIndex) {
        case 0:
            floatValue3 = polygonVertices->y - coordinateA;
            floatValue = polygonVertices->z - coordinateB;
            floatValue5 = pointA->y - coordinateA;
            floatValue7 = pointA->z - coordinateB;
            break;
        case 1:
            floatValue3 = polygonVertices->x - coordinateA;
            floatValue = polygonVertices->z - coordinateB;
            floatValue5 = pointA->x - coordinateA;
            floatValue7 = pointA->z - coordinateB;
            break;
        case 2:
            floatValue3 = polygonVertices->x - coordinateA;
            floatValue = polygonVertices->y - coordinateB;
            floatValue5 = pointA->x - coordinateA;
            floatValue7 = pointA->y - coordinateB;
            break;
    }
    floatValue8 = -(floatValue * floatValue5 - floatValue3 * floatValue7);
    return floatValue8;
}

u8 polygonEdgeVertexIndices[][2] = { { 0, 1 }, { 1, 2 }, { 2, 0 } };

float CheckPointInsidePolygon(Vec *polygonVertices, Vec *point, s32 axis)
{
    float floatValue3;
    float floatValue;
    float floatValue5;
    s32 i;

    switch (axis) {
        case 0:
            floatValue3 = point->y;
            floatValue = point->z;
            break;
        case 1:
            floatValue3 = point->x;
            floatValue = point->z;
            break;
        case 2:
            floatValue3 = point->x;
            floatValue = point->y;
            break;
    }
    floatValue5 = GetPolygonEdgeSide(&polygonVertices[polygonEdgeVertexIndices[0][0]], &polygonVertices[polygonEdgeVertexIndices[0][1]], index, floatValue3, floatValue);
    if (floatValue5 > 0.0f) {
        for (i = 1; i < 3; i++) {
            floatValue5 = GetPolygonEdgeSide(&polygonVertices[polygonEdgeVertexIndices[i][0]], &polygonVertices[polygonEdgeVertexIndices[i][1]], index, floatValue3, floatValue);
            if (floatValue5 == 0.0f) {
                return 1.0f;
            }
            if (floatValue5 < 0.0f) {
                return 0.0f;
            }
        }
        return 1.0f;
    }
    else {
        for (i = 1; i < 3; i++) {
            floatValue5 = GetPolygonEdgeSide(&polygonVertices[polygonEdgeVertexIndices[i][0]], &polygonVertices[polygonEdgeVertexIndices[i][1]], index, floatValue3, floatValue);
            if (floatValue5 == 0.0f) {
                return -1.0f;
            }
            if (floatValue5 > 0.0f) {
                return 0.0f;
            }
        }
        return -1.0f;
    }
}

void IntersectLineWithPlane(float *plane, Vec *lineStart, Vec *lineEnd, Vec *intersection, Vec *distance)
{
    float floatValue9;
    float floatValue2;
    float floatValue10;
    float floatValue3;
    float floatValue;
    float floatValue5;
    float floatValue4;
    float floatValue11;
    float floatValue12;
    float floatValue6;
    float floatValue13;
    float intersectionZ;
    float floatValue7;

    floatValue9 = plane[0];
    floatValue2 = plane[1];
    floatValue10 = plane[2];
    floatValue6 = intersection->x;
    floatValue13 = intersection->y;
    intersectionZ = intersection->z;
    floatValue3 = lineEnd->x;
    floatValue = lineEnd->y;
    floatValue5 = lineEnd->z;
    floatValue4 = lineStart->x;
    floatValue11 = lineStart->y;
    floatValue12 = lineStart->z;
    floatValue7 = sqrtf(floatValue3 * floatValue3 + floatValue * floatValue + floatValue5 * floatValue5);
    if (floatValue7 != 0.0f) {
        floatValue7 = 1.0f / floatValue7;
    }
    floatValue3 *= floatValue7;
    floatValue *= floatValue7;
    floatValue5 *= floatValue7;
    floatValue7 = floatValue9 * floatValue3 + floatValue2 * floatValue + floatValue10 * floatValue5;
    if (floatValue7 != 0.0f) {
        floatValue7 = -(floatValue9 * (floatValue4 - floatValue6) + floatValue2 * (floatValue11 - floatValue13) + floatValue10 * (floatValue12 - intersectionZ)) / floatValue7;
    }
    else {
        floatValue7 = 0.0f;
    }
    distance->x = lineStart->x + floatValue7 * lineEnd->x;
    distance->y = lineStart->y + floatValue7 * lineEnd->y;
    distance->z = lineStart->z + floatValue7 * lineEnd->z;
}

float GetPointToSegmentDistance(Vec *pointA, Vec *pointB, Vec *point, Vec *closestPoint, f32 *projectionRatio)
{
    Vec scratchData6;
    Vec segmentVector;
    Vec positionVector4;
    Vec positionVector;
    Vec positionVector2;
    float floatValue3;

    pointA->z = 0.0f;
    pointB->z = 0.0f;
    point->z = 0.0f;
    VECSubtract(pointB, pointA, &segmentVector);
    VECSubtract(point, pointA, &positionVector4);
    floatValue3 = (segmentVector.x * positionVector4.x + segmentVector.y * positionVector4.y) / (segmentVector.x * segmentVector.x + segmentVector.y * segmentVector.y);
    if (projectionRatio != NULL) {
        *projectionRatio = floatValue3;
    }
    VECScale(&segmentVector, &positionVector, floatValue3);
    VECAdd(&segmentVector, &positionVector, &scratchData6);
    if (closestPoint != NULL) {
        *closestPoint = scratchData6;
    }
    VECSubtract(point, &scratchData6, &positionVector2);
    return VECMag(&positionVector2);
}

s32 CheckSegmentBoxIntersection(Vec *boxCorners, Vec *startPosition, Vec *direction, Vec *collisionPoint, float collisionRadius, s32 faceIndex)
{
    Vec positionVector13;
    Vec positionVector9;
    Vec scratchData4;
    Vec normalizedDirection;
    Vec positionVector3;
    f32 floatValue;
    f32 floatValue5;
    f32 floatValue7;
    f32 floatValue3;

    VECAdd(startPosition, direction, &scratchData4);
    *collisionPoint = scratchData4;
    VECNormalize(direction, &normalizedDirection);
    VECScale(&normalizedDirection, &normalizedDirection, collisionRadius);
    VECAdd(startPosition, &normalizedDirection, &positionVector13);
    VECAdd(&scratchData4, &normalizedDirection, &positionVector9);
    VECNormalize(direction, &normalizedDirection);
    VECScale(&normalizedDirection, &positionVector3, -collisionRadius);
    switch (faceIndex) {
        case 0:
            if (positionVector13.y >= boxCorners[0].y && positionVector9.y < boxCorners[0].y) {
                if (normalizedDirection.y == 0.0f) {
                    floatValue3 = 0.0f;
                }
                else {
                    floatValue3 = (boxCorners[0].y - startPosition->y) / normalizedDirection.y;
                }
                floatValue = startPosition->x + floatValue3 * normalizedDirection.x;
                floatValue7 = startPosition->z + floatValue3 * normalizedDirection.z;
                if (floatValue >= boxCorners[0].x && floatValue <= boxCorners[2].x && floatValue7 >= boxCorners[0].z && floatValue7 <= boxCorners[2].z) {
                    collisionPoint->x = floatValue;
                    collisionPoint->y = boxCorners[0].y + collisionRadius + 1.0f;
                    collisionPoint->z = floatValue7;
                    VECAdd((Vec *)&collisionPoint, &positionVector3, (Vec *)&collisionPoint);
                    return 1;
                }
            }
            break;
        case 1:
            if (positionVector13.y <= boxCorners[0].y && positionVector9.y > boxCorners[0].y) {
                if (normalizedDirection.y == 0.0f) {
                    floatValue3 = 0.0f;
                }
                else {
                    floatValue3 = (boxCorners[0].y - startPosition->y) / normalizedDirection.y;
                }
                floatValue = startPosition->x + floatValue3 * normalizedDirection.x;
                floatValue7 = startPosition->z + floatValue3 * normalizedDirection.z;
                if (floatValue >= boxCorners[0].x && floatValue <= boxCorners[2].x && floatValue7 <= boxCorners[0].z && floatValue7 >= boxCorners[2].z) {
                    collisionPoint->x = floatValue;
                    collisionPoint->y = boxCorners[0].y - collisionRadius - 1.0f;
                    collisionPoint->z = floatValue7;
                    VECAdd((Vec *)&collisionPoint, &positionVector3, (Vec *)&collisionPoint);
                    return 1;
                }
            }
            break;
        case 2:
            if (positionVector13.x <= boxCorners[0].x && positionVector9.x > boxCorners[0].x) {
                if (normalizedDirection.x == 0.0f) {
                    floatValue3 = 0.0f;
                }
                else {
                    floatValue3 = (boxCorners[0].x - startPosition->x) / normalizedDirection.x;
                }
                floatValue5 = startPosition->y + floatValue3 * normalizedDirection.y;
                floatValue7 = startPosition->z + floatValue3 * normalizedDirection.z;
                if (floatValue5 <= boxCorners[0].y && floatValue5 >= boxCorners[2].y && floatValue7 >= boxCorners[0].z && floatValue7 <= boxCorners[2].z) {
                    collisionPoint->x = boxCorners[0].x - collisionRadius - 1.0f;
                    collisionPoint->y = floatValue5;
                    collisionPoint->z = floatValue7;
                    VECAdd((Vec *)&collisionPoint, &positionVector3, (Vec *)&collisionPoint);
                    return 1;
                }
            }
            break;
        case 3:
            if (positionVector13.x >= boxCorners[0].x && positionVector9.x < boxCorners[0].x) {
                if (normalizedDirection.x == 0.0f) {
                    floatValue3 = 0.0f;
                }
                else {
                    floatValue3 = (boxCorners[0].x - startPosition->x) / normalizedDirection.x;
                }
                floatValue5 = startPosition->y + floatValue3 * normalizedDirection.y;
                floatValue7 = startPosition->z + floatValue3 * normalizedDirection.z;
                if (floatValue5 <= boxCorners[0].y && floatValue5 >= boxCorners[2].y && floatValue7 <= boxCorners[0].z && floatValue7 >= boxCorners[2].z) {
                    collisionPoint->x = boxCorners[0].x + collisionRadius + 1.0f;
                    collisionPoint->y = floatValue5;
                    collisionPoint->z = floatValue7;
                    VECAdd((Vec *)&collisionPoint, &positionVector3, (Vec *)&collisionPoint);
                    return 1;
                }
            }
            break;
        case 4:
            if (positionVector13.z >= boxCorners[0].z && positionVector9.z < boxCorners[0].z) {
                if (normalizedDirection.z == 0.0f) {
                    floatValue3 = 0.0f;
                }
                else {
                    floatValue3 = (boxCorners[0].z - startPosition->z) / normalizedDirection.z;
                }
                floatValue = startPosition->x + floatValue3 * normalizedDirection.x;
                floatValue5 = startPosition->y + floatValue3 * normalizedDirection.y;
                if (floatValue >= boxCorners[0].x && floatValue <= boxCorners[2].x && floatValue5 <= boxCorners[0].y && floatValue5 >= boxCorners[2].y) {
                    collisionPoint->x = floatValue;
                    collisionPoint->y = floatValue5;
                    collisionPoint->z = boxCorners[0].z + collisionRadius + 1.0f;
                    VECAdd((Vec *)&collisionPoint, &positionVector3, (Vec *)&collisionPoint);
                    return 1;
                }
            }
            break;
        case 5:
            if (positionVector13.z <= boxCorners[0].z && positionVector9.z > boxCorners[0].z) {
                if (normalizedDirection.z == 0.0f) {
                    floatValue3 = 0.0f;
                }
                else {
                    floatValue3 = (boxCorners[0].z - startPosition->z) / normalizedDirection.z;
                }
                floatValue = startPosition->x + floatValue3 * normalizedDirection.x;
                floatValue5 = startPosition->y + floatValue3 * normalizedDirection.y;
                if (floatValue <= boxCorners[0].x && floatValue >= boxCorners[2].x && floatValue5 <= boxCorners[0].y && floatValue5 >= boxCorners[2].y) {
                    collisionPoint->x = floatValue;
                    collisionPoint->y = floatValue5;
                    collisionPoint->z = boxCorners[0].z - collisionRadius - 1.0f;
                    VECAdd((Vec *)&collisionPoint, &positionVector3, (Vec *)&collisionPoint);
                    return 1;
                }
            }
            break;
    }
    return 0;
}

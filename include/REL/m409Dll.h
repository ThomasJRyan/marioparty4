#ifndef REL_M409DLL_H
#define REL_M409DLL_H

#include "dolphin.h"

#include "game/animdata.h"
#include "game/hu3d.h"
#include "game/object.h"

extern s32 rand8(void);

// types
typedef void (*SceneModelDrawCallback)(HU3DMODEL *, struct SceneModelHookWork *, Mtx);
typedef void (*CursorEffectDrawCallback)(struct CursorEffectWork *);

typedef struct CameraPreset {
    float cameraZoom;
    Vec cameraCenter;
    Vec cameraRotation;
} CameraPreset;

typedef struct LightColorSettings {
    GXColor lightColor;
    Vec lightDirection;
} LightColorSettings; // sizeof 0x10

typedef struct TextureScrollState {
    s8 sequenceBusy;
    s8 textureEventState;
    s16 reservedState;
    struct SceneModelWork *activeModelSequence;
    s16 textureEventTimer;
    s16 textureEventDuration;
    s16 cloudScrollLayer1;
    s16 cloudScrollLayer2;
    s16 cloudScrollLayer3;
    s16 backgroundScrollLayer;
    float backgroundScrollOffset;
} TextureScrollState;

typedef struct SceneModelWork {
    union {
        u16 sequenceFlags;
        u8 isActive;
    };
    s16 primaryModelId;
    s16 secondaryModelId;
    s16 movingModelId;
    s16 forwardMotionId;
    s16 reverseMotionId;
    s16 sequenceIndex;
    s16 sequenceDelay;
    float transitionProgress;
    s16 effectSpawnTimer;
    s16 effectSpawnCount;
    float transitionStartX;
    float transitionEndX;
    struct SceneModelWork *nextSequence;
} SceneModelWork; // sizeof 0x24

typedef struct SceneParticleWork {
    Vec particleVelocity;
    Vec particleAngularVelocity;
    float particleColorIntensity;
    float colorIntensityStep;
    float particleAlpha;
    float alphaFadeSpeed;
    s16 animationFrame;
    s16 animationFrameInterval;
    s16 particleEffectType;
    s16 particlePoolIndex;
    float particleRotation;
    Vec particleScale;
    Vec particlePosition;
    s16 particleLifetime;
    s8 isActive;
} SceneParticleWork; // sizeof 0x50

typedef struct SceneModelHookWork {
    s16 particleCapacity;
    s16 hookState;
    u32 displayListCapacity;
    u8 hookFlags;
    SceneModelDrawCallback drawCallback;
    ANIMDATA *particleAnimation;
    s16 modelCapacity;
    SceneParticleWork *particleWork;
    Vec *particleVertices;
    HuVec2f *particleTexcoords;
    GXColor *particleColors;
    void *displayList;
    Vec quadTopLeft;
    Vec quadTopRight;
    Vec quadBottomLeft;
    Vec quadBottomRight;
    HuVec2f *particleTexcoordFrames;
    float particleFrameWidth;
    float particleFrameHeight;
} SceneModelHookWork; // sizeof 0x68

typedef struct M409PlayerWork {
    u8 playerIndex;
    u8 characterId;
    u8 playerPairIndex;
    u8 playerSide;
    u8 controllerIndex;
    u8 isCpu;
    u8 cpuDifficulty;
    char reservedPadding7[0x3];
    s16 stickX;
    s16 stickY;
    s16 buttonsPressed;
    s16 buttonsHeld;
    char reservedPadding12[0x1];
    s8 isActive;
    s8 isSelected;
    char reservedPadding15[0x3];
    s32 playerState;
    s16 motionIndex;
    char reservedPadding1E[0x6];
    Vec homePosition;
    Vec modelRotation;
    Vec inputDirection;
    Vec previousPosition;
    u8 cursorEffectId;
    u8 statusEffectSpriteId;
    u8 characterIconSpriteId;
    u8 cursorParticleEffectId;
    u8 selectionEffectId;
    u8 statusEffectId;
    u8 scoreDigitSprites[3][2];
    u8 cursorState;
    u8 randomCursorStyle;
    u8 targetRecordIndex;
    u8 hasTarget;
    u8 statusEffectFlags;
    char reservedPadding65[0x3];
    Vec playerPosition;
} M409PlayerWork; // sizeof 0x74

typedef struct CursorEffectLayerWork {
    Vec startPosition;
    Vec position;
    Vec scale;
    char depth[0xC];
    u8 layerFlags;
    u8 layerState;
    u8 quadCount;
    u32 layerAttributes;
    s16 displayListSize;
    void *vertexArray;
    void *texcoordArray;
    void *colorArray;
    Vec *normalVector;
    GXColor primaryColor;
    GXColor secondaryColor;
    struct CursorEffectColorKey *colorKeys;
    Vec positionDelta;
    Vec scaleDelta;
    union {
        s16 animationState[0x10];
        struct {
            s16 initialAnimationState;
            float scaleX;
            float scaleY;
            float scaleZ;
        };
    };
} CursorEffectLayerWork; // sizeof 0x90

typedef struct CursorEffectWork {
    s16 effectIndex;
    s16 layerCount;
    u8 textureLayerCount;
    s16 effectFlags;
    u32 effectAttributes;
    Vec position;
    Vec rotation;
    Vec scale;
    CursorEffectDrawCallback drawCallback;
    s16 *layerModelIds;
    char reservedPadding38[0x4];
    CursorEffectLayerWork *layers;
    char reservedPadding40[0x8];
    Vec originPosition;
    Vec targetPosition;
    s16 layerState[0x20];
} CursorEffectWork; // sizeof 0xA0

typedef struct CursorEffectModelWork {
    ANIMDATA *spriteData;
    s16 modelIndex;
    s16 isActive;
    u32 textureId;
    Vec position;
    Mtx transform;
    GXColor color;
    u8 alpha;
    s16 tileWidth;
    s16 tileHeight;
    float textureScaleX;
    float textureScaleY;
    float textureOffsetX;
    float textureOffsetY;
} CursorEffectModelWork; // sizeof 0x64

typedef struct CursorEffectSystemWork {
    s16 effectCapacity;
    s16 modelCapacity;
    s16 drawHookId;
    s16 activeEffectId;
    s16 activeModelId;
    s16 activeLayerId;
    CursorEffectModelWork *effectModels;
    CursorEffectWork *effects;
} CursorEffectSystemWork; // sizeof 0x14

typedef struct PlayerTargetWork {
    u8 targetModelId;
    u8 cursorEffectId;
    u8 targetState;
    u8 targetStateFlags;
    u8 targetFlags;
    char reservedPadding5[0x3];
    float targetEffectId;
    Vec targetPosition;
    Vec targetHeading;
    Vec targetVelocity;
    Vec targetOrigin;
    Vec targetDestination;
    s8 targetPlayerIndex;
    char reservedPadding49[0x3];
    float targetRotationAngle;
} PlayerTargetWork; // sizeof 0x50

typedef struct CursorEffectQuad {
    Vec topLeft;
    Vec topRight;
    Vec bottomRight;
    Vec bottomLeft;
} CursorEffectQuad; // sizeof 0x30

typedef struct CursorEffectColorKey {
    s8 keyType;
    s8 keyFlags;
    s16 keyTime;
    HuVec2f *colorValues;
    u8 keyCount;
    s16 keyIndex;
    float keyStartTime;
    float keyEndTime;
} CursorEffectColorKey;

// bss
extern CursorEffectSystemWork cursorEffectSystem;

// prototypes
void ObjectSetup(void);
void CheckOverlayExitRequest(OMOBJ *object);
void FinishOverlayExit(OMOBJ *object);
s8 UpdateIntroCameraTransition(void);
s8 UpdateOutroCameraTransition(void);
void CreateSceneModels(OMOBJ *object);
void UpdateSceneEffects(OMOBJ *object);
s8 UpdateIntroSequence(OMOBJ *object);
s8 UpdateRoundCameraSequence(OMOBJ *object);
void UpdateFinalCameraSequence(OMOBJ *object);
void SetSequenceState(s16);
void SetSequenceStatePair(u16, u16);
u16 GetSequenceState(u16);
void CreateGameSequenceController(OMOBJ *object);
void UpdateGameSequenceController(OMOBJ *object);
void AdvanceGameSequence(void);
void UpdateSceneModelState(SceneModelWork *, s16, s16);
s32 InterpolateCameraParameter(s32, float, float, float, float, float, float);
void CopyMatrix(Mtx, Mtx);
float InterpolateCameraValue(float, float, float, float);
void ResetSceneCamera(void);
void CreateSceneEffect(s16, float, float, float, s8, s16);
void CreateSceneEffectAtPosition(s16, float, float, float, s16);
void DrawSceneModelHook(HU3DMODEL *, SceneModelHookWork *, Mtx);
void DrawSceneSpriteHook(HU3DMODEL *, SceneModelHookWork *, Mtx);
void DrawSceneParticleHook(HU3DMODEL *, SceneModelHookWork *, Mtx);
SceneParticleWork *createSceneParticle(s16, Vec *, Vec *, float, GXColor *);
s16 CreateSceneModel(ANIMDATA *, s16, float, s16, s16);
void KillSceneModel(s16);
void SetSceneModelDrawCallback(s16, SceneModelDrawCallback);
void SetSceneModelFlags(s16, u8);
void DrawSceneModelCallback(HU3DMODEL *, Mtx);
void SetScreenRectangle(HuVec2f *, s16, s16, float, float);
void InterpolateScreenRectangle(HuVec2f *, HuVec2f *, s16, float, float);
void InterpolateVector(Vec *, Vec *, Vec *, s16);
// player.c
void InitializePlayerObjects(HUPROCESS *);
void CleanupPlayerObjects(void);
void CreatePlayerObject(OMOBJ *);
void UpdatePlayerObject(OMOBJ *);
void UpdateCpuPlayerInput(OMOBJ *);
void UpdatePlayerCursorEffect(CursorEffectWork *);
s8 GetPlayerCursorState(u8);
void UpdatePlayerStatusEffect(CursorEffectWork *);
void UpdatePlayerManager(OMOBJ *);
void UpdatePlayerStatus(OMOBJ *);
s32 UpdatePlayerTarget(PlayerTargetWork *);
s8 FindNearestTarget(Vec *, float *);
s32 GetRandomPlayerIndex(void);
void ClampIntegerRange(s32 *, s32);
float SmoothInterpolationCurve(float, float, float);
float InterpolateCubicValue(float, float, float, float);
void InterpolateCubicValues(float *, float *, float *, float *, float);
float InterpolateSmoothValue(float, float, float, float);
void InterpolateSmoothValues(float *, float *, float *, float *, float);
float VectorLength(Vec *);
float DistanceBetweenSegments(Vec *, Vec *, Vec *);
float ClosestPointsBetweenSegments(Vec *, Vec *, Vec *, Vec *, Vec *);
float GetPlayerRank(s16);
// cursor.c
void InitializeCursorEffectSystem(s16, s16);
void CleanupCursorEffectSystem(void);
void DrawCursorEffects(CursorEffectWork *);
void DrawCursorEffectModel(HU3DMODEL *model, Mtx matrix);
CursorEffectWork *getCursorEffect(s16 idx);
CursorEffectLayerWork *getCursorEffectLayer(s16, s16);
void AllocateQuadVertices(CursorEffectQuad **, s16, float, float);
void AllocateVectorArray(Vec **, s16, Vec *);
void AllocateColorArray(GXColor **, s16, GXColor *);
void AllocatePositionArray(Vec **, s16, Vec *);
void AllocateScreenVectorArray(HuVec2f **, s16, HuVec2f *);
void InitializeCursorEffectLayer(CursorEffectLayerWork *);
void UpdateCursorEffect(CursorEffectWork *);
s16 CreateCursorEffect(u8, u8);
void DestroyCursorEffect(s16);
void SetCursorEffectLayer(s16, s16, u8);
s16 CreateCursorEffectTexture(u32, s16, s16);
void DestroyCursorEffectTexture(s16);
s32 CreateCursorEffectSprite(s16, u8, u32, s16, s16);
s32 CreateCursorEffectSpriteSimple(s16, u8, u32);
void SetCursorEffectPosition(s16, float, float, float);
void SetCursorEffectPositionAtLayer(s16, s16, float, float, float);
void SetCursorEffectPositionImmediate(s16, float, float, float);
void MoveCursorEffectPosition(s16, float, float, float);
void MoveCursorEffectPositionAtLayer(s16, s16, float, float, float);
void MoveCursorEffectPosition(s16, float, float, float);
void MoveCursorEffectPositionImmediate(s16, float, float, float);
void SetCursorEffectScale(s16, float, float, float);
void SetCursorEffectScaleAtLayer(s16, s16, float, float, float);
void SetCursorEffectScaleImmediate(s16, float, float, float);
void SetCursorEffectDepth(s16, float, float, float);
void SetCursorEffectDepthAtLayer(s16, s16, float, float, float);
void SetCursorEffectDepthImmediate(s16, float, float, float);
void SetCursorEffectFlags(s16, s32);
void SetCursorEffectLayerFlags(s16, s16, s32);
void SetCursorEffectAllFlags(s16, s32);
void SetCursorEffectAttribute(s16, u32);
void SetCursorEffectLayerAttribute(s16, s16, u32);
void SetCursorEffectAllAttributes(s16, u32);
s32 CheckCursorEffectFlags(s16, s32);
s32 CheckCursorEffectLayerFlags(s16, s16, s32);
void SetCursorEffectColorAtLayer(s16, s16, u8, u8, u8, u8);
void SetCursorEffectColor(s16, u8, u8, u8, u8);
void SetCursorEffectLayerAlpha(s16, s16, u8);
void SetCursorEffectSubLayerAlpha(s16, s16, u8, u8);
void SetCursorEffectAllAlpha(s16, u8);
void SetCursorEffectLayerOpacity(s16, s16, float);
void SetCursorEffectSubLayerOpacity(s16, s16, u8, float);
void SetCursorEffectAllOpacity(s16, float);
void SetCursorEffectLayerPriority(s16, s16, u8);
void SetCursorEffectSubLayerPriority(s16, s16, u8, u8);
void SetCursorEffectAllPriority(s16, u8);

#endif

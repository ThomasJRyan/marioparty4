#ifndef REL_M440DLL_H
#define REL_M440DLL_H

#include "dolphin/types.h"
#include "game/hu3d.h"

#include "version.h"

#include "game/object.h"
extern s32 rand8(void);

////// TYPES //////
typedef void (*SpriteUpdateCallback)(HU3DMODEL *, struct _SpriteModelWork *, Mtx);
typedef void (*MeshDrawCallback)(struct _MeshWork *);
typedef void (*ModelDrawCallback)(struct _ModelDrawWork *);

typedef struct _CameraPreset {
    float zoom;
    Vec center;
    Vec rot;
} CameraPreset;

typedef struct _EffectWork {
    s32 stateFlags;
    s16 ownerPlayer;
    s16 effectType;
    s16 modelIndex;
    s16 effectTimer;
    s32 effectHandle;
    char reserved10[0xC];
    s16 modelCount;
    char reserved20[0x4];
    float modelHeight;
    char reserved28[0x4];
    s16 selectedModelIndex;
    s16 sceneModelIndex;
    float motionProgress;
} EffectWork;

typedef struct _PlayerSequenceWork {
    char reserved00[0x34];
    float rotationStart;
    char reserved38[0x8];
    float rotationTarget;
} PlayerSequenceWork;

typedef struct _ParticleWork {
    Vec velocity;
    char reserved0C[0xC];
    float horizontalDrag;
    float verticalAcceleration;
    char reserved20[0x4];
    float scale;
    float scaleStep;
    float alpha;
    float alphaStep;
    s16 animationFrame;
    s16 animationLength;
    char reserved38[0x4];
    s32 reserved3C;
    s16 reserved40;
    s16 reserved42;
    float reserved44;
    Vec renderScale;
    Vec position;
    s16 textureFrame;
    s8 isActive;
} ParticleWork; // sizeof 0x64

typedef struct _SpriteModelWork {
    s16 particleCount;
    s16 flags;
    u32 textureId;
    u8 isActive;
    SpriteUpdateCallback updateCallback;
    ANIMDATA *animation;
    s16 textureFrameCount;
    ParticleWork *particleData;
    Vec *particlePositions;
    HuVec2f *textureCoordinates;
    GXColor *particleColors;
    void *textureData;
    Vec topLeft;
    Vec topRight;
    Vec bottomLeft;
    Vec bottomRight;
    HuVec2f *textureGrid;
    float textureWidth;
    float textureHeight;
} SpriteModelWork;

typedef struct _MeshWork {
    s16 modelId;
    u16 flags;
    s16 meshIndex;
    s16 reserved06;
    u32 vertexCount;
    Vec *vertexPositions;
    Vec *vertexNormals;
    Vec *projectedVertices;
    Vec *textureCoordinates;
    HuVec2f *meshVertices;
    s32 isVisible;
    struct _TriangleMeshWork *faceData;
    HSFMATERIAL *material;
    HSFATTRIBUTE *attribute;
    void *meshData;
    u32 meshFlags;
    MeshDrawCallback drawCallback;
    s16 drawLayer;
    s16 updateTimer;
} MeshWork; // sizeof 0x40

typedef struct _LightParameters {
    GXColor color;
    Vec direction;
} LightParameters; // sizeof 0x10

typedef struct _TriangleIndices {
    s16 positionIndex;
    s16 normalIndex;
    s16 textureCoordinateIndex;
    s16 textureIndex;
} TriangleIndices;

typedef struct _TriangleMeshWork {
    TriangleIndices vertexIndices[3];
    s16 sourceVertexIndices[3];
    s16 additionalVertexIndex;
    TriangleIndices triangleIndices[3];
    char reserved38[0x30];
    float faceProperties[5];
    Vec center;
    Vec scale;
    Vec translation;
    Vec rotation;
    Vec velocity;
    Vec angularVelocity;
    Vec acceleration;
} TriangleMeshWork; // sizeof 0xD0

typedef struct _MeshFaceWork {
    s16 faceType;
    s16 faceCount;
    TriangleIndices faceIndices[3];
    u32 faceFlags;
    TriangleIndices *sourceFaces;
    char reserved24[0xC];
} MeshFaceWork; // sizeof 0x30

typedef struct _ModelEffectWork {
    char reserved00[0xA];
    u8 effectType;
    u8 effectFlags[3];
    u8 effectParameters[3];
    char reserved12[0x2];
    float effectScale;
    char reserved18[0x4];
    float effectProgress;
    char reserved20[0x10];
    u32 effectState;
    u32 effectTimer;
    s32 *effectWork;
} ModelEffectWork;

typedef struct _SpriteEffectWork {
    char reserved00[0xA];
    u8 effectType;
    char reserved0B[0x15];
    float effectScale;
    char reserved24[0x40];
    u32 effectFlags;
    u32 effectTimer;
    char reserved6C[0x14];
    struct _SequenceWork *sequenceWork;
} SpriteEffectWork; // sizeof 0x84
typedef struct _ModelDrawWork {
    s16 flags;
    s16 modelIndex;
    char reserved04[0x8];
    void *meshData;
    char reserved10[0x4];
    void *vertexData;
    ParticleWork *particleData;
    void *reserved1C;
    char reserved20[0x4];
    GXColor *vertexColors;
    HSFMATERIAL *material;
    HSFATTRIBUTE *attribute;
    void *drawData;
    u32 drawFlags;
    ModelDrawCallback drawCallback;
    s16 vertexColorIndex;
} ModelDrawWork;

typedef struct _SequenceWork {
    s32 stateFlags;
    s16 sequenceId;
    s16 timer;
    s16 sequenceHandle;
    s16 shakeFrame;
    s32 audioHandle;
} SequenceWork;

// object.c
typedef struct _PlayerWork {
    s32 stateFlags;
    s16 playerIndex;
    s16 padIndex;
    s16 characterId;
    s16 playerSlot;
    s16 stateFrame;
    s16 stateDuration;
    s16 inputFrameCount;
    s16 turnFrameCount;
    s16 movementFrameCount;
    s16 reserved16;
    Vec position;
    Vec targetPosition;
    Vec rotation;
    Vec entryRotation;
    u8 rewardStateFlags;
    s16 selectedModelId;
    float selectionThreshold;
    s16 selectedRewardId;
    s16 selectedRewardOrder;
    s16 remainingRewardChoices;
    s16 selectionFrameCount;
    s16 buttonRepeatDelay;
    s16 buttonRepeatActive;
    float characterRadius;
    s16 rewardIndex;
    s16 reserved62;
    char reserved64[0x4];
    float rotationStartAngle;
    float rotationTargetAngle;
    float reserved70;
} PlayerWork;

////// BSS //////
extern OMOBJ *playerObjects[4];

////// DATA //////
// main.c
extern float playerOrbitAngles[5];
extern s16 rewardModelVisibility[6];
// object.c
extern s16 remainingRewardCount;

////// FUNCTIONS //////
// main.c
void ObjectSetup(void);
void CheckExitRequest(OMOBJ *object);
void FinishExitSequence(OMOBJ *object);
s32 TransitionCameraToFirstPreset(void);
s32 TransitionCameraToSecondPreset(void);
void InitializeIntroSequence(OMOBJ *object);
void UpdateIntroSequence(OMOBJ *object);
u8 UpdateIntroCamera(OMOBJ *object);
u8 UpdateCameraForPlayerSequence(OMOBJ *object);
void StartPlayerReveal(void);
void StartPlayerCelebration(void);
void StartPlayerExit(void);
s16 GetCurrentSequencePlayer(void);
s16 GetNextSequencePlayer(void);
void ResetPlayerSequenceState(void);
u8 IsPlayerSequenceComplete(void);
void StartRewardSequence(void);
u8 IsRewardSequenceReady(void);
void PositionRewardModels(void);
void HideRewardModels(void);
void UpdateParticleModel(HU3DMODEL *data, SpriteModelWork *, Mtx);
u16 GetSequenceState(u16);
void SetSequenceState(u16, u16);
void InitializeEffectObject(OMOBJ *object);
void UpdateEffectObject(OMOBJ *object);
void InitializeEffectAnimation(OMOBJ *object);
void UpdateEffectAnimation(OMOBJ *object);
void InitializeSecondaryEffect(OMOBJ *object);
void UpdateSecondaryEffect(OMOBJ *object);
void UpdateEffectParticles(OMOBJ *object);
void InitializeCameraEffect(OMOBJ *object);
void InitializeHighlightEffect(OMOBJ *object);
void UpdateHighlightEffect(OMOBJ *object);
void InitializeHighlightObject(OMOBJ *object);
void UpdateHighlightObject(OMOBJ *object);
void InitializeSceneModel(OMOBJ *object);
void UpdateSceneModel(OMOBJ *object);
void SetEffectParameter(s16, float);
s16 GetEffectParameter(void);
s16 GetEffectModelId(s16);
u16 GetEffectFlags(u16);
void SetEffectFlags(u16, u16);
void InitializeMeshVertices(MeshWork *);
void TransformMeshVertices(MeshWork *, Vec *, float);
void UpdateMeshVertexNormals(MeshWork *);
void ProcessMeshObject(s16, HSFOBJECT *, MeshWork *, u16);
void CalculateMeshFaceNormals(MeshWork *, HSFOBJECT *);
void ProcessMeshFaces(MeshWork *, HSFOBJECT *);
void CalculateVertexTransform(MeshWork *, Vec *, s16, Vec);
void CalculateTriangleProjection(MeshWork *, TriangleMeshWork *, Vec *);
void BindMaterialTexture(HSFBITMAP *, HSFATTRIBUTE *, s16);
void DrawModelMeshes(HU3DMODEL *, Mtx);
void ConfigureMaterialLighting(HSFMATERIAL *, HSFATTRIBUTE *);
void CalculateHighlightMatrix(Mtx);
void CaptureFrameTexture(void);
void CalculateTriangleNormal(Vec *, Vec *, Vec *, float[5]);
float InterpolateAngle(float, float, float);
void TransposeMatrixRotation(Mtx, Mtx);
float InterpolateValue(float, float, float);
float QuadraticInterpolate(float, float, float, float);
ParticleWork *createParticleWork(s16, Vec *, Vec *, float, GXColor *);
s16 CreateSpriteParticleModel(ANIMDATA *, s16, float, s16, s16);
void DestroyEffectModel(s16);
SpriteModelWork *getSpriteModelWork(s16);
ParticleWork *getSpriteParticleWork(s16, s16);
void SetSpriteUpdateCallback(s16, SpriteUpdateCallback);
void SetSpriteModelFlags(s16, u8);
void DrawSpriteParticles(HU3DMODEL *, Mtx);
void CalculateSpriteTextureGrid(HuVec2f *, s16, s16, float, float);
void CalculateSpriteTextureCoordinates(HuVec2f *, HuVec2f *, s16, float, float);
void ScaleVectors(Vec *, Vec *, Vec *, s16);
void UpdateDebugLight(HU3DMODEL *, Mtx);
void UpdateDebugControls(void);
// object.c
void CreatePlayerObjects(HUPROCESS *);
void InitializePlayerObject(OMOBJ *);
void IdlePlayerObject(OMOBJ *);
void InitializePlayerPositions(OMOBJ *);
void UpdatePlayerObjects(OMOBJ *);
void AnimatePlayerOrbit(OMOBJ *, PlayerWork *);
void UpdatePlayerApproach(OMOBJ *, PlayerWork *);
void UpdatePlayerReward(OMOBJ *, PlayerWork *);
s16 GetPlayerStateFlags(PlayerWork *, u8);
void SetPlayerMotionTiming(PlayerWork *, float, float);
void UpdatePlayerMotion(OMOBJ *, PlayerWork *);
void UpdatePlayerCelebration(OMOBJ *, PlayerWork *);
void UpdatePlayerExit(OMOBJ *, PlayerWork *);
u8 UpdatePlayerCoinReward(OMOBJ *, PlayerWork *);
s32 SetPlayerMotion(OMOBJ *, s16, u32);
void RestorePlayerIdleMotions(void);
void SetPlayersIdleMotions(void);
s16 GetFirstActivePlayer(void);
void StartPlayerCelebrations(void);
void ChoosePlayerRewards(void);
u16 GetPlayerState(PlayerWork *, u16);
void SetPlayerState(PlayerWork *, u16, u16);
s32 GetPlayerSlot(s32);

#endif

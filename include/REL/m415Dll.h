#ifndef REL_M415DLL_H
#define REL_M415DLL_H

#include "dolphin.h"

#include "game/hu3d.h"
#include "game/object.h"

extern u8 texMtxTbl[];

#define FABS(x) ((x < 0.0f) ? -(x) : (x))

// types
typedef struct _MapVectorQuad {
    char vectorPadding[0x10];
} MapVectorQuad;

typedef struct _MapVertexAttributeQuads {
    MapVectorQuad attributeVectors[0x10];
} MapVertexAttributeQuads;

typedef struct _MapTexcoordQuad {
    Vec textureCoordinates[4];
} MapTexcoordQuad;

typedef struct _PlayerSelection {
    s16 digitSpriteIds[3];
    s16 playerIconSpriteId;
    s16 playerMarkerSpriteId;
} PlayerSelection; // sizeof 0xC

typedef struct _PlayerCharacterFlagData {
    char colorData[0xE];
    s8 activeFlag;
} PlayerCharacterFlagData;

typedef struct _PlayerCharacterState {
    u8 playerIndex;
    u8 characterId;
    u8 inputFlags;
    u8 animationFlags;
    u8 stateFlags0;
    u8 stateFlags1;
    u8 stateFlags2;
    u8 isActive;
    char reserved8[0x2];
    u8 effectFlags0;
    u8 effectFlags1;
    u8 effectFlags2;
    u8 effectFlags3;
    u8 effectFlags4;
    u8 effectFlags5;
    s16 currentGridX;
    s16 currentGridZ;
    s16 targetGridX;
    s16 targetGridZ;
    s16 paintGridX;
    s16 paintGridZ;
    float movementSpeed;
    float turnSpeed;
    float effectScale;
    float effectTimer;
    Vec position;
    Vec rotation;
    Vec previousPosition;
    Vec shadowPosition;
    u8 characterState;
    u8 movementState;
    union {
        s32 *paintCellCounts;
        struct {
            s16 paintCellX;
            s16 paintCellZ;
        };
    };
    s16 movementFrames;
    s16 effectFrames;
    s16 collisionFrames;
    Vec collisionPosition;
} PlayerCharacterState;

typedef struct _CameraTransitionState {
    u8 interpolationMode;
    Vec lookAt;
    Vec cameraRotation;
    Vec startLookAt;
    Vec startRotation;
    Vec targetLookAt;
    Vec targetRotation;
    Vec controlLookAt;
    Vec controlRotation;
    Vec lookAtPoints[3];
    Vec rotationPoints[3];
    Vec lookAtSlopes[3];
    Vec rotationSlopes[3];
    float lookAtDerivatives[3];
    float rotationDerivatives[3];
    float zoomDerivatives[3];
    float zoomSecondDerivatives[3];
    float zoomPoints[3];
    float zoom;
    float startZoom;
    float targetZoom;
    float zoomControl;
    float frameStep;
    float duration;
    float framesRemaining;
} CameraTransitionState;

typedef struct _MapTextureFrame {
    s8 currentFrame;
    s8 previousFrame;
    char reserved2[0x2];
    HuVec2f coordinates[4];
    u8 flags;
    char reserved25[0x3];
    float frameDuration;
    float frameFraction;
} MapTextureFrame;

// taken from m438
typedef struct _MapMesh {
    Vec position;
    Vec rotation;
    Vec scale;
    char reserved24[0xC];
    u8 renderMode;
    u8 billboardMode;
    u8 materialFlags;
    u8 textureCount;
    u32 flags;
    s16 displayListSize;
    void *displayList;
    Vec *vertices;
    GXColor *colors;
    Vec *normals;
    GXColor ambientColor;
    GXColor materialColor;
    MapTextureFrame *textureFrames;
    u8 normal;
    char reserved59[0x3];
    Vec translationOffset;
    Vec rotationOffset;
    s16 textureIds[2];
    s16 *vertexIndices;
    s16 *textureIndices;
    char reserved80[0x14];
} MapMesh; /* size = 0x94 */

typedef void (*MapObjectCallback)(struct _MapObject *);

typedef struct _MapObject {
    s16 objectIndex;
    s16 meshCount;
    u8 textureSlotCount;
    u32 flags;
    Vec position;
    Vec rotation;
    Vec scale;
    void (*animationCallback)(struct _MapObject *);
    s16 *textureIds;
    s16 displayListSize;
    char reserved3A[2];
    MapMesh *meshes;
    char reserved40[0x8];
    s16 displayListUsed;
    char *displayList;
    Vec *vertexPositions;
    GXColor *vertexColors;
    Vec *vertexNormals;
    MapTextureFrame *textureFrames;
    u8 renderMode;
    char reserved61[0x3];
    Vec sortPosition;
    Vec sortRotation;
    union {
        s16 renderParameters[0x20];
        struct {
            s16 animationTimer;
            s16 animationState;
            float animationParameter0;
            float animationParameter1;
            float animationParameter2;
            float animationParameter3;
            float animationParameter4;
            char reservedAnimationData[0x28];
        };
    };
} MapObject; /* size = 0xBC */

typedef struct _MapTexture {
    ANIMDATA *animation;
    s16 frame;
    s16 referenceCount;
    u32 resourceId;
    Vec translation;
    Mtx textureMatrix;
    GXColor color;
    u8 atlasRowCount;
    u8 scaledToFit;
    s16 width;
    s16 height;
    char reserved52[0x2];
    float scaleX;
    float scaleY;
    float texelWidth;
    float texelHeight;
} MapTexture; /* size = 0x64 */

typedef struct _MapRendererState {
    Mtx cameraMatrix;
    s16 objectCapacity;
    s16 textureCapacity;
    s16 hookModelId;
    s16 activeObjectId;
    s16 activeMeshId;
    s16 activeTextureSlot;
    MapTexture *textures;
    MapObject *objects;
} MapRendererState; /* size =  */

typedef struct _MapQuadVertices {
    Vec topLeft;
    Vec topRight;
    Vec bottomRight;
    Vec bottomLeft;
} MapQuadVertices;

// bss
extern MapRendererState mapRenderer;

// prototypes
// main.c
void OpeningCameraSetup(OMOBJ *object);
void OpeningSequenceUpdate(OMOBJ *object);
void MinigameIntroUpdate(OMOBJ *object);
void MinigameExitUpdate(OMOBJ *object);
void SceneModelsSetup(OMOBJ *object);
void ShadowTextureUpdate(OMOBJ *object);
void CameraTransitionInitialize(OMOBJ *object);
void CameraTransitionUpdate(OMOBJ *object);
void CameraPoseInterpolate(float, Vec *, Vec *, float);
void PlayerCharacterInitialize(OMOBJ *object);
void PlayerCharacterIntroUpdate(OMOBJ *object);
void PlayerCharacterStateUpdate(OMOBJ *object);
void PlayerCharacterUpdate(OMOBJ *object);
void PlayerCharacterMove(OMOBJ *object);
void MapTextureAnimationUpdate(MapObject *);
void PlayerCharactersCleanup(OMOBJ *object);
void TemporaryAllocationsInitialize(void);
void *temporaryAllocate(s32);
void TemporaryAllocationsFreeAll(void);
ANIMBMP *textureAnimationBitmapGet(s16);
s16 ShadowTextureCreate(void);
s32 MinigameRandom(void);
float AngleInterpolate(float, float, float);
void RotationMatrixToEuler(Mtx, Vec *);
void SplineSecondDerivativesCalculate(float *, float *, float *, s32);
float SplineScalarEvaluate(float, float *, float *, float *, s32);
void SplineVectorSecondDerivativesCalculate(float *, Vec *, Vec *, s32);
void SplineVectorEvaluate(float, Vec *, float *, Vec *, Vec *, s32);
// map.c
void MapRendererInitialize(s16 objectCapacity, s16 textureCapacity);
void MapRendererDrawHook(HU3DMODEL *, Mtx);
ANIMDATA **mapTextureAnimationGet(s16);
void MapObjectAnimationsUpdate(MapObject *);
s16 MapObjectCreate(u8, u8);
void MapObjectDestroy(s16);
s16 MapTextureAnimationLoad(u32, s16, s16);
void MapTextureAnimationCreate(s16, s16, s16, s16, s16, s16);
void MapTextureAnimationRelease(s16);
u8 MapObjectTextureLoad(s16, u8, s32, s16, s16);
u8 MapObjectTextureRead(s16 objectId, u8 textureSlot, s32 assetId);
s32 MapObjectTextureSet(s16, u8, s16);
void MapRendererShutdown(void);
void MapObjectPositionSet(s16, float, float, float);
void MapMeshPositionSet(s16 objectId, s16 meshIndex, f32 x, f32 y, f32 z);
void MapObjectRotationSet(s16, float, float, float);
void MapObjectScaleSet(s16, float, float, float);
void MapMeshScaleSet(s16, s16, float, float, float);
void MapMeshFlagsEnable(s16 objectId, s16 meshIndex, s32 flags);
void MapMeshColorSet(s16 objectId, s16 meshIndex, u8 red, u8 green, u8 blue, u8 alpha);
void MapMeshesRenderModeSet(s16, u8);
void MapMeshBillboardModeSet(s16, s16, u8);
void MapMeshTextureFlagsIndexedSet(s16, s16, u8, u8);
void MapMeshTextureAnimationSpeedIndexedSet(s16, s16, u8, float);
void MapTextureAnimationFrameSet(s16, u8);

#endif

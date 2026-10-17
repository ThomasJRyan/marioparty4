#include "ext_math.h"
#include "game/audio.h"
#include "game/disp.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/sprite.h"

#include "REL/m406Dll.h"

#ifndef __MWERKS__
#include <game/frand.h>
#include <string.h>
#endif

typedef struct SurfaceParticleData {
    /* 0x00 */ u8 isActive;
    /* 0x01 */ u8 isFirstRow;
    /* 0x02 */ char reservedBytes[2];
    /* 0x04 */ Float edgeFade;
    /* 0x08 */ Float edgeFadeScale;
    /* 0x0C */ Float verticalOffset;
    /* 0x10 */ Vec basePosition;
    /* 0x1C */ Vec positionVelocity;
    /* 0x28 */ Float textureCoordinateU;
    /* 0x2C */ Float textureCoordinateV;
} SurfaceParticleData; /* size = 0x30 */

typedef struct PathParticleSample {
    /* 0x00 */ s32 pathSampleIndex;
    /* 0x04 */ Float pathSampleFraction;
    /* 0x08 */ s32 effectSpawnCountdown;
    /* 0x0C */ s32 effectType;
    /* 0x10 */ Float verticalVelocity;
    /* 0x14 */ Float particleScale;
    /* 0x18 */ Vec particlePosition;
    /* 0x24 */ struct MapParticleEffect *particleObject;
} PathParticleSample; /* size = 0x28 */

typedef struct SurfaceParticleModel {
    /* 0x00 */ s32 modelId;
    /* 0x04 */ s32 effectState;
    /* 0x08 */ s32 effectFrame;
    /* 0x0C */ s32 frameDelay;
    /* 0x10 */ Float scaleFactor;
    /* 0x14 */ Float rotationAngle;
    /* 0x18 */ Float rotationVelocity;
    /* 0x1C */ Float opacity;
    /* 0x20 */ Float rotationX;
    /* 0x24 */ Float rotationY;
    /* 0x28 */ Float effectWidth;
    /* 0x2C */ Float effectHeight;
    /* 0x30 */ Vec modelPosition;
    /* 0x3C */ Vec modelVelocity;
} SurfaceParticleModel; /* size = 0x48 */

typedef struct MapSurfaceRenderer {
    /* 0x00 */ s32 rendererUpdateCount;
    /* 0x04 */ s32 cameraHookModelId;
    /* 0x08 */ s32 useDefaultDrawHook;
    /* 0x0C */ Float currentPathDistance;
    /* 0x10 */ Float pathDistanceSpeed;
    /* 0x14 */ Float pathSampleSpacing;
    /* 0x18 */ Float totalPathLength;
    /* 0x1C */ Float pathDistanceOverflow;
    /* 0x20 */ s32 audioEmitterId;
    /* 0x24 */ s32 audioStartDelay;
    /* 0x28 */ s32 displayListMode;
    /* 0x2C */ PathParticleSample *particlePathSamples;
    /* 0x30 */ SurfaceParticleModel *particleModels;
    /* 0x34 */ Vec startPathPoint;
    /* 0x40 */ Vec endPathPoint;
    /* 0x4C */ Mtx pathRotationMatrix;
    /* 0x7C */ s32 currentPathSegment;
    /* 0x80 */ s32 particleVertexCount;
    /* 0x84 */ Vec *particlePositions;
    /* 0x88 */ Vec *particleNormals;
    /* 0x8C */ Vec2f *particleTextureCoordinates;
    /* 0x90 */ GXColor *particleColors;
    /* 0x94 */ SurfaceParticleData *particleSurfaceData;
    /* 0x98 */ s32 displayListIndexCount;
    /* 0x9C */ s16 (*particleTriangleIndices)[4];
    /* 0xA0 */ u32 displayListSize;
    /* 0xA4 */ void *displayListData;
    /* 0xA8 */ u8 *particleTextureData;
    /* 0xAC */ s32 primaryColorRed;
    /* 0xB0 */ s32 primaryColorGreen;
    /* 0xB4 */ Float primaryTextureScrollX;
    /* 0xB8 */ Float primaryTextureScrollY;
    /* 0xBC */ u8 *secondaryTextureData;
    /* 0xC0 */ s32 secondaryColorRed;
    /* 0xC4 */ s32 secondaryColorGreen;
    /* 0xC8 */ Float secondaryTextureScrollX;
    /* 0xCC */ Float secondaryTextureScrollY;
} MapSurfaceRenderer; /* size = 0xD0 */

typedef struct MapParticleEffect {
    /* 0x00 */ s16 frameCount;
    /* 0x02 */ s16 frameLimit;
    /* 0x04 */ Vec velocity;
    /* 0x10 */ Float scaleFactor;
    /* 0x14 */ Float verticalMotionParameter;
    /* 0x18 */ Float rotationSpeed;
    /* 0x1C */ char reservedParticleData[0xC];
    /* 0x28 */ Float effectSize;
    /* 0x2C */ Float rotationAngle;
    /* 0x30 */ Vec position;
    /* 0x3C */ GXColor color;
    /* 0x40 */ s32 textureFrameIndex;
} MapParticleEffect; /* size = 0x44 */

typedef struct ParticleEffectSettings {
    /* 0x00 */ s16 reservedState;
    /* 0x02 */ s16 effectVariant;
    /* 0x04 */ s16 animationStep;
    /* 0x06 */ char reservedBytes[2];
} ParticleEffectSettings; /* size = 0x08 */

typedef void (*ParticleEffectDrawHook)(struct Hu3DModel_s *, struct ParticleEffectHookData *, Mtx);

typedef struct ParticleEffectHookData {
    /* 0x00 */ s16 reservedHeaderWord00;
    /* 0x02 */ s16 reservedHeaderWord02;
    /* 0x04 */ char reservedHeaderData[0x18];
    /* 0x1C */ ParticleEffectSettings *effectSettings;
    /* 0x20 */ u8 hookMode;
    /* 0x21 */ u8 reservedHookByte;
    /* 0x22 */ u8 effectFlags;
    /* 0x23 */ char reservedHookFlag;
    /* 0x24 */ s16 reservedHeaderWord24;
    /* 0x26 */ s16 particleCount;
    /* 0x28 */ s32 effectActive;
    /* 0x2C */ s32 lastUpdateFrame;
    /* 0x30 */ u32 activeParticleLimit;
    /* 0x34 */ u32 displayListByteCount;
    /* 0x38 */ ANIMDATA *animationData;
    /* 0x3C */ MapParticleEffect *particles;
    /* 0x40 */ Vec *particlePositions;
    /* 0x44 */ Vec2f *particleTextureCoordinates;
    /* 0x48 */ void *displayListBuffer;
    /* 0x4C */ ParticleEffectDrawHook DrawHook;
    /* 0x50 */ HU3DMODEL *relatedModel;
} ParticleEffectHookData; /* size = 0x54 */

typedef struct ParticlePool {
    /* 0x00 */ s32 modelId;
    /* 0x04 */ s32 activeEffectCount;
} ParticlePool; /* size = 0x08 */

typedef struct CameraTransitionEffect {
    /* 0x00 */ u8 isActive;
    /* 0x01 */ u8 reservedFlag;
    /* 0x02 */ char reservedBytes[2];
    /* 0x04 */ s32 modelId;
    /* 0x08 */ s32 frameCount;
    /* 0x0C */ Float elapsedTime;
    /* 0x10 */ Float timeStep;
    /* 0x14 */ Float transitionDuration;
    /* 0x18 */ Float fadeCurveAmplitude;
    /* 0x1C */ Mtx transformMatrix;
} CameraTransitionEffect; /* size = 0x4C */

typedef struct SceneAnimationGroup {
    /* 0x00 */ ANIMDATA *animation0;
    /* 0x04 */ ANIMDATA *animation1;
    /* 0x08 */ ANIMDATA *animation2;
    /* 0x0C */ ANIMDATA *animation3;
    /* 0x10 */ s32 modelId0;
    /* 0x14 */ s32 modelId1;
    /* 0x18 */ s32 modelId2;
    /* 0x1C */ s32 modelId3;
    /* 0x20 */ s32 updateHookId;
    /* 0x24 */ Float modelGroupEnabled;
    /* 0x28 */ Float overlayAlpha;
} SceneAnimationGroup; /* size = 0x2C */

typedef struct SparkleEffectGroup {
    /* 0x00 */ s32 modelId;
    /* 0x04 */ s32 activeEffectCount;
    /* 0x08 */ ANIMDATA *animationData;
} SparkleEffectGroup; /* size = 0x0C */

void UpdateMapShadow(OMOBJ *object);
void UpdateMapModels(OMOBJ *object);
void InitializeMapRenderer(void);
void DestroyMapRenderer(void);
void DrawMapSurface(HU3DMODEL *source, Mtx destination);
void DrawMapScene(HU3DMODEL *source, Mtx destination);
#ifdef NON_MATCHING
void CalculatePathSegmentGeometry(MapPathSegment *source, Float cameraPitch);
#else
void CalculatePathSegmentGeometry(MapPathSegment *source);
#endif
s32 GetPathSegmentAtDistance(Float scale, Float *source);
void UpdateSurfaceParticleModels(HU3DMODEL *source);
void CreateSparkleEffect(void);
void UpdateSparkleEffects(void);
void DestroySparkleEffects(void);
s32 CreateParticleEffect(Vec source, Vec destination, Float scale, Float rotationSpeed, GXColor effectColor);
void CreateSparkleGroup(void);
void UpdateParticleEffects(void);
void DestroyParticleEffects(void);
s32 CreateParticleEffectVariant(Vec source, Vec destination, Float scale, Float rotationSpeed, GXColor effectColor);
MapParticleEffect *AcquireParticleSlot(s32 source);
void CreateParticlePool(void);
void UpdateParticlePool(void);
void DestroyParticlePool(void);
s32 CreateParticleEffectSmall(Vec source, Vec destination, Float scale, GXColor effectColor);
void CreateParticleModelPool(void);
void UpdateParticleModelPool(void);
void DestroyParticleModelPool(void);
s32 CreateParticleEffectColored(Vec source, Vec destination, Float scale, GXColor effectColor);
void ConfigureParticleLifetime(s32 source, s32 destination, Float scale, Float rotationSpeed);
void CreateCameraTransitionEffects(void);
void UpdateCameraTransitionEffects(void);
void DestroyCameraTransitionEffects(void);
void CreateSceneAnimationGroup(void);
void DestroySceneAnimationGroup(void);
void DrawSceneAnimationGroup(HU3DMODEL *source, ParticleEffectHookData *destination, Mtx effectColor);
void SetOrthographicProjection(HU3DMODEL *source, Mtx destination);
void SetAnimationGroupEnabled(Float scale);
s16 CreateModelAnimation(ANIMDATA *source, s16 destination);
void DrawModelHook(HU3DMODEL *source, Mtx destination);
s16 UpdateTextureCoordinates(s16 source);
void DrawModelNormals(HU3DMODEL *source, Mtx destination);
void SetModelMaterialColors(s32 source, Float scale, Float rotationSpeed, Float secondScale, Float thirdScale);
void SetModelMaterialColor(s32 source, u8 destination, u8 effectColor, u8 channelValue);
s16 GetModelLightChannels(HU3DMODEL *source, Mtx destination, Mtx effectColor, Float scale);
void TransformLightDirection(HU3DLIGHT *source, s16 destination, Mtx effectColor, Mtx channelValue, Float scale);
void InterpolateVector(Float scale, Vec *source, Vec *destination, Vec *effectColor);
void TransformVector(Mtx source, Vec *destination);
Float ProjectVectorOntoAxis(Vec *source, Vec *destination, Float scale, Float rotationSpeed);

extern u32 GlobalCounter;

Vec leftSurfacePath[0x46] = {
    { -495.876f, 50.005f, -0.0f },
    { -500.023f, -2841.226f, -5007.74f },
    { -502.015f, -7170.411f, -12506.11f },
    { -515.622f, -8314.231f, -14487.27f },
    { -515.6f, -8761.908f, -15262.67f },
    { -508.661f, -9195.703f, -16014.02f },
    { -494.762f, -9629.499f, -16765.38f },
    { -473.947f, -10063.294f, -17516.73f },
    { -471.378f, -10495.727f, -18265.73f },
    { -495.719f, -10915.74f, -18993.21f },
    { -552.519f, -11337.781f, -19724.21f },
    { -621.485f, -11611.703f, -20198.66f },
    { -731.018f, -11887.653f, -20676.62f },
    { -950.131f, -12303.607f, -21397.07f },
    { -1209.766f, -12735.795f, -22145.64f },
    { -1469.474f, -13170.012f, -22897.73f },
    { -1627.69f, -13466.252f, -23410.83f },
    { -1757.595f, -13772.641f, -23941.51f },
    { -1838.731f, -14079.026f, -24472.19f },
    { -1871.186f, -14393.528f, -25016.92f },
    { -1834.734f, -14703.97f, -25554.62f },
    { -1749.541f, -15014.414f, -26092.33f },
    { -1623.78f, -15312.685f, -26608.94f },
    { -1384.433f, -15761.101f, -27385.62f },
    { -1124.798f, -16191.259f, -28130.68f },
    { -775.859f, -16769.535f, -29132.28f },
    { -568.923f, -17183.457f, -29849.22f },
    { -398.537f, -17621.73f, -30608.33f },
    { -187.485f, -18055.945f, -31360.41f },
    { 96.491f, -18634.223f, -32362.02f },
    { 254.886f, -19042.057f, -33068.41f },
    { 315.738f, -19315.977f, -33542.85f },
    { 331.965f, -19581.783f, -34003.24f },
    { 291.397f, -19839.473f, -34449.57f },
    { 189.969f, -20091.07f, -34885.35f },
    { -12.871f, -20340.645f, -35317.62f },
    { -325.245f, -20598.332f, -35763.96f },
    { -747.265f, -20870.219f, -36234.88f },
    { -1230.024f, -21152.258f, -36723.38f },
    { -2276.853f, -21730.535f, -37724.99f },
    { -2759.968f, -22028.799f, -38241.6f },
    { -3189.989f, -22343.3f, -38786.33f },
    { -3514.533f, -22669.979f, -39352.15f },
    { -3620.335f, -22840.408f, -39647.35f },
    { -3693.358f, -23018.965f, -39956.61f },
    { -3705.528f, -23193.463f, -40258.85f },
    { -3693.358f, -23367.96f, -40561.09f },
    { -3551.37f, -23698.695f, -41133.94f },
    { -3307.945f, -24019.275f, -41689.2f },
    { -2995.571f, -24323.633f, -42216.36f },
    { -2646.686f, -24621.904f, -42732.99f },
    { -1920.124f, -25200.174f, -43734.58f },
    { -1457.648f, -25612.07f, -44448.0f },
    { -1238.474f, -25871.78f, -44897.83f },
    { -1084.316f, -26137.584f, -45358.22f },
    { -958.555f, -26417.594f, -45843.21f },
    { -861.049f, -26697.596f, -46328.19f },
    { -783.97f, -26979.635f, -46816.69f },
    { -715.004f, -27261.672f, -47305.2f },
    { -617.584f, -27979.957f, -48549.3f },
    { -548.553f, -28700.271f, -49796.93f },
    { -491.712f, -29278.535f, -50798.51f },
    { -475.485f, -29856.814f, -51800.12f },
    { -479.514f, -30433.059f, -52798.2f },
    { -505.531f, -34619.363f, -60049.28f },
    { -513.632f, -34659.062f, -60100.39f },
    { -509.573f, -34721.215f, -60151.54f },
    { -509.589f, -34805.496f, -60191.15f },
    { -509.587f, -34970.055f, -60233.17f },
    { -509.591f, -45195.266f, -61377.84f },
};

Vec rightSurfacePath[0x46] = {
    { 497.979f, 49.997f, 0.0f },
    { 501.235f, -2841.251f, -5007.79f },
    { 497.983f, -7170.06f, -12505.5f },
    { 479.876f, -8314.231f, -14487.27f },
    { 479.899f, -8761.908f, -15262.67f },
    { 486.837f, -9195.703f, -16014.02f },
    { 500.736f, -9629.499f, -16765.38f },
    { 521.552f, -10063.294f, -17516.73f },
    { 524.121f, -10495.727f, -18265.73f },
    { 503.13f, -10936.027f, -19028.35f },
    { 446.486f, -11379.887f, -19797.14f },
    { 364.665f, -11685.236f, -20326.02f },
    { 245.082f, -11987.438f, -20849.45f },
    { 18.48f, -12434.451f, -21623.7f },
    { -252.156f, -12875.162f, -22387.03f },
    { -497.617f, -13300.134f, -23123.1f },
    { -655.129f, -13580.305f, -23608.37f },
    { -768.419f, -13847.879f, -24071.83f },
    { -843.945f, -14131.193f, -24562.54f },
    { -875.687f, -14393.528f, -25016.92f },
    { -850.353f, -14653.739f, -25467.62f },
    { -768.533f, -14927.61f, -25941.98f },
    { -642.655f, -15201.48f, -26416.33f },
    { -428.664f, -15620.156f, -27141.5f },
    { -170.569f, -16051.424f, -27888.48f },
    { 188.182f, -16636.941f, -28902.62f },
    { 408.467f, -17083.947f, -29676.86f },
    { 572.186f, -17512.059f, -30418.37f },
    { 786.178f, -17940.18f, -31159.9f },
    { 1075.696f, -18525.695f, -32174.04f },
    { 1245.631f, -18972.703f, -32948.28f },
    { 1315.06f, -19274.898f, -33471.7f },
    { 1327.648f, -19592.842f, -34022.39f },
    { 1283.591f, -19907.637f, -34567.64f },
    { 1157.714f, -20235.021f, -35134.68f },
    { 887.077f, -20562.406f, -35701.73f },
    { 496.857f, -20883.496f, -36257.87f },
    { 31.029f, -21188.842f, -36786.75f },
    { -491.693f, -21487.889f, -37304.71f },
    { -1517.594f, -22057.664f, -38291.59f },
    { -1958.808f, -22331.527f, -38765.94f },
    { -2323.853f, -22595.953f, -39223.93f },
    { -2588.195f, -22850.938f, -39665.58f },
    { -2651.134f, -22964.264f, -39861.87f },
    { -2695.42f, -23074.441f, -40052.7f },
    { -2714.302f, -23187.768f, -40248.99f },
    { -2689.126f, -23316.832f, -40472.53f },
    { -2594.718f, -23552.928f, -40881.46f },
    { -2393.337f, -23814.195f, -41334.0f },
    { -2116.406f, -24081.771f, -41797.45f },
    { -1782.831f, -24371.38f, -42299.07f },
    { -1059.036f, -24941.158f, -43285.95f },
    { -561.404f, -25391.299f, -44065.62f },
    { -290.767f, -25709.24f, -44616.31f },
    { -108.245f, -26024.033f, -45161.55f },
    { 23.926f, -26323.088f, -45679.52f },
    { 130.922f, -26625.29f, -46202.95f },
    { 212.947f, -26918.023f, -46709.98f },
    { 275.886f, -27213.93f, -47222.51f },
    { 376.588f, -27941.104f, -48482.01f },
    { 452.187f, -28658.824f, -49725.14f },
    { 508.832f, -29241.193f, -50733.83f },
    { 521.5f, -29814.105f, -51726.14f },
    { 515.985f, -30433.059f, -52798.2f },
    { 503.81f, -34617.785f, -60046.38f },
    { 495.709f, -34657.49f, -60097.48f },
    { 499.768f, -34719.64f, -60148.64f },
    { 499.752f, -34803.92f, -60188.25f },
    { 499.754f, -34968.48f, -60230.3f },
    { 499.75f, -45193.69f, -61374.92f },
};

Vec shadowModelPositions[3] = {
    { 725.7116f, -20607.168f, -35686.844f },
    { -3135.352f, -23367.234f, -40490.824f },
    { -282.94498f, -26285.674f, -45528.754f },
};

s16 surfaceHookModelId = -1;

s32 mapModelAssets[7] = {
    DATA_MAKE_NUM(DATADIR_M406, 35),
    DATA_MAKE_NUM(DATADIR_M406, 36),
    DATA_MAKE_NUM(DATADIR_M406, 37),
    DATA_MAKE_NUM(DATADIR_M406, 38),
    DATA_MAKE_NUM(DATADIR_M406, 39),
    DATA_MAKE_NUM(DATADIR_M406, 40),
    DATA_MAKE_NUM(DATADIR_M406, 41),
};

s32 mapOverlayAssets[7] = {
    DATA_MAKE_NUM(DATADIR_M406, 48),
    DATA_MAKE_NUM(DATADIR_M406, 49),
    DATA_MAKE_NUM(DATADIR_M406, 50),
    DATA_MAKE_NUM(DATADIR_M406, 51),
    DATA_MAKE_NUM(DATADIR_M406, 52),
    DATA_MAKE_NUM(DATADIR_M406, 53),
    0,
};

s32 particleModelAssets[6] = {
    DATA_MAKE_NUM(DATADIR_M406, 42),
    DATA_MAKE_NUM(DATADIR_M406, 43),
    DATA_MAKE_NUM(DATADIR_M406, 44),
    DATA_MAKE_NUM(DATADIR_M406, 45),
    DATA_MAKE_NUM(DATADIR_M406, 46),
    DATA_MAKE_NUM(DATADIR_M406, 47),
};

Vec mapLightPosition = { 0.0f, 0.0f, 0.0f };

Vec mapLightDirection = { -1.0f, -2.0f, -1.0f };

#if VERSION_NTSC
GXColor mapLightColor = { 255, 255, 255, 255 };
#else
GXColor mapLightColor = { 186, 186, 186, 255 };
#endif

Vec mapShadowPosition = { 0.0f, 2500.0f, -500.0f };

Vec mapShadowUp = { 0.0f, 1.0f, 0.0f };

Vec mapShadowTarget = { 0.0f, 0.0f, 0.0f };

Vec mapEffectOrigin = { 0.0f, 0.0f, 0.0f };

Vec mapEffectDirection = { 0.0f, 1.0f, -1.0f };

GXColor mapParticleColors[8] = {
    { 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xC4, 0x2F, 0, 0 },
    { 0x45, 0x1C, 0x40, 0 },
    { 0xC4, 0x2F, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0x80, 0x80, 0x80, 0xFF },
};

HUPROCESS *mapProcess;
OMOBJ *shadowObject;
OMOBJ *mapModelObject;
OMOBJ *surfaceObject;
OMOBJ *effectObject;
s32 mapState;
s32 mapRendererEnabled;
MapPathSegment *pathSegments;
s32 pathSegmentCount;
SparkleEffectGroup *sparkleEffectGroup;
SparkleEffectGroup *particleEffectGroup;
ParticlePool *particleModelPool;
ParticlePool *particleInstancePool;
CameraTransitionEffect *cameraTransitionEffects;
SceneAnimationGroup *sceneAnimationGroup;
s32 mapLightId;
char mapRendererPadding[0xC];
MapParticleEffect sharedParticleEffect;
Vec interpolatedVector;
s32 sceneLightIds[3];

extern HU3DLIGHT Hu3DLocalLight[0x20];

OMOBJ *CreateMapScene(HUPROCESS *source)
{
    s32 modelIndex;
    s32 modelId;
    s32 lightId;

    mapProcess = source;
    mapState = 0;
    mapRendererEnabled = 1;
    mapLightId = -1;
    omMakeGroupEx(source, 1, 1);
    omMakeGroupEx(source, 2, 1);
    omMakeGroupEx(source, 3, 1);
    shadowObject = omAddObjEx(source, 127, 0, 0, -1, UpdateMapShadow);
    lightId = Hu3DGLightCreateV(&mapLightPosition, &mapLightDirection, &mapLightColor);
    Hu3DGLightStaticSet(lightId, 1);
    Hu3DGLightInfinitytSet(lightId);
    mapModelObject = omAddObjEx(source, 0x7E, 0x15, 0, 1, UpdateMapModels);
    for (modelIndex = 0; modelIndex < 7; modelIndex++) {
        modelId = mapModelObject->mdlId[modelIndex] = Hu3DModelCreateFile(mapModelAssets[modelIndex]);
        Hu3DModelShadowMapSet(modelId);
        Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
        if (modelIndex < 6) {
            SetModelMaterialColors(modelId, 1.25f, 1.05f, 1.0f, 1.0f);
        }
        if (modelIndex < 6) {
            modelId = Hu3DModelCreateFile(mapOverlayAssets[modelIndex]);
            mapModelObject->mdlId[modelIndex + 7] = modelId;
        }
        else {
            modelId = -1;
            mapModelObject->mdlId[modelIndex + 7] = modelId;
        }
        if (modelId != -1) {
            Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
            Hu3DModelLayerSet(modelId, 5);
        }
    }
    modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M406, 54));
    mapModelObject->mdlId[14] = modelId;
    Hu3DModelPosSet(modelId, 0.0f, 10.0f, 0.0f);
    Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(modelId, 5);
    surfaceObject = omAddObjEx(source, 0x7D, 6, 0, 2, NULL);
    for (modelIndex = 0; modelIndex < 6; modelIndex++) {
        surfaceObject->mdlId[modelIndex] = Hu3DModelCreateFile(particleModelAssets[modelIndex]);
        Hu3DModelAttrSet(surfaceObject->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
    }
    effectObject = omAddObjEx(source, 0x7D, 3, 0, 3, NULL);
    effectObject->trans.x = shadowModelPositions->x;
    effectObject->trans.y = shadowModelPositions->y;
    effectObject->trans.z = shadowModelPositions->z;
    effectObject->scale.x = effectObject->scale.y = effectObject->scale.z = 3.0f;
    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
        if (modelIndex == 0) {
            effectObject->mdlId[modelIndex] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M406, 34));
        }
        else {
            effectObject->mdlId[modelIndex] = Hu3DModelLink(*effectObject->mdlId);
        }
        Hu3DModelShadowSet(effectObject->mdlId[modelIndex]);
        Hu3DModelScaleSet(effectObject->mdlId[modelIndex], effectObject->scale.x, effectObject->scale.y, effectObject->scale.z);
        Hu3DModelPosSet(effectObject->mdlId[modelIndex], shadowModelPositions[modelIndex].x, shadowModelPositions[modelIndex].y, shadowModelPositions[modelIndex].z);
    }
    InitializeMapRenderer();
    CreateSparkleEffect();
    CreateSparkleGroup();
    CreateParticlePool();
    CreateParticleModelPool();
    CreateCameraTransitionEffects();
    sceneLightIds[0] = 1;
    CreateSceneAnimationGroup();
    SetAnimationGroupAlpha(0.0f);
    SetAnimationGroupEnabled(0.0f);
    Hu3DShadowCreate(45.0f, 1000.0f, 250000.0f);
    Hu3DShadowTPLvlSet(0.3f);
    Hu3DShadowPosSet(&mapShadowPosition, &mapShadowUp, &mapShadowTarget);
    return shadowObject;
}

void UpdateMapShadow(OMOBJ *object)
{
    Vec shadowUpDirection;
    Vec shadowPosition;
    Vec cameraTarget;
    Float shadowDistance;

    Float cameraPitch;
    Float cameraYaw;
    Float cameraDistance;

    cameraDistance = 1000.0f + CZoom;
    cameraPitch = -65.0f;
    cameraYaw = 110.0f;
    shadowDistance = 0.0f;
    cameraTarget.x = Center.x;
    cameraTarget.y = Center.y;
    cameraTarget.z = Center.z;

    shadowPosition.x = cameraTarget.x + (cameraDistance * (sind(cameraYaw) * cosd(cameraPitch)));
    shadowPosition.y = cameraTarget.y + (cameraDistance * -sind(cameraPitch));
    shadowPosition.z = cameraTarget.z + (cameraDistance * (cosd(cameraYaw) * cosd(cameraPitch)));
    shadowUpDirection.x = sind(cameraYaw) * sind(cameraPitch);
    shadowUpDirection.y = cosd(cameraPitch);
    shadowUpDirection.z = cosd(cameraYaw) * sind(cameraPitch);
    Hu3DShadowPosSet(&shadowPosition, &shadowUpDirection, &cameraTarget);
}

void InitializeMapEffects(void)
{
    DestroyMapRenderer();
    DestroySparkleEffects();
    DestroyParticleEffects();
    DestroyParticlePool();
    DestroyParticleModelPool();
    DestroyCameraTransitionEffects();
    DestroySceneAnimationGroup();
}

void UpdateMapModels(OMOBJ *object)
{
    Float modelDepth;

    s32 modelIndex;
    s32 pairedModelIndex;

    if (!mapState) {
        for (modelIndex = 0; modelIndex < 6; modelIndex++) {
            modelDepth = -(5000.0f + (10000.0f * modelIndex));
            if (fabs(Center.z - modelDepth) > 10000.0 || !mapRendererEnabled) {
                if (object->mdlId[modelIndex] != -1) {
                    Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
                }
                pairedModelIndex = modelIndex + 7;
                if (object->mdlId[pairedModelIndex] != -1) {
                    Hu3DModelAttrSet(object->mdlId[pairedModelIndex], HU3D_ATTR_DISPOFF);
                }
            }
            else {
                if (object->mdlId[modelIndex] != -1) {
                    Hu3DModelAttrReset(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
                }
                pairedModelIndex = modelIndex + 7;
                if (object->mdlId[pairedModelIndex] != -1) {
                    Hu3DModelAttrReset(object->mdlId[pairedModelIndex], HU3D_ATTR_DISPOFF);
                }
            }
        }
        pairedModelIndex = 14;
        if (object->mdlId[pairedModelIndex] != -1) {
            modelDepth = 25000.0f;
            if (fabs(Center.z - modelDepth) < 15000.0) {
                Hu3DModelAttrSet(object->mdlId[pairedModelIndex], HU3D_ATTR_DISPOFF);
            }
            else {
                Hu3DModelAttrReset(object->mdlId[pairedModelIndex], HU3D_ATTR_DISPOFF);
            }
        }
        pairedModelIndex = 6;
        if (object->mdlId[pairedModelIndex] != -1) {
            Hu3DModelAttrSet(object->mdlId[pairedModelIndex], HU3D_ATTR_DISPOFF);
        }
        for (modelIndex = 0; modelIndex < 3; modelIndex++) {
            if (fabs(Center.z - shadowModelPositions[modelIndex].z) > 10000.0) {
                Hu3DModelAttrSet(effectObject->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
            }
            else {
                Hu3DModelAttrReset(effectObject->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
            }
        }
        return;
    }
    for (modelIndex = 0; modelIndex < 6; modelIndex++) {
        if (object->mdlId[modelIndex] != -1) {
            Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
        }
        pairedModelIndex = modelIndex + 7;
        if (object->mdlId[pairedModelIndex] != -1) {
            Hu3DModelAttrSet(object->mdlId[pairedModelIndex], HU3D_ATTR_DISPOFF);
        }
    }
    pairedModelIndex = 6;
    if (object->mdlId[pairedModelIndex] != -1) {
        Hu3DModelAttrReset(object->mdlId[pairedModelIndex], HU3D_ATTR_DISPOFF);
    }
    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
        Hu3DModelAttrSet(effectObject->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
    }
}

void SetMapState(s32 source)
{
    s32 modelIndex;

    if (mapState = source) {
        for (modelIndex = 0; modelIndex < 6; modelIndex++) {
            if (mapModelObject->mdlId[modelIndex] != -1) {
                Hu3DModelKill(mapModelObject->mdlId[modelIndex]);
            }
            mapModelObject->mdlId[modelIndex] = -1;
        }
        if (mapModelObject->mdlId[6] == -1) {
            mapModelObject->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M406, 41));
        }
    }
}

void InitializeMapRenderer(void)
{
    MapPathSegment *displayListAddresses[2]; // TODO display_list_addresses should probably be a void*
    s32 lightModelIds[2];

    Float edgeFadeFactor;
    Float gridX;
    Float gridZ;
    Float gridY;

    MapSurfaceRenderer *surfaceRenderer;
    SurfaceParticleData *surfaceVertex;
    s32 elementIndex;
    SurfaceParticleModel *particleModel;
    PathParticleSample *pathSample;
    HU3DMODEL *hookModel;
    s32 columnIndex;
    Vec *positionVertex;
    GXColor *colorVertex;
    s16(*triangleIndices)[4];
    Vec *normalVertex;
    Vec2f *textureVertex;
    s32 hookModelId;
    void *displayListBuffer;
    HU3DMODEL *lightModel;

#ifdef NON_MATCHING
    edgeFadeFactor = 0.0f;
#endif

    hookModelId = surfaceHookModelId = Hu3DHookFuncCreate(DrawMapSurface);
    hookModel = &Hu3DData[hookModelId];
    surfaceRenderer = HuMemDirectMallocNum(HEAP_MODEL, sizeof(MapSurfaceRenderer), hookModel->mallocNo);
    hookModel->hookData = surfaceRenderer;
    memset(surfaceRenderer, 0, sizeof(MapSurfaceRenderer));

    lightModelIds[0] = Hu3DHookFuncCreate(DrawMapScene);
    Hu3DModelLayerSet(lightModelIds[0], 7);
    lightModel = &Hu3DData[lightModelIds[0]];
    lightModel->hookData = surfaceRenderer;

    lightModelIds[1] = Hu3DLLightCreateV(hookModelId, &mapEffectOrigin, &mapEffectDirection, mapParticleColors);
    Hu3DLLightStaticSet(hookModelId, lightModelIds[1], 1);
    Hu3DLLightInfinitytSet(hookModelId, lightModelIds[1]);

    surfaceRenderer->rendererUpdateCount = 0;
    surfaceRenderer->cameraHookModelId = hookModelId;
    surfaceRenderer->useDefaultDrawHook = 0;
    surfaceRenderer->currentPathDistance = 0.0f;
    surfaceRenderer->pathDistanceSpeed = 0.0f;
    surfaceRenderer->pathSampleSpacing = 2000.0f;
    surfaceRenderer->displayListMode = 0;
    surfaceRenderer->particleTextureData = HuDataReadNum(DATA_MAKE_NUM(DATADIR_M406, 61), HU_MEMNUM_OVL);
    surfaceRenderer->primaryColorRed = 0x80;
    surfaceRenderer->primaryColorGreen = 0x80;
    surfaceRenderer->primaryTextureScrollX = 0.0f;
    surfaceRenderer->primaryTextureScrollY = 0.0f;
    surfaceRenderer->secondaryTextureData = HuDataReadNum(DATA_MAKE_NUM(DATADIR_M406, 62), HU_MEMNUM_OVL);
    surfaceRenderer->secondaryColorRed = 0x80;
    surfaceRenderer->secondaryColorGreen = 0x80;
    surfaceRenderer->secondaryTextureScrollX = 0.0f;
    surfaceRenderer->secondaryTextureScrollY = 0.0f;
    surfaceRenderer->particlePathSamples = HuMemDirectMallocNum(HEAP_MODEL, 105 * sizeof(PathParticleSample), hookModel->mallocNo);
    memset(surfaceRenderer->particlePathSamples, 0, 105 * sizeof(PathParticleSample));
    pathSample = surfaceRenderer->particlePathSamples;

    for (elementIndex = 0; elementIndex < 105; elementIndex++, pathSample++) {
        pathSample->pathSampleIndex = elementIndex / 3;
        pathSample->pathSampleFraction = 0.33333334f * (elementIndex % 3);
        switch (elementIndex % 4) {
            case 0:
                columnIndex = pathSample->effectSpawnCountdown = frand() & 0x1F;
                break;
            case 1:
                pathSample->effectSpawnCountdown = columnIndex + 10;
                break;
            default:
                pathSample->effectSpawnCountdown = columnIndex + 20;
                break;
        }
        pathSample->effectType = -1;
        pathSample->verticalVelocity = 0.0f;
        if (elementIndex == 0 || elementIndex == 104) {
            pathSample->particleScale = 0.3f;
        }
        if (elementIndex == 1 || elementIndex == 103) {
            pathSample->particleScale = 0.6f;
        }
        else {
            pathSample->particleScale = 1.0f;
        }
        pathSample->particlePosition.x = pathSample->particlePosition.y = pathSample->particlePosition.z = 0.0f;
        pathSample->particleObject = 0;
    }
    surfaceRenderer->particleModels = HuMemDirectMallocNum(HEAP_MODEL, 12 * sizeof(SurfaceParticleModel), hookModel->mallocNo);
    memset(surfaceRenderer->particleModels, 0, 12 * sizeof(SurfaceParticleModel));
    particleModel = surfaceRenderer->particleModels;

    for (elementIndex = 0; elementIndex < 12; elementIndex++, particleModel++) {
        switch (elementIndex) {
            case 0:
                particleModel->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M406, 56));
                break;
            case 1:
                particleModel->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M406, 57));
                break;
            case 2:
                particleModel->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M406, 58));
                break;
            default:
                particleModel->modelId = Hu3DModelLink(surfaceRenderer->particleModels[elementIndex % 3].particleModelAssetId);
                break;
        }
        if (elementIndex < 3) {
            SetModelMaterialColor(particleModel->modelId, 184, 207, 214);
        }
        particleModel->effectState = frand() & 1;
        particleModel->effectFrame = 0;
        particleModel->frameDelay = frand() % 60;
        particleModel->scaleFactor = 0.2f + (0.6f * (0.007874016f * (frand() & 0x7F)));
        particleModel->rotationAngle = particleModel->rotationVelocity = 0.0f;
        particleModel->opacity = 0.0f;
        particleModel->rotationX = particleModel->rotationY = 0.0f;
        particleModel->effectWidth = 0.0f;
        particleModel->effectHeight = 0.0f;
        particleModel->modelVelocity.x = particleModel->modelVelocity.y = particleModel->modelVelocity.z = 0.0f;
    }

    surfaceRenderer->startPathPoint.x = surfaceRenderer->startPathPoint.y = surfaceRenderer->startPathPoint.z = 0.0f;
    surfaceRenderer->endPathPoint.x = surfaceRenderer->endPathPoint.y = surfaceRenderer->endPathPoint.z = 0.0f;
    surfaceRenderer->currentPathSegment = 0;

    surfaceRenderer->particleVertexCount = 1050;
    surfaceRenderer->particlePositions = HuMemDirectMallocNum(HEAP_MODEL, surfaceRenderer->particleVertexCount * sizeof(Vec), hookModel->mallocNo);
    memset(surfaceRenderer->particlePositions, 0, surfaceRenderer->particleVertexCount * sizeof(Vec));

    surfaceRenderer->particleNormals = HuMemDirectMallocNum(HEAP_MODEL, surfaceRenderer->particleVertexCount * sizeof(Vec), hookModel->mallocNo);
    memset(surfaceRenderer->particleNormals, 0, surfaceRenderer->particleVertexCount * sizeof(Vec));

    surfaceRenderer->particleTextureCoordinates = HuMemDirectMallocNum(HEAP_MODEL, surfaceRenderer->particleVertexCount * sizeof(Vec2f), hookModel->mallocNo);
    memset(surfaceRenderer->particleTextureCoordinates, 0, surfaceRenderer->particleVertexCount * sizeof(Vec2f));

    surfaceRenderer->particleColors = HuMemDirectMallocNum(HEAP_MODEL, surfaceRenderer->particleVertexCount * sizeof(GXColor), hookModel->mallocNo);
    memset(surfaceRenderer->particleColors, 0, surfaceRenderer->particleVertexCount * sizeof(GXColor));

    surfaceRenderer->particleSurfaceData = HuMemDirectMallocNum(HEAP_MODEL, surfaceRenderer->particleVertexCount * sizeof(SurfaceParticleData), hookModel->mallocNo);
    memset(surfaceRenderer->particleSurfaceData, 0, surfaceRenderer->particleVertexCount * sizeof(SurfaceParticleData));

    surfaceRenderer->displayListIndexCount = 986;
    surfaceRenderer->particleTriangleIndices = HuMemDirectMallocNum(HEAP_MODEL, surfaceRenderer->displayListIndexCount * 8, hookModel->mallocNo);
    memset(surfaceRenderer->particleTriangleIndices, 0, surfaceRenderer->displayListIndexCount * 8);

    positionVertex = surfaceRenderer->particlePositions;
    normalVertex = surfaceRenderer->particleNormals;
    textureVertex = surfaceRenderer->particleTextureCoordinates;
    colorVertex = surfaceRenderer->particleColors;
    surfaceVertex = surfaceRenderer->particleSurfaceData;
    gridX = -500.0f;
    gridZ = -1000.0f;
    gridY = 0.0f;
    for (elementIndex = 0; elementIndex < 30; elementIndex++) {
        for (columnIndex = 0; columnIndex < 35; columnIndex++) {
            positionVertex->x = gridX;
            positionVertex->y = gridY;
            positionVertex->z = gridZ;

            gridX += 29.411764f;
            gridY = 0.0f;

            normalVertex->x = 0.0f;
            normalVertex->z = 0.0f;
            normalVertex->y = 1.0f;

            textureVertex->x = 0.0f;
            textureVertex->y = 0.0f;

            colorVertex->r = colorVertex->g = colorVertex->b = colorVertex->a = 0xFF;
            if (elementIndex == 0) {
                surfaceVertex->isFirstRow = 1;
                if (elementIndex < 6) {
                    surfaceVertex->edgeFadeScale = (6 - elementIndex) / 6.0f;
                }
                else {
                    surfaceVertex->edgeFadeScale = 1.0f;
                }
            }
            else {
                surfaceVertex->isFirstRow = 0;
                surfaceVertex->edgeFadeScale = 0.0f;
            }
            if (!columnIndex || columnIndex == 34 || !elementIndex) {
                surfaceVertex->isActive = 0;
                surfaceVertex->edgeFade = 0.0f;
            }
            else {
                surfaceVertex->isActive = 1;
                if (elementIndex < 3) {
                    surfaceVertex->edgeFade = 0.5f + (0.5f * (elementIndex / 3.0f));
                    surfaceVertex->edgeFade = sind(90.0f * surfaceVertex->edgeFade);
                }
                else {
                    surfaceVertex->edgeFade = 1.0f;
                }
            }
            surfaceVertex->basePosition.x = positionVertex->x;
            surfaceVertex->basePosition.y = positionVertex->y;
            surfaceVertex->basePosition.z = positionVertex->z;
            surfaceVertex->positionVelocity.x = surfaceVertex->positionVelocity.y = surfaceVertex->positionVelocity.z = 0.0f;
            surfaceVertex->verticalOffset = 200.0 * sind(90.0f * (elementIndex / 29.0f));
            if (columnIndex < 6) {
                edgeFadeFactor = columnIndex / 6.0f;
                surfaceVertex->edgeFade = surfaceVertex->edgeFade * sind(90.0f * edgeFadeFactor);
                surfaceVertex->verticalOffset = surfaceVertex->verticalOffset * sind(90.0f * edgeFadeFactor);
            }
            else if (columnIndex > 28) {
                edgeFadeFactor = (columnIndex - 28) / 6.0f;
                surfaceVertex->edgeFade = surfaceVertex->edgeFade * cosd(90.0f * edgeFadeFactor);
                surfaceVertex->verticalOffset = surfaceVertex->verticalOffset * cosd(90.0f * edgeFadeFactor);
            }
            surfaceVertex->textureCoordinateU = columnIndex / 34.0f;
            surfaceVertex->textureCoordinateV = (elementIndex * 2) / 29.0f;
            if (surfaceVertex->textureCoordinateU > 1.0f) {
                surfaceVertex->textureCoordinateU = fmod(surfaceVertex->textureCoordinateU, 1.0);
            }
            if (surfaceVertex->textureCoordinateV > 1.0f) {
                surfaceVertex->textureCoordinateV = fmod(surfaceVertex->textureCoordinateV, 1.0);
            }
            positionVertex++;
            normalVertex++;
            textureVertex++;
            colorVertex++;
            surfaceVertex++;
        }
        gridX = -500.0f;
        gridZ += 68.965515f;
    }
    triangleIndices = surfaceRenderer->particleTriangleIndices;
    for (elementIndex = 0; elementIndex < 29; elementIndex++) {
        for (columnIndex = 0; columnIndex < 34; triangleIndices++, columnIndex++) {
            (*triangleIndices)[0] = columnIndex + (elementIndex * 35);
            (*triangleIndices)[1] = columnIndex + 1 + (elementIndex * 35);
            (*triangleIndices)[2] = columnIndex + 1 + ((elementIndex + 1) * 35);
            (*triangleIndices)[3] = columnIndex + ((elementIndex + 1) * 35);
        }
    }
    displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, 0x10000, hookModel->mallocNo);
    displayListAddresses[0] = displayListBuffer;
    DCFlushRange(displayListBuffer, 0x10000);
    GXBeginDisplayList(displayListAddresses[0], 0x10000);
    GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 70);
    for (elementIndex = 0; elementIndex < 35; elementIndex++) {
        GXPosition1x16(elementIndex + 35);
        GXNormal1x16(elementIndex + 35);
        GXColor1x16(elementIndex + 35);
        GXPosition1x16(elementIndex);
        GXNormal1x16(elementIndex);
        GXColor1x16(elementIndex);
    }
    surfaceRenderer->displayListSize = GXEndDisplayList();
    if (surfaceRenderer->displayListSize >= 0x10000) {
        OSReport("ERROR! GList test size over! \n");
    }
    surfaceRenderer->displayListData = HuMemDirectMallocNum(HEAP_MODEL, surfaceRenderer->displayListSize, hookModel->mallocNo);
    memcpy(surfaceRenderer->displayListData, displayListBuffer, surfaceRenderer->displayListSize);
    DCFlushRange(surfaceRenderer->displayListData, surfaceRenderer->displayListSize);
    HuMemDirectFree(displayListBuffer);
    pathSegmentCount = 70;
    pathSegments = HuMemDirectMallocNum(HEAP_MODEL, pathSegmentCount * 32, hookModel->mallocNo);
    memset(pathSegments, 0, pathSegmentCount * 32);
#ifdef NON_MATCHING
    CalculatePathSegmentGeometry(pathSegments, edgeFadeFactor);
#else
    CalculatePathSegmentGeometry(pathSegments);
#endif
    displayListAddresses[1] = &pathSegments[pathSegmentCount - 1];
    surfaceRenderer->totalPathLength = displayListAddresses[1]->segmentStartDistance;
    surfaceRenderer->pathDistanceOverflow = 0.0f;
    surfaceRenderer->audioEmitterId = -1;
    surfaceRenderer->audioStartDelay = 0;
}

GXColor particleWhite = { 0xFF, 0xFF, 0xFF, 0xFF };
GXColor particleGray = { 0xB3, 0xBB, 0xC2, 0xFF };
GXColor particleOpaqueWhite = { 0xFF, 0xFF, 0xFF, 0xFF };
GXColor particleTranslucentWhite = { 0xFF, 0xFF, 0xFF, 0x40 };
GXColor particleFadeWhite = { 0xFF, 0xFF, 0xFF, 0xFF };

Float particleLifetimeValues[4] = { 1000.0f, 200.0f, 600.0f, 1200.0f };

Float particleSizeValues[4] = { 1.0f, 0.6f, 0.3f, 0.425f };

Vec particleOrigin = { 0.0f, -34700.0f, -62094.0f };

Vec particleUpDirection = { 0.0f, 1.0f, 0.0f };

GXColor particleDefaultColor = { 0xFF, 0xFF, 0xFF, 0xFF };

Vec particleSpawnPositions[4] = {
    { -0.5f, 0.5f, 0.0f },
    { 0.5f, 0.5f, 0.0f },
    { 0.5f, -0.5f, 0.0f },
    { -0.5f, -0.5f, 0.0f },
};

Vec2f particleTextureCoordinates[4] = { { 0.0f, 0.0f }, { 0.25f, 0.0f }, { 0.25f, 0.25f }, { 0.0f, 0.25f } };

Vec particleModelPositions[4] = {
    { 0.0f, 0.5f, 0.0f },
    { 0.5f, 0.0f, 0.0f },
    { 0.0f, -0.5f, 0.0f },
    { -0.5f, 0.0f, 0.0f },
};

void UpdateMapAmbience(void)
{
    Vec audioPosition;
    Vec pathStart;
    Vec pathEnd;
    void *pathSamples;
    Float pathSampleFraction;

    Float crossSectionFraction;
    Float pathDistance;
    Float pathYaw;
    Float vertexSpacing;

    MapSurfaceRenderer *surfaceRenderer;
    s32 pathSegmentIndex;
    SurfaceParticleData *surfaceVertex;
    Vec *positionVertex;
    s32 rowIndex;
    s32 columnIndex;
    HU3DMODEL *hookModel;

    pathSamples = pathSegments;
    hookModel = &Hu3DData[surfaceHookModelId];
    surfaceRenderer = hookModel->hookData;
    if (!surfaceRenderer->displayListMode) {
        if (++surfaceRenderer->audioStartDelay >= 1 && surfaceRenderer->audioEmitterId == -1) {
            audioPosition.x = 0.5f * (surfaceRenderer->startPathPoint.x + surfaceRenderer->endPathPoint.x);
            audioPosition.y = 0.5f * (surfaceRenderer->startPathPoint.y + surfaceRenderer->endPathPoint.y);
            audioPosition.z = 0.5f * (surfaceRenderer->startPathPoint.z + surfaceRenderer->endPathPoint.z);
            surfaceRenderer->audioEmitterId = HuAudFXEmiterPlay(1337, &audioPosition);
        }
        else if (surfaceRenderer->audioEmitterId != -1) {
            audioPosition.x = 0.5f * (surfaceRenderer->startPathPoint.x + surfaceRenderer->endPathPoint.x);
            audioPosition.y = 0.5f * (surfaceRenderer->startPathPoint.y + surfaceRenderer->endPathPoint.y);
            audioPosition.z = 0.5f * (surfaceRenderer->startPathPoint.z + surfaceRenderer->endPathPoint.z);
            HuAudFXEmiterUpDate(surfaceRenderer->audioEmitterId, &audioPosition);
        }
    }
    else if (surfaceRenderer->audioEmitterId != -1) {
        HuAudFXStop(surfaceRenderer->audioEmitterId);
        surfaceRenderer->audioEmitterId = -1;
    }
    surfaceRenderer->currentPathDistance = surfaceRenderer->currentPathDistance + surfaceRenderer->pathDistanceSpeed;
    if (surfaceRenderer->currentPathDistance > surfaceRenderer->totalPathLength) {
        surfaceRenderer->pathDistanceOverflow = surfaceRenderer->currentPathDistance - surfaceRenderer->totalPathLength;
        surfaceRenderer->currentPathDistance = surfaceRenderer->totalPathLength;
    }
    else {
        surfaceRenderer->pathDistanceOverflow = 0.0f;
    }
    positionVertex = surfaceRenderer->particlePositions;
    surfaceVertex = surfaceRenderer->particleSurfaceData;
    pathDistance = 0.0f;
    vertexSpacing = surfaceRenderer->pathSampleSpacing / 29.0f;

    for (rowIndex = 0; rowIndex < 30; rowIndex++) {
        pathSegmentIndex = GetPathSegmentAtDistance(surfaceRenderer->currentPathDistance - pathDistance, &pathSampleFraction);
        pathDistance += vertexSpacing;
        if (pathSegmentIndex < 0) {
            pathSegmentIndex = 0;
            pathStart = leftSurfacePath[pathSegmentIndex];
            pathEnd = rightSurfacePath[pathSegmentIndex];
        }
        else if (pathSegmentIndex < (pathSegmentCount - 1)) {
            InterpolateVector(pathSampleFraction, &leftSurfacePath[pathSegmentIndex], &leftSurfacePath[pathSegmentIndex + 1], &pathStart);
            InterpolateVector(pathSampleFraction, &rightSurfacePath[pathSegmentIndex], &rightSurfacePath[pathSegmentIndex + 1], &pathEnd);
        }
        else {
            pathSegmentIndex = pathSegmentCount - 1;
            pathStart = leftSurfacePath[pathSegmentIndex];
            pathEnd = rightSurfacePath[pathSegmentIndex];
        }
        if (rowIndex == 0) {
            surfaceRenderer->startPathPoint = pathStart;
            surfaceRenderer->endPathPoint = pathEnd;
            surfaceRenderer->currentPathSegment = pathSegmentIndex;
            pathYaw = atan2d(surfaceRenderer->endPathPoint.x - surfaceRenderer->startPathPoint.x, surfaceRenderer->endPathPoint.z - surfaceRenderer->startPathPoint.z);
            mtxRot(surfaceRenderer->pathRotationMatrix, 0.0f, pathYaw, 0.0f);
        }
        for (columnIndex = 0, crossSectionFraction = 0.0f; columnIndex < 35; positionVertex++, surfaceVertex++, columnIndex++) {
            InterpolateVector(crossSectionFraction, &pathStart, &pathEnd, positionVertex);
            surfaceVertex->basePosition = *positionVertex;
            crossSectionFraction += 0.029411765f;
            surfaceVertex->positionVelocity = pathSegments[pathSegmentIndex].segmentDirection;
        }
    }
}

void DestroyMapRenderer(void)
{
    MapSurfaceRenderer *surfaceRenderer;
    HU3DMODEL *hookModel;

    hookModel = &Hu3DData[surfaceHookModelId];
    surfaceRenderer = hookModel->hookData;
    HuMemDirectFree(surfaceRenderer->particleTextureData);
}

void DrawMapSurface(HU3DMODEL *source, Mtx destination)
{
    Mtx normalMatrix;

    MapSurfaceRenderer *surfaceRenderer;
    s16 rowIndex;
    s16 vertexStartIndex;
    s16 channelControlFlags;

    surfaceRenderer = source->hookData;
    if (!surfaceRenderer->useDefaultDrawHook) {
        DrawMapScene(source, destination);
    }
    if (!surfaceRenderer->displayListMode) {
        GXLoadPosMtxImm(destination, 0);
        MTXInvXpose(destination, normalMatrix);
        GXLoadNrmMtxImm(normalMatrix, 0);
        channelControlFlags = GetModelLightChannels(source, Hu3DCameraMtx, Hu3DCameraMtxXPose, 2.0f);
        GXSetNumTevStages(1);
        GXSetNumTexGens(0);
        GXSetNumChans(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_NRM, 0x36, GX_FALSE, 125);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, GX_SRC_REG, channelControlFlags, GX_DF_CLAMP, GX_AF_SPEC);
        GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
        GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GXSetZCompLoc(GX_TRUE);
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
        GXSetChanAmbColor(GX_COLOR0A0, particleGray);
        GXSetChanMatColor(GX_COLOR0A0, particleWhite);
        DCFlushRangeNoSync(surfaceRenderer->particlePositions, surfaceRenderer->particleVertexCount * sizeof(Vec));
        DCFlushRangeNoSync(surfaceRenderer->particleNormals, surfaceRenderer->particleVertexCount * sizeof(Vec));
        DCFlushRangeNoSync(surfaceRenderer->particleTextureCoordinates, surfaceRenderer->particleVertexCount * sizeof(Vec2f));
        DCFlushRangeNoSync(surfaceRenderer->particleColors, surfaceRenderer->particleVertexCount * sizeof(GXColor));
        PPCSync();
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetArray(GX_VA_POS, surfaceRenderer->particlePositions, sizeof(Vec));
        GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
        GXSetArray(GX_VA_NRM, surfaceRenderer->particleNormals, sizeof(Vec));
        GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GXSetArray(GX_VA_CLR0, surfaceRenderer->particleColors, 4);
        GXCallDisplayList(surfaceRenderer->displayListData, surfaceRenderer->displayListSize);
        for (rowIndex = 1; rowIndex < 29; rowIndex++) {
            vertexStartIndex = rowIndex * 35;
            GXSetArray(GX_VA_POS, &surfaceRenderer->particlePositions[vertexStartIndex], sizeof(Vec));
            GXSetArray(GX_VA_NRM, &surfaceRenderer->particleNormals[vertexStartIndex], sizeof(Vec));
            GXSetArray(GX_VA_CLR0, &surfaceRenderer->particleColors[vertexStartIndex], sizeof(GXColor));
            GXCallDisplayList(surfaceRenderer->displayListData, surfaceRenderer->displayListSize);
        }
    }
}

void UpdateSurfaceParticles(HU3DMODEL *source)
{
    MapParticleEffect unusedParticleRecord;
    Vec spawnPosition;
    Vec pathStart;
    Vec pathEnd;
    Vec spawnVelocity;
    Vec2f primaryTextureScale;
    Vec2f secondaryTextureScale;
    GXColor particleColor;
    Float pathSampleFraction;

    Float effectAngle;
    Float effectSpeed;
    Float effectScale;

    SurfaceParticleModel *particleModel;
    MapSurfaceRenderer *surfaceRenderer;
    PathParticleSample *pathSample;
    SurfaceParticleData *surfaceVertex;
    s32 elementIndex;
    s32 randomValue;
    Vec *positionVertex;
    s32 pathSegmentIndex;
    s32 textureX;
    s32 textureY;
    Vec *normalVertex;
    Vec2f *textureVertex;
    GXColor *colorVertex;

    surfaceRenderer = source->hookData;

    if (surfaceRenderer->displayListMode) {
        if (!omPauseChk()) {
            UpdateSurfaceParticleModels(source);
        }
        return;
    }
    if (!omPauseChk()) {
        surfaceRenderer->primaryTextureScrollX += 0.005f;
        surfaceRenderer->primaryTextureScrollY += (0.007499999832361937 + (0.001 * surfaceRenderer->pathDistanceOverflow));
        if (fabs(surfaceRenderer->primaryTextureScrollX) > 1.0) { // 1.0 or 1.0f?
            surfaceRenderer->primaryTextureScrollX = fmod(surfaceRenderer->primaryTextureScrollX, 1.0);
        }
        if (surfaceRenderer->primaryTextureScrollX < 0.0f) {
            surfaceRenderer->primaryTextureScrollX += 1.0f;
        }
        if (fabs(surfaceRenderer->primaryTextureScrollY) > 1.0) {
            surfaceRenderer->primaryTextureScrollY = fmod(surfaceRenderer->primaryTextureScrollY, 1.0);
        }
        if (surfaceRenderer->primaryTextureScrollY < 0.0f) {
            surfaceRenderer->primaryTextureScrollY += 1.0f;
        }
        surfaceRenderer->secondaryTextureScrollX += -0.005f;
        surfaceRenderer->secondaryTextureScrollY += (-0.007499999832361937 + (0.001 * surfaceRenderer->pathDistanceOverflow));
        if (fabs(surfaceRenderer->secondaryTextureScrollX) > 1.0) {
            surfaceRenderer->secondaryTextureScrollX = fmod(surfaceRenderer->secondaryTextureScrollX, 1.0);
        }
        if (surfaceRenderer->secondaryTextureScrollX < 0.0f) {
            surfaceRenderer->secondaryTextureScrollX += 1.0f;
        }
        if (fabs(surfaceRenderer->secondaryTextureScrollY) > 1.0) {
            surfaceRenderer->secondaryTextureScrollY = fmod(surfaceRenderer->secondaryTextureScrollY, 1.0);
        }
        if (surfaceRenderer->secondaryTextureScrollY < 0.0f) {
            surfaceRenderer->secondaryTextureScrollY += 1.0f;
        }
    }
    positionVertex = surfaceRenderer->particlePositions;
    normalVertex = surfaceRenderer->particleNormals;
    textureVertex = surfaceRenderer->particleTextureCoordinates;
    colorVertex = surfaceRenderer->particleColors;
    surfaceVertex = surfaceRenderer->particleSurfaceData;
    primaryTextureScale.x = surfaceRenderer->primaryColorRed - 1;
    primaryTextureScale.y = surfaceRenderer->primaryColorGreen - 1;
    secondaryTextureScale.x = surfaceRenderer->secondaryColorRed - 1;
    secondaryTextureScale.y = surfaceRenderer->secondaryColorGreen - 1;
    for (elementIndex = 0; elementIndex < surfaceRenderer->particleVertexCount; elementIndex++) {
        *positionVertex = surfaceVertex->basePosition;
        if (surfaceVertex->isActive || surfaceVertex->isFirstRow) {
            textureX = (s32)(primaryTextureScale.x * (surfaceVertex->textureCoordinateU + surfaceRenderer->primaryTextureScrollX)) & 0x7F;
            textureY = (s32)(primaryTextureScale.y * (surfaceVertex->textureCoordinateV + surfaceRenderer->primaryTextureScrollY)) & 0x7F;
            effectAngle = surfaceRenderer->particleTextureData[(textureY * surfaceRenderer->primaryColorRed) + textureX];
            textureX = (s32)(secondaryTextureScale.x * (surfaceVertex->textureCoordinateU + surfaceRenderer->secondaryTextureScrollX)) & 0x7F;
            textureY = (s32)(secondaryTextureScale.y * (surfaceVertex->textureCoordinateV + surfaceRenderer->secondaryTextureScrollY)) & 0x7F;
            effectAngle += surfaceRenderer->secondaryTextureData[(textureY * surfaceRenderer->secondaryColorRed) + textureX];
            if (surfaceVertex->isActive) {
                positionVertex->y = surfaceVertex->basePosition.y + surfaceVertex->verticalOffset + (100.0f * (0.003921569f * (surfaceVertex->edgeFade * effectAngle)));
            }
            if (surfaceVertex->isFirstRow) {
                positionVertex->z = surfaceVertex->basePosition.z - (50.0f * (0.003921569f * (surfaceVertex->edgeFadeScale * effectAngle)));
            }
            (void)effectAngle;
            (void)effectAngle;
            (void)effectAngle;
            (void)effectAngle;
        }
        positionVertex++;
        normalVertex++;
        textureVertex++;
        colorVertex++;
        surfaceVertex++;
    }
    if (omPauseChk()) {
        return;
    }
    for (elementIndex = 0; elementIndex < 5; elementIndex++) {
        randomValue = (u32)frand() % 34;
        InterpolateVector(0.007874016f * (frand() & 0x7F), &surfaceRenderer->particlePositions[randomValue], &surfaceRenderer->particlePositions[randomValue + 1], &spawnPosition);
        spawnPosition.x += 10.0f * surfaceRenderer->particleSurfaceData->positionVelocity.x;
        spawnPosition.z += 10.0f * surfaceRenderer->particleSurfaceData->positionVelocity.z;
        spawnVelocity.y = 5.0f + (5.0f * (0.007874016f * (frand() & 0x7F)));
        spawnVelocity.x = spawnVelocity.z = 0.0f;
        randomValue = (frand() & 0x1F) + 0xE0;
        particleColor.r = 0.96f * randomValue;
        particleColor.g = 0.98f * randomValue;
        particleColor.b = (Float)randomValue;
        particleColor.a = 0xC8;
        CreateParticleEffect(spawnPosition, spawnVelocity, 100.0f + (50.0f * (0.007874016f * (frand() & 0x7F))), -0.1f, particleColor);
    }
    pathSample = surfaceRenderer->particlePathSamples;
#if defined(NON_MATCHING)
    memset(&sharedParticleEffect, 0, sizeof(MapParticleEffect));
#else
    memset(sharedParticleEffect, 0, sizeof(MapParticleEffect));
#endif
    for (elementIndex = 0; elementIndex < 105; elementIndex++, pathSample++) {
        InterpolateVector(pathSample->pathSampleFraction, &surfaceRenderer->particlePositions[pathSample->pathSampleIndex], &surfaceRenderer->particlePositions[pathSample->pathSampleIndex + 1], &pathSample->particlePosition);
        if (pathSample->effectSpawnCountdown >= 0) {
            pathSample->effectSpawnCountdown--;
        }
        else {
            pathSample->particleObject = AcquireParticleSlot(pathSample->effectType);
            if (pathSample->particleObject == NULL) {
                pathSample->particleObject = &sharedParticleEffect;
            }
            if (pathSample->particleObject->effectSize <= 0.0f) {
                spawnPosition = pathSample->particlePosition;
                spawnVelocity.x = spawnVelocity.y = spawnVelocity.z = 0.0f;
                randomValue = (frand() & 0x1F) + 0xE0;
                particleColor.r = 0.96f * randomValue;
                particleColor.g = 0.98f * randomValue;
                particleColor.b = 1.0f * randomValue;
                particleColor.a = 0xFF;
                if (frand() & 1) {
                    effectAngle = 0.05f;
                }
                else {
                    effectAngle = -0.05f;
                }
                pathSample->verticalVelocity = 0.01f * (100.0f * (0.007874016f * (frand() & 0x7F)));
                pathSample->effectType = CreateParticleEffectVariant(spawnPosition, spawnVelocity, 75.0f + (pathSample->particleScale * (75.0f * (0.007874016f * (frand() & 0x7F)))), effectAngle, particleColor);
            }
            else {
                pathSample->particlePosition.y += pathSample->verticalVelocity;
                pathSample->particleObject->position = pathSample->particlePosition;
            }
        }
    }
    particleColor.r = particleColor.g = particleColor.b = particleColor.a = 0xFF;
    InterpolateVector(0.007874016f * (frand() & 0x7F), &surfaceRenderer->particlePositions[0], &surfaceRenderer->particlePositions[34], &spawnPosition);
    if ((frand() & 3) == 0) {
        effectAngle = 45.0f + (30.0f * (0.007874016f * (frand() & 0x7F)));
        effectScale = 0.75f + (0.75f * (0.007874016f * (frand() & 0x7F)));
    }
    else {
        effectAngle = -(15.0f * (0.007874016f * (frand() & 0x7F)));
        effectScale = 1.75f + (0.75f * (0.007874016f * (frand() & 0x7F)));
    }
    if (surfaceRenderer->pathDistanceSpeed != 0.0f) {
        effectSpeed = 15.000001f / surfaceRenderer->pathDistanceSpeed;
    }
    else {
        effectSpeed = 1.0f;
    }
    spawnVelocity.y = effectSpeed * ((7.5000005f * effectScale) * sind(effectAngle));
    effectSpeed = (0.75f * surfaceRenderer->pathDistanceSpeed) + (7.5000005f * effectScale) * cosd(effectAngle);
    effectSpeed = effectSpeed;
    spawnVelocity.x = surfaceRenderer->particleSurfaceData->positionVelocity.x * effectSpeed;
    spawnVelocity.z = surfaceRenderer->particleSurfaceData->positionVelocity.z * effectSpeed;

    CreateParticleEffectSmall(spawnPosition, spawnVelocity, 7.5f, particleColor);
    particleModel = surfaceRenderer->particleModels;
    for (elementIndex = 0; elementIndex < 12; elementIndex++, particleModel++) {
        if (particleModel->effectFrame == 0) {
            if (particleModel->frameDelay <= 0) {
                effectSpeed = 0.5f + (0.75f * (0.007874016f * (frand() & 0x7F)));
                Hu3DModelScaleSet(particleModel->modelId, effectSpeed, effectSpeed, effectSpeed);
                particleModel->effectState = frand() & 1;
                switch (particleModel->effectState) {
                    case 0:
                        particleModel->scaleFactor = 0.1f + (0.8f * (0.007874016f * (frand() & 0x7F)));
                        particleModel->rotationAngle = particleModel->rotationVelocity = 0.0f;
                        particleModel->rotationX = particleModel->rotationY = 0.0f;
                        particleModel->effectFrame = 0;
                        particleModel->frameDelay = 0;
                        particleModel->opacity = 100.0f * (0.5f + (0.75f * (0.007874016f * (frand() & 0x7F))));
                        particleModel->effectWidth = 0.0f;
                        particleModel->effectHeight = 15.0f + (15.0f * (0.007874016f * (frand() & 0x7F)));
                        break;
                    case 1:
                        particleModel->scaleFactor = 0.1f + (0.8f * (0.007874016f * (frand() & 0x7F)));
                        particleModel->rotationAngle = particleModel->rotationVelocity = 0.0f;
                        particleModel->rotationX = particleModel->rotationY = 0.0f;
                        particleModel->effectFrame = 0;
                        particleModel->frameDelay = 0;
                        particleModel->opacity = 100.0f * (0.5f + (0.007874016f * (frand() & 0x7F)));
                        particleModel->effectWidth = 100.0f * (0.07f + (0.03f * (0.007874016f * (frand() & 0x7F))));
                        particleModel->effectHeight = 15.0f + (15.0f * (0.007874016f * (frand() & 0x7F)));
                        break;
                    default:
                        break;
                }
            }
            else {
                Hu3DModelAttrSet(particleModel->modelId, HU3D_ATTR_DISPOFF);
                particleModel->effectFrame = -1;
                particleModel->frameDelay--;
            }
        }
        else if (particleModel->effectFrame >= 0) {
            pathSegmentIndex = GetPathSegmentAtDistance(surfaceRenderer->currentPathDistance + particleModel->rotationVelocity, &pathSampleFraction);
            if (pathSegmentIndex >= 0 && pathSegmentIndex < (pathSegmentCount - 1)) {
                InterpolateVector(pathSampleFraction, &leftSurfacePath[pathSegmentIndex], &leftSurfacePath[pathSegmentIndex + 1], &pathStart);
                InterpolateVector(pathSampleFraction, &rightSurfacePath[pathSegmentIndex], &rightSurfacePath[pathSegmentIndex + 1], &pathEnd);
                InterpolateVector(particleModel->scaleFactor, &pathStart, &pathEnd, &particleModel->modelVelocity);
                Hu3DModelAttrReset(particleModel->modelId, HU3D_ATTR_DISPOFF);
            }
            else {
                Hu3DModelAttrSet(particleModel->modelId, HU3D_ATTR_DISPOFF);
                particleModel->effectFrame = -1;
                continue;
            }

            switch (particleModel->effectState) {
                case 0:
                    particleModel->rotationVelocity = particleModel->opacity * sind(5.0f * particleModel->effectFrame);
                    particleModel->rotationX = particleModel->rotationX - particleModel->effectHeight;
                    Hu3DModelPosSet(particleModel->modelId, particleModel->modelVelocity.x, particleModel->modelVelocity.y, particleModel->modelVelocity.z);
                    Hu3DModelRotSet(particleModel->modelId, particleModel->rotationX, particleModel->rotationY, 0.0f);
                    break;
                case 1:
                    particleModel->rotationVelocity = particleModel->opacity * sind((Float)particleModel->effectFrame);
                    particleModel->rotationX = particleModel->rotationX - particleModel->effectHeight;
                    particleModel->rotationAngle = particleModel->rotationAngle + particleModel->effectWidth;
                    if (particleModel->rotationAngle < 0.0f) {
                        effectSpeed = 0.65 + (0.15 * (0.007874016f * (frand() & 0x7F)));
                        effectSpeed = effectSpeed;
                        particleModel->rotationAngle = 0.0f;
                        particleModel->effectWidth = particleModel->effectWidth * -effectSpeed;
                    }
                    particleModel->effectWidth = particleModel->effectWidth - 0.32666668f;
                    particleModel->modelVelocity.y += particleModel->rotationAngle;
                    Hu3DModelPosSet(particleModel->modelId, particleModel->modelVelocity.x, particleModel->modelVelocity.y, particleModel->modelVelocity.z);
                    Hu3DModelRotSet(particleModel->modelId, particleModel->rotationX, particleModel->rotationY, 0.0f);
                    break;
                default:
                    break;
            }
        }
        if (particleModel->rotationVelocity < (-0.1f * particleModel->opacity)) {
            particleModel->effectFrame = -1;
            particleModel->frameDelay = frand() & 0x1F;
        }
        particleModel->effectFrame++;
    }
    surfaceRenderer->rendererUpdateCount++;
}

void UpdateSurfaceGeometry(HU3DMODEL *source)
{
    Vec faceNormal;
    Vec edgeVectorA;
    Vec edgeVectorB;

    MapSurfaceRenderer *surfaceRenderer;
    s16(*triangleIndices)[4];
    s32 vertexIndex;
    Vec *normalVertex;

    surfaceRenderer = source->hookData;
    memset(surfaceRenderer->particleNormals, 0, surfaceRenderer->particleVertexCount * sizeof(Vec));
    for (vertexIndex = 0, triangleIndices = surfaceRenderer->particleTriangleIndices; vertexIndex < surfaceRenderer->displayListIndexCount; vertexIndex++, triangleIndices++) {
        VECSubtract(&surfaceRenderer->particlePositions[(*triangleIndices)[1]], &surfaceRenderer->particlePositions[(*triangleIndices)[0]], &edgeVectorA);
        VECSubtract(&surfaceRenderer->particlePositions[(*triangleIndices)[2]], &surfaceRenderer->particlePositions[(*triangleIndices)[1]], &edgeVectorB);
        VECCrossProduct(&edgeVectorA, &edgeVectorB, &faceNormal);

        VECAdd(&surfaceRenderer->particleNormals[(*triangleIndices)[0]], &faceNormal, &surfaceRenderer->particleNormals[(*triangleIndices)[0]]);
        VECAdd(&surfaceRenderer->particleNormals[(*triangleIndices)[1]], &faceNormal, &surfaceRenderer->particleNormals[(*triangleIndices)[1]]);
        VECAdd(&surfaceRenderer->particleNormals[(*triangleIndices)[2]], &faceNormal, &surfaceRenderer->particleNormals[(*triangleIndices)[2]]);
        VECAdd(&surfaceRenderer->particleNormals[(*triangleIndices)[3]], &faceNormal, &surfaceRenderer->particleNormals[(*triangleIndices)[3]]);
    }
    for (vertexIndex = 0, normalVertex = surfaceRenderer->particleNormals; vertexIndex < surfaceRenderer->particleVertexCount; vertexIndex++) {
        VECNormalize(normalVertex, normalVertex);
    }
}

void DrawMapScene(HU3DMODEL *source, Mtx destination)
{
    MapSurfaceRenderer *surfaceRenderer;
    HU3DMODEL *cameraModel;

    surfaceRenderer = source->hookData;
    cameraModel = &Hu3DData[surfaceRenderer->cameraHookModelId];
    if (!omPauseChk() && sceneLightIds[0]) {
        UpdateMapAmbience();
        UpdateSparkleEffects();
        UpdateParticleEffects();
        UpdateParticlePool();
        UpdateParticleModelPool();
        UpdateCameraTransitionEffects();
    }
    UpdateSurfaceParticles(cameraModel);
    UpdateSurfaceGeometry(cameraModel);
    surfaceRenderer->useDefaultDrawHook = 1;
}

#ifdef NON_MATCHING
void CalculatePathSegmentGeometry(MapPathSegment *source, Float cameraPitchScale)
{
#else
void CalculatePathSegmentGeometry(MapPathSegment *source)
{
    Float pathDistance; // ! - uninitialized
#endif

    MapPathSegment *pathSegment;
    s32 segmentIndex;
    MapPathSegment *nextSegment;

    for (segmentIndex = 0, pathSegment = source; segmentIndex < pathSegmentCount; segmentIndex++, pathSegment++) {
        pathSegment->segmentMidpoint.x = 0.5f * (leftSurfacePath[segmentIndex].x + rightSurfacePath[segmentIndex].x);
        pathSegment->segmentMidpoint.y = 0.5f * (leftSurfacePath[segmentIndex].y + rightSurfacePath[segmentIndex].y);
        pathSegment->segmentMidpoint.z = 0.5f * (leftSurfacePath[segmentIndex].z + rightSurfacePath[segmentIndex].z);
    }
    for (segmentIndex = 0, pathSegment = source; segmentIndex < pathSegmentCount; segmentIndex++, pathSegment++) {
        if (segmentIndex < pathSegmentCount - 1) {
            nextSegment = &source[segmentIndex + 1];
            VECSubtract(&nextSegment->segmentMidpoint, &pathSegment->segmentMidpoint, &pathSegment->segmentDirection);
            pathSegment->segmentLength = VECMag(&pathSegment->segmentDirection);
            VECNormalize(&pathSegment->segmentDirection, &pathSegment->segmentDirection);
        }
        else {
            pathSegment->segmentLength = 100000.0f;
            pathSegment->segmentDirection.x = 0.0f;
            pathSegment->segmentDirection.y = 0.0f;
            pathSegment->segmentDirection.z = -1.0f;
        }
        pathSegment->segmentStartDistance = pathDistance;
        pathDistance += pathSegment->segmentLength;
    }
}

s32 GetPathSegmentAtDistance(Float scale, Float *source)
{
    MapPathSegment *pathSegment;
    s32 segmentIndex;

    for (segmentIndex = 0, pathSegment = pathSegments; segmentIndex < pathSegmentCount; segmentIndex++, pathSegment++) {
        if (scale < (pathSegment->segmentStartDistance + pathSegment->segmentLength)) {
            *source = (scale - pathSegment->segmentStartDistance) / pathSegment->segmentLength;
            if (*source < 0.0f) {
                *source = 0.0f;
            }
            break;
        }
    }
    if (segmentIndex >= pathSegmentCount) {
        if (source) {
            *source = 0.0f;
        }
        return -1;
    }
    return segmentIndex;
}

void UpdateSurfaceParticleModels(HU3DMODEL *source)
{
    SurfaceParticleModel *particleModel;
    s32 modelIndex;
    MapSurfaceRenderer *surfaceRenderer;

    surfaceRenderer = source->hookData;
    particleModel = surfaceRenderer->particleModels;
    for (modelIndex = 0; modelIndex < 12; modelIndex++, particleModel++) {
        if (particleModel->effectState != 3 || particleModel->frameDelay > 0) {
            Hu3DModelAttrSet(particleModel->modelId, HU3D_ATTR_DISPOFF);
            particleModel->frameDelay--;
        }
        else {
            particleModel->rotationX = particleModel->rotationX - particleModel->effectHeight;
            particleModel->modelVelocity.x += particleModel->modelPosition.x;
            particleModel->modelVelocity.y += particleModel->modelPosition.y;
            particleModel->modelVelocity.z += particleModel->modelPosition.z;
            particleModel->modelPosition.y -= 0.49f;
            Hu3DModelPosSet(particleModel->modelId, particleModel->modelVelocity.x, particleModel->modelVelocity.y, particleModel->modelVelocity.z);
            Hu3DModelRotSet(particleModel->modelId, particleModel->rotationX, particleModel->rotationY, 0.0f);
            Hu3DModelAttrReset(particleModel->modelId, HU3D_ATTR_DISPOFF);
            if (++particleModel->effectFrame >= (REFRESH_RATE*10.0f)) {
                Hu3DModelAttrSet(particleModel->modelId, HU3D_ATTR_DISPOFF);
                particleModel->effectState = 0;
                particleModel->effectFrame = 0;
            }
        }
    }
}

void SetMapPathSpeed(Float scale)
{
    MapSurfaceRenderer *surfaceRenderer;
    HU3DMODEL *hookModel;

    if (surfaceHookModelId >= 0) {
        hookModel = &Hu3DData[surfaceHookModelId];
        surfaceRenderer = hookModel->hookData;
        surfaceRenderer->scaleFactor = scale;
    }
}

void GetMapPathOrigin(Vec *source)
{
    MapSurfaceRenderer *surfaceRenderer;
    HU3DMODEL *hookModel;

    source->x = source->y = source->z = 0.0f;
    if (surfaceHookModelId >= 0) {
        hookModel = &Hu3DData[surfaceHookModelId];
        surfaceRenderer = hookModel->hookData;
        *source = surfaceRenderer->particleSurfaceData[17].basePosition;
    }
}

s32 GetPathLateralOffset(Vec source)
{
    Vec startOffset;
    Vec endOffset;
    Vec pathMidpoint;

    Float signedDistance;

    MapSurfaceRenderer *surfaceRenderer;
    HU3DMODEL *hookModel;

    if (surfaceHookModelId < 0) {
        return 0;
    }
    hookModel = &Hu3DData[surfaceHookModelId];
    surfaceRenderer = hookModel->hookData;
    VECAdd(&surfaceRenderer->startPathPoint, &surfaceRenderer->endPathPoint, &pathMidpoint);
    VECScale(&pathMidpoint, &pathMidpoint, 0.5f);
    VECSubtract(&surfaceRenderer->startPathPoint, &pathMidpoint, &startOffset);
    VECSubtract(&surfaceRenderer->endPathPoint, &pathMidpoint, &endOffset);
    signedDistance = ProjectVectorOntoAxis(&startOffset, &endOffset, source.x - pathMidpoint.x, source.z - pathMidpoint.z);
    if (signedDistance <= 0.0f) {
        return 0;
    }
    return 1;
}

void StopMapPathAudio(void)
{
    MapSurfaceRenderer *surfaceRenderer;
    HU3DMODEL *hookModel;

    if (surfaceHookModelId >= 0) {
        hookModel = &Hu3DData[surfaceHookModelId];
        surfaceRenderer = hookModel->hookData;
        surfaceRenderer->effectWidth = 1;
    }
}

void SpawnPathParticles(Vec *source)
{
    Vec particlePosition;
    Vec particleVelocity;

    Float particleScale;

    s32 particleIndex;
    s32 particleId;

    for (particleIndex = 0; particleIndex < 32; particleIndex++) {
        particlePosition.x = source->x + (100.0f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
        particlePosition.y = source->y;
        particlePosition.z = source->z - 100.0f;
        particleVelocity.x = 2.0f * (0.5f - (0.007874016f * (frand() & 0x7F)));
        particleVelocity.z = -2.0f * (1.0f + (1.2f * (0.007874016f * (frand() & 0x7F))));
        particleVelocity.y = 7.5000005f * (1.0f + (0.7f * (0.007874016f * (frand() & 0x7F))));
        particleScale = 5.5f + (5.5f * (0.007874016f * (frand() & 0x7F)));

        particleId = CreateParticleEffectColored(particlePosition, particleVelocity, particleScale, particleOpaqueWhite);
        ConfigureParticleLifetime(particleId, (s32)(60.0f * (0.007874016f * (frand() & 0x7F))) + 60, 0.32666668f, 0.99f);
    }
}

void CreateSparkleEffect(void)
{
    s32 spriteModelId;

    SparkleEffectGroup *spriteEffect;
    ParticleEffectHookData *effectHookData;
    HU3DMODEL *effectModel;
    ANIMDATA *effectAnimation;

    spriteEffect = sparkleEffectGroup = HuMemDirectMallocNum(HEAP_HEAP, sizeof(SparkleEffectGroup), HU_MEMNUM_OVL);
    memset(spriteEffect, 0, sizeof(SparkleEffectGroup));
    effectAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M406, 63), HU_MEMNUM_OVL));
    spriteEffect->animationData = effectAnimation;
    spriteEffect->modelId = spriteModelId = CreateModelAnimation(effectAnimation, 0x800);
    Hu3DModelLayerSet(spriteEffect->modelId, 2);
    spriteEffect->activeEffectCount = 0;
    effectModel = &Hu3DData[spriteEffect->modelId];
    effectHookData = effectModel->hookData;
    effectHookData->hookMode = 0;
}

void UpdateSparkleEffects(void)
{
    MapParticleEffect *particleEffect;
    SparkleEffectGroup *effectManager;
    ParticleEffectHookData *effectHookData;
    s32 particleIndex;
    HU3DMODEL *effectModel;

    effectManager = sparkleEffectGroup;
    if (effectManager->activeEffectCount <= 0) {
        Hu3DModelAttrSet(effectManager->modelId, HU3D_ATTR_DISPOFF);
        return;
    }
    Hu3DModelAttrReset(effectManager->modelId, HU3D_ATTR_DISPOFF);
    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    particleEffect = effectHookData->particles;
    for (particleIndex = 0; particleIndex < effectHookData->particleCount; particleIndex++, particleEffect++) {
        if (particleEffect->effectSize <= 0.0f) {
            continue;
        }
        particleEffect->position.x += particleEffect->velocity.x;
        particleEffect->position.y += particleEffect->velocity.y;
        particleEffect->position.z += particleEffect->velocity.z;
        particleEffect->rotationAngle += particleEffect->rotationSpeed;
        if (particleEffect->rotationAngle >= 360.0f) {
            particleEffect->rotationAngle -= 360.0f;
        }
        particleEffect->textureFrameIndex = particleEffect->frameCount >> 1;
        particleEffect->frameCount++;
        if (particleEffect->textureFrameIndex >= 16) {
            particleEffect->textureFrameIndex = 0;
            particleEffect->frameCount = 0;
            particleEffect->effectSize = 0.0f;
            effectManager->activeEffectCount--;
        }
    }
}

void DestroySparkleEffects(void)
{
    SparkleEffectGroup *spriteEffect;

    spriteEffect = sparkleEffectGroup;
    Hu3DModelAttrSet(spriteEffect->modelId, HU3D_ATTR_DISPOFF);
    HuSprAnimKill(spriteEffect->animationData);
}

s32 CreateParticleEffect(Vec source, Vec destination, Float scale, Float rotationSpeed, GXColor effectColor)
{
    MapParticleEffect *particleEffect;
    ParticleEffectHookData *effectHookData;
    s32 particleIndex;
    SparkleEffectGroup *effectManager;
    HU3DMODEL *effectModel;

    effectManager = sparkleEffectGroup;
    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    particleEffect = effectHookData->particles;
    for (particleIndex = 0; particleIndex < effectHookData->particleCount; particleIndex++, particleEffect++) {
        if (particleEffect->effectSize <= 0.0f) {
            break;
        }
    }
    if (particleIndex >= effectHookData->particleCount) {
        return -1;
    }
    particleEffect->frameCount = particleEffect->frameLimit = 0;

    particleEffect->position.x = source.x;
    particleEffect->position.y = source.y;
    particleEffect->position.z = source.z;

    particleEffect->velocity.x = destination.x;
    particleEffect->velocity.y = destination.y;
    particleEffect->velocity.z = destination.z;

    particleEffect->rotationSpeed = rotationSpeed;
    particleEffect->effectSize = scale;

    particleEffect->color.r = effectColor.r;
    particleEffect->color.g = effectColor.g;
    particleEffect->color.b = effectColor.b;
    particleEffect->color.a = effectColor.a;

    particleEffect->rotationAngle = 0.0f;
    particleEffect->textureFrameIndex = 0;
    particleEffect->frameCount = 0;
    effectManager->activeEffectCount++;

    return particleIndex;
}

// TODO maybe pass struct by value
s32 CreateSplashParticles(Vec *source, Vec *destination, GXColor *effectColor, Float scale, Float rotationSpeed, Float secondScale)
{
    Vec particlePosition;
    Vec sidewaysVelocity;
    s32 firstParticleId;
    s32 secondParticleId;

    Float halfWidth;

    sidewaysVelocity.x = destination->z;
    sidewaysVelocity.z = destination->x;
    sidewaysVelocity.y = 0.0f;

    halfWidth = 0.5f * secondScale;

    particlePosition.x = source->x + (sidewaysVelocity.x * halfWidth);
    particlePosition.y = source->y + (sidewaysVelocity.y * halfWidth);
    particlePosition.z = source->z + (sidewaysVelocity.z * halfWidth);
    firstParticleId = CreateParticleEffect(particlePosition, *destination, scale, rotationSpeed, *effectColor);

    particlePosition.x = source->x - (sidewaysVelocity.x * halfWidth);
    particlePosition.y = source->y - (sidewaysVelocity.y * halfWidth);
    particlePosition.z = source->z - (sidewaysVelocity.z * halfWidth);

    secondParticleId = CreateParticleEffect(particlePosition, *destination, scale, -rotationSpeed, *effectColor);
    return (firstParticleId << 16) | secondParticleId;
}

MapParticleEffect *UpdateSurfaceParticlesSecondary(s32 source)
{
    ParticleEffectHookData *effectHookData;
    SparkleEffectGroup *effectManager;
    HU3DMODEL *effectModel;

    effectManager = sparkleEffectGroup;
    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    if (source < 0 || source >= effectHookData->particleCount) {
        return 0;
    }
    return &effectHookData->particles[source];
}

void CreateSparkleGroup(void)
{
    s32 spriteModelId;

    SparkleEffectGroup *spriteEffect;
    ParticleEffectHookData *effectHookData;
    HU3DMODEL *effectModel;
    ANIMDATA *effectAnimation;

    spriteEffect = particleEffectGroup = HuMemDirectMallocNum(HEAP_HEAP, sizeof(SparkleEffectGroup), HU_MEMNUM_OVL);
    memset(spriteEffect, 0, sizeof(SparkleEffectGroup));
    effectAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M406, 63), HU_MEMNUM_OVL));
    spriteEffect->animationData = effectAnimation;
    spriteEffect->modelId = spriteModelId = CreateModelAnimation(effectAnimation, 105);
    Hu3DModelLayerSet(spriteEffect->modelId, 2);
    spriteEffect->activeEffectCount = 0;
    effectModel = &Hu3DData[spriteEffect->modelId];
    effectHookData = effectModel->hookData;
    effectHookData->hookMode = 0;
}

void UpdateParticleEffects(void)
{
    MapParticleEffect *particleEffect;
    SparkleEffectGroup *effectManager;
    ParticleEffectHookData *effectHookData;
    s32 particleIndex;
    HU3DMODEL *effectModel;

    effectManager = particleEffectGroup;
    if (effectManager->activeEffectCount <= 0) {
        Hu3DModelAttrSet(effectManager->modelId, HU3D_ATTR_DISPOFF);
        return;
    }
    Hu3DModelAttrReset(effectManager->modelId, HU3D_ATTR_DISPOFF);
    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    particleEffect = effectHookData->particles;
    if (getSceneState() < 3) {
        effectHookData->effectFlags |= 1;
    }
    else {
        effectHookData->effectFlags &= 0xFFFFFFFE;
    }
    for (particleIndex = 0; particleIndex < effectHookData->particleCount; particleIndex++, particleEffect++) {
        if (particleEffect->effectSize <= 0.0f) {
            continue;
        }
        particleEffect->position.x += particleEffect->velocity.x;
        particleEffect->position.y += particleEffect->velocity.y;
        particleEffect->position.z += particleEffect->velocity.z;
        particleEffect->rotationAngle += particleEffect->rotationSpeed;
        if (particleEffect->rotationAngle >= 360.0f) {
            particleEffect->rotationAngle = particleEffect->rotationAngle - 360.0f;
        }
        particleEffect->textureFrameIndex = particleEffect->frameCount / 2;
        particleEffect->frameCount++;
        if (particleEffect->textureFrameIndex >= 0x10) {
            particleEffect->textureFrameIndex = 0;
            particleEffect->frameCount = 0;
            particleEffect->frameLimit = 0;
            particleEffect->effectSize = 0.0f;
            effectManager->activeEffectCount--;
        }
    }
}

void DestroyParticleEffects(void)
{
    SparkleEffectGroup *effectManager;

    effectManager = particleEffectGroup;
    Hu3DModelAttrSet(effectManager->modelId, HU3D_ATTR_DISPOFF);
    HuSprAnimKill(effectManager->animationData);
}

s32 CreateParticleEffectVariant(Vec source, Vec destination, Float scale, Float rotationSpeed, GXColor effectColor)
{
    MapParticleEffect *particleEffect;
    ParticleEffectHookData *effectHookData;
    s32 particleIndex;
    SparkleEffectGroup *effectManager;
    HU3DMODEL *effectModel;

    effectManager = particleEffectGroup;
    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    particleEffect = effectHookData->particles;
    for (particleIndex = 0; particleIndex < effectHookData->particleCount; particleIndex++, particleEffect++) {
        if (particleEffect->effectSize <= 0.0f) {
            break;
        }
    }
    if (particleIndex >= effectHookData->particleCount) {
        return -1;
    }
    particleEffect->frameCount = particleEffect->frameLimit = 0;

    particleEffect->position.x = source.x;
    particleEffect->position.y = source.y;
    particleEffect->position.z = source.z;

    particleEffect->velocity.x = destination.x;
    particleEffect->velocity.y = destination.y;
    particleEffect->velocity.z = destination.z;

    particleEffect->rotationSpeed = rotationSpeed;
    particleEffect->effectSize = scale;

    particleEffect->color.r = effectColor.r;
    particleEffect->color.g = effectColor.g;
    particleEffect->color.b = effectColor.b;
    particleEffect->color.a = effectColor.a;

    particleEffect->rotationAngle = 0.0f;
    particleEffect->textureFrameIndex = 0;
    particleEffect->frameCount = 0;
    effectManager->activeEffectCount++;

    return particleIndex;
}

MapParticleEffect *AcquireParticleSlot(s32 source)
{
    ParticleEffectHookData *effectHookData;
    SparkleEffectGroup *effectManager;
    HU3DMODEL *effectModel;

    effectManager = particleEffectGroup;
    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    if (source < 0 || source >= effectHookData->particleCount) {
        return NULL;
    }
    return &effectHookData->particles[source];
}

void CreateParticlePool(void)
{
    s32 hookModelId;

    ParticlePool *effectManager;
    ParticleEffectHookData *effectHookData;
    HU3DMODEL *effectModel;

    effectManager = particleModelPool = HuMemDirectMallocNum(HEAP_HEAP, sizeof(ParticlePool), HU_MEMNUM_OVL);
    memset(effectManager, 0, sizeof(ParticlePool));
    effectManager->modelId = hookModelId = UpdateTextureCoordinates(512);
    Hu3DModelLayerSet(effectManager->modelId, 3);
    effectManager->activeEffectCount = 0;
    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    effectHookData->hookMode = 0;
}

void UpdateParticlePool(void)
{
    MapParticleEffect *particleEffect;
    ParticlePool *effectManager;
    ParticleEffectHookData *effectHookData;
    s32 particleIndex;
    HU3DMODEL *effectModel;

    effectManager = particleModelPool;
    if (effectManager->activeEffectCount <= 0) {
        Hu3DModelAttrSet(effectManager->modelId, HU3D_ATTR_DISPOFF);
        return;
    }
    Hu3DModelAttrReset(effectManager->modelId, HU3D_ATTR_DISPOFF);
    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    particleEffect = effectHookData->particles;
    for (particleIndex = 0; particleIndex < effectHookData->particleCount; particleIndex++, particleEffect++) {
        if (particleEffect->effectSize <= 0.0f) {
            continue;
        }
        particleEffect->position.x += particleEffect->velocity.x;
        particleEffect->position.y += particleEffect->velocity.y;
        particleEffect->position.z += particleEffect->velocity.z;
        particleEffect->velocity.y -= 0.32666668f;
        if (++particleEffect->frameCount > 60 || particleEffect->position.y < particleEffect->verticalMotionParameter - 200.0f) {
            particleEffect->frameCount = 0;
            particleEffect->effectSize = 0.0f;
            effectManager->activeEffectCount--;
        }
    }
}

void DestroyParticlePool(void) { }

s32 CreateParticleEffectSmall(Vec source, Vec destination, Float scale, GXColor effectColor)
{
    MapParticleEffect *particleEffect;
    ParticleEffectHookData *effectHookData;
    s32 particleIndex;
    ParticlePool *effectManager;
    HU3DMODEL *effectModel;

    effectManager = particleModelPool;
    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    particleEffect = effectHookData->particles;
    for (particleIndex = 0; particleIndex < effectHookData->particleCount; particleIndex++, particleEffect++) {
        if (particleEffect->effectSize <= 0.0f) {
            break;
        }
    }
    if (particleIndex >= effectHookData->particleCount) {
        return -1;
    }
    particleEffect->modelId = particleEffect->frameLimit = 0;

    particleEffect->position.x = source.x;
    particleEffect->position.y = source.y;
    particleEffect->position.z = source.z;

    particleEffect->activeEffectCount.x = destination.x;
    particleEffect->activeEffectCount.y = destination.y;
    particleEffect->activeEffectCount.z = destination.z;

    particleEffect->verticalMotionParameter = source.y;
    particleEffect->effectSize = scale;
    particleEffect->particles = effectColor;
    particleEffect->modelId = 0;
    effectManager->activeEffectCount++;

    return particleIndex;
}

void CreateParticleModelPool(void)
{
    s32 hookModelId;

    ParticlePool *effectManager;
    ParticleEffectHookData *effectHookData;
    HU3DMODEL *effectModel;

    effectManager = particleInstancePool = HuMemDirectMallocNum(HEAP_HEAP, sizeof(ParticlePool), HU_MEMNUM_OVL);
    memset(effectManager, 0, sizeof(ParticlePool));
    effectManager->modelId = hookModelId = UpdateTextureCoordinates(512);
    Hu3DModelLayerSet(effectManager->modelId, 3);
    effectManager->activeEffectCount = 0;
    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    effectHookData->hookMode = 0;
}

void UpdateParticleModelPool(void)
{
    MapParticleEffect *particleEffect;
    ParticlePool *effectManager;
    ParticleEffectHookData *effectHookData;
    s32 particleIndex;
    HU3DMODEL *effectModel;

    effectManager = particleInstancePool;
    if (effectManager->activeEffectCount <= 0) {
        Hu3DModelAttrSet(effectManager->modelId, HU3D_ATTR_DISPOFF);
        return;
    }
    Hu3DModelAttrReset(effectManager->modelId, HU3D_ATTR_DISPOFF);
    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    particleEffect = effectHookData->particles;
    for (particleIndex = 0; particleIndex < effectHookData->particleCount; particleIndex++, particleEffect++) {
        if (particleEffect->effectSize <= 0.0f) {
            continue;
        }
        particleEffect->position.x += particleEffect->velocity.x;
        particleEffect->position.y += particleEffect->velocity.y;
        particleEffect->position.z += particleEffect->velocity.z;
        particleEffect->velocity.y -= particleEffect->verticalMotionParameter;
        if (particleEffect->scaleFactor != 0.0f) {
            particleEffect->effectSize *= particleEffect->scaleFactor;
        }
        if (++particleEffect->frameCount > particleEffect->frameLimit || particleEffect->velocity.y <= 0.0f) {
            particleEffect->frameCount = 0;
            particleEffect->effectSize = 0.0f;
            effectManager->activeEffectCount = effectManager->activeEffectCount - 1;
        }
    }
}

void DestroyParticleModelPool(void) { }

s32 CreateParticleEffectColored(Vec source, Vec destination, Float scale, GXColor effectColor)
{
    MapParticleEffect *particleEffect;
    ParticleEffectHookData *effectHookData;
    ParticlePool *effectManager;
    s32 particleIndex;

    HU3DMODEL *effectModel;

    effectManager = particleInstancePool;

    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    particleEffect = effectHookData->particles;
    for (particleIndex = 0; particleIndex < effectHookData->particleCount; particleIndex++, particleEffect++) {
        if (particleEffect->effectSize <= 0.0f) {

            break;
        }
    }

    if (particleIndex >= effectHookData->particleCount) {
        return -1;
    }
    particleEffect->modelId = particleEffect->frameLimit = 0;

    particleEffect->position.x = source.x;
    particleEffect->position.y = source.y;
    particleEffect->position.z = source.z;

    particleEffect->activeEffectCount.x = destination.x;
    particleEffect->activeEffectCount.y = destination.y;
    particleEffect->activeEffectCount.z = destination.z;

    particleEffect->verticalMotionParameter = 0.49f;
    particleEffect->elapsedTime = 0.0f;
    particleEffect->frameLimit = 30;

    particleEffect->effectSize = scale;

    particleEffect->particles.r = effectColor.r;
    particleEffect->particles.g = effectColor.g;
    particleEffect->particles.b = effectColor.b;
    particleEffect->particles.a = effectColor.a;

    particleEffect->modelId = 0;
    effectManager->activeEffectCount++;
    return particleIndex;
}

void ConfigureParticleLifetime(s32 source, s32 destination, Float scale, Float rotationSpeed)
{
    MapParticleEffect *particleEffect;
    ParticleEffectHookData *effectHookData;
    ParticlePool *effectManager;
    HU3DMODEL *effectModel;

    effectManager = particleInstancePool;
    effectModel = &Hu3DData[effectManager->modelId];
    effectHookData = effectModel->hookData;
    if (source < 0 || source >= effectHookData->particleCount) {
        return;
    }
    particleEffect = &effectHookData->particles[source];
    particleEffect->scaleFactor = rotationSpeed;
    particleEffect->verticalMotionParameter = scale;
    particleEffect->frameLimit = destination;
}

void SetModelEmissionTransform(Mtx source, s32 destination, Float scale)
{
    Mtx emissionRotation;
    Vec modelOrigin;
    Vec modelYaw;
    Vec firstEmissionEdge;
    Vec secondEmissionEdge;
    Vec particlePosition;
    Vec particleVelocity;

    Float particleSize;
    Float emissionForwardX;
    Float emissionForwardZ;

    s32 particleIndex;

    modelOrigin.x = source[0][3];
    modelOrigin.y = source[1][3];
    modelOrigin.z = source[2][3];
    modelYaw.x = modelYaw.z = 0.0f;
    modelYaw.y = atan2d(source[0][0], source[2][0]);
    MTXCopy(source, emissionRotation);
    emissionRotation[0][3] = emissionRotation[1][3] = emissionRotation[2][3] = emissionRotation[3][3] = 0.0f;
    firstEmissionEdge.x = -25.0f;
    firstEmissionEdge.z = -50.0f;
    firstEmissionEdge.y = 0.0f;
    TransformVector(emissionRotation, &firstEmissionEdge);
    firstEmissionEdge.x += modelOrigin.x;
    firstEmissionEdge.y += modelOrigin.y;
    firstEmissionEdge.z += modelOrigin.z;

    secondEmissionEdge.x = -25.0f;
    secondEmissionEdge.z = 50.0f;
    secondEmissionEdge.y = 0.0f;
    TransformVector(emissionRotation, &secondEmissionEdge);
    secondEmissionEdge.x += modelOrigin.x;
    secondEmissionEdge.y += modelOrigin.y;
    secondEmissionEdge.z += modelOrigin.z;
    mtxRotCat(emissionRotation, 0.0f, 180.0f + (90.0f - (45.0f * (0.007874016f * (frand() & 0x7F)))), 0.0f);
    particleTranslucentWhite.a = scale;
    emissionForwardX = emissionRotation[0][2];
    emissionForwardZ = emissionRotation[2][2];
    for (particleIndex = 0; particleIndex < (destination >> 1); particleIndex++) {
        InterpolateVector(0.007874016f * (frand() & 0x7F), &firstEmissionEdge, &secondEmissionEdge, &particlePosition);
        particlePosition.x += 100.0f * (0.1f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
        particlePosition.y += 100.0f * (0.1f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
        particlePosition.z += 100.0f * (0.1f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));

        particleVelocity.x = 2.0f * emissionForwardX * (1.0f + (0.007874016f * (frand() & 0x7F)));
        particleVelocity.z = 2.0f * emissionForwardZ * (1.0f + (0.007874016f * (frand() & 0x7F)));
        particleVelocity.y = 5.0f * (1.0f + (0.7f * (0.007874016f * (frand() & 0x7F))));
        particleSize = 5.0f + (15.0f * (0.007874016f * (frand() & 0x7F)));

        CreateParticleEffectColored(particlePosition, particleVelocity, particleSize, particleTranslucentWhite);
    }
    firstEmissionEdge.x = -25.0f;
    firstEmissionEdge.z = -50.0f;
    firstEmissionEdge.y = 0.0f;
    TransformVector(emissionRotation, &firstEmissionEdge);
    firstEmissionEdge.x += modelOrigin.x;
    firstEmissionEdge.y += modelOrigin.y;
    firstEmissionEdge.z = firstEmissionEdge.z + modelOrigin.z;
    secondEmissionEdge.x = -25.0f;
    secondEmissionEdge.z = 50.0f;
    secondEmissionEdge.y = 0.0f;
    TransformVector(emissionRotation, &secondEmissionEdge);
    secondEmissionEdge.x += modelOrigin.x;
    secondEmissionEdge.y += modelOrigin.y;
    secondEmissionEdge.z += modelOrigin.z;
    for (particleIndex = 0; particleIndex < (destination >> 1); particleIndex++) {
        InterpolateVector(0.007874016f * (frand() & 0x7F), &firstEmissionEdge, &secondEmissionEdge, &particlePosition);
        particlePosition.x += 100.0f * (0.1f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
        particlePosition.y += 100.0f * (0.1f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
        particlePosition.z += 100.0f * (0.1f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));

        particleVelocity.x = 2.0f * emissionForwardX * (1.0f + (0.007874016f * (frand() & 0x7F)));
        particleVelocity.z = 2.0f * emissionForwardZ * (1.0f + (0.007874016f * (frand() & 0x7F)));
        particleVelocity.y = 5.0f * (1.0f + (0.7f * (0.007874016f * (frand() & 0x7F))));
        particleSize = 5.0f + (15.0f * (0.007874016f * (frand() & 0x7F)));

        CreateParticleEffectColored(particlePosition, particleVelocity, particleSize, particleTranslucentWhite);
    }
}

void CreateCameraTransitionEffects(void)
{
    CameraTransitionEffect *transitionSlot;
    s32 slotIndex;

    transitionSlot = cameraTransitionEffects = HuMemDirectMallocNum(HEAP_HEAP, 128 * sizeof(CameraTransitionEffect), HU_MEMNUM_OVL);
    memset(cameraTransitionEffects, 0, 128 * sizeof(CameraTransitionEffect));
    for (slotIndex = 0, transitionSlot = cameraTransitionEffects; slotIndex < 64; slotIndex++, transitionSlot++) {
        transitionSlot->isActive = 0;
        transitionSlot->reservedFlag = 0;
        transitionSlot->frameCount = 0;
        transitionSlot->elapsedTime = 0.0f;
        transitionSlot->timeStep = 0.0f;
        transitionSlot->transitionDuration = 6.0f;
        transitionSlot->fadeCurveAmplitude = 1.0f;
        MTXIdentity(transitionSlot->transformMatrix);
        if (slotIndex == 0) {
            transitionSlot->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M406, 59));
        }
        else {
            transitionSlot->modelId = Hu3DModelLink(cameraTransitionEffects->modelId);
        }
        Hu3DModelLayerSet(transitionSlot->modelId, 3);
        Hu3DModelAttrSet(transitionSlot->modelId, HU3D_ATTR_DISPOFF);
    }

    for (slotIndex = 0, transitionSlot = &cameraTransitionEffects[64]; slotIndex < 64; slotIndex++, transitionSlot++) {
        transitionSlot->isActive = 0;
        transitionSlot->reservedFlag = 0;
        transitionSlot->frameCount = 0;
        transitionSlot->elapsedTime = 0.0f;
        transitionSlot->timeStep = 0.0f;
        transitionSlot->transitionDuration = 6.0f;
        transitionSlot->fadeCurveAmplitude = 1.0f;
        MTXIdentity(transitionSlot->transformMatrix);
        if (slotIndex == 0) {
            transitionSlot->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M406, 60));
        }
        else {
            transitionSlot->modelId = Hu3DModelLink(cameraTransitionEffects[64].modelId);
        }
        Hu3DModelLayerSet(transitionSlot->modelId, 3);
        Hu3DModelAttrSet(transitionSlot->modelId, HU3D_ATTR_DISPOFF);
    }
}

void UpdateCameraTransitionEffects(void)
{
    Mtx scaleMatrix;

    Float fadeScale;
    Float transitionProgress;

    CameraTransitionEffect *transitionSlot;
    HU3DMODEL *effectModel;
    s32 slotIndex;

    transitionSlot = cameraTransitionEffects;
    for (slotIndex = 0, transitionSlot = cameraTransitionEffects; slotIndex < 64; slotIndex++, transitionSlot++) {
        if (!transitionSlot->isActive) {
            continue;
        }
        transitionProgress = transitionSlot->elapsedTime / transitionSlot->transitionDuration;
        fadeScale = transitionSlot->fadeCurveAmplitude * cosd((90.0f * transitionProgress));
        fadeScale = fadeScale;
        effectModel = &Hu3DData[transitionSlot->modelId];
        MTXScale(scaleMatrix, fadeScale, fadeScale, fadeScale);
        MTXConcat(transitionSlot->transformMatrix, scaleMatrix, effectModel->mtx);
        effectModel->mtx[0][3] = transitionSlot->transformMatrix[0][3];
        effectModel->mtx[1][3] = transitionSlot->transformMatrix[1][3];
        effectModel->mtx[2][3] = transitionSlot->transformMatrix[2][3];
        Hu3DModelTPLvlSet(transitionSlot->modelId, 1.0f - transitionProgress);
        if ((transitionSlot->elapsedTime += transitionSlot->timeStep) >= transitionSlot->transitionDuration) {
            Hu3DModelAttrSet(transitionSlot->modelId, HU3D_ATTR_DISPOFF);
            transitionSlot->isActive = 0;
            transitionSlot->elapsedTime = 0.0f;
        }
        transitionSlot->frameCount++;
    }
    for (slotIndex = 0, transitionSlot = &cameraTransitionEffects[64]; slotIndex < 64; slotIndex++, transitionSlot++) {
        if (!transitionSlot->isActive) {
            continue;
        }
        transitionProgress = transitionSlot->elapsedTime / transitionSlot->transitionDuration;
        fadeScale = transitionSlot->fadeCurveAmplitude * cosd((90.0f * transitionProgress));
        fadeScale = fadeScale;
        effectModel = &Hu3DData[transitionSlot->modelId];
        MTXScale(scaleMatrix, fadeScale, fadeScale, fadeScale);
        MTXConcat(transitionSlot->transformMatrix, scaleMatrix, effectModel->mtx);
        effectModel->mtx[0][3] = transitionSlot->transformMatrix[0][3];
        effectModel->mtx[1][3] = transitionSlot->transformMatrix[1][3];
        effectModel->mtx[2][3] = transitionSlot->transformMatrix[2][3];
        Hu3DModelTPLvlSet(transitionSlot->modelId, 1.0f - transitionProgress);
        if ((transitionSlot->elapsedTime += transitionSlot->timeStep) >= transitionSlot->transitionDuration) {
            Hu3DModelAttrSet(transitionSlot->modelId, HU3D_ATTR_DISPOFF);
            transitionSlot->isActive = 0;
            transitionSlot->elapsedTime = 0.0f;
        }
        transitionSlot->frameCount++;
    }
}

void DestroyCameraTransitionEffects(void) { }

s32 SetEffectTransform(Mtx source, s32 destination, s32 effectColor)
{
    Mtx emissionRotation;
    Mtx particleScaleMatrix;
    Vec modelPosition;
    Vec modelYaw;
    Vec firstEdgePosition;
    Vec secondEdgePosition;
    Vec particlePosition;
    Vec particleVelocity;

    Float particleScale;
    Float emissionForwardX;
    Float emissionForwardZ;

    CameraTransitionEffect *transitionSlot;
    s32 particleIndex;
    HU3DMODEL *particleModel;

    modelPosition.x = source[0][3];
    modelPosition.y = source[1][3];
    modelPosition.z = source[2][3];
    modelYaw.x = modelYaw.z = 0.0f;
    modelYaw.y = atan2d(source[0][0], source[2][0]);
    MTXCopy(source, emissionRotation);
#ifdef NON_MATCHING
    emissionRotation[0][3] = emissionRotation[1][3] = emissionRotation[2][3] = 0.0f;
#else
    emissionRotation[0][3] = emissionRotation[1][3] = emissionRotation[2][3] = emissionRotation[3][3] = 0.0f;
#endif

    if (!effectColor) {
        firstEdgePosition.x = 25.0f;
        firstEdgePosition.z = -50.0f;
        firstEdgePosition.y = 0.0f;
        TransformVector(emissionRotation, &firstEdgePosition);
        firstEdgePosition.x += modelPosition.x;
        firstEdgePosition.y += modelPosition.y;
        firstEdgePosition.z += modelPosition.z;
        secondEdgePosition.x = 25.0f;
        secondEdgePosition.z = 50.0f;
        secondEdgePosition.y = 0.0f;
        TransformVector(emissionRotation, &secondEdgePosition);
        secondEdgePosition.x += modelPosition.x;
        secondEdgePosition.y += modelPosition.y;
        secondEdgePosition.z += modelPosition.z;
        mtxRotCat(emissionRotation, 0.0f, 180.0f + (45.0f * (0.007874016f * (frand() & 0x7F))), 0.0f);
    }
    else {
        firstEdgePosition = modelPosition;
        firstEdgePosition.x = -25.0f;
        firstEdgePosition.z = -50.0f;
        firstEdgePosition.y = 0.0f;
        TransformVector(emissionRotation, &firstEdgePosition);
        firstEdgePosition.x += modelPosition.x;
        firstEdgePosition.y += modelPosition.y;
        firstEdgePosition.z += modelPosition.z;
        secondEdgePosition = modelPosition;
        secondEdgePosition.x = -25.0f;
        secondEdgePosition.z = 50.0f;
        secondEdgePosition.y = 0.0f;
        TransformVector(emissionRotation, &secondEdgePosition);
        secondEdgePosition.x += modelPosition.x;
        secondEdgePosition.y += modelPosition.y;
        secondEdgePosition.z += modelPosition.z;
        mtxRotCat(emissionRotation, 0.0f, 180.0f - (45.0f * (0.007874016f * (frand() & 0x7F))), 0.0f);
    }
    emissionForwardX = emissionRotation[0][2];
    emissionForwardZ = emissionRotation[2][2];
    for (particleIndex = 0; particleIndex < destination; particleIndex++) {
        InterpolateVector(0.007874016f * (frand() & 0x7F), &firstEdgePosition, &secondEdgePosition, &particlePosition);
        particlePosition.x += 100.0f * (0.1f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
        particlePosition.y += 100.0f * (0.1f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
        particlePosition.z += 100.0f * (0.1f * ((0.007874016f * (frand() & 0x7F)) - 0.5f));
        particleVelocity.x = 2.0f * emissionForwardX * (1.0f + (0.007874016f * (frand() & 0x7F)));
        particleVelocity.z = 2.0f * emissionForwardZ * (1.0f + (0.007874016f * (frand() & 0x7F)));
        particleVelocity.y = 5.0f * (1.0f + (0.7f * (0.007874016f * (frand() & 0x7F))));

        CreateParticleEffectColored(particlePosition, particleVelocity, 7.5f, particleFadeWhite);
    }
    transitionSlot = cameraTransitionEffects;
    if (!effectColor) {
        for (particleIndex = 0, transitionSlot = cameraTransitionEffects; particleIndex < 64; particleIndex++, transitionSlot++) {
            if (!transitionSlot->isActive) {
                break;
            }
        }
        if (particleIndex >= 64) {
            return -1;
        }
    }
    else {
        for (particleIndex = 0, transitionSlot = &cameraTransitionEffects[64]; particleIndex < 64; particleIndex++, transitionSlot++) {
            if (!transitionSlot->isActive) {
                break;
            }
        }
        if (particleIndex >= 64) {
            return -1;
        }
    }
    particleScale = 1.2f * (2.0f + (0.007874016f * (frand() & 0x7F)));
    transitionSlot->isActive = 1;
    transitionSlot->frameCount = 0;
    transitionSlot->elapsedTime = 0.0f;
    transitionSlot->timeStep = 1.0f;
    transitionSlot->transitionDuration = 6.0f;
    transitionSlot->fadeCurveAmplitude = particleScale;
    MTXCopy(source, transitionSlot->transformMatrix);
    Hu3DMotionTimeSet(transitionSlot->modelId, 0.0f);
    Hu3DMotionSpeedSet(transitionSlot->modelId, 0.0f);
    particleModel = &Hu3DData[transitionSlot->modelId];
    MTXScale(particleScaleMatrix, particleScale, particleScale, particleScale);
    MTXConcat(source, particleScaleMatrix, particleModel->mtx);
    particleModel->mtx[0][3] = source[0][3];
    particleModel->mtx[1][3] = source[1][3];
    particleModel->mtx[2][3] = source[2][3];
    Hu3DModelPosSet(transitionSlot->modelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelRotSet(transitionSlot->modelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(transitionSlot->modelId, 1.0f, 1.0f, 1.0f);
    Hu3DModelAttrReset(transitionSlot->modelId, HU3D_ATTR_DISPOFF);
    (void)transitionSlot;
    return particleIndex;
}

void CreateSceneAnimationGroup(void)
{
    ParticleEffectHookData *modelHookData;
    SceneAnimationGroup *animationManager;
    HU3DMODEL *effectModel;
    ParticleEffectSettings *effectSettings;
    s32 effectModelId;

    animationManager = sceneAnimationGroup = HuMemDirectMallocNum(HEAP_HEAP, sizeof(SceneAnimationGroup), HU_MEMNUM_OVL);
    memset(animationManager, 0, sizeof(SceneAnimationGroup));
    animationManager->animation0 = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M406, 0x40), HU_MEMNUM_OVL));
    animationManager->animation1 = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M406, 0x41), HU_MEMNUM_OVL));
    animationManager->animation2 = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M406, 0x42), HU_MEMNUM_OVL));
    animationManager->animation3 = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M406, 0x43), HU_MEMNUM_OVL));
    animationManager->modelGroupEnabled = 0.0f;
    animationManager->overlayAlpha = 0.0f;
    effectModelId = CreateModelAnimation(animationManager->animation0, 1);
    animationManager->modelId0 = effectModelId;
    Hu3DModelLayerSet(effectModelId, 6);
    effectModel = &Hu3DData[effectModelId];
    modelHookData = effectModel->hookData;
    modelHookData->hookMode = 1;
    modelHookData->effectFlags = 2;
    modelHookData->DrawHook = DrawSceneAnimationGroup;
    effectSettings = modelHookData->effectSettings = HuMemDirectMallocNum(HEAP_MODEL, sizeof(ParticleEffectSettings), effectModel->mallocNo);
    memset(effectSettings, 0, 8);
    effectSettings->effectVariant = 0;
    effectModelId = CreateModelAnimation(animationManager->animation3, 0xA);
    animationManager->modelId1 = effectModelId;
    Hu3DModelLayerSet(effectModelId, 6);
    effectModel = &Hu3DData[effectModelId];
    modelHookData = effectModel->hookData;
    modelHookData->hookMode = 1;
    modelHookData->effectFlags = 3;
    modelHookData->DrawHook = DrawSceneAnimationGroup;
    effectSettings = modelHookData->effectSettings = HuMemDirectMallocNum(HEAP_MODEL, sizeof(ParticleEffectSettings), effectModel->mallocNo);
    memset(effectSettings, 0, 8);
    effectSettings->effectVariant = 1;
    effectModelId = CreateModelAnimation(animationManager->animation2, 4);
    animationManager->modelId2 = effectModelId;
    Hu3DModelLayerSet(effectModelId, 6);
    effectModel = &Hu3DData[effectModelId];
    modelHookData = effectModel->hookData;
    modelHookData->hookMode = 1;
    modelHookData->effectFlags = 3;
    modelHookData->DrawHook = DrawSceneAnimationGroup;
    effectSettings = modelHookData->effectSettings = HuMemDirectMallocNum(HEAP_MODEL, sizeof(ParticleEffectSettings), effectModel->mallocNo);
    memset(effectSettings, 0, 8);
    effectSettings->effectVariant = 2;
    effectModelId = CreateModelAnimation(animationManager->animation1, 6);
    animationManager->modelId3 = effectModelId;
    Hu3DModelLayerSet(effectModelId, 6);
    effectModel = &Hu3DData[effectModelId];
    modelHookData = effectModel->hookData;
    modelHookData->hookMode = 1;
    modelHookData->effectFlags = 3;
    modelHookData->DrawHook = DrawSceneAnimationGroup;
    effectSettings = modelHookData->effectSettings = HuMemDirectMallocNum(HEAP_MODEL, sizeof(ParticleEffectSettings), effectModel->mallocNo);
    memset(effectSettings, 0, 8);
    effectSettings->effectVariant = 3;
    animationManager->updateHookId = Hu3DHookFuncCreate(SetOrthographicProjection);
    Hu3DModelLayerSet(animationManager->updateHookId, 7);
}

void DestroySceneAnimationGroup(void)
{
    SceneAnimationGroup *animationManager;

    animationManager = sceneAnimationGroup;
    HuSprAnimKill(animationManager->animation0);
    HuSprAnimKill(animationManager->animation1);
    HuSprAnimKill(animationManager->animation2);
    HuSprAnimKill(animationManager->animation3);
}

void DrawSceneAnimationGroup(HU3DMODEL *source, ParticleEffectHookData *destination, Mtx effectColor)
{
    Vec cameraDirection;
    Vec cameraRelativeDirection;
    Vec reflectedDirection;
    Vec initialParticleDirection;
    void *animationManagerData;
    s16 callbackState;

    Float vectorScale;
    Float initialDirectionLengthSquared;
    Float cameraDirectionLengthSquared;
    Float particleDistance;
    Float normalizedParticleDistance;
    Float particleSpacing;

    MapParticleEffect *particleEffect;
    ParticleEffectSettings *effectSettings;
    s32 particleIndex;

    callbackState = 0;
    animationManagerData = sceneAnimationGroup;
    effectSettings = destination->effectSettings;
    switch (effectSettings->animationStep) {
        case 0:
            particleEffect = destination->particles;
            if (effectSettings->effectVariant) {
                initialParticleDirection.x = 10000.0f;
                initialParticleDirection.y = -20000.0f;
                initialParticleDirection.z = -20000.0f;
                initialDirectionLengthSquared = (initialParticleDirection.z * initialParticleDirection.z) + ((initialParticleDirection.x * initialParticleDirection.x) + (initialParticleDirection.y * initialParticleDirection.y));
                vectorScale = 1.0f / sqrtf(initialDirectionLengthSquared);
                initialParticleDirection.x *= vectorScale;
                initialParticleDirection.y *= vectorScale;
                initialParticleDirection.z *= vectorScale;
                particleSpacing = 6500.0f / destination->particleCount;
                for (particleIndex = 0; particleIndex < destination->particleCount; particleIndex++, particleEffect++) {
                    particleDistance = 500.0 + ((particleSpacing * particleIndex) + ((0.25f * particleSpacing) * (1.52587890625e-05 * (Float)(frand() & 0xFFFF))));
                    normalizedParticleDistance = particleDistance / 6500.0f;
                    particleEffect->scaleFactor = particleDistance;
                    particleEffect->position.x = particleDistance * initialParticleDirection.x;
                    particleEffect->position.y = -27000.0f + (particleDistance * initialParticleDirection.y);
                    particleEffect->position.z = -63000.0f + (particleDistance * initialParticleDirection.z);
                    particleEffect->effectSize = (0.3f + (0.7f * (normalizedParticleDistance * normalizedParticleDistance)))
                        * (particleLifetimeValues[effectSettings->effectVariant]
                            * (0.699999988079071 + (0.30000001192092896 * (1.52587890625e-05 * (Float)(frand() & 0xFFFF)))));
                    particleEffect->color.r = (s32)(160.0 * (1.52587890625e-05 * (Float)(frand() & 0xFFFF))) + 0x20;
                    particleEffect->color.g = (s32)(96.0 * (1.52587890625e-05 * (Float)(frand() & 0xFFFF))) + 0x20;
                    particleEffect->color.b = (s32)(128.0 * (1.52587890625e-05 * (Float)(frand() & 0xFFFF))) + 0x20;
                    particleEffect->color.a = ((1.0 - (0.4 * normalizedParticleDistance)) * (255.0f * particleSizeValues[effectSettings->effectVariant]))
                        - (int)(16.0 * (1.52587890625e-05 * (Float)(frand() & 0xFFFF)));
                }
                effectSettings->animationStep++;
            }
            else {
                particleEffect->position.x = 0.0f;
                particleEffect->position.y = -27000.0f;
                particleEffect->position.z = -63000.0f;
                particleEffect->effectSize = 1000.0f;
            }
            break;
        case 1:
            cameraDirection.x = sind(CRot.y) * cosd(CRot.x);
            cameraDirection.y = -sind(CRot.x);
            cameraDirection.z = cosd(CRot.y) * cosd(CRot.x);
            cameraRelativeDirection.x = -(Center.x + (1.25f * (cameraDirection.x * CZoom)));
            cameraRelativeDirection.y = -27000.0f - (Center.y + (cameraDirection.y * CZoom));
            cameraRelativeDirection.z = -63000.0f - (Center.z + (1.25f * (cameraDirection.z * CZoom)));
            cameraDirectionLengthSquared = (cameraRelativeDirection.z * cameraRelativeDirection.z) + ((cameraRelativeDirection.x * cameraRelativeDirection.x) + (cameraRelativeDirection.y * cameraRelativeDirection.y));
            vectorScale = 1.0f / sqrtf(cameraDirectionLengthSquared);
            cameraRelativeDirection.x *= vectorScale;
            cameraRelativeDirection.y *= vectorScale;
            cameraRelativeDirection.z *= vectorScale;
            vectorScale = -((-cameraRelativeDirection.z * -cameraDirection.z) + ((-cameraRelativeDirection.x * -cameraDirection.x) + (-cameraRelativeDirection.y * -cameraDirection.y)));
            reflectedDirection.x = -cameraDirection.x + (-2.0f * cameraRelativeDirection.x * vectorScale);
            reflectedDirection.y = -cameraDirection.y + (-2.0f * cameraRelativeDirection.y * vectorScale);
            reflectedDirection.z = -cameraDirection.z + (-2.0f * cameraRelativeDirection.z * vectorScale);
            particleEffect = destination->particles;
            for (particleIndex = 0; particleIndex < destination->particleCount; particleIndex++, particleEffect++) {
                particleEffect->position.x = reflectedDirection.x * particleEffect->scaleFactor;
                particleEffect->position.y = -27000.0f + (reflectedDirection.y * particleEffect->scaleFactor);
                particleEffect->position.z = -63000.0f + (reflectedDirection.z * particleEffect->scaleFactor);
            }
            break;
        default:
            break;
    }
    DCFlushRange(destination->particles, destination->particleCount * sizeof(MapParticleEffect));
    PPCSync();
}

void SetOrthographicProjection(HU3DMODEL *source, Float (*destination)[4])
{
    Mtx identityModelMatrix;
    Mtx44 orthographicProjection;

    HU3DCAMERA *camera;
    SceneAnimationGroup *animationManager;

    GXColor spC = { 0xFF, 0xFF, 0xFF, 0xFF };
    camera = &Hu3DCamera[0];
    animationManager = sceneAnimationGroup;
    if (animationManager->overlayAlpha <= 0.0f) {
        Hu3DModelAttrSet(animationManager->updateHookId, HU3D_ATTR_DISPOFF);
        return;
    }
    GXSetScissor(camera->scissorX, camera->scissorY, camera->scissorW, camera->scissorH);
    GXSetViewport(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    C_MTXOrtho(orthographicProjection, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    GXSetProjection(orthographicProjection, GX_ORTHOGRAPHIC);
    spC.a = 255.0f * animationManager->overlayAlpha;
    GXSetTevColor(GX_TEVREG0, spC);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_A0);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    MTXIdentity(identityModelMatrix);
    GXLoadPosMtxImm(identityModelMatrix, 0);
    GXSetZMode(GX_FALSE, GX_ALWAYS, GX_TRUE);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_U8, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3u8(0, 0, 0);
    GXPosition3u8(1, 0, 0);
    GXPosition3u8(1, 1, 0);
    GXPosition3u8(0, 1, 0);
}

void SetAnimationGroupAlpha(Float scale)
{

    SceneAnimationGroup *animationManager;

    animationManager = sceneAnimationGroup;
    animationManager->modelGroupEnabled = scale;
    if (animationManager->modelGroupEnabled <= 0.0f) {
        Hu3DModelAttrSet(animationManager->modelId0, HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(animationManager->modelId1, HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(animationManager->modelId2, HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(animationManager->modelId3, HU3D_ATTR_DISPOFF);
        return;
    }
    Hu3DModelAttrReset(animationManager->modelId0, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(animationManager->modelId1, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(animationManager->modelId2, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(animationManager->modelId3, HU3D_ATTR_DISPOFF);
}

void SetAnimationGroupEnabled(Float scale)
{
    SceneAnimationGroup *animationManager;

    animationManager = sceneAnimationGroup;
    animationManager->overlayAlpha = scale;
    if (animationManager->overlayAlpha <= 0.0f) {
        Hu3DModelAttrSet(animationManager->updateHookId, HU3D_ATTR_DISPOFF);
        return;
    }
    Hu3DModelAttrReset(animationManager->updateHookId, HU3D_ATTR_DISPOFF);
}

void CreateSceneLight(void)
{
    s32 lightId;
    s32 modelId;

    if (mapLightId == -1) {
        mapLightId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M406, 68));
        Hu3DModelLayerSet(mapLightId, 0);
        Hu3DModelPosSet(mapLightId, 0.0f, -26500.0f, -63000.0f);
        Hu3DModelRotSet(mapLightId, 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(mapLightId, 2.75f, 2.75f, 2.75f);
        modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M406, 55));
        Hu3DModelLayerSet(modelId, 0);
        lightId = Hu3DGLightCreateV(&particleOrigin, &particleUpDirection, &particleDefaultColor);
        Hu3DGLightStaticSet(lightId, 1);
        Hu3DGLightPointSet(lightId, 100.0f, 1.0f, 3);
        mapRendererEnabled = 0;
        UpdateMapModels(mapModelObject);
    }
}

void DestroySceneLight(void)
{
    if (mapLightId != -1) {
        Hu3DModelKill(mapLightId);
    }
    mapLightId = -1;
}

void ResetSceneLighting(void) { }

s16 CreateModelAnimation(ANIMDATA *source, s16 destination)
{
    ParticleEffectHookData *effectHookData;
    s16 vertexIndex;
    Vec2f *textureCoordinate;
    MapParticleEffect *particleEffect;
    HU3DMODEL *hookModel;
    Vec *positionVertex;
    void *displayListBuffer;
    s16 hookModelId;
    void *displayListBase;

    hookModelId = Hu3DHookFuncCreate(DrawModelHook);
    hookModel = &Hu3DData[hookModelId];
    effectHookData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(ParticleEffectHookData), hookModel->mallocNo);
    hookModel->hookData = effectHookData;
    effectHookData->animationData = source;
    effectHookData->particleCount = destination;
    effectHookData->hookMode = 0;
    effectHookData->effectFlags = 0;
    effectHookData->DrawHook = 0;
    effectHookData->relatedModel = 0;
    effectHookData->effectActive = 0;
    effectHookData->reservedHookByte = 0;
    effectHookData->activeParticleLimit = 0;

    effectHookData->reservedHeaderWord00 = effectHookData->reservedHeaderWord02 = 0;
    particleEffect = HuMemDirectMallocNum(HEAP_MODEL, destination * sizeof(MapParticleEffect), hookModel->mallocNo);
    effectHookData->particles = particleEffect;
    memset(particleEffect, 0, destination * sizeof(MapParticleEffect));
    for (vertexIndex = 0; vertexIndex < destination; vertexIndex++, particleEffect++) {
        particleEffect->effectActive = 0.0f;
        particleEffect->rotationAngle = 0.0f;
        particleEffect->activeParticleLimit.x = ((frand() & 0x7F) - 0x40) * 0x14;
        particleEffect->activeParticleLimit.y = ((frand() & 0x7F) - 0x40) * 0x1E;
        particleEffect->activeParticleLimit.z = ((frand() & 0x7F) - 0x40) * 0x14;
        particleEffect->particles.r = particleEffect->particles.g = particleEffect->particles.b = particleEffect->particles.a = 0xFF;
        particleEffect->particlePositions = 0;
    }
    positionVertex = HuMemDirectMallocNum(HEAP_MODEL, destination * sizeof(Vec) * 4, hookModel->mallocNo);
    effectHookData->particlePositions = positionVertex;
    for (vertexIndex = 0; vertexIndex < (destination * 4); vertexIndex++, positionVertex++) {
        positionVertex->x = positionVertex->y = positionVertex->z = 0.0f;
    }
    textureCoordinate = HuMemDirectMallocNum(HEAP_MODEL, destination * sizeof(Vec2f) * 4, hookModel->mallocNo);
    effectHookData->particleTextureCoordinates = textureCoordinate;
    for (vertexIndex = 0; vertexIndex < destination; vertexIndex++) {
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
    displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, 0x10000, hookModel->mallocNo);
    displayListBase = displayListBuffer;
    DCFlushRange(displayListBuffer, 0x10000);
    GXBeginDisplayList(displayListBase, 0x10000);
    GXBegin(GX_QUADS, GX_VTXFMT0, destination * 4);
    for (vertexIndex = 0; vertexIndex < destination; vertexIndex++) {
        GXPosition1x16(vertexIndex * 4);
        GXColor1x16(vertexIndex);
        GXTexCoord1x16(vertexIndex * 4);

        GXPosition1x16(vertexIndex * 4 + 1);
        GXColor1x16(vertexIndex);
        GXTexCoord1x16(vertexIndex * 4 + 1);

        GXPosition1x16(vertexIndex * 4 + 2);
        GXColor1x16(vertexIndex);
        GXTexCoord1x16(vertexIndex * 4 + 2);

        GXPosition1x16(vertexIndex * 4 + 3);
        GXColor1x16(vertexIndex);
        GXTexCoord1x16(vertexIndex * 4 + 3);
    }
    effectHookData->displayListByteCount = GXEndDisplayList();
    effectHookData->displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, effectHookData->displayListByteCount, hookModel->mallocNo);
    memcpy(effectHookData->displayListBuffer, displayListBuffer, effectHookData->displayListByteCount);
    DCFlushRange(effectHookData->displayListBuffer, effectHookData->displayListByteCount);
    HuMemDirectFree(displayListBuffer);
    (void)destination;
    return hookModelId;
}

s16 CloneModelHookData(s16 source)
{
    ParticleEffectHookData *clonedHookData;
    Vec2f *textureCoordinate;
    MapParticleEffect *particleEffect;
    HU3DMODEL *clonedModel;
    s16 particleIndex;
    ParticleEffectHookData *sourceHookData;
    Vec *positionVertex;
    s32 particleCount;
    HU3DMODEL *sourceModel;
    s16 clonedHookModelId;

    sourceModel = &Hu3DData[source];
    sourceHookData = sourceModel->hookData;
    clonedHookModelId = Hu3DHookFuncCreate(sourceModel->hookFunc);
    clonedModel = &Hu3DData[clonedHookModelId];
    clonedHookData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(ParticleEffectHookData), clonedModel->mallocNo);
    clonedModel->hookData = clonedHookData;
    clonedHookData->animationData = sourceHookData->animationData;
    clonedHookData->particleCount = sourceHookData->particleCount;
    particleCount = clonedHookData->particleCount;
    clonedHookData->hookMode = 0;
    clonedHookData->effectFlags = sourceHookData->effectFlags;
    clonedHookData->DrawHook = 0;
    clonedHookData->relatedModel = 0;
    clonedHookData->effectActive = 0;
    clonedHookData->reservedHookByte = 0;
    clonedHookData->activeParticleLimit = 0;

    clonedHookData->reservedHeaderWord00 = clonedHookData->reservedHeaderWord02 = 0;
    particleEffect = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(MapParticleEffect), clonedModel->mallocNo);
    clonedHookData->particles = particleEffect;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particleEffect++) {
        particleEffect->effectSize = 0.0f;
        particleEffect->rotationAngle = 0.0f;
        particleEffect->position.x = ((frand() & 0x7F) - 0x40) * 0x14;
        particleEffect->position.y = ((frand() & 0x7F) - 0x40) * 0x1E;
        particleEffect->position.z = ((frand() & 0x7F) - 0x40) * 0x14;
        particleEffect->color.r = particleEffect->color.g = particleEffect->color.b = particleEffect->color.a = 0xFF;
        particleEffect->textureFrameIndex = 0;
    }
    positionVertex = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec) * 4, clonedModel->mallocNo);
    clonedHookData->particlePositions = positionVertex;
    for (particleIndex = 0; particleIndex < (particleCount * 4); particleIndex++, positionVertex++) {
        positionVertex->x = positionVertex->y = positionVertex->z = 0.0f;
    }
    textureCoordinate = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec2f) * 4, clonedModel->mallocNo);
    clonedHookData->particleTextureCoordinates = textureCoordinate;
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
    clonedHookData->displayListByteCount = sourceHookData->displayListByteCount;
    clonedHookData->displayListBuffer = sourceHookData->displayListBuffer;
    return clonedHookModelId;
}

void DrawModelHook(HU3DMODEL *source, Float (*destination)[4])
{
    Mtx normalTransform;
    Mtx rotatedQuadTransform;
    Mtx particleRotation;
    Vec scaledQuadCorners[4];
    Vec rotatedQuadCorners[4];
    Vec cameraAlignedCorners[4];
    ROMtx reorderedNormalTransform;

    ParticleEffectHookData *hookData;
    Vec *positionVertex;
    MapParticleEffect *particleEffect;
    Vec2f *textureCoordinate;
    Vec *quadCorner;
    s16 particleIndex;
    s16 quadVertexIndex;
    Vec *rotatedCorner;
    s16 textureFormat;
    s16 textureFrameU;
    s16 textureFrameV;
    ParticleEffectDrawHook DrawHook;

    hookData = source->hookData;
    if (hookData->lastUpdateFrame != GlobalCounter || shadowModelDrawF) {
        if (hookData->relatedModel && hookData->relatedModel != source) {
            DrawModelHook(hookData->relatedModel, destination);
        }
        GXLoadPosMtxImm(destination, 0);
        GXSetNumTevStages(1);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, GX_FALSE, 0x7D);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        if (shadowModelDrawF) {
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ONE, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
            GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
        }
        else {
            textureFormat = hookData->animationData->bmp->dataFmt & 0xF;
            if (textureFormat == 7 || textureFormat == 8) {
                GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
            }
            else {
                GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
            }
            if (hookData->effectFlags & 1) {
                GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
            }
            else if (source->attr & 2) {
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
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
        HuSprTexLoad(hookData->animationData, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
        GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
        GXSetZCompLoc(GX_FALSE);
        switch (hookData->hookMode) {
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
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetArray(GX_VA_POS, hookData->particlePositions, 0xC);
        GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GXSetArray(GX_VA_CLR0, &hookData->particles->color, 0x44);
        GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
        GXSetArray(GX_VA_TEX0, hookData->particleTextureCoordinates, 8);
        if (HmfInverseMtxF3X3(destination, normalTransform) == 0) {
            MTXIdentity(normalTransform);
        }
        PSMTXReorder(normalTransform, reorderedNormalTransform);
        if (hookData->DrawHook) {
            DrawHook = hookData->DrawHook;
            DrawHook(source, hookData, destination);
        }
        particleEffect = hookData->particles;
        positionVertex = hookData->particlePositions;
        textureCoordinate = hookData->particleTextureCoordinates;
        PSMTXROMultVecArray(reorderedNormalTransform, particleSpawnPositions, &cameraAlignedCorners[0], 4);
        for (particleIndex = 0; particleIndex < hookData->particleCount; particleIndex++, particleEffect++) {
            if (!particleEffect->effectSize) {
                positionVertex->x = positionVertex->y = positionVertex->z = 0.0f;
                positionVertex++;
                positionVertex->x = positionVertex->y = positionVertex->z = 0.0f;
                positionVertex++;
                positionVertex->x = positionVertex->y = positionVertex->z = 0.0f;
                positionVertex++;
                positionVertex->x = positionVertex->y = positionVertex->z = 0.0f;
                positionVertex++;
            }
            else if (!particleEffect->rotationAngle) {
                quadCorner = &scaledQuadCorners[0];
                rotatedCorner = &cameraAlignedCorners[0];
                VECScale(rotatedCorner++, quadCorner, particleEffect->effectSize);
                VECAdd(quadCorner++, &particleEffect->position, positionVertex++);
                VECScale(rotatedCorner++, quadCorner, particleEffect->effectSize);
                VECAdd(quadCorner++, &particleEffect->position, positionVertex++);
                VECScale(rotatedCorner++, quadCorner, particleEffect->effectSize);
                VECAdd(quadCorner++, &particleEffect->position, positionVertex++);
                VECScale(rotatedCorner++, quadCorner, particleEffect->effectSize);
                VECAdd(quadCorner++, &particleEffect->position, positionVertex++);
            }
            else {
                VECScale(particleSpawnPositions, &scaledQuadCorners[0], particleEffect->effectSize);
                VECScale(&particleSpawnPositions[1], &scaledQuadCorners[1], particleEffect->effectSize);
                VECScale(&particleSpawnPositions[2], &scaledQuadCorners[2], particleEffect->effectSize);
                VECScale(&particleSpawnPositions[3], &scaledQuadCorners[3], particleEffect->effectSize);
                MTXRotRad(particleRotation, 90, particleEffect->rotationAngle);
                MTXConcat(normalTransform, particleRotation, rotatedQuadTransform);
                MTXMultVecArray(rotatedQuadTransform, &scaledQuadCorners[0], &rotatedQuadCorners[0], 4);
                VECAdd(&rotatedQuadCorners[0], &particleEffect->position, positionVertex++);
                VECAdd(&rotatedQuadCorners[1], &particleEffect->position, positionVertex++);
                VECAdd(&rotatedQuadCorners[2], &particleEffect->position, positionVertex++);
                VECAdd(&rotatedQuadCorners[3], &particleEffect->position, positionVertex++);
            }
        }
        particleEffect = hookData->particles;
        textureCoordinate = hookData->particleTextureCoordinates;
        if ((hookData->effectFlags & 2) == 0) {
            for (particleIndex = 0; particleIndex < hookData->particleCount; particleIndex++, particleEffect++) {
                textureFrameU = particleEffect->textureFrameIndex & 3;
                textureFrameV = (particleEffect->textureFrameIndex >> 2) & 3;
                for (quadVertexIndex = 0; quadVertexIndex < 4; quadVertexIndex++, textureCoordinate++) {
                    textureCoordinate->x = (0.25f * textureFrameU) + particleTextureCoordinates[quadVertexIndex].x;
                    textureCoordinate->y = (0.25f * textureFrameV) + particleTextureCoordinates[quadVertexIndex].y;
                }
            }
        }
        else {
            for (particleIndex = 0; particleIndex < hookData->particleCount; particleIndex++, particleEffect++) {
                for (quadVertexIndex = 0; quadVertexIndex < 4; quadVertexIndex++, textureCoordinate++) {
                    textureCoordinate->x = 4.0f * particleTextureCoordinates[quadVertexIndex].x;
                    textureCoordinate->y = 4.0f * particleTextureCoordinates[quadVertexIndex].y;
                }
            }
        }
        DCFlushRangeNoSync(hookData->particlePositions, hookData->particleCount * 0xC * 4);
        DCFlushRangeNoSync(hookData->particleTextureCoordinates, hookData->particleCount * 8 * 4);
        DCFlushRangeNoSync(hookData->particles, hookData->particleCount * 0x44);
        PPCSync();
        GXCallDisplayList(hookData->displayListBuffer, hookData->displayListByteCount);
        if (!shadowModelDrawF) {
            if ((hookData->reservedHookByte & 2) == 0) {
                hookData->effectActive++;
            }
            if ((hookData->activeParticleLimit != 0) && (hookData->activeParticleLimit <= hookData->effectActive)) {
                if (hookData->reservedHookByte & 1) {
                    hookData->effectActive = 0;
                }
                hookData->effectActive = hookData->activeParticleLimit;
            }
            hookData->lastUpdateFrame = GlobalCounter;
        }
    }
}

s16 UpdateTextureCoordinates(s16 source)
{
    Vec2f *textureCoordinate;

    ParticleEffectHookData *hookData;
    MapParticleEffect *particleEffect;
    s16 particleIndex;
    HU3DMODEL *hookModel;
    Vec *positionVertex;
    void *displayListBuffer;
    s16 hookModelId;
    void *displayListBase;

    hookModelId = Hu3DHookFuncCreate(DrawModelNormals);
    hookModel = &Hu3DData[hookModelId];
    hookData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(ParticleEffectHookData), hookModel->mallocNo);
    hookModel->hookData = hookData;
    hookData->animationData = 0;
    hookData->particleCount = source;
    hookData->hookMode = 0;
    hookData->effectFlags = 0;
    hookData->DrawHook = 0;
    hookData->relatedModel = 0;
    hookData->effectActive = 0;
    hookData->reservedHookByte = 0;
    hookData->activeParticleLimit = 0;

    hookData->reservedHeaderWord00 = hookData->reservedHeaderWord02 = 0;
    particleEffect = HuMemDirectMallocNum(HEAP_MODEL, source * sizeof(MapParticleEffect), hookModel->mallocNo);
    hookData->particles = particleEffect;
    memset(particleEffect, 0, source * sizeof(MapParticleEffect));
    for (particleIndex = 0; particleIndex < source; particleIndex++, particleEffect++) {
        particleEffect->effectSize = 0.0f;
        particleEffect->rotationAngle = 0.0f;
        particleEffect->position.x = ((frand() & 0x7F) - 0x40) * 0x14;
        particleEffect->position.y = ((frand() & 0x7F) - 0x40) * 0x1E;
        particleEffect->position.z = ((frand() & 0x7F) - 0x40) * 0x14;
        particleEffect->color.r = particleEffect->color.g = particleEffect->color.b = particleEffect->color.a = 0xFF;
        particleEffect->textureFrameIndex = 0;
    }
    positionVertex = HuMemDirectMallocNum(HEAP_MODEL, source * sizeof(Vec) * 4, hookModel->mallocNo);
    hookData->particlePositions = positionVertex;
    for (particleIndex = 0; particleIndex < (source * 4); particleIndex++, positionVertex++) {
        positionVertex->x = positionVertex->y = positionVertex->z = 0.0f;
    }
    hookData->particleTextureCoordinates = textureCoordinate = NULL;
    displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, 0x10000, hookModel->mallocNo);
    displayListBase = displayListBuffer;
    DCFlushRange(displayListBuffer, 0x10000);
    GXBeginDisplayList(displayListBase, 0x10000);
    GXBegin(GX_QUADS, GX_VTXFMT0, source * 4);
    for (particleIndex = 0; particleIndex < source; particleIndex++) {
        GXPosition1x16(particleIndex * 4);
        GXColor1x16(particleIndex);

        GXPosition1x16(particleIndex * 4 + 1);
        GXColor1x16(particleIndex);

        GXPosition1x16(particleIndex * 4 + 2);
        GXColor1x16(particleIndex);

        GXPosition1x16(particleIndex * 4 + 3);
        GXColor1x16(particleIndex);
    }
    hookData->displayListByteCount = GXEndDisplayList();
    hookData->displayListBuffer = HuMemDirectMallocNum(HEAP_MODEL, hookData->displayListByteCount, hookModel->mallocNo);
    memcpy(hookData->displayListBuffer, displayListBuffer, hookData->displayListByteCount);
    DCFlushRange(hookData->displayListBuffer, hookData->displayListByteCount);
    HuMemDirectFree(displayListBuffer);
    return hookModelId;
}

void DrawModelNormals(HU3DMODEL *source, Mtx destination)
{
    Mtx normalTransform;
    ROMtx reorderedNormalTransform;
    ROMtx cameraAlignedCorners;
    ROMtx positionTransform;
    Vec2f *textureCoordinate;

    ParticleEffectHookData *hookData;
    Vec *positionVertex;
    MapParticleEffect *particleEffect;
    Vec *quadCorner;
    Vec *transformedCorner;
    s16 particleIndex;
    ParticleEffectDrawHook DrawHook;

    hookData = source->hookData;
    if (hookData->lastUpdateFrame != GlobalCounter || shadowModelDrawF) {
        if (hookData->relatedModel && hookData->relatedModel != source) {
            DrawModelNormals(hookData->relatedModel, destination);
        }
        GXLoadPosMtxImm(destination, 0);
        GXSetNumTevStages(1);
        GXSetNumTexGens(0);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, GX_FALSE, 0x7D);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
        GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
        GXSetZCompLoc(GX_FALSE);
        if (source->attr & 2) {
            GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        }
        else {
            GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
        }
        switch (hookData->hookMode) {
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
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetArray(GX_VA_POS, hookData->particlePositions, sizeof(Vec));
        GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GXSetArray(GX_VA_CLR0, &hookData->particles->color, 0x44);
        if (HmfInverseMtxF3X3(destination, normalTransform) == 0) {
            MTXIdentity(normalTransform);
        }
        PSMTXReorder(normalTransform, positionTransform);
        if (hookData->DrawHook) {
            DrawHook = hookData->DrawHook;
            DrawHook(source, hookData, destination);
        }
        particleEffect = hookData->particles;
        positionVertex = hookData->particlePositions;
        textureCoordinate = hookData->particleTextureCoordinates;
        PSMTXROMultVecArray(positionTransform, particleModelPositions, (Vec *)cameraAlignedCorners, 4);
        for (particleIndex = 0; particleIndex < hookData->particleCount; particleIndex++, particleEffect++) {
            if (!particleEffect->effectSize) {
                positionVertex->x = positionVertex->y = positionVertex->z = 0.0f;
                positionVertex++;
                positionVertex->x = positionVertex->y = positionVertex->z = 0.0f;
                positionVertex++;
                positionVertex->x = positionVertex->y = positionVertex->z = 0.0f;
                positionVertex++;
                positionVertex->x = positionVertex->y = positionVertex->z = 0.0f;
                positionVertex++;
            }
            else {
                quadCorner = (Vec *)&reorderedNormalTransform;
                transformedCorner = (Vec *)&cameraAlignedCorners;
                VECScale(transformedCorner++, quadCorner, particleEffect->effectSize);
                VECAdd(quadCorner++, &particleEffect->position, positionVertex++);
                VECScale(transformedCorner++, quadCorner, particleEffect->effectSize);
                VECAdd(quadCorner++, &particleEffect->position, positionVertex++);
                VECScale(transformedCorner++, quadCorner, particleEffect->effectSize);
                VECAdd(quadCorner++, &particleEffect->position, positionVertex++);
                VECScale(transformedCorner++, quadCorner, particleEffect->effectSize);
                VECAdd(quadCorner++, &particleEffect->position, positionVertex++);
            }
        }
        DCFlushRangeNoSync(hookData->particlePositions, hookData->particleCount * 0xC * 4);
        DCFlushRangeNoSync(hookData->particles, hookData->particleCount * 0x44);
        PPCSync();
        GXCallDisplayList(hookData->displayListBuffer, hookData->displayListByteCount);
        if (!shadowModelDrawF) {
            if ((hookData->reservedHookByte & 2) == 0) {
                hookData->effectActive++;
            }
            if ((hookData->activeParticleLimit != 0) && (hookData->activeParticleLimit <= hookData->effectActive)) {
                if (hookData->reservedHookByte & 1) {
                    hookData->effectActive = 0;
                }
                hookData->effectActive = hookData->activeParticleLimit;
            }
            hookData->lastUpdateFrame = GlobalCounter;
        }
    }
}

s32 CopyModelMaterials(s32 source, s32 destination, s32 effectColor)
{
    HU3DMODEL *sourceModel;
    HU3DMODEL *targetModel;
    s32 lightSlot;

    sourceModel = &Hu3DData[source];
    targetModel = &Hu3DData[effectColor];
    for (lightSlot = 0; lightSlot < 8; lightSlot++) {
        if (targetModel->lLightId[lightSlot] == -1) {
            break;
        }
    }
    if (lightSlot == 8) {
        return -1;
    }
    targetModel->lLightId[lightSlot] = sourceModel->lLightId[destination];
    targetModel->attr |= 0x1000;
    return lightSlot;
}

void SetModelMaterialColors(s32 source, Float scale, Float rotationSpeed, Float secondScale, Float thirdScale)
{
    HSFMATERIAL *material;
    s32 channelValue;
    HSFDATA *modelData;
    s32 materialIndex;
    HU3DMODEL *model;
    s32 materialCount;

    model = &Hu3DData[source];
    modelData = model->hsf;
    material = modelData->material;
    materialCount = modelData->materialNum;
    for (materialIndex = 0; materialIndex < materialCount; materialIndex++, material++) {
        material->vtxMode = 1;
        channelValue = rotationSpeed * (material->litColor[0] * scale);
        if (channelValue > 0xFF) {
            channelValue = 0xFF;
        }
        material->litColor[0] = channelValue;
        channelValue = secondScale * (material->litColor[1] * scale);
        if (channelValue > 0xFF) {
            channelValue = 0xFF;
        }
        material->litColor[1] = channelValue;
        channelValue = thirdScale * (material->litColor[2] * scale);
        if (channelValue > 0xFF) {
            channelValue = 0xFF;
        }
        material->litColor[2] = channelValue;
    }
}

void SetModelMaterialColor(s32 source, u8 destination, u8 effectColor, u8 channelValue)
{
    HSFMATERIAL *material;
    HSFDATA *modelData;
    s32 materialIndex;
    HU3DMODEL *model;
    s32 materialCount;

    model = &Hu3DData[source];
    modelData = model->hsf;
    material = modelData->material;
    materialCount = modelData->materialNum;
    for (materialIndex = 0; materialIndex < materialCount; materialIndex++, material++) {
        material->vtxMode = 1;
        material->litColor[0] = destination;
        material->litColor[1] = effectColor;
        material->litColor[2] = channelValue;
    }
}

void SetModelMaterialAlpha(s32 source, s32 destination)
{
    s32 dataIndex;
    HSFDATA *targetModelData;
    HSFDATA *sourceModelData;
    HU3DMODEL *sourceModel;
    HU3DMODEL *targetModel;

    sourceModel = &Hu3DData[source];
    targetModel = &Hu3DData[destination];
    sourceModelData = sourceModel->hsf;
    targetModelData = targetModel->hsf;
    for (dataIndex = 0; dataIndex < targetModelData->bitmapNum; dataIndex++) {
        memcpy(&targetModelData->bitmap[dataIndex], &sourceModelData->bitmap[dataIndex], 0x20);
    }
    for (dataIndex = 0; dataIndex < targetModelData->attributeNum; dataIndex++) {
        memcpy(&targetModelData->attribute[dataIndex], &sourceModelData->attribute[dataIndex], 0x84);
    }
}

s16 GetModelLightChannels(HU3DMODEL *source, Mtx destination, Mtx effectColor, Float scale)
{
    s16 lightIndex;
    s16 lightChannelMask;
    s16 enabledChannels;
    HU3DLIGHT *localLight;

    enabledChannels = 0;
    lightChannelMask = 1;
    if (source->attr & 0x1000) {
        for (lightIndex = 0; lightIndex < 8; lightIndex++) {
            if (source->lLightId[lightIndex] != -1) {
                localLight = &Hu3DLocalLight[source->lLightId[lightIndex]];
                TransformLightDirection(localLight, lightChannelMask, effectColor, destination, scale);
                enabledChannels |= lightChannelMask;
                lightChannelMask <<= 1;
            }
        }
    }
    return enabledChannels;
}

void TransformLightDirection(HU3DLIGHT *source, s16 destination, Mtx effectColor, Mtx channelValue, Float scale)
{
    GXLightObj lightObject;
    Vec transformedDirection;
    Vec transformedPosition;

    switch ((u8)source->type) {
        case 0:
            GXInitLightAttn(&lightObject, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
            GXInitLightSpot(&lightObject, source->cutoff, source->func);
            break;
        case 1:
            GXInitLightAttn(&lightObject, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
            GXInitLightSpot(&lightObject, 20.0f, GX_SP_COS);
            GXInitLightAttnK(&lightObject, 1.0f, 0.0f, 0.0f);
            VECScale(&source->dir, &source->pos, -1000000.0f);
            break;
        case 2:
            GXInitLightAttn(&lightObject, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            GXInitLightDistAttn(&lightObject, source->cutoff, source->brightness, source->func);
            break;
        default:
            break;
    }
    if (source->type & 0x8000) {
        MTXMultVec(effectColor, &source->dir, &transformedDirection);
        MTXMultVec(channelValue, &source->pos, &transformedPosition);
        GXInitLightPos(&lightObject, transformedPosition.x, transformedPosition.y, transformedPosition.z);
    }
    else {
        GXInitLightPos(&lightObject, source->pos.x, source->pos.y, source->pos.z);
        transformedDirection = source->dir;
    }
    if (scale == 0.0f) {
        GXInitLightDir(&lightObject, transformedDirection.x, transformedDirection.y, transformedDirection.z);
    }
    else {
        GXInitSpecularDir(&lightObject, transformedDirection.x, transformedDirection.y, transformedDirection.z);
        GXInitLightAttn(&lightObject, 0.0f, 0.0f, 1.0f, scale / 2, 0.0f, 1.0f - (scale / 2));
    }
    GXInitLightColor(&lightObject, source->color);
    GXLoadLightObjImm(&lightObject, destination);
}

void InterpolateVector(Float scale, Vec *source, Vec *destination, Vec *effectColor)
{
    interpolatedVector.x = source->x + (scale * (destination->x - source->x));
    interpolatedVector.y = source->y + (scale * (destination->y - source->y));
    interpolatedVector.z = source->z + (scale * (destination->z - source->z));
    *effectColor = interpolatedVector;
}

void TransformVector(Mtx source, Vec *destination)
{
    Vec transformedVector;

    transformedVector.x = (source[0][2] * destination->z) + ((source[0][0] * destination->x) + (source[0][1] * destination->y));
    transformedVector.y = (source[1][2] * destination->z) + ((source[1][0] * destination->x) + (source[1][1] * destination->y));
    transformedVector.z = (source[2][2] * destination->z) + ((source[2][0] * destination->x) + (source[2][1] * destination->y));
    *destination = transformedVector;
}

Float ProjectVectorOntoAxis(Vec *source, Vec *destination, Float scale, Float rotationSpeed)
{
    Float firstXDelta;
    Float firstZDelta;
    Float secondXDelta;
    Float secondZDelta;
    Float signedCrossProduct;

    firstXDelta = source->x - scale;
    firstZDelta = source->z - rotationSpeed;
    secondXDelta = destination->x - scale;
    secondZDelta = destination->z - rotationSpeed;

    signedCrossProduct = -((firstZDelta * secondXDelta) - (firstXDelta * secondZDelta));
    return signedCrossProduct;
}

#include "REL/m427Dll.h"

#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXStruct.h"
#include "dolphin/gx/GXVert.h"
#include "dolphin/pad.h"
#include "ext_math.h"
#include "game/animdata.h"
#include "game/audio.h"
#include "game/disp.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "string.h"
#include "version.h"

extern s32 rand8(void);
extern HU3DLIGHT Hu3DLocalLight[0x20];

#define FABS(value) ((value < 0) ? -(value) : (value))

typedef struct RecordTimerDisplay {
    s32 teamSprites[3];
    s32 teamNameSprite;
    s32 recordLabelSprite;
    s32 recordTimeDigits[7];
    s32 elapsedTimeDigits[7];
    HuVec2f recordDigitOffsets[7];
    f32 recordDigitOffsetX;
    f32 recordDigitOffsetY;
    s32 digitScaleDirection;
    f32 recordDigitsOriginX;
    f32 recordDigitsOriginY;
    f32 digitScale;
    s32 recordTimeFrames;
    s32 elapsedTimeFrames;
} RecordTimerDisplay; // sizeof 0xA4

typedef struct CameraPreset {
    Vec rot;
    Vec cen;
    f32 zoom;
} CameraPreset; // sizeof 0x1C

typedef struct MapQuadRenderData {
    u8 drawFlags;
    char primitiveType;
    s16 cameraIndex;
    s16 modelId;
    char reservedBytes[0x2];
    ANIMDATA *animationData;
    Vec topLeftPosition;
    Vec topRightPosition;
    Vec bottomRightPosition;
    Vec bottomLeftPosition;
    Vec2f topLeftUv;
    Vec2f topRightUv;
    Vec2f bottomRightUv;
    Vec2f bottomLeftUv;
    Vec2f transformedUv[4];
    GXColor tintColor;
    void *displayList;
    s32 displayListSize;
    float textureOffsetX;
    float textureOffsetY;
    float secondaryTextureOffsetX;
    float secondaryTextureOffsetY;
} MapQuadRenderData; // sizeof 0x98

typedef struct CharacterEffectSlot {
    s8 activeState;
    s16 modelId;
    s16 animationId;
} CharacterEffectSlot; // sizeof 0x6

typedef struct RoutePoint {
    s16 routePointType;
    Vec worldPosition;
} RoutePoint; // sizeof 0x10

typedef struct RouteEvent {
    s16 routeType;
    Vec worldPosition;
    s16 linkedPointIndices[2];
    s16 stateDurations[4];
} RouteEvent; // sizeof 0x1C

typedef struct MapResourceEntry {
    s16 resourceType;
    s32 resourceId;
    void *resourceData;
} MapResourceEntry; // sizeof 0xC

typedef struct RouteEffectState {
    s16 routeEventIndex;
    s16 modelId;
    s16 lightIndex;
    s16 spriteId;
    u8 animationPhase;
    s16 phaseFrame;
    float opacity;
    float lightRadius;
    float spriteOpacity;
    s16 phaseOneFrames;
    s16 phaseTwoFrames;
    s16 phaseThreeFrames;
    s16 phaseFourFrames;
    float scalePulseAngle;
    float lightPulseAngle;
    float effectScale;
    float particleAngle;
} RouteEffectState; // sizeof 0x30

typedef struct RouteLightModelLink {
    s16 routePointIndex;
    s16 modelId;
} RouteLightModelLink; // sizeof 0x4

typedef struct TeamRenderTarget {
    s16 cameraIndex;
    void *textureBuffer;
    u32 textureBufferSize;
} TeamRenderTarget; // sizeof 0xC

typedef struct TeamMapState {
    s16 teamIndex;
    s16 modelIds[11];
    s16 modelHandleScratch;
    s16 lightModelId;
    s16 particleModelId;
    s16 backgroundHookModelId;
    s16 foregroundHookModelId;
    char reservedBytes[2];
    float effectAngles[4];
    s16 effectIds[4];
    s16 lightSlotIndices[4];
    s16 routeEffectIndices[0xB];
    s16 spotlightId;
    s16 highlightLightId;
    char reservedBytes2[2];
    Vec spotlightPosition;
    Vec spotlightTarget;
    Vec cameraFocusPosition;
    Vec worldPosition;
    f32 facingAngle;
    TeamRenderTarget renderTarget;
    char reservedBytes3[0xC];
    struct TerrainWaveRenderData *particleHookData;
    s16 bannerModelId[1];
    char reservedBytes4[0x6];
    RouteEffectState routeEffects[2][2];
    s16 activeRouteEffectCounts[3];
    RouteLightModelLink linkedLightModels[2][2];
    s16 activeRoutePointModels[2];
    char reservedBytes[4];
} TeamMapState; // sizeof 0x198

typedef struct BannerAnimationSet {
    ANIMDATA *primaryAnimation;
    ANIMDATA *secondaryAnimation;
    float offsetX;
    float offsetY;
    float scaleX;
    float scaleY;
} BannerAnimationSet; // sizeof 0x18

typedef struct MapState {
    u8 mapReady;
    TeamMapState teamStates[2];
    BannerAnimationSet bannerAnimationResources;
    RouteEvent routeEvents[3];
    RoutePoint routePoints[5];
} MapState; // sizeof 0x3F0

typedef struct GridQuad {
    s16 firstVertexIndex;
    s16 secondVertexIndex;
    s16 thirdVertexIndex;
    s16 fourthVertexIndex;
    Vec vertexNormal;
    char reservedBytes[0x8];
} GridQuad; // sizeof 0x1C

typedef struct MapSurfaceRenderData {
    s16 renderModelId;
    Vec *vertexPositions;
    Vec2f *textureCoordinates;
    Vec *animatedPositions;
    GXColor *vertexColors;
    s16 vertexCount;
    s16 gridWidth;
    s16 gridHeight;
    GridQuad *gridQuads;
    void *displayList;
    u32 displayListSize;
    void *textureBuffer;
    u32 textureBufferSize;
    void *secondaryTextureBuffer;
    u32 secondaryTextureBufferSize;
    f32 textureOffsetX;
    f32 textureOffsetY;
    f32 rotationAngleX;
    f32 rotationAngleY;
    s16 particleSpawnTimers[1];
    s16 quadCounterScratch;
} MapSurfaceRenderData; // sizeof 0x4C

typedef struct TeamMarkerRenderData {
    f32 positionX;
    f32 positionY;
    f32 positionZ;
    f32 collisionRadius;
    f32 opacity;
} TeamMarkerRenderData; // sizeof 0x14

typedef struct TerrainWaveRenderData {
    s16 cameraIndex;
    Vec *sourcePositions;
    Vec *targetPositions;
    float terrainOffset;
    float particleScale;
    float scrollSineOffset;
    float terrainWaveState;
    float textureScrollOffsetY;
    float textureScrollOffsetX;
    float textureScrollPhase;
    ANIMDATA *animationData;
    Vec *terrainVertices;
    Vec2f *terrainTextureCoordinates;
    Vec *terrainNormals;
    s16 vertexCount;
    s16 gridColumns;
    s16 gridRowCount;
    char reservedBytes3E[2];
    void *displayList;
    u32 displayListSize;
    GridQuad *gridQuads;
    float wavePhaseX;
    float wavePhaseZ;
    float wavePhaseStepX;
    float wavePhaseStepZ;
    float waveAmplitudeX;
    /* 0x60 */ float waveAmplitudeZ;
} TerrainWaveRenderData; /* size = 0x64 */

typedef struct ParticleState {
    /* 0x00 */ float velocityX;
    /* 0x04 */ float velocityY;
    /* 0x08 */ float velocityZ;
    /* 0x0C */ char reservedBytes0C[0xC]; /* maybe part of velocityZ[4]? */
    /* 0x18 */ float decelerationX;
    /* 0x1C */ float accelerationY;
    /* 0x20 */ float decelerationZ;
    /* 0x24 */ float scale;
    /* 0x28 */ float scaleDelta;
    /* 0x2C */ float opacity;
    /* 0x30 */ float opacityDelta;
    /* 0x34 */ s16 animationFrame;
    /* 0x36 */ s16 animationFrameCount;
    /* 0x38 */ s16 particleType;
    /* 0x3A */ char reservedBytes3A[2];
    /* 0x3C */ s32 particleStateFlags;
    /* 0x40 */ s16 particleIndex;
    /* 0x42 */ char reservedBytes42[2];
    /* 0x44 */ float rotationAngle;
    /* 0x48 */ Vec rotation;
    /* 0x54 */ Vec position;
    /* 0x60 */ s16 textureTileIndex;
    /* 0x62 */ s8 active;
    /* 0x63 */ char reservedBytes63[1];
} ParticleState; // sizeof 0x64

typedef void (*ParticleRenderCallback)(HU3DMODEL *model, struct ParticleSystemRenderData *particle, Mtx matrix);

typedef struct ParticleSystemRenderData {
    /* 0x00 */ s16 particleCount;
    /* 0x02 */ char reservedBytes02[2];
    /* 0x04 */ u32 displayListSize;
    /* 0x08 */ u8 renderMode;
    /* 0x09 */ char reservedBytes09[3]; /* maybe part of renderMode[4]? */
    /* 0x0C */ ParticleRenderCallback particleHook;
    /* 0x10 */ ANIMDATA *animationData;
    /* 0x14 */ s16 textureTileCount;
    /* 0x16 */ char reservedBytes16[2];
    /* 0x18 */ ParticleState *particles;
    /* 0x1C */ Vec *positions;
    /* 0x20 */ Vec2f *textureCoordinates;
    /* 0x24 */ GXColor *colors;
    /* 0x28 */ void *displayList;
    /* 0x2C */ Vec baseRotation;
    /* 0x38 */ float topLeftX;
    /* 0x3C */ float topRightX;
    /* 0x40 */ float bottomRightX;
    /* 0x44 */ float bottomLeftX;
    /* 0x48 */ float topLeftY;
    /* 0x4C */ float topRightY;
    /* 0x50 */ float bottomRightY;
    /* 0x54 */ float bottomLeftY;
    /* 0x58 */ float particleScale;
    /* 0x5C */ Vec2f *textureTiles;
    /* 0x60 */ float textureScaleX;
    /* 0x64 */ float textureScaleY;
} ParticleSystemRenderData; /* size = 0x68 */

GXColor whiteTintColor = { 0xFF, 0xFF, 0xFF, 0xFF };

OMOBJ *mapControllerObject;
MapState mapState;
CharacterEffectSlot characterEffectSlots[0x10];
ANIMDATA *mapEffectAnimationData;
MapQuadRenderData *mapQuadRenderData;
s16 teamPrimaryAnimationIds[2];
s16 teamSecondaryAnimationIds[2];
TeamMarkerRenderData *teamMarkerRenderData;
MapSurfaceRenderData mapSurfaceRenderState;
f32 mapAnimationFrame;
s8 mapEffectState;
s8 mapRenderState;

void LoadMapRouteData(void);
void UpdateMapController(OMOBJ *object);
void SetMapQuadVertices(f32 positionX, f32 positionY, f32 positionZ, f32 particleScale, f32 rotationAngle);
void RenderMapQuad(HU3DMODEL *model, Mtx matrix);
void RenderMapQuadWithOffset(HU3DMODEL *model, Mtx matrix);
void RenderMapQuadOverlay(HU3DMODEL *model, Mtx matrix);
void SetMapSurfaceAnimation(s32 animationType, s16 gridWidth, s16 gridHeight);
void BuildMapSurfaceDisplayList(MapSurfaceRenderData *surface);
void RenderMapSurface(HU3DMODEL *model, Mtx matrix);
void UpdateTeamMapState(OMOBJ *object);
void SetVectorFrame(Vec *firstPoint, Vec *secondPoint, Vec *thirdPoint, Vec *normal);
void RenderParticleSystem(HU3DMODEL *model, struct ParticleSystemRenderData *particleSystem, Mtx matrix);
s16 CreateMapAnimation(ANIMDATA *animationData, s16 animationType, float animationSpeed, s16 horizontalTileCount, s16 verticalTileCount);
void DestroyMapAnimation(s16 animationId);
void SetParticleRenderCallback(s16 animationId, ParticleRenderCallback renderCallback);
void SetParticleRenderMode(s16 animationId, u8 renderMode);

void UpdateMapEffects(OMOBJ *object);
void InitializeTeamRouteEffects(void);
void SetTeamMapPosition(TeamMapState *teamState, s16 teamIndex);
void UpdateTeamRouteEffects(void);
void SetTeamRouteState(s16 teamIndex, TeamMapState *teamState);
void UpdateTeamRouteModels(void);
void SetTeamRouteAnimation(s16 teamIndex, TeamMapState *teamState);
void UpdateRouteEffectState(TeamMapState *teamState, RouteEffectState *effectState, s16 teamIndex, float effectIntensity, float spriteOpacity);
void InitializeRouteEffect(u8 effectIndex, TeamMapState *teamState, RouteEffectState *effectState);
void UpdateRouteEffectController(OMOBJ *object);
void UpdateBannerAnimation(void);
void SetBannerAnimationFrame(s16 *animationIds, s16 animationFrame);
void SetBannerTextureScale(s32 primaryAnimationAsset, s32 secondaryAnimationAsset, float sourceTextureWidth, float sourceTextureHeight, float targetTextureWidth, float targetTextureHeight);
s16 CreateMapBanner(s32 assetId, s16 teamIndex, s16 animationId, float positionX, float positionY, float positionZ, float scale, float rotation);
void SetMapModelAnimation(s16 modelId, u8 animationIndex);
void SetMapModelAnimationSpeed(s16 modelId, u8 animationSpeed);
void SetMapModelColor(s16 modelId, u8 red, u8 green, u8 blue, u8 alpha);
void SetMapModelPosition(s16 modelId, float positionX, float positionY, float positionZ);
void SetMapModelRotation(s16 modelId, float rotationX, float rotationY, float rotationZ);
void SetMapModelScale(s16 modelId, float scaleX, float scaleY, float scaleZ);
void RenderMapEffectModel(HU3DMODEL *model, Mtx matrix);
void RenderMapEffectOverlay(HU3DMODEL *model, Mtx matrix);
s16 CreateTeamMarker(s32 assetId, s16 animationId, s16 modelIndex, s16 teamIndex, float markerScale, Vec *markerPosition, Vec *markerRotation);
void UpdateTeamMarker(TerrainWaveRenderData *terrain);
void UpdateTerrainWave(TerrainWaveRenderData *terrain, float waveAmplitude, Vec *waveDirection);
void RenderTerrainSurface(HU3DMODEL *model, Mtx matrix);
void RenderMapHighlight(HU3DMODEL *model, Mtx matrix);
void RenderMapShadow(HU3DMODEL *model, Mtx matrix);
void SetVectorComponents(Vec *vector, float x, float y, float z);
void SetVector2Components(Vec2f *vector, float x, float y);
s16 CreateMapLight(float x1, float y1, float z1, float x2, float y2, float z2, u8 r, u8 g, u8 b);
void SetMapLight(s16 modelId, s16 lightIndex, float x1, float y1, float z1, float x2, float y2, float z2, u8 r, u8 g, u8 b);
void ConfigureMapLight(HU3DLIGHT *light, Vec *lightPosition, Vec *lightTarget, GXColor *lightColor);
void CreateMapParticleSystem(u8 particleType, float positionX, float positionY, float positionZ, s16 particleCount, float scale, float rotation, s16 textureTileCount);
s16 CalculateParticleAnimationFrame(float elapsedFrames, float animationSpeed, float frameOffset, s16 frameCount);
ParticleState *CreateMapParticle(s16 particleType, Vec *position, Vec *velocity, float rotationAngle, GXColor *particleColor);
ParticleState *FindMapParticle(s16 animationId, s16 particleIndex);
void RenderMapParticles(HU3DMODEL *model, Mtx matrix);
void SetTextureScrollOffset(Vec2f *textureOffset, s16 textureTileX, s16 textureTileY, float scrollX, float scrollY);
void InterpolateTextureScroll(Vec2f *sourceOffset, Vec2f *targetOffset, s16 interpolationFrames, float scrollX, float scrollY);
void CalculateEffectDirection(Vec *sourcePosition, Vec *targetPosition, Vec *direction, s16 teamIndex);

void InitializeMapSystem(HUPROCESS *mapProcess)
{
    s16 modelSlotIndex;
    ANIMDATA *effectAnimationData;
    s32 primaryCameraId;
    s32 secondaryCameraId;

    mapState.mapReady = 0;
    mapControllerObject = omAddObjEx(mapProcess, 0x7F, 0, 0, -1, UpdateTeamMapState);
    mapQuadRenderData = HuMemDirectMallocNum(HEAP_HEAP, 0x10 * sizeof(MapQuadRenderData), HU_MEMNUM_OVL);

    for (modelSlotIndex = 0; modelSlotIndex < 0x10; modelSlotIndex++) {
        mapQuadRenderData[modelSlotIndex].cameraIndex = -1;
    }
    effectAnimationData = HuSprAnimRead(HuDataReadNum(0x3A0017, HU_MEMNUM_OVL));
    for (modelSlotIndex = 0; modelSlotIndex < 2; modelSlotIndex++) {
        teamPrimaryAnimationIds[modelSlotIndex] = CreateMapAnimation(effectAnimationData, 0x200, 80.0f, 0x40, 0x40);
        SetParticleRenderCallback(teamPrimaryAnimationIds[modelSlotIndex], RenderParticleSystem);
        SetParticleRenderMode(teamPrimaryAnimationIds[modelSlotIndex], 1);
        Hu3DModelLayerSet(teamPrimaryAnimationIds[modelSlotIndex], 7);
        if (modelSlotIndex == 0) {
            primaryCameraId = 1;
        }
        else {
            primaryCameraId = 2;
        }
        Hu3DModelCameraSet(teamPrimaryAnimationIds[modelSlotIndex], primaryCameraId);
    }
    effectAnimationData = HuSprAnimRead(HuDataReadNum(0x3A001A, HU_MEMNUM_OVL));
    for (modelSlotIndex = 0; modelSlotIndex < 2; modelSlotIndex++) {
        teamSecondaryAnimationIds[modelSlotIndex] = CreateMapAnimation(effectAnimationData, 4, 120.0f, 0, 0);
        SetParticleRenderMode(teamSecondaryAnimationIds[modelSlotIndex], 1);
        Hu3DModelLayerSet(teamSecondaryAnimationIds[modelSlotIndex], 7);
        if (modelSlotIndex == 0) {
            secondaryCameraId = 1;
        }
        else {
            secondaryCameraId = 2;
        }
        Hu3DModelCameraSet(teamSecondaryAnimationIds[modelSlotIndex], secondaryCameraId);
    }
    LoadMapRouteData();

    for (modelSlotIndex = 0; modelSlotIndex < 0x10; modelSlotIndex++) {
        characterEffectSlots[modelSlotIndex].activeState = -1;
        characterEffectSlots[modelSlotIndex].modelId = Hu3DModelCreateFile(0x3A0012);
        Hu3DModelAttrSet(characterEffectSlots[modelSlotIndex].modelId, 1);
        Hu3DModelScaleSet(characterEffectSlots[modelSlotIndex].modelId, 0.75f, 0.75f, 0.75f);
        Hu3DModelLayerSet(characterEffectSlots[modelSlotIndex].modelId, 4);
        Hu3DMotionSpeedSet(characterEffectSlots[modelSlotIndex].modelId, 2.0f);
        characterEffectSlots[modelSlotIndex].animationId = Hu3DAnimCreate(HuDataReadNum(0x3A001B, HU_MEMNUM_OVL), characterEffectSlots[modelSlotIndex].modelId, "db91");
        Hu3DAnimSpeedSet(characterEffectSlots[modelSlotIndex].animationId, 2.0f);
    }
}

void LoadMapRouteData(void)
{
    RoutePoint *routePointEntry = &mapState.routePoints[0];
    RouteEvent *routeEventEntry = &mapState.routeEvents[0];
    s16 routePointTypes[5] = { 0, 0, 1, 1, 0 };
    Vec routePointPositions[5] = { { -700.0f, -150.0f, -7000.0f }, { 0.0f, -300.0f, -12000.0f }, { 800.0f, -350.0f, -12700.0f }, { 600.0f, -350.0f, -13700.0f },
        { 825.0f, -450.0f, -15000.0f } };
    s16 routeTypes[3] = { 1, 1, 0 };
    s16 linkedRoutePointIndices[3][2] = { { 4, 5 }, { 8, -1 }, { 9, -1 } };
    s16 routeStateDurations[3][4] = {
        { REFRESH_RATE * 3,   REFRESH_RATE * 2, REFRESH_RATE, REFRESH_RATE },
        { REFRESH_RATE * 1.5, REFRESH_RATE * 3, REFRESH_RATE, REFRESH_RATE },
        { REFRESH_RATE * 1.5, REFRESH_RATE * 3, REFRESH_RATE, REFRESH_RATE },
    };
    Vec routeEventPositions[3] = { -350.0f, -300.0f, -10000.0f, -700.0f, -600.0f, -17000.0f, 700.0f, -600.0f, -19000.0f };
    s16 stateIndex;
    s16 entryIndex;

    for (entryIndex = 0; entryIndex < 5; entryIndex++, routePointEntry++) {
        routePointEntry->routePointType = routePointTypes[entryIndex];
        routePointEntry->worldPosition.x = routePointPositions[entryIndex].x;
        routePointEntry->worldPosition.y = routePointPositions[entryIndex].y;
        routePointEntry->worldPosition.z = routePointPositions[entryIndex].z;
    }

    for (entryIndex = 0; entryIndex < 3; entryIndex++, routeEventEntry++) {
        routeEventEntry->routeType = routeTypes[entryIndex];
        routeEventEntry->worldPosition.x = routeEventPositions[entryIndex].x;
        routeEventEntry->worldPosition.y = routeEventPositions[entryIndex].y;
        routeEventEntry->worldPosition.z = routeEventPositions[entryIndex].z;
        routeEventEntry->linkedPointIndices[0] = linkedRoutePointIndices[entryIndex][0];
        routeEventEntry->linkedPointIndices[1] = linkedRoutePointIndices[entryIndex][1];
        for (stateIndex = 0; stateIndex < 4; stateIndex++) {
            routeEventEntry->stateDurations[stateIndex] = routeStateDurations[entryIndex][stateIndex];
        }
    }
}

void DestroyMapSystem(void)
{
    MapQuadRenderData *mapRenderRecords;
    s16 teamIndex;
    TeamMapState *teamState;

    mapRenderRecords = mapQuadRenderData;
    HuSprAnimKill(mapState.bannerAnimationResources.primaryAnimation);
    HuSprAnimKill(mapState.bannerAnimationResources.secondaryAnimation);

    for (teamIndex = 0, teamState = &mapState.teamStates[0]; teamIndex < 2; teamIndex++, teamState++) {
        if (teamState->renderTarget.textureBuffer) {
            HuMemDirectFree(teamState->renderTarget.textureBuffer);
        }
    }
}

void PlayTeamRouteEffect(s8 effectIndex, s16 teamIndex)
{
    CharacterEffectSlot *effectSlot = &characterEffectSlots[0];
    TeamMapState *teamState = &mapState.teamStates[teamIndex];
    Vec effectSpawnOffsets[4] = { { 110.0f, -20.0f, 0.0f }, { -110.0f, -20.0f, 0.0f }, { 110.0f, -20.0f, 190.0f }, { -110.0f, -20.0f, 190.0f } };
    Mtx rotationMatrix;
    f32 effectFacingOffsets[4] = { 135.0f, 225.0f, 45.0f, 315.0f };
    Vec worldSpawnPosition;
    f32 teamFacingAngle;
    s16 effectSlotIndex;
    s32 cameraId;

    for (effectSlotIndex = 0; effectSlotIndex < 0x10; effectSlotIndex++, effectSlot++) {
        if (effectSlot->activeState == -1)
            break;
    }
    if (effectSlotIndex != 0x10) {
        effectSlot->activeState = effectIndex;
        teamFacingAngle = teamState->facingAngle;
        MTXRotRad(rotationMatrix, 0x59, MTXDegToRad(teamFacingAngle));
        MTXMultVec(rotationMatrix, &effectSpawnOffsets[effectIndex], &worldSpawnPosition);
        worldSpawnPosition.x += teamState->worldPosition.x;
        worldSpawnPosition.y += teamState->worldPosition.y;
        worldSpawnPosition.z += teamState->worldPosition.z;
        Hu3DModelAttrReset(effectSlot->modelId, 1);
        if (teamIndex == 0) {
            cameraId = 1;
        }
        else {
            cameraId = 2;
        }
        Hu3DModelCameraSet(effectSlot->modelId, cameraId);
        Hu3DModelPosSet(effectSlot->modelId, worldSpawnPosition.x, worldSpawnPosition.y, worldSpawnPosition.z);
        Hu3DModelRotSet(effectSlot->modelId, 0.0f, teamFacingAngle + effectFacingOffsets[effectIndex], 0.0f);
        Hu3DMotionTimeSet(effectSlot->modelId, 0.0f);
        if (effectIndex < 2) {
            Hu3DModelAmbSet(effectSlot->modelId, 1.0f, 1.0f, 1.0f);
        }
        else {
            Hu3DModelAmbSet(effectSlot->modelId, 0.5f, 0.5f, 0.5f);
        }
        Hu3DAnimBankSet(effectSlot->animationId, 0);
    }
}

void UpdateCharacterEffectAnimations(void)
{
    CharacterEffectSlot *effectSlot;
    s16 effectSlotIndex;
    f32 modelMotionEndTime;

    effectSlot = &characterEffectSlots[0];
    for (effectSlotIndex = 0; effectSlotIndex < 0x10; effectSlotIndex++, effectSlot++) {
        if (effectSlot->activeState != -1) {
            modelMotionEndTime = Hu3DMotionMaxTimeGet(effectSlot->modelId);
            if (Hu3DMotionTimeGet(effectSlot->modelId) >= modelMotionEndTime) {
                effectSlot->activeState = -1;
                Hu3DModelAttrSet(effectSlot->modelId, 1U);
            }
        }
    }
}

void ResetTeamMapRenderState(OMOBJ *mapObject)
{
    s32 teamCharacterModelIds[8];
    MapQuadRenderData *spriteRecord;
    TeamMapState *teamState;
    HU3DMODEL *characterModel;
    s16 modelOrLoopIndex;
    MapSurfaceRenderData *mapRenderState;
    s16 teamIndex;
    s16 modelIndex;

    mapRenderState = &mapSurfaceRenderState;
    for (teamIndex = 0, teamState = mapState.teamStates; teamIndex < 2; teamIndex++, teamState++) {
        modelIndex = findCharacterModelIndex(teamState->teamIndex, &teamCharacterModelIds[0]);

        for (modelOrLoopIndex = 0; modelOrLoopIndex < modelIndex; modelOrLoopIndex++) {
            characterModel = &Hu3DData[teamCharacterModelIds[modelOrLoopIndex]];
            characterModel->lLightId[4] = characterModel->lLightId[5] = characterModel->lLightId[6] = characterModel->lLightId[7] = -1;
        }

        for (modelOrLoopIndex = 0; modelOrLoopIndex < 2; modelOrLoopIndex++) {
            for (modelIndex = 0; modelIndex < 2; modelIndex++) {
                Hu3DModelKill(teamState->linkedLightModels[modelOrLoopIndex][modelIndex].modelId);
            }
        }
        for (modelOrLoopIndex = 0; modelOrLoopIndex < 2; modelOrLoopIndex++) {
            for (modelIndex = 0; modelIndex < 2; modelIndex++) {
                Hu3DModelKill(teamState->routeEffects[modelOrLoopIndex][modelIndex].modelId);
            }
        }
        HuSprAnimKill(teamState->particleHookData->animationData);
        Hu3DModelKill(teamState->lightModelId);
        Hu3DModelKill(teamState->particleModelId);
        HuMemDirectFree(teamState->renderTarget.textureBuffer);
        teamState->renderTarget.textureBuffer = NULL;
        DestroyMapAnimation(teamPrimaryAnimationIds[teamIndex]);
        DestroyMapAnimation(teamSecondaryAnimationIds[teamIndex]);

        for (modelOrLoopIndex = 0; modelOrLoopIndex < 11; modelOrLoopIndex++) {
            Hu3DModelKill(teamState->modelIds[modelOrLoopIndex]);
        }
        Hu3DModelKill(teamState->backgroundHookModelId);
        Hu3DModelKill(teamState->foregroundHookModelId);
    }
    for (teamIndex = 0, spriteRecord = mapQuadRenderData; teamIndex < 0x10; teamIndex++, spriteRecord++) {
        if (spriteRecord->cameraIndex != -1) {
            Hu3DModelKill(spriteRecord->modelId);
            HuSprAnimKill(spriteRecord->animationData);
            HuMemDirectFree(spriteRecord->displayList);
        }
    }
    HuMemDirectFree(mapQuadRenderData);
    Hu3DFogClear();
    modelOrLoopIndex = Hu3DModelCreateFile(0x3A0011);
    Hu3DModelCameraSet(modelOrLoopIndex, 1);
    Hu3DModelLayerSet(modelOrLoopIndex, 1);
    Hu3DModelPosSet(modelOrLoopIndex, 0.0f, 0.0f, 0.0f);
    mapRenderState->renderModelId = Hu3DModelCreateFile(0x3A0010);
    Hu3DModelCameraSet(mapRenderState->renderModelId, 1);
    Hu3DModelLayerSet(mapRenderState->renderModelId, 3);
    Hu3DModelPosSet(mapRenderState->renderModelId, 0.0f, 0.0f, 0.0f);
    mapState.mapReady = 1;
    mapRenderState->particleSpawnTimers[0] = 0;
    mapRenderState->quadCounterScratch = 0;
    mapRenderState->vertexPositions = 0;
    teamMarkerRenderData = HuMemDirectMallocNum(HEAP_HEAP, 0x10 * sizeof(TeamMarkerRenderData), HU_MEMNUM_OVL);
    for (teamIndex = 0; teamIndex < 0x10; teamIndex++) {
        teamMarkerRenderData[teamIndex].opacity = -1.0f;
    }
    SetMapSurfaceAnimation(0, 0x32, 0x32);
    mapObject->objFunc = UpdateMapController;
}

void UpdateMapController(OMOBJ *mapObject)
{
    f32 particleSpawnX;
    f32 particleSpawnY;
    f32 particleOpacity;
    TeamMarkerRenderData *particleRecord;
    s16 particleSlotIndex;
    MapSurfaceRenderData *mapRenderState;
    s32 mapControllerScratch;

    mapRenderState = &mapSurfaceRenderState;
    particleRecord = &teamMarkerRenderData[0];
    for (particleSlotIndex = 0; particleSlotIndex < 1; particleSlotIndex++) {
        if (--mapRenderState->particleSpawnTimers[particleSlotIndex] < 0.0f) {
            mapRenderState->particleSpawnTimers[particleSlotIndex] = (f32)((((rand8() << 8) | rand8()) % 241) + 0x78);
            particleSpawnX = ((((rand8() << 8) | rand8()) % 4001) - 0x7D0);
            particleSpawnY = -3000.0f;
            particleOpacity = ((((rand8() << 8) | rand8()) % 21) + 0xA);
            SetMapQuadVertices(particleSpawnX, 0.0f, particleSpawnY, particleOpacity, 0.0f);
        }
    }

    for (particleSlotIndex = 0; particleSlotIndex < 0x10; particleSlotIndex++, particleRecord++) {
        if (-1.0f != particleRecord->opacity) {
            particleRecord->collisionRadius += 20.0f;
            particleRecord->opacity -= 0.2f;
            if (particleRecord->opacity < 0.0f) {
                particleRecord->opacity = -1.0f;
            }
        }
    }
}

void SetMapQuadVertices(f32 positionX, f32 positionY, f32 positionZ, f32 particleScale, f32 rotationAngle)
{
    TeamMarkerRenderData *particleRecord;
    s16 particleSlotIndex;

    particleRecord = &teamMarkerRenderData[0];

    for (particleSlotIndex = 0; particleSlotIndex < 0x10; particleSlotIndex++, particleRecord++) {
        if (-1.0f == particleRecord->opacity)
            break;
    }
    if (particleSlotIndex != 0x10) {
        particleRecord->positionX = positionX;
        particleRecord->positionY = positionY;
        particleRecord->positionZ = positionZ;
        particleRecord->collisionRadius = rotationAngle;
        particleRecord->opacity = particleScale;
    }
}

f32 mapQuadScreenWidth = 450.0f;

f32 CalculateMapDistance(Vec *position)
{
    f32 horizontalDistanceX;
    f32 horizontalDistanceZ;
    f32 distanceFromParticle;
    f32 heightOffset;
    f32 distanceFromRadius;
    TeamMarkerRenderData *particleRecord;
    s16 particleSlotIndex;

    particleRecord = &teamMarkerRenderData[0];
    heightOffset = 0.0f;

    for (particleSlotIndex = 0; particleSlotIndex < 0x10; particleSlotIndex++, particleRecord++) {
        if (-1.0f != particleRecord->opacity) {
            horizontalDistanceX = position->x - particleRecord->positionX;
            horizontalDistanceZ = position->z - particleRecord->positionZ;
            if (FABS(horizontalDistanceX) > (particleRecord->collisionRadius + mapQuadScreenWidth)) {
                if (FABS(horizontalDistanceZ) > (particleRecord->collisionRadius + mapQuadScreenWidth)) {
                    continue;
                }
            }
            distanceFromParticle = sqrtf((horizontalDistanceX * horizontalDistanceX) + (horizontalDistanceZ * horizontalDistanceZ));
            distanceFromRadius = FABS(distanceFromParticle - particleRecord->collisionRadius);
            distanceFromParticle = 1.0f - (distanceFromRadius / mapQuadScreenWidth);
            if (distanceFromParticle < 0.0f) {
                distanceFromParticle = 0.0f;
            }
            heightOffset += particleRecord->opacity * sind(90.0f * distanceFromParticle);
        }
    }
    return heightOffset;
}

void RenderMapQuad(HU3DMODEL *model, Mtx matrix)
{
    Vec savedCameraCenter;
    Vec savedCameraRotation;
    Vec cameraPosition;
    Vec cameraUp;
    Vec cameraTarget;
    f32 cameraRoll;
    f32 savedCameraZoom;
    f32 cameraYaw;
    f32 cameraPitch;

    savedCameraCenter.x = CenterM->x;
    savedCameraCenter.y = CenterM->y;
    savedCameraCenter.z = CenterM->z;
    savedCameraRotation.x = CRotM->x;
    savedCameraRotation.y = CRotM->y;
    savedCameraRotation.z = CRotM->z;
    savedCameraZoom = *CZoomM;
    CenterM->x = -CenterM->x;
    CenterM->y = -CenterM->y;
    CenterM->z = CenterM->z;
    CRotM->x = 180.0f + CRotM->x;
    CRotM->y = 180.0f + -CRotM->y;
    CRotM->z = CRotM->z;
    *CZoomM = *CZoomM;
    cameraPitch = CRotM->x;
    cameraYaw = CRotM->y;
    cameraRoll = CRotM->z;
    cameraPosition.x = (CenterM->x + (*CZoomM * (sind(cameraYaw) * cosd(cameraPitch))));
    cameraPosition.y = (CenterM->y + (*CZoomM * -sind(cameraPitch)));
    cameraPosition.z = (CenterM->z + (*CZoomM * (cosd(cameraYaw) * cosd(cameraPitch))));
    cameraTarget.x = CenterM->x;
    cameraTarget.y = CenterM->y;
    cameraTarget.z = CenterM->z;
    cameraUp.x = (sind(cameraYaw) * sind(cameraPitch));
    cameraUp.y = cosd(cameraPitch);
    cameraUp.z = (cosd(cameraYaw) * sind(cameraPitch));
    Hu3DCameraPosSetV(1, &cameraPosition, &cameraUp, &cameraTarget);
    CenterM->x = savedCameraCenter.x;
    CenterM->y = savedCameraCenter.y;
    CenterM->z = savedCameraCenter.z;
    CRotM->x = savedCameraRotation.x;
    CRotM->y = savedCameraRotation.y;
    CRotM->z = savedCameraRotation.z;
    *CZoomM = savedCameraZoom;
}

void RenderMapQuadWithOffset(HU3DMODEL *model, Mtx matrix)
{
    Vec cameraPosition;
    Vec cameraTarget;
    Vec cameraUp;
    f32 cameraRoll;
    f32 cameraYaw;
    f32 cameraPitch;

    cameraPitch = CRotM->x;
    cameraYaw = CRotM->y;
    cameraRoll = CRotM->z;
    cameraPosition.x = CenterM->x + sind(cameraYaw) * cosd(cameraPitch) * *CZoomM;
    cameraPosition.y = CenterM->y + *CZoomM * -sind(cameraPitch);
    cameraPosition.z = CenterM->z + cosd(cameraYaw) * cosd(cameraPitch) * *CZoomM;
    cameraTarget.x = CenterM->x;
    cameraTarget.y = CenterM->y;
    cameraTarget.z = CenterM->z;
    cameraUp.x = sind(cameraYaw) * sind(cameraPitch);
    cameraUp.y = cosd(cameraPitch);
    cameraUp.z = cosd(cameraYaw) * sind(cameraPitch);
    Hu3DCameraPosSetV(1, &cameraPosition, &cameraUp, &cameraTarget);
    if (mapSurfaceRenderState.secondaryTextureBuffer) {
        GXSetTexCopySrc(0, 0, 0x280, 0x1E0);
        GXSetTexCopyDst(0x280, 0x1E0, GX_TF_RGB5A3, 0);
        GXCopyTex(mapSurfaceRenderState.secondaryTextureBuffer, 1);
    }
}

void RenderMapQuadOverlay(HU3DMODEL *model, Mtx matrix)
{
    if (mapSurfaceRenderState.textureBuffer) {
        GXSetTexCopySrc(0, 0, 0x280, 0x1E0);
        GXSetTexCopyDst(0x280, 0x1E0, GX_TF_RGB5A3, 0);
        GXCopyTex(mapSurfaceRenderState.textureBuffer, 0);
    }
}

void SetMapSurfaceAnimation(s32 animationType, s16 gridWidth, s16 gridHeight)
{
    s16 reservedRegisterValue12;
    s16 reservedRegisterValue10;
    s16 reservedRegisterValue0E;
    s16 reservedRegisterValue0C;
    s16 reservedRegisterValue0A;
    s16 reservedRegisterValue08;
    MapSurfaceRenderData *mapRenderState;
    GridQuad *quadRecord;
    s16 hookIdAndGridRow;
    HU3DMODEL *mapModel;
    s16 gridColumn;
    GridQuad *displayListSource;
    GridQuad *displayListWorkBuffer;

    mapRenderState = &mapSurfaceRenderState;
    hookIdAndGridRow = Hu3DHookFuncCreate(RenderMapSurface);
    Hu3DModelCameraSet(hookIdAndGridRow, 1);
    Hu3DModelLayerSet(hookIdAndGridRow, 5);
    mapModel = &Hu3DData[hookIdAndGridRow];
    hookIdAndGridRow = Hu3DHookFuncCreate(RenderMapQuad);
    Hu3DModelCameraSet(hookIdAndGridRow, 1);
    Hu3DModelLayerSet(hookIdAndGridRow, 0);
    hookIdAndGridRow = Hu3DHookFuncCreate(RenderMapQuadWithOffset);
    Hu3DModelCameraSet(hookIdAndGridRow, 1);
    Hu3DModelLayerSet(hookIdAndGridRow, 2);
    hookIdAndGridRow = Hu3DHookFuncCreate(RenderMapQuadOverlay);
    Hu3DModelCameraSet(hookIdAndGridRow, 1);
    Hu3DModelLayerSet(hookIdAndGridRow, 4);
    mapRenderState->textureOffsetX = mapRenderState->textureOffsetY = 0.0f;
    mapRenderState->secondaryTextureBufferSize = GXGetTexBufferSize(0x280, 0x1E0, 5, 0, 0);
    mapRenderState->secondaryTextureBuffer = HuMemDirectMallocNum(HEAP_MODEL, mapRenderState->secondaryTextureBufferSize, mapModel->mallocNo);
    mapRenderState->textureBufferSize = GXGetTexBufferSize(0x280, 0x1E0, 5, 0, 0);
    mapRenderState->textureBuffer = HuMemDirectMallocNum(HEAP_MODEL, mapRenderState->textureBufferSize, mapModel->mallocNo);
    mapRenderState->gridWidth = gridWidth + 1;
    mapRenderState->gridHeight = gridHeight + 1;
    mapRenderState->vertexCount = mapRenderState->gridWidth * mapRenderState->gridHeight;
    mapRenderState->vertexPositions = HuMemDirectMallocNum(HEAP_MODEL, mapRenderState->vertexCount * sizeof(Vec), mapModel->mallocNo);
    mapRenderState->textureCoordinates = HuMemDirectMallocNum(HEAP_MODEL, mapRenderState->vertexCount * sizeof(Vec2f), mapModel->mallocNo);
    mapRenderState->animatedPositions = HuMemDirectMallocNum(HEAP_MODEL, mapRenderState->vertexCount * sizeof(Vec), mapModel->mallocNo);
    mapRenderState->vertexColors = HuMemDirectMallocNum(HEAP_MODEL, mapRenderState->vertexCount * sizeof(Vec), mapModel->mallocNo);
    mapRenderState->gridQuads = HuMemDirectMallocNum(HEAP_MODEL, gridHeight * (gridWidth * sizeof(GridQuad)), mapModel->mallocNo);
    quadRecord = mapRenderState->gridQuads;
    displayListWorkBuffer = displayListSource = HuMemDirectMallocNum(HEAP_MODEL, 0x20000, mapModel->mallocNo);
    DCFlushRange(displayListSource, 0x20000);
    GXBeginDisplayList(displayListWorkBuffer, 0x20000);
    GXBegin(GX_QUADS, GX_VTXFMT0, (gridWidth * gridHeight * 4));

    for (hookIdAndGridRow = 0; hookIdAndGridRow < gridHeight; hookIdAndGridRow++) {
        for (gridColumn = 0; gridColumn < gridWidth; quadRecord++, gridColumn++) {
            quadRecord->firstVertexIndex = gridColumn + hookIdAndGridRow * mapRenderState->gridWidth;
            quadRecord->secondVertexIndex = (hookIdAndGridRow * mapRenderState->gridWidth) + gridColumn + 1;
            quadRecord->thirdVertexIndex = ((hookIdAndGridRow + 1) * mapRenderState->gridWidth) + gridColumn + 1;
            quadRecord->fourthVertexIndex = gridColumn + ((hookIdAndGridRow + 1) * mapRenderState->gridWidth);
            GXPosition1x16(quadRecord->firstVertexIndex);
            GXPosition1x16(quadRecord->firstVertexIndex);
            GXPosition1x16(quadRecord->firstVertexIndex);
            GXPosition1x16(quadRecord->secondVertexIndex);
            GXPosition1x16(quadRecord->secondVertexIndex);
            GXPosition1x16(quadRecord->secondVertexIndex);
            GXPosition1x16(quadRecord->thirdVertexIndex);
            GXPosition1x16(quadRecord->thirdVertexIndex);
            GXPosition1x16(quadRecord->thirdVertexIndex);
            GXPosition1x16(quadRecord->fourthVertexIndex);
            GXPosition1x16(quadRecord->fourthVertexIndex);
            GXPosition1x16(quadRecord->fourthVertexIndex);
            SetVectorFrame(&mapRenderState->vertexPositions[quadRecord->firstVertexIndex], &mapRenderState->vertexPositions[quadRecord->secondVertexIndex], &mapRenderState->vertexPositions[quadRecord->thirdVertexIndex], &quadRecord->vertexNormal);
        }
    }
    mapRenderState->displayListSize = GXEndDisplayList();
    mapRenderState->displayList = HuMemDirectMallocNum(HEAP_MODEL, mapRenderState->displayListSize, mapModel->mallocNo);
    memcpy(mapRenderState->displayList, displayListSource, mapRenderState->displayListSize);
    DCFlushRange(mapRenderState->displayList, mapRenderState->displayListSize);
    HuMemDirectFree(displayListSource);
    BuildMapSurfaceDisplayList(mapRenderState);
}

f32 disabledEffectOpacity = -1.0f;

void BuildMapSurfaceDisplayList(MapSurfaceRenderData *surface)
{
    Mtx cameraViewMatrix;
    Mtx cameraProjectionMatrix;
    Mtx cameraViewport;
    Vec2f gridZBounds;
    Vec2f gridXBounds;
    Vec2f rowXAndStep;
    Vec2f rowZAndStep;
    f32 projectedScreenX;
    f32 projectedScreenY;
    f32 projectedDepth;
    f32 waveAngleX;
    f32 waveAngleZ;
    f32 normalLength;
    GridQuad *gridQuad;
    Vec *gridVertexOrNormal;
    Vec *gridVertex;
    s16 vertexIndex;
    TeamMarkerRenderData *particleRecordScratch;
    Vec2f *textureCoordinate;
    s16 gridColumnOrQuadCount;
    HU3DCAMERA *mainCamera;
    s16 gridIndexScratch;

    mainCamera = &Hu3DCamera[0];
    if (disabledEffectOpacity == -1.0f) {
        mapAnimationFrame = 720.0f / surface->gridWidth;
        disabledEffectOpacity = 1440.0f / surface->gridHeight;
    }
    if (omPauseChk() == 0) {
        surface->rotationAngleX += 1.0f;
        surface->rotationAngleY += 1.5f;
    }
    if (surface->rotationAngleX >= 360.0f) {
        surface->rotationAngleX -= 360.0f;
    }
    if (surface->rotationAngleY >= 360.0f) {
        surface->rotationAngleY -= 360.0f;
    }
    gridVertex = surface->vertexPositions;
    gridVertexOrNormal = surface->animatedPositions;
    gridXBounds.x = 2000.0f;
    gridXBounds.y = -2000.0f;
    gridZBounds.x = 1000.0f;
    gridZBounds.y = -2500.0f;
    rowZAndStep.x = gridZBounds.x;
    rowZAndStep.y = (gridZBounds.y - gridZBounds.x) / (surface->gridHeight - 1);
    waveAngleZ = surface->rotationAngleY;

    for (vertexIndex = 0; vertexIndex < surface->gridHeight; vertexIndex++) {
        rowXAndStep.x = gridXBounds.x;
        rowXAndStep.y = (gridXBounds.y - gridXBounds.x) / (surface->gridWidth - 1);
        waveAngleX = surface->rotationAngleX;

        for (gridColumnOrQuadCount = 0; gridColumnOrQuadCount < surface->gridWidth; gridVertex++, gridVertexOrNormal++, gridColumnOrQuadCount++) {
            gridVertex->x = rowXAndStep.x;
            gridVertex->z = rowZAndStep.x;
            projectedScreenY = 5.0 * sind(waveAngleX) + 10.0 * sind(waveAngleZ);
            rowXAndStep.x += rowXAndStep.y;
            waveAngleX += mapAnimationFrame;
            if (waveAngleX >= 360.0f) {
                waveAngleX -= 360.0f;
            }
            gridVertex->y = projectedScreenY + CalculateMapDistance(gridVertex);
            gridVertexOrNormal->x = gridVertexOrNormal->y = gridVertexOrNormal->z = 0.0f;
        }
        rowZAndStep.x += rowZAndStep.y;
        waveAngleZ += disabledEffectOpacity;
        if (waveAngleZ >= 360.0f) {
            waveAngleZ -= 360.0f;
        }
    }
    gridQuad = surface->gridQuads;
    gridColumnOrQuadCount = (surface->gridWidth - 1) * (surface->gridHeight - 1);

    for (vertexIndex = 0; vertexIndex < gridColumnOrQuadCount; vertexIndex++, gridQuad++) {
        SetVectorFrame(&surface->vertexPositions[gridQuad->firstVertexIndex], &surface->vertexPositions[gridQuad->secondVertexIndex], &surface->vertexPositions[gridQuad->thirdVertexIndex], &gridQuad->vertexNormal);
        surface->animatedPositions[gridQuad->firstVertexIndex].x += gridQuad->vertexNormal.x;
        surface->animatedPositions[gridQuad->firstVertexIndex].y += gridQuad->vertexNormal.y;
        surface->animatedPositions[gridQuad->firstVertexIndex].z += gridQuad->vertexNormal.z;
        surface->animatedPositions[gridQuad->secondVertexIndex].x += gridQuad->vertexNormal.x;
        surface->animatedPositions[gridQuad->secondVertexIndex].y += gridQuad->vertexNormal.y;
        surface->animatedPositions[gridQuad->secondVertexIndex].z += gridQuad->vertexNormal.z;
        surface->animatedPositions[gridQuad->thirdVertexIndex].x += gridQuad->vertexNormal.x;
        surface->animatedPositions[gridQuad->thirdVertexIndex].y += gridQuad->vertexNormal.y;
        surface->animatedPositions[gridQuad->thirdVertexIndex].z += gridQuad->vertexNormal.z;
        surface->animatedPositions[gridQuad->fourthVertexIndex].x += gridQuad->vertexNormal.x;
        surface->animatedPositions[gridQuad->fourthVertexIndex].y += gridQuad->vertexNormal.y;
        surface->animatedPositions[gridQuad->fourthVertexIndex].z += gridQuad->vertexNormal.z;
    }

    gridVertexOrNormal = surface->animatedPositions;
    for (vertexIndex = 0; vertexIndex < surface->vertexCount; vertexIndex++, gridVertexOrNormal++) {
        normalLength = sqrtf((gridVertexOrNormal->x * gridVertexOrNormal->x) + (gridVertexOrNormal->y * gridVertexOrNormal->y) + (gridVertexOrNormal->z * gridVertexOrNormal->z));
        gridVertexOrNormal->x /= normalLength;
        gridVertexOrNormal->y /= normalLength;
        gridVertexOrNormal->z /= normalLength;
    }
    gridVertex = surface->vertexPositions;
    textureCoordinate = surface->textureCoordinates;
    C_MTXLookAt(cameraViewMatrix, &mainCamera->pos, &mainCamera->up, &mainCamera->target);
    GXGetProjectionv(&cameraProjectionMatrix[0][0]);
    GXGetViewportv(&cameraViewport[0][0]);

    for (vertexIndex = 0; vertexIndex < surface->vertexCount; vertexIndex++, gridVertex++, textureCoordinate++) {
        GXProject(gridVertex->x, gridVertex->y, gridVertex->z, cameraViewMatrix, &cameraProjectionMatrix[0][0], &cameraViewport[0][0], &projectedScreenX, &projectedScreenY, &projectedDepth);
        textureCoordinate->x = 0.0015625f * projectedScreenX;
        textureCoordinate->y = 0.0020833f * projectedScreenY;
        textureCoordinate->x += 0.05f * surface->animatedPositions[vertexIndex].x;
        textureCoordinate->y += 0.05f * surface->animatedPositions[vertexIndex].z;
        surface->vertexColors[vertexIndex].r = 0xFF;
        surface->vertexColors[vertexIndex].g = 0xFF;
        surface->vertexColors[vertexIndex].b = 0xFF;
        surface->vertexColors[vertexIndex].a = 0xFF;
    }
    DCFlushRangeNoSync(surface->vertexPositions, surface->vertexCount * 0xC);
    DCFlushRangeNoSync(surface->animatedPositions, surface->vertexCount * 0xC);
    DCFlushRangeNoSync(surface->textureCoordinates, surface->vertexCount * 8);
    DCFlushRangeNoSync(surface->vertexColors, surface->vertexCount * 4);
    PPCSync();
}

void RenderMapSurface(HU3DMODEL *model, Mtx matrix)
{
    Mtx indirectTextureMatrix;
    Mtx textureRotationMatrix;
    GXTexObj textureObject;
    GXColor environmentTint;
    MapSurfaceRenderData *mapRenderState = &mapSurfaceRenderState;
    BannerAnimationSet *bannerAnimations = &mapState.bannerAnimationResources;

    BuildMapSurfaceDisplayList(mapRenderState);
    GXLoadPosMtxImm(matrix, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0);
    GXSetArray(GX_VA_POS, mapRenderState->vertexPositions, 0xC);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_RGBA6, 0);
    GXSetArray(GX_VA_TEX0, mapRenderState->textureCoordinates, 8);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_TEX_ST, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, mapRenderState->vertexColors, 4);
    GXInitTexObj(&textureObject, mapRenderState->secondaryTextureBuffer, 0x280, 0x1E0, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, 0);
    GXInitTexObjLOD(&textureObject, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
    GXLoadTexObj(&textureObject, GX_TEXMAP0);
    GXInitTexObj(&textureObject, mapRenderState->textureBuffer, 0x280, 0x1E0, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, 0);
    GXInitTexObjLOD(&textureObject, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
    GXLoadTexObj(&textureObject, GX_TEXMAP1);
    HuSprTexLoad(bannerAnimations->primaryAnimation, 0, 2, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetNumTevStages(3);
    environmentTint.r = 0;
    environmentTint.g = 0x10;
    environmentTint.b = 0x1E;
    environmentTint.a = 0x4C;
    GXSetTevColor(GX_TEVREG2, environmentTint);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ONE, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG0);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG0);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP1, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_TEXC, GX_CC_C0, GX_CC_A2, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG1);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG1);
    GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD0, GX_TEXMAP1, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_C1, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C2);
    GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumIndStages(1);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD1, GX_TEXMAP2);
    GXSetTevIndWarp(GX_TEVSTAGE0, GX_INDTEXSTAGE0, 1, 0, GX_ITM_0);
    GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
    MTXScale(indirectTextureMatrix, -0.1f, 0.0f, 1.0f);
    GXSetIndTexMtx(GX_ITM_0, (f32(*)[3])(indirectTextureMatrix), 0);
    GXSetNumTexGens(2);
    if (omPauseChk() == 0) {
        mapRenderState->textureOffsetX += 0.001f;
        mapRenderState->textureOffsetY += 0.0005f;
    }
    MTXRotRad(indirectTextureMatrix, 0x58, 1.5707964f);
    MTXScale(textureRotationMatrix, 0.001f, 0.001f, 0.001f);
    MTXConcat(indirectTextureMatrix, textureRotationMatrix, indirectTextureMatrix);
    mtxTransCat(indirectTextureMatrix, mapRenderState->textureOffsetX, mapRenderState->textureOffsetY, 0.0f);
    GXLoadTexMtxImm(indirectTextureMatrix, 0x1E, GX_MTX2x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, GX_FALSE, 0x7D);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_POS, 0x1E, GX_FALSE, 0x7D);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZCompLoc(0);
    GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXCallDisplayList(mapRenderState->displayList, mapRenderState->displayListSize);
    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevDirect(GX_TEVSTAGE1);
}

s32 mapAssetIds[0xC] = {
    0x3A0000,
    0x3A0001,
    0x3A0002,
    0x3A0003,
    0x3A0004,
    0x3A0005,
    0x3A0006,
    0x3A0007,
    0x3A0008,
    0x3A0009,
    0x3A000A,
    0x3A000B,
};
s32 bannerAssetIds[2] = { 0x3A000C, 0x3A000D };
s32 bannerTextureParameters[4] = { 0x3A000E, 0x3A000F, 0x3F800000, 0x01000000 };

void UpdateTeamMapState(OMOBJ *object)
{
    s16 teamRenderModelIds[2][0x20];
    s16 teamRenderModelCount[2];
    s16 routeEffectModelIds[2];
    s16 linkedLightModelIds[2];
    TeamMapState *teamState;
    s16 modelOrEffectIndex;
    RouteEffectState *routeEffect;
    s16 teamIndex;
    RouteLightModelLink *linkedLightModel;
    s16 modelLinkIndex;
    TeamRenderTarget *bannerRenderTarget;
    MapState *mapState;
    u16 modelHandleScratch;

    mapState = &mapState;
    Hu3DFogSet(3500.0f, 20000.0f, 0, 0, 0);
    for (teamIndex = 0, teamState = &mapState->teamStates[0]; teamIndex < 2; teamIndex++, teamState++) {
        teamState->teamIndex = teamIndex;
        teamRenderModelCount[teamIndex] = 0;
        for (modelOrEffectIndex = 0; modelOrEffectIndex < 3; modelOrEffectIndex++) {
            teamState->activeRouteEffectCounts[modelOrEffectIndex] = 0;
        }
        for (modelOrEffectIndex = 0; modelOrEffectIndex < 0xB; modelOrEffectIndex++) {
            teamState->modelIds[modelOrEffectIndex] = Hu3DModelCreateFile(mapAssetIds[modelOrEffectIndex]);
            teamState->routeEffectIndices[modelOrEffectIndex] = -1;
            if (modelOrEffectIndex < 0xA) {
                Hu3DModelLayerSet(teamState->modelIds[modelOrEffectIndex], 2);
            }
            else {
                Hu3DModelLayerSet(teamState->modelIds[modelOrEffectIndex], 5);
            }
            Hu3DModelAmbSet(teamState->modelIds[modelOrEffectIndex], 0.1f, 0.1f, 0.1f);
            if (teamIndex == 0) {
                Hu3DModelCameraSet(teamState->modelIds[modelOrEffectIndex], 1);
            }
            else {
                Hu3DModelCameraSet(teamState->modelIds[modelOrEffectIndex], 2);
            }
            teamRenderModelIds[teamIndex][teamRenderModelCount[teamIndex]++] = teamState->modelIds[modelOrEffectIndex];
        }
        modelOrEffectIndex = Hu3DHookFuncCreate(RenderMapHighlight);
        teamState->backgroundHookModelId = modelOrEffectIndex;
        Hu3DModelLayerSet(modelOrEffectIndex, 4);
        if (teamIndex == 0) {
            Hu3DModelCameraSet(modelOrEffectIndex, 1);
        }
        else {
            Hu3DModelCameraSet(modelOrEffectIndex, 2);
        }
        modelOrEffectIndex = Hu3DHookFuncCreate(RenderMapShadow);
        teamState->foregroundHookModelId = modelOrEffectIndex;
        Hu3DModelLayerSet(modelOrEffectIndex, 6);
        if (teamIndex == 0) {
            Hu3DModelCameraSet(modelOrEffectIndex, 1);
        }
        else {
            Hu3DModelCameraSet(modelOrEffectIndex, 2);
        }
        for (modelOrEffectIndex = 0; modelOrEffectIndex < 2; modelOrEffectIndex++) {
            routeEffect = teamState->routeEffects[modelOrEffectIndex];
            for (modelLinkIndex = 0; modelLinkIndex < 2; modelLinkIndex++, routeEffect++) {
                if ((teamIndex == 0) && (modelLinkIndex == 0)) {
                    routeEffect->modelId = routeEffectModelIds[modelOrEffectIndex] = Hu3DModelCreateFile(bannerTextureParameters[modelOrEffectIndex]);
                }
                else {
                    routeEffect->modelId = Hu3DModelLink(routeEffectModelIds[modelOrEffectIndex]);
                }
                Hu3DModelAttrSet(routeEffect->modelId, HU3D_ATTR_DISPOFF);
                Hu3DModelLayerSet(routeEffect->modelId, 2);
                Hu3DModelAmbSet(routeEffect->modelId, 0.0f, 0.0f, 0.0f);
                if (teamIndex == 0) {
                    Hu3DModelCameraSet(routeEffect->modelId, 1);
                }
                else {
                    Hu3DModelCameraSet(routeEffect->modelId, 2);
                }
                teamRenderModelIds[teamIndex][teamRenderModelCount[teamIndex]++] = routeEffect->modelId;
                routeEffect->spriteId = CreateMapBanner(0x3A0018, teamIndex, 0, 0.0f, -400.0f, 700.0f, 800.0f, -5.0f);
                SetMapModelAnimation(routeEffect->spriteId, 1);
                SetMapModelColor(routeEffect->spriteId, 0xFF, 0xFF, 0xFF, 0);
                routeEffect->lightIndex = 2 * modelOrEffectIndex + 4 + modelLinkIndex;
                SetMapLight(routeEffect->modelId, routeEffect->lightIndex, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFF, 0xA0, 0x60);
                Hu3DLLightStaticSet(routeEffect->modelId, routeEffect->lightIndex, 1);
                Hu3DLLightPointSet(routeEffect->modelId, routeEffect->lightIndex, 1.0f, 0.6f, 3);
            }
        }
        for (modelOrEffectIndex = 0; modelOrEffectIndex < 2; modelOrEffectIndex++) {
            linkedLightModel = teamState->linkedLightModels[modelOrEffectIndex];
            for (modelLinkIndex = 0; modelLinkIndex < 2; modelLinkIndex++, linkedLightModel++) {
                if ((teamIndex == 0) && (modelLinkIndex == 0)) {
                    linkedLightModel->modelId = linkedLightModelIds[modelOrEffectIndex] = Hu3DModelCreateFile(bannerAssetIds[modelOrEffectIndex]);
                }
                else {
                    linkedLightModel->modelId = Hu3DModelLink(linkedLightModelIds[modelOrEffectIndex]);
                }
                Hu3DModelAttrSet(linkedLightModel->modelId, HU3D_ATTR_DISPOFF);
                Hu3DModelLayerSet(linkedLightModel->modelId, 2);
                Hu3DModelAmbSet(linkedLightModel->modelId, 0.0f, 0.0f, 0.0f);
                if (teamIndex == 0) {
                    Hu3DModelCameraSet(linkedLightModel->modelId, 1);
                }
                else {
                    Hu3DModelCameraSet(linkedLightModel->modelId, 2);
                }
                teamRenderModelIds[teamIndex][teamRenderModelCount[teamIndex]++] = linkedLightModel->modelId;
            }
        }
        for (modelOrEffectIndex = 0; modelOrEffectIndex < 4; modelOrEffectIndex++) {
            teamState->effectIds[modelOrEffectIndex] = CreateMapLight(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFF, 0xFF, 0xFF);
            teamState->effectAngles[modelOrEffectIndex] = -1.0f;
        }
    }
    for (teamIndex = 0, teamState = mapState->teamStates; teamIndex < 2; teamIndex++, teamState++) {
        teamState->highlightLightId = Hu3DLLightCreate(teamState->modelIds[0], 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xFA, 0xF0, 0xD2);
        Hu3DLLightSpotSet(teamState->modelIds[0], teamState->highlightLightId, 37.5f, 4);
        Hu3DLLightStaticSet(teamState->modelIds[0], teamState->highlightLightId, 1);
        teamState->cameraFocusPosition.x = 0.0f;
        teamState->cameraFocusPosition.y = 0.0f;
        teamState->cameraFocusPosition.z = 0.0f;
        teamState->worldPosition.x = 0.0f;
        teamState->worldPosition.y = 0.0f;
        teamState->worldPosition.z = 0.0f;
        teamState->facingAngle = 0.0f;
    }
    SetBannerTextureScale(0x3A0016, 0x3A0014, 2000.0f, 6000.0f, 4.0f, 9.0f);
    for (teamIndex = 0, teamState = mapState->teamStates; teamIndex < 2; teamIndex++, teamState++) {
        teamState->bannerModelId[0] = CreateMapBanner(0x3A0015, teamIndex, 0.0f, 0.0f, -900.0f, 500.0f, 700.0f, -10.0f);
        bannerRenderTarget = &teamState->renderTarget;
        bannerRenderTarget->cameraIndex = teamIndex;
        bannerRenderTarget->textureBufferSize = GXGetTexBufferSize(0x140, 0x1E0, 5, 0, 0);
        bannerRenderTarget->textureBuffer = HuMemDirectMalloc(HEAP_MODEL, bannerRenderTarget->textureBufferSize);
        modelOrEffectIndex = Hu3DHookFuncCreate(RenderMapEffectOverlay);
        teamState->particleModelId = modelOrEffectIndex;
        Hu3DModelCameraSet(modelOrEffectIndex, teamIndex ? 2 : 1);
        Hu3DModelLayerSet(modelOrEffectIndex, 1);
        Hu3DData[modelOrEffectIndex].hookData = bannerRenderTarget;
        teamState->lightModelId = CreateTeamMarker(0x3A0013, 0xA, 0x1E, teamIndex, 6000.0f, baseRightRouteEdge, baseLeftRouteEdge);
        SetMapLight(teamState->lightModelId, 1, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0xC8, 0xF0, 0xA0);
        Hu3DLLightStaticSet(teamState->lightModelId, 1, 1);
        Hu3DLLightPointSet(teamState->lightModelId, 1, 125.0f, 0.5f, 3);
        SetMapLight(teamState->lightModelId, 2, 0.0f, -500.0f, -23500.0f, 0.0f, 0.0f, 0.0f, 0xFA, 0xF0, 0xD2);
        Hu3DLLightStaticSet(teamState->lightModelId, 2, 1);
        Hu3DLLightPointSet(teamState->lightModelId, 2, 800.0f, 0.5f, 3);
        teamRenderModelIds[teamIndex][teamRenderModelCount[teamIndex]++] = teamState->lightModelId;
        SetBannerAnimationFrame(teamRenderModelIds[teamIndex], teamRenderModelCount[teamIndex]);
    }
    object->objFunc = UpdateMapEffects;
}

void UpdateMapEffects(OMOBJ *object)
{
    float effectMotionEndTime;
    CharacterEffectSlot *effectSlot;
    s16 effectSlotIndex;

    if (getGameplayState() == 7) {
        object->objFunc = ResetTeamMapRenderState;
    }
    InitializeTeamRouteEffects();
    UpdateTeamRouteEffects();
    UpdateTeamRouteModels();
    UpdateRouteEffectController(object);
    UpdateBannerAnimation();
    for (effectSlot = characterEffectSlots, effectSlotIndex = 0; effectSlotIndex < 0x10; effectSlotIndex++, effectSlot++) {
        if (effectSlot->activeState != -1) {
            effectMotionEndTime = Hu3DMotionMaxTimeGet(effectSlot->modelId);
            if (Hu3DMotionTimeGet(effectSlot->modelId) >= effectMotionEndTime) {
                effectSlot->activeState = -1;
                Hu3DModelAttrSet(effectSlot->modelId, HU3D_ATTR_DISPOFF);
            }
        }
    }
}

void InitializeTeamRouteEffects(void)
{
    float lightPulseOffset;
    TeamMapState *teamState;
    ParticleState *activeSpotEffect;
    s16 characterIndex;
    HU3DMODEL *characterModel;
    s16 teamIndex;

    for (teamIndex = 0, teamState = mapState.teamStates; teamIndex < 2; teamIndex++, teamState++) {
        for (characterIndex = 0; characterIndex < 0xA; characterIndex++) {
            characterModel = &Hu3DData[teamState->modelIds[characterIndex]];
            activeSpotEffect = FindMapParticle(teamSecondaryAnimationIds[teamState->teamIndex], teamState->lightSlotIndices[teamState->routeEffectIndices[characterIndex]]);
            lightPulseOffset = -2000.0f * characterIndex;
            if (((teamState->worldPosition.z - 25000.0f) < lightPulseOffset) && ((3000.0f + teamState->worldPosition.z) > lightPulseOffset)) {
                if (teamState->routeEffectIndices[characterIndex] == -1) {
                    SetTeamMapPosition(teamState, characterIndex);
                    characterModel->attr &= 0xFFFFFFFE;
                }
                else {
                    if (activeSpotEffect) {
                        activeSpotEffect->scale += 5.0f;
                        if (activeSpotEffect->scale >= 360.0f) {
                            activeSpotEffect->scale -= 360.0f;
                        }
                        activeSpotEffect->rotation.x = activeSpotEffect->rotation.y = activeSpotEffect->rotation.z = 1.0 + (0.05000000074505806 * sind(activeSpotEffect->scale));
                    }
                    teamState->effectAngles[teamState->routeEffectIndices[characterIndex]] += 5.0f;
                    if (teamState->effectAngles[teamState->routeEffectIndices[characterIndex]] >= 360.0f) {
                        teamState->effectAngles[teamState->routeEffectIndices[characterIndex]] -= 360.0f;
                    }
                    lightPulseOffset = 50.0 * sind(teamState->effectAngles[teamState->routeEffectIndices[characterIndex]]);
                    lightPulseOffset = lightPulseOffset;
                    Hu3DLLightPointSet(teamState->modelIds[characterIndex], 1, 400.0f + lightPulseOffset, 0.5f, 3);
                    if (characterIndex == 0) {
                        activeSpotEffect = FindMapParticle(teamSecondaryAnimationIds[teamState->teamIndex], teamState->spotlightId);
                        if (activeSpotEffect->scale >= 360.0f) {
                            activeSpotEffect->scale -= 360.0f;
                        }
                        activeSpotEffect->rotation.x = activeSpotEffect->rotation.y = activeSpotEffect->rotation.z = 1.0 + (0.05000000074505806 * sind(activeSpotEffect->scale));
                        teamState->effectAngles[1] += 5.0f;
                        if (teamState->effectAngles[1] >= 360.0f) {
                            teamState->effectAngles[1] -= 360.0f;
                        }
                        lightPulseOffset = 50.0 * sind(teamState->effectAngles[1]);
                        lightPulseOffset = lightPulseOffset;
                        Hu3DLLightPointSet(teamState->modelIds[characterIndex], 2, 400.0f + lightPulseOffset, 0.5f, 3);
                    }
                }
            }
            else {
                if (teamState->routeEffectIndices[characterIndex] != -1) {
                    if (activeSpotEffect) {
                        activeSpotEffect->active = 0;
                    }
                    characterModel->lLightId[1] = -1;
                    teamState->effectAngles[teamState->routeEffectIndices[characterIndex]] = -1.0f;
                    teamState->routeEffectIndices[characterIndex] = -1;
                    if (characterIndex == 0) {
                        activeSpotEffect = FindMapParticle(teamSecondaryAnimationIds[teamState->teamIndex], teamState->spotlightId);
                        activeSpotEffect->active = 0;
                        characterModel->lLightId[2] = -1;
                        teamState->effectAngles[1] = -1.0f;
                    }
                }
                characterModel->attr |= 1;
            }
        }
    }
}

Vec mapRoutePositions[0xC] = {
    { -670.0f, 400.0f, 1750.0f },
    { 930.0f, 400.0f, -1000.0f },
    { -930.0f, 400.0f, -3000.0f },
    { 230.0f, 400.0f, -5000.0f },
    { -1630.0f, 300.0f, -7000.0f },
    { 530.0f, 100.0f, -9000.0f },
    { -530.0f, 100.0f, -11000.0f },
    { 1630.0f, 100.0f, -13000.0f },
    { -230.0f, 0.0f, -15000.0f },
    { 930.0f, -200.0f, -17000.0f },
    { -930.0f, -200.0f, -19000.0f },
    { 930.0f, -200.0f, -21000.0f },
};

void SetTeamMapPosition(TeamMapState *teamState, s16 teamIndex)
{
    Vec lightPosition;
    s16 effectSlotIndex;
    HU3DMODEL *characterModel;

    characterModel = &Hu3DData[teamState->modelIds[teamIndex]];
    {
        Vec zeroLightDirection = { 0.0f, 0.0f, 0.0f };
        if (teamIndex != 0xA) {
            for (effectSlotIndex = 0; effectSlotIndex < 4; effectSlotIndex++) {
                if (teamState->effectAngles[effectSlotIndex] == -1.0f) {
                    break;
                }
            }
            if (effectSlotIndex != 4) {
                characterModel->lLightId[1] = teamState->effectIds[effectSlotIndex];
                teamState->effectAngles[effectSlotIndex] = 0.0f;
                teamState->routeEffectIndices[teamIndex] = effectSlotIndex;
                if (characterModel->lLightId[1] != -1) {
                    lightPosition.x = mapRoutePositions[teamIndex + 1].x;
                    lightPosition.y = mapRoutePositions[teamIndex + 1].y;
                    lightPosition.z = mapRoutePositions[teamIndex + 1].z;
                    Hu3DLLightPointSet(teamState->modelIds[teamIndex], 1, 400.0f, 0.5f, 3);
                    Hu3DLLightPosSetV(teamState->modelIds[teamIndex], 1, &lightPosition, &zeroLightDirection);
                }
                teamState->lightSlotIndices[effectSlotIndex] = CalculateParticleAnimationFrame(lightPosition.x, lightPosition.y, lightPosition.z, teamSecondaryAnimationIds[teamState->teamIndex]);
                if (teamIndex == 0) {
                    effectSlotIndex = 1;
                    characterModel->lLightId[2] = teamState->effectIds[effectSlotIndex];
                    teamState->effectAngles[effectSlotIndex] = 0.0f;
                    lightPosition.x = mapRoutePositions[0].x;
                    lightPosition.y = mapRoutePositions[0].y;
                    lightPosition.z = mapRoutePositions[0].z;
                    Hu3DLLightPointSet(teamState->modelIds[0], 2, 400.0f, 0.5f, 3);
                    Hu3DLLightPosSetV(teamState->modelIds[0], 2, &lightPosition, &zeroLightDirection);
                    teamState->spotlightId = CalculateParticleAnimationFrame(lightPosition.x, lightPosition.y, lightPosition.z, teamSecondaryAnimationIds[teamState->teamIndex]);
                }
            }
        }
    }
}

float routeEffectScaleBounds[2] = { 50.0f, 80.0f };

void UpdateTeamRouteEffects(void)
{
    Vec linkedModelPosition;
    TeamMapState *teamState;
    HU3DMODEL *linkedModel;
    s16 teamOrRoutePointIndex;
    s16 linkedModelGroup;
    RouteLightModelLink *linkedLightModel;
    RoutePoint *routePoint;
    s16 linkedModelIndex;

    for (routePoint = mapState.routePoints, teamOrRoutePointIndex = 0; teamOrRoutePointIndex < 5; teamOrRoutePointIndex++, routePoint++) {
        for (linkedModelGroup = 0, teamState = mapState.teamStates; linkedModelGroup < 2; linkedModelGroup++, teamState++) {
            if ((teamState->activeRoutePointModels[teamOrRoutePointIndex] == 0) && ((teamState->worldPosition.z - 7000.0f) < routePoint->worldPosition.z)
                && ((1000.0f + teamState->worldPosition.z) > routePoint->worldPosition.z)) {
                SetTeamRouteState(teamOrRoutePointIndex, teamState);
            }
        }
    }
    for (teamOrRoutePointIndex = 0, teamState = mapState.teamStates; teamOrRoutePointIndex < 2; teamOrRoutePointIndex++, teamState++) {
        for (linkedModelGroup = 0; linkedModelGroup < 2; linkedModelGroup++) {
            linkedLightModel = teamState->linkedLightModels[linkedModelGroup];
            for (linkedModelIndex = 0; linkedModelIndex < 2; linkedModelIndex++, linkedLightModel++) {
                linkedModel = &Hu3DData[linkedLightModel->modelId];
                if ((linkedModel->attr & 1) == 0) {
                    if (((teamState->worldPosition.z - 7000.0f) > linkedModel->pos.z) || ((1000.0f + teamState->worldPosition.z) < linkedModel->pos.z)) {
                        linkedModel->attr |= 1;
                        teamState->activeRoutePointModels[linkedLightModel->routePointIndex] = 0;
                    }
                    else {
                        linkedModelPosition.x = linkedModel->pos.x;
                        linkedModelPosition.z = linkedModel->pos.z;
                        TestRouteCircleCollision(linkedModelPosition, routeEffectScaleBounds[linkedModelGroup], 0, 1, teamState->teamIndex);
                    }
                }
            }
        }
    }
}

void SetTeamRouteState(s16 teamIndex, TeamMapState *teamState)
{
    RouteLightModelLink *linkedLightModel;
    RoutePoint *routePoint;
    s16 modelLinkIndex;
    HU3DMODEL *linkedModel;

    routePoint = &mapState.routePoints[teamIndex];
    linkedLightModel = teamState->linkedLightModels[routePoint->routePointType];
    for (modelLinkIndex = 0; modelLinkIndex < 2; modelLinkIndex++, linkedLightModel++) {
        linkedModel = &Hu3DData[linkedLightModel->modelId];
        if ((linkedModel->attr & 1) != 0) {
            Hu3DModelAttrReset(linkedLightModel->modelId, HU3D_ATTR_DISPOFF);
            Hu3DModelPosSet(linkedLightModel->modelId, routePoint->worldPosition.x, routePoint->worldPosition.y, routePoint->worldPosition.z);
            linkedLightModel->routePointIndex = teamIndex;
            teamState->activeRoutePointModels[teamIndex] = 1;
            return;
        }
    }
}

void UpdateTeamRouteModels(void)
{
    Vec effectPosition;
    float lightPulseOffset;
    RouteEffectState *routeEffect;
    TeamMapState *teamState;
    HU3DMODEL *effectModel;
    s16 teamIndex;
    s16 routeEventIndex;
    RouteEvent *routeEvent;
    s16 effectModelIndex;
    s32 soundEffectId;
    s32 soundSlotIndex;

    for (routeEvent = mapState.routeEvents, routeEventIndex = 0; routeEventIndex < 3; routeEventIndex++, routeEvent++) {
        for (teamIndex = 0, teamState = mapState.teamStates; teamIndex < 2; teamIndex++, teamState++) {
            if ((teamState->activeRouteEffectCounts[routeEventIndex] == 0) && ((teamState->worldPosition.z - 7000.0f) < routeEvent->worldPosition.z)
                && ((1000.0f + teamState->worldPosition.z) > routeEvent->worldPosition.z)) {
                SetTeamRouteAnimation(routeEventIndex, teamState);
            }
        }
    }
    for (routeEventIndex = 0, teamState = mapState.teamStates; routeEventIndex < 2; routeEventIndex++, teamState++) {
        for (teamIndex = 0; teamIndex < 2; teamIndex++) {
            routeEffect = teamState->routeEffects[teamIndex];
            for (effectModelIndex = 0; effectModelIndex < 2; effectModelIndex++, routeEffect++) {
                effectModel = &Hu3DData[routeEffect->modelId];
                if ((effectModel->attr & 1) == 0) {
                    if (((teamState->worldPosition.z - 7000.0f) > effectModel->pos.z) || ((1000.0f + teamState->worldPosition.z) < effectModel->pos.z)) {
                        if (routeEffect->animationPhase == 0) {
                            InitializeRouteEffect(1, teamState, routeEffect);
                            SetMapModelAnimation(routeEffect->spriteId, 1);
                            effectModel->attr |= 1;
                            routeEffect->animationPhase = 0x63;
                            teamState->activeRouteEffectCounts[routeEffect->routeEventIndex] = 0;
                        }
                        else if (routeEffect->animationPhase != 2) {
                            routeEffect->phaseFrame = 0;
                            routeEffect->animationPhase = 2;
                        }
                    }
                    switch (routeEffect->animationPhase) {
                        case 0:
                            if (++routeEffect->phaseFrame > routeEffect->phaseOneFrames) {
                                routeEffect->phaseFrame = 0;
                                routeEffect->opacity = 0.0f;
                                routeEffect->scalePulseAngle = ((rand8() << 8) | rand8()) % 361;
                                routeEffect->lightPulseAngle = ((rand8() << 8) | rand8()) % 361;
                                routeEffect->effectScale = 1.0f;
                                routeEffect->particleAngle = 0.0f;
                                routeEffect->animationPhase++;
                            }
                            break;
                        case 1:
                            if (++routeEffect->phaseFrame > routeEffect->phaseFourFrames) {
                                routeEffect->phaseFrame = 0;
                                routeEffect->animationPhase++;
                                if (getGameplayState() <= 4) {
                                    if (routeEventIndex == 0) {
                                        soundEffectId = HuAudFXPlay(0x65A);
                                    }
                                    else {
                                        soundEffectId = HuAudFXPlay(0x65B);
                                    }
                                    soundSlotIndex = routeEventIndex * 4 + teamIndex * 2 + effectModelIndex;
                                    soundEffectLifetimes[soundSlotIndex].soundEffectId = soundEffectId;
                                    soundEffectLifetimes[soundSlotIndex].remainingFrames = REFRESH_RATE * 2;
                                }
                            }
                            routeEffect->spriteOpacity = 0.1f;
                            UpdateRouteEffectState(teamState, routeEffect, teamIndex, 0.2f, routeEffect->spriteOpacity);
                            break;
                        case 2:
                            if (++routeEffect->phaseFrame > routeEffect->phaseTwoFrames) {
                                routeEffect->phaseFrame = 0;
                                routeEffect->animationPhase++;
                            }
                            if (routeEffect->spriteOpacity < 1.0f) {
                                routeEffect->spriteOpacity += 0.05f;
                            }
                            if (routeEffect->spriteOpacity > 1.0f) {
                                routeEffect->spriteOpacity = 1.0f;
                            }
                            routeEffect->lightRadius += 40.0f;
                            routeEffect->opacity += 0.05f;
                            UpdateRouteEffectState(teamState, routeEffect, teamIndex, 1.0f, routeEffect->spriteOpacity);
                            break;
                        case 3:
                            routeEffect->phaseFrame++;
                            lightPulseOffset = 1.0f - ((float)routeEffect->phaseFrame / routeEffect->phaseThreeFrames);
                            if (routeEffect->phaseFrame > routeEffect->phaseThreeFrames) {
                                routeEffect->phaseFrame = 0;
                                lightPulseOffset = 0.0f;
                                routeEffect->animationPhase = 0;
                            }
                            routeEffect->lightRadius = 400.0 * sind((90.0f * lightPulseOffset));
                            routeEffect->opacity = 1.2f * lightPulseOffset;
                            routeEffect->spriteOpacity = lightPulseOffset;
                            UpdateRouteEffectState(teamState, routeEffect, teamIndex, lightPulseOffset, routeEffect->spriteOpacity);
                            break;
                    }
                    effectPosition.x = effectModel->pos.x;
                    effectPosition.z = effectModel->pos.z;
                    TestRouteCircleCollision(effectPosition, 50.0f, 0, 1, teamState->teamIndex);
                }
            }
        }
    }
}

void SetTeamRouteAnimation(s16 teamIndex, TeamMapState *teamState)
{
    float routeEffectFacingAngle;
    RouteEffectState *routeEffect;
    RouteEvent *routeEvent;
    s16 effectModelIndex;
    HU3DMODEL *effectModel;

    routeEvent = &mapState.routeEvents[teamIndex];
    routeEffect = teamState->routeEffects[routeEvent->routeType];
    for (effectModelIndex = 0; effectModelIndex < 2; effectModelIndex++, routeEffect++) {
        effectModel = &Hu3DData[routeEffect->modelId];
        if ((effectModel->attr & 1) != 0) {
            Hu3DModelAttrReset(routeEffect->modelId, HU3D_ATTR_DISPOFF);
            Hu3DModelPosSet(routeEffect->modelId, routeEvent->worldPosition.x, routeEvent->worldPosition.y, routeEvent->worldPosition.z);
            routeEffect->animationPhase = 0;
            routeEffect->phaseFrame = 0;
            routeEffect->phaseOneFrames = routeEvent->stateDurations[0];
            routeEffect->phaseTwoFrames = routeEvent->stateDurations[1];
            routeEffect->phaseThreeFrames = routeEvent->stateDurations[2];
            routeEffect->phaseFourFrames = routeEvent->stateDurations[3];
            routeEffect->routeEventIndex = teamIndex;
            teamState->activeRouteEffectCounts[teamIndex] = 1;
            SetMapModelPosition(routeEffect->spriteId, routeEvent->worldPosition.x, routeEvent->worldPosition.y, routeEvent->worldPosition.z);
            if (routeEvent->routeType == 0) {
                routeEffectFacingAngle = 90.0f;
            }
            else {
                routeEffectFacingAngle = 270.0f;
            }
            SetMapModelScale(routeEffect->spriteId, 0.0f, routeEffectFacingAngle, 0.0f);
            SetMapModelAnimationSpeed(routeEffect->spriteId, 1);
            InitializeRouteEffect(0, teamState, routeEffect);
            return;
        }
    }
}

float teamSideDirections[2] = { -1.0f, 1.0f };
float teamFacingAngles[2] = { 270.0f, 90.0f };

void UpdateRouteEffectState(TeamMapState *teamState, RouteEffectState *effectState, s16 teamIndex, float effectIntensity, float spriteOpacity)
{
    Vec particleSpawnCenter;
    Vec zeroLightDirection;
    Vec effectScale;
    Vec leftEffectBoundary;
    Vec rightEffectBoundary;
    float particleStrength;
    float lightRadius;
    HU3DMODEL *effectModel;
    MapQuadRenderData *spriteRecord;

    effectModel = &Hu3DData[effectState->modelId];
    particleStrength = effectIntensity * effectState->effectScale;
    particleSpawnCenter.x = effectModel->pos.x + (60.0f * teamSideDirections[teamIndex]);
    particleSpawnCenter.y = 80.0f + effectModel->pos.y;
    particleSpawnCenter.z = effectModel->pos.z;
    if (effectState->animationPhase == 1) {
        CreateMapParticleSystem(0, particleSpawnCenter.x, particleSpawnCenter.y, particleSpawnCenter.z, 1, teamFacingAngles[teamIndex], particleStrength, teamPrimaryAnimationIds[teamState->teamIndex]);
        return;
    }
    CreateMapParticleSystem(1, particleSpawnCenter.x, particleSpawnCenter.y, particleSpawnCenter.z, 1, teamFacingAngles[teamIndex], particleStrength, teamPrimaryAnimationIds[teamState->teamIndex]);
    if (effectState->lightRadius > (400.0f * effectState->effectScale)) {
        effectState->lightRadius = 400.0f * effectState->effectScale;
    }
    else if (effectState->lightRadius < 1.0f) {
        effectState->lightRadius = 1.0f;
    }
    effectState->particleAngle += 30.0f;
    if (effectState->particleAngle >= 360.0f) {
        effectState->particleAngle -= 360.0f;
    }
    particleSpawnCenter.x = effectModel->pos.x + (240.0f * teamSideDirections[teamIndex]);
    particleSpawnCenter.y = 100.0f + effectModel->pos.y;
    particleSpawnCenter.z = effectModel->pos.z;
    zeroLightDirection.x = zeroLightDirection.y = zeroLightDirection.z = 0.0f;
    lightRadius = effectState->lightRadius + (effectIntensity * (50.0 * sind(effectState->particleAngle)));
    Hu3DLLightPointSet(effectState->modelId, effectState->lightIndex, lightRadius, 0.5f, 3);
    Hu3DLLightPosSetV(effectState->modelId, effectState->lightIndex, &particleSpawnCenter, &zeroLightDirection);
    spriteRecord = &mapQuadRenderData[effectState->spriteId];
    spriteRecord->secondaryTextureOffsetX -= 0.0005f;
    spriteRecord->textureOffsetY += 0.005f;
    spriteRecord->secondaryTextureOffsetY += 0.001f;
    if (effectState->opacity > 1.0f) {
        effectState->opacity = 1.0f;
    }
    else if (effectState->opacity < 0.0f) {
        effectState->opacity = 0.0f;
    }
    SetMapModelColor(effectState->spriteId, 0xFF, 0xA0, 0x60, 255.0f * effectState->opacity);
    effectState->scalePulseAngle += 2.0f;
    effectState->lightPulseAngle += (((rand8() << 8) | rand8()) % 5) + 4;
    if (effectState->scalePulseAngle >= 360.0f) {
        effectState->scalePulseAngle -= 360.0f;
    }
    if (effectState->lightPulseAngle >= 360.0f) {
        effectState->lightPulseAngle -= 360.0f;
    }
    effectScale.x = 1.0 + (0.009999999776482582 * sind(effectState->lightPulseAngle));
    effectScale.z = effectState->effectScale + (0.009999999776482582 * sind(effectState->scalePulseAngle));
    SetMapModelRotation(effectState->spriteId, effectScale.x, 1.0f, effectScale.z);
    if (teamIndex == 0) {
        leftEffectBoundary.x = effectModel->pos.x - (700.0f * particleStrength * spriteOpacity);
        rightEffectBoundary.x = effectModel->pos.x - 50.0f;
    }
    else {
        leftEffectBoundary.x = 50.0f + effectModel->pos.x;
        rightEffectBoundary.x = effectModel->pos.x + (700.0f * particleStrength * spriteOpacity);
    }
    leftEffectBoundary.z = effectModel->pos.z - 30.0f;
    rightEffectBoundary.z = 30.0f + effectModel->pos.z;
    TestRouteSegmentCollision(leftEffectBoundary, rightEffectBoundary, 1, 1, teamState->teamIndex);
}

void InitializeRouteEffect(u8 effectIndex, TeamMapState *teamState, RouteEffectState *effectState)
{
    s32 teamModelIds[8];
    s16 modelIndex;
    HU3DMODEL *model;
    RouteEvent *routeEvent;
    s16 lightId;
    HU3DMODEL *effectModel;
    s16 teamModelCount;

    routeEvent = &mapState.routeEvents[effectState->routeEventIndex];
    effectModel = &Hu3DData[effectState->modelId];
    if (effectIndex == 0) {
        lightId = effectModel->lLightId[effectState->lightIndex];
    }
    else {
        lightId = -1;
    }
    teamModelCount = findCharacterModelIndex(teamState->teamIndex, teamModelIds);
    for (modelIndex = 0; modelIndex < teamModelCount; modelIndex++) {
        model = &Hu3DData[teamModelIds[modelIndex]];
        model->lLightId[effectState->lightIndex] = lightId;
    }
    model = &Hu3DData[teamState->lightModelId];
    model->lLightId[effectState->lightIndex] = lightId;
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        if (routeEvent->linkedPointIndices[modelIndex] != -1) {
            model = &Hu3DData[teamState->modelIds[routeEvent->linkedPointIndices[modelIndex]]];
            model->lLightId[effectState->lightIndex] = lightId;
        }
    }
}

void UpdateRouteEffectController(OMOBJ *object)
{
    Vec lightPosition;
    Vec zeroLightDirection;
    float headingSine;
    float headingCosine;
    TeamMapState *teamState;
    s16 teamIndex;
    MapState *mapState;

    mapState = &mapState;
    for (teamIndex = 0, teamState = mapState->teamStates; teamIndex < 2; teamIndex++, teamState++) {
        headingSine = sind(teamState->facingAngle);
        headingCosine = cosd(teamState->facingAngle);
        teamState->spotlightPosition.x = teamState->worldPosition.x - (25.0f * headingSine);
        teamState->spotlightPosition.y = 100.0f + teamState->worldPosition.y;
        teamState->spotlightPosition.z = teamState->worldPosition.z - (25.0f * headingCosine);
        teamState->spotlightTarget.x = teamState->spotlightPosition.x - (50.0f * headingSine);
        teamState->spotlightTarget.y = teamState->spotlightPosition.y;
        teamState->spotlightTarget.z = teamState->spotlightPosition.z - (50.0f * headingCosine);
        Hu3DLLightPosSet(teamState->modelIds[0], teamState->highlightLightId, teamState->spotlightPosition.x, teamState->spotlightPosition.y, teamState->spotlightPosition.z, 0.0f, 0.0f, 0.0f);
        Hu3DLLightPosAimSetV(teamState->modelIds[0], teamState->highlightLightId, &teamState->spotlightPosition, &teamState->spotlightTarget);
        SetMapModelPosition(teamState->bannerModelId[0], teamState->worldPosition.x, teamState->worldPosition.y, teamState->worldPosition.z);
        SetMapModelScale(teamState->bannerModelId[0], 0.0f, teamState->facingAngle, 0.0f);
        lightPosition.x = teamState->worldPosition.x;
        lightPosition.y = 100.0f + teamState->worldPosition.y;
        lightPosition.z = 50.0f + teamState->worldPosition.z;
        zeroLightDirection.x = zeroLightDirection.y = zeroLightDirection.z = 0.0f;
        Hu3DLLightPosSetV(teamState->lightModelId, 1, &lightPosition, &zeroLightDirection);
    }
}

void SetBannerAnimationOffset(s16 teamIndex, float offsetX, float offsetY, float scaleX, float scaleY)
{
    TeamMapState *teamState;

    teamState = &mapState.teamStates[teamIndex];
    teamState->worldPosition.x = offsetX;
    teamState->worldPosition.y = offsetY;
    teamState->worldPosition.z = scaleX;
    teamState->facingAngle = scaleY;
    if (teamState->particleHookData) {
        teamState->particleHookData->updateOffset = 1500.0f + scaleX;
    }
}

void UpdateBannerAnimation(void)
{
    MapQuadRenderData *spriteRecord;
    TeamMapState *teamState;
    s16 bannerIndex;
    s16 teamIndex;

    for (teamState = mapState.teamStates, teamIndex = 0; teamIndex < 2; teamIndex++, teamState++) {
        for (bannerIndex = 0; bannerIndex < 1; bannerIndex++) {
            spriteRecord = &mapQuadRenderData[teamState->bannerModelId[bannerIndex]];
            spriteRecord->secondaryTextureOffsetX -= 0.0005f;
            spriteRecord->textureOffsetY += 0.0005f;
            spriteRecord->secondaryTextureOffsetY += 0.0005f;
        }
    }
}

void SetBannerAnimationFrame(s16 *animationIds, s16 animationFrame)
{
    s16 lightSlotIndex;
    HU3DMODEL *sourceModel;
    HU3DMODEL *linkedModel;
    s16 modelIndex;

    sourceModel = &Hu3DData[*animationIds];
    for (modelIndex = 1; modelIndex < animationFrame; modelIndex++) {
        linkedModel = &Hu3DData[animationIds[modelIndex]];
        for (lightSlotIndex = 0; lightSlotIndex < 8; lightSlotIndex++) {
            if (sourceModel->lLightId[lightSlotIndex] != -1) {
                linkedModel->lLightId[lightSlotIndex] = sourceModel->lLightId[lightSlotIndex];
            }
        }
        linkedModel->attr |= 0x1000;
    }
}

void SetBannerTextureScale(s32 primaryAnimationAsset, s32 secondaryAnimationAsset, float sourceTextureWidth, float sourceTextureHeight, float targetTextureWidth, float targetTextureHeight)
{
    BannerAnimationSet *bannerAnimations;

    bannerAnimations = &mapState.bannerAnimationResources;
    bannerAnimations->primaryAnimation = HuSprAnimRead(HuDataReadNum(primaryAnimationAsset, HU_MEMNUM_OVL));
    bannerAnimations->secondaryAnimation = HuSprAnimRead(HuDataReadNum(secondaryAnimationAsset, HU_MEMNUM_OVL));
    bannerAnimations->offsetX = 0.5f * sourceTextureWidth;
    bannerAnimations->offsetY = 0.5f * sourceTextureHeight;
    bannerAnimations->scaleX = targetTextureWidth / sourceTextureWidth;
    bannerAnimations->scaleY = targetTextureHeight / sourceTextureHeight;
}

void CalculateMapProjection(Vec *sourceTopLeft, Vec *sourceBottomLeft, Vec *sourceTopRight, Vec *sourceBottomRight, float screenScale, Vec2f *projectedBounds, s16 teamIndex)
{
    Mtx scaleMatrix;
    Mtx rotationMatrix;
    Vec rotatedCorner;
    Vec uvOffset;
    BannerAnimationSet *bannerAnimations;
    s16 vertexIndex;

    bannerAnimations = &mapState.bannerAnimationResources;
    for (vertexIndex = 0; vertexIndex < teamIndex; vertexIndex++, sourceTopRight++, projectedBounds++) {
        MTXScale(scaleMatrix, sourceBottomRight->x, sourceBottomRight->y, sourceBottomRight->z);
        MTXRotRad(rotationMatrix, 0x59, MTXDegToRad(screenScale));
        MTXConcat(scaleMatrix, rotationMatrix, scaleMatrix);
        MTXMultVec(scaleMatrix, sourceTopRight, &rotatedCorner);
        uvOffset.x = bannerAnimations->offsetX + ((rotatedCorner.x + sourceBottomLeft->x) - sourceTopLeft->x);
        uvOffset.z = bannerAnimations->offsetY + ((rotatedCorner.z + sourceBottomLeft->z) - sourceTopLeft->z);
        projectedBounds->x = uvOffset.x * bannerAnimations->scaleX;
        projectedBounds->y = uvOffset.z * bannerAnimations->scaleY;
    }
}

s16 CreateMapBanner(s32 assetId, s16 teamIndex, s16 animationId, float positionX, float positionY, float positionZ, float scale, float rotation)
{
    MapQuadRenderData *quadRecord;
    s16 vertexIndex;
    s16 quadSlotIndex;
    void *displayListAllocation;
    void *displayListWriteCursor;

    for (quadRecord = mapQuadRenderData, quadSlotIndex = 0; quadSlotIndex < 0x10; quadSlotIndex++, quadRecord++) {
        if (quadRecord->cameraIndex == -1) {
            break;
        }
    }
    if (quadSlotIndex == 0x10) {
        return -1;
    }
    quadRecord->modelId = Hu3DHookFuncCreate(RenderMapEffectModel);
    Hu3DModelCameraSet(quadRecord->modelId, teamIndex ? 2 : 1);
    Hu3DModelLayerSet(quadRecord->modelId, animationId);
    Hu3DData[quadRecord->modelId].hookData = quadRecord;
    Hu3DModelPosSet(quadRecord->modelId, 0.0f, 0.0f, -2000.0f);
    quadRecord->drawFlags = 0;
    quadRecord->cameraIndex = teamIndex;
    quadRecord->tintColor.r = quadRecord->tintColor.g = quadRecord->tintColor.b = quadRecord->tintColor.a = 0xFF;
    quadRecord->textureOffsetX = quadRecord->textureOffsetY = 0.0f;
    quadRecord->secondaryTextureOffsetX = quadRecord->secondaryTextureOffsetY = 0.0f;
    quadRecord->animationData = HuSprAnimRead(HuDataReadNum(assetId, HU_MEMNUM_OVL));
    SetVectorComponents(&quadRecord->topLeftPosition, positionX - positionZ, rotation, positionY - scale);
    SetVector2Components(&quadRecord->topLeftUv, 0.0f, 0.0f);
    SetVectorComponents(&quadRecord->topRightPosition, positionX + positionZ, rotation, positionY - scale);
    SetVector2Components(&quadRecord->topRightUv, 1.0f, 0.0f);
    SetVectorComponents(&quadRecord->bottomRightPosition, positionX + positionZ, rotation, positionY + scale);
    SetVector2Components(&quadRecord->bottomRightUv, 1.0f, 1.0f);
    SetVectorComponents(&quadRecord->bottomLeftPosition, positionX - positionZ, rotation, positionY + scale);
    SetVector2Components(&quadRecord->bottomLeftUv, 0.0f, 1.0f);
    DCFlushRangeNoSync(&quadRecord->topLeftPosition, 0x30);
    DCFlushRangeNoSync(&quadRecord->topLeftUv, 0x20);
    PPCSync();
    displayListAllocation = HuMemDirectMallocNum(HEAP_HEAP, 0x1000, HU_MEMNUM_OVL);
    displayListWriteCursor = displayListAllocation;
    DCFlushRange(displayListAllocation, 0x1000);
    GXBeginDisplayList(displayListWriteCursor, 0x1000);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    for (vertexIndex = 0; vertexIndex < 4; vertexIndex++) {
        GXPosition1x16(vertexIndex);
        GXPosition1x16(vertexIndex);
        GXPosition1x16(vertexIndex);
    }
    quadRecord->displayListSize = GXEndDisplayList();
    quadRecord->displayList = HuMemDirectMallocNum(HEAP_HEAP, quadRecord->displayListSize, HU_MEMNUM_OVL);
    memcpy(quadRecord->displayList, displayListAllocation, quadRecord->displayListSize);
    DCFlushRange(quadRecord->displayList, quadRecord->displayListSize);
    HuMemDirectFree(displayListAllocation);
    return quadSlotIndex;
}

void SetMapModelAnimation(s16 modelId, u8 animationIndex)
{
    MapQuadRenderData *quadRecord;

    if (modelId != -1) {
        quadRecord = &mapQuadRenderData[modelId];
        quadRecord->drawFlags |= animationIndex;
    }
}

void SetMapModelAnimationSpeed(s16 modelId, u8 animationSpeed)
{
    MapQuadRenderData *quadRecord;

    if (modelId != -1) {
        quadRecord = &mapQuadRenderData[modelId];
        quadRecord->drawFlags &= ~animationSpeed;
    }
}

void SetMapModelColor(s16 modelId, u8 red, u8 green, u8 blue, u8 alpha)
{
    MapQuadRenderData *quadRecord;

    if (modelId != -1) {
        quadRecord = &mapQuadRenderData[modelId];
        quadRecord->tintColor.r = red;
        quadRecord->tintColor.g = green;
        quadRecord->tintColor.b = blue;
        quadRecord->tintColor.a = alpha;
    }
}

void SetMapModelPosition(s16 modelId, float positionX, float positionY, float positionZ)
{
    MapQuadRenderData *quadRecord;

    if (modelId != -1) {
        quadRecord = &mapQuadRenderData[modelId];
        Hu3DModelPosSet(quadRecord->modelId, positionX, positionY, positionZ);
    }
}

void SetMapModelRotation(s16 modelId, float rotationX, float rotationY, float rotationZ)
{
    MapQuadRenderData *quadRecord;

    if (modelId != -1) {
        quadRecord = &mapQuadRenderData[modelId];
        Hu3DModelScaleSet(quadRecord->modelId, rotationX, rotationY, rotationZ);
    }
}

void SetMapModelScale(s16 modelId, float scaleX, float scaleY, float scaleZ)
{
    MapQuadRenderData *quadRecord;

    if (modelId != -1) {
        quadRecord = &mapQuadRenderData[modelId];
        Hu3DModelRotSet(quadRecord->modelId, scaleX, scaleY, scaleZ);
    }
}

void RenderMapEffectModel(HU3DMODEL *model, Mtx matrix)
{
    Mtx textureMatrix;
    MapQuadRenderData *quadRecord;
    TeamMapState *teamState;
    BannerAnimationSet *bannerAnimations;

    quadRecord = model->hookData;
    teamState = &mapState.teamStates[quadRecord->cameraIndex];
    bannerAnimations = &mapState.bannerAnimationResources;
    if ((quadRecord->drawFlags & 1) == 0) {
        CalculateMapProjection(&teamState->cameraFocusPosition, &model->pos, &quadRecord->topLeftPosition, &model->scale, model->rot.y, quadRecord->transformedUv, 4);
        DCFlushRangeNoSync(quadRecord->transformedUv, 4 * sizeof(Vec2f));
        PPCSync();
        GXLoadPosMtxImm(matrix, 0);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0);
        GXSetArray(GX_VA_POS, &quadRecord->topLeftPosition, sizeof(Vec));
        GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_RGBA6, 0);
        GXSetArray(GX_VA_TEX0, &quadRecord->topLeftUv, sizeof(Vec2f));
        GXSetVtxDesc(GX_VA_TEX1, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_RGBA6, 0);
        GXSetArray(GX_VA_TEX1, quadRecord->transformedUv, sizeof(Vec2f));
        HuSprTexLoad(quadRecord->animationData, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
        HuSprTexLoad(bannerAnimations->secondaryAnimation, 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
        HuSprTexLoad(bannerAnimations->primaryAnimation, 0, 2, GX_REPEAT, GX_REPEAT, GX_LINEAR);
        GXSetNumTexGens(3);
        MTXScale(textureMatrix, 1.0f, 1.0f, 1.0f);
        mtxTransCat(textureMatrix, quadRecord->textureOffsetX, quadRecord->secondaryTextureOffsetX, 0.0f);
        GXLoadTexMtxImm(textureMatrix, 0x1E, GX_MTX2x4);
        MTXScale(textureMatrix, 1.0f, 1.0f, 1.0f);
        mtxTransCat(textureMatrix, quadRecord->textureOffsetY, quadRecord->secondaryTextureOffsetY, 0.0f);
        GXLoadTexMtxImm(textureMatrix, 0x21, GX_MTX2x4);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, 0, 0x7D);
        GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX1, 0x1E, 0, 0x7D);
        GXSetTexCoordGen2(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX0, 0x21, 0, 0x7D);
        GXSetNumTevStages(2);
        GXSetTevColor(GX_TEVREG1, quadRecord->tintColor);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_C1, GX_CC_TEXA, GX_CC_ZERO);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG0);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG0);
        GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
        GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_C0, GX_CC_TEXA, GX_CC_ZERO);
        GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_A1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetNumIndStages(1);
        GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD2, GX_TEXMAP2);
        GXSetTevIndWarp(0, 0, 1, 0, 1);
        MTXScale(textureMatrix, 0.1f, 0.10000000149011612 * -sind(model->rot.y), 1.0f);
        GXSetIndTexMtx(GX_ITM_0, (float(*)[3])textureMatrix, 0);
        GXSetNumChans(0);
        GXSetZMode(0, GX_LEQUAL, 0);
        GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
        GXSetZCompLoc(0);
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
        GXCallDisplayList(quadRecord->displayList, quadRecord->displayListSize);
        GXSetNumIndStages(0);
        GXSetTevDirect(GX_TEVSTAGE0);
        GXSetTevDirect(GX_TEVSTAGE1);
    }
}

void RenderMapEffectOverlay(HU3DMODEL *model, Mtx matrix)
{
    TeamRenderTarget *renderTarget;

    renderTarget = model->hookData;
    if (renderTarget->cameraIndex == 0) {
        GXSetTexCopySrc(0, 0, 320, 480);
        GXSetTexCopyDst(320, 480, GX_TF_RGB5A3, 0);
    }
    else {
        GXSetTexCopySrc(320, 0, 320, 480);
        GXSetTexCopyDst(320, 480, GX_TF_RGB5A3, 0);
    }
    GXCopyTex(renderTarget->textureBuffer, 1);
    DCFlushRange(renderTarget->textureBuffer, renderTarget->textureBufferSize);
}

s16 CreateTeamMarker(s32 assetId, s16 animationId, s16 modelIndex, s16 teamIndex, float markerScale, Vec *markerPosition, Vec *markerRotation)
{
    float gridColumnStep;
    float gridRowStep;
    float textureUScale;
    float textureVScale;
    TerrainWaveRenderData *terrainHookState;
    GridQuad *gridQuad;
    HU3DMODEL *terrainHookModel;
    s16 hookIdOrGridRow;
    s16 gridColumn;
    Vec2f *terrainTextureCoordinates;
    void *displayListAllocation;
    MapState *mapState;
    u16 lightChannelIndex;
    s16 createdHookId;
    void *displayListWorkBuffer;

    mapState = &mapState;
    hookIdOrGridRow = Hu3DHookFuncCreate(RenderTerrainSurface);
    createdHookId = hookIdOrGridRow;
    Hu3DModelCameraSet(hookIdOrGridRow, teamIndex ? 2 : 1);
    Hu3DModelLayerSet(hookIdOrGridRow, 5);
    terrainHookModel = &Hu3DData[hookIdOrGridRow];
    terrainHookState = HuMemDirectMallocNum(HEAP_MODEL, sizeof(TerrainWaveRenderData), terrainHookModel->mallocNo);
    terrainHookModel->hookData = terrainHookState;
    mapState->teamStates[teamIndex].particleHookData = terrainHookState;
    terrainHookState->cameraIndex = teamIndex;
    terrainHookState->terrainOffset = 0.0f;
    terrainHookState->terrainDepth = markerScale;
    terrainHookState->sourcePositions = markerPosition;
    terrainHookState->targetPositions = markerRotation;
    terrainHookState->scrollSineOffset = terrainHookState->terrainWaveState = 0.0f;
    terrainHookState->textureScrollOffsetY = terrainHookState->textureScrollOffsetX = 0.0f;
    terrainHookState->wavePhaseX = terrainHookState->wavePhaseZ = 0.0f;
    terrainHookState->wavePhaseStepX = 120.0f;
    terrainHookState->wavePhaseStepZ = 72.0f;
    terrainHookState->waveAmplitudeX = 10.0f;
    terrainHookState->waveAmplitudeZ = 10.0f;
    terrainHookState->animationData = HuSprAnimRead(HuDataReadNum(assetId, HU_MEMNUM_OVL));
    terrainHookState->gridColumnCount = animationId + 1;
    terrainHookState->gridRowCount = modelIndex + 1;
    terrainHookState->vertexCount = terrainHookState->gridColumnCount * terrainHookState->gridRowCount;
    terrainHookState->terrainVertices = HuMemDirectMallocNum(HEAP_MODEL, terrainHookState->vertexCount * sizeof(Vec), terrainHookModel->mallocNo);
    terrainTextureCoordinates = HuMemDirectMallocNum(HEAP_MODEL, terrainHookState->vertexCount * sizeof(Vec2f), terrainHookModel->mallocNo);
    terrainHookState->terrainTextureCoordinates = terrainTextureCoordinates;
    terrainHookState->terrainNormals = HuMemDirectMallocNum(HEAP_MODEL, terrainHookState->vertexCount * sizeof(Vec), terrainHookModel->mallocNo);
    gridQuad = HuMemDirectMallocNum(HEAP_MODEL, modelIndex * (animationId * sizeof(GridQuad)), terrainHookModel->mallocNo);
    terrainHookState->gridQuads = gridQuad;
    displayListAllocation = HuMemDirectMallocNum(HEAP_MODEL, 0x20000, terrainHookModel->mallocNo);
    displayListWorkBuffer = displayListAllocation;
    DCFlushRange(displayListAllocation, 0x20000);
    GXBeginDisplayList(displayListWorkBuffer, 0x20000);
    GXBegin(GX_QUADS, GX_VTXFMT0, animationId * modelIndex * 4);
    for (hookIdOrGridRow = 0; hookIdOrGridRow < modelIndex; hookIdOrGridRow++) {
        for (gridColumn = 0; gridColumn < animationId; gridColumn++) {
            gridQuad->firstVertexIndex = gridColumn + (hookIdOrGridRow * terrainHookState->gridColumnCount);
            gridQuad->secondVertexIndex = gridColumn + hookIdOrGridRow * terrainHookState->gridColumnCount + 1;
            gridQuad->thirdVertexIndex = gridColumn + ((hookIdOrGridRow + 1) * terrainHookState->gridColumnCount) + 1;
            gridQuad->fourthVertexIndex = gridColumn + ((hookIdOrGridRow + 1) * terrainHookState->gridColumnCount);
            GXPosition1x16(gridQuad->firstVertexIndex);
            GXPosition1x16(gridQuad->firstVertexIndex);
            GXPosition1x16(gridQuad->firstVertexIndex);
            GXPosition1x16(gridQuad->secondVertexIndex);
            GXPosition1x16(gridQuad->secondVertexIndex);
            GXPosition1x16(gridQuad->secondVertexIndex);
            GXPosition1x16(gridQuad->thirdVertexIndex);
            GXPosition1x16(gridQuad->thirdVertexIndex);
            GXPosition1x16(gridQuad->thirdVertexIndex);
            GXPosition1x16(gridQuad->fourthVertexIndex);
            GXPosition1x16(gridQuad->fourthVertexIndex);
            GXPosition1x16(gridQuad->fourthVertexIndex);
            SetVectorFrame(&terrainHookState->terrainVertices[gridQuad->firstVertexIndex], &terrainHookState->terrainVertices[gridQuad->secondVertexIndex], &terrainHookState->terrainVertices[gridQuad->thirdVertexIndex], &gridQuad->vertexNormal);
            gridQuad++;
        }
    }
    terrainHookState->displayListSize = GXEndDisplayList();
    terrainHookState->displayList = HuMemDirectMallocNum(HEAP_MODEL, terrainHookState->displayListSize, terrainHookModel->mallocNo);
    memcpy(terrainHookState->displayList, displayListAllocation, terrainHookState->displayListSize);
    DCFlushRange(terrainHookState->displayList, terrainHookState->displayListSize);
    HuMemDirectFree(displayListAllocation);
    UpdateTeamMarker(terrainHookState);
    textureUScale = 3.0f / (terrainHookState->gridColumnCount - 1);
    textureVScale = 5.0f / (terrainHookState->gridRowCount - 1);
    gridRowStep = 0.0f;
    for (hookIdOrGridRow = 0; hookIdOrGridRow < terrainHookState->gridRowCount; hookIdOrGridRow++) {
        gridColumnStep = 0.0f;
        for (gridColumn = 0; gridColumn < terrainHookState->gridColumnCount; gridColumn++) {
            terrainTextureCoordinates->x = gridColumnStep;
            terrainTextureCoordinates->y = gridRowStep;
            gridColumnStep += textureUScale;
            terrainTextureCoordinates++;
        }
        gridRowStep += textureVScale;
    }
    DCFlushRangeNoSync(terrainHookState->terrainTextureCoordinates, terrainHookState->vertexCount * 8);
    PPCSync();
    return createdHookId;
}

void UpdateTeamMarker(TerrainWaveRenderData *terrain)
{
    Vec routeHeightSample;
    float gridColumnPositionAndStep[2];
    float gridRowDepthAndStep[2];
    float wavePhaseX;
    float wavePhaseZ;
    float normalLength;
    GridQuad *gridQuad;
    Vec *terrainNormal;
    s16 gridRow;
    Vec *terrainVertex;
    s16 gridColumn;
    s16 quadCount;

    if (omPauseChk() == 0) {
        terrain->wavePhaseX += 1.0f;
        terrain->wavePhaseZ += 1.5f;
    }
    if (terrain->wavePhaseX >= 360.0f) {
        terrain->wavePhaseX -= 360.0f;
    }
    if (terrain->wavePhaseZ >= 360.0f) {
        terrain->wavePhaseZ -= 360.0f;
    }
    terrainVertex = terrain->terrainVertices;
    terrainNormal = terrain->terrainNormals;
    gridRowDepthAndStep[0] = terrain->terrainOffset - terrain->terrainDepth;
    gridRowDepthAndStep[1] = terrain->terrainDepth / (terrain->gridRowCount - 1);
    wavePhaseZ = terrain->wavePhaseZ;
    for (gridRow = 0; gridRow < terrain->gridRowCount; gridRow++) {
        UpdateTerrainWave(terrain, gridRowDepthAndStep[0], &routeHeightSample);
        gridColumnPositionAndStep[0] = routeHeightSample.x;
        gridColumnPositionAndStep[1] = (routeHeightSample.y - routeHeightSample.x) / (terrain->gridColumnCount - 1);
        wavePhaseX = terrain->wavePhaseX;
        for (gridColumn = 0; gridColumn < terrain->gridColumnCount; gridColumn++) {
            terrainVertex->x = gridColumnPositionAndStep[0];
            terrainVertex->y = routeHeightSample.z + (terrain->waveAmplitudeX * sind(wavePhaseX)) + terrain->waveAmplitudeZ * sind(wavePhaseZ);
            terrainVertex->z = gridRowDepthAndStep[0];
            gridColumnPositionAndStep[0] += gridColumnPositionAndStep[1];
            terrainVertex++;
            terrainNormal->x = terrainNormal->y = terrainNormal->z = 0.0f;
            terrainNormal++;
            wavePhaseX += terrain->wavePhaseStepX;
            if (wavePhaseX >= 360.0f) {
                wavePhaseX -= 360.0f;
            }
        }
        gridRowDepthAndStep[0] += gridRowDepthAndStep[1];
        wavePhaseZ += terrain->wavePhaseStepZ;
        if (wavePhaseZ >= 360.0f) {
            wavePhaseZ -= 360.0f;
        }
    }
    gridQuad = terrain->gridQuads;
    quadCount = (terrain->gridColumnCount - 1) * (terrain->gridRowCount - 1);
    for (gridRow = 0; gridRow < quadCount; gridRow++) {
        SetVectorFrame(&terrain->terrainVertices[gridQuad->firstVertexIndex], &terrain->terrainVertices[gridQuad->secondVertexIndex], &terrain->terrainVertices[gridQuad->thirdVertexIndex], &gridQuad->vertexNormal);
        terrain->terrainNormals[gridQuad->firstVertexIndex].x += gridQuad->vertexNormal.x;
        terrain->terrainNormals[gridQuad->firstVertexIndex].y += gridQuad->vertexNormal.y;
        terrain->terrainNormals[gridQuad->firstVertexIndex].z += gridQuad->vertexNormal.z;

        terrain->terrainNormals[gridQuad->secondVertexIndex].x += gridQuad->vertexNormal.x;
        terrain->terrainNormals[gridQuad->secondVertexIndex].y += gridQuad->vertexNormal.y;
        terrain->terrainNormals[gridQuad->secondVertexIndex].z += gridQuad->vertexNormal.z;

        terrain->terrainNormals[gridQuad->thirdVertexIndex].x += gridQuad->vertexNormal.x;
        terrain->terrainNormals[gridQuad->thirdVertexIndex].y += gridQuad->vertexNormal.y;
        terrain->terrainNormals[gridQuad->thirdVertexIndex].z += gridQuad->vertexNormal.z;

        terrain->terrainNormals[gridQuad->fourthVertexIndex].x += gridQuad->vertexNormal.x;
        terrain->terrainNormals[gridQuad->fourthVertexIndex].y += gridQuad->vertexNormal.y;
        terrain->terrainNormals[gridQuad->fourthVertexIndex].z += gridQuad->vertexNormal.z;
        gridQuad++;
    }
    terrainNormal = terrain->terrainNormals;
    for (gridRow = 0; gridRow < terrain->vertexCount; gridRow++) {
        normalLength = sqrtf((terrainNormal->z * terrainNormal->z) + ((terrainNormal->x * terrainNormal->x) + (terrainNormal->y * terrainNormal->y)));
        terrainNormal->x /= normalLength;
        terrainNormal->y /= normalLength;
        terrainNormal->z /= normalLength;
        terrainNormal++;
    }
    DCFlushRangeNoSync(terrain->terrainVertices, terrain->vertexCount * sizeof(Vec));
    DCFlushRangeNoSync(terrain->terrainNormals, terrain->vertexCount * sizeof(Vec));
    PPCSync();
}

s16 teamMarkerModelIds[2] = { 0, 0 };

float CalculateTeamMarkerOpacity(s16 teamIndex, float distance, float opacityScale)
{
    float heightInterpolation[2];
    float heightInterpolationRatio;
    float gridColumnWidth;
    Vec *currentTerrainVertex;
    s16 vertexIndex;
    TeamMapState *teamState;
    Vec *terrainVertices;
    s16 vertexCount;
    s16 gridColumnCount;

    teamState = &mapState.teamStates[teamIndex];
    if (mapState.mapReady != 0) {
        if (mapState.mapReady == 1) {
            teamMarkerModelIds[teamIndex] = 0;
            mapState.mapReady = 2;
        }
        if (!mapSurfaceRenderState.vertexPositions) {
            return 0.0f;
        }
        terrainVertices = mapSurfaceRenderState.vertexPositions;
        currentTerrainVertex = &mapSurfaceRenderState.vertexPositions[teamMarkerModelIds[teamIndex]];
        vertexCount = mapSurfaceRenderState.vertexCount;
        gridColumnCount = mapSurfaceRenderState.gridWidth;
        for (vertexIndex = teamMarkerModelIds[teamIndex]; vertexIndex < vertexCount; vertexIndex++, currentTerrainVertex++) {
            if (opacityScale >= currentTerrainVertex->z && distance >= currentTerrainVertex->x) {
                break;
            }
        }
        if (vertexIndex == vertexCount) {
            return 0.0f;
        }
    }
    else {
        if (!teamState->particleHookData) {
            return 0.0f;
        }
        terrainVertices = teamState->particleHookData->terrainVertices;
        currentTerrainVertex = &teamState->particleHookData->terrainVertices[teamMarkerModelIds[teamIndex]];
        vertexCount = teamState->particleHookData->vertexCount;
        gridColumnCount = teamState->particleHookData->gridColumnCount;
        for (vertexIndex = teamMarkerModelIds[teamIndex]; vertexIndex < vertexCount; vertexIndex++, currentTerrainVertex++) {
            if (opacityScale <= currentTerrainVertex->z && distance <= currentTerrainVertex->x) {
                break;
            }
        }
        if (vertexIndex == vertexCount) {
            return 0.0f;
        }
    }
    heightInterpolation[0] = currentTerrainVertex->x - terrainVertices[vertexIndex - 1].x;
    heightInterpolation[1] = distance - terrainVertices[vertexIndex - 1].x;
    gridColumnWidth = heightInterpolation[1] / heightInterpolation[0];
    heightInterpolationRatio = currentTerrainVertex->y - terrainVertices[vertexIndex - 1].y;
    heightInterpolationRatio = terrainVertices[vertexIndex - 1].y + (heightInterpolationRatio * gridColumnWidth);
    teamMarkerModelIds[teamIndex] = vertexIndex - (gridColumnCount * 2);
    return 20.0f + heightInterpolationRatio;
}

void UpdateTerrainWave(TerrainWaveRenderData *terrain, float waveAmplitude, Vec *waveDirection)
{
    float interpolatedTargetX[2];
    float interpolatedSourceX[2];
    float pathInterpolationRatio;
    s16 pathPointIndex;

    for (pathPointIndex = routeSegmentCount; pathPointIndex != 0;) {
        pathPointIndex--;
        if (terrain->sourcePositions[pathPointIndex].z >= waveAmplitude) {
            if (pathPointIndex == (routeSegmentCount - 1)) {
                waveDirection->x = terrain->targetPositions[pathPointIndex].x;
                waveDirection->y = terrain->sourcePositions[pathPointIndex].x;
                waveDirection->z = terrain->sourcePositions[pathPointIndex].y;
            }
            else {
                pathInterpolationRatio = (terrain->sourcePositions[pathPointIndex].z - waveAmplitude) / (terrain->sourcePositions[pathPointIndex].z - terrain->sourcePositions[pathPointIndex + 1].z);
                interpolatedTargetX[0] = terrain->targetPositions[pathPointIndex].x - 100.0f;
                interpolatedTargetX[1] = terrain->targetPositions[pathPointIndex + 1].x - 100.0f;
                interpolatedSourceX[0] = 100.0f + terrain->sourcePositions[pathPointIndex].x;
                interpolatedSourceX[1] = 100.0f + terrain->sourcePositions[pathPointIndex + 1].x;
                waveDirection->x = interpolatedTargetX[0] + (pathInterpolationRatio * (interpolatedTargetX[1] - interpolatedTargetX[0]));
                waveDirection->y = interpolatedSourceX[0] + (pathInterpolationRatio * (interpolatedSourceX[1] - interpolatedSourceX[0]));
                waveDirection->z = terrain->sourcePositions[pathPointIndex].y + (pathInterpolationRatio * (terrain->sourcePositions[pathPointIndex + 1].y - terrain->sourcePositions[pathPointIndex].y));
            }
            return;
        }
    }
}

void RenderTerrainSurface(HU3DMODEL *model, Mtx matrix)
{
    Mtx lightProjectionMatrix;
    Mtx modelMatrix;
    Mtx textureMatrix;
    Mtx cameraProjectionMatrix;
    GXTexObj renderTargetTexture;
    Vec2f bannerUv;
    GXColor particleLightColor;
    float bannerRotation;
    TerrainWaveRenderData *terrainHookState;
    TeamMapState *teamState;
    TeamRenderTarget *renderTarget;
    BannerAnimationSet *bannerAnimations;
    s16 lightChannelIndex;

    terrainHookState = model->hookData;
    teamState = &mapState.teamStates[terrainHookState->cameraIndex];
    renderTarget = &teamState->renderTarget;
    bannerAnimations = &mapState.bannerAnimationResources;
    UpdateTeamMarker(terrainHookState);
    GXLoadPosMtxImm(matrix, 0);
    MTXInvXpose(matrix, textureMatrix);
    GXLoadNrmMtxImm(textureMatrix, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, terrainHookState->terrainVertices, sizeof(Vec));
    GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_NRM, terrainHookState->terrainNormals, sizeof(Vec));
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_TEX0, terrainHookState->terrainTextureCoordinates, 8);
    if (terrainHookState->cameraIndex == 0) {
        GXInitTexObj(&renderTargetTexture, renderTarget->textureBuffer, 0x140, 0x1E0, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, 0);
    }
    else {
        GXInitTexObj(&renderTargetTexture, renderTarget->textureBuffer, 0x140, 0x1E0, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, 0);
    }
    GXInitTexObjLOD(&renderTargetTexture, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
    GXLoadTexObj(&renderTargetTexture, GX_TEXMAP0);
    HuSprTexLoad(terrainHookState->animationData, 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    HuSprTexLoad(bannerAnimations->primaryAnimation, 0, 2, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetNumTevStages(3);
    particleLightColor.r = 0;
    particleLightColor.g = 0xB6;
    particleLightColor.b = 0xCC;
    particleLightColor.a = 0x40;
    GXSetTevColor(GX_TEVREG2, particleLightColor);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_4, GX_TRUE, GX_TEVREG0);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG0);
    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXA, GX_CC_C2, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG1);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG1);
    GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_C1, GX_CC_RASC, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumIndStages(1);
    GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD2, GX_TEXMAP2);
    GXSetTevIndWarp(1, 0, 1, 0, 1);
    MTXScale(textureMatrix, -0.1f, 0.1f, 1.0f);
    GXSetIndTexMtx(GX_ITM_0, (float(*)[3])textureMatrix, 0);
    GXSetNumTexGens(3);
    C_MTXLightPerspective(lightProjectionMatrix, 45.0f, 0.6f, 0.5f, -0.5f, 0.5f, 0.5f);
    MTXCopy(matrix, modelMatrix);
    MTXInverse(Hu3DCameraMtx, textureMatrix);
    MTXConcat(textureMatrix, modelMatrix, textureMatrix);
    MTXConcat(lightProjectionMatrix, Hu3DCameraMtx, cameraProjectionMatrix);
    MTXConcat(cameraProjectionMatrix, textureMatrix, textureMatrix);
    GXLoadTexMtxImm(textureMatrix, 0x1E, GX_MTX3x4);
    {
        Vec projectionScale = { 1.0f, 1.0f, 1.0f };
        bannerRotation = 0.0f;
        CalculateMapProjection(&teamState->cameraFocusPosition, &teamState->worldPosition, terrainHookState->terrainVertices, &projectionScale, bannerRotation, &bannerUv, 1);
    }
    if (omPauseChk() == 0) {
        terrainHookState->textureScrollOffsetX = terrainHookState->textureScrollOffsetX - 0.001f;
    }
    MTXScale(textureMatrix, 1.0f, 1.0f, 1.0f);
    mtxTransCat(textureMatrix, 0.0f, terrainHookState->textureScrollOffsetX + (0.3f * bannerUv.y), 0.0f);
    GXLoadTexMtxImm(textureMatrix, 0x21, GX_MTX2x4);
    if (omPauseChk() == 0) {
        terrainHookState->textureScrollPhase += 0.05f;
    }
    if (terrainHookState->textureScrollPhase >= 360.0f) {
        terrainHookState->textureScrollPhase -= 360.0f;
    }
    terrainHookState->scrollSineOffset = sind(terrainHookState->textureScrollPhase);
    if (omPauseChk() == 0) {
        terrainHookState->textureScrollOffsetY += 0.0005f;
    }
    MTXScale(textureMatrix, 0.5f, 0.5f, 0.5f);
    mtxTransCat(textureMatrix, terrainHookState->scrollSineOffset, terrainHookState->textureScrollOffsetY, 0.0f);
    GXLoadTexMtxImm(textureMatrix, 0x24, GX_MTX2x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, 0x1E, GX_FALSE, 0x7D);
    GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX0, 0x21, GX_FALSE, 0x7D);
    GXSetTexCoordGen2(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX0, 0x24, GX_FALSE, 0x7D);
    GXSetNumChans(1);
    lightChannelIndex = Hu3DLightSet(model, &Hu3DCameraMtx, &Hu3DCameraMtxXPose, 0.0f);
    GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, GX_SRC_REG, lightChannelIndex, GX_DF_CLAMP, GX_AF_SPOT);
    particleLightColor.r = particleLightColor.g = particleLightColor.b = 0x10;
    particleLightColor.a = 0xFF;
    GXSetChanAmbColor(GX_COLOR0A0, particleLightColor);
    particleLightColor.r = particleLightColor.g = particleLightColor.b = 0xFF;
    particleLightColor.a = 0xFF;
    GXSetChanMatColor(GX_COLOR0A0, particleLightColor);
    GXSetZMode(1, GX_LEQUAL, GX_TRUE);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZCompLoc(0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
    GXCallDisplayList(terrainHookState->displayList, terrainHookState->displayListSize);
    GXSetNumIndStages(0);
    GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevDirect(GX_TEVSTAGE1);
}

void RenderTerrainEffects(HU3DMODEL *model, Mtx matrix)
{
    GXColor fogColor = { 0, 0, 0, 0xFF };
    GXSetFog(GX_FOG_EXP, 10000.0f, 20000.0f, 300.0f, 50000.0f, fogColor);
    GXLoadPosMtxImm(matrix, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    HuSprTexLoad(mapEffectAnimationData, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, GX_FALSE, 0x7D);
    GXSetNumChans(0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZCompLoc(GX_FALSE);
    GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-1000.0f, 1000.0f, -24000.0f);
    GXTexCoord2f32(0.0f, 0.0f);

    GXPosition3f32(1000.0f, 1000.0f, -24000.0f);
    GXTexCoord2f32(1.0f, 0.0f);

    GXPosition3f32(1000.0f, -1000.0f, -24000.0f);
    GXTexCoord2f32(1.0f, 1.0f);

    GXPosition3f32(-1000.0f, -1000.0f, -24000.0f);
    GXTexCoord2f32(0.0f, 1.0f);

    GXSetFog(GX_FOG_EXP, 3500.0f, 20000.0f, 300.0f, 50000.0f, fogColor);
}

void RenderMapHighlight(HU3DMODEL *model, Mtx matrix)
{
    GXColor fogColor = { 0, 0, 0, 255 };
    GXSetFog(GX_FOG_EXP, 10000.0f, 20000.0f, 300.0f, 50000.0f, fogColor);
}

void RenderMapShadow(HU3DMODEL *model, Mtx matrix)
{
    GXColor fogColor = { 0, 0, 0, 255 };
    GXSetFog(GX_FOG_EXP, 3500.0f, 20000.0f, 300.0f, 50000.0f, fogColor);
}

void SetVectorComponents(Vec *vector, float x, float y, float z)
{
    vector->x = x;
    vector->y = y;
    vector->z = z;
}

void SetVector2Components(Vec2f *vector, float x, float y)
{
    vector->x = x;
    vector->y = y;
}

void SetVectorFrame(Vec *firstPoint, Vec *secondPoint, Vec *thirdPoint, Vec *normal)
{
    Vec firstEdgeVector;
    Vec secondEdgeVector;

    firstEdgeVector.x = secondPoint->x - firstPoint->x;
    firstEdgeVector.y = secondPoint->y - firstPoint->y;
    firstEdgeVector.z = secondPoint->z - firstPoint->z;
    secondEdgeVector.x = thirdPoint->x - secondPoint->x;
    secondEdgeVector.y = thirdPoint->y - secondPoint->y;
    secondEdgeVector.z = thirdPoint->z - secondPoint->z;
    normal->x = -((firstEdgeVector.y * secondEdgeVector.z) - (firstEdgeVector.z * secondEdgeVector.y));
    normal->y = -((firstEdgeVector.z * secondEdgeVector.x) - (firstEdgeVector.x * secondEdgeVector.z));
    normal->z = -((firstEdgeVector.x * secondEdgeVector.y) - (firstEdgeVector.y * secondEdgeVector.x));
}

// is this one of the structs?
void CopyVectorComponents(float *destination, float *source)
{
    source[0] = destination[0];
    source[4] = destination[1];
    source[8] = destination[2];
    source[1] = destination[4];
    source[5] = destination[5];
    source[9] = destination[6];
    source[2] = destination[8];
    source[6] = destination[9];
    source[10] = destination[10];
    source[3] = 0.0f;
    source[7] = 0.0f;
    source[11] = 0.0f;
}

s16 CreateMapLight(float x1, float y1, float z1, float x2, float y2, float z2, u8 r, u8 g, u8 b)
{
    Vec lightPosition;
    Vec lightDirection;
    GXColor lightColor;
    HU3DLIGHT *lightRecord;
    s16 lightIndex;

    lightPosition.x = x1;
    lightPosition.y = y1;
    lightPosition.z = z1;
    lightDirection.x = x2;
    lightDirection.y = y2;
    lightDirection.z = z2;
    lightColor.r = r;
    lightColor.g = g;
    lightColor.b = b;
    lightColor.a = 0xFF;
    for (lightRecord = Hu3DLocalLight, lightIndex = 0; lightIndex < 0x20; lightIndex++, lightRecord++) {
        if (lightRecord->type == -1) {
            break;
        }
    }
    if (lightIndex == 0x20) {
        return -1;
    }
    ConfigureMapLight(lightRecord, &lightPosition, &lightDirection, &lightColor);
    lightRecord->type |= 0x8000;
    return lightIndex;
}

void SetMapLight(s16 modelId, s16 lightIndex, float x1, float y1, float z1, float x2, float y2, float z2, u8 r, u8 g, u8 b)
{
    Vec lightPosition;
    Vec lightDirection;
    GXColor lightColor;
    HU3DMODEL *model;
    HU3DLIGHT *lightRecord;
    s16 lightSlotIndex;

    model = &Hu3DData[modelId];
    lightPosition.x = x1;
    lightPosition.y = y1;
    lightPosition.z = z1;
    lightDirection.x = x2;
    lightDirection.y = y2;
    lightDirection.z = z2;
    lightColor.r = r;
    lightColor.g = g;
    lightColor.b = b;
    lightColor.a = 0xFF;
    for (lightRecord = Hu3DLocalLight, lightSlotIndex = 0; lightSlotIndex < 0x20; lightSlotIndex++, lightRecord++) {
        if (lightRecord->type == -1) {
            break;
        }
    }
    if (lightSlotIndex == 0x20) {
        OSReport("Light Entry Over!\n");
        return;
    }
    ConfigureMapLight(lightRecord, &lightPosition, &lightDirection, &lightColor);
    if (model->lLightId[lightIndex] == -1) {
        model->lLightId[lightIndex] = lightSlotIndex;
        model->attr |= 0x1000;
    }
}

void ConfigureMapLight(HU3DLIGHT *light, Vec *lightPosition, Vec *lightTarget, GXColor *lightColor)
{
    light->type = 0;
    light->pos = *lightPosition;
    light->dir = *lightTarget;
    light->offset.x = light->offset.y = light->offset.z = 0.0f;
    light->cutoff = 30.0f;
    light->func = 2;
    VECNormalize(&light->dir, &light->dir);
    light->color.r = lightColor->r;
    light->color.g = lightColor->g;
    light->color.b = lightColor->b;
    light->color.a = lightColor->a;
}

void CreateMapParticleSystem(u8 particleType, float positionX, float positionY, float positionZ, s16 particleCount, float scale, float rotation, s16 textureTileCount)
{
    Vec particleSpawnPosition;
    Vec particleRotation;
    float spawnDirectionSinCos[2];
    GXColor particleColor;
    ParticleState *particle;
    s16 spawnIndex;

    if (particleType != 0) {
        scale += (((rand8() << 8) | rand8()) % 21) - 0xA;
    }
    else {
        scale += (((rand8() << 8) | rand8()) % 61) - 0x1E;
    }
    spawnDirectionSinCos[0] = sind(scale);
    spawnDirectionSinCos[1] = cosd(scale);
    for (spawnIndex = 0; spawnIndex < particleCount; spawnIndex++) {
        if (particleType != 0) {
            particleSpawnPosition.x = positionX + ((((rand8() << 8) | rand8()) % 21) - 0xA);
            particleSpawnPosition.y = positionY + ((((rand8() << 8) | rand8()) % 21) - 0xA);
            particleSpawnPosition.z = positionZ + ((((rand8() << 8) | rand8()) % 21) - 0xA);
        }
        else {
            particleSpawnPosition.x = positionX + ((((rand8() << 8) | rand8()) % 41) - 0x14);
            particleSpawnPosition.y = positionY + ((((rand8() << 8) | rand8()) % 41) - 0x14);
            particleSpawnPosition.z = positionZ + ((((rand8() << 8) | rand8()) % 41) - 0x14);
        }
        particleRotation.x = particleRotation.y = particleRotation.z = 0.0f;
        particleColor.r = particleColor.g = particleColor.b = particleColor.a = 0xFF;
        particle = CreateMapParticle(textureTileCount, &particleSpawnPosition, &particleRotation, 0.0f, &particleColor);
        if (!particle) {
            break;
        }
        particle->particleType = particleType;
        if (particleType != 0) {
            particle->animationFrame = 0;
            particle->animationFrameCount = 3;
            particle->velocityX = rotation * (spawnDirectionSinCos[0] * (0.1f * ((((rand8() << 8) | rand8()) % 151) + 0x96)));
            particle->velocityY = 0.1f * ((((rand8() << 8) | rand8()) % 31) - 0x14);
            particle->velocityZ = rotation * (spawnDirectionSinCos[1] * (0.1f * ((((rand8() << 8) | rand8()) % 151) + 0x96)));
            particle->decelerationX = 0.01f * particle->velocityX;
            particle->accelerationY = 0.1f;
            particle->decelerationZ = 0.01f * particle->velocityZ;
            particle->scale = rotation * (0.1f * ((((rand8() << 8) | rand8()) % 11) + 0xA));
            particle->scaleDelta = rotation * (0.01f * ((((rand8() << 8) | rand8()) % 101) + 0x32) / (particle->animationFrameCount * 0xE));
        }
        else {
            particle->animationFrame = 0;
            particle->animationFrameCount = 2;
            particle->velocityX = spawnDirectionSinCos[0] * (0.1f * ((((rand8() << 8) | rand8()) % 31) + 0x32));
            particle->velocityY = 0.1f * ((((rand8() << 8) | rand8()) % 31) - 0x14);
            particle->velocityZ = spawnDirectionSinCos[1] * (0.1f * (((((rand8() << 8) | rand8()) % 31)) + 0x32));
            particle->decelerationX = 0.0f;
            particle->accelerationY = 0.1f;
            particle->decelerationZ = 0.0f;
            particle->scale = 0.3f * (0.1f * ((((rand8() << 8) | rand8()) % 11) + 0xA));
            particle->scaleDelta = 0.4f * ((0.01f * ((((rand8() << 8) | rand8()) % 51) + 0x32)) / (particle->animationFrameCount * 0xE));
        }
        particle->opacity = 1024.0f * rotation;
        particle->opacityDelta = particle->opacity / (particle->animationFrameCount * 0xE);
    }
}

void RenderParticleSystem(HU3DMODEL *model, ParticleSystemRenderData *particleSystem, Mtx matrix)
{
    ParticleState *particle;
    GXColor *particleColor;
    s16 particleIndex;
    u8 particleAlpha;

    particle = particleSystem->particles;
    particleColor = particleSystem->colors;
    for (particleIndex = 0; particleIndex < particleSystem->particleCount; particleIndex++, particle++, particleColor++) {
        if (particle->active != 0) {
            particle->position.x += particle->velocityX;
            particle->position.y += particle->velocityY;
            particle->position.z += particle->velocityZ;
            if (particle->particleType != 0) {
                particle->velocityX -= particle->decelerationX;
                particle->velocityY += particle->accelerationY;
                particle->velocityZ -= particle->decelerationZ;
                particle->decelerationX = 0.01f * particle->velocityX;
                particle->accelerationY *= 1.04f;
                particle->decelerationZ = 0.01f * particle->velocityZ;
            }
            else {
                particle->velocityY += particle->accelerationY;
                particle->accelerationY *= 1.1f;
            }
            particle->scale += particle->scaleDelta;
            particle->rotation.x = particle->rotation.y = particle->rotation.z = particle->scale;
            particle->opacity -= particle->opacityDelta;
            if (particle->opacity > 255.0f) {
                particleAlpha = 0xFF;
            }
            else {
                particleAlpha = (s32)particle->opacity;
            }
            particleColor->a = (s32)particleAlpha;
            particle->animationFrame++;
            if (particle->animationFrame >= particle->animationFrameCount) {
                particle->animationFrame = 0;
                particle->textureTileIndex++;
            }
            if (particle->textureTileIndex >= particleSystem->textureTileCount) {
                particle->active = 0;
            }
        }
    }
}

s16 CalculateParticleAnimationFrame(float elapsedFrames, float animationSpeed, float frameOffset, s16 frameCount)
{
    Vec particlePosition;
    Vec particleRotation;
    GXColor particleColor;
    ParticleState *particle;

    particlePosition.x = elapsedFrames;
    particlePosition.y = animationSpeed;
    particlePosition.z = frameOffset;
    particleRotation.x = particleRotation.y = particleRotation.z = 1.0f;
    particleColor.r = 0xFF;
    particleColor.g = 0xA0;
    particleColor.b = 0x60;
    particleColor.a = 0xFF;
    particle = CreateMapParticle(frameCount, &particlePosition, &particleRotation, 0.0f, &particleColor);
    if (!particle) {
        return -1;
    }
    particle->scale = 0.0f;
    return particle->particleIndex;
}

ParticleState *CreateMapParticle(s16 particleType, Vec *position, Vec *velocity, float rotationAngle, GXColor *particleColor)
{
    ParticleState *particle;
    ParticleSystemRenderData *particleSystem;
    s16 particleIndex;
    HU3DMODEL *particleModel;

    particleModel = &Hu3DData[particleType];
    particleSystem = particleModel->hookData;
    for (particleIndex = 0, particle = particleSystem->particles; particleIndex < particleSystem->particleCount; particleIndex++, particle++) {
        if (particle->active == 0) {
            break;
        }
    }
    if (particleIndex == particleSystem->particleCount) {
        return NULL;
    }
    particleSystem->colors[particleIndex].r = particleColor->r;
    particleSystem->colors[particleIndex].g = particleColor->g;
    particleSystem->colors[particleIndex].b = particleColor->b;
    particleSystem->colors[particleIndex].a = particleColor->a;
    particle->particleIndex = particleIndex;
    particle->rotationAngle = rotationAngle;
    particle->rotation = *velocity;
    particle->position = *position;
    particle->textureTileIndex = 0;
    particle->textureIndex = 0;
    particle->active = 1;
    return particle;
}

s16 CreateMapAnimation(ANIMDATA *animationData, s16 animationType, float animationSpeed, s16 horizontalTileCount, s16 verticalTileCount)
{
    float textureFormatScratch;
    ParticleSystemRenderData *particleSystem;
    s16 particleSlotIndex;
    Vec2f *textureCoordinate;
    HU3DMODEL *particleModel;
    GXColor *particleColor;
    Vec *particleVertex;
    ParticleState *particle;
    s16 horizontalTileCount;
    void *displayListAllocation;
    s16 particleHookModelId;
    s16 verticalTileCount;
    void *displayListWorkBuffer;

    particleHookModelId = Hu3DHookFuncCreate(RenderMapParticles);
    particleModel = &Hu3DData[particleHookModelId];
    particleSystem = HuMemDirectMallocNum(HEAP_MODEL, sizeof(ParticleSystemRenderData), particleModel->mallocNo);
    particleModel->hookData = particleSystem;
    animationData->useNum++;
    particleSystem->animationData = animationData;
    particleSystem->particleCount = animationType;
    particleSystem->renderMode = 0;
    particleSystem->particleHook = 0;
    particle = HuMemDirectMallocNum(HEAP_MODEL, animationType * sizeof(ParticleState), particleModel->mallocNo);
    particleSystem->particles = particle;
    for (particleSlotIndex = 0; particleSlotIndex < animationType; particleSlotIndex++, particle++) {
        particle->textureTileIndex = -1;
        particle->active = 0;
    }
    particleVertex = HuMemDirectMallocNum(HEAP_MODEL, animationType * sizeof(Vec) * 4, particleModel->mallocNo);
    particleSystem->positions = particleVertex;
    for (particleSlotIndex = 0; particleSlotIndex < (animationType * 4); particleSlotIndex++, particleVertex++) {
        particleVertex->x = particleVertex->y = particleVertex->z = 0.0f;
    }
    particleColor = HuMemDirectMallocNum(HEAP_MODEL, animationType * sizeof(GXColor), particleModel->mallocNo);
    particleSystem->colors = particleColor;
    for (particleSlotIndex = 0; particleSlotIndex < animationType; particleSlotIndex++, particleColor++) {
        particleColor->r = particleColor->g = particleColor->b = particleColor->a = 0xFF;
    }
    textureCoordinate = HuMemDirectMallocNum(HEAP_MODEL, animationType * sizeof(Vec2f) * 4, particleModel->mallocNo);
    particleSystem->textureCoordinates = textureCoordinate;
    for (particleSlotIndex = 0; particleSlotIndex < animationType; particleSlotIndex++) {
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
    if ((horizontalTileCount != 0) || (verticalTileCount != 0)) {
        horizontalTileCount = animationData->bmp->sizeX / horizontalTileCount;
        verticalTileCount = animationData->bmp->sizeY / verticalTileCount;
        particleSystem->textureTileCount = horizontalTileCount * verticalTileCount;
        particleSystem->textureScaleX = 1.0f / horizontalTileCount;
        particleSystem->textureScaleY = 1.0f / verticalTileCount;
    }
    else {
        horizontalTileCount = 1;
        particleSystem->textureTileCount = 1;
        particleSystem->textureScaleX = 1.0f;
        particleSystem->textureScaleY = 1.0f;
    }
    particleSystem->textureTiles = HuMemDirectMallocNum(HEAP_MODEL, particleSystem->textureTileCount * sizeof(Vec2f), particleModel->mallocNo);
    SetTextureScrollOffset(particleSystem->textureTiles, particleSystem->textureTileCount, horizontalTileCount, particleSystem->textureScaleX, particleSystem->textureScaleY);
    particleSystem->baseRotation.x = particleSystem->bottomRightY = particleSystem->topLeftY = particleSystem->bottomLeftY = -animationSpeed;
    particleSystem->topLeftX = particleSystem->bottomLeftX = particleSystem->baseRotation.y = particleSystem->topRightX = animationSpeed;
    particleSystem->baseRotation.z = particleSystem->bottomRightX = particleSystem->topRightY = particleSystem->particleScale = 0.0f;
    displayListAllocation = HuMemDirectMallocNum(HEAP_MODEL, 0x20000, particleModel->mallocNo);
    displayListWorkBuffer = displayListAllocation;
    DCFlushRange(displayListAllocation, 0x20000);
    GXBeginDisplayList(displayListWorkBuffer, 0x20000);
    GXBegin(GX_QUADS, GX_VTXFMT0, animationType * 4);
    for (particleSlotIndex = 0; particleSlotIndex < animationType; particleSlotIndex++) {
        GXPosition1x16(particleSlotIndex * 4);
        GXPosition1x16(particleSlotIndex);
        GXPosition1x16(particleSlotIndex * 4);
        GXPosition1x16(particleSlotIndex * 4 + 1);
        GXPosition1x16(particleSlotIndex);
        GXPosition1x16(particleSlotIndex * 4 + 1);
        GXPosition1x16(particleSlotIndex * 4 + 2);
        GXPosition1x16(particleSlotIndex);
        GXPosition1x16(particleSlotIndex * 4 + 2);
        GXPosition1x16(particleSlotIndex * 4 + 3);
        GXPosition1x16(particleSlotIndex);
        GXPosition1x16(particleSlotIndex * 4 + 3);
    }
    particleSystem->displayListSize = GXEndDisplayList();
    particleSystem->displayList = HuMemDirectMallocNum(HEAP_MODEL, particleSystem->displayListSize, particleModel->mallocNo);
    memcpy(particleSystem->displayList, displayListAllocation, particleSystem->displayListSize);
    DCFlushRange(particleSystem->displayList, particleSystem->displayListSize);
    HuMemDirectFree(displayListAllocation);
    return particleHookModelId;
}

void DestroyMapAnimation(s16 animationId)
{
    HU3DMODEL *particleModel;
    ParticleSystemRenderData *particleSystem;

    particleModel = &Hu3DData[animationId];
    particleSystem = particleModel->hookData;
    HuSprAnimKill(particleSystem->animationData);
    Hu3DModelKill(animationId);
}

void *GetMapAnimationData(s16 animationId)
{
    HU3DMODEL *particleModel;

    particleModel = &Hu3DData[animationId];
    return particleModel->hookData;
}

ParticleState *FindMapParticle(s16 animationId, s16 particleIndex)
{
    HU3DMODEL *particleModel;
    ParticleSystemRenderData *particleSystem;

    particleModel = &Hu3DData[animationId];
    particleSystem = particleModel->hookData;
    if (particleIndex == -1) {
        return NULL;
    }
    return &particleSystem->particles[particleIndex];
}

void SetParticleRenderCallback(s16 animationId, ParticleRenderCallback renderCallback)
{
    HU3DMODEL *particleModel;
    ParticleSystemRenderData *particleSystem;

    particleModel = &Hu3DData[animationId];
    particleSystem = particleModel->hookData;
    particleSystem->particleHook = renderCallback;
}

void SetParticleRenderMode(s16 animationId, u8 renderMode)
{
    HU3DMODEL *particleModel;
    ParticleSystemRenderData *particleSystem;

    particleModel = &Hu3DData[animationId];
    particleSystem = particleModel->hookData;
    particleSystem->renderMode = renderMode;
}

void RenderMapParticles(HU3DMODEL *model, Mtx matrix)
{
    Mtx modelTransform;
    ROMtx reorderedModelTransform;
    Mtx particleRotationMatrix;
    Mtx particleTransform;
    Vec unrotatedQuadCorners[4];
    Vec rotatedQuadCorners[4];
    Vec bottomLeftPosition;
    Vec bottomRightPosition;
    Vec topRightPosition;
    Vec topLeftPosition;
    Vec *particleVertex;
    ParticleSystemRenderData *particleSystem;
    ParticleState *particle;
    Vec2f *textureCoordinate;
    s16 particleIndex;
    s16 bitmapFormat;
    ParticleRenderCallback particleUpdateHook;

    particleSystem = model->hookData;
    GXLoadPosMtxImm(matrix, 0);
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3C, GX_FALSE, 0x7D);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    bitmapFormat = particleSystem->animationData->bmp->dataFmt & 0xF;
    if ((bitmapFormat == 7) || (bitmapFormat == 8)) {
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXA, GX_CC_RASC, GX_CC_ZERO);
    }
    else {
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
    }
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    if ((model->attr & 2) != 0) {
        GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    }
    else {
        GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
    }
    HuSprTexLoad(particleSystem->animationData, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZCompLoc(0);
    switch (particleSystem->renderMode) {
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
    GXSetArray(GX_VA_POS, particleSystem->positions, 0xC);
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, particleSystem->colors, 4);
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_TEX0, particleSystem->textureCoordinates, 8);
    modelTransform[0][0] = matrix[0][0];
    modelTransform[1][0] = matrix[0][1];
    modelTransform[2][0] = matrix[0][2];
    modelTransform[0][1] = matrix[1][0];
    modelTransform[1][1] = matrix[1][1];
    modelTransform[2][1] = matrix[1][2];
    modelTransform[0][2] = matrix[2][0];
    modelTransform[1][2] = matrix[2][1];
    modelTransform[2][2] = matrix[2][2];
    modelTransform[0][3] = 0.0f;
    modelTransform[1][3] = 0.0f;
    modelTransform[2][3] = 0.0f;
    PSMTXReorder(modelTransform, reorderedModelTransform);
    if ((omPauseChk() == 0) && (particleSystem->particleHook)) {
        particleUpdateHook = particleSystem->particleHook;
        particleUpdateHook(model, particleSystem, matrix);
    }
    particle = particleSystem->particles;
    particleVertex = particleSystem->positions;
    textureCoordinate = particleSystem->textureCoordinates;
    PSMTXROMultVecArray(reorderedModelTransform, &particleSystem->baseRotation, unrotatedQuadCorners, 4);
    for (particleIndex = 0; particleIndex < particleSystem->particleCount; particleIndex++, particle++, textureCoordinate += 4) {
        if (particle->active == 0) {
            particleVertex->x = particleVertex->y = particleVertex->z = 0.0f;
            particleVertex++;
            particleVertex->x = particleVertex->y = particleVertex->z = 0.0f;
            particleVertex++;
            particleVertex->x = particleVertex->y = particleVertex->z = 0.0f;
            particleVertex++;
            particleVertex->x = particleVertex->y = particleVertex->z = 0.0f;
            particleVertex++;
        }
        else {
            if (particle->rotationAngle == 0.0f) {
                CalculateEffectDirection(unrotatedQuadCorners, rotatedQuadCorners, &particle->rotation, 4);
                VECAdd(&rotatedQuadCorners[0], &particle->position, particleVertex++);
                VECAdd(&rotatedQuadCorners[1], &particle->position, particleVertex++);
                VECAdd(&rotatedQuadCorners[2], &particle->position, particleVertex++);
                VECAdd(&rotatedQuadCorners[3], &particle->position, particleVertex++);
            }
            else {
                CalculateEffectDirection(&particleSystem->baseRotation, rotatedQuadCorners, &particle->rotation, 4);
                MTXRotRad(particleRotationMatrix, 0x5A, MTXDegToRad(particle->rotationAngle));
                MTXConcat(modelTransform, particleRotationMatrix, particleTransform);
                MTXMultVecArray(particleTransform, rotatedQuadCorners, &topLeftPosition, 4);
                VECAdd(&topLeftPosition, &particle->position, particleVertex++);
                VECAdd(&topRightPosition, &particle->position, particleVertex++);
                VECAdd(&bottomRightPosition, &particle->position, particleVertex++);
                VECAdd(&bottomLeftPosition, &particle->position, particleVertex++);
            }
            if (particle->textureTileIndex != -1) {
                InterpolateTextureScroll(textureCoordinate, particleSystem->textureTiles, particle->textureTileIndex, particleSystem->textureScaleX, particleSystem->textureScaleY);
            }
        }
    }
    DCFlushRangeNoSync(particleSystem->positions, particleSystem->particleCount * sizeof(Vec) * 4);
    DCFlushRangeNoSync(particleSystem->textureCoordinates, particleSystem->particleCount * sizeof(Vec2f) * 4);
    PPCSync();
    GXCallDisplayList(particleSystem->displayList, particleSystem->displayListSize);
}

void SetTextureScrollOffset(Vec2f *textureOffset, s16 textureTileX, s16 textureTileY, float scrollX, float scrollY)
{
    s16 tileIndex;
    s16 tileColumn;
    s16 tileRow;

    for (tileIndex = 0; tileIndex < textureTileX; tileIndex++, textureOffset++) {
        tileColumn = tileIndex % textureTileY;
        tileRow = tileIndex / textureTileY;
        textureOffset->x = tileColumn * scrollX;
        textureOffset->y = tileRow * scrollY;
    }
}

void InterpolateTextureScroll(Vec2f *sourceOffset, Vec2f *targetOffset, s16 interpolationFrames, float scrollX, float scrollY)
{
    sourceOffset[0].x = targetOffset[interpolationFrames].x;
    sourceOffset[0].y = targetOffset[interpolationFrames].y;
    sourceOffset[1].x = scrollX + targetOffset[interpolationFrames].x;
    sourceOffset[1].y = targetOffset[interpolationFrames].y;
    sourceOffset[2].x = scrollX + targetOffset[interpolationFrames].x;
    sourceOffset[2].y = scrollY + targetOffset[interpolationFrames].y;
    sourceOffset[3].x = targetOffset[interpolationFrames].x;
    sourceOffset[3].y = scrollY + targetOffset[interpolationFrames].y;
}

void CalculateEffectDirection(Vec *sourcePosition, Vec *targetPosition, Vec *direction, s16 teamIndex)
{
    s16 cornerIndex;

    for (cornerIndex = 0; cornerIndex < teamIndex; cornerIndex++, sourcePosition++, targetPosition++) {
        targetPosition->x = sourcePosition->x * direction->x;
        targetPosition->y = sourcePosition->y * direction->y;
        targetPosition->z = sourcePosition->z * direction->z;
    }
}

void InitializeMapEffectResources(void)
{
    if ((HuPadBtnDown[0] & PAD_TRIGGER_L) != 0) {
        mapEffectState ^= 1;
    }
    if (mapEffectState != 0) {
        if ((HuPadBtnDown[0] & PAD_TRIGGER_Z) != 0) {
            mapRenderState = mapRenderState ^ 1;
        }
        if ((HuPadBtn[0] & PAD_BUTTON_LEFT) != 0) {
            CRotM[mapRenderState].y += 1.0f;
        }
        if ((HuPadBtn[0] & PAD_BUTTON_RIGHT) != 0) {
            CRotM[mapRenderState].y -= 1.0f;
        }
        if ((HuPadBtn[0] & PAD_BUTTON_UP) != 0) {
            CRotM[mapRenderState].x -= 1.0f;
        }
        if ((HuPadBtn[0] & PAD_BUTTON_DOWN) != 0) {
            CRotM[mapRenderState].x += 1.0f;
        }
        CenterM[mapRenderState].x += HuPadSubStkX[0];
        if ((HuPadBtn[0] & PAD_TRIGGER_R) != 0) {
            CenterM[mapRenderState].y += HuPadSubStkY[0];
        }
        else {
            CenterM[mapRenderState].z += HuPadSubStkY[0];
        }
        if ((HuPadBtn[0] & PAD_BUTTON_X) != 0) {
            CZoomM[mapRenderState] += 10.0f;
        }
        if ((HuPadBtn[0] & PAD_BUTTON_Y) != 0) {
            CZoomM[mapRenderState] -= 10.0f;
        }
        if ((HuPadBtnDown[0] & PAD_BUTTON_A) != 0) {
            OSReport("\nCZoom = %.2f \n", CZoomM[mapRenderState]);
            OSReport("Center x = %.2f: y = %.2f: z = %.2f \n", CenterM[mapRenderState].x, CenterM[mapRenderState].y, CenterM[mapRenderState].z);
            OSReport("CRot x = %.2f: y = %.2f: z = %.2f \n", CRotM[mapRenderState].x, CRotM[mapRenderState].y, CRotM[mapRenderState].z);
        }
    }
}

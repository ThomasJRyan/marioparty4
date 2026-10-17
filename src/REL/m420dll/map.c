#include "ext_math.h"
#include "game/hu3d.h"

#include "REL/m420dll.h"

#ifndef __MWERKS__
#include "game/audio.h"
#endif

typedef struct M420DllMapObject {
    /* 0x00 */ struct {
        s8 isVisible : 1;
        s8 isAnimated : 1;
        s8 isAttached : 1;
        s8 useLighting : 1;
        s8 isTransparent : 1;
        s8 isWater : 1;
        s8 isActive : 1;
        s8 isShadowEnabled : 1;
    };
    /* 0x04 */ s16 mapVariant;
    /* 0x04 */ Vec mapEffects;
    /* 0x10 */ Vec mapTextures;
    /* 0x1C */ Vec mapScale;
    /* 0x28 */ Vec mapOffsetY;
    /* 0x34 */ Vec startingRotation;
    /* 0x40 */ Vec targetRotation;
    /* 0x4C */ Vec targetScale;
    /* 0x58 */ Vec animationPosition;
    /* 0x64 */ Vec animationRotation;
    /* 0x70 */ float animationFrame;
    /* 0x74 */ float animationSpeed;
    /* 0x78 */ s32 positionEasingMode;
    /* 0x7C */ float animationTimer;
    /* 0x80 */ float animationProgress;
    /* 0x84 */ s32 effectState;
    /* 0x88 */ float effectTimer;
    /* 0x8C */ float effectScale;
    /* 0x90 */ s32 effectFlags;
} M420DllMapObject; /* size = 0x94 */

typedef struct M420DllMapEffect {
    /* 0x00 */ M420DllMapObject *mapObjects;
    /* 0x04 */ M420DllMapObject *mapEffects;
    /* 0x08 */ M420DllMapObject *mapAnimations;
    /* 0x0C */ M420DllMapObject *mapLights;
    /* 0x10 */ M420DllMapObject *mapTextures;
    /* 0x14 */ s32 mapObjectCount;
    /* 0x18 */ char pad18[4];
    /* 0x1C */ float mapScale;
    /* 0x20 */ float mapRotation;
    /* 0x24 */ float mapOffsetX;
    /* 0x28 */ float mapOffsetY;
    /* 0x2C */ float mapOffsetZ;
    /* 0x30 */ s32 mapState;
    /* 0x34 */ s32 animationState;
    /* 0x38 */ s32 effectState;
    /* 0x3C */ s32 mapFlags;
} M420DllMapEffect; /* size = 0x40 */

typedef struct M420DllMapLightConfig {
    /* 0x00 */ Vec mapObjects;
    /* 0x0C */ Vec mapLights;
    /* 0x18 */ GXColor mapTintColor;
} M420DllMapLightConfig; /* size = 0x1C */

s32 MapEventIsRunning(s32 eventIndex);
void UpdateRoundCounterProcess(void);
void UpdateMapObjectsProcess(void);
void LoadMapModels(void);
void UnloadMapModels(void);
void InitializeMapLighting(void);
void UpdateMapObjectTransform(M420DllMapObject *mapObject);
void AnimateMapObjectPosition(M420DllMapObject *mapObject, Vec *targetPosition, s32 durationFrames, s32 easingMode);
void RunMapEventIntro(void);
void RunMapEventMiddle(void);
void RunMapEventFinale(void);
void RunMapEventCleanup(void);

M420DllMapLightConfig mapLightConfig = {
    { 1000.0f, 2000.0f, 0.0f },
    { -0.5f, -0.5f, -0.5f },
    { 0xFF, 0xFF, 0xFF, 0xFF },
};
void (*mapEventCallbacks[4])() = { RunMapEventIntro, RunMapEventMiddle, RunMapEventFinale, RunMapEventCleanup };

static const s32 mapModelDataIds[0x1B] = {
    DATA_MAKE_NUM(DATADIR_M420, 0x00),
    DATA_MAKE_NUM(DATADIR_M420, 0x14),
    DATA_MAKE_NUM(DATADIR_M420, 0x07),
    DATA_MAKE_NUM(DATADIR_M420, 0x08),
    DATA_MAKE_NUM(DATADIR_M420, 0x01),
    DATA_MAKE_NUM(DATADIR_M420, 0x02),
    DATA_MAKE_NUM(DATADIR_M420, 0x03),
    DATA_MAKE_NUM(DATADIR_M420, 0x04),
    DATA_MAKE_NUM(DATADIR_M420, 0x05),
    DATA_MAKE_NUM(DATADIR_M420, 0x06),
    DATA_MAKE_NUM(DATADIR_M420, 0x15),
    DATA_MAKE_NUM(DATADIR_M420, 0x16),
    DATA_MAKE_NUM(DATADIR_M420, 0x17),
    DATA_MAKE_NUM(DATADIR_M420, 0x09),
    DATA_MAKE_NUM(DATADIR_M420, 0x09),
    DATA_MAKE_NUM(DATADIR_M420, 0x0A),
    DATA_MAKE_NUM(DATADIR_M420, 0x0B),
    DATA_MAKE_NUM(DATADIR_M420, 0x0E),
    DATA_MAKE_NUM(DATADIR_M420, 0x0F),
    DATA_MAKE_NUM(DATADIR_M420, 0x0C),
    DATA_MAKE_NUM(DATADIR_M420, 0x0D),
    DATA_MAKE_NUM(DATADIR_M420, 0x12),
    DATA_MAKE_NUM(DATADIR_M420, 0x11),
    DATA_MAKE_NUM(DATADIR_M420, 0x10),
    DATA_MAKE_NUM(DATADIR_M420, 0x10),
    DATA_MAKE_NUM(DATADIR_M420, 0x13),
    DATA_MAKE_NUM(DATADIR_M420, 0x13),
};

HUPROCESS *roundCounterProcess;
HUPROCESS *mapUpdateProcess;
HUPROCESS *mapEventProcesses[4];
s32 mapEventState;
s16 mapModelIds[0x1C];
s16 mapGlobalLightId;
s16 mapLightId;
M420DllMapObject mapObjects[15];
struct M420DllMapEffect mapEffects[2];
s32 roundCounterValue;
s32 roundCounterTimer;
s32 mapRoundIndex;
s32 mapRevealComplete;

s32 InitializeMapSystem(void)
{
    M420DllMapObject *mapObject;
    M420DllMapEffect *mapEffect;
    s32 eventIndex;

    roundCounterProcess = HuPrcChildCreate(UpdateRoundCounterProcess, 0x3E9, 0x4000, 0, minigameObjectManager);
    mapUpdateProcess = HuPrcChildCreate(UpdateMapObjectsProcess, 0x3EA, 0x4000, 0, minigameObjectManager);
    for (eventIndex = 0; eventIndex < 4; eventIndex++) {
        mapEventProcesses[eventIndex] = NULL;
    }
    mapEventState = 0;
    InitializeMapLighting();
    LoadMapModels();

    mapObject = mapObjects;
    mapObject->mapVariant = mapModelIds[1];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = 0.0f;
    mapObject->mapEffects.y = 1000.0f;
    mapObject->mapEffects.z = 0.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[1];
    mapObject->mapVariant = mapModelIds[0xA];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = 0.0f;
    mapObject->mapEffects.y = 0.0f;
    mapObject->mapEffects.z = 0.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[2];
    mapObject->mapVariant = mapModelIds[0xC];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = 0.0f;
    mapObject->mapEffects.y = 0.0f;
    mapObject->mapEffects.z = 0.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[3];
    mapObject->mapVariant = mapModelIds[0xB];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = 0.0f;
    mapObject->mapEffects.y = 0.0f;
    mapObject->mapEffects.z = 0.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[4];
    mapObject->mapVariant = mapModelIds[0xD];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = -220.0f;
    mapObject->mapEffects.y = -60.0f;
    mapObject->mapEffects.z = 170.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[9];
    mapObject->mapVariant = mapModelIds[0x17];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = -170.0f;
    mapObject->mapEffects.y = 0.0f;
    mapObject->mapEffects.z = 170.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[11];
    mapObject->mapVariant = mapModelIds[0x19];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = -160.0f;
    mapObject->mapEffects.y = 0.0f;
    mapObject->mapEffects.z = 220.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[6];
    mapObject->mapVariant = mapModelIds[0x11];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = 0.0f;
    mapObject->mapEffects.y = -65.0f;
    mapObject->mapEffects.z = 0.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[5];
    mapObject->mapVariant = mapModelIds[0xE];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = 220.0f;
    mapObject->mapEffects.y = -60.0f;
    mapObject->mapEffects.z = 170.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[10];
    mapObject->mapVariant = mapModelIds[0x18];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = 190.0f;
    mapObject->mapEffects.y = 0.0f;
    mapObject->mapEffects.z = 170.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[12];
    mapObject->mapVariant = mapModelIds[0x1A];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = 185.0f;
    mapObject->mapEffects.y = 0.0f;
    mapObject->mapEffects.z = 220.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[7];
    mapObject->mapVariant = mapModelIds[0x12];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = 0.0f;
    mapObject->mapEffects.y = -65.0f;
    mapObject->mapEffects.z = 0.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[8];
    mapObject->mapVariant = mapModelIds[0x13];
    mapObject->useLighting = 1;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = 0.0f;
    mapObject->mapEffects.y = 300.0f;
    mapObject->mapEffects.z = 0.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[13];
    mapObject->mapVariant = mapModelIds[3];
    mapObject->useLighting = 0;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = 0.0f;
    mapObject->mapEffects.y = 0.0f;
    mapObject->mapEffects.z = 0.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    mapObject = &mapObjects[14];
    mapObject->mapVariant = mapModelIds[2];
    mapObject->useLighting = 0;
    mapObject->isVisible = 0;
    mapObject->isAnimated = 0;
    mapObject->mapEffects.x = 0.0f;
    mapObject->mapEffects.y = 0.0f;
    mapObject->mapEffects.z = 0.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    mapObject->targetScale.x = 1.0f;
    mapObject->targetScale.y = 1.0f;
    mapObject->targetScale.z = 1.0f;
    UpdateMapObjectTransform(mapObject);

    for (eventIndex = 0; eventIndex < 2; eventIndex++) {
        mapEffect = &mapEffects[eventIndex];
        mapEffect->mapObjectCount = eventIndex;
        mapEffect->mapScale = 20.0f;
        mapEffect->mapRotation = 0.0f;
        mapEffect->mapOffsetZ = 0.0f;
        mapEffect->mapOffsetX = 0.2f;
        mapEffect->mapOffsetY = 0.1f;
        mapEffect->effectState = 0;
        mapEffect->mapFlags = 0;
        mapEffect->mapState = 1;
        mapEffect->mapObjects = &mapObjects[eventIndex + 13];
        mapEffect->mapEffects = &mapObjects[eventIndex + 4];
        mapEffect->mapAnimations = &mapObjects[eventIndex + 6];
        mapEffect->mapLights = &mapObjects[eventIndex + 9];
        mapEffect->mapTextures = &mapObjects[eventIndex + 11];
    }
    roundCounterTimer = 0;
    mapRoundIndex = 0;
    mapRevealComplete = 0;
    HideMapMarker();

    {
        Vec secondaryTransformVector = { 0.0f, 0.0f, 0.0f };
        Vec transformVector = { 0.0f, 0.0f, -1.0f };
        HuAudFXListnerSetEX(&secondaryTransformVector, &transformVector, 3000.0, 1000.0, 100.0, 300.0, 300.0);
        HuAudFXPlay(0x5F0);
    }
    return 1;
}

void ShutdownMapSystem(void)
{
    HUPROCESS *process;
    s32 eventIndex;

    for (eventIndex = 0; eventIndex < 4; eventIndex++) {
        process = mapEventProcesses[eventIndex];
        if (process) {
            HuPrcKill(process);
        }
    }
    UnloadMapModels();
    HuPrcKill(mapUpdateProcess);
    HuPrcKill(roundCounterProcess);
}

s32 StartMapEvent(s32 eventIndex)
{
    if (MapEventIsRunning(eventIndex) != 0) {
        return 0;
    }
    mapEventProcesses[eventIndex] = HuPrcChildCreate(mapEventCallbacks[eventIndex], 0x3ED, 0x5000, 0, minigameObjectManager);
    return 1;
}

s32 MapEventIsRunning(s32 eventIndex)
{
    return mapEventProcesses[eventIndex] != NULL;
}

s32 AdvanceMapReveal(s32 effectIndex, float scaleStep)
{
    M420DllMapEffect *mapEffect;
    s32 secondaryModelIndex;
    s32 modelIndex;

    mapEffect = &mapEffects[effectIndex];
    if ((mapEffect->mapScale += scaleStep) >= 200.0f) {
        mapEffect->mapScale = 200.0f;
    }
    mapEffect->effectState = 1;
    mapEffect->mapState = (s32)(200.0f - mapEffect->mapScale) / 10.0f;
    switch (effectIndex) {
        case 0:
            secondaryModelIndex = 3;
            modelIndex = 0x15;
            break;
        case 1:
            secondaryModelIndex = 2;
            modelIndex = 0x16;
            break;
    }
    if (mapEffect->mapScale >= 200.0f) {
        if (mapRevealComplete == 0) {
            mapEffect->mapFlags = 1;
            Hu3DMotionTimeSet(mapModelIds[modelIndex], 0.0f);
            Hu3DModelAttrReset(mapModelIds[modelIndex], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(mapModelIds[modelIndex], HU3D_MOTATTR_LOOP);
            Hu3DModelAttrSet(mapModelIds[secondaryModelIndex], HU3D_ATTR_DISPOFF);
        }
        return 1;
    }
    return 0;
}

void UpdateMapEffect(M420DllMapEffect *mapEffect)
{
    Vec transformVector;

    if (mapEffect->effectState != 0) {
        if (mapEffect->mapState > 0) {
            mapEffect->mapState--;
        }
        if (mapEffect->mapState <= 0) {
            transformVector = mapEffect->mapEffects->mapEffects;
            transformVector.y = -60.0f + mapEffect->mapScale;
            AnimateMapObjectPosition(mapEffect->mapEffects, &transformVector, 0x1E, 0);

            transformVector = mapEffect->mapAnimations->mapEffects;
            transformVector.y = -65.0f + mapEffect->mapScale;
            AnimateMapObjectPosition(mapEffect->mapAnimations, &transformVector, 0x1E, 0);

            transformVector = mapEffect->mapLights->mapEffects;
            transformVector.y = 40.0f + mapEffect->mapScale;
            AnimateMapObjectPosition(mapEffect->mapLights, &transformVector, 0x1E, 1);

            Hu3DMotionTimeSet(mapEffect->mapLights->mapVariant, 0.0f);
            Hu3DModelAttrReset(mapEffect->mapLights->mapVariant, HU3D_ATTR_DISPOFF);

            transformVector = mapEffect->mapTextures->mapEffects;
            transformVector.y = -50.0f + mapEffect->mapScale;
            AnimateMapObjectPosition(mapEffect->mapTextures, &transformVector, 0x1E, 1);

            Hu3DMotionTimeSet(mapEffect->mapTextures->mapVariant, 0.0f);
            Hu3DModelAttrReset(mapEffect->mapTextures->mapVariant, HU3D_ATTR_DISPOFF);
            Hu3DMotionStartEndSet(mapEffect->mapObjects->mapVariant, 0.0f, 30.0f);
            Hu3DMotionTimeSet(mapEffect->mapObjects->mapVariant, 0.0f);
            Hu3DModelAttrReset(mapEffect->mapObjects->mapVariant, HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrReset(mapEffect->mapObjects->mapVariant, HU3D_MOTATTR_LOOP);
            mapEffect->animationState = 3;
            mapEffect->effectState = 0;
            mapEffect->mapOffsetX = 0.8f;
            mapEffect->mapOffsetY = 0.3f;
            mapEffect->mapOffsetZ = 0.0f;
            if (mapEffect->mapObjectCount == 0) {
                HuAudFXPlay(0x5F7);
            }
            else {
                HuAudFXPlay(0x5F8);
            }
        }
    }
    if ((Hu3DMotionEndCheck(mapEffect->mapObjects->mapVariant) != 0) && (mapEffect->animationState > 0)) {
        if (--mapEffect->animationState > 0) {
            Hu3DMotionTimeSet(mapEffect->mapObjects->mapVariant, 0.0f);
        }
        else {
            Hu3DMotionStartEndSet(mapEffect->mapObjects->mapVariant, 0.0f, 1.0f);
            Hu3DMotionTimeSet(mapEffect->mapObjects->mapVariant, 0.0f);
            Hu3DModelAttrSet(mapEffect->mapObjects->mapVariant, HU3D_MOTATTR_PAUSE);
        }
    }
    if (mapEffect->mapOffsetX > 0.2f) {
        if ((mapEffect->mapOffsetX -= 0.005f) <= 0.2f) {
            mapEffect->mapOffsetX = 0.2f;
        }
    }
    mapEffect->mapEffects->mapOffsetY.y += mapEffect->mapOffsetX;
    if (mapEffect->mapOffsetY > 0.05f) {
        if ((mapEffect->mapOffsetY -= 0.003f) <= 0.05f) {
            mapEffect->mapOffsetY = 0.05f;
        }
    }
    mapEffect->mapEffects->targetScale.y = 1.0 + (mapEffect->mapOffsetY * sind((90.0f * mapEffect->mapOffsetZ)));
    mapEffect->mapOffsetZ += 0.02f;
}

s32 AdvanceRoundCounter(void)
{
    float secondaryFrameProgress;
    float frameProgress;
    s32 result;

    if (roundCounterValue >= 6) {
        return 1;
    }
    secondaryFrameProgress = 5.0f * (6 - roundCounterValue);
    frameProgress = secondaryFrameProgress - 5.0f;
    Hu3DMotionStartEndSet(mapModelIds[0xF], frameProgress, secondaryFrameProgress);
    Hu3DMotionTimeSet(mapModelIds[0xF], secondaryFrameProgress);
    Hu3DModelAttrReset(mapModelIds[0xF], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(mapModelIds[0xF], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(mapModelIds[0xF], HU3D_MOTATTR_REV);
    roundCounterTimer = 5;
    roundCounterValue += 1;
    if (roundCounterValue == 6) {
        result = 1;
    }
    else {
        result = 0;
    }
    return result;
}

void ResetRoundCounter(void)
{
    float secondaryFrameProgress = 0.0f;
    float frameProgress = 30.0f;

    Hu3DMotionStartEndSet(mapModelIds[0xF], secondaryFrameProgress, frameProgress);
    Hu3DMotionTimeSet(mapModelIds[0xF], secondaryFrameProgress);
    Hu3DModelAttrReset(mapModelIds[0xF], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(mapModelIds[0xF], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(mapModelIds[0xF], HU3D_MOTATTR_REV);
    Hu3DMotionStartEndSet(mapModelIds[0x10], secondaryFrameProgress, frameProgress);
    Hu3DMotionTimeSet(mapModelIds[0x10], secondaryFrameProgress);
    roundCounterValue = 0;
}

void ResetMapMotion(void)
{
    float secondaryFrameProgress = 0.0f;
    float frameProgress = 30.0f;

    Hu3DMotionStartEndSet(mapModelIds[0x14], secondaryFrameProgress, frameProgress);
    Hu3DMotionTimeSet(mapModelIds[0x14], secondaryFrameProgress);
}

void FinishRoundCounter(void)
{
    float secondaryFrameProgress = 5.0f * (6 - roundCounterValue);
    float frameProgress = 30.0f;

    Hu3DMotionStartEndSet(mapModelIds[0xF], secondaryFrameProgress, frameProgress);
    Hu3DMotionTimeSet(mapModelIds[0xF], secondaryFrameProgress);
    Hu3DModelAttrReset(mapModelIds[0xF], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(mapModelIds[0xF], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(mapModelIds[0xF], HU3D_MOTATTR_REV);
    roundCounterValue = 0;
    roundCounterTimer = 0;
}

s32 RoundCounterIsFinished(void)
{
    s32 result = 0;

    if ((roundCounterValue == 6) && (roundCounterTimer == 0)) {
        result = 1;
    }
    return result;
}

void ShowMapMarker(void)
{
    Vec transformVector;
    M420DllMapObject *mapObject;

    mapObject = &mapObjects[8];
    mapObject->mapEffects.x = 480.0f;
    mapObject->mapEffects.y = 800.0f;
    mapObject->mapEffects.z = -314.0f;
    mapObject->mapOffsetY.x = 0.0f;
    mapObject->mapOffsetY.y = 0.0f;
    mapObject->mapOffsetY.z = 0.0f;
    UpdateMapObjectTransform(mapObject);
    transformVector.x = 480.0f;
    transformVector.y = 370.0f;
    transformVector.z = -314.0f;
    Hu3DModelAttrReset(mapObject->mapVariant, HU3D_ATTR_DISPOFF);
    AnimateMapObjectPosition(mapObject, &transformVector, 0x14, 0);
    Hu3DMotionTimeSet(GetMapMarkerModel(), 0.0f);
    Hu3DModelAttrSet(GetMapMarkerModel(), HU3D_MOTATTR_PAUSE);
}

s16 GetMapMarkerModel(void)
{
    return mapObjects[8].mapVariant;
}

s32 MapMarkerIsVisible(void)
{
    M420DllMapObject *mapObject = &mapObjects[8];
    return mapObject->isVisible;
}

void SetMapMarkerLighting(s32 enabled)
{
    M420DllMapObject *mapObject = &mapObjects[8];
    if (enabled == 0) {
        mapObject->mapEffects.x = 0.0f;
        mapObject->mapEffects.y = 0.0f;
        mapObject->mapEffects.z = 0.0f;
        UpdateMapObjectTransform(mapObject);
    }
    mapObject->useLighting = enabled;
}

void HideMapMarker(void)
{
    M420DllMapObject *mapObject = &mapObjects[8];
    mapObject->mapEffects.x = 0.0f;
    mapObject->mapEffects.y = 1000.0f;
    mapObject->mapEffects.z = 0.0f;
    UpdateMapObjectTransform(mapObject);
    Hu3DModelAttrSet(mapModelIds[0x13], HU3D_ATTR_DISPOFF);
}

s32 MapRevealIsFinished(s32 effectIndex)
{
    M420DllMapEffect *mapEffect = &mapEffects[effectIndex];
    return mapEffect->mapScale >= 200.0f;
}

s32 GetMapEffectFlags(s32 effectIndex)
{
    M420DllMapEffect *mapEffect = &mapEffects[effectIndex];
    return mapEffect->mapFlags;
}

s32 GetMapRoundIndex(void)
{
    return mapRoundIndex;
}

void UpdateRoundCounterProcess(void)
{
    while (TRUE) {
        if (roundCounterTimer > 0) {
            if (--roundCounterTimer <= 0) {
                Hu3DModelAttrSet(mapModelIds[0xF], HU3D_MOTATTR_PAUSE);
            }
        }
        HuPrcVSleep();
    }
}

void UpdateMapObjectsProcess(void)
{
    s32 mapObjectIndex;
    M420DllMapEffect *secondaryMapEffect;
    M420DllMapEffect *mapEffect;

    while (TRUE) {
        for (mapObjectIndex = 0; mapObjectIndex < 0xF; mapObjectIndex++) {
            UpdateMapObjectTransform(&mapObjects[mapObjectIndex]);
        }
        for (mapObjectIndex = 0; mapObjectIndex < 2; mapObjectIndex++) {
            UpdateMapEffect(&mapEffects[mapObjectIndex]);
        }
        secondaryMapEffect = mapEffects;
        if ((secondaryMapEffect->mapScale >= 200.0f) || (mapEffect = &mapEffects[1], mapEffect->mapScale >= 200.0f)) {
            mapRevealComplete = 1;
        }
        HuPrcVSleep();
    }
}

void LoadMapModels(void)
{
    s32 modelIndex;

    for (modelIndex = 0; modelIndex < 0x1B; modelIndex++) {
        mapModelIds[modelIndex] = Hu3DModelCreateFile(mapModelDataIds[modelIndex]);
    }
    Hu3DModelAttrSet(mapModelIds[1], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(mapModelIds[0xA], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(mapModelIds[3], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrSet(mapModelIds[2], HU3D_MOTATTR_PAUSE);
    Hu3DModelPosSet(mapModelIds[0xF], -219.0f, 300.0f, -92.0f);
    Hu3DModelRotSet(mapModelIds[0xF], 0.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(mapModelIds[0x10], 0.0f, 0.0f, 0.0f);
    Hu3DModelShadowMapSet(mapModelIds[0]);
    Hu3DModelShadowMapSet(mapModelIds[0xD]);
    Hu3DModelShadowMapSet(mapModelIds[0xE]);
    Hu3DModelShadowSet(mapModelIds[0xF]);
    Hu3DModelShadowSet(mapModelIds[1]);
    Hu3DModelShadowSet(mapModelIds[0xA]);
    Hu3DModelShadowSet(mapModelIds[0xB]);
    Hu3DModelShadowSet(mapModelIds[0x10]);
    Hu3DModelShadowSet(mapModelIds[0x14]);
    Hu3DModelShadowSet(mapModelIds[0x13]);
    Hu3DModelAttrSet(mapModelIds[4], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[5], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[6], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[7], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[8], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[9], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[0xB], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[0xC], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[0x15], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[0x16], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[0x17], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[0x18], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[0x19], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[0x1A], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[0xF], HU3D_MOTATTR_PAUSE);
    Hu3DMotionStartEndSet(mapModelIds[0xF], 30.0f, 30.0f);
    Hu3DMotionTimeSet(mapModelIds[0xF], 30.0f);
    Hu3DModelAttrSet(mapModelIds[0xF], HU3D_MOTATTR_PAUSE);
    Hu3DModelHookSet(mapModelIds[1], "itemhook_sao", mapModelIds[0xA]);
}

void UnloadMapModels(void)
{
    s32 modelIndex;

    for (modelIndex = 0; modelIndex < 0x1B; modelIndex++) {
        Hu3DModelKill(mapModelIds[modelIndex]);
    }
}

void InitializeMapLighting(void)
{
    HU3DLIGHT *globalLight;

    Hu3DLighInit();
    mapLightId = Hu3DGLightCreateV(&mapLightConfig.mapObjects, &mapLightConfig.mapLights, &mapLightConfig.mapTintColor);
    Hu3DGLightInfinitytSet(mapLightId);
    globalLight = &Hu3DGlobalLight[mapLightId];
    globalLight->type |= 0x8000;
}

void UpdateMapObjectTransform(M420DllMapObject *mapObject)
{
    float interpolationFactor;

    if (mapObject->useLighting) {
        if (mapObject->isVisible) {
            switch (mapObject->positionEasingMode) {
                case 0:
                    interpolationFactor = mapObject->animationFrame;
                    break;
                case 1:
                    interpolationFactor = sind((90.0f * mapObject->animationFrame));
                    interpolationFactor *= interpolationFactor;
                    break;
                case 2:
                    interpolationFactor = sind((90.0f * mapObject->animationFrame));
                    interpolationFactor *= interpolationFactor * interpolationFactor;
                    break;
            }
            mapObject->mapEffects.x = mapObject->mapTextures.x + (interpolationFactor * (mapObject->mapScale.x - mapObject->mapTextures.x));
            mapObject->mapEffects.y = mapObject->mapTextures.y + (interpolationFactor * (mapObject->mapScale.y - mapObject->mapTextures.y));
            mapObject->mapEffects.z = mapObject->mapTextures.z + (interpolationFactor * (mapObject->mapScale.z - mapObject->mapTextures.z));
            if ((mapObject->animationFrame += mapObject->animationSpeed) >= 1.0f) {
                mapObject->mapEffects = mapObject->mapScale;
                mapObject->isVisible = 0;
            }
        }
        Hu3DModelPosSet(mapObject->mapVariant, mapObject->mapEffects.x, mapObject->mapEffects.y, mapObject->mapEffects.z);
        if (mapObject->isAnimated) {
            switch (mapObject->effectState) {
                case 0:
                    interpolationFactor = mapObject->animationTimer;
                    break;
                case 1:
                    interpolationFactor = sind((90.0f * mapObject->animationTimer));
                    interpolationFactor *= interpolationFactor;
                    break;
            }
            mapObject->mapOffsetY.x = mapObject->startingRotation.x + (interpolationFactor * (mapObject->targetRotation.x - mapObject->startingRotation.x));
            mapObject->mapOffsetY.y = mapObject->startingRotation.y + (interpolationFactor * (mapObject->targetRotation.y - mapObject->startingRotation.y));
            mapObject->mapOffsetY.z = mapObject->startingRotation.z + (interpolationFactor * (mapObject->targetRotation.z - mapObject->startingRotation.z));
            if ((mapObject->animationTimer += mapObject->animationProgress) >= 1.0f) {
                mapObject->mapOffsetY = mapObject->targetRotation;
                mapObject->isAnimated = 0;
            }
        }
        Hu3DModelRotSet(mapObject->mapVariant, mapObject->mapOffsetY.x, mapObject->mapOffsetY.y, mapObject->mapOffsetY.z);
        if (mapObject->isAttached) {
            switch (mapObject->effectFlags) {
                case 0:
                    interpolationFactor = mapObject->effectTimer;
                    break;
                case 1:
                    interpolationFactor = sind((90.0f * mapObject->effectTimer));
                    interpolationFactor *= interpolationFactor;
                    break;
            }
            mapObject->targetScale.x = mapObject->animationPosition.x + (interpolationFactor * (mapObject->animationRotation.x - mapObject->animationPosition.x));
            mapObject->targetScale.y = mapObject->animationPosition.y + (interpolationFactor * (mapObject->animationRotation.y - mapObject->animationPosition.y));
            mapObject->targetScale.z = mapObject->animationPosition.z + (interpolationFactor * (mapObject->animationRotation.z - mapObject->animationPosition.z));
            if ((mapObject->effectTimer += mapObject->effectScale) >= 1.0f) {
                mapObject->targetScale = mapObject->animationRotation;
                mapObject->isAttached = 0;
            }
        }
        Hu3DModelScaleSet(mapObject->mapVariant, mapObject->targetScale.x, mapObject->targetScale.y, mapObject->targetScale.z);
    }
}

void AnimateMapObjectPosition(M420DllMapObject *mapObject, Vec *targetPosition, s32 durationFrames, s32 easingMode)
{
    mapObject->mapScale = *targetPosition;
    mapObject->mapTextures = mapObject->mapEffects;
    mapObject->animationFrame = 0.0f;
    mapObject->animationSpeed = 1.0f / durationFrames;
    mapObject->positionEasingMode = easingMode;
    mapObject->isVisible = 1;
}

void AnimateMapObjectRotation(M420DllMapObject *mapObject, Vec *targetRotation, s32 durationFrames, s32 easingMode)
{
    mapObject->targetRotation = *targetRotation;
    mapObject->startingRotation = mapObject->mapOffsetY;
    mapObject->animationTimer = 0.0f;
    mapObject->animationProgress = 1.0f / durationFrames;
    mapObject->effectState = easingMode;
    mapObject->isAnimated = 1;
}

void AnimateMapObjectScale(M420DllMapObject *mapObject, Vec *targetScale, s32 durationFrames, s32 easingMode)
{
    mapObject->animationRotation = *targetScale;
    mapObject->animationPosition = mapObject->targetScale;
    mapObject->effectTimer = 0.0f;
    mapObject->effectScale = 1.0f / durationFrames;
    mapObject->effectFlags = easingMode;
    mapObject->isAttached = 1;
}

float EvaluateSplineWeight(float curvePosition)
{
    float frameProgress;
    float secondaryFrameProgress;

    if (curvePosition < 0.0f) {
        curvePosition = -curvePosition;
    }
    if (curvePosition < 1.0f) {
        frameProgress = (4.0f + ((curvePosition * (3.0f * curvePosition * curvePosition)) - (6.0f * curvePosition * curvePosition))) / 6.0f;
    }
    else if (curvePosition < 2.0f) {
        secondaryFrameProgress = curvePosition - 2.0f;
        frameProgress = (secondaryFrameProgress * (-secondaryFrameProgress * secondaryFrameProgress)) / 6.0f;
    }
    else {
        frameProgress = 0.0f;
    }
    return frameProgress;
}

void EvaluateSplinePosition(float *controlPoint0, float *controlPoint1, float *controlPoint2, float *controlPoint3, float curvePosition)
{
    float secondaryFrameProgress;
    float secondaryFrameProgress;
    float frameProgress;

    s32 loopIndex;
    s32 result;
    s32 stateValue;

    stateValue = 0xB;
    secondaryFrameProgress = 0.0f;
    secondaryFrameProgress = 0.0f;
    for (loopIndex = -2; loopIndex <= stateValue + 2; loopIndex++) {
        result = loopIndex;
        if (loopIndex < 0) {
            result = 0;
        }
        if (loopIndex > stateValue) {
            result = stateValue;
        }
        frameProgress = EvaluateSplineWeight(curvePosition - loopIndex);
        secondaryFrameProgress += frameProgress * controlPoint0[result];
        secondaryFrameProgress += frameProgress * controlPoint1[result];
    }
    *controlPoint2 = secondaryFrameProgress;
    *controlPoint3 = secondaryFrameProgress;
}

void RunMapEventIntro(void)
{
    Vec secondaryTransformVector;
    Vec secondaryTransformVector;
    Vec transformVector;

    M420DllMapObject *mapObject;
    s32 mapObjectIndex;
    s32 result;

    result = 0;
    mapObject = &mapObjects[0];
    secondaryTransformVector.x = 0.0f;
    secondaryTransformVector.y = 1000.0f;
    secondaryTransformVector.z = 0.0f;

    secondaryTransformVector.x = 0.0f;
    secondaryTransformVector.y = 0.0f;
    secondaryTransformVector.z = 0.0f;

    transformVector.x = 1.0f;
    transformVector.y = 1.0f;
    transformVector.z = 1.0f;

    AnimateMapObjectPosition(mapObject, &secondaryTransformVector, 1, 0);
    AnimateMapObjectRotation(mapObject, &secondaryTransformVector, 1, 0);
    result = HuAudFXEmiterPlay(0x5EF, &secondaryTransformVector);
    secondaryTransformVector.y = 750.0f;
    AnimateMapObjectPosition(mapObject, &secondaryTransformVector, 80, 1);
    HuPrcSleep(0x50);
    {
        float splineParameter;
        float splineControlPointsX[12] = {
            0.0f,
            50.0f,
            300.0f,
            350.0f,
            300.0f,
            0.0f,
            -300.0f,
            -350.0f,
            -350.0f,
            -300.0f,
            -100.0f,
            0.0f,
        };
        float splineControlPointsY[12] = {
            750.0f,
            700.0f,
            600.0f,
            500.0f,
            400.0f,
            300.0f,
            350.0f,
            400.0f,
            450.0f,
            500.0f,
            600.0f,
            900.0f,
        };
        float splineStep = 0.050000004f;
        for (splineParameter = -1.0f; splineParameter < 12.0f; splineParameter += splineStep) {
            EvaluateSplinePosition(splineControlPointsX, splineControlPointsY, &secondaryTransformVector.x, &secondaryTransformVector.y, splineParameter);
            AnimateMapObjectPosition(mapObject, &secondaryTransformVector, 1, 0);
            HuAudFXEmiterUpDate(result, &secondaryTransformVector);
            HuPrcVSleep();
        }
    }
    for (mapObjectIndex = 0; mapObjectIndex < 0x1E; mapObjectIndex++) {
        Hu3DModelTPLvlSet(mapModelIds[1], 1.0f - (0.033333335f * mapObjectIndex));
        Hu3DModelTPLvlSet(mapModelIds[0xA], 1.0f - (0.033333335f * mapObjectIndex));
        HuAudFXEmiterUpDate(result, &secondaryTransformVector);
        HuPrcSleep(1);
    }
    HuAudFXStop(result);
    secondaryTransformVector.x = 1000.0f;
    secondaryTransformVector.y = 0.0f;
    secondaryTransformVector.z = 0.0f;
    AnimateMapObjectPosition(mapObject, &secondaryTransformVector, 1, 0);
    HuPrcSleep(1);
    Hu3DModelTPLvlSet(mapModelIds[1], 1.0f);
    Hu3DModelTPLvlSet(mapModelIds[0xA], 1.0f);
    *mapEventProcesses = NULL;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunMapEventMiddle(void)
{
    float secondaryInterpolationFactor;
    float secondaryInterpolationFactor;
    float secondaryInterpolationFactor;
    float secondaryInterpolationFactor;
    float secondaryInterpolationFactor;
    float interpolationFactor;

    M420DllMapObject *mapObject;
    s32 tertiaryResult;
    s32 secondaryResult;
    s32 result;
    M420DllMapEffect *mapEffect;
    s32 secondaryStateValue;
    s32 stateValue;

    mapObject = &mapObjects[0];
    {
        Vec transformVector = { 0.0f, 0.0f, 0.0f };
        secondaryInterpolationFactor = 800.0f;
        secondaryInterpolationFactor = 10.0f;
        secondaryResult = 0;
        result = 0;
        tertiaryResult = 0;
        mapObject->mapOffsetY.x = 0.0f;
        mapObject->mapOffsetY.y = 0.0f;
        mapObject->mapOffsetY.z = 0.0f;
        secondaryInterpolationFactor = 0.0f;
        interpolationFactor = 0.005f;
        mapEffect = &mapEffects[1];
        secondaryStateValue = mapEffect->mapFlags;
        stateValue = secondaryStateValue;
        if (stateValue != 0) {
            secondaryInterpolationFactor = 220.0f;
        }
        else {
            secondaryInterpolationFactor = -220.0f;
        }

        for (secondaryInterpolationFactor = secondaryInterpolationFactor; secondaryInterpolationFactor < 1.0f; secondaryInterpolationFactor += interpolationFactor) {
            transformVector.x = (secondaryInterpolationFactor * cosd((-180.0f * secondaryInterpolationFactor))) - (secondaryInterpolationFactor * sind((-180.0f * secondaryInterpolationFactor)));
            transformVector.y = 1200.0 + ((secondaryInterpolationFactor * cosd((-180.0f * secondaryInterpolationFactor))) + (secondaryInterpolationFactor * sind((-180.0f * secondaryInterpolationFactor))));
            transformVector.z = 0.0f;
            if (secondaryInterpolationFactor <= 0.0f) {
                tertiaryResult = HuAudFXEmiterPlay(0x5EF, &transformVector);
            }
            if ((transformVector.x < (200.0f + secondaryInterpolationFactor)) && (result == 0)) {
                Hu3DModelHookReset(mapModelIds[1]);
                Hu3DModelAttrSet(mapModelIds[0xA], HU3D_ATTR_DISPOFF);
                Hu3DModelHookSet(mapModelIds[1], "itemhook_sao", mapModelIds[0xC]);
                Hu3DModelAttrReset(mapModelIds[0xC], HU3D_ATTR_DISPOFF);
                Hu3DMotionTimeSet(mapModelIds[0xC], 0.0f);
                result = 1;
            }
            if ((transformVector.x < secondaryInterpolationFactor) && (secondaryResult == 0)) {
                transformVector.x = secondaryInterpolationFactor;
                if (mapEventProcesses[3] == NULL) {
                    mapEventProcesses[3] = HuPrcChildCreate(mapEventCallbacks[3], 0x3ED, 0x5000, 0, minigameObjectManager);
                }
                secondaryResult = 1;
            }
            AnimateMapObjectPosition(mapObject, &transformVector, 1, 0);
            HuAudFXEmiterUpDate(tertiaryResult, &transformVector);
            HuPrcVSleep();
        }
    }
    HuAudFXStop(tertiaryResult);
    mapEventProcesses[1] = NULL;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunMapEventFinale(void)
{
    Hu3DModelAttrReset(mapModelIds[4], HU3D_ATTR_DISPOFF);
    Hu3DMotionTimeSet(mapModelIds[4], 0.0f);
    Hu3DModelAttrReset(mapModelIds[6], HU3D_ATTR_DISPOFF);
    Hu3DMotionTimeSet(mapModelIds[6], 0.0f);
    Hu3DModelAttrReset(mapModelIds[8], HU3D_ATTR_DISPOFF);
    Hu3DMotionTimeSet(mapModelIds[8], 0.0f);
    HuPrcSleep(0x78);
    Hu3DModelAttrSet(mapModelIds[4], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[6], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[8], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(mapModelIds[5], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[5], HU3D_MOTATTR_LOOP);
    Hu3DMotionTimeSet(mapModelIds[5], 0.0f);
    Hu3DModelAttrReset(mapModelIds[7], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[7], HU3D_MOTATTR_LOOP);
    Hu3DMotionTimeSet(mapModelIds[7], 0.0f);
    Hu3DModelAttrReset(mapModelIds[9], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(mapModelIds[9], HU3D_MOTATTR_LOOP);
    Hu3DMotionTimeSet(mapModelIds[9], 0.0f);
    mapEventProcesses[2] = NULL;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunMapEventCleanup(void)
{
    Vec secondaryTransformVector;
    Vec transformVector;

    float secondaryFrameProgress;
    float frameProgress;

    M420DllMapObject *secondaryMapObject;
    M420DllMapEffect *mapEffect;
    M420DllMapObject *secondaryMapObject;
    M420DllMapObject *secondaryMapObject;
    M420DllMapObject *secondaryMapObject;
    M420DllMapObject *mapObject;
    s32 effectIndex;

    secondaryMapObject = &mapObjects[2];
    secondaryMapObject = &mapObjects[3];
    mapObject = &mapObjects[0];
    Hu3DModelHookReset(mapModelIds[1]);
    secondaryMapObject->mapEffects = mapObject->mapEffects;
    secondaryMapObject->mapEffects.y -= 100.0f;
    secondaryMapObject->mapEffects.z += 148.0f;
    UpdateMapObjectTransform(secondaryMapObject);

    secondaryTransformVector = secondaryMapObject->mapEffects;
    secondaryTransformVector.y = 80.0f;
    secondaryTransformVector.z = 170.0f;

    transformVector = secondaryMapObject->mapOffsetY;
    transformVector.x += 45.0f;

    AnimateMapObjectPosition(secondaryMapObject, &secondaryTransformVector, 30, 0);
    AnimateMapObjectRotation(secondaryMapObject, &transformVector, 30, 0);
    HuPrcSleep(0xA);
    HuAudFXPlay(0x5F5);
    effectIndex = MapRevealIsFinished(0) != 0 ? 0 : 1;
    mapEffect = &mapEffects[effectIndex];
    if (effectIndex == 0) {
        frameProgress = -220.0f;
    }
    else {
        frameProgress = 220.0f;
    }
    secondaryFrameProgress = frameProgress;

    secondaryTransformVector = mapEffect->mapLights->mapEffects;
    secondaryTransformVector.y = 40.0f + mapEffect->mapScale;
    secondaryTransformVector.x = secondaryFrameProgress;

    secondaryMapObject = mapEffect->mapLights;
    AnimateMapObjectPosition(secondaryMapObject, &secondaryTransformVector, 30, 1);

    Hu3DMotionTimeSet(mapEffect->mapLights->mapVariant, 0.0f);
    Hu3DModelAttrReset(mapEffect->mapLights->mapVariant, HU3D_ATTR_DISPOFF);
    secondaryTransformVector = mapEffect->mapTextures->mapEffects;
    secondaryTransformVector.y = -50.0f + mapEffect->mapScale;
    secondaryTransformVector.x = secondaryFrameProgress;
    secondaryMapObject = mapEffect->mapTextures;
    AnimateMapObjectPosition(secondaryMapObject, &secondaryTransformVector, 30, 1);
    Hu3DMotionTimeSet(mapEffect->mapTextures->mapVariant, 0.0f);
    Hu3DModelAttrReset(mapEffect->mapTextures->mapVariant, HU3D_ATTR_DISPOFF);
    HuPrcSleep(5);
    mapRoundIndex = 1;
    transformVector.x = 0.0f;
    AnimateMapObjectRotation(secondaryMapObject, &transformVector, 15, 1);
    HuPrcSleep(0xF);
    secondaryMapObject->mapEffects = secondaryMapObject->mapEffects;
    Hu3DModelAttrSet(secondaryMapObject->mapVariant, HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(secondaryMapObject->mapVariant, HU3D_ATTR_DISPOFF);
    Hu3DMotionTimeSet(secondaryMapObject->mapVariant, 0.0f);
    Hu3DModelAttrReset(secondaryMapObject->mapVariant, HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrSet(secondaryMapObject->mapVariant, HU3D_MOTATTR_LOOP);
    HuPrcSleep(0x78);
    mapEventProcesses[3] = NULL;
    HuPrcEnd();

    while (TRUE) {
        HuPrcVSleep();
    }
}

const float _pad = 0;

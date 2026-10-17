#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/flag.h"
#include "game/frand.h"
#include "game/gamework.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "string.h"

#include "REL/m453Dll.h"
#include "version.h"

typedef struct M453CameraTransitionWork {
    /* 0x00 */ u8 transitionMode;
    /* 0x04 */ Vec cameraCenter;
    /* 0x10 */ Vec cameraRotation;
    /* 0x1C */ Vec startCenter;
    /* 0x28 */ Vec startRotation;
    /* 0x34 */ Vec targetCenter;
    /* 0x40 */ Vec targetRotation;
    /* 0x4C */ Vec alternateStartCenter;
    /* 0x58 */ Vec alternateStartRotation;
    /* 0x64 */ Vec centerSplinePoints[3];
    /* 0x88 */ Vec rotationSplinePoints[3];
    /* 0xAC */ Vec alternateCenterSplinePoints[3];
    /* 0xD0 */ Vec alternateRotationSplinePoints[3];
    /* 0xF4 */ Float centerSplineValues[3];
    /* 0x100 */ Float rotationSplineValues[3];
    /* 0x10C */ Float zoomSplineValues[3];
    /* 0x118 */ Float alternateZoomSplineValues[3];
    /* 0x124 */ Float zoomSplineTimes[3];
    /* 0x130 */ Float cameraZoom;
    /* 0x134 */ Float startZoom;
    /* 0x138 */ Float targetZoom;
    /* 0x13C */ Float alternateTargetZoom;
    /* 0x140 */ Float zoomStep;
    /* 0x144 */ Float transitionDuration;
    /* 0x148 */ Float transitionTime;
} M453CameraTransitionWork; /* size 0x14C */

typedef struct M453PlayerAIState {
    s16 state;
    s16 timer;
    Float targetX;
    Float targetZ;
    char reservedPayload[0x34];
} M453PlayerAIState; /* 0x40 */

typedef struct M453PlayerWork {
    u8 playerIndex;
    u8 characterId;
    u8 teamIndex;
    u8 teamPlayerIndex;
    u8 controllerIndex;
    u8 isComputer;
    u8 difficulty;
    u8 stateFlags;
    char reservedStateBytes[2];
    u8 actionFlags;
    char reservedActionByte;
    u8 isAirborne;
    u8 wasHit;
    u8 nearbyPlayerCount;
    s16 stickX;
    s16 stickY;
    s16 buttonsDown;
    s16 buttonsHeld;
    s16 motionIndex;
    s16 previousMotionIndex;
    Float movementSpeed;
    Float movementAngle;
    Float stepAnimationTime;
    Float motionDuration;
    Vec position;
    Vec rotation;
    Float velocityX;
    Float verticalVelocity;
    Float velocityZ;
    Vec previousPosition;
    M453PlayerAIState aiState;
    u8 hasPendingAction;
    s32 isActive;
} M453PlayerWork; /* size 0xA4 */

void CameraControllerInitialize(OMOBJ *object);
void CameraIntroUpdate(OMOBJ *object);
void GameStateUpdate(OMOBJ *object);
void GameStateCleanup(OMOBJ *object);
void MapControllerInitialize(OMOBJ *object);
void MapControllerUpdate(OMOBJ *object);
void CameraTransitionInitialize(OMOBJ *object);
void CameraTransitionUpdate(OMOBJ *object);
void CameraTransitionStart(Float duration, Vec *targetCenter, Vec *targetRotation, Float targetZoom);
void CameraTransitionSequenceStart(Float duration, Vec *targetCenter, Vec *targetRotation, Float targetZoom, Vec *alternateStartCenter, Vec *alternateStartRotation, Float alternateTargetZoom);
void PlayerInitialize(OMOBJ *object);
void PlayerUpdate(OMOBJ *object);
void PlayerMovementUpdate(OMOBJ *object);
void PlayerAIUpdate(OMOBJ *object);
void PlayerPhysicsUpdate(OMOBJ *object);
s32 PlayerSeparationUpdate(void);
void PlayerEventUpdate(OMOBJ *object);
void PlayerCollisionUpdate(OMOBJ *object);
s16 ParticleCreate(s32 particleResourceId, s16 particleCount, HU3DPARTICLEHOOK particleUpdateHook);
void ParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
s32 RandomValueGet(void);
Float AngleApproach(Float currentAngle, Float targetAngle, Float approachRate);
void SplineLinearCoefficientsSet(Float *knotPositions, Float *knotValues, Float *secondDerivatives, s32 knotCount);
Float SplineLinearEvaluate(Float parameter, Float *knotPositions, Float *knotValues, Float *secondDerivatives, s32 knotCount);
void SplineCubicCoefficientsPrepare(Float *knotPositions, Float (*controlPoints)[3], Float (*tangentCoefficients)[3], s32 pointCount);
void SplineCubicEvaluate(Float parameter, Float *outputVector, Float *knotPositions, Float (*controlPoints)[3], Float (*tangentCoefficients)[3], s32 pointCount);
Float VectorLength(Vec *vector);
s32 HasActiveHumanPlayer(void);
void GameEnd(void);

s16 finalCountdown = -1;

Vec globalLightPosition = { 0.0f, 4000.0f, 4000.0f };

Vec globalLightDirection = { 0.0f, -0.7f, -0.6f };

GXColor globalLightColorTable[0xA] = {
    { 0xFF, 0xFF, 0xFF, 0xFF },
    { 0, 0, 0, 0 },
    { 0x45, 0x7A, 0, 0 },
    { 0xC4, 0xFA, 0, 0 },
    { 0, 0, 0, 0 },
    { 0x3F, 0x80, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
};

OMOBJ *cameraControllerObject;
OMOBJ *viewCleanupObject;
OMOBJ *cameraTransitionObject;
OMOBJ *mapControllerObject;
OMOBJ *playerObjects[4];
u16 gamePhase;
s16 startMessageSequenceId;
s16 activeMessageSequenceId;
s16 messageSequenceTimer;
s16 phaseTimer;
s16 playerMotionIds[0xA];
s16 playerMotionTimer;
s16 mapLightIds[2];
s32 lastScoringPlayerIndex;
s32 playerActionTimers[3];
s16 playerActionStates[3];
s16 finalScore;

void ObjectSetup(void)
{
    HUPROCESS *objectManager;
    s32 playerIndex;
    OMOBJ *playerObject;
    HU3DLIGHT *globalLight;
    s32 randomSeedAdvanceCount;
    s32 globalLightId;

    OSReport("******* M453ObjectSetup *********\n");
    Hu3DLighInit();
    globalLightId = Hu3DGLightCreateV(&globalLightPosition, &globalLightDirection, globalLightColorTable);
    Hu3DGLightInfinitytSet(globalLightId);
    globalLight = Hu3DGlobalLight;
    globalLight->type |= 0x8000;
    objectManager = omInitObjMan(0x32, 0x2000);
    randomSeedAdvanceCount = frand() & 0x1F;
    for (playerIndex = 0; playerIndex < randomSeedAdvanceCount; playerIndex++) {
        RandomValueGet();
    }
    CRot.x = -23.5f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    Center.x = 0.0f;
    Center.y = 320.0f;
    Center.z = 147.0f;
    CZoom = 1720.0f;
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 45.0f, 400.0f, 10000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    viewCleanupObject = omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutView);
    cameraControllerObject = omAddObjEx(objectManager, 0x10, 8, 0, -1, CameraControllerInitialize);
    mapControllerObject = omAddObjEx(objectManager, 0x20, 0x10, 0, -1, MapControllerInitialize);
    cameraTransitionObject = omAddObjEx(objectManager, 0x50, 0x10, 0, -1, CameraTransitionInitialize);
    omAddObjEx(objectManager, 0x60, 0, 0, -1, PlayerEventUpdate);
    MapInitialize(objectManager);
    MapSpaceSelect(3);
    omMakeGroupEx(objectManager, 0, 4);
    omGetGroupMemberListEx(objectManager, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObject = omAddObjEx(objectManager, 0x30, 2, 8, 0, PlayerInitialize);
        playerObjects[playerIndex] = playerObject;
        playerObject->work[0] = playerIndex;
        playerObject->work[1] = playerIndex;
    }
    omGameSysInit(objectManager);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    HuAudSndGrpSet(0x1C);
    playerActionStates[0] = 1;
    if (GWMGRecordGet(9) == 0) {
        GWMGRecordSet(9, 0);
    }
    ScoreDisplayInitialize(objectManager, GWMGRecordGet(9));
    ScoreDisplaySetMode(0);
}

void CameraControllerInitialize(OMOBJ *object)
{
    gamePhase = 0;
    object->objFunc = CameraIntroUpdate;
}

void CameraIntroUpdate(OMOBJ *object)
{
    Vec firstCameraCenter;
    Vec firstCameraRotation;
    Vec secondCameraCenter;
    Vec secondCameraRotation;

    Float firstCameraZoom;
    Float secondCameraZoom;

    switch (gamePhase) {
        case 0:
            firstCameraCenter.x = 0.0f;
            firstCameraCenter.y = 0.0f;
            firstCameraCenter.z = -50.0f;
            firstCameraRotation.x = -90.0f;
            firstCameraRotation.y = 90.0f;
            firstCameraRotation.z = 0.0f;
            firstCameraZoom = 2200.0f;
            CameraTransitionStart(1.0f, &firstCameraCenter, &firstCameraRotation, firstCameraZoom);
            phaseTimer = REFRESH_RATE;
            gamePhase += 1;
            break;
        case 1:
            if (--phaseTimer == 0) {
                gamePhase += 1;
            }
            break;
        case 2:
            if (phaseTimer == 0) {
                MapUpdate();
                phaseTimer = REFRESH_RATE * 1.5;
                firstCameraCenter.x = 200.0f;
                firstCameraCenter.y = 100.0f;
                firstCameraCenter.z = -50.0f;
                firstCameraRotation.x = -50.0f;
                firstCameraRotation.y = 90.0f;
                firstCameraRotation.z = 0.0f;
                firstCameraZoom = 2000.0f;
                secondCameraCenter.x = 0.0f;
                secondCameraCenter.y = 320.0f;
                secondCameraCenter.z = 147.0f;
                secondCameraRotation.x = -23.5f;
                secondCameraRotation.y = 0.0f;
                secondCameraRotation.z = 0.0f;
                secondCameraZoom = 1720.0f;
                CameraTransitionSequenceStart(REFRESH_RATE_F * 3, &secondCameraCenter, &secondCameraRotation, secondCameraZoom, &firstCameraCenter, &firstCameraRotation, firstCameraZoom);
            }
            if (--phaseTimer == 0) {
                gamePhase += 1;
            }
            break;
        case 3:
            if (phaseTimer == 0) {
                phaseTimer = REFRESH_RATE * 1.5;
            }
            if (--phaseTimer == 0) {
                gamePhase += 1;
            }
            break;
        case 4:
            gamePhase = 5;
            startMessageSequenceId = activeMessageSequenceId = -1;
            object->objFunc = GameStateUpdate;
            HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M403, 19), 0x10000000));
            break;
    }
    if ((omSysExitReq != 0) && (WipeStatGet() == 0)) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = GameStateCleanup;
    }
}

s32 countdownAudioSequenceIds[9] = {
    -1,
    0x610000,
    0x610001,
    0x610002,
    0x610003,
    0x610004,
    0x610005,
    0x610006,
    0x610007,
};

void GameStateUpdate(OMOBJ *object)
{
    Vec cameraCenter;
    Vec cameraRotation;
    s16 eligibleCharacterIds[4];
    Float cameraZoom;

    s32 playerIndex;
    s32 readyPlayerCount;
    s32 eligiblePlayerCount;

    switch (gamePhase) {
        case 5:
            if (activeMessageSequenceId == -1) {
                messageSequenceTimer = REFRESH_RATE * 30;
                activeMessageSequenceId = MGSeqCreate(3, 0);
                MGSeqPosSet(activeMessageSequenceId, 320.0f, 240.0f);
            }
            else if (MGSeqStatGet(activeMessageSequenceId) == 0) {
                activeMessageSequenceId = -1;
                gamePhase += 1;
                phaseTimer = 0;
                finalCountdown = 4;
            }
            if ((countdownAudioSequenceIds[0] == -1) && ((MGSeqStatGet(activeMessageSequenceId) & 0x10) != 0)) {
                countdownAudioSequenceIds[0] = HuAudSeqPlay(0x48);
            }
            break;
        case 6:
            readyPlayerCount = 0;
            if ((finalCountdown == 0) && (MapTransitionActiveGet() == 0)) {
                readyPlayerCount++;
            }
            if (readyPlayerCount != 0) {
                gamePhase += 1;
            }
            break;
        case 7:
            if (activeMessageSequenceId == -1) {
                activeMessageSequenceId = MGSeqCreate(3, 1);
                MGSeqPosSet(activeMessageSequenceId, 320.0f, 240.0f);
                MGSeqKill(startMessageSequenceId);
                HuAudSeqFadeOut(countdownAudioSequenceIds[0], 0x64);
            }
            else if (MGSeqStatGet(activeMessageSequenceId) == 0) {
                activeMessageSequenceId = -1;
                if (playerActionStates[0] == 0) {
                    if (finalCountdown != 0) {
                        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                            if (((M453PlayerWork *)playerObjects[playerIndex]->data)->stateFlags != 0) {
                                break;
                            }
                        }
                        cameraCenter.x = playerObjects[playerIndex]->trans.x;
                        cameraCenter.y = 100.0f;
                        cameraCenter.z = playerObjects[playerIndex]->trans.z;
                        cameraRotation.x = -18.0f;
                        cameraRotation.y = 0.0f;
                        cameraRotation.z = 0.0f;
                        cameraZoom = 600.0f;
                        CameraTransitionStart(REFRESH_RATE_F * 0.75f, &cameraCenter, &cameraRotation, cameraZoom);
                    }
                    gamePhase = 9;
                    phaseTimer = REFRESH_RATE * 1.5;
                }
                else {
                    if ((lastScoringPlayerIndex != 0) && (finalScore >= 0xF)) {
                        GWGameStat.present[0x31] = 1;
                    }
                    if ((lastScoringPlayerIndex != 0) && ((s32)GWMGRecordGet(9) < finalScore)) {
                        GWMGRecordSet(9, finalScore);
                        gamePhase = 8;
                        ScoreDisplaySetMode(1);
                    }
                    else {
                        activeMessageSequenceId = -1;
                        gamePhase = 9;
                        phaseTimer = REFRESH_RATE * 1.5;
                    }
                    GameEnd();
                }
            }
            break;
        case 8:
            if (activeMessageSequenceId == -1) {
                activeMessageSequenceId = MGSeqCreate(0xE, finalScore);
            }
            else if (MGSeqStatGet(activeMessageSequenceId) == 0) {
                activeMessageSequenceId = -1;
                phaseTimer = REFRESH_RATE * 1.5;
                gamePhase = 9;
            }
            break;
        case 9:
            if (--phaseTimer == 0) {
                gamePhase += 1;
            }
            break;
        case 10:
            if (playerActionStates[0] == 0) {
                if (activeMessageSequenceId == -1) {
                    if (finalCountdown == 0) {
                        activeMessageSequenceId = MGSeqCreate(3, 2);
                        HuAudSStreamPlay(4);
                    }
                    else {
                        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                            eligibleCharacterIds[playerIndex] = -1;
                        }
                        for (playerIndex = 0, eligiblePlayerCount = 0; playerIndex < 4; playerIndex++) {
                            if (((M453PlayerWork *)playerObjects[playerIndex]->data)->stateFlags != 0) {
                                eligibleCharacterIds[eligiblePlayerCount++] = GWPlayerCfg[playerIndex].character;
                            }
                        }
                        activeMessageSequenceId = MGSeqCreate(5, 3, eligibleCharacterIds[0], eligibleCharacterIds[1], eligibleCharacterIds[2], eligibleCharacterIds[3]);
                        HuAudSStreamPlay(1);
                    }
                }
                activeMessageSequenceId = -1;
                phaseTimer = REFRESH_RATE * 3.5;
                gamePhase += 1;
            }
            else {
                if (finalCountdown != 0) {
                    HuAudSStreamPlay(1);
                }
                else {
                    HuAudSStreamPlay(4);
                }
                activeMessageSequenceId = -1;
                phaseTimer = REFRESH_RATE * 3.5;
                gamePhase += 1;
            }
            break;
        case 11:
            if (--phaseTimer == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = GameStateCleanup;
            }
            break;
    }
    if ((omSysExitReq != 0) && (WipeStatGet() == 0)) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = GameStateCleanup;
    }
}

void GameStateCleanup(OMOBJ *object)
{
    if (WipeStatGet() == 0) {
        MapCleanup();
        CharModelKill(-1);
        MGSeqKill(startMessageSequenceId);
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void MapControllerInitialize(OMOBJ *object)
{
    Hu3DLightAllKill();
    mapLightIds[0] = Hu3DGLightCreateV(&globalLightPosition, &globalLightDirection, globalLightColorTable);
    Hu3DGLightStaticSet(mapLightIds[0], 1);
    Hu3DGLightInfinitytSet(mapLightIds[0]);
    mapLightIds[1] = Hu3DGLightCreate(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0, 0, 0);
    Hu3DGLightStaticSet(mapLightIds[1], 1);
    Hu3DGLightPointSet(mapLightIds[1], 0.0f, 0.0f, 3);
    Hu3DGLightColorSet(mapLightIds[0], 0, 0, 0, 0xFF);
    Hu3DGLightPosSet(mapLightIds[1], -1470.0f, 800.0f, -2000.0f, 0.0f, -1.0f, 0.0f);
    Hu3DGLightColorSet(mapLightIds[1], 0, 0, 0, 0xFF);
    Hu3DGLightPointSet(mapLightIds[1], 1200.0f, 0.6f, 3);
    object->objFunc = MapControllerUpdate;
}

void MapControllerUpdate(OMOBJ *object)
{
    s32 countdownStepIndex;

    s8 spaceHighlightByStep[9] = { 0x01, 0x00, 0x02, 0x01, 0x00, 0x03, 0x02, 0x01, 0x00 };
    s8 spaceSelectionByStep[9] = { 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03 };

    switch (gamePhase) {
        case 0:
            break;
        case 1:
            Hu3DGLightColorSet(mapLightIds[0], 0xF0, 0xF0, 0xF0, 0xFF);
            break;
        case 3:
            Hu3DGLightColorSet(mapLightIds[1], 0xFF, 0xE0, 0xD0, 0xFF);
            break;
        case 5:
            object->work[0] = 0x1E;
            object->work[1] = 0x11;
            playerActionTimers[0] = 1;
            break;
        case 6:
            if (MapTransitionActiveGet() == 0) {
                if (playerActionTimers[0] == 0) {
                    playerActionTimers[0] = REFRESH_RATE / 2;
                    finalScore++;
                    lastScoringPlayerIndex = HasActiveHumanPlayer();
                    if (finalScore == 0x63) {
                        gamePhase = 7;
                        return;
                    }
                }
                playerActionTimers[0]--;
                if (playerActionTimers[0] == 0) {
                    countdownStepIndex = (object->work[1] - 2) >> 1;
                    if (countdownStepIndex < 0) {
                        countdownStepIndex = 0;
                    }
                    MapSpaceHighlightSet(spaceHighlightByStep[countdownStepIndex]);
                    MapSpaceSelect(spaceSelectionByStep[countdownStepIndex]);
#if VERSION_PAL
                    MapTransitionRotationSpeedSet(0.02f * (object->work[0] * 0.6f));
#else
                    MapTransitionRotationSpeedSet(0.01f * object->work[0]);
#endif
                    object->work[0] += 5;
                    if (object->work[1] > 2) {
                        object->work[1] -= 1;
                    }
                }
            }
            break;
    }
}

void CameraTransitionInitialize(OMOBJ *object)
{
    M453CameraTransitionWork *cameraWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M453CameraTransitionWork), HU_MEMNUM_OVL);
    object->data = cameraWork;
    cameraWork->transitionTime = cameraWork->transitionDuration = 0.0f;
    cameraWork->zoomStep = 1.0f;
    cameraWork->cameraRotation.x = cameraWork->startRotation.x = -23.5f;
    cameraWork->cameraRotation.y = cameraWork->startRotation.y = 0.0f;
    cameraWork->cameraRotation.z = cameraWork->startRotation.z = 0.0f;
    cameraWork->cameraCenter.x = cameraWork->startCenter.x = 0.0f;
    cameraWork->cameraCenter.y = cameraWork->startCenter.y = 320.0f;
    cameraWork->cameraCenter.z = cameraWork->startCenter.z = 147.0f;
    cameraWork->cameraZoom = cameraWork->startZoom = 1720.0f;
    object->objFunc = CameraTransitionUpdate;
}

void CameraTransitionUpdate(OMOBJ *object)
{
    Float transitionProgress;
    M453CameraTransitionWork *cameraWork = object->data;

    if (cameraWork->transitionTime > 0.0f) {
        cameraWork->transitionTime = cameraWork->transitionTime - cameraWork->zoomStep;
        transitionProgress = (cameraWork->transitionDuration - cameraWork->transitionTime) / cameraWork->transitionDuration;
        transitionProgress = sind((90.0f * transitionProgress));
        if (cameraWork->transitionMode == 0) {
            cameraWork->cameraCenter.x = cameraWork->startCenter.x + (transitionProgress * (cameraWork->targetCenter.x - cameraWork->startCenter.x));
            cameraWork->cameraCenter.y = cameraWork->startCenter.y + (transitionProgress * (cameraWork->targetCenter.y - cameraWork->startCenter.y));
            cameraWork->cameraCenter.z = cameraWork->startCenter.z + (transitionProgress * (cameraWork->targetCenter.z - cameraWork->startCenter.z));
            cameraWork->cameraRotation.x = cameraWork->startRotation.x + (transitionProgress * (cameraWork->targetRotation.x - cameraWork->startRotation.x));
            cameraWork->cameraRotation.y = cameraWork->startRotation.y + (transitionProgress * (cameraWork->targetRotation.y - cameraWork->startRotation.y));
            cameraWork->cameraRotation.z = cameraWork->startRotation.z + (transitionProgress * (cameraWork->targetRotation.z - cameraWork->startRotation.z));
            cameraWork->cameraZoom = cameraWork->startZoom + (transitionProgress * (cameraWork->targetZoom - cameraWork->startZoom));
        }
        else {
            SplineCubicEvaluate(2.0f * transitionProgress, (Float *)&cameraWork->cameraCenter, cameraWork->centerSplineValues, (Float(*)[3])cameraWork->centerSplinePoints, (Float(*)[3])cameraWork->rotationSplinePoints, 3);
            SplineCubicEvaluate(2.0f * transitionProgress, (Float *)&cameraWork->cameraRotation, cameraWork->rotationSplineValues, (Float(*)[3])cameraWork->alternateCenterSplinePoints, (Float(*)[3])cameraWork->alternateRotationSplinePoints, 3);
            cameraWork->cameraZoom = SplineLinearEvaluate(2.0f * transitionProgress, cameraWork->zoomSplineTimes, cameraWork->zoomSplineValues, cameraWork->alternateZoomSplineValues, 3);
        }
    }
    CRot.x = cameraWork->cameraRotation.x;
    CRot.y = cameraWork->cameraRotation.y;
    CRot.z = cameraWork->cameraRotation.z;
    Center.x = cameraWork->cameraCenter.x;
    Center.y = cameraWork->cameraCenter.y;
    Center.z = cameraWork->cameraCenter.z;
    CZoom = cameraWork->cameraZoom;
}

void CameraTransitionStart(Float duration, Vec *targetCenter, Vec *targetRotation, Float targetZoom)
{
    M453CameraTransitionWork *cameraWork;

    cameraWork = cameraTransitionObject->data;
    cameraWork->transitionMode = 0;
    cameraWork->transitionDuration = duration;
    cameraWork->transitionTime = duration;
    cameraWork->startCenter = cameraWork->cameraCenter;
    cameraWork->targetCenter.x = targetCenter->x;
    cameraWork->targetCenter.y = targetCenter->y;
    cameraWork->targetCenter.z = targetCenter->z;
    cameraWork->startRotation = cameraWork->cameraRotation;
    cameraWork->targetRotation.x = targetRotation->x;
    cameraWork->targetRotation.y = targetRotation->y;
    cameraWork->targetRotation.z = targetRotation->z;
    cameraWork->startZoom = cameraWork->cameraZoom;
    cameraWork->targetZoom = targetZoom;
}

void CameraTransitionSequenceStart(Float duration, Vec *targetCenter, Vec *targetRotation, Float targetZoom, Vec *alternateStartCenter, Vec *alternateStartRotation, Float alternateTargetZoom)
{
    Float firstZoomSegmentDuration;
    Float secondZoomSegmentDuration;
    M453CameraTransitionWork *cameraWork = cameraTransitionObject->data;

    cameraWork->transitionMode = 1;
    cameraWork->transitionTime = cameraWork->transitionDuration = duration;
    cameraWork->startCenter = cameraWork->cameraCenter;
    cameraWork->targetCenter.x = targetCenter->x;
    cameraWork->targetCenter.y = targetCenter->y;
    cameraWork->targetCenter.z = targetCenter->z;
    cameraWork->alternateStartCenter.x = alternateStartCenter->x;
    cameraWork->alternateStartCenter.y = alternateStartCenter->y;
    cameraWork->alternateStartCenter.z = alternateStartCenter->z;
    cameraWork->startRotation = cameraWork->cameraRotation;
    cameraWork->targetRotation.x = targetRotation->x;
    cameraWork->targetRotation.y = targetRotation->y;
    cameraWork->targetRotation.z = targetRotation->z;
    cameraWork->alternateStartRotation.x = alternateStartRotation->x;
    cameraWork->alternateStartRotation.y = alternateStartRotation->y;
    cameraWork->alternateStartRotation.z = alternateStartRotation->z;
    cameraWork->startZoom = cameraWork->cameraZoom;
    cameraWork->targetZoom = targetZoom;
    cameraWork->alternateTargetZoom = alternateTargetZoom;
    cameraWork->centerSplinePoints[0] = cameraWork->startCenter;
    cameraWork->centerSplinePoints[1] = cameraWork->alternateStartCenter;
    cameraWork->centerSplinePoints[2] = cameraWork->targetCenter;
    cameraWork->alternateCenterSplinePoints[0] = cameraWork->startRotation;
    cameraWork->alternateCenterSplinePoints[1] = cameraWork->alternateStartRotation;
    cameraWork->alternateCenterSplinePoints[2] = cameraWork->targetRotation;
    cameraWork->zoomSplineValues[0] = cameraWork->startZoom;
    cameraWork->zoomSplineValues[1] = cameraWork->alternateTargetZoom;
    cameraWork->zoomSplineValues[2] = cameraWork->targetZoom;
    cameraWork->zoomSplineTimes[0] = 0.0f;
    if ((cameraWork->zoomSplineValues[1] - cameraWork->zoomSplineValues[0]) < 0.0f) {
        firstZoomSegmentDuration = -(cameraWork->zoomSplineValues[1] - cameraWork->zoomSplineValues[0]);
    }
    else {
        firstZoomSegmentDuration = cameraWork->zoomSplineValues[1] - cameraWork->zoomSplineValues[0];
    }
    cameraWork->zoomSplineTimes[1] = firstZoomSegmentDuration;
    if ((cameraWork->zoomSplineValues[2] - cameraWork->zoomSplineValues[1]) < 0.0f) {
        secondZoomSegmentDuration = -(cameraWork->zoomSplineValues[2] - cameraWork->zoomSplineValues[1]);
    }
    else {
        secondZoomSegmentDuration = cameraWork->zoomSplineValues[2] - cameraWork->zoomSplineValues[1];
    }
    cameraWork->zoomSplineTimes[2] = cameraWork->zoomSplineTimes[1] + secondZoomSegmentDuration;
    SplineCubicCoefficientsPrepare(cameraWork->centerSplineValues, (Float(*)[3])cameraWork->centerSplinePoints, (Float(*)[3])cameraWork->rotationSplinePoints, 3);
    SplineCubicCoefficientsPrepare(cameraWork->rotationSplineValues, (Float(*)[3])cameraWork->alternateCenterSplinePoints, (Float(*)[3])cameraWork->alternateRotationSplinePoints, 3);
    SplineLinearCoefficientsSet(cameraWork->zoomSplineTimes, cameraWork->zoomSplineValues, cameraWork->alternateZoomSplineValues, 3);
}

void CameraZoomSet(Float zoomStep)
{
    M453CameraTransitionWork *cameraWork = cameraTransitionObject->data;
    cameraWork->zoomStep = zoomStep;
}

s32 characterMotionIds[8] = { 0, 2, 3, 8, 0xA, 9, 0x17, 0x18 };

Vec playerStartingPositions[4] = {
    { -300.0f, 2200.0f, 0.0f },
    { -100.0f, 2200.0f, 0.0f },
    { 100.0f, 2200.0f, 0.0f },
    { 300.0f, 2200.0f, 0.0f },
};

void PlayerInitialize(OMOBJ *object)
{
    M453PlayerWork *playerWork;
    s32 motionIndex;
    s32 modelId;
    u32 startingPositionIndex;

    playerWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M453PlayerWork), HU_MEMNUM_OVL);
    object->data = playerWork;
    playerWork->playerIndex = object->work[0];
    playerWork->characterId = GWPlayerCfg[playerWork->playerIndex].character;
    playerWork->teamIndex = playerWork->playerIndex >> 1;
    playerWork->teamPlayerIndex = playerWork->playerIndex & 1;
    playerWork->controllerIndex = GWPlayerCfg[playerWork->playerIndex].pad_idx;
    playerWork->isComputer = GWPlayerCfg[playerWork->playerIndex].iscom;
    playerWork->difficulty = GWPlayerCfg[playerWork->playerIndex].diff;
    playerWork->motionIndex = playerWork->previousMotionIndex = 0;
    playerWork->stateFlags = 1;
    playerWork->isAirborne = 1;
    playerWork->wasHit = 0;
    playerWork->nearbyPlayerCount = 0;
    playerWork->actionFlags = 0;
    playerWork->hasPendingAction = 0;
    startingPositionIndex = object->work[1];
    playerWork->position.x = playerStartingPositions[startingPositionIndex].x;
    playerWork->position.y = playerStartingPositions[startingPositionIndex].y + (100.0f * ((RandomValueGet() - 0x8000) / 32768.0f));
    playerWork->position.z = playerStartingPositions[startingPositionIndex].z;
    playerWork->velocityX = playerWork->verticalVelocity = playerWork->velocityZ = 0.0f;
    playerWork->rotation.x = playerWork->rotation.y = playerWork->rotation.z = 0.0f;
    playerWork->movementAngle = playerWork->rotation.y;
    playerWork->stepAnimationTime = 0.0f;
    playerWork->movementSpeed = 0.0f;
    playerWork->motionDuration = 1.0f;
    memset(&playerWork->aiState, 0, sizeof(M453PlayerAIState));
    playerWork->isActive = 1;
    if ((playerActionStates[0] != 0) && (playerWork->isComputer == 0)) {
        playerMotionIds[playerMotionTimer++] = playerWork->playerIndex;
    }
    modelId = CharModelCreate(playerWork->characterId, 4);
    object->mdlId[0] = modelId;
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelId);
    for (motionIndex = 0; motionIndex < 8; motionIndex++) {
        object->mtnId[motionIndex] = CharMotionCreate(playerWork->characterId, characterMotionIds[motionIndex]);
    }
    CharMotionDataClose(playerWork->characterId);
    CharMotionSet(playerWork->characterId, object->mtnId[playerWork->motionIndex]);
    omSetTra(object, playerWork->position.x, playerWork->position.y, playerWork->position.z);
    Hu3DModelPosSet(modelId, playerWork->position.x, playerWork->position.y, playerWork->position.z);
    Hu3DModelRotSet(modelId, 0.0f, 30.0f, 0.0f);
    CharModelStepFxSet(playerWork->characterId, 1);
    CharMotionVoiceOnSet(playerWork->characterId, object->mtnId[3], 0);
    object->objFunc = PlayerUpdate;
}

void PlayerUpdate(OMOBJ *object)
{
    s32 modelId = *object->mdlId;
    M453PlayerWork *playerWork = object->data;
    playerWork->stickX = playerWork->stickY = 0;
    playerWork->buttonsDown = playerWork->buttonsHeld = 0;
    switch (gamePhase) {
        case 3:
            break;
        case 2:
            if (!(phaseTimer < REFRESH_RATE_F)) {
                case 1:
                    playerWork->motionIndex = 3;
                    playerWork->verticalVelocity = 0.0f;
                    playerWork->isAirborne = 0;
            }
            break;
        case 5:
            object->objFunc = PlayerMovementUpdate;
            break;
    }
    if (playerWork->position.y < 10.0f) {
        omVibrate(playerWork->playerIndex, 0xC, 6, 6);
        playerWork->position.y = 10.0f;
        playerWork->isAirborne = 1;
    }
    PlayerPhysicsUpdate(object);
}

void PlayerMovementUpdate(OMOBJ *object)
{
    M453PlayerWork *playerWork;
    s32 modelId;
    s32 particleModelId;

    modelId = object->mdlId[0];
    playerWork = object->data;
    if (playerWork->stateFlags != 0) {
        if ((gamePhase == 6) && ((playerWork->stateFlags & 2) == 0)) {
            if (playerWork->isComputer != 0) {
                PlayerAIUpdate(object);
            }
            else {
                playerWork->stickX = HuPadStkX[playerWork->controllerIndex];
                playerWork->stickY = HuPadStkY[playerWork->controllerIndex];
                playerWork->buttonsDown = HuPadBtnDown[playerWork->controllerIndex];
                playerWork->buttonsHeld = HuPadBtn[playerWork->controllerIndex];
            }
        }
        else {
            playerWork->stickX = playerWork->stickY = 0;
            playerWork->buttonsDown = playerWork->buttonsHeld = 0;
        }
        switch (gamePhase) {
            case 5:
            case 6:
            case 8:
                break;
            case 7:
            case 9:
                object->scale.y += 0.2f * (1.0f - object->scale.y);
                playerWork->rotation.y = AngleApproach(playerWork->rotation.y, 0.0f, 0.9f);
                break;
            case 10:
                if (playerWork->motionIndex != 6) {
                    playerWork->motionIndex = 6;
                }
                break;
        }
        if ((playerWork->stateFlags & 2) != 0) {
            playerWork->stickX = playerWork->stickY = 0;
            playerWork->buttonsDown = playerWork->buttonsHeld = 0;
            if (object->scale.y < 0.2f) {
                omVibrate(playerWork->playerIndex, 0xC, 4, 2);
                HuAudFXPlay(0x523);
                playerWork->stateFlags = 0;
                Hu3DModelShadowReset(modelId);
                Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
                finalCountdown--;
                particleModelId = ParticleCreate(0x220013, 0x64, ParticleUpdate);
                Hu3DModelPosSet(particleModelId, playerWork->position.x, playerWork->position.y, playerWork->position.z);
                Hu3DModelLayerSet(particleModelId, 2);
            }
        }
        if (MapTransitionActiveGet() != 0) {
            playerWork->isActive = 0;
        }
        else if (((playerWork->stateFlags & 2) == 0) && (playerWork->isActive == 0)) {
            omVibrate(playerWork->playerIndex, 0xC, 6, 6);
            playerWork->isActive = 1;
        }
        PlayerPhysicsUpdate(object);
    }
}

void PlayerAIUpdate(OMOBJ *object)
{
    Vec nearbySpacePositions[6];
    Float nearbySpaceDistances[6];
    Vec swapPosition;
    Vec targetDirection;

    Float targetDistance;
    Float targetAngle;
    Float moveSpeed;
    Float turnRate;

    M453PlayerWork *playerWork;
    s32 candidateIndex;
    s32 neighborIndex;
    s16 difficulty;
    s16 stickX;
    s16 stickY;
    s32 selectionWeight;
    s32 nearbySpaceCount;
    s32 randomSelection;
    s32 otherHumanPlayerNearby;
    s16 buttonsDown;
    s16 buttonsHeld;

    otherHumanPlayerNearby = 0;
    candidateIndex = 0;
    neighborIndex = 0;
    for (; candidateIndex < 4; candidateIndex++) {
        playerWork = playerObjects[candidateIndex]->data;
        if ((playerWork->isComputer == 0) && (playerWork->stateFlags != 0)) {
            break;
        }
    }
    playerWork = object->data;
    if ((candidateIndex >= 4) && (playerWork->hasPendingAction == 0)) {
        otherHumanPlayerNearby = 1;
    }
    difficulty = playerWork->difficulty;
    stickX = stickY = buttonsDown = buttonsHeld = 0;
    switch (playerWork->aiState.state) {
        case 0:
            if (MapTransitionActiveGet() != 0) {
                playerWork->aiState.state = 1;
            }
            break;
        case 1:
            playerWork->aiState.state = 2;
            playerWork->aiState.timer = (s32)(REFRESH_RATE_F * (1.2f - (0.2f * difficulty)) * (RandomValueGet() / 65536.0f));
            playerWork->aiState.targetX = 500.0f * ((RandomValueGet() - 0x8000) / 32768.0f);
            playerWork->aiState.targetZ = 200.0f * ((RandomValueGet() - 0x8000) / 32768.0f);
            stickX = playerWork->stickX;
            stickY = playerWork->stickY;
            break;
        case 2:
            targetDirection.x = playerWork->aiState.targetX - playerWork->position.x;
            targetDirection.y = 0.0f;
            targetDirection.z = playerWork->aiState.targetZ - playerWork->position.z;
            targetDistance = VectorLength(&targetDirection);
            if (targetDistance < 20.0f) {
                playerWork->aiState.state = 3;
            }
            else {
                if (targetDistance >= 100.0f) {
                    turnRate = 0.8f;
                }
                else {
                    turnRate = 0.4f;
                }
                targetAngle = AngleApproach(playerWork->rotation.y, atan2d(targetDirection.x, targetDirection.z), turnRate);
                moveSpeed = 48.0f + (4.0f * difficulty);
                stickX = moveSpeed * sind(targetAngle);
                stickY = moveSpeed * -cosd(targetAngle);
                if (MapTransitionActiveGet() == 0) {
                    playerWork->aiState.state = 0;
                    if (playerWork->aiState.timer == 0) {
                        playerWork->aiState.timer = -1;
                    }
                }
                else if (playerWork->aiState.timer >= 0) {
                    if (playerWork->aiState.timer == 0) {
                        if ((targetDistance < 150.0f) && (playerWork->nearbyPlayerCount != 0)) {
                            playerWork->aiState.state = 3;
                        }
                        else if (((s32)((difficulty + 2) * (RandomValueGet() / 65536.0f)) != 0) && (otherHumanPlayerNearby == 0)) {
                            playerWork->aiState.state = 4;
                        }
                        else {
                            playerWork->aiState.state = 1;
                        }
                    }
                    else {
                        playerWork->aiState.timer--;
                    }
                }
            }
            break;
        case 3:
            targetDirection.x = playerWork->aiState.targetX - playerWork->position.x;
            targetDirection.y = 0.0f;
            targetDirection.z = playerWork->aiState.targetZ - playerWork->position.z;
            if ((VectorLength(&targetDirection) >= 30.000002f) && (playerWork->nearbyPlayerCount == 0)) {
                playerWork->aiState.state = 2;
            }
            else if (MapTransitionActiveGet() == 0) {
                playerWork->aiState.state = 0;
            }
            else {
                if (playerWork->aiState.timer > 0) {
                    playerWork->aiState.timer--;
                }
                if (playerWork->aiState.timer == 0) {
                    if (((s32)((difficulty + 2) * (RandomValueGet() / 65536.0f)) != 0) && (otherHumanPlayerNearby == 0)) {
                        playerWork->aiState.state = 4;
                    }
                    else {
                        playerWork->aiState.state = 1;
                    }
                }
            }
            break;
        case 4:
            nearbySpaceCount = MapNearestSpaceGet(nearbySpacePositions);
            for (candidateIndex = 0; candidateIndex < nearbySpaceCount; candidateIndex++) {
                VECSubtract(&nearbySpacePositions[candidateIndex], &playerWork->position, &targetDirection);
                nearbySpaceDistances[candidateIndex] = VECMag(&targetDirection);
            }
            for (candidateIndex = 0; candidateIndex < (nearbySpaceCount - 1); candidateIndex++) {
                for (neighborIndex = candidateIndex + 1; neighborIndex < nearbySpaceCount; neighborIndex++) {
                    if (nearbySpaceDistances[candidateIndex] > nearbySpaceDistances[neighborIndex]) {
                        swapPosition = nearbySpacePositions[candidateIndex];
                        nearbySpacePositions[candidateIndex] = nearbySpacePositions[neighborIndex];
                        nearbySpacePositions[neighborIndex] = swapPosition;
                        targetDistance = nearbySpaceDistances[candidateIndex];
                        nearbySpaceDistances[candidateIndex] = nearbySpaceDistances[neighborIndex];
                        nearbySpaceDistances[neighborIndex] = targetDistance;
                    }
                }
            }
            selectionWeight = 0x3E8;
            randomSelection = selectionWeight * (RandomValueGet() / 65536.0f);
            for (candidateIndex = 0; candidateIndex < (nearbySpaceCount - 1); candidateIndex++) {
                selectionWeight = 0.1f * (selectionWeight * (5.0f + difficulty));
                randomSelection -= selectionWeight;
                if (randomSelection < 0) {
                    break;
                }
            }
            playerWork->aiState.targetX = nearbySpacePositions[candidateIndex].x + (100.0f * (0.5f - (0.15f * difficulty)) * ((RandomValueGet() - 0x8000) / 32768.0f));
            playerWork->aiState.targetZ = nearbySpacePositions[candidateIndex].z + (100.0f * (0.5f - (0.15f * difficulty)) * ((RandomValueGet() - 0x8000) / 32768.0f));
            playerWork->aiState.state = 2;
            playerWork->aiState.timer = -1;
            stickX = playerWork->stickX;
            stickY = playerWork->stickY;
            break;
    }
    playerWork->stickX = stickX;
    playerWork->stickY = stickY;
    playerWork->buttonsDown = buttonsDown;
    playerWork->buttonsHeld = buttonsHeld;
}

void PlayerPhysicsUpdate(OMOBJ *object)
{
    s16 buttonsPressed;
    s16 buttonsHeld;
    Float targetAngle;

    M453PlayerWork *playerWork;
    s16 stickX;
    s16 stickY;
    s16 nextMotionIndex;
    s16 shouldLoopMotion;
    s32 modelId;

    playerWork = object->data;
    modelId = object->mdlId[0];
    nextMotionIndex = playerWork->motionIndex;
    playerWork->previousPosition = playerWork->position;
    stickX = playerWork->stickX;
    stickY = playerWork->stickY;
    buttonsPressed = playerWork->buttonsDown;
    buttonsHeld = playerWork->buttonsHeld;
    switch (playerWork->motionIndex) {
        case 0:
        case 1:
        case 2:
            playerWork->movementSpeed = 0.14f * sqrtf((stickX * stickX) + (stickY * stickY));
            if (playerWork->movementSpeed > 0.1f) {
                if (playerWork->movementSpeed >= 6.0f) {
                    nextMotionIndex = 2;
                    shouldLoopMotion = 1;
                }
                else {
                    nextMotionIndex = 1;
                    shouldLoopMotion = 1;
                }
                targetAngle = atan2d(stickX, -stickY);
                playerWork->rotation.y = AngleApproach(playerWork->rotation.y, targetAngle, 0.5f);
                playerWork->movementAngle = playerWork->rotation.y;
                playerWork->stepAnimationTime += 1.0f;
                if (playerWork->stepAnimationTime >= (0.5f * playerWork->motionDuration)) {
                    playerWork->stepAnimationTime = playerWork->stepAnimationTime - (0.5f * playerWork->motionDuration);
                }
                playerWork->movementSpeed = playerWork->movementSpeed * (1.0 + (0.25 * cosd((90.0f * (playerWork->stepAnimationTime / (0.5f * playerWork->motionDuration))))));
            }
            else {
                playerWork->stepAnimationTime = 0.0f;
                playerWork->movementSpeed = 0.0f;
                nextMotionIndex = 0;
                shouldLoopMotion = 1;
            }
            playerWork->verticalVelocity = -146.0f / REFRESH_RATE_F;
            break;
        case 3:
            shouldLoopMotion = 0;
            if ((Hu3DData[modelId].motIdShift == -1) && (CharMotionEndCheck(playerWork->characterId) != 0)) {
                playerWork->verticalVelocity += -146.0f / REFRESH_RATE_F;
            }
            else {
                playerWork->verticalVelocity = 0.0f;
            }
            if (playerWork->isAirborne != 0) {
                if (playerWork->wasHit != 0) {
                    playerWork->verticalVelocity = 0.0f;
                    playerWork->wasHit = 0;
                    nextMotionIndex = 5;
                    shouldLoopMotion = 0;
                }
                else {
                    playerWork->verticalVelocity *= -0.1f;
                    playerWork->isAirborne = 0;
                    nextMotionIndex = 4;
                    shouldLoopMotion = 0;
                }
            }
            break;
        case 4:
            playerWork->verticalVelocity += -146.0f / REFRESH_RATE_F;
            if ((playerWork->isAirborne != 0) && (CharMotionEndCheck(playerWork->characterId) != 0)) {
                nextMotionIndex = 5;
                shouldLoopMotion = 0;
            }
            break;
        case 5:
            if (CharMotionEndCheck(playerWork->characterId) != 0) {
                nextMotionIndex = 0;
                shouldLoopMotion = 1;
            }
            break;
        case 6:
        case 7:
            shouldLoopMotion = 0;
            playerWork->verticalVelocity = -146.0f / REFRESH_RATE_F;
            break;
    }
    if (playerWork->stateFlags & 4) {
        if (playerWork->actionFlags & 2) {
            Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrReset(modelId, HU3D_ATTR_DISPOFF);
        }
        if (--playerWork->actionFlags == 0) {
            playerWork->stateFlags = playerWork->stateFlags & 0xFFFFFFFB;
        }
    }
    playerWork->velocityX = playerWork->movementSpeed * sind(playerWork->movementAngle);
    playerWork->velocityZ = playerWork->movementSpeed * cosd(playerWork->movementAngle);
    playerWork->position.x += playerWork->velocityX;
    if (playerWork->isAirborne == 0) {
        playerWork->position.y = playerWork->position.y + playerWork->verticalVelocity;
    }
    playerWork->position.z += playerWork->velocityZ;
    if (nextMotionIndex != playerWork->previousMotionIndex) {
        playerWork->previousMotionIndex = nextMotionIndex;
        playerWork->motionIndex = nextMotionIndex;
        CharMotionShiftSet(playerWork->characterId, object->mtnId[playerWork->motionIndex], 0.0f, 8.0f, shouldLoopMotion);
        playerWork->motionDuration = CharMotionMaxTimeGet(playerWork->characterId);
    }
    omSetTra(object, playerWork->position.x, playerWork->position.y, playerWork->position.z);
    omSetRot(object, playerWork->rotation.x, playerWork->rotation.y, playerWork->rotation.z);
}

void PlayersReset(void)
{
    s32 playerIndex;
    M453PlayerWork *playerWork;
    s32 waitFrameCount;

    waitFrameCount = 0;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerWork = playerObjects[playerIndex]->data;
        if (playerWork->stateFlags != 0) {
            playerWork->nearbyPlayerCount = 0;
        }
    }
    while ((PlayerSeparationUpdate() != 0) && (waitFrameCount < 0x32)) {
        waitFrameCount++;
    }
}

s32 PlayerSeparationUpdate(void)
{
    Vec separationAdjustments[4];
    Vec playerSeparation;
    Float separationStrength;
    Float playerDistance;

    M453PlayerWork *playerWork;
    s32 playerIndex;
    s32 otherPlayerIndex;
    M453PlayerWork *playerWork;
    M453PlayerWork *otherPlayerWork;
    s32 overlapCount;
    OMOBJ *playerObject;

    overlapCount = 0;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        separationAdjustments[playerIndex].x = separationAdjustments[playerIndex].y = separationAdjustments[playerIndex].z = 0.0f;
    }
    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        playerWork = playerObjects[playerIndex]->data;
        if (playerWork->stateFlags != 0) {
            for (otherPlayerIndex = playerIndex + 1; otherPlayerIndex < 4; otherPlayerIndex++) {
                otherPlayerWork = playerObjects[otherPlayerIndex]->data;
                if (otherPlayerWork->stateFlags != 0) {
                    VECSubtract(&playerWork->position, &otherPlayerWork->position, &playerSeparation);
                    playerDistance = VECMag(&playerSeparation);
                    if (playerDistance < 80.0f) {
                        VECNormalize(&playerSeparation, &playerSeparation);
                        separationStrength = 0.0001f + (0.5f * (80.0f - playerDistance));
                        VECScale(&playerSeparation, &playerSeparation, separationStrength);
                        VECAdd(&separationAdjustments[playerIndex], &playerSeparation, &separationAdjustments[playerIndex]);
                        VECSubtract(&separationAdjustments[otherPlayerIndex], &playerSeparation, &separationAdjustments[otherPlayerIndex]);
                        playerWork->nearbyPlayerCount++;
                        otherPlayerWork->nearbyPlayerCount++;
                        overlapCount++;
                    }
                }
            }
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObject = playerObjects[playerIndex];
        playerWork = playerObject->data;
        if (playerWork->stateFlags != 0) {
            VECAdd(&playerWork->position, &separationAdjustments[playerIndex], &playerWork->position);
            if (playerWork->position.x > 660.0f) {
                playerWork->position.x = 660.0f;
            }
            if (playerWork->position.x < -660.0f) {
                playerWork->position.x = -660.0f;
            }
            if (playerWork->position.z > 360.0f) {
                playerWork->position.z = 360.0f;
            }
            if (playerWork->position.z < -360.0f) {
                playerWork->position.z = -360.0f;
            }
            omSetTra(playerObject, playerWork->position.x, playerWork->position.y, playerWork->position.z);
        }
    }
    return overlapCount;
}

void PlayerEventUpdate(OMOBJ *eventObject)
{
    s32 activePlayerIndices[4];

    s32 playerIndex;
    s32 activePlayerCount;
    M453PlayerWork *playerWork;
    s32 playerIndex;
    s32 waitFrameCount;

    if (gamePhase >= 6) {
        if (MapTransitionActiveGet() != 0) {
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                PlayerCollisionUpdate(playerObjects[playerIndex]);
            }
        }
        else {
            for (playerIndex = 0, activePlayerCount = 0; playerIndex < 4; playerIndex++) {
                ((M453PlayerWork *)playerObjects[playerIndex]->data)->hasPendingAction = 0;
                if (((M453PlayerWork *)playerObjects[playerIndex]->data)->stateFlags != 0) {
                    activePlayerIndices[activePlayerCount++] = playerIndex;
                }
            }
            if (activePlayerCount != 0) {
                playerIndex = activePlayerIndices[(s32)(activePlayerCount * (RandomValueGet() / 65536.0f))];
                ((M453PlayerWork *)playerObjects[playerIndex]->data)->hasPendingAction = 1;
            }
        }
        waitFrameCount = 0;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerWork = playerObjects[playerIndex]->data;
            if (playerWork->stateFlags != 0) {
                playerWork->nearbyPlayerCount = 0;
            }
        }
        while ((PlayerSeparationUpdate() != 0) && (waitFrameCount < 0x32)) {
            waitFrameCount++;
        }
    }
}

void PlayerTargetPositionGet(Vec *playerPosition, Vec *targetPosition)
{
    Vec pathPositions[6];
    Vec positionDelta;
    Float nearestDistance;
    Float candidateDistance;

    s32 pathIndex;
    s32 pathPointCount;

    pathPointCount = MapNearestPathGet(pathPositions);
    nearestDistance = 2000.0f;
    for (pathIndex = 0; pathIndex < pathPointCount; pathIndex++) {
        VECSubtract(&pathPositions[pathIndex], playerPosition, &positionDelta);
        candidateDistance = VECMag(&positionDelta);
        if (!(candidateDistance >= nearestDistance)) {
            nearestDistance = candidateDistance;
            *targetPosition = pathPositions[pathIndex];
        }
    }
}

Float characterMapClearances[8] = { 40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 100.0f, 40.0f, 80.0f };

Float characterMapSafetyDistances[8] = { 160.0f, 160.0f, 180.0f, 160.0f, 160.0f, 180.0f, 180.0f, 200.0f };

void PlayerCollisionUpdate(OMOBJ *object)
{
    Vec safePosition;
    Float modelScale;
    Float distanceFromMapPlane;

    M453PlayerWork *playerWork;
    s32 playerIndex;
    s16 coinReward;

    playerWork = object->data;
    if (playerWork->stateFlags != 0) {
        distanceFromMapPlane = MapPositionDistance(&playerWork->position, characterMapClearances[playerWork->characterId]) - 10.0f;
        if (distanceFromMapPlane < 0.0f) {
            distanceFromMapPlane = 0.0f;
        }
        if ((playerWork->stateFlags & 2) == 0) {
            modelScale = 1.0f;
            if (distanceFromMapPlane < characterMapSafetyDistances[playerWork->characterId]) {
                if (MapSpaceContainsPosition(&playerWork->position, &safePosition) != 0) {
                    playerWork->position.x = safePosition.x;
                    playerWork->position.z = safePosition.z;
                }
                else {
                    if ((MapPlaneDistance(playerWork->position.x, playerWork->position.z) - 10.0f) < 100.0f) {
                        playerWork->stateFlags = playerWork->stateFlags | 2;
                        coinReward = finalScore;
                        playerIndex = playerWork->playerIndex;
                        GWPlayerCoinWinSet(playerIndex, coinReward);
                    }
                    modelScale = distanceFromMapPlane / characterMapSafetyDistances[playerWork->characterId];
                }
            }
        }
        else {
            modelScale = distanceFromMapPlane / characterMapSafetyDistances[playerWork->characterId];
        }
        if (object->scale.y < modelScale) {
            modelScale = object->scale.y + (0.2f * (modelScale - object->scale.y));
        }
        object->scale.y = modelScale;
    }
}

s16 ParticleCreate(s32 particleResourceId, s16 particleCount, HU3DPARTICLEHOOK particleUpdateHook)
{
    HU3DPARTICLE *particleWork;
    s32 particleModelId;
    ANIMDATA *particleAnimation;

    particleAnimation = HuSprAnimRead(HuDataReadNum(particleResourceId, 0x10000000));
    particleModelId = Hu3DParticleCreate(particleAnimation, particleCount);
    Hu3DParticleHookSet(particleModelId, particleUpdateHook);
    particleWork = Hu3DData[particleModelId].hookData;
    particleWork->unk_2E = particleModelId;
    particleWork->dataCnt = particleWork->emitCnt = 0;
    return particleModelId;
}

void ParticleUpdate(HU3DMODEL *particleModel, HU3DPARTICLE *particleWork, Mtx particleTransform)
{
    s16 particlePhase = 0;
    Float fadeProgress;
    Float movementScale;
    Float particleAngleStep;
    Float unusedFloat;

    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;

    switch (particleWork->dataCnt) {
        case 0:
            particleAngleStep = 360.0f / particleWork->maxCnt;
            for (particleData = particleWork->data, particleIndex = 0; particleIndex < particleWork->maxCnt; particleIndex++, particleData++) {
                particleData->vel.x = sind((particleAngleStep * particleIndex)) * (1.0f + (0.1f * (6.0f * ((RandomValueGet() - 0x8000) / 32768.0f))));
                particleData->vel.y = 0.0f;
                particleData->vel.z = cosd((particleAngleStep * particleIndex)) * (1.0f + (0.1f * (6.0f * ((RandomValueGet() - 0x8000) / 32768.0f))));
                particleData->pos.x = 0.0f;
                particleData->pos.y = 0.0f;
                particleData->pos.z = 0.0f;
            }
            particleWork->emitCnt = REFRESH_RATE / 2;
            particleWork->dataCnt++;
        case 1:
            particleWork->emitCnt--;
            fadeProgress = ((REFRESH_RATE_F / 2) - particleWork->emitCnt) / (REFRESH_RATE_F / 2);
            movementScale = particleWork->emitCnt / (REFRESH_RATE_F / 2);
            for (particleData = particleWork->data, particleIndex = 0; particleIndex < particleWork->maxCnt; particleIndex++, particleData++) {
                particleData->pos.x += 5.0f * movementScale * particleData->vel.x;
                particleData->pos.y = 20.0f;
                particleData->pos.z += 5.0f * movementScale * particleData->vel.z;
                particleData->scale = 20.0f + (60.0f * fadeProgress);
                particleData->color.r = particleData->color.g = particleData->color.b = 0xFF;
                particleData->color.a = 64.0f * (1.0f - (fadeProgress * fadeProgress));
            }
            if (particleWork->emitCnt == 0) {
                particleWork->dataCnt++;
            }
            break;
        case 2:
            Hu3DModelKill(particleWork->unk_2E);
            return;
    }
    DCFlushRange(particleWork->data, particleWork->maxCnt * sizeof(HU3DPARTICLEDATA));
}

u32 randomGeneratorState = 0x41C64E6D;

s32 RandomValueGet(void)
{
    randomGeneratorState *= 0x41C64E6D;
    randomGeneratorState += 0x3039;
    return randomGeneratorState >> 0x10;
}

Float AngleApproach(Float currentAngle, Float targetAngle, Float approachRate)
{
    Float angleDifference;

    if (currentAngle > 180.0f) {
        currentAngle -= 360.0f;
    }
    else if (currentAngle <= -180.0f) {
        currentAngle += 360.0f;
    }
    if (targetAngle > 180.0f) {
        targetAngle -= 360.0f;
    }
    else if (targetAngle <= -180.0f) {
        targetAngle += 360.0f;
    }
    angleDifference = currentAngle - targetAngle;
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    else if (angleDifference <= -180.0f) {
        angleDifference += 360.0f;
    }
    currentAngle = targetAngle + (angleDifference * approachRate);
    if (currentAngle > 180.0f) {
        return currentAngle - 360.0f;
    }
    if (currentAngle <= -180.0f) {
        currentAngle += 360.0f;
    }
    return currentAngle;
}

void VectorTransform(Mtx rotationMatrix, Float xRotationDegrees, Float yRotationDegrees, Float zRotationDegrees)
{
    Mtx xRotationMatrix;
    Mtx yRotationMatrix;
    if (zRotationDegrees != 0.0f) {
        MTXRotRad(rotationMatrix, 0x5A, 0.017453292f * zRotationDegrees);
    }
    else {
        MTXIdentity(rotationMatrix);
    }
    if (xRotationDegrees != 0.0f) {
        MTXRotRad(xRotationMatrix, 0x58, 0.017453292f * xRotationDegrees);
        MTXConcat(xRotationMatrix, rotationMatrix, rotationMatrix);
    }
    if (yRotationDegrees != 0.0f) {
        MTXRotRad(yRotationMatrix, 0x59, 0.017453292f * yRotationDegrees);
        MTXConcat(yRotationMatrix, rotationMatrix, rotationMatrix);
    }
}
void VectorRotate(Float *rotationMatrix, Vec *rotation)
{
    Float sineY;
    Float cosineY;

    rotation->x = atan2d(rotationMatrix[10], rotationMatrix[6]);
    rotation->z = atan2d(rotationMatrix[0], rotationMatrix[1]);
    sineY = -rotationMatrix[2];
    cosineY = sqrtf((1.0 - (sineY * sineY)) < 0.0 ? -(1.0 - (sineY * sineY)) : 1.0 - (sineY * sineY));
    if ((rotation->x > 90.0f) && (rotation->x < 270.0f) && (rotation->z > 90.0f) && (rotation->z < 270.0f)) {
        rotation->x = fmod(180.0f + rotation->x, 360.0);
        rotation->z = fmod(180.0f + rotation->z, 360.0);
        cosineY = -cosineY;
    }
    rotation->y = atan2d(cosineY, sineY);
}

Float SplineCubicCoefficientGet(Float progress, Float startValue, Float controlValue, Float endValue)
{
    Float remainingProgress;
    Float interpolatedValue;

    remainingProgress = 1.0 - progress;
    interpolatedValue = (endValue * (progress * progress)) + ((startValue * (remainingProgress * remainingProgress)) + (2.0 * (controlValue * (remainingProgress * progress))));
    return interpolatedValue;
}

void SplineCubicCoefficientsSet(Float *startValues, Float *controlValues, Float *endValues, Float *coefficients, Float progress)
{
    s32 componentIndex;

    for (componentIndex = 0; componentIndex < 3; componentIndex++) {
        *coefficients = SplineCubicCoefficientGet(progress, *startValues++, *controlValues++, *endValues++);
        coefficients++;
    }
}

Float SplineHermiteCoefficientGet(Float progress, Float startValue, Float startTangent, Float endValue)
{
    Float interpolatedValue = 2.0 * ((progress * endValue) + (((progress - 1.0) * startValue) + ((1.0 - (2.0 * progress)) * startTangent)));
    return interpolatedValue;
}

void SplineHermiteCoefficientsSet(Float *startValues, Float *controlValues, Float *endValues, Float *normalizedTangents, Float progress)
{
    Float tangentComponents[3];
    Float tangentMagnitude;
    Float unusedCoefficient;
    Float tangentMagnitudeSquared;
    Float unusedCoefficient28;
    Float inverseTangentMagnitude;
    Float unusedCoefficient27;
    Float unusedCoefficient26;
    Float unusedCoefficient25;
    Float unusedCoefficient24;
    Float unusedCoefficient23;
    Float unusedCoefficient22;
    Float unusedCoefficient21;

    s32 componentIndex;

    for (componentIndex = 0; componentIndex < 3; componentIndex++) {
        tangentComponents[componentIndex] = SplineHermiteCoefficientGet(progress, *startValues++, *controlValues++, *endValues++);
    }

    tangentMagnitudeSquared = (tangentComponents[2] * tangentComponents[2]) + ((tangentComponents[0] * tangentComponents[0]) + (tangentComponents[1] * tangentComponents[1]));
    inverseTangentMagnitude = sqrtf(tangentMagnitudeSquared);
    if (inverseTangentMagnitude) {
        inverseTangentMagnitude = 1.0 / inverseTangentMagnitude;
        for (componentIndex = 0; componentIndex < 3; componentIndex++) {
            *normalizedTangents = inverseTangentMagnitude * tangentComponents[componentIndex];
            normalizedTangents++;
        }
        return;
    }
    *normalizedTangents++ = 0.0f;
    *normalizedTangents++ = 0.0f;
    *normalizedTangents++ = 1.0f;
}

void SplineLinearCoefficientsSet(Float *knotPositions, Float *knotValues, Float *secondDerivatives, s32 knotCount)
{
    Float intervalWidths[16];
    Float slopeAndPivotScratch[16];
    double eliminationFactor;

    s32 knotIndex;

    secondDerivatives[0] = 0.0f;
    secondDerivatives[knotCount - 1] = 0.0f;
    for (knotIndex = 0; knotIndex < (knotCount - 1); knotIndex++) {
        intervalWidths[knotIndex] = knotPositions[knotIndex + 1] - knotPositions[knotIndex];
        if (intervalWidths[knotIndex] == 0.0f) {
            slopeAndPivotScratch[knotIndex + 1] = 0.0f;
        }
        else {
            slopeAndPivotScratch[knotIndex + 1] = (knotValues[knotIndex + 1] - knotValues[knotIndex]) / intervalWidths[knotIndex];
        }
    }
    secondDerivatives[1] = slopeAndPivotScratch[2] - slopeAndPivotScratch[1];
    slopeAndPivotScratch[1] = 2.0f * (knotPositions[2] - knotPositions[0]);
    for (knotIndex = 1; knotIndex < (knotCount - 2); knotIndex++) {
        eliminationFactor = intervalWidths[knotIndex] / slopeAndPivotScratch[knotIndex];
        secondDerivatives[knotIndex + 1] = (slopeAndPivotScratch[knotIndex + 2] - slopeAndPivotScratch[knotIndex + 1]) - (eliminationFactor * secondDerivatives[knotIndex]);
        slopeAndPivotScratch[knotIndex + 1] = (2.0f * (knotPositions[knotIndex + 2] - knotPositions[knotIndex])) - (eliminationFactor * intervalWidths[knotIndex]);
    }
    secondDerivatives[knotCount - 2] -= intervalWidths[knotCount - 2] * secondDerivatives[knotCount - 1];
    for (knotIndex = knotCount - 2; knotIndex > 0; knotIndex--) {
        if (slopeAndPivotScratch[knotIndex] == 0.0f) {
            secondDerivatives[knotIndex] = 0.0f;
        }
        else {
            secondDerivatives[knotIndex] = (secondDerivatives[knotIndex] - (intervalWidths[knotIndex] * secondDerivatives[knotIndex + 1])) / slopeAndPivotScratch[knotIndex];
        }
    }
}

Float SplineLinearEvaluate(Float parameter, Float *knotPositions, Float *knotValues, Float *secondDerivatives, s32 knotCount)
{
    Float intervalWidth;
    Float scaledParameter;
    Float interpolatedValue;

    s32 intervalIndex = parameter;
    parameter = parameter - intervalIndex;
    intervalWidth = knotPositions[intervalIndex + 1] - knotPositions[intervalIndex];
    scaledParameter = intervalWidth * parameter;
    interpolatedValue = knotValues[intervalIndex]
        + (scaledParameter
            * ((scaledParameter * ((3.0f * secondDerivatives[intervalIndex]) + ((scaledParameter * (secondDerivatives[intervalIndex + 1] - secondDerivatives[intervalIndex])) / intervalWidth)))
                + (((knotValues[intervalIndex + 1] - knotValues[intervalIndex]) / intervalWidth) - (intervalWidth * ((2.0f * secondDerivatives[intervalIndex]) + secondDerivatives[intervalIndex + 1])))));
    if (intervalWidth == 0.0f) {
        interpolatedValue = knotValues[intervalIndex];
    }
    return interpolatedValue;
}

void SplineCubicCoefficientsPrepare(Float *knotPositions, Float (*controlPoints)[3], Float (*tangentCoefficients)[3], s32 pointCount)
{
    Float componentValues[16];
    Float secondDerivatives[16];
    Float slopeAndPivotScratch[16];
    Float intervalWidths[16];
    Vec pointDelta;
    double eliminationFactor;

    s32 pointIndex;
    s32 componentIndex;
    s32 axisIndex;

    knotPositions[0] = 0.0f;
    for (componentIndex = 1; componentIndex < pointCount; componentIndex++) {
        VECSubtract((Vec *)&controlPoints[componentIndex], (Vec *)&controlPoints[componentIndex - 1], &pointDelta);
        knotPositions[componentIndex] = knotPositions[componentIndex - 1] + VECMag(&pointDelta);
    }
    for (axisIndex = 0; axisIndex < 3; axisIndex++) {
        for (componentIndex = 0; componentIndex < pointCount; componentIndex++) {
            componentValues[componentIndex] = controlPoints[componentIndex][axisIndex];
        }
        secondDerivatives[0] = 0.0f;
        secondDerivatives[pointCount - 1] = 0.0f;
        for (pointIndex = 0; pointIndex < (pointCount - 1); pointIndex++) {
            intervalWidths[pointIndex] = knotPositions[pointIndex + 1] - knotPositions[pointIndex];
            if (intervalWidths[pointIndex] == 0.0f) {
                slopeAndPivotScratch[pointIndex + 1] = 0.0f;
            }
            else {
                slopeAndPivotScratch[pointIndex + 1] = (componentValues[pointIndex + 1] - componentValues[pointIndex]) / intervalWidths[pointIndex];
            }
        }
        secondDerivatives[1] = slopeAndPivotScratch[2] - slopeAndPivotScratch[1];
        slopeAndPivotScratch[1] = 2.0f * (knotPositions[2] - knotPositions[0]);
        for (pointIndex = 1; pointIndex < (pointCount - 2); pointIndex++) {
            eliminationFactor = intervalWidths[pointIndex] / slopeAndPivotScratch[pointIndex];
            secondDerivatives[pointIndex + 1] = (slopeAndPivotScratch[pointIndex + 2] - slopeAndPivotScratch[pointIndex + 1]) - (eliminationFactor * secondDerivatives[pointIndex]);
            slopeAndPivotScratch[pointIndex + 1] = (2.0f * (knotPositions[pointIndex + 2] - knotPositions[pointIndex])) - (eliminationFactor * intervalWidths[pointIndex]);
        }
        secondDerivatives[pointCount - 2] -= intervalWidths[pointCount - 2] * secondDerivatives[pointCount - 1];
        for (pointIndex = pointCount - 2; pointIndex > 0; pointIndex--) {
            if (slopeAndPivotScratch[pointIndex] == 0.0f) {
                secondDerivatives[pointIndex] = 0.0f;
            }
            else {
                secondDerivatives[pointIndex] = (secondDerivatives[pointIndex] - (intervalWidths[pointIndex] * secondDerivatives[pointIndex + 1])) / slopeAndPivotScratch[pointIndex];
            }
        }
        for (componentIndex = 0; componentIndex < pointCount; componentIndex++) {
            tangentCoefficients[componentIndex][axisIndex] = secondDerivatives[componentIndex];
        }
    }
}

void SplineCubicEvaluate(Float parameter, Float *outputVector, Float *knotPositions, Float (*controlPoints)[3], Float (*tangentCoefficients)[3], s32 pointCount)
{
    Float controlPointComponents[16];
    Float tangentComponents[16];

    Float unusedCoefficient31;
    Float unusedCoefficient30;
    Float unusedCoefficient29;
    Float unusedCoefficient28;
    Float unusedCoefficient27;

    s32 unusedIndex31;
    s32 pointIndex;
    s32 axisIndex;

    for (axisIndex = 0; axisIndex < 3; axisIndex++) {
        for (pointIndex = 0; pointIndex < pointCount; pointIndex++) {
            controlPointComponents[pointIndex] = controlPoints[pointIndex][axisIndex];
            tangentComponents[pointIndex] = tangentCoefficients[pointIndex][axisIndex];
        }
        outputVector[axisIndex] = SplineLinearEvaluate(parameter, knotPositions, controlPointComponents, tangentComponents, 3);
    }
}

Float VectorLength(Vec *vector)
{
    Float unusedValue;
    Float vectorLength;
    Float inverseVectorLength;

    vectorLength = (vector->z * vector->z) + ((vector->x * vector->x) + (vector->y * vector->y));
    vectorLength = sqrtf(vectorLength);
    if (vectorLength != 0.0f) {
        inverseVectorLength = 1.0f / vectorLength;
        vector->x *= inverseVectorLength;
        vector->y *= inverseVectorLength;
        vector->z *= inverseVectorLength;
    }
    else {
        vector->x = vector->y = vector->z = 0.0f;
    }
    return vectorLength;
}

Float VectorProjection(Vec segmentStart, Vec segmentDirection, Vec *projectedPoint, Float segmentProgress)
{
    if (segmentProgress <= 0.0f) {
        projectedPoint->x = segmentStart.x;
        projectedPoint->y = segmentStart.y;
        projectedPoint->z = segmentStart.z;
        segmentProgress = 0.0f;
        return segmentProgress;
    }
    if (segmentProgress >= 1.0f) {
        projectedPoint->x = segmentStart.x + segmentDirection.x;
        projectedPoint->y = segmentStart.y + segmentDirection.y;
        projectedPoint->z = segmentStart.z + segmentDirection.z;
        segmentProgress = 1.0f;
        return segmentProgress;
    }
    projectedPoint->x = segmentStart.x + (segmentProgress * segmentDirection.x);
    projectedPoint->y = segmentStart.y + (segmentProgress * segmentDirection.y);
    projectedPoint->z = segmentStart.z + (segmentProgress * segmentDirection.z);
    return segmentProgress;
}

Float VectorDistance(Vec point, Vec lineOrigin, Vec lineDirection)
{
    Float lineParameter = (lineDirection.z * (lineOrigin.z - point.z)) + ((lineDirection.x * (lineOrigin.x - point.x)) + (lineDirection.y * (lineOrigin.y - point.y)));
    Float negativeDirectionLengthSquared = -((lineDirection.z * lineDirection.z) + ((lineDirection.x * lineDirection.x) + (lineDirection.y * lineDirection.y)));
    if (negativeDirectionLengthSquared != 0.0f) {
        lineParameter /= negativeDirectionLengthSquared;
    }
    return lineParameter;
}

Float VectorSegmentProjection(Vec *point, Vec *segmentStart, Vec *segmentDirection)
{
    Vec projectedPoint;
    Float segmentProgress;

    if (((segmentDirection->z * segmentDirection->z) + ((segmentDirection->x * segmentDirection->x) + (segmentDirection->y * segmentDirection->y))) == 0.0f) {
        return ((point->z - segmentStart->z) * (point->z - segmentStart->z))
            + (((point->x - segmentStart->x) * (point->x - segmentStart->x)) + ((point->y - segmentStart->y) * (point->y - segmentStart->y)));
    }
    segmentProgress = VectorDistance(*point, *segmentStart, *segmentDirection);
    VectorProjection(*segmentStart, *segmentDirection, &projectedPoint, segmentProgress);
    return ((point->z - projectedPoint.z) * (point->z - projectedPoint.z)) + (((point->x - projectedPoint.x) * (point->x - projectedPoint.x)) + ((point->y - projectedPoint.y) * (point->y - projectedPoint.y)));
}

Float VectorSegmentIntersection(Vec *point, Vec *segmentStart, Vec *segmentDirection, Vec *projectedPoint, Vec *separationVector)
{
    Float segmentProgress;
    if (((segmentDirection->z * segmentDirection->z) + ((segmentDirection->x * segmentDirection->x) + (segmentDirection->y * segmentDirection->y))) == 0.0f) {
        return ((point->z - segmentStart->z) * (point->z - segmentStart->z))
            + (((point->x - segmentStart->x) * (point->x - segmentStart->x)) + ((point->y - segmentStart->y) * (point->y - segmentStart->y)));
    }
    segmentProgress = VectorDistance(*point, *segmentStart, *segmentDirection);
    VectorProjection(*segmentStart, *segmentDirection, projectedPoint, segmentProgress);

    separationVector->x = projectedPoint->x - point->x;
    separationVector->y = projectedPoint->y - point->y;
    separationVector->z = projectedPoint->z - point->z;

    return VectorLength(separationVector);
}

s32 PlayerScoreGet(void)
{
    return finalScore;
}

s32 HasActiveHumanPlayer(void)
{
    s32 playerIndex;
    OMOBJ **playerObjectsByGroup;
    M453PlayerWork *playerWork;

    playerObjectsByGroup = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    for (playerIndex = 0; playerIndex < playerMotionTimer; playerIndex++) {
        playerWork = playerObjectsByGroup[playerMotionIds[playerIndex]]->data;
        if (playerWork->stateFlags != 0) {
            return 1;
        }
    }
    return 0;
}

void GameEnd(void)
{
    s32 playerIndex;
    M453PlayerWork *playerWork;
    OMOBJ **playerObjectsByGroup;
    s32 playerIndex;
    s16 coinReward;

    playerObjectsByGroup = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerWork = playerObjectsByGroup[playerIndex]->data;
        if (playerWork->stateFlags != 0) {
            coinReward = finalScore;
            playerIndex = playerWork->playerIndex;
            GWPlayerCoinWinSet(playerIndex, coinReward);
        }
    }
}

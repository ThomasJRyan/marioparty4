#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include <string.h>

#include "REL/m438Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

#define BOARD_FABS(value) ((value < 0.0f) ? -(value) : (value))

typedef struct M438PlayerWork {
    u8 playerIndex;
    u8 characterId;
    u8 teamIndex;
    u8 playerTeamSlot;
    u8 controllerPort;
    u8 isComputerControlled;
    u8 difficultyLevel;
    u8 playerStateFlags;
    char unreferencedInputPadding[2];
    u8 currentInputState;
    u8 modelVisibilityTimer;
    u8 unreferencedPlayerState;
    u8 groundContactFlag;
    u8 landingFlag;
    u8 unreferencedPlayerByte;
    s32 isInactive;
    s32 specialMotionState;
    s16 stickX;
    s16 stickY;
    s16 pressedButtons;
    s16 heldButtons;
    s16 currentMotionIndex;
    s16 previousMotionIndex;
    float movementSpeed;
    float facingAngle;
    float walkCyclePhase;
    float currentMotionDuration;
    s32 idleFrameLimit;
    s32 idleFrameCount;
    float difficultyMovementScale;
    u32 idleDelayFrames;
    u32 windCycleFrame;
    float windForceMagnitude;
    Vec windVelocity;
    Vec windTargetPosition;
    s32 windActive;
    s32 windUpdatePending;
    Vec worldPosition;
    float modelRotationX;
    float modelRotationY;
    float modelRotationZ;
    float velocityX;
    float verticalVelocity;
    float velocityZ;
    Vec previousWorldPosition;
    float windDirection;
    float windStrength;
    s32 characterEffectHandle;
    s32 impactSoundHandle;
    float unreferencedWindParameterA;
    float unreferencedWindParameterB;
    float unreferencedWindParameterC;
    char reservedB8[0x34];
    u8 reservedEC;
    u8 reservedED;
    char unreferencedTailPadding[2];
    char reservedF0[0x40]; // unknown type
} M438PlayerWork; /* size = 0x130 */

typedef struct M438CameraPositionWork {
    char reserved00[0x6C];
    Vec previousCameraPosition;
    char reserved78[0x0C];
    Vec cameraDisplacement;
    Vec cameraPosition;
} M438CameraPositionWork; /* size =  */ // TODO maybe same as M438PlayerWork?

typedef struct M438CutsceneCameraWork {
    u8 pathMode;
    s16 transitionTimer;
    union {
        Vec cameraCenter;
        s32 cameraTransitionEnabled;
    };
    Vec cameraRotation;
    Vec startCenter;
    Vec startRotation;
    Vec endCenter;
    Vec endRotation;
    Vec controlCenter;
    Vec controlRotation;
    Vec centerControlPoints[3];
    float centerSplineCoefficients[3][3];
    Vec rotationControlPoints[3];
    float rotationSplineCoefficients[3][3];
    float centerSplineValues[3];
    float rotationSplineValues[3];
    float zoomKeyframes[3];
    float zoomSplineCoefficients[3];
    float zoomKeyframeDistances[3];
    float currentZoom;
    float startZoom;
    float targetZoom;
    float controlZoom;
    float zoomStep;
    float transitionDuration;
    float transitionFramesRemaining;
    float shakeAmplitude[3];
    float shakeDamping[3];
    float shakeAngle[3];
    float shakeAngularSpeed[3];
    float shakeOffset[3];
    u16 shakeFramesRemaining[3];
} M438CutsceneCameraWork; /* size = 0x190 */

typedef struct M438WindEffectWork {
    /* 0x00 */ u8 effectState;
    u8 pathSegmentIndex;
    s8 pathGroupIndex;
    char unreferencedPadding;
    s16 stateTimer;
    s16 countdownFrames;
    s16 primaryParticleSystemId;
    s16 secondaryParticleSystemId;
    float riseHeight;
    float particleScale;
    float pathSegmentLength;
    Vec pathSegmentStart;
    Vec pathSegmentDirection;
} M438WindEffectWork; /* size = 0x30 */

typedef u8 M438CollisionGrid[0x30];

void InitializeSceneController(OMOBJ *object);
void UpdateSceneIntro(OMOBJ *object);
void UpdateSceneResults(OMOBJ *object);
void FinishScene(OMOBJ *object);
void InitializeWindPaths(OMOBJ *object);
void UpdateWindEffects(OMOBJ *object);
void UpdateCutsceneCamera(OMOBJ *object);
void EmptyPlayerCallback(OMOBJ *);
void SetCameraView(float transitionDuration, Vec *targetCenter, Vec *targetRotation, float targetZoom);
void ConfigureCameraShake(u8 shakeIndex, u16 durationFrames, float amplitude, float damping, float angularSpeed);
void InitializePlayer(OMOBJ *object);
void UpdatePlayerIntro(OMOBJ *object);
void UpdatePlayerObjects(OMOBJ *object);
void EmptySceneCallback(OMOBJ *object);
void UpdatePlayerMotion(OMOBJ *object);
void UpdatePlayerWindMotion(OMOBJ *object);
void InitializeCharacterCamera(OMOBJ *object);
void UpdateCharacterCamera(OMOBJ *object);
void UpdatePlayerEffects(OMOBJ *object);
void UpdateWindTrailParticles(M438ParticleSystem *particleSystem);
void UpdateFootstepParticles(M438ParticleSystem *particleSystem);
void UpdatePathDebrisParticles(M438ParticleSystem *particleSystem);
void UpdateImpactParticles(M438ParticleSystem *particleSystem);
void UpdateLandingParticles(M438ParticleSystem *particleSystem);
void UpdateSceneCleanup(OMOBJ *object);
void InitializeSceneAudio(void);
void *AllocateSceneMemory(s32 allocationSize);
void FreeAllSceneMemory(void);
s32 GetRandomU16(void);
void ChooseRandomIndices(s32 valueCount, s32 *indices);
float ApproachAngleSmoothly(float currentAngle, float targetAngle, float interpolationFactor);
void RotateVectorByMatrix(Mtx matrix, Vec *vector);
void SolveSplineCoefficients(float *controlPointDistances, float *controlPointValues, float *secondDerivatives, s32 controlPointCount);
float EvaluateSpline(float segmentPosition, float *controlPointDistances, float *controlPointValues, float *secondDerivatives, s32 controlPointCount);
void BuildSplineCoefficients(float *controlPointDistances, Vec *controlPoints, float splineCoefficients[3][3], s32 controlPointCount);
void EvaluateVectorSpline(float segmentPosition, float *outputVector, float *controlPointDistances, float splineCoefficients[3][3], float tangentCoefficients[3][3], s32 controlPointCount);
float NormalizeVector(Vec *vector);
float DistanceSquaredToSegment(Vec *point, Vec *segmentStart, Vec *segmentDirection);
float IntersectSegments(Vec *lineStart, Vec *lineDirection, Vec *segmentStart, Vec *segmentDirection);
void InitializeWindCollisionMap(OMOBJ *object);
void EmptyWindMapCallback(OMOBJ *object);
void UpdateWindCollisionMap(HU3DMODEL *model, Mtx matrix);
s32 FindWindAvoidancePosition(u32 playerIndex, float windStrength, Vec *originPosition, Vec *avoidancePosition);
float ProjectPointOnSegment(Vec *point, Vec *segmentStart, Vec *segmentDirection);

Vec globalLightPosition = { 1000.0f, 3000.0f, 1000.0f };
Vec globalLightDirection = { -0.4f, -0.6f, -0.1f };
GXColor globalLightColor = { 0xFF, 0xD0, 0xD0, 0xFF };
Vec shadowCameraPosition = { 500.0f, 3000.0f, 500.0f };
Vec shadowUpDirection = { 0.0f, 1.0f, 0.0f };
Vec shadowTargetPosition = { 0.0f, 0.0f, 0.0f };

M438FireManager fireManager;
OMOBJ *sceneControllerObject;
OMOBJ *outOfViewObject;
OMOBJ *cutsceneControllerObject;
OMOBJ *sceneUpdateObject;
OMOBJ *playerObjects[4];
OMOBJ *cameraControllerObject;
OMOBJ *windCollisionMapObject;
u16 scenePhase;
s16 countdownSequenceId;
s16 sceneMessageSequenceId;
s16 sceneCountdownFrames;
s16 scenePhaseTimer;
s16 playerRankByIndex[4];
s16 nextPlayerRank;
s32 sceneElapsedFrames;
void *heapAllocationSlots[0x80];
u32 sceneEffectTimers[3];
M438WindEffectWork windEffectRecords[48];
s32 windEffectTimer;
u16 collisionRiskScratch[0x144];
s32 secondaryFireSystemSetupCount;
s32 activeFireSystemId;
static u32 unusedStaticState;

void ObjectSetup(void)
{
    HUPROCESS *processManager;
    s32 playerIndex;
    OMOBJ *playerObject;
    s32 lightIndex;
    HU3DLIGHT *globalLight;
    s32 randomFrameOffset;

    OSReport("******* M438ObjectSetup *********\n");
    Hu3DLighInit();
    lightIndex = Hu3DGLightCreateV(&globalLightPosition, &globalLightDirection, &globalLightColor);
    Hu3DGLightSpotSet(lightIndex, 80.0f, 2);
    Hu3DGLightInfinitytSet(lightIndex);
    globalLight = Hu3DGlobalLight;
    globalLight->type |= 0x8000;
    processManager = omInitObjMan(0x32, 0x2000);
    randomFrameOffset = frand() & 0x1F;
    for (playerIndex = 0; playerIndex < randomFrameOffset; playerIndex++) {
        GetRandomU16();
    }
    CRot.x = -40.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    Center.x = 0.0f;
    Center.y = -10.0f;
    Center.z = 50.0f;
    CZoom = 2075.0f;
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 45.0f, 50.0f, 8000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 0x280, 0x1E0);
    outOfViewObject = omAddObjEx(processManager, 0x7FDA, 0, 0, -1, omOutView);
    sceneControllerObject = omAddObjEx(processManager, 0x10, 8, 0, -1, InitializeSceneController);
    sceneUpdateObject = omAddObjEx(processManager, 0x20, 0x10, 0, -1, InitializeWindPaths);
    cutsceneControllerObject = omAddObjEx(processManager, 0x50, 0x10, 0, -1, UpdateCutsceneCamera);
    cameraControllerObject = omAddObjEx(processManager, 0x40, 2, 4, -1, InitializeCharacterCamera);
    omAddObjEx(processManager, 0x60, 0, 0, -1, UpdateSceneCleanup);
    omMakeGroupEx(processManager, 0, 4);
    omGetGroupMemberListEx(processManager, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObject = omAddObjEx(processManager, 0x30, 2, 0xB, 0, InitializePlayer);
        playerObjects[playerIndex] = playerObject;
        playerObject->work[0] = playerIndex;
        playerObject->work[1] = playerIndex;
    }
    windCollisionMapObject = omAddObjEx(processManager, 0x30, 1, 0, 0, InitializeWindCollisionMap);
    Hu3DShadowCreate(30.0f, 100.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.4f);
    Hu3DShadowPosSet(&shadowCameraPosition, &shadowUpDirection, &shadowTargetPosition);
    SetupMapEffects(processManager);
    omGameSysInit(processManager);
    HuAudSndGrpSet(0x40);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    InitializeSceneAudio();
    InitializeFireManager(0x60, 0x40);
}

void InitializeSceneController(OMOBJ *object)
{
    scenePhase = 0;
    object->objFunc = UpdateSceneIntro;
}

void UpdateSceneIntro(OMOBJ *object)
{
    Vec cameraCenter;
    Vec cameraRotation;

    float targetZoom;

    switch (scenePhase) {
        case 0:
            cameraCenter.x = 0.0f;
            cameraCenter.y = 80.0f;
            cameraCenter.z = -1850.0f;

            cameraRotation.x = 6.0f;
            cameraRotation.y = 0.0f;
            cameraRotation.z = 0.0f;
            targetZoom = 800.0f;
            SetCameraView(1.0f, &cameraCenter, &cameraRotation, targetZoom);
            scenePhaseTimer = REFRESH_RATE * 1.2;
            ConfigureCameraShake(1, REFRESH_RATE_F / 2.5f + scenePhaseTimer, 10.0f, 0.999f, 90.0f);
            ConfigureCameraShake(2, REFRESH_RATE_F / 2.5f + scenePhaseTimer, 20.0f, 0.999f, 180.0f);
            scenePhase += 1;
            LoadParticleAnimation(DATA_MAKE_NUM(DATADIR_M438, 15), 0, 0);
            LoadParticleAnimation(DATA_MAKE_NUM(DATADIR_M438, 9), 0x40, 0x40);
            LoadParticleAnimation(DATA_MAKE_NUM(DATADIR_M438, 10), 0x40, 0x40);
            break;
        case 1:
            if (--scenePhaseTimer == 0) {
                scenePhaseTimer = 0x14;
                scenePhase += 1;
            }
            break;
        case 3:
            if (--scenePhaseTimer == 0) {
                cameraCenter.x = 0.0f;
                cameraCenter.y = -10.0f;
                cameraCenter.z = 50.0f;

                cameraRotation.x = -40.0f;
                cameraRotation.y = 0.0f;
                cameraRotation.z = 0.0f;
                targetZoom = 2075.0f;
                scenePhaseTimer = REFRESH_RATE;
                SetCameraView(scenePhaseTimer, &cameraCenter, &cameraRotation, targetZoom);
                scenePhase += 1;
            }
            break;
        case 4:
            if (--scenePhaseTimer == 0) {
                scenePhase += 1;
            }
            break;
        case 5:
            scenePhase = 6;
            countdownSequenceId = sceneMessageSequenceId = -1;
            object->objFunc = UpdateSceneResults;
            break;
    }
    if (omSysExitReq != 0 && WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = FinishScene;
    }
}

s32 introSequenceAudioHandle = -1;
void UpdateSceneResults(OMOBJ *object)
{
    s32 playerIndex;
    s32 otherPlayerIndex;
    s32 finishCondition;
    s16 remainingPlayerCount;
    s32 playerRank;
    M438PlayerWork *playerWork;
    s32 activePlayerFound;

    switch (scenePhase) {
        case 6:
            if (sceneMessageSequenceId == -1) {
                sceneCountdownFrames = REFRESH_RATE * 60;
                sceneMessageSequenceId = MGSeqCreate(3, 0);
                MGSeqPosSet(sceneMessageSequenceId, 320.0f, 240.0f);
            }
            else {
                if (MGSeqStatGet(sceneMessageSequenceId) == 0) {
                    countdownSequenceId = MGSeqCreate(1, sceneCountdownFrames / REFRESH_RATE, -1, -1);
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        playerRankByIndex[playerIndex] = 0;
                    }
                    nextPlayerRank = 4;
                    sceneMessageSequenceId = -1;
                    scenePhase += 1;
                    scenePhaseTimer = 0;
                }
            }
            if (introSequenceAudioHandle == -1 && (MGSeqStatGet(sceneMessageSequenceId) & 0x10) != 0) {
                introSequenceAudioHandle = HuAudSeqPlay(0x49);
            }
            sceneEffectTimers[0] = REFRESH_RATE * 5;
            break;
        case 7:
            finishCondition = 0;
            sceneCountdownFrames--;
            MGSeqParamSet(countdownSequenceId, 1, (sceneCountdownFrames + REFRESH_RATE - 1) / REFRESH_RATE);
            if (sceneCountdownFrames == 0) {
                finishCondition++;
            }
            remainingPlayerCount = 0;
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                if (playerRankByIndex[playerIndex] == 0) {
                    remainingPlayerCount++;
                }
            }
            if (remainingPlayerCount < 2) {
                finishCondition++;
            }
            if (finishCondition != 0) {
                scenePhase += 1;
            }
            break;
        case 8:
            if (sceneEffectTimers[0] != 0) {
                sceneEffectTimers[0]--;
            }
            activePlayerFound = 0;
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                if (playerRankByIndex[playerIndex] != 0) {
                    playerWork = playerObjects[playerIndex]->data;
                    if (playerWork->isInactive == 0) {
                        activePlayerFound = 1;
                    }
                }
            }
            if (sceneMessageSequenceId == -1) {
                sceneMessageSequenceId = MGSeqCreate(3, 1);
                MGSeqPosSet(sceneMessageSequenceId, 320.0f, 240.0f);
                if (countdownSequenceId >= 0) {
                    MGSeqParamSet(countdownSequenceId, 2, -1);
                    countdownSequenceId = -1;
                }
                HuAudSeqFadeOut(introSequenceAudioHandle, REFRESH_RATE);
            }
            else if ((MGSeqStatGet(sceneMessageSequenceId) == 0) && (activePlayerFound == 0 || sceneEffectTimers[0] == 0)) {
                sceneMessageSequenceId = -1;
                scenePhaseTimer = REFRESH_RATE;
                scenePhase += 1;
            }
            break;
        case 9:
            if (--scenePhaseTimer == 0) {
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if (playerRankByIndex[playerIndex] == 0) {
                        break;
                    }
                }
                if (playerIndex < 4) {
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        playerRank = 0;
                        for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                            if (playerRankByIndex[otherPlayerIndex] < playerRankByIndex[playerIndex]) {
                                playerRank++;
                            }
                        }
                        GWPlayerCoinWinSet(playerIndex, playerRank);
                    }
                }
                else {
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        GWPlayerCoinWinSet(playerIndex, 3);
                    }
                }
                playerIndex = 0;
                otherPlayerIndex = 0;
                for (; playerIndex < 4; playerIndex++) {
                    if (playerRankByIndex[playerIndex] == 0) {
                        playerRankByIndex[otherPlayerIndex++] = GWPlayerCfg[playerIndex].character;
                    }
                }
                for (; otherPlayerIndex < 4; otherPlayerIndex++) {
                    playerRankByIndex[otherPlayerIndex] = -1;
                }
                sceneElapsedFrames = 0;
                scenePhaseTimer = REFRESH_RATE;
                scenePhase += 1;
            }
            break;
        case 10:
            sceneElapsedFrames++;
            if (sceneMessageSequenceId == -1) {
                if (playerRankByIndex[0] == -1) {
                    HuAudSStreamPlay(4);
                }
                else {
                    HuAudSStreamPlay(1);
                }
                sceneMessageSequenceId = 0;
            }
            else if (sceneElapsedFrames > REFRESH_RATE_F * 3.5f) {
                scenePhaseTimer = REFRESH_RATE * 3;
                sceneMessageSequenceId = -1;
                scenePhase += 1;
            }
            break;
        case 11:
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
            object->objFunc = FinishScene;
            break;
    }
    if (omSysExitReq != 0 && WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = FinishScene;
    }
}

void FinishScene(OMOBJ *object)
{
    if (WipeStatGet() == 0) {
        CharModelKill(-1);
        ShutdownFireManager();
        FreeAllSceneMemory();
        EmptyMapCallback();
        HuAudFXListnerKill();
        if (countdownSequenceId >= 0) {
            MGSeqKill(countdownSequenceId);
            countdownSequenceId = -1;
        }
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

Vec windPathPoints[11][8] = { {
                                 { -528.6f, 0.0f, 350.7f },
                                 { -375.7f, 0.0f, 243.0f },
                                 { -214.2f, 0.0f, 331.4f },
                                 { -173.3f, 0.0f, 523.7f },
                                 { 15.000001f, 0.0f, 505.7f },
                                 { 148.59999f, 0.0f, 393.0f },
                                 { 0.0f, 1.0f, 0.0f },
                             },
    {
        { -198.7f, 0.0f, -122.899994f },
        { -11.599999f, 0.0f, -177.1f },
        { 63.4f, 0.0f, -349.7f },
        { 277.09999f, 0.0f, -296.4f },
        { 397.5f, 0.0f, -470.2f },
        { 0.0f, 1.0f, 0.0f },
    },
    {
        { -421.0f, 0.0f, -471.3f },
        { -270.1f, 0.0f, -460.0f },
        { -153.1f, 0.0f, -356.5f },
        { 12.4f, 0.0f, -473.99997f },
        { 0.0f, 1.0f, 0.0f },
    },
    {
        { -545.10004f, 0.0f, -318.9f },
        { -469.0f, 0.0f, -154.1f },
        { -313.4, 0.0f, -170.09999f },
        { -250.0f, 0.0f, -32.8f },
        { 0.0f, 1.0f, 0.0f },
    },
    { { -155.6f, 0.0f, 19.5f }, { -34.3f, 0.0f, 96.6f }, { -63.9f, 0.0f, 240.00002f }, { 37.5f, 0.0f, 402.8f }, { 0.0f, 1.0f, 0.0f } },
    { { 207.5f, 0.0f, -163.2f }, { 270.1f, 0.0f, -104.799995f }, { 402.19998f, 0.0f, -102.4f }, { 511.49997f, 0.0f, -25.099998f },
        { 627.3f, 0.0f, -142.0f }, { 0.0f, 1.0f, 0.0f } },
    {
        { -601.0f, 0.0f, 97.799995f },
        { -516.80005f, 0.0f, 60.000004f },
        { -417.7f, 0.0f, 92.9f },
        { -309.40002f, 0.0f, 76.1f },
        { 0.0f, 1.0f, 0.0f },
    },
    { { 82.8f, 0.0f, 164.2f }, { 179.40001f, 0.0f, 179.8f }, { 241.00002f, 0.0f, 87.1f }, { 323.0f, 0.0f, 38.800003f }, { 0.0f, 1.0f, 0.0f } },
    { { 243.4f, 0.0f, 573.5f }, { 227.7f, 0.0f, 461.2f }, { 319.30002f, 0.0f, 400.6f }, { 342.30002f, 0.0f, 310.0f }, { 0.0f, 1.0f, 0.0f } },
    { { 357.69998f, 0.0f, 146.8f }, { 418.80002f, 0.0f, 225.0f }, { 503.2f, 0.0f, 234.69998f }, { 548.1f, 0.0f, 321.19998f }, { 0.0f, 1.0f, 0.0f } },
    { { -100.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -100.0f }, { 100.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } } };
void InitializeWindPaths(OMOBJ *object)
{
    M438WindEffectWork *pathSegment;
    s32 pathGroupIndex;
    s32 routePointIndex;
    s32 totalPathSegmentCount;

    object->objFunc = UpdateWindEffects;

    for (pathSegment = windEffectRecords, pathGroupIndex = 0, totalPathSegmentCount = 0; pathGroupIndex < 10; pathSegment++, pathGroupIndex++) {
        for (routePointIndex = 0; routePointIndex < 8; routePointIndex++) {
            if (windPathPoints[pathGroupIndex][routePointIndex + 1].y != 0.0f) {
                break;
            }
            pathSegment->effectState = 0;
            pathSegment->pathGroupIndex = pathGroupIndex;
            pathSegment->pathSegmentIndex = totalPathSegmentCount + 1;

            pathSegment->pathSegmentStart = windPathPoints[pathGroupIndex][routePointIndex];
            VECSubtract(&windPathPoints[pathGroupIndex][routePointIndex + 1], &windPathPoints[pathGroupIndex][routePointIndex], &pathSegment->pathSegmentDirection);
            pathSegment->pathSegmentLength = VECMag(&pathSegment->pathSegmentDirection);
            VECNormalize(&pathSegment->pathSegmentDirection, &pathSegment->pathSegmentDirection);
            pathSegment->primaryParticleSystemId = pathSegment->secondaryParticleSystemId = 0;
            pathSegment->countdownFrames = 0;
            pathSegment->riseHeight = 0.0f;
            pathSegment++;
            totalPathSegmentCount++;
        }
        pathSegment--;
        pathSegment->pathSegmentIndex = 0;
    }
    for (; totalPathSegmentCount < 48; totalPathSegmentCount++, pathSegment++) {
        pathSegment->pathGroupIndex = -1;
    }
}

void UpdateWindEffects(OMOBJ *object)
{
    s32 pathGroupCandidates[48];

    M438WindEffectWork *effectRecord;
    s32 recordIndex;
    s32 secondarySystemId;
    s32 effectRecordIndex;
    s32 effectCountToCreate;
    s32 effectsCreated;
    s32 selectedPathGroup;
    s32 effectDurationFrames;

    switch (scenePhase) {
        case 0:
        case 1:
        case 4:
        case 5:
            break;
        case 6:
            windEffectTimer = 0x1E;
            break;
        case 7:
            if (--windEffectTimer == 0) {
                ChooseRandomIndices(10, pathGroupCandidates);
                if (sceneCountdownFrames > REFRESH_RATE * 40) {
                    effectCountToCreate = 1;
                }
                else if (sceneCountdownFrames > REFRESH_RATE * 10) {
                    effectCountToCreate = 2;
                }
                else {
                    effectCountToCreate = 3;
                }
                for (effectRecord = windEffectRecords, recordIndex = 0; recordIndex < 0x30; recordIndex++, effectRecord++) {
                    if (effectRecord->pathGroupIndex == -1) {
                        break;
                    }
                    if (effectRecord->pathSegmentIndex == 0 && effectRecord->effectState) {
                        effectCountToCreate--;
                    }
                }
                for (effectRecordIndex = 0, effectsCreated = 0; effectsCreated < effectCountToCreate; effectRecordIndex++) {
                    selectedPathGroup = pathGroupCandidates[effectRecordIndex];
                    effectDurationFrames = (s32)(80.0f * ((1.0f / 65536.0f) * GetRandomU16())) + 0x3C;
                    for (effectRecord = windEffectRecords, recordIndex = 0; recordIndex < 0x30; recordIndex++, effectRecord++) {
                        if (effectRecord->pathGroupIndex == selectedPathGroup) {
                            if (effectRecord->effectState != 0) {
                                break;
                            }
                            effectRecord->effectState = 1;
                            effectRecord->stateTimer = effectDurationFrames;
                        }
                    }
                    if (recordIndex == 0x30) {
                        effectsCreated++;
                    }
                }
                windEffectTimer = 60;
            }
            break;
    }
    for (effectRecord = windEffectRecords, effectRecordIndex = 0; effectRecordIndex < 0x30; effectRecordIndex++, effectRecord++) {
        switch (effectRecord->effectState) {
            case 0:
                break;
            case 1:
                if (effectRecord->pathSegmentIndex == 0) {
                    HuAudFXPlay(0x6FB);
                }
                if (effectRecord->primaryParticleSystemId != 0) {
                    SelectParticleMaterial(effectRecord->primaryParticleSystemId, 0, 0);
                }
                else {
                    effectRecord->primaryParticleSystemId = createParticleSystem(1, 1);
                    LoadAnimationToTextureSlotDefaultSize(fireManager.activeParticleSystemId, 0, 0x450009);
                }
                EnableParticleSystemFlags(effectRecord->primaryParticleSystemId, 4);
                SetMaterialPosition(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, effectRecord->pathSegmentStart.x, effectRecord->pathSegmentStart.y, effectRecord->pathSegmentStart.z);
                SetMaterialRotation(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, 0.0f, (atan2d(effectRecord->pathSegmentDirection.x, effectRecord->pathSegmentDirection.z)) - 90.0, 0.0f);
                SetMaterialSecondaryScale(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, 32.0f, 32.0f, 0.0f);
                SetMaterialScale(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, effectRecord->pathSegmentLength / 64, 0.78125f, 1.0f);
                SetMaterialTextureCoordinateCount(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, 0);
                SetMaterialAnimationFlags(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, fireManager.activeTextureSlot, 0x21);
                SetMaterialAnimationSpeed(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, fireManager.activeTextureSlot, 0.5f);
                if (effectRecord->secondaryParticleSystemId == 0) {
                    effectRecord->secondaryParticleSystemId = createParticleSystem(0x64, 1);
                }
                secondarySystemId = effectRecord->secondaryParticleSystemId;
                ClearParticleSystemFlags(effectRecord->secondaryParticleSystemId, 4);
                LoadAnimationToTextureSlotDefaultSize(secondarySystemId, 0, 0x45000A);
                fireManager.particleSystems[secondarySystemId].updateCallback = UpdatePathDebrisParticles;
                SetParticleSystemPosition(secondarySystemId, effectRecord->pathSegmentStart.x, effectRecord->pathSegmentStart.y, effectRecord->pathSegmentStart.z);
                SetAllMaterialsEnabled(secondarySystemId, 1);
                fireManager.particleSystems[secondarySystemId].effectFlags = 0;
                fireManager.particleSystems[secondarySystemId].pathDirectionX = effectRecord->pathSegmentDirection.x;
                fireManager.particleSystems[secondarySystemId].pathDirectionZ = effectRecord->pathSegmentDirection.z;
                fireManager.particleSystems[secondarySystemId].pathLength = effectRecord->pathSegmentLength;
                fireManager.particleSystems[secondarySystemId].effectHeight = 10.0f;
                fireManager.particleSystems[secondarySystemId].effectScale = 0.02f;
                effectRecord->countdownFrames = 0x3C;
                effectRecord->particleScale = 0.2f;
                effectRecord->riseHeight = 10.0f;
                effectRecord->effectState++;
                break;
            case 2:
                effectRecord->particleScale += 0.015f;
                if (effectRecord->particleScale >= 0.6f) {
                    effectRecord->particleScale = 0.6f;
                }
                fireManager.particleSystems[effectRecord->secondaryParticleSystemId].effectScale = effectRecord->particleScale;
                if (scenePhase != 7) {
                    effectRecord->effectState = 5;
                }
                else {
                    if (--effectRecord->countdownFrames == 0) {
                        fireManager.particleSystems[effectRecord->secondaryParticleSystemId].effectTimer = 1;
                        ClearParticleSystemFlags(effectRecord->primaryParticleSystemId, 4);
                        fireManager.particleSystems[effectRecord->secondaryParticleSystemId].effectScale = 1.0f;
                        effectRecord->effectState++;
                    }
                }
                break;
            case 3:
                effectRecord->particleScale += 0.06f;
                if (effectRecord->particleScale >= 1.0f) {
                    effectRecord->particleScale = 1.0f;
                }
                fireManager.particleSystems[effectRecord->secondaryParticleSystemId].effectScale = effectRecord->particleScale;
                effectRecord->riseHeight += 7.5000005f;
                SetMaterialScale(effectRecord->primaryParticleSystemId, 0, effectRecord->pathSegmentLength / 64, effectRecord->riseHeight / 64, 1.0f);
                fireManager.particleSystems[effectRecord->secondaryParticleSystemId].effectHeight = effectRecord->riseHeight;
                if (effectRecord->riseHeight >= 200.0f) {
                    effectRecord->effectState++;
                }
                if (scenePhase != 7) {
                    effectRecord->effectState = 5;
                }
                break;
            case 4:
                if (scenePhase != 7) {
                    effectRecord->effectState = 5;
                }
                else {
                    if (--effectRecord->stateTimer == 0) {
                        effectRecord->effectState++;
                    }
                }
                break;
            case 5:
                effectRecord->riseHeight = effectRecord->riseHeight - 7.5000005f;
                SetMaterialScale(effectRecord->primaryParticleSystemId, 0, effectRecord->pathSegmentLength / 64, effectRecord->riseHeight / 64, 1.0f);
                fireManager.particleSystems[effectRecord->secondaryParticleSystemId].effectHeight = effectRecord->riseHeight;
                effectRecord->particleScale = effectRecord->particleScale - 0.03f;
                if (effectRecord->particleScale <= 0.0f) {
                    effectRecord->particleScale = 0.01f;
                }
                fireManager.particleSystems[effectRecord->secondaryParticleSystemId].effectScale = effectRecord->particleScale;
                if (!(effectRecord->riseHeight > 0.0f)) {
                    for (recordIndex = 0; recordIndex < 0x5A; recordIndex++) {
                        fireManager.particleSystems[effectRecord->secondaryParticleSystemId].materials[recordIndex].renderFlags = 0;
                    }
                    EnableParticleSystemFlags(effectRecord->primaryParticleSystemId, 4);
                    EnableParticleSystemFlags(effectRecord->secondaryParticleSystemId, 4);
                    effectRecord->effectState = 0;
                }
                break;
        }
    }
}

u8 CheckPlayerWindCollision(OMOBJ *object)
{
    Vec collisionPoint;
    Vec segmentDisplacement;
    float cameraTravelDistance;

    float segmentFraction;
    float closestDistanceSquared;

    M438WindEffectWork *effectRecord;
    M438CameraPositionWork *cameraWork;
    s32 effectIndex;

    cameraWork = object->data;
    cameraTravelDistance = VECMag(&cameraWork->cameraDisplacement);
    for (effectRecord = windEffectRecords, effectIndex = 0; effectIndex < 0x30; effectIndex++, effectRecord++) {
        if (effectRecord->effectState > 2 && effectRecord->effectState < 5) {
            segmentDisplacement.x = effectRecord->pathSegmentDirection.x * effectRecord->pathSegmentLength;
            segmentDisplacement.y = 0.0f;
            segmentDisplacement.z = effectRecord->pathSegmentDirection.z * effectRecord->pathSegmentLength;
            segmentFraction = IntersectSegments(&cameraWork->cameraPosition, &cameraWork->cameraDisplacement, &effectRecord->pathSegmentStart, &segmentDisplacement);
            if (segmentFraction <= 1.0 && segmentFraction > 0.0) {
                collisionPoint.x = cameraWork->cameraPosition.x + (segmentFraction * cameraWork->cameraDisplacement.x);
                segmentFraction = (collisionPoint.x - effectRecord->pathSegmentStart.x) / segmentDisplacement.x;
                if (segmentFraction > 1.0) {
                    continue;
                }
                if (!(segmentFraction < 0.0)) {
                    break;
                }
            }
            else {
                closestDistanceSquared = DistanceSquaredToSegment(&cameraWork->previousCameraPosition, &effectRecord->pathSegmentStart, &segmentDisplacement);
                if (!(closestDistanceSquared > 3600.0005f)) {
                    break;
                }
            }
        }
    }
    if (effectIndex == 0x30) {
        return 0;
    }
    return 1;
}

void UpdateCutsceneCamera(OMOBJ *object)
{
    M438CutsceneCameraWork *cameraWork;

    cameraWork = AllocateSceneMemory(0x190);
    object->data = cameraWork;
    cameraWork->transitionFramesRemaining = cameraWork->transitionDuration = 0.0f;
    cameraWork->zoomStep = 1.0f;
    cameraWork->cameraRotation.x = cameraWork->startRotation.x = -40.0f;
    cameraWork->cameraRotation.y = cameraWork->startRotation.y = 0.0f;
    cameraWork->cameraRotation.z = cameraWork->startRotation.z = 0.0f;
    cameraWork->cameraCenter.x = cameraWork->startCenter.x = 0.0f;
    cameraWork->cameraCenter.y = cameraWork->startCenter.y = -10.0f;
    cameraWork->cameraCenter.z = cameraWork->startCenter.z = 50.0f;
    cameraWork->currentZoom = cameraWork->startZoom = 2075.0f;
    cameraWork->shakeFramesRemaining[0] = cameraWork->shakeFramesRemaining[1] = cameraWork->shakeFramesRemaining[2] = 0;
    object->objFunc = EmptyPlayerCallback;
    HuAudFXListnerSetEX(&Hu3DCamera->pos, &Hu3DCamera->up, 10000.0f,
#if VERSION_PAL
        4280.0f,
#else
        4166.6665f,
#endif
        0.0f, 300.0f, 0.0f);
}

void EmptyPlayerCallback(OMOBJ *object)
{
    Vec cameraDirection;

    float transitionProgress;

    M438CutsceneCameraWork *cameraWork;
    s32 shakeAxis;

    cameraWork = object->data;
    if (cameraWork->transitionFramesRemaining > 0.0f) {
        cameraWork->transitionFramesRemaining = cameraWork->transitionFramesRemaining - cameraWork->zoomStep;
        transitionProgress = (cameraWork->transitionDuration - cameraWork->transitionFramesRemaining) / cameraWork->transitionDuration;
        transitionProgress = sind((90.0f * transitionProgress));
        if (cameraWork->pathMode == 0) {
            cameraWork->cameraCenter.x = cameraWork->startCenter.x + (transitionProgress * (cameraWork->endCenter.x - cameraWork->startCenter.x));
            cameraWork->cameraCenter.y = cameraWork->startCenter.y + (transitionProgress * (cameraWork->endCenter.y - cameraWork->startCenter.y));
            cameraWork->cameraCenter.z = cameraWork->startCenter.z + (transitionProgress * (cameraWork->endCenter.z - cameraWork->startCenter.z));
            cameraWork->cameraRotation.x = cameraWork->startRotation.x + (transitionProgress * (cameraWork->endRotation.x - cameraWork->startRotation.x));
            cameraWork->cameraRotation.y = cameraWork->startRotation.y + (transitionProgress * (cameraWork->endRotation.y - cameraWork->startRotation.y));
            cameraWork->cameraRotation.z = cameraWork->startRotation.z + (transitionProgress * (cameraWork->endRotation.z - cameraWork->startRotation.z));
            cameraWork->currentZoom = cameraWork->startZoom + (transitionProgress * (cameraWork->targetZoom - cameraWork->startZoom));
        }
        else {
            EvaluateVectorSpline(2.0f * transitionProgress, (float *)&cameraWork->cameraCenter, cameraWork->centerSplineValues, (float(*)[3]) & cameraWork->centerControlPoints, cameraWork->centerSplineCoefficients, 3);
            EvaluateVectorSpline(2.0f * transitionProgress, (float *)&cameraWork->cameraRotation, cameraWork->rotationSplineValues, (float(*)[3]) & cameraWork->rotationControlPoints, cameraWork->rotationSplineCoefficients, 3);
            cameraWork->currentZoom = EvaluateSpline(2.0f * transitionProgress, cameraWork->zoomKeyframeDistances, cameraWork->zoomKeyframes, cameraWork->zoomSplineCoefficients, 3);
        }
    }
    CRot.x = cameraWork->cameraRotation.x;
    CRot.y = cameraWork->cameraRotation.y;
    CRot.z = cameraWork->cameraRotation.z;
    Center.x = cameraWork->cameraCenter.x;
    Center.y = cameraWork->cameraCenter.y;
    Center.z = cameraWork->cameraCenter.z;
    CZoom = cameraWork->currentZoom;
    for (shakeAxis = 0; shakeAxis < 3; shakeAxis++) {
        cameraWork->shakeOffset[shakeAxis] = 0.0f;
        if (cameraWork->shakeFramesRemaining[shakeAxis] != 0) {
            cameraWork->shakeOffset[shakeAxis] = cameraWork->shakeAmplitude[shakeAxis] * sind(cameraWork->shakeAngle[shakeAxis]);
            cameraWork->shakeAngle[shakeAxis] += cameraWork->shakeAngularSpeed[shakeAxis];
            cameraWork->shakeAmplitude[shakeAxis] *= cameraWork->shakeDamping[shakeAxis];
            cameraWork->shakeFramesRemaining[shakeAxis]--;
        }
    }
    Center.x += cameraWork->shakeOffset[0];
    Center.y += cameraWork->shakeOffset[1];
    Center.z += cameraWork->shakeOffset[2];
    cameraDirection.x = -(sind(CRot.y) * cosd(CRot.x));
    cameraDirection.y = sind(CRot.x);
    cameraDirection.z = -(cosd(CRot.y) * cosd(CRot.x));
    HuAudFXListnerUpdate(&Hu3DCamera->pos, &cameraDirection);
}

void SetCameraView(float transitionDuration, Vec *targetCenter, Vec *targetRotation, float targetZoom)
{
    M438CutsceneCameraWork *cameraWork;

    cameraWork = cutsceneControllerObject->data;
    cameraWork->pathMode = 0;
    cameraWork->transitionDuration = transitionDuration;
    cameraWork->transitionFramesRemaining = transitionDuration;
    cameraWork->startCenter = cameraWork->cameraCenter;
    cameraWork->endCenter.x = targetCenter->x;
    cameraWork->endCenter.y = targetCenter->y;
    cameraWork->endCenter.z = targetCenter->z;
    cameraWork->startRotation = cameraWork->cameraRotation;
    cameraWork->endRotation.x = targetRotation->x;
    cameraWork->endRotation.y = targetRotation->y;
    cameraWork->endRotation.z = targetRotation->z;
    cameraWork->startZoom = cameraWork->currentZoom;
    cameraWork->targetZoom = targetZoom;
}

void SetCameraSplineView(float transitionDuration, Vec *targetCenter, Vec *targetRotation, float targetZoom, Vec *centerControlPoint, Vec *rotationControlPoint, float controlZoom)
{
    float firstZoomSegmentLength;
    float secondZoomSegmentLength;

    M438CutsceneCameraWork *cameraWork;

    cameraWork = cutsceneControllerObject->data;
    cameraWork->pathMode = 1;
    cameraWork->transitionFramesRemaining = cameraWork->transitionDuration = transitionDuration;
    cameraWork->startCenter = cameraWork->cameraCenter;

    cameraWork->endCenter.x = targetCenter->x;
    cameraWork->endCenter.y = targetCenter->y;
    cameraWork->endCenter.z = targetCenter->z;

    cameraWork->controlCenter.x = centerControlPoint->x;
    cameraWork->controlCenter.y = centerControlPoint->y;
    cameraWork->controlCenter.z = centerControlPoint->z;

    cameraWork->startRotation = cameraWork->cameraRotation;

    cameraWork->endRotation.x = targetRotation->x;
    cameraWork->endRotation.y = targetRotation->y;
    cameraWork->endRotation.z = targetRotation->z;

    cameraWork->controlRotation.x = rotationControlPoint->x;
    cameraWork->controlRotation.y = rotationControlPoint->y;
    cameraWork->controlRotation.z = rotationControlPoint->z;

    cameraWork->startZoom = cameraWork->currentZoom;
    cameraWork->targetZoom = targetZoom;
    cameraWork->controlZoom = controlZoom;
    cameraWork->centerControlPoints[0] = cameraWork->startCenter;
    cameraWork->centerControlPoints[1] = cameraWork->controlCenter;
    cameraWork->centerControlPoints[2] = cameraWork->endCenter;
    cameraWork->rotationControlPoints[0] = cameraWork->startRotation;
    cameraWork->rotationControlPoints[1] = cameraWork->controlRotation;
    cameraWork->rotationControlPoints[2] = cameraWork->endRotation;

    cameraWork->zoomKeyframes[0] = cameraWork->startZoom;
    cameraWork->zoomKeyframes[1] = cameraWork->controlZoom;
    cameraWork->zoomKeyframes[2] = cameraWork->targetZoom;

    cameraWork->zoomKeyframeDistances[0] = 0.0f;
    if ((cameraWork->zoomKeyframes[1] - cameraWork->zoomKeyframes[0]) < 0.0f) {
        firstZoomSegmentLength = -(cameraWork->zoomKeyframes[1] - cameraWork->zoomKeyframes[0]);
    }
    else {
        firstZoomSegmentLength = cameraWork->zoomKeyframes[1] - cameraWork->zoomKeyframes[0];
    }
    cameraWork->zoomKeyframeDistances[1] = firstZoomSegmentLength;
    if ((cameraWork->zoomKeyframes[2] - cameraWork->zoomKeyframes[1]) < 0.0f) {
        secondZoomSegmentLength = -(cameraWork->zoomKeyframes[2] - cameraWork->zoomKeyframes[1]);
    }
    else {
        secondZoomSegmentLength = cameraWork->zoomKeyframes[2] - cameraWork->zoomKeyframes[1];
    }
    cameraWork->zoomKeyframeDistances[2] = cameraWork->zoomKeyframeDistances[1] + secondZoomSegmentLength;
    BuildSplineCoefficients(cameraWork->centerSplineValues, cameraWork->centerControlPoints, cameraWork->centerSplineCoefficients, 3);
    BuildSplineCoefficients(cameraWork->rotationSplineValues, cameraWork->rotationControlPoints, cameraWork->rotationSplineCoefficients, 3);
    SolveSplineCoefficients(cameraWork->zoomKeyframeDistances, cameraWork->zoomKeyframes, cameraWork->zoomSplineCoefficients, 3);
}

void UpdateCameraView(float zoomStep)
{
    M438CutsceneCameraWork *cameraWork;

    cameraWork = cutsceneControllerObject->data;
    cameraWork->zoomStep = zoomStep;
}

void ConfigureCameraShake(u8 shakeIndex, u16 durationFrames, float amplitude, float damping, float angularSpeed)
{
    M438CutsceneCameraWork *cameraWork = cutsceneControllerObject->data;

    cameraWork->shakeFramesRemaining[shakeIndex] = durationFrames;
    cameraWork->shakeAmplitude[shakeIndex] = amplitude;
    cameraWork->shakeDamping[shakeIndex] = damping;
    cameraWork->shakeAngularSpeed[shakeIndex] = angularSpeed;
    cameraWork->shakeAngle[shakeIndex] = 90.0f;
}

s32 characterMotionIds[11] = { 0, 2, 3, 8, 0xA, 9, 0x13, 0xD, 0x10, 0x17, 0x18 };
Vec playerStartingPositions[4] = {
    { -400.0f, 1600.0f, -400.0f },
    { 400.0f, 1600.0f, -400.0f },
    { -400.0f, 1600.0f, 400.0f },
    { 400.0f, 1600.0f, 400.0f },
};
void InitializePlayer(OMOBJ *object)
{
    M438PlayerWork *playerWork;
    s32 motionIndex;
    u32 startPositionIndex;
    s32 modelId;

    float pathOffset[4] = { 0.25f, 0.5f, 0.75f, 1.0f };
    playerWork = AllocateSceneMemory(sizeof(M438PlayerWork));
    object->data = playerWork;
    memset(playerWork, 0, sizeof(M438PlayerWork));
    playerWork->playerIndex = object->work[0];
    playerWork->characterId = GWPlayerCfg[playerWork->playerIndex].character;
    playerWork->teamIndex = playerWork->playerIndex >> 1;
    playerWork->playerTeamSlot = playerWork->playerIndex & 1;
    playerWork->controllerPort = GWPlayerCfg[playerWork->playerIndex].padIdx;
    playerWork->isComputerControlled = GWPlayerCfg[playerWork->playerIndex].iscom;
    playerWork->difficultyLevel = GWPlayerCfg[playerWork->playerIndex].diff;
    playerWork->currentMotionIndex = playerWork->previousMotionIndex = 0;
    playerWork->difficultyMovementScale = pathOffset[playerWork->difficultyLevel];
    playerWork->idleDelayFrames = (45.0f * REFRESH_RATE_F * (playerWork->difficultyMovementScale - 0.075f)) + (REFRESH_RATE_F / 100.0f * frandmod(0x3E8));
    if (playerWork->idleDelayFrames > REFRESH_RATE * 1000) {
        playerWork->idleDelayFrames = REFRESH_RATE * 3;
    }
    playerWork->windCycleFrame = frandmod(REFRESH_RATE * 3);
    playerWork->playerStateFlags = 1;
    playerWork->groundContactFlag = 1;
    playerWork->landingFlag = 0;
    playerWork->modelVisibilityTimer = 0;
    playerWork->reservedED = 0;
    playerWork->reservedEC = 0;
    playerWork->unreferencedPlayerByte = 0;
    playerWork->currentInputState = 0;
    startPositionIndex = object->work[1];
    playerWork->worldPosition.x = playerStartingPositions[startPositionIndex].x;
    playerWork->worldPosition.y = playerStartingPositions[startPositionIndex].y + 100.0f * ((1.0f / 32768.0f) * (GetRandomU16() - 0x8000));
    playerWork->worldPosition.z = playerStartingPositions[startPositionIndex].z;
    playerWork->velocityX = playerWork->verticalVelocity = playerWork->velocityZ = 0.0f;
    playerWork->modelRotationX = playerWork->modelRotationY = playerWork->modelRotationZ = 0.0f;
    playerWork->facingAngle = playerWork->modelRotationY;
    playerWork->walkCyclePhase = 0.0f;
    playerWork->movementSpeed = 0.0f;
    playerWork->currentMotionDuration = 1.0f;
    memset(playerWork->reservedF0, 0, sizeof(playerWork->reservedF0));
    modelId = CharModelCreate(playerWork->characterId, 8);
    object->mdlId[0] = modelId;
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelId);
    CharModelStepFxSet(playerWork->characterId, 0);
    playerWork->characterEffectHandle = -1;
    playerWork->impactSoundHandle = -1;
    for (motionIndex = 0; motionIndex < 11; motionIndex++) {
        object->mtnId[motionIndex] = CharMotionCreate(playerWork->characterId, characterMotionIds[motionIndex]);
    }
    CharMotionVoiceOnSet(playerWork->characterId, object->mtnId[3], 0);
    CharMotionDataClose(playerWork->characterId);
    CharMotionSet(playerWork->characterId, object->mtnId[playerWork->currentMotionIndex]);
    object->objFunc = UpdatePlayerIntro;
}

void UpdatePlayerIntro(OMOBJ *object)
{
    s32 modelId;
    M438PlayerWork *playerWork;

    modelId = object->mdlId[0];
    playerWork = object->data;
    playerWork->stickX = playerWork->stickY = 0;
    playerWork->pressedButtons = playerWork->heldButtons = 0;
    switch (scenePhase) {
        case 1:
        case 4:
            break;
        case 3:
            playerWork->currentMotionIndex = 3;
            playerWork->verticalVelocity = 0.0f;
            playerWork->groundContactFlag = 0;
            break;
        case 6:
            object->objFunc = UpdatePlayerObjects;
            break;
    }
    if (playerWork->worldPosition.y <= 0.0f) {
        playerWork->worldPosition.y = 0.0f;
        playerWork->groundContactFlag = 1;
    }
    UpdatePlayerMotion(object);
}

void UpdatePlayerObjects(OMOBJ *object)
{
    Vec groundPlanePosition;
    Vec viewportPosition;

    float groundDistance;
    float minimumHeight;
    float movementDirection;

    s32 modelId = object->mdlId[0];
    M438PlayerWork *playerWork = object->data;
    if (playerWork->playerStateFlags != 0) {
        if (scenePhase == 7 && (playerWork->playerStateFlags & 2) == 0) {
            if (playerWork->isComputerControlled != 0) {
                UpdatePlayerWindMotion(object);
            }
            else {
                playerWork->stickX = HuPadStkX[playerWork->controllerPort];
                playerWork->stickY = HuPadStkY[playerWork->controllerPort];
                playerWork->pressedButtons = HuPadBtnDown[playerWork->controllerPort];
                playerWork->heldButtons = HuPadBtn[playerWork->controllerPort];
            }
        }
        else {
            playerWork->stickX = playerWork->stickY = 0;
            playerWork->pressedButtons = playerWork->heldButtons = 0;
        }
        switch (scenePhase) {
            case 6:
                playerWork->unreferencedWindParameterA = playerWork->unreferencedWindParameterB = playerWork->unreferencedWindParameterC = 0.0f;
                break;
            case 7:
                if (CheckPlayerWindCollision(object) && (playerWork->playerStateFlags & 2) == 0) {
                    omVibrate(playerWork->playerIndex, 0xC, 6, 6);
                    playerWork->currentMotionIndex = 6;
                    playerWork->playerStateFlags = playerWork->playerStateFlags | 2;
                    playerWork->groundContactFlag = 0;
                    playerWork->facingAngle = atan2d(playerWork->worldPosition.x, playerWork->worldPosition.z);
                    playerWork->verticalVelocity = 1347.5f / REFRESH_RATE_F;
                    playerRankByIndex[playerWork->playerIndex] = nextPlayerRank--;
                }
                break;
            case 8:
            case 9:
                playerWork->modelRotationY = ApproachAngleSmoothly(playerWork->modelRotationY, 0.0f, 0.9f);
                break;
            case 10:
                if ((playerWork->playerStateFlags & 2) == 0) {
                    playerWork->currentMotionIndex = 9;
                }
                break;
        }
        if (playerWork->currentMotionIndex != 7) {
            groundPlanePosition = playerWork->worldPosition;
            groundPlanePosition.y = 0.0f;
            groundDistance = VECMag(&groundPlanePosition);
            if (groundDistance > 700.0f) {
                minimumHeight = -100.0f;
            }
            else {
                minimumHeight = 0.0f;
            }
            if (playerWork->worldPosition.y < minimumHeight) {
                playerWork->worldPosition.y = minimumHeight;
                playerWork->groundContactFlag = 1;
            }
            if (((playerWork->playerStateFlags & 2) == 0) && (groundDistance > 675.0f)) {
                movementDirection = atan2d(groundPlanePosition.x, groundPlanePosition.z);
                if (findAvailableRockAtAngle(movementDirection) != 0) {
                    NormalizeVector(&groundPlanePosition);
                    playerWork->worldPosition.x = 675.0f * groundPlanePosition.x;
                    playerWork->worldPosition.z = 675.0f * groundPlanePosition.z;
                }
                else if (groundDistance > 700.0f) {
                    omVibrate(playerWork->playerIndex, 0xC, 6, 6);
                    playerWork->playerStateFlags = playerWork->playerStateFlags | 2;
                    playerWork->currentMotionIndex = 6;
                    playerWork->groundContactFlag = 0;
                    playerWork->facingAngle = movementDirection;
                    playerWork->verticalVelocity = 1347.5f / REFRESH_RATE_F;
                    playerRankByIndex[playerWork->playerIndex] = nextPlayerRank--;
                }
            }
        }
        UpdatePlayerMotion(object);
        if ((playerWork->playerStateFlags & 2) != 0 && playerWork->isInactive == 0) {
            groundPlanePosition = playerWork->worldPosition;
            groundPlanePosition.y = 0.0f;
            if (groundPlanePosition.z < 0.0f) {
                groundPlanePosition.z *= 0.25f;
            }
            groundDistance = VECMag(&groundPlanePosition);
            Hu3D3Dto2D(&playerWork->worldPosition, 1, &viewportPosition);
            viewportPosition.x = viewportPosition.x - 288.0f;
            viewportPosition.y -= 240.0f;
            if (fabs(viewportPosition.x) > 308.0 || viewportPosition.y < -260.0f || viewportPosition.y > 290.0f || playerWork->worldPosition.y < -300.0f || groundDistance > 1400.0f) {
                playerWork->isInactive = 1;
            }
        }
        if (playerWork->isInactive != 0) {
            playerWork->playerStateFlags = 0;
            Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
            object->objFunc = EmptySceneCallback;
        }
    }
}

void EmptySceneCallback(OMOBJ *object) { }

void UpdatePlayerMotion(OMOBJ *object)
{
    Vec idleTargetOffset;
    float unreferencedIdleScratch;
    s16 pressedButtons;
    s16 heldButtons;

    float movementAngle;
    float idleTargetAngle;
    float unreferencedAngleScratch;

    M438PlayerWork *playerWork;
    s16 stickX;
    s16 stickY;
    s16 nextMotionIndex;
    s16 applyMotionShift;
    s32 characterModelId;
    s32 particleSystemId;
    M438CharacterCameraWork *cameraWork;

    cameraWork = cameraControllerObject->data;
    playerWork = object->data;
    characterModelId = object->mdlId[0];
    nextMotionIndex = playerWork->currentMotionIndex;
    playerWork->previousWorldPosition = playerWork->worldPosition;
    stickX = playerWork->stickX;
    stickY = playerWork->stickY;
    pressedButtons = playerWork->pressedButtons;
    heldButtons = playerWork->heldButtons;
    switch (playerWork->currentMotionIndex) {
        case 0:
        case 1:
        case 2:
            playerWork->movementSpeed = 0.14f * sqrtf((stickX * stickX) + (stickY * stickY));
            if (playerWork->movementSpeed > 0.1f) {
                if (playerWork->movementSpeed >= 6.0f) {
                    nextMotionIndex = 2;
                    applyMotionShift = 1;
                }
                else {
                    nextMotionIndex = 1;
                    applyMotionShift = 1;
                }
                idleTargetAngle = atan2d(stickX, -stickY);
                if (playerWork->isComputerControlled != 0) {
                    playerWork->modelRotationY = approachAngle(playerWork->modelRotationY, idleTargetAngle, 0.15f);
                }
                else {
                    playerWork->modelRotationY = approachAngle(playerWork->modelRotationY, idleTargetAngle, 0.2f);
                }
                playerWork->facingAngle = ApproachAngleSmoothly(playerWork->facingAngle, idleTargetAngle, 0.5f);
                playerWork->walkCyclePhase += 1.0f;
                if (playerWork->walkCyclePhase >= (0.5f * playerWork->currentMotionDuration)) {
                    playerWork->walkCyclePhase = playerWork->walkCyclePhase - (0.5f * playerWork->currentMotionDuration);
                }
                playerWork->movementSpeed = playerWork->movementSpeed * (1.0 + (0.25 * cosd((90.0f * (playerWork->walkCyclePhase / (0.5f * playerWork->currentMotionDuration))))));
                playerWork->idleFrameCount = 0;
                playerWork->idleFrameLimit = REFRESH_RATE_F * (0.3f + (0.00040000002f * frandmod(0x3E8)));
            }
            else {
                playerWork->walkCyclePhase = 0.0f;
                playerWork->movementSpeed = 0.0f;
                nextMotionIndex = 0;
                applyMotionShift = 1;
                if (scenePhase == 7) {
                    playerWork->idleFrameCount++;
                    if (playerWork->idleFrameCount > playerWork->idleFrameLimit) {
                        VECSubtract(&cameraWork->targetPosition, &playerWork->worldPosition, &idleTargetOffset);
                        unreferencedAngleScratch = atan2d(idleTargetOffset.x, idleTargetOffset.z);
                        if (fabs(angleDifference(playerWork->modelRotationY, unreferencedAngleScratch)) > 10.0) {
                            nextMotionIndex = 1;
                        }
                        playerWork->modelRotationY = approachAngle(playerWork->modelRotationY, unreferencedAngleScratch, 0.15f);
                    }
                }
            }
            playerWork->verticalVelocity = -122.5f / REFRESH_RATE_F;
            break;
        case 3:
            applyMotionShift = 0;
            if (Hu3DData[characterModelId].motIdShift == -1 && CharMotionEndCheck(playerWork->characterId)) {
                playerWork->verticalVelocity += -122.5f / REFRESH_RATE_F;
            }
            else {
                playerWork->verticalVelocity = 0.0f;
            }
            if (playerWork->groundContactFlag != 0) {
                if (playerWork->landingFlag != 0) {
                    playerWork->verticalVelocity = 0.0f;
                    playerWork->landingFlag = 0;
                    nextMotionIndex = 5;
                    applyMotionShift = 0;
                }
                else {
                    playerWork->verticalVelocity *= -0.1f;
                    playerWork->groundContactFlag = 0;
                    nextMotionIndex = 4;
                    applyMotionShift = 0;
                }
            }
            break;
        case 4:
            playerWork->verticalVelocity += -122.5f / REFRESH_RATE_F;
            if (playerWork->groundContactFlag != 0 && CharMotionEndCheck(playerWork->characterId)) {
                nextMotionIndex = 5;
                applyMotionShift = 0;
            }
            break;
        case 5:
            if (CharMotionEndCheck(playerWork->characterId) != 0) {
                nextMotionIndex = 0;
                applyMotionShift = 1;
            }
            break;
        case 6:
            applyMotionShift = 1;
            playerWork->movementSpeed = 12.0f;
            playerWork->verticalVelocity += -122.5f / REFRESH_RATE_F;
            playerWork->modelRotationY = ApproachAngleSmoothly(playerWork->modelRotationY, playerWork->facingAngle, 0.5f);
            if (playerWork->characterEffectHandle < 0) {
                playerWork->characterEffectHandle = CharFXPlayPos(playerWork->characterId, 0x123, &playerWork->worldPosition);
            }
            if (playerWork->groundContactFlag != 0) {
                playerWork->groundContactFlag = 0;
                playerWork->verticalVelocity = 1347.5f / REFRESH_RATE_F;
                if (playerWork->playerStateFlags != 0 && playerWork->isInactive == 0) {
                    CharFXPlayPos(playerWork->characterId, 0x11C, &playerWork->worldPosition);
                }
            }
            if ((GetRandomU16() & 3) == 0) {
                particleSystemId = createParticleSystem(1, 1);
                LoadAnimationToTextureSlotDefaultSize(fireManager.activeParticleSystemId, 0, 0x45000A);
                fireManager.particleSystems[particleSystemId].updateCallback = UpdateFootstepParticles;
                SetMaterialPosition(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, playerWork->worldPosition.x, playerWork->worldPosition.y, playerWork->worldPosition.z);
                SetMaterialTextureRotation(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, sind(playerWork->facingAngle), 0.0f, cosd(playerWork->facingAngle));
                SetMaterialScale(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, 1.0f, 1.0f, 1.0f);
                EnableMaterialFlags(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, 0x80000004);
                SetMaterialColor(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, 0, 0, 0, 0xFF);
                SetMaterialAnimationFlags(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, fireManager.activeTextureSlot, 0x11);
                SetMaterialAnimationSpeed(fireManager.activeParticleSystemId, fireManager.activeMaterialIndex, fireManager.activeTextureSlot, 0.5f);
                DestroyParticleSystem(particleSystemId);
            }
            break;
        case 7:
            if (playerWork->characterEffectHandle < 0) {
                playerWork->characterEffectHandle = CharFXPlayPos(playerWork->characterId, 0x123, &playerWork->worldPosition);
            }
            if (playerWork->impactSoundHandle < 0) {
                playerWork->impactSoundHandle = HuAudFXPlay(0x701);
            }
            applyMotionShift = 1;
            playerWork->movementSpeed += 2.0;
            playerWork->verticalVelocity += -122.5f / REFRESH_RATE_F;
            break;
        case 9:
        case 10:
            applyMotionShift = 0;
            playerWork->verticalVelocity = -122.5f / REFRESH_RATE_F;
            break;
    }
    if ((playerWork->playerStateFlags & 4) != 0) {
        if ((playerWork->modelVisibilityTimer & 2) != 0) {
            Hu3DModelAttrSet(characterModelId, HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrReset(characterModelId, HU3D_ATTR_DISPOFF);
        }
        if (--playerWork->modelVisibilityTimer == 0) {
            playerWork->playerStateFlags = playerWork->playerStateFlags & 0xFFFFFFFB;
        }
    }
    playerWork->velocityX = playerWork->movementSpeed * sind(playerWork->facingAngle);
    playerWork->velocityZ = playerWork->movementSpeed * cosd(playerWork->facingAngle);
    playerWork->worldPosition.x += playerWork->velocityX;
    if (playerWork->groundContactFlag == 0) {
        playerWork->worldPosition.y += playerWork->verticalVelocity;
    }
    playerWork->worldPosition.z += playerWork->velocityZ;
    if (nextMotionIndex != playerWork->previousMotionIndex) {
        if (playerWork->currentMotionIndex == 7 && playerWork->specialMotionState != 0) {
            playerWork->previousMotionIndex = nextMotionIndex;
            playerWork->currentMotionIndex = nextMotionIndex;
            CharMotionShiftSet(playerWork->characterId, object->mtnId[8], 0.0f, 8.0f, applyMotionShift);
        }
        else {
            switch (nextMotionIndex) {
                case 0:
                case 1:
                case 2:
                    if (playerWork->currentMotionIndex != 0 && CharMotionShiftIDGet(playerWork->characterId) >= 0)
                        break;
                default:
                    playerWork->previousMotionIndex = nextMotionIndex;
                    playerWork->currentMotionIndex = nextMotionIndex;
                    CharMotionShiftSet(playerWork->characterId, object->mtnId[playerWork->currentMotionIndex], 0.0f, 8.0f, applyMotionShift);
            }
        }
        playerWork->currentMotionDuration = CharMotionMaxTimeGet(playerWork->characterId);
    }
    omSetTra(object, playerWork->worldPosition.x, playerWork->worldPosition.y, playerWork->worldPosition.z);
    omSetRot(object, playerWork->modelRotationX, playerWork->modelRotationY, playerWork->modelRotationZ);
    if (playerWork->characterEffectHandle >= 0) {
        HuAudFXEmiterUpDate(playerWork->characterEffectHandle, &playerWork->worldPosition);
    }
}

void UpdatePlayerWindMotion(OMOBJ *object)
{
    Vec windTargetOffset;
    Vec windDisplacement;
    float unreferencedWindScratch;
    s32 unreferencedWindFlag;

    float windOscillation;
    float windDistance;
    float windInfluence;

    M438PlayerWork *playerWork;
    s16 windStickX;
    s16 windStickY;
    s16 pressedButtons;
    s16 heldButtons;

    unreferencedWindFlag = 1;
    playerWork = object->data;
    windStickX = windStickY = pressedButtons = heldButtons = 0;
    unreferencedWindScratch = windInfluence = 0.0f;
    if (playerWork->idleDelayFrames != 0) {
        playerWork->idleDelayFrames--;
    }
    if (playerWork->idleDelayFrames < REFRESH_RATE * 10) {
        windOscillation = 0.1f / REFRESH_RATE_F * (REFRESH_RATE * 10 - playerWork->idleDelayFrames);
        windOscillation *= 0.6f + (0.4f * (1.0f - playerWork->difficultyMovementScale));
        playerWork->windForceMagnitude = 200.0f * windOscillation;
    }
    playerWork->windCycleFrame++;
    if (playerWork->windCycleFrame > REFRESH_RATE * 3) {
        playerWork->windCycleFrame -= REFRESH_RATE * 3;
    }
    if (playerWork->windActive) {
        VECSubtract(&playerWork->windTargetPosition, &playerWork->windVelocity, &windTargetOffset);
        VECSubtract(&windTargetOffset, &playerWork->worldPosition, &windDisplacement);
        windDisplacement.y = 0.0f;
        windDistance = VECMag(&windDisplacement);
        if (windDistance > 10.0f) {
            if (windDistance > 20.0f) {
                windDistance = 20.0f;
            }
            windDistance *= 0.05f;
            windOscillation = atan2d(windDisplacement.x, windDisplacement.z);
            playerWork->windDirection = windOscillation;
            windInfluence = windDistance;
            windOscillation = 120.0f / REFRESH_RATE_F * playerWork->windCycleFrame;
            playerWork->windVelocity.x = playerWork->windForceMagnitude * sind(windOscillation);
            playerWork->windVelocity.z = playerWork->windForceMagnitude * cosd(windOscillation);
        }
    }
    playerWork->windStrength += 0.25f * (windInfluence - playerWork->windStrength);
    if (playerWork->windStrength > 0.2f) {
        windStickX = 80.0 * (playerWork->windStrength * sind(playerWork->windDirection));
        windStickY = 80.0 * (playerWork->windStrength * -cosd(playerWork->windDirection));
    }
    playerWork->stickX = windStickX;
    playerWork->stickY = windStickY;
    playerWork->pressedButtons = pressedButtons;
    playerWork->heldButtons = heldButtons;
    playerWork->windUpdatePending = 1;
    playerWork->windActive = 0;
}

void InitializeCharacterCamera(OMOBJ *object)
{
    M438CharacterCameraWork *cameraWork;

    s32 modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M438, 3));
    object->mdlId[0] = modelId;
    Hu3DModelReflectTypeSet(modelId, 4);
    object->mtnId[0] = Hu3DJointMotionFile(modelId, DATA_MAKE_NUM(DATADIR_M438, 5));
    object->mtnId[1] = Hu3DJointMotionFile(modelId, DATA_MAKE_NUM(DATADIR_M438, 4));
    object->mtnId[2] = Hu3DJointMotionFile(modelId, DATA_MAKE_NUM(DATADIR_M438, 6));
    object->mtnId[3] = Hu3DJointMotionFile(modelId, DATA_MAKE_NUM(DATADIR_M438, 7));
    Hu3DModelScaleSet(modelId, 1.5f, 1.5f, 1.5f);
    Hu3DMotionSet(modelId, object->mtnId[0]);
    Hu3DMotionSpeedSet(modelId, 2.0f);
    Hu3DModelAttrSet(modelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(modelId);
    cameraWork = AllocateSceneMemory(sizeof(M438CharacterCameraWork));
    object->data = cameraWork;
    memset(cameraWork, 0, sizeof(M438CharacterCameraWork));
    cameraWork->worldPosition.x = 0.0f;
    cameraWork->worldPosition.y = -450.0f;
    cameraWork->worldPosition.z = -1550.0f;
    cameraWork->modelRotation.x = cameraWork->modelRotation.y = cameraWork->modelRotation.z = 0.0f;
    cameraWork->cameraOffset.y = 35.0f;
    cameraWork->cameraAngle = 0.0f;
    cameraWork->cameraMode = 0;
    omSetTra(object, cameraWork->worldPosition.x, cameraWork->worldPosition.y, cameraWork->worldPosition.z);
    omSetRot(object, cameraWork->modelRotation.x, cameraWork->modelRotation.y, cameraWork->modelRotation.z);
    omSetSca(object, 1.25f, 1.25f, 1.25f);
    object->objFunc = UpdateCharacterCamera;
}

void UpdateCharacterCamera(OMOBJ *object)
{
    Vec cameraTargetPosition;
    float targetZoom;
    s32 playerIndex;
    M438CutsceneCameraWork *cameraWork;
    M438CutsceneCameraWork *cutsceneWork;

    cutsceneWork = object->data;
    switch (scenePhase) {
        case 1:
            if (activeFireSystemId == 0) {
                activeFireSystemId = createParticleSystem(0x90, 1);
                LoadAnimationToTextureSlotDefaultSize(activeFireSystemId, 0, 0x45000A);
                fireManager.particleSystems[activeFireSystemId].updateCallback = UpdateImpactParticles;
                SetParticleSystemPosition(activeFireSystemId, cutsceneWork->cameraRotation.x, -80.0f, cutsceneWork->cameraRotation.z);
                EnableAllMaterialFlags(activeFireSystemId, 4);
                SetAllMaterialsEnabled(activeFireSystemId, 1);
                fireManager.particleSystems[activeFireSystemId].effectFlags = 0;
            }
            break;
        case 2:
            cutsceneWork->cameraRotation.y += cutsceneWork->startRotation.y;
            cutsceneWork->startRotation.y -= -122.5f / REFRESH_RATE_F;
            if (cutsceneWork->cameraRotation.y >= -350.0f) {
                HuAudFXPlay(0x6FA);
                fireManager.particleSystems[activeFireSystemId].effectFlags = 1;
            }
            if (cutsceneWork->cameraRotation.y > 0.0f) {
                HuAudFXEmiterPlay(0x700, &cutsceneWork->cameraRotation);
                scenePhaseTimer = REFRESH_RATE * 2;
                scenePhase += 1;
                cutsceneWork->startRotation.z = 20.0f;
            }
            break;
        case 3:
            cutsceneWork->cameraRotation.y += cutsceneWork->startRotation.y;
            cutsceneWork->cameraRotation.z += cutsceneWork->startRotation.z;
            cutsceneWork->startRotation.y += -122.5f / REFRESH_RATE_F;
            if (cutsceneWork->cameraRotation.y < 0.0f) {
                cutsceneWork->startRotation.z -= 0.5f;
                if (cutsceneWork->startRotation.z < 0.0f) {
                    cutsceneWork->startRotation.z = 0.0f;
                }
                cutsceneWork->cameraRotation.y = 0.0f;
                if (secondaryFireSystemSetupCount == 0) {
                    HuAudFXPlay(0x6FE);
                    ConfigureCameraShake(1, REFRESH_RATE, 25.0f, 0.95f, 90.0f);
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        omVibrate(playerIndex, REFRESH_RATE * 0.9, 0xC, 0);
                    }
                    activeFireSystemId = createParticleSystem(0x3C, 1);
                    LoadAnimationToTextureSlotDefaultSize(activeFireSystemId, 0, 0x45000F);
                    fireManager.particleSystems[activeFireSystemId].updateCallback = UpdateWindTrailParticles;
                    SetParticleSystemPosition(activeFireSystemId, cutsceneWork->cameraRotation.x, 50.0f + cutsceneWork->cameraRotation.y, cutsceneWork->cameraRotation.z);
                    SetAllMaterialsEnabled(activeFireSystemId, 1);
                    secondaryFireSystemSetupCount++;
                }
            }
            cameraTargetPosition = cutsceneWork->cameraRotation;
            cameraTargetPosition.y += 120.00001f;
            cameraTargetPosition.z -= 300.0f;
            targetZoom = CZoom;
            cameraWork = cutsceneControllerObject->data;
            cameraWork->pathMode = 0;
            cameraWork->transitionFramesRemaining = cameraWork->transitionDuration = 1.0f;
            cameraWork->startCenter = cameraWork->cameraCenter;
            cameraWork->endCenter.x = cameraTargetPosition.x;
            cameraWork->endCenter.y = cameraTargetPosition.y;
            cameraWork->endCenter.z = cameraTargetPosition.z;
            cameraWork->startRotation = cameraWork->cameraRotation;
            cameraWork->endRotation.x = CRot.x;
            cameraWork->endRotation.y = CRot.y;
            cameraWork->endRotation.z = CRot.z;
            cameraWork->startZoom = cameraWork->currentZoom;
            cameraWork->targetZoom = targetZoom;
            break;
        case 5:
            Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
            Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_REV);
            object->objFunc = UpdatePlayerEffects;
            object->work[0] = 10;
            object->work[1] = 0;
            break;
    }
    omSetTra(object, cutsceneWork->cameraRotation.x, cutsceneWork->cameraRotation.y, cutsceneWork->cameraRotation.z);
    omSetRot(object, cutsceneWork->startCenter.x, cutsceneWork->startCenter.y, cutsceneWork->startCenter.z);
}

void UpdatePlayerEffects(OMOBJ *object)
{
    Mtx rotationMatrix;
    Vec playerOffset;
    float playerAngles[4];
    float angleDifference;
    float playerDistance;
    float orbitCosine;
    float cameraDistance;
    float absoluteAngleDifference;
    float unreferencedCameraScratch;
    s32 activePlayerCount;
    M438CutsceneCameraWork *unreferencedCameraWork;
    s32 playerIndex;
    M438PlayerWork *playerWork;
    M438CutsceneCameraWork *cameraWork;

    cameraWork = object->data;
    switch (scenePhase) {
        case 7:
            switch (cameraWork->pathMode) {
                case 0:
                    cameraWork->endCenter = cameraWork->cameraRotation;
                    Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[3], 0.0f, 8.0f, 0);
                    cameraWork->pathMode++;
                case 1:
                    cameraWork->endCenter = cameraWork->cameraRotation;

                    for (playerIndex = 0, activePlayerCount = 0; playerIndex < 4; playerIndex++) {
                        playerWork = playerObjects[playerIndex]->data;
                        if ((playerWork->playerStateFlags != 0) && ((playerWork->playerStateFlags & 2) == 0)) {
                            VECSubtract(&playerWork->worldPosition, &cameraWork->cameraRotation, &playerOffset);
                            playerAngles[activePlayerCount++] = atan2d(playerOffset.x, playerOffset.z);
                        }
                    }
                    cameraWork->startCenter.y += 18.0f + object->work[1];
                    if (cameraWork->startCenter.y > 180.0f) {
                        cameraWork->startCenter.y -= 360.0f;
                    }
                    if (cameraWork->startCenter.y <= -180.0f) {
                        cameraWork->startCenter.y += 360.0f;
                    }

                    for (playerIndex = 0; playerIndex < activePlayerCount; playerIndex++) {
                        angleDifference = angleDifference(playerAngles[playerIndex], object->rot.y);
                        if (angleDifference < 0.0f) {
                            absoluteAngleDifference = -angleDifference;
                        }
                        else {
                            absoluteAngleDifference = angleDifference;
                        }
                        if (absoluteAngleDifference > (20.0f + object->work[1]))
                            continue;
                        object->work[0]--;
                        if (object->work[0] != 0 && ((GetRandomU16() & 0xF) != 0))
                            continue;
                        cameraWork->cameraCenter.y = playerAngles[playerIndex];
                        cameraWork->startCenter.y = playerAngles[playerIndex];
                        cameraWork->startRotation.y = 980.0f / REFRESH_RATE_F;
                        cameraWork->cameraCenter.z = 6.0 + (0.5 * object->work[1]);
                        cameraWork->endCenter = cameraWork->cameraRotation;
                        cameraWork->endRotation.x = 20.0f;
                        cameraWork->endRotation.y = 0.0f;
                        cameraWork->transitionTimer = REFRESH_RATE / 2;
                        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 8.0f, 0x40000002);
                        cameraWork->pathMode++;
                        break;
                    }
                    break;
                case 2:
                    cameraWork->endRotation.y += 10.0f;
                    if (cameraWork->endRotation.y > 90.0f) {
                        cameraWork->endRotation.y = 0.0f;
                    }
                    orbitCosine = cosd(cameraWork->endRotation.y);
                    cameraWork->startCenter.x = cameraWork->endRotation.x * orbitCosine;
                    cameraWork->cameraRotation.x = cameraWork->endCenter.x + ((50.0f * orbitCosine) * sind(cameraWork->startCenter.y));
                    cameraWork->cameraRotation.z = cameraWork->endCenter.z + ((50.0f * orbitCosine) * cosd(cameraWork->startCenter.y));
                    if (--cameraWork->transitionTimer == 0) {
                        HuAudFXEmiterPlay(0x700, &cameraWork->cameraRotation);
                        Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
                        Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
                        cameraWork->startCenter.x = 0.0f;
                        cameraWork->pathMode++;
                    }
                    break;
                case 3:
                    cameraWork->cameraCenter.z += 0.5;
                    cameraWork->startRotation.y += (VERSION_PAL) ? -3.6750002f : -3.0625f;
                    cameraDistance = cameraWork->cameraCenter.z + (0.8f * cameraWork->startRotation.y);
                    cameraWork->startRotation.x = cameraDistance * sind(cameraWork->cameraCenter.y);
                    cameraWork->startRotation.z = cameraDistance * cosd(cameraWork->cameraCenter.y);
                    cameraWork->cameraRotation.x += cameraWork->startRotation.x;
                    cameraWork->cameraRotation.y += cameraWork->startRotation.y;
                    cameraWork->cameraRotation.z += cameraWork->startRotation.z;
                    cameraWork->endCenter = cameraWork->cameraRotation;
                    if (cameraWork->cameraRotation.y < 0.0f) {
                        cameraWork->cameraRotation.y = 0.0f;
                        cameraWork->startRotation.y = 980.0f / REFRESH_RATE_F;
                    }
                    playerOffset = cameraWork->cameraRotation;
                    playerOffset.y = 0.0f;
                    playerDistance = VECMag(&playerOffset);
                    if (playerDistance <= 700.0f)
                        break;
                    cameraWork->transitionTimer = 1;
                    if (findAvailableRockForAngle(atan2d(playerOffset.x, playerOffset.z)) != 0) {
                        cameraWork->transitionTimer = REFRESH_RATE * 2;
                        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                            playerWork = playerObjects[playerIndex]->data;
                            if ((playerWork->playerStateFlags != 0) && ((playerWork->playerStateFlags & 2) == 0)) {
                                omVibrate(playerIndex, 0xC, 4, 2);
                            }
                        }
                        ConfigureCameraShake(0, REFRESH_RATE, 25.0f, 0.95f, 90.0f);
                        HuAudFXEmiterPlay(0x6FC, &cameraWork->cameraRotation);
                        HuAudFXPlay(0x6FE);
                    }
                    NormalizeVector(&playerOffset);
                    cameraWork->cameraRotation.x = 700.0f * playerOffset.x;
                    cameraWork->cameraRotation.z = 700.0f * playerOffset.z;
                    cameraWork->endRotation = cameraWork->startCenter;
                    cameraWork->pathMode++;
                    break;
                case 4:
                    cameraWork->startRotation.y += (VERSION_PAL) ? -3.6750002f : -3.0625f;
                    cameraWork->cameraRotation.y += cameraWork->startRotation.y;
                    if (cameraWork->cameraRotation.y > 0.0f)
                        break;
                    cameraWork->cameraRotation.y = 0.0f;
                    cameraWork->endCenter = cameraWork->cameraRotation;
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_REV);
                    cameraWork->endRotation.y += 8.0f;
                    cameraWork->endRotation.x = 10.0 * sind(cameraWork->endRotation.y);
                    cameraWork->endRotation.z = 10.0 * cosd(cameraWork->endRotation.y);
                    mtxRot(rotationMatrix, 0.0f, 360.0f - cameraWork->cameraCenter.y, cameraWork->endRotation.z);
                    mtxRotCat(rotationMatrix, cameraWork->endRotation.x, 0.0f, 0.0f);
                    RotateVectorByMatrix(rotationMatrix, &cameraWork->startCenter);
                    if (--cameraWork->transitionTimer == 0) {
                        cameraWork->startCenter.x = cameraWork->startCenter.z = 0.0f;
                        cameraWork->startCenter.y = cameraWork->cameraCenter.y;
                        object->work[0] = 0.5f * (sceneCountdownFrames / REFRESH_RATE_F);
                        object->work[1]++;
                        cameraWork->pathMode = 0;
                    }
                    break;
            }
            break;
        case 8:
            cameraWork->startCenter.x = cameraWork->startCenter.z = 0.0f;
            Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
            Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_REV);
            break;
        case 9:
            cameraWork->startCenter.y = ApproachAngleSmoothly(cameraWork->startCenter.y, 0.0f, 0.9f);
            break;
        case 10:
            if (cameraWork->cameraTransitionEnabled == 0) {
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
                Hu3DMotionSet(object->mdlId[0], object->mtnId[1]);
                Hu3DMotionSpeedSet(object->mdlId[0], 1.0f);
                Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
                cameraWork->cameraTransitionEnabled = 1;
            }
            break;
    }
    if (scenePhase == 7) {
        for (playerIndex = 0, activePlayerCount = 0; playerIndex < 4; playerIndex++) {
            playerWork = playerObjects[playerIndex]->data;
            VECSubtract(&playerWork->worldPosition, &cameraWork->cameraRotation, &playerOffset);
            playerDistance = VECMag(&playerOffset);
            if (playerDistance >= 200.0f || playerWork->currentMotionIndex == 7)
                continue;
            omVibrate(playerWork->playerIndex, 0xC, 4, 2);
            playerWork->playerStateFlags |= 2;
            playerWork->groundContactFlag = 0;
            playerWork->currentMotionIndex = 7;
            playerWork->facingAngle = atan2d(playerOffset.x, playerOffset.z);
            if (playerWork->facingAngle >= 360.0f) {
                playerWork->facingAngle -= 360.0f;
            }
            if (fabs(angleDifference(playerWork->facingAngle, playerWork->modelRotationY)) > 90.0) {
                playerWork->modelRotationY = 180.0f + playerWork->facingAngle;
                playerWork->specialMotionState = 0;
            }
            else {
                playerWork->modelRotationY = playerWork->facingAngle;
                playerWork->specialMotionState = 1;
            }
            playerWork->movementSpeed = 10.0f;
            playerWork->verticalVelocity = (VERSION_PAL) ? 63.7f : 53.083336f;
            if (playerRankByIndex[playerIndex] == 0) {
                playerRankByIndex[playerIndex] = nextPlayerRank--;
            }
        }
    }
    omSetTra(object, cameraWork->cameraRotation.x, cameraWork->cameraRotation.y, cameraWork->cameraRotation.z);
    omSetRot(object, cameraWork->startCenter.x, cameraWork->startCenter.y, cameraWork->startCenter.z);
}

void UpdateWindTrailParticles(M438ParticleSystem *particleSystem)
{
    s16 unreferencedAnimationScratch;
    f32 animationProgress;
    f32 particleGrowthFactor;
    f32 colorVariation;
    f32 fadeFactor;
    f32 particleAngleStep;
    s32 materialIndex;
    M438FireMaterial *material;

    unreferencedAnimationScratch = 0;
    switch (particleSystem->coordinateFlags) { /* irregular */
        case 0:
            particleAngleStep = 360.0f / particleSystem->materialCount;

            material = &particleSystem->materials[0];
            for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
                material->particleOffset.x = sind(particleAngleStep * materialIndex) * (0.8f + (0.1f * (2.0f * ((1.0f / 32768.0f) * (GetRandomU16() - 0x8000)))));
                material->particleOffset.y = 0.0f;
                material->particleOffset.z = cosd(particleAngleStep * materialIndex) * (0.8f + (0.1f * (2.0f * ((1.0f / 32768.0f) * (GetRandomU16() - 0x8000)))));
                material->position.x = (40.0f * material->particleOffset.x);
                material->position.y = 6.0f;
                material->position.z = (40.0f * material->particleOffset.z);
                colorVariation = 0.01f * (100.0f * ((1.0f / 65536.0f) * GetRandomU16()));
                material->modulationColor.r = (80.0f + 60.0f * colorVariation);
                material->modulationColor.g = (64.0f + 60.0f * colorVariation);
                material->modulationColor.b = (56.0f + 60.0f * colorVariation);
            }
            particleSystem->effectFlags = REFRESH_RATE * 0.8;
            particleSystem->coordinateFlags++;
        case 1:
            particleSystem->effectFlags--;
            animationProgress = ((REFRESH_RATE_F * 0.8f) - particleSystem->effectFlags) / (REFRESH_RATE_F * 0.8f);
            fadeFactor = particleSystem->effectFlags / (REFRESH_RATE_F * 0.8f);

            material = particleSystem->materials;
            for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
                material->position.x += 12.5f * fadeFactor * material->particleOffset.x;
                material->position.z += 12.5f * fadeFactor * material->particleOffset.z;
                particleGrowthFactor = 2.4f + (6.0f * animationProgress);
                material->scale.x = particleGrowthFactor;
                material->scale.y = particleGrowthFactor;
                material->scale.z = particleGrowthFactor;
                material->modulationColor.a = (96.0f * (1.0f - (animationProgress * animationProgress)));
            }
            if (particleSystem->effectFlags == 0) {
                particleSystem->coordinateFlags++;
            }
            break;
        case 2:
            DestroyParticleSystem(particleSystem->systemId);
            return;
    }
}

void UpdateFootstepParticles(M438ParticleSystem *particleSystem)
{
    s16 unreferencedAnimationScratch;

    M438FireMaterial *material;

    unreferencedAnimationScratch = 0;
    switch (particleSystem->coordinateFlags) {
        case 0:
            material = particleSystem->materials;
            material->particleOffset.y = 5.0f;
            material->particleOffset.x *= -2.0f;
            material->particleOffset.z *= -2.0f;
            particleSystem->coordinateFlags++;

        case 1:
            material = particleSystem->materials;
            material->position.x += material->particleOffset.x;
            material->position.y += material->particleOffset.y;
            material->position.z += material->particleOffset.z;
            return;
        case 2:
            DestroyParticleSystem(particleSystem->systemId);
            return;
    }
}

void UpdatePathDebrisParticles(M438ParticleSystem *particleSystem)
{
    Vec particleVelocity;
    M438FireMaterial *material;
    f32 initialVelocity;
    f32 particleSize;
    f32 impactAngle;
    f32 randomVelocityStep;
    s32 materialIndex;

    material = particleSystem->materials;
    for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
        switch (material->renderFlags) {
            case 0:
                impactAngle = atan2d(particleSystem->pathDirectionX, particleSystem->pathDirectionZ) < 0 ? -atan2d(particleSystem->pathDirectionX, particleSystem->pathDirectionZ) : atan2d(particleSystem->pathDirectionX, particleSystem->pathDirectionZ);

                if (impactAngle > 90.0f) {
                    impactAngle = 180.0f - impactAngle;
                }
                randomVelocityStep = 40.0f * (0.1f + ((0.9f * impactAngle) / 90.0f));
                initialVelocity = (0.5f * randomVelocityStep) + ((particleSystem->pathLength - randomVelocityStep) * ((1.0f / 65536.0f) * GetRandomU16()));
                particleVelocity.x = initialVelocity * particleSystem->pathDirectionX;
                particleVelocity.z = initialVelocity * particleSystem->pathDirectionZ;
                material->position.x = particleVelocity.x;
                material->position.y = 0.0f;
                material->position.z = particleVelocity.z;

                for (initialVelocity = 0.0f, particleVelocity.y = 0.0f; initialVelocity < (particleSystem->effectHeight - 40.0f); initialVelocity += particleVelocity.y) {
                    particleVelocity.y += 0.8f;
                }
                material->particleOffset.x = 0.0f;
                material->particleOffset.y = particleVelocity.y + (1.5f * ((1.0f / 65536.0f) * GetRandomU16()));
                material->particleOffset.z = 0.0f;
                if ((GetRandomU16() & 7) != 0) {
                    material->blendMode = 1;
                    material->modulationColor.r = 0x40;
                    material->modulationColor.g = 0x10;
                    material->modulationColor.b = 4;
                    material->modulationColor.a = 0xFF;
                    particleSize = 3.75f + (1.75f * ((1.0f / 65536.0f) * GetRandomU16()));
                    material->textureOffset.x = (particleSize * (0.1f + ((0.9f * impactAngle) / 90.0f)));
                    material->textureOffset.y = material->textureOffset.z = particleSize;
                }
                else {
                    material->blendMode = 0;
                    material->modulationColor.r = 0;
                    material->modulationColor.g = 0;
                    material->modulationColor.b = 0;
                    material->modulationColor.a = 0xFF;
                    particleSize = 0.25f + (0.25f * ((1.0f / 65536.0f) * GetRandomU16()));
                    material->textureOffset.y = (1.5f * particleSize);
                    material->textureOffset.x = material->textureOffset.z = particleSize;
                }
                material->animationTracks->playbackFlags = 0;
                material->animationFrameData[0] = (s32)((60.0f * ((1.0f / 65536.0f) * GetRandomU16()))) + 1;
                material->renderFlags++;
            case 1:
                if (--material->animationFrameData[0] == 0) {
                    material->animationTracks->playbackFlags = 0x11U;
                    material->animationTracks->frameStep = 0.5f;
                    material->animationTracks->frameAccumulator = 0.0f;
                    material->animationTracks->frameIndex = 0;
                    material->renderFlags++;
                }
                break;
            case 2:
                material->position.x += material->particleOffset.x;
                material->position.y += material->particleOffset.y;
                material->position.z += material->particleOffset.z;
                if (material->position.y < 0.0f) {
                    material->position.y = 0.0f;
                }
                material->textureOffset.x *= 0.95f;
                material->textureOffset.y *= 0.95f;
                material->textureOffset.z *= 0.95f;
                material->particleOffset.y -= 0.8f;
                if ((material->animationTracks->playbackFlags & 8) != 0) {
                    material->renderFlags = 0;
                }
                break;
        }
        material->scale.x = material->textureOffset.x * particleSystem->effectScale;
        material->scale.y = material->textureOffset.y * particleSystem->effectScale;
        material->scale.z = material->textureOffset.z * particleSystem->effectScale;
    }
}

void UpdateImpactParticles(M438ParticleSystem *particleSystem)
{
    Vec particleDirection;
    M438FireMaterial *material;
    f32 particleScale;
    f32 directionAngle;
    f32 spawnRadius;
    s32 particleSystemId;
    s32 completedMaterialCount;
    s32 materialIndex;

    material = particleSystem->materials;
    completedMaterialCount = 0;

    for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
        if ((particleSystem->effectFlags != 0) && (material->animationFrameData[1] == 0)) {
            material->renderFlags = 0;
        }
        switch (material->renderFlags) {
            case 0:
                if (particleSystem->effectFlags != 0) {
                    directionAngle = (180.0f * ((1.0f / 65536.0f) * GetRandomU16())) - 90.0f;
                    spawnRadius = 125.0f;
                    particleDirection.x = sind(directionAngle);
                    particleDirection.z = cosd(directionAngle);
                    material->position.x = spawnRadius * particleDirection.x;
                    material->position.y = 0.0f;
                    material->position.z = spawnRadius * particleDirection.z;
                    particleDirection.y = 25.0f;
                    material->particleOffset.x = 100.0f * (0.1f * particleDirection.x);
                    material->particleOffset.y = particleDirection.y + (2.5f * ((1.0f / 65536.0f) * GetRandomU16()));
                    material->particleOffset.z = 100.0f * (0.1f * particleDirection.z);
                    material->animationFrameData[0] = (s32)(24.0f * ((1.0f / 65536.0f) * GetRandomU16())) + 1;
                    material->animationFrameData[1] = 1;
                }
                else {
                    directionAngle = (180.0f * ((1.0f / 65536.0f) * GetRandomU16())) - 90.0f;
                    spawnRadius = 125.0f * ((1.0f / 65536.0f) * GetRandomU16());
                    particleDirection.x = sind(directionAngle);
                    particleDirection.z = cosd(directionAngle);
                    material->position.x = spawnRadius * particleDirection.x;
                    material->position.y = 0.0f;
                    material->position.z = spawnRadius * particleDirection.z;
                    particleDirection.y = 5.0f;
                    material->particleOffset.x = (100.0 * (0.015 * particleDirection.x));
                    material->particleOffset.y = particleDirection.y + ((1.0f / 65536.0f) * GetRandomU16());
                    material->particleOffset.z = 100.0f * (0.015f * particleDirection.z);
                    material->animationFrameData[0] = (s32)(60.0f * ((1.0f / 65536.0f) * GetRandomU16())) + 1;
                    material->animationFrameData[1] = 0;
                }
                material->stateFlags &= ~4;
                if ((GetRandomU16() & 0xF) != 0) {
                    material->blendMode = 1;
                    material->modulationColor.r = 0x40;
                    material->modulationColor.g = 0x10;
                    material->modulationColor.b = 4;
                    material->modulationColor.a = 0xFF;
                    particleScale = 1.8f + ((1.0f / 65536.0f) * GetRandomU16());
                    material->scale.x = particleScale;
                    material->scale.y = particleScale;
                    material->scale.z = particleScale;
                }
                else if (particleSystem->effectFlags != 0) {
                    material->blendMode = 0;
                    material->modulationColor.r = 0;
                    material->modulationColor.g = 0;
                    material->modulationColor.b = 0;
                    material->modulationColor.a = 0xFF;
                    particleScale = 0.25f + (0.25f * ((1.0f / 65536.0f) * GetRandomU16()));
                    material->scale.x = particleScale;
                    material->scale.y = 1.5f * particleScale;
                    material->scale.z = particleScale;
                }
                else {
                    particleSystemId = createParticleSystem(8, 1);
                    LoadAnimationToTextureSlotDefaultSize(particleSystemId, 0, 0x45000A);
                    fireManager.particleSystems[particleSystemId].updateCallback = (void (*)(M438ParticleSystem *))UpdateLandingParticles;
                    SetParticleSystemPosition(particleSystemId, particleSystem->position.x + material->position.x, particleSystem->position.y + material->position.y, particleSystem->position.z + material->position.z);
                    EnableAllMaterialFlags(particleSystemId, 4);
                    SetAllMaterialsEnabled(particleSystemId, 1);
                    material->stateFlags |= 4;
                    break;
                }
                material->animationTracks->playbackFlags = 0;
                material->renderFlags++;
                break;
            case 1:
                if (material->animationFrameData[1] != 0) {
                    material->particleOffset.y -= 0.5f;
                }
                if (--material->animationFrameData[0] == 0) {
                    material->animationTracks->playbackFlags = 0x11;
                    if (material->animationFrameData[1] != 0) {
                        material->animationTracks->frameStep = 0.5f;
                    }
                    else {
                        material->animationTracks->frameStep = 0.5f;
                    }
                    material->animationTracks->frameAccumulator = 0.0f;
                    material->animationTracks->frameIndex = 0;
                    material->renderFlags++;
                }
                break;
            case 2:
                material->position.x += material->particleOffset.x;
                material->position.y += material->particleOffset.y;
                material->position.z += material->particleOffset.z;
                if (material->position.y < 0.0f) {
                    material->position.y = 0.0f;
                }
                material->scale.x *= 0.95f;
                material->scale.y *= 0.95f;
                material->scale.z *= 0.95f;
                material->particleOffset.y -= 1.5f;
                if ((material->animationTracks->playbackFlags & 8) != 0) {
                    if (material->animationFrameData[1] != 0) {
                        material->renderFlags = 3;
                    }
                    else {
                        material->renderFlags = 0;
                    }
                }
                break;
            case 3:
                completedMaterialCount++;
                break;
        }
    }
    if (completedMaterialCount >= particleSystem->materialCount) {
        DestroyParticleSystem(particleSystem->systemId);
    }
}

void UpdateLandingParticles(M438ParticleSystem *particleSystem)
{
    Vec particleDirection;
    M438FireMaterial *material;
    f32 spawnAngle;
    f32 radialSpeed;
    f32 particleScale;
    s32 completedMaterialCount;
    s32 materialIndex;

    material = particleSystem->materials;

    switch (particleSystem->coordinateFlags) {
        case 0:
            particleSystem->scale.x = particleSystem->scale.y = particleSystem->scale.z = particleScale = 0.01f;
            particleSystem->effectFlags = Hu3DModelCreateFile(0x45000B);
            particleSystem->effectTimer = (s32)(30.0f * ((1.0f / 65536.0f) * GetRandomU16())) + 30;

            for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
                material->stateFlags |= 4;
            }
            particleSystem->coordinateFlags++;
        case 1:
            particleSystem->scale.z += 0.008f;

            particleSystem->scale.x = particleSystem->scale.y = particleSystem->scale.z;
            Hu3DModelPosSetV(particleSystem->effectFlags, &particleSystem->position);
            Hu3DModelScaleSetV(particleSystem->effectFlags, &particleSystem->scale);
            if (--particleSystem->effectTimer == 0) {
                Hu3DModelKill(particleSystem->effectFlags);
                particleSystem->effectFlags = 0;

                for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
                    material->renderFlags = 1;
                }
                particleSystem->pathDirectionX = particleSystem->scale.x;

                particleSystem->scale.x = particleSystem->scale.y = particleSystem->scale.z = 1.0f;
                particleSystem->coordinateFlags++;
            }
            break;
    }
    completedMaterialCount = 0;
    material = particleSystem->materials;

    for (materialIndex = 0; materialIndex < particleSystem->materialCount; materialIndex++, material++) {
        switch (material->renderFlags) {
            case 1:
                material->stateFlags &= ~4;
                material->blendMode = 1;
                material->modulationColor.r = 0x60;
                material->modulationColor.g = 0x20;
                material->modulationColor.b = 0x10;
                material->modulationColor.a = 0xFF;
                particleScale = 0.4f + (0.15f * ((1.0f / 65536.0f) * GetRandomU16()));
                material->scale.x = particleScale;
                material->scale.y = particleScale;
                material->scale.z = particleScale;
                spawnAngle = 360.0f * ((1.0f / 65536.0f) * GetRandomU16());
                radialSpeed = 50.0f * particleSystem->pathDirectionX;
                particleDirection.x = sind(spawnAngle);
                particleDirection.z = cosd(spawnAngle);
                material->position.x = radialSpeed * particleDirection.x;
                material->position.y = radialSpeed;
                material->position.z = radialSpeed * particleDirection.z;
                particleDirection.y = 6.0f;
                material->particleOffset.x = 100.0f * (0.015f * particleDirection.x);
                material->particleOffset.y = particleDirection.y + (1.5f * ((1.0f / 65536.0f) * GetRandomU16()));
                material->particleOffset.z = 100.0f * (0.015f * particleDirection.z);
                material->animationTracks->playbackFlags = 0x11;
                material->animationTracks->frameStep = 0.75f;
                material->renderFlags++;
                break;
            case 2:
                material->position.x += material->particleOffset.x;
                material->position.y += material->particleOffset.y;
                material->position.z += material->particleOffset.z;
                material->particleOffset.y -= 0.75f;
                if ((material->animationTracks->playbackFlags & 8) != 0) {
                    material->renderFlags++;
                }
                break;
            case 3:
                completedMaterialCount++;
                break;
        }
    }
    if (completedMaterialCount >= particleSystem->materialCount) {
        DestroyParticleSystem(particleSystem->systemId);
    }
    if (scenePhase == 2) {
        if (particleSystem->effectFlags != 0) {
            Hu3DModelKill(particleSystem->effectFlags);
        }
        DestroyParticleSystem(particleSystem->systemId);
    }
}

void DestroyPlayerObject(OMOBJ *object)
{
    Vec separationDisplacement;
    Vec separationDirection;
    Vec selfGroundPosition;
    Vec otherGroundPosition;

    float separationDistance;
    float separationWeight;
    float selfTravelDistance;
    float otherTravelDistance;

    M438PlayerWork *playerWork;
    M438PlayerWork *otherPlayerWork;
    s32 otherPlayerIndex;

    playerWork = object->data;
    if (playerWork->playerStateFlags == 0 || (playerWork->playerStateFlags & 2) != 0) {
        return;
    }
    VECSubtract(&playerWork->worldPosition, &playerWork->previousWorldPosition, &separationDisplacement);
    selfTravelDistance = VECMag(&separationDisplacement);
    if (selfTravelDistance == 0.0f) {
        return;
    }
    for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
        if (otherPlayerIndex == playerWork->playerIndex) {
            continue;
        }
        otherPlayerWork = playerObjects[otherPlayerIndex]->data;
        if (otherPlayerWork->playerStateFlags == 0 || (otherPlayerWork->playerStateFlags & 2) != 0) {
            continue;
        }
        selfGroundPosition.x = playerWork->worldPosition.x;
        selfGroundPosition.y = playerWork->worldPosition.y;
        selfGroundPosition.z = playerWork->worldPosition.z;
        selfGroundPosition.y = 0.0f;

        otherGroundPosition.x = otherPlayerWork->worldPosition.x;
        otherGroundPosition.y = otherPlayerWork->worldPosition.y;
        otherGroundPosition.z = otherPlayerWork->worldPosition.z;
        otherGroundPosition.y = 0.0f;
        VECSubtract(&selfGroundPosition, &otherGroundPosition, &separationDisplacement);
        separationDistance = VECMag(&separationDisplacement);
        if (separationDistance >= 119.40001f) {
            continue;
        }
        VECSubtract(&otherPlayerWork->worldPosition, &otherPlayerWork->previousWorldPosition, &separationDisplacement);
        otherTravelDistance = VECMag(&separationDisplacement);
        VECSubtract(&selfGroundPosition, &otherGroundPosition, &separationDirection);
        VECNormalize(&separationDirection, &separationDirection);
        separationDistance = 120.00001f - separationDistance;
        separationWeight = ((0.6f * otherTravelDistance) + (0.4f * selfTravelDistance)) / (selfTravelDistance + otherTravelDistance);
        playerWork->worldPosition.x = playerWork->worldPosition.x + (separationWeight * (separationDistance * separationDirection.x));
        playerWork->worldPosition.y = playerWork->worldPosition.y + (separationWeight * (separationDistance * separationDirection.y));
        playerWork->worldPosition.z = playerWork->worldPosition.z + (separationWeight * (separationDistance * separationDirection.z));

        separationWeight = ((0.6f * selfTravelDistance) + (0.4f * selfTravelDistance)) / (selfTravelDistance + otherTravelDistance);
        otherPlayerWork->worldPosition.x = otherPlayerWork->worldPosition.x + (separationWeight * (separationDistance * -separationDirection.x));
        otherPlayerWork->worldPosition.y = otherPlayerWork->worldPosition.y + (separationWeight * (separationDistance * -separationDirection.y));
        otherPlayerWork->worldPosition.z = otherPlayerWork->worldPosition.z + (separationWeight * (separationDistance * -separationDirection.z));
        DestroyPlayerObject(playerObjects[otherPlayerIndex]);
    }
}

void UpdateSceneCleanup(OMOBJ *object)
{
    s32 playerIndex;

    if (scenePhase >= 7) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            DestroyPlayerObject(playerObjects[playerIndex]);
        }
    }
}

void InitializeSceneAudio(void)
{
    s32 allocationSlotIndex;

    for (allocationSlotIndex = 0; allocationSlotIndex < 0x80; allocationSlotIndex++) {
        heapAllocationSlots[allocationSlotIndex] = NULL;
    }
}

void *AllocateSceneMemory(s32 allocationSize)
{
    s32 freeSlotIndex;

    for (freeSlotIndex = 0; freeSlotIndex < 128; freeSlotIndex++) {
        if (!heapAllocationSlots[freeSlotIndex]) {
            break;
        }
    }
    if (freeSlotIndex == 0x80) {
        return NULL;
    }
    heapAllocationSlots[freeSlotIndex] = HuMemDirectMallocNum(HEAP_HEAP, allocationSize, HU_MEMNUM_OVL);
    return heapAllocationSlots[freeSlotIndex];
}

void FreeSceneMemory(void *allocation)
{
    s32 allocationSlotIndex;

    for (allocationSlotIndex = 0; allocationSlotIndex < 128; allocationSlotIndex++) {
        if (heapAllocationSlots[allocationSlotIndex] == allocation) {
            break;
        }
    }
    if (allocationSlotIndex != 128) {
        HuMemDirectFree(heapAllocationSlots[allocationSlotIndex]);
        heapAllocationSlots[allocationSlotIndex] = NULL;
    }
}

void FreeAllSceneMemory(void)
{
    s32 allocationSlotIndex;

    for (allocationSlotIndex = 0; allocationSlotIndex < 128; allocationSlotIndex++) {
        if (heapAllocationSlots[allocationSlotIndex]) {
            HuMemDirectFree(heapAllocationSlots[allocationSlotIndex]);
            heapAllocationSlots[allocationSlotIndex] = NULL;
        }
    }
}

s32 GetRandomU16(void)
{
    return frand() & 0xFFFF;
}

void ChooseRandomIndices(s32 valueCount, s32 *indices)
{
    s32 valueIndex;
    s32 randomIndex;
    s32 savedValue;
    s32 randomValue;

    for (valueIndex = 0; valueIndex < valueCount; valueIndex++) {
        indices[valueIndex] = valueIndex;
    }
    for (valueIndex = 0; valueIndex < (valueCount - 1); valueIndex++) {
        randomValue = frand() & 0xFFFF;
        randomIndex = valueCount * (0.000015258789f * randomValue);
        if (randomIndex >= valueCount) {
            randomIndex = valueCount - 1;
        }
        savedValue = indices[valueIndex];
        indices[valueIndex] = indices[randomIndex];
        indices[randomIndex] = savedValue;
    }
}

float ApproachAngleSmoothly(float currentAngle, float targetAngle, float interpolationFactor)
{
    float angleDifference;

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
    currentAngle = targetAngle + (angleDifference * interpolationFactor);
    if (currentAngle > 180.0f) {
        currentAngle -= 360.0f;
    }
    else if (currentAngle <= -180.0f) {
        currentAngle += 360.0f;
    }
    return currentAngle;
}

void SetMatrixTranslation(Mtx matrix, f32 rotationX, f32 rotationY, f32 rotationZ)
{
    Mtx xRotationMatrix;
    Mtx yRotationMatrix;

    if (0.0f != rotationZ) {
        MTXRotRad(matrix, 0x5A, MTXDegToRad(rotationZ));
    }
    else {
        MTXIdentity(matrix);
    }
    if (0.0f != rotationX) {
        MTXRotRad(xRotationMatrix, 0x58, MTXDegToRad(rotationX));
        MTXConcat(xRotationMatrix, matrix, matrix);
    }
    if (0.0f != rotationY) {
        MTXRotRad(yRotationMatrix, 0x59, MTXDegToRad(rotationY));
        MTXConcat(yRotationMatrix, matrix, matrix);
    }
}

void RotateVectorByMatrix(Mtx matrix, Vec *vector)
{
    f32 sineComponent;
    f32 cosineComponent;

    vector->x = atan2d(matrix[1][2], matrix[2][2]);
    if (vector->x < 0.0f) {
        vector->x += 360.0f;
    }
    vector->z = atan2d(matrix[0][1], matrix[0][0]);
    if (vector->z < 0.0f) {
        vector->z += 360.0f;
    }
    sineComponent = -matrix[0][2];
    cosineComponent = sqrtf(BOARD_FABS(1.0 - sineComponent * sineComponent));
    if ((vector->x > 90.0f) && (vector->x < 270.0f) && (vector->z > 90.0f) && (vector->z < 270.0f)) {
        vector->x = fmod((180.0f + vector->x), 360.0);
        vector->z = fmod((180.0f + vector->z), 360.0);
        cosineComponent = -cosineComponent;
    }
    vector->y = atan2d(sineComponent, cosineComponent);
    if (vector->y < 0.0f) {
        vector->y += 360.0f;
    }
}

void TransformVectorByMatrix(Mtx matrix, Vec *rotation)
{
    f32 sineComponent;
    f32 cosineComponent;

    rotation->z = atan2d(matrix[0][1], matrix[1][1]);
    rotation->y = atan2d(matrix[2][0], matrix[2][2]);
    sineComponent = -matrix[2][1];
    cosineComponent = sqrtf(BOARD_FABS(1.0 - sineComponent * sineComponent));
    rotation->x = atan2d(sineComponent, cosineComponent);
}

f32 CalculateSplineSegment(f32 segmentProgress, f32 startValue, f32 controlValue, f32 endValue)
{
    f32 oneMinusT;
    f32 bezierValue;

    oneMinusT = (1.0 - segmentProgress);
    bezierValue = ((oneMinusT * oneMinusT) * startValue) + (oneMinusT * segmentProgress * controlValue * 2.0) + ((segmentProgress * segmentProgress) * endValue);
    return bezierValue;
}

void CalculateSplineCoefficients(f32 *startValues, f32 *controlValues, f32 *endValues, f32 *splineValues, f32 segmentProgress)
{
    s32 coordinateIndex;

    for (coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++) {
        *splineValues++ = CalculateSplineSegment(segmentProgress, *startValues++, *controlValues++, *endValues++);
    }
}

f32 CalculateSplineSegmentAlt(f32 segmentProgress, f32 startValue, f32 controlValue, f32 endValue)
{
    f32 derivativeValue;

    derivativeValue = 2.0 * ((segmentProgress - 1.0) * startValue + (1.0 - (2.0 * segmentProgress)) * controlValue + (segmentProgress * endValue));
    return derivativeValue;
}

void CalculateSplineCoefficientsAlt(f32 *startValues, f32 *controlValues, f32 *endValues, f32 *derivativeValues, f32 segmentProgress)
{
    f32 derivativeVector[3];
    f32 vectorMagnitude;
    s32 coordinateIndex;

    for (coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++) {
        derivativeVector[coordinateIndex] = CalculateSplineSegmentAlt(segmentProgress, *startValues++, *controlValues++, *endValues++);
    }
    vectorMagnitude = sqrtf((derivativeVector[0] * derivativeVector[0]) + (derivativeVector[1] * derivativeVector[1]) + (derivativeVector[2] * derivativeVector[2]));
    if (vectorMagnitude) {
        vectorMagnitude = (1.0 / vectorMagnitude);
        for (coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++) {
            *derivativeValues++ = vectorMagnitude * derivativeVector[coordinateIndex];
        }
        return;
    }
    *derivativeValues++ = 0.0f;
    *derivativeValues++ = 0.0f;
    *derivativeValues++ = 1.0f;
}

void SolveSplineCoefficients(float *controlPointDistances, float *controlPointValues, float *secondDerivatives, s32 controlPointCount)
{
    float segmentWidths[16];
    float slopeCoefficients[16];
    double eliminationFactor;
    s32 pointIndex;

    secondDerivatives[0] = 0.0f;
    secondDerivatives[controlPointCount - 1] = 0.0f;

    for (pointIndex = 0; pointIndex < (controlPointCount - 1); pointIndex++) {
        segmentWidths[pointIndex] = controlPointDistances[pointIndex + 1] - controlPointDistances[pointIndex];
        if (segmentWidths[pointIndex] == 0.0f) {
            slopeCoefficients[pointIndex + 1] = 0.0f;
        }
        else {
            slopeCoefficients[pointIndex + 1] = (controlPointValues[pointIndex + 1] - controlPointValues[pointIndex]) / segmentWidths[pointIndex];
        }
    }
    secondDerivatives[1] = slopeCoefficients[2] - slopeCoefficients[1];
    slopeCoefficients[1] = 2.0f * (controlPointDistances[2] - controlPointDistances[0]);

    for (pointIndex = 1; pointIndex < (controlPointCount - 2); pointIndex++) {
        eliminationFactor = segmentWidths[pointIndex] / slopeCoefficients[pointIndex];
        secondDerivatives[pointIndex + 1] = (slopeCoefficients[pointIndex + 2] - slopeCoefficients[pointIndex + 1]) - (eliminationFactor * secondDerivatives[pointIndex]);
        slopeCoefficients[pointIndex + 1] = (2.0f * (controlPointDistances[pointIndex + 2] - controlPointDistances[pointIndex])) - (eliminationFactor * segmentWidths[pointIndex]);
    }
    secondDerivatives[controlPointCount - 2] -= segmentWidths[controlPointCount - 2] * secondDerivatives[controlPointCount - 1];

    for (pointIndex = controlPointCount - 2; pointIndex > 0; pointIndex--) {
        if (slopeCoefficients[pointIndex] == 0.0f) {
            secondDerivatives[pointIndex] = 0.0f;
        }
        else {
            secondDerivatives[pointIndex] = (secondDerivatives[pointIndex] - (segmentWidths[pointIndex] * secondDerivatives[pointIndex + 1])) / slopeCoefficients[pointIndex];
        }
    }
}

float EvaluateSpline(float segmentPosition, float *controlPointDistances, float *controlPointValues, float *secondDerivatives, s32 controlPointCount)
{
    float segmentLength;
    float segmentDistance;
    float interpolatedValue;

    s32 segmentIndex = segmentPosition;

    segmentPosition = (segmentPosition - segmentIndex);
    segmentLength = controlPointDistances[segmentIndex + 1] - controlPointDistances[segmentIndex];
    segmentDistance = segmentLength * segmentPosition;
    interpolatedValue = controlPointValues[segmentIndex]
        + (segmentDistance
            * ((segmentDistance * ((3.0f * secondDerivatives[segmentIndex]) + ((segmentDistance * (secondDerivatives[segmentIndex + 1] - secondDerivatives[segmentIndex])) / segmentLength)))
                + (((controlPointValues[segmentIndex + 1] - controlPointValues[segmentIndex]) / segmentLength) - (segmentLength * ((2.0f * secondDerivatives[segmentIndex]) + secondDerivatives[segmentIndex + 1])))));
    if (segmentLength == 0.0f) {
        interpolatedValue = controlPointValues[segmentIndex];
    }
    return interpolatedValue;
}

void BuildSplineCoefficients(float *controlPointDistances, Vec *controlPoints, float splineCoefficients[3][3], s32 controlPointCount)
{
    float controlPointValues[16];
    float splineSecondDerivatives[16];
    float splineSlopes[16];
    float segmentLengths[16];
    Vec controlPointDelta;

    double eliminationFactor;

    s32 segmentIndex;
    s32 controlPointIndex;
    s32 coordinateAxis;

    controlPointDistances[0] = 0.0f;
    for (controlPointIndex = 1; controlPointIndex < controlPointCount; controlPointIndex++) {
        VECSubtract(&controlPoints[controlPointIndex], &controlPoints[controlPointIndex - 1], &controlPointDelta);
        controlPointDistances[controlPointIndex] = controlPointDistances[controlPointIndex - 1] + VECMag(&controlPointDelta);
    }
    for (coordinateAxis = 0; coordinateAxis < 3; coordinateAxis++) {
        for (controlPointIndex = 0; controlPointIndex < controlPointCount; controlPointIndex++) {
            controlPointValues[controlPointIndex] = ((float *)&controlPoints[controlPointIndex])[coordinateAxis]; // TODO
        }
        splineSecondDerivatives[0] = 0.0f;
        splineSecondDerivatives[controlPointCount - 1] = 0.0f;
        for (segmentIndex = 0; segmentIndex < (controlPointCount - 1); segmentIndex++) {
            segmentLengths[segmentIndex] = controlPointDistances[segmentIndex + 1] - controlPointDistances[segmentIndex];
            if (segmentLengths[segmentIndex] == 0.0f) {
                splineSlopes[segmentIndex + 1] = 0.0f;
            }
            else {
                splineSlopes[segmentIndex + 1] = (controlPointValues[segmentIndex + 1] - controlPointValues[segmentIndex]) / segmentLengths[segmentIndex];
            }
        }
        splineSecondDerivatives[1] = splineSlopes[2] - splineSlopes[1];
        splineSlopes[1] = 2.0f * (controlPointDistances[2] - controlPointDistances[0]);
        for (segmentIndex = 1; segmentIndex < (controlPointCount - 2); segmentIndex++) {
            eliminationFactor = segmentLengths[segmentIndex] / splineSlopes[segmentIndex];
            splineSecondDerivatives[segmentIndex + 1] = (splineSlopes[segmentIndex + 2] - splineSlopes[segmentIndex + 1]) - (eliminationFactor * splineSecondDerivatives[segmentIndex]);
            splineSlopes[segmentIndex + 1] = (2.0f * (controlPointDistances[segmentIndex + 2] - controlPointDistances[segmentIndex])) - (eliminationFactor * segmentLengths[segmentIndex]);
        }
        splineSecondDerivatives[controlPointCount - 2] -= segmentLengths[controlPointCount - 2] * splineSecondDerivatives[controlPointCount - 1];
        for (segmentIndex = controlPointCount - 2; segmentIndex > 0; segmentIndex--) {
            if (splineSlopes[segmentIndex] == 0.0f) {
                splineSecondDerivatives[segmentIndex] = 0.0f;
            }
            else {
                splineSecondDerivatives[segmentIndex] = (splineSecondDerivatives[segmentIndex] - (segmentLengths[segmentIndex] * splineSecondDerivatives[segmentIndex + 1])) / splineSlopes[segmentIndex];
            }
        }
        for (controlPointIndex = 0; controlPointIndex < controlPointCount; controlPointIndex++) {
            splineCoefficients[controlPointIndex][coordinateAxis] = splineSecondDerivatives[controlPointIndex];
        }
    }
}

void EvaluateVectorSpline(float segmentPosition, float *outputVector, float *controlPointDistances, float splineCoefficients[3][3], float tangentCoefficients[3][3], s32 controlPointCount)
{
    float controlPointValues[16];
    float splineCoefficients[16];

    float unreferencedSplineScratch;

    s32 controlPointIndex;
    s32 coordinateAxis;

    for (coordinateAxis = 0; coordinateAxis < 3; coordinateAxis++) {
        for (controlPointIndex = 0; controlPointIndex < controlPointCount; controlPointIndex++) {
            controlPointValues[controlPointIndex] = splineCoefficients[controlPointIndex][coordinateAxis];
            splineCoefficients[controlPointIndex] = tangentCoefficients[controlPointIndex][coordinateAxis];
        }
        outputVector[coordinateAxis] = EvaluateSpline(segmentPosition, controlPointDistances, controlPointValues, splineCoefficients, controlPointCount);
    }
}

f32 NormalizeVector(Vec *vector)
{
    f32 vectorMagnitude;
    f32 normalizationFactor;
    vectorMagnitude = (vector->x * vector->x) + (vector->y * vector->y) + (vector->z * vector->z);
    vectorMagnitude = sqrtf(vectorMagnitude);
    if (vectorMagnitude != 0.0f) {
        normalizationFactor = 1.0f / vectorMagnitude;
        vector->x *= normalizationFactor;
        vector->y *= normalizationFactor;
        vector->z *= normalizationFactor;
    }
    else {
        vector->x = vector->y = vector->z = 0.0f;
    }
    return vectorMagnitude;
}

f32 InterpolateVector(Vec startVector, Vec endVector, Vec *interpolatedVector, f32 interpolationFactor)
{
    if (interpolationFactor <= 0.0f) {
        interpolatedVector->x = startVector.x;
        interpolatedVector->y = startVector.y;
        interpolatedVector->z = startVector.z;
        interpolationFactor = 0.0f;
    }
    else if (interpolationFactor >= 1.0f) {
        interpolatedVector->x = (startVector.x + endVector.x);
        interpolatedVector->y = (startVector.y + endVector.y);
        interpolatedVector->z = (startVector.z + endVector.z);
        interpolationFactor = 1.0f;
    }
    else {
        interpolatedVector->x = (startVector.x + (interpolationFactor * endVector.x));
        interpolatedVector->y = (startVector.y + (interpolationFactor * endVector.y));
        interpolatedVector->z = (startVector.z + (interpolationFactor * endVector.z));
    }
    return interpolationFactor;
}

f32 ProjectPointOntoLine(Vec lineStart, Vec lineEnd, Vec lineDirection)
{
    f32 negativeDirectionMagnitudeSquared;
    f32 projectionNumerator;

    projectionNumerator = (lineDirection.z * (lineEnd.z - lineStart.z)) + ((lineDirection.x * (lineEnd.x - lineStart.x)) + (lineDirection.y * (lineEnd.y - lineStart.y)));
    negativeDirectionMagnitudeSquared = -((lineDirection.z * lineDirection.z) + ((lineDirection.x * lineDirection.x) + (lineDirection.y * lineDirection.y)));
    if (0.0f != negativeDirectionMagnitudeSquared) {
        projectionNumerator /= negativeDirectionMagnitudeSquared;
    }
    return projectionNumerator;
}

f32 DistanceSquaredToSegment(Vec *point, Vec *segmentStart, Vec *segmentDirection)
{
    Vec closestPoint;
    f32 segmentParameter;
    f32 unreferencedDistanceScratch;
    f32 unreferencedProjectionScratch;

    if (0.0f == ((segmentDirection->z * segmentDirection->z) + ((segmentDirection->x * segmentDirection->x) + (segmentDirection->y * segmentDirection->y)))) {
        return ((point->z - segmentStart->z) * (point->z - segmentStart->z))
            + (((point->x - segmentStart->x) * (point->x - segmentStart->x)) + ((point->y - segmentStart->y) * (point->y - segmentStart->y)));
    }
    segmentParameter = ProjectPointOntoLine(*point, *segmentStart, *segmentDirection);
    InterpolateVector(*segmentStart, *segmentDirection, &closestPoint, segmentParameter);
    return ((point->z - closestPoint.z) * (point->z - closestPoint.z)) + (((point->x - closestPoint.x) * (point->x - closestPoint.x)) + ((point->y - closestPoint.y) * (point->y - closestPoint.y)));
}

f32 DistanceToSegmentWithDelta(Vec *point, Vec *segmentStart, Vec *segmentDirection, Vec *closestPoint, Vec *separationDirection)
{
    f32 unreferencedScalarScratch1;
    f32 unreferencedScalarScratch2;
    f32 unreferencedScalarScratch3;
    f32 unreferencedScalarScratch4;
    f32 unreferencedScalarScratch5;
    f32 unreferencedScalarScratch6;
    f32 unreferencedScalarScratch7;
    f32 unreferencedScalarScratch8;
    f32 unreferencedScalarScratch9;
    Vec unreferencedVectorScratch;
    f32 unreferencedScalarScratch10;
    f32 unreferencedScalarScratch11;
    f32 unreferencedScalarScratch12;
    f32 unreferencedScalarScratch13;
    f32 unreferencedScalarScratch14;
    f32 unreferencedScalarScratch15;
    f32 unreferencedScalarScratch16;
    f32 unreferencedScalarScratch17;
    f32 segmentParameter;
    f64 planeDeterminant;
    f64 unreferencedDeterminantScratch;
    f64 unreferencedIntersectionScratch;

    if (0.0f == ((segmentDirection->z * segmentDirection->z) + ((segmentDirection->x * segmentDirection->x) + (segmentDirection->y * segmentDirection->y)))) {
        return ((point->z - segmentStart->z) * (point->z - segmentStart->z))
            + (((point->x - segmentStart->x) * (point->x - segmentStart->x)) + ((point->y - segmentStart->y) * (point->y - segmentStart->y)));
    }
    segmentParameter = ProjectPointOntoLine(*point, *segmentStart, *segmentDirection);
    InterpolateVector(*segmentStart, *segmentDirection, closestPoint, segmentParameter);
    separationDirection->x = (point->x - closestPoint->x);
    separationDirection->y = (point->y - closestPoint->y);
    separationDirection->z = (point->z - closestPoint->z);

    return NormalizeVector(separationDirection);
}

f32 IntersectSegments(Vec *lineStart, Vec *lineDirection, Vec *segmentStart, Vec *segmentDirection)
{
    f32 planeDeterminant;
    f32 noIntersectionResult;
    f32 intersectionFraction;
    f32 absoluteDeltaX;
    f32 absoluteDeltaY;
    f32 absoluteDeltaZ;
    s32 dominantAxisMask;
    s32 axisMaskXOrZ;
    s32 axisMaskYOrZ;

    noIntersectionResult = -1.0f;
    dominantAxisMask = 0;

    absoluteDeltaX = BOARD_FABS(segmentDirection->x);
    absoluteDeltaY = BOARD_FABS(segmentDirection->y);
    absoluteDeltaZ = BOARD_FABS(segmentDirection->z);
    if (absoluteDeltaX >= absoluteDeltaY) {
        if (absoluteDeltaX >= absoluteDeltaZ) {
            axisMaskXOrZ = 1;
        }
        else {
            axisMaskXOrZ = 0x100;
        }
        dominantAxisMask = axisMaskXOrZ;
    }
    else {
        if (absoluteDeltaY >= absoluteDeltaZ) {
            axisMaskYOrZ = 0x10;
        }
        else {
            axisMaskYOrZ = 0x100;
        }
        dominantAxisMask = axisMaskYOrZ;
    }
    if ((dominantAxisMask & 3) != 0) {
        if ((0.0f != lineDirection->x) || (0.0f != lineDirection->z)) {
            if (0.0f == (planeDeterminant = (segmentDirection->x * lineDirection->z) - (segmentDirection->z * lineDirection->x))) {
                return noIntersectionResult;
            }
            intersectionFraction = ((segmentDirection->z * (lineStart->x - segmentStart->x)) - (segmentDirection->x * (lineStart->z - segmentStart->z))) / planeDeterminant;
        }
        else if (0.0f == (planeDeterminant = (segmentDirection->y * lineDirection->x) - (segmentDirection->x * lineDirection->y))) {
            return noIntersectionResult;
        }
        else {
            intersectionFraction = ((segmentDirection->x * (lineStart->y - segmentStart->y)) - (segmentDirection->y * (lineStart->x - segmentStart->x))) / planeDeterminant;
        }
    }
    else if ((dominantAxisMask & 0x30) != 0) {
        if ((0.0f != lineDirection->x) || (0.0f != lineDirection->y)) {
            if (0.0f == (planeDeterminant = (segmentDirection->y * lineDirection->x) - (segmentDirection->x * lineDirection->y))) {
                return noIntersectionResult;
            }
            intersectionFraction = ((segmentDirection->x * (lineStart->y - segmentStart->y)) - (segmentDirection->y * (lineStart->x - segmentStart->x))) / planeDeterminant;
        }
        else if (0.0f == (planeDeterminant = (segmentDirection->z * lineDirection->y) - (segmentDirection->y * lineDirection->z))) {
            return noIntersectionResult;
        }
        else {
            intersectionFraction = ((segmentDirection->y * (lineStart->z - segmentStart->z)) - (segmentDirection->z * (lineStart->y - segmentStart->y))) / planeDeterminant;
        }
    }
    else if ((0.0f != lineDirection->x) || (0.0f != lineDirection->z)) {
        if (0.0f == (planeDeterminant = (segmentDirection->x * lineDirection->z) - (segmentDirection->z * lineDirection->x))) {
            return noIntersectionResult;
        }
        intersectionFraction = ((segmentDirection->z * (lineStart->x - segmentStart->x)) - (segmentDirection->x * (lineStart->z - segmentStart->z))) / planeDeterminant;
    }
    else if (0.0f == (planeDeterminant = (segmentDirection->z * lineDirection->y) - (segmentDirection->y * lineDirection->z))) {
        return noIntersectionResult;
    }
    else {
        intersectionFraction = ((segmentDirection->y * (lineStart->z - segmentStart->z)) - (segmentDirection->z * (lineStart->y - segmentStart->y))) / planeDeterminant;
    }
    return intersectionFraction;
}

void InitializeWindCollisionMap(OMOBJ *object)
{
    object->data = HuMemDirectMallocNum(HEAP_HEAP, 0x900, HU_MEMNUM_OVL);
    memset(object->data, 0, 0x900);
    object->mdlId[0] = Hu3DHookFuncCreate(UpdateWindCollisionMap);
    Hu3DModelLayerSet(object->mdlId[0], 7);
    object->objFunc = EmptyWindMapCallback;
}

u32 unreferencedRandomMultiplier = 0x41C64E6D;

void EmptyWindMapCallback(OMOBJ *object) { }

Vec collisionMapOrigin = { -850.0f, 0.0f, -850.0f };
s32 neighborPathCellOffsets[25] = {
    -0x62,
    -0x61,
    -0x60,
    -0x5F,
    -0x5E,
    -0x32,
    -0x31,
    -0x30,
    -0x2F,
    -0x2E,
    -0x2,
    -0x1,
    0x0,
    0x1,
    0x2,
    0x2E,
    0x2F,
    0x30,
    0x31,
    0x32,
    0x5E,
    0x5F,
    0x60,
    0x61,
    0x62,
};

u32 neighborPathCosts[25] = {
    0x50,
    0x64,
    0x78,
    0x64,
    0x50,
    0x64,
    0x8C,
    0xA0,
    0x8C,
    0x64,
    0x78,
    0xA0,
    0xB4,
    0xA0,
    0x78,
    0x64,
    0x8C,
    0xA0,
    0x8C,
    0x64,
    0x50,
    0x64,
    0x78,
    0x64,
    0x50,
};

#define getData(v, off)                                                                                                                              \
    &(&(*omObjGetDataAs(windCollisionMapObject, M438CollisionGrid))[(u32)(0.028235294f * (v.x - off))])[(u32)(0.028235294f * (v.z - off)) * 0x30]

void UpdateWindCollisionMap(HU3DMODEL *model, Mtx matrix)
{
    Vec windAdjustedPosition;
    Vec pathOffset;
    M438WindEffectWork *effectWork;
    f32 pathDistance;
    OMOBJ *playerObject;
    s32 neighborIndex;
    s32 effectIndex;
    u8 *mapCell;
    u8 *pathMapCell;
    M438PlayerWork *playerWork;

    if ((u8)omPauseChk() == 0) {
        memset(windCollisionMapObject->data, 0, 0x900);
        if (scenePhase == 7) {
            effectWork = windEffectRecords;
            for (effectIndex = 0; effectIndex < 0x30; effectIndex++, effectWork++) {
                if ((effectWork->effectState > 1U) && (effectWork->effectState < 5U)) {

                    pathDistance = 0.0f;
                    for (pathDistance = 0.0f; pathDistance < (35.416668f + effectWork->pathSegmentLength); pathDistance += 35.416668f) {
                        VECScale(&effectWork->pathSegmentDirection, &pathOffset, pathDistance);
                        VECAdd(&effectWork->pathSegmentStart, &pathOffset, &windAdjustedPosition);
                        VECSubtract(&windAdjustedPosition, &collisionMapOrigin, &windAdjustedPosition);

                        // @todo make this look nicer
                        // pathMapCell = getData(windAdjustedPosition, 0);
                        pathMapCell = getData(windAdjustedPosition, 0);
                        for (neighborIndex = 0; neighborIndex < 0x19; neighborIndex++) {
                            mapCell = &pathMapCell[neighborPathCellOffsets[neighborIndex]];
                            if (mapCell[0] < neighborPathCosts[neighborIndex]) {
                                mapCell[0] = neighborPathCosts[neighborIndex];
                            }
                        }
                    }
                }
            }
            for (effectIndex = 0; effectIndex < 4; effectIndex++) {
                if ((OMOBJ *)playerObjects[effectIndex]) {
                    playerObject = playerObjects[effectIndex];
                    playerWork = playerObject->data;
                    if ((playerWork) && (playerWork->windUpdatePending != 0)) {
                        playerWork->windUpdatePending = 0;
                        VECAdd(&playerWork->worldPosition, &playerWork->windVelocity, &windAdjustedPosition);
                        playerWork->windActive = FindWindAvoidancePosition(playerObject->work[0], playerWork->modelRotationY, &windAdjustedPosition, &playerWork->windTargetPosition);
                    }
                }
            }
        }
    }
}

u32 CalculateWindCollisionForce(Vec *playerPosition, f32 windStrength)
{
    Vec targetPosition;
    Vec segmentVector;
    Vec normalizedDirection;
    Vec wallNormal;
    f32 distanceToSegment;
    f32 projectionFraction;
    f32 collisionWeight;
    f32 collisionFalloffRange;
    s32 hasDirectionVector;
    u32 collisionForce;
    M438CharacterCameraWork *cameraWork;

    collisionForce = 0;
    hasDirectionVector = 0;
    collisionFalloffRange = 100.0f * (4.5f - (2.0f * windStrength));
    distanceToSegment = VECMag(playerPosition);
    if (distanceToSegment > 280.0f) {
        if (distanceToSegment > 560.0f) {
            hasDirectionVector = 1;
            VECNormalize(playerPosition, &normalizedDirection);
        }
        distanceToSegment -= 280.0f;
        distanceToSegment *= 0.12987013f;
        if (distanceToSegment > 50.0f) {
            distanceToSegment = 500.0f;
        }
        collisionForce = distanceToSegment;
    }
    if (scenePhase == 7) {
        cameraWork = cameraControllerObject->data;
        targetPosition = cameraWork->targetPosition;
        targetPosition.y = 0.0f;
        switch (cameraWork->cameraMode) {
            case 2:
            case 3:
                wallNormal.x = sind(cameraWork->cameraAngle);
                wallNormal.y = 0.0f;
                wallNormal.z = cosd(cameraWork->cameraAngle);
                VECScale(&wallNormal, &segmentVector, 600.0f + (100.0f * (8.0f * windStrength)));
                projectionFraction = ProjectPointOnSegment(playerPosition, &targetPosition, &segmentVector);
                if (projectionFraction > -0.2f && projectionFraction < 1.0f) {
                    VECScale(&segmentVector, &segmentVector, projectionFraction);
                    VECAdd(&targetPosition, &segmentVector, &targetPosition);
                    VECSubtract(&targetPosition, playerPosition, &segmentVector);
                    distanceToSegment = VECMag(&segmentVector);
                    if (distanceToSegment < collisionFalloffRange) {
                        collisionWeight = 0.0f;
                        if (hasDirectionVector != 0) {
                            collisionWeight = VECDotProduct(&normalizedDirection, &wallNormal);
                            collisionWeight = 1.0 - fabs(collisionWeight);
                        }
                        distanceToSegment = (1.0f / collisionFalloffRange) * (collisionFalloffRange - distanceToSegment);
                        distanceToSegment = 0.5f * (distanceToSegment + (1.0f - ((1.0f - distanceToSegment) * (1.0f - distanceToSegment))));
                        distanceToSegment += (1.0f - distanceToSegment) * collisionWeight;
                        distanceToSegment = 100.0f + 50.0f * distanceToSegment;
                        projectionFraction = 0.5 + 0.5 * (1.0 - fabs(projectionFraction));
                        distanceToSegment *= projectionFraction;
                        if (distanceToSegment > 150.0f) {
                            distanceToSegment = 150.0f;
                        }
                        collisionForce += distanceToSegment;
                    }
                }
                break;
            default:
                VECSubtract(playerPosition, &targetPosition, &segmentVector);
                distanceToSegment = VECMag(&segmentVector);
                if (distanceToSegment < 600.0f) {
                    distanceToSegment = 800.0f - distanceToSegment;
                    distanceToSegment = 20.0f + (0.1f * distanceToSegment);
                    if (distanceToSegment > 100.0f) {
                        distanceToSegment = 100.0f;
                    }
                    collisionForce += distanceToSegment;
                }
                break;
        }
    }
    return collisionForce;
}

s32 neighborCollisionCellOffsets[9] = { 0, -49, -48, -47, -1, 1, 0x2F, 0x30, 0x31 };

Vec neighborWorldOffsets[9] = {
    { 0.0f, 0.0f, 0.0f },
    { -35.416668f, 0.0f, -35.416668f },
    { 0.0f, 0.0f, -35.416668f },
    { 35.416668f, 0.0f, -35.416668f },
    { -35.416668f, 0.0f, 0.0f },
    { 35.416668f, 0.0f, 0.0f },
    { -35.416668f, 0.0f, 35.416668f },
    { 0.0f, 0.0f, 35.416668f },
    { 35.416668f, 0.0f, 35.416668f },
};

s32 extendedCollisionCellOffsets[9] = { 0, -98, -144, -94, -3, 3, 0x5E, 0x90, 0x62 };

Vec extendedWorldOffsets[9] = {
    { 0.0f, 0.0f, 0.0f },
    { -70.833336f, 0.0f, -70.833336f },
    { 0.0f, 0.0f, -106.25f },
    { 70.833336f, 0.0f, -70.833336f },
    { -106.25f, 0.0f, 0.0f },
    { 106.25f, 0.0f, 0.0f },
    { -70.833336f, 0.0f, 70.833336f },
    { 0.0f, 0.0f, 106.25f },
    { 70.833336f, 0.0f, 70.833336f },
};

s32 collisionRiskOffsets[9] = { 0, -19, -18, -17, -1, 1, 0x11, 0x12, 0x13 };
s32 extendedCollisionRiskOffsets[9] = { 0, -38, -54, -34, -3, 3, 0x22, 0x36, 0x26 };

s32 FindWindAvoidancePosition(u32 playerIndex, f32 windStrength, Vec *originPosition, Vec *avoidancePosition)
{
    s32 candidateRisks[9];
    Vec samplePosition;
    Vec originPosition;
    Vec candidatePosition;
    Vec searchPosition;
    Vec playerSeparation;
    u8 *collisionCell;
    u32 candidateRow;
    u32 candidateColumn;
    f32 validNeighborCount;
    f32 playerDistance;
    f32 neighborRisk;
    f32 playerMovementScale;
    s32 neighborRiskValue;
    s32 candidateIndex;
    u32 totalCandidateRisk;
    s32 neighborIndex;
    u16 *collisionMapCell;
    s32 playerIndex;
    M438PlayerWork *playerWork;
    u8 *neighborCell;
    s32 searchLayer;
    u8 *candidateCellPointer;
    u16 *candidateRiskPointer;
    u32 minimumRisk;
    u32 bestCandidateIndex;
    u8 *boardCollisionMap;

    memset(collisionRiskScratch, 0, 0x288);
    originPosition = *originPosition;
    candidateColumn = 0.028235294f * (originPosition.x - -850.0f);
    candidateRow = 0.028235294f * (originPosition.z - -850.0f);
    boardCollisionMap = &(&(*omObjGetDataAs(windCollisionMapObject, M438CollisionGrid))[candidateColumn])[candidateRow * 0x30];
    originPosition.y = 0.0f;
    *avoidancePosition = originPosition;
    playerWork = playerObjects[playerIndex]->data;
    playerMovementScale = playerWork->difficultyMovementScale;
    minimumRisk = 0x2710;
    bestCandidateIndex = 0;

    for (candidateIndex = 0; candidateIndex < 9; candidateIndex++) {
        VECAdd(&originPosition, &neighborWorldOffsets[candidateIndex], &candidatePosition);
        collisionCell = boardCollisionMap + neighborCollisionCellOffsets[candidateIndex];
        (void)boardCollisionMap;
        candidateRiskPointer = &(&collisionRiskScratch[collisionRiskOffsets[candidateIndex]])[0xAB];
        validNeighborCount = 0.0f;
        totalCandidateRisk = 0;
        candidateCellPointer = collisionCell;
        searchPosition = candidatePosition;

        for (searchLayer = 0; searchLayer < 3; searchLayer++) {
            for (neighborIndex = 0; neighborIndex < 9; neighborIndex++) {
                neighborCell = candidateCellPointer + neighborCollisionCellOffsets[neighborIndex];
                VECAdd(&searchPosition, &neighborWorldOffsets[neighborIndex], &samplePosition);
                if ((windCollisionMapObject->data <= neighborCell)
                    && (((u8 *)windCollisionMapObject->data + 0x900 > neighborCell) && (fabs(samplePosition.x) < 850.0) && (fabs(samplePosition.z) < 850.0))) {
                    validNeighborCount++;
                    collisionMapCell = &candidateRiskPointer[collisionRiskOffsets[neighborIndex]];
                    if (*collisionMapCell == 0) {
                        neighborRiskValue = CalculateWindCollisionForce(&samplePosition, playerMovementScale);
                        neighborRiskValue += *neighborCell;
                        neighborRisk = 0.0f;
                        if (neighborRiskValue < 0x3C) {
                            neighborRisk = (1.0f / 60.0f) * (0x3C - neighborRiskValue);
                        }
                        neighborRisk = 80.0f + (100.0f * (2.5f * neighborRisk));
                        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                            if (playerIndex != playerIndex) {
                                playerWork = playerObjects[playerIndex]->data;
                                VECSubtract(&samplePosition, &playerWork->worldPosition, &playerSeparation);
                                playerSeparation.y = 0.0f;
                                playerDistance = VECMag(&playerSeparation);
                                if (playerDistance < neighborRisk) {
                                    playerDistance = neighborRisk - playerDistance;
                                    playerDistance *= (40.0f / neighborRisk);
                                    neighborRiskValue += playerDistance;
                                }
                            }
                        }
                        *collisionMapCell = ++neighborRiskValue;
                    }
                    else {
                        neighborRiskValue = *collisionMapCell;
                    }
                    totalCandidateRisk += neighborRiskValue;
                    if ((neighborIndex == 0) && (searchLayer == 0)) {
                        totalCandidateRisk += neighborRiskValue * 3;
                        validNeighborCount += 3.0f;
                    }
                }
            }
            VECAdd(&searchPosition, &extendedWorldOffsets[candidateIndex], &searchPosition);
            candidateCellPointer += extendedCollisionCellOffsets[candidateIndex];
            candidateRiskPointer = &candidateRiskPointer[extendedCollisionRiskOffsets[candidateIndex]];
            validNeighborCount *= 2.0f;
            totalCandidateRisk *= 2.0f;
        }
        if (validNeighborCount > 1.0f) {
            totalCandidateRisk /= validNeighborCount;
        }
        if (minimumRisk > totalCandidateRisk) {
            bestCandidateIndex = candidateIndex;
            minimumRisk = totalCandidateRisk;
            *avoidancePosition = candidatePosition;
        }
        candidateRisks[candidateIndex] = totalCandidateRisk;
    }
    if (bestCandidateIndex != 0) {
        return 1;
    }
    return 0;
}

f32 ProjectPointOnSegment(Vec *point, Vec *segmentStart, Vec *segmentDirection)
{
    Vec vectorDifference;
    f32 negativeDirectionMagnitudeSquared;
    f32 projectionFraction;

    VECSubtract(segmentStart, point, &vectorDifference);
    projectionFraction = VECDotProduct(&vectorDifference, segmentDirection);
    negativeDirectionMagnitudeSquared = -VECSquareMag(segmentDirection);
    if (negativeDirectionMagnitudeSquared != 0.0f) {
        projectionFraction /= negativeDirectionMagnitudeSquared;
    }
    return projectionFraction;
}

#include "REL/executor.h"
#include "REL/m403Dll.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/frand.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "ext_math.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/esprite.h"
#endif

typedef struct {
    /* 0x000 */ u8 useSplineInterpolation;
    /* 0x001 */ char characterId[3];
    /* 0x004 */ Vec cameraCenter;
    /* 0x010 */ Vec cameraRotation;
    /* 0x01C */ Vec startCameraCenter;
    /* 0x028 */ Vec startCameraRotation;
    /* 0x034 */ Vec targetCameraCenter;
    /* 0x040 */ Vec targetCameraRotation;
    /* 0x04C */ Vec splineTargetCenter;
    /* 0x058 */ Vec splineTargetRotation;
    /* 0x064 */ Vec splineStartCenter;
    /* 0x070 */ Vec splineControlCenter;
    /* 0x07C */ Vec splineEndCenter;
    /* 0x088 */ float centerSplineCoefficients[3][3];
    /* 0x0AC */ Vec splineStartRotation;
    /* 0x0B8 */ Vec splineControlRotation;
    /* 0x0C4 */ Vec splineEndRotation;
    /* 0x0D0 */ float rotationSplineCoefficients[3][3];
    /* 0x0F4 */ float centerSplineTimes[3];
    /* 0x100 */ float rotationSplineTimes[3];
    /* 0x10C */ float zoomSplineTimes[3];
    /* 0x118 */ float zoomSplineValues[3];
    /* 0x124 */ float zoomSplineDurations[3];
    /* 0x130 */ float cameraZoom;
    /* 0x134 */ float startCameraZoom;
    /* 0x138 */ float targetCameraZoom;
    /* 0x13C */ float intermediateCameraZoom;
    /* 0x140 */ float transitionSpeed;
    /* 0x144 */ float transitionDuration;
    /* 0x148 */ float transitionFramesRemaining;
} CameraTransitionWork; // Size 0x14C

typedef struct {
    /* 0x00 */ u8 playerIndex;
    /* 0x01 */ u8 characterId;
    /* 0x02 */ u8 actorType;
    /* 0x03 */ u8 reservedStatus;
    /* 0x04 */ u8 controllerPort;
    /* 0x05 */ u8 isComputerPlayer;
    /* 0x06 */ u8 playerDifficulty;
    /* 0x07 */ u8 actorStateFlags;
    /* 0x08 */ char alignmentPadding[2];
    /* 0x0A */ u8 actorActionTimer;
    /* 0x0B */ char actionPadding[1];
    /* 0x0C */ u8 isAirborne;
    /* 0x0D */ u8 landingState;
    /* 0x0E */ u8 nearbyActorCount;
    /* 0x0F */ char motionPadding[1];
    /* 0x10 */ s16 stickX;
    /* 0x12 */ s16 stickY;
    /* 0x14 */ s16 buttonsPressed;
    /* 0x16 */ s16 buttonsHeld;
    /* 0x18 */ s16 motionState;
    /* 0x1A */ s16 previousMotionState;
    /* 0x1C */ float movementSpeed;
    /* 0x20 */ float movementAngle;
    /* 0x24 */ float movementPhase;
    /* 0x28 */ float motionDuration;
    /* 0x2C */ Vec actorPosition;
    /* 0x38 */ float rotationX;
    /* 0x3C */ float rotationY;
    /* 0x40 */ float rotationZ;
    /* 0x44 */ float horizontalVelocityX;
    /* 0x48 */ float verticalVelocity;
    /* 0x4C */ float horizontalVelocityZ;
    /* 0x50 */ Vec previousPosition;
    /* 0x5C */ s16 computerState; // Start of a struct of size 0x40 (memset)?
    /* 0x5E */ s16 computerStateTimer;
    /* 0x60 */ float targetPositionX;
    /* 0x64 */ float targetPositionZ;
    /* 0x68 */ char computerData[0x34];
    /* 0x9C */ u8 isFinalActivePlayer;
    /* 0x9D */ char finalPlayerPadding[3];
    /* 0xA0 */ s32 isNearbyCollision;
} PlayerActorWork; // Size 0xA4

static void InitializeGameObject(OMOBJ *object);
static void UpdateGameObject(OMOBJ *object);
static void UpdateRoundFlow(OMOBJ *object);
static void InitializeRoundObjects(OMOBJ *object);
static void UpdateRoundObjects(OMOBJ *object);
static void UpdatePlayerCamera(OMOBJ *object);
static void UpdateCameraTransition(OMOBJ *object);
static void UpdateCameraTransitionState(OMOBJ *object);
static void InterpolateCameraValues(float parameter1, Vec *parameter2, Vec *parameter3, float parameter4);
static void InterpolateCameraSpline(float parameter1, Vec *parameter2, Vec *parameter3, float parameter4, Vec *parameter5, Vec *parameter6, float parameter7);
static void SetCameraZoom(float parameter1);
static void UpdateCameraSetup(OMOBJ *object);
static void UpdateCameraSpline(OMOBJ *object);
static void UpdateCameraTransitionObject(OMOBJ *object);
static void InitializePlayerActor(OMOBJ *object);
static void UpdatePlayerActor(OMOBJ *object);
static void CheckRoundCompletion(void);
static s32 CountActivePlayers(void);
static void UpdatePlayerStates(OMOBJ *object);
static void FindNearestSpawnPosition(Vec *parameter1, Vec *parameter2);
static void UpdatePlayerBoundaryScale(OMOBJ *object);
static s16 CreateParticleEffect(s32 parameter1, s16 parameter2, HU3DPARTICLEHOOK parameter3);
static void UpdateParticleTransform(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
static s32 GetRandomValue(void);
static float InterpolateAngle(float parameter1, float parameter2, float parameter3);
static void BuildRotationMatrix(Mtx parameter1, float parameter2, float parameter3, float parameter4);
static void ExtractRotationAngles(float *parameter1, float *parameter2);
static float EvaluateBezierCurve(float parameter1, float parameter2, float parameter3, float parameter4);
static void EvaluateBezierCurvePoints(float *parameter1, float *parameter2, float *parameter3, float *parameter4, float parameter5);
static float EvaluateBezierDerivative(float parameter1, float parameter2, float parameter3, float parameter4);
static void EvaluateBezierDerivatives(float *parameter1, float *parameter2, float *parameter3, float *parameter4, float parameter5);
static void CalculateSplineCoefficients(float *parameter1, float *parameter2, float *parameter3, s32 parameter4);
static float EvaluateSpline(float parameter1, float *parameter2, float *parameter3, float *parameter4, s32 parameter5);
static void BuildVectorSpline(float *parameter1, Vec *parameter2, float parameter3[][3], s32 parameter4);
static void EvaluateVectorSpline(float parameter1, Vec *parameter2, float *parameter3, Vec *parameter4, float parameter5[][3], s32 parameter6);
static float NormalizeVector(Vec *parameter1);
static void InterpolateVector(Vec parameter1, Vec parameter2, Vec *parameter3, float parameter4);
static float ProjectPointOntoLine(Vec parameter1, Vec parameter2, Vec parameter3);
static float DistanceToLineSegment(Vec *parameter1, Vec *parameter2, Vec *parameter3);
static float DistanceToLineSegmentAndDirection(Vec *parameter1, Vec *parameter2, Vec *parameter3, Vec *parameter4, Vec *parameter5);

static OMOBJ *roundControllerObject;
static OMOBJ *cameraControllerObject;
static OMOBJ *cameraTransitionObject;
static OMOBJ *roundSceneObject;
static OMOBJ *playerObjects[4];
static u16 roundState;
static s16 roundTimer;
static s16 cameraTransitionState;
static s16 activePlayerCount;
static s16 cameraTransitionTimer;
static s16 playerScores[6];
static s16 playerRoundResults[2];
static u8 reservedRoundStateData[8];
static s32 roundRandomSeed;
static u8 reservedRoundFlags[6];
static s16 winningPlayerIndex;
static s16 isRoundComplete;
static s16 roundSequenceState;

static s16 remainingRoundCount = -1;
static Vec cameraLightPosition = { 0.0f, 4000.0f, 4000.0f };
static Vec cameraLightDirection = { 0.0f, -0.7f, -0.6f };
static GXColor cameraLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };

static Vec reservedLightPosition = { 0.0f, 4000.0f, -2000.0f };
static Vec reservedCameraUp = { 0.0f, 1.0f, 0.0f };
static u8 reservedSceneData[0xC] = { 0 };

void ObjectSetup(void)
{
    HU3DLIGHT *globalLight;
    HUPROCESS *objectManager;
    OMOBJ *playerObject;
    s32 randomEffectCount;
    s32 globalLightId;
    s32 i;

    OSReport("******* M403ObjectSetup *********\n");
    Hu3DLighInit();
    globalLightId = Hu3DGLightCreateV(&cameraLightPosition, &cameraLightDirection, &cameraLightColor);
    Hu3DGLightInfinitytSet(globalLightId);
    globalLight = &Hu3DGlobalLight[0];
    globalLight->type |= 0x8000;
    objectManager = omInitObjMan(50, 0x2000);
    randomEffectCount = frand() & 0x1F;
    for (i = 0; i < randomEffectCount; i++) {
        GetRandomValue();
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
    cameraControllerObject = omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutView);
    roundControllerObject = omAddObjEx(objectManager, 0x10, 8, 0, -1, InitializeGameObject);
    roundSceneObject = omAddObjEx(objectManager, 0x20, 16, 0, -1, UpdateRoundObjects);
    cameraTransitionObject = omAddObjEx(objectManager, 0x50, 16, 0, -1, UpdateCameraTransition);
    omAddObjEx(objectManager, 0x60, 0, 0, -1, UpdatePlayerStates);
    CreatePuzzleScene(objectManager);
    SetScenePieceCount(3);
    omMakeGroupEx(objectManager, 0, 4);
    omGetGroupMemberListEx(objectManager, 0);
    for (i = 0; i < 4; i++) {
        playerObject = omAddObjEx(objectManager, 0x30, 2, 8, 0, UpdateCameraSetup);
        playerObjects[i] = playerObject;
        playerObject->work[0] = i;
        playerObject->work[1] = i;
    }
    omGameSysInit(objectManager);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    HuAudSndGrpSet(0x1C);
    isRoundComplete = 0;
    if (GWMGRecordGet(9) == 0) {
        GWMGRecordSet(9, 5);
    }
}

static void InitializeGameObject(OMOBJ *object)
{
    roundState = 0;
    object->objFunc = UpdateGameObject;
}

static void UpdateGameObject(OMOBJ *object)
{
    Vec cameraCenter;
    Vec cameraRotation;
    Vec targetCameraCenter;
    Vec targetCameraRotation;
    float cameraZoom;
    float targetCameraZoom;

    switch (roundState) {
        case 0:
            cameraCenter.x = 0.0f;
            cameraCenter.y = 0.0f;
            cameraCenter.z = -50.0f;
            cameraRotation.x = -90.0f;
            cameraRotation.y = 90.0f;
            cameraRotation.z = 0.0f;
            cameraZoom = 2200.0f;
            InterpolateCameraValues(1.0f, &cameraCenter, &cameraRotation, cameraZoom);
            cameraTransitionTimer = REFRESH_RATE;
            roundState++;
            break;
        case 1:
            if (--cameraTransitionTimer == 0) {
                roundState++;
            }
            break;
        case 2:
            if (cameraTransitionTimer == 0) {
                ResetSceneState();
                cameraTransitionTimer = (REFRESH_RATE*3)/2;
                cameraCenter.x = 200.0f;
                cameraCenter.y = 100.0f;
                cameraCenter.z = -50.0f;
                cameraRotation.x = -50.0f;
                cameraRotation.y = 90.0f;
                cameraRotation.z = 0.0f;
                cameraZoom = 2000.0f;
                targetCameraCenter.x = 0.0f;
                targetCameraCenter.y = 320.0f;
                targetCameraCenter.z = 147.0f;
                targetCameraRotation.x = -23.5f;
                targetCameraRotation.y = 0.0f;
                targetCameraRotation.z = 0.0f;
                targetCameraZoom = 1720.0f;
                InterpolateCameraSpline(REFRESH_RATE*3, &targetCameraCenter, &targetCameraRotation, targetCameraZoom, &cameraCenter, &cameraRotation, cameraZoom);
            }
            if (--cameraTransitionTimer == 0) {
                roundState++;
            }
            break;
        case 3:
            if (cameraTransitionTimer == 0) {
                cameraTransitionTimer = (REFRESH_RATE*3)/2;
            }
            if (--cameraTransitionTimer == 0) {
                roundState++;
            }
            break;
        case 4:
            roundState = 5;
            roundTimer = cameraTransitionState = -1;
            object->objFunc = UpdateRoundFlow;
            HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M403, 19), HU_MEMNUM_OVL));
            break;
    }
    if (omSysExitReq != 0 && WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = InitializeRoundObjects;
    }
}

static s32 roundAudioSequenceId = -1;

s32 characterPortraitAssetIds[] = { DATA_MAKE_NUM(DATADIR_MGCONST, 0), DATA_MAKE_NUM(DATADIR_MGCONST, 1), DATA_MAKE_NUM(DATADIR_MGCONST, 2),
    DATA_MAKE_NUM(DATADIR_MGCONST, 3), DATA_MAKE_NUM(DATADIR_MGCONST, 4), DATA_MAKE_NUM(DATADIR_MGCONST, 5), DATA_MAKE_NUM(DATADIR_MGCONST, 6),
    DATA_MAKE_NUM(DATADIR_MGCONST, 7) };

static void UpdateRoundFlow(OMOBJ *object)
{
    Vec screenPosition;
    Vec cameraRotation;
    float cameraZoom;
    s16 winningCharacters[4];
    s32 spriteId;
    s32 winnerCount;
    s32 activePlayerCount;
    s32 i;

    switch (roundState) {
        case 5:
            if (cameraTransitionState == -1) {
                activePlayerCount = REFRESH_RATE*30;
                cameraTransitionState = MGSeqStartCreate();
                MGSeqPosSet(cameraTransitionState, 320.0f, 240.0f);
            }
            else if (MGSeqStatGet(cameraTransitionState) == 0) {
                cameraTransitionState = -1;
                roundState++;
                cameraTransitionTimer = 0;
                remainingRoundCount = 4;
                if (isRoundComplete != 0) {
                    screenPosition.x = 476.0f;
                    screenPosition.y = 64.0f;
                    spriteId = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 51), 2, 0);
                    playerScores[0] = spriteId;
                    espPosSet(spriteId, screenPosition.x + 32.0f, screenPosition.y);
                    spriteId = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 51), 2, 0);
                    playerScores[1] = spriteId;
                    espPosSet(spriteId, screenPosition.x + 56.0f, screenPosition.y);
                    spriteId = espEntry(characterPortraitAssetIds[GWPlayerCfg[winningPlayerIndex].character], 1, 0);
                    playerScores[2] = spriteId;
                    espPosSet(spriteId, screenPosition.x, screenPosition.y);
                    spriteId = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 22), 3, 0);
                    playerScores[3] = spriteId;
                    espPosSet(spriteId, screenPosition.x + 36.0f, screenPosition.y);
                    spriteId = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 23), 4, 0);
                    playerScores[4] = spriteId;
                    espColorSet(spriteId, 0, 0, 0);
                    espTPLvlSet(spriteId, 0.5);
                    espPosSet(spriteId, screenPosition.x + 36.0f, screenPosition.y);
                }
            }
            if (roundAudioSequenceId == -1 && (MGSeqStatGet(cameraTransitionState) & 0x10)) {
                roundAudioSequenceId = HuAudSeqPlay(0x48);
            }
            break;
        case 6:
            activePlayerCount = 0;
            if (isRoundComplete != 0) {
                if (((PlayerActorWork *)playerObjects[winningPlayerIndex]->data)->actorStateFlags == 0 && getScenePieceCountPublic() == 0) {
                    activePlayerCount++;
                }
            }
            else if (remainingRoundCount <= 1 && getScenePieceCountPublic() == 0) {
                activePlayerCount++;
            }
            if (isRoundComplete != 0) {
                espBankSet(playerScores[0], roundSequenceState / 10);
                espBankSet(playerScores[1], roundSequenceState % 10);
            }
            if (activePlayerCount != 0) {
                roundState++;
            }
            break;
        case 7:
            if (cameraTransitionState == -1) {
                cameraTransitionState = MGSeqFinishCreate();
                MGSeqPosSet(cameraTransitionState, 320.0f, 240.0f);
                MGSeqKill(roundTimer);
                HuAudSeqFadeOut(roundAudioSequenceId, 100);
                break;
            }
            if (MGSeqStatGet(cameraTransitionState) == 0) {
                cameraTransitionState = -1;
                if (isRoundComplete == 0) {
                    if (remainingRoundCount != 0) {
                        for (i = 0; i < 4; i++) {
                            if (((PlayerActorWork *)playerObjects[i]->data)->actorStateFlags != 0) {
                                break;
                            }
                        }
                        screenPosition.x = playerObjects[i]->trans.x;
                        screenPosition.y = 100.0f;
                        screenPosition.z = playerObjects[i]->trans.z;
                        cameraRotation.x = -18.0f;
                        cameraRotation.y = 0.0f;
                        cameraRotation.z = 0.0f;
                        cameraZoom = 600.0f;
                        InterpolateCameraValues((REFRESH_RATE*3.0f)/4, &screenPosition, &cameraRotation, cameraZoom);
                    }
                    roundState = 9;
                    cameraTransitionTimer = (REFRESH_RATE*3.0f)/2;
                    break;
                }
                if ((s32)GWMGRecordGet(9) < roundSequenceState - 1) {
                    GWMGRecordSet(9, roundSequenceState - 1);
                    roundState = 8;
                    break;
                }
                cameraTransitionState = -1;
                roundState = 11;
                cameraTransitionTimer = (REFRESH_RATE*5)/2;
            }
            break;
        case 8:
            if (cameraTransitionState == -1) {
                cameraTransitionState = MGSeqRecordCreate(roundSequenceState - 1);
                break;
            }
            if (MGSeqStatGet(cameraTransitionState) == 0) {
                cameraTransitionState = -1;
                cameraTransitionTimer = (REFRESH_RATE*5)/2;
                roundState = 11;
            }
            break;
        case 9:
            if (--cameraTransitionTimer == 0) {
                roundState++;
            }
            break;
        case 10:
            if (cameraTransitionState == -1) {
                if (remainingRoundCount == 0) {
                    cameraTransitionState = MGSeqDrawCreate();
                    HuAudSStreamPlay(4);
                }
                else {
                    for (i = 0; i < 4; i++) {
                        winningCharacters[i] = -1;
                    }
                    for (i = 0, winnerCount = 0; i < 4; i++) {
                        if (((PlayerActorWork *)playerObjects[i]->data)->actorStateFlags != 0) {
                            GWPlayerCoinWinAdd(i, 10);
                            winningCharacters[winnerCount++] = GWPlayerCfg[i].character;
                        }
                    }
                    cameraTransitionState = MGSeqWinCreate(winningCharacters[0], winningCharacters[1], winningCharacters[2], winningCharacters[3]);
                    HuAudSStreamPlay(1);
                }
            }
            cameraTransitionState = -1;
            cameraTransitionTimer = (REFRESH_RATE*7)/2;
            roundState++;
            break;
        case 11:
            if (--cameraTransitionTimer == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = InitializeRoundObjects;
            }
            break;
    }
    if (omSysExitReq != 0 && WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = InitializeRoundObjects;
    }
}

static void InitializeRoundObjects(OMOBJ *object)
{
    if (WipeStatGet() == 0) {
        DestroyPuzzleScene();
        CharModelKill(-1);
        MGSeqKill(roundTimer);
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

static void UpdateRoundObjects(OMOBJ *object)
{
    Hu3DLightAllKill();
    playerRoundResults[0] = Hu3DGLightCreateV(&cameraLightPosition, &cameraLightDirection, &cameraLightColor);
    Hu3DGLightStaticSet(playerRoundResults[0], 1);
    Hu3DGLightInfinitytSet(playerRoundResults[0]);
    playerRoundResults[1] = Hu3DGLightCreate(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0, 0, 0);
    Hu3DGLightStaticSet(playerRoundResults[1], 1);
    Hu3DGLightPointSet(playerRoundResults[1], 0.0f, 0.0f, 3);
    Hu3DGLightColorSet(playerRoundResults[0], 0, 0, 0, 0xFF);
    Hu3DGLightPosSet(playerRoundResults[1], -1470.0f, 800.0f, -2000.0f, 0.0f, -1.0f, 0.0f);
    Hu3DGLightColorSet(playerRoundResults[1], 0, 0, 0, 0xFF);
    Hu3DGLightPointSet(playerRoundResults[1], 1200.0f, 0.6f, 3);
    object->objFunc = UpdatePlayerCamera;
}

static void UpdatePlayerCamera(OMOBJ *object)
{
    s8 countdownDigitSequence[] = { 1, 0, 2, 1, 0, 3, 2, 1, 0 };
    s8 countdownDisplaySequence[] = { 1, 1, 2, 2, 2, 3, 3, 3, 3 };
    s32 countdownStep;

    switch (roundState) {
        case 0:
            break;
        case 1:
            Hu3DGLightColorSet(playerRoundResults[0], 0xF0, 0xF0, 0xF0, 0xFF);
            break;
        case 2:
            break;
        case 3:
            Hu3DGLightColorSet(playerRoundResults[1], 0xFF, 0xE0, 0xD0, 0xFF);
            break;
        case 4:
            break;
        case 5:
            object->work[0] = 30;
            object->work[1] = 17;
            roundRandomSeed = 1;
            break;
        case 6:
            if (getScenePieceCountPublic() != 0) {
                break;
            }
            if (roundRandomSeed == 0) {
                roundRandomSeed = REFRESH_RATE/2;
                roundSequenceState++;
            }
            roundRandomSeed--;
            if (roundRandomSeed == 0) {
                countdownStep = (object->work[1] - 2) / 2;
                if (countdownStep < 0) {
                    countdownStep = 0;
                }
                SetActivePieceCount(countdownDigitSequence[countdownStep]);
                SetScenePieceCount(countdownDisplaySequence[countdownStep]);
                #if VERSION_NTSC
                SetSceneSpeed(0.01f * object->work[0]);
                #else
                SetSceneSpeed((0.6f * object->work[0])*(1.0f/REFRESH_RATE));
                #endif
                object->work[0] += 5;
                if (object->work[1] > 2) {
                    object->work[1]--;
                }
            }
            break;
    }
}

static void UpdateCameraTransition(OMOBJ *object)
{
    CameraTransitionWork *cameraTransition;

    cameraTransition = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*cameraTransition), HU_MEMNUM_OVL);
    object->data = cameraTransition;
    cameraTransition->transitionFramesRemaining = cameraTransition->transitionDuration = 0.0f;
    cameraTransition->transitionSpeed = 1.0f;
    cameraTransition->cameraRotation.x = cameraTransition->startCameraRotation.x = -23.5f;
    cameraTransition->cameraRotation.y = cameraTransition->startCameraRotation.y = 0.0f;
    cameraTransition->cameraRotation.z = cameraTransition->startCameraRotation.z = 0.0f;
    cameraTransition->cameraCenter.x = cameraTransition->startCameraCenter.x = 0.0f;
    cameraTransition->cameraCenter.y = cameraTransition->startCameraCenter.y = 320.0f;
    cameraTransition->cameraCenter.z = cameraTransition->startCameraCenter.z = 147.0f;
    cameraTransition->cameraZoom = cameraTransition->startCameraZoom = 1720.0f;
    object->objFunc = UpdateCameraTransitionState;
}

static void UpdateCameraTransitionState(OMOBJ *object)
{
    CameraTransitionWork *cameraTransition;
    float transitionProgress;

    cameraTransition = object->data;
    if (cameraTransition->transitionFramesRemaining > 0.0f) {
        cameraTransition->transitionFramesRemaining -= cameraTransition->transitionSpeed;
        transitionProgress = (cameraTransition->transitionDuration - cameraTransition->transitionFramesRemaining) / cameraTransition->transitionDuration;
        transitionProgress = sind(90.0f * transitionProgress);
        if (cameraTransition->useSplineInterpolation == 0) {
            cameraTransition->cameraCenter.x = cameraTransition->startCameraCenter.x + transitionProgress * (cameraTransition->targetCameraCenter.x - cameraTransition->startCameraCenter.x);
            cameraTransition->cameraCenter.y = cameraTransition->startCameraCenter.y + transitionProgress * (cameraTransition->targetCameraCenter.y - cameraTransition->startCameraCenter.y);
            cameraTransition->cameraCenter.z = cameraTransition->startCameraCenter.z + transitionProgress * (cameraTransition->targetCameraCenter.z - cameraTransition->startCameraCenter.z);
            cameraTransition->cameraRotation.x = cameraTransition->startCameraRotation.x + transitionProgress * (cameraTransition->targetCameraRotation.x - cameraTransition->startCameraRotation.x);
            cameraTransition->cameraRotation.y = cameraTransition->startCameraRotation.y + transitionProgress * (cameraTransition->targetCameraRotation.y - cameraTransition->startCameraRotation.y);
            cameraTransition->cameraRotation.z = cameraTransition->startCameraRotation.z + transitionProgress * (cameraTransition->targetCameraRotation.z - cameraTransition->startCameraRotation.z);
            cameraTransition->cameraZoom = cameraTransition->startCameraZoom + transitionProgress * (cameraTransition->targetCameraZoom - cameraTransition->startCameraZoom);
        }
        else {
            EvaluateVectorSpline(2.0f * transitionProgress, &cameraTransition->cameraCenter, cameraTransition->centerSplineTimes, &cameraTransition->splineStartCenter, cameraTransition->centerSplineCoefficients, 3);
            EvaluateVectorSpline(2.0f * transitionProgress, &cameraTransition->cameraRotation, cameraTransition->rotationSplineTimes, &cameraTransition->splineStartRotation, cameraTransition->rotationSplineCoefficients, 3);
            cameraTransition->cameraZoom = EvaluateSpline(2.0f * transitionProgress, cameraTransition->zoomSplineDurations, cameraTransition->zoomSplineTimes, cameraTransition->zoomSplineValues, 3);
        }
    }
    CRot.x = cameraTransition->cameraRotation.x;
    CRot.y = cameraTransition->cameraRotation.y;
    CRot.z = cameraTransition->cameraRotation.z;
    Center.x = cameraTransition->cameraCenter.x;
    Center.y = cameraTransition->cameraCenter.y;
    Center.z = cameraTransition->cameraCenter.z;
    CZoom = cameraTransition->cameraZoom;
}

static void InterpolateCameraValues(float value, Vec *position, Vec *position2, float value2)
{
    CameraTransitionWork *cameraTransition;

    cameraTransition = cameraTransitionObject->data;
    cameraTransition->useSplineInterpolation = 0;
    cameraTransition->transitionDuration = value;
    cameraTransition->transitionFramesRemaining = value;
    cameraTransition->startCameraCenter = cameraTransition->cameraCenter;
    cameraTransition->targetCameraCenter.x = position->x;
    cameraTransition->targetCameraCenter.y = position->y;
    cameraTransition->targetCameraCenter.z = position->z;
    cameraTransition->startCameraRotation = cameraTransition->cameraRotation;
    cameraTransition->targetCameraRotation.x = position2->x;
    cameraTransition->targetCameraRotation.y = position2->y;
    cameraTransition->targetCameraRotation.z = position2->z;
    cameraTransition->startCameraZoom = cameraTransition->cameraZoom;
    cameraTransition->targetCameraZoom = value2;
}

static void InterpolateCameraSpline(float value, Vec *position, Vec *position2, float value2, Vec *position3, Vec *position4, float value3)
{
    CameraTransitionWork *cameraTransition;

    cameraTransition = cameraTransitionObject->data;
    cameraTransition->useSplineInterpolation = 1;
    cameraTransition->transitionFramesRemaining = cameraTransition->transitionDuration = value;
    cameraTransition->startCameraCenter = cameraTransition->cameraCenter;
    cameraTransition->targetCameraCenter.x = position->x;
    cameraTransition->targetCameraCenter.y = position->y;
    cameraTransition->targetCameraCenter.z = position->z;
    cameraTransition->splineTargetCenter.x = position3->x;
    cameraTransition->splineTargetCenter.y = position3->y;
    cameraTransition->splineTargetCenter.z = position3->z;
    cameraTransition->startCameraRotation = cameraTransition->cameraRotation;
    cameraTransition->targetCameraRotation.x = position2->x;
    cameraTransition->targetCameraRotation.y = position2->y;
    cameraTransition->targetCameraRotation.z = position2->z;
    cameraTransition->splineTargetRotation.x = position4->x;
    cameraTransition->splineTargetRotation.y = position4->y;
    cameraTransition->splineTargetRotation.z = position4->z;
    cameraTransition->startCameraZoom = cameraTransition->cameraZoom;
    cameraTransition->targetCameraZoom = value2;
    cameraTransition->intermediateCameraZoom = value3;
    cameraTransition->splineStartCenter = cameraTransition->startCameraCenter;
    cameraTransition->splineControlCenter = cameraTransition->splineTargetCenter;
    cameraTransition->splineEndCenter = cameraTransition->targetCameraCenter;
    cameraTransition->splineStartRotation = cameraTransition->startCameraRotation;
    cameraTransition->splineControlRotation = cameraTransition->splineTargetRotation;
    cameraTransition->splineEndRotation = cameraTransition->targetCameraRotation;
    cameraTransition->zoomSplineTimes[0] = cameraTransition->startCameraZoom;
    cameraTransition->zoomSplineTimes[1] = cameraTransition->intermediateCameraZoom;
    cameraTransition->zoomSplineTimes[2] = cameraTransition->targetCameraZoom;
    cameraTransition->zoomSplineDurations[0] = 0.0f;
    cameraTransition->zoomSplineDurations[1] = ABS(cameraTransition->zoomSplineTimes[1] - cameraTransition->zoomSplineTimes[0]);
    cameraTransition->zoomSplineDurations[2] = ABS(cameraTransition->zoomSplineTimes[2] - cameraTransition->zoomSplineTimes[1]) + cameraTransition->zoomSplineDurations[1];
    BuildVectorSpline(cameraTransition->centerSplineTimes, &cameraTransition->splineStartCenter, cameraTransition->centerSplineCoefficients, 3);
    BuildVectorSpline(cameraTransition->rotationSplineTimes, &cameraTransition->splineStartRotation, cameraTransition->rotationSplineCoefficients, 3);
    CalculateSplineCoefficients(cameraTransition->zoomSplineDurations, cameraTransition->zoomSplineTimes, cameraTransition->zoomSplineValues, 3);
}

static void SetCameraZoom(float value)
{
    CameraTransitionWork *cameraTransition;

    cameraTransition = cameraTransitionObject->data;
    cameraTransition->transitionSpeed = value;
}

static s32 playerMotionIds[] = { 0, 2, 3, 8, 10, 9, 23, 24 };

static Vec playerSpawnPositions[] = { { -300.0f, 2200.0f, 0.0f }, { -100.0f, 2200.0f, 0.0f }, { 100.0f, 2200.0f, 0.0f }, { 300.0f, 2200.0f, 0.0f } };

static float characterBoundaryInsets[] = { 40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 100.0f, 40.0f, 80.0f };

static float characterBoundaryScales[] = { 160.0f, 160.0f, 180.0f, 160.0f, 160.0f, 180.0f, 180.0f, 200.0f };

static u32 randomNumberSeed = 0x41C64E6D;

static void UpdateCameraSetup(OMOBJ *object)
{
    PlayerActorWork *playerWork;
    s32 playerIndex;
    s32 i;
    s32 characterId;

    playerWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*playerWork), HU_MEMNUM_OVL);
    object->data = playerWork;
    playerWork->playerIndex = object->work[0];
    playerWork->characterId = GWPlayerCfg[playerWork->playerIndex].character;
    playerWork->actorType = playerWork->playerIndex >> 1;
    playerWork->reservedStatus = playerWork->playerIndex & 1;
    playerWork->controllerPort = GWPlayerCfg[playerWork->playerIndex].pad_idx;
    playerWork->isComputerPlayer = GWPlayerCfg[playerWork->playerIndex].iscom;
    playerWork->playerDifficulty = GWPlayerCfg[playerWork->playerIndex].diff;
    playerWork->motionState = playerWork->previousMotionState = 0;
    playerWork->actorStateFlags = 1;
    playerWork->isAirborne = 1;
    playerWork->landingState = 0;
    playerWork->nearbyActorCount = 0;
    playerWork->actorActionTimer = 0;
    playerWork->isFinalActivePlayer = 0;
    playerIndex = object->work[1];
    playerWork->actorPosition.x = playerSpawnPositions[playerIndex].x;
    playerWork->actorPosition.y = playerSpawnPositions[playerIndex].y + 100.0f * ((GetRandomValue() - 0x8000) / 32768.0f);
    playerWork->actorPosition.z = playerSpawnPositions[playerIndex].z;
    playerWork->horizontalVelocityX = playerWork->verticalVelocity = playerWork->horizontalVelocityZ = 0.0f;
    playerWork->rotationX = playerWork->rotationY = playerWork->rotationZ = 0.0f;
    playerWork->movementAngle = playerWork->rotationY;
    playerWork->movementPhase = 0.0f;
    playerWork->movementSpeed = 0.0f;
    playerWork->motionDuration = 1.0f;
    memset(&playerWork->computerState, 0, 0x40);
    playerWork->isNearbyCollision = 1;
    if (isRoundComplete != 0 && playerWork->isComputerPlayer == 0) {
        winningPlayerIndex = playerWork->playerIndex;
    }
    characterId = CharModelCreate(playerWork->characterId, 4);
    object->mdlId[0] = characterId;
    Hu3DModelAttrSet(characterId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(characterId);
    for (i = 0; i < 8; i++) {
        object->mtnId[i] = CharMotionCreate(playerWork->characterId, playerMotionIds[i]);
    }
    CharMotionDataClose(playerWork->characterId);
    CharMotionSet(playerWork->characterId, object->mtnId[playerWork->motionState]);
    omSetTra(object, playerWork->actorPosition.x, playerWork->actorPosition.y, playerWork->actorPosition.z);
    Hu3DModelPosSet(characterId, playerWork->actorPosition.x, playerWork->actorPosition.y, playerWork->actorPosition.z);
    Hu3DModelRotSet(characterId, 0.0f, 30.0f, 0.0f);
    CharModelStepFxSet(playerWork->characterId, 1);
    CharMotionVoiceOnSet(playerWork->characterId, object->mtnId[3], 0);
    object->objFunc = UpdateCameraSpline;
}

static void UpdateCameraSpline(OMOBJ *object)
{
    PlayerActorWork *playerWork;
    s32 playerPosition;

    playerPosition = object->mdlId[0];
    playerWork = object->data;
    playerWork->stickX = playerWork->stickY = 0;
    playerWork->buttonsPressed = playerWork->buttonsHeld = 0;
    switch (roundState) {
        case 3:
            break;
        case 2:
            if (cameraTransitionTimer < (float)(REFRESH_RATE)) {
                break;
            }
            /* fallthrough */
        case 1:
            playerWork->motionState = 3;
            playerWork->verticalVelocity = 0.0f;
            playerWork->isAirborne = 0;
            break;
        case 5:
            object->objFunc = UpdateCameraTransitionObject;
            break;
    }
    if (playerWork->actorPosition.y < 10.0f) {
        omVibrate(playerWork->playerIndex, 12, 6, 6);
        playerWork->actorPosition.y = 10.0f;
        playerWork->isAirborne = 1;
    }
    UpdatePlayerActor(object);
}

static void UpdateCameraTransitionObject(OMOBJ *object)
{
    PlayerActorWork *playerWork;
    s32 modelId;
    s32 effectModelId;

    modelId = object->mdlId[0];
    playerWork = object->data;
    if (playerWork->actorStateFlags != 0) {
        if (roundState == 6 && !(playerWork->actorStateFlags & 2)) {
            if (playerWork->isComputerPlayer != 0) {
                InitializePlayerActor(object);
            }
            else {
                playerWork->stickX = HuPadStkX[playerWork->controllerPort];
                playerWork->stickY = HuPadStkY[playerWork->controllerPort];
                playerWork->buttonsPressed = HuPadBtnDown[playerWork->controllerPort];
                playerWork->buttonsHeld = HuPadBtn[playerWork->controllerPort];
            }
        }
        else {
            playerWork->stickX = playerWork->stickY = 0;
            playerWork->buttonsPressed = playerWork->buttonsHeld = 0;
        }
        switch (roundState) {
            case 5:
            case 6:
                break;
            case 7:
            case 9:
                object->scale.y += 0.2f * (1.0f - object->scale.y);
                playerWork->rotationY = InterpolateAngle(playerWork->rotationY, 0.0f, 0.9f);
                break;
            case 10:
                if (playerWork->motionState != 6) {
                    playerWork->motionState = 6;
                }
                break;
        }
        if (playerWork->actorStateFlags & 2) {
            playerWork->stickX = playerWork->stickY = 0;
            playerWork->buttonsPressed = playerWork->buttonsHeld = 0;
            if (object->scale.y < 0.2f) {
                omVibrate(playerWork->playerIndex, 12, 4, 2);
                HuAudFXPlay(0x523);
                playerWork->actorStateFlags = 0;
                Hu3DModelShadowReset(modelId);
                Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
                remainingRoundCount--;
                effectModelId = CreateParticleEffect(DATA_MAKE_NUM(DATADIR_M403, 19), 100, UpdateParticleTransform);
                Hu3DModelPosSet(effectModelId, playerWork->actorPosition.x, playerWork->actorPosition.y, playerWork->actorPosition.z);
                Hu3DModelLayerSet(effectModelId, 2);
            }
        }
        if (getScenePieceCountPublic() != 0) {
            playerWork->isNearbyCollision = 0;
        }
        else if (!(playerWork->actorStateFlags & 2) && playerWork->isNearbyCollision == 0) {
            omVibrate(playerWork->playerIndex, 12, 6, 6);
            playerWork->isNearbyCollision = 1;
        }
        UpdatePlayerActor(object);
    }
}

static void InitializePlayerActor(OMOBJ *object)
{
    Vec targetPositions[6];
    Vec positionSwap;
    Vec targetOffset;
    float targetDistances[6];
    float distance;
    float movementAngle;
    float desiredSpeed;
    float turnRate;
    PlayerActorWork *playerWork;
    s16 stickX;
    s16 stickY;
    s16 pressedButtons;
    s16 heldButtons;
    s16 playerDifficulty;
    s32 weightedChoice;
    s32 isLastPlayer;
    s32 choiceWeight;
    s32 targetCount;
    s32 candidateIndex;
    s32 i;

    isLastPlayer = 0;
    for (i = 0, candidateIndex = 0; i < 4; i++) {
        playerWork = playerObjects[i]->data;
        if (playerWork->isComputerPlayer == 0 && playerWork->actorStateFlags != 0) {
            break;
        }
    }
    playerWork = object->data;
    if (i >= 4 && playerWork->isFinalActivePlayer == 0) {
        isLastPlayer = 1;
    }
    playerDifficulty = playerWork->playerDifficulty;
    stickX = stickY = pressedButtons = heldButtons = 0;
    switch (playerWork->computerState) {
        case 0:
            if (getScenePieceCountPublic() != 0) {
                playerWork->computerState = 1;
            }
            break;
        case 1:
            playerWork->computerState = 2;
            playerWork->computerStateTimer = (s32)(REFRESH_RATE * (1.2f - 0.2f * playerDifficulty) * (GetRandomValue() / 65536.0f));
            playerWork->targetPositionX = 500.0f * ((GetRandomValue() - 0x8000) / 32768.0f);
            playerWork->targetPositionZ = 200.0f * ((GetRandomValue() - 0x8000) / 32768.0f);
            stickX = playerWork->stickX;
            stickY = playerWork->stickY;
            break;
        case 2:
            targetOffset.x = playerWork->targetPositionX - playerWork->actorPosition.x;
            targetOffset.y = 0.0f;
            targetOffset.z = playerWork->targetPositionZ - playerWork->actorPosition.z;
            distance = NormalizeVector(&targetOffset);
            if (distance < 20.0f) {
                playerWork->computerState = 3;
            }
            else {
                if (distance >= 100.0f) {
                    turnRate = 0.8f;
                }
                else {
                    turnRate = 0.4f;
                }
                movementAngle = InterpolateAngle(playerWork->rotationY, atan2d(targetOffset.x, targetOffset.z), turnRate);
                desiredSpeed = 48.0f + 4.0f * playerDifficulty;
                stickX = desiredSpeed * sind(movementAngle);
                stickY = desiredSpeed * -cosd(movementAngle);
                if (getScenePieceCountPublic() == 0) {
                    playerWork->computerState = 0;
                    if (playerWork->computerStateTimer == 0) {
                        playerWork->computerStateTimer = -1;
                    }
                }
                else if (playerWork->computerStateTimer >= 0) {
                    if (playerWork->computerStateTimer == 0) {
                        if (distance < 150.0f && playerWork->nearbyActorCount != 0) {
                            playerWork->computerState = 3;
                        }
                        else if ((s32)((playerDifficulty + 2) * (GetRandomValue() / 65536.0f)) != 0 && isLastPlayer == 0) {
                            playerWork->computerState = 4;
                        }
                        else {
                            playerWork->computerState = 1;
                        }
                    }
                    else {
                        playerWork->computerStateTimer--;
                    }
                }
            }
            break;
        case 3:
            targetOffset.x = playerWork->targetPositionX - playerWork->actorPosition.x;
            targetOffset.y = 0.0f;
            targetOffset.z = playerWork->targetPositionZ - playerWork->actorPosition.z;
            if (NormalizeVector(&targetOffset) >= 30.000002f && playerWork->nearbyActorCount == 0) {
                playerWork->computerState = 2;
            }
            else if (getScenePieceCountPublic() == 0) {
                playerWork->computerState = 0;
            }
            else {
                if (playerWork->computerStateTimer > 0) {
                    playerWork->computerStateTimer--;
                }
                if (playerWork->computerStateTimer == 0) {
                    if ((s32)((playerDifficulty + 2) * (GetRandomValue() / 65536.0f)) != 0 && isLastPlayer == 0) {
                        playerWork->computerState = 4;
                    }
                    else {
                        playerWork->computerState = 1;
                    }
                }
            }
            break;
        case 4:
            targetCount = getPieceCenterPositions(&targetPositions[0]);
            for (i = 0; i < targetCount; i++) {
                VECSubtract(&targetPositions[i], &playerWork->actorPosition, &targetOffset);
                targetDistances[i] = VECMag(&targetOffset);
            }
            for (i = 0; i < targetCount - 1; i++) {
                for (candidateIndex = i + 1; candidateIndex < targetCount; candidateIndex++) {
                    if (targetDistances[i] > targetDistances[candidateIndex]) {
                        positionSwap = targetPositions[i];
                        targetPositions[i] = targetPositions[candidateIndex];
                        targetPositions[candidateIndex] = positionSwap;
                        distance = targetDistances[i];
                        targetDistances[i] = targetDistances[candidateIndex];
                        targetDistances[candidateIndex] = distance;
                    }
                }
            }
            choiceWeight = 1000;
            weightedChoice = choiceWeight * (GetRandomValue() / 65536.0f);
            for (i = 0; i < targetCount - 1; i++) {
                choiceWeight = choiceWeight * (5.0f + playerDifficulty) * 0.1f;
                weightedChoice -= choiceWeight;
                if (weightedChoice < 0) {
                    break;
                }
            }
            playerWork->targetPositionX = targetPositions[i].x + 100.0f * (0.5f - 0.15f * playerDifficulty) * ((GetRandomValue() - 0x8000) / 32768.0f);
            playerWork->targetPositionZ = targetPositions[i].z + 100.0f * (0.5f - 0.15f * playerDifficulty) * ((GetRandomValue() - 0x8000) / 32768.0f);
            playerWork->computerState = 2;
            playerWork->computerStateTimer = -1;
            stickX = playerWork->stickX;
            stickY = playerWork->stickY;
            break;
    }
    playerWork->stickX = stickX;
    playerWork->stickY = stickY;
    playerWork->buttonsPressed = pressedButtons;
    playerWork->buttonsHeld = heldButtons;
}

static void UpdatePlayerActor(OMOBJ *object)
{
    float movementAngle;
    PlayerActorWork *playerWork;
    s32 modelId;
    s16 motionLoopFlags;
    s16 nextMotionState;
    s16 stickX;
    s16 stickY;
    s16 buttonsPressed;
    s16 buttonsHeld;

    playerWork = object->data;
    modelId = object->mdlId[0];
    nextMotionState = playerWork->motionState;
    playerWork->previousPosition = playerWork->actorPosition;
    stickX = playerWork->stickX;
    stickY = playerWork->stickY;
    buttonsPressed = playerWork->buttonsPressed;
    buttonsHeld = playerWork->buttonsHeld;
    switch (playerWork->motionState) {
        case 0:
        case 1:
        case 2:
            playerWork->movementSpeed = 0.14f * sqrtf(stickX * stickX + stickY * stickY);
            if (playerWork->movementSpeed > 0.1f) {
                if (playerWork->movementSpeed >= 6.0f) {
                    nextMotionState = 2;
                    motionLoopFlags = 1;
                }
                else {
                    nextMotionState = 1;
                    motionLoopFlags = 1;
                }
                movementAngle = atan2d(stickX, -stickY);
                playerWork->rotationY = InterpolateAngle(playerWork->rotationY, movementAngle, 0.5f);
                playerWork->movementAngle = playerWork->rotationY;
                playerWork->movementPhase += 1.0f;
                if (playerWork->movementPhase >= 0.5f * playerWork->motionDuration) {
                    playerWork->movementPhase -= 0.5f * playerWork->motionDuration;
                }
                playerWork->movementSpeed *= 1.0 + 0.25 * cosd(90.0f * (playerWork->movementPhase / (0.5f * playerWork->motionDuration)));
            }
            else {
                playerWork->movementPhase = 0.0f;
                playerWork->movementSpeed = 0.0f;
                nextMotionState = 0;
                motionLoopFlags = 1;
            }
            playerWork->verticalVelocity = -146.0f/REFRESH_RATE;
            break;
        case 3:
            motionLoopFlags = 0;
            if (Hu3DData[modelId].motIdShift == -1 && CharMotionEndCheck(playerWork->characterId)) {
                playerWork->verticalVelocity += -146.0f/REFRESH_RATE;
            }
            else {
                playerWork->verticalVelocity = 0.0f;
            }
            if (playerWork->isAirborne != 0) {
                if (playerWork->landingState != 0) {
                    playerWork->verticalVelocity = 0.0f;
                    playerWork->landingState = 0;
                    nextMotionState = 5;
                    motionLoopFlags = 0;
                }
                else {
                    playerWork->verticalVelocity *= -0.1f;
                    playerWork->isAirborne = 0;
                    nextMotionState = 4;
                    motionLoopFlags = 0;
                }
            }
            break;
        case 4:
            playerWork->verticalVelocity += -146.0f/REFRESH_RATE;
            if (playerWork->isAirborne != 0 && CharMotionEndCheck(playerWork->characterId)) {
                nextMotionState = 5;
                motionLoopFlags = 0;
            }
            break;
        case 5:
            if (CharMotionEndCheck(playerWork->characterId)) {
                nextMotionState = 0;
                motionLoopFlags = 1;
            }
            break;
        case 6:
        case 7:
            motionLoopFlags = 0;
            playerWork->verticalVelocity = -146.0f/REFRESH_RATE;
            break;
    }
    if (playerWork->actorStateFlags & 4) {
        if (playerWork->actorActionTimer & 2) {
            Hu3DModelAttrSet(modelId, HU3D_ATTR_DISPOFF);
        }
        else {
            Hu3DModelAttrReset(modelId, HU3D_ATTR_DISPOFF);
        }
        if (--playerWork->actorActionTimer == 0) {
            playerWork->actorStateFlags &= ~4;
        }
    }
    playerWork->horizontalVelocityX = playerWork->movementSpeed * sind(playerWork->movementAngle);
    playerWork->horizontalVelocityZ = playerWork->movementSpeed * cosd(playerWork->movementAngle);
    playerWork->actorPosition.x += playerWork->horizontalVelocityX;
    if (playerWork->isAirborne == 0) {
        playerWork->actorPosition.y += playerWork->verticalVelocity;
    }
    playerWork->actorPosition.z += playerWork->horizontalVelocityZ;
    if (nextMotionState != playerWork->previousMotionState) {
        playerWork->previousMotionState = nextMotionState;
        playerWork->motionState = nextMotionState;
        CharMotionShiftSet(playerWork->characterId, object->mtnId[playerWork->motionState], 0.0f, 8.0f, motionLoopFlags);
        playerWork->motionDuration = CharMotionMaxTimeGet(playerWork->characterId);
    }
    omSetTra(object, playerWork->actorPosition.x, playerWork->actorPosition.y, playerWork->actorPosition.z);
    omSetRot(object, playerWork->rotationX, playerWork->rotationY, playerWork->rotationZ);
}

static void CheckRoundCompletion(void)
{
    s32 separationIterations;
    s32 i;
    PlayerActorWork *playerWork;

    separationIterations = 0;
    for (i = 0; i < 4; i++) {
        playerWork = playerObjects[i]->data;
        if (playerWork->actorStateFlags != 0) {
            playerWork->nearbyActorCount = 0;
        }
    }
    while (CountActivePlayers() != 0 && separationIterations < 50) {
        separationIterations++;
    }
}

static s32 CountActivePlayers(void)
{
    Vec separationOffsets[4];
    Vec playerSeparation;
    float playerDistance;
    float separationAmount;
    s32 overlapCount;
    OMOBJ *playerObject;
    PlayerActorWork *firstPlayerWork;
    PlayerActorWork *secondPlayerWork;
    PlayerActorWork *playerWork;
    s32 i;
    s32 j;

    overlapCount = 0;
    for (i = 0; i < 4; i++) {
        separationOffsets[i].x = separationOffsets[i].y = separationOffsets[i].z = 0.0f;
    }
    for (i = 0; i < 3; i++) {
        firstPlayerWork = playerObjects[i]->data;
        if (firstPlayerWork->actorStateFlags != 0) {
            for (j = i + 1; j < 4; j++) {
                secondPlayerWork = playerObjects[j]->data;
                if (secondPlayerWork->actorStateFlags != 0) {
                    VECSubtract(&firstPlayerWork->actorPosition, &secondPlayerWork->actorPosition, &playerSeparation);
                    playerDistance = VECMag(&playerSeparation);
                    if (playerDistance < 80.0f) {
                        VECNormalize(&playerSeparation, &playerSeparation);
                        separationAmount = 0.0001f + 0.5f * (80.0f - playerDistance);
                        VECScale(&playerSeparation, &playerSeparation, separationAmount);
                        VECAdd(&separationOffsets[i], &playerSeparation, &separationOffsets[i]);
                        VECSubtract(&separationOffsets[j], &playerSeparation, &separationOffsets[j]);
                        firstPlayerWork->nearbyActorCount++;
                        secondPlayerWork->nearbyActorCount++;
                        overlapCount++;
                    }
                }
            }
        }
    }
    for (i = 0; i < 4; i++) {
        playerObject = playerObjects[i];
        playerWork = playerObject->data;
        if (playerWork->actorStateFlags != 0) {
            VECAdd(&playerWork->actorPosition, &separationOffsets[i], &playerWork->actorPosition);
            if (playerWork->actorPosition.x > 660.0f) {
                playerWork->actorPosition.x = 660.0f;
            }
            if (playerWork->actorPosition.x < -660.0f) {
                playerWork->actorPosition.x = -660.0f;
            }
            if (playerWork->actorPosition.z > 360.0f) {
                playerWork->actorPosition.z = 360.0f;
            }
            if (playerWork->actorPosition.z < -360.0f) {
                playerWork->actorPosition.z = -360.0f;
            }
            omSetTra(playerObject, playerWork->actorPosition.x, playerWork->actorPosition.y, playerWork->actorPosition.z);
        }
    }
    return overlapCount;
}

static void UpdatePlayerStates(OMOBJ *object)
{
    s32 activePlayerIndices[4];
    s32 activePlayerCount;
    s32 i;

    if (roundState < 6) {
        return;
    }
    if (getScenePieceCountPublic() != 0) {
        for (i = 0; i < 4; i++) {
            UpdatePlayerBoundaryScale(playerObjects[i]);
        }
    }
    else {
        for (i = 0, activePlayerCount = 0; i < 4; i++) {
            ((PlayerActorWork *)playerObjects[i]->data)->isFinalActivePlayer = 0;
            if (((PlayerActorWork *)playerObjects[i]->data)->actorStateFlags != 0) {
                activePlayerIndices[activePlayerCount++] = i;
            }
        }
        if (activePlayerCount != 0) {
            i = activePlayerIndices[(s32)(activePlayerCount * (GetRandomValue() / 65536.0f))];
            ((PlayerActorWork *)playerObjects[i]->data)->isFinalActivePlayer = 1;
        }
    }
    CheckRoundCompletion();
}

static void FindNearestSpawnPosition(Vec *position, Vec *position2)
{
    Vec candidatePositions[6];
    Vec candidateOffset;
    float nearestDistance;
    float candidateDistance;
    s32 candidateCount;
    s32 i;

    candidateCount = getPieceTransformedPositions(candidatePositions);
    nearestDistance = 2000.0f;
    for (i = 0; i < candidateCount; i++) {
        VECSubtract(&candidatePositions[i], position, &candidateOffset);
        candidateDistance = VECMag(&candidateOffset);
        if (candidateDistance >= nearestDistance) {
            continue;
        }
        nearestDistance = candidateDistance;
        *position2 = candidatePositions[i];
    }
}

static void UpdatePlayerBoundaryScale(OMOBJ *object)
{
    Vec safePosition;
    float distanceToEdge;
    float targetScale;
    PlayerActorWork *playerWork;

    playerWork = object->data;
    if (playerWork->actorStateFlags != 0) {
        distanceToEdge = getDistanceToPieceFromPosition(&playerWork->actorPosition, characterBoundaryInsets[playerWork->characterId]) - 10.0f;
        if (distanceToEdge < 0.0f) {
            distanceToEdge = 0.0f;
        }
        if (!(playerWork->actorStateFlags & 2)) {
            targetScale = 1.0f;
            if (distanceToEdge < characterBoundaryScales[playerWork->characterId]) {
                if (findPieceAtPosition(&playerWork->actorPosition, &safePosition) != 0) {
                    playerWork->actorPosition.x = safePosition.x;
                    playerWork->actorPosition.z = safePosition.z;
                }
                else {
                    if (getDistanceToPiece(playerWork->actorPosition.x, playerWork->actorPosition.z) - 10.0f < 100.0f) {
                        playerWork->actorStateFlags |= 2;
                    }
                    targetScale = distanceToEdge / characterBoundaryScales[playerWork->characterId];
                }
            }
        }
        else {
            targetScale = distanceToEdge / characterBoundaryScales[playerWork->characterId];
        }
        if (object->scale.y < targetScale) {
            targetScale = object->scale.y + 0.2f * (targetScale - object->scale.y);
        }
        object->scale.y = targetScale;
    }
}

static s16 CreateParticleEffect(s32 index, s16 state, HU3DPARTICLEHOOK particleHook)
{
    ANIMDATA *particleAnimation;
    HU3DPARTICLE *particleData;
    s32 particleId;

    particleAnimation = HuSprAnimRead(HuDataReadNum(index, HU_MEMNUM_OVL));
    particleId = Hu3DParticleCreate(particleAnimation, state);
    Hu3DParticleHookSet(particleId, particleHook);
    particleData = Hu3DData[particleId].hookData;
    particleData->unk2E = particleId;
    particleData->dataCnt = particleData->emitCnt = 0;
    return particleId;
}

static void UpdateParticleTransform(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float particleFadeProgress;
    float particleTravelProgress;
    float particleAngleStep;
    HU3DPARTICLEDATA *particleData;
    s16 particleState;
    s32 i;

    particleState = 0;
    switch (particle->dataCnt) {
        case 0:
            particleAngleStep = 360.0f / particle->maxCnt;
            particleData = particle->data;
            for (i = 0; i < particle->maxCnt; i++, particleData++) {
                particleData->vel.x = sind(particleAngleStep * i) * (1.0f + 6.0f * ((GetRandomValue() - 0x8000) / 32768.0f) * 0.1f);
                particleData->vel.y = 0.0f;
                particleData->vel.z = cosd(particleAngleStep * i) * (1.0f + 6.0f * ((GetRandomValue() - 0x8000) / 32768.0f) * 0.1f);
                particleData->pos.x = 0.0f;
                particleData->pos.y = 0.0f;
                particleData->pos.z = 0.0f;
            }
            particle->emitCnt = REFRESH_RATE/2;
            particle->dataCnt++;
            /* fallthrough */
        case 1:
            particle->emitCnt--;
            particleFadeProgress = ((REFRESH_RATE/2.0f) - particle->emitCnt) / (REFRESH_RATE/2.0f);
            particleTravelProgress = particle->emitCnt / (REFRESH_RATE/2.0f);
            particleData = particle->data;
            for (i = 0; i < particle->maxCnt; i++, particleData++) {
                particleData->pos.x += 5.0f * particleTravelProgress * particleData->vel.x;
                particleData->pos.y = 20.0f;
                particleData->pos.z += 5.0f * particleTravelProgress * particleData->vel.z;
                particleData->scale = 20.0f + 60.0f * particleFadeProgress;
                particleData->color.r = particleData->color.g = particleData->color.b = 0xFF;
                particleData->color.a = 64.0f * (1.0f - particleFadeProgress * particleFadeProgress);
            }
            if (particle->emitCnt == 0) {
                particle->dataCnt++;
            }
            break;
        case 2:
            Hu3DModelKill(particle->unk2E);
            return;
    }
    DCFlushRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

static s32 GetRandomValue(void)
{
    randomNumberSeed *= 0x41C64E6D;
    randomNumberSeed += 0x3039;
    return randomNumberSeed >> 16;
}

static float InterpolateAngle(float value, float value2, float value3)
{
    float angleDifference;

    if (value > 180.0f) {
        value -= 360.0f;
    }
    else if (value <= -180.0f) {
        value += 360.0f;
    }
    if (value2 > 180.0f) {
        value2 -= 360.0f;
    }
    else if (value2 <= -180.0f) {
        value2 += 360.0f;
    }
    angleDifference = value - value2;
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    else if (angleDifference <= -180.0f) {
        angleDifference += 360.0f;
    }
    value = value2 + angleDifference * value3;
    if (value > 180.0f) {
        value -= 360.0f;
    }
    else if (value <= -180.0f) {
        value += 360.0f;
    }
    return value;
}

static void BuildRotationMatrix(Mtx transformMatrix, float value, float value2, float value3)
{
    Mtx xRotationMatrix;
    Mtx yRotationMatrix;

    if (value3 != 0.0f) {
        MTXRotRad(transformMatrix, 'Z', MTXDegToRad(value3));
    }
    else {
        MTXIdentity(transformMatrix);
    }
    if (value != 0.0f) {
        MTXRotRad(xRotationMatrix, 'X', MTXDegToRad(value));
        MTXConcat(xRotationMatrix, transformMatrix, transformMatrix);
    }
    if (value2 != 0.0f) {
        MTXRotRad(yRotationMatrix, 'Y', MTXDegToRad(value2));
        MTXConcat(yRotationMatrix, transformMatrix, transformMatrix);
    }
}

static void ExtractRotationAngles(float *values, float *values2)
{
    float sineComponent;
    float cosineComponent;
    double cosineSquared;

    values2[0] = atan2d(values[10], values[6]);
    values2[2] = atan2d(values[0], values[1]);
    sineComponent = -values[2];
    if (1.0 - sineComponent * sineComponent < 0.0) {
        cosineSquared = -(1.0 - sineComponent * sineComponent);
    }
    else {
        cosineSquared = 1.0 - sineComponent * sineComponent;
    }
    cosineComponent = sqrtf(cosineSquared);
    if (values2[0] > 90.0f && values2[0] < 270.0f && values2[2] > 90.0f && values2[2] < 270.0f) {
        values2[0] = fmod(180.0f + values2[0], 360.0);
        values2[2] = fmod(180.0f + values2[2], 360.0);
        cosineComponent = -cosineComponent;
    }
    values2[1] = atan2d(cosineComponent, sineComponent);
}

static float EvaluateBezierCurve(float value, float value2, float value3, float value4)
{
    float remainingProgress;
    float interpolatedValue;

    remainingProgress = 1.0 - value;
    interpolatedValue = remainingProgress * remainingProgress * value2 + remainingProgress * value * value3 * 2.0 + value * value * value4;
    return interpolatedValue;
}

static void EvaluateBezierCurvePoints(float *values, float *values2, float *values3, float *values4, float value)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        *(values4++) = EvaluateBezierCurve(value, *(values++), *(values2++), *(values3++));
    }
}

static float EvaluateBezierDerivative(float value, float value2, float value3, float value4)
{
    float curveDerivative = 2.0 * ((value - 1.0) * value2 + (1.0 - 2.0 * value) * value3 + value * value4);
    return curveDerivative;
}

static void EvaluateBezierDerivatives(float *values, float *values2, float *values3, float *values4, float value)
{
    float curveDerivative[3];
    float derivativeMagnitude;
    s32 i;

    for (i = 0; i < 3; i++) {
        curveDerivative[i] = EvaluateBezierDerivative(value, *(values++), *(values2++), *(values3++));
    }
    derivativeMagnitude = VECMagPoint(curveDerivative[0], curveDerivative[1], curveDerivative[2]);
    if (derivativeMagnitude) {
        derivativeMagnitude = 1.0 / derivativeMagnitude;
        for (i = 0; i < 3; i++) {
            *(values4++) = derivativeMagnitude * curveDerivative[i];
        }
    }
    else {
        *(values4++) = 0.0f;
        *(values4++) = 0.0f;
        *(values4++) = 1.0f;
    }
}

static void CalculateSplineCoefficients(float *values, float *values2, float *values3, s32 index)
{
    float intervalWidths[16];
    float splineSlopes[16];
    double splineRatio;
    s32 i;

    values3[0] = 0.0f;
    values3[index - 1] = 0.0f;
    for (i = 0; i < index - 1; i++) {
        intervalWidths[i] = values[i + 1] - values[i];
        if (intervalWidths[i] == 0.0f) {
            splineSlopes[i + 1] = 0.0f;
        }
        else {
            splineSlopes[i + 1] = (values2[i + 1] - values2[i]) / intervalWidths[i];
        }
    }
    values3[1] = splineSlopes[2] - splineSlopes[1];
    splineSlopes[1] = 2.0f * (values[2] - values[0]);
    for (i = 1; i < index - 2; i++) {
        splineRatio = intervalWidths[i] / splineSlopes[i];
        values3[i + 1] = splineSlopes[i + 2] - splineSlopes[i + 1] - splineRatio * values3[i];
        splineSlopes[i + 1] = 2.0f * (values[i + 2] - values[i]) - splineRatio * intervalWidths[i];
    }
    values3[index - 2] -= intervalWidths[index - 2] * values3[index - 1];
    for (i = index - 2; i > 0; i--) {
        if (splineSlopes[i] == 0.0f) {
            values3[i] = 0.0f;
        }
        else {
            values3[i] = (values3[i] - intervalWidths[i] * values3[i + 1]) / splineSlopes[i];
        }
    }
}

static float EvaluateSpline(float value, float *values, float *values2, float *values3, s32 index)
{
    float intervalWidth;
    float scaledProgress;
    float interpolatedValue;
    s32 segmentIndex;

    segmentIndex = value;
    value -= segmentIndex;
    intervalWidth = values[segmentIndex + 1] - values[segmentIndex];
    scaledProgress = intervalWidth * value;
    interpolatedValue = values2[segmentIndex]
        + scaledProgress
            * (scaledProgress * (3.0f * values3[segmentIndex] + scaledProgress * (values3[segmentIndex + 1] - values3[segmentIndex]) / intervalWidth)
                + ((values2[segmentIndex + 1] - values2[segmentIndex]) / intervalWidth - intervalWidth * (2.0f * values3[segmentIndex] + values3[segmentIndex + 1])));
    if (intervalWidth == 0.0f) {
        interpolatedValue = values2[segmentIndex];
    }
    return interpolatedValue;
}

static void BuildVectorSpline(float *values, Vec *position, float value[][3], s32 index)
{
    Vec segmentDelta;
    float segmentLength;
    float coordinateValues[16];
    float splineCoefficients[16];
    s32 j;
    s32 i;

    values[0] = 0.0f;
    for (i = 1; i < index; i++) {
        VECSubtract(&position[i], &position[i - 1], &segmentDelta);
        values[i] = values[i - 1] + VECMag(&segmentDelta);
    }
    for (j = 0; j < 3; j++) {
        for (i = 0; i < index; i++) {
            coordinateValues[i] = ((float *)&position[i])[j];
        }
        CalculateSplineCoefficients(values, coordinateValues, splineCoefficients, index);
        for (i = 0; i < index; i++) {
            value[i][j] = splineCoefficients[i];
        }
    }
}

static void EvaluateVectorSpline(float value, Vec *position, float *values, Vec *position2, float value2[][3], s32 index)
{
    float controlPoints[16];
    float splineCoefficients[16];
    s32 i;
    s32 j;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < index; j++) {
            controlPoints[j] = ((float *)&position2[j])[i];
            splineCoefficients[j] = value2[j][i];
        }
        ((float *)position)[i] = EvaluateSpline(value, values, controlPoints, splineCoefficients, index);
    }
}

static float NormalizeVector(Vec *position)
{
    float vectorLength;
    float inverseLength;

    vectorLength = position->x * position->x + position->y * position->y + position->z * position->z;
    vectorLength = sqrtf(vectorLength);
    if (vectorLength != 0.0f) {
        inverseLength = 1.0f / vectorLength;
        position->x *= inverseLength;
        position->y *= inverseLength;
        position->z *= inverseLength;
    }
    else {
        position->x = position->y = position->z = 0.0f;
    }
    return vectorLength;
}

static void InterpolateVector(Vec position, Vec position2, Vec *position3, float value)
{
    if (value <= 0.0f) {
        position3->x = position.x;
        position3->y = position.y;
        position3->z = position.z;
        value = 0.0f;
    }
    else if (value >= 1.0f) {
        position3->x = position.x + position2.x;
        position3->y = position.y + position2.y;
        position3->z = position.z + position2.z;
        value = 1.0f;
    }
    else {
        position3->x = position.x + value * position2.x;
        position3->y = position.y + value * position2.y;
        position3->z = position.z + value * position2.z;
    }
}

static float ProjectPointOntoLine(Vec position, Vec position2, Vec position3)
{
    float projectionNumerator;
    float negativeDirectionLength;

    projectionNumerator = position3.x * (position2.x - position.x) + position3.y * (position2.y - position.y) + position3.z * (position2.z - position.z);
    negativeDirectionLength = -(position3.x * position3.x + position3.y * position3.y + position3.z * position3.z);
    if (negativeDirectionLength != 0.0f) {
        projectionNumerator /= negativeDirectionLength;
    }
    return projectionNumerator;
}

static float DistanceToLineSegment(Vec *position, Vec *position2, Vec *position3)
{
    Vec closestPoint;
    float projectionFraction;

    if (position3->x * position3->x + position3->y * position3->y + position3->z * position3->z == 0.0f) {
        return (position->x - position2->x) * (position->x - position2->x) + (position->y - position2->y) * (position->y - position2->y) + (position->z - position2->z) * (position->z - position2->z);
    }
    projectionFraction = ProjectPointOntoLine(*position, *position2, *position3);
    InterpolateVector(*position2, *position3, &closestPoint, projectionFraction);
    return (position->x - closestPoint.x) * (position->x - closestPoint.x) + (position->y - closestPoint.y) * (position->y - closestPoint.y) + (position->z - closestPoint.z) * (position->z - closestPoint.z);
}

static float DistanceToLineSegmentAndDirection(Vec *position, Vec *position2, Vec *position3, Vec *position4, Vec *position5)
{
    float projectionFraction;

    if (position3->x * position3->x + position3->y * position3->y + position3->z * position3->z == 0.0f) {
        return (position->x - position2->x) * (position->x - position2->x) + (position->y - position2->y) * (position->y - position2->y) + (position->z - position2->z) * (position->z - position2->z);
    }
    projectionFraction = ProjectPointOntoLine(*position, *position2, *position3);
    InterpolateVector(*position2, *position3, position4, projectionFraction);
    position5->x = position4->x - position->x;
    position5->y = position4->y - position->y;
    position5->z = position4->z - position->z;
    return NormalizeVector(position5);
}

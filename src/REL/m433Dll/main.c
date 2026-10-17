#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "string.h"

#include "REL/m433Dll.h"

void InitializeMinigameState(OMOBJ *object);
void AdvanceMinigameState(OMOBJ *object);
void UpdateMinigameObject(OMOBJ *object);
void FinishMinigameWipe(OMOBJ *object);
void ReturnToBoard(OMOBJ *object);
void BeginRoundIntro(OMOBJ *object);
void UpdateRoundTimer(OMOBJ *object);
void FinishRoundIntro(OMOBJ *object);
OMOBJFUNC CreateResultsCameraCallback(HUPROCESS *process, OMOBJ *object);
void UpdateResultsCameraAndSequence(OMOBJ *object);
float InterpolateAngle(float currentAngle, float targetAngle, float interpolationFactor);

Vec globalLightPosition = { 100.0f, 800.0f, -100.0f };
Vec globalLightDirection = { 0.3f, -0.8f, 0.3f };
Vec shadowLightPosition = { 10.0f, 45.0f, 3500.0f };
GXColor globalLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };
Vec shadowCameraPosition = { 50.0f, 150000.0f, 50.0f };
Vec worldUpVector = { 0.0f, 1.0f, 0.0f };
Vec shadowLookAtTarget = { 0.0f, 0.0f, 0.0f };
Vec resultsCameraCenter = { 0.0f, 170.0f, 0.0f };
Vec resultsCameraRotationAndZoom = { 0.0f, -22.0f, 850.0f };

char useInitialMapMotion[0x4];
char unusedMinigameFlag2[0x4];
char unusedMinigameFlag[0x4];
HUPROCESS *minigameProcess;
OMOBJ *minigameManagerObject;
OMOBJ *viewManagerObject;
char unusedMinigameState[8];
s16 globalLightId;
s16 introSequence;
s16 outroSequence;
s16 resultsSequence;
s32 introSoundEffect;
s32 outroSoundEffect;
s32 roundMusicSequence;
s32 resultsMusicStream;

void ObjectSetup(void)
{
    Vec shadowTarget;
    Vec shadowPosition;
    HUPROCESS *process;
    HU3DLIGHT *globalLight;

    HuAudSndGrpSet(0x3A);
    introSoundEffect = outroSoundEffect = -1;
    roundMusicSequence = resultsMusicStream = -1;
    shadowCameraPosition.x = 200.0f;
    shadowCameraPosition.y = 1200.0f;
    shadowCameraPosition.z = 160.0f;
    worldUpVector.x = 0.0f;
    worldUpVector.y = 1.0f;
    worldUpVector.z = 0.0f;
    shadowLookAtTarget.x = 0.0f;
    shadowLookAtTarget.y = 0.0f;
    shadowLookAtTarget.z = 0.0f;
    Hu3DShadowCreate(30.0f, 20.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.625f);
    Hu3DShadowPosSet(&shadowCameraPosition, &worldUpVector, &shadowLookAtTarget);
    globalLightId = Hu3DGLightCreateV(&globalLightPosition, &globalLightDirection, &globalLightColor);
    Hu3DGLightInfinitytSet(globalLightId);
    globalLight = &Hu3DGlobalLight[globalLightId];
    globalLight->type |= 0x8000;
    shadowLightPosition.x = 55.74f;
    shadowLightPosition.y = 0.0f;
    shadowLightPosition.z = 3500.0f;
    shadowTarget.x = shadowTarget.y = shadowTarget.z = 0.0f;
    shadowPosition.x = shadowPosition.z = 0.0f;
    shadowPosition.y = shadowLightPosition.z;
    shadowPosition.x = 0.0f;
    shadowPosition.y = 3480.0002f;
    shadowPosition.z = 900.00006f;
    Hu3DGLightPosAimSetV(globalLightId, &shadowPosition, &shadowTarget);
    Hu3DShadowPosSet(&shadowPosition, &worldUpVector, &shadowTarget);
    introSequence = outroSequence = resultsSequence = -1;
    process = omInitObjMan(0x32, 0x2000);
    minigameProcess = process;
    omGameSysInit(process);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, -1.0f, 10.0f, 10000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    viewManagerObject = omAddObjEx(process, 0x7FDA, 0, 0, -1, omOutView);
    minigameManagerObject = omAddObjEx(process, 0xA, 0, 0, -1, InitializeMinigameState);
    InitializeMapObjects(process);
    InitializePlayerObjects(process);
}

void InitializeMinigameState(OMOBJ *object)
{
    M433DllWork *work;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M433DllWork), HU_MEMNUM_OVL);
    work = object->data;
    memset(work, 0, sizeof(M433DllWork));
    work->flightDuration = 0;
    work->elapsedTime = 0;
    work->roundResultState = 0;
    work->introFrameCounter = 0;
    work->teamIndex = 0;
    work->introTimer = 0;
    work->resultsState = 0;
    work->resultsTimer = 0;
    work->exitRequested = 0;
    work->roundResult = 2;
    work->playerResultFlags = 0;
    work->teamResultFlags = 0;
    object->objFunc = UpdateMinigameObject;
}

void UpdateMinigameFrameCounters(OMOBJ *object)
{
    M433DllWork *work = object->data;
    work->elapsedTime++;
    work->introFrameCounter++;
    work->introTimer++;
    work->resultsTimer++;
    if ((omSysExitReq != 0) && (work->flightDuration != 5)) {
        work->flightDuration = 4;
        AdvanceMinigameState(object);
    }
}

void AdvanceMinigameState(OMOBJ *object)
{
    M433DllWork *work = object->data;
    if (work->exitRequested == 0) {
        switch (work->flightDuration) {
            case 0:
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
                Hu3DCameraPerspectiveSet(1, 41.5f, 10.0f, 10000.0f, 1.2f);
                introSoundEffect = HuAudFXPlay(0x6AC);
                work->flightDuration = 1;
                work->introFrameCounter = 0;
                object->objFunc = createMapCameraCallback(minigameProcess, object);
                break;
            case 1:
                work->flightDuration = 2;
                work->introTimer = 0;
                object->objFunc = BeginRoundIntro;
                break;
            case 2:
                work->flightDuration = 3;
                work->resultsTimer = 0;
                object->objFunc = CreateResultsCameraCallback(minigameProcess, object);
                break;
            case 3:
            case 4:
                work->exitRequested = 1;
                work->flightDuration = 5;
                object->objFunc = FinishMinigameWipe;
                break;
            case 5:
            default:
                work->flightDuration = 5;
                object->objFunc = ReturnToBoard;
                break;
        }
        work->elapsedTime = 0;
    }
}

void UpdateMinigameObject(OMOBJ *object)
{
    UpdateMinigameFrameCounters(object);
    AdvanceMinigameState(object);
}

void FinishMinigameWipe(OMOBJ *object)
{
    M433DllWork *work = object->data;

    UpdateMinigameFrameCounters(object);
    if (WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        work->exitRequested = 0;
        AdvanceMinigameState(object);
    }
}

void ReturnToBoard(OMOBJ *object)
{
    UpdateMinigameFrameCounters(object);
    if (WipeStatGet() == 0) {
        CleanupPlayerObjects();
        CleanupMapObjects();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void BeginRoundIntro(OMOBJ *object)
{
    M433DllWork *work = object->data;
    UpdateMinigameFrameCounters(object);
    switch (work->teamIndex) {
        case 0:
            HuAudFXFadeOut(introSoundEffect, REFRESH_RATE/2);
            work->roundsRemaining = 0x1E;
            work->roundTimer = REFRESH_RATE;
            introSequence = MGSeqCreate(3, 0);
            MGSeqPosSet(introSequence, 320.0f, 240.0f);
            Center.x = 0.0f;
            Center.y = 200.0f;
            Center.z = 0.0f;
            CRot.x = -30.0f;
            CRot.y = 0.0f;
            CRot.z = 0.0f;
            CZoom = 1900.0f;
            work->teamIndex = 1;
            work->introTimer = 0;
            break;
        case 1:
            if ((roundMusicSequence < 0) && ((MGSeqStatGet(introSequence) & 0x10) != 0)) {
                roundMusicSequence = HuAudSeqPlay(0x47);
            }
            if ((MGSeqStatGet(introSequence) == 0) && (work->exitRequested == 0)) {
                work->teamIndex = 2;
                work->introTimer = 0;
                object->objFunc = UpdateRoundTimer;
            }
            break;
    }
}

void UpdateRoundTimer(OMOBJ *object)
{

    M433DllWork *work = object->data;
    s32 shouldStartOutro = 0;
    UpdateMinigameFrameCounters(object);
    work->roundTimer++;
    if (--work->roundTimer == 0) {
        work->roundTimer = REFRESH_RATE;
        work->roundsRemaining--;
        if (work->roundsRemaining == 0) {
            shouldStartOutro = 1;
        }
    }
    if (work->roundResult != 2) {
        shouldStartOutro = 1;
    }
    if (shouldStartOutro != 0) {
        outroSequence = MGSeqCreate(3, 1);
        MGSeqPosSet(outroSequence, 320.0f, 240.0f);
        HuAudSeqFadeOut(roundMusicSequence, 0x64);
        work->teamIndex = 3;
        work->introTimer = 0;
        if (work->exitRequested == 0) {
            object->objFunc = FinishRoundIntro;
        }
        outroSoundEffect = HuAudFXPlay(0x6AC);
    }
}

void FinishRoundIntro(OMOBJ *object)
{
    M433DllWork *work = object->data;
    UpdateMinigameFrameCounters(object);
    if ((MGSeqStatGet(outroSequence) == 0) && ((work->playerResultFlags + 0xFFFF0000) == 0x1111)) {
        work->teamIndex = 4;
        work->introTimer = 0;
        AdvanceMinigameState(object);
    }
}

OMOBJFUNC CreateResultsCameraCallback(HUPROCESS *process, OMOBJ *object)
{
    return UpdateResultsCameraAndSequence;
}

static inline s32 UpdateResultsCameraAndSequenceInline(void)
{
    return GWSystem.mgType;
}

void UpdateResultsCameraAndSequence(OMOBJ *object)
{
    M433DllWork *work = object->data;
    float targetCameraAngle = resultsCameraRotationAndZoom.y;
    if (isPlayerWinner(1) != 0) {
        targetCameraAngle *= -1.0f;
    }
    Center.x += 0.15f * (resultsCameraCenter.x - Center.x);
    Center.y += 0.15f * (resultsCameraCenter.y - Center.y);
    Center.z += 0.15f * (resultsCameraCenter.z - Center.z);
    CRot.x = InterpolateAngle(CRot.x, resultsCameraRotationAndZoom.x, 0.15f);
    CRot.y = InterpolateAngle(CRot.y, targetCameraAngle, 0.15f);
    CZoom += 0.15f * (resultsCameraRotationAndZoom.z - CZoom);
    UpdateMinigameFrameCounters(object);
    work->resultsState = 1;
    if (resultsMusicStream < 0) {
        resultsMusicStream = HuAudSStreamPlay(1);
    }
    if (resultsSequence < 0) {
        s32 winningCharacterIds[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
        s32 playerIndex;
        s32 characterCount = 0;
        s32 sequenceVariant;
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            if (work->winningPlayerIndices[playerIndex] >= 0) {
                winningCharacterIds[characterCount++] = GWPlayerCfg[work->winningPlayerIndices[playerIndex]].character;
                GWPlayerCoinWinAdd(work->winningPlayerIndices[playerIndex], 10);
            }
        }
        if (winningCharacterIds[0] == -1) {
            resultsSequence = MGSeqCreate(5, 2);
            return;
        }
        sequenceVariant = 3;
        if (UpdateResultsCameraAndSequenceInline() == 0) {
            if (isPlayerWinner(0) != 0) {
                sequenceVariant = 5;
            }
        }
        resultsSequence = MGSeqCreate(5, sequenceVariant, winningCharacterIds[0], winningCharacterIds[1], winningCharacterIds[2], winningCharacterIds[3]);
        return;
    }
    if ((MGSeqStatGet(resultsSequence) == 0) && (work->resultsTimer >= (REFRESH_RATE*3.5f))) {
        HuAudFXFadeOut(outroSoundEffect, REFRESH_RATE/2);
        work->resultsState = 2;
        AdvanceMinigameState(object);
    }
}

s32 GetMinigameState(void)
{
    return omObjGetDataAs(minigameManagerObject, M433DllWork)->flightDuration;
}

s32 GetMinigameResultState(void)
{
    return omObjGetDataAs(minigameManagerObject, M433DllWork)->roundResultState;
}

u32 GetRoundIntroState(void)
{
    return omObjGetDataAs(minigameManagerObject, M433DllWork)->teamIndex;
}

u32 GetResultsSequenceState(void)
{
    return omObjGetDataAs(minigameManagerObject, M433DllWork)->resultsState;
}

void SetRoundResultAndPlayers(s32 roundResult, s32 firstWinningPlayer, s32 secondWinningPlayer)
{
    M433DllWork *work = minigameManagerObject->data;
    work->roundResult = roundResult;
    work->winningPlayerIndices[0] = firstWinningPlayer;
    work->winningPlayerIndices[1] = secondWinningPlayer;
}

s32 GetRoundResult(void)
{
    return omObjGetDataAs(minigameManagerObject, M433DllWork)->roundResult;
}

void SetPlayerResultFlag(s32 playerIndex)
{
    omObjGetDataAs(minigameManagerObject, M433DllWork)->playerResultFlags |= (1 << ((playerIndex & 7) * 4));
}

void SetTeamResultFlag(s32 teamIndex)
{
    omObjGetDataAs(minigameManagerObject, M433DllWork)->teamResultFlags |= (1 << ((teamIndex & 3) * 4));
}

s32 IsOutroSequenceFinished(void)
{
    return MGSeqStatGet(outroSequence) == 0;
}

void SetCameraFromPositions(Vec *cameraPosition, Vec *cameraTarget)
{
    Vec cameraDirection;

    VECSubtract(cameraTarget, cameraPosition, &cameraDirection);
    Center = *cameraTarget;
    CRot.x = atan2d(cameraDirection.y, sqrtf((cameraDirection.x * cameraDirection.x) + (cameraDirection.z * cameraDirection.z)));
    CRot.y = atan2d(-cameraDirection.x, -cameraDirection.z);
    CRot.z = 0.0f;
    CZoom = VECMag(&cameraDirection);
}

void SetCameraFromCoordinates(float cameraPositionX, float cameraPositionY, float cameraPositionZ, float cameraTargetX, float cameraTargetY, float cameraTargetZ)
{
    Vec cameraPosition;
    Vec cameraTarget;

    cameraPosition.x = cameraPositionX;
    cameraPosition.y = cameraPositionY;
    cameraPosition.z = cameraPositionZ;
    cameraTarget.x = cameraTargetX;
    cameraTarget.y = cameraTargetY;
    cameraTarget.z = cameraTargetZ;
    SetCameraFromPositions(&cameraPosition, &cameraTarget);
}

float InterpolateAngle(float currentAngle, float targetAngle, float interpolationFactor)
{
    float targetCameraAngle;
    float angleDifference;
    float interpolatedAngle;

    angleDifference = fmod(targetAngle - currentAngle, 360.0);
    targetCameraAngle = angleDifference;
    if (targetCameraAngle < 0.0f) {
        targetCameraAngle += 360.0f;
    }
    if (targetCameraAngle > 180.0f) {
        targetCameraAngle -= 360.0f;
    }
    interpolatedAngle = fmod(currentAngle + (interpolationFactor * targetCameraAngle), 360.0);
    targetCameraAngle = interpolatedAngle;
    if (targetCameraAngle < 0.0f) {
        targetCameraAngle += 360.0f;
    }
    return targetCameraAngle;
}

float InterpolateValue(float currentValue, float targetValue)
{
    float targetCameraAngle;
    float angleDifference;

    angleDifference = fmod(targetValue - currentValue, 360.0);
    targetCameraAngle = angleDifference;
    if (targetCameraAngle < 0.0f) {
        targetCameraAngle += 360.0f;
    }
    if (targetCameraAngle > 180.0f) {
        targetCameraAngle -= 360.0f;
    }
    return targetCameraAngle;
}

s32 cameraMotionData[] = {
    0x00000000,
    0x00000000,
    0x45ABE000,
    0xC576E000,
    0x00000000,
    0xBF333333,
    0x3F19999A,
    0xFFFFFFFF,
    0x00000000,
    0x45ABE000,
    0xC576E000,
    0x00000000,
    0x3F800000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
};

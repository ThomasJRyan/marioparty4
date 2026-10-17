#include "ext_math.h"
#include "game/audio.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"

#include "REL/m417Dll.h"
#include "version.h"

typedef struct MinigameSequenceState {
    /* 0x00 */ char reservedSequenceData[0x10];
    /* 0x10 */ u32 sequenceFrame;
    /* 0x14 */ char reservedSequenceState[0x10];
    /* 0x24 */ s32 sequenceState;
    /* 0x28 */ u32 sequenceTimer;
    /* 0x2C */ s32 playerSequenceIds[4];
    /* 0x3C */ s32 playerSequenceResults[4];
    /* 0x3C */ s32 coinWinSequenceIds[4];
    /* 0x5C */ u32 sequenceMode;
} MinigameSequenceState; /* size = unknown */

void UpdateIntroCameraSequence(OMOBJ *object);
void CreateIntroCharacter(OMOBJ *object);
void UpdateIntroCharacter(OMOBJ *object);
void UpdateIntroCharacterFall(OMOBJ *object);
void UpdateResultsSequence(OMOBJ *object);

extern OMOBJ *introCharacterObject;
extern s32 resultsSequenceId;

OMOBJFUNC StartIntroCameraSequence(HUPROCESS *objectManager)
{
    CRot.x = -80.5f;
    CRot.y = CRot.z = 0.0f;
    Center.x = 0.0f;
    Center.y = 0.0f;
    Center.z = 0.0f;
    CZoom = 1000.0f;
    introCharacterObject = omAddObjEx(objectManager, 30, 1, 0, -1, CreateIntroCharacter);
    return UpdateIntroCameraSequence;
}

void UpdateIntroCameraSequence(OMOBJ *object)
{
    Vec cameraPathPosition;

    Float pathProgress;

    MinigameSequenceState *sequenceState;
    s32 transitionDuration;

    sequenceState = object->data;
    {
        Vec cameraCenterPath[2] = { { -400.0f, 0.0f, -200.0f }, { 0.0f, 150.0f, 0.0f } };
        Vec cameraRotationPath[2] = { { -50.0f, 30.0f, 1500.0f }, { -50.0f, 0.0f, 2700.0f } };

        transitionDuration = REFRESH_RATE;
        UpdateMinigameFrame(object);
        if (sequenceState->sequenceFrame < transitionDuration) {
            pathProgress = 0.0f;
        } else {
            pathProgress = (0.33333333f) / REFRESH_RATE_F * (sequenceState->sequenceFrame - transitionDuration);
        }
        VECSubtract(&cameraCenterPath[1], &cameraCenterPath[0], &cameraPathPosition);
        VECScale(&cameraPathPosition, &cameraPathPosition, pathProgress);
        VECAdd(&cameraCenterPath[0], &cameraPathPosition, &cameraPathPosition);
        Center.x = cameraPathPosition.x;
        Center.y = cameraPathPosition.y;
        Center.z = cameraPathPosition.z;
        VECSubtract(&cameraRotationPath[1], &cameraRotationPath[0], &cameraPathPosition);
        VECScale(&cameraPathPosition, &cameraPathPosition, pathProgress);
        VECAdd(&cameraRotationPath[0], &cameraPathPosition, &cameraPathPosition);
        CRot.x = cameraPathPosition.x;
        CRot.y = cameraPathPosition.y;
        CRot.z = 0.0f;
        CZoom = cameraPathPosition.z;
        if (pathProgress >= 1.0f) {
            AdvanceMinigameState(object);
        }
    }
}

void CreateIntroCharacter(OMOBJ *object)
{
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M417, 5));
    object->trans.x = -300.0f;
    object->trans.z = -200.0f;
    object->trans.y = 0.0f;
    object->rot.y = 20.0f;
    object->work[0] = object->work[1] = 0;
    object->objFunc = UpdateIntroCharacter;
}

void UpdateIntroCharacter(OMOBJ *object)
{
    Float rotationAngle;
    Float bounceFactor;

    object->work[0] += 1;
    switch (object->work[1]) {
        case 0:
            if (object->work[0] > 0.0f) {
                SetWaterWave(-object->trans.x, object->trans.y, -object->trans.z, 1.0f);
                object->work[0] = 0;
                object->work[1] += 1;
            }
            break;
        case 1:
            if (object->work[0] == REFRESH_RATE_F) {
                omVibrate(0, 12, 6, 6);
                omVibrate(1, 12, 6, 6);
                omVibrate(2, 12, 6, 6);
                omVibrate(3, 12, 6, 6);
            }
            if (object->work[0] > 1.6f * REFRESH_RATE_F) {
                object->work[0] = 0;
                object->work[1] += 1;
            }
            break;
        case 2:
            rotationAngle = (VERSION_PAL ? 3.6f : 3.0f) * object->work[0];
            object->rot.z = rotationAngle;
            bounceFactor = 0.5 + (0.5 * -cosd(rotationAngle));
            object->trans.x = -300.0f - (200.0f * bounceFactor);
            object->trans.z = -200.0f - (50.0f * bounceFactor);
            rotationAngle = (VERSION_PAL ? 7.2f : 6.0f) * object->work[0];
            if (rotationAngle >= 180.0f) {
                bounceFactor = 0.5 + (0.5 * cosd(rotationAngle));
                object->trans.y += -5.0f * bounceFactor;
            }
            if (object->work[0] > REFRESH_RATE_F) {
                object->work[0] = 0;
                object->work[1] += 1;
            }
            break;
        default:
            object->work[2] = 0;
            object->objFunc = UpdateIntroCharacterFall;
            break;
    }
    object->trans.y += 0.05f * (getWaterHeightAtPosition(object->trans.x, 0.0f, object->trans.z) - object->trans.y);
}

void UpdateIntroCharacterFall(OMOBJ *object)
{
    Float fallAngle;

    object->work[0] = (object->work[0] + 1) % (REFRESH_RATE * 2);
    fallAngle = (VERSION_PAL ? 7.2f : 6.0f) * object->work[0];
    object->rot.z = 180.0 + (10.0 * sind(fallAngle));
    fallAngle = (VERSION_PAL ? 3.6f : 3.0f) * object->work[0];
    object->rot.x = 5.0 * sind(fallAngle);
    if (object->work[2] < REFRESH_RATE_F * 2) {
        object->work[2] += 1;
    }
    fallAngle = 100.0f * (-2.0f * (REFRESH_FREQ / 2 * object->work[2]));
    object->trans.y += 0.1f * ((fallAngle + getWaterHeightAtPosition(object->trans.x, 0.0f, object->trans.z)) - object->trans.y);
}

OMOBJFUNC StartResultsSequence(HUPROCESS *objectManager, OMOBJ *minigameObject)
{
    MinigameSequenceState *sequenceState;
    s32 playerIndex;
    s32 playerWon;
    s16 coinSequenceIndex;
    s16 playerSequenceIndex;

    sequenceState = minigameObject->data;
    playerWon = 1;
    if (sequenceState->sequenceMode == 2) {
        playerWon = 0;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        sequenceState->coinWinSequenceIds[playerIndex] = -1;
        if (!sequenceState->playerSequenceIds[playerIndex] == playerWon) {
            sequenceState->coinWinSequenceIds[playerIndex] = sequenceState->playerSequenceResults[playerIndex];
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (sequenceState->coinWinSequenceIds[playerIndex] >= 0) {
            GWPlayerCoinWinAdd(playerIndex, 10);
        }
    }
    HuAudSStreamPlay(1);
    sequenceState->sequenceState = 1;
    resultsSequenceId = -1;
    return UpdateResultsSequence;
}

void UpdateResultsSequence(OMOBJ *object)
{
    MinigameSequenceState *sequenceState;

    sequenceState = object->data;
    UpdateMinigameFrame(object);
    if (resultsSequenceId < 0) {
        resultsSequenceId = MGSeqCreate(5, 3, sequenceState->coinWinSequenceIds[0], sequenceState->coinWinSequenceIds[1], sequenceState->coinWinSequenceIds[2], sequenceState->coinWinSequenceIds[3]);
        return;
    }
    if (!MGSeqStatGet(resultsSequenceId) && sequenceState->sequenceTimer >= REFRESH_RATE_F * 3.5f) {
        sequenceState->sequenceState = 2;
        AdvanceMinigameState(object);
    }
}

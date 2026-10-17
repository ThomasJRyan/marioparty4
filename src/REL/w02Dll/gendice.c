#include "REL/w02Dll.h"
#ifndef __MWERKS__
#include "game/audio.h"
#include "game/hsfex.h"
#include "game/board/pause.h"
#endif
#include "ext_math.h"

typedef struct GenDiceWork {
    union {
        /* 0x00 */ struct {
            u8 cancelRequested : 1;
            u8 soundStarted : 1;
            u8 animationState : 3;
            u8 dieIndex : 2;
            u8 reservedFlag : 1;
        };
        u8 controlFlags;
    };
    /* 0x01 */ u8 stateTimer;
    /* 0x02 */ u8 bounceCount;
    /* 0x03 */ u8 reserved03;
    /* 0x04 */ s16 rotationAngle;
    /* 0x06 */ s16 rotationFrame;
} GenDiceWork;

/* EXTERN FUNCTIONS */
void GenDiceRollProcess(void);
void GenDiceSpawnDie(W02GenDice *diceWork);
void GenDiceWaitForDieAnimation(W02GenDice *diceWork);
void GenDiceDieUpdate(OMOBJ *object);
void GenDiceDieDestroy(W02GenDice *diceWork, s32 dieIndex);
void GenDiceDieFinish(W02GenDice *diceWork);
void GenDiceNumberCreate(W02GenDice *diceWork, s32 dieIndex);
void GenDiceDieResultSet(W02GenDice *diceWork);
s32 GenDiceNumbersAtTargetCheck(W02GenDice *diceWork);
void GenDiceNumberDisplay(W02GenDice *diceWork, s32 rollResult);
void GenDiceNumberDestroy(W02GenDice *diceWork, s32 dieIndex);
void GenDiceNumbersDestroy(W02GenDice *diceWork);

/* LOCAL FUNCTIONS */
void GenDiceCreate(W02GenDice *diceWork)
{
    Vec screenPosition;
    s32 i;

    OSReport("W02GenDiceCreate\n");
    diceWork->rollProcess = HuPrcChildCreate(GenDiceRollProcess, 0x2005, 0x2000, 0, boardMainProc);
    diceWork->rollProcess->user_data = diceWork;
    diceWork->totalRollResult = 0;
    diceWork->dieAnimationComplete = 0;
    diceWork->rollComplete = 0;
    Hu3D3Dto2D(&diceWork->spawnPosition, 1, &screenPosition);

    /* Initialize Matrix */
    diceWork->resultScreenPositions[0] = diceWork->resultScreenPositions[1] = diceWork->singleDieResultPosition = screenPosition;
    diceWork->singleDieResultPosition.y = diceWork->resultScreenPositions[1].y = diceWork->resultScreenPositions[0].y = 155;
    diceWork->singleDieResultPosition.z = diceWork->resultScreenPositions[1].z = diceWork->resultScreenPositions[0].z = 1400;
    diceWork->activeDieIndex = -1;

    for (i = 0; i < 3; i++) {
        diceWork->dieObjects[i] = 0;
        diceWork->resultNumberObjects[i] = 0;
        diceWork->dieModelIds[i] = -1;
        diceWork->dieParticleIds[i] = -1;
        diceWork->dieSoundIds[i] = -1;
    }
}

void GenDiceRollProcess(void)
{
    s32 totalRoll;
    s32 i;
    s32 dieIndex;
    W02GenDice *dice;

    dice = HuPrcCurrentGet()->user_data;
    dice->dieFaceResults[0] = dice->dieFaceResults[1] = dice->dieFaceResults[2] = 0;
    totalRoll = 0;
    dice->activeDieIndex = 0;
    while (dice->activeDieIndex < dice->dieCount) {
        GenDiceSpawnDie(dice);
        GenDiceWaitForDieAnimation(dice);
        while (dice->dieAnimationComplete == 0) {
            HuPrcVSleep();
        }
        GenDiceDieResultSet(dice);
        while (dice->dieAnimationComplete != 0) {
            HuPrcVSleep();
        }
        dice->activeDieIndex += 1;
    }

    for (i = 0; i < dice->dieCount; i++) {
        totalRoll += dice->dieFaceResults[i];
    }
    OSReport("Dice res: %d\n", totalRoll);

    while (GenDiceNumbersAtTargetCheck(dice) == 0) {
        HuPrcVSleep();
    }
    dice->totalRollResult = (s16)totalRoll;
    dice->rollComplete = 1;
    if (dice->displayRollTotal != 0) {
        for (i = 0; i < dice->dieCount; i++) {
            GenDiceNumberDestroy(dice, i);
        }
        dice->rollProcess = NULL;
        HuPrcEnd();
    }
    while (TRUE) {
        HuPrcVSleep();
    }
}

void GenDiceDestroy(W02GenDice *diceWork)
{
    OSReport("W02GenDiceKill\n");
    if (diceWork->rollProcess) {
        HuPrcKill(diceWork->rollProcess);
    }
    GenDiceDieFinish(diceWork);
    GenDiceNumbersDestroy(diceWork);
}

void GenDiceSpawnDie(W02GenDice *diceWork)
{
    Vec diePosition;
    f32 motionFrame;
    OMOBJ *dieObject;
    s32 dieIndex;
    GenDiceWork *dieWork;

    dieIndex = diceWork->activeDieIndex;
    diceWork->dieModelIds[dieIndex] = BoardModelCreate(diceWork->dieModelData, NULL, 0);
    diceWork->dieMotionStartFrame = 0.5f + BoardRandMod(diceWork->maximumRoll);
    BoardModelMotionStart(diceWork->dieModelIds[dieIndex], 0, 0x40000001U);
    BoardModelMotionSpeedSet(diceWork->dieModelIds[dieIndex], 0);
    BoardModelMotionTimeSet(diceWork->dieModelIds[dieIndex], diceWork->dieMotionStartFrame);
    dieObject = omAddObjEx(boardObjMan, 0x102, 0U, 0U, -1, GenDiceDieUpdate);
    dieWork = (GenDiceWork *)dieObject->work;

    dieWork->cancelRequested = 0;
    dieWork->soundStarted = 0;
    dieWork->animationState = 0;
    dieWork->dieIndex = dieIndex;

    dieWork->rotationAngle = 0;
    dieWork->stateTimer = 3;
    dieWork->bounceCount = 0;
    dieObject->mode = (u32)diceWork;
    diceWork->dieObjects[dieIndex] = dieObject;
    diePosition = diceWork->spawnPosition;
    diePosition.y += (250.0f * diceWork->dieVerticalSpacing);
    dieObject->trans.x = diePosition.x;
    dieObject->trans.y = diePosition.y;
    dieObject->trans.z = diePosition.z;
    dieObject->scale.x = 0;
    dieObject->scale.y = 0;
    dieObject->scale.z = 0;
    diePosition.z += 70.0f;
    HuAudFXPlay(0x302);
}

void GenDiceWaitForDieAnimation(W02GenDice *diceWork)
{
    GenDiceWork *dieWork;

    dieWork = omObjGetWork(diceWork->dieObjects[diceWork->activeDieIndex], GenDiceWork);
    while (dieWork->animationState != 1) {
        HuPrcVSleep();
    }
}

s32 GenDiceAnimationCheck(W02GenDice *diceWork)
{
    GenDiceWork *dieWork;
    if (diceWork->activeDieIndex < 0) {
        return 0;
    }
    dieWork = omObjGetWork(diceWork->dieObjects[diceWork->activeDieIndex], GenDiceWork);
    if (dieWork->animationState != 1) {
        return 0;
    }
    return 1;
}

void GenDiceDieUpdate(OMOBJ *object)
{
    GenDiceWork *dieWork;
    W02GenDice *dice;
    s16 randomFrame;
    s16 previousMotionFrame;
    float angleRadians;
    float scaleFactor;
    float scaleOffset;
    dice = (W02GenDice *)object->mode;
    dieWork = omObjGetWork(object, GenDiceWork);
    if (dieWork->cancelRequested || BoardIsKill()) {
        GenDiceDieDestroy(dice, dieWork->dieIndex);
        dice->dieObjects[dieWork->dieIndex] = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    if (BoardPauseActiveCheck()) {
        return;
    }
    switch (dieWork->animationState) {
        case 0:
            if (dieWork->rotationFrame >= 1080) {
                dieWork->animationState = 1;
                object->scale.x = object->scale.y = object->scale.z = 1;

                object->rot.y = dieWork->rotationAngle = 0;
            }
            else {
                if (!dieWork->rotationAngle && dice->dieSoundIds[dieWork->dieIndex] == -1) {
                    dice->dieSoundIds[dieWork->dieIndex] = HuAudFXPlay(768);
                }
                object->rot.y = dieWork->rotationFrame % 360;
                dieWork->rotationFrame += 25;
                if (dieWork->rotationAngle < 180) {
                    scaleOffset = 0;
                    scaleFactor = 1;
                }
                else {
                    scaleOffset = 1;
                    scaleFactor = 0.5f;
                }
                object->scale.x = (scaleFactor * sind((float)(dieWork->rotationAngle % 180))) + scaleOffset;
                object->scale.y = object->scale.x;
                object->scale.z = object->scale.x;
                if (dieWork->rotationAngle <= 360) {
                    if (dieWork->rotationAngle < 180) {
                        dieWork->rotationAngle += 6;
                    }
                    else {
                        dieWork->rotationAngle += 18;
                    }
                    if (dieWork->rotationAngle > 360) {
                        dieWork->rotationAngle = 360;
                    }
                }
            }
            break;

        case 1:
            dieWork->rotationAngle += 4;
            if (dieWork->rotationAngle > 360) {
                dieWork->rotationAngle -= 360;
            }
            OSs16tof32(&dieWork->rotationAngle, &angleRadians);
            object->trans.y += sind(angleRadians) * 0.2f;
            break;

        case 2:
            dieWork->rotationAngle++;
            if (dieWork->rotationAngle >= 8) {
                if (dice->dieSoundIds[dieWork->dieIndex] != -1) {
                    HuAudFXStop(dice->dieSoundIds[dieWork->dieIndex]);
                    dice->dieSoundIds[dieWork->dieIndex] = -1;
                }
                HuAudFXPlay(771);
                HuAudFXPlay(780);
                dieWork->animationState = 3;
                dieWork->rotationAngle = 0;
                GenDiceNumberDisplay(dice, dice->dieFaceResults[dieWork->dieIndex]);
                GenDiceNumberCreate(dice, dieWork->dieIndex);
                Hu3DParManPosSet(dice->dieParticleIds[dieWork->dieIndex], object->trans.x, object->trans.y, object->trans.z);
                BoardModelVisibilitySet(dice->dieModelIds[dieWork->dieIndex], 0);
            }
            else {
                OSs16tof32(&dieWork->rotationAngle, &angleRadians);
                if (dieWork->rotationAngle < 4) {
                    object->trans.y += (0.5f * angleRadians) + 10.0f;
                    object->scale.x += (0.1f * sind(4.0f * angleRadians));
                    object->scale.y = object->scale.x;
                    object->scale.z = object->scale.x;
                }
                else {
                    BoardModelAlphaSet(dice->dieModelIds[dieWork->dieIndex], 255 - (dieWork->rotationAngle * 32));
                }
                object->trans.y += -0.49f * angleRadians * angleRadians;
            }
            break;

        case 3:
            if (++dieWork->rotationAngle >= 150) {
                dieWork->cancelRequested = 1;
            }
            break;
    }
    if (!dieWork->soundStarted || dieWork->animationState != 3) {
        if (dieWork->bounceCount++ > dieWork->stateTimer) {
            dieWork->bounceCount = 0;
            previousMotionFrame = BoardModelMotionTimeGet(dice->dieModelIds[dieWork->dieIndex]);
            do {
                randomFrame = BoardRandMod(dice->maximumRoll);
            } while (randomFrame == previousMotionFrame);
            BoardModelMotionTimeSet(dice->dieModelIds[dieWork->dieIndex], randomFrame + 0.5f);
        }
    }
    BoardModelPosSet(dice->dieModelIds[dieWork->dieIndex], object->trans.x, object->trans.y, object->trans.z);
    BoardModelRotSet(dice->dieModelIds[dieWork->dieIndex], object->rot.x, object->rot.y, object->rot.z);
    BoardModelScaleSet(dice->dieModelIds[dieWork->dieIndex], object->scale.x, object->scale.y, object->scale.z);
}

void GenDiceDieDestroy(W02GenDice *diceWork, s32 dieIndex)
{
    if (diceWork->dieModelIds[dieIndex] != -1) {
        BoardModelKill(diceWork->dieModelIds[dieIndex]);
        diceWork->dieModelIds[dieIndex] = -1;
    }

    if (diceWork->dieParticleIds[dieIndex] != -1) {
        Hu3DParManKill(diceWork->dieParticleIds[dieIndex]);
        diceWork->dieParticleIds[dieIndex] = -1;
    }
}

void GenDiceDieFinish(W02GenDice *diceWork)
{
    GenDiceDieDestroy(diceWork, 0);
    GenDiceDieDestroy(diceWork, 1);
    GenDiceDieDestroy(diceWork, 2);
}

HU3DPARMANPARAM genDiceParticleParams = { 150, { 0x00, 0x00 }, // padding?
    50, 0, 65.0f, { 0.0f, -0.195f, 0.0f }, 100.0f, 0.99f, 40.0f, 0.999f, 0x0004,
    { 0xFF, 0xFF, 0x20, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x00 } };

void GenDiceNumberCreate(W02GenDice *diceWork, s32 dieIndex)
{
    s16 particleModelId;
    ANIMDATA *particleAnimation;
    void *particleData;
    particleData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 0x01), HU_MEMNUM_OVL, HEAP_MODEL);
    particleAnimation = HuSprAnimRead(particleData);
    diceWork->dieParticleIds[dieIndex] = Hu3DParManCreate(particleAnimation, 100, &genDiceParticleParams);
    Hu3DParManAttrSet(diceWork->dieParticleIds[dieIndex], 0x108);
    Hu3DParManRotSet(diceWork->dieParticleIds[dieIndex], 0, 0, 0);
    Hu3DParManVecSet(diceWork->dieParticleIds[dieIndex], 0, 0.4f, 0.2f);
    particleModelId = Hu3DParManModelIDGet(diceWork->dieParticleIds[dieIndex]);
    Hu3DModelLayerSet(particleModelId, 2);
    Hu3DParticleBlendModeSet(particleModelId, 0);
}

void GenDiceDieResultSet(W02GenDice *diceWork)
{
    s32 dieIndex;
    GenDiceWork *dieWork;
    dieIndex = diceWork->activeDieIndex;
    dieWork = omObjGetWork(diceWork->dieObjects[dieIndex], GenDiceWork);
    dieWork->animationState = 2;
    dieWork->rotationAngle = 0;
    BoardModelMotionSpeedSet(diceWork->dieModelIds[dieIndex], 0);

    diceWork->dieFaceResults[dieIndex] = diceWork->dieResults[dieIndex];
}

typedef struct GenDiceNumberWork {
    u8 cancelRequested : 1;
    u8 settled : 1;
    u8 moveComplete : 1;
    u8 dieIndex : 2;
    u8 reservedFlags : 3;
    s8 stateTimer;
    s16 bounceCount;
    s16 digitModelIds[2];
    float movementStep;
} GenDiceNumberWork;

void GenDiceNumberStop(W02GenDice *diceWork)
{
    s32 dieIndex;
    GenDiceNumberWork *numberWork;
    for (dieIndex = 0; dieIndex < diceWork->dieCount; dieIndex++) {
        if (diceWork->resultNumberObjects[dieIndex]) {
            numberWork = omObjGetWork(diceWork->resultNumberObjects[dieIndex], GenDiceNumberWork);
            numberWork->settled = 1;
            numberWork->moveComplete = 1;
            numberWork->bounceCount = 0;
        }
    }
}

s32 GenDiceNumbersAtTargetCheck(W02GenDice *diceWork)
{
    s32 dieIndex;
    GenDiceNumberWork *numberWork;
    for (dieIndex = 0; dieIndex < diceWork->dieCount; dieIndex++) {
        if (!diceWork->resultNumberObjects[dieIndex]) {
            continue;
        }
        numberWork = omObjGetWork(diceWork->resultNumberObjects[dieIndex], GenDiceNumberWork);
        if (numberWork->moveComplete) {
            return 0;
        }
    }
    return 1;
}

void GenDiceNumberDestroy(W02GenDice *diceWork, s32 dieIndex)
{
    GenDiceNumberWork *numberWork;
    if (!diceWork->resultNumberObjects[dieIndex]) {
        return;
    }
    numberWork = omObjGetWork(diceWork->resultNumberObjects[dieIndex], GenDiceNumberWork);
    numberWork->cancelRequested = 1;
}

void GenDiceNumberUpdate(OMOBJ *object);

void GenDiceNumberDisplay(W02GenDice *diceWork, s32 rollResult)
{
    Vec numberPosition;
    s16 digits[2];
    GenDiceNumberWork *numberWork;
    s16 digitIndex;
    OMOBJ *numberObject;
    s32 dieIndex;
    s16 digitModelId;
    dieIndex = diceWork->activeDieIndex;
    if (rollResult > 99) {
        rollResult = 99;
    }
    digits[0] = rollResult % 10;
    digits[1] = rollResult / 10;
    numberPosition = diceWork->spawnPosition;
    numberPosition.y += 300.0f;
    numberObject = omAddObjEx(boardObjMan, 258, 0, 0, -1, GenDiceNumberUpdate);
    numberObject->mode = (u32)diceWork;
    numberWork = omObjGetWork(numberObject, GenDiceNumberWork);
    numberWork->cancelRequested = 0;
    numberWork->settled = 0;
    numberWork->moveComplete = 0;
    numberWork->dieIndex = dieIndex;
    numberWork->movementStep = 0.3f;
    numberWork->stateTimer = rollResult;
    numberWork->bounceCount = 360;
    for (digitIndex = 0; digitIndex < 2; digitIndex++) {
        digitModelId = BoardModelCreate(diceWork->dieFaceData[digits[digitIndex]], NULL, 0);
        BoardModelPosSetV(digitModelId, &numberPosition);
        numberWork->digitModelIds[digitIndex] = digitModelId;
    }
    if (digits[1] == 0) {
        BoardModelVisibilitySet(numberWork->digitModelIds[1], 0);
    }
    diceWork->resultNumberObjects[dieIndex] = numberObject;
}

void GenDiceNumberModelsDestroy(GenDiceNumberWork *numberWork);
void GenDiceNumberMove(W02GenDice *diceWork, GenDiceNumberWork *numberWork, s32 displayFinalTotal);
void GenDiceNumberRotate(W02GenDice *diceWork, GenDiceNumberWork *numberWork, s32 displayFinalTotal);

void GenDiceNumberUpdate(OMOBJ *object)
{
    GenDiceNumberWork *numberWork;
    W02GenDice *dice;
    dice = (W02GenDice *)object->mode;
    numberWork = omObjGetWork(object, GenDiceNumberWork);
    if (numberWork->cancelRequested || BoardIsKill()) {
        GenDiceNumberModelsDestroy(numberWork);
        dice->resultNumberObjects[numberWork->dieIndex] = NULL;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }
    if (BoardPauseActiveCheck()) {
        return;
    }
    GenDiceNumberMove(dice, numberWork, numberWork->settled);
    GenDiceNumberRotate(dice, numberWork, numberWork->settled);
}

void GenDiceNumberModelsDestroy(GenDiceNumberWork *numberWork)
{
    s32 i;
    for (i = 0; i < 2; i++) {
        if (numberWork->digitModelIds[i] == -1) {
            continue;
        }
        OSReport("%d:GenDiceNumExitOM Kill %d\n", i, numberWork->digitModelIds[i]);
        BoardModelKill(numberWork->digitModelIds[i]);
        numberWork->digitModelIds[i] = -1;
        (void)i;
    }
}

void GenDiceNumbersDestroy(W02GenDice *diceWork)
{
    s32 i;
    GenDiceNumberWork *numberWork;
    s32 j;
    for (i = 0; i < 3; i++) {
        if (!diceWork->resultNumberObjects[i]) {
            continue;
        }
        numberWork = omObjGetWork(diceWork->resultNumberObjects[i], GenDiceNumberWork);
        for (j = 0; j < 2; j++) {
            if (numberWork->digitModelIds[j] == -1) {
                continue;
            }
            OSReport("%d:GenDiceNumOMKillAll %d\n", j, numberWork->digitModelIds[j]);
            BoardModelKill(numberWork->digitModelIds[j]);
            numberWork->digitModelIds[j] = -1;
            (void)j;
        }
        GenDiceNumberDestroy(diceWork, i);
    }
}

void GenDiceNumberMove(W02GenDice *diceWork, GenDiceNumberWork *numberWork, s32 displayFinalTotal)
{
    float depthOffset;
    s16 digitIndex;
    Vec *targetPosition;
    s16 rotationFrame;
    s32 isShifting = 0;
    Vec modelPosition;
    Vec positionDelta;
    Vec numberDisplayCenter = { 288, 155, 1400 };
    Vec digitPositions[2];
    Vec singleDigitPositions[2] = { { 144, 155, 1400 }, { 432, 155, 1400 } };
    Vec doubleDigitPositions[3] = { { 144, 155, 1400 }, { 432, 155, 1400 }, { 288, 155, 1400 } };
    Vec centerPosition = { 288, 176, 1400 };
    rotationFrame = numberWork->stateTimer / 10;
    if (displayFinalTotal) {
        targetPosition = &centerPosition;
        depthOffset = -20.0f * numberWork->dieIndex;
    }
    else {
        switch (diceWork->dieCount) {
            case 1:
                targetPosition = &diceWork->singleDieResultPosition;
                break;

            case 2:
            case 3:
                targetPosition = &diceWork->resultScreenPositions[numberWork->dieIndex];
                break;
        }
        depthOffset = 0;
    }
    for (digitIndex = 0; digitIndex < 2; digitIndex++) {
        if (rotationFrame == 0) {
            digitPositions[digitIndex].x = targetPosition->x;
        }
        else {
            if (digitIndex) {
                digitPositions[digitIndex].x = targetPosition->x - 30;
            }
            else {
                digitPositions[digitIndex].x = targetPosition->x + 30;
            }
        }
        digitPositions[digitIndex].y = targetPosition->y;
        digitPositions[digitIndex].z = targetPosition->z + depthOffset;
    }
    for (digitIndex = 0; digitIndex < 2; digitIndex++) {
        BoardModelPosGet(numberWork->digitModelIds[digitIndex], &modelPosition);
        Hu3D2Dto3D(&digitPositions[digitIndex], 1, &digitPositions[digitIndex]);
        VECSubtract(&digitPositions[digitIndex], &modelPosition, &positionDelta);
        if (VECMag(&positionDelta) < 0.01f) {
            numberWork->moveComplete = 0;
            if (displayFinalTotal && numberWork->dieIndex) {
                BoardModelVisibilitySet(numberWork->digitModelIds[digitIndex], 0);
            }
        }
        else {
            numberWork->moveComplete = 1;
        }
        VECScale(&positionDelta, &positionDelta, numberWork->movementStep);
        VECAdd(&modelPosition, &positionDelta, &modelPosition);
        BoardModelPosSetV(numberWork->digitModelIds[digitIndex], &modelPosition);
    }
}

void GenDiceNumberRotate(W02GenDice *diceWork, GenDiceNumberWork *numberWork, s32 displayFinalTotal)
{
    s32 digitIndex;
    float rotationAngle;
    if (displayFinalTotal) {
        numberWork->bounceCount += 18;
        if (numberWork->bounceCount >= 360) {
            numberWork->bounceCount = 360;
        }
    }
    else {
        numberWork->bounceCount -= 18;
        if (numberWork->bounceCount <= 0) {
            numberWork->bounceCount = 0;
        }
    }
    OSs16tof32(&numberWork->bounceCount, &rotationAngle);
    for (digitIndex = 0; digitIndex < 2; digitIndex++) {
        if (numberWork->digitModelIds[digitIndex] == -1) {
            continue;
        }
        BoardModelRotSet(numberWork->digitModelIds[digitIndex], 0, rotationAngle, 0);
    }
}

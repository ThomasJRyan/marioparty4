#include "game/esprite.h"
#include "game/object.h"
#include "game/process.h"
#include "game/sprite.h"

#include "REL/m407dll.h"

typedef struct ScoreDisplayWork {
    /* 0x00 */ s32 scoreDisplayMode;
    /* 0x04 */ s32 displayState;
    /* 0x08 */ s16 coinScore;
    /* 0x0A */ s16 starScore;
    /* 0x0C */ s16 digitSpriteIds[8]; // what size is this??
    /* 0x1C */ s32 scoreTimer;
    /* 0x20 */ f32 scoreOpacity;
} ScoreDisplayWork; // sizeof 0x24

// func signatures
void ScoreDisplayStateHidden(void);
void ScoreDisplayStateShow(void);
void ScoreDisplayStateUpdate(void);
void UpdateScoreDisplay(OMOBJ *object);
void SetScoreDisplayState(s32 displayState);
s16 GetWhompScoreDisplayMode(void);

// bss
HUPROCESS *scoreDisplayProcess;
OMOBJ *scoreDisplayObject;

// data
VoidFuncs scoreDisplayCallbacks[] = { ScoreDisplayStateHidden, ScoreDisplayStateShow, ScoreDisplayStateUpdate };

void InitializeScoreDisplay(HUPROCESS *process, s16 initialScore)
{
    ScoreDisplayWork *workData;
    u32 i;

    scoreDisplayProcess = process;
    scoreDisplayObject = omAddObjEx(scoreDisplayProcess, 0x800, 0, 0, 7, UpdateScoreDisplay);
    scoreDisplayObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(ScoreDisplayWork), HU_MEMNUM_OVL);
    workData = scoreDisplayObject->data;
    workData->coinScore = initialScore;
    workData->starScore = 0;
    workData->digitSpriteIds[6] = espEntry(DATA_MAKE_NUM(DATADIR_M407, 0x25), 2, 0);
    espTPLvlSet(workData->digitSpriteIds[6], 0.5f);
    espPosSet(workData->digitSpriteIds[6], 508.0f, 64.0f);
    workData->digitSpriteIds[7] = espEntry(DATA_MAKE_NUM(DATADIR_M407, 0x22), 1, 0);
    espPosSet(workData->digitSpriteIds[7], 480.0f, 56.0f);

    for (i = 0; i < 3; i++) {
        workData->digitSpriteIds[i] = espEntry(DATA_MAKE_NUM(DATADIR_M407, 0x1F), 0, 0);
        espPosSet(workData->digitSpriteIds[i], i * 16 + 0x1F4, 56.0f);
        espDispOff(workData->digitSpriteIds[i]);
    }

    for (i = 0; i < 3; i++) {
        workData->digitSpriteIds[i + 3] = espEntry(DATA_MAKE_NUM(DATADIR_M407, 0x20), 1, 0);
        espPosSet(workData->digitSpriteIds[i + 3], i * 16 + 0x1F4, 74.0f);
    }

    for (i = 0; i < 8; i++) {
        espDispOff(workData->digitSpriteIds[i]);
    }

    SetScoreDisplayState(0);
}

void ResetScoreDisplay(void)
{
    s32 i;
    ScoreDisplayWork *scoreWork;

    scoreWork = scoreDisplayObject->data;
    for (i = 0; i < 7; i++) {
        espKill(scoreWork->digitSpriteIds[i]);
    }

    HuMemDirectFree(scoreDisplayObject->data);
}

void SetScoreDisplayMode(s32 displayMode)
{
    switch (displayMode) {
        case 0:
            SetScoreDisplayState(1);
            break;
        case 1:
            SetScoreDisplayState(2);
            break;
    }
}

void HideScoreDisplay(void)
{
    SetScoreDisplayState(0);
}

void SetScoreDisplayState(s32 displayState)
{
    ScoreDisplayWork *workData;

    workData = scoreDisplayObject->data;
    workData->scoreDisplayMode = displayState;
    workData->displayState = 0;
}

void UpdateScoreDisplay(OMOBJ *object)
{
    s16 scoreDigits;
    ScoreDisplayWork *scoreWork;

    scoreWork = scoreDisplayObject->data;
    scoreDisplayCallbacks[scoreWork->scoreDisplayMode]();
    scoreDigits = scoreWork->coinScore;
    espBankSet(scoreWork->digitSpriteIds[0], scoreDigits / 100);
    scoreDigits = (scoreDigits - ((scoreDigits / 100) * 100));
    espBankSet(scoreWork->digitSpriteIds[1], (scoreDigits / 10));
    scoreDigits = (scoreDigits - ((scoreDigits / 10) * 10));
    espBankSet(scoreWork->digitSpriteIds[2], scoreDigits);
    scoreDigits = scoreWork->starScore;
    espBankSet(scoreWork->digitSpriteIds[3], (scoreDigits / 100));
    scoreDigits = (scoreDigits - ((scoreDigits / 100) * 100));
    espBankSet(scoreWork->digitSpriteIds[4], (scoreDigits / 10));
    scoreDigits = (scoreDigits - ((scoreDigits / 10) * 10));
    espBankSet(scoreWork->digitSpriteIds[5], scoreDigits);
}

void ScoreDisplayStateHidden(void)
{
    s32 i;
    ScoreDisplayWork *workData;

    workData = scoreDisplayObject->data;

    if (workData->displayState == 0) {
        for (i = 0; i < ARRAY_COUNT(workData->digitSpriteIds); i++) {
            espDispOff(workData->digitSpriteIds[i]);
        }
        workData->displayState++;
    }
}

void ScoreDisplayStateShow(void)
{
    s16 i;
    ScoreDisplayWork *workData;

    workData = scoreDisplayObject->data;

    if (workData->displayState == 0) {
        for (i = 0; i < ARRAY_COUNT(workData->digitSpriteIds); i++) {
            espDispOn(workData->digitSpriteIds[i]);
        }
        workData->displayState++;
    }

    workData->starScore = GetWhompScoreDisplayMode();
}

void ScoreDisplayStateUpdate(void)
{
    s32 i;
    ScoreDisplayWork *workData;

    workData = scoreDisplayObject->data;

    if (workData->displayState == 0) {
        workData->scoreTimer = 0;
        workData->scoreOpacity = 1.0f;
        if (workData->starScore > workData->coinScore) {
            workData->coinScore = workData->starScore;
        }
        workData->displayState++;
    }

    if (workData->scoreTimer == 0) {
        if ((workData->scoreOpacity += 0.02f) >= 1.2f) {
            workData->scoreOpacity = 1.2f;
            workData->scoreTimer = 1;
        }
    }
    else {
        if ((workData->scoreOpacity -= 0.04f) <= 1.0f) {
            workData->scoreOpacity = 1.0f;
            workData->scoreTimer = 0;
        }
    }

    for (i = 0; i <= 2; i++) {
        espScaleSet(workData->digitSpriteIds[i], workData->scoreOpacity, workData->scoreOpacity);
    }

    espScaleSet(workData->digitSpriteIds[7], workData->scoreOpacity, workData->scoreOpacity);
}

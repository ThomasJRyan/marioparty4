#include "datadir_enum.h"
#include "game/data.h"

#include "REL/m460Dll.h"
#include "game/memory.h"

#include "game/esprite.h"
#ifndef __MWERKS__
#endif

typedef struct M460ScoreDisplayWork {
    s32 displayState;
    s32 stateTimer;
    s16 recordScore;
    s16 currentScore;
    s16 digitSprites[6];
    s32 isPulsing;
    float pulseScale;
} M460ScoreDisplayWork; /* size = 0x20 */

typedef void (*M460ScoreDisplayCallback)(void);

void M460SetScoreDisplayMode(s32 displayMode);
void M460UpdateScoreDisplay(OMOBJ *object);
void M460HideScoreDisplay(void);
void M460ShowScoreDisplay(void);
void M460UpdateRecordScorePulse(void);

M460ScoreDisplayCallback scoreDisplayCallbacks[3] = { M460HideScoreDisplay, M460ShowScoreDisplay, M460UpdateRecordScorePulse };

HUPROCESS *scoreProcess;
OMOBJ *scoreDisplayObject;
// ? lbl_1_rodata_3D0;

void M460CreateScoreDisplay(HUPROCESS *process, s16 recordScore)
{
    M460ScoreDisplayWork *scoreWork;
    u32 digitIndex;

    scoreProcess = process;
    scoreDisplayObject = omAddObjEx(scoreProcess, 0x800, 0, 0, 7, M460UpdateScoreDisplay);
    scoreDisplayObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M460ScoreDisplayWork), HU_MEMNUM_OVL);
    scoreWork = scoreDisplayObject->data;
    scoreWork->recordScore = recordScore;
    scoreWork->currentScore = 0;
    scoreWork->digitSprites[4] = espEntry(0x590024, 2, 0);
    espColorSet(scoreWork->digitSprites[4], 0, 0, 0);
    espTPLvlSet(scoreWork->digitSprites[4], 0.5f);
    espPosSet(scoreWork->digitSprites[4], 508.0f, 64.0f);
    scoreWork->digitSprites[5] = espEntry(0x610037, 1, 0);
    espPosSet(scoreWork->digitSprites[5], 490.0f, 56.0f);
    for (digitIndex = 0; digitIndex < 2; digitIndex++) {
        scoreWork->digitSprites[digitIndex] = espEntry(0x610030, 0, 0);
        espPosSet(scoreWork->digitSprites[digitIndex], (digitIndex * 0x10) + 0x1FE, 56.0f);
        espDispOff(scoreWork->digitSprites[digitIndex]);
    }
    for (digitIndex = 0; digitIndex < 2; digitIndex++) {
        scoreWork->digitSprites[digitIndex + 2] = espEntry(0x610031, 1, 0);
        espPosSet(scoreWork->digitSprites[digitIndex + 2], (digitIndex * 0x10) + 0x1FE, 74.0f);
    }
    for (digitIndex = 0; digitIndex < 6; digitIndex++) {
        espDispOff(scoreWork->digitSprites[digitIndex]);
    }
    HuDataDirClose(DATA_MAKE_NUM(DATADIR_MGCONST, 0));
    M460SetScoreDisplayMode(0);
}

void M460DestroyScoreDisplay(void)
{
    s32 digitIndex;
    M460ScoreDisplayWork *scoreWork = scoreDisplayObject->data;
    for (digitIndex = 0; digitIndex < 6; digitIndex++) {
        espKill(scoreWork->digitSprites[digitIndex]);
    }
    HuMemDirectFree(scoreDisplayObject->data);
}

void M460SetScoreDisplayState(s32 displayState)
{
    s32 unusedScoreTransition;
    s32 unusedScoreState;

    switch (displayState) {
        case 0:
            M460SetScoreDisplayMode(1);
            break;
        case 1:
            M460SetScoreDisplayMode(2);
            break;
    }
}

void M460ResetScoreDisplay(void)
{
    M460SetScoreDisplayMode(0);
}

void M460SetScoreDisplayMode(s32 displayMode)
{
    M460ScoreDisplayWork *scoreWork;

    scoreWork = scoreDisplayObject->data;
    scoreWork->displayState = displayMode;
    scoreWork->stateTimer = 0;
}

void M460UpdateScoreDisplay(OMOBJ *object)
{
    M460ScoreDisplayWork *scoreWork;
    s16 displayedDigit;

    scoreWork = scoreDisplayObject->data;
    scoreDisplayCallbacks[scoreWork->displayState]();
    displayedDigit = scoreWork->recordScore;
    espBankSet(scoreWork->digitSprites[0], displayedDigit / 10);
    displayedDigit -= (displayedDigit / 10) * 10;
    espBankSet(scoreWork->digitSprites[1], displayedDigit);
    displayedDigit = scoreWork->currentScore;
    espBankSet(scoreWork->digitSprites[2], displayedDigit / 10);
    displayedDigit -= (displayedDigit / 10) * 10;
    espBankSet(scoreWork->digitSprites[3], displayedDigit);
}

void M460HideScoreDisplay(void)
{
    M460ScoreDisplayWork *scoreWork;
    s32 digitIndex;

    scoreWork = scoreDisplayObject->data;
    if (scoreWork->stateTimer == 0) {
        for (digitIndex = 0; digitIndex < 6; digitIndex++) {
            espDispOff(scoreWork->digitSprites[digitIndex]);
        }
        scoreWork->stateTimer++;
    }
}

void M460ShowScoreDisplay(void)
{
    M460ScoreDisplayWork *scoreWork;
    s16 digitIndex;

    scoreWork = scoreDisplayObject->data;
    if (scoreWork->stateTimer == 0) {
        for (digitIndex = 0; digitIndex < 6; digitIndex++) {
            espDispOn(scoreWork->digitSprites[digitIndex]);
        }
        scoreWork->stateTimer++;
    }
    scoreWork->currentScore = M460GetCurrentFrameCount();
}

void M460UpdateRecordScorePulse(void)
{
    M460ScoreDisplayWork *scoreWork;
    s32 digitIndex;

    scoreWork = scoreDisplayObject->data;
    if (scoreWork->stateTimer == 0) {
        scoreWork->isPulsing = 0;
        scoreWork->pulseScale = 1.0f;
        if (scoreWork->currentScore > scoreWork->recordScore) {
            scoreWork->recordScore = scoreWork->currentScore;
        }
        scoreWork->stateTimer++;
    }
    if (scoreWork->isPulsing == 0) {
        if ((scoreWork->pulseScale += 0.02f) >= 1.2f) {
            scoreWork->pulseScale = 1.2f;
            scoreWork->isPulsing = 1;
        }
    }
    else {
        if ((scoreWork->pulseScale -= 0.04f) <= 1.0f) {
            scoreWork->pulseScale = 1.0f;
            scoreWork->isPulsing = 0;
        }
    }
    for (digitIndex = 0; digitIndex <= 1; digitIndex++) {
        espScaleSet(scoreWork->digitSprites[digitIndex], scoreWork->pulseScale, scoreWork->pulseScale);
    }
    espScaleSet(scoreWork->digitSprites[5], scoreWork->pulseScale, scoreWork->pulseScale);
}

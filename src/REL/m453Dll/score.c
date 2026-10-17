#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/esprite.h"
#include "game/flag.h"
#include "game/frand.h"
#include "game/gamework.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "REL/m453Dll.h"

typedef struct M453ScoreWork {
    s32 displayMode;
    s32 modeTimer;
    s16 bestScore;
    s16 currentScore;
    s16 digitSprites[4];
    s16 scoreLabelSprite;
    s16 medalSprite;
    s32 pulsePhase;
    Float pulseScale;
} M453ScoreWork; /* size = 0x20 */

typedef void (*M453ScoreModeFunc)(void);

void ScoreDisplayModeSet(s32 displayMode);
void ScoreDisplayObjectUpdate(OMOBJ *object);
void ScoreDigitsHide(void);
void ScoreDigitsShow(void);
void ScorePulseUpdate(void);

M453ScoreModeFunc scoreDisplayModes[3] = { ScoreDigitsHide, ScoreDigitsShow, ScorePulseUpdate };

HUPROCESS *scoreProcess;
OMOBJ *scoreObject;

void ScoreDisplayInitialize(HUPROCESS *process, s16 initialBestScore)
{
    M453ScoreWork *scoreWork;
    u32 digitIndex;

    scoreProcess = process;
    scoreObject = omAddObjEx(scoreProcess, 0x800, 0, 0, 7, ScoreDisplayObjectUpdate);
    scoreObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M453ScoreWork), HU_MEMNUM_OVL);
    scoreWork = scoreObject->data;
    scoreWork->bestScore = initialBestScore;
    scoreWork->currentScore = 0;
    scoreWork->scoreLabelSprite = espEntry(0x530000, 2, 0);
    espTPLvlSet(scoreWork->scoreLabelSprite, 0.5f);
    espPosSet(scoreWork->scoreLabelSprite, 520.0f, 64.0f);
    espColorSet(scoreWork->scoreLabelSprite, 0, 0, 0);
    scoreWork->medalSprite = espEntry(0x610037, 1, 0);
    espPosSet(scoreWork->medalSprite, 501.0f, 55.0f);
    for (digitIndex = 0; digitIndex < 2; digitIndex++) {
        scoreWork->digitSprites[digitIndex] = espEntry(0x610030, 0, 0);
        espPosSet(scoreWork->digitSprites[digitIndex], (digitIndex * 0x12) + 0x20A, 56.0f);
        espDispOff(scoreWork->digitSprites[digitIndex]);
    }
    for (digitIndex = 0; digitIndex < 2; digitIndex++) {
        scoreWork->digitSprites[digitIndex + 2] = espEntry(0x610031, 1, 0);
        espPosSet(scoreWork->digitSprites[digitIndex + 2], (digitIndex * 0x12) + 0x20A, 72.0f);
    }
    for (digitIndex = 0; digitIndex < 6; digitIndex++) {
        espDispOff(scoreWork->digitSprites[digitIndex]);
    }
    HuDataDirClose(0x530000);
    HuDataDirClose(0x610000);
    ScoreDisplayModeSet(0);
}

void ScoreDisplayDestroy(void)
{
    s32 digitIndex;
    M453ScoreWork *scoreWork = scoreObject->data;

    for (digitIndex = 0; digitIndex < 7; digitIndex++) {
        espKill(scoreWork->digitSprites[digitIndex]);
    }
    HuMemDirectFree(scoreObject->data);
}

void ScoreDisplaySetMode(s32 displayMode)
{
    switch (displayMode) {
        case 0:
            ScoreDisplayModeSet(1);
            break;
        case 1:
            ScoreDisplayModeSet(2);
            break;
    }
}

void ScoreDisplayUpdate(void)
{
    ScoreDisplayModeSet(0);
}

void ScoreDisplayModeSet(s32 displayMode)
{
    M453ScoreWork *scoreWork = scoreObject->data;

    scoreWork->displayMode = displayMode;
    scoreWork->modeTimer = 0;
}

void ScoreDisplayObjectUpdate(OMOBJ *object)
{
    s16 scoreValue;

    M453ScoreWork *scoreWork = scoreObject->data;

    scoreDisplayModes[scoreWork->displayMode]();
    scoreValue = scoreWork->bestScore;
    espBankSet(scoreWork->digitSprites[0], scoreValue / 10);
    scoreValue -= (scoreValue / 10) * 10;
    espBankSet(scoreWork->digitSprites[1], scoreValue);
    scoreValue = scoreWork->currentScore;
    espBankSet(scoreWork->digitSprites[2], scoreValue / 10);
    scoreValue -= (scoreValue / 10) * 10;
    espBankSet(scoreWork->digitSprites[3], scoreValue);
}

void ScoreDigitsHide(void)
{
    s32 spriteIndex;

    M453ScoreWork *scoreWork = scoreObject->data;
    if (scoreWork->modeTimer == 0) {
        for (spriteIndex = 0; spriteIndex < 6; spriteIndex++) {
            espDispOff(scoreWork->digitSprites[spriteIndex]);
        }
        scoreWork->modeTimer = scoreWork->modeTimer + 1;
    }
}

void ScoreDigitsShow(void)
{
    s16 spriteIndex;
    M453ScoreWork *scoreWork = scoreObject->data;

    if (scoreWork->modeTimer == 0) {
        for (spriteIndex = 0; spriteIndex < 6; spriteIndex++) {
            espDispOn(scoreWork->digitSprites[spriteIndex]);
        }
        scoreWork->modeTimer = scoreWork->modeTimer + 1;
    }
    scoreWork->currentScore = PlayerScoreGet();
}

void ScorePulseUpdate(void)
{
    s32 digitIndex;
    M453ScoreWork *scoreWork = scoreObject->data;

    if (scoreWork->modeTimer == 0) {
        scoreWork->pulsePhase = 0;
        scoreWork->pulseScale = 1.0f;
        if (scoreWork->currentScore > scoreWork->bestScore) {
            scoreWork->bestScore = scoreWork->currentScore;
        }
        scoreWork->modeTimer = scoreWork->modeTimer + 1;
    }
    if (scoreWork->pulsePhase == 0) {
        if ((scoreWork->pulseScale += 0.02f) >= 1.2f) {
            scoreWork->pulseScale = 1.2f;
            scoreWork->pulsePhase = 1;
        }
    }
    else {
        if ((scoreWork->pulseScale -= 0.04f) <= 1.0f) {
            scoreWork->pulseScale = 1.0f;
            scoreWork->pulsePhase = 0;
        }
    }
    for (digitIndex = 0; digitIndex <= 1; digitIndex++) {
        espScaleSet(scoreWork->digitSprites[digitIndex], scoreWork->pulseScale, scoreWork->pulseScale);
    }
    espScaleSet(scoreWork->medalSprite, scoreWork->pulseScale, scoreWork->pulseScale);
}

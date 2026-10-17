#include "game/data.h"
#include "game/esprite.h"

#include "REL/m442Dll.h"

s32 SetScoreDisplayState(s32 displayMode);
void UpdateScoreDisplay(OMOBJ *object);
void HideScoreDigits(void);
void ShowScoreDigits(void);
void AnimateHighScore(void);

typedef void (*M442ScoreUpdateFunc)(void);

M442ScoreUpdateFunc scoreDisplayModeCallbacks[3] = { HideScoreDigits, ShowScoreDigits, AnimateHighScore };

HUPROCESS *scoreProcess;
OMOBJ *scoreDisplayObject;

typedef struct M442ScoreDisplayWork {
    /* 0x00 */ s32 displayMode;
    /* 0x04 */ s32 modeInitialized;
    /* 0x08 */ s16 bestScore;
    /* 0x0A */ s16 currentScore;
    /* 0x0C */ s16 scoreDigitSprites[6];
    /* 0x18 */ s16 scoreLabelSprite;
    /* 0x1A */ s16 scoreIconSprite;
    /* 0x1C */ s32 pulseGrowing;
    /* 0x20 */ Float pulseScale;
} M442ScoreDisplayWork; /* size = 0x24 */

s32 InitializeScoreDisplay(HUPROCESS *process, s16 bestScore)
{
    M442ScoreDisplayWork *scoreWork;
    u32 spriteIndex;

    scoreProcess = process;
    scoreDisplayObject = omAddObjEx(scoreProcess, 0x800, 0, 0, 7, UpdateScoreDisplay);
    scoreDisplayObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M442ScoreDisplayWork), HU_MEMNUM_OVL);
    scoreWork = scoreDisplayObject->data;
    scoreWork->bestScore = bestScore;
    scoreWork->currentScore = 0;
    scoreWork->scoreLabelSprite = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x23), 2, 0);
    espTPLvlSet(scoreWork->scoreLabelSprite, 0.5f);
    espPosSet(scoreWork->scoreLabelSprite, 508.0f, 64.0f);
    espColorSet(scoreWork->scoreLabelSprite, 0, 0, 0);
    scoreWork->scoreIconSprite = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x37), 1, 0);
    espPosSet(scoreWork->scoreIconSprite, 480.0f, 56.0f);
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        scoreWork->scoreDigitSprites[spriteIndex] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x30), 0, 0);
        espPosSet(scoreWork->scoreDigitSprites[spriteIndex], (spriteIndex * 0x10) + 0x1F4, 56.0f);
        espDispOff(scoreWork->scoreDigitSprites[spriteIndex]);
    }
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        scoreWork->scoreDigitSprites[spriteIndex + 3] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x31), 1, 0);
        espPosSet(scoreWork->scoreDigitSprites[spriteIndex + 3], (spriteIndex * 0x10) + 0x1F4, 74.0f);
    }
    for (spriteIndex = 0; spriteIndex < 8; spriteIndex++) {
        espDispOff(scoreWork->scoreDigitSprites[spriteIndex]);
    }
    HuDataDirClose(DATADIR_MGCONST);
    return SetScoreDisplayState(0);
}

void DestroyScoreDisplay(void)
{
    s32 spriteIndex;
    M442ScoreDisplayWork *scoreWork;

    scoreWork = scoreDisplayObject->data;
    for (spriteIndex = 0; spriteIndex < 8; spriteIndex++) {
        espKill(scoreWork->scoreDigitSprites[spriteIndex]);
    }
    HuMemDirectFree(scoreDisplayObject->data);
}

void SetScoreDisplayMode(s32 mode)
{
    switch (mode) {
        case 0:
            SetScoreDisplayState(1);
            break;
        case 1:
            SetScoreDisplayState(2);
            break;
        default:
            break;
    }
}

void HideScoreDisplay(void)
{
    SetScoreDisplayState(0);
}

s32 SetScoreDisplayState(s32 displayMode)
{
    M442ScoreDisplayWork *scoreWork;

    scoreWork = scoreDisplayObject->data;
    scoreWork->displayMode = displayMode;
    scoreWork->modeInitialized = 0;
}

void UpdateScoreDisplay(OMOBJ *object)
{
    s16 displayedScore;
    M442ScoreDisplayWork* scoreWork;

    scoreWork = scoreDisplayObject->data;
    scoreDisplayModeCallbacks[scoreWork->displayMode]();
    displayedScore = scoreWork->bestScore;
    espBankSet(scoreWork->scoreDigitSprites[0], displayedScore / 100);
    displayedScore = (displayedScore - ((displayedScore / 100) * 100));
    espBankSet(scoreWork->scoreDigitSprites[1], (displayedScore / 10));
    displayedScore =  (displayedScore - ((displayedScore / 10) * 10));
    espBankSet(scoreWork->scoreDigitSprites[2], displayedScore);
    displayedScore = scoreWork->currentScore;
    espBankSet(scoreWork->scoreDigitSprites[3], (displayedScore / 100));
    displayedScore = (displayedScore - ((displayedScore / 100) * 100));
    espBankSet(scoreWork->scoreDigitSprites[4],  (displayedScore / 10));
    displayedScore = (displayedScore - ((displayedScore / 10) * 10));
    espBankSet(scoreWork->scoreDigitSprites[5], displayedScore);
}

void HideScoreDigits(void)
{
    M442ScoreDisplayWork *scoreWork;
    s32 spriteIndex;

    scoreWork = scoreDisplayObject->data;
    if (scoreWork->modeInitialized == 0) {
        for (spriteIndex = 0; spriteIndex < 8; spriteIndex++) {
            espDispOff(scoreWork->scoreDigitSprites[spriteIndex]);
        }
        scoreWork->modeInitialized++;
    }
}

void ShowScoreDigits(void)
{
    s16 spriteIndex;
    M442ScoreDisplayWork *scoreWork;

    scoreWork = scoreDisplayObject->data;
    if (scoreWork->modeInitialized == 0) {
        for (spriteIndex = 0; spriteIndex < 8; spriteIndex++) {
            espDispOn(scoreWork->scoreDigitSprites[spriteIndex]);
        }
        scoreWork->modeInitialized++;
    }
    scoreWork->currentScore = getFinalScore();
}

void AnimateHighScore(void)
{
    M442ScoreDisplayWork *scoreWork;
    s32 spriteIndex;

    scoreWork = scoreDisplayObject->data;
    if (scoreWork->modeInitialized == 0) {
        scoreWork->pulseGrowing = 0;
        scoreWork->pulseScale = 1.0f;
        if (scoreWork->currentScore > scoreWork->bestScore) {
            scoreWork->bestScore = scoreWork->currentScore;
        }
        scoreWork->modeInitialized++;
    }
    if (scoreWork->pulseGrowing == 0) {
        if ((scoreWork->pulseScale += 0.02f) >= 1.2f) {
            scoreWork->pulseScale = 1.2f;
            scoreWork->pulseGrowing = 1;
        }
    }
    else {
        if ((scoreWork->pulseScale -= 0.04f) <= 1.0f) {
            scoreWork->pulseScale = 1.0f;
            scoreWork->pulseGrowing = 0;
        }
    }
    for (spriteIndex = 0; spriteIndex <= 2; spriteIndex++) {
        espScaleSet(scoreWork->scoreDigitSprites[spriteIndex], scoreWork->pulseScale, scoreWork->pulseScale);
    }
    espScaleSet(scoreWork->scoreIconSprite, scoreWork->pulseScale, scoreWork->pulseScale);
}

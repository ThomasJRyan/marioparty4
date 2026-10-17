#ifndef REL_M430DLL_H
#define REL_M430DLL_H

#include "game/object.h"

typedef struct M433DllWork {
    u32 minigameState;
    s32 frameCounter;
    s32 exitRequested;
    s32 roundResultState;
    s32 introFrameCounter;
    s32 introState;
    s32 introTimer;
    u32 roundsRemaining;
    u32 roundTimer;
    s32 resultsState;
    u32 resultsTimer;
    s32 winningPlayerIndices[2];
    u32 roundResult;
    s32 playerResultFlags;
    s32 teamResultFlags;
} M433DllWork; /* size = 0x40 */

void UpdateMinigameFrameCounters(OMOBJ *object);
void AdvanceMinigameState(OMOBJ *object);
u32 GetRoundIntroState(void);
u32 GetResultsSequenceState(void);
void SetRoundResultAndPlayers(s32 roundResult, s32 firstWinningPlayer, s32 secondWinningPlayer);
void SetPlayerResultFlag(s32 playerIndex);
void SetTeamResultFlag(s32 teamIndex);
s32 IsOutroSequenceFinished(void);
float InterpolateAngle(float currentAngle, float targetAngle, float interpolationFactor);

void InitializeMapObjects(HUPROCESS *process);
void CleanupMapObjects(void);
OMOBJFUNC CreateMapCameraCallback(HUPROCESS *process, OMOBJ *object);
void SetMapMotion(s32 transitionFrame);

void InitializePlayerObjects(HUPROCESS *process);
void CleanupPlayerObjects(void);
s32 IsPlayerWinner(s32 teamIndex);

#endif

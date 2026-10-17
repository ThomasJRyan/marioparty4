#ifndef REL_M442DLL_H
#define REL_M442DLL_H

#include "game/object.h"
#include "game/process.h"

s16 GetFinalScore(void);
s32 InitializeScoreDisplay(HUPROCESS *process, s16 bestScore);
void SetScoreDisplayMode(s32 mode);

#endif

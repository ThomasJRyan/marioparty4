#ifndef _GAME_ENVELOPE_EXEC_H
#define _GAME_ENVELOPE_EXEC_H

#include "game/hsfformat.h"

void InitEnvelope(HSFDATA *model_hsf);
void EnvelopeProc(HSFDATA *model_hsf);
void InitVtxParm(HSFDATA *model_hsf);

extern Vec *Vertextop;

#endif

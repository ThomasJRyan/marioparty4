#ifndef REL_M403DLL_H
#define REL_M403DLL_H

#include "game/process.h"

#include "dolphin.h"

void CreatePuzzleScene(HUPROCESS *objectManager);
void DestroyPuzzleScene(void);
void SetSceneSpeed(float speed);
void SetScenePieceCount(s32 pieceCount);
void ResetSceneState(void);
float GetDistanceToPiece(float gridX, float worldZ);
float GetDistanceToPieceFromPosition(Vec *position, float referenceZ);
s32 GetScenePieceCountPublic(void);
s32 GetPieceCenterPositions(Vec *positions);
s32 GetPieceTransformedPositions(Vec *positions);
void SetActivePieceCount(s32 pieceCount);
s32 FindPieceAtPosition(Vec *position, Vec *piecePosition);

#endif

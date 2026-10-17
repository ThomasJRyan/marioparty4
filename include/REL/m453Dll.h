#ifndef REL_M453DLL_H
#define REL_M453DLL_H

#include "game/object.h"

s32 PlayerScoreGet(void);
void MapInitialize(HUPROCESS *objectManager);
void MapCleanup(void);
void MapTransitionRotationSpeedSet(float rotationStep);
void MapSpaceSelect(s32 spaceCount);
void MapUpdate(void);
float MapPlaneDistance(float worldX, float worldZ);
float MapPositionDistance(Vec *position, float heightOffset);
s32 MapTransitionActiveGet(void);
s32 MapNearestSpaceGet(Vec *neighborPositions);
s32 MapNearestPathGet(Vec *neighborPathPoints);
void MapSpaceHighlightSet(s32 highlightedSpaceCount);
s32 MapSpaceContainsPosition(Vec *position, Vec *hitPosition);
void ScoreDisplayInitialize(HUPROCESS *process, s16 initialBestScore);
void ScoreDisplaySetMode(s32 displayMode);

#endif

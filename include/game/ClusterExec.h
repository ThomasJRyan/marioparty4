#ifndef _GAME_CLUSTER_EXEC_H
#define _GAME_CLUSTER_EXEC_H

#include "game/hsfformat.h"
#include "game/hu3d.h"

float GetClusterCurve(HSFTRACK *track, float sample_time);
float GetClusterWeightCurve(HSFTRACK *track, float sample_time);
void SetClusterMain(HSFCLUSTER *cluster);
void ClusterProc(HU3DMODEL *model);
void ClusterMotionExec(HU3DMODEL *model);

#endif

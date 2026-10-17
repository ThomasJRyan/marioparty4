#ifndef REL_M455DLL_H
#define REL_M455DLL_H

#include "game/object.h"

void StageObjectsCreate(HUPROCESS *objectManager);
void StageObjectsDestroy(void);
s32 SnowParticleSpawn(Vec *position, float horizontalSpeed, float depthSpeed, s32 lifetime);
float PlatformHeightGet(void);
s32 SnowmanParticleSpawn(Vec *position, float scale);

#endif

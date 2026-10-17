#ifndef REL_M406DLL_H
#define REL_M406DLL_H

#include "game/object.h"
#include "game/process.h"

typedef struct MapPathSegment {
    /* 0x00 */ float segmentLength;
    /* 0x04 */ float segmentStartDistance;
    /* 0x08 */ Vec segmentDirection;
    /* 0x14 */ Vec segmentMidpoint;
} MapPathSegment; /* size = 0x20 */

s32 GetSceneState(void);
void BeginCameraTransition(void);
void SetSelectedPlayer(s32 source);
void AdvanceCameraTransition(void);
void CompleteSceneTransition(void);

OMOBJ *createMapScene(HUPROCESS *source);
void InitializeMapEffects(void);
void SetMapState(s32 source);
void SetMapPathSpeed(float scale);
void StopMapPathAudio(void);
void GetMapPathOrigin(Vec *source);
s32 GetPathLateralOffset(Vec source);
void SpawnPathParticles(Vec *source);
void SetModelEmissionTransform(Mtx source, s32 destination, float scale);
s32 SetEffectTransform(Mtx source, s32 destination, s32 effectColor);
void SetAnimationGroupAlpha(float scale);
void CreateSceneLight(void);
void DestroySceneLight(void);
void InitializePlayerSystem(HUPROCESS *source);
void CreatePlayerObjects(void);

extern MapPathSegment *pathSegments;
extern s32 pathSegmentCount;

#endif

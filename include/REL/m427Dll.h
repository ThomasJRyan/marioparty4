#ifndef REL_M428DLL_H
#define REL_M428DLL_H

#include "dolphin.h"
#include "game/object.h"

typedef struct SoundEffectLifetime {
    s32 soundEffectId;
    s32 remainingFrames;
} SoundEffectLifetime;

s32 GetGameplayState(void);
void SetTeamProgress(s32 teamIndex, float progress);
s32 GetWinningTeamResult(void);
s32 GetResultCameraState(void);

void InitializeMapSystem(HUPROCESS *mapProcess);
void PlayTeamRouteEffect(s8 effectIndex, s16 teamIndex);
void DestroyMapSystem(void);
void SetBannerAnimationOffset(s16 teamIndex, float offsetX, float offsetY, float scaleX, float scaleY);
float CalculateTeamMarkerOpacity(s16 teamIndex, float distance, float opacityScale);
void InitializePlayerSystem(HUPROCESS *playerProcess);
void DestroyPlayerSystem(void);
s32 FindCharacterModelIndex(s32 teamIndex, s32 *modelIds);
void GetTeamCharacterIds(s32 teamIndex, s32 *characterIds);
void GetTeamPlayerIndices(s32 teamIndex, s32 *playerIndices);
void SetPlayerModelOpacity(float modelDepthFactor);
s32 GetTeamComputerPlayerCount(s32 teamIndex);
float InterpolateAngle(float interpolationRate, float currentAngle, float targetAngle);

s32 TestRouteSegmentCollision(Vec firstRoutePoint, Vec secondRoutePoint, s32 routeIndex, s32 collisionShape, s32 collisionSide);
s32 TestRouteCircleCollision(Vec circleCenter, float collisionRadius, s32 routeIndex, s32 collisionShape, s32 collisionSide);

extern s32 routeSegmentCount;
extern Vec *baseLeftRouteEdge;
extern Vec *baseRightRouteEdge;
extern Vec *baseRouteCenterline;
extern SoundEffectLifetime soundEffectLifetimes[8];

#endif

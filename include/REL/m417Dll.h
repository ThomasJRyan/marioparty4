#ifndef REL_M417DLL_H
#define REL_M417DLL_H

#include "game/object.h"
#include "game/process.h"

typedef struct WaterContactInfo {
    /* 0x00 */ s32 waterContactIndex;
    /* 0x04 */ float waterHeight;
    /* 0x08 */ float waveAngle;
    /* 0x0C */ float waveStrength;
} WaterContactInfo; /* size = 0x10 */

void UpdateMinigameFrame(OMOBJ *object);
void AdvanceMinigameState(OMOBJ *object);
u32 GetSequenceState(void);
u32 GetResultState(void);
void SetMinigameMode(u32 mode);
s32 GetPlayerWinSequence(s32 playerIndex);
void MarkPlayerItemCollected(s32 playerIndex);
void MarkPlayerScore(s32 playerIndex);
s32 IsResultSequenceFinished(void);
void SetPlayerResult(s32 playerIndex, s32 result, s32 coinResult);
float InterpolateAngle(float startAngle, float targetAngle, float blendFactor);
float AngleDifference(float startAngle, float targetAngle);
void CreateWaterObjects(HUPROCESS *objectManager);
void DestroyWaterObjects(void);
void SetWaterWave(float worldX, float worldY, float worldZ, float waveStrength);
float GetWaterHeight(void);
float GetWaterHeightAtPosition(float worldX, float worldY, float worldZ);
WaterContactInfo *getWaterContactInfo(void);
void CreatePlayerObjects(HUPROCESS *objectManager);
OMOBJFUNC StartIntroCameraSequence(HUPROCESS *objectManager);
OMOBJFUNC StartResultsSequence(HUPROCESS *objectManager, OMOBJ *minigameObject);

#endif

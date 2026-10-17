#ifndef REL_M420DLL_H
#define REL_M420DLL_H

#include "game/object.h"

typedef struct M420DllCameraPose {
    /* 0x00 */ Vec lookAtPosition;
    /* 0x0C */ Vec cameraAngles;
    /* 0x18 */ float cameraDistance;
} M420DllCameraPose; /* size = 0x1C */ // same as in m447dll/camera.c

typedef struct M420DllPlayerStruct {
    /* 0x00 */ struct {
        s8 isComputerControlled : 1;
        s8 itemHookAttached : 1;
        s8 rotationTransitionActive : 1;
    };
    /* 0x04 */ s32 playerIndex;
    /* 0x08 */ u8 difficulty;
    /* 0x0A */ s16 characterId;
    /* 0x0C */ u8 controllerPort;
    /* 0x10 */ Vec position;
    /* 0x1C */ Vec rotation;
    /* 0x28 */ Vec rotationStart;
    /* 0x34 */ Vec rotationTarget;
    /* 0x40 */ HUPROCESS *playerProcess;
    /* 0x44 */ char reservedPlayerData[4];
    /* 0x48 */ s32 actionState;
    /* 0x4C */ s32 formationIndex;
    /* 0x50 */ s16 actionProgress;
    /* 0x52 */ char reservedAnimationData[6];
    /* 0x58 */ s32 actionDelay;
    /* 0x5C */ s32 activeEffectSprites[0xA];
    /* 0x84 */ s32 effectSpriteFrames[0xA];
    /* 0xAC */ s16 characterModelId;
    /* 0xAE */ s16 characterMotionIds[0xB];
    /* 0xC4 */ s16 effectSpriteIds[0xA];
    /* 0xD8 */ s16 statusSpriteId;
    /* 0xDC */ float rotationProgress;
    /* 0xE0 */ float rotationProgressStep;
    /* 0xE4 */ s32 rotationEasingMode;
    /* 0xE8 */ struct M420DllPlayerStruct *linkedPlayer;
} M420DllPlayerStruct; /* size = 0xEC */

void UpdateCamera(void);

s32 InitializeMapSystem(void);
void ShutdownMapSystem(void);
s32 StartMapEvent(s32);
s32 AdvanceMapReveal(s32 effectIndex, float scaleStep);
s32 AdvanceRoundCounter(void);
void ResetRoundCounter(void);
void ResetMapMotion(void);
void FinishRoundCounter(void);
void ShowMapMarker(void);
s16 GetMapMarkerModel(void);
void SetMapMarkerLighting(s32 enabled);
void HideMapMarker(void);
s32 GetMapEffectFlags(s32);
s32 GetMapRoundIndex(void);
void InitializeRandomGenerator(void);
s32 GetRandomInteger(void);
M420DllPlayerStruct *createPlayer(s32, s32);
void DestroyPlayer(M420DllPlayerStruct *);

extern HUPROCESS *minigameObjectManager;

#endif

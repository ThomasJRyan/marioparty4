#ifndef REL_M408DLL_H
#define REL_M408DLL_H

#include "game/object.h"

typedef struct CharacterEffectWork {
    u16 playerIndex;
    u16 characterId;
    u32 animationState;
    u32 animationSpeed;
    float motionScale;
    u32 animationEventTimer;
    u32 score;
    u32 motionDirection;
    Vec position;
    Vec velocity;
    Vec velocityChange;
    Vec steeringVector;
    Vec modelRotation;
    Vec rotation;
    float animationPhaseTimer;
    Vec modelOffset;
    u32 steeringTimer;
    u32 steeringUpdateCounter;
    float steeringOffset[2];
    float steeringAngles[2];
    s16 targetCharacterIndex;
    u16 steeringState;
    float movementSpeed;
    Vec particlePosition;
    Vec particleScaleVector;
    Vec reservedVectorAC;
    Vec reservedVectorB8;
} CharacterEffectWork;

void SetStageProgress(u16 stageIndex, u16 progressState);
u32 GetStageEventState(void);
float GetStageEventValue(void);
CharacterEffectWork *GetCharacterWork(s32 playerIndex);
void GetCameraPositionRotation(Vec *cameraPosition, Vec *cameraRotation);
void SetCameraParameters(float targetX, float targetY, float targetZ, float positionX, float positionY, float positionZ);
float ApproachAngle(float currentAngle, float targetAngle, float stepScale);
void GetCameraTargetTransform(Vec *cameraPosition, Vec *cameraRotation);
u32 GetCameraSequenceState(void);
s32 GetStageEffectState(void);
void CleanupStageEffects(void);
void CreateCameraObjects(HUPROCESS *objman);
void CreateStageEffects(HUPROCESS *objman);
s32 GetRandomStageValue(void);
void CreateEffectObjects(HUPROCESS *objman);
s32 FindNearestEffectPosition(Vec *referencePosition, Vec *nearestPosition);

extern s32 minigameExitAllowed;
extern s32 minigameExitRequested;

#endif

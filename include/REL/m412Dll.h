#ifndef REL_M412DLL_H
#define REL_M412DLL_H

#include "dolphin.h"

#include "game/hu3d.h"
#include "game/object.h"


// types
typedef struct _DebrisPiece {
    s16 modelId;
    s16 reflectedModelId;
    s16 settledModelId;
    s16 slotIndex;
    s16 hasCollided;
    s16 particleSpawnCount;
    s16 debrisType;
    Vec position;
    HuVec2f velocity;
    Vec launchVelocity;
    float collisionRadius;
} DebrisPiece;

typedef struct _SceneModelSet {
    s16 primaryModelId;
    s16 reflectedModelId;
    s16 detailModelId;
    s16 effectModelId;
    s16 linkedModelId;
    s16 auxiliaryModelId;
    s16 characterMotions[5];
    s16 jointMotions[5];
    s16 activeState;
    s32 playerIndex;
    s32 selectionOrder;
    Vec position;
    Vec rotation;
} SceneModelSet; // sizeof 0x44

typedef struct _PlayerState {
    s16 characterModelId;
    s16 reflectedModelId;
    s16 resultModelId;
    s16 effectModelId;
    s16 auxiliaryModelId;
    s16 extraModelId;
    s16 characterMotions[6];
    s16 jointMotions[6];
    s32 playerIndex;
    s32 selectionOrder;
    s32 padIndex;
    s32 roundStatus;
    char reserved34[0x4];
    s32 collidingPlayerIndex;
    s32 circleSlot;
    Vec position;
    HuVec2f previousPosition;
    Vec rotation;
    Vec collisionParameters;
    float headingAngle;
    char reserved70[0xC];
    Vec velocity;
    s8 stickX;
    s8 stickY;
    s8 triggerRight;
    s8 triggerLeft;
    u16 buttonState;
    u16 buttonsPressed;
    HuVec2f analogStick;
    float fleeDistanceThreshold;
    s32 playerStateFlags;
    s32 targetDebrisSlot;
    s32 fleeFramesRemaining;
} PlayerState; // sizeof 0xA8

typedef struct _ShardAnimationData {
    s32 modelId;
    s32 objectIndex;
    s32 animationActive;
    Vec originPosition;
} ShardAnimationData;

// prototypes
void ObjectSetup(void);
void *playPositionedSoundEffect(float, float, float, s32);
void UpdateAudioListenerProcess(void);
void MoveSoundEmitterProcess(void);
void InitializeHiddenObject(OMOBJ *object);
void InitializeHiddenObjectCallback(OMOBJ *object);
void InitializeMinigameScene(void);
float WrapDegrees(float);
void FreeShardAnimationData(void);
void AnimateShardFragment(void);
void ReleaseDebrisSlot(void);
void RunDebrisPhysics(void);
s32 AllocateDebrisSlot(void);
float CalculateAngularStep(s32, float, float);
void RunGameplaySequence(void);
void RunPlayerAnimationSequence(void);
s8 CheckPlayerCollision(s32, s32);
void RotateVectorTowardAngle(Vec *, float, float);
s8 CheckPlayerCollisionAlternate(s32, s32);
u8 GetPlayerCollisionState(s32, s32);
u16 GetPlayerCollisionValue(s32);
u16 GetPlayerCollisionFlags(s32);
s32 FindPlayerCollision(PlayerState *, float, float, float);
s32 FindNearestPlayer(PlayerState *, s32 *);
void ResolvePlayerCollisions(PlayerState *);
float InterpolateAngle(float, float, float);
float CalculatePlayerAngle(float, float, PlayerState *);
void UpdatePlayerState(PlayerState *);
void InitializeParticleEffects(void);
void UpdateDebrisParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void UpdateSparkParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
//...
void UpdateShardParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix); // *
void WaitForMinigameExit(void); // *
void RunMinigameController(void); // *

#endif

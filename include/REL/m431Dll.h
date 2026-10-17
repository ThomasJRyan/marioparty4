#ifndef REL_M431DLL_H
#define REL_M431DLL_H

#include "game/object.h"

#define rand16() ((rand8() << 8) | (rand8()))

typedef struct PlayerCollisionBoundary {
    Vec planeNormal;
    float planeDistance;
    s8 dominantNormalAxis;
} PlayerCollisionBoundary;

typedef struct PlayerGroupState {
    s16 playerIds[2];
    s16 playerGroupId;
    s16 groupState;
    s16 playerCount;
    Vec startingPosition;
    float itemRingRadius;
    PlayerCollisionBoundary collisionBoundaries[10];
    float itemPositions[10][2];
    s16 playerReadyFlags[2];
    s16 completedRoundCounts[2];
} PlayerGroupState;

typedef struct PlayerCharacterState {
    u32 stateFlags;
    s16 playerIndex;
    s16 controllerIndex;
    s16 characterId;
    s16 groupIndex;
    s16 isHuman;
    float modelRadiusOffset;
    struct PlayerCharacterState *opponent;
    Vec initialPosition;
    Vec position;
    float facingAngle;
    OMOBJFUNC motionCallback;
    s16 motionIndex;
    s16 queuedMotionIndex;
#if VERSION_PAL
	s16 palMotionTransitionFlag;
	s16 unusedPalState3E;
#endif
    u32 queuedMotionFlags;
    s16 itemIndex;
    s16 targetItemIndex;
    Vec ringMovementState;
    s16 previousItemIndex;
    s8 stickInputState;
    u32 directionInputFlags;
    s16 previousStickX;
    s16 previousStickY;
    s16 capturedItemModelId;
    s16 availableItemIndices[8];
    s16 availableItemCount;
    s16 selectedItemCount;
    s16 computerChoiceBias;
    u32 unusedState74;
    s16 facingTransitionActive;
    s16 actionButtonState;
    s16 computerInputDelay;
    s16 opponentPriorityFlag;
    s16 resultAnimationState;
    s16 unusedState82;
    s16 groupTargetItemIndex;
    s16 hasMovementInput;
    float unusedFloat88;
    Vec facingTransition;
    float unusedFloat98;
    Vec resultFacingTransition;
    float unusedFloatA8;
    s32 alternateResultMotion;
    s16 buttonsHeld;
    s16 buttonsPressed;
    s8 stickX;
    s8 stickY;
} PlayerCharacterState;

typedef void (*ParticleUpdateCallback)(struct ParticleState *);
typedef void (*ParticleSystemCallback)(HU3DMODEL *, struct ParticleSystemState *, Mtx);

typedef struct ParticleState {
    Vec velocity;
    Vec velocityDecay;
    char reservedParticleData[16];
    s16 lifetimeFrame;
    s16 lifetimeLimit;
    s16 modelId;
    ParticleUpdateCallback updateCallback;
    s16 particleIndex;
    float rotation;
    Vec scale;
    Vec position;
    s16 textureTileIndex;
    s8 isActive;
} ParticleState;

typedef struct ParticleSystemState {
    s16 particleCount;
    u32 displayListSize;
    u8 renderMode;
    ParticleSystemCallback renderCallback;
    ANIMDATA *animation;
    s16 textureTileCount;
    ParticleState *particles;
    Vec *vertexPositions;
    HuVec2f *textureCoordinates;
    GXColor *vertexColors;
    void *displayList;
    Vec quadVertices[4];
    HuVec2f *textureTiles;
    float textureTileWidth;
    float textureTileHeight;
} ParticleSystemState;

extern s32 playerWon;
extern PlayerGroupState playerGroups[4];
extern OMOBJ *playerObjects[4];
extern float playerItemSpinOffset;

void SetSelectedWinner(s16 playerIndex);
void UpdateSequenceStatusFlags(u16 clearMask, u16 setMask);
u16 GetSequenceStatusFlags(u16 statusMask);
s16 GetStoryItemAnimationPhase(void);
u32 GetStoryItemFlags(s16 playerSide, s16 itemIndex);
s16 SelectStoryItem(s16 playerSide, s16 itemIndex, s16 *playerMatched);
s16 IsStoryItemAvailable(s16 playerSide, s16 itemIndex);

void CreateParticleEffect(float x, float y, float z, s16 particleCount, ParticleUpdateCallback updateCallback, s16 modelId);

void CalculateCollisionPlane(Vec *triangleVertices, PlayerCollisionBoundary *collisionPlane);
float InterpolateAngle(float startAngle, float endAngle, float progress);

void InitializePlayerGroups(HUPROCESS *objman);
s32 MovePlayerPairToTarget(s16 groupIndex);
void StartPlayerGroupVictory(s16 groupIndex);
void StartPlayerGroupDraw(s16 groupIndex);

s32 CheckPlayerGroupReady(s16 groupIndex);

#endif

#ifndef REL_M447DLL_H
#define REL_M447DLL_H

#include "game/animdata.h"
#include "game/process.h"

typedef struct {
    /* 0x00 */ HUPROCESS* mainProcess;
    /* 0x04 */ HUPROCESS* dialogProcess;
    /* 0x08 */ s32 shutdownComplete;
    /* 0x0C */ s32 dialogComplete;
    /* 0x10 */ s32 bonusRoundActive;
    /* 0x14 */ s32 gamePhase;
    /* 0x18 */ s32 computerPlayerCount;
    /* 0x1C */ s32 phaseStep;
    /* 0x20 */ s32 phaseTimer;
    /* 0x24 */ s32 roundTimer;
    /* 0x28 */ s32 roundState;
    /* 0x2C */ s32 roundNumber;
    /* 0x30 */ s16 resultWindowId;
    /* 0x32 */ s16 countdownWindowId;
    /* 0x34 */ s16 hudSprites[25];
    /* 0x66 */ char alignmentPadding[0xA];
    /* 0x70 */ s32 musicSequenceId;
} M447GameState; // Size 0x74

typedef struct {
    /* 0x00 */ s16 modelIds[22];
    /* 0x2C */ s16 lightIds[6];
    /* 0x38 */ s32 actionState;
    /* 0x3C */ s32 actionTimer;
    /* 0x40 */ s32 stateTimer;
    /* 0x44 */ s32 countdownFrames;
    /* 0x48 */ float cardMotionProgress;
    /* 0x4C */ float cardMotionSpeed;
    /* 0x50 */ ANIMDATA* stageAnimationData;
    /* 0x54 */ s32 playerCardMotionActive[5];
    /* 0x68 */ s32 playerCardRevealed[5];
    /* 0x7C */ HUPROCESS* process;
} M447StageState; // Size 0x80

typedef struct {
    /* 0x00 */ s32 cardIndex;
    /* 0x04 */ s32 actionState;
    /* 0x08 */ s32 specialCardFlag;
    /* 0x0C */ s32 actionStep;
    /* 0x10 */ Vec position;
    /* 0x1C */ Vec rotation;
    /* 0x28 */ Vec rotationStart;
    /* 0x34 */ float targetRotationX;
    /* 0x38 */ float targetRotationY;
    /* 0x3C */ float targetRotationZ;
    /* 0x40 */ float modelRotationX;
    /* 0x44 */ float modelRotationY;
    /* 0x48 */ float modelRotationZ;
    /* 0x4C */ float modelScaleX;
    /* 0x50 */ float modelScaleY;
    /* 0x54 */ float modelScaleZ;
    /* 0x58 */ s16 modelIds[23];
    /* 0x86 */ char alignmentPadding[2];
    /* 0x88 */ float rotationSpeedY;
    /* 0x8C */ float modelAlpha;
    /* 0x90 */ float actionProgress;
    /* 0x94 */ float rotationProgress;
    /* 0x98 */ float actionProgressStep;
    /* 0x9C */ float rotationProgressStep;
    /* 0xA0 */ ANIMDATA* particleAnimation;
    /* 0xA4 */ HUPROCESS* process;
    /* 0xA8 */ Vec particlePath[3];
} M447BlockState; // Size 0xCC

typedef struct {
    /* 0x00 */ s16 playerIndex;
    /* 0x02 */ char alignmentPadding02[2];
    /* 0x04 */ s32 inputEnabled;
    /* 0x08 */ s32 isComputer;
    /* 0x0C */ s32 movementState;
    /* 0x10 */ s32 cardSelectionActive;
    /* 0x14 */ s32 roundWinFlag;
    /* 0x18 */ u8 characterIndex;
    /* 0x19 */ u8 difficulty;
    /* 0x1A */ u8 controllerIndex;
    /* 0x1B */ u8 scoreCount;
    /* 0x1C */ s32 isSpecialNpc;
    /* 0x20 */ s16 stateStep;
    /* 0x22 */ char alignmentPadding22[2];
    /* 0x24 */ s32 state;
    /* 0x28 */ Vec position;
    /* 0x34 */ float rotationX;
    /* 0x38 */ float facingAngle;
    /* 0x3C */ float rotationZ;
    /* 0x40 */ float movementSpeed;
    /* 0x44 */ float movementX;
    /* 0x48 */ float movementY;
    /* 0x4C */ float movementZ;
    /* 0x50 */ float collisionRadius;
    /* 0x54 */ char reserved54[0xC];
    /* 0x60 */ s32 stateTimer;
    /* 0x64 */ s32 computerActionDelay;
    /* 0x68 */ s16 modelId;
    /* 0x6A */ s16 motionIds[9];
    /* 0x7C */ s16 hudSpriteIds[2];
    /* 0x80 */ char reserved80[0x24];
    /* 0xA4 */ s32 activeMotionId;
    /* 0xA8 */ M447BlockState* selectedBlock;
    /* 0xAC */ HUPROCESS* process;
    /* 0xB0 */ HUPROCESS* npcRightDustProcess;
    /* 0xB4 */ HUPROCESS* npcLeftDustProcess;
} M447PlayerState; // Size 0xB8

M447StageState* M447StageCreate(void);
void M447StageDestroy(M447StageState* stage);
void M447StageActionSet(M447StageState* stage, s32 actionIndex);
BOOL M447StageActionActiveCheck(M447StageState* stage);
void M447PlayerSpawnPositionGet(s32 cardIndex, Vec* position);
void M447PlayerCardReveal(M447StageState* stage, s32 playerIndex);
s32 M447UnrevealedPlayerCountGet(M447StageState* stage);
void M447StageCountdownSet(M447StageState* stage, s32 frameCount);

s32 M447CameraInit(void);
void M447CameraShutdown(void);
void M447CameraPositionTargetSet(const Vec* targetPosition, s32 duration);
void M447CameraLookTargetSet(const Vec* targetDirection, s32 duration);
void M447CameraZoomTargetSet(float targetZoom, s32 duration);
void M447CameraPositionGet(Vec* position);
void M447CameraPositionSet(Vec* position);

void M447PlayerRegistryInit(void);
M447PlayerState* M447PlayerCreate(s32 playerIndex);
void M447PlayerStateSet(M447PlayerState* player, s32 state);
void M447PlayerDisplayOff(M447PlayerState* player);
void M447PlayerVibrationSet(M447PlayerState* player, s32 vibrationType, s32 duration);

void M447SplinePositionGet(Vec* resultPosition, Vec* controlPoints, s32 pointCount, float splineProgress);
s32 M447CircleLineIntersectionGet(float* circle, float* line, float* intersections);
s32 M447CircleCollisionResolve(float* movingCircle, float* targetCircle, s32 moveFirst, s32 adjustTarget);

M447BlockState* M447BlockCreate(void);
void M447BlockActionSet(M447BlockState* block, s32 action);
void M447BlockCardSet(M447BlockState* block, s32 cardIndex, s32 isSpecialCard);
void M447BlockLayoutSet(s32 alternateLayout);
void M447RandomSeedSet(s32 seed);
s32 M447RandomValueGet(void);

extern M447StageState* stageState;
extern M447GameState* gameState;
extern HUPROCESS* objectManager;
extern M447PlayerState* playerStates[2];
extern const float blockLandingHeights[20];
extern M447BlockState* blockStates[5];

#endif

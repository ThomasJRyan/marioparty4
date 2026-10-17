#ifndef REL_M410DLL_H
#define REL_M410DLL_H

#include "dolphin.h"
#include "game/hsfformat.h"
#include "game/process.h"
#include "game/object.h"

typedef struct MinigameStateWork {
    u32 sequenceState;
    s32 elapsedFrames;
    s32 transitionBlocked;
    s32 minigameStateFlags;
    s32 introTimer;
    u32 resultState;
    s32 resultTimer;
    u32 resultSequenceParameter;
    u32 countdownTimer;
    s32 resultPhase;
    u32 resultPhaseTimer;
    s32 playerFlagMask;
    s32 playerControlFlags;
    s32 playerSequenceFlags;
} MinigameStateWork; // size 0x38

typedef struct PlayerPhysicsWork {
    struct {
        u8 isActive : 1;
        u8 isConnectedToGoal : 1;
        u8 isVisible : 1;
        u8 isVerticalCollisionState : 1;
        u8 goalCollisionPending : 1;
        u8 collisionUpdatePending : 1;
        u8 wrappedAroundStage : 1;
        u8 reservedFlag7 : 1;
    };
    Vec position;
    Vec previousPosition;
    Vec velocity;
    float collisionRadius;
    float collisionHeight;
    s16 modelId;
    s16 sphereIndex;
    s16 playerIndex;
    s16 stateTimer;
    struct PlayerPhysicsWork *linkedPhysicsWork;
    s16 linkTimer;
    void *collisionPartner;
    s16 collisionCooldown;
    s16 reservedState46;
} PlayerPhysicsWork; // size 0x48

typedef struct PlayerVectorWork {
    s32 reservedWords[6];
    Vec directionVector;
} PlayerVectorWork; // size 0x24

typedef struct PlayerCollisionWork {
    s32 collisionType;
    u8 collisionAxes;
    u8 collisionKind;
    u8 firstContactFrame;
    u8 reservedByte07;
    PlayerPhysicsWork *firstPhysicsWork;
    union {
        PlayerPhysicsWork *secondPhysicsWork;
        OMOBJ *collidingObject;
    };
} PlayerCollisionWork; // size 0x10

typedef struct PlayerCharacterWork {
    s32 characterModelId;
    u32 randomizedPlayerRank;
    s32 hookIndex;
    s32 currentMotionId;
    u32 pressedButtons;
    s32 resetModelPose;
    s32 isGrounded;
    s32 isJumping;
    s32 isStunned;
    s32 specialInputReady;
    s32 movementActive;
    s32 alternateActionReady;
    s32 actionRequested;
    u16 animationState;
    s16 stateTimer;
    s16 animationTimer;
    s16 reservedS163a;
    float currentFacingAngle;
    float targetFacingAngle;
    float facingAngleStep;
    float facingAngleAcceleration;
    float movementProbability;
    u16 aiDecisionState;
    u16 aiDecisionDelay;
    u16 aiActionDuration;
    Vec worldPosition;
    float reservedFloat64;
    float verticalVelocity;
    float reservedFloat6c;
    Vec modelAnchorPosition;
    Vec movementVelocity;
    Vec previousTargetPosition;
} PlayerCharacterWork; // size 0x94

typedef struct PlayerFloatParameters {
    float movementChanceByDifficulty[4];
} PlayerFloatParameters; // size 0x10

typedef struct PlayerIntegerParameters {
    u32 playerWaitFrames[4];
} PlayerIntegerParameters; // size 0x10

typedef struct PlayerModelWork {
    u16 playerIndex;
    u16 characterId;
    float modelOpacity;
    float animationBlend;
    float animationSpeed;
    Vec modelPosition;
} PlayerModelWork; // size 0x1C

typedef struct PlayerAnimationWork {
    s32 scorePopupActive;
    s32 scoreDelta;
    u32 displayedScore;
    float scoreDisplayX;
    float scoreDisplayY;
    s32 popupState;
    s32 popupTimer;
    s32 tensDigit;
    s32 onesDigit;
    s32 hundredsDigit;
    s32 reservedScoreValue;
    float digitScale[2];
    s32 reservedDigitData[2];
    s16 digitSpriteIds[2];
    s16 characterSpriteId;
    s16 scoreIconSpriteId;
    s16 scoreLabelSpriteId;
    s16 reservedSpriteData;
} PlayerAnimationWork; // size 0x48

typedef struct PlayerMotionTiming {
    float blendInFrames;
    float blendOutFrames;
    float motionDuration;
    s32 motionAttributes;
} PlayerMotionTiming; // size 0x10

typedef struct PlayerMotionWork {
    float vertex0X;
    float vertex0Z;
    float vertex1X;
    float vertex1Z;
    float vertex2X;
    float vertex2Z;
    float vertex3X;
    float vertex3Z;
    float vertex4X;
    float vertex4Z;
    float vertex5X;
    float vertex5Z;
    float vertex6X;
    float vertex6Z;
    float vertex7X;
} PlayerMotionWork; // size 0x3C

typedef struct PlayerMotionParameterWork {
    float radialOffsetX;
    float radialOffsetZ;
    float reservedParameter08;
    float reservedParameter0c;
    float reservedParameter10;
    float reservedParameter14;
    float reservedParameter18;
    float reservedParameter1c;
    float reservedParameter20;
    float reservedParameter24;
} PlayerMotionParameterWork; // size 0x28

typedef struct PlayerDisplayListWork {
    PlayerModelWork modelWork[16];
    PlayerMotionWork particleVertexPositions[32];
    PlayerMotionParameterWork particleVertexTextureCoordinates[32];
    GXColor particleVertexColors[5];
    s32 displayListSize;
    u32 displayListLength;
    void *displayListBuffer;
} PlayerDisplayListWork; // size 0xE60

typedef struct PlayerRenderWork {
    PlayerAnimationWork animationWork[4];
    PlayerDisplayListWork displayListWork;
    u32 framebufferSize;
    void *framebufferBuffer;
    char reservedRenderData[0x2E98];
} PlayerRenderWork; // size 0x3E20

s32 GetMinigameState(void);
u32 GetMinigamePhase(void);
u32 GetMinigameSubstate(void);
u32 GetMinigameFlags(void);
void SetMinigameFlags(s32 flags);
u32 GetMinigameControlFlags(void);
void SetMinigameControlFlag(s32 playerIndex);
void SetMinigamePlayerFlag(s32 playerIndex);
s32 IsMinigameSequenceDone(void);
float ApproachAngle(float currentAngle, float targetAngle, float stepAngle);
float AngleDifference(float firstAngle, float secondAngle);
void InitializeStageObjects(HUPROCESS *process);
void InitializeStageState(void);
void InitializeGameObjects(HUPROCESS *objman);
void SetGameObjectsHidden(void);
s32 SetPlayerSphereState(Vec position, Vec velocity, u16 playerIndex);
void SetGoalCallback(void *callback);
void GetPlayerPosition(u16 playerIndex, Vec *positionOut);
void GetPlayerVelocity(u16 playerIndex, Vec *velocityOut);
void GetPlayerFuturePosition(u16 playerIndex, u16 framesAhead, Vec *positionOut);
s32 FindModelJointMatrix(s16 modelId, char *jointName, Mtx transformOut);
void SearchJointHierarchy(HSFOBJECT *joint, Mtx parentTransform, char *jointName, Mtx transformOut);
void InitializePlayers(HUPROCESS *objman);
void CleanupPlayers(void);

#endif

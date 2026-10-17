#include "datadir_enum.h"
#include "dolphin/mtx.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/flag.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "string.h"

#include "REL/m433Dll.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/frand.h"
#endif

typedef struct M433PlayerState {
    s32 characterId;
    s32 npcModelIndex;
    s32 unusedPlayerState08;
    s32 actionCounter;
    u32 playerFlags10;
    u32 teamIndex;
    u32 playerSide;
    u32 pressedButtons;
    s32 heldButtons;
    float stickMagnitude;
    float stickAngle;
    s32 isCpu;
    s32 isPlaying;
    s32 hasActiveAction;
    s32 actionSubstate;
    s32 actionFlags;
    s32 actionCooldownActive;
    s32 opponentTargetingState;
    s32 opponentRecentState;
    s32 isHighBallContact;
    char reservedPlayerPadding50[4];
    s32 movementLockActive;
    s32 isItemCrossingNearPlayer;
    s32 canContactBall;
    s32 hasContactedBall;
    s32 contactFollowThroughActive;
    s32 attackReady;
    s32 defenseActive;
    s32 previousGamePhase;
    u32 playerActionState;
    u32 stateFlags78;
    u32 stateFlags7C;
    u32 stateFlags80;
    u32 escapeState;
    u32 framesInAction;
    s32 actionFrame;
    u32 aiActionFlags;
    char reservedPlayerPadding94[4];
    float lastBallContactTime;
    s16 playerState9C;
    char reservedPlayerPadding9E[6];
    float currentRotation;
    float targetRotation;
    float rotationStep;
    float rotationAcceleration;
    float modelScale;
    float modelScaleTarget;
    float modelRotation;
    float aiDifficulty;
    char reservedPlayerPaddingC4[8];
    u32 aiReactionDelay;
    u32 aiReactionTimer;
    u32 aiActionThreshold;
    u32 aiActionCount;
    float aiTargetAngle;
    float aiTargetSpeed;
    s32 aiTargetFlag;
    s32 aiTargetMode;
    s32 aiPreviousTargetMode;
    s32 aiActionMode;
    s32 aiDecisionState;
    u32 aiDecisionFlags;
    Vec retreatPosition;
    Vec aiTargetPosition;
    Vec ballTargetPosition;
    float characterSkill;
    float aiMovementSpeed;
    Vec position;
    Vec velocity;
    Vec movementTarget;
    OMOBJ *opponentObject;
} M433PlayerState; /* size = 0x150 */

typedef struct M433ItemTrajectory {
    float flightDuration;
    float elapsedTime;
    float floorCrossingTime;
    float floorHeightThreshold;
    float horizontalDistance;
    float horizontalVelocity;
    float horizontalAcceleration;
    float verticalDisplacement;
    float verticalVelocity;
    float verticalAcceleration;
    float trajectoryScale;
    Vec startPosition;
    Vec targetPosition;
} M433ItemTrajectory; /* size = 0x44 */

typedef struct M433ItemManagerState {
    s32 playerVisualMode;
    s32 itemCollected;
    s32 itemActionState;
    s32 itemOutcome;
    s32 itemActive;
    char reservedItemPadding14[4];
    u16 playerSide;
    char reservedItemPadding1A[0x16];
    Vec position;
    Vec targetPosition;
    float elapsedTime;
    Vec itemStartPosition;
    char reservedItemPadding58[0x14];
    u32 itemType;
    s32 itemAction;
    s32 itemSubAction;
    u32 itemAnimationActive;
    float itemScaleTargets[2];
    float itemScale[2];
    s32 itemModelState;
    u32 itemEffectState;
    u32 itemSpinState;
    s32 itemSpinScale;
    float itemScaleFactor;
    float itemScaleTarget;
    float itemWorldOffset;
    float itemSpinSpeed;
    Vec itemPosition;
    float spinAngle;
    Vec spinAxis;
    Mtx rotationMatrix;
    M433ItemTrajectory itemPhysics;
} M433ItemManagerState; /* size = 0x13C */

typedef struct M433PlayerScoreDisplay {
    s16 score;
    s16 digitSpriteIds[5];
} M433PlayerScoreDisplay; /* size = 0xC */

typedef struct M433ScoreAnimationState {
    s16 digitSpriteIds[2];
    float digitXOffsets[2];
    float displayX;
    float displayY;
    float xVelocity;
    float yVelocity;
    float horizontalScale;
    float verticalScale;
    float opacity;
    float rotation;
} M433ScoreAnimationState; /* size 0x2C */

typedef struct M433ScoreSpriteRow {
    s32 scoreSpriteIds[2];
    char reservedScoreSpritePadding[8];
} M433ScoreSpriteRow; /* size 0x10 */

typedef struct M433ScoreboardState {
    s32 animationState;
    u32 animationFrame;
    u32 animationTimer;
    s32 previousScore;
    s32 previousOpponentScore;
    s32 currentScores[2];
    u32 servingPlayer;
    M433ScoreAnimationState scoreAnimations[4];
    M433ScoreSpriteRow scoreSpriteRows[3][0x14];
    M433ScoreSpriteRow finalScoreSprites;
} M433ScoreboardState; /* size = 0x4A0 */

typedef struct M433GameEventState {
    u32 uGameEvent;
    s32 sWinGrp;
    u32 uServeGrp;
    u32 uPointGrp;
    u32 auServeCel[2];
    u32 auEvents[2];
    u32 playerPoints[2];
    u32 eventFrames;
    s32 pointScored;
    s32 ballInPlay;
    u32 uContactCnt;
    s32 sGroundGrp;
    u32 uContactGrp;
    u32 uContactCel;
    u32 uTargetGrp;
    u32 contactElapsedFrames;
    M433PlayerScoreDisplay scoreboardPlayers[2];
    M433ScoreboardState scoreboardAnimation;
} M433GameEventState; /* size = 0x504 */

typedef struct M433PlayerMotionTransition {
    /* 0x00 */ u16 motionIndex;
    /* 0x04 */ float transitionDuration;
    /* 0x08 */ float motionStartTime;
    /* 0x0C */ float motionEndTime;
    /* 0x10 */ u32 motionAttributes;
} M433PlayerMotionTransition; /* size = 0x14 */

typedef struct M433ItemActionProperties {
    /* 0x00 */ s32 startsContactEvent;
    /* 0x04 */ s32 switchesTargetGroup;
    /* 0x08 */ s32 countsContact;
} M433ItemActionProperties; /* size = 0x0C */

void ClampPlayerPosition(Vec *position, u32 playerSide);
void UpdateItemMotion(float playerSpeed, float depthDifference, u32 playerSide, u32 playImpactSound, Vec *itemPosition);
s32 NormalizeItemPosition(Vec *normalizedPosition);
void ResetItemState(void);
u32 GetItemTargetPosition(Vec *targetPosition);
u32 GetItemPosition(Vec *itemPosition);
float GetItemVerticalSpeed(void);
u32 GetItemState(void);
u32 GetItemAction(void);
float UpdateItemFlightPosition(float elapsedTimeDelta, Vec *itemPosition);
s32 GetItemSoundPosition(Vec *soundPosition);
u32 GetActiveContactGroup(void);
u32 GetPlayerGamePhase(u32 teamIndex);
s32 IsPlayerServing(u32 teamIndex);
u32 GetPlayerServeCell(u32 teamIndex);
s32 IsPlayerContacting(u32 teamIndex);
u32 GetPlayerPointGroup(void);
s32 IsPlayerContact(u32 teamIndex, s32 contactCell);
s32 IsPlayerContactGroup(u32 teamIndex);
float FindClosestPointOnSegments(Vec *firstSegmentStart, Vec *firstSegmentEnd, Vec *secondSegmentStart, Vec *secondSegmentEnd);
void InitializePlayerObject(OMOBJ *object);
void InitializePlayerAnimation(OMOBJ *object);
void UpdatePlayerObject(OMOBJ *object);
void UpdatePlayerAction(OMOBJ *object);
void UpdatePlayerAnimation(OMOBJ *object);
void UpdatePlayerMovement(OMOBJ *object);
void UpdatePlayerCollision(OMOBJ *object);
void UpdatePlayerScoring(OMOBJ *object);
s32 GetPlayerTargetPosition(u32 teamIndex, Vec *targetPosition);
void UpdatePlayerAi(OMOBJ *object);
void UpdatePlayerActionState(OMOBJ *object);
void UpdatePlayerModel(OMOBJ *object);
void UpdatePlayerContact(OMOBJ *object);
void UpdatePlayerResponse(OMOBJ *object);
void UpdatePlayerState(OMOBJ *object);
void SetPlayerAnimation(OMOBJ *object, u32 animationIndex);
s32 GetPlayerState(OMOBJ *object);
s32 GetPlayerActionState(OMOBJ *object);
s32 CheckPlayerAction(OMOBJ *object, u32 actionIndex);
void UpdatePlayerManager(OMOBJ *object);
void UpdatePlayerEvents(OMOBJ *object);
void InitializePlayerEffects(OMOBJ *object);
s32 UpdatePlayerEffects(OMOBJ *object);
void UpdateItemObject(OMOBJ *object);
void InitializeItemTrajectory(M433ItemTrajectory *trajectory, Vec *startPosition, Vec *targetPosition, u32 itemType, float playerSpeed);
void UpdateItemTrajectory(M433ItemTrajectory *trajectory);
void ResetItemTrajectory(M433ItemTrajectory *trajectory);
s32 CalculateItemTrajectory(M433ItemTrajectory *trajectory, Vec *itemPosition, Vec *positionDelta, float elapsedTime);
void UpdateItemParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void ApproachVector(Vec *position, float approachFactor);
void ClampItemPosition(Vec *itemPosition);
void UpdateItemEffectParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);
void InitializeGameStateObject(OMOBJ *object);
void UpdateGameStateObject(OMOBJ *object);
void InitializeContactEvents(M433PlayerScoreDisplay *scoreboardPlayers, M433ScoreboardState *scoreboardAnimation);
void UpdateContactState(M433PlayerScoreDisplay *scoreboardPlayers, M433ScoreboardState *scoreboardAnimation);
void SetPlayerScoreState(M433ScoreboardState *scoreboardAnimation, s32 playerScore, s32 opponentScore, u32 servingPlayer);
void SetPlayerItemState(Vec *itemPosition, u32 itemType, u32 contactGroup, s32 contactCell);

s32 playerOrderIndexes[6] = { 0, 1, 2, 3, 4, 5 };
float characterModelHeights[0xE] = {
    130.0f,
    130.0f,
    160.0f,
    130.0f,
    130.0f,
    160.0f,
    160.0f,
    180.0f,
    240.00002f,
    100.0f,
    130.0f,
    130.0f,
    120.00001f,
    130.0f,
};
float characterSkillValues[0xE] = {
    50.0f,
    50.0f,
    50.0f,
    50.0f,
    50.0f,
    70.0f,
    50.0f,
    70.0f,
    110.0f,
    50.0f,
    50.0f,
    50.0f,
    50.0f,
    50.0f,
};
s32 characterSoundEffects[6][2] = {
    { 0x3A, 0x3B },
    { 0x36, 0x38 },
    { 0x40, 0x42 },
    { 0x4B, 0x4D },
    { 0x46, 0x48 },
    { 0x3E, 0x3F },
};
s32 npcCharacterModelFiles[0x11] = {
    DATA_MAKE_NUM(DATADIR_M433, 97),
    DATA_MAKE_NUM(DATADIR_M433, 98),
    DATA_MAKE_NUM(DATADIR_M433, 99),
    DATA_MAKE_NUM(DATADIR_M433, 100),
    DATA_MAKE_NUM(DATADIR_M433, 101),
    DATA_MAKE_NUM(DATADIR_M433, 102),
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
u32 characterMotionOffsets[0x11] = {
    0,
    2,
    3,
    DATA_MAKE_NUM(DATADIR_M433, 0x11),
    DATA_MAKE_NUM(DATADIR_M433, 0x19),
    DATA_MAKE_NUM(DATADIR_M433, 0x21),
    DATA_MAKE_NUM(DATADIR_M433, 0x29),
    DATA_MAKE_NUM(DATADIR_M433, 0x31),
    DATA_MAKE_NUM(DATADIR_M433, 0x39),
    DATA_MAKE_NUM(DATADIR_M433, 0x41),
    DATA_MAKE_NUM(DATADIR_M433, 0x49),
    DATA_MAKE_NUM(DATADIR_M433, 0x51),
    DATA_MAKE_NUM(DATADIR_M433, 0x59),
    0x17,
    0x18,
    0x48,
    0x49,
};
s32 npcMotionOffsets[0x11] = {
    DATA_MAKE_NUM(DATADIR_M433, 0x67),
    DATA_MAKE_NUM(DATADIR_M433, 0x6D),
    DATA_MAKE_NUM(DATADIR_M433, 0x73),
    DATA_MAKE_NUM(DATADIR_M433, 0x79),
    DATA_MAKE_NUM(DATADIR_M433, 0x7F),
    DATA_MAKE_NUM(DATADIR_M433, 0x85),
    DATA_MAKE_NUM(DATADIR_M433, 0x8B),
    DATA_MAKE_NUM(DATADIR_M433, 0x91),
    DATA_MAKE_NUM(DATADIR_M433, 0x97),
    DATA_MAKE_NUM(DATADIR_M433, 0x9D),
    DATA_MAKE_NUM(DATADIR_M433, 0xA3),
    DATA_MAKE_NUM(DATADIR_M433, 0xA9),
    DATA_MAKE_NUM(DATADIR_M433, 0xAF),
    DATA_MAKE_NUM(DATADIR_M433, 0xB5),
    DATA_MAKE_NUM(DATADIR_M433, 0xC1),
    DATA_MAKE_NUM(DATADIR_M433, 0xBB),
    DATA_MAKE_NUM(DATADIR_M433, 0xC7),
};
M433PlayerMotionTransition playerMotionTransitions[0x15] = {
    { 0, 0.2f, 0.0f, -1.0f, 0x40000001 },
    { 1, 0.2f, 0.0f, -1.0f, 0x40000001 },
    { 2, 0.2f, 0.0f, -1.0f, 0x40000001 },
    { 3, 0.15f, 0.0f, 0.0f, 0 },
    { 3, 0.02f, 0.0f, -1.0f, 0 },
    { 4, 0.02f, 0.2f, -1.0f, 0 },
    { 5, 0.05f, 0.25f, -1.0f, 0 },
    { 6, 0.1f, 0.0f, -1.0f, 0 },
    { 7, 0.02f, 0.0f, -1.0f, 0 },
    { 0xB, 0.02f, 0.4f, -1.0f, 0 },
    { 0xC, 0.02f, 0.36666667f, -1.0f, 0 },
    { 8, 0.01f, 0.2f, -1.0f, 0 },
    { 9, 0.02f, 0.0f, -1.0f, 0 },
    { 0xA, 0.02f, 0.0f, -1.0f, 0 },
    { 5, 0.08f, 0.0f, 0.25f, 0 },
    { 0xB, 0.2f, 0.0f, 0.4f, 0 },
    { 0xC, 0.08f, 0.0f, 0.3f, 0 },
    { 0xD, 0.2f, 0.0f, -1.0f, 0 },
    { 0xE, 0.2f, 0.0f, -1.0f, 0 },
    { 0xF, 0.2f, 0.0f, -1.0f, 0 },
    { 0x10, 0.2f, 0.0f, -1.0f, 0 },
};
u32 characterAiPriorities[0xE] = {
    0x32,
    0x32,
    0x3C,
    0x32,
    0x32,
    0x5A,
    0x32,
    0x3C,
    0x64,
    0x1E,
    0x32,
    0x1E,
    0x28,
    0x32,
};

// M433PlayerState
OMOBJ *playerObjects[5];
// M433ItemManagerState
OMOBJ *itemManagerObject;
// M433GameEventState
OMOBJ *gameEventObject;
u32 playerOrderBySide[4];

void InitializePlayerObjects(HUPROCESS *process)
{
    u32 characterModels[4];
    s32 initialPositions[4];

    s16 playerIndex;
    s16 opponentIndex;
    u32 playerObject;
    OMOBJ *characterId;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        characterModels[playerIndex] = GWPlayerCfg[playerIndex].group;
        initialPositions[playerIndex] = playerIndex;
    }
    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        for (opponentIndex = playerIndex + 1; opponentIndex < 4; opponentIndex++) {
            if (characterModels[playerIndex] > characterModels[opponentIndex]) {
                playerObject = characterModels[playerIndex];
                characterModels[playerIndex] = characterModels[opponentIndex];
                characterModels[opponentIndex] = playerObject;
                playerObject = initialPositions[playerIndex];
                initialPositions[playerIndex] = initialPositions[opponentIndex];
                initialPositions[opponentIndex] = playerObject;
            }
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerOrderBySide[initialPositions[playerIndex]] = playerIndex;
    }
    gameEventObject = omAddObjEx(process, 0x14, 0, 0, 0, InitializeGameStateObject);
    itemManagerObject = omAddObjEx(process, 0x15, 0x1F, 0, 0, UpdatePlayerManager);
    omMakeGroupEx(process, 0, 4);
    omGetGroupMemberListEx(process, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        characterId = playerObjects[playerIndex] = omAddObjEx(process, 0x28, 2, 0x11, 0, InitializePlayerObject);
        characterId->work[0] = playerIndex;
    }
}

void CleanupPlayerObjects(void)
{
    CharModelKill(-1);
}

void InitializePlayerObject(OMOBJ *object)
{
    s32 unusedPlayerStateValue;
    M433PlayerState *work;
    s16 motionIndex;
    u32 characterId;
    s32 playerIndex;

    float aiDifficultyByLevel[4] = { 0.25f, 0.5f, 0.75f, 1.0f };
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M433PlayerState), HU_MEMNUM_OVL);
    work = object->data;
    memset(work, 0, sizeof(M433PlayerState));
    playerIndex = object->work[0];
    characterId = GWPlayerCfg[playerIndex].character;
    work->characterId = characterId;
    work->npcModelIndex = -1;
    if (characterId >= 8) {
        work->npcModelIndex = characterId - 8;
    }
    unusedPlayerStateValue = work->unusedPlayerState08 = 0;
    work->teamIndex = (playerOrderBySide[object->work[0]] >> 1) & 1;
    work->playerSide = playerOrderBySide[object->work[0]] & 1;
    for (motionIndex = 0; motionIndex < 4; motionIndex++) {
        if ((motionIndex != object->work[0]) && (work->teamIndex == ((playerOrderBySide[motionIndex] >> 1) & 1))) {
            break;
        }
    }
    work->opponentObject = playerObjects[motionIndex];
    work->aiDifficulty = aiDifficultyByLevel[GWPlayerCfg[playerIndex].diff & 3];
    work->isCpu = GWPlayerCfg[object->work[0]].iscom;
    if (work->npcModelIndex < 0) {
        object->mdlId[0] = CharModelCreate(characterId, 4);
        CharModelStepFxSet(characterId, 2);
    }
    else {
        object->mdlId[0] = Hu3DModelCreateFile(npcCharacterModelFiles[work->npcModelIndex]);
    }
    Hu3DModelShadowSet(object->mdlId[0]);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 8));
    Hu3DModelLayerSet(object->mdlId[1], 1);
    for (motionIndex = 0; motionIndex < 0x11; motionIndex++) {
        if (work->npcModelIndex < 0) {
            if (characterMotionOffsets[motionIndex] < 0x10000) {
                object->mtnId[motionIndex] = CharMotionCreate(characterId, characterMotionOffsets[motionIndex]);
            }
            else {
                object->mtnId[motionIndex] = Hu3DJointMotionFile(object->mdlId[0], characterId + characterMotionOffsets[motionIndex]);
            }
        }
        else {
            object->mtnId[motionIndex] = Hu3DJointMotionFile(object->mdlId[0], work->npcModelIndex + npcMotionOffsets[motionIndex]);
        }
    }
    if (work->npcModelIndex < 0) {
        CharMotionVoiceOnSet(characterId, object->mtnId[16], 0);
        CharMotionDataClose(characterId);
    }
    else {
        CharNpcDustVoiceOffSet(object->mdlId[0], object->mtnId[1], 0);
        CharNpcDustVoiceOffSet(object->mdlId[0], object->mtnId[2], 1);
    }
    object->trans.x = work->position.x = (0.4f * (700.0f * (object->work[0] + 1))) - 700.0f;
    object->trans.y = work->position.y = 0.0f;
    object->trans.z = work->position.z = 0.0f;
    work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
    work->currentRotation = work->targetRotation = 0.0f;
    work->rotationStep = 0.0f;
    work->rotationAcceleration = 0.02f;
    work->characterSkill = characterSkillValues[characterId];
    work->actionCounter = -1;
    work->previousGamePhase = -1;
    SetPlayerAnimation(object, 0);
    work->playerActionState = 0;
    object->objFunc = InitializePlayerAnimation;
}

void InitializePlayerAnimation(OMOBJ *object)
{
    M433PlayerState *work = object->data;
    UpdatePlayerAnimation(object);
    work->playerActionState = 0;
    object->objFunc = UpdatePlayerObject;
}

void UpdatePlayerObject(OMOBJ *object)
{
    Vec opponentPosition;
    float stickX;
    float stickY;

    M433PlayerState *work = object->data;
    s32 shouldEndSelection = 0;
    M433PlayerState *opponent = work->opponentObject->data;
    work->pressedButtons = 0;
    work->heldButtons = 0;
    work->stickAngle = 0.0f;
    work->stickMagnitude = 0.0f;
    if ((work->isPlaying == 0) && (work->previousGamePhase != GetPlayerGamePhase(work->teamIndex))) {
        work->previousGamePhase = GetPlayerGamePhase(work->teamIndex);
        switch (GetPlayerGamePhase(work->teamIndex)) {
            case 0:
                work->defenseActive = 0;
                work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                work->playerActionState = 0;
                work->actionSubstate = 0;
                work->actionFlags = work->actionCooldownActive = 0;
                work->position.y = 0.0f;
                if (work->playerSide == GetPlayerServeCell(work->teamIndex)) {
                    if (IsPlayerServing(work->teamIndex) != 0) {
                        work->position.z = 0.0f;
                        work->position.x = 840.00006f;
                        work->playerActionState = 1;
                        work->actionSubstate = 1;
                    }
                    else {
                        work->position.z = 140.0f;
                        work->position.x = 540.2f;
                    }
                }
                else {
                    work->position.z = -140.0f;
                    work->position.x = 385.1f;
                }
                work->currentRotation = work->targetRotation = -90.0f;
                work->rotationStep = 1.0f;
                work->rotationAcceleration = 1.0f;
                if (work->teamIndex == 0) {
                    work->position.x *= -1.0f;
                    work->currentRotation = work->targetRotation = 90.0f;
                }
                work->actionCounter = -1;
                work->hasActiveAction = 1;
                work->framesInAction = work->actionFrame = 0;
                work->stateFlags7C = work->stateFlags78 = work->stateFlags80 = 0;
                break;
            case 1:
            case 2:
                if (work->playerActionState == 1) {
                    work->hasActiveAction = 0;
                };
                break;
            case 3:
                work->playerActionState = 2;
                work->stateFlags7C = work->stateFlags78 = work->stateFlags80 = 0;
                work->hasActiveAction = 0;
                break;
            case 4:
            case 5:
                work->playerActionState = 3;
                break;
            case 6:
            case 7:
                if (IsPlayerContacting(work->teamIndex) != 0) {
                    SetPlayerAnimation(object, 0x13);
                    if (work->npcModelIndex >= 0) {
                        HuAudFXPlay(characterSoundEffects[work->npcModelIndex][0]);
                    }
                    VECSubtract(&opponent->position, &work->position, &opponentPosition);
                    work->targetRotation = atan2d(opponentPosition.x, opponentPosition.z);
                    work->rotationStep = 0.15f;
                    work->rotationAcceleration = 0.0f;
                }
                else {
                    SetPlayerAnimation(object, 0x14);
                }
                work->playerActionState = 4;
                break;
            default:
                break;
        }
    }
    if (getRoundIntroState() == 2) {
        if (work->isCpu != 0) {
            UpdatePlayerMovement(object);
        }
        else {
            work->heldButtons = HuPadBtn[GWPlayerCfg[object->work[0]].pad_idx];
            work->pressedButtons = HuPadBtnDown[GWPlayerCfg[object->work[0]].pad_idx];
            stickX = HuPadStkX[GWPlayerCfg[object->work[0]].pad_idx];
            stickY = -HuPadStkY[GWPlayerCfg[object->work[0]].pad_idx];
            work->stickAngle = atan2d(stickX, stickY);
            work->stickMagnitude = 0.013888889f * sqrtf((stickX * stickX) + (stickY * stickY));
            if (work->stickMagnitude > 1.0f) {
                work->stickMagnitude = 1.0f;
            }
        }
    }
    switch (work->playerActionState) {
        case 0:
            SetPlayerAnimation(object, 0);
            shouldEndSelection = 1;
            break;
        case 1:
            UpdatePlayerActionState(object);
            break;
        case 2:
            UpdatePlayerAi(object);
            UpdatePlayerResponse(object);
            UpdatePlayerModel(object);
            break;
        case 3:
            UpdatePlayerAi(object);
            UpdatePlayerContact(object);
            UpdatePlayerModel(object);
            break;
        case 4:
            work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
            work->isPlaying = 0;
            if (GetPlayerState(object) != 0) {
                work->isPlaying = 0;
                work->playerActionState = 0;
            }
            break;
    }
    VECAdd(&work->position, &work->velocity, &work->position);
    if (GetPlayerGamePhase(work->teamIndex) >= 2) {
        UpdatePlayerState(object);
    }
    UpdatePlayerAnimation(object);
    if ((getRoundIntroState() == 3) && (shouldEndSelection != 0)) {
        work->playerActionState = 0;
        object->objFunc = UpdatePlayerAction;
    }
}

void UpdatePlayerAction(OMOBJ *object)
{
    Vec cameraTarget;
    Vec movementDelta;
    float candidateDistances[2];
    float distanceToTarget;
    M433PlayerState *candidatePlayer;
    M433PlayerState *otherPlayer;
    M433PlayerState *opponent;
    Vec *targetPoints;
    s32 targetIndex;
    M433PlayerState *temporaryPlayer;
    s32 shouldEscape;

    M433PlayerState *work = object->data;
    {
        Vec primaryEscapeTargets[2] = { { 130.0f, 0.0f, -50.0f }, { 250.0f, 0.0f, 50.0f } };
        Vec alternateEscapeTargets[2] = { { 150.0f, 0.0f, 100.0f }, { 300.0f, 0.0f, -100.0f } };
        opponent = work->opponentObject->data;
        VECAdd(&work->position, &work->velocity, &work->position);
        UpdatePlayerState(object);
        UpdatePlayerAnimation(object);
        switch (work->playerActionState) {
            case 0:
                SetPlayerAnimation(object, 0);
                if (isOutroSequenceFinished() != 0) {
                    if (work->playerSide == 0) {
                        shouldEscape = 1;
                        if (work->teamIndex == 0) {
                            primaryEscapeTargets[0].x *= -1.0f;
                            primaryEscapeTargets[1].x *= -1.0f;
                            alternateEscapeTargets[0].x *= -1.0f;
                            alternateEscapeTargets[1].x *= -1.0f;
                        }
                        candidatePlayer = work;
                        otherPlayer = opponent;
                        targetPoints = primaryEscapeTargets;
                        work->escapeState = opponent->escapeState = 0;
                        if (characterAiPriorities[candidatePlayer->characterId] != characterAiPriorities[otherPlayer->characterId]) {
                            if (characterAiPriorities[candidatePlayer->characterId] < characterAiPriorities[otherPlayer->characterId]) {
                                temporaryPlayer = candidatePlayer;
                                candidatePlayer = otherPlayer;
                                otherPlayer = temporaryPlayer;
                            }
                            distanceToTarget = FindClosestPointOnSegments(&candidatePlayer->position, &primaryEscapeTargets[0], &otherPlayer->position, &primaryEscapeTargets[1]);
                            candidateDistances[0] = 0.8f * (candidatePlayer->characterSkill + otherPlayer->characterSkill);
                            if (distanceToTarget < candidateDistances[0]) {
                                targetPoints = alternateEscapeTargets;
                                work->escapeState = opponent->escapeState = 1;
                                OSReport("Escape!\n");
                            }
                            else {
                                shouldEscape = 0;
                                OSReport("Direct!\n");
                            }
                        }
                        if (shouldEscape != 0) {
                            candidatePlayer = work;
                            otherPlayer = opponent;
                            for (targetIndex = 0; targetIndex < 2; targetIndex++) {
                                candidateDistances[targetIndex] = FindClosestPointOnSegments(&candidatePlayer->position, &targetPoints[targetIndex & 1], &otherPlayer->position, &targetPoints[(targetIndex + 1) & 1]);
                            }
                            if (candidateDistances[0] < candidateDistances[1]) {
                                temporaryPlayer = candidatePlayer;
                                candidatePlayer = otherPlayer;
                                otherPlayer = temporaryPlayer;
                            }
                        }
                        candidatePlayer->movementTarget = targetPoints[0];
                        candidatePlayer->movementTarget.z -= candidatePlayer->characterSkill;
                        otherPlayer->movementTarget = targetPoints[1];
                    }
                    work->isPlaying = work->hasContactedBall = work->hasActiveAction = 0;
                    if (work->escapeState != 0) {
                        work->playerActionState++;
                    }
                    else {
                        work->playerActionState += 2;
                    }
                    work->framesInAction = 0;
                    break;
                }
                break;
            case 1:
                if (work->escapeState == 1) {
                    VECSubtract(&work->movementTarget, &work->position, &movementDelta);
                    movementDelta.y = 0.0f;
                    distanceToTarget = VECMag(&movementDelta);
                    if (distanceToTarget < 20.0f) {
                        distanceToTarget = 0.01f;
                        work->escapeState++;
                    }
                    else {
                        if (VECMag(&movementDelta) >= 100.0f) {
                            distanceToTarget = 100.0f;
                        }
                        distanceToTarget = 0.2f + (0.008f * distanceToTarget);
                    }
                    work->stickAngle = 180.0 * (atan2(movementDelta.x, movementDelta.z) / 3.141592653589793);
                    work->stickMagnitude = distanceToTarget;
                    UpdatePlayerAi(object);
                }
                else if (work->escapeState == 2) {
                    CheckPlayerAction(object, 0);
                    if ((work->playerSide == 0) && (opponent->escapeState != 0)) {
                        if (work->teamIndex == 0) {
                            primaryEscapeTargets[0].x *= -1.0f;
                            primaryEscapeTargets[1].x *= -1.0f;
                        }
                        candidatePlayer = work;
                        otherPlayer = opponent;
                        if (characterAiPriorities[candidatePlayer->characterId] < characterAiPriorities[otherPlayer->characterId]) {
                            temporaryPlayer = candidatePlayer;
                            candidatePlayer = otherPlayer;
                            otherPlayer = temporaryPlayer;
                        }
                        candidatePlayer->movementTarget = primaryEscapeTargets[0];
                        candidatePlayer->movementTarget.z -= candidatePlayer->characterSkill;
                        otherPlayer->movementTarget = primaryEscapeTargets[1];
                        work->escapeState = opponent->escapeState = 3;
                    }
                }
                if (work->escapeState == 3) {
                    work->playerActionState++;
                    work->framesInAction = 0;
                    return;
                }
                break;
            case 2:
                VECSubtract(&work->movementTarget, &work->position, &movementDelta);
                movementDelta.y = 0.0f;
                distanceToTarget = VECMag(&movementDelta);
                if (distanceToTarget < 10.0f) {
                    distanceToTarget = 0.01f;
                }
                else {
                    if (VECMag(&movementDelta) >= 100.0f) {
                        distanceToTarget = 100.0f;
                    }
                    distanceToTarget = 0.2f + (0.008f * distanceToTarget);
                }
                work->stickAngle = atan2d(movementDelta.x, movementDelta.z);
                work->stickMagnitude = distanceToTarget;
                UpdatePlayerAi(object);
                if ((work->stickMagnitude < 0.02f) || (work->framesInAction > (REFRESH_RATE*5.0f))) {
                    work->targetRotation = 0.0f;
                    work->rotationStep = 0.15f;
                    work->rotationAcceleration = 0.0f;
                    work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                    SetPlayerResultFlag(object->work[0]);
                    work->playerActionState++;
                    return;
                }
                break;
            case 3:
                if (getResultsSequenceState() == 1) {
                    if (IsPlayerWinner(work->teamIndex) != 0) {
                        SetPlayerAnimation(object, 0x11);
                        if (work->npcModelIndex >= 0) {
                            HuAudFXPlay(characterSoundEffects[work->npcModelIndex][0]);
                        }
                    }
                    else {
                        SetPlayerAnimation(object, 0x12);
                        if (work->npcModelIndex >= 0) {
                            HuAudFXPlay(characterSoundEffects[work->npcModelIndex][1]);
                        }
                    }
                    work->playerActionState++;
                    return;
                }
                CheckPlayerAction(object, 0);
                return;
            case 4:
                if (IsPlayerWinner(work->teamIndex) != 0) {
                    cameraTarget.x = Center.x + (CZoom * (sind(CRot.y) * cosd(CRot.x)));
                    cameraTarget.y = Center.y + (CZoom * -sind(CRot.x));
                    cameraTarget.z = Center.z + (CZoom * (cosd(CRot.y) * cosd(CRot.x)));
                    VECSubtract(&cameraTarget, &work->position, &movementDelta);
                    work->targetRotation = atan2d(movementDelta.x, movementDelta.z);
                }
                if (GetPlayerState(object) != 0) {
                    SetTeamResultFlag(object->work[0]);
                    work->playerActionState++;
                }
                break;
        }
    }
}

char *itemHookJointNames[6] = {
    "itemhook_R",
    "itemhook_r",
    "itemhook_R",
    "itemhook_R",
    "itemhook_r",
    "itemhook_r",
};
s32 itemSideBySideFlags[9] = { 0, 0, 1, 1, 1, 1, 1, 0, 1 };
float itemSpinSpeeds[0x12] = {
    0.0f,
    (40.0f*(1.0f/REFRESH_RATE)),
    (130.0f*(1.0f/REFRESH_RATE)),
    (120.0f*(1.0f/REFRESH_RATE)),
    (60.0f*(1.0f/REFRESH_RATE)),
    (420.0f*(1.0f/REFRESH_RATE)),
    (90.0f*(1.0f/REFRESH_RATE)),
    (90.0f*(1.0f/REFRESH_RATE)),
    (180.0f*(1.0f/REFRESH_RATE)),
    0.0f,
    0.0f,
    1e-45f,
    0.0f,
    0.0f,
    1e-45f,
    0.0f,
    0.0f,
    1e-45f,
};
Vec itemTrajectoryParameters[9] = {
    { 0.0f, 0.0f, 1.0f },
    { 0.0f, 1.5f, 1.0f },
    { 236.5f, 0.0f, 1300.0f },
    { 0.0f, 1.5f, 1.0f },
    { 0.0f, 1.8f, 1.0f },
    { 231.0f, 0.0f, 1500.0f },
    { 231.0f, 0.0f, 1000.0f },
    { 0.0f, 1.2f, 1.0f },
    { 0.0f, 0.8f, 1.0f },
};
float itemTrajectoryScales[9] = { 1.0f, 1.0f, 3.0f, 1.0f, 1.0f, 4.0f, 2.0f, 1.0f, 1.0f };

void UpdatePlayerAnimation(OMOBJ *object)
{
    Vec screenPosition;

    M433PlayerState *work = object->data;
    work->playerState9C++;
    work->framesInAction++;
    object->trans.x = work->position.x;
    object->trans.y = work->position.y;
    object->trans.z = work->position.z;
    work->rotationStep = work->rotationStep + work->rotationAcceleration;
    if (work->rotationStep > 0.45f) {
        work->rotationStep = 0.45f;
    }
    work->currentRotation = interpolateAngle(work->currentRotation, work->targetRotation, work->rotationStep);
    object->rot.y = work->currentRotation;
    Hu3DModelPosSet(object->mdlId[1], work->position.x, 11.0f, work->position.z);
    work->modelRotation += (1.0f/REFRESH_RATE)*180.0f;
    if (work->modelRotation >= 360.0f) {
        work->modelRotation -= 360.0f;
    }
    Hu3DModelRotSet(object->mdlId[1], 0.0f, work->modelRotation, 0.0f);
    work->modelScaleTarget = 0.0f;
    if (getRoundIntroState() == 2) {
        if (((GetActiveContactGroup() == 4) || (GetActiveContactGroup() == 3)) && (work->teamIndex == GetItemTargetPosition(&screenPosition)) && (GetPlayerPointGroup() != 0) && (GetPlayerPointGroup() < 3)
            && (work->hasContactedBall == 0) && (IsPlayerContact(work->teamIndex, work->playerSide) != 0)) {
            work->modelScaleTarget = 1.0f;
        }
        if (work->playerActionState == 1) {
            work->modelScaleTarget = 1.0f;
        }
    }
    work->modelScale += 0.1f * (work->modelScaleTarget - work->modelScale);
    Hu3DModelScaleSet(object->mdlId[1], work->modelScale, 1.0f, work->modelScale);
    if ((work->modelScaleTarget <= 0.0f) && (work->modelScale < 0.1f)) {
        Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
        work->modelScale = 0.0f;
        return;
    }
    Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
}

void UpdatePlayerMovement(OMOBJ *object)
{
    Vec ballPosition;
    Vec movementDelta;
    M433PlayerState *opponent;
    float distanceToTarget;

    M433PlayerState *work = object->data;
    s32 shouldContinueAi = 1;
    s32 inputState = 0;
    opponent = work->opponentObject->data;
    switch (work->playerActionState) {
        case 0:
            work->defenseActive = 0;
            UpdatePlayerScoring(object);
            break;
        case 1:
            if (work->actionFlags != 0) {
                work->pressedButtons = work->pressedButtons | 0x100;
                work->aiTargetAngle = frandmod(0x168);
                work->aiTargetSpeed = 0.3f + (0.00040000002f * (work->aiDifficulty * frandmod(0x3E8)));
                work->aiTargetFlag = 0;
                if (frandmod(0x3E8) > (300.0f + (500.0f * work->aiDifficulty))) {
                    work->aiTargetFlag = 1;
                }
            }
            if (NormalizeItemPosition(&ballPosition) != 0) {
                if (work->aiTargetFlag != 0) {
                    if ((fabs(ballPosition.z) > 1.100000023841858) || (ballPosition.x > 1.1f)) {
                        work->aiTargetSpeed = 0.0f;
                    }
                }
                else {
                    if ((fabs(ballPosition.z) > 0.949999988079071) || (ballPosition.x > 0.95f)) {
                        work->aiTargetSpeed = 0.0f;
                    }
                }
            }
            work->stickAngle = work->aiTargetAngle;
            work->stickMagnitude = work->aiTargetSpeed;
            if (work->actionCooldownActive != 0) {
                if (work->aiReactionTimer++ >= work->aiReactionDelay) {
                    work->pressedButtons = work->pressedButtons | 0x100;
                }
            }
            else if (work->hasContactedBall != 0) {
                work->aiReactionDelay = ((REFRESH_RATE * (0.35f * work->aiDifficulty)) / 1000.0f) * frandmod(0x3E8);
                work->aiReactionTimer = 0;
            }
            UpdatePlayerScoring(object);
            break;
        case 2:
            work->aiActionMode = 0;
            if (frandmod(0x3E8) < (300.0f * work->aiDifficulty)) {
                work->aiActionMode = 1;
            }
            UpdatePlayerCollision(object);
            if (work->aiDecisionState != 0) {
                if (GetActiveContactGroup() == 2) {
                    work->aiDecisionState = 0;
                    work->aiDecisionFlags = 0;
                }
                if (work->teamIndex == GetItemPosition(&ballPosition)) {
                    work->aiActionCount++;
                }
                if ((work->teamIndex == GetItemPosition(&ballPosition)) && (work->aiDecisionFlags == 0)) {
                    UpdateItemFlightPosition(0.9f, &work->aiTargetPosition);
                    work->aiTargetPosition.x = 115.0f;
                    if (work->teamIndex == 0) {
                        work->aiTargetPosition.x *= -1.0f;
                    }
                    work->aiDecisionFlags++;
                }
                if (work->aiDecisionFlags == 1) {
                    if ((work->aiActionCount >= (0.5f * work->aiActionThreshold)) && (GetItemSoundPosition(&work->aiTargetPosition) != 0)) {
                        work->aiTargetPosition.x = 115.0f;
                        if (work->teamIndex == 0) {
                            work->aiTargetPosition.x *= -1.0f;
                        }
                        work->aiDecisionFlags++;
                    }
                }
                else if ((work->aiDecisionFlags == 2) && ((work->aiActionCount >= work->aiActionThreshold) || (work->teamIndex == GetItemTargetPosition(&ballPosition)))) {
                    work->pressedButtons = work->pressedButtons | 0x200;
                    work->aiDecisionState = 0;
                    work->aiDecisionFlags = 0;
                }
            }
            if ((work->teamIndex == GetItemPosition(&ballPosition)) && (GetItemAction() >= (REFRESH_RATE * (0.8f - (0.4f * work->aiDifficulty)))) && (work->defenseActive != 0)) {
                VECSubtract(&ballPosition, &work->position, &movementDelta);
                movementDelta.y = 0.0f;
                distanceToTarget = VECMag(&movementDelta);
                if (VECMag(&movementDelta) >= 20.0f) {
                    if (distanceToTarget >= 50.0f) {
                        distanceToTarget = 50.0f;
                    }
                    work->stickAngle = atan2d(movementDelta.x, movementDelta.z);
                    work->stickMagnitude = distanceToTarget / 50.0f;
                }
                if ((GetItemVerticalSpeed() < (0.8f - (0.6f * work->aiDifficulty))) && ((work->opponentTargetingState != 0) || ((work->opponentRecentState != 0) && (work->aiPreviousTargetMode != 0)))) {
                    work->pressedButtons = work->pressedButtons | 0x100;
                }
                shouldContinueAi = 0;
            }
            if (work->aiDecisionFlags != 0) {
                VECSubtract(&work->aiTargetPosition, &work->position, &movementDelta);
                movementDelta.y = 0.0f;
                distanceToTarget = VECMag(&movementDelta);
                if (distanceToTarget >= 10.0f) {
                    if (distanceToTarget >= 50.0f) {
                        distanceToTarget = 50.0f;
                    }
                    work->stickAngle = atan2d(movementDelta.x, movementDelta.z);
                    work->stickMagnitude = distanceToTarget / 50.0f;
                }
                else {
                    work->stickAngle = 90.0f;
                    if (work->teamIndex != 0) {
                        work->stickAngle *= -1.0f;
                    }
                    work->stickMagnitude = 0.01f;
                }
                shouldContinueAi = 0;
            }
            if (shouldContinueAi != 0) {
                VECSubtract(&work->retreatPosition, &work->position, &movementDelta);
                movementDelta.y = 0.0f;
                distanceToTarget = VECMag(&movementDelta);
                distanceToTarget -= 100.0f * (1.25f * (2.0f - work->aiDifficulty));
                if (distanceToTarget >= 10.0f) {
                    if (distanceToTarget >= 70.0f) {
                        distanceToTarget = 70.0f;
                    }
                    work->stickAngle = atan2d(movementDelta.x, movementDelta.z);
                    work->stickMagnitude = distanceToTarget / 70.0f;
                    break;
                }
                work->stickAngle = 90.0f;
                if (work->teamIndex != 0) {
                    work->stickAngle *= -1.0f;
                    break;
                }
            }
            break;
        case 3:
            UpdatePlayerCollision(object);
            UpdatePlayerScoring(object);
            if (work->teamIndex == GetItemTargetPosition(&ballPosition)) {
                if (work->attackReady == 0) {
                    if (IsPlayerContact(work->teamIndex, work->playerSide) != 0) {
                        VECSubtract(&ballPosition, &work->position, &movementDelta);
                        movementDelta.y = 0.0f;
                        distanceToTarget = VECMag(&movementDelta);
                        if (distanceToTarget >= 30.000002f) {
                            if (distanceToTarget >= 70.0f) {
                                distanceToTarget = 70.0f;
                            }
                            work->stickMagnitude = distanceToTarget / 70.0f;
                            work->stickAngle = atan2d(movementDelta.x, movementDelta.z);
                        }
                    }
                    if ((((work->defenseActive == 0) && (GetActiveContactGroup() == 3) && (work->aiActionMode != 0)) || ((work->defenseActive != 0) && (GetActiveContactGroup() == 4)))
                        && (work->canContactBall != 0)) {
                        work->pressedButtons = work->pressedButtons | 0x200;
                        work->aiTargetMode = 0;
                        if (frandmod(0x3E8) < (s32)(300.0f * (work->aiDifficulty - 0.4f))) {
                            work->aiTargetMode = GetPlayerTargetPosition(work->teamIndex, &work->ballTargetPosition);
                            work->aiTargetAngle = 0.0f;
                            work->aiTargetSpeed = 0.0f;
                            work->aiTargetFlag = 0;
                        }
                        if (work->aiTargetMode == 0) {
                            work->aiTargetAngle = frandmod(0x168);
                            work->aiTargetSpeed = 0.3f + (0.00040000002f * (work->aiDifficulty * frandmod(0x3E8)));
                            work->aiTargetFlag = 0;
                            if (frandmod(0x3E8) > (300.0f + (500.0f * work->aiDifficulty))) {
                                work->aiTargetFlag = 1;
                            }
                        }
                    }
                    if ((work->isHighBallContact != 0) || (work->opponentTargetingState != 0) || (work->opponentRecentState != 0)) {
                        work->pressedButtons = work->pressedButtons | 0x100;
                        break;
                    }
                }
                else {
                    if (NormalizeItemPosition(&ballPosition) != 0) {
                        if (work->aiTargetMode != 0) {
                            VECSubtract(&work->ballTargetPosition, &ballPosition, &movementDelta);
                            movementDelta.y = 0.0f;
                            distanceToTarget = VECMag(&movementDelta);
                            if (distanceToTarget < 0.05f) {
                                work->aiTargetAngle = 0.0f;
                                work->aiTargetSpeed = 0.0f;
                            }
                            else {
                                if (work->teamIndex != 0) {
                                    movementDelta.x *= -1.0f;
                                }
                                work->aiTargetAngle = atan2d(movementDelta.x, movementDelta.z);
                                work->aiTargetSpeed = 1.0f;
                            }
                        }
                        if (work->aiTargetFlag != 0) {
                            if ((fabs(ballPosition.z) > 1.100000023841858) || (ballPosition.x > 1.1f)) {
                                work->aiTargetSpeed = 0.0f;
                            }
                        }
                        else {
                            if ((fabs(ballPosition.z) > 0.949999988079071) || (ballPosition.x > 0.95f)) {
                                work->aiTargetSpeed = 0.0f;
                            }
                        }
                    }
                    work->stickAngle = work->aiTargetAngle;
                    work->stickMagnitude = work->aiTargetSpeed;
                    if (work->movementLockActive != 0) {
                        if (work->aiReactionTimer++ >= work->aiReactionDelay) {
                            work->pressedButtons |= 0x100;
                            break;
                        }
                    }
                    else {
                        work->aiReactionDelay = ((REFRESH_RATE * (0.35f * work->aiDifficulty)) / 1000.0f) * frandmod(0x3E8);
                        work->aiReactionTimer = 0;
                    }
                }
            }
            break;
    }
}

void UpdatePlayerCollision(OMOBJ *object)
{
    Vec ballPosition;
    Vec playerDelta;
    float playerDistance;
    float opponentDistance;

    M433PlayerState *work = object->data;
    M433PlayerState *opponent = work->opponentObject->data;

    if (opponent->isCpu == 0) {
        opponent->aiDifficulty = work->aiDifficulty - 0.1f;
    }
    if ((work->teamIndex == GetItemPosition(&ballPosition)) && (GetItemState() >= (REFRESH_RATE/5.0f))) {
        if (((opponent->defenseActive == 0) && (work->defenseActive == 0)) || (GetItemState() < 0x3E8)) {
            if ((work->isPlaying == 0) || (opponent->isPlaying == 0)) {
                VECSubtract(&ballPosition, &work->position, &playerDelta);
                playerDelta.y = 0.0f;
                playerDistance = VECMag(&playerDelta) - (100.0f * (2.0f * work->aiDifficulty));
                VECSubtract(&ballPosition, &opponent->position, &playerDelta);
                playerDelta.y = 0.0f;
                opponentDistance = VECMag(&playerDelta) - (100.0f * (2.0f * opponent->aiDifficulty));
                if ((work->isPlaying != 0) || (work->hasContactedBall != 0)) {
                    playerDistance = 2000.0f;
                }
                if ((opponent->isPlaying != 0) || (opponent->hasContactedBall != 0)) {
                    opponentDistance = 2000.0f;
                }
                if (playerDistance >= 300.0f) {
                    opponent->aiDecisionState = 0;
                }
                if (opponentDistance >= 300.0f) {
                    work->aiDecisionState = 0;
                }
                if (work->aiDecisionState != 0) {
                    playerDistance += 300.0f;
                }
                if (opponent->aiDecisionState != 0) {
                    opponentDistance += 300.0f;
                }
                opponent->defenseActive = work->defenseActive = 0;
                if (playerDistance <= opponentDistance) {
                    work->defenseActive = 1;
                    work->aiDecisionState = 0;
                }
                else {
                    opponent->defenseActive = 1;
                    opponent->aiDecisionState = 0;
                }
                if (work->aiDecisionState == 0) {
                    work->aiDecisionFlags = 0;
                }
                if (opponent->aiDecisionState == 0) {
                    opponent->aiDecisionFlags = 0;
                }
            }
        }
        else if (GetPlayerPointGroup() == 1) {
            work->defenseActive = IsPlayerContact(work->teamIndex, work->playerSide) == 0;
            opponent->defenseActive = work->defenseActive == 0;
        }
    }
    else if (work->teamIndex != GetItemTargetPosition(&ballPosition)) {
        opponent->defenseActive = work->defenseActive = 0;
    }
}

void UpdatePlayerScoring(OMOBJ *object)
{
    M433PlayerState *work = object->data;
    work->aiPreviousTargetMode = 0;
    if (frandmod(0x3E8) < (600.0f * work->aiDifficulty)) {
        work->aiPreviousTargetMode = 1;
    }
    work->aiDecisionState = 0;
    work->aiDecisionFlags = 0;
    if ((work->playerSide != GetPlayerServeCell(work->teamIndex)) && (work->playerActionState != 0) && (frandmod(0x3E8) < (500.0f * work->aiDifficulty))) {
        work->aiDecisionState = 1;
        work->aiActionThreshold = ((REFRESH_RATE * (0.5f * work->aiDifficulty)) / 1000.0f) * frandmod(0x3E8);
        work->aiActionCount = 0;
    }
    if (work->playerSide == GetPlayerServeCell(work->teamIndex)) {
        work->retreatPosition.z = 140.0f;
        work->retreatPosition.x = 540.2f;
    }
    else {
        work->retreatPosition.z = -140.0f;
        work->retreatPosition.x = 300.5f;
    }
    if (work->teamIndex == 0) {
        work->retreatPosition.x *= -1.0f;
    }
}

s32 GetPlayerTargetPosition(u32 teamIndex, Vec *targetPosition)
{
    Vec candidatePositions[2];
    Vec playerDelta;
    Vec targetPosition;
    Vec opponentDelta;
    float targetDistance;
    float opponentDistance;
    float candidateAngle;
    float candidateScore;
    u32 candidateIndex;
    u32 selectedCandidate;

    for (candidateIndex = 0, selectedCandidate = 0; (selectedCandidate < 4) && (candidateIndex < 2); selectedCandidate++) {
        M433PlayerState *work = playerObjects[selectedCandidate]->data;
        if (work->teamIndex != teamIndex) {
            candidatePositions[candidateIndex] = work->position;
            candidatePositions[candidateIndex].y = 0.0f;
            candidateIndex++;
        }
    }
    if (candidateIndex < 2) {
        return 0;
    }
    if (teamIndex == 1) {
        candidatePositions[0].x *= -1.0f;
        candidatePositions[1].x *= -1.0f;
    }
    candidateAngle = 47.0f;
    candidateScore = 70.0f;
    opponentDistance = -1.0f;
    targetPosition.z = -350.0f;
    targetPosition.y = 0.0f;
    for (selectedCandidate = 0; selectedCandidate <= 0xA; selectedCandidate++) {
        targetPosition.x = 230.0f;
        for (candidateIndex = 0; candidateIndex <= 0xA; candidateIndex++) {
            VECSubtract(&targetPosition, &candidatePositions[0], &playerDelta);
            targetDistance = VECMag(&playerDelta);
            VECSubtract(&targetPosition, &candidatePositions[1], &playerDelta);
            targetDistance *= VECMag(&playerDelta);
            if (targetDistance > opponentDistance) {
                opponentDistance = targetDistance;
                opponentDelta = targetPosition;
            }
            targetPosition.x += candidateAngle;
        }
        targetPosition.z += candidateScore;
    }
    *targetPosition = opponentDelta;
    targetPosition->x -= 465.0f;
    targetPosition->x *= 0.004255319f;
    targetPosition->z *= 0.0028571428f;
    return 1;
}

void UpdatePlayerAi(OMOBJ *object)
{
    Vec targetPosition;
    Vec movementDelta;
    float distanceToTarget;
    float movementSpeed;

    M433PlayerState *work = object->data;
    s32 movementState = 1;
    if ((work->isPlaying == 0) && (work->hasContactedBall == 0)) {
        if ((work->stickMagnitude > 0.0f) && (work->hasActiveAction == 0)) {
            work->targetRotation = work->stickAngle;
        }
        if ((work->stickMagnitude > 0.05f) && (work->hasActiveAction == 0)) {
            work->velocity.x = (1.0f/REFRESH_RATE) * (550.0 * (work->stickMagnitude * sind(work->stickAngle)));
            work->velocity.z = (1.0f/REFRESH_RATE) * (550.0 * (work->stickMagnitude * cosd(work->stickAngle)));
            if (work->isCpu != 0) {
                work->rotationStep = 0.1f;
            }
            else {
                work->rotationStep = 0.2f;
            }
            work->rotationAcceleration = 0.0f;
            if (GetPlayerActionState(object) != 0) {
                movementSpeed = 0.5f;
                if (work->actionCounter == 2) {
                    movementSpeed = 0.45f;
                }
                if (work->stickMagnitude > movementSpeed) {
                    SetPlayerAnimation(object, 2);
                    distanceToTarget = work->stickMagnitude;
                }
                else {
                    SetPlayerAnimation(object, 1);
                    distanceToTarget = work->stickMagnitude / movementSpeed;
                }
                if (work->npcModelIndex < 0) {
                    CharMotionSpeedSet(work->characterId, distanceToTarget);
                }
                else {
                    Hu3DMotionSpeedSet(object->mdlId[0], distanceToTarget);
                }
            }
            if ((work->stickMagnitude > 0.5f) && (((work->framesInAction + 0xF) & 0xF) == 0)) {
                ApproachVector(&work->position, 0.2f);
            }
        }
        else {
            work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
            if ((work->canContactBall != 0) && (GetPlayerPointGroup() >= 2)) {
                SetPlayerAnimation(object, 0x10);
            }
            else if (work->isHighBallContact != 0) {
                SetPlayerAnimation(object, 0xF);
            }
            else if (work->opponentTargetingState != 0) {
                SetPlayerAnimation(object, 0xE);
            }
            else {
                CheckPlayerAction(object, 0);
            }
            if ((movementState != 0) && (work->stickMagnitude == 0.0f)) {
                UpdateItemFlightPosition(0.0f, &targetPosition);
                VECSubtract(&targetPosition, &work->position, &movementDelta);
                work->targetRotation = atan2d(movementDelta.x, movementDelta.z);
                work->rotationStep = 0.1f;
                work->rotationAcceleration = 0.0f;
            }
        }
    }
}

void UpdatePlayerActionState(OMOBJ *object)
{
    Vec targetPosition;
    Vec movementDelta;
    float distanceToTarget;
    float movementSpeed;

    M433PlayerState *work = object->data;
    s32 movementState = work->actionCooldownActive;
    {
        s32 actionSoundEffectIds[2] = { 0x6A4, 0x6A5 };
        work->actionFlags = 0;
        work->actionCooldownActive = 0;
        if (work->npcModelIndex < 0) {
            char *itemHookName = CharModelItemHookGet(work->characterId, 4, 0);
            Hu3DModelObjPosGet(object->mdlId[0], itemHookName, &movementDelta);
        }
        else {
            Hu3DModelObjPosGet(object->mdlId[0], itemHookJointNames[work->npcModelIndex], &movementDelta);
        }
        switch (work->actionFrame) {
            case 0:
                work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                work->actionFrame++;
                work->framesInAction = 0;

            case 1:
                work->isPlaying = 0;
                work->actionSubstate = 1;
                SetPlayerAnimation(object, 3);
                SetPlayerItemState(&movementDelta, 0, work->teamIndex, work->playerSide);
                if (GetPlayerGamePhase(work->teamIndex) != 1) {
                    work->framesInAction = 0;
                    break;
                }
                if (work->hasActiveAction == 0) {
                    work->actionFlags = 1;
                    if ((work->pressedButtons & 0x100) || (work->framesInAction > (REFRESH_RATE*5.0f))) {
                        SetPlayerAnimation(object, 4);
                        work->isPlaying = 1;
                        work->actionFrame++;
                        work->framesInAction = 0;
                    }
                }
                break;
            case 2:
                if (work->hasContactedBall == 0) {
                    work->velocity.x = (100.0f*(1.0f/REFRESH_RATE)) * (2.0 * sind((10.0f + (((1800.0f/REFRESH_RATE)/7) * work->framesInAction))));
                    if (work->teamIndex != 0) {
                        work->velocity.x *= -1.0f;
                    }
                }
                if ((work->framesInAction >= (REFRESH_RATE*0.15f)) && (work->actionSubstate != 0)) {
                    if (work->teamIndex != 0) {
                        movementSpeed = -1.0f;
                    }
                    else {
                        movementSpeed = 1.0f;
                    }
                    targetPosition.x = object->trans.x + (250.0f * movementSpeed);
                    targetPosition.y = 80.0f + object->trans.y;
                    targetPosition.z = movementDelta.z;
                    SetPlayerItemState(&targetPosition, 1, work->teamIndex, work->playerSide);
                    work->actionSubstate = 0;
                }
                else if (work->framesInAction >= ((REFRESH_RATE*28.0f)/60.0f)) {
                    distanceToTarget = 0.5f;
                    work->aiMovementSpeed = (2.0f * -(350.0f - characterModelHeights[work->characterId])) / (distanceToTarget * distanceToTarget);
                    work->velocity.y = (1.0f/REFRESH_RATE) * (-work->aiMovementSpeed * distanceToTarget);
                    work->hasContactedBall = 1;
                    work->actionFrame++;
                    work->framesInAction = 0;
                    ApproachVector(&work->position, 0.5f);
                }
                if (work->actionSubstate == 0) {
                    UpdateItemMotion(work->stickAngle, work->stickMagnitude, work->teamIndex, 0, &targetPosition);
                    break;
                }
                SetPlayerItemState(&movementDelta, 0, work->teamIndex, work->playerSide);
                break;
            case 3:
                work->velocity.y += (1.0f/REFRESH_RATE) * ((1.0f/REFRESH_RATE) * work->aiMovementSpeed);
                if ((work->framesInAction > (REFRESH_RATE*0.45f)) && (work->framesInAction < (REFRESH_RATE*0.85f))) {
                    work->actionCooldownActive = 1;
                }
                UpdateItemMotion(work->stickAngle, work->stickMagnitude, work->teamIndex, work->actionCooldownActive, &targetPosition);
                if (((work->actionCooldownActive != 0) && ((work->pressedButtons & 0x100) != 0)) || ((work->actionCooldownActive == 0) && (movementState != 0))) {
                    SetPlayerAnimation(object, 5);
                    if (work->actionCooldownActive == 0) {
                        ResetItemState();
                    }
                    SetPlayerItemState(&targetPosition, 2, work->teamIndex, work->playerSide);
                    work->actionCooldownActive = 0;
                    work->actionFrame = 5;
                    work->framesInAction = 0;
                    HuAudFXPlay(actionSoundEffectIds[work->teamIndex]);
                }
                break;
            case 4:
                if (work->framesInAction >= (REFRESH_RATE/5.0f)) {
                    work->hasContactedBall = 0;
                    work->actionFrame = 1;
                    work->framesInAction = 0;
                }
                break;
            case 5:
                work->velocity.y += (1.0f/REFRESH_RATE) * ((1.0f/REFRESH_RATE) * work->aiMovementSpeed);
                if (work->position.y <= 0.0f) {
                    work->hasContactedBall = 0;
                    ApproachVector(&work->position, 0.5f);
                    work->isPlaying = 0;
                    work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                }
                break;
        }
    }
}

void UpdatePlayerModel(OMOBJ *object)
{
    Vec playerPosition;
    Vec targetPosition;
    float targetDistance;
    float directionX;
    float directionZ;

    M433PlayerState *work = object->data;
    s32 itemTypesByPointGroup[3][3] = { { 3, 4, 6 }, { 3, 4, 6 }, { 3, 4, 7 } };
    s32 secondaryItemSoundEffects[2] = { 0x6AD, 0x6AE };
    s32 itemSoundEffects[2] = { 0x6A6, 0x6A7 };
    s32 contactSoundEffects[2] = { 0x6AA, 0x6AB };
    M433PlayerState *playerState = work->opponentObject->data;

    work->isHighBallContact = 0;
    work->opponentTargetingState = 0;
    work->opponentRecentState = 0;
    if (work->hasContactedBall == 0) {
        switch (work->stateFlags7C) {
            case 0:
                if ((work->stateFlags7C == 0) && (work->stateFlags78 == 0) && (work->stateFlags80 == 0)
                    && (work->isPlaying = 0, work->contactFollowThroughActive = 0, ((work->hasActiveAction == 0) != 0)) && (work->teamIndex == GetItemTargetPosition(&playerPosition))
                    && (IsPlayerContact(work->teamIndex, work->playerSide) != 0) && (GetPlayerPointGroup() < 3)) {
                    work->movementTarget = playerPosition;
                    targetDistance = GetItemVerticalSpeed();
                    if ((targetDistance <= 0.8f) && (targetDistance >= 0.02f) && (playerState->contactFollowThroughActive == 0)) {
                        VECSubtract(&playerPosition, &work->position, &targetPosition);
                        targetPosition.y = 0.0f;
                        directionX = VECMag(&targetPosition);
                        work->lastBallContactTime = directionX;
                        if (directionX <= 200.0f) {
                            if ((targetDistance >= 0.2f) && (directionX <= 70.0f) && (targetDistance >= (0.2f + (0.001f * directionX))) && (GetPlayerPointGroup() >= 1)) {
                                work->isHighBallContact = 1;
                            }
                            if ((targetDistance >= 0.02f) && (directionX <= 100.0f) && (targetDistance >= (0.02f + (0.001f * directionX)))) {
                                work->opponentTargetingState = 1;
                            }
                            if ((targetDistance >= 0.1f) && (targetDistance <= 0.3f) && (directionX > 100.0f) && (directionX <= 200.0f)
                                && (targetDistance >= (0.1f + (0.0002f * (directionX - 100.0f))))) {
                                work->opponentRecentState = 1;
                            }
                        }
                    }
                    if ((work->pressedButtons & 0x100) != 0) {
                        work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                        if (work->isHighBallContact != 0) {
                            work->stateFlags7C = 1;
                            targetDistance -= 0.12f;
                            work->lastBallContactTime *= 0.014285714f;
                            SetPlayerAnimation(object, 0xF);
                            UpdateItemFlightPosition(0.0f, &playerPosition);
                        }
                        else if (work->opponentTargetingState != 0) {
                            work->stateFlags7C = 2;
                            targetDistance -= 0.03f;
                            work->lastBallContactTime *= 0.01f;
                            SetPlayerAnimation(object, 0xE);
                            UpdateItemFlightPosition(0.0f, &playerPosition);
                        }
                        else if (work->opponentRecentState != 0) {
                            work->stateFlags7C = 3;
                            targetDistance -= 0.03f;
                            work->lastBallContactTime *= 0.005f;
                            SetPlayerAnimation(object, 7);
                            ApproachVector(&work->position, 0.75f);
                            HuAudFXPlay(contactSoundEffects[work->teamIndex]);
                        }
                        else {
                            if (GetPlayerPointGroup() >= 1) {
                                SetPlayerAnimation(object, 9);
                            }
                            else {
                                SetPlayerAnimation(object, 6);
                            }
                            work->stateFlags7C = 4;
                        }
                        if (work->stateFlags7C < 4) {
                            work->contactFollowThroughActive = 1;
                            if (targetDistance < 0.0f) {
                                targetDistance = 0.0f;
                            }
                            work->aiActionFlags = REFRESH_RATE * targetDistance;
                        }
                        VECSubtract(&playerPosition, &work->position, &targetPosition);
                        work->targetRotation = atan2d(targetPosition.x, targetPosition.z);
                        work->rotationStep = 0.2f;
                        work->rotationAcceleration = 0.0f;
                        work->isPlaying = 1;
                        work->framesInAction = 0;
                    }
                }
                break;
            case 1:
                if (work->framesInAction >= work->aiActionFlags) {
                    if (work->contactFollowThroughActive != 0) {
                        if (GetPlayerPointGroup() >= 2) {
                            playerPosition.x = 350.0f;
                            playerPosition.y = 11.0f;
                            playerPosition.z = 0.0f;
                            if (work->teamIndex != 0) {
                                playerPosition.x *= -1.0f;
                            }
                        }
                        else {
                            playerPosition = playerState->position;
                            playerPosition.y = 11.0f;
                            playerPosition.x *= 0.5f;
                        }
                        directionZ = frandmod(0x168);
                        directionX = 200.0f * work->lastBallContactTime;
                        playerPosition.x += directionX * sind(directionZ);
                        playerPosition.z += directionX * cosd(directionZ);
                        if (GetPlayerPointGroup() < 2) {
                            ClampPlayerPosition(&playerPosition, work->teamIndex);
                        }
                        SetPlayerItemState(&playerPosition, itemTypesByPointGroup[0][GetPlayerPointGroup()], work->teamIndex, work->playerSide);
                        ApproachVector(&work->position, 0.0f);
                        HuAudFXPlay(itemSoundEffects[work->teamIndex]);
                    }
                    work->contactFollowThroughActive = 0;
                    SetPlayerAnimation(object, 9);
                    VECSubtract(&playerPosition, &work->position, &targetPosition);
                    work->targetRotation = atan2d(targetPosition.x, targetPosition.z);
                    work->rotationStep = 0.1f;
                    work->aiActionFlags = 0x186A0;
                    work->framesInAction = 0;
                    break;
                }
                if (work->aiActionFlags < 0x2710) {
                    work->velocity.x = 0.07f * (work->movementTarget.x - work->position.x);
                    work->velocity.z = 0.07f * (work->movementTarget.z - work->position.z);
                    if (work->contactFollowThroughActive == 0) {
                        work->aiActionFlags = 0;
                    }
                }
                else {
                    work->velocity.x *= 0.7f;
                    work->velocity.z *= 0.7f;
                    if (GetPlayerState(object) != 0) {
                        work->stateFlags7C = 0;
                    }
                }
                break;
            case 2:
                if (work->framesInAction >= work->aiActionFlags) {
                    if (work->contactFollowThroughActive != 0) {
                        switch (GetPlayerPointGroup()) {
                            case 0:
                                playerPosition = playerState->position;
                                playerPosition.y = 11.0f;
                                break;
                            case 1:
                                playerPosition = playerState->position;
                                playerPosition.y = 11.0f;
                                playerPosition.x *= 0.5f;
                                break;
                            case 2:
                            default:
                                playerPosition.x = 350.0f;
                                playerPosition.y = 11.0f;
                                playerPosition.z = 0.0f;
                                if (work->teamIndex != 0) {
                                    playerPosition.x *= -1.0f;
                                }
                                break;
                        }
                        directionZ = frandmod(0x168);
                        directionX = 200.0f * work->lastBallContactTime;
                        playerPosition.x += directionX * sind(directionZ);
                        playerPosition.z += directionX * cosd(directionZ);
                        if (GetPlayerPointGroup() < 2) {
                            ClampPlayerPosition(&playerPosition, work->teamIndex);
                        }
                        SetPlayerItemState(&playerPosition, itemTypesByPointGroup[1][GetPlayerPointGroup()], work->teamIndex, work->playerSide);
                        ApproachVector(&work->position, 0.0f);
                        if (GetPlayerPointGroup() < 1) {
                            HuAudFXPlay(secondaryItemSoundEffects[work->teamIndex]);
                        }
                        else {
                            HuAudFXPlay(itemSoundEffects[work->teamIndex]);
                        }
                    }
                    work->contactFollowThroughActive = 0;
                    SetPlayerAnimation(object, 6);
                    VECSubtract(&playerPosition, &work->position, &targetPosition);
                    work->targetRotation = atan2d(targetPosition.x, targetPosition.z);
                    work->rotationStep = 0.1f;
                    work->aiActionFlags = 0x186A0;
                    work->framesInAction = 0;
                    break;
                }
                if (work->aiActionFlags < 0x2710) {
                    work->velocity.x = 0.07f * (work->movementTarget.x - work->position.x);
                    work->velocity.z = 0.07f * (work->movementTarget.z - work->position.z);
                    if (work->contactFollowThroughActive == 0) {
                        work->aiActionFlags = 0;
                        break;
                    }
                }
                else {
                    work->velocity.x *= 0.7f;
                    work->velocity.z *= 0.7f;
                    if (GetPlayerState(object) != 0) {
                        work->stateFlags7C = 0;
                        break;
                    }
                }
                break;
            case 3:
                if (work->framesInAction >= work->aiActionFlags) {
                    if (work->contactFollowThroughActive != 0) {
                        playerPosition = work->position;
                        playerPosition.y = 11.0f;
                        VECSubtract(&playerState->position, &work->position, &targetPosition);
                        directionZ = 180.0 + (atan2d(targetPosition.x, targetPosition.z));
                        directionZ += frandmod(0xB4) - 90.0f;
                        directionX = 200.0f * work->lastBallContactTime;
                        playerPosition.x += directionX * sind(directionZ);
                        playerPosition.z += directionX * cosd(directionZ);
                        ClampPlayerPosition(&playerPosition, work->teamIndex);
                        SetPlayerItemState(&playerPosition, itemTypesByPointGroup[2][GetPlayerPointGroup()], work->teamIndex, work->playerSide);
                        if (GetPlayerPointGroup() < 1) {
                            HuAudFXPlay(secondaryItemSoundEffects[work->teamIndex]);
                        }
                        else {
                            HuAudFXPlay(itemSoundEffects[work->teamIndex]);
                        }
                    }
                    work->contactFollowThroughActive = 0;
                    work->aiActionFlags = 0x186A0;
                    work->framesInAction = 0;
                    ApproachVector(&work->position, 1.3f);
                    break;
                }
                if (work->aiActionFlags < 0x2710) {
                    work->velocity.x = 0.15f * (work->movementTarget.x - work->position.x);
                    work->velocity.z = 0.15f * (work->movementTarget.z - work->position.z);
                    break;
                }
                work->velocity.x *= 0.95f;
                work->velocity.z *= 0.95f;
                if (GetPlayerState(object) != 0) {
                    SetPlayerAnimation(object, 8);
                    work->stateFlags7C = 4;
                }
                break;
            case 4:
                work->velocity.x *= 0.95f;
                work->velocity.z *= 0.95f;
                if (GetPlayerState(object) != 0) {
                    work->stateFlags7C = 0;
                }
                break;
        }
    }
}

void UpdatePlayerContact(OMOBJ *object)
{
    Vec targetPosition;
    Vec playerPosition;
    M433PlayerState *movementDelta;
    float targetDistance;
    float movementSpeed;
    float targetAngle;
    M433PlayerState *work = object->data;
    s32 contactSoundEffects[2] = { 0x6A8, 0x6A9 };
    movementDelta = work->opponentObject->data;
    work->movementLockActive = 0;
    work->canContactBall = 0;
    switch (work->stateFlags80) {
        case 0:
            if ((work->stateFlags7C == 0) && (work->stateFlags78 == 0) && (work->stateFlags80 == 0) && (work->isPlaying = 0, work->attackReady = 0, ((work->hasActiveAction == 0) != 0))
                && (IsPlayerContact(work->teamIndex, work->playerSide) != 0) && (GetPlayerPointGroup() < 3) && (work->teamIndex == GetItemTargetPosition(&targetPosition))) {
                VECSubtract(&targetPosition, &work->position, &playerPosition);
                playerPosition.y = 0.0f;
                if (!(VECMag(&playerPosition) > 500.0f)) {
                    UpdateItemFlightPosition(0.5f, &targetPosition);
                    targetPosition.y -= characterModelHeights[work->characterId];
                    VECSubtract(&targetPosition, &work->position, &playerPosition);
                    playerPosition.y = 0.0f;
                    if ((VECMag(&playerPosition) < 200.0f) && (targetPosition.y >= 50.0f)) {
                        work->canContactBall = 1;
                    }
                    if (((work->pressedButtons & 0x200) != 0) && (work->canContactBall != 0)) {
                        work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                        SetPlayerAnimation(object, 0xA);
                        work->stateFlags80 = 1;
                        work->framesInAction = 0;
                        work->isPlaying = 1;
                        if (work->canContactBall != 0) {
                            movementSpeed = 0.5f;
                            work->aiMovementSpeed = (2.0f * -targetPosition.y) / (movementSpeed * movementSpeed);
                            work->velocity.y = (1.0f/REFRESH_RATE) * (-work->aiMovementSpeed * movementSpeed);
                            VECSubtract(&targetPosition, &work->position, &playerPosition);
                            playerPosition.y = 0.0f;
                            work->velocity.x = (1.0f/REFRESH_RATE) * (playerPosition.x / 0.5f);
                            work->velocity.z = (1.0f/REFRESH_RATE) * (playerPosition.z / 0.5f);
                            if (work->teamIndex != 0) {
                                targetAngle = -90.0f;
                            }
                            else {
                                targetAngle = 90.0f;
                            }
                            work->targetRotation = targetAngle;
                            work->rotationStep = 0.2f;
                            work->rotationAcceleration = 0.0f;
                            work->attackReady = 1;
                        }
                        else {
                            work->velocity.y = 890.0f*(1.0f/REFRESH_RATE);
                        }
                        work->hasContactedBall = 1;
                        ApproachVector(&work->position, 0.75f);
                    }
                }
            }
            break;
        case 1:
            work->velocity.y += (1.0f/REFRESH_RATE) * ((1.0f/REFRESH_RATE) * work->aiMovementSpeed);
            if (work->attackReady != 0) {
                targetDistance = ((1.0f/REFRESH_RATE) * work->framesInAction) - 0.35f;
                if ((targetDistance >= 0.0f) && (targetDistance < 0.4)) {
                    work->movementLockActive = 1;
                    if ((work->pressedButtons & 0x100) != 0) {
                        SetPlayerAnimation(object, 0xB);
                        UpdateItemMotion(work->stickAngle, work->stickMagnitude, work->teamIndex, work->movementLockActive, &targetPosition);
                        SetPlayerItemState(&targetPosition, 5, work->teamIndex, work->playerSide);
                        work->attackReady = 0;
                        work->movementLockActive = 0;
                        HuAudFXPlay(contactSoundEffects[work->teamIndex]);
                    }
                }
                else if (targetDistance > 0.0f) {
                    work->attackReady = 0;
                    work->movementLockActive = 0;
                }
                UpdateItemMotion(work->stickAngle, work->stickMagnitude, work->teamIndex, work->movementLockActive, &targetPosition);
            }
            if (work->position.y <= 0.0f) {
                ApproachVector(&work->position, 0.75f);
                work->isPlaying = 0;
                work->stateFlags80 = 0;
                work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                work->hasContactedBall = 0;
                work->attackReady = 0;
            }
            break;
    }
}

void UpdatePlayerResponse(OMOBJ *object)
{
    Vec targetPosition;
    Vec movementDelta;
    float playerSpeed;
    float opponentSpeed;

    M433PlayerState *work = object->data;
    s32 player = 0;
    s32 soundIds[2] = { 0x6A8, 0x6A9 };
    M433PlayerState *opponent = work->opponentObject->data;

    work->isItemCrossingNearPlayer = 0;
    switch (work->playerActionState) {
        case 0:
            if ((work->stateFlags7C == 0) && (work->playerActionState == 0) && (work->stateFlags80 == 0) && (work->isPlaying = 0, work->attackReady = 0, ((work->hasActiveAction == 0) != 0))
                && (work->teamIndex == GetItemPosition(&targetPosition)) && (IsPlayerContactGroup(work->teamIndex) == 0)) {
                float playerX = work->position.x;
                float depthDifference;
                if (work->teamIndex == 0) {
                    playerX *= -1.0f;
                }
                if (!(playerX > 230.0f) && (GetActiveContactGroup() != 2)) {
                    if (GetItemSoundPosition(&targetPosition) != 0) {
                        player = 1;
                    }
                    if (player != 0) {
                        playerX = targetPosition.z - work->position.z;
                        if (playerX > 100.0f) {
                            playerX = 100.0f;
                        }
                        else if (playerX < -100.0f) {
                            playerX = -100.0f;
                        }
                        else {
                            work->isItemCrossingNearPlayer = 1;
                        }
                    }
                    if ((work->pressedButtons & 0x200) != 0) {
                        work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                        SetPlayerAnimation(object, 0xA);
                        work->playerActionState = 1;
                        work->framesInAction = 0;
                        work->position = 1;
                        if (work->teamIndex != 0) {
                            opponentSpeed = -90.0f;
                        }
                        else {
                            opponentSpeed = 90.0f;
                        }
                        work->itemSpinSpeed = opponentSpeed;
                        work->itemPosition = 0.2f;
                        work->rotationAcceleration = 0.0f;
                        ApproachVector(&work->position, 0.75f);
                        depthDifference = 0.5f;
                        work->aiMovementSpeed = (2.0f * -(350.0f - characterModelHeights[work->playerVisualMode])) / (depthDifference * depthDifference);
                        work->velocity.y = (1.0f/REFRESH_RATE) * (-work->aiMovementSpeed * depthDifference);
                        work->velocity.x = (1.0f/REFRESH_RATE) * (-work->position.x / 0.75f);
                        if (player != 0) {
                            work->velocity.z = (1.0f/REFRESH_RATE) * (playerX / 0.5f);
                        }
                        work->hasContactedBall = 1;
                        work->isItemCrossingNearPlayer = 0;
                    }
                }
            }
            break;
        case 1:
            work->velocity.y += (1.0f/REFRESH_RATE) * ((1.0f/REFRESH_RATE) * work->aiMovementSpeed);
            if ((work->teamIndex == GetItemTargetPosition(&targetPosition)) && (GetPlayerPointGroup() == 0)) {
                UpdateItemFlightPosition(0.0f, &targetPosition);
                #if VERSION_NTSC
                if ((targetPosition.x * work->position.x) > 0.0f) {
                #else
                if ((targetPosition.x * work->position.x) > 0.0f && targetPosition.y > 100) {
                #endif
                    targetPosition.y = work->position.y;
                    VECSubtract(&targetPosition, &work->position, &movementDelta);
                    if (VECMag(&movementDelta) < 60.000004f) {
                        if (frandmod(0x64) < 0x1E) {
                            GetItemTargetPosition(&targetPosition);
                            targetPosition.x *= -0.5f;
                            SetPlayerItemState(&targetPosition, 5, work->teamIndex, work->playerSide);
                            HuAudFXPlay(soundIds[work->teamIndex]);
                        }
                        else {
                            GetItemTargetPosition(&targetPosition);
                            playerSpeed = frandmod(0x168);
                            targetPosition.x += 100.0 * (2.0 * sind(playerSpeed));
                            targetPosition.z += 100.0 * (2.0 * cosd(playerSpeed));
                            ClampPlayerPosition(&targetPosition, work->teamIndex);
                            SetPlayerItemState(&targetPosition, 8, work->teamIndex, work->playerSide);
                            HuAudFXPlay(soundIds[work->teamIndex]);
                        }
                        SetPlayerAnimation(object, 0xD);
                        opponent->contactFollowThroughActive = 0;
                    }
                }
            }
            if (work->position.y <= 0.0f) {
                ApproachVector(&work->position, 0.75f);
                work->isPlaying = 0;
                work->playerActionState = 0;
                work->velocity.x = work->velocity.y = work->velocity.z = 0.0f;
                work->hasContactedBall = 0;
            }
            break;
    }
}

void UpdatePlayerState(OMOBJ *object)
{
    Vec separationVector;
    float playerDistance;
    M433PlayerState *player;
    M433PlayerState *player;
    s32 playerIndex;
    s32 opponentIndex;
    s32 teamIndex;

    for (teamIndex = 0; teamIndex < 2; teamIndex++) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            player = playerObjects[playerIndex]->data;
            for (opponentIndex = 0; opponentIndex < 4; opponentIndex++) {
                if (playerIndex != opponentIndex) {
                    player = playerObjects[opponentIndex]->data;
                    VECSubtract(&player->position, &player->position, &separationVector);
                    separationVector.y = 0.0f;
                    playerDistance = VECMag(&separationVector);
                    if (playerDistance < (player->characterSkill + player->characterSkill)) {
                        if (playerDistance > 0.0f) {
                            playerDistance = 0.5f * ((player->characterSkill + player->characterSkill) - playerDistance);
                        }
                        else {
                            playerDistance = 0.5f * (player->characterSkill + player->characterSkill);
                            separationVector.x = frandmod(0x3E8) - 0x1F4;
                            separationVector.z = frandmod(0x3E8) - 0x1F4;
                        }
                        VECNormalize(&separationVector, &separationVector);
                        VECScale(&separationVector, &separationVector, playerDistance);
                        VECAdd(&player->position, &separationVector, &player->position);
                        VECSubtract(&player->position, &separationVector, &player->position);
                    }
                }
            }
            if (player->position.y < 0.0f) {
                player->position.y = 0.0f;
            }
            if ((player->position.z - player->characterSkill) < -500.0f) {
                player->position.z = -500.0f + player->characterSkill;
            }
            else if ((player->position.z + player->characterSkill) > 500.0f) {
                player->position.z = 500.0f - player->characterSkill;
            }
            if (player->teamIndex == 0) {
                player->position.x *= -1.0f;
            }
            if ((player->position.x - player->characterSkill) < 20.0f) {
                player->position.x = 20.0f + player->characterSkill;
            }
            else if ((60.000004f + player->position.x) > 800.0f) {
                player->position.x = 740.0f;
            }
            if (player->teamIndex == 0) {
                player->position.x *= -1.0f;
            }
        }
    }
}

void SetPlayerAnimation(OMOBJ *object, u32 animationIndex)
{
    M433PlayerState *work = object->data;
    if ((work->itemOutcome != animationIndex) && (animationIndex < 0x15)) {
        float playerX = REFRESH_RATE * playerMotionTransitions[animationIndex].transitionDuration;
        if (work->itemOutcome < 0) {
            playerX = 0.0f;
        }
        work->itemOutcome = animationIndex;
        if (work->itemCollected < 0) {
            CharMotionShiftSet(
                work->playerVisualMode, object->mtnId[playerMotionTransitions[animationIndex].motionIndex], REFRESH_RATE * playerMotionTransitions[animationIndex].motionStartTime, playerX, playerMotionTransitions[animationIndex].motionAttributes);
        }
        else {
            Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[playerMotionTransitions[animationIndex].motionIndex], REFRESH_RATE * playerMotionTransitions[animationIndex].motionStartTime, playerX,
                playerMotionTransitions[animationIndex].motionAttributes);
        }
        if (playerMotionTransitions[animationIndex].motionEndTime >= 0.0f) {
            Hu3DMotionShiftStartEndSet(object->mdlId[0], REFRESH_RATE * playerMotionTransitions[animationIndex].motionStartTime, REFRESH_RATE * playerMotionTransitions[animationIndex].motionEndTime);
        }
        work->itemActive = 0;
    }
}

s32 GetPlayerState(OMOBJ *object)
{
    M433PlayerState *work = object->data;
    s32 motionFinished = 0;
    if (work->itemCollected < 0) {
        if (((CharMotionEndCheck(work->playerVisualMode) != 0) || (work->itemActive != 0)) && (CharMotionShiftIDGet(work->playerVisualMode) < 0)) {
            motionFinished = 1;
        }
    }
    else if (((Hu3DMotionEndCheck(object->mdlId[0]) != 0) || (work->itemActive != 0)) && (Hu3DMotionShiftIDGet(object->mdlId[0]) < 0)) {
        motionFinished = 1;
    }
    return motionFinished;
}

s32 GetPlayerActionState(OMOBJ *object)
{
    M433PlayerState *work = object->data;
    s32 player = 0;
    if (work->itemCollected < 0) {
        if (CharMotionShiftIDGet(work->playerVisualMode) < 0) {
            player = 1;
        }
    }
    else if (Hu3DMotionShiftIDGet(object->mdlId[0]) < 0) {
        player = 1;
    }
    return player;
}

s32 CheckPlayerState(OMOBJ *object, u32 stateIndex)
{
    s32 motionStarted = 0;
    if (GetPlayerState(object)) {
        SetPlayerAnimation(object, stateIndex);
        motionStarted = 1;
    }
    return motionStarted;
}

s32 CheckPlayerAction(OMOBJ *object, u32 actionIndex)
{
    s32 motionStarted = 0;
    if (GetPlayerActionState(object)) {
        SetPlayerAnimation(object, actionIndex);
        motionStarted = 1;
    }
    return motionStarted;
}

void UpdatePlayerManager(OMOBJ *object)
{
    M433ItemManagerState *work;
    s32 particleIndex;
    s32 opponent;
    HU3DPARTICLEDATA *particleData;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M433ItemManagerState), HU_MEMNUM_OVL);
    work = object->data;
    memset(work, 0, sizeof(M433ItemManagerState));
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 0));
    Hu3DModelShadowSet(object->mdlId[0]);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 4));
    Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_LOOP);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 4));
    Hu3DModelAttrSet(object->mdlId[2], HU3D_MOTATTR_LOOP);
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 3));
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 7));
    Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 5));
    Hu3DModelAttrSet(object->mdlId[6], HU3D_ATTR_DISPOFF);
    Hu3DModelScaleSet(object->mdlId[6], 0.75f, 0.75f, 0.75f);
    Hu3DModelLayerSet(object->mdlId[6], 2);
    object->mdlId[7] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 6));
    Hu3DModelAttrSet(object->mdlId[7], HU3D_ATTR_DISPOFF);
    Hu3DModelScaleSet(object->mdlId[7], 0.75f, 0.75f, 0.75f);
    Hu3DModelLayerSet(object->mdlId[7], 2);
    opponent = Hu3DParticleCreate(HuSprAnimRead(HuDataReadNum(0x120002, HU_MEMNUM_OVL)), 1);
    object->mdlId[8] = opponent;
    Hu3DModelLayerSet(opponent, 2);
    Hu3DParticleHookSet(opponent, UpdateItemEffectParticles);
    Hu3DModelAttrSet(opponent, HU3D_ATTR_DISPOFF);
    opponent = Hu3DParticleCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_M433, 9)), 0xC8);
    object->mdlId[5] = opponent;
    Hu3DModelLayerSet(opponent, 2);
    Hu3DParticleHookSet(opponent, UpdateItemParticles);
    for (particleData = ((HU3DPARTICLE *)Hu3DData[opponent].hookData)->data, particleIndex = 0; particleIndex < 0xC8; particleIndex++, particleData++) {
        particleData->scale = 0.0f;
        particleData->color.a = 0;
        particleData->time = 0;
    }
    object->mdlId[9] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 2));
    Hu3DModelScaleSet(object->mdlId[9], 1.05f, 1.05f, 1.05f);
    Hu3DModelAttrSet(object->mdlId[9], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(object->mdlId[9], 1);
    Hu3DModelShadowSet(object->mdlId[9]);
    for (particleIndex = 1; particleIndex < 0x15; particleIndex++) {
        if (particleIndex == 0xA) {
            object->mdlId[particleIndex + 9] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M433, 1));
        }
        else {
            object->mdlId[particleIndex + 9] = Hu3DModelLink(object->mdlId[9]);
            Hu3DModelLayerSet(object->mdlId[particleIndex + 9], 1);
        }
        Hu3DModelShadowSet(object->mdlId[particleIndex + 9]);
        Hu3DModelScaleSet(object->mdlId[particleIndex + 9], 1.05f, 1.05f, 1.05f);
        Hu3DModelAttrSet(object->mdlId[particleIndex + 9], HU3D_ATTR_DISPOFF);
    }
    object->trans.x = work->position.x = 0.0f;
    object->trans.y = work->position.y = 0.0f;
    object->trans.z = work->position.z = 0.0f;
    work->spinAngle = 0.0f;
    work->spinAxis.x = work->spinAxis.y = 0.0f;
    work->spinAxis.z = 1.0f;
    MTXIdentity(work->rotationMatrix);
    work->playerSide = 0;
    object->objFunc = UpdatePlayerEvents;
}

void UpdatePlayerEvents(OMOBJ *object)
{
    s32 modelIndex;

    M433ItemManagerState *work = object->data;
    s32 unusedInitializationValue = 0;
    work->playerVisualMode = 0;
    switch (work->playerSide) {
        case 0:
            work->itemCollected = 1;
            break;
        case 1:
            work->spinAngle = 0.0f;
            work->spinAxis.x = work->spinAxis.y = 0.0f;
            work->spinAxis.z = 1.0f;
            MTXIdentity(work->rotationMatrix);
            work->playerVisualMode = 1;
            break;
        case 10:
            work->playerVisualMode = 1;
            break;
    }
    work->itemModelState++;
    if (work->itemEffectState != 0) {
        work->itemScaleTarget = 1.0f;
        if (work->itemSpinState == 2) {
            work->itemWorldOffset += 720.0f*(1.0f/REFRESH_RATE);
            if (work->itemWorldOffset >= 360.0f) {
                work->itemWorldOffset -= 360.0f;
            }
            work->itemScaleTarget = 1.5f;
            work->itemSpinSpeed += 0.9f*(1.0f/REFRESH_RATE);
            Hu3DMotionSpeedSet(object->mdlId[3], 2.5f);
        }
        else {
            Hu3DMotionTimeSet(object->mdlId[3], 0.0f);
            Hu3DMotionSpeedSet(object->mdlId[3], 0.0f);
        }
        Hu3DModelRotSet(object->mdlId[3], 0.0f, work->itemWorldOffset, 0.0f);
        Hu3DModelAttrReset(object->mdlId[3], HU3D_ATTR_DISPOFF);
        Hu3DModelPosSet(object->mdlId[3], work->itemPosition.x, 3.0f + work->itemPosition.y, work->itemPosition.z);
        work->itemSpinScale++;
        work->itemEffectState = 0;
    }
    else if (work->itemSpinState != 0) {
        work->itemSpinState = 0;
        work->itemScaleTarget = 0.0f;
    }
    work->itemScaleFactor = work->itemScaleFactor + (0.2f * (work->itemScaleTarget - work->itemScaleFactor));
    Hu3DModelScaleSet(object->mdlId[3], work->itemScaleFactor, 1.0f, work->itemScaleFactor);
    if ((work->itemScaleTarget <= 0.0f) && (work->itemScaleFactor < 0.1f)) {
        Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
        work->itemScaleFactor = 0.0f;
    }
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        work->stateFlags7C[modelIndex] += (0.3f * (work->itemScale[modelIndex] - work->stateFlags7C[modelIndex]));
        if (work->itemScale[modelIndex] > 0.0f) {
            Hu3DModelAttrReset(object->mdlId[modelIndex + 1], HU3D_ATTR_DISPOFF);
        }
        else if (work->stateFlags7C[modelIndex] < 0.1f) {
            Hu3DModelAttrSet(object->mdlId[modelIndex + 1], HU3D_ATTR_DISPOFF);
        }
        Hu3DModelScaleSet(object->mdlId[modelIndex + 1], work->stateFlags7C[modelIndex], 1.0f, work->stateFlags7C[modelIndex]);
    }
    UpdatePlayerEffects(object);
    UpdateItemObject(object);
    object->trans.x = work->position.x;
    object->trans.y = work->position.y;
    object->trans.z = work->position.z;
    if ((getRoundIntroState() >= 3) && (work->itemCollected != 0) && (isOutroSequenceFinished() != 0)) {
        ClampItemPosition(&work->position);
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        object->objFunc = InitializePlayerEffects;
    }
}

void InitializePlayerEffects(OMOBJ *object) { }

s32 UpdatePlayerEffects(OMOBJ *object)
{
    Mtx rotationMatrix;
    Vec movementDelta;
    M433GameEventState *gameEventState;
    s32 opponentIndex;

    M433ItemManagerState *work = object->data;
    if ((work->playerVisualMode == 0) && (work->itemCollected == 0) && (work->itemActionState == 0)) {
        if (work->itemActive == 0) {
            InitializeItemTrajectory(&work->itemPhysics, &work->position, &work->targetPosition, work->itemType, work->itemSpinSpeed);
            ResetItemTrajectory(&work->itemPhysics);
            UpdateItemTrajectory(&work->itemPhysics);
            work->spinAngle = work->itemSpinSpeed * (8.0f * itemSpinSpeeds[work->itemType]);
            VECSubtract(&work->targetPosition, &work->position, &movementDelta);
            movementDelta.y = 0.0f;
            if (VECMag(&movementDelta) <= 0.0f) {
                work->spinAxis.x = work->spinAxis.y = 0.0f;
                work->spinAxis.z = 1.0f;
            }
            else {
                VECNormalize(&movementDelta, &movementDelta);
                work->spinAxis.x = -movementDelta.z;
                work->spinAxis.y = 0.0f;
                work->spinAxis.z = movementDelta.x;
            }
            work->elapsedTime = 0.0f;
            opponentIndex = 0;
            if (work->itemScale[0] > 0.0f) {
                opponentIndex++;
            }
            Hu3DModelPosSet(object->mdlId[opponentIndex + 1], work->itemPhysics.targetPosition.x, 3.0f + work->itemPhysics.targetPosition.y, work->itemPhysics.targetPosition.z);
            if (work->itemScaleFactor <= 0.0f) {
                #if VERSION_NTSC
                work->itemModelState = 9;
                #else
                work->itemModelState = 7;
                #endif
            }
            if (itemSideBySideFlags[work->itemType] != 0) {
                work->itemScale[opponentIndex] = 1.0f;
            }
            work->itemScale[1 - opponentIndex] = 0.0f;
        }
        work->itemActive = 1;
        if (work->spinAngle != 0.0f) {
            MTXRotAxisRad(rotationMatrix, &work->spinAxis, 0.017453292f * work->spinAngle);
            MTXConcat(rotationMatrix, work->rotationMatrix, work->rotationMatrix);
            MTXCopy(work->rotationMatrix, Hu3DData[object->mdlId[0]].mtx);
        }
        work->elapsedTime += (1.0f/REFRESH_RATE);
        work->itemActionState = CalculateItemTrajectory(&work->itemPhysics, &work->position, &work->itemStartPosition, work->elapsedTime);
        if (work->itemActionState != 0) {
            work->itemScale[0] = work->itemScale[1] = 0.0f;
            if (work->position.y <= 12.0f) {
                gameEventState = gameEventObject->data;
                work->playerActionState = 0;
                Hu3DModelPosSetV(object->mdlId[4], &work->position);
                Hu3DMotionTimeSet(object->mdlId[4], 0.0f);
                if ((work->itemType == 5) || (work->itemType == 2)) {
                    Hu3DMotionSpeedSet(object->mdlId[4], 1.5f);
                    ApproachVector(&work->position, 1.0f);
                }
                else {
                    Hu3DMotionSpeedSet(object->mdlId[4], 1.0f);
                    ApproachVector(&work->position, 0.5f);
                    work->position.y += 11.0f;
                }
                work->itemType = 0;
                work->itemCollected = 1;
                gameEventState->pointScored = 1;
                gameEventState->sGroundGrp = -1;
                if (fabs(work->position.z) < 350.0) {
                    if (fabs(work->position.x) < 700.0) {
                        gameEventState->sGroundGrp = 0;
                        if (work->position.x >= 0.0f) {
                            gameEventState->sGroundGrp = 1;
                        }
                    }
                }
                #if VERSION_PAL
                for(opponentIndex=0; opponentIndex<4; opponentIndex++) {
                    omObjGetDataAs(playerObjects[opponentIndex], M433PlayerState)->contactFollowThroughActive = 0;
                }
                #endif
            }
        }
    }
    else {
        work->itemActive = 0;
    }
    return work->itemActionState;
}

void UpdateItemObject(OMOBJ *object)
{
    Mtx particleMatrix;
    Mtx particlePosition;
    Vec particleVelocity;
    Vec particleDirection;
    float particleSpeed;
    float particleScale;
    float playerSpeed;
    float opponentSpeed;
    s32 particleIndex;

    M433ItemManagerState *work = object->data;
    if (work->playerActionState != 0) {
        Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
        particleScale = work->itemPhysics.elapsedTime;
        playerSpeed = particleScale;
        particleSpeed = playerSpeed - ((1.2f/REFRESH_RATE) * work->itemSpinSpeed);
        if (particleSpeed < 0.0f) {
            particleSpeed = 0.0f;
        }
        particleSpeed = (particleScale - particleSpeed) / 21.0f;
        MTXCopy(work->rotationMatrix, particlePosition);
        opponentSpeed = (work->spinAngle * (1.2f * work->itemSpinSpeed)) / 21.0f;
        MTXRotAxisRad(particleMatrix, &work->spinAxis, MTXDegToRad(-opponentSpeed));
        for (particleIndex = 0; particleIndex < 0x15; particleIndex++) {
            Hu3DModelAttrReset(object->mdlId[particleIndex + 9], HU3D_ATTR_DISPOFF);
            CalculateItemTrajectory(&work->itemPhysics, &particleVelocity, &particleDirection, particleScale);
            Hu3DModelPosSetV(object->mdlId[particleIndex + 9], &particleVelocity);
            MTXCopy(particlePosition, Hu3DData[object->mdlId[particleIndex + 9]].mtx);
            MTXConcat(particleMatrix, particlePosition, particlePosition);
            particleScale -= particleSpeed;
        }
        work->itemPhysics.elapsedTime = playerSpeed;
        return;
    }
    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
    for (particleIndex = 0; particleIndex < 0x15; particleIndex++) {
        Hu3DModelAttrSet(object->mdlId[particleIndex + 9], HU3D_ATTR_DISPOFF);
    }
}

void InitializeItemTrajectory(M433ItemTrajectory *trajectory, Vec *startPosition, Vec *targetPosition, u32 itemType, float playerSpeed)
{
    Vec segmentDelta;
    Vec floorImpactPosition;
    float flightDuration;
    float segmentDistance;
    float playerSpeed;

    trajectory->floorHeightThreshold = 0.0f;
    trajectory->floorCrossingTime = 0.0f;
    flightDuration = itemTrajectoryParameters[itemType].y;
    trajectory->flightDuration = flightDuration;
    if (trajectory->flightDuration <= 0.0f) {
        if (((targetPosition->x * startPosition->x) < 0.0f) && (startPosition->x != 0.0f) && (itemTrajectoryParameters[itemType].x > 0.0f)) {
            trajectory->floorHeightThreshold = itemTrajectoryParameters[itemType].x;
            trajectory->floorCrossingTime = fabs(startPosition->x / (targetPosition->x - startPosition->x));
            playerSpeed = startPosition->y + (trajectory->floorCrossingTime * (targetPosition->y - startPosition->y));
            if (playerSpeed < trajectory->floorHeightThreshold) {
                floorImpactPosition.x = startPosition->x + (trajectory->floorCrossingTime * (targetPosition->x - startPosition->x));
                floorImpactPosition.y = trajectory->floorHeightThreshold;
                floorImpactPosition.z = startPosition->z + (trajectory->floorCrossingTime * (targetPosition->z - startPosition->z));
                VECSubtract(targetPosition, &floorImpactPosition, &segmentDelta);
                segmentDistance = VECMag(&segmentDelta);
                VECSubtract(&floorImpactPosition, startPosition, &segmentDelta);
                segmentDistance += VECMag(&segmentDelta);
            }
            else {
                VECSubtract(targetPosition, startPosition, &segmentDelta);
                segmentDistance = VECMag(&segmentDelta);
            }
        }
        else {
            VECSubtract(targetPosition, startPosition, &segmentDelta);
            segmentDistance = VECMag(&segmentDelta);
        }
        flightDuration = segmentDistance / (playerSpeed * itemTrajectoryParameters[itemType].z);
        trajectory->flightDuration = flightDuration;
    }
    trajectory->elapsedTime = 0.0f;
    trajectory->startPosition = *startPosition;
    trajectory->targetPosition = *targetPosition;
    trajectory->verticalDisplacement = targetPosition->y - startPosition->y;
    trajectory->verticalAcceleration = -980.0f;
    VECSubtract(targetPosition, startPosition, &segmentDelta);
    segmentDelta.y = 0.0f;
    trajectory->horizontalDistance = VECMag(&segmentDelta);
    trajectory->horizontalAcceleration = 0.0f;
    if (flightDuration > 0.0f) {
        trajectory->verticalVelocity = (trajectory->verticalDisplacement - (0.5f * (-980.0f * (flightDuration * flightDuration)))) / flightDuration;
        trajectory->horizontalVelocity = trajectory->horizontalDistance / flightDuration;
    }
    else {
        trajectory->verticalVelocity = trajectory->verticalDisplacement;
        trajectory->horizontalVelocity = trajectory->horizontalDistance;
    }
    trajectory->trajectoryScale = itemTrajectoryScales[itemType];
}

void UpdateItemTrajectory(M433ItemTrajectory *trajectory)
{
    float horizontalSpeed;
    float durationEstimate;
    float inverseDuration;
    float playerSpeed;
    float opponentSpeed;
    float upperDurationBound;
    float initialVerticalSpeed;
    float estimatedHeight;
    s32 refinementStep;

    if (!(trajectory->floorHeightThreshold <= 0.0f)) {
        durationEstimate = trajectory->flightDuration;
        opponentSpeed = trajectory->flightDuration;
        upperDurationBound = 10.0f * trajectory->flightDuration;
        for (refinementStep = 0; refinementStep < 0x10; refinementStep++) {
            inverseDuration = 1.0f / durationEstimate;
            horizontalSpeed = trajectory->horizontalDistance * inverseDuration;
            initialVerticalSpeed = (trajectory->verticalDisplacement * inverseDuration) - (0.5f * trajectory->verticalAcceleration * durationEstimate);
            playerSpeed = trajectory->floorCrossingTime * durationEstimate;
            estimatedHeight = trajectory->startPosition.y + ((playerSpeed * (0.5f * trajectory->verticalAcceleration * playerSpeed)) + (initialVerticalSpeed * playerSpeed));
            if (trajectory->floorHeightThreshold > estimatedHeight) {
                opponentSpeed = durationEstimate;
            }
            else {
                upperDurationBound = durationEstimate;
            }
            durationEstimate = 0.5f * (opponentSpeed + upperDurationBound);
        }
        inverseDuration = 1.0f / durationEstimate;
        trajectory->flightDuration = durationEstimate;
        trajectory->horizontalVelocity = trajectory->horizontalDistance * inverseDuration;
        trajectory->verticalVelocity = (trajectory->verticalDisplacement * inverseDuration) - (0.5f * trajectory->verticalAcceleration * durationEstimate);
    }
}

void ResetItemTrajectory(M433ItemTrajectory *trajectory)
{
    float horizontalSpeed;
    float gravityEstimate;
    float inverseDuration;
    float playerSpeed;
    float opponentSpeed;
    float upperGravityBound;
    float initialVerticalSpeed;
    float estimatedHeight;
    s32 refinementStep;

    if (!(trajectory->floorHeightThreshold <= 0.0f)) {
        playerSpeed = trajectory->floorCrossingTime * trajectory->flightDuration;
        inverseDuration = 1.0f / trajectory->flightDuration;
        gravityEstimate = trajectory->verticalAcceleration;
        opponentSpeed = trajectory->verticalAcceleration;
        upperGravityBound = trajectory->verticalAcceleration * trajectory->trajectoryScale;
        for (refinementStep = 0; refinementStep < 0x10; refinementStep++) {
            horizontalSpeed = trajectory->horizontalDistance * inverseDuration;
            initialVerticalSpeed = (trajectory->verticalDisplacement * inverseDuration) - (0.5f * gravityEstimate * trajectory->flightDuration);
            estimatedHeight = trajectory->startPosition.y + ((playerSpeed * (0.5f * gravityEstimate * playerSpeed)) + (initialVerticalSpeed * playerSpeed));
            if (trajectory->floorHeightThreshold > estimatedHeight) {
                opponentSpeed = gravityEstimate;
            }
            else {
                upperGravityBound = gravityEstimate;
            }
            gravityEstimate = 0.5f * (opponentSpeed + upperGravityBound);
        }
        trajectory->verticalAcceleration = gravityEstimate;
        trajectory->horizontalVelocity = trajectory->horizontalDistance * inverseDuration;
        trajectory->verticalVelocity = (trajectory->verticalDisplacement * inverseDuration) - (0.5f * trajectory->verticalAcceleration * trajectory->flightDuration);
    }
}

s32 CalculateItemTrajectory(M433ItemTrajectory *trajectory, Vec *itemPosition, Vec *positionDelta, float elapsedTime)
{
    Vec previousPosition;
    Vec horizontalDirection;
    float horizontalDisplacement;
    float playerSpeed;

    if (elapsedTime < 0.0f) {
        elapsedTime = 0.0f;
    }
    if (elapsedTime > trajectory->flightDuration) {
        elapsedTime = trajectory->flightDuration;
    }
    trajectory->elapsedTime = elapsedTime;
    if (trajectory->horizontalDistance > 0.0f) {
        VECSubtract(&trajectory->targetPosition, &trajectory->startPosition, &horizontalDirection);
        horizontalDirection.y = 0.0f;
        VECNormalize(&horizontalDirection, &horizontalDirection);
    }
    else {
        horizontalDirection.x = horizontalDirection.y = horizontalDirection.z = 0.0f;
    }
    previousPosition = *itemPosition;
    horizontalDisplacement = (elapsedTime * (0.5f * trajectory->horizontalAcceleration * elapsedTime)) + (trajectory->horizontalVelocity * elapsedTime);
    playerSpeed = (elapsedTime * (0.5f * trajectory->verticalAcceleration * elapsedTime)) + (trajectory->verticalVelocity * elapsedTime);
    itemPosition->x = trajectory->startPosition.x + (horizontalDirection.x * horizontalDisplacement);
    itemPosition->y = trajectory->startPosition.y + playerSpeed;
    itemPosition->z = trajectory->startPosition.z + (horizontalDirection.z * horizontalDisplacement);
    VECSubtract(itemPosition, &previousPosition, positionDelta);
    if (elapsedTime >= trajectory->flightDuration) {
        return 1;
    }
    return 0;
}

void UpdateItemParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    s32 particleIndex;

    for (particleData = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->time != 0) {
            VECAdd(&particleData->pos, &particleData->vel, &particleData->pos);
            particleData->vel.x *= 0.92f;
            particleData->vel.z *= 0.92f;
            particleData->vel.y += -980.0f*(1.0f/REFRESH_RATE)*(1.0f/REFRESH_RATE);
            particleData->scale += 2.0f;
            particleData->color.a *= 0.99f;
            if (particleData->time < (REFRESH_RATE*0.4f)) {
                particleData->color.a = 0.9f * particleData->color.a;
            }
            if (--particleData->time == 0) {
                particleData->time = 0;
                particleData->scale = 0.0f;
                particleData->color.a = 0;
            }
        }
    }
    DCStoreRange(particle->data, particle->maxCnt * 0x44);
}

void ApproachVector(Vec *position, float approachFactor)
{
    Vec movementDelta;
    float movementLength;
    float movementScale;
    HU3DPARTICLEDATA *playerState;
    s32 movementIndex;
    u32 opponent;

    playerState = ((HU3DPARTICLE *)Hu3DData[itemManagerObject->mdlId[5]].hookData)->data;
    opponent = 3.0f + (12.0f * approachFactor);
    for (movementIndex = 0; movementIndex < 0xC8; movementIndex++, playerState++) {
        if (playerState->time == 0) {
            playerState->time = REFRESH_RATE * (0.6f + (0.0006f * frandmod(0x3E8)));
            movementScale = frandmod(0x168);
            movementDelta.x = sind(movementScale);
            movementDelta.y = 0.0f;
            movementDelta.z = cosd(movementScale);
            movementLength = 0.030000001f * frandmod(0x3E8);
            playerState->pos.x = position->x + (movementDelta.x * movementLength);
            playerState->pos.y = 10.0f + (position->y + (0.030000001f * frandmod(0x3E8)));
            playerState->pos.z = position->z + (movementDelta.z * movementLength);
            movementLength = (50.0f*(1.0f/REFRESH_RATE)) + ((0.1f/REFRESH_RATE) * frandmod(0x3E8));
            movementLength *= 1.0f + approachFactor;
            playerState->vel.x = movementDelta.x * movementLength;
            playerState->vel.z = movementDelta.z * movementLength;
            playerState->vel.y = ((0.3f*100)*(1.0f/REFRESH_RATE)) + ((0.07f*(1.0f/REFRESH_RATE)) * frandmod(0x3E8));
            playerState->vel.y *= (1.0f + approachFactor);
            playerState->zRot = frandmod(0x168);
            playerState->scale = 60.0f + frandmod(0x1E);
            playerState->color.a = frandmod(0x64) + 0x78;
            movementLength = 0.001f * frandmod(0x3E8);
            playerState->color.r = 235.0f + (20.0f * movementLength);
            playerState->color.g = 180.0f + (75.0f * movementLength);
            playerState->color.b = 155.0f + (100.0f * movementLength);
            if (--opponent < 1) {
                break;
            }
        }
    }
}

void ClampItemPosition(Vec *itemPosition)
{
    HU3DPARTICLEDATA *player;

    HU3DPARTICLE *playerState = Hu3DData[itemManagerObject->mdlId[8]].hookData;
    Hu3DModelAttrReset(itemManagerObject->mdlId[8], HU3D_ATTR_DISPOFF);
    player = playerState->data;
    playerState->dataCnt = 0;
    playerState->emitCnt = REFRESH_RATE;
    player->scale = 100.0f;
    player->pos = *itemPosition;
}

void UpdateItemEffectParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *player;
    s32 particleIndex;
    s8 opponent;

    if (particle->dataCnt == 0) {
        particle->dataCnt = 1;
        for (player = (HU3DPARTICLEDATA *)&particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, player++) {
            player->scale = 0.0f;
        }
    }
    opponent = (1.0f/REFRESH_RATE) * (particle->emitCnt * 0xFF);
    for (player = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, player++) {
        player->color.a = opponent;
    }
    if (--particle->emitCnt == 0) {
        model->attr |= 1;
    }
    DCStoreRange(particle->data, particle->maxCnt * sizeof(HU3DPARTICLEDATA));
}

void InitializeGameStateObject(OMOBJ *object)
{
    M433GameEventState *work;

    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M433GameEventState), HU_MEMNUM_OVL);
    work = object->data;
    memset(work, 0, sizeof(M433GameEventState));
    work->uGameEvent = 0;
    work->sWinGrp = -1;
    work->uServeGrp = 0;
    work->auEvents[0] = work->auEvents[1] = 0;
    work->pointScored = work->isPlaying = 0;
    InitializeContactEvents(work->itemStartPosition, &work->scoreboardAnimation);
    object->objFunc = UpdateGameStateObject;
}

void InitializePlayerItemState(OMOBJ *player)
{
    M433GameEventState *work = player->data;
    OSReport("\n");
    OSReport("uGameEvent       %d\n", work->uGameEvent);
    OSReport("sWinGrp          %d\n", work->sWinGrp);
    OSReport("uServeGrp        %d\n", work->uServeGrp);
    OSReport("uPointGrp        %d\n", work->uPointGrp);
    OSReport("auServeCel[2]    %d,%d\n", work->auServeCel[0], work->auServeCel[1]);
    OSReport("auEvents[2]      %d,%d\n", work->auEvents[0], work->auEvents[1]);
    OSReport("uContactCnt      %d\n", work->uContactCnt);
    OSReport("sGroundGrp       %d\n", work->sGroundGrp);
    OSReport("uContactGrp      %d\n", work->uContactGrp);
    OSReport("sContactCel      %d\n", work->uContactCel);
    OSReport("uTargetGrp       %d\n", work->uTargetGrp);
    OSReport("\n");
}

void UpdateGameStateObject(OMOBJ *object)
{
    s32 playerResults[4];
    u32 player;
    s32 animationIndex;
    s32 winnerCount;
    s16 playerIndex;
    s16 targetTeam;

    M433GameEventState *work = object->data;
    s32 hasScored = 0;
    s32 opponent = 0;
    work->eventFrames++;
    work->itemStartPosition[0].flightDuration = work->playerPoints[0];
    work->itemStartPosition[1].flightDuration = work->playerPoints[1];
    if (getRoundIntroState() >= 2) {
        UpdateContactState(work->itemStartPosition, &work->scoreboardAnimation);
    }
    switch (work->uGameEvent) {
        case 0:
            work->uContactCnt = 0;
            work->uContactCel = -1;
            if (work->eventFrames == (REFRESH_RATE/5)) {
                SetMapMotion(1);
            }
            if ((work->eventFrames >= (REFRESH_RATE/2.0f)) && (getRoundIntroState() >= 2)) {
                work->uGameEvent = 1;
                work->eventFrames = 0;
                work->auEvents[0] = work->auEvents[1] = 1;
            }
            break;
        case 1:
            if (work->position != 0) {
                work->elapsedTime = 0;
                work->uContactCel = -1;
                work->uContactCnt = 0;
                work->uGameEvent = 2;
                work->eventFrames = 0;
                work->auEvents[0] = work->auEvents[1] = 3;
                if (work->uTargetGrp != 0) {
                    playerIndex = 5;
                }
                else {
                    playerIndex = 6;
                }
                SetMapMotion(playerIndex);
            }
            break;
        case 2:
        case 5:
            if (work->position != 0) {
                if (work->uContactCnt != 0) {
                    work->elapsedTime++;
                    work->uGameEvent = 3;
                    work->auEvents[work->uContactGrp] = 4;
                    if ((work->elapsedTime >= 6) && ((work->elapsedTime % 3) == 0)) {
                        SetMapMotion(3);
                    }
                }
                work->eventFrames = 0;
            }
            opponent = 1;
            break;
        case 3:
        case 4:
            if (work->position != 0) {
                if (work->uContactGrp == work->uTargetGrp) {
                    work->uGameEvent = 4;
                    work->auEvents[work->uContactGrp] = 5;
                }
                else {
                    work->uContactCel = -1;
                    work->uContactCnt = 0;
                    work->uGameEvent = 5;
                    work->auEvents[work->uContactGrp] = 3;
                    if (work->uTargetGrp != 0) {
                        targetTeam = 5;
                    }
                    else {
                        targetTeam = 6;
                    }
                    SetMapMotion(targetTeam);
                }
                work->eventFrames = 0;
            }
            opponent = 1;
            break;
        case 6:
            if (work->eventFrames >= (REFRESH_RATE*2.0f)) {
                work->uGameEvent = 0;
                work->eventFrames = 0;
                work->auEvents[0] = work->auEvents[1] = 0;
            }
            break;
        case 7:
            break;
    }
    if ((work->pointScored != 0) && (opponent != 0)) {
        if (work->sGroundGrp >= 0) {
            player = 1 - work->sGroundGrp;
            SetMapMotion(1);
        }
        else {
            player = 1 - work->uContactGrp;
            SetMapMotion(2);
        }
        work->uPointGrp = player;
        work->playerPoints[player]++;
        if (work->uServeGrp != player) {
            work->uServeGrp = player;
            work->auServeCel[work->uServeGrp] = 1 - work->auServeCel[work->uServeGrp];
        }
        if (work->playerPoints[player] >= 5) {
            work->sWinGrp = player;
            work->uGameEvent = 7;
            work->auEvents[0] = work->auEvents[1] = 7;
            winnerCount = 0;
            for (animationIndex = 0; animationIndex < 4; animationIndex++) {
                playerResults[animationIndex] = -1;
                if (work->sWinGrp == omObjGetDataAs(playerObjects[animationIndex], M433PlayerState)->teamIndex) {
                    playerResults[winnerCount++] = playerObjects[animationIndex]->work[0];
                }
            }
            SetRoundResultAndPlayers(1, playerResults[0], playerResults[1]);
            SetPlayerResultFlag(4);
        }
        else {
            work->uGameEvent = 6;
            work->auEvents[0] = work->auEvents[1] = 6;
            SetPlayerScoreState(&work->scoreboardAnimation, work->playerPoints[0], work->playerPoints[1], player);
            HuAudFXPlay(0x16);
        }
        work->eventFrames = 0;
    }
    work->pointScored = 0;
    work->isPlaying = 0;
}

u16 scoreDisplayPlayerOrder[6] = { 0, 1, 2, 3, 3, 0 };
s32 scoreSequenceIds[2] = { 0x61001E, 0x61001F };
Vec2f scoreDigitOffsets[5] = {
    { 0.0f, 0.0f },
    { 0.0f, 0.0f },
    { -23.0f, 0.0f },
    { 1.0f, 0.0f },
    { 21.0f, 0.0f },
};
Vec2f scoreDisplayOrigins[2] = { { 72.0f, 56.0f }, { 504.0f, 56.0f } };
s32 scoreDigitSpriteIds[0xB][4] = {
    { 0x13000B, 0, 0, 0 },
    { 0x13000C, 0, 0, 0 },
    { 0x13000D, 0, 0, 0 },
    { 0x13000E, 0, 0, 0 },
    { 0x13000F, 0, 0, 0 },
    { 0x130010, 0, 0, 0 },
    { 0x130011, 0, 0, 0 },
    { 0x130012, 0, 0, 0 },
    { 0x130013, 0, 0, 0 },
    { 0x130014, 0, 0, 0 },
    { 0x130099, 0, 0, 0 },
};
s32 itemHorizontalVelocities[9] = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
M433ItemActionProperties itemActionProperties[9] = {
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 1, 1, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
    { 1, 1, 0 },
    { 1, 1, 0 },
    { 1, 0, 1 },
    { 1, 0, 1 },
};
u32 itemActionStateMap[9] = { 0, 1, 2, 1, 1, 2, 1, 1, 1 };

void InitializeContactEvents(M433PlayerScoreDisplay *scoreboardPlayers, M433ScoreboardState *scoreboardAnimation)
{
    s32 digitIndex;
    s32 opponent;
    s32 spriteType;

    for (opponent = 0; opponent < 2; opponent++, scoreboardPlayers++) {
        for (digitIndex = 0; (u32)digitIndex < 5; digitIndex++) {
            switch (scoreDisplayPlayerOrder[digitIndex]) {
                case 0:
                    scoreboardPlayers->digitSpriteIds[digitIndex] = espEntry(scoreSequenceIds[opponent], 1, 0);
                    espPriSet(scoreboardPlayers->digitSpriteIds[digitIndex], 0x100);
                    break;
                case 1:
                    scoreboardPlayers->digitSpriteIds[digitIndex] = espEntry(0x610020, 1, 0);
                    espPriSet(scoreboardPlayers->digitSpriteIds[digitIndex], 0x101);
                    espColorSet(scoreboardPlayers->digitSpriteIds[digitIndex], 0, 0, 0);
                    espTPLvlSet(scoreboardPlayers->digitSpriteIds[digitIndex], 0.5);
                    break;
                case 2:
                    scoreboardPlayers->digitSpriteIds[digitIndex] = espEntry(DATA_MAKE_NUM(DATADIR_M433, 10), 1, 0);
                    espPriSet(scoreboardPlayers->digitSpriteIds[digitIndex], 0xFF);
                    break;
                case 3:
                    scoreboardPlayers->digitSpriteIds[digitIndex] = espEntry(0x610033, 1, 0);
                    espPriSet(scoreboardPlayers->digitSpriteIds[digitIndex], 0xFF);
                    break;
            }
            espDispOff(scoreboardPlayers->digitSpriteIds[digitIndex]);
            espDrawNoSet(scoreboardPlayers->digitSpriteIds[digitIndex], 0);
            espAttrSet(scoreboardPlayers->digitSpriteIds[digitIndex], HUSPR_ATTR_NOANIM);
            espAttrSet(scoreboardPlayers->digitSpriteIds[digitIndex], HUSPR_ATTR_LINEAR);
            espPosSet(
                scoreboardPlayers->digitSpriteIds[digitIndex], scoreDisplayOrigins[opponent].x + scoreDigitOffsets[digitIndex].x, scoreDisplayOrigins[opponent].y + scoreDigitOffsets[digitIndex].y);
        }
    }
    HuDataDirClose(0x610000);
    for (opponent = 0; opponent < 3; opponent++) {
        for (digitIndex = 0; digitIndex < 0x14; digitIndex++) {
            for (spriteType = 0; spriteType < 2; spriteType++) {
                if (scoreDigitSpriteIds[digitIndex % 10][spriteType] != 0) {
                    scoreboardAnimation->scoreSpriteRows[opponent][digitIndex].animationState[spriteType] = espEntry(scoreDigitSpriteIds[digitIndex % 10][spriteType], 2, 0);
                    espAttrSet(scoreboardAnimation->scoreSpriteRows[opponent][digitIndex].animationState[spriteType], HUSPR_ATTR_LINEAR);
                    espDispOff(scoreboardAnimation->scoreSpriteRows[opponent][digitIndex].animationState[spriteType]);
                }
                else {
                    scoreboardAnimation->scoreSpriteRows[opponent][digitIndex].animationState[spriteType] = -1;
                }
            }
        }
    }
    for (spriteType = 0; spriteType < 2; spriteType++) {
        if (scoreDigitSpriteIds[10][spriteType] != 0) {
            scoreboardAnimation->finalScoreSprites.animationState[spriteType] = espEntry(scoreDigitSpriteIds[10][spriteType], 2, 0);
            espAttrSet(scoreboardAnimation->finalScoreSprites.animationState[spriteType], HUSPR_ATTR_LINEAR);
            espDispOff(scoreboardAnimation->finalScoreSprites.animationState[spriteType]);
        }
        else {
            scoreboardAnimation->finalScoreSprites.animationState[spriteType] = -1;
        }
    }
    for (opponent = 0; opponent < 2; opponent++) {
        scoreboardAnimation->scoreAnimations[opponent].animationState[0] = -1;
        scoreboardAnimation->scoreAnimations[opponent].animationState[1] = -1;
    }
    scoreboardAnimation->scoreAnimations[2].animationState[0] = -1;
    scoreboardAnimation->scoreAnimations[2].animationState[1] = -1;
    scoreboardAnimation->scoreAnimations[3].animationState[0] = -1;
    scoreboardAnimation->scoreAnimations[3].animationState[1] = -1;
    HuDataDirClose(scoreDigitSpriteIds[0][0]);
}

void UpdateContactScale(M433ScoreAnimationState *scoreAnimation)
{
    float horizontalOffset;
    float verticalOffset;
    s32 player;

    for (player = 0; player < 2; player++) {
        if (scoreAnimation->digitSpriteIds[player] >= 0) {
            espDispOn(scoreAnimation->digitSpriteIds[player]);
            horizontalOffset = (scoreAnimation->digitXOffsets[player] * scoreAnimation->horizontalScale) * cosd(scoreAnimation->rotation);
            verticalOffset = (scoreAnimation->digitXOffsets[player] * scoreAnimation->horizontalScale) * sind(scoreAnimation->rotation);
            espPosSet(scoreAnimation->digitSpriteIds[player], scoreAnimation->displayX + horizontalOffset, scoreAnimation->displayY + verticalOffset);
            espZRotSet(scoreAnimation->digitSpriteIds[player], scoreAnimation->rotation);
            espScaleSet(scoreAnimation->digitSpriteIds[player], scoreAnimation->horizontalScale, scoreAnimation->verticalScale);
            espTPLvlSet(scoreAnimation->digitSpriteIds[player], scoreAnimation->opacity);
        }
    }
}

void UpdateContactEvent(M433ScoreSpriteRow *scoreSpriteRows, M433ScoreAnimationState *scoreAnimation, u32 scoreValue)
{
    u32 scoreDigitIndex;
    u32 player;

    scoreDigitIndex = scoreValue % 10;
    player = scoreValue / 10;
    if (player == 0) {
        scoreAnimation->digitSpriteIds[0] = scoreSpriteRows[scoreDigitIndex].scoreSpriteIds[0];
        scoreAnimation->digitSpriteIds[1] = -1;
        scoreAnimation->digitXOffsets[0] = 0.0f;
    }
    else {
        scoreAnimation->digitSpriteIds[0] = scoreSpriteRows[player + 0xA].scoreSpriteIds[0];
        scoreAnimation->digitSpriteIds[1] = scoreSpriteRows[scoreDigitIndex].scoreSpriteIds[0];
        scoreAnimation->digitXOffsets[0] = -23.039999f;
        scoreAnimation->digitXOffsets[1] = 23.039999f;
    }
    scoreAnimation->displayX = scoreAnimation->displayY = 0.0f;
    scoreAnimation->xVelocity = scoreAnimation->yVelocity = 0.0f;
    scoreAnimation->horizontalScale = scoreAnimation->verticalScale = 1.5f;
    scoreAnimation->opacity = 1.0f;
    scoreAnimation->rotation = 0.0f;
}

void UpdateContactState(M433PlayerScoreDisplay *scoreboardPlayers, M433ScoreboardState *scoreboardAnimation)
{
    float scoreOpacity;
    float scorePosition;
    float playerSpeed;
    float opponentSpeed;
    M433ScoreAnimationState *player;
    M433ScoreSpriteRow *scoreSprite;
    s32 opponent;
    s32 digitIndex;

    for (opponent = 0; opponent < 2; opponent++, scoreboardPlayers++) {
        for (digitIndex = 0; digitIndex < 5; digitIndex++) {
            espDispOn(scoreboardPlayers->digitSpriteIds[digitIndex]);
        }
        espBankSet(scoreboardPlayers->digitSpriteIds[3], scoreboardPlayers->score / 10);
        espBankSet(scoreboardPlayers->digitSpriteIds[4], scoreboardPlayers->score % 10);
    }
    opponentSpeed = 864.0f/(REFRESH_RATE*5);
    if (scoreboardAnimation->servingPlayer == 0) {
        player = scoreboardAnimation->scoreAnimations;
    }
    else {
        player = &scoreboardAnimation->scoreAnimations[1];
        opponentSpeed *= -1.0f;
    }
    switch (scoreboardAnimation->animationState) {
        case 0:
            break;
        case 1:
            UpdateContactEvent(&scoreboardAnimation->scoreSpriteRows[0][0], &scoreboardAnimation->scoreAnimations[0], scoreboardAnimation->previousScore);
            UpdateContactEvent(&scoreboardAnimation->scoreSpriteRows[1][0], &scoreboardAnimation->scoreAnimations[1], scoreboardAnimation->previousOpponentScore);
            UpdateContactEvent(&scoreboardAnimation->finalScoreSprites, &scoreboardAnimation->scoreAnimations[2], 0);
            UpdateContactEvent(&scoreboardAnimation->scoreSpriteRows[2][0], &scoreboardAnimation->scoreAnimations[3], scoreboardAnimation->currentScores[scoreboardAnimation->servingPlayer]);

            scorePosition = -48.0f;
            scoreboardAnimation->scoreAnimations[0].displayX = 172.8f;
            scoreboardAnimation->scoreAnimations[0].displayY = 360.0f - scorePosition;
            scoreboardAnimation->scoreAnimations[1].displayX = 403.19998f;
            scoreboardAnimation->scoreAnimations[1].displayY = 360.0f - scorePosition;
            scoreboardAnimation->scoreAnimations[2].displayX = 288.0f;
            scoreboardAnimation->scoreAnimations[2].displayY = 360.0f - scorePosition;
            scoreboardAnimation->scoreAnimations[0].opacity = scoreboardAnimation->scoreAnimations[1].opacity = scoreboardAnimation->scoreAnimations[2].opacity = 0.0f;
            scorePosition = scorePosition / (REFRESH_RATE*(1.0f/3.125f));
            scoreboardAnimation->scoreAnimations[0].yVelocity = scorePosition;
            scoreboardAnimation->scoreAnimations[1].yVelocity = scorePosition;
            scoreboardAnimation->scoreAnimations[2].yVelocity = scorePosition;
            scoreboardAnimation->scoreAnimations[3].displayX = 806.39996f;
            scoreboardAnimation->scoreAnimations[3].displayY = 144.0f;
            scoreboardAnimation->scoreAnimations[3].horizontalScale += 0.5f;
            scoreboardAnimation->scoreAnimations[3].verticalScale -= 0.5f;
            scoreboardAnimation->scoreAnimations[3].xVelocity = (403.19998f - scoreboardAnimation->scoreAnimations[3].displayX) / (REFRESH_RATE*0.4f);
            scoreboardAnimation->scoreAnimations[3].yVelocity = (360.0f - scoreboardAnimation->scoreAnimations[3].displayY) / (REFRESH_RATE*0.4f);
            if (scoreboardAnimation->servingPlayer == 0) {
                scoreboardAnimation->scoreAnimations[3].displayX = -230.40001f;
                scoreboardAnimation->scoreAnimations[3].xVelocity *= -1.0f;
            }
            scoreboardAnimation->animationState++;
            scoreboardAnimation->animationFrame = 0;
            break;
        case 2:
            scoreOpacity = scoreboardAnimation->scoreAnimations[0].opacity;
            scoreOpacity += ((10.0f/3.0f)/REFRESH_RATE);
            if (scoreOpacity > 0.95f) {
                scoreOpacity = 1.0f;
            }
            scoreboardAnimation->scoreAnimations[2].opacity = scoreOpacity;
            scoreboardAnimation->scoreAnimations[1].opacity = scoreOpacity;
            scoreboardAnimation->scoreAnimations[0].opacity = scoreOpacity;
            scoreboardAnimation->scoreAnimations[0].displayY += scoreboardAnimation->scoreAnimations[0].yVelocity;
            scoreboardAnimation->scoreAnimations[1].displayY += scoreboardAnimation->scoreAnimations[1].yVelocity;
            scoreboardAnimation->scoreAnimations[2].displayY += scoreboardAnimation->scoreAnimations[2].yVelocity;
            scoreboardAnimation->scoreAnimations[3].displayX += scoreboardAnimation->scoreAnimations[3].xVelocity;
            scoreboardAnimation->scoreAnimations[3].displayY += scoreboardAnimation->scoreAnimations[3].yVelocity;
            if (scoreboardAnimation->animationFrame >= (REFRESH_RATE*(1.0f/3.125f))) {
                player->xVelocity = opponentSpeed;
                player->yVelocity = (-384.0f/REFRESH_RATE)*2.5f;
                scoreboardAnimation->animationState++;
                scoreboardAnimation->animationTimer = 0;
            }
            break;
        case 3:
            if (scoreboardAnimation->animationFrame <= (REFRESH_RATE*0.4f)) {
                scoreboardAnimation->scoreAnimations[3].displayX += scoreboardAnimation->scoreAnimations[3].xVelocity;
                scoreboardAnimation->scoreAnimations[3].displayY += scoreboardAnimation->scoreAnimations[3].yVelocity;
            }
            if (scoreboardAnimation->animationTimer < (REFRESH_RATE*0.4f)) {
                scoreboardAnimation->animationTimer++;
                scoreOpacity = (2025.0f/REFRESH_RATE) * scoreboardAnimation->animationTimer;
                playerSpeed = (2.5f/REFRESH_RATE) * scoreboardAnimation->animationTimer;
                playerSpeed = 0.5f * (1.0f - (playerSpeed * playerSpeed));
                if (scoreboardAnimation->animationTimer == (REFRESH_RATE*0.4f)) {
                    playerSpeed = 0.0f;
                }
                scoreboardAnimation->scoreAnimations[3].horizontalScale = 1.5 + (playerSpeed * cosd(scoreOpacity));
                scoreboardAnimation->scoreAnimations[3].verticalScale = 1.5 + (playerSpeed * sind((scoreOpacity - 90.0f)));
            }
            player->horizontalScale = scoreboardAnimation->scoreAnimations[3].verticalScale;
            player->verticalScale = scoreboardAnimation->scoreAnimations[3].horizontalScale;
            player->displayX += player->xVelocity;
            player->displayY += player->yVelocity;
            player->yVelocity += 2880.0f*(1.0f/REFRESH_RATE)*(1.0f/REFRESH_RATE);
            player->rotation += 500.0f*(1.0f/REFRESH_RATE);
            player->opacity -= 1.07f*(1.0f/REFRESH_RATE);
            if (player->opacity < 0.0f) {
                player->opacity = 0.0f;
            }
            if (GetActiveContactGroup() == 0) {
                scoreboardAnimation->animationState++;
                scoreboardAnimation->animationFrame = 0;
            }
            break;
        case 4:
            scoreOpacity = scoreboardAnimation->scoreAnimations[3].opacity;
            scoreOpacity -= ((10.0f/3.0f)/REFRESH_RATE);
            if (scoreOpacity < 0.05f) {
                scoreOpacity = 0.0f;
                scoreboardAnimation->animationState++;
                scoreboardAnimation->animationFrame = 0;
            }
            scoreboardAnimation->scoreAnimations[3].opacity = scoreOpacity;
            scoreboardAnimation->scoreAnimations[2].opacity = scoreOpacity;
            scoreboardAnimation->scoreAnimations[1].opacity = scoreOpacity;
            scoreboardAnimation->scoreAnimations[0].opacity = scoreOpacity;
            break;
        default:
            for (opponent = 0; opponent < 2; opponent++) {
                scoreboardAnimation->scoreAnimations[opponent].digitSpriteIds[0] = -1;
                scoreboardAnimation->scoreAnimations[opponent].digitSpriteIds[1] = -1;
            }
            scoreboardAnimation->scoreAnimations[2].digitSpriteIds[0] = -1;
            scoreboardAnimation->scoreAnimations[2].digitSpriteIds[1] = -1;
            scoreboardAnimation->scoreAnimations[3].digitSpriteIds[0] = -1;
            scoreboardAnimation->scoreAnimations[3].digitSpriteIds[1] = -1;
            scoreboardAnimation->animationState = 0;
            break;
    }
    for (opponent = 0; opponent < 3; opponent++) {
        scoreSprite = &scoreboardAnimation->scoreSpriteRows[opponent][0];
        for (digitIndex = 0; digitIndex < 0x14; digitIndex++, scoreSprite++) {
            if (scoreSprite->scoreSpriteIds[0] >= 0) {
                espDispOff(scoreSprite->scoreSpriteIds[0]);
            }
            if (scoreSprite->scoreSpriteIds[1] >= 0) {
                espDispOff(scoreSprite->scoreSpriteIds[1]);
            }
        }
    }
    scoreSprite = &scoreboardAnimation->finalScoreSprites;
    if (scoreSprite->scoreSpriteIds[0] >= 0) {
        espDispOff(scoreSprite->scoreSpriteIds[0]);
    }
    if (scoreSprite->scoreSpriteIds[1] >= 0) {
        espDispOff(scoreSprite->scoreSpriteIds[1]);
    }
    UpdateContactScale(&scoreboardAnimation->scoreAnimations[0]);
    UpdateContactScale(&scoreboardAnimation->scoreAnimations[1]);
    UpdateContactScale(&scoreboardAnimation->scoreAnimations[2]);
    UpdateContactScale(&scoreboardAnimation->scoreAnimations[3]);
    scoreboardAnimation->animationFrame++;
}

void SetPlayerScoreState(M433ScoreboardState *scoreboardAnimation, s32 playerScore, s32 opponentScore, u32 servingPlayer)
{
    scoreboardAnimation->animationState = 1;
    scoreboardAnimation->previousScore = scoreboardAnimation->currentScores[0];
    scoreboardAnimation->previousOpponentScore = scoreboardAnimation->currentScores[1];
    scoreboardAnimation->currentScores[0] = playerScore;
    scoreboardAnimation->currentScores[1] = opponentScore;
    scoreboardAnimation->servingPlayer = servingPlayer;
}

void SetPlayerItemState(Vec *itemPosition, u32 itemType, u32 contactGroup, s32 contactCell)
{
    M433ItemManagerState *gameState = itemManagerObject->data;
    gameState->horizontalVelocity = 0;
    if (itemType < 9) {
        gameState->horizontalVelocity = itemHorizontalVelocities[itemType];
        if (itemActionProperties[itemType].startsContactEvent != 0) {
            M433GameEventState *player = gameEventObject->data;
            player->position = 1;
            player->uContactGrp = contactGroup;
            player->uContactCel = contactCell;
            player->uTargetGrp = contactGroup;
            if (itemActionProperties[itemType].switchesTargetGroup != 0) {
                player->uTargetGrp = 1 - player->uTargetGrp;
            }
            if (itemActionProperties[itemType].countsContact != 0) {
                player->uContactCnt++;
            }
        }
    }
    if (itemType == 5) {
        s32 opponent = 6;
        if (gameState->itemType == 5) {
            opponent++;
        }
        Hu3DModelPosSetV(itemManagerObject->mdlId[opponent], &gameState->position);
        Hu3DModelAttrReset(itemManagerObject->mdlId[opponent], HU3D_ATTR_DISPOFF);
        Hu3DMotionTimeSet(itemManagerObject->mdlId[opponent], 0.0f);
        Hu3DMotionShapeTimeSet(itemManagerObject->mdlId[opponent], 0.0f);
    }
    else if (itemType != 2) {
        gameState->itemSpinSpeed = 1.0f;
    }
    gameState->itemOutcome = 1;
    gameState->horizontalAcceleration = 0;
    gameState->elapsedTime = 0;
    gameState->itemActionState = 0;
    gameState->itemType = itemType;
    gameState->itemAction = contactGroup;
    gameState->itemSubAction = contactCell;
    if (itemActionStateMap[itemType] != 0) {
        gameState->playerActionState = itemActionStateMap[itemType];
    }
    else if (gameState->playerActionState != 0) {
        gameState->playerActionState--;
    }
    gameState->targetPosition = *itemPosition;
    if (itemType == 0) {
        gameState->position = gameState->targetPosition;
    }
    gameState->itemPhysics.flightDuration = 1.0f;
    gameState->itemPhysics.flightDuration = 0.0f;
}

void ClampPlayerPosition(Vec *position, u32 playerSide)
{
    float edgeClearance = 44.0f;
    if ((position->z - edgeClearance) < -500.0f) {
        position->z = -500.0f + edgeClearance;
    }
    else if ((position->z + edgeClearance) > 500.0f) {
        position->z = 500.0f - edgeClearance;
    }
    if (playerSide == 0) {
        position->x *= -1.0f;
    }
    if ((position->x - edgeClearance) < 20.0f) {
        position->x = 20.0f + edgeClearance;
    }
    else if ((position->x + edgeClearance) > 800.0f) {
        position->x = 800.0f - edgeClearance;
    }
    if (playerSide == 0) {
        position->x *= -1.0f;
    }
}

void UpdateItemMotion(float playerSpeed, float depthDifference, u32 playerSide, u32 playImpactSound, Vec *itemPosition)
{
    M433ItemManagerState *itemState = itemManagerObject->data;
    if (depthDifference > 1.0f) {
        depthDifference = 1.0f;
    }
    if (itemState->itemSpinState == 0) {
        itemState->itemPosition.x = 465.0f;
        itemState->itemPosition.y = 11.0f;
        itemState->itemPosition.z = 0.0f;
        if (playerSide != 0) {
            itemState->itemPosition.x *= -1.0f;
        }
        itemState->itemSpinState = 1;
        itemState->itemSpinScale = 0;
        itemState->itemWorldOffset = 0.0f;
        itemState->itemModelState = 0;
        itemState->itemSpinSpeed = 1.0f;
    }
    if (playImpactSound != 0) {
        itemState->itemSpinState = 2;
        depthDifference *= 1.5f;
    }
    itemState->itemEffectState = 1;
    itemState->itemPosition.x += (1.0f/REFRESH_RATE) * (800.0 * (depthDifference * sind(playerSpeed)));
    itemState->itemPosition.z += (1.0f/REFRESH_RATE) * (800.0 * (depthDifference * cosd(playerSpeed)));
    if (playerSide != 0) {
        itemState->itemPosition.x *= -1.0f;
    }
    if (itemState->itemPosition.x < 230.0f) {
        itemState->itemPosition.x = 230.0f;
    }
    if (playerSide != 0) {
        itemState->itemPosition.x *= -1.0f;
    }
    ClampPlayerPosition(&itemState->itemPosition, 1 - playerSide);
    *itemPosition = itemState->itemPosition;
}

s32 NormalizeItemPosition(Vec *normalizedPosition)
{
    M433ItemManagerState *work = itemManagerObject->data;

    normalizedPosition->x = normalizedPosition->y = normalizedPosition->z = 0.0f;
    if (work->itemSpinState == 0) {
        return 0;
    }
    *normalizedPosition = work->itemPosition;
    normalizedPosition->x = fabs(normalizedPosition->x);
    normalizedPosition->x -= 465.0f;
    normalizedPosition->x *= 0.004255319f;
    normalizedPosition->z *= 0.0028571428f;
    return 1;
}

void ResetItemState(void)
{
    M433ItemManagerState *work = itemManagerObject->data;
    work->itemSpinSpeed = 1.0f;
}

u32 GetItemTargetPosition(Vec *targetPosition)
{
    u32 player;
    M433ItemManagerState *work = itemManagerObject->data;
    *targetPosition = work->targetPosition;
    if (work->targetPosition.x >= 0.0f) {
        player = 1;
    }
    else {
        player = 0;
    }
    return player;
}

u32 GetItemPosition(Vec *itemPosition)
{
    u32 player;
    M433ItemManagerState *itemState = itemManagerObject->data;
    if (itemState->itemSpinState != 0) {
        *itemPosition = itemState->itemPosition;
    }
    else {
        *itemPosition = itemState->targetPosition;
    }
    if (itemPosition->x >= 0.0f) {
        player = 1;
    }
    else {
        player = 0;
    }
    return player;
}

float GetItemVerticalSpeed(void)
{
    M433ItemManagerState *itemState = itemManagerObject->data;
    return itemState->itemPhysics.flightDuration - itemState->itemPhysics.elapsedTime;
}

float GetItemVerticalOffset(void)
{
    M433ItemManagerState *itemState = itemManagerObject->data;
    if (itemState->itemSpinState != 0) {
        return 1.5f;
    }
    return itemState->itemPhysics.flightDuration - itemState->itemPhysics.elapsedTime;
}

u32 GetItemState(void)
{
    M433ItemManagerState *itemState = itemManagerObject->data;
    if (itemState->itemSpinState != 0) {
        return itemState->itemSpinScale;
    }
    return 0x2710;
}

u32 GetItemAction(void)
{
    M433ItemManagerState *itemState = itemManagerObject->data;
    return itemState->itemModelState;
}

float CalculateItemFlightTime(float targetHeight)
{
    float verticalTravelTime;
    float inverseGravity;
    float heightAtInitialVelocity;
    M433ItemTrajectory *physics;
    M433ItemManagerState *itemState;

    itemState = itemManagerObject->data;
    physics = &itemState->itemPhysics;
    inverseGravity = 1.0f / physics->verticalAcceleration;
    verticalTravelTime = -physics->verticalVelocity * inverseGravity;
    heightAtInitialVelocity = physics->startPosition.y + ((verticalTravelTime * (0.5f * physics->verticalAcceleration * verticalTravelTime)) + (physics->verticalVelocity * verticalTravelTime));
    if (targetHeight > (heightAtInitialVelocity)) {
        return -1.0f;
    }
    verticalTravelTime = inverseGravity * (-physics->verticalVelocity - sqrtf((physics->verticalVelocity * physics->verticalVelocity) - (2.0f * physics->verticalAcceleration * (physics->startPosition.y - targetHeight))));
    verticalTravelTime -= physics->elapsedTime;
    return verticalTravelTime;
}

float UpdateItemFlightPosition(float elapsedTimeDelta, Vec *itemPosition)
{
    Vec positionDelta;
    Vec horizontalDirection;
    Vec previousPosition;
    float newElapsedTime;
    float previousElapsedTime;
    float horizontalDisplacement;
    float verticalDisplacement;
    M433ItemTrajectory *physics;
    M433ItemManagerState *itemState;

    itemState = itemManagerObject->data;
    physics = &itemState->itemPhysics;
    previousElapsedTime = physics->elapsedTime;
    newElapsedTime = previousElapsedTime + elapsedTimeDelta;
    if (newElapsedTime < 0.0f) {
        newElapsedTime = 0.0f;
    }
    if (newElapsedTime > physics->flightDuration) {
        newElapsedTime = physics->flightDuration;
    }
    physics->elapsedTime = newElapsedTime;
    if (physics->horizontalAcceleration > 0.0f) {
        VECSubtract(&physics->targetPosition, &physics->startPosition, &horizontalDirection);
        horizontalDirection.y = 0.0f;
        VECNormalize(&horizontalDirection, &horizontalDirection);
    }
    else {
        horizontalDirection.x = horizontalDirection.y = horizontalDirection.z = 0.0f;
    }
    previousPosition = *itemPosition;
    horizontalDisplacement = (newElapsedTime * (0.5f * physics->horizontalVelocity * newElapsedTime)) + (physics->horizontalVelocity * newElapsedTime);
    verticalDisplacement = (newElapsedTime * (0.5f * physics->verticalAcceleration * newElapsedTime)) + (physics->verticalVelocity * newElapsedTime);
    itemPosition->x = physics->startPosition.x + (horizontalDirection.x * horizontalDisplacement);
    itemPosition->y = physics->startPosition.y + verticalDisplacement;
    itemPosition->z = physics->startPosition.z + (horizontalDirection.z * horizontalDisplacement);
    VECSubtract(itemPosition, &previousPosition, &positionDelta);
    newElapsedTime >= physics->flightDuration;
    physics->elapsedTime = previousElapsedTime;
    return previousElapsedTime + elapsedTimeDelta;
}

static inline float GetItemSoundPositionInline(Vec *soundPosition)
{
    M433ItemManagerState *itemState = itemManagerObject->data;
    if (itemState->itemSpinState != 0) {
        *soundPosition = itemState->itemPosition;
    }
    else {
        *soundPosition = itemState->targetPosition;
    }
}

s32 GetItemSoundPosition(Vec *soundIds)
{
    Vec crossingDelta;
    Vec targetPosition;
    float crossingRatio;
    float unusedInitializationValue;
    M433ItemManagerState *itemState;
    M433ItemManagerState *unusedItemState;

    itemState = itemManagerObject->data;
    GetItemSoundPositionInline(&targetPosition);
    VECSubtract(&targetPosition, &itemState->position, &crossingDelta);
    if ((targetPosition.x * itemState->position.x) >= 0.0f) {
        return 0;
    }
    if ((itemState->position.x == 0.0f) || (crossingDelta.x == 0.0f)) {
        return 0;
    }
    crossingRatio = fabs(itemState->position.x / crossingDelta.x);
    VECScale(&crossingDelta, &crossingDelta, crossingRatio);
    VECAdd(&itemState->position, &crossingDelta, soundIds);
    return 1;
}

u32 GetActiveContactGroup(void)
{
    M433GameEventState *gameState = gameEventObject->data;
    return gameState->uGameEvent;
}

u32 GetPlayerGamePhase(u32 teamIndex)
{
    M433GameEventState *work = gameEventObject->data;
    return work->auEvents[teamIndex & 1];
}

s32 IsPlayerServing(u32 teamIndex)
{
    M433GameEventState *gameState = gameEventObject->data;
    return gameState->uServeGrp == (teamIndex & 1);
}

u32 GetPlayerServeCell(u32 teamIndex)
{
    M433GameEventState *gameState = gameEventObject->data;
    return gameState->auServeCel[teamIndex & 1];
}

s32 IsPlayerContacting(u32 teamIndex)
{
    M433GameEventState *work = gameEventObject->data;
    return work->uPointGrp == (teamIndex & 1);
}

s32 IsPlayerWinner(s32 teamIndex)
{
    M433GameEventState *work = gameEventObject->data;
    return work->sWinGrp == (teamIndex & 1);
}

u32 GetPlayerPointGroup(void)
{
    M433GameEventState *gameState = gameEventObject->data;
    return gameState->uContactCnt;
}

s32 IsPlayerContact(u32 teamIndex, s32 contactCell)
{
    M433GameEventState *work = gameEventObject->data;
    return !((work->uContactGrp == (teamIndex & 1)) && (work->uContactCel == (contactCell & 1)));
}

s32 IsPlayerContactGroup(u32 teamIndex)
{
    M433GameEventState *work = gameEventObject->data;
    return work->uContactGrp == (teamIndex & 1);
}

float ProjectPointOntoLine(Vec *lineStart, Vec *lineEnd, Vec *point)
{
    Vec movementDelta;
    float projectionNumerator;
    float negativeLineLengthSquared;

    VECSubtract(lineEnd, lineStart, &movementDelta);
    projectionNumerator = VECDotProduct(&movementDelta, point);
    negativeLineLengthSquared = -VECSquareMag(point);
    if (negativeLineLengthSquared != 0.0f) {
        projectionNumerator /= negativeLineLengthSquared;
    }
    return projectionNumerator;
}

float FindClosestPointOnSegments(Vec *firstSegmentStart, Vec *firstSegmentEnd, Vec *secondSegmentStart, Vec *secondSegmentEnd)
{
    Vec closestPoint;
    Vec firstLineDirection;
    Vec targetPosition;
    Vec movementDelta;
    float firstLineParameter;
    float secondLineParameter;
    float closestPointDistance;
    float lineSeparation;

    VECSubtract(firstSegmentEnd, firstSegmentStart, &firstLineDirection);
    VECSubtract(secondSegmentEnd, secondSegmentStart, &targetPosition);
    VECSubtract(&firstLineDirection, &targetPosition, &firstLineDirection);
    if (VECMag(&firstLineDirection) <= 0.0f) {
        closestPoint = *firstSegmentStart;
    }
    else {
        VECSubtract(firstSegmentStart, secondSegmentStart, &movementDelta);
        secondLineParameter = VECDotProduct(&movementDelta, &firstLineDirection);
        closestPointDistance = -VECSquareMag(&firstLineDirection);
        if (closestPointDistance != 0.0f) {
            secondLineParameter /= closestPointDistance;
        }
        lineSeparation = secondLineParameter;
        firstLineParameter = lineSeparation;
        if (firstLineParameter < 0.0f) {
            firstLineParameter = 0.0f;
        }
        else if (firstLineParameter > 1.0f) {
            firstLineParameter = 1.0f;
        }
        VECScale(&firstLineDirection, &targetPosition, firstLineParameter);
        VECAdd(firstSegmentStart, &targetPosition, &closestPoint);
    }
    VECSubtract(secondSegmentStart, &closestPoint, &firstLineDirection);
    return VECMag(&firstLineDirection);
}

#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/esprite.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"

#include "ext_math.h"
#include "version.h"
#include <string.h>

#ifndef __MWERKS__
s32 rand8(void);
#endif

typedef struct {
    /* 0x00 */ s32 modelIndex;
    /* 0x04 */ u32 cooldownFrames;
} ModelCooldownEntry; // Size 4

typedef struct {
    /* 0x00 */ s32 positionCount;
    /* 0x04 */ float positions[3];
} PlayerStartPosition; // Size 0x10

typedef struct {
    /* 0x00 */ s32 playerIndex;
    /* 0x04 */ s32 controllerConfigIndex;
    /* 0x08 */ s32 characterId;
    /* 0x0C */ s32 controllerPort;
    /* 0x10 */ s32 aiDifficulty;
    /* 0x14 */ s32 randomSeed;
    /* 0x18 */ s32 currentMotionIndex;
    /* 0x1C */ s32 forcedMotionIndex;
    /* 0x20 */ s32 playerActionState;
    /* 0x24 */ s32 actionTimer;
    /* 0x28 */ float movementSpeed;
    /* 0x2C */ float movementVelocityX;
    /* 0x30 */ float verticalVelocity;
    /* 0x34 */ float movementVelocityZ;
    /* 0x38 */ float desiredStickX;
    /* 0x3C */ float desiredStickY;
    /* 0x40 */ s32 repeatedInputFrames;
} PlayerObjectState; // Size 0x44

typedef struct {
    /* 0x00 */ s32 pieceTypeIndex;
    /* 0x04 */ s32 pieceState;
    /* 0x08 */ u32 animationFrame;
    /* 0x0C */ float positionX;
    /* 0x10 */ float positionY;
    /* 0x14 */ float positionZ;
    /* 0x18 */ float positionDeltaX;
    /* 0x1C */ float positionDeltaY;
    /* 0x20 */ float positionDeltaZ;
    /* 0x24 */ float rotationY;
    /* 0x28 */ char reservedBytes[4];
    /* 0x2C */ s32 drawDelayFrames;
} ArenaPieceState; // Size 0x30

void DestroyPlayerObjects(OMOBJ *object);

ModelCooldownEntry temporaryModelCooldowns[3][8];
PlayerStartPosition playerStartPositions[11];
OMOBJ *minigameExitObject;
OMOBJ *viewExitObject;
OMOBJ *playerObjects[4];
OMOBJ *arenaObjects;
s32 roundPhase;
HUPROCESS *objectManager;
s16 roundSequenceId;
s16 roundResultSequenceId;
s32 roundCountdown;
s32 globalLightId;
s32 roundState;
s32 unusedStateValue; // unused
s32 roundWinner;
s32 roundScore;
s32 unusedCounter; // unused
s32 frameCounter;
float roundTimer;
float alternateRoundTimer;
float playerMoveSpeed;

const float arenaPieceCollisionRadii[12] = { 25.0f, 95.0f, 25.0f, 75.0f, 75.0f, 75.0f, 35.0f, 35.0f, 35.0f, 25.0f, 50.0f, 50.0f };

const float arenaPieceCollisionHalfWidths[12] = { 75.0f, 0.0f, 75.0f, 0.0f, 0.0f, 0.0f, 20.0f, 20.0f, 20.0f, 50.0f, 0.0f, 0.0f };

const float arenaPieceCollisionHalfDepths[12] = { 75.0f, 0.0f, 37.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 70.0f, 0.0f, 0.0f };

const float arenaPieceVerticalOffsets[12] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 75.0f, 50.0f, 0.0f };

const float zeroArenaPieceOffsets[12] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

const float arenaPieceBaseHeights[12] = { 30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 30.0f, 40.0f, 40.0f, 40.0f, 0.0f, 0.0f, 0.0f };

const float arenaPieceRaisedHeights[12] = { 290.0f, 290.0f, 290.0f, 370.0f, 370.0f, 370.0f, 450.0f, 450.0f, 450.0f, 0.0f, 0.0f, 0.0f };

s32 arenaPieceResourceIds[] = { DATA_MAKE_NUM(DATADIR_MGCONST, 41), DATA_MAKE_NUM(DATADIR_MGCONST, 40), DATA_MAKE_NUM(DATADIR_MGCONST, 51),
    DATA_MAKE_NUM(DATADIR_MGCONST, 51), DATA_MAKE_NUM(DATADIR_MGCONST, 8), DATA_MAKE_NUM(DATADIR_M422, 51), DATA_MAKE_NUM(DATADIR_MGCONST, 0),
    DATA_MAKE_NUM(DATADIR_MGCONST, 37), DATA_MAKE_NUM(DATADIR_MGCONST, 36), DATA_MAKE_NUM(DATADIR_MGCONST, 51), DATA_MAKE_NUM(DATADIR_MGCONST, 51),
    DATA_MAKE_NUM(DATADIR_MGCONST, 8), DATA_MAKE_NUM(DATADIR_M422, 51), DATA_MAKE_NUM(DATADIR_MGCONST, 12), DATA_MAKE_NUM(DATADIR_MGCONST, 12),
    DATA_MAKE_NUM(DATADIR_MGCONST, 12) };

s32 arenaPieceSprites[] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x005E0000, 0x00190000, 0x006C0000, 0x00890000, 0x00840000, 0x00100000,
    0x000C0000, 0x00800000 };

s32 playerMotionResourceIds[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), DATA_MAKE_NUM(DATADIR_MARIOMOT, 2), DATA_MAKE_NUM(DATADIR_MARIOMOT, 3),
    DATA_MAKE_NUM(DATADIR_M422, 0), DATA_MAKE_NUM(DATADIR_M422, 8), DATA_MAKE_NUM(DATADIR_MARIOMOT, 6), DATA_MAKE_NUM(DATADIR_MARIOMOT, 23),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 24) };

u32 playerMotionCharacterOffsets[] = { 0, 0, 0, 1, 1, 0, 0, 0 };
u8 playerMotionLoopFlags[] = { 0, 0, 0, 1, 0, 1, 1 };
u8 playerMotionDurations[] = { 8, 8, 8, 0, 12, 0, 8, 8, 0 };
ArenaPieceState arenaPieceStates[12][10] = { 0 };

char *arenaPieceJointNames[]
    = { "m422al01-cube8", "m422al02-cyl143", "m422al03-cube11", "m422am01-cyl76", "m422am02-cube13", "m422am03-cube12", "m422as01-cube9",
          "m422test11-cube9", "m422test12-cube9", "m422m09m-m422m09m", "coin", "m422a04-sphere2", "m422m09m-m422m09m", "m422m09m-m422m09m",
          "m422m09m-m422m09m", "m422m09m-m422m09m", "m422m09m-m422m09m", "m422m09m-m422m09m", "m422m09m-m422m09m", "m422m09m-m422m09m" };

s32 playerRoundScores[11] = { 0 };
s32 roundWinCounts[2] = { 0 };
s32 roundMusicSequenceId = -1;
s32 roundSoundSequenceId = -1;

float playerSpawnTransforms[][4]
    = { { 0.0f, 300.0f, 450.0f, 180.0f }, { -200.0f, 0.0f, 1050.0f, 180.0f }, { 0.0f, 0.0f, 1050.0f, 180.0f }, { 200.0f, 0.0f, 1050.0f, 180.0f } };

s32 playerCollectedCoins[] = { 0, 0 };

float unusedScaleValue = 1.0f; // unused
s32 bonusCoinWinner = -1;

void UpdateDebugCamera(void)
{
    Center.x += 10.0 * (((HuPadBtn[0] & 2) != 0) - ((HuPadBtn[0] & 1) != 0));
    Center.z -= 10.0 * (((HuPadBtn[0] & 8) != 0) - ((HuPadBtn[0] & 4) != 0));
    Center.y += ((HuPadTrigL[0] - HuPadTrigR[0]) / 128.0) * 30.0;
    CRot.y += HuPadSubStkX[0] / 16.0;
    CRot.x += HuPadSubStkY[0] / -16.0;
}

void UpdatePlayerObject(OMOBJ *object)
{
    Mtx rotationMatrix;
    s32 unusedLocalValue1CC;
    s32 unusedLocalValue1C8;
    s32 unusedLocalValue1A4;
    float negativeTargetZOffsets;
    float movementSpeed;
    float targetStickX;
    float targetStickY;
    float targetAngle;
    float targetDistance;
    s32 arenaModelIndex;
    float playerDepthOffset;
    float targetHorizontalOffset;
    float horizontalBlend;
    float centerBlend;
    float previousHeight;
    float previousDepth;
    float playerSeparation;
    s32 pieceModelIndex;
    float pieceDistance;
    float collisionPointX;
    float collisionPointZ;
    s32 selectedLaneIndex;
    s32 playerIndexCopy;
    float stickDeltaX;
    float stickDeltaY;
    float targetDeltaX;
    float targetDeltaZ;
    float otherPlayerDeltaX;
    float pieceDeltaX;
    float otherPlayerDeltaZ;
    float pieceDeltaZ;
    ArenaPieceState *arenaPiece;
    ArenaPieceState *nearbyArenaPiece;
    PlayerObjectState *playerState;
    s32 alternateLaneIndex;
    s32 scanIndex;
    s32 targetLaneIndex;
    s32 candidateLaneIndex;
    s32 otherPlayerIndex;
    s16 motionIndex;
    s32 arenaRowIndex;
    s32 arenaColumnIndex;
    s32 animationVariant;
    s32 loopIndex;

    playerState = object->data;
    movementSpeed = 0.0f;
    targetStickX = 0.0f;
    targetStickY = 0.0f;
    motionIndex = -1;
    if (playerState->playerActionState == 0 || playerState->playerActionState == 1) {
        object->trans.z += playerMoveSpeed;
    }
    playerState->movementVelocityX = playerState->movementVelocityZ = 0.0f;
    if (roundPhase != 1004 || !(roundTimer < 45.0f)) {
        if (roundPhase >= 1004 && playerState->playerActionState != 0) {
            targetDeltaX = playerSpawnTransforms[playerState->playerIndex][0] - object->trans.x;
            targetDeltaZ = playerSpawnTransforms[playerState->playerIndex][2] - object->trans.z;
            targetAngle = 270.0 + atan2d(targetDeltaX, targetDeltaZ) - CRot.y;
            targetDistance = sqrtf(targetDeltaX * targetDeltaX + targetDeltaZ * targetDeltaZ);
            targetDistance = (targetDistance > 100.0f) ? 72.0f : (targetDistance < 12.0f) ? 0.0f : (72.0f * (targetDistance / 100.0f));
            targetStickX = targetDistance * cosd(targetAngle);
            targetStickY = targetDistance * sind(targetAngle);
            if (targetDistance == 0.0f) {
                playerState->playerActionState = 0;
            }
        }
        else if (playerState->playerActionState == 1) {
            if (playerState->controllerPort != -1) {
                targetStickX = HuPadStkX[playerState->controllerPort];
                targetStickY = HuPadStkY[playerState->controllerPort];
            }
            else {
                float playerLaneHeights[7] = { 0 };
                float opponentLaneHeights[7] = { 0 };
                float playerLaneDepthOffsets[] = { -400.0f, -550.0f, -700.0f, -800.0f };
                float opponentLaneDepthOffsets[] = { -800.0f, -900.0f, -1100.0f, -2100.0f };
                Vec pieceWorldPosition;

                targetStickX = targetStickY = 0.0f;
                for (loopIndex = 0; loopIndex < 7; loopIndex++) {
                    playerLaneHeights[loopIndex] = (playerState->playerIndex == 0 ? 600.0f : 1400.0f) + playerLaneDepthOffsets[playerState->aiDifficulty];
                    opponentLaneHeights[loopIndex] = (playerState->playerIndex == 0 ? 600.0f : 1400.0f) + playerLaneDepthOffsets[playerState->aiDifficulty];
                }
                for (loopIndex = 0; loopIndex < 12; loopIndex++) {
                    for (scanIndex = 0; scanIndex < (loopIndex < 10 ? 4 : 10); scanIndex++) {
                        arenaPiece = &arenaPieceStates[loopIndex][scanIndex];
                        arenaModelIndex = (loopIndex < 10) ? (loopIndex * 4 + 43 + scanIndex) : ((loopIndex - 10) * 10 + 83 + scanIndex);
                        if (arenaPiece->pieceState != 0 && !(arenaPiece->positionZ < 0.0f)) {
                            Hu3DModelObjPosGet(arenaObjects->mdlId[arenaModelIndex], arenaPieceJointNames[arenaPiece->pieceTypeIndex], &pieceWorldPosition);
                            targetLaneIndex = (s32)(300.0f + pieceWorldPosition.x + 2000.0f) / 100 - 20;
                            targetLaneIndex--;
                            for (candidateLaneIndex = 0; candidateLaneIndex < 3; targetLaneIndex++, candidateLaneIndex++) {
                                if (targetLaneIndex >= 0 && targetLaneIndex < 7
                                    && object->trans.z + 50.0f >= pieceWorldPosition.z - arenaPieceCollisionRadii[arenaPiece->pieceTypeIndex] - arenaPieceCollisionHalfDepths[arenaPiece->pieceTypeIndex]
                                    && pieceWorldPosition.x - arenaPieceCollisionRadii[arenaPiece->pieceTypeIndex] - arenaPieceCollisionHalfDepths[arenaPiece->pieceTypeIndex] - 50.0f < (targetLaneIndex - 3) * 100
                                    && pieceWorldPosition.x + arenaPieceCollisionRadii[arenaPiece->pieceTypeIndex] + arenaPieceCollisionHalfDepths[arenaPiece->pieceTypeIndex] + 50.0f > (targetLaneIndex - 3) * 100) {
                                    if (arenaPiece->pieceTypeIndex < 10) {
                                        if (playerLaneHeights[targetLaneIndex] < pieceWorldPosition.z + arenaPieceCollisionRadii[arenaPiece->pieceTypeIndex] + arenaPieceCollisionHalfDepths[arenaPiece->pieceTypeIndex]) {
                                            playerLaneHeights[targetLaneIndex] = pieceWorldPosition.z + arenaPieceCollisionRadii[arenaPiece->pieceTypeIndex] + arenaPieceCollisionHalfDepths[arenaPiece->pieceTypeIndex];
                                        }
                                    }
                                    else {
                                        if (opponentLaneHeights[targetLaneIndex] < pieceWorldPosition.z + arenaPieceCollisionRadii[arenaPiece->pieceTypeIndex] + arenaPieceCollisionHalfDepths[arenaPiece->pieceTypeIndex]) {
                                            opponentLaneHeights[targetLaneIndex] = pieceWorldPosition.z + arenaPieceCollisionRadii[arenaPiece->pieceTypeIndex] + arenaPieceCollisionHalfDepths[arenaPiece->pieceTypeIndex]
                                                + (arenaPiece->pieceTypeIndex < 11);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                targetLaneIndex = (s32)(300.0f + object->trans.x + 2000.0f) / 100 - 20;
                targetLaneIndex = (targetLaneIndex < 0) ? 0 : (targetLaneIndex >= 7) ? 6 : targetLaneIndex;
                for (loopIndex = 0, candidateLaneIndex = targetLaneIndex; loopIndex < 7; loopIndex++) {
                    alternateLaneIndex = ((playerState->playerIndex / 2) % 2 == 0) ? loopIndex : (6 - loopIndex);
                    if (opponentLaneHeights[candidateLaneIndex] - 60.0 * fabs(candidateLaneIndex - targetLaneIndex) < opponentLaneHeights[alternateLaneIndex] - 60.0 * fabs(alternateLaneIndex - targetLaneIndex)
                        && playerLaneHeights[alternateLaneIndex] < opponentLaneHeights[alternateLaneIndex]) {
                        candidateLaneIndex = alternateLaneIndex;
                    }
                }
                playerLaneHeights[candidateLaneIndex] -= 1.0f;
                for (loopIndex = 0, scanIndex = targetLaneIndex; loopIndex < 7; loopIndex++) {
                    selectedLaneIndex = ((playerState->playerIndex / 2) % 2 == 0) ? loopIndex : (6 - loopIndex);
                    if (playerLaneHeights[scanIndex] > playerLaneHeights[selectedLaneIndex]) {
                        scanIndex = selectedLaneIndex;
                    }
                }
                playerDepthOffset = object->trans.z - (playerState->playerIndex == 0 ? -100.0f : 700.0f);
                targetHorizontalOffset = (((playerLaneHeights[candidateLaneIndex] > playerDepthOffset - 400.0f || playerDepthOffset > 550.0f) ? scanIndex
                                 : (playerDepthOffset > 500.0f) ? ((playerDepthOffset - 500.0f) / 50.0f) * scanIndex + (1.0f - (playerDepthOffset - 500.0f) / 50.0f) * candidateLaneIndex
                                                    : candidateLaneIndex)
                            - 3.0f)
                        * 100.0f
                    - object->trans.x;
                {
                    float negativeTargetXOffsets[] = { -21.0f, 0.0f, 15.0f };
                    float positiveTargetXOffsets[] = { 60.0f, 65.0f, 53.0f };
                    float positiveTargetZOffsets[] = { 54.0f, 72.0f, 59.0f };
                    float negativeTargetZOffsets[] = { -15.0f, 7.0f, 22.0f };

                    animationVariant = (playerState->randomSeed / 5) % 3;
                    animationVariant = (playerState->playerIndex == 0) ? (object->trans.z < 200.0f ? 0 : 1) : 2;
                    targetHorizontalOffset /= 20.0f;
                    targetHorizontalOffset = (targetHorizontalOffset < -1.0f) ? targetHorizontalOffset : (targetHorizontalOffset > 1.0f) ? 1.0f : targetHorizontalOffset;
                    horizontalBlend = (targetHorizontalOffset < 0.0f) ? -targetHorizontalOffset : targetHorizontalOffset;
                    centerBlend = 1.0f - horizontalBlend;
                    targetStickX = (targetHorizontalOffset < 0.0f) ? (36.0f * centerBlend + horizontalBlend * negativeTargetXOffsets[animationVariant]) : (36.0f * centerBlend + horizontalBlend * positiveTargetXOffsets[animationVariant]);
                    targetStickY = (targetHorizontalOffset < 0.0f) ? (42.0f * centerBlend + horizontalBlend * positiveTargetZOffsets[animationVariant]) : (42.0f * centerBlend + horizontalBlend * negativeTargetZOffsets[animationVariant]);
                }
                stickDeltaX = playerState->desiredStickX - targetStickX;
                stickDeltaY = playerState->desiredStickY - targetStickY;
                if (sqrtf(stickDeltaX * stickDeltaX + stickDeltaY * stickDeltaY) > 50.0f) {
                    playerState->repeatedInputFrames++;
                }
                else {
                    playerState->repeatedInputFrames = 0;
                }
                if (playerState->repeatedInputFrames >= 2) {
                    targetStickX = playerState->desiredStickX;
                    targetStickY = playerState->desiredStickY;
                    playerState->repeatedInputFrames = 0;
                }
                playerState->desiredStickX = targetStickX;
                playerState->desiredStickY = targetStickY;
            }
        }
    }
    if (playerState->playerActionState == 2) {
        previousHeight = object->trans.y;
        previousDepth = object->trans.z;
        motionIndex = 4;
        object->trans.z += 9.375;
        if (playerState->playerIndex == 0 && bonusCoinWinner != -1 && object->trans.z > (object->trans.y > 0.0f ? 620.0f : 1400.0f) + 50.0f
            && previousDepth <= (object->trans.y > 0.0f ? 620.0f : 1400.0f) + 50.0f) {
            HuAudFXStop(bonusCoinWinner);
            bonusCoinWinner = -1;
        }
        playerState->verticalVelocity -= 1.0f;
        object->trans.y += (object->trans.z > (object->trans.y > 0.0f ? 620.0f : 1400.0f) - 50.0f) ? playerState->verticalVelocity : 0.0f;
        if (playerState->playerIndex == 0 && bonusCoinWinner == -1 && object->trans.z > 670.0f && object->trans.z < 1450.0f && object->trans.y == 0.0f
            && previousHeight > 0.0f) {
            bonusCoinWinner = HuAudFXPlay(0x612);
        }
        if (object->trans.z < 1400.0f && object->trans.y <= 0.0f) {
            object->trans.y = 0.0f;
            playerState->verticalVelocity = 0.0f;
        }
        object->rot.y += (object->rot.y < 0.0f) ? 360 : (object->rot.y > 360.0f) ? -360 : 0;
        object->rot.y = (object->rot.y > 140.0f && object->rot.y < 220.0f) ? 180.0f
            : (object->rot.y < 180.0f)                                 ? (40.0f + object->rot.y)
            : (object->rot.y > 180.0f)                                 ? (object->rot.y - 40.0f)
                                                                     : object->trans.y;
        ;
        if (frameCounter % 4 < 3) {
            Hu3DModelAttrReset(object->mdlId[0], 1);
        }
        else {
            Hu3DModelAttrSet(object->mdlId[0], 1);
        }
        if (object->trans.y < -500.0f) {
            playerState->playerActionState = 3;
            Hu3DModelAttrSet(object->mdlId[0], 1);
        }
    }
    else if (playerState->playerActionState != 3 && playerState->playerActionState != 4) {
        if (playerState->playerActionState == 5) {
            if (playerState->actionTimer == 0) {
                motionIndex = 3;
                Hu3DMotionSpeedSet(object->mdlId[0], 1.0f);
                Hu3DModelAttrReset(object->mdlId[0], 1);
            }
            playerState->actionTimer++;
            if (playerState->actionTimer == 30) {
                playerState->forcedMotionIndex = -1;
                playerState->currentMotionIndex = motionIndex = 0;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
            }
            if (playerState->actionTimer >= 31 && playerState->actionTimer <= 38) {
                object->trans.x -= 150.0 * -sind(object->rot.y) / 7.0;
                object->trans.z -= 150.0 * -cosd(object->rot.y) / 7.0;
            }
            else {
                object->trans.x = arenaPieceStates[9][playerState->playerIndex].positionX;
                object->trans.y = (playerState->playerIndex == 0) ? 300 : 0;
                object->trans.z = arenaPieceStates[9][playerState->playerIndex].positionZ;
            }
            if (playerState->actionTimer == 34 && playerState->playerIndex == 0) {
                Hu3DMotionTimeSet(arenaObjects->mdlId[playerState->playerIndex + 79], 15.0f + Hu3DMotionTimeGet(arenaObjects->mdlId[playerState->playerIndex + 79]));
            }
            if (playerState->actionTimer == 38) {
                playerState->playerActionState = 1;
            }
        }
        else {
            float movementVelocityX = 0.25 * targetStickX;
            float movementVelocityZ = 0.25 * -targetStickY;
            Vec movementVector = { 0.0f, 0.0f, 0.0f };

            movementVector.x = movementVelocityX;
            movementVector.z = movementVelocityZ;
            MTXRotRad(rotationMatrix, 'y', MTXDegToRad(CRot.y));
            MTXMultVec(rotationMatrix, &movementVector, &movementVector);
            movementVelocityX = movementVector.x;
            movementVelocityZ = movementVector.z;
            movementSpeed = sqrtf(movementVelocityX * movementVelocityX + movementVelocityZ * movementVelocityZ);
            if (movementSpeed >= 0.5f) {
                if (movementSpeed < 10.0f) {
                    playerState->movementVelocityX = movementVelocityX;
                    playerState->movementVelocityZ += movementVelocityZ;
                }
                else {
                    movementVelocityX /= movementSpeed;
                    movementVelocityZ /= movementSpeed;
                    movementSpeed = 10.0f;
                    playerState->movementVelocityX = movementVelocityX * movementSpeed;
                    playerState->movementVelocityZ = movementVelocityZ * movementSpeed;
                }
                playerState->movementSpeed = movementSpeed;
                object->rot.y = atan2d(playerState->movementVelocityX, playerState->movementVelocityZ);
                if (playerState->playerActionState == 1) {
                    if (playerState->movementSpeed >= 8.0f) {
                        motionIndex = 2;
                    }
                    else {
                        motionIndex = 1;
                    }
                }
            }
            else if (playerState->playerActionState == 1) {
                motionIndex = 0;
            }
            if (roundPhase == 1005 && (playerState->playerActionState == 1 || playerState->playerActionState == 0)) {
                playerState->forcedMotionIndex = (playerCollectedCoins[playerState->playerIndex == 0 ? 0 : 1] > 0) ? 6 : 7;
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
                playerState->playerActionState = 0;
                if (roundTimer == 1.0f && playerCollectedCoins[playerState->playerIndex == 0 ? 0 : 1] > 0) {
                    s32 coinSoundEffectIds[] = { 0x122, 0x162, 0x1A2, 0x1E2, 0x222, 0x262, 0x2A2, 0x2E2 };

                    HuAudFXPlay(coinSoundEffectIds[playerState->characterId]);
                }
            }
            if (roundPhase >= 1004 && playerState->playerActionState == 0) {
                object->rot.y = 2.4f + 0.8f * object->rot.y;
                if (playerState->currentMotionIndex != 6 && playerState->currentMotionIndex != 7) {
                    motionIndex = 0;
                }
            }
            object->trans.x += playerState->movementVelocityX;
            object->trans.z += playerState->movementVelocityZ;
            if (playerState->playerIndex != 0) {
                for (otherPlayerIndex = 1; otherPlayerIndex < 4; otherPlayerIndex++) {
                    otherPlayerDeltaX = playerObjects[otherPlayerIndex]->trans.x - object->trans.x;
                    otherPlayerDeltaZ = playerObjects[otherPlayerIndex]->trans.z - object->trans.z;
                    if (otherPlayerIndex != playerState->playerIndex) {
                        if (otherPlayerDeltaX == 0.0f && otherPlayerDeltaZ == 0.0f) {
                            otherPlayerDeltaZ = -1.0f;
                        }
                        playerSeparation = sqrtf(otherPlayerDeltaX * otherPlayerDeltaX + otherPlayerDeltaZ * otherPlayerDeltaZ);
                        if (playerSeparation < 100.0f) {
                            otherPlayerDeltaX *= (100.0f / playerSeparation);
                            otherPlayerDeltaZ *= (100.0f / playerSeparation);
                            object->trans.x = playerObjects[otherPlayerIndex]->trans.x - otherPlayerDeltaX;
                            object->trans.z = playerObjects[otherPlayerIndex]->trans.z - otherPlayerDeltaZ;
                        }
                    }
                }
            }
            if (roundPhase > 1001) {
                for (arenaRowIndex = 0; arenaRowIndex < 12; arenaRowIndex++) {
                    if (arenaRowIndex == 9 && roundPhase <= 1001) { }
                    for (arenaColumnIndex = 0; arenaColumnIndex < (arenaRowIndex < 10 ? 4 : 10); arenaColumnIndex++) {
                        nearbyArenaPiece = &arenaPieceStates[arenaRowIndex][arenaColumnIndex];
                        pieceModelIndex = (arenaRowIndex < 10) ? (arenaRowIndex * 4 + 43 + arenaColumnIndex) : ((arenaRowIndex - 10) * 10 + 83 + arenaColumnIndex);
                        if (nearbyArenaPiece->pieceState != 0 && nearbyArenaPiece->drawDelayFrames <= 0) {
                            Vec pieceWorldPosition;

                            Hu3DModelObjPosGet(arenaObjects->mdlId[pieceModelIndex], arenaPieceJointNames[nearbyArenaPiece->pieceTypeIndex], &pieceWorldPosition);
                            if (((nearbyArenaPiece->pieceState == 3 || (nearbyArenaPiece->pieceState == 4 && pieceWorldPosition.y >= 250.0f)) && playerState->playerIndex == 0)
                                || (((nearbyArenaPiece->pieceState == 4 && pieceWorldPosition.y < 250.0f) || nearbyArenaPiece->pieceState == 5 || nearbyArenaPiece->pieceState == 6)
                                    && playerState->playerIndex != 0)) {
                                collisionPointX = pieceWorldPosition.x;
                                collisionPointZ = pieceWorldPosition.z;
                                if (arenaPieceCollisionHalfWidths[nearbyArenaPiece->pieceTypeIndex] > 0.0f || arenaPieceCollisionHalfDepths[nearbyArenaPiece->pieceTypeIndex] > 0.0f) {
                                    collisionPointX = (object->trans.x < collisionPointX - arenaPieceCollisionHalfWidths[nearbyArenaPiece->pieceTypeIndex]) ? (collisionPointX - arenaPieceCollisionHalfWidths[nearbyArenaPiece->pieceTypeIndex])
                                        : (object->trans.x > collisionPointX + arenaPieceCollisionHalfWidths[nearbyArenaPiece->pieceTypeIndex])   ? (collisionPointX + arenaPieceCollisionHalfWidths[nearbyArenaPiece->pieceTypeIndex])
                                                                                                       : object->trans.x;
                                    collisionPointZ = (object->trans.z < collisionPointZ - arenaPieceCollisionHalfDepths[nearbyArenaPiece->pieceTypeIndex]) ? (collisionPointZ - arenaPieceCollisionHalfDepths[nearbyArenaPiece->pieceTypeIndex])
                                        : (object->trans.z > collisionPointZ + arenaPieceCollisionHalfDepths[nearbyArenaPiece->pieceTypeIndex])   ? (collisionPointZ + arenaPieceCollisionHalfDepths[nearbyArenaPiece->pieceTypeIndex])
                                                                                                       : object->trans.z;
                                }
                                pieceDeltaX = collisionPointX - object->trans.x;
                                pieceDeltaZ = collisionPointZ - object->trans.z;
                                if (arenaRowIndex < 10 && pieceDeltaX == 0.0f && pieceDeltaZ == 0.0f) {
                                    pieceDeltaZ = -1.0f;
                                }
                                pieceDistance = sqrtf(pieceDeltaX * pieceDeltaX + pieceDeltaZ * pieceDeltaZ);
                                if (pieceDistance < 50.0f + arenaPieceCollisionRadii[nearbyArenaPiece->pieceTypeIndex]) {
                                    if (arenaRowIndex < 10) {
                                        pieceDeltaX *= (50.0f + arenaPieceCollisionRadii[nearbyArenaPiece->pieceTypeIndex]) / pieceDistance;
                                        pieceDeltaZ *= (50.0f + arenaPieceCollisionRadii[nearbyArenaPiece->pieceTypeIndex]) / pieceDistance;
                                        object->trans.x = collisionPointX - pieceDeltaX;
                                        object->trans.z = collisionPointZ - pieceDeltaZ;
                                    }
                                    else {
                                        nearbyArenaPiece->pieceState = 0;
                                        playerCollectedCoins[playerState->playerIndex == 0 ? 0 : 1] += (arenaRowIndex == 10 ? 1 : 5);
                                        CharEffectCoinGlowCreate(-1, &pieceWorldPosition);
                                        if (playerStartPositions[arenaRowIndex == 10 ? 8 : 9].positionCount < 3) {
                                            playerStartPositions[arenaRowIndex == 10 ? 8 : 9].positions[playerStartPositions[arenaRowIndex == 10 ? 8 : 9].positionCount] = 0.0f;
                                            playerStartPositions[arenaRowIndex == 10 ? 8 : 9].positionCount++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (object->trans.z > (playerState->playerIndex == 0 ? 620.0f : 1400.0f) - 50.0f) {
                playerState->playerActionState = 2;
                playerState->verticalVelocity = 0.0f;
                CharFXPlay(playerState->characterId, 0x123);
            }
            if (playerState->playerActionState != 2) {
                if (object->trans.x > (playerState->playerIndex == 0 ? 400 : 400) - 50.0f) {
                    object->trans.x = (playerState->playerIndex == 0 ? 400 : 400) - 50.0f;
                }
                if (object->trans.x < -(playerState->playerIndex == 0 ? 400 : 400) + 50.0f) {
                    object->trans.x = -(playerState->playerIndex == 0 ? 400 : 400) + 50.0f;
                }
                if (object->trans.z < (playerState->playerIndex == 0 ? 0.0f : 600.0f) + 50.0f) {
                    object->trans.z = (playerState->playerIndex == 0 ? 0.0f : 600.0f) + 50.0f;
                }
                if (object->trans.z > (playerState->playerIndex == 0 ? 620.0f : 1400.0f) - 50.0f) {
                    object->trans.z = (playerState->playerIndex == 0 ? 620.0f : 1400.0f) - 50.0f;
                }
            }
        }
    }
    if (roundPhase == 1001) {
        playerIndexCopy = playerState->playerIndex;
        if (roundTimer == 23.0f) {
            Hu3DMotionSet(arenaObjects->mdlId[playerState->playerIndex + 79], arenaObjects->mtnId[playerState->playerIndex + 22]);
            Hu3DModelAttrReset(arenaObjects->mdlId[playerState->playerIndex + 79], HU3D_MOTATTR_PAUSE);
            if (playerStartPositions[4].positionCount < 3) {
                playerStartPositions[4].positions[playerStartPositions[4].positionCount] = 0.0f;
                playerStartPositions[4].positionCount++;
            }
        }
        if (roundTimer == 43.0f) {
            Hu3DModelAttrSet(arenaObjects->mdlId[playerState->playerIndex + 79], HU3D_MOTATTR_PAUSE);
        }
        if (roundTimer == 80.0f) {
            Hu3DMotionTimeSet(arenaObjects->mdlId[playerState->playerIndex + 79], 20.0f);
            Hu3DModelAttrReset(arenaObjects->mdlId[playerState->playerIndex + 79], HU3D_MOTATTR_PAUSE);
        }
        if (roundTimer == 100.0f && playerStartPositions[0].positionCount < 3) {
            playerStartPositions[0].positions[playerStartPositions[0].positionCount] = 0.0f;
            playerStartPositions[0].positionCount++;
        }
        if (roundTimer == 106.0f) {
            Hu3DModelAttrReset(arenaObjects->mdlId[playerState->playerIndex + 39], 1);
            Hu3DParticleCntSet(arenaObjects->mdlId[playerState->playerIndex + 39], 0);
            roundWinner = 1;
        }
        if (roundTimer == 60.0f) {
            Hu3DModelAttrReset(object->mdlId[0], 1);
            Hu3DMotionSet(object->mdlId[0], object->mtnId[3]);
            Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
        }
        if (roundTimer == 61.0f) {
            Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
        }
        if (roundTimer == 120.0f) {
            motionIndex = 3;
            Hu3DModelAttrReset(object->mdlId[0], 1);
        }
        if (roundTimer == 155.0f) {
            motionIndex = 0;
        }
        object->trans.x = arenaPieceStates[9][playerIndexCopy].positionX;
        object->trans.z = (roundTimer > 163.0f) ? object->trans.z : (roundTimer > 155.0f) ? (object->trans.z - 18.75) : arenaPieceStates[9][playerIndexCopy].positionZ;
    }
    if (playerState->forcedMotionIndex >= 0) {
        motionIndex = playerState->forcedMotionIndex;
    }
    if (motionIndex >= 0 && motionIndex != playerState->currentMotionIndex) {
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[motionIndex], 0.0f, playerMotionDurations[motionIndex],
            (playerMotionLoopFlags[motionIndex] != 0) ? HU3D_MOTATTR_NONE : HU3D_MOTATTR_LOOP);
        playerState->currentMotionIndex = motionIndex;
    }
}

s32 IsPlayerNearArenaPiece(float playerPositionX, float playerPositionZ, float collisionRadius, s32 playerIndex, s32 collisionMode)
{
    Vec pieceWorldPosition;
    float pieceCenterX;
    float pieceCenterZ;
    float lowerDepthBound;
    float upperDepthBound;
    float separationX;
    float separationZ;
    float separationDistance;
    ArenaPieceState *pieceState;
    s32 pieceModelIndex;
    s32 pieceRowIndex;
    s32 pieceColumnIndex;

    for (pieceRowIndex = 0; pieceRowIndex < 12; pieceRowIndex++) {
        for (pieceColumnIndex = 0; pieceColumnIndex < (pieceRowIndex < 10 ? 4 : 10); pieceColumnIndex++) {
            pieceState = &arenaPieceStates[pieceRowIndex][pieceColumnIndex];
            pieceModelIndex = (pieceRowIndex < 10) ? (pieceRowIndex * 4 + 43 + pieceColumnIndex) : ((pieceRowIndex - 10) * 10 + 83 + pieceColumnIndex);
            if (pieceState->pieceState != 0 && pieceState->drawDelayFrames <= 0) {
                Hu3DModelObjPosGet(arenaObjects->mdlId[pieceModelIndex], arenaPieceJointNames[pieceState->pieceTypeIndex], &pieceWorldPosition);
                if (pieceState->pieceState == 3 || pieceState->pieceState == 5) {
                    pieceCenterX = (pieceState->animationFrame == 0) ? pieceState->positionX : pieceWorldPosition.x;
                    pieceCenterZ = (pieceState->animationFrame == 0) ? pieceState->positionZ : pieceWorldPosition.z;
                    lowerDepthBound = 50.0f + arenaPieceCollisionHalfDepths[pieceState->pieceTypeIndex];
                    upperDepthBound = 50.0f + arenaPieceCollisionHalfDepths[pieceState->pieceTypeIndex];
                    if (pieceState->pieceState == 3) {
                        float characterCollisionOffsets[][12] = { { -25.0f, -25.0f, -25.0f, -6.6875f, -6.6875f, -6.6875f, 62.765625f, 62.765625f, 62.765625f, 51.99469f,
                                                 76.875f, 45.0f },
                            { -27.647583f, -27.647583f, -27.647583f, -9.64325f, -9.64325f, -9.64325f, 60.49022f, 60.49022f, 60.49022f, 51.99469f,
                                76.875f, 45.0f } };

                        pieceCenterZ += characterCollisionOffsets[pieceState->animationFrame == 0 ? 0 : 1][pieceState->pieceTypeIndex];
                        lowerDepthBound += 50.0f;
                        upperDepthBound += 50.0f;
                    }
                    else if (pieceState->pieceState == 5 && pieceState->animationFrame == 0) {
                        pieceCenterZ += arenaPieceRaisedHeights[pieceState->pieceTypeIndex];
                    }
                    else if (pieceState->pieceState == 1) {
                        pieceCenterX = (pieceState->positionX < 0.0f ? -1 : 1) * 321;
                        pieceCenterZ = 880.0f - playerMoveSpeed * (100 - pieceState->animationFrame);
                    }
                    if (pieceState->pieceTypeIndex == 9) {
                        upperDepthBound += 300.0f;
                    }
                    if (arenaPieceCollisionHalfWidths[pieceState->pieceTypeIndex] > 0.0f || lowerDepthBound > 0.0f || upperDepthBound > 0.0f) {
                        pieceCenterX = (playerPositionX < pieceCenterX - arenaPieceCollisionHalfWidths[pieceState->pieceTypeIndex]) ? (pieceCenterX - arenaPieceCollisionHalfWidths[pieceState->pieceTypeIndex])
                            : (playerPositionX > pieceCenterX + arenaPieceCollisionHalfWidths[pieceState->pieceTypeIndex])     ? (pieceCenterX + arenaPieceCollisionHalfWidths[pieceState->pieceTypeIndex])
                                                                                      : playerPositionX;
                        pieceCenterZ = (playerPositionZ < pieceCenterZ - lowerDepthBound) ? (pieceCenterZ - lowerDepthBound) : (playerPositionZ > pieceCenterZ + upperDepthBound) ? (pieceCenterZ + upperDepthBound) : playerPositionZ;
                    }
                    separationX = pieceCenterX - playerPositionX;
                    separationZ = pieceCenterZ - playerPositionZ;
                    if (pieceRowIndex < 10 && separationX == 0.0f && separationZ == 0.0f) {
                        separationZ = -1.0f;
                    }
                    separationDistance = sqrtf(separationX * separationX + separationZ * separationZ);
                    if (separationDistance < collisionRadius + arenaPieceCollisionRadii[pieceState->pieceTypeIndex]) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

void UpdateArenaObjects(OMOBJ *object)
{
    PlayerObjectState *playerStates[4];
    Vec piecePosition;
    s32 mainPlayerEventCount;
    s32 primaryEventCount;
    s32 secondaryEventCount;
    s32 tertiaryEventCount;
    s32 randomModelAttempts;
    s32 otherPlayerEventCount;
    s32 bonusEventCount;
    s32 retryCount;
    s32 pieceModelIndex;
    s32 bonusPieceModelIndex;
    s32 scoreEntryIndex;
    float previousPositionX;
    float previousPositionY;
    float previousPositionZ;
    s32 motionFrame;
    float piecePositionX;
    float piecePositionZ;
    float soundSpeedRatio;
    float animationProgress;
    float remainingProgress;
    float cooldownProgress;
    float inverseProgress;
    float cameraStartWeight;
    float animationWeight;
    float bounceProgress;
    float cameraEndWeight;
    float cameraProgress;
    ArenaPieceState *pieceState;
    s32 overlapIndex;
    s32 pieceGroupIndex;
    s32 pieceEventType;
    s32 pieceModelIndex;
    s32 outerIndex;
    s32 innerIndex;

    for (outerIndex = 0; outerIndex < 4; outerIndex++) {
        playerStates[outerIndex] = playerObjects[outerIndex]->data;
    }
    frameCounter++;
    playerMoveSpeed = (roundPhase <= 1002) ? 1.5
        : (roundPhase == 1003)         ? (7.5 * (roundTimer > 120.0f ? 1.0 : (0.2 + 0.8 * (roundTimer / 120.0))))
                                         : 0.0;
    alternateRoundTimer = (roundPhase < 1005 && playerStates[0]->playerActionState >= 2 && playerStates[0]->playerActionState <= 5 && !(roundPhase == 1003 && roundTimer <= 120.0f))
        ? 7.5
        : playerMoveSpeed;
    Hu3DMotionSpeedSet(object->mdlId[1], 1.5 * alternateRoundTimer / 7.5);
    Hu3DMotionSpeedSet(object->mdlId[3], 1.5 * playerMoveSpeed / 7.5);
    if (roundSoundSequenceId != -1) {
        soundSpeedRatio = 1.5 * alternateRoundTimer / 7.5;
        HuAudFXPitchSet(roundSoundSequenceId, 8190.0f * soundSpeedRatio - 8190.0f);
        HuAudFXVolSet(roundSoundSequenceId, soundSpeedRatio <= 0.001 ? 0 : 127);
    }
    if (roundPhase <= 1001) {
        cameraProgress = (roundPhase == 1000 || roundTimer < 60.0f) ? 0.0 : (roundTimer < 120.0f) ? (roundTimer - 60.0f) / 60.0 : 1.0;
        cameraProgress = 1.0f - cameraProgress;
        cameraProgress *= cameraProgress;
        cameraProgress = 1.0f - cameraProgress;
        cameraStartWeight = 1.0f - cameraProgress;
        Center.x = 690.0f * cameraProgress;
        Center.y = 400.0f * cameraStartWeight + 1340.0f * cameraProgress;
        Center.z = 500.0f * cameraStartWeight + 1380.0f * cameraProgress;
        CRot.x = -10.0f * cameraStartWeight + -50.8125f * cameraProgress;
        CRot.y = 40.0f * cameraProgress;
        CZoom = 2126.6f * cameraStartWeight + 2126.6f * cameraProgress;
        Center.x = 700.0f * cameraProgress;
        Center.y = 400.0f * cameraStartWeight + 870.0f * cameraProgress;
        Center.z = 500.0f * cameraStartWeight + 1320.0f * cameraProgress;
        CRot.x = -10.0f * cameraStartWeight + -37.25f * cameraProgress;
        CRot.y = 40.0f * cameraProgress;
    }
    if (roundPhase == 1005 && roundTimer < 100.0f) {
        cameraEndWeight = (roundTimer < 90.0f) ? (roundTimer / 90.0) : 1.0;
        cameraEndWeight = 1.0f - cameraEndWeight;
        cameraEndWeight *= cameraEndWeight;
        cameraEndWeight = 1.0f - cameraEndWeight;
        inverseProgress = 1.0f - cameraEndWeight;
        Center.x = 120.0f * cameraEndWeight + 700.0f * inverseProgress;
        Center.y = 292.5f * cameraEndWeight + 870.0f * inverseProgress;
        Center.z = 1200.0f * cameraEndWeight + 1320.0f * inverseProgress;
        CRot.x = -19.5f * cameraEndWeight + -37.25f * inverseProgress;
        CRot.y = 13.5f * cameraEndWeight + 40.0 * inverseProgress;
        CZoom = 2126.6f * cameraEndWeight + 2126.6f * inverseProgress;
    }
    mainPlayerEventCount = (playerStates[0]->playerActionState == 3 && (arenaPieceStates[9][0].pieceState == 0 || roundPhase != 1003));
    otherPlayerEventCount = (playerStates[1]->playerActionState == 3 && (arenaPieceStates[9][1].pieceState == 0 || roundPhase != 1003))
        + (playerStates[2]->playerActionState == 3 && (arenaPieceStates[9][2].pieceState == 0 || roundPhase != 1003))
        + (playerStates[3]->playerActionState == 3 && (arenaPieceStates[9][3].pieceState == 0 || roundPhase != 1003));
    primaryEventCount = (roundPhase == 1003 && frameCounter % 60 == 0) ? (rand8() % 2 + 1 + mainPlayerEventCount) : mainPlayerEventCount;
    secondaryEventCount = (roundPhase == 1003 && frameCounter % 120 == 0) ? (rand8() % 2 + 1) : 0;
    tertiaryEventCount = otherPlayerEventCount + (roundPhase == 1003 && frameCounter % 60 == 0 && roundCountdown > REFRESH_RATE * 2);
    bonusEventCount = (roundPhase == 1003 && frameCounter % 60 == 0) ? (rand8() % 2 + 2) : 0;
    retryCount = 0;
    primaryEventCount = (roundPhase == 1003 && frameCounter % 60 == 0) ? ((rand8() % 4 < 3) + 1 + mainPlayerEventCount) : mainPlayerEventCount;
    secondaryEventCount = (roundPhase == 1003 && frameCounter % 120 == 1) ? (rand8() % 2 + 1) : 0;
    tertiaryEventCount = otherPlayerEventCount + (roundPhase == 1003 && frameCounter % 60 == 38 && roundCountdown > REFRESH_RATE * 2);
    bonusEventCount = (roundPhase == 1003 && frameCounter % 60 == 0) ? (rand8() % 2 + 2) : 0;
    while (primaryEventCount > 0 || secondaryEventCount > 0 || tertiaryEventCount > 0 || bonusEventCount > 0) {
        pieceEventType = (rand8() % 2 == 0) ? ((tertiaryEventCount != 0) ? 3
                          : (bonusEventCount != 0)               ? 4
                          : (primaryEventCount != 0)             ? 1
                          : (secondaryEventCount != 0)             ? 2
                                                       : 0)
                                     : ((tertiaryEventCount != 0)        ? 3
                                             : (primaryEventCount != 0) ? 1
                                             : (bonusEventCount != 0)   ? 4
                                             : (secondaryEventCount != 0) ? 2
                                                              : 0);
        retryCount++;
        if (retryCount > 8) {
            primaryEventCount -= (pieceEventType == 1);
            secondaryEventCount -= (pieceEventType == 2);
            tertiaryEventCount -= (pieceEventType == 3);
            bonusEventCount -= (pieceEventType == 4);
            retryCount = 0;
        }
        else if (pieceEventType == 1 || pieceEventType == 2 || pieceEventType == 3 || pieceEventType == 4) {
            pieceGroupIndex = (pieceEventType != 4) ? (rand8() % 9) : 10;
            if (pieceEventType == 1 && mainPlayerEventCount != 0) {
                pieceGroupIndex = 9;
                outerIndex = 0;
            }
            else if (pieceEventType == 3 && otherPlayerEventCount != 0) {
                pieceGroupIndex = 9;
                outerIndex = (playerStates[(frameCounter % 3) + 1]->playerActionState == 3 && (arenaPieceStates[9][frameCounter % 3 + 1].pieceState == 0 || roundPhase != 1003))
                    ? (frameCounter % 3 + 1)
                    : (playerStates[(frameCounter + 1) % 3 + 1]->playerActionState == 3
                          && (arenaPieceStates[9][(frameCounter + 1) % 3 + 1].pieceState == 0 || roundPhase != 1003))
                    ? ((frameCounter + 1) % 3 + 1)
                    : (playerStates[(frameCounter + 2) % 3 + 1]->playerActionState == 3
                          && (arenaPieceStates[9][(frameCounter + 2) % 3 + 1].pieceState == 0 || roundPhase != 1003))
                    ? ((frameCounter + 2) % 3 + 1)
                    : 0;
            }
            else {
                if (pieceGroupIndex == 10 && (roundCountdown / REFRESH_RATE) / 15 + 1 <= 2 - roundScore
                    && rand8() % (((roundCountdown / REFRESH_RATE - (1 - roundScore) * 15) * 3) + 1) == 0) {
                    pieceGroupIndex = 11;
                }
                for (outerIndex = 0; outerIndex < (pieceEventType != 4 ? 4 : 10); outerIndex++) {
                    if (arenaPieceStates[pieceGroupIndex][outerIndex].pieceState == 0) {
                        break;
                    }
                }
                if (outerIndex >= (pieceEventType != 4 ? 4 : 10)) {
                    continue;
                }
            }
            arenaPieceStates[pieceGroupIndex][outerIndex].positionX = (pieceEventType != 3) ? (rand8() % 7 - 3) * 100 : ((rand8() % 2) * 2 - 1) * 1595;
            if (outerIndex == 0 && roundPhase >= 1004 && pieceGroupIndex == 9) {
                arenaPieceStates[pieceGroupIndex][outerIndex].positionX = 0.0f;
            }
            arenaPieceStates[pieceGroupIndex][outerIndex].positionY = (pieceEventType == 1) ? 300 : (pieceEventType == 2) ? 0 : (pieceEventType == 3) ? 500 : (pieceEventType == 4) ? 300 : 0;
            arenaPieceStates[pieceGroupIndex][outerIndex].positionZ = (pieceEventType == 1) ? -450 : (pieceEventType == 2) ? -450 : (pieceEventType == 3) ? 105 : (pieceEventType == 4) ? -150 : 0;
            arenaPieceStates[pieceGroupIndex][outerIndex].positionZ += (pieceEventType == 1) ? -(pieceGroupIndex <= 8 ? 0 : 0)
                : (pieceEventType == 2)                              ? -(pieceGroupIndex <= 8 ? (pieceGroupIndex / 3) * 80 : 0)
                : (pieceEventType == 4)                              ? -50
                                                              : 0;
            arenaPieceStates[pieceGroupIndex][outerIndex].positionZ += (pieceEventType == 1) ? ((pieceGroupIndex <= 2) ? 315.0
                                                       : (pieceGroupIndex <= 5)         ? 296.6875
                                                       : (pieceGroupIndex <= 8)         ? 227.234375
                                                       : (pieceGroupIndex == 9)         ? 200
                                                                                : 0)
                                                              : 0.0;
            if (pieceEventType == 1 || pieceEventType == 4) {
                float characterCollisionOffsets[] = { -25.0f, -25.0f, -25.0f, -6.6875f, -6.6875f, -6.6875f, 62.765625f, 62.765625f, 62.765625f, 51.99469f, 76.875f, 45.0f,
                    -27.647583f, -27.647583f, -27.647583f, -9.64325f, -9.64325f, -9.64325f, 60.49022f, 60.49022f, 60.49022f, 51.99469f, 76.875f,
                    45.0f };

                piecePositionX = arenaPieceStates[pieceGroupIndex][outerIndex].positionX;
                piecePositionZ = characterCollisionOffsets[pieceGroupIndex] + arenaPieceStates[pieceGroupIndex][outerIndex].positionZ;
            }
            else if (pieceEventType == 2) {
                piecePositionX = arenaPieceStates[pieceGroupIndex][outerIndex].positionX;
                piecePositionZ = arenaPieceRaisedHeights[pieceGroupIndex] + arenaPieceStates[pieceGroupIndex][outerIndex].positionZ;
            }
            else if (pieceEventType == 3) {
                piecePositionX = (arenaPieceStates[pieceGroupIndex][outerIndex].positionX < 0.0f) ? -335 : 335;
                piecePositionZ = 830.0f - 94.0f * playerMoveSpeed;
            }
            else {
                piecePositionX = arenaPieceStates[pieceGroupIndex][outerIndex].positionX;
                piecePositionZ = arenaPieceStates[pieceGroupIndex][outerIndex].positionZ;
            }
            if (IsPlayerNearArenaPiece(
                    piecePositionX, piecePositionZ, arenaPieceCollisionRadii[arenaPieceStates[pieceGroupIndex][outerIndex].pieceTypeIndex] + arenaPieceCollisionHalfWidths[arenaPieceStates[pieceGroupIndex][outerIndex].pieceTypeIndex], pieceGroupIndex, 0)
                != 0) {
                continue;
            }
            if (pieceEventType == 3 && roundWinCounts[(arenaPieceStates[pieceGroupIndex][outerIndex].positionX < 0.0f) ? 0 : 1] > 0) {
                continue;
            }
            if (pieceEventType == 4) {
                for (overlapIndex = 0; overlapIndex < 10; overlapIndex++) {
                    if (arenaPieceStates[10][overlapIndex].pieceState != 0 && fabs(arenaPieceStates[pieceGroupIndex][outerIndex].positionZ - arenaPieceStates[10][overlapIndex].positionZ) <= 100.0
                        && fabs(arenaPieceStates[pieceGroupIndex][outerIndex].positionX - arenaPieceStates[10][overlapIndex].positionX) <= 200.0) {
                        break;
                    }
                    if (arenaPieceStates[11][overlapIndex].pieceState != 0 && fabs(arenaPieceStates[pieceGroupIndex][outerIndex].positionZ - arenaPieceStates[11][overlapIndex].positionZ) <= 100.0
                        && fabs(arenaPieceStates[pieceGroupIndex][outerIndex].positionX - arenaPieceStates[11][overlapIndex].positionX) <= 200.0) {
                        break;
                    }
                }
                if (overlapIndex < 10 || (arenaPieceStates[pieceGroupIndex][outerIndex].positionX >= -100.0f && arenaPieceStates[pieceGroupIndex][outerIndex].positionX <= 100.0f)) {
                    continue;
                }
            }
            if (pieceGroupIndex == 11) {
                roundScore++;
            }
            if ((roundCountdown / REFRESH_RATE) / 15 == 0 && roundScore == 0) {
                roundScore++;
            }
            arenaPieceStates[pieceGroupIndex][outerIndex].pieceTypeIndex = pieceGroupIndex;
            arenaPieceStates[pieceGroupIndex][outerIndex].pieceState = (pieceEventType == 1) ? 3 : (pieceEventType == 2) ? 5 : (pieceEventType == 3) ? 1 : (pieceEventType == 4) ? 3 : 0;
            arenaPieceStates[pieceGroupIndex][outerIndex].animationFrame = 0;
            arenaPieceStates[pieceGroupIndex][outerIndex].drawDelayFrames = 0;
            arenaPieceStates[pieceGroupIndex][outerIndex].positionDeltaX = arenaPieceStates[pieceGroupIndex][outerIndex].positionDeltaY = arenaPieceStates[pieceGroupIndex][outerIndex].positionDeltaZ = 0.0f;
            if (pieceEventType != 4) {
                pieceModelIndex = pieceGroupIndex * 4 + 43 + outerIndex;
                Hu3DMotionTimeSet(object->mdlId[pieceModelIndex], (pieceEventType == 1 || pieceEventType == 3) ? 1.0f : Hu3DMotionMaxTimeGet(object->mdlId[pieceModelIndex]));
                Hu3DModelAttrSet(object->mdlId[pieceModelIndex], HU3D_MOTATTR_PAUSE);
                Hu3DModelAttrReset(object->mdlId[pieceModelIndex], 1);
                Hu3DModelRotSet(object->mdlId[pieceModelIndex], 0.0f, 0.0f, 0.0f);
            }
            else {
                Hu3DModelAttrReset(object->mdlId[outerIndex + (pieceGroupIndex == 10 ? 83 : 93)], 1);
            }
            if (pieceEventType == 1 && mainPlayerEventCount != 0) {
                mainPlayerEventCount--;
                playerStates[0]->playerActionState = 4;
            }
            else if (pieceEventType == 3 && otherPlayerEventCount != 0) {
                otherPlayerEventCount--;
                playerStates[outerIndex]->playerActionState = 4;
            }
            if (pieceEventType == 3) {
                roundWinCounts[(arenaPieceStates[pieceGroupIndex][outerIndex].positionX < 0.0f) ? 0 : 1] = 85;
                Hu3DMotionTimeSet(object->mdlId[(arenaPieceStates[pieceGroupIndex][outerIndex].positionX < 0.0f) ? 6 : 5], 9.0f);
                if (pieceGroupIndex == 9 && playerStartPositions[(arenaPieceStates[pieceGroupIndex][outerIndex].positionX < 0.0f) ? 7 : 6].positionCount < 3) {
                    playerStartPositions[(arenaPieceStates[pieceGroupIndex][outerIndex].positionX < 0.0f) ? 7 : 6]
                        .positions[playerStartPositions[(arenaPieceStates[pieceGroupIndex][outerIndex].positionX < 0.0f) ? 7 : 6].positionCount]
                        = 0.0f;
                    playerStartPositions[(arenaPieceStates[pieceGroupIndex][outerIndex].positionX < 0.0f) ? 7 : 6].positionCount++;
                }
            }
            if (pieceGroupIndex == 9) {
                bonusPieceModelIndex = pieceGroupIndex * 4 + 43 + outerIndex;
                Hu3DMotionSet(object->mdlId[bonusPieceModelIndex], object->mtnId[outerIndex + 22]);
                Hu3DMotionTimeSet(object->mdlId[bonusPieceModelIndex], 0.0f);
                Hu3DMotionSpeedSet(object->mdlId[bonusPieceModelIndex], 2.0f);
            }
            primaryEventCount -= (pieceEventType == 1);
            secondaryEventCount -= (pieceEventType == 2);
            tertiaryEventCount -= (pieceEventType == 3);
            bonusEventCount -= (pieceEventType == 4);
            retryCount = 0;
        }
    }
    for (outerIndex = 0; outerIndex < 12; outerIndex++) {
        for (innerIndex = 0; innerIndex < (outerIndex < 10 ? 4 : 10); innerIndex++) {
            pieceState = &arenaPieceStates[outerIndex][innerIndex];
            pieceModelIndex = (outerIndex < 10) ? (outerIndex * 4 + 43 + innerIndex) : ((outerIndex - 10) * 10 + 83 + innerIndex);
            piecePosition.x = piecePosition.y = piecePosition.z = 0.0f;
            if (pieceState->pieceState == 0) {
                Hu3DModelAttrSet(object->mdlId[pieceModelIndex], 1);
            }
            else {
                pieceState->animationFrame++;
                pieceState->rotationY += (pieceState->pieceTypeIndex == 10) ? 0 : 0;
                pieceState->rotationY += (pieceState->rotationY < 0.0f) ? 360 : (pieceState->rotationY > 360.0f) ? -360 : 0;
                Hu3DModelObjPosGet(object->mdlId[pieceModelIndex], arenaPieceJointNames[pieceState->pieceTypeIndex], &piecePosition);
                switch (pieceState->pieceState) {
                    case 3:
                    case 5:
                    case 6:
                        if (pieceState->pieceTypeIndex >= 3 && pieceState->pieceTypeIndex >= 9 && pieceState->pieceState == 3 && pieceState->positionZ < -120.0f
                            && pieceState->positionZ + alternateRoundTimer >= -120.0f) {
                            scoreEntryIndex = (500.0f + pieceState->positionX) / 100.0f;
                            scoreEntryIndex = (scoreEntryIndex < 0) ? 0 : (scoreEntryIndex >= 11) ? 10 : scoreEntryIndex;
                            playerRoundScores[scoreEntryIndex] = 15;
                            Hu3DMotionShiftSet(object->mdlId[scoreEntryIndex + 8], object->mtnId[11], 20.0f, 15.0f, HU3D_MOTATTR_NONE);
                            if (pieceState->pieceTypeIndex == 9 && playerStartPositions[5].positionCount < 3) {
                                playerStartPositions[5].positions[playerStartPositions[5].positionCount] = 0.0f;
                                playerStartPositions[5].positionCount++;
                            }
                        }
                        pieceState->positionZ += (pieceState->pieceState == 3 ? alternateRoundTimer : playerMoveSpeed);
                        if (roundPhase >= 1003 && roundPhase < 1005) {
                            if ((pieceState->pieceState == 3 && pieceState->pieceTypeIndex == 9 && pieceState->positionZ >= 150.0f && pieceState->positionZ - alternateRoundTimer < 150.0f)
                                || (pieceState->pieceState == 6 && pieceState->pieceTypeIndex == 9 && pieceState->animationFrame == 1)) {
                                playerStates[innerIndex]->playerActionState = 5;
                                playerStates[innerIndex]->actionTimer = 0;
                                playerObjects[innerIndex]->trans.x = pieceState->positionX;
                                playerObjects[innerIndex]->trans.y = (innerIndex == 0 ? 300 : 0);
                                playerObjects[innerIndex]->trans.z = pieceState->positionZ;
                                playerObjects[innerIndex]->rot.y
                                    = 180 + ((roundState == 4 && pieceState->pieceState == 6) ? (pieceState->positionX < 0.0f ? -45 : 45) : 0);
                                Hu3DMotionSpeedSet(object->mdlId[innerIndex + 79], 1.0f);
                                Hu3DMotionShiftSet(object->mdlId[innerIndex + 79], object->mtnId[innerIndex + 22], 44.0f, 2.0f, HU3D_MOTATTR_NONE);
                                Hu3DParticleCntSet(object->mdlId[innerIndex + 39], innerIndex == 0 ? 15 : 0);
                                if (playerStartPositions->playerOrModelIndex < 3) {
                                    playerStartPositions->stateOrTimer[playerStartPositions->playerOrModelIndex] = 0.0f;
                                    playerStartPositions->playerOrModelIndex++;
                                }
                            }
                        }
                        if (pieceState->pieceState == 3 && pieceState->positionZ > 510.0f) {
                            pieceState->pieceState = 4;
                            pieceState->animationFrame = 0;
                            if (pieceState->pieceTypeIndex != 10) {
                                if (pieceState->pieceTypeIndex != 9) {
                                    Hu3DMotionTimeSet(object->mdlId[pieceModelIndex], 1.0f);
                                }
                                else {
                                    Hu3DMotionSet(object->mdlId[pieceModelIndex], object->mtnId[innerIndex + 26]);
                                    Hu3DMotionTimeSet(object->mdlId[pieceModelIndex], 1.0f);
                                    Hu3DMotionSpeedSet(object->mdlId[pieceModelIndex], 1.0f);
                                }
                                Hu3DModelAttrReset(object->mdlId[pieceModelIndex], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
                            }
                        }
                        if ((pieceState->pieceState == 5 || pieceState->pieceState == 6) && piecePosition.z > 1400.0f && pieceState->positionZ > 700.0f) {
                            pieceState->animationFrame = 0;
                            pieceState->positionY = piecePosition.y;
                            pieceState->positionZ = piecePosition.z;
                            pieceState->positionDeltaY = 0.0f;
                            if (pieceState->pieceTypeIndex == 10) {
                                Hu3DModelAttrSet(object->mdlId[pieceModelIndex], HU3D_MOTATTR_LOOP);
                            }
                            else if (pieceState->pieceTypeIndex != 9) {
                                Hu3DMotionTimeSet(object->mdlId[pieceModelIndex], 2.0f);
                                Hu3DModelAttrReset(object->mdlId[pieceModelIndex], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
                            }
                            else {
                                Hu3DMotionSet(object->mdlId[pieceModelIndex], object->mtnId[(pieceState->pieceState == 6) ? (innerIndex + 26) : (innerIndex + 30)]);
                                Hu3DMotionTimeSet(object->mdlId[pieceModelIndex], 1.0f);
                                Hu3DMotionSpeedSet(object->mdlId[pieceModelIndex], 1.0f);
                            }
                            pieceState->pieceState = 7;
                            Hu3DModelPosSet(object->mdlId[pieceModelIndex], pieceState->positionX, pieceState->positionY, pieceState->positionZ);
                        }
                        else {
                            Hu3DModelPosSet(object->mdlId[pieceModelIndex], pieceState->positionX,
                                ((pieceState->pieceTypeIndex >= 0 && pieceState->pieceTypeIndex < 10)
                                        ? ((pieceState->pieceState != 6 ? 300 : 0) + arenaPieceBaseHeights[pieceState->pieceTypeIndex])
                                        : pieceState->positionY)
                                    + arenaPieceVerticalOffsets[pieceState->pieceTypeIndex],
                                pieceState->positionZ + zeroArenaPieceOffsets[pieceState->pieceTypeIndex]);
                        }
                        if (pieceState->pieceTypeIndex >= 10) {
                            Hu3DModelRotSet(object->mdlId[pieceModelIndex], 0.0f, pieceState->rotationY, 0.0f);
                        }
                        break;
                    case 4:
                        if (pieceState->pieceTypeIndex >= 0 && pieceState->pieceTypeIndex < 10) {
                            float pieceAnimationRates[] = { 0.0f, 0.0f, 0.0f, 0.9f, 0.9f, 0.9f, 1.75f, 1.75f, 1.75f, 0.0f, 0.0f, 0.0f };
                            s32 pieceAnimationEndFrames[] = { 42, 42, 42, 41, 41, 41, 48, 48, 48, 27, 27, 27 };

                            if (pieceState->pieceTypeIndex >= 6 && pieceState->pieceTypeIndex <= 8) {
                                bounceProgress = pieceState->animationFrame;
                                animationWeight = (bounceProgress <= 1.0f)                 ? 0.0
                                    : (bounceProgress >= 30.0f && bounceProgress < 40.0f) ? -1.0 * (1.0 - (bounceProgress - 30.0f) / 9.0)
                                    : (bounceProgress >= 40.0f)                     ? 1.0 - (bounceProgress - 40.0f) / 8.0
                                                                              : 1.0;
                                animationWeight = (bounceProgress >= 40.0f) ? -1.0 + (animationWeight * 0.5 + animationWeight * animationWeight * 0.5) : animationWeight;
                                animationWeight = (bounceProgress <= 1.0f) ? 0.0
                                    : (bounceProgress < 23.0f)      ? 1.0
                                    : (bounceProgress < 32.0f)      ? 0.5
                                    : (bounceProgress < 40.0f)      ? -0.8
                                                              : (-1.0 - 0.4 * (1.0 - (bounceProgress - 40.0f) / 8.0));
                                pieceState->positionZ -= (alternateRoundTimer * pieceAnimationRates[pieceState->pieceTypeIndex] / 4.0) * animationWeight;
                            }
                            else {
                                pieceState->positionZ -= alternateRoundTimer * pieceAnimationRates[pieceState->pieceTypeIndex] / 4.0;
                            }
                            if (pieceState->animationFrame >= pieceAnimationEndFrames[pieceState->pieceTypeIndex]) {
                                pieceState->positionY = 0.0f;
                                pieceState->pieceState = 5;
                                pieceState->animationFrame = 0;
                            }
                        }
                        else {
                            pieceState->positionDeltaY -= 1.0f;
                            pieceState->positionY += pieceState->positionDeltaY;
                            pieceState->positionZ += 1.25 * alternateRoundTimer;
                            if (pieceState->positionY <= 0.0f) {
                                if (pieceState->positionDeltaY < -10.0f && pieceState->pieceTypeIndex == 10) {
                                    pieceState->positionDeltaY = 9.0f;
                                }
                                else {
                                    pieceState->positionDeltaY = 0.0f;
                                    pieceState->positionY = 0.0f;
                                    pieceState->pieceState = 5;
                                    pieceState->animationFrame = 0;
                                }
                            }
                        }
                        Hu3DModelPosSet(object->mdlId[pieceModelIndex], pieceState->positionX,
                            arenaPieceVerticalOffsets[pieceState->pieceTypeIndex]
                                + ((pieceState->pieceTypeIndex >= 0 && pieceState->pieceTypeIndex < 10) ? (300.0f + arenaPieceBaseHeights[pieceState->pieceTypeIndex]) : pieceState->positionY),
                            pieceState->positionZ + zeroArenaPieceOffsets[pieceState->pieceTypeIndex]);
                        if (pieceState->pieceTypeIndex >= 10) {
                            Hu3DModelRotSet(object->mdlId[pieceModelIndex], 0.0f, pieceState->rotationY, 0.0f);
                        }
                        break;
                    case 1:
                        animationProgress = pieceState->animationFrame / 90.0;
                        remainingProgress = 1.0f - animationProgress;
                        previousPositionX = pieceState->positionX;
                        previousPositionY = pieceState->positionY;
                        previousPositionZ = pieceState->positionZ;
                        pieceState->positionX = (460.0f + 1135.0f * remainingProgress) * (pieceState->positionX < 0.0f ? -1 : 1);
                        pieceState->positionY = 100.0f * animationProgress + 500.0f * remainingProgress;
                        pieceState->positionZ = 800.0f - 695.0f * remainingProgress;
                        pieceState->positionDeltaX = pieceState->positionX - previousPositionX;
                        pieceState->positionDeltaY = pieceState->positionY - previousPositionY;
                        pieceState->positionDeltaZ = pieceState->positionZ - previousPositionZ;
                        if (pieceState->animationFrame >= 90) {
                            pieceState->pieceState = 2;
                            pieceState->animationFrame = 0;
                        }
                        Hu3DModelPosSet(object->mdlId[pieceModelIndex], pieceState->positionX, pieceState->positionY + arenaPieceVerticalOffsets[pieceState->pieceTypeIndex],
                            pieceState->positionZ + zeroArenaPieceOffsets[pieceState->pieceTypeIndex]);
                        Hu3DModelRotSet(object->mdlId[pieceModelIndex], 9.0f, (pieceState->positionX < 0.0f) ? -31 : 31, (pieceState->positionX < 0.0f) ? -18 : 18);
                        break;
                    case 2:
                        pieceState->positionDeltaY -= (pieceState->pieceTypeIndex != 9) ? 0.625 : 0.9;
                        pieceState->positionX += pieceState->positionDeltaX;
                        pieceState->positionY += pieceState->positionDeltaY;
                        pieceState->positionZ += pieceState->positionDeltaZ;
                        if (pieceState->positionY <= arenaPieceBaseHeights[pieceState->pieceTypeIndex]) {
                            pieceState->positionY = -arenaPieceBaseHeights[pieceState->pieceTypeIndex];
                            pieceState->positionDeltaY = 0.0f;
                            pieceState->pieceState = 6;
                            pieceState->animationFrame = 0;
                            Hu3DModelPosSet(object->mdlId[pieceModelIndex], pieceState->positionX,
                                pieceState->positionY + arenaPieceVerticalOffsets[pieceState->pieceTypeIndex] + arenaPieceBaseHeights[pieceState->pieceTypeIndex]
                                    + arenaPieceBaseHeights[pieceState->pieceTypeIndex],
                                pieceState->positionZ + zeroArenaPieceOffsets[pieceState->pieceTypeIndex]);
                            Hu3DModelRotSet(object->mdlId[pieceModelIndex], 0.0f, (pieceState->positionX < 0.0f) ? -31 : 31, 0.0f);
                        }
                        else {
                            Hu3DModelPosSet(object->mdlId[pieceModelIndex], pieceState->positionX, pieceState->positionY + arenaPieceVerticalOffsets[pieceState->pieceTypeIndex],
                                pieceState->positionZ + zeroArenaPieceOffsets[pieceState->pieceTypeIndex]);
                        }
                        break;
                    case 7:
                        pieceState->positionDeltaY -= 1.0f;
                        pieceState->positionY += pieceState->positionDeltaY;
                        pieceState->positionZ += (pieceState->pieceTypeIndex >= 10) ? playerMoveSpeed : 0.0f;
                        if (pieceState->positionY <= -800.0f) {
                            Hu3DModelAttrSet(object->mdlId[pieceModelIndex], 1);
                            if (pieceState->pieceTypeIndex == 9) {
                                Hu3DMotionSet(object->mdlId[pieceModelIndex], object->mtnId[innerIndex + 22]);
                                Hu3DMotionTimeSet(object->mdlId[pieceModelIndex], 0.0f);
                                Hu3DMotionSpeedSet(object->mdlId[pieceModelIndex], 2.0f);
                            }
                            pieceState->pieceState = 0;
                        }
                        Hu3DModelPosSet(object->mdlId[pieceModelIndex], pieceState->positionX,
                            (pieceState->pieceTypeIndex >= 0 && pieceState->pieceTypeIndex < 9) ? (300.0f + arenaPieceBaseHeights[pieceState->pieceTypeIndex]) : pieceState->positionY,
                            pieceState->positionZ + zeroArenaPieceOffsets[pieceState->pieceTypeIndex]);
                        Hu3DModelPosSet(object->mdlId[pieceModelIndex], pieceState->positionX, pieceState->positionY, pieceState->positionZ);
                        if (pieceState->pieceTypeIndex >= 10) {
                            Hu3DModelRotSet(object->mdlId[pieceModelIndex], 0.0f, pieceState->rotationY, 0.0f);
                        }
                        break;
                }
                if (pieceState->pieceTypeIndex >= 0 && pieceState->pieceTypeIndex <= 8 && (pieceState->pieceState == 4 || pieceState->pieceState == 5)) {
                    motionFrame = Hu3DMotionTimeGet(object->mdlId[pieceModelIndex]);
                    if (pieceState->pieceTypeIndex >= 0 && pieceState->pieceTypeIndex <= 2 && (motionFrame == 26 || motionFrame == -41) && playerStartPositions[3].positionCount < 3) {
                        playerStartPositions[3].positions[playerStartPositions[3].positionCount] = 0.0f;
                        playerStartPositions[3].positionCount++;
                    }
                    if (pieceState->pieceTypeIndex >= 3 && pieceState->pieceTypeIndex <= 5 && (motionFrame == 23 || motionFrame == -39) && playerStartPositions[2].positionCount < 3) {
                        playerStartPositions[2].positions[playerStartPositions[2].positionCount] = 0.0f;
                        playerStartPositions[2].positionCount++;
                    }
                    if (pieceState->pieceTypeIndex >= 6 && pieceState->pieceTypeIndex <= 8 && (motionFrame == 20 || motionFrame == -29) && playerStartPositions[1].positionCount < 3) {
                        playerStartPositions[1].positions[playerStartPositions[1].positionCount] = 0.0f;
                        playerStartPositions[1].positionCount++;
                    }
                }
                if (outerIndex == 9 && roundWinner != 0) {
                    HU3DMODEL *particleModel = &Hu3DData[object->mdlId[innerIndex + 39]];
                    HU3DPARTICLE *particleEmitter = particleModel->hookData;

                    if (particleEmitter->count == 63) {
                        HU3DMODEL *particleSourceModel = &Hu3DData[object->mdlId[pieceModelIndex]];
                        HU3DMODEL *particleTargetModel = &Hu3DData[object->mdlId[innerIndex + 39]];

                        particleTargetModel->pos.x = particleSourceModel->pos.x;
                        particleTargetModel->pos.y = particleSourceModel->pos.y;
                        particleTargetModel->pos.z = particleSourceModel->pos.z;
                        particleTargetModel->rot.x = particleSourceModel->rot.x;
                        particleTargetModel->rot.y = particleSourceModel->rot.y;
                        particleTargetModel->rot.z = particleSourceModel->rot.z;
                    }
                }
                if (pieceState->drawDelayFrames == 0 && (roundPhase == 1004 || roundPhase == 1005)) {
                    if (pieceState->pieceTypeIndex != 9
                        || (pieceState->pieceState != 1 && pieceState->pieceState != 2
                            && ((pieceState->pieceState != 3 && pieceState->pieceState != 6) || playerStates[innerIndex]->playerActionState > 5 || playerStates[innerIndex]->playerActionState < 3))) {
                        pieceState->drawDelayFrames = 1;
                    }
                }
                pieceState->drawDelayFrames += (pieceState->drawDelayFrames > 0);
                if (pieceState->drawDelayFrames != 0 && (pieceState->drawDelayFrames % 2 == 0 || pieceState->drawDelayFrames > 25)) {
                    Hu3DModelAttrSet(object->mdlId[pieceModelIndex], 1);
                }
                else {
                    Hu3DModelAttrReset(object->mdlId[pieceModelIndex], 1);
                }
            }
        }
    }
    for (outerIndex = 0; outerIndex < 11; outerIndex++) {
        if (playerRoundScores[outerIndex] == 1) {
            Hu3DMotionShiftSet(object->mdlId[outerIndex + 8], object->mtnId[0], 0.0f, 60.0f, HU3D_MOTATTR_LOOP);
        }
        playerRoundScores[outerIndex] -= (playerRoundScores[outerIndex] > 0);
    }
    roundWinCounts[0] -= (roundWinCounts[0] > 0);
    roundWinCounts[1] -= (roundWinCounts[1] > 0);
    for (outerIndex = 0; outerIndex < 3; outerIndex++) {
        for (innerIndex = 0; innerIndex < 8; innerIndex++) {
            s32 cooldownModelIndex = temporaryModelCooldowns[outerIndex][innerIndex].modelIndex + 21;

            temporaryModelCooldowns[outerIndex][innerIndex].cooldownFrames++;
            if (temporaryModelCooldowns[outerIndex][innerIndex].cooldownFrames >= 520) {
                if (temporaryModelCooldowns[outerIndex][innerIndex].modelIndex != -1) {
                    Hu3DModelAttrReset(object->mdlId[temporaryModelCooldowns[outerIndex][innerIndex].modelIndex + 21], 1);
                }
                temporaryModelCooldowns[outerIndex][innerIndex].cooldownFrames = 0;
                temporaryModelCooldowns[outerIndex][innerIndex].modelIndex = -1;
                for (randomModelAttempts = 0; randomModelAttempts < 14; randomModelAttempts++) {
                    s32 occupiedModelIndex;
                    s32 candidateModelIndex = rand8() % 18;

                    for (occupiedModelIndex = 0; occupiedModelIndex < 24; occupiedModelIndex++) {
                        if (candidateModelIndex == temporaryModelCooldowns[occupiedModelIndex / 8][occupiedModelIndex % 8].modelIndex) {
                            break;
                        }
                    }
                    if (occupiedModelIndex == 24) {
                        temporaryModelCooldowns[outerIndex][innerIndex].modelIndex = candidateModelIndex;
                        break;
                    }
                }
                if (temporaryModelCooldowns[outerIndex][innerIndex].modelIndex != -1) {
                    Hu3DModelAttrReset(object->mdlId[temporaryModelCooldowns[outerIndex][innerIndex].modelIndex + 21], 1);
                }
            }
            cooldownModelIndex = temporaryModelCooldowns[outerIndex][innerIndex].modelIndex + 21;
            cooldownProgress = temporaryModelCooldowns[outerIndex][innerIndex].cooldownFrames / 520.0f;
            if (temporaryModelCooldowns[outerIndex][innerIndex].modelIndex != -1) {
                switch (outerIndex) {
                    case 0:
                        Hu3DModelPosSet(object->mdlId[cooldownModelIndex], 2000.0f - 4000.0f * cooldownProgress, -490.0f, 1000.0f);
                        break;
                    case 1:
                        Hu3DModelPosSet(object->mdlId[cooldownModelIndex], -1200.0f, -290.0f, 2000.0f - 4000.0f * cooldownProgress);
                        break;
                    case 2:
                        Hu3DModelPosSet(object->mdlId[cooldownModelIndex], 1200.0f, -290.0f, 2000.0f - 4000.0f * cooldownProgress);
                        break;
                }
            }
        }
    }
    for (outerIndex = 0; outerIndex < 2; outerIndex++) {
        espBankSet(arenaPieceSprites[outerIndex * 7 + 2], playerCollectedCoins[outerIndex] % 10);
        espBankSet(arenaPieceSprites[outerIndex * 7 + 3], (playerCollectedCoins[outerIndex] / 10) % 10);
    }
    for (outerIndex = 0; outerIndex < 4; outerIndex++) {
        HU3DMODEL *playerModel = &Hu3DData[playerObjects[outerIndex]->mdlId[0]];
        float shadowMotionProgress = 0.0f;
        float currentMotionMatch = (playerModel->motId == playerObjects[outerIndex]->mtnId[3]) ? 0 : -1;
        float shiftedMotionMatch = (playerModel->motIdShift == playerObjects[outerIndex]->mtnId[3]) ? 0 : -1;
        float currentMotionProgress = (currentMotionMatch == -1.0f)   ? 0.0f
            : (playerModel->motWork.time < 4.0f)  ? 0.0f
            : (playerModel->motWork.time > 28.0f) ? 1.0f
                                         : (playerModel->motWork.time - 4.0f) / 24.0f;
        float shiftedMotionProgress = (shiftedMotionMatch == -1.0f)   ? 0.0f
            : (playerModel->motShiftWork.time < 4.0f)  ? 0.0f
            : (playerModel->motShiftWork.time > 28.0f) ? 1.0f
                                         : (playerModel->motShiftWork.time - 4.0f) / 24.0f;
        float motionBlendWeight = (playerModel->motIdShift == -1) ? 0.0f : (playerModel->motOvlWork.end == 0.0f) ? 1.0f : (playerModel->motOvlWork.start / playerModel->motOvlWork.end);

        shadowMotionProgress = currentMotionProgress * (1.0f - motionBlendWeight) + shiftedMotionProgress * motionBlendWeight;
        if (!(playerModel->attr & 1) && playerStates[outerIndex]->playerActionState != 3 && playerStates[outerIndex]->playerActionState != 4 && playerObjects[outerIndex]->trans.z < 1400.0f) {
            Hu3DModelAttrReset(playerObjects[outerIndex]->mdlId[1], 1);
            Hu3DModelPosSet(playerObjects[outerIndex]->mdlId[1], playerObjects[outerIndex]->trans.x, (playerObjects[outerIndex]->trans.z < 620.0f) ? 5 : -295,
                playerObjects[outerIndex]->trans.z - 165.0f * shadowMotionProgress);
            Hu3DModelScaleSet(
                playerObjects[outerIndex]->mdlId[1], 0.75 * (playerStates[outerIndex]->characterId == 5 ? 1.25 : 1.0), 1.0f, 1.25 * (playerStates[outerIndex]->characterId == 5 ? 1.25 : 1.0));
        }
        else {
            Hu3DModelAttrSet(playerObjects[outerIndex]->mdlId[1], 1);
        }
    }
}

void UpdateMinigameRound(OMOBJ *object)
{
    PlayerObjectState *playerStates[4];
    s32 sequenceId;
    s32 activePlayerCount;
    s32 playersAwaitingFinish;
    s32 playerIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerStates[playerIndex] = playerObjects[playerIndex]->data;
    }
    for (playerIndex = 0; playerIndex < 11; playerIndex++) {
        s32 roundSoundEffectIds[] = { 0x60A, 0x60B, 0x60C, 0x60D, 0x60E, 0x60F, 0x611, 0x610, 0x007, 0x013, 0x612 };

        if (playerStartPositions[playerIndex].positionCount > 3) {
            playerStartPositions[playerIndex].positionCount = 3;
        }
        if (playerStartPositions[playerIndex].positionCount > 0) {
            sequenceId = HuAudFXPlay(roundSoundEffectIds[playerIndex]);
            playerStartPositions[playerIndex].positionCount--;
        }
    }
    if (omSysExitReq != 0 && WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = DestroyPlayerObjects;
    }
    switch (roundPhase) {
        case 1000:
            roundTimer += 1.0f;
            if (roundTimer == 5.0f) {
                roundSoundSequenceId = HuAudFXPlay(0x609);
            }
            if (WipeStatGet() == 0) {
                roundPhase++;
                roundTimer = 0.0f;
            }
            break;
        case 1001:
            roundTimer += 1.0f;
            if (180.0f < roundTimer) {
                roundPhase++;
                roundTimer = 0.0f;
            }
            break;
        case 1002:
            roundTimer += 1.0f;
            if (roundTimer == 5.0f && roundSequenceId < 0) {
                roundSequenceId = MGSeqStartCreate();
            }
            if (roundTimer >= 5.0f && roundMusicSequenceId == -1 && (MGSeqStatGet(roundSequenceId) & 0x10)) {
                roundMusicSequenceId = HuAudSeqPlay(0x47);
            }
            if (roundTimer >= 5.0f && MGSeqStatGet(roundSequenceId) == 0 && roundSequenceId >= 0) {
                roundPhase++;
                roundCountdown = REFRESH_RATE * 30;
                roundResultSequenceId = MGSeqTimerCreate(roundCountdown / REFRESH_RATE);
                roundSequenceId = -1;
                roundTimer = 0.0f;
                for (playerIndex = 0; playerIndex < 16; playerIndex++) {
                    espDispOn(arenaPieceSprites[playerIndex]);
                }
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    playerStates[playerIndex]->playerActionState = 1;
                }
            }
            break;
        case 1003:
            roundTimer += 1.0f;
            MGSeqParamSet(roundResultSequenceId, 1, (roundCountdown + REFRESH_RATE - 2) / REFRESH_RATE);
            roundCountdown--;
            if (roundCountdown <= 0) {
                roundPhase = 1004;
                roundTimer = 0.0f;
                roundState = 4;
            }
            break;
        case 1004:
            if (roundSequenceId < 0 && roundTimer == 0.0f) {
                MGSeqParamSet(roundResultSequenceId, 2, -1);
                roundSequenceId = MGSeqFinishCreate();
                if (roundMusicSequenceId != -1) {
                    HuAudSeqFadeOut(roundMusicSequenceId, 100);
                }
            }
            else {
                activePlayerCount = 0;
                playersAwaitingFinish = 0;
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if (playerStates[playerIndex]->playerActionState == 1 || (playerStates[playerIndex]->playerActionState >= 2 && playerStates[playerIndex]->playerActionState <= 5)) {
                        activePlayerCount++;
                    }
                    if (playerIndex == 0 && ((playerStates[0]->playerActionState >= 2 && playerStates[0]->playerActionState <= 4) || (playerStates[0]->playerActionState == 4 && arenaPieceStates[9][0].positionZ < 70.0f))) {
                        playersAwaitingFinish++;
                    }
                    else if (playerIndex != 0 && playerStates[playerIndex]->playerActionState >= 2 && playerStates[playerIndex]->playerActionState <= 4) {
                        playersAwaitingFinish++;
                    }
                }
                if (MGSeqStatGet(roundSequenceId) == 0 &&
                      ((roundTimer >= (VERSION_PAL ? 125.0f : 150.0f) && activePlayerCount == 0 && playersAwaitingFinish == 0)
                    || (roundTimer >= REFRESH_RATE_F * 4 && playersAwaitingFinish == 0)
                    || (roundTimer >= REFRESH_RATE_F * 6))) {
                    roundPhase++;
                    roundTimer = 0.0f;
                    GWPlayerCoinCollectSet(playerStates[0]->controllerConfigIndex, playerCollectedCoins[0]);
                    GWPlayerCoinCollectSet(playerStates[1]->controllerConfigIndex, playerCollectedCoins[1]);
                    GWPlayerCoinCollectSet(playerStates[2]->controllerConfigIndex, playerCollectedCoins[1]);
                    GWPlayerCoinCollectSet(playerStates[3]->controllerConfigIndex, playerCollectedCoins[1]);
                    if (playerCollectedCoins[0] == 0 && playerCollectedCoins[1] == 0) {
                        HuAudSStreamPlay(4);
                    }
                    else {
                        HuAudSStreamPlay(1);
                    }
                }
            }
            roundTimer += 1.0f;
            break;
        case 1005:
            roundTimer += 1.0f;
            if (REFRESH_RATE_F * 3.5f < roundTimer) {
                HuAudFXStop(roundSoundSequenceId);
                roundSequenceId = -1;
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = DestroyPlayerObjects;
            }
            break;
        default:
            OSReport("*** main mode error(%d)!!\n", roundPhase);
            break;
    }
}

void UpdateSparkleParticles(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    HU3DPARTICLEDATA *particleData;
    float particleAngle;
    float particleSpeed;
    s16 particleIndex;

    if (particle->count < 50) {
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->time = particle->count + 41;
            particleData->color.a = 0;
            particleData->scale = 0.0f;
        }
    }
    if (particle->count >= 255) {
        particleData = particle->data;
        for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->time = 255;
            particleData->color.a = 0;
            particleData->scale = 0.0f;
        }
    }
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (!particleData->scale) {
            break;
        }
    }
    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->time == 104) {
            particleData->pos.x = particleData->pos.z = 0.0f;
            particleData->pos.y = 72.0f;
            particleAngle = frandu8() * 360 / 256;
            particleSpeed = 0.5 + (rand8() / 256.0) * 1.5;
            particleData->pos.x = 75.0 * sind(particleAngle);
            particleData->pos.z = 75.0 * cosd(particleAngle);
            particleData->vel.x = particleSpeed * sind(particleAngle) * 2.0;
            particleData->vel.y = 3.0 + (rand8() / 256.0) * 1.5;
            particleData->vel.z = particleSpeed * cosd(particleAngle) * 2.0;
            particleData->color.r = 0xFF;
            particleData->color.g = 0xFF;
            particleData->color.b = 0xFF;
            particleData->color.a = 0x60;
            particleData->scale = 50.0f;
            VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
        }
        if (particleData->time < 240) {
            VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
            particleData->vel.x *= 0.9f;
            particleData->vel.z *= 0.9f;
            particleData->vel.y *= 0.9f;
            particleData->color.a = particleData->color.a * 16 / 17;
            particleData->vel.z *= 0.99f;
            particleData->scale += 2.0f;
            if (particleData->scale <= 0.0f) {
                particleData->scale = 0.0f;
            }
            if (particleData->color.a <= 3) {
                particleData->color.a = 0;
                particleData->scale = 0.0f;
            }
        }
        particleData->time += (particleData->time < 0x8000);
    }
}

void InitializeArenaObjects(OMOBJ *object)
{
    s32 modelIndex;
    s32 rowIndex;
    s32 columnIndex;
    s32 linkedModelIndex;

    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M422, 18));
    omSetTra(object, 0.0f, 0.0f, 0.0f);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M422, 17));
    Hu3DModelPosSet(object->mdlId[1], 0.0f, 300.0f, -100.0f);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[1], 0);
    Hu3DMotionSpeedSet(object->mdlId[1], 1.5f);
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M422, 16));
    Hu3DModelPosSet(object->mdlId[3], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(object->mdlId[3], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(object->mdlId[3], 0);
    Hu3DMotionSpeedSet(object->mdlId[3], 1.5f);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M422, 19));
    Hu3DModelAttrSet(object->mdlId[4], HU3D_MOTATTR_LOOP);
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M422, 20));
    Hu3DModelPosSet(object->mdlId[5], 460.0f, 0.0f, 800.0f);
    Hu3DModelAttrReset(object->mdlId[5], HU3D_MOTATTR_LOOP);
    Hu3DMotionTimeSet(object->mdlId[5], Hu3DMotionMaxTimeGet(object->mdlId[5]));
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M422, 21));
    Hu3DModelPosSet(object->mdlId[6], -460.0f, 0.0f, 800.0f);
    Hu3DModelAttrReset(object->mdlId[6], HU3D_MOTATTR_LOOP);
    Hu3DMotionTimeSet(object->mdlId[6], Hu3DMotionMaxTimeGet(object->mdlId[6]));
    object->mdlId[7] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M422, 22));
    Hu3DModelAttrSet(object->mdlId[7], HU3D_MOTATTR_LOOP);
    object->mdlId[8] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M422, 23));
    object->mtnId[0] = Hu3DJointMotionFile(object->mdlId[8], DATA_MAKE_NUM(DATADIR_M422, 24));
    object->mtnId[11] = Hu3DJointMotionFile(object->mdlId[8], DATA_MAKE_NUM(DATADIR_M422, 25));
    for (rowIndex = 0; rowIndex < 11; rowIndex++) {
        if (rowIndex != 0) {
            object->mdlId[rowIndex + 8] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M422, 23));
            object->mtnId[rowIndex] = Hu3DJointMotionFile(object->mdlId[rowIndex + 8], DATA_MAKE_NUM(DATADIR_M422, 24));
            object->mtnId[rowIndex + 11] = Hu3DJointMotionFile(object->mdlId[rowIndex + 8], DATA_MAKE_NUM(DATADIR_M422, 25));
        }
        Hu3DModelPosSet(object->mdlId[rowIndex + 8], rowIndex * 100 - 500, 550.0f, 0.0f);
        Hu3DMotionSet(object->mdlId[rowIndex + 8], object->mtnId[0]);
        Hu3DModelAttrSet(object->mdlId[rowIndex + 8], HU3D_MOTATTR_LOOP);
        Hu3DModelAttrReset(object->mdlId[rowIndex + 8], HU3D_MOTATTR_PAUSE);
        Hu3DMotionTimeSet(object->mdlId[rowIndex + 8], rand8() % (s32)Hu3DMotionMaxTimeGet(object->mdlId[rowIndex + 8]));
    }
    object->mdlId[19] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M422, 26));
    Hu3DModelPosSet(object->mdlId[19], 1800.0f, 0.0f, -22.0f);
    Hu3DModelAttrSet(object->mdlId[19], HU3D_MOTATTR_LOOP);
    object->mdlId[20] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M422, 27));
    Hu3DModelPosSet(object->mdlId[20], -1800.0f, 0.0f, -22.0f);
    Hu3DModelAttrSet(object->mdlId[20], HU3D_MOTATTR_LOOP);
    for (rowIndex = 0; rowIndex < 10; rowIndex++) {
        for (columnIndex = 0; columnIndex < 4; columnIndex++) {
            s32 standardPieceModelIds[] = { DATA_MAKE_NUM(DATADIR_M422, 41), DATA_MAKE_NUM(DATADIR_M422, 42), DATA_MAKE_NUM(DATADIR_M422, 43),
                DATA_MAKE_NUM(DATADIR_M422, 44), DATA_MAKE_NUM(DATADIR_M422, 45), DATA_MAKE_NUM(DATADIR_M422, 46), DATA_MAKE_NUM(DATADIR_M422, 47),
                DATA_MAKE_NUM(DATADIR_M422, 48), DATA_MAKE_NUM(DATADIR_M422, 49), DATA_MAKE_NUM(DATADIR_M422, 30) };
            s32 characterPieceModelIds[] = { DATA_MAKE_NUM(DATADIR_M422, 30), DATA_MAKE_NUM(DATADIR_M422, 31), DATA_MAKE_NUM(DATADIR_M422, 32),
                DATA_MAKE_NUM(DATADIR_M422, 33), DATA_MAKE_NUM(DATADIR_M422, 34), DATA_MAKE_NUM(DATADIR_M422, 35), DATA_MAKE_NUM(DATADIR_M422, 37),
                DATA_MAKE_NUM(DATADIR_M422, 36) };

            modelIndex = rowIndex * 4 + 43 + columnIndex;
            if (rowIndex == 9) {
                PlayerObjectState *playerState = playerObjects[columnIndex]->data;

                object->mdlId[modelIndex] = Hu3DModelCreateFile(characterPieceModelIds[playerState->characterId]);
            }
            else if (columnIndex == 0) {
                object->mdlId[modelIndex] = Hu3DModelCreateFile(standardPieceModelIds[rowIndex]);
            }
            else {
                object->mdlId[modelIndex] = Hu3DModelLink(object->mdlId[modelIndex - columnIndex]);
            }
            Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrReset(object->mdlId[modelIndex], HU3D_MOTATTR_LOOP);
            Hu3DMotionTimeSet(object->mdlId[modelIndex], 0.0f);
            Hu3DMotionSpeedSet(object->mdlId[modelIndex], (rowIndex != 9) ? 0.9375 : 1.0);
            if (rowIndex == 9) {
                object->mtnId[columnIndex + 22] = Hu3DJointMotionFile(object->mdlId[modelIndex], DATA_MAKE_NUM(DATADIR_M422, 38));
                object->mtnId[columnIndex + 26] = Hu3DJointMotionFile(object->mdlId[modelIndex], DATA_MAKE_NUM(DATADIR_M422, 39));
                object->mtnId[columnIndex + 30] = Hu3DJointMotionFile(object->mdlId[modelIndex], DATA_MAKE_NUM(DATADIR_M422, 40));
                Hu3DMotionSet(object->mdlId[modelIndex], object->mtnId[columnIndex + 22]);
                Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_MOTATTR_PAUSE);
            }
        }
        if (rowIndex <= 8) {
            for (linkedModelIndex = 0; linkedModelIndex < 2; linkedModelIndex++) {
                object->mdlId[rowIndex * 2 + 21 + linkedModelIndex] = Hu3DModelLink(object->mdlId[rowIndex * 4 + 43]);
                Hu3DModelAttrSet(object->mdlId[rowIndex * 2 + 21 + linkedModelIndex], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE);
            }
        }
    }
    for (rowIndex = 0; rowIndex < 2; rowIndex++) {
        for (columnIndex = 0; columnIndex < 10; columnIndex++) {
            s32 bonusPieceModelIds[] = { DATA_MAKE_NUM(DATADIR_M422, 28), DATA_MAKE_NUM(DATADIR_M422, 29) };

            modelIndex = rowIndex * 10 + 83 + columnIndex;
            if (columnIndex == 0) {
                object->mdlId[modelIndex] = Hu3DModelCreateFile(bonusPieceModelIds[rowIndex]);
            }
            else {
                object->mdlId[modelIndex] = Hu3DModelLink(object->mdlId[modelIndex - columnIndex]);
            }
            Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_MOTATTR_LOOP);
            Hu3DModelAttrSet(object->mdlId[modelIndex], 1);
        }
    }
    for (rowIndex = 0; rowIndex < 3; rowIndex++) {
        for (columnIndex = 0; columnIndex < 8; columnIndex++) {
            temporaryModelCooldowns[rowIndex][columnIndex].cooldownFrames = columnIndex * 520 / 8;
            temporaryModelCooldowns[rowIndex][columnIndex].modelIndex = -1;
        }
    }
    for (rowIndex = 0; rowIndex < 16; rowIndex++) {
        s32 coinSpriteXOffsets[] = { 88, 473 };
        s32 coinSpriteYOffsets[] = { 78, 70 };
        s32 coinSpritePositionsX[] = { 0, 0, 42, 22, -20, 2, -54, 0, 0, 55, 35, -7, 15, -51, -65, -37 };
        s32 coinSpritePositionsY[] = { 0, 0, 1, 1, 1, 1, -2, 0, 0, 9, 9, 9, 9, -14, 10, 10 };

        if (arenaPieceResourceIds[rowIndex] == DATA_MAKE_NUM(DATADIR_MGCONST, 12) || arenaPieceResourceIds[rowIndex] == DATA_MAKE_NUM(DATADIR_MGCONST, 0)) {
            PlayerObjectState *playerState = playerObjects[(rowIndex == 6) ? 0 : (rowIndex == 13) ? 1 : (rowIndex == 14) ? 2 : 3]->data;

            arenaPieceSprites[rowIndex] = espEntry(arenaPieceResourceIds[rowIndex] + playerState->characterId, 0, 0);
        }
        else {
            arenaPieceSprites[rowIndex] = espEntry(arenaPieceResourceIds[rowIndex], 0, 0);
        }
        espDrawNoSet(arenaPieceSprites[rowIndex], 0);
        espDispOff(arenaPieceSprites[rowIndex]);
        espAttrSet(arenaPieceSprites[rowIndex], 1);
        espBankSet(arenaPieceSprites[rowIndex], 0);
        espPriSet(arenaPieceSprites[rowIndex], 128 - rowIndex);
        espPosSet(arenaPieceSprites[rowIndex], coinSpritePositionsX[rowIndex] + coinSpriteXOffsets[rowIndex < 7 ? 0 : 1], coinSpritePositionsY[rowIndex] + coinSpriteYOffsets[rowIndex < 7 ? 0 : 1]);
        switch (rowIndex >= 7 ? rowIndex - 7 : rowIndex) {
            case 0:
                espTPLvlSet(arenaPieceSprites[rowIndex], 0.5f);
                espColorSet(arenaPieceSprites[rowIndex], 0, 0, 0);
                break;
            case 5:
                espBankSet(arenaPieceSprites[rowIndex], 10);
                break;
        }
    }
    for (rowIndex = 0; rowIndex < 4; rowIndex++) {
        s32 particleModelIndex = rowIndex + 39;

        object->mdlId[particleModelIndex] = Hu3DParticleCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_EFFECT, 3), HU_MEMNUM_OVL)), 40);
        Hu3DModelPosSet(object->mdlId[particleModelIndex], 2000.0f, -2000.0f, -2000.0f);
        Hu3DModelScaleSet(object->mdlId[particleModelIndex], 1.0f, 1.0f, 1.0f);
        Hu3DModelLayerSet(object->mdlId[particleModelIndex], 3);
        Hu3DModelAttrReset(object->mdlId[particleModelIndex], 1);
        Hu3DParticleColSet(object->mdlId[particleModelIndex], 0xFF, 0xFF, 0xFF);
        Hu3DParticleScaleSet(object->mdlId[particleModelIndex], 5.0f);
        Hu3DParticleHookSet(object->mdlId[particleModelIndex], UpdateSparkleParticles);
        Hu3DParticleCntSet(object->mdlId[particleModelIndex], 45);
    }
    for (rowIndex = 0; rowIndex < 4; rowIndex++) {
        s32 playerStateIndex = 9;
        s32 playerModelIndex = 43 + playerStateIndex * 4 + rowIndex;

        arenaPieceStates[playerStateIndex][rowIndex].positionX = playerSpawnTransforms[rowIndex][0];
        arenaPieceStates[playerStateIndex][rowIndex].positionY = (rowIndex == 0 ? 300 : 0);
        arenaPieceStates[playerStateIndex][rowIndex].positionZ = (rowIndex == 0 ? 50 : 800);
        arenaPieceStates[playerStateIndex][rowIndex].pieceTypeIndex = 9;
        arenaPieceStates[playerStateIndex][rowIndex].pieceState = (rowIndex == 0 ? 3 : 6);
        arenaPieceStates[playerStateIndex][rowIndex].animationFrame = 0;
        arenaPieceStates[playerStateIndex][rowIndex].positionDeltaX = arenaPieceStates[playerStateIndex][rowIndex].positionDeltaY = arenaPieceStates[playerStateIndex][rowIndex].positionDeltaZ = 0.0f;
        Hu3DMotionTimeSet(object->mdlId[playerModelIndex], 0.0f);
        Hu3DModelAttrSet(object->mdlId[playerModelIndex], HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(object->mdlId[playerModelIndex], 1);
        Hu3DModelRotSet(object->mdlId[playerModelIndex], 0.0f, 0.0f, 0.0f);
    }
    object->objFunc = UpdateArenaObjects;
}

void InitializePlayerObject(OMOBJ *object)
{
    s32 groupPlayerIndices[] = { 0, 1, 2, 3 };
    PlayerObjectState *playerState;
    s32 groupPlayerCount;
    s32 playerIndex;
    s32 groupIndex;

    groupPlayerCount = 0;
    playerState = object->data;
    memset(playerState, 0, sizeof(*playerState));
    for (groupIndex = 0; groupIndex < 4; groupIndex++) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (groupIndex == GWPlayerCfg[playerIndex].group) {
                groupPlayerIndices[groupPlayerCount] = playerIndex;
                groupPlayerCount++;
            }
        }
    }
    playerState->playerIndex = object->work[0];
    playerState->controllerConfigIndex = groupPlayerIndices[playerState->playerIndex];
    if (playerState->controllerConfigIndex == 4) {
        OSReport("m422:wrong player group\n");
        playerState->controllerConfigIndex = playerState->playerIndex;
    }
    playerState->characterId = GWPlayerCfg[playerState->controllerConfigIndex].character;
    playerState->controllerPort = (GWPlayerCfg[playerState->controllerConfigIndex].iscom ? -1 : GWPlayerCfg[playerState->controllerConfigIndex].pad_idx);
    playerState->aiDifficulty = (playerState->controllerPort != -1) ? 0 : GWPlayerCfg[playerState->controllerConfigIndex].diff;
    playerState->randomSeed = rand8();
    playerState->playerActionState = 0;
    object->mdlId[0] = CharModelCreate(playerState->characterId, playerState->playerIndex == 0 ? 2 : 2);
    Hu3DModelLayerSet(object->mdlId[0], 1);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrSet(object->mdlId[0], 1);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M422, 50));
    Hu3DModelPosSet(object->mdlId[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelLayerSet(object->mdlId[1], 0);
    playerState->currentMotionIndex = 0;
    playerState->forcedMotionIndex = -1;
    for (groupIndex = 0; groupIndex < 8; groupIndex++) {
        object->mtnId[groupIndex] = CharMotionCreate(playerState->characterId, playerMotionResourceIds[groupIndex] + (playerMotionCharacterOffsets[groupIndex] != 0 ? playerState->characterId : 0));
    }
    CharMotionVoiceOnSet(playerState->characterId, object->mtnId[6], 0);
    Hu3DMotionSet(object->mdlId[0], object->mtnId[3]);
    object->trans.x = playerSpawnTransforms[playerState->playerIndex][0];
    object->trans.y = playerSpawnTransforms[playerState->playerIndex][1];
    object->trans.z = playerSpawnTransforms[playerState->playerIndex][2];
    object->rot.y = playerSpawnTransforms[playerState->playerIndex][3];
    object->scale.x = object->scale.y = object->scale.z = 1.0f;
    object->rot.x = object->rot.z = 0.0f;
    object->objFunc = UpdatePlayerObject;
}

void InitializeMinigameRound(OMOBJ *object)
{
    roundPhase = 1000;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    object->objFunc = UpdateMinigameRound;
}

void ObjectSetup(void)
{
    OMOBJ *playerObject;
    s32 playerIndex;

    OSReport("******* M422 ObjectSetup *********\n");
    HuAudSndGrpSet(47);
    objectManager = omInitObjMan(600, 0x2000);
    omGameSysInit(objectManager);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 30.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    CZoom = 1676.6f;
    Center.x = 690.0f;
    Center.y = 1340.0f;
    Center.z = 1380.0f;
    CRot.x = -50.8125f;
    CRot.y = 40.0f;
    CRot.z = 0.0f;
    CZoom = 2126.6f;
    Center.x = 700.0f;
    Center.y = 870.0f;
    Center.z = 1320.0f;
    CRot.x = -37.25f;
    CRot.y = 40.0f;
    CRot.z = 0.0f;
    {
        Vec lightPosition = { -822.5f, 6743.0f, 4625.5f };
        Vec lightDirection = { 0.1545085f, -0.788462f, -0.5850064f };
        GXColor lightColor = { 0xFF, 0xFF, 0xFF, 0xFF };

        globalLightId = Hu3DGLightCreateV(&lightPosition, &lightDirection, &lightColor);
    }
    Hu3DGLightStaticSet(globalLightId, 1);
    Hu3DGLightInfinitytSet(globalLightId);
    roundSequenceId = -1;
    roundState = -1;
    arenaObjects = omAddObjEx(objectManager, 101, 103, 34, 1, InitializeArenaObjects);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObject = playerObjects[playerIndex] = omAddObjEx(objectManager, 100, 2, 8, 0, InitializePlayerObject);
        playerObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerObjectState), HU_MEMNUM_OVL);
        playerObject->work[0] = playerIndex;
    }
    viewExitObject = omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutView);
    minigameExitObject = omAddObjEx(objectManager, 102, 0, 0, -1, InitializeMinigameRound);
}

void DestroyPlayerObjects(OMOBJ *object)
{
    PlayerObjectState *playerState;
    s32 playerIndex;

    if (WipeStatGet() == 0) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerState = playerObjects[playerIndex]->data;
            CharModelKill(playerState->characterId);
        }
        if (roundSequenceId >= 0) {
            MGSeqKill(roundSequenceId);
        }
        MGSeqKillAll();
        HuAudAllStop();
        omOvlReturnEx(1, 1);
    }
}

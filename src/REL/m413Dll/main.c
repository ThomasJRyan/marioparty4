#include "game/audio.h"
#include "game/disp.h"
#include "game/gamework_data.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/wipe.h"

#include "game/hu3d.h"
#include "game/pad.h"
#include "game/sprite.h"


#include "ext_math.h"

#ifndef __MWERKS__
#include "string.h"
#include "game/hsfex.h"
#endif

s32 rand8(void);

typedef struct BoardNode BoardNode;

typedef struct BoardEvent BoardEvent;

typedef struct BoardEvent {
    u8 eventType;
    u8 nodeCount;
    u8 playerMask;
    s32 playerValues[4];
    BoardNode *firstNode;
    BoardNode *secondNode;
    BoardEvent *nextEvent;
} BoardEvent;

typedef struct PlayerWork {
    s32 cursorDirection;
    s32 cursorRepeatFrames;
    s32 buttonState;
    s32 buttonRepeatFrames;
    s32 playerSlot;
    s32 playerIndex;
    s32 characterId;
    s32 controllerIndex;
    s32 cpuDifficulty;
    s32 targetScore;
    s32 previousScore;
    s32 rankedScore;
    u8 reservedStateBytes[0x10];
    float displayedScore;
    s32 scoreHoldFrames;
    BoardNode *boardNodes[2];
    u8 cardLayout[8][2];
    u8 cardColumn;
    u8 cardState;
    u8 cardAnimation;
    u8 cardTypes[2];
    u8 cardAnimationFrames[2];
    s32 turnScore;
    s32 turnScoreTarget;
    u8 selectedLane;
    u8 inputDelay;
    u8 playerState;
    s32 selectionTimer;
    u8 inputEnabled;
    u8 turnComplete;
    s32 targetLane;
    s32 targetDirection;
    s32 aiDelay;
    s32 targetLaneHoldFrames;
    s32 currentLane;
    s32 laneChangeFrames;
    s32 playerFlags;
    float cameraX;
    float cameraY;
    float cameraZ;
    s16 playerSpriteGroup;
} PlayerWork;

typedef struct BoardNode {
    u8 objectKind;
    u8 stateFlags;
    u8 ownerGroup;
    u8 playerMask;
    u8 pathId;
    u8 animationFrame;
    s32 playerScores[4];
    union {
        s8 laneIndex;
        u8 characterId;
    };

    Vec position;
    u32 nodeValue;
    BoardEvent *associatedObject;
    BoardNode *nextGroupNode;
    BoardNode *nextChainNode;
    BoardNode *pairedNodeLeft;
    BoardNode *pairedNodeRight;
    BoardNode *pairedNodeLower;
    BoardNode *pairedNodeUpper;
    BoardNode *nextLaneNode;
    BoardNode *previousLaneNode;
    PlayerWork *ownerWork;
} BoardNode;

typedef struct ScorePopup {
    float screenX;
    float screenY;
    s32 scoreValue;
    s32 lifetimeFrames;
    u8 playerMask;
} ScorePopup;

typedef struct FallingPiece {
    u8 pieceType;
    Vec position;
    Vec velocity;
    u8 rotationPhase;
    u8 spinPhase;
} FallingPiece;

typedef struct CameraShakeState {
    s32 shakePhase;
    s32 shakeAmplitude;
    float previousHorizontalOffset;
    Vec shakeDirection;
} CameraShakeState;

typedef struct PendingVoiceEvent {
    s32 queuedSoundCount;
    float queuedPanValues[3];
} PendingVoiceEvent;

BoardNode *laneHeads[12];
BoardNode boardNodePool[238];
ScorePopup scorePopupPool[50];
s32 scorePopupHead;
FallingPiece fallingPiecePool[300];
s32 fallingPieceHead;
s32 playerModelHideTimers[5][2];
s32 playerModelHiddenCount;
CameraShakeState cameraShakeState;
PendingVoiceEvent pendingVoiceEvents[7];
OMOBJ *mainSequenceObject;
OMOBJ *playerObjects[4];
OMOBJ *boardObject;
float sequenceTimer;
s32 boardState;
HUPROCESS *objectManager;
s16 minigameSequenceId;
s32 boardLightId;
s32 winningPlayer;
s16 popupSpriteGroup;
s32 winnerSoundFadeTimer;
s32 winnerSoundHandles[2];
s32 frameCounter;
s32 unusedState;

s32 sequenceAudioId = -1;
s32 winnerSoundEffects[2] = { -1, -1 };
s32 playerOrderByRank[4] = { 0, 1, 2, 3 };
float boardCameraDefaults[7] = { 1000, 1000, 1000, 0, 0, 0, 30 };

const Vec boardCameraStartPositions[2] = { 0, 7955, 3436, 0, -0.898794f, -0.43837115f };

const GXColor boardCameraClearColor = { 0, 0, 0, 255 };

const Vec boardCameraAlternatePositions[2] = { 0, 5966.25f, 3436, 0, -0.6740955f, -0.43837115f };

const GXColor boardCameraAlternateClearColor = { 0, 0, 0, 255 };

void TransformCamerasWithControllerInput(s32 cameraSelectionMask)
{
    Mtx44 cameraTransform;
    Mtx44 pitchMatrix;
    Mtx44 yawMatrix;
    Mtx44 rotationMatrix;
    Mtx44 translationMatrix;
    HU3DCAMERA *camera;
    s16 cameraIndex;
    s16 cameraMask;
    s16 unusedCameraRegister0;
    s16 unusedCameraRegister1;
    s16 unusedCameraRegister2;
    s16 unusedCameraRegister3;

    MTXRotDeg(yawMatrix, 'y', HuPadSubStkX[0] / 16.0);
    MTXRotDeg(pitchMatrix, 'x', HuPadSubStkY[0] / -16.0);
    MTXTrans(translationMatrix, (((HuPadBtn[0] & PAD_BUTTON_RIGHT) != 0) - ((HuPadBtn[0] & PAD_BUTTON_LEFT) != 0)) * 10.0,
        (((HuPadBtn[0] & PAD_BUTTON_UP) != 0) - ((HuPadBtn[0] & PAD_BUTTON_DOWN) != 0)) * 10.0, ((HuPadTrigL[0] - HuPadTrigR[0]) / 128.0) * 30.0);
    MTXConcat(pitchMatrix, yawMatrix, rotationMatrix);
    MTXConcat(translationMatrix, rotationMatrix, cameraTransform);
    for (cameraIndex = 0, cameraMask = 1; cameraIndex < 16; cameraIndex++, cameraMask <<= 1) {
        if (cameraSelectionMask & cameraMask) {
            camera = &Hu3DCamera[cameraIndex];
            MTXMultVec(cameraTransform, &camera->pos, &camera->pos);
            MTXMultVec(rotationMatrix, &camera->up, &camera->up);
            MTXMultVec(cameraTransform, &camera->target, &camera->target);
        }
    }
    (void)unusedCameraRegister0;
    (void)unusedCameraRegister1;
    (void)unusedCameraRegister2;
    (void)unusedCameraRegister3;
    (void)unusedCameraRegister0;
    (void)unusedCameraRegister1;
    (void)unusedCameraRegister2;
    (void)unusedCameraRegister3;
}

s32 SelectRandomPlayerFromMask(u8 *playerMask, s32 *playerValues)
{
    s32 selectionOffset;
    s32 randomStart = rand8() % 4;

    if (!playerMask || *playerMask == 0) {
        return 0;
    }
    for (selectionOffset = 0; selectionOffset < 4; selectionOffset++) {
        s32 candidateIndex = (selectionOffset + randomStart) % 4;
        if (*playerMask & (1 << candidateIndex)) {
            *playerMask = 1 << candidateIndex;
            if (playerValues) {
                playerValues[(candidateIndex + 1) % 4] = playerValues[(candidateIndex + 2) % 4] = playerValues[(candidateIndex + 3) % 4] = 0;
            }
            return candidateIndex;
        }
    }
    return 0;
}

s32 GetRandomSelectedPlayerValue(u8 *playerMask, s32 *playerValues)
{
    s32 selectionOffset;

    s32 randomStart = rand8() % 4;
    if (!playerMask || *playerMask == 0) {
        return 0;
    }
    for (selectionOffset = 0; selectionOffset < 4; selectionOffset++) {
        s32 candidateIndex = (selectionOffset + randomStart) % 4;
        if (*playerMask & (1 << candidateIndex)) {
            return playerValues[candidateIndex];
        }
    }
    return 0;
}

#pragma dontInline on
s32 LanePositionIsOccupied(s8 laneIndex, float rowPosition, BoardNode *ignoredNode, BoardNode **blockingNodeOut)
{
    BoardNode *laneNode;
    if (blockingNodeOut) {
        *blockingNodeOut = NULL;
    }
    if (laneIndex < 0 || laneIndex >= 12 || rowPosition < 0 || rowPosition > 16) {
        return 1;
    }
    for (laneNode = laneHeads[laneIndex]; laneNode;) {
        if (ignoredNode == NULL || (laneNode != ignoredNode && (!(ignoredNode->stateFlags & 0x2) || !(laneNode->stateFlags & 0x2) || (ignoredNode->ownerGroup != laneNode->ownerGroup)))) {
            if (laneNode->position.y > rowPosition - 1.0f && laneNode->position.y < rowPosition + 1.0f) {
                if (blockingNodeOut) {
                    *blockingNodeOut = laneNode;
                }
                return 1;
            }
        }
        laneNode = laneNode->nextLaneNode;
        if (!laneNode) {
            continue;
        }
        if (laneNode->position.y >= rowPosition + 1.0f) {
            break;
        }
    }
    return 0;
}
#pragma dontInline off

static inline s32 LanePositionIsOccupiedInline(s8 laneIndex, float rowPosition, BoardNode *ignoredNode, BoardNode **blockingNodeOut)
{
    BoardNode *laneNode;
    if (blockingNodeOut) {
        *blockingNodeOut = NULL;
    }
    if (laneIndex < 0 || laneIndex >= 12 || rowPosition < 0 || rowPosition > 16) {
        return 1;
    }
    for (laneNode = laneHeads[laneIndex]; laneNode;) {
        if (ignoredNode == NULL || (laneNode != ignoredNode && (!(ignoredNode->stateFlags & 0x2) || !(laneNode->stateFlags & 0x2) || (ignoredNode->ownerGroup != laneNode->ownerGroup)))) {
            if (laneNode->position.y > rowPosition - 1.0f && laneNode->position.y < rowPosition + 1.0f) {
                if (blockingNodeOut) {
                    *blockingNodeOut = laneNode;
                }
                return 1;
            }
        }
        laneNode = laneNode->nextLaneNode;
        if (!laneNode) {
            continue;
        }
        if (laneNode->position.y >= rowPosition + 1.0f) {
            break;
        }
    }
    return 0;
}

void InsertBoardNodeInLane(s8 laneIndex, float rowPosition, BoardNode *node)
{
    BoardNode *laneNode;
    BoardNode *previousNode;
    if (node->previousLaneNode == NULL) {
        laneHeads[node->laneIndex] = node->nextLaneNode;
    }
    else {
        node->previousLaneNode->nextLaneNode = node->nextLaneNode;
    }
    if (node->nextLaneNode != NULL) {
        node->nextLaneNode->previousLaneNode = node->previousLaneNode;
    }
    for (laneNode = laneHeads[laneIndex], previousNode = NULL; laneNode; laneNode = laneNode->nextLaneNode) {
        if (laneNode->position.y > rowPosition) {
            break;
        }
        previousNode = laneNode;
    }
    node->laneIndex = laneIndex;
    node->position.y = rowPosition;
    node->nextLaneNode = laneNode;
    if (node->nextLaneNode) {
        node->nextLaneNode->previousLaneNode = node;
    }
    node->previousLaneNode = previousNode;
    if (node->previousLaneNode) {
        node->previousLaneNode->nextLaneNode = node;
    }
    if (laneHeads[laneIndex] == node->nextLaneNode) {
        laneHeads[laneIndex] = node;
    }
}

s32 RemoveBoardNodeAndScore(BoardNode *node, BoardEvent *event)
{
    s32 scoreValue;
    PlayerWork *firstPlayerWork;
    PlayerWork *secondPlayerWork;
    PlayerWork *thirdPlayerWork;
    PlayerWork *fourthPlayerWork;
    s32 particleIndex;
    scoreValue = 0;
    if (!node) {
        return scoreValue;
    }
    if (node->objectKind == 0) {
        return scoreValue;
    }
    if (laneHeads[node->laneIndex] == node) {
        laneHeads[node->laneIndex] = node->nextLaneNode;
    }
    if (node->previousLaneNode) {
        node->previousLaneNode->nextLaneNode = node->nextLaneNode;
    }
    if (node->nextLaneNode) {
        node->nextLaneNode->previousLaneNode = node->previousLaneNode;
    }
    if (node->pairedNodeLeft) {
        node->pairedNodeLeft->pairedNodeRight = NULL;
    }
    if (node->pairedNodeRight) {
        node->pairedNodeRight->pairedNodeLeft = NULL;
    }
    if (node->pairedNodeUpper) {
        node->pairedNodeUpper->pairedNodeLower = NULL;
    }
    if (node->pairedNodeLower) {
        node->pairedNodeLower->pairedNodeUpper = NULL;
    }

    if (node->objectKind >= 1 && node->objectKind <= 4) {
        firstPlayerWork = playerObjects[0]->data;
        secondPlayerWork = playerObjects[1]->data;
        thirdPlayerWork = playerObjects[2]->data;
        fourthPlayerWork = playerObjects[3]->data;
        scoreValue = (GetRandomSelectedPlayerValue(&event->playerMask, event->playerValues) > 0) ? 2 : 1;
        if (winningPlayer == -1) {
            if (event->playerMask & 0x1) {
                firstPlayerWork->targetScore += scoreValue;
                firstPlayerWork->turnScore += (node->stateFlags & 0x10) ? 1 : 0;
            }
            if (event->playerMask & 0x2) {
                secondPlayerWork->targetScore += scoreValue;
                secondPlayerWork->turnScore += (node->stateFlags & 0x10) ? 1 : 0;
            }
            if (event->playerMask & 0x4) {
                thirdPlayerWork->targetScore += scoreValue;
                thirdPlayerWork->turnScore += (node->stateFlags & 0x10) ? 1 : 0;
            }
            if (event->playerMask & 0x8) {
                fourthPlayerWork->targetScore += scoreValue;
                fourthPlayerWork->turnScore += (node->stateFlags & 0x10) ? 1 : 0;
            }
        }
        for (particleIndex = 0; particleIndex < 2; particleIndex++) {
            fallingPiecePool[fallingPieceHead].pieceType = node->objectKind;
            fallingPiecePool[fallingPieceHead].position.x = ((node->laneIndex * 100) - 500.0) - 50.0;
            fallingPiecePool[fallingPieceHead].position.y = ((100.0f * node->position.y) - 500.0f) + 50.0f;
            fallingPiecePool[fallingPieceHead].position.z = 120;
            fallingPiecePool[fallingPieceHead].velocity.x = ((rand8() / 255.0) * 24.0) - 12;
            fallingPiecePool[fallingPieceHead].velocity.y = (rand8() / 255.0) * 16.0;
            fallingPiecePool[fallingPieceHead].velocity.z = ((rand8() / 255.0) * 10.0) + 6;
            fallingPiecePool[fallingPieceHead].rotationPhase = rand8();
            fallingPiecePool[fallingPieceHead].spinPhase = rand8();
            fallingPieceHead++;
            fallingPieceHead = (fallingPieceHead >= 300) ? 0 : fallingPieceHead;
        }
    }
    node->objectKind = node->stateFlags = node->ownerGroup = node->playerMask = 0;
    node->nextLaneNode = node->previousLaneNode = NULL;
    return scoreValue;
}

void ShufflePlayerCardLayout(PlayerWork *playerWork)
{
    s32 groupIndex;
    s32 assignmentCount;
    s32 randomColumn;
    s32 randomSlot;
    s32 swapSlot;
    s32 firstGroup;
    u8 secondGroup;

    for (groupIndex = 0; groupIndex < 8; groupIndex++) {
        for (assignmentCount = 0; assignmentCount < 2; assignmentCount++) {
            playerWork->cardLayout[groupIndex][assignmentCount] = 1;
        }
    }
    for (groupIndex = 1; groupIndex < 4; groupIndex++) {
        for (assignmentCount = 0; assignmentCount < 4;) {
            randomSlot = rand8() % 8;
            randomColumn = rand8() % 2;
            if (playerWork->cardLayout[randomSlot][randomColumn] == 1) {
                playerWork->cardLayout[randomSlot][randomColumn] = groupIndex + 1;
                assignmentCount++;
            }
        }
    }
    for (groupIndex = 0; groupIndex < 8; groupIndex++) {
        if (playerWork->cardLayout[groupIndex][0] == playerWork->cardLayout[groupIndex][1]) {
            do {
                assignmentCount = rand8() % 8;
            } while (
                groupIndex == assignmentCount || playerWork->cardLayout[groupIndex][0] == playerWork->cardLayout[assignmentCount][0] || playerWork->cardLayout[groupIndex][0] == playerWork->cardLayout[assignmentCount][1]);

            swapSlot = rand8() % 2;
            firstGroup = playerWork->cardLayout[groupIndex][0];
            secondGroup = playerWork->cardLayout[assignmentCount][swapSlot];
            playerWork->cardLayout[groupIndex][rand8() % 2] = secondGroup;
            playerWork->cardLayout[assignmentCount][swapSlot] = firstGroup;
        }
    }
}

void ChooseCpuMove(PlayerWork *playerWork)
{
    s32 laneIndex;
    s32 candidateDirection;
    u8 laneCellTypes[12][17] = {0};
#ifdef NON_MATCHING
    u8 occupiedCellsPerLane[13] = {0};
#else
    u8 occupiedCellsPerLane[12] = {0};
#endif
    s32 minimumLane = (playerWork->boardNodes[0]->laneIndex - 4 < 0) ? 0 : (playerWork->boardNodes[0]->laneIndex - 4);
    s32 maximumLane = (playerWork->boardNodes[0]->laneIndex + 4 > 11) ? 11 : (playerWork->boardNodes[0]->laneIndex + 4);
    s32 candidateStartLanes[2] = {};
    s32 candidateEndLanes[2] = {};
    s32 maximumStackHeights[2] = { 9, 9 };
    s32 bestCandidateScore = 0;
    s32 bestCandidateLane = 0;
    s32 bestCandidateDirection = 0;
    u8 playerPieceTypes[2];
    candidateStartLanes[0] = candidateStartLanes[1] = minimumLane;
    candidateEndLanes[0] = candidateEndLanes[1] = maximumLane;
    maximumStackHeights[0] = playerWork->boardNodes[0]->position.y - 0.5;
    maximumStackHeights[1] = playerWork->boardNodes[0]->position.y - 1.5;
    maximumStackHeights[1] = (maximumStackHeights[1] < 0) ? 0 : maximumStackHeights[1];
    playerPieceTypes[0] = playerWork->boardNodes[0]->objectKind;
    playerPieceTypes[1] = playerWork->boardNodes[1]->objectKind;
    for (laneIndex = minimumLane; laneIndex <= maximumLane; laneIndex++) {
        BoardNode *existingNode;
        s32 occupiedCellCount;
        for (existingNode = laneHeads[laneIndex], occupiedCellCount = 0; existingNode; existingNode = existingNode->nextLaneNode) {
            if (existingNode->stateFlags & 0x2) {
                continue;
            }
            laneCellTypes[laneIndex][occupiedCellCount] = existingNode->objectKind;
            occupiedCellCount++;
        }
        occupiedCellsPerLane[laneIndex] = occupiedCellCount;
        for (candidateDirection = 0; candidateDirection < 2; candidateDirection++) {
            if (occupiedCellCount > maximumStackHeights[candidateDirection]) {
                if (laneIndex <= playerWork->boardNodes[0]->laneIndex) {
                    candidateStartLanes[candidateDirection] = laneIndex + 1;
                }
                else {
                    candidateEndLanes[candidateDirection] = laneIndex - 1;
                }
            }
        }
    }
    for (candidateDirection = 0; candidateDirection < 4; candidateDirection++) {
        for (laneIndex = candidateStartLanes[candidateDirection == 2]; laneIndex <= candidateEndLanes[candidateDirection == 2]; laneIndex++) {
            if (laneIndex != candidateStartLanes[candidateDirection == 2] || candidateDirection != 3) {
                if (laneIndex != candidateEndLanes[candidateDirection == 2] - 1 || candidateDirection != 1) {
                    s32 neighborSet;
                    s32 candidateScores[4][12];
                    for (neighborSet = 0; neighborSet < 2; neighborSet++) {
                        s32 rowOffsets[4] = { 0, 1, 0, -1 };
                        s32 laneOffsets[4] = { 1, 0, -1, 0 };
                        s32 neighborRow;
                        s32 neighborColumn;
                        s32 neighborCenterRow;
                        s32 neighborCenterColumn;

                        candidateScores[candidateDirection][laneIndex] = playerWork->boardNodes[0]->laneIndex - laneIndex;
                        candidateScores[candidateDirection][laneIndex] = (candidateScores[candidateDirection][laneIndex] < 0) ? (-candidateScores[candidateDirection][laneIndex]) : candidateScores[candidateDirection][laneIndex];
                        candidateScores[candidateDirection][laneIndex]
                            = (candidateScores[candidateDirection][laneIndex] < 3) ? ((3 - candidateScores[candidateDirection][laneIndex]) * (3 - candidateScores[candidateDirection][laneIndex])) : 0;
                        candidateScores[candidateDirection][laneIndex] *= (playerWork->cpuDifficulty == 0) ? 4 : ((playerWork->cpuDifficulty == 1) ? 2 : 1);
                        neighborCenterRow = laneIndex + ((neighborSet == 1) ? rowOffsets[candidateDirection] : 0);
                        for (neighborColumn = neighborCenterRow - 2; neighborColumn <= neighborCenterRow + 2; neighborColumn++) {
                            neighborCenterColumn = occupiedCellsPerLane[neighborCenterRow] + ((neighborSet == 1) ? laneOffsets[candidateDirection] : 0);
                            for (neighborRow = neighborCenterColumn - 2; neighborRow <= neighborCenterColumn + 2; neighborRow++) {
                                if (neighborColumn < 0 || neighborColumn >= 12 || neighborRow < 0 || neighborRow >= 9) {
                                    continue;
                                }
                                if (laneCellTypes[neighborColumn][neighborRow] != 5 && playerWork->boardNodes[neighborSet]->objectKind == laneCellTypes[neighborColumn][neighborRow]) {
                                    u8 matchingPieceWeights[5][5] = { 0, 3, 0, 3, 0, 2, 10, 24, 10, 2, 10, 24, 0, 24, 10, 2, 10, 24, 10, 2, 0, 2, 10, 2, 0 };
                                    candidateScores[candidateDirection][laneIndex] += matchingPieceWeights[neighborColumn - (neighborCenterRow - 2)][neighborRow - (neighborCenterColumn - 2)];
                                }
                                if (laneCellTypes[neighborColumn][neighborRow] == 5) {
                                    u8 hazardWeights[5][5] = { 0, 0, 0, 0, 0, 0, 40, 40, 40, 0, 0, 40, 0, 40, 0, 0, 40, 40, 40, 0, 0, 0, 0, 0, 0 };
                                    candidateScores[candidateDirection][laneIndex]
                                        += hazardWeights[neighborColumn - (neighborCenterRow - 2)][neighborRow - (neighborCenterColumn - 2)] / ((playerWork->cpuDifficulty == 0) ? 4 : 1);
                                }
                            }
                        }
                    }
                    if (candidateScores[candidateDirection][laneIndex] > bestCandidateScore) {
                        bestCandidateScore = candidateScores[candidateDirection][laneIndex];
                        bestCandidateDirection = candidateDirection;
                        bestCandidateLane = laneIndex;
                    }
                }
            }
        }
    }
    playerWork->targetLane = bestCandidateLane;
    playerWork->targetDirection = bestCandidateDirection;
}

void EmptyPlayerObjectCallback(OMOBJ *object) { }

void UpdatePlayerObject(OMOBJ *object)
{
    PlayerWork *playerState;
    s32 inputDirection;
    s32 unusedInputState;
    s32 unusedRepeatState;

    playerState = object->data;
    playerState->previousScore = playerState->targetScore;

    playerState->cardAnimationFrames[0] = (playerState->cardAnimationFrames[0] == 0 || playerState->cardAnimationFrames[0] >= 15) ? 0 : playerState->cardAnimationFrames[0] + 1;
    if (playerState->controllerIndex != -1) {
        inputDirection = (HuPadStkX[playerState->controllerIndex] > 26) - (HuPadStkX[playerState->controllerIndex] < -26);
    }
    else {
        inputDirection = (playerState->playerState == 1 && playerState->targetLane != -1) ? ((playerState->boardNodes[0]) ? ((playerState->targetLane < playerState->boardNodes[0]->laneIndex) ? -1
                                                                           : (playerState->targetLane > playerState->boardNodes[0]->laneIndex)               ? 1
                                                                                                                                         : 0)
                                                                                        : 0)
                                                                : 0;
    }
    playerState->targetLaneHoldFrames = ((playerState->playerState == 1 && playerState->boardNodes[0]) && playerState->targetLane == playerState->boardNodes[0]->laneIndex) ? (playerState->targetLaneHoldFrames + 1) : 0;
    if (inputDirection != playerState->cursorDirection) {
        playerState->buttonRepeatFrames = (inputDirection == 0) ? 0 : 1;
    }
    else {
        playerState->buttonRepeatFrames = (playerState->cursorDirection != 0) ? playerState->buttonRepeatFrames + 1 : 0;
    }
    playerState->buttonRepeatFrames = (playerState->buttonRepeatFrames >= 17) ? 14 : playerState->buttonRepeatFrames;
    playerState->cursorDirection = inputDirection;
    if (playerState->playerState == 2) {
        float transitionProgress;
        float remainingProgress;
        s32 candidateNodeIndex;
        s32 laneSearchIndex;
        s32 candidateLane;
        BoardNode *lastNodeInLane;

        for (laneSearchIndex = 0; laneSearchIndex < 3; laneSearchIndex++) {
            s32 laneSearchOrder[3] = { 1, 2, 0 };
            candidateLane = (playerState->playerSlot * 3) + ((playerState->playerSlot < 2) ? laneSearchOrder[laneSearchIndex] : (2 - laneSearchOrder[laneSearchIndex]));
            if (LanePositionIsOccupiedInline(candidateLane, 9.5f, NULL, NULL) == 0) {
                if (LanePositionIsOccupiedInline(candidateLane, 8.5f, NULL, NULL) == 0) {
                    break;
                }
            }
        }
        candidateLane = (laneSearchIndex == 3) ? ((playerState->playerSlot * 3) + 1) : candidateLane;
        transitionProgress = (candidateLane * 100) - 490;
        playerState->cameraZ = (playerState->selectionTimer == 0)
            ? transitionProgress
            : ((playerState->cameraZ - 10 > transitionProgress)
                    ? (playerState->cameraZ - 10)
                    : (((playerState->cameraZ > transitionProgress)
                            ? transitionProgress
                            : ((playerState->cameraZ + 10 < transitionProgress) ? (playerState->cameraZ + 10)
                                                                 : ((playerState->cameraZ < transitionProgress) ? transitionProgress : playerState->cameraZ)))));
        playerState->selectionTimer++;
        transitionProgress = playerState->selectionTimer / 24.0f;
        transitionProgress = (transitionProgress > 1.0f) ? 1.0f : transitionProgress;
        transitionProgress = (0.5 * sind((transitionProgress * 180.0f) + 270.0f)) + 0.5;
        remainingProgress = 1.0f - transitionProgress;
        if (winningPlayer == -1) {
            Vec cameraWorldPosition;
            Vec cameraScreenPosition;
            cameraWorldPosition.x = (playerState->cameraZ * transitionProgress) + (playerState->cameraX * remainingProgress);
            cameraWorldPosition.y = (425.0f * transitionProgress) + (playerState->cameraY * remainingProgress);
            cameraWorldPosition.z = 55;
            Hu3D3Dto2D(&cameraWorldPosition, 1, &cameraScreenPosition);
            HuSprPosSet(playerState->playerSpriteGroup, 0, cameraScreenPosition.x, cameraScreenPosition.y);
        }
        if (mainSequenceObject->work[0] == 1002 && playerState->selectionTimer >= 24) {
            for (laneSearchIndex = 0; laneSearchIndex < 3; laneSearchIndex++) {
                s32 laneSearchOrder[3] = { 1, 2, 0 };
                candidateLane = (playerState->playerSlot * 3) + ((playerState->playerSlot < 2) ? laneSearchOrder[laneSearchIndex] : (2 - laneSearchOrder[laneSearchIndex]));
                if (LanePositionIsOccupiedInline(candidateLane, 9.5f, NULL, NULL) == 0) {
                    if (LanePositionIsOccupiedInline(candidateLane, 8.5f, NULL, NULL) == 0) {
                        break;
                    }
                }
            }
            if (laneSearchIndex != 3) {
                for (lastNodeInLane = laneHeads[candidateLane]; lastNodeInLane && lastNodeInLane->nextLaneNode; lastNodeInLane = lastNodeInLane->nextLaneNode)
                    ;
                for (laneSearchIndex = 0; laneSearchIndex < 2; laneSearchIndex++) {
                    candidateNodeIndex = (unusedState + 1) % 238;
                    while (candidateNodeIndex != unusedState) {
                        if (boardNodePool[candidateNodeIndex].objectKind == 0) {
                            break;
                        }
                        candidateNodeIndex++;
                        candidateNodeIndex = candidateNodeIndex % 238;
                        if (candidateNodeIndex == 0) {
                            OSReport("looped.\n");
                        }
                    }
                    if (candidateNodeIndex == unusedState) {
                        OSReport("m413:error:block empty.\n");
                    }
                    unusedState = candidateNodeIndex;
                    playerState->boardNodes[laneSearchIndex] = &boardNodePool[candidateNodeIndex];
                    boardNodePool[candidateNodeIndex].objectKind = playerState->cardLayout[playerState->cardColumn][laneSearchIndex];
                    boardNodePool[candidateNodeIndex].stateFlags = 2;
                    boardNodePool[candidateNodeIndex].playerMask = 1 << playerState->playerSlot;
                    boardNodePool[candidateNodeIndex].ownerGroup = playerState->playerSlot;
                    boardNodePool[candidateNodeIndex].laneIndex = candidateLane;
                    boardNodePool[candidateNodeIndex].position.x = candidateLane;
                    boardNodePool[candidateNodeIndex].position.y = laneSearchIndex + 8.9375f;
                    boardNodePool[candidateNodeIndex].position.z = 0;
                    boardNodePool[candidateNodeIndex].playerScores[0] = boardNodePool[candidateNodeIndex].playerScores[1] = boardNodePool[candidateNodeIndex].playerScores[2]
                        = boardNodePool[candidateNodeIndex].playerScores[3] = 0;
                    boardNodePool[candidateNodeIndex].associatedObject = 0;
                    boardNodePool[candidateNodeIndex].nextGroupNode = 0;
                    boardNodePool[candidateNodeIndex].nextChainNode = 0;
                    boardNodePool[candidateNodeIndex].ownerWork = playerState;
                }
                playerState->boardNodes[1]->nextLaneNode = NULL;
                playerState->boardNodes[1]->previousLaneNode = playerState->boardNodes[0];
                playerState->boardNodes[0]->nextLaneNode = playerState->boardNodes[1];
                playerState->boardNodes[0]->previousLaneNode = lastNodeInLane;
                if (!lastNodeInLane) {
                    laneHeads[playerState->boardNodes[0]->laneIndex] = playerState->boardNodes[0];
                }
                else {
                    playerState->boardNodes[0]->previousLaneNode->nextLaneNode = playerState->boardNodes[0];
                }
                playerState->cardColumn++;
                if (playerState->cardColumn >= 8) {
                    ShufflePlayerCardLayout(playerState);
                    playerState->cardColumn = 0;
                }
                playerState->cardTypes[0] = playerState->boardNodes[0]->objectKind;
                playerState->cardTypes[1] = playerState->boardNodes[1]->objectKind;
                playerState->cardAnimationFrames[0] = 1;
                if (((playerState->turnScore + 5) / 7) != ((playerState->turnScoreTarget + 5) / 7)) {
                    if (((playerState->turnScore + 5) / 7) % 4 == 2) {
                        playerState->cardLayout[playerState->cardColumn][0] = 5;
                        playerState->cardLayout[playerState->cardColumn][1] = 5;
                    }
                    else {
                        playerState->cardLayout[playerState->cardColumn][rand8() % 2] = 5;
                    }
                }
                playerState->turnScoreTarget = playerState->turnScore;
                playerState->playerState = 1;
                playerState->selectionTimer = 0;
                playerState->inputDelay = 20;
                playerState->selectedLane = 0;
                playerState->targetLane = -1;
                playerState->targetDirection = -1;
                if (playerState->controllerIndex == -1) {
                    playerState->aiDelay = 0;
                    playerState->targetLaneHoldFrames = 0;
                    playerState->currentLane = playerState->boardNodes[0]->laneIndex;
                    playerState->laneChangeFrames = 0;
                }
                playerState->inputEnabled = 0;
                playerState->turnComplete = 0;
                HuSprAttrReset(playerState->playerSpriteGroup, 0, HUSPR_ATTR_DISPOFF);
                HuSprAttrReset(playerState->playerSpriteGroup, 0, HUSPR_ATTR_REVERSE);
                {
                    s32 laneOccupancyCounts[12] = {};
                    s32 laneScanIndex;
                    for (laneScanIndex = 0; laneScanIndex < 3; laneScanIndex++) {
                        BoardNode *laneNode;
                        for (laneNode = laneHeads[laneScanIndex + (playerState->playerSlot * 3)]; laneNode; laneNode = laneNode->nextLaneNode) {

                            laneOccupancyCounts[laneScanIndex] += (laneNode && laneNode->objectKind && (laneNode->stateFlags & 0x2) == 0 && laneOccupancyCounts[laneScanIndex] < 9);
                        }
                    }
                    Hu3DModelRotSet(object->mdlId[4], 0, ((laneOccupancyCounts[0] + laneOccupancyCounts[1] + laneOccupancyCounts[2] >= 22) ? -90 : 0), 0);
                }
            }
            else {
                s32 laneHeights[3];
                s32 candidateLaneIndex;
                for (candidateLaneIndex = 0; candidateLaneIndex < 3; candidateLaneIndex++) {
                    s32 laneHeight;
                    BoardNode *laneNode;
                    for (laneNode = laneHeads[candidateLaneIndex + (playerState->playerSlot * 3)], laneHeight = 0; laneHeight < 9; laneNode = laneNode->nextLaneNode, laneHeight++) {
                        if (!laneNode || laneNode->stateFlags != 1) {
                            break;
                        }
                    }
                    laneHeights[candidateLaneIndex] = laneHeight;
                }
                if (laneHeights[0] >= 9 && laneHeights[1] >= 9 && laneHeights[2] >= 9) {
                    playerState->inputEnabled += playerState->inputEnabled < 255;
                    Hu3DModelRotSet(object->mdlId[4], 0, -90, 0);
                    playerState->turnComplete = 1;
                }
                else {
                    playerState->turnComplete = 0;
                }
            }
        }
    }
    else if (playerState->playerState == 1) {
        if (!playerState->boardNodes[0] || !playerState->boardNodes[1]) {
            playerState->playerState = 2;
            playerState->selectionTimer = 0;
            HuSprAttrSet(playerState->playerSpriteGroup, 0, HUSPR_ATTR_REVERSE);
        }
        else {
            s32 selectedLaneIndex;
            s32 movementDirection;

            movementDirection = (playerState->buttonRepeatFrames == 1 || playerState->buttonRepeatFrames == 14) ? playerState->cursorDirection : 0;
            selectedLaneIndex = playerState->selectedLane;
            if (playerState->controllerIndex != -1) {
                selectedLaneIndex = (HuPadBtnDown[playerState->controllerIndex] & (PAD_BUTTON_X | PAD_BUTTON_Y))
                    ? (playerState->selectedLane + 2)
                    : (playerState->selectedLane + ((HuPadBtnDown[playerState->controllerIndex] & PAD_BUTTON_A) ? 1 : 0)
                        + ((HuPadBtnDown[playerState->controllerIndex] & PAD_BUTTON_B) ? -1 : 0));
            }
            else {
                if (playerState->targetDirection != -1 && playerState->targetDirection != playerState->selectedLane && playerState->aiDelay > 5) {
                    selectedLaneIndex = playerState->targetDirection;
                    playerState->aiDelay = 0;
                }
                else {
                    playerState->aiDelay++;
                }
            }
            selectedLaneIndex = (selectedLaneIndex >= 4) ? (selectedLaneIndex - 4) : ((selectedLaneIndex < 0) ? (selectedLaneIndex + 4) : selectedLaneIndex);
            playerState->selectionTimer++;
            if (playerState->controllerIndex == -1) {
                s32 easyCpuWaitFrames[4] = { 120, 60, 25, 2 };
                s32 harderCpuWaitFrames[3] = { 60, 45, 25 };
                playerState->laneChangeFrames = (playerState->currentLane == playerState->boardNodes[0]->laneIndex && playerState->currentLane != playerState->targetLane) ? (playerState->laneChangeFrames + 1) : 0;
                playerState->currentLane = playerState->boardNodes[0]->laneIndex;
                if (playerState->targetLane == -1 && playerState->laneChangeFrames >= easyCpuWaitFrames[playerState->cpuDifficulty]
                    || (playerState->cpuDifficulty >= 1 && (playerState->laneChangeFrames >= harderCpuWaitFrames[playerState->cpuDifficulty - 1]))) {
                    s32 cpuLaneChangeDelay[4] = { 30, 30, 20, 10 };
                    ChooseCpuMove(playerState);
                    playerState->aiDelay = 0;
                    playerState->targetLaneHoldFrames = 0;
                    playerState->playerFlags = (rand8() % cpuLaneChangeDelay[playerState->cpuDifficulty]) - (cpuLaneChangeDelay[playerState->cpuDifficulty] / 2);
                }
            }
#if VERSION_PAL
#define _VALUE -0.34375f
#else
#define _VALUE -0.26875f
#endif
            if (winningPlayer == -1) {
                if (playerState->controllerIndex != -1) {
                    playerState->boardNodes[0]->position.z = playerState->boardNodes[1]->position.z
                        = -0.03125f + ((HuPadStkY[playerState->controllerIndex] < -36) ? (_VALUE * ((-HuPadStkY[playerState->controllerIndex] - 36) / 36.0f)) : 0);
                    playerState->cursorRepeatFrames = (HuPadStkY[playerState->controllerIndex] < -36) ? 1 : 0;
                }
                else {
                    s32 cpuJumpDelay[3] = { 50, 30, 25 };
                    s32 cpuRepeatJumpDelay[3] = { 90, 80, 70 };

                    playerState->cursorRepeatFrames
                        = ((playerState->cpuDifficulty
                              && (playerState->targetLaneHoldFrames > playerState->playerFlags + cpuJumpDelay[playerState->cpuDifficulty - 1] || playerState->laneChangeFrames > cpuRepeatJumpDelay[playerState->cpuDifficulty - 1])))
                        ? 1
                        : 0;
                    playerState->boardNodes[0]->position.z = playerState->boardNodes[1]->position.z = -0.03125f + (playerState->cursorRepeatFrames ? _VALUE : 0.0f);
                }
            }
#undef _VALUE
            else {
                playerState->boardNodes[0]->position.z = playerState->boardNodes[1]->position.z = 0;
            }
            if (winningPlayer == -1 && movementDirection) {
                if (LanePositionIsOccupiedInline(playerState->boardNodes[0]->characterId + movementDirection, playerState->boardNodes[0]->position.y, playerState->boardNodes[0], NULL) == 0) {
                    if (LanePositionIsOccupiedInline(playerState->boardNodes[1]->characterId + movementDirection, playerState->boardNodes[1]->position.y, playerState->boardNodes[1], NULL) == 0) {
                        float previousFirstLane = playerState->boardNodes[0]->laneIndex;
                        float previousSecondLane = playerState->boardNodes[1]->laneIndex;
                        InsertBoardNodeInLane(playerState->boardNodes[0]->characterId + movementDirection, playerState->boardNodes[0]->position.y, playerState->boardNodes[0]);
                        InsertBoardNodeInLane(playerState->boardNodes[1]->characterId + movementDirection, playerState->boardNodes[1]->position.y, playerState->boardNodes[1]);
                        playerState->boardNodes[0]->position.x = (previousFirstLane * 0.5) + (0.5 * playerState->boardNodes[0]->laneIndex);
                        playerState->boardNodes[1]->position.x = (previousSecondLane * 0.5) + (0.5 * playerState->boardNodes[1]->laneIndex);
                    }
                }
            }
            if (winningPlayer != -1) {
                selectedLaneIndex = playerState->selectedLane;
            }
            if (selectedLaneIndex != playerState->selectedLane) {
                BoardNode *firstNode = playerState->boardNodes[0];
                BoardNode *secondNode = playerState->boardNodes[1];
                s32 startingLane = firstNode->laneIndex;
                float startingRow = firstNode->position.y;
                s32 laneOffsets[4] = { 0, 1, 0, -1 };
                s32 rowOffsets[4] = { 1, 0, -1, 0 };
                BoardNode *blockingNode;
                if (LanePositionIsOccupiedInline(startingLane + laneOffsets[selectedLaneIndex], startingRow + rowOffsets[selectedLaneIndex], secondNode, &blockingNode) == 0) {
                    if (playerState->selectedLane != selectedLaneIndex) {
                        secondNode->animationFrame = 1;
                        secondNode->pathId = (((((selectedLaneIndex - playerState->selectedLane) + 8) % 4) - 1) * 4) + playerState->selectedLane + 1;
                    }
                    playerState->selectedLane = selectedLaneIndex;
                    InsertBoardNodeInLane(startingLane + laneOffsets[selectedLaneIndex], startingRow + rowOffsets[selectedLaneIndex], secondNode);
                    secondNode->position.x = secondNode->laneIndex;
                }
                else {
                    float targetRow;
                    s32 targetLane;
                    if (selectedLaneIndex % 2 == 0) {
                        s32 isVerticalMove = (selectedLaneIndex == 2) ? 1 : 0;
                        targetRow = startingRow + ((isVerticalMove == 1) ? rowOffsets[selectedLaneIndex] : rowOffsets[selectedLaneIndex]);
                        LanePositionIsOccupiedInline(startingLane, targetRow, (isVerticalMove == 0) ? secondNode : secondNode, &blockingNode);
                        if (selectedLaneIndex == 2) {
                            if (!blockingNode || blockingNode->position.y <= targetRow) {
                                targetRow = ((isVerticalMove == 1) ? 1.0f : 0.0f) + ((blockingNode) ? blockingNode->position.y + 1.0f : 0.0f);
                                targetLane = startingLane;
                            }
                            else {
                                targetRow = (blockingNode->position.y - 1.0f) - (isVerticalMove == 0);
                                targetLane = startingLane;
                            }
                        }
                        else {
                            if (blockingNode->position.y >= targetRow) {
                                targetRow = ((isVerticalMove == 1) ? 1.0f : 0.0f) + ((blockingNode) ? blockingNode->position.y - 2.0f : 0.0f);
                                targetLane = startingLane;
                            }
                            else {
                                targetRow = (blockingNode->position.y - 1.0f) - (isVerticalMove == 0);
                                targetLane = startingLane;
                            }
                        }
                    }
                    else {
                        targetLane = startingLane - laneOffsets[selectedLaneIndex];
                        targetRow = startingRow;
                    }
                    if (LanePositionIsOccupiedInline(targetLane, targetRow, firstNode, NULL) == 0) {
                        if (LanePositionIsOccupiedInline(targetLane + laneOffsets[selectedLaneIndex], targetRow + rowOffsets[selectedLaneIndex], secondNode, NULL) == 0) {
                            float previousFirstLane = playerState->boardNodes[0]->laneIndex;
                            float previousSecondLane = playerState->boardNodes[1]->laneIndex;
                            if (playerState->selectedLane != selectedLaneIndex) {
                                secondNode->animationFrame = 1;
                                secondNode->pathId = (((((selectedLaneIndex - playerState->selectedLane) + 8) % 4) - 1) * 4) + playerState->selectedLane + 1;
                            }
                            playerState->selectedLane = selectedLaneIndex;
                            if (selectedLaneIndex == 2) {
                                InsertBoardNodeInLane(targetLane, targetRow + (0.5 * (float)rowOffsets[selectedLaneIndex]), firstNode);
                                InsertBoardNodeInLane(targetLane + laneOffsets[selectedLaneIndex], targetRow + rowOffsets[selectedLaneIndex], secondNode);
                                InsertBoardNodeInLane(targetLane, targetRow, firstNode);
                            }
                            else {
                                InsertBoardNodeInLane(targetLane, targetRow, firstNode);
                                InsertBoardNodeInLane(targetLane + laneOffsets[selectedLaneIndex], targetRow + rowOffsets[selectedLaneIndex], secondNode);
                            }
                            playerState->boardNodes[0]->position.x = (previousFirstLane * 0.5) + (0.5 * playerState->boardNodes[0]->laneIndex);
                            playerState->boardNodes[1]->position.x = (previousSecondLane * 0.5) + (0.5 * playerState->boardNodes[1]->laneIndex);
                        }
                    }
                }
            }
        }
        if (playerState->boardNodes[0]) {
            playerState->cameraX = 10.0 + ((100.0 * ((playerState->boardNodes[0]->laneIndex + playerState->boardNodes[0]->position.x) / 2.0)) - 500);
            playerState->cameraY = 25.0f + ((100.0f * playerState->boardNodes[0]->position.y) - 500.0f);
        }
        if (winningPlayer == -1) {
            Vec cameraWorldPosition;
            Vec screenPosition;
            cameraWorldPosition.x = playerState->cameraX;
            cameraWorldPosition.y = playerState->cameraY;
            cameraWorldPosition.z = 55;
            Hu3D3Dto2D(&cameraWorldPosition, 1, &screenPosition);
            HuSprPosSet(playerState->playerSpriteGroup, 0, screenPosition.x, screenPosition.y);
        }
    }
}

void PropagateChainScore(BoardNode *node, BoardEvent *event);

#define CHECK_2E48(lane, row, grid)                                                                                                                 \
    (((lane)-1 <= 0 || (grid[(lane)-1][(row)] && grid[(lane)-1][(row)]->objectKind))                                                                     \
        && ((lane)-1 >= 11 || (grid[(lane) + 1][(row)] && grid[(lane) + 1][(row)]->objectKind))                                                          \
        && ((row)-1 <= 0 || (grid[(lane)][(row)-1] && grid[(lane)][(row)-1]->objectKind))                                                               \
        && ((row)-1 >= 8 || (grid[(lane)][(row) + 1] && grid[(lane)][(row) + 1]->objectKind))                                                           \
        && ((lane)-1 >= 11 || (row)-1 <= 0 || (grid[(lane) + 1][(row)-1] && grid[(lane) + 1][(row)-1]->objectKind))                                     \
        && ((lane)-1 >= 11 || (row)-1 >= 8 || (grid[(lane) + 1][(row) + 1] && grid[(lane) + 1][(row) + 1]->objectKind))                                 \
        && ((lane)-1 <= 0 || (row)-1 <= 0 || (grid[(lane)-1][(row)-1] && grid[(lane)-1][(row)-1]->objectKind))                                          \
        && ((lane)-1 <= 0 || (row)-1 >= 8 || (grid[(lane)-1][(row) + 1] && grid[(lane)-1][(row) + 1]->objectKind)))

void ResolveBoardMatches(s32 activePlayerIndex)
{
    OMOBJ *boardObject = boardObject;
    BoardNode *boardGrid[14][19] = {};
    BoardNode *excludedNode = NULL;
    BoardNode *followingNode;
    BoardNode *relatedNode;
    s32 eventIndex;
    s32 eventCount;
    s32 scanIndex;
    s32 laneIndex;
    float animationProgress;
    float verticalVelocity;
    float verticalOffset;

    BoardEvent boardEvents[204];
    BoardNode *removedNodes[476];
    BoardEvent *activeEvents[204];
    float eventLanePositions[204];
    float eventRowPositions[204];
    s16 eventNodeCounts[204];

    for (laneIndex = 0; laneIndex < 12; laneIndex++) {
        for (followingNode = laneHeads[laneIndex]; followingNode; followingNode = followingNode->nextLaneNode) {
            s32 nodeFlags = followingNode->stateFlags;
            s32 includeNode = (((nodeFlags & 0x2) == 0 && (activePlayerIndex == 4)) || ((nodeFlags & 0x2) && (followingNode->playerMask & (1 << activePlayerIndex))));
            verticalOffset = (includeNode) ? followingNode->position.z : 0;
            followingNode->associatedObject = (void *)(followingNode->nextGroupNode = followingNode->nextChainNode = NULL);
            followingNode->pairedNodeLeft = followingNode->pairedNodeRight = followingNode->pairedNodeLower = followingNode->pairedNodeUpper = NULL;
            if (activePlayerIndex == 4) {
                followingNode->animationFrame += followingNode->pathId != 0 && followingNode->animationFrame != 0;
                followingNode->animationFrame = (followingNode->pathId == 0 || (followingNode->animationFrame > (((followingNode->pathId - 1) / 4 == 1) ? 15 : 15))) ? 0 : followingNode->animationFrame;
                followingNode->pathId = (followingNode->animationFrame == 0) ? 0 : followingNode->pathId;
                followingNode->position.x = (0.5 * followingNode->laneIndex) + (0.5 * followingNode->position.x);
            }
            if (followingNode->objectKind) {
                s32 relatedNodeCount = 0;
                BoardNode *previousLaneNode;
                if ((followingNode->stateFlags & 0x1) && followingNode->position.y == 0.0f && !followingNode->previousLaneNode) {
                    relatedNodeCount = 1;
                    previousLaneNode = NULL;
                }
                else if ((followingNode->stateFlags & 0x1) && followingNode->previousLaneNode && (followingNode->previousLaneNode->position.y == followingNode->position.y - 1.0f)) {
                    relatedNodeCount = 2;
                    previousLaneNode = followingNode->previousLaneNode;
                }
                else {
                    if (LanePositionIsOccupied(laneIndex, followingNode->position.y + verticalOffset, followingNode, &previousLaneNode)) {
                        if (!previousLaneNode || followingNode->position.y + verticalOffset >= previousLaneNode->position.y) {
                            relatedNodeCount = 2;
                        }
                    }
                }
                if (relatedNodeCount != 0) {
                    if (followingNode->stateFlags & 0x2) {
                        float verticalGap = (followingNode->position.y + verticalOffset) - ((!previousLaneNode) ? 0.0f : (1.0f + previousLaneNode->position.y));
                        verticalGap = followingNode->position.y - ((!previousLaneNode) ? 0.0f : (1.0f + previousLaneNode->position.y));
                        if (followingNode->ownerWork->boardNodes[0]->laneIndex != followingNode->ownerWork->boardNodes[1]->laneIndex) {
                            followingNode->ownerWork->boardNodes[0]->position.y = ((!previousLaneNode) ? 0.0f : (1.0f + previousLaneNode->position.y));
                            followingNode->ownerWork->boardNodes[1]->position.y = ((!previousLaneNode) ? 0.0f : (1.0f + previousLaneNode->position.y));
                        }
                        else {
                            followingNode->ownerWork->boardNodes[0]->position.y -= verticalGap;
                            followingNode->ownerWork->boardNodes[1]->position.y -= verticalGap;
                        }
                        followingNode->ownerWork->boardNodes[0]->position.z = 0;
                        followingNode->ownerWork->boardNodes[1]->position.z = 0;
                        if (includeNode) {
                            if (!previousLaneNode || ((previousLaneNode->stateFlags & 0x1) && (previousLaneNode->stateFlags & 0x2) == 0)) {
                                followingNode->ownerWork->inputDelay -= (followingNode->ownerWork->inputDelay != 0 && winningPlayer == -1);
                            }
                        }
                        if (followingNode->ownerWork->inputDelay == 0
                            || followingNode->ownerWork->cursorRepeatFrames != 0 && (!previousLaneNode || ((previousLaneNode->stateFlags & 0x1) && (previousLaneNode->stateFlags & 0x2) == 0))) {
                            followingNode->ownerWork->boardNodes[0]->stateFlags &= ~0x2;
                            followingNode->ownerWork->boardNodes[1]->stateFlags &= ~0x2;
                            followingNode->ownerWork->boardNodes[0]->stateFlags |= 0x10;
                            followingNode->ownerWork->boardNodes[1]->stateFlags |= 0x10;
                            followingNode->stateFlags |= 0x1;
                            if (followingNode->ownerWork->boardNodes[0]->laneIndex != followingNode->ownerWork->boardNodes[1]->laneIndex) {
                                s32 nodeScore;
                                s32 activePlayerSide = (followingNode == followingNode->ownerWork->boardNodes[1]) ? 1 : 0;
                                s32 firstPlayerLane = followingNode->ownerWork->boardNodes[activePlayerSide]->laneIndex;
                                s32 secondPlayerLane = followingNode->ownerWork->boardNodes[1 - activePlayerSide]->laneIndex;
                                float firstPlayerRow = followingNode->ownerWork->boardNodes[activePlayerSide]->position.y;
                                float secondPlayerRow = followingNode->ownerWork->boardNodes[1 - activePlayerSide]->position.y;
                                nodeScore = (firstPlayerLane < secondPlayerLane || (firstPlayerLane == secondPlayerLane && firstPlayerRow < secondPlayerRow)) ? 1 : 0;
                                if (nodeScore == 1) {
                                    BoardNode *opponentNode = followingNode->ownerWork->boardNodes[1 - activePlayerSide];
                                    BoardNode *precedingNode = (opponentNode) ? opponentNode->previousLaneNode : NULL;
                                    if (opponentNode) {
                                        if ((!precedingNode && opponentNode->position.y == 0.0f)
                                            || (precedingNode && opponentNode->position.y == precedingNode->position.y + 1.0f && (precedingNode->stateFlags & 0x1)
                                                && (precedingNode->stateFlags & 0x2) == 0)) {
                                            opponentNode->position.y = (precedingNode) ? (precedingNode->position.y + 1.0f) : 0.0f;
                                            opponentNode->stateFlags &= ~0x1;
                                            opponentNode->stateFlags |= (precedingNode && ((precedingNode->stateFlags & 0x1) == 0 || (precedingNode->stateFlags & 0x2) != 0)) ? 0 : 1;
                                            opponentNode->position.z
                                                = (opponentNode->stateFlags & 0x1) ? -0.010416667f : ((!precedingNode) ? -0.010416667f : precedingNode->position.z);
                                            excludedNode = opponentNode;
                                        }
                                    }
                                }
                            }
                            followingNode->pathId = followingNode->animationFrame = 0;
                            if (followingNode->ownerWork->boardNodes[0]->laneIndex == followingNode->ownerWork->boardNodes[1]->laneIndex) {
                                s32 matchedPlayerSide = (followingNode->ownerWork->boardNodes[1] == followingNode) ? 1 : 0;
                                s32 higherRowSide = followingNode->ownerWork->boardNodes[0]->position.y > followingNode->ownerWork->boardNodes[1]->position.y;
                                followingNode->ownerWork->boardNodes[1 - higherRowSide]->position.y = 1 + followingNode->ownerWork->boardNodes[higherRowSide]->position.y;
                                followingNode->ownerWork->boardNodes[1 - higherRowSide]->position.z = -0.010416667f;
                                followingNode->ownerWork->boardNodes[0]->stateFlags |= 0x1;
                                followingNode->ownerWork->boardNodes[1]->stateFlags |= 0x1;
                                followingNode->ownerWork->boardNodes[0]->pathId = followingNode->ownerWork->boardNodes[0]->animationFrame = 0;
                                followingNode->ownerWork->boardNodes[1]->pathId = followingNode->ownerWork->boardNodes[1]->animationFrame = 0;
                                followingNode->pairedNodeLeft = (followingNode->ownerWork->boardNodes[1] == followingNode) ? followingNode->ownerWork->boardNodes[0] : followingNode->ownerWork->boardNodes[1];
                                if (matchedPlayerSide) {
                                    followingNode->ownerWork->boardNodes[0]->stateFlags |= 0x8;
                                }
                                else {
                                    followingNode->ownerWork->boardNodes[1]->stateFlags |= 0x8;
                                }
                            }
                            followingNode->ownerWork->boardNodes[0] = NULL;
                            followingNode->ownerWork->boardNodes[1] = NULL;
                        }
                    }
                    else {
                        followingNode->position.y = (previousLaneNode) ? (1.0f + previousLaneNode->position.y) : 0.0f;
                        followingNode->stateFlags &= ~0x1;
                        followingNode->stateFlags |= (previousLaneNode && ((previousLaneNode->stateFlags & 0x1) == 0 || (previousLaneNode->stateFlags & 0x2) != 0)) ? 0 : 1;
                        followingNode->position.z = (followingNode->stateFlags & 0x1) ? -0.010416667f : ((!previousLaneNode) ? -0.010416667f : previousLaneNode->position.z);
                    }
                }
                else {
                    followingNode->position.y += verticalOffset;
                    followingNode->position.z = (includeNode) ? ((followingNode->stateFlags & 0x3) ? 0.0f : -0.010416667f + followingNode->position.z) : followingNode->position.z;
                    if (followingNode->stateFlags & 0x1) {
                        followingNode->stateFlags &= ~0x1;
                    }
                }
            }
            if ((followingNode->stateFlags & 0x1) && (nodeFlags & 0x1) && (followingNode->stateFlags & 0x8) == 0 && followingNode != excludedNode) {
                followingNode->playerScores[0] = followingNode->playerScores[1] = followingNode->playerScores[2] = followingNode->playerScores[3] = 0;
                followingNode->playerMask = 0;
                followingNode->stateFlags &= ~0x10;
            }
            followingNode->stateFlags &= ~0x8;
            if ((followingNode->stateFlags & 0x1) && (followingNode->stateFlags & 0x2) == 0) {
                boardGrid[followingNode->laneIndex + 1][(int)followingNode->position.y + 1] = followingNode;
            }
            if (followingNode->stateFlags & 0x2) {
                followingNode->position.z = 0;
            }
        }
    }
    eventCount = 0;
    {
        s32 eventPass;
        for (laneIndex = 0; laneIndex < 12; laneIndex++) {
            BoardNode *nextLaneNode = (laneIndex < 11) ? laneHeads[laneIndex + 1] : NULL;
            BoardNode *adjacentNodeA = NULL;
            BoardNode *adjacentNodeB = NULL;
            float highestNeighborRow = (nextLaneNode) ? (nextLaneNode->position.y) : 0.0f;
            for (followingNode = laneHeads[laneIndex]; followingNode; followingNode = followingNode->nextLaneNode) {
                s32 neighborLane = followingNode->laneIndex + 1;
                s32 neighborRow = (int)followingNode->position.y + 1;
                if ((followingNode->stateFlags & 0x1) && (followingNode->stateFlags & 0x2) == 0) {
                    if (boardGrid[neighborLane][neighborRow + 1]) {
                        followingNode->pairedNodeLeft = boardGrid[neighborLane][neighborRow + 1];
                        followingNode->pairedNodeLeft->pairedNodeRight = followingNode;
                    }
                    if (boardGrid[neighborLane + 1][neighborRow]) {
                        nextLaneNode = boardGrid[neighborLane + 1][neighborRow];
                        followingNode->pairedNodeUpper = nextLaneNode;
                        followingNode->pairedNodeUpper->pairedNodeLower = followingNode;
                    }
                    adjacentNodeA = boardGrid[neighborLane + 1][neighborRow + 1];
                    adjacentNodeB = boardGrid[neighborLane + 1][neighborRow - 1];
                    if (followingNode->objectKind == 5 && followingNode->associatedObject == NULL && followingNode->position.y <= 8.0f && neighborRow - 1 < 9) {
                        s32 isSpecialNode = followingNode->objectKind == 5;
                        boardEvents[eventCount].eventType = 5;
                        boardEvents[eventCount].playerValues[0] = (isSpecialNode) ? followingNode->playerScores[0] : relatedNode->playerScores[0];
                        boardEvents[eventCount].playerValues[1] = (isSpecialNode) ? followingNode->playerScores[0] : relatedNode->playerScores[1];
                        boardEvents[eventCount].playerValues[2] = (isSpecialNode) ? followingNode->playerScores[0] : relatedNode->playerScores[2];
                        boardEvents[eventCount].playerValues[3] = (isSpecialNode) ? followingNode->playerScores[0] : relatedNode->playerScores[3];
                        boardEvents[eventCount].nodeCount = 1;
                        boardEvents[eventCount].playerMask = 0;
                        boardEvents[eventCount].firstNode = (isSpecialNode) ? followingNode : relatedNode;
                        boardEvents[eventCount].secondNode = (isSpecialNode) ? followingNode : relatedNode;
                        boardEvents[eventCount].nextEvent = 0;
                        if (isSpecialNode) {
                            followingNode->nextGroupNode = NULL;
                            followingNode->nextChainNode = NULL;
                            followingNode->associatedObject = &boardEvents[eventCount];
                        }
                        else {
                            relatedNode->nextGroupNode = NULL;
                            relatedNode->nextChainNode = NULL;
                            relatedNode->associatedObject = &boardEvents[eventCount];
                        }
                        eventCount++;
                    }
                    for (eventIndex = 0; eventIndex < ((followingNode && followingNode->objectKind == 5) ? 2 : 0) + 2; eventIndex++) {
                        relatedNode = (eventIndex == 0) ? followingNode->nextLaneNode : ((eventIndex == 1) ? nextLaneNode : ((eventIndex == 2) ? adjacentNodeA : adjacentNodeB));
                        if (relatedNode && (relatedNode->stateFlags & 0x1)) {
                            if (eventIndex != 1 || followingNode->position.y == relatedNode->position.y) {
                                if (followingNode->objectKind == relatedNode->objectKind) {
                                    if ((followingNode->objectKind >= 1 && followingNode->objectKind <= 4)
                                        || (followingNode->objectKind == 5 && (followingNode->position.y <= 8.0f || relatedNode->position.y <= 8.0f))) {
                                        BoardEvent *eventType = followingNode->associatedObject;
                                        BoardEvent *nodeType = relatedNode->associatedObject;
                                        if (followingNode->associatedObject && relatedNode->associatedObject) {
                                            if (followingNode->associatedObject != relatedNode->associatedObject) {
                                                BoardNode *chainNodeIndex = eventType->firstNode;
                                                BoardNode *chainNode = nodeType->firstNode;
                                                while (1) {
                                                    chainNode->associatedObject = eventType;
                                                    if (!chainNode->nextChainNode) {
                                                        break;
                                                    }
                                                    chainNode = chainNode->nextChainNode;
                                                }
                                                nodeType->firstNode->nextGroupNode = eventType->secondNode;
                                                eventType->secondNode->nextChainNode = nodeType->firstNode;
                                                eventType->pathId[0] = (eventType->pathId[0] > nodeType->pathId[0]) ? eventType->pathId[0] : nodeType->pathId[0];
                                                eventType->pathId[1] = (eventType->pathId[1] > nodeType->pathId[1]) ? eventType->pathId[1] : nodeType->pathId[1];
                                                eventType->pathId[2] = (eventType->pathId[2] > nodeType->pathId[2]) ? eventType->pathId[2] : nodeType->pathId[2];
                                                eventType->pathId[3] = (eventType->pathId[3] > nodeType->pathId[3]) ? eventType->pathId[3] : nodeType->pathId[3];
                                                eventType->stateFlags = eventType->stateFlags + nodeType->stateFlags;
                                                eventType->ownerGroup |= nodeType->ownerGroup;
                                                eventType->secondNode = chainNode;
                                                nodeType->firstNode = nodeType->secondNode = NULL;
                                                nodeType->stateFlags = 0;
                                                nodeType->objectKind = 0;
                                                nodeType->position = eventType;
                                            }
                                        }
                                        else {
                                            if (followingNode->associatedObject) {
                                                relatedNode->associatedObject = followingNode->associatedObject;
                                                eventType->secondNode->nextChainNode = relatedNode;
                                                relatedNode->nextGroupNode = eventType->secondNode;
                                                eventType->secondNode = relatedNode;
                                                eventType->stateFlags++;
                                                eventType->ownerGroup |= relatedNode->playerMask;
                                                eventType->pathId[0] = (eventType->pathId[0] < relatedNode->playerScores[0]) ? relatedNode->playerScores[0] : eventType->pathId[0];
                                                eventType->pathId[1] = (eventType->pathId[1] < relatedNode->playerScores[1]) ? relatedNode->playerScores[1] : eventType->pathId[1];
                                                eventType->pathId[2] = (eventType->pathId[2] < relatedNode->playerScores[2]) ? relatedNode->playerScores[2] : eventType->pathId[2];
                                                eventType->pathId[3] = (eventType->pathId[3] < relatedNode->playerScores[3]) ? relatedNode->playerScores[3] : eventType->pathId[3];
                                            }
                                            else if (relatedNode->associatedObject) {
                                                followingNode->associatedObject = relatedNode->associatedObject;
                                                nodeType->secondNode->nextChainNode = followingNode;
                                                followingNode->nextGroupNode = nodeType->secondNode;
                                                nodeType->secondNode = followingNode;
                                                nodeType->stateFlags++;
                                                nodeType->ownerGroup |= followingNode->playerMask;
                                                nodeType->pathId[0] = (nodeType->pathId[0] < followingNode->playerScores[0]) ? followingNode->playerScores[0] : nodeType->pathId[0];
                                                nodeType->pathId[1] = (nodeType->pathId[1] < followingNode->playerScores[1]) ? followingNode->playerScores[1] : nodeType->pathId[1];
                                                nodeType->pathId[2] = (nodeType->pathId[2] < followingNode->playerScores[2]) ? followingNode->playerScores[2] : nodeType->pathId[2];
                                                nodeType->pathId[3] = (nodeType->pathId[3] < followingNode->playerScores[3]) ? followingNode->playerScores[3] : nodeType->pathId[3];
                                            }
                                            else {
                                                boardEvents[eventCount].eventType = followingNode->objectKind;
                                                boardEvents[eventCount].playerValues[0]
                                                    = (followingNode->playerScores[0] > relatedNode->playerScores[0]) ? followingNode->playerScores[0] : relatedNode->playerScores[0];
                                                boardEvents[eventCount].playerValues[1]
                                                    = (followingNode->playerScores[1] > relatedNode->playerScores[1]) ? followingNode->playerScores[1] : relatedNode->playerScores[1];
                                                boardEvents[eventCount].playerValues[2]
                                                    = (followingNode->playerScores[2] > relatedNode->playerScores[2]) ? followingNode->playerScores[2] : relatedNode->playerScores[2];
                                                boardEvents[eventCount].playerValues[3]
                                                    = (followingNode->playerScores[3] > relatedNode->playerScores[3]) ? followingNode->playerScores[3] : relatedNode->playerScores[3];
                                                boardEvents[eventCount].nodeCount = 2;
                                                boardEvents[eventCount].playerMask = (followingNode->playerMask | relatedNode->playerMask);
                                                boardEvents[eventCount].firstNode = followingNode;
                                                boardEvents[eventCount].secondNode = relatedNode;
                                                boardEvents[eventCount].nextEvent = NULL;
                                                followingNode->nextGroupNode = NULL;
                                                followingNode->nextChainNode = relatedNode;
                                                relatedNode->nextGroupNode = followingNode;
                                                relatedNode->nextChainNode = NULL;
                                                followingNode->associatedObject = &boardEvents[eventCount];
                                                relatedNode->associatedObject = &boardEvents[eventCount];
                                                eventCount++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        for (eventIndex = 0; eventIndex < eventCount; eventIndex++) {
            BoardNode *eventNode;
            if (boardEvents[eventIndex].eventType != 5) {
                continue;
            }
            eventNode = boardEvents[eventIndex].firstNode;
            for (scanIndex = 0; scanIndex < boardEvents[eventIndex].nodeCount; scanIndex++) {
                s32 nodeLane = eventNode->laneIndex + 1;
                s32 nodeRow = (int)eventNode->position.y + 1;
                if (boardEvents[eventIndex].eventType != 0 && nodeRow <= 9) {
                    if (!CHECK_2E48(nodeLane, nodeRow, boardGrid)) {
                        boardEvents[eventIndex].eventType = 0;
                    }
                    else {
                        boardEvents[eventIndex].playerMask |= ((boardGrid[nodeLane - 1][nodeRow]) ? boardGrid[nodeLane - 1][nodeRow]->playerMask : 0)
                            | ((boardGrid[nodeLane + 1][nodeRow]) ? boardGrid[nodeLane + 1][nodeRow]->playerMask : 0)
                            | ((boardGrid[nodeLane][nodeRow - 1]) ? boardGrid[nodeLane][nodeRow - 1]->playerMask : 0)
                            | ((boardGrid[nodeLane][nodeRow + 1]) ? boardGrid[nodeLane][nodeRow + 1]->playerMask : 0)
                            | ((boardGrid[nodeLane + 1][nodeRow - 1]) ? boardGrid[nodeLane + 1][nodeRow - 1]->playerMask : 0)
                            | ((boardGrid[nodeLane + 1][nodeRow + 1]) ? boardGrid[nodeLane + 1][nodeRow + 1]->playerMask : 0)
                            | ((boardGrid[nodeLane - 1][nodeRow - 1]) ? boardGrid[nodeLane - 1][nodeRow - 1]->playerMask : 0)
                            | ((boardGrid[nodeLane - 1][nodeRow + 1]) ? boardGrid[nodeLane - 1][nodeRow + 1]->playerMask : 0);
                    }
                    eventNode = eventNode->nextChainNode;
                    if (!eventNode) {
                        break;
                    }
                }
            }
        }
        for (eventIndex = 0; eventIndex < eventCount; eventIndex++) {
            if ((boardEvents[eventIndex].eventType >= 1 && boardEvents[eventIndex].eventType <= 4 && boardEvents[eventIndex].nodeCount >= 3) || boardEvents[eventIndex].eventType == 5) {
                SelectRandomPlayerFromMask(&boardEvents[eventIndex].playerMask, boardEvents[eventIndex].playerValues);
            }
        }
        eventPass = 1;
        while (eventPass >= 0) {
            s32 eventNodeIndex = 0;
            s32 eventNodeCount = 0;
            for (eventIndex = 0; eventIndex < eventCount; eventIndex++) {
                if ((!eventPass && (boardEvents[eventIndex].eventType >= 1 && boardEvents[eventIndex].eventType <= 4 && boardEvents[eventIndex].nodeCount >= 3))
                    || (eventPass == 1 && boardEvents[eventIndex].eventType == 5)) {
                    BoardNode *eventNode = boardEvents[eventIndex].firstNode;
                    s32 laneScanStart = 0;
                    s32 laneScanCount = 0;
                    float animationProgress = -1;
                    float verticalVelocity = -1;
                    if (boardEvents[eventIndex].eventType == 5) {
                        playerModelHiddenCount++;
                        playerModelHiddenCount %= 5;
                        playerModelHideTimers[playerModelHiddenCount][1] = 1;
                        playerModelHideTimers[playerModelHiddenCount][0] = 0;
                        for (scanIndex = 0; scanIndex < 10; scanIndex++) {
                            Hu3DModelAttrSet(boardObject->mdlId[((playerModelHiddenCount * 10) + 13 + scanIndex)], HU3D_ATTR_DISPOFF);
                        }
                    }
                    for (scanIndex = 0; scanIndex < boardEvents[eventIndex].nodeCount; scanIndex++) {
                        if (-1.0f == animationProgress) {
                            laneScanCount = 1;
                            animationProgress = eventNode->laneIndex;
                            verticalVelocity = eventNode->position.y;
                        }
                        else {
                            laneScanCount++;
                            animationProgress = (animationProgress * (1.0 - (1.0 / laneScanCount))) + (eventNode->laneIndex * (1.0 / laneScanCount));
                            verticalVelocity = (verticalVelocity * (1.0 - (1.0 / laneScanCount))) + (eventNode->position.y * (1.0 / laneScanCount));
                        }
                        if (boardEvents[eventIndex].eventType == 5) {
                            if (playerModelHideTimers[playerModelHiddenCount][0] < 10) {
                                Hu3DModelPosSet(boardObject->mdlId[((playerModelHiddenCount * 10) + 13 + playerModelHideTimers[playerModelHiddenCount][0])],
                                    (((eventNode->laneIndex * 100) - 500.0) - 50.0), 50.0f + ((100.0f * eventNode->position.y) - 500.0f), 120);
                                Hu3DModelAttrReset(
                                    boardObject->mdlId[((playerModelHiddenCount * 10) + 13 + playerModelHideTimers[playerModelHiddenCount][0])], HU3D_ATTR_DISPOFF);
                                Hu3DMotionTimeSet(boardObject->mdlId[((playerModelHiddenCount * 10) + 13 + playerModelHideTimers[playerModelHiddenCount][0])], 2.0f);
                                playerModelHideTimers[playerModelHiddenCount][0]++;
                                cameraShakeState.shakeAmplitude += 4;
                                if (playerModelHideTimers[playerModelHiddenCount][0] == 1) {
                                    cameraShakeState.shakeAmplitude += 7;
                                }
                            }
                            PropagateChainScore(eventNode->pairedNodeLeft, &boardEvents[eventIndex]);
                            PropagateChainScore((eventNode->pairedNodeUpper && eventNode->pairedNodeUpper->pairedNodeLeft) ? eventNode->pairedNodeUpper->pairedNodeLeft : eventNode->pairedNodeUpper, &boardEvents[eventIndex]);
                            PropagateChainScore((eventNode->pairedNodeLower && eventNode->pairedNodeLower->pairedNodeLeft) ? eventNode->pairedNodeLower->pairedNodeLeft : eventNode->pairedNodeLower, &boardEvents[eventIndex]);
                            removedNodes[eventNodeIndex] = eventNode->pairedNodeLeft;
                            eventNodeIndex++;
                            removedNodes[eventNodeIndex] = eventNode->pairedNodeRight;
                            eventNodeIndex++;
                            if (eventNode->pairedNodeUpper) {
                                removedNodes[eventNodeIndex] = eventNode->pairedNodeUpper->pairedNodeLeft;
                                eventNodeIndex++;
                                removedNodes[eventNodeIndex] = eventNode->pairedNodeUpper->pairedNodeRight;
                                eventNodeIndex++;
                            }
                            removedNodes[eventNodeIndex] = eventNode->pairedNodeUpper;
                            eventNodeIndex++;
                            if (eventNode->pairedNodeLower) {
                                removedNodes[eventNodeIndex] = eventNode->pairedNodeLower->pairedNodeLeft;
                                eventNodeIndex++;
                                removedNodes[eventNodeIndex] = eventNode->pairedNodeLower->pairedNodeRight;
                                eventNodeIndex++;
                            }
                            removedNodes[eventNodeIndex] = eventNode->pairedNodeLower;
                            eventNodeIndex++;
                            removedNodes[eventNodeIndex] = eventNode;
                            eventNodeIndex++;
                        }
                        else {
                            PropagateChainScore(eventNode, &boardEvents[eventIndex]);
                            laneScanStart += RemoveBoardNodeAndScore(eventNode, &boardEvents[eventIndex]);
                        }
                        eventNode = eventNode->nextChainNode;
                        if (!eventNode) {
                            break;
                        }
                    }
                    if (boardEvents[eventIndex].eventType == 5 && (boardEvents[eventIndex].playerMask & 0xF)) {
                        PlayerWork *playerWork[4];
                        s32 playerIndex;
                        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                            if ((1 << playerIndex) & (boardEvents[eventIndex].playerMask)) {
                                playerWork[playerIndex] = playerObjects[playerIndex]->data;
                                omVibrate(playerWork[playerIndex]->playerIndex, 12, 6, 6);
                            }
                        }
                    }
                    if (pendingVoiceEvents[(boardEvents[eventIndex].queuedSoundCount == 5) ? 0 : 1].queuedSoundCount < 3) {
                        pendingVoiceEvents[(boardEvents[eventIndex].queuedSoundCount == 5) ? 0 : 1].queuedPanValues[pendingVoiceEvents[(boardEvents[eventIndex].queuedSoundCount == 5) ? 0 : 1].queuedSoundCount]
                            = 0.45 * (((animationProgress - 5.0) - 0.5) / 5.5);
                        pendingVoiceEvents[(boardEvents[eventIndex].queuedSoundCount == 5) ? 0 : 1].queuedSoundCount++;
                    }
                    if (eventPass == 0 && laneScanStart > 0 && winningPlayer == -1) {
                        s32 playerCount = 0;
                        s32 voicePlayerIndex = GetRandomSelectedPlayerValue(&boardEvents[eventIndex].playerMask, boardEvents[eventIndex].playerValues);
                        Vec popupWorldPosition = { 0, 0, 50 };
                        s32 playerIndex;
                        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                            playerCount += ((1 << playerIndex) != 0) & boardEvents[eventIndex].playerMask;
                        }
                        playerCount--;
                        popupWorldPosition.x = (100.0f * animationProgress) - 500.0;
                        popupWorldPosition.y = 100 + ((100 * verticalVelocity) - 500);
                        popupWorldPosition.x += playerCount * 12;
                        popupWorldPosition.y -= playerCount * 30;
                        for (playerIndex = 3; playerIndex >= 0; playerIndex--) {
                            if ((1 << playerIndex) & boardEvents[eventIndex].playerMask) {
                                Vec popupScreenPosition;
                                scorePopupPool[scorePopupHead].lifetimeFrames = 1;
                                scorePopupPool[scorePopupHead].scoreValue = laneScanStart;
                                scorePopupPool[scorePopupHead].playerMask = 1 << playerIndex;
                                Hu3D3Dto2D(&popupWorldPosition, 1, &popupScreenPosition);
                                scorePopupPool[scorePopupHead].screenX = popupScreenPosition.x;
                                scorePopupPool[scorePopupHead].screenY = popupScreenPosition.y;
                                scorePopupHead++;
                                scorePopupHead %= 50;
                                popupWorldPosition.x -= 24.0f;
                                popupWorldPosition.y += 60.0f;
                            }
                        }
                        voicePlayerIndex = (voicePlayerIndex > 3) ? 3 : voicePlayerIndex;
                        if (pendingVoiceEvents[voicePlayerIndex + 3].queuedSoundCount < 3) {
                            pendingVoiceEvents[voicePlayerIndex + 3].queuedPanValues[pendingVoiceEvents[voicePlayerIndex + 3].queuedSoundCount] = 0.45 * (((animationProgress - 5.0) - 0.5) / 5.5);
                            pendingVoiceEvents[voicePlayerIndex + 3].queuedSoundCount++;
                        }
                    }
                    activeEvents[eventNodeCount] = &boardEvents[eventIndex];
                    eventNodeCounts[eventNodeCount] = eventNodeIndex;
                    eventLanePositions[eventNodeCount] = animationProgress;
                    eventRowPositions[eventNodeCount] = verticalVelocity;
                    eventNodeCount++;
                }
            }
            for (eventIndex = 0; eventIndex < eventNodeCount; eventIndex++) {
                s32 removedScore = 0;

                for (scanIndex = (eventIndex < 1) ? 0 : eventNodeCounts[eventIndex - 1]; scanIndex < eventNodeCounts[eventIndex]; scanIndex++) {
                    removedScore += RemoveBoardNodeAndScore(removedNodes[scanIndex], activeEvents[eventIndex]);
                }
                if (eventPass == 1 && removedScore > 0 && winningPlayer == -1) {
                    s32 playerIndex;
                    s32 playerCount = 0;
                    s32 voicePlayerIndex = GetRandomSelectedPlayerValue(&activeEvents[eventIndex]->ownerGroup, activeEvents[eventIndex]->pathId);
                    Vec popupWorldPosition = { 0, 0, 50 };
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        playerCount += ((1 << playerIndex) != 0) & activeEvents[eventIndex]->ownerGroup;
                    }
                    playerCount--;
                    popupWorldPosition.x = (100.0f * eventLanePositions[eventIndex]) - 500.0;
                    popupWorldPosition.y = 100 + ((100 * eventRowPositions[eventIndex]) - 500);
                    popupWorldPosition.x += playerCount * 12;
                    popupWorldPosition.y -= playerCount * 30;
                    for (playerIndex = 3; playerIndex >= 0; playerIndex--) {
                        if ((1 << playerIndex) & activeEvents[eventIndex]->ownerGroup) {
                            Vec popupScreenPosition;
                            scorePopupPool[scorePopupHead].lifetimeFrames = 1;
                            scorePopupPool[scorePopupHead].scoreValue = removedScore;
                            scorePopupPool[scorePopupHead].playerMask = 1 << playerIndex;
                            Hu3D3Dto2D(&popupWorldPosition, 1, &popupScreenPosition);
                            scorePopupPool[scorePopupHead].screenX = popupScreenPosition.x;
                            scorePopupPool[scorePopupHead].screenY = popupScreenPosition.y;
                            scorePopupHead++;
                            scorePopupHead %= 50;
                            popupWorldPosition.x -= 24.0f;
                            popupWorldPosition.y += 60.0f;
                        }
                    }
                    voicePlayerIndex = (voicePlayerIndex > 3) ? 3 : voicePlayerIndex;
                    if (pendingVoiceEvents[voicePlayerIndex + 3].queuedSoundCount < 3) {
                        pendingVoiceEvents[voicePlayerIndex + 3].queuedPanValues[pendingVoiceEvents[voicePlayerIndex + 3].queuedSoundCount] = 0.45 * (((eventLanePositions[eventIndex] - 5.0) - 0.5) / 5.5);
                        pendingVoiceEvents[voicePlayerIndex + 3].queuedSoundCount++;
                    }
                }
            }
            eventPass--;
        }
    }
}
#undef CHECK_2E48

void PropagateChainScore(BoardNode *node, BoardEvent *event)
{
    BoardNode *followingChainNode = node;
    if (!node) {
        return;
    }
    if (node->objectKind == 0) {
        return;
    }
    while (followingChainNode->nextLaneNode) {
        if (!followingChainNode->nextLaneNode->objectKind || (followingChainNode->nextLaneNode->stateFlags & 0x1) == 0 || followingChainNode->nextLaneNode->position.y != followingChainNode->position.y + 1.0f) {
            break;
        }
        followingChainNode = followingChainNode->nextLaneNode;
        followingChainNode->playerScores[0] = event->playerValues[0] + ((event->playerMask & 0x1) ? 1 : 0);
        followingChainNode->playerScores[1] = event->playerValues[1] + ((event->playerMask & 0x2) ? 1 : 0);
        followingChainNode->playerScores[2] = event->playerValues[2] + ((event->playerMask & 0x4) ? 1 : 0);
        followingChainNode->playerScores[3] = event->playerValues[3] + ((event->playerMask & 0x8) ? 1 : 0);
        followingChainNode->playerMask = event->playerMask;
    }
}

void UpdateGameplaySequence(OMOBJ *object)
{
    s32 playerIndex;
    s32 digitIndex;

    PlayerWork *playerStates[4];
    frameCounter++;
    {
        s32 playerIndex;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            UpdatePlayerObject(playerObjects[playerIndex]);
            ResolveBoardMatches(playerIndex);
        }
    }

    ResolveBoardMatches(4);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerStates[playerIndex] = playerObjects[playerIndex]->data;
    }
    if (winningPlayer == -1) {
        if (playerStates[0]->targetScore >= 100 || playerStates[1]->targetScore >= 100 || playerStates[2]->targetScore >= 100 || playerStates[3]->targetScore >= 100) {
            s32 remainingPlayerOrder[4] = { 0, 1, 2, 3 };
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                s32 randomPlayerIndex = rand8() % 4;
                if (playerStates[randomPlayerIndex]->targetScore >= 100) {
                    winningPlayer = randomPlayerIndex;
                    for (digitIndex = 0; digitIndex < 4; digitIndex++) {
                        if (digitIndex != randomPlayerIndex) {
                            if (playerStates[digitIndex]->targetScore >= 100) {
                                playerStates[digitIndex]->targetScore = playerStates[digitIndex]->previousScore;
                            }
                        }
                    }
                }
                else {
                    for (digitIndex = randomPlayerIndex; digitIndex < 3 - playerIndex; digitIndex++) {
                        remainingPlayerOrder[digitIndex] = remainingPlayerOrder[digitIndex + 1];
                    }
                }
            }
        }
    }
    playerIndex = (playerStates[0]->turnComplete != 0) + (playerStates[1]->turnComplete != 0) + (playerStates[2]->turnComplete != 0) + (playerStates[3]->turnComplete != 0);
    if (winningPlayer == -1 && playerIndex == 3) {
        for (digitIndex = 0; digitIndex < 4; digitIndex++) {
            if (!playerStates[digitIndex]->inputEnabled) {
                winningPlayer = digitIndex;
            }
        }
    }
    if (winningPlayer == -1 && playerIndex == 4) {
        winningPlayer = 4;
    }
    if (winningPlayer != -1) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerStates[playerIndex]->boardNodes[0]) {
                playerStates[playerIndex]->boardNodes[0]->stateFlags &= ~0x2;
            }
            if (playerStates[playerIndex]->boardNodes[1]) {
                playerStates[playerIndex]->boardNodes[1]->stateFlags &= ~0x2;
            }
            playerStates[playerIndex]->boardNodes[0] = NULL;
            playerStates[playerIndex]->boardNodes[1] = NULL;
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerStates[playerIndex]->scoreHoldFrames = (playerStates[playerIndex]->displayedScore == playerStates[playerIndex]->targetScore) ? playerStates[playerIndex]->scoreHoldFrames + 1 : 0;
        playerStates[playerIndex]->displayedScore += (playerStates[playerIndex]->displayedScore < playerStates[playerIndex]->targetScore) ? 0.375 : 0;
        playerStates[playerIndex]->displayedScore = (playerStates[playerIndex]->displayedScore > playerStates[playerIndex]->targetScore) ? (float)playerStates[playerIndex]->targetScore : playerStates[playerIndex]->displayedScore;
        for (digitIndex = 0; digitIndex < 3; digitIndex++) {
            float animatedScore = playerStates[playerIndex]->displayedScore;
            float hundredsDigit = (s32)(animatedScore / 100.0f) % 10;
            float tensDigit = (s32)(animatedScore / 10.0f) % 10;
            float onesDigit = ((s32)animatedScore) % 10;
            float fractionalDigit = animatedScore - (s32)animatedScore;
            float displayedDigit = (digitIndex == 0) ? ((playerStates[playerIndex]->targetScore / 100) % 10)
                                             : ((digitIndex == 1) ? ((playerStates[playerIndex]->targetScore / 10) % 10) : playerStates[playerIndex]->targetScore % 10);
            s32 digitOffsets[5] = { 0, 15, -9, 3, 0 };
            switch (digitIndex) {
                case 0:
                    displayedDigit = hundredsDigit + (fractionalDigit * ((9.0f == onesDigit && 9.0f == tensDigit) ? 1.0 : 0.0));
                    break;

                case 1:
                    displayedDigit = tensDigit + (fractionalDigit * ((9.0f == onesDigit) ? 1.0 : 0.0));
                    break;

                case 2:
                    displayedDigit = onesDigit + fractionalDigit;
                    break;
            }
            Hu3DModelRotSet(playerObjects[playerIndex]->mdlId[digitIndex + 1], 36.0f * displayedDigit, 0, 0);
        }
    }
    {
        s32 playerColors[8][3] = { 255, 64, 64, 32, 192, 255, 255, 128, 255, 32, 208, 32, 224, 32, 224, 230, 128, 32, 255, 240, 32, 160, 160, 160 };
        s32 soundHandle = scorePopupHead;
        s32 spriteIndex = 0;
        PlayerWork *playerStates[4];
        for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
            playerStates[spriteIndex] = playerObjects[spriteIndex]->data;
        }
        do {
            soundHandle--;
            soundHandle = (soundHandle < 0) ? soundHandle + 50 : soundHandle;
            if (scorePopupPool[soundHandle].lifetimeFrames == 0 || (scorePopupPool[soundHandle].playerMask & 0xF) == 0) {
                HuSprAttrSet(popupSpriteGroup, spriteIndex, HUSPR_ATTR_DISPOFF);
            }
            else {
                s32 activePlayerCount = 0;
                u32 playerCharacter;
                s32 digitCount;

                s32 glyphIndex;

                s32 activePlayerIndices[4];
                digitCount = ((scorePopupPool[soundHandle].scoreValue >= 10) ? 2 : 1) + 1;
                if (scorePopupPool[soundHandle].playerMask & 0x1) {
                    activePlayerIndices[activePlayerCount] = 0;
                    activePlayerCount++;
                }
                if (scorePopupPool[soundHandle].playerMask & 0x2) {
                    activePlayerIndices[activePlayerCount] = 1;
                    activePlayerCount++;
                }
                if (scorePopupPool[soundHandle].playerMask & 0x4) {
                    activePlayerIndices[activePlayerCount] = 2;
                    activePlayerCount++;
                }
                if (scorePopupPool[soundHandle].playerMask & 0x8) {
                    activePlayerIndices[activePlayerCount] = 3;
                    activePlayerCount++;
                }
                playerCharacter = playerStates[activePlayerIndices[frameCounter % activePlayerCount]]->laneIndex;
                for (glyphIndex = 0; glyphIndex < digitCount; spriteIndex++, glyphIndex++) {
                    HuSprPosSet(popupSpriteGroup, spriteIndex, (glyphIndex * 14) + (scorePopupPool[soundHandle].screenX - ((digitCount * 14) / 2)),
                        scorePopupPool[soundHandle].screenY - scorePopupPool[soundHandle].lifetimeFrames);
                    HuSprAttrReset(popupSpriteGroup, spriteIndex, HUSPR_ATTR_DISPOFF);
                    HuSprColorSet(popupSpriteGroup, spriteIndex, playerColors[playerCharacter][0], playerColors[playerCharacter][1], playerColors[playerCharacter][2]);
                    HuSprTPLvlSet(popupSpriteGroup, spriteIndex,
                        (scorePopupPool[soundHandle].lifetimeFrames <= 30) ? 1.0f : (1.0f - ((scorePopupPool[soundHandle].lifetimeFrames - 30) / 10.0f)));
                    HuSprBankSet(popupSpriteGroup, spriteIndex,
                        (glyphIndex == 0)
                            ? 10
                            : ((glyphIndex == 2 || digitCount == 2) ? scorePopupPool[soundHandle].scoreValue % 10 : ((scorePopupPool[soundHandle].scoreValue / 10) % 10)));
                }
            }
            scorePopupPool[soundHandle].lifetimeFrames += scorePopupPool[soundHandle].lifetimeFrames > 0;
            if (scorePopupPool[soundHandle].lifetimeFrames >= 40) {
                scorePopupPool[soundHandle].lifetimeFrames = 0;
            }

        } while (soundHandle != scorePopupHead);
        for (; spriteIndex < 150; spriteIndex++) {
            HuSprAttrSet(popupSpriteGroup, spriteIndex, HUSPR_ATTR_DISPOFF);
        }
    }
    {
        s32 pieceCursor = fallingPieceHead;
        s32 hideTimerIndex;
        s32 particleIndex;
        for (particleIndex = 0; particleIndex < 300; particleIndex++) {
            pieceCursor = pieceCursor + 1;
            pieceCursor %= 300;
            if (!fallingPiecePool[pieceCursor].pieceType) {
                continue;
            }
            fallingPiecePool[pieceCursor].position.x += fallingPiecePool[pieceCursor].velocity.x;
            fallingPiecePool[pieceCursor].position.y += fallingPiecePool[pieceCursor].velocity.y;
            fallingPiecePool[pieceCursor].position.z += fallingPiecePool[pieceCursor].velocity.z;
            fallingPiecePool[pieceCursor].velocity.y += -1.5;
            if (fallingPiecePool[pieceCursor].position.y < -1000.0f) {
                fallingPiecePool[pieceCursor].pieceType = 0;
            }
            fallingPiecePool[pieceCursor].rotationPhase += 9;
        }
        for (hideTimerIndex = 0; hideTimerIndex < 5; hideTimerIndex++) {
            if (playerModelHideTimers[hideTimerIndex][0] > 0) {
                playerModelHideTimers[hideTimerIndex][1]++;
                if (playerModelHideTimers[hideTimerIndex][1] > 30) {
                    s32 modelIndex;
                    playerModelHideTimers[hideTimerIndex][0] = 0;
                    playerModelHideTimers[hideTimerIndex][1] = 0;
                    for (modelIndex = 0; modelIndex < 10; modelIndex++) {
                        Hu3DModelAttrSet(object->mdlId[(hideTimerIndex * 10) + 13 + modelIndex], HU3D_ATTR_DISPOFF);
                    }
                }
            }
        }
        if (mainSequenceObject->work[0] >= 1002) {
            float cameraProgress = 0;
            float cameraBlend = 0;
            if (cameraShakeState.shakePhase % 2 >= 0) {
                cameraShakeState.previousHorizontalOffset = cameraShakeState.shakeDirection.y;
                cameraShakeState.shakeDirection.x = cameraShakeState.shakeDirection.z;
                do {
                    cameraShakeState.shakeDirection.y = (2.0 * (rand8() / 255.0)) - 1.0;
                    cameraShakeState.shakeDirection.z = (2.0 * (rand8() / 255.0)) - 1.0;
                    cameraShakeState.shakeDirection.y = 0;
                } while (VECMagPoint(cameraShakeState.shakeDirection.y - cameraShakeState.previousHorizontalOffset, cameraShakeState.shakeDirection.z - cameraShakeState.shakeDirection.x, 0) < 0.75);
            }
            cameraProgress = cameraShakeState.previousHorizontalOffset;
            cameraBlend = cameraShakeState.shakeDirection.x;
            cameraProgress *= cameraShakeState.shakeAmplitude * 2;
            cameraBlend *= cameraShakeState.shakeAmplitude * 2;
            cameraShakeState.shakePhase = !cameraShakeState.shakePhase;
            cameraShakeState.shakeAmplitude = (cameraShakeState.shakeAmplitude > 24) ? 23 : ((cameraShakeState.shakeAmplitude - 1 > 0) ? cameraShakeState.shakeAmplitude - 1 : 0);
            Hu3DCameraPosSet(1, cameraProgress, 37.139069 + cameraBlend, 2599.7346f, 0, 1, 0, cameraProgress, 50.0f + cameraBlend, 0);
        }
        if (winningPlayer != -1) {
            float cameraHorizontalOffset;
            s32 soundIndex;
            winnerSoundFadeTimer++;
            cameraHorizontalOffset = 1.0f - ((winnerSoundFadeTimer > REFRESH_RATE * 3) ? 1.0f : (winnerSoundFadeTimer / (REFRESH_RATE_F * 3)));
            Hu3DMotionSpeedSet(boardObject->mdlId[0], 0.5 * cameraHorizontalOffset);
            for (soundIndex = 0; soundIndex < 4; soundIndex++) {
                Hu3DMotionSpeedSet(playerObjects[soundIndex]->mdlId[5], 0.5 * cameraHorizontalOffset);
            }
            if (cameraHorizontalOffset > 0.0f && winnerSoundEffects[0] != -1) {
                HuAudFXPitchSet(winnerSoundEffects[0], -8192.0f + (8192.0f * cameraHorizontalOffset));
                HuAudFXPitchSet(winnerSoundEffects[1], -8192.0f + (8192.0f * cameraHorizontalOffset));
                HuAudFXVolSet(winnerSoundEffects[0], 63.0f + (64.0f * cameraHorizontalOffset));
                HuAudFXVolSet(winnerSoundEffects[1], 63.0f + (64.0f * cameraHorizontalOffset));
            }
            else {
                if (winnerSoundEffects[0] != -1) {
                    HuAudFXStop(winnerSoundEffects[0]);
                    HuAudFXStop(winnerSoundEffects[1]);
                    winnerSoundEffects[0] = winnerSoundEffects[1] = -1;
                }
            }
        }
    }
}

void UpdateCameraSequence(OMOBJ *object);

void UpdateBoardCamera(OMOBJ *object)
{
    PlayerWork *playerStates[4];
    s32 playerIndex;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerStates[playerIndex] = playerObjects[playerIndex]->data;
    }
    for (playerIndex = 0; playerIndex < 7; playerIndex++) {
        s32 voiceEffectIds[7] = { 1422, 1423, 1424, 1425, 1426, 1427, 1428 };
        if (pendingVoiceEvents[playerIndex].queuedSoundCount > 3) {
            pendingVoiceEvents[playerIndex].queuedSoundCount = 3;
        }
        if (pendingVoiceEvents[playerIndex].queuedSoundCount > 0) {
            s32 voiceHandle = HuAudFXPlay(voiceEffectIds[playerIndex]);
            HuAudFXPanning(voiceHandle, 0.5 + ((63 * pendingVoiceEvents[playerIndex].queuedPanValues[pendingVoiceEvents[playerIndex].queuedSoundCount - 1]) + 64));
            pendingVoiceEvents[playerIndex].queuedSoundCount--;
        }
    }
    if (omSysExitReq && !WipeStatGet()) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = UpdateCameraSequence;
    }
    switch (object->work[0]) {
        case 1000:
            if (!WipeStatGet()) {
                s32 brightness;
                s32 playerIndex;
                s32 motionIndex;
                s32 playerIndex;
                sequenceTimer++;
                if (sequenceTimer == 10.0f) {
#if VERSION_PAL
                    sequenceTimer = 59.0f;
#else
                    sequenceTimer = 50.0f;
#endif
                }
                brightness = (sequenceTimer > 180.0f) ? 255.0 : ((sequenceTimer > 120.0f) ? (255.0 * ((sequenceTimer - 120.0f) / 60.0)) : 0.0);
                brightness = (sequenceTimer >= 235.0f)
                    ? 255
                    : ((sequenceTimer >= 205.0f) ? 192 : ((sequenceTimer >= 190.0f) ? 128 : (((sequenceTimer >= 160.0f) ? 64 : 0))));
                Hu3DGLightColorSet(boardLightId, brightness, brightness, brightness, 255);
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {

                    Hu3DMotionTimeSet(playerObjects[playerIndex]->mdlId[0], 60.0 + (40.0 * (brightness / 255.0)));
                    for (motionIndex = 0; motionIndex < 3; motionIndex++) {
                        Hu3DMotionTimeSet(playerObjects[playerIndex]->mdlId[motionIndex + 1], 60.0 + (40.0 * (brightness / 255.0)));
                    }
                    Hu3DMotionTimeSet(playerObjects[playerIndex]->mdlId[4], 60.0 + (40.0 * (brightness / 255.0)));
                }
                Hu3DMotionSpeedSet(
                    boardObject->mdlId[0], 0.5 * ((sequenceTimer < 100.0f) ? 0.0 : ((sequenceTimer < 320.0f) ? ((sequenceTimer - 100) / 220.0) : 1.0)));
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    Hu3DMotionSpeedSet(playerObjects[playerIndex]->mdlId[5],
                        0.5 * ((sequenceTimer < 100.0f) ? 0.0 : ((sequenceTimer < 280.0f) ? ((sequenceTimer - 100) / 180.0) : 1.0)));
                }
                if (100.0f == sequenceTimer) {
                    Hu3DMotionSpeedSet(boardObject->mdlId[1], 1.0f);
                    omVibrate(playerStates[0]->playerIndex, 12, 4, 2);
                    omVibrate(playerStates[1]->playerIndex, 12, 4, 2);
                    omVibrate(playerStates[2]->playerIndex, 12, 4, 2);
                    omVibrate(playerStates[3]->playerIndex, 12, 4, 2);
                    HuAudFXPlay(1431);
                }
                if (145.0f == sequenceTimer) {
                    winnerSoundEffects[0] = HuAudFXPlay(1429);
                }
                if (160.0f == sequenceTimer) {
                    winnerSoundEffects[1] = HuAudFXPlay(1430);
                }
                if (350.0f < sequenceTimer) {
                    object->work[0]++;
                    sequenceTimer = 0;
                }
            }
            break;

        case 1001:
            sequenceTimer++;
            if (5.0f == sequenceTimer && minigameSequenceId < 0) {
                minigameSequenceId = MGSeqStartCreate();
            }
            if (sequenceTimer >= 5.0f && sequenceAudioId == -1 && (MGSeqStatGet(minigameSequenceId) & 0x10)) {
                sequenceAudioId = HuAudSeqPlay(71);
            }
            if (sequenceTimer >= 5.0f && !MGSeqStatGet(minigameSequenceId) && minigameSequenceId >= 0) {
                object->work[0]++;
                minigameSequenceId = -1;
                sequenceTimer = 0;
            }
            else {
                float cameraBlend = (sequenceTimer > 60.0f) ? 1.0 : ((sequenceTimer > 0.0f) ? (sequenceTimer / 60.0) : 0.0);
                float cameraBlendInverse = 1.0f - cameraBlend;
                float cameraX = 830.0f * cameraBlendInverse;
                float cameraY = (-1792.0f * cameraBlendInverse) + (50.0f * cameraBlend);
                float cameraZ = (2854.0f * cameraBlendInverse) + (3500.0f * cameraBlend);
                float cameraDistanceScale = ((2578.212 * cameraBlendInverse) + (2600.0 * cameraBlend)) / VECMagPoint(cameraX, cameraY, cameraZ);
                cameraX *= cameraDistanceScale;
                cameraY *= cameraDistanceScale;
                cameraZ *= cameraDistanceScale;
                Hu3DCameraPosSet(1, cameraX, cameraY, cameraZ, -0.05f * cameraBlendInverse, 1, 0, 0, 50 * cameraBlend, 0);
                (void)cameraBlendInverse;
            }
            break;

        case 1002:
            if (winningPlayer != -1) {
                object->work[0] = 1003;
                sequenceTimer = 0;
            }
            break;

        case 1003:
            if (minigameSequenceId < 0 && sequenceTimer == 0.0f) {
                minigameSequenceId = MGSeqFinishCreate();
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    PlayerWork *playerState = playerObjects[playerIndex]->data;
                    HuSprAttrSet(playerState->playerSpriteGroup, 0, HUSPR_ATTR_DISPOFF);
                }
                if (sequenceAudioId != -1) {
                    HuAudSeqFadeOut(sequenceAudioId, 100);
                }
            }
            else if (!MGSeqStatGet(minigameSequenceId)) {
                sequenceTimer++;
                if (sequenceTimer >= REFRESH_RATE_F / 2) {
                    object->work[0]++;
                    sequenceTimer = 0;
                    if (winningPlayer != 4) {
                        minigameSequenceId = MGSeqWinCreate(playerStates[winningPlayer]->characterId, -1, -1, -1);
                        GWPlayerCoinWinSet(playerStates[winningPlayer]->playerIndex, 10);
                        HuAudSStreamPlay(1);
                    }
                    else if (winningPlayer == 4) {
                        minigameSequenceId = MGSeqDrawCreate();
                        HuAudSStreamPlay(4);
                    }
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        Hu3DModelRotSet(playerObjects[playerIndex]->mdlId[4], 0, (winningPlayer == playerIndex) ? 90 : -90, 0);
                    }
                }
            }
            break;

        case 1004:
            sequenceTimer++;
            if (REFRESH_RATE_F * 3.5f < sequenceTimer) {
                minigameSequenceId = -1;
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = UpdateCameraSequence;
            }
            break;

        default:
            OSReport("*** main mode error(%d)!!\n", object->work[0]);

            break;
    }
}

void ApplyBoardModelTransform(HU3DMODEL *model, Mtx transform)
{
    GXLoadPosMtxImm(transform, GX_PNMTX0);
    GXSetNumTevStages(1);
    GXSetNumTexGens(0);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    {
        s8 pieceModelIndices[9] = { -1, 0, 1, 2, 3, 4, -1, -1, -1 };
        char *pieceModelNames[6] = { "m413_11-m413_11", "m413_12-m413_12", "m413_13-m413_13", "m413_14-m413_14", "g005m3A-bomhei_A", "g005m3B-bomhei_B" };
        s8 characterModelIndices[6] = { 3, 4, 5, 6, 11, 12 };
        char *characterModelNames[4] = { "m413_11b-m413_11b", "m413_12b-m143_12b", "m413_13b-m413_13b", "twest-m413_14b" };
        s8 fallingPieceIndices[4] = { 7, 8, 9, 10 };
        s32 laneIndex;
        Mtx modelTransform;
        Mtx auxiliaryTransform;
        for (laneIndex = 0; laneIndex < 12; laneIndex++) {
            BoardNode *boardNode;
            for (boardNode = laneHeads[laneIndex]; boardNode; boardNode = boardNode->nextLaneNode) {
                float pieceHeight;
                if (boardNode->objectKind) {
                    s32 horizontalScale = 100;
                    s32 verticalScale = 100;
                    float screenX = boardNode->position.x;
                    float screenY = boardNode->position.y;
                    float screenZ = 0;
                    if (boardNode->pathId && boardNode->animationFrame) {
                        s32 rowDirections[4] = { 0, 1, 0, -1 };
                        s32 laneDirections[4] = { 1, 0, -1, 0 };
                        s32 pathCorner = (boardNode->pathId + 3) % 4;
                        s32 pathIndex = ((((boardNode->pathId + 3) % 4) + (9 + ((boardNode->pathId - 1) / 4)))) % 4;
                        s32 curveStep = 0;
                        float curveProgress = 1;
                        screenX -= rowDirections[pathIndex];
                        screenY -= laneDirections[pathIndex];
                        switch ((boardNode->pathId - 1) / 4) {
                            case 0:
                            case 2:
                                for (curveStep = boardNode->animationFrame - 1; curveStep > 0; curveStep--) {
                                    curveProgress /= 2.0;
                                }
                                pieceHeight = 1.0f - curveProgress;
                                pathIndex = (pathCorner + 3 == pathIndex) ? (pathIndex - 4) : ((pathCorner - 3 == pathIndex) ? (pathIndex + 4) : pathIndex);
                                screenX += sind((curveProgress * (pathCorner * 90)) + (pieceHeight * (pathIndex * 90)));
                                screenY += cosd((curveProgress * (pathCorner * 90)) + (pieceHeight * (pathIndex * 90)));
                                break;

                            case 1:
                                for (curveStep = boardNode->animationFrame - 1; curveStep > 0; curveStep--) {
                                    curveProgress /= 2.0;
                                }
                                pieceHeight = (boardNode->animationFrame - 1) / 3.0;
                                pieceHeight = (pieceHeight < 0.0f) ? 0.0f : ((pieceHeight > 1.0f) ? 1.0f : pieceHeight);
                                pieceHeight = (0.75 * ((1.0f - curveProgress))) + (0.25 * pieceHeight);
                                curveProgress = 1.0f - pieceHeight;
                                screenX += rowDirections[pathCorner] * cosd(pieceHeight * 180.0f);
                                screenY += laneDirections[pathCorner] * cosd(pieceHeight * 180.0f);
                                screenZ += sind(pieceHeight * 180.0f);

                                break;

                            default:
                                break;
                        }
                    }
                    screenX = (screenX * horizontalScale) - (horizontalScale * 6);
                    screenY = (screenY * verticalScale) - (verticalScale * 5);
                    screenZ = 100 * screenZ;
                    if (pieceModelIndices[boardNode->objectKind] != -1) {
                        s32 pieceModelIndex = pieceModelIndices[boardNode->objectKind]
                            + (boardNode->objectKind == 5 && ((boardNode->position.z < 0.0f && (boardNode->stateFlags & 0x1) == 0) || (boardNode->stateFlags & 0x2) != 0));
                        MTXTrans(modelTransform, 50 + screenX, 50 + screenY, screenZ);
                        MTXConcat(transform, modelTransform, modelTransform);
                        Hu3DModelObjDraw(boardObject->mdlId[characterModelIndices[pieceModelIndex]], pieceModelNames[pieceModelIndex], modelTransform);
                    }
                }
            }
        }
        for (laneIndex = 0; laneIndex < 4; laneIndex++) {
            s32 playerIndex;
            for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                s32 pieceType = 25;
                s32 playerScreenX = (laneIndex * 300) - 450;
                s32 playerScreenY = (505 - pieceType) + (playerIndex * (pieceType * 2));
                PlayerWork *playerWork = playerObjects[laneIndex]->data;
                s32 pieceKind = playerWork->cardLayout[playerWork->cardColumn][playerIndex];
                if (playerWork->cardAnimationFrames[0] == 0 || playerWork->cardAnimationFrames[0] > 7) {
                    if (pieceModelIndices[pieceKind] != -1) {
                        MTXScale(modelTransform, 0.5f, 0.5f, 0.5f);
                        MTXTrans(auxiliaryTransform, playerScreenX, playerScreenY + ((playerWork->cardAnimationFrames[0]) ? (200.0f * ((15 - playerWork->cardAnimationFrames[0]) / 15.0f)) : 0.0f), 125.0f);
                        MTXConcat(auxiliaryTransform, modelTransform, modelTransform);
                        MTXConcat(transform, modelTransform, modelTransform);
                        Hu3DModelObjDraw(boardObject->mdlId[characterModelIndices[pieceModelIndices[pieceKind]]], pieceModelNames[pieceModelIndices[pieceKind]], modelTransform);
                    }
                }
                if (playerWork->cardAnimationFrames[0] != 0 && playerWork->cardAnimationFrames[0] < (15 - (playerIndex * 4)) && pieceModelIndices[playerWork->cardTypes[playerIndex]] != -1) {
                    MTXScale(modelTransform, 0.5f, 0.5f, 0.5f);
                    MTXTrans(auxiliaryTransform, playerScreenX, playerScreenY, 125.0 - (0.25 * (40.0f * (playerWork->cardAnimationFrames[0] / 15.0f))));
                    MTXConcat(auxiliaryTransform, modelTransform, modelTransform);
                    MTXConcat(transform, modelTransform, modelTransform);
                    Hu3DModelObjDraw(boardObject->mdlId[characterModelIndices[pieceModelIndices[playerWork->cardTypes[playerIndex]]]], pieceModelNames[pieceModelIndices[playerWork->cardTypes[playerIndex]]], modelTransform);
                }
            }
        }
        {
            s32 particleCursor = fallingPieceHead;
            s32 particleIndex;
            for (particleIndex = 0; particleIndex < 300; particleIndex++) {
                particleCursor++;
                particleCursor %= 300;
                if (fallingPiecePool[particleCursor].pieceType) {
                    MTXScale(modelTransform, 0.4f, 0.4f, 0.4f);
                    MTXRotDeg(auxiliaryTransform, 'x', 360.0 * (fallingPiecePool[particleCursor].rotationPhase / 255.0));
                    MTXConcat(auxiliaryTransform, modelTransform, modelTransform);
                    MTXRotDeg(auxiliaryTransform, 'z', 360.0 * (fallingPiecePool[particleCursor].spinPhase / 255.0));
                    MTXConcat(auxiliaryTransform, modelTransform, modelTransform);
                    MTXTrans(auxiliaryTransform, fallingPiecePool[particleCursor].position.x, fallingPiecePool[particleCursor].position.y, fallingPiecePool[particleCursor].position.z);
                    MTXConcat(auxiliaryTransform, modelTransform, modelTransform);
                    MTXConcat(transform, modelTransform, modelTransform);
                    Hu3DModelObjDraw(boardObject->mdlId[fallingPieceIndices[pieceModelIndices[fallingPiecePool[particleCursor].pieceType]]], characterModelNames[pieceModelIndices[fallingPiecePool[particleCursor].pieceType]], modelTransform);
                }
            }
        }
    }
}

void InitializeBoardObject(OMOBJ *object)
{
    s32 modelIndex;
    memset(boardNodePool, 0, sizeof(boardNodePool));
    memset(laneHeads, 0, sizeof(laneHeads));
    memset(scorePopupPool, 0, sizeof(scorePopupPool));
    scorePopupHead = 0;
    memset(fallingPiecePool, 0, sizeof(fallingPiecePool));
    fallingPieceHead = 0;
    memset(playerModelHideTimers, 0, sizeof(playerModelHideTimers));
    playerModelHiddenCount = 0;
    memset(cameraShakeState, 0, sizeof(cameraShakeState));
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M413, 0x00));
    omSetTra(object, 0, 0, 0);
    Hu3DMotionSpeedSet(object->mdlId[0], 0);
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M413, 0x01));
    Hu3DMotionSpeedSet(object->mdlId[1], 0);
    Hu3DModelPosSet(object->mdlId[1], 0, 0, 0);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    object->mdlId[2] = Hu3DHookFuncCreate(ApplyBoardModelTransform);
    Hu3DModelLayerSet(object->mdlId[2], 1);
    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
        object->mdlId[modelIndex + 3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M413, 0x03) + modelIndex);
        Hu3DModelAttrSet(object->mdlId[modelIndex + 3], HU3D_ATTR_DISPOFF);
        object->mdlId[modelIndex + 7] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M413, 0x07) + modelIndex);
        Hu3DModelAttrSet(object->mdlId[modelIndex + 7], HU3D_ATTR_DISPOFF);
    }
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        s32 modelDataId;
        if (modelIndex == 0) {
            modelDataId = DATA_MAKE_NUM(DATADIR_M413, 0x0B);
        }
        else {
            modelDataId = DATA_MAKE_NUM(DATADIR_M413, 0x0C);
        }
        object->mdlId[modelIndex + 11] = Hu3DModelCreateFile(modelDataId);
        Hu3DModelAttrSet(object->mdlId[modelIndex + 11], HU3D_ATTR_DISPOFF);
    }
    for (modelIndex = 0; modelIndex < 5; modelIndex++) {
        s32 linkedModelIndex;
        object->mdlId[(modelIndex * 10) + 13] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M413, 0x1E));
        for (linkedModelIndex = 0; linkedModelIndex < 10; linkedModelIndex++) {
            if (linkedModelIndex != 0) {
                object->mdlId[(13 + (modelIndex * 10) + linkedModelIndex)] = Hu3DModelLink(object->mdlId[(modelIndex * 10) + 13]);
            }
            Hu3DModelAttrSet(object->mdlId[(13 + (modelIndex * 10) + linkedModelIndex)], HU3D_ATTR_DISPOFF);
            Hu3DModelLayerSet(object->mdlId[(13 + (modelIndex * 10) + linkedModelIndex)], 2);
        }
    }
    {
        void *animationData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_M413, 0x1F), HU_MEMNUM_OVL, HEAP_MODEL);
        ANIMDATA *spriteAnimation = HuSprAnimRead(animationData);
        popupSpriteGroup = HuSprGrpCreate(150);
        for (modelIndex = 0; modelIndex < 150; modelIndex++) {
            s16 spriteId = HuSprCreate(spriteAnimation, 0, 0);
            HuSprGrpMemberSet(popupSpriteGroup, modelIndex, spriteId);
            HuSprAttrSet(popupSpriteGroup, modelIndex, HUSPR_ATTR_DISPOFF);
            HuSprColorSet(popupSpriteGroup, modelIndex, 255, 255, 255);
            HuSprTPLvlSet(popupSpriteGroup, modelIndex, 1.0f);
            HuSprPriSet(popupSpriteGroup, modelIndex, 256 + modelIndex);
        }
    }
    object->objFunc = UpdateGameplaySequence;
}

void InitializePlayerObject(OMOBJ *object)
{
    s32 orderedPlayerSlots[4] = { 0, 1, 2, 3 };
    s32 orderedPlayerCount = 0;
    PlayerWork *playerState = object->data;
    s32 rankIndex;
    s32 playerSlot;
    memset(playerState, 0, sizeof(PlayerWork));
    if (object->work[0] == 0) {
        playerOrderByRank[0] = playerOrderByRank[1] = playerOrderByRank[2] = playerOrderByRank[3] = -1;
        for (rankIndex = 0; rankIndex < 4; rankIndex++) {
            s32 randomPlayerSlot;
        repeatIter:
            randomPlayerSlot = rand8() % 4;
            if (playerOrderByRank[randomPlayerSlot] != -1) {
                goto repeatIter;
            }
            playerOrderByRank[randomPlayerSlot] = rankIndex;
        }
    }
    for (rankIndex = 0; rankIndex < 4; rankIndex++) {
        for (playerSlot = 0; playerSlot < 4; playerSlot++) {
            if (rankIndex == playerOrderByRank[playerSlot]) {
                orderedPlayerSlots[orderedPlayerCount] = playerSlot;
                orderedPlayerCount++;
            }
        }
    }
    playerState->cursorDirection = playerState->cursorRepeatFrames = playerState->buttonState = playerState->buttonRepeatFrames = 0;
    playerState->playerSlot = object->work[0];
    playerState->playerIndex = orderedPlayerSlots[playerState->playerSlot];
    if (playerState->playerIndex == 4) {
        OSReport("m413:wrong player group\n");
        playerState->playerIndex = playerState->playerSlot;
    }
    playerState->characterId = GWPlayerCfg[playerState->playerIndex].character;
    playerState->controllerIndex = (GWPlayerCfg[playerState->playerIndex].iscom) ? -1 : GWPlayerCfg[playerState->playerIndex].pad_idx;
    playerState->cpuDifficulty = (playerState->controllerIndex != -1) ? 0 : GWPlayerCfg[playerState->playerIndex].diff;
    playerState->rankedScore = -1;
    playerState->targetScore = 0;
    playerState->previousScore = playerState->targetScore;
    playerState->displayedScore = playerState->targetScore;
    playerState->scoreHoldFrames = 100;
    playerState->playerState = 2;
    playerState->selectionTimer = 0;
    ShufflePlayerCardLayout(playerState);
    playerState->cardColumn = 0;
    playerState->turnScore = ((rand8() % 4) * 7) + 3;
    playerState->turnScoreTarget = playerState->turnScore;
    playerState->inputDelay = 20;
    playerState->selectedLane = 0;
    playerState->cameraX = 0;
    playerState->cameraY = 0;
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M413, 0x02));
    Hu3DModelLayerSet(object->mdlId[0], 1);
    object->mdlId[1] = Hu3DModelLink(object->mdlId[0]);
    object->mdlId[2] = Hu3DModelLink(object->mdlId[0]);
    object->mdlId[3] = Hu3DModelLink(object->mdlId[0]);
    Hu3DModelPosSet(object->mdlId[1], (playerState->playerSlot * 300) - 525, -602, 40);
    Hu3DModelPosSet(object->mdlId[2], (playerState->playerSlot * 300) - 450, -602, 40);
    Hu3DModelPosSet(object->mdlId[3], (playerState->playerSlot * 300) - 375, -602, 40);
    Hu3DMotionTimeSet(object->mdlId[1], 60.0f);
    Hu3DMotionTimeSet(object->mdlId[2], 60.0f);
    Hu3DMotionTimeSet(object->mdlId[3], 60.0f);
    Hu3DMotionSpeedSet(object->mdlId[1], 0.0f);
    Hu3DMotionSpeedSet(object->mdlId[2], 0.0f);
    Hu3DMotionSpeedSet(object->mdlId[3], 0.0f);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DMotionTimeSet(object->mdlId[0], 60.0f);
    Hu3DMotionSpeedSet(object->mdlId[0], 0.0f);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M413, 0x0D) + playerState->characterId);
    Hu3DModelPosSet(object->mdlId[4], (playerState->playerSlot * 300) - 450, 670, 60);
    Hu3DMotionTimeSet(object->mdlId[4], 60.0f);
    Hu3DMotionSpeedSet(object->mdlId[4], 0.0f);
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M413, 0x15) + playerState->characterId);
    Hu3DModelPosSet(object->mdlId[5], (playerState->playerSlot * 300) - 450, 0, 0);
    Hu3DModelAttrSet(object->mdlId[5], HU3D_MOTATTR_LOOP);
    Hu3DMotionSpeedSet(object->mdlId[5], 0.0f);
    {
        ANIMDATA *spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGCONST, 0x38) + GWPlayerCfg[playerState->playerIndex].character);
        s16 playerSpriteId;
        playerState->playerSpriteGroup = HuSprGrpCreate(1);
        playerSpriteId = HuSprCreate(spriteAnimation, 0, 0);
        HuSprGrpMemberSet(playerState->playerSpriteGroup, 0, playerSpriteId);
        HuSprAttrSet(playerState->playerSpriteGroup, 0, HUSPR_ATTR_LOOP | HUSPR_ATTR_DISPOFF | HUSPR_ATTR_LINEAR);
        HuSprColorSet(playerState->playerSpriteGroup, 0, 255, 255, 255);
        HuSprTPLvlSet(playerState->playerSpriteGroup, 0, 1);
        HuSprPriSet(playerState->playerSpriteGroup, 0, 512);
        HuSprScaleSet(playerState->playerSpriteGroup, 0, 0.875f, 0.875f);
        HuSprSpeedSet(playerState->playerSpriteGroup, 0, 1.0f);
    }

    object->objFunc = EmptyPlayerObjectCallback;
}

void InitializeGameplaySequence(OMOBJ *object)
{
    object->work[0] = 1000;
    memset(pendingVoiceEvents, 0, sizeof(PendingVoiceEvent));
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    object->objFunc = UpdateBoardCamera;
}

void ObjectSetup(void)
{
    s32 i;
    OSReport("******* M413 ObjectSetup *********\n");
    HuAudSndGrpSet(38);
    objectManager = omInitObjMan(600, 8192);
    omGameSysInit(objectManager);
    Hu3DCameraViewportSet(1, 0, 0, 640, 480, 0, 1);
    Hu3DCameraPerspectiveSet(1, 40, 10, 20000, 1.2f);
    {
        Vec cameraPosition = { 616.5714f, -1331.2f, 2120.1142f };
        Vec cameraUp = { -0.05f, 1.0f, 0.0f };
        Vec cameraTarget = { 0, 0, 0 };
        Vec unusedCameraVector = { 0, 0, 0 };
        Vec cameraForward = { 0, 0, -1 };
        Hu3DCameraPosSetV(1, &cameraPosition, &cameraUp, &cameraTarget);
    }
    {
        Vec boardLightPosition = { -822.5f, 6743.0f, 4625.5f };
        Vec boardLightDirection = { 0.1545085f, -0.788462f, -0.5850064f };
        GXColor boardLightColor = { 0, 0, 0, 255 };
        boardLightId = Hu3DGLightCreateV(&boardLightPosition, &boardLightDirection, &boardLightColor);
    }
    Hu3DGLightStaticSet(boardLightId, 1);
    Hu3DGLightSpotSet(boardLightId, 20, 2);
    minigameSequenceId = -1;
    winningPlayer = -1;
    boardObject = omAddObjEx(objectManager, 101, 63, 0, 1, InitializeBoardObject);
    for (i = 0; i < 4; i++) {
        OMOBJ *object = playerObjects[i] = omAddObjEx(objectManager, 100, 6, 0, 0, InitializePlayerObject);
        object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerWork), HU_MEMNUM_OVL);
        object->work[0] = i;
    }
    mainSequenceObject = omAddObjEx(objectManager, 102, 0, 0, -1, InitializeGameplaySequence);
}

void UpdateCameraSequence(OMOBJ *object)
{
    if (!WipeStatGet()) {
        if (minigameSequenceId >= 0) {
            MGSeqKill(minigameSequenceId);
        }
        MGSeqKillAll();
        HuAudAllStop();
        omOvlReturnEx(1, 1);
    }
}

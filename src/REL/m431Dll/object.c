#include "game/audio.h"
#include "game/chrman.h"
#include "game/hu3d.h"
#include "game/gamework_data.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"

#include "REL/m431Dll.h"
#include "ext_math.h"
#include <string.h>
#include "version.h"

s32 rand8(void);

typedef struct ItemRingRotationState {
    u8 isActive;
    float rotationAngle;
} ItemRingRotationState;

ItemRingRotationState itemRotationStates[10];
OMOBJ *playerObjects[4];
PlayerGroupState playerGroups[4];
s32 collisionResponseTable[16][2];
u32 playerFinishFlags[4];
s8 activePlayerCount;

float itemStartingAngle = 18;
float playerItemRotationOffsets[8] = { 30.000002f, 0, 20.0f, 0, 20.0f, 50.0f, 20.0f, 30.000002f };

u32 playerMotions[8] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03), -1,
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x48), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x49), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x72) };

char *playerModelHookNames[8] = { "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r",
    "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r" };

void UpdatePlayerController(OMOBJ *object);
void InitializePlayerCharacter(OMOBJ *object);

void UpdatePlayerGroup(HUPROCESS *objman);

void InitializePlayerGroups(HUPROCESS *objman)
{
    Vec collisionTriangle[3];
    PlayerGroupState *playerGroup;
    s16 playerIndex;
    s16 completeGroupCount;
    ItemRingRotationState *itemRotationState;
    OMOBJ *playerObject;
    omMakeGroupEx(objman, 0, 4);
    omGetGroupMemberListEx(objman, 0);
    memset(playerGroups, 0, sizeof(playerGroups));
    for (playerIndex = 0, completeGroupCount = 0; playerIndex < 4; playerIndex++) {
        playerGroup = &playerGroups[GWPlayerCfg[playerIndex].group];
        if (GWPlayerCfg[playerIndex].group != 0 && GWPlayerCfg[playerIndex].group != 1) {
            break;
        }
        playerGroup->playerIds[playerGroup->playerCount++] = playerIndex;
        if (playerGroup->playerCount == 2) {
            completeGroupCount++;
        }
    }
    if (completeGroupCount != 2) {
        GWPlayerCfg[0].group = GWPlayerCfg[1].group = 0;
        GWPlayerCfg[2].group = GWPlayerCfg[3].group = 1;
        playerGroups[0].playerIds[0] = 0;
        playerGroups[0].playerIds[1] = 1;
        playerGroups[0].playerCount = 2;
        playerGroups[1].playerIds[0] = 2;
        playerGroups[1].playerIds[1] = 3;
        playerGroups[1].playerCount = 2;
    }
    for (playerIndex = 0, itemRotationState = &itemRotationStates[0]; playerIndex < 10; playerIndex++, itemRotationState++) {
        itemRotationState->isActive = 1;
        itemRotationState->rotationAngle = itemStartingAngle + (playerIndex * 36.0f);
    }
    playerGroups[0].startingPosition.x = -317;
    playerGroups[1].startingPosition.x = 317;
    playerGroups[0].startingPosition.y = playerGroups[1].startingPosition.y = 0;
    playerGroups[0].startingPosition.z = playerGroups[1].startingPosition.z = 100;
    playerGroups[0].itemRingRadius = playerGroups[1].itemRingRadius = 230;
    for (playerIndex = 0, playerGroup = &playerGroups[0]; playerIndex < 2; playerIndex++, playerGroup++) {
        collisionTriangle[0].x = playerGroup->startingPosition.x;
        collisionTriangle[0].y = playerGroup->startingPosition.y;
        collisionTriangle[0].z = playerGroup->startingPosition.z;
        for (completeGroupCount = 0; completeGroupCount < 10; completeGroupCount++) {
            collisionTriangle[1].x = collisionTriangle[2].x = (playerGroup->itemRingRadius * sind(itemRotationStates[completeGroupCount].rotationAngle)) + collisionTriangle[0].x;
            collisionTriangle[1].z = collisionTriangle[2].z = (playerGroup->itemRingRadius * cosd(itemRotationStates[completeGroupCount].rotationAngle)) + collisionTriangle[0].z;
            collisionTriangle[1].y = collisionTriangle[0].y;
            collisionTriangle[2].y = collisionTriangle[0].y - 50;
            CalculateCollisionPlane(collisionTriangle, &playerGroup->collisionBoundaries[completeGroupCount]);
            playerGroup->itemPositions[completeGroupCount][0] = collisionTriangle[1].x;
            playerGroup->itemPositions[completeGroupCount][1] = collisionTriangle[1].z;
        }
        playerGroup->completedRoundCounts[0] = 0;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        OSReport("PNo:%d Grp:%d\n", playerIndex, GWPlayerCfg[playerIndex].group);
        playerObject = omAddObjEx(objman, 99, 0, 0, -1, UpdatePlayerController);
        playerObjects[playerIndex] = omAddObjEx(objman, 100, 9, 8, 0, InitializePlayerCharacter);
        playerObjects[playerIndex]->work[0] = playerObject->work[0] = playerIndex;
    }
    UpdatePlayerGroup(objman);
}

s16 playerInputEnabled[4] = { 0, 0, 0, 0 };
s16 playerStatusValues[2] = { 0, 9 };

s16 CalculateItemPath(PlayerCharacterState *playerState, s16 startItemIndex, s16 targetItemIndex, s16 *directionOut, u8 avoidOpponent);
void UpdatePlayerMovement(OMOBJ *object);

s16 GetItemRingIndex(float ringAngle);
s16 GetItemRingDirection(s16 itemIndex);

s32 MovePlayerPairToTarget(s16 groupIndex)
{
    PlayerCharacterState *playerState;
    PlayerGroupState *playerGroup = &playerGroups[groupIndex];
    s16 playerPriorityOrder[4] = { 0, 9, 9, 0 };

    s16 playerMoveResults[2];
    s16 playerTargetAngles[2];
    s16 playerDirections[2];
    s16 playerDirection;
    s16 playerIndex;
    s32 playerIsBlocked;
    if (activePlayerCount != 0) {
        (void)activePlayerCount;
    }
    else {
        for (playerIndex = 0, playerIsBlocked = 0; playerIndex < 2; playerIndex++) {
            playerState = playerObjects[playerGroup->playerIds[playerIndex]]->data;
            if ((playerState->stateFlags & 0xC0) || (playerState->stateFlags & 0x600)) {
                playerIsBlocked = 1;
            }
            UpdatePlayerMovement(playerObjects[playerGroup->playerIds[playerIndex]]);
            playerTargetAngles[playerIndex] = playerState->itemIndex = GetItemRingIndex(playerState->ringMovementState.y);
            playerDirections[playerIndex] = GetItemRingDirection(playerState->itemIndex);
        }
        if (playerIsBlocked != 0) {
            return 0;
        }
        if (playerDirections[0] != playerDirections[1]) {
            if (playerDirections[0] == 0) {
                playerState = playerObjects[playerGroup->playerIds[0]]->data;
                playerState->groupTargetItemIndex = playerStatusValues[0];
                playerState->opponent->groupTargetItemIndex = playerStatusValues[1];
            }
            else {
                playerState = playerObjects[playerGroup->playerIds[1]]->data;
                playerState->groupTargetItemIndex = playerStatusValues[0];
                playerState->opponent->groupTargetItemIndex = playerStatusValues[1];
            }
        }
        else {
            if (playerDirections[0] == 0) {
                if (playerTargetAngles[0] < playerTargetAngles[1]) {
                    playerState = playerObjects[playerGroup->playerIds[0]]->data;
                    playerState->groupTargetItemIndex = playerStatusValues[1];
                    playerState->opponent->groupTargetItemIndex = playerStatusValues[0];
                }
                else {
                    playerState = playerObjects[playerGroup->playerIds[1]]->data;
                    playerState->groupTargetItemIndex = playerStatusValues[1];
                    playerState->opponent->groupTargetItemIndex = playerStatusValues[0];
                }
            }
            else {
                if (playerTargetAngles[0] > playerTargetAngles[1]) {
                    playerState = playerObjects[playerGroup->playerIds[0]]->data;
                    playerState->groupTargetItemIndex = playerStatusValues[0];
                    playerState->opponent->groupTargetItemIndex = playerStatusValues[1];
                }
                else {
                    playerState = playerObjects[playerGroup->playerIds[1]]->data;
                    playerState->groupTargetItemIndex = playerStatusValues[0];
                    playerState->opponent->groupTargetItemIndex = playerStatusValues[1];
                }
            }
        }
        activePlayerCount++;
    }
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        playerState = playerObjects[playerGroup->playerIds[playerIndex]]->data;
        playerState->isHuman = 0;
        playerState->hasMovementInput = 1;
        playerState->stickX = 0;
        playerState->stickY = 0;
        playerMoveResults[playerIndex] = CalculateItemPath(playerState, playerState->itemIndex, playerState->groupTargetItemIndex, &playerDirection, 0);
        if (playerMoveResults[playerIndex] != 0) {
            playerState->stickX = (playerDirection == 0) ? 50 : -50;
        }
        else {
            playerState->hasMovementInput = 0;
        }
        UpdatePlayerMovement(playerObjects[playerGroup->playerIds[playerIndex]]);
        playerMoveResults[playerIndex] = (playerState->stateFlags & 0xC0) ? 1 : 0;
        if (playerState->stateFlags & 0x600) {
            playerMoveResults[playerIndex] = 1;
        }
    }
    if (playerMoveResults[0] == 0 && playerMoveResults[1] == 0) {
        return 1;
    }
    return 0;
}

s32 CheckPlayerGroupReady(s16 groupIndex)
{
    s32 playerBlockedFlags[2];
    s32 playerIndex;
    PlayerCharacterState *playerState;
    PlayerGroupState *playerGroup;
    playerGroup = &playerGroups[groupIndex];
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        playerState = playerObjects[playerGroup->playerIds[playerIndex]]->data;
        UpdatePlayerMovement(playerObjects[playerGroup->playerIds[playerIndex]]);
        if ((playerState->stateFlags & 0xC0) || (playerState->stateFlags & 0x600) || (playerState->stateFlags & 0x20)) {
            playerBlockedFlags[playerIndex] = 1;
        }
        else {
            playerBlockedFlags[playerIndex] = 0;
        }
    }
    if (playerBlockedFlags[0] == 0 && playerBlockedFlags[1] == 0) {
        return 1;
    }
    return 0;
}

void UpdateComputerPlayerController(OMOBJ *object);
PlayerCharacterState *GetOpposingPlayerState(OMOBJ *object);
u32 GetPlayerStateFlags(OMOBJ *object, u32 stateMask);

void UpdatePlayerController(OMOBJ *object)
{
    PlayerCharacterState *playerState;
    u16 gameState;
    if (playerInputEnabled[object->work[0]] == 0) {
        return;
    }
    playerState = playerObjects[object->work[0]]->data;
    playerState->buttonsHeld = HuPadBtn[playerState->controllerIndex];
    playerState->buttonsPressed = HuPadBtnDown[playerState->controllerIndex];
    playerState->stickX = HuPadStkX[playerState->controllerIndex];
    playerState->stickY = HuPadStkY[playerState->controllerIndex];
    if (getSequenceStatusFlags(1) != 0) {
        return;
    }
    gameState = getSequenceStatusFlags(28);
    if (gameState == 12 || gameState == 16) {
        if (GetPlayerStateFlags(playerObjects[object->work[0]], 0x600) == 0 && playerInputEnabled[object->work[0]] != 0) {
            playerState = playerObjects[object->work[0]]->data;
            if (playerState->opponent == 0) {
                playerState->opponent = GetOpposingPlayerState(playerObjects[object->work[0]]);
            }
            if (GWPlayerCfg[playerState->playerIndex].iscom) {
                UpdateComputerPlayerController(playerObjects[object->work[0]]);
            }
            UpdatePlayerMovement(playerObjects[object->work[0]]);
        }
    }
}

void UpdateComputerPlayerController(OMOBJ *object)
{
    PlayerCharacterState *playerState;
    s16 candidateIndex;
    s16 bestCandidateIndex;
    s16 unavailableItemCount;
    u32 itemFlags;
    s16 candidateScores[9];
    s16 candidateDirections[8];
    playerState = object->data;
    playerState->stickX = playerState->stickY = 0;
    playerState->buttonsPressed = 0;
    if (getSequenceStatusFlags(224)) {
        return;
    }
    candidateScores[8] = 999;
    for (candidateIndex = 0, unavailableItemCount = 0, bestCandidateIndex = 8; candidateIndex < playerState->availableItemCount; candidateIndex++) {
        itemFlags = getStoryItemFlags(playerState->groupIndex, playerState->availableItemIndices[candidateIndex]);
        if (itemFlags & 0x1) {
            playerState->availableItemIndices[candidateIndex] = -1;
        }
        candidateScores[candidateIndex] = CalculateItemPath(playerState, playerState->itemIndex, playerState->availableItemIndices[candidateIndex], &candidateDirections[candidateIndex], 1);
        if (candidateScores[candidateIndex] < candidateScores[bestCandidateIndex]) {
            bestCandidateIndex = candidateIndex;
        }
        if (playerState->availableItemIndices[candidateIndex] == -1) {
            unavailableItemCount++;
        }
    }
    if (unavailableItemCount == playerState->availableItemCount) {
        return;
    }
    if (playerState->selectedItemCount < playerState->computerChoiceBias && bestCandidateIndex < 4) {
        for (candidateIndex = 4; candidateIndex < playerState->availableItemCount; candidateIndex++) {
            if (candidateScores[candidateIndex] < 10) {
                bestCandidateIndex = candidateIndex;
                break;
            }
        }
    }
    if (playerState->computerInputDelay == 0) {
        if (GWPlayerCfg[playerState->opponent->playerIndex].iscom) {
            if (playerState->playerIndex > playerState->opponent->playerIndex && (playerState->stateFlags & 0x20)) {
                playerState->computerInputDelay = 1;
            }
        }
        else {
            if (playerState->stateFlags & 0x20) {
                playerState->computerInputDelay = 1;
            }
        }
    }
    if (playerState->computerInputDelay && unavailableItemCount == playerState->availableItemCount - 1) {
        if (++playerState->computerInputDelay > 2 * REFRESH_RATE) {
            playerState->computerInputDelay = 0;
        }
        if ((playerState->opponent->stateFlags & 0xC0) == 0x40) {
            candidateDirections[bestCandidateIndex] = 0;
        }
        else {
            if ((playerState->opponent->stateFlags & 0xC0) == 0x80) {
                candidateDirections[bestCandidateIndex] = 1;
            }
            else {
                playerState->computerInputDelay = 0;
            }
        }
    }
    if (candidateScores[bestCandidateIndex] != -1) {
        playerState->stickX = (candidateDirections[bestCandidateIndex] == 0) ? 50 : -50;
    }
    else {
        if (playerState->ringMovementState.z <= 15.0f) {
            playerState->buttonsPressed = PAD_BUTTON_A;
        }
    }
}

s16 CalculateItemPath(PlayerCharacterState *playerState, s16 startItemIndex, s16 targetItemIndex, s16 *directionOut, u8 avoidOpponent)
{
    s16 candidateIndex;
    s16 pathDistance;
    s16 clockwiseSteps;
    s16 counterclockwiseSteps;
    s16 clockwiseBlocked;
    s16 counterclockwiseBlocked;

    if (targetItemIndex == -1) {
        *directionOut = rand16() % 2;
        pathDistance = 99;
    }
    else if (startItemIndex == targetItemIndex) {
        *directionOut = -1;
        pathDistance = (avoidOpponent) ? -1 : 0;
    }
    else {
        clockwiseSteps = counterclockwiseSteps = clockwiseBlocked = counterclockwiseBlocked = 0;
        candidateIndex = startItemIndex;
        do {
            candidateIndex++;
            clockwiseSteps++;
            if (candidateIndex >= 10) {
                candidateIndex = 0;
            }
            if (avoidOpponent && candidateIndex == playerState->opponent->itemIndex) {
                clockwiseBlocked = 99;
                break;
            }
        } while (candidateIndex != targetItemIndex);
        candidateIndex = startItemIndex;
        do {
            candidateIndex--;
            counterclockwiseSteps++;
            if (candidateIndex < 0) {
                candidateIndex = 9;
            }
            if (avoidOpponent && candidateIndex == playerState->opponent->itemIndex) {
                counterclockwiseBlocked = 99;
                break;
            }
        } while (candidateIndex != targetItemIndex);
        if (clockwiseBlocked != counterclockwiseBlocked) {
            clockwiseSteps += clockwiseBlocked;
            counterclockwiseSteps += counterclockwiseBlocked;
            if (clockwiseSteps < counterclockwiseSteps) {
                *directionOut = 0;
                pathDistance = clockwiseSteps;
            }
            else {
                *directionOut = 1;
                pathDistance = counterclockwiseSteps;
            }
        }
        else {
            if (clockwiseSteps < counterclockwiseSteps) {
                *directionOut = 0;
                pathDistance = clockwiseSteps;
            }
            else {
                *directionOut = 1;
                pathDistance = counterclockwiseSteps;
            }
            pathDistance += 10;
        }
    }
    return pathDistance;
}

s16 GetRelativePlayerSide(OMOBJ *object);
void UpdatePlayerStateFlags(OMOBJ *object, u32 clearMask, u32 setMask);

void UpdatePlayerMovement(OMOBJ *object)
{
    float playerDeltaX;
    float playerDeltaZ;
    float distanceToOpponent;

    PlayerCharacterState *playerState;
    s16 moveRight;
    s16 moveLeft;
    s16 stickX;
    s16 targetPlayerDelta;
    s16 stickY;

    Vec targetPosition;
    u16 gameState;
    PlayerCollisionBoundary *playerBoundary;
    playerState = object->data;
    if (playerState->motionIndex == 3 || playerState->motionIndex == 5) {
        return;
    }
    gameState = getSequenceStatusFlags(28);
    if ((playerState->stateFlags & 0x20) || (getSequenceStatusFlags(28) != 12 && playerState->hasMovementInput == 0)) {
        stickX = stickY = 0;
    }
    else {
        stickX = playerState->stickX;
        stickY = playerState->stickY;
    }
    moveRight = moveLeft = 0;
    if (playerState->stickInputState == 0) {
        switch (playerState->isHuman) {
            case 0:
                if (stickX > 10) {
                    moveRight = 1;
                }
                else if (stickX < -10) {
                    moveLeft = 1;
                }
                break;

            case 1:
                if (ABS(stickX) >= 10 || ABS(stickY) >= 10) {
                    playerBoundary = &playerGroups[playerState->groupIndex].collisionBoundaries[playerState->itemIndex];
                    targetPosition.x = stickX + playerGroups[playerState->groupIndex].itemPositions[playerState->itemIndex][0];
                    targetPosition.z = playerGroups[playerState->groupIndex].itemPositions[playerState->itemIndex][1] - stickY;
                    distanceToOpponent = (playerBoundary->planeNormal.x * targetPosition.x) + (playerBoundary->planeNormal.z * targetPosition.z) - playerBoundary->planeDistance;
                    if (distanceToOpponent > 0.0f) {
                        if (playerState->previousItemIndex != playerState->itemIndex) {
                            if (playerState->directionInputFlags == 128) {
                                playerState->stickInputState = 1;
                            }
                        }
                        else {
                            moveRight = 1;
                        }
                    }
                    else {
                        if (distanceToOpponent < 0.0f) {
                            if (playerState->previousItemIndex != playerState->itemIndex) {
                                if (playerState->directionInputFlags == 64) {
                                    playerState->stickInputState = 1;
                                }
                            }
                            else {
                                moveLeft = 1;
                            }
                        }
                    }
                }
                break;
        }
    }
    else {
        if (playerState->previousStickX != stickX || playerState->previousStickY != stickY) {
            playerState->stickInputState = 0;
        }
    }
    playerDeltaX = playerState->position.x - playerState->opponent->position.x;
    playerDeltaZ = playerState->position.z - playerState->opponent->position.z;
    distanceToOpponent = VECMagPoint(playerDeltaX, 0, playerDeltaZ);
    if (distanceToOpponent < 90 && (playerState->stateFlags & 0x20) == 0) {
        if (GetRelativePlayerSide(object) == 0) {
            moveRight = 0;
            moveLeft = 1;
        }
        else {
            moveRight = 1;
            moveLeft = 0;
        }
        playerState->stateFlags |= 0x20;
    }
    else {
        if (playerState->directionInputFlags == 64) {
            targetPlayerDelta = playerState->targetItemIndex - playerState->itemIndex;
            if (targetPlayerDelta < 0) {
                targetPlayerDelta += 10;
            }
            if (targetPlayerDelta > 1) {
                playerState->targetItemIndex = 1;
            }
        }
        else {
            targetPlayerDelta = playerState->itemIndex - playerState->targetItemIndex;
            if (targetPlayerDelta < 0) {
                targetPlayerDelta += 10;
            }
            if (targetPlayerDelta > 1) {
                playerState->targetItemIndex = 9;
            }
        }
    }
    if (moveRight) {
        moveRight = 0;
        if (playerState->directionInputFlags == 64) {
            if (playerState->ringMovementState.z <= 2.0f) {
                moveRight = 1;
            }
        }
        else if (playerState->directionInputFlags == 128) {
            if (playerState->itemIndex != playerState->targetItemIndex) {
                playerState->itemIndex = playerState->targetItemIndex;
            }
            moveRight = 1;
        }
        else {
            moveRight = 1;
        }
        if (moveRight) {
            UpdatePlayerStateFlags(object, 192, 64);
            playerState->targetItemIndex = playerState->itemIndex + 1;
            if (playerState->targetItemIndex >= 10) {
                playerState->targetItemIndex = 0;
            }
        }
    }
    else if (moveLeft) {
        moveLeft = 0;
        if (playerState->directionInputFlags == 128) {
            if (playerState->ringMovementState.z <= 2.0f) {
                moveLeft = 1;
            }
        }
        else if (playerState->directionInputFlags == 64) {
            if (playerState->itemIndex != playerState->targetItemIndex) {
                playerState->itemIndex = playerState->targetItemIndex;
            }
            moveLeft = 1;
        }
        else {
            moveLeft = 1;
        }
        if (moveLeft) {
            UpdatePlayerStateFlags(object, 192, 128);
            playerState->targetItemIndex = playerState->itemIndex - 1;
            if (playerState->targetItemIndex < 0) {
                playerState->targetItemIndex = 9;
            }
        }
    }
    playerState->previousItemIndex = playerState->itemIndex;
    playerState->directionInputFlags = GetPlayerStateFlags(object, 192);
    playerState->previousStickX = stickX;
    playerState->previousStickY = stickY;
}

s16 GetRelativePlayerSide(OMOBJ *object)
{
    PlayerCharacterState *playerState;
    float relativeHeading;
    playerState = object->data;
    relativeHeading = playerState->ringMovementState.y - playerState->opponent->ringMovementState.y;
    if (relativeHeading > 180.0f) {
        relativeHeading -= 360.0f;
    }
    else if (relativeHeading < -180.0f) {
        relativeHeading = 360.0f + relativeHeading;
    }
    return (relativeHeading <= 0.0f) ? 0 : 1;
}

void UpdatePlayerCharacter(OMOBJ *object);

void SetPlayerMotion(OMOBJ *object, s16 motionIndex, u32 motionFlags);

void InitializePlayerCharacter(OMOBJ *object)
{
    PlayerCharacterState *playerState;
    PlayerGroupState *playerGroup;
    s16 motionIndex;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerCharacterState), HU_MEMNUM_OVL);
    playerState = object->data;
    memset(playerState, 0, sizeof(PlayerCharacterState));
    object->stat |= 0x100;
    playerState->playerIndex = object->work[0];
    playerState->controllerIndex = GWPlayerCfg[playerState->playerIndex].pad_idx;
    playerState->characterId = GWPlayerCfg[playerState->playerIndex].character;
    playerState->groupIndex = GWPlayerCfg[playerState->playerIndex].group;
    playerState->isHuman = (GWPlayerCfg[playerState->playerIndex].iscom) ? 0 : 1;
    playerState->alternateResultMotion = 0;
    object->mdlId[0] = CharModelCreate(playerState->characterId, 2);
    Hu3DModelLayerSet(object->mdlId[0], 0);
    Hu3DModelShadowSet(object->mdlId[0]);
    CharModelStepFxSet(playerState->characterId, 0);
    for (motionIndex = 0; motionIndex < 8; motionIndex++) {
        if (playerMotions[motionIndex] != -1) {
            object->mtnId[motionIndex] = CharMotionCreate(playerState->characterId, playerMotions[motionIndex]);
        }
        else {
            object->mtnId[motionIndex] = Hu3DJointMotionFile(object->mdlId[0], DATA_MAKE_NUM(DATADIR_M431, 0x21) + playerState->characterId);
        }
        if (motionIndex == 4 || motionIndex == 5) {
            CharMotionVoiceOnSet(playerState->characterId, object->mtnId[motionIndex], 0);
        }
    }
    CharMotionDataClose(playerState->characterId);
    playerGroup = &playerGroups[playerState->groupIndex];
    for (motionIndex = 0; motionIndex < 8; motionIndex++) {
        playerState->availableItemIndices[motionIndex] = -1;
    }
    if (playerState->groupIndex == 0) {
        if (playerGroup->playerIds[0] == object->work[0]) {
            playerState->ringMovementState.y = itemRotationStates[9].rotationAngle;
        }
        else {
            playerState->ringMovementState.y = itemRotationStates[4].rotationAngle;
        }
    }
    else {
        if (playerGroup->playerIds[0] == object->work[0]) {
            playerState->ringMovementState.y = itemRotationStates[5].rotationAngle;
        }
        else {
            playerState->ringMovementState.y = itemRotationStates[0].rotationAngle;
        }
    }
    playerState->itemIndex = (playerState->ringMovementState.y / 360.0f) * 10.0f;
    playerState->targetItemIndex = playerState->itemIndex;
    playerState->modelRadiusOffset = playerItemRotationOffsets[playerState->characterId];
    playerState->position.x = ((playerGroup->itemRingRadius + playerState->modelRadiusOffset) * sind(playerState->ringMovementState.y)) + playerGroup->startingPosition.x;
    playerState->position.y = playerGroup->startingPosition.y;
    playerState->position.z = ((playerGroup->itemRingRadius + playerState->modelRadiusOffset) * cosd(playerState->ringMovementState.y)) + playerGroup->startingPosition.z;
    playerState->facingAngle = atan2d(playerGroup->startingPosition.x - playerState->position.x, playerGroup->startingPosition.z - playerState->position.z);
    Hu3DModelPosSet(object->mdlId[0], playerState->position.x, playerState->position.y, playerState->position.z);
    Hu3DModelRotSet(object->mdlId[0], 0, playerState->facingAngle, 0);
    playerState->motionIndex = playerState->queuedMotionIndex = -1;
    SetPlayerMotion(object, 0, 10);
    playerInputEnabled[object->work[0]] = 1;
    object->objFunc = UpdatePlayerCharacter;
}

void HandlePlayerItemSelection(OMOBJ *object);
void UpdatePlayerRingMovement(OMOBJ *object);
void UpdatePlayerFacing(OMOBJ *object);
void UpdatePlayerResultAnimation(OMOBJ *object);

void CleanupPlayerItemEffect(OMOBJ *object);

void SetPlayerMotionTimer(OMOBJ *object, s16 motionIndex, u32 motionFlags);
void SetPlayerObjectCallback(OMOBJ *object, OMOBJFUNC callback);

void DestroyPlayerCharacter(OMOBJ *object);

s32 PlayPlayerSoundEffect(s32 soundId, s32 volume);

void UpdatePlayerCharacter(OMOBJ *object)
{
    Vec modelPositionDelta;

    PlayerCharacterState *playerState;
    HU3DMODEL *playerModel;
    playerState = object->data;
    playerModel = &Hu3DData[object->mdlId[0]];
    modelPositionDelta.x = playerState->position.x - playerModel->pos.x;
    modelPositionDelta.z = playerState->position.z - playerModel->pos.z;
    if ((playerState->stateFlags & 0x600) == 0 || 0.0f != modelPositionDelta.x || 0.0f != modelPositionDelta.z || (playerState->stateFlags & 0x20)) {
        HandlePlayerItemSelection(object);
        UpdatePlayerRingMovement(object);
        UpdatePlayerFacing(object);
    }
    else {
        UpdatePlayerResultAnimation(object);
    }
    DestroyPlayerCharacter(object);
    Hu3DModelPosSet(object->mdlId[0], playerState->position.x, playerState->position.y, playerState->position.z);
    Hu3DModelRotSet(object->mdlId[0], 0, playerState->facingAngle, 0);
}

#if VERSION_NTSC
#define handlePlayerItemSelectionCHECK (playerModel->motIdShift == -1 && playerModel->motWork.time == 14.0f)
#else
#define handlePlayerItemSelectionCHECK (playerModel->motIdShift == -1 && playerModel->motWork.time >= 14.0f && playerState->palMotionTransitionFlag == 0)
#endif

void HandlePlayerItemSelection(OMOBJ *object)
{
    PlayerCharacterState *playerState;
    HU3DMODEL *playerModel;

    playerModel = &Hu3DData[object->mdlId[0]];
    playerState = object->data;
    if (getSequenceStatusFlags(28) != 12 || getSequenceStatusFlags(224) != 0) {
        return;
    }
    if (getStoryItemAnimationPhase() == 2) {
        return;
    }
    if (GetPlayerStateFlags(object, 192) != 0) {
        return;
    }
    if (playerState->motionIndex == 0) {
        if (playerModel->motIdShift == -1 && playerState->buttonsPressed == PAD_BUTTON_A) {
            if (isStoryItemAvailable(playerState->groupIndex, playerState->itemIndex)) {
                SetPlayerMotion(object, 3, 4);
                SetPlayerMotionTimer(object, 0, 8);
#if VERSION_PAL
                playerState->palMotionTransitionFlag = 0;
#endif
            }
        }
    }
    else {
        if (playerState->motionIndex == 3) {
            if (handlePlayerItemSelectionCHECK) {
#if VERSION_PAL
                playerState->palMotionTransitionFlag = 1;
#endif
                playerState->capturedItemModelId = selectStoryItem(playerState->groupIndex, playerState->itemIndex, &playerState->opponentPriorityFlag);
                if (playerState->capturedItemModelId != -1) {
                    if (playerState->groupIndex == 0) {
                        HuAudFXPlay(1668);
                    }
                    else {
                        HuAudFXPlay(1673);
                    }
                    if (playerState->opponentPriorityFlag != 0) {
                        omVibrate(playerState->playerIndex, 12, 6, 6);
                        if (playerState->groupIndex == 0) {
                            PlayPlayerSoundEffect(1666, 0.5f * REFRESH_RATE);
                        }
                        else {
                            PlayPlayerSoundEffect(1671, 0.5f * REFRESH_RATE);
                        }
                    }
                    else {
                        if (playerState->groupIndex == 0) {
                            PlayPlayerSoundEffect(1667, 0.5f * REFRESH_RATE);
                        }
                        else {
                            PlayPlayerSoundEffect(1672, 0.5f * REFRESH_RATE);
                        }
                    }
                    playerState->selectedItemCount++;
                    Hu3DModelPosSet(playerState->capturedItemModelId, 0, 0, 0);
                    Hu3DModelHookSet(object->mdlId[0], playerModelHookNames[playerState->characterId], playerState->capturedItemModelId);
                    SetPlayerObjectCallback(object, CleanupPlayerItemEffect);
                }
            }
        }
    }
}

void UpdatePlayerRingMovement(OMOBJ *object)
{
    float targetAngle;
    PlayerCharacterState *playerState;
    PlayerGroupState *playerGroup;
    s8 movementComplete;
    u32 playerStep;
    playerState = object->data;
    playerGroup = &playerGroups[playerState->groupIndex];
    movementComplete = 0;
    if ((playerStep = GetPlayerStateFlags(object, 192)) == 0) {
        return;
    }
    if (playerStep == 192) {
        movementComplete = 1;
    }
    else if (playerStep == 64) {
        playerState->ringMovementState.x += 0.3f;
        if (playerState->ringMovementState.x > 1.5f) {
            playerState->ringMovementState.x = 1.5f;
        }
        playerState->ringMovementState.y += playerState->ringMovementState.x;
        targetAngle = itemRotationStates[playerState->targetItemIndex].rotationAngle;
        if (playerState->itemIndex == 9 && playerState->targetItemIndex == 0) {
            targetAngle += 360.0f;
        }
        else {
            if (playerState->ringMovementState.y >= 360.0f) {
                playerState->ringMovementState.y -= 360.0f;
            }
        }
        playerState->ringMovementState.z = ABS(playerState->ringMovementState.y - targetAngle);
        if (playerState->ringMovementState.y >= targetAngle) {
            movementComplete = 1;
        }
    }
    else {
        playerState->ringMovementState.x -= 0.3f;
        if (playerState->ringMovementState.x < -1.5f) {
            playerState->ringMovementState.x = -1.5f;
        }
        playerState->ringMovementState.y += playerState->ringMovementState.x;
        targetAngle = itemRotationStates[playerState->targetItemIndex].rotationAngle;
        if (playerState->targetItemIndex == 9 && playerState->itemIndex == 0) {
            if (playerState->ringMovementState.y < 0.0f) {
                playerState->ringMovementState.y += 360.0f;
            }
            else {
                targetAngle -= 360.0f;
            }
        }
        else {
            if (playerState->targetItemIndex == 0 || playerState->itemIndex == 0) {
                if (playerState->ringMovementState.y < 0.0f) {
                    playerState->ringMovementState.y += 360.0f;
                    targetAngle += 360.0f;
                }
            }
            else {
                if (playerState->ringMovementState.y < 0.0f) {
                    playerState->ringMovementState.y += 360.0f;
                }
            }
        }
        playerState->ringMovementState.z = ABS(playerState->ringMovementState.y - targetAngle);
        if (playerState->ringMovementState.y <= targetAngle) {
            movementComplete = 1;
        }
    }
    if (movementComplete != 0) {
        playerState->ringMovementState.x = 0;
        playerState->ringMovementState.y = itemRotationStates[playerState->targetItemIndex].rotationAngle;
        playerState->stateFlags &= ~0xE0;
    }
    playerState->itemIndex = (playerState->ringMovementState.y / 360.0f) * 10.0f;
    if (playerState->itemIndex >= 10) {
        playerState->itemIndex -= 10;
    }
    playerState->position.x = ((playerGroup->itemRingRadius + playerState->modelRadiusOffset) * sind(playerState->ringMovementState.y)) + playerGroup->startingPosition.x;
    playerState->position.y = playerGroup->startingPosition.y;
    playerState->position.z = ((playerGroup->itemRingRadius + playerState->modelRadiusOffset) * cosd(playerState->ringMovementState.y)) + playerGroup->startingPosition.z;
    SetPlayerMotion(object, 2, 8);
}

void UpdatePlayerFacing(OMOBJ *object)
{
    Vec playerPositionDelta;
    PlayerCharacterState *playerState;
    HU3DMODEL *playerModel;
    PlayerGroupState *playerGroup;
    s32 startTurn;
    playerModel = &Hu3DData[object->mdlId[0]];
    playerState = object->data;
    playerGroup = &playerGroups[playerState->groupIndex];
    startTurn = 0;
    if (playerState->motionIndex == 3 || playerState->motionIndex == 5) {
        return;
    }
    playerPositionDelta.x = playerState->position.x - playerModel->pos.x;
    playerPositionDelta.z = playerState->position.z - playerModel->pos.z;
    if (0.0f == playerPositionDelta.x && 0.0f == playerPositionDelta.z) {
        if (getSequenceStatusFlags(224) == 0 && playerState->motionIndex == 2 && (playerState->buttonsHeld & PAD_BUTTON_A)) {
            playerState->actionButtonState = 1;
        }
        if (playerState->facingTransitionActive == 0) {
            playerPositionDelta.x = playerGroup->startingPosition.x - playerState->position.x;
            playerPositionDelta.z = playerGroup->startingPosition.z - playerState->position.z;
            playerState->facingTransition.x = playerState->facingAngle;
            playerState->facingTransition.y = atan2d(playerPositionDelta.x, playerPositionDelta.z);
            playerState->facingTransition.z = 0;
            if (playerState->facingTransition.y < 0.0f) {
                playerState->facingTransition.y += 360.0f;
            }
            playerState->facingTransitionActive = 1;
        }
        playerState->facingTransition.z += 0.1f;
        if (playerState->facingTransition.z >= 1.0f) {
            playerState->facingTransition.z = 1.0f;
            if (playerState->actionButtonState == 0) {
                SetPlayerMotion(object, 0, 8);
            }
            else {
                if (isStoryItemAvailable(playerState->groupIndex, playerState->itemIndex) && getSequenceStatusFlags(28) == 12 && getSequenceStatusFlags(224) == 0 && getStoryItemAnimationPhase() < 2) {
                    SetPlayerMotion(object, 3, 4);
                    SetPlayerMotionTimer(object, 0, 8);
#if VERSION_PAL
                    playerState->palMotionTransitionFlag = 0;
#endif
                    startTurn = 1;
                }
                if (startTurn == 0) {
                    SetPlayerMotion(object, 0, 8);
                }
            }
            playerState->actionButtonState = 0;
        }
        playerState->facingAngle = interpolateAngle(playerState->facingTransition.x, playerState->facingTransition.y, playerState->facingTransition.z);
    }
    else {
        playerState->facingTransitionActive = playerState->actionButtonState = 0;
        playerState->facingAngle = atan2d(playerPositionDelta.x, playerPositionDelta.z);
        if (playerState->facingAngle < 0.0f) {
            playerState->facingAngle += 360.0f;
        }
    }
    playerFinishFlags[object->work[0]] = playerState->stateFlags & 0xC0;
}

void UpdatePlayerResultAnimation(OMOBJ *object)
{
    float targetYaw;
    PlayerCharacterState *playerState;
    s32 gameMode;
    HU3DMODEL *playerModel;
    playerModel = &Hu3DData[object->mdlId[0]];
    playerState = object->data;
    if (playerState->motionIndex == 3) {
        return;
    }
    playerState->actionButtonState = 0;
    switch (playerState->resultAnimationState) {
        case 0:
            playerState->resultFacingTransition.y += 0.05f;
            if (playerState->resultFacingTransition.y > 1.0f) {
                playerState->resultFacingTransition.y = 1.0f;
                playerState->resultAnimationState++;
            }
            gameMode = getSequenceStatusFlags(28);
            if (gameMode >= 16 && playerWon) {
                targetYaw = CRot.y;
            }
            else {
                targetYaw = 0.0f;
            }
            if (targetYaw < 0.0f) {
                targetYaw += 360.0f;
            }
            else if (targetYaw >= 360.0f) {
                targetYaw -= 360.0f;
            }
            playerState->facingAngle = interpolateAngle(playerState->resultFacingTransition.x, targetYaw, playerState->resultFacingTransition.y);
            SetPlayerMotion(object, 2, 8);
            break;

        case 1:
            playerState->queuedMotionIndex = -1;
            gameMode = getSequenceStatusFlags(28);
            if (gameMode >= 16 && playerWon) {
                if (playerState->alternateResultMotion == 0) {
                    SetPlayerMotion(object, 6, 4);
                }
                else {
                    SetPlayerMotion(object, 7, 4);
                }
                playerState->resultAnimationState = 99;
            }
            else {
                SetPlayerMotion(object, 4, 4);
                CharFXPlay(playerState->characterId, 290);
                playerState->resultAnimationState++;
            }
            break;

        case 2:
            if (playerModel->motIdShift == -1) {
                if (Hu3DMotionTimeGet(object->mdlId[0]) >= Hu3DMotionMaxTimeGet(object->mdlId[0]) - 1.0f) {
                    SetPlayerMotion(object, 1, 8);
                    playerState->resultFacingTransition.y = 0;
                    playerState->resultAnimationState++;
                }
            }
            break;

        case 3:
            playerState->resultFacingTransition.y += 0.05f;
            if (playerState->resultFacingTransition.y > 1.0f) {
                if (getSequenceStatusFlags(1)) {
                    UpdateSequenceStatusFlags(2, 2);
                }
                SetPlayerMotion(object, 0, 8);
                UpdatePlayerStateFlags(object, 0x600, 0);
                playerState->resultFacingTransition.y = 1.0f;
                playerState->resultAnimationState++;
            }
            if (!playerState->alternateResultMotion) {
                playerState->facingAngle = interpolateAngle(0, playerState->resultFacingTransition.x, playerState->resultFacingTransition.y);
            }
            break;
    }
}

void StartPlayerGroupVictory(s16 groupIndex)
{
    PlayerCharacterState *playerState;
    PlayerGroupState *playerGroup;
    s16 groupIndex;
    s16 playerIndex;
    playerGroup = &playerGroups[groupIndex];
    if (groupIndex == -1) {
        return;
    }
    for (groupIndex = 0; groupIndex < 1; groupIndex++) {
        if (groupIndex == 0) {
            playerGroup = &playerGroups[groupIndex];
        }
        else {
            playerGroup = &playerGroups[groupIndex ^ 1];
        }
        for (playerIndex = 0; playerIndex < playerGroup->playerCount; playerIndex++) {
            playerState = playerObjects[playerGroup->playerIds[playerIndex]]->data;
            playerState->resultAnimationState = 0;
            playerState->resultFacingTransition.x = playerState->facingAngle;
            playerState->resultFacingTransition.y = 0;
            playerState->stateFlags &= ~0x600;

            if (groupIndex == 0) {
                playerState->stateFlags |= 0x200;
            }
            else {
                playerState->stateFlags |= 0x400;
            }
        }
    }
}

void StartPlayerGroupDraw(s16 groupIndex)
{
    PlayerCharacterState *playerState;
    PlayerGroupState *playerGroup;
    s16 groupIndex;
    s16 playerIndex;
    playerGroup = &playerGroups[groupIndex];
    if (groupIndex == -1) {
        return;
    }
    for (groupIndex = 0; groupIndex < 1; groupIndex++) {
        if (groupIndex == 0) {
            playerGroup = &playerGroups[groupIndex];
        }
        else {
            playerGroup = &playerGroups[groupIndex ^ 1];
        }
        for (playerIndex = 0; playerIndex < playerGroup->playerCount; playerIndex++) {
            playerState = playerObjects[playerGroup->playerIds[playerIndex]]->data;
            playerState->resultAnimationState = 1;
            playerState->resultFacingTransition.x = playerState->facingAngle;
            playerState->resultFacingTransition.y = 0;
            playerState->stateFlags &= ~0x600;

            if (groupIndex == 0) {
                playerState->stateFlags |= 0x200;
            }
            else {
                playerState->stateFlags |= 0x400;
            }
            playerState->alternateResultMotion = 1;
        }
    }
}

void GetPlayerWorldPosition(OMOBJ *object, Vec *worldPositionOut);
void ResetPlayerMotionState(OMOBJ *object);

void CleanupPlayerItemEffect(OMOBJ *object)
{
    Vec particlePosition;
    PlayerCharacterState *playerState;
    playerState = object->data;
    GetPlayerWorldPosition(object, &particlePosition);
    CreateParticleEffect(particlePosition.x, particlePosition.y, particlePosition.z, 20, NULL, -1);
    Hu3DModelAttrSet(playerState->capturedItemModelId, HU3D_ATTR_DISPOFF);
    Hu3DModelHookReset(object->mdlId[0]);
    playerState->capturedItemModelId = -1;
    if (playerState->opponentPriorityFlag == 0) {
        SetPlayerMotion(object, 5, 4);
        SetPlayerMotionTimer(object, 0, 8);
    }
    ResetPlayerMotionState(object);
}

PlayerCharacterState *GetOpposingPlayerState(OMOBJ *object)
{
    PlayerGroupState *playerGroup;
    s16 playerIndex;
    PlayerCharacterState *playerState = object->data;
    s32 unreferencedPlayerSearchFlag = 0;

    playerGroup = &playerGroups[playerState->groupIndex];
    for (playerIndex = 0; playerIndex < playerGroup->playerCount; playerIndex++) {
        if (object->work[playerIndex] != playerGroup->playerIds[playerIndex]) {
            return playerObjects[playerGroup->playerIds[playerIndex]]->data;
        }
    }
    return NULL;
}

u32 GetPlayerStateFlags(OMOBJ *object, u32 stateMask)
{
    PlayerCharacterState *playerState = object->data;
    return playerState->stateFlags & stateMask;
}

void UpdatePlayerStateFlags(OMOBJ *object, u32 clearMask, u32 setMask)
{
    PlayerCharacterState *playerState = object->data;
    playerState->stateFlags &= ~clearMask;
    playerState->stateFlags |= setMask;
}

void UpdatePlayerFlagsByIndex(s16 playerIndex, u32 clearMask, u32 setMask)
{
    PlayerGroupState *playerGroup;
    PlayerCharacterState *playerState;
    s16 playerIndex;
    playerGroup = &playerGroups[playerIndex];
    for (playerIndex = 0; playerIndex < playerGroup->playerCount; playerIndex++) {
        playerState = playerObjects[playerGroup->playerIds[playerIndex]]->data;
        playerState->stateFlags &= ~clearMask;
        playerState->stateFlags |= setMask;
    }
}

void GetPlayerWorldPosition(OMOBJ *object, Vec *worldPositionOut)
{
    PlayerCharacterState *playerState;
    float itemAngle;
    playerState = object->data;
    itemAngle = itemRotationStates[playerState->itemIndex].rotationAngle;
    worldPositionOut->x = playerState->position.x - (50.0 * sind(itemAngle));
    worldPositionOut->y = playerState->position.y + 100;
    worldPositionOut->z = playerState->position.z - (50.0 * cosd(itemAngle));
}

float GetItemRingPosition(s8 stickX)
{
    float normalizedStickX;
    if (stickX > 65) {
        stickX = 65;
    }
    else if (stickX < -65) {
        stickX = -65;
    }
    normalizedStickX = stickX;
    normalizedStickX /= 65.0f;
    return normalizedStickX;
}

float GetRelativeItemRingPosition(s8 stickX, s8 stickY)
{
    float normalizedStickMagnitude;
    normalizedStickMagnitude = 0.01539 * VECMagPoint(stickX, stickY, 0);
    if (normalizedStickMagnitude > 1.0f) {
        normalizedStickMagnitude = 1.0f;
    }
    return normalizedStickMagnitude;
}

void SetPlayerMotion(OMOBJ *object, s16 motionIndex, u32 motionFlags)
{
    PlayerCharacterState *playerState = object->data;
    if (playerState->motionIndex != motionIndex) {
        playerState->stateFlags &= ~0xF;
        playerState->motionIndex = motionIndex;
        playerState->stateFlags |= motionFlags;
    }
}

void SetPlayerMotionTimer(OMOBJ *object, s16 motionIndex, u32 motionFlags)
{
    PlayerCharacterState *playerState = object->data;
    playerState->queuedMotionIndex = motionIndex;
    playerState->queuedMotionFlags = motionFlags;
}

void SetPlayerObjectCallback(OMOBJ *object, OMOBJFUNC callback)
{
    PlayerCharacterState *playerState = object->data;
    playerState->motionCallback = callback;
}

void ResetPlayerMotionState(OMOBJ *object)
{
    SetPlayerObjectCallback(object, NULL);
}

void DestroyPlayerCharacter(OMOBJ *object)
{
    PlayerCharacterState *playerState;
    u32 motionAttributes;
    HU3DMODEL *playerModel;
    OMOBJFUNC completionCallback;
    playerModel = &Hu3DData[object->mdlId[0]];
    playerState = object->data;
    if (playerState->stateFlags & 0x1) {
        if (playerState->queuedMotionIndex != -1 && playerModel->motIdShift == -1) {
            if (Hu3DMotionTimeGet(object->mdlId[0]) >= Hu3DMotionMaxTimeGet(object->mdlId[0])) {
                SetPlayerMotion(object, playerState->queuedMotionIndex, playerState->queuedMotionFlags);
                playerState->queuedMotionIndex = -1;
                if (playerState->motionCallback) {
                    completionCallback = playerState->motionCallback;
                    completionCallback(object);
                }
            }
        }
        else {
            return;
        }
    }
    else {
        Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP | HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
        switch (playerState->stateFlags & 0xC) {
            case 0:
                motionAttributes = HU3D_MOTATTR_PAUSE;
                break;

            case 4:
                motionAttributes = HU3D_MOTATTR_NONE;
                break;

            case 8:
                motionAttributes = HU3D_MOTATTR_LOOP;
                break;
        }
        if (playerState->stateFlags & 0x2) {
            Hu3DMotionSet(object->mdlId[0], object->mtnId[playerState->motionIndex]);
            Hu3DModelAttrSet(object->mdlId[0], motionAttributes);
        }
        else {
            Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[playerState->motionIndex], 0, 8, motionAttributes);
        }
        playerState->stateFlags |= 0x1;
    }
}

void DestroyPlayerGroup(OMOBJ *object);

void UpdatePlayerGroup(HUPROCESS *objman)
{
    s32 *soundQueueEntry;
    s32 queueIndex;
    omAddObjEx(objman, 100, 0, 0, -1, DestroyPlayerGroup);
    soundQueueEntry = &collisionResponseTable[0][0];
    for (queueIndex = 0; queueIndex < 16; queueIndex++, soundQueueEntry += 2) {
        soundQueueEntry[0] = -1;
        soundQueueEntry[1] = -1;
    }
}

void DestroyPlayerGroup(OMOBJ *object)
{
    s32 *soundQueueEntry;
    s32 queueIndex;
    soundQueueEntry = &collisionResponseTable[0][0];
    for (queueIndex = 0; queueIndex < 16; queueIndex++, soundQueueEntry += 2) {
        if (soundQueueEntry[0] >= 0 && soundQueueEntry[1] > 0) {
            if (--soundQueueEntry[1] <= 0) {
                HuAudFXPlay(soundQueueEntry[0]);
                soundQueueEntry[0] = -1;
                soundQueueEntry[1] = -1;
            }
        }
    }
}

s32 PlayPlayerSoundEffect(s32 soundId, s32 volume)
{
    s32 *soundQueueEntry;
    s32 queueIndex;
    soundQueueEntry = &collisionResponseTable[0][0];
    for (queueIndex = 0; queueIndex < 16; queueIndex++, soundQueueEntry += 2) {
        if (soundQueueEntry[0] == -1) {
            break;
        }
    }
    if (queueIndex >= 16) {
        return -1;
    }
    soundQueueEntry[0] = soundId;
    soundQueueEntry[1] = volume;
    return queueIndex;
}

s16 GetItemRingIndex(float ringAngle)
{
    return 10.0f * (ringAngle / 360.0f);
}

s16 GetItemRingDirection(s16 itemIndex)
{
    if (itemIndex >= 0 && itemIndex <= 4) {
        return 0;
    }
    return 1;
}

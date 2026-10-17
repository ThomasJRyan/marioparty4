// Panel Panic 9 Player

#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/esprite.h"
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

#include "ext_math.h"
#include <string.h>

extern s32 rand8(void);

typedef struct {
    /* 0x00 */ s32 gridColumn;
    /* 0x04 */ s32 gridRow;
    /* 0x08 */ float characterHeight;
    /* 0x0C */ s32 heightTimer;
} BoardCellState; // Size 0x10

typedef struct {
    /* 0x00 */ s32 activeEffectCount;
    /* 0x04 */ float effectOffsets[3];
} CellEffectQueue; // Size 0x10

typedef struct {
    /* 0x00 */ s32 player;
    /* 0x04 */ s32 playerCfg;
    /* 0x08 */ s32 character;
    /* 0x0C */ s32 padIdx;
    /* 0x10 */ s32 diff;
    /* 0x14 */ s32 aiMoveTimer;
    /* 0x18 */ s32 playerState;
    /* 0x1C */ s32 stateTimer;
    /* 0x20 */ s32 isMoving;
    /* 0x24 */ s32 pendingMove;
    /* 0x28 */ float movementChoice;
    /* 0x2C */ Vec velocity;
    /* 0x38 */ s32 gridCell;
    /* 0x3C */ s32 gridColumn;
    /* 0x40 */ s32 gridRow;
    /* 0x44 */ s32 selectedCell;
    /* 0x48 */ s32 effectTimer;
    /* 0x4C */ s32 effectState;
    /* 0x50 */ s32 hasMoved;
    /* 0x54 */ s16 parMan;
    /* 0x58 */ s32 parManTimer;
    /* 0x5C */ s32 isFinished;
} PanelPanicPlayerData; // Size 0x60

typedef struct {
    /* 0x00 */ s32 windTimer;
    /* 0x04 */ s32 windFrame;
    /* 0x08 */ float windX;
    /* 0x0C */ float windZ;
    /* 0x10 */ float targetWindX;
    /* 0x14 */ float targetWindZ;
} WindAnimationState; // Size 0x18

s16 WindParticleCreate(void);
void MinigameExit(OMOBJ *exitObject);

BoardCellState characterGrid[9];
CellEffectQueue effectState[28];
OMOBJ *exitObject;
OMOBJ *viewObject;
OMOBJ *playerObjs[9];
OMOBJ *boardObject;
HU3DPARMANPARAM playerParticleParams[9];
s32 particleParamIndex;
s32 phaseTimer;
s32 minigamePhase;           //// TODO: setup enum when what this is gets figured out
char unusedBss4C[4]; // unused
s32 unusedState48;
s32 unusedState44;
HUPROCESS *objectManager;
s32 lightId;
s32 winnerPlayer;
s32 windCycleTimer;
char unusedBss30[4]; // unused
float cameraTargetYaw;
float cameraTargetZ;
float cameraTargetX;
float cameraTransitionProgress;
s32 cameraTransitionDirection;
s32 windEffectTimer;
s32 playerMoveCount;
char unusedBssE[6]; // unused
s16 messageWindow;
char unusedBss8[4]; // unused
s32 frameCounter;
s32 playerMoveTimer;

s32 cellSpriteData[9] = { DATA_MAKE_NUM(DATADIR_M458, 38), DATA_MAKE_NUM(DATADIR_M458, 38), DATA_MAKE_NUM(DATADIR_M458, 38),
    DATA_MAKE_NUM(DATADIR_M458, 38), DATA_MAKE_NUM(DATADIR_M458, 38), DATA_MAKE_NUM(DATADIR_M458, 38), DATA_MAKE_NUM(DATADIR_M458, 38),
    DATA_MAKE_NUM(DATADIR_M458, 38), DATA_MAKE_NUM(DATADIR_M458, 38) };

s32 cellSprites[9] = { 0 };

s32 characterMotionData[9] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 2),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 3),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 5),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 6),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 47),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 26),
    DATA_MAKE_NUM(DATADIR_M458, 0),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 23),
};

u32 characterMotionIsCharacterSpecific[9] = { 0, 0, 0, 0, 0, 0, 0, 1, 0 };

u32 koopaMotionData[] = { DATA_MAKE_NUM(DATADIR_BKOOPA, 8), 0, 0, DATA_MAKE_NUM(DATADIR_BKOOPA, 10), 0, DATA_MAKE_NUM(DATADIR_M458, 42),
    DATA_MAKE_NUM(DATADIR_M458, 40), DATA_MAKE_NUM(DATADIR_M458, 41), DATA_MAKE_NUM(DATADIR_M458, 39) };

u8 characterMotionLoopFlags[9] = { 0, 0, 0, 1, 1, 1, 0, 1, 1 };
u8 koopaMotionLoopFlags[9] = { 0, 0, 0, 1, 1, 1, 0, 1, 1 };
s32 characterMotionDurations[9] = { 8, 8, 8, 0, 0, 8, 8, 8, 8 };
s32 characterMotionTimeOffsets[9] = { 0, 0, 0, 0, 3, 0, 0, 0, 0 };
s32 koopaMotionDurations[9] = { 8, 8, 8, 8, 8, 8, 8, 8, 8 };

WindAnimationState windAnimation = { 0 };

s16 windState = -1;
s16 countdownSequence = -1;
s32 windMusicSequence = -1;
s32 introSoundEffect = -1;

// unused
float unusedCameraPositions[] = { -90.0f, 0.0f, 525.0f, 180.0f, 90.0f, 0.0f, 525.0f, 180.0f };

void DebugCameraUpdate(void)
{
    if (HuPadBtnDown[0] & PAD_BUTTON_B) {
        Center.x = playerObjs[0]->trans.x;
        Center.y = playerObjs[0]->trans.y;
        Center.z = playerObjs[0]->trans.z;
    }
    Center.x += 10.0 * (((HuPadBtn[0] & PAD_BUTTON_RIGHT) != 0) - ((HuPadBtn[0] & PAD_BUTTON_LEFT) != 0));
    Center.z -= 10.0 * (((HuPadBtn[0] & PAD_BUTTON_UP) != 0) - ((HuPadBtn[0] & PAD_BUTTON_DOWN) != 0));
    Center.y += 30.0 * ((HuPadTrigL[0] - HuPadTrigR[0]) / 128.0);
    CZoom += 25.0 * (((HuPadBtn[0] & PAD_BUTTON_X) != 0) - ((HuPadBtn[0] & PAD_BUTTON_Y) != 0));
    CRot.y += HuPadSubStkX[0] / 16.0;
    CRot.x += HuPadSubStkY[0] / -16.0;
    print8(64, 392, 1.0f, "%f,%f,%f", Center.x, Center.y, Center.z);
    print8(64, 408, 1.0f, "%f,%f,%f", CRot.x, CRot.y, CRot.z);
    print8(64, 424, 1.0f, "%f", CZoom);
}

s32 windParticleEffect = -1;

void PlayerMovementUpdate(OMOBJ *object)
{
    HU3DMODEL *windEffectModel;
    HU3DMODEL *fallingCellModel;
    float movementVelocityX;
    float movementVelocityZ;
    float movementSpeed;
    float modelScale;
    float modelScaleProgress;
    float cellEffectScale;
    float modelTransparency;
    float requestedColumnVelocity;
    float requestedRowVelocity;
    float effectPulseScale;
    float effectBaseScale;
    PanelPanicPlayerData *playerData;
    s16 requestedMotionId;
    s32 effectFrame;
    s32 columnStep;
    s32 rowStep;
    s32 playerIndex;
    s32 confirmPressed;
    PanelPanicPlayerData *otherPlayerData;
    s32 neighborColumn;
    s32 neighborRow;
    PanelPanicPlayerData *otherPlayerData;
    s32 player;
    float messageWindowSize[2];
    s32 firstNearbyColumn;
    s32 lastNearbyColumn;
    s32 firstNearbyRow;
    s32 lastNearbyRow;
    s32 unusedCollisionStateA;
    s32 unusedCollisionStateB;
    s32 candidatePlayerIndex;
    PanelPanicPlayerData *candidatePlayerData;
    s32 activePlayerIndex;
    s32 remainingPlayerCount;
    PanelPanicPlayerData *countedPlayerData;
    s32 cellColumnOffset;
    u32 cellRowOffset;
    float cellEffectProgress;
    s32 cellEffectSoundId;
    s32 activePlayerIndex;
    s32 remainingPlayerCount;
    PanelPanicPlayerData *activePlayerData;
    HU3DMODEL *effectModel;
    float cellEffectAngle;
    float landingEffectAngle;
    s32 selectionAttempt;
    s32 previousSelectedCell;

    playerData = object->data;
    requestedColumnVelocity = 0.0f;
    requestedRowVelocity = 0.0f;
    movementSpeed = 0.0f;
    confirmPressed = 0;
    requestedMotionId = -1;
    playerData->velocity.x = playerData->velocity.z = 0.0f;
    if (minigamePhase == 1005 && playerData->playerState == 3 && playerData->stateTimer >= 360) {
        if (playerData->stateTimer == 360) {
            playerData->aiMoveTimer = rand8() % 15;
        }
        if (playerData->padIdx != -1) {
            confirmPressed = (HuPadBtnDown[playerData->padIdx] & PAD_BUTTON_A) ? 1 : 0;
        }
        else {
            playerData->aiMoveTimer += (playerData->aiMoveTimer < 0x10000);
            if (playerData->aiMoveTimer == 30) {
                confirmPressed = 1;
            }
        }
    }
    if (playerData->player < 4 && minigamePhase == 1002) {
        columnStep = 0;
        rowStep = 0;
        if (playerData->padIdx != -1) {
            columnStep = (HuPadStkX[playerData->padIdx] < -20) ? -1 : (HuPadStkX[playerData->padIdx] > 20) ? 1 : 0;
            rowStep = (-HuPadStkY[playerData->padIdx] < -20) ? -1 : (-HuPadStkY[playerData->padIdx] > 20) ? 1 : 0;
            confirmPressed = (HuPadBtnDown[playerData->padIdx] & PAD_BUTTON_A) ? 1 : 0;
        }
        else {
            if (phaseTimer == 1) {
                playerData->aiMoveTimer = rand8() % 64 + 30;
            }
            playerData->aiMoveTimer -= (playerData->aiMoveTimer > 0);
            if (playerData->aiMoveTimer == 1) {
                columnStep = (playerData->gridColumn > 0 ? -1 : 1) * (rand8() % 2);
                rowStep = (playerData->gridRow > 0 ? -1 : 1) * (rand8() % 2);
            }
            if (playerData->aiMoveTimer == 0) {
                confirmPressed = (rand8() % 64 == 1) ? 1 : 0;
            }
        }
        if (phaseTimer >= 50 || playerData->hasMoved != 0) {
            columnStep = rowStep = 0;
        }
        for (player = 0; player < 4; player++) {
            otherPlayerData = playerObjs[player]->data;
            unusedCollisionStateA = 0;
            unusedCollisionStateB = 0;
            if (player != playerData->player) {
                if (fabs(500.0 * playerData->gridColumn - object->trans.x) < 10.0) {
                    firstNearbyColumn = lastNearbyColumn = (object->trans.x < -250.0) ? -1 : (object->trans.x > 250.0) ? 1 : 0;
                }
                else {
                    firstNearbyColumn = (object->trans.x < 0.0f) ? -1 : 0;
                    lastNearbyColumn = firstNearbyColumn + 1;
                }
                if (fabs(500.0 * playerData->gridRow - object->trans.z) < 10.0) {
                    firstNearbyRow = lastNearbyRow = (object->trans.z < -250.0) ? -1 : (object->trans.z > 250.0) ? 1 : 0;
                }
                else {
                    firstNearbyRow = (object->trans.z < 0.0f) ? -1 : 0;
                    lastNearbyRow = firstNearbyRow + 1;
                }
                for (neighborColumn = firstNearbyColumn; neighborColumn <= lastNearbyColumn; neighborColumn++) {
                    for (neighborRow = firstNearbyRow; neighborRow <= lastNearbyRow; neighborRow++) {
                        if (otherPlayerData->gridCell == (1 + neighborColumn + columnStep) + (1 + neighborRow + rowStep) * 3) {
                            columnStep = rowStep = 0;
                        }
                        if (otherPlayerData->gridCell == (1 + neighborColumn + columnStep) + (1 + neighborRow) * 3) {
                            columnStep = 0;
                        }
                        if (otherPlayerData->gridCell == (1 + neighborColumn) + (1 + neighborRow + rowStep) * 3) {
                            rowStep = 0;
                        }
                    }
                }
            }
        }
        if (columnStep * (500.0 * playerData->gridColumn - object->trans.x) < 0.0) {
            playerData->gridColumn += columnStep;
        }
        if (fabs(500.0 * playerData->gridColumn - object->trans.x) < 10.0 && columnStep != 0 && playerData->gridColumn + columnStep >= -1
            && playerData->gridColumn + columnStep <= 1) {
            playerData->gridColumn += columnStep;
        }
        requestedColumnVelocity = ((500.0 * playerData->gridColumn - object->trans.x) / 0.25) / 1.75;
        if (rowStep * (500.0 * playerData->gridRow - object->trans.z) < 0.0) {
            playerData->gridRow += rowStep;
        }
        if (fabs(500.0 * playerData->gridRow - object->trans.z) < 10.0 && rowStep != 0 && playerData->gridRow + rowStep >= -1
            && playerData->gridRow + rowStep <= 1) {
            playerData->gridRow += rowStep;
        }
        requestedRowVelocity = ((500.0 * playerData->gridRow - object->trans.z) / -0.25) / 1.75;
        playerData->gridCell = (playerData->gridColumn + 1) + (playerData->gridRow + 1) * 3;
    }
    if (playerData->playerState == 0) {
        if (minigamePhase > (playerData->player < 4 ? 1001 : 1003) && minigamePhase < 1006) {
            movementVelocityX = 0.25 * requestedColumnVelocity;
            movementVelocityZ = 0.25 * -requestedRowVelocity;
            {
                Vec movementDirection = { 0.0f, 0.0f, 0.0f };
                Mtx movementRotationMatrix;

                movementDirection.x = movementVelocityX;
                movementDirection.z = movementVelocityZ;
                MTXRotRad(movementRotationMatrix, 'y', MTXDegToRad(CRot.y));
                MTXMultVec(movementRotationMatrix, &movementDirection, &movementDirection);
                movementVelocityX = movementDirection.x;
                movementVelocityZ = movementDirection.z;
                movementSpeed = sqrtf(movementVelocityX * movementVelocityX + movementVelocityZ * movementVelocityZ);
                if (movementSpeed >= 0.5f) {
                    if (movementSpeed < 10.0f) {
                        playerData->velocity.x = movementVelocityX;
                        playerData->velocity.z += movementVelocityZ;
                    }
                    else {
                        movementVelocityX /= movementSpeed;
                        movementVelocityZ /= movementSpeed;
                        movementSpeed = 10.0f;
                        playerData->velocity.x = movementVelocityX * movementSpeed;
                        playerData->velocity.z = movementVelocityZ * movementSpeed;
                    }
                    playerData->movementChoice = movementSpeed;
                    object->rot.y = atan2d(playerData->velocity.x, playerData->velocity.z);
                    if (playerData->movementChoice >= 8.0f) {
                        requestedMotionId = 2;
                    }
                    else {
                        requestedMotionId = 2;
                    }
                }
                else {
                    requestedMotionId = 0;
                }
            }
        }
    }
    object->trans.x += 1.75f * playerData->velocity.x;
    object->trans.z += 1.75f * playerData->velocity.z;
    switch (minigamePhase) {
        case 1001:
        case 1003:
            if (minigamePhase != (playerData->player < 4 ? 1001 : 1003)) {
                break;
            }
            if (phaseTimer == 0) {
                Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
                if (playerData->player < 4) {
                    playerData->gridCell = (playerData->player == 0) ? 0 : (playerData->player == 1) ? 2 : (playerData->player == 2) ? 6 : 8;
                    playerData->gridColumn = playerData->gridCell % 3 - 1;
                    playerData->gridRow = playerData->gridCell / 3 - 1;
                    object->trans.x = 500.0 * playerData->gridColumn;
                    object->trans.z = 500.0 * playerData->gridRow;
                }
                else {
                    do {
                        playerData->gridCell = rand8() % 9;
                        for (candidatePlayerIndex = 0; candidatePlayerIndex < playerData->player; candidatePlayerIndex++) {
                            candidatePlayerData = playerObjs[candidatePlayerIndex]->data;
                            if (playerData->gridCell == candidatePlayerData->gridCell) {
                                break;
                            }
                        }
                    } while (candidatePlayerIndex != playerData->player);
                    playerData->gridColumn = playerData->gridCell % 3 - 1;
                    playerData->gridRow = playerData->gridCell / 3 - 1;
                    object->trans.x = 500.0 * playerData->gridColumn;
                    object->trans.z = 500.0 * playerData->gridRow;
                }
            }
            if (phaseTimer <= 30) {
                if (playerData->character == -1) {
                    Hu3DMotionTimeSet(object->mdlId[0], 138.0f);
                }
                object->trans.y = 2400.0 * (1.0 - phaseTimer / 30.0);
            }
            if (playerData->character != -1 && phaseTimer == 30) {
                requestedMotionId = 4;
            }
            if (phaseTimer == (playerData->character != -1 ? 10 : 60) + 30) {
                requestedMotionId = 0;
            }
            if (phaseTimer == 30) {
                windAnimation.windFrame += 50;
            }
            if (phaseTimer == 30 && (playerData->player == 0 || playerData->player == 4)) {
                HuAudFXPlay(0x33D);
            }
            break;
        case 1002:
            if (phaseTimer != 0 && playerData->player < 4 && (confirmPressed != 0 || playerMoveTimer == 0) && playerData->hasMoved == 0) {
                playerData->hasMoved = 1;
                playerMoveCount++;
                characterGrid[playerData->gridCell].characterHeight = 83.333336f;
            }
            if (playerData->player != 0) {
                break;
            }
            if (phaseTimer == 0) {
                playerMoveTimer = REFRESH_RATE*5;
                countdownSequence = MGSeqTimerCreate(10);
                HuWinMesMaxSizeGet(1, messageWindowSize, MAKE_MESSID(40, 25));
                messageWindow = HuWinCreate(-10000.0f, 400.0f, messageWindowSize[0], messageWindowSize[1], 1);
                HuWinBGTPLvlSet(messageWindow, 0.0f);
                HuWinMesSpeedSet(messageWindow, 0);
                HuWinMesSet(messageWindow, MAKE_MESSID(40, 25));
            }
            playerMoveTimer--;
            if (countdownSequence != -1) {
                MGSeqParamSet(countdownSequence, 1, (playerMoveTimer + (REFRESH_RATE-1)) / REFRESH_RATE);
            }
            if (playerMoveCount >= 4) {
                phaseTimer = 50;
                MGSeqParamSet(countdownSequence, 2, -1);
                countdownSequence = -1;
                HuWinExCleanup(messageWindow);
            }
            if (phaseTimer == 50) {
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    otherPlayerData = playerObjs[playerIndex]->data;
                    if (!(fabs(500.0 * otherPlayerData->gridColumn - playerObjs[playerIndex]->trans.x) < 1.0)
                        || !(fabs(500.0 * otherPlayerData->gridRow - playerObjs[playerIndex]->trans.z) < 1.0) || otherPlayerData->isMoving != 0) {
                        break;
                    }
                }
                if (playerIndex == 4) {
                    phaseTimer = 60;
                }
            }
            break;
        case 1005:
            if (playerData->isFinished != 0) {
                if (object->trans.y < -15000.0f) {
                    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
                }
                else {
                    object->trans.y -= 77.0f;
                }
            }
            break;
        case 1006:
            if (phaseTimer <= 1) {
                Hu3DModelAttrSet(object->mdlId[playerData->selectedCell + 2], HU3D_ATTR_DISPOFF);
                characterGrid[playerData->selectedCell].heightTimer = 0;
            }
            if (characterGrid[playerData->gridCell].heightTimer != 0 || playerData->isFinished != 0) {
                break;
            }
            if (phaseTimer == 120) {
                Hu3DModelAttrReset(boardObject->mdlId[playerData->gridCell + 1], HU3D_MOTATTR_PAUSE);
            }
            if (phaseTimer == 153) {
                Hu3DModelAttrSet(boardObject->mdlId[playerData->gridCell + 1], HU3D_MOTATTR_PAUSE);
            }
            if (phaseTimer == 183) {
                Hu3DModelAttrReset(boardObject->mdlId[playerData->gridCell + 1], HU3D_MOTATTR_PAUSE);
            }
            if (phaseTimer == 210) {
                remainingPlayerCount = 0;
                for (activePlayerIndex = 0; activePlayerIndex < 9; activePlayerIndex++) {
                    countedPlayerData = playerObjs[activePlayerIndex]->data;
                    remainingPlayerCount += (characterGrid[countedPlayerData->gridCell].heightTimer == 0 && countedPlayerData->isFinished == 0);
                }
                requestedMotionId = 7;
                HuAudFXPlayVol(0x7F7 + (playerData->character != -1 ? playerData->character : 8), 1651 / (remainingPlayerCount + 12));
            }
            if (phaseTimer >= 210 && phaseTimer <= 214) {
                s32 characterSpriteYOffset[] = { 0, -12, -36, 0, 0, -12, -36, -48, 0 };
                float cellColumnSpacing[] = { 150.0f, 175.0f, 205.0f };
                float cellRowPositions[] = { 100.0f, 170.0f, 260.0f };

                cellColumnOffset = playerData->gridCell % 3 - 1;
                cellRowOffset = playerData->gridCell / 3;
                cellEffectProgress = (phaseTimer - 210) / 4.0;
                modelScaleProgress = (phaseTimer - 210) / 4.0;
                modelScaleProgress = (modelScaleProgress > 1.0f) ? 1.0f : modelScaleProgress;
                espAttrReset(cellSprites[playerData->gridCell], HUSPR_ATTR_DISPOFF);
                espPosSet(cellSprites[playerData->gridCell], 290.0f + cellColumnOffset * cellColumnSpacing[cellRowOffset],
                    cellRowPositions[cellRowOffset] + 0.5 * (64.0f * (1.0f - cellEffectProgress) * 0.5 + characterSpriteYOffset[playerData->character != -1 ? playerData->character : 8]));
                espScaleSet(cellSprites[playerData->gridCell], 0.5 * modelScaleProgress, 0.5 * modelScaleProgress);
            }
            if (phaseTimer == 270) {
                Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
                espAttrSet(cellSprites[playerData->gridCell], HUSPR_ATTR_DISPOFF);
            }
            if (phaseTimer == 290) {
                remainingPlayerCount = 0;
                for (activePlayerIndex = 0; activePlayerIndex < 9; activePlayerIndex++) {
                    activePlayerData = playerObjs[activePlayerIndex]->data;
                    remainingPlayerCount += (characterGrid[activePlayerData->gridCell].heightTimer == 0 && activePlayerData->isFinished == 0);
                }
                cellEffectSoundId = HuAudFXPlayVol(0x800 + (playerData->character != -1 ? playerData->character : 8), 1651 / (remainingPlayerCount + 12));
                HuAudFXFadeOut(cellEffectSoundId, 0x9C4);
            }
            if (phaseTimer >= 315) {
                object->trans.y -= 77.0f;
            }
            break;
        case 1007:
            if (playerData->player == winnerPlayer) {
                requestedMotionId = 8;
                if (playerData->player >= 4 && playerData->character != -1 && phaseTimer == 1) {
                    HuAudFXPlay(0x809 + playerData->character);
                }
                if (playerData->character == -1 && phaseTimer == 1) {
                    HuAudFXPlay(0x3A);
                }
            }
            else if (object->trans.y < -40000.0f) {
                Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
            }
            else {
                object->trans.y -= 77.0f;
            }
            break;
    }
    switch (playerData->playerState) {
        case 0:
            playerData->stateTimer++;
            break;
        case 1:
            playerData->stateTimer++;
            if (playerData->stateTimer <= 60) {
                object->rot.y = -180.0 * (playerData->stateTimer / 60.0);
            }
            else {
                Hu3DMotionTimeSet(object->mdlId[0], 120.0f);
                requestedMotionId = 0;
                playerData->playerState = 0;
                playerData->stateTimer = 0;
            }
            break;
        case 2:
            playerData->stateTimer++;
            break;
        case 3:
            if (playerData->stateTimer == 0) {
                playerData->effectTimer = 6;
                playerData->effectState = 0;
            }
            if (playerData->stateTimer < 360) {
                windEffectModel = &Hu3DData[object->mdlId[1]];
                if (playerData->stateTimer == 0) {
                    Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
                    Hu3DModelPosSet(object->mdlId[1], object->trans.x, object->trans.y + 625.0f, object->trans.z);
                    Hu3DModelScaleSet(object->mdlId[1], 0.1f, 0.1f, 0.1f);
                    Hu3DModelTPLvlSet(object->mdlId[1], 1.0f);
                    playerData->effectState = 0;
                    if (windEffectTimer == 0) {
                        HuAudFXPlay(0x7F4);
                        windParticleEffect = HuAudFXPlay(0x7F3);
                    }
                    windEffectTimer++;
                }
                windEffectModel->rot.y = playerData->effectState % 360;
                playerData->effectState += 25;
                if (playerData->stateTimer < 180) {
                    effectBaseScale = 0.0f;
                    effectPulseScale = 1.0f;
                }
                else {
                    effectBaseScale = 1.0f;
                    effectPulseScale = 0.5f;
                }
                windEffectModel->scale.x = effectBaseScale + effectPulseScale * sind((float)(playerData->stateTimer % 180));
                windEffectModel->scale.y = windEffectModel->scale.z = windEffectModel->scale.x;
                if (playerData->stateTimer < 180) {
                    playerData->stateTimer += 6;
                }
                else {
                    playerData->stateTimer += 18;
                }
                if (playerData->stateTimer >= 360) {
                    playerData->stateTimer = 360;
                    Hu3DModelRotSet(object->mdlId[1], 0.0f, 0.0f, 0.0f);
                }
            }
            else {
                effectModel = &Hu3DData[object->mdlId[1]];
                playerData->stateTimer += 4;
                if (playerData->stateTimer > 720) {
                    playerData->stateTimer -= 360;
                }
                effectModel->pos.y += 0.2f * sind(playerData->stateTimer - 360);
                if (confirmPressed != 0 || playerMoveTimer == 0) {
                    phaseTimer = 50;
                    playerData->playerState = 4;
                    playerData->stateTimer = 0;
                    requestedMotionId = 5;
                }
            }
            break;
        case 4:
            playerData->stateTimer++;
            if (playerData->stateTimer < 27) {
                break;
            }
            fallingCellModel = &Hu3DData[object->mdlId[1]];
            effectFrame = playerData->stateTimer - 27;
            if (effectFrame == 0) {
                if (playerData->playerCfg != -1) {
                    omVibrate(playerData->playerCfg, 12, 4, 2);
                }
                playerData->effectTimer = -1;
                windEffectTimer--;
                if (windEffectTimer == 0 && windParticleEffect != -1) {
                    HuAudFXStop(windParticleEffect);
                    windParticleEffect = -1;
                }
                HuAudFXPlay(0x7F5);
            }
            if (effectFrame == 8) {
                if (playerData->parMan == -1) {
                    particleParamIndex = playerData->player;
                    playerData->parMan = WindParticleCreate();
                    Hu3DParManPosSet(playerData->parMan, fallingCellModel->pos.x, fallingCellModel->pos.y, fallingCellModel->pos.z);
                    playerData->parManTimer = 150;
                }
                Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
            }
            else if (effectFrame > 8) {
                if (playerData->isMoving == 5) {
                    if (Hu3DMotionTimeGet(object->mdlId[0]) >= Hu3DMotionMaxTimeGet(object->mdlId[0])) {
                        requestedMotionId = 0;
                    }
                }
                if (effectFrame <= 28) {
                    modelScale = (effectFrame - 8) / 10.0;
                    cellEffectAngle = (effectFrame - 8) * 18;
                    modelScale = (modelScale > 1.0f) ? 2.5 : (2.5 * modelScale);
                    Hu3DModelAttrReset(object->mdlId[playerData->selectedCell + 2], HU3D_ATTR_DISPOFF);
                    Hu3DModelPosSet(object->mdlId[playerData->selectedCell + 2], fallingCellModel->pos.x, fallingCellModel->pos.y, fallingCellModel->pos.z);
                    Hu3DModelScaleSet(object->mdlId[playerData->selectedCell + 2], modelScale, modelScale, modelScale);
                }
                if (effectFrame >= 45 && effectFrame <= 65) {
                    cellEffectScale = (effectFrame - 45) / 5.0;
                    landingEffectAngle = (effectFrame - 45) * 18;
                    cellEffectScale = (cellEffectScale > 1.0f) ? 1.0f : cellEffectScale;
                    cellEffectScale = 0.0f;
                    Hu3DModelAttrReset(object->mdlId[playerData->selectedCell + 2], HU3D_ATTR_DISPOFF);
                    Hu3DModelPosSet(object->mdlId[playerData->selectedCell + 2], fallingCellModel->pos.x, fallingCellModel->pos.y - 30.0f * cellEffectScale, fallingCellModel->pos.z);
                    playerData->stateTimer += 5;
                }
                if (effectFrame == 158) {
                    playerData->playerState = 0;
                    playerData->stateTimer = 0;
                }
            }
            else {
                if (effectFrame < 4) {
                    fallingCellModel->pos.y += 10.0f + 0.5f * effectFrame;
                    fallingCellModel->scale.x += 0.1f * sind(4.0f * effectFrame);
                    fallingCellModel->scale.y = fallingCellModel->scale.z = fallingCellModel->scale.x;
                }
                else {
                    modelTransparency = 255 - (effectFrame << 5);
                    modelTransparency = modelTransparency * (1.0f / 255);
                    Hu3DModelTPLvlSet(object->mdlId[1], modelTransparency);
                }
                fallingCellModel->pos.y += -0.49f * effectFrame * effectFrame;
            }
            break;
        case 5:
            if (playerData->stateTimer == 0) {
                Hu3DModelAttrReset(boardObject->mdlId[playerData->selectedCell + 1], HU3D_MOTATTR_PAUSE);
                characterGrid[playerData->selectedCell].heightTimer = 0;
                if (effectState->activeEffectCount < 3) {
                    effectState->effectOffsets[effectState->activeEffectCount] = 0.0f;
                    effectState->activeEffectCount++;
                }
            }
            playerData->stateTimer++;
            if (playerData->stateTimer == 60 && playerData->parMan != -1) {
                Hu3DParManKill(playerData->parMan);
                playerData->parMan = -1;
            }
            if (playerData->stateTimer == 33) {
                Hu3DModelAttrSet(boardObject->mdlId[playerData->selectedCell + 1], HU3D_MOTATTR_PAUSE);
            }
            if (playerData->stateTimer == 63) {
                Hu3DModelAttrReset(boardObject->mdlId[playerData->selectedCell + 1], HU3D_MOTATTR_PAUSE);
            }
            if (playerData->stateTimer == 68 && effectState[1].activeEffectCount < 3) {
                effectState[1].effectOffsets[effectState[1].activeEffectCount] = 0.0f;
                effectState[1].activeEffectCount++;
            }
            if (playerData->stateTimer == 130) {
                playerData->playerState = 0;
                playerData->stateTimer = 0;
            }
            break;
    }
    if ((playerData->playerState == 3 || playerData->playerState == 4) && playerData->effectTimer >= 0) {
        playerData->effectTimer++;
        if (playerData->effectTimer >= 6) {
            previousSelectedCell = playerData->selectedCell;
            for (selectionAttempt = 0; selectionAttempt < 50; selectionAttempt++) {
                do {
                    playerData->selectedCell = rand8() % 9;
                    if (playerData->selectedCell >= 9) {
                        break;
                    }
                } while (characterGrid[playerData->selectedCell].heightTimer == 0);
                if (previousSelectedCell != playerData->selectedCell) {
                    break;
                }
            }
            previousSelectedCell = playerData->selectedCell;
            playerData->effectTimer = 0;
            Hu3DMotionTimeSet(object->mdlId[1], 0.5 + previousSelectedCell);
        }
    }
    if (playerData->pendingMove >= 0) {
        requestedMotionId = playerData->pendingMove;
    }
    if (requestedMotionId >= 0 && requestedMotionId != playerData->isMoving) {
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[requestedMotionId], characterMotionTimeOffsets[requestedMotionId],
            (playerData->character != -1) ? characterMotionDurations[requestedMotionId] : koopaMotionDurations[requestedMotionId],
            (requestedMotionId == 7)                                                                  ? HU3D_MOTATTR_PAUSE
                : (playerData->character != -1 ? characterMotionLoopFlags[requestedMotionId] : koopaMotionLoopFlags[requestedMotionId]) ? 0
                                                                                            : HU3D_MOTATTR_LOOP);
        playerData->isMoving = requestedMotionId;
    }
    playerData->parManTimer -= (playerData->parManTimer > -1);
    if (playerData->parManTimer == 0) {
        if (playerData->parMan != -1) {
            Hu3DParManKill(playerData->parMan);
            playerData->parMan = -1;
        }
        playerData->parManTimer = -1;
    }
}

s32 playerLandingSoundPlayed[4] = { 0 };
s32 cameraMode = -1;
float cameraStartZoom = 3500.0f;
float cameraStartY = 100.0f;
float cameraStartPitch = -59.0f;

static inline void CameraTransitionUpdate(void) {
    float cameraEaseProgress;
    s32 previousCameraMode = cameraMode;
    float cameraPresets[][6] = { { 3500.0f, 0.0f, 100.0f, 0.0f, -59.0f, 0.0f }, { 3200.0f, 0.0f, 100.0f, 0.0f, -30.0f, 0.0f },
        { 3800.0f, 0.0f, 100.0f, 0.0f, -30.0f, 0.0f }, { 2000.0f, 0.0f, 200.0f, 0.0f, -30.0f, 0.0f },
        { 4000.0f, 0.0f, 200.0f, 0.0f, -89.0f, 0.0f }, { 2700.0f, 0.0f, 200.0f, 0.0f, -30.0f, 0.0f } };

    if (winnerPlayer != -1 && winnerPlayer != 9) {
        cameraPresets[5][1] = playerObjs[winnerPlayer]->trans.x;
        cameraPresets[5][3] = playerObjs[winnerPlayer]->trans.z;
    }
    if (cameraMode == -1) {
        cameraMode = 0;
        cameraTransitionDirection = 1;
        cameraTransitionProgress = 1.0f;
    }
    if (cameraMode == 0) {
        cameraMode = 1;
        cameraTransitionDirection = 1;
        cameraTransitionProgress = 0.5f;
    }
    if (minigamePhase == 1004 && phaseTimer >= 1 && cameraMode == 1) {
        cameraMode = 2;
        cameraTransitionDirection = 0;
        cameraTransitionProgress = 0.0f;
    }
    if (minigamePhase == 1006 && phaseTimer >= 270 && cameraMode == 2) {
        cameraMode = 4;
        cameraTransitionDirection = 1;
        cameraTransitionProgress = 1.0f;
    }
    if (minigamePhase == 1005 && cameraMode == 4) {
        cameraMode = 2;
        cameraTransitionDirection = 0;
        cameraTransitionProgress = 0.0f;
    }
    if (minigamePhase == 1007 && winnerPlayer != 9 && cameraMode == 4) {
        cameraMode = 5;
        cameraTransitionDirection = 0;
        cameraTransitionProgress = 0.0f;
    }
    if (previousCameraMode != cameraMode && previousCameraMode != -1) {
        cameraStartZoom = CZoom;
        cameraStartY = Center.y;
        cameraTargetZ = Center.z;
        cameraStartPitch = CRot.x;
        cameraTargetYaw = CRot.y;
    }
    if (cameraTransitionProgress <= 1.0f) {
        if (cameraTransitionDirection == 0) {
            cameraTransitionProgress = 1.0 - 0.95 * (1.0f - cameraTransitionProgress);
            cameraEaseProgress = cameraTransitionProgress;
        }
        else {
            cameraTransitionProgress += 0.008333333333333333;
            cameraTransitionProgress = (cameraTransitionProgress > 1.0f) ? 1.0f : cameraTransitionProgress;
            cameraEaseProgress = 0.5 + -0.5 * cosd(180.0f * cameraTransitionProgress);
        }
        cameraTransitionProgress = (cameraTransitionProgress > 1.0f) ? 1.0f : cameraTransitionProgress;
        CZoom = cameraEaseProgress * cameraPresets[cameraMode][0] + cameraStartZoom * (1.0f - cameraEaseProgress);
        Center.x = cameraEaseProgress * cameraPresets[cameraMode][1] + cameraTargetX * (1.0f - cameraEaseProgress);
        Center.y = cameraEaseProgress * cameraPresets[cameraMode][2] + cameraStartY * (1.0f - cameraEaseProgress);
        Center.z = cameraEaseProgress * cameraPresets[cameraMode][3] + cameraTargetZ * (1.0f - cameraEaseProgress);
        CRot.x = cameraEaseProgress * cameraPresets[cameraMode][4] + cameraStartPitch * (1.0f - cameraEaseProgress);
        CRot.y = cameraEaseProgress * cameraPresets[cameraMode][5] + cameraTargetYaw * (1.0f - cameraEaseProgress);
    }
}

void BoardSceneUpdate(OMOBJ *boardObject)
{
    float boardExpansionProgress;
    float windOffsetX;
    float windOffsetY;
    float windDistanceSquared;
    PanelPanicPlayerData *playerDataByIndex[9];
    HU3DMODEL *playerCellModel;
    HU3DMODEL *boardCellModel;
    s32 playedLandingSound;
    s32 i;
    s32 j;

    for (i = 0; i < 9; i++) {
        playerDataByIndex[i] = playerObjs[i]->data;
    }
    frameCounter++;
    switch (minigamePhase) {
        case 1002:
            windCycleTimer++;
            windCycleTimer %= 360;
            /* fallthrough */
        case 1003:
            playedLandingSound = 0;
            for (i = 0; i < 4; i++) {
                if (playerDataByIndex[i]->hasMoved == 0 || playerDataByIndex[i]->isMoving != 0) {
                    continue;
                }
                playerCellModel = &Hu3DData[boardObject->mdlId[playerDataByIndex[i]->gridCell + 1]];
                if (playedLandingSound == 0 && playerLandingSoundPlayed[i] == 0) {
                    playerLandingSoundPlayed[i] = 1;
                    playedLandingSound = 1;
                    HuAudFXPlay(0x7F6);
                }
                playerCellModel->pos.y = 0.9 * playerCellModel->pos.y + 0.1 * characterGrid[playerDataByIndex[i]->gridCell].characterHeight;
                playerObjs[i]->trans.y = playerCellModel->pos.y - -16.666666666666668;
                playerObjs[i]->rot.y = 0.0f;
            }
            break;
        case 1004:
            if (phaseTimer <= 60) {
                boardExpansionProgress = phaseTimer / 60.0;
                boardExpansionProgress = 1.0f - (1.0f - boardExpansionProgress) * (1.0f - boardExpansionProgress);
                if (phaseTimer == 0) {
                    characterGrid[playerDataByIndex[0]->gridCell].characterHeight = -16.666666f;
                }
                for (i = 0; i < 9; i++) {
                    boardCellModel = &Hu3DData[boardObject->mdlId[i + 1]];
                    Hu3DModelPosSet(boardObject->mdlId[i + 1], (500.0 + 200.0f * boardExpansionProgress) * (characterGrid[i].gridColumn - 1),
                        0.9 * boardCellModel->pos.y + 0.1 * characterGrid[playerDataByIndex[0]->gridCell].characterHeight, (500.0 + 200.0f * boardExpansionProgress) * (characterGrid[i].gridRow - 1));
                    for (j = 0; j < 9; j++) {
                        if (i == playerDataByIndex[j]->gridCell) {
                            playerObjs[j]->trans.x = (500.0 + 200.0f * boardExpansionProgress) * (characterGrid[i].gridColumn - 1);
                            playerObjs[j]->trans.y = boardCellModel->pos.y - -16.666666666666668;
                            playerObjs[j]->trans.z = (500.0 + 200.0f * boardExpansionProgress) * (characterGrid[i].gridRow - 1);
                        }
                    }
                }
            }
            break;
    }
    CameraTransitionUpdate();
    if (minigamePhase == 1001 || minigamePhase == 1003) {
        windOffsetX = 0.0f;
        windOffsetY = 0.0f;
        if (windAnimation.windTimer % 2 >= 0) {
            windAnimation.windX = windAnimation.targetWindX;
            windAnimation.windZ = windAnimation.targetWindZ;
            do {
                windAnimation.targetWindX = 2.0 * (rand8() / 255.0) - 1.0;
                windAnimation.targetWindZ = 2.0 * (rand8() / 255.0) - 1.0;
                windAnimation.targetWindX = 0.0f;
                windDistanceSquared = (windAnimation.targetWindX - windAnimation.windX) * (windAnimation.targetWindX - windAnimation.windX)
                    + (windAnimation.targetWindZ - windAnimation.windZ) * (windAnimation.targetWindZ - windAnimation.windZ);
            } while (sqrtf(windDistanceSquared) < 0.75);
        }
        windAnimation.windFrame = (windAnimation.windFrame > 24) ? 24 : (windAnimation.windFrame > 0) ? windAnimation.windFrame : 0;
        windOffsetX = windAnimation.windX;
        windOffsetY = windAnimation.windZ;
        windOffsetX *= windAnimation.windFrame * 2;
        windOffsetY *= windAnimation.windFrame * 2;
        windAnimation.windTimer = !windAnimation.windTimer;
        windAnimation.windFrame = (windAnimation.windFrame > 24) ? 23 : (windAnimation.windFrame - 1 > 0) ? (windAnimation.windFrame - 1) : 0;
        Center.x = 0.0f;
        Center.y = 100.0f + windOffsetY;
    }
}

s16 WindParticleCreate(void)
{
    ANIMDATA *particleAnimation;
    void *particleData;
    s16 particleManagerId;
    s16 particleModelId;
    HU3DPARMANPARAM particleParameters = { 0x96, { 0x00, 0x00 }, 50.0f, 0.0f, 65.0f, { 0.0f, -0.195f, 0.0f }, 80.0f, 0.99f, 40.0f, 0.999f, 4,
        { { 0xFF, 0xFF, 0x20, 0xFF }, { 0xFF, 0xFF, 0x00, 0xFF }, { 0xFF, 0xFF, 0x80, 0xFF }, { 0xFF, 0xFF, 0xFF, 0xFF } },
        { { 0xFF, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 }, { 0xFF, 0xFF, 0xFF, 0x00 } } };

    playerParticleParams[particleParamIndex] = particleParameters;
    particleData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_BOARD, 107), HU_MEMNUM_OVL, HEAP_MODEL);
    particleAnimation = HuSprAnimRead(particleData);
    particleManagerId = Hu3DParManCreate(particleAnimation, 40, &playerParticleParams[particleParamIndex]);
    Hu3DParManAttrSet(particleManagerId, 8 | 0x100);
    Hu3DParManRotSet(particleManagerId, 0.0f, 0.0f, 0.0f);
    Hu3DParManVecSet(particleManagerId, 0.0f, 0.4f, 0.2f);
    particleModelId = Hu3DParManModelIDGet(particleManagerId);
    Hu3DModelLayerSet(particleModelId, 2);
    Hu3DParticleBlendModeSet(particleModelId, 0);
    return particleManagerId;
}

#if VERSION_PAL
#define FN45F8_CONST 452
#else
#define FN45F8_CONST 480
#endif

void MinigameStateUpdate(OMOBJ *stateObject)
{
    PanelPanicPlayerData *playerDataByIndex[9];
    float messageWindowSize[2];
    s32 playedEffectId;
    s32 inactivePlayerCount;
    s32 finishedPlayerCount;
    s32 lastActivePlayer;
    s32 remainingCellCount;
    s32 i;

    for (i = 0; i < 28; i++) {
        s32 cellEffectSoundIds[] = { 0x7F1, 0x7F2, 0x7F7, 0x7F8, 0x7F9, 0x7FA, 0x7FB, 0x7FC, 0x7FD, 0x7FE, 0x7FF, 0x800, 0x801, 0x802, 0x803, 0x804, 0x7FB, 0x7FC,
            0x7FD, 0x7FE, 0x7FF, 0x800, 0x801, 0x802, 0x803, 0x804, 0x7FB, 0x7FC };

        if (effectState[i].activeEffectCount > 3) {
            effectState[i].activeEffectCount = 3;
        }
        if (effectState[i].activeEffectCount > 0) {
            playedEffectId = HuAudFXPlay(cellEffectSoundIds[i]);
            effectState[i].activeEffectCount--;
        }
    }
    for (i = 0; i < 9; i++) {
        playerDataByIndex[i] = playerObjs[i]->data;
    }
    if (omSysExitReq != 0 && WipeStatGet() == 0) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        stateObject->objFunc = MinigameExit;
    }
    switch (minigamePhase) {
        case 1000:
            if (WipeStatGet() != 0) {
                break;
            }
            minigamePhase++;
            phaseTimer = 0;
            for (i = 0; i < 9; i++) {
                if (playerDataByIndex[i]->parMan != -1) {
                    Hu3DParManKill(playerDataByIndex[i]->parMan);
                    playerDataByIndex[i]->parMan = -1;
                }
            }
            break;
        case 1001:
            phaseTimer++;
            if (phaseTimer == 90 && windState < 0) {
                windState = MGSeqStartCreate();
            }
            if (phaseTimer >= 90 && windMusicSequence == -1 && (MGSeqStatGet(windState) & 0x10)) {
                windMusicSequence = HuAudSeqPlay(0x49);
            }
            if (phaseTimer >= 90 && MGSeqStatGet(windState) == 0 && windState >= 0) {
                minigamePhase = 1002;
                phaseTimer = 0;
            }
            break;
        case 1002:
            phaseTimer += (phaseTimer < 30);
            if (phaseTimer >= 60) {
                minigamePhase++;
                phaseTimer = 0;
            }
            break;
        case 1003:
            phaseTimer++;
            if (phaseTimer == (playerDataByIndex[1]->character == -1 ? 120 : 60) + 30) {
                minigamePhase++;
                if (playerDataByIndex[1]->character == -1) {
                    minigamePhase++;
                }
                phaseTimer = 0;
            }
            break;
        case 1004:
            phaseTimer++;
            if (phaseTimer == (REFRESH_RATE*3)/2) {
                minigamePhase++;
                phaseTimer = 0;
            }
            break;
        case 1005:
            if (phaseTimer == 0) {
                for (i = 0; i < 9; i++) {
                    if (characterGrid[playerDataByIndex[i]->gridCell].heightTimer != 0) {
                        playerDataByIndex[i]->playerState = 3;
                        playerDataByIndex[i]->stateTimer = 0;
                    }
                    else {
                        playerDataByIndex[i]->isFinished = 1;
                    }
                }
                HuWinMesMaxSizeGet(1, messageWindowSize, MAKE_MESSID(48, 37));
                messageWindow = HuWinCreate(-10000.0f, 400.0f, messageWindowSize[0], messageWindowSize[1], 1);
                HuWinBGTPLvlSet(messageWindow, 0.0f);
                HuWinMesSpeedSet(messageWindow, 0);
                HuWinMesSet(messageWindow, MAKE_MESSID(48, 37));
                playerMoveTimer = REFRESH_RATE*3;
            }
            phaseTimer++;
            if (phaseTimer == 30) {
                countdownSequence = MGSeqTimerCreate(10);
                MGSeqPosSet(countdownSequence, 489.6f, 64.0f);
            }
            if (phaseTimer >= 30 && countdownSequence != -1) {
                inactivePlayerCount = 0;
                for (i = 0; i < 9; i++) {
                    if (characterGrid[playerDataByIndex[i]->gridCell].heightTimer == 0 || playerDataByIndex[i]->playerState == 4 || playerDataByIndex[i]->playerState == 0) {
                        inactivePlayerCount++;
                    }
                }
                if (inactivePlayerCount >= 9 || playerMoveTimer == 0) {
                    MGSeqParamSet(countdownSequence, 2, -1);
                    countdownSequence = -1;
                }
                else {
                    playerMoveTimer -= (playerMoveTimer > 0);
                    MGSeqParamSet(countdownSequence, 1, (playerMoveTimer + (REFRESH_RATE-1)) / REFRESH_RATE);
                }
            }
            finishedPlayerCount = 0;
            for (i = 0; i < 9; i++) {
                if (characterGrid[playerDataByIndex[i]->gridCell].heightTimer == 0 || playerDataByIndex[i]->playerState == 0) {
                    finishedPlayerCount++;
                }
            }
            if (finishedPlayerCount >= 9) {
                minigamePhase++;
                phaseTimer = 0;
            }
            break;
        case 1006:
            if (phaseTimer == 1) {
                HuWinExCleanup(messageWindow);
            }
            phaseTimer++;
            if (phaseTimer == 3) {
                phaseTimer = 118;
            }
            if (phaseTimer == 223) {
                phaseTimer = 233;
            }
            if (phaseTimer == 120 && effectState->activeEffectCount < 3) {
                effectState->effectOffsets[effectState->activeEffectCount] = 0.0f;
                effectState->activeEffectCount++;
            }
            if (phaseTimer == 188 && effectState[1].activeEffectCount < 3) {
                effectState[1].effectOffsets[effectState[1].activeEffectCount] = 0.0f;
                effectState[1].activeEffectCount++;
            }
            if (phaseTimer == 360) {
                remainingCellCount = 0;
                lastActivePlayer = -1;
                for (i = 0; i < 9; i++) {
                    remainingCellCount += characterGrid[i].heightTimer;
                    lastActivePlayer = (characterGrid[playerDataByIndex[i]->gridCell].heightTimer != 0) ? i : lastActivePlayer;
                }
                if (remainingCellCount <= 1) {
                    winnerPlayer = (remainingCellCount == 0) ? 9 : lastActivePlayer;
                }
                if (winnerPlayer != -1) {
                    windState = MGSeqFinishCreate();
                    if (windMusicSequence != -1) {
                        HuAudSeqFadeOut(windMusicSequence, 100);
                    }
                }
            }
            if (phaseTimer >= FN45F8_CONST) {
                if (winnerPlayer != -1) {
                    if (MGSeqStatGet(windState) == 0 && phaseTimer >= 510) {
                        minigamePhase = 1007;
                        phaseTimer = 0;
                    }
                }
                else {
                    minigamePhase = 1005;
                    phaseTimer = 0;
                }
            }
            break;
        case 1007:
            phaseTimer++;
            if (phaseTimer == 1) {
                if (winnerPlayer != 9) {
                    if (winnerPlayer < 9) {
                        HuAudSStreamPlay(1);
                    }
                    else {
                        HuAudSStreamPlay(1);
                    }
                    if (winnerPlayer < 4 && playerDataByIndex[winnerPlayer]->playerCfg != -1) {
                        GWPlayerCoinWinSet(playerDataByIndex[winnerPlayer]->playerCfg, 10);
                    }
                }
                else if (winnerPlayer == 9) {
                    HuAudSStreamPlay(4);
                }
            }
            if (((REFRESH_RATE*7.0f)/2) + (winnerPlayer == 9 ? 60 : 0) < phaseTimer) {
                if (introSoundEffect != -1) {
                    HuAudFXStop(introSoundEffect);
                    introSoundEffect = -1;
                }
                windState = -1;
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                stateObject->objFunc = MinigameExit;
            }
            break;
    }
}

void BoardSceneInit(OMOBJ *boardObject)
{
    PanelPanicPlayerData *playerDataByIndex[9];
    s32 i;

    for (i = 0; i < 9; i++) {
        playerDataByIndex[i] = playerObjs[i]->data;
    }
    boardObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M458, 8));
    Hu3DModelAttrSet(boardObject->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DMotionSpeedSet(boardObject->mdlId[0], 0.1f);
    for (i = 0; i < 9; i++) {
        boardObject->mdlId[i + 1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M458, 10 + i));
        characterGrid[i].gridColumn = i % 3;
        characterGrid[i].gridRow = i / 3;
        characterGrid[i].characterHeight = 0.0f;
        characterGrid[i].heightTimer = 1;
        Hu3DModelPosSet(boardObject->mdlId[i + 1], 500.0 * (characterGrid[i].gridColumn - 1), -16.666666f, 500.0 * (characterGrid[i].gridRow - 1));
        Hu3DModelScaleSet(boardObject->mdlId[i + 1], 1.6666666f, 1.6666666f, 1.6666666f);
        Hu3DModelAttrSet(boardObject->mdlId[i + 1], HU3D_MOTATTR_PAUSE);
        Hu3DModelShadowMapSet(boardObject->mdlId[i + 1]);
        Hu3DModelLayerSet(boardObject->mdlId[i + 1], 1);
    }
    for (i = 0; i < 9; i++) {
        s32 unusedCellSpriteWidths[9] = { 288, 290 };
        s32 unusedCellSpriteHeights[9] = { 240, 140 };

        cellSprites[i] = espEntry(cellSpriteData[i], 0, 0);
        espDrawNoSet(cellSprites[i], 0);
        espAttrSet(cellSprites[i], HUSPR_ATTR_NOANIM | HUSPR_ATTR_DISPOFF);
        espBankSet(cellSprites[i], 0);
        espPriSet(cellSprites[i], 0x80 - i);
        espPosSet(cellSprites[i], 290.0f, 140.0f);
        espTPLvlSet(cellSprites[i], 1.0f);
    }
    boardObject->objFunc = BoardSceneUpdate;
}

s32 boardDecorationModelData[] = { DATA_MAKE_NUM(DATADIR_M458, 19), DATA_MAKE_NUM(DATADIR_M458, 20), DATA_MAKE_NUM(DATADIR_M458, 21),
    DATA_MAKE_NUM(DATADIR_M458, 22), DATA_MAKE_NUM(DATADIR_M458, 23), DATA_MAKE_NUM(DATADIR_M458, 24), DATA_MAKE_NUM(DATADIR_M458, 25),
    DATA_MAKE_NUM(DATADIR_M458, 26), DATA_MAKE_NUM(DATADIR_M458, 27), DATA_MAKE_NUM(DATADIR_M458, 28), DATA_MAKE_NUM(DATADIR_M458, 29) };

void PlayerObjectInit(OMOBJ *playerObject)
{
    PanelPanicPlayerData *playerData;
    s32 playerConfigByGroup[] = { 0, 1, 2, 3 };
    s32 matchedGroupCount;
    s32 characterIndex;
    s32 i;
    s32 j;

    matchedGroupCount = 0;
    playerData = playerObject->data;
    memset(playerData, 0, sizeof(*playerData));
    playerData->player = playerObject->work[0];
    if (playerData->player >= 4) {
        characterIndex = -1;
        for (i = 0; i <= playerData->player - 4; i++) {
            do {
                characterIndex++;
            } while (GWPlayerCfg[0].character == characterIndex || GWPlayerCfg[1].character == characterIndex || GWPlayerCfg[2].character == characterIndex
                || GWPlayerCfg[3].character == characterIndex);
        }
        playerData->character = (characterIndex == 8) ? -1 : characterIndex;
        playerData->padIdx = -1;
        playerData->playerCfg = -1;
        playerData->diff = 0;
    }
    else {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                if (i == GWPlayerCfg[j].group) {
                    playerConfigByGroup[matchedGroupCount] = j;
                    matchedGroupCount++;
                }
            }
        }
        playerData->player = playerObject->work[0];
        playerData->playerCfg = playerConfigByGroup[playerData->player];
        if (playerData->playerCfg == 4) {
            OSReport("m463:wrong player group\n");
            playerData->playerCfg = playerData->player;
        }
        playerData->character = GWPlayerCfg[playerData->playerCfg].character;
        playerData->padIdx = (playerData->character == -1 || GWPlayerCfg[playerData->playerCfg].iscom) ? -1 : GWPlayerCfg[playerData->playerCfg].pad_idx;
        playerData->diff = (playerData->padIdx != -1) ? 0 : GWPlayerCfg[playerData->playerCfg].diff;
    }
    playerData->playerState = 0;
    playerData->isMoving = 3;
    playerData->pendingMove = -1;
    playerData->effectTimer = -1;
    if (playerData->character != -1) {
        playerObject->mdlId[0] = CharModelCreate(playerData->character, 4);
    }
    else {
        playerObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_BKOOPA, 7));
    }
    omSetTra(playerObject, 0.0f, playerData->player == 0 ? 0 : 300, 0.0f);
    if (playerData->character != -1) {
        omSetSca(playerObject, 2.5f, 2.5f, 2.5f);
    }
    else {
        omSetSca(playerObject, 2.0f, 2.0f, 2.0f);
    }
    for (i = 0; i < 9; i++) {
        if (playerData->character != -1) {
            if (playerData->player < 4 || (i != 1 && i != 2)) {
                playerObject->mtnId[i] = CharMotionCreate(playerData->character, characterMotionData[i] + (characterMotionIsCharacterSpecific[i] ? playerData->character : 0));
            }
        }
        else if (koopaMotionData[i] != 0) {
            playerObject->mtnId[i] = Hu3DJointMotionFile(playerObject->mdlId[0], koopaMotionData[i]);
        }
    }
    Hu3DMotionSet(playerObject->mdlId[0], playerObject->mtnId[playerData->isMoving]);
    Hu3DMotionTimeSet(playerObject->mdlId[0], 10.0f);
    Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(playerObject->mdlId[0]);
    Hu3DModelAttrSet(playerObject->mdlId[0], 1U);
    Hu3DModelLayerSet(playerObject->mdlId[0], 1);
    CharModelLayerSetAll2(1);
    if (playerData->character != -1) {
        CharModelDataClose(playerData->character);
    }
    playerObject->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M458, 9));
    Hu3DModelAttrSet(playerObject->mdlId[1], HU3D_MOTATTR_LOOP);
    Hu3DModelPosSet(playerObject->mdlId[1], 0.0f, 400.0f, 0.0f);
    Hu3DModelAttrSet(playerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(playerObject->mdlId[1], 1);
    Hu3DMotionSpeedSet(playerObject->mdlId[1], 0.0f);
    for (i = 0; i < 9; i++) {
        if (playerData->player == 0) {
            playerObject->mdlId[i + 2] = Hu3DModelCreateFile(boardDecorationModelData[i]);
        }
        else {
            playerObject->mdlId[i + 2] = Hu3DModelLink(playerObjs[0]->mdlId[i + 2]);
        }
        Hu3DMotionTimeSet(playerObject->mdlId[i + 2], 10.0f);
        Hu3DMotionSpeedSet(playerObject->mdlId[i + 2], 0.0f);
        Hu3DModelAttrSet(playerObject->mdlId[i + 2], HU3D_ATTR_DISPOFF);
        Hu3DModelLayerSet(playerObject->mdlId[i + 2], 1);
    }
    particleParamIndex = playerData->player;
    playerData->parMan = WindParticleCreate();
    Hu3DParManPosSet(playerData->parMan, 10000.0f, -10000.0f, 10000.0f);
    playerObject->objFunc = PlayerMovementUpdate;
}

void MinigameControllerInit(OMOBJ *controllerObject)
{
    minigamePhase = 1000;
    phaseTimer = 0;
    unusedState48 = 0;
    unusedState44 = 0;
    introSoundEffect = HuAudFXPlay(0x6D7);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    controllerObject->objFunc = MinigameStateUpdate;
}

void ObjectSetup(void)
{
    OMOBJ *playerObject;
    s32 i;

    OSReport("******* M463 ObjectSetup *********\n");
    objectManager = omInitObjMan(600, 0x2000);
    omGameSysInit(objectManager);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 30.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    CZoom = 3200.0f;
    Center.x = 0.0f;
    Center.y = 0.0f;
    Center.z = -100.0f;
    CRot.x = -30.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    {
        Vec lightPosition = { 0.0f, 0.0f, 0.0f };
        Vec lightDirection = { 0.25f, -0.75f, -0.45f };
        GXColor lightColor = { 0xFF, 0xFF, 0xFF, 0xFF };

        lightId = Hu3DGLightCreateV(&lightPosition, &lightDirection, &lightColor);
    }
    Hu3DGLightStaticSet(lightId, 1);
    Hu3DGLightInfinitytSet(lightId);
    {
        Vec shadowPosition;
        Vec shadowUpVector;
        Vec shadowDepthVector;

        shadowPosition.x = 0.0f;
        shadowPosition.y = 10000.0f;
        shadowPosition.z = 0.0f;
        shadowUpVector.x = 0.0f;
        shadowUpVector.y = 1.0f;
        shadowUpVector.z = 0.0f;
        shadowDepthVector.x = 0.0f;
        shadowDepthVector.y = 0.0f;
        shadowDepthVector.z = 0.5f;
        Hu3DShadowCreate(12.0f, 9000.0f, 10010.0f);
        Hu3DShadowTPLvlSet(0.625f);
        Hu3DShadowPosSet(&shadowPosition, &shadowUpVector, &shadowDepthVector);
    }
    Hu3DShadowSizeSet(0xE0);
    windState = -1;
    winnerPlayer = -1;
    HuWinInit(1);
    boardObject = omAddObjEx(objectManager, 0x65, 10, 0, 1, BoardSceneInit);
    for (i = 0; i < 9; i++) {
        playerObject = playerObjs[i] = omAddObjEx(objectManager, 100, 11, 9, 0, PlayerObjectInit);
        playerObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PanelPanicPlayerData), HU_MEMNUM_OVL);
        playerObject->work[0] = i;
    }
    viewObject = omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutView);
    exitObject = omAddObjEx(objectManager, 103, 0, 0, -1, MinigameControllerInit);
}

void MinigameExit(OMOBJ *exitObject)
{
    PanelPanicPlayerData *playerDataByIndex[9];
    void *unusedPlayerData;
    s32 playerIndex;
    s32 cleanupPlayerIndex;

    for (playerIndex = 0; playerIndex < 9; playerIndex++) {
        playerDataByIndex[playerIndex] = playerObjs[playerIndex]->data;
        if (playerDataByIndex[playerIndex]->parMan != -1) {
            Hu3DParManKill(playerDataByIndex[playerIndex]->parMan);
            playerDataByIndex[playerIndex]->parMan = -1;
        }
    }
    if (WipeStatGet() == 0) {
        for (cleanupPlayerIndex = 0; cleanupPlayerIndex < 9; cleanupPlayerIndex++) {
            unusedPlayerData = playerObjs[cleanupPlayerIndex]->data;
        }
        if (windState >= 0) {
            MGSeqKill(windState);
        }
        MGSeqKillAll();
        HuAudAllStop();
        omOvlReturnEx(1, 1);
    }
}

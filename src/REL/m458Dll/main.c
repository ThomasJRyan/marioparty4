#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/esprite.h"
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

#include "game/frand.h"
#include "version.h"

extern s32 rand8(void);

typedef struct CameraKeyframe {
    /* 0x00 */ float zoom;
    /* 0x04 */ float centerX;
    /* 0x08 */ float centerY;
    /* 0x0C */ float centerZ;
    /* 0x10 */ float rotationX;
    /* 0x14 */ float rotationY;
} CameraKeyframe; /* size = 0x18 */

typedef struct EffectSoundQueueEntry {
    /* 0x00 */ s32 effectCount;
    /* 0x04 */ float effectTimers[3];
} EffectSoundQueueEntry; /* size = 0x10 */

typedef struct BoardSpaceState {
    /* 0x00 */ s32 boardColumn;
    /* 0x04 */ s32 boardRow;
    /* 0x08 */ float heightOffset;
    /* 0x0C */ s32 isAvailable;
} BoardSpaceState; /* size = 0x10 */

typedef struct WindState {
    /* 0x00 */ s32 windUpdatePhase;
    /* 0x04 */ s32 windTransitionTime;
    /* 0x08 */ float windX;
    /* 0x0C */ float windZ;
    /* 0x10 */ float targetWindX;
    /* 0x14 */ float targetWindZ;
} WindState; /* size = 0x18 */

typedef struct PlayerWork {
    s32 teamId;
    s32 playerIndex;
    s32 characterId;
    s32 controllerIndex;
    s32 difficulty;
    s32 stateTimer;
    s32 playerState;
    s32 stateFrame;
    s32 motionIndex;
    s32 requestedMotionIndex;
    float movementMagnitude;
    float velocityX;
    char unreferencedBytes[4];
    float velocityZ;
    s32 boardSpaceIndex;
    s32 boardColumn;
    s32 boardRow;
} PlayerWork; /* size = 0x44 */

s16 WindParticleCreate();
void MinigameCleanup(OMOBJ *object);

s32 playerMarkerModelFiles[2] = { 0x570025, 0x570026 };
s32 playerMarkerSprites[2] = { 0, 0 };
s32 characterMotionFiles[0xA] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x05),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x06),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x36),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x2F),
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x1A),
    0x570000,
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17),
};
u32 characterSpecificMotionFlags[0xA] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 };
u32 fallbackMotionFiles[0xA] = { 0x30008, 0, 0, 0x3000A, 0, 0, 0x57002A, 0x570028, 0x570029, 0x570027 };
u8 characterMotionLoopFlags[0xA] = { 0, 0, 0, 1, 1, 1, 1, 0, 1, 1 };
u8 fallbackMotionLoopFlags[0xA] = { 0, 0, 0, 1, 1, 1, 1, 0, 1, 1 };
s32 characterMotionBlendFrames[0xA] = { 8, 8, 8, 0, 0, 0, 8, 8, 8, 8 };
s32 motionTransitionFrames[0xA] = { 0, 0, 0, 0, 3, 0, 0, 0, 0, 0 };
s32 fallbackMotionBlendFrames[0xA] = { 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };
WindState windState = { 0, 0, 0.0f, 0.0f, 0.0f, 0.0f };
s16 minigameSequenceId = -1;
s16 countdownSequenceId = -1;
s32 backgroundMusicId = -1;
s32 startSoundId = -1;
s16 windParticleManagerId[0x12] = {
    -1,
    0,
    -0x3D4C,
    0,
    0,
    0,
    0x4403,
    0x4000,
    0x4334,
    0,
    0x42B4,
    0,
    0,
    0,
    0x4403,
    0x4000,
    0x4334,
    0,
};
s32 movementSoundTimer = -1;

BoardSpaceState boardSpaces[9];
EffectSoundQueueEntry effectSoundQueue[0x1C];
OMOBJ *controllerObject;
OMOBJ *outOfViewObject;
OMOBJ *unusedObject;
OMOBJ *playerObjects;
OMOBJ *sceneObjects;
s32 gameStateTimer;
s32 gameState;
s32 unusedStateAC;
s32 unusedStateA8;
s32 unusedStateA4;
HUPROCESS *objectManager;
s32 directionalLightId;
s32 winningTeam;
s32 teamZeroRemainingMoves;
s32 teamOneRemainingMoves;
s32 swingAnimationFrame;
HU3DPARMANPARAM windParticleParametersWork;
float unusedFloat38;
float cameraStartRotationY;
float cameraStartCenterZ;
float cameraStartCenterX;
float cameraTransitionProgress;
s32 cameraTransitionDirection;
s32 hasPlayedLandingSound;
s32 playerSpinAngle;
s32 selectedBoardSpace;
s32 isSinglePlayer;
s32 isMessageActive;
s16 messageWindowId;
s32 unusedState8;
s32 sceneUpdateFrame;
s32 countdownTimer;

void CameraDebugInputUpdate(void)
{
    if ((HuPadBtnDown[0] & PAD_BUTTON_B) != 0) {
        Center.x = playerObjects->trans.x;
        Center.y = playerObjects->trans.y;
        Center.z = playerObjects->trans.z;
    }
    Center.x += 10.0 * (((HuPadBtn[0] & PAD_BUTTON_RIGHT) != 0) - ((HuPadBtn[0] & PAD_BUTTON_LEFT) != 0));
    Center.z -= 10.0 * (((HuPadBtn[0] & PAD_BUTTON_UP) != 0) - ((HuPadBtn[0] & PAD_BUTTON_DOWN) != 0));
    Center.y += (30.0 * ((HuPadTrigL[0] - HuPadTrigR[0]) / 128.0));
    CZoom += 25.0 * (((HuPadBtn[0] & PAD_BUTTON_X) != 0) - ((HuPadBtn[0] & PAD_BUTTON_Y) != 0));
    CRot.y += HuPadSubStkX[0] / 16.0;
    CRot.x += HuPadSubStkY[0] / -16.0;
    print8(0x40, 0x188, 1.0f, "%f,%f,%f", Center.x, Center.y, Center.z);
    print8(0x40, 0x198, 1.0f, "%f,%f,%f", CRot.x, CRot.y, CRot.z);
    print8(0x40, 0x1A8, 1.0f, "%f", CZoom);
}

s32 windSoundHandle = -1;

void PlayerCharacterUpdate(OMOBJ *object)
{
    float worldVelocityX;
    float worldVelocityZ;
    float movementSpeed;
    float effectScale;
    float markerScale;
    float effectFade;
    float effectAlpha;
    float targetVelocityX;
    float targetVelocityZ;
    float pulseScaleRange;
    float minimumPulseScale;

    PlayerWork *playerWork;
    HU3DMODEL *effectModel;
    s16 requestedMotionIndex;
    PlayerWork *opponentWork;
    s32 stateAnimationFrame;
    s32 horizontalMoveDirection;
    s32 verticalMoveDirection;
    HU3DMODEL *animatedModel;
    s32 confirmButtonPressed;
    s32 scanColumn;
    s32 scanRow;
    s32 candidateColumn;
    s32 candidateRow;
    s32 scanColumnStart;

    Mtx cameraRotationMatrix;
    float countdownMessageSize[2];
    float resultMessageSize[2];
    s32 scanColumnEnd;
    s32 scanRowStart;
    s32 scanRowEnd;
    s32 unusedHorizontalOffset;
    s32 unusedVerticalOffset;
    float markerSlideProgress;
    s32 voiceSoundId;
    HU3DMODEL *sceneModel;
    float modelMotionOffset;
    float effectMotionOffset;
    s32 spaceSearchAttempt;
    s32 previousSelectedSpace;

    playerWork = object->data;
    opponentWork = (&playerObjects)[1 - object->work[0]]->data;
    targetVelocityX = 0.0f;
    targetVelocityZ = 0.0f;
    movementSpeed = 0.0f;
    confirmButtonPressed = 0;
    requestedMotionIndex = -1;
    playerWork->velocityX = playerWork->velocityZ = 0.0f;
    if ((gameState == (playerWork->teamId == 0 ? 0x3F1 : 0x3F2)) && (playerWork->playerState == 3) && (playerWork->stateFrame >= 0x168)) {
        if (playerWork->stateFrame == 0x168) {
            playerWork->stateTimer = rand8() % 15;
        }
        if (playerWork->controllerIndex != -1) {
            confirmButtonPressed = (HuPadBtnDown[playerWork->controllerIndex] & 0x100) != 0 ? 1 : 0;
        }
        else {
            playerWork->stateTimer += playerWork->stateTimer < 0x10000;
            if (playerWork->stateTimer == 0x1E) {
                confirmButtonPressed = 1;
            }
        }
    }
    if (gameState == (playerWork->teamId == 0 ? 0x3ED : 0x3EF)) {
        horizontalMoveDirection = 0;
        verticalMoveDirection = 0;
        if (playerWork->controllerIndex != -1) {
            horizontalMoveDirection = HuPadStkX[playerWork->controllerIndex] < -0x14 ? -1 : (HuPadStkX[playerWork->controllerIndex] > 0x14 ? 1 : 0);
            verticalMoveDirection = -HuPadStkY[playerWork->controllerIndex] < -0x14 ? -1 : (-HuPadStkY[playerWork->controllerIndex] > 0x14 ? 1 : 0);
            confirmButtonPressed = (HuPadBtnDown[playerWork->controllerIndex] & 0x100) != 0 ? 1 : 0;
        }
        else {
            playerWork->stateTimer += playerWork->stateTimer < 0x10000;
            if (playerWork->stateTimer == 0x1E) {
                teamZeroRemainingMoves = rand8() % (playerWork->teamId + 2);
                teamOneRemainingMoves = rand8() % (playerWork->teamId + 2);
            }
            if (fabs((500.0 * playerWork->boardColumn) - object->trans.x) < 10.0) {
                horizontalMoveDirection = teamZeroRemainingMoves > 0 ? (playerWork->teamId == 0 ? (((rand8() % 2) * 2) - 1) : -1) : 0;
                teamZeroRemainingMoves -= teamZeroRemainingMoves > 0;
            }
            if (fabs((500.0 * playerWork->boardRow) - object->trans.z) < 10.0) {
                verticalMoveDirection = teamOneRemainingMoves > 0 ? (playerWork->teamId == 0 ? (((rand8() % 2) * 2) - 1) : -1) : 0;
                teamOneRemainingMoves -= teamOneRemainingMoves > 0;
            }
            if ((playerWork->stateTimer >= 0x2D) && ((playerWork->stateTimer % 30) == 0xF) && (teamZeroRemainingMoves == 0) && (teamOneRemainingMoves == 0)) {
                confirmButtonPressed = 1;
            }
        }
        if (gameStateTimer >= 0x32) {
            horizontalMoveDirection = verticalMoveDirection = 0;
        }
        if (playerWork->teamId == 1) {
            unusedHorizontalOffset = 0;
            unusedVerticalOffset = 0;
            if (fabs((500.0 * playerWork->boardColumn) - object->trans.x) < 10.0) {
                scanColumnStart = scanColumnEnd = object->trans.x < -250.0 ? -1 : (object->trans.x > 250.0 ? 1 : 0);
            }
            else {
                scanColumnStart = (object->trans.x < 0.0f ? -1 : 0);
                scanColumnEnd = scanColumnStart + 1;
            }
            if (fabs((500.0 * playerWork->boardRow) - object->trans.z) < 10.0) {
                scanRowStart = scanRowEnd = object->trans.z < -250.0 ? -1 : (object->trans.z > 250.0 ? 1 : 0);
            }
            else {
                scanRowStart = object->trans.z < 0.0f ? -1 : 0;
                scanRowEnd = scanRowStart + 1;
            }
            for (scanColumn = scanColumnStart; scanColumn <= scanColumnEnd; scanColumn++) {
                for (scanRow = scanRowStart; scanRow <= scanRowEnd; scanRow++) {
                    if (opponentWork->boardSpaceIndex == 1 + scanColumn + horizontalMoveDirection + (1 + scanRow + verticalMoveDirection) * 3) {
                        horizontalMoveDirection = verticalMoveDirection = 0;
                    }
                    if (opponentWork->boardSpaceIndex == 1 + scanColumn + horizontalMoveDirection + (scanRow + 1) * 3) {
                        horizontalMoveDirection = 0;
                    }
                    if (opponentWork->boardSpaceIndex == 1 + scanColumn + (1 + scanRow + verticalMoveDirection) * 3) {
                        verticalMoveDirection = 0;
                    }
                }
            }
        }
        if ((horizontalMoveDirection * ((500.0 * playerWork->boardColumn) - object->trans.x)) < 0.0) {
            playerWork->boardColumn = playerWork->boardColumn + horizontalMoveDirection;
        }
        if ((fabs((500.0 * playerWork->boardColumn) - object->trans.x) < 10.0) && (horizontalMoveDirection != 0) && ((playerWork->boardColumn + horizontalMoveDirection) >= -1)
            && ((playerWork->boardColumn + horizontalMoveDirection) <= 1)) {
            playerWork->boardColumn = playerWork->boardColumn + horizontalMoveDirection;
        }
        targetVelocityX = (((500.0 * playerWork->boardColumn) - object->trans.x) / 0.25) / 1.75;
        if ((verticalMoveDirection * ((500.0 * playerWork->boardRow) - object->trans.z)) < 0.0) {
            playerWork->boardRow = playerWork->boardRow + verticalMoveDirection;
        }
        if ((fabs((500.0 * playerWork->boardRow) - object->trans.z) < 10.0) && (verticalMoveDirection != 0) && ((playerWork->boardRow + verticalMoveDirection) >= -1)
            && ((playerWork->boardRow + verticalMoveDirection) <= 1)) {
            playerWork->boardRow = playerWork->boardRow + verticalMoveDirection;
        }
        targetVelocityZ = (((500.0 * playerWork->boardRow) - object->trans.z) / -0.25) / 1.75;
    }
    if (((playerWork->teamId != 1) || (playerWork->characterId != -1)) && (playerWork->playerState == 0)) {
        if ((gameState > (playerWork->teamId == 0 ? 0x3E9 : 0x3EE)) && (gameState < 0x3F3)) {
            worldVelocityX = 0.25 * targetVelocityX;
            worldVelocityZ = 0.25 * -targetVelocityZ;
            {
                Vec movementVector = { 0.0f, 0.0f, 0.0f };
                movementVector.x = worldVelocityX;
                movementVector.z = worldVelocityZ;
                MTXRotRad(cameraRotationMatrix, 0x79, 0.017453292f * CRot.y);
                MTXMultVec(cameraRotationMatrix, &movementVector, &movementVector);
                worldVelocityX = movementVector.x;
                worldVelocityZ = movementVector.z;
            }
            movementSpeed = sqrtf((worldVelocityX * worldVelocityX) + (worldVelocityZ * worldVelocityZ));
            if (movementSpeed >= 0.5f) {
                if (movementSpeed < 10.0f) {
                    playerWork->velocityX = worldVelocityX;
                    playerWork->velocityZ += worldVelocityZ;
                }
                else {
                    worldVelocityX = worldVelocityX / movementSpeed;
                    worldVelocityZ = worldVelocityZ / movementSpeed;
                    movementSpeed = 10.0f;
                    playerWork->velocityX = worldVelocityX * movementSpeed;
                    playerWork->velocityZ = worldVelocityZ * movementSpeed;
                }
                playerWork->movementMagnitude = movementSpeed;
                object->rot.y = atan2d(playerWork->velocityX, playerWork->velocityZ);
                if (playerWork->movementMagnitude >= 8.0f) {
                    requestedMotionIndex = 2;
                }
                else {
                    requestedMotionIndex = 2;
                }
            }
            else {
                requestedMotionIndex = 0;
            }
        }
    }
    object->trans.x += 1.75f * playerWork->velocityX;
    object->trans.z += 1.75f * playerWork->velocityZ;
    switch (gameState) {
        case 0x3E9:
        case 0x3EE:
            if (gameState == (playerWork->teamId == 0 ? 0x3E9 : 0x3EE)) {
                if (gameStateTimer == 0) {
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_ATTR_DISPOFF);
                    if (playerWork->teamId == 0) {
                        object->trans.x = 0.0f;
                        object->trans.z = 0.0f;
                    }
                    else if (playerWork->characterId != -1) {
                        playerWork->boardColumn = -2;
                        for (candidateRow = 2; candidateRow >= 0; candidateRow--) {
                            for (candidateColumn = 2; candidateColumn >= 0; candidateColumn--) {
                                if (opponentWork->boardSpaceIndex != (candidateRow + (candidateColumn * 3))) {
                                    playerWork->boardColumn = candidateColumn - 1;
                                    playerWork->boardRow = candidateRow - 1;
                                    object->trans.x = 500.0 * playerWork->boardColumn;
                                    object->trans.z = 500.0 * playerWork->boardRow;
                                    break;
                                }
                            }
                            if (playerWork->boardColumn != -2) {
                                break;
                            }
                        }
                    }
                    else {
                        do {
                            playerWork->boardSpaceIndex = rand8() % 9;
                        } while (opponentWork->boardSpaceIndex == playerWork->boardSpaceIndex);
                        playerWork->boardColumn = (playerWork->boardSpaceIndex % 3) - 1;
                        playerWork->boardRow = (playerWork->boardSpaceIndex / 3) - 1;
                        object->trans.x = 500.0 * playerWork->boardColumn;
                        object->trans.z = 500.0 * playerWork->boardRow;
                    }
                }
                if (gameStateTimer <= 0x1E) {
                    if (playerWork->characterId == -1) {
                        Hu3DMotionTimeSet(object->mdlId[0], 138.0f);
                    }
                    object->trans.y = 2400.0 * (1.0 - (gameStateTimer / 30.0));
                }
                if ((playerWork->characterId != -1) && (gameStateTimer == 0x1E)) {
                    requestedMotionIndex = 4;
                }
                if (gameStateTimer == ((playerWork->characterId != -1 ? 0xA : 0x3C) + 0x1E)) {
                    requestedMotionIndex = 0;
                }
                if (gameStateTimer == 0x1E) {
                    windState.windTransitionTime += 0x32;
                }
                if (gameStateTimer == 0x1E) {
                    HuAudFXPlay(0x33D);
                }
            }
            break;
        case 0x3EB:
            if ((gameStateTimer == 0) && (playerWork->teamId == 0)) {
                playerWork->playerState = 1;
                playerWork->stateFrame = 0;
            }
            break;
        case 0x3ED:
        case 0x3EF:
            if (gameState == (playerWork->teamId == 0 ? 0x3ED : 0x3EF)) {
                if (gameStateTimer == 0) {
                    countdownTimer = REFRESH_RATE * 5;
                    countdownSequenceId = MGSeqCreate(1, 0xA, -1, -1);
                    HuWinMesMaxSizeGet(1, countdownMessageSize, 0x280019);
                    messageWindowId = HuWinCreate(-10000.0f, 400.0f, countdownMessageSize[0], countdownMessageSize[1], 1);
                    HuWinBGTPLvlSet(messageWindowId, 0.0f);
                    HuWinMesSpeedSet(messageWindowId, 0);
                    HuWinMesSet(messageWindowId, 0x280019);
                }
                if (countdownSequenceId != -1) {
                    MGSeqParamSet(countdownSequenceId, 1, (countdownTimer + REFRESH_RATE - 2) / REFRESH_RATE);
                }
                countdownTimer--;
                if ((confirmButtonPressed != 0) || (countdownTimer == 0)) {
                    gameStateTimer = 0x32;
                    MGSeqKill(countdownSequenceId);
                    countdownSequenceId = -1;
                    HuWinExCleanup(messageWindowId);
                }
                if (gameStateTimer == 0x32) {
                    if (fabs((500.0 * playerWork->boardColumn) - object->trans.x) < 1.0) {
                        if (fabs((500.0 * playerWork->boardRow) - object->trans.z) < 1.0) {
                            gameStateTimer = 0x3C;
                            object->rot.y = 0.0f;
                            playerWork->boardSpaceIndex = 1 + playerWork->boardColumn + ((playerWork->boardRow + 1) * 3);
                        }
                    }
                }
            }
            break;
        case 0x3F1:
        case 0x3F2:
            if ((gameState == (playerWork->teamId == 0 ? 0x3F1 : 0x3F2)) && (gameStateTimer == 0) && (playerWork->characterId != -1)) {
                HuWinMesMaxSizeGet(1, resultMessageSize, 0x300025);
                messageWindowId = HuWinCreate(-10000.0f, 400.0f, resultMessageSize[0], resultMessageSize[1], 1);
                HuWinBGTPLvlSet(messageWindowId, 0.0f);
                HuWinMesSpeedSet(messageWindowId, 0);
                HuWinMesSet(messageWindowId, 0x300025);
            }
            break;
        case 0x3F3:
            if (winningTeam != playerWork->teamId) {
                if (gameStateTimer == 1) {
                    requestedMotionIndex = 7;
                }
                if (gameStateTimer == 0x78) {
                    Hu3DModelAttrReset(sceneObjects->mdlId[playerWork->boardSpaceIndex + 4], HU3D_MOTATTR_PAUSE);
                    if (effectSoundQueue[0].effectCount < 3) {
                        effectSoundQueue[0].effectTimers[effectSoundQueue[0].effectCount] = 0.0f;
                        effectSoundQueue[0].effectCount++;
                    }
                }
                if (gameStateTimer == 0x99) {
                    Hu3DModelAttrSet(sceneObjects->mdlId[playerWork->boardSpaceIndex + 4], HU3D_MOTATTR_PAUSE);
                }
                if (gameStateTimer == 0xB7) {
                    Hu3DModelAttrReset(sceneObjects->mdlId[playerWork->boardSpaceIndex + 4], HU3D_MOTATTR_PAUSE);
                }
                if ((gameStateTimer == 0xBC) && (effectSoundQueue[1].effectCount < 3)) {
                    effectSoundQueue[1].effectTimers[effectSoundQueue[1].effectCount] = 0.0f;
                    effectSoundQueue[1].effectCount++;
                }
                if (gameStateTimer == 0xD2) {
                    requestedMotionIndex = 8;
                    espAttrReset(playerMarkerSprites[1], HUSPR_ATTR_DISPOFF);
                    if (effectSoundQueue[(playerWork->characterId != -1 ? playerWork->characterId : 8) + 2].effectCount < 3) {
                        effectSoundQueue[(playerWork->characterId != -1 ? playerWork->characterId : 8) + 2]
                            .effectTimers[effectSoundQueue[(playerWork->characterId != -1 ? playerWork->characterId : 8) + 2].effectCount]
                            = 0.0f;
                        effectSoundQueue[(playerWork->characterId != -1 ? playerWork->characterId : 8) + 2].effectCount++;
                    }
                    if (backgroundMusicId != -1) {
                        HuAudSeqFadeOut(backgroundMusicId, 0x64);
                        backgroundMusicId = -1;
                    }
                }
                if ((gameStateTimer >= 0xD2) && (gameStateTimer <= 0xD6)) {
                    s32 characterNameVerticalOffsets[9] = { 0, -12, -36, 0, 0, -12, -36, -48, 0 };
                    markerSlideProgress = (gameStateTimer - 0xD2) / 4.0;
                    markerScale = (gameStateTimer - 0xD2) / 4.0;
                    markerScale = markerScale > 1.0f ? 1.0f : markerScale;
                    espPosSet(playerMarkerSprites[1], 290.0f, (64.0f * (1.0f - markerSlideProgress)) + (characterNameVerticalOffsets[(playerWork->characterId != -1 ? playerWork->characterId : 8)] + 0x8C));
                    espScaleSet(playerMarkerSprites[1], markerScale, markerScale);
                }
                if (gameStateTimer == 0x10E) {
                    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_PAUSE);
                    espAttrSet(playerMarkerSprites[1], HUSPR_ATTR_DISPOFF);
                }
                if (gameStateTimer == 0x122) {
                    voiceSoundId = HuAudFXPlay((playerWork->characterId != -1 ? playerWork->characterId : 8) + 0x800);
                    HuAudFXFadeOut(voiceSoundId, 0x9C4);
                }
                if (gameStateTimer >= 0x13B) {
                    object->trans.y -= 77.0f;
                }
            }
            else if ((gameStateTimer == 0x3C) && (windParticleManagerId[0] != -1)) {
                Hu3DParManKill(windParticleManagerId[0]);
                windParticleManagerId[0] = -1;
            }
            break;
        case 0x3F4:
            if (playerWork->teamId == winningTeam) {
                if ((gameStateTimer == 1) && (playerWork->characterId == -1)) {
                    HuAudFXPlay(0x3A);
                }
                requestedMotionIndex = 9;
            }
            break;
    }
    switch (playerWork->playerState) {
        case 0:
            playerWork->stateFrame++;
            break;
        case 1:
            playerWork->stateFrame++;
            if (playerWork->stateFrame <= 0x3C) {
                requestedMotionIndex = 5;
                object->rot.y = -180.0 * (playerWork->stateFrame / 60.0);
            }
            else {
                Hu3DMotionTimeSet(object->mdlId[0], 120.0f);
                requestedMotionIndex = 0;
                playerWork->playerState = 0;
                playerWork->stateFrame = 0;
            }
            break;
        case 2:
            playerWork->stateFrame++;
            break;
        case 3:
            if (playerWork->stateFrame == 0) {
                movementSoundTimer = 6;
            }
            if (playerWork->stateFrame < 0x168) {
                animatedModel = &Hu3DData[sceneObjects->mdlId[3]];
                if (playerWork->stateFrame == 0) {
                    Hu3DModelAttrReset(sceneObjects->mdlId[3], HU3D_ATTR_DISPOFF);
                    Hu3DModelPosSet(sceneObjects->mdlId[3], object->trans.x, 625.0f + object->trans.y, object->trans.z);
                    Hu3DModelScaleSet(sceneObjects->mdlId[3], 0.1f, 0.1f, 0.1f);
                    Hu3DModelTPLvlSet(sceneObjects->mdlId[3], 1.0f);
                    playerSpinAngle = 0;
                    HuAudFXPlay(0x7F4);
                    windSoundHandle = HuAudFXPlay(0x7F3);
                }
                animatedModel->rot.y = playerSpinAngle % 360;
                playerSpinAngle += 0x19;
                if (playerWork->stateFrame < 0xB4) {
                    minimumPulseScale = 0.0f;
                    pulseScaleRange = 1.0f;
                }
                else {
                    minimumPulseScale = 1.0f;
                    pulseScaleRange = 0.5f;
                }
                animatedModel->scale.x = minimumPulseScale + (pulseScaleRange * sind((float)(playerWork->stateFrame % 180)));
                animatedModel->scale.y = animatedModel->scale.z = animatedModel->scale.x;
                if (playerWork->stateFrame < 0xB4) {
                    playerWork->stateFrame += 6;
                }
                else {
                    playerWork->stateFrame = playerWork->stateFrame + 0x12;
                }
                if (playerWork->stateFrame >= 0x168) {
                    playerWork->stateFrame = 0x168;
                    Hu3DModelRotSet(sceneObjects->mdlId[3], 0.0f, 0.0f, 0.0f);
                }
            }
            else {
                sceneModel = &Hu3DData[sceneObjects->mdlId[3]];
                playerWork->stateFrame = playerWork->stateFrame + 4;
                if (playerWork->stateFrame > 0x2D0) {
                    playerWork->stateFrame -= 0x168;
                }
                sceneModel->pos.y += (0.20000000298023224 * sind((playerWork->stateFrame - 0x168)));
                if (confirmButtonPressed != 0) {
                    gameStateTimer = 0x32;
                    playerWork->playerState = 4;
                    playerWork->stateFrame = 0;
                    requestedMotionIndex = 6;
                }
            }
            break;
        case 4:
            playerWork->stateFrame++;
            if (playerWork->stateFrame >= 0x1B) {
                effectModel = &Hu3DData[sceneObjects->mdlId[3]];
                stateAnimationFrame = playerWork->stateFrame - 0x1B;
                if (stateAnimationFrame == 0) {
                    if (playerWork->playerIndex != -1) {
                        omVibrate(playerWork->playerIndex, 0xC, 4, 2);
                    }
                    HuWinExCleanup(messageWindowId);
                    movementSoundTimer = -1;
                    if (windSoundHandle != -1) {
                        HuAudFXStop(windSoundHandle);
                        windSoundHandle = -1;
                    }
                    HuAudFXPlay(0x7F5);
                }
                if (stateAnimationFrame == 8) {
                    if (windParticleManagerId[0] == -1) {
                        windParticleManagerId[0] = WindParticleCreate();
                        Hu3DParManPosSet(windParticleManagerId[0], effectModel->pos.x, effectModel->pos.y, effectModel->pos.z);
                    }
                    Hu3DModelAttrSet(sceneObjects->mdlId[3], HU3D_ATTR_DISPOFF);
                }
                else if (stateAnimationFrame > 8) {
                    if (playerWork->motionIndex == 6) {
                        if (Hu3DMotionTimeGet(object->mdlId[0]) >= Hu3DMotionMaxTimeGet(object->mdlId[0])) {
                            requestedMotionIndex = 0;
                        }
                    }
                    if (stateAnimationFrame <= 0x1C) {
                        effectScale = (stateAnimationFrame - 8) / 10.0;
                        modelMotionOffset = ((stateAnimationFrame - 8) * 0x12);
                        effectScale = effectScale > 1.0f ? 2.5 : 2.5 * effectScale;
                        Hu3DModelAttrReset(sceneObjects->mdlId[selectedBoardSpace + 0xD], HU3D_ATTR_DISPOFF);
                        Hu3DModelPosSet(sceneObjects->mdlId[selectedBoardSpace + 0xD], effectModel->pos.x, effectModel->pos.y, effectModel->pos.z);
                        Hu3DModelScaleSet(sceneObjects->mdlId[selectedBoardSpace + 0xD], effectScale, effectScale, effectScale);
                    }
                    if ((stateAnimationFrame >= 0x2D) && (stateAnimationFrame <= 0x41)) {
                        effectFade = (stateAnimationFrame - 0x2D) / 5.0;
                        effectMotionOffset = (stateAnimationFrame - 0x2D) * 0x12;
                        effectFade = effectFade > 1.0f ? 1.0f : effectFade;
                        effectFade = 0.0;
                        Hu3DModelAttrReset(sceneObjects->mdlId[selectedBoardSpace + 0xD], HU3D_ATTR_DISPOFF);
                        Hu3DModelPosSet(sceneObjects->mdlId[selectedBoardSpace + 0xD], effectModel->pos.x, effectModel->pos.y - (30.0f * effectFade), effectModel->pos.z);
                        playerWork->stateFrame += 5;
                    }
                    if (stateAnimationFrame == 0x62) {
                        Hu3DModelAttrSet(sceneObjects->mdlId[selectedBoardSpace + 0xD], HU3D_ATTR_DISPOFF);
                        selectedBoardSpace = selectedBoardSpace >= 9 ? opponentWork->boardSpaceIndex : selectedBoardSpace;
                        if (selectedBoardSpace == opponentWork->boardSpaceIndex) {
                            winningTeam = playerWork->teamId;
                        }
                        else if (selectedBoardSpace == playerWork->boardSpaceIndex) {
                            winningTeam = opponentWork->teamId;
                        }
                        playerWork->playerState = winningTeam == -1 ? 5 : 0;
                        playerWork->stateFrame = 0;
                    }
                }
                else {
                    if (stateAnimationFrame < 4) {
                        effectModel->pos.y += 10.0f + (0.5f * stateAnimationFrame);
                        effectModel->scale.x += 0.10000000149011612 * sind((4.0f * stateAnimationFrame));
                        effectModel->scale.y = effectModel->scale.z = effectModel->scale.x;
                    }
                    else {
                        effectAlpha = (0xFF - (stateAnimationFrame << 5));
                        effectAlpha = effectAlpha * 0.003921569f; // 1 / 255
                        Hu3DModelTPLvlSet(sceneObjects->mdlId[3], effectAlpha);
                    }
                    effectModel->pos.y += -0.49f * stateAnimationFrame * stateAnimationFrame;
                }
            }
            break;
        case 5:
            if (playerWork->stateFrame == 0) {
                Hu3DModelAttrReset(sceneObjects->mdlId[selectedBoardSpace + 4], HU3D_MOTATTR_PAUSE);
                boardSpaces[selectedBoardSpace].isAvailable = 0;
                if (effectSoundQueue[0].effectCount < 3) {
                    effectSoundQueue[0].effectTimers[effectSoundQueue->teamId] = 0.0f;
                    effectSoundQueue[0].effectCount++;
                }
            }
            playerWork->stateFrame++;
            if ((playerWork->stateFrame == 0x3C) && (*windParticleManagerId != -1)) {
                Hu3DParManKill(*windParticleManagerId);
                windParticleManagerId[0] = -1;
            }
            if (playerWork->stateFrame == 0x21) {
                Hu3DModelAttrSet(sceneObjects->mdlId[selectedBoardSpace + 4], HU3D_MOTATTR_PAUSE);
            }
            if (playerWork->stateFrame == 0x3F) {
                Hu3DModelAttrReset(sceneObjects->mdlId[selectedBoardSpace + 4], HU3D_MOTATTR_PAUSE);
            }
            if ((playerWork->stateFrame == 0x44) && (effectSoundQueue[1].effectCount < 3)) {
                effectSoundQueue[1].effectTimers[effectSoundQueue[1].effectCount] = 0.0f;
                effectSoundQueue[1].effectCount++;
            }
            if (playerWork->stateFrame == 0x82) {
                playerWork->playerState = 0;
                playerWork->stateFrame = 0;
            }
            break;
    }
    if (((playerWork->playerState == 3) || (playerWork->playerState == 4)) && (movementSoundTimer >= 0)) {
        movementSoundTimer++;
        if (movementSoundTimer >= 6) {
            previousSelectedSpace = selectedBoardSpace;
            for (spaceSearchAttempt = 0; spaceSearchAttempt < 0x32; spaceSearchAttempt++) {
                do {
                    selectedBoardSpace = rand8() % 11;
                } while ((selectedBoardSpace >= 9 && selectedBoardSpace == playerWork->teamId + 9) || (selectedBoardSpace < 9 && boardSpaces[selectedBoardSpace].isAvailable == 0));
                if (previousSelectedSpace != selectedBoardSpace) {
                    break;
                }
            }
            previousSelectedSpace = selectedBoardSpace >= 9 ? (opponentWork->characterId == -1 ? 0x11 : (opponentWork->characterId + 9)) : selectedBoardSpace;
            movementSoundTimer = 0;
            Hu3DMotionTimeSet(sceneObjects->mdlId[3], 0.5 + previousSelectedSpace);
        }
    }
    if (playerWork->requestedMotionIndex >= 0) {
        requestedMotionIndex = playerWork->requestedMotionIndex;
    }
    if ((requestedMotionIndex >= 0) && (requestedMotionIndex != playerWork->motionIndex)) {
        Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[requestedMotionIndex], motionTransitionFrames[requestedMotionIndex],
            (playerWork->characterId != -1 ? characterMotionBlendFrames[requestedMotionIndex] : fallbackMotionBlendFrames[requestedMotionIndex]),
            (requestedMotionIndex == 8                                                                            ? 0x40000002
                    : (playerWork->characterId != -1 ? characterMotionLoopFlags[requestedMotionIndex] : fallbackMotionLoopFlags[requestedMotionIndex]) != 0 ? 0
                                                                                                     : 0x40000001));
        playerWork->motionIndex = requestedMotionIndex;
    }
}

s32 cameraSequenceState = -1;
float cameraStartZoom = 3500.0f;
float cameraStartHeight = 100.0f;
float cameraStartRotationX = -59.0f;

void MinigameSceneUpdate(OMOBJ *controllerObject)
{
    PlayerWork *playerWorks[2];
    float cameraBlend;
    float boardExpansionProgress;
    float entranceProgress;
    float windOffsetZ;
    float windOffsetX;
    float markerOpacity;

    s32 itemIndex;
    s32 boardSpaceIndex;
    HU3DMODEL *playerBoardModel;
    HU3DMODEL *boardSpaceModel;
    HU3DMODEL *backdropModel;
    s32 previousCameraSequenceState;

    for (itemIndex = 0; itemIndex < 2; itemIndex++) {
        playerWorks[itemIndex] = (&playerObjects)[itemIndex]->data;
    }
    sceneUpdateFrame++;
    switch (gameState) {
        case 0x3EA:
            if (gameStateTimer <= 0x77) {
                entranceProgress = gameStateTimer / 119.0;
                entranceProgress = entranceProgress > 1.0f ? 1.0f : entranceProgress;
                Hu3DModelPosSet(sceneObjects->mdlId[2], 0.0f, 100.0f + (-1500.0f * (1.0f - entranceProgress)), -950.0f);
                Hu3DModelAttrReset(sceneObjects->mdlId[1], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(sceneObjects->mdlId[2], HU3D_ATTR_DISPOFF);
            }
            break;
        case 0x3EB:
            if (gameStateTimer == 0) {
                swingAnimationFrame = 0;
            }
#if VERSION_PAL
            if (gameStateTimer == 0xF) {
#else
            if (gameStateTimer == 0x14) {
#endif
                espAttrReset(playerMarkerSprites[0], HUSPR_ATTR_DISPOFF);
            }
#if VERSION_PAL
            if (((gameStateTimer >= 0xF) && (gameStateTimer <= 0x28)) || ((gameStateTimer >= 0x96) && (gameStateTimer <= 0xAF))) {
                markerOpacity = gameStateTimer <= 0x78 ? ((gameStateTimer - 0xF) / (REFRESH_RATE_F / 2.0)) : (1.0 - ((gameStateTimer - 0x96) / (REFRESH_RATE_F / 2.0)));
#else
            if (((gameStateTimer >= 0x14) && (gameStateTimer <= 0x32)) || ((gameStateTimer >= 0xB4) && (gameStateTimer <= 0xD2))) {
                markerOpacity = gameStateTimer <= 0x78 ? ((gameStateTimer - 0x14) / (REFRESH_RATE_F / 2.0)) : (1.0 - ((gameStateTimer - 0xB4) / (REFRESH_RATE_F / 2.0)));
#endif
                espTPLvlSet(playerMarkerSprites[0], markerOpacity);
            }
            if (gameStateTimer == (VERSION_PAL ? 0xAF : 0xD2)) {
                espAttrSet(playerMarkerSprites[0], HUSPR_ATTR_DISPOFF);
            }
        case 0x3EC:
        case 0x3ED:
            swingAnimationFrame++;
            swingAnimationFrame %= 0x168;
            if (isSinglePlayer != 0) {
                Hu3DModelPosSet(sceneObjects->mdlId[2], 0.0f, 100.0 + (25.0 * sind((360.0 * ((swingAnimationFrame % 360) / 360.0)))), -950.0f);
            }
            break;
        case 0x3EE:
            if (isSinglePlayer != 0) {
                backdropModel = &Hu3DData[sceneObjects->mdlId[2]];
                if (gameStateTimer >= 7) {
                    backdropModel->pos.y += 12.5;
                }
                if (gameStateTimer == 0) {
                    Hu3DMotionShiftSet(controllerObject->mdlId[1], controllerObject->mtnId[1], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                }
            }
            else {
                playerBoardModel = &Hu3DData[sceneObjects->mdlId[playerWorks[0]->boardSpaceIndex + 4]];
                if (hasPlayedLandingSound == 0) {
                    hasPlayedLandingSound = 1;
                    HuAudFXPlay(0x7F6);
                }
                if (gameStateTimer == 1) {
                    boardSpaces[playerWorks[0]->boardSpaceIndex].heightOffset = 83.333336f;
                }
                playerBoardModel->pos.y = (0.9 * playerBoardModel->pos.y) + (0.1 * boardSpaces[playerWorks[0]->boardSpaceIndex].heightOffset);
                playerObjects->trans.y = playerBoardModel->pos.y - -16.666666666666668;
            }
            break;
        case 0x3F0:
            Hu3DModelAttrSet(sceneObjects->mdlId[1], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(sceneObjects->mdlId[2], HU3D_ATTR_DISPOFF);
            if (gameStateTimer <= 0x3C) {
                boardExpansionProgress = gameStateTimer / 60.0;
                boardExpansionProgress = boardExpansionProgress;
                boardExpansionProgress = 1.0f - ((1.0f - boardExpansionProgress) * (1.0f - boardExpansionProgress));
                if (gameStateTimer == 0) {
                    boardSpaces[playerWorks[0]->boardSpaceIndex].heightOffset = -16.666666f;
                }
                for (itemIndex = 0; itemIndex < 9; itemIndex++) {
                    boardSpaceModel = &Hu3DData[sceneObjects->mdlId[itemIndex + 4]];
                    Hu3DModelPosSet(sceneObjects->mdlId[itemIndex + 4], (500.0 + (200.0f * boardExpansionProgress)) * (boardSpaces[itemIndex].boardColumn - 1),
                        (0.9 * boardSpaceModel->pos.y) + (0.1 * boardSpaces[playerWorks[0]->boardSpaceIndex].heightOffset),
                        (500.0 + (200.0f * boardExpansionProgress)) * (boardSpaces[itemIndex].boardRow - 1));
                    for (boardSpaceIndex = 0; boardSpaceIndex < 2; boardSpaceIndex++) {
                        if (itemIndex == playerWorks[boardSpaceIndex]->boardSpaceIndex) {
                            (&playerObjects)[boardSpaceIndex]->trans.x = (500.0 + (200.0f * boardExpansionProgress)) * (boardSpaces[itemIndex].boardColumn - 1);
                            (&playerObjects)[boardSpaceIndex]->trans.y = boardSpaceModel->pos.y - -16.666666666666668;
                            (&playerObjects)[boardSpaceIndex]->trans.z = (500.0 + (200.0f * boardExpansionProgress)) * (boardSpaces[itemIndex].boardRow - 1);
                        }
                    }
                }
            }
            break;
    }
    previousCameraSequenceState = cameraSequenceState;
    {
        CameraKeyframe cameraKeyframes[6] = {
            { 3500.0f, 0.0f, 100.0f, 0.0f, -59.0f, 0.0f },
            { 3200.0f, 0.0f, 100.0f, 0.0f, -30.0f, 0.0f },
            { 3800.0f, 0.0f, 100.0f, 0.0f, -30.0f, 0.0f },
            { 2000.0f, 0.0f, 200.0f, 0.0f, -30.0f, 0.0f },
            { 800.0f, 0.0f, 200.0f, 0.0f, -89.0f, 0.0f },
            { 1800.0f, 0.0f, 200.0f, 0.0f, -30.0f, 0.0f },
        };

        if (winningTeam != -1) {
            cameraKeyframes[3].centerX = (&playerObjects)[1 - winningTeam]->trans.x;
            cameraKeyframes[3].centerZ = (&playerObjects)[1 - winningTeam]->trans.z;
            cameraKeyframes[4].centerX = (&playerObjects)[1 - winningTeam]->trans.x;
            cameraKeyframes[4].centerZ = (&playerObjects)[1 - winningTeam]->trans.z;
            cameraKeyframes[5].centerX = (&playerObjects)[winningTeam]->trans.x;
            cameraKeyframes[5].centerZ = (&playerObjects)[winningTeam]->trans.z;
        }
        if (cameraSequenceState == -1) {
            cameraSequenceState = 0;
            cameraTransitionDirection = 1;
            cameraTransitionProgress = 1.0f;
        }
        if (cameraSequenceState == 0) {
            cameraSequenceState = 1;
            cameraTransitionDirection = 1;
            cameraTransitionProgress = 0.5f;
        }
        if ((gameState == 0x3F0) && (gameStateTimer >= 1) && (cameraSequenceState == 1)) {
            cameraSequenceState = 2;
            cameraTransitionDirection = 0;
            cameraTransitionProgress = 0.0f;
        }
        if ((gameState == 0x3F3) && (cameraSequenceState == 2)) {
            cameraSequenceState = 3;
            cameraTransitionDirection = 0;
            cameraTransitionProgress = 0.0f;
        }
        if ((gameState == 0x3F3) && (gameStateTimer >= 0x12C) && (cameraSequenceState == 3)) {
            cameraSequenceState = 4;
            cameraTransitionDirection = 1;
            cameraTransitionProgress = 1.0f;
        }
        if ((gameState == 0x3F4) && (cameraSequenceState == 4)) {
            cameraSequenceState = 5;
            cameraTransitionDirection = 0;
            cameraTransitionProgress = 0.0f;
        }
        if ((previousCameraSequenceState != cameraSequenceState) && (previousCameraSequenceState != -1)) {
            cameraStartZoom = CZoom;
            cameraStartHeight = Center.y;
            cameraStartCenterZ = Center.z;
            cameraStartRotationX = CRot.x;
            cameraStartRotationY = CRot.y;
        }
        if (cameraTransitionProgress <= 1.0f) {
            if (cameraTransitionDirection == 0) {
                cameraTransitionProgress = 1.0 - (0.95 * (1.0f - cameraTransitionProgress));
                cameraBlend = cameraTransitionProgress;
            }
            else {
                cameraTransitionProgress += 0.008333333333333333;
                cameraTransitionProgress = cameraTransitionProgress > 1.0f ? 1.0f : cameraTransitionProgress;
                cameraBlend = 0.5 + (-0.5 * cosd((180.0f * cameraTransitionProgress)));
                cameraBlend = cameraBlend;
            }
            cameraTransitionProgress = cameraTransitionProgress > 1.0f ? 1.0f : cameraTransitionProgress;
            CZoom = (cameraBlend * cameraKeyframes[cameraSequenceState].zoom) + (cameraStartZoom * (1.0f - cameraBlend));
            Center.x = (cameraBlend * cameraKeyframes[cameraSequenceState].centerX) + (cameraStartCenterX * (1.0f - cameraBlend));
            Center.y = (cameraBlend * cameraKeyframes[cameraSequenceState].heightOffset) + (cameraStartHeight * (1.0f - cameraBlend));
            Center.z = (cameraBlend * cameraKeyframes[cameraSequenceState].centerZ) + (cameraStartCenterZ * (1.0f - cameraBlend));
            CRot.x = (cameraBlend * cameraKeyframes[cameraSequenceState].rotationX) + (cameraStartRotationX * (1.0f - cameraBlend));
            CRot.y = (cameraBlend * cameraKeyframes[cameraSequenceState].rotationY) + (cameraStartRotationY * (1.0f - cameraBlend));
        }
    }
    if ((gameState == 0x3E9) || (gameState == 0x3EE)) {
        windOffsetX = 0.0f;
        windOffsetZ = 0.0f;
        if ((windState.windUpdatePhase % 2) >= 0) {
            windState.windX = windState.targetWindX;
            windState.windZ = windState.targetWindZ;
            do {
                windState.targetWindX = 2.0 * (rand8() / 255.0) - 1.0;
                windState.targetWindZ = (2.0 * (rand8() / 255.0)) - 1.0;
                windState.targetWindX = 0.0f;
            } while (sqrtf(((windState.targetWindX - windState.windX) * (windState.targetWindX - windState.windX))
                         + ((windState.targetWindZ - windState.windZ) * (windState.targetWindZ - windState.windZ)))
                < 0.75);
        }
        windOffsetX = windState.windX;
        windOffsetZ = windState.windZ;
        windOffsetX *= windState.windTransitionTime * 2;
        windOffsetZ *= windState.windTransitionTime * 2;
        windState.windUpdatePhase = !windState.windUpdatePhase;
        windState.windTransitionTime = (windState.windTransitionTime > 24) ? 23 : (windState.windTransitionTime - 1 > 0) ? (windState.windTransitionTime - 1) : 0;
        Center.x = 0.0f;
        Center.y = 100.0f + windOffsetZ;
    }
}

const HU3DPARMANPARAM windParticleParameters = {
    0x0096,
    { 0x00, 0x00 },
    50.0f,
    0.0f,
    65.0f,
    { 0.0f, -0.195f, 0.0f },
    100.0f,
    0.99f,
    40.0f,
    0.999f,
    4,
    {
        { 0xFF, 0xFF, 0x20, 0xFF },
        { 0xFF, 0xFF, 0x00, 0xFF },
        { 0xFF, 0xFF, 0x80, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF },
    },
    {
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 },
    },
};

s16 WindParticleCreate(void)
{
    s16 particleManagerId;
    s16 particleModelId;
    ANIMDATA *particleAnimation;
    void *particleArchiveData;

    HU3DPARMANPARAM particleParameters = windParticleParameters;
    windParticleParametersWork = particleParameters;
    particleArchiveData = HuDataSelHeapReadNum(0x7006B, HU_MEMNUM_OVL, HEAP_MODEL);
    particleAnimation = HuSprAnimRead(particleArchiveData);
    particleManagerId = Hu3DParManCreate(particleAnimation, 0x64, &windParticleParametersWork);
    Hu3DParManAttrSet(particleManagerId, 0x108);
    Hu3DParManRotSet(particleManagerId, 0.0f, 0.0f, 0.0f);
    Hu3DParManVecSet(particleManagerId, 0.0f, 0.4f, 0.2f);
    particleModelId = Hu3DParManModelIDGet(particleManagerId);
    Hu3DModelLayerSet(particleModelId, 2);
    Hu3DParticleBlendModeSet(particleModelId, 0);
    return particleManagerId;
}

void ResultsMessageProcess(void)
{
    PlayerWork *playerWorks[2];
    s16 unusedWindowState;
    s16 windowId;
    s32 playerIndex;
    WindowData *windowData;
    s32 secondPlayerPadMask;
    s32 secondPlayerFallbackPadMask;
    s32 firstPlayerPadMask;
    s32 firstPlayerFallbackPadMask;

    unusedWindowState = 0;
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        playerWorks[playerIndex] = (&playerObjects)[playerIndex]->data;
    }
    HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
    HuWinComKeyReset();
    windowId = HuWinExCreateStyled(36.0f, 344.0f, 0x1F8, 0x60, 6, 0);
    windowData = &winData[windowId];
    if (playerWorks[1]->controllerIndex != -1) {
        secondPlayerPadMask = 1 << playerWorks[1]->controllerIndex;
    }
    else {
        if (playerWorks[1]->characterId != -1) {
            secondPlayerFallbackPadMask = 1 << GWPlayerCfg[playerWorks[1]->playerIndex].pad_idx;
        }
        else {
            secondPlayerFallbackPadMask = 0;
        }
        secondPlayerPadMask = secondPlayerFallbackPadMask;
    }
    if (playerWorks[0]->controllerIndex != -1) {
        firstPlayerPadMask = 1 << playerWorks[0]->controllerIndex;
    }
    else {
        if (playerWorks[0]->characterId != -1) {
            firstPlayerFallbackPadMask = 1 << GWPlayerCfg[playerWorks[0]->playerIndex].pad_idx;
        }
        else {
            firstPlayerFallbackPadMask = 0;
        }
        firstPlayerPadMask = firstPlayerFallbackPadMask;
    }
    windowData->active_pad = firstPlayerPadMask | secondPlayerPadMask;
    OSReport("%d\n", windowData->active_pad);
    HuWinBGTPLvlSet(windowId, 0.8f);
    HuWinMesSpeedSet(windowId, 1);
    HuWinExAnimIn(windowId);
    HuWinMesSet(windowId, 0x300024);
    if (((playerWorks[0]->controllerIndex == -1) || (playerWorks[0]->characterId == -1)) && ((playerWorks[1]->controllerIndex == -1) || (playerWorks[1]->characterId == -1))) {
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
    }
    HuWinMesWait(windowId);
    HuWinExAnimOut(windowId);
    HuWinExCleanup(windowId);
    isMessageActive = 0;
    HuPrcEnd();
}

void MinigameControllerUpdate(OMOBJ *controllerObject)
{
    PlayerWork *playerWorks[2];
    s32 soundEffectId;
    s32 effectIndex;

    for (effectIndex = 0; effectIndex < 0x1C; effectIndex++) {
        s32 effectSoundIds[] = {
            0x7F1,
            0x7F2,
            0x7F7,
            0x7F8,
            0x7F9,
            0x7FA,
            0x7FB,
            0x7FC,
            0x7FD,
            0x7FE,
            0x7FF,
            0x800,
            0x801,
            0x802,
            0x803,
            0x804,
            0x7FB,
            0x7FC,
            0x7FD,
            0x7FE,
            0x7FF,
            0x800,
            0x801,
            0x802,
            0x803,
            0x804,
            0x7FB,
            0x7FC,
        };
        if (effectSoundQueue[effectIndex].effectCount > 3) {
            effectSoundQueue[effectIndex].effectCount = 3;
        }
        if (effectSoundQueue[effectIndex].effectCount > 0) {
            soundEffectId = HuAudFXPlay(effectSoundIds[effectIndex]);
            effectSoundQueue[effectIndex].effectCount--;
        }
    }
    for (effectIndex = 0; effectIndex < 2; effectIndex++) {
        playerWorks[effectIndex] = (&playerObjects)[effectIndex]->data;
    }
    if ((omSysExitReq != 0) && (WipeStatGet() == 0)) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        controllerObject->objFunc = MinigameCleanup;
    }
    switch (gameState) {
        case 0x3E8:
            if (WipeStatGet() == 0) {
                gameState++;
                gameStateTimer = 0.0f;
                if (windParticleManagerId[0] != -1) {
                    Hu3DParManKill(*windParticleManagerId);
                    windParticleManagerId[0] = -1;
                }
            }
        default:
            break;
        case 0x3E9:
            gameStateTimer++;
            if (gameStateTimer == ((isSinglePlayer == 0 ? 0x1E : 0) + 0x3C)) {
                gameState = isSinglePlayer != 0 ? 0x3EA : 0x3ED;
                gameStateTimer = 0.0f;
            }
            break;
        case 0x3EA:
            gameStateTimer++;
            if (gameStateTimer == 0x78) {
                gameState++;
                gameStateTimer = 0.0f;
            }
            break;
        case 0x3EB:
            gameStateTimer++;
            if (gameStateTimer > (s32)(REFRESH_RATE * 3.5)) {
                gameState++;
                gameStateTimer = 0.0f;
            }
            break;
        case 0x3EC:
            if (gameStateTimer == 0) {
                HuPrcChildCreate(ResultsMessageProcess, 0x64, 0x2000, 0, HuPrcCurrentGet());
                isMessageActive = 1;
                gameStateTimer++;
                break;
            }
            if (isMessageActive == 0) {
                gameStateTimer++;
                if (playerWorks[0]->playerState == 0) {
                    gameState++;
                    gameStateTimer = 0.0f;
                }
            }
            break;
        case 0x3ED:
            gameStateTimer += gameStateTimer < 0x1E;
            if (gameStateTimer >= 0x3C) {
                gameState++;
                gameStateTimer = 0.0f;
            }
            break;
        case 0x3EE:
            gameStateTimer++;
            if (gameStateTimer == ((playerWorks[1]->characterId == -1 ? 0x78 : 0x3C) + (isSinglePlayer == 0 ? 0x1E : 0))) {
                gameState++;
                if (playerWorks[1]->characterId == -1) {
                    gameState++;
                }
                gameStateTimer = 0.0f;
            }
            break;
        case 0x3EF:
            gameStateTimer += gameStateTimer < 0x1E;
            if (gameStateTimer >= 0x3C) {
                gameState++;
                gameStateTimer = 0.0f;
            }
            break;
        case 0x3F0:
            gameStateTimer++;
            if (gameStateTimer == 0x5A) {
                gameState++;
                gameStateTimer = 0.0f;
            }
            break;
        case 0x3F1:
        case 0x3F2:
            if (gameStateTimer == 0) {
                playerWorks[gameState == 0x3F1 ? 0 : 1]->playerState = 3;
                playerWorks[gameState == 0x3F1 ? 0 : 1]->stateFrame = 0;
            }
            gameStateTimer++;
            if (playerWorks[gameState == 0x3F1 ? 0 : 1]->playerState == 0) {
                if (winningTeam != -1) {
                    gameState = 0x3F3;
                }
                else {
                    gameState = gameState == 0x3F1 ? 0x3F2 : 0x3F1;
                }
                gameStateTimer = 0.0f;
            }
            break;
        case 0x3F3:
            gameStateTimer++;
#if VERSION_PAL
            if (gameStateTimer >= 0x1C4) {
#else
            if (gameStateTimer >= 0x1E0) {
#endif
                gameState++;
                gameStateTimer = 0.0f;
            }
            break;
        case 0x3F4:
            gameStateTimer++;
            if (gameStateTimer == 1) {
                if (winningTeam != 4) {
                    if (playerWorks[winningTeam]->characterId != -1) {
                        HuAudSStreamPlay(1);
                    }
                    else {
                        HuAudSStreamPlay(3);
                    }
                    if (isSinglePlayer != 0) {
                        if (winningTeam == 0) {
                            _SetFlag(0x10007);
                        }
                    }
                    else {
                        GWPlayerCoinWinSet(playerWorks[winningTeam]->playerIndex, 10);
                    }
                }
                else {
                    winningTeam == 4;
                }
            }
            if (REFRESH_RATE_F * 3.5f < gameStateTimer) {
                if (startSoundId != -1) {
                    HuAudFXStop(startSoundId);
                    startSoundId = -1;
                }
                minigameSequenceId = -1;
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 0x3C);
                controllerObject->objFunc = MinigameCleanup;
            }
            break;
    }
}

s32 boardModelFiles[0xB] = {
    0x570013,
    0x570014,
    0x570015,
    0x570016,
    0x570017,
    0x570018,
    0x570019,
    0x57001A,
    0x57001B,
    0x57001C,
    0x57001D,
};

void SceneObjectsCreate(OMOBJ *object)
{
    PlayerWork *playerWorks[2];

    s32 modelIndex;
    s16 particleManagerId;
    s16 particleModelId;
    s32 modelFileId;
    void *particleArchiveData;
    ANIMDATA *particleAnimation;
    s16 particleManagerAlias;
    s16 particleManagerHandle;
    s16 createdParticleManagerId;

    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        playerWorks[modelIndex] = (&playerObjects)[modelIndex]->data;
    }
    object->mdlId[1] = Hu3DModelCreateFile(0x3000E);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_LOOP);
    object->mdlId[2] = Hu3DModelCreateFile(0x30000);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_MOTATTR_LOOP);
    Hu3DModelHookSet(object->mdlId[2], "minik01", object->mdlId[1]);
    Hu3DModelPosSet(object->mdlId[2], 0.0f, -750.0f, -700.0f);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    object->mtnId[0] = Hu3DJointMotionFile(object->mdlId[1], 0x3000F);
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], 0x30012);
    Hu3DMotionSet(object->mdlId[1], object->mtnId[0]);
    Hu3DModelLayerSet(object->mdlId[2], 1);
    Hu3DModelLayerSet(object->mdlId[1], 1);
    object->mdlId[0] = Hu3DModelCreateFile(0x570008);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DMotionSpeedSet(object->mdlId[0], 0.1f);
    object->mdlId[3] = Hu3DModelCreateFile(0x570009);
    Hu3DModelAttrSet(object->mdlId[3], HU3D_MOTATTR_LOOP);
    Hu3DModelPosSet(object->mdlId[3], 0.0f, 400.0f, 0.0f);
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(object->mdlId[3], 1);
    Hu3DMotionSpeedSet(object->mdlId[3], 0.0f);
    for (modelIndex = 0; modelIndex < 9; modelIndex++) {
        object->mdlId[modelIndex + 4] = Hu3DModelCreateFile(modelIndex + 0x57000A);
        boardSpaces[modelIndex].boardColumn = modelIndex % 3;
        boardSpaces[modelIndex].boardRow = modelIndex / 3;
        boardSpaces[modelIndex].heightOffset = 0.0f;
        boardSpaces[modelIndex].isAvailable = 1;
        Hu3DModelPosSet(
            sceneObjects->mdlId[modelIndex + 4], 500.0 * (boardSpaces[modelIndex].boardColumn - 1), -16.666666f, 500.0 * (boardSpaces[modelIndex].boardRow - 1));
        Hu3DModelScaleSet(sceneObjects->mdlId[modelIndex + 4], 1.6666666f, 1.6666666f, 1.6666666f);
        Hu3DModelAttrSet(object->mdlId[modelIndex + 4], HU3D_MOTATTR_PAUSE);
        Hu3DModelShadowMapSet(object->mdlId[modelIndex + 4]);
        Hu3DModelLayerSet(object->mdlId[modelIndex + 4], 1);
    }
    for (modelIndex = 0; modelIndex < 0xB; modelIndex++) {
        if ((modelIndex == 9) || (modelIndex == 0xA)) {
            if (playerWorks[modelIndex - 9]->characterId != -1) {
                modelFileId = playerWorks[modelIndex - 9]->characterId + 0x57001C;
            }
            else {
                modelFileId = 0x570024;
            }
            boardModelFiles[modelIndex] = modelFileId;
        }
        object->mdlId[modelIndex + 0xD] = Hu3DModelCreateFile(boardModelFiles[modelIndex]);
        Hu3DMotionTimeSet(object->mdlId[modelIndex + 0xD], 10.0f);
        Hu3DMotionSpeedSet(object->mdlId[modelIndex + 0xD], 0.0f);
        Hu3DModelAttrSet(object->mdlId[modelIndex + 0xD], HU3D_ATTR_DISPOFF);
        Hu3DModelLayerSet(object->mdlId[modelIndex + 0xD], 1);
    }
    for (modelIndex = 0; modelIndex < 2; modelIndex++) {
        s32 markerXPositions[2] = { 0x120, 0x122 };
        s32 markerYPositions[2] = { 0xF0, 0x8C };
        playerMarkerSprites[modelIndex] = espEntry(playerMarkerModelFiles[modelIndex], 0, 0);
        espDrawNoSet(playerMarkerSprites[modelIndex], 0);
        espAttrSet(playerMarkerSprites[modelIndex], HUSPR_ATTR_NOANIM | HUSPR_ATTR_DISPOFF);
        espBankSet(playerMarkerSprites[modelIndex], 0);
        espPriSet(playerMarkerSprites[modelIndex], 0x80 - modelIndex);
        espPosSet(playerMarkerSprites[modelIndex], markerXPositions[modelIndex], markerYPositions[modelIndex]);
        espTPLvlSet(playerMarkerSprites[modelIndex], 1.0f);
    }
    {
        HU3DPARMANPARAM particleParameters = windParticleParameters;
        windParticleParametersWork = particleParameters;
    }
    particleArchiveData = HuDataSelHeapReadNum(0x7006B, HU_MEMNUM_OVL, HEAP_MODEL);
    particleAnimation = HuSprAnimRead(particleArchiveData);
    particleManagerId = Hu3DParManCreate(particleAnimation, 0x64, &windParticleParametersWork);
    Hu3DParManAttrSet(particleManagerId, 0x108);
    Hu3DParManRotSet(particleManagerId, 0.0f, 0.0f, 0.0f);
    Hu3DParManVecSet(particleManagerId, 0.0f, 0.4f, 0.2f);
    particleModelId = Hu3DParManModelIDGet(particleManagerId);
    Hu3DModelLayerSet(particleModelId, 2);
    Hu3DParticleBlendModeSet(particleModelId, 0);
    particleManagerAlias = particleManagerId;
    createdParticleManagerId = particleManagerAlias;
    windParticleManagerId[0] = createdParticleManagerId;
    Hu3DParManPosSet(windParticleManagerId[0], 10000.0f, -10000.0f, 10000.0f);
    object->objFunc = MinigameSceneUpdate;
}

void PlayerObjectCreate(OMOBJ *object)
{
    s32 opponentPlayerIndex;
    void *opponentWork;
    PlayerWork *work;
    s32 motionIndex;

    work = object->data;
    opponentWork = (&playerObjects)[1 - object->work[0]]->data;
    memset(work, 0, sizeof(PlayerWork));
    work->teamId = object->work[0];
    work->playerIndex = GWPlayerCfg->group == work->teamId
        ? 0
        : (GWPlayerCfg[1].group == work->teamId ? 1 : (GWPlayerCfg[2].group == work->teamId ? 2 : (GWPlayerCfg[3].group == work->teamId ? 3 : 4)));
    opponentPlayerIndex = GWPlayerCfg->group == (1 - work->teamId)
        ? 0
        : (GWPlayerCfg[1].group == (1 - work->teamId)
                ? 1
                : (GWPlayerCfg[2].group == (1 - work->teamId) ? 2 : (GWPlayerCfg[3].group == (1 - work->teamId) ? 3 : 4)));
    if (work->playerIndex == 4) {
        OSReport("m458:wrong player group\n");
        work->playerIndex = work->teamId;
    }
    if ((work->teamId == 1) && (isSinglePlayer != 0)) {
        work->characterId = -1;
        work->controllerIndex = -1;
        work->playerIndex = -1;
        work->difficulty = 0;
    }
    else {
        work->characterId = GWPlayerCfg[work->playerIndex].character;
        work->controllerIndex = ((work->characterId == -1) || (GWPlayerCfg[work->playerIndex].iscom != 0)) ? -1 : GWPlayerCfg[work->playerIndex].pad_idx;
        work->difficulty = work->controllerIndex != -1 ? 0 : GWPlayerCfg[work->playerIndex].diff;
    }
    work->playerState = 0;
    work->motionIndex = 3;
    work->requestedMotionIndex = -1;
    if (work->characterId != -1) {
        object->mdlId[0] = CharModelCreate(work->characterId, 1);
    }
    else {
        object->mdlId[0] = Hu3DModelCreateFile(0x30007);
    }
    omSetTra(object, 0.0f, work->teamId == 0 ? 0 : 0x12C, 0.0f);
    if (work->characterId != -1) {
        omSetSca(object, 2.5f, 2.5f, 2.5f);
    }
    else {
        omSetSca(object, 2.0f, 2.0f, 2.0f);
    }
    for (motionIndex = 0; motionIndex < 0xA; motionIndex++) {
        if (work->characterId != -1) {
            object->mtnId[motionIndex] = CharMotionCreate(work->characterId, characterMotionFiles[motionIndex] + (characterSpecificMotionFlags[motionIndex] != 0 ? work->characterId : 0));
        }
        else if (fallbackMotionFiles[motionIndex] != 0) {
            object->mtnId[motionIndex] = Hu3DJointMotionFile(object->mdlId[0], fallbackMotionFiles[motionIndex]);
        }
    }
    Hu3DMotionSet(object->mdlId[0], object->mtnId[work->motionIndex]);
    Hu3DMotionTimeSet(object->mdlId[0], 10.0f);
    Hu3DModelAttrReset(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(object->mdlId[0]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    Hu3DModelLayerSet(object->mdlId[0], 1);
    CharModelLayerSetAll2(1);
    object->objFunc = PlayerCharacterUpdate;
}

void MinigameStart(OMOBJ *object)
{
    gameState = 0x3E8;
    gameStateTimer = 0.0f;
    unusedStateA8 = 0;
    unusedStateA4 = 0;
    if (backgroundMusicId == -1) {
        backgroundMusicId = HuAudSeqPlay(0x4D);
    }
    startSoundId = HuAudFXPlay(0x6D7);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    object->objFunc = MinigameControllerUpdate;
}

void ObjectSetup(void)
{
    s32 i;
    OMOBJ *object;

    OSReport("******* M458 ObjectSetup *********\n");
    objectManager = omInitObjMan(0x258, 0x2000);
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
        Vec lightOrigin = { 0.0f, 0.0f, 0.0f };
        Vec lightDirection = { 0.25f, -0.75f, -0.45f };
        GXColor lightColor = { 255, 255, 255, 255 };
        directionalLightId = Hu3DGLightCreateV(&lightOrigin, &lightDirection, &lightColor);
        Hu3DGLightStaticSet(directionalLightId, 1);
        Hu3DGLightInfinitytSet(directionalLightId);
    }
    {
        Vec shadowPosition;
        Vec shadowUpDirection;
        Vec shadowOffset;

        shadowPosition.x = 0.0f;
        shadowPosition.y = 10000.0f;
        shadowPosition.z = 0.0f;
        shadowUpDirection.x = 0.0f;
        shadowUpDirection.y = 1.0f;
        shadowUpDirection.z = 0.0f;
        shadowOffset.x = 0.0f;
        shadowOffset.y = 0.0f;
        shadowOffset.z = 0.5f;
        Hu3DShadowCreate(12.0f, 9000.0f, 10010.0f);
        Hu3DShadowTPLvlSet(0.625f);
        Hu3DShadowPosSet(&shadowPosition, &shadowUpDirection, &shadowOffset);
    }
    Hu3DShadowSizeSet(0xE0);
    minigameSequenceId = -1;
    winningTeam = -1;
    isSinglePlayer = _CheckFlag(0x10000) != 0;
    HuWinInit(1);
    sceneObjects = omAddObjEx(objectManager, 0x65, 0x18, 2, 1, SceneObjectsCreate);
    for (i = 0; i < 2; i++) {
        object = (&playerObjects)[i] = omAddObjEx(objectManager, 0x64, 0xB, 0xA, 0, PlayerObjectCreate);
        object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerWork), HU_MEMNUM_OVL);
        object->work[0] = i;
    }
    outOfViewObject = omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutView);
    controllerObject = omAddObjEx(objectManager, 0x67, 0, 0, -1, MinigameStart);
}

void MinigameCleanup(OMOBJ *object)
{
    if (windParticleManagerId[0] != -1) {
        Hu3DParManKill(windParticleManagerId[0]);
        windParticleManagerId[0] = -1;
    }
    if (WipeStatGet() == 0) {
        s32 i;
        for (i = 0; i < 2; i++) {
            void *playerWorkData = (&playerObjects)[i]->data;
        }
        if (minigameSequenceId >= 0) {
            MGSeqKill(minigameSequenceId);
        }
        MGSeqKillAll();
        HuAudAllStop();
        omOvlReturnEx(1, 1);
    }
}

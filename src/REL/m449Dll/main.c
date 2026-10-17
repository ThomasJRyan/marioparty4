#include "dolphin/gx/GXEnum.h"
#include "dolphin/gx/GXVert.h"
#include "dolphin/pad.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/esprite.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "string.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/hsfex.h"
#endif

extern s32 rand8(void);

typedef struct M449CameraPreset {
    /* 0x00 */ float zoom;
    /* 0x04 */ float centerX;
    /* 0x08 */ float centerY;
    /* 0x0C */ float centerZ;
    /* 0x10 */ float pitch;
    /* 0x14 */ float yaw;
} M449CameraPreset; /* size = 0x18 */

typedef struct M449TokenModelIds {
    /* 0x00 */ s32 primaryTokenModelIds[4];
    /* 0x10 */ s32 secondaryTokenModelIds[4];
} M449TokenModelIds; /* size = 0x20 */

typedef struct M449DllWork {
    s32 playerIndex;
    s32 playerNumber;
    s32 characterId;
    s32 controllerIndex;
    s32 difficulty;
    s32 activeTokenIndex;
    s32 currentAction;
    s32 actionTimer;
    float targetX;
    float targetZ;
    float directionX;
    float directionZ;
    s32 previousHitTarget;
    s32 consecutiveHits;
    s32 hitCooldown;
    float rotationX;
    float rotationZ;
    s32 score;
    s32 hitLimit;
    s32 playerState;
    s32 stateTimer;
    s32 noticeTimer;
    float inputX;
    float inputY;
    float tokenX[4];
    float tokenY[4];
    float tokenZ[4];
    float tokenVelocityX[4];
    float tokenVelocityY[4];
    float tokenVelocityZ[4];
    float tokenRotation[4];
    float tokenAngularVelocity[4];
    float targetAngularVelocity[4];
    s32 tokenActive[4];
    s32 tokenAction[4];
    s32 tokenHitMask[4];
    s32 tokenPushMask[4];
    s32 obstacleMask[4];
    s32 collisionMask[4];
    s32 tokenSpin[4];
    float tokenPitch[4];
    float tokenRoll[4];
    s32 tokenHitStun[4];
    s32 boardRound;
    float launchOriginX[4];
    float launchOriginZ[4];
    s32 lastHitBy[4];
    s32 collisionTimer[4];
    char reservedData1D4[0x30];
    s32 aiTarget;
    s32 isCpuPlayer;
    s32 aiMoveX;
    s32 aiMoveZ;
    s32 winnerIndex;
} M449DllWork; /* size = 0x218 */

typedef struct M449SoundEffectQueueEntry {
    /* 0x00 */ s32 pendingSoundCount;
    /* 0x04 */ float soundQueueState[2];
} M449SoundEffectQueueEntry; /* size = 0x0C */

void M449Exit(OMOBJ *object);

s32 scoreboardAssetIds[0x12] = {
    0x50001A,
    0x500019,
    0x500016,
    0x500016,
    0x500016,
    0x500017,
    0x500017,
    0x500017,
    0x610000,
    0x50001A,
    0x500019,
    0x500016,
    0x500016,
    0x500016,
    0x500017,
    0x500017,
    0x500017,
    0x610000,
};
s32 scoreboardSpriteIds[0x12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
s32 characterMotionIds[4]
    = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00), 0x500000, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17), DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x85) };
s32 alternateMotionIds[4] = { 0x50001C, 0x50001E, 0x50001D, 0x50001F };
u8 characterSpecificMotionFlags[0x38] = { 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08

};
char stageObjectNames[4][20] = {
    "m432_00-a00",
    "m432_00-b00",
    "m432_00-player00",
    "m432_00-player01",
};
char tokenObjectNames[4][20] = {
    "m449_01a-a00",
    "m449_01a-a1",
    "m449_01a-a2",
    "m449_01a-a3",
};
char hitPartObjectNames[4][20] = {
    "m449_01a-b00",
    "m449_01a-b1",
    "m449_01a-b2",
    "m449_01a-b3",
};
char tokenTargetObjectNames[4][20] = {
    "m449_01a-c00",
    "m449_01a-c1",
    "m449_01a-c2",
    "m449_01a-c3",
};
s16 tutorialSequenceId = -1;
s16 countdownSequenceId = -1;
s32 musicSequenceId = -1;
s32 boardRoundOrder[3] = { -1, -1, -1 };
s32 playerPatternValues[0xA] = {
    0,
    1,
    0xC2B40000,
    0,
    0x44034000,
    0x43340000,
    0x42B40000,
    0,
    0x44034000,
    0x43340000,
};

M449SoundEffectQueueEntry soundEffectQueue[12];
OMOBJ *exitObject;
OMOBJ *outOfViewObject;
OMOBJ *playerOneObject;
OMOBJ *playerZeroObject;
OMOBJ *stageObject;
s32 stateFrameCount;
s32 gameState;
s32 scoreboardState;
s32 gameEventFlags;
s32 gameEventTimer;
HUPROCESS *objectManager;
s32 directionalLightId;
s32 winnerIndex;
s32 roundResult;
float cameraStartYaw;
float cameraStartCenterX;
float cameraTransitionProgress;
s32 cameraTransitionDirection;
float cameraTargetValue;
s32 gameTimer;
s32 tutorialMessageActive;
s32 gameOptionFlags;
s32 lastHitPlayer;
s32 currentRound;
s32 gameplayFlags;

static const s32 scoreGoalByDifficulty[4] = { 0x8C, 0x6E, 0x3C, 0 };

void M449CameraDebugInputUpdate(void)
{
    if ((HuPadBtnDown[0] & PAD_BUTTON_B) != 0) {
        Center.x = playerZeroObject->trans.x;
        Center.y = playerZeroObject->trans.y;
        Center.z = playerZeroObject->trans.z;
    }
    Center.x += 10.0 * (((HuPadBtn[0] & PAD_BUTTON_RIGHT) != 0) - ((HuPadBtn[0] & PAD_BUTTON_LEFT) != 0));
    Center.z -= 10.0 * (((HuPadBtn[0] & PAD_BUTTON_UP) != 0) - ((HuPadBtn[0] & PAD_BUTTON_DOWN) != 0));
    Center.y += (30.0 * ((HuPadTrigL[0] - HuPadTrigR[0]) / 128.0));
    CZoom += 25.0 * (((HuPadBtn[0] & PAD_BUTTON_X) != 0) - ((HuPadBtn[0] & PAD_BUTTON_Y) != 0));
    CRot.y += HuPadSubStkX[0] / 16.0;
    CRot.x += HuPadSubStkY[0] / -16.0;
}

void M449DrawTokenHighlight(s32 object, Mtx modelMatrix)
{
    float circleVertexX;
    float circleVertexZ;
    s32 circleVertexIndex;
    s32 highlightSide;

    GXLoadPosMtxImm(modelMatrix, 0);
    GXSetNumTevStages(1);
    GXSetNumTexGens(0);
    GXSetZMode(1, GX_LEQUAL, 1);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    for (highlightSide = 0; highlightSide < 2; highlightSide++) {
        GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, 0x12);
        GXPosition3f32(0.0f, 0.0f, 0.0f);
        GXColor4u8(0xFF, 0xFF, 0x00, 0xFF);
        for (circleVertexIndex = 0; circleVertexIndex < 0x11; circleVertexIndex++) {
            circleVertexZ = (highlightSide == 0 ? 1 : -1) * (75.0 * cosd((360.0 * (circleVertexIndex / 16.0))));
            circleVertexX = 75.0 * sind((360.0 * (circleVertexIndex / 16.0)));
            GXPosition3f32(circleVertexX, highlightSide * 10, circleVertexZ);
            GXColor4u8(0xFF, 0xFF, 0x00, 0xFF);
        }
    }
}

void M449PlayerObjectUpdate(OMOBJ *playerObject)
{
    float tokenPositionZ;
    float tokenPositionX;
    float inputDirectionX;
    float inputDirectionZ;
    float tokenPlanarSpeed;
    float noticeAnimationProgress;
    float tokenRotationDelta;
    float tokenSeparation;
    float tokenTargetDistance;
    float tokenSpinSpeed;
    float roundTransitionProgress;
    float collisionNormalX;
    float collisionNormalZ;
    float averageCollisionVelocityX;

    M449DllWork *playerWork;
    s32 tokenIndex;
    s32 collisionTargetIndex;
    s32 otherTokenIndex;
    s32 candidateTokenIndex;
    s32 activeTokenIndex;
    HU3DMODEL *effectModel;
    s32 actionChoiceIndex;
    s32 nearestActionIndex;
    s32 collisionTimerIndex;
    s32 hitMaskIndex;
    HU3DMODEL *tokenModel;
    s32 inputAxisIndex;
    s32 aiPatternIndex;

    Vec hitPartOffsets[3];
    Vec tokenTargetOffsets[4];
    Mtx inputRotationMatrix;
    Mtx tokenRotationMatrix;
    Mtx rotationMatrixScratch;
    Vec playerStagePosition;
    Vec tokenTargetPosition;
    Vec nextRoundTokenPosition;
    Vec tokenLocalPosition;
    Vec tokenEulerRotation;
    void *opponentWork;
    s32 nextAiPatternIndex;
    s32 nearestInactiveTokenIndex;
    float nearestInactiveTokenDistance;
    float tokenPositionDeltaX;
    float tokenPositionDeltaZ;
    s32 particleModelIndex;
    float transitionTimerFraction;
    float transitionTimerFraction;
    s32 particleModelIndex;
    float roundScaleProgress;
    float roundModelScale;
    float noticeTransparency;
    s32 noticeSpriteIndex;
    s32 activeTokenSkipCount;
    s32 activeTokenCount;
    float nearestActionDistanceSquared;
    s32 previousActionTimer;
    float candidateTokenPositionX;
    float candidateTokenPositionZ;
    float targetDeltaX;
    float targetDeltaZ;
    float tokenPreviousXDelta;
    float tokenPreviousZDelta;
    float inputMagnitude;
    float previousInputX;
    float previousInputY;
    float inputDeadZone;
    float inputSmoothingFactor;
    float previousTokenVelocityX;
    float previousTokenVelocityZ;
    float tokenSeparationX;
    float tokenSeparationZ;
    float tokenPreImpactVelocityX;
    float tokenPreImpactVelocityZ;
    float firstTokenNormalVelocity;
    float secondTokenNormalVelocity;
    float averageCollisionVelocityZ;
    float tokenDisplacementX;
    float tokenDisplacementZ;
    float tokenDistance;
    float tokenNormalX;
    float tokenNormalZ;
    float tokenVelocityBeforeImpactX;
    float tokenVelocityBeforeImpactZ;
    float normalVelocityProjection;
    float obstacleSeparationX;
    float obstacleSeparationZ;
    float obstacleDistance;
    float obstacleNormalX;
    float obstacleNormalZ;
    float obstacleVelocityX;
    float obstacleVelocityZ;
    float obstacleNormalVelocity;
    s32 unresolvedCollisionCount;
    float targetSeparationX;
    float targetSeparationZ;
    s32 targetCollisionBlocked;
    float targetNormalX;
    float targetNormalZ;
    float verticalCollisionOffset;
    float verticalCollisionAngle;
    float tokenSpeed3D;
    float tokenPlanarSpeed;
    float tokenElevationAngle;
    float previousTokenHeading;
    float tokenHeading;
    float previousPlayerYaw;
    float targetPlayerYaw;
    float safeTokenRotation;

    {
        s32 aiTargetByRoundDifficultyPattern[4][4][3] = {
            {
                { 2, 5, 5 },
                { 2, 7, 11 },
                { 0, 4, 7 },
                { 0, 7, 8 },
            },
            {
                { 1, 6, 3 },
                { 1, 1, 2 },
                { 8, 2, 3 },
                { 8, 12, 16 },
            },
            {
                { 3, 5, 5 },
                { 1, 6, 13 },
                { 1, 3, 4 },
                { 3, 8, 12 },
            },
            {
                { 2, 3, 4 },
                { 1, 2, 3 },
                { 0, 1, 3 },
                { 2, 3, 3 },
            },
        };
        s32 aiMoveXByRoundDifficultyPattern[4][4][3] = {
            {
                { 0, 44, 23 },
                { 32, -64, -29 },
                { 0, 52, -60 },
                { 0, -51, 56 },
            },
            {
                { -29, 28, -52 },
                { 32, 31, -22 },
                { 0, 20, -49 },
                { 0, 0, 0 },
            },
            {
                { 56, 59, -37 },
                { -36, 33, 47 },
                { 27, 39, 28 },
                { 55, -31, 0 },
            },
            {
                { -52, -61, 31 },
                { 42, -55, 30 },
                { 0, 22, -24 },
                { -48, 25, 61 },
            },
        };

        s32 aiMoveZByRoundDifficultyPattern[4][4][3] = {
            {
                { 0, -44, 23 },
                { -32, 64, 29 },
                { 0, -52, -60 },
                { 0, -51, -56 },
            },
            {
                { -29, 28, 52 },
                { 32, -31, -22 },
                { 0, 20, 49 },
                { 0, 0, 0 },
            },
            {
                { -56, 59, -37 },
                { 36, 33, -47 },
                { -27, 39, -28 },
                { -55, -31, 0 },
            },
            {
                { 52, -61, -31 },
                { -42, 55, -30 },
                { 0, -22, -24 },
                { 48, 25, -61 },
            },
        };
        playerWork = playerObject->data;
        opponentWork = (&playerZeroObject)[1 - playerObject->work[0]]->data;
        inputDirectionX = 0.0f;
        inputDirectionZ = 0.0f;
        Hu3DModelObjPosGet(stageObject->mdlId[1], stageObjectNames[playerWork->playerIndex], &playerStagePosition);
        for (tokenIndex = 0; tokenIndex < (boardRoundOrder[playerWork->boardRound] == 2 ? 2 : 3); tokenIndex++) {
            Hu3DModelObjPosGet(playerObject->mdlId[playerWork->boardRound + 4], hitPartObjectNames[tokenIndex], &hitPartOffsets[tokenIndex]);
            hitPartOffsets[tokenIndex].x -= playerStagePosition.x;
            hitPartOffsets[tokenIndex].y -= playerStagePosition.y;
            hitPartOffsets[tokenIndex].z -= playerStagePosition.z;
        }
        for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
            Hu3DModelObjPosGet(playerObject->mdlId[playerWork->boardRound + 4], tokenTargetObjectNames[tokenIndex], &tokenTargetOffsets[tokenIndex]);
            tokenTargetOffsets[tokenIndex].x -= playerStagePosition.x;
            tokenTargetOffsets[tokenIndex].y -= playerStagePosition.y;
            tokenTargetOffsets[tokenIndex].z -= playerStagePosition.z;
        }
        switch (playerWork->playerState) {
            case 0:
                if (gameState == 0x3EC) {
                    playerWork->playerState = 1;
                    playerWork->activeTokenIndex = playerWork->aiTarget % 4;
                    playerWork->activeTokenIndex = playerWork->actionTimer = playerWork->previousHitTarget = playerWork->consecutiveHits = playerWork->hitCooldown = 0;
                    playerWork->currentAction = -1;
                    playerWork->rotationX = playerWork->rotationZ = playerWork->score = playerWork->hitLimit = 0;
                    playerWork->isCpuPlayer = playerWork->controllerIndex == -1 ? 1 : 0;
                    if (playerWork->controllerIndex == -1) {
                        aiPatternIndex = playerPatternValues[playerWork->playerIndex];
                        playerWork->aiTarget = aiTargetByRoundDifficultyPattern[boardRoundOrder[playerWork->boardRound]][playerWork->difficulty][aiPatternIndex];
                        playerWork->aiMoveX = aiMoveXByRoundDifficultyPattern[boardRoundOrder[playerWork->boardRound]][playerWork->difficulty][aiPatternIndex];
                        playerWork->aiMoveZ = aiMoveZByRoundDifficultyPattern[boardRoundOrder[playerWork->boardRound]][playerWork->difficulty][aiPatternIndex];
                    }
                    for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
                        playerWork->tokenVelocityX[tokenIndex] = playerWork->tokenVelocityY[tokenIndex] = playerWork->tokenVelocityZ[tokenIndex] = playerWork->tokenAngularVelocity[tokenIndex]
                            = playerWork->targetAngularVelocity[tokenIndex] = 0.0f;
                        playerWork->tokenActive[tokenIndex] = playerWork->tokenAction[tokenIndex] = playerWork->tokenHitMask[tokenIndex] = playerWork->tokenPushMask[tokenIndex]
                            = playerWork->obstacleMask[tokenIndex] = playerWork->collisionMask[tokenIndex] = 0;
                        playerWork->tokenPitch[tokenIndex] = playerWork->tokenRoll[tokenIndex] = playerWork->tokenSpin[tokenIndex] = 0;
                    }
                }
                break;
            case 1:
                if (winnerIndex != -1) {
                    playerWork->playerState = 3;
                    if ((winnerIndex == playerWork->playerIndex) && (playerWork->tokenActive[0] != 0) && (playerWork->tokenActive[1] != 0) && (playerWork->tokenActive[2] != 0)
                        && (playerWork->tokenActive[3] != 0)) {
                        playerWork->noticeTimer = 0x1E;
                        HuAudFXPlay(9);
                    }
                }
                else if ((playerWork->boardRound < 2) && (playerWork->tokenActive[0] != 0) && (playerWork->tokenActive[1] != 0) && (playerWork->tokenActive[2] != 0)
                    && (playerWork->tokenActive[3] != 0)) {
                    if (soundEffectQueue[3].pendingSoundCount < 2) {
                        soundEffectQueue[3].soundQueueState[soundEffectQueue[3].pendingSoundCount] = 0.0f;
                        soundEffectQueue[3].pendingSoundCount++;
                    }
                    playerWork->playerState = 2;
                    playerWork->stateTimer = 0;
                    Hu3DMotionSpeedSet(playerObject->mdlId[playerWork->boardRound + 1], -1.0f);
                    playerWork->activeTokenIndex = playerWork->actionTimer = playerWork->previousHitTarget = playerWork->consecutiveHits = playerWork->hitCooldown = 0;
                    playerWork->currentAction = -1;
                    playerWork->rotationX = playerWork->rotationZ = playerWork->score = playerWork->hitLimit = 0;
                    playerWork->isCpuPlayer = playerWork->controllerIndex == -1 ? 1 : 0;
                    if (playerWork->controllerIndex == -1) {
                        nextAiPatternIndex = (playerWork->boardRound + playerPatternValues[playerWork->playerIndex] + 1) % 3;
                        playerWork->aiTarget = aiTargetByRoundDifficultyPattern[boardRoundOrder[playerWork->boardRound + 1]][playerWork->difficulty][nextAiPatternIndex];
                        playerWork->aiMoveX = aiMoveXByRoundDifficultyPattern[boardRoundOrder[playerWork->boardRound + 1]][playerWork->difficulty][nextAiPatternIndex];
                        playerWork->aiMoveZ = aiMoveZByRoundDifficultyPattern[boardRoundOrder[playerWork->boardRound + 1]][playerWork->difficulty][nextAiPatternIndex];
                    }
                    for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
                        playerWork->tokenVelocityX[tokenIndex] = playerWork->tokenVelocityY[tokenIndex] = playerWork->tokenVelocityZ[tokenIndex] = playerWork->tokenAngularVelocity[tokenIndex]
                            = playerWork->targetAngularVelocity[tokenIndex] = 0.0f;
                        playerWork->tokenActive[tokenIndex] = playerWork->tokenAction[tokenIndex] = playerWork->tokenHitMask[tokenIndex] = playerWork->tokenPushMask[tokenIndex]
                            = playerWork->obstacleMask[tokenIndex] = playerWork->collisionMask[tokenIndex] = 0;
                        playerWork->tokenPitch[tokenIndex] = playerWork->tokenRoll[tokenIndex] = playerWork->tokenSpin[tokenIndex] = 0;
                    }
                    for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
                        nearestInactiveTokenIndex = -1;
                        nearestInactiveTokenDistance = 10000.0f;
                        Hu3DModelObjPosGet(playerObject->mdlId[4], tokenObjectNames[tokenIndex], &tokenTargetPosition);
                        for (collisionTargetIndex = 0; collisionTargetIndex < 4; collisionTargetIndex++) {
                            tokenPositionDeltaX = tokenTargetPosition.x - playerWork->tokenX[collisionTargetIndex];
                            tokenPositionDeltaZ = tokenTargetPosition.z - playerWork->tokenZ[collisionTargetIndex];
                            if ((playerWork->tokenActive[collisionTargetIndex] == 0) && (sqrtf((tokenPositionDeltaX * tokenPositionDeltaX) + (tokenPositionDeltaZ * tokenPositionDeltaZ)) < nearestInactiveTokenDistance)) {
                                nearestInactiveTokenIndex = collisionTargetIndex;
                                nearestInactiveTokenDistance = sqrtf((tokenPositionDeltaX * tokenPositionDeltaX) + (tokenPositionDeltaZ * tokenPositionDeltaZ));
                            }
                        }
                        playerWork->lastHitBy[nearestInactiveTokenIndex] = tokenIndex;
                        playerWork->tokenActive[nearestInactiveTokenIndex] = 1;
                        playerWork->launchOriginX[tokenIndex] = playerWork->tokenX[tokenIndex];
                        playerWork->launchOriginZ[tokenIndex] = playerWork->tokenZ[tokenIndex];
                    }
                    playerWork->tokenActive[0] = playerWork->tokenActive[1] = playerWork->tokenActive[2] = playerWork->tokenActive[3] = 0;
                    for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
                        particleModelIndex = tokenIndex + 7;
                        tokenModel = &Hu3DData[playerObject->mdlId[tokenIndex + 0xC]];
                        Hu3DModelPosSet(playerObject->mdlId[particleModelIndex], tokenModel->pos.x, 0.0f, tokenModel->pos.z);
                        Hu3DModelAttrReset(playerObject->mdlId[particleModelIndex], HU3D_ATTR_DISPOFF);
                        Hu3DParticleCntSet(playerObject->mdlId[particleModelIndex], 0);
                    }
                    if (winnerIndex == -1) {
                        playerWork->noticeTimer = 0x1E;
                        HuAudFXPlay(9);
                    }
                }
                break;
            case 2:
                playerWork->stateTimer++;
                if (playerWork->stateTimer == 0xA) {
                    for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
                        Hu3DModelAttrSet(playerObject->mdlId[tokenIndex + 0xC], HU3D_ATTR_DISPOFF);
                    }
                }
                if (playerWork->stateTimer <= 0x3C) {
                    transitionTimerFraction = playerWork->stateTimer / 60.0;
                    for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) { }
                }
                playerWork->stateTimer == 0x3C;
                if ((playerWork->stateTimer >= 0) && (playerWork->stateTimer <= 0x3C)) {
                    transitionTimerFraction = playerWork->stateTimer / 60.0;
                    roundTransitionProgress = 0.5 + (-0.5 * cosd((180.0f * transitionTimerFraction)));
                    roundTransitionProgress = transitionTimerFraction < 0.75 ? 0 : 1;
                    for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
                        Hu3DModelObjPosGet(playerObject->mdlId[playerWork->boardRound + 5], tokenObjectNames[tokenIndex], &nextRoundTokenPosition);
                        playerWork->tokenX[tokenIndex] = (roundTransitionProgress * (nextRoundTokenPosition.x - playerStagePosition.x)) + (playerWork->launchOriginX[tokenIndex] * (1.0f - roundTransitionProgress));
                        playerWork->tokenZ[tokenIndex] = (roundTransitionProgress * (nextRoundTokenPosition.z - playerStagePosition.z)) + (playerWork->launchOriginZ[tokenIndex] * (1.0f - roundTransitionProgress));
                        playerWork->tokenY[tokenIndex] = 300.0 * sind((180.0f * transitionTimerFraction));
                        playerWork->tokenY[tokenIndex] = -25.0f * (1.0f - roundTransitionProgress);
                    }
                }
                if (playerWork->stateTimer == 0x3C) {
                    Hu3DMotionSpeedSet(playerObject->mdlId[playerWork->boardRound + 1], 1.0f);
                    Hu3DModelAttrSet(playerObject->mdlId[playerWork->boardRound + 1], HU3D_ATTR_DISPOFF);
                    playerWork->boardRound++;
                    Hu3DModelAttrReset(playerObject->mdlId[playerWork->boardRound + 1], HU3D_ATTR_DISPOFF);
                    Hu3DModelAttrReset(playerObject->mdlId[playerWork->boardRound + 1], HU3D_MOTATTR_LOOP);
                    Hu3DMotionTimeSet(playerObject->mdlId[playerWork->boardRound + 1], 0.0f);
                    for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
                        particleModelIndex = tokenIndex + 7;
                        effectModel = &Hu3DData[playerObject->mdlId[tokenIndex + 0xC]];
                        Hu3DModelPosSet(playerObject->mdlId[particleModelIndex], effectModel->pos.x, effectModel->pos.y, effectModel->pos.z);
                        Hu3DModelAttrReset(playerObject->mdlId[particleModelIndex], HU3D_ATTR_DISPOFF);
                        Hu3DModelAttrReset(playerObject->mdlId[tokenIndex + 0xC], HU3D_ATTR_DISPOFF);
                        Hu3DModelScaleSet(playerObject->mdlId[tokenIndex + 0xC], 0.0f, 0.0f, 0.0f);
                        Hu3DParticleCntSet(playerObject->mdlId[particleModelIndex], 0);
                    }
                }
                if ((playerWork->stateTimer >= 0x3E) && (playerWork->stateTimer <= 0x78)) {
                    roundScaleProgress = (playerWork->stateTimer - 0x3E) / 58.0;
                    roundModelScale = roundScaleProgress < 0.25 ? (roundScaleProgress / 0.25) : 1.0;
                    roundModelScale = playerWork->stateTimer < 0x46 ? 0 : 1;
                    for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
                        Hu3DModelScaleSet(playerObject->mdlId[tokenIndex + 0xC], roundModelScale, roundModelScale, roundModelScale);
                    }
                }
                if (playerWork->stateTimer == 0x78) {
                    playerWork->playerState = 1;
                    playerWork->stateTimer = 0;
                }
                break;
            case 3:
                break;
        }
        if (playerWork->noticeTimer > 0) {
            noticeAnimationProgress = 1.0 - ((playerWork->noticeTimer - 1) / 30.0);
            noticeSpriteIndex = playerWork->boardRound + 5 + (playerWork->playerIndex * 9);
            noticeAnimationProgress = (0.5 * noticeAnimationProgress) + (0.5 * (1.0f - ((1.0f - noticeAnimationProgress) * (1.0f - noticeAnimationProgress))));
            noticeTransparency = noticeAnimationProgress < 0.75 ? (noticeAnimationProgress / 0.75) : 1.0;
            noticeAnimationProgress = 1.0f - ((1.0f - noticeAnimationProgress) * (1.0f - noticeAnimationProgress));
            noticeTransparency = 1.0f - ((1.0f - noticeTransparency) * (1.0f - noticeTransparency));
            if (playerWork->noticeTimer == 0x1E) {
                espDispOn(scoreboardSpriteIds[noticeSpriteIndex]);
                espDispOff(scoreboardSpriteIds[playerWork->boardRound + 2 + (playerWork->playerIndex * 9)]);
            }
            espTPLvlSet(scoreboardSpriteIds[noticeSpriteIndex], noticeTransparency);
            espScaleSet(scoreboardSpriteIds[noticeSpriteIndex], 4.0f - (3.0f * noticeAnimationProgress), 4.0f - (3.0f * noticeAnimationProgress));
            playerWork->noticeTimer--;
        }
        if (playerWork->playerState == 1) {
            if (playerWork->controllerIndex != -1) {
                inputDirectionX = HuPadStkX[playerWork->controllerIndex];
                inputDirectionZ = HuPadStkY[playerWork->controllerIndex];
                inputDirectionX /= 40.0f;
                inputDirectionZ /= 40.0f;
            }
            else if (playerWork->isCpuPlayer != 0) {
                inputDirectionX = 3.0f * -playerWork->inputX;
                inputDirectionZ = -3.0f * -playerWork->inputY;
            }
            else if ((playerWork->controllerIndex == -1) && ((playerWork->aiMoveX != 0) || (playerWork->aiMoveZ != 0))) {
                inputDirectionX = playerWork->aiMoveX < 0 ? -1 : (playerWork->aiMoveX > 0 ? 1 : 0);
                inputDirectionZ = playerWork->aiMoveZ < 0 ? -1 : (playerWork->aiMoveZ > 0 ? 1 : 0);
                playerWork->aiMoveX = playerWork->aiMoveX + (playerWork->aiMoveX < 0 ? 1 : (playerWork->aiMoveX > 0 ? -1 : 0));
                playerWork->aiMoveZ = playerWork->aiMoveZ + (playerWork->aiMoveZ < 0 ? 1 : (playerWork->aiMoveZ > 0 ? -1 : 0));
            }
            else if ((playerWork->tokenActive[0] == 0) || (playerWork->tokenActive[1] == 0) || (((playerWork->tokenActive[2] == 0) != 0)) || (playerWork->tokenActive[3] == 0)) {
                if (playerWork->tokenActive[playerWork->activeTokenIndex] != 0) {
                    actionChoiceIndex = -1;
                    activeTokenCount = playerWork->tokenActive[3] + (playerWork->tokenActive[0] + playerWork->tokenActive[1] + playerWork->tokenActive[2]);
                    for (activeTokenSkipCount = 0; activeTokenSkipCount <= (activeTokenCount == 1 ? ((playerWork->aiTarget / 4) % 3) : (activeTokenCount == 2 ? (((playerWork->aiTarget / 4) / 3) % 2) : 0));
                         activeTokenSkipCount++) {
                        for (actionChoiceIndex++; playerWork->tokenActive[actionChoiceIndex] != 0; actionChoiceIndex++) { }
                    }
                    playerWork->activeTokenIndex = actionChoiceIndex;
                }
                activeTokenIndex = playerWork->activeTokenIndex;
                if (activeTokenIndex != -1) {
                    tokenPositionX = playerWork->tokenX[activeTokenIndex] + playerWork->tokenVelocityX[activeTokenIndex];
                    tokenPositionZ = playerWork->tokenZ[activeTokenIndex] + playerWork->tokenVelocityZ[activeTokenIndex];
                    nearestActionIndex = -1;
                    nearestActionDistanceSquared = -1.0f;
                    previousActionTimer = playerWork->actionTimer;
                    if (playerWork->tokenAction[playerWork->currentAction] != 0) {
                        playerWork->actionTimer = 0;
                        playerWork->currentAction = -1;
                    }
                    if (playerWork->actionTimer != 0) {
                        for (candidateTokenIndex = 0; candidateTokenIndex < 4; candidateTokenIndex++) {
                            candidateTokenPositionX = playerWork->tokenX[candidateTokenIndex] + playerWork->tokenVelocityX[candidateTokenIndex];
                            candidateTokenPositionZ = playerWork->tokenZ[candidateTokenIndex] + playerWork->tokenVelocityZ[candidateTokenIndex];
                            if (playerWork->tokenActive[candidateTokenIndex] == 0) {
                                if (playerWork->directionX != 0.0f && !(playerWork->directionX < 0.0f ? (candidateTokenPositionX < playerWork->targetX) : (candidateTokenPositionX > playerWork->targetX))) { }
                                else {
                                    if (playerWork->directionZ != 0.0f
                                        && !(playerWork->directionZ < 0.0f ? (candidateTokenPositionZ < playerWork->targetZ) : (candidateTokenPositionZ > playerWork->targetZ))) { }
                                    else {
                                        playerWork->actionTimer = 0;
                                        playerWork->activeTokenIndex = candidateTokenIndex;
                                        activeTokenIndex = candidateTokenIndex;
                                        tokenPositionX = candidateTokenPositionX;
                                        tokenPositionZ = candidateTokenPositionZ;
                                    }
                                }
                            }
                        }
                    }
                    switch (boardRoundOrder[playerWork->boardRound]) {
                        case 0:
                            if ((playerWork->tokenAction[0] != 0) && (playerWork->currentAction == 1) && (tokenPositionZ > 0.0f) && (tokenPositionX < 0.0f)) {
                                playerWork->actionTimer = 1;
                                playerWork->targetX = 150.0f;
                                playerWork->targetZ = 150.0f;
                                playerWork->directionX = 1.0f;
                                playerWork->directionZ = 1.0f;
                            }
                            if ((playerWork->tokenAction[0] != 0) && (playerWork->currentAction == 1) && (tokenPositionZ > 100.0f) && (tokenPositionX < 150.0f)) {
                                playerWork->actionTimer = 1;
                                playerWork->targetX = 150.0f;
                                playerWork->targetZ = 150.0f;
                                playerWork->directionX = 1.0f;
                                playerWork->directionZ = 1.0f;
                            }
                            if ((playerWork->tokenAction[0] != 0) && (playerWork->actionTimer == 0) && (previousActionTimer == 1)) {
                                playerWork->actionTimer = 2;
                                playerWork->targetZ = -50.0f;
                                playerWork->directionX = 0.0f;
                                playerWork->directionZ = -1.0f;
                            }
                            if ((playerWork->tokenAction[0] != 0) && (playerWork->currentAction >= 2) && (tokenPositionX < -100.0f) && (tokenPositionZ < -120.0f)) {
                                playerWork->actionTimer = 3;
                                playerWork->targetZ = -120.0f;
                                playerWork->directionX = 0.0f;
                                playerWork->directionZ = 1.0f;
                            }
                            if ((playerWork->tokenAction[0] != 0) && (playerWork->currentAction >= 2) && (playerWork->actionTimer == 0) && (tokenPositionX < 0.0f)
                                && (tokenPositionZ < 0.0f)) {
                                playerWork->actionTimer = 4;
                                playerWork->targetX = 0.0f;
                                playerWork->targetZ = -120.0f;
                                playerWork->directionX = 1.0f;
                                playerWork->directionZ = 1.0f;
                            }
                            if ((playerWork->tokenAction[0] != 0) && (playerWork->currentAction >= 2) && (playerWork->actionTimer == 0) && (previousActionTimer == 3)) {
                                playerWork->actionTimer = 5;
                                playerWork->targetX = 190.0f;
                                playerWork->targetZ = 190.0f;
                                playerWork->directionX = 1.0f;
                                playerWork->directionZ = 1.0f;
                            }
                            if ((playerWork->tokenAction[0] != 0) && (playerWork->actionTimer == 0) && (previousActionTimer == 5) && (playerWork->currentAction == 3)) {
                                playerWork->actionTimer = 6;
                                playerWork->targetX = 0.0f;
                                playerWork->directionX = -1.0f;
                                playerWork->directionZ = 0.0f;
                            }
                            break;
                        case 1:
                            if ((playerWork->currentAction == 3) && (tokenPositionZ > 75.0f) && ((tokenPositionX < -150.0f) || (tokenPositionX > 150.0f))) {
                                playerWork->actionTimer = 1;
                                playerWork->targetZ = 75.0f;
                                playerWork->directionX = 0.0f;
                                playerWork->directionZ = -1.0f;
                            }
                            if ((playerWork->tokenAction[1] != 0) && (tokenPositionZ < -120.0f) && (tokenPositionX > 100.0f)) {
                                playerWork->actionTimer = 2;
                                playerWork->targetZ = -120.0f;
                                playerWork->directionX = 0.0f;
                                playerWork->directionZ = 1.0f;
                            }
                            if ((playerWork->currentAction == 2) && (playerWork->tokenAction[0] != 0) && (tokenPositionZ < -120.0f) && (tokenPositionX < -120.0f)) {
                                playerWork->actionTimer = 3;
                                if (tokenPositionX < tokenPositionZ) {
                                    playerWork->targetX = -150.0f;
                                    playerWork->targetZ = -80.0f;
                                    playerWork->directionX = -1.0f;
                                    playerWork->directionZ = 1.0f;
                                }
                                else {
                                    playerWork->targetZ = -150.0f;
                                    playerWork->targetX = -80.0f;
                                    playerWork->directionZ = -1.0f;
                                    playerWork->directionX = 1.0f;
                                }
                            }
                            break;
                        case 2:
                            if ((playerWork->tokenAction[3] != 0) && (tokenPositionZ > 100.0f) && (tokenPositionX < -75.0f)) {
                                playerWork->actionTimer = 1;
                                playerWork->targetX = -75.0f;
                                playerWork->targetZ = 175.0f;
                                playerWork->directionX = 1.0f;
                                playerWork->directionZ = 1.0f;
                            }
                            if ((playerWork->tokenAction[3] != 0) && (tokenPositionZ < -50.0f) && (tokenPositionX < -150.0f)) {
                                playerWork->actionTimer = 2;
                                playerWork->targetX = -200.0f;
                                playerWork->targetZ = -25.0f;
                                playerWork->directionX = -1.0f;
                                playerWork->directionZ = 1.0f;
                            }
                            if ((playerWork->currentAction == 2) && (playerWork->tokenAction[2] != 0) && ((tokenPositionX + tokenPositionZ) > 300.0f)) {
                                playerWork->actionTimer = 3;
                                if (tokenPositionX > tokenPositionZ) {
                                    playerWork->targetX = 200.0f;
                                    playerWork->targetZ = 80.0f;
                                    playerWork->directionX = 1.0f;
                                    playerWork->directionZ = -1.0f;
                                }
                                else {
                                    playerWork->targetZ = 200.0f;
                                    playerWork->targetX = 80.0f;
                                    playerWork->directionZ = 1.0f;
                                    playerWork->directionX = -1.0f;
                                }
                            }
                            if ((playerWork->currentAction == 1) && (playerWork->tokenAction[2] != 0) && ((tokenPositionX - tokenPositionZ) > 300.0f)) {
                                playerWork->actionTimer = 4;
                                if (tokenPositionX > -tokenPositionZ) {
                                    playerWork->targetX = 200.0f;
                                    playerWork->targetZ = -80.0f;
                                    playerWork->directionX = 1.0f;
                                    playerWork->directionZ = 1.0f;
                                }
                                else {
                                    playerWork->targetZ = -200.0f;
                                    playerWork->targetX = 80.0f;
                                    playerWork->directionZ = -1.0f;
                                    playerWork->directionX = -1.0f;
                                }
                            }
                            break;
                        case 3:
                            if ((tokenPositionZ > 200.0f) && (tokenPositionX > 200.0f) && (playerWork->currentAction != 3)) {
                                playerWork->actionTimer = 2;
                                if (playerWork->currentAction == 2) {
                                    playerWork->targetX = -150.0f;
                                    playerWork->targetZ = 150.0f;
                                    playerWork->directionX = -1.0f;
                                    playerWork->directionZ = 1.0f;
                                }
                                else {
                                    playerWork->targetX = 150.0f;
                                    playerWork->targetZ = -150.0f;
                                    playerWork->directionX = 1.0f;
                                    playerWork->directionZ = -1.0f;
                                }
                            }
                            if ((tokenPositionZ > 0.0f) && (tokenPositionX > 0.0f) && (!(tokenPositionX < 100.0f) || !(tokenPositionZ < 100.0f)) && (playerWork->currentAction != 3)
                                && (playerWork->actionTimer == 0)) {
                                playerWork->actionTimer = 1;
                                playerWork->targetX = 200.0f;
                                playerWork->targetZ = 200.0f;
                                playerWork->directionX = 1.0f;
                                playerWork->directionZ = 1.0f;
                            }
                            if ((tokenPositionZ > 200.0f) && (tokenPositionX < -200.0f) && (playerWork->currentAction != 2)) {
                                playerWork->actionTimer = 4;
                                if (playerWork->currentAction == 3) {
                                    playerWork->targetX = 150.0f;
                                    playerWork->targetZ = 150.0f;
                                    playerWork->directionX = 1.0f;
                                    playerWork->directionZ = 1.0f;
                                }
                                else {
                                    playerWork->targetX = -150.0f;
                                    playerWork->targetZ = -150.0f;
                                    playerWork->directionX = -1.0f;
                                    playerWork->directionZ = -1.0f;
                                }
                            }
                            if ((tokenPositionZ > 0.0f) && (tokenPositionX < 0.0f) && (playerWork->currentAction != 2) && (playerWork->actionTimer == 0)
                                && ((playerWork->tokenAction[0] != 0) || (playerWork->currentAction == 3))) {
                                playerWork->actionTimer = 3;
                                playerWork->targetX = -200.0f;
                                playerWork->targetZ = 200.0f;
                                playerWork->directionX = -1.0f;
                                playerWork->directionZ = 1.0f;
                            }
                            if ((tokenPositionZ < -200.0f) && (tokenPositionX > 200.0f) && (playerWork->currentAction == 3) && (playerWork->actionTimer == 0)) {
                                playerWork->actionTimer = 9;
                                playerWork->targetX = 200.0f;
                                playerWork->targetZ = 0.0;
                                playerWork->directionX = 1.0f;
                                playerWork->directionZ = 1.0f;
                            }
                            if ((tokenPositionZ < -200.0f) && (tokenPositionX < -200.0f) && (playerWork->currentAction == 2) && (playerWork->actionTimer == 0)) {
                                playerWork->actionTimer = 8;
                                playerWork->targetX = -200.0f;
                                playerWork->targetZ = 0.0f;
                                playerWork->directionX = -1.0f;
                                playerWork->directionZ = 1.0f;
                            }
                            if ((tokenPositionZ < -200.0f) && (playerWork->currentAction >= 2) && (playerWork->actionTimer == 0)) {
                                playerWork->actionTimer = 7;
                                playerWork->targetZ = -200.0f;
                                playerWork->directionZ = -1.0f;
                                if (playerWork->currentAction == 3) {
                                    playerWork->targetX = 200.0f;
                                    playerWork->directionX = 1.0f;
                                }
                                else {
                                    playerWork->targetX = -200.0f;
                                    playerWork->directionX = -1.0f;
                                }
                            }
                            if ((tokenPositionZ < 0.0f) && (playerWork->currentAction >= 2) && (playerWork->actionTimer == 0)) {
                                playerWork->actionTimer = 6;
                                playerWork->targetZ = -200.0f;
                                playerWork->directionX = 0.0;
                                playerWork->directionZ = -1.0f;
                            }
                            if ((tokenPositionZ < -175.0f) && (tokenPositionX > 175.0f) && ((playerWork->currentAction == 0) || (playerWork->currentAction == 2))
                                && (playerWork->actionTimer == 0)) {
                                playerWork->actionTimer = 0xA;
                                playerWork->targetX = 0.0f;
                                playerWork->targetZ = -200.0f;
                                playerWork->directionX = -1.0f;
                                playerWork->directionZ = -1.0f;
                            }
                            break;
                    }
                    if (playerWork->currentAction == -1) {
                        for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
                            if ((playerWork->tokenAction[tokenIndex] == 0)
                                && ((targetDeltaX = tokenTargetOffsets[tokenIndex].x - playerWork->tokenX[activeTokenIndex], targetDeltaZ = tokenTargetOffsets[tokenIndex].z - playerWork->tokenZ[activeTokenIndex],
                                        ((nearestActionIndex == -1) != 0))
                                    || (((targetDeltaX * targetDeltaX) + (targetDeltaZ * targetDeltaZ)) < nearestActionDistanceSquared))) {
                                nearestActionIndex = tokenIndex;
                                nearestActionDistanceSquared = (targetDeltaX * targetDeltaX) + (targetDeltaZ * targetDeltaZ);
                            }
                        }
                        if (nearestActionIndex != -1) {
                            playerWork->currentAction = nearestActionIndex;
                        }
                    }
                    else {
                        nearestActionIndex = playerWork->currentAction;
                    }
                    if ((playerWork->currentAction != -1) && (playerWork->actionTimer == 0)) {
                        if (((playerWork->inputX < -0.95) || (playerWork->inputX > 0.95)) && ((playerWork->inputY < -0.95) || (playerWork->inputY > 0.95))
                            && (playerWork->consecutiveHits > 0xF) && (playerWork->hitCooldown < 0x3C)) {
                            inputDirectionX = tokenPositionX < tokenTargetOffsets[playerWork->currentAction].x ? 1 : (tokenPositionX > tokenTargetOffsets[playerWork->currentAction].x ? -1 : 0);
                            inputDirectionZ = tokenPositionZ < tokenTargetOffsets[playerWork->currentAction].z ? 1 : (tokenPositionZ > tokenTargetOffsets[playerWork->currentAction].z ? -1 : 0);
                        }
                        else {
                            inputDirectionX = tokenPositionX < tokenTargetOffsets[playerWork->currentAction].x ? 1 : (tokenPositionX > tokenTargetOffsets[playerWork->currentAction].x ? -1 : 0);
                            inputDirectionZ = tokenPositionZ < tokenTargetOffsets[playerWork->currentAction].z ? 1 : (tokenPositionZ > tokenTargetOffsets[playerWork->currentAction].z ? -1 : 0);
                        }
                    }
                    else if (playerWork->actionTimer != 0) {
                        if (playerWork->directionX != 0.0f && !(playerWork->directionX < 0.0f ? (tokenPositionX < playerWork->targetX) : (tokenPositionX > playerWork->targetX))) {
                            inputDirectionX = playerWork->directionX < 0.0f ? (tokenPositionX > playerWork->targetX ? -1 : 0) : (tokenPositionX < playerWork->targetX ? 1 : 0);
                        }
                        else {
                            inputDirectionX = playerWork->inputX < -0.1 ? 1.0f : (playerWork->inputX > 0.1 ? -1.0f : (-10.0f * playerWork->inputX));
                        }
                        if (playerWork->directionZ != 0.0f && (playerWork->directionZ < 0.0f ? (tokenPositionZ < playerWork->targetZ) : (tokenPositionZ > playerWork->targetZ)) == 0) {
                            inputDirectionZ = playerWork->directionZ < 0.0f ? (tokenPositionZ > playerWork->targetZ ? -1 : 0) : (tokenPositionZ < playerWork->targetZ ? 1 : 0);
                        }
                        else {
                            inputDirectionZ = playerWork->inputY < -0.1 ? 1.0f : (playerWork->inputY > 0.1 ? -1.0f : (-10.0f * playerWork->inputY));
                        }
                    }
                    for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
                        tokenPreviousXDelta = playerWork->tokenX[tokenIndex] - playerWork->tokenPitch[tokenIndex];
                        tokenPreviousZDelta = playerWork->tokenZ[tokenIndex] - playerWork->tokenRoll[tokenIndex];
                        if (((tokenPreviousXDelta * tokenPreviousXDelta) + (tokenPreviousZDelta * tokenPreviousZDelta)) < 9.0f) {
                            playerWork->tokenSpin[tokenIndex]++;
                        }
                        else {
                            playerWork->tokenPitch[tokenIndex] = playerWork->tokenX[tokenIndex];
                            playerWork->tokenRoll[tokenIndex] = playerWork->tokenZ[tokenIndex];
                            playerWork->tokenSpin[tokenIndex] = 0;
                        }
                    }
                    if (playerWork->score <= 0) {
                        playerWork->rotationX = inputDirectionX;
                        playerWork->rotationZ = inputDirectionZ;
                        playerWork->score = playerWork->hitLimit;
                        playerWork->hitLimit -= (playerWork->hitLimit >= (playerWork->hitLimit / 10) ? (playerWork->hitLimit / 10) : 0);
                    }
                    else {
                        inputDirectionX = playerWork->rotationX;
                        inputDirectionZ = playerWork->rotationZ;
                        playerWork->score--;
                    }
                    inputDirectionX *= 0.5;
                    inputDirectionZ *= -0.5;
                }
            }
            if (((inputDirectionX * inputDirectionX) + (inputDirectionZ * inputDirectionZ)) > 1.0f) {
                inputMagnitude = sqrt((inputDirectionX * inputDirectionX) + (inputDirectionZ * inputDirectionZ));
                inputDirectionX *= 1.0f / inputMagnitude;
                inputDirectionZ *= 1.0f / inputMagnitude;
            }
            if ((((inputDirectionX * inputDirectionX) + (inputDirectionZ * inputDirectionZ)) < 0.0625) && (playerWork->isCpuPlayer == 0)) {
                inputDirectionX = 0.0f;
                inputDirectionZ = 0.0f;
            }
        }
        previousInputX = playerWork->inputX;
        previousInputY = playerWork->inputY;
        playerWork->inputX = playerWork->inputX + (inputDirectionX / 3.0f);
        playerWork->inputX = playerWork->inputX < -1.0f ? -1.0f : (playerWork->inputX > 1.0f ? 1.0f : playerWork->inputX);
        playerWork->inputY = playerWork->inputY + (-inputDirectionZ / 3.0f);
        playerWork->inputY = playerWork->inputY < -1.0f ? -1.0f : (playerWork->inputY > 1.0f ? 1.0f : playerWork->inputY);
        playerWork->isCpuPlayer = ((playerWork->inputX == 0.0f) && (playerWork->inputY == 0.0f)) ? 0 : playerWork->isCpuPlayer;
        if ((gameState == 0x3EC) && ((previousInputX != playerWork->inputX) || (previousInputY != playerWork->inputY))) {
            if (playerWork->winnerIndex == -1) {
                playerWork->winnerIndex = HuAudFXPlay(0x7A6);
            }
        }
        else {
            if (playerWork->winnerIndex != -1) {
                HuAudFXStop(playerWork->winnerIndex);
            }
            playerWork->winnerIndex = -1;
        }
        if (gameState >= 0x3ED) {
            inputDeadZone = 0.666f;
            inputSmoothingFactor = 0.9f;
            playerWork->inputY = playerWork->inputY > inputDeadZone
                ? ((playerWork->inputY * inputSmoothingFactor) + (inputDeadZone * (1.0f - inputSmoothingFactor)))
                : (playerWork->inputY < -inputDeadZone ? ((playerWork->inputY * inputSmoothingFactor) - (inputDeadZone * (1.0f - inputSmoothingFactor))) : playerWork->inputY);
        }
        if (playerWork->playerState != 2) {
            for (inputAxisIndex = 0; inputAxisIndex < 2; inputAxisIndex++) {
                for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
                    playerWork->tokenHitMask[tokenIndex] = playerWork->tokenPushMask[tokenIndex] = playerWork->obstacleMask[tokenIndex] = playerWork->collisionMask[tokenIndex] = 0;
                    if (playerWork->tokenActive[tokenIndex] == 0) {
                        playerWork->tokenVelocityX[tokenIndex] += 0.13888888888888887 * playerWork->inputX * (playerWork->isCpuPlayer != 0 ? 0 : 1)
                            * (playerWork->tokenY[tokenIndex] >= 0.0f ? 1.0f : (1.0f - (playerWork->tokenY[tokenIndex] / -25.0f)));
                        playerWork->tokenVelocityZ[tokenIndex] += 0.13888888888888887 * playerWork->inputY * (playerWork->isCpuPlayer != 0 ? 0 : 1)
                            * (playerWork->tokenY[tokenIndex] >= 0.0f ? 1.0f : (1.0f - (playerWork->tokenY[tokenIndex] / -25.0f)));
                        playerWork->tokenY[tokenIndex] += playerWork->tokenVelocityY[tokenIndex];
                        playerWork->hitCooldown += playerWork->hitCooldown < 0x8000;
                        tokenPlanarSpeed
                            = sqrtf((playerWork->tokenVelocityX[tokenIndex] * playerWork->tokenVelocityX[tokenIndex]) + (playerWork->tokenVelocityZ[tokenIndex] * playerWork->tokenVelocityZ[tokenIndex]));
                        previousTokenVelocityX = playerWork->tokenVelocityX[tokenIndex];
                        previousTokenVelocityZ = playerWork->tokenVelocityZ[tokenIndex];
                        if (tokenPlanarSpeed > 0.0f) {
                            if (tokenPlanarSpeed > 7.59375) {
                                playerWork->tokenVelocityX[tokenIndex] *= 7.5 / tokenPlanarSpeed;
                                playerWork->tokenVelocityZ[tokenIndex] *= 7.5 / tokenPlanarSpeed;
                            }
                            else {
                                playerWork->tokenVelocityX[tokenIndex] *= (tokenPlanarSpeed < 0.09375 ? 0.0 : (tokenPlanarSpeed - 0.09375)) / tokenPlanarSpeed;
                                playerWork->tokenVelocityZ[tokenIndex] *= (tokenPlanarSpeed < 0.09375 ? 0.0 : (tokenPlanarSpeed - 0.09375)) / tokenPlanarSpeed;
                            }
                        }
                        playerWork->tokenX[tokenIndex] += playerWork->tokenVelocityX[tokenIndex];
                        playerWork->tokenZ[tokenIndex] += playerWork->tokenVelocityZ[tokenIndex];
                        tokenPlanarSpeed
                            = sqrtf((playerWork->tokenVelocityX[tokenIndex] * playerWork->tokenVelocityX[tokenIndex]) + (playerWork->tokenVelocityZ[tokenIndex] * playerWork->tokenVelocityZ[tokenIndex]));
                        for (collisionTargetIndex = 0; collisionTargetIndex < 4; collisionTargetIndex++) {
                            tokenSeparationX = playerWork->tokenX[tokenIndex] - playerWork->tokenX[collisionTargetIndex];
                            tokenSeparationZ = playerWork->tokenZ[tokenIndex] - playerWork->tokenZ[collisionTargetIndex];
                            tokenSeparation = sqrtf((tokenSeparationX * tokenSeparationX) + (tokenSeparationZ * tokenSeparationZ));
                            if ((tokenIndex != collisionTargetIndex) && (playerWork->tokenActive[collisionTargetIndex] == 0) && (tokenSeparation > 0.0f) && (tokenSeparation < 62.5)) {
                                collisionNormalX = tokenSeparationX / tokenSeparation;
                                collisionNormalZ = tokenSeparationZ / tokenSeparation;
                                tokenPreImpactVelocityX = playerWork->tokenVelocityX[tokenIndex];
                                tokenPreImpactVelocityZ = playerWork->tokenVelocityZ[tokenIndex];
                                averageCollisionVelocityX = (playerWork->tokenVelocityX[tokenIndex] + playerWork->tokenVelocityX[collisionTargetIndex]) / 2.0;
                                averageCollisionVelocityZ = (playerWork->tokenVelocityZ[tokenIndex] + playerWork->tokenVelocityZ[collisionTargetIndex]) / 2.0;
                                firstTokenNormalVelocity = (collisionNormalX * -(playerWork->tokenVelocityX[tokenIndex] - averageCollisionVelocityX)) - (collisionNormalZ * (playerWork->tokenVelocityZ[tokenIndex] - averageCollisionVelocityZ));
                                secondTokenNormalVelocity = (-collisionNormalX * -(playerWork->tokenVelocityX[collisionTargetIndex] - averageCollisionVelocityX)) - (-collisionNormalZ * (playerWork->tokenVelocityZ[collisionTargetIndex] - averageCollisionVelocityZ));
                                playerWork->tokenX[tokenIndex] += collisionNormalX * (0.1 + (62.5 - tokenSeparation)) / 2.0;
                                playerWork->tokenZ[tokenIndex] += collisionNormalZ * (0.1 + (62.5 - tokenSeparation)) / 2.0;
                                playerWork->tokenX[collisionTargetIndex] -= collisionNormalX * (0.1 + (62.5 - tokenSeparation)) / 2.0;
                                playerWork->tokenZ[collisionTargetIndex] -= collisionNormalZ * (0.1 + (62.5 - tokenSeparation)) / 2.0;
                                playerWork->tokenVelocityX[tokenIndex] = averageCollisionVelocityX + (0.75 * ((playerWork->tokenVelocityX[tokenIndex] - averageCollisionVelocityX) + (2.0f * collisionNormalX * firstTokenNormalVelocity)));
                                playerWork->tokenVelocityZ[tokenIndex] = averageCollisionVelocityZ + (0.75 * ((playerWork->tokenVelocityZ[tokenIndex] - averageCollisionVelocityZ) + (2.0f * collisionNormalZ * firstTokenNormalVelocity)));
                                playerWork->tokenVelocityX[collisionTargetIndex] = averageCollisionVelocityX + (0.75 * ((playerWork->tokenVelocityX[collisionTargetIndex] - averageCollisionVelocityX) + (2.0f * -collisionNormalX * secondTokenNormalVelocity)));
                                playerWork->tokenVelocityZ[collisionTargetIndex] = averageCollisionVelocityZ + (0.75 * ((playerWork->tokenVelocityZ[collisionTargetIndex] - averageCollisionVelocityZ) + (2.0f * -collisionNormalZ * secondTokenNormalVelocity)));
                                playerWork->tokenHitMask[tokenIndex] = 1 << collisionTargetIndex;
                                if (((playerWork->tokenVelocityX[tokenIndex] + playerWork->tokenVelocityZ[tokenIndex]) > 0.0) && (playerWork->collisionTimer[tokenIndex] > 0xF)
                                    && (playerWork->collisionTimer[collisionTargetIndex] > 0xF) && (soundEffectQueue[1].pendingSoundCount < 2)) {
                                    soundEffectQueue[1].soundQueueState[soundEffectQueue[1].pendingSoundCount] = 0.0f;
                                    soundEffectQueue[1].pendingSoundCount++;
                                }
                                playerWork->collisionTimer[tokenIndex] = 0;
                                playerWork->collisionTimer[collisionTargetIndex] = 0;
                            }
                        }
                        for (collisionTargetIndex = 0; collisionTargetIndex < 4; collisionTargetIndex++) {
                            tokenDisplacementX = playerWork->tokenX[tokenIndex] - playerWork->tokenX[collisionTargetIndex];
                            tokenDisplacementZ = playerWork->tokenZ[tokenIndex] - playerWork->tokenZ[collisionTargetIndex];
                            tokenDistance = sqrtf((tokenDisplacementX * tokenDisplacementX) + (tokenDisplacementZ * tokenDisplacementZ));
                            if ((playerWork->tokenActive[collisionTargetIndex] != 0) && (tokenPlanarSpeed > 0.0f)) {
                                if (tokenDistance < 62.5) {
                                    tokenNormalX = tokenDisplacementX / tokenDistance;
                                    tokenNormalZ = tokenDisplacementZ / tokenDistance;
                                    tokenVelocityBeforeImpactX = playerWork->tokenVelocityX[tokenIndex];
                                    tokenVelocityBeforeImpactZ = playerWork->tokenVelocityZ[tokenIndex];
                                    normalVelocityProjection = (-tokenVelocityBeforeImpactX * tokenNormalX) - (tokenVelocityBeforeImpactZ * tokenNormalZ);
                                    playerWork->tokenX[tokenIndex] += tokenNormalX * (0.1 + (62.5 - tokenDistance));
                                    playerWork->tokenZ[tokenIndex] += tokenNormalZ * (0.1 + (62.5 - tokenDistance));
                                    playerWork->tokenVelocityX[tokenIndex] = 0.75 * (tokenVelocityBeforeImpactX + (2.0f * tokenNormalX * normalVelocityProjection));
                                    playerWork->tokenVelocityZ[tokenIndex] = 0.75 * (tokenVelocityBeforeImpactZ + (2.0f * tokenNormalZ * normalVelocityProjection));
                                    playerWork->tokenX[tokenIndex] += playerWork->tokenVelocityX[tokenIndex];
                                    playerWork->tokenZ[tokenIndex] += playerWork->tokenVelocityZ[tokenIndex];
                                    playerWork->consecutiveHits = playerWork->previousHitTarget == (collisionTargetIndex + 1) ? (playerWork->consecutiveHits + 1) : 1;
                                    playerWork->previousHitTarget = collisionTargetIndex + 1;
                                    playerWork->hitCooldown = 0;
                                    playerWork->tokenPushMask[tokenIndex] = 1 << collisionTargetIndex;
                                    if (((playerWork->tokenVelocityX[tokenIndex] + playerWork->tokenVelocityZ[tokenIndex]) > 0.0) && (playerWork->collisionTimer[tokenIndex] > 0xF)
                                        && (playerWork->collisionTimer[collisionTargetIndex] > 0xF) && (soundEffectQueue[1].pendingSoundCount < 2)) {
                                        soundEffectQueue[1].soundQueueState[soundEffectQueue[1].pendingSoundCount] = 0.0f;
                                        soundEffectQueue[1].pendingSoundCount++;
                                    }
                                    playerWork->collisionTimer[tokenIndex] = 0;
                                    playerWork->collisionTimer[collisionTargetIndex] = 0;
                                }
                            }
                        }
                        for (collisionTargetIndex = 0; collisionTargetIndex < (boardRoundOrder[playerWork->boardRound] == 2 ? 2 : 3); collisionTargetIndex++) {
                            obstacleSeparationX = playerWork->tokenX[tokenIndex] - hitPartOffsets[collisionTargetIndex].x;
                            obstacleSeparationZ = playerWork->tokenZ[tokenIndex] - hitPartOffsets[collisionTargetIndex].z;
                            obstacleDistance = sqrtf((obstacleSeparationX * obstacleSeparationX) + (obstacleSeparationZ * obstacleSeparationZ));
                            if ((obstacleDistance > 0.0f) && (obstacleDistance < 62.5)) {
                                obstacleNormalX = obstacleSeparationX / obstacleDistance;
                                obstacleNormalZ = obstacleSeparationZ / obstacleDistance;
                                obstacleVelocityX = playerWork->tokenVelocityX[tokenIndex];
                                obstacleVelocityZ = playerWork->tokenVelocityZ[tokenIndex];
                                obstacleNormalVelocity = (-obstacleVelocityX * obstacleNormalX) - (obstacleVelocityZ * obstacleNormalZ);
                                playerWork->tokenX[tokenIndex] += obstacleNormalX * (0.1 + (62.5 - obstacleDistance));
                                playerWork->tokenZ[tokenIndex] += obstacleNormalZ * (0.1 + (62.5 - obstacleDistance));
                                playerWork->tokenVelocityX[tokenIndex] = 0.75 * (obstacleVelocityX + (2.0f * obstacleNormalX * obstacleNormalVelocity));
                                playerWork->tokenVelocityZ[tokenIndex] = 0.75 * (obstacleVelocityZ + (2.0f * obstacleNormalZ * obstacleNormalVelocity));
                                playerWork->tokenX[tokenIndex] += playerWork->tokenVelocityX[tokenIndex];
                                playerWork->tokenZ[tokenIndex] += playerWork->tokenVelocityZ[tokenIndex];
                                playerWork->consecutiveHits = playerWork->previousHitTarget == (collisionTargetIndex + 5) ? (playerWork->consecutiveHits + 1) : 1;
                                playerWork->previousHitTarget = collisionTargetIndex + 5;
                                playerWork->hitCooldown = 0;
                                playerWork->obstacleMask[tokenIndex] = 1 << collisionTargetIndex;
                                if (((playerWork->tokenVelocityX[tokenIndex] + playerWork->tokenVelocityZ[tokenIndex]) > 0.0) && (playerWork->collisionTimer[tokenIndex] > 0xF)
                                    && (soundEffectQueue[1].pendingSoundCount < 2)) {
                                    soundEffectQueue[1].soundQueueState[soundEffectQueue[1].pendingSoundCount] = 0.0f;
                                    soundEffectQueue[1].pendingSoundCount++;
                                }
                                playerWork->collisionTimer[tokenIndex] = 0;
                            }
                        }
                        if (((playerWork->tokenX[tokenIndex] - 37.5) < -250.0) || ((37.5 + playerWork->tokenX[tokenIndex]) > 250.0)) {
                            playerWork->tokenX[tokenIndex] = (playerWork->tokenX[tokenIndex] - 37.5) < -250.0 ? -212.5 : 212.5;
                            playerWork->tokenVelocityX[tokenIndex] *= -0.75;
                            playerWork->tokenX[tokenIndex] += playerWork->tokenVelocityX[tokenIndex];
                            if (((playerWork->tokenVelocityX[tokenIndex] + playerWork->tokenVelocityZ[tokenIndex]) > 0.0) && (playerWork->collisionTimer[tokenIndex] > 0xF)
                                && (soundEffectQueue[1].pendingSoundCount < 2)) {
                                soundEffectQueue[1].soundQueueState[soundEffectQueue[1].pendingSoundCount] = 0.0f;
                                soundEffectQueue[1].pendingSoundCount++;
                            }
                            playerWork->collisionTimer[tokenIndex] = 0;
                        }
                        if (((playerWork->tokenZ[tokenIndex] - 37.5) < -250.0) || ((37.5 + playerWork->tokenZ[tokenIndex]) > 250.0)) {
                            playerWork->tokenZ[tokenIndex] = (playerWork->tokenZ[tokenIndex] - 37.5) < -250.0 ? -212.5 : 212.5;
                            playerWork->tokenVelocityZ[tokenIndex] *= -0.75;
                            playerWork->tokenZ[tokenIndex] += playerWork->tokenVelocityZ[tokenIndex];
                            if (((playerWork->tokenVelocityX[tokenIndex] + playerWork->tokenVelocityZ[tokenIndex]) > 0.0) && (playerWork->collisionTimer[tokenIndex] > 0xF)
                                && (soundEffectQueue[1].pendingSoundCount < 2)) {
                                soundEffectQueue[1].soundQueueState[soundEffectQueue[1].pendingSoundCount] = 0.0f;
                                soundEffectQueue[1].pendingSoundCount++;
                            }
                            playerWork->collisionTimer[tokenIndex] = 0;
                        }
                        collisionTimerIndex = 0;
                        for (collisionTimerIndex = 0; collisionTimerIndex < 0x10; collisionTimerIndex++) {
                            playerWork->collisionTimer[collisionTimerIndex]++;
                        }
                        unresolvedCollisionCount = 0;
                        for (collisionTargetIndex = 0; collisionTargetIndex < 4; collisionTargetIndex++) {
                            targetSeparationX = playerWork->tokenX[tokenIndex] - tokenTargetOffsets[collisionTargetIndex].x;
                            targetSeparationZ = ((targetSeparationX == 0.0f) && (playerWork->tokenZ[tokenIndex] == tokenTargetOffsets[collisionTargetIndex].z))
                                ? 1e-10
                                : (playerWork->tokenZ[tokenIndex] - tokenTargetOffsets[collisionTargetIndex].z);
                            tokenTargetDistance = sqrtf((targetSeparationX * targetSeparationX) + (targetSeparationZ * targetSeparationZ));
                            targetCollisionBlocked = 0;
                            for (hitMaskIndex = 0; hitMaskIndex < 4; hitMaskIndex++) {
                                if (((playerWork->tokenHitMask[tokenIndex] & (1 << hitMaskIndex)) != 0) && (playerWork->obstacleMask[hitMaskIndex] != 0)) {
                                    targetCollisionBlocked = 1;
                                }
                            }
                            if ((playerWork->controllerIndex != -1) && (boardRoundOrder[playerWork->boardRound] == 1) && (collisionTargetIndex == 3)) {
                                if (playerWork->tokenHitStun[tokenIndex] > 0) {
                                    tokenTargetDistance = 1000.0f;
                                    playerWork->tokenHitStun[tokenIndex]--;
                                }
                                for (otherTokenIndex = 0; otherTokenIndex < 4; otherTokenIndex++) {
                                    if ((playerWork->playerIndex == 0) && ((HuPadBtn[0] & PAD_BUTTON_B) != 0)) {
                                        OSReport("%d | %d\n%d | %d\n%f,%f\n%f,%f\n", playerWork->tokenHitMask[tokenIndex], 1 << otherTokenIndex,
                                            playerWork->tokenHitMask[otherTokenIndex], 1 << tokenIndex, playerWork->tokenX[tokenIndex], playerWork->tokenZ[tokenIndex],
                                            playerWork->tokenX[otherTokenIndex], playerWork->tokenZ[otherTokenIndex]);
                                    }
                                    if ((((playerWork->tokenHitMask[tokenIndex] & (1 << otherTokenIndex)) != 0) || ((playerWork->tokenHitMask[otherTokenIndex] & (1 << tokenIndex)) != 0))
                                        && (playerWork->tokenZ[otherTokenIndex] > playerWork->tokenZ[tokenIndex]) && (playerWork->tokenX[tokenIndex] < 100.0f)
                                        && (playerWork->tokenX[tokenIndex] > -100.0f) && (playerWork->tokenZ[tokenIndex] > 150.0f)
                                        && (playerWork->tokenX[otherTokenIndex] < 100.0f) && (playerWork->tokenX[otherTokenIndex] > -100.0f)
                                        && (playerWork->tokenZ[otherTokenIndex] > 150.0f)) {
                                        tokenTargetDistance = 1000.0f;
                                        playerWork->tokenHitStun[tokenIndex] = 3;
                                    }
                                }
                            }
                            if ((targetCollisionBlocked == 0) && (tokenTargetDistance < 37.5)) {
                                targetNormalX = targetSeparationX / tokenTargetDistance;
                                targetNormalZ = targetSeparationZ / tokenTargetDistance;
                                verticalCollisionOffset = 37.5 * sind((90.0 * (-playerWork->tokenY[tokenIndex] / 37.5)));
                                verticalCollisionAngle = playerWork->tokenY[tokenIndex] <= -37.5 ? 90.0 : (asind(1.0 + (playerWork->tokenY[tokenIndex] / 37.5)));
                                playerWork->collisionMask[tokenIndex] = 1 << collisionTargetIndex;
                                tokenSpeed3D = sqrtf((playerWork->tokenVelocityZ[tokenIndex] * playerWork->tokenVelocityZ[tokenIndex])
                                    + ((playerWork->tokenVelocityX[tokenIndex] * playerWork->tokenVelocityX[tokenIndex])
                                        + (playerWork->tokenVelocityY[tokenIndex] * playerWork->tokenVelocityY[tokenIndex])));
                                tokenPlanarSpeed = sqrtf(
                                    (playerWork->tokenVelocityX[tokenIndex] * playerWork->tokenVelocityX[tokenIndex]) + (playerWork->tokenVelocityZ[tokenIndex] * playerWork->tokenVelocityZ[tokenIndex]));
                                tokenElevationAngle = atan2d(playerWork->tokenVelocityY[tokenIndex], tokenPlanarSpeed);
                                playerWork->tokenVelocityX[tokenIndex] -= 0.375 * targetNormalX;
                                playerWork->tokenVelocityZ[tokenIndex] -= 0.375 * targetNormalZ;
                                playerWork->tokenX[tokenIndex] -= 0.375 * targetNormalX;
                                playerWork->tokenZ[tokenIndex] -= 0.375 * targetNormalZ;
                                if (playerWork->tokenY[tokenIndex] < (-25.0 * sind((90.0 * (1.0 - (tokenTargetDistance / 37.5)))))) {
                                    playerWork->tokenY[tokenIndex] = -25.0 * sind((90.0 * (1.0 - (tokenTargetDistance / 37.5))));
                                }
                                if ((playerWork->tokenY[tokenIndex] <= -24.5) && (tokenTargetDistance < 0.75)) {
                                    playerWork->tokenY[tokenIndex] = -25.0f;
                                    playerWork->tokenVelocityY[tokenIndex] = 0.0f;
                                    playerWork->tokenActive[tokenIndex] = 1;
                                    playerWork->tokenAction[collisionTargetIndex] = 1;
                                    playerWork->hitLimit = scoreGoalByDifficulty[playerWork->difficulty];
                                    if (soundEffectQueue[2].pendingSoundCount < 2) {
                                        soundEffectQueue[2].soundQueueState[soundEffectQueue[2].pendingSoundCount] = 0.0f;
                                        soundEffectQueue[2].pendingSoundCount++;
                                    }
                                    omVibrate(playerWork->playerNumber, 0xC, 6, 6);
                                }
                            }
                            else {
                                unresolvedCollisionCount++;
                            }
                            if (collisionTargetIndex == 3) {
                                if ((unresolvedCollisionCount == 4) && (playerWork->tokenY[tokenIndex] < 0.0f)) {
                                    playerWork->tokenY[tokenIndex] = 0.0f;
                                    playerWork->tokenVelocityY[tokenIndex] *= -0.0;
                                }
                                else {
                                    playerWork->tokenVelocityY[tokenIndex] += -0.1875;
                                }
                            }
                        }
                        previousTokenHeading = atan2d(previousTokenVelocityX, previousTokenVelocityZ);
                        tokenHeading = atan2d(playerWork->tokenVelocityX[tokenIndex], playerWork->tokenVelocityZ[tokenIndex]);
                        tokenSpinSpeed
                            = sqrtf((playerWork->tokenVelocityX[tokenIndex] * playerWork->tokenVelocityX[tokenIndex]) + (playerWork->tokenVelocityZ[tokenIndex] * playerWork->tokenVelocityZ[tokenIndex]));
                        tokenRotationDelta = tokenHeading - previousTokenHeading;
                        tokenRotationDelta += tokenRotationDelta < -180.0f ? 0x168 : (tokenRotationDelta > 180.0f ? -0x168 : 0);
                        tokenRotationDelta += tokenRotationDelta < -180.0f ? 0x168 : (tokenRotationDelta > 180.0f ? -0x168 : 0);
                        tokenSpinSpeed = tokenSpinSpeed <= (1.03125 * sqrt(0.03858024691358024)) ? 0.0f : tokenSpinSpeed;
                        playerWork->tokenAngularVelocity[tokenIndex]
                            -= (2.625 * ((tokenSpinSpeed * (tokenRotationDelta < -90.0f ? -1.0 : (tokenRotationDelta > 90.0f ? 1.0 : (tokenRotationDelta / 90.0)))) / 3.75));
                        playerWork->tokenAngularVelocity[tokenIndex] = playerWork->tokenAngularVelocity[tokenIndex] >= 7.875 ? 7.875 : playerWork->tokenAngularVelocity[tokenIndex];
                        playerWork->tokenRotation[tokenIndex] += playerWork->tokenAngularVelocity[tokenIndex];
                        playerWork->tokenRotation[tokenIndex] += playerWork->tokenRotation[tokenIndex] < -180.0f ? 0xB4 : (playerWork->tokenRotation[tokenIndex] > 180.0f ? -0xB4 : 0);
                        if (fabs(tokenSpinSpeed) < fabs(playerWork->targetAngularVelocity[tokenIndex])) {
                            playerWork->tokenAngularVelocity[tokenIndex]
                                = playerWork->targetAngularVelocity[tokenIndex] == 0.0f ? 0.0f : ((playerWork->tokenAngularVelocity[tokenIndex] * tokenSpinSpeed) / playerWork->targetAngularVelocity[tokenIndex]);
                        }
                        playerWork->targetAngularVelocity[tokenIndex] = tokenSpinSpeed;
                    }
                }
            }
        }
        Hu3DModelRotSet(playerObject->mdlId[playerWork->boardRound + 1], 10.0f * playerWork->inputY, 0.0f, 10.0f * -playerWork->inputX);
        Hu3DModelRotSet(playerObject->mdlId[11], 10.0f * playerWork->inputY, 0.0f, 10.0f * -(playerWork->inputX));
        if (gameState == 0x3EB) {
            previousPlayerYaw = playerObject->rot.y;
            targetPlayerYaw = 180.0 + ((8.0f * playerWork->inputX) * (0.125 + ((0.6 + playerWork->inputY) / 0.875)));
            playerObject->rot.y = (3.0f + previousPlayerYaw) < targetPlayerYaw ? (3.0f + previousPlayerYaw) : ((previousPlayerYaw - 3.0f) > targetPlayerYaw ? (previousPlayerYaw - 3.0f) : targetPlayerYaw);
            if (stateFrameCount == 0xA) {
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[1], 39.0 * (0.5 + (0.5 * playerWork->inputY)), 20.0f, HU3D_MOTATTR_PAUSE);
            }
        }
        else if (gameState == 0x3EC) {
            omSetRot(playerObject, 0.0f, 180.0 + ((8.0f * playerWork->inputX) * (0.125 + ((0.6 + playerWork->inputY) / 0.875))), 0.0f);
            Hu3DMotionTimeSet(playerObject->mdlId[0], 39.0 * (0.5 + (0.5 * playerWork->inputY)));
        }
        else if (gameState == 0x3ED) {
            if (stateFrameCount == 1) {
                Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_PAUSE);
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[0], 0.0f, 12.0f, HU3D_MOTATTR_LOOP);
            }
            if (stateFrameCount >= 0x19) {
                playerObject->rot.y *= 0.8;
            }
        }
        else if (gameState >= 0x3EE) {
            playerObject->rot.y = 0.0f;
            if ((stateFrameCount == 1) && (gameState == 0x3EE)) {
                if ((playerWork->playerIndex != winnerIndex) && (playerWork->characterId != -1)) {
                    s32 characterReactionSoundIds[8] = { 0x121, 0x161, 0x1A1, 0x1E1, 0x221, 0x261, 0x2A1, 0x2E1 };
                    HuAudFXPlay(characterReactionSoundIds[playerWork->characterId]);
                }
                Hu3DMotionShiftSet(playerObject->mdlId[0], playerObject->mtnId[playerWork->playerIndex == winnerIndex ? 2 : 3], 0.0f, 8.0f,
                    ((playerWork->playerIndex == winnerIndex) && (playerWork->characterId == -1)) ? 0x40000001 : 0);
            }
        }
        for (tokenIndex = 0; tokenIndex < 4; tokenIndex++) {
            cameraTargetValue = playerWork->tokenRotation[tokenIndex];
            cameraTargetValue = cameraTargetValue < -180.0f ? (360.0f + cameraTargetValue) : (cameraTargetValue > 180.0f ? (cameraTargetValue - 360.0f) : cameraTargetValue);
            safeTokenRotation = ((-90.175 <= cameraTargetValue) && (cameraTargetValue <= -90.0f)) != 0
                ? -90.175
                : ((-90.0f <= cameraTargetValue) && (cameraTargetValue <= -89.825) ? -89.825 : cameraTargetValue);
            MTXRotRad(rotationMatrixScratch, 0x78, 0.017453292f * (10.0f * playerWork->inputY));
            MTXRotRad(inputRotationMatrix, 0x7A, 0.017453292f * (10.0f * -playerWork->inputX));
            MTXConcat(inputRotationMatrix, rotationMatrixScratch, inputRotationMatrix);
            MTXRotRad(rotationMatrixScratch, 0x79, 0.017453292f * safeTokenRotation);
            MTXRotRad(tokenRotationMatrix, 0x78, 0.017453292f * (15.0f * playerWork->inputY));
            MTXConcat(tokenRotationMatrix, rotationMatrixScratch, tokenRotationMatrix);
            MTXRotRad(rotationMatrixScratch, 0x7A, 0.017453292f * (15.0f * -playerWork->inputX));
            MTXConcat(rotationMatrixScratch, tokenRotationMatrix, tokenRotationMatrix);
            Hu3DMtxRotGet(tokenRotationMatrix, &tokenEulerRotation);
            tokenLocalPosition.x = playerWork->tokenX[tokenIndex];
            tokenLocalPosition.y = playerWork->tokenY[tokenIndex];
            tokenLocalPosition.z = playerWork->tokenZ[tokenIndex];
            MTXMultVec(inputRotationMatrix, &tokenLocalPosition, &tokenLocalPosition);
            Hu3DModelPosSet(playerObject->mdlId[tokenIndex + 0xC], playerStagePosition.x + tokenLocalPosition.x, playerStagePosition.y + tokenLocalPosition.y, playerStagePosition.z + tokenLocalPosition.z);
            Hu3DModelRotSet(playerObject->mdlId[tokenIndex + 0xC], 10.0f * playerWork->inputY, 0.0f, 10.0f * -playerWork->inputX);
            Hu3DModelRotSet(playerObject->mdlId[tokenIndex + 0xC], tokenEulerRotation.x, tokenEulerRotation.y, tokenEulerRotation.z);
        }
    }
}

s32 cameraSceneMode = -1;
float cameraStartZoom = 700.0f;
float cameraStartCenterY = 314.0f;
float cameraStartCenterZ = -100.0f;
float cameraStartPitch = -30.0f;

void M449GameCameraUpdate(OMOBJ *object)
{
    M449DllWork *playerWork[2];
    s32 cameraModeBeforeUpdate;
    float cameraBlend;

    s32 playerSlot;
    s32 playerZeroHasFullChain;
    s32 playerOneHasFullChain;
    s32 previousCameraMode;
    s32 playerZeroToken3ChainActive;
    s32 playerZeroToken2ChainActive;
    s32 playerZeroToken1ChainActive;
    s32 playerZeroToken0ChainActive;
    s32 playerOneToken3ChainActive;
    s32 playerOneToken2ChainActive;
    s32 playerOneToken1ChainActive;
    s32 playerOneToken0ChainActive;

    for (playerSlot = 0; playerSlot < 2; playerSlot++) {
        playerWork[playerSlot] = (&playerZeroObject)[playerSlot]->data;
    }
    currentRound++;
    if (gameState == 0x3EC) {
        gameTimer++;
    }
    playerZeroToken3ChainActive = 0;
    playerZeroToken2ChainActive = 0;
    playerZeroToken1ChainActive = 0;
    playerZeroToken0ChainActive = 0;
    if ((playerWork[0]->boardRound == 2) && (playerWork[0]->tokenActive[0] != 0)) {
        playerZeroToken0ChainActive = 1;
    }
    if ((playerZeroToken0ChainActive != 0) && (playerWork[0]->tokenActive[1] != 0)) {
        playerZeroToken1ChainActive = 1;
    }
    if ((playerZeroToken1ChainActive != 0) && (playerWork[0]->tokenActive[2] != 0)) {
        playerZeroToken2ChainActive = 1;
    }
    if ((playerZeroToken2ChainActive != 0) && (playerWork[0]->tokenActive[3] != 0)) {
        playerZeroToken3ChainActive = 1;
    }
    playerZeroHasFullChain = playerZeroToken3ChainActive;
    playerOneToken3ChainActive = 0;
    playerOneToken2ChainActive = 0;
    playerOneToken1ChainActive = 0;
    playerOneToken0ChainActive = 0;
    if ((playerWork[1]->boardRound == 2) && (playerWork[1]->tokenActive[0] != 0)) {
        playerOneToken0ChainActive = 1;
    }
    if ((playerOneToken0ChainActive != 0) && (playerWork[1]->tokenActive[1] != 0)) {
        playerOneToken1ChainActive = 1;
    }
    if ((playerOneToken1ChainActive != 0) && (playerWork[1]->tokenActive[2] != 0)) {
        playerOneToken2ChainActive = 1;
    }
    if ((playerOneToken2ChainActive != 0) && (playerWork[1]->tokenActive[3] != 0)) {
        playerOneToken3ChainActive = 1;
    }
    playerOneHasFullChain = playerOneToken3ChainActive;
    winnerIndex = winnerIndex == -1 ? ((playerZeroHasFullChain != 0) && (playerOneHasFullChain != 0) ? (rand8() % 2) : (playerZeroHasFullChain != 0 ? 0 : (playerOneHasFullChain != 0 ? 1 : winnerIndex)))
                                      : winnerIndex;
    winnerIndex = ((winnerIndex == -1) && (gameTimer >= REFRESH_RATE * 300)) != 0
        ? (playerWork[0]->boardRound > playerWork[1]->boardRound ? 0 : (playerWork[0]->boardRound < playerWork[1]->boardRound ? 1 : 4))
        : winnerIndex;
    previousCameraMode = cameraSceneMode;
    {
        M449CameraPreset cameraPresets[4] = {
            { 2500.0f, 0.0f, 0.0f, -100.0f, -40.0f, 0.0f },
            { 700.0f, 0.0f, 0.0f, -750.0f, -30.0f, 0.0f },
            { 775.0f, -350.0f, -27.0f, 400.0f, -25.0f, -7.0f },
            { 775.0f, 350.0f, -27.0f, 400.0f, -25.0f, 7.0f },
        };
        if (cameraSceneMode == -1) {
            cameraSceneMode = 1;
            cameraTransitionDirection = 1;
            cameraTransitionProgress = 1.0f;
        }
        if ((gameState == 0x3E9) && (cameraSceneMode == 1)) {
            cameraSceneMode = 0;
            cameraTransitionDirection = 1;
            cameraTransitionProgress = 0.0f;
        }
        if ((gameState == 0x3EE) && (stateFrameCount == 1) && (winnerIndex == 0)) {
            cameraSceneMode = 2;
            cameraTransitionDirection = 0;
            cameraTransitionProgress = 0.0f;
        }
        if ((gameState == 0x3EE) && (stateFrameCount == 1) && (winnerIndex == 1) && (playerWork[1]->characterId != -1)) {
            cameraSceneMode = 3;
            cameraTransitionDirection = 0;
            cameraTransitionProgress = 0.0f;
        }
        if ((previousCameraMode != cameraSceneMode) && (previousCameraMode != -1)) {
            cameraStartZoom = CZoom;
            cameraStartCenterY = Center.y;
            cameraStartCenterZ = Center.z;
            cameraStartPitch = CRot.x;
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
            }
            cameraTransitionProgress = cameraTransitionProgress > 1.0f ? 1.0f : cameraTransitionProgress;
            CZoom = (cameraBlend * cameraPresets[cameraSceneMode].zoom) + (cameraStartZoom * (1.0f - cameraBlend));
            Center.x = (cameraBlend * cameraPresets[cameraSceneMode].centerX) + (cameraStartCenterX * (1.0f - cameraBlend));
            Center.y = (cameraBlend * cameraPresets[cameraSceneMode].centerY) + (cameraStartCenterY * (1.0f - cameraBlend));
            Center.z = (cameraBlend * cameraPresets[cameraSceneMode].centerZ) + (cameraStartCenterZ * (1.0f - cameraBlend));
            CRot.x = (cameraBlend * cameraPresets[cameraSceneMode].pitch) + (cameraStartPitch * (1.0f - cameraBlend));
            CRot.y = (cameraBlend * cameraPresets[cameraSceneMode].yaw) + (cameraStartYaw * (1.0f - cameraBlend));
        }
    }
}

static const u32 tutorialMessageIds[6] = { 0x2C0003, 0x2C0004, 0x2C0005, 0x2C0006, 0x2C0007, 0 };

void M449TutorialMessageProcess(void)
{
    M449DllWork *playerWork[2];

    s16 messageWindowId;
    s32 messageIndex;
    s16 tutorialChoice;
    u32 tutorialMessageId;

    tutorialChoice = 0;
    for (messageIndex = 0; messageIndex < 2; messageIndex++) {
        playerWork[messageIndex] = (&playerZeroObject)[messageIndex]->data;
    }
    HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
    HuWinComKeyReset();
    messageWindowId = HuWinExCreateStyled(-10000.0f, 60.0f, 0x118, 0x78, -1, 1);
    HuWinExAnimIn(messageWindowId);
    HuWinBGTPLvlSet(messageWindowId, 0.8f);
    HuWinMesSpeedSet(messageWindowId, 1);
    HuWinDisablePlayerSet(
        messageWindowId, 0xF & ((playerWork[0]->controllerIndex != -1) ? ~(1 << playerWork[0]->controllerIndex) : 0xF) & ((playerWork[1]->controllerIndex != -1) ? ~(1 << playerWork[1]->controllerIndex) : 0xF));
    HuWinMesColSet(messageWindowId, 0);
    HuWinMesSet(messageWindowId, 0x2C0001);
    if (((playerWork[0]->controllerIndex == -1) || (playerWork[0]->characterId == -1)) && ((playerWork[1]->controllerIndex == -1) || (playerWork[1]->characterId == -1))) {
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
    }
    HuWinMesWait(messageWindowId);
    if ((playerWork[0]->controllerIndex != -1) || (playerWork[1]->controllerIndex != -1)) {
        HuWinMesColSet(messageWindowId, 0);
        HuWinMesSet(messageWindowId, 0x2C0002);
        if ((playerWork[0]->controllerIndex == -1) && (playerWork[1]->controllerIndex == -1)) {
            HuPrcSleep(REFRESH_RATE);
            tutorialChoice = 1;
        }
        else {
            tutorialChoice = HuWinChoiceGet(messageWindowId, 0);
        }
    }
    else {
        tutorialChoice = 1;
    }
    if (tutorialChoice == 0) {
        messageIndex = 0;
        while ((tutorialMessageId = tutorialMessageIds[messageIndex++]) != 0) {
            HuWinMesColSet(messageWindowId, 0);
            HuWinMesSet(messageWindowId, tutorialMessageId);
            if ((playerWork[0]->controllerIndex == -1 || playerWork[0]->characterId == -1) && (playerWork[1]->controllerIndex == -1 || playerWork[1]->characterId == -1)) {
                HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
            }
            HuWinMesWait(messageWindowId);
        }
    }
    else {
        HuWinMesColSet(messageWindowId, 0);
        HuWinMesSet(messageWindowId, 0x2C0008);
        if (((playerWork[0]->controllerIndex == -1) || (playerWork[0]->characterId == -1)) && ((playerWork[1]->controllerIndex == -1) || (playerWork[1]->characterId == -1))) {
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, REFRESH_RATE);
        }
        HuWinMesWait(messageWindowId);
    }
    HuWinExAnimOut(messageWindowId);
    HuWinExCleanup(messageWindowId);
    tutorialMessageActive = 0;
    HuPrcEnd();
}

void M449GameStateUpdate(OMOBJ *stateObject)
{
    M449DllWork *playerWork[2];
    s32 soundEffectId;

    s32 soundQueueIndex;
    s32 scoreboardIndex;

    for (soundQueueIndex = 0; soundQueueIndex < 0xC; soundQueueIndex++) {
        s32 soundEffectIds[12] = { 0x7A2, 0x7A3, 0x7A4, 0x7A5, 0, 0, 0, 0, 0, 0, 0, 0 };
        if (soundEffectQueue[soundQueueIndex].pendingSoundCount > 3) {
            soundEffectQueue[soundQueueIndex].pendingSoundCount = 3;
        }
        if ((soundEffectQueue[soundQueueIndex].pendingSoundCount > 0) && (soundQueueIndex != 1)) {
            soundEffectId = HuAudFXPlay(soundEffectIds[soundQueueIndex]);
            soundEffectQueue[soundQueueIndex].pendingSoundCount--;
        }
    }
    for (soundQueueIndex = 0; soundQueueIndex < 2; soundQueueIndex++) {
        playerWork[soundQueueIndex] = (&playerZeroObject)[soundQueueIndex]->data;
    }
    if ((omSysExitReq != 0) && (WipeStatGet() == 0)) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        stateObject->objFunc = M449Exit;
    }
    switch (gameState) {
        case 0x3E8:
            if (WipeStatGet() == 0) {
                gameState++;
                stateFrameCount = 0;
                return;
            }
            break;
        case 0x3E9:
            stateFrameCount++;
            if (stateFrameCount > 0x78) {
                gameState++;
                if ((playerWork[0]->characterId != -1) && (playerWork[1]->characterId != -1)) {
                    gameState++;
                }
                scoreboardState = 0;
                stateFrameCount = 0;
                return;
            }
            break;
        case 0x3EA:
            stateFrameCount++;
            if (stateFrameCount == 1) {
                HuPrcChildCreate(M449TutorialMessageProcess, 0x64, 0x2000, 0, HuPrcCurrentGet());
                tutorialMessageActive = 1;
                if (musicSequenceId == -1) {
                    musicSequenceId = HuAudSeqPlay(0x3D);
                    return;
                }
            }
            else if (tutorialMessageActive == 0) {
                gameState++;
                if (musicSequenceId != -1) {
                    HuAudSeqFadeOut(musicSequenceId, 0x3E8);
                    musicSequenceId = -1;
                }
                stateFrameCount = 0;
                return;
            }
            break;
        case 0x3EB:
            stateFrameCount++;
            if ((stateFrameCount == 5) && (tutorialSequenceId < 0)) {
                tutorialSequenceId = MGSeqCreate(3, 0);
            }
            if ((stateFrameCount >= 5) && (musicSequenceId == -1) && ((MGSeqStatGet(tutorialSequenceId) & 0x10) != 0)) {
                musicSequenceId = HuAudSeqPlay(0x3E);
            }
            if ((stateFrameCount >= 5) && (MGSeqStatGet(tutorialSequenceId) == 0) && (tutorialSequenceId >= 0)) {
                for (scoreboardIndex = 0; scoreboardIndex < 0x12; scoreboardIndex++) {
                    if (((scoreboardIndex % 9) < 5) || ((scoreboardIndex % 9) > 7)) {
                        espDispOn(scoreboardSpriteIds[scoreboardIndex]);
                    }
                }
                gameState++;
                tutorialSequenceId = -1;
                stateFrameCount = 0;
                return;
            }
            break;
        case 0x3EC:
            stateFrameCount++;
            if (gameTimer == REFRESH_RATE * 270) {
                countdownSequenceId = MGSeqCreate(1, 0x1E, -1, -1);
            }
            if (countdownSequenceId != -1) {
                MGSeqParamSet(countdownSequenceId, 1, ((REFRESH_RATE * 300 + REFRESH_RATE - 1) - gameTimer) / REFRESH_RATE);
            }
            if (winnerIndex != -1) {
                gameState = 0x3ED;
                stateFrameCount = 0;
                return;
            }
            break;
        case 0x3ED:
            stateFrameCount++;
            if ((tutorialSequenceId < 0) && (stateFrameCount == 1)) {
                if (countdownSequenceId != -1) {
                    MGSeqParamSet(countdownSequenceId, 2, -1);
                }
                tutorialSequenceId = MGSeqCreate(3, 1);
                if (musicSequenceId != -1) {
                    HuAudSeqFadeOut(musicSequenceId, 0x64);
                    musicSequenceId = -1;
                    return;
                }
            }
            else if (MGSeqStatGet(tutorialSequenceId) == 0) {
                gameState++;
                stateFrameCount = 0;
                if (winnerIndex == 4) {
                    winnerIndex = playerWork[0]->characterId == -1 ? 0 : (playerWork[1]->characterId == -1 ? 1 : winnerIndex);
                    return;
                }
            }
            break;
        case 0x3EE:
            stateFrameCount++;
            if (stateFrameCount == 1) {
                if (winnerIndex != 4) {
                    if (playerWork[winnerIndex]->characterId != -1) {
                        HuAudSStreamPlay(1);
                    }
                    else {
                        HuAudSStreamPlay(3);
                    }
                    GWPlayerCoinWinSet(playerWork[winnerIndex]->playerNumber, 10);
                }
                else if (winnerIndex == 4) {
                    HuAudSStreamPlay(4);
                }
            }
            if ((REFRESH_RATE_F * 3.5f + (winnerIndex == 4 ? 0x1E : 0)) < stateFrameCount) {
                tutorialSequenceId = -1;
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 0x3C);
                stateObject->objFunc = M449Exit;
            }
            break;
    }
}

void M449StageModelsInit(OMOBJ *stageObject)
{
    stageObject->mdlId[0] = Hu3DModelCreateFile(0x500008);
    Hu3DModelAttrSet(stageObject->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelShadowMapSet(stageObject->mdlId[0]);
    stageObject->mdlId[1] = Hu3DModelCreateFile(0x500009);
    stageObject->mdlId[2] = Hu3DModelCreateFile(0x50000A);
    Hu3DModelAttrSet(stageObject->mdlId[2], HU3D_MOTATTR_LOOP);
    stageObject->objFunc = M449GameCameraUpdate;
}

void M449TokenParticleUpdate(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    float particleAngle;
    float particleSpeedScale;

    HU3DPARTICLEDATA *particleData;
    s16 particleIndex;

    if (particle->count == 0) {
        for (particleData = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->time = 0;
            particleData->color.a = 0;
            particleData->scale = 0.0f;
        }
    }
    if (particle->count >= 0xFF) {
        for (particleData = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
            particleData->time = 0xFF;
            particleData->color.a = 0;
            particleData->scale = 0.0f;
        }
    }
    for (particleData = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (!particleData->scale) {
            break;
        }
    }
    for (particleData = particle->data, particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        if (particleData->time == 1) {
            particleAngle = ((frand() & 0xFF) * 0x168) >> 8;
            particleSpeedScale = 0.5 + (1.5 * (rand8() / 256.0));
            particleData->vel.x = 2.5 * (particleSpeedScale * sind(particleAngle));
            particleData->vel.y = 4.5 * (particleSpeedScale * cosd(particleAngle));
            particleData->vel.y *= particleData->vel.y < 0.0f ? -0.5 : 1.0;
            particleData->vel.x = (12.0 * (rand8() / 255.0)) - 6.0;
            particleData->vel.y = 12.0 * (rand8() / 255.0);
            particleData->vel.z = (rand8() / 255.0) - 0.5;
            particleData->pos.x = 2.0f * particleData->vel.x;
            particleData->pos.y = 10.0f + (2.0f * particleData->vel.y);
            particleData->pos.z = 50.0f + (2.0f * particleData->vel.z);
            particleData->color.r = 0xFF;
            particleData->color.g = 0xFF;
            particleData->color.b = 0xFF;
            particleData->color.a = 0xFF;
            particleData->scale = 50.0f;
        }
        if (particleData->time < 0xF0) {
            VECAdd(&particleData->vel, &particleData->pos, &particleData->pos);
            particleData->vel.x *= 0.9f;
            particleData->vel.z *= 0.9f;
            particleData->vel.y *= 0.9f;
            particleData->color.a = (particleData->color.a * 0xB) / 12;
            particleData->scale += 1.01f;
            if (particleData->scale <= 0.0f) {
                particleData->scale = 0.0f;
            }
            if (particleData->color.a == 0) {
                particleData->color.a = 0;
                particleData->scale = 0.0f;
            }
        }
        particleData->time += particleData->time < 0x8000;
        if (particleData->time >= 0xF0) {
            particleData->color.a = 0;
            particleData->scale = 0.0f;
        }
    }
}

void M449PlayerObjectInit(OMOBJ *playerObject)
{
    Vec tokenModelPosition;
    Vec playerModelPosition;
    void *opponentWork;

    M449DllWork *playerWork;
    s32 setupIndex;
    s32 particleModelIndex;
    s32 boardRoundChoice;
    s32 bonusCharacterPlayerIndex;
    M449DllWork *scoreboardPlayerWork;
    s32 specialPlayerNumber;
    s32 opponentPlayerConfigIndex;

    bonusCharacterPlayerIndex = -1;
    specialPlayerNumber = -1;
    playerWork = playerObject->data;
    opponentWork = (&playerZeroObject)[1 - playerObject->work[0]]->data;
    memset(playerWork, 0, sizeof(M449DllWork));
    playerWork->playerIndex = playerObject->work[0];
    if ((GWPlayerCfg->character == 0xC) || (GWPlayerCfg[1].character == 0xC) || (GWPlayerCfg[2].character == 0xC)
        || (GWPlayerCfg[3].character == 0xC)) {
        for (setupIndex = 0; setupIndex < 4; setupIndex++) {
            if (GWPlayerCfg[setupIndex].character == 0xC) {
                bonusCharacterPlayerIndex = setupIndex;
            }
        }
        specialPlayerNumber = GWPlayerCfg[bonusCharacterPlayerIndex].group == 0 ? 1 : 0;
        if (playerWork->playerIndex == 0) {
            playerWork->playerNumber = specialPlayerNumber;
            opponentPlayerConfigIndex = bonusCharacterPlayerIndex;
        }
        else {
            playerWork->playerNumber = bonusCharacterPlayerIndex;
            opponentPlayerConfigIndex = specialPlayerNumber;
        }
    }
    else {
        playerWork->playerNumber = GWPlayerCfg[0].group == playerWork->playerIndex
            ? 0
            : (GWPlayerCfg[1].group == playerWork->playerIndex
                    ? 1
                    : (GWPlayerCfg[2].group == playerWork->playerIndex ? 2 : (GWPlayerCfg[3].group == playerWork->playerIndex ? 3 : 4)));
        opponentPlayerConfigIndex = GWPlayerCfg->group == (1 - playerWork->playerIndex)
            ? 0
            : (GWPlayerCfg[1].group == (1 - playerWork->playerIndex)
                    ? 1
                    : (GWPlayerCfg[2].group == (1 - playerWork->playerIndex) ? 2 : (GWPlayerCfg[3].group == (1 - playerWork->playerIndex) ? 3 : 4)));
    }
    if (playerWork->playerNumber == 4) {
        OSReport("m448:wrong player group\n");
        playerWork->playerNumber = playerWork->playerIndex;
    }
    playerWork->characterId = ((bonusCharacterPlayerIndex != -1) && (playerWork->playerIndex == 1)) != 0 ? -1 : GWPlayerCfg[playerWork->playerNumber].character;
    playerWork->controllerIndex = ((playerWork->characterId == -1) || (GWPlayerCfg[playerWork->playerNumber].iscom != 0)) != 0 ? -1 : GWPlayerCfg[playerWork->playerNumber].pad_idx;
    playerWork->difficulty = playerWork->controllerIndex != -1 ? 0 : GWPlayerCfg[playerWork->playerNumber].diff;
    playerWork->currentAction = -1;
    playerWork->actionTimer = 0;
    playerWork->hitLimit = scoreGoalByDifficulty[playerWork->difficulty];
    playerWork->playerState = 0;
    playerWork->winnerIndex = -1;
    if (playerWork->characterId != -1) {
        playerObject->mdlId[0] = CharModelCreate(playerWork->characterId, 1);
    }
    else {
        playerObject->mdlId[0] = Hu3DModelCreateFile(0x50001B);
    }
    Hu3DModelObjPosGet(stageObject->mdlId[1], stageObjectNames[playerWork->playerIndex + 2], &playerModelPosition);
    omSetTra(playerObject, playerModelPosition.x, playerModelPosition.y, playerModelPosition.z);
    if (playerObject->trans.x == -360.0f) {
        playerObject->trans.x = -350.0f;
    }
    omSetRot(playerObject, 0.0f, (playerWork->playerIndex * 0xB4) + 0x5A, 0.0f);
    for (setupIndex = 0; setupIndex < 4; setupIndex++) {
        if (playerWork->characterId != -1) {
            playerObject->mtnId[setupIndex]
                = CharMotionCreate(playerWork->characterId, characterMotionIds[setupIndex] + (characterSpecificMotionFlags[setupIndex] != 0 ? playerWork->characterId : 0));
        }
        else {
            playerObject->mtnId[setupIndex] = Hu3DJointMotionFile(playerObject->mdlId[0], alternateMotionIds[setupIndex]);
        }
    }
    Hu3DMotionSet(playerObject->mdlId[0], playerObject->mtnId[0]);
    Hu3DMotionTimeSet(playerObject->mdlId[0], 20.0f);
    Hu3DModelAttrSet(playerObject->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(playerObject->mdlId[0]);
    CharMotionVoiceOnSet(playerWork->characterId, playerObject->mtnId[3], 0);
    playerPatternValues[playerWork->playerIndex] = playerWork->playerIndex == 0 ? (rand8() % 3) : ((playerPatternValues[0] + ((rand8() % 2)) + 1) % 3);
    if (playerWork->playerIndex == 0) {
        for (setupIndex = 0; setupIndex < 3; setupIndex++) {
            M449TokenModelIds tokenModelIds = {
                { 0x0050000B, 0x0050000D, 0x0050000F, 0x00500011 },
                { 0x0050000C, 0x0050000E, 0x00500010, 0x00500012 },
            };
            do {
                while (TRUE) {
                    boardRoundChoice = rand8() % 4;
                    if (boardRoundOrder[0] == boardRoundChoice) {
                        continue;
                    }
                    if (boardRoundOrder[1] == boardRoundChoice) {
                        continue;
                    }
                    break;
                }
            } while (boardRoundOrder[2] == boardRoundChoice);
            boardRoundOrder[setupIndex] = boardRoundChoice;
            playerObject->mdlId[setupIndex + 1] = Hu3DModelCreateFile(tokenModelIds.primaryTokenModelIds[boardRoundChoice]);
            playerObject->mdlId[setupIndex + 4] = Hu3DModelCreateFile(tokenModelIds.secondaryTokenModelIds[boardRoundChoice]);
        }
        playerObject->mdlId[11] = Hu3DModelCreateFile(0x500013);
    }
    else {
        for (setupIndex = 0; setupIndex < 3; setupIndex++) {
            playerObject->mdlId[setupIndex + 1] = Hu3DModelLink(playerZeroObject->mdlId[setupIndex + 1]);
            playerObject->mdlId[setupIndex + 4] = Hu3DModelLink(playerZeroObject->mdlId[setupIndex + 4]);
        }
        playerObject->mdlId[11] = Hu3DModelLink(playerZeroObject->mdlId[11]);
    }
    Hu3DModelObjPosGet(stageObject->mdlId[1], stageObjectNames[playerWork->playerIndex], &playerModelPosition);
    for (setupIndex = 0; setupIndex < 3; setupIndex++) {
        if (setupIndex != 0) {
            Hu3DModelAttrSet(playerObject->mdlId[setupIndex + 1], HU3D_ATTR_DISPOFF);
        }
        Hu3DModelPosSet(playerObject->mdlId[setupIndex + 1], playerModelPosition.x, playerModelPosition.y, playerModelPosition.z);
        Hu3DModelPosSet(playerObject->mdlId[setupIndex + 4], playerModelPosition.x, playerModelPosition.y, playerModelPosition.z);
        Hu3DModelShadowSet(playerObject->mdlId[setupIndex + 1]);
    }
    Hu3DModelPosSet(playerObject->mdlId[11], playerModelPosition.x, playerModelPosition.y, playerModelPosition.z);
    Hu3DModelShadowSet(playerObject->mdlId[11]);
    for (setupIndex = 0; setupIndex < 4; setupIndex++) {
        if (setupIndex == 0) {
            playerObject->mdlId[setupIndex + 0xC] = Hu3DModelCreateFile(playerWork->playerIndex == 0 ? 0x500014 : 0x500015);
        }
        else {
            playerObject->mdlId[setupIndex + 0xC] = Hu3DModelLink(playerObject->mdlId[12]);
        }
        Hu3DModelObjPosGet(stageObject->mdlId[1], stageObjectNames[playerWork->playerIndex], &playerModelPosition);
        Hu3DModelObjPosGet(playerObject->mdlId[4], tokenObjectNames[setupIndex], &tokenModelPosition);
        playerWork->tokenX[setupIndex] = tokenModelPosition.x - playerModelPosition.x;
        playerWork->tokenZ[setupIndex] = tokenModelPosition.z - playerModelPosition.z;
        playerWork->tokenVelocityX[setupIndex] = playerWork->tokenVelocityZ[setupIndex] = 0.0f;
        Hu3DModelPosSet(playerObject->mdlId[setupIndex + 0xC], tokenModelPosition.x, tokenModelPosition.y, tokenModelPosition.z);
    }
    for (setupIndex = 0; setupIndex < 4; setupIndex++) {
        particleModelIndex = setupIndex + 7;
        Hu3DModelObjPosGet(playerObject->mdlId[4], tokenObjectNames[setupIndex], &tokenModelPosition);
        playerObject->mdlId[particleModelIndex] = Hu3DParticleCreate(HuSprAnimRead(HuDataReadNum(0x120003, 0x10000000)), 0x32);
        Hu3DModelPosSet(playerObject->mdlId[particleModelIndex], 2000.0f, -2000.0f, -2000.0f);
        Hu3DModelScaleSet(playerObject->mdlId[particleModelIndex], 1.0f, 1.0f, 1.0f);
        Hu3DModelLayerSet(playerObject->mdlId[particleModelIndex], 3);
        Hu3DModelAttrReset(playerObject->mdlId[particleModelIndex], HU3D_ATTR_DISPOFF);
        Hu3DParticleColSet(playerObject->mdlId[particleModelIndex], 0xFF, 0xFF, 0xFF);
        Hu3DParticleScaleSet(playerObject->mdlId[particleModelIndex], 5.0f);
        Hu3DParticleHookSet(playerObject->mdlId[particleModelIndex], M449TokenParticleUpdate);
        Hu3DParticleCntSet(playerObject->mdlId[particleModelIndex], 0xFF);
    }
    if (playerWork->playerIndex == 1) {
        for (setupIndex = 0; setupIndex < 0x12; setupIndex++) {
            s32 scoreboardColumnXOffsets[2] = { 0x64, 0x1E5 };
            s32 scoreboardRowXOffsets[0x12] = { 0, 0, -14, 16, 46, -14, 16, 46, -50, 0, 0, -14, 16, 46, -14, 16, 46, -50 };
            s32 scoreboardRowYOffset[0x12] = { 0, 0, -2, -2, -2, -2, -2, -2, 0, 0, 0, -2, -2, -2, -2, -2, -2, 0 };
            if ((scoreboardAssetIds[setupIndex] + 0xFF9F0000) == 0) {
                scoreboardPlayerWork = (&playerZeroObject)[setupIndex == 8 ? 0 : (setupIndex == 0x11 ? 1 : 0)]->data;
                scoreboardSpriteIds[setupIndex] = espEntry(scoreboardPlayerWork->characterId == -1 ? 0x500018 : (scoreboardPlayerWork->characterId + scoreboardAssetIds[setupIndex]), 0, 0);
            }
            else {
                scoreboardSpriteIds[setupIndex] = espEntry(scoreboardAssetIds[setupIndex], 0, 0);
            }
            espDrawNoSet(scoreboardSpriteIds[setupIndex], 0);
            espAttrSet(scoreboardSpriteIds[setupIndex], HUSPR_ATTR_NOANIM | HUSPR_ATTR_DISPOFF);
            espBankSet(scoreboardSpriteIds[setupIndex], 0);
            espPriSet(scoreboardSpriteIds[setupIndex], 0x80 - setupIndex);
            espPosSet(scoreboardSpriteIds[setupIndex], scoreboardRowXOffsets[setupIndex] + scoreboardColumnXOffsets[setupIndex < 9 ? 0 : 1], scoreboardRowYOffset[setupIndex] + 0x46);
            if ((setupIndex >= 9 ? (setupIndex - 9) : setupIndex) != 0) {
                (void)setupIndex;
                continue;
            }
            else {
                espTPLvlSet(scoreboardSpriteIds[setupIndex], 0.5f);
                espColorSet(scoreboardSpriteIds[setupIndex], 0, 0, 0);
            }
        }
    }
    playerObject->objFunc = M449PlayerObjectUpdate;
}

void M449RoundStart(OMOBJ *roundObject)
{
    gameState = 0x3E8;
    stateFrameCount = 0;
    gameEventFlags = 0;
    gameEventTimer = 0;
    if (soundEffectQueue[0].pendingSoundCount < 2) {
        soundEffectQueue[0].soundQueueState[soundEffectQueue[0].pendingSoundCount] = 0.0f;
        soundEffectQueue[0].pendingSoundCount++;
    }
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    roundObject->objFunc = M449GameStateUpdate;
}

void ObjectSetup(void)
{
    s32 playerSlot;
    OMOBJ *playerObject;

    OSReport("******* M449 ObjectSetup *********\n");
    objectManager = omInitObjMan(0x258, 0x2000);
    omGameSysInit(objectManager);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 30.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    CZoom = 2500.0f;
    Center.x = 0.0f;
    Center.y = 0.0f;
    Center.z = -100.0f;
    CRot.x = -40.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    {
        Vec lightPosition = { 0.0f, 0.0f, 0.0f };
        Vec lightDirection = { -0.5850064f, -0.788462f, 0.1545085f };
        Vec shadowCameraPosition;
        Vec shadowUpDirection;
        Vec shadowCenterOffset;
        GXColor lightColor = { 0xFF, 0xFF, 0xFF, 0xFF };
        directionalLightId = Hu3DGLightCreateV(&lightPosition, &lightDirection, &lightColor);
        Hu3DGLightStaticSet(directionalLightId, 1);
        Hu3DGLightInfinitytSet(directionalLightId);
        shadowCameraPosition.x = 0.0f;
        shadowCameraPosition.y = 10000.0f;
        shadowCameraPosition.z = 0.0f;
        shadowUpDirection.x = 0.0f;
        shadowUpDirection.y = 1.0f;
        shadowUpDirection.z = 0.0f;
        shadowCenterOffset.x = 0.0f;
        shadowCenterOffset.y = 0.0f;
        shadowCenterOffset.z = 0.5f;
        Hu3DShadowCreate(6.0f, 9000.0f, 11500.0f);
        Hu3DShadowTPLvlSet(0.625f);
        Hu3DShadowPosSet(&shadowCameraPosition, &shadowUpDirection, &shadowCenterOffset);
    }
    Hu3DShadowSizeSet(0xE0);
    tutorialSequenceId = -1;
    winnerIndex = -1;
    HuWinInit(1);
    stageObject = omAddObjEx(objectManager, 0x64, 3, 0, 1, M449StageModelsInit);
    for (playerSlot = 0; playerSlot < 2; playerSlot++) {
        playerObject = (&playerZeroObject)[playerSlot] = omAddObjEx(objectManager, 0x65, 0x14, 4, 0, M449PlayerObjectInit);
        playerObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M449DllWork), HU_MEMNUM_OVL);
        playerObject->work[0] = playerSlot;
    }
    outOfViewObject = omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutView);
    exitObject = omAddObjEx(objectManager, 0x67, 0, 0, -1, M449RoundStart);
}

void M449Exit(OMOBJ *object)
{
    void *playerData;
    s32 playerSlot;

    if (WipeStatGet() == 0) {
        for (playerSlot = 0; playerSlot < 2; playerSlot++) {
            playerData = (&playerZeroObject)[playerSlot]->data;
        }
        if (tutorialSequenceId >= 0) {
            MGSeqKill(tutorialSequenceId);
        }
        MGSeqKillAll();
        HuAudAllStop();
        omOvlReturnEx(1, 1);
    }
}

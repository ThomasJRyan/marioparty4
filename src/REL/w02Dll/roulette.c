#include "REL/w02Dll.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/frand.h"
#ifndef __MWERKS__
#include "game/objsub.h"
#endif
#include "game/pad.h"

#ifndef __MWERKS__
#include <stdlib.h>
#endif

#include "ext_math.h"

#ifndef __MWERKS__
#include "game/board/com.h"
#include "game/board/player.h"
#include "game/board/view.h"
#endif

/* DATA */
s32 rouletteSectorSpaceFlags[] = { -1, 2, 2, 2, -1, 1, 1, 1, -1, 0, 0, 0, -1, 3, 3, 3 };

HUPROCESS *rouletteProcess;
s16 roulettePlayerIndex;
s32 rouletteSoundId;
float rouletteWheelAngle;

// Local Functions
void RouletteBribePayout(void);
s32 RouletteComputerChoice(void);
void RouletteModelsAnimate(void);

s32 RouletteStarRegionGet(void);
void RouletteWheelRotate(f32);

// Unknown Structures
typedef struct RouletteWeightedPoint {
    f32 spaceWeight;
    Vec point;
} RouletteWeightedPoint;

/* FUNCTIONS */
void RouletteModelsInitialize(void)
{
    Vec modelTransform;

    BoardModelPosGet(boardModelIds[15], &modelTransform);
    modelTransform.z -= 90.0f;
    modelTransform.y += 50.0f;
    BoardModelPosSetV(boardModelIds[6], &modelTransform);
    BoardModelAttrSet(boardModelIds[5], 0x40000002);
    BoardModelMotionTimeSet(boardModelIds[5], 0.0f);
    BoardModelRotGet(boardModelIds[15], &modelTransform);
    modelTransform.y = 0.0f;
    BoardModelRotSetV(boardModelIds[15], &modelTransform);
}

void RouletteEventProcess(void)
{
    s32 spaceLinkFlags;
    s32 winningSector;
    s32 bribeProbability;

    BoardRollDispSet(0);
    spaceLinkFlags = BoardSpaceLinkFlagSearch(0, (s32)GWPlayer[roulettePlayerIndex].space_curr, 2U);
    BoardPlayerMoveToAsync(roulettePlayerIndex, spaceLinkFlags);
    while (GWPlayer[roulettePlayerIndex].moving) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(roulettePlayerIndex);
    BoardCameraViewSet(2);
    BoardCameraMotionWait();
    BoardPlayerMotBlendSet((s32)roulettePlayerIndex, 0x5A, 0xF);
    while (BoardPlayerMotBlendCheck((s32)roulettePlayerIndex) == 0) {
        HuPrcVSleep();
    }
    BoardModelRotYSet(boardModelIds[15], -90.0f);
    W02MesExec(MAKE_MESSID(0x13, 0x00));
    HuAudFXPlay(0x44);
    bribeProbability = RouletteBribeChoice();
    BoardStatusShowSetAll(0);
    W02MesExec(MAKE_MESSID(0x13, 0x01));
    winningSector = RouletteSpin(bribeProbability);
    RoulettePlayerMove(winningSector);
    BoardCameraViewSet(1);
    BoardStatusShowSetAll(1);
    BoardCameraMotionWait();
    BoardRollDispSet(1);
    HuPrcKill(NULL);
    while (TRUE) {
        HuPrcVSleep();
    }
}

s32 RouletteEventCleanup(void)
{
    rouletteProcess = 0;
}

s32 RouletteBribeChoice(void)
{
    s32 coinAmount;
    s32 bribeProbability;
    s16 playerCoinRegion;
    s32 bribeChoice;
    s32 i;
    s32 rouletteChoice;
    s32 mesg;

    bribeProbability = -1;
    coinAmount = BoardPlayerCoinsGet((s32)roulettePlayerIndex);
    if (coinAmount < 5) {
        return bribeProbability;
    }
    W02MesExec(MAKE_MESSID(0x13, 0x02));

    do {
        BoardWinCreateChoice(2, MAKE_MESSID(19, 29), 4, 0);
        /* COM has 50% to bribe goomba */
        if (GWPlayer[roulettePlayerIndex].com) {
            if (frandBool()) {
                BoardComKeySetRight();
            }
            else {
                BoardComKeySetLeft();
            }
        }
        BoardWinWait();
        BoardWinKill();
        rouletteChoice = BoardWinChoiceGet();
        // If chose to play roulette
        if (rouletteChoice == 0) {
            BoardWinCreateChoice(1, MAKE_MESSID(19, 3), 4, 0);
            playerCoinRegion = 3;
            if (coinAmount < 0x14) {
                BoardWinChoiceDisable(0);
                playerCoinRegion--;
            }
            if (coinAmount < 0xA) {
                BoardWinChoiceDisable(1);
                playerCoinRegion--;
            }
            /* COM always bribes max unless it has 20 or
               more coins. Then 50% max, 50% 10 coin bribe. */
            if (GWPlayer[roulettePlayerIndex].com) {
                if (playerCoinRegion < 2) {
                    BoardComKeySetUp();
                }
                else if (frandBool()) {
                    BoardComKeySetUp();
                }
                else {
                    BoardComKeySetDown();
                }
            }
            BoardWinWait();
            BoardWinKill();
            bribeChoice = BoardWinChoiceGet();
            if (bribeChoice == -1 || bribeChoice == 3) {
                continue;
            }

            // Coin amount is now the bribe amount in coins
            switch (bribeChoice) {
                case 0:
                    mesg = MAKE_MESSID(0x13, 0x04);
                    coinAmount = 0x14;
                    bribeProbability = 0x5A; // 90%
                    break;
                case 1:
                    mesg = MAKE_MESSID(0x13, 0x05);
                    coinAmount = 0xA;
                    bribeProbability = 0x3C; // 60%
                    break;
                case 2:
                    mesg = MAKE_MESSID(0x13, 0x06);
                    coinAmount = 5;
                    bribeProbability = 0x1E; // 30%
                    break;
            }
            W02MesExec(mesg);

            for (i = 0; i < coinAmount; i++) {
                BoardPlayerCoinsAdd(roulettePlayerIndex, -1);
                HuAudFXPlay(0xE);
                HuPrcSleep(6);
            }
            HuAudFXPlay(0xF);
            // If chose to view map
        }
        else if (rouletteChoice == 2) {
            BoardViewMapExec(roulettePlayerIndex);
        }
    } while (rouletteChoice == 2); // While viewing map
    if (bribeProbability == -1) {
        W02MesExec(MAKE_MESSID(0x13, 0x07));
    }
    return bribeProbability;
}

s32 RouletteSpin(s32 bribeProbability)
{
    Vec rouletteRotation;
    Vec ballPosition;
    Vec ballVelocity;
    Vec rouletteCenter;

    f32 ballLaunchSpeed;
    f32 unusedDistance;
    f32 ballDistance;
    f32 bounceSpeed;
    f32 rouletteRotationStep;
    f64 unusedScale;
    f32 ballAngle;

    s32 targetSpaceIndex;
    s32 bounceCount;
    s16 audioPitch;
    s32 winningSector;
    s32 sectorRoll;
    s32 i;

    if ((s32)frandmod(0x64U) < bribeProbability) {
        s32 starRegion = RouletteStarRegionGet();
        bounceCount = 4;
        sectorRoll = frandmod(3U);
        if ((s32)frandmod(0x64U) < 0x14) {
            sectorRoll += 1;
        }
        sectorRoll = sectorRoll + (3 - starRegion) * 4;
    }
    else {
        sectorRoll = frandmod(0x10U);
        bounceCount = frandmod(4U);
    }
    BoardModelPosGet(boardModelIds[4], &rouletteCenter);
    BoardCameraViewSet(1);
    BoardCameraMotionWait();
    targetSpaceIndex = BoardSpaceFlagSearch(0, 0x800U);
    BoardCameraTargetSpaceSet(targetSpaceIndex);
    BoardPlayerMotBlendSet(roulettePlayerIndex, 0xB4, 0xF);
    BoardModelRotGet(boardModelIds[15], &rouletteRotation);

    for (i = 0; i < 0xF; i++) {
        rouletteRotation.y -= 6.0f;
        BoardModelRotSetV(boardModelIds[15], &rouletteRotation);
        HuPrcVSleep();
    }

    rouletteRotation.y = 180.0f;
    BoardModelRotSetV(boardModelIds[15], &rouletteRotation);
    BoardCameraMotionWait();
    rouletteSoundId = HuAudFXPlay(0x41D);
    audioPitch = -0x1FFF;
    rouletteRotationStep = 0.0f;
    for (i = 0; i < 0x3C; i++) {
        rouletteRotationStep -= 0.375f;
        RouletteWheelRotate(rouletteRotationStep);
        audioPitch += 136.51666f;
        HuAudFXPitchSet(rouletteSoundId, audioPitch);
        HuPrcVSleep();
    }

    rouletteRotationStep = -22.5f;
    HuAudFXPitchSet(rouletteSoundId, 0);
    BoardModelMotionStart(boardModelIds[15], 2, 0x40000001);
    BoardModelPosGet(boardModelIds[15], &rouletteRotation);

    for (i = 0; i < 0x1E; i++) {
        rouletteRotation.z += 2.1f;
        BoardModelPosSetV(boardModelIds[15], &rouletteRotation);
        RouletteWheelRotate(rouletteRotationStep);
        HuPrcVSleep();
    }
    BoardModelMotionStart(boardModelIds[15], 1, 0x40000001);
    while (sectorRoll) {
        RouletteWheelRotate(rouletteRotationStep);
        HuPrcVSleep();
        sectorRoll -= 1;
    }
    BoardModelMotionStart(boardModelIds[15], 3, 0x40000001);
    BoardModelMotionSpeedSet(boardModelIds[15], 4.0f);

    for (i = 0; i < 0xA; i++) {
        rouletteRotation.z -= 6.2999997f;
        BoardModelPosSetV(boardModelIds[15], &rouletteRotation);
        RouletteWheelRotate(rouletteRotationStep);
        HuPrcVSleep();
    }
    BoardModelMotionStart(boardModelIds[15], 1, 0x40000001);
    BoardModelMotionSpeedSet(boardModelIds[15], 1.0f);
    HuAudFXPlay(0x41F);
    HuAudFXPlay(0x42D);
    omVibrate(roulettePlayerIndex, 0xC, 6, 6);
    ballAngle = 0.0f;
    ballLaunchSpeed = 16.0f;
    ballVelocity.x = 0.0f;
    ballVelocity.z = -50.0f;

    // Roulette motion and ball ?
    while (TRUE) {
        ballVelocity.y = ballLaunchSpeed - (0.55833334f * (0.25f * (ballAngle * ballAngle)));
        ballAngle += 1.0f;
        BoardModelPosGet(boardModelIds[6], &ballPosition);
        VECAdd(&ballPosition, &ballVelocity, &ballPosition);
        ballDistance = VECDistanceXZ(&ballPosition, &rouletteCenter);
        if (ballDistance >= 430.0f) {
            if (ballPosition.y <= (55.0f + rouletteCenter.y)) {
                VECSubtract(&ballPosition, &rouletteCenter, (Vec *)&ballVelocity);
                VECNormalize((Vec *)&ballVelocity, (Vec *)&ballVelocity);
                ballVelocity.y = 0.0f;
                VECScale((Vec *)&ballVelocity, (Vec *)&ballVelocity, 429.0f);
                VECAdd((Vec *)&ballVelocity, &rouletteCenter, (Vec *)&ballVelocity);
                ballPosition.x = ballVelocity.x;
                ballPosition.z = ballVelocity.z;
                ballVelocity.x = -5.0f;
                ballVelocity.z = 10.0f;
            }
        }

        if (ballPosition.y < rouletteCenter.y) {
            ballPosition.y = rouletteCenter.y;
            if (bounceCount >= 3) {
                break;
            }
            // ballAngle = ((1.0f/255.0f) * (6.0f * frand8()));
            ballLaunchSpeed = 6.0f + ((1.0f / 255.0f) * (6.0f * frand8()));
            ballAngle = 1.0f;
            bounceCount += 1;
            // ballAngle = ((1.0f/255.0f) * (5.0f * frand8()));
            ballVelocity.x = 15.0f + ((1.0f / 255.0f) * (5.0f * frand8()));
            ballVelocity.z = 15.0f + ((1.0f / 255.0f) * (5.0f * frand8()));
            HuAudFXPlay(0x41E);
        }

        BoardModelPosSetV(boardModelIds[6], (Vec *)&ballPosition);
        RouletteWheelRotate(rouletteRotationStep);
        HuPrcVSleep();
    }

    HuAudFXPlay(0x41E);
    VECSubtract(&ballPosition, &rouletteCenter, (Vec *)&ballVelocity);
    VECNormalize((Vec *)&ballVelocity, (Vec *)&ballVelocity);
    ballAngle = atan2d(-ballVelocity.z, ballVelocity.x);
    BoardModelRotGet(boardModelIds[4], &rouletteRotation);
    ballAngle = ballAngle - rouletteRotation.y;
    if (ballAngle < 0.0f) {
        ballAngle += 360.0f;
    }
    winningSector = (s32)(ballAngle / 22.5f);
    if (fmod(ballAngle, 22.5) > 11.25) {
        winningSector += 1;
    }
    if (winningSector >= 0x10) {
        winningSector -= 0x10;
    }
    ballVelocity.y = 0.0f;

    while (TRUE) {
        RouletteWheelRotate(rouletteRotationStep);
        BoardModelRotGet(boardModelIds[4], &rouletteRotation);
        ballAngle = (22.5f * winningSector) + rouletteRotation.y; // TODO: Update rodata for asm change
        ballVelocity.x = cosd(ballAngle);
        ballVelocity.z = -sind(ballAngle);
        VECScale(&ballVelocity, &ballVelocity, 429.0f);
        VECAdd(&ballVelocity, &rouletteCenter, &ballVelocity);
        ballPosition.x = ballVelocity.x;
        ballPosition.z = ballVelocity.z;
        BoardModelPosSetV(boardModelIds[6], &ballPosition);
        if (rouletteRotation.y >= 292.5f && rouletteRotation.y < 315.0f) {
            break;
        }
        HuPrcVSleep();
    }

    for (i = 0; i < 0x3C; i++) {
        rouletteRotationStep += 0.375f;
        RouletteWheelRotate(rouletteRotationStep);
        BoardModelRotGet(boardModelIds[4], &rouletteRotation);
        ballAngle = (22.5f * (f32)winningSector) + rouletteRotation.y;
        ballVelocity.x = cosd(ballAngle);
        ballVelocity.z = -sind(ballAngle);
        VECScale(&ballVelocity, &ballVelocity, 429.0f);
        VECAdd((Vec *)&ballVelocity, &rouletteCenter, (Vec *)&ballVelocity);
        ballPosition.x = ballVelocity.x;
        ballPosition.z = ballVelocity.z;
        BoardModelPosSetV(boardModelIds[6], (Vec *)&ballPosition);
        audioPitch += -136.51666f;
        HuAudFXPitchSet(rouletteSoundId, audioPitch);
        HuPrcVSleep();
    }
    BoardModelRotYSet(boardModelIds[4], 0.0f);
    HuAudFXStop(rouletteSoundId);
    return rouletteSectorSpaceFlags[winningSector];
}

// Place player on space post roulette?
void RoulettePlayerMove(s32 targetSector)
{
    Vec targetSpacePosition;
    Vec modelPosition;
    Vec playerScale;

    f32 rotationStep;
    f32 unusedRotation;
    f32 scaleValue;

    s16 unusedPlayerIndex;
    u32 spaceFlags;
    s32 unusedState;
    s32 targetSpaceIndex;
    s32 linkIndex;
    s32 nextSpaceIndex;
    BoardSpace *currentSpace;

    if (targetSector < 0) {
        RouletteBribePayout();
        targetSector = RouletteComputerChoice();
    }
    currentSpace = BoardSpaceGet(0, BoardSpaceFlagSearch(0, 0x800));

    for (linkIndex = 0; linkIndex < currentSpace->link_cnt; linkIndex++) {
        targetSpaceIndex = currentSpace->link[linkIndex];
        spaceFlags = ((BoardSpaceFlagGet(0, targetSpaceIndex) & 0xE) >> 1U);
        if (spaceFlags == (targetSector + 2)) {
            break;
        }
    }
    BoardSpacePosGet(0, targetSpaceIndex, &targetSpacePosition);
    BoardModelAttrReset(boardModelIds[5], 0x40000002);
    HuAudFXPlay(0x420);
    if (BoardPlayerSizeGet(roulettePlayerIndex) == 2) {
        while (BoardModelMotionTimeGet(boardModelIds[5]) < 25.0f) {
            HuPrcVSleep();
        }
        BoardPlayerScaleGet(roulettePlayerIndex, &playerScale);

        while (BoardModelMotionTimeGet(boardModelIds[5]) < 30.0f) {
            playerScale.x -= 0.25f;
            playerScale.y -= 0.25f;
            // temp_f1 = playerScale.z;
            playerScale.z = playerScale.z - 0.25f;
            BoardPlayerScaleSetV(roulettePlayerIndex, &playerScale);
            HuPrcVSleep();
        }

        playerScale.x = playerScale.y = playerScale.z = 1.25f;
        BoardPlayerScaleSetV(roulettePlayerIndex, &playerScale);
    }

    while (BoardModelMotionTimeGet(boardModelIds[5]) < 30.0f) {
        HuPrcVSleep();
    }
    if (GWPlayer[roulettePlayerIndex].bowser_suit) {
        BoardModelVisibilitySet(BoardBowserSuitModelGet(), 0);
        BoardModelVisibilitySet(BoardBowserSuitPlayerModelGet(), 0);
    }
    else {
        BoardModelVisibilitySet(BoardPlayerModelGet(roulettePlayerIndex), 0);
    }
    HuAudFXPlay(0x421);
    omVibrate(roulettePlayerIndex, 0xC, 4, 2);

    while (BoardModelMotionTimeGet(boardModelIds[5]) < 60.0f) {
        HuPrcVSleep();
    }

    BoardModelPosGet(boardModelIds[5], &modelPosition);
    rotationStep = atan2d(targetSpacePosition.x - modelPosition.x, targetSpacePosition.z - modelPosition.z) / 30;

    while (BoardModelMotionTimeGet(boardModelIds[5]) < 90.0f) {
        BoardModelRotYSet(boardModelIds[5], rotationStep + BoardModelRotYGet(boardModelIds[5]));
        HuPrcVSleep();
    }
    BoardModelRotYSet(boardModelIds[5], atan2d(targetSpacePosition.x - modelPosition.x, targetSpacePosition.z - modelPosition.z));
    omVibrate(roulettePlayerIndex, 0xC, 4, 2);
    if (GWPlayer[roulettePlayerIndex].bowser_suit) {

        BoardModelVisibilitySet(BoardBowserSuitModelGet(), 1);
        BoardModelVisibilitySet(BoardBowserSuitPlayerModelGet(), 1);
    }
    else {
        BoardModelVisibilitySet(BoardPlayerModelGet(roulettePlayerIndex), 1);
    }
    GWPlayer[roulettePlayerIndex].space_curr = (s16)targetSpaceIndex;
    BoardPlayerPosSetV(roulettePlayerIndex, &targetSpacePosition);
    currentSpace = BoardSpaceGet(0, targetSpaceIndex);

    for (nextSpaceIndex = 0; nextSpaceIndex < (s32)currentSpace->link_cnt; nextSpaceIndex++) {
        if (BoardSpaceTypeGet(0, currentSpace->link[nextSpaceIndex])) {
            break;
        }
    }

    if (nextSpaceIndex != (s32)currentSpace->link_cnt) {
        BoardSpacePosGet(0, currentSpace->link[nextSpaceIndex], &modelPosition);
        BoardPlayerRotYSet(roulettePlayerIndex, atan2d(modelPosition.x - targetSpacePosition.x, modelPosition.z - targetSpacePosition.z));
    }
    if (BoardPlayerSizeGet(roulettePlayerIndex) == 2) {
        scaleValue = playerScale.x;

        while (scaleValue < 2.5f) {
            scaleValue += 0.104166664f;
            if (scaleValue > 2.5f) {
                scaleValue = 2.5f;
            }
            playerScale.z = scaleValue;
            playerScale.y = scaleValue;
            playerScale.x = scaleValue;
            BoardPlayerScaleSetV(roulettePlayerIndex, &playerScale);
            HuPrcVSleep();
        }
    }
    HuAudFXPlay(0x42A);
    while (BoardModelMotionTimeGet(boardModelIds[5]) < BoardModelMotionMaxTimeGet(boardModelIds[5])) {
        HuPrcVSleep();
    }
    HuPrcCreate(RouletteModelsAnimate, 0x2004, 0x1000, 0);
}

void RouletteBribePayout(void)
{
    float coinFallSpeed;
    float coinHeightScale;
    s32 coinIndex;
    s32 collectedCoinCount;
    s16 coinModelIds[20];
    Vec coinPosition;
    Vec coinRotation;
    Vec playerPosition;
    float playerSizeScales[3] = { 1.0f, 0.3f, 2.5f };
    W02MesExec(MAKE_MESSID(0x13, 0x08));
    BoardStatusShowSetAll(1);
    BoardPlayerMotBlendSet(roulettePlayerIndex, 0, 15);
    BoardPlayerPosGet(roulettePlayerIndex, &playerPosition);
    for (coinIndex = 0; coinIndex < 20; coinIndex++) {
        coinModelIds[coinIndex] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x0A), NULL, 1);
        coinPosition.x = (playerPosition.x - 25.0f) + ((1.0f / 255.0f) * (frand8() * 50.0f));
        coinPosition.y = (playerPosition.y + 1200.0f) + (coinIndex * 130.0f) + ((1.0f / 255.0f) * (frand8() * 60.0f));
        coinPosition.z = (playerPosition.z - 25.0f) + ((1.0f / 255.0f) * (frand8() * 50.0f));
        BoardModelPosSetV(coinModelIds[coinIndex], &coinPosition);
        coinRotation.x = coinRotation.z = 0;
        coinRotation.y = (1.0f / 255.0f) * (frand8() * 360.0f);
        BoardModelRotSetV(coinModelIds[coinIndex], &coinRotation);
    }
    coinFallSpeed = -4;
    collectedCoinCount = 0;
    coinHeightScale = playerSizeScales[BoardPlayerSizeGet(roulettePlayerIndex)];
    while (1) {
        for (coinIndex = 0; coinIndex < 20; coinIndex++) {
            if (coinIndex < collectedCoinCount) {
                continue;
            }
            BoardModelPosGet(coinModelIds[coinIndex], &coinPosition);
            BoardModelRotGet(coinModelIds[coinIndex], &coinRotation);
            coinPosition.y += coinFallSpeed;
            coinRotation.y += 45.0f;
            BoardModelPosSetV(coinModelIds[coinIndex], &coinPosition);
            BoardModelRotSetV(coinModelIds[coinIndex], &coinRotation);
            if (coinPosition.y <= playerPosition.y + (180.0f * coinHeightScale)) {
                CharEffectCoinGlowCreate(1, &coinPosition);
                BoardModelVisibilitySet(coinModelIds[coinIndex], 0);
                collectedCoinCount++;
                BoardPlayerCoinsAdd(roulettePlayerIndex, 1);
                HuAudFXPlay(7);
                omVibrate(roulettePlayerIndex, 12, 6, 6);
            }
        }
        coinFallSpeed *= 1.05f;
        if (coinFallSpeed < -40.0f) {
            coinFallSpeed = -40.0f;
        }
        if (collectedCoinCount == 20) {
            BoardPlayerMotionShiftSet(roulettePlayerIndex, 7, 0, 10, HU3D_MOTATTR_NONE);
            break;
        }
        HuPrcVSleep();
    }
    while (!BoardPlayerMotionEndCheck(roulettePlayerIndex)) {
        HuPrcVSleep();
    }
    for (coinIndex = 0; coinIndex < 20; coinIndex++) {
        BoardModelKill(coinModelIds[coinIndex]);
    }
    HuPrcSleep(60);
    BoardPlayerMotBlendSet(roulettePlayerIndex, 180, 15);
    BoardStatusShowSetAll(0);
    while (!BoardPlayerMotBlendCheck(roulettePlayerIndex)) {
        HuPrcVSleep();
    }
}

s32 RouletteComputerChoice(void)
{
    Vec rouletteCenter;
    Vec spaceDirection;
    Vec linkedSpacePosition;
    Vec modelScale;
    s16 quadrantModelIds[4];
    float windowDimensions[2];
    s32 linkIndex;

    float stickAngle;
    float selectionPulseAngle;

    s32 quadrantIndex;
    s16 windowId;
    BoardSpace *targetSpace;
    s32 linkIndex;
    s16 selectedQuadrant;
    s16 stickY;
    s32 starQuadrant;
    s16 buttonFlags;
    s16 controllerPort;
    s16 cpuInputDelay;

    u32 linkFlags;
    W02MesExec(MAKE_MESSID(0x13, 0x09));
    targetSpace = BoardSpaceGet(0, BoardSpaceFlagSearch(0, 0x800));
    BoardModelPosGet(boardModelIds[3], &rouletteCenter);
    for (quadrantIndex = 0; quadrantIndex < 4; quadrantIndex++) {
        quadrantModelIds[quadrantIndex] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x00), NULL, 1);
        BoardModelLayerSet(quadrantModelIds[quadrantIndex], 2);
        for (linkIndex = 0; linkIndex < targetSpace->link_cnt; linkIndex++) {
            linkFlags = (BoardSpaceFlagGet(0, targetSpace->link[linkIndex]) & 0xE) >> 1;
            if (linkFlags == quadrantIndex + 2) {
                break;
            }
        }
        linkIndex = BoardSpacePosGet(0, targetSpace->link[linkIndex], &linkedSpacePosition);
        VECSubtract(&linkedSpacePosition, &rouletteCenter, &spaceDirection);
        BoardModelRotYSet(quadrantModelIds[quadrantIndex], atan2d(spaceDirection.x, spaceDirection.z));
        VECNormalize(&spaceDirection, &spaceDirection);
        VECScale(&spaceDirection, &spaceDirection, 400);
        VECAdd(&spaceDirection, &rouletteCenter, &spaceDirection);
        spaceDirection.y += 300.0f;
        BoardModelPosSetV(quadrantModelIds[quadrantIndex], &spaceDirection);
        modelScale.x = modelScale.y = modelScale.z = 3;
        BoardModelScaleSetV(quadrantModelIds[quadrantIndex], &modelScale);
    }
    starQuadrant = RouletteStarRegionGet();
    HuWinMesMaxSizeGet(1, windowDimensions, MAKE_MESSID(0x13, 0x1E));
    windowId = HuWinCreate(-10000, 356, windowDimensions[0], windowDimensions[1], 0);
    HuWinMesSet(windowId, MAKE_MESSID(0x13, 0x1E));
    HuWinBGTPLvlSet(windowId, 0);
    HuWinMesSpeedSet(windowId, 0);
    controllerPort = GWPlayer[roulettePlayerIndex].port;
    selectionPulseAngle = 0;
    cpuInputDelay = frandmod(20) + 20;
restart:
    while (1) {
        if (GWPlayer[roulettePlayerIndex].com) {
            selectedQuadrant = 0;
            stickY = 0;
            if (cpuInputDelay) {
                cpuInputDelay--;
                buttonFlags = 0;
            }
            else {
                buttonFlags = PAD_BUTTON_A;
            }
            break;
        }
        buttonFlags = HuPadBtnDown[controllerPort];
        selectedQuadrant = HuPadStkX[controllerPort];
        stickY = HuPadStkY[controllerPort];
        if (!(buttonFlags & PAD_TRIGGER_Z)) {
            break;
        }
        HuAudFXPlay(1);
        for (quadrantIndex = 0; quadrantIndex < 4; quadrantIndex++) {
            BoardModelVisibilitySet(quadrantModelIds[quadrantIndex], 0);
        }
        HuWinDispOff(windowId);
        BoardViewMapExec(roulettePlayerIndex);
        HuWinDispOn(windowId);
        for (quadrantIndex = 0; quadrantIndex < 4; quadrantIndex++) {
            BoardModelVisibilitySet(quadrantModelIds[quadrantIndex], 1);
        }
    }
    if (!(buttonFlags & PAD_BUTTON_A)) {
        if (abs(selectedQuadrant) < 10) {
            selectedQuadrant = 0;
        }
        if (abs(stickY) < 10) {
            stickY = 0;
        }
        if (selectedQuadrant != 0 || stickY != 0) {
            stickAngle = atan2d(-selectedQuadrant, -stickY);
            if (stickAngle < 0.0f) {
                stickAngle += 360.0f;
            }
            quadrantIndex = stickAngle / 90.0f;
            stickAngle = fmod(stickAngle, 90.0);
            if (quadrantIndex != starQuadrant && stickAngle >= 20.0f && stickAngle <= 70.0f) {
                selectionPulseAngle = 0;
                starQuadrant = quadrantIndex;
                HuAudFXPlay(0);
            }
        }
        for (quadrantIndex = 0; quadrantIndex < 4; quadrantIndex++) {
            if (quadrantIndex == starQuadrant) {
                modelScale.x = modelScale.y = modelScale.z = sind(selectionPulseAngle) + 3.0;
                selectionPulseAngle += 8.0f;
                if (selectionPulseAngle > 180.0f) {
                    selectionPulseAngle -= 180.0f;
                }
            }
            else {
                modelScale.x = modelScale.y = modelScale.z = 3.0f;
            }

            BoardModelScaleSetV(quadrantModelIds[quadrantIndex], &modelScale);
        }
        HuPrcVSleep();
        goto restart;
    }
    else {
        HuAudFXPlay(2);
        for (quadrantIndex = 0; quadrantIndex < 4; quadrantIndex++) {
            BoardModelKill(quadrantModelIds[quadrantIndex]);
        }
        HuWinKill(windowId);
        return starQuadrant;
    }
}

void RouletteModelsAnimate(void)
{
    Vec modelScale;
    Vec roulettePosition;
    s32 animationFrame;
    float wheelRotation;
    float rouletteRotation;
    float wheelRotationStep;
    float rouletteRotationStep;
    BoardModelMotionTimeSet(boardModelIds[5], 0);
    BoardModelAttrSet(boardModelIds[5], 0x40000002);
    wheelRotation = BoardModelRotYGet(boardModelIds[5]);
    wheelRotationStep = -wheelRotation / 20.0f;
    rouletteRotation = BoardModelRotYGet(boardModelIds[15]);
    rouletteRotationStep = -rouletteRotation / 20.0f;
    BoardModelScaleGet(boardModelIds[6], &modelScale);
    for (animationFrame = 0; animationFrame < 10; animationFrame++) {
        modelScale.x = modelScale.y = modelScale.z = 1.0f - (animationFrame * 0.1f);
        if (modelScale.x < 0.0f) {
            modelScale.x = modelScale.y = modelScale.z = 0.0f;
        }
        BoardModelScaleSetV(boardModelIds[6], &modelScale);
        wheelRotation += wheelRotationStep;
        BoardModelRotYSet(boardModelIds[5], wheelRotation);
        rouletteRotation += rouletteRotationStep;
        BoardModelRotYSet(boardModelIds[15], rouletteRotation);
        HuPrcVSleep();
    }
    BoardModelPosGet(boardModelIds[15], &roulettePosition);
    roulettePosition.z -= 90.0f;
    roulettePosition.y += 50.0f;
    BoardModelPosSetV(boardModelIds[6], &roulettePosition);
    for (animationFrame = 0; animationFrame < 10; animationFrame++) {
        modelScale.x = modelScale.y = modelScale.z = animationFrame * 0.1f;
        if (modelScale.x > 1.0f) {
            modelScale.x = modelScale.y = modelScale.z = 1.0f;
        }
        BoardModelScaleSetV(boardModelIds[6], &modelScale);
        wheelRotation += wheelRotationStep;
        BoardModelRotYSet(boardModelIds[5], wheelRotation);
        rouletteRotation += rouletteRotationStep;
        BoardModelRotYSet(boardModelIds[15], rouletteRotation);
        HuPrcVSleep();
    }
    modelScale.x = modelScale.y = modelScale.z = 1.0f;
    BoardModelScaleSetV(boardModelIds[6], &modelScale);
    BoardModelRotYSet(boardModelIds[5], 0);
    BoardModelRotYSet(boardModelIds[15], 0);
    HuPrcEnd();
}

s32 RouletteStarRegionGet(void)
{
    switch (GWSystem.star_pos) {
        case 0:
            return 3;

        case 1:
        case 2:
            return 0;

        case 3:
        case 4:
            return 1;

        case 5:
        case 6:
        case 7:
            return 2;

        default:
            return -1;
    }
}

/* Rotate (yaw) of model 4 */
void RouletteWheelRotate(f32 degrees)
{
    Vec modelRotation;
    rouletteWheelAngle += degrees;
    if (rouletteWheelAngle >= 360.0f) {
        rouletteWheelAngle -= 360.0f;
    }
    if (rouletteWheelAngle < 0.0f) {
        rouletteWheelAngle += 360.0f;
    }
    BoardModelRotGet(boardModelIds[4], &modelRotation);
    modelRotation.y = rouletteWheelAngle;
    BoardModelRotSetV(boardModelIds[4], &modelRotation);
}

void RouletteEventStart(void)
{
    roulettePlayerIndex = GWSystem.player_curr;
    OSReport("Roulette Player %d\n", roulettePlayerIndex);
    rouletteProcess = HuPrcChildCreate(RouletteEventProcess, 8195, 8192, 0, boardMainProc);
    HuPrcDestructorSet2(rouletteProcess, (void *)RouletteEventCleanup); // RouletteEventCleanup requires implicit return
    while (rouletteProcess) {
        HuPrcVSleep();
    }
}

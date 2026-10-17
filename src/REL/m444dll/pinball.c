#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"


#include "game/pad.h"

#include "game/audio.h"
#include "game/chrman.h"
#include "game/window.h"

#include "game/gamework_data.h"

#include "REL/m444dll.h"
#include "ext_math.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

s16 playerBallState[5][2];
Vec upperBallTrailPositions[4096];
Vec lowerBallTrailPositions[4096];
Vec playerPositions[5];
s32 playerActiveFlags[5];
Vec ballPosition;
Vec ballVelocity;
u16 ballTrailCount;
u8 ballTrailFlags[4096];
Vec ballModelPositions[3];
Vec playerShadowPositions[5];
Mtx playerBallRotations[3];
float playerBallAngles[3];
Vec collisionPosition;
Vec collisionNormal;
float ballTrailDistances[256];
Vec awardOrbPositions[3];
Vec awardOrbScales[3];
s16 playerOrder[4];
Vec collisionVelocity;
s16 collisionCooldown;
u8 ballTrailHead;

float playerBallStartTimes[3] = { 90, 250, 420 };

Vec playerStartPositions[5] = { { -75, -300, 0 }, { 25, -300, 0 }, { -75, -300, 0 }, { 25, -300, 0 }, { -25, -400, 0 } };

float playerHorizontalBounds[5 * 2] = { -100, -60, 10, 50, -100, -60, 10, 50, -50, 0 };

s16 playerBallSlots[5] = {
    1,
    1,
    2,
    2,
    2,
};

void PinballStateInit(void)
{
    s16 i;
    for (i = 0; i < 5; i++) {
        playerPositions[i] = playerStartPositions[i];
        playerShadowPositions[i].x = playerPositions[i].x;
        playerShadowPositions[i].y = 215;
        playerShadowPositions[i].z = playerPositions[i].y;
        playerBallState[i][0] = 10;
        playerBallState[i][1] = 0;
    }
    for (i = 0; i < 3; i++) {
        ballModelPositions[i].x = 128;
        ballModelPositions[i].y = 230;
        ballModelPositions[i].z = -100;
        MTXIdentity(playerBallRotations[i]);
        playerBallAngles[i] = 0;
    }
}

void PinballRoundStart(s16 playerIndex)
{
    s16 shuffleIndex;
    s16 firstSwapIndex;
    s16 secondSwapIndex;
    s16 modelId;
    collisionCooldown = 0;
    for (shuffleIndex = 0; shuffleIndex < 256; shuffleIndex++) {
        ballTrailDistances[shuffleIndex] = 1000000.0f;
    }
    ballPosition.x = 128;
    ballPosition.y = -100;
    ballPosition.z = 0;
    ballVelocity.x = ballVelocity.z = 0;
    ballVelocity.y = -30;
    ballModelPositions[playerIndex].x = ballPosition.x;
    ballModelPositions[playerIndex].y = 230;
    ballModelPositions[playerIndex].z = ballPosition.y;
    PinballEffectStart(loadedModelIds[(playerIndex * 2) + 4]);
    if (playerIndex == 0) {
        for (shuffleIndex = 0; shuffleIndex < 4; shuffleIndex++) {
            playerOrder[shuffleIndex] = shuffleIndex;
        }
        for (shuffleIndex = 0; shuffleIndex < 20; shuffleIndex++) {
            firstSwapIndex = frandmod(4);
            secondSwapIndex = frandmod(4);
            modelId = playerOrder[firstSwapIndex];
            playerOrder[firstSwapIndex] = playerOrder[secondSwapIndex];
            playerOrder[secondSwapIndex] = modelId;
        }
        Hu3DModelHookSet(loadedModelIds[3], "d1_itemhook1", loadedModelIds[playerOrder[0] + 47]);
        Hu3DModelHookSet(loadedModelIds[3], "d1_itemhook2", loadedModelIds[playerOrder[1] + 47]);
        Hu3DModelHookSet(loadedModelIds[3], "d1_itemhook3", loadedModelIds[playerOrder[2] + 47]);
        Hu3DModelHookSet(loadedModelIds[3], "d1_itemhook4", loadedModelIds[playerOrder[3] + 47]);
        for (shuffleIndex = 0; shuffleIndex < 4; shuffleIndex++) {
            Hu3DModelAttrSet(loadedModelIds[shuffleIndex + 51], HU3D_ATTR_DISPOFF);
        }
    }
    else if (playerIndex == 1) {
        for (shuffleIndex = firstSwapIndex = 0; shuffleIndex < 4; shuffleIndex++) {
            if (shuffleIndex != firstAwardPlayer) {
                playerOrder[firstSwapIndex] = shuffleIndex;
                firstSwapIndex++;
            }
        }
        for (shuffleIndex = 0; shuffleIndex < 20; shuffleIndex++) {
            firstSwapIndex = frandmod(3);
            secondSwapIndex = frandmod(3);
            modelId = playerOrder[firstSwapIndex];
            playerOrder[firstSwapIndex] = playerOrder[secondSwapIndex];
            playerOrder[secondSwapIndex] = modelId;
        }
        Hu3DModelAttrReset(loadedModelIds[playerOrder[0] + 51], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(loadedModelIds[playerOrder[1] + 51], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(loadedModelIds[playerOrder[2] + 51], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(loadedModelIds[firstAwardPlayer + 51], HU3D_ATTR_DISPOFF);
        Hu3DModelHookSet(loadedModelIds[5], "d2_itemhook1", loadedModelIds[playerOrder[0] + 51]);
        Hu3DModelHookSet(loadedModelIds[5], "d2_itemhook2", loadedModelIds[playerOrder[1] + 51]);
        Hu3DModelHookSet(loadedModelIds[5], "d2_itemhook3", loadedModelIds[playerOrder[2] + 51]);
    }
    Hu3DModelAttrSet(loadedModelIds[playerIndex + 65], HU3D_MOTATTR_PAUSE);
    Hu3DMotionTimeSet(loadedModelIds[playerIndex + 65], 0);
    Hu3DModelAttrSet(loadedModelIds[playerIndex + 55], HU3D_MOTATTR_PAUSE);
    Hu3DMotionTimeSet(loadedModelIds[playerIndex + 55], 0);
    Hu3DMotionSpeedSet(loadedModelIds[playerIndex + 55], 2);
    Hu3DMotionSet(loadedModelIds[playerIndex + 58], loadedMotionIds[61]);
    Hu3DModelAttrSet(loadedModelIds[playerIndex + 58], HU3D_MOTATTR_LOOP);
    for (shuffleIndex = 0; shuffleIndex < 5; shuffleIndex++) {
        if (playerIndex == playerBallSlots[shuffleIndex]) {
            playerActiveFlags[shuffleIndex] = 1;
        }
        else {
            playerActiveFlags[shuffleIndex] = 0;
        }
        playerBallState[shuffleIndex][0] = 10;
        playerBallState[shuffleIndex][1] = 0;
    }
}

void PinballBallLaunch(s16 playerIndex, s16 ballIndex);
s16 PinballSegmentIntersect(Vec *ballPosition, Vec *ballVelocity, s16 ballIndex);

s16 PinballBallUpdate(s16 ballIndex)
{
    s16 characterIndex;
    s16 characterCount;
    s32 hasLaunchedBall;
    s16 landingCharacterIndex;
    Mtx rotationMatrix;
    Vec spinAxis;
    Vec zeroVector;
    float unusedScalar = 0;
    Hu3DMotionTimeSet(loadedModelIds[9], playerBallStartTimes[ballIndex]);
    Hu3DModelAttrSet(loadedModelIds[9], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(loadedModelIds[ballIndex + 65], HU3D_MOTATTR_PAUSE);
    Hu3DMotionSpeedSet(loadedModelIds[ballIndex + 65], 0.1 * (-ballVelocity.y - 14.0f) + 1.0);
    hasLaunchedBall = 0;
    zeroVector.z = zeroVector.y = zeroVector.x = 0;
    CameraViewTransition(1);
    while (1) {
        landingCharacterIndex = PinballSegmentIntersect(&ballPosition, &ballVelocity, ballIndex);
        ballModelPositions[ballIndex].x = ballPosition.x;
        ballModelPositions[ballIndex].y = 230;
        ballModelPositions[ballIndex].z = ballPosition.y;
        if (VECMag(&ballVelocity)) {
            spinAxis.x = 0;
            spinAxis.y = 0;
            spinAxis.z = 1;
            VECCrossProduct(&spinAxis, &ballVelocity, &spinAxis);
            spinAxis.z = spinAxis.y;
            spinAxis.y = 0;
            MTXRotAxisDeg(rotationMatrix, &spinAxis, 360.0 * (VECMag(&ballVelocity) / 81.68140899333463));
            MTXConcat(playerBallRotations[ballIndex], rotationMatrix, playerBallRotations[ballIndex]);
        }
        cameraTargetPosition.z = ballModelPositions[ballIndex].z;
        for (characterIndex = characterCount = 0; characterIndex < 5; characterIndex++) {
            if (ballIndex == playerBallSlots[characterIndex]) {
                PinballBallLaunch(characterIndex, ballIndex);
                if (playerBallState[characterIndex][1] & 0x2) {
                    characterCount++;
                }
            }
        }
        if (characterCount) {
            if (Hu3DMotionIDGet(loadedModelIds[ballIndex + 58]) != loadedMotionIds[62]) {
                Hu3DMotionSet(loadedModelIds[ballIndex + 58], loadedMotionIds[62]);
            }
        }
        else {
            if (Hu3DMotionIDGet(loadedModelIds[ballIndex + 58]) != loadedMotionIds[61]) {
                Hu3DMotionSet(loadedModelIds[ballIndex + 58], loadedMotionIds[61]);
            }
        }
        if (landingCharacterIndex != -1) {
            break;
        }
        if (ballPosition.x < 85.0f && hasLaunchedBall == 0) {
            upperBallTrailPositions[ballTrailCount].x = 100;
            upperBallTrailPositions[ballTrailCount].y = 100;
            upperBallTrailPositions[ballTrailCount].z = 0;
            lowerBallTrailPositions[ballTrailCount].x = 100;
            lowerBallTrailPositions[ballTrailCount].y = -500;
            lowerBallTrailPositions[ballTrailCount].z = 0;
            ballTrailCount++;
            Hu3DModelAttrReset(loadedModelIds[ballIndex + 55], HU3D_MOTATTR_PAUSE);
            Hu3DMotionTimeSet(loadedModelIds[ballIndex + 55], 0);
            hasLaunchedBall = 1;
        }
        HuPrcVSleep();
    }
    for (characterIndex = 0; characterIndex < 5; characterIndex++) {
        if (ballIndex == playerBallSlots[characterIndex]) {
            playerBallState[characterIndex][1] = 4;
            PinballBallLaunch(characterIndex, ballIndex);
        }
    }
    HuAudFXPlay(1851);
    return landingCharacterIndex;
}

void PinballPlayerUpdate(s16 playerIndex)
{
    float powerAngle;
    float motionDelta;
    float motionTime;
    float motionEndTime;

    s16 characterIndex;
    s16 windowId;
    s16 decelerationFrame;
    s16 computerWaitFrames;
    s16 pressedButtons;
    s16 stickY;
    s16 launchPower;
    s16 heldButtons;
    float windowSize[2];
    powerAngle = 0;
    CameraViewTransition(0);
    Hu3DModelPosSet(activeCharacterModel, 124, 0, 70);
    Hu3DModelRotSet(activeCharacterModel, 0, 180, 0);
    CharMotionShiftSet(activeCharacterId, characterMotions[3], 0, 4, HU3D_MOTATTR_SHIFT_REV);
    HuPrcSleep(12);
    HuWinMesMaxSizeGet(1, windowSize, MAKE_MESSID(0x1C, 0x0F));
    windowId = HuWinCreate(-10000, 340, windowSize[0], windowSize[1], 1);
    HuWinMesSpeedSet(windowId, 0);
    HuWinBGTPLvlSet(windowId, 0);
    HuWinMesSet(windowId, MAKE_MESSID(0x1C, 0x0F));
    CharMotionSet(activeCharacterId, characterMotions[2]);
    Hu3DModelAttrSet(activeCharacterModel, HU3D_MOTATTR_PAUSE);
    motionTime = Hu3DMotionTimeGet(activeCharacterModel);
    motionEndTime = Hu3DMotionMaxTimeGet(activeCharacterModel);
    if (GWPlayerCfg[activePlayerConfigIndex].iscom) {
        computerWaitFrames = frandmod(100) + 20;
    }
    while (1) {
        pressedButtons = HuPadBtnDown[activeControllerId];
        heldButtons = HuPadBtn[activeControllerId];
        stickY = HuPadStkY[activeControllerId];
        if (GWPlayerCfg[activePlayerConfigIndex].iscom) {
            heldButtons = 0;
            if (computerWaitFrames != 0) {
                computerWaitFrames--;
                stickY = -72;
            }
            else {
                pressedButtons = PAD_BUTTON_A;
            }
        }
        if ((pressedButtons & PAD_BUTTON_A) && playerBallAngles[playerIndex] > 0) {
            break;
        }
        if (HuPadTrigR[activeControllerId] > 10) {
            if (powerAngle < 90.0f) {
                powerAngle += 5.0f;
            }
            CameraViewTransition(playerIndex + 2);
        }
        else {
            CameraViewTransition(0);
            if (powerAngle > 0.0f) {
                powerAngle -= 5.0f;
            }
        }
        for (characterIndex = 0; characterIndex < 3; characterIndex++) {
            awardOrbPositions[characterIndex].y = 80.0 - (150.0 * sind(powerAngle));
        }
        motionDelta = -(stickY >> 3);
        motionDelta /= 20.0f;
        if (motionDelta) {
            if (motionDelta + playerBallAngles[playerIndex] < 0.0f) {
                motionDelta = -playerBallAngles[playerIndex];
            }
            else {
                if (motionDelta + playerBallAngles[playerIndex] > 50.0f) {
                    motionDelta = 50.0f - playerBallAngles[playerIndex];
                }
            }
            motionTime += motionDelta;
            if (motionTime >= motionEndTime) {
                motionTime -= motionEndTime;
            }
        }
        else {
            motionDelta = -0.3f;
            if (motionDelta + playerBallAngles[playerIndex] < 0.0f) {
                motionDelta = -playerBallAngles[playerIndex];
            }
        }
        Hu3DMotionTimeSet(activeCharacterModel, motionTime);
        playerBallAngles[playerIndex] += motionDelta;
        Hu3DModelPosSet(activeCharacterModel, 124, 0, 70 + playerBallAngles[playerIndex]);
        for (characterIndex = 0; characterIndex < 5; characterIndex++) {
            if (playerIndex == playerBallSlots[characterIndex]) {
                PinballBallLaunch(characterIndex, playerIndex);
            }
        }
        HuPrcVSleep();
    }
    if (powerAngle > 0) {
        CameraViewTransition(0);
        for (decelerationFrame = 0; decelerationFrame < 10; decelerationFrame++) {
            powerAngle -= 10.0f;
            if (powerAngle < 0.0f) {
                powerAngle = 0.0f;
            }
            for (characterIndex = 0; characterIndex < 3; characterIndex++) {
                awardOrbPositions[characterIndex].y = 80.0 - (150.0 * sind(powerAngle));
            }
            if (powerAngle == 0.0f) {
                break;
            }
            HuPrcVSleep();
        }
    }
    CharMotionShiftSet(activeCharacterId, characterMotions[3], 0, 8, HU3D_MOTATTR_NONE);
    HuPrcSleep(4);
    launchPower = playerBallAngles[playerIndex] / 4;
    ballVelocity.y = (-15 - launchPower) + (0.1 * (float)frandmod(10));
    playerBallAngles[playerIndex] = 0;
    HuWinExCleanup(windowId);
    HuAudFXPlay(1852);
}

void PinballTargetHit(s16 playerIndex, s16 targetIndex)
{
    Mtx objectMatrix;
    Mtx translationMatrix;
    HU3DMODEL *modelData;

    Hu3DMotionSet(loadedModelIds[playerIndex + 58], loadedMotionIds[63]);
    Hu3DModelAttrSet(loadedModelIds[playerIndex + 58], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrReset(loadedModelIds[64], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(loadedModelIds[64], HU3D_MOTATTR_LOOP);
    Hu3DMotionTimeSet(loadedModelIds[64], 0);
    MTXTrans(translationMatrix, 0, -150, 0);
    if (playerIndex == 0) {
        switch (targetIndex) {
            case 0:
                Hu3DModelObjMtxGet(loadedModelIds[3], "d1_itemhook1", objectMatrix);
                break;

            case 1:
                Hu3DModelObjMtxGet(loadedModelIds[3], "d1_itemhook2", objectMatrix);
                break;

            case 2:
                Hu3DModelObjMtxGet(loadedModelIds[3], "d1_itemhook3", objectMatrix);
                break;

            case 3:
                Hu3DModelObjMtxGet(loadedModelIds[3], "d1_itemhook4", objectMatrix);
                break;
        }
    }
    else if (playerIndex == 1) {
        switch (targetIndex) {
            case 0:
                Hu3DModelObjMtxGet(loadedModelIds[5], "d2_itemhook1", objectMatrix);
                break;

            case 1:
                Hu3DModelObjMtxGet(loadedModelIds[5], "d2_itemhook2", objectMatrix);
                break;

            case 2:
                Hu3DModelObjMtxGet(loadedModelIds[5], "d2_itemhook3", objectMatrix);
                break;
        }
    }
    else {
        switch (targetIndex) {
            case 0:
                Hu3DModelObjMtxGet(loadedModelIds[7], "d3_itemhook1", objectMatrix);
                break;

            case 1:
                Hu3DModelObjMtxGet(loadedModelIds[7], "d3_itemhook4", objectMatrix);
                break;

            case 2:
                Hu3DModelObjMtxGet(loadedModelIds[7], "d3_itemhook2", objectMatrix);
                break;

            case 3:
                Hu3DModelObjMtxGet(loadedModelIds[7], "d3_itemhook7", objectMatrix);
                break;

            case 4:
                Hu3DModelObjMtxGet(loadedModelIds[7], "d3_itemhook6", objectMatrix);
                break;

            case 5:
                Hu3DModelObjMtxGet(loadedModelIds[7], "d3_itemhook3", objectMatrix);
                break;

            case 6:
                Hu3DModelObjMtxGet(loadedModelIds[7], "d3_itemhook5", objectMatrix);
                break;
        }
        mtxScaleCat(translationMatrix, 0.8, 1, 0.8);
    }
    modelData = &Hu3DData[loadedModelIds[64]];
    MTXConcat(objectMatrix, translationMatrix, objectMatrix);
    MTXCopy(objectMatrix, modelData->mtx);
}

void PinballPlayerProcess(void)
{
    s16 modelIndex;
    s16 orbIndex;
    s16 randomCharacterIndex;
    s16 selectedCharacterModel;
    s16 animationFrame;
    Vec worldPosition;
    Vec screenPosition;
    float orbAngles[3] = { 0, 40, 60 };
    for (orbIndex = 0; orbIndex < 3; orbIndex++) {
        awardOrbPositions[orbIndex].y = -100;
        awardOrbPositions[orbIndex].z = 200;
        awardOrbScales[orbIndex].x = awardOrbScales[orbIndex].y = awardOrbScales[orbIndex].z = 0.3f;
        Hu3DModelAttrReset(loadedModelIds[orbIndex + 71], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(loadedModelIds[orbIndex + 71], HU3D_MOTATTR_LOOP);
    }
    awardOrbPositions[0].x = 188;
    awardOrbPositions[1].x = 388;
    awardOrbPositions[2].x = 288;
    randomCharacterIndex = animationFrame = 0;
    while (1) {
        for (orbIndex = 0; orbIndex < 3; orbIndex++) {
            screenPosition = awardOrbPositions[orbIndex];
            screenPosition.y += 5.0 * sind(orbAngles[orbIndex]);
            Hu3D2Dto3D(&screenPosition, 1, &worldPosition);
            Hu3DModelPosSet(loadedModelIds[orbIndex + 71], worldPosition.x, worldPosition.y, worldPosition.z);
            Hu3DModelRotSet(loadedModelIds[orbIndex + 71], cameraRotation[0].x, cameraRotation[0].y, cameraRotation[0].z);
            Hu3DModelScaleSet(
                loadedModelIds[orbIndex + 71], awardOrbScales[orbIndex].x / 2.0, awardOrbScales[orbIndex].y / 2.0, awardOrbScales[orbIndex].z / 2.0);
            switch (orbIndex) {
                case 0:
                    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
                        Hu3DModelScaleSet(loadedModelIds[modelIndex + 74], awardOrbScales[orbIndex].x * 1.2, awardOrbScales[orbIndex].y * 1.2,
                            awardOrbScales[orbIndex].z * 1.2);
                    }
                    if (firstAwardPlayer != -2) {
                        if (firstAwardPlayer == -1) {
                            if ((animationFrame & 0x7) == 0 && (++randomCharacterIndex) >= 4) {
                                randomCharacterIndex = 0;
                            }
                            animationFrame++;
                            selectedCharacterModel = loadedModelIds[randomCharacterIndex + 74];
                            for (modelIndex = 0; modelIndex < 4; modelIndex++) {
                                Hu3DModelAttrSet(loadedModelIds[modelIndex + 74], HU3D_ATTR_DISPOFF);
                            }
                            Hu3DModelAttrReset(selectedCharacterModel, HU3D_ATTR_DISPOFF);
                        }
                        else {
                            selectedCharacterModel = loadedModelIds[firstAwardPlayer + 74];
                            for (modelIndex = 0; modelIndex < 4; modelIndex++) {
                                Hu3DModelAttrSet(loadedModelIds[modelIndex + 74], HU3D_ATTR_DISPOFF);
                            }
                            Hu3DModelAttrReset(selectedCharacterModel, HU3D_ATTR_DISPOFF);
                        }
                        Hu3DModelPosSet(selectedCharacterModel, worldPosition.x, worldPosition.y, worldPosition.z);
                        Hu3DModelRotSet(selectedCharacterModel, 90.0f + cameraRotation[0].x, cameraRotation[0].y, cameraRotation[0].z);
                    }
                    break;

                case 1:
                    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
                        if (modelIndex == firstAwardPlayer) {
                            continue;
                        }
                        Hu3DModelScaleSet(loadedModelIds[modelIndex + 74], awardOrbScales[orbIndex].x * 1.2, awardOrbScales[orbIndex].y * 1.2,
                            awardOrbScales[orbIndex].z * 1.2);
                    }
                    if (secondAwardPlayer != -2) {
                        if (secondAwardPlayer == -1) {
                            if ((animationFrame & 0x7) == 0) {
                                randomCharacterIndex++;
                                if (randomCharacterIndex == firstAwardPlayer) {
                                    randomCharacterIndex++;
                                }
                                if (randomCharacterIndex >= 4) {
                                    randomCharacterIndex = 0;
                                    if (firstAwardPlayer == 0) {
                                        randomCharacterIndex++;
                                    }
                                }
                            }
                            else {
                                if (randomCharacterIndex == firstAwardPlayer) {
                                    randomCharacterIndex++;
                                    if (randomCharacterIndex >= 4) {
                                        randomCharacterIndex = 0;
                                    }
                                }
                            }
                            animationFrame++;
                            selectedCharacterModel = loadedModelIds[randomCharacterIndex + 74];
                            for (modelIndex = 0; modelIndex < 4; modelIndex++) {
                                if (modelIndex != firstAwardPlayer) {
                                    Hu3DModelAttrSet(loadedModelIds[modelIndex + 74], HU3D_ATTR_DISPOFF);
                                }
                            }
                            Hu3DModelAttrReset(selectedCharacterModel, HU3D_ATTR_DISPOFF);
                        }
                        else {
                            selectedCharacterModel = loadedModelIds[secondAwardPlayer + 74];
                            for (modelIndex = 0; modelIndex < 4; modelIndex++) {
                                if (modelIndex != firstAwardPlayer) {
                                    Hu3DModelAttrSet(loadedModelIds[modelIndex + 74], HU3D_ATTR_DISPOFF);
                                }
                            }
                            Hu3DModelAttrReset(selectedCharacterModel, HU3D_ATTR_DISPOFF);
                        }
                        Hu3DModelPosSet(selectedCharacterModel, worldPosition.x, worldPosition.y, worldPosition.z);
                        Hu3DModelRotSet(selectedCharacterModel, 90.0f + cameraRotation[0].x, cameraRotation[0].y, cameraRotation[0].z);
                    }
                    break;

                case 2:
                    for (modelIndex = 0; modelIndex < 7; modelIndex++) {
                        Hu3DModelScaleSet(loadedModelIds[modelIndex + 78], awardOrbScales[orbIndex].x * 1.2, awardOrbScales[orbIndex].y * 1.2,
                            awardOrbScales[orbIndex].z * 1.2);
                    }
                    if (thirdAwardPlayer != -2) {
                        if (thirdAwardPlayer == -1) {
                            if ((animationFrame & 0x7) == 0 && (++randomCharacterIndex) >= 7) {
                                randomCharacterIndex = 0;
                            }
                            animationFrame++;
                            selectedCharacterModel = loadedModelIds[randomCharacterIndex + 78];
                            for (modelIndex = 0; modelIndex < 7; modelIndex++) {
                                Hu3DModelAttrSet(loadedModelIds[modelIndex + 78], HU3D_ATTR_DISPOFF);
                            }
                            Hu3DModelAttrReset(selectedCharacterModel, HU3D_ATTR_DISPOFF);
                        }
                        else {
                            selectedCharacterModel = loadedModelIds[thirdAwardPlayer + 78];
                            for (modelIndex = 0; modelIndex < 7; modelIndex++) {
                                Hu3DModelAttrSet(loadedModelIds[modelIndex + 78], HU3D_ATTR_DISPOFF);
                            }
                            Hu3DModelAttrReset(selectedCharacterModel, HU3D_ATTR_DISPOFF);
                        }
                        Hu3DModelPosSet(selectedCharacterModel, worldPosition.x, worldPosition.y, worldPosition.z);
                        Hu3DModelRotSet(selectedCharacterModel, 90.0f + cameraRotation[0].x, cameraRotation[0].y, cameraRotation[0].z);
                    }
                    break;
            }
            orbAngles[orbIndex] += 4.0 * (1.0 + (orbIndex * 0.05));
            if (orbAngles[orbIndex] > 360.0f) {
                orbAngles[orbIndex] -= 360.0f;
            }
        }
        HuPrcVSleep();
    }
}

void PinballBallLaunch(s16 playerIndex, s16 ballIndex)
{
    float moveDirection;
    float turnAngle;
    float unusedFloat;
    s16 *characterState;
    s16 characterModel;
    s16 otherCharacterIndex;
    Vec separationVector;
    Vec candidatePosition;
    characterState = &playerBallState[playerIndex][0];
    characterModel = loadedModelIds[playerIndex + 38];
    if (characterState[1] & 0x2) {
        if (characterState[0] == 85) {
            if (Hu3DMotionIDGet(characterModel) != loadedMotionIds[45] && Hu3DMotionShiftIDGet(characterModel) != loadedMotionIds[45]) {
                Hu3DMotionShiftSet(characterModel, loadedMotionIds[45], 0, 8, HU3D_MOTATTR_NONE);
            }
        }
        else {
            if (Hu3DMotionIDGet(characterModel) != loadedMotionIds[46] && Hu3DMotionShiftIDGet(characterModel) != loadedMotionIds[46]) {
                Hu3DMotionShiftSet(characterModel, loadedMotionIds[46], 0, 8, HU3D_MOTATTR_LOOP);
            }
        }
        characterState[0]--;
        if (characterState[0] <= 0) {
            characterState[1] &= ~0x2;
            characterState[1] |= 0x4;
            characterState[0] = 20;
        }
    }
    else {
        if (characterState[1] & 0x4) {
            if (Hu3DMotionIDGet(characterModel) != loadedMotionIds[43] && Hu3DMotionShiftIDGet(characterModel) != loadedMotionIds[43]) {
                Hu3DMotionShiftSet(characterModel, loadedMotionIds[43], 0, 16, HU3D_MOTATTR_LOOP);
            }
            characterState[0]--;
            if (characterState[0] <= 0) {
                characterState[1] &= ~0x4;
                characterState[0] = frandmod(30) + 30;
            }
        }
        else {
            characterState[0]--;
            if (characterState[0] <= 0) {
                characterState[1] |= 4;
                characterState[0] = 60;
            }
            else {
                if (Hu3DMotionIDGet(characterModel) != loadedMotionIds[44]) {
                    Hu3DMotionSet(characterModel, loadedMotionIds[44]);
                }
                if (characterState[1] & 0x1) {
                    moveDirection = -1;
                    turnAngle = -90;
                }
                else {
                    moveDirection = 1;
                    turnAngle = 90;
                }
                candidatePosition = playerPositions[playerIndex];
                candidatePosition.x += moveDirection;
                for (otherCharacterIndex = 0; otherCharacterIndex < 5; otherCharacterIndex++) {
                    if (otherCharacterIndex == playerIndex || playerActiveFlags[otherCharacterIndex] == 0) {
                        continue;
                    }
                    VECSubtract(&candidatePosition, &playerPositions[otherCharacterIndex], &separationVector);
                    if (VECMag(&separationVector) < 55.0f) {
                        break;
                    }
                }
                if (otherCharacterIndex == 5) {
                    playerPositions[playerIndex].x += moveDirection;
                    if (playerPositions[playerIndex].x < playerHorizontalBounds[(playerIndex * 2)]) {
                        playerPositions[playerIndex].x = playerHorizontalBounds[(playerIndex * 2)];
                        characterState[1] ^= 0x1;
                        characterState[1] |= 0x4;
                        characterState[0] = 60;
                    }
                    if (playerPositions[playerIndex].x > playerHorizontalBounds[(playerIndex * 2) + 1]) {
                        playerPositions[playerIndex].x = playerHorizontalBounds[(playerIndex * 2) + 1];
                        characterState[1] ^= 0x1;
                        characterState[1] |= 0x4;
                        characterState[0] = 60;
                    }
                }
                else {
                    characterState[1] ^= 0x1;
                    characterState[1] |= 0x4;
                    characterState[0] = 60;
                }
            }
        }
    }
    playerShadowPositions[playerIndex].x = playerPositions[playerIndex].x;
    playerShadowPositions[playerIndex].y = 215;
    playerShadowPositions[playerIndex].z = playerPositions[playerIndex].y;
}

s16 awardOrbCounts[] = { 4, 3, 7 };

Vec singleAwardOrbPositions[] = { { -118.75, -70, 0 }, { -56.25, -70, 0 }, { 6.25, -70, 0 }, { 68.75, -70, 0 } };

Vec multiAwardOrbPositions[] = {
    { -108.35, -70, 0 },
    { -25.05, -70, 0 },
    { 58.25, -70, 0 },
};

Vec bonusAwardOrbPositions[] = { { -125, -70, 0 }, { -75, -70, 0 }, { -25, -70, 0 }, { 25, -70, 0 }, { 75, -70, 0 }, { -85, -190, 0 }, { 35, -190, 0 } };

Vec *awardOrbPositionSets[] = { singleAwardOrbPositions, multiAwardOrbPositions, bonusAwardOrbPositions };

Vec *PinballCollisionSurfaceGet(Vec *point, Vec *lineOrigin, Vec *lineDirection);

float VectorDistance(Vec *point, Vec *lineOrigin, Vec *lineDirection);

s16 PinballSegmentIntersect(Vec *ballPosition, Vec *ballVelocity, s16 ballIndex)
{
    Vec nextBallPosition;
    Vec obstacleDirection;
    Vec collisionVector;
    Vec reflectionVector;
    Vec ballVelocity;
    Vec previousBallPosition;
    s32 unusedCollisionFlag;

    float stepDistance;
    float distance;
    float remainingDistance;
    float projection;

    s16 obstacleIndex;
    Vec *targetPosition;
    s32 collisionOccurred;
    Vec *collisionSurface;
    u8 trailHistoryIndex;

    s16 pocketIndex = -1;
    unusedCollisionFlag = 1;
    remainingDistance = VECMag(ballVelocity);
    ballVelocity = *ballVelocity;
    collisionOccurred = 0;
    previousBallPosition = *ballPosition;
    trailHistoryIndex = ballTrailHead - 120;
    for (obstacleIndex = distance = 0; obstacleIndex < 120; obstacleIndex++, trailHistoryIndex++) {
        distance += ballTrailDistances[trailHistoryIndex];
    }
    if (distance < 120.0f * sqrtf(6.0f)) {
        collisionCooldown = 10;
    }
    while (remainingDistance > 0.0f) {
        if (remainingDistance > 3.0f) {
            remainingDistance -= 3.0f;
            stepDistance = 3.0f;
        }
        else {
            stepDistance = remainingDistance;
            remainingDistance = 0;
        }
        VECNormalize(&ballVelocity, &ballVelocity);
        VECScale(&ballVelocity, &ballVelocity, stepDistance);
        VECAdd(ballPosition, &ballVelocity, &nextBallPosition);
        stepDistance = VECMag(&ballVelocity);
        if (collisionCooldown == 0) {
            for (obstacleIndex = 0; obstacleIndex < 5; obstacleIndex++) {
                if (playerActiveFlags[obstacleIndex] == 0) {
                    continue;
                }
                VECSubtract(&nextBallPosition, &playerPositions[obstacleIndex], &collisionVector);
                distance = VECMag(&collisionVector);
                if (distance < 33.0) {
                    VECNormalize(&collisionVector, &collisionVector);
                    VECScale(&collisionVector, &reflectionVector, 33.0f);
                    VECAdd(&playerPositions[obstacleIndex], &reflectionVector, &nextBallPosition);
                    VECScale(&ballVelocity, &reflectionVector, -1.0f);
                    projection = (reflectionVector.x * collisionVector.x) + (reflectionVector.y * collisionVector.y);
                    collisionVector.x = (2.0 * collisionVector.x * projection) - reflectionVector.x;
                    collisionVector.y = (2.0 * collisionVector.y * projection) - reflectionVector.y;
                    collisionVector.z = 0;
                    VECNormalize(&collisionVector, &collisionVector);
                    VECScale(&collisionVector, &ballVelocity, stepDistance);
                    VECAdd(&nextBallPosition, &ballVelocity, &collisionVector);
                    distance = VECSquareDistance(&collisionVector, &playerPositions[obstacleIndex]);
                    if (distance < 1089.0) {
                        VECScale(&ballVelocity, &ballVelocity, -1.0f);
                    }
                    playerBallState[obstacleIndex][1] = 2;
                    playerBallState[obstacleIndex][0] = 100;
                    collisionOccurred = 1;
                }
            }
        }
        else {
            collisionCooldown--;
        }
        for (obstacleIndex = 0; obstacleIndex < ballTrailCount; obstacleIndex++) {
            if (VECSquareDistance(&upperBallTrailPositions[obstacleIndex], &nextBallPosition) <= 169.0f) {
                VECSubtract(&nextBallPosition, &upperBallTrailPositions[obstacleIndex], &collisionVector);
                VECNormalize(&collisionVector, &collisionVector);
                VECScale(&collisionVector, &collisionVector, 13.05f);
                VECAdd(&upperBallTrailPositions[obstacleIndex], &collisionVector, &nextBallPosition);
                VECScale(ballVelocity, ballVelocity, 0.96f);
                collisionOccurred = 1;
            }
            else {
                if (VECSquareDistance(&lowerBallTrailPositions[obstacleIndex], &nextBallPosition) <= 169.0f) {
                    VECSubtract(&nextBallPosition, &lowerBallTrailPositions[obstacleIndex], &collisionVector);
                    VECNormalize(&collisionVector, &collisionVector);
                    VECScale(&collisionVector, &collisionVector, 13.05f);
                    VECAdd(&lowerBallTrailPositions[obstacleIndex], &collisionVector, &nextBallPosition);
                    VECScale(ballVelocity, ballVelocity, 0.96f);
                    collisionOccurred = 1;
                }
                else {
                    continue;
                }
            }
            *ballPosition = nextBallPosition;
            break;
        }
        if (stepDistance == 0.0) {
            continue;
        }
        for (obstacleIndex = 0; obstacleIndex < ballTrailCount; obstacleIndex++) {
            VECSubtract(&lowerBallTrailPositions[obstacleIndex], &upperBallTrailPositions[obstacleIndex], &obstacleDirection);
            distance = VectorDistance(&nextBallPosition, &upperBallTrailPositions[obstacleIndex], &obstacleDirection);
            if (distance < 13.0f) {
                collisionSurface = PinballCollisionSurfaceGet(&nextBallPosition, &upperBallTrailPositions[obstacleIndex], &obstacleDirection);
                if (VECSquareDistance(&upperBallTrailPositions[obstacleIndex], &nextBallPosition) >= 169.0f && VECSquareDistance(&lowerBallTrailPositions[obstacleIndex], &nextBallPosition) >= 169.0f) {
                    VECSubtract(collisionSurface, &upperBallTrailPositions[obstacleIndex], &collisionVector);
                    if (VECMag2Point(&collisionVector) > VECMag2Point(&obstacleDirection)) {
                        continue;
                    }
                    VECSubtract(collisionSurface, &lowerBallTrailPositions[obstacleIndex], &collisionVector);
                    if (VECMag2Point(&collisionVector) > VECMag2Point(&obstacleDirection)) {
                        continue;
                    }
                }
                VECSubtract(&nextBallPosition, collisionSurface, &collisionVector);
                VECNormalize(&collisionVector, &collisionVector);
                VECScale(&collisionVector, &collisionVector, 13.0f);
                VECAdd(collisionSurface, &collisionVector, &nextBallPosition);
                VECSubtract(ballPosition, &nextBallPosition, &collisionVector);
                *ballPosition = nextBallPosition;
                stepDistance -= VECMag(&collisionVector);
                if (stepDistance < 0.0f) {
                    remainingDistance -= stepDistance;
                    stepDistance = ABS(stepDistance);
#if VERSION_REV2
                    if (stepDistance == 0.0) {
                        ballVelocity.x = ballVelocity.y = ballVelocity.z = 0.0f;
                        break;
                    }
#endif
                }
                collisionVector = obstacleDirection;
                collisionVector.z = 1;
                VECCrossProduct(&collisionVector, &obstacleDirection, &collisionVector);
                VECNormalize(&collisionVector, &collisionVector);
                VECScale(&ballVelocity, &reflectionVector, -1.0f);
                projection = (reflectionVector.x * collisionVector.x) + (reflectionVector.y * collisionVector.y);
                collisionVector.x = (1.5 * collisionVector.x * projection) - reflectionVector.x;
                collisionVector.y = (1.5 * collisionVector.y * projection) - reflectionVector.y;
                collisionVector.z = 0;
                VECNormalize(&collisionVector, &collisionVector);
                VECScale(&collisionVector, &ballVelocity, stepDistance);
                ballTrailFlags[obstacleIndex] = 10;
                VECScale(ballVelocity, ballVelocity, 0.96f);
                VECNormalize(&ballVelocity, &collisionVector);
                VECNormalize(ballVelocity, &reflectionVector);
                if (VECDotProduct(&collisionVector, &reflectionVector) < 0.5) {
                    collisionOccurred = 1;
                }
            }
        }
        VECAdd(ballPosition, &ballVelocity, ballPosition);
        if (ballPosition->x >= collisionPosition.x - 13.0f) {
            ballPosition->x = collisionPosition.x - 13.0f - 1.0f;
        }
        if (ballPosition->y >= collisionPosition.y - 13.0f) {
            ballPosition->y = collisionPosition.y - 13.0f - 1.0f;
        }
        if (ballPosition->x <= collisionNormal.x + 13.0f) {
            ballPosition->x = collisionNormal.x + 13.0f + 1.0f;
        }
        if (ballPosition->y <= collisionNormal.y + 13.0f) {
            ballPosition->y = collisionNormal.y + 13.0f + 1.0f;
        }
    }
    stepDistance = VECMag(ballVelocity);
#if VERSION_REV2
    if (VECMag(&ballVelocity) == 0.0) {
        ballVelocity.x = 0.0f;
        ballVelocity.y = 1.0f;
        ballVelocity.z = 0.0f;
    }
#endif
    VECNormalize(&ballVelocity, &ballVelocity);
    VECScale(&ballVelocity, ballVelocity, stepDistance);
    if (collisionOccurred != 0) {
        HuAudFXPlay(1853);
    }
    if (ballIndex < 0 || ballIndex >= 3) {
        ballIndex = 0;
    }
    targetPosition = awardOrbPositionSets[ballIndex];
    for (obstacleIndex = 0; obstacleIndex < awardOrbCounts[ballIndex]; obstacleIndex++, targetPosition++) {
        VECSubtract(targetPosition, ballPosition, &collisionVector);
        distance = VECMag(&collisionVector);
        if (distance < 3.0f && VECMag(ballVelocity) < 1.0f) {
            ballVelocity->x = ballVelocity->y = ballVelocity->z = 0.0f;
            *ballPosition = *targetPosition;
            pocketIndex = obstacleIndex;
            break;
        }
        if (distance < 20.0f
#if VERSION_REV2
            && !(distance < 0.000001)
#endif
        ) {
            VECScale(ballVelocity, ballVelocity, 0.9f);
            VECNormalize(&collisionVector, &collisionVector);
            VECScale(&collisionVector, &collisionVector, 0.1 * (20.0f - distance));
            VECAdd(ballVelocity, &collisionVector, ballVelocity);
            break;
        }
    }
    if (obstacleIndex == awardOrbCounts[ballIndex]) {
        ballVelocity->y += 0.3;
#if VERSION_REV2
        if (VECMag((Vec *)&ballVelocity) < 0.000001) {
            ballVelocity->y += 0.3;
        }
#endif
    }
    if (ballPosition->x >= collisionPosition.x - 13.0f) {
        ballPosition->x = collisionPosition.x - 13.0f - 1.0f;
    }
    if (ballPosition->y >= collisionPosition.y - 13.0f) {
        ballPosition->y = collisionPosition.y - 13.0f - 1.0f;
    }
    if (ballPosition->x <= collisionNormal.x + 13.0f) {
        ballPosition->x = collisionNormal.x + 13.0f + 1.0f;
    }
    if (ballPosition->y <= collisionNormal.y + 13.0f) {
        ballPosition->y = collisionNormal.y + 13.0f + 1.0f;
    }
    VECSubtract(ballPosition, &previousBallPosition, &collisionVector);
    ballTrailDistances[ballTrailHead++] = VECMag(&collisionVector);
    return pocketIndex;
}

void PinballCollisionProcess(void)
{
    Mtx modelMatrix;
    Mtx translationMatrix;
    Mtx boardMatrix;
    Mtx rotationMatrix;
    HU3DMODEL *modelData;
    s16 ballIndex;
    s16 characterIndex;
    MTXRotDeg(rotationMatrix, 'z', 120);
    while (1) {
        modelData = &Hu3DData[loadedModelIds[9]];
        Hu3DMotionExec(loadedModelIds[9], modelData->motId, modelData->motWork.time, 0);
        Hu3DModelObjMtxGet(loadedModelIds[9], "kaitendai-r_dai", boardMatrix);
        for (ballIndex = 0; ballIndex < 3; ballIndex++) {
            modelData = &Hu3DData[loadedModelIds[ballIndex]];
            MTXTrans(translationMatrix, ballModelPositions[ballIndex].x, ballModelPositions[ballIndex].y, ballModelPositions[ballIndex].z);
            MTXConcat(translationMatrix, playerBallRotations[ballIndex], translationMatrix);
            MTXConcat(boardMatrix, translationMatrix, modelMatrix);
            MTXCopy(modelMatrix, modelData->mtx);
            modelData = &Hu3DData[loadedModelIds[ballIndex + 55]];
            MTXTrans(translationMatrix, 0, 150, 0);
            MTXConcat(boardMatrix, translationMatrix, modelMatrix);
            MTXCopy(modelMatrix, modelData->mtx);
            modelData = &Hu3DData[loadedModelIds[ballIndex + 58]];
            MTXTrans(translationMatrix, 0, 150, 0);
            MTXConcat(boardMatrix, translationMatrix, modelMatrix);
            MTXCopy(modelMatrix, modelData->mtx);
            modelData = &Hu3DData[loadedModelIds[ballIndex + 65]];
            MTXTrans(translationMatrix, 0, 150, 0);
            MTXConcat(boardMatrix, translationMatrix, modelMatrix);
            MTXCopy(modelMatrix, modelData->mtx);
            modelData = &Hu3DData[loadedModelIds[ballIndex + 68]];
            MTXTrans(translationMatrix, 0, 150, playerBallAngles[ballIndex] - 5.0f);
            MTXConcat(boardMatrix, translationMatrix, modelMatrix);
            MTXCopy(modelMatrix, modelData->mtx);
            for (characterIndex = 0; characterIndex < 5; characterIndex++) {
                if (ballIndex != playerBallSlots[characterIndex]) {
                    continue;
                }
                modelData = &Hu3DData[loadedModelIds[characterIndex + 38]];
                MTXTrans(translationMatrix, playerShadowPositions[characterIndex].x, playerShadowPositions[characterIndex].y, playerShadowPositions[characterIndex].z);
                mtxScaleCat(translationMatrix, 0.35f, 0.35f, 0.35f);
                MTXConcat(boardMatrix, translationMatrix, modelMatrix);
                MTXCopy(modelMatrix, modelData->mtx);
                modelData = &Hu3DData[loadedModelIds[characterIndex + 85]];
                MTXTrans(translationMatrix, playerShadowPositions[characterIndex].x, playerShadowPositions[characterIndex].y + 1.0f, playerShadowPositions[characterIndex].z);
                mtxScaleCat(translationMatrix, 0.5f, 0.5f, 0.5f);
                MTXConcat(boardMatrix, translationMatrix, modelMatrix);
                MTXCopy(modelMatrix, modelData->mtx);
            }
            MTXConcat(boardMatrix, rotationMatrix, boardMatrix);
        }
        HuPrcVSleep();
    }
}

void PinballModelMatrixUpdate(HU3DMODEL *model, Mtx matrix)
{
    s16 trailIndex;
    u8 red;
    u8 green;
    u8 blue;

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_S8, 0);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXLoadPosMtxImm(matrix, GX_PNMTX0);
    red = green = blue = 255;
    for (trailIndex = 0; trailIndex < ballTrailCount; trailIndex++) {
        if (ballTrailFlags[trailIndex]) {
            red = 255;
            green = blue = 0;
        }
        else {
            red = green = blue = 255;
        }
        GXBegin(GX_LINES, GX_VTXFMT0, 2);
        GXPosition3f32(upperBallTrailPositions[trailIndex].x, 150, upperBallTrailPositions[trailIndex].y);
        GXColor3u8(red, green, blue);
        GXPosition3f32(lowerBallTrailPositions[trailIndex].x, 150, lowerBallTrailPositions[trailIndex].y);
        GXColor3u8(red, green, blue);
    }
}

#define VECDistance2XYZ3(a, b, c)                                                                                                                    \
    ((((a)->x * (b)->x) - ((a)->x * (c)->x)) + (((a)->y * (b)->y) - ((a)->y * (c)->y)) + (((a)->z * (b)->z) - ((a)->z * (c)->z)))

Vec *PinballCollisionSurfaceGet(Vec *ballPosition, Vec *obstaclePosition, Vec *obstacleDirection)
{
    float projectionDistance;
    Vec normalizedDirection;
    VECNormalize(obstacleDirection, &normalizedDirection);
    projectionDistance = -VECDistance2XYZ3(&normalizedDirection, obstaclePosition, ballPosition);
    projectionDistance /= VECMag2Point(&normalizedDirection);
    collisionVelocity.x = (obstaclePosition->x + (normalizedDirection.x * projectionDistance));
    collisionVelocity.y = (obstaclePosition->y + (normalizedDirection.y * projectionDistance));
    collisionVelocity.z = (obstaclePosition->z + (normalizedDirection.z * projectionDistance));
    return &collisionVelocity;
}

float VectorDistance(Vec *point, Vec *lineOrigin, Vec *lineDirection)
{
    Vec *trailIndex = PinballCollisionSurfaceGet(startPosition, endPosition, closestPoint);
    trailIndex->x -= startPosition->x;
    trailIndex->y -= startPosition->y;
    trailIndex->z -= startPosition->z;
    return sqrtf(VECMag2Point(trailIndex));
}

s32 SegmentSphereIntersect(Vec *segmentStart, Vec *segmentDirection, Vec *sphereCenter, Vec *sphereDirection, Vec *intersectionPoint)
{
    float intersectionDistance;
    float lineParameter;
    s32 unusedFlag = 1;
    intersectionDistance = (segmentDirection->x * (sphereCenter->y - segmentStart->y)) - (segmentDirection->y * (sphereCenter->x - segmentStart->x));
    if (0.0f != (sphereDirection->x * segmentDirection->y) - (sphereDirection->y * segmentDirection->x)) {
        intersectionDistance /= (sphereDirection->x * segmentDirection->y) - (sphereDirection->y * segmentDirection->x);
    }
    else {
        return 0;
    }
    if (segmentDirection->x != 0.0) {
        lineParameter = (((intersectionDistance * sphereDirection->x) + sphereCenter->x) - segmentStart->x) / segmentDirection->x;
    }
    else if (segmentDirection->y != 0.0) {
        lineParameter = (((intersectionDistance * sphereDirection->y) + sphereCenter->y) - segmentStart->y) / segmentDirection->y;
    }
    else {
        lineParameter = (((intersectionDistance * sphereDirection->z) + sphereCenter->z) - segmentStart->z) / segmentDirection->z;
    }
    intersectionPoint->x = (lineParameter * segmentDirection->x) + segmentStart->x;
    intersectionPoint->y = (lineParameter * segmentDirection->y) + segmentStart->y;
    intersectionPoint->z = (lineParameter * segmentDirection->z) + segmentStart->z;
    return 1;
}

s32 SegmentCapsuleIntersect(Vec *segmentStart, Vec *segmentEnd, Vec *capsuleStart, Vec *capsuleEnd, Vec *intersectionPoint)
{
    float intersectionDistance;
    float lineParameter;
    Vec firstLineDirection;
    Vec secondLineDirection;
    firstLineDirection.x = segmentEnd->x - segmentStart->x;
    firstLineDirection.y = segmentEnd->y - segmentStart->y;
    firstLineDirection.z = segmentEnd->z - segmentStart->z;
    secondLineDirection.x = capsuleEnd->x - capsuleStart->x;
    secondLineDirection.y = capsuleEnd->y - capsuleStart->y;
    secondLineDirection.z = capsuleEnd->z - capsuleStart->z;
    intersectionDistance = (firstLineDirection.x * (capsuleStart->y - segmentStart->y)) - (firstLineDirection.y * (capsuleStart->x - segmentStart->x));
    if (0.0f != (secondLineDirection.x * firstLineDirection.y) - (secondLineDirection.y * firstLineDirection.x)) {
        intersectionDistance /= (secondLineDirection.x * firstLineDirection.y) - (secondLineDirection.y * firstLineDirection.x);
    }
    else {
        return 0;
    }
    if (firstLineDirection.x != 0.0) {
        lineParameter = (((intersectionDistance * secondLineDirection.x) + capsuleStart->x) - segmentStart->x) / firstLineDirection.x;
    }
    else if (firstLineDirection.y != 0.0) {
        lineParameter = (((intersectionDistance * secondLineDirection.y) + capsuleStart->y) - segmentStart->y) / firstLineDirection.y;
    }
    else {
        lineParameter = (((intersectionDistance * secondLineDirection.z) + capsuleStart->z) - segmentStart->z) / firstLineDirection.z;
    }
    if (lineParameter < 0.0f || lineParameter > 1.0f || intersectionDistance < 0.0f || intersectionDistance > 1.0f) {
        return 0;
    }
    intersectionPoint->x = (lineParameter * firstLineDirection.x) + segmentStart->x;
    intersectionPoint->y = (lineParameter * firstLineDirection.y) + segmentStart->y;
    intersectionPoint->z = (lineParameter * firstLineDirection.z) + segmentStart->z;
    return 1;
}

void PinballEffectStart(s16 model)
{
    Vec faceVertices[4];
    Vec vertexPosition;
    s32 outlineVertexCount;
    s32 uniqueVertexIndex;
    HSFDATA *modelData;
    HSFBUFFER *faceBuffer;
    HSFFACE *face;
    s32 cornerIndex;
    s32 uniqueVertexCount;
    s32 faceIndex;
    s32 faceVertexIndex;
    HSFBUFFER *vertexBuffer;
    modelData = Hu3DData[model].hsf;
    collisionPosition.x = collisionPosition.y = -100000;
    collisionNormal.x = collisionNormal.y = 100000;
    faceBuffer = modelData->face;
    for (faceIndex = outlineVertexCount = 0; faceIndex < modelData->faceNum; faceIndex++, faceBuffer++) {
        face = faceBuffer->data;
        for (faceVertexIndex = 0; faceVertexIndex < faceBuffer->count; faceVertexIndex++, face++) {
            switch (face->type & 0x7) {
                case 0:
                case 1:
                case 2:
                    break;

                case 3:
                    vertexBuffer = modelData->vertex;
                    for (cornerIndex = uniqueVertexCount = 0; cornerIndex < 4; cornerIndex++) {
                        vertexPosition = ((Vec *)vertexBuffer->data)[face->indices[cornerIndex][0]];
                        for (uniqueVertexIndex = 0; uniqueVertexIndex < uniqueVertexCount; uniqueVertexIndex++) {
                            if (vertexPosition.y != faceVertices[uniqueVertexIndex].y) {
                                break;
                            }
                        }
                        if (uniqueVertexIndex == uniqueVertexCount) {
                            faceVertices[uniqueVertexIndex].x = vertexPosition.x;
                            faceVertices[uniqueVertexIndex].y = vertexPosition.y;
                            faceVertices[uniqueVertexIndex].z = vertexPosition.z;
                            uniqueVertexCount++;
                        }
                    }
                    upperBallTrailPositions[outlineVertexCount].x = faceVertices[0].x;
                    upperBallTrailPositions[outlineVertexCount].y = faceVertices[0].z;
                    upperBallTrailPositions[outlineVertexCount].z = 0;
                    if (faceVertices[0].x > collisionPosition.x) {
                        collisionPosition.x = faceVertices[0].x;
                    }
                    if (faceVertices[0].z > collisionPosition.y) {
                        collisionPosition.y = faceVertices[0].z;
                    }
                    if (faceVertices[0].x < collisionNormal.x) {
                        collisionNormal.x = faceVertices[0].x;
                    }
                    if (faceVertices[0].z < collisionNormal.y) {
                        collisionNormal.y = faceVertices[0].z;
                    }
                    lowerBallTrailPositions[outlineVertexCount].x = faceVertices[1].x;
                    lowerBallTrailPositions[outlineVertexCount].y = faceVertices[1].z;
                    lowerBallTrailPositions[outlineVertexCount].z = 0;

                    if (faceVertices[1].x > collisionPosition.x) {
                        collisionPosition.x = faceVertices[1].x;
                    }
                    if (faceVertices[1].z > collisionPosition.y) {
                        collisionPosition.y = faceVertices[1].z;
                    }
                    if (faceVertices[1].x < collisionNormal.x) {
                        collisionNormal.x = faceVertices[1].x;
                    }
                    if (faceVertices[1].z < collisionNormal.y) {
                        collisionNormal.y = faceVertices[1].z;
                    }
                    outlineVertexCount++;
                    break;

                case 4:
                    break;

                default:
                    break;
            }
        }
    }
    ballTrailCount = outlineVertexCount;
}

#include "REL/m424Dll.h"

#include "ext_math.h"
#include "game/audio.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/wipe.h"
#include "version.h"

// BSS
HUPROCESS* m424ObjectManager;
s32 minigameSequenceId;
s32 countdownSequenceId;
s32 m424SceneState;
s32 sceneStateTimer;
s32 unusedSceneCounter; // UNUSED
s32 sequenceTimer;
s32 exitSelection;
s32 backgroundMusicId;
s16 playerOrder[4];
s16 characterOrder[4];
OMOBJ* sceneObject;
OMOBJ* cameraObject;
s32 cameraMode;
s32 cameraState;
f32 unusedCameraValues[2]; // UNUSED
f32 cameraRotation;

// data
M424CameraConfig cameraConfigs[3] = {
    {
        { 320.0f,  0.0f,   0.0f  },
        { 201.0f,  175.0f, 72.0f },
        2025.0f
    },
    {
        { 320.0f,  0.0f,    0.0f  },
        { 0.0f,    225.0f, -72.0f },
        550.0f
    },
    {
        { 320.0f,  0.0f,   0.0f  },
        { 401.0f,  175.0f, 72.0f },
        625.0f
    }
};
Vec shadowLightPosition = { 1000.0f, 2000.0f, 1000.0f };
Vec shadowLightDirection = { 0.0f, 1.0f, 0.0f };
Vec shadowTargetPosition = { 0.0f, 0.0f, -450.0f };

// PROTO
void M424SceneInit(OMOBJ* object);
void M424SceneUpdate(OMOBJ* object);
void M424ExitComplete(OMOBJ* object);
void M424ExitUpdate(OMOBJ* object);
void M424CameraSetup(void);
void M424EmptyObjectCallback(OMOBJ* object);
void M424CameraConfigLerp(s32 sourceConfigIndex, s32 targetConfigIndex, f32 interpolationProgress);
f32 AngleLerpDegrees(f32, f32, f32);

/// FUNCTIONS ///

void ObjectSetup(void) {
    s32 playerIndex;
    HUPROCESS *objectManager;
    HU3DCAMERA *mainCamera;

    OSReport("******* M424ObjectSetup *********\n");
    objectManager = omInitObjMan(0x32, 0x2000);
    m424ObjectManager = objectManager;
    omGameSysInit(objectManager);
    M424CameraSetup();
    mainCamera = &Hu3DCamera[0];
    mainCamera->fov = -1.0f;
    Hu3DLightAllKill();
    Hu3DLighInit();
    HuAudSndGrpSet(0x31);
    sceneObject = omAddObjEx(objectManager, 0xFF, 2, 0, -1, M424SceneInit);
    minigameSequenceId = countdownSequenceId = -1;
    m424SceneState = 0;
    sceneStateTimer = 0;
    sequenceTimer = 0;
    backgroundMusicId = -1;
    
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerOrder[playerIndex] = -1;
        characterOrder[playerIndex] = -1;
    }
    Hu3DShadowCreate(30.0f, 1000.0f, 250000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowPosSet(&shadowLightPosition, &shadowLightDirection, &shadowTargetPosition);
    Hu3DBGColorSet(0xFFU, 0xFFU, 0xFFU);
}

Vec sceneLightPosition = { 0.0f, 0.0f, 0.0f };
Vec sceneLightDirection = { -1.0f, -2.0f, -1.0f };
GXColor sceneLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };

void M424SceneInit(OMOBJ* object) {
    s32 playerIndex;
    s32 aiPlayerCount;
    s32 lightId;

    lightId = Hu3DGLightCreateV(&sceneLightPosition, &sceneLightDirection, &sceneLightColor);
    Hu3DGLightStaticSet(lightId, 1);
    Hu3DGLightInfinitytSet(lightId);
    
    for (playerIndex = 0, aiPlayerCount = 0; playerIndex < 4; playerIndex++) {
        if (GWPlayerCfg[playerIndex].group == 0) {
            playerOrder[3] = playerIndex;
            characterOrder[3] = GWPlayerCfg[playerIndex].character;
        } else {
            playerOrder[aiPlayerCount] = playerIndex;
            characterOrder[aiPlayerCount] = GWPlayerCfg[playerIndex].character;
            aiPlayerCount++;
        }
    }
    if (aiPlayerCount >= 4) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerOrder[playerIndex] = playerIndex;
            characterOrder[playerIndex] = GWPlayerCfg[playerIndex].character;
        }
    }
    MapCameraCreate(m424ObjectManager);
    M424BallObjectsCreate(m424ObjectManager);
    ClawObjectCreate(m424ObjectManager);
    object->objFunc = M424SceneUpdate;
}

void M424SceneUpdate(OMOBJ* object) {
    s16 unusedStackValue16;
    s16 unusedStackValue12;
    s16 unusedStackValueE;
    s16 unusedStackValueA;
    f32 interpolationProgress; // ! - uninitalized
    s32 sequenceStatusFlags;
    s32 countdownSeconds;
    HU3DCAMERA *mainCamera;

    switch (m424SceneState) {
        case 0:
            cameraRotation += MapCameraHeightGet();
#ifdef NON_MATCHING
            interpolationProgress = sequenceTimer / 60.0f;
#endif
            M424CameraConfigLerp(2, 2, interpolationProgress);
            if (cameraRotation >= 360.0f) {
                WipeCreate(1, 0, 0x3C);
                mainCamera = &Hu3DCamera[0];
                mainCamera->fov = 45.0f;
                m424SceneState = 1;
                sceneStateTimer = 0;
            }
            break;
        case 1:
            if (WipeStatGet() == 0) {
                m424SceneState = 2;
                sceneStateTimer = 0;
            }
            break;
        case 2:
            if (++sceneStateTimer >= REFRESH_RATE_F * 2) {
                minigameSequenceId = MGSeqCreate(3, 0);
                m424SceneState = 3;
                sceneStateTimer = 0;
                sequenceTimer = 0;
            }
            break;
        case 3:
            sequenceTimer++;
            interpolationProgress = sequenceTimer / REFRESH_RATE_F;
            if (interpolationProgress > 1.0f) {
                interpolationProgress = 1.0f;
            }
            interpolationProgress = sind((90.0f * (interpolationProgress * interpolationProgress)));
            M424CameraConfigLerp(2, 0, interpolationProgress);
            if (minigameSequenceId != -1) {
                sequenceStatusFlags = MGSeqStatGet(minigameSequenceId);
                if (((sequenceStatusFlags & 0x10) != 0) && (backgroundMusicId == -1)) {
                    backgroundMusicId = HuAudSeqPlay(0x48);
                }
            } else {
                sequenceStatusFlags = 0;
            }
            if ((minigameSequenceId == -1) || (sequenceStatusFlags == 0)) {
                if (sequenceTimer >= REFRESH_RATE_F) {
                    minigameSequenceId = -1;
                    m424SceneState = 4;
                    sceneStateTimer = 0;
                    sequenceTimer = REFRESH_RATE_F * 60;
                    countdownSequenceId = MGSeqCreate(1, sequenceTimer / REFRESH_RATE, -1, -1);
                }
            }
            break;
        case 4:
            countdownSeconds = (sequenceTimer + REFRESH_RATE - 1) / REFRESH_RATE;
            if (countdownSeconds < 0) {
                countdownSeconds = 0;
            }
            MGSeqParamSet(countdownSequenceId, 1, countdownSeconds);
            if ((--sequenceTimer < -1) || (ClawPlayerThresholdReached() != 0)) {
                MGSeqParamSet(countdownSequenceId, 2, -1);
                countdownSequenceId = -1;
                minigameSequenceId = MGSeqCreate(3, 1);
                if (backgroundMusicId != -1) {
                    HuAudSeqFadeOut(backgroundMusicId, 0x64);
                }
                m424SceneState = 5;
                sceneStateTimer = 0;
            }
            break;
        case 5:
            if ((minigameSequenceId == -1) || ((MGSeqStatGet(minigameSequenceId) == 0) && (ClawMotionPhase10Reached() != 0) && (ActiveBallExists() != 0) && (MapCameraHeightGet() <= 0.0f))) {
                if (ClawPlayerThresholdReached() != 0) {
                    exitSelection = 0;
                } else {
                    exitSelection = 1;
                }
                HuAudAllStop();
                m424SceneState = 6;
                sceneStateTimer = 0;
            }
            break;
        case 6:
            if (exitSelection == 0) {
                if (ClawMotionPhase13Reached() != 0) {
                    m424SceneState = 7;
                    sceneStateTimer = 0;
                }
            } else {
                if (++sceneStateTimer >= REFRESH_RATE_F * 2) {
                    m424SceneState = 7;
                    sceneStateTimer = 0;
                }
            }
            break;
        case 7:
            if (exitSelection != 0) {
                minigameSequenceId = MGSeqCreate(5, 3, characterOrder[0], characterOrder[1], characterOrder[2], -1);
                GWPlayerCoinWinSet(playerOrder[0], GWPlayerCoinWinGet(playerOrder[0]) + 10);
                GWPlayerCoinWinSet(playerOrder[1], GWPlayerCoinWinGet(playerOrder[1]) + 10);
                GWPlayerCoinWinSet(playerOrder[2], GWPlayerCoinWinGet(playerOrder[2]) + 10);
                HuAudSStreamPlay(1);
                m424SceneState = 8;
                sequenceTimer = 0;
            } else {
                minigameSequenceId = MGSeqCreate(5, 3, characterOrder[3], -1, -1, -1);
                GWPlayerCoinWinSet(playerOrder[3], GWPlayerCoinWinGet(playerOrder[3]) + 10);
                HuAudSStreamPlay(1);
                m424SceneState = 8;
                sequenceTimer = 0;
            }
            break;
        case 8:
            if (exitSelection == 0) {
                interpolationProgress = sequenceTimer / REFRESH_RATE_F;
                if (interpolationProgress > 1.0f) {
                    interpolationProgress = 1.0f;
                }
                interpolationProgress = sind(90.0f * (interpolationProgress * interpolationProgress));
                M424CameraConfigLerp(0, 1, interpolationProgress);
            }
            if (++sequenceTimer >= REFRESH_RATE_F * 3.5f) {
                minigameSequenceId = -1;
                m424SceneState = 9;
                sequenceTimer = 0;
            }
            break;
        case 9:
            WipeCreate(2, 0, 0x3C);
            object->objFunc = M424ExitComplete;
            break;
    }
    M424ExitUpdate(object);
}

void M424ExitComplete(OMOBJ* object) {
    if (WipeStatGet() == 0) {
        Hu3DBGColorSet(0, 0, 0);
        MapCameraDestroy();
        M424BallObjectsDestroy();
        ClawObjectDestroy();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void M424ExitUpdate(OMOBJ* object) {
    s32 exitRequested;

    exitRequested = 0;
    if ((WipeStatGet() != 0) || (object->objFunc == M424ExitComplete)) {
        return;
    }
    if (omSysExitReq != 0) {
        exitRequested = 1;
    }
    if (exitRequested != 0) {
        WipeCreate(2, 0, 0x3C);
        HuAudStreamFadeOut(0x1E);
        object->objFunc = M424ExitComplete;
    }
}

s32 M424SceneStateGet(void) {
    return m424SceneState;
}

void M424CameraSetup(void) {
    OMOBJ *mainCameraObject;

    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 45.0f, 50.0f, 50000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 0x280, 0x1E0);
    mainCameraObject = cameraObject = omAddObjEx(m424ObjectManager, 0x7FDA, 0, 0, -1, omOutView);
    mainCameraObject->work[0] = 1;
    CRot.x = cameraConfigs[0].rot.x;
    CRot.y = cameraConfigs[0].rot.y;
    CRot.z = cameraConfigs[0].rot.z;
    Center.x = cameraConfigs[0].center.x;
    Center.y = cameraConfigs[0].center.y;
    Center.z = cameraConfigs[0].center.z;
    CZoom = cameraConfigs[0].zoom;
    cameraState = 0;
    cameraMode = -1;
    omAddObjEx(m424ObjectManager, 0xFF, 0, 0, -1, M424EmptyObjectCallback);
}

void M424EmptyObjectCallback(OMOBJ* object) {}
void M424EmptyCameraCallback(void) {}

s32 M424CameraStateGet(void) {
    return cameraState;
}

void M424CameraConfigLerp(s32 sourceConfigIndex, s32 targetConfigIndex, f32 interpolationProgress) {
    M424CameraConfig *sourceConfig;
    M424CameraConfig *targetConfig;

    sourceConfig = &cameraConfigs[sourceConfigIndex];
    targetConfig = &cameraConfigs[targetConfigIndex];
    CRot.x = AngleLerpDegrees(interpolationProgress, sourceConfig->rot.x, targetConfig->rot.x);
    CRot.y = AngleLerpDegrees(interpolationProgress, sourceConfig->rot.y, targetConfig->rot.y);
    CRot.z = AngleLerpDegrees(interpolationProgress, sourceConfig->rot.z, targetConfig->rot.z);
    Center.x = sourceConfig->center.x + (interpolationProgress * (targetConfig->center.x - sourceConfig->center.x));
    Center.y = sourceConfig->center.y + (interpolationProgress * (targetConfig->center.y - sourceConfig->center.y));
    Center.z = sourceConfig->center.z + (interpolationProgress * (targetConfig->center.z - sourceConfig->center.z));
    CZoom = sourceConfig->zoom + (interpolationProgress * (targetConfig->zoom - sourceConfig->zoom));
}

f32 AngleDeltaDegrees(f32 firstAngle, f32 secondAngle) {
    f32 angleDifference;

    if (firstAngle >= 360.0f) {
        firstAngle -= 360.0f;
    } else if (firstAngle < 0.0f) {
        firstAngle += 360.0f;
    }
    if (secondAngle >= 360.0f) {
        secondAngle -= 360.0f;
    } else if (secondAngle < 0.0f) {
        secondAngle += 360.0f;
    }
    angleDifference = firstAngle - secondAngle;
    if (angleDifference <= -180.0f) {
        angleDifference += 360.0f;
    } else {
        if (angleDifference >= 180.0f) {
            angleDifference -= 360.0f;
        }
    }
    return angleDifference;
}

f32 AngleApproachDegrees(f32 currentAngle, f32 targetAngle, f32 maximumStep) {
    f32 clockwiseDistance;
    f32 adjustedAngle;

    if (currentAngle >= 360.0) {
        currentAngle -= 360.0;
    } else if (currentAngle < 0.0) {
        currentAngle += 360.0;
    }
    if (targetAngle >= 360.0) {
        targetAngle -= 360.0;
    } else if (targetAngle < 0.0) {
        targetAngle += 360.0;
    }
    clockwiseDistance = (360.0 + (currentAngle - targetAngle));
    if (fabs(clockwiseDistance) >= 360.0) {
        clockwiseDistance = fmod(clockwiseDistance, 360.0);
    }
    if (clockwiseDistance < 180.0) {
        if (clockwiseDistance <= maximumStep) {
            adjustedAngle = clockwiseDistance;
        } else {
            adjustedAngle = maximumStep;
        }
    } else {
        if ((360.0 - clockwiseDistance) <= maximumStep) {
            adjustedAngle = -(360.0 - clockwiseDistance);
        } else {
            adjustedAngle = -maximumStep;
        }
    }
    adjustedAngle = adjustedAngle + targetAngle;
    if (adjustedAngle >= 360.0) {
        adjustedAngle -= 360.0;
    } else if (adjustedAngle < 0.0) {
        adjustedAngle += 360.0;
    }
    return adjustedAngle;
}

f32 AngleLerpDegrees(f32 interpolationProgress, f32 startAngle, f32 endAngle) {
    f32 maximumStep;
    f32 angleDifference;

    angleDifference = AngleDeltaDegrees(endAngle, startAngle);
    maximumStep = fabs(angleDifference * interpolationProgress);
    return AngleApproachDegrees(endAngle, startAngle, maximumStep);
}

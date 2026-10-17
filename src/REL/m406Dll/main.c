
#include "ext_math.h"
#include "game/audio.h"
#include "game/disp.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/wipe.h"

#include "REL/m406Dll.h"

typedef struct M406CameraParameters {
    Float initialRotationX;
    Float initialRotationY;
    Float initialRotationZ;
    Float unusedCameraValue0C;
    Float unusedCameraValue10;
    Float unusedCameraValue14;
    Float unusedCameraValue18;
    Float transitionRotationStartX;
    Float transitionRotationStartY;
    Float transitionRotationStartZ;
    Float transitionCenterStartX;
    Float transitionCenterStartY;
    Float transitionCenterStartZ;
    Float transitionZoomStart;
    Float initialViewRotationX;
    Float initialViewRotationY;
    Float initialViewRotationZ;
    Float initialViewCenterX;
    Float initialViewCenterY;
    Float initialViewCenterZ;
    Float initialViewZoom;
    Float endingViewRotationX;
    Float endingViewRotationY;
    Float endingViewRotationZ;
    Float endingViewCenterX;
    Float endingViewCenterY;
    Float endingViewCenterZ;
    Float endingViewZoom;
    Float transitionRotationEndX;
    Float transitionRotationEndY;
    Float transitionRotationEndZ;
    Float transitionCenterEndX;
    Float transitionCenterEndY;
    Float transitionCenterEndZ;
    Float transitionZoomEnd;
} M406CameraParameters; /* size = unknown */

void StartSceneTransition(OMOBJ *object);
void UpdateSceneExit(OMOBJ *object);
void InitializeSceneCamera(void);
void InitializeSceneView(OMOBJ *object);
void UpdateIntroCamera(Float scale);
void UpdateTransitionCamera(Float scale);
Float InterpolateAngle(Float rotationSpeed, Float secondScale, Float thirdScale);
void InitializeSceneObjects(OMOBJ *object);
void UpdateSceneState(OMOBJ *object);

M406CameraParameters cameraParameters = {
    330.0f,
    105.0f,
    0.0f,
    0.0f,
    100.0f,
    0.0f,
    1650.0f,
    91.0f,
    124.0f,
    0.0f,
    0.0f,
    -35440.0f,
    -60022.0f,
    1226.0f,
    0.0f,
    180.0f,
    0.0f,
    0.0f,
    -1300.0f,
    -2000.0f,
    1000.0f,
    -5.0f,
    0.0f,
    0.0f,
    31.0f,
    75.0f,
    -250.0f,
    675.0f,
    111.0f,
    164.0f,
    0.0f,
    0.0f,
    -35440.0f,
    -60022.0f,
    1226.0f,
};

HUPROCESS *objectProcess;
OMOBJ *mainObject;
s32 minigameSequenceId;
s32 resultsSequenceId;
s32 sceneState;
s32 sceneStateFrames;
s32 cameraTransitionFrames;
s32 selectedPlayer;
s32 audioSequenceId;
s32 sceneTransitionActive;
OMOBJ *cameraObject;
s32 sceneCompleted;

void ObjectSetup(void)
{
    Vec cameraPosition;
    Vec cameraDirection;

    HUPROCESS *objectManager;

    OSReport("******* M426ObjectSetup *********\n");
    objectManager = omInitObjMan(50, 8192);
    objectProcess = objectManager;
    omGameSysInit(objectManager);
    InitializeSceneCamera();
    HuAudSndGrpSet(0x1F);
    cameraPosition.x = Center.x + (CZoom * (sind(CRot.y) * cosd(CRot.x)));
    cameraPosition.y = Center.y + (CZoom * -sind(CRot.x));
    cameraPosition.z = Center.z + (CZoom * (cosd(CRot.y) * cosd(CRot.x)));
    VECSubtract(&Center, &cameraPosition, &cameraDirection);
    VECNormalize(&cameraDirection, &cameraDirection);
    HuAudFXListnerSetEX(&cameraPosition, &cameraDirection, 100000.0f, (34000.0f/REFRESH_RATE)+3600, 0.0f, 300.0f, 300.0f);
    Hu3DLightAllKill();
    Hu3DLighInit();
    mainObject = omAddObjEx(objectManager, 255, 2, 0, -1, InitializeSceneObjects);
    minigameSequenceId = resultsSequenceId = -1;
    sceneState = 0;
    sceneStateFrames = 0;
    selectedPlayer = -1;
    audioSequenceId = -1;
    sceneTransitionActive = 0;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    sceneTransitionActive = 0;
}

void InitializeSceneObjects(OMOBJ *object)
{
    CreateMapScene(objectProcess);
    InitializePlayerSystem(objectProcess);
    object->objFunc = UpdateSceneState;
}

void UpdateSceneState(OMOBJ *object)
{
    Vec cameraPosition;
    Vec cameraDirection;

    Float transitionProgress;

    u8 sequenceStatus;

    switch (sceneState) {
        case 0:
            if (!WipeStatGet()) {
                sceneState = 1;
                sceneStateFrames = 0;
            }
            break;
        case 1:
            sceneStateFrames++;
            if (sceneStateFrames >= (REFRESH_RATE*0.8f)) {
                SetMapPathSpeed((1800.0001f/REFRESH_RATE));
            }
            break;
        case 2:
            sceneStateFrames++;
            transitionProgress = sceneStateFrames / (REFRESH_RATE*0.75f);
            UpdateIntroCamera(transitionProgress);
            if (sceneStateFrames >= (REFRESH_RATE*0.75f)) {
                minigameSequenceId = MGSeqCreate(3, 0);
                sceneState = 3;
                sceneStateFrames = 0;
            }
            break;
        case 3:
            if (minigameSequenceId != -1) {
                sequenceStatus = MGSeqStatGet(minigameSequenceId);
                if (sequenceStatus & 16 && audioSequenceId == -1) {
                    audioSequenceId = HuAudSeqPlay(74);
                }
            }
            else {
                sequenceStatus = 0;
            }
            if (minigameSequenceId == -1 || !sequenceStatus) {
                minigameSequenceId = -1;
                sceneState = 4;
                sceneStateFrames = 0;
            }
            break;
        case 4:
            if (selectedPlayer != -1) {
                minigameSequenceId = MGSeqCreate(3, 1);
                HuAudFXAllStop();
                if (audioSequenceId != -1) {
                    HuAudSeqFadeOut(audioSequenceId, 100);
                }
                if (selectedPlayer != 99) {
                    CreateSceneLight();
                    SetAnimationGroupAlpha(1.0f);
                    StopMapPathAudio();
                    UpdateTransitionCamera(0.0f);
                    Hu3DBGColorSet(0x40, 0x40, 0xFF);
                    sceneState = 5;
                    sceneStateFrames = 0;
                }
                else {
                    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, REFRESH_RATE/2);
                    #if VERSION_NTSC
                    WipeColorSet(255, 255, 255);
                    #else
                    WipeColorSet(160, 160, 160);
                    #endif
                    sceneTransitionActive = 1;
                    sceneState = 9;
                    sceneStateFrames = 0;
                    cameraTransitionFrames = 0;
                }
            }
            break;
        case 5:
            cameraTransitionFrames++;
            transitionProgress = cameraTransitionFrames / (REFRESH_RATE*1.5f);
            if (transitionProgress > 1.0f) {
                transitionProgress = 1.0f;
            }
            UpdateTransitionCamera(transitionProgress);
            Hu3DBGColorSet(0x40, 0x40, 0xFF);
            break;
        case 6:
            cameraTransitionFrames++;
            transitionProgress = cameraTransitionFrames / (REFRESH_RATE*1.5f);
            if (transitionProgress > 1.0f) {
                transitionProgress = 1.0f;
            }
            UpdateTransitionCamera(transitionProgress);
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, REFRESH_RATE/2);
            #if VERSION_NTSC
            WipeColorSet(255, 255, 255);
            #else
            WipeColorSet(160, 160, 160);
            #endif
            sceneTransitionActive = 1;
            sceneState = 7;
            sceneStateFrames = 0;
            break;
        case 7:
            cameraTransitionFrames++;
            transitionProgress = cameraTransitionFrames / (REFRESH_RATE*1.5f);
            if (transitionProgress > 1.0f) {
                transitionProgress = 1.0f;
            }
            UpdateTransitionCamera(transitionProgress);
            if (!WipeStatGet() && (minigameSequenceId == -1 || !MGSeqStatGet(minigameSequenceId))) {
                minigameSequenceId = -1;
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, REFRESH_RATE/2);
                #if VERSION_NTSC
                WipeColorSet(255, 255, 255);
                #else
                WipeColorSet(160, 160, 160);
                #endif
                sceneTransitionActive = 0;
                CRot.x = cameraParameters.endingViewRotationX;
                CRot.y = cameraParameters.endingViewRotationY;
                CRot.z = cameraParameters.endingViewRotationZ;
                Center.x = cameraParameters.endingViewCenterX;
                Center.y = cameraParameters.endingViewCenterY;
                Center.z = cameraParameters.endingViewCenterZ;
                CZoom = cameraParameters.endingViewZoom;
                SetMapState(1);
                sceneState = 8;
                sceneStateFrames = 0;
            }
            break;
        case 8:
            if (!WipeStatGet()) {
                sceneState = 11;
                sceneStateFrames = 0;
            }
            break;
        case 9:
            if (!WipeStatGet() && (minigameSequenceId == -1 || !MGSeqStatGet(minigameSequenceId))) {
                minigameSequenceId = -1;
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, REFRESH_RATE/2);
                #if VERSION_NTSC
                WipeColorSet(255, 255, 255);
                #else
                WipeColorSet(160, 160, 160);
                #endif
                sceneTransitionActive = 0;
                CRot.x = cameraParameters.endingViewRotationX;
                CRot.y = cameraParameters.endingViewRotationY;
                CRot.z = cameraParameters.endingViewRotationZ;
                Center.x = cameraParameters.endingViewCenterX;
                Center.y = cameraParameters.endingViewCenterY;
                Center.z = cameraParameters.endingViewCenterZ;
                CZoom = cameraParameters.endingViewZoom;
                cameraPosition.x = Center.x + (CZoom * (sind(CRot.y) * cosd(CRot.x)));
                cameraPosition.y = Center.y + (CZoom * -sind(CRot.x));
                cameraPosition.z = Center.z + (CZoom * (cosd(CRot.y) * cosd(CRot.x)));
                VECSubtract(&Center, &cameraPosition, &cameraDirection);
                VECNormalize(&cameraDirection, &cameraDirection);
                HuAudFXListnerUpdate(&cameraPosition, &cameraDirection);
                StopMapPathAudio();
                SetMapState(1);
                sceneState = 10;
                sceneStateFrames = 0;
            }
            break;
        case 10:
            if (!WipeStatGet()) {
                sceneState = 11;
                sceneStateFrames = 0;
            }
            break;
        case 11:
            break;
        case 12:
            if (selectedPlayer != 99) {
                minigameSequenceId = MGSeqCreate(5, 3, GWPlayerCfg[selectedPlayer].character, -1, -1, -1);
                HuAudSStreamPlay(1);
                GWPlayerCoinWinAdd(selectedPlayer, 10);
                sceneState = 13;
                sceneStateFrames = 0;
            }
            else {
                minigameSequenceId = MGSeqCreate(3, 2);
                HuAudSStreamPlay(4);
                sceneState = 13;
                sceneStateFrames = 0;
            }
            break;
        case 13:
            if (++sceneStateFrames >= (REFRESH_RATE*3.5f)) {
                minigameSequenceId = -1;
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                sceneTransitionActive = 1;
                sceneState = 14;
                sceneStateFrames = 0;
            }
            break;
        case 14:
            if (!WipeStatGet()) {
                Hu3DBGColorSet(0, 0, 0);
                sceneState = 15;
                sceneStateFrames = 0;
            }
            break;
        case 15:
        default:
            object->objFunc = StartSceneTransition;
            break;
    }
    UpdateSceneExit(object);
}

void StartSceneTransition(OMOBJ *object)
{
    if (!WipeStatGet()) {
        CreatePlayerObjects();
        InitializeMapEffects();
        DestroySceneLight();
        HuAudFXListnerKill();
        HuDataDirClose(DATA_MAKE_NUM(DATADIR_M406, 0));
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void UpdateSceneExit(OMOBJ *object)
{
    s32 exitRequested;

    exitRequested = 0;
    if (WipeStatGet() || object->objFunc == StartSceneTransition) {
        return;
    }
    if (omSysExitReq) {
        exitRequested = 1;
    }
    if (exitRequested) {
        if (!sceneTransitionActive) {
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        }
        sceneTransitionActive = 1;
        HuAudStreamFadeOut(30);
        object->objFunc = StartSceneTransition;
    }
}

s32 GetSceneState(void)
{
    return sceneState;
}

void BeginCameraTransition(void)
{
    if (sceneState <= 1) {
        sceneState = 2;
        sceneStateFrames = 0;
    }
}

void SetSelectedPlayer(s32 source)
{
    selectedPlayer = source;
}

void AdvanceCameraTransition(void)
{
    if (sceneState == 5) {
        sceneState = 6;
        sceneStateFrames = 0;
    }
}

void CompleteSceneTransition(void)
{
    if (selectedPlayer != 99) {
        if (sceneState == 11) {
            sceneState = 12;
            sceneStateFrames = 0;
        }
    }
    else if (sceneState == 11) {
        sceneState = 12;
        sceneStateFrames = 0;
    }
}

void InitializeSceneCamera(void)
{
    OMOBJ *cameraObject;

    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 45.0f, 100.0f, 50000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 640, 480);
    cameraObject = cameraObject = omAddObjEx(objectProcess, 0x7FDA, 0, 0, -1, omOutView);
    CRot.x = cameraParameters.initialViewRotationX;
    CRot.y = cameraParameters.initialViewRotationY;
    CRot.z = cameraParameters.initialViewRotationZ;
    Center.x = cameraParameters.initialViewCenterX;
    Center.y = cameraParameters.initialViewCenterY;
    Center.z = cameraParameters.initialViewCenterZ;
    CZoom = cameraParameters.initialViewZoom;
    sceneCompleted = 0;
    omAddObjEx(objectProcess, 0xFF, 0, 0, -1, InitializeSceneView);
}

void InitializeSceneView(OMOBJ *object) { }

void EmptySceneCallback(void) { }

void RestoreInitialSceneView(void)
{
    CRot.x = cameraParameters.initialViewRotationX;
    CRot.y = cameraParameters.initialViewRotationY;
    CRot.z = cameraParameters.initialViewRotationZ;
    Center.x = cameraParameters.initialViewCenterX;
    Center.y = cameraParameters.initialViewCenterY;
    Center.z = cameraParameters.initialViewCenterZ;
    CZoom = cameraParameters.initialViewZoom;
}

void UpdateIntroCamera(Float scale)
{
    Vec cameraPosition;
    Vec cameraDirection;

    CRot.x = InterpolateAngle(scale, cameraParameters.initialViewRotationX, cameraParameters.initialRotationX);
    CRot.y = InterpolateAngle(scale, cameraParameters.initialViewRotationY, cameraParameters.initialRotationY);
    CRot.z = InterpolateAngle(scale, cameraParameters.initialViewRotationZ, cameraParameters.initialRotationZ);
    cameraPosition.x = Center.x + (CZoom * (sind(CRot.y) * cosd(CRot.x)));
    cameraPosition.y = Center.y + (CZoom * -sind(CRot.x));
    cameraPosition.z = Center.z + (CZoom * (cosd(CRot.y) * cosd(CRot.x)));
    VECSubtract(&Center, &cameraPosition, &cameraDirection);
    VECNormalize(&cameraDirection, &cameraDirection);
    HuAudFXListnerUpdate(&cameraPosition, &cameraDirection);
}

void UpdateTransitionCamera(Float scale)
{
    Vec cameraPosition;
    Vec cameraDirection;

    CRot.x = InterpolateAngle(scale, cameraParameters.transitionRotationStartX, cameraParameters.transitionRotationEndX);
    CRot.y = InterpolateAngle(scale, cameraParameters.transitionRotationStartY, cameraParameters.transitionRotationEndY);
    CRot.z = InterpolateAngle(scale, cameraParameters.transitionRotationStartZ, cameraParameters.transitionRotationEndZ);
    Center.x = cameraParameters.transitionCenterStartX + (scale * (cameraParameters.transitionCenterEndX - cameraParameters.transitionCenterStartX));
    Center.y = cameraParameters.transitionCenterStartY + (scale * (cameraParameters.transitionCenterEndY - cameraParameters.transitionCenterStartY));
    Center.z = cameraParameters.transitionCenterStartZ + (scale * (cameraParameters.transitionCenterEndZ - cameraParameters.transitionCenterStartZ));
    CZoom = cameraParameters.transitionZoomStart + (scale * (cameraParameters.transitionZoomEnd - cameraParameters.transitionZoomStart));
    cameraPosition.x = Center.x + (CZoom * (sind(CRot.y) * cosd(CRot.x)));
    cameraPosition.y = Center.y + (CZoom * -sind(CRot.x));
    cameraPosition.z = Center.z + (CZoom * (cosd(CRot.y) * cosd(CRot.x)));
    VECSubtract(&Center, &cameraPosition, &cameraDirection);
    VECNormalize(&cameraDirection, &cameraDirection);
    HuAudFXListnerUpdate(&cameraPosition, &cameraDirection);
}

s32 IsSceneComplete(void)
{
    return sceneCompleted;
}

Float ApproachAngleValue(Float scale, Float rotationSpeed)
{
    Float angleDifference;

    if (scale >= 360.0f) {
        scale -= 360.0f;
    }
    else if (scale < 0.0f) {
        scale += 360.0f;
    }
    if (rotationSpeed >= 360.0f) {
        rotationSpeed -= 360.0f;
    }
    else if (rotationSpeed < 0.0f) {
        rotationSpeed += 360.0f;
    }
    angleDifference = scale - rotationSpeed;
    if (angleDifference <= -180.0f) {
        angleDifference += 360.0f;
    }
    else if (angleDifference >= 180.0f) {
        angleDifference -= 360.0f;
    }
    return angleDifference;
}

Float NormalizeAngleDifference(Float scale, Float rotationSpeed, Float secondScale)
{
    Float adjustedAngle;
    Float angleDifference;

    if (scale >= 360.0) {
        scale -= 360.0;
    }
    else if (scale < 0.0) {
        scale += 360.0;
    }
    if (rotationSpeed >= 360.0) {
        rotationSpeed -= 360.0;
    }
    else if (rotationSpeed < 0.0) {
        rotationSpeed += 360.0;
    }
    angleDifference = 360.0 + (scale - rotationSpeed);
    if (fabs(angleDifference) >= 360.0) {
        angleDifference = fmod(angleDifference, 360.0);
    }
    if (angleDifference < 180.0) {
        if (angleDifference <= secondScale) {
            adjustedAngle = angleDifference;
        }
        else {
            adjustedAngle = secondScale;
        }
    }
    else if (360.0 - angleDifference <= secondScale) {
        adjustedAngle = -(360.0 - angleDifference);
    }
    else {
        adjustedAngle = -secondScale;
    }
    adjustedAngle += rotationSpeed;
    if (adjustedAngle >= 360.0) {
        adjustedAngle -= 360.0;
    }
    else if (adjustedAngle < 0.0) {
        adjustedAngle += 360.0;
    }
    return adjustedAngle;
}

Float InterpolateAngle(Float scale, Float rotationSpeed, Float secondScale)
{
    Float maximumAngleStep;
    Float angleDifference;

    angleDifference = ApproachAngleValue(secondScale, rotationSpeed);
    maximumAngleStep = fabs(angleDifference * scale);
    return NormalizeAngleDifference(secondScale, rotationSpeed, maximumAngleStep);
}

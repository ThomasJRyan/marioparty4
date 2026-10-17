#include "REL/m435Dll.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/esprite.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include <string.h>

#include "ext_math.h"
#include "version.h"

s32 rand8(void);
#ifndef __MWERKS__
#include "game/audio.h"
#endif

typedef struct {
    /* 0x00 */ OMOBJ* characterObject;
    /* 0x04 */ s32 modelVisibilityState;
    /* 0x08 */ s32 materialFadeState;
    /* 0x0C */ s32 aiRouteIndex;
    /* 0x10 */ s32 aiActionState;
    /* 0x14 */ s32 playerIntroAnimationFrames;
    /* 0x18 */ s32 unreferencedPlayerState18;
    /* 0x1C */ s32 routeDelayFrames;
    /* 0x20 */ s32 routeFrame;
    /* 0x24 */ s32 playerIndex;
    /* 0x28 */ s32 playerGroup;
    /* 0x2C */ s32 isCpu;
    /* 0x30 */ s32 aiDifficulty;
    /* 0x34 */ s32 characterId;
    /* 0x38 */ s32 controllerIndex;
    /* 0x3C */ float materialOpacity;
    /* 0x40 */ float unreferencedPlayerState40;
    /* 0x44 */ float unreferencedPlayerState44;
    /* 0x48 */ float unreferencedPlayerState48;
    /* 0x4C */ float unreferencedPlayerState4C;
    /* 0x50 */ float unreferencedPlayerState50;
    /* 0x54 */ float unreferencedPlayerState54;
    /* 0x58 */ MotionTargetState turnTracker;
} PlayerCharacterState; // Size 0x4D8

typedef struct {
    /* 0x00 */ OMOBJ* mainObject;
    /* 0x04 */ s32 mainAnimationState;
    /* 0x08 */ s32 unreferencedAnimationState08;
    /* 0x0C */ s32 unreferencedAnimationState0C;
    /* 0x10 */ s32 unreferencedAnimationState10;
    /* 0x14 */ s32 idleAnimationFrames;
    /* 0x18 */ s32 unreferencedAnimationState18;
    /* 0x1C */ s32 motionEventState;
    /* 0x20 */ s32 unreferencedAnimationState20;
} GameAnimationState; // Size 0x24

typedef struct {
    /* 0x000 */ OMOBJ* mapObject;
    /* 0x004 */ s32 entranceAnimationState;
    /* 0x008 */ s32 environmentAnimationState;
    /* 0x00C */ s32 unreferencedMapState0C;
    /* 0x010 */ s32 environmentAnimationSubstate;
    /* 0x014 */ s32 fallDurationFrames[3];
    /* 0x020 */ s32 fallAnimationFrame;
    /* 0x024 */ s32 targetScore;
    /* 0x028 */ s32 pieceSettlingState;
    /* 0x02C */ s32 pieceState[3];
    /* 0x038 */ s32 pieceFrame[3];
    /* 0x044 */ s32 pieceMovementFrame[3];
    /* 0x050 */ s32 pieceMovementDuration[3];
    /* 0x05C */ s32 pieceLandingResult[3];
    /* 0x068 */ s32 pieceCollisionResult[3];
    /* 0x074 */ float wheelAngle;
    /* 0x078 */ float wheelSpeed;
    /* 0x07C */ float pieceDistance[3];
    /* 0x088 */ float pieceRadius[3];
    /* 0x094 */ float pieceAngle[3];
    /* 0x0A0 */ float alternatePieceAngle[3];
    /* 0x0AC */ Vec pieceStartPosition[3];
    /* 0x0D0 */ Vec piecePosition[3];
    /* 0x0F4 */ Vec pieceTargetPosition[3];
    /* 0x118 */ Vec pieceRotation[3];
    /* 0x13C */ Vec fallStartPosition[3];
    /* 0x160 */ Vec fallTargetPosition[3];
    /* 0x184 */ Vec fallRotation[3];
    /* 0x1A8 */ float pieceOffset[3];
} WheelMapState; // Size 0x1B4

typedef struct {
    /* 0x00 */ s32 ambientSoundId;
    /* 0x04 */ s32 wheelSoundId;
    /* 0x08 */ char unreferencedSoundState08[0xC];
    /* 0x14 */ s32 wheelSoundActive;
    /* 0x18 */ char unreferencedSoundPadding[0x20];
} AmbientSoundState; // Size 0x38

typedef struct {
    /* 0x00 */ s32 scoreCollectionState;
    /* 0x04 */ s32 roundScore;
    /* 0x08 */ s32 roundHasNegativeScore;
    /* 0x0C */ s32 roundWrapUpState;
    /* 0x10 */ s32 currentRoundIndex;
    /* 0x14 */ s32 activeRoundCount;
    /* 0x18 */ s32 unreferencedRoundState18;
    /* 0x1C */ s32 unreferencedRoundState1C;
} RoundScoreState; // Size 0x20

typedef struct {
    /* 0x00 */ OMOBJ* uiObject;
    /* 0x04 */ s32 unreferencedUiState04;
    /* 0x08 */ s32 unreferencedUiState08;
    /* 0x0C */ s32 unreferencedUiState0C;
    /* 0x10 */ s32 unreferencedUiState10;
} GameUiState; // Size 0x14

typedef struct {
    /* 0x000 */ OMOBJ* playerObject;
    /* 0x004 */ Vec playerStartPosition;
    /* 0x010 */ Vec playerTargetPosition;
    /* 0x01C */ s32 playerState;
    /* 0x020 */ s32 playerFrame;
    /* 0x024 */ s32 playerAction;
    /* 0x028 */ s32 playerSubstate;
    /* 0x02C */ s32 playerTimer;
    /* 0x030 */ s32 playerTargetIndex;
    /* 0x034 */ s32 playerMovementState;
    /* 0x038 */ s32 playerActionFrame;
    /* 0x03C */ MotionTargetState turnTracker;
} PlayerActorState; // Size 0x4BC

typedef struct {
    /* 0x00 */ OMOBJ* spriteObject;
    /* 0x04 */ s32 scoreSprites[6];
    /* 0x1C */ char unreferencedSpritePadding[0x10];
    /* 0x2C */ s32 displayState;
    /* 0x30 */ s32 displayFrame;
    /* 0x34 */ s32 spriteState;
    /* 0x38 */ s32 spriteTimer;
    /* 0x3C */ s32 scoreInterpolationFrame;
    /* 0x40 */ s32 unreferencedSpriteState40;
    /* 0x44 */ s32 unreferencedSpriteState44;
    /* 0x48 */ s32 unreferencedSpriteState48;
    /* 0x4C */ float opacity;
    /* 0x50 */ s32 scoreValue;
    /* 0x54 */ s32 previousScoreValue;
} ScoreDisplayState; // Size 0x58

typedef struct {
    /* 0x00 */ OMOBJ* playerObject;
    /* 0x04 */ s32 audioState;
    /* 0x08 */ float playerVolume[6];
    /* 0x20 */ char unreferencedAudioPadding20[8];
    /* 0x28 */ float effectVolume[6];
    /* 0x40 */ char unreferencedAudioPadding40[8];
    /* 0x48 */ float ambientVolume[6];
    /* 0x60 */ char unreferencedAudioPadding60[8];
    /* 0x68 */ float sequenceVolume[6];
    /* 0x80 */ char unreferencedAudioPadding80[8];
    /* 0x88 */ float musicVolume[6];
    /* 0xA0 */ char unreferencedAudioPaddingA0[8];
} AudioVolumeState; // Size 0xA8

void SetWheelEntranceState(s32 entranceState);
void SetWheelEnvironmentSubstate(s32 substate);
void SetPrimaryPlayerFrame(s32 frame);
void SetPrimaryPlayerMotion(s32 motionIndex);
s32 GetGamePlayerIndex(s32 localPlayerIndex);
s32 GetPlayerCharacterId(s32 playerIndex);

AmbientSoundState ambientSoundState;
s32 sequenceSoundIds[2];
RoundScoreState roundScoreState;
char unusedBssPadding[0x2D00]; // unused
PlayerCharacterState playerCharacterStates[4];
TransformTween cameraState;
GameAnimationState gameAnimationState;
WheelMapState wheelMapState;
GameUiState gameUiState;
PlayerActorState playerActorStates[2];
ScoreDisplayState scoreDisplayStates[4];
AudioVolumeState audioVolumeState;
SequencePlayer gameSequencePlayer;
SequencePlayer roundSequencePlayer;
Vec playerStartingPositions[4];
SequencePlayer characterIntroSequencePlayer;
s32 minigameSequenceId;
s32 mainGameState;
s32 unusedState18; // unused
s32 activePlayerIndex;
s32 roundWinnerIndex;
s32 roundWinnerScore;
s32 randomizedPlayerIndex;
s32 gameEventState;
HUPROCESS* objectManager;

s32 characterModelData[][3] = {
    { DATA_MAKE_NUM(DATADIR_M435, 27), DATA_MAKE_NUM(DATADIR_M435, 26), DATA_MAKE_NUM(DATADIR_MGCONST, 0) },
    { DATA_MAKE_NUM(DATADIR_M435, 28), DATA_MAKE_NUM(DATADIR_M435, 26), DATA_MAKE_NUM(DATADIR_MGCONST, 1) },
    { DATA_MAKE_NUM(DATADIR_M435, 30), DATA_MAKE_NUM(DATADIR_M435, 26), DATA_MAKE_NUM(DATADIR_MGCONST, 2) },
    { DATA_MAKE_NUM(DATADIR_M435, 29), DATA_MAKE_NUM(DATADIR_M435, 26), DATA_MAKE_NUM(DATADIR_MGCONST, 3) },
    { DATA_MAKE_NUM(DATADIR_M435, 32), DATA_MAKE_NUM(DATADIR_M435, 26), DATA_MAKE_NUM(DATADIR_MGCONST, 4) },
    { DATA_MAKE_NUM(DATADIR_M435, 31), DATA_MAKE_NUM(DATADIR_M435, 26), DATA_MAKE_NUM(DATADIR_MGCONST, 5) },
    { DATA_MAKE_NUM(DATADIR_M435, 33), DATA_MAKE_NUM(DATADIR_M435, 26), DATA_MAKE_NUM(DATADIR_MGCONST, 6) },
    { DATA_MAKE_NUM(DATADIR_M435, 34), DATA_MAKE_NUM(DATADIR_M435, 26), DATA_MAKE_NUM(DATADIR_MGCONST, 7) }
};

char* characterModelNames[] = {
    "c000m1-ske_head",
    "c001m1-ske_head",
    "c002m1-ske_head",
    "c003m1-ske_head",
    "c004m1-ske_head",
    "c005m1-ske_head",
    "c006m1-ske_head",
    "c007m1-ske_head"
};

s32 characterAnimationFlags[][2] = {
    { 0x00000100, 0x00000001 },
    { 0x00000100, 0x00000001 },
    { 0x00000080, 0x00000000 },
    { 0x00000080, 0x00000000 },
    { 0x00000080, 0x00000000 },
    { 0x00000100, 0x00000001 },
    { 0x00000080, 0x00000000 },
    { 0x00000080, 0x00000000 }
};

s32 characterAnimationEventFrames[][5] = {
    {  91, 105, 126, 141, -1 },
    {  91, 105, 126, 141, -1 },
    { 124, 137,  -1,  -1, -1 },
    {  85, 100, 127,  -1, -1 },
    { 124, 139,  -1,  -1, -1 },
    {  85,  97, 127, 139, -1 },
    { 127, 137,  -1,  -1, -1 },
    {  79,  95, 109, 127, -1 }
};

s32 currentCharacterMotionId = -1;
s32 activeCharacterMotionId = -1;
s32 activeCharacterId = -1;

void PlayCharacterMotionEvents(void) {
    s32 i;

    while (TRUE) {
        HuPrcVSleep();
        if (currentCharacterMotionId == -1 || activeCharacterMotionId == -1 || activeCharacterId == -1 || activeCharacterMotionId != Hu3DMotionIDGet(currentCharacterMotionId)) {
            continue;
        }
        for (i = 0; i < 5; i++) {
            if (characterAnimationEventFrames[activeCharacterId][i] != -1 && Hu3DMotionTimeGet(currentCharacterMotionId) >= characterAnimationEventFrames[activeCharacterId][i]) {
                OSReport("KE-System  CHR:%d TIME:%d\n", activeCharacterId, characterAnimationEventFrames[activeCharacterId][i]);
                HuAudFXPlay(0x6D8);
                characterAnimationEventFrames[activeCharacterId][i] = -1;
            }
        }
    }
}

void CreateGlobalLight(void) {
    s32 globalLightIndex;

    globalLightIndex = Hu3DGLightCreate(0.0f, 1000.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0xFF, 0xFF, 0xFF);
    Hu3DGLightInfinitytSet(globalLightIndex);
}

void SetCameraShake(s32 cameraIndex, s32 frameCount, float shakeAmplitude) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    cameraTween->targetMotionIds[cameraIndex] = frameCount;
    if (cameraTween->targetMotionValues[cameraIndex] < 0.0f) {
        cameraTween->targetMotionValues[cameraIndex] = -shakeAmplitude;
    }
    else {
        cameraTween->targetMotionValues[cameraIndex] = shakeAmplitude;
    }
}

void SetCameraShakeImpulse(s32 cameraIndex, float amplitude) {
    SetCameraShake(cameraIndex, -1, amplitude);
}

float UpdateCameraShake(s32 cameraIndex) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    if (cameraTween->targetMotionValues[cameraIndex] != 0.0f) {
        cameraTween->targetMotionValues[cameraIndex] *= -0.9f;
        if (cameraTween->targetMotionIds[cameraIndex] == -1) {
            if (cameraTween->targetMotionValues[cameraIndex] >= -0.5f && cameraTween->targetMotionValues[cameraIndex] <= 0.5f) {
                cameraTween->targetMotionValues[cameraIndex] = 0.0f;
            }
        }
        else {
            cameraTween->targetMotionIds[cameraIndex]--;
            if (cameraTween->targetMotionIds[cameraIndex] <= 0) {
                cameraTween->targetMotionIds[cameraIndex] = 0;
                cameraTween->targetMotionValues[cameraIndex] = 0.0f;
            }
        }
    }
    return cameraTween->targetMotionValues[cameraIndex];
}

void UpdateCameraDebugControls(void) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    if (HuPadBtn[0] & 0x200) {
        if (HuPadBtn[0] & 0x100) {
            cameraTween->cameraCenterZ -= HuPadStkY[0] / 10.0f;
        }
        else {
            cameraTween->cameraCenterX += HuPadStkX[0] / 10.0f;
            cameraTween->cameraCenterY += HuPadStkY[0] / 10.0f;
        }
    }
    if (HuPadBtn[0] & 0x800) {
        cameraTween->cameraRotationX -= HuPadStkY[0] / 100.0f;
        cameraTween->cameraRotationY += HuPadStkX[0] / 100.0f;
        if (cameraTween->cameraRotationX < 0.0f) {
            cameraTween->cameraRotationX += 360.0f;
        }
        if (cameraTween->cameraRotationX >= 360.0f) {
            cameraTween->cameraRotationX -= 360.0f;
        }
        if (cameraTween->cameraRotationY < 0.0f) {
            cameraTween->cameraRotationY += 360.0f;
        }
        if (cameraTween->cameraRotationY >= 360.0f) {
            cameraTween->cameraRotationY -= 360.0f;
        }
    }
    if (HuPadBtn[0] & 0x400) {
        cameraTween->cameraZoom -= HuPadStkY[0] / 10.0f;
    }
    print8(16, 100, 1.0f, ">>>>>>>>>> CAMERA TEST <<<<<<<<<<");
    print8(16, 110, 1.0f, "CENTER : %.2f, %.2f, %.2f", cameraTween->cameraCenterX, cameraTween->cameraCenterY, cameraTween->cameraCenterZ);
    print8(16, 120, 1.0f, "ROT    : %.2f, %.2f, %.2f", cameraTween->cameraRotationX, cameraTween->cameraRotationY, cameraTween->cameraRotationZ);
    print8(16, 130, 1.0f, "ZOOM   : %.2f", cameraTween->cameraZoom);
}

void ResetCameraView(void) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    cameraTween->cameraCenterX = 0.0f;
    cameraTween->cameraCenterY = 225.0f;
    cameraTween->cameraCenterZ = 1740.0f;
    cameraTween->cameraRotationX = -10.0f;
    cameraTween->cameraRotationY = 0.0f;
    cameraTween->cameraRotationZ = 0.0f;
    cameraTween->cameraZoom = 300.0f;
}

void SetCameraUpdateCallback(void (*callback)(void)) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    cameraTween->positionCallback = callback;
}

void SetCameraControlCallback(void (*callback)(void)) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    cameraTween->rotationCallback = callback;
}

void UpdateCameraControls(OMOBJ* object) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    if (cameraTween->positionCallback != NULL) {
        cameraTween->positionCallback();
    }
}

void UpdateCameraView(OMOBJ* object) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    UpdateCameraControls(object);
    Center.x = cameraTween->cameraCenterX + UpdateCameraShake(0);
    Center.y = cameraTween->cameraCenterY + UpdateCameraShake(1);
    Center.z = cameraTween->cameraCenterZ + UpdateCameraShake(2);
    CRot.x = cameraTween->cameraRotationX + UpdateCameraShake(3);
    CRot.y = cameraTween->cameraRotationY + UpdateCameraShake(4);
    CRot.z = cameraTween->cameraRotationZ + UpdateCameraShake(5);
    CZoom = cameraTween->cameraZoom + UpdateCameraShake(6);
}

void InitializeCamera(void) {
    Hu3DCameraCreate(1);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraPerspectiveSet(1, 45.0f, 0.1f, 10000.0f, 1.2f);
    SetCameraUpdateCallback(ResetCameraView);
    SetCameraControlCallback(UpdateCameraDebugControls);
    omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutView);
    omAddObjEx(objectManager, 0x7FD9, 0, 0, -1, UpdateCameraView);
}

void InitializeShadow(void) {
    Vec position = { 0.0f, 2500.0f, 1.0f };
    Vec position2 = { 0.0f, 0.0f, 0.0f };
    Vec position3 = { 0.0f, 1.0f, 0.0f };

    Hu3DShadowCreate(45.0f, 2000.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.8f);
    Hu3DShadowPosSet(&position, &position3, &position2);
}

void DrawWindow(s32 windowId, float horizontalAnchor, float verticalAnchor, s32 choiceIndex) {
    WindowData* window;
    float windowLeft;
    float windowTop;
    float windowWidth;
    float windowHeight;
    float expandedXScale;
    float expandedYScale;
    float expandedX;
    float expandedY;
    float currentX;
    float currentY;
    float currentXScale;
    float currentYScale;
    s32 animationFrame;

    window = &winData[windowId];
    windowLeft = window->pos_x;
    windowTop = window->pos_y;
    windowWidth = window->w;
    windowHeight = window->h;
    expandedX = windowLeft + windowWidth * horizontalAnchor;
    expandedXScale = 4.0f / windowWidth;
    if (expandedX + 4.0f - (windowLeft + windowWidth) >= 0.0f) {
        expandedX = windowLeft + windowWidth - 4.0f;
    }
    else {
        if (windowLeft - (expandedX - 2.0f) >= 0.0f) {
            expandedX = windowLeft;
        }
        else {
            expandedX -= 2.0f;
        }
    }
    expandedY = windowTop + windowHeight * verticalAnchor;
    expandedYScale = 4.0f / windowHeight;
    if (expandedY + 4.0f - (windowTop + windowHeight) >= 0.0f) {
        expandedY = windowTop + windowHeight - 4.0f;
    }
    else {
        if (windowTop - (expandedY - 2.0f) >= 0.0f) {
            expandedY = windowTop;
        }
        else {
            expandedY -= 2.0f;
        }
    }
    if (choiceIndex != 0) {
        HuWinPosSet(windowId, expandedX, expandedY);
        HuWinScaleSet(windowId, expandedXScale, expandedYScale);
        currentX = expandedX;
        currentXScale = expandedXScale;
        currentY = expandedY;
        currentYScale = expandedYScale;
    }
    else {
        HuWinPosSet(windowId, windowLeft, windowTop);
        HuWinScaleSet(windowId, 1.0f, 1.0f);
        currentX = windowLeft;
        currentXScale = 1.0f;
        currentY = windowTop;
        currentYScale = 1.0f;
    }
    HuWinDispOn(windowId);
    for (animationFrame = 0; animationFrame <= 15; animationFrame++) {
        HuPrcVSleep();
        if (choiceIndex != 0) {
            if (animationFrame <= 10) {
                currentX = interpolateSineEaseOut(expandedX, windowLeft, animationFrame, 10.0f);
                currentXScale = interpolateSineEaseOut(expandedXScale, 1.0f, animationFrame, 10.0f);
            }
            else {
                currentY = interpolateSineEaseOut(expandedY, windowTop, animationFrame - 10, 5.0f);
                currentYScale = interpolateSineEaseOut(expandedYScale, 1.0f, animationFrame - 10, 5.0f);
            }
        }
        else {
            if (animationFrame <= 10) {
                currentY = interpolateSineEaseOut(windowTop, expandedY, animationFrame, 10.0f);
                currentYScale = interpolateSineEaseOut(1.0f, expandedYScale, animationFrame, 10.0f);
            }
            else {
                currentX = interpolateSineEaseOut(windowLeft, expandedX, animationFrame - 10, 5.0f);
                currentXScale = interpolateSineEaseOut(1.0f, expandedXScale, animationFrame - 10, 5.0f);
            }
        }
        HuWinPosSet(windowId, currentX, currentY);
        HuWinScaleSet(windowId, currentXScale, currentYScale);
    }
    if (choiceIndex != 0) {
        HuWinPosSet(windowId, windowLeft, windowTop);
        HuWinScaleSet(windowId, 1.0f, 1.0f);
    }
    else {
        HuWinPosSet(windowId, expandedX, expandedY);
        HuWinScaleSet(windowId, expandedXScale, expandedYScale);
    }
    HuPrcVSleep();
}

void InitializeWindows(void) {
    HuWinInit(1);
}

void AnimateWindowOpen(s32 windowId, float horizontalAnchor, float verticalAnchor) {
    WindowData* window;
    float windowLeft;
    float windowTop;
    float windowWidth;
    float windowHeight;
    float expandedXScale;
    float expandedYScale;
    float expandedX;
    float expandedY;
    float currentX;
    float currentY;
    float currentXScale;
    float currentYScale;
    s32 animationFrame;

    window = &winData[windowId];
    windowLeft = window->pos_x;
    windowTop = window->pos_y;
    windowWidth = window->w;
    windowHeight = window->h;
    expandedX = windowLeft + windowWidth * horizontalAnchor;
    expandedXScale = 4.0f / windowWidth;
    if (expandedX + 4.0f - (windowLeft + windowWidth) >= 0.0f) {
        expandedX = windowLeft + windowWidth - 4.0f;
    }
    else {
        if (windowLeft - (expandedX - 2.0f) >= 0.0f) {
            expandedX = windowLeft;
        }
        else {
            expandedX -= 2.0f;
        }
    }
    expandedY = windowTop + windowHeight * verticalAnchor;
    expandedYScale = 4.0f / windowHeight;
    if (expandedY + 4.0f - (windowTop + windowHeight) >= 0.0f) {
        expandedY = windowTop + windowHeight - 4.0f;
    }
    else {
        if (windowTop - (expandedY - 2.0f) >= 0.0f) {
            expandedY = windowTop;
        }
        else {
            expandedY -= 2.0f;
        }
    }
    HuWinPosSet(windowId, expandedX, expandedY);
    HuWinScaleSet(windowId, expandedXScale, expandedYScale);
    HuPrcVSleep();
    for (animationFrame = 0; animationFrame < 10; animationFrame++) {
        currentX = interpolateSineEaseOut(expandedX, windowLeft, animationFrame, 10.0f);
        currentXScale = interpolateSineEaseOut(expandedXScale, 1.0f, animationFrame, 10.0f);
        currentY = interpolateSineEaseOut(expandedY, windowTop, animationFrame, 10.0f);
        currentYScale = interpolateSineEaseOut(expandedYScale, 1.0f, animationFrame, 10.0f);
        HuWinPosSet(windowId, currentX, currentY);
        HuWinScaleSet(windowId, currentXScale, currentYScale);
        HuPrcVSleep();
    }
    HuWinPosSet(windowId, windowLeft, windowTop);
    HuWinScaleSet(windowId, 1.0f, 1.0f);
    HuPrcVSleep();
}

s32 CreateTintedMessageWindow(float left, float top, float width, float height, s32 teamColorIndex) {
    s32 windowId;
    s32 teamColorPresets[][6] = {
        { 0xFF, 0xFF, 0xCC, 0x00, 0x09, 0x09 },
        { 0xCC, 0xFF, 0xFF, 0x00, 0x09, 0x09 },
        { 0xFF, 0xCC, 0xFF, 0x00, 0x09, 0x09 }
    };

    windowId = HuWinCreate(left, top, width, height, 1);
    HuSprColorSet(winData[windowId].group, 0, teamColorPresets[teamColorIndex][0], teamColorPresets[teamColorIndex][1], teamColorPresets[teamColorIndex][2]);
    winData[windowId].mess_color = teamColorPresets[teamColorIndex][3];
    winData[windowId].mess_shadow_color = teamColorPresets[teamColorIndex][4];
    winData[windowId].attr |= 0x80;
    HuWinBGTPLvlSet(windowId, teamColorPresets[teamColorIndex][5] / 10.0f);
    winData[windowId].active_pad = 0;
    DrawWindow(windowId, 0.5f, 0.5f, 1);
    return windowId;
}

s32 CreateMessageWindow(float left, float top, float width, float height, s32 teamColorIndex) {
    return CreateTintedMessageWindow(left, top, width, height, teamColorIndex);
}

void AnimateWindowClose(s32 windowId, float horizontalAnchor, float verticalAnchor) {
    WindowData* window;
    float windowLeft;
    float windowTop;
    float windowWidth;
    float windowHeight;
    float expandedXScale;
    float expandedYScale;
    float expandedX;
    float expandedY;
    float currentX;
    float currentY;
    float currentXScale;
    float currentYScale;
    s32 animationFrame;

    window = &winData[windowId];
    windowLeft = window->pos_x;
    windowTop = window->pos_y;
    windowWidth = window->w;
    windowHeight = window->h;
    expandedX = windowLeft + windowWidth * horizontalAnchor;
    expandedXScale = 4.0f / windowWidth;
    if (expandedX + 4.0f - (windowLeft + windowWidth) >= 0.0f) {
        expandedX = windowLeft + windowWidth - 4.0f;
    }
    else {
        if (windowLeft - (expandedX - 2.0f) >= 0.0f) {
            expandedX = windowLeft;
        }
        else {
            expandedX = expandedX - 2.0f;
        }
    }
    expandedY = windowTop + windowHeight * verticalAnchor;
    expandedYScale = 4.0f / windowHeight;
    if (expandedY + 4.0f - (windowTop + windowHeight) >= 0.0f) {
        expandedY = windowTop + windowHeight - 4.0f;
    }
    else {
        if (windowTop - (expandedY - 2.0f) >= 0.0f) {
            expandedY = windowTop;
        }
        else {
            expandedY = expandedY - 2.0f;
        }
    }
    HuWinPosSet(windowId, windowLeft, windowTop);
    HuWinScaleSet(windowId, 1.0f, 1.0f);
    HuPrcVSleep();
    for (animationFrame = 0; animationFrame < 10; animationFrame++) {
        currentX = interpolateCosineEaseIn(windowLeft, expandedX, animationFrame, 10.0f);
        currentXScale = interpolateCosineEaseIn(1.0f, expandedXScale, animationFrame, 10.0f);
        currentY = interpolateCosineEaseIn(windowTop, expandedY, animationFrame, 10.0f);
        currentYScale = interpolateCosineEaseIn(1.0f, expandedYScale, animationFrame, 10.0f);
        HuWinPosSet(windowId, currentX, currentY);
        HuWinScaleSet(windowId, currentXScale, currentYScale);
        HuPrcVSleep();
    }
    HuWinPosSet(windowId, expandedX, expandedY);
    HuWinScaleSet(windowId, expandedXScale, expandedYScale);
    HuPrcVSleep();
}

void CloseMessageWindow(s32 windowId) {
    HuWinMesSet(windowId, MAKE_MESSID(29, 22));
    HuPrcVSleep();
    DrawWindow(windowId, 0.5f, 0.5f, 0);
    HuWinKill(windowId);
}

void DismissMessageWindow(s32 windowId) {
    CloseMessageWindow(windowId);
}

void WaitForMessageAcknowledgement(s32 windowId, s32 waitFrames) {
    s32 i;
    s32 hasHumanPlayer;
    s32 messageDelay;

    hasHumanPlayer = 0;
    messageDelay = GWMessDelayGet();
    if (waitFrames > 0) {
        HuPrcSleep(waitFrames);
        return;
    }
    HuWinComKeyReset();
    for (i = 0; i < 4; i++) {
        if (playerCharacterStates[i].isCpu == 0) {
            hasHumanPlayer = 1;
            winData[windowId].active_pad = 0xF;
        }
    }
    if (hasHumanPlayer == 0) {
        winData[windowId].active_pad = 0xF;
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, messageDelay);
    }
    HuWinMesWait(windowId);
    HuWinComKeyReset();
}

void ShowMessageChoice(s32 windowId) {
    s32 hasHumanPlayerOrChoice;
    s32 messageDelay;
    s32 playerIndex;

    hasHumanPlayerOrChoice = 0;
    messageDelay = GWMessDelayGet();
    HuWinComKeyReset();
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerCharacterStates[playerIndex].isCpu == 0) {
            hasHumanPlayerOrChoice = 1;
            winData[windowId].active_pad = 0xF;
        }
    }
    if (hasHumanPlayerOrChoice == 0) {
        winData[windowId].active_pad = 0xF;
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, messageDelay);
    }
    hasHumanPlayerOrChoice = HuWinChoiceGet(windowId, 1);
    if (hasHumanPlayerOrChoice == -1 || hasHumanPlayerOrChoice == 1) {
        randomizedPlayerIndex = 0;
    }
    else {
        randomizedPlayerIndex = 1;
    }
    HuWinComKeyReset();
}

void SetWindowMessage(s32 windowId, s32 messageId) {
    winData[windowId].mess_color = 0;
    winData[windowId].mess_shadow_color = 9;
    winData[windowId].attr |= 0x80;
    HuWinMesSet(windowId, messageId);
}

void ExpandMessageDimensions(float* maxDimensions, s32 messageId) {
    float messageSize[2];

    HuWinMesMaxSizeGet(1, messageSize, messageId);
    if (messageSize[0] > maxDimensions[0]) {
        maxDimensions[0] = messageSize[0];
    }
    if (messageSize[1] > maxDimensions[1]) {
        maxDimensions[1] = messageSize[1];
    }
}

void StartResultCountdown(s32 countdownSeconds) {
    if (roundWinnerIndex == 0) {
        roundWinnerIndex = 1;
        roundWinnerScore = countdownSeconds * REFRESH_RATE;
    }
}

s32 IsCountdownComplete(void) {
    if (roundWinnerIndex == 3) {
        return 1;
    } else {
        return 0;
    }
}

s32 SkipResultCountdown(void) {
    if (roundWinnerIndex == 2) {
        roundWinnerIndex = 3;
        roundWinnerScore = 30;
    }
}

s32 countdownSequenceId = -1;

void UpdateResultCountdown(OMOBJ* countdownObject) {
    switch (roundWinnerIndex) {
        case 1:
            if (countdownSequenceId == -1) {
                countdownSequenceId = MGSeqCreate(1, roundWinnerScore / REFRESH_RATE, -1, -1);
            }
            roundWinnerIndex = 2;
            break;
        case 2:
            roundWinnerScore--;
            if (countdownSequenceId != -1) {
                MGSeqParamSet(countdownSequenceId, 1, (roundWinnerScore + REFRESH_RATE - 1) / REFRESH_RATE);
            }
            if (roundWinnerScore <= 0) {
                roundWinnerScore = 30;
                roundWinnerIndex = 3;
            }
            break;
        case 3:
            if (countdownSequenceId != -1) {
                MGSeqParamSet(countdownSequenceId, 2, -1);
                countdownSequenceId = -1;
            }
            roundWinnerScore = 0;
            roundWinnerIndex = 0;
            break;
    }
}

void InitializeResultCountdown(void) {
    roundWinnerScore = 0;
    roundWinnerIndex = 0;
    omAddObjEx(objectManager, 0x2000, 0, 0, -1, UpdateResultCountdown);
}

void RotateWheelModels(OMOBJ* wheelObject) {
    s32 i;

    for (i = 0; i < 3; i++) {
        Hu3DData[wheelObject->mdlId[i + 5]].rot.y += 360.0f / (2500.0f + 2500.0f * i);
    }
}

void SetGameAnimationState(s32 animationState) {
    GameAnimationState* cameraState;

    cameraState = &gameAnimationState;
    cameraState->mainAnimationState = animationState;
}

void UpdateGameAnimation(OMOBJ* wheelObject) {
    GameAnimationState* animationStateData;

    animationStateData = &gameAnimationState;
    switch (animationStateData->mainAnimationState) {
        case 2:
            SetModelMotion(wheelObject, 9, 9, 1, 0);
            Hu3DModelAttrReset(wheelObject->mdlId[9], 1);
            animationStateData->motionEventState = 0;
            SetCameraShake(1, 10, 4.0f);
            HuAudFXPlay(0x6CF);
            /* fallthrough */
        case 1:
            SetModelMotion(wheelObject, 0, 0, 1, 0);
            SetModelMotion(wheelObject, 3, 3, 1, 0);
            SetModelMotion(wheelObject, 4, 4, 1, 0);
            SetModelMotion(wheelObject, 5, 5, 1, 0);
            SetModelMotion(wheelObject, 6, 6, 1, 0);
            SetModelMotion(wheelObject, 7, 7, 1, 0);
            if (animationStateData->mainAnimationState == 1) {
                HuAudFXPlay(0x6CE);
            }
            animationStateData->mainAnimationState = 20;
            break;
        case 3:
            animationStateData->mainAnimationState = 10;
            SetModelMotion(wheelObject, 0, 0, 1, 0);
            SetModelMotion(wheelObject, 1, 1, 1, 0);
            SetModelMotion(wheelObject, 2, 2, 1, 0);
            SetModelMotion(wheelObject, 3, 3, 1, 0);
            SetModelMotion(wheelObject, 4, 4, 1, 0);
            SetModelMotion(wheelObject, 5, 5, 1, 0);
            SetModelMotion(wheelObject, 6, 6, 1, 0);
            SetModelMotion(wheelObject, 7, 7, 1, 0);
            SetModelMotion(wheelObject, 8, 8, 0, 0);
            HuAudFXPlay(0x6D0);
            Hu3DModelAttrReset(wheelObject->mdlId[8], 1);
            break;
        case 10:
            SetCameraShake(6, -1, 8.0f);
            if (Hu3DMotionTimeGet(wheelObject->mdlId[3]) >= 30.0f) {
                animationStateData->mainAnimationState = 20;
                SetWheelEntranceState(1);
                SetCameraShake(6, -1, 128.0f);
                HuAudFXPlay(0x6DA);
                omVibrate(GetGamePlayerIndex(0), 12, 12, 0);
                omVibrate(GetGamePlayerIndex(1), 12, 12, 0);
                omVibrate(GetGamePlayerIndex(2), 12, 12, 0);
                omVibrate(GetGamePlayerIndex(3), 12, 12, 0);
            }
            break;
        case 20:
            if (Hu3DMotionEndCheck(wheelObject->mdlId[3])) {
                animationStateData->mainAnimationState = 0;
                SetModelMotion(wheelObject, 3, 10, 1, 1);
                SetModelMotion(wheelObject, 4, 11, 1, 1);
                Hu3DModelAttrSet(wheelObject->mdlId[8], 1);
                Hu3DModelAttrSet(wheelObject->mdlId[9], 1);
            }
            break;
    }
    if (Hu3DMotionTimeGet(wheelObject->mdlId[9]) >= 25.0f && animationStateData->motionEventState == 0) {
        animationStateData->motionEventState = 1;
        SetCameraShake(1, 10, 4.0f);
    } else if (Hu3DMotionTimeGet(wheelObject->mdlId[9]) >= 40.0f && animationStateData->motionEventState == 1) {
        animationStateData->motionEventState = 2;
        SetCameraShake(1, 10, 4.0f);
    }
    if (animationStateData->mainAnimationState == 0) {
        if (animationStateData->idleAnimationFrames++ >= REFRESH_RATE * 5) {
            animationStateData->idleAnimationFrames = 0;
            if (rand8() % 2 != 0) {
                SetGameAnimationState(1);
            }
        }
    }
}

void UpdateGameScene(OMOBJ* wheelObject) {
    RotateWheelModels(wheelObject);
    UpdateGameAnimation(wheelObject);
}

void InitializeWheelModels(GameAnimationState* animationState) {
    OMOBJ* wheelObject;

    wheelObject = animationState->mainObject;
    SetModelMotion(wheelObject, 0, 0, 1, 2);
    Hu3DModelPosSet(wheelObject->mdlId[1], 0.0f, 0.0f, -680.0f);
    SetModelMotion(wheelObject, 1, 1, 1, 2);
    Hu3DModelPosSet(wheelObject->mdlId[2], 0.0f, 0.0f, -680.0f);
    SetModelMotion(wheelObject, 2, 2, 1, 2);
    Hu3DModelPosSet(wheelObject->mdlId[3], 525.0f, 0.0f, -1150.0f);
    SetModelMotion(wheelObject, 3, 10, 1, 1);
    Hu3DModelPosSet(wheelObject->mdlId[4], -525.0f, 0.0f, -1150.0f);
    SetModelMotion(wheelObject, 4, 11, 1, 1);
    Hu3DModelPosSet(wheelObject->mdlId[5], 50.0f, 150.0f, 50.0f);
    SetModelMotion(wheelObject, 5, 5, 1, 2);
    Hu3DModelPosSet(wheelObject->mdlId[6], 0.0f, 150.0f, 0.0f);
    SetModelMotion(wheelObject, 6, 6, 1, 2);
    Hu3DModelPosSet(wheelObject->mdlId[7], 0.0f, 150.0f, 0.0f);
    SetModelMotion(wheelObject, 7, 7, 1, 2);
    Hu3DModelAttrSet(wheelObject->mdlId[8], 1);
    SetModelMotion(wheelObject, 8, 8, 1, 2);
    Hu3DModelAttrSet(wheelObject->mdlId[9], 1);
    SetModelMotion(wheelObject, 9, 9, 1, 2);
    wheelObject->work[0] = wheelObject->work[1] = wheelObject->work[2] = wheelObject->work[3] = 0;
    animationState->mainAnimationState = animationState->unreferencedAnimationState08 = animationState->unreferencedAnimationState0C = animationState->unreferencedAnimationState10 = 0;
    animationState->idleAnimationFrames = animationState->unreferencedAnimationState18 = animationState->motionEventState = animationState->unreferencedAnimationState20 = 0;
}

void CreateWheelModels(OMOBJ* wheelObject) {
    GameAnimationState* animationStateData;

    animationStateData = &gameAnimationState;
    wheelObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 39));
    wheelObject->mtnId[0] = Hu3DMotionIDGet(wheelObject->mdlId[0]);
    Hu3DModelShadowMapSet(wheelObject->mdlId[0]);
    SetModelMotion(wheelObject, 0, 0, 1, 2);
    wheelObject->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 40));
    wheelObject->mtnId[1] = Hu3DMotionIDGet(wheelObject->mdlId[1]);
    Hu3DModelPosSet(wheelObject->mdlId[1], 0.0f, 0.0f, -680.0f);
    Hu3DModelShadowMapSet(wheelObject->mdlId[1]);
    SetModelMotion(wheelObject, 1, 1, 1, 2);
    wheelObject->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 41));
    wheelObject->mtnId[2] = Hu3DMotionIDGet(wheelObject->mdlId[2]);
    Hu3DModelPosSet(wheelObject->mdlId[2], 0.0f, 0.0f, -680.0f);
    Hu3DModelShadowSet(wheelObject->mdlId[2]);
    SetModelMotion(wheelObject, 2, 2, 1, 2);
    wheelObject->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 42));
    wheelObject->mtnId[3] = Hu3DMotionIDGet(wheelObject->mdlId[3]);
    wheelObject->mtnId[10] = Hu3DJointMotionFile(wheelObject->mdlId[3], DATA_MAKE_NUM(DATADIR_M435, 44));
    Hu3DModelPosSet(wheelObject->mdlId[3], 525.0f, 0.0f, -1150.0f);
    Hu3DModelShadowSet(wheelObject->mdlId[3]);
    SetModelMotion(wheelObject, 3, 10, 1, 1);
    wheelObject->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 43));
    wheelObject->mtnId[4] = Hu3DMotionIDGet(wheelObject->mdlId[4]);
    wheelObject->mtnId[11] = Hu3DJointMotionFile(wheelObject->mdlId[4], DATA_MAKE_NUM(DATADIR_M435, 45));
    Hu3DModelPosSet(wheelObject->mdlId[4], -525.0f, 0.0f, -1150.0f);
    Hu3DModelShadowSet(wheelObject->mdlId[4]);
    SetModelMotion(wheelObject, 4, 11, 1, 1);
    wheelObject->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 36));
    wheelObject->mtnId[5] = Hu3DMotionIDGet(wheelObject->mdlId[5]);
    Hu3DModelPosSet(wheelObject->mdlId[5], 50.0f, 150.0f, 50.0f);
    SetModelMotion(wheelObject, 5, 5, 1, 2);
    wheelObject->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 37));
    wheelObject->mtnId[6] = Hu3DMotionIDGet(wheelObject->mdlId[6]);
    Hu3DModelPosSet(wheelObject->mdlId[6], 0.0f, 150.0f, 0.0f);
    SetModelMotion(wheelObject, 6, 6, 1, 2);
    wheelObject->mdlId[7] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 38));
    wheelObject->mtnId[7] = Hu3DMotionIDGet(wheelObject->mdlId[7]);
    Hu3DModelPosSet(wheelObject->mdlId[7], 0.0f, 150.0f, 0.0f);
    SetModelMotion(wheelObject, 7, 7, 1, 2);
    wheelObject->mdlId[8] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 46));
    wheelObject->mtnId[8] = Hu3DMotionIDGet(wheelObject->mdlId[8]);
    Hu3DModelAttrSet(wheelObject->mdlId[8], 1);
    Hu3DModelLayerSet(wheelObject->mdlId[8], 1);
    SetModelMotion(wheelObject, 8, 8, 1, 2);
    wheelObject->mdlId[9] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 47));
    wheelObject->mtnId[9] = Hu3DMotionIDGet(wheelObject->mdlId[9]);
    Hu3DModelAttrSet(wheelObject->mdlId[9], 1);
    Hu3DModelLayerSet(wheelObject->mdlId[9], 1);
    SetModelMotion(wheelObject, 9, 9, 1, 2);
    wheelObject->work[0] = wheelObject->work[1] = wheelObject->work[2] = wheelObject->work[3] = 0;
    animationStateData->mainAnimationState = animationStateData->unreferencedAnimationState08 = animationStateData->unreferencedAnimationState0C = animationStateData->unreferencedAnimationState10 = 0;
    animationStateData->idleAnimationFrames = animationStateData->unreferencedAnimationState18 = animationStateData->motionEventState = animationStateData->unreferencedAnimationState20 = 0;
    wheelObject->objFunc = UpdateGameScene;
}

void SetWheelEntranceState(s32 entranceState) {
    WheelMapState* mapState;

    mapState = &wheelMapState;
    mapState->entranceAnimationState = entranceState;
}

float wheelRotationAngle = 270.0f;

void UpdateWheelEntrance(OMOBJ* wheelObject) {
    Vec modelPosition;
    Vec screenPosition;
    HU3DMODEL* wheelModel;
    WheelMapState* mapState;
    s32 audioPan;

    wheelModel = &Hu3DData[wheelObject->mdlId[1]];
    mapState = &wheelMapState;
    if (mapState->entranceAnimationState == 1) {
        mapState->entranceAnimationState = 0;
        SetModelMotion(wheelObject, 0, 0, 1, 0);
        SetModelMotion(wheelObject, 1, 2, 1, 0);
        Hu3DModelAttrReset(wheelObject->mdlId[0], 1);
        Hu3DModelAttrReset(wheelObject->mdlId[1], 1);
    } else if (mapState->entranceAnimationState >= 2) {
        wheelModel->pos.x = 750.0 * cosd(wheelRotationAngle);
        wheelModel->pos.z = 500.0 + 750.0 * sind(wheelRotationAngle);
        wheelModel->rot.y += 0.2f;
        if (mapState->entranceAnimationState == 2) {
            mapState->entranceAnimationState = 3;
            ambientSoundState.ambientSoundId = HuAudFXPlay(0x6DB);
            SetModelMotion(wheelObject, 1, 1, 1, 1);
        }
        wheelRotationAngle -= 0.2f;
        if (wheelRotationAngle < 180.0f) {
            mapState->entranceAnimationState = 0;
            HuAudFXStop(ambientSoundState.ambientSoundId);
            ambientSoundState.ambientSoundId = -1;
            wheelRotationAngle = 270.0f;
            Hu3DModelPosSet(wheelObject->mdlId[1], -750.0f, 0.0f, 500.0f);
            Hu3DModelRotSet(wheelObject->mdlId[1], 0.0f, 90.0f, 0.0f);
            SetModelMotion(wheelObject, 1, 1, 1, 2);
            SetPrimaryPlayerFrame(6);
        }
    }
    modelPosition.x = wheelModel->pos.x;
    modelPosition.y = wheelModel->pos.y;
    modelPosition.z = wheelModel->pos.z;
    Hu3D3Dto2D(&modelPosition, 1, &screenPosition);
    audioPan = 32.0f + screenPosition.x * 0.1f;
    if (audioPan <= 32) {
        audioPan = 32;
    } else if (audioPan >= 96) {
        audioPan = 96;
    }
    if (ambientSoundState.ambientSoundId != -1) {
        HuAudFXPanning(ambientSoundState.ambientSoundId, audioPan);
    }
}

void SetWheelEnvironmentState(s32 environmentState) {
    WheelMapState* mapState;

    mapState = &wheelMapState;
    mapState->environmentAnimationState = environmentState;
}

void UpdateWheelRotation(OMOBJ* wheelObject) {
    WheelMapState* mapState;
    s32 i;

    mapState = &wheelMapState;
    switch (mapState->environmentAnimationState) {
        case 0:
            mapState->environmentAnimationSubstate = 0;
            mapState->wheelSpeed = 7.5f;
            mapState->wheelAngle = mapState->wheelAngle;
            break;
        case 1:
            if (ambientSoundState.wheelSoundActive == 0) {
                ambientSoundState.wheelSoundActive = 1;
                ambientSoundState.wheelSoundId = HuAudFXPlay(0x6DE);
            }
            SetWheelEnvironmentSubstate(1);
            mapState->wheelAngle -= mapState->wheelSpeed;
            if (mapState->wheelAngle < 0.0f) {
                mapState->wheelAngle += 360.0f;
            }
            break;
        case 2:
            if (ambientSoundState.wheelSoundActive == 1) {
                ambientSoundState.wheelSoundActive = 0;
                HuAudFXStop(ambientSoundState.wheelSoundId);
            }
            for (i = 0; i < 3; i++) {
                if (mapState->pieceState[i] != 7) {
                    break;
                }
            }
            mapState->wheelAngle -= mapState->wheelSpeed;
            if (mapState->wheelAngle < 0.0f) {
                mapState->wheelAngle += 360.0f;
            }
            if (i == 3) {
                mapState->environmentAnimationState = 3;
            }
            break;
        case 3:
            mapState->wheelAngle -= mapState->wheelSpeed;
            if (mapState->wheelAngle < 0.0f) {
                mapState->wheelAngle += 360.0f;
            }
            mapState->wheelSpeed -= 0.025f;
            if (mapState->wheelSpeed < 0.0f) {
                mapState->wheelSpeed = 0.0f;
            }
            break;
    }
    omSetRot(wheelObject, 0.0f, 0.0f, mapState->wheelAngle);
}

void SetWheelEnvironmentSubstate(s32 substate) {
    WheelMapState* mapState;

    mapState = &wheelMapState;
    if (mapState->environmentAnimationSubstate == 0) {
        mapState->environmentAnimationSubstate = substate;
    }
}

void UpdateFallingWheelPieces(OMOBJ* wheelObject) {
    Vec modelPosition;
    RoundScoreState* roundState;
    WheelMapState* mapState;
    HU3DMODEL* fallingModel;
    HU3DMODEL* fallingModelData;
    s32 roundIndex;
    s32 pieceIndex;

    roundState = &roundScoreState;
    mapState = &wheelMapState;
    roundIndex = roundScoreState.currentRoundIndex - 1;
    switch (mapState->environmentAnimationSubstate) {
        case 1:
            mapState->environmentAnimationSubstate = 2;
            if (roundIndex < 0) {
                mapState->environmentAnimationSubstate = 0;
                break;
            }
            for (pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
                fallingModel = &Hu3DData[wheelObject->mdlId[pieceIndex + roundIndex * 3 + 2]];
                mapState->fallStartPosition[pieceIndex].x = fallingModel->pos.x;
                mapState->fallStartPosition[pieceIndex].y = fallingModel->pos.y;
                mapState->fallStartPosition[pieceIndex].z = fallingModel->pos.z;
                mapState->fallTargetPosition[pieceIndex].x = mapState->fallStartPosition[pieceIndex].x + rand8() % 50;
                mapState->fallTargetPosition[pieceIndex].y = 7.5f;
                mapState->fallTargetPosition[pieceIndex].z = mapState->fallStartPosition[pieceIndex].z + (rand8() % 150 - 75);
                mapState->fallRotation[pieceIndex].x = 0.0f;
                mapState->fallRotation[pieceIndex].y = rand8() % 120;
                if (rand8() % 2 == 0) {
                    mapState->fallRotation[pieceIndex].y *= -1.0f;
                }
                mapState->fallRotation[pieceIndex].z = 5.0f + rand8() % 5;
                mapState->fallDurationFrames[pieceIndex] = rand8() % 10 + 15;
            }
            mapState->fallAnimationFrame = 0;
            break;
        case 2:
            for (pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
                fallingModelData = &Hu3DData[wheelObject->mdlId[pieceIndex + roundIndex * 3 + 2]];
                modelPosition.x = interpolateCosineEaseIn(mapState->fallStartPosition[pieceIndex].x, mapState->fallTargetPosition[pieceIndex].x, mapState->fallAnimationFrame, mapState->fallDurationFrames[pieceIndex]);
                modelPosition.y = interpolateCosineEaseIn(mapState->fallStartPosition[pieceIndex].y, mapState->fallTargetPosition[pieceIndex].y, mapState->fallAnimationFrame, mapState->fallDurationFrames[pieceIndex]);
                modelPosition.z = interpolateCosineEaseIn(mapState->fallStartPosition[pieceIndex].z, mapState->fallTargetPosition[pieceIndex].z, mapState->fallAnimationFrame, mapState->fallDurationFrames[pieceIndex]);
                Hu3DModelPosSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 2], modelPosition.x, modelPosition.y, modelPosition.z);
                if (mapState->fallAnimationFrame <= mapState->fallDurationFrames[pieceIndex]) {
                    fallingModelData->rot.y = approachValueOverDuration(fallingModelData->rot.y, mapState->fallRotation[pieceIndex].y, 15.0f);
                    fallingModelData->rot.z = approachValueOverDuration(fallingModelData->rot.z, 90.0f, 15.0f);
                } else {
                    fallingModelData->rot.z = approachValueOverDuration(fallingModelData->rot.z, 0.0f, 3.0f);
                }
            }
            mapState->fallAnimationFrame++;
            if (mapState->fallAnimationFrame >= 60) {
                mapState->fallAnimationFrame = 0;
                mapState->environmentAnimationSubstate = 3;
            }
            break;
    }
}

s32 ClassifyPieceLanding(float angle, float distanceFromCenter) {
    s32 angleResultTable[] = { 90, 5, 70, 30, 50, 1, 100, 10 };
    s32 landingResult;

    if (angle >= 0.0f && angle < 45.0f) {
        landingResult = angleResultTable[0];
    } else if (angle >= 45.0f && angle < 90.0f) {
        landingResult = angleResultTable[1];
    } else if (angle >= 90.0f && angle < 135.0f) {
        landingResult = angleResultTable[2];
    } else if (angle >= 135.0f && angle < 180.0f) {
        landingResult = angleResultTable[3];
    } else if (angle >= 180.0f && angle < 225.0f) {
        landingResult = angleResultTable[4];
    } else if (angle >= 225.0f && angle < 270.0f) {
        landingResult = angleResultTable[5];
    } else if (angle >= 270.0f && angle < 315.0f) {
        landingResult = angleResultTable[6];
    } else if (angle >= 315.0f && angle < 360.0f) {
        landingResult = angleResultTable[7];
    }
    if (distanceFromCenter > 75.0f) {
        landingResult = 0;
    }
    if (distanceFromCenter < 25.0f) {
        landingResult = -999;
    }
    return landingResult;
}

void UpdatePieceLandingResult(s32 pieceIndex) {
    Vec radialOffset;
    WheelMapState* mapState;

    mapState = &wheelMapState;
    radialOffset.x = -675.0f;
    radialOffset.y = mapState->piecePosition[pieceIndex].y - 175.0f;
    radialOffset.z = mapState->piecePosition[pieceIndex].z - 500.0f;
    mapState->pieceDistance[pieceIndex] = mapState->pieceOffset[pieceIndex] + sqrtf(radialOffset.y * radialOffset.y + radialOffset.z * radialOffset.z);
    mapState->pieceAngle[pieceIndex] = mapState->wheelAngle - mapState->wheelSpeed * (mapState->pieceMovementDuration[pieceIndex] + 2) + atan2d(radialOffset.y, radialOffset.z);
    if (mapState->pieceAngle[pieceIndex] >= 360.0f) {
        mapState->pieceAngle[pieceIndex] -= 360.0f;
    } else if (mapState->pieceAngle[pieceIndex] < 0.0f) {
        mapState->pieceAngle[pieceIndex] += 360.0f;
    }
    if (mapState->pieceAngle[pieceIndex] >= 360.0f) {
        mapState->pieceAngle[pieceIndex] -= 360.0f;
    } else if (mapState->pieceAngle[pieceIndex] < 0.0f) {
        mapState->pieceAngle[pieceIndex] += 360.0f;
    }
    if (mapState->pieceAngle[pieceIndex] >= 360.0f) {
        mapState->pieceAngle[pieceIndex] -= 360.0f;
    } else if (mapState->pieceAngle[pieceIndex] < 0.0f) {
        mapState->pieceAngle[pieceIndex] += 360.0f;
    }
    mapState->pieceLandingResult[pieceIndex] = ClassifyPieceLanding(mapState->pieceAngle[pieceIndex], mapState->pieceDistance[pieceIndex]);
}

void RandomizePieceRingOffset(s32 pieceIndex) {
    WheelMapState* mapState;

    mapState = &wheelMapState;
    if ((mapState->pieceDistance[pieceIndex] >= 23.0f && mapState->pieceDistance[pieceIndex] <= 27.0f)
        || (mapState->pieceDistance[pieceIndex] >= 73.0f && mapState->pieceDistance[pieceIndex] <= 77.0f))
    {
        mapState->pieceOffset[pieceIndex] = 5.0f;
        if (rand8() % 2 == 0) {
            mapState->pieceOffset[pieceIndex] *= -1.0f;
        }
    }
}

void AdjustPieceMovementDelay(s32 pieceIndex) {
    WheelMapState* mapState;
    float pieceAngle;

    mapState = &wheelMapState;
    pieceAngle = mapState->pieceAngle[pieceIndex];
    if ((pieceAngle >= -2.0f && pieceAngle < 2.0f)
        || (pieceAngle >= 43.0f && pieceAngle < 47.0f)
        || (pieceAngle >= 88.0f && pieceAngle < 92.0f)
        || (pieceAngle >= 133.0f && pieceAngle < 137.0f)
        || (pieceAngle >= 178.0f && pieceAngle < 182.0f)
        || (pieceAngle >= 223.0f && pieceAngle < 227.0f)
        || (pieceAngle >= 268.0f && pieceAngle < 272.0f)
        || (pieceAngle >= 313.0f && pieceAngle < 317.0f))
    {
        mapState->pieceMovementDuration[pieceIndex]++;
    }
}

s32 ValidateRoundScore(void) {
    WheelMapState* mapState;
    s32 combinedLandingResult;
    s32 pieceIndex;

    combinedLandingResult = 0;
    mapState = &wheelMapState;
    for (pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
        combinedLandingResult += mapState->pieceLandingResult[pieceIndex];
    }
    if (combinedLandingResult < 0) {
        return 1;
    }
    if (mapState->targetScore == combinedLandingResult) {
        for (pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
            mapState->pieceMovementDuration[pieceIndex] += 3;
            UpdatePieceLandingResult(pieceIndex);
        }
        return 0;
    }
    if (mapState->targetScore > combinedLandingResult) {
        mapState->targetScore = combinedLandingResult;
    }
    return 1;
}

s32 AreAllPiecesClear(void) {
    Vec radialOffset;
    float distanceFromWheelCenter;
    WheelMapState* mapState;
    s32 allPiecesClear;
    s32 pieceIndex;

    allPiecesClear = 1;
    mapState = &wheelMapState;
    for (pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
        radialOffset.x = -675.0f;
        radialOffset.y = mapState->piecePosition[pieceIndex].y - 175.0f;
        radialOffset.z = mapState->piecePosition[pieceIndex].z - 500.0f;
        distanceFromWheelCenter = sqrtf(radialOffset.y * radialOffset.y + radialOffset.z * radialOffset.z);
        if (distanceFromWheelCenter <= 30.0f) {
            allPiecesClear = 0;
        }
    }
    return allPiecesClear;
}

void SetAllPieceStates(s32 pieceState) {
    WheelMapState* mapState;
    s32 pieceIndex;

    mapState = &wheelMapState;
    for (pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
        mapState->pieceState[pieceIndex] = pieceState;
    }
}

void UpdateWheelPieces(OMOBJ* wheelObject) {
    Vec radialOffset;
    float modelScale;
    float modelOpacity;
    WheelMapState* mapState;
    s32 roundIndex;
    s32 pieceIndex;

    mapState = &wheelMapState;
    roundIndex = roundScoreState.activeRoundCount;
    for (pieceIndex = 0; pieceIndex < 3; pieceIndex++) {
        switch (mapState->pieceState[pieceIndex]) {
            case 1:
                mapState->pieceStartPosition[pieceIndex].x = mapState->piecePosition[pieceIndex].x = -675.0f;
                mapState->pieceStartPosition[pieceIndex].y = mapState->piecePosition[pieceIndex].y = 175.0f;
                mapState->pieceStartPosition[pieceIndex].z = mapState->piecePosition[pieceIndex].z = 500.0f;
                mapState->pieceFrame[pieceIndex] = 0;
                mapState->pieceTargetPosition[pieceIndex].x = 0.0f;
                mapState->pieceTargetPosition[pieceIndex].y = -325.0f;
                mapState->pieceTargetPosition[pieceIndex].z = 500.0f;
                mapState->pieceRotation[pieceIndex].x = mapState->pieceRotation[pieceIndex].y = mapState->pieceRotation[pieceIndex].z = 0.0f;
                mapState->pieceMovementFrame[pieceIndex] = 0;
                mapState->pieceMovementDuration[pieceIndex] = pieceIndex + 15;
                mapState->pieceDistance[pieceIndex] = mapState->pieceRadius[pieceIndex] = 0.0f;
                mapState->pieceAngle[pieceIndex] = mapState->alternatePieceAngle[pieceIndex] = 0.0f;
                mapState->pieceLandingResult[pieceIndex] = mapState->pieceCollisionResult[pieceIndex] = 0;
                mapState->pieceOffset[pieceIndex] = 0.0f;
                Hu3DModelPosSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 2], mapState->pieceTargetPosition[pieceIndex].x, mapState->pieceTargetPosition[pieceIndex].y, mapState->pieceTargetPosition[pieceIndex].z);
                Hu3DModelRotSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 2], 0.0f, 0.0f, 0.0f);
                /* fallthrough */
            case 2:
                mapState->pieceState[pieceIndex] = 2;
                modelScale = interpolateSineEaseOut(1.5f, 0.5f, mapState->pieceFrame[pieceIndex], 15.0f);
                modelOpacity = interpolateSineEaseOut(0.0f, 1.0f, mapState->pieceFrame[pieceIndex], 10.0f);
                Hu3DModelAttrReset(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 14], 1);
                Hu3DModelScaleSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 14], modelScale, modelScale, modelScale);
                Hu3DModelTPLvlSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 14], modelOpacity);
                Hu3DModelPosSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 14], mapState->piecePosition[pieceIndex].x, mapState->piecePosition[pieceIndex].y, mapState->piecePosition[pieceIndex].z);
                if (mapState->pieceFrame[pieceIndex]++ >= 20) {
                    mapState->pieceFrame[pieceIndex] = 0;
                    mapState->pieceState[pieceIndex] = 3;
                }
                break;
            case 3:
                if (mapState->pieceFrame[pieceIndex]++ >= 30) {
                    mapState->pieceFrame[pieceIndex] = 0;
                    modelOpacity = 100.0f - pieceIndex * 15;
                    modelScale = 2.0f * (rand8() % 180);
                    mapState->pieceStartPosition[pieceIndex].x = -675.0f;
                    mapState->pieceStartPosition[pieceIndex].y = 175.0 + modelOpacity * sind(modelScale);
                    mapState->pieceStartPosition[pieceIndex].z = 500.0 + modelOpacity * cosd(modelScale);
                }
                mapState->piecePosition[pieceIndex].x = mapState->pieceStartPosition[pieceIndex].x;
                mapState->piecePosition[pieceIndex].y = approachValueOverDuration(mapState->piecePosition[pieceIndex].y, mapState->pieceStartPosition[pieceIndex].y, 30.0f);
                mapState->piecePosition[pieceIndex].z = approachValueOverDuration(mapState->piecePosition[pieceIndex].z, mapState->pieceStartPosition[pieceIndex].z, 30.0f);
                Hu3DModelPosSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 14], mapState->piecePosition[pieceIndex].x, mapState->piecePosition[pieceIndex].y, mapState->piecePosition[pieceIndex].z);
                break;
            case 4:
                mapState->pieceState[pieceIndex] = 100;
                UpdatePieceLandingResult(pieceIndex);
                RandomizePieceRingOffset(pieceIndex);
                AdjustPieceMovementDelay(pieceIndex);
                UpdatePieceLandingResult(pieceIndex);
                mapState->pieceSettlingState = 0;
                HuAudFXPlay(0x6DC);
                /* fallthrough */
            case 100:
                mapState->pieceState[pieceIndex] = 5;
                if (mapState->pieceSettlingState == 1) {
                    mapState->pieceState[pieceIndex] = 5;
                }
                if (pieceIndex == 2) {
                    while (ValidateRoundScore() == 0) {
                    }
                    mapState->pieceSettlingState = 1;
                }
                Hu3DModelAttrReset(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 2], 1);
                break;
            case 5:
                mapState->pieceTargetPosition[pieceIndex].x = interpolateLinearClamped(0.0f, -675.0f, mapState->pieceMovementFrame[pieceIndex], mapState->pieceMovementDuration[pieceIndex]);
                mapState->pieceTargetPosition[pieceIndex].y = interpolateLinearClamped(175.0f, mapState->piecePosition[pieceIndex].y, mapState->pieceMovementFrame[pieceIndex], mapState->pieceMovementDuration[pieceIndex]);
                mapState->pieceTargetPosition[pieceIndex].z = interpolateLinearClamped(500.0f, mapState->piecePosition[pieceIndex].z, mapState->pieceMovementFrame[pieceIndex], mapState->pieceMovementDuration[pieceIndex]);
                mapState->pieceRotation[pieceIndex].x -= 360.0f / mapState->pieceMovementDuration[pieceIndex];
                if (mapState->pieceRotation[pieceIndex].x >= 360.0f) {
                    mapState->pieceRotation[pieceIndex].x -= 360.0f;
                } else if (mapState->pieceRotation[pieceIndex].x < 0.0f) {
                    mapState->pieceRotation[pieceIndex].x += 360.0f;
                }
                Hu3DModelPosSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 2], mapState->pieceTargetPosition[pieceIndex].x, mapState->pieceTargetPosition[pieceIndex].y, mapState->pieceTargetPosition[pieceIndex].z);
                Hu3DModelRotSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 2], mapState->pieceRotation[pieceIndex].x, 0.0f, 0.0f);
                if (mapState->pieceMovementFrame[pieceIndex]++ >= mapState->pieceMovementDuration[pieceIndex]) {
                    mapState->pieceState[pieceIndex] = 6;
                    mapState->pieceFrame[pieceIndex] = 0;
                }
                break;
            case 6:
                mapState->pieceState[pieceIndex] = 7;
                radialOffset.x = -675.0f;
                radialOffset.y = mapState->pieceTargetPosition[pieceIndex].y - 175.0f;
                radialOffset.z = mapState->pieceTargetPosition[pieceIndex].z - 500.0f;
                mapState->pieceRadius[pieceIndex] = mapState->pieceOffset[pieceIndex] + sqrtf(radialOffset.y * radialOffset.y + radialOffset.z * radialOffset.z);
                mapState->alternatePieceAngle[pieceIndex] = mapState->wheelAngle + atan2d(radialOffset.y, radialOffset.z);
                if (mapState->alternatePieceAngle[pieceIndex] >= 360.0f) {
                    mapState->alternatePieceAngle[pieceIndex] -= 360.0f;
                } else if (mapState->alternatePieceAngle[pieceIndex] < 0.0f) {
                    mapState->alternatePieceAngle[pieceIndex] += 360.0f;
                }
                mapState->pieceRotation[pieceIndex].y = atan2d(radialOffset.y, radialOffset.z);
                mapState->pieceCollisionResult[pieceIndex] = ClassifyPieceLanding(mapState->alternatePieceAngle[pieceIndex], mapState->pieceRadius[pieceIndex]);
                if (mapState->pieceCollisionResult[pieceIndex] >= 0) {
                    HuAudFXPlay(0x6DD);
                } else {
                    HuAudFXPlay(0x6DF);
                }
                SetPrimaryPlayerMotion(7);
                SetWheelEnvironmentState(2);
                omVibrate(GetGamePlayerIndex(roundScoreState.activeRoundCount), 12, 6, 6);
                /* fallthrough */
            case 7:
                modelScale = interpolateSineEaseOut(0.5f, 1.5f, mapState->pieceFrame[pieceIndex], 15.0f);
                modelOpacity = interpolateSineEaseOut(1.0f, 0.0f, mapState->pieceFrame[pieceIndex], 10.0f);
                Hu3DModelScaleSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 14], modelScale, modelScale, modelScale);
                Hu3DModelTPLvlSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 14], modelOpacity);
                if (mapState->pieceFrame[pieceIndex]++ >= 20) {
                    Hu3DModelAttrSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 14], 1);
                }
                mapState->pieceTargetPosition[pieceIndex].x = -675.0f;
                mapState->pieceTargetPosition[pieceIndex].y = 175.0 + mapState->pieceRadius[pieceIndex] * sind(mapState->pieceRotation[pieceIndex].y);
                mapState->pieceTargetPosition[pieceIndex].z = 500.0 + mapState->pieceRadius[pieceIndex] * cosd(mapState->pieceRotation[pieceIndex].y);
                Hu3DModelPosSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 2], mapState->pieceTargetPosition[pieceIndex].x, mapState->pieceTargetPosition[pieceIndex].y, mapState->pieceTargetPosition[pieceIndex].z);
                Hu3DModelRotSet(wheelObject->mdlId[pieceIndex + roundIndex * 3 + 2], mapState->pieceRotation[pieceIndex].x, 0.0f, 0.0f);
                mapState->pieceRotation[pieceIndex].z = mapState->pieceRotation[pieceIndex].y + mapState->wheelAngle;
                if (mapState->pieceRotation[pieceIndex].z >= 360.0f) {
                    mapState->pieceRotation[pieceIndex].z -= 360.0f;
                } else if (mapState->pieceRotation[pieceIndex].z < 0.0f) {
                    mapState->pieceRotation[pieceIndex].z += 360.0f;
                }
                mapState->pieceRotation[pieceIndex].x -= mapState->wheelSpeed;
                if (mapState->pieceRotation[pieceIndex].x >= 360.0f) {
                    mapState->pieceRotation[pieceIndex].x -= 360.0f;
                } else if (mapState->pieceRotation[pieceIndex].x < 0.0f) {
                    mapState->pieceRotation[pieceIndex].x += 360.0f;
                }
                mapState->pieceRotation[pieceIndex].y += mapState->wheelSpeed;
                if (mapState->pieceRotation[pieceIndex].y >= 360.0f) {
                    mapState->pieceRotation[pieceIndex].y -= 360.0f;
                } else if (mapState->pieceRotation[pieceIndex].y < 0.0f) {
                    mapState->pieceRotation[pieceIndex].y += 360.0f;
                }
                break;
        }
    }
}

void UpdateWheelObjects(OMOBJ* wheelObject) {
    UpdateWheelEntrance(wheelObject);
    UpdateWheelRotation(wheelObject);
    UpdateWheelPieces(wheelObject);
    UpdateFallingWheelPieces(wheelObject);
}

void InitializeWheelMap(WheelMapState* mapState) {
    OMOBJ* playerState;
    s32 i;
    s32 j;

    playerState = mapState->mapObject;
    Hu3DModelRotSet(playerState->mdlId[0], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(playerState->mdlId[0], 1);
    Hu3DModelPosSet(playerState->mdlId[1], 0.0f, 0.0f, -250.0f);
    Hu3DModelRotSet(playerState->mdlId[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(playerState->mdlId[1], 1);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            Hu3DModelPosSet(playerState->mdlId[i + 2], 0.0f, -325.0f, 500.0f);
            Hu3DModelAttrSet(playerState->mdlId[j + i * 3 + 2], 1);
            Hu3DModelAttrSet(playerState->mdlId[j + i * 3 + 14], 1);
        }
    }
    mapState->wheelAngle = 0.0f;
    for (i = 0; i < 3; i++) {
        mapState->pieceState[i] = 0;
        mapState->pieceStartPosition[i].x = mapState->piecePosition[i].x = -675.0f;
        mapState->pieceStartPosition[i].y = mapState->piecePosition[i].y = 175.0f;
        mapState->pieceStartPosition[i].z = mapState->piecePosition[i].z = 500.0f;
        mapState->pieceFrame[i] = 0;
        mapState->pieceTargetPosition[i].x = 0.0f;
        mapState->pieceTargetPosition[i].y = -325.0f;
        mapState->pieceTargetPosition[i].z = 500.0f;
        mapState->pieceRotation[i].x = mapState->pieceRotation[i].y = mapState->pieceRotation[i].z = 0.0f;
        mapState->pieceMovementFrame[i] = 0;
        mapState->pieceMovementDuration[i] = 0;
        mapState->pieceDistance[i] = mapState->pieceRadius[i] = 0.0f;
        mapState->pieceAngle[i] = mapState->alternatePieceAngle[i] = 0.0f;
        mapState->pieceLandingResult[i] = mapState->pieceCollisionResult[i] = 0;
    }
    mapState->targetScore = 9999;
    playerState->work[0] = playerState->work[1] = playerState->work[2] = playerState->work[3] = 0;
    mapState->entranceAnimationState = mapState->environmentAnimationState = mapState->unreferencedMapState0C = mapState->environmentAnimationSubstate = 0;
    mapState->fallDurationFrames[0] = mapState->fallDurationFrames[1] = mapState->fallDurationFrames[2] = mapState->fallAnimationFrame = 0;
}

void CreateWheelMap(OMOBJ* mapObject) {
    WheelMapState* mapState;
    s32 i;
    s32 j;

    mapState = &wheelMapState;
    mapObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 24));
    mapObject->mtnId[0] = Hu3DMotionIDGet(mapObject->mdlId[0]);
    Hu3DModelRotSet(mapObject->mdlId[0], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(mapObject->mdlId[0], 1U);
    Hu3DModelLayerSet(mapObject->mdlId[0], 1);
    Hu3DModelShadowSet(mapObject->mdlId[0]);
    mapObject->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 25));
    mapObject->mtnId[1] = Hu3DMotionIDGet(mapObject->mdlId[1]);
    mapObject->mtnId[2] = Hu3DJointMotionFile(mapObject->mdlId[1], DATA_MAKE_NUM(DATADIR_M435, 35));
    Hu3DModelPosSet(mapObject->mdlId[1], 0.0f, 0.0f, -250.0f);
    Hu3DModelRotSet(mapObject->mdlId[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(mapObject->mdlId[1], 1);
    Hu3DModelLayerSet(mapObject->mdlId[1], 1);
    Hu3DModelShadowSet(mapObject->mdlId[1]);
    Hu3DModelHookSet(mapObject->mdlId[1], "m435a02-itemfook_d", mapObject->mdlId[0]);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3; j++) {
            mapObject->mdlId[j + i * 3 + 2] = Hu3DModelCreateFile(characterModelData[GetPlayerCharacterId(i)][0]);
            Hu3DModelAttrSet(mapObject->mdlId[j + i * 3 + 2], 1);
            Hu3DModelShadowSet(mapObject->mdlId[j + i * 3 + 2]);
            mapObject->mdlId[j + i * 3 + 14] = Hu3DModelCreateFile(characterModelData[GetPlayerCharacterId(i)][1]);
            Hu3DModelRotSet(mapObject->mdlId[j + i * 3 + 14], 0.0f, 90.0f, 0.0f);
            Hu3DModelAttrSet(mapObject->mdlId[j + i * 3 + 14], 1);
        }
    }
    mapObject->work[0] = mapObject->work[1] = mapObject->work[2] = mapObject->work[3] = 0;
    mapState->entranceAnimationState = mapState->environmentAnimationState = mapState->unreferencedMapState0C = mapState->environmentAnimationSubstate = 0;
    mapState->fallDurationFrames[0] = mapState->fallDurationFrames[1] = mapState->fallDurationFrames[2] = mapState->fallAnimationFrame = 0;
    mapObject->objFunc = UpdateWheelObjects;
}

void SetUiAnimationState(s32 animationState) {
    GameUiState* mapState;

    mapState = &gameUiState;
    mapState->uiObject->work[0] = animationState;
}

void UpdateUiAnimation(OMOBJ* uiObject) {
    switch (uiObject->work[0]) {
        case 1:
            uiObject->work[0] = 0;
            SetModelMotion(uiObject, 0, 1, 30, 1);
            break;
        case 2:
            uiObject->work[0] = 0;
            SetModelMotion(uiObject, 0, 2, 10, 0);
            break;
        case 3:
            uiObject->work[0] = 0;
            SetModelMotion(uiObject, 0, 3, 10, 0);
            break;
    }
}

void InitializeUiObject(GameUiState* uiState) {
    OMOBJ* mapObject;

    mapObject = uiState->uiObject;
    omSetTra(mapObject, 0.0f, 37.0f, -565.0f);
    omSetSca(mapObject, 1.5f, 1.5f, 1.5f);
    SetModelMotion(mapObject, 0, 1, 1, 1);
    mapObject->work[0] = mapObject->work[1] = mapObject->work[2] = mapObject->work[3] = 0;
    uiState->unreferencedUiState04 = uiState->unreferencedUiState08 = uiState->unreferencedUiState0C = uiState->unreferencedUiState10 = 0;
}

void CreateUiObject(OMOBJ* uiObject) {
    GameUiState* mapState;

    mapState = &gameUiState;
    uiObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 48));
    uiObject->mtnId[0] = Hu3DJointMotionFile(uiObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 53));
    uiObject->mtnId[1] = Hu3DJointMotionFile(uiObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 49));
    uiObject->mtnId[2] = Hu3DJointMotionFile(uiObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 50));
    uiObject->mtnId[3] = Hu3DJointMotionFile(uiObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 51));
    uiObject->mtnId[4] = Hu3DJointMotionFile(uiObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 52));
    omSetTra(uiObject, 0.0f, 37.0f, -565.0f);
    omSetSca(uiObject, 1.5f, 1.5f, 1.5f);
    Hu3DModelShadowSet(uiObject->mdlId[0]);
    SetModelMotion(uiObject, 0, 1, 1, 1);
    uiObject->work[0] = uiObject->work[1] = uiObject->work[2] = uiObject->work[3] = 0;
    mapState->unreferencedUiState04 = mapState->unreferencedUiState08 = mapState->unreferencedUiState0C = mapState->unreferencedUiState10 = 0;
    uiObject->objFunc = UpdateUiAnimation;
}

void SetPlayerActorState(s32 playerIndex, s32 playerState) {
    PlayerActorState* mapObject;

    mapObject = &playerActorStates[playerIndex];
    mapObject->playerState = playerState;
}

void SetPrimaryPlayerFrame(s32 frame) {
    PlayerActorState* mapObject;

    mapObject = &playerActorStates[1];
    mapObject->playerFrame = frame;
}

void UpdatePlayerActorState(OMOBJ* playerObject) {
    PlayerActorState* playerObject;

    playerObject = &playerActorStates[playerObject->work[0]];
    switch (playerObject->playerState) {
        case 1:
            playerObject->playerState = 0;
            if (playerObject->work[0] != 0) {
                SetModelMotion(playerObject, 0, 0, 30, 1);
            } else {
                SetModelMotion(playerObject, 0, 0, 30, 1);
                SetModelMotion(playerObject, 1, 5, 30, 1);
            }
            break;
        case 2:
            playerObject->playerState = 0;
            if (playerObject->work[0] != 0) {
                SetModelMotion(playerObject, 0, 4, 10, 0);
            } else {
                SetModelMotion(playerObject, 0, 2, 10, 0);
                SetModelMotion(playerObject, 1, 5, 10, 2);
            }
            break;
        case 3:
            playerObject->playerState = 0;
            if (playerObject->work[0] != 0) {
                SetModelMotion(playerObject, 0, 9, 5, 0);
            }
            break;
    }
}

void UpdatePlayerTurnAnimation(OMOBJ* playerObject) {
    PlayerActorState* playerActionState;
    Mtx playerModelMatrix;
    Mtx mapModelMatrix;

    playerActionState = &playerActorStates[playerObject->work[0]];
    switch (playerActionState->playerFrame) {
        case 1:
        case 3:
        case 6:
            playerActionState->playerStartPosition.x = playerObject->trans.x;
            playerActionState->playerStartPosition.y = playerObject->trans.y;
            playerActionState->playerStartPosition.z = playerObject->trans.z;
            Hu3DModelObjMtxGet(wheelMapState.mapObject->mdlId[1], "435a02-point00", playerModelMatrix);
            playerActionState->playerTargetPosition.x = playerModelMatrix[0][3];
            playerActionState->playerTargetPosition.y = playerModelMatrix[1][3];
            playerActionState->playerTargetPosition.z = playerModelMatrix[2][3];
            if (playerActionState->playerFrame == 1) {
                playerActionState->playerTargetPosition.x = playerActionState->playerStartPosition.x - 30.0f;
            } else if (playerActionState->playerFrame == 6) {
                playerActionState->playerTargetPosition.x = (120.0f + playerModelMatrix[0][3]);
                playerActionState->playerTargetPosition.z = (playerModelMatrix[2][3] - 50.0f);
            }
            playerActionState->playerTimer = 0;
            playerActionState->playerFrame++;
            break;
        case 2:
            SetModelMotionIfChanged(playerObject, 0, 1, 10, 1);
            UpdateModelTransformTweenPositionAndRotation(playerObject, 0, &playerActionState->playerStartPosition, &playerActionState->playerTargetPosition, 5, playerActionState->playerTimer++, 0xF);
            if (playerActionState->playerTimer > 15) {
                playerActionState->playerFrame++;
            }
            break;
        case 4:
            if (playerActionState->playerTimer == 25) {
                SetModelMotionIfChanged(playerObject, 0, 0, 5, 1);
            } else if (playerActionState->playerTimer == 40) {
                SetModelMotionIfChanged(playerObject, 0, 5, 30, 1);
            }
            UpdateModelTransformTweenPositionAndRotation(playerObject, 0, &playerActionState->playerStartPosition, &playerActionState->playerTargetPosition, 10, playerActionState->playerTimer++, 25);
            if (playerActionState->playerTimer > 60) {
                SetWheelEntranceState(2);
                playerActionState->playerFrame++;
            }
            break;
        case 5:
            Hu3DModelObjMtxGet(wheelMapState.mapObject->mdlId[1], "435a02-point00", mapModelMatrix);
            omSetTra(playerObject, mapModelMatrix[0][3], mapModelMatrix[1][3], mapModelMatrix[2][3]);
            omSetRot(playerObject, 0.0f, Hu3DData[wheelMapState.mapObject->mdlId[1]].rot.y - 90.0f, 0.0f);
            break;
        case 7:
            SetModelMotionIfChanged(playerObject, 0, 1, 10, 1);
            UpdateModelTransformTweenPosition(playerObject, 0, &playerActionState->playerStartPosition, &playerActionState->playerTargetPosition, 5, playerActionState->playerTimer++, 15);
            UpdateModelRotationTween(playerObject, 0, 90.0f, 5);
            if (playerActionState->playerTimer > 15) {
                playerActionState->playerFrame++;
            }
            break;
        case 8:
            SetModelMotionIfChanged(playerObject, 0, 0, 5, 1);
            activePlayerIndex = 1;
            playerActionState->playerFrame = playerActionState->playerTimer = 0;
            break;
    }
}

void SetPrimaryPlayerAction(s32 actionState) {
    PlayerActorState* mapObject;

    mapObject = &playerActorStates[1];
    mapObject->playerAction = actionState;
}

void UpdatePlayerAction(OMOBJ* playerObject) {
    PlayerActorState* playerActionState;
    Mtx mapModelMatrix;

    playerActionState = &playerActorStates[playerObject->work[0]];
    switch (playerActionState->playerAction) {
        case 1:
        case 3:
        case 5:
            SetModelMotionIfChanged(playerActionState->playerObject, 0, 1, 10, 1);
            playerActionState->playerStartPosition.x = playerObject->trans.x;
            playerActionState->playerStartPosition.y = playerObject->trans.y;
            playerActionState->playerStartPosition.z = playerObject->trans.z;
            if (playerActionState->playerAction == 1) {
                playerActionState->playerTargetPosition.x = -400.0f;
                playerActionState->playerTargetPosition.y = playerObject->trans.y;
                playerActionState->playerTargetPosition.z = -310.0f;
            } else if (playerActionState->playerAction == 3) {
                playerActionState->playerTargetPosition.x = 400.0f;
                playerActionState->playerTargetPosition.y = playerObject->trans.y;
                playerActionState->playerTargetPosition.z = playerObject->trans.z;
            } else if (playerActionState->playerAction == 5) {
                playerActionState->playerTargetPosition.x = 400.0f;
                playerActionState->playerTargetPosition.y = 37.0f;
                playerActionState->playerTargetPosition.z = -425.0f;
            }
            playerActionState->playerTimer = 0;
            playerActionState->playerAction++;
            break;
        case 2:
            UpdateModelTransformTweenPositionAndRotation(playerObject, 0, &playerActionState->playerStartPosition, &playerActionState->playerTargetPosition, 10, playerActionState->playerTimer++, 90);
            if (playerActionState->playerTimer > 90) {
                playerActionState->playerAction++;
            }
            break;
        case 4:
            UpdateModelTransformTweenPositionAndRotation(playerObject, 0, &playerActionState->playerStartPosition, &playerActionState->playerTargetPosition, 10, playerActionState->playerTimer++, 90);
            if (playerActionState->playerTimer > 90) {
                playerActionState->playerAction++;
            }
            break;
        case 6:
            UpdateModelTransformTweenPositionAndRotation(playerObject, 0, &playerActionState->playerStartPosition, &playerActionState->playerTargetPosition, 10, playerActionState->playerTimer++, 15);
            if (playerActionState->playerTimer > 15) {
                playerActionState->playerAction++;
                playerActionState->playerTimer = 0;
            }
            break;
        case 7:
            UpdateModelRotationTween(playerObject, 0, -15.0f, 5);
            if (playerActionState->playerTimer++ > 15) {
                playerActionState->playerAction++;
            }
            break;
        case 8:
            SetModelMotionIfChanged(playerActionState->playerObject, 0, 0, 10, 1);
            playerActionState->playerAction = 0;
            break;
        case 10:
            playerActionState->playerAction = 0;
            Hu3DModelObjMtxGet(wheelMapState.mapObject->mdlId[1], "435a02-point00", mapModelMatrix);
            playerObject->trans.x = mapModelMatrix[0][3];
            playerObject->trans.y = mapModelMatrix[1][3];
            playerObject->trans.z = mapModelMatrix[2][3];
            break;
    }
}

void SetPrimaryPlayerMotion(s32 motionIndex) {
    PlayerActorState* mapObject;

    mapObject = &playerActorStates[1];
    SetModelMotionIfChanged(mapObject->playerObject, 0, motionIndex, 1, 0);
}

void UpdatePlayerActor(OMOBJ* playerObject) {
    PlayerActorState* playerState;

    playerState = &playerActorStates[playerObject->work[0]];
    if (playerState->playerState != 0) {
        UpdatePlayerActorState(playerObject);
    }
    if (playerState->playerFrame != 0) {
        UpdatePlayerTurnAnimation(playerObject);
    }
    UpdatePlayerAction(playerObject);
}

void InitializePlayerActor(PlayerActorState* playerState) {
    OMOBJ* playerObject;

    playerObject = playerState->playerObject;
    if (playerObject->work[0] != 0) {
        omSetTra(playerObject, 400.0f, 37.0f, -425.0f);
        omSetRot(playerObject, 0.0f, -15.0f, 0.0f);
        SetModelMotion(playerObject, 0, 0, 1, 1);
        ResetMotionTarget(&playerState->turnTracker, 0);
        ReleaseMotionTarget(&playerState->turnTracker, -1);
        ClearMotionTargetModel(&playerState->turnTracker, -1);
    } else {
        omSetTra(playerObject, -400.0f, 37.0f, -425.0f);
        SetModelMotion(playerObject, 0, 0, 1, 1);
        SetModelMotion(playerObject, 1, 5, 1, 1);
    }
    playerObject->work[1] = playerObject->work[2] = playerObject->work[3] = 0;
    playerState->playerState = playerState->playerFrame = playerState->playerAction = playerState->playerSubstate = 0;
    playerState->playerTimer = playerState->playerTargetIndex = playerState->playerMovementState = playerState->playerActionFrame = 0;
    activePlayerIndex = 0;
}

void CreatePlayerActor(OMOBJ* playerObject) {
    PlayerActorState* playerState;
    s32 playerIndex;

    playerIndex = playerObject->work[0];
    playerState = &playerActorStates[playerIndex];
    playerObject->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 56));
    if (playerObject->work[0] != 0) {
        playerObject->mtnId[0] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 57));
        playerObject->mtnId[1] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 58));
        playerObject->mtnId[2] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 62));
        playerObject->mtnId[3] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 63));
        playerObject->mtnId[4] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 64));
        playerObject->mtnId[5] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 65));
        playerObject->mtnId[6] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 66));
        playerObject->mtnId[7] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 67));
        playerObject->mtnId[8] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 68));
        playerObject->mtnId[9] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 69));
        playerObject->mtnId[10] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 70));
        playerObject->mtnId[11] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 71));
        playerObject->mtnId[12] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 72));
        omSetTra(playerObject, 400.0f, 37.0f, -425.0f);
        omSetRot(playerObject, 0.0f, -15.0f, 0.0f);
        SetModelMotion(playerObject, 0, 0, 1, 1);
        RunMotionTargetProcess(objectManager, &playerState->turnTracker, playerObject->mdlId[0], "g007m0-skl_at", 0x100, 0);
        ResetMotionTarget(&playerState->turnTracker, 0);
        ReleaseMotionTarget(&playerState->turnTracker, -1);
        ClearMotionTargetModel(&playerState->turnTracker, -1);
    } else {
        playerObject->mtnId[0] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 59));
        playerObject->mtnId[1] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 60));
        playerObject->mtnId[2] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 61));
        playerObject->mtnId[3] = Hu3DJointMotionFile(playerObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 73));
        omSetTra(playerObject, -400.0f, 37.0f, -425.0f);
        SetModelMotion(playerObject, 0, 0, 1, 1);
        playerObject->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 74));
        playerObject->mtnId[5] = Hu3DJointMotionFile(playerObject->mdlId[1], DATA_MAKE_NUM(DATADIR_M435, 75));
        playerObject->mtnId[6] = Hu3DJointMotionFile(playerObject->mdlId[1], DATA_MAKE_NUM(DATADIR_M435, 76));
        Hu3DModelHookSet(playerObject->mdlId[0], "g007m0-itemhook_r", playerObject->mdlId[1]);
        Hu3DModelLayerSet(playerObject->mdlId[0], 2);
        Hu3DModelLayerSet(playerObject->mdlId[1], 2);
        SetModelMotion(playerObject, 1, 5, 1, 1);
    }
    Hu3DModelShadowSet(playerObject->mdlId[0]);
    playerObject->work[1] = playerObject->work[2] = playerObject->work[3] = 0;
    playerState->playerState = playerState->playerFrame = playerState->playerAction = playerState->playerSubstate = 0;
    playerState->playerTimer = playerState->playerTargetIndex = playerState->playerMovementState = playerState->playerActionFrame = 0;
    activePlayerIndex = 0;
    playerObject->objFunc = UpdatePlayerActor;
}

s32 GetGamePlayerIndex(s32 localPlayerIndex) {
    PlayerCharacterState* playerState;

    playerState = &playerCharacterStates[localPlayerIndex];
    return playerState->playerIndex;
}

s32 CountCpuPlayers(void) {
    s32 activePlayerCount;
    s32 playerIndex;

    activePlayerCount = 0;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerCharacterStates[playerIndex].isCpu != 0) {
            activePlayerCount++;
        }
    }
    return activePlayerCount;
}

s32 IsPlayerCpu(s32 playerIndex) {
    if (playerCharacterStates[playerIndex].isCpu != 0) {
        return 1;
    } else {
        return 0;
    }
}

s32 IsPlayerConfirmPressed(s32 playerIndex) {
    if (HuPadBtnDown[playerCharacterStates[playerIndex].controllerIndex] == 0x100) {
        return 1;
    } else {
        return 0;
    }
}

s32 GetPlayerCharacterId(s32 playerIndex) {
    return playerCharacterStates[playerIndex].characterId;
}

void InitializePlayerCharacterStates(void) {
    PlayerCharacterState* playerState;
    s32 playerOrder[4];
    s32 playerIndex;

    playerOrder[0] = GWPlayerCurrGet();
    for (playerIndex = 1; playerIndex < 4; playerIndex++) {
        playerOrder[playerIndex] = playerOrder[playerIndex - 1] + 1;
        if (playerOrder[playerIndex] >= 4) {
            playerOrder[playerIndex] = 0;
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerState = &playerCharacterStates[playerIndex];
        playerState->playerIndex = playerOrder[playerIndex];
        playerState->playerGroup = GWPlayerCfg[playerState->playerIndex].group;
        playerState->isCpu = GWPlayerCfg[playerState->playerIndex].iscom;
        playerState->aiDifficulty = GWPlayerCfg[playerState->playerIndex].diff;
        playerState->characterId = GWPlayerCfg[playerState->playerIndex].character;
        playerState->controllerIndex = GWPlayerCfg[playerState->playerIndex].pad_idx;
    }
}

void MoveCharacterTowardTarget(OMOBJ* characterObject, Vec targetPosition, float targetRotation, float movementSpeed, float stopDistance, float rotationSmoothingFrames, s32 useFastWalk) {
    PlayerCharacterState* playerState;
    float deltaX;
    float deltaZ;
    float targetRotation;
    float distanceToTarget;

    playerState = &playerCharacterStates[characterObject->work[0]];
    deltaX = targetPosition.x - characterObject->trans.x;
    deltaZ = characterObject->trans.z - targetPosition.z;
    targetRotation = 90.0 + atan2d(deltaZ, deltaX);
    distanceToTarget = sqrtf(deltaX * deltaX + deltaZ * deltaZ);
    if (distanceToTarget <= stopDistance) {
        deltaX = deltaZ = 0.0f;
    } else {
        deltaX = 72.0f * (deltaX / distanceToTarget);
        deltaZ = 72.0f * (deltaZ / distanceToTarget);
    }
    distanceToTarget = movementSpeed * (sqrtf(deltaX * deltaX + deltaZ * deltaZ) / 72.0f);
    if (distanceToTarget >= movementSpeed) {
        distanceToTarget = movementSpeed;
    } else if (distanceToTarget <= 1.0f) {
        deltaX = deltaZ = distanceToTarget = 0.0f;
    }
    if (deltaX != 0.0f || deltaZ != 0.0f || targetRotation >= 0.0f) {
        if (targetRotation >= 0.0f && deltaX == 0.0f && deltaZ == 0.0f) {
            targetRotation = targetRotation;
        }
        if (targetRotation >= 180.0f) {
            if (characterObject->rot.y - targetRotation >= 180.0f) {
                characterObject->rot.y -= 360.0f;
            }
            if (characterObject->rot.y - targetRotation < -180.0f) {
                characterObject->rot.y += 360.0f;
            }
        } else {
            if (characterObject->rot.y - targetRotation > 180.0f) {
                characterObject->rot.y -= 360.0f;
            }
            if (characterObject->rot.y - targetRotation <= -180.0f) {
                characterObject->rot.y += 360.0f;
            }
        }
        characterObject->rot.y = (targetRotation + characterObject->rot.y * (rotationSmoothingFrames - 1.0f)) / rotationSmoothingFrames;
    }
    characterObject->trans.x += distanceToTarget * sind(characterObject->rot.y);
    characterObject->trans.z += distanceToTarget * cosd(characterObject->rot.y);
    if (distanceToTarget >= 6.75f) {
        if (useFastWalk != 0) {
            SetModelMotionIfChanged(characterObject, 0, 5, 10, 1);
        } else {
            SetModelMotionIfChanged(characterObject, 0, 2, 10, 1);
        }
    } else {
        if (distanceToTarget > 0.0f) {
            SetModelMotionIfChanged(characterObject, 0, 2, 10, 1);
        } else {
            SetModelMotionIfChanged(characterObject, 0, 0, 10, 1);
        }
    }
}

void SetCharacterVisibilityState(s32 playerIndex, s32 visibilityState) {
    PlayerCharacterState* playerState;

    playerState = &playerCharacterStates[playerIndex];
    playerState->modelVisibilityState = visibilityState;
}

void UpdateCharacterVisibility(OMOBJ* characterObject) {
    PlayerCharacterState* playerState;

    playerState = &playerCharacterStates[characterObject->work[0]];
    switch (playerState->modelVisibilityState) {
        case 0:
            playerState->modelVisibilityState = 99;
            Hu3DModelAttrSet(characterObject->mdlId[0], 1);
            break;
        case 1:
            playerState->modelVisibilityState = 99;
            Hu3DModelAttrReset(characterObject->mdlId[0], 1);
            break;
    }
}

void SetCharacterFadeState(s32 playerIndex, s32 fadeState) {
    PlayerCharacterState* playerState;

    playerState = &playerCharacterStates[playerIndex];
    playerState->materialFadeState = fadeState;
}

void FadeCharacterMaterials(OMOBJ* characterObject) {
    PlayerCharacterState* playerState;
    HSFMATERIAL* material;
    HSFDATA* modelData;
    char** eyeBitmaps;
    s32 materialHasEye;
    s32 materialIndex, attributeIndex;
    HU3DMODEL* model;
    HSFATTRIBUTE* attribute;

    playerState = &playerCharacterStates[characterObject->work[0]];
    if (playerState->materialFadeState != 1) {
        return;
    }
    materialHasEye = 1;
    model = &Hu3DData[characterObject->mdlId[0]];
    modelData = model->hsf;
    material = modelData->material;
    if (playerState->characterId == 3) {
        for (materialIndex = 0; materialIndex < model->hsf->materialNum; materialIndex++, material++) {
            if (materialIndex == 1 || materialIndex == 2) {
                continue;
            }
            material->color[0] *= playerState->materialOpacity;
            if (material->color[0] == 0) {
                material->color[0] = 0;
            }
            material->color[1] *= playerState->materialOpacity;
            if (material->color[1] == 0) {
                material->color[1] = 0;
            }
            material->color[2] *= playerState->materialOpacity;
            if (material->color[2] == 0) {
                material->color[2] = 0;
            }
        }
    } else {
        eyeBitmaps = CharModelEyeBmpGet(playerState->characterId, 2);
        for (materialIndex = 0; materialIndex < modelData->materialNum; materialIndex++, material++) {
            materialHasEye = 1;
            for (attributeIndex = 0; attributeIndex < material->attrNum; attributeIndex++) {
                attribute = &modelData->attribute[material->attr[attributeIndex]];
                if (strcmp(eyeBitmaps[0], attribute->bitmap->name) == 0 || strcmp(eyeBitmaps[1], attribute->bitmap->name) == 0) {
                    materialHasEye = 0;
                }
            }
            if (materialHasEye == 0) {
                continue;
            }
            material->color[0] *= playerState->materialOpacity;
            if (material->color[0] == 0) {
                material->color[0] = 0;
            }
            material->color[1] *= playerState->materialOpacity;
            if (material->color[1] == 0) {
                material->color[1] = 0;
            }
            material->color[2] *= playerState->materialOpacity;
            if (material->color[2] == 0) {
                material->color[2] = 0;
            }
        }
    }
    playerState->materialOpacity -= 0.01;
    if (playerState->materialOpacity < 0.0f) {
        playerState->materialFadeState = 99;
        playerState->materialOpacity = 0.0f;
    }
}

void SetCharacterAiRoute(s32 playerIndex, s32 routeIndex) {
    PlayerCharacterState* playerState;

    playerState = &playerCharacterStates[playerIndex];
    playerState->aiRouteIndex = routeIndex;
    playerState->routeDelayFrames = 15 - routeIndex * 5;
    playerState->routeFrame = 0;
}

void UpdateCharacterAiMovement(OMOBJ* characterObject) {
    PlayerCharacterState* playerState = &playerCharacterStates[characterObject->work[0]];
    Vec routePositions[] = {
        {     0.0f, 0.0f,   0.0f },
        {   600.0f, 0.0f, 800.0f },
        {  -200.0f, 0.0f, 800.0f },
        { -1000.0f, 0.0f, 400.0f }
    };

    if (playerState->aiRouteIndex >= 1) {
        if (playerState->routeDelayFrames-- <= 0) {
            playerState->routeFrame = 1;
        }
        if (playerState->routeFrame != 0) {
            MoveCharacterTowardTarget(characterObject, routePositions[playerState->aiRouteIndex], -1.0f, 7.0f + playerState->aiRouteIndex, 30.0f, 5.0f, 1);
        }
    }
}

void UpdateCharacterObject(OMOBJ* characterObject) {
    UpdateCharacterVisibility(characterObject);
    FadeCharacterMaterials(characterObject);
    UpdateCharacterAiMovement(characterObject);
}

void InitializeCharacterState(PlayerCharacterState* playerState) {
    OMOBJ* characterObject = playerState->characterObject;
    Vec characterPositionByPlayer[] = {
        {   0.0f, 0.0f, 500.0f },
        { 200.0f, 0.0f, 300.0f },
        { 200.0f, 0.0f, 450.0f },
        { 200.0f, 0.0f, 600.0f }
    };
    s32 playerIndex;

    playerIndex = characterObject->work[0];
    omSetTra(characterObject, characterPositionByPlayer[playerIndex].x, characterPositionByPlayer[playerIndex].y, characterPositionByPlayer[playerIndex].z);
    omSetRot(characterObject, 0.0f, -90.0f, 0.0f);
    Hu3DModelAttrSet(characterObject->mdlId[0], 1);
    SetModelMotionIfChanged(characterObject, 0, 0, 0, 1);
    ResetMotionTarget(&playerState->turnTracker, 0);
    ReleaseMotionTarget(&playerState->turnTracker, -1);
    ClearMotionTargetModel(&playerState->turnTracker, -1);
    characterObject->work[1] = characterObject->work[2] = characterObject->work[3] = 0;
    playerState->modelVisibilityState = playerState->materialFadeState = playerState->aiRouteIndex = playerState->aiActionState = 0;
    playerState->playerIntroAnimationFrames = playerState->unreferencedPlayerState18 = playerState->routeDelayFrames = playerState->routeFrame = 0;
    playerState->unreferencedPlayerState40 = playerState->unreferencedPlayerState44 = playerState->unreferencedPlayerState48 = 0.0f;
    playerState->unreferencedPlayerState4C = playerState->unreferencedPlayerState50 = playerState->unreferencedPlayerState54 = 0.0f;
    playerState->materialOpacity = 1.0f;
}

void CreateCharacterModel(OMOBJ* characterObject) {
    s32 playerIndex = characterObject->work[0];
    PlayerCharacterState* playerState = &playerCharacterStates[playerIndex];
    Vec characterStartingPositions[] = {
        {   0.0f, 0.0f, 500.0f },
        { 200.0f, 0.0f, 300.0f },
        { 200.0f, 0.0f, 450.0f },
        { 200.0f, 0.0f, 600.0f }
    };

    characterObject->mdlId[0] = CharModelCreate(playerState->characterId, 2);
    characterObject->mtnId[0] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0));
    characterObject->mtnId[1] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 2));
    characterObject->mtnId[2] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 3));
    characterObject->mtnId[6] = CharMotionCreate(playerState->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 28));
    characterObject->mtnId[3] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, playerState->characterId));
    characterObject->mtnId[4] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 8 + playerState->characterId));
    characterObject->mtnId[5] = Hu3DJointMotionFile(characterObject->mdlId[0], DATA_MAKE_NUM(DATADIR_M435, 16 + playerState->characterId));
    omSetTra(characterObject, characterStartingPositions[playerIndex].x, characterStartingPositions[playerIndex].y, characterStartingPositions[playerIndex].z);
    omSetRot(characterObject, 0.0f, -90.0f, 0.0f);
    Hu3DModelShadowSet(characterObject->mdlId[0]);
    Hu3DModelAttrSet(characterObject->mdlId[0], 1);
    SetModelMotionIfChanged(characterObject, 0, 0, 0, 1);
    RunMotionTargetProcess(objectManager, &playerState->turnTracker, characterObject->mdlId[0], characterModelNames[playerState->characterId], characterAnimationFlags[playerState->characterId][0], characterAnimationFlags[playerState->characterId][1]);
    ResetMotionTarget(&playerState->turnTracker, 0);
    ReleaseMotionTarget(&playerState->turnTracker, -1);
    ClearMotionTargetModel(&playerState->turnTracker, -1);
    characterObject->work[1] = characterObject->work[2] = characterObject->work[3] = 0;
    playerState->modelVisibilityState = playerState->materialFadeState = playerState->aiRouteIndex = playerState->aiActionState = 0;
    playerState->playerIntroAnimationFrames = playerState->unreferencedPlayerState18 = playerState->routeDelayFrames = playerState->routeFrame = 0;
    playerState->unreferencedPlayerState40 = playerState->unreferencedPlayerState44 = playerState->unreferencedPlayerState48 = 0.0f;
    playerState->unreferencedPlayerState4C = playerState->unreferencedPlayerState50 = playerState->unreferencedPlayerState54 = 0.0f;
    playerState->materialOpacity = 1.0f;
    CharMotionDataClose(playerState->characterId);
    characterObject->objFunc = UpdateCharacterObject;
}

void SetScoreDisplayState(s32 playerIndex, s32 scoreValue) {
    ScoreDisplayState* scoreDisplayState;

    scoreDisplayState = &scoreDisplayStates[playerIndex];
    scoreDisplayState->displayState = scoreValue;
}

void UpdateScoreDisplayOpacity(OMOBJ* scoreObject) {
    ScoreDisplayState* scoreDisplayState;
    s32 i;

    scoreDisplayState = &scoreDisplayStates[scoreObject->work[0]];
    switch (scoreDisplayState->displayState) {
        case 0:
            scoreDisplayState->opacity -= 0.1f;
            if (scoreDisplayState->opacity < 0.0f) {
                scoreDisplayState->opacity = 0.0f;
            }
            break;
        case 1:
            scoreDisplayState->opacity += 0.1f;
            if (scoreDisplayState->opacity > 1.0f) {
                scoreDisplayState->opacity = 1.0f;
            }
            break;
    }
    for (i = 0; i < 6; i++) {
        if (scoreDisplayState->opacity > 0.0f) {
            espDispOn(scoreDisplayState->scoreSprites[i]);
        } else {
            espDispOff(scoreDisplayState->scoreSprites[i]);
        }
        if (i == 5) {
            espTPLvlSet(scoreDisplayState->scoreSprites[i], 0.5f * scoreDisplayState->opacity);
        } else {
            espTPLvlSet(scoreDisplayState->scoreSprites[i], scoreDisplayState->opacity);
        }
    }
}

void SetPlayerScore(s32 playerIndex, s32 playerState) {
    ScoreDisplayState* scoreDisplayState;

    scoreDisplayState = &scoreDisplayStates[playerIndex];
    scoreDisplayState->previousScoreValue = scoreDisplayState->scoreValue;
    scoreDisplayState->scoreValue = playerState;
    if (scoreDisplayState->scoreValue < 0) {
        scoreDisplayState->scoreValue = 0;
    } else if (scoreDisplayState->scoreValue > 999) {
        scoreDisplayState->scoreValue = 999;
    }
    scoreDisplayState->scoreInterpolationFrame = 0;
}

void UpdateScoreDigits(OMOBJ* scoreObject) {
    ScoreDisplayState* scoreDisplay;
    s32 scoreDigit;
    s32 displayedScore;

    scoreDisplay = &scoreDisplayStates[scoreObject->work[0]];
    if (scoreDisplay->scoreInterpolationFrame <= 60) {
        displayedScore = interpolateSineEaseOut(scoreDisplay->previousScoreValue, scoreDisplay->scoreValue, scoreDisplay->scoreInterpolationFrame++, 60.0f);
    } else {
        displayedScore = scoreDisplay->previousScoreValue = scoreDisplay->scoreValue;
    }
    if ((scoreDigit = displayedScore / 100) == 0) {
        espDispOff(scoreDisplay->scoreSprites[0]);
    }
    espBankSet(scoreDisplay->scoreSprites[0], scoreDigit);
    if ((scoreDigit = (displayedScore % 100) / 10) == 0 && displayedScore / 100 == 0) {
        espDispOff(scoreDisplay->scoreSprites[1]);
    }
    espBankSet(scoreDisplay->scoreSprites[1], scoreDigit);
    scoreDigit = displayedScore % 10;
    espBankSet(scoreDisplay->scoreSprites[2], scoreDigit);
}

void UpdateScoreDisplay(OMOBJ* scoreObject) {
    UpdateScoreDisplayOpacity(scoreObject);
    UpdateScoreDigits(scoreObject);
}

void InitializeScoreDisplay(ScoreDisplayState* playerIndex) {
    OMOBJ* object;

    object = playerIndex->spriteObject;
    espTPLvlSet(playerIndex->scoreSprites[5], 0.0f);
    espDispOff(playerIndex->scoreSprites[5]);
    espTPLvlSet(playerIndex->scoreSprites[4], 0.0f);
    espDispOff(playerIndex->scoreSprites[4]);
    espTPLvlSet(playerIndex->scoreSprites[3], 0.0f);
    espDispOff(playerIndex->scoreSprites[3]);
    espTPLvlSet(playerIndex->scoreSprites[2], 0.0f);
    espDispOff(playerIndex->scoreSprites[2]);
    espTPLvlSet(playerIndex->scoreSprites[1], 0.0f);
    espDispOff(playerIndex->scoreSprites[1]);
    espTPLvlSet(playerIndex->scoreSprites[0], 0.0f);
    espDispOff(playerIndex->scoreSprites[0]);
    object->work[1] = object->work[2] = object->work[3] = 0;
    playerIndex->opacity = 0.0f;
    playerIndex->scoreValue = playerIndex->previousScoreValue = 0;
    playerIndex->displayState = playerIndex->displayFrame = playerIndex->spriteState = playerIndex->spriteTimer = 0;
    playerIndex->scoreInterpolationFrame = playerIndex->unreferencedSpriteState40 = playerIndex->unreferencedSpriteState44 = playerIndex->unreferencedSpriteState48 = 0;
}

void CreateScoreDisplay(OMOBJ* scoreObject) {
    ScoreDisplayState* scoreDisplay;
    PlayerCharacterState* playerState;
    s32 playerIndex;
    s32 scoreY;
    s32 scoreX;

    playerIndex = scoreObject->work[0];
    scoreY = 62;
    scoreDisplay = &scoreDisplayStates[playerIndex];
    playerState = &playerCharacterStates[playerIndex];
    if (playerIndex < 2) {
        scoreX = playerIndex * 118 + 84;
    } else {
        scoreX = (playerIndex - 2) * 118 + 390;
    }
    scoreDisplay->scoreSprites[5] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 21), 100, 0);
    espPosSet(scoreDisplay->scoreSprites[5], scoreX, scoreY);
    espColorSet(scoreDisplay->scoreSprites[5], 1, 1, 1);
    espTPLvlSet(scoreDisplay->scoreSprites[5], 0.0f);
    espDispOff(scoreDisplay->scoreSprites[5]);
    scoreDisplay->scoreSprites[4] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 20), 100, 0);
    espPosSet(scoreDisplay->scoreSprites[4], scoreX, scoreY);
    espTPLvlSet(scoreDisplay->scoreSprites[4], 0.0f);
    espDispOff(scoreDisplay->scoreSprites[4]);
    scoreDisplay->scoreSprites[3] = espEntry(characterModelData[playerState->characterId][2], 100, 0);
    espPosSet(scoreDisplay->scoreSprites[3], scoreX - 46, scoreY - 2);
    espTPLvlSet(scoreDisplay->scoreSprites[3], 0.0f);
    espDispOff(scoreDisplay->scoreSprites[3]);
    scoreDisplay->scoreSprites[0] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 51), 100, 0);
    espPosSet(scoreDisplay->scoreSprites[0], scoreX - 14, scoreY + 2);
    espTPLvlSet(scoreDisplay->scoreSprites[2], 0.0f);
    espDispOff(scoreDisplay->scoreSprites[2]);
    scoreDisplay->scoreSprites[1] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 51), 100, 0);
    espPosSet(scoreDisplay->scoreSprites[1], scoreX + 6, scoreY + 2);
    espTPLvlSet(scoreDisplay->scoreSprites[1], 0.0f);
    espDispOff(scoreDisplay->scoreSprites[1]);
    scoreDisplay->scoreSprites[2] = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 51), 100, 0);
    espPosSet(scoreDisplay->scoreSprites[2], scoreX + 26, scoreY + 2);
    espTPLvlSet(scoreDisplay->scoreSprites[0], 0.0f);
    espDispOff(scoreDisplay->scoreSprites[0]);
    scoreObject->work[1] = scoreObject->work[2] = scoreObject->work[3] = 0;
    scoreDisplay->opacity = 0.0f;
    scoreDisplay->scoreValue = scoreDisplay->previousScoreValue = 0;
    scoreDisplay->displayState = scoreDisplay->displayFrame = scoreDisplay->spriteState = scoreDisplay->spriteTimer = 0;
    scoreDisplay->scoreInterpolationFrame = scoreDisplay->unreferencedSpriteState40 = scoreDisplay->unreferencedSpriteState44 = scoreDisplay->unreferencedSpriteState48 = 0;
    scoreObject->objFunc = UpdateScoreDisplay;
}

s32 UpdatePlayerAndAudioEffects(OMOBJ* playerObject) {
    RoundScoreState* roundState;
    AudioVolumeState* audioState;
    GameUiState* uiState;
    PlayerCharacterState* playerState;
    Mtx modelMatrix;
    s32 volumeIndex;

    roundState = &roundScoreState;
    audioState = &audioVolumeState;
    uiState = &gameUiState;
    playerState = &playerCharacterStates[roundState->currentRoundIndex];
    if (playerObject->work[3] == 1) {
        playerObject->work[3] = 0;
        playerObject->work[1] = 1;
        playerObject->work[2] = 0;
        audioState->audioState = 1;
        for (volumeIndex = 0; volumeIndex < 6; volumeIndex++) {
            audioState->playerVolume[volumeIndex] = -(50.0f * volumeIndex);
        }
    }
    switch (playerObject->work[1]) {
        case 1:
            playerObject->work[1] = 2;
            ResetMotionTarget(&playerState->turnTracker, 0);
            SetModelMotion(uiState->uiObject, 0, 4, 5, 0);
            break;
        case 2:
            if (Hu3DMotionTimeGet(uiState->uiObject->mdlId[0]) == 90.0f) {
                playerObject->work[1] = 3;
                SetModelMotion(playerState->characterObject, 0, 6, 5, 0);
                Hu3DMotionTimeSet(uiState->uiObject->mdlId[0], 90.0f);
                Hu3DMotionSpeedSet(uiState->uiObject->mdlId[0], 0.0f);
                CharFXPlay(playerState->characterId, 0x123);
                HuAudFXPlay(0x6D5);
                omVibrate(playerState->playerIndex, 120, 12, 0);
            }
            break;
        case 3:
            Hu3DModelObjMtxGet(uiState->uiObject->mdlId[0], "g000m0-itemhook_M", modelMatrix);
            SetCharacterFadeState(roundState->currentRoundIndex, 1);
            for (volumeIndex = 0; volumeIndex < 6; volumeIndex++) {
                if (audioState->audioState == 1 || (audioState->audioState == 0 && audioState->playerVolume[volumeIndex] > 0.0f)) {
                    audioState->playerVolume[volumeIndex] += 10.0f;
                }
                if (audioState->playerVolume[volumeIndex] > 0.0f) {
                    audioState->ambientVolume[volumeIndex] += 0.1f;
                    audioState->effectVolume[volumeIndex] -= 5.0f;
                    if (audioState->musicVolume[volumeIndex] == 0.0f) {
                        SetModelMotion(playerObject, volumeIndex + 1, volumeIndex + 1, 1, 1);
                    }
                    if (audioState->musicVolume[volumeIndex] < 8.0f) {
                        audioState->sequenceVolume[volumeIndex] += 0.025f;
                    } else if (audioState->musicVolume[volumeIndex] < 23.0f) {
                        audioState->sequenceVolume[volumeIndex] = 0.7f;
                    } else if (audioState->musicVolume[volumeIndex] < 31.0f) {
                        audioState->sequenceVolume[volumeIndex] -= 0.1f;
                        if (audioState->sequenceVolume[volumeIndex] < 0.0f) {
                            audioState->sequenceVolume[volumeIndex] = 0.0f;
                        }
                    }
                    audioState->musicVolume[volumeIndex] += 1.0f;
                }
                Hu3DModelPosSet(playerObject->mdlId[volumeIndex + 1], modelMatrix[0][3], modelMatrix[1][3] + audioState->effectVolume[volumeIndex], modelMatrix[2][3] + audioState->playerVolume[volumeIndex]);
                Hu3DModelScaleSet(playerObject->mdlId[volumeIndex + 1], 1.5f * audioState->ambientVolume[volumeIndex], 1.5f * audioState->ambientVolume[volumeIndex], 1.5f * audioState->ambientVolume[volumeIndex]);
                Hu3DModelTPLvlSet(playerObject->mdlId[volumeIndex + 1], audioState->sequenceVolume[volumeIndex]);
                if (audioState->musicVolume[volumeIndex] == 31.0f) {
                    audioState->playerVolume[volumeIndex] = 0.0f;
                    audioState->effectVolume[volumeIndex] = 0.0f;
                    audioState->ambientVolume[volumeIndex] = 0.0f;
                    audioState->sequenceVolume[volumeIndex] = 0.5f;
                    audioState->musicVolume[volumeIndex] = 0.0f;
                }
            }
            if (playerObject->work[2]++ >= 120) {
                audioState->audioState = 0;
                Hu3DMotionSpeedSet(uiState->uiObject->mdlId[0], 1.0f);
            }
            if (playerObject->work[2] >= 10 && playerObject->work[2] <= 120) {
                SetCameraShake(6, -1, 8.0f);
            }
            break;
    }
}

void InitializeAudioEffects(AudioVolumeState* index) {
    OMOBJ* object;
    s32 i;

    object = index->playerObject;
    for (i = 0; i < 6; i++) {
        SetModelMotion(object, i + 1, i + 1, 5, 1);
        index->playerVolume[i] = -(50.0f * i);
        index->effectVolume[i] = 0.0f;
        index->ambientVolume[i] = 0.0f;
        index->sequenceVolume[i] = 0.5f;
        index->musicVolume[i] = 0.0f;
    }
    index->audioState = 1;
    object->work[0] = object->work[1] = object->work[2] = object->work[3] = 0;
}

void CreateAudioEffects(OMOBJ *object) {
    AudioVolumeState* audioState;
    s32 i;

    audioState = &audioVolumeState;
    for (i = 0; i < 6; i++) {
        if (i % 2 == 0) {
            object->mdlId[i + 1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 54));
            object->mtnId[i + 1] = Hu3DMotionIDGet(object->mdlId[i + 1]);
        }
        else {
            object->mdlId[i + 1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M435, 55));
            object->mtnId[i + 1] = Hu3DMotionIDGet(object->mdlId[i + 1]);
        }
        SetModelMotion(object, i + 1, i + 1, 5, 1);
        Hu3DModelPosSet(object->mdlId[i + 1], 0.0f, 0.0f, 0.0f);
        Hu3DModelRotSet(object->mdlId[i + 1], 0.0f, 0.0f, 0.0f);
        Hu3DModelScaleSet(object->mdlId[i + 1], 0.0f, 0.0f, 0.0f);
        Hu3DModelLayerSet(object->mdlId[i + 1], 2);
        audioState->playerVolume[i] = -(50.0f * i);
        audioState->effectVolume[i] = 0.0f;
        audioState->ambientVolume[i] = 0.0f;
        audioState->sequenceVolume[i] = 0.5f;
        audioState->musicVolume[i] = 0.0f;
    }
    audioState->audioState = 1;
    object->objFunc = (void*) UpdatePlayerAndAudioEffects; // update_player_and_audio_effects must return s32 to match
}

void SetCameraViewWide(void) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    cameraTween->targetPositionX = 0.0f;
    cameraTween->targetPositionY = 300.0f;
    cameraTween->targetPositionZ = 1740.0f;
    cameraTween->targetRotationX = -10.0f;
    cameraTween->targetRotationY = 0.0f;
    cameraTween->targetRotationZ = 0.0f;
    cameraTween->targetDistance = 300.0f;
    InterpolateTransformTowardSelf(cameraTween, 200.0f);
}

void UpdateCameraViewWide(void) {
    TransformTween* cameraTween;
    TransformTween targetCameraView;

    cameraTween = &cameraState;
    targetCameraView.cameraCenterX = 0.0f;
    targetCameraView.cameraCenterY = 200.0f;
    targetCameraView.cameraCenterZ = -50.0f;
    targetCameraView.cameraRotationX = 5.0f;
    targetCameraView.cameraRotationY = 0.0f;
    targetCameraView.cameraRotationZ = 0.0f;
    targetCameraView.cameraZoom = 200.0f;
    InterpolateTransformCosineAndBlend(cameraTween, &targetCameraView, cameraTween->elapsedTime++, 30.0f, 5.0f);
}

void SetCameraViewWheel(void) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    cameraTween->targetPositionX = 0.0f;
    cameraTween->targetPositionY = 200.0f;
    cameraTween->targetPositionZ = -50.0f;
    cameraTween->targetRotationX = 5.0f;
    cameraTween->targetRotationY = 0.0f;
    cameraTween->targetRotationZ = 0.0f;
    cameraTween->targetDistance = 200.0f;
    InterpolateTransformTowardSelf(cameraTween, 200.0f);
}

void SetCameraViewMap(void) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    cameraTween->targetPositionX = 0.0f;
    cameraTween->targetPositionY = 300.0f;
    cameraTween->targetPositionZ = -550.0f;
    cameraTween->targetRotationX = -15.0f;
    cameraTween->targetRotationY = 0.0f;
    cameraTween->targetRotationZ = 0.0f;
    cameraTween->targetDistance = 1200.0f;
    InterpolateTransformTowardSelf(cameraTween, 45.0f);
}

void SetCameraViewScoreboard(void) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    cameraTween->targetPositionX = 0.0f;
    cameraTween->targetPositionY = 150.0f;
    cameraTween->targetPositionZ = -550.0f;
    cameraTween->targetRotationX = -15.0f;
    cameraTween->targetRotationY = 0.0f;
    cameraTween->targetRotationZ = 0.0f;
    cameraTween->targetDistance = 1200.0f;
    InterpolateTransformTowardSelf(cameraTween, 30.0f);
}

void SetCameraViewWheelCloseup(void) {
    WheelMapState* mapState;
    PlayerActorState* playerActorState;
    TransformTween* cameraTween;
    HU3DMODEL* wheelModel;

    mapState = &wheelMapState;
    playerActorState = &playerActorStates[1];
    cameraTween = &cameraState;
    wheelModel = &Hu3DData[mapState->mapObject->mdlId[1]];
    cameraTween->targetPositionX = wheelModel->pos.x;
    cameraTween->targetPositionY = wheelModel->pos.y + 175.0f;
    cameraTween->targetPositionZ = wheelModel->pos.z;
    cameraTween->targetRotationX = -5.0f;
    cameraTween->targetRotationY = wheelModel->rot.y;
    cameraTween->targetRotationZ = 0.0f;
    cameraTween->targetDistance = 600.0f;
    InterpolateTransformTowardSelf(cameraTween, 15.0f);
}

void UpdateCameraViewWheel(void) {
    TransformTween* cameraTween;
    TransformTween targetCameraView;

    cameraTween = &cameraState;
    targetCameraView.cameraCenterX = 0.0f;
    targetCameraView.cameraCenterY = 175.0f;
    targetCameraView.cameraCenterZ = -250.0f;
    targetCameraView.cameraRotationX = -5.0f;
    targetCameraView.cameraRotationY = 0.0f;
    targetCameraView.cameraRotationZ = 0.0f;
    targetCameraView.cameraZoom = 600.0f;
    InterpolateTransformSineAndBlend(cameraTween, &targetCameraView, cameraTween->elapsedTime++, 10.0f, 5.0f);
}

void UpdateCameraViewPlayer(void) {
    TransformTween* cameraTween;
    TransformTween targetCameraView;

    cameraTween = &cameraState;
    targetCameraView.cameraCenterX = 0.0f;
    targetCameraView.cameraCenterY = 145.0f;
    targetCameraView.cameraCenterZ = 500.0f;
    targetCameraView.cameraRotationX = -15.0f;
    targetCameraView.cameraRotationY = 90.0f;
    targetCameraView.cameraRotationZ = 0.0f;
    targetCameraView.cameraZoom = 800.0f;
    InterpolateTransformSineAndBlend(cameraTween, &targetCameraView, cameraTween->elapsedTime++, 60.0f, 5.0f);
}

void UpdateCameraViewLeftSide(void) {
    TransformTween* cameraTween;
    TransformTween targetCameraView;

    cameraTween = &cameraState;
    targetCameraView.cameraCenterX = -1000.0f;
    targetCameraView.cameraCenterY = 150.0f;
    targetCameraView.cameraCenterZ = 435.0f;
    targetCameraView.cameraRotationX = 0.0f;
    targetCameraView.cameraRotationY = 90.0f;
    targetCameraView.cameraRotationZ = 0.0f;
    targetCameraView.cameraZoom = 800.0f;
    InterpolateTransformCosineAndBlend(cameraTween, &targetCameraView, cameraTween->elapsedTime++, 30.0f, 15.0f);
}

void UpdateCameraViewLeftSideHigh(void) {
    TransformTween* cameraTween;
    TransformTween targetCameraView;

    cameraTween = &cameraState;
    targetCameraView.cameraCenterX = -1000.0f;
    targetCameraView.cameraCenterY = 200.0f;
    targetCameraView.cameraCenterZ = 500.0f;
    targetCameraView.cameraRotationX = 10.0f;
    targetCameraView.cameraRotationY = 105.0f;
    targetCameraView.cameraRotationZ = 0.0f;
    targetCameraView.cameraZoom = 800.0f;
    InterpolateTransformSineAndBlend(cameraTween, &targetCameraView, cameraTween->elapsedTime++, 30.0f, 15.0f);
}

void UpdateCameraViewLeftSideQuick(void) {
    TransformTween* cameraTween;
    TransformTween targetCameraView;

    cameraTween = &cameraState;
    targetCameraView.cameraCenterX = -1000.0f;
    targetCameraView.cameraCenterY = 200.0f;
    targetCameraView.cameraCenterZ = 500.0f;
    targetCameraView.cameraRotationX = 10.0f;
    targetCameraView.cameraRotationY = 105.0f;
    targetCameraView.cameraRotationZ = 0.0f;
    targetCameraView.cameraZoom = 800.0f;
    InterpolateTransformCosineAndBlend(cameraTween, &targetCameraView, cameraTween->elapsedTime++, 10.0f, 5.0f);
}

void SetCameraViewMapCloseup(void) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    cameraTween->targetPositionX = 0.0f;
    cameraTween->targetPositionY = 150.0f;
    cameraTween->targetPositionZ = -550.0f;
    cameraTween->targetRotationX = -15.0f;
    cameraTween->targetRotationY = 0.0f;
    cameraTween->targetRotationZ = 0.0f;
    cameraTween->targetDistance = 1200.0f;
    InterpolateTransformTowardSelf(cameraTween, 20.0f);
}

void SetCameraViewMapHigh(void) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    cameraTween->targetPositionX = 0.0f;
    cameraTween->targetPositionY = 300.0f;
    cameraTween->targetPositionZ = -550.0f;
    cameraTween->targetRotationX = -15.0f;
    cameraTween->targetRotationY = 0.0f;
    cameraTween->targetRotationZ = 0.0f;
    cameraTween->targetDistance = 1200.0f;
    InterpolateTransformTowardSelf(cameraTween, 20.0f);
}

void SelectCameraView(s32 cameraIndex) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    CopyTransformToTarget(cameraTween);
    switch (cameraIndex) {
        case 0:
            SetCameraUpdateCallback(SetCameraViewWide);
            break;
        case 1:
            SetCameraUpdateCallback(UpdateCameraViewWide);
            break;
        case 2:
            SetCameraUpdateCallback(SetCameraViewWheel);
            break;
        case 3:
            SetCameraUpdateCallback(SetCameraViewMap);
            break;
        case 4:
            SetCameraUpdateCallback(SetCameraViewScoreboard);
            break;
        case 5:
            SetCameraUpdateCallback(SetCameraViewWheelCloseup);
            break;
        case 6:
            SetCameraUpdateCallback(UpdateCameraViewWheel);
            break;
        case 7:
            SetCameraUpdateCallback(UpdateCameraViewPlayer);
            break;
        case 8:
            SetCameraUpdateCallback(UpdateCameraViewLeftSide);
            break;
        case 10:
            SetCameraUpdateCallback(SetCameraViewMapCloseup);
            break;
        case 11:
            SetCameraUpdateCallback(SetCameraViewMapHigh);
            break;
    }
}

void ShowOpeningMessages(void) {
    float messageSize[] = { 0.0f, 0.0f };
    s32 messageWindowId;

    gameEventState = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 0));
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 1));
    messageWindowId = CreateMessageWindow((580.0f - messageSize[0]) / 2, 80.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D1);
    SetUiAnimationState(2);
    SetWindowMessage(messageWindowId, MAKE_MESSID(29, 0));
    WaitForMessageAcknowledgement(messageWindowId, 0);
    SetUiAnimationState(2);
    SetWindowMessage(messageWindowId, MAKE_MESSID(29, 1));
    WaitForMessageAcknowledgement(messageWindowId, 0);
    SetUiAnimationState(1);
    CloseMessageWindow(messageWindowId);
    gameEventState = 1;
    HuPrcEnd();
}

void ShowWheelStartMessage(void) {
    float messageSize[] = { 0.0f, 0.0f };
    s32 messageWindowId;

    gameEventState = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 2));
    messageWindowId = CreateMessageWindow((580.0f - messageSize[0]) / 2, 100.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D2);
    SetUiAnimationState(3);
    SetWindowMessage(messageWindowId, MAKE_MESSID(29, 2));
    WaitForMessageAcknowledgement(messageWindowId, REFRESH_RATE);
    SetGameAnimationState(3);
    WaitForMessageAcknowledgement(messageWindowId, REFRESH_RATE);
    SetUiAnimationState(1);
    CloseMessageWindow(messageWindowId);
    gameEventState = 1;
    HuPrcEnd();
}

void ShowPlayerTurnChoice(void) {
    float messageSize[] = { 0.0f, 0.0f };
    s32 messageWindowId;

    gameEventState = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 64));
    messageWindowId = CreateMessageWindow(550.0f - messageSize[0], 340.0f, messageSize[0], messageSize[1], 1);
    SetPlayerActorState(1, 2);
    HuAudFXPlayVolPan(0x65, 0x7F, 0x60);
    SetWindowMessage(messageWindowId, MAKE_MESSID(29, 64));
    ShowMessageChoice(messageWindowId);
    SetPlayerActorState(1, 1);
    CloseMessageWindow(messageWindowId);
    gameEventState = 1;
    HuPrcEnd();
}

void UpdateRoundResults(s32 roundScore) {
    float messageSize[] = { 0.0f, 0.0f };
    s32 messageIds[] = {
        MAKE_MESSID(29, 6),
        MAKE_MESSID(29, 39),
        MAKE_MESSID(29, 41),
        MAKE_MESSID(29, 43),
        MAKE_MESSID(29, 45),
        MAKE_MESSID(29, 47),
        MAKE_MESSID(29, 49),
        MAKE_MESSID(29, 51),
        -1
    };
    s32 messageWindowId;

    if (messageIds[roundScore] == -1) {
        return;
    }
    SetPlayerActorState(0, 2);
    HuPrcSleep(15);
    ExpandMessageDimensions(messageSize, messageIds[roundScore]);
    messageWindowId = CreateMessageWindow(20.0f, 360.0f, messageSize[0], messageSize[1], 0);
    HuAudFXPlayVolPan(0x3E, 0x7F, 0x20);
    SetWindowMessage(messageWindowId, messageIds[roundScore]);
    WaitForMessageAcknowledgement(messageWindowId, 0);
    SetPlayerActorState(0, 1);
    CloseMessageWindow(messageWindowId);
}

void ShowRoundScoreMessages(void) {
    float messageSize[] = { 0.0f, 0.0f };
    s32 messageIds[] = {
        MAKE_MESSID(29, 5),
        MAKE_MESSID(29, 38),
        MAKE_MESSID(29, 40),
        MAKE_MESSID(29, 42),
        MAKE_MESSID(29, 44),
        MAKE_MESSID(29, 46),
        MAKE_MESSID(29, 48),
        MAKE_MESSID(29, 50),
        MAKE_MESSID(29, 52)
    };
    s32 messageWindowId;
    s32 messageIndex;

    gameEventState = 0;
    for (messageIndex = 0; messageIndex < 9; messageIndex++) {
        ExpandMessageDimensions(messageSize, messageIds[messageIndex]);
    }
    messageWindowId = CreateMessageWindow(550.0f - messageSize[0], 340.0f, messageSize[0], messageSize[1], 1);
    HuAudFXPlayVolPan(0x65, 0x7F, 0x60);
    for (messageIndex = 0; messageIndex < 9; messageIndex++) {
        SetPlayerActorState(1, 2);
        SetWindowMessage(messageWindowId, messageIds[messageIndex]);
        WaitForMessageAcknowledgement(messageWindowId, 0);
        UpdateRoundResults(messageIndex);
    }
    SetPlayerActorState(1, 1);
    CloseMessageWindow(messageWindowId);
    gameEventState = 1;
    HuPrcEnd();
}

void ShowRoundInstruction(void) {
    float messageSize[] = { 0.0f, 0.0f };
    s32 messageWindowId;

    gameEventState = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 53));
    messageWindowId = CreateMessageWindow((580.0f - messageSize[0]) / 2, 80.0f, messageSize[0], messageSize[1], 2);
    omVibrate(GetGamePlayerIndex(0), 12, 4, 2);
    omVibrate(GetGamePlayerIndex(1), 12, 4, 2);
    omVibrate(GetGamePlayerIndex(2), 12, 4, 2);
    omVibrate(GetGamePlayerIndex(3), 12, 4, 2);
    HuAudFXPlay(0x6D2);
    SetUiAnimationState(2);
    SetWindowMessage(messageWindowId, MAKE_MESSID(29, 53));
    WaitForMessageAcknowledgement(messageWindowId, 0);
    SetUiAnimationState(1);
    CloseMessageWindow(messageWindowId);
    gameEventState++;
    HuPrcEnd();
}

void ShowScoreResultMessage(void) {
    float messageSize[] = { 0.0f, 0.0f };
    PlayerActorState* playerState;
    s32 messageWindowId;

    gameEventState = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 54));
    messageWindowId = CreateMessageWindow(580.0f - messageSize[0] - 40.0f, 260.0f, messageSize[0], messageSize[1], 1);
    playerState = &playerActorStates[1];
    SetModelMotion(playerState->playerObject, 0, 12, 1, 0);
    HuAudFXPlayVolPan(0x3E, 0x7F, 0x20);
    SetWindowMessage(messageWindowId, MAKE_MESSID(29, 54));
    WaitForMessageAcknowledgement(messageWindowId, 15);
    SetPrimaryPlayerFrame(1);
    WaitForMessageAcknowledgement(messageWindowId, 15);
    CloseMessageWindow(messageWindowId);
    SelectCameraView(5);
    gameEventState = 1;
    HuPrcEnd();
}

void ShowPlayerResultMessage(void) {
    float messageSize[] = { 0.0f, 0.0f };
    PlayerActorState* playerState;
    s32 messageWindowId;

    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 55));
    messageWindowId = CreateMessageWindow(40.0f, 260.0f, messageSize[0], messageSize[1], 0);
    playerState = &playerActorStates[0];
    SetModelMotion(playerState->playerObject, 0, 3, 1, 0);
    HuAudFXPlayVolPan(0x65, 0x7F, 0x60);
    SetWindowMessage(messageWindowId, MAKE_MESSID(29, 55));
    WaitForMessageAcknowledgement(messageWindowId, 15);
    SetPlayerActorState(0, 1);
    WaitForMessageAcknowledgement(messageWindowId, 15);
    CloseMessageWindow(messageWindowId);
    HuPrcEnd();
}

void ShowRoundTransitionMessage(void) {
    float messageSize[] = { 0.0f, 0.0f };
    s32 messageWindowId;

    gameEventState = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 56));
    SelectCameraView(6);
    messageWindowId = CreateMessageWindow((580.0f - messageSize[0]) / 2, 320.0f, messageSize[0], messageSize[1], 2);
    HuAudFXPlay(0x6D2);
    SetUiAnimationState(2);
    SetWindowMessage(messageWindowId, MAKE_MESSID(29, 56));
    WaitForMessageAcknowledgement(messageWindowId, 0);
    SetUiAnimationState(1);
    CloseMessageWindow(messageWindowId);
    gameEventState = 1;
    HuPrcEnd();
}

void EmptyRoundIntroCallback(void) {
}

s32 UpdateRoundIntroSequence(void) {
    if (UpdateSequenceTimer(&gameSequencePlayer, 0) == 80) {
        SelectCameraView(1);
        sequenceSoundIds[0] = HuAudSeqPlay(6);
    }
    if (UpdateSequenceTimer(&gameSequencePlayer, 0) == 115) {
        SelectCameraView(2);
    }
    if (UpdateSequenceTimer(&gameSequencePlayer, 0) == 180) {
        SelectCameraView(3);
    }
    if (UpdateSequenceTimer(&gameSequencePlayer, 0) == 0) {
        SetGameAnimationState(2);
    }
    if (UpdateSequenceTimer(&gameSequencePlayer, 0) == 150) {
        SetGameAnimationState(1);
    }
    if (UpdateSequenceTimer(&gameSequencePlayer, 1) != 0) {
        return AdvanceSequence(&gameSequencePlayer);
    }
    return 0;
}

void StartOpeningDialogue(void) {
    HuPrcChildCreate(ShowOpeningMessages, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 WaitForOpeningDialogue(void) {
    if (gameEventState != 0 && UpdateSequenceTimer(&gameSequencePlayer, 1) != 0) {
        return AdvanceSequence(&gameSequencePlayer);
    }
    return 0;
}

void StartWheelIntro(void) {
    s32 i;

    HuPrcChildCreate(ShowWheelStartMessage, 100, 0x2000, 0, HuPrcCurrentGet());
    for (i = 0; i < 4; i++) {
        SetCharacterVisibilityState(i, 1);
    }
}

s32 WaitForWheelIntro(void) {
    if (gameEventState != 0 && UpdateSequenceTimer(&gameSequencePlayer, 1) != 0) {
        return AdvanceSequence(&gameSequencePlayer);
    }
    return 0;
}

void SetRoundCameraView(void) {
    GameUiState* uiState;

    uiState = &gameUiState;
    SelectCameraView(10);
}

s32 WaitForRoundCamera(void) {
    if (UpdateSequenceTimer(&gameSequencePlayer, 1) != 0) {
        return AdvanceSequence(&gameSequencePlayer);
    }
    return 0;
}

void StartPlayerTurnChoice(void) {
    HuPrcChildCreate(ShowPlayerTurnChoice, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 WaitForPlayerTurnChoice(void) {
    if (gameEventState != 0 && UpdateSequenceTimer(&gameSequencePlayer, 1) != 0) {
        if (randomizedPlayerIndex != 0) {
            return AdvanceSequence(&gameSequencePlayer);
        }
        SetSequenceStep(&gameSequencePlayer, 6);
    }
    return 0;
}

void StartRoundScoreDialogue(void) {
    GameUiState* uiState;

    uiState = &gameUiState;
    SetModelMotion(uiState->uiObject, 0, 0, 60, 1);
    HuPrcChildCreate(ShowRoundScoreMessages, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 WaitForRoundScoreDialogue(void) {
    if (gameEventState != 0 && UpdateSequenceTimer(&gameSequencePlayer, 1) != 0) {
        return AdvanceSequence(&gameSequencePlayer);
    }
    return 0;
}

void StartRoundActionCamera(void) {
    GameUiState* uiState;

    uiState = &gameUiState;
    SelectCameraView(11);
    SetModelMotion(uiState->uiObject, 0, 1, 60, 1);
}

s32 WaitForRoundActionCamera(void) {
    if (UpdateSequenceTimer(&gameSequencePlayer, 1) != 0) {
        return AdvanceSequence(&gameSequencePlayer);
    }
    return 0;
}

void StartTurnTracker(void) {
    GameUiState* uiState;
    PlayerActorState* playerActor;

    gameEventState = 0;
    HuPrcChildCreate(ShowRoundInstruction, 100, 0x2000, 0, HuPrcCurrentGet());
    playerActor = &playerActorStates[1];
    uiState = &gameUiState;
    ResetMotionTarget(&playerActor->turnTracker, 1);
    ReleaseMotionTarget(&playerActor->turnTracker, -1);
    ClearMotionTargetModel(&playerActor->turnTracker, -1);
    SetMotionTargetModel(&playerActor->turnTracker, uiState->uiObject->mdlId[0]);
}

s32 WaitForTurnTracker(void) {
    if (gameEventState != 0) {
        return AdvanceSequence(&gameSequencePlayer);
    }
    return 0;
}

void StartPlayerReactionDialogues(void) {
    HuPrcChildCreate(ShowScoreResultMessage, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(ShowPlayerResultMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 WaitForPlayerReactions(void) {
    if (gameEventState != 0 && UpdateSequenceTimer(&gameSequencePlayer, 1) != 0) {
        return AdvanceSequence(&gameSequencePlayer);
    }
    return 0;
}

void EmptyRoundMessageCallback(void) {
}

s32 WaitForRoundMessage(void) {
    if (UpdateSequenceTimer(&gameSequencePlayer, 1) != 0) {
        return AdvanceSequence(&gameSequencePlayer);
    }
    return 0;
}

void StartRoundEventMessage(void) {
    gameEventState = 0;
    HuPrcChildCreate(ShowRoundTransitionMessage, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 WaitForRoundEventMessage(void) {
    if (gameEventState != 0 && UpdateSequenceTimer(&gameSequencePlayer, 1) != 0) {
        return AdvanceSequence(&gameSequencePlayer);
    }
    return 0;
}

void StartWheelPieceSequence(void) {
    s32 i;
    WheelMapState* mapState;
    GameUiState* uiState;
    PlayerCharacterState* playerState;
    PlayerActorState* playerActor;

    mapState = &wheelMapState;
    uiState = &gameUiState;
    SetModelMotion(uiState->uiObject, 0, 0, 60, 1);
    SelectCameraView(7);
    for (i = 0; i < 4; i++) {
        playerState = &playerCharacterStates[i];
        ResetMotionTarget(&playerState->turnTracker, 1);
        ReleaseMotionTarget(&playerState->turnTracker, -1);
        ClearMotionTargetModel(&playerState->turnTracker, -1);
        SetMotionTargetModel(&playerState->turnTracker, mapState->mapObject->mdlId[1]);
    }
    playerActor = &playerActorStates[1];
    ResetMotionTarget(&playerActor->turnTracker, 0);
    ReleaseMotionTarget(&playerActor->turnTracker, -1);
    ClearMotionTargetModel(&playerActor->turnTracker, -1);
}

s32 WaitForWheelPieceSequence(void) {
    if (UpdateSequenceTimer(&gameSequencePlayer, 1) != 0 && activePlayerIndex != 0) {
        return AdvanceSequence(&gameSequencePlayer);
    }
    return 0;
}

void PrepareRoundWrapUp(void) {
    RoundScoreState* roundState;
    PlayerActorState* playerActor;
    PlayerCharacterState* playerState;

    roundState = &roundScoreState;
    playerActor = &playerActorStates[1];
    playerState = &playerCharacterStates[roundState->currentRoundIndex];
    roundState->roundWrapUpState = 0;
    SetModelMotion(playerActor->playerObject, 0, 6, 10, 0);
    SetModelMotion(playerState->characterObject, 0, 4, 60, 1);
    roundState->activeRoundCount = roundState->currentRoundIndex;
    HuAudSeqFadeOut(sequenceSoundIds[0], 1000);
}

s32 AdvanceRoundWrapUp(void) {
    RoundScoreState* roundState;
    PlayerActorState* playerActor;
    WheelMapState* mapState;
    PlayerActorState* playerActor2;
    s32 i;

    roundState = &roundScoreState;
    playerActor = &playerActorStates[1];
    if (isMotionAtFrame(playerActor->playerObject, 0, 6, 50, 0) != 0) {
        UpdateModelRotationTween(playerActor->playerObject, 0, 0.0f, 10);
    } else if (isMotionAtFrame(playerActor->playerObject, 0, 6, 50, 1) != 0) {
        UpdateModelRotationTween(playerActor->playerObject, 0, 90.0f, 10);
        if (roundState->roundWrapUpState == 0) {
            roundState->roundWrapUpState = 1;
            for (i = 0; i < 4; i++) {
                SetScoreDisplayState(i, 1);
            }
            SetWheelEnvironmentState(1);
            SetAllPieceStates(1);
        }
    }
    if (UpdateSequenceTimer(&gameSequencePlayer, 1) != 0) {
        mapState = &wheelMapState;
        playerActor2 = &playerActorStates[1];
        ResetMotionTarget(&playerActor2->turnTracker, 1);
        ReleaseMotionTarget(&playerActor2->turnTracker, -1);
        ClearMotionTargetModel(&playerActor2->turnTracker, -1);
        for (i = 0; i < 3; i++) {
            ReserveMotionTarget(&playerActor2->turnTracker, mapState->mapObject->mdlId[i + roundState->currentRoundIndex * 3 + 2]);
        }
        SelectCameraView(8);
        return AdvanceSequence(&gameSequencePlayer);
    }
    return 0;
}

SequenceStep roundSequenceSteps[] = {
    { EmptyRoundIntroCallback, UpdateRoundIntroSequence, 300 },
    { StartOpeningDialogue, WaitForOpeningDialogue,  60 },
    { StartWheelIntro, WaitForWheelIntro,  60 },
    { SetRoundCameraView, WaitForRoundCamera,  45 },
    { StartPlayerTurnChoice, WaitForPlayerTurnChoice,  60 },
    { StartRoundScoreDialogue, WaitForRoundScoreDialogue,  60 },
    { StartRoundActionCamera, WaitForRoundActionCamera,  45 },
    { StartTurnTracker, WaitForTurnTracker,  60 },
    { StartPlayerReactionDialogues, WaitForPlayerReactions,  60 },
    { EmptyRoundMessageCallback, WaitForRoundMessage,  90 },
    { StartRoundEventMessage, WaitForRoundEventMessage,  60 },
    { StartWheelPieceSequence, WaitForWheelPieceSequence, 180 },
    { StartRoundWrapUp, UpdateRoundWrapUp, 180 },
    {      NULL,      NULL,  -1 }
};

void StartRoundSequence(void) {
    StartSequence(&gameSequencePlayer, roundSequenceSteps);
}

s32 UpdateRoundSequenceCallbacks(void) {
    s32 sequenceFinished;

    sequenceFinished = 0;
    if (gameSequencePlayer.onStep != NULL) {
        gameSequencePlayer.onStep();
        gameSequencePlayer.onStep = NULL;
    }
    if (gameSequencePlayer.onComplete != NULL) {
        sequenceFinished = gameSequencePlayer.onComplete();
    }
    return sequenceFinished;
}

s32 CheckPlayerActionInput(s32 playerIndex, s32 playerState) {
    s32 shouldCpuAct;
    PlayerCharacterState* playerState;

    shouldCpuAct = 0;
    playerState = &playerCharacterStates[playerIndex];
    if (playerState->isCpu != 0) {
        switch (playerState->aiDifficulty) {
            case 0:
                if (value % 60 == 1 && AreAllPiecesClear()) {
                    shouldCpuAct = 1;
                }
                break;
            case 1:
                if (value % 45 == 1 && AreAllPiecesClear()) {
                    shouldCpuAct = 1;
                }
                break;
            case 2:
                if (value % 30 == 1 && AreAllPiecesClear()) {
                    shouldCpuAct = 1;
                }
                break;
            case 3:
                if (value % 15 == 1 && AreAllPiecesClear()) {
                    shouldCpuAct = 1;
                }
                break;
        }
    } else if (HuPadBtnDown[playerState->controllerIndex] & 0x100) {
        shouldCpuAct = 1;
    }
    return shouldCpuAct;
}

void InitializeWheelCameraView(void) {
    PlayerActorState* spC;
    TransformTween* cameraTween;

    spC = &playerActorStates[1];
    cameraTween = &cameraState;
}

void UpdateWheelCameraView(void) {
    TransformTween targetCameraView;
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    targetCameraView.cameraCenterX = 0.0f;
    targetCameraView.cameraCenterY = 200.0f;
    targetCameraView.cameraCenterZ = -550.0f;
    targetCameraView.cameraRotationX = 5.0f;
    targetCameraView.cameraRotationY = 0.0f;
    targetCameraView.cameraRotationZ = 0.0f;
    targetCameraView.cameraZoom = 1300.0f;
    InterpolateTransformLinearAndBlend(cameraTween, &targetCameraView, cameraTween->elapsedTime++, 90.0f, 10.0f);
}

void UpdateWheelCameraViewFar(void) {
    TransformTween targetCameraView;
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    targetCameraView.cameraCenterX = 0.0f;
    targetCameraView.cameraCenterY = 200.0f;
    targetCameraView.cameraCenterZ = -850.0f;
    targetCameraView.cameraRotationX = 5.0f;
    targetCameraView.cameraRotationY = 0.0f;
    targetCameraView.cameraRotationZ = 0.0f;
    targetCameraView.cameraZoom = 1300.0f;
    InterpolateTransformLinearAndBlend(cameraTween, &targetCameraView, cameraTween->elapsedTime++, 120.0f, 10.0f);
}

void SelectRoundCameraView(s32 cameraIndex) {
    TransformTween* cameraTween;

    cameraTween = &cameraState;
    CopyTransformToTarget(cameraTween);
    switch (cameraIndex) {
        case 0:
            SetCameraUpdateCallback(UpdateCameraViewPlayer);
            break;
        case 1:
            SetCameraUpdateCallback(UpdateCameraViewLeftSide);
            break;
        case 2:
            SetCameraUpdateCallback(UpdateCameraViewLeftSideHigh);
            break;
        case 3:
            SetCameraUpdateCallback(UpdateCameraViewLeftSideQuick);
            break;
        case 4:
            SetCameraUpdateCallback(InitializeWheelCameraView);
            break;
        case 5:
            SetCameraUpdateCallback(UpdateWheelCameraView);
            break;
        case 6:
            SetCameraUpdateCallback(UpdateWheelCameraViewFar);
            break;
    }
}

void ShowRoundScoreMessage(void) {
    char scoreText[8];
    float messageSize[] = { 0.0f, 0.0f };
    RoundScoreState* roundState;
    PlayerCharacterState* playerState;
    s32 messageWindowId;

    roundState = &roundScoreState;
    playerState = &playerCharacterStates[roundState->currentRoundIndex];
    gameEventState = 0;
    sprintf(scoreText, "%d", roundState->roundScore);
    HuWinInsertMesSizeGet(playerState->characterId, 0);
    HuWinInsertMesSizeGet(MAKE_MESSID_PTR(scoreText), 1);
    HuWinMesMaxSizeGet(1, messageSize, MAKE_MESSID(29, 63));
    messageWindowId = CreateMessageWindow(550.0f - messageSize[0], 180.0f, messageSize[0], messageSize[1], 1);
    HuAudFXPlayVolPan(0x65, 0x7F, 0x60);
    SetPlayerScore(roundState->currentRoundIndex, roundState->roundScore);
    HuWinInsertMesSet(messageWindowId, MAKE_MESSID_PTR(playerState->characterId), 0);
    HuWinInsertMesSet(messageWindowId, MAKE_MESSID_PTR(scoreText), 1);
    SetPlayerActorState(1, 2);
    SetWindowMessage(messageWindowId, MAKE_MESSID(29, 63));
    WaitForMessageAcknowledgement(messageWindowId, REFRESH_RATE * 2);
    SetPlayerActorState(1, 1);
    CloseMessageWindow(messageWindowId);
    gameEventState = 1;
    HuPrcEnd();
}

void ShowPlayerActionMessages(void) {
    float messageSize[] = { 0.0f, 0.0f };
    s32 messageOptions[][2] = {
        { MAKE_MESSID(29, 60), 9 },
        { MAKE_MESSID(29, 61), 11 }
    };
    PlayerActorState* playerState;
    s32 messageWindowId;
    s32 messageIndex;

    gameEventState = 0;
    for (messageIndex = 0; messageIndex < 2; messageIndex++) {
        if (messageIndex == 1) {
            playerState = &playerActorStates[1];
            ResetMotionTarget(&playerState->turnTracker, 0);
            ReleaseMotionTarget(&playerState->turnTracker, -1);
            ClearMotionTargetModel(&playerState->turnTracker, -1);
        }
        ExpandMessageDimensions(messageSize, messageOptions[messageIndex][0]);
        messageWindowId = CreateMessageWindow(550.0f - messageSize[0], 180.0f, messageSize[0], messageSize[1], 1);
        HuAudFXPlayVolPan(0x65, 0x7F, 0x60);
        SetPrimaryPlayerMotion(messageOptions[messageIndex][1]);
        SetWindowMessage(messageWindowId, messageOptions[messageIndex][0]);
        WaitForMessageAcknowledgement(messageWindowId, REFRESH_RATE * 2);
        CloseMessageWindow(messageWindowId);
    }
    gameEventState = 1;
    HuPrcEnd();
}

void ShowWheelResultMessage(void) {
    float messageSize[] = { 0.0f, 0.0f };
    float windowStartX;
    float windowTargetX;
    s32 soundEffectId;
    s32 messageWindowId;
    s32 animationFrame;
    GameUiState* uiState;
    PlayerActorState* playerState;

    gameEventState = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 62));
    windowStartX = 640.0f;
    windowTargetX = (580.0f - messageSize[0]) / 2 - 1.0f;
    messageWindowId = CreateMessageWindow(windowStartX, 100.0f, messageSize[0], messageSize[1], 2);
    SetPrimaryPlayerMotion(9);
    uiState = &gameUiState;
    playerState = &playerActorStates[1];
    ResetMotionTarget(&playerState->turnTracker, 1);
    ReleaseMotionTarget(&playerState->turnTracker, -1);
    ClearMotionTargetModel(&playerState->turnTracker, -1);
    SetMotionTargetModel(&playerState->turnTracker, uiState->uiObject->mdlId[0]);
    soundEffectId = HuAudFXPlay(0x6D6);
    HuAudFXPanning(soundEffectId, 0x60);
    SetUiAnimationState(2);
    SetWindowMessage(messageWindowId, MAKE_MESSID(29, 62));
    HuPrcSleep(5);
    for (animationFrame = 0; animationFrame < REFRESH_RATE * 2; animationFrame++) {
        windowStartX = interpolateCosineEaseIn(640.0f, windowTargetX, animationFrame, 30.0f);
        HuWinPosSet(messageWindowId, windowStartX, 100.0f);
        HuPrcVSleep();
    }
    HuPrcSleep(60);
    SetUiAnimationState(1);
    CloseMessageWindow(messageWindowId);
    gameEventState = 1;
    HuPrcEnd();
}

void ShowWheelAnnouncement(void) {
    float messageSize[] = { 0.0f, 0.0f };
    float windowStartX;
    float windowTargetX;
    s32 soundEffectId;
    s32 messageWindowId;
    s32 animationFrame;
    GameUiState* uiState;
    PlayerActorState* playerState;

    gameEventState = 0;
    ExpandMessageDimensions(messageSize, MAKE_MESSID(29, 57));
    windowStartX = 640.0f;
    windowTargetX = (580.0f - messageSize[0]) / 2 - 1.0f;
    messageWindowId = CreateMessageWindow(windowStartX, 100.0f, messageSize[0], messageSize[1], 2);
    uiState = &gameUiState;
    playerState = &playerActorStates[1];
    ResetMotionTarget(&playerState->turnTracker, 1);
    ReleaseMotionTarget(&playerState->turnTracker, -1);
    ClearMotionTargetModel(&playerState->turnTracker, -1);
    SetMotionTargetModel(&playerState->turnTracker, uiState->uiObject->mdlId[0]);
    soundEffectId = HuAudFXPlay(0x6D2);
    HuAudFXPanning(soundEffectId, 0x60);
    SetUiAnimationState(2);
    SetWindowMessage(messageWindowId, MAKE_MESSID(29, 57));
    HuPrcSleep(5);
    for (animationFrame = 0; animationFrame < REFRESH_RATE * 2; animationFrame++) {
        windowStartX = interpolateCosineEaseIn(640.0f, windowTargetX, animationFrame, 30.0f);
        HuWinPosSet(messageWindowId, windowStartX, 100.0f);
        HuPrcVSleep();
    }
    SetUiAnimationState(1);
    CloseMessageWindow(messageWindowId);
    gameEventState = 1;
    HuPrcEnd();
}

void ShowWinnerAndLoserMessages(void) {
    float messageSize[] = { 0.0f, 0.0f };
    s32 messageIds[] = {
        MAKE_MESSID(29, 58),
        MAKE_MESSID(29, 59)
    };
    PlayerActorState* winnerState;
    PlayerActorState* loserState;
    AudioVolumeState* audioState;
    s32 messageWindowId;
    s32 messageIndex;

    gameEventState = 0;
    for (messageIndex = 0; messageIndex < 2; messageIndex++) {
        if (messageIndex == 1) {
            winnerState = &playerActorStates[1];
            loserState = &playerActorStates[0];
            audioState = &audioVolumeState;
            audioState->playerObject->work[3] = 1;
            SetModelMotion(loserState->playerObject, 0, 1, 30, 1);
            SetModelMotion(loserState->playerObject, 1, 6, 30, 1);
            SetModelMotion(winnerState->playerObject, 0, 10, 60, 0);
            gameEventState = 1;
        }
        ExpandMessageDimensions(messageSize, messageIds[messageIndex]);
        messageWindowId = CreateMessageWindow((580.0f - messageSize[0]) / 2 - 1.0f, 75.0f, messageSize[0], messageSize[1], 2);
        HuAudFXPlay(0x6D6);
        if (messageIndex == 0) {
            SetUiAnimationState(2);
        }
        SetWindowMessage(messageWindowId, messageIds[messageIndex]);
        WaitForMessageAcknowledgement(messageWindowId, REFRESH_RATE);
        if (messageIndex == 0) {
            SetUiAnimationState(1);
        }
        if (messageIndex == 1) {
            WaitForMessageAcknowledgement(messageWindowId, REFRESH_RATE);
        }
        CloseMessageWindow(messageWindowId);
    }
    HuPrcEnd();
}

void EmptyRoundSequenceCallback(void) {
}

s32 WaitForRoundSequence(void) {
    if (UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
        return AdvanceSequence(&roundSequencePlayer);
    }
    return 0;
}

void StartRoundWrapUp(void) {
    RoundScoreState* roundState;
    PlayerActorState* playerActor;
    PlayerCharacterState* playerState;

    roundState = &roundScoreState;
    playerActor = &playerActorStates[1];
    playerState = &playerCharacterStates[roundState->currentRoundIndex];
    roundState->roundWrapUpState = 0;
    SetModelMotion(playerActor->playerObject, 0, 6, 10, 0);
    SetModelMotion(playerState->characterObject, 0, 4, 60, 1);
    roundState->activeRoundCount = roundState->currentRoundIndex;
}

s32 UpdateRoundWrapUp(void) {
    s32 i;
    RoundScoreState* roundState;
    PlayerActorState* playerActor;
    WheelMapState* mapState;
    PlayerActorState* playerActor2;

    roundState = &roundScoreState;
    playerActor = &playerActorStates[1];
    if (isMotionAtFrame(playerActor->playerObject, 0, 6, 50, 0) != 0) {
        UpdateModelRotationTween(playerActor->playerObject, 0, 0.0f, 10);
    } else if (isMotionAtFrame(playerActor->playerObject, 0, 6, 50, 1) != 0) {
        UpdateModelRotationTween(playerActor->playerObject, 0, 90.0f, 10);
        if (roundState->roundWrapUpState == 0) {
            roundState->roundWrapUpState = 1;
            SetWheelEnvironmentState(1);
            SetAllPieceStates(1);
        }
    }
    if (UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
        mapState = &wheelMapState;
        playerActor2 = &playerActorStates[1];
        ResetMotionTarget(&playerActor2->turnTracker, 1);
        ReleaseMotionTarget(&playerActor2->turnTracker, -1);
        ClearMotionTargetModel(&playerActor2->turnTracker, -1);
        for (i = 0; i < 3; i++) {
            SetMotionTargetModel(&playerActor2->turnTracker, mapState->mapObject->mdlId[i + roundState->currentRoundIndex * 3 + 2]);
        }
        return AdvanceSequence(&roundSequencePlayer);
    }
    return 0;
}

void SetRoundCameraViewIntro(void) {
    SelectRoundCameraView(1);
}

s32 WaitForRoundIntroCamera(void) {
    if (UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
        return AdvanceSequence(&roundSequencePlayer);
    }
    return 0;
}

void StartPlayerActionCountdown(void) {
    if (roundWinnerIndex == 0) {
        roundWinnerIndex = 1;
        roundWinnerScore = REFRESH_RATE * 5;
    }
}

s32 WaitForPlayerAction(void) {
    s32 shouldCpuAct;
    RoundScoreState* roundState;

    shouldCpuAct = 0;
    roundState = &roundScoreState;
    UpdateSequenceTimer(&roundSequencePlayer, 1);
    if (UpdateSequenceTimer(&roundSequencePlayer, 0) <= 10) {
        return 0;
    }
    if (CheckPlayerActionInput(roundState->currentRoundIndex, UpdateSequenceTimer(&roundSequencePlayer, 0)) != 0) {
        shouldCpuAct = 1;
        SkipResultCountdown();
    }
    if (shouldCpuAct != 0 || IsCountdownComplete() != 0) {
        SetAllPieceStates(4);
        return AdvanceSequence(&roundSequencePlayer);
    }
    return 0;
}

void ResetRoundScore(void) {
    RoundScoreState* roundState;

    roundState = &roundScoreState;
    roundState->scoreCollectionState = roundState->roundScore = roundState->roundHasNegativeScore = 0;
}

s32 CollectPieceScores(void) {
    s32 i;
    s32 j;
    RoundScoreState* roundState;
    WheelMapState* mapState;

    roundState = &roundScoreState;
    mapState = &wheelMapState;
    if (roundState->scoreCollectionState == 0) {
        for (i = 0; i < 3; i++) {
            if (mapState->pieceState[i] != 7) {
                break;
            }
        }
        if (i == 3) {
            roundState->scoreCollectionState = 1;
            for (j = 0; j < 3; j++) {
                roundState->roundScore += mapState->pieceCollisionResult[j];
            }
        }
    } else if (roundState->scoreCollectionState == 1) {
        if (roundState->roundScore < 0) {
            roundState->roundHasNegativeScore = 1;
            SetSequenceStep(&roundSequencePlayer, 9);
            SelectRoundCameraView(3);
        } else if (UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
            return AdvanceSequence(&roundSequencePlayer);
        }
    }
    return 0;
}

void PlayPlayerScoreAnimation(void) {
    PlayerActorState* playerActor;

    playerActor = &playerActorStates[1];
    SetModelMotion(playerActor->playerObject, 0, 8, 10, 1);
}

s32 WaitForScoreAnimation(void) {
    if (UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
        return AdvanceSequence(&roundSequencePlayer);
    }
    return 0;
}

void ShowPlayerScoreResult(void) {
    RoundScoreState* roundState;
    PlayerCharacterState* spC;
    PlayerActorState* playerActor;
    WheelMapState* mapState;
    PlayerActorState* playerActor2;

    roundState = &roundScoreState;
    spC = &playerCharacterStates[roundState->currentRoundIndex];
    playerActor = &playerActorStates[1];
    SetModelMotion(playerActor->playerObject, 0, 0, 10, 1);
    HuPrcChildCreate(ShowRoundScoreMessage, 100, 0x2000, 0, HuPrcCurrentGet());
    SelectRoundCameraView(3);
    mapState = &wheelMapState;
    playerActor2 = &playerActorStates[1];
    ResetMotionTarget(&playerActor2->turnTracker, 0);
    ReleaseMotionTarget(&playerActor2->turnTracker, -1);
    ClearMotionTargetModel(&playerActor2->turnTracker, -1);
}

s32 WaitForPlayerScoreResult(void) {
    RoundScoreState* roundState;
    PlayerCharacterState* playerState;

    roundState = &roundScoreState;
    if (gameEventState != 0 && UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
        playerState = &playerCharacterStates[roundState->currentRoundIndex];
        SetModelMotion(playerState->characterObject, 0, 0, 1, 1);
        if (roundState->currentRoundIndex == 3) {
            SetSequenceStep(&roundSequencePlayer, 11);
        } else {
            return AdvanceSequence(&roundSequencePlayer);
        }
    }
    return 0;
}

void SetPlayerTurnCamera(void) {
    RoundScoreState* roundState;

    roundState = &roundScoreState;
    SelectRoundCameraView(0);
}

s32 WaitForPlayerTurnCamera(void) {
    if (UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
        return AdvanceSequence(&roundSequencePlayer);
    }
    return 0;
}

void CapturePlayerIntroPositions(void) {
    RoundScoreState* roundState;
    PlayerCharacterState* playerState;
    s32 rotatingPlayerIndex;
    s32 playerIndex;

    roundState = &roundScoreState;
    rotatingPlayerIndex = roundState->currentRoundIndex + 1;
    if (rotatingPlayerIndex >= 4) {
        rotatingPlayerIndex = 0;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerState = &playerCharacterStates[rotatingPlayerIndex];
        playerStartingPositions[playerIndex].x = playerState->characterObject->trans.x;
        playerStartingPositions[playerIndex].y = playerState->characterObject->trans.y;
        playerStartingPositions[playerIndex].z = playerState->characterObject->trans.z;
        playerState->playerIntroAnimationFrames = 0;
        SetModelMotionIfChanged(playerState->characterObject, 0, 2, 10, 1);
        rotatingPlayerIndex++;
        if (rotatingPlayerIndex >= 4) {
            rotatingPlayerIndex = 0;
        }
    }
}

s32 AnimatePlayerTurnPositions(void) {
    RoundScoreState* roundState = &roundScoreState;
    PlayerCharacterState* playerState;
    s32 playerIndex;
    s32 rotatingPlayerIndex;
    Vec playerStartingPositions[] = {
        {   0.0f, 0.0f, 500.0f },
        { 200.0f, 0.0f, 300.0f },
        { 200.0f, 0.0f, 450.0f },
        { 200.0f, 0.0f, 600.0f }
    };

    rotatingPlayerIndex = roundState->currentRoundIndex + 1;
    if (rotatingPlayerIndex >= 4) {
        rotatingPlayerIndex = 0;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerState = &playerCharacterStates[rotatingPlayerIndex];
        if (playerState->playerIntroAnimationFrames++ <= 30) {
            UpdateModelTransformTweenPositionAndRotation(playerState->characterObject, 0, &playerStartingPositions[playerIndex], &playerStartingPositions[playerIndex], 5, playerState->playerIntroAnimationFrames, 30);
        } else {
            UpdateModelRotationTween(playerState->characterObject, 0, -90.0f, 10);
            if (playerState->playerIntroAnimationFrames >= 40) {
                SetModelMotionIfChanged(playerState->characterObject, 0, 0, 10, 1);
            }
        }
        rotatingPlayerIndex++;
        if (rotatingPlayerIndex == 4) {
            rotatingPlayerIndex = 0;
        }
    }
    if (UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
        roundState->currentRoundIndex++;
        if (roundState->currentRoundIndex >= 4) {
            roundState->currentRoundIndex = 0;
        }
        SetWheelEnvironmentState(0);
        SetAllPieceStates(0);
        SetSequenceStep(&roundSequencePlayer, 0);
    }
    return 0;
}

void StartPlayerActionMessages(void) {
    HuPrcChildCreate(StartPlayerActionMessages, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 WaitForPlayerActionMessages(void) {
    if (gameEventState != 0 && UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
        return AdvanceSequence(&roundSequencePlayer);
    }
    return 0;
}

void StartWheelResultMessages(void) {
    HuPrcChildCreate(StartWheelResultMessages, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 WaitForWheelResultMessage(void) {
    if (gameEventState != 0 && UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
        SetSequenceStep(&roundSequencePlayer, 12);
    }
    return 0;
}

void StartWheelAnnouncement(void) {
    HuPrcChildCreate(StartWheelAnnouncement, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 WaitForWheelAnnouncement(void) {
    if (gameEventState != 0 && UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
        return AdvanceSequence(&roundSequencePlayer);
    }
    return 0;
}

void PrepareFinalScoreDisplay(void) {
    RoundScoreState* roundState;
    GameUiState* uiState;
    ScoreDisplayState* scoreDisplay;
    s32 lowestScore;
    s32 playerIndex;

    lowestScore = 999;
    roundState = &roundScoreState;
    uiState = &gameUiState;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        SetScoreDisplayState(playerIndex, 0);
    }
    SetPrimaryPlayerAction(1);
    if (roundState->roundHasNegativeScore != 1) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            scoreDisplay = &scoreDisplayStates[playerIndex];
            if (lowestScore > scoreDisplay->scoreValue) {
                lowestScore = scoreDisplay->scoreValue;
                roundState->currentRoundIndex = playerIndex;
            }
        }
    }
    {
        PlayerCharacterState* playerState;
        PlayerActorState* nextPlayerState;
        Vec winnerPositions[] = {
            {    0.0f, 0.0f, -225.0f },
            { -200.0f, 0.0f, -225.0f },
            {  200.0f, 0.0f, -225.0f },
            {  400.0f, 0.0f, -225.0f }
        };

        lowestScore = roundState->currentRoundIndex;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerState = &playerCharacterStates[lowestScore];
            omSetTra(playerState->characterObject, winnerPositions[playerIndex].x, winnerPositions[playerIndex].y, winnerPositions[playerIndex].z);
            omSetRot(playerState->characterObject, 0.0f, 180.0f, 0.0f);
            SetModelMotion(playerState->characterObject, 0, 0, 10, 1);
            lowestScore++;
            if (lowestScore >= 4) {
                lowestScore = 0;
            }
            nextPlayerState = &playerActorStates[1];
            SetMotionTargetModel(&playerState->turnTracker, nextPlayerState->playerObject->mdlId[0]);
        }
    }
    SelectRoundCameraView(5);

    // Hack required to match.
    if (0) {
        s32 unreferencedMatchLocal25;
        s32 unreferencedMatchLocal26;

        (void) unreferencedMatchLocal26;
        (void) unreferencedMatchLocal26;
        (void) unreferencedMatchLocal26;
        (void) unreferencedMatchLocal26;
        (void) unreferencedMatchLocal25;
        (void) unreferencedMatchLocal25;
        (void) unreferencedMatchLocal25;
        (void) unreferencedMatchLocal25;
    }
}

// Unused. Mirrors the Vec array defined in prepare_final_score_display.
const Vec roundScorePositions[] = {
    {    0.0f, 0.0f, -225.0f },
    { -200.0f, 0.0f, -225.0f },
    {  200.0f, 0.0f, -225.0f },
    {  400.0f, 0.0f, -225.0f }
};

static inline void PreserveFinalScoreMatchRegisters(s32 playerIndex) {
    RoundScoreState* roundState;

    roundState = &roundScoreState;
    (void) roundState;
    (void) roundState;
    (void) roundState;
    (void) roundState;

    if (playerIndex) {
        s32 unreferencedMatchLocal31;
        s32 unreferencedMatchLocal30;
        s32 unreferencedMatchLocal26;
        s32 unreferencedMatchLocal23;
        s32 unreferencedMatchLocal22;
        s32 unreferencedMatchLocal21;
        s32 unreferencedMatchLocal20;
        s32 unreferencedMatchLocal19;
        s32 unreferencedMatchLocal18;
        s32 unreferencedMatchLocal17;

        (void) unreferencedMatchLocal31;
        (void) unreferencedMatchLocal31;
        (void) unreferencedMatchLocal31;
        (void) unreferencedMatchLocal31;
        (void) unreferencedMatchLocal31;
        (void) unreferencedMatchLocal31;
        (void) unreferencedMatchLocal30;
        (void) unreferencedMatchLocal30;
        (void) unreferencedMatchLocal30;
        (void) unreferencedMatchLocal30;
        (void) unreferencedMatchLocal30;
        (void) unreferencedMatchLocal30;
        (void) unreferencedMatchLocal26;
        (void) unreferencedMatchLocal26;
        (void) unreferencedMatchLocal26;
        (void) unreferencedMatchLocal26;
        (void) unreferencedMatchLocal23;
        (void) unreferencedMatchLocal23;
        (void) unreferencedMatchLocal23;
        (void) unreferencedMatchLocal22;
        (void) unreferencedMatchLocal22;
        (void) unreferencedMatchLocal22;
        (void) unreferencedMatchLocal21;
        (void) unreferencedMatchLocal21;
        (void) unreferencedMatchLocal21;
        (void) unreferencedMatchLocal20;
        (void) unreferencedMatchLocal20;
        (void) unreferencedMatchLocal20;
        (void) unreferencedMatchLocal19;
        (void) unreferencedMatchLocal19;
        (void) unreferencedMatchLocal19;
        (void) unreferencedMatchLocal18;
        (void) unreferencedMatchLocal18;
        (void) unreferencedMatchLocal18;
        (void) unreferencedMatchLocal17;
        (void) unreferencedMatchLocal17;
        (void) unreferencedMatchLocal17;
    }
}

s32 UpdateFinalScoreDisplay(void) {
    GameUiState* uiState;
    WheelMapState* mapState;
    PlayerCharacterState* playerStateByIndex;
    s32 playerIndex;
    s32 modelIndex;

    PreserveFinalScoreMatchRegisters(0); // Required to match.
    if (UpdateSequenceTimer(&roundSequencePlayer, 0) == 180) {
        uiState = &gameUiState;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerStateByIndex = &playerCharacterStates[playerIndex];
            SetMotionTargetModel(&playerStateByIndex->turnTracker, uiState->uiObject->mdlId[0]);
        }
    }
    if (UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
        mapState = &wheelMapState;
        for (modelIndex = 0; modelIndex < 14; modelIndex++) {
            Hu3DModelAttrSet(mapState->mapObject->mdlId[modelIndex], 1);
        }
        return AdvanceSequence(&roundSequencePlayer);
    }
    return 0;
}

void StartWinnerAnnouncement(void) {
    gameEventState = 0;
    HuPrcChildCreate(ShowWinnerAndLoserMessages, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 UpdateWinnerAnnouncement(void) {
    s32 rotatingPlayerIndex;
    s32 playerIndex;
    RoundScoreState* roundState;
    PlayerCharacterState* playerState;

    roundState = &roundScoreState;
    rotatingPlayerIndex = roundState->currentRoundIndex;
    if (rotatingPlayerIndex == 4) {
        rotatingPlayerIndex = 0;
    }
    if (gameEventState != 0 && UpdateSequenceTimer(&roundSequencePlayer, 0) == 20) {
        SelectRoundCameraView(6);
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerIndex != 0) {
                playerState = &playerCharacterStates[rotatingPlayerIndex];
                playerState->aiRouteIndex = playerIndex;
                playerState->routeDelayFrames = 15 - playerIndex * 5;
                playerState->routeFrame = 0;
            }
            rotatingPlayerIndex++;
            if (rotatingPlayerIndex >= 4) {
                rotatingPlayerIndex = 0;
            }
        }
    }
    if (gameEventState != 0 && UpdateSequenceTimer(&roundSequencePlayer, 1) != 0) {
        return AdvanceSequence(&roundSequencePlayer);
    }
    return 0;
}

SequenceStep mainSequenceSteps[] = {
    { EmptyRoundSequenceCallback, WaitForRoundSequence,   1 },
    { StartRoundWrapUp, UpdateRoundWrapUp, 120 },
    { SetRoundCameraViewIntro, WaitForRoundIntroCamera,  90 },
    { StartPlayerActionCountdown, WaitForPlayerAction, 120 },
    { ResetRoundScore, CollectPieceScores,  -1 },
    { PlayPlayerScoreAnimation, WaitForScoreAnimation, 120 },
    { ShowPlayerScoreResult, WaitForPlayerScoreResult,  60 },
    { SetPlayerTurnCamera, WaitForPlayerTurnCamera,  30 },
    { CapturePlayerIntroPositions, AnimatePlayerTurnPositions, 120 },
    { StartPlayerActionMessages, WaitForPlayerActionMessages,  60 },
    { StartWheelResultMessages, WaitForWheelResultMessage,  60 },
    { StartWheelAnnouncement, WaitForWheelAnnouncement,  60 },
    { PrepareFinalScoreDisplay, UpdateFinalScoreDisplay, 300 },
    { StartWinnerAnnouncement, UpdateWinnerAnnouncement,  60 },
    {       NULL,       NULL,  -1 }
};

void StartMainSequence(void) {
    StartSequence(&roundSequencePlayer, mainSequenceSteps);
    SetSequenceStep(&roundSequencePlayer, 3);
}

s32 UpdateMainSequenceCallbacks(void) {
    s32 sequenceFinished;

    sequenceFinished = 0;
    if (roundSequencePlayer.onStep != NULL) {
        roundSequencePlayer.onStep();
        roundSequencePlayer.onStep = NULL;
    }
    if (roundSequencePlayer.onComplete != NULL) {
        sequenceFinished = roundSequencePlayer.onComplete();
    }
    return sequenceFinished;
}

void StartCharacterIntroSequence(void) {
    RoundScoreState* roundState;
    PlayerCharacterState* playerState;

    roundState = &roundScoreState;
    playerState = &playerCharacterStates[roundState->currentRoundIndex];
    roundState->unreferencedRoundState1C = 0;
    currentCharacterMotionId = playerState->characterObject->mdlId[0];
    activeCharacterMotionId = playerState->characterObject->mtnId[3];
    activeCharacterId = playerState->characterId;
    HuPrcChildCreate(PlayCharacterMotionEvents, 100, 0x2000, 0, HuPrcCurrentGet());
}

s32 UpdateCharacterIntroSequence(void) {
    RoundScoreState* roundState;
    PlayerCharacterState* playerState;

    roundState = &roundScoreState;
    playerState = &playerCharacterStates[roundState->currentRoundIndex];
    if (UpdateSequenceTimer(&characterIntroSequencePlayer, 0) == 120) {
        SetModelMotion(playerState->characterObject, 0, 3, 30, 0);
        HuAudSStreamPlay(3);
    }
    if (UpdateSequenceTimer(&characterIntroSequencePlayer, 1) != 0) {
        return AdvanceSequence(&characterIntroSequencePlayer);
    }
    return 0;
}

SequenceStep characterIntroSequenceSteps[] = {
    { StartCharacterIntroSequence, UpdateCharacterIntroSequence, 360 },
    {       NULL,       NULL,  -1 }
};

void StartCharacterIntro(void) {
    StartSequence(&characterIntroSequencePlayer, characterIntroSequenceSteps);
}

s32 UpdateCharacterIntroCallbacks(void) {
    s32 sequenceFinished;

    sequenceFinished = 0;
    if (characterIntroSequencePlayer.onStep != NULL) {
        characterIntroSequencePlayer.onStep();
        characterIntroSequencePlayer.onStep = NULL;
    }
    if (characterIntroSequencePlayer.onComplete != NULL) {
        sequenceFinished = characterIntroSequencePlayer.onComplete();
    }
    return sequenceFinished;
}

void ReturnToMinigameSelect(OMOBJ* minigameObject) {
    RoundScoreState* roundState;
    PlayerCharacterState* playerState;
    s32 i;

    if (WipeStatGet() == 0) {
        CharModelKill(-1);
        MGSeqKillAll();
        HuAudFadeOut(1);
        roundState = &roundScoreState;
        playerState = &playerCharacterStates[roundState->currentRoundIndex];
        for (i = 0; i < 4; i++) {
            GWPlayerCoinWinSet(i, 0);
        }
        GWPlayerCoinWinSet(playerState->playerIndex, 1);
        omOvlReturnEx(1, 1);
        minigameObject->objFunc = NULL;
    }
}

void HandleMinigameExit(OMOBJ* minigameObject) {
    if (omSysExitReq == 1) {
        HuAudSeqAllFadeOut(250);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
        minigameObject->objFunc = ReturnToMinigameSelect;
    }
}

void InitializeMinigameState(void) {
    RoundScoreState* roundState;
    s32 i;

    InitializeWheelModels(&gameAnimationState);
    InitializeWheelMap(&wheelMapState);
    InitializeUiObject(&gameUiState);
    for (i = 0; i < 2; i++) {
        InitializePlayerActor(&playerActorStates[i]);
    }
    for (i = 0; i < 4; i++) {
        InitializeCharacterState(&playerCharacterStates[i]);
    }
    for (i = 0; i < 4; i++) {
        InitializeScoreDisplay(&scoreDisplayStates[i]);
    }
    InitializeAudioEffects(&audioVolumeState);
    roundState = &roundScoreState;
    roundState->scoreCollectionState = roundState->roundScore = roundState->roundHasNegativeScore = roundState->roundWrapUpState = 0;
    roundState->currentRoundIndex = roundState->activeRoundCount = roundState->unreferencedRoundState18 = roundState->unreferencedRoundState1C = 0;
    StartRoundSequence();
    StartMainSequence();
    StartCharacterIntro();
    SelectCameraView(0);
    HuAudFXPlay(0x6D7);
}

void UpdateMinigameState(OMOBJ* minigameObject) {
    GameAnimationState* animationState;

    switch (mainGameState) {
        case 0:
            InitializeMinigameState();
            mainGameState = 1;
            break;
        case 1:
            if (WipeStatGet() == 0) {
                mainGameState = 2;
            }
            break;
        case 2:
            if (UpdateRoundSequenceCallbacks()) {
                mainGameState = 3;
            }
            break;
        case 3:
            minigameSequenceId = MGSeqStartCreate();
            mainGameState = 4;
            sequenceSoundIds[1] = -1;
            break;
        case 4:
            if ((MGSeqStatGet(minigameSequenceId) & 0x10) && sequenceSoundIds[1] == -1) {
                sequenceSoundIds[1] = HuAudSeqPlay(0x4D);
            }
            if (MGSeqStatGet(minigameSequenceId) == 0) {
                mainGameState = 5;
            }
            break;
        case 5:
            if (UpdateMainSequenceCallbacks()) {
                mainGameState = 6;
            }
            break;
        case 6:
            minigameSequenceId = MGSeqFinishCreate();
            mainGameState = 7;
            HuAudSeqPauseAll(0);
            HuAudSeqAllFadeOut(250);
            break;
        case 7:
            if (MGSeqStatGet(minigameSequenceId) == 0) {
                mainGameState = 8;
            }
            break;
        case 8:
            animationState = &gameAnimationState;
            animationState->mainAnimationState = 999;
            if (UpdateCharacterIntroCallbacks()) {
                mainGameState = 9;
            }
            break;
        case 9:
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
            minigameObject->objFunc = ReturnToMinigameSelect;
            break;
    }
    HandleMinigameExit(minigameObject);

    // Hack required to match.
    if (0) {
        s32 unreferencedMatchLocal31;
        s32 unreferencedMatchLocal28;

        (void) unreferencedMatchLocal31;
        (void) unreferencedMatchLocal31;
        (void) unreferencedMatchLocal31;
        (void) unreferencedMatchLocal31;
        (void) unreferencedMatchLocal28;
        (void) unreferencedMatchLocal28;
        (void) unreferencedMatchLocal28;
    }
}

void CreateMinigameObjects(OMOBJ* minigameObject) {
    s32 i;

    gameAnimationState.mainObject = omAddObjEx(objectManager, 0x200, 16, 16, -1, CreateWheelModels);
    wheelMapState.mapObject = omAddObjEx(objectManager, 0x300, 32, 32, -1, CreateWheelMap);
    gameUiState.uiObject = omAddObjEx(objectManager, 0x400, 16, 16, -1, CreateUiObject);
    for (i = 0; i < 2; i++) {
        playerActorStates[i].playerObject = omAddObjEx(objectManager, 0x500, 16, 16, -1, CreatePlayerActor);
        playerActorStates[i].playerObject->work[0] = i;
    }
    for (i = 0; i < 4; i++) {
        playerCharacterStates[i].characterObject = omAddObjEx(objectManager, 0x50, 16, 16, -1, CreateCharacterModel);
        playerCharacterStates[i].characterObject->work[0] = i;
    }
    for (i = 0; i < 4; i++) {
        scoreDisplayStates[i].spriteObject = omAddObjEx(objectManager, 0x800, 16, 16, -1, CreateScoreDisplay);
        scoreDisplayStates[i].spriteObject->work[0] = i;
    }
    audioVolumeState.playerObject = omAddObjEx(objectManager, 0x700, 16, 16, -1, CreateAudioEffects);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    minigameObject->objFunc = UpdateMinigameState;
}

#ifdef __MWERKS__
#include "src/REL/executor.c"
#endif

void StartMinigame(void) {
    objectManager = omInitObjMan(62, 0x2000);
    omGameSysInit(objectManager);
    CreateGlobalLight();
    InitializeCamera();
    InitializeShadow();
    InitializeWindows();
    InitializeResultCountdown();
    InitializePlayerCharacterStates();
    omAddObjEx(objectManager, 0x1000, 0, 0, -1, CreateMinigameObjects);
}

void ObjectSetup(void) {
    StartMinigame();
}

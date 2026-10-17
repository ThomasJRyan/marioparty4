#include "REL/mstory3Dll.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/esprite.h"
#include "game/flag.h"
#include "game/gamework.h"
#include "game/gamework_data.h"

#ifndef __MWERKS__
#include "game/audio.h"
#endif

#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "msm/msmse.h"

#include "ext_math.h"

#ifndef __MWERKS__
extern s32 rand8(Void);

Void Story3CreateParticleTextEffect(s32 animationFileId, Vec* emissionOrigin, Vec* destinationCenter, s16 lifetimeFrames);
Void Story3MarkParticleTextEffectReady(Void);
#endif

typedef struct {
    /* 0x00 */ s32 firstLightId;
    /* 0x04 */ s32 secondLightId;
    /* 0x08 */ char unreferencedAnimationProcessPadding[0xC];
} PlayerLightState; // Size 0x14

typedef struct {
    /* 0x00 */ StoryPlayerStats playerStats;
    /* 0x34 */ char unreferencedPlayerResultPadding[0x60];
} PlayerSequenceResult; // Size 0x94

typedef struct {
    /* 0x000 */ s32 boardId;
    /* 0x004 */ s32 bonusStarMode;
    /* 0x008 */ s32 sequenceVariant;
    /* 0x00C */ s32 currentPlayerIndex;
    /* 0x010 */ s32 audioSequenceId;
    /* 0x014 */ s32 winningPlayerIndex;
    /* 0x018 */ s32 sequenceState;
    /* 0x01C */ s32 globalLightId;
    /* 0x020 */ s32 globalLightBrightness;
    /* 0x024 */ PlayerLightState playerLights[4];
    /* 0x074 */ char unreferencedSequencePadding74[0x50];
    /* 0x0C4 */ PlayerSequenceResult playerResults[4];
    /* 0x314 */ s32 playerObjectCount;
    /* 0x318 */ char unreferencedSequencePadding318[0xC];
    /* 0x324 */ s32 sequenceProgress;
    /* 0x328 */ char unreferencedSequencePadding328[0xC];
    /* 0x334 */ s32 playerParticleManagers[4];
    /* 0x338 */ char unreferencedSequencePadding338[0xC];
    /* 0x344 */ s32 teamMode;
    /* 0x348 */ s32 winningTeam;
    /* 0x34C */ char unreferencedSequencePadding[4];
} StoryResultSequenceState; // Size 0x350

typedef struct {
    /* 0x00 */ s32 animationState;
    /* 0x04 */ s32 framesUntilNextBurst;
    /* 0x08 */ s32 animationFrame;
    /* 0x0C */ char unreferencedParticlePadding[4];
    /* 0x10 */ float rotationVelocityX;
    /* 0x14 */ float rotationVelocityY;
    /* 0x18 */ float rotationVelocityZ;
    /* 0x1C */ char unreferencedParticlePadding2[4];
    /* 0x20 */ float horizontalDriftVelocity;
    /* 0x24 */ float verticalDriftVelocity;
    /* 0x28 */ float depthDriftVelocity;
    /* 0x2C */ float targetX;
    /* 0x30 */ float targetY;
    /* 0x34 */ float targetZ;
} ResultParticleMotionState; // Size 0x38

Void Story3UpdatePlayerResultLighting(s32 playerLightHandle, s32 targetAlpha, s32 frame, s32 duration, s32 playerIndex);
Void Story3ResetSequenceCamera(Void);
s32 Story3DetermineWinningTeam(Void);
Void Story3CreateResultLogoObject(Void);
Void Story3UpdateResultAmbientLight(OMOBJ* object);
Void Story3AnimateResultLogos(OMOBJ* object);
Void Story3CreateResultObjects(Void);
Void Story3SortPlayersForPresentation(s32 playerCount);
Void Story3AnimatePlayerMedal(OMOBJ* object);
Void Story3AnimateTeamMedal(OMOBJ* object);
Void Story3AwardPlayerStar(OMOBJ* object);
Void Story3AnimateAwardedStarParticles(OMOBJ* object);
Void Story3FinishPlayerResultProcess(OMOBJ* object);
Void Story3NoopResultCallback(OMOBJ* object);
Void Story3UpdatePlayerRankMarker(OMOBJ* object);
Void Story3FindTopMinigameScores(s32 playerIndex);
Void Story3FindTiedMinigameScores(s32 playerIndex);
Void Story3SortPlayersByMinigameScore(Void);
Void Story3CreatePlayerResultObject(Void);
Void Story3CountFirstPlacePlayers(Void);
Void Story3CalculatePlayerMinigameScores(Void);
Void Story3NoopSequenceCallback(OMOBJ* object);
Void Story3ShowResultMessageSequence(s32 windowId, s32 messageId, s32 mode, s32 mode3);
Void Story3ShowBoardResultMessage(s32 windowId, s32 messageId, s32 mode);
Void Story3InsertCharacterNameMessage(s32 windowId, s32 messageId, s32 mode);
Void Story3CreateSharedSequenceObject(Void);
Void Story3StartPlayerPresentationProcess(s32 playerIndex);

StoryResultSequenceState resultSequenceState;
StorySequenceProcessState resultProcessCallbackState;
StorySequenceProcessState sequenceCallbackState;
StorySequenceProcessState playerSequenceProcessStates[4];
StorySequenceProcessState sharedSequenceProcessState;
s32 sequencePlayerCountAndWinner[4];
s32 playerSequenceOrder[4];
s32 playerSequenceResultIndices[4];
s32 playerSequenceScoreOrder[4];
float cameraFocusCenterX;
s32 audioSequenceMode;
float logoRotationState[3][2];
ResultParticleMotionState resultParticleMotionStates[88];
float logoFadeAlpha;
float particleEffectEnabled;
float particleEffectCenterX;
s32 resultAnimationFrame;
s32 cameraTargetCharacterId;
float resultAnimationActive;
float playerModelVerticalOffset;
HUPROCESS* resultObjectManager;
s32 resultSoundEffectIds[16];
u8 unreferencedResultSequenceData[0xC]; // unused
s32 resultAudioSequenceId;

Vec playerModelPositions[] = {
    { -300.0f, 0.0f, 460.0f },
    { -100.0f, 0.0f, 460.0f },
    {  100.0f, 0.0f, 460.0f },
    {  300.0f, 0.0f, 460.0f }
};

HU3DPARMANPARAM starParticleParams = {
    60, { 0, 0 },
    1000.0f, 0.0f, 180.0f,
    { 0.0f, -0.009f, 0.0f },
    30.0f, 0.9f, 20.0f, 0.99f, 4,
    {
        { 0xFF, 0xFF, 0x00, 0xFF },
        { 0xFF, 0xFF, 0x20, 0xFF },
        { 0xFF, 0xFF, 0x80, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF }
    },
    {
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 }
    }
};

HU3DPARMANPARAM glowParticleParams = {
    60, { 0, 0 },
    1000.0f, 0.0f, 180.0f,
    { 0.0f, -0.009f, 0.0f },
    30.0f, 0.9f, 20.0f, 0.99f, 4,
    {
        { 0xFF, 0xFF, 0x00, 0xFF },
        { 0xFF, 0xFF, 0x20, 0xFF },
        { 0xFF, 0xFF, 0x80, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF }
    },
    {
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 }
    }
};

HU3DPARMANPARAM scoreParticleParams = {
    500, { 0, 0 },
    0.25f, 0.0f, 30.0f,
    { 0.0f, -0.009f, 0.001f },
    30.0f, 0.9f, 20.0f, 0.999f, 4,
    {
        { 0xFF, 0xFF, 0x00, 0xFF },
        { 0xFF, 0xFF, 0x20, 0xFF },
        { 0xFF, 0xFF, 0x80, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF }
    },
    {
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 }
    }
};


Void Story3InitializeResultSequence(HUPROCESS* objectManager, s32 boardId, s32 bonusStarMode, s32 sequenceVariant, s32 playerPresentationMode) {
    s32 playerIndex;

    resultObjectManager = objectManager;
    resultSequenceState.boardId = boardId;
    resultSequenceState.bonusStarMode = bonusStarMode;
    if (sequenceVariant == 1) {
        resultSequenceState.sequenceVariant = 0;
    } else {
        resultSequenceState.sequenceVariant = 1;
        resultSequenceState.bonusStarMode = 1;
    }
    resultSequenceState.winningPlayerIndex = story3CountHumanPlayers();
    resultSequenceState.teamMode = GWSystem.team;
    _ClearFlag(FLAG_ID_MAKE(1, 29));
    resultSequenceState.winningTeam = -1;
    Story3InitializeWindows();
    Story3InitializeResultCamera(resultObjectManager, Story3ResetSequenceCamera);
    resultSequenceState.globalLightId = story3CreateGlobalLight(resultSequenceState.globalLightBrightness = 0xFF);
    Story3SortPlayersForPresentation(playerPresentationMode);
    Story3SetDefaultCamera();
    Story3SortPlayersByMinigameScore();
    HuPrcChildCreate(Story3CreateResultObjects, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(Story3CreateSharedSequenceObject, 100, 0x2000, 0, HuPrcCurrentGet());
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        HuPrcChildCreate(Story3CreatePlayerResultObject, 200, 0x2000, 0, HuPrcCurrentGet());
    }
    HuPrcChildCreate(Story3CreateResultLogoObject, 100, 0x2000, 0, HuPrcCurrentGet());
}



Void Story3SetPlayerResultLights(s32 targetBrightness, s32 animationDuration, s32 updatePlayerLights) {
    s32 playerLightHandles[4];
    s32 playerLightAlphas[4];
    s32 initialBrightness;
    s32 animationFrame;
    s32 playerIndex;

    initialBrightness = resultSequenceState.globalLightBrightness;
    sequenceCallbackState.callback = (Void*) Story3UpdateResultAmbientLight;
    for (animationFrame = 0; animationFrame < 4; animationFrame++) {
        playerSequenceProcessStates[animationFrame].callback = (Void*) Story3NoopResultCallback;
    }
    sharedSequenceProcessState.callback = (Void*) Story3NoopSequenceCallback;
    if (updatePlayerLights != 0) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerLightHandles[playerIndex] = playerSequenceProcessStates[playerIndex].object->work[0];
            playerLightAlphas[playerIndex] = 0xFF;
        }
    }
    for (animationFrame = 0; animationFrame < animationDuration; animationFrame++) {
        Story3SleepOneFrame();
        resultSequenceState.globalLightBrightness = interpolateLinearOverTime(initialBrightness, targetBrightness, animationFrame, animationDuration);
        Hu3DGLightColorSet(resultSequenceState.globalLightId, resultSequenceState.globalLightBrightness, resultSequenceState.globalLightBrightness, resultSequenceState.globalLightBrightness, 0xFF);
        if (updatePlayerLights != 0) {
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                Story3UpdatePlayerResultLighting(playerLightHandles[playerIndex], playerLightAlphas[playerIndex], animationFrame, animationDuration, playerIndex);
            }
        }
    }
    if (updatePlayerLights != 0) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerSequenceProcessStates[playerIndex].object->work[0] = 0;
            playerSequenceProcessStates[playerIndex].object->work[1] = 0;
            Hu3DModelAttrSet(playerSequenceProcessStates[playerIndex].object->mdlId[6], 1);
            Hu3DLLightColorSet(playerSequenceProcessStates[playerIndex].object->mdlId[1], resultSequenceState.playerLights[playerIndex].firstLightId, 0, 0, 0, 0xFF);
            Hu3DLLightColorSet(playerSequenceProcessStates[playerIndex].object->mdlId[2], resultSequenceState.playerLights[playerIndex].secondLightId, 0, 0, 0, 0xFF);
        }
    }
    sequenceCallbackState.callback = NULL;
    for (animationFrame = 0; animationFrame < 4; animationFrame++) {
        playerSequenceProcessStates[animationFrame].callback = NULL;
    }
    sharedSequenceProcessState.callback = NULL;
}

Void Story3AnimatePairGlow(s32 index, s32 count, s32 mode, s32 option) {
    float glowScale;
    float glowAlpha;
    s32 initialGlow;
    s32 initialBrightness;
    s32 lightBrightness;
    s32 animationFrame;

    initialGlow = playerSequenceProcessStates[mode].object->work[0];
    initialBrightness = index;
    Hu3DModelPosSet(playerSequenceProcessStates[mode].object->mdlId[6], playerModelPositions[mode].x, 500.0f + playerModelPositions[mode].y + playerModelVerticalOffset, playerModelPositions[mode].z + 0.1f * playerModelVerticalOffset);
    Hu3DModelScaleSet(playerSequenceProcessStates[mode].object->mdlId[6], 0.0f, 1.25f, 0.0f);
    Hu3DModelTPLvlSet(playerSequenceProcessStates[mode].object->mdlId[6], 0.0f);
    Hu3DModelAttrReset(playerSequenceProcessStates[mode].object->mdlId[6], 1);
    Hu3DModelPosSet(playerSequenceProcessStates[option].object->mdlId[6], playerModelPositions[option].x, 500.0f + playerModelPositions[option].y + playerModelVerticalOffset, playerModelPositions[option].z + 0.1f * playerModelVerticalOffset);
    Hu3DModelScaleSet(playerSequenceProcessStates[option].object->mdlId[6], 0.0f, 1.25f, 0.0f);
    Hu3DModelTPLvlSet(playerSequenceProcessStates[option].object->mdlId[6], 0.0f);
    Hu3DModelAttrReset(playerSequenceProcessStates[option].object->mdlId[6], 1);
    for (animationFrame = 0; animationFrame <= count; animationFrame++) {
        Story3SleepOneFrame();
        glowScale = interpolateSineEaseIn(0.0f, 1.5f, animationFrame, count);
        glowAlpha = interpolateSineEaseIn(0.0f, 0.5f, animationFrame, count);
        lightBrightness = interpolateSineEaseIn(0.0f, 128.0f, animationFrame, count);
        Hu3DModelScaleSet(playerSequenceProcessStates[mode].object->mdlId[6], glowScale, 1.25f, glowScale);
        Hu3DModelTPLvlSet(playerSequenceProcessStates[mode].object->mdlId[6], glowAlpha);
        Hu3DModelScaleSet(playerSequenceProcessStates[option].object->mdlId[6], glowScale, 1.25f, glowScale);
        Hu3DModelTPLvlSet(playerSequenceProcessStates[option].object->mdlId[6], glowAlpha);
        Hu3DLLightColorSet(playerSequenceProcessStates[mode].object->mdlId[1], resultSequenceState.playerLights[mode].firstLightId, lightBrightness, lightBrightness, lightBrightness, 0xFF);
        Hu3DLLightColorSet(playerSequenceProcessStates[mode].object->mdlId[2], resultSequenceState.playerLights[mode].secondLightId, lightBrightness, lightBrightness, lightBrightness, 0xFF);
        Hu3DLLightColorSet(playerSequenceProcessStates[option].object->mdlId[1], resultSequenceState.playerLights[option].firstLightId, lightBrightness, lightBrightness, lightBrightness, 0xFF);
        Hu3DLLightColorSet(playerSequenceProcessStates[option].object->mdlId[2], resultSequenceState.playerLights[option].secondLightId, lightBrightness, lightBrightness, lightBrightness, 0xFF);
    }
    playerSequenceProcessStates[mode].object->work[1] = 1;
    playerSequenceProcessStates[option].object->work[1] = 1;
}

Void Story3AnimatePlayerGlow(s32 playerIndex, s32 count, s32 mode) {
    float glowScale;
    float glowAlpha;
    s32 initialGlow;
    s32 targetGlow;
    s32 lightBrightness;
    s32 animationFrame;

    initialGlow = playerSequenceProcessStates[mode].object->work[0];
    targetGlow = playerIndex;
    Hu3DModelPosSet(playerSequenceProcessStates[mode].object->mdlId[6], playerModelPositions[mode].x, 500.0f + playerModelPositions[mode].y + playerModelVerticalOffset, playerModelPositions[mode].z + 0.1f * playerModelVerticalOffset);
    Hu3DModelScaleSet(playerSequenceProcessStates[mode].object->mdlId[6], 0.0f, 1.25f, 0.0f);
    Hu3DModelTPLvlSet(playerSequenceProcessStates[mode].object->mdlId[6], 0.0f);
    Hu3DModelAttrReset(playerSequenceProcessStates[mode].object->mdlId[6], 1);
    for (animationFrame = 0; animationFrame <= count; animationFrame++) {
        Story3SleepOneFrame();
        playerSequenceProcessStates[mode].object->work[0] = interpolateSineEaseIn(initialGlow, targetGlow, animationFrame, count);
        glowScale = interpolateSineEaseIn(0.0f, 1.5f, animationFrame, count);
        glowAlpha = interpolateSineEaseIn(0.0f, 0.5f, animationFrame, count);
        lightBrightness = interpolateSineEaseIn(0.0f, 128.0f, animationFrame, count);
        Hu3DModelScaleSet(playerSequenceProcessStates[mode].object->mdlId[6], glowScale, 1.25f, glowScale);
        Hu3DModelTPLvlSet(playerSequenceProcessStates[mode].object->mdlId[6], glowAlpha);
        Hu3DLLightColorSet(playerSequenceProcessStates[mode].object->mdlId[1], resultSequenceState.playerLights[mode].firstLightId, lightBrightness, lightBrightness, lightBrightness, 0xFF);
        Hu3DLLightColorSet(playerSequenceProcessStates[mode].object->mdlId[2], resultSequenceState.playerLights[mode].secondLightId, lightBrightness, lightBrightness, lightBrightness, 0xFF);
    }
    playerSequenceProcessStates[mode].object->work[1] = 1;
}

Void Story3UpdatePlayerResultLighting(s32 playerLightHandle, s32 targetAlpha, s32 frame, s32 duration, s32 playerIndex) {
    float glowAlpha;
    s32 lightBrightness;

    if (playerSequenceProcessStates[playerIndex].object->work[1] != 0) {
        playerSequenceProcessStates[playerIndex].object->work[0] = interpolateLinearOverTime(playerLightHandle, targetAlpha, frame, duration);
        glowAlpha = interpolateLinearOverTime(0.5f, 0.0f, frame, duration);
        Hu3DModelTPLvlSet(playerSequenceProcessStates[playerIndex].object->mdlId[6], glowAlpha);
        lightBrightness = interpolateLinearOverTime(128.0f, 0.0f, frame, duration);
        Hu3DLLightColorSet(playerSequenceProcessStates[playerIndex].object->mdlId[1], resultSequenceState.playerLights[playerIndex].firstLightId, lightBrightness, lightBrightness, lightBrightness, 0xFF);
        Hu3DLLightColorSet(playerSequenceProcessStates[playerIndex].object->mdlId[2], resultSequenceState.playerLights[playerIndex].secondLightId, lightBrightness, lightBrightness, lightBrightness, 0xFF);
    }
}

s32 Story3CreateParticleManager(s32 particleIndex) {
    s32 particleManagerId;

    if (particleIndex == 0) {
        particleManagerId = Hu3DParManCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_EFFECT, 1)), 100, &starParticleParams);
    } else if (particleIndex == 1) {
        particleManagerId = Hu3DParManCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_EFFECT, 1)), 500, &glowParticleParams);
    } else if (particleIndex == 2) {
        particleManagerId = Hu3DParManCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_EFFECT, 1)), 250, &scoreParticleParams);
    }
    if (particleIndex != 2) {
        Hu3DParManTimeLimitSet(particleManagerId, 60);
        Hu3DParManAttrSet(particleManagerId, 4 | 0x40);
        Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(particleManagerId), 1);
    } else {
        Hu3DParManAttrSet(particleManagerId, 4 | 0x40);
    }
    return particleManagerId;
}

s32 Story3CreateStandardParticleManager(Void) {
    return Story3CreateParticleManager(0);
}

Void Story3ResetSequenceCamera(Void) {
    StoryCameraState* cameraState;

    cameraState = &storyCameraState;
    cameraState->cameraCenter.x = 0.0f;
    cameraState->cameraCenter.y = 125.0f;
    cameraState->cameraCenter.z = 0.0f;
    cameraState->cameraRotation.x = -5.0f;
    cameraState->cameraRotation.y = 0.0f;
    cameraState->cameraRotation.z = 0.0f;
    cameraState->cameraZoom = 1750.0f;
}

Void Story3AimResultCamera(Void) {
    StoryCameraState* cameraState;
    StoryCameraState cameraTarget;

    cameraState = &storyCameraState;
    cameraTarget.cameraCenter.x = 0.0f;
    cameraTarget.cameraCenter.y = 170.0f;
    cameraTarget.cameraCenter.z = 0.0f;
    cameraTarget.cameraRotation.x = -5.0f;
    cameraTarget.cameraRotation.y = 0.0f;
    cameraTarget.cameraRotation.z = 0.0f;
    cameraTarget.cameraZoom = 1700.0f;
    Story3EaseAndSmoothCamera(cameraState, &cameraTarget, cameraState->cameraMotionFrame++, 60.0f, 30.0f);
}

Void Story3FocusCameraOnPlayer(Void) {
    StoryCameraState* cameraState = &storyCameraState;
    StoryCameraState cameraTarget;
    float playerCameraHeights[] = {
        530.0f, 530.0f, 540.0f, 530.0f,
        530.0f, 580.0f, 540.0f, 630.0f
    };

    if (resultSequenceState.teamMode == 1) {
        if (resultSequenceState.winningTeam == 0) {
            cameraTarget.cameraCenter.x = 100.0f + Hu3DData[playerSequenceProcessStates[0].object->mdlId[1]].pos.x;
        } else {
            cameraTarget.cameraCenter.x = 100.0f + Hu3DData[playerSequenceProcessStates[2].object->mdlId[1]].pos.x;
        }
    } else {
        cameraTarget.cameraCenter.x = Hu3DData[playerSequenceProcessStates[sequencePlayerCountAndWinner[1]].object->mdlId[1]].pos.x;
        cameraTargetCharacterId = resultSequenceState.playerResults[sequencePlayerCountAndWinner[1]].playerStats.characterId;
    }
    if (resultSequenceState.teamMode == 1) {
        cameraTarget.cameraCenter.y = 570.0f;
        cameraTarget.cameraCenter.z = -700.0f;
        cameraTarget.cameraRotation.x = 15.0f;
        cameraTarget.cameraRotation.y = 0.0f;
        cameraTarget.cameraRotation.z = 0.0f;
        cameraTarget.cameraZoom = 1750.0f;
        Story3HandleSystemExitC0(cameraState, &cameraTarget, cameraState->cameraMotionFrame++, 120.0f);
    } else {
        cameraTarget.cameraCenter.y = playerCameraHeights[cameraTargetCharacterId];
        cameraTarget.cameraCenter.z = -700.0f;
        cameraTarget.cameraRotation.x = 15.0f;
        cameraTarget.cameraRotation.y = 0.0f;
        cameraTarget.cameraRotation.z = 0.0f;
        cameraTarget.cameraZoom = 1600.0f;
        Story3HandleSystemExitC0(cameraState, &cameraTarget, cameraState->cameraMotionFrame++, 120.0f);
    }
}

Void Story3TransitionCameraToResult(Void) {
    StoryCameraState* cameraState;
    StoryCameraState cameraTarget;

    cameraState = &storyCameraState;
    cameraTarget.cameraCenter.x = cameraFocusCenterX;
    cameraTarget.cameraCenter.y = 125.0f;
    cameraTarget.cameraCenter.z = 0.0f;
    cameraTarget.cameraRotation.x = -5.0f;
    cameraTarget.cameraRotation.y = 0.0f;
    cameraTarget.cameraRotation.z = 0.0f;
    cameraTarget.cameraZoom = 1250.0f;
    cameraTarget.cameraZoom = 1350.0f;
    Story3EaseAndSmoothCamera(cameraState, &cameraTarget, cameraState->cameraMotionFrame++, 10.0f, 5.0f);
}

Void Story3TransitionCameraToTeam(Void) {
    StoryCameraState* cameraState;
    StoryCameraState cameraTarget;

    cameraState = &storyCameraState;
    cameraTarget.cameraCenter.x = 0.0f;
    cameraTarget.cameraCenter.y = 170.0f;
    cameraTarget.cameraCenter.z = 0.0f;
    cameraTarget.cameraRotation.x = -5.0f;
    cameraTarget.cameraRotation.y = 0.0f;
    cameraTarget.cameraRotation.z = 0.0f;
    cameraTarget.cameraZoom = 1700.0f;
    Story3EaseAndSmoothCamera(cameraState, &cameraTarget, cameraState->cameraMotionFrame++, 30.0f, 10.0f);
}

Void Story3TransitionCameraToWinner(Void) {
    StoryCameraState* cameraState;
    StoryCameraState cameraTarget;

    cameraState = &storyCameraState;
    cameraTarget.cameraCenter.x = 300.0f;
    cameraTarget.cameraCenter.y = 100.0f;
    cameraTarget.cameraCenter.z = 0.0f;
    cameraTarget.cameraRotation.x = -5.0f;
    cameraTarget.cameraRotation.y = 0.0f;
    cameraTarget.cameraRotation.z = 0.0f;
    cameraTarget.cameraZoom = 1000.0f;
    Story3LerpAndSmoothCamera(cameraState, &cameraTarget, cameraState->cameraMotionFrame++, 60.0f, 10.0f);
}

Void Story3TransitionCameraToPlayers(Void) {
    StoryCameraState* cameraState;
    StoryCameraState cameraTarget;

    cameraState = &storyCameraState;
    cameraTarget.cameraCenter.x = -300.0f;
    cameraTarget.cameraCenter.y = 100.0f;
    cameraTarget.cameraCenter.z = 0.0f;
    cameraTarget.cameraRotation.x = -5.0f;
    cameraTarget.cameraRotation.y = 0.0f;
    cameraTarget.cameraRotation.z = 0.0f;
    cameraTarget.cameraZoom = 1000.0f;
    Story3LerpAndSmoothCamera(cameraState, &cameraTarget, cameraState->cameraMotionFrame++, 180.0f, 10.0f);
}

Void Story3TransitionCameraToBoard(Void) {
    StoryCameraState* cameraState;
    StoryCameraState cameraTarget;

    cameraState = &storyCameraState;
    cameraTarget.cameraCenter.x = 0.0f;
    cameraTarget.cameraCenter.y = 170.0f;
    cameraTarget.cameraCenter.z = 0.0f;
    cameraTarget.cameraRotation.x = -5.0f;
    cameraTarget.cameraRotation.y = 0.0f;
    cameraTarget.cameraRotation.z = 0.0f;
    cameraTarget.cameraZoom = 1700.0f;
    Story3LerpAndSmoothCamera(cameraState, &cameraTarget, cameraState->cameraMotionFrame++, 120.0f, 10.0f);
}

Void Story3StartResultCameraMotion(Void) {
    Story3ResetCameraTarget(&storyCameraState);
    Story3SetCameraCallback(Story3TransitionCameraToWinner);
    Story3SleepFrames(120);
    Story3ResetCameraTarget(&storyCameraState);
    Story3SetCameraCallback(Story3TransitionCameraToPlayers);
    Story3SleepFrames(210);
    Story3ResetCameraTarget(&storyCameraState);
    Story3SetCameraCallback(Story3TransitionCameraToBoard);
    Story3SleepFrames(120);
}

Void Story3DrawCameraDebugOverlay(Void) {
    s32 debugWindowX = 16;
    s32 debugWindowY = 40;
    s32 debugOverlayEnabled = 0;
    GXColor debugWindowColor = { 0x00, 0x00, 0x80, 0x80 };
    s32 playerIndex;

    while (TRUE) {
        Story3SleepOneFrame();
        if (HuPadBtnDown[0] & PAD_TRIGGER_R) {
            debugOverlayEnabled++;
            debugOverlayEnabled %= 2;
        }
        if (debugOverlayEnabled == 0) {
            continue;
        }
        printWin(debugWindowX, debugWindowY, 220, 200, &debugWindowColor);
        if (resultProcessCallbackState.callback == NULL) {
            print8(debugWindowX, debugWindowY, 1.0f, "PROC_MAIN    -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY, 1.0f, "PROC_MAIN    -> TRUE");
        }
        if (sequenceCallbackState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 10, 1.0f, "PROC_MAP     -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 10, 1.0f, "PROC_MAP     -> TRUE");
        }
        if (sharedSequenceProcessState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 20, 1.0f, "PROC_SUB     -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 20, 1.0f, "PROC_SUB     -> TRUE");
        }
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerSequenceProcessStates[playerIndex].callback == NULL) {
                print8(debugWindowX, debugWindowY + 30 + playerIndex * 10, 1.0f, "PROC_PLAYER%d -> FALSE", playerIndex);
            } else {
                print8(debugWindowX, debugWindowY + 30 + playerIndex * 10, 1.0f, "PROC_PLAYER%d -> TRUE", playerIndex);
            }
        }
        print8(debugWindowX, debugWindowY + 80, 1.0f, "M:%d P:%d W:%d", resultSequenceState.boardId, resultSequenceState.currentPlayerIndex, resultSequenceState.winningPlayerIndex);
        print8(debugWindowX, debugWindowY + 100, 1.0f, "NO) IDX:GRP:COM:DIF:PAD:CHR");
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            print8(debugWindowX, debugWindowY + 110 + playerIndex * 10, 1.0f, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", playerIndex,
                resultSequenceState.playerResults[playerIndex].playerStats.playerIndex, resultSequenceState.playerResults[playerIndex].playerStats.team, resultSequenceState.playerResults[playerIndex].playerStats.isComputer,
                resultSequenceState.playerResults[playerIndex].playerStats.difficulty, resultSequenceState.playerResults[playerIndex].playerStats.controllerPort, resultSequenceState.playerResults[playerIndex].playerStats.characterId);
        }
        print8(debugWindowX, debugWindowY + 150, 1.0f, "NO) RNK:STA:COI:MGS:COS:HAS");
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            print8(debugWindowX, debugWindowY + 160 + playerIndex * 10, 1.0f, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", playerIndex,
                resultSequenceState.playerResults[playerIndex].playerStats.rank, resultSequenceState.playerResults[playerIndex].playerStats.stars, resultSequenceState.playerResults[playerIndex].playerStats.coins,
                resultSequenceState.playerResults[playerIndex].playerStats.minigameCoins, resultSequenceState.playerResults[playerIndex].playerStats.maxCoins, resultSequenceState.playerResults[playerIndex].playerStats.questionSpaces);
        }
    }
}

Void Story3FadeOutResultScene(Void) {
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (TRUE) {
        if (WipeStatGet() == 0) {
            break;
        }
        Story3SleepOneFrame();
    }
    Story3KillAllWindows();
    CharModelKill(-1);
    MGSeqKillAll();
    HuAudSStreamAllStop();
    msmSeStopAll(0, 1000);
    omOvlGotoEx(DLL_mstory3dll, 1, 1, 0);
}

Void Story3StartResultAudio(Void) {
    resultAudioSequenceId = HuAudSeqPlay(0x31);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    while (TRUE) {
        if (WipeStatGet() == 0) {
            break;
        }
        Story3SleepOneFrame();
    }
}

Void Story3BeginWinnerPresentation(Void) {
    Vec cameraFocusOffset = { -320.0f, 0.0f, 910.0f };

    Story3SleepFrames(60);
    resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(1, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][0]);
    Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0, 3);
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
    Story3ResetCameraTarget(&storyCameraState);
    Story3SetCameraCallback(Story3AimResultCamera);
    Story3SetModelMotion(sequenceCallbackState.object, 2, 2, 0, 0);
    Story3SetModelMotion(sharedSequenceProcessState.object, 1, 2, 15, 1);
    Story3UpdateObjectTransform(sharedSequenceProcessState.object, 1, cameraFocusOffset, 30.0f, 60.0f, 10.0f);
    Story3SetModelMotion(sharedSequenceProcessState.object, 1, 1, 15, 1);
    Story3WaitForModelMotionEnd(sequenceCallbackState.object, 2, 2, 0);
}

Void Story3UpdateMainResultProcess(OMOBJ* object) {
    StorySequenceProcessState* sequenceProcessState;

    sequenceProcessState = object->data;
    resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(0, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][2]);
    Story3ShowResultMessageSequence(resultSequenceState.audioSequenceId, 1, -1, 3);
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
    sequenceProcessState->callback = 0;
}

Void Story3RunPlayerResultSequence(Void) {
    s32 playerIndex;

    sequenceCallbackState.callback = (Void*) Story3UpdateMainResultProcess;
    do {
        Story3SleepOneFrame();
    } while (resultSequenceState.sequenceState != 99);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSequenceProcessStates[playerIndex].callback = (Void*) Story3AnimatePlayerMedal;
        playerSequenceProcessStates[playerIndex].callbackState = playerSequenceProcessStates[playerIndex].callbackResultState = 0;
    }
    Story3FindTopMinigameScores(0);
    for (playerIndex = 0; playerIndex < sequencePlayerCountAndWinner[0]; playerIndex++) {
        if (resultSequenceState.playerResults[playerSequenceOrder[playerIndex]].playerStats.stars != 0) {
            playerSequenceProcessStates[playerSequenceOrder[playerIndex]].callbackResultState = 1;
        }
    }
    HuAudFXPlay(0xB9);
    do {
        Story3SleepOneFrame();
    } while (sequenceCallbackState.callback != NULL);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSequenceProcessStates[playerIndex].callbackState = 1;
    }
    do {
        Story3SleepOneFrame();
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerSequenceProcessStates[playerIndex].callbackState != 2) {
                break;
            }
        }
    } while (playerIndex < 4);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSequenceProcessStates[playerIndex].callbackState = 3;
    }
    do {
        Story3SleepOneFrame();
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerSequenceProcessStates[playerIndex].callback != NULL) {
                break;
            }
        }
    } while (playerIndex < 4);
}

Void Story3UpdateTeamResultProcess(OMOBJ* object) {
    StorySequenceProcessState* sequenceProcessState;

    sequenceProcessState = object->data;
    resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(0, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][2]);
    Story3ShowResultMessageSequence(resultSequenceState.audioSequenceId, 2, -1, 3);
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
    sequenceProcessState->callback = 0;
}

Void Story3RunTeamResultSequence(Void) {
    s32 playerIndex;

    sequenceCallbackState.callback = (Void*) Story3UpdateTeamResultProcess;
    do {
        Story3SleepOneFrame();
    } while (resultSequenceState.sequenceState != 99);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSequenceProcessStates[playerIndex].callback = (Void*) Story3AnimateTeamMedal;
        playerSequenceProcessStates[playerIndex].callbackState = playerSequenceProcessStates[playerIndex].callbackResultState = 0;
    }
    Story3FindTopMinigameScores(1);
    for (playerIndex = 0; playerIndex < sequencePlayerCountAndWinner[0]; playerIndex++) {
        if (resultSequenceState.playerResults[playerSequenceOrder[playerIndex]].playerStats.coins != 0) {
            playerSequenceProcessStates[playerSequenceOrder[playerIndex]].callbackResultState = 1;
        }
    }
    HuAudFXPlay(0xB8);
    do {
        Story3SleepOneFrame();
    } while (sequenceCallbackState.callback != NULL);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSequenceProcessStates[playerIndex].callbackState = 1;
    }
    do {
        Story3SleepOneFrame();
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerSequenceProcessStates[playerIndex].callbackState != 2) {
                break;
            }
        }
    } while (playerIndex < 4);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSequenceProcessStates[playerIndex].callbackState = 3;
    }
    do {
        Story3SleepOneFrame();
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerSequenceProcessStates[playerIndex].callback != NULL) {
                break;
            }
        }
    } while (playerIndex < 4);
}

Void Story3UpdatePlayerArrivalProcess(OMOBJ* object) {
    StorySequenceProcessState* sequenceProcessState;

    sequenceProcessState = object->data;
    HuAudFXPlay(0x9A);
    Story3ShowResultMessageSequence(resultSequenceState.audioSequenceId, audioSequenceMode, -1, 4);
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
    sequenceProcessState->callback = 0;
}

Void Story3UpdateResultTransitionProcess(OMOBJ* object) {
    StorySequenceProcessState* sequenceProcessState;

    sequenceProcessState = object->data;
    Story3SleepFrames(60);
    HuAudSeqPause(resultAudioSequenceId, 0, 1000);
    sequenceProcessState->callback = 0;
}

Void Story3SortPlayersByScreenPosition(s32 playerCount) {
    s32 leftmostPlayerIndex;
    s32 rightmostPlayerIndex;
    s32 rankedPlayerIndex;
    s32 playerIndex;

    leftmostPlayerIndex = 9999;
    rightmostPlayerIndex = -1;
    audioSequenceMode = playerCount;
    resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(0, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][2]);
    Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, playerCount * 3 + 3, 3);
    Story3FindTiedMinigameScores(playerCount);
    cameraFocusCenterX = 0.0f;
    switch (sequencePlayerCountAndWinner[0]) {
        case 1:
            cameraFocusCenterX = playerModelPositions[playerSequenceOrder[0]].x;
            break;
        case 2:
            cameraFocusCenterX = (playerModelPositions[playerSequenceOrder[0]].x + playerModelPositions[playerSequenceOrder[1]].x) / 2;
            Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, playerCount * 3 + 4, 3);
            break;
        case 3:
            for (playerIndex = 0; playerIndex < 3; playerIndex++) {
                if (leftmostPlayerIndex >= playerSequenceOrder[playerIndex]) {
                    leftmostPlayerIndex = playerSequenceOrder[playerIndex];
                }
                if (rightmostPlayerIndex <= playerSequenceOrder[playerIndex]) {
                    rightmostPlayerIndex = playerSequenceOrder[playerIndex];
                }
            }
            cameraFocusCenterX = (playerModelPositions[rightmostPlayerIndex].x + playerModelPositions[leftmostPlayerIndex].x) / 2;
            Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, playerCount * 3 + 5, 3);
            break;
        case 4:
            break;
    }
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
    HuAudSeqPause(resultAudioSequenceId, 1, 1000);
    resultSoundEffectIds[0] = HuAudFXPlay(0x9E);
    Story3SetPlayerResultLights(0x40, 30, 0);
    Story3SleepFrames(60);
    if (sequencePlayerCountAndWinner[0] < 4) {
        Story3ResetCameraTarget(&storyCameraState);
        Story3SetCameraCallback(Story3TransitionCameraToResult);
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            for (rankedPlayerIndex = 0; rankedPlayerIndex < 4; rankedPlayerIndex++) {
                if (playerIndex == playerSequenceOrder[rankedPlayerIndex]) {
                    playerSequenceProcessStates[playerIndex].callback = (Void*) Story3AnimateAwardedStarParticles;
                    playerSequenceProcessStates[playerIndex].callbackState = 0;
                    break;
                }
            }
            if (rankedPlayerIndex == 4) {
                playerSequenceProcessStates[playerIndex].callback = (Void*) Story3AwardPlayerStar;
                playerSequenceProcessStates[playerIndex].callbackState = 0;
            }
        }
    } else {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerSequenceProcessStates[playerIndex].callback = (Void*) Story3AwardPlayerStar;
            playerSequenceProcessStates[playerIndex].callbackState = 0;
        }
    }
    if (sequencePlayerCountAndWinner[0] != 4) {
        resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(1, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    } else {
        resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(0, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    }
    if (sequencePlayerCountAndWinner[0] <= 3) {
        HuAudFXPlay(0x9F);
    }
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][2]);
    switch (sequencePlayerCountAndWinner[0]) {
        case 1:
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, playerSequenceOrder[0], 0);
            audioSequenceMode = 12;
            sequenceCallbackState.callback = (Void*) Story3UpdatePlayerArrivalProcess;
            sharedSequenceProcessState.callback = (Void*) Story3UpdateResultTransitionProcess;
            break;
        case 2:
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, playerSequenceOrder[0], 0);
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, playerSequenceOrder[1], 1);
            audioSequenceMode = 13;
            sequenceCallbackState.callback = (Void*) Story3UpdatePlayerArrivalProcess;
            sharedSequenceProcessState.callback = (Void*) Story3UpdateResultTransitionProcess;
            break;
        case 3:
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, playerSequenceOrder[0], 0);
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, playerSequenceOrder[1], 1);
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, playerSequenceOrder[2], 2);
            audioSequenceMode = 14;
            sequenceCallbackState.callback = (Void*) Story3UpdatePlayerArrivalProcess;
            sharedSequenceProcessState.callback = (Void*) Story3UpdateResultTransitionProcess;
            break;
        case 4:
            HuAudFXFadeOut(resultSoundEffectIds[0], 500);
            sharedSequenceProcessState.callback = (Void*) Story3UpdateResultTransitionProcess;
            if (resultSequenceState.playerResults[playerSequenceOrder[0]].playerStats.questionSpaces == 0 && playerCount == 2) {
                Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 26, 3);
            } else {
                Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 15, 3);
            }
            Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
            break;
    }
    for (playerIndex = 0; playerIndex < sequencePlayerCountAndWinner[0]; playerIndex++) {
        playerSequenceProcessStates[playerSequenceOrder[playerIndex]].callbackState = 1;
    }
    do {
        Story3SleepOneFrame();
        for (playerIndex = 0; playerIndex < sequencePlayerCountAndWinner[0]; playerIndex++) {
            if (playerSequenceProcessStates[playerSequenceOrder[playerIndex]].callback != NULL) {
                break;
            }
        }
    } while (playerIndex < sequencePlayerCountAndWinner[0]);
    resultSequenceState.sequenceState = 1;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        Hu3DModelAttrReset(playerSequenceProcessStates[playerIndex].object->mdlId[1], HU3D_MOTATTR_PAUSE);
    }
    Story3SleepFrames(10);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        Story3SetModelMotion(playerSequenceProcessStates[playerIndex].object, 1, 1, 15, 1);
    }
    do {
        Story3SleepOneFrame();
    } while (sequenceCallbackState.callback != NULL);
    Story3SortPlayersByMinigameScore();
    HuAudSeqPause(resultAudioSequenceId, 0, 1000);
    Story3ResetCameraTarget(&storyCameraState);
    Story3SetCameraCallback(Story3TransitionCameraToTeam);
    Story3SetPlayerResultLights(0xFF, 30, 1);
    Story3SleepFrames(60);
}

s32 Story3PresentIndividualWinner(Void) {
    s32 winningTeam = 0;

    resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(0, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][2]);
    Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0x10, 3);
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
    Story3SortPlayersByMinigameScore();
    Story3CountFirstPlacePlayers();
    if (resultSequenceState.teamMode == 1) {
        winningTeam = Story3DetermineWinningTeam();
    }
    if (winningTeam != -1 && sequencePlayerCountAndWinner[0] <= 1) {
        HuAudSeqFadeOut(resultAudioSequenceId, 1000);
    } else {
        HuAudSeqPause(resultAudioSequenceId, 1, 1000);
    }
    resultSoundEffectIds[0] = HuAudFXPlay(0x9E);
    resultAnimationActive = 1.0f;
    sequenceCallbackState.callback = (Void*) Story3AnimateResultLogos;
    Story3SetPlayerResultLights(0x40, 30, 0);
    Story3StartResultCameraMotion();
    playerModelVerticalOffset = 125.0f;
    return winningTeam;
}

Void Story3FinishIndividualWinnerPresentation(Void) {
    s32 playerIndex;

    HuAudSeqPause(resultAudioSequenceId, 0, 1000);
    HuAudFXFadeOut(resultSoundEffectIds[0], 1000);
    resultAnimationActive = 0.0f;
    Story3SetPlayerResultLights(0xFF, 30, 1);
    Story3SleepFrames(60);
    resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(0, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][1]);
    Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0x11, 3);
    switch (sequencePlayerCountAndWinner[0]) {
        case 1:
            break;
        case 2:
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, playerSequenceOrder[0], 0);
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, playerSequenceOrder[1], 1);
            Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 13, 3);
            break;
        case 3:
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, playerSequenceOrder[0], 0);
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, playerSequenceOrder[1], 1);
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, playerSequenceOrder[2], 2);
            Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 14, 3);
            break;
        case 4:
            Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0x12, 3);
            break;
    }
    Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0x13, 3);
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSequenceResultIndices[playerIndex] = playerSequenceScoreOrder[playerIndex] = -1;
    }
    for (playerIndex = 0; playerIndex < sequencePlayerCountAndWinner[0]; playerIndex++) {
        playerSequenceProcessStates[playerSequenceOrder[playerIndex]].callback = (Void*) Story3FinishPlayerResultProcess;
        playerSequenceProcessStates[playerSequenceOrder[playerIndex]].callbackState = 0;
    }
    do {
        Story3SleepOneFrame();
        for (playerIndex = 0; playerIndex < sequencePlayerCountAndWinner[0]; playerIndex++) {
            if (playerSequenceProcessStates[playerSequenceOrder[playerIndex]].callbackState != 1) {
                break;
            }
        }
    } while (playerIndex < sequencePlayerCountAndWinner[0]);
    for (playerIndex = 0; playerIndex < sequencePlayerCountAndWinner[0]; playerIndex++) {
        playerSequenceProcessStates[playerSequenceOrder[playerIndex]].callbackState = 2;
    }
    Story3SleepFrames(0x96);
    Story3CalculatePlayerMinigameScores();
    resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(0, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][2]);
    Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, sequencePlayerCountAndWinner[1], 0);
    Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0x14, 3);
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
    for (playerIndex = 0; playerIndex < sequencePlayerCountAndWinner[0]; playerIndex++) {
        if (sequencePlayerCountAndWinner[1] == playerSequenceOrder[playerIndex]) {
            Story3SetModelMotion(playerSequenceProcessStates[playerSequenceOrder[playerIndex]].object, 1, 14, 15, 0);
        } else {
            Story3SetModelMotion(playerSequenceProcessStates[playerSequenceOrder[playerIndex]].object, 1, 15, 15, 0);
        }
    }
    Story3SleepFrames(0x23);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        Story3SetModelMotion(playerSequenceProcessStates[playerIndex].object, 1, 1, 15, 1);
    }
    Story3SleepFrames(0x55);
    resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(0, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][3]);
    Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0x15, 3);
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        Story3SetModelMotion(playerSequenceProcessStates[playerIndex].object, 1, 1, 15, 1);
    }
    HuAudSeqFadeOut(resultAudioSequenceId, 1000);
    resultSoundEffectIds[0] = HuAudFXPlay(0x9E);
    resultAnimationActive = 1.0f;
    Story3SetPlayerResultLights(0x40, 30, 0);
    Story3StartResultCameraMotion();
}

Void Story3PresentTeamWinners(Void) {
    s32 playerIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSequenceProcessStates[playerIndex].callback = (Void*) Story3UpdatePlayerRankMarker;
    }
    do {
        Story3SleepOneFrame();
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerSequenceProcessStates[playerIndex].callback != NULL) {
                break;
            }
        }
    } while (playerIndex < 4);
    resultAnimationActive = 0.0f;
    Story3SleepFrames(15);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (resultSequenceState.playerResults[playerIndex].playerStats.rank == 1) {
            break;
        }
    }
    if (resultSequenceState.teamMode == 1) {
        if (resultSequenceState.winningTeam == 0) {
            Story3AnimatePairGlow(0xFF, 5, 0, 1);
        } else {
            Story3AnimatePairGlow(0xFF, 5, 2, 3);
        }
    } else {
        Story3AnimatePlayerGlow(0xFF, 5.0f, playerIndex);
    }
    HuAudFXPlay(0x9F);
    HuAudSeqPlay(0x32);
    Story3SleepFrames(30);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (resultSequenceState.teamMode == 1) {
            if (resultSequenceState.winningTeam == 0) {
                CharMotionVoiceOnSet(resultSequenceState.playerResults[0].playerStats.characterId, playerSequenceProcessStates[0].object->mtnId[3], 0);
                CharMotionVoiceOnSet(resultSequenceState.playerResults[1].playerStats.characterId, playerSequenceProcessStates[1].object->mtnId[3], 0);
                CharMotionVoiceOnSet(resultSequenceState.playerResults[2].playerStats.characterId, playerSequenceProcessStates[2].object->mtnId[4], 0);
                CharMotionVoiceOnSet(resultSequenceState.playerResults[3].playerStats.characterId, playerSequenceProcessStates[3].object->mtnId[4], 0);
                Story3SetModelMotion(playerSequenceProcessStates[0].object, 1, 3, 15, 0);
                Story3SetModelMotion(playerSequenceProcessStates[1].object, 1, 3, 15, 0);
                Story3SetModelMotion(playerSequenceProcessStates[2].object, 1, 4, 15, 0);
                Story3SetModelMotion(playerSequenceProcessStates[3].object, 1, 4, 15, 0);
                resultSequenceState.currentPlayerIndex = 0;
            } else {
                CharMotionVoiceOnSet(resultSequenceState.playerResults[0].playerStats.characterId, playerSequenceProcessStates[0].object->mtnId[4], 0);
                CharMotionVoiceOnSet(resultSequenceState.playerResults[1].playerStats.characterId, playerSequenceProcessStates[1].object->mtnId[4], 0);
                CharMotionVoiceOnSet(resultSequenceState.playerResults[2].playerStats.characterId, playerSequenceProcessStates[2].object->mtnId[3], 0);
                CharMotionVoiceOnSet(resultSequenceState.playerResults[3].playerStats.characterId, playerSequenceProcessStates[3].object->mtnId[3], 0);
                Story3SetModelMotion(playerSequenceProcessStates[0].object, 1, 4, 15, 0);
                Story3SetModelMotion(playerSequenceProcessStates[1].object, 1, 4, 15, 0);
                Story3SetModelMotion(playerSequenceProcessStates[2].object, 1, 3, 15, 0);
                Story3SetModelMotion(playerSequenceProcessStates[3].object, 1, 3, 15, 0);
                resultSequenceState.currentPlayerIndex = 2;
            }
        } else if (resultSequenceState.playerResults[playerIndex].playerStats.rank == 1) {
            CharMotionVoiceOnSet(resultSequenceState.playerResults[playerIndex].playerStats.characterId, playerSequenceProcessStates[playerIndex].object->mtnId[3], 0);
            Story3SetModelMotion(playerSequenceProcessStates[playerIndex].object, 1, 3, 15, 0);
            resultSequenceState.currentPlayerIndex = playerIndex;
        } else {
            CharMotionVoiceOnSet(resultSequenceState.playerResults[playerIndex].playerStats.characterId, playerSequenceProcessStates[playerIndex].object->mtnId[4], 0);
            Story3SetModelMotion(playerSequenceProcessStates[playerIndex].object, 1, 4, 15, 0);
        }
        GWPlayer[resultSequenceState.playerResults[playerIndex].playerStats.playerIndex].rank = resultSequenceState.playerResults[playerIndex].playerStats.rank - 1;
    }
    resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(0, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][2]);
    if (resultSequenceState.teamMode == 1) {
        if (resultSequenceState.winningTeam == 0) {
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, 0, 0);
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, 1, 1);
            _SetFlag(FLAG_ID_MAKE(1, 29));
        } else {
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, 2, 0);
            Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, 3, 1);
        }
        Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0x19, 5);
    } else {
        Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, sequencePlayerCountAndWinner[1], 0);
        Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0x16, 5);
    }
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
}

s32 Story3DetermineWinningTeam(Void) {
    s32 winningTeam;
    s32 teamZeroStars;
    s32 teamOneStars;
    s32 teamZeroCoins;
    s32 teamOneCoins;

    winningTeam = 1;
    teamZeroStars = resultSequenceState.playerResults[0].playerStats.stars + resultSequenceState.playerResults[1].playerStats.stars;
    teamOneStars = resultSequenceState.playerResults[2].playerStats.stars + resultSequenceState.playerResults[3].playerStats.stars;
    teamZeroCoins = resultSequenceState.playerResults[0].playerStats.coins + resultSequenceState.playerResults[1].playerStats.coins;
    teamOneCoins = resultSequenceState.playerResults[2].playerStats.coins + resultSequenceState.playerResults[3].playerStats.coins;
    if (teamZeroStars > teamOneStars) {
        resultSequenceState.winningTeam = 0;
    } else if (teamZeroStars < teamOneStars) {
        resultSequenceState.winningTeam = 1;
    } else if (teamZeroCoins > teamOneCoins) {
        resultSequenceState.winningTeam = 0;
    } else if (teamZeroCoins < teamOneCoins) {
        resultSequenceState.winningTeam = 1;
    } else {
        winningTeam = -1;
    }
    return winningTeam;
}

Void Story3FinishTeamWinnerPresentation(Void) {
    s32 playerIndex;

    HuAudSeqPause(resultAudioSequenceId, 0, 1000);
    HuAudFXFadeOut(resultSoundEffectIds[0], 1000);
    resultAnimationActive = 0.0f;
    Story3SetPlayerResultLights(0xFF, 30, 1);
    Story3SleepFrames(60);
    resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(0, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][1]);
    Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0x17, 3);
    Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0x13, 3);
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSequenceResultIndices[playerIndex] = playerSequenceScoreOrder[playerIndex] = -1;
    }
    sequencePlayerCountAndWinner[0] = 2;
    playerSequenceOrder[0] = 0;
    playerSequenceOrder[1] = 2;
    for (playerIndex = 0; playerIndex < sequencePlayerCountAndWinner[0]; playerIndex++) {
        playerSequenceProcessStates[playerSequenceOrder[playerIndex]].callback = (Void*) Story3FinishPlayerResultProcess;
        playerSequenceProcessStates[playerSequenceOrder[playerIndex]].callbackState = 0;
    }
    do {
        Story3SleepOneFrame();
        for (playerIndex = 0; playerIndex < sequencePlayerCountAndWinner[0]; playerIndex++) {
            if (playerSequenceProcessStates[playerSequenceOrder[playerIndex]].callbackState != 1) {
                break;
            }
        }
    } while (playerIndex < sequencePlayerCountAndWinner[0]);
    for (playerIndex = 0; playerIndex < sequencePlayerCountAndWinner[0]; playerIndex++) {
        playerSequenceProcessStates[playerSequenceOrder[playerIndex]].callbackState = 2;
    }
    Story3SleepFrames(150);
    if (playerSequenceScoreOrder[0] > playerSequenceScoreOrder[2]) {
        resultSequenceState.winningTeam = 0;
    } else {
        resultSequenceState.winningTeam = 1;
    }
    resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(0, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][2]);
    if (resultSequenceState.winningTeam == 0) {
        Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, 0, 0);
        Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, 1, 1);
    } else {
        Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, 2, 0);
        Story3InsertCharacterNameMessage(resultSequenceState.audioSequenceId, 3, 1);
    }
    Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0x18, 3);
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
    for (playerIndex = 0; playerIndex < sequencePlayerCountAndWinner[0]; playerIndex++) {
        if (resultSequenceState.winningTeam == 0) {
            Story3SetModelMotion(playerSequenceProcessStates[0].object, 1, 14, 15, 0);
            Story3SetModelMotion(playerSequenceProcessStates[1].object, 1, 14, 15, 0);
            Story3SetModelMotion(playerSequenceProcessStates[2].object, 1, 15, 15, 0);
            Story3SetModelMotion(playerSequenceProcessStates[3].object, 1, 15, 15, 0);
        } else {
            Story3SetModelMotion(playerSequenceProcessStates[0].object, 1, 15, 15, 0);
            Story3SetModelMotion(playerSequenceProcessStates[1].object, 1, 15, 15, 0);
            Story3SetModelMotion(playerSequenceProcessStates[2].object, 1, 14, 15, 0);
            Story3SetModelMotion(playerSequenceProcessStates[3].object, 1, 14, 15, 0);
        }
    }
    Story3SleepFrames(35);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        Story3SetModelMotion(playerSequenceProcessStates[playerIndex].object, 1, 1, 15, 1);
    }
    Story3SleepFrames(85);
    resultSequenceState.audioSequenceId = story3CreateStandardMessageWindow(0, 0, 21, 2, resultSequenceState.winningPlayerIndex);
    HuAudFXPlay(recordMessageIdTable[resultSequenceState.boardId][3]);
    Story3ShowBoardResultMessage(resultSequenceState.audioSequenceId, 0x15, 3);
    Story3ShowCenteredMessage(resultSequenceState.audioSequenceId);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        Story3SetModelMotion(playerSequenceProcessStates[playerIndex].object, 1, 1, 15, 1);
    }
    HuAudSeqFadeOut(resultAudioSequenceId, 1000);
    resultSoundEffectIds[0] = HuAudFXPlay(0x9E);
    resultAnimationActive = 1.0f;
    Story3SetPlayerResultLights(0x40, 30, 0);
    Story3StartResultCameraMotion();
}

Void Story3BobWinnerModel(Void) {
    float modelBobOffset;

    modelBobOffset = interpolateSinePulse(0.0f, 0.1f, resultAnimationFrame, 180.0f);
    Hu3DData[sequenceCallbackState.object->mdlId[20]].pos.y += modelBobOffset;
    resultAnimationFrame++;
    if (resultAnimationFrame >= 180) {
        resultAnimationFrame -= 180;
    }
}

Void Story3PulseWinnerModelScale(Void) {
    float winnerModelScale;

    winnerModelScale = interpolateSinePulse(1.0f, 1.1f, resultAnimationFrame, 60.0f);
    Hu3DModelScaleSet(sequenceCallbackState.object->mdlId[23], winnerModelScale, winnerModelScale, 1.0f);
    resultAnimationFrame++;
    if (resultAnimationFrame >= 60) {
        resultAnimationFrame -= 60;
    }
}

s32 winnerParticleManagerId = -1;
s32 unreferencedLogoData = -1; // unused

Void Story3MoveCameraToWinner(float startValue) {
    float playerCameraHeights[] = {
        50.0f, 50.0f, 60.0f,  50.0f,
        50.0f, 80.0f, 60.0f, 120.0f
    };
    Vec winnerStartPosition;
    Vec winnerTargetPosition;
    float winnerScreenX;
    float winnerHeight;
    float winnerRotation;
    float winnerAlpha;
    float winnerScale;
    float winnerDepth;
    s32 animationFrame;

    particleEffectCenterX = startValue;
    Story3SleepOneFrame();
    winnerScreenX = startValue;
    winnerHeight = 0.0f;
    winnerRotation = 3600.0f;
    Hu3DModelPosSet(sequenceCallbackState.object->mdlId[20], winnerScreenX, 1000.0f, 460.0f);
    Hu3DModelScaleSet(sequenceCallbackState.object->mdlId[20], 0.5f, 0.5f, 0.5f);
    Hu3DModelAttrReset(sequenceCallbackState.object->mdlId[20], HU3D_ATTR_DISPOFF);
    Story3SetModelMotion(sequenceCallbackState.object, 20, 20, 0, 2);
    Hu3DModelPosSet(sequenceCallbackState.object->mdlId[21], winnerScreenX, 1000.0f, 460.0f);
    Hu3DModelScaleSet(sequenceCallbackState.object->mdlId[21], 0.5f, 0.5f, 0.5f);
    Story3SetModelMotion(sequenceCallbackState.object, 21, 21, 0, 2);
    Hu3DModelPosSet(sequenceCallbackState.object->mdlId[22], winnerScreenX, 1000.0f, 460.0f);
    Hu3DModelScaleSet(sequenceCallbackState.object->mdlId[22], 0.5f, 0.5f, 0.5f);
    Hu3DMotionShapeSpeedSet(sequenceCallbackState.object->mdlId[22], 2.0f);
    Hu3DMotionSpeedSet(sequenceCallbackState.object->mdlId[22], 2.0f);
    Hu3DMotionShiftSpeedSet(sequenceCallbackState.object->mdlId[22], 2.0f);
    Story3SetModelMotion(sequenceCallbackState.object, 22, 22, 0, 2);
    Hu3DModelScaleSet(sequenceCallbackState.object->mdlId[23], 0.0f, 0.0f, 1.0f);
    Hu3DModelAttrSet(sequenceCallbackState.object->mdlId[23], HU3D_ATTR_DISPOFF);
    Story3SleepFrames(60);
    Story3ResetCameraTarget(&storyCameraState);
    Story3SetCameraCallback(Story3FocusCameraOnPlayer);
    for (animationFrame = 0; animationFrame <= 120; animationFrame++) {
        Story3SleepOneFrame();
        if (resultSequenceState.teamMode == 0) {
            winnerHeight = interpolateSineEaseIn(1000.0f, 250.0f + playerCameraHeights[resultSequenceState.playerResults[sequencePlayerCountAndWinner[1]].playerStats.characterId], animationFrame, 120.0f);
        } else {
            winnerHeight = interpolateSineEaseIn(1000.0f, 350.0f, animationFrame, 120.0f);
        }
        winnerRotation = interpolateSineEaseIn(1800.0f, 0.0f, animationFrame, 120.0f);
        winnerAlpha = interpolateSineEaseIn(1.0f, 0.0f, animationFrame, 120.0f);
        Hu3DModelPosSet(sequenceCallbackState.object->mdlId[20], winnerScreenX, winnerHeight, 460.0f);
        Hu3DModelRotSet(sequenceCallbackState.object->mdlId[20], 0.0f, winnerRotation, 0.0f);
    }
    sequenceCallbackState.callback = (Void*) Story3BobWinnerModel;
    Story3SleepFrames(30);
    Story3SetModelMotion(sequenceCallbackState.object, 20, 20, 0, 0);
    Story3SleepFrames(30);
    HuAudFXPlay(0x33B);
    Story3SleepFrames(30);
    Hu3DModelPosSet(sequenceCallbackState.object->mdlId[22], winnerScreenX, Hu3DData[sequenceCallbackState.object->mdlId[20]].pos.y - 10.0f, 460.0f);
    Hu3DModelAttrReset(sequenceCallbackState.object->mdlId[22], HU3D_ATTR_DISPOFF);
    Story3SetModelMotion(sequenceCallbackState.object, 22, 22, 0, 0);
    winnerStartPosition.x = winnerScreenX;
    winnerStartPosition.y = Hu3DData[sequenceCallbackState.object->mdlId[20]].pos.y;
    winnerStartPosition.z = 460.0f;
    winnerTargetPosition.x = 288.0f;
    winnerTargetPosition.y = 100.0f;
    winnerTargetPosition.z = 500.0f;
    Story3CreateParticleTextEffect(DATA_MAKE_NUM(DATADIR_MSTORY3, 0), &winnerStartPosition, &winnerTargetPosition, 100);
    Story3MarkParticleTextEffectReady();
    Story3SleepFrames(15);
    if (resultSequenceState.teamMode == 1) {
        if (resultSequenceState.winningTeam == 0) {
            CharFXPlay(resultSequenceState.playerResults[0].playerStats.characterId, 0x12B);
            CharFXPlay(resultSequenceState.playerResults[1].playerStats.characterId, 0x12B);
        } else {
            CharFXPlay(resultSequenceState.playerResults[2].playerStats.characterId, 0x12B);
            CharFXPlay(resultSequenceState.playerResults[3].playerStats.characterId, 0x12B);
        }
    } else {
        CharFXPlay(resultSequenceState.playerResults[sequencePlayerCountAndWinner[1]].playerStats.characterId, 0x12B);
    }
    HuAudFXPlay(0xBA);
    HuAudFXPlay(0x9A);
    Hu3DModelPosSet(sequenceCallbackState.object->mdlId[21], winnerScreenX, Hu3DData[sequenceCallbackState.object->mdlId[20]].pos.y, 510.0f);
    Hu3DModelAttrReset(sequenceCallbackState.object->mdlId[21], HU3D_ATTR_DISPOFF);
    Story3SetModelMotion(sequenceCallbackState.object, 21, 21, 0, 0);
    Story3SleepFrames(5);
    winnerParticleManagerId = Story3CreateParticleManager(1);
    Hu3DParManPosSet(winnerParticleManagerId, winnerScreenX, Hu3DData[sequenceCallbackState.object->mdlId[21]].pos.y, 510.0f);
    Hu3DModelAttrSet(sequenceCallbackState.object->mdlId[20], HU3D_ATTR_DISPOFF);
    particleEffectEnabled = 1.0f;
    for (animationFrame = 0; animationFrame <= 30; animationFrame++) {
        Story3SleepOneFrame();
        winnerScale = interpolateLinearOverTime(0.0f, 1.0f, animationFrame, 30.0f);
        winnerHeight = interpolateLinearOverTime(0.0f, 75.0f, animationFrame, 30.0f);
        winnerDepth = interpolateLinearOverTime(550.0f, 640.0f, animationFrame, 30.0f);
        Hu3DModelPosSet(sequenceCallbackState.object->mdlId[23], winnerScreenX, Hu3DData[sequenceCallbackState.object->mdlId[20]].pos.y - winnerHeight, winnerDepth);
        Hu3DModelScaleSet(sequenceCallbackState.object->mdlId[23], winnerScale, winnerScale, 1.0f);
        winnerAlpha = interpolateLinearOverTime(1.0f, 0.0f, animationFrame, 30.0f);
        Hu3DModelTPLvlSet(sequenceCallbackState.object->mdlId[22], winnerAlpha);
    }
    resultAnimationFrame = 0;
    sequenceCallbackState.callback = (Void*) Story3PulseWinnerModelScale;
    Story3SleepFrames(30);
    Hu3DParManKill(winnerParticleManagerId);
    winnerParticleManagerId = -1;
    Story3SleepFrames(240);
    Story3CreateDataWindow(MAKE_MESSID(36, 7), 0);
    if (resultSequenceState.teamMode == 1) {
        if (resultSequenceState.winningTeam == 0) {
            if (resultSequenceState.playerResults[0].playerStats.isComputer == 1 && resultSequenceState.playerResults[1].playerStats.isComputer == 1) {
                Story3SleepFrames(30);
            } else {
                while (TRUE) {
                    Story3SleepOneFrame();
                    if (resultSequenceState.playerResults[0].playerStats.isComputer == 0 && (HuPadBtnDown[resultSequenceState.playerResults[0].playerStats.controllerPort] & PAD_BUTTON_A)) {
                        break;
                    }
                    if (resultSequenceState.playerResults[1].playerStats.isComputer == 0 && (HuPadBtnDown[resultSequenceState.playerResults[1].playerStats.controllerPort] & PAD_BUTTON_A)) {
                        break;
                    }
                }
            }
        } else {
            if (resultSequenceState.playerResults[2].playerStats.isComputer == 1 && resultSequenceState.playerResults[3].playerStats.isComputer == 1) {
                Story3SleepFrames(30);
            } else {
                while (TRUE) {
                    Story3SleepOneFrame();
                    if (resultSequenceState.playerResults[0].playerStats.isComputer == 0 && (HuPadBtnDown[resultSequenceState.playerResults[0].playerStats.controllerPort] & PAD_BUTTON_A)) {
                        break;
                    }
                    if (resultSequenceState.playerResults[1].playerStats.isComputer == 0 && (HuPadBtnDown[resultSequenceState.playerResults[1].playerStats.controllerPort] & PAD_BUTTON_A)) {
                        break;
                    }
                }
            }
        }
    } else if (resultSequenceState.sequenceVariant == 0) {
        if (resultSequenceState.playerResults[resultSequenceState.currentPlayerIndex].playerStats.isComputer != 1) {
            Story3WaitForControllerButton(resultSequenceState.playerResults[resultSequenceState.currentPlayerIndex].playerStats.controllerPort, PAD_BUTTON_A);
        } else {
            Story3SleepFrames(30);
        }
    } else {
        Story3WaitForControllerButton(0, PAD_BUTTON_A);
    }
    HuAudFXPlay(0x1C);
}

Void Story3RunWinnerPresentation(OMOBJ* object) {
    StorySequenceProcessState* resultProcessState = object->data;
    s32 winningTeam;

    Story3StartResultAudio();
    Story3BeginWinnerPresentation();
    Story3RunPlayerResultSequence();
    Story3RunTeamResultSequence();
    if (resultSequenceState.bonusStarMode != 0) {
        Story3SortPlayersByScreenPosition(0);
        Story3SortPlayersByScreenPosition(1);
        Story3SortPlayersByScreenPosition(2);
    }
    winningTeam = Story3PresentIndividualWinner();
    if (resultSequenceState.teamMode == 1) {
        if (winningTeam == -1) {
            Story3FinishTeamWinnerPresentation();
        }
    } else if (sequencePlayerCountAndWinner[0] > 1) {
        Story3FinishIndividualWinnerPresentation();
    }
    Story3PresentTeamWinners();
    if (resultSequenceState.teamMode == 1) {
        if (resultSequenceState.winningTeam == 0) {
            Story3MoveCameraToWinner(playerModelPositions[0].x + 100.0f);
        } else {
            Story3MoveCameraToWinner(playerModelPositions[2].x + 100.0f);
        }
    } else {
        Story3MoveCameraToWinner(playerModelPositions[sequencePlayerCountAndWinner[1]].x);
    }
    Story3SleepFrames(60);
    Story3FadeOutResultScene();
}

Void Story3CreateResultLogoObject(Void) {
    OMOBJ* createdResultObject;

    createdResultObject = omAddObjEx(resultObjectManager, 0x100, 16, 16, -1, NULL);
    resultProcessCallbackState.object = createdResultObject;
    resultProcessCallbackState.callback = (Void*) Story3RunWinnerPresentation;
    resultProcessCallbackState.object->data = &resultProcessCallbackState;
    while (TRUE) {
        Story3SleepOneFrame();
        if (resultProcessCallbackState.callback != NULL) {
            resultProcessCallbackState.callback(createdResultObject);
        }
    }
}

Void Story3AnimateResultLogoScale(OMOBJ* object) {
    float minimumLogoScale;
    float logoScale;
    float maximumLogoScale;
    StorySequenceProcessState* processState;
    s32 animationFrame;
    s32 pulseFrame;

    processState = object->data;
    maximumLogoScale = 1.75f;
    minimumLogoScale = 0.5f;
    pulseFrame = 0;
    HuAudFXPlay(0xBA);
    HuAudFXPlay(0x9A);
    resultSequenceState.sequenceProgress = 1;
    espDispOn(processState->spriteId);
    espScaleSet(processState->spriteId, 0.0f, 0.0f);
    for (animationFrame = 0; animationFrame < 120; animationFrame++) {
        Story3SleepOneFrame();
        if (animationFrame < 60) {
            logoScale = interpolateSineEaseIn(0.0f, 1.75f, animationFrame, 60.0f);
        } else {
            logoScale = interpolateSineEaseOut(maximumLogoScale, 1.0f, pulseFrame, 15.0f);
            pulseFrame++;
            if (pulseFrame == 15) {
                if (maximumLogoScale > 1.25f) {
                    maximumLogoScale -= 0.1f;
                }
                pulseFrame = 0;
            }
        }
        espScaleSet(processState->spriteId, logoScale, logoScale);
    }
    processState->callback = 0;
}

Void Story3UpdateResultAmbientLight(OMOBJ* object) {
    float ambientLightIntensity;

    ambientLightIntensity = resultSequenceState.globalLightBrightness / 255.0f;
    Hu3DModelAmbSet(object->mdlId[1], ambientLightIntensity, ambientLightIntensity, ambientLightIntensity);
    Hu3DModelAmbSet(object->mdlId[2], ambientLightIntensity, ambientLightIntensity, ambientLightIntensity);
    Hu3DModelAmbSet(object->mdlId[3], ambientLightIntensity, ambientLightIntensity, ambientLightIntensity);
}

float logoRotationSpeeds[] = { 0.0f, 120.0f, 240.0f };

Void Story3AnimateResultLogos(OMOBJ* object) {
    s32 logoModelIndex;

    if (resultAnimationActive == 1.0f) {
        Hu3DModelAttrReset(object->mdlId[4], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(object->mdlId[5], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(object->mdlId[6], HU3D_ATTR_DISPOFF);
        logoFadeAlpha += 0.033333335f;
        if (logoFadeAlpha >= 0.5f) {
            logoFadeAlpha = 0.5f;
        }
    } else {
        if (resultAnimationActive == 0.0f) {
            logoFadeAlpha -= 0.033333335f;
            if (logoFadeAlpha <= 0.0f) {
                logoFadeAlpha = 0.0f;
                Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrSet(object->mdlId[6], HU3D_ATTR_DISPOFF);
                return;
            }
        } else {
            Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(object->mdlId[6], HU3D_ATTR_DISPOFF);
            return;
        }
    }
    for (logoModelIndex = 0; logoModelIndex < 3; logoModelIndex++) {
        Hu3DModelTPLvlSet(object->mdlId[logoModelIndex + 4], logoFadeAlpha);
        Hu3DModelRotSet(object->mdlId[logoModelIndex + 4], 0.0f, 0.0f, logoRotationState[logoModelIndex][1]);
        logoRotationState[logoModelIndex][0] = 10.0 * cosd(11.0f * logoRotationSpeeds[logoModelIndex]);
        logoRotationState[logoModelIndex][1] = 30.0 * sind(8.0f * logoRotationSpeeds[logoModelIndex]);
        if ((logoRotationSpeeds[logoModelIndex] += 0.4f) >= 360.0f) {
            logoRotationSpeeds[logoModelIndex] -= 360.0f;
        }
    }
}

Void Story3LoadStoryResultModels(OMOBJ* object, s32 count, s32 mode) {
    s32 storyModelDataIds[] = { DATA_MAKE_NUM(DATADIR_MSTORY3, 6), DATA_MAKE_NUM(DATADIR_MSTORY3, 5) };
    s32 modelIndex;

    if (mode < 5) {
        object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 4));
        object->mtnId[0] = Hu3DMotionIDGet(object->mdlId[1]);
        Story3SetModelMotion(object, 1, 0, 0, 2);
        object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 8));
        object->mdlId[3] = Hu3DModelCreateFile(storyModelDataIds[count]);
        object->mtnId[3] = Hu3DMotionIDGet(object->mdlId[3]);
        Story3SetModelMotion(object, 3, 3, 0, 1);
        Hu3DModelShadowMapObjSet(object->mdlId[1], "base_fix9-base");
    } else {
        object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 7));
        object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 8));
        object->mdlId[3] = Hu3DModelCreateFile(storyModelDataIds[count]);
        Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
        object->mtnId[3] = Hu3DMotionIDGet(object->mdlId[1]);
        Story3SetModelMotion(object, 1, 3, 0, 1);
        Hu3DModelShadowMapObjSet(object->mdlId[1], "base_story-base");
    }
    for (modelIndex = 4; modelIndex < 7; modelIndex++) {
        object->mdlId[modelIndex] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 11));
        Hu3DModelTPLvlSet(object->mdlId[modelIndex], 0.0f);
        Hu3DModelPosSet(object->mdlId[modelIndex], -200.0f + 200.0f * (modelIndex - 4), 750.0f, 500.0f);
        Hu3DModelScaleSet(object->mdlId[modelIndex], 1.25f, 2.0f, 1.25f);
        Hu3DModelAttrSet(object->mdlId[modelIndex], HU3D_ATTR_DISPOFF);
    }
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY3, 9));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MSTORY3, 10));
    sequenceCallbackState.spriteId = espEntry(DATA_MAKE_NUM(DATADIR_MSTORY3, 30), 0, 0);
    espPosSet(sequenceCallbackState.spriteId, 288.0f, 110.0f);
    espDispOff(sequenceCallbackState.spriteId);
    Story3SetModelMotion(object, 2, 2, 0, 2);
    Story3ConfigureModelMaterials(object->mdlId[1]);
    Story3ConfigureModelMaterials(object->mdlId[2]);
    Story3ConfigureModelMaterials(object->mdlId[3]);
    object->mdlId[20] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 31) + mode);
    object->mtnId[20] = Hu3DMotionIDGet(object->mdlId[20]);
    Hu3DModelPosSet(object->mdlId[20], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(object->mdlId[20], HU3D_ATTR_DISPOFF);
    Story3SetModelMotion(object, 20, 20, 0, 2);
    object->mdlId[21] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 37));
    object->mtnId[21] = Hu3DMotionIDGet(object->mdlId[21]);
    Hu3DModelPosSet(object->mdlId[21], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(object->mdlId[21], HU3D_ATTR_DISPOFF);
    Story3SetModelMotion(object, 21, 21, 0, 2);
    object->mdlId[22] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 38));
    object->mtnId[22] = Hu3DMotionIDGet(object->mdlId[22]);
    Hu3DModelPosSet(object->mdlId[22], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(object->mdlId[22], HU3D_ATTR_DISPOFF);
    Story3SetModelMotion(object, 22, 22, 0, 2);
    object->mdlId[23] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 39));
    Hu3DModelPosSet(object->mdlId[23], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrSet(object->mdlId[23], HU3D_ATTR_DISPOFF);
}

Void Story3ConfigureResultParticles(OMOBJ* object) {
    s32 ambientColorChoice;
    s32 randomPositionOffset;
    s32 particleModelIndex;

    if (particleEffectEnabled == 0.0f) {
        return;
    }
    Hu3DGLightColorSet(resultSequenceState.globalLightId, resultSequenceState.globalLightBrightness, resultSequenceState.globalLightBrightness, resultSequenceState.globalLightBrightness, 0xFF);
    resultSequenceState.globalLightBrightness++;
    if (resultSequenceState.globalLightBrightness >= 128) {
        resultSequenceState.globalLightBrightness = 128;
    }
    for (particleModelIndex = 1; particleModelIndex < 80; particleModelIndex++) {
        switch (resultParticleMotionStates[particleModelIndex].animationState) {
            case 0:
                resultParticleMotionStates[particleModelIndex].targetY = Hu3DData[object->mdlId[particleModelIndex]].pos.y = 450.0f + rand8() % 150;
                resultParticleMotionStates[particleModelIndex].animationState = 1;
                break;
            case 3:
                resultParticleMotionStates[particleModelIndex].targetY = Hu3DData[object->mdlId[particleModelIndex]].pos.y = 500.0f;
                /* fallthrough */
            case 1:
                if (particleModelIndex % 3 != 0) {
                    ambientColorChoice = rand8() % 4;
                } else {
                    ambientColorChoice = 5;
                }
                switch (ambientColorChoice) {
                    case 0:
                        Hu3DModelAmbSet(object->mdlId[particleModelIndex], 1.0f, 1.0f, 1.0f);
                        break;
                    case 1:
                        Hu3DModelAmbSet(object->mdlId[particleModelIndex], 1.0f, 0.0f, 0.0f);
                        break;
                    case 2:
                        Hu3DModelAmbSet(object->mdlId[particleModelIndex], 0.0f, 1.0f, 0.0f);
                        break;
                    case 3:
                        Hu3DModelAmbSet(object->mdlId[particleModelIndex], 0.0f, 0.0f, 1.0f);
                        break;
                    case 5:
                        break;
                }
                Hu3DModelAttrReset(object->mdlId[particleModelIndex], HU3D_ATTR_DISPOFF);
                if (resultSequenceState.teamMode == 0) {
                    resultParticleMotionStates[particleModelIndex].targetX = Hu3DData[object->mdlId[particleModelIndex]].pos.x = -50.0f + particleEffectCenterX + rand8() % 100;
                } else {
                    resultParticleMotionStates[particleModelIndex].targetX = Hu3DData[object->mdlId[particleModelIndex]].pos.x = -100.0f + particleEffectCenterX + rand8() % 200;
                }
                resultParticleMotionStates[particleModelIndex].targetZ = Hu3DData[object->mdlId[particleModelIndex]].pos.z = 460.0f;
                Hu3DData[object->mdlId[particleModelIndex]].rot.x = rand8();
                Hu3DData[object->mdlId[particleModelIndex]].rot.y = rand8();
                Hu3DData[object->mdlId[particleModelIndex]].rot.z = rand8();
                resultParticleMotionStates[particleModelIndex].horizontalDriftVelocity = (rand8() % 50 + -1.0f) / 100.0f;
                if (particleModelIndex % 2 == 0) {
                    resultParticleMotionStates[particleModelIndex].horizontalDriftVelocity *= -1.0f;
                }
                resultParticleMotionStates[particleModelIndex].verticalDriftVelocity = ((rand8() % 100 + 100) / 100.0f) * -1.0f;
                resultParticleMotionStates[particleModelIndex].depthDriftVelocity = (rand8() % 100 - 1) / 100;
                resultParticleMotionStates[particleModelIndex].rotationVelocityX = (rand8() % 100 - 50) / 5;
                resultParticleMotionStates[particleModelIndex].rotationVelocityY = (rand8() % 100 - 50) / 5;
                resultParticleMotionStates[particleModelIndex].rotationVelocityZ = (rand8() % 100 - 50) / 5;
                resultParticleMotionStates[particleModelIndex].animationState = 2;
                break;
            case 2:
                if (resultParticleMotionStates[particleModelIndex].framesUntilNextBurst-- <= 0) {
                    resultParticleMotionStates[particleModelIndex].framesUntilNextBurst = rand8() % 30 + 30;
                    resultParticleMotionStates[particleModelIndex].horizontalDriftVelocity = (rand8() % 50 + -1.0f) / 100.0f;
                    if (particleModelIndex % 2 == 0) {
                        resultParticleMotionStates[particleModelIndex].horizontalDriftVelocity *= -1.0f;
                    }
                    resultParticleMotionStates[particleModelIndex].verticalDriftVelocity = ((rand8() % 100 + 100) / 100.0f) *-1.0f;
                    resultParticleMotionStates[particleModelIndex].depthDriftVelocity = (rand8() % 100 - 1) / 100;
                    resultParticleMotionStates[particleModelIndex].rotationVelocityX = (rand8() % 100 - 50) / 5;
                    resultParticleMotionStates[particleModelIndex].rotationVelocityY = (rand8() % 100 - 50) / 5;
                    resultParticleMotionStates[particleModelIndex].rotationVelocityZ = (rand8() % 100 - 50) / 5;
                }
                resultParticleMotionStates[particleModelIndex].targetX += resultParticleMotionStates[particleModelIndex].horizontalDriftVelocity;
                resultParticleMotionStates[particleModelIndex].targetY += resultParticleMotionStates[particleModelIndex].verticalDriftVelocity;
                resultParticleMotionStates[particleModelIndex].targetZ += resultParticleMotionStates[particleModelIndex].depthDriftVelocity;
                Hu3DData[object->mdlId[particleModelIndex]].rot.x += resultParticleMotionStates[particleModelIndex].rotationVelocityX;
                Hu3DData[object->mdlId[particleModelIndex]].rot.y += resultParticleMotionStates[particleModelIndex].rotationVelocityY;
                Hu3DData[object->mdlId[particleModelIndex]].rot.z += resultParticleMotionStates[particleModelIndex].rotationVelocityZ;
                if (Hu3DData[object->mdlId[particleModelIndex]].pos.y <= 0.0f) {
                    resultParticleMotionStates[particleModelIndex].animationState = 3;
                }
                break;
            case 10:
                Hu3DModelAttrReset(object->mdlId[particleModelIndex], HU3D_ATTR_DISPOFF);
                if (particleModelIndex % 3 != 0) {
                    ambientColorChoice = rand8() % 4;
                } else {
                    ambientColorChoice = 5;
                }
                switch (ambientColorChoice) {
                    case 0:
                        Hu3DModelAmbSet(object->mdlId[particleModelIndex], 1.0f, 1.0f, 1.0f);
                        break;
                    case 1:
                        Hu3DModelAmbSet(object->mdlId[particleModelIndex], 1.0f, 0.0f, 0.0f);
                        break;
                    case 2:
                        Hu3DModelAmbSet(object->mdlId[particleModelIndex], 0.0f, 1.0f, 0.0f);
                        break;
                    case 3:
                        Hu3DModelAmbSet(object->mdlId[particleModelIndex], 0.0f, 0.0f, 1.0f);
                        break;
                    case 5:
                        break;
                }
                resultParticleMotionStates[particleModelIndex].targetX = Hu3DData[object->mdlId[particleModelIndex]].pos.x = particleEffectCenterX;
                resultParticleMotionStates[particleModelIndex].targetY = Hu3DData[object->mdlId[particleModelIndex]].pos.y = 375.0f;
                resultParticleMotionStates[particleModelIndex].targetZ = Hu3DData[object->mdlId[particleModelIndex]].pos.z = 460.0f;
                randomPositionOffset = rand8();
                if (rand8() % 2 == 0) {
                    randomPositionOffset *= -1;
                }
                resultParticleMotionStates[particleModelIndex].targetX += randomPositionOffset;
                randomPositionOffset = rand8();
                if (rand8() % 2 == 0) {
                    randomPositionOffset *= -1;
                }
                resultParticleMotionStates[particleModelIndex].targetY += randomPositionOffset;
                randomPositionOffset = rand8() % 50;
                if (rand8() % 2 == 0) {
                    randomPositionOffset *= -1;
                }
                resultParticleMotionStates[particleModelIndex].targetZ += randomPositionOffset;
                resultParticleMotionStates[particleModelIndex].horizontalDriftVelocity = (rand8() % 50 + -1.0f) / 100.0f;
                if (particleModelIndex % 2 == 0) {
                    resultParticleMotionStates[particleModelIndex].horizontalDriftVelocity *= -1.0f;
                }
                resultParticleMotionStates[particleModelIndex].verticalDriftVelocity = ((rand8() % 100 + 100) / 100.0f) * -1.0f;
                resultParticleMotionStates[particleModelIndex].depthDriftVelocity = (rand8() % 100 - 1) / 100;
                resultParticleMotionStates[particleModelIndex].rotationVelocityX = (rand8() % 100 - 50) / 5;
                resultParticleMotionStates[particleModelIndex].rotationVelocityY = (rand8() % 100 - 50) / 5;
                resultParticleMotionStates[particleModelIndex].rotationVelocityZ = (rand8() % 100 - 50) / 5;
                resultParticleMotionStates[particleModelIndex].animationFrame = 0;
                resultParticleMotionStates[particleModelIndex].framesUntilNextBurst = rand8() % 30 + 30;
                resultParticleMotionStates[particleModelIndex].animationState = 11;
                break;
            case 11:
                if (resultParticleMotionStates[particleModelIndex].framesUntilNextBurst-- <= 0) {
                    resultParticleMotionStates[particleModelIndex].framesUntilNextBurst = rand8() % 30 + 30;
                    resultParticleMotionStates[particleModelIndex].horizontalDriftVelocity = (rand8() % 50 + -1.0f) / 100.0f;
                    if (particleModelIndex % 2 == 0) {
                        resultParticleMotionStates[particleModelIndex].horizontalDriftVelocity *= -1.0f;
                    }
                    resultParticleMotionStates[particleModelIndex].verticalDriftVelocity = ((rand8() % 100 + 100) / 100.0f) * -1.0f;
                    resultParticleMotionStates[particleModelIndex].depthDriftVelocity = (rand8() % 100 - 1) / 100;
                    resultParticleMotionStates[particleModelIndex].rotationVelocityX = (rand8() % 100 - 50) / 5;
                    resultParticleMotionStates[particleModelIndex].rotationVelocityY = (rand8() % 100 - 50) / 5;
                    resultParticleMotionStates[particleModelIndex].rotationVelocityZ = (rand8() % 100 - 50) / 5;
                }
                resultParticleMotionStates[particleModelIndex].targetX += resultParticleMotionStates[particleModelIndex].horizontalDriftVelocity;
                resultParticleMotionStates[particleModelIndex].targetY += resultParticleMotionStates[particleModelIndex].verticalDriftVelocity / 2;
                resultParticleMotionStates[particleModelIndex].targetZ += resultParticleMotionStates[particleModelIndex].depthDriftVelocity;
                Hu3DData[object->mdlId[particleModelIndex]].rot.x += resultParticleMotionStates[particleModelIndex].rotationVelocityX;
                Hu3DData[object->mdlId[particleModelIndex]].rot.y += resultParticleMotionStates[particleModelIndex].rotationVelocityY;
                Hu3DData[object->mdlId[particleModelIndex]].rot.z += resultParticleMotionStates[particleModelIndex].rotationVelocityZ;
                if (resultParticleMotionStates[particleModelIndex].animationFrame++ >= 50) {
                    resultParticleMotionStates[particleModelIndex].animationState = 2;
                }
                break;
        }
        Hu3DData[object->mdlId[particleModelIndex]].pos.x = interpolateSmoothAverage(Hu3DData[object->mdlId[particleModelIndex]].pos.x, resultParticleMotionStates[particleModelIndex].targetX, 30.0f);
        Hu3DData[object->mdlId[particleModelIndex]].pos.y = interpolateSmoothAverage(Hu3DData[object->mdlId[particleModelIndex]].pos.y, resultParticleMotionStates[particleModelIndex].targetY, 30.0f);
        Hu3DData[object->mdlId[particleModelIndex]].pos.z = interpolateSmoothAverage(Hu3DData[object->mdlId[particleModelIndex]].pos.z, resultParticleMotionStates[particleModelIndex].targetZ, 30.0f);
    }
}

Void Story3CreateResultParticleModels(OMOBJ* object) {
    s32 particleModelIndex;

    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 17));
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 18));
    Hu3DModelAttrSet(object->mdlId[0], HU3D_ATTR_DISPOFF);
    for (particleModelIndex = 2; particleModelIndex < 80; particleModelIndex++) {
        if (particleModelIndex % 3 == 0) {
            object->mdlId[particleModelIndex] = Hu3DModelLink(object->mdlId[1]);
        } else {
            object->mdlId[particleModelIndex] = Hu3DModelLink(object->mdlId[0]);
        }
        Hu3DModelAttrSet(object->mdlId[particleModelIndex], HU3D_ATTR_DISPOFF);
        resultParticleMotionStates[particleModelIndex].framesUntilNextBurst = resultParticleMotionStates[particleModelIndex].animationFrame = 0;
        resultParticleMotionStates[particleModelIndex].animationState = 10;
    }
    object->objFunc = Story3ConfigureResultParticles;
}

Void Story3CreateResultObjects(Void) {
    OMOBJ* resultObject;
    OMOBJ* particleObject;

    resultObject = omAddObjEx(resultObjectManager, 0x100, 0x20, 0x20, -1, NULL);
    particleObject = omAddObjEx(resultObjectManager, 0x100, 0x80, 0, -1, Story3CreateResultParticleModels);
    sequenceCallbackState.object = resultObject;
    sequenceCallbackState.callback = NULL;
    sequenceCallbackState.object->data = &sequenceCallbackState;
    Story3LoadStoryResultModels(resultObject, resultSequenceState.sequenceVariant, resultSequenceState.boardId);
    while (TRUE) {
        Story3SleepOneFrame();
        if (sequenceCallbackState.callback != NULL) {
            sequenceCallbackState.callback(resultObject);
        }
        Story3AnimateResultLogos(resultObject);
    }
}

Void Story3SortPlayersForPresentation(s32 playerCount) {
    s32 playersByRank[4];
    s32 originalPlayerOrder[4];
    s32 playerTeams[4];
    s32 i, j;
    s32 teamSortOrder;

    playersByRank[0] = 0;
    for (i = 1; i < 4; i++) {
        playersByRank[i] = playersByRank[i - 1] + 1;
        if (playersByRank[i] >= 4) {
            playersByRank[i] = 0;
        }
        if (playerCount == playersByRank[i]) {
            resultSequenceState.currentPlayerIndex = i;
        }
    }
    if (resultSequenceState.sequenceVariant != 0) {
        resultSequenceState.winningPlayerIndex = playerCount;
    }
    if (resultSequenceState.teamMode == 1) {
        teamSortOrder = 0;
        for (i = 0; i < 4; i++) {
            playerTeams[0] = GWPlayer[playersByRank[0]].team;
            playerTeams[1] = GWPlayer[playersByRank[1]].team;
            playerTeams[2] = GWPlayer[playersByRank[2]].team;
            playerTeams[3] = GWPlayer[playersByRank[3]].team;
            originalPlayerOrder[i] = playersByRank[i];
        }
        OSReport("########## Sort Start WIN:%d  IDX (%d,%d), (%d,%d), (%d,%d), (%d,%d)\n",
            teamSortOrder, playerTeams[0], playersByRank[0], playerTeams[1], playersByRank[1], playerTeams[2], playersByRank[2], playerTeams[3], playersByRank[3]);
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                if (playerTeams[j] != -1 && teamSortOrder == playerTeams[j]) {
                    playerTeams[j] = -1;
                    playersByRank[i] = originalPlayerOrder[j];
                    break;
                }
            }
            if (i == 1) {
                teamSortOrder++;
                teamSortOrder %= 2;
            }
        }
        OSReport("########## Sort End WIN:%d  IDX %d, %d, %d, %d\n", teamSortOrder, playersByRank[0], playersByRank[1], playersByRank[2], playersByRank[3]);
    }
    for (i = 0; i < 4; i++) {
        Story3CopyPlayerStats(&resultSequenceState.playerResults[i].playerStats, playersByRank[i]);
    }
    for (i = 0; i < 4; i++) {
        resultSequenceState.playerResults[i].playerStats.coins = GWPlayer[playersByRank[i]].coins;
        resultSequenceState.playerResults[i].playerStats.stars = GWPlayer[playersByRank[i]].stars;
        resultSequenceState.playerResults[i].playerStats.minigameCoins = GWPlayer[playersByRank[i]].coins_mg;
        resultSequenceState.playerResults[i].playerStats.maxCoins = GWPlayer[playersByRank[i]].coins_max;
        resultSequenceState.playerResults[i].playerStats.questionSpaces = GWPlayer[playersByRank[i]].question_count;
    }
}

Void Story3AnimatePlayerMedal(OMOBJ* object) {
    float medalHeight;
    float medalRotation;
    float medalScale;
    s32 scoreDigit;
    s32 celebrationFrame;
    s32 scoreDigitCount = 3;
    StorySequenceProcessState* playerProcessState = object->data;
    char* medalAttachmentNames[][3] = {
        { "submit07-star_null3", "submit07-star_null1", "submit07-star_null1" },
        { "submit07-star_null4", "submit07-star_null2", "submit07-star_null1" },
        { "submit07-star_null5", "submit07-star_null3", "submit07-star_null1" }
    };
    Mtx medalMatrix;
    Mtx medalMatrix;
    Mtx medalMatrix;
    s32 animationFrame;
    s32 medalIndex;

    Hu3DModelPosSet(object->mdlId[3], playerModelPositions[playerProcessState->isComputer].x, 100.0f, playerModelPositions[playerProcessState->isComputer].z - 20.0f);
    Hu3DModelRotSet(object->mdlId[3], 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(object->mdlId[3], 0.0f, 0.0f, 0.0f);
    if ((scoreDigit = resultSequenceState.playerResults[playerProcessState->isComputer].playerStats.stars / 100) == 0) {
        scoreDigitCount = 2;
    } else {
        Hu3DMotionTimeSet(object->mdlId[9], (scoreDigit + 1) % 10);
    }
    if ((scoreDigit = (resultSequenceState.playerResults[playerProcessState->isComputer].playerStats.stars % 100) / 10) == 0 && scoreDigitCount == 2) {
        scoreDigitCount = 1;
    } else {
        Hu3DMotionTimeSet(object->mdlId[8], (scoreDigit + 1) % 10);
    }
    scoreDigit = resultSequenceState.playerResults[playerProcessState->isComputer].playerStats.stars % 10;
    Hu3DMotionTimeSet(object->mdlId[7], (scoreDigit + 1) % 10);
    Hu3DModelAttrReset(object->mdlId[3], HU3D_ATTR_DISPOFF);
    for (animationFrame = 0; animationFrame < 90; animationFrame++) {
        Story3SleepOneFrame();
        medalHeight = interpolateSineEaseIn(100.0f, 300.0f, animationFrame, 90.0f);
        medalRotation = interpolateSineEaseIn(0.0f, 1080.0f, animationFrame, 90.0f);
        medalScale = interpolateSineEaseIn(0.0f, 0.75f, animationFrame, 90.0f);
        Hu3DModelPosSet(object->mdlId[3], playerModelPositions[playerProcessState->isComputer].x, medalHeight, playerModelPositions[playerProcessState->isComputer].z - 20.0f);
        Hu3DModelRotSet(object->mdlId[3], 0.0f, medalRotation, 0.0f);
        Hu3DModelScaleSet(object->mdlId[3], medalScale, medalScale, medalScale);
        for (medalIndex = 0; medalIndex < scoreDigitCount; medalIndex++) {
            if (medalRotation >= 900.0f) {
                Hu3DModelAttrReset(object->mdlId[medalIndex + 7], HU3D_ATTR_DISPOFF);
            }
            Hu3DModelObjMtxGet(object->mdlId[3], medalAttachmentNames[scoreDigitCount - 1][medalIndex], medalMatrix);
            Hu3DModelPosSet(object->mdlId[medalIndex + 7], medalMatrix[0][3], medalMatrix[1][3], medalMatrix[2][3]);
            Hu3DModelRotSet(object->mdlId[medalIndex + 7], 0.0f, medalRotation, 0.0f);
            Hu3DModelScaleSet(object->mdlId[medalIndex + 7], medalScale, medalScale, medalScale);
        }
    }
    animationFrame = medalIndex = celebrationFrame = 0;
    if (playerProcessState->coins == 1) {
        Story3SetModelMotion(object, 1, 14, 5, 0);
    } else {
        Story3SetModelMotion(object, 1, 15, 5, 0);
    }
    do {
        Story3SleepOneFrame();
        if (playerProcessState->coins == 1) {
            medalScale = interpolateSinePulse(0.75f, 1.0f, celebrationFrame, 30.0f);
            if (celebrationFrame++ >= 29) {
                celebrationFrame = 0;
            }
            Hu3DModelScaleSet(object->mdlId[3], medalScale, medalScale, medalScale);
            for (medalIndex = 0; medalIndex < scoreDigitCount; medalIndex++) {
                Hu3DModelObjMtxGet(object->mdlId[3], medalAttachmentNames[scoreDigitCount - 1][medalIndex], medalMatrix);
                Hu3DModelPosSet(object->mdlId[medalIndex + 7], medalMatrix[0][3], medalMatrix[1][3], medalMatrix[2][3]);
                Hu3DModelRotSet(object->mdlId[medalIndex + 7], 0.0f, medalRotation, 0.0f);
                Hu3DModelScaleSet(object->mdlId[medalIndex + 7], medalScale, medalScale, medalScale);
            }
        }
        if (animationFrame == 60) {
            Story3SetModelMotion(object, 1, 1, 15, 1);
        }
        if (animationFrame == 120) {
            resultSequenceState.sequenceState = 1;
        }
        if (animationFrame++ >= 120 && (celebrationFrame == 0 || celebrationFrame == 15) && playerProcessState->rank == 1) {
            playerProcessState->rank = 2;
        }
    } while (playerProcessState->rank != 3);
    HuAudFXPlay(0xBB);
    for (animationFrame = 0; animationFrame < 60; animationFrame++) {
        Story3SleepOneFrame();
        if (animationFrame > 30) {
            medalHeight = interpolateCosineEaseOut(300.0f, 600.0f, animationFrame - 30, 30.0f);
        }
        medalRotation = interpolateCosineEaseOut(0.0f, 360.0f, animationFrame, 60.0f);
        Hu3DModelPosSet(object->mdlId[3], playerModelPositions[playerProcessState->isComputer].x, medalHeight, playerModelPositions[playerProcessState->isComputer].z - 20.0f);
        Hu3DModelRotSet(object->mdlId[3], 0.0f, medalRotation, 0.0f);
        for (medalIndex = 0; medalIndex < scoreDigitCount; medalIndex++) {
            if (medalRotation >= 180.0f) {
                Hu3DModelAttrSet(object->mdlId[medalIndex + 7], HU3D_ATTR_DISPOFF);
            }
            Hu3DModelObjMtxGet(object->mdlId[3], medalAttachmentNames[scoreDigitCount - 1][medalIndex], medalMatrix);
            Hu3DModelPosSet(object->mdlId[medalIndex + 7], medalMatrix[0][3], medalMatrix[1][3], medalMatrix[2][3]);
            Hu3DModelRotSet(object->mdlId[medalIndex + 7], 0.0f, medalRotation, 0.0f);
        }
    }
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    for (animationFrame = 0; animationFrame < 3; animationFrame++) {
        Hu3DModelAttrSet(object->mdlId[animationFrame + 7], HU3D_ATTR_DISPOFF);
    }
    playerProcessState->team = 0;
}

Void Story3AnimateTeamMedal(OMOBJ* object) {
    float medalHeight;
    float medalRotation;
    float medalScale;
    s32 scoreDigit;
    s32 celebrationFrame;
    s32 scoreDigitCount = 3;
    StorySequenceProcessState* playerProcessState = object->data;
    char* medalAttachmentNames[][3] = {
        { "coin-coin-null3", "coin-coin_null1", "coin-coin-null1" },
        { "coin-coin-null4", "coin-coin-null2", "coin-coin-null1" },
        { "coin-coin-null5", "coin-coin-null3", "coin-coin-null1" }
    };
    Mtx medalMatrix;
    Mtx medalMatrix;
    Mtx medalMatrix;
    s32 animationFrame;
    s32 medalIndex;

    Hu3DModelPosSet(object->mdlId[4], playerModelPositions[playerProcessState->isComputer].x, 100.0f, playerModelPositions[playerProcessState->isComputer].z - 20.0f);
    Hu3DModelRotSet(object->mdlId[4], 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(object->mdlId[4], 0.0f, 0.0f, 0.0f);
    if ((scoreDigit = resultSequenceState.playerResults[playerProcessState->isComputer].playerStats.coins / 100) == 0) {
        scoreDigitCount = 2;
    } else {
        Hu3DMotionTimeSet(object->mdlId[9], (scoreDigit + 1) % 10);
    }
    if ((scoreDigit = (resultSequenceState.playerResults[playerProcessState->isComputer].playerStats.coins % 100) / 10) == 0 && scoreDigitCount == 2) {
        scoreDigitCount = 1;
    } else {
        Hu3DMotionTimeSet(object->mdlId[8], (scoreDigit + 1) % 10);
    }
    scoreDigit = resultSequenceState.playerResults[playerProcessState->isComputer].playerStats.coins % 10;
    Hu3DMotionTimeSet(object->mdlId[7], (scoreDigit + 1) % 10);
    Hu3DModelAttrReset(object->mdlId[4], HU3D_ATTR_DISPOFF);
    for (animationFrame = 0; animationFrame < 90; animationFrame++) {
        Story3SleepOneFrame();
        medalHeight = interpolateSineEaseIn(100.0f, 300.0f, animationFrame, 90.0f);
        medalRotation = interpolateSineEaseIn(0.0f, 1080.0f, animationFrame, 90.0f);
        medalScale = interpolateSineEaseIn(0.0f, 0.75f, animationFrame, 90.0f);
        Hu3DModelPosSet(object->mdlId[4], playerModelPositions[playerProcessState->isComputer].x, medalHeight, playerModelPositions[playerProcessState->isComputer].z - 20.0f);
        Hu3DModelRotSet(object->mdlId[4], 0.0f, medalRotation, 0.0f);
        Hu3DModelScaleSet(object->mdlId[4], medalScale, medalScale, medalScale);
        for (medalIndex = 0; medalIndex < scoreDigitCount; medalIndex++) {
            if (medalRotation >= 900.0f) {
                Hu3DModelAttrReset(object->mdlId[medalIndex + 7], HU3D_ATTR_DISPOFF);
            }
            Hu3DModelObjMtxGet(object->mdlId[4], medalAttachmentNames[scoreDigitCount - 1][medalIndex], medalMatrix);
            Hu3DModelPosSet(object->mdlId[medalIndex + 7], medalMatrix[0][3], medalMatrix[1][3], medalMatrix[2][3]);
            Hu3DModelRotSet(object->mdlId[medalIndex + 7], 0.0f, medalRotation, 0.0f);
            Hu3DModelScaleSet(object->mdlId[medalIndex + 7], medalScale, medalScale, medalScale);
        }
    }
    animationFrame = medalIndex = celebrationFrame = 0;
    if (playerProcessState->coins == 1) {
        Story3SetModelMotion(object, 1, 14, 5, 0);
    } else {
        Story3SetModelMotion(object, 1, 15, 5, 0);
    }
    do {
        Story3SleepOneFrame();
        if (playerProcessState->coins == 1) {
            medalScale = interpolateSinePulse(0.75f, 1.0f, celebrationFrame, 30.0f);
            if (celebrationFrame++ >= 29) {
                celebrationFrame = 0;
            }
            Hu3DModelScaleSet(object->mdlId[4], medalScale, medalScale, medalScale);
            for (medalIndex = 0; medalIndex < scoreDigitCount; medalIndex++) {
                Hu3DModelObjMtxGet(object->mdlId[4], medalAttachmentNames[scoreDigitCount - 1][medalIndex], medalMatrix);
                Hu3DModelPosSet(object->mdlId[medalIndex + 7], medalMatrix[0][3], medalMatrix[1][3], medalMatrix[2][3]);
                Hu3DModelRotSet(object->mdlId[medalIndex + 7], 0.0f, medalRotation, 0.0f);
                Hu3DModelScaleSet(object->mdlId[medalIndex + 7], medalScale, medalScale, medalScale);
            }
        }
        if (animationFrame == 60) {
            Story3SetModelMotion(object, 1, 1, 15, 1);
        }
        if (animationFrame == 120) {
            resultSequenceState.sequenceState = 1;
        }
        if (animationFrame++ >= 120 && (celebrationFrame == 0 || celebrationFrame == 15) && playerProcessState->rank == 1) {
            playerProcessState->rank = 2;
        }
    } while (playerProcessState->rank != 3);
    Story3SetModelMotion(object, 1, 1, 15, 1);
    HuAudFXPlay(0xBC);
    for (animationFrame = 0; animationFrame < 60; animationFrame++) {
        Story3SleepOneFrame();
        if (animationFrame > 30) {
            medalHeight = interpolateCosineEaseOut(300.0f, 600.0f, animationFrame - 30, 30.0f);
        }
        medalRotation = interpolateCosineEaseOut(0.0f, 360.0f, animationFrame, 60.0f);
        Hu3DModelPosSet(object->mdlId[4], playerModelPositions[playerProcessState->isComputer].x, medalHeight, playerModelPositions[playerProcessState->isComputer].z - 20.0f);
        Hu3DModelRotSet(object->mdlId[4], 0.0f, medalRotation, 0.0f);
        for (medalIndex = 0; medalIndex < scoreDigitCount; medalIndex++) {
            if (medalRotation >= 180.0f) {
                Hu3DModelAttrSet(object->mdlId[medalIndex + 7], HU3D_ATTR_DISPOFF);
            }
            Hu3DModelObjMtxGet(object->mdlId[4], medalAttachmentNames[scoreDigitCount - 1][medalIndex], medalMatrix);
            Hu3DModelPosSet(object->mdlId[medalIndex + 7], medalMatrix[0][3], medalMatrix[1][3], medalMatrix[2][3]);
            Hu3DModelRotSet(object->mdlId[medalIndex + 7], 0.0f, medalRotation, 0.0f);
        }
    }
    Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
    for (animationFrame = 0; animationFrame < 3; animationFrame++) {
        Hu3DModelAttrSet(object->mdlId[animationFrame + 7], HU3D_ATTR_DISPOFF);
    }
    playerProcessState->team = 0;
}

Void Story3AwardPlayerStar(OMOBJ* object) {
    StorySequenceProcessState* playerProcessState;

    playerProcessState = object->data;
    Story3SleepFrames(25);
    Story3SetModelMotion(object, 1, 15, 5, 0);
    Story3SleepFrames(10);
    while (TRUE) {
        Story3SleepOneFrame();
        if (Hu3DMotionTimeGet(object->mdlId[1]) >= 40.0f) {
            break;
        }
    }
    Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_PAUSE);
    playerProcessState->team = 0;
}

Void Story3AnimateAwardedStarParticles(OMOBJ* object) {
    float particleScale;
    float particleSpin;
    float initialParticleHeight;
    float playerModelScale;
    float lightAlpha;
    float starY;
    float starRotation;
    StorySequenceProcessState* playerProcessState;
    s32 playerIndex;
    s32 lightBrightness;
    s32 initialLightBrightness;
    s32 animationFrame;
    s32 lightColor;
    s32 starAnimationFrame;

    particleSpin = 0.0f;
    playerProcessState = object->data;
    playerIndex = playerProcessState->workValue0;
    initialLightBrightness = playerSequenceProcessStates[playerIndex].object->work[0];
    lightBrightness = 0xFF;
    Hu3DModelPosSet(playerSequenceProcessStates[playerIndex].object->mdlId[6], playerModelPositions[playerIndex].x, 500.0f + playerModelPositions[playerIndex].y + playerModelVerticalOffset, playerModelPositions[playerIndex].z + 0.1f * playerModelVerticalOffset);
    Hu3DModelScaleSet(playerSequenceProcessStates[playerIndex].object->mdlId[6], 0.0f, 1.25f, 0.0f);
    Hu3DModelTPLvlSet(playerSequenceProcessStates[playerIndex].object->mdlId[6], 0.0f);
    Hu3DModelAttrReset(playerSequenceProcessStates[playerIndex].object->mdlId[6], HU3D_ATTR_DISPOFF);
    for (animationFrame = 0; animationFrame <= 5; animationFrame++) {
        Story3SleepOneFrame();
        playerSequenceProcessStates[playerIndex].object->work[0] = interpolateSineEaseIn(initialLightBrightness, lightBrightness, animationFrame, 5.0f);
        playerModelScale = interpolateSineEaseIn(0.0f, 1.5f, animationFrame, 5.0f);
        lightAlpha = interpolateSineEaseIn(0.0f, 0.5f, animationFrame, 5.0f);
        lightColor = interpolateSineEaseIn(0.0f, 128.0f, animationFrame, 5.0f);
        Hu3DModelScaleSet(playerSequenceProcessStates[playerIndex].object->mdlId[6], playerModelScale, 1.25f, playerModelScale);
        Hu3DModelTPLvlSet(playerSequenceProcessStates[playerIndex].object->mdlId[6], lightAlpha);
        Hu3DLLightColorSet(playerSequenceProcessStates[playerIndex].object->mdlId[1], resultSequenceState.playerLights[playerIndex].firstLightId, lightColor, lightColor, lightColor, 0xFF);
        Hu3DLLightColorSet(playerSequenceProcessStates[playerIndex].object->mdlId[2], resultSequenceState.playerLights[playerIndex].secondLightId, lightColor, lightColor, lightColor, 0xFF);
    }
    playerSequenceProcessStates[playerIndex].object->work[1] = 1;
    Story3SleepFrames(20);
    Story3SetModelMotion(object, 1, 3, 15, 0);
    Story3WaitForModelMotionNearEnd(object, 1, 3, 15);
    do {
        Story3SleepOneFrame();
    } while (playerProcessState->callbackState == 0);
    Hu3DModelPosSet(object->mdlId[5], playerModelPositions[playerProcessState->workValue0].x, 300.0f, playerModelPositions[playerProcessState->workValue0].z - 50.0f);
    Hu3DModelRotSet(object->mdlId[5], 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(object->mdlId[5], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrReset(object->mdlId[5], HU3D_ATTR_DISPOFF);
    HuAudFXPlay(0x338);
    for (starAnimationFrame = 0; starAnimationFrame < 60; starAnimationFrame++) {
        Story3SleepOneFrame();
        starY = 10.0 + 10.0 * sind(particleSpin);
        starRotation = 10.0 * sind(2.0f * particleSpin);
        if (starAnimationFrame < 10) {
            particleScale = interpolateCosineEaseOut(0.0f, 1.0f, starAnimationFrame, 10.0f);
        } else {
            particleScale = interpolateCosineEaseOut(1.0f, 0.5f, starAnimationFrame - 10, 5.0f);
        }
        if (starAnimationFrame == 10) {
            resultSequenceState.playerParticleManagers[playerProcessState->workValue0] = Story3CreateStandardParticleManager();
            Hu3DParManPosSet(resultSequenceState.playerParticleManagers[playerProcessState->workValue0], playerModelPositions[playerProcessState->workValue0].x, 300.0f + starY, playerModelPositions[playerProcessState->workValue0].z);
        }
        if ((particleSpin += 2.0f) >= 360.0f) {
            particleSpin -= 360.0f;
        }
        Hu3DModelPosSet(object->mdlId[5], playerModelPositions[playerProcessState->workValue0].x, 300.0f + starY, playerModelPositions[playerProcessState->workValue0].z - 50.0f);
        Hu3DModelRotSet(object->mdlId[5], 0.0f, starRotation, 0.0f);
        Hu3DModelScaleSet(object->mdlId[5], particleScale, particleScale, particleScale);
    }
    Hu3DParManKill(resultSequenceState.playerParticleManagers[playerProcessState->workValue0]);
    resultSequenceState.playerParticleManagers[playerProcessState->workValue0] = -1;
    starY = Hu3DData[object->mdlId[5]].pos.y;
    initialParticleHeight = starY;
    starRotation = Hu3DData[object->mdlId[5]].rot.y;
    particleScale = 0.5f;
    particleSpin = 0.0f;
    for (starAnimationFrame = 0; starAnimationFrame < 90; starAnimationFrame++) {
        Story3SleepOneFrame();
        if (starAnimationFrame == 45) {
            HuAudFXPlay(0x339);
        }
        if (starAnimationFrame > 45) {
            starY = interpolateCosineEaseOut(initialParticleHeight, 100.0f, starAnimationFrame - 45, 45.0f);
            particleScale = interpolateCosineEaseOut(0.5f, 0.0f, starAnimationFrame - 45, 45.0f);
        }
        starRotation += particleSpin += 0.25f;
        Hu3DModelPosSet(object->mdlId[5], playerModelPositions[playerProcessState->workValue0].x, starY, playerModelPositions[playerProcessState->workValue0].z - 50.0f);
        Hu3DModelRotSet(object->mdlId[5], 0.0f, starRotation, 0.0f);
        Hu3DModelScaleSet(object->mdlId[5], particleScale, particleScale, particleScale);
    }
    Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
    HuAudFXPlay(8);
    resultSequenceState.playerResults[playerProcessState->workValue0].playerStats.stars++;
    GWPlayer[resultSequenceState.playerResults[playerProcessState->workValue0].playerStats.playerIndex].stars++;
    if (GWPlayer[resultSequenceState.playerResults[playerProcessState->workValue0].playerStats.playerIndex].stars >= 999) {
        GWPlayer[resultSequenceState.playerResults[playerProcessState->workValue0].playerStats.playerIndex].stars = 999;
    }
    playerProcessState->callback = 0;
}

Void Story3FinishPlayerResultProcess(OMOBJ* object) {
    StorySequenceProcessState* playerProcessState;

    playerProcessState = object->data;
    Story3StartPlayerPresentationProcess(playerProcessState->workValue0);
    playerProcessState->callback = 0;
}

Void Story3NoopResultCallback(OMOBJ* object) {
}

Void Story3UpdatePlayerRankMarker(OMOBJ* object) {
    float rankMarkerHeights[] = { 0.0f, -30.0f, -60.0f, -90.0f };
    float rankMarkerY;
    StorySequenceProcessState* playerProcessState;
    s32 winningTeamFirstPlayer;
    s32 rankIndex;
    s32 animationFrame;

    playerProcessState = object->data;
    if (resultSequenceState.teamMode == 1) {
        winningTeamFirstPlayer = 0;
        if (resultSequenceState.winningTeam == 1) {
            winningTeamFirstPlayer = 2;
        }
        if ((resultSequenceState.winningTeam == 0 && playerProcessState->workValue0 <= 1) || (resultSequenceState.winningTeam == 1 && playerProcessState->workValue0 >= 2)) {
            Hu3DModelPosSet(object->mdlId[2], 100.0f + playerModelPositions[playerProcessState->workValue0].x, -90.0f, playerModelPositions[playerProcessState->workValue0].z);
            Story3SleepFrames(90);
            if (playerProcessState->workValue0 == winningTeamFirstPlayer) {
                Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
            }
            if (object->work[0] == 0) {
                HuAudFXPlay(0xA8);
            }
            for (animationFrame = 0; animationFrame <= 30; animationFrame++) {
                Story3SleepOneFrame();
                rankMarkerY = interpolateSineEaseIn(-90.0f, 0.0f, animationFrame, 30.0f);
                Hu3DModelPosSet(object->mdlId[1], playerModelPositions[playerProcessState->workValue0].x, 90.0f + rankMarkerY, playerModelPositions[playerProcessState->workValue0].z - 20.0f);
                Hu3DModelPosSet(object->mdlId[2], 100.0f + playerModelPositions[playerProcessState->workValue0].x, rankMarkerY, playerModelPositions[playerProcessState->workValue0].z - 20.0f);
            }
        }
    } else {
        Hu3DModelPosSet(object->mdlId[2], playerModelPositions[playerProcessState->workValue0].x, -90.0f, playerModelPositions[playerProcessState->workValue0].z);
        if (resultSequenceState.playerResults[playerProcessState->workValue0].playerStats.rank != 4) {
        }
        for (rankIndex = 0; rankIndex < 4; rankIndex++) {
            if (object->work[0] == 0 && rankIndex >= 1) {
                HuAudFXPlay(0xA5 + rankIndex);
            }
            if (rankIndex != 3) {
                for (animationFrame = 0; animationFrame < 30; animationFrame++) {
                    Story3SleepOneFrame();
                    if (rankMarkerHeights[resultSequenceState.playerResults[playerProcessState->workValue0].playerStats.rank - 1] >= rankMarkerHeights[3 - rankIndex] && 4 - rankIndex < 4) {
                        rankMarkerY = interpolateSineEaseIn(rankMarkerHeights[4 - rankIndex], rankMarkerHeights[3 - rankIndex], animationFrame, 30.0f);
                        if (rankIndex == 1) {
                            Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
                        }
                        Hu3DModelPosSet(object->mdlId[1], playerModelPositions[playerProcessState->workValue0].x, 90.0f + rankMarkerY, playerModelPositions[playerProcessState->workValue0].z - 20.0f);
                        Hu3DModelPosSet(object->mdlId[2], playerModelPositions[playerProcessState->workValue0].x, rankMarkerY, playerModelPositions[playerProcessState->workValue0].z - 20.0f);
                    }
                }
            } else {
                for (animationFrame = 0; animationFrame < 10; animationFrame++) {
                    Story3SleepOneFrame();
                    if ((rankMarkerHeights[resultSequenceState.playerResults[playerProcessState->workValue0].playerStats.rank - 1] >= rankMarkerHeights[3 - rankIndex]) && ((4 - rankIndex) < 4)) {
                        rankMarkerY = interpolateSineEaseIn(rankMarkerHeights[4 - rankIndex], rankMarkerHeights[3 - rankIndex], animationFrame, 10.0f);
                        if (rankIndex == 1) {
                            Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
                        }
                        Hu3DModelPosSet(object->mdlId[1], playerModelPositions[playerProcessState->workValue0].x, 90.0f + rankMarkerY, playerModelPositions[playerProcessState->workValue0].z - 20.0f);
                        Hu3DModelPosSet(object->mdlId[2], playerModelPositions[playerProcessState->workValue0].x, rankMarkerY, playerModelPositions[playerProcessState->workValue0].z - 20.0f);
                    }
                }
            }
            if (rankIndex == 2) {
                Story3SleepFrames(120);
            } else if (rankIndex != 3) {
                Story3SleepFrames(60);
            }
        }
    }
    playerProcessState->callback = 0;
}

Void Story3FindTopMinigameScores(s32 playerIndex) {
    s32 playerScores[4];
    s32 tiedPlayerCount;
    s32 highestScore;
    s32 i;

    tiedPlayerCount = 0;
    highestScore = -1;
    for (i = 0; i < 4; i++) {
        sequencePlayerCountAndWinner[i] = playerSequenceOrder[i] = 0;
    }
    for (i = 0; i < 4; i++) {
        switch (playerIndex) {
            case 0:
                playerScores[i] = resultSequenceState.playerResults[i].playerStats.stars;
                break;
            case 1:
                playerScores[i] = resultSequenceState.playerResults[i].playerStats.coins;
                break;
        }
    }
    for (i = 0; i < 4; i++) {
        if (highestScore <= playerScores[i]) {
            highestScore = playerScores[i];
        }
    }
    for (i = 0; i < 4; i++) {
        if (highestScore == playerScores[i]) {
            sequencePlayerCountAndWinner[0]++;
            playerSequenceOrder[tiedPlayerCount++] = i;
        }
    }
}

Void Story3FindTiedMinigameScores(s32 playerIndex) {
    s32 playerScores[4];
    s32 tiedPlayerCount;
    s32 highestScore;
    s32 i;

    tiedPlayerCount = 0;
    highestScore = -1;
    for (i = 0; i < 4; i++) {
        sequencePlayerCountAndWinner[i] = 0;
        playerSequenceOrder[i] = -1;
    }
    for (i = 0; i < 4; i++) {
        switch (playerIndex) {
            case 0:
                playerScores[i] = resultSequenceState.playerResults[i].playerStats.minigameCoins;
                break;
            case 1:
                playerScores[i] = resultSequenceState.playerResults[i].playerStats.maxCoins;
                break;
            case 2:
                playerScores[i] = resultSequenceState.playerResults[i].playerStats.questionSpaces;
                break;
        }
    }
    for (i = 0; i < 4; i++) {
        if (highestScore <= playerScores[i]) {
            highestScore = playerScores[i];
        }
    }
    for (i = 0; i < 4; i++) {
        if (highestScore == playerScores[i]) {
            sequencePlayerCountAndWinner[0]++;
            playerSequenceOrder[tiedPlayerCount++] = i;
        }
    }
}

Void Story3SortPlayersByMinigameScore(Void) {
    s32 playerOrderByScore[4];
    s32 playerIndexSwap;
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        playerOrderByScore[i] = i;
    }
    for (i = 0; i < 4; i++) {
        for (j = i; j < 4; j++) {
            if (resultSequenceState.playerResults[playerOrderByScore[j]].playerStats.stars >= resultSequenceState.playerResults[playerOrderByScore[i]].playerStats.stars) {
                playerIndexSwap = playerOrderByScore[i];
                playerOrderByScore[i] = playerOrderByScore[j];
                playerOrderByScore[j] = playerIndexSwap;
            }
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = i; j < 4; j++) {
            if ((resultSequenceState.playerResults[playerOrderByScore[j]].playerStats.stars == resultSequenceState.playerResults[playerOrderByScore[i]].playerStats.stars) && (resultSequenceState.playerResults[playerOrderByScore[j]].playerStats.coins >= resultSequenceState.playerResults[playerOrderByScore[i]].playerStats.coins)) {
                playerIndexSwap = playerOrderByScore[i];
                playerOrderByScore[i] = playerOrderByScore[j];
                playerOrderByScore[j] = playerIndexSwap;
            }
        }
    }
    resultSequenceState.playerResults[playerOrderByScore[0]].playerStats.rank = j = 1;
    for (i = 1; i < 4; i++) {
        j++;
        resultSequenceState.playerResults[playerOrderByScore[i]].playerStats.rank = j;
        if ((resultSequenceState.playerResults[playerOrderByScore[i]].playerStats.stars == resultSequenceState.playerResults[playerOrderByScore[i - 1]].playerStats.stars) && (resultSequenceState.playerResults[playerOrderByScore[i]].playerStats.coins == resultSequenceState.playerResults[playerOrderByScore[i - 1]].playerStats.coins)) {
            resultSequenceState.playerResults[playerOrderByScore[i]].playerStats.rank = resultSequenceState.playerResults[playerOrderByScore[i - 1]].playerStats.rank;
        }
    }
}

Void Story3CountFirstPlacePlayers(Void) {
    s32 firstPlacePlayerCount;
    s32 i;

    firstPlacePlayerCount = 0;
    for (i = 0; i < 4; i++) {
        sequencePlayerCountAndWinner[i] = playerSequenceOrder[i] = 0;
    }
    for (i = 0; i < 4; i++) {
        if (resultSequenceState.playerResults[i].playerStats.rank == 1) {
            sequencePlayerCountAndWinner[0]++;
            playerSequenceOrder[firstPlacePlayerCount++] = i;
            sequencePlayerCountAndWinner[1] = i;
        }
    }
}

Void Story3CalculatePlayerMinigameScores(Void) {
    s32 playerMinigameScores[4];
    s32 highestMinigameScore;
    s32 i;

    highestMinigameScore = -1;
    for (i = 0; i < 4; i++) {
        playerMinigameScores[i] = playerSequenceScoreOrder[i];
    }
    for (i = 0; i < 4; i++) {
        if (highestMinigameScore <= playerMinigameScores[i]) {
            highestMinigameScore = playerMinigameScores[i];
        }
    }
    for (i = 0; i < 4; i++) {
        if (highestMinigameScore == playerMinigameScores[i]) {
            sequencePlayerCountAndWinner[1] = i;
        }
    }
    for (i = 0; i < sequencePlayerCountAndWinner[0]; i++) {
        if (sequencePlayerCountAndWinner[1] != playerSequenceOrder[i]) {
            resultSequenceState.playerResults[playerSequenceOrder[i]].playerStats.rank = 2;
        }
    }
}

Void Story3SetupPlayerResultModels(OMOBJ* object, s32 count) {
    StoryPlayerStats* playerStats;
    s32 teamModelVariant;

    playerStats = &resultSequenceState.playerResults[count].playerStats;
    object->mdlId[1] = CharModelCreate(playerStats->characterId, 4);
    object->mtnId[1] = CharMotionCreate(playerStats->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0));
    object->mtnId[2] = CharMotionCreate(playerStats->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 72));
    object->mtnId[5] = CharMotionCreate(playerStats->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 47));
    object->mtnId[3] = CharMotionCreate(playerStats->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 23));
    object->mtnId[4] = CharMotionCreate(playerStats->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 24));
    object->mtnId[14] = CharMotionCreate(playerStats->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 72));
    object->mtnId[15] = CharMotionCreate(playerStats->characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 73));
    CharMotionDataClose(playerStats->characterId);
    Hu3DModelPosSet(object->mdlId[1], playerModelPositions[count].x, playerModelPositions[count].y, playerModelPositions[count].z - 20.0f);
    Hu3DModelRotSet(object->mdlId[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(object->mdlId[1], 1.0f, 1.0f, 1.0f);
    Hu3DModelShadowSet(object->mdlId[1]);
    Story3SetModelMotion(object, 1, 1, 0, 1);
    if (resultSequenceState.teamMode == 1) {
        teamModelVariant = 0;
        if (count > 1) {
            teamModelVariant = 1;
        }
        object->mdlId[2] = Hu3DModelCreateFile(teamModelVariant + DATA_MAKE_NUM(DATADIR_MSTORY3, 144));
        Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
        Hu3DModelScaleSet(object->mdlId[2], 1.25f, 1.0f, 1.5f);
        Hu3DModelShadowMapSet(object->mdlId[2]);
    } else {
        object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 12));
        Hu3DModelScaleSet(object->mdlId[2], 1.5f, 1.0f, 1.5f);
        Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
        Hu3DModelShadowMapSet(object->mdlId[2]);
    }
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 14));
    Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 15));
    Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 13));
    Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
    object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 11));
    Hu3DModelAttrSet(object->mdlId[6], HU3D_ATTR_DISPOFF);
    for (count = 0; count < 3; count++) {
        object->mdlId[count + 7] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 16));
        Hu3DModelAttrSet(object->mdlId[count + 7], HU3D_ATTR_DISPOFF);
        Hu3DMotionSpeedSet(object->mdlId[count + 7], 0.0f);
    }
    resultSequenceState.playerLights[count].firstLightId = Hu3DLLightCreate(object->mdlId[1], 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0);
    Hu3DLLightInfinitytSet(object->mdlId[1], resultSequenceState.playerLights[count].firstLightId);
    Hu3DLLightPosAimSet(object->mdlId[1], resultSequenceState.playerLights[count].firstLightId, 0.0f, 700.0f, 1200.0f, 0.0f, 0.0f, 640.0f);
    resultSequenceState.playerLights[count].secondLightId = Hu3DLLightCreate(object->mdlId[2], 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0, 0);
    Hu3DLLightInfinitytSet(object->mdlId[2], resultSequenceState.playerLights[count].secondLightId);
    Hu3DLLightPosAimSet(object->mdlId[2], resultSequenceState.playerLights[count].secondLightId, 0.0f, 700.0f, 1200.0f, 0.0f, 0.0f, 640.0f);
}

Void Story3CreatePlayerResultObject(Void) {
    OMOBJ* playerObject;
    s32 playerIndex;

    playerIndex = resultSequenceState.playerObjectCount++;
    playerObject = omAddObjEx(resultObjectManager, 0x100, 0x10, 0x10, -1, NULL);
    playerSequenceProcessStates[playerIndex].object = playerObject;
    playerSequenceProcessStates[playerIndex].callback = NULL;
    playerSequenceProcessStates[playerIndex].workValue0 = playerIndex;
    playerSequenceProcessStates[playerIndex].object->data = &playerSequenceProcessStates[playerIndex];
    Story3SetupPlayerResultModels(playerSequenceProcessStates[playerIndex].object, playerIndex);
    while (TRUE) {
        Story3SleepOneFrame();
        if (playerSequenceProcessStates[playerIndex].callback != NULL) {
            playerSequenceProcessStates[playerIndex].callback(playerSequenceProcessStates[playerIndex].object);
        }
    }
}

Void Story3NoopSequenceCallback(OMOBJ* object) {
}

s32 resultMessageIds[][27] = {
    {
        MAKE_MESSID(37, 0x00), MAKE_MESSID(37, 0x01), MAKE_MESSID(37, 0x02),
        MAKE_MESSID(37, 0x03), MAKE_MESSID(37, 0x05), MAKE_MESSID(37, 0x06),
        MAKE_MESSID(37, 0x0B), MAKE_MESSID(37, 0x0C), MAKE_MESSID(37, 0x0D),
        MAKE_MESSID(37, 0x0E), MAKE_MESSID(37, 0x0F), MAKE_MESSID(37, 0x10),
        MAKE_MESSID(37, 0x07), MAKE_MESSID(37, 0x08), MAKE_MESSID(37, 0x09),
        MAKE_MESSID(37, 0x0A), MAKE_MESSID(37, 0x11), MAKE_MESSID(37, 0x12),
        MAKE_MESSID(37, 0x13), MAKE_MESSID(37, 0x14), MAKE_MESSID(37, 0x15),
        MAKE_MESSID(37, 0x16), MAKE_MESSID(37, 0x17), MAKE_MESSID(37, 0x8C),
        MAKE_MESSID(37, 0x8D), MAKE_MESSID(37, 0x8E), MAKE_MESSID(37, 0x9E)
    },
    {
        MAKE_MESSID(37, 0x19), MAKE_MESSID(37, 0x1A), MAKE_MESSID(37, 0x1B),
        MAKE_MESSID(37, 0x1C), MAKE_MESSID(37, 0x1D), MAKE_MESSID(37, 0x1E),
        MAKE_MESSID(37, 0x23), MAKE_MESSID(37, 0x24), MAKE_MESSID(37, 0x25),
        MAKE_MESSID(37, 0x26), MAKE_MESSID(37, 0x27), MAKE_MESSID(37, 0x28),
        MAKE_MESSID(37, 0x1F), MAKE_MESSID(37, 0x20), MAKE_MESSID(37, 0x21),
        MAKE_MESSID(37, 0x22), MAKE_MESSID(37, 0x29), MAKE_MESSID(37, 0x2A),
        MAKE_MESSID(37, 0x2B), MAKE_MESSID(37, 0x2C), MAKE_MESSID(37, 0x2D),
        MAKE_MESSID(37, 0x2E), MAKE_MESSID(37, 0x2F), MAKE_MESSID(37, 0x8F),
        MAKE_MESSID(37, 0x90), MAKE_MESSID(37, 0x91), MAKE_MESSID(37, 0x9F)
    },
    {
        MAKE_MESSID(37, 0x30), MAKE_MESSID(37, 0x31), MAKE_MESSID(37, 0x32),
        MAKE_MESSID(37, 0x33), MAKE_MESSID(37, 0x34), MAKE_MESSID(37, 0x35),
        MAKE_MESSID(37, 0x3A), MAKE_MESSID(37, 0x3B), MAKE_MESSID(37, 0x3C),
        MAKE_MESSID(37, 0x3D), MAKE_MESSID(37, 0x3E), MAKE_MESSID(37, 0x3F),
        MAKE_MESSID(37, 0x36), MAKE_MESSID(37, 0x37), MAKE_MESSID(37, 0x38),
        MAKE_MESSID(37, 0x39), MAKE_MESSID(37, 0x40), MAKE_MESSID(37, 0x41),
        MAKE_MESSID(37, 0x42), MAKE_MESSID(37, 0x43), MAKE_MESSID(37, 0x44),
        MAKE_MESSID(37, 0x45), MAKE_MESSID(37, 0x46), MAKE_MESSID(37, 0x92),
        MAKE_MESSID(37, 0x93), MAKE_MESSID(37, 0x94), MAKE_MESSID(37, 0xA0)
    },
    {
        MAKE_MESSID(37, 0x47), MAKE_MESSID(37, 0x48), MAKE_MESSID(37, 0x49),
        MAKE_MESSID(37, 0x4A), MAKE_MESSID(37, 0x4B), MAKE_MESSID(37, 0x4C),
        MAKE_MESSID(37, 0x51), MAKE_MESSID(37, 0x52), MAKE_MESSID(37, 0x53),
        MAKE_MESSID(37, 0x54), MAKE_MESSID(37, 0x55), MAKE_MESSID(37, 0x56),
        MAKE_MESSID(37, 0x4D), MAKE_MESSID(37, 0x4E), MAKE_MESSID(37, 0x4F),
        MAKE_MESSID(37, 0x50), MAKE_MESSID(37, 0x57), MAKE_MESSID(37, 0x58),
        MAKE_MESSID(37, 0x59), MAKE_MESSID(37, 0x5A), MAKE_MESSID(37, 0x5B),
        MAKE_MESSID(37, 0x5C), MAKE_MESSID(37, 0x5D), MAKE_MESSID(37, 0x95),
        MAKE_MESSID(37, 0x96), MAKE_MESSID(37, 0x97), MAKE_MESSID(37, 0xA1)
    },
    {
        MAKE_MESSID(37, 0x5E), MAKE_MESSID(37, 0x5F), MAKE_MESSID(37, 0x60),
        MAKE_MESSID(37, 0x61), MAKE_MESSID(37, 0x62), MAKE_MESSID(37, 0x63),
        MAKE_MESSID(37, 0x68), MAKE_MESSID(37, 0x69), MAKE_MESSID(37, 0x6A),
        MAKE_MESSID(37, 0x6B), MAKE_MESSID(37, 0x6C), MAKE_MESSID(37, 0x6D),
        MAKE_MESSID(37, 0x64), MAKE_MESSID(37, 0x65), MAKE_MESSID(37, 0x66),
        MAKE_MESSID(37, 0x67), MAKE_MESSID(37, 0x6E), MAKE_MESSID(37, 0x6F),
        MAKE_MESSID(37, 0x70), MAKE_MESSID(37, 0x71), MAKE_MESSID(37, 0x72),
        MAKE_MESSID(37, 0x73), MAKE_MESSID(37, 0x74), MAKE_MESSID(37, 0x98),
        MAKE_MESSID(37, 0x99), MAKE_MESSID(37, 0x9A), MAKE_MESSID(37, 0xA2)
    },
    {
        MAKE_MESSID(37, 0x75), MAKE_MESSID(37, 0x76), MAKE_MESSID(37, 0x77),
        MAKE_MESSID(37, 0x78), MAKE_MESSID(37, 0x79), MAKE_MESSID(37, 0x7A),
        MAKE_MESSID(37, 0x7F), MAKE_MESSID(37, 0x80), MAKE_MESSID(37, 0x81),
        MAKE_MESSID(37, 0x82), MAKE_MESSID(37, 0x83), MAKE_MESSID(37, 0x84),
        MAKE_MESSID(37, 0x7B), MAKE_MESSID(37, 0x7C), MAKE_MESSID(37, 0x7D),
        MAKE_MESSID(37, 0x7E), MAKE_MESSID(37, 0x85), MAKE_MESSID(37, 0x86),
        MAKE_MESSID(37, 0x87), MAKE_MESSID(37, 0x88), MAKE_MESSID(37, 0x89),
        MAKE_MESSID(37, 0x8A), MAKE_MESSID(37, 0x8B), MAKE_MESSID(37, 0x9B),
        MAKE_MESSID(37, 0x9C), MAKE_MESSID(37, 0x9D), MAKE_MESSID(37, 0xA3)
    }
};

Void Story3ShowResultMessageSequence(s32 windowId, s32 messageId, s32 mode, s32 mode3) {
    s32 waitForWindowState = 0;
    s32 messageLineCount = 0;
    s32 i;

    resultSequenceState.sequenceState = 0;
    Story3SetModelMotion(sharedSequenceProcessState.object, 1, mode3, 15, 1);
    messageLineCount = (s16) HuWinKeyWaitNumGet(resultMessageIds[resultSequenceState.boardId][messageId]);
    if (mode == -1) {
        mode = messageLineCount - 1;
    }
    HuWinComKeyReset();
    if (resultSequenceState.winningPlayerIndex == -1) {
        for (i = 0; i < 10; i++) {
            HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, GWMessDelayGet());
        }
    }
    HuWinMesSet(windowId, resultMessageIds[resultSequenceState.boardId][messageId]);
    for (i = 0; i < messageLineCount; i++) {
        waitForWindowState = 0;
        do {
            HuPrcVSleep();
        } while (HuWinStatGet(windowId) != 1);
        if (i == mode) {
            HuWinAttrSet(windowId, 0x400);
            waitForWindowState = 1;
            resultSequenceState.sequenceState = 99;
        }
        while (TRUE) {
            HuPrcVSleep();
            if (waitForWindowState != 0) {
                if (waitForWindowState == 1) {
                    if (HuWinStatGet(windowId) != 1) {
                        waitForWindowState = 2;
                        continue;
                    }
                }
                if (waitForWindowState == 2) {
                    do {
                        Story3SleepOneFrame();
                    } while (resultSequenceState.sequenceState != 1);
                    HuWinAttrReset(windowId, 0x400);
                    HuWinKeyWaitEntry(windowId);
                    break;
                }
            } else if (HuWinStatGet(windowId) != 1) {
                break;
            }
        }
    }
    Story3DisplayMessageAndWait(windowId, -1);
    Story3SetWindowMessage(windowId, MAKE_MESSID(37, 4));
    HuWinComKeyReset();
    Story3SetModelMotion(sharedSequenceProcessState.object, 1, 1, 15, 1);
}

Void Story3ShowBoardResultMessage(s32 windowId, s32 messageId, s32 mode) {
    Story3SetModelMotion(sharedSequenceProcessState.object, 1, mode, 15, 1);
    Story3SetWindowMessage(windowId, resultMessageIds[resultSequenceState.boardId][messageId]);
    Story3DisplayMessageAndWait(windowId, -1);
    Story3SetWindowMessage(windowId, MAKE_MESSID(37, 4));
    Story3SetModelMotion(sharedSequenceProcessState.object, 1, 1, 15, 1);
}

Void Story3InsertCharacterNameMessage(s32 windowId, s32 messageId, s32 mode) {
    s32 characterId;

    characterId = resultSequenceState.playerResults[messageId].playerStats.characterId;
    HuWinInsertMesSet(windowId, characterId, mode);
}

Void Story3LoadPlayerResultModels(OMOBJ* object, s32 count) {
    s32 characterModelDataIds[] = {
        DATA_MAKE_NUM(DATADIR_MSTORY3, 40),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 46),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 52),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 58),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 64),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 70)
    };
    s32 characterMotionDataIds[][5] = {
        { DATA_MAKE_NUM(DATADIR_MSTORY3, 0x29), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x2A), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x2B), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x2C), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x2D) },
        { DATA_MAKE_NUM(DATADIR_MSTORY3, 0x2F), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x30), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x31), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x32), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x33) },
        { DATA_MAKE_NUM(DATADIR_MSTORY3, 0x35), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x36), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x37), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x38), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x39) },
        { DATA_MAKE_NUM(DATADIR_MSTORY3, 0x3B), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x3C), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x3D), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x3E), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x3F) },
        { DATA_MAKE_NUM(DATADIR_MSTORY3, 0x41), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x42), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x43), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x44), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x45) },
        { DATA_MAKE_NUM(DATADIR_MSTORY3, 0x47), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x48), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x49), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x4A), DATA_MAKE_NUM(DATADIR_MSTORY3, 0x4A) }
    };

    object->mdlId[1] = Hu3DModelCreateFile(characterModelDataIds[count]);
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], characterMotionDataIds[count][0]);
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], characterMotionDataIds[count][1]);
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], characterMotionDataIds[count][2]);
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], characterMotionDataIds[count][3]);
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], characterMotionDataIds[count][4]);
    Hu3DModelPosSet(object->mdlId[1], 0.0f, 0.0f, 670.0f);
    Hu3DModelRotSet(object->mdlId[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelScaleSet(object->mdlId[1], 1.0f, 1.0f, 1.0f);
    if (count == 3) {
        Hu3DData[object->mdlId[1]].pos.y = 50.0f;
    }
    Hu3DModelShadowSet(object->mdlId[1]);
    Story3SetModelMotion(object, 1, 1, 0, 1);
    {
        s32 characterDustEffectIds[] = { 9, 14, 10, 11, 12, 13 };

        if (count != 3) {
            CharNpcDustSet(object->mdlId[1], object->mtnId[2], 1, characterDustEffectIds[count]);
        }
    }
}

Void Story3CreateSharedSequenceObject(Void) {
    OMOBJ* createdResultObject;

    createdResultObject = omAddObjEx(resultObjectManager, 0x100, 0x10, 0x10, -1, NULL);
    sharedSequenceProcessState.object = createdResultObject;
    sharedSequenceProcessState.callback = NULL;
    sharedSequenceProcessState.object->data = &sharedSequenceProcessState;
    Story3LoadPlayerResultModels(createdResultObject, resultSequenceState.boardId);
    while (TRUE) {
        Story3SleepOneFrame();
        if (sharedSequenceProcessState.callback != NULL) {
            sharedSequenceProcessState.callback(createdResultObject);
        }
    }
}

Void Story3WaitForPlayerConfirmation(s32 playerIndex) {
    s32 remainingWaitFrames;

    remainingWaitFrames = -1;
    if (GWPlayerCfg[resultSequenceState.playerResults[playerIndex].playerStats.playerIndex].iscom) {
        remainingWaitFrames = rand8() % 120 + 60;
    }
    do {
        Story3SleepOneFrame();
        if (!GWPlayerCfg[resultSequenceState.playerResults[playerIndex].playerStats.playerIndex].iscom && (HuPadBtnDown[GWPlayerCfg[resultSequenceState.playerResults[playerIndex].playerStats.playerIndex].pad_idx] & PAD_BUTTON_A)) {
            break;
        }
    } while (remainingWaitFrames-- != 0);
    Story3SetModelMotion(playerSequenceProcessStates[playerIndex].object, 1, 5, 15, 0);
}

Void Story3WaitForPlayerMotionEnd(s32 playerIndex) {
    while (TRUE) {
        Story3SleepOneFrame();
        if (playerSequenceProcessStates[playerIndex].object->mtnId[5] != Hu3DMotionShiftIDGet(playerSequenceProcessStates[playerIndex].object->mdlId[1])
            && playerSequenceProcessStates[playerIndex].object->mtnId[5] == Hu3DMotionIDGet(playerSequenceProcessStates[playerIndex].object->mdlId[1])
            && Hu3DMotionTimeGet(playerSequenceProcessStates[playerIndex].object->mdlId[1]) >= 27.0f)
        {
            break;
        }
    }
    HuAudFXStop(resultSoundEffectIds[playerIndex + 1]);
    HuAudFXPlay(0x303);
}

HU3DPARMANPARAM resultParticleParams = {
    60, { 0, 0 },
    1000.0f, 0.0f, 180.0f,
    { 0.0f, 0.0f, 0.0f },
    50.0f, 0.9f, 20.0f, 0.99f, 4,
    {
        { 0xFF, 0xFF, 0x00, 0xFF },
        { 0xFF, 0xFF, 0x20, 0xFF },
        { 0xFF, 0xFF, 0x80, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF }
    },
    {
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 },
        { 0xFF, 0xFF, 0xFF, 0x00 }
    }
};

s32 Story3CreateScoreParticleEffect(Void) {
    s32 particleManagerId;

    particleManagerId = Hu3DParManCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_EFFECT, 1)), 250, &resultParticleParams);
    Hu3DParManTimeLimitSet(particleManagerId, 60);
    Hu3DParManAttrSet(particleManagerId, 4 | 0x40);
    Hu3DParticleBlendModeSet(Hu3DParManModelIDGet(particleManagerId), 1);
    return particleManagerId;
}

Void Story3DestroyResultObject(OMOBJ* object) {
    Hu3DModelKill(object->mdlId[0]);
    omDelObjEx(resultObjectManager, object);
}

Void Story3AnimatePlayerScoreDisplay(OMOBJ* object) {
    float modelFadeAlpha;
    s32 otherPlayerScores[3];
    s32 newMinigameScore;
    s32 playerScoreIndex;
    s32 currentMinigameScore;
    s32 previousMinigameScore;
    s32 playerIndex;

    switch (object->work[1]) {
        case 0:
            if (object->work[2] >= 1080) {
                object->work[1] = 1;
                object->rot.y = 0.0f;
                object->scale.x = object->scale.y = object->scale.z = 1.0f;
                object->work[2] = object->work[3] = 0;
                resultSoundEffectIds[object->work[0] + 1] = HuAudFXPlay(0x300);
            } else {
                object->rot.y = object->work[2] % 360;
                object->work[2] += 25;
                if (object->work[3] < 180) {
                    object->scale.x = sind(object->work[3] % 180);
                    object->work[3] += 6;
                } else {
                    object->scale.x = 1.0 + 0.5 * sind(object->work[3] % 180);
                    object->work[3] += 18;
                }
                object->scale.y = object->scale.z = object->scale.x;
                if (object->work[3] > 360) {
                    object->work[3] = 360;
                }
            }
            break;
        case 1:
            if (object->work[2] >= 40) {
                object->work[1] = 2;
            }
            /* fallthrough */
        case 2:
            object->trans.y += 0.2f * sind(object->work[2]);
            if ((object->work[2] += 4) > 360) {
                object->work[2] -= 360;
            }
            break;
        case 3:
            if (object->work[3]++ >= 8) {
                object->work[1] = 4;
                object->work[3] = 0;
                Hu3DModelAttrSet(object->mdlId[0], 1);
            } else {
                modelFadeAlpha = interpolateLinearOverTime(1.0f, 0.0f, object->work[3], 8.0f);
                if (object->work[3] == 1) {
                    object->work[2] = Story3CreateScoreParticleEffect();
                    Hu3DParManPosSet(object->work[2], object->trans.x, 275.0f, object->trans.z);
                }
                object->trans.y = 250.0f + interpolateLinearOverTime(0.0f, 25.0f, object->work[3], 4.0f);
                object->scale.x = 1.0f + 0.5f * (1.0f - modelFadeAlpha);
                object->scale.y = object->scale.z = object->scale.x;
                Hu3DModelTPLvlSet(object->mdlId[0], modelFadeAlpha);
            }
            break;
        case 4:
            if (object->work[3]++ >= 60) {
                Hu3DParManKill(object->work[2]);
                object->work[2] = -1;
                object->objFunc = NULL;
            }
            break;
    }
    if (object->work[1] >= 3) {
        playerScoreIndex = 0;
        if (playerSequenceScoreOrder[object->work[0]] == -1) {
            currentMinigameScore = playerSequenceResultIndices[object->work[0]];
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                if (playerIndex != object->work[0]) {
                    otherPlayerScores[playerScoreIndex++] = playerSequenceScoreOrder[playerIndex];
                }
            }
            while (TRUE) {
                if (currentMinigameScore != otherPlayerScores[0] && currentMinigameScore != otherPlayerScores[1] && currentMinigameScore != otherPlayerScores[2]) {
                    playerSequenceScoreOrder[object->work[0]] = currentMinigameScore;
                    break;
                }
                currentMinigameScore = rand8() % 10 + 1;
            }
            Hu3DMotionTimeSet(object->mdlId[0], playerSequenceScoreOrder[object->work[0]] - 1 + 0.5f);
        }
    } else {
        previousMinigameScore = playerSequenceResultIndices[object->work[0]] - 1;
        do {
            newMinigameScore = rand8() % 10;
        } while (previousMinigameScore == newMinigameScore);
        playerSequenceResultIndices[object->work[0]] = newMinigameScore + 1;
        Hu3DMotionTimeSet(object->mdlId[0], newMinigameScore + 0.5f);
    }
}

Void Story3CreatePlayerMedalObject(OMOBJ* object) {
    s32 playerIndex;

    playerIndex = object->work[0];
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 19));
    omSetTra(object, playerModelPositions[playerIndex].x, 250.0f, playerModelPositions[playerIndex].z);
    omSetRot(object, 0.0f, 0.0f, 0.0f);
    omSetSca(object, 0.0f, 0.0f, 0.0f);
    Hu3DMotionSpeedSet(object->mdlId[0], 0.0f);
    HuAudFXPlay(0x302);
    object->objFunc = Story3AnimatePlayerScoreDisplay;
}

Void Story3DestroyMedalObject(OMOBJ* object) {
    Hu3DModelKill(object->mdlId[1]);
    Hu3DModelKill(object->mdlId[2]);
    omDelObjEx(resultObjectManager, object);
}

Void Story3SetupPlayerMedalModels(OMOBJ* object) {
    s32 playerIndex;
    s32 modelIndex;

    playerIndex = object->work[0];
    for (modelIndex = 1; modelIndex < 3; modelIndex++) {
        if (object->work[3] <= 40) {
            Hu3DData[object->mdlId[modelIndex]].rot.y = interpolateSineEaseIn(0.0f, 360.0f, object->work[3], 40.0f);
        } else {
            Hu3DData[object->mdlId[modelIndex]].rot.y = interpolateCosineEaseOut(360.0f, 0.0f, object->work[3] - 40, 20.0f);
        }
        if (object->work[1] / 10 == 0) {
            Hu3DData[object->mdlId[modelIndex]].pos.x = playerModelPositions[playerIndex].x;
        } else {
            Hu3DData[object->mdlId[modelIndex]].pos.x = playerModelPositions[playerIndex].x + interpolateLinearOverTime(0.0f, 40.0f + -80.0f * (modelIndex - 1), object->work[3], 20.0f);
        }
        Hu3DData[object->mdlId[modelIndex]].pos.y = 275.0f + interpolateLinearOverTime(0.0f, 50.0f, object->work[3], 20.0f);
        Hu3DData[object->mdlId[modelIndex]].pos.z = playerModelPositions[playerIndex].z;
    }
    if (object->work[3]++ >= 60) {
        object->objFunc = NULL;
    }
}

Void Story3CreateTeamMedalModels(OMOBJ* object) {
    s32 teamMedalModelDataIds[] = {
        DATA_MAKE_NUM(DATADIR_MSTORY3, 20),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 21),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 22),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 23),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 24),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 25),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 26),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 27),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 28),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 29)
    };

    object->mdlId[1] = Hu3DModelCreateFile(teamMedalModelDataIds[object->work[1] % 10]);
    object->mdlId[2] = Hu3DModelCreateFile(teamMedalModelDataIds[object->work[1] / 10]);
    Hu3DMotionSpeedSet(object->mdlId[1], 0.0f);
    Hu3DMotionSpeedSet(object->mdlId[2], 0.0f);
    if (object->work[1] / 10 == 0) {
        Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    }
    object->objFunc = Story3SetupPlayerMedalModels;
}

Void Story3StartPlayerPresentationProcess(s32 playerIndex) {
    OMOBJ* createdResultObject;
    OMOBJ* teamMedalObject;

    createdResultObject = omAddObjEx(resultObjectManager, 0x2000, 0x10, 0x10, -1, Story3CreatePlayerMedalObject);
    createdResultObject->work[0] = playerIndex;
    do {
        Story3SleepOneFrame();
    } while (createdResultObject->work[1] != 2);
    Story3WaitForPlayerConfirmation(playerIndex);
    Story3WaitForPlayerMotionEnd(playerIndex);
    createdResultObject->work[1] = 3;
    Story3SleepOneFrame();
    teamMedalObject = omAddObjEx(resultObjectManager, 0x2000, 0x10, 0x10, -1, Story3CreateTeamMedalModels);
    teamMedalObject->work[0] = playerIndex;
    teamMedalObject->work[1] = playerSequenceScoreOrder[playerIndex];
    Story3SleepFrames(10);
    Story3SetModelMotion(playerSequenceProcessStates[playerIndex].object, 1, 1, 15, 1);
    do {
        Story3SleepOneFrame();
    } while (teamMedalObject->objFunc);
    playerSequenceProcessStates[playerIndex].callbackState = 1;
    do {
        Story3SleepOneFrame();
    } while (playerSequenceProcessStates[playerIndex].callbackState != 2);
    Story3SleepFrames(150);
    Story3DestroyResultObject(createdResultObject);
    Story3DestroyMedalObject(teamMedalObject);
}

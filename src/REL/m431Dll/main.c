#include "game/hu3d.h"
#include "game/msm.h"

#include "game/audio.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/wipe.h"

#include "game/gamework_data.h"
#include "game/minigame_seq.h"
#include "game/sprite.h"

#include "ext_math.h"

#include "REL/m431Dll.h"
#include "version.h"
#include <string.h>

s32 rand8(void);

typedef struct StorySelectionState {
    s16 spriteModelIds[6];
    s16 randomizedPlayerOrder[2];
    s16 motionMode;
    s16 motionTransitionState;
    s16 stageMotionRepeatCount;
    s16 selectedStageMotionVariant;
    float cameraCenterX;
    float unusedState1C;
    float unusedState20;
    float unusedState24;
    float cameraYawOffsets[2];
    float unusedState30;
    float unusedState34;
} StorySelectionState;

typedef struct CameraViewParams {
    float zoom;
    Vec pos;
    Vec rot;
} CameraViewParams;

StorySelectionState storySelectionState;
s16 itemModelIds[20];
OMOBJ *storyEventObject;
OMOBJ *storyItemsObject;
s16 globalLightId;
float playerTargetAngles[2];
float playerStartAngles[2];
s16 spinDuration;
s8 coinAwardSide;
s16 animationTimer;
s8 wipeStarted;
s16 resultTimer;
s16 sequenceStep;
s16 cameraTransitionTimer;
s32 playerWon;
s8 exitRequested;

Vec globalLightPosition = { 800, 1300, 1000 };
Vec globalLightDirection = { 0, 0, 0 };
GXColor globalLightColor = { 255, 255, 255, 255 };
Vec unusedvector1C = { 10, 45, 0 };
Vec shadowPosition = { 1300, 2500, 1300 };
Vec shadowDirection = { 0, 1, 0 };
Vec shadowTarget = { 0, 0, -1200 };

CameraViewParams cameraViewPresets[4] = { { 760, { -324, -97, 0 }, { -43, 0, 0 } }, { 800, { 325, -64, 0 }, { -43, -20, 0 } },
    { 1460, { 0, 57, 0 }, { -37, 0, 0 } }, { 880, { -381, 25, 0 }, { -14, 10, 0 } } };

s32 itemSpriteOffsets[3] = { -32, -2, 28 };

s32 globalLightDirection8[2] = { 90, 486 };

void CheckExitRequest(OMOBJ *object);
void InitializeStorySelectionObjects(OMOBJ *object);
void InitializeStoryItemDisplay(OMOBJ *object);

void ObjectSetup(void)
{
    Vec lightAimTarget;
    Vec cameraLookAt;
    Vec cameraPosition;
    Vec cameraDirection;
    HUPROCESS *objectManager;
    HU3DLIGHT *globalLight;
    HU3DCAMERA *camera;

    Hu3DLightAllKill();
    globalLightId = Hu3DGLightCreateV(&globalLightPosition, &globalLightDirection, &globalLightColor);
    Hu3DGLightInfinitytSet(globalLightId);
    globalLight = &Hu3DGlobalLight[globalLightId];
    globalLight->type |= 0x8000;
    lightAimTarget.x = lightAimTarget.y = lightAimTarget.z = 0;
    Hu3DGLightPosAimSetV(globalLightId, &globalLightPosition, &lightAimTarget);
    Hu3DShadowCreate(45, 20, 10000);
    Hu3DShadowTPLvlSet(0.425f);
    Hu3DShadowPosSet(&shadowPosition, &shadowDirection, &shadowTarget);
    objectManager = omInitObjMan(50, 8192);
    omGameSysInit(objectManager);
    HuAudSndGrpSet(56);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 41.5, 5, 5000, 1.2f);
    omAddObjEx(objectManager, 32730, 0, 0, -1, omOutView);
    camera = &Hu3DCamera[0];
    camera->fov = -1;
    cameraPosition.x = 0;
    cameraPosition.y = 374;
    cameraPosition.z = 1086;
    cameraLookAt.x = 0;
    cameraLookAt.y = 568;
    cameraLookAt.z = -1316;
    cameraDirection.z = cameraLookAt.x - cameraPosition.x;
    cameraDirection.y = cameraLookAt.y - cameraPosition.y;
    cameraDirection.x = cameraLookAt.z - cameraPosition.z;
    CRot.x = cameraViewPresets[0].rot.x;
    CRot.y = cameraViewPresets[0].rot.y;
    CRot.z = cameraViewPresets[0].rot.z;
    Center.x = cameraViewPresets[0].pos.x;
    Center.y = cameraViewPresets[0].pos.y;
    Center.z = cameraViewPresets[0].pos.z;
    CZoom = cameraViewPresets[0].zoom;
    omAddObjEx(objectManager, 1000, 0, 0, -1, CheckExitRequest);
    storyEventObject = omAddObjEx(objectManager, 10, 9, 0, -1, InitializeStorySelectionObjects);
    storyItemsObject = omAddObjEx(objectManager, 50, 9, 9, -1, InitializeStoryItemDisplay);
    Hu3DBGColorSet(0, 0, 0);
    InitializePlayerGroups(objectManager);
}

void CleanupStorySelection(OMOBJ *object);

void CheckExitRequest(OMOBJ *object)
{
    if (omSysExitReq || exitRequested) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        HuAudSeqAllFadeOut(100);
        object->objFunc = CleanupStorySelection;
    }
}

void DestroyParticleModel(s16 modelId);

void CleanupStorySelection(OMOBJ *object)
{
    s16 i;
    if (WipeStatGet()) {
        return;
    }
    DestroyParticleModel(storyEventObject->mdlId[0]);
    for (i = 0; i < 6; i++) {
        DestroyParticleModel(storySelectionState.spriteModelIds[i]);
    }
    HuSprGrpKill(storyEventObject->mdlId[1]);
    HuSprGrpKill(storyEventObject->mdlId[2]);
    HuSprGrpKill(storyEventObject->mdlId[3]);
    MGSeqKillAll();
    HuAudFadeOut(1);
    omOvlReturnEx(1, 1);
}

float InterpolateValue(float startValue, float endValue, float progress);

u8 TransitionToStorySelectionCamera(void)
{
    float easedProgress;
    float transitionProgress;
    CameraViewParams *cameraViews;
    s32 transitionComplete;
    cameraViews = &cameraViewPresets[0];
    transitionComplete = 0;
    cameraTransitionTimer++;
    transitionProgress = cameraTransitionTimer / (1.5f * REFRESH_RATE);
    if (cameraTransitionTimer > (s32)(1.5f * REFRESH_RATE)) {
        cameraTransitionTimer = 0;
        transitionProgress = 1;
        transitionComplete = 1;
    }
    easedProgress = sind(90 * transitionProgress) * sind(90 * transitionProgress);
    CZoom = InterpolateValue(cameraViews[0].zoom, cameraViews[2].zoom, easedProgress);
    Center.x = InterpolateValue(cameraViews[0].pos.x, cameraViews[2].pos.x, easedProgress);
    Center.y = InterpolateValue(cameraViews[0].pos.y, cameraViews[2].pos.y, easedProgress);
    Center.z = InterpolateValue(cameraViews[0].pos.z, cameraViews[2].pos.z, easedProgress);
    CRot.x = InterpolateValue(cameraViews[0].rot.x, cameraViews[2].rot.x, easedProgress);
    CRot.y = InterpolateValue(cameraViews[0].rot.y, cameraViews[2].rot.y, easedProgress);
    CRot.z = InterpolateValue(cameraViews[0].rot.z, cameraViews[2].rot.z, easedProgress);
    return transitionComplete;
}

typedef struct StoryEventState {
    u16 statusFlags;
    s16 sequenceId;
    s16 selectedWinner;
    s16 eventAndSpriteState[2];
    s16 eventTimer;
    s32 sequenceAudioId;
    s16 sequenceTimerId;
    s16 spritePulseFrame;
    s16 unusedState14;
    s16 spriteMemberIndex;
} StoryEventState;

s32 TransitionToWinnerCamera(void)
{
    float easedProgress;
    float transitionProgress;
    float mirroredCameraX;
    float mirroredCameraYaw;
    CameraViewParams *cameraViews;
    StoryEventState *transitionState;
    s32 transitionComplete;
    transitionState = storyEventObject->data;
    cameraViews = &cameraViewPresets[0];
    transitionComplete = 0;
    cameraTransitionTimer++;
    transitionProgress = cameraTransitionTimer / (2.0f * REFRESH_RATE);
    if (cameraTransitionTimer > (2 * REFRESH_RATE)) {
        cameraTransitionTimer = 2 * REFRESH_RATE;
        transitionProgress = 1;
        transitionComplete = 1;
    }
    easedProgress = sind(90 * transitionProgress) * sind(90 * transitionProgress);
    if (transitionState->selectedWinner == 0) {
        mirroredCameraX = cameraViews[3].pos.x;
        mirroredCameraYaw = cameraViews[3].rot.y;
    }
    else {
        mirroredCameraX = -cameraViews[3].pos.x;
        mirroredCameraYaw = -cameraViews[3].rot.y;
    }
    CZoom = InterpolateValue(cameraViews[2].zoom, cameraViews[3].zoom, easedProgress);
    Center.x = InterpolateValue(cameraViews[2].pos.x, mirroredCameraX, easedProgress);
    Center.y = InterpolateValue(cameraViews[2].pos.y, cameraViews[3].pos.y, easedProgress);
    Center.z = InterpolateValue(cameraViews[2].pos.z, cameraViews[3].pos.z, easedProgress);
    CRot.x = InterpolateValue(cameraViews[2].rot.x, cameraViews[3].rot.x, easedProgress);
    CRot.y = InterpolateValue(cameraViews[2].rot.y, mirroredCameraYaw, easedProgress);
    CRot.z = InterpolateValue(cameraViews[2].rot.z, cameraViews[3].rot.z, easedProgress);
    return transitionComplete;
}

void InitializeParticleModel(HU3DMODEL *model, ParticleSystemState *particleSystem, Mtx modelMatrix);
s16 CreateParticleModel(ANIMDATA *animation, s16 particleCount, float particleSize, s16 tileWidth, s16 tileHeight);
void SetParticleModelCallback(s16 modelId, ParticleSystemCallback callback);

void UpdateStorySelectionSequence(OMOBJ *object);

void InitializeStorySelectionObjects(OMOBJ *object)
{
    float spriteCenterX;

    s16 spriteIndex;
    s16 spriteGroupId;
    s16 spriteId;

    ANIMDATA *spriteAnimation;
    StoryEventState *modelAnimation;
    s16 modelId;

    ANIMDATA *alternateAnimation;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(StoryEventState), HU_MEMNUM_OVL);
    modelAnimation = object->data;
    modelAnimation->statusFlags = 4;
    modelAnimation->eventAndSpriteState[0] = 0;
    modelAnimation->eventTimer = 15 * REFRESH_RATE;
    modelAnimation->sequenceTimerId = -1;
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M431, 0x1A), HU_MEMNUM_OVL));
    object->mdlId[0] = CreateParticleModel(spriteAnimation, 1024, 25, 64, 64);
    SetParticleModelCallback(object->mdlId[0], InitializeParticleModel);
    Hu3DModelLayerSet(object->mdlId[0], 7);
    spriteGroupId = HuSprGrpCreate(4);
    object->mdlId[3] = spriteGroupId;
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 0x1B), HU_MEMNUM_OVL));
    spriteId = HuSprCreate(spriteAnimation, 16, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    HuSprAttrSet(spriteGroupId, 0, HUSPR_ATTR_DISPOFF);
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 0x1D), HU_MEMNUM_OVL));
    spriteId = HuSprCreate(spriteAnimation, 32, 0);
    HuSprGrpMemberSet(spriteGroupId, 2, spriteId);
    HuSprAttrSet(spriteGroupId, 2, HUSPR_ATTR_DISPOFF);
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 0x1C), HU_MEMNUM_OVL));
    spriteId = HuSprCreate(spriteAnimation, 16, 0);
    HuSprGrpMemberSet(spriteGroupId, 1, spriteId);
    HuSprAttrSet(spriteGroupId, 1, HUSPR_ATTR_DISPOFF);
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 0x1D), HU_MEMNUM_OVL));
    spriteId = HuSprCreate(spriteAnimation, 32, 0);
    HuSprGrpMemberSet(spriteGroupId, 3, spriteId);
    HuSprAttrSet(spriteGroupId, 3, HUSPR_ATTR_DISPOFF);
    spriteCenterX = 288;
    HuSprPosSet(spriteGroupId, 0, spriteCenterX - 198, 64);
    HuSprPosSet(spriteGroupId, 1, spriteCenterX + 198, 64);
    HuSprPosSet(spriteGroupId, 2, spriteCenterX - 198, 64);
    HuSprTPLvlSet(spriteGroupId, 2, 88);
    HuSprColorSet(spriteGroupId, 2, 0, 0, 0);
    HuSprPosSet(spriteGroupId, 3, spriteCenterX + 198, 64);
    HuSprTPLvlSet(spriteGroupId, 3, 88);
    HuSprColorSet(spriteGroupId, 3, 0, 0, 0);
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 0x0B), HU_MEMNUM_OVL));
    alternateAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGCONST, 0x0A), HU_MEMNUM_OVL));
    spriteGroupId = HuSprGrpCreate(6);
    object->mdlId[1] = spriteGroupId;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        spriteId = HuSprCreate(spriteAnimation, 0, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex, spriteId);
        HuSprPosSet(spriteGroupId, spriteIndex, globalLightDirection8[0] + itemSpriteOffsets[spriteIndex], 63);
        HuSprAttrSet(spriteGroupId, spriteIndex, HUSPR_ATTR_DISPOFF);
    }
    for (spriteIndex = 3; spriteIndex < 6; spriteIndex++) {
        modelId = spriteIndex - 3;
        spriteId = HuSprCreate(alternateAnimation, 8, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex, spriteId);
        HuSprPosSet(spriteGroupId, spriteIndex, globalLightDirection8[0] + itemSpriteOffsets[modelId], 63);
        HuSprAttrSet(spriteGroupId, spriteIndex, HUSPR_ATTR_DISPOFF);
    }
    spriteGroupId = HuSprGrpCreate(6);
    object->mdlId[2] = spriteGroupId;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        spriteId = HuSprCreate(spriteAnimation, 0, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex, spriteId);
        HuSprPosSet(spriteGroupId, spriteIndex, globalLightDirection8[1] + itemSpriteOffsets[spriteIndex], 63);
        HuSprAttrSet(spriteGroupId, spriteIndex, HUSPR_ATTR_DISPOFF);
    }
    for (spriteIndex = 3; spriteIndex < 6; spriteIndex++) {
        modelId = spriteIndex - 3;
        spriteId = HuSprCreate(alternateAnimation, 8, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex, spriteId);
        HuSprPosSet(spriteGroupId, spriteIndex, globalLightDirection8[1] + itemSpriteOffsets[modelId], 63);
        HuSprAttrSet(spriteGroupId, spriteIndex, HUSPR_ATTR_DISPOFF);
    }
    (void)spriteIndex;
    (void)spriteIndex;
    object->objFunc = UpdateStorySelectionSequence;
}

void UpdateWinnerSpritePulse(OMOBJ *object);
u8 UpdateStorySelectionIntro(OMOBJ *object);
u8 UpdateWinnerAnnouncement(OMOBJ *object);

void HideStorySelectionParticles(void);

void UpdateStorySelectionSequence(OMOBJ *object)
{
    StoryEventState *eventState;
    s16 winnerPlayerIndex;
    s16 remainingSeconds;
    HU3DCAMERA *gameCamera;
    eventState = object->data;
    winnerPlayerIndex = 99;
    switch (GetSequenceStatusFlags(28)) {
        case 4:
            if (wipeStarted == 0) {
                wipeStarted = 1;
                WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
                gameCamera = &Hu3DCamera[0];
                gameCamera->fov = 41.5f;
            }
            if (!WipeStatGet()) {
                UpdateSequenceStatusFlags(28, 8);
                UpdateSequenceStatusFlags(224, 32);
            }
            break;

        case 8:
            if (UpdateStorySelectionIntro(object)) {
                UpdateSequenceStatusFlags(1, 0);
                UpdateSequenceStatusFlags(28, 12);
            }
            break;

        case 12:
            if (eventState->sequenceTimerId != -1) {
                remainingSeconds = (eventState->eventTimer + REFRESH_RATE - 1) / REFRESH_RATE;
                if (remainingSeconds < 0) {
                    remainingSeconds = 0;
                }
                MGSeqParamSet(eventState->sequenceTimerId, 1, remainingSeconds);
            }
            UpdateWinnerSpritePulse(object);
            if (GetSequenceStatusFlags(224) == 128) {
                if (playerGroups[0].completedRoundCounts[0] == 3) {
                    winnerPlayerIndex = 0;
                }
                else if (playerGroups[1].completedRoundCounts[0] == 3) {
                    winnerPlayerIndex = 1;
                }
                else {
                    if (eventState->eventAndSpriteState[0] == 5) {
                        if (playerGroups[0].completedRoundCounts[0] == playerGroups[1].completedRoundCounts[0]) {
                            winnerPlayerIndex = -1;
                        }
                        else {
                            if (playerGroups[0].completedRoundCounts[0] > playerGroups[1].completedRoundCounts[0]) {
                                winnerPlayerIndex = 0;
                            }
                            else {
                                winnerPlayerIndex = 1;
                            }
                        }
                    }
                }
            }
            if (winnerPlayerIndex != 99) {
                if (GetSequenceStatusFlags(1) == 0) {
                    eventState->sequenceId = MGSeqFinishCreate();
                    HuAudSeqAllFadeOut(100);
                }
                HideStorySelectionParticles();
                SetSelectedWinner(winnerPlayerIndex);
                UpdateSequenceStatusFlags(1, 1);
                if (eventState->statusFlags & 0x2) {
                    if (winnerPlayerIndex == -1) {
                        sequenceStep = 1;
                    }
                    UpdateSequenceStatusFlags(28, 16);
                }
                if (winnerPlayerIndex != -1) {
                    CheckPlayerGroupReady(winnerPlayerIndex ^ 1);
                }
                else {
                    CheckPlayerGroupReady(0);
                    CheckPlayerGroupReady(1);
                }
            }
            break;

        case 16:
            if (UpdateWinnerAnnouncement(object)) {
                UpdateSequenceStatusFlags(28, 20);
            }
            break;

        case 20:
            if (++resultTimer > (3.5f * REFRESH_RATE)) {
                exitRequested = 1;
            }
            break;
    }
}

void UpdateWinnerSpritePulse(OMOBJ *object)
{
    float pulseProgress;
    float spriteOpacity;
    StoryEventState *eventState = object->data;
    if ((eventState->statusFlags & 0x300) == 0) {
        return;
    }
    eventState->spritePulseFrame++;
    pulseProgress = eventState->spritePulseFrame / (3.0f / 10 * REFRESH_RATE);
    if (eventState->spritePulseFrame > (3.0f / 10 * REFRESH_RATE)) {
        eventState->spritePulseFrame = 0;
        pulseProgress = 1.0f;
        eventState->spritePulseFrame = 0.3f * REFRESH_RATE;
        eventState->statusFlags &= ~0x300;
    }
    spriteOpacity = 1.0 - cosd(90.0f * pulseProgress);

    pulseProgress = 1.0 + sind(180.0f * pulseProgress);
    HuSprAttrReset(storyEventObject->mdlId[eventState->eventAndSpriteState[1] + 1], eventState->spriteMemberIndex, HUSPR_ATTR_DISPOFF);
    HuSprScaleSet(storyEventObject->mdlId[eventState->eventAndSpriteState[1] + 1], eventState->spriteMemberIndex, pulseProgress, pulseProgress);
    HuSprTPLvlSet(storyEventObject->mdlId[eventState->eventAndSpriteState[1] + 1], eventState->spriteMemberIndex, spriteOpacity);
}

void SetStoryAnimationTimer(s16 timer);

u8 UpdateStorySelectionIntro(OMOBJ *object)
{
    StoryEventState *eventState = object->data;
    s32 spriteIndex;
    s32 spriteGroupId;
    s32 sequenceStatus;
    switch (sequenceStep) {
        case 0:
            if (GetSequenceStatusFlags(224) == 64) {
                UpdateSequenceStatusFlags(1, 1);
                sequenceStep++;
            }
            break;

        case 1:
            if (TransitionToStorySelectionCamera()) {
                sequenceStep++;
            }
            break;

        case 2:
            eventState->sequenceId = MGSeqStartCreate();
            eventState->sequenceAudioId = -1;
            sequenceStep++;
            break;

        case 3:
            if (eventState->sequenceId != -1) {
                sequenceStatus = MGSeqStatGet(eventState->sequenceId);
                if ((sequenceStatus & 0x10) && eventState->sequenceAudioId == -1) {
                    eventState->sequenceAudioId = HuAudSeqPlay(72);
                }
            }
            if (eventState->sequenceId == -1 || sequenceStatus == 0) {
                spriteGroupId = object->mdlId[1];
                for (spriteIndex = 3; spriteIndex < 6; spriteIndex++) {
                    HuSprAttrReset(spriteGroupId, spriteIndex, HUSPR_ATTR_DISPOFF);
                }
                spriteGroupId = object->mdlId[2];
                for (spriteIndex = 3; spriteIndex < 6; spriteIndex++) {
                    HuSprAttrReset(spriteGroupId, spriteIndex, HUSPR_ATTR_DISPOFF);
                }
                spriteGroupId = object->mdlId[3];
                for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
                    HuSprAttrReset(spriteGroupId, spriteIndex, HUSPR_ATTR_DISPOFF);
                }
                SetStoryAnimationTimer(999);
                sequenceStep = 0;

                return 1;
            }
            break;
    }
    return 0;
}

u8 UpdateWinnerAnnouncement(OMOBJ *object)
{
    StoryEventState *eventState;
    PlayerGroupState *playerPair;
    s32 winnerReady;
    s32 opponentReady;
    eventState = object->data;
    switch (sequenceStep) {
        case 0:
            winnerReady = movePlayerPairToTarget(eventState->selectedWinner);
            CheckPlayerGroupReady(eventState->selectedWinner ^ 1);
            winnerReady &= TransitionToWinnerCamera();
            if ((u8)winnerReady) {
                playerWon = 1;
                StartPlayerGroupVictory(eventState->selectedWinner);
                HuAudSStreamPlay(1);
                resultTimer = 0;
                sequenceStep++;
            }
            break;

        case 1:
            if (eventState->selectedWinner == -1) {
                winnerReady = CheckPlayerGroupReady(0);
                opponentReady = CheckPlayerGroupReady(1);
                if ((u8)winnerReady == 0 || (u8)opponentReady == 0) {
                    return 0;
                }
                if (eventState->sequenceId == -1 || !MGSeqStatGet(eventState->sequenceId)) {
                    eventState->sequenceId = -1;
                    playerWon = 1;
                    StartPlayerGroupDraw(0);
                    StartPlayerGroupDraw(1);
                    HuAudSStreamPlay(4);
                    sequenceStep++;
                }
            }
            else {
                if (++resultTimer > 1) {
                    sequenceStep++;
                }
            }
            break;

        case 2:
            if (eventState->selectedWinner != -1) {
                playerPair = &playerGroups[eventState->selectedWinner];
                eventState->sequenceId = MGSeqWinCreate(GWPlayerCfg[playerPair->playerIds[0]].character, GWPlayerCfg[playerPair->playerIds[1]].character, -1, -1);
                GWPlayerCoinWinAdd(playerPair->playerIds[0], 10);
                GWPlayerCoinWinAdd(playerPair->playerIds[1], 10);
            }
            else {
                eventState->sequenceId = MGSeqDrawCreate();
            }
            sequenceStep++;
            break;

        case 3:
            sequenceStep = resultTimer = 0;
            return 1;

        default:
            break;
    }
    return 0;
}

void SetSelectedWinner(s16 playerIndex)
{
    StoryEventState *eventState = storyEventObject->data;
    if (playerIndex == -1 || eventState->eventAndSpriteState[1] == -1) {
        eventState->statusFlags |= 0x2;
    }
    eventState->selectedWinner = playerIndex;
}

void UpdateSequenceStatusFlags(u16 clearMask, u16 setMask)
{
    StoryEventState *eventState = storyEventObject->data;
    eventState->statusFlags &= ~clearMask;
    eventState->statusFlags |= setMask;
}

u16 GetSequenceStatusFlags(u16 statusMask)
{
    StoryEventState *eventState = storyEventObject->data;
    if (!eventState) {
        return 0;
    }
    return eventState->statusFlags & statusMask;
}

void StartWinnerSpritePulse(s16 spriteMemberIndex)
{
    StoryEventState *eventState = storyEventObject->data;
    eventState->statusFlags |= 0x100;
    eventState->spritePulseFrame = 0;
    eventState->spriteMemberIndex = spriteMemberIndex;
}

s32 playerIdleMotions[2][2] = {
    DATA_MAKE_NUM(DATADIR_M431, 0x01),
    DATA_MAKE_NUM(DATADIR_M431, 0x03),
    DATA_MAKE_NUM(DATADIR_M431, 0x05),
    DATA_MAKE_NUM(DATADIR_M431, 0x06),
};

s32 playerVictoryMotions[2][2] = {
    DATA_MAKE_NUM(DATADIR_M431, 0x07),
    DATA_MAKE_NUM(DATADIR_M431, 0x08),
    DATA_MAKE_NUM(DATADIR_M431, 0x09),
    DATA_MAKE_NUM(DATADIR_M431, 0x0A),
};

s32 storyStageMotions[5] = {
    DATA_MAKE_NUM(DATADIR_M431, 0x0C),
    DATA_MAKE_NUM(DATADIR_M431, 0x0D),
    DATA_MAKE_NUM(DATADIR_M431, 0x0E),
    DATA_MAKE_NUM(DATADIR_M431, 0x0F),
    DATA_MAKE_NUM(DATADIR_M431, 0x10),
};

s32 storyResultMotions[5] = {
    DATA_MAKE_NUM(DATADIR_M431, 0x1C),
    DATA_MAKE_NUM(DATADIR_M431, 0x1D),
    DATA_MAKE_NUM(DATADIR_M431, 0x1E),
    DATA_MAKE_NUM(DATADIR_M431, 0x1F),
    DATA_MAKE_NUM(DATADIR_M431, 0x20),
};

typedef struct StoryItemState {
    u32 stateFlags;
    float spinAngle;
    float heading;
    Vec position;
    s16 animationFrame;
    s16 primaryModelId;
    s16 itemModelId;
    s16 overlayModelId;
    float verticalVelocity;
} StoryItemState;

typedef struct StoryItemManagerState {
    float playerHeading;
    StoryItemState itemStates[10];
    s16 itemOrder[10];
} StoryItemManagerState;

void UpdateStoryItemSequence(OMOBJ *object);

void SortStoryItemsByAngle(StoryItemManagerState *playerState);
void UpdateStoryItemPositions(s16 playerSide, StoryItemManagerState *playerState);

void InitializeStoryItemDisplay(OMOBJ *object)
{
    float modelY;
    float modelX;
    float modelZ;
    s16 modelIndex;
    StoryItemState *itemState;
    StoryItemManagerState *playerState;
    s16 itemIndex;
    ANIMDATA *spriteAnimation;
    s16 modelId;

    object->stat |= 0x100;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, 2 * sizeof(StoryItemManagerState), HU_MEMNUM_OVL);
    playerState = object->data;
    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M431, 0x00));
    Hu3DModelLayerSet(object->mdlId[0], 0);
    Hu3DModelScaleSet(object->mdlId[0], 1.2f, 1.2f, 1.2f);
    modelIndex = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M431, 0x11));
    Hu3DModelAttrSet(modelIndex, HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(modelIndex, 0);
    Hu3DMotionSpeedSet(modelIndex, 0.25f);
    modelIndex = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M431, 0x12));
    Hu3DModelAttrSet(modelIndex, HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(modelIndex, 0);
    Hu3DMotionSpeedSet(modelIndex, 0.25f);
    modelIndex = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M431, 0x13));
    object->mdlId[5] = modelIndex;
    object->mtnId[0] = Hu3DJointMotionFile(modelIndex, DATA_MAKE_NUM(DATADIR_M431, 0x14));
    object->mtnId[1] = Hu3DJointMotionFile(modelIndex, DATA_MAKE_NUM(DATADIR_M431, 0x15));
    object->mtnId[2] = Hu3DJointMotionFile(modelIndex, DATA_MAKE_NUM(DATADIR_M431, 0x16));
    object->mtnId[3] = Hu3DJointMotionFile(modelIndex, DATA_MAKE_NUM(DATADIR_M431, 0x17));
    object->mtnId[4] = Hu3DJointMotionFile(modelIndex, DATA_MAKE_NUM(DATADIR_M431, 0x14));
    object->mtnId[5] = Hu3DJointMotionFile(modelIndex, DATA_MAKE_NUM(DATADIR_M431, 0x18));
    object->mtnId[6] = Hu3DJointMotionFile(modelIndex, DATA_MAKE_NUM(DATADIR_M431, 0x19));
    Hu3DModelPosSet(modelIndex, 0, 0, -230);
    Hu3DModelShadowSet(modelIndex);
    Hu3DModelAttrSet(modelIndex, HU3D_MOTATTR_LOOP);
    Hu3DMotionSet(modelIndex, object->mtnId[0]);
    for (modelIndex = 0; modelIndex < 2; modelIndex++, playerState++) {
        modelId = Hu3DModelCreateFile(playerIdleMotions[0][modelIndex]);
        modelX = (modelIndex == 0) ? -317.0f : 317.0f;
        modelY = 0;
        modelZ = 100;
        Hu3DModelPosSet(modelId, modelX, modelY, modelZ);
        object->mdlId[modelIndex + 1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M431, 0x02));
        modelX = (modelIndex == 0) ? -317.0f : 317.0f;
        modelY = 0;
        modelZ = 100;
        Hu3DModelPosSet(object->mdlId[modelIndex + 1], modelX, modelY, modelZ);
        Hu3DModelAttrSet(object->mdlId[modelIndex + 1], HU3D_MOTATTR_PAUSE);

        object->mdlId[modelIndex + 3] = Hu3DModelCreateFile(playerIdleMotions[1][modelIndex]);
        modelY = 45;
        Hu3DModelPosSet(object->mdlId[modelIndex + 3], modelX, modelY, modelZ);
        Hu3DModelRotSet(object->mdlId[modelIndex + 3], 0, playerItemSpinOffset, 0);
        for (itemIndex = 0, itemState = &playerState->itemStates[0]; itemIndex < 10; itemIndex++, itemState++) {
            if (fmod((float)itemIndex, 2)) {
                if (itemIndex == 1) {
                    itemState->overlayModelId = Hu3DModelCreateFile(playerVictoryMotions[modelIndex][0]);
                }
                else {
                    itemState->overlayModelId = Hu3DModelLink(playerState->itemStates[1].overlayModelId);
                }
            }
            else {
                if (itemIndex == 0) {
                    itemState->overlayModelId = Hu3DModelCreateFile(playerVictoryMotions[modelIndex][1]);
                }
                else {
                    itemState->overlayModelId = Hu3DModelLink(playerState->itemStates[0].overlayModelId);
                }
            }
            Hu3DModelPosSet(itemState->overlayModelId, 0, -500, 0);
            if (itemIndex == 0) {
                itemState->itemModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M431, 0x0B));
            }
            else {
                itemState->itemModelId = Hu3DModelLink(playerState->itemStates[0].itemModelId);
            }
            Hu3DData[itemState->itemModelId].pos.y = 1000;
            Hu3DModelAttrSet(itemState->itemModelId, HU3D_ATTR_DISPOFF);
            itemState->stateFlags = 0;
            itemState->spinAngle = playerItemSpinOffset + (36.0f * itemIndex);
        }
        playerState->stateFlags = 0;
        SortStoryItemsByAngle(playerState);
        UpdateStoryItemPositions(modelIndex, playerState);
    }
    for (modelIndex = 0; modelIndex < 20; modelIndex++) {
        if (modelIndex < 5) {
            itemModelIds[modelIndex] = Hu3DModelCreateFile(storyStageMotions[modelIndex]);
        }
        else {
            itemIndex = fmod((float)modelIndex, 5);
            itemModelIds[modelIndex] = Hu3DModelLink(itemModelIds[itemIndex]);
        }
        Hu3DModelPosSet(itemModelIds[modelIndex], 0, -500, 0);
    }
    Hu3DModelShadowMapObjSet(object->mdlId[0], "field");
    Hu3DModelShadowMapSet(object->mdlId[1]);
    Hu3DModelShadowMapSet(object->mdlId[2]);
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M431, 0x1B), HU_MEMNUM_OVL));
    storySelectionState.spriteModelIds[0] = CreateParticleModel(spriteAnimation, 1, 140, 0, 0);
    Hu3DModelLayerSet(storySelectionState.spriteModelIds[0], 7);
    for (modelIndex = 0; modelIndex < 5; modelIndex++) {
        spriteAnimation = HuSprAnimRead(HuDataReadNum(storyResultMotions[modelIndex], HU_MEMNUM_OVL));
        storySelectionState.spriteModelIds[modelIndex + 1] = CreateParticleModel(spriteAnimation, 2, 45, 0, 0);
        Hu3DModelLayerSet(storySelectionState.spriteModelIds[modelIndex + 1], 7);
    }
    object->objFunc = UpdateStoryItemSequence;
}

void SetStoryAnimationTimer(s16 timer)
{
    animationTimer = timer;
}

void UpdateItemDropAnimation(StoryItemManagerState *playerStates);
void AdvanceStoryItemSelection(StoryItemManagerState *playerStates);
void UpdateStoryItemSpin(StoryItemManagerState *playerStates);
void ProcessSelectedStoryItems(StoryItemManagerState *playerStates);
void UpdateSelectedItemAnimation(StoryItemManagerState *playerStates);
void UpdateStoryStageMotion(void);

void UpdateStoryItemSequence(OMOBJ *object)
{
    StoryItemManagerState *playerStates = object->data;
    if (!GetSequenceStatusFlags(1)) {
        if (GetSequenceStatusFlags(28) == 12 || GetSequenceStatusFlags(28) == 8) {
            switch (GetSequenceStatusFlags(224)) {
                case 32:
                    UpdateItemDropAnimation(playerStates);
                    break;

                case 64:
                    AdvanceStoryItemSelection(playerStates);
                    break;

                case 96:
                    UpdateStoryItemSpin(playerStates);
                    break;

                case 128:
                    if (GetSequenceStatusFlags(28) == 12) {
                        ProcessSelectedStoryItems(playerStates);
                    }
                    break;

                case 0:
                    UpdateSelectedItemAnimation(playerStates);
                    break;
            }
            UpdateStoryItemPositions(0, &playerStates[0]);
            UpdateStoryItemPositions(1, &playerStates[1]);
        }
    }
    UpdateStoryStageMotion();
}

u16 itemAnimationPhase = 1;

void RandomizeStoryItemModels(StoryItemManagerState *playerStates);

void UpdateItemDropAnimation(StoryItemManagerState *playerStates)
{
    float dropAnimationProgress;
    float itemHeight;
    StoryItemState *itemState;
    s16 playerSide;
    s16 itemIndex;
    StoryEventState *eventState;
    eventState = storyEventObject->data;

    switch (itemAnimationPhase) {
        case 0:
            if (animationTimer == 1) {
                HuAudFXPlay(1670);
            }
            animationTimer++;
            dropAnimationProgress = animationTimer / (1.0f * REFRESH_RATE);
            if (dropAnimationProgress >= 1.0f) {
                eventState->eventTimer = 15 * REFRESH_RATE;
                animationTimer = 0;
                dropAnimationProgress = 1;
                itemAnimationPhase++;
            }
            dropAnimationProgress = cosd(dropAnimationProgress * 90.0f);
            itemHeight = -50 + (95 * dropAnimationProgress);
            break;

        case 1:
            itemHeight = -50;
            RandomizeStoryItemModels(playerStates);
            itemAnimationPhase++;
            break;

        case 2:
            if (animationTimer == 1) {
                HuAudFXPlay(1663);
            }
            animationTimer++;
            dropAnimationProgress = animationTimer / (1.0f * REFRESH_RATE);
            if (dropAnimationProgress >= 1.0f) {
                animationTimer = 0;
                dropAnimationProgress = 1.0f;
                itemAnimationPhase++;
            }
            dropAnimationProgress = sind(dropAnimationProgress * 90.0f);
            itemHeight = -50 + (95 * dropAnimationProgress);
            break;

        case 3:
            itemHeight = 45;
            UpdateSequenceStatusFlags(224, 64);
            itemAnimationPhase = 0;
            break;
    }
    for (playerSide = 0; playerSide < 2; playerSide++, playerStates++) {
        itemState = &playerStates->itemStates[0];
        for (itemIndex = 0; itemIndex < 10; itemIndex++, itemState++) {
            if (itemAnimationPhase || -50.0f != Hu3DData[itemState->primaryModelId].pos.y) {
                Hu3DModelPosSet(itemState->primaryModelId, itemState->position.x, itemHeight, itemState->position.z);
                Hu3DModelRotSet(itemState->primaryModelId, 0, itemState->heading, 0);
                Hu3DModelPosSet(itemState->overlayModelId, itemState->position.x, itemHeight, itemState->position.z);
                Hu3DModelRotSet(itemState->overlayModelId, 0, itemState->heading, 0);
            }
        }
    }
}

void AdvanceStoryItemSelection(StoryItemManagerState *playerStates)
{
    float itemHeight;
    s16 playerDropTimers[2];
    StoryItemState *itemState;
    s16 itemOrderIndex;
    s16 itemIndex;
    s16 playerSide;
    s16 itemMoved;
    u32 itemStateFlags;
    switch (itemAnimationPhase) {
        case 0:
            animationTimer++;
            if (animationTimer > REFRESH_RATE) {
                animationTimer = 0;
                itemAnimationPhase++;
            }
            return;

        case 1:
            itemIndex = playerStates[0].itemOrder[4];
            playerStates[0].itemStates[itemIndex].stateFlags = 4;
            itemIndex = playerStates[1].itemOrder[4];
            playerStates[1].itemStates[itemIndex].stateFlags = 4;
            animationTimer = 999;
            itemAnimationPhase++;
            break;

        case 2:
            itemState = &playerStates[0].itemStates[playerStates[0].itemOrder[5]];
            if ((itemState->stateFlags & 0x1C) == 0x10) {
                animationTimer = 0;
                UpdateSequenceStatusFlags(224, 96);
                itemAnimationPhase = 0;
                return;
            }
            break;
    }
    itemMoved = 0;
    playerDropTimers[0] = playerDropTimers[1] = ++animationTimer;
    for (playerSide = 0; playerSide < 2; playerSide++, playerStates++) {
        for (itemOrderIndex = 4; itemOrderIndex > -6; itemOrderIndex--) {
            itemIndex = (itemOrderIndex < 0) ? playerStates->itemOrder[itemOrderIndex + 10] : playerStates->itemOrder[itemOrderIndex];
            itemState = &playerStates[0].itemStates[itemIndex];
            switch (itemStateFlags = itemState->stateFlags & 0x1C) {
                case 4:
                    if (playerDropTimers[playerSide] > (REFRESH_RATE/4)) {
                        playerDropTimers[playerSide] = 0;
                        itemMoved = 1;
                        if (itemOrderIndex - 1 > -6) {
                            itemIndex = ((itemOrderIndex - 1) < 0) ? playerStates->itemOrder[(itemOrderIndex - 1) + 10] : playerStates->itemOrder[(itemOrderIndex - 1)];
                            playerStates[0].itemStates[itemIndex].stateFlags &= ~0x1C;
                            playerStates[0].itemStates[itemIndex].stateFlags |= 0x4;
                        }
                        itemState->stateFlags &= ~0x1C;
                        itemState->stateFlags |= 8;
                        itemState->verticalVelocity = 0;
                        Hu3DModelAttrReset(itemState->itemModelId, HU3D_ATTR_DISPOFF);
                    }
                    break;

                case 8:
                case 12:
                    itemHeight = Hu3DData[itemState->itemModelId].pos.y;
                    itemState->verticalVelocity -= 0.98f;
                    itemHeight += itemState->verticalVelocity;
                    if (itemHeight <= 50.0f) {
                        itemHeight = 50.0f;
                        itemState->stateFlags &= ~0x1C;
                        if (itemStateFlags == 8) {
                            itemState->verticalVelocity = 6.0f;
                            itemState->stateFlags |= 12;
                            HuAudFXPlay(1664);
                        }
                        else {
                            itemState->verticalVelocity = 0;
                            itemState->stateFlags |= 16;
                        }
                    }
                    Hu3DModelPosSet(itemState->itemModelId, itemState->position.x, itemHeight, itemState->position.z);
                    Hu3DModelRotSet(itemState->itemModelId, 0, itemState->heading, 0);
                    break;

                default:
                    break;
            }
        }
    }
    if (itemMoved) {
        animationTimer = 0;
    }
}

s32 itemSpinSoundId = -1;
Vec itemSpinAxis = { -150, 150, 0 };
void RandomizePlayerItemTargets(void);
void ConfigureComputerItemChoices(s16 playerSide, PlayerCharacterState *characterState);
void CreateStorySelectionParticles(void);
void SetStoryStageMotion(s16 motionMode);
s32 PlaySoundEffectWithVolume(s32 soundId, s16 volume);

void UpdateStoryItemSpin(StoryItemManagerState *playerStates)
{
    float playerDisplayAngles[2];
    float spinProgress;
    float itemHeight;
    StoryItemState *itemState;
    s16 playerSide;
    PlayerCharacterState *playerData;
    s16 itemIndex;
    s16 spinComplete;
    spinComplete = 0;
    switch (itemAnimationPhase) {
        case 0:
            playerTargetAngles[0] = playerStates[0].playerHeading;
            playerTargetAngles[1] = playerStates[1].playerHeading;
            playerSide = rand16() % 11;
            spinProgress = (playerSide < 5) ? 360.0f : 180.0f;
            spinProgress += playerSide * 36.0f;
            playerStartAngles[0] = playerTargetAngles[0] + spinProgress;
            playerStartAngles[1] = playerTargetAngles[1] - spinProgress;
            spinDuration = (0.01f * spinProgress) * REFRESH_RATE;
            itemAnimationPhase++;
            itemSpinSoundId = HuAudFXPlay(1665);
            break;

        case 1:
            if (animationTimer == spinDuration) {
                playerStates[0].playerHeading = fmod(playerStates[0].playerHeading, 360);
                playerStates[1].playerHeading = fmod(playerStates[1].playerHeading, 360);
                if (playerStates[0].playerHeading < 0.0f) {
                    playerStates[0].playerHeading += 360.0f;
                }
                if (playerStates[1].playerHeading < 0.0f) {
                    playerStates[1].playerHeading += 360.0f;
                }
                SortStoryItemsByAngle(&playerState[0]);
                SortStoryItemsByAngle(&playerState[1]);
                itemAnimationPhase++;
                animationTimer = 0;
                spinComplete = 1;
            }
            break;

        case 2:
            spinComplete = 1;
            RandomizePlayerItemTargets();
            CreateStorySelectionParticles();
            SetStoryStageMotion(1);
            for (playerSide = 0; playerSide < 4; playerSide++) {
                playerData = playerObjects[playerSide]->data;
                ConfigureComputerItemChoices(playerData->groupIndex, playerData);
            }
            itemAnimationPhase = 0;
            UpdateSequenceStatusFlags(224, 0);
            break;
    }
    playerDisplayAngles[0] = playerItemSpinOffset + playerStates[0].playerHeading;
    playerDisplayAngles[1] = playerItemSpinOffset + playerStates[1].playerHeading;
    if (spinComplete == 0) {
        animationTimer++;
        spinProgress = animationTimer / (float)spinDuration;
        spinProgress = sind(spinProgress * 90.0f);
        if (animationTimer > spinDuration - 120) {
            itemHeight = (animationTimer - (spinDuration - 120)) / 120.0f;
            if (itemHeight < 0.0f) {
                itemHeight = 0.0f;
            }
            else {
                if (itemHeight > 1.0f) {
                    itemHeight = 1.0f;
                }
            }
            if (itemSpinSoundId != -1) {
                HuAudFXPitchSet(itemSpinSoundId, -8191.0f * itemHeight);
                PlaySoundEffectWithVolume(itemSpinSoundId, (1.0f - itemHeight) * 127);
            }
            if (animationTimer >= spinDuration) {
                if (itemSpinSoundId != -1) {
                    HuAudFXStop(itemSpinSoundId);
                    itemSpinSoundId = -1;
                    HuAudFXPlay(1675);
                }
            }
        }
        if (animationTimer > spinDuration) {
            animationTimer = spinDuration;
            spinProgress = 1;
        }
        playerStates[0].playerHeading = playerTargetAngles[0] + (spinProgress * (playerStartAngles[0] - playerTargetAngles[0]));
        playerStates[1].playerHeading = playerTargetAngles[1] + (spinProgress * (playerStartAngles[1] - playerTargetAngles[1]));
    }
    Hu3DData[storyItemsObject->mdlId[3]].rot.y = playerDisplayAngles[0];
    Hu3DData[storyItemsObject->mdlId[4]].rot.y = playerDisplayAngles[1];
    for (playerSide = 0; playerSide < 2; playerSide++, playerStates++) {
        itemState = &playerStates->itemStates[0];
        for (itemIndex = 0; itemIndex < 10; itemIndex++, itemState++) {
            float itemPositionX = itemState->position.x;
            float itemPositionZ = itemState->position.z;
            Hu3DModelPosSet(itemState->primaryModelId, itemPositionX, 45, itemPositionZ);
            Hu3DModelRotSet(itemState->primaryModelId, 0, itemState->heading, 0);
            Hu3DModelPosSet(itemState->overlayModelId, itemPositionX, 45, itemPositionZ);
            Hu3DModelRotSet(itemState->overlayModelId, 0, itemState->heading, 0);
            Hu3DModelPosSet(itemState->itemModelId, itemPositionX, 50, itemPositionZ);
            Hu3DModelRotSet(itemState->itemModelId, 0, itemState->heading, 0);
        }
    }
}

void HideSelectedItemParticle(ParticleState *particle);

void ProcessSelectedStoryItems(StoryItemManagerState *playerStates)
{
    StoryItemState *itemState;
    HU3DMODEL *itemModel;
    s16 playerSide;
    StoryEventState *eventState;
    s16 itemIndex;
    switch (itemAnimationPhase) {
        case 0:
            if (++animationTimer > REFRESH_RATE) {
                animationTimer = 0;
                itemAnimationPhase++;
            }
            break;

        case 1:
            for (playerSide = 0; playerSide < 2; playerSide++, playerStates++) {
                itemState = &playerStates->itemStates[0];
                for (itemIndex = 0; itemIndex < 10; itemIndex++, itemState++) {
                    itemModel = &Hu3DData[itemState->itemModelId];
                    itemState->stateFlags &= ~0x1C;
                    if (!(itemState->stateFlags & 0x1)) {
                        CreateParticleEffect(itemModel->pos.x, 20 + itemModel->pos.y, itemModel->pos.z, 20, HideSelectedItemParticle, itemState->itemModelId);
                        if (playerSide == 0) {
                            HuAudFXPlay(1669);
                        }
                        else {
                            HuAudFXPlay(1674);
                        }
                    }
                    else {
                        itemModel->pos.y = 1000;
                    }
                    itemState->stateFlags &= ~0x3;
                    Hu3DModelScaleSet(itemState->primaryModelId, 1, 1, 1);
                }
            }
            itemAnimationPhase++;
            break;

        case 2:
            if (++animationTimer > REFRESH_RATE) {
                animationTimer = itemAnimationPhase = 0;
                UpdateSequenceStatusFlags(224, 32);
            }
            break;
    }
}

void HideSelectedItemParticle(ParticleState *particle)
{
    Hu3DData[particle->modelId].pos.y = 1000;
    Hu3DModelAttrSet(particle->modelId, HU3D_ATTR_DISPOFF);
}

void UpdateSelectedItemAnimation(StoryItemManagerState *playerStates)
{
    float bounceScale;
    float itemHeight;
    StoryItemState *itemState;
    StoryEventState *eventState;
    s16 playerSide;
    s16 itemIndex;
    s16 selectedPlayer;
    PlayerCharacterState *playerData;
    eventState = storyEventObject->data;
    selectedPlayer = -1;
    switch (itemAnimationPhase) {
        case 0:
            eventState->sequenceTimerId = MGSeqTimerCreate(15);
            itemAnimationPhase++;
            break;

        case 1:
            if (playerGroups[0].playerReadyFlags[0] && playerGroups[0].playerReadyFlags[1]) {
                selectedPlayer = 0;
            }
            if (playerGroups[1].playerReadyFlags[0] && playerGroups[1].playerReadyFlags[1]) {
                if (selectedPlayer == 0) {
                    selectedPlayer = (rand16() % 2) ? 0 : 1;
                }
                else {
                    selectedPlayer = 1;
                }
            }
            eventState->eventAndSpriteState[1] = selectedPlayer;
            if (selectedPlayer != -1) {
                StartPlayerGroupVictory(eventState->eventAndSpriteState[1]);
                StartWinnerSpritePulse(playerGroups[selectedPlayer].completedRoundCounts[0]);
                HideStorySelectionParticles();
                SetStoryStageMotion(3);
                playerGroups[selectedPlayer].completedRoundCounts[0]++;
                MGSeqParamSet(eventState->sequenceTimerId, 2, -1);
                eventState->sequenceTimerId = -1;
                eventState->eventAndSpriteState[0]++;
                animationTimer = 0;
                itemAnimationPhase++;
            }
            else {
                if (--eventState->eventTimer < -1) {
                    for (playerSide = 0; playerSide < 4; playerSide++) {
                        playerData = playerObjects[playerSide]->data;
                        for (itemIndex = 0; itemIndex < 8; itemIndex++) {
                            playerData->availableItemIndices[itemIndex] = -1;
                        }
                    }
                    HideStorySelectionParticles();
                    SetStoryStageMotion(4);
                    eventState->eventTimer = 0;
                    MGSeqParamSet(eventState->sequenceTimerId, 2, -1);

                    eventState->sequenceTimerId = -1;
                    eventState->eventAndSpriteState[0]++;
                    animationTimer = 0;
                    itemAnimationPhase++;
                }
            }
            if (eventState->eventAndSpriteState[0] >= 5) {
                if (playerGroups[0].completedRoundCounts[0] == playerGroups[1].completedRoundCounts[0]) {
                    HuAudFXPlay(11);
                }
            }
            break;

        case 2:
            if (++animationTimer > (1.5f * REFRESH_RATE)) {
                animationTimer = 0;
                itemAnimationPhase = 1;
                UpdateSequenceStatusFlags(224, 128);
            }
            break;
    }
    for (playerSide = 0; playerSide < 2; playerSide++, playerStates++) {
        itemState = &playerStates->itemStates[0];
        for (itemIndex = 0; itemIndex < 10; itemIndex++, itemState++) {
            if (!(itemState->stateFlags & 0x1)) {
                continue;
            }
            if (itemState->stateFlags & 0x2) {
                itemState->animationFrame++;
                bounceScale = itemState->animationFrame / (0.5f * REFRESH_RATE);
                if (bounceScale >= 1.0f) {
                    itemState->verticalVelocity -= 1.0f;
                    if (itemState->verticalVelocity < 0.4f) {
                        itemState->verticalVelocity = 0.4f;
                    }
                    itemState->animationFrame = 0;
                    bounceScale = 1.0f;
                }
                bounceScale = (itemState->verticalVelocity * 0.7f) * sind(180.0f * bounceScale) + 1.0f;
                Hu3DModelScaleSet(itemState->primaryModelId, bounceScale, bounceScale, bounceScale);
            }
            else {
                if (!itemState->verticalVelocity) {
                    if (++itemState->animationFrame > (0.5f * REFRESH_RATE)) {
                        itemState->verticalVelocity = 1;
                        itemState->animationFrame = 0;
                    }
                }
                else {
                    itemState->animationFrame++;
                    bounceScale = itemState->animationFrame / (REFRESH_RATE*0.9f);
                    if (bounceScale >= 1.0f) {
                        itemState->animationFrame = (REFRESH_RATE*0.9f);
                        bounceScale = 1.0f;
                    }
                    bounceScale = cosd(bounceScale * 90.0f);
                    itemHeight = -50 + (95 * bounceScale);
                    Hu3DModelPosSet(itemState->primaryModelId, itemState->position.x, itemHeight, itemState->position.z);
                    Hu3DModelPosSet(itemState->overlayModelId, itemState->position.x, itemHeight, itemState->position.z);
                }
            }
        }
    }
}

s16 GetStoryItemAnimationPhase(void)
{
    return itemAnimationPhase;
}

u32 GetStoryItemFlags(s16 playerSide, s16 itemIndex)
{
    StoryItemManagerState *playerState;
    playerState = storyItemsObject->data;
    playerState += playerSide;
    if (itemIndex == -1) {
        return 0;
    }
    return playerState->itemStates[playerState->itemOrder[itemIndex]].stateFlags;
}

void InitializeAndShuffleIndices(s16 *values, s16 startValue, s16 count);

void RandomizeStoryItemModels(StoryItemManagerState *playerStates)
{
    s16 itemIndex;
    s16 shuffledItemIds[10];
    StoryItemState *playerItems[2];
    InitializeAndShuffleIndices(shuffledItemIds, 0, 10);
    playerItems[0] = &playerStates[0].itemStates[0];
    playerItems[1] = &playerStates[1].itemStates[0];
    for (itemIndex = 0; itemIndex < 10; itemIndex++, playerItems[0]++, playerItems[1]++) {
        playerItems[0]->primaryModelId = itemModelIds[shuffledItemIds[itemIndex]];
        playerItems[1]->primaryModelId = itemModelIds[shuffledItemIds[itemIndex] + 10];
        playerItems[0]->animationFrame = playerItems[1]->animationFrame = (shuffledItemIds[itemIndex] < 5) ? shuffledItemIds[itemIndex] : shuffledItemIds[itemIndex] - 5;
    }
}

void SortStoryItemsByAngle(StoryItemManagerState *playerState)
{
    float itemAngle;
    s16 itemOrderIndex;
    s16 itemIndex;
    for (itemIndex = 0; itemIndex < 10; itemIndex++) {
        itemAngle = playerState->playerHeading + playerState->itemStates[itemIndex].spinAngle;
        itemOrderIndex = (itemAngle / 360.0f) * 10.0f;
        if (itemOrderIndex >= 10) {
            itemOrderIndex -= 10;
        }
        else if (itemOrderIndex < 0) {
            itemOrderIndex += 10;
        }
        playerState->itemOrder[itemOrderIndex] = itemIndex;
    }
}

void UpdateStoryItemPositions(s16 playerSide, StoryItemManagerState *playerState)
{
    float itemAngle;
    StoryItemState *itemState;
    PlayerGroupState *playerConfig;
    s16 itemIndex;
    itemState = &playerState->itemStates[0];
    playerConfig = &playerGroups[playerSide];
    for (itemIndex = 0; itemIndex < 10; itemIndex++, itemState++) {
        itemAngle = playerState->playerHeading + itemState->spinAngle;
        if (itemAngle >= 360.0f) {
            itemAngle -= 360.0f;
        }
        itemState->position.x = playerConfig->startingPosition.x + (153.0 * sind(itemAngle));
        itemState->position.z = playerConfig->startingPosition.z + (153.0 * cosd(itemAngle));
        itemState->heading = atan2d(playerConfig->startingPosition.x - itemState->position.x, playerConfig->startingPosition.z - itemState->position.z);
    }
}

s16 SelectStoryItem(s16 playerSide, s16 itemIndex, s16 *playerMatched)
{
    StoryItemState *selectedItem;
    s16 playerIndex;
    StoryItemManagerState *playerState;
    playerState = storyItemsObject->data;
    *playerMatched = 0;
    playerState += playerSide;
    selectedItem = &playerState->itemStates[playerState->itemOrder[itemIndex]];
    if (selectedItem->stateFlags & 0x1) {
        return -1;
    }
    selectedItem->stateFlags |= 0x1;
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        if (!playerGroups[playerSide].playerReadyFlags[playerIndex] && storySelectionState.randomizedPlayerOrder[playerIndex] == selectedItem->animationFrame) {
            *playerMatched = 1;
            playerGroups[playerSide].playerReadyFlags[playerIndex] = 1;
            selectedItem->stateFlags |= 0x2;
            selectedItem->animationFrame = 0;
            selectedItem->verticalVelocity = 1;
            Hu3DMotionTimeSet(storyItemsObject->mdlId[playerSide + 1], 0);
            Hu3DModelAttrReset(storyItemsObject->mdlId[playerSide + 1], HU3D_MOTATTR_PAUSE);
            HuAudFXPlay(9);
            break;
        }
    }
    return selectedItem->itemModelId;
}

s16 IsStoryItemAvailable(s16 playerSide, s16 itemIndex)
{
    StoryItemManagerState *playerState;
    StoryItemState *selectedItem;
    playerState = storyItemsObject->data;
    playerState += playerSide;
    selectedItem = &playerState->itemStates[playerState->itemOrder[itemIndex]];
    if (selectedItem->stateFlags & 0x1) {
        return 0;
    }
    else {
        return 1;
    }
}

void RandomizePlayerItemTargets(void)
{
    storySelectionState.randomizedPlayerOrder[0] = rand16() % 5;
    storySelectionState.randomizedPlayerOrder[1] = rand16() % 5;
    playerGroups[0].playerReadyFlags[0] = playerGroups[0].playerReadyFlags[1] = 0;
    playerGroups[1].playerReadyFlags[0] = playerGroups[1].playerReadyFlags[1] = 0;
}

void ShuffleShortArray(s16 *values, s16 count);

void ConfigureComputerItemChoices(s16 playerGroupIndex, PlayerCharacterState *characterState)
{
    s16 claimedItemFlags[10];
    s16 remainingItemIds[10];
    s16 itemIndex;
    s16 orderedItemCount;
    s16 opposingPlayerIndex;
    StoryItemManagerState *storyItemStates;
    StoryItemState *itemState;
    storyItemStates = storyItemsObject->data;
    for (itemIndex = 0; itemIndex < 10; itemIndex++) {
        claimedItemFlags[itemIndex] = remainingItemIds[itemIndex] = 0;
    }
    for (itemIndex = 0; itemIndex < 8; itemIndex++) {
        characterState->availableItemIndices[itemIndex] = -1;
    }
    storyItemStates += playerGroupIndex;
    for (opposingPlayerIndex = 0, orderedItemCount = 0; opposingPlayerIndex < 2; opposingPlayerIndex++) {
        for (itemIndex = 0; itemIndex < 10; itemIndex++) {
            itemState = &storyItemStates->itemStates[storyItemStates->itemOrder[itemIndex]];
            if (itemState->animationFrame == storySelectionState.randomizedPlayerOrder[opposingPlayerIndex] && !claimedItemFlags[itemIndex]) {
                characterState->availableItemIndices[orderedItemCount++] = itemIndex;
                claimedItemFlags[itemIndex] = 1;
            }
        }
    }
    for (opposingPlayerIndex = 0, itemIndex = 0; opposingPlayerIndex < 10; opposingPlayerIndex++) {
        if (!claimedItemFlags[opposingPlayerIndex]) {
            remainingItemIds[itemIndex++] = opposingPlayerIndex;
        }
    }
    ShuffleShortArray(remainingItemIds, itemIndex);
    itemIndex = 0;
    if (orderedItemCount == 2) {
        orderedItemCount += 2;
    }

    while (1) {
        if (orderedItemCount >= 8) {
            break;
        }
        characterState->availableItemIndices[orderedItemCount++] = remainingItemIds[itemIndex++];
    }
    characterState->selectedItemCount = 0;
    orderedItemCount = characterState->opponent->itemIndex;
    switch (GWPlayerCfg[characterState->playerIndex].diff) {
        case 0:
            characterState->computerChoiceBias = (GWPlayerCfg[orderedItemCount].iscom) ? ((rand16() % 3) + 2) : ((rand16() % 3) + 1);
            characterState->availableItemCount = 8;
            break;

        case 1:
            characterState->computerChoiceBias = (GWPlayerCfg[orderedItemCount].iscom) ? ((rand16() % 3) + 1) : ((rand16() % 3) + 0);
            characterState->availableItemCount = 7;
            break;

        case 2:
            characterState->computerChoiceBias = (GWPlayerCfg[orderedItemCount].iscom) ? ((rand16() % 3) + 0) : ((rand16() % 2) + 0);
            characterState->availableItemCount = 6;
            break;

        case 3:
            characterState->computerChoiceBias = (GWPlayerCfg[orderedItemCount].iscom) ? ((rand16() % 2) + 0) : 0;
            characterState->availableItemCount = 5;
            break;
    }
    if (characterState->availableItemCount < 4) {
        characterState->availableItemCount = 4;
    }
}

void UpdateStorySelectionParticlePositions(void);

ParticleState *CreateModelParticle(s16 modelId, Vec *position, Vec *scale, float rotation, GXColor *color);

void CreateStorySelectionParticles(void)
{
    StorySelectionState *storyState;
    s16 playerIndex;
    ParticleState *particle;
    Vec particlePosition;
    Vec particleScale;
    GXColor particleColor = { 255, 255, 255, 255 };
    storyState = &storySelectionState;
    particlePosition.x = 0;
    particlePosition.y = 200;
    particlePosition.z = -300;
    particleScale.x = particleScale.z = 1;
    particleScale.y = 0.5f;
    CreateModelParticle(storyState->spriteModelIds[0], &particlePosition, &particleScale, 0, &particleColor);
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        particlePosition.x = (playerIndex == 0) ? -60.0f : 60.0f;
        particlePosition.y = 220;
        particlePosition.z = -290;
        particleScale.x = particleScale.y = particleScale.z = 1;
        particle = CreateModelParticle(storyState->spriteModelIds[storyState->randomizedPlayerOrder[playerIndex] + 1], &particlePosition, &particleScale, 0, &particleColor);
        particle->lifetimeFrame = playerIndex;
    }
    UpdateStorySelectionParticlePositions();
}

ParticleSystemState *GetParticleModelState(s16 modelId);

void HideStorySelectionParticles(void)
{
    ParticleSystemState *particleModel;
    StorySelectionState *storyState = &storySelectionState;
    s16 playerIndex;
    particleModel = GetParticleModelState(storyState->spriteModelIds[0]);
    particleModel->particles[0].isActive = 0;
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        particleModel = GetParticleModelState(storyState->spriteModelIds[storyState->randomizedPlayerOrder[playerIndex] + 1]);
        particleModel->particles[0].isActive = particleModel->particles[1].isActive = 0;
    }
}

void UpdateStorySelectionParticlePositions(void)
{
    float playerOffsetX;
    StorySelectionState *storyState;
    ParticleSystemState *particleModel;
    s16 playerIndex;
    s16 playerSide;
    storyState = &storySelectionState;
    particleModel = GetParticleModelState(storyState->spriteModelIds[0]);
    particleModel->particles[0].position.x = storyState->cameraCenterX;
    for (playerSide = 0; playerSide < 2; playerSide++) {
        particleModel = GetParticleModelState(storyState->spriteModelIds[storyState->randomizedPlayerOrder[playerSide] + 1]);
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            if (particleModel->particles[playerIndex].isActive) {
                playerOffsetX = (particleModel->particles[playerIndex].lifetimeFrame == 0) ? -60.0f : 60.0f;
                particleModel->particles[playerIndex].position.x = playerOffsetX + storyState->cameraCenterX;
            }
        }
    }
}

void SetStoryStageMotion(s16 motionMode)
{
    HU3DMODEL *stageModel = &Hu3DData[storyItemsObject->mdlId[5]];
    storySelectionState.stageMotionRepeatCount = 0;
    storySelectionState.motionTransitionState = 0;
    storySelectionState.motionMode = motionMode;
    storySelectionState.unusedState20 = storySelectionState.unusedState34 = 0;
    storySelectionState.cameraYawOffsets[1] = storySelectionState.cameraYawOffsets[0];
}

void UpdateStoryStageMotion(void)
{
    StorySelectionState *storyState;
    HU3DMODEL *stageModel;
    StoryEventState *eventState;
    stageModel = &Hu3DData[storyItemsObject->mdlId[5]];
    eventState = storyEventObject->data;
    storyState = &storySelectionState;
    switch (storyState->motionMode) {
        case 0:
            if (stageModel->motId != storyItemsObject->mtnId[0]) {
                Hu3DMotionShiftSet(storyItemsObject->mdlId[5], storyItemsObject->mtnId[0], 0, 8, HU3D_MOTATTR_LOOP);
            }
            break;

        case 1:
            if (storyState->motionTransitionState == 0) {
                storyState->motionTransitionState = 1;
                Hu3DMotionShiftSet(storyItemsObject->mdlId[5], storyItemsObject->mtnId[1], 0, 8, HU3D_MOTATTR_NONE);
            }
            else {
                if (stageModel->motIdShift == -1) {
                    if (stageModel->motWork.time >= Hu3DMotionMaxTimeGet(storyItemsObject->mdlId[5])) {
                        storyState->selectedStageMotionVariant = rand16() % 2;
                        storyState->motionTransitionState = 0;
                        storyState->motionMode = 2;
                    }
                }
            }
            break;

        case 2:
            if (storyState->motionTransitionState == 0) {
                storyState->motionTransitionState = 1;
                Hu3DMotionShiftSet(storyItemsObject->mdlId[5], storyItemsObject->mtnId[5], 0, 8, HU3D_MOTATTR_LOOP);
            }
            else {
                if (eventState->eventTimer < (3 * REFRESH_RATE)) {
                    storyState->motionTransitionState = 0;
                    storyState->motionMode = 5;
                }
            }
            break;

        case 3:
            if (storyState->motionTransitionState == 0) {
                storyState->motionTransitionState = 1;
                Hu3DMotionShiftSet(storyItemsObject->mdlId[5], storyItemsObject->mtnId[2], 0, 8, HU3D_MOTATTR_LOOP);
            }
            else {
                if (stageModel->motIdShift == -1) {
                    if (stageModel->motWork.time >= Hu3DMotionMaxTimeGet(storyItemsObject->mdlId[5]) - 1.0f) {
                        if (++storyState->stageMotionRepeatCount > 1) {
                            storyState->motionTransitionState = 0;
                            storyState->motionMode = 0;
                        }
                    }
                }
            }
            break;

        case 4:
            if (storyState->motionTransitionState == 0) {
                storyState->motionTransitionState = 1;
                Hu3DMotionShiftSet(storyItemsObject->mdlId[5], storyItemsObject->mtnId[3], 0, 8, HU3D_MOTATTR_NONE);
            }
            else {
                if (stageModel->motIdShift == -1) {
                    if (stageModel->motWork.time >= Hu3DMotionMaxTimeGet(storyItemsObject->mdlId[5])) {
                        storyState->motionTransitionState = 0;
                        storyState->motionMode = 0;
                    }
                }
            }
            break;

        case 5:
            if (storyState->motionTransitionState == 0) {
                storyState->motionTransitionState = 1;
                Hu3DMotionShiftSet(storyItemsObject->mdlId[5], storyItemsObject->mtnId[6], 0, 8, HU3D_MOTATTR_LOOP);
            }
            break;
    }
    stageModel->pos.x = storyState->cameraCenterX;
    stageModel->rot.y = storyState->cameraYawOffsets[0];
}

void CreateParticleEffect(float x, float y, float z, s16 particleCount, ParticleUpdateCallback updateCallback, s16 modelId)
{
    Vec particlePosition;
    Vec particleScale;
    GXColor particleColor;
    float particleAngle;
    float angleStep;

    ParticleState *particle;
    s16 particleIndex;
    angleStep = 360.0f / particleCount;
    particleAngle = rand16() % 361;
    for (particleIndex = 0; particleIndex < particleCount; particleIndex++, particleAngle += angleStep) {
        particlePosition.x = x + ((rand16() % 21) - 10);
        particlePosition.y = y + ((rand16() % 21) - 10);
        particlePosition.z = z + ((rand16() % 21) - 10);
        particleScale.x = particleScale.y = particleScale.z = 0.1f * ((rand16() % 11) + 15);
        particleColor.r = particleColor.g = particleColor.b = (rand16() % 128) + 128;
        particleColor.a = 128;
        particle = CreateModelParticle(storyEventObject->mdlId[0], &particlePosition, &particleScale, 0, &particleColor);
        if (!particle) {
            break;
        }
        particle->updateCallback = updateCallback;
        particle->modelId = modelId;
        particle->lifetimeFrame = 0;
        particle->lifetimeLimit = (rand16() % 2) + 2;
        particle->velocity.x = 3.5 * sind(particleAngle);
        particle->velocity.y = 0.01f * ((rand16() % 16) + 20);
        particle->velocity.z = 3.5 * cosd(particleAngle);
        particle->velocityDecay.x = particle->velocity.x * 0.03f;
        particle->velocityDecay.y = particle->velocity.y * 0.03f;
        particle->velocityDecay.z = particle->velocity.z * 0.03f;
    }
}

void InitializeParticleModel(HU3DMODEL *model, ParticleSystemState *particleSystem, Mtx modelMatrix)
{
    ParticleState *particle;
    s16 particleIndex;
    GXColor *particleColor;
    ParticleUpdateCallback particleCallback;
    particle = particleSystem->particles;
    particleColor = particleSystem->vertexColors;
    for (particleIndex = 0; particleIndex < particleSystem->particleCount; particleIndex++, particle++, particleColor++) {
        if (particle->isActive == 0) {
            continue;
        }
        particle->position.x += particle->velocity.x;
        particle->position.y += particle->velocity.y;
        particle->position.z += particle->velocity.z;
        particle->velocity.x -= particle->velocityDecay.x;
        particle->velocity.y -= particle->velocityDecay.y;
        particle->velocity.z -= particle->velocityDecay.z;
        particle->lifetimeFrame++;
        if (particle->lifetimeFrame >= particle->lifetimeLimit) {
            particle->lifetimeFrame = 0;
            particle->textureTileIndex++;
        }
        if (particle->textureTileIndex >= particleSystem->textureTileCount) {
            particle->updateCallback = NULL;
            particle->isActive = 0;
        }
        else {
            if (particle->textureTileIndex == 2 && particle->updateCallback) {
                particleCallback = particle->updateCallback;
                particleCallback(particle);
            }
        }
    }
}

void CalculateCollisionPlane(Vec *triangleVertices, PlayerCollisionBoundary *collisionPlane)
{
    s16 axisIndex;
    s16 largestNormalAxis;
    float planeNormal[3];
    float normalLength;
    float edgeOneX;
    float edgeOneY;
    float edgeOneZ;
    float edgeTwoX;
    float edgeTwoY;
    float edgeTwoZ;
    edgeOneX = triangleVertices[1].x - triangleVertices[0].x;
    edgeOneY = triangleVertices[1].y - triangleVertices[0].y;
    edgeOneZ = triangleVertices[1].z - triangleVertices[0].z;
    edgeTwoX = triangleVertices[2].x - triangleVertices[0].x;
    edgeTwoY = triangleVertices[2].y - triangleVertices[0].y;
    edgeTwoZ = triangleVertices[2].z - triangleVertices[0].z;
    planeNormal[0] = (edgeOneY * edgeTwoZ) - (edgeOneZ * edgeTwoY);
    planeNormal[1] = (edgeOneZ * edgeTwoX) - (edgeOneX * edgeTwoZ);
    planeNormal[2] = (edgeOneX * edgeTwoY) - (edgeOneY * edgeTwoX);
    normalLength = VECMagPoint(planeNormal[0], planeNormal[1], planeNormal[2]);
    if (0.0f != normalLength) {
        planeNormal[0] /= normalLength;
        planeNormal[1] /= normalLength;
        planeNormal[2] /= normalLength;
    }
    collisionPlane->planeDistance = (planeNormal[0] * triangleVertices[0].x) + (planeNormal[1] * triangleVertices[0].y) + (planeNormal[2] * triangleVertices[0].z);
    largestNormalAxis = 0;
    for (axisIndex = 1; axisIndex < 3; axisIndex++) {
        if (ABS(planeNormal[largestNormalAxis]) < ABS(planeNormal[axisIndex])) {
            largestNormalAxis = axisIndex;
        }
    }
    collisionPlane->planeNormal.x = planeNormal[0];
    collisionPlane->planeNormal.y = planeNormal[1];
    collisionPlane->planeNormal.z = planeNormal[2];
    collisionPlane->dominantNormalAxis = largestNormalAxis;
}

float InterpolateAngle(float startAngle, float endAngle, float progress)
{
    float angleDelta = endAngle - startAngle;
    if (angleDelta > 180.0f) {
        angleDelta -= 360.0f;
    }
    else if (angleDelta < -180.0f) {
        angleDelta += 360.0f;
    }
    angleDelta = startAngle + (angleDelta * progress);
    if (angleDelta >= 360.0f) {
        angleDelta -= 360.0f;
    }
    else if (angleDelta < 0.0f) {
        angleDelta += 360.0f;
    }
    return angleDelta;
}

float InterpolateValue(float startValue, float endValue, float progress)
{
    return startValue + (progress * (endValue - startValue));
}

float InterpolateQuadraticCurve(float startValue, float controlValue, float endValue, float progress)
{
    float inverseProgress = 1.0f - progress;
    return (startValue * (inverseProgress * inverseProgress)) + ((2.0f * inverseProgress) * progress * controlValue) + (endValue * (progress * progress));
}

void ShuffleShortArray(s16 *values, s16 count)
{
    s16 itemIndex;
    s16 shuffledIndices[99];
    s16 originalIndices[99];
    memcpy(originalIndices, values, count * sizeof(s16));
    InitializeAndShuffleIndices(shuffledIndices, 0, count);
    for (itemIndex = 0; itemIndex < count; itemIndex++) {
        values[itemIndex] = originalIndices[shuffledIndices[itemIndex]];
    }
}

void InitializeAndShuffleIndices(s16 *values, s16 startValue, s16 count)
{
    s16 itemIndex;
    s16 randomIndex;
    s16 swappedValue;
    for (itemIndex = 0; itemIndex < count; itemIndex++) {
        values[itemIndex] = startValue + itemIndex;
    }
    for (itemIndex = count - 1; itemIndex >= 0; itemIndex--) {
        randomIndex = rand16() % (s16)(itemIndex + 1);
        swappedValue = values[itemIndex];
        values[itemIndex] = values[randomIndex];
        values[randomIndex] = swappedValue;
    }
}

void UpdateStorySelectionBlink(void)
{
    if (HuPadBtnDown[0] & PAD_TRIGGER_L) {
        coinAwardSide ^= 1;
    }
    if (coinAwardSide == 0) {
        return;
    }
    if (HuPadBtn[0] & PAD_BUTTON_LEFT) {
        CRot.y++;
    }
    if (HuPadBtn[0] & PAD_BUTTON_RIGHT) {
        CRot.y--;
    }
    if (HuPadBtn[0] & PAD_BUTTON_UP) {
        CRot.x--;
    }
    if (HuPadBtn[0] & PAD_BUTTON_DOWN) {
        CRot.x++;
    }
    Center.x += HuPadSubStkX[0];
    if (HuPadBtn[0] & PAD_TRIGGER_R) {
        Center.y += HuPadSubStkY[0];
    }
    else {
        Center.z += HuPadSubStkY[0];
    }
    if (HuPadBtn[0] & PAD_BUTTON_X) {
        CZoom += 10;
    }
    if (HuPadBtn[0] & PAD_BUTTON_Y) {
        CZoom -= 10;
    }
    if (HuPadBtnDown[0] & PAD_BUTTON_A) {
        OSReport("\nCZoom = %.2f \n", CZoom);
        OSReport("Center x = %.2f: y = %.2f: z = %.2f \n", Center.x, Center.y, Center.z);
        OSReport("CRot x = %.2f: y = %.2f: z = %.2f \n", CRot.x, CRot.y, CRot.z);
    }
}

ParticleState *CreateModelParticle(s16 modelId, Vec *position, Vec *scale, float rotation, GXColor *color)
{
    ParticleState *particle;
    ParticleSystemState *particleSystem;
    s16 particleIndex;
    HU3DMODEL *model;
    model = &Hu3DData[modelId];
    particleSystem = model->hookData;
    for (particleIndex = 0, particle = particleSystem->particles; particleIndex < particleSystem->particleCount; particleIndex++, particle++) {
        if (!particle->isActive) {
            break;
        }
    }
    if (particleIndex == particleSystem->particleCount) {
        return NULL;
    }
    particleSystem->vertexColors[particleIndex].r = color->r;
    particleSystem->vertexColors[particleIndex].g = color->g;
    particleSystem->vertexColors[particleIndex].b = color->b;
    particleSystem->vertexColors[particleIndex].a = color->a;
    particle->particleIndex = particleIndex;
    particle->rotation = rotation;
    particle->scale = *scale;
    particle->position = *position;
    particle->textureTileIndex = 0;
    particle->updateCallback = NULL;
    particle->isActive = 1;
    return particle;
}

void DrawParticleModel(HU3DMODEL *model, Mtx mtx);
void BuildTextureTileCoordinates(HuVec2f *coordinates, s16 tileCount, s16 tilesPerRow, float tileWidth, float tileHeight);

s16 CreateParticleModel(ANIMDATA *animation, s16 particleCount, float particleSize, s16 tileWidth, s16 tileHeight)
{
    ParticleSystemState *modelHookData;
    s16 spriteIndex;
    HuVec2f *textureCoordinates;
    HU3DMODEL *model;
    GXColor *vertexColors;
    Vec *vertexPositions;
    ParticleState *particleStates;
    s16 horizontalTileCount;
    void *displayListSource;
    s16 modelId;
    s16 verticalTileCount;
    void *displayList;
    modelId = Hu3DHookFuncCreate(DrawParticleModel);
    model = &Hu3DData[modelId];
    modelHookData = HuMemDirectMallocNum(HEAP_MODEL, sizeof(ParticleSystemState), model->mallocNo);
    model->hookData = modelHookData;
    animation->useNum++;
    modelHookData->animation = animation;
    modelHookData->particleCount = particleCount;
    modelHookData->renderMode = 0;
    modelHookData->renderCallback = 0;
    particleStates = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(ParticleState), model->mallocNo);
    modelHookData->particles = particleStates;
    for (spriteIndex = 0; spriteIndex < particleCount; spriteIndex++, particleStates++) {
        particleStates->textureTileIndex = -1;
        particleStates->isActive = 0;
    }
    vertexPositions = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(Vec) * 4, model->mallocNo);
    modelHookData->vertexPositions = vertexPositions;
    for (spriteIndex = 0; spriteIndex < particleCount * 4; spriteIndex++, vertexPositions++) {
        vertexPositions->x = vertexPositions->y = vertexPositions->z = 0;
    }
    vertexColors = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(GXColor), model->mallocNo);
    modelHookData->vertexColors = vertexColors;
    for (spriteIndex = 0; spriteIndex < particleCount; spriteIndex++, vertexColors++) {
        vertexColors->r = vertexColors->g = vertexColors->b = vertexColors->a = 255;
    }
    textureCoordinates = HuMemDirectMallocNum(HEAP_MODEL, particleCount * sizeof(HuVec2f) * 4, model->mallocNo);
    modelHookData->textureCoordinates = textureCoordinates;
    for (spriteIndex = 0; spriteIndex < particleCount; spriteIndex++) {
        textureCoordinates->x = 0;
        textureCoordinates->y = 0;
        textureCoordinates++;
        textureCoordinates->x = 1;
        textureCoordinates->y = 0;
        textureCoordinates++;
        textureCoordinates->x = 1;
        textureCoordinates->y = 1;
        textureCoordinates++;
        textureCoordinates->x = 0;
        textureCoordinates->y = 1;
        textureCoordinates++;
    }
    if (tileWidth != 0 || tileHeight != 0) {
        horizontalTileCount = animation->bmp->sizeX / tileWidth;
        verticalTileCount = animation->bmp->sizeY / tileHeight;
        modelHookData->textureTileCount = horizontalTileCount * verticalTileCount;
        modelHookData->textureTileWidth = 1.0f / horizontalTileCount;
        modelHookData->textureTileHeight = 1.0f / verticalTileCount;
    }
    else {
        horizontalTileCount = 1;
        modelHookData->textureTileCount = 1;
        modelHookData->textureTileWidth = 1;
        modelHookData->textureTileHeight = 1;
    }
    modelHookData->textureTiles = HuMemDirectMallocNum(HEAP_MODEL, modelHookData->textureTileCount * sizeof(HuVec2f), model->mallocNo);
    BuildTextureTileCoordinates(modelHookData->textureTiles, modelHookData->textureTileCount, horizontalTileCount, modelHookData->textureTileWidth, modelHookData->textureTileHeight);
    modelHookData->quadVertices[0].x = modelHookData->quadVertices[3].x = modelHookData->quadVertices[2].y = modelHookData->quadVertices[3].y = -particleSize;
    modelHookData->quadVertices[1].x = modelHookData->quadVertices[2].x = modelHookData->quadVertices[0].y = modelHookData->quadVertices[1].y = particleSize;
    modelHookData->quadVertices[0].z = modelHookData->quadVertices[1].z = modelHookData->quadVertices[2].z = modelHookData->quadVertices[3].z = 0;
    displayListSource = HuMemDirectMallocNum(HEAP_MODEL, 0x20000, model->mallocNo);
    displayList = displayListSource;
    DCFlushRange(displayListSource, 0x20000);
    GXBeginDisplayList(displayList, 0x20000);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4 * particleCount);
    for (spriteIndex = 0; spriteIndex < particleCount; spriteIndex++) {
        GXPosition1x16(spriteIndex * 4);
        GXColor1x16(spriteIndex);
        GXTexCoord1x16(spriteIndex * 4);
        GXPosition1x16((spriteIndex * 4) + 1);
        GXColor1x16(spriteIndex);
        GXTexCoord1x16((spriteIndex * 4) + 1);
        GXPosition1x16((spriteIndex * 4) + 2);
        GXColor1x16(spriteIndex);
        GXTexCoord1x16((spriteIndex * 4) + 2);
        GXPosition1x16((spriteIndex * 4) + 3);
        GXColor1x16(spriteIndex);
        GXTexCoord1x16((spriteIndex * 4) + 3);
    }
    modelHookData->displayListSize = GXEndDisplayList();
    modelHookData->displayList = HuMemDirectMallocNum(HEAP_MODEL, modelHookData->displayListSize, model->mallocNo);
    memcpy(modelHookData->displayList, displayListSource, modelHookData->displayListSize);
    DCFlushRange(modelHookData->displayList, modelHookData->displayListSize);
    HuMemDirectFree(displayListSource);
    return modelId;
}

void DestroyParticleModel(s16 modelId)
{
    HU3DMODEL *model;
    ParticleSystemState *spriteData;
    model = &Hu3DData[modelId];
    spriteData = model->hookData;
    HuSprAnimKill(spriteData->animation);
    Hu3DModelKill(modelId);
}

ParticleSystemState *GetParticleModelState(s16 modelId)
{
    HU3DMODEL *model;
    model = &Hu3DData[modelId];
    return model->hookData;
}

void SetParticleModelCallback(s16 modelId, ParticleSystemCallback callback)
{
    HU3DMODEL *model;
    ParticleSystemState *modelHookData;
    model = &Hu3DData[modelId];
    modelHookData = model->hookData;
    modelHookData->renderCallback = callback;
}

void SetParticleModelMode(s16 modelId, u8 mode)
{
    HU3DMODEL *model;
    ParticleSystemState *modelHookData;
    model = &Hu3DData[modelId];
    modelHookData = model->hookData;
    modelHookData->renderMode = mode;
}

void SetParticleTextureCoordinates(HuVec2f *coordinates, HuVec2f *tileCoordinates, s16 tileIndex, float tileWidth, float tileHeight);
void ScaleVectorArray(Vec *sourceVectors, Vec *scaledVectors, Vec *scale, s16 vectorCount);

void TransposeMatrix(Mtx sourceMatrix, Mtx transposedMatrix);

void DrawParticleModel(HU3DMODEL *model, Mtx mtx)
{
    Mtx modelPositionMatrix;
    ROMtx modelRotationMatrix;
    Mtx modelViewMatrix;
    Mtx textureMatrix;
    Vec worldQuadPositions[4];
    Vec localQuadPositions[4];
    Vec transformedQuadPositions[4];
    Vec *vertexPosition;
    ParticleSystemState *modelHookData;
    ParticleState *particle;
    HuVec2f *textureCoordinate;
    s16 particleIndex;
    s16 textureFormat;
    ParticleSystemCallback particleCallback;
    modelHookData = model->hookData;
    GXLoadPosMtxImm(mtx, GX_PNMTX0);
    GXSetNumTevStages(1);
    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    textureFormat = modelHookData->animation->bmp->dataFmt & ANIM_BMP_FMTMASK;
    if (textureFormat == ANIM_BMP_I8 || textureFormat == ANIM_BMP_I4) {
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ONE, GX_CC_RASC, GX_CC_ZERO);
    }
    else {
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
    }
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
    if (model->attr & 0x2) {
        GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    }
    else {
        GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    }
    HuSprTexLoad(modelHookData->animation, 0, 0, GX_REPEAT, GX_REPEAT, GX_LINEAR);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetZCompLoc(GX_FALSE);
    switch (modelHookData->renderMode) {
        case 0:
            GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
            break;

        case 1:
            GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_NOOP);
            break;

        case 2:
            GXSetBlendMode(GX_BM_BLEND, GX_BL_ZERO, GX_BL_INVSRCCLR, GX_LO_NOOP);
            break;
    }
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetArray(GX_VA_POS, modelHookData->vertexPositions, sizeof(Vec));
    GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GXSetArray(GX_VA_CLR0, modelHookData->vertexColors, sizeof(GXColor));
    GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GXSetArray(GX_VA_TEX0, modelHookData->textureCoordinates, sizeof(HuVec2f));
    TransposeMatrix(mtx, modelPositionMatrix);
    PSMTXReorder(modelPositionMatrix, modelRotationMatrix);
    if (!omPauseChk() && modelHookData->renderCallback) {
        particleCallback = modelHookData->renderCallback;
        particleCallback(model, modelHookData, mtx);
    }
    particle = modelHookData->particles;
    vertexPosition = modelHookData->vertexPositions;
    textureCoordinate = modelHookData->textureCoordinates;
    PSMTXROMultVecArray(modelRotationMatrix, modelHookData->quadVertices, worldQuadPositions, 4);
    for (particleIndex = 0; particleIndex < modelHookData->particleCount; particleIndex++, particle++, textureCoordinate += 4) {
        if (particle->isActive == 0) {
            vertexPosition->x = vertexPosition->y = vertexPosition->z = 0;
            vertexPosition++;
            vertexPosition->x = vertexPosition->y = vertexPosition->z = 0;
            vertexPosition++;
            vertexPosition->x = vertexPosition->y = vertexPosition->z = 0;
            vertexPosition++;
            vertexPosition->x = vertexPosition->y = vertexPosition->z = 0;
            vertexPosition++;
        }
        else {
            if (0.0f == particle->rotation) {
                ScaleVectorArray(worldQuadPositions, localQuadPositions, &particle->scale, 4);
                VECAdd(&localQuadPositions[0], &particle->position, vertexPosition++);
                VECAdd(&localQuadPositions[1], &particle->position, vertexPosition++);
                VECAdd(&localQuadPositions[2], &particle->position, vertexPosition++);
                VECAdd(&localQuadPositions[3], &particle->position, vertexPosition++);
            }
            else {
                ScaleVectorArray(modelHookData->quadVertices, localQuadPositions, &particle->scale, 4);
                MTXRotDeg(modelViewMatrix, 'Z', particle->rotation);
                MTXConcat(modelPositionMatrix, modelViewMatrix, textureMatrix);
                MTXMultVecArray(textureMatrix, localQuadPositions, transformedQuadPositions, 4);
                VECAdd(&transformedQuadPositions[0], &particle->position, vertexPosition++);
                VECAdd(&transformedQuadPositions[1], &particle->position, vertexPosition++);
                VECAdd(&transformedQuadPositions[2], &particle->position, vertexPosition++);
                VECAdd(&transformedQuadPositions[3], &particle->position, vertexPosition++);
            }
            if (particle->textureTileIndex != -1) {
                SetParticleTextureCoordinates(textureCoordinate, modelHookData->textureTiles, particle->textureTileIndex, modelHookData->textureTileWidth, modelHookData->textureTileHeight);
            }
        }
    }
    DCFlushRangeNoSync(modelHookData->vertexPositions, (modelHookData->particleCount * sizeof(Vec)) * 4);
    DCFlushRangeNoSync(modelHookData->textureCoordinates, (modelHookData->particleCount * sizeof(HuVec2f)) * 4);
    PPCSync();
    GXCallDisplayList(modelHookData->displayList, modelHookData->displayListSize);
}

void BuildTextureTileCoordinates(HuVec2f *coordinates, s16 tileCount, s16 tilesPerRow, float tileWidth, float tileHeight)
{
    s16 horizontalTile;
    s16 verticalTile;
    s16 textureIndex;
    for (textureIndex = 0; textureIndex < tileCount; textureIndex++, coordinates++) {
        horizontalTile = textureIndex % tilesPerRow;
        verticalTile = textureIndex / tilesPerRow;
        coordinates->x = horizontalTile * tileWidth;
        coordinates->y = verticalTile * tileHeight;
    }
}

void SetParticleTextureCoordinates(HuVec2f *coordinates, HuVec2f *tileCoordinates, s16 tileIndex, float tileWidth, float tileHeight)
{
    coordinates[0].x = tileCoordinates[tileIndex].x;
    coordinates[0].y = tileCoordinates[tileIndex].y;
    coordinates[1].x = tileWidth + tileCoordinates[tileIndex].x;
    coordinates[1].y = tileCoordinates[tileIndex].y;
    coordinates[2].x = tileWidth + tileCoordinates[tileIndex].x;
    coordinates[2].y = tileHeight + tileCoordinates[tileIndex].y;
    coordinates[3].x = tileCoordinates[tileIndex].x;
    coordinates[3].y = tileHeight + tileCoordinates[tileIndex].y;
}

void ScaleVectorArray(Vec *sourceVectors, Vec *scaledVectors, Vec *scale, s16 vectorCount)
{
    s16 vertexIndex;
    for (vertexIndex = 0; vertexIndex < vectorCount; vertexIndex++, sourceVectors++, scaledVectors++) {
        scaledVectors->x = sourceVectors->x * scale->x;
        scaledVectors->y = sourceVectors->y * scale->y;
        scaledVectors->z = sourceVectors->z * scale->z;
    }
}

void TransposeMatrix(Mtx sourceMatrix, Mtx transposedMatrix)
{
    transposedMatrix[0][0] = sourceMatrix[0][0];
    transposedMatrix[1][0] = sourceMatrix[0][1];
    transposedMatrix[2][0] = sourceMatrix[0][2];
    transposedMatrix[0][1] = sourceMatrix[1][0];
    transposedMatrix[1][1] = sourceMatrix[1][1];
    transposedMatrix[2][1] = sourceMatrix[1][2];
    transposedMatrix[0][2] = sourceMatrix[2][0];
    transposedMatrix[1][2] = sourceMatrix[2][1];
    transposedMatrix[2][2] = sourceMatrix[2][2];
    transposedMatrix[0][3] = 0;
    transposedMatrix[1][3] = 0;
    transposedMatrix[2][3] = 0;
}

s32 PlaySoundEffectWithVolume(s32 soundId, s16 vol)
{
    MSM_SEPARAM seParam;
    seParam.flag = MSM_SEPARAM_VOL;
    seParam.vol = vol;
    return msmSeSetParam(soundId, &seParam);
}

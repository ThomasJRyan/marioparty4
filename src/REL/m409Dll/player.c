#include "game/audio.h"
#include "game/chrman.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "ext_math.h"

#include "REL/m409Dll.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#endif

// bss
OMOBJ *playerObjects[4];
OMOBJ *playerManagerObject;
s32 playerReadyFlags[4];
PlayerTargetWork playerTargetRecords[8];
s16 playerSequenceState;
s16 selectedPlayerIndex;
s16 playerSequenceResult;
CursorEffectSystemWork cursorEffectSystem;

// data
u32 playerColorAlphaMask = 0xFFFF0000;
s32 playerMotionAssetIds[7] = { 0x30, 0x2D, 0x3A, 0x2E, DATA_MAKE_NUM(DATADIR_M409, 0x32), 0x17, 0x18 };
GXColor playerCursorColors[8] = { { 0xFF, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0xFF, 0xFF }, { 0xFF, 0x80, 0x80, 0xFF }, { 0x00, 0xFF, 0x00, 0xFF },
    { 0x80, 0x00, 0xC0, 0xFF }, { 0x90, 0x30, 0x00, 0xFF }, { 0xFF, 0xFF, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF } };
HuVec2f playerScreenPositions[4] = {
    { 72.0f, 80.0f },
    { 504.0f, 80.0f },
    { 72.0f, 400.0f },
    { 504.0f, 400.0f },
};
GXColor playerStatusColors[8] = { { 0xD0, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0xFF, 0xFF }, { 0x00, 0xA0, 0x00, 0xFF }, { 0xFF, 0xFF, 0x00, 0xFF },
    { 0xA0, 0x00, 0xFF, 0xFF }, { 0xFF, 0x80, 0x40, 0xFF }, { 0xFF, 0xA0, 0xF0, 0xFF }, { 0x00, 0xFF, 0x40, 0xFF } };

void InitializePlayerObjects(HUPROCESS *process)
{
    s32 playerIndex;
    s32 randomStartCount;
    OMOBJ *object;

    omMakeGroupEx(process, 0, 4);
    omGetGroupMemberListEx(process, 0);

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObjects[playerIndex] = object = omAddObjEx(process, 0x30, 3, 7, 0, CreatePlayerObject);
        object->work[0] = playerIndex;
    }
    playerManagerObject = object = omAddObjEx(process, 0x40, 8, 0xA, 4, UpdatePlayerManager);
    InitializeCursorEffectSystem(0x40, 0x40);
    CreateCursorEffectTexture(DATA_MAKE_NUM(DATADIR_M409, 0x2C), 0x40, 0x40);
    randomStartCount = (u8)frand();

    for (playerIndex = 0; playerIndex < (randomStartCount + 1); playerIndex++) {
        GetRandomPlayerIndex();
    }
}

void CleanupPlayerObjects(void)
{
    CharModelKill(-1);
    CleanupCursorEffectSystem();
}

void CreatePlayerObject(OMOBJ *object)
{
    Vec playerStartPositions[4] = { { -185.0f, 35.0f, 100.0f }, { -62.5f, 35.0f, 100.0f }, { 62.5f, 35.0f, 100.0f }, { 185.0f, 35.0f, 100.0f } };
    s8 characterHookNames[8][32] = { "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r",
        "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r", "test11_tex_we-itemhook-r" };
    M409PlayerWork *playerWork;
    s32 resourceId;
    s32 resourceIndex;

    playerWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M409PlayerWork), HU_MEMNUM_OVL);
    object->data = playerWork;
    playerWork->playerIndex = object->work[0];
    playerWork->characterId = GWPlayerCfg[playerWork->playerIndex].character;
    playerWork->playerPairIndex = (playerWork->playerIndex >> 1);
    playerWork->playerSide = (playerWork->playerIndex & 1);
    playerWork->controllerIndex = GWPlayerCfg[playerWork->playerIndex].pad_idx;
    playerWork->isCpu = GWPlayerCfg[playerWork->playerIndex].iscom;
    playerWork->cpuDifficulty = GWPlayerCfg[playerWork->playerIndex].diff;
    playerWork->motionIndex = 0;
    playerWork->isActive = 1;
    playerWork->isSelected = 0;
    playerWork->randomCursorStyle = GetRandomPlayerIndex() & 0x1;
    playerWork->statusEffectFlags = playerWork->hasTarget = 0;
    playerWork->playerState = -1;
    playerWork->homePosition.x = playerStartPositions[playerWork->playerIndex].x;
    playerWork->homePosition.y = playerStartPositions[playerWork->playerIndex].y;
    playerWork->homePosition.z = playerStartPositions[playerWork->playerIndex].z;
    playerWork->modelRotation.x = playerWork->modelRotation.y = playerWork->modelRotation.z = 0.0f;
    playerWork->inputDirection.y = 180.0f;
    playerWork->inputDirection.x = playerWork->inputDirection.z = 0.0f;
    resourceId = CharModelCreate(playerWork->characterId, 4);
    object->mdlId[0] = resourceId;
    Hu3DModelAttrSet(resourceId, HU3D_MOTATTR_LOOP);

    for (resourceIndex = 0; resourceIndex < 7; resourceIndex++) {
        if ((playerMotionAssetIds[resourceIndex] & 0xFFFF0000) != 0) {
            object->mtnId[resourceIndex] = CharMotionCreate(playerWork->characterId, playerWork->characterId + playerMotionAssetIds[resourceIndex]);
        }
        else {
            object->mtnId[resourceIndex] = CharMotionCreate(playerWork->characterId, playerMotionAssetIds[resourceIndex]);
        }
    }
    CharMotionDataClose(playerWork->characterId);
    CharMotionSet(playerWork->characterId, object->mtnId[playerWork->motionIndex]);
    omSetTra(object, playerWork->homePosition.x, playerWork->homePosition.y, playerWork->homePosition.z);
    Hu3DModelPosSet(resourceId, playerWork->homePosition.x, playerWork->homePosition.y, playerWork->homePosition.z);
    Hu3DModelRotSet(resourceId, 0.0f, 30.0f, 0.0f);
    CharModelStepFxSet(playerWork->characterId, 0);
    object->mdlId[1] = resourceId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M409, 0x12));
    Hu3DModelHookSet(object->mdlId[0], (char *)characterHookNames[playerWork->characterId], object->mdlId[1]);
    resourceId = createCursorEffect(2, 1);
    playerWork->cursorParticleEffectId = resourceId;
    cursorEffectSystem.effects[resourceId].drawCallback = UpdatePlayerCursorEffect;
    CreateCursorEffectSprite(resourceId, 0, DATA_MAKE_NUM(DATADIR_M409, 0x2B), 0x40, 0x40);
    SetCursorEffectColor(
        resourceId, playerCursorColors[playerWork->characterId].r, playerCursorColors[playerWork->characterId].g, playerCursorColors[playerWork->characterId].b, playerCursorColors[playerWork->characterId].a);
    SetCursorEffectColorAtLayer(resourceId, 1, playerCursorColors[playerWork->characterId].r, playerCursorColors[playerWork->characterId].g, playerCursorColors[playerWork->characterId].b, 0x98);
    SetCursorEffectPositionAtLayer(resourceId, 0, playerWork->homePosition.x, 100.0f + playerWork->homePosition.y, -1200.0f);
    SetCursorEffectLayerFlags(resourceId, 0, 0x10);
    SetCursorEffectDepthAtLayer(resourceId, 0, 2.0f, 2.0f, 1.0f);
    SetCursorEffectDepthAtLayer(resourceId, 1, 2.5f, 2.5f, 1.0f);
    SetCursorEffectLayerFlags(resourceId, 1, 0x14);
    SetCursorEffectLayerPriority(resourceId, 1, 1);
    resourceId = createCursorEffect(1, 1);
    SetCursorEffectLayer(resourceId, 0, 0);
    CreateCursorEffectSpriteSimple(cursorEffectSystem.activeEffectId, 0, DATA_MAKE_NUM(DATADIR_M409, 0x2D));
    playerWork->selectionEffectId = resourceId;
    cursorEffectSystem.effects[resourceId].drawCallback = &UpdatePlayerStatusEffect;
    SetCursorEffectLayerFlags(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 4);
    cursorEffectSystem.effects[cursorEffectSystem.activeEffectId].layers[cursorEffectSystem.activeModelId].animationState[0] = playerWork->playerIndex;
    cursorEffectSystem.effects[cursorEffectSystem.activeEffectId].layers[cursorEffectSystem.activeModelId].animationState[1] = -1;
    SetCursorEffectDepthAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 0.5f, 0.5f, 1.0f);
    playerWork->statusEffectId = 0;
    playerWork->cursorState = 0;
    resourceId = espEntry(DATA_MAKE_NUM(DATADIR_M409, 0x31), 1, 0);
    playerWork->statusEffectSpriteId = resourceId;
    espColorSet(resourceId, 0, 0, 0);
    espTPLvlSet(resourceId, 0.5);
    espPosSet(resourceId, playerScreenPositions[playerWork->playerIndex].x, playerScreenPositions[playerWork->playerIndex].y);
    espAttrSet(resourceId, HUSPR_ATTR_DISPOFF);
    resourceId = espEntry(DATA_MAKE_NUM(DATADIR_M409, 0x30), 1, 0);
    playerWork->cursorEffectId = resourceId;
    espPosSet(resourceId, playerScreenPositions[playerWork->playerIndex].x, playerScreenPositions[playerWork->playerIndex].y);
    espAttrSet(resourceId, HUSPR_ATTR_DISPOFF);
    resourceId = espEntry(playerWork->characterId + DATA_MAKE_NUM(DATADIR_MGCONST, 0x00), 1, 0);
    playerWork->characterIconSpriteId = resourceId;
    espPosSet(resourceId, playerScreenPositions[playerWork->playerIndex].x - 27.0f, playerScreenPositions[playerWork->playerIndex].y - 20.0f);
    espAttrSet(resourceId, HUSPR_ATTR_DISPOFF);

    for (resourceIndex = 0; resourceIndex < 3; resourceIndex++) {
        resourceId = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x0A), 0, 0);
        playerWork->scoreDigitSprites[resourceIndex][0] = resourceId;
        espAttrSet(resourceId, HUSPR_ATTR_DISPOFF);
        espPosSet(resourceId, playerScreenPositions[playerWork->playerIndex].x - 32.0f + (resourceIndex * 0x1E), 16.0f + playerScreenPositions[playerWork->playerIndex].y);
        resourceId = espEntry(DATA_MAKE_NUM(DATADIR_MGCONST, 0x0B), 0, 0);
        playerWork->scoreDigitSprites[resourceIndex][1] = resourceId;
        espAttrSet(resourceId, HUSPR_ATTR_DISPOFF);
        espPosSet(resourceId, playerScreenPositions[playerWork->playerIndex].x - 32.0f + (resourceIndex * 0x1E), 16.0f + playerScreenPositions[playerWork->playerIndex].y);
    }
    object->objFunc = UpdatePlayerObject;
}

void UpdatePlayerObject(OMOBJ *object)
{
    s16 buttonsHeld;
    F32 stickMagnitude;
    F32 cursorYAngle;
    F32 cursorXAngle;
    F32 unusedCameraOffset;
    F32 unusedCursorOffset;
    F32 unusedAnimationOffset;
    F32 motionBlendWeight;
    s16 unusedPlayerStateIndex;
    s32 modelId;
    s16 buttonsPressed;
    s16 stickX;
    s16 stickY;
    s16 unusedMotionValueA;
    s16 unusedMotionValueB;
    s16 motionBlendFlags;
    s16 nextMotionIndex;
    s32 scoreDigitIndex;
    s32 playerActionResult;
    CursorEffectLayerWork *cursorLayer;
    M409PlayerWork *playerWork;

    motionBlendWeight = 0.0f;
    playerWork = (M409PlayerWork *)object->data;
    modelId = object->mdlId[0];
    playerWork->previousPosition = playerWork->homePosition;
    nextMotionIndex = playerWork->motionIndex;
    if (playerWork->isCpu != 0) {
        UpdateCpuPlayerInput(object);
    }
    else {
        playerWork->stickX = HuPadStkX[playerWork->controllerIndex];
        playerWork->stickY = HuPadStkY[playerWork->controllerIndex];
        playerWork->buttonsPressed = HuPadBtnDown[playerWork->controllerIndex];
        playerWork->buttonsHeld = HuPadBtn[playerWork->controllerIndex];
    }
    stickX = playerWork->stickX;
    stickY = playerWork->stickY;
    buttonsPressed = playerWork->buttonsPressed;
    buttonsHeld = playerWork->buttonsHeld;
    SetCursorEffectLayer(playerWork->cursorParticleEffectId, 0, 0);
    SetCursorEffectSubLayerPriority(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, cursorEffectSystem.activeLayerId, 2);
    cursorLayer = GetCursorEffectLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId);
    if (getSequenceState(7) != 3) {
        SetCursorEffectFlags(playerWork->cursorParticleEffectId, 4);
    }
    else {
        espAttrReset(playerWork->cursorEffectId, HUSPR_ATTR_DISPOFF);
        espAttrReset(playerWork->statusEffectSpriteId, HUSPR_ATTR_DISPOFF);

        for (scoreDigitIndex = 0; scoreDigitIndex < 3; scoreDigitIndex++) {
            espAttrReset(playerWork->scoreDigitSprites[scoreDigitIndex][0], HUSPR_ATTR_DISPOFF);
        }
        espAttrReset(playerWork->characterIconSpriteId, HUSPR_ATTR_DISPOFF);
        SetCursorEffectAttribute(playerWork->cursorParticleEffectId, 4);
        stickMagnitude = (ABS(stickX) + ABS(stickY));
        cursorXAngle = 0.008f * (stickMagnitude * stickX);
        cursorYAngle = 0.008f * (stickMagnitude * stickY);
        MoveCursorEffectPositionAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, cursorXAngle, cursorYAngle, 0.0f);
    }
    switch (playerSequenceState) {
        case 4:
            if (playerWork->motionIndex == 2) {
                SetCursorEffectSubLayerPriority(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, cursorEffectSystem.activeLayerId, 0);
                if ((buttonsPressed & 0x100) != 0) {
                    playerActionResult = GetPlayerCursorState(playerWork->playerIndex);
                    SetCursorEffectLayer(playerWork->selectionEffectId, 0, 0);
                    SetCursorEffectLayerAttribute(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 4);
                    cursorEffectSystem.effects[cursorEffectSystem.activeEffectId].layers[cursorEffectSystem.activeModelId].animationState[1] = playerActionResult;
                    cursorEffectSystem.effects[cursorEffectSystem.activeEffectId].layers[cursorEffectSystem.activeModelId].animationState[2] = 0;
                    cursorEffectSystem.effects[cursorEffectSystem.activeEffectId].layers[cursorEffectSystem.activeModelId].animationState[3] = 0;
                    SetCursorEffectPositionAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, playerWork->homePosition.x, 100.0f + playerWork->homePosition.y, playerWork->homePosition.z);
                    if (playerActionResult == -1) {
                        cursorEffectSystem.effects[playerWork->selectionEffectId].layers[0].positionDelta.x = cursorLayer->startPosition.x;
                        cursorEffectSystem.effects[playerWork->selectionEffectId].layers[0].positionDelta.y = cursorLayer->startPosition.y;
                        cursorEffectSystem.effects[playerWork->selectionEffectId].layers[0].positionDelta.z = cursorLayer->startPosition.z;
                    }
                }
            }
        case 0:
        case 1:
        case 2:
        case 3:
        case 5:
            if (playerWork->statusEffectId >= 3) {
                playerSequenceResult = playerWork->playerIndex + 1;
            }
            if (playerWork->cursorState == 0) {
                cursorYAngle = atan2d(cursorLayer->startPosition.x - playerWork->homePosition.x, cursorLayer->startPosition.z - playerWork->homePosition.z);
                cursorXAngle = atan2d(playerWork->homePosition.y - cursorLayer->startPosition.y, playerWork->homePosition.z - cursorLayer->startPosition.z);
                cursorXAngle *= (1.0f - (0.015f * ABS(cursorXAngle)));
                playerWork->inputDirection.x = SmoothInterpolationCurve(playerWork->inputDirection.x, cursorXAngle, 0.5f);
                playerWork->inputDirection.y = SmoothInterpolationCurve(playerWork->inputDirection.y, cursorYAngle, 0.5f);
            }
            break;
        case 6:
            SetCursorEffectLayerFlags(playerWork->cursorParticleEffectId, 0, 4);
            Hu3DModelHookReset(modelId);
            Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
            playerActionResult = createCursorEffect(1, 1);
            SetCursorEffectLayer(playerActionResult, 0, 0);
            CreateCursorEffectSpriteSimple(cursorEffectSystem.activeEffectId, 0, DATA_MAKE_NUM(DATADIR_M409, 0x2C));
            SetCursorEffectPositionAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 20.0f + playerWork->homePosition.x, 100.0f + playerWork->homePosition.y, playerWork->homePosition.z - 25.0f);
            SetCursorEffectDepthAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 2.0f, 2.0f, 1.0f);
            SetCursorEffectLayerFlags(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 0x80000000);
            SetCursorEffectColorAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 0xFF, 0xFF, 0xFF, 0xFF);
            SetCursorEffectSubLayerAlpha(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, cursorEffectSystem.activeLayerId, 0x11);
            SetCursorEffectSubLayerOpacity(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, cursorEffectSystem.activeLayerId, 0.5f);
        case 7:
            playerWork->inputDirection.y = SmoothInterpolationCurve(playerWork->inputDirection.y, 0.0f, 0.8f);
            playerWork->inputDirection.x = 0.0f;
            if (getSequenceState(0x80) == 0x80) {
                if (playerWork->statusEffectId >= 3) {
                    nextMotionIndex = 5;
                }
                else {
                    nextMotionIndex = 6;
                }
                motionBlendFlags = 0;
                motionBlendWeight = 0.0f;
            }
            break;
    }
    switch (playerWork->motionIndex) {
        case 0:
            if ((playerSequenceState != 0) && (playerSequenceState < 6)) {
                nextMotionIndex = 1;
                motionBlendFlags = 0;
                motionBlendWeight = 0.0f;
                playerWork->targetRecordIndex = (s8)(8.0f * (GetRandomPlayerIndex() / 65536.0f));
            }
            break;
        case 2:
            if (playerSequenceState == 0) {
                nextMotionIndex = 1;
                motionBlendFlags = 4;
                motionBlendWeight = GetPlayerRank(object->mtnId[1]);
                playerReadyFlags[object->work[0]] = 1;
            }
            if ((playerSequenceState == 4) && ((buttonsPressed & 0x100) != 0)) {
                HuAudFXPlay(0x556);
                nextMotionIndex = 3;
                motionBlendFlags = 0;
                motionBlendWeight = 0.0f;
            }
            break;
        case 1:
            if (CharMotionEndCheck(playerWork->characterId) != 0) {
                if (CharMotionTimeGet(playerWork->characterId) <= 0.0f) {
                    nextMotionIndex = 0;
                }
                else {
                    nextMotionIndex = 2;
                }
                motionBlendFlags = 1;
                motionBlendWeight = 0.0f;
            }
            break;
        case 3:
            if (playerWork->cursorState != 0) {
                playerWork->inputDirection.x = SmoothInterpolationCurve(playerWork->inputDirection.x, 0.0f, 0.8f);
                playerWork->inputDirection.y = SmoothInterpolationCurve(playerWork->inputDirection.y, 0.0f, 0.8f);
                if (ABS(playerWork->inputDirection.y) < 10.0f) {
                    playerWork->inputDirection.y = 0.0f;
                    nextMotionIndex = 4;
                    motionBlendFlags = 0;
                    motionBlendWeight = 0.0f;
                }
            }
            else if (CharMotionEndCheck(playerWork->characterId) != 0) {
                nextMotionIndex = 2;
                motionBlendFlags = 1;
                motionBlendWeight = 0.0f;
            }
            break;
        case 4:
            if (playerWork->playerState < 0) {
                playerWork->playerState = CharFXPlay(playerWork->characterId, 0x125);
            }
            if (CharMotionEndCheck(playerWork->characterId) != 0) {
                playerWork->inputDirection.y = SmoothInterpolationCurve(playerWork->inputDirection.y, 180.0f, 0.8f);
                if (ABS(playerWork->inputDirection.y) > 170.0f) {
                    nextMotionIndex = 2;
                    motionBlendFlags = 1;
                    motionBlendWeight = 0.0f;
                    playerWork->cursorState = 0;
                    playerWork->playerState = -1;
                }
            }
            else {
                playerWork->inputDirection.y = 0.0f;
            }
            break;
        case 5:
        case 6:
            break;
    }
    if (nextMotionIndex != playerWork->motionIndex) {
        playerWork->motionIndex = nextMotionIndex;
        CharMotionShiftSet(playerWork->characterId, object->mtnId[playerWork->motionIndex], motionBlendWeight, 8.0f, motionBlendFlags);
    }
    omSetTra(object, playerWork->homePosition.x, playerWork->homePosition.y, playerWork->homePosition.z);
    omSetRot(object, playerWork->inputDirection.x, playerWork->inputDirection.y, playerWork->inputDirection.z);
}

void UpdateCpuPlayerInput(OMOBJ *object)
{
    Vec targetPosition;
    Vec targetDelta;
    s32 playerModelId;
    F32 unusedDistanceOutput;
    s16 previousMotionIndex;
    PlayerTargetWork *targetWork;
    CursorEffectLayerWork *cursorLayer;
    F32 randomOffsetX;
    F32 randomOffsetY;
    F32 randomOffsetZ;
    F32 targetTolerance;
    F32 targetDistanceSquared;
    F32 unusedDistanceEstimate;
    s32 targetIndex;
    M409PlayerWork *playerWork;

    playerWork = (M409PlayerWork *)object->data;
    playerModelId = object->mdlId[0];
    previousMotionIndex = playerWork->motionIndex;
    playerWork->buttonsPressed = playerWork->buttonsHeld = 0;
    cursorLayer = GetCursorEffectLayer(playerWork->cursorParticleEffectId, 0);
    targetPosition.x = targetPosition.y = targetPosition.z = 0.0f;
    switch (playerSequenceState) { /* irregular */
        case 2:
            if (playerWork->randomCursorStyle == 0) {
                targetWork = &playerTargetRecords[0];

                for (targetIndex = 0; targetIndex < 8; targetIndex++, targetWork++) {
                    if (targetWork->targetState == 0)
                        break;
                    targetPosition.x += targetWork->targetPosition.x;
                    targetPosition.y += targetWork->targetPosition.y;
                    targetPosition.z += targetWork->targetPosition.z;
                }
                targetPosition.x /= targetIndex;
                targetPosition.y /= targetIndex;
                targetPosition.z /= targetIndex;
            }
            else {
                targetWork = &playerTargetRecords[playerWork->targetRecordIndex];
                if (targetWork->targetState != 0) {
                    targetPosition.x = targetWork->targetPosition.x;
                    targetPosition.y = targetWork->targetPosition.y;
                    targetPosition.z = targetWork->targetPosition.z;
                }
                else {
                    targetPosition.x = 0.0f;
                    targetPosition.y = 800.0f;
                    targetPosition.z = -1200.0f;
                }
            }
            break;
        case 3:
            playerWork->targetRecordIndex = 0xFF;
            playerWork->statusEffectFlags = (0x28 - (playerWork->cpuDifficulty * 10));
        case 4:
            targetPosition.x = cursorLayer->startPosition.x;
            targetPosition.y = cursorLayer->startPosition.y;
            targetPosition.z = cursorLayer->startPosition.z;
            if (playerWork->statusEffectFlags != 0) {
                playerWork->statusEffectFlags--;
            }
            if ((playerWork->statusEffectFlags == 0)) {
                if (((s32)((0x30 - (playerWork->cpuDifficulty * 8)) * (GetRandomPlayerIndex() / 65536.0f)) == 0)) {
                    playerWork->targetRecordIndex = selectedPlayerIndex;
                }
            }
            if (playerWork->targetRecordIndex != 0xFF) {
                targetWork = &playerTargetRecords[playerWork->targetRecordIndex];
                targetPosition.x = targetWork->targetPosition.x;
                targetPosition.y = targetWork->targetPosition.y;
                targetPosition.z = targetWork->targetPosition.z;
                targetTolerance = 100.0f * (0.5f * (4 - playerWork->cpuDifficulty));
                targetTolerance = 20.0f + (targetTolerance * (GetRandomPlayerIndex() / 65536.0f));
                targetDistanceSquared = ((targetWork->targetPosition.z - cursorLayer->startPosition.z) * (targetWork->targetPosition.z - cursorLayer->startPosition.z))
                    + (((targetWork->targetPosition.x - cursorLayer->startPosition.x) * (targetWork->targetPosition.x - cursorLayer->startPosition.x))
                        + ((targetWork->targetPosition.y - cursorLayer->startPosition.y) * (targetWork->targetPosition.y - cursorLayer->startPosition.y)));
                if ((sqrtf(targetDistanceSquared) < targetTolerance) && ((s32)((0x20 - (playerWork->cpuDifficulty * 8)) * (GetRandomPlayerIndex() / 65536.0f)) == 0)) {
                    playerWork->buttonsPressed = (playerWork->buttonsPressed | 0x100);
                }
            }
            break;
        default:
            targetPosition.x = 0.0f;
            targetPosition.y = 800.0f;
            targetPosition.z = -1200.0f;
            break;
    }
    if (playerWork->hasTarget == 0) {
        targetTolerance = 100.0f * (0.5f * (4 - playerWork->cpuDifficulty));
        targetPosition.x += (targetTolerance * ((GetRandomPlayerIndex() - 0x8000) / 32768.0f));
        targetPosition.y += (targetTolerance * ((GetRandomPlayerIndex() - 0x8000) / 32768.0f));
        playerWork->playerPosition.x = targetPosition.x;
        playerWork->playerPosition.y = targetPosition.y;
        playerWork->playerPosition.z = targetPosition.z;
        targetDelta.x = playerWork->playerPosition.x - cursorLayer->startPosition.x;
        targetDelta.y = playerWork->playerPosition.y - cursorLayer->startPosition.y;
        targetDelta.z = playerWork->playerPosition.z - cursorLayer->startPosition.z;
        VectorLength(&targetDelta);
        playerWork->stickX = (targetDelta.x * ((playerWork->cpuDifficulty * 6) + 0x18));
        playerWork->stickY = (targetDelta.y * ((playerWork->cpuDifficulty * 6) + 0x18));
        playerWork->hasTarget = ((s32)(((4 - playerWork->cpuDifficulty) * 8) * (GetRandomPlayerIndex() / 65536.0f)) + 8);
    }
    playerWork->hasTarget--;
}

void UpdatePlayerCursorEffect(CursorEffectWork *effect)
{
    Vec positionDelta;
    Vec cursorPosition;
    F32 targetSearchRadius;
    F32 cosineWeight;
    F32 sineWeight;
    CursorEffectLayerWork *cursorLayer;
    Vec *cursorPositionPointer;
    s32 targetIndex;
    PlayerTargetWork *targetWork;
    CursorEffectLayerWork *targetMarkerLayer;

    cursorLayer = &effect->layers[0];
    if (cursorLayer->startPosition.x < -600.0f) {
        cursorLayer->startPosition.x = -600.0f;
    }
    if (cursorLayer->startPosition.x > 600.0f) {
        cursorLayer->startPosition.x = 600.0f;
    }
    if (cursorLayer->startPosition.y < 350.0f) {
        cursorLayer->startPosition.y = 350.0f;
    }
    if (cursorLayer->startPosition.y > 1200.0f) {
        cursorLayer->startPosition.y = 1200.0f;
    }
    targetSearchRadius = 75.0f;
    cursorPosition = cursorLayer->startPosition;
    cursorPositionPointer = &cursorPosition;
    targetIndex = FindNearestTarget(cursorPositionPointer, &targetSearchRadius);
    targetMarkerLayer = &effect->layers[1];
    if (targetIndex == -1) {
        if (targetMarkerLayer->animationState[0] != -1) {
            targetMarkerLayer->animationState[0] = -1;
            targetMarkerLayer->scaleX = 0.0f;
            targetMarkerLayer->positionDelta.x = targetMarkerLayer->startPosition.x;
            targetMarkerLayer->positionDelta.y = targetMarkerLayer->startPosition.y;
            targetMarkerLayer->positionDelta.z = targetMarkerLayer->startPosition.z;
            targetMarkerLayer->scaleY = targetMarkerLayer->scale.x;
        }
        if (targetMarkerLayer->scaleX < 90.0f) {
            targetMarkerLayer->scaleX += 9.0f;
            if (targetMarkerLayer->scaleX > 90.0f) {
                targetMarkerLayer->scaleX = 90.0f;
            }
            sineWeight = sind(targetMarkerLayer->scaleX);
            cosineWeight = cosd(targetMarkerLayer->scaleX);
            positionDelta.x = cursorLayer->startPosition.x - targetMarkerLayer->positionDelta.x;
            positionDelta.y = cursorLayer->startPosition.y - targetMarkerLayer->positionDelta.y;
            positionDelta.z = cursorLayer->startPosition.z - targetMarkerLayer->positionDelta.z;
            targetMarkerLayer->startPosition.x = (targetMarkerLayer->positionDelta.x + (positionDelta.x * sineWeight));
            targetMarkerLayer->startPosition.y = (targetMarkerLayer->positionDelta.y + (positionDelta.y * sineWeight));
            targetMarkerLayer->startPosition.z = (targetMarkerLayer->positionDelta.z + (positionDelta.z * sineWeight));
            targetMarkerLayer->scale.x = (0.001f + (targetMarkerLayer->scaleY * cosineWeight));
            targetMarkerLayer->scale.y = (0.001f + (targetMarkerLayer->scaleY * cosineWeight));
            targetMarkerLayer->scale.z = 1.0f;
            return;
        }
        targetMarkerLayer->layerAttributes |= 4;
        targetMarkerLayer->startPosition.x = cursorLayer->startPosition.x;
        targetMarkerLayer->startPosition.y = cursorLayer->startPosition.y;
        targetMarkerLayer->startPosition.z = cursorLayer->startPosition.z;
        return;
    }
    targetWork = &playerTargetRecords[targetIndex];
    if (targetMarkerLayer->animationState[0] != targetIndex) {
        targetMarkerLayer->animationState[0] = targetIndex;
        targetMarkerLayer->scaleX = 0.0f;
        targetMarkerLayer->positionDelta.x = targetMarkerLayer->startPosition.x;
        targetMarkerLayer->positionDelta.y = targetMarkerLayer->startPosition.y;
        targetMarkerLayer->positionDelta.z = targetMarkerLayer->startPosition.z;
        targetMarkerLayer->scaleY = targetMarkerLayer->scale.x;
        targetMarkerLayer->scaleZ = (2.5f - targetMarkerLayer->scale.x);
    }
    if (targetMarkerLayer->scaleX < 90.0f) {
        targetMarkerLayer->scaleX += 9.0f;
        if (targetMarkerLayer->scaleX > 90.0f) {
            targetMarkerLayer->scaleX = 90.0f;
        }
        sineWeight = sind(targetMarkerLayer->scaleX);
        positionDelta.x = targetWork->targetPosition.x - targetMarkerLayer->positionDelta.x;
        positionDelta.y = targetWork->targetPosition.y - targetMarkerLayer->positionDelta.y;
        positionDelta.z = targetWork->targetPosition.z - targetMarkerLayer->positionDelta.z;
        targetMarkerLayer->startPosition.x = (targetMarkerLayer->positionDelta.x + (positionDelta.x * sineWeight));
        targetMarkerLayer->startPosition.y = (targetMarkerLayer->positionDelta.y + (positionDelta.y * sineWeight));
        targetMarkerLayer->startPosition.z = (targetMarkerLayer->positionDelta.z + (positionDelta.z * sineWeight));
        targetMarkerLayer->scale.x = (targetMarkerLayer->scaleY + (targetMarkerLayer->scaleZ * sineWeight));
        targetMarkerLayer->scale.y = (targetMarkerLayer->scaleY + (targetMarkerLayer->scaleZ * sineWeight));
        targetMarkerLayer->scale.z = 1.0f;
    }
    else {
        targetMarkerLayer->startPosition.x = targetWork->targetPosition.x;
        targetMarkerLayer->startPosition.y = targetWork->targetPosition.y;
        targetMarkerLayer->startPosition.z = targetWork->targetPosition.z;
    }
    targetMarkerLayer->layerAttributes &= ~4;
}

s8 GetPlayerCursorState(u8 playerIndex)
{
    M409PlayerWork *data;
    CursorEffectLayerWork *cursorLayer;

    data = (M409PlayerWork *)playerObjects[playerIndex]->data;
    cursorLayer = GetCursorEffectLayer(data->cursorParticleEffectId, 1);
    return cursorLayer->animationState[0];
}

void UpdatePlayerStatusEffect(CursorEffectWork *effect)
{
    Vec collisionOrigin;
    Vec closestPosition;
    Vec targetSegmentDelta;
    s32 targetIndex;
    s32 targetScanIndex;
    F32 segmentDistance;
    PlayerTargetWork *targetWork;
    M409PlayerWork *playerWork;
    CursorEffectLayerWork *statusEffectLayer;

    statusEffectLayer = &effect->layers[0];
    if ((statusEffectLayer->layerAttributes & 4) == 0) {
        targetIndex = statusEffectLayer->animationState[1];
        if (statusEffectLayer->animationState[2] == 0) {
            if (targetIndex == -1) {
                targetSegmentDelta.x = statusEffectLayer->positionDelta.x - statusEffectLayer->startPosition.x;
                targetSegmentDelta.y = statusEffectLayer->positionDelta.y - statusEffectLayer->startPosition.y;
                targetSegmentDelta.z = statusEffectLayer->positionDelta.z - statusEffectLayer->startPosition.z;
                VectorLength(&targetSegmentDelta);
                statusEffectLayer->scaleDelta.x = targetSegmentDelta.x;
                statusEffectLayer->scaleDelta.y = targetSegmentDelta.y;
                statusEffectLayer->scaleDelta.z = targetSegmentDelta.z;
            }
            statusEffectLayer->animationState[2] = 0x10;
        }
        if (statusEffectLayer->animationState[3] == 0) {
            if (targetIndex != -1) {
                targetWork = &playerTargetRecords[targetIndex];
                if ((targetWork->targetState != 0) && (targetWork->targetState <= 4)) {
                    targetSegmentDelta.x = targetWork->targetPosition.x - statusEffectLayer->startPosition.x;
                    targetSegmentDelta.y = targetWork->targetPosition.y - statusEffectLayer->startPosition.y;
                    targetSegmentDelta.z = targetWork->targetPosition.z - statusEffectLayer->startPosition.z;
                    VectorLength(&targetSegmentDelta);
                    statusEffectLayer->scaleDelta.x = targetSegmentDelta.x;
                    statusEffectLayer->scaleDelta.y = targetSegmentDelta.y;
                    statusEffectLayer->scaleDelta.z = targetSegmentDelta.z;
                }
            }
            targetSegmentDelta.x = 100.0f * (2.0f * statusEffectLayer->scaleDelta.x);
            targetSegmentDelta.y = 100.0f * (2.0f * statusEffectLayer->scaleDelta.y);
            targetSegmentDelta.z = 100.0f * (2.0f * statusEffectLayer->scaleDelta.z);
            for (targetWork = &playerTargetRecords[0], targetScanIndex = 0; targetScanIndex < 8; targetScanIndex++, targetWork++) {
                if ((Hu3DModelAttrGet(targetWork->targetModelId) & 1) == 0) {
                    segmentDistance = DistanceBetweenSegments(&targetWork->targetPosition, &statusEffectLayer->startPosition, &targetSegmentDelta);
                    if (!(segmentDistance >= 2250.0f) && (targetWork->targetState == 3)) {
                        targetWork->targetState = 4;
                        statusEffectLayer->layerAttributes |= 4;
                        playerWork = (M409PlayerWork *)playerObjects[statusEffectLayer->animationState[0]]->data;
                        if (targetWork->targetReferencePosition == selectedPlayerIndex) {
                            espAttrSet(playerWork->scoreDigitSprites[playerWork->statusEffectId][0], HUSPR_ATTR_DISPOFF);
                            espAttrReset(playerWork->scoreDigitSprites[playerWork->statusEffectId][1], HUSPR_ATTR_DISPOFF);
                            HuAudFXPlay(9);
                            playerWork->statusEffectId++;
                            omVibrate(playerWork->playerIndex, 0xC, 6, 6);
                            if (playerWork->statusEffectId < 3) {
                                playerWork->cursorState++;
                            }
                        }
                        else if (playerWork->statusEffectId != 0) {
                            espAttrReset(playerWork->scoreDigitSprites[playerWork->statusEffectId][0], HUSPR_ATTR_DISPOFF);
                            espAttrSet(playerWork->scoreDigitSprites[playerWork->statusEffectId][1], HUSPR_ATTR_DISPOFF);
                        }
                    }
                }
            }
            collisionOrigin.x = -290.0f;
            collisionOrigin.y = 350.0f;
            collisionOrigin.z = -1200.0f;
            segmentDistance = ClosestPointsBetweenSegments(&collisionOrigin, &statusEffectLayer->startPosition, &targetSegmentDelta, &closestPosition, &targetSegmentDelta);
            if (segmentDistance <= 100.0f) {
                statusEffectLayer->startPosition.x = closestPosition.x;
                statusEffectLayer->startPosition.y = closestPosition.y;
                statusEffectLayer->startPosition.z = closestPosition.z;
                statusEffectLayer->scaleDelta.x = targetSegmentDelta.x;
                statusEffectLayer->scaleDelta.y = targetSegmentDelta.y;
                statusEffectLayer->scaleDelta.z = targetSegmentDelta.z;
                statusEffectLayer->animationState[3] = 1;
                return;
            }
        }
        statusEffectLayer->startPosition.x = (statusEffectLayer->startPosition.x + (200.0f * statusEffectLayer->scaleDelta.x));
        statusEffectLayer->startPosition.y = (statusEffectLayer->startPosition.y + (200.0f * statusEffectLayer->scaleDelta.y));
        statusEffectLayer->startPosition.z = (statusEffectLayer->startPosition.z + (200.0f * statusEffectLayer->scaleDelta.z));
        if (--statusEffectLayer->animationState[2] == 0) {
            statusEffectLayer->layerAttributes |= 4;
        }
    }
}

void UpdatePlayerManager(OMOBJ *object)
{
    s32 resourceId;
    s32 targetIndex;
    PlayerTargetWork *targetWork;

    playerSequenceState = 0;
    resourceId = Hu3DModelCreateFile(0x20000);
    object->mdlId[0] = resourceId;
    Hu3DModelAttrSet(resourceId, HU3D_MOTATTR_LOOP);

    for (targetIndex = 0; targetIndex < 3; targetIndex++) {
        object->mtnId[targetIndex]
            = Hu3DJointMotion(resourceId, HuDataSelHeapReadNum(targetIndex + DATA_MAKE_NUM(DATADIR_M409, 0x0F), HU_MEMNUM_OVL, HEAP_MODEL));
    }
    Hu3DMotionSet(resourceId, object->mtnId[0]);
    omSetTra(object, 419.99997f, 250.0f, -180.0f);
    omSetRot(object, 0.0f, -30.0f, 0.0f);

    for (targetIndex = 0; targetIndex < 4; targetIndex++) {
        object->work[targetIndex] = 0;
    }

    for (targetIndex = 0; targetIndex < 8; targetIndex++) {
        resourceId = Hu3DModelCreateFile(targetIndex + DATA_MAKE_NUM(DATADIR_M409, 0x1B));
        object->mdlId[targetIndex + 1] = resourceId;
        Hu3DModelAttrSet(resourceId, HU3D_ATTR_DISPOFF);
    }
    playerSequenceResult = 0;

    for (targetWork = playerTargetRecords, targetIndex = 0; targetIndex < 8; targetIndex++, targetWork++) {
        resourceId = Hu3DModelCreateFile(targetIndex + DATA_MAKE_NUM(DATADIR_M409, 0x13));
        targetWork->targetModelId = resourceId;
        Hu3DModelLayerSet(resourceId, 3);
        targetWork->targetPosition.x = -400.0f + (100.0f * targetIndex);
        targetWork->targetPosition.y = 300.0f + (100.0f * targetIndex);
        targetWork->targetPosition.z = -1200.0f;
        Hu3DModelPosSet(resourceId, targetWork->targetPosition.x, targetWork->targetPosition.y, targetWork->targetPosition.z);
        targetWork->targetState = targetWork->targetStateFlags = 0;
        targetWork->targetHeading.x = targetWork->targetHeading.y = targetWork->targetHeading.z = 0.0f;
        targetWork->targetVelocity.x = targetWork->targetVelocity.y = targetWork->targetVelocity.z = 1.0f;
        targetWork->targetDestination.x = targetWork->targetDestination.y = targetWork->targetDestination.z = 0.0f;
        targetWork->targetOrigin.x = targetWork->targetPosition.x;
        targetWork->targetOrigin.y = targetWork->targetPosition.y;
        targetWork->targetOrigin.z = targetWork->targetPosition.z;
        targetWork->targetReferencePosition = targetIndex;
        SetCursorEffectLayer((targetWork->cursorEffectId = createCursorEffect(1, 1)), 0, 0);
        CreateCursorEffectSprite(cursorEffectSystem.activeEffectId, 0, targetIndex + DATA_MAKE_NUM(DATADIR_M409, 0x23), 0x40, 0x40);
        SetCursorEffectDepthAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 2.0f, 2.0f, 1.0f);
        SetCursorEffectColorAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 0xFF, 0xFF, 0xFF, 0xA0);
        SetCursorEffectLayerFlags(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 0x40000004);
        SetCursorEffectSubLayerAlpha(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, cursorEffectSystem.activeLayerId, 0x11);
        SetCursorEffectSubLayerOpacity(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, cursorEffectSystem.activeLayerId, 1.0f);
    }
    object->objFunc = UpdatePlayerStatus;
}

void UpdatePlayerStatus(OMOBJ *object)
{
    s32 targetOrder[8];
    PlayerTargetWork *targetWork;
    s32 targetIndex;
    s32 placementRank;
    s32 activeTargetCount;
    s32 effectId;
    F32 targetRotationSine;

    activeTargetCount = 0;
    targetWork = &playerTargetRecords[0];

    switch (playerSequenceState) {
        case 0:
            if (getSequenceState(0x18) == 8) {
                playerSequenceState++;
            }
            for (targetIndex = 0; targetIndex < 4; targetIndex++) {
                if (playerReadyFlags[targetIndex] == 0)
                    break;
            }
            if (targetIndex < 4)
                break;
            if (playerSequenceResult == 0)
                break;

            playerSequenceState = 6;
            break;
        case 1:
            object->work[0] = (s32)((GetRandomPlayerIndex() / 65536.0f) * 3.0f) + 6;
            object->work[1] = selectedPlayerIndex = (u16)((GetRandomPlayerIndex() / 65536.0f) * *object->work);
            object->work[2]++;
            if (object->work[2] >= 12) {
                playerSequenceResult = -1;
            }
            object->work[3] = 0;
            for (targetIndex = 0; targetIndex < object->work[0]; targetIndex++, targetWork++) {
                targetWork->targetState = 1;
            }
            playerSequenceState++;
            break;
        case 3:
            Hu3DModelAttrReset(effectId = object->mdlId[selectedPlayerIndex + 1], HU3D_ATTR_DISPOFF);
            Hu3DModelHookSet(object->mdlId[0], "g001mk-itemhook_r", effectId);
            Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[1], 0.0f, 8.0f, HU3D_MOTATTR_NONE);
            HuAudFXPlay(0x558);
            playerSequenceState++;
            for (targetIndex = 0; targetIndex < 4; targetIndex++) {
                playerReadyFlags[targetIndex] = 0;
            }
            SetSequenceStatePair(0x18, 0x10);
            break;
        case 5:
            if (object->work[3] == 0) {
                ClampIntegerRange(targetOrder, object->work[0]);
                for (targetIndex = 0, placementRank = 1; targetIndex < object->work[0]; targetIndex++) {
                    targetWork = &playerTargetRecords[targetOrder[targetIndex]];
                    if (targetWork->targetState == 3) {
                        targetWork->targetState = 4;
                        targetWork->targetFlags = placementRank * 10;
                        placementRank++;
                    }
                }
                object->work[3] = placementRank * 10 + 10;
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[2], 0.0f, 8.0f, HU3D_MOTATTR_NONE);
                if (playerSequenceResult == 0) {
                    AdvanceGameSequence();
                }
                break;
            }
            if (--object->work[3] == 0) {
                playerSequenceState = 0;
                Hu3DModelHookReset(object->mdlId[0]);
                Hu3DMotionShiftSet(object->mdlId[0], object->mtnId[0], 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
                Hu3DModelAttrSet(object->mdlId[selectedPlayerIndex + 1], HU3D_ATTR_DISPOFF);
            }
            break;
        case 6:
            SetSequenceStatePair(7, 4);
            SetSequenceState(playerSequenceResult - 1);
            playerSequenceState++;
            break;
        case 7:
            break;
    }

    targetWork = &playerTargetRecords[0];
    for (targetIndex = 0; targetIndex < 8; targetIndex++, targetWork++) {
        SetCursorEffectLayer(targetWork->cursorEffectId, 0, 0);

        switch (targetWork->targetState) {
            case 0:
                Hu3DModelAttrSet(targetWork->targetModelId, HU3D_ATTR_DISPOFF);
                targetWork->targetPosition.x = targetWork->targetPosition.y = targetWork->targetPosition.z = 0.0f;
                continue;
            case 1:
                targetWork->targetPosition.x = ((GetRandomPlayerIndex() - 0x8000) / 32768.0f) * 150.0f;
                targetWork->targetPosition.y = (GetRandomPlayerIndex() / 65536.0f) * 100.0f;
                targetWork->targetPosition.z = -1200.0f;
                Hu3DModelPosSet(targetWork->targetModelId, targetWork->targetPosition.x, targetWork->targetPosition.y, targetWork->targetPosition.z);
                Hu3DModelAttrSet(targetWork->targetModelId, HU3D_ATTR_DISPOFF);
                SetCursorEffectDepthAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 2.0f, 2.0f, 1.0f);
                targetWork->targetHeading.z = ((GetRandomPlayerIndex() - 0x8000) / 32768.0f) * 60.0f;
                targetWork->targetFlags = ((100.0f - targetWork->targetPosition.y) * 0.5f);
                targetWork->targetEffectId = 100.0f * ((((GetRandomPlayerIndex() / 65536.0f) * 0.006f) + 0.0128f) + (0.000035f * targetWork->targetFlags));
                targetWork->targetRotationAngle = ((GetRandomPlayerIndex() - 0x8000) / 32768.0f) * 360.0f;
                targetWork->targetState++;
            case 2:
                if (--targetWork->targetFlags != 0)
                    break;
                targetWork->targetFlags = 1;
                targetWork->targetState++;
            case 3:
                if (targetWork->targetHeading.z < 0.0f) {
                    targetWork->targetHeading.z += 0.08f;
                    if (targetWork->targetHeading.z >= 0.0f) {
                        targetWork->targetHeading.z = 0.0f;
                    }
                }
                else {
                    targetWork->targetHeading.z -= 0.08f;
                    if (targetWork->targetHeading.z <= 0.0f) {
                        targetWork->targetHeading.z = 0.0f;
                    }
                }
                if (targetWork->targetPosition.y >= 375.0f) {
                    targetWork->targetEffectId -= 0.000001f * targetWork->targetPosition.y;
                }
                targetWork->targetPosition.x += sind(targetWork->targetHeading.z) * targetWork->targetEffectId;
                targetWork->targetPosition.y += cosd(targetWork->targetHeading.z) * targetWork->targetEffectId;
                if (targetWork->targetPosition.y >= 200.0) {
                    Hu3DModelAttrReset(targetWork->targetModelId, HU3D_ATTR_DISPOFF);
                }
                if (targetWork->targetPosition.x >= 550.0f) {
                    targetWork->targetHeading.z = -((GetRandomPlayerIndex() / 65536.0f) * 20.0f);
                }
                if (targetWork->targetPosition.x <= -550.0f) {
                    targetWork->targetHeading.z = ((GetRandomPlayerIndex() / 65536.0f) * 20.0f);
                }
                if (targetWork->targetPosition.y >= 1250.0f) {
                    targetWork->targetState = 4;
                }
                break;
            case 4:
                if (--targetWork->targetFlags != 0)
                    break;
                Hu3DModelAttrSet(targetWork->targetModelId, HU3D_ATTR_DISPOFF);
                SetCursorEffectPositionAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, targetWork->targetPosition.x, targetWork->targetPosition.y, targetWork->targetPosition.z);
                SetCursorEffectLayerAttribute(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 4);
                if (selectedPlayerIndex == targetWork->targetReferencePosition) {
                    HuAudFXPlay(0x557);
                }
                else {
                    HuAudFXPlay(0x55A);
                }
                if (targetWork->targetReferencePosition == object->work[1]) {
                    playerSequenceState = 5;
                }
                effectId = createCursorEffect(1, 1);
                SetCursorEffectLayer(effectId, 0, 0);
                CreateCursorEffectSpriteSimple(cursorEffectSystem.activeEffectId, 0, DATA_MAKE_NUM(DATADIR_M409, 0x2C));
                SetCursorEffectPositionAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, targetWork->targetPosition.x, targetWork->targetPosition.y, targetWork->targetPosition.z);
                SetCursorEffectDepthAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 4.0f, 4.0f, 1.0f);
                SetCursorEffectLayerFlags(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 0x80000000);
                SetCursorEffectColorAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, 0xFF, 0xFF, 0xFF, 0x60);
                SetCursorEffectSubLayerAlpha(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, cursorEffectSystem.activeLayerId, 0x11);
                SetCursorEffectSubLayerOpacity(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, cursorEffectSystem.activeLayerId, 0.5f);
                targetWork->targetState++;
                break;
            case 5:
                targetWork->targetFlags++;
                SetCursorEffectDepthAtLayer(cursorEffectSystem.activeEffectId, cursorEffectSystem.activeModelId, (targetWork->targetFlags * 0.1f) + 2.0f, (targetWork->targetFlags * 0.1f) + 2.0f, 1.0f);
                targetWork->targetPosition.x = targetWork->targetPosition.y = targetWork->targetPosition.z = 0.0f;
                break;
        }
        if (targetWork->targetPosition.y <= 375.0f) {
            if (targetWork->targetPosition.x >= 150.0f) {
                targetWork->targetPosition.x = 150.0f;
            }
            if (targetWork->targetPosition.x <= -150.0f) {
                targetWork->targetPosition.x = -150.0f;
            }
        }
        else {
            activeTargetCount++;
        }

        UpdatePlayerTarget(targetWork);
        Hu3DModelPosSet(targetWork->targetModelId, targetWork->targetPosition.x, targetWork->targetPosition.y, targetWork->targetPosition.z);
        targetRotationSine = sind(targetWork->targetRotationAngle);
        targetWork->targetRotationAngle += 3.0f;
        Hu3DModelRotSet(
            targetWork->targetModelId, 8.0 * sind(targetWork->targetRotationAngle * 0.3), targetWork->targetRotationAngle * 0.05f, (-targetWork->targetHeading.z * 0.25f) + (targetRotationSine * 8.0f));
    }

    if (playerSequenceState == 2 && activeTargetCount >= object->work[0]) {
        playerSequenceState++;
    }
}

s32 UpdatePlayerTarget(PlayerTargetWork *target)
{
    s32 nearbyTargetIndices[8];
    Vec separationOffset;
    Vec targetDelta;
    F32 minimumSeparation;
    F32 targetDistance;
    F32 targetHeadingAngle;
    s32 nearbyTargetCount;
    s32 targetIndex;
    PlayerTargetWork *otherTarget;

    if ((target->targetState == 0) || (target->targetState >= 4U)) {
        return 0;
    }
    minimumSeparation = 100.0f;
    nearbyTargetCount = 0;
    separationOffset.x = separationOffset.y = separationOffset.z = 0.0f;
    otherTarget = playerTargetRecords;

    for (targetIndex = 0; targetIndex < 8; targetIndex++, otherTarget++) {
        if ((otherTarget->targetState != 0) && (otherTarget->targetState < 4U) && (otherTarget != target)) {
            targetDelta.x = target->targetPosition.x - otherTarget->targetPosition.x;
            targetDelta.y = target->targetPosition.y - otherTarget->targetPosition.y;
            targetDelta.z = target->targetPosition.z - otherTarget->targetPosition.z;
            targetDistance = VectorLength(&targetDelta);
            if (!(targetDistance > minimumSeparation)) {
                nearbyTargetIndices[nearbyTargetCount] = targetIndex;
                nearbyTargetCount += 1;
                separationOffset.x += 0.5f * (targetDelta.x * (minimumSeparation - targetDistance));
                separationOffset.y += 0.5f * (targetDelta.y * (minimumSeparation - targetDistance));
                separationOffset.z += 0.5f * (targetDelta.z * (minimumSeparation - targetDistance));
            }
        }
    }
    if (nearbyTargetCount == 0) {
        return 0;
    }
    target->targetPosition.x += separationOffset.x;
    target->targetPosition.y += separationOffset.y;
    target->targetPosition.z += separationOffset.z;
    targetHeadingAngle = atan2d(separationOffset.x, separationOffset.y);
    if (targetHeadingAngle > 180.0f) {
        targetHeadingAngle -= 360.0f;
    }
    if (targetHeadingAngle > 60.0f) {
        targetHeadingAngle = 60.0f;
    }
    if (targetHeadingAngle < -60.0f) {
        targetHeadingAngle = -60.0f;
    }
    target->targetHeading.z = targetHeadingAngle = SmoothInterpolationCurve(target->targetHeading.z, targetHeadingAngle, 0.5f);
    otherTarget = playerTargetRecords;

    for (targetIndex = 0; targetIndex < 8; targetIndex++, otherTarget++) {
        if ((otherTarget != target) && (otherTarget->targetState != 0) && (otherTarget->targetState < 4U)) {
            targetDelta.x = otherTarget->targetPosition.x - target->targetPosition.x;
            targetDelta.y = otherTarget->targetPosition.y - target->targetPosition.y;
            targetDelta.z = otherTarget->targetPosition.z - target->targetPosition.z;
            targetDistance = VectorLength(&targetDelta);
            if (!(targetDistance > minimumSeparation)) {
                otherTarget->targetPosition.x += targetDelta.x * (minimumSeparation - targetDistance);
                otherTarget->targetPosition.y += targetDelta.y * (minimumSeparation - targetDistance);
                otherTarget->targetPosition.z += targetDelta.z * (minimumSeparation - targetDistance);
                targetHeadingAngle = atan2d(targetDelta.x, targetDelta.y);
                if (targetHeadingAngle > 180.0f) {
                    targetHeadingAngle -= 360.0f;
                }
                if (targetHeadingAngle > 60.0f) {
                    targetHeadingAngle = 60.0f;
                }
                if (targetHeadingAngle < -60.0f) {
                    targetHeadingAngle = -60.0f;
                }
                otherTarget->targetHeading.z = targetHeadingAngle = SmoothInterpolationCurve(otherTarget->targetHeading.z, targetHeadingAngle, 0.5f);
            }
        }
    }
    return 1;
}

s8 FindNearestTarget(Vec *position, F32 *searchRadius)
{
    F32 unusedRadiusSquared;
    F32 distanceSquared;
    F32 nearestDistanceSquared;
    F32 unusedDistanceValue;
    s32 nearestTargetIndex;
    s32 targetIndex;
    PlayerTargetWork *targetWork;

    nearestTargetIndex = -1;
    targetWork = playerTargetRecords;
    nearestDistanceSquared = *searchRadius;
    nearestDistanceSquared *= nearestDistanceSquared;

    for (targetIndex = 0; targetIndex < 8; targetIndex++, targetWork++) {
        if (((Hu3DModelAttrGet(targetWork->targetModelId) & 1) == 0) && !(targetWork->targetPosition.y < 300.0f)) {
            distanceSquared = ((position->z - targetWork->targetPosition.z) * (position->z - targetWork->targetPosition.z))
                + (((position->x - targetWork->targetPosition.x) * (position->x - targetWork->targetPosition.x)) + ((position->y - targetWork->targetPosition.y) * (position->y - targetWork->targetPosition.y)));
            if (!(distanceSquared > nearestDistanceSquared)) {
                nearestTargetIndex = targetIndex;
                nearestDistanceSquared = distanceSquared;
            }
        }
    }
    *searchRadius = sqrtf(nearestDistanceSquared);
    return nearestTargetIndex;
}

u32 playerRandomSeed = 0x41C64E6D;

s32 GetRandomPlayerIndex(void)
{
    playerRandomSeed *= 0x41C64E6D;
    playerRandomSeed += 0x3039;
    return (playerRandomSeed >> 0x10);
}

void ClampIntegerRange(s32 *values, s32 valueCount)
{
    s32 swapValue;
    s32 randomIndex;
    s32 valueIndex;

    for (valueIndex = 0; valueIndex < valueCount; valueIndex++) {
        values[valueIndex] = valueIndex;
    }

    for (valueIndex = 0; valueIndex < valueCount; valueIndex++) {
        randomIndex = (valueCount * (GetRandomPlayerIndex() / 65536.0f));
        swapValue = values[valueIndex];
        values[valueIndex] = values[randomIndex];
        values[randomIndex] = swapValue;
    }
}

F32 SmoothInterpolationCurve(F32 currentAngle, F32 targetAngle, F32 interpolationWeight)
{
    F32 angleDelta;
    F32 unusedNormalizedAngle;
    F32 unusedAngleValue;

    angleDelta = fmodf(targetAngle - currentAngle, 360.0);
    if (angleDelta < 0.0f) {
        angleDelta += 360.0f;
    }
    if (angleDelta > 180.0f) {
        angleDelta -= 360.0f;
    }
    angleDelta = fmodf(currentAngle + ((1.0f - interpolationWeight) * angleDelta), 360.0);
    if (angleDelta < 0.0f) {
        angleDelta += 360.0f;
    }
    if (angleDelta >= 180.0f) {
        angleDelta -= 360.0f;
    }
    return angleDelta;
}

F32 InterpolateCubicValue(F32 interpolationWeight, F32 startValue, F32 controlValue, F32 endValue)
{
    F32 remainingWeight;
    F32 interpolatedValue;

    remainingWeight = (1.0 - interpolationWeight);
    interpolatedValue = ((endValue * (interpolationWeight * interpolationWeight)) + ((startValue * (remainingWeight * remainingWeight)) + (2.0 * (controlValue * (remainingWeight * interpolationWeight)))));
    return interpolatedValue;
}

void InterpolateCubicValues(F32 *startValues, F32 *controlValues, F32 *endValues, F32 *outputValues, F32 interpolationWeight)
{
    s32 componentIndex;

    for (componentIndex = 0; componentIndex < 3; componentIndex++) {
        *outputValues = InterpolateCubicValue(interpolationWeight, *startValues++, *controlValues++, *endValues++);
        outputValues++;
    }
}

F32 InterpolateSmoothValue(F32 interpolationWeight, F32 startValue, F32 controlValue, F32 endValue)
{
    F32 interpolatedValue;

    interpolatedValue = (2.0 * ((interpolationWeight * endValue) + (((interpolationWeight - 1.0) * startValue) + ((1.0 - (2.0 * interpolationWeight)) * controlValue))));
    return interpolatedValue;
}

void InterpolateSmoothValues(F32 *startValues, F32 *controlValues, F32 *endValues, F32 *outputValues, F32 interpolationWeight)
{
    F32 interpolatedVector[3];
    F32 VectorLength;
    s32 componentIndex;

    for (componentIndex = 0; componentIndex < 3; componentIndex++) {
        interpolatedVector[componentIndex] = InterpolateSmoothValue(interpolationWeight, *startValues++, *controlValues++, *endValues++);
    }

    VectorLength = sqrtf((interpolatedVector[2] * interpolatedVector[2]) + ((interpolatedVector[0] * interpolatedVector[0]) + (interpolatedVector[1] * interpolatedVector[1])));

    if (VectorLength) {
        VectorLength = (1.0 / VectorLength);
        for (componentIndex = 0; componentIndex < 3; componentIndex++) {
            *outputValues++ = VectorLength * interpolatedVector[componentIndex];
        }
    }
    else {
        *outputValues++ = 0.0f;
        *outputValues++ = 0.0f;
        *outputValues++ = 1.0f;
    }
}

F32 VectorLength(Vec *vector)
{
    F32 VectorLength;
    F32 inverseLength;

    VectorLength = (vector->z * vector->z) + ((vector->x * vector->x) + (vector->y * vector->y));
    VectorLength = sqrtf(VectorLength);
    if (VectorLength != 0) {
        inverseLength = 1.0f / VectorLength;
        vector->x *= inverseLength;
        vector->y *= inverseLength;
        vector->z *= inverseLength;
    }
    else {
        vector->x = vector->y = vector->z = 0.0f;
    }
    return VectorLength;
}

F32 ProjectPointOntoSegment(Vec segmentOrigin, Vec segmentDelta, Vec *projectedPoint, F32 projectionParameter)
{
    if (projectionParameter <= 0.0f) {
        projectedPoint->x = segmentOrigin.x;
        projectedPoint->y = segmentOrigin.y;
        projectedPoint->z = segmentOrigin.z;
        projectionParameter = 0.0f;
    }
    else if (projectionParameter >= 1.0f) {
        projectedPoint->x = (segmentOrigin.x + segmentDelta.x);
        projectedPoint->y = (segmentOrigin.y + segmentDelta.y);
        projectedPoint->z = (segmentOrigin.z + segmentDelta.z);
        projectionParameter = 1.0f;
    }
    else {
        projectedPoint->x = (segmentOrigin.x + (projectionParameter * segmentDelta.x));
        projectedPoint->y = (segmentOrigin.y + (projectionParameter * segmentDelta.y));
        projectedPoint->z = (segmentOrigin.z + (projectionParameter * segmentDelta.z));
    }
    return projectionParameter;
}

F32 DistanceBetweenPoints(Vec point, Vec lineOrigin, Vec lineDirection)
{
    F32 projectionDenominator;
    F32 projectionParameter;

    projectionParameter = (lineDirection.z * (lineOrigin.z - point.z)) + ((lineDirection.x * (lineOrigin.x - point.x)) + (lineDirection.y * (lineOrigin.y - point.y)));
    projectionDenominator = -((lineDirection.z * lineDirection.z) + ((lineDirection.x * lineDirection.x) + (lineDirection.y * lineDirection.y)));
    if (projectionDenominator != 0.0f) {
        projectionParameter /= projectionDenominator;
    }
    return projectionParameter;
}

F32 DistanceBetweenSegments(Vec *point, Vec *segmentOrigin, Vec *segmentDelta)
{
    Vec projectedPoint;
    F32 projectionParameter;

    if (((segmentDelta->z * segmentDelta->z) + ((segmentDelta->x * segmentDelta->x) + (segmentDelta->y * segmentDelta->y))) == 0.0f) {
        return ((point->z - segmentOrigin->z) * (point->z - segmentOrigin->z))
            + (((point->x - segmentOrigin->x) * (point->x - segmentOrigin->x)) + ((point->y - segmentOrigin->y) * (point->y - segmentOrigin->y)));
    }
    else {
        projectionParameter = DistanceBetweenPoints(*point, *segmentOrigin, *segmentDelta);
        ProjectPointOntoSegment(*segmentOrigin, *segmentDelta, &projectedPoint, projectionParameter);
        return ((point->z - projectedPoint.z) * (point->z - projectedPoint.z)) + (((point->x - projectedPoint.x) * (point->x - projectedPoint.x)) + ((point->y - projectedPoint.y) * (point->y - projectedPoint.y)));
    }
}

F32 ClosestPointsBetweenSegments(Vec *point, Vec *segmentOrigin, Vec *segmentDelta, Vec *closestPoint, Vec *closestDelta)
{
    F32 projectionParameter;

    if (((segmentDelta->z * segmentDelta->z) + ((segmentDelta->x * segmentDelta->x) + (segmentDelta->y * segmentDelta->y))) == 0.0f) {
        return ((point->z - segmentOrigin->z) * (point->z - segmentOrigin->z))
            + (((point->x - segmentOrigin->x) * (point->x - segmentOrigin->x)) + ((point->y - segmentOrigin->y) * (point->y - segmentOrigin->y)));
    }
    else {
        projectionParameter = DistanceBetweenPoints(*point, *segmentOrigin, *segmentDelta);
        ProjectPointOntoSegment(*segmentOrigin, *segmentDelta, closestPoint, projectionParameter);
    }
    closestDelta->x = closestPoint->x - point->x;
    closestDelta->y = closestPoint->y - point->y;
    closestDelta->z = closestPoint->z - point->z;
    return VectorLength(closestDelta);
}

void DrawPlayerParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    s32 particleIndex;
    HU3DPARTICLEDATA *particleData;

    particleData = particle->data;
    for (particleIndex = 0; particleIndex < particle->maxCnt; particleIndex++, particleData++) {
        particleData->pos.x = 0.0f;
        particleData->pos.y = 200.0f;
        particleData->pos.z = -500.0f;
        particleData->scale = 80.0f;
    }
    DCFlushRange(particle->data, particle->maxCnt * 0x44);
}

F32 GetPlayerRank(s16 playerIndex)
{
    HU3DMOTION *motionData;
    HSFMOTION *hsfMotionData;
    F32 length;

    motionData = &Hu3DMotion[playerIndex];
    hsfMotionData = motionData->hsf->motion;
    length = hsfMotionData->maxTime;
    return length;
}

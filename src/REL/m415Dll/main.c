#include "REL/m415Dll.h"

#include "ext_math.h"
#include "game/EnvelopeExec.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/esprite.h"
#include "game/frand.h"
#include "game/hsfload.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include "math.h"
#include "version.h"

#ifndef __MWERKS__
#include <string.h>
#endif

// bss
MapRendererState mapRenderer;
OMOBJ *openingSequenceObject;
OMOBJ *outOfViewObject;
OMOBJ *cameraTransitionObject;
OMOBJ *sceneObject;
OMOBJ *playerObjects[4];
u16 introState;
s16 outroSequenceId;
s16 introSequenceId;
s16 introCountdownFrames;
s16 introWaitFrames;
s16 playerPaintCounts[4];
s16 canvasTextureAnimationId;
u8 *canvasPixels;
s32 playerPaintCellCounts[4][4][4];
void *temporaryAllocations[0x80];
s32 sceneState[2];
PlayerSelection playerSelections[4];
static s32 reservedBssStorage;

// data
Vec keyLightPosition = { 0.0f, 3000.0f, 2000.0f };
Vec keyLightDirection = { 0.0f, -0.7f, -0.6f };
GXColor whiteLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };
Vec shadowOrigin = { 0.0f, 3000.0f, -2000.0f };
Vec worldUpVector = { 0.0f, 1.0f, 0.0f };
Vec shadowTarget = { 0.0f, 0.0f, 0.0f };

void ObjectSetup(void)
{
    s32 playerIndex;
    HUPROCESS *process;
    OMOBJ *object;
    HU3DLIGHT *light;
    s32 randomWarmupCount;
    s32 keyLightId;

    OSReport("******* M415ObjectSetup *********\n");
    Hu3DLighInit();
    keyLightId = Hu3DGLightCreateV(&keyLightPosition, &keyLightDirection, &whiteLightColor);
    Hu3DGLightInfinitytSet(keyLightId);
    light = &Hu3DGlobalLight[0];
    light->type |= 0x8000;
    process = omInitObjMan(0x32, 0x2000);
    randomWarmupCount = frand() & 0x1F;

    for (playerIndex = 0; playerIndex < randomWarmupCount; playerIndex++) {
        MinigameRandom();
    }

    CRot.x = -55.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    Center.x = 0.0f;
    Center.y = 200.0f;
    Center.z = 200.0f;
    CZoom = 1500.0f;
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, -1.0f, 5.0f, 10000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    outOfViewObject = omAddObjEx(process, 0x7FDA, 0, 0, -1, omOutView);
    openingSequenceObject = omAddObjEx(process, 0x10, 8, 0, -1, OpeningCameraSetup);
    sceneObject = omAddObjEx(process, 0x20, 0x10, 0, -1, SceneModelsSetup);
    cameraTransitionObject = omAddObjEx(process, 0x50, 0x10, 0, -1, CameraTransitionInitialize);
    omAddObjEx(process, 0x60, 0, 0, -1, PlayerCharactersCleanup);
    omMakeGroupEx(process, 0, 4);
    omGetGroupMemberListEx(process, 0);

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObjects[playerIndex] = object = omAddObjEx(process, 0x30, 6, 0xA, 0, PlayerCharacterInitialize);
        object->work[0] = playerIndex;
        object->work[1] = playerIndex;
    }
    Hu3DShadowCreate(30.0f, 100.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.4f);
    Hu3DShadowPosSet(&shadowOrigin, &worldUpVector, &shadowTarget);
    omGameSysInit(process);
    HuAudSndGrpSet(0x28);
    TemporaryAllocationsInitialize();
    MapRendererInitialize(0x40, 0x40);
}

void OpeningCameraSetup(OMOBJ *object)
{
    HU3DCAMERA *camera;

    camera = &Hu3DCamera[0];
    introState = 0;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    camera->fov = 45.0f;
    object->objFunc = OpeningSequenceUpdate;
}

void OpeningSequenceUpdate(OMOBJ *object)
{
    Vec initialLookAt;
    Vec initialRotation;
    s32 itemIndex;
    f32 initialZoom;

    switch (introState) {
        case 0:
            initialLookAt.x = 0.0f;
            initialLookAt.y = 200.0f;
            initialLookAt.z = 200.0f;
            initialRotation.x = -55.0f;
            initialRotation.y = 0.0f;
            initialRotation.z = 0.0f;
            initialZoom = 1500.0f;
            CameraPoseInterpolate(1.0f, &initialLookAt, &initialRotation, initialZoom);
            MapTextureAnimationLoad(0x2E000D, 0, 0);
            HuDataDirClose(0x2E000D);
            MapTextureAnimationLoad(0x2E000C, 0x40, 0x40);
            HuDataDirClose(0x2E000C);
            introWaitFrames = REFRESH_RATE * 2;
            HuAudFXPlay(0x5AE);
            introState++;
            break;
        case 1:
            if (REFRESH_RATE_F * 1.2f == introWaitFrames) {
                HuAudFXPlay(0x5AF);
                for (itemIndex = 0; itemIndex < 4; itemIndex++) {
                    omVibrate(itemIndex, 0xC, 4, 2);
                }
            }
            if (--introWaitFrames == 0) {
                introState++;
            }
            break;
        case 2:
            if (introWaitFrames == 0) {
                introWaitFrames = REFRESH_RATE * 1.5;
            }
            if (--introWaitFrames == 0) {
                introState++;
            }
            break;
        case 3:
            introState = 4;
            outroSequenceId = introSequenceId = -1;
            object->objFunc = MinigameIntroUpdate;
            HuSprAnimRead(HuDataReadNum(0x2E000D, 0x10000000));
            break;
    }
    if ((omSysExitReq != 0) && (WipeStatGet() == 0)) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = MinigameExitUpdate;
    }
}

extern s32 introMusicSequenceId = -1;

void MinigameIntroUpdate(OMOBJ *object)
{
    GXColor playerScoreColors[4] = { { 0xF0, 0x00, 0x00, 0xFF }, { 0x30, 0x60, 0xF0, 0xFF }, { 0xE0, 0xE0, 0x00, 0xFF }, { 0xFF, 0x80, 0x80, 0xFF } };
    s32 playerScores[4];
    s32 playerIndex;
    s32 digitIndex;
    s32 scoreIndex;
    s32 highestScore;
    s32 countdownFinished;
    IntroColorPalette *playerFlagData;
    s16 playerScore;
    s16 coinReward;
    s16 currentCoins;

    switch (introState) {
        case 4:
            if (introSequenceId == -1) {
                introCountdownFrames = 30 * REFRESH_RATE;
                introSequenceId = MGSeqCreate(3, 0);
                MGSeqPosSet(introSequenceId, 320.0f, 240.0f);
            }
            else {
                if (MGSeqStatGet(introSequenceId) == 0) {
                    introSequenceId = -1;
                    introState++;
                    introWaitFrames = 0;
                    outroSequenceId = MGSeqCreate(1, introCountdownFrames / REFRESH_RATE, -1, -1);
                }
            }
            if ((introMusicSequenceId == -1) && ((MGSeqStatGet(introSequenceId) & 0x10) != 0)) {
                introMusicSequenceId = HuAudSeqPlay(0x45);
            }
            break;
        case 5:
            countdownFinished = 0;
            introCountdownFrames--;
            MGSeqParamSet(outroSequenceId, 1, ((introCountdownFrames + REFRESH_RATE - 1) / REFRESH_RATE));
            if (introCountdownFrames == 0) {
                countdownFinished++;
            }
            if (countdownFinished != 0) {
                introState += 1;
            }
            break;
        case 6:
            if (introSequenceId == -1) {
                introSequenceId = MGSeqCreate(3, 1);
                MGSeqPosSet(introSequenceId, 320.0f, 240.0f);
                MGSeqParamSet(outroSequenceId, 2, -1);
                HuAudSeqFadeOut(introMusicSequenceId, 0x64);
            }
            else if (MGSeqStatGet(introSequenceId) == 0) {
                introSequenceId = -1;
                introState++;
                introWaitFrames = 0;
            }
            break;
        case 7:
            if (introWaitFrames == 0) {
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    playerSelections[playerIndex].playerMarkerSpriteId = espEntry(0x2E0010, 10, 0);
                    espColorSet(playerSelections[playerIndex].playerMarkerSpriteId, 0, 0, 0);
                    espTPLvlSet(playerSelections[playerIndex].playerMarkerSpriteId, 0.5f);
                    espPosSet(
                        playerSelections[playerIndex].playerMarkerSpriteId, 144.0f * (((playerIndex & 1) * 2) + 1), 120.0f * (((s16)(((playerIndex < 0) + playerIndex) >> 1) << 1) + 1));
                    playerSelections[playerIndex].playerIconSpriteId = espEntry(0x2E000F, 10, 0);
                    espPosSet(
                        playerSelections[playerIndex].playerIconSpriteId, 144.0f * (((playerIndex & 1) * 2) + 1), 120.0f * (((s16)(((playerIndex < 0) + playerIndex) >> 1) << 1) + 1));
                    for (digitIndex = 0; digitIndex < 3; digitIndex++) {
                        playerSelections[playerIndex].digitSpriteIds[digitIndex] = espEntry(0x2E0011, 10, 0);
                        espPosSet(playerSelections[playerIndex].digitSpriteIds[digitIndex], (32.0f * digitIndex) + ((144.0f * (((playerIndex & 1) * 2) + 1)) - 32.0f),
                            (120.0f * (((s16)(((playerIndex < 0) + playerIndex) >> 1) << 1) + 1)));
                        espColorSet(playerSelections[playerIndex].digitSpriteIds[digitIndex], playerScoreColors[playerIndex].r, playerScoreColors[playerIndex].g, playerScoreColors[playerIndex].b);
                    }
                    espBankSet(playerSelections[playerIndex].digitSpriteIds[2], 10);
                }
                introWaitFrames = REFRESH_RATE;
                HuAudFXPlay(0x5B4);
            }
            else {
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    for (digitIndex = 0; digitIndex < 2; digitIndex++) {
                        espBankSet(playerSelections[playerIndex].digitSpriteIds[digitIndex], (s32)((MinigameRandom() / 65536.0f) * 10.0f));
                    }
                }
                if (--introWaitFrames == 0) {
                    HuAudFXPlay(0x5B5);
                    highestScore = 6;
                    for (scoreIndex = 0; scoreIndex < 4; scoreIndex++) {
                        playerScores[scoreIndex] = 0;
                        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                            for (digitIndex = 0; digitIndex < 4; digitIndex++) {
                                playerScores[scoreIndex] += playerPaintCellCounts[scoreIndex][playerIndex][digitIndex];
                            }
                        }
                        playerScores[scoreIndex] = (0.99f + ((100.0f * playerScores[scoreIndex]) / 360000.0f));
                        if (highestScore < playerScores[scoreIndex]) {
                            highestScore = playerScores[scoreIndex];
                        }
                    }
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        playerScore = playerScores[playerIndex];
                        espBankSet(playerSelections[playerIndex].digitSpriteIds[0], playerScore / 10);
                        espBankSet(playerSelections[playerIndex].digitSpriteIds[1], playerScore % 10);
                    }
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        playerPaintCounts[playerIndex] = -1;
                    }

                    playerIndex = 0;
                    digitIndex = 0;
                    for (; playerIndex < 4; playerIndex++) {
                        if (playerScores[playerIndex] >= highestScore) {
                            currentCoins = GWPlayer[playerIndex].coin_win;
                            coinReward = currentCoins + 10;
                            if (_CheckFlag(0x1000CU) == 0) {
                                GWPlayer[playerIndex].coin_win = coinReward;
                            }
                            playerPaintCounts[digitIndex++] = GWPlayerCfg[playerIndex].character;
                            playerFlagData = playerObjects[playerIndex]->data;
                            playerFlagData->activeFlag = 1;
                        }
                    }
                    introWaitFrames = REFRESH_RATE;
                    introState++;
                }
            }
            break;
        case 8:
            if (introSequenceId == -1) {
                if (--introWaitFrames == 0) {
                    if (playerPaintCounts[0] == -1) {
                        introSequenceId = MGSeqCreate(3, 2);
                        HuAudSStreamPlay(4);
                    }
                    else {
                        introSequenceId = MGSeqCreate(5, 3, playerPaintCounts[0], playerPaintCounts[1], playerPaintCounts[2], playerPaintCounts[3]);
                        HuAudSStreamPlay(1);
                    }
                    introSequenceId = -1;
                    introState++;
                    introWaitFrames = REFRESH_RATE * 3.5;
                }
            }
            break;
        case 9:
            if (--introWaitFrames == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                object->objFunc = MinigameExitUpdate;
            }
            break;
    }
    if ((omSysExitReq != 0) && (WipeStatGet() == 0)) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = MinigameExitUpdate;
    }
}

void MinigameExitUpdate(OMOBJ *object)
{
    if (WipeStatGet() == 0) {
        CharModelKill(-1);
        MapRendererShutdown();
        TemporaryAllocationsFreeAll();
        MGSeqKill(outroSequenceId);
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
    }
}

void ModelPoseEvaluate(s16 modelId, char *objectName, f32 motionTime, Mtx objectMatrix)
{
    HU3DMODEL *model;

    model = &Hu3DData[modelId];
    if (model->motId != -1) {
        Hu3DMotionExec(modelId, model->motId, motionTime, 0);
    }
    if (model->motIdShift != -1) {
        Hu3DSubMotionExec(modelId);
    }
    if (model->hsf->cenvNum != 0) {
        EnvelopeProc(model->hsf);
    }
    Hu3DModelObjMtxGet(modelId, objectName, objectMatrix);
}

void SceneModelsSetup(OMOBJ *object)
{
    Mtx modelObjectMatrix;
    Vec modelPosition;
    Vec modelRotation;
    s32 canvasBufferSize;
    s32 shadowTextureId;
    HU3DMODEL *model;
    f32 modelMotionDuration;
    s32 sceneModelId;

    object->mdlId[0] = sceneModelId = Hu3DModelCreateFile(0x2E0000);
    Hu3DModelShadowSet(sceneModelId);
    modelMotionDuration = Hu3DMotionMaxTimeGet(sceneModelId);
    ModelPoseEvaluate(sceneModelId, "oekaki", modelMotionDuration, modelObjectMatrix);
    modelPosition.x = -modelObjectMatrix[0][3];
    modelPosition.y = -modelObjectMatrix[1][3];
    modelPosition.z = -modelObjectMatrix[2][3];
    RotationMatrixToEuler(modelObjectMatrix, &modelRotation);
    modelRotation.x = -(modelRotation.x - 90.0f);
    modelRotation.y = -(modelRotation.y - 90.0f);
    modelRotation.z = -(modelRotation.z - 90.0f);
    Hu3DMotionTimeSet(sceneModelId, 0.0f);
    Hu3DMotionSpeedSet(sceneModelId, 2.0f);
    omSetTra(object, modelPosition.x, modelPosition.y, modelPosition.z);
    omSetRot(object, modelRotation.x, modelRotation.y, modelRotation.z);
    Hu3DModelPosSetV(sceneModelId, &modelPosition);
    Hu3DModelRotSetV(sceneModelId, &modelRotation);
    canvasTextureAnimationId = Hu3DAnimCreate(HuDataReadNum(0x2E000A, 0x10000000), sceneModelId, "dummy");
    canvasBufferSize = 0x57E40;
    canvasPixels = HuMemDirectMallocNum(HEAP_MODEL, canvasBufferSize, HU_MEMNUM_OVL);
    memset(canvasPixels, 0, canvasBufferSize);
    (void)canvasBufferSize;
    memset(&playerPaintCellCounts, 0, 0x100);
    sceneModelId = mapObjectCreate(1, 2);
    object->mdlId[1] = sceneModelId;
    MapObjectTextureLoad(sceneModelId, 0, 0x2E000B, 0x200, 0x200);
    MapObjectPositionSet(sceneModelId, 0.0f, -50.0f, 0.0f);
    MapObjectRotationSet(sceneModelId, -90.0f, 0.0f, 0.0f);
    MapObjectScaleSet(sceneModelId, 5.0f, 5.0f, 1.0f);
    MapMeshBillboardModeSet(sceneModelId, 0, 0);
    shadowTextureId = ShadowTextureCreate();
    object->mdlId[2] = shadowTextureId;
    MapTextureAnimationFrameSet(shadowTextureId, 4);
    MapObjectTextureSet(sceneModelId, 1, shadowTextureId);
    sceneModelId = Hu3DModelCreateFile(0x2E0002);
    object->mdlId[3] = sceneModelId;
    Hu3DModelShadowSet(sceneModelId);
    Hu3DMotionSpeedSet(sceneModelId, 2.0f);
    object->objFunc = ShadowTextureUpdate;
}

void ShadowTextureUpdate(OMOBJ *object)
{
    u32 shadowPixelCount;
    ANIMDATA **textureAnimation;

    switch (introState) {
        case 0:
            break;
        case 1:
            GXDrawDone();
            textureAnimation = mapTextureAnimationGet(object->mdlId[2]);
            shadowPixelCount = Hu3DShadowData.size * Hu3DShadowData.size;
            memcpy((*textureAnimation)->bmp->data, OSCachedToUncached(Hu3DShadowData.buf), shadowPixelCount);
            DCStoreRangeNoSync((*textureAnimation)->bmp->data, shadowPixelCount);
            break;
        case 2:
            Hu3DModelShadowMapObjSet(object->mdlId[0], "kyanbasu");
            Hu3DModelShadowReset(object->mdlId[0]);
            Hu3DModelShadowReset(object->mdlId[3]);
            break;
        case 3:
        case 4:
        case 5:
            break;
    }
    return;
}

void PlayerPaintEffectDraw(PlayerCharacterState *playerState)
{
    Mtx rotationMatrix;
    Mtx translationMatrix;
    Vec brushBounds[4];
    Vec brushEdges[4];
    GXColor playerBaseColors[8] = { { 0xF0, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0xF0, 0xFF }, { 0xFF, 0x80, 0x80, 0xFF }, { 0x00, 0xFF, 0x00, 0xFF },
        { 0x80, 0x00, 0xC0, 0xFF }, { 0x80, 0x40, 0x00, 0xFF }, { 0xE0, 0xE0, 0x00, 0xFF }, { 0x00, 0x00, 0x00, 0xFF } };
    GXColor playerHighlightColors[8] = { { 0xFF, 0x60, 0x60, 0xFF }, { 0x60, 0x60, 0xFF, 0xFF }, { 0xFF, 0xC0, 0xC0, 0xFF }, { 0x40, 0xFF, 0x40, 0xFF },
        { 0xC0, 0x40, 0xFF, 0xFF }, { 0xD0, 0x80, 0x40, 0xFF }, { 0xFF, 0xFF, 0x20, 0xFF }, { 0x80, 0x80, 0x80, 0xFF } };
    GXColor brushColors[4] = { { 0xF0, 0x00, 0x00, 0xFF }, { 0x00, 0x00, 0xF0, 0xFF }, { 0xE0, 0xE0, 0x00, 0xFF }, { 0xFF, 0x80, 0x80, 0xFF } };
    GXColor brushHighlightColors[4] = { { 0xFF, 0x60, 0x60, 0xFF }, { 0x60, 0x60, 0xFF, 0xFF }, { 0xFF, 0xFF, 0x20, 0xFF }, { 0xFF, 0xC0, 0xC0, 0xFF } };
    Vec playerPosition;
    s32 temporaryPosition;
    s16 canvasCenterX;
    s16 canvasCenterZ;
    s16 pixelIndex;
    s16 textureRow;
    s16 tileSize;
    s16 tileOffset;
    u8 characterId;
    f32 interpolationFactor;
    f32 playerAngle;
    f32 leftEdgeX;
    f32 rightEdgeX;
    s32 pixelY;
    s16 brushRadius;
    s32 pixelX;
    u8 playerIndex;
    s16 tileRow;
    s32 pixelOffset;
    s16 cellX;
    s16 cellZ;
    u16 *canvasPixelData;
    u8 previousPlayerId;
    ANIMBMP *canvasBitmap;
    s16 tileColumn;
    s16 tileDepth;
    s32 brushRadiusSquared;

    playerPosition = playerState->position;
    playerAngle = playerState->turnSpeed;
    characterId = playerState->characterId;
    playerIndex = playerState->playerIndex;
    canvasCenterX = 0.5f * (600.0f + playerPosition.x);
    canvasCenterZ = 0.5f * (600.0f + playerPosition.z);
    canvasBitmap = TextureAnimationBitmapGet(canvasTextureAnimationId);
    canvasPixelData = (u16 *)canvasBitmap->data;
    tileRow = 0x32;
    brushRadius = tileRow >> 1;
    tileSize = 0x96;
    tileOffset = 0;
    if ((s16)playerAngle == ((s16)(playerAngle / 90.0f) * 90)) {
        brushBounds[0].x = -brushRadius;
        brushBounds[0].y = -brushRadius;
        brushBounds[1].x = -brushRadius;
        brushBounds[1].y = brushRadius;
        brushBounds[2].x = brushRadius;
        brushBounds[2].y = -brushRadius;
        brushBounds[3].x = brushRadius;
        brushBounds[3].y = brushRadius;
        brushBounds[0].z = brushBounds[1].z = brushBounds[2].z = brushBounds[3].z = 0.0f;
    }
    else {
        brushBounds[0].x = tileRow;
        brushBounds[0].y = tileRow;
        brushBounds[1].x = tileRow;
        brushBounds[1].y = -tileRow;
        brushBounds[2].x = -tileRow;
        brushBounds[2].y = tileRow;
        brushBounds[3].x = -tileRow;
        brushBounds[3].y = -tileRow;
        brushBounds[0].z = brushBounds[1].z = brushBounds[2].z = brushBounds[3].z = 0.0f;
        for (pixelY = -1; pixelY < 2; pixelY += 2) {
            for (pixelX = -1; pixelX < 2; pixelX += 2) {
                MTXRotDeg(rotationMatrix, 0x59, playerAngle);
                MTXTrans(translationMatrix, (brushRadius * pixelY), 0.0f, (brushRadius * pixelX));
                MTXConcat(rotationMatrix, translationMatrix, translationMatrix);
                if ((brushBounds[0].y > translationMatrix[2][3]) || ((brushBounds[0].y == translationMatrix[2][3]) && (brushBounds[0].x > translationMatrix[0][3]))) {
                    brushBounds[0].x = translationMatrix[0][3];
                    brushBounds[0].y = translationMatrix[2][3];
                }
                if ((brushBounds[1].x > translationMatrix[0][3]) || (brushBounds[1].x >= translationMatrix[0][3]) && (brushBounds[1].y < translationMatrix[2][3])) {
                    brushBounds[1].x = translationMatrix[0][3];
                    brushBounds[1].y = translationMatrix[2][3];
                }
                if ((brushBounds[2].x < translationMatrix[0][3]) || (brushBounds[2].x <= translationMatrix[0][3]) && (brushBounds[2].y > translationMatrix[2][3])) {
                    brushBounds[2].x = translationMatrix[0][3];
                    brushBounds[2].y = translationMatrix[2][3];
                }
                if ((brushBounds[3].y <= translationMatrix[2][3]) || (brushBounds[3].y <= translationMatrix[2][3]) && (brushBounds[3].x < translationMatrix[0][3])) {
                    brushBounds[3].x = translationMatrix[0][3];
                    brushBounds[3].y = translationMatrix[2][3];
                }
            }
        }
    }
    VECSubtract(&brushBounds[1], &brushBounds[0], &brushEdges[0]);
    VECSubtract(&brushBounds[2], &brushBounds[0], &brushEdges[1]);
    VECSubtract(&brushBounds[3], &brushBounds[1], &brushEdges[2]);
    VECSubtract(&brushBounds[3], &brushBounds[2], &brushEdges[3]);
    brushRadiusSquared = (1.5 * (brushRadius * brushRadius));
    for (pixelY = brushBounds[0].y; pixelY < brushBounds[3].y; pixelY++) {
        cellZ = canvasCenterZ + pixelY;
        if (cellZ >= 0 && (!(cellZ >= 600.0f))) {
            tileDepth = (cellZ / 150.0f);
            if (pixelY <= brushBounds[1].y) {
                interpolationFactor = ((pixelY - brushBounds[0].y) / brushEdges[0].y);
                leftEdgeX = interpolationFactor * brushEdges[0].x + brushBounds[0].x;
            }
            else {
                interpolationFactor = ((pixelY - brushBounds[1].y) / brushEdges[2].y);
                leftEdgeX = interpolationFactor * brushEdges[2].x + brushBounds[1].x;
            }
            if (pixelY <= brushBounds[2].y) {
                interpolationFactor = ((pixelY - brushBounds[0].y) / brushEdges[1].y);
                rightEdgeX = interpolationFactor * brushEdges[1].x + brushBounds[0].x;
            }
            else {
                interpolationFactor = ((pixelY - brushBounds[2].y) / brushEdges[3].y);
                rightEdgeX = interpolationFactor * brushEdges[3].x + brushBounds[2].x;
            }

            for (pixelX = leftEdgeX; pixelX < rightEdgeX; pixelX++) {
                cellX = canvasCenterX + pixelX;
                if (cellX >= 0 && (!(cellX >= 600.0f))) {
                    tileColumn = (cellX / 150.0f);
                    pixelOffset = (600.0f * cellZ + cellX);
                    previousPlayerId = canvasPixels[pixelOffset];
                    canvasPixels[pixelOffset] = playerIndex + 1;
                    pixelIndex = (cellX >> 2);
                    textureRow = (cellZ >> 2);
                    pixelOffset = ((cellZ % 4) * 4) + ((pixelIndex * 0x10) + (cellX % 4) + textureRow * ((tileSize * 0x10) + (tileOffset * 4)));
                    temporaryPosition = ((MinigameRandom() / 65536.0f) * (1.0f + ((6.0f * (brushRadiusSquared - ((pixelY * pixelY) + (pixelX * pixelX)))) / brushRadiusSquared)));
                    if (temporaryPosition == 0) {
                        if (previousPlayerId != (playerIndex + 1)) {
                            canvasPixelData[pixelOffset] = ((brushHighlightColors[playerIndex].r >> 3) << 10) | ((brushHighlightColors[playerIndex].g >> 3) << 5) | (brushHighlightColors[playerIndex].b >> 3) | 0x8000;
                        }
                    }
                    else {
                        canvasPixelData[pixelOffset] = ((brushColors[playerIndex].r >> 3) << 10) | ((brushColors[playerIndex].g >> 3) << 5) | (brushColors[playerIndex].b >> 3) | 0x8000;
                    }
                    if (previousPlayerId != (playerIndex + 1)) {
                        playerPaintCellCounts[playerIndex][tileColumn][tileDepth]++;
                        if (previousPlayerId != 0) {
                            playerPaintCellCounts[previousPlayerId - 1][tileColumn][tileDepth]--;
                        }
                    }
                }
            }
        }
    }
    DCStoreRange(canvasPixelData, 0xAFC80);
}

void CameraTransitionInitialize(OMOBJ *cameraObject)
{
    CameraTransitionState *cameraState;

    cameraState = TemporaryAllocate(0x14C);
    cameraObject->data = cameraState;
    cameraState->framesRemaining = cameraState->duration = 0.0f;
    cameraState->frameStep = 1.0f;
    cameraState->cameraRotation.x = cameraState->startRotation.x = -55.0f;
    cameraState->cameraRotation.y = cameraState->startRotation.y = 0.0f;
    cameraState->cameraRotation.z = cameraState->startRotation.z = 0.0f;
    cameraState->lookAt.x = cameraState->startLookAt.x = 0.0f;
    cameraState->lookAt.y = cameraState->startLookAt.y = 200.0f;
    cameraState->lookAt.z = cameraState->startLookAt.z = 200.0f;
    cameraState->zoom = cameraState->startZoom = 1500.0f;
    cameraObject->objFunc = CameraTransitionUpdate;
}

void CameraTransitionUpdate(OMOBJ *object)
{
    f32 interpolationRatio;
    CameraTransitionState *cameraState;

    cameraState = (CameraTransitionState *)object->data;
    if (cameraState->framesRemaining > 0.0f) {
        cameraState->framesRemaining -= cameraState->frameStep;
        interpolationRatio = ((cameraState->duration - cameraState->framesRemaining) / cameraState->duration);
        interpolationRatio = sind(90.0f * interpolationRatio);
        if (cameraState->interpolationMode == 0) {
            cameraState->lookAt.x = (cameraState->startLookAt.x + (interpolationRatio * (cameraState->targetLookAt.x - cameraState->startLookAt.x)));
            cameraState->lookAt.y = (cameraState->startLookAt.y + (interpolationRatio * (cameraState->targetLookAt.y - cameraState->startLookAt.y)));
            cameraState->lookAt.z = (cameraState->startLookAt.z + (interpolationRatio * (cameraState->targetLookAt.z - cameraState->startLookAt.z)));
            cameraState->cameraRotation.x = (cameraState->startRotation.x + (interpolationRatio * (cameraState->targetRotation.x - cameraState->startRotation.x)));
            cameraState->cameraRotation.y = (cameraState->startRotation.y + (interpolationRatio * (cameraState->targetRotation.y - cameraState->startRotation.y)));
            cameraState->cameraRotation.z = (cameraState->startRotation.z + (interpolationRatio * (cameraState->targetRotation.z - cameraState->startRotation.z)));
            cameraState->zoom = (cameraState->startZoom + (interpolationRatio * (cameraState->targetZoom - cameraState->startZoom)));
        }
        else {
            SplineVectorEvaluate(2.0f * interpolationRatio, &cameraState->lookAt, cameraState->lookAtDerivatives, cameraState->lookAtPoints, cameraState->rotationPoints, 3);
            SplineVectorEvaluate(2.0f * interpolationRatio, &cameraState->cameraRotation, cameraState->rotationDerivatives, cameraState->lookAtSlopes, cameraState->rotationSlopes, 3);
            cameraState->zoom = SplineScalarEvaluate(2.0f * interpolationRatio, cameraState->zoomPoints, cameraState->zoomDerivatives, cameraState->zoomSecondDerivatives, 3);
        }
    }
    CRot.x = cameraState->cameraRotation.x;
    CRot.y = cameraState->cameraRotation.y;
    CRot.z = cameraState->cameraRotation.z;
    Center.x = cameraState->lookAt.x;
    Center.y = cameraState->lookAt.y;
    Center.z = cameraState->lookAt.z;
    CZoom = cameraState->zoom;
}

void CameraPoseInterpolate(f32 durationFrames, Vec *targetLookAt, Vec *targetRotation, f32 targetZoom)
{
    CameraTransitionState *cameraState;

    cameraState = cameraTransitionObject->data;
    cameraState->interpolationMode = 0;
    cameraState->duration = durationFrames;
    cameraState->framesRemaining = durationFrames;
    cameraState->startLookAt = cameraState->lookAt;
    cameraState->targetLookAt.x = targetLookAt->x;
    cameraState->targetLookAt.y = targetLookAt->y;
    cameraState->targetLookAt.z = targetLookAt->z;
    cameraState->startRotation = cameraState->cameraRotation;
    cameraState->targetRotation.x = targetRotation->x;
    cameraState->targetRotation.y = targetRotation->y;
    cameraState->targetRotation.z = targetRotation->z;
    cameraState->startZoom = cameraState->zoom;
    cameraState->targetZoom = targetZoom;
}

void CameraSplineConfigure(f32 durationFrames, Vec *targetLookAt, Vec *targetRotation, f32 targetZoom, Vec *controlLookAt, Vec *controlRotation, f32 controlZoom)
{
    CameraTransitionState *cameraState;

    cameraState = cameraTransitionObject->data;
    cameraState->interpolationMode = 1;
    cameraState->framesRemaining = cameraState->duration = durationFrames;
    cameraState->startLookAt = cameraState->lookAt;
    cameraState->targetLookAt.x = targetLookAt->x;
    cameraState->targetLookAt.y = targetLookAt->y;
    cameraState->targetLookAt.z = targetLookAt->z;
    cameraState->controlLookAt.x = controlLookAt->x;
    cameraState->controlLookAt.y = controlLookAt->y;
    cameraState->controlLookAt.z = controlLookAt->z;
    cameraState->startRotation = cameraState->cameraRotation;
    cameraState->targetRotation.x = targetRotation->x;
    cameraState->targetRotation.y = targetRotation->y;
    cameraState->targetRotation.z = targetRotation->z;
    cameraState->controlRotation.x = controlRotation->x;
    cameraState->controlRotation.y = controlRotation->y;
    cameraState->controlRotation.z = controlRotation->z;
    cameraState->startZoom = cameraState->zoom;
    cameraState->targetZoom = targetZoom;
    cameraState->zoomControl = controlZoom;
    cameraState->lookAtPoints[0] = cameraState->startLookAt;
    cameraState->lookAtPoints[1] = cameraState->controlLookAt;
    cameraState->lookAtPoints[2] = cameraState->targetLookAt;
    cameraState->lookAtSlopes[0] = cameraState->startRotation;
    cameraState->lookAtSlopes[1] = cameraState->controlRotation;
    cameraState->lookAtSlopes[2] = cameraState->targetRotation;
    cameraState->zoomDerivatives[0] = cameraState->startZoom;
    cameraState->zoomDerivatives[1] = cameraState->zoomControl;
    cameraState->zoomDerivatives[2] = cameraState->targetZoom;
    cameraState->zoomPoints[0] = 0.0f;
    cameraState->zoomPoints[1] = ABS(cameraState->zoomDerivatives[1] - cameraState->zoomDerivatives[0]);
    cameraState->zoomPoints[2] = (cameraState->zoomPoints[1] + ABS(cameraState->zoomDerivatives[2] - cameraState->zoomDerivatives[1]));
    SplineVectorSecondDerivativesCalculate(cameraState->lookAtDerivatives, cameraState->lookAtPoints, cameraState->rotationPoints, 3);
    SplineVectorSecondDerivativesCalculate(cameraState->rotationDerivatives, cameraState->lookAtSlopes, cameraState->rotationSlopes, 3);
    SplineSecondDerivativesCalculate(cameraState->zoomPoints, cameraState->zoomDerivatives, cameraState->zoomSecondDerivatives, 3);
}

void CameraTransitionSpeedSet(f32 frameStep)
{
    CameraTransitionState *data;

    data = (CameraTransitionState *)cameraTransitionObject->data;
    data->frameStep = frameStep;
}

s32 playerMotionIds[10] = { 0, 3, 5, 6, 0x2E0012, 0x2E001A, 0x2E0022, 0x2E001A, 0x17, 0x72 };

Vec playerSpawnPositions[4] = { { -200.0f, 1600.0f, -200.0f }, { 200.0f, 1600.0f, -200.0f }, { -200.0f, 1600.0f, 200.0f }, { 200.0f, 1600.0f, 200.0f } };

void PlayerCharacterInitialize(OMOBJ *playerObject)
{
    s32 characterModelId;
    s32 motionIndex;
    u32 spawnPositionIndex;
    PlayerCharacterState *playerState;

    playerObject->data = playerState = TemporaryAllocate(0xA0);
    playerState->playerIndex = playerObject->work[0];
    playerState->characterId = GWPlayerCfg[playerState->playerIndex].character;
    playerState->inputFlags = (playerState->playerIndex >> 1);
    playerState->animationFlags = (playerState->playerIndex & 1);
    playerState->stateFlags0 = GWPlayerCfg[playerState->playerIndex].pad_idx;
    playerState->stateFlags1 = GWPlayerCfg[playerState->playerIndex].iscom;
    playerState->stateFlags2 = GWPlayerCfg[playerState->playerIndex].diff;
    playerState->paintGridX = playerState->paintGridZ = 4;
    playerState->isActive = 1;
    playerState->effectFlags2 = 1;
    playerState->effectFlags3 = 0;
    playerState->effectFlags0 = 0;
    playerState->movementState = 0;
    playerState->characterState = 0;
    playerState->effectFlags4 = 0;

    spawnPositionIndex = playerObject->work[1];
    playerState->position.x = playerSpawnPositions[spawnPositionIndex].x;
    playerState->position.y = playerSpawnPositions[spawnPositionIndex].y + (400.0f * ((MinigameRandom() - 0x8000) / 32768.0f));
    playerState->position.z = playerSpawnPositions[spawnPositionIndex].z;
    playerState->previousPosition.x = playerState->previousPosition.y = playerState->previousPosition.z = 0.0f;
    playerState->rotation.x = playerState->rotation.y = playerState->rotation.z = 0.0f;
    playerState->turnSpeed = playerState->rotation.y;
    playerState->effectScale = 0.0f;
    playerState->movementSpeed = 0.0f;
    playerState->effectTimer = 1.0f;
    memset(&playerState->paintCellCounts, 0, 0x40);
    characterModelId = CharModelCreate(playerState->characterId, 4);
    playerObject->mdlId[0] = characterModelId;
    Hu3DModelAttrSet(characterModelId, HU3D_MOTATTR_LOOP);
    Hu3DModelShadowSet(characterModelId);

    for (motionIndex = 0; motionIndex < 10; motionIndex++) {
        if ((playerMotionIds[motionIndex] & 0xFFFF0000) != 0) {
            playerObject->mtnId[motionIndex] = CharMotionCreate(playerState->characterId, playerState->characterId + playerMotionIds[motionIndex]);
        }
        else {
            playerObject->mtnId[motionIndex] = CharMotionCreate(playerState->characterId, playerMotionIds[motionIndex]);
        }
    }
    CharMotionDataClose(playerState->characterId);
    CharMotionSet(playerState->characterId, playerObject->mtnId[playerState->paintGridX]);
    CharModelStepFxSet(playerState->characterId, 1);
    CharMotionVoiceOnSet(playerState->characterId, playerObject->mtnId[2], 0);
    playerObject->mdlId[2] = characterModelId = Hu3DModelCreateFile(playerState->playerIndex + 0x2E0003);
    playerObject->mdlId[3] = Hu3DJointMotion(characterModelId, HuDataSelHeapReadNum(0x2E0007, 0x10000000, HEAP_MODEL));
    playerObject->mdlId[4] = Hu3DJointMotion(characterModelId, HuDataSelHeapReadNum(0x2E0008, 0x10000000, HEAP_MODEL));
    playerObject->mdlId[5] = Hu3DJointMotion(characterModelId, HuDataSelHeapReadNum(0x2E0009, 0x10000000, HEAP_MODEL));
    Hu3DModelPosSet(characterModelId, playerState->position.x, playerState->position.y, playerState->position.z);
    Hu3DModelHookSet(characterModelId, "stamp00", playerObject->mdlId[0]);
    Hu3DMotionSet(playerObject->mdlId[2], playerObject->mdlId[4]);
    Hu3DModelShadowSet(characterModelId);
    playerObject->objFunc = PlayerCharacterIntroUpdate;
}

void PlayerCharacterIntroUpdate(OMOBJ *playerObject)
{
    s32 characterModelId;
    PlayerCharacterState *playerState;

    characterModelId = playerObject->mdlId[0];
    playerState = (PlayerCharacterState *)playerObject->data;

    playerState->currentGridX = playerState->currentGridZ = 0;
    playerState->targetGridX = playerState->targetGridZ = 0;
    switch (introState) {
        case 1:
            break;
        case 2:
            if (introWaitFrames == 0) {
                playerState->paintGridX = 6;
                playerState->previousPosition.y = 0.0f;
                playerState->effectFlags2 = 0;
            }
            break;
        case 4:
            playerObject->objFunc = PlayerCharacterStateUpdate;
            break;
    }
    if (playerState->position.y < 0.0f) {
        omVibrate(playerState->playerIndex, 0xC, 6, 6);
        HuAudFXPlay(0x5B0);
        playerState->position.y = 0.0f;
        playerState->effectFlags2 = 1;
    }
    PlayerCharacterUpdate(playerObject);
}

void PlayerCharacterStateUpdate(OMOBJ *playerObject)
{
    s32 characterModelId;
    s32 stampObjectId;
    PlayerCharacterState *playerState;

    characterModelId = playerObject->mdlId[0];
    playerState = (PlayerCharacterState *)playerObject->data;
    if (playerState->isActive != 0) {
        if ((introState == 5) && ((playerState->isActive & 2) == 0)) {
            if (playerState->stateFlags1 != 0) {
                PlayerCharacterMove(playerObject);
            }
            else {
                playerState->currentGridX = HuPadStkX[playerState->stateFlags0];
                playerState->currentGridZ = HuPadStkY[playerState->stateFlags0];
                playerState->targetGridX = HuPadBtnDown[playerState->stateFlags0];
                playerState->targetGridZ = HuPadBtn[playerState->stateFlags0];
            }
        }
        else {
            playerState->currentGridX = playerState->currentGridZ = 0;
            playerState->targetGridX = playerState->targetGridZ = 0;
        }
        switch (introState) {
            case 4:
            case 5:
                break;
            case 6:
                if ((Hu3DModelAttrGet(playerObject->mdlId[2]) & 1) == 0) {
                    Hu3DModelHookReset(playerObject->mdlId[2]);
                    Hu3DModelAttrSet(playerObject->mdlId[2], HU3D_ATTR_DISPOFF);
                    omSetTra(playerObject, playerState->position.x, playerState->position.y, playerState->position.z);
                    omSetRot(playerObject, playerState->rotation.x, playerState->rotation.y, playerState->rotation.z);
                    stampObjectId = mapObjectCreate(1, 1);
                    MapObjectTextureRead(mapRenderer.activeObjectId, 0, 0x2E000C);
                    MapMeshPositionSet(mapRenderer.activeObjectId, mapRenderer.activeMeshId, playerState->position.x, 50.0f + playerState->position.y, playerState->position.z);
                    MapMeshScaleSet(mapRenderer.activeObjectId, mapRenderer.activeMeshId, 3.0f, 3.0f, 1.0f);
                    MapMeshFlagsEnable(mapRenderer.activeObjectId, mapRenderer.activeMeshId, 0x80000000);
                    MapMeshColorSet(mapRenderer.activeObjectId, mapRenderer.activeMeshId, 0xFF, 0xFF, 0xFF, 0xFF);
                    MapMeshTextureFlagsIndexedSet(mapRenderer.activeObjectId, mapRenderer.activeMeshId, mapRenderer.activeTextureSlot, 0x11);
                    MapMeshTextureAnimationSpeedIndexedSet(mapRenderer.activeObjectId, mapRenderer.activeMeshId, mapRenderer.activeTextureSlot, 0.5f);
                    if (playerState->effectFlags2 != 0) {
                        playerState->effectFlags2 = 0;
                        playerState->effectFlags1 = 1;
                        // technically 1606.0f / REFRESH_RATE_F, but i can't get the float errors
                        playerState->previousPosition.y = (VERSION_PAL) ? 32.120003f : 26.766666f;
                    }
                    playerState->paintGridX = 2;
                    playerState->position.y += 100.65;
                }
                break;
            case 7:
                playerState->turnSpeed = AngleInterpolate(playerState->turnSpeed, 0.0f, 0.9f);
                break;
            case 9:
                if (playerState->effectFlags4 != 0) {
                    playerState->paintGridX = 8;
                }
                else {
                    playerState->paintGridX = 9;
                }
                break;
        }
        if (playerState->position.y <= 0.0f) {
            playerState->position.y = 0.0f;
            playerState->effectFlags2 = 1;
        }
        PlayerCharacterUpdate(playerObject);
    }
}

void PlayerCharacterUpdate(OMOBJ *object)
{
    s16 targetGridX;
    PlayerCharacterState *playerState;
    s16 nextMotionId;
    s16 motionBlendMode;
    s16 movementInputX;
    s16 movementInputZ;
    s16 shiftMotionId;
    s16 movementInputButtons;
    s32 characterModelId;
    s32 stampObjectId;

    shiftMotionId = 0;
    characterModelId = object->mdlId[0];
    playerState = (PlayerCharacterState *)object->data;
    movementInputX = playerState->currentGridX;
    movementInputZ = playerState->currentGridZ;
    targetGridX = playerState->targetGridX;
    movementInputButtons = playerState->targetGridZ;
    playerState->shadowPosition = playerState->position;
    nextMotionId = playerState->paintGridX;
    motionBlendMode = 0;
    switch (playerState->paintGridX) {
        case 0:
            motionBlendMode = 1;
            break;
        case 1:
            if (playerState->movementSpeed > 0.1f) {
                nextMotionId = 1;
                motionBlendMode = 1;
                playerState->turnSpeed = playerState->rotation.y;
                playerState->effectScale++;
                if (playerState->effectScale >= (0.5f * playerState->effectTimer)) {
                    playerState->effectScale -= 0.5f * playerState->effectTimer;
                }
                playerState->movementSpeed *= 1.0 + (0.25 * cosd(90.0f * (playerState->effectScale / (0.5f * playerState->effectTimer))));
            }
            else {
                playerState->effectScale = 0.0f;
                playerState->movementSpeed = 0.0f;
                nextMotionId = 0;
                motionBlendMode = 1;
            }
            break;
        case 2:
            playerState->previousPosition.y += -146.0f / REFRESH_RATE;
            if (playerState->effectFlags2 != 0) {
                nextMotionId = 3;
                motionBlendMode = 0;
                playerState->previousPosition.y = 0.0f;
            }
            break;
        case 3:
            if (CharMotionEndCheck(playerState->characterId) != 0) {
                nextMotionId = 0;
                motionBlendMode = 1;
            }
            break;
        case 4:
            playerState->previousPosition.y = -146.0f / REFRESH_RATE;
            if (FABS(movementInputX) + FABS(movementInputZ) > 8) {
                playerState->turnSpeed = atan2d(movementInputX, -movementInputZ);
            }
            if ((movementInputButtons & 0x100) != 0) {
                playerState->effectFlags1 = 0U;
                nextMotionId = 5;
                motionBlendMode = 0;
                shiftMotionId = 3;
                if (FABS(movementInputX) + FABS(movementInputZ) > 8) {
                    playerState->turnSpeed = atan2d(movementInputX, -movementInputZ);
                    playerState->movementSpeed = 6.0f;
                }
            }
            break;
        case 5:
            if (playerState->effectFlags1 < 0x20) {
                playerState->effectFlags1++;
            }
            if (FABS(movementInputX) + FABS(movementInputZ) > 8) {
                playerState->turnSpeed = atan2d(movementInputX, -movementInputZ);
                playerState->movementSpeed = 6.0f;
            }
            else {
                playerState->movementSpeed = 0.0f;
            }
            if ((movementInputButtons & 0x100) == 0) {
                playerState->previousPosition.y = (146.0f / REFRESH_RATE * (2.5f + (0.009f * playerState->effectFlags1 * playerState->effectFlags1)));
                nextMotionId = 6;
                motionBlendMode = 0;
                shiftMotionId = 4;
                if (playerState->effectFlags1 < 0xC) {
                    HuAudFXPlay(0x5B1);
                }
                else if (playerState->effectFlags1 < 0x18) {
                    HuAudFXPlay(0x5B2);
                }
                else {
                    HuAudFXPlay(0x5B3);
                }
            }
            break;
        case 6:
            if ((Hu3DMotionEndCheck(object->mdlId[2]) == 0) && (Hu3DMotionShiftIDGet(object->mdlId[2]) >= 0)) {
                playerState->effectFlags2 = 1;
            }
            else {
                if (playerState->previousPosition.y >= 0.0f) {
                    playerState->effectFlags2 = 0U;
                }
                playerState->previousPosition.y += -146.0f / REFRESH_RATE;
                if (playerState->effectFlags2 != 0) {
                    if ((movementInputButtons & 0x100) != 0) {
                        nextMotionId = 4;
                        motionBlendMode = 1;
                    }
                    else {
                        nextMotionId = 7;
                        motionBlendMode = 0;
                    }
                    playerState->movementSpeed = 0.0f;
                    playerState->previousPosition.y = 0.0f;
                    shiftMotionId = 5;
                    PlayerPaintEffectDraw(playerState);
                    stampObjectId = mapObjectCreate(0x28, 1);
                    MapObjectTextureRead(stampObjectId, 0, 0x2E000D);
                    mapRenderer.objects[stampObjectId].animationCallback = &MapTextureAnimationUpdate;
                    MapObjectPositionSet(stampObjectId, playerState->position.x, playerState->position.y, playerState->position.z);
                    MapMeshesRenderModeSet(stampObjectId, 1);
                }
            }
            break;
        case 7:
            if ((movementInputButtons & 0x100) != 0) {
                nextMotionId = 4;
                motionBlendMode = 1;
            }
            else {
                if (Hu3DMotionTimeGet(characterModelId) <= 0.0f) {
                    nextMotionId = 4;
                    motionBlendMode = 1;
                }
                else {
                    if ((Hu3DMotionTimeGet(characterModelId) >= 10.0f) && (Hu3DMotionShiftIDGet(characterModelId) < 0)) {
                        Hu3DModelAttrSet(characterModelId, HU3D_MOTATTR_REV);
                    }
                }
            }
            break;
        case 8:
        case 9:
            motionBlendMode = 0;
            playerState->previousPosition.y = -146.0f / REFRESH_RATE;
            break;
    }
    playerState->rotation.y = AngleInterpolate(playerState->rotation.y, playerState->turnSpeed, 0.75f);
    playerState->previousPosition.x = (playerState->movementSpeed * sind(playerState->turnSpeed));
    playerState->previousPosition.z = (playerState->movementSpeed * cosd(playerState->turnSpeed));
    if ((playerState->effectFlags2 == 0) || (nextMotionId == 1)) {
        playerState->position.x += playerState->previousPosition.x;
        playerState->position.y += playerState->previousPosition.y;
        playerState->position.z += playerState->previousPosition.z;
    }
    playerState->paintGridX = nextMotionId;
    if (nextMotionId != playerState->paintGridZ) {
        playerState->paintGridZ = nextMotionId;
        CharMotionShiftSet(playerState->characterId, object->mtnId[playerState->paintGridX], 0.0f, 8.0f, motionBlendMode);
        playerState->effectTimer = CharMotionMaxTimeGet(playerState->characterId);
        if (shiftMotionId != 0) {
            Hu3DMotionShiftSet(object->mdlId[2], object->mdlId[shiftMotionId], 0.0f, 8.0f, HU3D_MOTATTR_NONE);
        }
    }
    if ((Hu3DModelAttrGet(object->mdlId[2]) & 1) != 0) {
        omSetTra(object, playerState->position.x, playerState->position.y, playerState->position.z);
        omSetRot(object, playerState->rotation.x, playerState->rotation.y, playerState->rotation.z);
    }
    else {
        omSetTra(object, 0.0f, 0.0f, 0.0f);
        omSetRot(object, 0.0f, 0.0f, 0.0f);
        Hu3DModelPosSet(object->mdlId[2], playerState->position.x, playerState->position.y, playerState->position.z);
        Hu3DModelRotSet(object->mdlId[2], playerState->rotation.x, playerState->rotation.y, playerState->rotation.z);
    }
}

void PlayerCharacterMove(OMOBJ *object)
{
    s16 tilePaintCounts[3][3];
    Vec candidateCellPosition;
    Vec offsetFromPlayer;
    Vec forwardPosition;
    s32 characterModelId;
    s32 canvasPixelOffset;
    s16 currentMotionId;
    f32 arenaBoundary;
    f32 candidateDistance;
    f32 nearestDistance;
    PlayerCharacterState *playerState;
    s32 gridXIndex;
    s32 gridZIndex;
    u8 cellX;
    u8 cellZ;
    s32 lowestTilePaintCount;
    s32 highestTilePaintCount;
    s16 movementInputX;
    s16 movementInputZ;
    PlayerCharacterState *playerState2;
    u8 selectedCellX;
    u8 selectedCellZ;
    s16 targetGridX;
    s16 targetGridZ;
    s16 playerDifficulty;

    playerState = (PlayerCharacterState *)object->data;
    characterModelId = object->mdlId[0];
    currentMotionId = playerState->paintGridX;
    playerDifficulty = playerState->stateFlags2;
    movementInputX = movementInputZ = targetGridX = targetGridZ = 0;
    switch (playerState->movementState) {
        case 0:
            cellX = ((0.5f * (600.0f + playerState->position.x)) / 150.0f);
            cellZ = ((0.5f * (600.0f + playerState->position.z)) / 150.0f);
            if (playerPaintCellCounts[playerState->playerIndex][cellX][cellZ] < 16875.0f) {
                playerState->paintCellX = cellX;
                playerState->paintCellZ = cellZ;
                playerState->movementState++;
            }
            else {
                lowestTilePaintCount = 0x57E4;
                for (gridXIndex = cellX - 1; gridXIndex <= (cellX + 1); gridXIndex++) {
                    if ((gridXIndex >= 0) && (gridXIndex < 4)) {
                        for (gridZIndex = cellZ - 1; gridZIndex <= (cellZ + 1); gridZIndex++) {
                            if ((gridZIndex >= 0) && (gridZIndex < 4) && (playerPaintCellCounts[playerState->playerIndex][gridXIndex][gridZIndex] <= lowestTilePaintCount)
                                && ((lowestTilePaintCount != playerPaintCellCounts[playerState->playerIndex][gridXIndex][gridZIndex]) || ((MinigameRandom() & 1) == 0))) {

                                selectedCellX = gridXIndex;
                                selectedCellZ = gridZIndex;
                                lowestTilePaintCount = playerPaintCellCounts[playerState->playerIndex][gridXIndex][gridZIndex];
                            }
                        }
                    }
                }
                if (lowestTilePaintCount < 16875.0f) {
                    playerState->paintCellX = selectedCellX;
                    playerState->paintCellZ = selectedCellZ;
                    playerState->movementState++;
                }
                else {
                    lowestTilePaintCount = 0x57E4;
                    for (gridXIndex = 0; gridXIndex < 4; gridXIndex++) {
                        for (gridZIndex = 0; gridZIndex < 4; gridZIndex++) {
                            if (playerPaintCellCounts[playerState->playerIndex][gridXIndex][gridZIndex] <= lowestTilePaintCount) {
                                selectedCellX = gridXIndex;
                                selectedCellZ = gridZIndex;
                                lowestTilePaintCount = playerPaintCellCounts[playerState->playerIndex][gridXIndex][gridZIndex];
                            }
                        }
                    }
                    playerState->paintCellX = selectedCellX;
                    playerState->paintCellZ = selectedCellZ;
                    playerState->movementState++;
                }
            }
            break;
        case 1:
            nearestDistance = 360000.0f;
            forwardPosition = playerState->position;
            forwardPosition.x += 50.0f * sind(playerState->rotation.y);
            forwardPosition.z += 50.0f * cosd(playerState->rotation.y);

            for (gridXIndex = 0; gridXIndex < 3.0f; gridXIndex++) {
                for (gridZIndex = 0; gridZIndex < 3.0f; gridZIndex++) {
                    tilePaintCounts[gridXIndex][gridZIndex] = 0;
                }
            }

            for (gridXIndex = 0; gridXIndex < 150.0f; gridXIndex++) {
                for (gridZIndex = 0; gridZIndex < 150.0f; gridZIndex++) {
                    candidateCellPosition.x = (150.0f * playerState->paintCellX) + gridXIndex;
                    candidateCellPosition.z = (150.0f * playerState->paintCellZ) + gridZIndex;
                    canvasPixelOffset = (candidateCellPosition.x + (600.0f * candidateCellPosition.z));
                    if ((playerState->playerIndex + 1) != canvasPixels[canvasPixelOffset]) {
                        tilePaintCounts[(s16)(gridXIndex / 50.0f)][(s16)(gridZIndex / 50.0f)]++;
                    }
                }
            }
            cellX = (((0.5f * (600.0f + playerState->position.x)) - (150.0f * playerState->paintCellX)) / 50.0f);
            cellZ = (((0.5f * (600.0f + playerState->position.x)) - (150.0f * playerState->paintCellZ)) / 50.0f);
            highestTilePaintCount = 0;
            for (gridXIndex = cellX - 1; gridXIndex <= (cellX + 1); gridXIndex++) {
                if (gridXIndex >= 0) {
                    if (gridXIndex >= 3.0f)
                        continue;
                    for (gridZIndex = cellZ - 1; gridZIndex <= (cellZ + 1); gridZIndex++) {
                        if (gridZIndex >= 0) {
                            if (gridZIndex >= 3.0f)
                                continue;
                            if (tilePaintCounts[gridXIndex][gridZIndex] < 1250.0f)
                                continue;
                            if (tilePaintCounts[gridXIndex][gridZIndex] < highestTilePaintCount)
                                continue;
                            if ((highestTilePaintCount != tilePaintCounts[gridXIndex][gridZIndex]) || ((MinigameRandom() & 1) == 0)) {
                                highestTilePaintCount = tilePaintCounts[gridXIndex][gridZIndex];
                                candidateCellPosition.x = 25.0f + ((150.0f * playerState->paintCellX) + (50.0f * gridXIndex));
                                candidateCellPosition.z = 25.0f + ((150.0f * playerState->paintCellZ) + (50.0f * gridZIndex));
                                candidateCellPosition.x = (2.0f * candidateCellPosition.x) - 600.0f;
                                candidateCellPosition.z = (2.0f * candidateCellPosition.z) - 600.0f;
                                candidateCellPosition.y = 0.0f;
                                playerState->collisionPosition.x = candidateCellPosition.x;
                                playerState->collisionPosition.y = candidateCellPosition.y;
                                playerState->collisionPosition.z = candidateCellPosition.z;
                            }
                        }
                    }
                }
            }
            if (highestTilePaintCount == 0) {
                highestTilePaintCount = 0;

                for (gridXIndex = 0; gridXIndex < 3.0f; gridXIndex++) {
                    for (gridZIndex = 0; gridZIndex < 3.0f; gridZIndex++) {
                        if (tilePaintCounts[gridXIndex][gridZIndex] >= highestTilePaintCount) {
                            candidateCellPosition.x = 25.0f + ((150.0f * playerState->paintCellX) + (50.0f * gridXIndex));
                            candidateCellPosition.z = 25.0f + ((150.0f * playerState->paintCellZ) + (50.0f * gridZIndex));
                            candidateCellPosition.x = (2.0f * candidateCellPosition.x) - 600.0f;
                            candidateCellPosition.z = (2.0f * candidateCellPosition.z) - 600.0f;
                            candidateCellPosition.y = 0.0f;
                            VECSubtract(&forwardPosition, &candidateCellPosition, &offsetFromPlayer);
                            candidateDistance = VECMag(&offsetFromPlayer);
                            if ((highestTilePaintCount != tilePaintCounts[gridXIndex][gridZIndex]) || !(candidateDistance > nearestDistance)) {
                                if (!(candidateDistance > nearestDistance) || !(tilePaintCounts[gridXIndex][gridZIndex] < (1.5f * highestTilePaintCount))) {
                                    playerState->collisionPosition.x = candidateCellPosition.x;
                                    playerState->collisionPosition.y = candidateCellPosition.y;
                                    playerState->collisionPosition.z = candidateCellPosition.z;
                                    highestTilePaintCount = tilePaintCounts[gridXIndex][gridZIndex];
                                    nearestDistance = candidateDistance;
                                }
                            }
                        }
                    }
                }
            }
            playerState->movementFrames = (1.0f + ((5 - playerDifficulty) * (MinigameRandom() / 65536.0f)));
            playerState->collisionFrames = (1.0f + ((0x32 - (playerDifficulty * 0x10)) * (MinigameRandom() / 65536.0f)));
            playerState->movementState++;
            break;
        case 2:
            if (--playerState->collisionFrames == 0) {
                VECSubtract(&playerState->collisionPosition, &playerState->position, &offsetFromPlayer);
                VECNormalize(&offsetFromPlayer, &offsetFromPlayer);
                movementInputX = (32.0f * offsetFromPlayer.x);
                movementInputZ = (32.0f * -offsetFromPlayer.z);
                targetGridX = targetGridZ = 0x100;
                playerState->effectFrames = 0x18 + (s32)(((20 - (playerDifficulty * 6)) * ((MinigameRandom() - 0x8000) / 32768.0f)));
                playerState->movementState++;
            }
            break;
        case 3:
            movementInputX = playerState->currentGridX;
            movementInputZ = playerState->currentGridZ;
            targetGridX = targetGridZ = 0x100;
            if (--playerState->effectFrames == 0) {
                playerState->movementState++;
            }
            break;
        case 4:
            if (currentMotionId != 7) {
                movementInputX = playerState->currentGridX;
                movementInputZ = playerState->currentGridZ;
            }
            else {
                for (gridXIndex = 0; gridXIndex < 4; gridXIndex++) {
                    playerState2 = playerObjects[gridXIndex]->data;
                    if (playerState2 == playerState)
                        continue;
                    cellX = ((0.5f * (600.0f + playerState2->position.x)) / 150.0f);
                    cellZ = ((0.5f * (600.0f + playerState2->position.z)) / 150.0f);
                    if (playerState->paintCellX == cellX && playerState->paintCellZ == cellZ)
                        break;
                }
                if (gridXIndex != 4) {
                    lowestTilePaintCount = 0x57E4;
                    for (gridXIndex = 0; gridXIndex < 4; gridXIndex++) {
                        for (gridZIndex = 0; gridZIndex < 4; gridZIndex++) {
                            if ((playerPaintCellCounts[playerState->playerIndex][gridXIndex][gridZIndex] <= lowestTilePaintCount)
                                && ((playerState->paintCellX != gridXIndex) || (playerState->paintCellZ != gridZIndex))) {
                                selectedCellX = gridXIndex;
                                selectedCellZ = gridZIndex;
                                lowestTilePaintCount = playerPaintCellCounts[playerState->playerIndex][gridXIndex][gridZIndex];
                            }
                        }
                    }
                    playerState->paintCellX = selectedCellX;
                    playerState->paintCellZ = selectedCellZ;
                    playerState->movementState = 1;
                }
                else {
                    movementInputX = (32.0 * sind(playerState->rotation.y));
                    movementInputZ = (32.0 * -cosd(playerState->rotation.y));
                    arenaBoundary = 517.5f;
                    if ((playerState->position.x > arenaBoundary) || (playerState->position.x < -arenaBoundary)) {
                        movementInputX = -movementInputX;
                    }
                    arenaBoundary = 517.5f;
                    if ((playerState->position.z > arenaBoundary) || (playerState->position.z < -arenaBoundary)) {
                        movementInputZ = -movementInputZ;
                    }
                    if (--playerState->movementFrames != 0) {
                        targetGridX = targetGridZ = 0x100;
                        playerState->effectFrames = 0x18 + (s32)(((20 - (playerDifficulty * 6)) * ((MinigameRandom() - 0x8000) / 32768.0f)));
                        playerState->movementState = 3;
                    }
                    else {
                        cellX = playerState->paintCellX;
                        cellZ = playerState->paintCellZ;
                        if (playerPaintCellCounts[playerState->playerIndex][cellX][cellZ] > 16875.0f) {
                            playerState->movementState = 0;
                        }
                        else {
                            playerState->movementState = 1;
                        }
                    }
                }
            }
            break;
    }
    playerState->currentGridX = movementInputX;
    playerState->currentGridZ = movementInputZ;
    playerState->targetGridX = targetGridX;
    playerState->targetGridZ = targetGridZ;
}

void MapTextureAnimationUpdate(MapObject *mapObject)
{
    s16 animationPassState;
    f32 animationProgress;
    f32 meshScale;
    f32 animationFade;
    f32 rotationStepDegrees;
    s32 meshIndex;
    MapMesh *mapMesh;

    animationPassState = 0;
    switch (mapObject->renderMode) {
        case 0:
            rotationStepDegrees = 360.0f / mapObject->meshCount;
            mapMesh = mapObject->meshes;
            for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++, mapMesh++) {
                mapMesh->rotationOffset.x = sind(rotationStepDegrees * meshIndex) * (0.5f + (0.1f * (1.2f * ((MinigameRandom() - 0x8000) / 32768.0f))));
                mapMesh->rotationOffset.y = 0.0f;
                mapMesh->rotationOffset.z = cosd(rotationStepDegrees * meshIndex) * (0.5f + (0.1f * (1.2f * ((MinigameRandom() - 0x8000) / 32768.0f))));
                mapMesh->position.x = 65.0f * mapMesh->rotationOffset.x;
                mapMesh->position.y = 6.0f;
                mapMesh->position.z = 65.0f * mapMesh->rotationOffset.z;
            }
            mapObject->animationTimer = REFRESH_RATE / 2;
            mapObject->renderMode++;
        case 1:
            mapObject->animationTimer--;
            animationProgress = ((REFRESH_RATE_F / 2) - mapObject->animationTimer) / (REFRESH_RATE_F / 2);
            animationFade = mapObject->animationTimer / (REFRESH_RATE_F / 2);
            mapMesh = mapObject->meshes;
            for (meshIndex = 0; meshIndex < mapObject->meshCount; meshIndex++, mapMesh++) {
                mapMesh->position.x += 5.0f * animationFade * mapMesh->rotationOffset.x;
                mapMesh->position.z += 5.0f * animationFade * mapMesh->rotationOffset.z;
                meshScale = 0.4f + (1.2f * animationProgress);
                mapMesh->scale.x = meshScale;
                mapMesh->scale.y = meshScale;
                mapMesh->scale.z = meshScale;
                mapMesh->materialColor.r = 0xFF;
                mapMesh->materialColor.g = 0xFF;
                mapMesh->materialColor.b = 0xFF;
                mapMesh->materialColor.a = 96.0f * (1.0f - (animationProgress * animationProgress));
            }
            if (mapObject->animationTimer == 0) {
                mapObject->renderMode++;
            }
            break;
        case 2:
            MapObjectDestroy(mapObject->objectIndex);
            return;
    }
}

void PlayerCharacterShadowUpdate(OMOBJ *playerObject)
{
    Vec movementDelta;
    Vec separationDirection;
    Vec playerPosition;
    Vec otherPlayerPosition;
    f32 overlapDistance;
    f32 arenaLimit;
    f32 selfSeparationRatio;
    f32 selfMovementDistance;
    f32 otherMovementDistance;
    s32 otherPlayerIndex;
    PlayerCharacterState *otherPlayerState;
    PlayerCharacterState *otherPlayerState;

    otherPlayerState = (PlayerCharacterState *)playerObject->data;
    if (otherPlayerState->isActive != 0) {
        VECSubtract(&otherPlayerState->position, &otherPlayerState->shadowPosition, &movementDelta);
        selfMovementDistance = VECMag(&movementDelta);
        if (selfMovementDistance != 0.0f) {
            for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                if (otherPlayerIndex != otherPlayerState->playerIndex) {
                    otherPlayerState = (PlayerCharacterState *)playerObjects[otherPlayerIndex]->data;
                    if (otherPlayerState->isActive != 0) {
                        playerPosition.x = otherPlayerState->position.x;
                        playerPosition.y = otherPlayerState->position.y;
                        playerPosition.z = otherPlayerState->position.z;
                        playerPosition.y = 0.0f;
                        otherPlayerPosition.x = otherPlayerState->position.x;
                        otherPlayerPosition.y = otherPlayerState->position.y;
                        otherPlayerPosition.z = otherPlayerState->position.z;
                        otherPlayerPosition.y = 0.0f;
                        VECSubtract(&playerPosition, &otherPlayerPosition, &movementDelta);
                        overlapDistance = VECMag(&movementDelta);
                        if (overlapDistance >= 109.45f)
                            continue;
                        VECSubtract(&otherPlayerState->position, &otherPlayerState->shadowPosition, &movementDelta);
                        otherMovementDistance = VECMag(&movementDelta);
                        VECSubtract(&playerPosition, &otherPlayerPosition, &separationDirection);
                        VECNormalize(&separationDirection, &separationDirection);
                        overlapDistance = 110.0f - overlapDistance;
                        selfSeparationRatio = ((0.6f * otherMovementDistance) + (0.4f * selfMovementDistance)) / (selfMovementDistance + otherMovementDistance);
                        otherPlayerState->position.x += selfSeparationRatio * (overlapDistance * separationDirection.x);
                        otherPlayerState->position.y += selfSeparationRatio * (overlapDistance * separationDirection.y);
                        otherPlayerState->position.z += selfSeparationRatio * (overlapDistance * separationDirection.z);
                        selfSeparationRatio = ((0.6f * selfMovementDistance) + (0.4f * selfMovementDistance)) / (selfMovementDistance + otherMovementDistance);
                        otherPlayerState->position.x += selfSeparationRatio * (overlapDistance * -separationDirection.x);
                        otherPlayerState->position.y += selfSeparationRatio * (overlapDistance * -separationDirection.y);
                        otherPlayerState->position.z += selfSeparationRatio * (overlapDistance * -separationDirection.z);
                        PlayerCharacterShadowUpdate(playerObjects[otherPlayerIndex]);
                    }
                }
            }
            arenaLimit = 555.0f;
            if (otherPlayerState->position.x > arenaLimit) {
                otherPlayerState->position.x = arenaLimit;
            }
            if (otherPlayerState->position.x < -arenaLimit) {
                otherPlayerState->position.x = -arenaLimit;
            }

            arenaLimit = 555.0f;
            if (otherPlayerState->position.z > arenaLimit) {
                otherPlayerState->position.z = arenaLimit;
            }
            if (otherPlayerState->position.z < -arenaLimit) {
                otherPlayerState->position.z = -arenaLimit;
            }
        }
    }
}

void PlayerCharactersCleanup(OMOBJ *object)
{
    s32 itemIndex;

    if (introState >= 5) {
        for (itemIndex = 0; itemIndex < 4; itemIndex++) {
            PlayerCharacterShadowUpdate(playerObjects[itemIndex]);
        }
    }
}

void TemporaryAllocationsInitialize(void)
{
    s32 index;

    for (index = 0; index < 0x80; index++) {
        temporaryAllocations[index] = NULL;
    }
}

void *TemporaryAllocate(s32 allocationSize)
{
    s32 index;

    for (index = 0; index < 0x80; index++) {
        if (!temporaryAllocations[index])
            break;
    }
    if (index == 0x80) {
        return NULL;
    }
    temporaryAllocations[index] = HuMemDirectMallocNum(HEAP_HEAP, allocationSize, HU_MEMNUM_OVL);
    return temporaryAllocations[index];
}

void TemporaryFree(void *allocation)
{
    s32 index;

    for (index = 0; index < 0x80; index++) {
        if (temporaryAllocations[index] == allocation)
            break;
    }
    if (index != 0x80) {
        HuMemDirectFree(temporaryAllocations[index]);
        temporaryAllocations[index] = NULL;
    }
}

void TemporaryAllocationsFreeAll(void)
{
    s32 index;

    for (index = 0; index < 0x80; index++) {
        if (temporaryAllocations[index]) {
            OSReport("adr %x\n", temporaryAllocations[index]);
            HuMemDirectFree(temporaryAllocations[index]);
            temporaryAllocations[index] = NULL;
        }
    }
}

void ModelMaterialColorSet(s16 modelId, char *objectName, u8 red, u8 green, u8 blue)
{
    char objectNameBuffer[0x100];
    HSFOBJECT *modelObject;
    HSFMATERIAL *material2;
    HSFDATA *modelData;
    HSFBUFFER *modelBuffer;
    HSFOBJECT *modelObject2;
    s32 index;
    s32 index2;

    modelData = Hu3DData[modelId].hsf;
    modelObject2 = modelData->object;
    strcpy(&objectNameBuffer, MakeObjectName(objectName));

    for (index = 0; index < modelData->objectNum; index++, modelObject2++) {
        modelObject = modelObject2;
        modelBuffer = modelObject->mesh.face;
        if (modelObject->constData && strcmp(&objectNameBuffer, modelObject->name) == 0) {

            for (index2 = 0; index2 < modelBuffer->count; index2++) {
                material2 = &modelObject->mesh.material[((HSFFACE *)modelBuffer->data)[index2].mat & 0xFFF];
                material2->color[0] = red;
                material2->color[1] = green;
                material2->color[2] = blue;
            }
            return;
        }
    }
}

ANIMBMP *TextureAnimationBitmapGet(s16 textureAnimationId)
{
    return Hu3DTexAnimData[textureAnimationId].anim->bmp;
}

s16 ShadowTextureCreate(void)
{
    ANIMBMP *shadowBitmap;
    s32 textureAnimationId;
    ANIMDATA **animationHandle;
    s32 pixelBufferSize;

    textureAnimationId = MapTextureAnimationLoad(0xFFFF0000, Hu3DShadowData.size, Hu3DShadowData.size);
    MapTextureAnimationCreate(textureAnimationId, 7, Hu3DShadowData.size, Hu3DShadowData.size, Hu3DShadowData.size, Hu3DShadowData.size);
    animationHandle = mapTextureAnimationGet(textureAnimationId);
    shadowBitmap = (*animationHandle)->bmp;
    pixelBufferSize = shadowBitmap->sizeX * shadowBitmap->sizeY;
    memcpy(shadowBitmap->data, Hu3DShadowData.buf, pixelBufferSize);
    return textureAnimationId;
}

u32 randomSeed = 0x41C64E6D;

s32 MinigameRandom(void)
{
    randomSeed *= 0x41C64E6D;
    randomSeed += 0x3039;
    return randomSeed >> 0x10;
}

f32 AngleInterpolate(f32 currentAngle, f32 targetAngle, f32 blendFactor)
{
    f32 angleDelta;

    if (currentAngle > 180.0f) {
        currentAngle -= 360.0f;
    }
    else if (currentAngle <= -180.0f) {
        currentAngle += 360.0f;
    }
    if (targetAngle > 180.0f) {
        targetAngle -= 360.0f;
    }
    else if (targetAngle <= -180.0f) {
        targetAngle += 360.0f;
    }
    angleDelta = currentAngle - targetAngle;
    if (angleDelta > 180.0f) {
        angleDelta -= 360.0f;
    }
    else if (angleDelta <= -180.0f) {
        angleDelta += 360.0f;
    }
    currentAngle = targetAngle + (angleDelta * blendFactor);
    if (currentAngle > 180.0f) {
        return currentAngle - 360.0f;
    }
    if (currentAngle <= -180.0f) {
        currentAngle += 360.0f;
    }
    return currentAngle;
}

void RotationMatrixCreate(Mtx rotationMatrix, f32 xRotation, f32 yRotation, f32 zRotation)
{
    Mtx xRotationMatrix;
    Mtx yRotationMatrix;

    if (zRotation != 0.0f) {
        MTXRotDeg(rotationMatrix, 0x5A, zRotation);
    }
    else {
        MTXIdentity(rotationMatrix);
    }
    if (xRotation != 0.0f) {
        MTXRotDeg(xRotationMatrix, 0x58, xRotation);
        MTXConcat(xRotationMatrix, rotationMatrix, rotationMatrix);
    }
    if (yRotation != 0.0f) {
        MTXRotDeg(yRotationMatrix, 0x59, yRotation);
        MTXConcat(yRotationMatrix, rotationMatrix, rotationMatrix);
    }
}

void RotationMatrixToEuler(Mtx rotationMatrix, Vec *eulerAngles)
{
    f32 sinePitch;
    f32 cosinePitch;

    eulerAngles->x = atan2d(rotationMatrix[2][2], rotationMatrix[1][2]);
    eulerAngles->z = atan2d(rotationMatrix[0][0], rotationMatrix[0][1]);
    sinePitch = -rotationMatrix[0][2];
    cosinePitch = sqrtf(ABS(1.0 - (sinePitch * sinePitch)));
    if ((eulerAngles->x > 90.0f) && (eulerAngles->x < 270.0f) && (eulerAngles->z > 90.0f) && (eulerAngles->z < 270.0f)) {
        eulerAngles->x = fmod((180.0f + eulerAngles->x), 360.0);
        eulerAngles->z = fmod((180.0f + eulerAngles->z), 360.0);
        cosinePitch = -cosinePitch;
    }
    eulerAngles->y = atan2d(cosinePitch, sinePitch);
}

f32 BezierScalarEvaluate(f32 parameter, f32 startValue, f32 controlValue, f32 endValue)
{
    f32 remainingWeight;
    f32 interpolatedValue;

    remainingWeight = (1.0 - parameter);
    interpolatedValue = (endValue * (parameter * parameter)) + ((startValue * (remainingWeight * remainingWeight)) + (2.0 * (controlValue * (remainingWeight * parameter))));
    return interpolatedValue;
}

void BezierVectorEvaluate(f32 *startValues, f32 *controlValues, f32 *endValues, f32 *outputValues, f32 parameter)
{
    s32 axisIndex;

    for (axisIndex = 0; axisIndex < 3; axisIndex++) {
        *outputValues++ = BezierScalarEvaluate(parameter, *startValues++, *controlValues++, *endValues++);
    }
}

f32 HermiteTangentEvaluate(f32 parameter, f32 startTangent, f32 controlValue, f32 endTangent)
{
    f32 tangent;

    tangent = (2.0 * ((parameter * endTangent) + (((parameter - 1.0) * startTangent) + ((1.0 - (2.0 * parameter)) * controlValue))));
    return tangent;
}

void HermiteTangentNormalize(f32 *startValues, f32 *controlValues, f32 *endValues, f32 *outputTangent, f32 parameter)
{
    f32 tangentComponents[3];
    f32 tangentLength;
    s32 axisIndex;

    for (axisIndex = 0; axisIndex < 3; axisIndex++) {
        tangentComponents[axisIndex] = HermiteTangentEvaluate(parameter, *startValues++, *controlValues++, *endValues++);
    }
    tangentLength = sqrtf((tangentComponents[2] * tangentComponents[2]) + ((tangentComponents[0] * tangentComponents[0]) + (tangentComponents[1] * tangentComponents[1])));
    if (tangentLength) {
        tangentLength = (1.0 / tangentLength);
        for (axisIndex = 0; axisIndex < 3; axisIndex++) {
            *outputTangent++ = tangentLength * tangentComponents[axisIndex];
        }
        return;
    }
    *outputTangent++ = 0.0f;
    *outputTangent++ = 0.0f;
    *outputTangent++ = 1.0f;
}

void SplineSecondDerivativesCalculate(f32 *parameterValues, f32 *sampleValues, f32 *secondDerivatives, s32 sampleCount)
{
    f32 intervalWidths[0x10];
    f32 slopeValues[0x10];
    f32 eliminationFactor;
    s32 index;

    secondDerivatives[0] = 0.0f;
    secondDerivatives[sampleCount - 1] = 0.0f;

    for (index = 0; index < sampleCount - 1; index++) {
        intervalWidths[index] = parameterValues[index + 1] - parameterValues[index];
        if (intervalWidths[index] == 0.0f) {
            slopeValues[index + 1] = 0.0f;
        }
        else {
            slopeValues[index + 1] = (sampleValues[index + 1] - sampleValues[index]) / intervalWidths[index];
        }
    }
    secondDerivatives[1] = (slopeValues[2] - slopeValues[1]);
    slopeValues[1] = 2.0f * (parameterValues[2] - parameterValues[0]);

    for (index = 1; index < sampleCount - 2; index++) {
        eliminationFactor = intervalWidths[index] / slopeValues[index];
        secondDerivatives[index + 1] = ((slopeValues[index + 2] - slopeValues[index + 1]) - (eliminationFactor * (f64)secondDerivatives[index]));
        slopeValues[index + 1] = ((2.0f * (parameterValues[index + 2] - parameterValues[index])) - (eliminationFactor * (f64)intervalWidths[index]));
    }

    secondDerivatives[sampleCount - 2] -= intervalWidths[sampleCount - 2] * secondDerivatives[sampleCount - 1];

    for (index = sampleCount - 2; index > 0; index--) {
        if (0.0f == slopeValues[index]) {
            secondDerivatives[index] = 0.0f;
        }
        else {
            secondDerivatives[index] = (secondDerivatives[index] - (intervalWidths[index] * secondDerivatives[index + 1])) / slopeValues[index];
        }
    }
}

f32 SplineScalarEvaluate(f32 parameter, f32 *parameterValues, f32 *sampleValues, f32 *secondDerivatives, s32 sampleCount)
{
    f32 intervalWidth;
    f32 scaledParameter;
    f32 interpolatedValue;
    s32 intervalIndex;

    intervalIndex = (s32)parameter;
    parameter -= intervalIndex;
    scaledParameter = parameterValues[intervalIndex + 1] - parameterValues[intervalIndex];
    intervalWidth = scaledParameter * parameter;
    interpolatedValue = sampleValues[intervalIndex]
        + (intervalWidth
            * ((intervalWidth * ((3.0f * secondDerivatives[intervalIndex]) + ((intervalWidth * (secondDerivatives[intervalIndex + 1] - secondDerivatives[intervalIndex])) / scaledParameter)))
                + (((sampleValues[intervalIndex + 1] - sampleValues[intervalIndex]) / scaledParameter) - (scaledParameter * (2.0f * secondDerivatives[intervalIndex] + secondDerivatives[intervalIndex + 1])))));
    if (scaledParameter == 0.0f) {
        interpolatedValue = sampleValues[intervalIndex];
    }
    return interpolatedValue;
}

void SplineVectorSecondDerivativesCalculate(f32 *cumulativeDistances, Vec *sampleVectors, Vec *secondDerivativeVectors, s32 sampleCount)
{
    f32 componentValues[0x10];
    f32 secondDerivativeValues[0x10];
    f32 slopeValues[0x10];
    f32 intervalWidths[0x10];
    Vec vectorDifference;
    f32 eliminationFactor;
    s32 axisIndex;
    s32 index;
    s32 index2;

    cumulativeDistances[0] = 0.0f;

    for (index = 1; index < sampleCount; index++) {
        VECSubtract(&sampleVectors[index], &sampleVectors[index - 1], &vectorDifference);
        cumulativeDistances[index] = cumulativeDistances[index - 1] + VECMag(&vectorDifference);
    }

    for (axisIndex = 0; axisIndex < 3; axisIndex++) {
        for (index = 0; index < sampleCount; index++) {
            componentValues[index] = ((f32(*)[3])sampleVectors)[index][axisIndex];
        }
        secondDerivativeValues[0] = 0.0f;
        secondDerivativeValues[sampleCount - 1] = 0.0f;

        for (index2 = 0; index2 < (sampleCount - 1); index2++) {
            intervalWidths[index2] = cumulativeDistances[index2 + 1] - cumulativeDistances[index2];
            if (intervalWidths[index2] == 0.0f) {
                slopeValues[index2 + 1] = 0.0f;
            }
            else {
                slopeValues[index2 + 1] = (componentValues[index2 + 1] - componentValues[index2]) / intervalWidths[index2];
            }
        }
        secondDerivativeValues[1] = slopeValues[2] - slopeValues[1];
        slopeValues[1] = 2.0f * (cumulativeDistances[2] - cumulativeDistances[0]);

        for (index2 = 1; index2 < (sampleCount - 2); index2++) {
            eliminationFactor = intervalWidths[index2] / slopeValues[index2];
            secondDerivativeValues[index2 + 1] = ((slopeValues[index2 + 2] - slopeValues[index2 + 1]) - (eliminationFactor * (f64)secondDerivativeValues[index2]));
            slopeValues[index2 + 1] = ((2.0f * (cumulativeDistances[index2 + 2] - cumulativeDistances[index2])) - (eliminationFactor * (f64)intervalWidths[index2]));
        }
        secondDerivativeValues[sampleCount - 2] -= intervalWidths[sampleCount - 2] * secondDerivativeValues[sampleCount - 1];
        for (index2 = sampleCount - 2; index2 > 0; index2--) {
            if (slopeValues[index2] == 0.0f) {
                secondDerivativeValues[index2] = 0.0f;
            }
            else {
                secondDerivativeValues[index2] = (secondDerivativeValues[index2] - (intervalWidths[index2] * secondDerivativeValues[index2 + 1])) / slopeValues[index2];
            }
        }

        for (index = 0; index < sampleCount; index++) {
            ((f32(*)[3])secondDerivativeVectors)[index][axisIndex] = secondDerivativeValues[index];
            (void)secondDerivativeVectors;
        }
    }
}

void SplineVectorEvaluate(f32 parameter, Vec *outputVector, f32 *parameterValues, Vec *sampleVectors, Vec *secondDerivativeVectors, s32 sampleCount)
{
    f32 componentValues[0x10];
    f32 secondDerivativeValues[0x10];
    f32 intervalWidth;
    f32 scaledParameter;
    f32 interpolatedValue;
    s32 sampleIndex;
    s32 axisIndex;
    s32 index;

    for (axisIndex = 0; axisIndex < 3; axisIndex++) {
        for (index = 0; index < sampleCount; index++) {
            componentValues[index] = ((f32(*)[3])sampleVectors)[index][axisIndex];
            secondDerivativeValues[index] = ((f32(*)[3])secondDerivativeVectors)[index][axisIndex];
        }

        ((f32(*)[3])outputVector)[0][axisIndex] = SplineScalarEvaluate(parameter, parameterValues, componentValues, secondDerivativeValues, 0);
    }
}

f32 VectorNormalize(Vec *vector)
{
    f32 inverseLength;
    f32 length;

    length = (vector->z * vector->z) + ((vector->x * vector->x) + (vector->y * vector->y));
    length = sqrtf(length);

    if (length != 0.0f) {
        inverseLength = 1.0f / length;
        vector->x *= inverseLength;
        vector->y *= inverseLength;
        vector->z *= inverseLength;
    }
    else {
        vector->x = vector->y = vector->z = 0.0f;
    }
    return length;
}

f32 VectorOffsetInterpolate(Vec startPosition, Vec offset, Vec *outputPosition, f32 blendFactor)
{
    if (blendFactor <= 0.0f) {
        outputPosition->x = startPosition.x;
        outputPosition->y = startPosition.y;
        outputPosition->z = startPosition.z;
        blendFactor = 0.0f;
    }
    else if (blendFactor >= 1.0f) {
        outputPosition->x = startPosition.x + offset.x;
        outputPosition->y = startPosition.y + offset.y;
        outputPosition->z = startPosition.z + offset.z;
        blendFactor = 1.0f;
    }
    else {
        outputPosition->x = startPosition.x + (blendFactor * offset.x);
        outputPosition->y = startPosition.y + (blendFactor * offset.y);
        outputPosition->z = startPosition.z + (blendFactor * offset.z);
    }
    return blendFactor;
}

f32 LineProjectionParameter(Vec lineStart, Vec lineEnd, Vec lineDirection)
{
    f32 negativeDirectionLengthSquared;
    f32 projectionNumerator;

    projectionNumerator = (lineDirection.z * (lineEnd.z - lineStart.z)) + ((lineDirection.x * (lineEnd.x - lineStart.x)) + (lineDirection.y * (lineEnd.y - lineStart.y)));
    negativeDirectionLengthSquared = -((lineDirection.z * lineDirection.z) + ((lineDirection.x * lineDirection.x) + (lineDirection.y * lineDirection.y)));
    if (negativeDirectionLengthSquared != 0.0f) {
        projectionNumerator /= negativeDirectionLengthSquared;
    }
    return projectionNumerator;
}

f32 PointLineDistanceSquared(Vec *point, Vec *lineOrigin, Vec *lineDirection)
{
    Vec closestPoint;
    f32 projectionParameter;

    if (0.0f == ((lineDirection->z * lineDirection->z) + ((lineDirection->x * lineDirection->x) + (lineDirection->y * lineDirection->y)))) {
        return ((point->z - lineOrigin->z) * (point->z - lineOrigin->z))
            + (((point->x - lineOrigin->x) * (point->x - lineOrigin->x)) + ((point->y - lineOrigin->y) * (point->y - lineOrigin->y)));
    }

    projectionParameter = LineProjectionParameter(*point, *lineOrigin, *lineDirection);
    VectorOffsetInterpolate(*lineOrigin, *lineDirection, &closestPoint, projectionParameter);
    return ((point->z - closestPoint.z) * (point->z - closestPoint.z)) + (((point->x - closestPoint.x) * (point->x - closestPoint.x)) + ((point->y - closestPoint.y) * (point->y - closestPoint.y)));
}

f32 PointLineDistanceAndOffset(Vec *point, Vec *lineOrigin, Vec *lineDirection, Vec *closestPoint, Vec *offset)
{
    f32 distance;
    f32 projectionParameter;

    if (0.0f == ((lineDirection->z * lineDirection->z) + ((lineDirection->x * lineDirection->x) + (lineDirection->y * lineDirection->y)))) {
        return ((point->z - lineOrigin->z) * (point->z - lineOrigin->z))
            + (((point->x - lineOrigin->x) * (point->x - lineOrigin->x)) + ((point->y - lineOrigin->y) * (point->y - lineOrigin->y)));
    }
    else {
        projectionParameter = LineProjectionParameter(*point, *lineOrigin, *lineDirection);
        VectorOffsetInterpolate(*lineOrigin, *lineDirection, closestPoint, projectionParameter);
    }

    offset->x = closestPoint->x - point->x;
    offset->y = closestPoint->y - point->y;
    offset->z = closestPoint->z - point->z;

    return VectorNormalize(offset);
}

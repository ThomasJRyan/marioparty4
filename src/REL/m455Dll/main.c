#include "game/disp.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"

#include "game/pad.h"

#include "REL/m455Dll.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/minigame_seq.h"
#include "game/wipe.h"

#include "ext_math.h"
#include "math.h"
#include "version.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/hsfex.h"
#endif

typedef struct CameraViewParams {
    float zoom;
    Vec pos;
    Vec rot;
} CameraViewParams;

CameraViewParams cameraPresets[2] = { { 500, { 0, 1000, 0 }, { 40, 0, 0 } }, { 1985, { 0, 300, 0 }, { -6, 0, 0 } } };
Vec lightPosition = { 700, 2500, -700 };
Vec originPosition = { 0, 0, 0 };
GXColor lightColor = { 255, 255, 255, 255 };
Vec shadowPosition = { 200, 15000, -200 };
Vec shadowDirection = { 0, 1, 0 };

OMOBJ *overlayUpdateObject;
OMOBJ *outOfViewObject;
OMOBJ *mainSequenceObject;
s32 mainState;
s32 firstPlacePlayerIndex;
s32 stateTimer;
s32 nextPlayerIndex;
s16 countdownValue;
s16 countdownTickTimer;
s16 currentPlace;
s16 finishedPlayerCount;
s16 finishedPlayerIndices[4];
s16 playerStartTimer;
s16 playerActionTimer;
#if VERSION_PAL
s32 rumbleAllowed;
#endif
Vec lightDirection;
s16 minigameSequenceId;
s32 reservedState;

void MainStateUpdate(OMOBJ *object);
void PlayersCreate(HUPROCESS *objectManager);
void CameraCreate(HUPROCESS *objectManager);
s32 RumbleEnabledGet(void);

void ObjectSetup(void)
{
    HUPROCESS *objectManager;
    HU3DCAMERA *camera = &Hu3DCamera[0];
    s16 light;
    OSReport("******* M455ObjectSetup *********\n");
    objectManager = omInitObjMan(50, 8192);
    omGameSysInit(objectManager);
    Hu3DCameraCreate(HU3D_CAM0);
    Hu3DCameraPerspectiveSet(HU3D_CAM0, -1, 20, 25000, 1.2f);
    Hu3DCameraViewportSet(HU3D_CAM0, 0, 0, 640, 480, 0, 1);
    outOfViewObject = omAddObjEx(objectManager, 32730, 0, 0, -1, omOutView);
    lightDirection.x = originPosition.x - lightPosition.x;
    lightDirection.y = originPosition.y - lightPosition.y;
    lightDirection.z = originPosition.z - lightPosition.z;
    VECNormalize(&lightDirection, &lightDirection);
    light = Hu3DGLightCreateV(&lightPosition, &lightDirection, &lightColor);
    Hu3DGLightInfinitytSet(light);
    Hu3DGLightStaticSet(light, 1);
    Hu3DShadowCreate(10, camera->near, camera->far);
    Hu3DShadowTPLvlSet(0.7f);
    Hu3DShadowPosSet(&shadowPosition, &shadowDirection, &originPosition);
    HuAudSndGrpSet(77);
    mainSequenceObject = omAddObjEx(objectManager, 0, 0, 0, -1, MainStateUpdate);
    CameraCreate(objectManager);
    PlayersCreate(objectManager);
    StageObjectsCreate(objectManager);
}

void MainStateSet(s32 state)
{
    mainState = state;
}

s32 MainStateGet(void)
{
    return mainState;
}

void MainSequenceUpdate(OMOBJ *object);

void MainStateUpdate(OMOBJ *object)
{
    HU3DCAMERA *camera = &Hu3DCamera[0];
    mainState = 0;
    stateTimer = REFRESH_RATE;
    firstPlacePlayerIndex = -1;
    countdownValue = 30;
    countdownTickTimer = REFRESH_RATE;
    currentPlace = -1;
    finishedPlayerCount = 0;
    playerStartTimer = ((frand16() * (REFRESH_RATE_F * 4.5f)) / 65536.0f) + (REFRESH_RATE_F / 2);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    camera->fov = 30.0f;
    object->objFunc = MainSequenceUpdate;
}

void MainSequenceFinish(OMOBJ *object);
void PlayersUpdate(void);
s32 PlayersAllFinished(void);

typedef struct M455Player {
    u8 hasHookedItem : 1;
    u8 hasCompletedTurn : 1;
    u8 hasPlayedImpactEffects : 1;
    s16 playerIndex;
    s16 controllerIndex;
    s8 stickX;
    s8 stickY;
    u16 buttonsHeld;
    u16 buttonsDown;
    s32 rumbleEnabled;
    s16 characterId;
    s16 difficulty;
    s16 playerGroup;
    s16 turnState;
    s32 reservedState;
    s32 motionIndex;
    u32 motionFlags;
    s16 throwTimer;
    s16 throwDuration;
    s16 winningPlace;
    s16 promptSprite;
    s16 promptBlinkTimer;
    s16 promptBlinkDuration;
    s16 promptDisplayTime;
    Vec hookVertices[2];
    Vec hookStartPosition;
    s16 startDelay;
    s32 reservedValues[4];
} M455Player;

s16 countdownTimerSequence = -1;
s32 countdownMusicSequence = -1;
s32 countdownSoundEffect = -1;

void MainSequenceUpdate(OMOBJ *object)
{
    s32 i;
    OMOBJ **playerList;
    M455Player *player;

    switch (MainStateGet()) {
        case 0:
            if (countdownSoundEffect < 0) {
                countdownSoundEffect = HuAudFXPlay(1985);
            }
            if (--stateTimer == 0) {
                MainStateSet(1);
                stateTimer = REFRESH_RATE * 2;
            }
            break;

        case 1:
            if (--stateTimer == 0) {
                MainStateSet(2);
                minigameSequenceId = MGSeqStartCreate();
            }
            break;

        case 2:
            if (!MGSeqStatGet(minigameSequenceId)) {
                MainStateSet(3);
            }
            if (countdownMusicSequence < 0 && (MGSeqStatGet(minigameSequenceId) & 0x10)) {
                countdownMusicSequence = HuAudSeqPlay(60);
            }
            break;

        case 3:
            if (--countdownTickTimer == 0) {
                countdownValue--;
                if (countdownTimerSequence >= 0) {
                    MGSeqParamSet(countdownTimerSequence, 1, countdownValue);
                }
                countdownTickTimer = REFRESH_RATE;
                if (countdownValue == 10) {
                    countdownTimerSequence = MGSeqTimerCreate(10);
                }
            }
            if (playerStartTimer >= 0) {
                playerStartTimer--;
            }
            if (countdownValue == 0 || PlayersAllFinished()) {
                MainStateSet(4);
                minigameSequenceId = MGSeqFinishCreate();
                HuAudSeqFadeOut(countdownMusicSequence, 100);
            }
            break;

        case 4:
            if (countdownTimerSequence >= 0) {
                MGSeqParamSet(countdownTimerSequence, 2, -1);
                countdownTimerSequence = -1;
            }
            if (!MGSeqStatGet(minigameSequenceId)) {
                MainStateSet(7);
                stateTimer = REFRESH_RATE * 2;
            }
            break;

        case 7:
            if (--stateTimer == 0) {
                MainStateSet(9);
                if (firstPlacePlayerIndex >= 0) {
                    HuAudSStreamPlay(1);
                    playerList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
                    for (i = 0; i < 4; i++) {
                        player = playerList[i]->data;
                        if (player->winningPlace < 0) {
                            GWPlayerCoinWinSet(i, currentPlace + 1);
                        }
                    }
                }
                else {
                    HuAudSStreamPlay(4);
                    for (i = 0; i < 4; i++) {
                        GWPlayerCoinWinSet(i, 3);
                    }
                }
                stateTimer = REFRESH_RATE * 3.5;
            }
            break;

        case 9:
            if (--stateTimer == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
                HuAudFXStop(countdownSoundEffect);
                object->objFunc = MainSequenceFinish;
            }
            break;
    }
    if (finishedPlayerCount) {
        if (finishedPlayerCount >= 2) {
            for (i = 0; i < 100; i++) {
                s32 sourceIndex = (frand8() * finishedPlayerCount) >> 8;
                s32 destinationIndex = (frand8() * finishedPlayerCount) >> 8;
                s32 swappedPlayerIndex = finishedPlayerIndices[sourceIndex];
                finishedPlayerIndices[sourceIndex] = finishedPlayerIndices[destinationIndex];
                finishedPlayerIndices[destinationIndex] = swappedPlayerIndex;
            }
        }
        playerList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
        for (i = 0; i < finishedPlayerCount; i++) {
            currentPlace++;
            GWPlayerCoinWinSet(finishedPlayerIndices[i], currentPlace);
            player = playerList[finishedPlayerIndices[i]]->data;
            player->winningPlace = currentPlace;
        }
        finishedPlayerCount = 0;
    }
    if (omSysExitReq && !WipeStatGet()) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        object->objFunc = MainSequenceFinish;
    }
}

void MainSequenceFinish(OMOBJ *object)
{
    if (WipeStatGet()) {
        return;
    }
    PlayersUpdate();
    StageObjectsDestroy();
    MGSeqKillAll();
    HuAudFadeOut(1);
    omOvlReturnEx(1, 1);
    OSReport("******* M455Exit *********\n");
}

void PlayerCreate(OMOBJ *object);

void PlayersCreate(HUPROCESS *objectManager)
{
    s32 i;
    nextPlayerIndex = 0;
    playerActionTimer = 3;
#if VERSION_PAL
    rumbleAllowed = RumbleEnabledGet();
#endif
    omMakeGroupEx(objectManager, 0, 4);
    for (i = 0; i < 4; i++) {
        omAddObjEx(objectManager, 5, 10, 50, 0, PlayerCreate);
    }
}

void PlayerDestroy(OMOBJ *object);

void PlayersUpdate(void)
{
    s32 i;
    OMOBJ **playerList;
    playerList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    if (playerList == NULL) {
        return;
    }
    for (i = 0; i < 4; i++) {
        PlayerDestroy(playerList[i]);
    }
}

s32 PlayersAllFinished(void)
{
    s32 i;
    OMOBJ **playerList;
    playerList = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    for (i = 0; i < 4; i++) {
        M455Player *player = playerList[i]->data;
        if (!player->hasCompletedTurn) {
            return 0;
        }
    }
    return 1;
}

u32 characterMotionIds[5][2] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    0,
    0x540024,
    1,
    0x54001C,
    1,
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17),
    0,
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18),
    0,
};

Vec playerStartPositions[4] = {
    { -300, 0, -100 },
    { -100, 0, -100 },
    { 100, 0, -100 },
    { 300, 0, -100 },
};

void PlayerUpdate(OMOBJ *object);

void HookModelRender(HU3DMODEL *model, Mtx matrix);

void PlayerCreate(OMOBJ *object)
{
    M455Player *player;
    s32 characterId;
    u32 i;
    player = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M455Player), HU_MEMNUM_OVL);
    object->data = player;
    memset(player, 0, sizeof(M455Player));
    player->playerIndex = nextPlayerIndex++;
    player->controllerIndex = GWPlayerCfg[player->playerIndex].pad_idx;
#if VERSION_PAL
    player->rumbleEnabled = rumbleAllowed;
#else
    player->rumbleEnabled = RumbleEnabledGet();
#endif
    characterId = GWPlayerCfg[player->playerIndex].character;
    player->characterId = characterId;
    player->difficulty = GWPlayerCfg[player->playerIndex].diff;
    player->playerGroup = GWPlayerCfg[player->playerIndex].group;
    object->mdlId[0] = CharModelCreate(characterId, 8);
    Hu3DModelShadowSet(object->mdlId[0]);
    Hu3DModelLayerSet(object->mdlId[0], 3);
    Hu3DModelCameraSet(object->mdlId[0], HU3D_CAM0);
    object->mdlId[1] = Hu3DModelCreateFile(0x540002);
    Hu3DModelHookSet(object->mdlId[0], "test11_tex_we-itemhook-r", object->mdlId[1]);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_MOTATTR_SHAPE_PAUSE | HU3D_MOTATTR_PAUSE);
    player->motionIndex = 1;
    for (i = 0; i < 5; i++) {
        if (characterMotionIds[i][1] == 0) {
            object->mtnId[i] = CharMotionCreate(player->characterId, characterMotionIds[i][0]);
        }
        else {
            object->mtnId[i] = CharMotionCreate(player->characterId, player->characterId + characterMotionIds[i][0]);
        }
    }
    Hu3DMotionSet(object->mdlId[0], object->mtnId[player->motionIndex]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    object->trans.x = playerStartPositions[player->playerIndex].x;
    object->trans.y = playerStartPositions[player->playerIndex].y;
    object->trans.z = playerStartPositions[player->playerIndex].z;
    player->promptSprite = espEntry(0x54000C, 0x8000, 0);
    espDispOff(player->promptSprite);
    object->mdlId[3] = Hu3DHookFuncCreate(HookModelRender);
    Hu3DModelLayerSet(object->mdlId[3], 3);
    Hu3DModelCameraSet(object->mdlId[3], HU3D_CAM0);
    Hu3DData[object->mdlId[3]].hookData = object;
    object->mdlId[4] = Hu3DModelCreateFile(0x54000A);
    Hu3DModelLayerSet(object->mdlId[4], 3);
    Hu3DModelCameraSet(object->mdlId[4], HU3D_CAM0);
    Hu3DModelShadowSet(object->mdlId[4]);
    i = Hu3DMotionCreateFile(0x54000B);
    Hu3DMotionSet(object->mdlId[4], i);
    Hu3DModelAttrSet(object->mdlId[4], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(object->mdlId[4], HU3D_MOTATTR_PAUSE);
    player->hookStartPosition.x = object->trans.x + (50.0f * ((((s32)frand() & 0x1FF) - 256.0f) / 256.0f));
    player->hookStartPosition.y = -200;
    player->hookStartPosition.z = -1500;
    Hu3DModelPosSetV(object->mdlId[4], &player->hookStartPosition);
    Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
    player->hookVertices[0].x = object->trans.x;
    player->hookVertices[0].y = 0;
    player->hookVertices[0].z = -500;
    player->hookVertices[1].x = object->trans.x;
    player->hookVertices[1].y = -200;
    player->hookVertices[1].z = -1000;
    CharModelDataClose(player->characterId);
    object->rot.y = 180;
    if (((s32)frand() & 0x1F) < 4 - player->difficulty) {
        player->startDelay = playerStartTimer - (REFRESH_RATE_F * (0.05f * (4 - player->difficulty))) - ((frand8() / 256.0f) * (REFRESH_RATE_F / 10));
        if (player->startDelay < 0) {
            player->startDelay = 0;
        }
    }
    else {
        player->startDelay = playerStartTimer + (REFRESH_RATE_F * (0.1f * (4 - player->difficulty))) + ((frand8() / 256.0f) * (REFRESH_RATE_F / 2)) + (REFRESH_RATE_F / 10);
    }
    player->winningPlace = -1;
    object->objFunc = PlayerUpdate;
}

void PlayerTurnFinish(OMOBJ *object)
{
    M455Player *player = object->data;
    if (player->startDelay != 0) {
        player->startDelay--;
    }
    else {
        player->buttonsDown |= PAD_BUTTON_A;
    }
}

float playerLaneOffsets[4] = { -350, -170, 170, 350 };

float placeScaleFactors[3] = { 1.5f, 1.0f, 0.5f };

float AngleLerp(float startAngle, float endAngle, float weight);
s32 QuadraticBezierVector(Vec *controlPoints, Vec *result, float progress);

void PlayerUpdate(OMOBJ *object)
{
    M455Player *player;
    s32 motionIndex;
    u32 motionAttributes;
    Vec hookPath[3];
    Vec worldPosition;
    Vec hookDirection;
    float animationProgress;

    player = object->data;
    motionIndex = player->motionIndex;
    motionAttributes = player->motionFlags;
    object->trans.y = PlatformHeightGet();
    if (MainStateGet() < 3) {
        return;
    }
    switch (player->turnState) {
        case 0:
            if (MainStateGet() == 4 || playerActionTimer == 0) {
                player->hasCompletedTurn = 1;
                player->turnState = 2;
                espDispOff(player->promptSprite);
                return;
            }
            if (!GWPlayerCfg[player->playerIndex].iscom) {
                player->stickX = HuPadStkX[player->controllerIndex];
                player->stickY = HuPadStkY[player->controllerIndex];
                player->buttonsHeld = HuPadBtn[player->controllerIndex];
                player->buttonsDown = HuPadBtnDown[player->controllerIndex];
            }
            else {
                player->buttonsHeld = player->buttonsDown = 0;
                player->stickX = player->stickY = 0;
                PlayerTurnFinish(object);
            }
            if (playerStartTimer == 0) {
                if (player->rumbleEnabled) {
                    omVibrate(player->playerIndex, 12, 12, 0);
                }
                else {
                    worldPosition.x = object->trans.x;
                    worldPosition.y = object->trans.y + 300;
                    worldPosition.z = object->trans.z;
                    Hu3D3Dto2D(&worldPosition, HU3D_CAM0, &worldPosition);
                    espPosSet(player->promptSprite, worldPosition.x, worldPosition.y);
                    espDispOn(player->promptSprite);
                    player->promptDisplayTime = 0;
                }
                player->promptBlinkTimer = player->promptBlinkDuration = REFRESH_RATE / 2;
            }

            if (player->buttonsDown & PAD_BUTTON_A) {
                player->turnState = 1;
                player->throwTimer = player->throwDuration = REFRESH_RATE * 1.5;
                player->promptBlinkTimer = -1;
                if (playerStartTimer <= 0) {
                    finishedPlayerIndices[finishedPlayerCount++] = player->playerIndex;
                }
                Hu3DModelAttrReset(object->mdlId[1], HU3D_MOTATTR_SHAPE_PAUSE | HU3D_MOTATTR_PAUSE);
                Hu3DMotionSpeedSet(object->mdlId[1], 1.2f);
                Hu3DMotionShapeSpeedSet(object->mdlId[1], 1.2f);
                HuAudFXPlay(1986);
            }
            motionIndex = 1;
            motionAttributes = HU3D_MOTATTR_LOOP;
            break;

        case 1:
            motionIndex = 2;
            motionAttributes = HU3D_MOTATTR_NONE;
            if (!player->hasHookedItem) {
                if (player->winningPlace >= 0 && player->winningPlace < 3) {
                    player->hasHookedItem = 1;
                    if (player->winningPlace == 0) {
                        firstPlacePlayerIndex = player->playerIndex;
                    }
                    animationProgress = placeScaleFactors[player->winningPlace];
                    Hu3DModelScaleSet(object->mdlId[4], animationProgress, animationProgress, animationProgress);
                    Hu3DModelAttrReset(object->mdlId[4], HU3D_ATTR_DISPOFF);
                }
            }
            animationProgress = 1.0f - ((float)player->throwTimer / player->throwDuration);
            hookPath[0].x = player->hookStartPosition.x;
            hookPath[0].y = player->hookStartPosition.y;
            hookPath[0].z = player->hookStartPosition.z;
            hookPath[1].x = object->trans.x;
            hookPath[1].y = object->trans.y + 1500;
            hookPath[1].z = object->trans.z - 200;
            hookPath[2].x = playerLaneOffsets[player->playerIndex];
            hookPath[2].y = 0;
            hookPath[2].z = 150;
            QuadraticBezierVector(hookPath, &worldPosition, animationProgress);
            VECSubtract(&worldPosition, &Hu3DData[object->mdlId[4]].pos, &hookDirection);
            Hu3DModelRotSet(object->mdlId[4], atan2d(-hookDirection.y, VECMagXZ(&hookDirection)), atan2d(hookDirection.x, hookDirection.z), 0);
            Hu3DModelPosSetV(object->mdlId[4], &worldPosition);
            if (worldPosition.y > 0.0f) {
                if (!player->hasPlayedImpactEffects && player->hasHookedItem) {
                    SnowmanParticleSpawn(&worldPosition, -1);
                    SnowParticleSpawn(&worldPosition, -1, -1, -1);
                    player->hasPlayedImpactEffects = 1;
                }
            }
            player->hookVertices[0].x = animationProgress * (2.0f * playerLaneOffsets[player->playerIndex]);
            player->hookVertices[0].y = -200.0f + (-400.0f * animationProgress);
            player->hookVertices[0].z = -500.0f + (-1500.0f * animationProgress);
            player->hookVertices[1].x = animationProgress * playerLaneOffsets[player->playerIndex];
            player->hookVertices[1].y = -200.0f + (400.0f * animationProgress);
            player->hookVertices[1].z = -1000.0f + (750.0f * animationProgress);
            if (--player->throwTimer == 0) {
                player->turnState = 2;
                player->hasCompletedTurn = 1;
                if (player->hasHookedItem) {
                    Hu3DModelAttrReset(object->mdlId[4], HU3D_MOTATTR_PAUSE);
                    Hu3DModelRotSet(object->mdlId[4], 0, 0, 0);
                    playerActionTimer--;
                }
            }
            break;

        case 2:
            if (MainStateGet() == 7) {
                player->turnState = 3;
                Hu3DModelObjPosGet(object->mdlId[0], "test11_tex_we-itemhook-r", &worldPosition);
                CharEffectSmokeCreate(1, &worldPosition);
                Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
            }
            break;

        case 3:
            if (MainStateGet() < 9) {
                motionIndex = 0;
                motionAttributes = HU3D_MOTATTR_LOOP;
            }
            else {
                object->rot.y = AngleLerp(object->rot.y, 0, 0.2f);
                if (player->playerIndex == firstPlacePlayerIndex) {
                    motionIndex = 3;
                    motionAttributes = HU3D_MOTATTR_NONE;
                }
                else {
                    motionIndex = 4;
                    motionAttributes = HU3D_MOTATTR_NONE;
                }
            }
            break;

        default:
            break;
    }
    if (player->promptBlinkTimer) {
        if (player->promptBlinkTimer >= 0) {
            animationProgress = (0.5 * sind((90.0f * (float)player->promptBlinkTimer) / (float)player->promptBlinkDuration)) + 0.5;
            espScaleSet(player->promptSprite, animationProgress, animationProgress);
            if (--player->promptBlinkTimer == 0) {
                player->promptBlinkTimer = player->promptBlinkDuration;
            }
            player->promptDisplayTime++;
        }
        else {
            if (player->promptDisplayTime > REFRESH_RATE_F / 2) {
                espDispOff(player->promptSprite);
                player->promptBlinkTimer = 0;
            }
            else {
                player->promptDisplayTime++;
            }
        }
    }
    if (player->turnState >= 2 && player->turnState <= 3 && player->hasHookedItem) {
        Hu3DData[object->mdlId[4]].rot.y += 2.0f;
        if (0.0f == Hu3DData[object->mdlId[4]].motWork.time) {
            HuAudFXPlay(1987);
        }
    }
    if ((motionIndex >= 0 && motionIndex != player->motionIndex) || motionAttributes != player->motionFlags) {
        player->motionIndex = motionIndex;
        player->motionFlags = motionAttributes;
        CharMotionShiftSet(player->characterId, object->mtnId[player->motionIndex], 0, 8, motionAttributes);
    }
}

void PlayerDestroy(OMOBJ *object)
{
    M455Player *player = object->data;
    if (object->data != NULL) {
        HuMemDirectFree(object->data);
        object->data = NULL;
    }
}

GXColor hookModelColor = { 255, 255, 255, 255 };

void CubicBezierVector(Vec *controlPoints, Vec *result, float progress);

void HookModelRender(HU3DMODEL *model, Mtx matrix)
{
    Vec vtxPos[4];
    Mtx mtxHook;

    Vec pos;
    Vec itemHookPos;
    s32 i;
    OMOBJ *object = model->hookData;
    M455Player *player = object->data;
    HU3DMODEL *modelPuku = &Hu3DData[object->mdlId[4]];
    GXLoadPosMtxImm(matrix, GX_PNMTX0);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanMatColor(GX_COLOR0A0, hookModelColor);
    GXSetNumTexGens(0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXSetNumTevStages(1);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    Hu3DModelObjMtxGet(object->mdlId[0], "test11_tex_we-itemhook-r", mtxHook);
    Hu3DModelObjPosGet(object->mdlId[1], "hook", &pos);
    MTXMultVec(mtxHook, &pos, &vtxPos[0]);
    for (i = 0; i < 2; i++) {
        vtxPos[i + 1].x = player->hookVertices[i].x;
        vtxPos[i + 1].y = player->hookVertices[i].y;
        vtxPos[i + 1].z = player->hookVertices[i].z;
    }
    if (player->hasHookedItem) {
        Mtx hookMtx;
        Vec hookPos;
        Hu3DModelObjMtxGet(object->mdlId[4], "itemhook_C", hookMtx);
        hookPos.x = 0;
        hookPos.y = -60.000004f;
        hookPos.z = 35.0f;
        MTXMultVec(hookMtx, &hookPos, &vtxPos[3]);
    }
    else {
        vtxPos[3].x = modelPuku->pos.x;
        vtxPos[3].y = modelPuku->pos.y;
        vtxPos[3].z = modelPuku->pos.z;
    }
    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 20);
    for (i = 0; i < 20; i++) {
        CubicBezierVector(vtxPos, &pos, i / 19.0f);
        GXPosition3f32(pos.x, pos.y, pos.z);
    }
    GXEnd();
}

float AngleLerp(float startAngle, float endAngle, float weight)
{
    float angleDifference = fmod(endAngle - startAngle, 360);
    float interpolatedAngle;
    if (angleDifference < 0) {
        angleDifference += 360.0f;
    }
    if (angleDifference > 180) {
        angleDifference -= 360.0f;
    }
    interpolatedAngle = fmod(startAngle + (weight * angleDifference), 360);
    if (interpolatedAngle < 0) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

float QuadraticBezier(float startPoint, float controlPoint, float endPoint, float progress)
{
    float inverseProgress = 1.0f - progress;
    return (startPoint * (inverseProgress * inverseProgress)) + ((2.0f * inverseProgress) * progress * controlPoint) + (endPoint * (progress * progress));
}

s32 QuadraticBezierVector(Vec *controlPoints, Vec *result, float progress)
{
    result->x = QuadraticBezier(controlPoints[0].x, controlPoints[1].x, controlPoints[2].x, progress);
    result->y = QuadraticBezier(controlPoints[0].y, controlPoints[1].y, controlPoints[2].y, progress);
    result->z = QuadraticBezier(controlPoints[0].z, controlPoints[1].z, controlPoints[2].z, progress);
}

float CubicBezier(float progress, float startPoint, float controlPoint1, float controlPoint2, float endPoint)
{
    float inverseProgress = 1.0f - progress;
    float inverseProgressSquared = inverseProgress * inverseProgress;
    float inverseProgressCubed = inverseProgress * inverseProgress * inverseProgress;
    return (endPoint * (progress * (progress * progress))) + ((controlPoint2 * (progress * (3.0f * inverseProgress * progress))) + ((inverseProgressCubed * startPoint) + (controlPoint1 * (3.0f * inverseProgressSquared * progress))));
}

void CubicBezierVector(Vec *controlPoints, Vec *result, float progress)
{
    result->x = CubicBezier(progress, controlPoints[0].x, controlPoints[1].x, controlPoints[2].x, controlPoints[3].x);
    result->y = CubicBezier(progress, controlPoints[0].y, controlPoints[1].y, controlPoints[2].y, controlPoints[3].y);
    result->z = CubicBezier(progress, controlPoints[0].z, controlPoints[1].z, controlPoints[2].z, controlPoints[3].z);
}

float hookAnimationProgress[2] = {};

void CameraInterpolate(s32 startPreset, s32 endPreset, float progress);

void CameraUpdate(OMOBJ *object);

void CameraCreate(HUPROCESS *objectManager)
{
    CameraInterpolate(0, 0, 0);
    overlayUpdateObject = omAddObjEx(objectManager, 10, 0, 0, -1, CameraUpdate);
}

void CameraInterpolate(s32 startPreset, s32 endPreset, float progress)
{
    CZoom = cameraPresets[startPreset].zoom + (progress * (cameraPresets[endPreset].zoom - cameraPresets[startPreset].zoom));
    Center.x = cameraPresets[startPreset].pos.x + (progress * (cameraPresets[endPreset].pos.x - cameraPresets[startPreset].pos.x));
    Center.y = cameraPresets[startPreset].pos.y + (progress * (cameraPresets[endPreset].pos.y - cameraPresets[startPreset].pos.y));
    Center.z = cameraPresets[startPreset].pos.z + (progress * (cameraPresets[endPreset].pos.z - cameraPresets[startPreset].pos.z));
    CRot.x = cameraPresets[startPreset].rot.x + (progress * (cameraPresets[endPreset].rot.x - cameraPresets[startPreset].rot.x));
    CRot.y = cameraPresets[startPreset].rot.y + (progress * (cameraPresets[endPreset].rot.y - cameraPresets[startPreset].rot.y));
    CRot.z = cameraPresets[startPreset].rot.z + (progress * (cameraPresets[endPreset].rot.z - cameraPresets[startPreset].rot.z));
}

void CameraUpdate(OMOBJ *object)
{
    HU3DCAMERA *camera = &Hu3DCamera[0];
    switch (MainStateGet()) {
        case 1:
            CameraInterpolate(0, 1, sind(hookAnimationProgress[0] * 90.0f));
            if (hookAnimationProgress[0] < 1.0f) {
                hookAnimationProgress[0] += REFRESH_FREQ;
                if (hookAnimationProgress[0] > 1.0f) {
                    hookAnimationProgress[0] = 1.0f;
                }
            }
            break;

        case 6:
            CameraInterpolate(1, 1, 0);
            break;

        case 4:
        case 5:
        case 7:
            break;
    }
}

u32 controllerRumbleFlags[4] = { PAD_CHAN0_BIT, PAD_CHAN1_BIT, PAD_CHAN2_BIT, PAD_CHAN3_BIT };
s32 RumbleEnabledGet(void)
{
    s32 rumble = HuPadRumbleGet();
    s32 i;
    for (i = 0; i < 4; i++) {
        if (!GWPlayerCfg[i].iscom) {
            if ((rumble & controllerRumbleFlags[GWPlayerCfg[i].pad_idx]) == 0) {
                return 0;
            }
        }
    }
    return GWGameStat.rumble;
}

float hookParticleParameters[] = { 0, 355, 0, 0, 0, 275, 128, 950, 6, 0, 0, 0, 190, 0, 550 };

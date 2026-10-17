#include "REL/w06Dll.h"
#include "game/audio.h"
#include "game/data.h"
#include "game/flag.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/hsfex.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/process.h"
#include "game/window.h"
#include "game/wipe.h"
#include "game/board/audio.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/ui.h"
#ifndef __MWERKS__
#include "game/board/view.h"
#endif
#include "game/board/window.h"

#include "dolphin.h"
#include "ext_math.h"

typedef struct {
    struct {
        u8 isActive : 1;
    };
    char padding[3];
    s32 modelId;
    s32 motionId;
    s16 playerIndex;
} BowserEventWork;

static void W06BowserEventProcess(void);
static void W06BowserEventProcessCleanup(void);
static void W06BowserPlayerSizeOneSequence(s32 playerIndex);
static void W06BowserPlayerSizeZeroSequence(s32 playerIndex);
static void W06BowserPlayerSizeTwoSequence(s32 playerIndex);
static void W06BowserPlayerEncounterSequence(s32 playerIndex, s32 spaceIndex);
static void W06BowserSuitPlayerSequence(s32 playerIndex);
static void W06BowserModelRotateToAngle(float targetAngle, s16 rotationFrames);
static void W06BowserModelFollowUpdate(OMOBJ *modelObject);
static void W06BowserItemModelSet(s32 isVisible);
static void W06BowserModelFollowEnable(s32 isEnabled);
static void W06BowserModelFollowStop(void);
static void W06BowserPlayerFaceModel(s32 playerIndex);
static void W06BowserTurnEventProcess(void);
static s32 W06BowserPlayerResizeHook(void);
static s32 W06BowserStarSpaceCheck(s32 playerIndex, s32 spaceIndex);
static void W06BowserModelPositionSet(void);

static s8 bowserActivePlayer;
static s32 bowserEventResult;
static OMOBJ *bowserUpdateObject;
static HUPROCESS *bowserEventProcess;

static s16 bowserModel = -1;
static s16 bowserStageModel = -1;
static s16 bowserPlayerMotion = -1;
static s16 bowserHeldItemModel = -1;
static s16 bowserStarModel = -1;
static s16 bowserAuxiliaryModel = -1;

static s8 bowserStarAngles[] = {
    0, 0, 90,   0,
    0, 0,  0, 166
};

static s32 bowserCharacterSounds[8] = {
    0x00000128,
    0x00000168,
    0x000001A8,
    0x000001E8,
    0x00000228,
    0x00000268,
    0x000002A8,
    0x000002E8
};

static s32 bowserPlayerSounds[8] = {
    0x00000123,
    0x00000163,
    0x000001A3,
    0x000001E3,
    0x00000223,
    0x00000263,
    0x000002A3,
    0x000002E3
};

#if VERSION_JP
#define MDL_ID_SHIFT 0
#else
#define MDL_ID_SHIFT 1
#endif

static s32 bowserModelParts[] = {
    DATA_MAKE_NUM(DATADIR_BKOOPA, 8),
    DATA_MAKE_NUM(DATADIR_BKOOPA, 10),
    DATA_MAKE_NUM(DATADIR_BKOOPA, 11),
    DATA_MAKE_NUM(DATADIR_BKOOPA, 12),
    DATA_MAKE_NUM(DATADIR_W06, 24),
    DATA_MAKE_NUM(DATADIR_BKOOPA, 9),
    DATA_MAKE_NUM(DATADIR_W06, 25),
    DATA_MAKE_NUM(DATADIR_W06, 26),
    -1
};

static s32 bowserCharacterMotions[8] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 19),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 19),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 19),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 19),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 19),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 19),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 19),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 19)
};

void W06BowserEventInitialize(void) {
    OMOBJ *bowserObject;
    BowserEventWork *bowserWork;
    s32 starSpaceIndex;
    s32 i;

    bowserObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W06BowserModelFollowUpdate);
    bowserUpdateObject = bowserObject;
    bowserWork = omObjGetWork(bowserObject, BowserEventWork);
    bowserWork->isActive = 0;
    bowserWork->playerIndex = 0;
    bowserWork->motionId = 0;
    bowserWork->modelId = 1;
    bowserModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BKOOPA, 7), bowserModelParts, 0);
    BoardModelMotionStart(bowserModel, 1, 0x40000001);
    BoardModelScaleSet(bowserModel, 2.0f, 2.0f, 2.0f);
    bowserStageModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W06, 11), NULL, 0);
    BoardModelMotionStart(bowserStageModel, 0, 0x40000001);
    if (_CheckFlag(FLAG_ID_MAKE(1, 1))) {
        W06BowserModelPositionSet();
        if (w06BoardData->isStarSpaceOccupied == 0) {
            starSpaceIndex = BoardSpaceStarGet(w06BoardData->starSpaceIndex - 1);
            BoardSpaceTypeSet(0, starSpaceIndex, 10);
        }
    } else {
        BoardModelVisibilitySet(bowserStageModel, 0);
        BoardModelVisibilitySet(bowserModel, 0);
    }
    HuDataDirClose(DATADIR_BKOOPA);
    if (_CheckFlag(FLAG_ID_MAKE(1, 9))) {
        for (i = 0; i < 4; i++) {
            if (w06BoardData->bowserTargetPlayers & (1 << i)) {
                BoardPlayerPreTurnHookSet(i, W06BowserPlayerResizeHook);
            }
        }
    }
    if (w06BoardData->isBowserEventEnabled != 0) {
        BoardModelMotionStart(bowserModel, 2, 0);
        BoardModelMotionTimeSet(bowserModel, 90.0f);
        BoardModelMotionSpeedSet(bowserModel, 0.0f);
    }
}

void W06BowserEventCleanup(void) {
    W06BowserModelFollowStop();
    if (bowserModel != -1) {
        BoardModelKill(bowserModel);
        bowserModel = -1;
    }
    if (bowserStageModel != -1) {
        BoardModelKill(bowserStageModel);
        bowserStageModel = -1;
    }
    if (bowserHeldItemModel != -1) {
        BoardModelKill(bowserHeldItemModel);
        bowserHeldItemModel = -1;
    }
}

void W06BowserTurnStart(s32 playerIndex, s32 spaceIndex) {
    BoardRollCreate(playerIndex, 0);
    W06BowserEventStart(playerIndex, spaceIndex);
}

s32 W06BowserEventStart(s32 playerIndex, s32 spaceIndex) {
    bowserEventResult = 0;
    if (W06BowserStarSpaceCheck(playerIndex, spaceIndex) == 0) {
        return bowserEventResult;
    }
    bowserActivePlayer = playerIndex;
    BoardRollDispSet(0);
    bowserEventProcess = HuPrcChildCreate(W06BowserEventProcess, 0x2003, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(bowserEventProcess, W06BowserEventProcessCleanup);
    while (bowserEventProcess) {
        HuPrcVSleep();
    }
    BoardRollDispSet(1);
    return bowserEventResult;
}

static void W06BowserEventProcess(void) {
    s32 currentPlayerIndex;
    s32 currentSpaceIndex;

    currentPlayerIndex = GWSystem.player_curr;
    currentSpaceIndex = GWPlayer[currentPlayerIndex].space_curr;
    BoardAudSeqPause(0, 1, 0x1F4);
    W06BowserPlayerFaceModel(currentPlayerIndex);
    if (_CheckFlag(FLAG_ID_MAKE(1, 6))) {
        W06BowserPlayerEncounterSequence(currentPlayerIndex, currentSpaceIndex);
    } else if (GWPlayer[currentPlayerIndex].bowser_suit) {
        W06BowserSuitPlayerSequence(currentPlayerIndex);
    } else {
        switch (BoardPlayerSizeGet(currentPlayerIndex)) {
            case 1:
                W06BowserPlayerSizeOneSequence(currentPlayerIndex);
                break;
            case 0:
                W06BowserPlayerSizeZeroSequence(currentPlayerIndex);
                break;
            case 2:
                W06BowserPlayerSizeTwoSequence(currentPlayerIndex);
                break;
        }
    }
    HuPrcEnd();
}

static void W06BowserEventProcessCleanup(void) {
    if (bowserAuxiliaryModel != -1) {
        BoardModelKill(bowserAuxiliaryModel);
        bowserAuxiliaryModel = -1;
    }
    if (bowserStarModel != -1) {
        BoardModelKill(bowserStarModel);
        bowserStarModel = -1;
    }
    if (bowserPlayerMotion != -1) {
        BoardPlayerMotionKill(GWSystem.player_curr, bowserPlayerMotion);
        bowserPlayerMotion = -1;
    }
    bowserEventProcess = NULL;
}

static void W06BowserPlayerSizeOneSequence(s32 playerIndex) {
    Vec playerPosition;
    Vec bowserPosition;
    Vec directionToBowser;
    Vec cameraTargetOffset;
    Vec cameraRotation;
    float bounceCount;
    float jumpFrame;
    s32 destinationSpaceIndex;
    s32 bowserSoundHandle;

    bowserPlayerMotion = BoardPlayerMotionCreate(playerIndex, bowserCharacterMotions[GWPlayer[playerIndex].character]);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardModelPosGet(bowserStageModel, &bowserPosition);
    VECSubtract(&bowserPosition, &playerPosition, &directionToBowser);
    VECNormalize(&directionToBowser, &directionToBowser);
    BoardModelMotionShiftSet(bowserModel, 4, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(8);
    HuAudFXPlay(0x3C);
    BoardMusStart(1, 6, 0x7F, 0);
    cameraTargetOffset.x = 0.0f;
    cameraTargetOffset.y = 150.0f;
    cameraTargetOffset.z = 0.0f;
    cameraRotation.x = -14.0f;
    cameraRotation.y = atan2d(-directionToBowser.x, -directionToBowser.z);
    cameraRotation.z = 0.0f;
    BoardCameraMotionStartEx(bowserStageModel, &cameraRotation, &cameraTargetOffset, 1620.0f, -1.0f, 21);
    BoardCameraMotionWait();
    BoardCameraTargetModelSet(-1);
    omVibrate(playerIndex, 12, 4, 2);
    BoardPlayerMotionStart(playerIndex, 9, 0);
    HuAudFXPlay(bowserCharacterSounds[GWPlayer[playerIndex].character]);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    while (!BoardPlayerMotionEndCheck(playerIndex)) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(playerIndex);
    HuPrcSleep(30);
    BoardModelAttrSet(bowserModel, 0x40000004);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(bowserModel, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    BoardWinCreate(2, MAKE_MESSID(48, 2), 5);
    BoardWinWait();
    BoardWinKill();
    BoardCameraMotionStartEx(-1, NULL, NULL, 2000.0f, -1.0f, 21);
    BoardCameraMotionWait();
    BoardModelMotionShiftSet(bowserModel, 5, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(8);
    while (BoardModelMotionTimeGet(bowserModel) < 80.0f) {
        HuPrcVSleep();
    }
    bowserSoundHandle = HuAudFXPlay(0x34E);
    W06BowserItemModelSet(1);
    while (BoardModelMotionTimeGet(bowserModel) < 60.0f) {
        HuPrcVSleep();
    }
    HuAudFXPlay(bowserPlayerSounds[GWPlayer[playerIndex].character]);
    omVibrate(playerIndex, 12, 4, 2);
    BoardPlayerMotionStart(playerIndex, bowserPlayerMotion, 0x40000001);
    destinationSpaceIndex = BoardSpaceFlagPosGet(0, 0x8000, &bowserPosition);
    VECSubtract(&bowserPosition, &playerPosition, &directionToBowser);
    VECNormalize(&directionToBowser, &directionToBowser);
    directionToBowser.x = 500.0f * directionToBowser.x / 90.0f;
    directionToBowser.z = 500.0f * directionToBowser.z / 90.0f;
    playerPosition.y += 150.0f;
    for (jumpFrame = 0.0f; jumpFrame < 90.0f; jumpFrame += 1.0f) {
        playerPosition.x += directionToBowser.x;
        playerPosition.z += directionToBowser.z;
        if (playerPosition.y < 0.0f) {
            playerPosition.y = 0.0f;
            break;
        }
        playerPosition.y += 30.0f + -0.08166667f * jumpFrame * jumpFrame * 0.125f;
        BoardPlayerPosSetV(playerIndex, &playerPosition);
        HuPrcVSleep();
    }
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_CROSS, 1);
    HuPrcSleep(2);
    BoardCameraMoveSet(0);
    BoardModelVisibilitySet(BoardPlayerModelGet(playerIndex), 0);
    BoardPlayerRotYSet(playerIndex, 0.0f);
    BoardCameraViewSet(2);
    BoardCameraMotionWait();
    BoardCameraTargetSpaceSet(destinationSpaceIndex);
    HuAudFXFadeOut(bowserSoundHandle, 0xC8);
    W06BowserItemModelSet(0);
    BoardModelMotionStart(bowserModel, 1, 0x40000001);
    BoardModelRotYSet(bowserModel, 0.0f);
    playerPosition.x = bowserPosition.x;
    playerPosition.y = bowserPosition.y + 1000.0f;
    playerPosition.z = bowserPosition.z;
    BoardPlayerPosSetV(playerIndex, &playerPosition);
    BoardModelVisibilitySet(BoardPlayerModelGet(playerIndex), 1);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_CROSS, 30);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    jumpFrame = 0.0f;
    bounceCount = jumpFrame;
    while (TRUE) {
        if (playerPosition.y < bowserPosition.y) {
            if (bounceCount >= 3.0f) {
                break;
            }
            HuAudFXPlay(0x4A7);
            omVibrate(playerIndex, 12, 6, 6);
            playerPosition.y = bowserPosition.y + 30.0f * (3.0f - bounceCount);
            jumpFrame = 0.0f;
            bounceCount += 1.0f;
        }
        playerPosition.y += 5.0f + -0.08166667f * jumpFrame * jumpFrame;
        BoardPlayerPosSetV(playerIndex, &playerPosition);
        HuPrcVSleep();
        jumpFrame += 1.0f;
    }
    BoardPlayerPosSetV(playerIndex, &bowserPosition);
    BoardPlayerMotionStart(playerIndex, 6, 0x40000001);
    HuPrcSleep(60);
    GWPlayer[playerIndex].space_curr = destinationSpaceIndex;
    BoardAudSeqFadeOut(1, 1000);
    HuPrcSleep(24);
    W06BowserModelRotateToAngle(0.0f, 20);
    BoardCameraMoveSet(1);
    BoardCameraViewSet(1);
    BoardCameraMotionWait();
    BoardAudSeqPause(0, 0, 1000);
}

static void W06BowserPlayerSizeZeroSequence(s32 playerIndex) {
    Vec playerPosition;
    Vec bowserPosition;
    Vec directionToBowser;
    Vec cameraTargetOffset;
    Vec cameraRotation;
    u16 coinChangeAmount;
    s32 playerMotionId;
    s32 messageId;
    s16 i;
    s16 coinDelayFrames;
    s16 coinDelta;

    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardModelPosGet(bowserStageModel, &bowserPosition);
    VECSubtract(&bowserPosition, &playerPosition, &directionToBowser);
    VECNormalize(&directionToBowser, &directionToBowser);
    BoardModelMotionShiftSet(bowserModel, 4, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(8);
    HuAudFXPlay(0x3C);
    BoardMusStart(1, 6, 0x7F, 0);
    cameraTargetOffset.x = 0.0f;
    cameraTargetOffset.y = 150.0f;
    cameraTargetOffset.z = 0.0f;
    cameraRotation.x = -14.0f;
    cameraRotation.y = atan2d(-directionToBowser.x, -directionToBowser.z);
    cameraRotation.z = 0.0f;
    BoardCameraMotionStartEx(bowserStageModel, &cameraRotation, &cameraTargetOffset, 1620.0f, -1.0f, 21);
    BoardCameraMotionWait();
    BoardCameraTargetModelSet(-1);
    omVibrate(playerIndex, 12, 4, 2);
    BoardPlayerMotionStart(playerIndex, 9, 0);
    HuAudFXPlay(bowserCharacterSounds[GWPlayer[playerIndex].character]);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    while (!BoardPlayerMotionEndCheck(playerIndex)) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(playerIndex);
    HuPrcSleep(30);
    BoardModelAttrSet(bowserModel, 0x40000004);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(bowserModel, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(9);
    while (BoardModelMotionTimeGet(bowserModel) < 33.0f) {
        HuPrcVSleep();
    }
    HuAudFXPlay(0x51);
    coinChangeAmount = BoardPlayerCoinsGet(playerIndex);
    if (coinChangeAmount != 0) {
        messageId = MAKE_MESSID(48, 3);
        coinDelta = -1;
        coinChangeAmount = BoardPlayerCoinsGet(playerIndex) - (coinChangeAmount >> 1);
        playerMotionId = 8;
    } else {
        messageId = MAKE_MESSID(48, 5);
        coinDelta = 1;
        coinChangeAmount = 50;
        playerMotionId = 7;
    }
    BoardWinCreate(2, messageId, 5);
    BoardWinWait();
    if (coinChangeAmount >= 50) {
        coinDelayFrames = 1;
    } else if (coinChangeAmount >= 20) {
        coinDelayFrames = 3;
    } else {
        coinDelayFrames = 6;
    }
    for (i = 0; i < coinChangeAmount; i++) {
        BoardPlayerCoinsAdd(playerIndex, coinDelta);
        if (coinDelta == -1) {
            HuAudFXPlay(0xE);
        } else {
            HuAudFXPlay(7);
        }
        HuPrcSleep(coinDelayFrames);
    }
    HuAudFXPlay(0xF);
    BoardPlayerMotBlendSet(playerIndex, BoardPlayerRotYGet(playerIndex) + 180.0f, 0x14);
    while (!BoardPlayerMotBlendCheck(playerIndex)) {
        HuPrcVSleep();
    }
    BoardModelAttrSet(bowserModel, 0x40000004);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(bowserModel, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    BoardPlayerMotionShiftSet(playerIndex, playerMotionId, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(10);
    while (!BoardPlayerMotionEndCheck(playerIndex)) {
        HuPrcVSleep();
    }
    if (coinDelta == -1) {
        messageId = MAKE_MESSID(48, 4);
    } else {
        messageId = MAKE_MESSID(48, 6);
    }
    BoardModelMotionShiftSet(bowserModel, 3, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuAudFXPlay(0x3A);
    BoardWinCreate(2, messageId, 5);
    BoardWinWait();
    BoardWinKill();
    BoardAudSeqFadeOut(1, 1000);
    HuPrcSleep(24);
    BoardPlayerIdleSet(playerIndex);
    BoardCameraMoveSet(1);
    BoardCameraViewSet(1);
    BoardCameraMotionWait();
    BoardModelMotionShiftSet(bowserModel, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    W06BowserModelRotateToAngle(0.0f, 20);
    BoardAudSeqPause(0, 0, 1000);
}

static void W06BowserPlayerSizeTwoSequence(s32 playerIndex) {
    Vec playerPosition;
    Vec bowserPosition;
    Vec directionToBowser;
    Vec cameraTargetOffset;
    Vec cameraRotation;
    float jumpFrame;
    OMOVL bowserOverlayId;
    s32 starSpaceIndex;
    s32 teamId;
    s32 i;

    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardModelPosGet(bowserStageModel, &bowserPosition);
    VECSubtract(&bowserPosition, &playerPosition, &directionToBowser);
    VECNormalize(&directionToBowser, &directionToBowser);
    BoardModelMotionShiftSet(bowserModel, 4, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(8);
    HuAudFXPlay(0x3C);
    BoardMusStart(1, 6, 0x7F, 0);
    cameraTargetOffset.x = 0.0f;
    cameraTargetOffset.y = 250.0f;
    cameraTargetOffset.z = 0.0f;
    cameraRotation.x = -8.0f;
    cameraRotation.y = atan2d(-directionToBowser.x, -directionToBowser.z) - 45.0;
    cameraRotation.z = 0.0f;
    BoardCameraMotionStartEx(bowserStageModel, &cameraRotation, &cameraTargetOffset, 1700.0f, -1.0f, 63);
    BoardCameraMotionWait();
    BoardCameraTargetModelSet(-1);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    BoardModelAttrSet(bowserModel, 0x40000004);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(bowserModel, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    BoardWinCreate(2, MAKE_MESSID(48, 7), 5);
    BoardWinWait();
    BoardWinKill();
    W06BowserModelFollowEnable(0);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardAudSeqFadeOut(1, 1000);
    BoardModelMotionShiftSet(bowserModel, 2, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(55);
    BoardPlayerMotionShiftSet(playerIndex, 4, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(5);
    soundHandle = HuAudFXPlay(0x3D);
    BoardPlayerPosGet(playerIndex, &directionToBowser);
    jumpFrame = 0.0f;
    while (TRUE) {
        if (BoardModelMotionTimeGet(bowserModel) >= 90.0f) {
            BoardModelAttrSet(bowserModel, 0x40000002);
        }
        playerPosition.y += 50.0f;
        if (playerPosition.y > directionToBowser.y + 700.0f) {
            break;
        }
        BoardPlayerPosSetV(playerIndex, &playerPosition);
        HuPrcVSleep();
        jumpFrame += 1.0f;
    }
    BoardModelVisibilitySet(bowserModel, 0);
    _SetFlag(FLAG_ID_MAKE(1, 28));
    _SetFlag(FLAG_ID_MAKE(1, 14));
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    if (soundHandle >= 0) {
        while (HuAudFXStatusGet(soundHandle) != 0) {
            HuPrcVSleep();
        }
    }
    HuAudFXAllStop();
    BoardAudSeqFadeOutAll();
    w06BoardData->previousItemIndex = BoardItemPrevGet() + 1;
    if (BoardRandMod(100) < 50) {
        bowserOverlayId = DLL_m457dll;
        GWMGAvailSet(0x1C9);
        GWSystem.mg_next = 0x38;
    } else {
        bowserOverlayId = DLL_m458dll;
        GWMGAvailSet(0x1CA);
        GWSystem.mg_next = 0x39;
    }
    _SetFlag(FLAG_ID_MAKE(1, 6));
    BoardRollUpdateSet(playerIndex);
    if (!_CheckFlag(FLAG_ID_MAKE(1, 12))) {
        GWPlayer->coin_win = 0;
    }
    if (!_CheckFlag(FLAG_ID_MAKE(1, 12))) {
        GWPlayer[1].coin_win = 0;
    }
    if (!_CheckFlag(FLAG_ID_MAKE(1, 12))) {
        GWPlayer[2].coin_win = 0;
    }
    if (!_CheckFlag(FLAG_ID_MAKE(1, 12))) {
        GWPlayer[3].coin_win = 0;
    }
    _ClearFlag(FLAG_ID_MAKE(1, 7));
    for (i = 0; i < 4; i++) {
        if (GWPlayerTeamGet(i)) {
            GWPlayer[i].team_backup = 1;
        } else {
            GWPlayer[i].team_backup = 0;
        }
        if (i == playerIndex) {
            GWPlayerCfg[i].group = 0;
        } else {
            GWPlayerCfg[i].group = 1;
        }
    }
    if (GWPlayer[playerIndex].com && !GWMGShowComGet()) {
        if (BoardRandMod(100) > 60) {
            _SetFlag(FLAG_ID_MAKE(1, 7));
        }
        BoardAudSeqFadeOutFast(0);
        BoardAudSeqFadeOutFast(1);
        w06BoardData->isBowserEventEnabled = 0;
        W06BowserModelFollowEnable(1);
        HuPrcSleep(60);
        for (i = 0; i < 4; i++) {
            teamId = GWPlayer[i].team_backup;
            GWPlayer[i].team = teamId;
            GWPlayerCfg[i].group = teamId;
        }
        W06BowserPlayerEncounterSequence(playerIndex, GWPlayer[playerIndex].space_curr);
        BoardMusStartBoard();
        _ClearFlag(FLAG_ID_MAKE(1, 6));
        BoardCameraViewSet(1);
        BoardCameraMotionWait();
        BoardRollCreate(playerIndex, 0);
    } else {
        BoardNextOvlSet(bowserOverlayId);
    }
}

static void W06BowserPlayerEncounterSequence(s32 playerIndex, s32 spaceIndex) {
    Vec playerPosition;
    Vec bowserPosition;
    Vec directionToBowser;
    Vec cameraTargetOffset;
    Vec cameraRotation;
    s16 i;
    s16 coinDelayFrames;
    s16 coinChangeAmount;
    s16 starSpaceIndex;
    s32 messageId;
    s32 playerMotionId;
    s32 playerCoinCount;
    s32 isBadLuck;

    isBadLuck = 0;
    bowserEventResult = 1;
    if (GWPlayer[playerIndex].com && !GWMGShowComGet()) {
        BoardModelVisibilitySet(bowserModel, 1);
        BoardModelVisibilitySet(bowserStageModel, 1);
        BoardModelMotionStart(bowserModel, 1, 0x40000001);
        BoardPlayerMotionStart(playerIndex, 1, 0x40000001);
    }
    BoardStatusItemSet(1);
    BoardMusStart(1, 6, 0x7F, 0);
    BoardItemPrevSet(w06BoardData->previousItemIndex - 1);
    w06BoardData->previousItemIndex = 0;
    BoardSpacePosGet(0, spaceIndex, &playerPosition);
    BoardPlayerPosSetV(playerIndex, &playerPosition);
    BoardModelPosGet(bowserStageModel, &bowserPosition);
    VECSubtract(&bowserPosition, &playerPosition, &directionToBowser);
    VECNormalize(&directionToBowser, &directionToBowser);
    BoardPlayerRotYSet(playerIndex, atan2d(directionToBowser.x, directionToBowser.z));
    BoardPlayerSizeSet(playerIndex, 2);
    cameraTargetOffset.x = 0.0f;
    cameraTargetOffset.y = 150.0f;
    cameraTargetOffset.z = 0.0f;
    cameraRotation.x = -25.0f;
    cameraRotation.y = atan2d(-directionToBowser.x, -directionToBowser.z);
    cameraRotation.z = 0.0f;
    BoardCameraMotionStartEx(bowserStageModel, &cameraRotation, &cameraTargetOffset, 2000.0f, -1.0f, 2);
    BoardCameraMotionWait();
    BoardCameraTargetModelSet(-1);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 21);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    _ClearFlag(FLAG_ID_MAKE(1, 28));
    _ClearFlag(FLAG_ID_MAKE(1, 14));
    if (_CheckFlag(FLAG_ID_MAKE(1, 7))) {
        isBadLuck = 0;
    } else {
        isBadLuck = 1;
    }
    if (isBadLuck != 0) {
        messageId = MAKE_MESSID(48, 10);
        playerMotionId = 8;
        HuAudFXPlay(0x3A);
        BoardModelMotionShiftSet(bowserModel, 3, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    } else {
        messageId = MAKE_MESSID(48, 8);
        playerMotionId = 7;
    }
    HuAudFXPlay(0x3C);
    BoardWinCreate(2, messageId, 5);
    BoardWinWait();
    BoardWinKill();
    BoardPlayerMotBlendSet(playerIndex, BoardPlayerRotYGet(playerIndex) + 180.0f, 0x14);
    while (!BoardPlayerMotBlendCheck(playerIndex)) {
        HuPrcVSleep();
    }
    if (isBadLuck != 0) {
        BoardModelMotionShiftSet(bowserModel, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
        playerCoinCount = BoardPlayerCoinsGet(playerIndex);
        coinChangeAmount = playerCoinCount - (BoardPlayerCoinsGet(playerIndex) >> 1);
        if (coinChangeAmount < 0) {
            coinChangeAmount = 0;
        }
        if (coinChangeAmount >= 50) {
            coinDelayFrames = 1;
        } else if (coinChangeAmount >= 20) {
            coinDelayFrames = 3;
        } else {
            coinDelayFrames = 6;
        }
        for (i = 0; i < coinChangeAmount; i++) {
            BoardPlayerCoinsAdd(playerIndex, -1);
            HuAudFXPlay(0xE);
            HuPrcSleep(coinDelayFrames);
        }
        HuAudFXPlay(0xF);
    } else {
        BoardModelMotionShiftSet(bowserModel, 8, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
        BoardWinCreate(2, MAKE_MESSID(48, 9), 5);
        BoardWinWait();
        BoardWinKill();
        BoardAudSeqFadeOut(1, 1000);
        bowserAuxiliaryModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W06, 16), NULL, 0);
        HuAudFXPlay(0x4A8);
        BoardModelPosGet(bowserStageModel, &bowserPosition);
        BoardModelPosSetV(bowserAuxiliaryModel, &bowserPosition);
        BoardModelMotionStart(bowserAuxiliaryModel, 0, 0);
        BoardModelScaleSet(bowserAuxiliaryModel, 2.0f, 2.0f, 2.0f);
        while (BoardModelMotionTimeGet(bowserAuxiliaryModel) < 110.0f) {
            HuPrcVSleep();
        }
        BoardModelVisibilitySet(bowserModel, 0);
        BoardModelVisibilitySet(bowserStageModel, 0);
        while (!BoardModelMotionEndCheck(bowserAuxiliaryModel)) {
            HuPrcVSleep();
        }
        BoardModelVisibilitySet(bowserAuxiliaryModel, 0);
        w06BoardData->isStarSpaceOccupied = 1;
        starSpaceIndex = BoardSpaceStarGet(w06BoardData->starSpaceIndex - 1);
        BoardSpaceTypeSet(0, starSpaceIndex, 1);
        BoardMusStartBoard();
    }
    BoardPlayerMotionShiftSet(playerIndex, playerMotionId, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(8);
    BoardModelAttrSet(bowserModel, 0x40000004);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    while (!BoardPlayerMotionEndCheck(playerIndex)) {
        HuPrcVSleep();
    }
    if (isBadLuck != 0) {
        BoardAudSeqFadeOut(1, 1000);
    }
    HuPrcSleep(48);
    BoardModelMotionShiftSet(bowserModel, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    BoardPlayerIdleSet(playerIndex);
    W06BowserModelRotateToAngle(0.0f, 20);
}

static void W06BowserSuitPlayerSequence(s32 playerIndex) {
    Vec playerPosition;
    Vec bowserPosition;
    Vec directionToBowser;
    Vec cameraTargetOffset;
    Vec cameraRotation;
    s16 i;
    s32 messageId;
    s16 coinDelayFrames;
    u16 coinReward;

    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardModelPosGet(bowserStageModel, &bowserPosition);
    VECSubtract(&bowserPosition, &playerPosition, &directionToBowser);
    VECNormalize(&directionToBowser, &directionToBowser);
    BoardModelMotionShiftSet(bowserModel, 4, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(8);
    HuAudFXPlay(0x3C);
    cameraTargetOffset.x = 0.0f;
    cameraTargetOffset.y = 150.0f;
    cameraTargetOffset.z = 0.0f;
    cameraRotation.x = -14.0f;
    cameraRotation.y = atan2d(-directionToBowser.x, -directionToBowser.z);
    cameraRotation.z = 0.0f;
    BoardCameraMotionStartEx(bowserStageModel, &cameraRotation, &cameraTargetOffset, 1620.0f, -1.0f, 21);
    BoardCameraMotionWait();
    BoardCameraTargetModelSet(-1);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(playerIndex);
    HuPrcSleep(30);
    BoardModelAttrSet(bowserModel, 0x40000004);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(bowserModel, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    BoardMusStart(1, 6, 0x7F, 0);
    BoardModelMotionShiftSet(bowserModel, 7, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(8);
    messageId = MAKE_MESSID(48, 38);
    coinReward = 50;
    BoardWinCreate(2, messageId, 5);
    BoardWinWait();
    coinDelayFrames = 1;
    for (i = 0; i < coinReward; i++) {
        BoardPlayerCoinsAdd(playerIndex, 1);
        HuAudFXPlay(7);
        HuPrcSleep(coinDelayFrames);
    }
    HuAudFXPlay(0xF);
    BoardPlayerMotBlendSet(playerIndex, BoardPlayerRotYGet(playerIndex) + 180.0f, 0x14);
    while (!BoardPlayerMotBlendCheck(playerIndex)) {
        HuPrcVSleep();
    }
    BoardModelAttrSet(bowserModel, 0x40000004);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    messageId = MAKE_MESSID(48, 39);
    BoardModelMotionShiftSet(bowserModel, 3, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuAudFXPlay(0x3A);
    BoardWinCreate(2, messageId, 5);
    BoardWinWait();
    BoardWinKill();
    BoardModelMotionShiftSet(bowserModel, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    BoardAudSeqFadeOut(1, 1000);
    HuPrcSleep(24);
    BoardPlayerIdleSet(playerIndex);
    BoardCameraMoveSet(1);
    BoardCameraViewSet(1);
    BoardCameraMotionWait();
    W06BowserModelRotateToAngle(0.0f, 20);
    BoardAudSeqPause(0, 0, 1000);
}

static void W06BowserModelRotateToAngle(float targetAngle, s16 rotationFrames) {
    float rotationDelta;
    float absoluteRotationDelta;
    float rotationStep;
    float currentRotation;
    float frameCount;
    float rotationSpeed;

    if (rotationFrames <= 0) {
        rotationFrames = 1;
    }
    OSs16tof32(&rotationFrames, &frameCount);
    targetAngle = BoardDAngleCalc(targetAngle);
    currentRotation = BoardDAngleCalc(BoardModelRotYGet(bowserModel));
    rotationDelta = BoardDAngleCalc(targetAngle - currentRotation);
    rotationStep = rotationDelta / frameCount;
    if (rotationDelta < 0.0f) {
        absoluteRotationDelta = -rotationDelta;
    } else {
        absoluteRotationDelta = rotationDelta;
    }
    if (absoluteRotationDelta < 5.0f) {
        BoardModelRotYSet(bowserModel, targetAngle);
        return;
    }
    BoardModelMotionShiftSet(bowserModel, 6, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    BoardModelMotionSpeedSet(bowserModel, 2.0f);
    while (rotationFrames > 0) {
        currentRotation = rotationStep + BoardModelRotYGet(bowserModel);
        BoardModelRotYSet(bowserModel, currentRotation);
        HuPrcVSleep();
        rotationFrames--;
    }
    BoardModelRotYSet(bowserModel, targetAngle);
    BoardModelMotionShiftSet(bowserModel, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
}

static void W06BowserModelFollowUpdate(OMOBJ *modelObject) {
    Mtx bowserJointMatrix;
    Vec jointWorldPosition;
    float verticalBobOffset;
    BowserEventWork *bowserWork;

    bowserWork = omObjGetWork(modelObject, BowserEventWork);
    if (bowserWork->isActive != 0 || bowserStageModel == -1 || BoardIsKill()) {
        W06BowserItemModelSet(0);
        bowserUpdateObject = NULL;
        omDelObjEx(HuPrcCurrentGet(), modelObject);
        return;
    }
    // Bug? Likely to be "%=" instead.
    bowserWork->phaseAngle % 360;
    OSs16tof32(&bowserWork->phaseAngle, &verticalBobOffset);
    verticalBobOffset = 0.5 * sind(verticalBobOffset);
    BoardModelPosGet(bowserStageModel, &jointWorldPosition);
    jointWorldPosition.y += verticalBobOffset;
    BoardModelPosSetV(bowserStageModel, &jointWorldPosition);
    if (bowserWork->isStageModelFollowing != 0 && w06BoardData->isBowserEventEnabled == 0) {
        Hu3DModelObjMtxGet(BoardModelIDGet(bowserStageModel), "kuppa", bowserJointMatrix);
        Hu3DMtxTransGet(bowserJointMatrix, &jointWorldPosition);
        BoardModelPosSetV(bowserModel, &jointWorldPosition);
    }
    if (bowserWork->isItemModelFollowing != 0) {
        Hu3DModelObjMtxGet(BoardModelIDGet(bowserModel), "itemhook_M", bowserJointMatrix);
        Hu3DMtxTransGet(bowserJointMatrix, &jointWorldPosition);
        BoardModelPosSetV(bowserHeldItemModel, &jointWorldPosition);
    }
    bowserWork->phaseAngle += 4;
}

static void W06BowserItemModelSet(s32 isVisible) {
    float starAngle;
    BowserEventWork *bowserWork;

    if (bowserUpdateObject) {
        bowserWork = omObjGetWork(bowserUpdateObject, BowserEventWork);
        bowserWork->isItemModelFollowing = isVisible;
    }
    if (isVisible != 0) {
        if (bowserHeldItemModel == -1) {
            OSs8tof32(&bowserStarAngles[w06BoardData->starSpaceIndex - 1], &starAngle);
            bowserHeldItemModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W06, 14), NULL, 0);
            BoardModelRotSet(bowserHeldItemModel, 45.0f, starAngle, 0.0f);
            BoardModelScaleSet(bowserHeldItemModel, 1.8f, 1.8f, 1.8f);
            BoardModelLayerSet(bowserHeldItemModel, 2);
            BoardModelPassSet(bowserHeldItemModel, 1);
            BoardModelMotionStart(bowserHeldItemModel, 0, 0x40000001);
            BoardModelVisibilitySet(bowserHeldItemModel, 1);
        }
    } else if (bowserHeldItemModel != -1) {
        BoardModelKill(bowserHeldItemModel);
        bowserHeldItemModel = -1;
    }
}

static void W06BowserModelFollowEnable(s32 isEnabled) {
    BowserEventWork *bowserWork;

    if (bowserUpdateObject) {
        bowserWork = omObjGetWork(bowserUpdateObject, BowserEventWork);
        bowserWork->isStageModelFollowing = isEnabled;
    }
}

static void W06BowserModelFollowStop(void) {
    BowserEventWork *bowserWork;

    if (bowserUpdateObject) {
        bowserWork = omObjGetWork(bowserUpdateObject, BowserEventWork);
        bowserWork->isActive = 1;
    }
}

static void W06BowserPlayerFaceModel(s32 playerIndex) {
    Vec bowserPosition;
    Vec playerPosition;
    Vec directionToPlayer;
    float playerFacingAngle;
    float starAngle;

    OSs8tof32(&bowserStarAngles[w06BoardData->starSpaceIndex - 1], &starAngle);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardModelPosGet(bowserStageModel, &bowserPosition);
    VECSubtract(&playerPosition, &bowserPosition, &directionToPlayer);
    playerFacingAngle = atan2d(directionToPlayer.x, directionToPlayer.z);
    BoardPlayerMotBlendSet(playerIndex, playerFacingAngle + 180.0f, 0x14);
    W06BowserModelRotateToAngle(starAngle, 20);
    while (!BoardPlayerMotBlendCheck(playerIndex)) {
        HuPrcVSleep();
    }
    BoardPlayerIdleSet(playerIndex);
}

void W06BowserModelAnimationSet(s32 showBowser) {
    if (showBowser != 0) {
        BoardModelMotionShiftSet(bowserModel, 2, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
        BoardModelMotionSpeedSet(bowserModel, 2.0f);
        HuPrcSleep(30);
        w06BoardData->isBowserEventEnabled = 1;
        while (BoardModelMotionTimeGet(bowserModel) < 90.0f) {
            HuPrcVSleep();
        }
        BoardModelAttrSet(bowserModel, 0x40000002);
    } else {
        w06BoardData->isBowserEventEnabled = 0;
        BoardModelMotionStart(bowserModel, 2, 0);
        BoardModelMotionTimeSet(bowserModel, 90.0f);
        BoardModelMotionSpeedSet(bowserModel, 2.0f);
        HuPrcSleep(2);
        while (!BoardModelMotionEndCheck(bowserModel)) {
            HuPrcVSleep();
        }
        BoardModelMotionShiftSet(bowserModel, 1, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    }
}

void W06BowserTurnEventCheck(void) {
    s32 i;

    if (w06BoardData->isStarSpaceOccupied != 0) {
        return;
    }
    if (w06BoardData->boardEventCounter != 0) {
        w06BoardData->boardEventCounter--;
        return;
    }
    for (i = 0; i < 4; i++) {
        if (BoardPlayerAutoSizeGet(i) != 0) {
            return;
        }
    }
    if ((BoardPlayerCoinsGet(0) > 5 || BoardPlayerCoinsGet(1) > 5 || BoardPlayerCoinsGet(2) > 5 || BoardPlayerCoinsGet(3) > 5) && GWSystem.turn > 5) {
        bowserEventProcess = HuPrcChildCreate(W06BowserTurnEventProcess, 0x2003, 0x3800, 0, boardMainProc);
        HuPrcDestructorSet2(bowserEventProcess, W06BowserEventProcessCleanup);
        while (bowserEventProcess) {
            HuPrcVSleep();
        }
    }
}

static void W06BowserTurnEventProcess(void) {
    Vec bowserPosition;
    Vec cameraTargetOffset;
    Vec cameraRotation;
    float starAngle;
    s16 affectedStarSpace;
    s16 i;

    OSs8tof32(&bowserStarAngles[w06BoardData->starSpaceIndex - 1], &starAngle);
    cameraTargetOffset.x = 0.0f;
    cameraTargetOffset.y = 150.0f;
    cameraTargetOffset.z = 0.0f;
    cameraRotation.x = -14.0f;
    cameraRotation.y = starAngle;
    cameraRotation.z = 0.0f;
    BoardCameraMotionStartEx(bowserStageModel, &cameraRotation, &cameraTargetOffset, 1620.0f, -1.0f, 1);
    BoardCameraMotionWait();
    BoardCameraTargetModelSet(-1);
    BoardModelRotYSet(bowserModel, starAngle);
    bowserStarModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 98 + MDL_ID_SHIFT), NULL, 0);
    BoardModelVisibilitySet(bowserStarModel, 0);
    BoardModelLayerSet(bowserStarModel, 2);
    affectedStarSpace = BoardSpaceStarGet(w06BoardData->starSpaceIndex - 1);
    affectedStarSpace = BoardSpaceLinkFlagSearch(0, affectedStarSpace, 0x04000000);
    BoardSpacePosGet(0, affectedStarSpace, &cameraTargetOffset);
    BoardModelPosGet(bowserModel, &bowserPosition);
    bowserPosition.x += -100.0 * sind(BoardModelRotYGet(bowserModel));
    bowserPosition.z += -100.0 * cosd(BoardModelRotYGet(bowserModel));
    bowserPosition.y += 250.0f;
    BoardModelPosSetV(bowserStarModel, &bowserPosition);
    BoardModelScaleSet(bowserStarModel, 10.0f, 10.0f, 10.0f);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 21);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    BoardMusStart(1, 6, 0x7F, 0);
    HuAudFXPlay(0x3B);
    BoardWinCreate(2, MAKE_MESSID(48, 11), 5);
    BoardWinWait();
    BoardWinKill();
    HuAudFXPlay(0x359);
    BoardModelMotionShiftSet(bowserModel, 4, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(11);
    HuAudFXPlay(0x33F);
    HuAudFXPlay(0x3C);
    for (i = 0; i < 4; i++) {
        omVibrate(i, 12, 0xC, 0);
    }
    HuAudFXPlay(0x352);
    BoardModelVisibilitySet(bowserStarModel, 1);
    BoardModelMotionStart(bowserStarModel, 0, 0);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    HuPrcSleep(90);
    BoardModelAttrSet(bowserModel, 0x40000004);
    BoardModelVisibilitySet(bowserStarModel, 0);
    while (!BoardModelMotionEndCheck(bowserModel)) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(bowserModel, 1, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    BoardWinCreate(0, MAKE_MESSID(48, 12), -1);
    BoardWinWait();
    BoardWinKill();
    BoardAudSeqFadeOut(1, 1000);
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 21);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    BoardModelRotYSet(bowserModel, 0.0f);
    BoardCameraMoveSet(0);
    BoardCameraViewSet(2);
    BoardCameraMotionWait();
    BoardCameraMoveSet(1);
    w06BoardData->boardEventCounter = 3;
    _SetFlag(FLAG_ID_MAKE(1, 9));
    for (i = 0; i < 4; i++) {
        BoardPlayerPreTurnHookSet(i, W06BowserPlayerResizeHook);
        w06BoardData->bowserTargetPlayers |= (1 << i);
    }
    HuPrcEnd();
}

static s32 W06BowserPlayerResizeHook(void) {
    omVibrate(GWSystem.player_curr, 90, 6, 6);
    HuAudFXPlay(0x314);
    HuAudFXPlay(0x313);
    BoardPlayerResizeAnimExec(GWSystem.player_curr, 1);
    BoardPlayerSizeSet(GWSystem.player_curr, 1);
    BoardWinCreate(0, MAKE_MESSID(48, 13), -1);
    BoardWinWait();
    BoardWinKill();
    w06BoardData->bowserTargetPlayers &= ~(1 << GWSystem.player_curr);
    return 1;
}

static s32 W06BowserStarSpaceCheck(s32 playerIndex, s32 spaceIndex) {
    if (BoardSpaceTypeGet(0, spaceIndex) == 10) {
        return 1;
    } else {
        return 0;
    }
}

static void W06BowserModelPositionSet(void) {
    Vec starPosition;
    float starAngle;
    s16 starIndex;
    s16 linkedSpaceIndex;
    s16 starSpaceIndex;

    if (w06BoardData->isStarSpaceOccupied != 0) {
        BoardModelVisibilitySet(bowserModel, 0);
        BoardModelVisibilitySet(bowserStageModel, 0);
        return;
    }
    BoardModelVisibilitySet(bowserStageModel, 1);
    BoardModelVisibilitySet(bowserModel, 1);
    starIndex = w06BoardData->starSpaceIndex - 1;
    OSs8tof32(&bowserStarAngles[starIndex], &starAngle);
    starSpaceIndex = BoardSpaceStarGet(starIndex);
    linkedSpaceIndex = BoardSpaceLinkFlagSearch(0, starSpaceIndex, 0x04000000);
    BoardSpacePosGet(0, linkedSpaceIndex, &starPosition);
    BoardModelPosSetV(bowserStageModel, &starPosition);
    BoardModelPosSetV(bowserModel, &starPosition);
    BoardModelRotYSet(bowserStageModel, starAngle);
    BoardModelVisibilitySet(bowserModel, 1);
    BoardModelVisibilitySet(bowserStageModel, 1);
    BoardModelRotYSet(bowserModel, 0.0f);
}

void W06BowserStarGiveHook(void) {
    s32 starSpaceIndex;

    starSpaceIndex = BoardSpaceStarGet(w06BoardData->starSpaceIndex - 1);
    BoardSpaceTypeSet(0, starSpaceIndex, 1);
    BoardModelVisibilitySet(bowserModel, 0);
    BoardModelVisibilitySet(bowserStageModel, 0);
}

void W06BowserNextStarShowHook(void) {
    Vec boardViewPosition;
    Vec bowserPosition;
    Vec viewDirection;
    Vec starPosition;
    float viewDistance;
    float starAngle;
    s32 starIndex;
    s16 selectedStarSpace;
    s16 linkedSpaceIndex;
    s16 starSpaceIndex;
    s16 viewTransitionFrames;
    s8 *starAngleEntry;

    if (w06BoardData->isStarSpaceOccupied != 0) {
        w06BoardData->isStarSpaceOccupied = 0;
    }
    if (w06BoardData->isBowserEventEnabled != 0) {
        w06BoardData->isBowserEventEnabled = 0;
    }
    starIndex = BoardSpaceStarGetRandom(w06BoardData->starSpaceIndex - 1);
    w06BoardData->starSpaceIndex = starIndex + 1;
    W06BridgeStateUpdate();
    if (w06BoardData->isStarSpaceOccupied != 0) {
        BoardModelVisibilitySet(bowserModel, 0);
        BoardModelVisibilitySet(bowserStageModel, 0);
    } else {
        BoardModelVisibilitySet(bowserStageModel, 1);
        BoardModelVisibilitySet(bowserModel, 1);
        starIndex = w06BoardData->starSpaceIndex - 1;
        starAngleEntry = &bowserStarAngles[starIndex];
        OSs8tof32(starAngleEntry, &starAngle);
        selectedStarSpace = BoardSpaceStarGet(starIndex);
        linkedSpaceIndex = BoardSpaceLinkFlagSearch(0, selectedStarSpace, 0x04000000);
        BoardSpacePosGet(0, linkedSpaceIndex, &starPosition);
        BoardModelPosSetV(bowserStageModel, &starPosition);
        BoardModelPosSetV(bowserModel, &starPosition);
        BoardModelRotYSet(bowserStageModel, starAngle);
        BoardModelVisibilitySet(bowserModel, 1);
        BoardModelVisibilitySet(bowserStageModel, 1);
        BoardModelRotYSet(bowserModel, 0.0f);
    }
    BoardModelVisibilitySet(bowserModel, 1);
    BoardModelVisibilitySet(bowserStageModel, 1);
    BoardModelPosGet(bowserModel, &bowserPosition);
    BoardViewFocusGetPos(&boardViewPosition);
    VECSubtract(&boardViewPosition, &bowserPosition, &viewDirection);
    viewDirection.y = 0.0f;
    viewDistance = VECMag(&viewDirection);
    if (viewDistance > 3000.0f) {
        viewTransitionFrames = 180;
    } else if (viewDistance > 1500.0f) {
        viewTransitionFrames = 90;
    } else {
        viewTransitionFrames = 60;
    }
    BoardViewFocusSet(&bowserPosition, viewTransitionFrames);
            starSpaceIndex = BoardSpaceStarGet(w06BoardData->starSpaceIndex - 1);
            BoardSpaceTypeSet(0, starSpaceIndex, 10);
    BoardViewMoveEnd();
    while (!BoardViewMoveCheck()) {
        HuPrcVSleep();
    }
    BoardWinCreate(3, MAKE_MESSID(48, 1), -1);
    BoardWinWait();
    BoardWinKill();
}

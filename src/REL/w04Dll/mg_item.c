#include "REL/w04Dll.h"
#include "game/audio.h"
#include "game/data.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/board/audio.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/window.h"

#include "dolphin.h"
#include "ext_math.h"

static void UpdateW04ItemParticleEffect(void);
static void RunW04ItemAcquisitionAnimation(void);
static void UpdateW04ItemParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

static s16 itemRecipientPlayer;
static s16 offeredItemIds[3];
static s16 itemActionModelIds[4];
static s16 offeredItemModelIds[3];
static float itemStandHeight;
static s32 reservedItemState;
static s32 itemAcquisitionSoundId;
static ANIMDATA *itemEffectAnimation;

static char *itemModelHookNames[] = {
    "item1", "item2", "item3",
    "switch1", "switch2", "switch3"
};

static s32 itemMessageIds[] = {
    0x00000466,
    0x00000467,
    0x00000468
};

#if VERSION_JP
#define MDL_ID_SHIFT 0
#else
#define MDL_ID_SHIFT 1
#endif

static s32 itemModelDataIds[] = {
    DATA_MAKE_NUM(DATADIR_BOARD, 108 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 109 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 110 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 111 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 112 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 113 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 114 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 115 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 117 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 118 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 119 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 120 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 121 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_BOARD, 122 + MDL_ID_SHIFT),
    DATA_MAKE_NUM(DATADIR_W02, 24),
    DATA_MAKE_NUM(DATADIR_W02, 25),
    DATA_MAKE_NUM(DATADIR_W02, 26),
    -1
};

void W04ItemSetup(void) {
    Vec itemStandPosition;
    s32 i;

    BoardModelAttrSet(w04BoardModelIds[6], 0x40000002);
    for (i = 0; i < 3; i++) {
        itemActionModelIds[i] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 13), NULL, 1);
        BoardModelHookSet(w04BoardModelIds[6], itemModelHookNames[i + 3], itemActionModelIds[i]);
        BoardModelAttrSet(itemActionModelIds[i], 0x40000002);
    }
    BoardModelPosGet(w04BoardModelIds[6], &itemStandPosition);
    itemStandHeight = itemStandPosition.y;
}

void W04ItemAcquisitionEvent(void) {
    Vec itemHookPositions[3];
    Vec playerPosition;
    Vec playerTargetPosition;
    Vec cameraStartPosition;
    Vec cameraTargetPosition;
    float modelRotationAngle;
    s16 itemMotionId;
    s32 itemsAtHookCount;
    s32 i;

    itemRecipientPlayer = GWSystem.player_curr;
    itemMotionId = BoardModelMotionCreate(w04BoardModelIds[10], DATA_MAKE_NUM(DATADIR_BGUEST, 25));
    HuPrcSleep(60);
    modelRotationAngle = 0.0f;
    for (i = 0; i < 30; i++) {
        modelRotationAngle += 3.0f;
        BoardModelRotYSet(w04BoardModelIds[10], modelRotationAngle);
        HuPrcVSleep();
    }
    BoardModelMotionStart(w04BoardModelIds[10], itemMotionId, 0);
    for (i = 0; i < 3; i++) {
        Hu3DModelObjPosGet(BoardModelIDGet(w04BoardModelIds[6]), itemModelHookNames[i], &itemHookPositions[i]);
    }
    while (TRUE) {
        itemsAtHookCount = 0;
        for (i = 0; i < 3; i++) {
            BoardModelPosGet(offeredItemModelIds[i], &playerPosition);
            playerPosition.y += 2.0f;
            if (playerPosition.y >= itemHookPositions[i].y) {
                playerPosition.y = itemHookPositions[i].y;
                itemsAtHookCount++;
            }
            BoardModelPosSetV(offeredItemModelIds[i], &playerPosition);
        }
        if (itemsAtHookCount == 3) {
            break;
        }
        HuPrcVSleep();
    }
    for (i = 0; i < 3; i++) {
        BoardModelHookSet(w04BoardModelIds[6], itemModelHookNames[i], offeredItemModelIds[i]);
    }
    while (BoardModelMotionTimeGet(w04BoardModelIds[10]) < BoardModelMotionMaxTimeGet(w04BoardModelIds[10])) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(w04BoardModelIds[10], 1, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    BoardAudSeqPause(0, 1, 1000);
    BoardModelMotionTimeSet(w04BoardModelIds[6], 0.0f);
    BoardModelAttrReset(w04BoardModelIds[6], 0x40000002);
    HuPrcSleep((REFRESH_RATE+2)/3);
    HuAudFXPlay(0x469);
    HuPrcSleep(((REFRESH_RATE*5)+3)/6);
    BoardMusStart(1, 0x1E, 0x7F, 0);
    HuAudFXPlay(0x46C);
    while (BoardModelMotionTimeGet(w04BoardModelIds[6]) < 202.0f) {
        HuPrcVSleep();
    }
    BoardModelMotionTimeSet(w04BoardModelIds[6], 202.0f);
    BoardModelAttrSet(w04BoardModelIds[6], 0x40000002);
    BoardModelMotionKill(w04BoardModelIds[10], itemMotionId);
    cameraStartPosition.x = -35.0f;
    cameraStartPosition.y = cameraStartPosition.z = 0.0f;
    cameraTargetPosition.x = 0.0f;
    cameraTargetPosition.y = 0.0f;
    cameraTargetPosition.z = -150.0f;
    BoardCameraMotionStartEx(w04BoardModelIds[6], &cameraStartPosition, &cameraTargetPosition, 1500.0f, 25.0f, 21);
    BoardPlayerPosGet(itemRecipientPlayer, &playerPosition);
    Hu3DModelObjPosGet(BoardModelIDGet(w04BoardModelIds[6]), itemModelHookNames[4], &playerTargetPosition);
    playerTargetPosition.z = playerPosition.z - 50.0f;
    playerTargetPosition.y = playerPosition.y;
    BoardPlayerPosLerpStart(itemRecipientPlayer, &playerPosition, &playerTargetPosition, 0x3C);
    while (GWPlayer[itemRecipientPlayer].moving) {
        HuPrcVSleep();
    }
    BoardPlayerMotBlendSet(itemRecipientPlayer, 0xB4, 0xF);
    while (!BoardPlayerMotBlendCheck(itemRecipientPlayer)) {
        HuPrcVSleep();
    }
    BoardCameraMotionWait();
    RunW04ItemAcquisitionAnimation();
    BoardMGExit();
    while (TRUE) {
        if (BoardMGDoneFlagGet() == 1) {
            BoardMGDoneFlagSet(0);
            W04ItemModelsCleanup();
            break;
        }
        HuPrcVSleep();
    }
    HuPrcEnd();
}

static void UpdateW04ItemParticleEffect(void) {
    Vec modelPosition;
    float bobAngle;
    s16 particleModelId;

    bobAngle = 0.0f;
    particleModelId = (s16) HuPrcCurrentGet()->user_data;
    while (TRUE) {
        BoardModelPosGet(particleModelId, &modelPosition);
        modelPosition.y += 0.25 * sind(bobAngle);
        bobAngle += 5.0f;
        if (bobAngle >= 360.0f) {
            bobAngle -= 360.0f;
        }
        BoardModelPosSetV(particleModelId, &modelPosition);
        HuPrcVSleep();
    }
}

static void RunW04ItemAcquisitionAnimation(void) {
    Vec itemHookPositions[3];
    Vec modelScale;
    Vec playerVelocity;
    Vec playerPosition;
    Vec itemPosition;
    Vec playerStartPosition;
    float messageWindowSize[2];
    float jumpHeight;
    float jumpFrame;
    float distanceToItem;
    HUPROCESS *process;
    s16 selectedItemIndex;
    s16 itemActionModelId;
    s16 messageWindowId;
    s16 particleModelId;
    s16 sequenceId;
    s16 remainingItemTime;
    u16 acquiredItemIndex;
    u16 stickRepeat;
    s16 computerInputDelay;
    s16 timerFrames;
    s16 computerInputCount;
    u16 controllerPort;
    u16 pressedButtons;
    HU3DPARTICLE *particleData;
    s32 audioStreamId;
    s32 i;

    itemActionModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 14), NULL, 0);
    BoardModelAttrSet(itemActionModelId, 0x40000001);
    BoardModelVisibilitySet(itemActionModelId, 0);
    selectedItemIndex = 1;
    for (i = 0; i < 3; i++) {
        Hu3DModelObjPosGet(BoardModelIDGet(w04BoardModelIds[6]), itemModelHookNames[i + 3], &itemHookPositions[i]);
        itemHookPositions[i].y += 50.0f;
    }
    sequenceId = MGSeqCreate(3, 0);
    while (MGSeqStatGet(sequenceId) != 0) {
        HuPrcVSleep();
    }
    remainingItemTime = 5;
    timerFrames = 60;
    sequenceId = MGSeqCreate(1, remainingItemTime, 0x120, 0x40);
    HuWinMesMaxSizeGet(1, messageWindowSize, MAKE_MESSID(32, 22));
    messageWindowId = HuWinCreate(-10000.0f, 368.0f, messageWindowSize[0], messageWindowSize[1], 0);
    HuWinMesSet(messageWindowId, MAKE_MESSID(32, 22));
    HuWinBGTPLvlSet(messageWindowId, 0.0f);
    HuWinMesSpeedSet(messageWindowId, 0);
    BoardModelVisibilitySet(itemActionModelId, 1);
    BoardModelPosSetV(itemActionModelId, &itemHookPositions[selectedItemIndex]);
    computerInputDelay = 30;
    computerInputCount = 0;
    controllerPort = GWPlayer[itemRecipientPlayer].port;
    while (TRUE) {
        timerFrames--;
        if (timerFrames == 0) {
            timerFrames = 60;
            if (--remainingItemTime < 0) {
                break;
            }
            MGSeqParamSet(sequenceId, 1, remainingItemTime);
        }
        if (GWPlayer[itemRecipientPlayer].com) {
            if (computerInputDelay != 0) {
                computerInputDelay--;
            } else {
                computerInputDelay = frandmod(30) + 15;
                if (frandBool() || computerInputCount == 3) {
                    pressedButtons = 0x100;
                } else {
                    if (frandBool()) {
                        stickRepeat = 1;
                    } else {
                        stickRepeat = 2;
                    }
                    computerInputCount++;
                }
            }
        } else {
            pressedButtons = HuPadBtnDown[controllerPort];
            stickRepeat = HuPadDStkRep[controllerPort];
        }
        if (pressedButtons & 0x100) {
            break;
        }
        if ((stickRepeat & 1) && selectedItemIndex != 0) {
            HuAudFXPlay(0);
            selectedItemIndex--;
        }
        if ((stickRepeat & 2) && selectedItemIndex != 2) {
            HuAudFXPlay(0);
            selectedItemIndex++;
        }
        BoardModelPosSetV(itemActionModelId, &itemHookPositions[selectedItemIndex]);
        HuPrcVSleep();
    }
    HuAudFXPlay(2);
    MGSeqParamSet(sequenceId, 2, -1);
    HuWinKill(messageWindowId);
    for (i = 0; i < 360; i += 12) {
        modelScale.x = modelScale.y = modelScale.z = 1.0 + 0.8f * sind(i % 180);
        BoardModelScaleSetV(itemActionModelId, &modelScale);
        HuPrcVSleep();
    }
    BoardModelVisibilitySet(itemActionModelId, 0);
    BoardPlayerPosGet(itemRecipientPlayer, &playerStartPosition);
    BoardPlayerMotionShiftSet(itemRecipientPlayer, 4, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    jumpFrame = 0.0f;
    jumpHeight = 12.0f;
    playerVelocity = itemHookPositions[selectedItemIndex];
    BoardPlayerPosGet(itemRecipientPlayer, &playerPosition);
    VECSubtract(&playerVelocity, &playerPosition, &playerVelocity);
    VECScale(&playerVelocity, &playerVelocity, 0.05f);
    while (TRUE) {
        BoardPlayerPosGet(itemRecipientPlayer, &playerPosition);
        playerVelocity.y = jumpHeight - jumpFrame * jumpFrame * 0.25f * 0.075f;
        jumpFrame += 1.0f;
        VECAdd(&playerVelocity, &playerPosition, &playerPosition);
        if (playerPosition.y <= itemHookPositions[selectedItemIndex].y - 40.0f && playerVelocity.y < 0.0f) {
            playerPosition.y = itemHookPositions[selectedItemIndex].y - 40.0f;
            break;
        }
        BoardPlayerPosSetV(itemRecipientPlayer, &playerPosition);
        modelScale = itemHookPositions[selectedItemIndex];
        modelScale.y = playerPosition.y = 0.0f;
        VECSubtract(&modelScale, &playerPosition, &playerPosition);
        distanceToItem = VECMag(&playerPosition);
        if (distanceToItem <= 1.0f) {
            playerVelocity.x = playerVelocity.z = 0.0f;
        }
        HuPrcVSleep();
    }
    BoardAudSeqFadeOut(1, 100);
    HuAudFXPlay(itemMessageIds[selectedItemIndex]);
    BoardModelMotionTimeSet(itemActionModelIds[selectedItemIndex], 0.0f);
    BoardModelAttrReset(itemActionModelIds[selectedItemIndex], 0x40000002);
    BoardPlayerPosSetV(itemRecipientPlayer, &playerPosition);
    BoardPlayerMotionShiftSet(itemRecipientPlayer, 1, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    while (BoardModelMotionTimeGet(itemActionModelIds[selectedItemIndex]) < BoardModelMotionMaxTimeGet(itemActionModelIds[selectedItemIndex])) {
        if (BoardModelMotionTimeGet(itemActionModelIds[selectedItemIndex]) >= 13.0f) {
            BoardPlayerPosGet(itemRecipientPlayer, &playerPosition);
            playerPosition.y += 1.0f;
            BoardPlayerPosSetV(itemRecipientPlayer, &playerPosition);
        }
        HuPrcVSleep();
    }
    acquiredItemIndex = frandmod(3);
    BoardModelHookReset(w04BoardModelIds[6]);
    for (i = 0; i < 3; i++) {
        BoardModelHookSet(w04BoardModelIds[6], itemModelHookNames[i + 3], itemActionModelIds[i]);
        if (i != acquiredItemIndex) {
            BoardModelVisibilitySet(offeredItemModelIds[i], 0);
        }
    }
    BoardModelHookSet(w04BoardModelIds[6], itemModelHookNames[1], offeredItemModelIds[acquiredItemIndex]);
    BoardModelAttrReset(w04BoardModelIds[6], 0x40000002);
    HuPrcSleep(REFRESH_RATE/2);
    HuAudFXPlay(0x310);
    while (BoardModelMotionTimeGet(w04BoardModelIds[6]) < BoardModelMotionMaxTimeGet(w04BoardModelIds[6])) {
        HuPrcVSleep();
    }
    BoardModelHookReset(w04BoardModelIds[6]);
    for (i = 0; i < 3; i++) {
        BoardModelHookSet(w04BoardModelIds[6], itemModelHookNames[i + 3], itemActionModelIds[i]);
    }
    Hu3DModelObjPosGet(BoardModelIDGet(w04BoardModelIds[6]), itemModelHookNames[1], &itemPosition);
    BoardModelPosSetV(offeredItemModelIds[acquiredItemIndex], &itemPosition);
    process = HuPrcChildCreate(UpdateW04ItemParticleEffect, 0x2002, 0x800, 0, boardMainProc);
    process->user_data = (void*) offeredItemModelIds[acquiredItemIndex];
    particleModelId = Hu3DParticleCreate(itemEffectAnimation, 100);
    particleData = Hu3DData[particleModelId].hookData;
    particleData->emitCnt = offeredItemModelIds[acquiredItemIndex];
    Hu3DParticleHookSet(particleModelId, UpdateW04ItemParticle);
    Hu3DParticleColSet(particleModelId, 0xFF, 0xFF, 0);
    Hu3DModelPosSet(particleModelId, 0.0f, 0.0f, 0.0f);
    Hu3DParticleBlendModeSet(particleModelId, 1);
    Hu3DModelLayerSet(particleModelId, 2);
    itemAcquisitionSoundId = HuAudFXPlay(0x35F);
    BoardPlayerMotBlendSet(itemRecipientPlayer, 0, 0xF);
    while (!BoardPlayerMotBlendCheck(itemRecipientPlayer)) {
        HuPrcVSleep();
    }
    BoardPlayerMotionShiftSet(itemRecipientPlayer, 4, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    jumpFrame = 0.0f;
    jumpHeight = 12.0f;
    BoardPlayerPosGet(itemRecipientPlayer, &playerPosition);
    VECSubtract(&playerStartPosition, &playerPosition, &playerVelocity);
    VECScale(&playerVelocity, &playerVelocity, 0.05f);
    while (TRUE) {
        BoardPlayerPosGet(itemRecipientPlayer, &playerPosition);
        playerVelocity.y = jumpHeight - jumpFrame * jumpFrame * 0.25f * 0.075f;
        jumpFrame += 1.0f;
        VECAdd(&playerVelocity, &playerPosition, &playerPosition);
        if (playerPosition.y < playerStartPosition.y) {
            playerPosition.y = playerStartPosition.y;
            break;
        }
        BoardPlayerPosSetV(itemRecipientPlayer, &playerPosition);
        modelScale = playerStartPosition;
        modelScale.y = playerPosition.y = 0.0f;
        VECSubtract(&modelScale, &playerPosition, &playerPosition);
        distanceToItem = VECMag(&playerPosition);
        if (distanceToItem <= 1.0f) {
            playerVelocity.x = playerVelocity.z = 0.0f;
        }
        HuPrcVSleep();
    }
    omVibrate(itemRecipientPlayer, 12, 6, 6);
    BoardPlayerPosSetV(itemRecipientPlayer, &playerPosition);
    BoardPlayerMotionShiftSet(itemRecipientPlayer, 1, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    BoardModelPosGet(offeredItemModelIds[acquiredItemIndex], &itemPosition);
    playerVelocity = itemPosition;
    playerVelocity.z += 100.0f;
    VECSubtract(&playerVelocity, &itemPosition, &playerVelocity);
    VECScale(&playerVelocity, &playerVelocity, 0.033333335f);
    for (i = 0; i < 30; i++) {
        BoardModelPosGet(offeredItemModelIds[acquiredItemIndex], &itemPosition);
        VECAdd(&itemPosition, &playerVelocity, &itemPosition);
        BoardModelPosSetV(offeredItemModelIds[acquiredItemIndex], &itemPosition);
        HuPrcVSleep();
    }
    BoardPlayerPosGet(itemRecipientPlayer, &playerPosition);
    itemPosition.y = playerPosition.y;
    BoardPlayerPosLerpStart(itemRecipientPlayer, &playerPosition, &itemPosition, 0x3C);
    while (GWPlayer[itemRecipientPlayer].moving) {
        HuPrcVSleep();
    }
    BoardPlayerMotBlendSet(itemRecipientPlayer, 0, 0xF);
    while (!BoardPlayerMotBlendCheck(itemRecipientPlayer)) {
        HuPrcVSleep();
    }
    distanceToItem = 90.0f;
    for (i = 0; i < 15; i++) {
        distanceToItem -= 3.0f;
        BoardModelRotYSet(w04BoardModelIds[10], distanceToItem);
        HuPrcVSleep();
    }
    BoardWinCreate(2, MAKE_MESSID(32, 15), 3);
    BoardWinInsertMesSet(MAKE_MESSID(8, offeredItemIds[acquiredItemIndex]), 0);
    BoardWinWait();
    BoardWinKill();
    HuAudFXPlay(0x30D);
    HuPrcKill(process);
    BoardPlayerPosGet(itemRecipientPlayer, &playerPosition);
    BoardModelPosGet(offeredItemModelIds[acquiredItemIndex], &itemPosition);
    jumpHeight = 1.0f;
    distanceToItem = (playerPosition.y - itemPosition.y) / 30.0f;
    jumpFrame = -0.04f;
    messageWindowId = 0;
    HuAudFXFadeOut(itemAcquisitionSoundId, 0x1F3);
    while (messageWindowId == 0) {
        itemPosition.y += distanceToItem;
        jumpHeight += jumpFrame;
        if (jumpHeight < 0.0f) {
            jumpHeight = 0.0f;
            messageWindowId = 1;
        }
        modelScale.x = modelScale.y = modelScale.z = jumpHeight;
        BoardModelPosSetV(offeredItemModelIds[acquiredItemIndex], &itemPosition);
        BoardModelScaleSetV(offeredItemModelIds[acquiredItemIndex], &modelScale);
        HuPrcVSleep();
    }
    HuAudFXStop(itemAcquisitionSoundId);
    BoardModelVisibilitySet(offeredItemModelIds[acquiredItemIndex], 0);
    BoardPlayerItemAdd(itemRecipientPlayer, offeredItemIds[acquiredItemIndex]);
    omVibrate(itemRecipientPlayer, 12, 6, 6);
    audioStreamId = HuAudSStreamPlay(2);
    BoardPlayerMotionShiftSet(itemRecipientPlayer, 7, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    while (!BoardPlayerMotionEndCheck(itemRecipientPlayer)) {
        HuPrcVSleep();
    }
    distanceToItem = 45.0f;
    for (i = 0; i < 15; i++) {
        distanceToItem -= 3.0f;
        BoardModelRotYSet(w04BoardModelIds[10], distanceToItem);
        HuPrcVSleep();
    }
    BoardModelRotYSet(w04BoardModelIds[10], 0.0f);
    HuPrcSleep(REFRESH_RATE/2);
    while (HuAudSStreamStatGet(audioStreamId) != 0) {
        HuPrcVSleep();
    }
    BoardAudSeqPause(0, 0, 1000);
    BoardWinCreate(2, MAKE_MESSID(32, 16), 3);
    BoardWinWait();
    BoardWinKill();
    BoardModelMotionTimeSet(w04BoardModelIds[6], 0.0f);
    BoardModelAttrSet(w04BoardModelIds[6], 0x40000002);
    Hu3DModelKill(particleModelId);
    BoardModelKill(itemActionModelId);
}

void W04ItemModelsCleanup(void) {
    s32 i;

    for (i = 0; i < 3; i++) {
        BoardModelKill(offeredItemModelIds[i]);
    }
    if (itemEffectAnimation) {
        HuSprAnimKill(itemEffectAnimation);
        itemEffectAnimation = NULL;
    }
}

void W04ItemModelsSpawn(s16 itemIds[3]) {
    Vec itemHookPositions[3];
    Vec itemMovementDirections[3];
    Vec itemPosition;
    Vec itemScale;
    float itemFallFrames[3];
    float verticalStep;
    float horizontalDistance;
    float scaleProgress;
    s32 landedItemCount;
    s32 i;

    for (i = 0; i < 3; i++) {
        offeredItemIds[i] = itemIds[i];
        offeredItemModelIds[i] = BoardModelCreate(itemModelDataIds[offeredItemIds[i]], NULL, 0);
        BoardModelVisibilitySet(offeredItemModelIds[i], 0);
        if (offeredItemIds[i] == 11) {
            BoardModelAttrSet(offeredItemModelIds[i], 0x40000001);
        }
        if (offeredItemIds[i] == 5) {
            Hu3DData[BoardModelIDGet(offeredItemModelIds[i])].mtx[1][3] = -50.0f;
        }
        if (offeredItemIds[i] == 4) {
            BoardModelMotionSpeedSet(offeredItemModelIds[i], 0.0f);
        }
    }
    if (itemEffectAnimation == NULL) {
        itemEffectAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_EFFECT, 0));
        HuSprAnimLock(itemEffectAnimation);
    }
    BoardModelMotionTimeSet(w04BoardModelIds[6], 0.0f);
    BoardModelAttrSet(w04BoardModelIds[6], 0x40000002);
    itemScale.x = itemScale.y = itemScale.z = 0.0f;
    for (i = 0; i < 3; i++) {
        Hu3DModelObjPosGet(BoardModelIDGet(w04BoardModelIds[6]), itemModelHookNames[i], &itemHookPositions[i]);
        itemHookPositions[i].y += -50.0f;
        BoardModelVisibilitySet(offeredItemModelIds[i], 1);
        BoardModelPosSetV(offeredItemModelIds[i], &itemHookPositions[i]);
        BoardModelPosGet(w04BoardModelIds[10], &itemPosition);
        BoardModelPosSetV(offeredItemModelIds[i], &itemPosition);
        BoardModelScaleSetV(offeredItemModelIds[i], &itemScale);
        VECSubtract(&itemHookPositions[i], &itemPosition, &itemMovementDirections[i]);
        VECNormalize(&itemMovementDirections[i], &itemMovementDirections[i]);
        itemFallFrames[i] = 0.0f;
    }
    itemMovementDirections[0].y = 16.0f;
    itemMovementDirections[1].y = 13.0f;
    itemMovementDirections[2].y = 10.0f;
    HuAudFXPlay(0x362);
    scaleProgress = 0.0f;
    while (TRUE) {
        landedItemCount = 0;
        scaleProgress += 0.05f;
        if (scaleProgress > 1.0f) {
            scaleProgress = 1.0f;
        }
        itemScale.x = itemScale.y = itemScale.z = scaleProgress;
        for (i = 0; i < 3; i++) {
            BoardModelPosGet(offeredItemModelIds[i], &itemPosition);
            verticalStep = itemMovementDirections[i].y - itemFallFrames[i] * itemFallFrames[i] * 0.25f * 0.075f;
            itemPosition.x += 6.55f * itemMovementDirections[i].x;
            itemPosition.z += 6.55f * itemMovementDirections[i].z;
            itemPosition.y += verticalStep;
            horizontalDistance = VECDistanceXZ(&itemHookPositions[i], &itemPosition);
            if (horizontalDistance < 3.0f) {
                itemMovementDirections[i].x = itemMovementDirections[i].z = 0.0f;
                itemPosition.x = itemHookPositions[i].x;
                itemPosition.z = itemHookPositions[i].z;
            }
            if (itemPosition.y <= itemHookPositions[i].y && verticalStep < 0.0f) {
                itemPosition.y = itemHookPositions[i].y;
                landedItemCount++;
            }
            BoardModelPosSetV(offeredItemModelIds[i], &itemPosition);
            BoardModelScaleSetV(offeredItemModelIds[i], &itemScale);
            itemFallFrames[i] += 1.0f;
        }
        if (landedItemCount == 3) {
            break;
        }
        HuPrcVSleep();
    }
    for (i = 0; i < 3; i++) {
        BoardModelPosSetV(offeredItemModelIds[i], &itemHookPositions[i]);
    }
}

void W04ItemModelsShrink(void) {
    Vec itemScale;
    float scale;
    s32 i;
    s32 j;

    scale = 1.0f;
    for (i = 0; i < 20; i++) {
        scale -= 0.05f;
        if (scale < 0.0f) {
            scale = 0.0f;
        }
        itemScale.x = itemScale.y = itemScale.z = scale;
        for (j = 0; j < 3; j++) {
            BoardModelScaleSetV(offeredItemModelIds[j], &itemScale);
        }
        HuPrcVSleep();
    }
}

static void UpdateW04ItemParticle(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix) {
    Vec emitterPosition;
    HU3DPARTICLEDATA *particleData;
    float emitterScale;
    float particleAngle;
    float particleRadius;
    s32 particleSlot;
    s32 i;

    if (particle->count == 0) {
        particleData = particle->data;
        for (i = 0; i < particle->maxCnt; i++, particleData++) {
            particleData->color.a = 0;
            particleData->scale = 0.0f;
        }
        particle->dataCnt = 0;
    }
    BoardModelScaleGet(particle->emitCnt, &emitterPosition);
    emitterScale = emitterPosition.x;
    BoardModelPosGet(particle->emitCnt, &emitterPosition);
    for (i = 0; i < 22; i++) {
        particleData = particle->data;
        for (particleSlot = 0; particleSlot < particle->maxCnt; particleSlot++, particleData++) {
            if (particleData->scale == 0.0f) {
                break;
            }
        }
        if (particleSlot != particle->maxCnt) {
            particleAngle = 0.003921569f * frand8() * 360.0f;
            particleRadius = emitterScale * (20.0f + 0.003921569f * frand8() * 60.0f);
            particleData->pos.x = emitterPosition.x + particleRadius * sind(particleAngle);
            particleData->pos.z = emitterPosition.z + particleRadius * cosd(particleAngle);
            particleData->pos.y = emitterPosition.y + emitterScale * (-30.0f + 0.003921569f * frand8() * 40.0f);
            particleData->vel.x = 0.5f + 0.003921569f * frand8() * 3.0f;
            particleData->vel.y = 0.3f + 0.003921569f * frand8() * 2.0f;
            particleData->color.a = 0xB4;
            particleData->scale = emitterScale * 15.0f;
        }
    }
    particleData = particle->data;
    for (i = 0; i < particle->maxCnt; i++, particleData++) {
        if (particleData->scale != 0.0f) {
            particleData->pos.y -= particleData->vel.x;
            particleData->scale -= particleData->vel.y;
            if (particleData->scale <= 0.0f) {
                particleData->scale = 0.0f;
            }
        }
    }
}

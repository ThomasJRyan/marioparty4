#include "REL/w06Dll.h"
#include "game/audio.h"
#include "game/data.h"
#include "game/gamework_data.h"
#include "game/objsub.h"
#include "game/process.h"
#include "game/window.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/window.h"

#include "dolphin.h"
#include "ext_math.h"

static void W06FireEventProcess(void);
static void W06FireEventCleanup(void);
static s32 W06FirePostTurnHook(void);
static void W06FireAffectedPlayersFind(s32 activePlayerIndex);
static void W06FirePlayerFacingSet(float activePlayerIndex);
static void W06FirePlayerReactionSet(s32 activePlayerIndex);
static void W06FirePlayerItemsRemove(void);
static void W06FireEffectCreate(s32 activePlayerIndex);
static void W06FireCameraSetup(s32 activePlayerIndex);
static s32 W06FireItemSelect(s32 activePlayerIndex);

static s8 fireOnLeftSide;
static s8 fireAffectedPlayers[4];
static s16 fireModel;
static s32 fireSoundEffect;
static s32 fireSoundHandle;
static u8 fireReservedData[0x24];
static HUPROCESS *fireProcess;

static s32 fireCharacterSounds[8] = {
    0x00000123,
    0x00000163,
    0x000001A3,
    0x000001E3,
    0x00000223,
    0x00000263,
    0x000002A3,
    0x000002E3
};

static s8 fireEventState = -1;
static s16 fireCameraModel = -1;
static s16 fireEffectModel = -1;
static s16 fireAuxiliaryModel = -1;

#if VERSION_PAL
static s32 fireItemMessages[] = {
    MAKE_MESSID(8, 0),
    MAKE_MESSID(8, 1),
    MAKE_MESSID(8, 2),
    MAKE_MESSID(8, 3),
    MAKE_MESSID(8, 4),
    MAKE_MESSID(8, 5),
    MAKE_MESSID(8, 6),
    MAKE_MESSID(8, 7),
    MAKE_MESSID(8, 8),
    MAKE_MESSID(8, 9),
    MAKE_MESSID(8, 10),
    MAKE_MESSID(8, 11),
    MAKE_MESSID(8, 12),
    MAKE_MESSID(8, 13)
};
#else
static s32 fireItemMessages[] = {
    MAKE_MESSID(8, 14),
    MAKE_MESSID(8, 15),
    MAKE_MESSID(8, 16),
    MAKE_MESSID(8, 17),
    MAKE_MESSID(8, 18),
    MAKE_MESSID(8, 19),
    MAKE_MESSID(8, 20),
    MAKE_MESSID(8, 21),
    MAKE_MESSID(8, 22),
    MAKE_MESSID(8, 23),
    MAKE_MESSID(8, 24),
    MAKE_MESSID(8, 25),
    MAKE_MESSID(8, 26),
    MAKE_MESSID(8, 27)
};
#endif
void W06FireEventStart(void) {
    BoardRollDispSet(0);
    fireProcess = HuPrcChildCreate(W06FireEventProcess, 0x2003, 0x3800, 0, boardMainProc);
    HuPrcDestructorSet2(fireProcess, W06FireEventCleanup);
    while (fireProcess) {
        HuPrcVSleep();
    }
    BoardRollDispSet(1);
}

static void W06FireEventProcess(void) {
    s32 currentPlayerIndex;
    s32 currentSpaceIndex;

    fireSoundHandle = -1;
    currentPlayerIndex = GWSystem.player_curr;
    currentSpaceIndex = GWPlayer[currentPlayerIndex].space_curr;
    if (BoardSpaceFlagGet(0, currentSpaceIndex) & 0x2000) {
        fireOnLeftSide = 0;
        fireModel = w06MapObjectModelIds[2];
        fireSoundEffect = 0x4A6;
    } else {
        fireOnLeftSide = 1;
        fireModel = w06MapObjectModelIds[3];
        fireSoundEffect = 0x49B;
    }
    W06FireAffectedPlayersFind(currentPlayerIndex);
    W06FireCameraSetup(currentPlayerIndex);
    BoardCameraMotionWait();
    BoardModelMotionShiftSet(fireModel, 0, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    HuPrcSleep(8);
    while (BoardModelMotionTimeGet(fireModel) < 22.0f) {
        HuPrcVSleep();
    }
    W06FireEffectCreate(currentPlayerIndex);
    BoardModelMotionStartEndSet(fireModel, 30, 60);
    fireEventState = 2;
    HuPrcSleep(30);
    W06FirePlayerReactionSet(1);
    while (!BoardModelMotionEndCheck(fireEffectModel)) {
        HuPrcVSleep();
    }
    fireEventState = 3;
    BoardModelHookReset(fireModel);
    if (fireSoundHandle != -1) {
        HuAudFXStop(fireSoundHandle);
    }
    BoardModelMotionShiftSet(fireModel, 0, 60.0f, 8.0f, HU3D_MOTATTR_NONE);
    W06FirePlayerFacingSet(0.0f);
    W06FirePlayerItemsRemove();
    while (!BoardModelMotionEndCheck(fireModel)) {
        HuPrcVSleep();
    }
    BoardPlayerPostTurnHookSet(currentPlayerIndex, W06FirePostTurnHook);
    HuPrcEnd();
}

static void W06FireEventCleanup(void) {
    if (fireEffectModel != -1) {
        BoardModelKill(fireEffectModel);
        fireEffectModel = -1;
    }
    if (fireCameraModel != -1) {
        BoardModelKill(fireCameraModel);
        fireCameraModel = -1;
    }
    fireProcess = NULL;
}

static s32 W06FirePostTurnHook(void) {
    W06FirePlayerReactionSet(0);
    return 1;
}

static void W06FireAffectedPlayersFind(s32 playerIndex) {
    Vec firePosition;
    Vec playerPosition;
    Vec fireDirection;
    s32 affectedSideFlag;
    s32 i;

    if (fireOnLeftSide == 0) {
        affectedSideFlag = 0x2000;
    } else {
        affectedSideFlag = 0x1000;
    }
    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardModelPosGet(fireModel, &firePosition);
    VECSubtract(&firePosition, &playerPosition, &fireDirection);
    for (i = 0; i < 4; i++) {
        if (affectedSideFlag & BoardSpaceFlagGet(0, GWPlayer[i].space_curr)) {
            fireAffectedPlayers[i] = 1;
        } else {
            fireAffectedPlayers[i] = 0;
        }
    }
    W06FirePlayerFacingSet(atan2d(fireDirection.x, fireDirection.z));
}

static void W06FirePlayerFacingSet(float facingAngle) {
    s32 isBlendPending;
    s32 i;

    for (i = 0; i < 4; i++) {
        if (fireAffectedPlayers[i] != 0) {
            BoardPlayerMotBlendSet(i, facingAngle, 15);
        }
    }
    do {
        isBlendPending = 0;
        for (i = 0; i < 4; i++) {
            if (fireAffectedPlayers[i] != 0 && !BoardPlayerMotBlendCheck(i)) {
                isBlendPending = 1;
            }
        }
        HuPrcVSleep();
    } while (isBlendPending != 0);
}

static void W06FirePlayerReactionSet(s32 shouldPlayReaction) {
    s32 i;

    for (i = 0; i < 4; i++) {
        if (fireAffectedPlayers[i] == 0) {
            continue;
        }
        if (shouldPlayReaction != 0) {
            omVibrate(i, 12, 4, 2);
            HuAudFXPlay(fireCharacterSounds[GWPlayer[i].character]);
        }
        BoardPlayerCopyEyeMat(i, shouldPlayReaction);
    }
}

static void W06FirePlayerItemsRemove(void) {
    s32 selectedItemSlot;
    s32 selectedItemId;
    #if VERSION_PAL
    s32 playerNo = GWSystem.player_curr;
    #endif
    s32 characterMessageId;
    s32 i;
    
    for (i = 0; i < 4; i++) {
        if (fireAffectedPlayers[i] == 0) {
            continue;
        }
        selectedItemSlot = W06FireItemSelect(i);
        if (selectedItemSlot != -1) {
            selectedItemId = GWPlayer[i].items[selectedItemSlot];
            BoardPlayerItemRemove(i, selectedItemSlot);
            characterMessageId = BoardPlayerGetCharMess(i);
            BoardWinCreate(0, MAKE_MESSID(48, 16), -1);
            BoardWinInsertMesSet(characterMessageId, 0);
            BoardWinInsertMesSet(fireItemMessages[selectedItemId], 1);
            #if VERSION_PAL
            GWSystem.player_curr = i;
            #endif
            BoardWinPlayerSet(i);
            BoardWinWait();
            BoardWinKill();
        }
    }
    #if VERSION_PAL
    GWSystem.player_curr = playerNo;
    #endif
}

static void W06FireEffectCreate(s32 playerIndex) {
    Vec firePosition;
    float effectFacingAngle;

    if (fireOnLeftSide == 0) {
        effectFacingAngle = -90.0f;
    } else {
        effectFacingAngle = 90.0f;
    }
    fireEffectModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W06, 15), NULL, 0);
    BoardModelPosGet(fireModel, &firePosition);
    BoardModelMotionStart(fireEffectModel, 0, 0);
    BoardModelHookSet(fireModel, "fire", fireEffectModel);
    fireSoundHandle = HuAudFXPlay(fireSoundEffect);
}

static void W06FireCameraSetup(s32 playerIndex) {
    Vec cameraTarget;
    Vec firePosition;
    Vec targetSpacePosition;
    Vec cameraOffset;
    u32 targetSpaceFlag;
    u32 currentSpaceFlags;
    s16 targetSpaceIndex;
    s32 i;

    if (fireOnLeftSide == 0) {
        targetSpaceFlag = 0x6000;
    } else {
        targetSpaceFlag = 0x5000;
    }
    for (targetSpaceIndex = i = 0; i < BoardSpaceCountGet(0); i++) {
        currentSpaceFlags = BoardSpaceFlagGet(0, i + 1);
        if (targetSpaceFlag == (currentSpaceFlags & targetSpaceFlag)) {
            targetSpaceIndex = i + 1;
            break;
        }
    }
    BoardModelPosGet(fireModel, &firePosition);
    BoardSpacePosGet(0, targetSpaceIndex, &targetSpacePosition);
    VECSubtract(&targetSpacePosition, &firePosition, &cameraOffset);
    cameraOffset.x *= 0.4f;
    cameraOffset.y *= 0.4f;
    cameraOffset.z *= 0.4f;
    VECAdd(&firePosition, &cameraOffset, &cameraTarget);
    fireCameraModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 10), NULL, 0);
    BoardModelVisibilitySet(fireCameraModel, 0);
    BoardModelPosSetV(fireCameraModel, &cameraTarget);
    cameraOffset.x = 0.0f;
    cameraOffset.y = 200.0f;
    cameraOffset.z = 0.0f;
    BoardCameraMotionStartEx(fireCameraModel, 0, &cameraOffset, 2500.0f, -1.0f, 21);
    BoardCameraMotionWait();
}

static s8 fireItemWeights[] = {
    0, 0, 1, 1, 0,
    0, 1, 1, 1, 1,
    2, 2, 2, 2
};

static s8 fireRankThresholds[] = { 40, 30, 20, 10 };

static s8 fireFullInventoryThresholds[4][2] = {
    { 35, 80 }, { 30, 70 },
    { 20, 60 }, { 10, 50 }
};

static s32 W06FireItemSelect(s32 playerIndex) {
    s8 heldItemIds[ARRAY_COUNT(GWPlayer->items)];
    s8 heldItemSlots[ARRAY_COUNT(GWPlayer->items)];
    s32 playerItemCount;
    s32 itemId;
    s32 itemCount;
    s32 randomPercentile;
    s32 selectedItemIndex;
    s32 i;

    playerItemCount = BoardPlayerItemCount(playerIndex);
    if (playerItemCount == 0) {
        return -1;
    }
    for (itemCount = i = 0; i < ARRAY_COUNT(GWPlayer->items); i++) {
        itemId = GWPlayer[playerIndex].items[i];
        if (itemId != BOARD_ITEM_NONE) {
            heldItemIds[itemCount] = itemId;
            heldItemSlots[itemCount] = i;
            itemCount++;
        }
    }
    if (itemCount == 0) {
        return -1;
    }
    if (itemCount == 1) {
        return 0;
    }
    randomPercentile = BoardRandMod(100);
    if (fireItemWeights[heldItemIds[0]] > fireItemWeights[heldItemIds[1]]) {
        if (fireRankThresholds[GWPlayer[playerIndex].rank] > randomPercentile) {
            selectedItemIndex = 1;
        } else {
            selectedItemIndex = 0;
        }
    } else if (fireRankThresholds[GWPlayer[playerIndex].rank] > randomPercentile) {
        selectedItemIndex = 0;
    } else {
        selectedItemIndex = 1;
    }
    if (itemCount == ARRAY_COUNT(GWPlayer->items)) {
        if (fireItemWeights[heldItemIds[selectedItemIndex]] > fireItemWeights[heldItemIds[2]]) { //TODO: heldItemIds[ARRAY_COUNT(GWPlayer->items)-1] ?
            if (fireFullInventoryThresholds[GWPlayer[playerIndex].rank][1] > randomPercentile) {
                selectedItemIndex = 2; //TODO: selectedItemIndex = ARRAY_COUNT(GWPlayer->items)-1 ?
            }
        } else if (fireFullInventoryThresholds[GWPlayer[playerIndex].rank][1] < randomPercentile) {
            selectedItemIndex = 2;  //TODO: selectedItemIndex = ARRAY_COUNT(GWPlayer->items)-1 ?
        }
    }
    return heldItemSlots[selectedItemIndex];
}

static char *fireItemNames[] = {
    "01 : CHIBI KINOKO",
    "02 : DEKA KINOKO",
    "03 : SUPER CHIBI KINOKO",
    "04 : SUPER DEKA KINOKO",
    "05 : DEKACHIBI HAMMER",
    "06 : WARP DOKAN",
    "07 : TRADE CARD",
    "08 : BIRI-Q SEAL",
    "09 : TERE-YOKE LIGHT",
    "10 : WAN-WAN WHISTLE",
    "11 : KOOPA SUIT",
    "12 : TERESA SUISYO",
    "13 : MAJIN NO LAMP",
    "14 : ITEM BUKURO"
};

static u8 fireReservedEffects[0x4C] = { 0 };

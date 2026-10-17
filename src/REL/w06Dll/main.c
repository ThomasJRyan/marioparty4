#include "REL/w06Dll.h"
#include "REL/board_executor.h"
#include "game/audio.h"
#include "game/data.h"
#include "game/flag.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/process.h"
#include "game/board/boo_house.h"
#include "game/board/lottery.h"
#include "game/board/main.h"
#include "game/board/map_object.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/shop.h"
#include "game/board/space.h"
#include "game/board/star.h"

#include "dolphin.h"

typedef struct {
    struct {
        u8 isActive : 1;
    };
    s8 soundCuePlayed[10];
} W06BoardAudioWork;

typedef struct {
    /* 0x00 */ s32 motionFrame;
    /* 0x04 */ Vec soundPosition;
} W06AmbientSoundCue;

static void W06SetBoardLighting(void);
static void W06ClearBoardFog(void);
static s32 W06HandleBridgeSpaceWalk(void);
static s32 W06HandleMiniGameSpaceWalk(void);
static s32 W06HandleSpaceLanding(void);
static void W06StartAmbientAudio(void);
static void W06StopAmbientAudio(void);
static void W06UpdateAmbientAudio(OMOBJ *audioObject);

s16 w06MapObjectModelIds[MAPOBJ_MAX];

static BoardMapObject w06MapObjects[MAPOBJ_MAX] = {
    { {   450.0f, 200.0f, -2950.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W06, 6) },
    { { -1650.0f,   0.0f,  2100.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W06, 8) },
    { {  2700.0f, 100.0f, -1950.0f }, { 0.0f, -89.999985f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W06, 5) },
    { { -3000.0f,   0.0f,  2550.0f }, { 0.0f,  89.999985f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_W06, 5) },
    { { -1800.0f,   0.0f,  2250.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_BGUEST, 33) },
    { {   300.0f, 200.0f, -2850.0f }, { 0.0f,        0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, DATA_MAKE_NUM(DATADIR_BGUEST, 33) }
};

static OMOBJ *w06AudioListenerObject;
static s32 w06AllAudioPaused;
W06BoardData *w06BoardData;

static s16 w06BoardBackgroundModel = -1;
static s16 w06BoardSecondaryModel = -1;
static s16 w06StarHostModel = -1;
static s16 w06ServiceHostModel = -1;

static s32 w06BoardModelData[] = {
    DATA_MAKE_NUM(DATADIR_W06, 18),
    -1
};

static s32 w06GuestModelData[] = {
    DATA_MAKE_NUM(DATADIR_BGUEST, 34),
    -1
};

static W06AmbientSoundCue w06AmbientSoundCues[] = {
    0x00000002, { -3359.9998f, 170.0f, -3559.9998f },
    0x0000002A, {  3540.0002f, 170.0f,     1320.0f },
    0x00000052, {     -710.0f, 170.0f,    -4580.0f },
    0x0000007A, {    -3670.0f, 170.0f,     1020.0f },
    0x000000A2, {  1970.0001f, 170.0f, -3840.0002f },
    0x000000CA, {    -3160.0f, 170.0f,    -3770.0f },
    0x000000F2, {    -3500.0f, 170.0f,    -2340.0f },
    0x0000011A, {    -3700.0f, 170.0f, -810.00005f },
    0x00000142, {      100.0f, 170.0f,    -4100.0f },
    0x0000016A, {  3290.0002f, 170.0f,     -340.0f }
};

void BoardCreate(void) {
    BoardMapObject *mapObject;
    s32 *mapObjectData;
    s32 i;

    w06BoardData = (W06BoardData*) GWSystem.boardData;
    BoardSpaceInit(DATA_MAKE_NUM(DATADIR_W06, 0));
    w06BoardBackgroundModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W06, 1), NULL, 0);
    fn_8006DDE8(w06BoardBackgroundModel, -1.0f);
    BoardModelMotionStart(w06BoardBackgroundModel, 0, 0x40000001);
    w06BoardSecondaryModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W06, 2), NULL, 0);
    fn_8006DDE8(w06BoardSecondaryModel, -1.0f);
    BoardModelPosSet(w06BoardSecondaryModel, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(w06BoardSecondaryModel, 0, 0x40000001);
    w06StarHostModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W06, 17), w06BoardModelData, 0);
    BoardModelPosSet(w06StarHostModel, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(w06StarHostModel, 1, 0x40000001);
    w06ServiceHostModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BGUEST, 33), w06GuestModelData, 0);
    BoardModelPosSet(w06ServiceHostModel, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(w06ServiceHostModel, 1, 0x40000001);
    for (i = 0; i < MAPOBJ_MAX; i++) {
        mapObject = &w06MapObjects[i];
        if (mapObject->data_num != -1) {
            if (i == 4 || i == 5) {
                mapObjectData = w06GuestModelData;
            } else {
                mapObjectData = NULL;
            }
            w06MapObjectModelIds[i] = BoardModelCreate(mapObject->data_num, mapObjectData, 0);
            BoardModelPosSetV(w06MapObjectModelIds[i], &mapObject->pos);
            BoardModelRotSetV(w06MapObjectModelIds[i], &mapObject->rot);
            BoardModelScaleSetV(w06MapObjectModelIds[i], &mapObject->scale);
            BoardModelVisibilitySet(w06MapObjectModelIds[i], 1);
        }
    }
    BoardModelMotionStart(w06MapObjectModelIds[4], 1, 0x40000001);
    BoardModelMotionStart(w06MapObjectModelIds[5], 1, 0x40000001);
    BoardModelMotionStart(w06MapObjectModelIds[2], 0, 0);
    BoardModelMotionSpeedSet(w06MapObjectModelIds[2], 0.0f);
    BoardModelLayerSet(w06MapObjectModelIds[2], 1);
    BoardModelMotionStart(w06MapObjectModelIds[3], 0, 0);
    BoardModelMotionSpeedSet(w06MapObjectModelIds[3], 0.0f);
    BoardModelLayerSet(w06MapObjectModelIds[3], 1);
    W06ItemBoardInitialize();
    W06CoinBoardInitialize();
    W06BridgeCreate();
    W06BowserEventInitialize();
    BoardLightHookSet(W06SetBoardLighting, W06ClearBoardFog);
    BoardSpaceWalkEventFuncSet(W06HandleBridgeSpaceWalk);
    BoardSpaceWalkMiniEventFuncSet(W06HandleMiniGameSpaceWalk);
    BoardSpaceLandEventFuncSet(W06HandleSpaceLanding);
    BoardStarHostSet(w06StarHostModel);
    BoardBooHouseHostSet(w06ServiceHostModel);
    BoardLotteryHostSet(w06ServiceHostModel);
    BoardShopHostSet(w06ServiceHostModel);
    BoardJunctionMaskSet(0x3000);
    boardTurnFunc = W06BowserTurnEventCheck;
    boardStarShowNextHook = W06BowserNextStarShowHook;
    boardTurnStartFunc = W06BowserTurnStart;
    boardStarGiveHook = W06BowserStarGiveHook;
    boardBowserHook = W06BowserModelAnimationSet;
    W06StartAmbientAudio();
}

void BoardDestroy(void) {
    s32 i;

    W06StopAmbientAudio();
    boardTurnFunc
        = (void*) boardStarShowNextHook
        = (void*) boardTurnStartFunc
        = (void*) boardStarGiveHook
        = (void*) boardBowserHook = NULL;
    W06BowserEventCleanup();
    W06BridgeDestroy();
    W06ItemBoardCleanup();
    W06CoinBoardCleanup();
    for (i = 0; i < MAPOBJ_MAX; i++) {
        BoardModelKill(w06MapObjectModelIds[i]);
        w06MapObjectModelIds[i] = -1;
    }
    if (w06ServiceHostModel != -1) {
        BoardModelKill(w06ServiceHostModel);
        w06ServiceHostModel = -1;
    }
    if (w06StarHostModel != -1) {
        BoardModelKill(w06StarHostModel);
        w06StarHostModel = -1;
    }
    if (w06BoardBackgroundModel != -1) {
        BoardModelKill(w06BoardBackgroundModel);
        w06BoardBackgroundModel = -1;
    }
    if (w06BoardSecondaryModel != -1) {
        BoardModelKill(w06BoardSecondaryModel);
        w06BoardSecondaryModel = -1;
    }
}

static void W06SetBoardLighting(void) {
    s16 modelId;

    modelId = BoardModelIDGet(w06BoardBackgroundModel);
    Hu3DModelLightInfoSet(modelId, 1);
    Hu3DFogSet(4200.0f, 30000.0f, 0xF8, 0x30, 0);
}

static void W06ClearBoardFog(void) {
    Hu3DFogClear();
}

static s32 W06HandleBridgeSpaceWalk(void) {
    s32 bridgeCrossingTypes[] = { 0, 1 };
    s32 bridgeExitFlags[] = { 0x300, 0xC0 };
    BoardSpace *currentSpace;
    s16 currentSpaceIndex;
    s32 currentPlayerIndex;
    s32 i;
    s32 j;

    currentPlayerIndex = GWSystem.player_curr;
    currentSpaceIndex = GWPlayer[currentPlayerIndex].space_curr;
    if (W06BowserEventStart(currentPlayerIndex, currentSpaceIndex) != 0) {
        return 1;
    }
    for (i = 0; i < 2; i++) {
        if (BoardSpaceLinkFlagSearch(0, currentSpaceIndex, bridgeExitFlags[i]) == -1) {
            continue;
        }
        currentSpace = BoardSpaceGet(0, currentSpaceIndex);
        for (j = 0; j < currentSpace->link_cnt; j++) {
            if ((BoardSpaceFlagGet(0, currentSpace->link[j]) & 0x04000000) == 0) {
                W06BridgeCrossingStart(bridgeCrossingTypes[i], currentSpace->link[j]);
                return 1;
            }
        }
    }
    return 0;
}

static s32 W06HandleMiniGameSpaceWalk(void) {
    s16 currentSpaceIndex;
    s32 miniGameSpaceFlags;

    currentSpaceIndex = GWPlayer[GWSystem.player_curr].space_curr;
    miniGameSpaceFlags = BoardSpaceFlagGet(0, currentSpaceIndex) & 0x600000;
    if (miniGameSpaceFlags == 0x200000) {
        W06ItemSpaceEventStart();
    } else if (miniGameSpaceFlags == 0x400000) {
        W06CoinSpaceEventStart();
    }
}

static s32 W06HandleSpaceLanding(void) {
    if (BoardPlayerSizeGet(GWSystem.player_curr) != 2) {
        W06FireEventStart();
    }
}

static void W06StartAmbientAudio(void) {
    Vec cameraPosition;
    Vec cameraDirection;
    OMOBJ *audioObject;
    W06BoardAudioWork *audioWork;
    s32 i;

    audioObject = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, W06UpdateAmbientAudio);
    w06AudioListenerObject = audioObject;
    audioWork = omObjGetWork(audioObject, W06BoardAudioWork);
    audioWork->isActive = 0;
    BoardCameraPosGet(&cameraPosition);
    BoardCameraDirGet(&cameraDirection);
    HuAudFXListnerSetEX(&cameraPosition, &cameraDirection, 6600.0f, 36000.0f, 0.0f, 1650.0f, 1650.0f);
    for (i = 0; i < 10; i++) {
        audioWork->soundCuePlayed[i] = 0;
    }
    if (!_CheckFlag(FLAG_ID_MAKE(1, 16))) {
        if (w06AllAudioPaused == 0) {
            HuAudFXPauseAll(1);
            w06AllAudioPaused = 1;
        }
    } else if (w06AllAudioPaused != 0) {
        HuAudFXPauseAll(0);
        w06AllAudioPaused = 0;
    }
}

static void W06StopAmbientAudio(void) {
    if (w06AudioListenerObject) {
        omObjGetWork(w06AudioListenerObject, W06BoardAudioWork)->isActive = 1;
    }
}

static void W06UpdateAmbientAudio(OMOBJ *audioObject) {
    W06BoardAudioWork *audioWork;
    Vec cameraTarget;
    Vec cameraDirection;
    s16 backgroundMotionFrame;
    s16 playedCueCount;
    s16 i;

    audioWork = omObjGetWork(audioObject, W06BoardAudioWork);
    if (audioWork->isActive != 0 || BoardIsKill()) {
        HuAudFXListnerKill();
        w06AudioListenerObject = NULL;
        omDelObjEx(HuPrcCurrentGet(), audioObject);
        return;
    }
    backgroundMotionFrame = BoardModelMotionTimeGet(w06BoardBackgroundModel);
    BoardCameraDirGet(&cameraDirection);
    BoardCameraTargetGet(&cameraTarget);
    cameraTarget.y += 2000.0f;
    if (!_CheckFlag(FLAG_ID_MAKE(1, 16))) {
        if (w06AllAudioPaused == 0) {
            HuAudFXPauseAll(1);
            w06AllAudioPaused = 1;
        }
    } else if (w06AllAudioPaused != 0) {
        HuAudFXPauseAll(0);
        w06AllAudioPaused = 0;
    }
    if (!_CheckFlag(FLAG_ID_MAKE(1, 28))) {
        for (playedCueCount = i = 0; i < 10; i++) {
            if (audioWork->soundCuePlayed[i] != 0) {
                playedCueCount++;
            } else if (backgroundMotionFrame == w06AmbientSoundCues[i].motionFrame) {
                HuAudFXEmiterPlay(0x4A1, &w06AmbientSoundCues[i].soundPosition);
                audioWork->soundCuePlayed[i] = 1;
                goto block_20;
            }
        }
        if (playedCueCount < 10) {
            return;
        }
        for (i = 0; i < 10; i++) {
            audioWork->soundCuePlayed[i] = 0;
        }
    }
block_20:
    HuAudFXListnerUpdate(&cameraTarget, &cameraDirection);
}

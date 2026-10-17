#include "dolphin.h"
#include "REL/executor.h"
#include "game/audio.h"
#include "game/data.h"
#include "game/gamework_data.h"
#include "game/process.h"
#include "game/wipe.h"
#include "REL/w03Dll.h"
#include "game/board/lottery.h"
#include "game/board/boo_house.h"

#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/shop.h"
#include "game/board/space.h"
#include "game/board/star.h"
#include "game/object.h"
#include "game/board/player.h"
#include "game/board/map_object.h"

#include "math.h"
#include "game/hu3d.h"

//function signatures
void w03Initialize(void);
void W03ConfigureLightingAndFog(void);
void w03Destroy(void);
void W03ClearFog(void);
s32 W03HandleCurrentSpaceEvent(void);
s32 W03HandleMapObjectEvent(void);
void W03UpdateJunctionMask(void);
void W03CreateAmbientEffects(void);
void W03StopAmbientEffects(void);
void W03UpdateAmbientEffects(OMOBJ* object);
s32 StatueEventStart(void);
s32 StatueRewardTimerUpdate(s32 state);

#ifndef __MWERKS__
void CondorCreate(void);
s32 CondorCheckPlayerChoice(void);
void RiverSetWaterHeight(s32 value);
void RiverCreateRaft(void);
s16 RiverRunBoardEvent(void);
void SmokeEffectsCreate(void);
void CoinPedestalCreate(void);
void CoinPedestalDestroy(void);
void CoinEventStart(void);
void ItemEventPrepare(void);
void ItemEventNoop(void);
void ItemEventStart(void);
#endif

typedef void (*VoidFunc)(void);
extern const VoidFunc _ctors[];
extern const VoidFunc _dtors[];

//Map Objects
s16 boardModelIds[MAPOBJ_MAX];

BoardMapObject boardMapObjects[MAPOBJ_MAX] = {
	{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, DATA_MAKE_NUM(DATADIR_W03, 4)},
	{{145.0988f, 65.6173f, -2004.14f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, DATA_MAKE_NUM(DATADIR_W03, 5)},
	{{-1548.14f, -1148.76f, -1095.54f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, DATA_MAKE_NUM(DATADIR_W03, 6)},
	{{1050.0f, 50.0f, -1810.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, DATA_MAKE_NUM(DATADIR_W03, 11)},
	{{-1950.0f, 50.0f, 1790.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.914f, 1.0f}, DATA_MAKE_NUM(DATADIR_W03, 11)},
	{{1050.0f, 0.0f, -1750.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, DATA_MAKE_NUM(DATADIR_W03, 10)},
	{{-1950.0f, 0.0f, 1850.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.914f, 1.0f}, DATA_MAKE_NUM(DATADIR_W03, 10)},
	{{1950.0f, 0.0f, -1800.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, DATA_MAKE_NUM(DATADIR_W03, 16)},
	{{-3150.0f, 0.0f, -600.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, DATA_MAKE_NUM(DATADIR_W03, 17)},
	{{-3150.0f, 270.0f, -600.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, DATA_MAKE_NUM(DATADIR_W03, 18)},
	{{2080.0f, 0.0f, -1635.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, DATA_MAKE_NUM(DATADIR_BGUEST, 13)},
	{{-3000.0f, 0.0f, -500.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, DATA_MAKE_NUM(DATADIR_BGUEST, 13)},
};

s16 lightHookModel = -1;
s16 ambientLightModelId = -1;
s16 boardFeatureModelId = -1;
s16 boardPropModelId = -1;
s16 boardWaterModelId = -1;

typedef struct CameraMotionTiming {
    f32 cameraStartX;
    f32 cameraStartY;
    f32 cameraStartZ;
    f32 cameraEndX;
    f32 cameraEndY;
    f32 cameraEndZ;
    f32 cameraTargetX;
    f32 cameraTargetY;
} CameraMotionTiming;

CameraMotionTiming cameraMotionTiming = {
	0.0f, 118.0f, 120.0f, 180.0f, 180.0f, 240.0f, 240.0f, 300.0f
};

s32 boardFeatureMotionIds[] = {
    DATA_MAKE_NUM(DATADIR_W03, 0x001C),
	DATA_NUM_LISTEND
};

s32 boardPropMotionIds[] = {
    DATA_MAKE_NUM(DATADIR_BGUEST, 0x000E),
	DATA_NUM_LISTEND
};

s32 ambientSoundEmitterIds[] = {
    -1, -1, -1, -1
};

Vec ambientSoundPositions[] = {
    {3900.0f, 0.0f, 2100.0f},
    {3900.0f, 0.0f, 3600.0f},
    {2700.0f, 0.0f, 5400.0f},
    {2700.0f, 0.0f, 6300.0f},
};

s32 normalAmbientSoundIds[] = {
    0x0000044B, 0x00000439, 0x00000439, 0x0000044D
};

s32 alternateAmbientSoundIds[] = {
    0x0000044C, 0x0000043A, 0x0000043A, 0x0000044E
};

//BSS
s32 ambientAudioPaused;
OMOBJ* ambientAudioProcessObject;
W03BoardState* boardState;

void W03UpdateJunctionLinks(void);

void BoardCreate(void) {
    s32* value;
    f32 floatValue;
    f32 floatValue2;
    s32 integerValue;
    s32 integerValue2;
    BoardMapObject* value2;
    s32 i;

    boardState = (W03BoardState*)&GWSystem.board_data;
    boardState->eventActive = 0;
    BoardSpaceInit(DATA_MAKE_NUM(DATADIR_W03, 0x00));

    lightHookModel = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x01), NULL, 0);
    Fn8006DDE8(lightHookModel, -1.0f);
    BoardModelPosSet(lightHookModel, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(lightHookModel, 0, 0x40000001);

    boardWaterModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x03), NULL, 0);
    Fn8006DDE8(boardWaterModelId, -1.0f);
    BoardModelPosSet(boardWaterModelId, 0.0f, 0.0f, 0.0f);
    BoardModelLayerSet(boardWaterModelId, 2);

    ambientLightModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x02), NULL, 0);
    Fn8006DDE8(ambientLightModelId, -1.0f);
    BoardModelPosSet(ambientLightModelId, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(ambientLightModelId, 0, 0x40000001);

    boardFeatureModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x1B), boardFeatureMotionIds, 0);
    BoardModelPosSet(boardFeatureModelId, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(boardFeatureModelId, 1, 0x40000001);

    boardPropModelId = BoardModelCreate(0x2000D, boardPropMotionIds, 0);
    BoardModelPosSet(boardPropModelId, 0.0f, 0.0f, 0.0f);
    BoardModelMotionStart(boardPropModelId, 1, 0x40000001);

    for (i = 0; i < MAPOBJ_MAX; i++) {
        value2 = &boardMapObjects[i];
        if (value2->data_num != -1) {
            if ((i == 10) || (i == 11)) {
                value = boardPropMotionIds;
            } else {
                value = NULL;
            }
            boardModelIds[i]= BoardModelCreate(value2->data_num, value, 0);
            BoardModelPosSetV(boardModelIds[i], &value2->pos);
            BoardModelRotSetV(boardModelIds[i], &value2->rot);
            BoardModelScaleSetV(boardModelIds[i], &value2->scale);
            BoardModelVisibilitySet(boardModelIds[i], 1);
        }
    }

    BoardModelMotionStart(boardModelIds[10], 1, 0x40000001);
    BoardModelMotionStart(boardModelIds[11], 1, 0x40000001);
    BoardModelMotionStart(boardModelIds[7], 0, 0x40000002);
    BoardModelMotionStart(boardModelIds[9], 0, 0x40000002);
    BoardModelMotionStart(boardModelIds[1], 0, 0x40000002);

    CondorCreate();
    RiverCreateRaft();
    CoinPedestalCreate();
    ItemEventPrepare();

    BoardLightHookSet(&W03ConfigureLightingAndFog, &W03ClearFog);
    BoardSpaceWalkEventFuncSet(&W03HandleCurrentSpaceEvent);
    BoardSpaceWalkMiniEventFuncSet((void*)&W03HandleMapObjectEvent);
    BoardSpaceLandEventFuncSet((void*)&W03UpdateJunctionMask);
    BoardStarHostSet(boardFeatureModelId);
    BoardBooHouseHostSet(boardPropModelId);
    BoardLotteryHostSet(boardPropModelId);
    BoardShopHostSet(boardPropModelId);
    BoardJunctionMaskSet(0x0000C000);
    BoardJunctionMaskSet(0x3000);
    W03UpdateJunctionLinks();
    BoardModelMotionStart(boardModelIds[0], 0, 0x40000001);

    if (boardState->statueRewardCount != 0) {
        floatValue = cameraMotionTiming.cameraEndY;
        floatValue2 = cameraMotionTiming.cameraEndZ;
        RiverSetWaterHeight(0);
    } else {
        floatValue = cameraMotionTiming.cameraStartX;
        floatValue2 = cameraMotionTiming.cameraStartY;
        RiverSetWaterHeight(1);
    }

    BoardModelMotionStartEndSet(boardModelIds[0], floatValue, floatValue2);
    BoardModelMotionStart(boardModelIds[2], 0, 0x40000001);
    BoardModelMotionStartEndSet(boardModelIds[2], 1, 100);
    SmokeEffectsCreate();

    if (boardState->rewardPlayerMask != 0) {
        for (i = 0; i < 4; i++) {
            if ((boardState->rewardPlayerMask & (1 << i)) != 0) {
                BoardPlayerPreTurnHookSet(i, &StatueRewardTimerUpdate);
                break;
            }
        }
    }

    W03CreateAmbientEffects();
}

void BoardDestroy(void) {
    s32 i;

    W03StopAmbientEffects();
    ItemEventNoop();
    CoinPedestalDestroy();

    for (i = 0; i < 0x0C; i++) {
        BoardModelKill(boardModelIds[i]);
        boardModelIds[i] = -1;
    }

    if (boardPropModelId != -1) {
        BoardModelKill(boardPropModelId);
        boardPropModelId = -1;
    }

    if (boardFeatureModelId != -1) {
        BoardModelKill(boardFeatureModelId);
        boardFeatureModelId = -1;
    }

    if (lightHookModel != -1) {
        BoardModelKill(lightHookModel);
        lightHookModel = -1;
    }

    if (ambientLightModelId != -1) {
        BoardModelKill(ambientLightModelId);
        ambientLightModelId = -1;
    }
}

void W03ConfigureLightingAndFog(void) {
    s16 var = BoardModelIDGet(lightHookModel);
    Hu3DModelLightInfoSet(var, 1);
    Hu3DFogSet(5000.0f, 30000.0f, 0xE4U, 0xF0U, 0xFFU);
}

void W03ClearFog(void) {
    Hu3DFogClear();
}

s32 W03HandleCurrentSpaceEvent(void) {
    u32 playerIndex;
    s32 curPlayerIndex;
    s32 currentSpace;

    curPlayerIndex = GWSystem.player_curr;
    currentSpace = GWPlayer[curPlayerIndex].space_curr;
    playerIndex = BoardSpaceFlagGet(0, currentSpace);

    if (playerIndex & 0xC000) {
        if ((BoardPlayerSizeGet(curPlayerIndex) == 2) || ((u8) (( GWPlayer[curPlayerIndex].bowser_suit)))) {
            return 0;
        }
        if (playerIndex & 0x4000) {
            if (boardState->condorSide == 0) {
                return 0;
            }
        }
        else if (playerIndex & 0x8000 && boardState->condorSide != 0) {
            return 0;
        }

        return CondorCheckPlayerChoice();
    }

    if (playerIndex & 0x200) {
        if ((BoardPlayerSizeGet(curPlayerIndex) == 2) || ((u8) ((GWPlayer[curPlayerIndex].bowser_suit)) != 0)) {
            return 0;
        }
        return RiverRunBoardEvent();
    }

    return 0;
}

s32 W03HandleMapObjectEvent(void) {
    s16 currentSpace = GWPlayer[GWSystem.player_curr].space_curr;
    u32 var = BoardSpaceFlagGet(0, currentSpace) & 0x600000;

    if (var + -0x200000 == 0) {
        CoinEventStart();
        return;
    }
    if (var + -0x400000 == 0) {
        ItemEventStart();
    }
}

void W03UpdateJunctionMask(void) {
    if ((BoardPlayerSizeGet(GWSystem.player_curr) == 0) && (StatueEventStart() != 0) && (boardState->rewardPlayerMask != 0)) {
        BoardJunctionMaskSet(0xC00);
    }
}
void W03SetBridgeMotion(s32 value) {
    f32 floatValue;
    f32 floatValue2;
    f32 floatValue3;
    f32 floatValue4;
    f32 animationFrame;

    if (value != 0) {
        floatValue2 = cameraMotionTiming.cameraStartZ;
        floatValue3 = cameraMotionTiming.cameraEndX;
        floatValue = cameraMotionTiming.cameraEndY;
        floatValue4 = cameraMotionTiming.cameraEndZ;
        RiverSetWaterHeight(0);
    } else {
        floatValue2 = cameraMotionTiming.cameraTargetX;
        floatValue3 = cameraMotionTiming.cameraTargetY;
        floatValue = cameraMotionTiming.cameraStartX;
        floatValue4 = cameraMotionTiming.cameraStartY;
        RiverSetWaterHeight(1);
    }

    BoardModelMotionStart(boardModelIds[0], 0, 0);
    BoardModelMotionTimeSet(boardModelIds[0], floatValue2);

    while (1) {
        animationFrame = BoardModelMotionTimeGet(boardModelIds[0]);
        if (animationFrame >= floatValue3) {
            break;
        }
        HuPrcVSleep();
    }

    BoardModelAttrSet(boardModelIds[0], 0x40000001);
    BoardModelMotionTimeSet(boardModelIds[0], floatValue);
    BoardModelMotionStartEndSet(boardModelIds[0], floatValue, floatValue4);
}

void W03UpdateJunctionLinks(void) {
    s16 var;
    s32 i, j;
    BoardSpace* value;
    BoardSpace* value2;

    for (i = 0; i < BoardSpaceCountGet(0); i++) {
        var = i + 1;
        value2 = BoardSpaceGet(0, var);
        for (j = 0; j < value2->link_cnt; j++) {
            if (BoardSpaceFlagGet(0, value2->link[j]) & 0xC00) {
                value = BoardSpaceGet(0, value2->link[j]);
                if (boardState->statueRewardCount != 0) {
                    value->flag |= 0x04000000;
                    BoardJunctionMaskSet(0xC00);
                } else {
                    value->flag &= ~0x04000000;
                    BoardJunctionMaskReset(0xC00);
                }
            }
        }
    }
}

void W03CreateAmbientEffects(void) {
    Vec position;
    Vec position2;
    Vec position3;
    OMOBJ* object;
    s32 integerValue;
    s32 integerValue2;
    AmbientAudioProcessWork* work;
    s32 i;

    object = omAddObjEx(boardObjMan, 0x101, 0, 0, -1, &W03UpdateAmbientEffects);
    ambientAudioProcessObject = object;
    work = omObjGetWork(object, AmbientAudioProcessWork);
    work->shouldStop = 0;
    ambientAudioPaused = 0;
    BoardCameraPosGet(&position);
    BoardCameraDirGet(&position2);
    HuAudFXListnerSetEX(&position, &position2, 4800.0f, 36000.0f, 0.0f, 1200.0f, 1200.0f);

    for (i = 0; i < ARRAY_COUNT(ambientSoundPositions); i++) {
        position3.x = ambientSoundPositions[i].x - 3600.0f;
        position3.y = ambientSoundPositions[i].y;
        position3.z = ambientSoundPositions[i].z - 3600.0f;
        if (boardState->rewardPlayerMask != 0) {
            ambientSoundEmitterIds[i] = HuAudFXEmiterPlay(alternateAmbientSoundIds[i], &position3);
        } else {
            ambientSoundEmitterIds[i] = HuAudFXEmiterPlay(normalAmbientSoundIds[i], &position3);
        }
    }

    for (i = 0; i < 4; i++) {
        if ((WipeStatGet() != 0) || (_CheckFlag(FLAG_ID_MAKE(1, 28)) != 0)) {
            HuAudFXVolSet(ambientSoundEmitterIds[i], 0);
        } else {
            HuAudFXVolSet(ambientSoundEmitterIds[i], 0x7F);
        }
    }

    if (_CheckFlag(FLAG_ID_MAKE(1, 16)) == 0) {
        if (ambientAudioPaused == 0) {
            HuAudFXPauseAll(1);
            ambientAudioPaused = 1;
        }
    } else if (ambientAudioPaused != 0) {
        HuAudFXPauseAll(0);
        ambientAudioPaused = 0;
    }
}

void W03StopAmbientEffects(void) {
    if (ambientAudioProcessObject) {
        omObjGetWork(ambientAudioProcessObject, AmbientAudioProcessWork)->shouldStop = 1;
    }
}

void W03UpdateAmbientEffects(OMOBJ* object) {
    Vec position;
    Vec position2;
    s32 i;
    AmbientAudioProcessWork* work;

    work = omObjGetWork(object, AmbientAudioProcessWork);

    if (work->shouldStop != 0 || (BoardIsKill() != 0)) {
        for (i = 0; i < ARRAY_COUNT(ambientSoundEmitterIds); i++) {
            if (ambientSoundEmitterIds[i] != -1) {
                HuAudFXFadeOut(ambientSoundEmitterIds[i], 100);
            }
        }
        HuAudFXListnerKill();
        ambientAudioProcessObject = 0;
        omDelObjEx(HuPrcCurrentGet(), object);
        return;
    }

    BoardCameraDirGet(&position2);
    BoardCameraTargetGet(&position);

    for (i = 0; i < 4; i++) {
        if ((WipeStatGet() != 0) || (_CheckFlag(0x1001C) != 0)) {
            HuAudFXVolSet(ambientSoundEmitterIds[i], 0);
        } else {
            HuAudFXVolSet(ambientSoundEmitterIds[i], 0x7F);
        }
    }

    position.y += 2000.0f;

    if (_CheckFlag(0x10010) == 0) {
        if (ambientAudioPaused == 0) {
            HuAudFXPauseAll(1);
            ambientAudioPaused = 1;
        }
    } else if (ambientAudioPaused != 0) {
        HuAudFXPauseAll(0);
        ambientAudioPaused = 0;
    }

    HuAudFXListnerUpdate(&position, &position2);
}

#ifndef REL_W03DLL_H
#define REL_W03DLL_H

#include "game/gamework_data.h"
#include "dolphin.h"
#include "REL/executor.h"
#include "game/process.h"
#include "game/window.h"
#include "game/object.h"

enum {
    MAPOBJ_MAX = 12
};

typedef struct AmbientAudioProcessWork{
    u8 shouldStop : 1;
} AmbientAudioProcessWork;

typedef struct W03BoardState {
           s8 statueRewardCount;
           u8 condorSide;
           u8 rewardPlayerMask;
           s8 eventActive;
           u16 reserved04;
           u16 reserved06;
           s16 reserved08;
} W03BoardState;

extern W03BoardState* boardState;
extern s16 boardModelIds[MAPOBJ_MAX];

#endif

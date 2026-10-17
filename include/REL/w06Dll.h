#ifndef REL_W06DLL_H
#define REL_W06DLL_H

#include "dolphin/types.h"

enum {
    MAPOBJ_MAX = 6
};

typedef struct {
    struct {
        u8 isJunctionPathOpen : 1;
        u8 isBoardEventEnabled : 1;
        u8 hasCustomStarSpace : 1;
        u8 isStarSpaceOccupied : 1;
        u8 isBowserEventEnabled : 1;
    };
    s8 boardEventState;
    s8 boardEventCounter;
    s8 reservedBoardState;
    s8 starSpaceIndex;
    u8 bowserTargetPlayers;
    s16 boardEventTimer;
} W06BoardData;

extern void W06ItemBoardInitialize(void);
extern void W06ItemBoardCleanup(void);
extern void W06ItemSpaceEventStart(void);

extern void W06CoinBoardInitialize(void);
extern void W06CoinBoardCleanup(void);
extern void W06CoinSpaceEventStart(void);

extern void W06FireEventStart(void);

extern void W06BridgeCreate(void);
extern void W06BridgeDestroy(void);
extern void W06BridgeStateUpdate(void);
extern void W06BridgeCrossingStart(s32 crossingType, s16 spaceIndex);

extern void W06BowserEventInitialize(void);
extern void W06BowserEventCleanup(void);
extern void W06BowserTurnStart(s32 playerIndex, s32 spaceIndex);
extern s32 W06BowserEventStart(s32 playerIndex, s32 spaceIndex);
extern void W06BowserModelAnimationSet(s32 showBowser);
extern void W06BowserTurnEventCheck(void);
extern void W06BowserStarGiveHook(void);
extern void W06BowserNextStarShowHook(void);

extern s16 w06MapObjectModelIds[MAPOBJ_MAX];
extern W06BoardData *w06BoardData;

#endif

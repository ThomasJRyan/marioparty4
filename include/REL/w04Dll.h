#ifndef REL_W04DLL_H
#define REL_W04DLL_H

#include "game/board/map_object.h"

#include "dolphin/types.h"

enum {
    MAPOBJ_MAX = 16
};

typedef struct {
    s8 bridgeState;
    u8 booEncounterCount;
} W04BoardEventState;

extern s32 W04ModelRotateYToAngle(s16 modelId, float targetAngle, float rotationStep);

extern void W04BridgeSetup(void);
extern void W04BridgeCleanup(void);
extern void W04BridgeToggleEvent(void);
extern void W04BridgeWalkEvent(void);

extern void W04BooEvent(void);

extern void W04BigBooSetup(void);
extern void W04BigBooEvent(void);

extern void W04ItemSetup(void);
extern void W04ItemAcquisitionEvent(void);
extern void W04ItemModelsCleanup(void);
extern void W04ItemModelsSpawn(s16 itemIds[3]);
extern void W04ItemModelsShrink(void);

extern void W04CoinSetup(void);
extern void W04CoinGameEvent(void);

extern s16 w04BoardModelIds[MAPOBJ_MAX];
extern BoardMapObject w04BoardMapObjects[MAPOBJ_MAX];
extern W04BoardEventState *w04BoardEventState;

#endif

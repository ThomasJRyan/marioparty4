#ifndef REL_W02DLL_H
#define REL_W02DLL_H

#include "dolphin.h"
#include "game/board/main.h"
#include "game/board/map_object.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/window.h"
#include "game/gamework_data.h"
#include "game/process.h"
#include "math.h"

/* STRUCTS*/
typedef struct W02GenDice {
    /* 0x00 */ s16 dieCount;
    /* 0x02 */ s16 rollFlags;
    /* 0x04 */ s32 dieModelData;
    /* 0x08 */ s32 *dieFaceData;          // ? Holds the objects for the dice faces
    /* 0x0C */ Vec spawnPosition;           // ? Position of the dice
    /* 0x18 */ s16 minimumRoll;           // ? Holds the minimum value
    /* 0x1A */ s16 maximumRoll;           // ? Holds the maximum value
    /* 0x1C */ u16 dieResults[3];        // ? Holds the values of up to 3 dice
    /* 0x22 */ s16 displayRollTotal;
    /* 0x24 */ float dieVerticalSpacing;
    /* 0x28 */ s16 rollComplete;
    /* 0x2A */ u8 reserved2A[0x2];
    /* 0x2C */ HUPROCESS *rollProcess;
    /* 0x30 */ s16 dieModelIds[3];
    /* 0x36 */ s16 dieFaceResults[3];
    /* 0x3C */ OMOBJ *dieObjects[3];
    /* 0x48 */ s16 dieParticleIds[3];
    /* 0x4E */ u8 reserved4E[0x2];
    /* 0x50 */ OMOBJ *resultNumberObjects[3];
    /* 0x5C */ u8 reserved5C[0x08];
    /* 0x64 */ Vec resultScreenPositions[2];
    /* 0x7C */ Vec singleDieResultPosition;
    /* 0x88 */ s32 dieSoundIds[3];
    /* 0x94 */ s16 totalRollResult;
    /* 0x96 */ s16 activeDieIndex;
    /* 0x98 */ float dieMotionStartFrame;
    /* 0x9C */ s16 dieAnimationComplete;
} W02GenDice;

extern s16 rouletteGambleMarkerModelId;
extern s16 shuffleboardGambleMarkerModelId;
extern Vec shuffleboardNorthModelPosition;
extern Vec shuffleboardSouthModelPosition;
extern s16 boardModelIds[0x10]; // Model List
extern s16 goombaHostModelId;

// main.c
extern void BoardAmbientLightDisable();
extern void BoardAmbientLightEnable();
extern s32 BoardSpaceCounterDecrement();
extern s32 BoardSpaceEventExecute();
extern void BoardMinigameAssetsCreate();
extern s32 BoardMinigameEventExecute();
extern void W02MesExec(u32);

void GenDiceCreate(W02GenDice *diceWork);
void GenDiceDestroy(W02GenDice *diceWork);
s32 GenDiceAnimationCheck(W02GenDice *diceWork);

// roulette.c
extern void RouletteBribePayout();
extern s32 RouletteComputerChoice();
extern void RouletteModelsAnimate();
// unorganized
extern void GambleMain(s32 gambleSpaceIndex);
extern void CoinMinigameStart();
extern void CoinMinigameModelsInitialize();
extern void ItemMinigameModelsInitialize();
extern void ItemMinigameStart();
extern void ItemMinigamePlayerDestroy();
extern void ItemMinigameItemsRandomize(s16 *itemChoices);
extern void ItemMinigameItemsDrop(u16 waitForAnimation);
extern s32 ShuffleboardModelsPosition();
extern void ShuffleboardEventStart(u32 directionFlag);
extern void RouletteModelsInitialize();
extern s32 RouletteBribeChoice();
extern s32 RouletteSpin(s32 bribeProbability);
extern void RoulettePlayerMove(s32 targetSector);
void RouletteEventStart(void);

#endif // W02DLL

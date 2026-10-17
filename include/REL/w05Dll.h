#ifndef REL_W05DLL_H
#define REL_W05DLL_H

#include "dolphin/types.h"

#include "game/board/main.h"
#include "game/board/map_object.h"
#include "game/board/player.h"

// structs
typedef struct HotelBoardState {
    u32 coinsInvested;
    s8 hotelLevel;
} HotelBoardState;

typedef struct ItemMinigameState {
    s16 remainingRounds;
    s16 roundTimer;
    s16 sequenceId;
    s16 messageWindowId;
    s16 selectedChoice;
    s16 choiceToItem[3];
    s16 cpuInputDelay;
    s16 cpuInputCountdown;
} ItemMinigameState;

typedef struct CoinMinigameState {
    s16 sequenceId;
    s16 inputDelay;
    s16 roundTimer;
    s16 messageWindowId;
    s16 selectedFishIndex;
    s16 fishModelId;
    Vec fishPositions[3];
    s16 fishInputDelay;
    s16 fishMissCount;
} CoinMinigameState;

// functions
void BoardCreate(void);
void BoardDestroy(void);
void w05SetBoardModelLighting(void);
void w05BoardLightingNoop(void);
s32 w05HandleBoardLandingEvent(void);
s32 w05HandleBoardWalkEvent(void);
void w05InitializeBoardMinigames(void);
void w05BoardEventNoop(void);
void w05RunItemSpaceEvent(void);
void w05CleanupItemSpaceEvent(void);
void w05StartItemSpaceEvent(void);
void w05StartBoardMusicAfterDelay(void);
void w05RunCoinSpaceEvent(void);
s32 w05HandleBoardMiniEventSpace(void);
s32 w05TurnBoardModelTowardAngle(s16, float, float);

void w05HotelCreateLevelModel(void);
void w05HotelDestroyLevelModel(void);
s16 w05HotelGetLevelForCoinCount(void);
void w05HotelCollectToll(void);
void w05HotelUpgrade(void);

void w05CreateMonkeyModels(void);
void w05RunMonkeyBoardEvent(s32);
void w05AnimateMonkeyDrop(void);
void w05SetMonkeyPlayerAnimation(void);
void w05AnimateMonkeyPlayerDrop(void);
void w05MoveBowserPlayerToNextSpace(void);
void w05UpdateMonkeyEffects(void);

void w05PauseMusicAfterDelay(void);
void w05FadeOutMusicAfterDelay(void);
void w05RunDolphinBoardEvent(s32);
void w05DolphinMovePlayerToRewardSpace(s32, s32, Vec *);
void w05DolphinEventCleanup(void);
void w05DolphinMovePlayerToItemSpace(s32, s32, Vec *);
void w05DolphinDestroyEffects(void);
OMOBJ *w05DolphinSpawnEffect(u32, Vec *);
void w05DolphinEffectUpdate(OMOBJ *);
void w05DolphinFinishPlayerMovement(s32, Vec *);
void w05DolphinInitializeModels(void);
void w05DolphinCreateEffect(void);
void w05DolphinDestroyEffects(void);
OMOBJ *w05DolphinSpawnEffect(u32, Vec *);
void w05DolphinEffectUpdate(OMOBJ *);

void w05InitializeItemMinigame(void);
void w05StartItemMinigame(void);
void w05PollItemMinigameEnd(OMOBJ *);
void w05ExitItemMinigame(void);
void w05CleanupItemMinigame(void);
void w05FinishItemMinigameCleanup(void);
void w05ShuffleItemChoices(s16[3]);
void w05PlayItemSpaceExitAnimation(void);
void w05PollItemMinigameSequence(OMOBJ *);
void w05PollItemInput(OMOBJ *);
void w05RunItemMinigameRound(void);
void w05ExitItemMinigameFromObject(OMOBJ *);
void w05UpdateItemMinigameState(ItemMinigameState *);
void w05UpdateItemTargets(OMOBJ *);
void w05UpdateItemMovement(OMOBJ *);
void w05UpdateItemSelection(OMOBJ *);
void w05MoveItemModels(OMOBJ *);
void w05UpdateItemMinigameResult(OMOBJ *);
void w05GetItemMinigameResult(ItemMinigameState *, s32 *, s32 *, s32 *);
void *w05GetItemMinigameInput(void);
void w05SetItemMinigameInputState(void);
void w05RunItemMinigameIntro(void);
void w05DestroyItemMinigameModels(void);
void w05UpdateItemParticle(HU3DMODEL *, HU3DPARTICLE *, float[3][4]);

void w05PrepareCoinBoardModel(void);
void w05AnimateCoinBoardModelIntro(void);
void w05AnimateCoinBoardModelOutro(void);
void w05StartCoinMinigame(void);
void w05PollCoinMinigameEnd(OMOBJ *);
void w05ExitCoinMinigame(void);
void w05CleanupCoinMinigame(void);
void w05FinishCoinMinigame(void);
void w05PollCoinMinigameSequence(OMOBJ *);
void w05UpdateCoinModelPosition(CoinMinigameState *);
void w05UpdateCoinMinigameObject(OMOBJ *);
void w05UpdateCoinPickup(OMOBJ *);
void w05UpdateCoinMovement(OMOBJ *);
void w05UpdateCoinAnimation(OMOBJ *);
void w05UpdateCoinEffect(OMOBJ *);
void w05UpdateCoinMinigamePlayer(OMOBJ *);
void w05RunCoinMinigame(void);
void w05UpdateCoinMinigameTimer(OMOBJ *);
void w05UpdateCoinResult(OMOBJ *, u16 *, s16 *);
void w05FinishCoinMinigame(OMOBJ *);

// data
extern BoardMapObject boardMapObjects[7];
extern s16 mainBoardModel;
extern s16 secondaryBoardModel;
extern s16 starHostModel;
extern s16 eventHostModel;
extern s16 upperBoardModel;
extern s32 characterIdleMotions[8];
extern s32 starModelMotionList[2];
extern s32 boardModelMotionListEnd;

extern s32 hotelLevelModels[5];
extern s16 hotelModel;
extern u32 hotelCoinThresholds[4];
extern s32 hotelMotion;
extern s16 hotelMotionId;
extern s32 hotelUpgradeMotions[4];
extern s32 hotelGuestMotions[2];

extern s32 monkeyModelList[3];
extern char *monkeyHookNames[2];
extern s32 monkeyPlayerMotions[3];

extern char *dolphinObjectNames[6];
extern s32 dolphinEffectModels[6];
extern Vec dolphinRotations[6];
extern Vec dolphinPositionOffsets[6];
extern s32 dolphinPlayerMotions[2];
extern s32 dolphinEffectParameters[6];

extern s32 itemPlayerMotions[2];
extern char *itemObjectNames[3];
extern s32 itemMinigameModels[14];
extern s32 itemMinigameModels[14];
extern s16 itemChoiceSequence[92];
extern float itemChoiceRotations[3];

extern char *coinFishObjectNames[3];
extern s32 coinPlayerMotionIds[3];
extern s32 coinFishModels[3];
extern char *coinFishHookNames[3];
extern s16 coinRewardValues[3];
extern s32 coinRewardMessages[3];

// bss
extern s16 itemPlayerMotionIds[3];
extern s16 coinEffectModel;
extern s16 coinFishModel;
extern s16 coinDropModels[20];
extern s16 coinTokenModel;
extern OMOBJ *coinPlayerObject;
extern OMOBJ *coinMinigameObject;
extern OMOBJ *coinControllerObject;
extern s16 coinRewardCount;
extern s16 coinMinigameExitRequested;
extern s16 coinFishChoice;
extern s16 coinPlayerIndex;
extern u32 coinFishObjectType;
extern s32 coinFishIndex;

extern s16 itemPlayerIndex;
extern s16 itemModels[3];
extern s16 itemModelMotions[3];
extern s16 itemMotionModels[3];
extern s16 itemEffectModel;
extern s16 itemPlayerMotionIds[2];
extern s16 itemMinigameExitRequested;
extern HUPROCESS *itemMinigameProcess;
extern OMOBJ *itemControllerObject;
extern OMOBJ *itemMinigameObject;
extern OMOBJ *itemInputObject;
extern s32 itemMinigameState;
extern s32 itemParticleModels[3];
extern ANIMDATA *itemAnimationData;

extern s16 dolphinPlayerMotionIds[2];
extern s16 dolphinEffectModelIds[3];
extern s32 dolphinPlayerIndex;
extern s16 dolphinBoardModel;
extern s16 dolphinSpaceList[6];

extern s16 monkeyModels[2];
extern s16 monkeyPlayerIndex;
extern s16 monkeyBoardModel;
extern s16 monkeySide;
extern s16 monkeyEffectModels[2];
extern Vec monkeyLandingPosition;
extern s16 monkeyPlayerMotionIds[4];

extern s16 hotelPlayerIndex;

extern s16 boardMapModelIds[7];
extern s32 *boardMotionList;
extern s16 itemChoiceIds[4];
extern HUPROCESS *boardEventProcess;
extern HotelBoardState *hotelBoardState;

#endif

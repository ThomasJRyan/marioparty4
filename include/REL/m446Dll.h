#ifndef REL_M446DLL_H
#define REL_M446DLL_H

#include "dolphin/types.h"

#include "game/object.h"
#include "game/process.h"


////// TYPES //////
typedef struct _M446FlowWork {
    s32 flowState;
    s16 resultSequenceId;
    s16 messageSequenceId;
    char reservedPad[0x4];
    s32 countdownFrames;
    s32 stateStep;
    s32 stateTimer;
    s32 audioSequenceId;
    s32 currentPlayerIndex;
    struct _M446TableWork *tableWork;
    struct _M446StageWork *stageWork;
    struct _M446DeckWork *deckWork;
    struct _M446PlayerWork *playerWorks[2];
    struct _M446CardWork *selectedCard;
    struct _M446CardWork *cardBuffer[23];
    s32 cardBufferCount;
    HUPROCESS *flowProcess;
} M446FlowWork;

typedef struct _M446PointerList {
    void **items;
    s32 capacity;
    s32 count;
} M446PointerList;

typedef struct _M446ListNode {
    void *value;
    struct _M446ListNode *previous;
    struct _M446ListNode *next;
} M446ListNode;

typedef struct _M446CardWork {
    s32 cardId;
    s32 animationState;
    u8 animationSubstate;
    u8 modelFlags;
    s16 modelIds[2];
    s16 modelIndex;
    Vec position;
    Vec positionStart;
    Vec positionTarget;
    Vec rotation;
    Vec rotationStart;
    Vec rotationTarget;
    float scale;
    float scaleStart;
    float scaleTarget;
    s32 faceDown;
    s32 selectionState;
    s32 cardFaceVisible;
    s32 positionAnimating;
    s32 rotationAnimating;
    s32 scaleAnimating;
    s32 positionDelay;
    float positionProgress;
    float positionStep;
    float rotationProgress;
    float rotationStep;
    float scaleProgress;
    float scaleStep;
} M446CardWork;

typedef struct _M446DeckWork {
    M446PointerList *cards;
    Vec cardPosition;
} M446DeckWork;

typedef struct _M446TableWork {
    struct _M446CursorWork *cursorWork;
    s16 modelId;
    s16 reservedModelIdPadding;
    M446CardWork *cards[21];
    s32 cardCount;
    s32 selectedCardIndex;
    s32 automaticCardIndex;
    s32 selectionDelay;
    s32 automaticSelectionActive;
} M446TableWork;

typedef struct _M446CursorWork {
    struct {
        s8 cursorShown : 1;
        s8 movementActive : 1;
        s8 reverseDirection : 1;
        s8 selectionAnimationActive : 1;
    };
    s32 cursorState;
    s32 playerIndex;
    s16 modelIds[9];
    Vec position;
    Vec positionStart;
    Vec positionTarget;
    float scale;
    u8 controllerPort;
    float movementProgress;
    float movementStep;
} M446CursorWork;

typedef struct _M446PlayerWork {
    s32 playerIndex;
    M446TableWork *tableWork;
    M446CursorWork *cursorWork;
    s32 handCardCount;
    s32 selectedCardIndex;
    s32 actionTimer;
    s32 actionState;
    s32 actionSubstate;
    s32 cardMovementState;
    s32 cardFlippedState;
    s32 targetCardId;
    M446CardWork *handCards[3];
    s32 handCount;
    s16 modelIds[2];
    s16 motionIds[4];
    Vec position;
    Vec rotation;
    float modelScale;
    u8 characterId;
    u8 difficulty;
    u8 controllerPort;
    u8 playerFlags;
    s32 isComputer;
    s32 selectionActive;
    s32 usesCustomModel;
    s32 selectedHandIndex;
    char reservedPad[0x4];
    s32 selectionDelay;
    Vec positionStart;
    Vec positionTarget;
    s32 positionAnimating;
    Vec rotationStart;
    Vec rotationTarget;
    s32 rotationAnimating;
    float positionProgress;
    float positionStep;
    float rotationProgress;
    float rotationStep;
    s32 animationId;
} M446PlayerWork;

typedef struct _M446StageWork {
    s16 modelIds[5];
    s16 lightIds[2];
    s16 extraLightId;
    float playerLightIntensity;
    float stageLightIntensity;
    s32 stageMotionState;
    s32 leftMotionState;
    s32 rightMotionState;
    s32 stageActive;
    s32 stageLightsActive;
    s32 reserved;
} M446StageWork;

typedef struct _M446StageVertex {
    Vec position;
    Vec direction;
    GXColor color;
} M446StageVertex;

typedef void (*M446RandomValueCallback)(s32);
typedef void (*M446FlowStateCallback)(M446FlowWork *);
typedef void (*M446CardActionCallback)(M446CardWork *);
typedef void (*M446PlayerActionCallback)(M446PlayerWork *);

////// PROTOTYPES //////
// main.c
void ObjectSetup(void);
void m446ObjectManagerUpdate(OMOBJ *objectEntry);
M446FlowWork *m446FlowCreate(void);
void m446FlowDestroy(M446FlowWork *flowWork);
void m446FlowStateUpdate(M446FlowWork *flowWork);
void m446FlowInitialize(M446FlowWork *flowWork);
void m446FlowPlay(M446FlowWork *flowWork);
void m446FlowResolve(M446FlowWork *flowWork);
void m446FlowFinishTurn(M446FlowWork *flowWork);
void m446FlowFinishGame(M446FlowWork *flowWork);
s32 m446FlowPlayerCountGet(M446FlowWork *flowWork);
void m446FlowResultCreate(M446FlowWork *flowWork);
void m446FlowResultDestroy(M446FlowWork *flowWork);
void m446FlowResultUpdate(M446FlowWork *flowWork);
void m446FlowProcess(void);
// card.c
M446PointerList *m446PointerListCreate(s32 listCapacity);
void m446PointerListDestroy(M446PointerList *pointerList);
s32 m446PointerListAppend(M446PointerList *pointerList, void *value);
s32 m446PointerListPop(M446PointerList *pointerList, void **valueOut);
s32 m446PointerListPopValue(M446PointerList *pointerList, void **valueOut);
BOOL m446PointerListIsEmpty(M446PointerList *pointerList);
s32 m446PointerListCount(M446PointerList *pointerList);
M446ListNode *m446ListNodeCreate(void *value);
void m446ListNodeDestroy(M446ListNode *listNode);
void m446ListDestroy(M446ListNode **listHead);
void m446ListAppend(M446ListNode *listNode, M446ListNode *listNode2);
void m446ListRemove(M446ListNode *listNode);
void m446BytesCopy(u8 *destination, u8 *source, s32 byteCount);
void m446BytesFill(u8 *destination, s8 fillValue, s32 byteCount);
void m446BytesClear(u8 *destination, s32 byteCount);
s32 m446SignedBytesCompare(s8 *leftBuffer, s8 *rightBuffer, s32 byteCount);
s32 m446CardInitialize(void);
void m446CardsDestroyAll(void);
void m446CardsUpdate(void);
M446CardWork *m446CardCreate(s32 cardId);
void m446CardDestroy(M446CardWork *cardWork);
void m446CardFlipDown(M446CardWork *cardWork);
void m446CardFlipUp(M446CardWork *cardWork);
void m446CardMoveTo(M446CardWork *cardWork, Vec *targetPosition, s32 durationFrames);
void m446CardMoveToDelayed(M446CardWork *cardWork, Vec *targetPosition, s32 durationFrames, s32 delayFrames);
void m446CardRotateTo(M446CardWork *cardWork, Vec *targetRotation, s32 durationFrames);
void m446CardScaleTo(M446CardWork *cardWork, f32 targetScale, s32 durationFrames);
void m446CardFaceShow(M446CardWork *cardWork);
void m446CardFaceHide(M446CardWork *cardWork);
void m446CardTurnBegin(M446CardWork *cardWork);
void m446CardTurnUpdate(M446CardWork *cardWork);
void m446CardShuffleBegin(M446CardWork *cardWork);
void m446CardShuffleUpdate(M446CardWork *cardWork);
void m446CardShadowSet(M446CardWork *cardWork, s32 shadowEnabled);
void m446CardRenderUpdate(M446CardWork *cardWork);
// deck.c
M446DeckWork *m446DeckCreate(void);
void m446DeckDestroy(M446PointerList** cardList);
void m446DeckFanOut(M446PointerList** cardList, M446CardWork** cardArray, s32* cardCountOut, s32 animateCards);
void m446DeckDealCards(M446DeckWork* deckWork, M446CardWork** cardsOut, s32 cardPoolSize, s32 dealMode);
s32 m446DeckPushCard(M446DeckWork* deckWork, M446CardWork* cardWork, s32 placementMode);
s32 m446DeckPopCard(M446PointerList** cardList, M446CardWork** cardOut);
void m446DeckRefresh(M446PointerList** cardList);
void m446RandomSeedSet(u32 randomSeed);
s32 m446RandomGet(void);
// table.c
s32 m446TableInitialize(void);
void m446TablesUpdate(void);
void m446TablesSync(void);
M446TableWork *m446TableCreate(void);
void m446TableDestroy(M446TableWork *tableWork);
s32 m446TableAddCard(M446TableWork *tableWork, M446CardWork *cardWork, s32 cardPlacementMode);
s32 m446TableRemoveCard(M446TableWork *tableWork, M446CardWork *cardWork, s32 cardRemovalMode);
s32 m446TableCardGet(M446TableWork *tableWork, s32 cardIndex, M446CardWork **cardOut);
void m446TableSelectionReset(M446TableWork *tableWork, s32 reverseOrder);
s32 m446TableSelectionUpdate(M446TableWork *tableWork, u8 playerIndex, u8 controllerPort, s32 cursorMode, M446CardWork **cardOut, s32 selectionMode);
void m446TableReserveSpaces(M446TableWork *tableWork);
void m446TableClearReservedSpaces(M446TableWork *tableWork);
s32 m446TableCardCountByOwner(M446TableWork *tableWork, s32 ownerPlayerIndex);
void m446TableRevealOwnerCards(M446TableWork *tableWork);
// player.c
s32 m446PlayersInitialize(void);
void m446PlayersDestroy(void);
void m446PlayersUpdate(void);
M446PlayerWork *m446PlayerCreate(s32 configuredPlayerId, M446TableWork *tableWork);
void m446PlayerDestroy(M446PlayerWork *playerWork);
s32 m446PlayerAppendCard(M446PlayerWork *playerWork, M446CardWork *cardWork, s32 cardPlacementMode);
s32 m446PlayerRemoveCard(M446PlayerWork *playerWork, M446CardWork *cardWork, s32 cardRemovalMode);
s32 m446PlayerCardFind(M446PlayerWork *playerWork, s32 cardIndex, M446CardWork **cardOut);
s32 m446PlayerCardSelect(M446PlayerWork *playerWork, M446CardWork **cardOut, s32 selectionMode);
void m446PlayerIdleBegin(M446PlayerWork *playerWork);
void m446PlayerIdleUpdate(M446PlayerWork *playerWork);
void m446PlayerLookBegin(M446PlayerWork *playerWork);
void m446PlayerLookUpdate(M446PlayerWork *playerWork);
void m446PlayerMoveBegin(M446PlayerWork *playerWork);
void m446PlayerMoveUpdate(M446PlayerWork *playerWork);
void m446PlayerEmotionSet(M446PlayerWork *playerWork, s32 emotionId);
void m446PlayerCardReaction(M446PlayerWork *playerWork);
void m446PlayerPositionMove(M446PlayerWork *playerWork, Vec *targetPosition, s32 durationFrames);
void m446PlayerRotationMove(M446PlayerWork *playerWork, Vec *targetRotation, s32 durationFrames);
void m446PlayerAnimationSet(M446PlayerWork *playerWork, s32 animationId);
void m446PlayerMotionSet(M446PlayerWork *playerWork, s32 motionId, s32 vibrationStrength);
void m446PlayerRenderUpdate(M446PlayerWork *playerWork);
void m446PlayerShadowUpdate(M446PlayerWork *playerWork);
// camera.c
s32 m446CameraInitialize(void);
void m446CameraTransitionStop(void);
void m446CameraUpdate(void);
void m446CameraTargetSet(Vec* lookAtPosition);
void m446CameraRotationSet(Vec* cameraAngles);
void m446CameraDistanceSet(f32 cameraDistance);
void m446CameraTransitionBegin(Vec* targetPosition, Vec* targetAngles, f32* targetDistance, s32 durationFrames);
s32 m446CameraTransitionIsActive(void);
void m446CameraTransitionReset(void);
// cursor.c
s32 m446CursorInitialize(void);
void m446CursorUpdate(void);
void m446CursorsUpdate(void);
M446CursorWork *m446CursorCreate(s32 initialPlayerIndex);
void m446CursorDestroy(M446CursorWork *cursorWork);
void m446CursorPlayerSet(M446CursorWork *cursorWork, u8 cursorPlayer);
void m446CursorActivate(M446CursorWork *cursorWork);
void m446CursorMoveTo(M446CursorWork *cursorWork, Vec *targetPosition, s32 durationFrames);
void m446CursorUpdatePosition(M446CursorWork *cursorWork);
void m446CursorSelect(M446CursorWork *cursorWork);
void m446CursorDeselect(M446CursorWork *cursorWork);
// stage.c
s32 m446StageInitialize(void);
void m446StageUpdate(void);
void m446StagesUpdate(void);
M446StageWork *m446StageCreate(void);
void m446StageDestroy(M446StageWork *stageWork);
void m446StageStart(M446StageWork *stageWork);
void m446StageUpdateState(M446StageWork *stageWork);
void m446StageRenderUpdate(M446StageWork *stageWork);

#endif

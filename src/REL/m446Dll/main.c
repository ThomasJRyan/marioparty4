#include "REL/m446Dll.h"
#include "math.h"

#include "game/audio.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/window.h"
#include "game/wipe.h"
#include "version.h"

// bss
s32 m446FlowProcessActive;
s32 m446ActivePlayerCount;
HUPROCESS *m446ObjectProcessManager;
OMOBJ *m446ObjectManager;
s32 m446FlowManagerState;
M446FlowWork *m446Flow;

// data
Vec m446CameraStartRotation = { 0.0f, 90.0f, 0.0f };
Vec m446CameraTargetRotation = { -40.0f, 40.0f, 0.0f };
f32 m446CameraStartDistanceValue = 1260.0f;
Vec m446CameraAlternateStartRotation = { 0.0f, 90.0f, 0.0f };
Vec m446CameraAlternateTargetRotation = { -90.0f, 0.0f, 0.0f };
f32 m446CameraAlternateDistance = 260.0f;
Vec m446CameraPlayerPosition[2] = { { 0.0f, 100.0f, 120.0f }, { 0.0f, 100.0f, 120.0f } };
Vec m446CameraPlayerRotation[2] = { { -30.0f, 0.0f, 0.0f }, { -30.0f, 0.0f, 0.0f } };
f32 m446CameraPlayerDistance[2] = { 600.0f, 600.0f };
s32 m446MessagePositions[2][2] = { { 0x46, 0x1A4 }, { 0x1F4, 0x3C } };

void ObjectSetup(void)
{
    m446ObjectProcessManager = omInitObjMan(5, 0x2000);
    omGameSysInit(m446ObjectProcessManager);
    m446ObjectManager = omAddObjEx(m446ObjectProcessManager, 0x3E8, 0x32U, 0x32U, 0, M446ObjectManagerUpdate);
    omSetStatBit(m446ObjectManager, 0x100U);
    m446FlowManagerState = 0;
}

void M446ObjectManagerUpdate(OMOBJ *objectEntry)
{
    switch (m446FlowManagerState) {
        case 0:
            HuWinInit(1);
            Hu3DLighInit();
            M446RandomSeedSet(frand());
            M446CameraInitialize();
            M446StageInitialize();
            M446CardInitialize();
            M446PlayersInitialize();
            M446TableInitialize();
            M446CursorInitialize();
            m446Flow = M446FlowCreate();
            m446FlowManagerState = 1;
            break;
        case 1:
            M446FlowStateUpdate(m446Flow);
            M446StagesUpdate();
            M446PlayersUpdate();
            M446TablesSync();
            M446CursorsUpdate();
            M446CardsUpdate();
            M446CameraUpdate();
            if (m446Flow->flowState != 5)
                break;
            m446FlowManagerState = 3;
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
        case 2:
            break;
        case 3:
            if (WipeStatGet() == 0) {
                m446FlowManagerState = 4;
            }
            break;
        case 4:
            HuAudFadeOut(0x64);
            M446FlowDestroy(m446Flow);
            M446CursorUpdate();
            M446PlayersDestroy();
            M446TablesUpdate();
            M446CardsDestroyAll();
            M446StageUpdate();
            M446CameraTransitionStop();
            omOvlReturnEx(1, 1);
            break;
    }
}

M446FlowWork *M446FlowCreate(void)
{
    s32 playerIndex;
    M446FlowWork *object;

    object = HuMemDirectMallocNum(HEAP_HEAP, 0x9C, HU_MEMNUM_OVL);
    if (object == 0) {
        OSReport("[!] m446FlowCreate() ERROR...\n");
        return NULL;
    }
    object->resultSequenceId = -1;
    object->messageSequenceId = -1;
    object->currentPlayerIndex = 0;
    object->flowState = 0;
    object->stateStep = 0;
    object->flowProcess = NULL;
    object->audioSequenceId = -1;

    for (playerIndex = 0; playerIndex < 23; playerIndex++) {
        object->cardBuffer[playerIndex] = 0;
    }
    object->stageWork = M446StageCreate();
    object->tableWork = M446TableCreate();
    m446ActivePlayerCount = 0;

    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        object->playerWorks[playerIndex] = M446PlayerCreate(playerIndex, object->tableWork);
        if (object->playerWorks[playerIndex]->isComputer != 0) {
            m446ActivePlayerCount += 1;
        }
    }
    object->deckWork = M446DeckCreate();
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
    return object;
}

void M446FlowDestroy(M446FlowWork *flowWork)
{
    s32 playerIndex;

    if (flowWork->flowProcess) {
        HuPrcKill(flowWork->flowProcess);
    }
    if (flowWork->resultSequenceId == -1) {
        M446FlowResultCreate(flowWork);
    }
    if (flowWork->messageSequenceId == -1) {
        MGSeqParamSet(flowWork->messageSequenceId, 2, -1);
    }

    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        M446PlayerDestroy(flowWork->playerWorks[playerIndex]);
    }
    M446TableDestroy(flowWork->tableWork);
    M446DeckDestroy(&flowWork->deckWork->cards);
    M446StageDestroy(flowWork->stageWork);
    HuMemDirectFree(flowWork);
}

M446FlowStateCallback m446FlowStateUpdates[5] = { M446FlowInitialize, M446FlowResolve, M446FlowFinishGame, M446FlowFinishTurn, M446FlowPlay };

void M446FlowStateUpdate(M446FlowWork *flowWork)
{
    if (m446FlowStateUpdates[flowWork->flowState]) {
        m446FlowStateUpdates[flowWork->flowState](flowWork);
    }
    if (omSysExitReq != 0) {
        flowWork->flowState = 5;
    }
}

void M446FlowInitialize(M446FlowWork *flowWork)
{
    s32 playerIndex;
    s32 cardIndex;

    switch (flowWork->stateStep) {
        case 0:
            M446CameraTargetSet(&m446CameraStartRotation);
            M446CameraRotationSet(&m446CameraTargetRotation);
            M446CameraDistanceSet(m446CameraStartDistanceValue);
            flowWork->stateStep = 1;
            flowWork->stateTimer = 0;
            HuAudFXPlay(0x773);
            break;
        case 1:
            if (flowWork->stateTimer == 20) {
                M446StageStart(flowWork->stageWork);
            }
            if (flowWork->stateTimer++ < 0xB4)
                break;
            flowWork->audioSequenceId = HuAudSeqPlay(0x3D);
            if (flowWork->playerWorks[1]->characterId == 8) {
                flowWork->stateStep = 2;
            }
            else {
                flowWork->stateStep = 4;
            }
            break;
        case 2:
            flowWork->flowProcess = HuPrcChildCreate(M446FlowProcess, 0x64, 0x2000, 0, m446ObjectProcessManager);
            m446FlowProcessActive = 1;
            flowWork->stateStep = 3;
            break;
        case 3:
            if (m446FlowProcessActive == 0) {
                flowWork->flowProcess = NULL;
                flowWork->stateStep = 4;
            }
            break;
        case 4:
            M446CameraTransitionBegin(&m446CameraAlternateStartRotation, &m446CameraAlternateTargetRotation, &m446CameraAlternateDistance, 0xB4);
            flowWork->stateStep = 5;
            break;
        case 5:
            if (M446CameraTransitionIsActive())
                break;
            M446StageRenderUpdate(flowWork->stageWork);
            for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                M446PlayerAnimationSet(flowWork->playerWorks[playerIndex], 1);
            }
            flowWork->stateStep = 6;
            break;
        case 6:
            M446DeckFanOut(&flowWork->deckWork->cards, &flowWork->cardBuffer[0], &flowWork->cardBufferCount, 1);
            flowWork->stateStep = 7;
            flowWork->stateTimer = 0;
            break;
        case 7:
            if (flowWork->stateTimer++ < 0x78)
                break;
            M446DeckDealCards(flowWork->deckWork, &flowWork->cardBuffer[0], flowWork->cardBufferCount, 1);
            flowWork->stateStep = 8;
            flowWork->stateTimer = 0;
            break;
        case 8:
            if (flowWork->stateTimer++ < 0x78)
                break;

            for (cardIndex = 0; cardIndex < 2; cardIndex++) {
                for (playerIndex = 0; playerIndex < 3; playerIndex++) {
                    M446DeckPopCard(&flowWork->deckWork->cards, &flowWork->selectedCard);
                    M446PlayerAppendCard(flowWork->playerWorks[cardIndex], flowWork->selectedCard, 1);
                    M446CardShadowSet(flowWork->selectedCard, 0);
                }
            }
            HuAudFXPlay(0x767);
            flowWork->stateStep = 9;
            break;
        case 9:
            if (flowWork->selectedCard->positionAnimating != 0)
                break;
            if (flowWork->selectedCard->scaleAnimating != 0)
                break;
            if (flowWork->selectedCard->rotationAnimating != 0)
                break;
            if (flowWork->selectedCard->animationState != 0)
                break;

            for (cardIndex = 0; cardIndex < 2; cardIndex++) {
                for (playerIndex = 0; playerIndex < 3; playerIndex++) {
                    M446PlayerCardFind(flowWork->playerWorks[cardIndex], playerIndex, &flowWork->selectedCard);
                    M446CardFlipDown(flowWork->selectedCard);
                }
            }
            flowWork->stateStep = 10;
            break;
        case 10:
            if (flowWork->selectedCard->positionAnimating != 0)
                break;
            if (flowWork->selectedCard->scaleAnimating != 0)
                break;
            if (flowWork->selectedCard->rotationAnimating != 0)
                break;
            if (flowWork->selectedCard->animationState != 0)
                break;
            M446TableReserveSpaces(flowWork->tableWork);

            while (M446DeckPopCard(&flowWork->deckWork->cards, &flowWork->selectedCard) != 0) {
                M446TableAddCard(flowWork->tableWork, flowWork->selectedCard, 1);
            }
            M446TableClearReservedSpaces(flowWork->tableWork);
            if (flowWork->audioSequenceId != -1) {
                HuAudSeqFadeOut(flowWork->audioSequenceId, 0x7D0);
            }
            flowWork->stateStep = 11;
            flowWork->stateTimer = 0;
            break;
        case 11:
            if (flowWork->stateTimer++ < 0x78)
                break;
            flowWork->flowState = 1;
            flowWork->stateStep = 0;
            break;
    }
}

void M446FlowPlay(M446FlowWork *flowWork)
{
    s32 object;
    s32 object2;
    s32 object3;
    s32 playerIndex;

    switch (flowWork->stateStep) {
        case 0:
            for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                M446PlayerAnimationSet(flowWork->playerWorks[playerIndex], 0);
            }
            M446CameraTransitionBegin(&m446CameraPlayerPosition[flowWork->currentPlayerIndex], &m446CameraPlayerRotation[flowWork->currentPlayerIndex], &m446CameraPlayerDistance[flowWork->currentPlayerIndex], 0x4E);
            flowWork->stateStep = 1;
            flowWork->stateTimer = 0;
            break;
        case 1:
            if (flowWork->stateTimer++ >= 0x1E) {
                M446PlayerMoveBegin(flowWork->playerWorks[0]);
                flowWork->stateStep = 2;
                flowWork->stateTimer = 0;
            }
            break;
        case 2:
            if (flowWork->stateTimer++ >= 0x36) {
                if ((flowWork->playerWorks[0]->cardFlippedState == 0) && (flowWork->playerWorks[1]->cardFlippedState == 0)) {
                    M446PlayerLookBegin(flowWork->playerWorks[0]);
                    if (flowWork->playerWorks[1]->characterId == 8) {
                        M446PlayerIdleBegin(flowWork->playerWorks[1]);
                        HuAudSStreamPlay(3);
                    }
                    else {
                        M446PlayerLookBegin(flowWork->playerWorks[1]);
                        HuAudSStreamPlay(4);
                    }
                }
                else if (flowWork->playerWorks[0]->cardFlippedState != 0) {
                    M446PlayerIdleBegin(flowWork->playerWorks[0]);
                    M446PlayerLookBegin(flowWork->playerWorks[1]);
                    HuAudSStreamPlay(1);
                }
                else if (flowWork->playerWorks[1]->cardFlippedState != 0) {
                    M446PlayerIdleBegin(flowWork->playerWorks[1]);
                    M446PlayerLookBegin(flowWork->playerWorks[0]);
                    if (flowWork->playerWorks[1]->characterId == 8) {
                        HuAudSStreamPlay(3);
                    }
                    else {
                        HuAudSStreamPlay(1);
                    }
                }
                flowWork->stateStep = 3;
                flowWork->stateTimer = 0;
                return;
            }
            break;
        case 3:
            if (flowWork->stateTimer++ >= 0xD2) {
                flowWork->flowState = 5;
            }
            break;
    }
}

void M446FlowResolve(M446FlowWork *flowWork)
{
    switch (flowWork->stateStep) {
        case 0:
            flowWork->audioSequenceId = HuAudSeqPlay(0x3F);
            M446FlowResultDestroy(flowWork);
            flowWork->stateStep = 1;
            flowWork->stateTimer = 0;
            break;
        case 1:
            if (!M446FlowPlayerCountGet(flowWork)) {
                M446FlowResultCreate(flowWork);
                flowWork->flowState = 2;
                flowWork->stateStep = 0;
            }
            break;
    }
}

void M446FlowFinishTurn(M446FlowWork *flowWork)
{
    switch (flowWork->stateStep) {
        case 0:
            HuAudSeqFadeOut(flowWork->audioSequenceId, 0x64);
            M446FlowResultUpdate(flowWork);
            flowWork->stateStep = 1;
            flowWork->stateTimer = 0;
            break;
        case 1:
            if (M446FlowPlayerCountGet(flowWork) == 0) {
                M446FlowResultCreate(flowWork);
                flowWork->flowState = 4;
                flowWork->stateStep = 0;
            }
            break;
    }
}

void M446FlowFinishGame(M446FlowWork *flowWork)
{
    s32 cardIndex;
    M446PlayerWork *playerWork2;

    playerWork2 = flowWork->playerWorks[flowWork->currentPlayerIndex];
    switch (flowWork->stateStep) {
        case 0:
            flowWork->currentPlayerIndex = 0;
            flowWork->stateStep = 1;
            break;
        case 1:
            flowWork->countdownFrames = REFRESH_RATE * 5;
            flowWork->messageSequenceId = MGSeqCreate(1, flowWork->countdownFrames / REFRESH_RATE, -1, -1);
            MGSeqPosSet(flowWork->messageSequenceId, m446MessagePositions[flowWork->currentPlayerIndex][0], m446MessagePositions[flowWork->currentPlayerIndex][1]);
            flowWork->stateStep = 2;
        case 2:
            if (M446PlayerCardSelect(playerWork2, &flowWork->selectedCard, 0) == 0) {
                if ((--flowWork->countdownFrames % REFRESH_RATE) == 0) {
                    MGSeqParamSet(flowWork->messageSequenceId, 1, flowWork->countdownFrames / REFRESH_RATE);
                }
                if (flowWork->countdownFrames == 0) {
                    flowWork->stateStep = 3;
                    flowWork->stateTimer = 0;
                }
            }
            else {
                flowWork->stateStep = 4;
            }
            break;
        case 3:
            if (flowWork->stateTimer++ >= 60) {
                M446PlayerCardSelect(playerWork2, &flowWork->selectedCard, 0);
                flowWork->stateStep = 4;
            }
            break;
        case 4:
            MGSeqParamSet(flowWork->messageSequenceId, 2, -1);
            flowWork->messageSequenceId = -1;
            M446PlayerRemoveCard(playerWork2, flowWork->selectedCard, 3);
            flowWork->stateStep = 5;
            break;
        case 5:
            if (flowWork->selectedCard->positionAnimating == 0 && flowWork->selectedCard->scaleAnimating == 0 && flowWork->selectedCard->rotationAnimating == 0 && flowWork->selectedCard->animationState == 0) {
                M446TableAddCard(flowWork->tableWork, flowWork->selectedCard, 4);
                flowWork->stateStep = 6;
            }
            break;
        case 6:
            if (flowWork->selectedCard->positionAnimating == 0 && flowWork->selectedCard->scaleAnimating == 0 && flowWork->selectedCard->rotationAnimating == 0 && flowWork->selectedCard->animationState == 0) {
                M446CardShadowSet(flowWork->selectedCard, 1);
                flowWork->countdownFrames = REFRESH_RATE * 5;;
                flowWork->messageSequenceId = MGSeqCreate(1, flowWork->countdownFrames / REFRESH_RATE, -1, -1);
                MGSeqPosSet(flowWork->messageSequenceId, m446MessagePositions[flowWork->currentPlayerIndex][0], m446MessagePositions[flowWork->currentPlayerIndex][1]);
                M446TableSelectionReset(flowWork->tableWork, flowWork->currentPlayerIndex == 1 >> 5);
                flowWork->stateStep = 7;
            }
            else
                break;
        case 7:
            if (M446TableSelectionUpdate(flowWork->tableWork, playerWork2->characterId, playerWork2->controllerPort, flowWork->currentPlayerIndex == 1, &flowWork->selectedCard, 0) == 0) {
                if ((--flowWork->countdownFrames % REFRESH_RATE) == 0) {
                    MGSeqParamSet(flowWork->messageSequenceId, 1, flowWork->countdownFrames / REFRESH_RATE);
                }
                if (flowWork->countdownFrames <= 0) {
                    if (M446TableSelectionUpdate(flowWork->tableWork, playerWork2->characterId, playerWork2->controllerPort, flowWork->currentPlayerIndex == 1, &flowWork->selectedCard, 2) == 0) {
                        M446TableSelectionReset(flowWork->tableWork, flowWork->currentPlayerIndex == 1 >> 5);
                        flowWork->stateStep = 8;
                        flowWork->stateTimer = 0;
                        break;
                    }
                }
                else
                    break;
            }
            flowWork->stateStep = 9;
            break;
        case 8:
            M446TableSelectionUpdate(flowWork->tableWork, playerWork2->characterId, playerWork2->controllerPort, flowWork->currentPlayerIndex == 1, &flowWork->selectedCard, 1);
            if (flowWork->stateTimer++ >= 0x1E) {
                M446TableSelectionUpdate(flowWork->tableWork, playerWork2->characterId, playerWork2->controllerPort, flowWork->currentPlayerIndex == 1, &flowWork->selectedCard, 2);
                flowWork->stateStep = 9;
            }
            break;
        case 9:
            MGSeqParamSet(flowWork->messageSequenceId, 2, -1);
            flowWork->messageSequenceId = -1;
            M446CardShadowSet(flowWork->selectedCard, 0);
            M446TableRemoveCard(flowWork->tableWork, flowWork->selectedCard, 3);
            flowWork->stateStep = 10;
            break;
        case 10:
            if (flowWork->selectedCard->positionAnimating == 0 && flowWork->selectedCard->scaleAnimating == 0 && flowWork->selectedCard->rotationAnimating == 0 && flowWork->selectedCard->animationState == 0) {
                if (flowWork->selectedCard->cardId == 4) {
                    flowWork->stateStep = 13;
                }
                else {
                    M446PlayerAppendCard(playerWork2, flowWork->selectedCard, 4);
                    flowWork->stateStep = 11;
                }
            }
            break;
        case 11:
            if (flowWork->selectedCard->positionAnimating == 0 && flowWork->selectedCard->scaleAnimating == 0 && flowWork->selectedCard->rotationAnimating == 0 && flowWork->selectedCard->animationState == 0) {
                if (playerWork2->cardFlippedState != 0) {
                    flowWork->stateStep = 24;
                }
                else {
                    flowWork->stateStep = 12;
                }
            }
            break;
        case 12:
            if (M446TableCardCountByOwner(flowWork->tableWork, 5) == 0) {
                flowWork->currentPlayerIndex = 1;
                flowWork->stateStep = 25;
                break;
            }
            else if (flowWork->currentPlayerIndex == 0) {
                flowWork->currentPlayerIndex = 1;
            }
            else {
                flowWork->currentPlayerIndex = 0;
            }
            flowWork->stateStep = 1;
            break;
        case 13:
            M446PlayerMotionSet(playerWork2, 0, 60);
            HuAudFXPlay(0x4E);
            flowWork->stateStep = 14;
        case 14:
            M446CardTurnBegin(flowWork->selectedCard);
            flowWork->stateStep = 15;
            break;
        case 15:
            if (flowWork->selectedCard->positionAnimating == 0 && flowWork->selectedCard->scaleAnimating == 0 && flowWork->selectedCard->rotationAnimating == 0 && flowWork->selectedCard->animationState == 0) {
                M446TableRemoveCard((M446TableWork *)playerWork2, flowWork->selectedCard, 4);
                M446CardDestroy(flowWork->selectedCard);
                for (cardIndex = 0; cardIndex < 3; cardIndex++) {
                    M446PlayerCardFind(playerWork2, cardIndex, &flowWork->selectedCard);
                    if (flowWork->selectedCard) {
                        M446PlayerRemoveCard(playerWork2, flowWork->selectedCard, 4);
                        M446TableAddCard(flowWork->tableWork, flowWork->selectedCard, 3);
                    }
                }

                flowWork->stateStep = 16;
                flowWork->stateTimer = 0;
            }
            break;
        case 16:
            if (flowWork->stateTimer++ >= 60) {
                for (cardIndex = 0; cardIndex < 21; cardIndex++) {
                    M446TableCardGet(flowWork->tableWork, cardIndex, &flowWork->selectedCard);
                    if (flowWork->selectedCard) {
                        M446CardFlipUp(flowWork->selectedCard);
                        M446CardShadowSet(flowWork->selectedCard, 1);
                    }
                }
                flowWork->stateStep = 17;
                flowWork->stateTimer = 0;
            }
            break;
        case 17:
            if (flowWork->stateTimer++ >= 60) {
                for (cardIndex = 0; cardIndex < 21; cardIndex++) {
                    M446TableCardGet(flowWork->tableWork, cardIndex, &flowWork->selectedCard);
                    if (flowWork->selectedCard) {
                        M446TableRemoveCard(flowWork->tableWork, flowWork->selectedCard, 4);
                        M446DeckPushCard(flowWork->deckWork, flowWork->selectedCard, 2);
                    }
                }
                flowWork->stateStep = 18;
                flowWork->stateTimer = 0;
            }
            break;
        case 18:
            if (flowWork->stateTimer++ >= 60) {
                M446DeckFanOut(&flowWork->deckWork->cards, &flowWork->cardBuffer[0], &flowWork->cardBufferCount, 1);
                flowWork->stateStep = 19;
                flowWork->stateTimer = 0;
            }
            break;
        case 19:
            if (flowWork->stateTimer++ >= 120) {
                M446DeckDealCards(flowWork->deckWork, &flowWork->cardBuffer[0], flowWork->cardBufferCount, 1);
                flowWork->stateStep = 20;
                flowWork->stateTimer = 0;
            }
            break;
        case 20:
            if (flowWork->stateTimer++ >= 120) {
                for (cardIndex = 0; cardIndex < 3; cardIndex++) {
                    M446DeckPopCard(&flowWork->deckWork->cards, &flowWork->selectedCard);
                    M446PlayerAppendCard(playerWork2, flowWork->selectedCard, 1);
                    M446CardShadowSet(flowWork->selectedCard, 0);
                }
                HuAudFXPlay(0x767);
                flowWork->stateStep = 21;
            }
            break;
        case 21:
            if (flowWork->selectedCard->positionAnimating == 0 && flowWork->selectedCard->scaleAnimating == 0 && flowWork->selectedCard->rotationAnimating == 0 && flowWork->selectedCard->animationState == 0) {
                for (cardIndex = 0; cardIndex < 3; cardIndex++) {
                    M446PlayerCardFind(playerWork2, cardIndex, &flowWork->selectedCard);
                    M446CardFlipDown(flowWork->selectedCard);
                }

                flowWork->stateStep = 22;
            }
            break;
        case 22:
            if (flowWork->selectedCard->positionAnimating == 0 && flowWork->selectedCard->scaleAnimating == 0 && flowWork->selectedCard->rotationAnimating == 0 && flowWork->selectedCard->animationState == 0) {
                M446TableReserveSpaces(flowWork->tableWork);

                while (M446DeckPopCard(&flowWork->deckWork->cards, &flowWork->selectedCard)) {
                    M446TableAddCard(flowWork->tableWork, flowWork->selectedCard, 1);
                }
                M446TableClearReservedSpaces(flowWork->tableWork);
                flowWork->stateStep = 23;
                flowWork->stateTimer = 0;
            }
            break;
        case 23:
            if (flowWork->stateTimer++ >= 120) {
                flowWork->stateStep = 12;
            }
            break;
        case 24:
            for (cardIndex = 0; cardIndex < 3; cardIndex++) {
                M446PlayerCardFind(playerWork2, cardIndex, &flowWork->selectedCard);
                if (flowWork->selectedCard) {
                    M446CardShuffleBegin(flowWork->selectedCard);
                }
            }
            GWPlayerCoinWinAdd(flowWork->currentPlayerIndex, 10);
        case 25:
            flowWork->flowState = 3;
            flowWork->stateStep = 0;
            break;
    }
}

s32 M446FlowPlayerCountGet(M446FlowWork *flowWork)
{
    return MGSeqStatGet(flowWork->resultSequenceId) != 0;
}

void M446FlowResultCreate(M446FlowWork *flowWork)
{
    MGSeqKill(flowWork->resultSequenceId);
    flowWork->resultSequenceId = -1;
}

void M446FlowResultDestroy(M446FlowWork *flowWork)
{
    flowWork->resultSequenceId = MGSeqCreate(3, 0);
}

void M446FlowResultUpdate(M446FlowWork *flowWork)
{
    flowWork->resultSequenceId = MGSeqCreate(3, 1);
}

void M446MessageShow(s16 windowId, s32 waitFrames)
{
    HuWinComKeyReset();
    if (m446ActivePlayerCount == 2) {
        HuWinComKeyWait(0x100, 0x100, 0x100, 0x100, waitFrames);
    }
    winData[windowId].active_pad = 1;
    HuWinMesWait(windowId);
}

s32 M446MessageWait(s16 windowId, s32 choiceCount, s32 cancelValue, s32 delayFrames)
{
    if (m446ActivePlayerCount == 2) {
        HuPrcSleep(delayFrames);
        return cancelValue;
    }
    else {
        return HuWinChoiceGet(windowId, choiceCount);
    }
}

static const s32 m446MessageIds[12] = { MAKE_MESSID(0x18, 0x03), MAKE_MESSID(0x18, 0x04), MAKE_MESSID(0x18, 0x05), MAKE_MESSID(0x18, 0x06),
    MAKE_MESSID(0x18, 0x07), MAKE_MESSID(0x18, 0x08), MAKE_MESSID(0x18, 0x09), MAKE_MESSID(0x18, 0x0A), MAKE_MESSID(0x18, 0x0B),
    MAKE_MESSID(0x18, 0x0C), MAKE_MESSID(0x18, 0x0D), 0 };

void M446FlowProcess(void)
{
    s16 object;
    s16 playerIndex;
    s32 result;
    s32 cardIndex;
    u32 cursorWork;

    object = HuWinExCreateStyled(-10000.0f, 40.0f, 0x138, 0x78, -1, 1);
    HuWinExAnimIn(object);
    HuWinBGTPLvlSet(object, 0.8f);
    HuWinMesSpeedSet(object, 1);
    HuWinMesColSet(object, 0);
    HuWinMesSet(object, MAKE_MESSID(0x18, 0x01));
    M446MessageShow(object, 0x3C);
    if (m446ActivePlayerCount < 2) {
        HuWinMesColSet(object, 0);
        HuWinMesSet(object, MAKE_MESSID(0x18, 0x02));
        playerIndex = M446MessageWait(object, 0, 1, 0x3C);
    }
    else {
        playerIndex = 1;
    }
    if (playerIndex == 0) {
        cardIndex = 0;
        while ((cursorWork = m446MessageIds[cardIndex++]) != 0) {
            HuWinMesColSet(object, 0);
            HuWinMesSet(object, cursorWork);
            M446MessageShow(object, 0x3C);
        }
    }
    else {
        HuWinMesColSet(object, 0);
        HuWinMesSet(object, MAKE_MESSID(0x18, 0x0D));
        M446MessageShow(object, 0x3C);
    }
    HuWinExAnimOut(object);
    HuWinExCleanup(object);
    m446FlowProcessActive = 0;
    HuPrcEnd();
    while (1) {
        HuPrcVSleep();
    }
}

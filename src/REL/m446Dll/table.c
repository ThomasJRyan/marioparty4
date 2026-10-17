#include "REL/m446Dll.h"
#include "game/audio.h"
#include "game/hu3d.h"
#include "game/pad.h"

// bss
M446ListNode *m446TableListHead;

s32 M446TableInitialize(void)
{
    m446TableListHead = NULL;
    return 1;
}

void M446TablesUpdate(void)
{
    M446ListNode *listNode;
    M446TableWork *tableWork2;

    listNode = m446TableListHead;
    if (listNode) {
        do {
            tableWork2 = listNode->value;
            M446TableDestroy(tableWork2);
            listNode = listNode->next;
        } while (listNode);
    }
}

void M446TablesSync(void)
{
    M446CardWork *cardWork;
    M446ListNode *listNode;

    listNode = m446TableListHead;
    if (listNode) {
        do {
            cardWork = listNode->value;
            listNode = listNode->next;
        } while (listNode);
    }
}

M446TableWork *M446TableCreate(void)
{
    s32 playerIndex;
    M446ListNode *listNode3;
    M446TableWork *tableWork2;

    tableWork2 = HuMemDirectMallocNum(HEAP_HEAP, 0x70, HU_MEMNUM_OVL);
    if (!tableWork2) {
        return NULL;
    }
    tableWork2->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M446, 0x05));
    Hu3DModelAmbSet(tableWork2->modelId, 0.0f, 0.0f, 0.0f);
    Hu3DModelShadowMapSet(tableWork2->modelId);
    Hu3DModelLayerSet(tableWork2->modelId, 0);
    tableWork2->cursorWork = M446CursorCreate(0);
    M446CursorActivate(tableWork2->cursorWork);
    tableWork2->selectedCardIndex = 10;
    tableWork2->cardCount = 0;
    tableWork2->automaticSelectionActive = 0;

    for (playerIndex = 0; playerIndex < 21; playerIndex++) {
        tableWork2->cards[playerIndex] = NULL;
    }
    M446TableReserveSpaces(tableWork2);
    listNode3 = M446ListNodeCreate(tableWork2);
    if (!listNode3) {
        OSReport("[!] m446TableCreate ERROR...\n");
        HuMemDirectFree(tableWork2);
        return NULL;
    }
    if (m446TableListHead) {
        M446ListAppend(m446TableListHead, listNode3);
    }
    else {
        m446TableListHead = listNode3;
    }
    return tableWork2;
}

void M446TableDestroy(M446TableWork *tableWork)
{
    M446ListNode *listNode2;

    listNode2 = m446TableListHead;
    do {
        if (tableWork == listNode2->value) {
            if (listNode2 == m446TableListHead) {
                m446TableListHead = listNode2->next;
            }
            M446ListRemove(listNode2);
            M446ListNodeDestroy(listNode2);
            Hu3DModelKill(tableWork->modelId);
            M446CursorDestroy(tableWork->cursorWork);
            HuMemDirectFree(tableWork);
            return;
        }
        listNode2 = listNode2->next;
    } while (listNode2);
}

s32 M446TableAddCard(M446TableWork *tableWork, M446CardWork *cardWork, s32 cardPlacementMode)
{
    Vec offset;
    Vec targetPosition;
    s32 cardId;

    if (tableWork->cardCount >= 21) {
        return 0;
    }
    do {
        cardId = M446RandomGet() % 21;
    } while (tableWork->cards[cardId] != 0);
    tableWork->cards[cardId] = cardWork;
    tableWork->cardCount++;
    offset.x = -60.0f + (20.0f * (cardId % 7));
    offset.y = 85.0f;
    offset.z = -20.0f + (20.0f * (cardId / 7));
    targetPosition = cardWork->rotation;
    switch (cardPlacementMode) {
        case 3:
        case 4:
            targetPosition.z = 720.0f;
            cardWork->rotation = targetPosition;
            targetPosition.z = 0.0f;
            M446CardMoveTo(cardWork, &offset, 0x3C);
            M446CardRotateTo(cardWork, &targetPosition, 0x3C);
            M446CardScaleTo(cardWork, 1.0f, 0x3C);
            break;
        case 0:
            targetPosition.y = 0.0f;
            cardWork->position = offset;
            cardWork->rotation = targetPosition;
            cardWork->scale = 1.0f;
            break;
        case 2:
            break;
        case 1:
            targetPosition.y = 0.0f;
            M446CardMoveTo(cardWork, &offset, 0x3C);
            M446CardRotateTo(cardWork, &targetPosition, 0x3C);
            break;
    }
    return 1;
}

s32 M446TableRemoveCard(M446TableWork *tableWork, M446CardWork *cardWork, s32 cardRemovalMode)
{
    Vec targetPosition;
    s32 loopIndex;

    for (loopIndex = 0; loopIndex < 0x15; loopIndex++) {
        if (tableWork->cards[loopIndex] == cardWork) {
            tableWork->cards[loopIndex] = NULL;
            tableWork->cardCount--;
            break;
        }
    }
    if (loopIndex == 0x15) {
        OSReport("m446TableRemoveCard ERROR...\n");
        return 0;
    }
    switch (cardRemovalMode) {
        case 3:
            targetPosition = cardWork->position;
            targetPosition.y = 90.0f;
            M446CardFlipDown(cardWork);
            M446CardMoveTo(cardWork, &targetPosition, 0x1E);
            M446CardScaleTo(cardWork, 1.5f, 0x1E);
            HuAudFXPlay(0x76C);
    }
    return 1;
}

s32 M446TableCardGet(M446TableWork *tableWork, s32 cardIndex, M446CardWork **cardOut)
{
    if ((cardIndex < 0) || (cardIndex >= 0x15)) {
        return 0;
    }
    *cardOut = tableWork->cards[cardIndex];
    return 1;
}

void M446TableSelectionReset(M446TableWork *tableWork, s32 reverseOrder)
{
    s32 loopIndex;
    M446CardWork *cardWork;

    for (loopIndex = 0; loopIndex < 21; loopIndex++) {
        if (reverseOrder != 0) {
            cardWork = tableWork->cards[20 - loopIndex];
        }
        else {
            cardWork = tableWork->cards[loopIndex];
        }
        if ((cardWork) && (cardWork->faceDown == 0)) {
            if (reverseOrder != 0) {
                tableWork->selectedCardIndex = (0x14 - loopIndex);
                return;
            }
            else {
                tableWork->selectedCardIndex = loopIndex;
                return;
            }
        }
    }
}

s32 M446TableSelectionUpdate(M446TableWork *tableWork, u8 playerIndex, u8 controllerPort, s32 cursorMode, M446CardWork **cardOut, s32 selectionMode)
{
    Vec scaleVector;
    s32 modelIndex;
    s32 stateIndex;
    M446CardWork *cardWork;
    s32 cardCount;
    s32 cardId;
    s32 playerCount;
    s32 cardIndex;
    s32 spaceIndex;

    cardCount = 0;
    cardId = 0;
    modelIndex = 0;
    stateIndex = 0;
    playerCount = 0;
    *cardOut = NULL;
    if (controllerPort >= 4) {
        if (tableWork->automaticSelectionActive == 0) {
            tableWork->automaticCardIndex = (s32)(M446RandomGet() % 21);
            do {
                if (!(cardWork = tableWork->cards[tableWork->automaticCardIndex]) || cardWork->faceDown != 0) {
                    tableWork->automaticCardIndex = ++tableWork->automaticCardIndex % 21;
                }
                else
                    break;
                (void)cardWork;
            } while (TRUE);
            tableWork->automaticSelectionActive = 1;
            tableWork->selectionDelay = 0x1E;
        }
        if ((tableWork->selectionDelay > 0) && (selectionMode == 0)) {
            tableWork->selectionDelay--;
        }
        else {
            modelIndex = (tableWork->automaticCardIndex / 7) < (tableWork->selectedCardIndex / 7);
            stateIndex = (tableWork->automaticCardIndex / 7) > (tableWork->selectedCardIndex / 7);
            if ((modelIndex == 0) && (stateIndex == 0)) {
                cardCount = (tableWork->automaticCardIndex) < (tableWork->selectedCardIndex);
                cardId = (tableWork->automaticCardIndex) > (tableWork->selectedCardIndex);
            }
            playerCount = tableWork->automaticCardIndex == tableWork->selectedCardIndex;
            tableWork->selectionDelay = 0x12;
        }
    }
    else if (selectionMode == 0) {
        cardCount = (HuPadDStkRep[controllerPort] & 1) != 0;
        cardId = (HuPadDStkRep[controllerPort] & 2) != 0;
        modelIndex = (HuPadDStkRep[controllerPort] & 8) != 0;
        stateIndex = (HuPadDStkRep[controllerPort] & 4) != 0;
        playerCount = (HuPadBtnDown[controllerPort] & 0x100) != 0;
    }
    if (cardCount != 0) {
        if ((tableWork->selectedCardIndex % 7) == 0) {
            tableWork->selectedCardIndex = (tableWork->selectedCardIndex + 6);
        }
        else {
            tableWork->selectedCardIndex = (tableWork->selectedCardIndex - 1);
        }
        HuAudFXPlay(0);
    }
    if (cardId != 0) {
        if ((tableWork->selectedCardIndex % 7) == 6) {
            tableWork->selectedCardIndex = (tableWork->selectedCardIndex - 6);
        }
        else {
            tableWork->selectedCardIndex = (tableWork->selectedCardIndex + 1);
        }
        HuAudFXPlay(0);
    }
    if (modelIndex != 0) {
        if ((tableWork->selectedCardIndex / 7) == 0) {
            tableWork->selectedCardIndex = (tableWork->selectedCardIndex + 0xE);
        }
        else {
            tableWork->selectedCardIndex = (tableWork->selectedCardIndex - 7);
        }
        HuAudFXPlay(0);
    }
    if (stateIndex != 0) {
        if ((tableWork->selectedCardIndex / 7) == 2) {
            tableWork->selectedCardIndex = (tableWork->selectedCardIndex - 0xE);
        }
        else {
            tableWork->selectedCardIndex = (tableWork->selectedCardIndex + 7);
        }
        HuAudFXPlay(0);
    }
    cardIndex = tableWork->selectedCardIndex % 7;
    spaceIndex = tableWork->selectedCardIndex / 7;
    scaleVector.x = -60.0f + (20.0f * cardIndex);
    scaleVector.y = 86.0f;
    scaleVector.z = -20.0f + (20.0f * spaceIndex);
    tableWork->cursorWork->position = scaleVector;
    if (tableWork->cursorWork->cursorShown == 0) {
        M446CursorPlayerSet(tableWork->cursorWork, playerIndex);
        tableWork->cursorWork->reverseDirection = cursorMode;
    }
    if ((playerCount != 0) || (selectionMode == 2)) {
        *cardOut = tableWork->cards[((cardIndex + (spaceIndex * 7)))];
        if (*cardOut) {
            if ((*cardOut)->faceDown == 0) {
                HuAudFXPlay(0x76A);
                M446CursorActivate(tableWork->cursorWork);
                tableWork->automaticSelectionActive = 0;
                return 1;
            }
            HuAudFXPlay(4);
        }
        else {
            HuAudFXPlay(4);
        }
    }
    return 0;
}

void M446TableReserveSpaces(M446TableWork *tableWork)
{
    tableWork->cards[0] = (M446CardWork *)-1;
    tableWork->cards[6] = (M446CardWork *)-1;
    tableWork->cards[14] = (M446CardWork *)-1;
    tableWork->cards[20] = (M446CardWork *)-1;
}

void M446TableClearReservedSpaces(M446TableWork *tableWork)
{
    s32 loopIndex;

    for (loopIndex = 0; loopIndex < 21; loopIndex++) {
        if (tableWork->cards[loopIndex] == (M446CardWork *)-1) {
            tableWork->cards[loopIndex] = 0;
        }
    }
}

s32 M446TableCardCountByOwner(M446TableWork *tableWork, s32 ownerPlayerIndex)
{
    s32 cardIndex;
    s32 playerIndex;
    M446CardWork *cardWork;

    for (cardIndex = 0, playerIndex = 0; playerIndex < 21; playerIndex++) {
        cardWork = tableWork->cards[playerIndex];
        if ((cardWork) && (cardWork->faceDown == 0) && ((cardWork->cardId == ownerPlayerIndex) || (ownerPlayerIndex == 5))) {
            cardIndex++;
        }
    }
    return cardIndex;
}

void M446TableRevealOwnerCards(M446TableWork *tableWork)
{
    M446CardWork *cardWork2;
    s32 playerIndex;

    for (playerIndex = 0; playerIndex < 0x15; playerIndex++) {
        cardWork2 = tableWork->cards[playerIndex];
        if (cardWork2 && (cardWork2->faceDown != 0)) {
            M446CardFlipUp(cardWork2);
        }
    }
}

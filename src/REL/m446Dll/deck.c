#include "REL/m446Dll.h"

#include "game/audio.h"
#include "ext_math.h"

// bss
u32 m446RandomSeed;

// data
static const s32 m446DeckCardCounts[5][2] = {
    {1, 5}, {0, 5}, {2, 5}, {3, 5}, {4, 3}
};

M446DeckWork* M446DeckCreate(void) {
    M446CardWork* stateIndex;
    s32 modelIndex;
    s32 cardIndex;
    s32 playerIndex;
    M446DeckWork* object;

    object = HuMemDirectMallocNum(HEAP_HEAP, 0x10, HU_MEMNUM_OVL);
    if (!object) {
        return NULL;
    }
    object->cards = M446PointerListCreate(0x17);
    if (!object->cards) {
        HuMemDirectFree(object);
        return NULL;
    }
    object->cardPosition.x = 0.0f;
    object->cardPosition.y = 85.0f;
    object->cardPosition.z = 0.0f;
    
    for (modelIndex = 0, playerIndex = 0; playerIndex < 5; playerIndex++) {
        for (cardIndex = 0; cardIndex < m446DeckCardCounts[playerIndex][1]; cardIndex++, modelIndex++) {
            stateIndex = M446CardCreate(m446DeckCardCounts[playerIndex][0]);
            M446DeckPushCard(object, stateIndex, 0);
        }
    }
    return object;
}

void M446DeckDestroy(M446PointerList** cardList) {
    M446CardWork* cardWork2;

    while (M446PointerListPop(*cardList, &cardWork2) != 0) {
        M446CardDestroy(cardWork2);
    }
    M446PointerListDestroy(*cardList);
    HuMemDirectFree(cardList);
}

void M446DeckFanOut(M446PointerList** cardList, M446CardWork** cardArray, s32* cardCountOut, s32 animateCards) {
    Vec scaleVector;
    M446CardWork *cardWork2;
    f32 movementMagnitude;
    f32 movementAngle;
    s32 cardIndex;
    s32 loopIndex;
    
    HuAudFXPlay(0x765);
    movementMagnitude = 360.0f / M446PointerListCount(*cardList);
    if (M446RandomGet() % 2 != 0) {
        movementMagnitude = -movementMagnitude;
    }
    
    movementAngle = M446RandomGet() % 360;
    cardIndex = 0;
    loopIndex = 0;
    while (M446PointerListPop(*cardList, &cardWork2) != 0) {
        if (animateCards != 0) {
            scaleVector = cardWork2->position;
            scaleVector.x = ((25.0 * cosd(movementAngle)) - (25.0 * sind(movementAngle)));
            scaleVector.z = ((25.0 * cosd(movementAngle)) + (25.0 * sind(movementAngle)));
            scaleVector.y = 81.0f + (0.1f * loopIndex);
            M446CardMoveToDelayed(cardWork2, &scaleVector, 0x12, cardIndex += 3);
            movementAngle += movementMagnitude;
        }
        cardArray[loopIndex++] = cardWork2;
    }
    *cardCountOut = loopIndex;
}

void M446DeckDealCards(M446DeckWork* deckWork, M446CardWork** cardsOut, s32 cardPoolSize, s32 dealMode) {
    Vec scaleVector;
    M446CardWork* cardWork2;
    s32 cardWork;
    s32 itemIndex;
    s32 cardCount;
    s32 cardId;
    s32 controllerPort;
    M446PointerList* deckEntry;
    s32 stateIndex;

    controllerPort = cardPoolSize;
    HuAudFXPlay(0x766);
    deckEntry = M446PointerListCreate(6);
    stateIndex = 0;
    do {
        
        do {
        cardWork = M446RandomGet() % controllerPort;
        cardWork2 = cardsOut[cardWork];
        } while (!cardWork2);
        
        if (cardWork2->cardId != 4) {
            if ((stateIndex % 3) == 0) {
                cardCount = cardWork2->cardId;
            } else if ((stateIndex % 3) == 1) {
                itemIndex = cardWork2->cardId;
                if (cardWork2->cardId == cardCount) {
                    continue;
                }
            } else if ((cardWork2->cardId == cardCount) || (cardWork2->cardId == itemIndex)) continue;
            cardsOut[cardWork] = NULL;
            M446PointerListAppend(deckEntry, cardWork2);
            stateIndex++;
        }
    } while (M446PointerListCount(deckEntry) < 6);
    cardId = 0;
    do {
        if (cardPoolSize > 6) {
            do {
                cardWork = M446RandomGet() % controllerPort;
                cardWork2 = cardsOut[cardWork];
            } while (!cardWork2);
            cardsOut[cardWork] = NULL;
        } else {
            M446PointerListPop(deckEntry, (void **)&cardWork2);
        }
        M446PointerListAppend(deckEntry, cardWork2);
        scaleVector = deckWork->cardPosition;
        scaleVector.y += 0.5f * M446PointerListCount(deckWork->cards);
        if (dealMode != 0) {
            M446CardMoveToDelayed(cardWork2, &scaleVector, 0x12, cardId += 3);
        } else {
            cardWork2->position = scaleVector;
        }
        cardPoolSize--;
    } while (cardPoolSize != 0);
    M446PointerListDestroy(deckEntry);
}

s32 M446DeckPushCard(M446DeckWork* deckWork, M446CardWork* cardWork, s32 placementMode) {
    Vec offset;
    Vec targetPosition;
    
    if (M446PointerListAppend(deckWork->cards, cardWork) == 0) {
        OSReport("[!] m446DeckPushCard ERROR...\n");
        return 0;
    }
    offset = deckWork->cardPosition;
    offset.y += 0.5f * M446PointerListCount(deckWork->cards);
    targetPosition = cardWork->rotation;
    targetPosition.y = 0.0f;
    switch (placementMode) {
        case 2:
            M446CardMoveTo(cardWork, &offset, 0x3C);
            M446CardRotateTo(cardWork, &targetPosition, 0x3C);
            break;
        default:
            cardWork->position = offset;
            cardWork->rotation = targetPosition;
    }
    return 1;
}

s32 M446DeckPopCard(M446PointerList** cardList, M446CardWork** cardOut) {
    if (M446PointerListPop(*cardList, cardOut) == 0) {
        OSReport("[!] m446DeckPopCard ERROR...\n");
        return 0;
    } else {
        return 1;
    }
}

void M446DeckRefresh(M446PointerList** cardList) {
    M446PointerListCount(*cardList);
}

void M446RandomSeedSet(u32 randomSeed) {
    m446RandomSeed = randomSeed;
}

s32 M446RandomGet(void) {
    return ((m446RandomSeed = (m446RandomSeed * 0x41C64E6D) + 0x3039) >> 16) % 32768;
}

#include "REL/m446Dll.h"

#include "game/hu3d.h"
#include "ext_math.h"

// data
M446CardActionCallback m446CardStateUpdates[3] = { NULL, M446CardTurnUpdate, M446CardShuffleUpdate };
static const s32 m446CardFaceModelFiles[5] = { 0x4D0001, 0x4D0002, 0x4D0003, 0x4D0004, 0x4D0000 };
static const s32 m446CardBackModelFiles[5] = { 0x4D001B, 0x4D001C, 0x4D001D, 0x4D001E, 0x4D001E };

// bss
M446ListNode *m446CardListHead;

M446PointerList *M446PointerListCreate(s32 listCapacity)
{
    M446PointerList *object;

    object = HuMemDirectMallocNum(HEAP_HEAP, 0xC, HU_MEMNUM_OVL);
    if (!object) {
        return NULL;
    }
    object->items = HuMemDirectMallocNum(HEAP_HEAP, listCapacity * sizeof(void *), HU_MEMNUM_OVL);
    if (!object->items) {
        HuMemDirectFree(object);
        return NULL;
    }
    object->capacity = listCapacity;
    object->count = 0;
    return object;
}

void M446PointerListDestroy(M446PointerList *pointerList)
{
    HuMemDirectFree(pointerList->items);
    HuMemDirectFree(pointerList);
}

s32 M446PointerListAppend(M446PointerList *pointerList, void *item)
{
    if (pointerList->capacity <= pointerList->count) {
        return 0;
    }
    else {
        pointerList->items[pointerList->count] = item;
        pointerList->count++;
        return 1;
    }
}

s32 M446PointerListPop(M446PointerList *pointerList, M446CardWork **cardOut)
{
    if (pointerList->count <= 0) {
        return 0;
    }
    else {
        pointerList->count--;
        *cardOut = pointerList->items[pointerList->count];
        return 1;
    }
}

s32 M446PointerListPopValue(M446PointerList *pointerList, void **valueOut)
{
    if (pointerList->count <= 0) {
        return 0;
    }
    else {
        *valueOut = *(&pointerList->items[pointerList->count] - 1);
        return 1;
    }
}

BOOL M446PointerListIsEmpty(M446PointerList *pointerList)
{
    return pointerList->count == 0;
}

s32 M446PointerListCount(M446PointerList *pointerList)
{
    return pointerList->count;
}

#pragma dontInline on

M446ListNode *M446ListNodeCreate(void *value)
{
    M446ListNode *object;

    object = HuMemDirectMallocNum(HEAP_HEAP, 0xC, HU_MEMNUM_OVL);
    if (!object) {
        return NULL;
    }
    object->previous = NULL;
    object->next = NULL;
    object->value = value;
    return object;
}

void M446ListNodeDestroy(M446ListNode *listNode)
{
    HuMemDirectFree(listNode);
}

void M446ListDestroy(M446ListNode **listHead)
{
    M446ListNode *listNode = *listHead;
    HuMemDirectFree(listNode);
    *listHead = NULL;
}

void M446ListAppend(M446ListNode *listNode, M446ListNode *listNode2)
{
    if (listNode) {
        listNode2->next = listNode->next;
        listNode2->previous = listNode;
        listNode->next = listNode2;
        if (listNode2->next) {
            listNode2->next->previous = listNode2;
        }
    }
    else {
        listNode2->previous = listNode2;
        listNode2->next = listNode2;
    }
}

void M446ListRemove(M446ListNode *listNode)
{
    if (listNode->next) {
        listNode->next->previous = listNode->previous;
    }
    if (listNode->previous) {
        listNode->previous->next = listNode->next;
    }
}

#pragma dontInline reset

void M446BytesCopy(u8 *destination, u8 *source, s32 byteCount)
{
    u8 *playerIndex = destination;
    u8 *loopIndex = source;
    if (byteCount <= 0) {
        return;
    }
    while (byteCount-- != 0) {
        *playerIndex = *loopIndex++;
        *playerIndex++;
    }
}

void M446BytesFill(u8 *destination, s8 fillValue, s32 byteCount)
{
    u8 *playerIndex = destination;
    if (byteCount <= 0) {
        return;
    }
    while (byteCount-- != 0) {
        *playerIndex = fillValue;
        *playerIndex++;
    }
}

void M446BytesClear(u8 *destination, s32 byteCount)
{
    u8 *playerIndex = destination;
    if (byteCount <= 0) {
        return;
    }
    while (byteCount-- != 0) {
        *playerIndex = 0;
        *playerIndex++;
    }
}

s32 M446SignedBytesCompare(s8 *leftBuffer, s8 *rightBuffer, s32 byteCount)
{
    s8 *playerIndex = leftBuffer;
    s8 *loopIndex = rightBuffer;

    if (byteCount <= 0) {
        return 0;
    }
    while (*playerIndex == *loopIndex) {
        playerIndex++;
        loopIndex++;
    }
    return *playerIndex - *loopIndex;
}

s32 M446CardInitialize(void)
{
    m446CardListHead = 0;
    return 1;
}

void M446CardsDestroyAll(void)
{
    M446CardWork *cardWork2;
    M446ListNode *listNode;
#ifdef NON_MATCHING
    M446ListNode *listNode2;
#endif

    listNode = m446CardListHead;
    if (listNode) {
        do {
            cardWork2 = listNode->value;
#ifdef NON_MATCHING
            // fixes use after free
            listNode2 = listNode->next;
            M446CardDestroy(cardWork2);
            listNode = listNode2;
#else
            M446CardDestroy(cardWork2);
            listNode = listNode->next;
#endif
        } while (listNode);
    }
}

void M446CardsUpdate(void)
{
    M446FlowStateCallback *windowId;
    f32 distance;
    s32 messageId;
    M446CardWork *cardWork2;
    M446ListNode *listNode;

    listNode = m446CardListHead;
    if (listNode) {
        do {
            cardWork2 = listNode->value;
            if (m446CardStateUpdates[cardWork2->animationState]) {
                m446CardStateUpdates[cardWork2->animationState](cardWork2);
            }
            if (cardWork2->positionAnimating != 0) {
                if (cardWork2->positionDelay > 0) {
                    cardWork2->positionDelay--;
                }
                else {
                    distance = sind(90.0f * cardWork2->positionProgress);
                    cardWork2->position.x = cardWork2->positionStart.x + (distance * (cardWork2->positionTarget.x - cardWork2->positionStart.x));
                    cardWork2->position.y = cardWork2->positionStart.y + (distance * (cardWork2->positionTarget.y - cardWork2->positionStart.y));
                    cardWork2->position.z = cardWork2->positionStart.z + (distance * (cardWork2->positionTarget.z - cardWork2->positionStart.z));
                    cardWork2->positionProgress = cardWork2->positionProgress + cardWork2->positionStep;
                    if (cardWork2->positionProgress >= 1.0f) {
                        cardWork2->position.x = cardWork2->positionTarget.x;
                        cardWork2->position.y = cardWork2->positionTarget.y;
                        cardWork2->position.z = cardWork2->positionTarget.z;
                        cardWork2->positionAnimating = 0;
                    }
                }
            }
            if (cardWork2->rotationAnimating != 0) {
                distance = sind(90.0f * cardWork2->rotationProgress);
                cardWork2->rotation.x = (cardWork2->rotationStart.x + (distance * (cardWork2->rotationTarget.x - cardWork2->rotationStart.x)));
                cardWork2->rotation.y = (cardWork2->rotationStart.y + (distance * (cardWork2->rotationTarget.y - cardWork2->rotationStart.y)));
                cardWork2->rotation.z = (cardWork2->rotationStart.z + (distance * (cardWork2->rotationTarget.z - cardWork2->rotationStart.z)));
                cardWork2->rotationProgress = (cardWork2->rotationProgress + cardWork2->rotationStep);
                if (cardWork2->rotationProgress >= 1.0f) {
                    cardWork2->rotation.x = cardWork2->rotationTarget.x;
                    cardWork2->rotation.y = cardWork2->rotationTarget.y;
                    cardWork2->rotation.z = cardWork2->rotationTarget.z;
                    cardWork2->rotationAnimating = 0;
                }
            }
            if (cardWork2->scaleAnimating != 0) {
                distance = sind(90.0f * cardWork2->scaleProgress);
                cardWork2->scale = (cardWork2->scaleStart + (distance * (cardWork2->scaleTarget - cardWork2->scaleStart)));
                cardWork2->scaleProgress = (cardWork2->scaleProgress + cardWork2->scaleStep);
                if (cardWork2->scaleProgress >= 1.0f) {
                    cardWork2->scale = cardWork2->scaleTarget;
                    cardWork2->scaleAnimating = 0;
                }
            }
            M446CardRenderUpdate(cardWork2);
            listNode = listNode->next;
        } while (listNode);
    }
}

M446CardWork *M446CardCreate(s32 cardId)
{
    M446CardWork *cardWork2;
    M446ListNode *cardWork;

    cardWork2 = HuMemDirectMallocNum(HEAP_HEAP, 0x98, HU_MEMNUM_OVL);
    if (!cardWork2) {
        OSReport("[!] m446CardCreate ERROR...\n");
        return NULL;
    }
    cardWork2->cardId = cardId;
    cardWork2->animationState = 0;
    cardWork2->position.x = 0.0f;
    cardWork2->position.y = 0.0f;
    cardWork2->position.z = 0.0f;
    cardWork2->rotation.x = 0.0f;
    cardWork2->rotation.y = 0.0f;
    cardWork2->rotation.z = 180.0f;
    cardWork2->scale = 1.0f;
    cardWork2->faceDown = 0;
    cardWork2->selectionState = 1;
    cardWork2->cardFaceVisible = 0;
    cardWork2->positionAnimating = 0;
    cardWork2->rotationAnimating = 0;
    cardWork2->scaleAnimating = 0;
    cardWork2->positionDelay = 0;
    cardWork2->modelIds[0] = Hu3DModelCreateFile(m446CardFaceModelFiles[cardId]);
    Hu3DModelAmbSet(cardWork2->modelIds[0], 0.0f, 0.0f, 0.0f);
    Hu3DModelShadowSet(cardWork2->modelIds[0]);
    Hu3DModelLayerSet(cardWork2->modelIds[0], 0);
    cardWork2->modelIds[1] = Hu3DModelCreateFile(m446CardBackModelFiles[cardId]);
    Hu3DModelAmbSet(cardWork2->modelIds[1], 1.0f, 1.0f, 1.0f);
    Hu3DModelAttrSet(cardWork2->modelIds[1], HU3D_MOTATTR_LOOP);
    Hu3DModelLayerSet(cardWork2->modelIds[0], 0);
    M446CardFaceHide(cardWork2);
    M446CardShadowSet(cardWork2, 1);
    cardWork = M446ListNodeCreate(cardWork2);
    if (!cardWork) {
        OSReport("[!] m446CardCreate ERROR...\n");
        HuMemDirectFree(cardWork2);
        return NULL;
    }
    if (m446CardListHead) {
        M446ListAppend(m446CardListHead, cardWork);
    }
    else {
        m446CardListHead = cardWork;
    }
    return cardWork2;
}

void M446CardDestroy(M446CardWork *cardWork)
{
    M446ListNode *listNode2 = m446CardListHead;
    do {
        if (cardWork == listNode2->value) {
            if (listNode2 == m446CardListHead) {
                m446CardListHead = listNode2->next;
            }
            M446ListRemove(listNode2);
            M446ListNodeDestroy(listNode2);
            Hu3DModelKill(cardWork->modelIds[0]);
            Hu3DModelKill(cardWork->modelIds[1]);
            HuMemDirectFree(cardWork);
            return;
        }
        listNode2 = listNode2->next;
    } while (listNode2);
}

void M446CardFlipDown(M446CardWork *cardWork)
{
    Vec position;

    position = cardWork->rotation;
    position.z = 0.0f;
    cardWork->faceDown = 1;
    M446CardRotateTo(cardWork, &position, 0x1E);
}

void M446CardFlipUp(M446CardWork *cardWork)
{
    Vec position;

    position = cardWork->rotation;
    position.z = 180.0f;
    cardWork->faceDown = 0;
    M446CardRotateTo(cardWork, &position, 0x3C);
}

void M446CardMoveTo(M446CardWork *cardWork, Vec *targetPosition, s32 durationFrames)
{
    cardWork->positionDelay = 0;
    cardWork->positionTarget = *targetPosition;
    cardWork->positionStart = cardWork->position;
    cardWork->positionProgress = 0.0f;
    cardWork->positionStep = (1.0f / durationFrames);
    cardWork->positionAnimating = 1;
}

void M446CardMoveToDelayed(M446CardWork *cardWork, Vec *targetPosition, s32 durationFrames, s32 delayFrames)
{
    cardWork->positionDelay = 0;
    cardWork->positionTarget = *targetPosition;
    cardWork->positionStart = cardWork->position;
    cardWork->positionProgress = 0.0f;
    cardWork->positionStep = (1.0f / durationFrames);
    cardWork->positionAnimating = 1;
    cardWork->positionDelay = delayFrames;
}

void M446CardRotateTo(M446CardWork *cardWork, Vec *targetRotation, s32 durationFrames)
{
    cardWork->rotationTarget = *targetRotation;
    cardWork->rotationStart = cardWork->rotation;
    cardWork->rotationProgress = 0.0f;
    cardWork->rotationStep = (1.0f / durationFrames);
    cardWork->rotationAnimating = 1;
}

void M446CardScaleTo(M446CardWork *cardWork, f32 targetScale, s32 durationFrames)
{
    cardWork->scaleTarget = targetScale;
    cardWork->scaleStart = cardWork->scale;
    cardWork->scaleProgress = 0.0f;
    cardWork->scaleStep = (1.0f / durationFrames);
    cardWork->scaleAnimating = 1;
}

void M446CardFaceShow(M446CardWork *cardWork)
{
    Hu3DModelAttrReset(cardWork->modelIds[1], HU3D_ATTR_DISPOFF);
    cardWork->cardFaceVisible = 1;
}

void M446CardFaceHide(M446CardWork *cardWork)
{
    Hu3DModelAttrSet(cardWork->modelIds[1], HU3D_ATTR_DISPOFF);
    cardWork->cardFaceVisible = 0;
}

void M446CardTurnBegin(M446CardWork *cardWork)
{
    cardWork->animationState = 1;
    cardWork->animationSubstate = 0;
}

void M446CardTurnUpdate(M446CardWork *cardWork)
{
    Vec position;

    switch (cardWork->animationSubstate) {
        case 0:
            cardWork->animationSubstate = 1;
            Hu3DModelAttrReset(cardWork->modelIds[1], HU3D_ATTR_DISPOFF);
            cardWork->cardFaceVisible = 1;
            cardWork->animationSubstate = 1; // ?
        case 1:
            cardWork->scaleTarget = 2.5f;
            cardWork->scaleStart = cardWork->scale;
            cardWork->scaleProgress = 0.0f;
            cardWork->scaleStep = 0.055555556f;
            cardWork->scaleAnimating = 1;
            cardWork->animationSubstate = 2; // ?????
            break;

        case 2:
            if (cardWork->scaleAnimating != 0)
                break;
            cardWork->animationSubstate = 3; // ??????????
        case 3:
            cardWork->scaleTarget = 1.5f;
            cardWork->scaleStart = cardWork->scale;
            cardWork->scaleProgress = 0.0f;
            cardWork->scaleStep = 0.055555556f;
            cardWork->scaleAnimating = 1;
            cardWork->animationSubstate = 4;
            break;

        case 4:
            if (cardWork->scaleAnimating != 0)
                break;
            cardWork->animationSubstate = 5;
        case 5:
            cardWork->scaleTarget = 2.5f;
            cardWork->scaleStart = cardWork->scale;
            cardWork->scaleProgress = 0.0f;
            cardWork->scaleStep = 0.055555556f;
            cardWork->scaleAnimating = 1;
            cardWork->animationSubstate = 6;
            break;

        case 6:
            if (cardWork->scaleAnimating != 0)
                break;
            cardWork->animationSubstate = 8;
        case 7:
            cardWork->scaleTarget = 1.5f;
            cardWork->scaleStart = cardWork->scale;
            cardWork->scaleProgress = 0.0f;
            cardWork->scaleStep = 0.055555556f;
            cardWork->scaleAnimating = 1;
            cardWork->animationSubstate = 8;
        case 8:
            if (cardWork->scaleAnimating != 0)
                break;
            cardWork->animationSubstate = 9;
        case 9:
            position = cardWork->rotation;
            position.y = 720.0f;
            cardWork->rotationTarget = position;
            cardWork->rotationStart = cardWork->rotation;
            cardWork->rotationProgress = 0.0f;
            cardWork->rotationStep = 0.016666668f;
            cardWork->rotationAnimating = 1;
            cardWork->scaleTarget = 0.0f;
            cardWork->scaleStart = cardWork->scale;
            cardWork->scaleProgress = 0.0f;
            cardWork->scaleStep = 0.016666668f;
            cardWork->scaleAnimating = 1;
            cardWork->animationSubstate = 10;
            break;
        case 10:
            if (cardWork->scaleAnimating != 0)
                break;
            cardWork->animationState = 0;
            cardWork->animationSubstate = 0;
            Hu3DModelAttrSet(cardWork->modelIds[1], HU3D_ATTR_DISPOFF);
            cardWork->cardFaceVisible = 0;
            return;
    }
}

void M446CardShuffleBegin(M446CardWork *cardWork)
{
    cardWork->animationState = 2;
    cardWork->animationSubstate = 0;
}

void M446CardShuffleUpdate(M446CardWork *cardWork)
{
    f32 movementAngle;
    f32 movementMagnitude;

    switch (cardWork->animationSubstate) {
        case 0:
            cardWork->animationSubstate = 1;
        case 1:
            if (cardWork->scaleAnimating == 0) {
                movementAngle = cardWork->scale + 0.25f;
                cardWork->scaleTarget = movementAngle;
                cardWork->scaleStart = cardWork->scale;
                cardWork->scaleProgress = 0.0f;
                cardWork->scaleStep = 0.033333335f;
                cardWork->scaleAnimating = 1;
                cardWork->animationSubstate = 2U;
            }
            break;
        case 2:
            if (cardWork->scaleAnimating == 0) {
                movementMagnitude = cardWork->scale - 0.25f;
                cardWork->scaleTarget = movementMagnitude;
                cardWork->scaleStart = cardWork->scale;
                cardWork->scaleProgress = 0.0f;
                cardWork->scaleStep = 0.033333335f;
                cardWork->scaleAnimating = 1;
                cardWork->animationSubstate = 1;
            }
            break;
    }
}

void M446CardShadowSet(M446CardWork *cardWork, s32 shadowEnabled)
{
    if (shadowEnabled != 0) {
        Hu3DModelShadowDispOn(cardWork->modelIds[0]);
        Hu3DModelAmbSet(cardWork->modelIds[0], 0.0f, 0.0f, 0.0f);
        return;
    }
    Hu3DModelShadowDispOff(cardWork->modelIds[0]);
    Hu3DModelAmbSet(cardWork->modelIds[0], 1.0f, 1.0f, 1.0f);
}

void M446CardRenderUpdate(M446CardWork *cardWork)
{
    if ((cardWork->scale > 1.0f) || (cardWork->scale < 1.0f)) {
        Hu3DModelLayerSet(cardWork->modelIds[0], 1);
        Hu3DModelLayerSet(cardWork->modelIds[1], 1);
    }
    else {
        Hu3DModelLayerSet(cardWork->modelIds[0], 0);
        Hu3DModelLayerSet(cardWork->modelIds[1], 0);
    }
    Hu3DModelPosSet(cardWork->modelIds[0], cardWork->position.x, cardWork->position.y, cardWork->position.z);
    Hu3DModelPosSet(cardWork->modelIds[1], cardWork->position.x, 0.1f + cardWork->position.y, cardWork->position.z);
    Hu3DModelScaleSet(cardWork->modelIds[0], cardWork->scale, cardWork->scale, cardWork->scale);
    Hu3DModelScaleSet(cardWork->modelIds[1], cardWork->scale, cardWork->scale, cardWork->scale);
    Hu3DModelRotSet(cardWork->modelIds[0], cardWork->rotation.x, cardWork->rotation.y, cardWork->rotation.z);
}

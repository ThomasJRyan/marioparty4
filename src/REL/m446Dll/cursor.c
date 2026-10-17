#include "REL/m446Dll.h"

#include "game/hu3d.h"
#include "ext_math.h"

// bss
M446ListNode *m446CursorListHead;

// data
s32 m446CursorModelFiles[9] = { 0x4D0012, 0x4D0013, 0x4D0014, 0x4D0015, 0x4D0016, 0x4D0017, 0x4D0018, 0x4D0019, 0x4D001A };

s32 M446CursorInitialize(void)
{
    m446CursorListHead = NULL;
    return 1;
}

void M446CursorUpdate(void)
{
    M446ListNode *listNode;
    M446CursorWork *cursorWork2;

    listNode = m446CursorListHead;
    if (listNode) {
        do {
            cursorWork2 = listNode->value;
            M446CursorDestroy(cursorWork2);
            listNode = listNode->next;
        } while (listNode);
    }
}

void M446CursorsUpdate(void)
{
    f32 distance;
    M446ListNode *listNode;
    M446CursorWork *cursorWork2;

    listNode = m446CursorListHead;
    if (listNode) {
        do {
            cursorWork2 = listNode->value;
            if (cursorWork2->movementActive != 0) {
                distance = sind(90.0f * cursorWork2->movementProgress);
                distance *= distance;
                cursorWork2->position.x = (cursorWork2->positionStart.x + (distance * (cursorWork2->positionTarget.x - cursorWork2->positionStart.x)));
                cursorWork2->position.y = (cursorWork2->positionStart.y + (distance * (cursorWork2->positionTarget.y - cursorWork2->positionStart.y)));
                cursorWork2->position.z = (cursorWork2->positionStart.z + (distance * (cursorWork2->positionTarget.z - cursorWork2->positionStart.z)));
                cursorWork2->movementProgress += cursorWork2->movementStep;
                if (cursorWork2->movementProgress >= 1.0f) {
                    cursorWork2->position.x = cursorWork2->positionTarget.x;
                    cursorWork2->position.y = cursorWork2->positionTarget.y;
                    cursorWork2->position.z = cursorWork2->positionTarget.z;
                    cursorWork2->movementActive = 0;
                }
            }
            if ((cursorWork2->listNode == 0) && (cursorWork2->selectionAnimationActive != 0) && (Hu3DMotionEndCheck(cursorWork2->modelIds[1]) != 0)) {
                M446CursorActivate(cursorWork2);
            }
            M446CursorDeselect(cursorWork2);
            listNode = listNode->next;
        } while (listNode);
    }
}

M446CursorWork *M446CursorCreate(s32 initialPlayerIndex)
{
    s32 playerIndex;
    M446CursorWork *cursorWork2;
    M446ListNode *listNode3;

    cursorWork2 = HuMemDirectMallocNum(HEAP_HEAP, 0x54, HU_MEMNUM_OVL);
    if (!cursorWork2) {
        return NULL;
    }
    cursorWork2->cursorState = 0;
    cursorWork2->position.x = 0.0f;
    cursorWork2->position.y = 72.0f;
    cursorWork2->position.z = 0.0f;
    cursorWork2->scale = 1.0f;
    cursorWork2->movementActive = 0;
    cursorWork2->reverseDirection = 0;
    cursorWork2->selectionAnimationActive = 0;
    cursorWork2->playerIndex = initialPlayerIndex;
    cursorWork2->controllerPort = 0;
    cursorWork2->modelIds[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M446, 0x09));
    Hu3DModelLayerSet(cursorWork2->modelIds[0], 6);
    if (cursorWork2->playerIndex == 0) {
        for (playerIndex = 0; playerIndex < 9; playerIndex++) {
            cursorWork2->modelIds[playerIndex + 1] = Hu3DModelCreateFile(m446CursorModelFiles[playerIndex]);
            Hu3DModelLayerSet(cursorWork2->modelIds[playerIndex + 1], 7);
        }
    }
    M446CursorPlayerSet(cursorWork2, 0);
    listNode3 = M446ListNodeCreate(cursorWork2);
    if (!listNode3) {
        OSReport("[!] m446CursorCreate ERROR...\n");
        HuMemDirectFree(cursorWork2);
        return NULL;
    }
    if (m446CursorListHead) {
        M446ListAppend(m446CursorListHead, listNode3);
    }
    else {
        m446CursorListHead = listNode3;
    }
    return cursorWork2;
}

void M446CursorDestroy(M446CursorWork *cursorWork)
{
    s32 cardIndex;
    M446ListNode *listNode2;

    listNode2 = m446CursorListHead;
    if (listNode2) {
        do {
            if (cursorWork == listNode2->value) {
                if (listNode2 == m446CursorListHead) {
                    m446CursorListHead = listNode2->next;
                }
                M446ListRemove(listNode2);
                M446ListNodeDestroy(listNode2);
                Hu3DModelKill(cursorWork->modelIds[0]);
                if (cursorWork->playerIndex == 0) {
                    for (cardIndex = 0; cardIndex < 9; cardIndex++) {
                        Hu3DModelKill(cursorWork->modelIds[cardIndex + 1]);
                    }
                }
                HuMemDirectFree(cursorWork);
                return;
            }
            listNode2 = listNode2->next;
        } while (listNode2);
    }
}

void M446CursorPlayerSet(M446CursorWork *cursorWork, u8 cursorPlayer)
{
    s32 playerIndex;

    cursorWork->cursorShown = 1;
    cursorWork->controllerPort = cursorPlayer;
    Hu3DModelAttrReset(cursorWork->modelIds[0], HU3D_ATTR_DISPOFF);
    if (cursorWork->playerIndex == 0) {
        for (playerIndex = 0; playerIndex < 9; playerIndex++) {
            if (playerIndex == cursorPlayer) {
                Hu3DModelAttrReset(cursorWork->modelIds[playerIndex + 1], HU3D_ATTR_DISPOFF);
            }
            else {
                Hu3DModelAttrSet(cursorWork->modelIds[playerIndex + 1], HU3D_ATTR_DISPOFF);
            }
        }
    }
}

void M446CursorActivate(M446CursorWork *cursorWork)
{
    s32 playerIndex;

    cursorWork->cursorShown = 0;
    Hu3DModelAttrSet(cursorWork->modelIds[0], HU3D_ATTR_DISPOFF);
    if (cursorWork->playerIndex == 0) {
        for (playerIndex = 0; playerIndex < 9; playerIndex++) {
            Hu3DModelAttrSet(cursorWork->modelIds[playerIndex + 1], HU3D_ATTR_DISPOFF);
        }
    }
}

void M446CursorMoveTo(M446CursorWork *cursorWork, Vec *targetPosition, s32 durationFrames)
{
    cursorWork->positionTarget = *targetPosition;
    cursorWork->positionStart = cursorWork->position;
    cursorWork->movementProgress = 0.0f;
    cursorWork->movementStep = (1.0f / durationFrames);
    cursorWork->movementActive = 1;
}

void M446CursorUpdatePosition(M446CursorWork *cursorWork)
{
    s32 playerIndex;

    if (cursorWork->playerIndex == 0) {
        for (playerIndex = 0; playerIndex < 9; playerIndex++) {
            Hu3DModelAttrReset(cursorWork->modelIds[playerIndex + 1], HU3D_MOTATTR_PAUSE);
            Hu3DMotionTimeSet(cursorWork->modelIds[playerIndex + 1], 0.0f);
        }
    }
    cursorWork->selectionAnimationActive = 1;
}

void M446CursorSelect(M446CursorWork *cursorWork)
{
    s32 loopIndex;

    if (cursorWork->playerIndex == 0) {
        for (loopIndex = 0; loopIndex < 9; loopIndex++) {
            Hu3DModelAttrSet(cursorWork->modelIds[loopIndex + 1], HU3D_MOTATTR_PAUSE);
        }
    }
    cursorWork->selectionAnimationActive = 0;
}

void M446CursorDeselect(M446CursorWork *cursorWork)
{
    f32 movementAngle;
    s32 cardId;
    s32 playerIndex;

    Hu3DModelPosSet(cursorWork->modelIds[0], cursorWork->position.x, cursorWork->position.y, cursorWork->position.z);
    Hu3DModelScaleSet(cursorWork->modelIds[0], cursorWork->scale, cursorWork->scale, cursorWork->scale);
    if (cursorWork->playerIndex == 0) {
        for (playerIndex = 0; playerIndex < 9; playerIndex++) {
            cardId = playerIndex + 1;
            Hu3DModelPosSet(cursorWork->modelIds[cardId], cursorWork->position.x, 0.1f + cursorWork->position.y, cursorWork->position.z);
            Hu3DModelScaleSet(cursorWork->modelIds[cardId], cursorWork->scale, cursorWork->scale, cursorWork->scale);
            if (cursorWork->reverseDirection != 0) {
                movementAngle = 180.0f;
            }
            else {
                movementAngle = 0.0f;
            }
            Hu3DModelRotSet(cursorWork->modelIds[cardId], 0.0f, movementAngle, 0.0f);
        }
    }
}

#include "REL/m446Dll.h"

#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "ext_math.h"

// cursor.c
extern M446CursorWork *M446CursorCreate(s32 initialPlayerIndex);
extern void M446CursorDestroy(M446CursorWork *cursorWork);
extern void M446CursorPlayerSet(M446CursorWork *cursorWork, u8 cursorPlayer);

// bss
M446ListNode *m446PlayerListHead;

// data
s32 m446PlayerModelFiles[9] = {
    DATA_MAKE_NUM(DATADIR_M446, 10),
    DATA_MAKE_NUM(DATADIR_M446, 11),
    DATA_MAKE_NUM(DATADIR_M446, 12),
    DATA_MAKE_NUM(DATADIR_M446, 13),
    DATA_MAKE_NUM(DATADIR_M446, 14),
    DATA_MAKE_NUM(DATADIR_M446, 15),
    DATA_MAKE_NUM(DATADIR_M446, 16),
    DATA_MAKE_NUM(DATADIR_M446, 17),
    DATA_MAKE_NUM(DATADIR_M446, 33),
};

s32 m446PlayerMotionFiles[9][4] = {
    {
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 54),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 23),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 114),
    },
    {
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 54),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 23),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 114),
    },
    {
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 54),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 23),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 114),
    },
    {
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 54),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 23),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 114),
    },
    {
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 54),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 23),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 114),
    },
    {
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 54),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 23),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 114),
    },
    {
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 54),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 23),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 114),
    },
    {
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 54),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 23),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 114),
    },
    {
        DATA_MAKE_NUM(DATADIR_M446, 0x22),
        DATA_MAKE_NUM(DATADIR_M446, 0x23),
        DATA_MAKE_NUM(DATADIR_M446, 0x25),
        DATA_MAKE_NUM(DATADIR_M446, 0x24),
    },
};
M446PlayerActionCallback m446PlayerStateUpdates[5] = { NULL, M446PlayerIdleUpdate, M446PlayerLookUpdate, M446PlayerMoveUpdate, M446PlayerCardReaction };

s32 M446PlayersInitialize(void)
{
    m446PlayerListHead = 0;
    return 1;
}

void M446PlayersDestroy(void)
{
    M446ListNode *listNode;
    void *playerIndex;

    listNode = m446PlayerListHead;
    if (listNode) {
        do {
            playerIndex = listNode->value;
            M446PlayerDestroy(playerIndex);
            listNode = listNode->next;
        } while (listNode);
    }
}

void M446PlayersUpdate(void)
{
    f32 distance;
    M446PlayerWork *playerWork2;
    M446ListNode *listNode;

    listNode = m446PlayerListHead;
    if (listNode) {
        do {
            playerWork2 = listNode->value;
            if (m446PlayerStateUpdates[playerWork2->actionState]) {
                m446PlayerStateUpdates[playerWork2->actionState](playerWork2);
            }
            if (playerWork2->rotationAnimating != 0) {
                distance = sind(90.0f * playerWork2->rotationProgress);
                distance *= distance;
                playerWork2->position.x = (playerWork2->rotationStart.x + (distance * (playerWork2->rotationTarget.x - playerWork2->rotationStart.x)));
                playerWork2->position.y = (playerWork2->rotationStart.y + (distance * (playerWork2->rotationTarget.y - playerWork2->rotationStart.y)));
                playerWork2->position.z = (playerWork2->rotationStart.z + (distance * (playerWork2->rotationTarget.z - playerWork2->rotationStart.z)));
                playerWork2->rotationProgress += playerWork2->rotationStep;
                if (playerWork2->rotationProgress >= 1.0f) {
                    playerWork2->position.x = playerWork2->rotationTarget.x;
                    playerWork2->position.y = playerWork2->rotationTarget.y;
                    playerWork2->position.z = playerWork2->rotationTarget.z;
                    playerWork2->rotationAnimating = 0;
                }
            }
            if (playerWork2->positionAnimating != 0) {
                distance = sind(90.0f * playerWork2->positionProgress);
                distance *= distance;
                playerWork2->rotation.x = (playerWork2->positionStart.x + (distance * (playerWork2->positionTarget.x - playerWork2->positionStart.x)));
                playerWork2->rotation.y = (playerWork2->positionStart.y + (distance * (playerWork2->positionTarget.y - playerWork2->positionStart.y)));
                playerWork2->rotation.z = (playerWork2->positionStart.z + (distance * (playerWork2->positionTarget.z - playerWork2->positionStart.z)));
                playerWork2->positionProgress = (playerWork2->positionProgress + playerWork2->positionStep);
                if (playerWork2->positionProgress >= 1.0f) {
                    playerWork2->rotation.x = playerWork2->positionTarget.x;
                    playerWork2->rotation.y = playerWork2->positionTarget.y;
                    playerWork2->rotation.z = playerWork2->positionTarget.z;
                    playerWork2->positionAnimating = 0;
                }
            }
            M446PlayerRenderUpdate(playerWork2);
            listNode = listNode->next;
        } while (listNode);
    }
}

M446PlayerWork *M446PlayerCreate(s32 configuredPlayerId, M446TableWork *tableWork)
{
    f32 movementAngle;
    s32 stateIndex;
    s32 counter;
    s16 cardCount;
    s16 cardId;
    M446ListNode *listNode3;
    s32 playerIndex;
    M446PlayerWork *playerWork2;

    playerWork2 = HuMemDirectMallocNum(HEAP_HEAP, 0xCC, HU_MEMNUM_OVL);
    if (!playerWork2) {
        return NULL;
    }
    playerWork2->playerIndex = configuredPlayerId;
    if (GWPlayerCfg[playerIndex].iscom != 0) {
        stateIndex = 1;
    }
    else {
        stateIndex = 0;
    }
    playerWork2->isComputer = stateIndex;
    playerWork2->tableWork = tableWork;
    if (GWPlayerCfg[playerIndex].character == 0xB) {
        counter = 1;
    }
    else {
        counter = 0;
    }
    playerWork2->usesCustomModel = counter;
    if (playerWork2->usesCustomModel != 0) {
        cardCount = 8;
    }
    else {
        cardCount = GWPlayerCfg[playerIndex].character;
    }
    playerWork2->characterId = cardCount;
    if (playerWork2->isComputer != 0) {
        cardId = 4;
    }
    else {
        cardId = GWPlayerCfg[playerIndex].pad_idx;
    }
    playerWork2->controllerPort = cardId;
    playerWork2->difficulty = GWPlayerCfg[playerIndex].diff;
    playerWork2->selectionActive = 0;
    playerWork2->handCardCount = 0;
    playerWork2->selectedCardIndex = 2;
    playerWork2->position.x = 0.0f;
    if (playerWork2->usesCustomModel != 0) {
        movementAngle = 70.0f;
    }
    else {
        movementAngle = 0.0f;
    }
    playerWork2->position.y = movementAngle;
    playerWork2->position.z = 0.0f;
    playerWork2->rotation.x = 0.0f;
    playerWork2->rotation.y = 0.0f;
    playerWork2->rotation.z = 0.0f;
    playerWork2->modelScale = 1.0f;
    playerWork2->handCount = 0;
    playerWork2->cardFlippedState = 0;
    playerWork2->selectedHandIndex = 0;
    playerWork2->actionState = 0;
    playerWork2->actionSubstate = 0;
    playerWork2->positionAnimating = 0;
    playerWork2->rotationAnimating = 0;

    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        playerWork2->handCards[playerIndex] = NULL;
    }
    playerWork2->cursorWork = M446CursorCreate(1);
    if (!playerWork2->cursorWork) {
        HuMemDirectFree(playerWork2);
        return NULL;
    }
    M446CursorActivate(playerWork2->cursorWork);
    switch (playerWork2->playerIndex) {
        case 0:
            playerWork2->position.z = 150.0f;
            playerWork2->rotation.y = 180.0f;
            break;
        case 1:
            playerWork2->position.z = -150.0f;
            playerWork2->rotation.y = 0.0f;
            break;
    }
    if (playerWork2->usesCustomModel != 0) {
        playerWork2->modelIds[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M446, 0x20));
        playerWork2->modelIds[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M446, 0x21));
    }
    else {
        playerWork2->modelIds[0] = CharModelCreate(playerWork2->characterId, 2);
        playerWork2->modelIds[1] = Hu3DModelCreateFile(m446PlayerModelFiles[playerWork2->characterId]);
    }
    Hu3DModelAmbSet(playerWork2->modelIds[0], 0.0f, 0.0f, 0.0f);
    Hu3DModelAmbSet(playerWork2->modelIds[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelShadowSet(playerWork2->modelIds[0]);
    Hu3DModelShadowSet(playerWork2->modelIds[1]);
    Hu3DModelPosSet(playerWork2->modelIds[1], 0.0f, 100.0f, 0.0f);
    Hu3DModelAttrReset(playerWork2->modelIds[1], HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(playerWork2->modelIds[1], HU3D_ATTR_DISPOFF);
    Hu3DModelPosSet(playerWork2->modelIds[1], 2000.0f, 2000.0f, 2000.0f);
    M446PlayerAnimationSet(playerWork2, 0);

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerWork2->usesCustomModel != 0) {
            playerWork2->motionIds[playerIndex] = Hu3DJointMotionFile(playerWork2->modelIds[0], m446PlayerMotionFiles[playerWork2->characterId][playerIndex]);
        }
        else {
            playerWork2->motionIds[playerIndex] = CharMotionCreate(playerWork2->characterId, m446PlayerMotionFiles[playerWork2->characterId][playerIndex]);
        }
    }
    if (playerWork2->usesCustomModel == 0) {
        CharMotionDataClose(playerWork2->characterId);
    }
    if (playerWork2->usesCustomModel != 0) {
        Hu3DMotionSet(playerWork2->modelIds[0], playerWork2->motionIds[0]);
    }
    else {
        CharMotionSet(playerWork2->characterId, playerWork2->motionIds[0]);
    }
    Hu3DModelAttrSet(playerWork2->modelIds[0], HU3D_MOTATTR_LOOP);
    listNode3 = M446ListNodeCreate(playerWork2);
    if (!listNode3) {
        OSReport("[!] m446PlayerCreate ERROR...\n");
        HuMemDirectFree(playerWork2);
        return NULL;
    }
    if (m446PlayerListHead) {
        M446ListAppend(m446PlayerListHead, listNode3);
    }
    else {
        m446PlayerListHead = listNode3;
    }
    return playerWork2;
}

void M446PlayerDestroy(M446PlayerWork *playerWork)
{
    s32 cardIndex;
    M446ListNode *listNode2;

    listNode2 = m446PlayerListHead;

    do {
        if (playerWork == listNode2->value) {
            if (listNode2 == m446PlayerListHead) {
                m446PlayerListHead = listNode2->next;
            }
            M446ListRemove(listNode2);
            M446ListNodeDestroy(listNode2);

            for (cardIndex = 0; cardIndex < 4; cardIndex++) {
                if (playerWork->usesCustomModel != 0) {
                    Hu3DMotionKill(playerWork->motionIds[cardIndex]);
                }
                else {
                    CharMotionKill(playerWork->characterId, playerWork->motionIds[cardIndex]);
                }
            }
            if (playerWork->usesCustomModel != 0) {
                Hu3DModelKill(playerWork->modelIds[0]);
                Hu3DModelKill(playerWork->modelIds[1]);
            }
            else {
                CharModelKill(playerWork->characterId);
                Hu3DModelKill(playerWork->modelIds[1]);
            }
            M446CursorDestroy(playerWork->cursorWork);
            HuMemDirectFree(playerWork);
            break;
        }
        listNode2 = listNode2->next;
    } while (listNode2);
}

s32 M446PlayerAppendCard(M446PlayerWork *playerWork, M446CardWork *cardWork, s32 cardPlacementMode)
{
    Vec offset;
    Vec targetPosition;
    s32 result2;
    s32 deckEntry;
    s32 cursorWork;
    s32 cardIndex;

    if (playerWork->handCount >= 3) {
        OSReport("m446PlayerAppendCard ERROR...\n");
        return 0;
    }

    for (cardIndex = 0; cardIndex < 3; cardIndex++) {
        if (!playerWork->handCards[cardIndex]) {
            break;
        }
    }

    if (cardIndex == 3) {
        OSReport("m446PlayerAppendCard ERROR...\n");
        return 0;
    }
    playerWork->handCards[cardIndex] = cardWork;
    playerWork->handCount++;
    offset.y = 86.0f;
    switch (playerWork->playerIndex) {
        case 0:
            offset.x = -30.0f + (30.0f * cardIndex);
            offset.z = 45.0f;
            break;
        case 1:
            offset.x = 30.0f - (30.0f * cardIndex);
            offset.z = -45.0f;
            break;
    }

    targetPosition = cardWork->rotation;
    switch (playerWork->playerIndex) {
        case 0:
            targetPosition.y = 0.0f;
            break;
        case 1:
            targetPosition.y = 180.0f;
            break;
    }
    switch (cardPlacementMode) {
        case 0:
            cardWork->position = offset;
            cardWork->rotation = targetPosition;
            cardWork->scale = 1.5f;
            M446CardFlipDown(cardWork);
            break;
        case 1:
            M446CardMoveTo(cardWork, &offset, 0x3C);
            M446CardRotateTo(cardWork, &targetPosition, 0x1E);
            M446CardScaleTo(cardWork, 1.5f, 0x1E);
            break;
        case 4:
            M446CardMoveTo(cardWork, &offset, 0x1E);
            M446CardRotateTo(cardWork, &targetPosition, 0x1E);
            M446CardScaleTo(cardWork, 1.5f, 0x1E);
            break;
    }
    deckEntry = playerWork->cardFlippedState;
    cursorWork = playerWork->cardMovementState;
    M446PlayerShadowUpdate(playerWork);
    if ((playerWork->cardFlippedState != 0) && (deckEntry == 0)) {
        HuAudFXPlay(0x76B);
        M446PlayerMotionSet(playerWork, 1, 0x3C);
    }
    else if ((playerWork->cardMovementState != 0) && (cursorWork == 0)) {
        HuAudFXPlay(0x76B);
        M446PlayerMotionSet(playerWork, 0, 0x3C);
    }
    return 1;
}

s32 M446PlayerRemoveCard(M446PlayerWork *playerWork, M446CardWork *cardWork, s32 cardRemovalMode)
{
    Vec targetPosition;
    s32 playerIndex;

    if (playerWork->handCount <= 0) {
        OSReport("m446PlayerRemoveCard ERROR...\n");
        return 0;
    }

    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        if (playerWork->handCards[playerIndex] == cardWork) {
            break;
        }
    }
    if (playerIndex == 3) {
        OSReport("m446PlayerRemoveCard ERROR...\n");
        return 0;
    }
    playerWork->handCards[playerIndex] = NULL;
    playerWork->handCount--;
    M446CardFaceHide(cardWork);
    M446PlayerShadowUpdate(playerWork);
    switch (cardRemovalMode) {
        case 3:
            targetPosition = cardWork->position;
            targetPosition.y = 86.0f;
            targetPosition.z = 0.0f;
            M446CardMoveToDelayed(cardWork, &targetPosition, 0x1E, 0x21);
            M446PlayerEmotionSet(playerWork, playerIndex);
            break;
    }
    return 1;
}

s32 M446PlayerCardFind(M446PlayerWork *playerWork, s32 cardIndex, M446CardWork **cardOut)
{
    if ((cardIndex < 0) || (cardIndex >= 3)) {
        return 0;
    }
    *cardOut = playerWork->handCards[cardIndex];
    return 1;
}

s32 M446PlayerCardSelect(M446PlayerWork *playerWork, M446CardWork **cardOut, s32 selectionMode)
{
    s32 rotation[3];
    Vec position;
    s32 modelIndex;
    s32 playerIndex;
    s32 cardCount;
    s32 cardId;
    s32 stateIndex;
    s32 counter;
    M446CardWork *cardWork;

    stateIndex = 0;
    counter = 0;
    cardCount = 0;
    *cardOut = NULL;
    if (playerWork->isComputer != 0) {
        if (playerWork->selectionActive == 0) {
            if (playerWork->cardMovementState != 0) {
                cardId = M446TableCardCountByOwner(playerWork->tableWork, playerWork->targetCardId);
                if (cardId >= 1) {
                    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
                        cardWork = playerWork->handCards[playerIndex];
                        if (cardWork->cardId != playerWork->targetCardId) {
                            playerWork->selectedHandIndex = playerIndex;
                            break;
                        }
                    }
                }
                else {
                    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
                        cardWork = playerWork->handCards[playerIndex];
                        if (cardWork->cardId != playerWork->targetCardId) {
                            break;
                        }
                    }

                    cardId = M446TableCardCountByOwner(playerWork->tableWork, cardWork->cardId);
                    if (cardId >= 2) {
                        for (playerIndex = 0; playerIndex < 3; playerIndex++) {
                            cardWork = playerWork->handCards[playerIndex];
                            if (cardWork->cardId == playerWork->targetCardId) {
                                playerWork->selectedHandIndex = playerIndex;
                                break;
                            }
                        }
                    }
                    else {
                        playerWork->selectedHandIndex = (M446RandomGet() % 3);
                    }
                }
            }
            else {
                for (playerIndex = 0; playerIndex < 3; playerIndex++) {
                    cardWork = playerWork->handCards[playerIndex];
                    rotation[playerIndex] = M446TableCardCountByOwner(playerWork->tableWork, cardWork->cardId);
                }
                for (modelIndex = 0; modelIndex < 5; modelIndex++) {
                    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
                        if (modelIndex == rotation[playerIndex]) {
                            break;
                        }
                    }
                    if (playerIndex < 3) {
                        break;
                    }
                }
                if (modelIndex == 5) {
                    playerWork->selectedHandIndex = (M446RandomGet() % 3);
                }
                else {
                    playerWork->selectedHandIndex = playerIndex;
                }
            }
            playerWork->selectionActive = 1;
            playerWork->selectionDelay = 0x1E;
        }
        if (playerWork->selectionDelay > 0) {
            playerWork->selectionDelay--;
        }
        else {
            stateIndex = playerWork->selectedHandIndex < playerWork->selectedCardIndex;
            counter = playerWork->selectedHandIndex > playerWork->selectedCardIndex;
            cardCount = playerWork->selectedHandIndex == playerWork->selectedCardIndex;
            playerWork->selectionDelay = 0x12;
        }
    }
    else {
        switch (playerWork->playerIndex) {
            case 0:
                stateIndex = (HuPadDStkRep[playerWork->controllerPort] & 1) != 0;
                counter = (HuPadDStkRep[playerWork->controllerPort] & 2) != 0;
                break;
            case 1:
                stateIndex = (HuPadDStkRep[playerWork->controllerPort] & 2) != 0;
                counter = (HuPadDStkRep[playerWork->controllerPort] & 1) != 0;
                break;
        }
        cardCount = (HuPadBtnDown[playerWork->controllerPort] & 0x100) != 0;
    }
    if (selectionMode == 0) {
        if (stateIndex != 0) {
            if (--playerWork->selectedCardIndex < 0) {
                playerWork->selectedCardIndex = 2;
            }
            HuAudFXPlay(0);
        }
        if (counter != 0) {
            if (++playerWork->selectedCardIndex > 2) {
                playerWork->selectedCardIndex = 0;
            }
            HuAudFXPlay(0);
        }
    }
    cardWork = playerWork->handCards[playerWork->selectedCardIndex];
    if (!cardWork) {
        return 0;
    }
    if (playerWork->cursorWork->cursorShown == 0) {
        M446CursorPlayerSet(playerWork->cursorWork, playerWork->characterId);
    }
    position = cardWork->position;
    position.y++;
    playerWork->cursorWork->position = position;
    playerWork->cursorWork->scale = 1.5f;
    if ((cardCount != 0) || (selectionMode != 0)) {
        HuAudFXPlay(0x769);
        *cardOut = cardWork;
        M446CursorActivate(playerWork->cursorWork);
        playerWork->selectionActive = 0;
        return 1;
    }
    return 0;
}

void M446PlayerIdleBegin(M446PlayerWork *playerWork)
{
    playerWork->actionState = 1;
    playerWork->actionSubstate = 0;
}

static const s32 m446PlayerMotionIndices[4] = { 0, 1, 2, 3 };

void M446PlayerIdleUpdate(M446PlayerWork *playerWork)
{
    switch (playerWork->actionSubstate) {
        case 0:
            if (playerWork->usesCustomModel != 0) {
                Hu3DMotionShiftSet(playerWork->modelIds[0], playerWork->motionIds[2], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            }
            else {
                CharMotionShiftSet(playerWork->characterId, playerWork->motionIds[2], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            }
            playerWork->actionSubstate = 1;
            break;
        case 1:
            if (Hu3DMotionEndCheck(playerWork->modelIds[0]) != 0) {
                playerWork->actionState = 0;
            }
    }
}

void M446PlayerLookBegin(M446PlayerWork *playerWork)
{
    playerWork->actionState = 2;
    playerWork->actionSubstate = 0;
}

void M446PlayerLookUpdate(M446PlayerWork *playerWork)
{
    switch (playerWork->actionSubstate) {
        case 0:
            if (playerWork->usesCustomModel != 0) {
                Hu3DMotionShiftSet(playerWork->modelIds[0], playerWork->motionIds[3], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            }
            else {
                CharMotionShiftSet(playerWork->characterId, playerWork->motionIds[3], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            }
            playerWork->actionSubstate = 1;
            break;
        case 1:
            if (Hu3DMotionEndCheck(playerWork->modelIds[0]) != 0) {
                playerWork->actionState = 0;
            }
    }
}

void M446PlayerMoveBegin(M446PlayerWork *playerWork)
{
    playerWork->actionState = 3;
    playerWork->actionSubstate = 0;
}

void M446PlayerMoveUpdate(M446PlayerWork *playerWork)
{
    Vec position;

    position = playerWork->rotation;
    switch (playerWork->actionSubstate) {
        case 0:
            Hu3DModelAmbSet(playerWork->modelIds[0], 0.8f, 0.8f, 0.8f);
            if (playerWork->usesCustomModel != 0) {
                Hu3DMotionShiftSet(playerWork->modelIds[0], playerWork->motionIds[1], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            }
            else {
                CharMotionShiftSet(playerWork->characterId, playerWork->motionIds[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
                Hu3DMotionStartEndSet(playerWork->modelIds[0], 0.0f, 59.0f);
            }
            position.y = 0.0f;
            M446PlayerPositionMove(playerWork, &position, 0x3C);
            playerWork->actionSubstate = 1;
            playerWork->actionTimer = 0;
            return;
        case 1:
            if (playerWork->positionAnimating == 0) {
                playerWork->actionState = 0;
            }
    }
}

void M446PlayerEmotionSet(M446PlayerWork *playerWork, s32 emotionId)
{
    Vec rotation;
    Vec position;
    f32 movementAngle;
    f32 movementMagnitude;
    f32 positionYOffset;

    if (playerWork->usesCustomModel != 0) {
        movementAngle = 95.0f;
    }
    else {
        movementAngle = 80.0f;
    }
    rotation.y = movementAngle;

    switch (playerWork->playerIndex) {
        case 0:
            rotation.x = -30.0f + (30.0f * emotionId);
            rotation.z = 105.0f;
            position.x = 0.0f;
            position.y = 90.0f;
            position.z = 0.0f;
            break;
        case 1:
            rotation.x = 30.0f - (30.0f * emotionId);
            if (playerWork->usesCustomModel != 0) {
                movementMagnitude = 30.0f;
            }
            else {
                movementMagnitude = 55.0f;
            }
            rotation.z = -50.0f - movementMagnitude;
            position.x = 0.0f;
            if (playerWork->usesCustomModel != 0) {
                positionYOffset = 180.0f;
            }
            else {
                positionYOffset = -90.0f;
            }
            position.y = positionYOffset;
            position.z = 0.0f;
            break;
    }
    playerWork->position = rotation;
    playerWork->rotation = position;
    playerWork->actionState = 4;
    playerWork->actionSubstate = 0;
}

void M446PlayerCardReaction(M446PlayerWork *playerWork)
{
    Vec position;
    f32 movementAngle;
    f32 movementMagnitude;

    switch (playerWork->actionSubstate) {
        case 0:
            playerWork->actionSubstate = 1;
            playerWork->actionTimer = 0;
        case 1:
            position = playerWork->position;
            if (playerWork->playerIndex == 0) {
                movementAngle = -25.0f;
            }
            else {
                movementAngle = 25.0f;
            }
            position.z += movementAngle;
            M446PlayerRotationMove(playerWork, &position, 0x1E);

            playerWork->actionSubstate = 2;
            playerWork->actionTimer = 0;
            break;
        case 2:
            if (playerWork->actionTimer++ < 0x1E)
                break;
            playerWork->actionSubstate = 3;
        case 3:
            Hu3DModelAttrSet(playerWork->modelIds[1], HU3D_MOTATTR_LOOP);
            playerWork->actionSubstate = 4;
            playerWork->actionTimer = 0;
            break;
        case 4:
            if (playerWork->actionTimer == 1) {
                Hu3DModelAttrReset(playerWork->modelIds[1], HU3D_MOTATTR_LOOP);
            }
            if (playerWork->actionTimer++ < 0x1E)
                break;
            playerWork->actionSubstate = 5;
        case 5:
            position = playerWork->position;
            if (playerWork->playerIndex == 0) {
                movementMagnitude = -50.0f;
            }
            else {
                movementMagnitude = 50.0f;
            }
            position.z -= movementMagnitude;
            M446PlayerRotationMove(playerWork, &position, 12);
            playerWork->actionSubstate = 6;
            playerWork->actionTimer = 0;
            break;
        case 6:
            if (playerWork->actionTimer++ >= 12) {
                playerWork->actionState = 0;
            }
            break;
    }
}

void M446PlayerPositionMove(M446PlayerWork *playerWork, Vec *targetPosition, s32 durationFrames)
{
    playerWork->positionTarget = *targetPosition;
    playerWork->positionStart = playerWork->rotation;
    playerWork->positionProgress = 0.0f;
    playerWork->positionStep = (1.0f / durationFrames);
    playerWork->positionAnimating = 1;
}

void M446PlayerRotationMove(M446PlayerWork *playerWork, Vec *targetRotation, s32 durationFrames)
{
    playerWork->rotationTarget = *targetRotation;
    playerWork->rotationStart = playerWork->position;
    playerWork->rotationProgress = 0.0f;
    playerWork->rotationStep = (1.0f / durationFrames);
    playerWork->rotationAnimating = 1;
}

void M446PlayerAnimationSet(M446PlayerWork *playerWork, s32 animationId)
{
    f32 movementAngle;

    playerWork->animationId = animationId;
    switch (animationId) {
        case 0:
            Hu3DModelAttrReset(playerWork->modelIds[0], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(playerWork->modelIds[0], HU3D_MOTATTR_LOOP);
            Hu3DModelAttrSet(playerWork->modelIds[1], HU3D_ATTR_DISPOFF);
            playerWork->position.x = 0.0f;
            if (playerWork->usesCustomModel != 0) {
                movementAngle = 70.0f;
            }
            else {
                movementAngle = 0.0f;
            }
            playerWork->position.y = movementAngle;
            switch (playerWork->playerIndex) {
                case 0:
                    playerWork->position.z = 150.0f;
                    playerWork->rotation.y = 180.0f;
                    return;
                case 1:
                    playerWork->position.z = -150.0f;
                    playerWork->rotation.y = 0.0f;
                    return;
            }
            break;
        case 1:
            Hu3DModelAttrReset(playerWork->modelIds[1], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(playerWork->modelIds[0], HU3D_ATTR_DISPOFF);
            break;
    }
}

void M446PlayerMotionSet(M446PlayerWork *playerWork, s32 motionId, s32 vibrationStrength)
{
    if (playerWork->usesCustomModel == 0) {
        switch (motionId) {
            case 0:
                omVibrate(playerWork->playerIndex, vibrationStrength, 6, 6);
                return;
            case 1:
                omVibrate(playerWork->playerIndex, vibrationStrength, 4, 2);
                return;
            case 2:
                omVibrate(playerWork->playerIndex, vibrationStrength, 12, 0);
                break;
        }
    }
}

static inline s32 M446PlayerMotionIsActive(M446PlayerWork *playerWork)
{
    if (playerWork->animationId == 0) {
        return 0;
    }
    else {
        return 1;
    }
}

void M446PlayerRenderUpdate(M446PlayerWork *playerWork)
{
    s32 playerIndex;

    playerIndex = M446PlayerMotionIsActive(playerWork);
    Hu3DModelPosSet(playerWork->modelIds[playerIndex], playerWork->position.x, playerWork->position.y, playerWork->position.z);
    Hu3DModelRotSet(playerWork->modelIds[playerIndex], playerWork->rotation.x, playerWork->rotation.y, playerWork->rotation.z);
    Hu3DModelScaleSet(playerWork->modelIds[playerIndex], playerWork->modelScale, playerWork->modelScale, playerWork->modelScale);
}

void M446PlayerShadowUpdate(M446PlayerWork *playerWork)
{
    s32 counter;
    M446CardWork *cursorWork;
    M446CardWork *cardWork;
    s32 cardIndex;
    s32 modelIndex;

    playerWork->cardMovementState = 0;
    playerWork->cardFlippedState = 0;

    for (cardIndex = 0; cardIndex < 3; cardIndex++) {
        cardWork = playerWork->handCards[cardIndex];
        if (cardWork) {
            M446CardFaceHide(cardWork);
        }
    }
    for (cardIndex = 0; cardIndex < 3; cardIndex++) {
        counter = 0;
        cardWork = playerWork->handCards[cardIndex];
        if (cardWork) {

            for (modelIndex = 0; modelIndex < 3; modelIndex++) {
                cursorWork = playerWork->handCards[modelIndex];
                if ((cardIndex != modelIndex) && (cursorWork) && (cardWork->cardId == cursorWork->cardId)) {
                    counter++;
                }
            }
            if (counter > 0) {
                playerWork->targetCardId = cardWork->cardId;
                if (counter == 1) {
                    playerWork->cardMovementState = 1;
                }
                else if (counter == 2) {
                    playerWork->cardFlippedState = 1;
                }

                for (modelIndex = 0; modelIndex < 3; modelIndex++) {
                    cursorWork = playerWork->handCards[modelIndex];
                    if ((cursorWork) && (cardWork->cardId == cursorWork->cardId)) {
                        M446CardFaceShow(cursorWork);
                    }
                }
                break;
            }
        }
    }
}

#include "REL/m447dll.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"
#include "game/esprite.h"
#include "game/gamework_data.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"

#include "ext_math.h"

void M447PlayerDestroy(M447PlayerState *playerState);
float M447PlayerHeightGet(M447PlayerState *playerState);
void M447PlayerProcess(void);
void M447PlayerModelCreate(M447PlayerState *playerState);
void M447PlayerMotionsCreate(M447PlayerState *playerState);
void M447PlayerSpritesCreate(M447PlayerState *playerState);
void M447PlayerModelDestroy(M447PlayerState *playerState);
void M447PlayerMotionsDestroy(M447PlayerState *playerState);
void M447PlayerSpritesDestroy(M447PlayerState *playerState);
void M447PlayerIdleUpdate(M447PlayerState *playerState);
void M447PlayerControlUpdate(M447PlayerState *playerState);
void M447PlayerMovementUpdate(M447PlayerState *playerState);
float M447AngleApproach(float currentAngle, float targetAngle, float interpolationFactor);
void M447PlayerMoveToCard(M447PlayerState *playerState);
void M447PlayerCardSelect(M447PlayerState *playerState);
void M447PlayerCardWinSequenceUpdate(M447PlayerState *playerState);
void M447PlayerCardResponse(M447PlayerState *playerState);
void M447PlayerCelebrationUpdate(M447PlayerState *playerState);
void M447PlayerReactionUpdate(M447PlayerState *playerState);
void M447PlayerAnimationUpdate(M447PlayerState *playerState);
void M447PlayerActionUpdate(M447PlayerState *playerState);
void M447PlayerFinishUpdate(M447PlayerState *playerState);
void M447PlayerUpdate(M447PlayerState *playerState);
void M447PlayerMotionSet(M447PlayerState *playerState, s32 motionIndex, s32 modelAttributes);
void M447PlayerMotionPlay(M447PlayerState *playerState, s32 motionIndex, float startTime, float transitionTime, s32 modelAttributes);
void M447PlayerPositionUpdate(M447PlayerState *playerState);
void M447PlayerCollisionUpdate(M447PlayerState *playerState);
void M447PlayerBlockCollisionResolve(M447PlayerState *playerState);
s32 M447PlayerCardIndexGet(M447PlayerState *playerState);
void M447PlayerDisplayOn(M447PlayerState *playerState);

M447PlayerState *playerStates[2];

const s32 characterMotionIds[][9]
    = { { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0), DATA_MAKE_NUM(DATADIR_MARIOMOT, 2), DATA_MAKE_NUM(DATADIR_MARIOMOT, 3),
            DATA_MAKE_NUM(DATADIR_MARIOMOT, 113), DATA_MAKE_NUM(DATADIR_MARIOMOT, 31), DATA_MAKE_NUM(DATADIR_MARIOMOT, 28),
            DATA_MAKE_NUM(DATADIR_MARIOMOT, 72), DATA_MAKE_NUM(DATADIR_MARIOMOT, 23), DATA_MAKE_NUM(DATADIR_MARIOMOT, 24) },
          { DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 2), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 3),
              DATA_MAKE_NUM(DATADIR_LUIGIMOT, 113), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 31), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 28),
              DATA_MAKE_NUM(DATADIR_LUIGIMOT, 72), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 23), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 24) },
          { DATA_MAKE_NUM(DATADIR_PEACHMOT, 0), DATA_MAKE_NUM(DATADIR_PEACHMOT, 2), DATA_MAKE_NUM(DATADIR_PEACHMOT, 3),
              DATA_MAKE_NUM(DATADIR_PEACHMOT, 113), DATA_MAKE_NUM(DATADIR_PEACHMOT, 31), DATA_MAKE_NUM(DATADIR_PEACHMOT, 28),
              DATA_MAKE_NUM(DATADIR_PEACHMOT, 72), DATA_MAKE_NUM(DATADIR_PEACHMOT, 23), DATA_MAKE_NUM(DATADIR_PEACHMOT, 24) },
          { DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0), DATA_MAKE_NUM(DATADIR_YOSHIMOT, 2), DATA_MAKE_NUM(DATADIR_YOSHIMOT, 3),
              DATA_MAKE_NUM(DATADIR_YOSHIMOT, 113), DATA_MAKE_NUM(DATADIR_YOSHIMOT, 31), DATA_MAKE_NUM(DATADIR_YOSHIMOT, 28),
              DATA_MAKE_NUM(DATADIR_YOSHIMOT, 72), DATA_MAKE_NUM(DATADIR_YOSHIMOT, 23), DATA_MAKE_NUM(DATADIR_YOSHIMOT, 24) },
          { DATA_MAKE_NUM(DATADIR_WARIOMOT, 0), DATA_MAKE_NUM(DATADIR_WARIOMOT, 2), DATA_MAKE_NUM(DATADIR_WARIOMOT, 3),
              DATA_MAKE_NUM(DATADIR_WARIOMOT, 113), DATA_MAKE_NUM(DATADIR_WARIOMOT, 31), DATA_MAKE_NUM(DATADIR_WARIOMOT, 28),
              DATA_MAKE_NUM(DATADIR_WARIOMOT, 72), DATA_MAKE_NUM(DATADIR_WARIOMOT, 23), DATA_MAKE_NUM(DATADIR_WARIOMOT, 24) },
          { DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 2), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 3),
              DATA_MAKE_NUM(DATADIR_DONKEYMOT, 113), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 31), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 28),
              DATA_MAKE_NUM(DATADIR_DONKEYMOT, 72), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 23), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 24) },
          { DATA_MAKE_NUM(DATADIR_DAISYMOT, 0), DATA_MAKE_NUM(DATADIR_DAISYMOT, 2), DATA_MAKE_NUM(DATADIR_DAISYMOT, 3),
              DATA_MAKE_NUM(DATADIR_DAISYMOT, 113), DATA_MAKE_NUM(DATADIR_DAISYMOT, 31), DATA_MAKE_NUM(DATADIR_DAISYMOT, 28),
              DATA_MAKE_NUM(DATADIR_DAISYMOT, 72), DATA_MAKE_NUM(DATADIR_DAISYMOT, 23), DATA_MAKE_NUM(DATADIR_DAISYMOT, 24) },
          { DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 2), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 3),
              DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 113), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 31), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 28),
              DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 72), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 23), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 24) },
          { DATA_MAKE_NUM(DATADIR_M447, 32), DATA_MAKE_NUM(DATADIR_M447, 33), DATA_MAKE_NUM(DATADIR_M447, 34), DATA_MAKE_NUM(DATADIR_M447, 41),
              DATA_MAKE_NUM(DATADIR_M447, 39), DATA_MAKE_NUM(DATADIR_M447, 38), DATA_MAKE_NUM(DATADIR_M447, 37), DATA_MAKE_NUM(DATADIR_M447, 36),
              DATA_MAKE_NUM(DATADIR_M447, 40) } };

const float characterHeights[] = { 150.0f, 170.0f, 188.0f, 169.0f, 164.0f, 140.0f, 180.0f, 145.0f, 150.0f };

const float playerSpawnTransforms[][6] = { { -150.0f, 0.0f, 350.0f, 0.0f, 90.0f, 0.0f }, { 150.0f, 0.0f, 350.0f, 0.0f, -90.0f, 0.0f },
    { -150.0f, 0.0f, -200.0f, 0.0f, 90.0f, 0.0f }, { 150.0f, 0.0f, -200.0f, 0.0f, -90.0f, 0.0f } };

const u8 computerSelectionWeights[][5] = { { 5, 6, 7, 8, 9 }, { 4, 5, 6, 7, 8 }, { 3, 4, 5, 6, 7 }, { 2, 3, 4, 5, 6 } };

void M447PlayerRegistryInit(void)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        playerStates[i] = NULL;
    }
}

M447PlayerState *M447PlayerCreate(s32 playerIndex)
{
    M447PlayerState *object;
    s16 modelIndex;
    s16 stateIndex;
    s32 i;

    object = HuMemDirectMallocNum(HEAP_HEAP, sizeof(*object), HU_MEMNUM_OVL);
    if (!object) {
        OSReport("M447PlayerCreate ERROR...\n");
        return NULL;
    }
    object->playerIndex = playerIndex;
    object->difficulty = GWPlayerCfg[playerIndex].diff;
    object->isComputer = GWPlayerCfg[playerIndex].iscom;
    if (object->isComputer != 0) {
        modelIndex = 4;
    }
    else {
        modelIndex = GWPlayerCfg[playerIndex].pad_idx;
    }
    object->controllerIndex = modelIndex;
    object->isSpecialNpc = (GWPlayerCfg[playerIndex].character == 10);
    if (object->isSpecialNpc != 0) {
        stateIndex = 8;
    }
    else {
        stateIndex = GWPlayerCfg[playerIndex].character;
    }
    object->characterIndex = stateIndex;
    object->state = 0;
    object->stateStep = 0;
    object->scoreCount = 0;
    object->position.x = playerSpawnTransforms[playerIndex][0];
    object->position.y = playerSpawnTransforms[playerIndex][1];
    object->position.z = playerSpawnTransforms[playerIndex][2];
    object->rotationX = playerSpawnTransforms[playerIndex][3];
    object->facingAngle = playerSpawnTransforms[playerIndex][4];
    object->rotationZ = playerSpawnTransforms[playerIndex][5];
    object->movementX = 0.0f;
    object->movementY = 0.0f;
    object->movementZ = 0.0f;
    object->collisionRadius = 50.0f;
    object->inputEnabled = 0;
    object->npcRightDustProcess = NULL;
    object->npcLeftDustProcess = NULL;
    M447PlayerModelCreate(object);
    Hu3DModelLayerSet(object->modelId, 6);
    M447PlayerMotionsCreate(object);
    M447PlayerSpritesCreate(object);
    M447PlayerMotionSet(object, 0, 0x40000001);
    if (object->isSpecialNpc == 0) {
        CharMotionDataClose(object->characterIndex);
    }
    if (object->isSpecialNpc != 0) {
        object->npcRightDustProcess = (HUPROCESS *)CharNpcDustSet(object->modelId, object->motionIds[2], 1, 10);
        object->npcLeftDustProcess = (HUPROCESS *)CharNpcDustSet(object->modelId, object->motionIds[1], 0, 10);
    }
    CharEffectLayerSet(2);
    object->process = HuPrcChildCreate(M447PlayerProcess, 102, 0x3000, 0, objectManager);
    object->process->user_data = object;
    for (i = 0; i < 2; i++) {
        if (!playerStates[i]) {
            playerStates[i] = object;
            break;
        }
    }
    return object;
}

void M447PlayerDestroy(M447PlayerState *playerState)
{
    s32 i;

    if (playerState->npcRightDustProcess) {
        HuPrcKill(playerState->npcRightDustProcess);
    }
    if (playerState->npcLeftDustProcess) {
        HuPrcKill(playerState->npcLeftDustProcess);
    }
    M447PlayerSpritesDestroy(playerState);
    M447PlayerMotionsDestroy(playerState);
    M447PlayerModelDestroy(playerState);
    HuPrcKill(playerState->process);
    for (i = 0; i < 2; i++) {
        if (playerStates[i] == playerState) {
            playerStates[i] = NULL;
            break;
        }
    }
    HuMemDirectFree(playerState);
}

void M447PlayerStateSet(M447PlayerState *playerState, s32 state)
{
    playerState->state = state;
    playerState->stateStep = 0;
}

float M447PlayerHeightGet(M447PlayerState *playerState)
{
    return characterHeights[playerState->characterIndex];
}

void (*playerStateHandlers[])(M447PlayerState *)
    = { M447PlayerIdleUpdate, M447PlayerControlUpdate, M447PlayerMoveToCard, M447PlayerCardSelect, M447PlayerCardWinSequenceUpdate, M447PlayerCardResponse, M447PlayerActionUpdate, M447PlayerFinishUpdate, M447PlayerCelebrationUpdate, M447PlayerReactionUpdate, M447PlayerAnimationUpdate };

void M447PlayerProcess(void)
{
    M447PlayerState *objectWork;

    objectWork = HuPrcCurrentGet()->user_data;
    while (TRUE) {
        playerStateHandlers[objectWork->state](objectWork);
        M447PlayerUpdate(objectWork);
        HuPrcVSleep();
    }
}

void M447PlayerModelCreate(M447PlayerState *playerState)
{
    if (playerState->isSpecialNpc != 0) {
        playerState->modelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_M447, 31));
    }
    else {
        playerState->modelId = CharModelCreate(playerState->characterIndex, 2);
    }
    Hu3DModelShadowSet(playerState->modelId);
}

void M447PlayerMotionsCreate(M447PlayerState *playerState)
{
    s32 i;

    if (playerState->isSpecialNpc != 0) {
        for (i = 0; i < 9; i++) {
            playerState->motionIds[i] = Hu3DJointMotionFile(playerState->modelId, characterMotionIds[playerState->characterIndex][i]);
        }
    }
    else {
        for (i = 0; i < 9; i++) {
            playerState->motionIds[i] = CharMotionCreate(playerState->characterIndex, characterMotionIds[playerState->characterIndex][i]);
        }
    }
}

void M447PlayerSpritesCreate(M447PlayerState *playerState)
{
    playerState->hudSpriteIds[0] = espEntry(DATA_MAKE_NUM(DATADIR_M447, 42), 0, 0);
    espPriSet(playerState->hudSpriteIds[0], playerState->playerIndex * 10);
    playerState->hudSpriteIds[1] = espEntry(DATA_MAKE_NUM(DATADIR_M447, 43), 0, 0);
    espPriSet(playerState->hudSpriteIds[1], playerState->playerIndex * 10 + 1);
    espDispOff(playerState->hudSpriteIds[0]);
    espDispOff(playerState->hudSpriteIds[1]);
    espTPLvlSet(playerState->hudSpriteIds[1], 0.8f);
}

void M447PlayerModelDestroy(M447PlayerState *playerState)
{
    if (playerState->isSpecialNpc != 0) {
        Hu3DModelKill(playerState->modelId);
    }
    else {
        CharModelKill(playerState->characterIndex);
    }
}

void M447PlayerMotionsDestroy(M447PlayerState *playerState)
{
    s32 i;

    if (playerState->isSpecialNpc != 0) {
        for (i = 0; i < 9; i++) {
            Hu3DMotionKill(playerState->motionIds[i]);
        }
    }
    else {
        for (i = 0; i < 9; i++) {
            CharMotionKill(playerState->characterIndex, playerState->motionIds[i]);
        }
    }
}

void M447PlayerSpritesDestroy(M447PlayerState *playerState)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        espKill(playerState->hudSpriteIds[i]);
    }
}

void M447PlayerIdleUpdate(M447PlayerState *playerState) { }

void M447PlayerControlUpdate(M447PlayerState *playerState)
{
    Vec position;
    float distance;
    s32 cardIndex;
    s32 modelIndex;
    s32 i;

    switch (playerState->stateStep) {
        case 0:
            playerState->movementState = 0;
            playerState->cardSelectionActive = 0;
            playerState->position.y = 0.0f;
            playerState->stateStep = 1;
            playerState->roundWinFlag = 0;
            /* fallthrough */
        case 1:
            if (playerState->inputEnabled != 0) {
                if (playerState->isComputer != 0) {
                    if (playerState->movementState == 0 && playerState->cardSelectionActive == 0) {
                        playerState->movementX = 0.0f;
                        playerState->movementY = 0.0f;
                        playerState->movementZ = 0.0f;
                        if (M447RandomValueGet() % computerSelectionWeights[playerState->difficulty][gameState->unk2C] == 0) {
                            cardIndex = 5;
                            for (i = 0; i < 5; i++) {
                                if (stageState->playerCardMotionActive[i] == 0 && blockStates[i]->specialCardFlag != 0) {
                                    cardIndex = i;
                                    break;
                                }
                            }
                        }
                        else {
                            for (i = 0; i < 10; i++) {
                                cardIndex = M447RandomValueGet() % 5;
                                if (stageState->playerCardMotionActive[cardIndex] == 0) {
                                    break;
                                }
                            }
                            if (i == 10) {
                                cardIndex = 5;
                            }
                        }
                        if (cardIndex < 5) {
                            M447PlayerSpawnPositionGet(cardIndex, &position);
                            distance = 10.0f - 0.1f * (M447RandomValueGet() % 10);
                            playerState->movementX = (position.x - playerState->position.x) / distance;
                            playerState->movementZ = (position.z - playerState->position.z) / distance;
                            playerState->movementY = 0.0f;
                            playerState->computerActionDelay = M447RandomValueGet() % 20 + 10;
                            playerState->movementState = 1;
                        }
                    }
                }
                else {
                    playerState->movementX = (float)HuPadStkX[playerState->controllerIndex] / 4;
                    playerState->movementZ = -(float)HuPadStkY[playerState->controllerIndex] / 4;
                }
                modelIndex = 0;
                cardIndex = M447PlayerCardIndexGet(playerState);
                if (cardIndex != 5) {
                    if (stageState->playerCardRevealed[cardIndex] == 0) {
                        M447PlayerDisplayOn(playerState);
                        if (playerState->isComputer != 0) {
                            playerState->cardSelectionActive = 1;
                            playerState->movementX = 0.0f;
                            playerState->movementY = 0.0f;
                            playerState->movementZ = 0.0f;
                            if (playerState->computerActionDelay-- <= 0) {
                                modelIndex = 1;
                            }
                        }
                        else if (HuPadBtnDown[playerState->controllerIndex] & 0x100) {
                            modelIndex = 1;
                        }
                        if (modelIndex != 0) {
                            playerState->movementX = 0.0f;
                            playerState->movementZ = 0.0f;
                            stageState->playerCardRevealed[cardIndex] = 1;
                            M447PlayerStateSet(playerState, 2);
                            playerState->cardSelectionActive = 0;
                        }
                    }
                }
                else {
                    if (playerState->cardSelectionActive != 0) {
                        playerState->cardSelectionActive = 0;
                        playerState->movementState = 0;
                    }
                    M447PlayerDisplayOff(playerState);
                }
            }
            else {
                playerState->movementX = 0.0f;
                playerState->movementY = 0.0f;
                playerState->movementZ = 0.0f;
            }
            M447PlayerMovementUpdate(playerState);
            break;
    }
}

void M447PlayerMovementUpdate(M447PlayerState *playerState)
{
    s16 playerIndex;

    playerIndex = 0;
    playerState->movementSpeed = sqrtf(playerState->movementX * playerState->movementX + playerState->movementZ * playerState->movementZ);
    if (0.5f <= playerState->movementSpeed) {
        if (10.0f <= playerState->movementSpeed) {
            playerState->movementX /= playerState->movementSpeed;
            playerState->movementZ /= playerState->movementSpeed;
            playerState->movementSpeed = 10.0f;
            playerState->movementX = playerState->movementX * 10.0f;
            playerState->movementZ = playerState->movementZ * 10.0f;
        }
        playerState->facingAngle = M447AngleApproach(playerState->facingAngle, atan2d(playerState->movementX, playerState->movementZ), 0.4f);
        if (8.0f <= playerState->movementSpeed) {
            playerIndex = 2;
        }
        else {
            playerIndex = 1;
        }
    }
    playerState->position.x += playerState->movementX;
    playerState->position.z += playerState->movementZ;
    switch (playerState->state) {
        case 1:
            M447PlayerPositionUpdate(playerState);
            M447PlayerBlockCollisionResolve(playerState);
            M447PlayerCollisionUpdate(playerState);
            break;
        case 6:
        case 7:
            M447PlayerBlockCollisionResolve(playerState);
            M447PlayerCollisionUpdate(playerState);
            break;
    }
    if (playerIndex >= 0 && playerState->activeMotionId != playerIndex) {
        M447PlayerMotionPlay(playerState, playerIndex, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
    }
}

float M447AngleApproach(float currentAngle, float targetAngle, float interpolationFactor)
{
    float movementAngle;
    float movementMagnitude;

    movementAngle = fmod(targetAngle - currentAngle, 360.0);
    if (0.0f > movementAngle) {
        movementAngle += 360.0f;
    }
    if (180.0f < movementAngle) {
        movementAngle -= 360.0f;
    }
    movementMagnitude = fmod(currentAngle + movementAngle * interpolationFactor, 360.0);
    if (0.0f > movementMagnitude) {
        movementMagnitude += 360.0f;
    }
    return movementMagnitude;
}

void M447PlayerMoveToCard(M447PlayerState *playerState)
{
    Vec rotation;
    Vec position;
    s32 playerIndex;

    switch (playerState->stateStep) {
        case 0:
            M447PlayerDisplayOff(playerState);
            playerIndex = M447PlayerCardIndexGet(playerState);
            M447PlayerSpawnPositionGet(playerIndex, &rotation);
            VECSubtract(&rotation, &playerState->position, &position);
            playerState->facingAngle = M447AngleApproach(playerState->facingAngle, atan2d(position.x, position.z), 1.0f);
            M447PlayerMotionPlay(playerState, 3, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
            playerState->stateStep = 1;
            /* fallthrough */
        case 1:
            if (!Hu3DMotionEndCheck(playerState->modelId)) {
                break;
            }
            playerState->stateStep = 2;
            /* fallthrough */
        case 2:
            M447PlayerStateSet(playerState, 3);
            break;
    }
}

void M447PlayerCardSelect(M447PlayerState *playerState)
{
    Vec position;
    s32 cardIndex;
    M447BlockState *cardWork;
    s32 i;

    switch (playerState->stateStep) {
        case 0:
            cardIndex = M447PlayerCardIndexGet(playerState);
            M447PlayerMotionSet(playerState, 0, 0x40000001);
            M447PlayerSpawnPositionGet(cardIndex, &position);
            playerState->position.x = position.x;
            playerState->position.z = position.z;
            playerState->position.y = 30.0f;
            M447PlayerCardReveal(stageState, cardIndex);
            HuAudFXPlay(0x784);
            playerState->stateTimer = 0;
            playerState->stateStep = 1;
            /* fallthrough */
        case 1:
            playerState->position.y -= 1.0f;
            if (playerState->stateTimer++ >= 30) {
                playerState->position.y = 2.0f;
                playerState->stateStep = 2;
                cardIndex = M447PlayerCardIndexGet(playerState);
                playerState->selectedBlock = blockStates[cardIndex];
                if (playerState->selectedBlock->specialCardFlag != 0) {
                    M447BlockActionSet(playerState->selectedBlock, 5);
                    for (i = 0; i < 5; i++) {
                        cardWork = blockStates[i];
                        if (cardWork != playerState->selectedBlock && stageState->playerCardRevealed[i] == 0) {
                            M447PlayerCardReveal(stageState, i);
                            if (cardWork->actionState == 2) {
                                M447BlockActionSet(cardWork, 4);
                            }
                        }
                    }
                    for (i = 0; i < 2; i++) {
                        if (playerStates[i] != playerState) {
                            playerStates[i]->inputEnabled = 0;
                        }
                    }
                    playerState->scoreCount++;
                    playerState->roundWinFlag = 1;
                    M447PlayerStateSet(playerState, 5);
                }
                else {
                    M447BlockActionSet(playerState->selectedBlock, 3);
                    M447PlayerStateSet(playerState, 4);
                }
            }
            break;
        case 2:
            M447PlayerStateSet(playerState, 1);
            break;
    }
}

const s32 celebrationSoundEffects[] = { 0x0000012A, 0x0000016A, 0x000001AA, 0x000001EA, 0x0000022A, 0x0000026A, 0x000002AA, 0x000002EA };

void M447PlayerCardWinSequenceUpdate(M447PlayerState *playerState)
{
    Vec position;
    float movementAngle;
    float movementMagnitude;

    switch (playerState->stateStep) {
        case 0:
            M447PlayerMotionPlay(playerState, 5, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
            playerState->stateTimer = 0;
            playerState->stateStep = 1;
            /* fallthrough */
        case 1:
            movementMagnitude = characterHeights[playerState->characterIndex];
            movementAngle = movementMagnitude; // inline?
            position = playerState->selectedBlock->position;
            if (movementAngle + blockLandingHeights[playerState->selectedBlock->cardIndex] + 120.0f < position.y) {
                break;
            }
            playerState->stateStep = 2;
            /* fallthrough */
        case 2:
            M447PlayerMotionSet(playerState, 4, 0);
            playerState->stateTimer = 0;
            playerState->stateStep = 3;
            M447PlayerVibrationSet(playerState, 1, 60);
            if (playerState->characterIndex == 8) {
                HuAudFXPlay(0x42);
            }
            else {
                HuAudFXPlay(celebrationSoundEffects[playerState->characterIndex]);
            }
            /* fallthrough */
        case 3:
            if (playerState->stateTimer++ < 40) {
                break;
            }
            playerState->stateTimer = 0;
            playerState->stateStep = 4;
            /* fallthrough */
        case 4:
            Hu3DModelAttrSet(playerState->modelId, HU3D_MOTATTR_PAUSE);
            playerState->stateTimer = 0;
            playerState->stateStep = 5;
            /* fallthrough */
        case 5:
            if (playerState->stateTimer++ < 20) {
                break;
            }
            playerState->stateStep = 6;
            /* fallthrough */
        case 6:
            Hu3DModelAttrReset(playerState->modelId, HU3D_MOTATTR_PAUSE);
            Hu3DModelAttrReset(playerState->modelId, HU3D_MOTATTR_LOOP);
            playerState->stateTimer = 0;
            playerState->stateStep = 7;
            /* fallthrough */
        case 7:
            if (playerState->stateTimer++ < 40) {
                break;
            }
            playerState->stateStep = 8;
        case 8:
            /* fallthrough */
            M447PlayerStateSet(playerState, 1);
            break;
    }
}

void M447PlayerCardResponse(M447PlayerState *playerState)
{
    Vec rotation;
    Vec position;
    s32 playerIndex;

    switch (playerState->stateStep) {
        case 0:
            Hu3DModelAttrReset(playerState->modelId, HU3D_MOTATTR_LOOP);
            M447PlayerMotionPlay(playerState, 5, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
            playerState->stateTimer = 0;
            playerState->stateStep = 1;
            /* fallthrough */
        case 1:
            playerIndex = M447PlayerCardIndexGet(playerState);
            rotation = blockStates[playerIndex]->position;
            VECSubtract(&rotation, &playerState->position, &position);
            playerState->facingAngle = M447AngleApproach(playerState->facingAngle, atan2d(position.x, position.z), 0.1f);
            if (playerState->stateTimer++ < 30) {
                break;
            }
            playerState->stateStep = 2;
        case 2:
            M447PlayerMotionPlay(playerState, 0, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
            M447PlayerStateSet(playerState, 8);
            break;
    }
}

const s32 reactionSoundEffects[] = { 0x00000122, 0x00000162, 0x000001A2, 0x000001E2, 0x00000222, 0x00000262, 0x000002A2, 0x000002E2, 0x00000124,
    0x00000164, 0x000001A4, 0x000001E4, 0x00000224, 0x00000264, 0x000002A4, 0x000002E4, 0x00000121, 0x00000161, 0x000001A1, 0x000001E1, 0x00000221,
    0x00000261, 0x000002A1, 0x000002E1 };

void M447PlayerCelebrationUpdate(M447PlayerState *playerState)
{
    switch (playerState->stateStep) {
        case 0:
            if (playerState->characterIndex == 8) {
                HuAudFXPlay(0x40);
            }
            else {
                HuAudFXPlay(reactionSoundEffects[playerState->characterIndex]);
            }
            M447PlayerMotionPlay(playerState, 6, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
            playerState->stateStep = 1;
            /* fallthrough */
        case 1:
            if (!Hu3DMotionEndCheck(playerState->modelId)) {
                break;
            }
            playerState->stateStep = 2;
            /* fallthrough */
        case 2:
            M447PlayerMotionPlay(playerState, 0, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
            M447PlayerStateSet(playerState, 0);
            M447PlayerVibrationSet(playerState, 0, 60);
            break;
    }
}

void M447PlayerReactionUpdate(M447PlayerState *playerState)
{
    switch (playerState->stateStep) {
        case 0:
            if (playerState->isSpecialNpc != 0) {
                HuAudFXPlay(0x40);
            }
            M447PlayerMotionPlay(playerState, 7, 0.0f, 8.0f, (playerState->isSpecialNpc != 0) ? HU3D_MOTATTR_LOOP : HU3D_MOTATTR_NONE);
            playerState->stateStep = 1;
            /* fallthrough */
        case 1:
            if (!Hu3DMotionEndCheck(playerState->modelId)) {
                break;
            }
            playerState->stateStep = 2;
            /* fallthrough */
        case 2:
            M447PlayerStateSet(playerState, 0);
            break;
    }
}

void M447PlayerAnimationUpdate(M447PlayerState *playerState)
{
    switch (playerState->stateStep) {
        case 0:
            M447PlayerMotionPlay(playerState, 8, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
            playerState->stateStep = 1;
            /* fallthrough */
        case 1:
            if (!Hu3DMotionEndCheck(playerState->modelId)) {
                break;
            }
            playerState->stateStep = 2;
            /* fallthrough */
        case 2:
            M447PlayerStateSet(playerState, 0);
            break;
    }
}

void M447PlayerActionUpdate(M447PlayerState *playerState)
{
    float verticalSpeed;
    float horizontalSpeed;
    float movementSpeed;

    switch (playerState->stateStep) {
        case 0:
            M447PlayerDisplayOff(playerState);
            verticalSpeed = playerState->position.x - playerSpawnTransforms[playerState->playerIndex][0];
            horizontalSpeed = playerState->position.z - playerSpawnTransforms[playerState->playerIndex][2] - 25.0f;
            if (ABS(verticalSpeed) < 5.0f && ABS(horizontalSpeed) < 5.0f && 180.0f == playerState->facingAngle) {
                playerState->stateStep = 3;
            }
            else {
                playerState->stateStep = 1;
            }
            /* fallthrough */
        case 1:
            movementSpeed = 8.0f;
            playerState->movementX = (playerSpawnTransforms[playerState->playerIndex][0] - playerState->position.x) / movementSpeed;
            playerState->movementZ = (playerSpawnTransforms[playerState->playerIndex][2] - playerState->position.z) / movementSpeed;
            playerState->movementY = 0.0f;
            if (!(playerState->movementX > -0.1f && playerState->movementX < 0.1f && playerState->movementZ > -0.1f && playerState->movementZ < 0.1f)) {
                M447PlayerMovementUpdate(playerState);
                break;
            }
            playerState->stateStep = 2;
            /* fallthrough */
        case 2:
            movementSpeed = 8.0f;
            playerState->movementX = (playerSpawnTransforms[playerState->playerIndex][0] - playerState->position.x) / movementSpeed;
            playerState->movementZ = (playerSpawnTransforms[playerState->playerIndex][2] - 25.0f - playerState->position.z) / movementSpeed;
            playerState->movementY = 0.0f;
            if (!(playerState->movementX > -0.1f && playerState->movementX < 0.1f && playerState->movementZ > -0.1f && playerState->movementZ < 0.1f)) {
                M447PlayerMovementUpdate(playerState);
                break;
            }
            playerState->stateStep = 3;
            /* fallthrough */
        case 3:
            M447PlayerStateSet(playerState, 1);
            break;
    }
}

void M447PlayerFinishUpdate(M447PlayerState *playerState)
{
    float ratio;

    switch (playerState->stateStep) {
        case 0:
            M447PlayerDisplayOff(playerState);
            playerState->stateStep = 1;
            /* fallthrough */
        case 1:
            ratio = 8.0f;
            playerState->movementX = (playerSpawnTransforms[playerState->playerIndex][12] - playerState->position.x) / ratio;
            playerState->movementZ = (playerSpawnTransforms[playerState->playerIndex][14] - 25.0f - playerState->position.z) / ratio;
            playerState->movementY = 0.0f;
            M447PlayerMovementUpdate(playerState);
            if (!(playerState->movementX > -0.1f && playerState->movementX < 0.1f && playerState->movementZ > -0.1f && playerState->movementZ < 0.1f)) {
                break;
            }
            playerState->stateStep = 2;
            /* fallthrough */
        case 2:
            ratio = 8.0f;
            playerState->movementX = (playerSpawnTransforms[playerState->playerIndex][12] - playerState->position.x) / ratio;
            playerState->movementZ = (playerSpawnTransforms[playerState->playerIndex][14] - playerState->position.z) / ratio;
            playerState->movementY = 0.0f;
            M447PlayerMovementUpdate(playerState);
            if (!(playerState->movementX > -0.1f && playerState->movementX < 0.1f && playerState->movementZ > -0.1f && playerState->movementZ < 0.1f)) {
                break;
            }
            playerState->stateStep = 3;
            /* fallthrough */
        case 3:
            M447PlayerStateSet(playerState, 1);
            break;
    }
}

void M447PlayerUpdate(M447PlayerState *playerState)
{
    Vec rotation;
    Vec position;

    Hu3DModelPosSet(playerState->modelId, playerState->position.x, playerState->position.y, playerState->position.z);
    Hu3DModelRotSet(playerState->modelId, playerState->rotationX, playerState->facingAngle, playerState->rotationZ);
    rotation = playerState->position;
    rotation.y += 300.0f;
    Hu3D3Dto2D(&rotation, 1, &position);
    espPosSet(playerState->hudSpriteIds[1], position.x, position.y + 3.0f);
    espPosSet(playerState->hudSpriteIds[0], position.x, position.y);
}

void M447PlayerMotionSet(M447PlayerState *playerState, s32 motionIndex, s32 modelAttributes)
{
    if (playerState->isSpecialNpc != 0) {
        Hu3DMotionSet(playerState->modelId, playerState->motionIds[motionIndex]);
    }
    else {
        CharMotionSet(playerState->characterIndex, playerState->motionIds[motionIndex]);
    }
    Hu3DModelAttrSet(playerState->modelId, modelAttributes);
    playerState->activeMotionId = motionIndex;
}

void M447PlayerMotionPlay(M447PlayerState *playerState, s32 motionIndex, float startTime, float transitionTime, s32 modelAttributes)
{
    if (playerState->isSpecialNpc != 0) {
        Hu3DMotionShiftSet(playerState->modelId, playerState->motionIds[motionIndex], startTime, transitionTime, modelAttributes);
    }
    else {
        CharMotionShiftSet(playerState->characterIndex, playerState->motionIds[motionIndex], startTime, transitionTime, modelAttributes);
    }
    playerState->activeMotionId = motionIndex;
}

float collisionBoundaryLines[][4] = { { -700.0f, -700.0f, -700.0f, 700.0f }, { 700.0f, -700.0f, 700.0f, 700.0f }, { -800.0f, 450.0f, 800.0f, 450.0f },
    { -700.0f, -700.0f, 700.0f, -700.0f } };

// Vec[]?
float collisionCircles[][3] = { { -700.0f, -700.0f, 200.0f }, { 700.0f, -700.0f, 200.0f } };

void M447PlayerPositionUpdate(M447PlayerState *playerState)
{
    float cursorPosition[3]; // Vec?
    float offset[4];
    float targetPosition[3]; // Vec?
    float distance;
    float scale;
    float rotationSpeed;
    s32 i;

    cursorPosition[0] = playerState->position.x;
    cursorPosition[1] = playerState->position.z;
    cursorPosition[2] = 95.0f;
    for (i = 0; i < 4; i++) {
        if (M447CircleLineIntersectionGet(cursorPosition, collisionBoundaryLines[i], offset) == 0) {
            continue;
        }
        if (i / 2 == 0) {
            if (collisionBoundaryLines[i][0] < 0.0f) {
                playerState->position.x = cursorPosition[2] + collisionBoundaryLines[i][0];
            }
            else {
                playerState->position.x = collisionBoundaryLines[i][0] - cursorPosition[2];
            }
        }
        else {
            if (collisionBoundaryLines[i][1] < 0.0f) {
                playerState->position.z = cursorPosition[2] + collisionBoundaryLines[i][1];
            }
            else {
                playerState->position.z = collisionBoundaryLines[i][1] - cursorPosition[2];
            }
        }
        playerState->movementState = 0;
    }
    targetPosition[0] = playerState->position.x;
    targetPosition[1] = playerState->position.z;
    targetPosition[2] = playerState->collisionRadius;
    for (i = 0; i < 2; i++) {
        if (M447CircleCollisionResolve(targetPosition, collisionCircles[i], 1, 0) != 0) {
            playerState->position.x = targetPosition[0];
            playerState->position.z = targetPosition[1];
            playerState->movementState = 0;
        }
    }
    if (playerState->position.z > 0.0f) {
        distance = playerState->position.x;
        scale = playerState->position.z - -120.0f;
        rotationSpeed = sqrtf(distance * distance + scale * scale);
        if (700.0f - playerState->collisionRadius < rotationSpeed) {
            distance /= rotationSpeed;
            scale /= rotationSpeed;
            playerState->position.x = distance * (700.0f - playerState->collisionRadius);
            playerState->position.z = -120.0f + scale * (700.0f - playerState->collisionRadius);
        }
    }
}

void M447PlayerCollisionUpdate(M447PlayerState *playerState)
{
    float rotation[3]; // Vec?
    float position[3]; // Vec?
    M447PlayerState *blockWork;
    s32 counter;
    s32 i;

    for (i = 0; i < 2; i++) {
        blockWork = playerStates[i];
        if (!blockWork || blockWork == playerState) {
            continue;
        }
        rotation[0] = playerState->position.x;
        rotation[1] = playerState->position.z;
        rotation[2] = playerState->collisionRadius;
        position[0] = blockWork->position.x;
        position[1] = blockWork->position.z;
        position[2] = blockWork->collisionRadius;
        counter = (blockWork->state == 1) ? 1 : 0;
        if (M447CircleCollisionResolve(rotation, position, 1, counter) == 0) {
            continue;
        }
        switch (playerState->state) {
            case 1:
                playerState->position.x = rotation[0];
                playerState->position.z = rotation[1];
                blockWork->position.x = position[0];
                blockWork->position.z = position[1];
                playerState->movementState = 0;
                break;
            case 6:
            case 7:
                playerState->position.x = rotation[0];
                playerState->position.z = rotation[1];
                if (ABS(playerState->position.z - blockWork->position.z) < 2.0f) {
                    playerState->position.z += 55.0f * (0.0f > playerState->position.z ? -1 : 1);
                }
                playerState->movementState = 0;
                break;
        }
        switch (playerState->state) {
            case 1:
                M447PlayerPositionUpdate(playerState);
                M447PlayerPositionUpdate(blockWork);
                break;
        }
    }
}

void M447PlayerBlockCollisionResolve(M447PlayerState *playerState)
{
    float velocity[3]; // Vec?
    float rotation[3]; // Vec?
    Vec position;
    s32 i;

    velocity[0] = playerState->position.x;
    velocity[1] = playerState->position.z;
    velocity[2] = playerState->collisionRadius;
    for (i = 0; i < 5; i++) {
        if (stageState->playerCardMotionActive[i] != 0) {
            continue;
        }
        M447PlayerSpawnPositionGet(i, &position);
        rotation[0] = position.x;
        rotation[1] = position.z;
        rotation[2] = 35.0f;
        if (M447CircleCollisionResolve(velocity, rotation, 1, 0) != 0) {
            playerState->position.x = velocity[0];
            playerState->position.z = velocity[1];
            playerState->movementState = 0;
        }
    }
}

s32 M447PlayerCardIndexGet(M447PlayerState *playerState)
{
    float velocity[3]; // Vec?
    float rotation[3]; // Vec?
    Vec position;
    s32 i;

    velocity[0] = playerState->position.x;
    velocity[1] = playerState->position.z;
    velocity[2] = playerState->collisionRadius;
    for (i = 0; i < 5; i++) {
        M447PlayerSpawnPositionGet(i, &position);
        rotation[0] = position.x;
        rotation[1] = position.z;
        rotation[2] = 40.0f;
        if (M447CircleCollisionResolve(velocity, rotation, 0, 0) != 0) {
            return i;
        }
    }
    return 5;
}

void M447PlayerDisplayOn(M447PlayerState *playerState)
{
    espDispOn(playerState->hudSpriteIds[1]);
    espDispOn(playerState->hudSpriteIds[0]);
}

void M447PlayerDisplayOff(M447PlayerState *playerState)
{
    espDispOff(playerState->hudSpriteIds[1]);
    espDispOff(playerState->hudSpriteIds[0]);
}

void M447PlayerVibrationSet(M447PlayerState *playerState, s32 vibrationType, s32 durationFrames)
{
    if (playerState->isSpecialNpc == 0) {
        switch (vibrationType) {
            case 0:
                omVibrate(playerState->playerIndex, durationFrames, 6, 6);
                return;
            case 1:
                omVibrate(playerState->playerIndex, durationFrames, 4, 2);
                return;
            case 2:
                omVibrate(playerState->playerIndex, durationFrames, 12, 0);
                break;
        }
    }
}

#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"

#include "REL/m417Dll.h"
#include "version.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/frand.h"
#endif

typedef struct PlayerSequenceEntry {
    /* 0x00 */ u16 playerIndex;
    /* 0x04 */ Float actionState;
    /* 0x08 */ Float actionTimer;
    /* 0x0C */ Float characterId;
    /* 0x10 */ u32 playerFlags;
} PlayerSequenceEntry; /* size = 0x14 */

typedef struct MinigamePlayerState {
    /* 0x04 */ s32 playerIndex;
    /* 0x04 */ s32 actionState;
    /* 0x08 */ s32 actionTimer;
    /* 0x0C */ u32 characterId;
    /* 0x10 */ u32 controllerButtons;
    /* 0x14 */ s32 movementState;
    /* 0x18 */ s32 animationState;
    /* 0x1C */ s32 triggerButtonState;
    /* 0x20 */ Float movementSpeed;
    /* 0x24 */ Float movementAcceleration;
    /* 0x28 */ s32 movementTimer;
    /* 0x2C */ s32 motionEnabled;
    /* 0x30 */ s32 motionState;
    /* 0x34 */ s32 motionTimer;
    /* 0x38 */ s32 effectState;
    /* 0x3C */ s32 effectTimer;
    /* 0x40 */ s32 actionIndex;
    /* 0x44 */ s32 isComputer;
    /* 0x48 */ s32 controllerPort;
    /* 0x4C */ s32 computerActionActive;
    /* 0x50 */ u16 playerActionState;
    /* 0x52 */ char reservedPlayerData[2];
    /* 0x54 */ s32 characterState;
    /* 0x58 */ char reservedMotionData[4];
    /* 0x5C */ s16 characterMotionId;
    /* 0x5E */ char reservedCharacterData[6];
    /* 0x64 */ Float runSpeed;
    /* 0x68 */ Float walkSpeed;
    /* 0x6C */ Float jumpSpeed;
    /* 0x70 */ Float fallSpeed;
    /* 0x74 */ Float difficultyScale;
    /* 0x78 */ s32 reservedMovementFlags;
    /* 0x7C */ Float gravity;
    /* 0x80 */ u32 computerActionTimer;
    /* 0x84 */ u32 computerMovementTimer;
    /* 0x88 */ Float rotationSpeed;
    /* 0x8C */ Float movementScale;
    /* 0x90 */ u32 computerRestTimer;
    /* 0x94 */ char reservedAnimationData[4];
    /* 0x98 */ Float animationSpeed;
    /* 0x9C */ s32 fallSoundEffectId;
    /* 0xA0 */ s32 motionFrame;
    /* 0xA4 */ u32 motionTransitionTimer;
    /* 0xA8 */ u32 modelFlags;
    /* 0xAC */ Float animationFrame;
    /* 0xB0 */ Float waterSplashTimer;
    /* 0xB4 */ Vec spawnPosition;
    /* 0xC0 */ u32 actionFrameCounter;
    /* 0xC4 */ u32 actionFrameSync;
    /* 0xC8 */ Vec position;
    /* 0xD4 */ Vec targetPosition;
    /* 0xE0 */ Vec previousPosition;
    /* 0xEC */ Vec velocity;
    /* 0xF8 */ Vec movementTarget;
    /* 0x104 */ Vec movementStart;
    /* 0x110 */ Vec movementDelta;
    /* 0x11C */ Quaternion facingRotation;
    /* 0x12C */ Float turnAngle;
    /* 0x130 */ Float movementAngle;
    /* 0x134 */ Float movementDistance;
    /* 0x138 */ Float turnSpeed;
    /* 0x13C */ Float motionSpeed;
    /* 0x140 */ Float jumpHeight;
    /* 0x144 */ Float fallDistance;
    /* 0x148 */ Float groundHeight;
    /* 0x14C */ Vec movementVector;
    /* 0x158 */ char reservedTransformData[0xC];
    /* 0x164 */ Quaternion startRotation;
    /* 0x174 */ Quaternion targetRotation;
    /* 0x184 */ Quaternion currentRotation;
    /* 0x194 */ Quaternion previousRotation;
    /* 0x1A4 */ Quaternion movementRotation;
    /* 0x1B4 */ Quaternion turnRotation;
    /* 0x1C4 */ Mtx modelMatrix;
    /* 0x1F4 */ Mtx previousModelMatrix;
    /* 0x224 */ Mtx targetModelMatrix;
    /* 0x254 */ Vec playerPathPoints[16]; // maybe less
} MinigamePlayerState; /* size = 0x314 */

void InitializePlayerObject(OMOBJ *object);
void UpdateHumanPlayer(OMOBJ *object);
void UpdateComputerPlayer(OMOBJ *object);
void UpdatePlayerActionState(OMOBJ *object);
void UpdatePlayerReactionState(OMOBJ *object);
void UpdatePlayerFallState(OMOBJ *object);
void UpdatePlayerMovement(OMOBJ *object);
void UpdatePlayerInput(OMOBJ *object);
void UpdatePlayerAction(OMOBJ *object);
void UpdatePlayerAnimation(OMOBJ *object);
void UpdatePlayerPhysics(OMOBJ *object);
void UpdatePlayerModels(OMOBJ *object);
void UpdateComputerMovementTarget(OMOBJ *object);
void UpdateComputerActionTiming(OMOBJ *object);
void SetPlayerMotion(OMOBJ *object, u32 motionIndex);
s32 IsPlayerMotionFinished(OMOBJ *object);
void SetQuaternionFromAngles(Float rotationAngle, Float rotationScale, Quaternion *rotation);

s32 playerModelFiles[][2] = {
    { DATA_MAKE_NUM(DATADIR_M417, 1), DATA_MAKE_NUM(DATADIR_M417, 6) },
    { DATA_MAKE_NUM(DATADIR_M417, 1), DATA_MAKE_NUM(DATADIR_M417, 7) },
    { DATA_MAKE_NUM(DATADIR_M417, 1), DATA_MAKE_NUM(DATADIR_M417, 8) },
    { DATA_MAKE_NUM(DATADIR_M417, 1), DATA_MAKE_NUM(DATADIR_M417, 9) },
    { DATA_MAKE_NUM(DATADIR_M417, 1), DATA_MAKE_NUM(DATADIR_M417, 10) },
    { DATA_MAKE_NUM(DATADIR_M417, 1), DATA_MAKE_NUM(DATADIR_M417, 11) },
    { DATA_MAKE_NUM(DATADIR_M417, 1), DATA_MAKE_NUM(DATADIR_M417, 12) },
    { DATA_MAKE_NUM(DATADIR_M417, 1), DATA_MAKE_NUM(DATADIR_M417, 13) },
};

u32 playerMotionFiles[11][3] = {
    { 0, 1, 1 },
    { 23, 1, 1 },
    { 24, 1, 1 },
    { 2, 1, 0 },
    { 3, 1, 0 },
    { DATA_MAKE_NUM(DATADIR_M417, 18), 1, 0 },
    { DATA_MAKE_NUM(DATADIR_M417, 26), 1, 0 },
    { 5, 0, 1 },
    { 6, 0, 1 },
    { 8, 0, 1 },
    { 9, 0, 1 },
};

PlayerSequenceEntry playerMotionSettings[11] = {
    { 0, 0.2f, 0.0f, -1.0f, HU3D_MOTATTR_LOOP },
    { 1, 0.2f, 0.0f, -1.0f, HU3D_MOTATTR_NONE },
    { 2, 0.2f, 0.0f, -1.0f, HU3D_MOTATTR_NONE },
    { 3, 0.2f, 0.0f, -1.0f, HU3D_MOTATTR_LOOP },
    { 4, 0.2f, 0.0f, -1.0f, HU3D_MOTATTR_LOOP },
    { 5, 0.5f, 0.0f, -1.0f, HU3D_MOTATTR_NONE },
    { 6, 0.2f, 0.0f, -1.0f, HU3D_MOTATTR_LOOP },
    { 7, 0.03f, 0.0f, -1.0f, HU3D_MOTATTR_NONE },
    { 8, 0.0f, 0.0f, -1.0f, HU3D_MOTATTR_NONE },
    { 9, 0.2f, 0.0f, -1.0f, HU3D_MOTATTR_NONE },
    { 10, 0.0f, 0.0f, -1.0f, HU3D_MOTATTR_NONE },
};

s32 playerObject; // unused
s32 resultsSequenceId; // unused
OMOBJ *playerObjects[4];
u32 playerCharacterTypes[4];
u32 lastComputerPlayerIndex;
u32 computerActionTimer;

void CreatePlayerObjects(HUPROCESS *objectManager)
{
    u32 playerGroups[4];
    u32 playerIndicesByGroup[4];

    s16 playerIndex;
    s16 nextPlayerIndex;
    s32 swapValue;
    OMOBJ *playerObject;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerGroups[playerIndex] = GWPlayerCfg[playerIndex].group;
        playerIndicesByGroup[playerIndex] = playerIndex;
    }
    for (playerIndex = 0; playerIndex < 3; playerIndex++) {
        for (nextPlayerIndex = playerIndex + 1; nextPlayerIndex < 4; nextPlayerIndex++) {
            if (playerGroups[playerIndex] > playerGroups[nextPlayerIndex]) {
                swapValue = playerGroups[playerIndex];
                playerGroups[playerIndex] = playerGroups[nextPlayerIndex];
                playerGroups[nextPlayerIndex] = swapValue;
                swapValue = playerIndicesByGroup[playerIndex];
                playerIndicesByGroup[playerIndex] = playerIndicesByGroup[nextPlayerIndex];
                playerIndicesByGroup[nextPlayerIndex] = swapValue;
            }
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerCharacterTypes[playerIndicesByGroup[playerIndex]] = playerIndex;
    }
    omMakeGroupEx(objectManager, 0, 4);
    omGetGroupMemberListEx(objectManager, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerObject = playerObjects[playerIndex] = omAddObjEx(objectManager, 40, 2, 11, 0, InitializePlayerObject);
        playerObject->work[0] = playerIndex;
    }
    computerActionTimer = lastComputerPlayerIndex = 0;
}

void DestroyPlayerObjects(void)
{
    CharModelKill(-1);
}

void InitializePlayerObject(OMOBJ *playerObject)
{
    s32 initialActionState;

    MinigamePlayerState *playerState;
    s32 fallSoundEffectId;
    s32 characterIndex;
    u32 usesAlternateModel;
    u32 playerConfigIndex;

    Float difficultyScales[4] = { 0.25f, 0.5f, 0.75f, 1.0f };
    Float spawnDistances[4] = { 0.0f, 650.0f, 650.0f, 650.0f };
    Float spawnAngles[4] = { 180.0f, 180.0f, 300.0f, 60.0f };

    playerObject->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(MinigamePlayerState), HU_MEMNUM_OVL);
    playerState = playerObject->data;
    memset(playerState, 0, sizeof(MinigamePlayerState));
    playerObject->stat |= 256;
    playerConfigIndex = playerObject->work[0];
    characterIndex = GWPlayerCfg[playerConfigIndex].character;
    playerState->playerIndex = characterIndex;
    initialActionState = playerState->actionState = 0;
    playerState->characterId = playerCharacterTypes[playerObject->work[0]];
    usesAlternateModel = 0;
    if (playerState->characterId) {
        usesAlternateModel++;
    }
    SetPlayerResult(playerObject->work[0], playerState->characterId, playerState->playerIndex);
    playerState->difficultyScale = difficultyScales[GWPlayerCfg[playerConfigIndex].diff & 3];
    playerState->isComputer = GWPlayerCfg[playerObject->work[0]].iscom;
    playerObject->mdlId[0] = CharModelCreate(characterIndex, 4);
    Hu3DModelLayerSet(playerObject->mdlId[0], 4);
    Hu3DModelShadowSet(playerObject->mdlId[0]);
    for (motionIndex = 0; motionIndex < 11; motionIndex++) {
        if (playerMotionFiles[motionIndex][usesAlternateModel + 1]) {
            if (playerMotionFiles[motionIndex][0] < 65536) {
                playerObject->mtnId[motionIndex] = CharMotionCreate(characterIndex, playerMotionFiles[motionIndex][0]);
            }
            else {
                playerObject->mtnId[motionIndex] = Hu3DJointMotionFile(playerObject->mdlId[0], characterIndex + playerMotionFiles[motionIndex][0]);
            }
        }
    }
    CharMotionDataClose(characterIndex);
    playerObject->mdlId[1] = Hu3DModelCreateFile(playerModelFiles[characterIndex][usesAlternateModel]);
    Hu3DModelShadowMapSet(playerObject->mdlId[1]);

    playerState->spawnPosition.x = spawnAngles[playerState->characterId];
    playerState->spawnPosition.y = 0.0f;
    playerState->spawnPosition.z = spawnDistances[playerState->characterId];

    playerObject->trans.x = playerState->position.x = playerState->spawnPosition.z * sind(playerState->spawnPosition.x);
    playerObject->trans.y = playerState->position.y = 0.0f;
    playerObject->trans.z = playerState->position.z = playerState->spawnPosition.z * cosd(playerState->spawnPosition.x);

    playerState->targetPosition = playerState->position;
    playerState->movementTarget = playerState->position;
    playerState->movementStart = playerState->movementTarget;

    playerState->runSpeed = playerState->walkSpeed = playerState->spawnPosition.x - 180.0f;
    playerState->jumpSpeed = 0.0f;
    playerState->fallSpeed = 0.02f;
    if (playerState->characterId) {
        Vec worldUp = { 0.0f, 1.0f, 0.0f };
        MTXRotAxisRad(playerState->modelMatrix, &worldUp, 0.017453292f * playerState->runSpeed);
    }
    else {
        Vec worldUpAxis = { 0.0f, 1.0f, 0.0f };
        MTXIdentity(playerState->modelMatrix);
        C_QUATRotAxisRad(&playerState->currentRotation, &worldUpAxis, 0.0f);
        playerState->previousRotation = playerState->currentRotation;
        QUATInverse(&playerState->previousRotation, &playerState->movementRotation);
        playerState->turnRotation = playerState->movementRotation;
        playerState->startRotation = playerState->previousRotation;
        playerState->targetRotation = playerState->previousRotation;
        playerState->fallDistance = 1.0f;
        playerState->targetPosition.z += 50.0f;
    }
    MTXInverse(playerState->modelMatrix, playerState->previousModelMatrix);
    MTXCopy(playerState->previousModelMatrix, playerState->targetModelMatrix);
    playerState->movementDistance = playerState->turnSpeed = 0.0f;
    playerState->turnAngle = 1.0f;
    playerState->actionTimer = -1;
    SetPlayerMotion(playerObject, 0);
    playerState->reservedMovementFlags = 0;
    playerState->fallSoundEffectId = -1;
    lastComputerPlayerIndex = playerObject->work[0];
    computerActionTimer = REFRESH_RATE_F * (4.0f + (0.002f * frandmod(1000)));
    playerState->movementTimer = 1;
    playerState->playerActionState = 0;
    if (!usesAlternateModel) {
        playerObject->objFunc = UpdateHumanPlayer;
        return;
    }
    playerState->motionEnabled = 1;
    playerObject->objFunc = UpdateComputerPlayer;
}

void UpdateHumanPlayer(OMOBJ *playerObject)
{
    Float stickX;
    Float stickY;
    double unusedInputValue;
    Float unusedMovementValue;

    MinigamePlayerState *playerState;
    s32 inputComplete;

    playerState = playerObject->data;
    inputComplete = 0;
    playerState->controllerButtons = playerState->animationState = 0;
    playerState->movementState = 0;
    playerState->movementSpeed = 0.0f;
    playerState->movementAcceleration = 0.0f;
    if (getSequenceState() == 2) {
        if (playerState->isComputer) {
            UpdateComputerMovementTarget(playerObject);
        }
        else {
            playerState->movementState = HuPadBtn[GWPlayerCfg[playerObject->work[0]].pad_idx];
            playerState->controllerButtons = HuPadBtnDown[GWPlayerCfg[playerObject->work[0]].pad_idx];
            stickX = HuPadStkX[GWPlayerCfg[playerObject->work[0]].pad_idx];
            stickY = -HuPadStkY[GWPlayerCfg[playerObject->work[0]].pad_idx];
            playerState->movementSpeed = atan2d(stickX, stickY);
            playerState->movementAcceleration = 0.013888889f * sqrtf((stickX * stickX) + (stickY * stickY));
            if (playerState->movementAcceleration > 1.0f) {
                playerState->movementAcceleration = 1.0f;
            }
        }
    }
    if ((s32)playerState->controllerButtons != 0) {
        (void)inputComplete;
    }
    else {
        inputComplete = 1;
    }
    UpdatePlayerPhysics(playerObject);
    UpdatePlayerAnimation(playerObject);
    UpdatePlayerMovement(playerObject);
    UpdatePlayerModels(playerObject);
    if (getSequenceState() == 3) {
        if (inputComplete && isResultSequenceFinished()) {
            playerState->controllerButtons = 0;
            playerObject->objFunc = UpdatePlayerActionState;
        }
    }
    else if (playerState->effectTimer) {
        playerState->actionIndex = 1;
        SetMinigameMode(2);
        playerState->controllerButtons = 0;
        playerObject->objFunc = UpdatePlayerFallState;
    }
}

void UpdateComputerPlayer(OMOBJ *playerObject)
{
    Float stickX;
    Float stickY;
    double unusedInputValue;
    Float unusedMovementValue;

    MinigamePlayerState *playerState;
    s32 actionComplete;
    s32 previousTriggerState;

    playerState = playerObject->data;
    actionComplete = 0;
    playerState->controllerButtons = playerState->animationState = 0;
    playerState->movementState = 0;
    playerState->movementSpeed = 0.0f;
    playerState->movementAcceleration = 0.0f;
    if (getSequenceState() == 2) {
        if (playerState->isComputer != 0) {
            playerState->triggerButtonState = 0;
            UpdateComputerActionTiming(playerObject);
        }
        else {
            playerState->movementState = HuPadBtn[GWPlayerCfg[playerObject->work[0]].pad_idx];
            playerState->controllerButtons = HuPadBtnDown[GWPlayerCfg[playerObject->work[0]].pad_idx];
            stickX = HuPadStkX[GWPlayerCfg[playerObject->work[0]].pad_idx];
            stickY = -HuPadStkY[GWPlayerCfg[playerObject->work[0]].pad_idx];
            playerState->movementSpeed = atan2d(stickX, stickY);
            playerState->movementAcceleration = 0.013888889f * sqrtf((stickX * stickX) + (stickY * stickY));
            if (playerState->movementAcceleration > 1.0f) {
                playerState->movementAcceleration = 1.0f;
            }
            previousTriggerState = playerState->triggerButtonState;
            playerState->triggerButtonState = 0;
            if (HuPadTrigL[GWPlayerCfg[playerObject->work[0]].pad_idx] > 80) {
                playerState->triggerButtonState = 1;
            }
            playerState->animationState = playerState->triggerButtonState & ~previousTriggerState;
        }
    }
    UpdatePlayerInput(playerObject);
    if ((s32)playerState->controllerButtons != 0) {
        (void)actionComplete;
    }
    else if (playerState->motionEnabled) {
        actionComplete = 1;
    }
    UpdatePlayerMovement(playerObject);
    UpdatePlayerModels(playerObject);
    if (getSequenceState() == 3 && actionComplete && isResultSequenceFinished()) {
        MarkPlayerItemCollected(playerObject->work[0]);
        playerState->controllerButtons = 0;
        playerObject->objFunc = UpdatePlayerReactionState;
    }
}

void UpdatePlayerActionState(OMOBJ *playerObject)
{
    Vec horizontalVelocity;

    Float horizontalSpeed;

    MinigamePlayerState *playerState;

    playerState = playerObject->data;
    playerState->playerActionState = playerState->animationState = 0;
    playerState->movementState = 0;
    playerState->movementSpeed = 0.0f;
    playerState->movementAcceleration = 0.0f;

    switch (playerState->playerActionState) {
        case 0:
            horizontalVelocity = playerState->velocity;
            horizontalVelocity.y = 0.0f;
            horizontalSpeed = VECMag(&horizontalVelocity);
            if (horizontalSpeed > 30.000002f) {
                playerState->movementSpeed = atan2d(-horizontalVelocity.x, -horizontalVelocity.z);
                playerState->movementAcceleration = 0.01f * horizontalSpeed;
                if (playerState->movementAcceleration > 1.0f) {
                    playerState->movementAcceleration = 1.0f;
                }
            }
            else {
                playerState->movementSpeed = 0.0f;
                playerState->movementAcceleration = 0.01f;
                MarkPlayerItemCollected(playerObject->work[0]);
                playerState->playerActionState++;
            }
            break;
        case 1:
            if (getResultState() == 1) {
                playerState->movementTimer = 0;
                if (getPlayerWinSequence(playerObject->work[0]) >= 0) {
                    SetPlayerMotion(playerObject, 1);
                }
                else {
                    SetPlayerMotion(playerObject, 2);
                }
                playerState->playerActionState++;
            }
            break;
        case 2:
            if (IsPlayerMotionFinished(playerObject)) {
                MarkPlayerScore(playerObject->work[0]);
            }
            break;
    }
    UpdatePlayerPhysics(playerObject);
    UpdatePlayerAnimation(playerObject);
    UpdatePlayerMovement(playerObject);
    UpdatePlayerModels(playerObject);
}

void UpdatePlayerReactionState(OMOBJ *playerObject)
{
    MinigamePlayerState *playerState;

    playerState = playerObject->data;
    playerState->playerActionState = playerState->animationState = 0;
    playerState->movementState = 0;
    playerState->movementSpeed = 0.0f;
    playerState->movementAcceleration = 0.0f;
    switch (playerState->playerActionState) {
        case 0:
            if (getResultState() == 1) {
                playerState->movementTimer = 0;
                if (getPlayerWinSequence(playerObject->work[0]) >= 0) {
                    playerState->walkSpeed = 0.0f;
                    SetPlayerMotion(playerObject, 1);
                }
                else {
                    SetPlayerMotion(playerObject, 2);
                }
                playerState->playerActionState++;
            }
            break;
        case 1:
            if (IsPlayerMotionFinished(playerObject)) {
                MarkPlayerScore(playerObject->work[0]);
            }
            break;
    }
    UpdatePlayerInput(playerObject);
    UpdatePlayerMovement(playerObject);
    UpdatePlayerModels(playerObject);
}

void UpdatePlayerFallState(OMOBJ *playerObject)
{
    Vec recoveryPosition;
    Vec fallDirection;

    Float waterHeight;

    MinigamePlayerState *playerState;

    playerState = playerObject->data;
    playerState->playerActionState = playerState->animationState = 0;
    playerState->movementState = 0;
    playerState->movementSpeed = 0.0f;
    playerState->movementAcceleration = 0.0f;
    switch (playerState->playerActionState) {
        case 0:
            fallDirection = playerState->movementVector;
            fallDirection.y = 0.0f;
            if (VECMag(&fallDirection) > 0.0f) {
                VECNormalize(&fallDirection, &fallDirection);
            }
            else {
                fallDirection.x = 1.0f;
                fallDirection.z = 0.0f;
            }
            playerState->walkSpeed = atan2d(-fallDirection.x, -fallDirection.z);
            VECScale(&fallDirection, &playerState->previousPosition, REFRESH_FREQ * 100.0f);
            SetPlayerMotion(playerObject, 5);
            Hu3DModelLayerSet(playerObject->mdlId[0], 0);
            Hu3DModelShadowReset(playerObject->mdlId[0]);
            playerState->playerActionState++;

        case 1:
#if VERSION_PAL
            playerState->previousPosition.y += -1.176f;
#else
            playerState->previousPosition.y += -0.8166667f;
#endif
            if ((playerState->targetPosition.y < -50.0f) && (playerState->fallSoundEffectId < 0)) {
                playerState->fallSoundEffectId = HuAudFXPlay(1483);
            }
            if (playerState->targetPosition.y < -300.0f) {
                fallDirection = playerState->targetPosition;
                fallDirection.y = 0.0f;
                if (VECMag(&fallDirection) > 0.0f) {
                    VECNormalize(&fallDirection, &fallDirection);
                }
                else {
                    fallDirection.x = 0.0f;
                    fallDirection.z = 1.0f;
                }
                VECScale(&fallDirection, &recoveryPosition, 250.0f);
                recoveryPosition.y = -100.0f;
                VECSubtract(&recoveryPosition, &playerState->targetPosition, &fallDirection);
                VECScale(&fallDirection, &playerState->previousPosition, REFRESH_FREQ);
                SetPlayerMotion(playerObject, 6);
                playerState->playerActionState++;
            }
            break;
        case 2:
            if (-100.0f > playerState->targetPosition.y) {
                break;
            }
            playerState->previousPosition.x = playerState->previousPosition.y = playerState->previousPosition.z = 0.0f;
            MarkPlayerItemCollected(playerObject->work[0]);
            MarkPlayerScore(playerObject->work[0]);
            playerState->playerActionState++;
        case 3:
            waterHeight = getWaterHeightAtPosition(playerState->targetPosition.x, playerState->targetPosition.y, playerState->targetPosition.z);
            playerState->targetPosition.y += (0.1f * ((waterHeight - 80.0f) - playerState->targetPosition.y));

            break;
    }
    VECAdd(&playerState->targetPosition, &playerState->previousPosition, &playerState->targetPosition);
    UpdatePlayerPhysics(playerObject);
    UpdatePlayerMovement(playerObject);
    UpdatePlayerModels(playerObject);
}

void UpdatePlayerMovement(OMOBJ *playerObject)
{
    MinigamePlayerState *playerState;

    playerState = playerObject->data;
    playerState->characterMotionId++;
    playerState->characterState++;
    playerObject->trans.x = playerState->position.x;
    playerObject->trans.y = playerState->position.y;
    playerObject->trans.z = playerState->position.z;
    playerState->jumpSpeed += playerState->fallSpeed;
    if (playerState->jumpSpeed > 0.45f) {
        playerState->jumpSpeed = 0.45f;
    }
    playerState->runSpeed = interpolateAngle(playerState->runSpeed, playerState->walkSpeed, playerState->jumpSpeed);
    playerObject->rot.y = playerState->runSpeed;
}

void UpdatePlayerInput(OMOBJ *playerObject)
{
    Vec playerForward;
    Vec movementDirection;

    Float waterHeight;

    MinigamePlayerState *playerState;

    playerState = playerObject->data;
    {
        Vec worldUp = { 0.0f, 1.0f, 0.0f };
        UpdatePlayerAction(playerObject);
        waterHeight = getWaterHeightAtPosition(playerState->movementTarget.x, playerState->movementTarget.y, playerState->movementTarget.z);
        playerState->movementTarget.y += (0.2f * (waterHeight - playerState->movementTarget.y));
        waterHeight = 0.0f;
        if (playerState->movementAcceleration > 0.05f && playerState->motionEnabled) {
            playerForward.x = sind(90.0f + playerState->spawnPosition.x);
            playerForward.y = 0.0f;
            playerForward.z = cosd(90.0f + playerState->spawnPosition.x);
            movementDirection.x = sind(playerState->movementSpeed);
            movementDirection.y = 0.0f;
            movementDirection.z = cosd(playerState->movementSpeed);
            waterHeight = VECDotProduct(&playerForward, &movementDirection);
            waterHeight *= REFRESH_FREQ * (30.0f * playerState->movementAcceleration);
        }
        playerState->spawnPosition.y += (0.1f * (waterHeight - playerState->spawnPosition.y));
        playerState->spawnPosition.x += playerState->spawnPosition.y;
        if (playerState->spawnPosition.x >= 360.0f) {
            playerState->spawnPosition.x -= 360.0f;
        }
        else if (playerState->spawnPosition.x < 0.0f) {
            playerState->spawnPosition.x += 360.0f;
        }
        MTXRotAxisRad(playerState->modelMatrix, &worldUp, 0.017453292f * playerState->spawnPosition.x);
    }
    playerObject->rot.y = playerState->walkSpeed = playerState->spawnPosition.x - 180.0f;
    playerObject->trans.x = playerState->targetPosition.x = playerState->movementTarget.x = playerState->position.x = playerState->spawnPosition.z * sind(playerState->spawnPosition.x);
    playerObject->trans.z = playerState->targetPosition.z = playerState->movementTarget.z = playerState->position.z = playerState->spawnPosition.z * cosd(playerState->spawnPosition.x);
    if (playerState->controllerButtons & 256) {
        playerState->modelFlags = VERSION_PAL ? 7 : 9;
    }
    if (playerState->modelFlags) {
        playerState->modelFlags--;
    }
    switch (playerState->motionFrame) {
        case 0:
            playerState->motionEnabled = 1;
            playerState->motionTimer = 0;
            if (playerState->movementTimer) {
                SetPlayerMotion(playerObject, 0);
                if (playerState->modelFlags) {
                    playerState->motionEnabled = 0;
                    playerState->effectState = 0;
                    SetPlayerMotion(playerObject, 7);
                    playerState->motionTransitionTimer = VERSION_PAL ? 8 : 9;
                    playerState->motionFrame++;
                }
            }
            break;
        case 1:
            playerState->motionTransitionTimer--;
            if (!playerState->motionTransitionTimer) {
                playerState->motionState = 1;
                SetQuaternionFromAngles(400.0f, 0.3f, &playerState->facingRotation);
                playerState->previousPosition.y = REFRESH_FREQ * playerState->facingRotation.z;
                playerState->animationFrame = 0.0f;
                playerState->motionFrame++;
            }
            break;
        case 2:
            if (playerState->previousPosition.y >= 0.0f) {
                playerState->animationFrame += REFRESH_FREQ;
            }
            else {
                playerState->animationFrame -= REFRESH_FREQ;
            }
            playerState->motionTimer = 0;
            playerState->waterSplashTimer = 0.0f;
            waterHeight = playerState->animationFrame - 0.060000002f;
            if (waterHeight > 0.0f) {
                playerState->motionTimer = 1;
                waterHeight = waterHeight * (VERSION_PAL ? 4.7619047f : 9.009008f);
                if (waterHeight > 1.0f) {
                    waterHeight = 1.0f;
                }
                playerState->waterSplashTimer = waterHeight;
            }
            if (playerState->motionTimer && playerState->controllerButtons & 320) {
                playerState->motionTimer = 0;
                playerState->effectState = 1;
                playerState->previousPosition.y = 0.0f;
                SetPlayerMotion(playerObject, 9);
                playerState->motionFrame++;
            }
            else {
                playerState->previousPosition.y += (REFRESH_FREQ * (REFRESH_FREQ * playerState->facingRotation.w));
                playerState->targetPosition.y += playerState->previousPosition.y;
                if (playerState->targetPosition.y <= playerState->movementTarget.y) {
                    playerState->motionState = 0;
                    playerState->motionTimer = 0;
                    playerState->previousPosition.y = 0.0f;
                    playerState->targetPosition.y = playerState->movementTarget.y;
                    playerState->motionFrame = 0;
                }
            }
            break;
        case 3:
            if (IsPlayerMotionFinished(playerObject)) {
                playerState->motionFrame++;
            }
            break;
        case 4:
            playerState->previousPosition.y += (REFRESH_FREQ * (REFRESH_FREQ * playerState->facingRotation.w));
            playerState->targetPosition.y += +playerState->previousPosition.y;
            if (playerState->targetPosition.y <= playerState->movementTarget.y) {
                SetWaterWave(playerState->movementTarget.x, 0.0f, playerState->movementTarget.z, playerState->waterSplashTimer);
                HuAudFXEmiterPlay(1482, &playerState->targetPosition);
                omVibrate(playerObject->work[0], 12, 4, 2);
                playerState->motionState = 0;
                playerState->previousPosition.y = 0.0f;
                playerState->targetPosition.y = playerState->movementTarget.y;
                SetPlayerMotion(playerObject, 10);
                playerState->motionFrame = playerState->motionFrame + 1;
            }
            break;
        case 5:
            if (IsPlayerMotionFinished(playerObject) != 0) {
                playerState->effectState = 0;
                playerState->motionFrame = 0;
            }
            break;
    }
    if (!playerState->motionState) {
        playerState->targetPosition.y = playerState->movementTarget.y;
    }
}

void UpdatePlayerAction(OMOBJ *playerObject)
{
    Float angleDifference;
    Float separationAngle;

    s32 playerIndex;
    MinigamePlayerState *playerState;
    MinigamePlayerState *otherPlayerState;
    MinigamePlayerState *targetPlayerState;
    s32 otherPlayerIndex;

    playerState = playerObject->data;
    playerState->actionFrameCounter++;
    if (((MinigamePlayerState *)playerObjects[0]->data)->actionFrameSync == playerState->actionFrameCounter) {
        return;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        ((MinigamePlayerState *)playerObjects[playerIndex]->data)->actionFrameSync = playerState->actionFrameCounter;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        otherPlayerState = playerObjects[playerIndex]->data;
        if (!otherPlayerState->characterId) {
            continue;
        }
        for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
            if (playerIndex == otherPlayerIndex) {
                continue;
            }
            targetPlayerState = playerObjects[otherPlayerIndex]->data;
            if (!targetPlayerState->characterId) {
                continue;
            }
            angleDifference = angleDifference(otherPlayerState->spawnPosition.x, targetPlayerState->spawnPosition.x);
            if (fabs(angleDifference) > 23.0) {
                continue;
            }
            if (angleDifference == 0.0f) {
                separationAngle = 11.5f;
            }
            else {
                separationAngle = 23.0 - fabs(angleDifference);
                separationAngle = (0.5f * separationAngle) * (angleDifference / fabs(angleDifference));
            }
            otherPlayerState->spawnPosition.x -= separationAngle;
            targetPlayerState->spawnPosition.x += separationAngle;
        }
    }
}

void UpdatePlayerAnimation(OMOBJ *playerObject)
{
    Qtrn normalizedRotation;
    Vec relativeMovement;
    Vec horizontalMovement;
    Float movementAngle;
    Float horizontalSpeed;

    Float movementDistance;
    Float motionBlendThreshold;
    Float motionPlaybackSpeed;

    MinigamePlayerState *playerState;

    playerState = playerObject->data;
    if (playerState->movementAcceleration > 0.0f) {
        playerState->walkSpeed = playerState->movementSpeed;
    }
    if (playerState->movementAcceleration > 0.05f) {
        playerState->previousPosition.x = (1.0 * REFRESH_FREQ) * (500.0 * (playerState->movementAcceleration * sind(playerState->movementSpeed)));
        playerState->previousPosition.z = (1.0 * REFRESH_FREQ) * (500.0 * (playerState->movementAcceleration * cosd(playerState->movementSpeed)));
        playerState->jumpSpeed = 0.2f;
        playerState->fallSpeed = 0.0f;
        if (CharMotionShiftIDGet(playerState->playerIndex) < 0) {
            motionBlendThreshold = 0.5f;
            if (playerState->actionTimer == 4) {
                motionBlendThreshold = 0.45f;
            }
            if (playerState->movementAcceleration > motionBlendThreshold) {
                SetPlayerMotion(playerObject, 4);
                motionPlaybackSpeed = playerState->movementAcceleration;
            }
            else {
                SetPlayerMotion(playerObject, 3);
                motionPlaybackSpeed = (playerState->movementAcceleration / motionBlendThreshold);
            }
#if VERSION_PAL
            motionPlaybackSpeed *= 1.2f;
#endif
            CharMotionSpeedSet(playerState->playerIndex, motionPlaybackSpeed);
        }
    }
    else {
        playerState->previousPosition.x = playerState->previousPosition.y = playerState->previousPosition.z = 0.0f;
        if (playerState->movementTimer && CharMotionShiftIDGet(playerState->playerIndex) < 0) {
            SetPlayerMotion(playerObject, 0);
        }
    }
    VECSubtract(&playerState->targetPosition, &playerState->movementStart, &relativeMovement);
    if (getSequenceState() == 2) {
        relativeMovement.y += -350.0f * REFRESH_FREQ * playerState->groundHeight;
    }
    MTXMultVec(playerState->targetModelMatrix, &relativeMovement, &relativeMovement);
    VECAdd(&relativeMovement, &playerState->previousPosition, &relativeMovement);
    horizontalMovement = relativeMovement;
    horizontalMovement.y = 0.0f;
    movementDistance = horizontalSpeed = VECMag(&horizontalMovement);
    if (movementDistance > 150.0f) {
        movementDistance = 150.0f / movementDistance;
        relativeMovement.x = relativeMovement.x * movementDistance;
        relativeMovement.z *= movementDistance;
    }
    relativeMovement.y = 80.0f;
    playerState->velocity = relativeMovement;
    movementAngle = atan2d(relativeMovement.x, relativeMovement.z);
    MTXMultVec(playerState->modelMatrix, &relativeMovement, &relativeMovement);
    VECAdd(&relativeMovement, &playerState->movementTarget, &playerState->targetPosition);
    if (playerState->motionSpeed >= 0.95f) {
        playerState->effectTimer = 1;
        return;
    }
    QUATNormalize(&playerState->previousRotation, &normalizedRotation);
    if (normalizedRotation.w < cosd(25)) {
        playerState->effectTimer = 1;
    }
}

void UpdatePlayerPhysics(OMOBJ *playerObject)
{
    Float waterHeight;
    Float waveLift;
    Float rotationProgress;
    Float rotationPhase;
    Float unusedRotationPhase;
    Float normalizedPhase;
    Float phaseFraction;
    Float turnPhase;
    Float turnFraction;

    MinigamePlayerState *playerState;
    s32 pathPointIndex;
    WaterContactInfo *waterContact;
    s32 contactIndex;

    playerState = playerObject->data;
    {
        Quaternion actionRotation;
        Quaternion combinedRotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        Vec movementVelocity;
        Vec movementCorrection;
        Vec rotationAxis = { 0.0f, 0.0f, 0.0f };

        if (getSequenceState() == 2) {
            playerState->groundHeight += VERSION_PAL ? 0.0013333333f : 0.0011111111f;
            if (playerState->groundHeight > 1.0f) {
                playerState->groundHeight = 1.0f;
            }
        }
        playerState->turnRotation = playerState->movementRotation;
        MTXCopy(playerState->previousModelMatrix, playerState->targetModelMatrix);
        playerState->movementStart = playerState->movementTarget;
        waterHeight = getWaterHeight();
        for (pathPointIndex = 0; pathPointIndex < 8; pathPointIndex++) {
            rotationProgress = 45.0f * pathPointIndex;
            waterHeight += getWaterHeightAtPosition(0.5 * (150.0 * sind(rotationProgress)), 0.0f, 0.5 * (150.0 * cosd(rotationProgress)));
        }
        waterHeight *= 0.11111111f;
        playerState->movementTarget.y = playerState->movementTarget.y + playerState->animationSpeed;
        if (waterHeight > playerState->movementTarget.y || (!playerState->motionState && getSequenceState() != 2)) {
#if VERSION_PAL
            playerState->movementTarget.y = playerState->movementTarget.y + (0.16666667f * (waterHeight - playerState->movementTarget.y));
#else
            playerState->movementTarget.y = playerState->movementTarget.y + (0.1388889f * (waterHeight - playerState->movementTarget.y));
#endif
            playerState->animationSpeed = playerState->movementTarget.y - playerState->movementStart.y;
            playerState->animationSpeed += -980.0f * REFRESH_FREQ * REFRESH_FREQ;
            playerState->motionState = 0;
        }
        else {
            playerState->animationSpeed = playerState->movementTarget.y - playerState->movementStart.y;
            playerState->animationSpeed += VERSION_PAL ? -1.176f : -0.8166667f;
            playerState->motionState = 1;
        }
        if (playerState->actionIndex) {
            playerState->turnAngle += REFRESH_FREQ;
            if (playerState->turnAngle > 2.0f) {
                playerState->turnAngle = 2.0f;
            }
        }
        playerState->movementAngle += REFRESH_FREQ;
        rotationPhase = 0.1f * playerState->movementAngle;
        normalizedPhase = fmod(rotationPhase, 1.0);
        phaseFraction = normalizedPhase;
        playerState->movementDistance = 360.0f * phaseFraction;
        unusedRotationPhase = playerState->movementAngle;
        turnPhase = fmod(unusedRotationPhase, 1.0);
        turnFraction = turnPhase;
        playerState->turnSpeed = 0.04712389037013054 * sind((360.0f * turnFraction));

        rotationAxis.x = sind(playerState->movementDistance);
        rotationAxis.y = 0.0f;
        rotationAxis.z = cosd(playerState->movementDistance);
        C_QUATRotAxisRad(&playerState->startRotation, &rotationAxis, playerState->turnSpeed * playerState->turnAngle);
        if (!playerState->actionIndex) {
            movementVelocity = playerState->velocity;
            movementVelocity.y = 0.0f;
            waterHeight = (2.0f / 300.0f) * VECMag(&movementVelocity);
            if (waterHeight <= 0.0f) {
                waterHeight = 0.0f;
                rotationAxis.x = 1.0f;
                rotationAxis.y = rotationAxis.z = 0.0f;
            }
            else {
                if (waterHeight > 1.0f) {
                    waterHeight = 1.0f;
                }
                rotationAxis.x = movementVelocity.z;
                rotationAxis.y = 0.0f;
                rotationAxis.z = -movementVelocity.x;
            }
            C_QUATRotAxisRad(&playerState->targetRotation, &rotationAxis, 0.09424778f * waterHeight);
            if (!playerState->motionState && !playerState->actionIndex) {
                for (waterContact = GetWaterContactInfo(); waterContact->playerIndex > -2; waterContact++) {
                    if (waterContact->playerIndex >= 0 && waterContact->playerIndex < 16) {
                        contactIndex = waterContact->playerIndex;
                        waterHeight = waterContact->actionTimer * waterContact->characterId;
                        if (playerState->playerPathPoints[contactIndex].y <= waterHeight) {
                            playerState->playerPathPoints[contactIndex].y += VERSION_PAL ? 0.05333333f : 0.044444446f;
                            playerState->playerPathPoints[contactIndex].x = waterContact->actionState;
                        }
                        if (getSequenceState() == 2 && playerState->playerPathPoints[contactIndex].z <= waterHeight) {
                            playerState->playerPathPoints[contactIndex].z += VERSION_PAL ? 0.10666666f : 0.08888889f;
                        }
                    }
                }
            }
        }
        combinedRotation.w = 1.0f;
        combinedRotation.x = combinedRotation.y = combinedRotation.z = 0.0f;

        movementVelocity.x = movementVelocity.y = movementVelocity.z = 0.0f;

        waveLift = 0.0f;
        for (pathPointIndex = 0; pathPointIndex < 16; pathPointIndex++) {
            if (playerState->playerPathPoints[pathPointIndex].z > 0.0f) {
                waveLift += 0.5f * playerState->playerPathPoints[pathPointIndex].z;
                if (playerState->motionState == 0) {
                    playerState->playerPathPoints[pathPointIndex].z -= REFRESH_FREQ;
                    if (playerState->playerPathPoints[pathPointIndex].z < 0.0f) {
                        playerState->playerPathPoints[pathPointIndex].z = 0.0f;
                    }
                }
            }
            if (playerState->playerPathPoints[pathPointIndex].y > 0.0f) {
                movementVelocity.x += playerState->playerPathPoints[pathPointIndex].y * sind(playerState->playerPathPoints[pathPointIndex].x);
                movementVelocity.z += playerState->playerPathPoints[pathPointIndex].y * cosd(playerState->playerPathPoints[pathPointIndex].x);
                playerState->playerPathPoints[pathPointIndex].y -= VERSION_PAL ? 0.015384615f : 0.012820514f;
                if (playerState->playerPathPoints[pathPointIndex].y < 0.0f) {
                    playerState->playerPathPoints[pathPointIndex].y = 0.0f;
                }
            }
        }
        if (waveLift > 1.0f) {
            waveLift = 1.0f;
        }
        waveLift *= REFRESH_FREQ;
        waveLift *= playerState->groundHeight;
        playerState->fallDistance = playerState->fallDistance + (0.1f * (waveLift - playerState->fallDistance));
        waterHeight = 90.0 + (atan2d(movementVelocity.x, movementVelocity.z));
        rotationAxis.x = sind(waterHeight);
        rotationAxis.y = 0.0f;
        rotationAxis.z = cosd(waterHeight);
        rotationProgress = VECMag(&movementVelocity);
        if (rotationProgress > 0.5f) {
            rotationProgress = 0.5f;
        }
        C_QUATRotAxisRad(&combinedRotation, &rotationAxis, 3.1415927f * rotationProgress * 0.18f);
        if (!playerState->actionIndex) {
            if (getSequenceState() == 2) {
                movementVelocity = playerState->velocity;
                movementVelocity.y = 0.0f;
                waterHeight = VECMag(&movementVelocity);
                rotationProgress = 0.006666667f * waterHeight;
                if (waterHeight > 0.0f) {
                    VECNormalize(&movementVelocity, &movementVelocity);
                }
                VECSubtract(&movementVelocity, &playerState->movementVector, &movementCorrection);
                if (VECMag(&movementCorrection)) {
                    VECNormalize(&movementCorrection, &movementCorrection);
                }
                VECScale(&movementCorrection, &movementCorrection, ((REFRESH_FREQ * 0.2f) * rotationProgress) + playerState->fallDistance);
                VECAdd(&playerState->movementVector, &movementCorrection, &playerState->movementVector);
            }
            else {
                VECScale(&playerState->movementVector, &playerState->movementVector, 0.98f);
            }
            playerState->movementVector.y = 0.0f;
            waterHeight = VECMag(&playerState->movementVector);
            if (waterHeight > 1.0f) {
                waterHeight = 1.0f / waterHeight;
                playerState->movementVector.x *= waterHeight;
                playerState->movementVector.z *= waterHeight;
                waterHeight = 1.0f;
            }
            playerState->motionSpeed = waterHeight;
            playerState->jumpHeight = atan2d(playerState->movementVector.x, playerState->movementVector.z);
            if (waterHeight > 0.0f) {
                rotationAxis.x = playerState->movementVector.z;
                rotationAxis.y = 0.0f;
                rotationAxis.z = -playerState->movementVector.x;
            }
            else {
                rotationAxis.x = 1.0f;
                rotationAxis.y = rotationAxis.z = 0.0f;
            }
            C_QUATRotAxisRad(&playerState->currentRotation, &rotationAxis, 0.9424779f * waterHeight);
        }
        else {
            rotationAxis.x = playerState->movementVector.z;
            rotationAxis.y = 0.0f;
            rotationAxis.z = -playerState->movementVector.x;
            C_QUATRotAxisRad(&actionRotation, &rotationAxis, M_PI);
            C_QUATSlerp(&playerState->currentRotation, &actionRotation, &playerState->currentRotation, (5.0f / 3.0f) * REFRESH_FREQ);
        }
        QUATMultiply(&playerState->targetRotation, &playerState->currentRotation, &playerState->previousRotation);
        QUATMultiply(&playerState->startRotation, &playerState->previousRotation, &playerState->previousRotation);
        QUATMultiply(&combinedRotation, &playerState->previousRotation, &playerState->previousRotation);
        MTXQuat(playerState->modelMatrix, &playerState->previousRotation);
        MTXInverse(playerState->modelMatrix, playerState->previousModelMatrix);
        QUATInverse(&playerState->previousRotation, &playerState->movementRotation);
    }
}

void UpdatePlayerModels(OMOBJ *playerObject)
{
    MinigamePlayerState *playerState;

    playerState = playerObject->data;
    Hu3DModelPosSetV(playerObject->mdlId[0], &playerState->targetPosition);
    Hu3DModelRotSet(playerObject->mdlId[0], 0.0f, playerState->runSpeed, 0.0f);
    Hu3DModelPosSetV(playerObject->mdlId[1], &playerState->movementTarget);
    MTXCopy(playerState->modelMatrix, Hu3DData[playerObject->mdlId[1]].mtx);
}

void UpdateComputerMovementTarget(OMOBJ *playerObject)
{
    Vec desiredMovement;
    Vec currentMovement;
    Vec movementDifference;

    Float movementDistance;
    Float movementBlend;

    MinigamePlayerState *playerState;
    s32 useStrictThreshold;
    s32 thresholdIndex;

    playerState = playerObject->data;
    {
        Float distanceThreshold[2] = { 30.000002f, 80.0f };

        if (!playerState->computerMovementTimer) {
            desiredMovement = playerState->movementVector;
            desiredMovement.y = 0.0f;
            movementDistance = 0.75f + (2.5f * (playerState->difficultyScale * playerState->difficultyScale));
            VECScale(&desiredMovement, &desiredMovement, 150.0f * -movementDistance);
            movementDistance = VECMag(&desiredMovement);
            if (movementDistance > 150.0f) {
                movementDistance = 150.0f / movementDistance;
                desiredMovement.x *= movementDistance;
                desiredMovement.z *= movementDistance;
            }
            desiredMovement.x += (100.0 * ((0.9f - (0.3f * playerState->difficultyScale)) * ((0.002 * (Float)frandmod(1000)) - 1.0)));
            desiredMovement.z += (100.0 * ((0.9f - (0.3f * playerState->difficultyScale)) * ((0.002 * (Float)frandmod(1000)) - 1.0)));
            currentMovement = playerState->movementDelta;
            currentMovement.y = 0.0f;
            VECSubtract(&desiredMovement, &currentMovement, &movementDifference);
            movementDistance = VECMag(&movementDifference);
            if (movementDistance > (100.0f * (1.2f - (0.3f * playerState->difficultyScale)))) {
                playerState->movementDelta = desiredMovement;
            }
            playerState->computerMovementTimer = REFRESH_RATE_F * ((0.23f - (0.04f * playerState->difficultyScale)) + (0.001f * ((0.4f - (0.15f * playerState->difficultyScale)) * frandmod(1000))));
        }
        else {
            playerState->computerMovementTimer--;
        }
        desiredMovement = playerState->movementDelta;
        desiredMovement.y = 0.0f;
        currentMovement = playerState->velocity;
        currentMovement.y = 0.0f;
        VECSubtract(&desiredMovement, &currentMovement, &movementDifference);
        movementDistance = VECMag(&movementDifference);
        if (playerState->controllerPort) {
            useStrictThreshold = 1;
        }
        else {
            useStrictThreshold = 0;
        }
        thresholdIndex = useStrictThreshold;
        if (movementDistance > distanceThreshold[thresholdIndex]) {
            playerState->controllerPort = 0;
            playerState->movementSpeed = interpolateAngle(playerState->runSpeed, atan2d(movementDifference.x, movementDifference.z), 0.5f);
            movementBlend = 0.014285714f * movementDistance;
            if (movementBlend > 1.0f) {
                movementBlend = 1.0f;
            }
            playerState->movementAcceleration = movementBlend;
            return;
        }
        playerState->controllerPort = 1;
        playerState->movementDelta = playerState->velocity;
    }
}

void UpdateComputerActionTiming(OMOBJ *playerObject)
{
    MinigamePlayerState *playerState;
    s32 startActionDelay;

    playerState = playerObject->data;
    startActionDelay = 0;
    if (playerState->computerRestTimer) {
        playerState->computerRestTimer--;
    }
    else {
        playerState->rotationSpeed = 1.0f;
        if (frandmod(1000) < 500) {
            playerState->rotationSpeed = -1.0f;
        }
        playerState->movementScale = 0.001f * frandmod(1000);
        playerState->computerRestTimer = (VERSION_PAL ? 0.125f : 0.15f) * frandmod(1000);
    }
    playerState->movementSpeed = playerState->spawnPosition.x + (90.0f * playerState->rotationSpeed);
    playerState->movementAcceleration = 0.5f + (0.5f * playerState->difficultyScale);
    if (lastComputerPlayerIndex >= playerObject->work[0]) {
        if (computerActionTimer) {
            computerActionTimer--;
        }
        else {
            computerActionTimer = REFRESH_RATE_F * (8.0f + (0.004f * frandmod(1000)));
        }
    }
    lastComputerPlayerIndex = playerObject->work[0];
    if (playerState->motionEnabled) {
        if (!playerState->computerActionActive && computerActionTimer < REFRESH_RATE_F * 2.2f && computerActionTimer > REFRESH_RATE_F / 2) {
            playerState->computerActionTimer = 0;
            startActionDelay = 1;
        }
        if (!playerState->computerActionTimer) {
            if (startActionDelay) {
                playerState->computerActionTimer = computerActionTimer;
                playerState->computerActionTimer -= (REFRESH_RATE_F * (0.16f + (0.6f * (0.1f + (0.5f * playerState->difficultyScale)))));
                playerState->computerActionActive = 1;
                if (playerState->computerActionTimer > REFRESH_RATE_F * 4) {
                    playerState->computerActionTimer = 1;
                }
            }
            else {
                playerState->computerActionTimer = REFRESH_RATE_F * (0.00080000004f * ((1.05f - playerState->difficultyScale) * frandmod(1000)));
                playerState->computerActionTimer += (playerState->movementScale * ((REFRESH_RATE_F / 2) + ((REFRESH_RATE_F / 2000) * frandmod(1000))));
            }
        }
        else {
            playerState->computerActionTimer--;
            if (!playerState->computerActionTimer) {
                playerState->controllerButtons |= 256;
                playerState->gravity = (0.5f * playerState->difficultyScale) + (((1.2f * playerState->difficultyScale) / 1000.0f) * frandmod(1000));
                if (playerState->gravity < 0.25f) {
                    playerState->gravity = 0.25f;
                }
                if (playerState->computerActionActive) {
                    playerState->gravity = 0.0f;
                }
                playerState->computerActionActive = 0;
            }
        }
    }
    if (playerState->motionTimer && ((playerState->previousPosition.y < 0.0f && playerState->waterSplashTimer < playerState->gravity) || !computerActionTimer)) {
        playerState->controllerButtons |= 256;
    }
}

void SetPlayerMotion(OMOBJ *playerObject, u32 motionIndex)
{
    Float motionBlendDuration;

    MinigamePlayerState *playerState;

    playerState = playerObject->data;
    if (playerState->actionTimer != motionIndex && motionIndex < 11) {
        motionBlendDuration = REFRESH_RATE_F * playerMotionSettings[motionIndex].actionState;
        if (playerState->actionTimer < 0) {
            motionBlendDuration = 0.0f;
        }
        playerState->actionTimer = motionIndex;
        CharMotionShiftSet(
            playerState->playerIndex,
            playerObject->mtnId[playerMotionSettings[motionIndex].playerIndex],
            REFRESH_RATE_F * playerMotionSettings[motionIndex].actionTimer,
            motionBlendDuration,
            playerMotionSettings[motionIndex].playerFlags
        );
        if (playerMotionSettings[motionIndex].characterId >= 0.0f) {
            Hu3DMotionShiftStartEndSet(
                *playerObject->mdlId,
                REFRESH_RATE_F * playerMotionSettings[motionIndex].actionTimer,
                REFRESH_RATE_F * playerMotionSettings[motionIndex].characterId
            );
        }
#if VERSION_PAL
        CharMotionSpeedSet(playerState->playerIndex, 1.2f);
#endif
    }
}

s32 IsPlayerMotionFinished(OMOBJ *object)
{
    MinigamePlayerState *playerState;
    s32 motionFinished;

    playerState = object->data;
    motionFinished = 0;
    if (CharMotionEndCheck(playerState->playerIndex) && CharMotionShiftIDGet(playerState->playerIndex) < 0) {
        motionFinished = 1;
    }
    return motionFinished;
}

s32 SetPlayerMotionIfFinished(OMOBJ *playerObject, u32 motionIndex)
{
    s32 motionWasSet;

    motionWasSet = 0;
    if (IsPlayerMotionFinished(playerObject)) {
        SetPlayerMotion(playerObject, motionIndex);
        motionWasSet = 1;
    }
    return motionWasSet;
}

void SetQuaternionFromAngles(Float rotationAngle, Float rotationScale, Quaternion *rotation)
{
    rotation->x = rotationScale;
    rotation->y = rotationAngle;
    rotation->w = (2.0f * -rotationAngle) / (rotationScale * rotationScale);
    rotation->z = -rotation->w * rotationScale;
}

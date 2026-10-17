#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/sprite.h"

#include <string.h>

#include "REL/m410Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/frand.h"
#endif

OMOBJ *playerManagerObject;
ANIMDATA *playerSpriteAnimation;
s32 playerOrderByRandomRank[4];

char *playerHookDummyNames[] = { "dmy-itemhook-r", "dmy-itemhook-r", "dmy-itemhook-r", "dmy-itemhook-r", "dmy-itemhook-r", "dmy-itemhook-r",
    "dmy-itemhook-r", "dmy-itemhook-r" };

char *playerHookNames[] = { "itemhook-r", "itemhook-r", "itemhook-r", "itemhook-r", "itemhook-r", "itemhook-r", "itemhook-r", "itemhook-r" };

s32 playerModelDataIds[] = {
    DATA_MAKE_NUM(DATADIR_M410, 0),
    DATA_MAKE_NUM(DATADIR_M410, 1),
    DATA_MAKE_NUM(DATADIR_M410, 2),
    DATA_MAKE_NUM(DATADIR_M410, 3),
    DATA_MAKE_NUM(DATADIR_M410, 4),
    DATA_MAKE_NUM(DATADIR_M410, 5),
    DATA_MAKE_NUM(DATADIR_M410, 6),
    DATA_MAKE_NUM(DATADIR_M410, 7),
};

u32 playerMotionDataIds[] = {
    0,
    DATA_MAKE_NUM(DATADIR_M410, 70),
    DATA_MAKE_NUM(DATADIR_M410, 22),
    DATA_MAKE_NUM(DATADIR_M410, 30),
    DATA_MAKE_NUM(DATADIR_M410, 38),
    DATA_MAKE_NUM(DATADIR_M410, 46),
    DATA_MAKE_NUM(DATADIR_M410, 62),
    DATA_MAKE_NUM(DATADIR_M410, 54),
    DATA_MAKE_NUM(DATADIR_E3SETUP, 23),
    DATA_MAKE_NUM(DATADIR_E3SETUP, 24),
};

PlayerMotionTiming playerMotionTimings[] = {
    { 0.0f, 0.0f, REFRESH_RATE_F * 0.2f, HU3D_MOTATTR_LOOP },
    { 0.0f, 0.0f, REFRESH_RATE_F * 0.02f, HU3D_MOTATTR_NONE },
    { 0.0f, 0.0f, REFRESH_RATE_F * 0.2f, HU3D_MOTATTR_LOOP },
    { 0.0f, 0.0f, REFRESH_RATE_F * 0.1f, HU3D_MOTATTR_NONE },
    { REFRESH_RATE_F * 0.1f, 0.0f, REFRESH_RATE_F * 0.08f, HU3D_MOTATTR_NONE },
    { 0.0f, 0.0f, REFRESH_RATE_F * 0.02f, HU3D_MOTATTR_NONE },
    { 0.0f, 0.0f, REFRESH_RATE_F * 0.02f, HU3D_MOTATTR_NONE },
    { 0.0f, 0.0f, REFRESH_RATE_F * 0.02f, HU3D_MOTATTR_NONE },
    { 0.0f, 0.0f, REFRESH_RATE_F * 0.2f, HU3D_MOTATTR_NONE },
    { 0.0f, 0.0f, REFRESH_RATE_F * 0.2f, HU3D_MOTATTR_NONE },
};

s32 playerCharacterDataIds[] = {
    DATA_MAKE_NUM(DATADIR_MGCONST, 0),
    DATA_MAKE_NUM(DATADIR_MGCONST, 1),
    DATA_MAKE_NUM(DATADIR_MGCONST, 2),
    DATA_MAKE_NUM(DATADIR_MGCONST, 3),
    DATA_MAKE_NUM(DATADIR_MGCONST, 4),
    DATA_MAKE_NUM(DATADIR_MGCONST, 5),
    DATA_MAKE_NUM(DATADIR_MGCONST, 6),
    DATA_MAKE_NUM(DATADIR_MGCONST, 7),
};

GXColor playerDisplayColors[] = {
    { 102, 0, 0, 255 },
    { 0, 0, 102, 255 },
    { 102, 0, 91, 255 },
    { 0, 102, 0, 255 },
    { 66, 0, 76, 255 },
    { 71, 40, 0, 255 },
    { 102, 102, 0, 255 },
    { 0, 0, 0, 255 },
};

void CreatePlayerObject(OMOBJ *object);
void UpdatePlayerMovement(OMOBJ *object);
void UpdatePlayerAnimationState(OMOBJ *object);
void UpdatePlayerCollisionState(OMOBJ *object);
void UpdatePlayerGoalState(OMOBJ *object);
void UpdatePlayerPhysics(OMOBJ *object);
void SetPlayerMotion(OMOBJ *object, u32 motionId);
void UpdatePlayerController(OMOBJ *object);
s32 UpdatePlayerAi(OMOBJ *object);
s32 UpdatePlayerAnimation(OMOBJ *object);
s32 UpdatePlayerInput(OMOBJ *object);
s32 SetPlayerAnimation(OMOBJ *object, u32 animationId);
void CreatePlayerManager(OMOBJ *object);
void UpdatePlayerManager(OMOBJ *object);
void SetPlayerScore(u16 playerIndex, u16 scoreDelta, u16 scoreType);
void BuildPlayerDisplayList(PlayerDisplayListWork *displayListWork);
void UpdatePlayerModelMaterial(HU3DMODEL *model, Mtx transform);

void InitializePlayers(HUPROCESS *objman)
{
    u32 temporaryValue[4];
    u32 temporaryValue2[4];

    s16 loopIndex;
    s16 loopIndex2;
    u32 stateValue;
    OMOBJ *object;

    for (loopIndex = 0; loopIndex < 4; loopIndex++) {
        temporaryValue[loopIndex] = frandmod(1000);
        temporaryValue2[loopIndex] = loopIndex;
    }

    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        for (loopIndex2 = loopIndex + 1; loopIndex2 < 4; loopIndex2++) {
            if (temporaryValue[loopIndex] > temporaryValue[loopIndex2]) {
                stateValue = temporaryValue[loopIndex];
                temporaryValue[loopIndex] = temporaryValue[loopIndex2];
                temporaryValue[loopIndex2] = stateValue;
                stateValue = temporaryValue2[loopIndex];
                temporaryValue2[loopIndex] = temporaryValue2[loopIndex2];
                temporaryValue2[loopIndex2] = stateValue;
            }
        }
    }

    for (loopIndex = 0; loopIndex < 4; loopIndex++) {
        playerOrderByRandomRank[temporaryValue2[loopIndex]] = loopIndex;
    }
    omMakeGroupEx(objman, 0, 4);
    omGetGroupMemberListEx(objman, 0);

    for (loopIndex = 0; loopIndex < 4; loopIndex++) {
        object = omAddObjEx(objman, 40, 2, 10, 0, CreatePlayerObject);
        object->work[0] = loopIndex;
    }
    playerManagerObject = omAddObjEx(objman, 10, 1, 0, -1, CreatePlayerManager);
    playerSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_M410, 19), HU_MEMNUM_OVL));
}

void CleanupPlayers(void)
{
    HuSprAnimKill(playerSpriteAnimation);
    CharModelKill(-1);
}

void CreatePlayerObject(OMOBJ *object)
{
    s32 temporaryValue;

    PlayerCharacterWork *playerWork;
    s16 loopIndex;
    s32 stateValue;
    s32 stateValue2;

    PlayerFloatParameters temporaryValue = { { 0.0f, 0.13f, 0.26f, 0.4f } };
    object->data = HuMemDirectMallocNum(HEAP_HEAP, 148, HU_MEMNUM_OVL);
    playerWork = object->data;
    memset(playerWork, 0, 148);
    stateValue2 = object->work[0];
    stateValue = GWPlayerCfg[stateValue2].character;
    playerWork->characterModelId = stateValue;
    playerWork->randomizedPlayerRank = playerOrderByRandomRank[object->work[0]];
    temporaryValue = playerWork->hookIndex = 0;
    playerWork->movementProbability = temporaryValue.movementChanceByDifficulty[GWPlayerCfg[stateValue2].diff & 3];
    object->mdlId[0] = CharModelCreate(stateValue, 2);
    Hu3DModelLayerSet(object->mdlId[0], 4);
    Hu3DModelShadowSet(object->mdlId[0]);
    object->mdlId[1] = Hu3DModelCreateFile(playerModelDataIds[stateValue]);
    Hu3DModelShadowSet(object->mdlId[1]);
    Hu3DModelLayerSet(object->mdlId[1], 4);

    for (loopIndex = 0; loopIndex < 10; loopIndex++) {
        if (playerMotionDataIds[loopIndex] < 0x10000) {
            object->mtnId[loopIndex] = CharMotionCreate(stateValue, playerMotionDataIds[loopIndex]);
        }
        else {
            object->mtnId[loopIndex] = Hu3DJointMotionFile(object->mdlId[0], stateValue + playerMotionDataIds[loopIndex]);
        }
    }
    CharMotionDataClose(stateValue);
    object->trans.x = playerWork->worldPosition.x = 2.0f * playerWork->randomizedPlayerRank * 100.0f - 300.0f;
    object->trans.y = playerWork->worldPosition.y = 0.0f;
    object->trans.z = playerWork->worldPosition.z = 0.0f;

    playerWork->reservedFloat64 = playerWork->verticalVelocity = playerWork->reservedFloat6c = 0.0f;
    playerWork->currentFacingAngle = playerWork->targetFacingAngle = 0.0f;
    playerWork->facingAngleStep = 0.0f;
    playerWork->facingAngleAcceleration = 0.02f;
    playerWork->modelAnchorPosition.x = playerWork->worldPosition.x;
    playerWork->modelAnchorPosition.y = 100.0f + playerWork->worldPosition.y;
    playerWork->modelAnchorPosition.z = 400.0f + playerWork->worldPosition.z;
    playerWork->resetModelPose = 1;
    playerWork->currentMotionId = -1;
    SetPlayerMotion(object, 0);
    Hu3DModelPosSet(object->mdlId[1], playerWork->modelAnchorPosition.x, playerWork->modelAnchorPosition.y, playerWork->modelAnchorPosition.z);
    playerWork->animationState = 0;
    object->objFunc = UpdatePlayerMovement;
}

void UpdatePlayerMovement(OMOBJ *object)
{
    PlayerCharacterWork *playerWork;

    playerWork = object->data;
    {
        PlayerIntegerParameters temporaryValue = { {
            7 * REFRESH_RATE / 10,
            13 * REFRESH_RATE / 10,
            18 * REFRESH_RATE / 10,
            25 * REFRESH_RATE / 12,
        } };

        switch (playerWork->animationState) {
            case 0:
                SetPlayerAnimation(object, 0);
                if (getMinigamePhase()) {
                    playerWork->animationTimer = 0;
                    playerWork->animationState++;
                }
                break;
            case 1:
                if (playerWork->animationTimer >= temporaryValue.playerWaitFrames[playerWork->randomizedPlayerRank]) {
                    playerWork->movementActive = 1;
                    playerWork->animationState++;
                }
                break;
            case 2:
                UpdatePlayerController(object);
                if (playerWork->animationTimer >= (2.8f + 0.00040000002f * frandmod(1000)) * REFRESH_RATE_F) {
                    playerWork->alternateActionReady = 1;
                    playerWork->animationTimer = 0;
                    playerWork->animationState++;
                }
                break;
            case 3:
                if (playerWork->specialInputReady) {
                    if (playerWork->animationTimer >=  9 * REFRESH_RATE_F / 20) {
                        playerWork->actionRequested = 1;
                    }
                }
                if (UpdatePlayerAi(object)) {
                    playerWork->animationState++;
                }
                break;
            case 4:
                SetPlayerAnimation(object, 0);
                break;
        }
    }
    UpdatePlayerGoalState(object);
    playerWork->animationTimer++;
    if (getMinigamePhase() == 2) {
        playerWork->movementActive = 1;
        playerWork->animationState = 0;
        object->objFunc = UpdatePlayerAnimationState;
    }
}

void UpdatePlayerAnimationState(OMOBJ *object)
{
    float motionValue;
    float motionValue2;
    float motionValue3;
    PlayerCharacterWork *playerWork;
    s32 stateValue;

    playerWork = object->data;
    stateValue = 0;
    playerWork->pressedButtons = 0;
    if (getMinigameSubstate() == 2) {
        playerWork->pressedButtons = HuPadBtnDown[GWPlayerCfg[object->work[0]].padIdx];
        if (GWPlayerCfg[object->work[0]].iscom) {
            UpdatePlayerPhysics(object);
        }
    }
    switch (playerWork->animationState) {
        case 0:
            if (SetPlayerAnimation(object, 0)) {
                playerWork->movementActive = 1;
            }
            UpdatePlayerController(object);
            if (playerWork->isGrounded) {
                playerWork->animationState++;
            }
            break;
        case 1:
            if (!playerWork->isJumping && !playerWork->isStunned) {
                if (playerWork->pressedButtons & 0x100) {
                    if (getMinigameSubstate() == 2) {
                        playerWork->alternateActionReady = 1;
                    }
                }
                else if (playerWork->pressedButtons & 0x200 && getMinigameSubstate() == 2) {
                    if (fabs(angleDifference(playerWork->currentFacingAngle, playerWork->targetFacingAngle)) < 45.0) {
                        playerWork->actionRequested = 1;
                    }
                }
                stateValue = 1;
            }
            else if (playerWork->specialInputReady && playerWork->pressedButtons & 0x200) {
                playerWork->actionRequested = 1;
            }
            if (UpdatePlayerAi(object) || UpdatePlayerAnimation(object)) {
                playerWork->stateTimer = 0;
                playerWork->animationState--;
            }
            break;
    }
    UpdatePlayerGoalState(object);
    if (getMinigameSubstate() == 3 && stateValue) {
        SetMinigameControlFlag(object->work[0]);
        playerWork->animationState = 0;
        object->objFunc = UpdatePlayerCollisionState;
    }
}

void UpdatePlayerCollisionState(OMOBJ *object)
{
    PlayerCharacterWork *playerWork;

    playerWork = object->data;
    UpdatePlayerGoalState(object);
    switch (playerWork->animationState) {
        case 0:
            playerWork->animationState++;
            return;
        case 1:
            if (getMinigameFlags() == 1) {
                Hu3DModelHookReset(object->mdlId[0]);
                Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
                if ((1 << object->work[0]) & getMinigameControlFlags()) {
                    SetPlayerMotion(object, 8);
                }
                else {
                    SetPlayerMotion(object, 9);
                }
                playerWork->targetFacingAngle = 0.0f;
                playerWork->facingAngleStep = 0.0f;
                playerWork->facingAngleAcceleration = 0.005f;
                playerWork->animationState++;
            }
            break;
        case 2:
            if (UpdatePlayerInput(object)) {
                SetMinigamePlayerFlag(object->work[0]);
                playerWork->animationState++;
            }
            break;
        default:
            break;
    }
}

void UpdatePlayerGoalState(OMOBJ *object)
{
    PlayerCharacterWork *playerWork;

    playerWork = object->data;
    playerWork->stateTimer++;
    object->trans.x = playerWork->worldPosition.x;
    object->trans.y = playerWork->worldPosition.y;
    object->trans.z = playerWork->worldPosition.z;

    playerWork->facingAngleStep += playerWork->facingAngleAcceleration;
    if (playerWork->facingAngleStep > 0.45f) {
        playerWork->facingAngleStep = 0.45f;
    }
    playerWork->currentFacingAngle = approachAngle(playerWork->currentFacingAngle, playerWork->targetFacingAngle, playerWork->facingAngleStep);
    object->rot.y = playerWork->currentFacingAngle;
}

void UpdatePlayerPhysics(OMOBJ *object)
{
    Vec temporaryPosition;
    Vec temporaryPosition2;

    float motionValue;
    float motionValue2;
    float motionValue3;
    float motionValue4;
    float motionValue5;

    PlayerCharacterWork *playerWork;
    s32 loopIndex;
    s32 stateValue;

    playerWork = object->data;
    playerWork->pressedButtons = 0;
    switch (playerWork->aiDecisionState) {
        case 0:
            if (!playerWork->isGrounded) {
                return;
            }
            if (frandmod(1000) <= 1000.0f * playerWork->movementProbability) {
                motionValue4 = 7 * REFRESH_RATE_F / 10;
                if (frandmod(1000) <= 1000.0f * playerWork->movementProbability) {
                    motionValue3 = 280.0f;
                    playerWork->aiActionDuration = REFRESH_RATE_F * 0.53f + frandmod(REFRESH_RATE / 10);
                }
                else {
                    motionValue3 = 419.99997f;
                    playerWork->aiActionDuration = REFRESH_RATE_F * 0.15f + frandmod(REFRESH_RATE / 4);
                }
                motionValue5 = motionValue4 + playerWork->aiActionDuration;
                stateValue = -1;
                motionValue = 10000.0f;
                for (loopIndex = 0; loopIndex < 18; loopIndex++) {
                    GetPlayerFuturePosition(loopIndex, (s32)motionValue5, &temporaryPosition2);
                    if (fabs(temporaryPosition2.y - motionValue3) > 10.0) {
                        continue;
                    }
                    GetPlayerVelocity(loopIndex, &temporaryPosition);
                    if (fabs(temporaryPosition.x) < 115 * REFRESH_FREQ) {
                        continue;
                    }
                    motionValue2 = temporaryPosition2.x - playerWork->previousTargetPosition.x;
                    if (temporaryPosition.x >= 0.0f) {
                        if (motionValue2 < 0.0f) {
                            if (fabs(motionValue2) < motionValue) {
                                stateValue = loopIndex;
                                motionValue = fabs(motionValue2);
                            }
                        }
                    }
                    else if (motionValue2 > 0.0f) {
                        if (fabs(motionValue2) < motionValue) {
                            stateValue = loopIndex;
                            motionValue = fabs(motionValue2);
                        }
                    }
                }
                if (stateValue < 0) {
                    return;
                }
                GetPlayerVelocity(stateValue, &temporaryPosition);
                motionValue /= fabs(temporaryPosition.x);
                playerWork->aiDecisionDelay = motionValue5 + motionValue - motionValue4 - playerWork->aiActionDuration;
            }
            else {
                playerWork->aiDecisionDelay = (2 * REFRESH_RATE_F / 5) + frandmod(6 * REFRESH_RATE / 10);
                playerWork->aiActionDuration = (1 * REFRESH_RATE_F / 5) + frandmod(3 * REFRESH_RATE / 10);
            }
            playerWork->aiDecisionState++;
            return;
        case 1:
            if (!playerWork->aiDecisionDelay) {
                playerWork->pressedButtons |= 256;
                playerWork->aiDecisionState++;
            }
            playerWork->aiDecisionDelay--;
            return;
        case 2:
            if (!playerWork->aiActionDuration) {
                playerWork->pressedButtons |= 512;
                playerWork->aiDecisionState = 0;
            }
            playerWork->aiActionDuration--;
            break;
    }
}

void UpdatePlayerEffects(OMOBJ *object)
{
    Mtx temporaryTransform;
    Vec temporaryPosition;
    Vec temporaryPosition2;
    Vec temporaryPosition3;
    Vec temporaryPosition4;

    float motionValue;
    float motionValue2;
    float motionValue3;
    float motionValue4;
    float motionValue5;
    float motionValue6;
    float motionValue7;
    float motionValue8;
    float motionValue9;
    float motionValue10;
    double motionValue11;
    double motionValue12;

    PlayerCharacterWork *playerWork;
    s32 loopIndex;
    s32 stateValue;
    s16 stateValue2;
    Vec *temporaryPosition;
    Vec *temporaryPosition2;

    playerWork = object->data;
    FindModelJointMatrix(object->mdlId[0], playerHookNames[playerWork->hookIndex], temporaryTransform);

    playerWork->modelAnchorPosition.x = temporaryTransform[0][3];
    playerWork->modelAnchorPosition.y = temporaryTransform[1][3];
    playerWork->modelAnchorPosition.z = temporaryTransform[2][3];

    if (getMinigamePhase() == 1) {
        playerWork->previousTargetPosition = playerWork->modelAnchorPosition;
        stateValue = -1;
        motionValue2 = 10000.0f;

        for (loopIndex = 0; loopIndex < 18; loopIndex++) {
            GetPlayerPosition(loopIndex, &temporaryPosition2);
            if (temporaryPosition2.y < 350.0f) {
                continue;
            }
            VECSubtract(&temporaryPosition2, &playerWork->worldPosition, &temporaryPosition);
            motionValue4 = VECMag(&temporaryPosition);
            if (motionValue4 < motionValue2) {
                stateValue = loopIndex;
                motionValue2 = motionValue4;
            }
        }
        GetPlayerPosition(stateValue, &temporaryPosition2);
        motionValue3 = 5.0f + temporaryPosition2.y;
        motionValue = 0.65f + 0.000100000005f * frandmod(1000);
#if VERSION_PAL
        motionValue6 = -3430.0f;
#else
        motionValue6 = -3430.0002f;
#endif
        motionValue5 = (motionValue3 - playerWork->modelAnchorPosition.y - (motionValue * motionValue * motionValue6 * 0.5f)) / motionValue;
        temporaryPosition.x = (REFRESH_FREQ * (temporaryPosition2.x - playerWork->modelAnchorPosition.x)) / motionValue;
        temporaryPosition.y = REFRESH_FREQ * motionValue5;
        temporaryPosition.z = (REFRESH_FREQ * (temporaryPosition2.z - playerWork->modelAnchorPosition.z)) / motionValue;
    }
    else {
        if (playerWork->isStunned != 0) {
            motionValue2 = -450.0f + (0.05f * frandmod(1000));
            motionValue3 = -100.0f;
            motionValue = 0.75f + (0.000100000005f * frandmod(1000));
            temporaryPosition2.x = playerWork->modelAnchorPosition.x + (0.05f * (frandmod(2000) - 1000));
            temporaryPosition2.y = motionValue3;
            temporaryPosition2.z = motionValue2;
        }
        else {
            motionValue2 = -700.0f;
            motionValue3 = 420.0f;
            if (playerWork->verticalVelocity < -600.0f) {
                motionValue3 = 280.0f;
            }
            motionValue = 0.65f + (0.000100000005f * frandmod(1000));
            stateValue2 = REFRESH_RATE * motionValue;
            stateValue = -1;
            motionValue2 = 10000.0f;

            for (loopIndex = 0; loopIndex < 18; loopIndex++) {
                GetPlayerFuturePosition(loopIndex, stateValue2, &temporaryPosition2);
                if (fabs(temporaryPosition2.y - motionValue3) > 20.0) {
                    continue;
                }
                motionValue4 = fabs(temporaryPosition2.x - playerWork->previousTargetPosition.x);
                if (motionValue4 < motionValue2) {
                    stateValue = loopIndex;
                    motionValue2 = motionValue4;
                }
            }
            motionValue7 = 75.0f;
            if (GWPlayerCfg[object->work[0]].iscom) {
                motionValue7 *= 0.25f;
            }
            if (stateValue >= 0 && motionValue2 <= motionValue7) {
                GetPlayerFuturePosition(stateValue, stateValue2, &temporaryPosition2);
            }
            else {
                temporaryPosition2.x = playerWork->previousTargetPosition.x;
                temporaryPosition2.y = motionValue3;
                temporaryPosition2.z = -700.0f;
            }
        }
#if VERSION_PAL
        motionValue6 = -3430.0f;
#else
        motionValue6 = -3430.0002f;
#endif
        motionValue5 = ((temporaryPosition2.y - playerWork->modelAnchorPosition.y) - motionValue * motionValue * motionValue6 * 0.5f) / motionValue;
        temporaryPosition.x = (temporaryPosition2.x - playerWork->modelAnchorPosition.x) * REFRESH_FREQ / motionValue;
        temporaryPosition.y = REFRESH_FREQ * motionValue5;
        temporaryPosition.z = (temporaryPosition2.z - playerWork->modelAnchorPosition.z) * REFRESH_FREQ / motionValue;
    }
    SetPlayerSphereState(playerWork->modelAnchorPosition, temporaryPosition, object->work[0]);
    playerWork->isGrounded = 0;
    playerWork->specialInputReady = 0;
    if (playerWork->isJumping) {
        SetPlayerMotion(object, 5);
    }
    Hu3DModelHookReset(object->mdlId[0]);
    Hu3DModelPosSet(object->mdlId[1], playerWork->modelAnchorPosition.x, playerWork->modelAnchorPosition.y, playerWork->modelAnchorPosition.z);
}

void UpdatePlayerController(OMOBJ *object)
{
    float motionValue;
    float motionValue2;
    float motionValue3;
    float motionValue4;
    float motionValue5;

    PlayerCharacterWork *playerWork;

    playerWork = object->data;
    if (!playerWork->isGrounded && (!playerWork->resetModelPose || playerWork->movementActive)) {
#if VERSION_PAL
        motionValue3 = -3430.0f;
#else
        motionValue3 = -3430.0002f;
#endif
        motionValue2 = 70.0f + object->trans.y;
        motionValue = 0.3f;
        if (playerWork->resetModelPose && playerWork->movementActive) {
            playerWork->resetModelPose = 0;
            playerWork->movementActive = 0;
            Hu3DModelAttrReset(object->mdlId[1], HU3D_ATTR_DISPOFF);
            playerWork->stateTimer = 0;
            playerWork->modelAnchorPosition.x = object->trans.x;
            playerWork->modelAnchorPosition.y = 100.0f + object->trans.y;
            playerWork->modelAnchorPosition.z = 500.0f + object->trans.z;
            motionValue4 = ((motionValue2 - playerWork->modelAnchorPosition.y) - (motionValue * motionValue * motionValue3 * 0.5f)) / motionValue;
            motionValue5 = 70.0f + object->trans.z;
            playerWork->movementVelocity.x = 0.0f;
            playerWork->movementVelocity.y = REFRESH_FREQ * motionValue4;
            playerWork->movementVelocity.z = (REFRESH_FREQ * (motionValue5 - playerWork->modelAnchorPosition.z)) / motionValue;
            playerWork->targetFacingAngle = 0.0f;
            playerWork->facingAngleStep = 0.0f;
            playerWork->facingAngleAcceleration = 0.04f;
        }
        VECAdd(&playerWork->modelAnchorPosition, &playerWork->movementVelocity, &playerWork->modelAnchorPosition);
        playerWork->movementVelocity.y = playerWork->movementVelocity.y + ((REFRESH_FREQ * REFRESH_FREQ) * motionValue3);
        if ((playerWork->movementVelocity.y < 0.0f) && (playerWork->modelAnchorPosition.y <= motionValue2)) {
            playerWork->isGrounded = 1;
            playerWork->resetModelPose = 1;
            playerWork->movementActive = 0;
            Hu3DModelHookSet(object->mdlId[0], playerHookDummyNames[playerWork->hookIndex], object->mdlId[1]);
            CharMotionSpeedSet(playerWork->characterModelId, 2.0f);
            playerWork->targetFacingAngle = 180.0f;
            playerWork->facingAngleStep = 0.0f;
            playerWork->facingAngleAcceleration = 0.005f;
            HuAudFXPlay(1376);
        }
        if (playerWork->isGrounded != 0) {
            Hu3DModelPosSet(object->mdlId[1], 0.0f, 0.0f, 0.0f);
        }
        else {
            Hu3DModelPosSet(object->mdlId[1], playerWork->modelAnchorPosition.x, playerWork->modelAnchorPosition.y, playerWork->modelAnchorPosition.z);
        }
        if (playerWork->stateTimer >= (REFRESH_RATE_F * (motionValue - 0.17f))) {
            SetPlayerMotion(object, 1);
        }
    }
}

s32 UpdatePlayerAi(OMOBJ *object)
{
    PlayerCharacterWork *playerWork;
    s32 stateValue;

    playerWork = object->data;
    stateValue = 0;
    if (playerWork->isStunned) {
        return stateValue;
    }
    if (!playerWork->isGrounded) {
        Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    }
    if (playerWork->isGrounded && !playerWork->isJumping && playerWork->alternateActionReady) {
        playerWork->isJumping = 1;
        playerWork->alternateActionReady = 0;
        SetPlayerMotion(object, 4);
        playerWork->facingAngleAcceleration = 0.05f;
        playerWork->verticalVelocity = 1300.0f;
    }
    if (playerWork->isJumping) {
        playerWork->worldPosition.y = playerWork->worldPosition.y + (REFRESH_FREQ * playerWork->verticalVelocity);
#if VERSION_PAL
        playerWork->verticalVelocity += -79.99999821186066;
#else
        playerWork->verticalVelocity += -66.66667014360428;
#endif
        if (playerWork->worldPosition.y <= 0.0f) {
            playerWork->isJumping = 0;
            if (playerWork->isGrounded) {
                SetPlayerMotion(object, 7);
            }
            else {
                stateValue = 1;
                SetPlayerMotion(object, 6);
            }
            playerWork->worldPosition.y = 0.0f;
        }
        playerWork->specialInputReady = 0;
        if (playerWork->worldPosition.y >= 30.000002f && playerWork->isGrounded) {
            playerWork->specialInputReady = 1;
            if (playerWork->actionRequested) {
                UpdatePlayerEffects(object);
                playerWork->specialInputReady = 0;
                playerWork->actionRequested = 0;
            }
        }
    }
    else if (playerWork->isGrounded) {
        SetPlayerAnimation(object, 2);
    }
    return stateValue;
}

s32 UpdatePlayerAnimation(OMOBJ *object)
{
    PlayerCharacterWork *playerWork;
    s32 stateValue;

    object = object;
    playerWork = object->data;
    stateValue = 0;
    if (!playerWork->isGrounded) {
        Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    }
    if (!playerWork->isStunned && playerWork->isGrounded && !playerWork->isJumping && playerWork->actionRequested) {
        SetPlayerMotion(object, 3);
        playerWork->actionRequested = 0;
        playerWork->isStunned = 1;
        playerWork->stateTimer = 0;
    }
    else if (playerWork->isStunned) {
        if (playerWork->isGrounded) {
            if (playerWork->stateTimer >= REFRESH_RATE_F * 0.3f) {
                UpdatePlayerEffects(object);
            }
        }
        else if (playerWork->stateTimer >= REFRESH_RATE_F / 2 && UpdatePlayerInput(object)) {
            playerWork->actionRequested = 0;
            playerWork->alternateActionReady = 0;
            playerWork->isStunned = 0;
            stateValue = 1;
        }
    }
    return stateValue;
}

void SetPlayerMotion(OMOBJ *object, u32 motionId)
{
    float motionValue;

    PlayerCharacterWork *playerWork;

    playerWork = object->data;
    if ((playerWork->currentMotionId != motionId) && (motionId < 10)) {
        motionValue = playerMotionTimings[motionId].motionDuration;
        if (playerWork->currentMotionId < 0) {
            motionValue = 0.0f;
        }
        playerWork->currentMotionId = motionId;
        CharMotionShiftSet(playerWork->characterModelId, object->mtnId[motionId], playerMotionTimings[motionId].blendInFrames, motionValue, playerMotionTimings[motionId].motionAttributes);
        CharMotionSpeedSet(playerWork->characterModelId, 1.0f);
    }
}

s32 UpdatePlayerInput(OMOBJ *object)
{
    PlayerCharacterWork *playerWork;
    s32 stateValue;

    playerWork = object->data;
    stateValue = 0;
    if (CharMotionEndCheck(playerWork->characterModelId) && (CharMotionShiftIDGet(playerWork->characterModelId) < 0)) {
        stateValue = 1;
    }
    return stateValue;
}

s32 SetPlayerAnimation(OMOBJ *object, u32 motionId)
{
    PlayerCharacterWork *playerWork;
    s32 stateValue;
    s32 stateValue2;

    stateValue = 0;
    playerWork = object->data;
    stateValue2 = 0;
    if (CharMotionEndCheck(playerWork->characterModelId) && (CharMotionShiftIDGet(playerWork->characterModelId) < 0)) {
        stateValue2 = 1;
    }
    if (stateValue2) {
        SetPlayerMotion(object, motionId);
        stateValue = 1;
    }
    return stateValue;
}

void CreatePlayerManager(OMOBJ *object)
{

    PlayerAnimationWork *animationWork;
    PlayerRenderWork *renderWork;
    s32 loopIndex;

    float temporaryValue[4][2] = { { 36.0f, 64.0f }, { 476.0f, 64.0f }, { 36.0f, 416.0f }, { 476.0f, 416.0f } };
    object->stat |= 256;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerRenderWork), HU_MEMNUM_OVL);
    renderWork = object->data;
    memset(renderWork, 0, sizeof(PlayerRenderWork));
    renderWork->framebufferSize = GXGetTexBufferSize(640, 480, GX_TF_RGB5A3, GX_FALSE, 0);
    renderWork->framebufferBuffer = HuMemDirectMallocNum(HEAP_HEAP, renderWork->framebufferSize, HU_MEMNUM_OVL);
    DCFlushRange(renderWork->framebufferBuffer, renderWork->framebufferSize);
    BuildPlayerDisplayList(&renderWork->displayListWork);
    animationWork = renderWork->animationWork;

    for (loopIndex = 0; loopIndex < 4; loopIndex++, animationWork++) {
        animationWork->scoreDisplayX = temporaryValue[playerOrderByRandomRank[loopIndex]][0];
        animationWork->scoreDisplayY = temporaryValue[playerOrderByRandomRank[loopIndex]][1];
        animationWork->digitScale[0] = animationWork->digitScale[1] = 1.0f;
        animationWork->digitSpriteIds[0] = animationWork->digitSpriteIds[1] = animationWork->characterSpriteId = animationWork->scoreIconSpriteId = -1;
        animationWork->digitSpriteIds[0] = espEntry(6357043, 2, 0);
        animationWork->digitSpriteIds[1] = espEntry(6357043, 2, 0);
        animationWork->characterSpriteId = espEntry(playerCharacterDataIds[GWPlayerCfg[loopIndex].character], 2, 0);
        animationWork->scoreIconSpriteId = espEntry(6357014, 3, 0);
        espPosSet(animationWork->scoreIconSpriteId, 36.0f + animationWork->scoreDisplayX, animationWork->scoreDisplayY);
        animationWork->scoreLabelSpriteId = espEntry(6357015, 4, 0);
        espPosSet(animationWork->scoreLabelSpriteId, 36.0f + animationWork->scoreDisplayX, animationWork->scoreDisplayY);
        espColorSet(animationWork->scoreLabelSpriteId, 0, 0, 0);
        espTPLvlSet(animationWork->scoreLabelSpriteId, 0.5);
        espDispOff(animationWork->digitSpriteIds[0]);
        espDispOff(animationWork->digitSpriteIds[1]);
        espDispOff(animationWork->characterSpriteId);
        espDispOff(animationWork->scoreIconSpriteId);
        espDispOff(animationWork->scoreLabelSpriteId);
        animationWork->scorePopupActive = 0;
    }
    SetGoalCallback(SetPlayerScore);
    object->mdlId[0] = Hu3DHookFuncCreate(UpdatePlayerModelMaterial);
    Hu3DModelLayerSet(object->mdlId[0], 3);
    HuDataDirClose(DATADIR_MGCONST);
    object->objFunc = UpdatePlayerManager;
}

void UpdatePlayerManager(OMOBJ *object)
{
    float motionValue;

    PlayerAnimationWork *animationWork;
    PlayerModelWork *modelWork;
    s32 loopIndex;
    s32 loopIndex2;
    PlayerRenderWork *renderWork;
    u32 stateValue;
    s32 stateValue2;

    renderWork = object->data;
    animationWork = renderWork->animationWork;

    for (loopIndex = 0; loopIndex < 4; loopIndex++, animationWork++) {
        switch (animationWork->popupState) {
            case 0:
                if (getMinigameSubstate() == 2) {
                    animationWork->popupState = 1;
                }
                break;
            case 1:
                animationWork->popupTimer = 0;
                animationWork->onesDigit = animationWork->displayedScore % 10;
                animationWork->tensDigit = (animationWork->displayedScore / 10) % 10;
                espDispOn(animationWork->digitSpriteIds[0]);
                espDispOn(animationWork->digitSpriteIds[1]);
                espDispOn(animationWork->characterSpriteId);
                espDispOn(animationWork->scoreIconSpriteId);
                espDispOn(animationWork->scoreLabelSpriteId);
                espBankSet(animationWork->digitSpriteIds[0], animationWork->tensDigit);
                espBankSet(animationWork->digitSpriteIds[1], animationWork->onesDigit);
                espBankSet(animationWork->characterSpriteId, animationWork->hundredsDigit);
                if (animationWork->scorePopupActive) {
                    animationWork->digitScale[0] = animationWork->digitScale[1] = 0.5f;
                    animationWork->popupState++;
                }
                else {
                    animationWork->scorePopupActive = 1;
                    animationWork->digitScale[0] = animationWork->digitScale[1] = 1.0f;
                    animationWork->popupState += 3;
                    break;
                }
            case 2:
                motionValue = animationWork->digitScale[0];
                motionValue += REFRESH_FREQ * 6;
                if (motionValue >= 1.5f) {
                    animationWork->popupState++;
                }
                animationWork->digitScale[0] = animationWork->digitScale[1] = motionValue;
                break;
            case 3:
                motionValue = animationWork->digitScale[0];
                motionValue -= REFRESH_FREQ * 6;
                if (motionValue <= 1.0f) {
                    motionValue = 1.0f;
                    animationWork->popupState = animationWork->popupState + 1;
                }
                animationWork->digitScale[0] = animationWork->digitScale[1] = motionValue;
                break;
        }
        animationWork->popupTimer++;

        for (loopIndex2 = 0; loopIndex2 < 2; loopIndex2++) {
            espPosSet(animationWork->digitSpriteIds[loopIndex2], 34.0f + animationWork->scoreDisplayX + (21.0f * loopIndex2), 2.0f + animationWork->scoreDisplayY);
            espScaleSet(animationWork->digitSpriteIds[loopIndex2], animationWork->digitScale[loopIndex2], animationWork->digitScale[loopIndex2]);
        }
        espPosSet(animationWork->characterSpriteId, animationWork->scoreDisplayX, animationWork->scoreDisplayY - 2.0f);
    }
    modelWork = renderWork->displayListWork.modelWork;

    for (loopIndex = 0; loopIndex < 16; loopIndex++, modelWork++) {
        if (modelWork->isActive) {
            modelWork->isActive++;
            if (modelWork->isActive < REFRESH_RATE_F / 5) {
                modelWork->modelScale += 3.5f * REFRESH_FREQ;
            }
            else {
                modelWork->modelScale += 1.5f * REFRESH_FREQ;
                modelWork->modelOpacity -= 3.0f * REFRESH_FREQ;
            }
            if (modelWork->modelOpacity < 0.0f) {
                modelWork->isActive = 0;
            }
        }
    }
    if (getMinigameSubstate() == 3) {
        stateValue2 = 0;
        animationWork = renderWork->animationWork;
        stateValue = 0;

        for (loopIndex = 0; loopIndex < 4; loopIndex++, animationWork++) {
            if (animationWork->scoreDelta > stateValue) {
                stateValue = animationWork->scoreDelta;
            }
        }
        if (stateValue) {
            animationWork = renderWork->animationWork;

            for (loopIndex = 0; loopIndex < 4; loopIndex++, animationWork++) {
                if (animationWork->scoreDelta >= stateValue) {
                    stateValue2 |= 1 << loopIndex;
                }
            }
        }
        SetMinigameFlags(stateValue2);
    }
}

void SetPlayerScore(u16 playerIndex, u16 playerIndex2, u16 playerIndex3)
{
    PlayerModelWork *modelWork;
    PlayerAnimationWork *animationWork;
    s32 loopIndex;
    PlayerRenderWork *renderWork;

    renderWork = playerManagerObject->data;
    if (getMinigameSubstate() == 2 || getMinigameSubstate() == 3) {
        animationWork = renderWork->animationWork;
        animationWork[playerIndex].scoreDelta += playerIndex2;
        animationWork[playerIndex].displayedScore += playerIndex2;
        animationWork[playerIndex].popupState = 1;

        modelWork = renderWork->displayListWork.modelWork;

        for (loopIndex = 0; loopIndex < 16; loopIndex++, modelWork++) {
            if (!modelWork->isActive) {
                break;
            }
        }
        if (loopIndex < 16) {
            modelWork->isActive = 1;
            modelWork->characterId = GWPlayerCfg[playerIndex].character;
            modelWork->modelOpacity = 1.0f;
            modelWork->modelScale = 0.25f;
            modelWork->modelRotation = frandmod(360);
            GetPlayerPosition(playerIndex3, &modelWork->modelPosition);
            modelWork->modelPosition.y -= 30.000002f;
        }
    }
}

void BuildPlayerDisplayList(PlayerDisplayListWork *displayListWork)
{
    float temporaryValue[32][2];
    float temporaryValue2[32];

    float motionValue;
    float motionValue2;
    float motionValue3;
    float motionValue4;

    s32 loopIndex;
    PlayerMotionParameterWork *motionWork;
    PlayerMotionWork *motionWork2;

    motionValue = 0.0f;
    for (loopIndex = 0; loopIndex < 32; loopIndex++) {
        motionValue4 = 1.0f + (0.001f * frandmod(1000));
        temporaryValue2[loopIndex] = motionValue4;
        motionValue += motionValue4;
        temporaryValue[loopIndex][1] = 80.0f * motionValue4;
        temporaryValue[loopIndex][0] = 10.0f + (0.02f * frandmod(1000));
    }
    motionValue = 360.0f / motionValue;
    for (loopIndex = 0; loopIndex < 32; loopIndex++) {
        temporaryValue2[loopIndex] *= motionValue;
    }
    motionValue = frandmod(90);
    motionWork2 = displayListWork->particleVertexPositions;
    motionWork = displayListWork->particleVertexTextureCoordinates;
    for (loopIndex = 0; loopIndex < 32; motionWork2++, motionWork++, loopIndex++) {
        motionValue3 = -cosd(motionValue);
        motionValue2 = sind(motionValue);
        motionWork2->vertex5X = motionValue3 * temporaryValue[loopIndex][0];
        motionWork2->vertex5Z = motionValue2 * temporaryValue[loopIndex][0];
        motionWork2->vertex1X = motionValue3 * (temporaryValue[loopIndex][0] + temporaryValue[loopIndex][1]);
        motionWork2->vertex1Z = motionValue2 * (temporaryValue[loopIndex][0] + temporaryValue[loopIndex][1]);
        motionWork2->vertex0X = motionValue3 * (temporaryValue[loopIndex][0] + (0.85f * temporaryValue[loopIndex][1]));
        motionWork2->vertex0Z = motionValue2 * (temporaryValue[loopIndex][0] + (0.85f * temporaryValue[loopIndex][1]));
        motionWork->radialOffsetX = -0.3f * -motionValue3;
        motionWork->radialOffsetZ = -0.3f * motionValue2;
        motionValue3 = -cosd(motionValue - (0.4f * temporaryValue2[loopIndex]));
        motionValue2 = sind(motionValue - (0.4f * (&temporaryValue2[0])[loopIndex]));
        motionWork2->vertex7X = motionValue3 * (temporaryValue[loopIndex][0] + (0.7f * temporaryValue[loopIndex][1]));
        motionWork2->vertex7Z = motionValue2 * (temporaryValue[loopIndex][0] + (0.7f * temporaryValue[loopIndex][1]));
        motionValue3 = -cosd(motionValue + (0.4f * temporaryValue2[loopIndex]));
        motionValue2 = sind(motionValue + (0.4f * temporaryValue2[loopIndex]));
        motionWork2->vertex3X = motionValue3 * (temporaryValue[loopIndex][0] + (0.7f * temporaryValue[loopIndex][1]));
        motionWork2->vertex3Z = motionValue2 * (temporaryValue[loopIndex][0] + (0.7f * temporaryValue[loopIndex][1]));
        motionWork->reservedParameter08 = motionWork->reservedParameter0c = motionWork->reservedParameter10 = motionWork->reservedParameter14 = 0.0f;
        motionWork->reservedParameter18 = motionWork->reservedParameter1c = motionWork->reservedParameter20 = motionWork->reservedParameter24 = 0.0f;
        motionValue += temporaryValue2[loopIndex];
    }

    for (loopIndex = 0; loopIndex < 5; loopIndex++) {
        displayListWork->particleVertexColors[loopIndex].r = displayListWork->particleVertexColors[loopIndex].g = displayListWork->particleVertexColors[loopIndex].b = 255;
        displayListWork->particleVertexColors[loopIndex].a = 255;
    }
    displayListWork->displayListSize = 0x6C0;
    displayListWork->displayListBuffer = HuMemDirectMallocNum(HEAP_HEAP, displayListWork->displayListSize, HU_MEMNUM_OVL);
    DCFlushRange(displayListWork->displayListBuffer, displayListWork->displayListSize);
    GXBeginDisplayList(displayListWork->displayListBuffer, displayListWork->displayListSize);

    for (loopIndex = 0; loopIndex < 32; loopIndex++) {
        GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, 6);

        GXPosition1x16(loopIndex * 5);
        GXColor1x16(0);
        GXTexCoord1x16(loopIndex * 5);

        GXPosition1x16(loopIndex * 5 + 1);
        GXColor1x16(1);
        GXTexCoord1x16(loopIndex * 5 + 1);

        GXPosition1x16(loopIndex * 5 + 2);
        GXColor1x16(2);
        GXTexCoord1x16(loopIndex * 5 + 2);

        GXPosition1x16(loopIndex * 5 + 3);
        GXColor1x16(3);
        GXTexCoord1x16(loopIndex * 5 + 3);

        GXPosition1x16(loopIndex * 5 + 4);
        GXColor1x16(4);
        GXTexCoord1x16(loopIndex * 5 + 4);

        GXPosition1x16(loopIndex * 5 + 1);
        GXColor1x16(1);
        GXTexCoord1x16(loopIndex * 5 + 1);
    }
    displayListWork->displayListLength = GXEndDisplayList();
    DCStoreRange(displayListWork, sizeof(PlayerDisplayListWork));
}

void UpdatePlayerModelMaterial(HU3DMODEL *model, Mtx transform)
{
    Mtx temporaryTransform;
    Mtx temporaryTransform2;
    Mtx temporaryTransform3;
    Mtx temporaryTransform4;
    Mtx temporaryTransform5;
    Mtx temporaryTransform6;
    GXTexObj temporaryValue;

    PlayerModelWork *modelWork;
    PlayerRenderWork *renderWork;
    s32 loopIndex;

    renderWork = playerManagerObject->data;
    {
        GXColor temporaryValue = { 204, 204, 204, 204 };
        GXSetTexCopySrc(0, 0, 640, 480);
        GXSetTexCopyDst(640, 480, GX_TF_RGB5A3, GX_FALSE);
        GXCopyTex(renderWork->framebufferBuffer, GX_FALSE);
        GXPixModeSync();
        DCStoreRange(&temporaryValue, 4);

        DCStoreRange(&playerDisplayColors, 32);
        MTXTrans(temporaryTransform2, -400.0f, 400.0f, -700.0f);
        MTXConcat(Hu3DCameraMtx, temporaryTransform2, temporaryTransform5);
        GXLoadPosMtxImm(temporaryTransform5, 0);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_NONE);
        GXInitTexObj(&temporaryValue, renderWork->framebufferBuffer, 640, 480, 5, GX_CLAMP, GX_CLAMP, GX_FALSE);
        GXInitTexObjLOD(&temporaryValue, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
        GXLoadTexObj(&temporaryValue, GX_TEXMAP0);
        HuSprTexLoad(playerSpriteAnimation, 0, 1, GX_REPEAT, GX_REPEAT, GX_LINEAR);
        GXSetNumTexGens(2);
        C_MTXLightPerspective(temporaryTransform4, 41.5f, 1.2f, 0.5555555f, -0.5f, 0.5555555f, 0.5f);
        MTXConcat(temporaryTransform4, Hu3DCameraMtx, temporaryTransform4);
        MTXInverse(Hu3DCameraMtx, temporaryTransform6);
        MTXConcat(temporaryTransform6, temporaryTransform5, temporaryTransform);
        MTXConcat(temporaryTransform4, temporaryTransform, temporaryTransform2);
        GXLoadTexMtxImm(temporaryTransform2, 30, GX_MTX3x4);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, 30, GX_FALSE, 125);
        GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEXCOORD0, 33, GX_FALSE, 125);
        GXSetNumIndStages(1);
        GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD1, GX_TEXMAP1);
        GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
        GXSetTevIndWarp(GX_TEVSTAGE0, GX_INDTEXSTAGE0, GX_TRUE, GX_FALSE, GX_ITM_0);
        MTXScale(temporaryTransform2, -0.5f, -0.5f, 0.5f);
        GXSetIndTexMtx(GX_ITM_0, (float(*)[3])temporaryTransform2, -1);
        GXSetNumTevStages(1);
        GXSetTevColor(GX_TEVREG1, temporaryValue);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C1, GX_CC_C0);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_RASA, GX_CA_A0, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
        GXSetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
        GXSetVtxDesc(GX_VA_CLR0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
        GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
        GXSetArray(GX_VA_POS, renderWork->displayListWork.particleVertexPositions, 12);
        GXSetArray(GX_VA_CLR0, renderWork->displayListWork.particleVertexColors, 4);
        GXSetArray(GX_VA_TEX0, renderWork->displayListWork.particleVertexTextureCoordinates, 8);
        modelWork = renderWork->displayListWork.modelWork;

        for (loopIndex = 0; loopIndex < 16; loopIndex++, modelWork++) {
            if (modelWork->isActive) {
                playerDisplayColors[modelWork->characterId].a = 255.0f * modelWork->modelOpacity;
                GXSetTevColor(GX_TEVREG0, playerDisplayColors[modelWork->characterId]);
                MTXTrans(temporaryTransform2, modelWork->modelPosition.x, modelWork->modelPosition.y, modelWork->modelPosition.z);
                MTXRotRad(temporaryTransform, 90, 0.017453292f * modelWork->modelRotation);
                MTXConcat(temporaryTransform2, temporaryTransform, temporaryTransform2);
                MTXScale(temporaryTransform3, modelWork->modelScale, modelWork->modelScale, modelWork->modelScale);
                MTXConcat(temporaryTransform2, temporaryTransform3, temporaryTransform2);
                MTXConcat(Hu3DCameraMtx, temporaryTransform2, temporaryTransform5);
                GXLoadPosMtxImm(temporaryTransform5, 0);
                MTXConcat(temporaryTransform6, temporaryTransform5, temporaryTransform);
                MTXConcat(temporaryTransform4, temporaryTransform, temporaryTransform2);
                GXLoadTexMtxImm(temporaryTransform2, 30, GX_MTX3x4);
                MTXRotRad(temporaryTransform, 90, 0.017453292f * -modelWork->modelRotation);
                GXLoadTexMtxImm(temporaryTransform, 33, GX_MTX2x4);
                GXCallDisplayList(renderWork->displayListWork.displayListBuffer, renderWork->displayListWork.displayListLength);
            }
        }
        GXSetNumIndStages(0);
        GXSetTevDirect(GX_TEVSTAGE0);
    }
}

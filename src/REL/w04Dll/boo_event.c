#include "REL/w04Dll.h"
#include "game/audio.h"
#include "game/board/audio.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/ui.h"
#include "game/board/window.h"
#include "game/data.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"

#include "dolphin.h"
#include "ext_math.h"

#ifndef __MWERKS__
#include <stdlib.h>
#endif

typedef struct {
    /* 0x00 */ char reserved[0x50];
    /* 0x50 */ u32 particleEffectState; // pointer?
    /* 0x54 */ s32 isActive;
} BooParticleEffectState; // Size unknown

typedef struct {
    /* 0x00 */ s16 eventState;
    /* 0x02 */ s16 isActive;
    /* 0x04 */ s16 modelId;
    /* 0x06 */ s16 eventIndex;
    /* 0x08 */ s16 playerIndex;
    /* 0x0A */ s16 eventParameter;
    /* 0x0C */ s16 targetSpace;
    /* 0x0E */ s16 previousSpace;
    /* 0x10 */ float rotationSpeed;
    /* 0x14 */ float motionAngle;
    /* 0x18 */ float modelAlpha;
    /* 0x1C */ float verticalOffset;
    /* 0x20 */ Vec targetPosition;
    /* 0x2C */ HUPROCESS *eventProcess;
} BooEventProcessData; // Size 0x30

static void RunW04BooEventSequence(void);
static void RunW04BooMovementEvent(void);
static void CleanupW04BooEventProcesses(void);
static void CreateW04BooEventProcesses(void);
static void KillW04BooEventProcess(s16 processIndex);
static void SetW04BooEventState(s32 eventState);
static s32 CheckW04BooEventState(u32 expectedEventState);
static BooEventProcessData *GetW04BooEventProcessData(s16 processIndex);
static void RunW04BooMotionProcess(void);
static void RunW04BooPlayerEventProcess(void);
static void ReturnW04BooPlayerToSpace(void);
static s32 ChooseW04BooDestinationSpace(void);
static void RunW04BooPlayerInteraction(void);
static void ResetW04BooEventCamera(void);
static s32 FindW04BooTargetPlayer(s16 targetSpace);
static void StealW04BooPlayerItem(void);
static void RunW04BooLaunchProcess(void);
static void AlignW04BooPlayers(s16 targetSpace, Vec *playerPositions);
static void FinishW04BooEventProcess(void);
static void CreateW04BooParticles(void);
static void CleanupW04BooParticles(void);
static s16 FindW04BooNearestPlayer(Vec *targetPosition);
static void SetW04BooPlayerState(s16 playerIndex);
static void UpdateW04BooParticleObject(OMOBJ *particleObject);
static void UpdateW04BooParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix);

static s16 booModelId;
static s16 booDestinationMarkerModelIds[3];
static HUPROCESS *booEventProcesses[4];
static HUPROCESS *booPlayerProcesses[4];
static OMOBJ *booParticleObjects[4];
static s16 booTargetPlayerCount;
static s16 booTargetPlayerIds[4];
static s16 booPlayerEventStates[4];
static s16 booDestinationSpace;
static Vec booPlayerPositions[4];
static s16 activeBooPlayerProcessCount;
static s32 booCurrentSpace;
static s32 booPreviousSpace;
static s16 booRouteChoiceFlag;
static s32 booSoundEffectId;
static s16 booChainEventFlag;
static s16 booPlayerMotionIds[4][3];
static s16 booSelectedTargetPlayer;
static ANIMDATA *booAnimationData;
static HUPROCESS *booEventProcess;

static s32 characterMotionDirectories[8] = { DATADIR_MARIOMOT, DATADIR_LUIGIMOT, DATADIR_PEACHMOT, DATADIR_YOSHIMOT, DATADIR_WARIOMOT, DATADIR_DONKEYMOT,
    DATADIR_DAISYMOT, DATADIR_WALUIGIMOT };

static s32 booPlayerMotionId = 28;

static s32 booPlayerMotionIdsA[8]
    = { DATA_MAKE_NUM(DATADIR_W04, 35), DATA_MAKE_NUM(DATADIR_W04, 36), DATA_MAKE_NUM(DATADIR_W04, 37), DATA_MAKE_NUM(DATADIR_W04, 38),
          DATA_MAKE_NUM(DATADIR_W04, 39), DATA_MAKE_NUM(DATADIR_W04, 40), DATA_MAKE_NUM(DATADIR_W04, 41), DATA_MAKE_NUM(DATADIR_W04, 42) };

static s32 booPlayerMotionIdsB[8]
    = { DATA_MAKE_NUM(DATADIR_W04, 43), DATA_MAKE_NUM(DATADIR_W04, 44), DATA_MAKE_NUM(DATADIR_W04, 45), DATA_MAKE_NUM(DATADIR_W04, 46),
          DATA_MAKE_NUM(DATADIR_W04, 47), DATA_MAKE_NUM(DATADIR_W04, 48), DATA_MAKE_NUM(DATADIR_W04, 49), DATA_MAKE_NUM(DATADIR_W04, 50) };

static char *booModelHookNames[] = { "kan1", "kan2", "kan3", "kan4" };

static float booRotationSpeeds[] = { -150.00001f, -170.00001f, -30.0f, -220.0f };

static void RunW04BooEventSequence(void)
{
    Vec booModelPosition;
    Vec targetPosition;
    Vec movementStep;
    float modelAlpha;
    s32 currentPlayer;
    s32 booMotionId;
    BoardSpace *boardSpace;
    BoardMapObject *mapObject;
    s32 i;
    s32 j;

    currentPlayer = GWSystem.player_curr;
    booRouteChoiceFlag = 0;
    activeBooPlayerProcessCount = 0;
    booMotionId = characterMotionDirectories[GWPlayer[currentPlayer].character];
    booPlayerMotionIds[currentPlayer][0] = BoardPlayerMotionCreate(currentPlayer, booMotionId | booPlayerMotionId);
    booPlayerMotionIds[currentPlayer][1] = BoardPlayerMotionCreate(currentPlayer, booPlayerMotionIdsA[GWPlayer[currentPlayer].character]);
    for (i = 0; i < 4; i++) {
        if (i != currentPlayer) {
            booPlayerMotionIds[i][0] = BoardPlayerMotionCreate(i, booMotionId | booPlayerMotionId);
            booPlayerMotionIds[i][1] = BoardPlayerMotionCreate(i, booPlayerMotionIdsA[GWPlayer[i].character]);
            booPlayerMotionIds[i][2] = BoardPlayerMotionCreate(i, booPlayerMotionIdsB[GWPlayer[i].character]);
        }
    }
    for (i = 0; i < 3; i++) {
        booDestinationMarkerModelIds[i] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_BOARD, 0x00), NULL, 1);
        BoardModelLayerSet(booDestinationMarkerModelIds[i], 2);
        BoardModelVisibilitySet(booDestinationMarkerModelIds[i], 0);
    }
    mapObject = &w04BoardMapObjects[8];
    booModelId = w04BoardModelIds[8];
    BoardModelPosSetV(booModelId, &mapObject->pos);
    BoardModelRotSetV(booModelId, &mapObject->rot);
    BoardModelVisibilitySet(booModelId, 1);
    BoardModelAlphaSet(booModelId, 0);
    modelAlpha = 0.0f;
    for (i = 0; i < 4; i++) {
        booPlayerEventStates[i] = 0;
        booPlayerProcesses[i] = NULL;
    }
    CreateW04BooParticles();
    CreateW04BooEventProcesses();
    BoardRollDispSet(0);
    BoardPlayerMotBlendSet(currentPlayer, 180, 15);
    BoardCameraViewSet(2);
    while (!BoardPlayerMotBlendCheck(currentPlayer)) {
        HuPrcVSleep();
    }
    BoardCameraMotionWait();
    BoardPlayerMotionShiftSet(currentPlayer, booPlayerMotionIds[currentPlayer][0], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    PlayerFXPlay(currentPlayer, 0x128);
    BoardModelPosGet(w04BoardModelIds[3], &booModelPosition);
    BoardModelPosSetV(booModelId, &booModelPosition);
    BoardModelVisibilitySet(booModelId, 1);
    for (i = 0; i < 60; i++) {
        modelAlpha += 4.25f;
        BoardModelAlphaSet(booModelId, modelAlpha);
        HuPrcVSleep();
    }
    BoardModelAlphaSet(booModelId, 0xFF);
    HuAudFXPlay(0x4C);
    BoardPlayerIdleSet(currentPlayer);
    HuPrcSleep(8);
    BoardWinCreate(2, MAKE_MESSID(32, 1), 3);
    BoardWinWait();
    BoardWinKill();
    boardSpace = BoardSpaceGet(0, GWPlayer[currentPlayer].space_curr);
    booPreviousSpace = booCurrentSpace = boardSpace->link[0];
    BoardSpacePosGet(0, booCurrentSpace, &targetPosition);
    BoardModelPosGet(booModelId, &booModelPosition);
    while (W04ModelRotateYToAngle(booModelId, Atan2d(targetPosition.x - booModelPosition.x, targetPosition.z - booModelPosition.z), 2.0f) == 0) {
        HuPrcVSleep();
    }
    VECSubtract(&targetPosition, &booModelPosition, &movementStep);
    VECScale(&movementStep, &movementStep, 0.022222223f);
    for (i = 0; i < 45; i++) {
        VECAdd(&booModelPosition, &movementStep, &booModelPosition);
        BoardModelPosSetV(booModelId, &booModelPosition);
        HuPrcVSleep();
    }
    BoardAudSeqPause(0, 1, 1000);
    BoardModelPosSetV(booModelId, &targetPosition);
    BoardPlayerPosGet(currentPlayer, &targetPosition);
    while (W04ModelRotateYToAngle(booModelId, Atan2d(targetPosition.x - booModelPosition.x, targetPosition.z - booModelPosition.z), 3.0f) == 0) {
        HuPrcVSleep();
    }
    SetW04BooEventState(1);
    while (CheckW04BooEventState(0) == 0) {
        HuPrcVSleep();
    }
    GetW04BooEventProcessData(0)->eventState = 2;
    while (W04ModelRotateYToAngle(booModelId, 90.0f, 3.0f) == 0) {
        HuPrcVSleep();
    }
    while (CheckW04BooEventState(0) == 0) {
        HuPrcVSleep();
    }
    RunW04BooMovementEvent();
    CleanupW04BooEventProcesses();
    BoardAudSeqPause(0, 0, 1000);
    BoardRollDispSet(1);
    BoardCameraViewSet(1);
    BoardCameraMotionWait();
    BoardPlayerMotionKill(currentPlayer, booPlayerMotionIds[currentPlayer][0]);
    BoardPlayerMotionKill(currentPlayer, booPlayerMotionIds[currentPlayer][1]);
    for (i = 0; i < 4; i++) {
        if (i != currentPlayer) {
            for (j = 0; j < 3; j++) {
                BoardPlayerMotionKill(i, booPlayerMotionIds[i][j]);
            }
        }
    }
    for (i = 0; i < 3; i++) {
        BoardModelKill(booDestinationMarkerModelIds[i]);
    }
    CleanupW04BooParticles();
    BoardModelVisibilitySet(booModelId, 0);
    HuPrcEnd();
}

static void RunW04BooMovementEvent(void)
{
    Vec destinationPosition;
    Vec booPosition;
    Vec movementStep;
    float targetAngle;
    float modelAlpha;
    BoardSpace *boardSpace;
    s16 targetPlayer;
    s32 eventProcessIndex;
    s32 i;
    HUPROCESS *process;
    float *floatValue;

    process = HuPrcChildCreate(RunW04BooPlayerInteraction, 0x2001, 0x800, 0, boardMainProc);
    HuPrcDestructorSet2(process, ResetW04BooEventCamera);
    floatValue = (float *)&process->user_data;
    booTargetPlayerCount = FindW04BooTargetPlayer(booCurrentSpace);
    targetPlayer = 0;
    while (targetPlayer == 0) {
        eventProcessIndex = ChooseW04BooDestinationSpace();
        if (eventProcessIndex != -1) {
            boardSpace = BoardSpaceGet(0, eventProcessIndex);
            if (boardSpace->flag & 1) {
                booSelectedTargetPlayer = eventProcessIndex;
            }
            if (boardSpace->flag & 8) {
                AlignW04BooPlayers(booSelectedTargetPlayer, booPlayerPositions);
                targetPlayer = 1;
                modelAlpha = 255.0f;
                HuPrcKill(process);
            }
            BoardSpacePosGet(0, eventProcessIndex, &destinationPosition);
            BoardModelPosGet(booModelId, &booPosition);
            targetAngle = Atan2d(destinationPosition.x - booPosition.x, destinationPosition.z - booPosition.z);
            BoardModelRotYSet(booModelId, targetAngle);
            VECSubtract(&destinationPosition, &booPosition, &movementStep);
            VECScale(&movementStep, &movementStep, 0.022222223f);
            *floatValue = targetAngle - 180.0f;
            SetW04BooEventState(3);
            for (i = 0; i < 45; i++) {
                VECAdd(&movementStep, &booPosition, &booPosition);
                BoardModelPosSetV(booModelId, &booPosition);
                RunW04BooLaunchProcess();
                if (targetPlayer != 0) {
                    modelAlpha -= 5.6666665f;
                    BoardModelAlphaSet(booModelId, modelAlpha);
                }
                HuPrcVSleep();
            }
            BoardModelPosSetV(booModelId, &destinationPosition);
            booPreviousSpace = booCurrentSpace;
            booCurrentSpace = eventProcessIndex;
            if (targetPlayer != 0) {
                BoardModelVisibilitySet(booModelId, 0);
            }
        }
        HuPrcVSleep();
    }
}

static void CleanupW04BooEventProcesses(void)
{
    s32 i;

    while (CheckW04BooEventState(4) == 0 || activeBooPlayerProcessCount != 0) {
        SetW04BooEventState(3);
        HuPrcVSleep();
    }
    HuAudFXStop(booSoundEffectId);
    for (i = 0; i < 4; i++) {
        KillW04BooEventProcess(i);
    }
}

static void CreateW04BooEventProcesses(void)
{
    Vec hookPosition;
    BooEventProcessData *eventProcessData;
    s32 i;

    for (i = 0; i < 4; i++) {
        eventProcessData = HuMemDirectMallocNum(HEAP_HEAP, sizeof(BooEventProcessData), HU_MEMNUM_OVL);
        eventProcessData->modelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W04, 3), NULL, 0);
        Hu3DModelObjPosGet(BoardModelIDGet(w04BoardModelIds[3]), booModelHookNames[i], &hookPosition);
        BoardModelLayerSet(eventProcessData->modelId, 2);
        BoardModelPosSetV(eventProcessData->modelId, &hookPosition);
        eventProcessData->rotationSpeed = booRotationSpeeds[i];
        BoardModelRotYSet(eventProcessData->modelId, eventProcessData->rotationSpeed);
        eventProcessData->modelAlpha = 0.0f;
        BoardModelAlphaSet(eventProcessData->modelId, eventProcessData->modelAlpha);
        BoardModelVisibilitySet(eventProcessData->modelId, 0);
        eventProcessData->motionAngle = 0.003921569f * frand8() * 360.0f;
        eventProcessData->playerIndex = -1;
        eventProcessData->eventState = 0;
        eventProcessData->eventIndex = i;
        eventProcessData->verticalOffset = 0.0f;
        eventProcessData->isActive = 1;
        eventProcessData->targetSpace = 0;
        if (i == 0) {
            eventProcessData->eventParameter = booModelId;
        }
        else {
            eventProcessData->eventParameter = ((BooEventProcessData *)booEventProcesses[i - 1]->user_data)->modelId;
        }
        booEventProcesses[i] = HuPrcChildCreate(RunW04BooPlayerEventProcess, 0x2003, 0x1000, 0, boardMainProc);
        booEventProcesses[i]->user_data = eventProcessData;
        eventProcessData->eventProcess = HuPrcChildCreate(RunW04BooMotionProcess, 0x2002, 0x1000, 0, boardMainProc);
        eventProcessData->eventProcess->user_data = eventProcessData;
    }
}

static void KillW04BooEventProcess(s16 processIndex)
{
    BooEventProcessData *eventProcessData;

    eventProcessData = GetW04BooEventProcessData(processIndex);
    BoardModelKill(eventProcessData->modelId);
    HuPrcKill(eventProcessData->eventProcess);
    HuPrcKill(booEventProcesses[processIndex]);
    HuMemDirectFree(booEventProcesses[processIndex]->user_data);
}

static void SetW04BooEventState(s32 eventState)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        if (GetW04BooEventProcessData(i)->isActive != 0) {
            GetW04BooEventProcessData(i)->eventState = eventState;
        }
    }
}

static s32 CheckW04BooEventState(u32 expectedEventState)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        if (expectedEventState != GetW04BooEventProcessData(i)->eventState) {
            break;
        }
    }
    if (i != 4) {
        return 0;
    }
    else {
        return 1;
    }
}

static BooEventProcessData *GetW04BooEventProcessData(s16 processIndex)
{
    return booEventProcesses[processIndex]->user_data;
}

static void RunW04BooMotionProcess(void)
{
    Vec modelPosition;
    BooEventProcessData *eventProcessData;

    eventProcessData = HuPrcCurrentGet()->user_data;
    while (TRUE) {
        BoardModelPosGet(eventProcessData->modelId, &modelPosition);
        modelPosition.y = eventProcessData->verticalOffset + 4.0 * sind(eventProcessData->motionAngle);
        BoardModelPosSetV(eventProcessData->modelId, &modelPosition);
        if (eventProcessData->playerIndex != -1) {
            modelPosition.y += 50.0f;
            BoardPlayerPosSetV(eventProcessData->playerIndex, &modelPosition);
        }
        eventProcessData->motionAngle += 2.0f;
        HuPrcVSleep();
    }
}

static void RunW04BooPlayerEventProcess(void)
{
    Vec modelPosition;
    Vec movementStep;
    float targetAngle;
    BooEventProcessData *eventProcessData;
    BoardSpace *boardSpace;
    HUPROCESS *process;
    s16 targetSpace;
    s16 playerIndex;
    s32 i;

    targetSpace = 0;
    eventProcessData = HuPrcCurrentGet()->user_data;
    while (TRUE) {
        switch (eventProcessData->eventState) {
            case 0:
            case 4:
                break;
            case 1:
                BoardModelPosGet(eventProcessData->modelId, &modelPosition);
                BoardMusStart(1, 0x20, 0x7F, 0);
                booChainEventFlag = 1;
                HuAudFXPlay(0x45F);
                playerIndex = FindW04BooNearestPlayer(&modelPosition);
                HuPrcSleep(45);
                if (eventProcessData->eventIndex == 0) {
                    BoardPlayerPosGet(GWSystem.player_curr, &eventProcessData->targetPosition);
                    eventProcessData->targetPosition.y += 65.0f;
                }
                else {
                    eventProcessData->targetPosition = GetW04BooEventProcessData(eventProcessData->eventIndex - 1)->targetPosition;
                    eventProcessData->targetPosition.z -= 300.0f;
                }
                BoardModelPosGet(w04BoardModelIds[3], &modelPosition);
                eventProcessData->verticalOffset = modelPosition.y;
                BoardModelVisibilitySet(eventProcessData->modelId, 1);
                if (eventProcessData->eventIndex == 0) {
                    booSoundEffectId = HuAudFXPlay(0x460);
                }
                for (i = 0; i < 60; i++) {
                    eventProcessData->modelAlpha += 4.25f;
                    BoardModelAlphaSet(eventProcessData->modelId, eventProcessData->modelAlpha);
                    eventProcessData->verticalOffset += 1.0833334f;
                    HuPrcVSleep();
                }
                SetW04BooPlayerState(playerIndex);
                BoardModelPosGet(eventProcessData->modelId, &modelPosition);
                targetAngle = Atan2d(eventProcessData->targetPosition.x - modelPosition.x, eventProcessData->targetPosition.z - modelPosition.z);
                while (W04ModelRotateYToAngle(eventProcessData->modelId, targetAngle, 5.0f) == 0) {
                    HuPrcVSleep();
                }
                VECSubtract(&eventProcessData->targetPosition, &modelPosition, &movementStep);
                VECScale(&movementStep, &movementStep, 0.016666668f);
                for (i = 0; i < 60; i++) {
                    VECAdd(&movementStep, &modelPosition, &modelPosition);
                    BoardModelPosSetV(eventProcessData->modelId, &modelPosition);
                    HuPrcVSleep();
                }
                BoardModelPosSetV(eventProcessData->modelId, &eventProcessData->targetPosition);
                while (W04ModelRotateYToAngle(eventProcessData->modelId, 0.0f, 5.0f) == 0) {
                    HuPrcVSleep();
                }
                if (eventProcessData->eventIndex == 0) {
                    i = GWSystem.player_curr;
                    targetAngle = BoardPlayerRotYGet(i);
                    omVibrate(i, 12, 6, 6);
                    while (TRUE) {
                        if (targetAngle > 0.0f) {
                            targetAngle -= 2.0f;
                            if (targetAngle < 0.0f) {
                                targetAngle = 0.0f;
                            }
                        }
                        BoardPlayerRotYSet(i, targetAngle);
                        BoardPlayerPosGet(i, &movementStep);
                        BoardModelPosGet(eventProcessData->modelId, &modelPosition);
                        movementStep.y += 2.2f;
                        if (movementStep.y > modelPosition.y + 50.0f) {
                            movementStep.y = modelPosition.y + 50.0f;
                            break;
                        }
                        BoardPlayerPosSetV(i, &movementStep);
                        HuPrcVSleep();
                    }
                    BoardPlayerPosSetV(i, &movementStep);
                    BoardPlayerRotYSet(i, 0.0f);
                    BoardPlayerMotionShiftSet(i, booPlayerMotionIds[GWSystem.player_curr][1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
                    eventProcessData->playerIndex = i;
                }
                if (eventProcessData->eventIndex == 0) {
                    BoardModelPosGet(booModelId, &eventProcessData->targetPosition);
                }
                else {
                    BoardModelPosGet(GetW04BooEventProcessData(eventProcessData->eventIndex - 1)->modelId, &eventProcessData->targetPosition);
                }
                eventProcessData->eventState = 0;
                break;
            case 2:
                while (W04ModelRotateYToAngle(eventProcessData->modelId, 90.0f, 3.0f) == 0) {
                    BoardPlayerRotYSet(eventProcessData->playerIndex, BoardModelRotYGet(eventProcessData->modelId));
                    HuPrcVSleep();
                }
                BoardPlayerRotYSet(eventProcessData->playerIndex, BoardModelRotYGet(eventProcessData->modelId));
                eventProcessData->eventState = 0;
                break;
            case 3:
                BoardModelPosGet(eventProcessData->modelId, &modelPosition);
                targetAngle = Atan2d(eventProcessData->targetPosition.x - modelPosition.x, eventProcessData->targetPosition.z - modelPosition.z);
                BoardModelRotYSet(eventProcessData->modelId, targetAngle);
                if (eventProcessData->playerIndex != -1) {
                    BoardPlayerRotYSet(eventProcessData->playerIndex, targetAngle);
                }
                VECSubtract(&eventProcessData->targetPosition, &modelPosition, &movementStep);
                VECScale(&movementStep, &movementStep, 0.022222223f);
                if (eventProcessData->targetSpace > 0) {
                    boardSpace = BoardSpaceGet(0, eventProcessData->targetSpace);
                    if (boardSpace->flag & 1) {
                        targetSpace = 1;
                        if (eventProcessData->playerIndex != -1) {
                            process = HuPrcChildCreate(ReturnW04BooPlayerToSpace, 0x2002, 0x1000, 0, boardMainProc);
                            process->user_data = eventProcessData;
                            activeBooPlayerProcessCount++;
                        }
                    }
                }
                for (i = 0; i < 45; i++) {
                    VECAdd(&modelPosition, &movementStep, &modelPosition);
                    BoardModelPosSetV(eventProcessData->modelId, &modelPosition);
                    if (eventProcessData->playerIndex != -1) {
                        BoardPlayerPosSetV(eventProcessData->playerIndex, &modelPosition);
                    }
                    if (targetSpace != 0) {
                        if (eventProcessData->eventIndex == 2 && booChainEventFlag != 0) {
                            BoardAudSeqFadeOut(1, 1000);
                            booChainEventFlag = 0;
                        }
                        eventProcessData->modelAlpha -= 5.6666665f;
                        BoardModelAlphaSet(eventProcessData->modelId, eventProcessData->modelAlpha);
                    }
                    HuPrcVSleep();
                }
                eventProcessData->previousSpace = eventProcessData->targetSpace;
                if (eventProcessData->eventIndex == 0) {
                    eventProcessData->targetSpace = booPreviousSpace;
                }
                else {
                    eventProcessData->targetSpace = GetW04BooEventProcessData(eventProcessData->eventIndex - 1)->previousSpace;
                }
                BoardModelPosSetV(eventProcessData->modelId, &eventProcessData->targetPosition);
                if (eventProcessData->playerIndex != -1) {
                    BoardPlayerPosSetV(eventProcessData->playerIndex, &eventProcessData->targetPosition);
                }
                if (eventProcessData->eventIndex == 0) {
                    BoardModelPosGet(booModelId, &eventProcessData->targetPosition);
                }
                else {
                    BoardModelPosGet(GetW04BooEventProcessData(eventProcessData->eventIndex - 1)->modelId, &eventProcessData->targetPosition);
                }
                if (targetSpace != 0) {
                    BoardModelVisibilitySet(eventProcessData->modelId, 0);
                    eventProcessData->isActive = 0;
                    eventProcessData->eventState = 4;
                }
                else {
                    eventProcessData->eventState = 0;
                }
                break;
        }
        HuPrcVSleep();
    }
}

static void ReturnW04BooPlayerToSpace(void)
{
    Vec playerPosition;
    Vec landingPosition;
    Vec movementVelocity;
    float facingAngle;
    float jumpHeight;
    float jumpFrame;
    BooEventProcessData *eventProcessData;
    s16 playerIndex;

    OSReport("Getoff\n");
    eventProcessData = HuPrcCurrentGet()->user_data;
    playerIndex = eventProcessData->playerIndex;
    eventProcessData->playerIndex = -1;
    BoardPlayerMotionShiftSet(playerIndex, 4, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    landingPosition = booPlayerPositions[playerIndex];
    GWPlayer[playerIndex].space_curr = booDestinationSpace;
    BoardPlayerPosGet(playerIndex, &playerPosition);
    facingAngle = Atan2d(landingPosition.x - playerPosition.x, landingPosition.z - playerPosition.z);
    BoardPlayerRotYSet(playerIndex, facingAngle);
    VECSubtract(&landingPosition, &playerPosition, &movementVelocity);
    VECScale(&movementVelocity, &movementVelocity, 0.025f);
    jumpHeight = 12.0f;
    jumpFrame = 0.0f;
    while (TRUE) {
        movementVelocity.y = jumpHeight - jumpFrame * jumpFrame * 0.25f * 0.075f;
        jumpFrame += 1.0f;
        VECAdd(&movementVelocity, &playerPosition, &playerPosition);
        if (playerPosition.y <= landingPosition.y) {
            playerPosition.y = landingPosition.y;
            break;
        }
        facingAngle = VECDistanceXZ(&landingPosition, &playerPosition);
        if (facingAngle <= 1.0f) {
            movementVelocity.x = movementVelocity.z = 0.0f;
        }
        BoardPlayerPosSetV(playerIndex, &playerPosition);
        HuPrcVSleep();
    }
    BoardPlayerPosSetV(playerIndex, &landingPosition);
    BoardPlayerIdleSet(playerIndex);
    HuPrcSleep(8);
    BoardPlayerMotBlendSet(playerIndex, 0, 0xF);
    while (!BoardPlayerMotBlendCheck(playerIndex)) {
        HuPrcVSleep();
    }
    activeBooPlayerProcessCount--;
    HuPrcEnd();
}

static s32 ChooseW04BooDestinationSpace(void)
{
    Vec booPosition;
    Vec candidatePosition;
    Vec directionVector;
    float candidateAngles[4];
    s16 candidateSpaceIds[4];
    s16 candidateSpaceTypes[4];
    s16 linkedSpaceIds[4];
    float windowSize[2];
    BoardSpace *boardSpace;
    BoardSpace *boardSpace2;
    float booFacingAngle;
    float stickAngle;
    float selectionPulse;
    s8 stickX;
    s8 stickY;
    s32 destinationSpace;
    s16 candidateIndex;
    s16 windowId;
    s16 playerPort;
    s16 pressedButtons;
    s16 computerDecisionTimer;
    s16 selectedCandidateIndex;
    s16 candidateCount;
    s16 computerDecisionCount;
    s32 currentPlayer;
    s32 i;

    boardSpace = BoardSpaceGet(0, booCurrentSpace);
    if (boardSpace->flag & 8) {
        return -1;
    }
    if (boardSpace->flag & 0x400) {
        candidateCount = 0;
        i = BoardSpaceLinkTargetListGet(0, booCurrentSpace, linkedSpaceIds);
        for (i--; i >= 0; i--) {
            boardSpace2 = BoardSpaceGet(0, linkedSpaceIds[i]);
            if (boardSpace2->flag & 4) {
                candidateSpaceIds[candidateCount] = linkedSpaceIds[i];
                candidateSpaceTypes[candidateCount] = 1;
                candidateCount++;
            }
        }
        for (i = 0; i < boardSpace->link_cnt; i++) {
            boardSpace2 = BoardSpaceGet(0, boardSpace->link[i]);
            if (boardSpace2->flag & 4) {
                candidateSpaceIds[candidateCount] = boardSpace->link[i];
                candidateSpaceTypes[candidateCount] = 0;
                candidateCount++;
            }
        }
        selectedCandidateIndex = 0;
        BoardModelPosGet(booModelId, &booPosition);
        for (i = 0; i < candidateCount; i++) {
            BoardSpacePosGet(0, candidateSpaceIds[i], &candidatePosition);
            booPosition.y = candidatePosition.y;
            VECSubtract(&candidatePosition, &booPosition, &directionVector);
            selectionPulse = Atan2d(directionVector.x, directionVector.z);
            if (selectionPulse < 0.0f) {
                selectionPulse += 360.0f;
            }
            candidateAngles[i] = selectionPulse;
            VECNormalize(&directionVector, &candidatePosition);
            VECScale(&candidatePosition, &candidatePosition, 110.0f);
            candidatePosition.y += 200.0f;
            VECAdd(&booPosition, &candidatePosition, &candidatePosition);
            BoardModelPosSetV(booDestinationMarkerModelIds[i], &candidatePosition);
            BoardModelRotYSet(booDestinationMarkerModelIds[i], selectionPulse);
            BoardModelVisibilitySet(booDestinationMarkerModelIds[i], 1);
        }
        HuWinMesMaxSizeGet(1, windowSize, MAKE_MESSID(19, 27));
        windowId = HuWinCreate(-10000.0f, 368.0f, windowSize[0], windowSize[1], 0);
        HuWinMesSet(windowId, MAKE_MESSID(19, 27));
        HuWinBGTPLvlSet(windowId, 0.0f);
        HuWinMesSpeedSet(windowId, 0);
        selectionPulse = 0.0f;
        currentPlayer = GWSystem.player_curr;
        playerPort = GWPlayer[currentPlayer].port;
        booFacingAngle = BoardModelRotYGet(booModelId) - 180.0f;
        if (booFacingAngle < 0.0f) {
            booFacingAngle += 360.0f;
        }
        computerDecisionTimer = frandmod(30) + 30;
        computerDecisionCount = 0;
        while (TRUE) {
            candidateIndex = selectedCandidateIndex;
            if (GWPlayer[currentPlayer].com) {
                stickX = stickY = 0;
                pressedButtons = 0;
                if (computerDecisionTimer != 0) {
                    computerDecisionTimer--;
                }
                else {
                    computerDecisionTimer = frandmod(10) + 15;
                    if (frandBool() || computerDecisionCount == 3) {
                        pressedButtons = 0x100;
                    }
                    else {
                        OSReport("com sel chg: %d \n", candidateCount);
                        computerDecisionCount++;
                        candidateIndex = frandmod(candidateCount);
                    }
                }
            }
            else {
                stickX = HuPadStkX[playerPort];
                stickY = HuPadStkY[playerPort];
                pressedButtons = HuPadBtnDown[playerPort];
            }
            if (abs(stickX) < 10) {
                stickX = 0;
            }
            if (abs(stickY) < 10) {
                stickY = 0;
            }
            if (stickX != 0 || stickY != 0) {
                stickAngle = Atan2d(-stickX, stickY) - 180.0;
                stickAngle += booFacingAngle;
                if (stickAngle >= 360.0f) {
                    stickAngle -= 360.0f;
                }
                if (stickAngle < 0.0f) {
                    stickAngle += 360.0f;
                }
                for (i = 0; i < candidateCount; i++) {
                    if (fabs(candidateAngles[i] - stickAngle) < 30.0) {
                        candidateIndex = i;
                        break;
                    }
                }
            }
            if (candidateIndex != selectedCandidateIndex) {
                HuAudFXPlay(0);
                selectionPulse = 0.0f;
                selectedCandidateIndex = candidateIndex;
            }
            if (pressedButtons & 0x100) {
                break;
            }
            for (i = 0; i < candidateCount; i++) {
                if (selectedCandidateIndex == i) {
                    directionVector.x = directionVector.y = directionVector.z = 1.8f + sind(selectionPulse);
                    selectionPulse += 8.0f;
                    if (selectionPulse > 180.0f) {
                        selectionPulse -= 180.0f;
                    }
                }
                else {
                    directionVector.x = directionVector.y = directionVector.z = 1.8f;
                }
                BoardModelScaleSetV(booDestinationMarkerModelIds[i], &directionVector);
            }
            HuPrcVSleep();
        }
        HuAudFXPlay(2);
        for (i = 0; i < candidateCount; i++) {
            BoardModelVisibilitySet(booDestinationMarkerModelIds[i], 0);
        }
        HuWinKill(windowId);
        destinationSpace = candidateSpaceIds[selectedCandidateIndex];
        booRouteChoiceFlag = candidateSpaceTypes[selectedCandidateIndex];
        booTargetPlayerCount = FindW04BooTargetPlayer(destinationSpace);
    }
    else if (booRouteChoiceFlag != 0) {
        i = BoardSpaceLinkTargetListGet(0, booCurrentSpace, linkedSpaceIds);
        for (i--; i >= 0; i--) {
            boardSpace2 = BoardSpaceGet(0, linkedSpaceIds[i]);
            if (boardSpace2->flag & 0x200) {
                break;
            }
        }
        destinationSpace = linkedSpaceIds[i];
    }
    else {
        for (i = 0; i < boardSpace->link_cnt; i++) {
            boardSpace2 = BoardSpaceGet(0, boardSpace->link[i]);
            if (boardSpace2->flag & 0x200) {
                break;
            }
        }
        destinationSpace = boardSpace->link[i];
    }
    return destinationSpace;
}

static void RunW04BooPlayerInteraction(void)
{
    float *floatValue;
    s32 cameraWaitFrames;

    cameraWaitFrames = 15;
    floatValue = (float *)&HuPrcCurrentGet()->user_data;
    while (TRUE) {
        if (cameraWaitFrames == 0) {
            BoardCameraMoveSet(0);
            BoardCameraTargetPlayerSet(GWSystem.player_curr);
            BoardCameraZoomSet(700.0f);
            BoardStatusShowSetAll(0);
            break;
        }
        cameraWaitFrames--;
        HuPrcVSleep();
    }
    while (TRUE) {
        BoardCameraRotSet(-15.0f, *floatValue);
        HuPrcVSleep();
    }
}

static void ResetW04BooEventCamera(void)
{
    BoardCameraMoveSet(0);
    BoardCameraTargetPlayerSet(GWSystem.player_curr);
    BoardCameraViewSet(2);
    BoardStatusShowSetAll(1);
    BoardCameraMoveSet(1);
}

static s32 FindW04BooTargetPlayer(s16 targetSpace)
{
    BoardSpace *boardSpace;
    BoardSpace *boardSpace2;
    s16 linkedSpaceIds[4];
    s16 routeEndFound;
    s32 targetPlayerCount;
    s32 i;

    targetPlayerCount = 0;
    routeEndFound = 0;
    boardSpace = BoardSpaceGet(0, targetSpace);
    booTargetPlayerCount = 0;
    while (TRUE) {
        for (i = 0; i < 4; i++) {
            if (i != GWSystem.player_curr && targetSpace == GWPlayer[i].space_curr) {
                booTargetPlayerIds[targetPlayerCount++] = i;
            }
        }
        if (targetPlayerCount == 3 || routeEndFound == 1) {
            break;
        }
        if (booRouteChoiceFlag != 0) {
            i = BoardSpaceLinkTargetListGet(0, targetSpace, linkedSpaceIds);
            for (i--; i >= 0; i--) {
                boardSpace2 = BoardSpaceGet(0, linkedSpaceIds[i]);
                if (boardSpace2->flag & 0x200) {
                    break;
                }
            }
            targetSpace = linkedSpaceIds[i];
        }
        else {
            for (i = 0; i < boardSpace->link_cnt; i++) {
                boardSpace2 = BoardSpaceGet(0, boardSpace->link[i]);
                if (boardSpace2->flag & 0x200) {
                    break;
                }
            }
            targetSpace = boardSpace->link[i];
        }
        boardSpace = BoardSpaceGet(0, targetSpace);
        if (boardSpace->flag & 0x401) {
            routeEndFound = 1;
        }
    }
    return targetPlayerCount;
}

static void StealW04BooPlayerItem(void)
{
    Vec playerPosition;
    Vec booPosition;
    float playerDistance;
    BooEventProcessData *eventProcessData;
    s32 playerIndex;
    s32 i;

    playerIndex = (s32)HuPrcCurrentGet()->user_data;
    BoardPlayerPosGet(playerIndex, &playerPosition);
    BoardPlayerMotBlendSet(playerIndex, BoardModelRotYGet(booModelId) - 180.0f, 0xA);
    while (!BoardPlayerMotBlendCheck(playerIndex)) {
        BoardModelPosGet(booModelId, &booPosition);
        playerDistance = VECDistanceXZ(&playerPosition, &booPosition);
        if (playerDistance <= 200.0f) {
            PlayerFXPlay(playerIndex, 0x128);
            goto blockA;
        }
        HuPrcVSleep();
    }
    BoardPlayerMotionShiftSet(playerIndex, booPlayerMotionIds[playerIndex][0], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    PlayerFXPlay(playerIndex, 0x128);
    for (i = 0; i < 65; i++) {
        BoardModelPosGet(booModelId, &booPosition);
        playerDistance = VECDistanceXZ(&playerPosition, &booPosition);
        if (playerDistance <= 200.0f) {
            goto blockA;
        }
        HuPrcVSleep();
    }
    BoardPlayerMotionShiftSet(playerIndex, 1, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    BoardModelPosGet(booModelId, &booPosition);
    playerDistance = VECDistanceXZ(&playerPosition, &booPosition);
    if (playerDistance > 200.0f) {
        HuPrcVSleep();
    }
blockA:
    booPlayerEventStates[playerIndex] = 2;
    BoardPlayerMotionStart(playerIndex, booPlayerMotionIds[playerIndex][2], 0x40000001);
    playerDistance = 20.0f;
    omVibrate(playerIndex, 12, 4, 2);
    while (TRUE) {
        playerPosition.y += playerDistance;
        BoardPlayerPosSetV(playerIndex, &playerPosition);
        if (playerPosition.y >= 600.0f) {
            break;
        }
        HuPrcVSleep();
    }
    BoardPlayerMotionStart(playerIndex, booPlayerMotionIds[playerIndex][1], 0x40000001);
    for (i = 0; i < 4; i++) {
        eventProcessData = GetW04BooEventProcessData(i);
        if (eventProcessData->playerIndex == -1) {
            break;
        }
    }
    eventProcessData->playerIndex = playerIndex;
    BoardPlayerRotYSet(playerIndex, BoardModelRotYGet(eventProcessData->modelId));
    for (i = 0; i < 4; i++) {
        HuPrcSetStat(booEventProcesses[i], 1);
    }
    for (i = 0; i < 4; i++) {
        if (booPlayerProcesses[i] && i != playerIndex) {
            HuPrcSetStat(booPlayerProcesses[i], 1);
        }
    }
    HuPrcSetStat(booEventProcess, 1);
    BoardWinCreate(0, MAKE_MESSID(32, 2), -1);
    BoardWinInsertMesSet(GWPlayer[playerIndex].character, 0);
    BoardWinWait();
    BoardWinKill();
    for (i = 0; i < 4; i++) {
        HuPrcResetStat(booEventProcesses[i], 1);
    }
    for (i = 0; i < 4; i++) {
        if (booPlayerProcesses[i] && i != playerIndex) {
            HuPrcResetStat(booPlayerProcesses[i], 1);
        }
    }
    HuPrcResetStat(booEventProcess, 1);
    booPlayerProcesses[playerIndex] = NULL;
    HuPrcEnd();
}

static void RunW04BooLaunchProcess(void)
{
    Vec playerPosition;
    Vec booPosition;
    float playerDistance;
    HUPROCESS *process;
    s32 i;

    BoardModelPosGet(booModelId, &booPosition);
    for (i = 0; i < booTargetPlayerCount; i++) {
        BoardPlayerPosGet(booTargetPlayerIds[i], &playerPosition);
        playerDistance = VECDistanceXZ(&playerPosition, &booPosition);
        if (playerDistance < 900.0f && booPlayerEventStates[booTargetPlayerIds[i]] == 0) {
            process = HuPrcChildCreate(StealW04BooPlayerItem, 0x2003, 0x2000, 0, boardMainProc);
            booPlayerProcesses[booTargetPlayerIds[i]] = process;
            process->user_data = (u8 *)booTargetPlayerIds[i];
            booPlayerEventStates[booTargetPlayerIds[i]] = 1;
        }
    }
}

static void AlignW04BooPlayers(s16 targetSpace, Vec *playerPositions)
{
    Vec originalPlayerPositions[4];
    Vec originalPlayerRotations[4];
    BooEventProcessData *eventProcessData;
    BoardSpace *boardSpace;
    BoardSpace *boardSpace2;
    s16 currentPlayerSpaces[4];
    s32 playersAtDestinationCount;
    s32 i;

    OSReport("Landing Pos Get\n");
    booDestinationSpace = -1;
    boardSpace = BoardSpaceGet(0, targetSpace);
    for (i = 0; i < boardSpace->link_cnt; i++) {
        boardSpace2 = BoardSpaceGet(0, boardSpace->link[i]);
        if (boardSpace2->flag & 2) {
            booDestinationSpace = boardSpace->link[i];
            break;
        }
    }
    if (booDestinationSpace == -1) {
        i = BoardSpaceLinkTargetListGet(0, targetSpace, currentPlayerSpaces);
        for (i--; i >= 0; i--) {
            boardSpace2 = BoardSpaceGet(0, currentPlayerSpaces[i]);
            if (boardSpace2->flag & 2) {
                booDestinationSpace = currentPlayerSpaces[i];
                break;
            }
        }
    }
    if (booDestinationSpace == -1) {
        OSReport("\x92\x85\x92\x6E\x92\x6E\x93_\x82\xAA\x96\xB3\x82\xA2\n");
        while (TRUE) {
            HuPrcVSleep();
        }
    }
    playersAtDestinationCount = 0;
    for (i = 0; i < 4; i++) {
        BoardPlayerPosGet(i, &originalPlayerPositions[i]);
        BoardPlayerRotGet(i, &originalPlayerRotations[i]);
        currentPlayerSpaces[i] = GWPlayer[i].space_curr;
        if (booDestinationSpace == currentPlayerSpaces[i]) {
            playersAtDestinationCount += 1;
        }
    }
    for (i = 0; i < 4; i++) {
        eventProcessData = GetW04BooEventProcessData(i);
        if (eventProcessData->playerIndex != -1 && eventProcessData->playerIndex != GWSystem.player_curr) {
            BoardSpaceCornerPosGet(booDestinationSpace, playersAtDestinationCount++, &playerPositions[eventProcessData->playerIndex]);
        }
    }
    for (i = 0; i < 4; i++) {
        BoardPlayerPosSetV(i, &originalPlayerPositions[i]);
        BoardPlayerRotSetV(i, &originalPlayerRotations[i]);
        GWPlayer[i].space_curr = currentPlayerSpaces[i];
    }
    BoardSpacePosGet(0, booDestinationSpace, &playerPositions[GWSystem.player_curr]);
}

static void FinishW04BooEventProcess(void)
{
    booEventProcess = NULL;
}

void W04BooEvent(void)
{
    s32 currentPlayer;

    currentPlayer = GWSystem.player_curr;
    booEventProcess = HuPrcChildCreate(RunW04BooEventSequence, 0x2004, 0x2000, 0, boardMainProc);
    HuPrcDestructorSet2(booEventProcess, FinishW04BooEventProcess);
    while (booEventProcess) {
        HuPrcVSleep();
    }
}

static void CreateW04BooParticles(void)
{
    HU3DPARTICLE *particle;
    s32 i;

    booAnimationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_W04, 6));
    for (i = 0; i < 4; i++) {
        booParticleObjects[i] = omAddObjEx(boardObjMan, 0x101, 1, 0, -1, UpdateW04BooParticleObject);
        booParticleObjects[i]->mdlId[0] = Hu3DParticleCreate(booAnimationData, 100);
        Hu3DParticleColSet(booParticleObjects[i]->mdlId[0], 0x40, 0xD2, 0xFF);
        Hu3DParticleScaleSet(booParticleObjects[i]->mdlId[0], 5.0f);
        Hu3DParticleHookSet(booParticleObjects[i]->mdlId[0], UpdateW04BooParticleEffect);
        Hu3DParticleBlendModeSet(booParticleObjects[i]->mdlId[0], 1);
        Hu3DModelAttrSet(booParticleObjects[i]->mdlId[0], HU3D_ATTR_DISPOFF);
        particle = Hu3DData[booParticleObjects[i]->mdlId[0]].hookData;
        particle->work = booParticleObjects[i];
    }
}

static void CleanupW04BooParticles(void)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        Hu3DModelKill(booParticleObjects[i]->mdlId[0]);
        omDelObjEx(boardObjMan, booParticleObjects[i]);
    }
}

static s16 FindW04BooNearestPlayer(Vec *targetPosition)
{
    s16 i;

    for (i = 0; i < 4; i++) {
        if (booParticleObjects[i]->work[0] == 0) {
            booParticleObjects[i]->work[0] = 1;
            booParticleObjects[i]->trans.x = targetPosition->x;
            booParticleObjects[i]->trans.y = targetPosition->y;
            booParticleObjects[i]->trans.z = targetPosition->z;
            return i;
        }
    }
    return -1;
}

static void SetW04BooPlayerState(s16 playerIndex)
{
    booParticleObjects[playerIndex]->work[1] = 1;
    booParticleObjects[playerIndex]->work[2] = 0;
}

static void UpdateW04BooParticleObject(OMOBJ *particleObject)
{
    if (particleObject->work[0] != 0) {
        Hu3DModelAttrReset(particleObject->mdlId[0], HU3D_ATTR_DISPOFF);
    }
    if (particleObject->work[2] != 0) {
        particleObject->work[0] = 0;
        Hu3DModelAttrSet(particleObject->mdlId[0], HU3D_ATTR_DISPOFF);
    }
}

static void UpdateW04BooParticleEffect(HU3DMODEL *model, HU3DPARTICLE *particle, Mtx matrix)
{
    s32 inactiveParticleCount;
    s32 i;
    s32 j;
    HU3DPARTICLEDATA *particleData;
    BooParticleEffectState *data;

    data = particle->work;
    if (particle->count == 0) {
        particleData = particle->data;
        for (i = 0; i < particle->maxCnt; i++, particleData++) {
            particleData->vel.z = 0.0f;
            particleData->scale = 0.0f;
        }
    }
    if (data->particleEffectState == 0) {
        for (i = 0; i < 3; i++) {
            particleData = particle->data;
            for (j = 0; j < particle->maxCnt; j++, particleData++) {
                if (particleData->vel.z == 0.0f) {
                    break;
                }
            }
            if (j != particle->maxCnt) {
                particleData->vel.x = 0.003921569f * frand8() * 360.0f;
                particleData->vel.y = 1.0f + 0.003921569f * frand8() * 1.5f;
                particleData->vel.z = 240.0f;
                particleData->color.r = 0x40;
                particleData->color.g = 0xD2;
                particleData->color.b = 0xFF;
                particleData->speedDecay = 2.0f + 0.003921569f * frand8() * 8.0f;
                particleData->colorIdx = 2.0f + 0.003921569f * frand8() * 6.0f;
                particleData->scaleBase = 1.0f + 0.003921569f * frand8() * 3.0f;
                particleData->pos.x = -100.0f + 0.003921569f * frand8() * 200.0f;
                particleData->pos.y = 0.003921569f * frand8() * 10.0f;
                particleData->pos.z = -100.0f + 0.003921569f * frand8() * 200.0f;
                particleData->scale = 7.0f + 0.003921569f * frand8() * 3.0f;
            }
        }
    }
    particleData = particle->data;
    inactiveParticleCount = 0;
    for (i = 0; i < particle->maxCnt; i++, particleData++) {
        if (particleData->vel.z != 0.0f) {
            particleData->pos.x += particleData->scaleBase * sind(particleData->vel.x);
            particleData->pos.y += particleData->vel.y;
            particleData->pos.z += particleData->scaleBase * cosd(particleData->vel.x);
            particleData->vel.x += particleData->colorIdx * sind(particleData->vel.x);
            if (particleData->vel.x >= 360.0f) {
                particleData->vel.x -= 360.0f;
            }
            particleData->color.a = particleData->vel.z;
            particleData->vel.z -= particleData->speedDecay;
            if (particleData->vel.z <= 0.0f) {
                particleData->vel.z = 0.0f;
            }
        }
        else {
            inactiveParticleCount++;
        }
    }
    if (inactiveParticleCount == particle->maxCnt) {
        data->isActive = 1;
    }
}

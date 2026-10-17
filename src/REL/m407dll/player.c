#include "game/audio.h"
#include "game/chrman.h"
#include "game/frand.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "math.h"

#include "REL/m407dll.h"

typedef struct PlayerCharacterWork {
    u8 playerIndex;
    u8 characterId;
    u8 difficulty;
    // padding unk_03
    s32 isComputer;
    s32 animationId;
    u8 controllerPort;
    // padding unk_0D
    f32 positionX;
    f32 positionY;
    f32 positionZ;
    s32 actionState;
    s32 actionFrame;
    f32 verticalVelocity;
    f32 inputFrame;
    f32 nextInputFrame;
    f32 inputFrameStep;
    s32 motionTransitionActive;
    s32 motionAction;
} PlayerCharacterWork;

// function signatures
void SetPlayerActionState(OMOBJ *, s32);
void CreatePlayerCharacter(OMOBJ *);
void DestroyPlayerCharacter(OMOBJ *);
void UpdatePlayerCharacter(OMOBJ *object);
void IncrementPlayerWhompCount(u8);
void SpawnEffect(s32, s32, f32, f32, f32);
s16 GetPlayerWhompCount(u8);
s16 GetPlayerWhompModelId(u8);
void PlayerMotionStateIdle(OMOBJ *object);
void PlayerMotionStateWalk(OMOBJ *object);
void PlayerMotionStateReact(OMOBJ *object);
void PlayerMotionStateJump(OMOBJ *object);
void PlayerMotionStateCheer(OMOBJ *object);
void PlayerMotionStateFall(OMOBJ *object);
void PlayerMotionStateWin(OMOBJ *object);
void PlayerMotionStateLose(OMOBJ *object);
void PlayerMotionStateLand(OMOBJ *object);
void PlayerMotionStateSpecial(OMOBJ *object);

// bss
HUPROCESS *playerProcess;
OMOBJ *playerObjects[4];
HUPROCESS *playerIdleProcesses[2];

// data
u32 characterMotionAssetIds[8][8] = {
    {
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x24),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x29),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x4A),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x4B),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x24),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x29),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x4A),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x4B),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x24),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x29),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x4A),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x4B),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x24),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x29),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x4A),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x4B),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x24),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x29),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x4A),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x4B),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x24),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x29),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x4A),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x4B),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x24),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x29),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x4A),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x4B),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x18),
    },
    {
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x24),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x29),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x05),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x06),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x4A),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x4B),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x18),
    },
};

s32 characterModelAssetIds[] = {
    DATA_MAKE_NUM(DATADIR_M407, 0x0D),
    DATA_MAKE_NUM(DATADIR_M407, 0x0E),
    DATA_MAKE_NUM(DATADIR_M407, 0x0F),
    DATA_MAKE_NUM(DATADIR_M407, 0x10),
    DATA_MAKE_NUM(DATADIR_M407, 0x11),
    DATA_MAKE_NUM(DATADIR_M407, 0x12),
    DATA_MAKE_NUM(DATADIR_M407, 0x13),
    DATA_MAKE_NUM(DATADIR_M407, 0x14),
};

char itemHookTextureName[] = "test11_tex_we-itemhook-r";
char *characterHookNames[]
    = { itemHookTextureName, itemHookTextureName, itemHookTextureName, itemHookTextureName, itemHookTextureName, itemHookTextureName, itemHookTextureName, itemHookTextureName };

ObjFuncs playerActionCallbacks[] = { PlayerMotionStateIdle, PlayerMotionStateWalk, PlayerMotionStateReact, PlayerMotionStateJump, PlayerMotionStateCheer, PlayerMotionStateFall, PlayerMotionStateWin, PlayerMotionStateLose, PlayerMotionStateLand, PlayerMotionStateSpecial };

u8 playerSpawnOffsets[4][2] = { { 0x3C, 0x0F }, { 0x50, 0x0F }, { 0x64, 0x14 }, { 0x78, 0x14 } };

inline void SetPlayerActionAndMotion(OMOBJ *object, s32 actionState, s32 actionFrame)
{
    PlayerCharacterWork *playerWork = object->data;
    playerWork->actionState = actionState;
    playerWork->actionFrame = actionFrame;
}

void ObjectSetup(void)
{
    playerIdleProcesses[0] = omInitObjMan(0x2F8, 0x2000);
    omGameSysInit(playerIdleProcesses[0]);
    InitializeMinigameSequence(playerIdleProcesses[0]);
}

void InitializePlayerSystem(HUPROCESS *process)
{
    u32 i;
    playerProcess = process;

    for (i = 0; i < 4; i++) {
        playerObjects[i] = omAddObjEx(playerProcess, 0x100, 2, 8, 0, CreatePlayerCharacter);
        playerObjects[i]->work[0] = i;
    }
}

void PlayPlayerIdleMotion(void)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        DestroyPlayerCharacter(playerObjects[i]);
    }
}

void PlayPlayerWalkMotion(void)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        SetPlayerActionState(playerObjects[i], 1);
    }
}

void PlayPlayerTurnMotion(void)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        SetPlayerActionState(playerObjects[i], 2);
    }
}

void PlayPlayerReactionMotion(void)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        SetPlayerActionState(playerObjects[i], 3);
    }
}

void PlayPlayerJumpMotion(void)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        SetPlayerActionState(playerObjects[i], 4);
    }
}

void PlayPlayerLandMotion(void)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        SetPlayerActionState(playerObjects[i], 5);
    }
}

void SetPlayerMotionAction(u8 playerIndex, s32 motionAction)
{
    OMOBJ *obj = playerObjects[playerIndex];
    PlayerCharacterWork *playerWork = obj->data;
    playerWork->motionAction = motionAction;
}

void PlayPlayerMotionAction(u8 playerIndex)
{
    SetPlayerActionState(playerObjects[playerIndex], 8);
}

void StopPlayerMotion(void)
{
    OMOBJ *workData;
    PlayerCharacterWork *playerWork;
    s32 void;
    u8 i;

    for (i = 0; i < 4; i++) {
        workData = playerObjects[i];
        playerWork = workData->data;
        switch (playerWork->motionAction) {
            case 0:
                SetPlayerActionState(workData, 9);
                break;
            case 1:
                SetPlayerActionState(workData, 6);
                break;
            case 2:
                SetPlayerActionState(workData, 9);
                break;
        }
    }
}

s32 GetPlayerMotionState(u8 playerIndex)
{
    PlayerCharacterWork *playerWork;

    if (playerIndex >= 4) {
        return -1;
    }

    playerWork = playerObjects[playerIndex]->data;
    return playerWork->characterId;
}

s32 GetPlayerActionState(u8 playerIndex)
{
    PlayerCharacterWork *playerWork = playerObjects[playerIndex]->data;
    return playerWork->isComputer;
}

s32 IsPlayerIdle(u8 playerIndex)
{
    return ((PlayerCharacterWork *)(playerObjects[playerIndex]->data))->actionState == 0;
}

void VibratePlayerController(s16 vibrationPattern, s16 vibrationDuration, s16 vibrationStrength)
{
    PlayerCharacterWork *playerWork;

    playerWork = playerObjects[vibrationPattern]->data;
    switch (vibrationPattern) {
        case 0:
            omVibrate(playerWork->playerIndex, vibrationDuration, 6, 6);
            break;
        case 1:
            omVibrate(playerWork->playerIndex, vibrationDuration, 4, 2);
            break;
        case 2:
            omVibrate(playerWork->playerIndex, vibrationDuration, 0xC, 0);
            break;
    }
}

void SetPlayerActionState(OMOBJ *object, s32 actionState)
{
    PlayerCharacterWork *workData;

    workData = object->data;
    workData->actionState = actionState;
    workData->actionFrame = 0;
}

void CreatePlayerCharacter(OMOBJ *object)
{
    u32 playerIndex;
    PlayerCharacterWork *unusedPlayerWork;
    PlayerCharacterWork *workData;
    u32 i;

    object->objFunc = UpdatePlayerCharacter;
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(PlayerCharacterWork), HU_MEMNUM_OVL);
    workData = object->data;
    playerIndex = object->work[0];
    workData->playerIndex = playerIndex;
    workData->characterId = GWPlayerCfg[playerIndex].character;
    workData->controllerPort = GWPlayerCfg[playerIndex].pad_idx;
    workData->isComputer = GWPlayerCfg[playerIndex].iscom;
    workData->difficulty = GWPlayerCfg[playerIndex].diff;
    workData->animationId = 0;
    workData->motionTransitionActive = 0;
    workData->motionAction = 0;
    workData->inputFrame = 0.0f;
    workData->inputFrameStep = (600.0f / (playerSpawnOffsets[workData->difficulty][0] - frandmod31(playerSpawnOffsets[workData->difficulty][1])));
    workData->nextInputFrame = workData->inputFrameStep;
    object->mdlId[0] = CharModelCreate(workData->characterId, 8);
    object->mdlId[1] = Hu3DModelCreateFile(characterModelAssetIds[workData->characterId]);
    Hu3DModelAmbSet(object->mdlId[0], 1.0f, 1.0f, 1.0f);
    Hu3DModelAmbSet(object->mdlId[1], 1.0f, 1.0f, 1.0f);

    for (i = 0; i < 8; i++) {
        object->mtnId[i] = CharMotionCreate(workData->characterId, characterMotionAssetIds[workData->characterId][i]);
    }

    Hu3DModelShadowSet(object->mdlId[0]);
    Hu3DModelShadowSet(object->mdlId[1]);
    CharMotionDataClose(workData->characterId);
    CharMotionSet(workData->characterId, object->mtnId[1]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    Hu3DModelHookSet(object->mdlId[0], characterHookNames[workData->characterId], object->mdlId[1]);
    workData->positionX = (600.0f - (400.0f * playerIndex));
    workData->positionY = 0.0f;
    workData->positionZ = -500.0f;
    SetPlayerActionAndMotion(object, 0, 0);
}

void DestroyPlayerCharacter(OMOBJ *object)
{
    s32 i;

    for (i = 0; i < 8; i++) {
        Hu3DMotionKill(object->mtnId[i]);
    }

    for (i = 0; i < 2; i++) {
        Hu3DModelKill(object->mdlId[i]);
    }

    HuMemDirectFree(object->data);
}

void UpdatePlayerCharacter(OMOBJ *object)
{
    PlayerCharacterWork *workData;

    workData = object->data;
    playerActionCallbacks[workData->actionState](object);
    omSetTra(object, workData->positionX, workData->positionY, workData->positionZ);
}

void PlayerMotionStateIdle(OMOBJ *object) { }

void PlayerMotionStateWalk(OMOBJ *object)
{
    s32 actionPressed;
    PlayerCharacterWork *workData;

    actionPressed = 0;
    workData = object->data;
    workData->inputFrame += 1.0f;

    if (workData->isComputer == 0) {
        actionPressed = (HuPadBtnDown[workData->controllerPort] & 0x100) ? 1 : 0;
    }
    else if ((u32)workData->inputFrame >= (u32)workData->nextInputFrame) {
        actionPressed = 1;
        workData->nextInputFrame += workData->inputFrameStep;
    }

    if (actionPressed != 0) {
        CharMotionShiftSet(workData->characterId, object->mtnId[2], 0.0f, 0.0f, HU3D_MOTATTR_NONE);
        SpawnEffect(4, 0x10, 600.0f - (400.0f * workData->playerIndex), -20.0f, -450.0f);
        IncrementPlayerWhompCount(workData->playerIndex);
        workData->motionTransitionActive = 1;
        return;
    }

    if ((workData->motionTransitionActive != 0) && (Hu3DMotionEndCheck(object->mdlId[0]) != 0)) {
        CharMotionShiftSet(workData->characterId, object->mtnId[1], 0.0f, 0.0f, HU3D_MOTATTR_LOOP);
        workData->motionTransitionActive = 0;
    }
}

void PlayerMotionStateReact(OMOBJ *object)
{
    PlayerCharacterWork *playerWork;

    playerWork = object->data;
    if (Hu3DMotionEndCheck(object->mdlId[0]) != 0) {
        CharMotionShiftSet(playerWork->characterId, object->mtnId[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
        playerWork->motionTransitionActive = 0;
        SetPlayerActionAndMotion(object, 0, 0);
    }
}

void PlayerMotionStateJump(OMOBJ *object)
{
    PlayerCharacterWork *workData;

    workData = object->data;
    if (workData->actionFrame == 0) {
        CharMotionShiftSet(workData->characterId, object->mtnId[1], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
        CharModelHookDustCreate(workData->characterId, characterHookNames[workData->characterId]);
    }
    if (workData->actionFrame++ >= 30) {
        SetPlayerActionAndMotion(object, 0, 0);
    }
}

void PlayerMotionStateCheer(OMOBJ *object)
{
    f32 verticalDisplacement;
    f32 forwardDisplacement;
    PlayerCharacterWork *workData;

    workData = object->data;

    if (workData->actionFrame == 0) {
        CharMotionVoiceOnSet(workData->characterId, object->mtnId[3], 0);
        CharMotionShiftSet(workData->characterId, object->mtnId[3], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
        workData->verticalVelocity = 0.0f;
        workData->actionFrame++;
    }

    verticalDisplacement = 32.0f * sin(M_PI / 3) - 9.8f * workData->verticalVelocity;
    forwardDisplacement = 22.0f * cos(M_PI / 3);
    workData->positionZ = workData->positionZ + forwardDisplacement;
    workData->positionY = workData->positionY + verticalDisplacement;
    workData->verticalVelocity += 0.3f;

    if (!(verticalDisplacement < 0.0f) || !(workData->positionY <= 30.0f)) {
        return;
    }

    workData->positionY = 30.0f;
    CharMotionShiftSet(workData->characterId, object->mtnId[4], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    SetPlayerActionAndMotion(object, 0, 0);
}

// can also be placed into player_motion_state_fall as static const
const s32 characterFallSoundIds[] = { 0x00000122, 0x00000162, 0x000001A2, 0x000001E2, 0x00000222, 0x00000262, 0x000002A2, 0x000002E2 };

void PlayerMotionStateFall(OMOBJ *object)
{
    PlayerCharacterWork *workData;

    workData = object->data;

    if (workData->actionFrame == 0) {
        CharMotionShiftSet(workData->characterId, object->mtnId[5], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
        HuAudFXPlay(characterFallSoundIds[workData->characterId]);
    }

    if (workData->actionFrame++ >= 120) {
        CharMotionShiftSet(workData->characterId, object->mtnId[0], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
        SetPlayerActionAndMotion(object, 0, 0);
    }
}

void PlayerMotionStateWin(OMOBJ *object)
{
    PlayerCharacterWork *workData;

    workData = object->data;
    if (workData->actionFrame == 0) {
        CharMotionVoiceOnSet(workData->characterId, object->mtnId[3], 0);
        CharMotionShiftSet(workData->characterId, object->mtnId[3], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
        workData->verticalVelocity = 0.0f;
        workData->actionFrame = 1;
    }

    workData->positionY += 80.0 * sin(M_PI / 2) - 9.8 * workData->verticalVelocity;
    workData->verticalVelocity += 0.3f;

    if (!(workData->positionY < 1000.0f)) {
        CharMotionShiftSet(workData->characterId, object->mtnId[4], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
        SetPlayerActionAndMotion(object, 7, 0);
    }
}

void PlayerMotionStateLose(OMOBJ *object)
{
    PlayerCharacterWork *workData;
    s16 whompModelId;

    workData = object->data;
    if (workData->actionFrame == 0) {
        CharMotionVoiceOnSet(workData->characterId, object->mtnId[3], 0);
        CharMotionShiftSet(workData->characterId, object->mtnId[3], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
        workData->positionY = 1000.0f;
        workData->positionZ = 200.0f * (GetPlayerWhompCount(workData->playerIndex) + 2);
        workData->verticalVelocity = 0.0f;
        workData->actionFrame = 1;
    }

    workData->positionY = (workData->positionY - (2.0f + (9.8f * workData->verticalVelocity)));
    workData->verticalVelocity += 0.3f;

    if (!(workData->positionY > 300.0f)) {
        workData->positionY = 300.0f;
        CharMotionVoiceOnSet(workData->characterId, object->mtnId[4], 0);
        CharMotionShiftSet(workData->characterId, object->mtnId[4], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
        whompModelId = GetPlayerWhompModelId(workData->playerIndex);
        Hu3DModelHookSet(whompModelId, "itemhook_c", object->mdlId[0]);
        workData->positionX = 0.0f;
        workData->positionY = 0.0f;
        workData->positionZ = 0.0f;
        SetPlayerActionAndMotion(object, 0, 0);
    }
}

void PlayerMotionStateLand(OMOBJ *object)
{
    PlayerCharacterWork *playerWork;

    playerWork = object->data;
    CharMotionShiftSet(playerWork->characterId, object->mtnId[6], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    SetPlayerActionAndMotion(object, 0, 0);
}

void PlayerMotionStateSpecial(OMOBJ *object)
{
    PlayerCharacterWork *playerWork;

    playerWork = object->data;
    CharMotionVoiceOnSet(playerWork->characterId, object->mtnId[7], 0);
    CharMotionShiftSet(playerWork->characterId, object->mtnId[7], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    SetPlayerActionAndMotion(object, 0, 0);
}

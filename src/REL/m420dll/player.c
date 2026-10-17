#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/esprite.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"

#include "REL/m420dll.h"

typedef void (*M420DllPlayerActionFunc)(M420DllPlayerStruct *);

typedef struct M420DllPlayerConfig {
    /* 0x00 */ s32 initialWaitFrames;
    /* 0x04 */ s32 initialWaitVariance;
    /* 0x08 */ s32 aiActionDelayFrames;
    /* 0x0C */ s32 aiActionDelayVariance;
    /* 0x10 */ s32 eventDelayFrames;
    /* 0x14 */ s32 eventDelayVariance;
    /* 0x18 */ s32 configuredActionCount;
    /* 0x1C */ s32 actionFlags;
} M420DllPlayerConfig; /* size = 0x20 */

void UpdatePlayerProcess(void);
void CreatePlayerModel(M420DllPlayerStruct *player);
void LoadPlayerMotions(M420DllPlayerStruct *player);
void CreatePlayerEffectSprites(M420DllPlayerStruct *player);
void DestroyPlayerModel(M420DllPlayerStruct *player);
void DestroyPlayerMotions(M420DllPlayerStruct *player);
void DestroyPlayerEffectSprites(M420DllPlayerStruct *player);
void WaitForPlayerActionChange(M420DllPlayerStruct *player);
void HandlePlayerIdle(M420DllPlayerStruct *player);
void HandlePlayerAction(M420DllPlayerStruct *player);
void HandlePlayerWin(M420DllPlayerStruct *player);
void HandlePlayerCelebration(M420DllPlayerStruct *player);
void HandlePlayerMovement(M420DllPlayerStruct *player);
void HandlePlayerEffect(M420DllPlayerStruct *player);
void UpdatePlayerEffectSprites(M420DllPlayerStruct *player);
void HidePlayerEffectSprites(M420DllPlayerStruct *player);
s32 GetPlayerInputState(M420DllPlayerStruct *player);
void UpdatePlayerModel(M420DllPlayerStruct *player);
void AttachPlayerItemHook(M420DllPlayerStruct *player);
void DetachPlayerItemHook(M420DllPlayerStruct *player);
void SpawnPlayerItem(M420DllPlayerStruct *player);
void AnimatePlayerItemSpawn(void);
void HandlePlayerItemAction(M420DllPlayerStruct *player);
void HandlePlayerItemResult(M420DllPlayerStruct *player);
void HandlePlayerItemReturn(M420DllPlayerStruct *player);
void HandlePlayerItemSuccess(M420DllPlayerStruct *player);
void HandlePlayerItemFailure(M420DllPlayerStruct *player);

M420DllPlayerConfig playerConfigTable[4] = {
    { 0x19, 0x01, 0x2A, 0x08, 0x2D, 0x10, 0x01, 0x01 },
    { 0x19, 0x01, 0x22, 0x06, 0x22, 0x0C, 0x01, 0x01 },
    { 0x19, 0x01, 0x1B, 0x05, 0x14, 0x0A, 0x01, 0x01 },
    { 0x19, 0x01, 0x14, 0x04, 0x0A, 0x08, 0x01, 0x01 },
};

static const s32 characterMotionDataIds[8][0xB] = {
    {
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x68),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x69),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x6A),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x35),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x6B),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x6E),
        DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x49),
        0x33001B,
    },
    {
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x18),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x68),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x69),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x6A),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x35),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x6B),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x6E),
        DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x49),
        0x33001C,
    },
    {
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x18),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x68),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x69),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x6A),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x35),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x6B),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x6E),
        DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x49),
        0x33001D,
    },
    {
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x18),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x68),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x69),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x6A),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x35),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x6B),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x6E),
        DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x49),
        0x33001E,
    },
    {
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x18),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x68),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x69),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x6A),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x35),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x6B),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x6E),
        DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x49),
        0x33001F,
    },
    {
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x18),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x68),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x69),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x6A),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x35),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x6B),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x6E),
        DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x49),
        0x330020,
    },
    {
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x18),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x68),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x69),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x6A),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x35),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x6B),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x6E),
        DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x49),
        0x330021,
    },
    {
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x17),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x18),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x68),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x69),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x6A),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x35),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x6B),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x6E),
        DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x49),
        0x330022,
    },
};
static const Vec playerFormationPoses[4][2] = {
    { { -372.0f, 300.0f, -220.0f }, { 0.0f, 50.0f, 0.0f } },
    { { 435.0f, 300.0f, -260.0f }, { 0.0f, 140.0f, 0.0f } },
    { { 315.0f, 300.0f, -153.0f }, { 0.0f, 140.0f, 0.0f } },
    { { 195.0f, 300.0f, -46.0f }, { 0.0f, 140.0f, 0.0f } },
};

M420DllPlayerStruct *CreatePlayer(s32 playerIndex, s32 formationIndex)
{
    M420DllPlayerStruct *stateValue;
    s32 spriteIndex;

    stateValue = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M420DllPlayerStruct), HU_MEMNUM_OVL);
    if (!stateValue) {
        OSReport("M420PlayerCreate ERROR...\n");
        return NULL;
    }
    stateValue->playerIndex = playerIndex;
    stateValue->formationIndex = formationIndex;
    stateValue->difficulty = GWPlayerCfg[playerIndex].diff;
    stateValue->isComputerControlled = GWPlayerCfg[playerIndex].iscom;
    stateValue->characterId = GWPlayerCfg[playerIndex].character;
    stateValue->controllerPort = GWPlayerCfg[playerIndex].pad_idx;
    stateValue->position = playerFormationPoses[formationIndex][0];
    stateValue->rotation = playerFormationPoses[formationIndex][1];
    stateValue->actionState = 0;
    stateValue->actionProgress = 0;
    stateValue->rotationTransitionActive = 0;
    stateValue->itemHookAttached = 0;
    stateValue->linkedPlayer = NULL;
    for (spriteIndex = 0; spriteIndex < 0xA; spriteIndex++) {
        stateValue->effectSpriteFrames[spriteIndex] = 7;
    }
    CreatePlayerModel(stateValue);
    LoadPlayerMotions(stateValue);

    switch (formationIndex) {
        case 0:
            if (stateValue->characterId == 5) {
                CharMotionSet(stateValue->characterId, stateValue->characterMotionIds[0xA]);
                Hu3DMotionStartEndSet(stateValue->characterModelId, 0.0f, 1.0f);
                Hu3DMotionTimeSet(stateValue->characterModelId, 0.0f);
            }
            else {
                CharMotionSet(stateValue->characterId, stateValue->characterMotionIds[0]);
            }
            break;
        case 1:
        case 2:
        case 3:
            stateValue->rotation.y = 0.0f;
            CharMotionSet(stateValue->characterId, stateValue->characterMotionIds[0]);
            break;
        default:
            break;
    }

    CharMotionDataClose(stateValue->characterId);
    CreatePlayerEffectSprites(stateValue);
    stateValue->playerProcess = HuPrcChildCreate(UpdatePlayerProcess, 0x64, 0x5000, 0, minigameObjectManager);
    stateValue->playerProcess->user_data = stateValue;
    return stateValue;
}

void DestroyPlayer(M420DllPlayerStruct *player)
{
    DestroyPlayerEffectSprites(player);
    DestroyPlayerMotions(player);
    DestroyPlayerModel(player);
    HuPrcKill(player->playerProcess);
    HuMemDirectFree(player);
}

void VibratePlayerController(M420DllPlayerStruct *player, s32 vibrationMode, s32 durationFrames)
{
    switch (vibrationMode) {
        case 0:
            omVibrate(player->playerIndex, durationFrames, 6, 6);
            break;
        case 1:
            omVibrate(player->playerIndex, durationFrames, 4, 2);
            break;
        case 2:
            omVibrate(player->playerIndex, durationFrames, 0xC, 0);
            break;
    }
}

M420DllPlayerActionFunc playerActionCallbacks[0xC] = {
    WaitForPlayerActionChange,
    HandlePlayerIdle,
    HandlePlayerCelebration,
    HandlePlayerAction,
    HandlePlayerWin,
    HandlePlayerMovement,
    WaitForPlayerActionChange,
    HandlePlayerItemSuccess,
    HandlePlayerItemFailure,
    HandlePlayerItemResult,
    HandlePlayerItemReturn,
    HandlePlayerItemAction,
};

void UpdatePlayerProcess(void)
{
    M420DllPlayerStruct *player = HuPrcCurrentGet()->user_data;
    while (TRUE) {
        playerActionCallbacks[player->actionState](player);
    }
}

void CreatePlayerModel(M420DllPlayerStruct *player)
{
    player->characterModelId = CharModelCreate(player->characterId, 2);
    Hu3DModelShadowSet(player->characterModelId);
}

void LoadPlayerMotions(M420DllPlayerStruct *player)
{
    s32 motionIndex;

    for (motionIndex = 0; motionIndex < 0xB; motionIndex++) {
        player->characterMotionIds[motionIndex] = CharMotionCreate(player->characterId, characterMotionDataIds[player->characterId][motionIndex]);
    }
}

void CreatePlayerEffectSprites(M420DllPlayerStruct *player)
{
    s32 spriteIndex;

    for (spriteIndex = 0; spriteIndex <= 9; spriteIndex++) {
        player->effectSpriteIds[spriteIndex] = espEntry(0x330018, 0, 0);
        espDispOff(player->effectSpriteIds[spriteIndex]);
    }
    switch (player->formationIndex) {
        case 0:
            player->statusSpriteId = espEntry(0x330019, 1, 0);
            break;
        case 1:
        case 2:
        case 3:
            player->statusSpriteId = espEntry(0x33001A, 1, 0);
            break;
        default:
            break;
    }
    espDispOff(player->statusSpriteId);
    espTPLvlSet(player->statusSpriteId, 0.8f);
}

void DestroyPlayerModel(M420DllPlayerStruct *player)
{
    CharModelKill(player->characterId);
}

void DestroyPlayerMotions(M420DllPlayerStruct *player)
{
    s32 motionIndex;

    for (motionIndex = 0; motionIndex < 0xB; motionIndex++) {
        CharMotionKill(player->characterId, player->characterMotionIds[motionIndex]);
    }
}

void DestroyPlayerEffectSprites(M420DllPlayerStruct *player)
{
    s32 spriteIndex;

    for (spriteIndex = 0; spriteIndex < 0xB; spriteIndex++) {
        espKill(player->effectSpriteIds[spriteIndex]);
    }
}

void AnimatePlayerRotation(M420DllPlayerStruct *player, Vec *targetRotation, s32 durationFrames, s32 easingMode)
{
    player->rotationTarget = *targetRotation;
    player->rotationStart = player->rotation;
    player->rotationProgress = 0.0f;
    player->rotationProgressStep = 1.0f / durationFrames;
    player->rotationEasingMode = easingMode;
    player->rotationTransitionActive = 1;
}

void WaitForPlayerActionChange(M420DllPlayerStruct *player)
{
    s32 playerIndex;
    s32 stateValue;

    while (TRUE) {
        for (playerIndex = 0; playerIndex < 1; playerIndex++) {
            UpdatePlayerModel(player);
            stateValue = player->actionState;
            HuPrcVSleep();
            if (player->actionState != stateValue) {
                return;
            }
        }
    }
}

#define WAITMODECHG(player, maxTime)                                                                                                                 \
    do {                                                                                                                                             \
        s32 time;                                                                                                                                    \
        s32 mode;                                                                                                                                    \
        for (time = 0; time < maxTime; time++) {                                                                                                     \
            UpdatePlayerModel(player);                                                                                                                         \
            mode = player->actionState;                                                                                                                     \
            HuPrcVSleep();                                                                                                                           \
            if (player->actionState != mode) {                                                                                                              \
                return;                                                                                                                              \
            }                                                                                                                                        \
        }                                                                                                                                            \
    } while (0)

void HandlePlayerIdle(M420DllPlayerStruct *player)
{
    float secondaryFrameProgress;
    float frameProgress;
    M420DllPlayerConfig *stateValue;
    s32 result;
    s32 playerIndex;
    s32 fourthStateValue;
    s32 tertiaryStateValue;
    s32 secondaryStateValue;
    s32 stateValue;

    stateValue = &playerConfigTable[player->difficulty];
    HandlePlayerEffect(player);
    CharMotionShiftSet(player->characterId, player->characterMotionIds[8], 0.0f, 10.0f, 0x40000001);
    player->actionProgress = 0;
    player->actionDelay = stateValue->initialWaitFrames + (getRandomInteger() % stateValue->initialWaitVariance);

    while (player->actionProgress < 6) {
        UpdatePlayerEffectSprites(player);
        do {
            if (player->isComputerControlled == 0) {
                result = GetPlayerInputState(player);
            }
            else if (player->actionDelay > 0) {
                player->actionDelay--;
                result = 0;
            }
            else if ((getRandomInteger() % 30) != 0) {
                result = 1;
                player->actionDelay = (stateValue->aiActionDelayFrames - (stateValue->aiActionDelayVariance / 2)) + (getRandomInteger() % stateValue->aiActionDelayVariance);
            }
            else {
                result = 2;
                player->actionDelay = (stateValue->aiActionDelayFrames - (stateValue->aiActionDelayVariance / 2)) + (getRandomInteger() % stateValue->aiActionDelayVariance);
            }
            switch (result) {
                case 0:
                    WAITMODECHG(player, 1);
                    break;
                case 1:
                    secondaryFrameProgress = 5.0f * player->actionProgress;
                    frameProgress = 5.0f + secondaryFrameProgress;
                    CharMotionSet(player->characterId, player->characterMotionIds[5]);
                    Hu3DMotionStartEndSet(player->characterModelId, secondaryFrameProgress, frameProgress);
                    Hu3DMotionTimeSet(player->characterModelId, secondaryFrameProgress);
                    Hu3DModelAttrReset(player->characterModelId, HU3D_MOTATTR_PAUSE);
                    Hu3DModelAttrReset(player->characterModelId, HU3D_MOTATTR_LOOP);
                    Hu3DModelAttrReset(player->characterModelId, HU3D_MOTATTR_REV);
                    player->activeEffectSprites[player->actionProgress++] = 1;
                    HuAudFXPlay(0x5F4);
                    AdvanceRoundCounter();
                    WAITMODECHG(player, 5);
                    break;
                case 2:
                    if (player->actionProgress <= 0) {
                        WAITMODECHG(player, 1);
                    }
                    else {
                        Hu3DModelAttrReset(player->characterModelId, HU3D_MOTATTR_PAUSE);
                        Hu3DModelAttrReset(player->characterModelId, HU3D_MOTATTR_LOOP);
                        Hu3DModelAttrSet(player->characterModelId, HU3D_MOTATTR_REV);
                        FinishRoundCounter();
                        while (!Hu3DMotionEndCheck(player->characterModelId))
                            WAITMODECHG(player, 1);
                        for (playerIndex = 0; playerIndex < 0xA; playerIndex++) {
                            player->activeEffectSprites[playerIndex] = 0;
                        }
                        player->actionProgress = 0;
                        secondaryFrameProgress = 0.0f;
                        frameProgress = 0.0f;
                        CharMotionSet(player->characterId, player->characterMotionIds[5]);
                        Hu3DMotionStartEndSet(player->characterModelId, secondaryFrameProgress, frameProgress);
                        Hu3DMotionTimeSet(player->characterModelId, secondaryFrameProgress);
                        Hu3DModelAttrReset(player->characterModelId, HU3D_MOTATTR_LOOP);
                    }
                    break;
                default:
                    break;
            }
        } while (result == 0);
    }
    UpdatePlayerEffectSprites(player);
    WAITMODECHG(player, 5);
    HidePlayerEffectSprites(player);
    player->actionState = 2;
}

void HandlePlayerAction(M420DllPlayerStruct *player)
{
    s32 selectedPlayerSlots[4];

    float secondaryFrameProgress;
    float frameProgress;

    s32 result;
    M420DllPlayerConfig *stateValue;
    s32 stateValue;
    s32 stateValue;
    s32 stateValue;
    s32 stateValue;
    s32 eighthStateValue;
    s32 seventhStateValue;
    s32 sixthStateValue;
    s32 fifthStateValue;
    s32 fourthStateValue;
    s32 tertiaryStateValue;
    s32 secondaryStateValue;
    s32 stateValue;

    stateValue = &playerConfigTable[player->difficulty];
    if (player->formationIndex == 1) {
        HuAudFXPlay(0x5F1);
        SetMapMarkerLighting(1);
        ShowMapMarker();
        WAITMODECHG(player, 15);
        CharMotionShiftSet(player->characterId, player->characterMotionIds[7], 0.0f, 6.0f, 0);
        WAITMODECHG(player, 6);
        SetMapMarkerLighting(0);
        AttachPlayerItemHook(player);
        HuAudFXPlay(0x5F2);
    }
    HandlePlayerEffect(player);
    player->actionProgress = 0;
    player->actionDelay = stateValue->eventDelayFrames + (getRandomInteger() % stateValue->eventDelayVariance);
    while (player->actionProgress < 1) {
        UpdatePlayerEffectSprites(player);
        do {
            if (player->isComputerControlled == 0) {
                result = GetPlayerInputState(player);
            }
            else if (player->actionDelay > 0) {
                player->actionDelay--;
                result = 0;
            }
            else if (getRandomInteger() % 30) {
                result = 1;
            }
            else {
                result = 2;
            }
            switch (result) {
                case 0:
                    WAITMODECHG(player, 1);
                    break;

                case 1:
                    if (player->formationIndex != 1) {
                        CharMotionShiftSet(player->characterId, player->characterMotionIds[7], 0.0f, 6.0f, 0);
                        WAITMODECHG(player, 6);
                        DetachPlayerItemHook(player->linkedPlayer);
                        AttachPlayerItemHook(player);
                        HuAudFXPlay(0x5F2);
                    }
                    player->activeEffectSprites[player->actionProgress++] = 1;
                    switch (player->formationIndex) {
                        case 1:
                        case 2:
                            secondaryFrameProgress = 0.0f;
                            frameProgress = 34.0f;
                            CharMotionSet(player->characterId, player->characterMotionIds[4]);
                            Hu3DMotionStartEndSet(player->characterModelId, secondaryFrameProgress, frameProgress);
                            Hu3DMotionTimeSet(player->characterModelId, secondaryFrameProgress);
                            Hu3DMotionSpeedSet(player->characterModelId, 2.0f);
                            break;

                        case 3:
                            player->actionState = 5;
                            return;
                    }
                    break;

                case 2:
                    switch (player->formationIndex) {
                        case 1:
                            secondaryFrameProgress = 0.0f;
                            frameProgress = 10.0f;
                            CharMotionSet(player->characterId, player->characterMotionIds[4]);
                            Hu3DMotionStartEndSet(player->characterModelId, secondaryFrameProgress, frameProgress);
                            Hu3DMotionTimeSet(player->characterModelId, secondaryFrameProgress);
                            Hu3DModelAttrReset(player->characterModelId, HU3D_MOTATTR_PAUSE);
                            WAITMODECHG(player, 30);
                            secondaryFrameProgress = 60.0f;
                            frameProgress = 70.0f;
                            CharMotionSet(player->characterId, player->characterMotionIds[4]);
                            Hu3DMotionStartEndSet(player->characterModelId, secondaryFrameProgress, frameProgress);
                            Hu3DMotionTimeSet(player->characterModelId, secondaryFrameProgress);
                            Hu3DModelAttrReset(player->characterModelId, HU3D_MOTATTR_PAUSE);
                            WAITMODECHG(player, 30);
                            break;

                        case 2:
                        case 3:
                            CharMotionShiftSet(player->characterId, player->characterMotionIds[9], 0.0f, 2.0f, 0);
                            WAITMODECHG(player, 60);
                            CharMotionShiftSet(player->characterId, player->characterMotionIds[0], 0.0f, 8.0f, 0);
                            break;
                    }
                    break;
            }
        } while (result == 0);
    }
    UpdatePlayerEffectSprites(player);
    WAITMODECHG(player, 18);
    HidePlayerEffectSprites(player);
    player->actionState = 6;
}

void HandlePlayerWin(M420DllPlayerStruct *player)
{
    CharMotionShiftSet(player->characterId, player->characterMotionIds[0], 0.0f, 30.0f, 0x40000001);

    WAITMODECHG(player, 35);
    player->actionState = 0;
}

void HandlePlayerCelebration(M420DllPlayerStruct *player)
{
    float secondaryFrameProgress;
    float frameProgress;
    s32 playerIndex;
    s32 stateValue;

    frameProgress = 0.0f;
    secondaryFrameProgress = 30.0f;
    Hu3DMotionStartEndSet(player->characterModelId, frameProgress, secondaryFrameProgress);
    Hu3DMotionTimeSet(player->characterModelId, secondaryFrameProgress);
    Hu3DModelAttrReset(player->characterModelId, HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(player->characterModelId, HU3D_MOTATTR_LOOP);
    Hu3DModelAttrSet(player->characterModelId, HU3D_MOTATTR_REV);
    HuAudFXPlay(0x5F6);
    ResetRoundCounter();
    AdvanceMapReveal(0, 22.5f);
    VibratePlayerController(player, 1, 0x1E);
    // wait_for_player_action_change?
    for (playerIndex = 0; playerIndex < 0x1E; playerIndex++) {
        UpdatePlayerModel(player);
        stateValue = player->actionState;
        HuPrcVSleep();
        if (player->actionState != stateValue) {
            return;
        }
    }
    player->actionState = 0;
}

void HandlePlayerMovement(M420DllPlayerStruct *player)
{
    s16 playerIndex;
    s32 stateValue;
    s32 stateValue;
    s32 stateValue;
    s32 stateValue;
    s32 stateValue;
    s32 stateValue;
    s32 stateValue;
    s32 eighthStateValue;

    UpdatePlayerEffectSprites(player);
    CharMotionShiftSet(player->characterId, player->characterMotionIds[3], 0.0f, 1.0f, 0);
    playerIndex = getMapMarkerModel();

    Hu3DModelAttrReset(playerIndex, HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrSet(playerIndex, HU3D_MOTATTR_LOOP);
    Hu3DMotionTimeSet(playerIndex, 0.0f);
    {
        s32 playerIndex;
        s32 stateValue;
        for (playerIndex = 0; playerIndex < 24; playerIndex++) {
            UpdatePlayerModel(player);
            stateValue = player->actionState;
            HuPrcVSleep();
            if (player->actionState != stateValue) {
                return;
            }
        }
    }
    HuAudFXPlay(0x5F7);
    ResetMapMotion();
    VibratePlayerController(player, 1, 0x1E);
    Hu3DModelAttrSet(playerIndex, HU3D_MOTATTR_PAUSE);
    AdvanceMapReveal(1, 22.5f);
    {
        s32 playerIndex;
        s32 stateValue;
        for (playerIndex = 0; playerIndex < 20; playerIndex++) {
            UpdatePlayerModel(player);
            stateValue = player->actionState;
            HuPrcVSleep();
            if (player->actionState != stateValue) {
                return;
            }
        }
    }
    HuAudFXPlay(0x5F8);
    HidePlayerEffectSprites(player);
    SpawnPlayerItem(player);
    {
        s32 playerIndex;
        s32 stateValue;
        for (playerIndex = 0; playerIndex < 20; playerIndex++) {
            UpdatePlayerModel(player);
            stateValue = player->actionState;
            HuPrcVSleep();
            if (player->actionState != stateValue) {
                return;
            }
        }
    }
    CharMotionShiftSet(player->characterId, player->characterMotionIds[0], 0.0f, 30.0f, 0x40000001);
    {
        s32 playerIndex;
        s32 stateValue;
        for (playerIndex = 0; playerIndex < 25; playerIndex++) {
            UpdatePlayerModel(player);
            stateValue = player->actionState;
            HuPrcVSleep();
            if (player->actionState != stateValue) {
                return;
            }
        }
    }
    player->actionState = 6;
}

void HandlePlayerEffect(M420DllPlayerStruct *player)
{
    s32 playerIndex;
    s32 spriteIndex;
    s32 result;

    for (playerIndex = 0; playerIndex < 0xA; playerIndex++) {
        player->activeEffectSprites[playerIndex] = 0;
    }
    player->actionProgress = 0;
    switch (player->formationIndex) {
        case 0:
            for (spriteIndex = 0; spriteIndex < 6;) {
                player->effectSpriteFrames[spriteIndex] = getRandomInteger() % 7;
                if (player->effectSpriteFrames[spriteIndex] == 6) {
                    player->effectSpriteFrames[spriteIndex] = 0;
                }
                if ((spriteIndex <= 0) || (player->effectSpriteFrames[spriteIndex] != player->effectSpriteFrames[spriteIndex - 1])) {
                    for (playerIndex = 0, result = 0; playerIndex < spriteIndex; playerIndex++) {
                        if (player->effectSpriteFrames[spriteIndex] == player->effectSpriteFrames[playerIndex]) {
                            result++;
                        }
                    }
                    if (result < 2) {
                        spriteIndex++;
                    }
                }
            }
            break;
        case 1:
        case 2:
        case 3:
            for (playerIndex = 0; playerIndex < 0xA; playerIndex++) {
                player->effectSpriteFrames[1] = player->effectSpriteFrames[0];
                player->effectSpriteFrames[0] = getRandomInteger() % 7;
                if (player->effectSpriteFrames[0] == 6) {
                    player->effectSpriteFrames[0] = 0;
                }
                if (player->effectSpriteFrames[0] != player->effectSpriteFrames[1]) {
                    break;
                }
            }
            break;
        default:
            break;
    }
}

static const s32 playerEffectSpriteCounts[4] = { 6, 1, 1, 1 };

void UpdatePlayerEffectSprites(M420DllPlayerStruct *player)
{
    Vec secondaryPositionVector;
    Vec positionVector;

    s32 spriteIndex;
    s16 tertiaryStateValue;
    s32 secondaryStateValue;
    s32 stateValue;

    secondaryPositionVector = player->position;
    secondaryPositionVector.y += 300.0f;
    Hu3D3Dto2D(&secondaryPositionVector, 1, &positionVector);

    stateValue = player->formationIndex == 0 ? -6 : 0;
    espPosSet(player->statusSpriteId, positionVector.x + stateValue, 6.0f + positionVector.y);
    espDispOn(player->statusSpriteId);
    secondaryStateValue = playerEffectSpriteCounts[player->formationIndex];
    positionVector.x -= (secondaryStateValue / 2) * 0x1E;
    for (spriteIndex = 0; spriteIndex < secondaryStateValue; spriteIndex++) {
        tertiaryStateValue = player->effectSpriteIds[spriteIndex];
        if (player->activeEffectSprites[spriteIndex] != 0) {
            espBankSet(tertiaryStateValue, player->effectSpriteFrames[spriteIndex] + 7);
        }
        else {
            espBankSet(tertiaryStateValue, player->effectSpriteFrames[spriteIndex]);
        }
        espPosSet(tertiaryStateValue, positionVector.x + spriteIndex * 0x20, positionVector.y);
        espDispOn(tertiaryStateValue);
    }
}

void HidePlayerEffectSprites(M420DllPlayerStruct *player)
{
    s32 spriteIndex;

    for (spriteIndex = 0; spriteIndex < 0xA; spriteIndex++) {
        espDispOff(player->effectSpriteIds[spriteIndex]);
    }
    espDispOff(player->statusSpriteId);
}

static const s32 controllerButtonMasks[7] = { 0x100, 0x200, 0x40, 0x20, 0x800, 0x400, 0x10 };

s32 GetPlayerInputState(M420DllPlayerStruct *player)
{
    s32 result;
    u16 secondaryStateValue;
    u16 stateValue;

    result = 0;
    secondaryStateValue = HuPadBtnDown[player->controllerPort] & 0xF60;
    stateValue = controllerButtonMasks[player->effectSpriteFrames[player->actionProgress]];
    if (secondaryStateValue != 0) {
        result = 2;
    }
    if (((secondaryStateValue & stateValue) != 0) && ((secondaryStateValue & ~stateValue) == 0)) {
        result = 1;
    }
    return result;
}

void UpdatePlayerModel(M420DllPlayerStruct *player)
{
    float interpolationFactor;

    player = HuPrcCurrentGet()->user_data;
    Hu3DModelPosSet(player->characterModelId, player->position.x, player->position.y, player->position.z);
    if (player->rotationTransitionActive) {
        switch (player->rotationEasingMode) {
            case 0:
                interpolationFactor = player->rotationProgress;
                break;
            case 1:
                interpolationFactor = sind((90.0f * player->rotationProgress));
                interpolationFactor *= interpolationFactor;
                break;
        }
        player->rotation.x = player->rotationStart.x + (interpolationFactor * (player->rotationTarget.x - player->rotationStart.x));
        player->rotation.y = player->rotationStart.y + (interpolationFactor * (player->rotationTarget.y - player->rotationStart.y));
        player->rotation.z = player->rotationStart.z + (interpolationFactor * (player->rotationTarget.z - player->rotationStart.z));
        if ((player->rotationProgress += player->rotationProgressStep) >= 1.0f) {
            player->rotation = player->rotationTarget;
            player->rotationTransitionActive = 0;
        }
    }
    Hu3DModelRotSet(player->characterModelId, player->rotation.x, player->rotation.y, player->rotation.z);
}

void AttachPlayerItemHook(M420DllPlayerStruct *player)
{
    Hu3DModelHookSet(player->characterModelId, "test11_tex_we-itemhook-r", getMapMarkerModel());
    player->itemHookAttached = 1;
    VibratePlayerController(player, 1, 0x14);
}

void DetachPlayerItemHook(M420DllPlayerStruct *player)
{
    Hu3DModelHookReset(player->characterModelId);
    player->itemHookAttached = 0;
}

void SpawnPlayerItem(M420DllPlayerStruct *player)
{
    Mtx modelMatrix;
    Vec secondaryPositionVector;
    Vec positionVector;

    if (player->itemHookAttached) {
        Hu3DModelObjMtxGet(player->characterModelId, "test11_tex_we-itemhook-r", modelMatrix);
        Hu3DModelHookObjReset(player->characterModelId, "test11_tex_we-itemhook-r");
        Hu3DMtxTransGet(modelMatrix, &secondaryPositionVector);
        Hu3DMtxRotGet(modelMatrix, &positionVector);
        Hu3DModelPosSetV(getMapMarkerModel(), &secondaryPositionVector);
        Hu3DModelRotSetV(getMapMarkerModel(), &positionVector);
    }
    HuPrcChildCreate(AnimatePlayerItemSpawn, 0x64, 0x2000, 0, minigameObjectManager);
    player->itemHookAttached = 0;
}

char emptyModelObjectName[1] = { 0 };

void AnimatePlayerItemSpawn(void)
{
    Mtx modelMatrix;
    Vec secondaryTransformVector;
    Vec secondaryTransformVector;
    Vec transformVector;

    float interpolationFactor;
    s16 stateValue;

    stateValue = getMapMarkerModel();
    Hu3DModelObjMtxGet(stateValue, emptyModelObjectName, modelMatrix);
    Hu3DMtxRotGet(modelMatrix, &transformVector);
    Hu3DMtxTransGet(modelMatrix, &secondaryTransformVector);
    interpolationFactor = 0.0f;

    while (interpolationFactor < 1.0f) {
        secondaryTransformVector.x = cosd((90.0f * interpolationFactor));
        secondaryTransformVector.y = secondaryTransformVector.z = secondaryTransformVector.x;
        Hu3DModelScaleSet(stateValue, secondaryTransformVector.x, secondaryTransformVector.y, secondaryTransformVector.z);
        interpolationFactor += 0.05f;
        HuPrcVSleep();
    }
    HideMapMarker();
    secondaryTransformVector.x = secondaryTransformVector.y = secondaryTransformVector.z = 1.0f;
    Hu3DModelScaleSet(stateValue, secondaryTransformVector.x, secondaryTransformVector.y, secondaryTransformVector.z);
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

void HandlePlayerItemAction(M420DllPlayerStruct *player)
{
    Vec transformVector;

    s32 secondaryPlayerIndex;
    s32 playerIndex;
    s32 secondaryStateValue;
    s32 stateValue;

    if (player->itemHookAttached) {
        SpawnPlayerItem(player);
    }
    else if (player->formationIndex == 1) {
        SpawnPlayerItem(player);
    }
    HidePlayerEffectSprites(player);
    if (player->formationIndex == 0) {
        Hu3DModelAttrReset(player->characterModelId, HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(player->characterModelId, HU3D_MOTATTR_LOOP);
        Hu3DModelAttrSet(player->characterModelId, HU3D_MOTATTR_REV);
        FinishRoundCounter();
        CharMotionShiftSet(player->characterId, player->characterMotionIds[6], 0.0f, 32.0f, 0);
        Hu3DMotionShiftStartEndSet(player->characterModelId, 0.0f, 59.0f);
        transformVector = player->rotation;
        transformVector.y = 0.0f;
        AnimatePlayerRotation(player, &transformVector, 60, 1);
    }
    for (secondaryPlayerIndex = 0; secondaryPlayerIndex < 0x3C; secondaryPlayerIndex++) {
        UpdatePlayerModel(player);
        secondaryStateValue = player->actionState;
        HuPrcVSleep();
        if (player->actionState != secondaryStateValue) {
            return;
        }
    }
    CharMotionShiftSet(player->characterId, player->characterMotionIds[0], 0.0f, 16.0f, 0);
    for (playerIndex = 0; playerIndex < 0x3C; playerIndex++) {
        UpdatePlayerModel(player);
        stateValue = player->actionState;
        HuPrcVSleep();
        if (player->actionState != stateValue) {
            return;
        }
    }
    player->actionState = 0;
}

void HandlePlayerItemResult(M420DllPlayerStruct *player)
{
    Vec transformVector;
    s32 stateValue;
    s32 playerIndex;

    if (player->formationIndex != 0) {
        CharMotionSet(player->characterId, player->characterMotionIds[6]);
        Hu3DMotionStartEndSet(player->characterModelId, 60.0f, 119.0f);
        Hu3DMotionTimeSet(player->characterModelId, 60.0f);
        transformVector = player->rotation;
        transformVector.y = playerFormationPoses[player->formationIndex][1].y;
        AnimatePlayerRotation(player, &transformVector, 60, 1);
        while (player->rotationTransitionActive != 0) {
            for (playerIndex = 0; playerIndex < 1; playerIndex++) {
                UpdatePlayerModel(player);
                stateValue = player->actionState;
                HuPrcVSleep();
                if (player->actionState != stateValue) {
                    return;
                }
            }
        }
        CharMotionShiftSet(player->characterId, player->characterMotionIds[0], 0.0f, 4.0f, 0);
    }
    player->actionState = 0;
}

void HandlePlayerItemReturn(M420DllPlayerStruct *player)
{
    Vec transformVector;
    s32 stateValue;
    s32 playerIndex;

    if (player->formationIndex != 0) {
        CharMotionSet(player->characterId, player->characterMotionIds[6]);
        Hu3DMotionStartEndSet(player->characterModelId, 0.0f, 59.0f);
        transformVector = player->rotation;
        transformVector.y = 0.0f;
        AnimatePlayerRotation(player, &transformVector, 60, 1);
        while (player->rotationTransitionActive != 0) {
            for (playerIndex = 0; playerIndex < 1; playerIndex++) {
                UpdatePlayerModel(player);
                stateValue = player->actionState;
                HuPrcVSleep();
                if (player->actionState != stateValue) {
                    return;
                }
            }
        }
        CharMotionShiftSet(player->characterId, player->characterMotionIds[0], 0.0f, 8.0f, 0);
    }
    player->actionState = 0;
}

void HandlePlayerItemSuccess(M420DllPlayerStruct *player)
{
    float frameProgress;
    M420DllPlayerStruct *player;
    s32 secondaryStateValue;
    s32 stateValue;

    CharMotionShiftSet(player->characterId, player->characterMotionIds[1], 0.0f, 4.0f, 0);
    WaitForPlayerActionChange(player);
}

void HandlePlayerItemFailure(M420DllPlayerStruct *player)
{
    float frameProgress;
    M420DllPlayerStruct *player;
    s32 secondaryStateValue;
    s32 stateValue;

    CharMotionShiftSet(player->characterId, player->characterMotionIds[2], 0.0f, 4.0f, 0);
    WaitForPlayerActionChange(player);
}

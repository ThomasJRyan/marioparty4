#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/memory.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/wipe.h"
#include "string.h"

#include "REL/m456Dll.h"
#include "version.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/frand.h"
#endif

typedef struct M456DllMainWork {
    struct {
        u8 actionComplete : 1;
        u8 actionSoundPlayed : 1;
        u8 actionFinished : 1;
        u8 actionWasRepeated : 1;
    };
    s16 characterIndex;
    s16 controllerPort;
    s8 stickX;
    s8 stickY;
    u16 buttonsHeld;
    u16 buttonsPressed;
    s32 selectedInput;
    s32 motionIndex;
    s16 characterId;
    s16 motionVariant;
    s16 playerGroup;
    s16 actionState;
    char reserved1C[4];
    s32 currentMotion;
    s32 motionFlags;
    s16 actionTimer;
    s16 actionDuration;
    s16 repeatTimer;
    s16 repeatInterval;
    s16 repeatCount;
} M456DllMainWork; /* size = 0x34 */

typedef struct CameraSettings {
    /* 0x00 */ float cameraZoom;
    /* 0x04 */ float centerX;
    /* 0x08 */ float centerY;
    /* 0x0C */ float centerZ;
    /* 0x10 */ float rotationX;
    /* 0x14 */ float rotationY;
    /* 0x18 */ float rotationZ;
} CameraSettings; /* size = 0x1C */

typedef struct CountdownDigitSprite {
    /* 0x00 */ u32 spriteData;
    /* 0x04 */ s32 spriteLayer;
} CountdownDigitSprite; /* size = 0x08 */

typedef struct CharacterMotionConfig {
    /* 0x00 */ s32 motionOffset;
    /* 0x04 */ u32 usesMotionOffset;
} CharacterMotionConfig; /* size = 0x08 */

typedef struct CountdownDisplayWork {
    s32 isVisible;
    s32 isPulsing;
    s32 pulseDirection;
    float originX;
    float originY;
    s32 remainingTime;
    s32 elapsedTime;
    float scale;
    s16 digitSprites[0x10];
} CountdownDisplayWork; /* size 0x40 */

void UpdatePlayerCharacter(OMOBJ *object);
void DestroyPlayerWork(OMOBJ *object);
float ApproachAngle(float currentAngle, float targetAngle, float interpolationWeight);
void CreateCountdownCamera(HUPROCESS *process);
void InterpolateCameraSettings(s32 startSettingIndex, s32 endSettingIndex, float interpolationWeight);
void UpdateCountdownCamera(OMOBJ *object);
void CreateTimerObject(HUPROCESS *process);
void UpdateTimerObject(OMOBJ *object);
void InitializeGameSequence(OMOBJ *object);
void UpdateGameSequence(OMOBJ *);
void FinishGameSequence(OMOBJ *);
void CreatePlayerObjects(HUPROCESS *process);
void UpdatePlayerObjects(void);
s32 FindWinningPlayers(void);
s32 AllPlayerActionsFinished(void);
void InitializePlayerObject(OMOBJ *object);

CameraSettings cameraSettings[2] = {
    { 500.0f, 0.0f, 1000.0f, 0.0f, 40.0f, 0.0f, 0.0f },
    { 1985.0f, 0.0f, 100.0f, 100.0f, -13.0f, 0.0f, 0.0f },
};
Vec keyLightPosition = { 700.0f, 2500.0f, -700.0f };
Vec originPosition = { 0.0f, 0.0f, 0.0f };
GXColor keyLightColor = { 0xFF, 0xFF, 0xFF, 0xFF };
Vec shadowPosition = { 200.0f, 15000.0f, -200.0f };
Vec shadowUpVector = { 0.0f, 1.0f, 0.0f };

OMOBJ *countdownObject;
OMOBJ *clearScreenObject;
OMOBJ *gameSequenceObject;
s32 gameSequenceState;
s32 winningPlayerIndices[4];
s32 stateDelayFrames;
s32 playerCount;
s32 winnerCount;
s16 remainingCountdownTicks;
s16 sequenceWaitFrames;
u32 previousRecordFrames;
u32 currentRecordFrames;
CountdownDisplayWork countdownDisplayWork;
u32 recordDurationFrames;
Vec keyLightDirection;
s16 activeMessageSequence;
s16 resultMessageSequence;
s32 unusedGlobalState;

void ObjectSetup(void)
{
    HUPROCESS *objectManager;
    HU3DCAMERA *camera;
    s16 lightId;

    camera = Hu3DCamera;
    OSReport("******* M456ObjectSetup *********\n");
    objectManager = omInitObjMan(0x32, 0x2000);
    omGameSysInit(objectManager);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, -1.0f, 20.0f, 25000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    clearScreenObject = omAddObjEx(objectManager, 0x7FDA, 0, 0, -1, omOutView);
    keyLightDirection.x = originPosition.x - keyLightPosition.x;
    keyLightDirection.y = originPosition.y - keyLightPosition.y;
    keyLightDirection.z = originPosition.z - keyLightPosition.z;
    VECNormalize(&keyLightDirection, &keyLightDirection);
    lightId = Hu3DGLightCreateV(&keyLightPosition, &keyLightDirection, &keyLightColor);
    Hu3DGLightInfinitytSet(lightId);
    Hu3DGLightStaticSet(lightId, 1);
    Hu3DShadowCreate(10.0f, camera->near, camera->far);
    Hu3DShadowTPLvlSet(0.7f);
    Hu3DShadowPosSet(&shadowPosition, &shadowUpVector, &originPosition);
    HuAudSndGrpSet(0x4E);
    gameSequenceObject = omAddObjEx(objectManager, 0, 0, 0, -1, InitializeGameSequence);
    CreateCountdownCamera(objectManager);
    CreatePlayerObjects(objectManager);
    InitializeStageObjects(objectManager);
    previousRecordFrames = GWMGRecordGet(0xA);
    if (previousRecordFrames == 0) {
        previousRecordFrames = 5 * REFRESH_RATE;
    }
    currentRecordFrames = 0;
    CreateTimerObject(objectManager);
    countdownDisplayWork.remainingTime = previousRecordFrames;
    countdownDisplayWork.isVisible = 1;
    countdownDisplayWork.originX = 536.0f;
}

s32 endingMusicSequence = -1;
s32 soundEffectHandle = -1;

void SetGameSequenceState(u32 sequenceState)
{
    gameSequenceState = sequenceState;
}

s32 GetGameSequenceState(void)
{
    return gameSequenceState;
}

void InitializeGameSequence(OMOBJ *object)
{
    HU3DCAMERA *camera = Hu3DCamera;
    SetGameSequenceState(0);
    stateDelayFrames = REFRESH_RATE;
    winningPlayerIndices[0] = winningPlayerIndices[1] = winningPlayerIndices[2] = winningPlayerIndices[3] = -1;
    remainingCountdownTicks = 5;
    sequenceWaitFrames = REFRESH_RATE;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 0x3C);
    camera->fov = 30.0f;
    object->objFunc = UpdateGameSequence;
}

void UpdateGameSequence(OMOBJ *object)
{
    s16 winningCharacterIds[4];

    s32 playerIndex;
    OMOBJ **playerObjects;
    M456DllMainWork *playerWork;

    switch (GetGameSequenceState()) {
        case 0:
            if (soundEffectHandle < 0) {
                soundEffectHandle = HuAudFXPlay(0x7D1);
            }
            if (--stateDelayFrames == 0) {
                SetGameSequenceState(1);
                stateDelayFrames = 2 * REFRESH_RATE;
            }
            break;
        case 1:
            if (--stateDelayFrames == 0) {
                SetGameSequenceState(2);
                activeMessageSequence = MGSeqCreate(3, 0);
            }
            break;
        case 2:
            if (MGSeqStatGet(activeMessageSequence) == 0) {
                SetGameSequenceState(3);
                resultMessageSequence = MGSeqCreate(1, 5, -1, -1);
            }
            if ((endingMusicSequence < 0) && ((MGSeqStatGet(activeMessageSequence) & 0x10) != 0)) {
                endingMusicSequence = HuAudSeqPlay(0x43);
            }
            break;
        case 3:
            if (--sequenceWaitFrames == 0) {
                remainingCountdownTicks--;
                MGSeqParamSet(resultMessageSequence, 1, remainingCountdownTicks);
                sequenceWaitFrames = REFRESH_RATE;
            }
            if (remainingCountdownTicks == 0) {
                winnerCount = FindWinningPlayers();
                if (winnerCount == 0) {
                    SetGameSequenceState(5);
                    activeMessageSequence = MGSeqCreate(3, 1);
                    HuAudSeqFadeOut(endingMusicSequence, 0x64);
                }
                else {
                    SetGameSequenceState(4);
                    HuAudFXPlay(0x11);
                    stateDelayFrames = 2 * REFRESH_RATE;
                }
            }
            break;
        case 4:
            if (resultMessageSequence >= 0) {
                MGSeqParamSet(resultMessageSequence, 2, -1);
                resultMessageSequence = -1;
            }
            if (stateDelayFrames != 0) {
                stateDelayFrames--;
            }
            else if (countdownDisplayWork.elapsedTime < recordDurationFrames) {
                countdownDisplayWork.elapsedTime++;
            }
            if (AllPlayerActionsFinished() != 0) {
                SetGameSequenceState(5);
                activeMessageSequence = MGSeqCreate(3, 1);
                HuAudSeqFadeOut(endingMusicSequence, 0x64);
            }
            break;
        case 5:
            if (resultMessageSequence >= 0) {
                MGSeqParamSet(resultMessageSequence, 2, -1);
                resultMessageSequence = -1;
            }
            if (MGSeqStatGet(activeMessageSequence) == 0) {
                if (currentRecordFrames == 0) {
                    SetGameSequenceState(9);
                    stateDelayFrames = 2 * REFRESH_RATE;
                }
                else {
                    SetGameSequenceState(6);
                    activeMessageSequence = MGSeqCreate(0xE, currentRecordFrames);
                    GWMGRecordSet(0xA, currentRecordFrames);
                    countdownDisplayWork.isPulsing = 1;
                    countdownDisplayWork.remainingTime = countdownDisplayWork.elapsedTime;
                }
            }
            break;
        case 6:
            if (MGSeqStatGet(activeMessageSequence) == 0) {
                SetGameSequenceState(9);
                stateDelayFrames = 2 * REFRESH_RATE;
            }
            break;
        case 9:
            if (stateDelayFrames != 0) {
                stateDelayFrames--;
            }
            if (stateDelayFrames == 0) {
                if (AllPlayerActionsFinished() != 0) {
                    SetGameSequenceState(0xB);
                    if (winnerCount != 0) {
                        playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
                        for (playerIndex = 0; playerIndex < winnerCount; playerIndex++) {
                            playerWork = playerObjects[winningPlayerIndices[playerIndex]]->data;
                            winningCharacterIds[playerIndex] = playerWork->characterId;
                            GWPlayerCoinWinSet(winningPlayerIndices[playerIndex], 10);
                        }
                        for (; playerIndex < 4; playerIndex++) {
                            winningCharacterIds[playerIndex] = -1;
                        }
                        activeMessageSequence = MGSeqCreate(5, 3, winningCharacterIds[0], winningCharacterIds[1], winningCharacterIds[2], winningCharacterIds[3]);
                        HuAudSStreamPlay(1);
                    }
                    else {
                        activeMessageSequence = MGSeqCreate(3, 2);
                        HuAudSStreamPlay(4);
                    }
                    stateDelayFrames = 3.5 * REFRESH_RATE;
                }
            }
            break;
        case 11:
            if (--stateDelayFrames == 0) {
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 0x3C);
                HuAudFXStop(soundEffectHandle);
                object->objFunc = FinishGameSequence;
            }
            break;
    }
    if ((omSysExitReq != 0) && (WipeStatGet() == 0)) {
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 0x3C);
        object->objFunc = FinishGameSequence;
    }
}

void FinishGameSequence(OMOBJ *object)
{
    if (WipeStatGet() == 0) {
        UpdatePlayerObjects();
        DestroyStageObjects();
        MGSeqKillAll();
        HuAudFadeOut(1);
        omOvlReturnEx(1, 1);
        OSReport("******* M456Exit *********\n");
    }
}

void CreatePlayerObjects(HUPROCESS *process)
{
    s32 playerIndex;

    playerCount = 0;
    omMakeGroupEx(process, 0, 4);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        omAddObjEx(process, 5, 0xA, 0x32, 0, InitializePlayerObject);
    }
}

void UpdatePlayerObjects(void)
{
    s32 playerIndex;
    OMOBJ **playerObjects;

    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    if (playerObjects != NULL) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            DestroyPlayerWork(playerObjects[playerIndex]);
        }
    }
}

s32 FindWinningPlayers(void)
{
    s32 playerIndices[4];

    s32 playerIndex;
    M456DllMainWork *playerWork;
    s32 winnerCount;
    u32 longestStreak;
    OMOBJ **playerObjects;

    winnerCount = 0;
    longestStreak = 0;
    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerWork = playerObjects[playerIndex]->data;
        if ((playerWork->repeatCount != 0) && (playerWork->repeatCount < 5)) {
            playerWork->repeatCount = 5;
        }
        if (playerWork->repeatCount > longestStreak) {
            longestStreak = playerWork->repeatCount;
            winnerCount = 0;
            playerIndices[winnerCount++] = playerIndex;
        }
        else if (playerWork->repeatCount == longestStreak) {
            playerIndices[winnerCount++] = playerIndex;
        }
    }
    if (longestStreak == 0) {
        winningPlayerIndices[playerIndex] = -1;
        return 0;
    }
    recordDurationFrames = (longestStreak * REFRESH_RATE) / 5;
    for (playerIndex = 0; playerIndex < winnerCount; playerIndex++) {
        if (GWPlayerCfg[playerIndices[playerIndex]].iscom == 0) {
            break;
        }
    }
    if (playerIndex < winnerCount) {
        if (recordDurationFrames > previousRecordFrames) {
            currentRecordFrames = recordDurationFrames;
        }
        if (recordDurationFrames >= 7.5f * REFRESH_RATE) {
            GWGameStat.present[0x33] = 1;
        }
    }
    for (playerIndex = 0; playerIndex < winnerCount; playerIndex++) {
        winningPlayerIndices[playerIndex] = playerIndices[playerIndex];
    }
    return winnerCount;
}

s32 AllPlayerActionsFinished(void)
{
    s32 playerIndex;
    OMOBJ **playerObjects;
    M456DllMainWork *playerWork;

    playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerWork = playerObjects[playerIndex]->data;
        if (playerWork->actionFinished == 0) {
            return 0;
        }
    }
    return 1;
}

CharacterMotionConfig characterMotionConfigs[0xA] = {
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00), 0 },
    { DATA_MAKE_NUM(DATADIR_M456, 0x1C), 1 },
    { DATA_MAKE_NUM(DATADIR_M456, 0x24), 1 },
    { DATA_MAKE_NUM(DATADIR_M456, 0x2C), 1 },
    { DATA_MAKE_NUM(DATADIR_M456, 0x34), 1 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x2A), 0 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x05), 0 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17), 0 },
    { DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18), 0 },
    { DATA_MAKE_NUM(DATADIR_M456, 0x3C), 1 },
};
Vec playerStartPositions[4] = {
    { -300.0f, 0.0f, 0.0f },
    { -100.0f, 0.0f, 0.0f },
    { 100.0f, 0.0f, 0.0f },
    { 300.0f, 0.0f, 0.0f },
};
float animationOffsets[8] = { -50.0f, -50.0f, -50.0f, -50.0f, -50.0f, -50.0f, -50.0f, -50.0f };
u16 cpuInputButtons[2] = { 0x40, 0x20 };

void InitializePlayerObject(OMOBJ *object)
{
    M456DllMainWork *playerWork;
    u32 motionIndex;
    s32 characterId;
    s32 rumbleSetting;

    playerWork = HuMemDirectMallocNum(HEAP_HEAP, sizeof(M456DllMainWork), HU_MEMNUM_OVL);
    object->data = playerWork;
    memset(playerWork, 0, sizeof(M456DllMainWork));
    playerWork->characterIndex = playerCount++;
    playerWork->controllerPort = GWPlayerCfg[playerWork->characterIndex].pad_idx;
    rumbleSetting = GWGameStat.rumble;
    playerWork->motionIndex = rumbleSetting;
    characterId = GWPlayerCfg[playerWork->characterIndex].character;
    playerWork->characterId = characterId;
    playerWork->motionVariant = GWPlayerCfg[playerWork->characterIndex].diff;
    playerWork->playerGroup = GWPlayerCfg[playerWork->characterIndex].group;
    playerWork->selectedInput = -1;
    object->mdlId[0] = CharModelCreate(characterId, 8);
    Hu3DModelShadowSet(object->mdlId[0]);
    Hu3DModelLayerSet(object->mdlId[0], 3);
    Hu3DModelCameraSet(object->mdlId[0], 1);
    playerWork->currentMotion = 0;
    for (motionIndex = 0; motionIndex < 0xA; motionIndex++) {
        if (characterMotionConfigs[motionIndex].usesMotionOffset == 0) {
            object->mtnId[motionIndex] = CharMotionCreate(playerWork->characterId, characterMotionConfigs[motionIndex].motionOffset);
        }
        else {
            object->mtnId[motionIndex] = CharMotionCreate(playerWork->characterId, playerWork->characterId + characterMotionConfigs[motionIndex].motionOffset);
        }
    }
    Hu3DMotionSet(object->mdlId[0], object->mtnId[playerWork->currentMotion]);
    Hu3DModelAttrSet(object->mdlId[0], HU3D_MOTATTR_LOOP);
    CharMotionVoiceOnSet(playerWork->characterId, object->mtnId[5], 0);
    CharMotionVoiceOnSet(playerWork->characterId, object->mtnId[6], 0);
    playerWork->repeatInterval = REFRESH_RATE;
    object->trans.x = playerStartPositions[playerWork->characterIndex].x;
    object->trans.y = playerStartPositions[playerWork->characterIndex].y;
    object->trans.z = playerStartPositions[playerWork->characterIndex].z;
    object->rot.y = 0.0f;
    CharModelDataClose(playerWork->characterId);
    object->objFunc = UpdatePlayerCharacter;
}

#if VERSION_PAL
#define generateCpuInput_COND (((playerWork->motionVariant + 4) * 60) / 50.0f)
#else
#define generateCpuInput_COND (playerWork->motionVariant + 4)
#endif

void GenerateCpuInput(OMOBJ *object)
{
    M456DllMainWork *playerWork = object->data;

    if ((frand() & 0x3F) < generateCpuInput_COND) {
        if (playerWork->selectedInput <= 0) {
            playerWork->buttonsPressed |= 0x40;
            return;
        }
        playerWork->buttonsPressed |= 0x20;
    }
}

void UpdatePlayerCharacter(OMOBJ *playerObject)
{
    Vec cameraPosition;
    Vec targetPosition;
    float motionProgress;

    M456DllMainWork *playerWork;
    s32 currentMotion;
    u32 motionFlags;
    void *groupIndex;
    s32 playerIndex;
    OMOBJ **playerObjects;
    s32 motionIndex;

    playerWork = playerObject->data;
    currentMotion = playerWork->currentMotion;
    motionFlags = playerWork->motionFlags;
    if (playerWork->actionComplete == 0) {
        playerObject->trans.y = GetStageHeight();
    }
    if (GetGameSequenceState() < 3) {
        return;
    }
    switch (playerWork->actionState) {
        case 0:
            if (GetGameSequenceState() == 5) {
                playerWork->actionFinished = 1;
                playerWork->actionState = 6;
                return;
            }
            if (GetGameSequenceState() == 4) {
                if (playerWork->repeatCount != 0) {
                    playerWork->actionState = 1;
                    playerWork->actionTimer = REFRESH_RATE;
                }
                else {
                    playerWork->actionState = 6;
                    playerWork->actionFinished = 1;
                }
                return;
            }
            if (GWPlayerCfg[playerWork->characterIndex].iscom == 0) {
                playerWork->stickX = HuPadStkX[playerWork->controllerPort];
                playerWork->stickY = HuPadStkY[playerWork->controllerPort];
                playerWork->buttonsHeld = HuPadBtn[playerWork->controllerPort];
                playerWork->buttonsPressed = HuPadBtnDown[playerWork->controllerPort] & 0xFFFF9FFF;
            }
            else {
                playerWork->buttonsHeld = playerWork->buttonsPressed = 0;
                playerWork->stickX = playerWork->stickY = 0;
                GenerateCpuInput(playerObject);
            }
            if (playerWork->repeatTimer != 0) {
                if (--playerWork->repeatTimer == 0) {
                    currentMotion = 0;
                    motionFlags = 0x40000001;
                }
            }
            if ((playerWork->buttonsPressed & 0x60) != 0) {
                if (playerWork->selectedInput < 0) {
                    playerWork->selectedInput = (playerWork->buttonsPressed & 0x40) ? 0 : 1;
                }
                if (((playerWork->buttonsPressed & cpuInputButtons[playerWork->selectedInput]) != 0) && ((playerWork->buttonsPressed & ~cpuInputButtons[playerWork->selectedInput]) == 0)) {
                    playerWork->repeatCount++;
                    playerWork->repeatTimer = playerWork->repeatInterval;
                    currentMotion = 1;
                    motionFlags = 0x40000001;
                    playerWork->selectedInput ^= 1;
                }
            }
            break;
        case 1:
            if (--playerWork->actionTimer == 0) {
                playerWork->actionComplete = 1;
                playerWork->actionState = 2;
                playerWork->actionTimer = playerWork->actionDuration = REFRESH_RATE;
            }
            currentMotion = 2;
            motionFlags = 0;
            break;
        case 2:
            currentMotion = 3;
            motionFlags = 0;
            if (playerWork->actionTimer < (playerWork->actionDuration - 0xF)) {
                motionProgress = (float)playerWork->actionTimer / (playerWork->actionDuration - 0xF);
                playerObject->trans.y = -300.0 * cosd((270.0f * motionProgress));
                playerObject->trans.z = 400.0f + (-400.0f * motionProgress);
                if ((playerWork->actionSoundPlayed == 0) && (motionProgress < 0.35f)) {
                    playerWork->actionSoundPlayed = 1;
                    CreateWaterEffect(&playerObject->trans, -1.0f);
                    CreateSprinkleEffect(&playerObject->trans, -1.0f, -1.0f, -1);
                    SendWhalesOffscreen();
                    HuAudFXPlay(0x7D2);
                }
            }
            if (--playerWork->actionTimer == 0) {
                playerObject->trans.y = -300.0f;
                playerObject->trans.z = 400.0f;
                playerWork->actionState = 3;
                playerWork->actionTimer = (playerWork->repeatCount * REFRESH_RATE) / 5;
                for (playerIndex = 0; playerIndex < winnerCount; playerIndex++) {
                    if (playerWork->characterIndex == winningPlayerIndices[playerIndex]) {
                        break;
                    }
                }
                if (playerIndex < winnerCount) {
                    playerWork->actionWasRepeated = 1;
                    playerWork->actionTimer -= 0.33f*REFRESH_RATE;
                }
                else {
                    playerWork->actionTimer -= 0.8f * REFRESH_RATE;
                }
                playerWork->actionDuration = playerWork->actionTimer;
            }
            break;
        case 3:
            currentMotion = 5;
            motionFlags = 0x40000001;
            if (playerWork->actionTimer < 1.0f * REFRESH_RATE) {
                if ((frand() & 3) == 0) {
                    cameraPosition.x = playerObject->trans.x;
                    cameraPosition.y = 100.0f + playerObject->trans.y;
                    cameraPosition.z = playerObject->trans.z;
                    CreateSprinkleObject(&cameraPosition, -1.0f);
                }
            }
            else if ((frand() & 0xF) == 0) {
                cameraPosition.x = playerObject->trans.x;
                cameraPosition.y = 100.0f + playerObject->trans.y;
                cameraPosition.z = playerObject->trans.z;
                CreateSprinkleObject(&cameraPosition, -1.0f);
            }
            if (--playerWork->actionTimer == 0) {
                playerWork->actionSoundPlayed = 0;
                if (playerWork->actionWasRepeated == 0) {
                    playerWork->actionState = 4;
                    playerWork->actionTimer = playerWork->actionDuration = REFRESH_RATE / 1.25;
                }
                else {
                    playerWork->actionState = 5;
                    playerWork->actionTimer = playerWork->actionDuration = REFRESH_RATE;
                }
            }
            break;
        case 4:
            currentMotion = 4;
            motionFlags = 0x40000001;
            motionProgress = (float)playerWork->actionTimer / playerWork->actionDuration;
            playerObject->trans.y = -300.0 * sind((90.0f * motionProgress));
            if ((frand() & 3) == 0) {
                CreateSprinkleObject(&playerObject->trans, -1.0f);
            }
            if ((playerWork->actionSoundPlayed == 0) && (motionProgress < 0.2f)) {
                playerWork->actionSoundPlayed = 1;
                CreateWaterEffect(&playerObject->trans, -1.0f);
                CreateSprinkleEffect(&playerObject->trans, -1.0f, -1.0f, -1);
                HuAudFXPlay(0x7D3);
            }
            if (--playerWork->actionTimer == 0) {
                playerWork->actionFinished = 1;
                playerWork->actionState = 6;
                playerObject->trans.y = animationOffsets[playerWork->characterId];
            }
            break;
        case 5:
            currentMotion = 6;
            motionFlags = 0;
            motionProgress = (float)playerWork->actionTimer / playerWork->actionDuration;
            playerObject->trans.y = -300.0 * -sind((270.0f * motionProgress));
            if ((playerWork->actionSoundPlayed == 0) && (motionProgress < 0.7f)) {
                playerWork->actionSoundPlayed = 1;
                CreateWaterEffect(&playerObject->trans, 5.0f);
                CreateSprinkleEffect(&playerObject->trans, -1.0f, -1.0f, -1);
                HuAudFXPlay(0x7D3);
                CharFXPlay(playerWork->characterId, 0x125);
            }
            if (--playerWork->actionTimer == 0) {
                playerWork->actionFinished = 1;
                playerWork->actionState = 6;
                playerObject->trans.y = animationOffsets[playerWork->characterId];
                CreateWaterEffect(&playerObject->trans, -1.0f);
                CreateSprinkleEffect(&playerObject->trans, -1.0f, -1.0f, -1);
            }
            break;
        case 6:
            if (playerWork->actionComplete == 0) {
                currentMotion = 0;
            }
            else {
                currentMotion = 5;
            }
            motionFlags = 0x40000001;
            if ((GetGameSequenceState() == 5) && (playerWork->actionWasRepeated == 0) && (winnerCount == 1)) {
                playerObjects = omGetGroupMemberListEx(HuPrcCurrentGet(), 0);
                VECSubtract(&playerObjects[*winningPlayerIndices]->trans, &playerObject->trans, &targetPosition);
                playerObject->rot.y = ApproachAngle(playerObject->rot.y, atan2d(targetPosition.x, targetPosition.z), 0.1f);
            }
            if (GetGameSequenceState() == 0xB) {
                playerWork->actionState = 7;
                if (playerWork->actionWasRepeated != 0) {
                    Hu3DMotionOverlaySet(playerObject->mdlId[0], playerObject->mtnId[9]);
                    CharFXPlay(playerWork->characterId, 0x124);
                }
            }
            break;
        case 7:
            if (playerWork->actionWasRepeated != 0) {
                currentMotion = 5;
                motionFlags = 0x40000001;
                if (73.0f <= Hu3DMotionOverlayTimeGet(playerObject->mdlId[0])) {
                    Hu3DMotionOverlayTimeSet(playerObject->mdlId[0], 35.0f);
                    Hu3DModelAttrReset(playerObject->mdlId[0], HU3D_MOTATTR_OVL_PAUSE);
                }
            }
            else if (playerWork->actionComplete == 0) {
                currentMotion = 8;
                motionFlags = 0;
            }
            break;
    }
    if ((playerWork->actionComplete != 0) && (playerWork->actionState >= 6) && (playerWork->actionState <= 7) && ((frand() & 0x3F) == 0)) {
        CreateSprinkleEffect(&playerObject->trans, 0.3f, -1.0f, -1);
    }
    if ((currentMotion != playerWork->currentMotion) || (motionFlags != playerWork->motionFlags)) {
        playerWork->currentMotion = currentMotion;
        playerWork->motionFlags = motionFlags;
        CharMotionShiftSet(playerWork->characterId, playerObject->mtnId[playerWork->currentMotion], 0.0f, 8.0f, motionFlags);
    }
}

void DestroyPlayerWork(OMOBJ *object)
{
    M456DllMainWork *playerWork = object->data;
    if (object->data != NULL) {
        HuMemDirectFree(object->data);
        object->data = NULL;
    }
}

float ApproachAngle(float currentAngle, float targetAngle, float interpolationWeight)
{
    float angleDifference;
    float interpolatedAngle;

    angleDifference = fmod(targetAngle - currentAngle, 360.0);
    if (angleDifference < 0.0f) {
        angleDifference += 360.0f;
    }
    if (angleDifference > 180.0f) {
        angleDifference -= 360.0f;
    }
    interpolatedAngle = fmod(currentAngle + (interpolationWeight * angleDifference), 360.0);
    if (interpolatedAngle < 0.0f) {
        interpolatedAngle += 360.0f;
    }
    return interpolatedAngle;
}

void CreateCountdownCamera(HUPROCESS *process)
{
    InterpolateCameraSettings(0, 0, 0.0f);
    countdownObject = omAddObjEx(process, 0xA, 0, 0, -1, UpdateCountdownCamera);
}

void InterpolateCameraSettings(s32 startSettingIndex, s32 endSettingIndex, float interpolationWeight)
{
    CZoom = cameraSettings[startSettingIndex].cameraZoom + (interpolationWeight * (cameraSettings[endSettingIndex].cameraZoom - cameraSettings[startSettingIndex].cameraZoom));
    Center.x = cameraSettings[startSettingIndex].centerX + (interpolationWeight * (cameraSettings[endSettingIndex].centerX - cameraSettings[startSettingIndex].centerX));
    Center.y = cameraSettings[startSettingIndex].centerY + (interpolationWeight * (cameraSettings[endSettingIndex].centerY - cameraSettings[startSettingIndex].centerY));
    Center.z = cameraSettings[startSettingIndex].centerZ + (interpolationWeight * (cameraSettings[endSettingIndex].centerZ - cameraSettings[startSettingIndex].centerZ));
    CRot.x = cameraSettings[startSettingIndex].rotationX + (interpolationWeight * (cameraSettings[endSettingIndex].rotationX - cameraSettings[startSettingIndex].rotationX));
    CRot.y = cameraSettings[startSettingIndex].rotationY + (interpolationWeight * (cameraSettings[endSettingIndex].rotationY - cameraSettings[startSettingIndex].rotationY));
    CRot.z = cameraSettings[startSettingIndex].rotationZ + (interpolationWeight * (cameraSettings[endSettingIndex].rotationZ - cameraSettings[startSettingIndex].rotationZ));
}

float cameraTransitionProgress[2] = { 0.0f, 0.0f };

void UpdateCountdownCamera(OMOBJ *object)
{
    HU3DCAMERA *work = Hu3DCamera;

    switch (GetGameSequenceState()) {
        case 1:
            InterpolateCameraSettings(0, 1, sind((90.0f * cameraTransitionProgress[0])));
            if (cameraTransitionProgress[0] < 1.0f) {
                cameraTransitionProgress[0] += 1.0f / REFRESH_RATE;
                if (cameraTransitionProgress[0] > 1.0f) {
                    cameraTransitionProgress[0] = 1.0f;
                    break;
                }
            }
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            break;
        case 8:
            InterpolateCameraSettings(1, 1, 0.0f);
            break;
        case 9:
            break;
    }
}

CountdownDigitSprite countdownDigitSprites[0x10] = {
    { DATA_MAKE_NUM(DATADIR_M456, 0x44), 0x10E },
    { 0x610037, 0xFA },
    { 0x610030, 0xFA },
    { 0x610030, 0xFA },
    { 0x610030, 0xFA },
    { 0x610030, 0xFA },
    { 0x610030, 0xFA },
    { 0x610030, 0xFA },
    { 0x610030, 0xFA },
    { 0x610031, 0xFA },
    { 0x610031, 0xFA },
    { 0x610031, 0xFA },
    { 0x610031, 0xFA },
    { 0x610031, 0xFA },
    { 0x610031, 0xFA },
    { 0x610031, 0xFA },
};

Vec2f countdownDigitPositions[0x17] = {
    { 0.0f, 0.0f },
    { -52.0f, -9.0f },
    { 53.0f, -8.0f },
    { 35.0f, -8.0f },
    { 9.0f, -8.0f },
    { -9.0f, -8.0f },
    { -31.0f, -8.0f },
    { 23.0f, -8.0f },
    { -17.0f, -8.0f },
    { 53.0f, 8.0f },
    { 35.0f, 8.0f },
    { 9.0f, 8.0f },
    { -9.0f, 8.0f },
    { -31.0f, 8.0f },
    { 23.0f, 8.0f },
    { -17.0f, 8.0f },
    { 355.0f, 0.0f },
    { 0.0f, 0.0f },
    { 275.0f, 128.0f },
    { 950.0f, 6.0f },
    { 0.0f, 0.0f },
    { 0.0f, 190.0f },
    { 0.0f, 550.0f },
};

void CreateTimerObject(HUPROCESS *objectManager)
{
    CountdownDisplayWork *timerWork;
    s32 digitIndex;
    OMOBJ *timerObject;

    timerObject = omAddObjEx(objectManager, 0x7FD9, 0, 0, -1, UpdateTimerObject);
    timerWork = &countdownDisplayWork;
    timerObject->data = timerWork;
    memset(timerWork, 0, sizeof(CountdownDisplayWork));
    timerWork->remainingTime = 60 * REFRESH_RATE;
    timerWork->spriteX = 320.0f;
    timerWork->spriteY = 64.0f;
    timerWork->scale = 1.0f;
    for (digitIndex = 0; digitIndex < 0x10; digitIndex++) {
        timerWork->digitSprites[digitIndex] = -1;
        if (countdownDigitSprites[digitIndex].spriteData != 0) {
            timerWork->digitSprites[digitIndex] = espEntry(countdownDigitSprites[digitIndex].spriteData, countdownDigitSprites[digitIndex].spriteLayer, 0);
            espAttrSet(timerWork->digitSprites[digitIndex], HUSPR_ATTR_LINEAR);
            espDispOff(timerWork->digitSprites[digitIndex]);
        }
    }
    einterpolationWeightolorSet(timerWork->digitSprites[0], 0, 0, 0);
    espTPLvlSet(timerWork->digitSprites[0], 0.699999988079071);
    espBankSet(timerWork->digitSprites[7], 0xB);
    espBankSet(timerWork->digitSprites[8], 0xA);
    espBankSet(timerWork->digitSprites[14], 0xB);
    espBankSet(timerWork->digitSprites[15], 0xA);
}

void UpdateTimerObject(OMOBJ *timerObject)
{
    u32 timeDigits[3];

    CountdownDisplayWork *timerWork;
    s32 spriteIndex;
    u32 remainingSeconds;
    s32 digitIndex;
    s32 digitSpriteIndex;

    timerWork = timerObject->data;
    for (spriteIndex = 0; spriteIndex < 0x10; spriteIndex++) {
        if (timerWork->digitSprites[spriteIndex] >= 0) {
            if (timerWork->isVisible == 0) {
                espDispOff(timerWork->digitSprites[spriteIndex]);
            }
            else {
                espDispOn(timerWork->digitSprites[spriteIndex]);
                espPosSet(
                    timerWork->digitSprites[spriteIndex], 0.9f * (timerWork->spriteX + countdownDigitPositions[spriteIndex].x), timerWork->spriteY + countdownDigitPositions[spriteIndex].y);
            }
        }
    }
    for (spriteIndex = 0; spriteIndex < 2; spriteIndex++) {
        if (spriteIndex != 0) {
            remainingSeconds = timerWork->elapsedTime;
            digitSpriteIndex = 9;
        }
        else {
            remainingSeconds = timerWork->remainingTime;
            digitSpriteIndex = 2;
        }
        timeDigits[0] = remainingSeconds % REFRESH_RATE;
        remainingSeconds = (remainingSeconds - timeDigits[0]) / REFRESH_RATE;
#if VERSION_PAL
        timeDigits[0] = 100.0f * (timeDigits[0] / (float)REFRESH_RATE);
#else
        timeDigits[0] = 100.0f * (1.0f / REFRESH_RATE * timeDigits[0]);
#endif
        timeDigits[1] = remainingSeconds % 60;
        remainingSeconds = (remainingSeconds - timeDigits[1]) / 60;
        timeDigits[2] = remainingSeconds;
        for (digitIndex = 0; digitIndex < 3; digitIndex++) {
            espBankSet(timerWork->digitSprites[digitSpriteIndex++], timeDigits[digitIndex] % 10);
            if (digitIndex < 2) {
                espBankSet(timerWork->digitSprites[digitSpriteIndex++], timeDigits[digitIndex] / 10);
            }
        }
    }
    if (timerWork->isPulsing != 0) {
        if (timerWork->pulseDirection == 0) {
            if ((timerWork->scale += 0.02f) >= 1.2f) {
                timerWork->scale = 1.2f;
                timerWork->pulseDirection = 1;
            }
        }
        else {
            if ((timerWork->scale -= 0.04f) <= 1.0f) {
                timerWork->scale = 1.0f;
                timerWork->pulseDirection = 0;
            }
        }
        for (spriteIndex = 1; spriteIndex < 9; spriteIndex++) {
            espScaleSet(timerWork->digitSprites[spriteIndex], timerWork->scale, timerWork->scale);
        }
    }
}

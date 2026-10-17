#include "game/board/main.h"
#include "REL/mentDll.h"
#include "ext_math.h"
#include "game/armem.h"
#include "game/chrman.h"
#include "game/hsfex.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/saveload.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#ifndef __MWERKS__
extern s32 rand8(void);
#include "game/audio.h"
#endif

typedef struct MentDllParticleEffectWork {
    /* 0x00 */ s32 particleLifetime;
    /* 0x04 */ s32 particleState;
    /* 0x08 */ s32 rotationSpeed;
    /* 0x0C */ float horizontalVelocity;
    /* 0x10 */ float verticalVelocity;
    /* 0x14 */ float depthVelocity;
    /* 0x18 */ float particleAlpha;
} MentDllParticleEffectWork; /* size = 0x1C */

typedef struct MentDllAnimatedModelWork {
    /* 0x00 */ s32 animationTimer;
    /* 0x04 */ s32 animationDuration;
    /* 0x08 */ s32 isActive;
    /* 0x0C */ float positionX;
    /* 0x10 */ float positionY;
    /* 0x14 */ float positionZ;
    /* 0x18 */ float startX;
    /* 0x1C */ float startY;
    /* 0x20 */ float startZ;
    /* 0x24 */ float velocityX;
    /* 0x28 */ float velocityY;
    /* 0x2C */ float velocityZ;
} MentDllAnimatedModelWork; /* size = 0x30 */

typedef void (*MentDllUpdateCallback)(OMOBJ *, ...);

typedef struct MentDllRecordPlayerWork {
    /* 0x00 */ OMOBJ *ownerObject;
    /* 0x04 */ MentDllUpdateCallback updateCallback;
    /* 0x08 */ s32 state;
    /* 0x0C */ s32 stateTimer;
    /* 0x10 */ char pad10[0x48]; /* maybe part of stateTimer[0x13]? */
    /* 0x58 */ s32 recordMinutes;
    /* 0x5C */ s32 recordSeconds;
    /* 0x60 */ s32 isComputerPlayer;
    /* 0x64 */ s32 difficulty;
    /* 0x68 */ s32 character;
    /* 0x6C */ s32 controllerPort;
    /* 0x70 */ s32 recordFields[4];
    /* 0x80 */ s32 recordExtras[4];
} MentDllRecordPlayerWork; /* size = 0x90 */

typedef struct MentDllMenuItemWork { // maybe the same as the other one?
    /* 0x00 */ OMOBJ *ownerObject;
    /* 0x04 */ MentDllUpdateCallback updateCallback;
    /* 0x08 */ s32 isVisible;
    /* 0x0C */ s32 state;
    /* 0x10 */ char pad10[0x20];
    /* 0x30 */ s32 selectionIndex;
    /* 0x34 */ s32 selectionTimer;
    /* 0x38 */ char pad38[0x20];
} MentDllMenuItemWork; /* size = 0x58 */

typedef struct MentDllPlayerSelectionWork {
    /* 0x00 */ OMOBJ *ownerObject;
    /* 0x04 */ MentDllUpdateCallback updateCallback;
    /* 0x08 */ s32 playerSelections[5];
    /* 0x1C */ s32 selectionFlags[5];
    /* 0x30 */ s32 selectionState;
    /* 0x34 */ s32 selectionTimer;
    /* 0x38 */ s32 selectionIndex;
    /* 0x3C */ s32 selectionMode;
    /* 0x40 */ s32 playerCount;
    /* 0x44 */ s32 selectionHistory[5];
} MentDllPlayerSelectionWork; /* size = 0x58 */

void MentCreateMenuCameraObject(void);
void MentUpdateMenuItemAnimation(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork);
void MentCreateMenuItemModel(OMOBJ *object, s32 itemIndex);
void MentCreateMenuBackgroundObjects(void);
void MentUpdateMenuItemObject(OMOBJ *object, MentDllMenuItemWork *menuItemWork);
void MentCreateMenuItemObjects(void);
void MentCreateMenuEffectObject(void);
void MentCreateRecordPlayerObjects(void);
void MentSetDefaultMenuCamera(void);
void MentZoomMenuCameraIn(void);
void MentSetAlternateMenuCamera(void);
void MentZoomAlternateMenuCameraIn(void);
void MentAnimateMenuCameraToFront(void);
void MentAnimateMenuCameraToBoard(void);
void MentAnimateMenuCameraToDefaultView(void);
void MentAnimateMenuCameraAcrossBoard(void);
void MentAnimateMenuCameraAroundPlayer(void);
void MentFocusMenuCameraOnPlayer(void);
void MentAnimateMenuCameraReset(void);
void MentAnimateMenuCameraToCloseView(void);
void MentResetMenuCamera(void);
void MentResetAlternateMenuCamera(void);
void MentAnimateMenuCameraToSelectionView(void);
void MentInitializePlayerSelectionSpriteGroup(void);
void MentAnimateCharacterRecordSpritesIn(void);
void MentAnimateCharacterRecordSpritesOut(void);
void MentAnimatePlayerSelection(MentDllPlayerSelectionWork *selectionWork, s32 selectionIndex, s32 playerCount, s32 displayMode);
void MentCreateRecordSprites(void);
void MentUpdateRecordDisplay(OMOBJ *object, MentDllRecordPlayerWork *recordWork);
s32 MentGetRecordSpriteIndex(MentDllRecordPlayerWork *recordWork);
void MentUpdateRecordSpriteAnimation(OMOBJ *object, MentDllRecordPlayerWork *recordWork);
void MentUpdateRecordPlayerSelection(MentDllRecordPlayerWork *recordWork);
void MentGetPlayerSelectionState(MentDllRecordPlayerWork *recordWork);
void MentUpdateRecordSelection(void);
void MentAnimatePlayerSelectionCursor(MentDllPlayerSelectionWork *selectionWork, s32 selectionIndex, s32 playerIndex, s32 animationFrames);
void MentCreateRecordSelectionSprites(void);
void MentUpdateRecordSelectionSprites(void);
void MentCreatePlayerSelectionSprites(void);
void MentAnimatePlayerSelectionTransition(MentDllPlayerSelectionWork *selectionWork, s32 selectionIndex, s32 targetIndex, s32 animationFrames);
void MentAnimateSelectionWindow(s32 animationMode);
void MentUpdateSelectionWindow(void);
void MentUpdatePlayerSelectionDisplay(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork);
void MentCreatePlayerSelectionWindows(void);
void MentUpdatePlayerSelectionCursor(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork);
void MentAnimatePlayerMenuTransition(MentDllPlayerSelectionWork *selectionWork, s32 selectionIndex, s32 targetIndex, s32 animationFrames);
void MentCreatePlayerMenuWindows(void);
void MentUpdatePlayerMenuWindows(void);
void MentCreateCharacterSelectionSprites(void);
void MentUpdateCharacterSelectionSprites(void);
void MentUpdateCharacterSelectionWindows(void);
void MentAnimateCharacterSelectionTransition(MentDllPlayerSelectionWork *selectionWork, s32 selectionIndex, s32 targetIndex, s32 animationFrames);
void MentCreateBoardSelectionSprites(void);
void MentUpdateBoardSelectionSprites(void);
void MentCreateBoardSelectionWindows(void);
void MentUpdateBoardSelectionWindows(void);
void MentUpdateBoardRecordDisplay(OMOBJ *object, MentDllRecordPlayerWork *recordWork);
void MentUpdateBoardPlayerSelection(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork);
void MentAnimateMenuOptionTransition(MentDllPlayerSelectionWork *selectionWork, s32 selectionIndex, s32 targetIndex, s32 animationFrames);
void MentCreateMenuOptionWindows(void);
void MentUpdateMenuOptionWindows(void);
void MentUpdateMenuOptionHighlights(void);
void MentUpdateMenuPlayerSelection(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork);
void MentSetMenuCursorPosition(s32 cursorIndex);
void MentUpdateMenuCursor(void);
void MentShowMinigamePlayerCountIndicators(void);
void MentHideMinigamePlayerCountIndicators(void);
void MentAnimatePlayerCountSelection(MentDllPlayerSelectionWork *selectionWork, s32 currentPlayerCount, s32 targetPlayerCount, s32 animationFrames);
void MentCreateParticleEffectObject(OMOBJ *object);
void MentCreateMenuParticles(OMOBJ *object);
void MentReturnToMenu(s32 menuMode);
void MentFadeOutMenuMusic(s32 isSinglePlayerMode);
void MentLaunchSelectedMode(s32 unusedMode);

Vec menuCameraPositions[6] = {
    { 0.0f, 0.0f, 460.0f },
    { -240.0f, 0.0f, 460.0f },
    { -120.0f, 0.0f, 460.0f },
    { 120.0f, 50.0f, 460.0f },
    { 240.0f, 0.0f, 460.0f },
    { 1200.0f, 0.0f, 460.0f },
};

float menuCameraYawAngles[6] = { 0.0f, 20.0f, 10.0f, -10.0f, -20.0f, 0.0f };

MentDllPlayerSelectionWork selectedPlayerWork;
MentDllPlayerSelectionWork playerSelectionWork[2];
MentDllMenuItemWork menuItemWork[6];
MentDllMenuItemWork selectedMenuItemWork;
MentDllRecordPlayerWork recordPlayerWork[4];
OMOBJ *menuObjects[3];
OMOBJ *particleEffectObject;
MentDllAnimatedModelWork animatedModelWork[0x8C];
MentDllParticleEffectWork particleEffectWork[0xC8];
s32 menuSelectionMode;
s32 cameraTargetX;
s32 menuTransitionComplete;
s32 recordPlayerIndex;
s32 characterSelectionCount;
s32 specialMenuMode;
HUPROCESS *menuObjectManagerProcess;
s32 menuLaunchParameters[8];
MentDllCameraWork menuCameraWork;
s32 menuSoundEffectHandles[16];
s32 menuSequenceHandles[4];
s32 unusedMenuState[3];

void MentInitializeModeScene(HUPROCESS *objectManagerProcess)
{
    s32 initializeModeSceneIndex;

    menuObjectManagerProcess = objectManagerProcess;
    MentInitializeWindowSystem();
    MentCreateDirectionalLight();
    MentCreateMenuShadow(menuLaunchParameters[0]);
    if (menuLaunchParameters[0] != 0xB) {
        CharDataClose(-1);
    }
    menuLaunchParameters[4] = GWGameStat.open_w06;
    menuLaunchParameters[5] = GWGameStat.veryHardUnlock;
    menuLaunchParameters[6] = GWGameStat.customPackEnable;
    switch (menuLaunchParameters[0]) {
        case 0:
            HuAudSndGrpSetSet(7);
            if (menuLaunchParameters[1] == 0) {
                MentCreateMenuCamera(menuObjectManagerProcess, MentSetDefaultMenuCamera);
            }
            else {
                MentCreateMenuCamera(menuObjectManagerProcess, MentSetAlternateMenuCamera);
            }
            for (initializeModeSceneIndex = 0; initializeModeSceneIndex < 2; initializeModeSceneIndex++) {
                HuPrcChildCreate(MentCreateMenuBackgroundObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            }
            for (initializeModeSceneIndex = 0; initializeModeSceneIndex < 6; initializeModeSceneIndex++) {
                HuPrcChildCreate(MentCreateMenuItemObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            }
            for (initializeModeSceneIndex = 0; initializeModeSceneIndex < 4; initializeModeSceneIndex++) {
                HuPrcChildCreate(MentCreateRecordPlayerObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            }
            if (menuLaunchParameters[1] == 0) {
                menuObjects[0] = omAddObjEx(menuObjectManagerProcess, 0x1000, 0x8D, 0x10, -1, MentCreateParticleEffectObject);
            }
            break;
        case 1:
            HuAudSndGrpSetSet(7);
            if (menuLaunchParameters[1] == 0) {
                MentCreateMenuCamera(menuObjectManagerProcess, MentSetDefaultMenuCamera);
            }
            else {
                MentCreateMenuCamera(menuObjectManagerProcess, MentSetAlternateMenuCamera);
            }
            HuPrcChildCreate(MentCreateMenuBackgroundObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            for (initializeModeSceneIndex = 0; initializeModeSceneIndex < 5; initializeModeSceneIndex++) {
                HuPrcChildCreate(MentCreateMenuItemObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            }
            HuPrcChildCreate(MentCreateRecordPlayerObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            if (menuLaunchParameters[1] == 0) {
                menuObjects[0] = omAddObjEx(menuObjectManagerProcess, 0x1000, 0x8D, 0x10, -1, MentCreateParticleEffectObject);
            }
            particleEffectObject = omAddObjEx(menuObjectManagerProcess, 0x1000, 0xC9, 0x10, -1, MentCreateMenuParticles);
            break;
        case 3:
            HuAudSndCommonGrpSet(0x56, 1);
            HuAudSndCommonGrpSet(0x6B, 0);
            MentCreateMenuCamera(menuObjectManagerProcess, MentSetDefaultMenuCamera);
            HuPrcChildCreate(MentCreateMenuBackgroundObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            characterSelectionCount = 4;
            HuPrcChildCreate(MentCreateMenuItemObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            menuObjects[0] = omAddObjEx(menuObjectManagerProcess, 0x1000, 0x8D, 0x10, -1, MentCreateParticleEffectObject);
            break;
        case 4:
            HuAudSndGrpSetSet(3);
            MentCreateMenuCamera(menuObjectManagerProcess, MentSetDefaultMenuCamera);
            HuPrcChildCreate(MentCreateMenuBackgroundObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            characterSelectionCount = 2;
            HuPrcChildCreate(MentCreateMenuItemObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            menuObjects[0] = omAddObjEx(menuObjectManagerProcess, 0x1000, 0x8D, 0x10, -1, MentCreateParticleEffectObject);
            break;
        case 5:
            MentCreateMenuCamera(menuObjectManagerProcess, MentSetDefaultMenuCamera);
            HuPrcChildCreate(MentCreateMenuBackgroundObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            characterSelectionCount = 3;
            HuPrcChildCreate(MentCreateMenuItemObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            menuObjects[0] = omAddObjEx(menuObjectManagerProcess, 0x1000, 0x8D, 0x10, -1, MentCreateParticleEffectObject);
            break;
        case 10:
            HuAudSndGrpSetSet(7);
            MentCreateMenuCamera(menuObjectManagerProcess, MentResetMenuCamera);
            HuPrcChildCreate(MentCreateMenuBackgroundObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            for (initializeModeSceneIndex = 0; initializeModeSceneIndex < 5; initializeModeSceneIndex++) {
                HuPrcChildCreate(MentCreateMenuItemObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            }
            HuPrcChildCreate(MentCreateRecordPlayerObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            break;
        case 11:
            specialMenuMode = 1;
            HuAudSndGrpSetSet(7);
            MentCreateMenuCamera(menuObjectManagerProcess, MentResetAlternateMenuCamera);
            HuPrcChildCreate(MentCreateMenuBackgroundObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            HuPrcChildCreate(MentCreateMenuEffectObject, 0x64, 0x3000, 0, HuPrcCurrentGet());
            MentSleepTicks(2);
            HuDataDirClose(DATADIR_MENT);
            HuPrcChildCreate(MentCreateRecordPlayerObjects, 0x64, 0x3000, 0, HuPrcCurrentGet());
            break;
    }
    HuPrcChildCreate(MentCreateMenuCameraObject, 0xC8, 0x3000, 0, HuPrcCurrentGet());
}

void MentShowProcessDebugOverlay(void)
{
    s32 showProcessDebugOverlayIndex;
    s32 showProcessDebugOverlayStateValue = 0x10;
    s32 showProcessDebugOverlayStateValue2 = 0x28;
    s32 showProcessDebugOverlayStateValue3 = 0;
    {
        GXColor showProcessDebugOverlayStateValue4 = { 0x00, 0x00, 0x80, 0x80 };
        while (1) {
            MentSleepFrame();
            if (HuPadBtnDown[0] & PAD_TRIGGER_R) {
                showProcessDebugOverlayStateValue3++;
                showProcessDebugOverlayStateValue3 = showProcessDebugOverlayStateValue3 % 2;
            }
            if (showProcessDebugOverlayStateValue3 == 0) {
                continue;
            }
            printWin(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2, 0xDC, 0x122, &showProcessDebugOverlayStateValue4);
            if (selectedPlayerWork.updateCallback == NULL) {
                print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2, 1.0f, "PROC_MAIN    -> FALSE");
            }
            else {
                print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2, 1.0f, "PROC_MAIN    -> TRUE");
            }
            for (showProcessDebugOverlayIndex = 0; showProcessDebugOverlayIndex < 2; showProcessDebugOverlayIndex++) {
                if (playerSelectionWork[showProcessDebugOverlayIndex].updateCallback == NULL) {
                    print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2 + 0x14 + (showProcessDebugOverlayIndex * 0xA), 1.0f, "PROC_MAP%d    -> FALSE", showProcessDebugOverlayIndex);
                }
                else {
                    print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2 + 0x14 + (showProcessDebugOverlayIndex * 0xA), 1.0f, "PROC_MAP%d    -> TRUE", showProcessDebugOverlayIndex);
                }
            }
            for (showProcessDebugOverlayIndex = 0; showProcessDebugOverlayIndex < 6; showProcessDebugOverlayIndex++) {
                if (menuItemWork[showProcessDebugOverlayIndex].updateCallback == NULL) {
                    print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2 + 0x32 + (showProcessDebugOverlayIndex * 0xA), 1.0f, "PROC_SUB%d    -> FALSE", showProcessDebugOverlayIndex);
                }
                else {
                    print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2 + 0x32 + (showProcessDebugOverlayIndex * 0xA), 1.0f, "PROC_SUB%d    -> TRUE", showProcessDebugOverlayIndex);
                }
            }
            for (showProcessDebugOverlayIndex = 0; showProcessDebugOverlayIndex < 4; showProcessDebugOverlayIndex++) {
                if (recordPlayerWork[showProcessDebugOverlayIndex].updateCallback == NULL) {
                    print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2 + 0x78 + (showProcessDebugOverlayIndex * 0xA), 1.0f, "PROC_PLAYER%d -> FALSE", showProcessDebugOverlayIndex);
                }
                else {
                    print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2 + 0x78 + (showProcessDebugOverlayIndex * 0xA), 1.0f, "PROC_PLAYER%d -> TRUE", showProcessDebugOverlayIndex);
                }
            }
            print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2 + 0xAA, 1.0f, "MODE:%d MAP:%d", menuLaunchParameters[0], menuLaunchParameters[2]);
            print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2 + 0xBE, 1.0f, "NO) IDX:GRP:COM:DIF:PAD:CHR");
            for (showProcessDebugOverlayIndex = 0; showProcessDebugOverlayIndex < 4; showProcessDebugOverlayIndex++) {

                print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2 + 0xC8 + (showProcessDebugOverlayIndex * 0xA), 1.0f, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", showProcessDebugOverlayIndex, recordPlayerWork[showProcessDebugOverlayIndex].recordMinutes,
                    recordPlayerWork[showProcessDebugOverlayIndex].recordSeconds, recordPlayerWork[showProcessDebugOverlayIndex].isComputerPlayer, recordPlayerWork[showProcessDebugOverlayIndex].difficulty, recordPlayerWork[showProcessDebugOverlayIndex].controllerPort, recordPlayerWork[showProcessDebugOverlayIndex].character);
            }
            print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2 + 0xF0, 1.0f, "NO) FG0:FG1:FG2:FG3");
            for (showProcessDebugOverlayIndex = 0; showProcessDebugOverlayIndex < 4; showProcessDebugOverlayIndex++) {
                print8(showProcessDebugOverlayStateValue, showProcessDebugOverlayStateValue2 + 0xFA + (showProcessDebugOverlayIndex * 0xA), 1.0f, "%2d) %3d:%3d:%3d:%3d", showProcessDebugOverlayIndex, recordPlayerWork[showProcessDebugOverlayIndex].recordFields[0],
                    recordPlayerWork[showProcessDebugOverlayIndex].recordFields[1], recordPlayerWork[showProcessDebugOverlayIndex].recordFields[2], recordPlayerWork[showProcessDebugOverlayIndex].recordFields[3]);
            }
        }
    }
}

s32 MentCalculateUnlockedMapDifficulty(void)
{
    s32 calculateUnlockedMapDifficultyStateValue = 0;

    if (_CheckFlag(FLAG_ID_MAKE(0, 2)) != 0) {
        calculateUnlockedMapDifficultyStateValue++;
    }
    if (_CheckFlag(FLAG_ID_MAKE(0, 3)) != 0) {
        calculateUnlockedMapDifficultyStateValue++;
    }
    if (_CheckFlag(FLAG_ID_MAKE(0, 4)) != 0) {
        calculateUnlockedMapDifficultyStateValue++;
    }
    if (_CheckFlag(FLAG_ID_MAKE(0, 5)) != 0) {
        calculateUnlockedMapDifficultyStateValue++;
    }
    if (_CheckFlag(FLAG_ID_MAKE(0, 6)) != 0) {
        calculateUnlockedMapDifficultyStateValue++;
    }
    if (_CheckFlag(FLAG_ID_MAKE(0, 7)) != 0) {
        calculateUnlockedMapDifficultyStateValue++;
    }
    OSReport("DIF OMAKASE MAP %d\n", calculateUnlockedMapDifficultyStateValue);
    calculateUnlockedMapDifficultyStateValue = calculateUnlockedMapDifficultyStateValue / 2;
    if (calculateUnlockedMapDifficultyStateValue < 0) {
        calculateUnlockedMapDifficultyStateValue = 0;
    }
    else if (calculateUnlockedMapDifficultyStateValue > 3) {
        calculateUnlockedMapDifficultyStateValue = 3;
    }
    OSReport("DIF OMAKASE DIF %d\n", calculateUnlockedMapDifficultyStateValue);
    return calculateUnlockedMapDifficultyStateValue;
}

s32 boardDataDirectories[0xF] = {
    DATADIR_W01,
    DATADIR_W02,
    DATADIR_W03,
    DATADIR_W04,
    DATADIR_W05,
    DATADIR_W06,
    DATADIR_W10,
    DATADIR_MARIOMDL1,
    DATADIR_LUIGIMDL1,
    DATADIR_PEACHMDL1,
    DATADIR_YOSHIMDL1,
    DATADIR_WARIOMDL1,
    DATADIR_DONKEYMDL1,
    DATADIR_DAISYMDL1,
    DATADIR_WALUIGIMDL1,
};

void MentLoadBoardData(void)
{
    s32 loadBoardDataAsyncReadHandle = HuDataDirReadAsync(DATADIR_BOARD);

    if (loadBoardDataAsyncReadHandle != -1) {
        while (HuDataGetAsyncStat(loadBoardDataAsyncReadHandle) == 0) {
            HuPrcVSleep();
        }
    }
    HuAR_MRAMtoARAM(DATADIR_BOARD);
    while (HuARDMACheck() != 0) {
        HuPrcVSleep();
    }
    HuDataDirClose(DATADIR_BOARD);
    if (_CheckFlag(FLAG_ID_MAKE(1, 11)) != 0) {
        loadBoardDataAsyncReadHandle = HuDataDirReadAsync(DATADIR_W10);
    }
    else {
        loadBoardDataAsyncReadHandle = HuDataDirReadAsync(boardDataDirectories[GWSystem.board]);
    }
    if (loadBoardDataAsyncReadHandle != -1) {
        while (HuDataGetAsyncStat(loadBoardDataAsyncReadHandle) == 0) {
            HuPrcVSleep();
        }
    }
    menuTransitionComplete = 1;
    HuPrcEnd();
    while (1) {
        HuPrcVSleep();
    }
}

void MentConfigureBoardPlayers(void)
{
    s32 configureBoardPlayersPlayerIndex;
    s32 configureBoardPlayersStateValue;
    s32 configureBoardPlayersPlayerIndex2;

    for (configureBoardPlayersPlayerIndex = 1; configureBoardPlayersPlayerIndex < 4; configureBoardPlayersPlayerIndex++) {
        recordPlayerWork[configureBoardPlayersPlayerIndex].character = -1;
        recordPlayerWork[configureBoardPlayersPlayerIndex].controllerPort = configureBoardPlayersPlayerIndex;
        recordPlayerWork[configureBoardPlayersPlayerIndex].isComputerPlayer = 1;
    }
    for (configureBoardPlayersPlayerIndex = 1; configureBoardPlayersPlayerIndex < 4; configureBoardPlayersPlayerIndex++) {
        do {
            recordPlayerWork[configureBoardPlayersPlayerIndex].character = rand8() % 8;
            for (configureBoardPlayersPlayerIndex2 = 0; configureBoardPlayersPlayerIndex2 < 4; configureBoardPlayersPlayerIndex2++) {
                if ((configureBoardPlayersPlayerIndex != configureBoardPlayersPlayerIndex2) && (recordPlayerWork[configureBoardPlayersPlayerIndex2].character != -1)
                    && (recordPlayerWork[configureBoardPlayersPlayerIndex].character == recordPlayerWork[configureBoardPlayersPlayerIndex2].character)) {
                    break;
                }
            }
        } while (configureBoardPlayersPlayerIndex2 != 4);
    }
    GWSystem.diff_story = playerSelectionWork[0].selectionFlags[0];
    for (configureBoardPlayersPlayerIndex = 0; configureBoardPlayersPlayerIndex < 4; configureBoardPlayersPlayerIndex++) {
        recordPlayerWork[configureBoardPlayersPlayerIndex].difficulty = GWSystem.diff_story;
        if (GWSystem.diff_story == 4) {
            recordPlayerWork[configureBoardPlayersPlayerIndex].difficulty = MentCalculateUnlockedMapDifficulty();
        }
    }
    for (configureBoardPlayersPlayerIndex = 0; configureBoardPlayersPlayerIndex < 4; configureBoardPlayersPlayerIndex++) {
        GWPlayerCfg[configureBoardPlayersPlayerIndex].character = recordPlayerWork[configureBoardPlayersPlayerIndex].character;
        GWPlayerCfg[configureBoardPlayersPlayerIndex].pad_idx = recordPlayerWork[configureBoardPlayersPlayerIndex].controllerPort;
        GWPlayerCfg[configureBoardPlayersPlayerIndex].diff = recordPlayerWork[configureBoardPlayersPlayerIndex].difficulty;
        GWPlayerCfg[configureBoardPlayersPlayerIndex].group = 0;
        GWPlayerCfg[configureBoardPlayersPlayerIndex].iscom = recordPlayerWork[configureBoardPlayersPlayerIndex].isComputerPlayer;
    }
    BoardSaveInit(menuLaunchParameters[2]);
    CharDataClose(-1);
}

void MentLaunchSelectedBoard(void)
{
    s32 launchSelectedBoardStateValue[7] = { 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F };
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        MentSleepFrame();
    }
    CharModelKill(-1);
    MGSeqKillAll();
    {
        OMOVLHIS *launchSelectedBoardOverlayHistory = omOvlHisGet(0);
    }
    omOvlHisChg(0, DLL_mstory3dll, 0, 0);
    do {
        MentSleepFrame();
    } while (menuTransitionComplete != 1);
    HuAudSeqAllFadeOut(0x3E8);
    HuAudSStreamAllFadeOut(0x3E8);
    CharMotionInit(GWPlayerCfg[0].character);
    CharMotionInit(GWPlayerCfg[1].character);
    CharMotionInit(GWPlayerCfg[2].character);
    CharMotionInit(GWPlayerCfg[3].character);
    omOvlCallEx(launchSelectedBoardStateValue[GWSystem.board], 1, 0, 0);
    while (1) {
        MentSleepFrame();
    }
}

void MentPlayMenuIntroAnimation(void)
{
    s32 playMenuIntroAnimationMenuItemIndex;

    MentSetMenuCameraCallback(MentZoomMenuCameraIn);
    MentSleepTicks(0xF0);
    for (playMenuIntroAnimationMenuItemIndex = 0; playMenuIntroAnimationMenuItemIndex < 5; playMenuIntroAnimationMenuItemIndex++) {
        Hu3DModelAttrReset(menuItemWork[playMenuIntroAnimationMenuItemIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    }
    menuObjects[0]->work[0] = 1;
    MentSetMenuCameraCallback(MentZoomAlternateMenuCameraIn);
    MentSleepTicks(0x8C);
}

void MentRevealMenuItems(void)
{
    s32 revealMenuItemsMenuItemIndex;

    MentSleepTicks(0x3C);
    menuSequenceHandles[0] = HuAudSeqPlay(0x30);
    for (revealMenuItemsMenuItemIndex = 0; revealMenuItemsMenuItemIndex < 5; revealMenuItemsMenuItemIndex++) {
        Hu3DModelAttrReset(menuItemWork[revealMenuItemsMenuItemIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    }
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);

    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    MentSleepTicks(0x3C);
}

void MentUpdateMenuCamera(void)
{
    MentDllCameraWork updateMenuCameraCameraTargetState;
    MentDllCameraWork *updateMenuCameraCamera;

    updateMenuCameraCamera = &menuCameraWork;
    updateMenuCameraCameraTargetState.center.x = cameraTargetX;
    updateMenuCameraCameraTargetState.center.y = 60.0f;
    updateMenuCameraCameraTargetState.center.z = 0.0f;
    updateMenuCameraCameraTargetState.rot.x = 0.0f;
    updateMenuCameraCameraTargetState.rot.y = 0.0f;
    updateMenuCameraCameraTargetState.rot.z = 0.0f;
    updateMenuCameraCameraTargetState.zoom = 850.0f;
    MentAnimateCameraState(updateMenuCameraCamera, &updateMenuCameraCameraTargetState, updateMenuCameraCamera->transitionFrame++, 10.0f, 5.0f);
}

s32 playerSelectionMessageWindow = -1;

void MentRunMenuShowcase(void)
{
    s32 runMenuShowcaseMenuItemIndex;
    s32 runMenuShowcaseStateValue;
    s32 runMenuShowcaseSoundEffectId;

    _ClearFlag(2);
    _ClearFlag(3);
    _ClearFlag(4);
    _ClearFlag(5);
    _ClearFlag(6);
    _ClearFlag(7);
    _ClearFlag(1);
    _ClearFlag(9);
    for (runMenuShowcaseMenuItemIndex = 0; runMenuShowcaseMenuItemIndex < 5; runMenuShowcaseMenuItemIndex++) {
        MentSetModelMotionIfChanged(menuItemWork[runMenuShowcaseMenuItemIndex].ownerObject, 1, 6, 5, 1);
    }
    MentSetModelMotion(playerSelectionWork[0].ownerObject, 2, 3, 0, 0);
    particleEffectObject->work[0] = 1;
    runMenuShowcaseSoundEffectId = -1;
    runMenuShowcaseSoundEffectId = HuAudFXPlay(0x43);
    HuAudFXPanning(runMenuShowcaseSoundEffectId, 0x20);
    runMenuShowcaseSoundEffectId = HuAudFXPlay(0x40);
    HuAudFXPanning(runMenuShowcaseSoundEffectId, 0x30);
    runMenuShowcaseSoundEffectId = HuAudFXPlay(0x37);
    HuAudFXPanning(runMenuShowcaseSoundEffectId, 0x40);
    runMenuShowcaseSoundEffectId = HuAudFXPlay(0x4B);
    HuAudFXPanning(runMenuShowcaseSoundEffectId, 0x4C);
    runMenuShowcaseSoundEffectId = HuAudFXPlay(0x46);
    HuAudFXPanning(runMenuShowcaseSoundEffectId, 0x60);
    HuAudFXPlay(0x9A);
    HuAudFXPlay(0x306);
    MentSleepTicks(0x96);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
    MentSleepTicks(0x1E);
    for (runMenuShowcaseMenuItemIndex = 0; runMenuShowcaseMenuItemIndex < 5; runMenuShowcaseMenuItemIndex++) {
        menuItemWork[runMenuShowcaseMenuItemIndex].isVisible = 0;
        menuItemWork[runMenuShowcaseMenuItemIndex].updateCallback = (MentDllUpdateCallback)MentUpdateMenuItemObject;
    }
    menuItemWork->isVisible = 1;
    MentSleepTicks(0x3C);
    for (runMenuShowcaseMenuItemIndex = 1; runMenuShowcaseMenuItemIndex < 5; runMenuShowcaseMenuItemIndex++) {
        MentSetModelMotionIfChanged(menuItemWork[runMenuShowcaseMenuItemIndex].ownerObject, 1, 1, 0x1E, 1);
    }
    runMenuShowcaseStateValue = MentCreateStandardMessageWindow(0, 0, 0);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
    HuWinInsertMesSet(runMenuShowcaseStateValue, recordPlayerWork->character, 0);
    MentSetWindowMessage(runMenuShowcaseStateValue, 0x1E005F, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
    MentCloseMessageWindow(runMenuShowcaseStateValue);
    MentSleepTicks(0x1E);
    for (runMenuShowcaseMenuItemIndex = 1; runMenuShowcaseMenuItemIndex < 5; runMenuShowcaseMenuItemIndex++) {
        menuItemWork[runMenuShowcaseMenuItemIndex].isVisible = 0;
    }
    menuItemWork[3].isVisible = 1;
    menuCameraWork.transitionFrame = 0;
    cameraTargetX = menuCameraPositions[3].x;
    MentSetMenuCameraCallback(MentUpdateMenuCamera);
    MentSleepTicks(0x1E);
    runMenuShowcaseStateValue = MentCreateStandardMessageWindow(0, 0, 0);
    MentSetModelMotionIfChanged(menuItemWork[3].ownerObject, 1, 6, 0xF, 1);
    MentSetWindowMessage(runMenuShowcaseStateValue, 0x1E0060, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[3].ownerObject, 1, 1, 0xF, 1);
    MentCloseMessageWindow(runMenuShowcaseStateValue);
    for (runMenuShowcaseMenuItemIndex = 0; runMenuShowcaseMenuItemIndex < 5; runMenuShowcaseMenuItemIndex++) {
        menuItemWork[runMenuShowcaseMenuItemIndex].isVisible = 0;
    }
    menuItemWork[1].isVisible = 1;
    menuCameraWork.transitionFrame = 0;
    cameraTargetX = menuCameraPositions[1].x;
    MentSleepTicks(0x1E);
    runMenuShowcaseStateValue = MentCreateStandardMessageWindow(0, 0, 0);
    MentSetModelMotionIfChanged(menuItemWork[1].ownerObject, 1, 6, 0xF, 1);
    MentSetWindowMessage(runMenuShowcaseStateValue, 0x1E0061, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[1].ownerObject, 1, 1, 0xF, 1);
    MentCloseMessageWindow(runMenuShowcaseStateValue);
    for (runMenuShowcaseMenuItemIndex = 0; runMenuShowcaseMenuItemIndex < 5; runMenuShowcaseMenuItemIndex++) {
        menuItemWork[runMenuShowcaseMenuItemIndex].isVisible = 0;
    }
    menuItemWork[4].isVisible = 1;
    menuCameraWork.transitionFrame = 0;
    cameraTargetX = menuCameraPositions[4].x;
    MentSleepTicks(0x1E);
    runMenuShowcaseStateValue = MentCreateStandardMessageWindow(0, 0, 0);
    MentSetModelMotionIfChanged(menuItemWork[4].ownerObject, 1, 6, 0xF, 1);
    MentSetWindowMessage(runMenuShowcaseStateValue, 0x1E0062, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[4].ownerObject, 1, 1, 0xF, 1);
    MentCloseMessageWindow(runMenuShowcaseStateValue);
    for (runMenuShowcaseMenuItemIndex = 0; runMenuShowcaseMenuItemIndex < 5; runMenuShowcaseMenuItemIndex++) {
        menuItemWork[runMenuShowcaseMenuItemIndex].isVisible = 0;
    }
    menuItemWork[2].isVisible = 1;
    menuCameraWork.transitionFrame = 0;
    cameraTargetX = menuCameraPositions[2].x;
    MentSleepTicks(0x1E);
    runMenuShowcaseStateValue = MentCreateStandardMessageWindow(0, 0, 0);
    MentSetModelMotionIfChanged(menuItemWork[2].ownerObject, 1, 1, 0xF, 1);
    MentSetWindowMessage(runMenuShowcaseStateValue, 0x1E0063, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[2].ownerObject, 1, 1, 0xF, 1);
    MentCloseMessageWindow(runMenuShowcaseStateValue);
    particleEffectObject->work[0] = 3;
    for (runMenuShowcaseMenuItemIndex = 0; runMenuShowcaseMenuItemIndex < 5; runMenuShowcaseMenuItemIndex++) {
        menuItemWork[runMenuShowcaseMenuItemIndex].isVisible = 0;
    }
    menuItemWork->isVisible = 1;
    MentSetMenuCameraCallback(MentZoomAlternateMenuCameraIn);
    MentSleepTicks(0x3C);
    runMenuShowcaseStateValue = MentCreateStandardMessageWindow(0, 0, 0);
    playerSelectionMessageWindow = runMenuShowcaseStateValue;
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(runMenuShowcaseStateValue, 0x1E0064, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
    for (runMenuShowcaseMenuItemIndex = 0; runMenuShowcaseMenuItemIndex < 5; runMenuShowcaseMenuItemIndex++) {
        menuItemWork[runMenuShowcaseMenuItemIndex].updateCallback = NULL;
    }
}

void MentFinishRecordMenu(OMOBJ *object, void *callbackData, void *callbackContext)
{
    MentCreateMenuOptionWindows();
    recordPlayerWork->updateCallback = NULL;
}

void MentStartRecordMenu(void)
{
    recordPlayerWork[0].updateCallback = (MentDllUpdateCallback)MentFinishRecordMenu;
}

void MentStartMenuMusicAndWipe(void)
{
    MentSleepTicks(0x3C);
    HuAudSStreamPlay(0xC);
    OSReport("########### ME_MainProcFunc000\n");
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
}

void MentShowPlayerSelectionIntro(void)
{
    s32 showPlayerSelectionIntroPlayerIndex;

    MentSleepTicks(0x3C);
    menuSequenceHandles[0] = HuAudSeqPlay(0x30);
    if (menuLaunchParameters[0] == 0) {
        for (showPlayerSelectionIntroPlayerIndex = 0; showPlayerSelectionIntroPlayerIndex < 6; showPlayerSelectionIntroPlayerIndex++) {
            Hu3DModelAttrReset(menuItemWork[showPlayerSelectionIntroPlayerIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
        }
    }
    else {
        for (showPlayerSelectionIntroPlayerIndex = 0; showPlayerSelectionIntroPlayerIndex < 5; showPlayerSelectionIntroPlayerIndex++) {
            Hu3DModelAttrReset(menuItemWork[showPlayerSelectionIntroPlayerIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
        }
    }
    Hu3DModelPosSet(menuItemWork[0].ownerObject->mdlId[1], 0.0f, 0.0f, 560.0f);
    MentSetModelMotion(playerSelectionWork[0].ownerObject, 2, 2, 0, 2);
    Hu3DModelAttrSet(playerSelectionWork[0].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
    OSReport("########### ME_MainProcFunc400\n");
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);

    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    MentSleepTicks(0x3C);
}

void MentAnimatePrimaryMenuItem(void)
{
    s32 animatePrimaryMenuItemMenuItemIndex;

    Vec animatePrimaryMenuItemTargetPosition = { 0.0f, 0.0f, 560.0f };
    MentSetMenuCameraCallback(MentZoomMenuCameraIn);
    MentSleepTicks(0x5A);
    if (menuLaunchParameters[0] == 0) {
        for (animatePrimaryMenuItemMenuItemIndex = 0; animatePrimaryMenuItemMenuItemIndex < 6; animatePrimaryMenuItemMenuItemIndex++) {
            Hu3DModelAttrReset(menuItemWork[animatePrimaryMenuItemMenuItemIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
        }
    }
    else {
        for (animatePrimaryMenuItemMenuItemIndex = 0; animatePrimaryMenuItemMenuItemIndex < 5; animatePrimaryMenuItemMenuItemIndex++) {
            Hu3DModelAttrReset(menuItemWork[animatePrimaryMenuItemMenuItemIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
        }
    }
    MentSetModelMotion(playerSelectionWork[0].ownerObject, 2, 3, 0, 0);
    MentSleepTicks(0x96);
    menuObjects[0]->work[0] = 1;
    MentSetMenuCameraCallback(MentZoomAlternateMenuCameraIn);
    MentSleepTicks(0x5A);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 2, 0xF, 1);
    MentMoveAndTurnModelToTarget(menuItemWork[0].ownerObject, 1, animatePrimaryMenuItemTargetPosition, 0.0f, 3.0f, 0.0f, 1, 0);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
    Hu3DModelAttrSet(playerSelectionWork[0].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
    MentSleepTicks(0x1E);
}

void MentHandleContinueGamePrompt(void)
{
    s32 handleContinueGamePromptStateValue;
    s32 handleContinueGamePromptStateValue2;

    menuSequenceHandles[0] = HuAudSeqPlay(0x30);
    handleContinueGamePromptStateValue = MentCreateStandardMessageWindow(0, 0, 0);
    HuAudFXPlay(menuSoundEffectIds[0][0]);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(handleContinueGamePromptStateValue, 0x1A0000, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
    MentCloseMessageWindow(handleContinueGamePromptStateValue);
    saveExecF = 0;
    if (GWGameStat.party_continue == 1) {
        handleContinueGamePromptStateValue = MentCreateStandardMessageWindow(0, 0, 0);
        while (1) {
            MentSleepFrame();
            MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
            MentSetWindowMessage(handleContinueGamePromptStateValue, 0x1A0001, -1, 5);
            MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
            handleContinueGamePromptStateValue2 = MentShowYesNoPrompt(0x1E0035, 3, 0);
            if (handleContinueGamePromptStateValue2 == -1) {
                MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
                MentSetWindowMessage(handleContinueGamePromptStateValue, 0x1A0003, -1, 5);
                MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
                handleContinueGamePromptStateValue2 = MentShowAlternateYesNoPrompt(0x1E0035, 3, 0);
                if (handleContinueGamePromptStateValue2 == 0) {
                    MentCloseMessageWindow(handleContinueGamePromptStateValue);
                    MentReturnToMenu(0);
                }
                continue;
            }
            if (handleContinueGamePromptStateValue2 == 0) {
                HuAudFXPlay(menuSoundEffectIds[0][3]);
                saveExecF = 1;
                SLLoadBoard();
                HuDataDirClose(DATADIR_MENT);
                HuPrcChildCreate(MentLoadBoardData, 0x64, 0x3000, 0, menuObjectManagerProcess);
                MentCloseMessageWindow(handleContinueGamePromptStateValue);
                MentReturnToMenu(1);
                continue;
            }
            if (handleContinueGamePromptStateValue2 == 1) {
                break;
            }
        }
        MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
        MentSetWindowMessage(handleContinueGamePromptStateValue, 0x1A0002, -1, -1);
        MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
        MentCloseMessageWindow(handleContinueGamePromptStateValue);
    }
}

void MentHandleQuickStartPrompt(void)
{
    s32 handleQuickStartPromptIndex;
    s32 handleQuickStartPromptStateValue;
    s32 handleQuickStartPromptStateValue2;

    handleQuickStartPromptStateValue = MentCreateStandardMessageWindow(0, 0, 0);
    HuAudFXPlay(menuSoundEffectIds[0][3]);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(handleQuickStartPromptStateValue, 0x1A0004, -1, 5);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
    handleQuickStartPromptStateValue2 = MentShowAlternateYesNoPrompt(0x1E0035, 3, 0);
    MentCloseMessageWindow(handleQuickStartPromptStateValue);
    if (handleQuickStartPromptStateValue2 == 0) {
        s32 handleQuickStartPromptStateValue3[4] = { 3, 0, 2, 4 };
        _SetFlag(0x1000B);
        for (handleQuickStartPromptIndex = 0; handleQuickStartPromptIndex < 4; handleQuickStartPromptIndex++) {
            GWPlayerCfg[handleQuickStartPromptIndex].character = handleQuickStartPromptStateValue3[handleQuickStartPromptIndex];
            GWPlayerCfg[handleQuickStartPromptIndex].pad_idx = handleQuickStartPromptIndex;
            GWPlayerCfg[handleQuickStartPromptIndex].diff = 0;
            GWPlayerCfg[handleQuickStartPromptIndex].group = 0;
            GWPlayerCfg[handleQuickStartPromptIndex].iscom = 1;
            OSReport("ID-%d CHR-%d PAD-%d DIF-%d GRP-%d COM-%d\n", handleQuickStartPromptIndex, GWPlayerCfg[handleQuickStartPromptIndex].character, GWPlayerCfg[handleQuickStartPromptIndex].pad_idx,
                GWPlayerCfg[handleQuickStartPromptIndex].diff, GWPlayerCfg[handleQuickStartPromptIndex].group, GWPlayerCfg[handleQuickStartPromptIndex].iscom);
        }
        BoardSaveInit(6);
        GWSystem.max_turn = 0x14;
        HuDataDirClose(DATADIR_MENT);
        HuPrcChildCreate(MentLoadBoardData, 0x64, 0x3000, 0, menuObjectManagerProcess);
        {
            OMOVLHIS *handleQuickStartPromptOverlayHistory = omOvlHisGet(0);
        }
        omOvlHisChg(0, DLL_mentdll, 0, 1);
        MentReturnToMenu(2);
    }
    else {
        _ClearFlag(0x1000B);
    }
    MentInitializePlayerSelectionSpriteGroup();
}

void MentSetSelectedOption(s32 optionIndex)
{
    MentSetMenuCursorPosition(optionIndex);
}

s32 MentRunPlayerCountSelection(void)
{
    s32 runPlayerCountSelectionPlayerIndex;
    s32 runPlayerCountSelectionPlayerCount;
    s32 runPlayerCountSelectionSelectionState;
    s32 runPlayerCountSelectionSelectionState2;
    s32 runPlayerCountSelectionPlayerCount2;
    s32 runPlayerCountSelectionSelectionState3;
    s32 runPlayerCountSelectionSelectionState4;
    s32 runPlayerCountSelectionSelectionState5;
    s32 runPlayerCountSelectionSelectionState6;

    runPlayerCountSelectionPlayerCount = 0;
    runPlayerCountSelectionSelectionState6 = 0;
    runPlayerCountSelectionSelectionState2 = 0;
    if (menuLaunchParameters[0] == 5) {
        runPlayerCountSelectionSelectionState2 = 3;
    }
    runPlayerCountSelectionSelectionState3 = MentCreateStandardMessageWindow(0, 0, 0);
    HuAudFXPlay(menuSoundEffectIds[runPlayerCountSelectionSelectionState2][2]);
    while (1) {
        MentSleepFrame();
        MentSleepFrame();
        MentSetModelMotion(menuItemWork[runPlayerCountSelectionSelectionState2].ownerObject, 1, 4, 0xF, 1);
        MentSetWindowMessage(runPlayerCountSelectionSelectionState3, 0x1A0005, -1, -1);
        MentSetModelMotion(menuItemWork[runPlayerCountSelectionSelectionState2].ownerObject, 1, 1, 0xF, 1);
        MentShowMessageWindow(0x1A0021);
        MentShowMinigamePlayerCountIndicators();
        playerSelectionWork[0].updateCallback = (MentDllUpdateCallback)MentUpdateMenuPlayerSelection;
        runPlayerCountSelectionSelectionState4 = 0x63;
        runPlayerCountSelectionSelectionState6 = 0;
        while (1) {
            MentSleepFrame();
            runPlayerCountSelectionPlayerCount = 0;
            for (runPlayerCountSelectionPlayerIndex = 0; runPlayerCountSelectionPlayerIndex < 4; runPlayerCountSelectionPlayerIndex++) {
                if (recordPlayerWork[runPlayerCountSelectionPlayerIndex].recordFields[3] == 0) {
                    runPlayerCountSelectionPlayerCount++;
                }
            }
            if (runPlayerCountSelectionSelectionState4 != runPlayerCountSelectionPlayerCount) {
                runPlayerCountSelectionSelectionState4 = runPlayerCountSelectionPlayerCount;
                MentSetWindowMessage(runPlayerCountSelectionSelectionState3, runPlayerCountSelectionPlayerCount + 0x1A001B, -1, -0x3E7);
            }
            if (playerSelectionWork[0].selectionFlags[0] != 0) {
                continue;
            }
            if (HuPadBtnDown[recordPlayerWork->controllerPort] & PAD_BUTTON_A) {
                runPlayerCountSelectionSelectionState = 1;
                HuAudFXPlay(2);
                playerSelectionWork[0].updateCallback = NULL;
                MentSleepTicks(2);
                runPlayerCountSelectionPlayerCount2 = 0;
                runPlayerCountSelectionPlayerCount = runPlayerCountSelectionPlayerCount2;
                for (runPlayerCountSelectionPlayerIndex = 0; runPlayerCountSelectionPlayerIndex < 4; runPlayerCountSelectionPlayerIndex++) {
                    if (recordPlayerWork[runPlayerCountSelectionPlayerIndex].isComputerPlayer == 0) {
                        runPlayerCountSelectionPlayerCount2++;
                    }
                }
                for (runPlayerCountSelectionPlayerIndex = 0; runPlayerCountSelectionPlayerIndex < 4; runPlayerCountSelectionPlayerIndex++) {
                    if (runPlayerCountSelectionPlayerCount < runPlayerCountSelectionPlayerCount2) {
                        if (HuPadStatGet(runPlayerCountSelectionPlayerIndex) != -1) {
                            runPlayerCountSelectionPlayerCount++;
                            recordPlayerWork[runPlayerCountSelectionPlayerIndex].isComputerPlayer = 0;
                        }
                        else {
                            recordPlayerWork[runPlayerCountSelectionPlayerIndex].isComputerPlayer = 1;
                        }
                    }
                }
                break;
            }
            else if (HuPadBtnDown[recordPlayerWork->controllerPort] & PAD_BUTTON_B) {
                runPlayerCountSelectionSelectionState = 0;
                HuAudFXPlay(3);
                break;
            }
        }
        MentHideMinigamePlayerCountIndicators();
        playerSelectionWork[0].updateCallback = NULL;
        MentClosePersistentMessageWindow(0);
        if (runPlayerCountSelectionSelectionState == 0) {
            MentSetModelMotionIfChanged(menuItemWork[runPlayerCountSelectionSelectionState2].ownerObject, 1, 4, 0xF, 1);
            MentSetWindowMessage(runPlayerCountSelectionSelectionState3, 0x1A0003, -1, 5);
            MentSetModelMotionIfChanged(menuItemWork[runPlayerCountSelectionSelectionState2].ownerObject, 1, 1, 0xF, 1);
            runPlayerCountSelectionSelectionState5 = MentShowAlternateYesNoPrompt(0x1E0035, 3, 0);
            if (runPlayerCountSelectionSelectionState5 == 0) {
                MentCloseMessageWindow(runPlayerCountSelectionSelectionState3);
                MentReturnToMenu(0);
            }
        }
        else if (runPlayerCountSelectionSelectionState == 1) {
            break;
        }
    }
    MentCloseMessageWindow(runPlayerCountSelectionSelectionState3);
    runPlayerCountSelectionPlayerCount = 0;
    for (runPlayerCountSelectionPlayerIndex = 0; runPlayerCountSelectionPlayerIndex < 4; runPlayerCountSelectionPlayerIndex++) {
        if (recordPlayerWork[runPlayerCountSelectionPlayerIndex].isComputerPlayer == 0) {
            runPlayerCountSelectionPlayerCount++;
        }
    }
    if (runPlayerCountSelectionPlayerCount == 0) {
        runPlayerCountSelectionSelectionState = 2;
    }
    else {
        runPlayerCountSelectionSelectionState = 1;
    }
    return runPlayerCountSelectionSelectionState;
}

void MentClearRecordMenuCallback(void)
{
    s32 clearRecordMenuCallbackPlayerIndex;

    MentUpdateMenuCursor();
    MentCreateRecordSprites();
    for (clearRecordMenuCallbackPlayerIndex = 0; clearRecordMenuCallbackPlayerIndex < 4; clearRecordMenuCallbackPlayerIndex++) {
        recordPlayerWork[clearRecordMenuCallbackPlayerIndex].character = clearRecordMenuCallbackPlayerIndex;
    }
}

void MentFinishBoardSelection(void)
{
    MentAnimateCharacterRecordSpritesIn();
}

s32 MentRunBoardSelection(void)
{
    s32 runBoardSelectionBoardIndex;
    s32 runBoardSelectionSelectionState;
    s32 runBoardSelectionSelectionState2;
    s32 runBoardSelectionSelectionState3;
    s32 runBoardSelectionSelectionState4;
    s32 runBoardSelectionSelectionState5;

    runBoardSelectionSelectionState5 = 0;
    runBoardSelectionSelectionState2 = 0;
    if (menuLaunchParameters[0] == 5) {
        runBoardSelectionSelectionState2 = 3;
    }
    MentCreateRecordSprites();
    runBoardSelectionSelectionState4 = MentCreateStandardMessageWindow(0, 0, 0);
    HuWinPushKeySet(runBoardSelectionSelectionState4, 0x300);
    HuAudFXPlay(menuSoundEffectIds[runBoardSelectionSelectionState2][2]);
    MentSleepFrame();
    MentSetModelMotion(menuItemWork[runBoardSelectionSelectionState2].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(runBoardSelectionSelectionState4, 0x1A0009, -1, -1);
    MentSetModelMotion(menuItemWork[runBoardSelectionSelectionState2].ownerObject, 1, 1, 0xF, 1);
    MentShowMessageWindow(0x1A0020);
    for (runBoardSelectionBoardIndex = 0; runBoardSelectionBoardIndex < 4; runBoardSelectionBoardIndex++) {
        if (recordPlayerWork[runBoardSelectionBoardIndex].isComputerPlayer == 0) {
            recordPlayerWork[runBoardSelectionBoardIndex].updateCallback = (MentDllUpdateCallback)MentUpdateRecordDisplay;
        }
    }
    while (1) {
        MentSleepFrame();
        for (runBoardSelectionBoardIndex = 0; runBoardSelectionBoardIndex < 4; runBoardSelectionBoardIndex++) {
            if ((recordPlayerWork[runBoardSelectionBoardIndex].isComputerPlayer == 0) && (HuPadBtnDown[recordPlayerWork[runBoardSelectionBoardIndex].controllerPort] & PAD_BUTTON_A)) {
                runBoardSelectionSelectionState5 = 1;
            }
        }
        if ((recordPlayerWork[0].recordFields[0] == 0) && (HuPadBtnDown[recordPlayerWork->controllerPort] & PAD_BUTTON_B)) {
            HuAudFXPlay(3);
            runBoardSelectionSelectionState = 0;
            break;
        }
        else {
            for (runBoardSelectionBoardIndex = 0; runBoardSelectionBoardIndex < 4; runBoardSelectionBoardIndex++) {
                if ((recordPlayerWork[runBoardSelectionBoardIndex].isComputerPlayer == 0) && (recordPlayerWork[runBoardSelectionBoardIndex].recordFields[0] != 1)) {
                    break;
                }
            }
            if (runBoardSelectionBoardIndex == 4) {
                runBoardSelectionSelectionState = 2;
                break;
            }
        }
    }
    for (runBoardSelectionBoardIndex = 0; runBoardSelectionBoardIndex < 4; runBoardSelectionBoardIndex++) {
        recordPlayerWork[runBoardSelectionBoardIndex].updateCallback = NULL;
    }
    MentClosePersistentMessageWindow(0);
    MentCloseMessageWindow(runBoardSelectionSelectionState4);
    if (runBoardSelectionSelectionState == 2) {
        runBoardSelectionSelectionState3 = 0;
        for (runBoardSelectionBoardIndex = 0; runBoardSelectionBoardIndex < 4; runBoardSelectionBoardIndex++) {
            if (recordPlayerWork[runBoardSelectionBoardIndex].isComputerPlayer != 0) {
                recordPlayerWork[runBoardSelectionBoardIndex].character = 0;
                runBoardSelectionSelectionState3++;
            }
        }
        if (runBoardSelectionSelectionState3 == 0) {
            runBoardSelectionSelectionState = 3;
        }
        else {
            runBoardSelectionSelectionState = 2;
        }
    }
    else {
        runBoardSelectionSelectionState = 0;
    }
    return runBoardSelectionSelectionState;
}

s32 MentRunGameModeSelection(void)
{
    s32 runGameModeSelectionSelectionValue;
    s32 runGameModeSelectionIndex;
    s32 runGameModeSelectionSelectionState;
    s32 runGameModeSelectionSelectionState2;
    s32 runGameModeSelectionSelectionState3;
    s32 runGameModeSelectionSelectionState4;
    s32 runGameModeSelectionSelectionState5;
    s32 runGameModeSelectionSelectionState6;
    s32 runGameModeSelectionSelectionState7;

    runGameModeSelectionSelectionState = -1;
    runGameModeSelectionSelectionState5 = -1;
    runGameModeSelectionSelectionState7 = -1;
    runGameModeSelectionSelectionValue = 0;
    runGameModeSelectionSelectionState6 = 0;
    if (menuLaunchParameters[0] == 5) {
        runGameModeSelectionSelectionState6 = 3;
    }
    runGameModeSelectionSelectionState4 = MentCreateStandardMessageWindow(0, 0, 0);
    HuAudFXPlay(menuSoundEffectIds[runGameModeSelectionSelectionState6][3]);
    runGameModeSelectionSelectionState3 = 0;
    for (runGameModeSelectionIndex = 0; runGameModeSelectionIndex < 4; runGameModeSelectionIndex++) {
        if (recordPlayerWork[runGameModeSelectionIndex].isComputerPlayer != 0) {
            runGameModeSelectionSelectionState3++;
        }
    }
    HuWinInsertMesSet(runGameModeSelectionSelectionState4, runGameModeSelectionSelectionState3 + 0x1A0023, 1);
    MentSetModelMotionIfChanged(menuItemWork[runGameModeSelectionSelectionState6].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(runGameModeSelectionSelectionState4, 0x1A0007, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[runGameModeSelectionSelectionState6].ownerObject, 1, 1, 0xF, 1);
    for (runGameModeSelectionIndex = 0; runGameModeSelectionIndex < 4; runGameModeSelectionIndex++) {
        if (recordPlayerWork[runGameModeSelectionIndex].isComputerPlayer != 0) {
            recordPlayerWork[runGameModeSelectionIndex].character = recordPlayerWork[runGameModeSelectionIndex].difficulty = 0;
        }
    }
    while (1) {
        MentSleepFrame();
        MentShowMessageWindow(0x1A0020);
        for (runGameModeSelectionIndex = 0; runGameModeSelectionIndex < 4; runGameModeSelectionIndex++) {
            if ((recordPlayerWork[runGameModeSelectionIndex].isComputerPlayer != 0) && (recordPlayerWork[runGameModeSelectionIndex].recordFields[0] == 0)) {
                runGameModeSelectionSelectionState = runGameModeSelectionIndex;
                recordPlayerWork[runGameModeSelectionIndex].recordFields[1] = runGameModeSelectionSelectionState5;
                MentUpdateRecordPlayerSelection(&recordPlayerWork[runGameModeSelectionIndex]);
                recordPlayerWork[runGameModeSelectionIndex].updateCallback = (MentDllUpdateCallback)MentUpdateRecordSpriteAnimation;
                break;
            }
        }
        while (1) {
            MentSleepFrame();
            if ((runGameModeSelectionSelectionState7 != recordPlayerWork[runGameModeSelectionSelectionState].character) && (recordPlayerWork[runGameModeSelectionSelectionState].recordFields[0] == 0)) {
                runGameModeSelectionSelectionState7 = recordPlayerWork[runGameModeSelectionSelectionState].character;
                MentSetWindowMessage(runGameModeSelectionSelectionState4, recordPlayerWork[runGameModeSelectionSelectionState].character + 0x1B0008, -1, -0x3E7);
            }
            if ((HuPadBtnDown[recordPlayerWork->controllerPort] & PAD_BUTTON_B) && (recordPlayerWork[runGameModeSelectionSelectionState].recordFields[0] == 0)) {
                HuAudFXPlay(3);
                if (runGameModeSelectionSelectionState5 == -1) {
                    runGameModeSelectionSelectionState2 = 0;
                }
                else {
                    runGameModeSelectionSelectionState2 = 1;
                }
                break;
            }
            else if (recordPlayerWork[runGameModeSelectionSelectionState].recordFields[0] == 1) {
                for (runGameModeSelectionIndex = 0; runGameModeSelectionIndex < 4; runGameModeSelectionIndex++) {
                    if (recordPlayerWork[runGameModeSelectionIndex].recordFields[0] == 0) {
                        recordPlayerWork[runGameModeSelectionIndex].character = 0;
                    }
                }
                recordPlayerWork[runGameModeSelectionSelectionState].updateCallback = NULL;
                MentSetWindowMessage(runGameModeSelectionSelectionState4, 0x1A0022, -1, -0x3E7);
                MentShowMessageWindow(0x1A0023);
                runGameModeSelectionSelectionState2 = MentGetRecordSpriteIndex(&recordPlayerWork[runGameModeSelectionSelectionState]);
                if (runGameModeSelectionSelectionState2 == 1) {
                    recordPlayerWork[runGameModeSelectionSelectionState].recordFields[2] = 1;
                    runGameModeSelectionSelectionState2 = 4;
                    runGameModeSelectionSelectionState5 = runGameModeSelectionSelectionState;
                }
                else {
                    recordPlayerWork[runGameModeSelectionSelectionState].recordFields[2] = 0;
                    runGameModeSelectionSelectionState2 = 4;
                    recordPlayerWork[runGameModeSelectionSelectionState].recordFields[0] = 0;
                    MentGetPlayerSelectionState(&recordPlayerWork[runGameModeSelectionSelectionState]);
                    MentUpdateRecordPlayerSelection(&recordPlayerWork[runGameModeSelectionSelectionState]);
                }
                break;
            }
        }
        for (runGameModeSelectionIndex = 0; runGameModeSelectionIndex < 4; runGameModeSelectionIndex++) {
            recordPlayerWork[runGameModeSelectionIndex].updateCallback = NULL;
        }
        for (runGameModeSelectionIndex = 0; runGameModeSelectionIndex < 4; runGameModeSelectionIndex++) {
            if (recordPlayerWork[runGameModeSelectionIndex].recordFields[0] == 0) {
                break;
            }
        }
        if (runGameModeSelectionIndex == 4) {
            runGameModeSelectionSelectionState2 = 3;
            break;
        }
        else {
            if (runGameModeSelectionSelectionState2 == 1) {
                MentUpdateRecordPlayerSelection(&recordPlayerWork[runGameModeSelectionSelectionState]);
                MentGetPlayerSelectionState(&recordPlayerWork[runGameModeSelectionSelectionState]);
                MentUpdateRecordPlayerSelection(&recordPlayerWork[recordPlayerWork[runGameModeSelectionSelectionState].recordFields[1]]);
                runGameModeSelectionSelectionState5 = recordPlayerWork[recordPlayerWork[runGameModeSelectionSelectionState].recordFields[1]].recordFields[1];
                recordPlayerWork[recordPlayerWork[runGameModeSelectionSelectionState].recordFields[1]].recordFields[0] = 0;
            }
            else if (runGameModeSelectionSelectionState2 == 4) {
            }
            else if (runGameModeSelectionSelectionState2 == 0) {
                MentGetPlayerSelectionState(&recordPlayerWork[runGameModeSelectionSelectionState]);
                break;
            }
            else {
                break;
            }
        }
        runGameModeSelectionSelectionState7 = -1;
    }
    MentClosePersistentMessageWindow(0);
    MentCloseMessageWindow(runGameModeSelectionSelectionState4);
    if (runGameModeSelectionSelectionState2 == 0) {
        runGameModeSelectionSelectionState3 = 0;
        for (runGameModeSelectionIndex = 0; runGameModeSelectionIndex < 4; runGameModeSelectionIndex++) {
            if (recordPlayerWork[runGameModeSelectionIndex].isComputerPlayer == 0) {
                runGameModeSelectionSelectionState3++;
            }
        }
        if (runGameModeSelectionSelectionState3 != 0) {
            runGameModeSelectionSelectionState2 = 1;
        }
        else {
            runGameModeSelectionSelectionState2 = 0;
        }
    }
    return runGameModeSelectionSelectionState2;
}

void MentUpdateModeSelection(void)
{
    s32 updateModeSelectionIndex;
    s32 updateModeSelectionIndex2;

    MentAnimateCharacterRecordSpritesOut();
    for (updateModeSelectionIndex = 0; updateModeSelectionIndex < 4; updateModeSelectionIndex++) {
        for (updateModeSelectionIndex2 = 0; updateModeSelectionIndex2 < 4; updateModeSelectionIndex2++) {
            if (recordPlayerWork[updateModeSelectionIndex].isComputerPlayer == 1) {
                recordPlayerWork[updateModeSelectionIndex].recordFields[updateModeSelectionIndex2] = 0;
                recordPlayerWork[updateModeSelectionIndex].recordExtras[updateModeSelectionIndex2] = 0;
                MentGetPlayerSelectionState(&recordPlayerWork[updateModeSelectionIndex]);
            }
        }
    }
}

void MentFinishModeSelection(void)
{
    MentCreateRecordSelectionSprites();
}

s32 MentRunModeSelection(void)
{
    s32 runModeSelectionSelectionState;
    s32 runModeSelectionSelectionState2;
    s32 runModeSelectionSelectionState3;
    s32 runModeSelectionSelectionState4;
    s32 runModeSelectionSelectionState5;
    s32 i;

    s32 runModeSelectionSelectionValue[6] = {
        0x00000001,
        0x00000002,
        0x00000000,
        0x00000003,
        0x00000004,
        0x00000005,
    };

    runModeSelectionSelectionState5 = 2;
    runModeSelectionSelectionState2 = MentCreateStandardMessageWindow(0, 0, 0);
    while (1) {
        MentSleepFrame();
        runModeSelectionSelectionState3 = 0;
        menuItemWork->selectionTimer = 1;
        HuAudFXPlay(menuSoundEffectIds[0][3]);
        MentSetModelMotionIfChanged(menuItemWork[runModeSelectionSelectionValue[runModeSelectionSelectionState5]].ownerObject, 1, 4, 0xF, 1);
        MentSetWindowMessage(runModeSelectionSelectionState2, 0x1A0008U, -1, -1);
        MentSetModelMotionIfChanged(menuItemWork[runModeSelectionSelectionValue[runModeSelectionSelectionState5]].ownerObject, 1, 1, 0xF, 1);
        MentShowMessageWindow(0x1A0020U);

        for (i = 0; i < 6; i++) {
            menuItemWork[i].isVisible = 0;
            menuItemWork[i].updateCallback = (void (*)(OMOBJ *, ...))MentUpdateMenuItemObject;
        }

        menuItemWork[runModeSelectionSelectionValue[runModeSelectionSelectionState5]].isVisible = 1;
        playerSelectionWork->updateCallback = (void (*)(OMOBJ *, ...))MentUpdatePlayerSelectionDisplay;
        while (1) {
            MentSleepFrame();
            for (i = 0; i < 6; i++) {
                if (menuItemWork[i].isVisible != 2) {
                    break;
                }
            }

            if (i == 6 && runModeSelectionSelectionState3 >= 0x15) {
                if ((HuPadBtnDown[recordPlayerWork->controllerPort] & 0x200)) {
                    HuAudFXPlay(3);
                    runModeSelectionSelectionState4 = 0;
                    break;
                }
                else if ((HuPadBtnDown[recordPlayerWork->controllerPort] & 0x100)) {
                    HuAudFXPlay(2);
                    runModeSelectionSelectionState4 = 2;
                    break;
                }
                else {
                    if ((HuPadStkX[recordPlayerWork->controllerPort] >= 50) || ((HuPadBtn[recordPlayerWork->controllerPort] & 2))) {
                        if (menuLaunchParameters[4] == 1) {
                            if (++runModeSelectionSelectionState5 > 5) {
                                runModeSelectionSelectionState5 = 5;
                            }
                            else {
                                runModeSelectionSelectionState3 = 0;
                            }
                        }
                        else {
                            if (++runModeSelectionSelectionState5 > 4) {
                                runModeSelectionSelectionState5 = 4;
                            }
                            else {
                                runModeSelectionSelectionState3 = 0;
                            }
                        }
                    }
                    else if ((HuPadStkX[recordPlayerWork->controllerPort] <= -50) || ((HuPadBtn[recordPlayerWork->controllerPort] & 1))) {
                        if (--runModeSelectionSelectionState5 < 0) {
                            runModeSelectionSelectionState5 = 0;
                        }
                        else {
                            runModeSelectionSelectionState3 = 0;
                        }
                    }
                }
            }
            if (runModeSelectionSelectionState3 == 0) {
                if (runModeSelectionSelectionState5 != 5) {
                    MentSetMenuCameraCallback(MentAnimateMenuCameraToFront);
                }
                else {
                    MentSetMenuCameraCallback(MentAnimateMenuCameraToBoard);
                }
                for (i = 0; i < 6; i++) {
                    menuItemWork[i].isVisible = 0;
                }
                menuItemWork[runModeSelectionSelectionValue[runModeSelectionSelectionState5]].isVisible = 1;
            }
            if (runModeSelectionSelectionState3++ == 0x14) {
                runModeSelectionSelectionState3 = 0x15;
                MentAnimateSelectionWindow(runModeSelectionSelectionValue[runModeSelectionSelectionState5]);
                MentSetWindowMessage(runModeSelectionSelectionState2, runModeSelectionSelectionValue[runModeSelectionSelectionState5] + 0x1B0001, -1, -999);
                if ((s32)runModeSelectionSelectionValue[runModeSelectionSelectionState5] != 0) {
                    HuAudFXPlay(menuSoundEffectIds[runModeSelectionSelectionValue[runModeSelectionSelectionState5]][0]);
                }
                else {
                    HuAudFXPlay(menuSoundEffectIds[runModeSelectionSelectionValue[runModeSelectionSelectionState5]][2]);
                }
            }
        }
        MentClosePersistentMessageWindow(0);
        if (runModeSelectionSelectionState4 == 0) {
            MentSetMenuCameraCallback(MentAnimateMenuCameraToFront);
            if (runModeSelectionSelectionState5 != 2) {
                runModeSelectionSelectionState5 = 2;
                for (i = 0; i < 6; i++) {
                    menuItemWork[i].isVisible = 0;
                }

                menuItemWork[runModeSelectionSelectionValue[runModeSelectionSelectionState5]].isVisible = 1;
            }
            MentUpdateSelectionWindow();
            MentSleepTicks(0x1E);
        }
        else if (runModeSelectionSelectionState4 == 1) {
            MentSetMenuCameraCallback(MentAnimateMenuCameraToFront);
            if (runModeSelectionSelectionState5 != 2) {
                runModeSelectionSelectionState5 = 2;
                for (i = 0; i < 6; i++) {
                    menuItemWork[i].isVisible = 0;
                }
                menuItemWork[runModeSelectionSelectionValue[runModeSelectionSelectionState5]].isVisible = 1;
            }
            MentUpdateSelectionWindow();
            MentSleepTicks(0x1E);
            MentSetModelMotionIfChanged(menuItemWork->ownerObject, 1, 4, 0xF, 1);
            MentSetWindowMessage(runModeSelectionSelectionState2, 0x1A0003U, -1, 5);
            MentSetModelMotionIfChanged(menuItemWork->ownerObject, 1, 1, 0xF, 1);
            runModeSelectionSelectionState = MentShowAlternateYesNoPrompt(0x1E0035U, 3, 0);
            if (runModeSelectionSelectionState == 0) {
                MentCloseMessageWindow(runModeSelectionSelectionState2);
                MentReturnToMenu(0);
            }
            continue;
        }
        else if (runModeSelectionSelectionState4 == 2) {
            for (i = 0; i < 6; i++) {
                menuItemWork[i].updateCallback = NULL;
            }
            menuLaunchParameters[2] = runModeSelectionSelectionValue[runModeSelectionSelectionState5];
        }
        else {
            continue;
        }
        break;
    }

    MentCloseMessageWindow(runModeSelectionSelectionState2);

    if (runModeSelectionSelectionState4 == 0) {
        for (i = 0; i < 4; i++) {
            if (recordPlayerWork[i].isComputerPlayer != 0) {
                break;
            }
        }
        if (i != 4) {
            runModeSelectionSelectionState4 = 2;
        }
        else {
            runModeSelectionSelectionState4 = 1;
        }
    }
    else {
        runModeSelectionSelectionState4 = 3;
    }
    return runModeSelectionSelectionState4;
}

void MentFinishBoardSetup(void)
{
    MentUpdateRecordSelectionSprites();
}

void MentRunBoardSetup(void)
{
    s32 runBoardSetupStateValue;

    runBoardSetupStateValue = menuLaunchParameters[2];
    MentUpdateRecordSelectionSprites();
    HuAudFXPlay(menuSoundEffectIds[runBoardSetupStateValue][3]);
    if (runBoardSetupStateValue != 5) {
        Vec runBoardSetupTargetPosition = { -350.0f, 0.0f, 800.0f };
        MentSetMenuCameraCallback(MentAnimateMenuCameraToDefaultView);
        MentSetModelMotion(menuItemWork[runBoardSetupStateValue].ownerObject, 1, 3, 0xF, 1);
        MentMoveAndTurnModelToTarget(menuItemWork[runBoardSetupStateValue].ownerObject, 1, runBoardSetupTargetPosition, 30.0f, 6.0f, 10.0f, 1, 1);
        MentSetModelMotion(menuItemWork[runBoardSetupStateValue].ownerObject, 1, 1, 0xF, 1);
        Hu3DModelAttrReset(playerSelectionWork[0].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
        MentSetModelMotion(playerSelectionWork[0].ownerObject, 2, 2, 0, 0);
        MentWaitForModelMotion(playerSelectionWork[0].ownerObject, 2, 0);
    }
    else {
        Vec runBoardSetupTargetPosition2 = { 850.0f, 0.0f, 800.0f };
        MentSetMenuCameraCallback(MentAnimateMenuCameraAcrossBoard);
        MentSetModelMotion(menuItemWork[runBoardSetupStateValue].ownerObject, 1, 3, 0xF, 1);
        MentMoveAndTurnModelToTarget(menuItemWork[runBoardSetupStateValue].ownerObject, 1, runBoardSetupTargetPosition2, 30.0f, 6.0f, 10.0f, 1, 1);
        MentSetModelMotion(menuItemWork[runBoardSetupStateValue].ownerObject, 1, 1, 0xF, 1);
        Hu3DModelAttrReset(playerSelectionWork[1].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
        MentSetModelMotion(playerSelectionWork[1].ownerObject, 2, 2, 0, 0);
        MentWaitForModelMotion(playerSelectionWork[1].ownerObject, 2, 0);
    }
}

void MentFinishPlayerSetup(void)
{
    MentCreateCharacterSelectionSprites();
}

s32 MentRunPlayerSetup(void)
{
    char runPlayerSetupStateValue[10];
    s32 runPlayerSetupStateValue2;
    s32 runPlayerSetupStateValue3;
    s32 runPlayerSetupStateValue4;
    s32 runPlayerSetupStateValue5;
    s32 runPlayerSetupStateValue6;
    s32 runPlayerSetupStateValue7;

    runPlayerSetupStateValue6 = -1;
    runPlayerSetupStateValue7 = -1;
    runPlayerSetupStateValue3 = menuLaunchParameters[2];
    runPlayerSetupStateValue2 = MentCreateStandardMessageWindow(0, 0, 0);
    if (runPlayerSetupStateValue3 == 0) {
        HuAudFXPlay(menuSoundEffectIds[runPlayerSetupStateValue3][2]);
    }
    else {
        HuAudFXPlay(menuSoundEffectIds[runPlayerSetupStateValue3][0]);
    }
loop3:
    MentSleepFrame();
    runPlayerSetupStateValue6 = runPlayerSetupStateValue7 = -1;
    MentSetModelMotionIfChanged(menuItemWork[runPlayerSetupStateValue3].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(runPlayerSetupStateValue2, runPlayerSetupStateValue3 + 0x1A000A, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[runPlayerSetupStateValue3].ownerObject, 1, 1, 0xF, 1);
    MentShowMessageWindow(0x1A0021);
    playerSelectionWork[0].playerSelections[1] = 0;
    MentCreatePlayerMenuWindows();
    playerSelectionWork[1].updateCallback = (MentDllUpdateCallback)MentUpdatePlayerSelectionCursor;
loop4:
    MentSleepFrame();
    if ((runPlayerSetupStateValue6 != playerSelectionWork[1].playerSelections[0]) || runPlayerSetupStateValue7 != playerSelectionWork[1].selectionFlags[playerSelectionWork[1].playerSelections[0]]) {
        runPlayerSetupStateValue6 = playerSelectionWork[1].playerSelections[0];
        runPlayerSetupStateValue7 = playerSelectionWork[1].selectionFlags[playerSelectionWork[1].playerSelections[0]];
        switch (playerSelectionWork[1].playerSelections[0]) {
            case 0:
                switch (playerSelectionWork[1].selectionFlags[0]) {
                    case 0:
                        MentSetWindowMessage(runPlayerSetupStateValue2, 0x1B0015, -1, -0x3E7);
                        break;
                    case 1:
                        HuWinInsertMesSet(runPlayerSetupStateValue2, 0x1B001E, 0);
                        HuWinInsertMesSet(runPlayerSetupStateValue2, 0x1B001F, 1);
                        HuWinInsertMesSet(runPlayerSetupStateValue2, 0x1B0020, 2);
                        MentSetWindowMessage(runPlayerSetupStateValue2, 0x1B0016, -1, -0x3E7);
                        break;
                    case 2:
                        HuWinInsertMesSet(runPlayerSetupStateValue2, 0x1B001F, 0);
                        HuWinInsertMesSet(runPlayerSetupStateValue2, 0x1B001E, 1);
                        HuWinInsertMesSet(runPlayerSetupStateValue2, 0x1B0020, 2);
                        MentSetWindowMessage(runPlayerSetupStateValue2, 0x1B0016, -1, -0x3E7);
                        break;
                    case 3:
                        HuWinInsertMesSet(runPlayerSetupStateValue2, 0x1B0020, 0);
                        HuWinInsertMesSet(runPlayerSetupStateValue2, 0x1B001E, 1);
                        HuWinInsertMesSet(runPlayerSetupStateValue2, 0x1B001F, 2);
                        MentSetWindowMessage(runPlayerSetupStateValue2, 0x1B0016, -1, -0x3E7);
                        break;
                }
                break;
            case 1:
                sprintf(runPlayerSetupStateValue, "%d", playerSelectionWork[1].selectionFlags[1]);
                HuWinInsertMesSet(runPlayerSetupStateValue2, MAKE_MESSID_PTR(runPlayerSetupStateValue), 0);
                MentSetWindowMessage(runPlayerSetupStateValue2, 0x1B0017, -1, -0x3E7);
                break;
            case 2:
                MentSetWindowMessage(runPlayerSetupStateValue2, playerSelectionWork[1].selectionFlags[2] + 0x1B0018, -1, -0x3E7);
                break;
            case 3:
                MentSetWindowMessage(runPlayerSetupStateValue2, playerSelectionWork[1].selectionFlags[3] + 0x1B001B, -1, -0x3E7);
                break;
            case 4:
                HuWinInsertMesSet(runPlayerSetupStateValue2, recordPlayerWork[playerSelectionWork[1].selectionFlags[4]].character, 0);
                MentSetWindowMessage(runPlayerSetupStateValue2, 0x1B001D, -1, -0x3E7);
                break;
        }
    }
    if (playerSelectionWork[0].playerSelections[3] != 0) {
        goto loop4;
    }
    if (playerSelectionWork[0].playerSelections[1] == -1) {
        MentUpdatePlayerMenuWindows();
        runPlayerSetupStateValue4 = 0;
    }
    else if (playerSelectionWork[0].playerSelections[1] == 1) {
        MentUpdatePlayerMenuWindows();
        runPlayerSetupStateValue4 = 2;
    }
    else {
        goto loop4;
    }
    MentClosePersistentMessageWindow(0);
    playerSelectionWork[1].updateCallback = NULL;
    MentUpdatePlayerMenuWindows();
    if (runPlayerSetupStateValue4 == 0) {
        runPlayerSetupStateValue4 = 1;
    }
    else if (runPlayerSetupStateValue4 == 1) {
        MentSetModelMotionIfChanged(menuItemWork[runPlayerSetupStateValue3].ownerObject, 1, 4, 0xF, 1);
        MentSetWindowMessage(runPlayerSetupStateValue2, 0x1A0003, -1, 5);
        MentSetModelMotionIfChanged(menuItemWork[runPlayerSetupStateValue3].ownerObject, 1, 1, 0xF, 1);
        runPlayerSetupStateValue5 = MentShowAlternateYesNoPrompt(0x1E0035, 3, 0);
        if (runPlayerSetupStateValue5 == 0) {
            MentCloseMessageWindow(runPlayerSetupStateValue2);
            MentReturnToMenu(0);
        }
        goto loop3;
    }
    else if (runPlayerSetupStateValue4 == 2) {
        HuAudFXPlay(menuSoundEffectIds[runPlayerSetupStateValue3][2]);
        MentSetWindowMessage(runPlayerSetupStateValue2, 0x1B0000, -1, 5);
        runPlayerSetupStateValue5 = MentShowYesNoPrompt(0x1E0035, 3, 0);
        if (runPlayerSetupStateValue5 == 0) {
            runPlayerSetupStateValue4 = 0x63;
        }
        else {
            MentSetWindowMessage(runPlayerSetupStateValue2, 0x1B0007, -1, 5);
            runPlayerSetupStateValue5 = MentShowAlternateYesNoPrompt(0x1E0035, 3, 0);
            if (runPlayerSetupStateValue5 == 0) {
                runPlayerSetupStateValue4 = 0;
            }
            else {
                goto loop3;
            }
        }
    }
    else {
        goto loop3;
    }
    MentCloseMessageWindow(runPlayerSetupStateValue2);
    return runPlayerSetupStateValue4;
}

void MentFinishCharacterSelection(void)
{
    MentUpdateCharacterSelectionSprites();
}

void MentRunCharacterSelection(void)
{
    s32 runCharacterSelectionSelectionState;
    s32 runCharacterSelectionSelectionState2;
    s32 runCharacterSelectionIndex;
    s32 runCharacterSelectionSelectionState3;

    runCharacterSelectionSelectionState2 = menuLaunchParameters[2];
    {
        Vec runCharacterSelectionTargetPosition = { -120.0f, 0.0f, 670.0f };
        if (menuLaunchParameters[2] == 5) {
            runCharacterSelectionSelectionState = 1;
        }
        else {
            runCharacterSelectionSelectionState = 0;
        }
        for (runCharacterSelectionIndex = 0; runCharacterSelectionIndex < 6; runCharacterSelectionIndex++) {
            Hu3DModelAttrSet(menuItemWork[runCharacterSelectionIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
        }
        Hu3DModelAttrReset(menuItemWork[runCharacterSelectionSelectionState2].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
        MentCreateMenuItemModel(playerSelectionWork[runCharacterSelectionSelectionState].ownerObject, menuLaunchParameters[2]);
        Hu3DModelAttrReset(playerSelectionWork[runCharacterSelectionSelectionState].ownerObject->mdlId[4], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(playerSelectionWork[runCharacterSelectionSelectionState].ownerObject->mdlId[5], HU3D_ATTR_DISPOFF);
        playerSelectionWork[runCharacterSelectionSelectionState].updateCallback = (MentDllUpdateCallback)MentUpdateMenuItemAnimation;
        if (runCharacterSelectionSelectionState2 == 5) {
            runCharacterSelectionTargetPosition.x += 1200.0f;
        }
        MentSetModelMotion(menuItemWork[runCharacterSelectionSelectionState2].ownerObject, 1, 3, 0xF, 1);
        MentMoveAndTurnModelToTarget(menuItemWork[runCharacterSelectionSelectionState2].ownerObject, 1, runCharacterSelectionTargetPosition, 5.0f, 6.0f, 10.0f, 1, 1);
        MentSetModelMotion(menuItemWork[runCharacterSelectionSelectionState2].ownerObject, 1, 1, 0xF, 1);
        HuDataDirClose(DATADIR_MENT);
        HuPrcChildCreate(MentLoadBoardData, 0x64, 0x3000, 0, menuObjectManagerProcess);
        MentSleepTicks(0x1E);
        MentSetModelMotion(playerSelectionWork[runCharacterSelectionSelectionState].ownerObject, 2, 3, 0, 0);
        runCharacterSelectionSelectionState3 = MentCreateStandardMessageWindow(0, 0, 0);
        HuAudFXPlay(menuSoundEffectIds[runCharacterSelectionSelectionState2][0]);
        MentSetModelMotionIfChanged(menuItemWork[runCharacterSelectionSelectionState2].ownerObject, 1, 5, 0xF, 1);
        MentSetWindowMessage(runCharacterSelectionSelectionState3, runCharacterSelectionSelectionState2 + 0x1A0010, -1, -1);
        MentCloseMessageWindow(runCharacterSelectionSelectionState3);
        HuAudSeqFadeOut(menuSequenceHandles[0], 0xBB8);
        MentWaitForModelMotion(playerSelectionWork[runCharacterSelectionSelectionState].ownerObject, 2, 0);
        Hu3DModelAttrSet(playerSelectionWork[runCharacterSelectionSelectionState].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
        MentSetMenuCameraCallback(MentAnimateMenuCameraAroundPlayer);
        MentSleepTicks(0xA);
        HuAudFXPlay(GWPlayerCfg->character + 0x75);
        HuAudFXPlay(GWPlayerCfg[1].character + 0x75);
        HuAudFXPlay(GWPlayerCfg[2].character + 0x75);
        HuAudFXPlay(GWPlayerCfg[3].character + 0x75);
        MentSleepTicks(0x8C);
    }
}

void MentUpdateCharacterSelection(void)
{
    s32 updateCharacterSelectionIndex;
    s32 updateCharacterSelectionSelectionState;
    s32 updateCharacterSelectionSelectionState2;
    s32 updateCharacterSelectionSelectionState3;

    s32 updateCharacterSelectionSelectionValue[6] = { 1, 2, 0, 3, 4, 5 };
    updateCharacterSelectionSelectionState = menuLaunchParameters[2];
    updateCharacterSelectionSelectionState2 = menuLaunchParameters[2];
    if (updateCharacterSelectionSelectionState != 5) {
        updateCharacterSelectionSelectionState3 = 0;
    }
    else {
        updateCharacterSelectionSelectionState3 = 1;
    }
    MentSetModelMotion(playerSelectionWork[updateCharacterSelectionSelectionState3].ownerObject, 2, 3, 0, 0);
    MentWaitForModelMotion(playerSelectionWork[updateCharacterSelectionSelectionState3].ownerObject, 2, 0);
    Hu3DModelAttrSet(playerSelectionWork[0].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(playerSelectionWork[1].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
    if (updateCharacterSelectionSelectionState != 5) {
        MentSetMenuCameraCallback(MentFocusMenuCameraOnPlayer);
    }
    else {
        MentSetMenuCameraCallback(MentAnimateMenuCameraReset);
    }
    {
        Vec updateCharacterSelectionTargetPosition = { 0.0f, 0.0f, 560.0f };
        updateCharacterSelectionTargetPosition.x = menuCameraPositions[updateCharacterSelectionSelectionState].x;
        MentSetModelMotion(menuItemWork[updateCharacterSelectionSelectionState].ownerObject, 1, 3, 0xF, 1);
        MentMoveAndTurnModelToTarget(menuItemWork[updateCharacterSelectionSelectionState].ownerObject, 1, updateCharacterSelectionTargetPosition, menuCameraYawAngles[updateCharacterSelectionSelectionState], 6.0f, 10.0f, 1, 1);
        MentSetModelMotion(menuItemWork[updateCharacterSelectionSelectionState].ownerObject, 1, 1, 0xF, 1);
        for (updateCharacterSelectionIndex = 0; updateCharacterSelectionIndex < 6; updateCharacterSelectionIndex++) {
            menuItemWork[updateCharacterSelectionIndex].isVisible = 0;
            menuItemWork[updateCharacterSelectionIndex].updateCallback = (MentDllUpdateCallback)MentUpdateMenuItemObject;
        }
        menuItemWork[updateCharacterSelectionSelectionValue[updateCharacterSelectionSelectionState2]].isVisible = 1;
        MentSetMenuCameraCallback(MentAnimateMenuCameraToFront);
        if (updateCharacterSelectionSelectionState2 != 2) {
            updateCharacterSelectionSelectionState2 = 2;
            for (updateCharacterSelectionIndex = 0; updateCharacterSelectionIndex < 6; updateCharacterSelectionIndex++) {
                menuItemWork[updateCharacterSelectionIndex].isVisible = 0;
            }
            menuItemWork[updateCharacterSelectionSelectionValue[updateCharacterSelectionSelectionState2]].isVisible = 1;
        }
        MentUpdateSelectionWindow();
        MentSleepTicks(0x1E);
    }
}

void MentShowSelectedCharacter(void)
{
    s32 showSelectedCharacterStateValue;
    s32 showSelectedCharacterStateValue2;
    s32 showSelectedCharacterTargetPosition2;
    s32 showSelectedCharacterStateValue4;
    s32 showSelectedCharacterStateValue5;
    float showSelectedCharacterCoordinateValue;
    s32 showSelectedCharacterStateValue7;
    s32 showSelectedCharacterStateValue8;
    s32 showSelectedCharacterStateValue9;
    s32 showSelectedCharacterIndex;
    MentDllRecordPlayerWork *showSelectedCharacterRecordPlayer;
    s32 showSelectedCharacterIndex2;
    s32 showSelectedCharacterStateValue10;
    s32 showSelectedCharacterStateValue11;
    MentDllRecordPlayerWork *showSelectedCharacterRecordPlayer2;
    MentDllMenuItemWork *showSelectedCharacterMenuItem;
    MentDllPlayerSelectionWork *showSelectedCharacterPlayerSelection;

    showSelectedCharacterStateValue10 = menuLaunchParameters[2];
    if (showSelectedCharacterStateValue10 != 5) {
        showSelectedCharacterStateValue11 = 0;
    }
    else {
        showSelectedCharacterStateValue11 = 1;
    }
    for (showSelectedCharacterIndex = 0; showSelectedCharacterIndex < 6; showSelectedCharacterIndex++) {
        if (showSelectedCharacterIndex != showSelectedCharacterStateValue10) {
            Hu3DModelAttrSet(menuItemWork[showSelectedCharacterIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
        }
    }
    MentSetModelMotion(playerSelectionWork[showSelectedCharacterStateValue11].ownerObject, 2, 3, 0, 0);
    MentWaitForModelMotion(playerSelectionWork[showSelectedCharacterStateValue11].ownerObject, 2, 0);
    if (showSelectedCharacterStateValue10 != 5) {
        MentSetMenuCameraCallback(MentFocusMenuCameraOnPlayer);
    }
    else {
        MentSetMenuCameraCallback(MentAnimateMenuCameraReset);
    }
    {
        Vec showSelectedCharacterTargetPosition = { 0.0f, 0.0f, 460.0f };

        if (showSelectedCharacterStateValue10 == 5) {
            showSelectedCharacterTargetPosition.x = 1200.0f;
        }
        MentSetModelMotion(menuItemWork[showSelectedCharacterStateValue10].ownerObject, 1, 3, 0xF, 1);
        MentMoveAndTurnModelToTarget(menuItemWork[showSelectedCharacterStateValue10].ownerObject, 1, showSelectedCharacterTargetPosition, 0.0f, 6.0f, 10.0f, 1, 1);
        MentSetModelMotion(menuItemWork[showSelectedCharacterStateValue10].ownerObject, 1, 1, 0xF, 1);
        MentSetModelMotion(playerSelectionWork[showSelectedCharacterStateValue11].ownerObject, 2, 2, 0, 0);
        MentWaitForModelMotion(playerSelectionWork[showSelectedCharacterStateValue11].ownerObject, 2, 0);
        MentSetModelMotion(playerSelectionWork[0].ownerObject, 2, 3, 0, 2);
        Hu3DModelAttrReset(playerSelectionWork[0].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
        if (showSelectedCharacterStateValue10 == 5) {
            MentSetMenuCameraCallback(MentAnimateMenuCameraToFront);
        }
        MentSleepTicks(0x1E);
        for (showSelectedCharacterIndex = 0; showSelectedCharacterIndex < 4; showSelectedCharacterIndex++) {
            showSelectedCharacterRecordPlayer = &recordPlayerWork[showSelectedCharacterIndex];
            showSelectedCharacterRecordPlayer2 = &recordPlayerWork[showSelectedCharacterIndex];
            showSelectedCharacterRecordPlayer->recordMinutes = showSelectedCharacterIndex;
            showSelectedCharacterRecordPlayer->recordSeconds = 0;
            showSelectedCharacterRecordPlayer->isComputerPlayer = showSelectedCharacterRecordPlayer->recordMinutes;
            if (showSelectedCharacterRecordPlayer->isComputerPlayer > 0) {
                showSelectedCharacterRecordPlayer->isComputerPlayer = 1;
            }
            showSelectedCharacterRecordPlayer->difficulty = 0;
            showSelectedCharacterRecordPlayer->character = showSelectedCharacterRecordPlayer->recordMinutes;
            showSelectedCharacterRecordPlayer->controllerPort = showSelectedCharacterRecordPlayer->recordMinutes;
            for (showSelectedCharacterIndex2 = 0; showSelectedCharacterIndex2 < 4; showSelectedCharacterIndex2++) {
                showSelectedCharacterRecordPlayer->recordFields[0] = showSelectedCharacterRecordPlayer->recordFields[1] = 0;
            }
            for (showSelectedCharacterIndex2 = 0; showSelectedCharacterIndex2 < 5; showSelectedCharacterIndex2++) {
                showSelectedCharacterRecordPlayer2->state = showSelectedCharacterRecordPlayer2->stateTimer = 0;
            }
        }
        for (showSelectedCharacterIndex = 0; showSelectedCharacterIndex < 6; showSelectedCharacterIndex++) {
            showSelectedCharacterMenuItem = &menuItemWork[showSelectedCharacterIndex];
            for (showSelectedCharacterIndex2 = 0; showSelectedCharacterIndex2 < 5; showSelectedCharacterIndex2++) {
                showSelectedCharacterMenuItem->isVisible = showSelectedCharacterMenuItem->state = 0;
            }
            Hu3DModelPosSet(menuItemWork[showSelectedCharacterIndex].ownerObject->mdlId[1], menuCameraPositions[showSelectedCharacterIndex].x, menuCameraPositions[showSelectedCharacterIndex].y, menuCameraPositions[showSelectedCharacterIndex].z);
            Hu3DModelRotSet(menuItemWork[showSelectedCharacterIndex].ownerObject->mdlId[1], 0.0f, menuCameraYawAngles[showSelectedCharacterIndex], 0.0f);
            Hu3DModelAttrReset(menuItemWork[showSelectedCharacterIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
        }
        for (showSelectedCharacterIndex = 0; showSelectedCharacterIndex < 2; showSelectedCharacterIndex++) {
            showSelectedCharacterPlayerSelection = &playerSelectionWork[showSelectedCharacterIndex];
            for (showSelectedCharacterIndex2 = 0; showSelectedCharacterIndex2 < 5; showSelectedCharacterIndex2++) {
                showSelectedCharacterPlayerSelection->playerSelections[0] = showSelectedCharacterPlayerSelection->playerSelections[1] = 0;
            }
            Hu3DModelAttrSet(playerSelectionWork[1].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
        }
        MentInitializePlayerSelectionSpriteGroup();
        MentUpdateRecordSelection();
        MentCreatePlayerSelectionSprites();
        MentCreatePlayerSelectionWindows();
        MentSetModelMotion(playerSelectionWork[0].ownerObject, 2, 3, 0, 0);
        MentWaitForModelMotion(playerSelectionWork[0].ownerObject, 2, 0);
        Hu3DModelAttrSet(playerSelectionWork[0].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
        {
            Vec showSelectedCharacterTargetPosition2 = { 0.0f, 0.0f, 560.0f };
            MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 2, 0xF, 1);
            MentMoveAndTurnModelToTarget(menuItemWork[0].ownerObject, 1, showSelectedCharacterTargetPosition2, 0.0f, 3.0f, 0.0f, 1, 0);
            MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
        }
    }
}

void MentUpdatePlayerSelectionObject(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork)
{
    s32 updatePlayerSelectionObjectPlayerIndex;
    s32 updatePlayerSelectionObjectSelectionState;
    s32 updatePlayerSelectionObjectSelectionState2;
    s32 updatePlayerSelectionObjectSelectionState3;
    s32 updatePlayerSelectionObjectSelectionState4;
    s32 updatePlayerSelectionObjectSelectionState5;

    updatePlayerSelectionObjectSelectionState = 0xA;
    updatePlayerSelectionObjectSelectionState2 = 0;
    if (menuLaunchParameters[1] == 0) {
        MentStartMenuMusicAndWipe();
        MentAnimatePrimaryMenuItem();
        MentHandleContinueGamePrompt();
        MentHandleQuickStartPrompt();
    }
    else {
        MentShowPlayerSelectionIntro();
        MentHandleQuickStartPrompt();
    }
    do {
        MentSleepFrame();
        switch (updatePlayerSelectionObjectSelectionState) {
            case 10:
                updatePlayerSelectionObjectSelectionState = 0;
                MentSetMenuCursorPosition(updatePlayerSelectionObjectSelectionState2);
                updatePlayerSelectionObjectSelectionState2 = MentRunPlayerCountSelection();
                MentClearRecordMenuCallback();
                break;
            case 0:
                updatePlayerSelectionObjectSelectionState = 1;
                MentAnimateCharacterRecordSpritesIn();
                while (1) {
                    MentSleepFrame();
                    switch (updatePlayerSelectionObjectSelectionState2) {
                        case 1:
                            updatePlayerSelectionObjectSelectionState2 = MentRunBoardSelection();
                            break;
                        case 2:
                            updatePlayerSelectionObjectSelectionState2 = MentRunGameModeSelection();
                            break;
                    }
                    if (updatePlayerSelectionObjectSelectionState2 == 0) {
                        updatePlayerSelectionObjectSelectionState = 0xA;
                        break;
                    }
                    else if (updatePlayerSelectionObjectSelectionState2 == 3) {
                        break;
                    }
                }
                MentUpdateModeSelection();
                break;
            case 1:
                updatePlayerSelectionObjectSelectionState = 2;
                MentCreateRecordSelectionSprites();
                updatePlayerSelectionObjectSelectionState2 = MentRunModeSelection();
                if (updatePlayerSelectionObjectSelectionState2 != 3) {
                    MentUpdateRecordSelectionSprites();
                    updatePlayerSelectionObjectSelectionState = 0;
                }
                else {
                    MentRunBoardSetup();
                }
                break;
            case 2:
                MentCreateCharacterSelectionSprites();
                updatePlayerSelectionObjectSelectionState = MentRunPlayerSetup();
                MentUpdateCharacterSelectionSprites();
                switch (updatePlayerSelectionObjectSelectionState) {
                    case 0:
                        updatePlayerSelectionObjectSelectionState = 0xA;
                        updatePlayerSelectionObjectSelectionState2 = 0;
                        MentShowSelectedCharacter();
                        break;
                    case 1:
                        updatePlayerSelectionObjectSelectionState = 1;
                        MentUpdateCharacterSelection();
                        break;
                }
                break;
        }
    } while (updatePlayerSelectionObjectSelectionState != 0x63);
    if (playerSelectionWork[1].selectionFlags[0] == 0) {
        updatePlayerSelectionObjectSelectionState4 = 0;
    }
    else {
        updatePlayerSelectionObjectSelectionState4 = 1;
    }
    if (playerSelectionWork[1].selectionFlags[3] == 0) {
        updatePlayerSelectionObjectSelectionState5 = 1;
    }
    else {
        updatePlayerSelectionObjectSelectionState5 = 0;
    }
    switch (playerSelectionWork[1].selectionFlags[2]) {
        case 0:
            updatePlayerSelectionObjectSelectionState3 = 0;
            break;
        case 1:
            updatePlayerSelectionObjectSelectionState3 = 1;
            break;
        case 2:
            updatePlayerSelectionObjectSelectionState3 = 2;
            break;
        default:
            updatePlayerSelectionObjectSelectionState3 = 0;
            break;
    }
    {
        s32 updatePlayerSelectionObjectSelectionState6 = playerSelectionWork[1].selectionFlags[1];
        s32 updatePlayerSelectionObjectSelectionState7 = playerSelectionWork[1].playerSelections[1];
        s32 updatePlayerSelectionObjectSelectionState8 = playerSelectionWork[1].playerSelections[2];
        s32 updatePlayerSelectionObjectSelectionValue = playerSelectionWork[1].playerSelections[3];
        s32 updatePlayerSelectionObjectSelectionValue2 = playerSelectionWork[1].playerSelections[4];
        BoardPartyConfigSet(updatePlayerSelectionObjectSelectionState4, updatePlayerSelectionObjectSelectionState5, updatePlayerSelectionObjectSelectionState3, updatePlayerSelectionObjectSelectionState6, updatePlayerSelectionObjectSelectionState7, updatePlayerSelectionObjectSelectionState8, updatePlayerSelectionObjectSelectionValue, updatePlayerSelectionObjectSelectionValue2);
    }
    for (updatePlayerSelectionObjectPlayerIndex = 0; updatePlayerSelectionObjectPlayerIndex < 4; updatePlayerSelectionObjectPlayerIndex++) {
        GWPlayerCfg[updatePlayerSelectionObjectPlayerIndex].character = recordPlayerWork[updatePlayerSelectionObjectPlayerIndex].character;
        GWPlayerCfg[updatePlayerSelectionObjectPlayerIndex].pad_idx = recordPlayerWork[updatePlayerSelectionObjectPlayerIndex].controllerPort;
        GWPlayerCfg[updatePlayerSelectionObjectPlayerIndex].diff = recordPlayerWork[updatePlayerSelectionObjectPlayerIndex].difficulty;
        GWPlayerCfg[updatePlayerSelectionObjectPlayerIndex].group = 0;
        GWPlayerCfg[updatePlayerSelectionObjectPlayerIndex].iscom = recordPlayerWork[updatePlayerSelectionObjectPlayerIndex].isComputerPlayer;
    }
    if (updatePlayerSelectionObjectSelectionState4 == 1) {
        switch (playerSelectionWork[1].selectionFlags[0]) {
            case 1:
                GWPlayerCfg[2].group = GWPlayerCfg[3].group = 1;
                break;
            case 2:
                GWPlayerCfg[1].group = GWPlayerCfg[3].group = 1;
                break;
            case 3:
                GWPlayerCfg[1].group = GWPlayerCfg[2].group = 1;
                break;
            default:
                GWPlayerCfg[2].group = GWPlayerCfg[3].group = 1;
                break;
        }
    }
    BoardSaveInit(menuLaunchParameters[2]);
    MentRunCharacterSelection();
    if (menuLaunchParameters[2] != 5) {
        WipeColorSet(0xFF, 0xFF, 0xFF);
    }
    else {
        WipeColorSet(0, 0, 0);
    }
    MentReturnToMenu(3);
}

void MentReturnToMenu(s32 menuMode)
{
    s32 returnToMenuMenuValue[7] = { 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F };
    if (menuMode == 0 || menuMode == 1 || menuMode == 2) {
        MentSleepTicks(0x3C);
        HuAudSeqFadeOut(menuSequenceHandles[0], 0x3E8);
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);

    while (WipeStatGet() != 0) {
        MentSleepFrame();
    }
    CharModelKill(-1);
    MGSeqKillAll();

    if (menuMode != 0) {
        if (menuMode == 1 || menuMode == 3) {
            OMOVLHIS *returnToMenuOverlayHistory = omOvlHisGet(0);
            omOvlHisChg(0, DLL_mstory3dll, 0, 0);
        }
        do {
            MentSleepFrame();
        } while (menuTransitionComplete != 1);
        CharMotionInit(GWPlayerCfg[0].character);
        CharMotionInit(GWPlayerCfg[1].character);
        CharMotionInit(GWPlayerCfg[2].character);
        CharMotionInit(GWPlayerCfg[3].character);
        HuAudSeqAllFadeOut(0x3E8);
        HuAudSStreamAllFadeOut(0x3E8);
    }

    switch (menuMode) {
        case 0:
            omOvlReturnEx(1, 1);
            break;
        case 2:
            omOvlCallEx(returnToMenuMenuValue[6], 1, 0, 0);
            break;
        case 1:
        case 3:
            omOvlCallEx(returnToMenuMenuValue[GWSystem.board], 1, 0, 0);
            break;
    }
    while (1) {
        MentSleepFrame();
    }
}

s32 MentIsMenuOptionUnlocked(s32 optionIndex)
{
    if (optionIndex == 0 && _CheckFlag(FLAG_ID_MAKE(0, 2)) != 0) {
        return 1;
    }
    if (optionIndex == 1 && _CheckFlag(FLAG_ID_MAKE(0, 3)) != 0) {
        return 1;
    }
    if (optionIndex == 2 && _CheckFlag(FLAG_ID_MAKE(0, 4)) != 0) {
        return 1;
    }
    if (optionIndex == 3 && _CheckFlag(FLAG_ID_MAKE(0, 5)) != 0) {
        return 1;
    }
    if (optionIndex == 4 && _CheckFlag(FLAG_ID_MAKE(0, 6)) != 0) {
        return 1;
    }
    if (optionIndex == 5 && _CheckFlag(FLAG_ID_MAKE(0, 7)) != 0) {
        return 1;
    }
    return 0;
}

s32 MentAreAllBoardsUnlocked(void)
{
    if ((_CheckFlag(FLAG_ID_MAKE(0, 2)) != 0) && (_CheckFlag(FLAG_ID_MAKE(0, 3)) != 0) && (_CheckFlag(FLAG_ID_MAKE(0, 4)) != 0)
        && (_CheckFlag(FLAG_ID_MAKE(0, 5)) != 0) && (_CheckFlag(FLAG_ID_MAKE(0, 6)) != 0)) {
        return 1;
    }
    return 0;
}

void MentRunOptionSelection(void)
{
    s32 runOptionSelectionSelectionState;
    s32 runOptionSelectionSelectionState2;

    runOptionSelectionSelectionState = MentCreateStandardMessageWindow(0, 0, 0);
    menuSequenceHandles[0] = HuAudSeqPlay(0x30);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(runOptionSelectionSelectionState, 0x1E005C, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
    MentCloseMessageWindow(runOptionSelectionSelectionState);
    saveExecF = 0;
    if (GWGameStat.story_continue != 1) {
        return;
    }
    runOptionSelectionSelectionState = MentCreateStandardMessageWindow(0, 0, 0);
    while (1) {
        MentSleepFrame();
        MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
        MentSetWindowMessage(runOptionSelectionSelectionState, 0x1E0001, -1, 5);
        MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
        runOptionSelectionSelectionState2 = MentShowYesNoPrompt(0x1E0035, 3, 0);
        if (runOptionSelectionSelectionState2 == -1) {
            MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
            MentSetWindowMessage(runOptionSelectionSelectionState, 0x1E0003, -1, 5);
            MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
            runOptionSelectionSelectionState2 = MentShowAlternateYesNoPrompt(0x1E0035, 3, 0);
            if (runOptionSelectionSelectionState2 == 0) {
                MentCloseMessageWindow(runOptionSelectionSelectionState);
                MentReturnToMenu(0);
            }
            continue;
        }
        if (runOptionSelectionSelectionState2 == 0) {
            MentCloseMessageWindow(runOptionSelectionSelectionState);
            SLLoadBoardStory();
            if (_CheckFlag(FLAG_ID_MAKE(0, 7)) != 0 && _CheckFlag(FLAG_ID_MAKE(0, 9)) != 0) {
                OSReport("########## Next to Ending\n");
                MentFadeOutMenuMusic(1);
                omOvlGotoEx(DLL_mstory2dll, 1, 4, 0);
            }
            else {
                if (_CheckFlag(0x10000) != 0) {
                    OSReport("########### Next to Turn\n");
                    HuDataDirClose(DATADIR_MENT);
                    HuPrcChildCreate(MentLoadBoardData, 0x64, 0x3000, 0, menuObjectManagerProcess);
                    MentSleepFrame();
                    MentLaunchSelectedMode(0);
                }
                else {
                    if (MentIsMenuOptionUnlocked(GWSystem.board) != 0) {
                        if (_CheckFlag(FLAG_ID_MAKE(0, 9)) != 0) {
                            if (MentAreAllBoardsUnlocked() != 0) {
                                OSReport("########## Next to MapSelect KoopaEvent\n");
                                MentFadeOutMenuMusic(1);
                                omOvlGotoEx(DLL_mstory2dll, 1, 0, 0);
                            }
                            else {
                                OSReport("########## Next to MapSelect NormalEvent\n");
                                MentFadeOutMenuMusic(1);
                                omOvlGotoEx(DLL_mentdll, 1, 0xA, 0);
                            }
                        }
                        else if (GWSystem.board == 5) {
                            OSReport("########## Next to BoardClear KoopaEvent\n");
                            MentFadeOutMenuMusic(0);
                            omOvlGotoEx(DLL_mstory2dll, 1, 1, 0);
                        }
                        else {
                            OSReport("########## Next to BoardClear NormalEvent\n");
                            MentFadeOutMenuMusic(0);
                            omOvlGotoEx(DLL_mstorydll, 1, 0, 0);
                        }
                    }
                    else {
                        if (_CheckFlag(FLAG_ID_MAKE(0, 9)) != 0) {
                            if (GWSystem.board == 5) {
                                OSReport("########## Next to BoardMiss KoopaEvent\n");
                                MentFadeOutMenuMusic(0);
                                omOvlGotoEx(DLL_mstory2dll, 1, 2, 0);
                            }
                            else {
                                OSReport("########### Next to BoardMiss NormalEvent\n");
                                MentFadeOutMenuMusic(0);
                                omOvlGotoEx(DLL_mstorydll, 1, 1, 0);
                            }
                        }
                        else {
                            OSReport("########### Next to Turn\n");
                            HuDataDirClose(DATADIR_MENT);
                            HuPrcChildCreate(MentLoadBoardData, 0x64, 0x3000, 0, menuObjectManagerProcess);
                            MentSleepFrame();
                            MentLaunchSelectedMode(0);
                        }
                    }
                }
            }
            while (1) {
                MentSleepFrame();
            }
        }
        if (runOptionSelectionSelectionState2 == 1) {
            MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
            MentSetWindowMessage(runOptionSelectionSelectionState, 0x1E0002, -1, -1);
            MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
            MentCloseMessageWindow(runOptionSelectionSelectionState);
            break;
        }
    }
}

void MentUpdateOptionSelection(void)
{
    s32 updateOptionSelectionMenuItemIndex;
    s32 updateOptionSelectionSelectionState;
    s32 updateOptionSelectionSelectionState2;

    updateOptionSelectionSelectionState = MentCreateStandardMessageWindow(0, 0, 0);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(updateOptionSelectionSelectionState, 0x1A0004, -1, 5);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
    updateOptionSelectionSelectionState2 = MentShowAlternateYesNoPrompt(0x1E0035, 3, 0);
    MentCloseMessageWindow(updateOptionSelectionSelectionState);
    if (updateOptionSelectionSelectionState2 == 0) {
        s32 updateOptionSelectionSelectionValue[4] = { 3, 0, 2, 4 };
        _SetFlag(0x1000B);
        for (updateOptionSelectionMenuItemIndex = 0; updateOptionSelectionMenuItemIndex < 4; updateOptionSelectionMenuItemIndex++) {
            GWPlayerCfg[updateOptionSelectionMenuItemIndex].character = updateOptionSelectionSelectionValue[updateOptionSelectionMenuItemIndex];
            GWPlayerCfg[updateOptionSelectionMenuItemIndex].pad_idx = updateOptionSelectionMenuItemIndex;
            GWPlayerCfg[updateOptionSelectionMenuItemIndex].diff = 0;
            GWPlayerCfg[updateOptionSelectionMenuItemIndex].group = 0;
            GWPlayerCfg[updateOptionSelectionMenuItemIndex].iscom = 1;
            OSReport("ID-%d CHR-%d PAD-%d DIF-%d GRP-%d COM-%d\n", updateOptionSelectionMenuItemIndex, GWPlayerCfg[updateOptionSelectionMenuItemIndex].character, GWPlayerCfg[updateOptionSelectionMenuItemIndex].pad_idx,
                GWPlayerCfg[updateOptionSelectionMenuItemIndex].diff, GWPlayerCfg[updateOptionSelectionMenuItemIndex].group, GWPlayerCfg[updateOptionSelectionMenuItemIndex].iscom);
        }
        BoardSaveInit(6);
        GWSystem.max_turn = 0x14;
        HuDataDirClose(DATADIR_MENT);
        HuPrcChildCreate(MentLoadBoardData, 0x64, 0x3000, 0, menuObjectManagerProcess);
        {
            OMOVLHIS *updateOptionSelectionOverlayHistory = omOvlHisGet(0);
        }
        omOvlHisChg(0, DLL_mentdll, 1, 1);
        MentReturnToMenu(2);
    }
    else {
        _ClearFlag(0x1000B);
    }
}

void MentFinishMenuTransition(void)
{
    MentCreateBoardSelectionWindows();
}

s32 MentRunMenuTransition(void)
{
    s32 runMenuTransitionMenuValue;
    s32 runMenuTransitionAnimationFrame;
    s32 runMenuTransitionAnimationFrame2;
    s32 runMenuTransitionAnimationFrame3;

    runMenuTransitionMenuValue = 0;
    MentUpdateCharacterSelectionWindows();
    runMenuTransitionAnimationFrame = MentCreateStandardMessageWindow(0, 0, 0);
    while (1) {
        MentSleepFrame();
        MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
        MentSetWindowMessage(runMenuTransitionAnimationFrame, 0x1E005E, -1, -1);
        MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
        MentShowMessageWindow(0x1A0020);
        recordPlayerWork->updateCallback = (MentDllUpdateCallback)MentUpdateBoardRecordDisplay;
        while (1) {
            if (recordPlayerWork->recordFields[0] == 1) {
                runMenuTransitionAnimationFrame2 = 1;
                break;
            }
            else if ((HuPadBtnDown[recordPlayerWork->controllerPort] & PAD_BUTTON_B) != 0) {
                HuAudFXPlay(3);
                recordPlayerWork->updateCallback = NULL;
                runMenuTransitionAnimationFrame2 = 0;
                break;
            }
            else {
                MentSleepFrame();
            }
        }
        recordPlayerWork->updateCallback = NULL;
        MentClosePersistentMessageWindow(0);
        if (runMenuTransitionAnimationFrame2 != 0) {
            break;
        }
        MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
        MentSetWindowMessage(runMenuTransitionAnimationFrame, 0x1E0005, -1, 5);
        MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
        runMenuTransitionAnimationFrame3 = MentShowAlternateYesNoPrompt(0x1E0035, 3, 0);
        if (runMenuTransitionAnimationFrame3 == 0) {
            MentCloseMessageWindow(runMenuTransitionAnimationFrame);
            MentReturnToMenu(0);
        }
    }
    MentCloseMessageWindow(runMenuTransitionAnimationFrame);
    MentSleepTicks(0x1E);
    return runMenuTransitionAnimationFrame2;
}

s32 MentRunBoardTransition(void)
{
    s32 runBoardTransitionStateValue;
    s32 runBoardTransitionAnimationFrame;
    s32 runBoardTransitionAnimationFrame2;
    s32 runBoardTransitionAnimationFrame3;
    s32 runBoardTransitionAnimationFrame4;
    s32 runBoardTransitionAnimationFrame5;

    runBoardTransitionStateValue = 0;
    runBoardTransitionAnimationFrame4 = -1;
    runBoardTransitionAnimationFrame5 = -1;
    runBoardTransitionAnimationFrame = MentCreateStandardMessageWindow(0, 0, 0);
    while (1) {
        MentSleepFrame();
        MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
        MentSetWindowMessage(runBoardTransitionAnimationFrame, 0x1E0009, -1, -1);
        MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
        MentShowMessageWindow(0x1A0021);
        playerSelectionWork[1].playerSelections[1] = 0;
        MentCreateBoardSelectionSprites();
        playerSelectionWork[0].updateCallback = (MentDllUpdateCallback)MentUpdateBoardPlayerSelection;
        while (1) {
            MentSleepFrame();
            if ((runBoardTransitionAnimationFrame4 != playerSelectionWork[0].playerSelections[0]) || (runBoardTransitionAnimationFrame5 != playerSelectionWork[0].selectionFlags[playerSelectionWork[0].playerSelections[0] - 1])) {
                runBoardTransitionAnimationFrame4 = playerSelectionWork[0].playerSelections[0];
                runBoardTransitionAnimationFrame5 = playerSelectionWork[0].selectionFlags[playerSelectionWork[0].playerSelections[0] - 1];
                switch (playerSelectionWork[0].playerSelections[0]) {
                    case 1:
                        HuWinInsertMesSet(runBoardTransitionAnimationFrame, playerSelectionWork[0].selectionFlags[0] + 0x1B0028, 0);
                        MentSetWindowMessage(runBoardTransitionAnimationFrame, 0x1B0027, -1, -0x3E7);
                        break;
                    case 2:
                        MentSetWindowMessage(runBoardTransitionAnimationFrame, playerSelectionWork[0].selectionFlags[1] + 0x1B0018, -1, -0x3E7);
                        break;
                }
            }
            if (playerSelectionWork[0].playerSelections[3] != 0) {
                continue;
            }
            if (playerSelectionWork[1].playerSelections[1] == 1) {
                runBoardTransitionAnimationFrame2 = 2;
                break;
            }
            else if (playerSelectionWork[1].playerSelections[1] == -1) {
                runBoardTransitionAnimationFrame2 = 0;
                break;
            }
        }
        MentUpdateBoardSelectionSprites();
        playerSelectionWork[0].updateCallback = NULL;
        MentClosePersistentMessageWindow(0);
        if (runBoardTransitionAnimationFrame2 == 1) {
            MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
            MentSetWindowMessage(runBoardTransitionAnimationFrame, 0x1E0005, -1, 5);
            MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
            runBoardTransitionAnimationFrame3 = MentShowAlternateYesNoPrompt(0x1E0035, 3, 0);
            if (runBoardTransitionAnimationFrame3 == 0) {
                MentCloseMessageWindow(runBoardTransitionAnimationFrame);
                MentReturnToMenu(0);
            }
        }
        else if (runBoardTransitionAnimationFrame2 == 2) {
            MentSetWindowMessage(runBoardTransitionAnimationFrame, 0x1B0000, -1, 5);
            runBoardTransitionAnimationFrame3 = MentShowYesNoPrompt(0x1E0035, 3, 0);
            if (runBoardTransitionAnimationFrame3 == 0) {
                break;
            }
            MentSetWindowMessage(runBoardTransitionAnimationFrame, 0x1B0007, -1, 5);
            runBoardTransitionAnimationFrame3 = MentShowAlternateYesNoPrompt(0x1E0035, 3, 0);
            if (runBoardTransitionAnimationFrame3 == 0) {
                MentUpdateCharacterSelectionWindows();
                runBoardTransitionAnimationFrame2 = 0;
                break;
            }
            else {
                runBoardTransitionAnimationFrame4 = -1;
                runBoardTransitionAnimationFrame5 = -1;
            }
        }
        else {
            runBoardTransitionAnimationFrame2 = 0;
            break;
        }
    }
    menuSelectionMode = playerSelectionWork[0].selectionFlags[1];
    MentCloseMessageWindow(runBoardTransitionAnimationFrame);
    return runBoardTransitionAnimationFrame2;
}

void MentFinishMenuSequence(void)
{
    MentUpdateBoardSelectionWindows();
}

void MentResetMenuSequence(void)
{
    MentCreateMenuOptionWindows();
}

void MentRunMenuSequence(void)
{
    s32 runMenuSequenceSoundEffectId;
    s32 runMenuSequenceMenuItemIndex;
    s32 runMenuSequenceStateValue;

    MentSleepTicks(0x3C);
    runMenuSequenceStateValue = MentCreateStandardMessageWindow(0, 0, 0);
    particleEffectObject->work[0] = 1;
    runMenuSequenceSoundEffectId = -1;
    runMenuSequenceSoundEffectId = HuAudFXPlay(0x43);
    HuAudFXPanning(runMenuSequenceSoundEffectId, 0x20);
    runMenuSequenceSoundEffectId = HuAudFXPlay(0x40);
    HuAudFXPanning(runMenuSequenceSoundEffectId, 0x30);
    runMenuSequenceSoundEffectId = HuAudFXPlay(0x37);
    HuAudFXPanning(runMenuSequenceSoundEffectId, 0x40);
    runMenuSequenceSoundEffectId = HuAudFXPlay(0x4B);
    HuAudFXPanning(runMenuSequenceSoundEffectId, 0x4C);
    runMenuSequenceSoundEffectId = HuAudFXPlay(0x46);
    HuAudFXPanning(runMenuSequenceSoundEffectId, 0x60);
    HuAudFXPlay(0x9A);
    for (runMenuSequenceMenuItemIndex = 0; runMenuSequenceMenuItemIndex < 5; runMenuSequenceMenuItemIndex++) {
        MentSetModelMotionIfChanged(menuItemWork[runMenuSequenceMenuItemIndex].ownerObject, 1, 6, 5, 1);
    }
    HuWinInsertMesSet(runMenuSequenceStateValue, recordPlayerWork->character, 0);
    MentSetWindowMessage(runMenuSequenceStateValue, 0x1E0006, 0, 0xB4);
    for (runMenuSequenceMenuItemIndex = 0; runMenuSequenceMenuItemIndex < 5; runMenuSequenceMenuItemIndex++) {
        MentSetModelMotionIfChanged(menuItemWork[runMenuSequenceMenuItemIndex].ownerObject, 1, 1, 0xF, 1);
    }
    MentSleepTicks(0x1E);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
    HuWinInsertMesSet(runMenuSequenceStateValue, recordPlayerWork->character, 0);
    MentSetWindowMessage(runMenuSequenceStateValue, 0x1E0007, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
    MentSetMenuCameraCallback(MentAnimateMenuCameraToCloseView);
    HuAudFXPlay(0x39);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(runMenuSequenceStateValue, 0x1E0008, 0, 0x78);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
    MentSetMenuCameraCallback(MentZoomAlternateMenuCameraIn);
    MentCloseMessageWindow(runMenuSequenceStateValue);
    MentSleepTicks(0x78);
    particleEffectObject->objFunc = NULL;
    _ClearFlag(2);
    _ClearFlag(3);
    _ClearFlag(4);
    _ClearFlag(5);
    _ClearFlag(6);
    _ClearFlag(7);
    _ClearFlag(1);
    _ClearFlag(9);
}

void MentUpdateMenuSequence(void)
{
    s32 updateMenuSequenceStateValue;
    s32 i;
    s32 updateMenuSequenceStateValue2;
    s32 updateMenuSequenceStateValue3;
    s32 updateMenuSequenceStateValue4;
    s32 updateMenuSequenceStateValue5;

    s32 updateMenuSequenceMenuValue[6] = { 1, 2, 0, 3, 4, 5 };
    updateMenuSequenceStateValue3 = 0;
    updateMenuSequenceStateValue = 2;

    if (playerSelectionMessageWindow == -1) {
        updateMenuSequenceStateValue2 = MentCreateStandardMessageWindow(0, 0, 0);
    }
    else {
        updateMenuSequenceStateValue2 = playerSelectionMessageWindow;
        playerSelectionMessageWindow = -1;
    }
    while (1) {
        MentSleepFrame();
        updateMenuSequenceStateValue3 = 0;
        menuItemWork[0].selectionTimer = 1;
        HuAudFXPlay(menuSoundEffectIds[0][3]);
        MentSetModelMotionIfChanged(menuItemWork[updateMenuSequenceMenuValue[updateMenuSequenceStateValue]].ownerObject, 1, 4, 0xF, 1);
        MentSetWindowMessage(updateMenuSequenceStateValue2, 0x1A0008, -1, -1);
        MentSetModelMotionIfChanged(menuItemWork[updateMenuSequenceMenuValue[updateMenuSequenceStateValue]].ownerObject, 1, 1, 0xF, 1);
        MentShowMessageWindow(0x1A0020);
        for (i = 0; i < 5; i++) {
            menuItemWork[i].isVisible = 0;
            menuItemWork[i].updateCallback = (MentDllUpdateCallback)MentUpdateMenuItemObject;
        }
        menuItemWork[updateMenuSequenceMenuValue[updateMenuSequenceStateValue]].isVisible = 1;
        playerSelectionWork[0].updateCallback = (MentDllUpdateCallback)MentUpdatePlayerSelectionDisplay;
        while (1) {
            MentSleepFrame();
            for (i = 0; i < 5; i++) {
                if (menuItemWork[i].isVisible != 2) {
                    break;
                }
            }
            if (i == 5 && updateMenuSequenceStateValue3 >= 0x15) {
                if ((HuPadBtnDown[recordPlayerWork->controllerPort] & 0x100)) {
                    HuAudFXPlay(2);
                    updateMenuSequenceStateValue5 = 2;
                    break;
                }

                if ((HuPadStkX[recordPlayerWork[0].controllerPort] >= 0x32) || ((HuPadBtn[recordPlayerWork[0].controllerPort] & PAD_BUTTON_RIGHT) != 0)) {
                    updateMenuSequenceStateValue++;
                    if (updateMenuSequenceStateValue > 4) {
                        updateMenuSequenceStateValue = 4;
                    }
                    else {
                        updateMenuSequenceStateValue3 = 0;
                    }
                }
                else if ((HuPadStkX[recordPlayerWork[0].controllerPort] <= -50) || ((HuPadBtn[recordPlayerWork[0].controllerPort] & PAD_BUTTON_LEFT) != 0)) {
                    updateMenuSequenceStateValue--;
                    if (updateMenuSequenceStateValue < 0) {
                        updateMenuSequenceStateValue = 0;
                    }
                    else {
                        updateMenuSequenceStateValue3 = 0;
                    }
                }
                else if ((HuPadBtnDown[0] & 0x200)) {
                    updateMenuSequenceStateValue5 = 1;
                    break;
                }
            }
            if (updateMenuSequenceStateValue3 == 0) {
                for (i = 0; i < 5; i++) {
                    menuItemWork[i].isVisible = 0;
                }
                menuItemWork[updateMenuSequenceMenuValue[updateMenuSequenceStateValue]].isVisible = 1;
            }
            if (updateMenuSequenceStateValue3++ == 0x14) {
                updateMenuSequenceStateValue3 = 0x15;
                MentAnimateSelectionWindow(updateMenuSequenceMenuValue[updateMenuSequenceStateValue]);
                MentSetWindowMessage(updateMenuSequenceStateValue2, updateMenuSequenceMenuValue[updateMenuSequenceStateValue] + 0x1B0001, -1, -0x3E7);
                if (updateMenuSequenceMenuValue[updateMenuSequenceStateValue] != 0) {
                    HuAudFXPlay(menuSoundEffectIds[updateMenuSequenceMenuValue[updateMenuSequenceStateValue]][0]);
                }
                else {
                    HuAudFXPlay(menuSoundEffectIds[updateMenuSequenceMenuValue[updateMenuSequenceStateValue]][2]);
                }
            }
        }
        MentClosePersistentMessageWindow(0);
        if (updateMenuSequenceStateValue5 == 1) {
            MentSetMenuCameraCallback(MentAnimateMenuCameraToFront);
            if (updateMenuSequenceStateValue != 2) {
                updateMenuSequenceStateValue = 2;
                for (i = 0; i < 5; i++) {
                    menuItemWork[i].isVisible = 0;
                }
                menuItemWork[updateMenuSequenceMenuValue[updateMenuSequenceStateValue]].isVisible = 1;
                menuItemWork[updateMenuSequenceMenuValue[updateMenuSequenceStateValue]].selectionTimer = 1;
            }
            MentUpdateSelectionWindow();
            MentSleepTicks(0x1E);
            MentSetWindowMessage(updateMenuSequenceStateValue2, 0x1E0005, -1, 5);
            updateMenuSequenceStateValue4 = MentShowAlternateYesNoPrompt(0x1E0035, 3, 0);
            if (updateMenuSequenceStateValue4 == 0) {
                MentCloseMessageWindow(updateMenuSequenceStateValue2);
                MentReturnToMenu(0);
            }
            continue;
        }
        if (updateMenuSequenceStateValue5 != 2) {
            continue;
        }
        if (((updateMenuSequenceMenuValue[updateMenuSequenceStateValue] == 0) && (_CheckFlag(FLAG_ID_MAKE(0, 2)) != 0)) || ((updateMenuSequenceMenuValue[updateMenuSequenceStateValue] == 1) && (_CheckFlag(FLAG_ID_MAKE(0, 3)) != 0))
            || ((updateMenuSequenceMenuValue[updateMenuSequenceStateValue] == 2) && (_CheckFlag(FLAG_ID_MAKE(0, 4)) != 0)) || ((updateMenuSequenceMenuValue[updateMenuSequenceStateValue] == 3) && (_CheckFlag(FLAG_ID_MAKE(0, 5)) != 0))
            || ((updateMenuSequenceMenuValue[updateMenuSequenceStateValue] == 4) && (_CheckFlag(FLAG_ID_MAKE(0, 6)) != 0))) {
            MentSetModelMotionIfChanged(menuItemWork[updateMenuSequenceMenuValue[updateMenuSequenceStateValue]].ownerObject, 1, 4, 0xF, 1);
            HuWinInsertMesSet(updateMenuSequenceStateValue2, updateMenuSequenceMenuValue[updateMenuSequenceStateValue] + 0x1B0021, 1);
            MentSetWindowMessage(updateMenuSequenceStateValue2, updateMenuSequenceMenuValue[updateMenuSequenceStateValue] + 0x1E0052, -1, -1);
            MentSetWindowMessage(updateMenuSequenceStateValue2, updateMenuSequenceMenuValue[updateMenuSequenceStateValue] + 0x1E0057, -1, 5);
            MentSetModelMotionIfChanged(menuItemWork[updateMenuSequenceMenuValue[updateMenuSequenceStateValue]].ownerObject, 1, 1, 0xF, 1);
            updateMenuSequenceStateValue4 = MentShowAlternateYesNoPrompt(0x1E0035, 3, 0);
            if (updateMenuSequenceStateValue4 == 0) {
                for (i = 0; i < 5; i++) {
                    menuItemWork[i].updateCallback = NULL;
                }
                menuLaunchParameters[2] = updateMenuSequenceMenuValue[updateMenuSequenceStateValue];
            }
            else {
                if (updateMenuSequenceStateValue != 2) {
                    updateMenuSequenceStateValue = 2;
                    for (i = 0; i < 5; i++) {
                        menuItemWork[i].isVisible = 0;
                    }
                    menuItemWork[updateMenuSequenceMenuValue[updateMenuSequenceStateValue]].isVisible = 1;
                    menuItemWork[updateMenuSequenceMenuValue[updateMenuSequenceStateValue]].selectionTimer = 1;
                }
                MentUpdateSelectionWindow();
                MentSleepTicks(0x1E);
                continue;
            }
        }
        else {
            for (i = 0; i < 5; i++) {
                menuItemWork[i].updateCallback = NULL;
            }
            menuLaunchParameters[2] = updateMenuSequenceMenuValue[updateMenuSequenceStateValue];
        }
        break;
    }
    MentCloseMessageWindow(updateMenuSequenceStateValue2);
}

void MentRunMenuConfirmation(void)
{
    s32 runMenuConfirmationStateValue = menuLaunchParameters[2];
    {
        Vec runMenuConfirmationTargetPosition = { -120.0f, 0.0f, 670.0f };

        MentUpdateMenuOptionWindows();
        MentSetMenuCameraCallback(MentAnimateMenuCameraToDefaultView);
        MentSetModelMotion(menuItemWork[runMenuConfirmationStateValue].ownerObject, 1, 3, 0xF, 1);
        MentMoveAndTurnModelToTarget(menuItemWork[runMenuConfirmationStateValue].ownerObject, 1, runMenuConfirmationTargetPosition, 5.0f, 6.0f, 10.0f, 1, 1);
        MentSetModelMotion(menuItemWork[runMenuConfirmationStateValue].ownerObject, 1, 1, 0xF, 1);
        Hu3DModelAttrReset(playerSelectionWork[0].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
        MentSetModelMotion(playerSelectionWork[0].ownerObject, 2, 2, 0, 0);
        MentWaitForModelMotion(playerSelectionWork[0].ownerObject, 2, 0);
        MentSleepTicks(0x3C);
    }
}

void MentUpdateMenuConfirmation(void)
{
    s32 updateMenuConfirmationStateValue;
    s32 updateMenuConfirmationMenuItemIndex;
    s32 updateMenuConfirmationStateValue2;
    s32 updateMenuConfirmationStateValue3;

    updateMenuConfirmationStateValue = 0;
    updateMenuConfirmationStateValue2 = menuLaunchParameters[2];
    for (updateMenuConfirmationMenuItemIndex = 0; updateMenuConfirmationMenuItemIndex < 5; updateMenuConfirmationMenuItemIndex++) {
        Hu3DModelAttrSet(menuItemWork[updateMenuConfirmationMenuItemIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    }
    Hu3DModelAttrReset(menuItemWork[updateMenuConfirmationStateValue2].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    MentCreateMenuItemModel(playerSelectionWork[updateMenuConfirmationStateValue].ownerObject, menuLaunchParameters[2]);
    Hu3DModelAttrReset(playerSelectionWork[updateMenuConfirmationStateValue].ownerObject->mdlId[4], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(playerSelectionWork[updateMenuConfirmationStateValue].ownerObject->mdlId[5], HU3D_ATTR_DISPOFF);
    playerSelectionWork[updateMenuConfirmationStateValue].updateCallback = (MentDllUpdateCallback)MentUpdateMenuItemAnimation;
    HuDataDirClose(DATADIR_MENT);
    HuPrcChildCreate(MentLoadBoardData, 0x64, 0x3000, 0, menuObjectManagerProcess);
    MentSetModelMotion(playerSelectionWork[updateMenuConfirmationStateValue].ownerObject, 2, 3, 0, 0);
    updateMenuConfirmationStateValue3 = MentCreateStandardMessageWindow(0, 0, 0);
    MentSetModelMotionIfChanged(menuItemWork[updateMenuConfirmationStateValue2].ownerObject, 1, 5, 0xF, 1);
    MentSetWindowMessage(updateMenuConfirmationStateValue3, updateMenuConfirmationStateValue2 + 0x1E000C, -1, -1);
    MentCloseMessageWindow(updateMenuConfirmationStateValue3);
    MentWaitForModelMotion(playerSelectionWork[updateMenuConfirmationStateValue].ownerObject, 2, 0);
    Hu3DModelAttrSet(playerSelectionWork[updateMenuConfirmationStateValue].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
    HuAudSeqFadeOut(menuSequenceHandles[0], 0xBB8);
    MentSetMenuCameraCallback(MentAnimateMenuCameraAroundPlayer);
    MentSleepTicks(0xA);
    HuAudFXPlay(GWPlayerCfg->character + 0x75);
    MentSleepTicks(0x8C);
}

void MentStartStoryModeSequence(void)
{
    menuSequenceHandles[0] = HuAudSeqPlay(0x30);
    OSReport("########### ME_MainProcFunc200\n");
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        MentSleepFrame();
    }
    MentSleepTicks(0x3C);
}

void MentAnimateStoryModeCharacter(void)
{
    Vec animateStoryModeCharacterTargetPosition = { 0.0f, 0.0f, 560.0f };
    Hu3DModelAttrReset(playerSelectionWork[0].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
    MentSetModelMotion(playerSelectionWork[0].ownerObject, 2, 3, 0, 0);
    MentWaitForModelMotion(playerSelectionWork[0].ownerObject, 2, 0);
    MentResetMenuSequence();
    MentSetMenuCameraCallback(MentFocusMenuCameraOnPlayer);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 2, 0xF, 1);
    MentMoveAndTurnModelToTarget(menuItemWork[0].ownerObject, 1, animateStoryModeCharacterTargetPosition, 0.0f, 3.0f, 0.0f, 1, 0);
    MentSetModelMotionIfChanged(menuItemWork[0].ownerObject, 1, 1, 0xF, 1);
}

void MentStartMinigameModeSequence(void)
{
    menuSequenceHandles[0] = HuAudSeqPlay(6);
    OSReport("########### ME_MainProcFunc300\n");
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        MentSleepFrame();
    }
    MentResetMenuSequence();
    playerSelectionWork[1].updateCallback = (MentDllUpdateCallback)MentUpdatePlayerSelectionDisplay;
    MentAnimateSelectionWindow(5);
    MentSleepTicks(0x3C);
}

void MentRunModeTransition(void)
{
    s32 runModeTransitionAnimationFrame;

    menuLaunchParameters[2] = 5;
    runModeTransitionAnimationFrame = MentCreateStandardMessageWindow(0, 0, 0);
    HuAudFXPlay(menuSoundEffectIds[6][2]);
    MentSetWindowMessage(runModeTransitionAnimationFrame, 0x1F0008, 0, 0x3C);
    MentSetModelMotionIfChanged(selectedMenuItemWork.ownerObject, 1, 1, 0xF, 1);
    MentCloseMessageWindow(runModeTransitionAnimationFrame);
    MentUpdateMenuOptionHighlights();
    MentSleepTicks(0x3C);
    Hu3DModelAttrReset(playerSelectionWork[1].ownerObject->mdlId[4], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(playerSelectionWork[1].ownerObject->mdlId[5], HU3D_ATTR_DISPOFF);
    MentConfigureBoardPlayers();
    HuDataDirClose(DATADIR_MENT);
    HuPrcChildCreate(MentLoadBoardData, 0x64, 0x3000, 0, menuObjectManagerProcess);
    MentSetModelMotionIfChanged(selectedMenuItemWork.ownerObject, 1, 2, 0xF, 1);
    MentSetModelMotion(playerSelectionWork[1].ownerObject, 2, 3, 0, 0);
    MentSleepTicks(0x3C);
    HuAudFXPlay(menuSoundEffectIds[6][0]);
    MentWaitForModelMotion(playerSelectionWork[1].ownerObject, 2, 0);
    Hu3DModelAttrSet(playerSelectionWork[1].ownerObject->mdlId[2], HU3D_ATTR_DISPOFF);
    HuAudSeqFadeOut(menuSequenceHandles[0], 0xBB8);
    MentSetMenuCameraCallback(MentAnimateMenuCameraAroundPlayer);
    MentSleepTicks(0xA);
    MentSleepTicks(0x8C);
}

void MentFadeOutMenuMusic(s32 isSinglePlayerMode)
{
    MentSleepTicks(0x3C);
    HuAudSeqFadeOut(menuSequenceHandles[0], 0x3E8);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        MentSleepFrame();
    }
    CharModelKill(-1);
    MGSeqKillAll();
    HuAudAllStop();
    HuDataDirClose(DATADIR_MENT);
    CharMotionInit(GWPlayerCfg[0].character);
    if (isSinglePlayerMode == 0) {
        CharMotionInit(GWPlayerCfg[1].character);
        CharMotionInit(GWPlayerCfg[2].character);
        CharMotionInit(GWPlayerCfg[3].character);
    }
}

void MentLaunchSelectedMode(s32 unusedMode)
{
    s32 launchSelectedModeStateValue[7] = { 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F };
    MentSleepTicks(0x3C);
    HuAudSeqFadeOut(menuSequenceHandles[0], 0x3E8);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        MentSleepFrame();
    }
    CharModelKill(-1);
    MGSeqKillAll();
    {
        OMOVLHIS *launchSelectedModeOverlayHistory = omOvlHisGet(0);
    }
    omOvlHisChg(0, DLL_mstory3dll, 0, 0);
    do {
        MentSleepFrame();
    } while (menuTransitionComplete != 1);
    CharMotionInit(GWPlayerCfg[0].character);
    CharMotionInit(GWPlayerCfg[1].character);
    CharMotionInit(GWPlayerCfg[2].character);
    CharMotionInit(GWPlayerCfg[3].character);
    HuAudSeqAllFadeOut(0x3E8);
    HuAudSStreamAllFadeOut(0x3E8);
    omOvlCallEx(launchSelectedModeStateValue[GWSystem.board], 1, 0, 0);
    while (1) {
        MentSleepFrame();
    }
}

void MentUpdateStoryModeSelection(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork)
{
    s32 updateStoryModeSelectionIndex;
    s32 updateStoryModeSelectionSelectionState;
    s32 updateStoryModeSelectionIndex2;
    s32 updateStoryModeSelectionSelectionState2;
    s32 updateStoryModeSelectionSelectionState3;
    s32 updateStoryModeSelectionSelectionState4;

    updateStoryModeSelectionSelectionState2 = 0;
    if (menuLaunchParameters[1] == 0) {
        MentStartMenuMusicAndWipe();
        MentPlayMenuIntroAnimation();
        MentRunOptionSelection();
        MentUpdateOptionSelection();
    }
    else {
        MentRevealMenuItems();
        MentUpdateOptionSelection();
    }
    MentCreateBoardSelectionWindows();
    do {
        MentSleepFrame();
        switch (updateStoryModeSelectionSelectionState2) {
            case 0:
                updateStoryModeSelectionSelectionState2 = MentRunMenuTransition();
                break;
            case 1:
                updateStoryModeSelectionSelectionState2 = MentRunBoardTransition();
                break;
        }
    } while (updateStoryModeSelectionSelectionState2 != 2);
    MentFinishMenuSequence();
    MentStartRecordMenu();
    MentRunMenuShowcase();
    MentUpdateMenuSequence();
    MentRunMenuConfirmation();
    switch (menuSelectionMode) {
        case 0:
            updateStoryModeSelectionSelectionState3 = 0;
            break;
        case 1:
            updateStoryModeSelectionSelectionState3 = 1;
            break;
        case 2:
            updateStoryModeSelectionSelectionState3 = 2;
            break;
        default:
            updateStoryModeSelectionSelectionState3 = 0;
            break;
    }
    if (playerSelectionWork[0].selectionFlags[0] == 4) {
        updateStoryModeSelectionSelectionState4 = 1;
    }
    else {
        updateStoryModeSelectionSelectionState4 = 0;
    }
    BoardStoryConfigSet(updateStoryModeSelectionSelectionState3, updateStoryModeSelectionSelectionState4);
    GWStoryCharSet(recordPlayerWork[0].character);
    // looks a bit similar to mentConfigureBoardPlayers
    for (updateStoryModeSelectionIndex = 1; updateStoryModeSelectionIndex < 4; updateStoryModeSelectionIndex++) {
        recordPlayerWork[updateStoryModeSelectionIndex].character = -1;
        recordPlayerWork[updateStoryModeSelectionIndex].controllerPort = updateStoryModeSelectionIndex;
        recordPlayerWork[updateStoryModeSelectionIndex].isComputerPlayer = 1;
    }
    for (updateStoryModeSelectionIndex = 1; updateStoryModeSelectionIndex < 4; updateStoryModeSelectionIndex++) {
        do {
            recordPlayerWork[updateStoryModeSelectionIndex].character = rand8() % 8;
            for (updateStoryModeSelectionIndex2 = 0; updateStoryModeSelectionIndex2 < 4; updateStoryModeSelectionIndex2++) {
                if ((updateStoryModeSelectionIndex != updateStoryModeSelectionIndex2) && (recordPlayerWork[updateStoryModeSelectionIndex2].character != -1)
                    && (recordPlayerWork[updateStoryModeSelectionIndex].character == recordPlayerWork[updateStoryModeSelectionIndex2].character)) {
                    break;
                }
            }
        } while (updateStoryModeSelectionIndex2 != 4);
    }
    GWSystem.diff_story = playerSelectionWork[0].selectionFlags[0];
    for (updateStoryModeSelectionIndex = 0; updateStoryModeSelectionIndex < 4; updateStoryModeSelectionIndex++) {
        recordPlayerWork[updateStoryModeSelectionIndex].difficulty = GWSystem.diff_story;
        if (GWSystem.diff_story == 4) {
            recordPlayerWork[updateStoryModeSelectionIndex].difficulty = MentCalculateUnlockedMapDifficulty();
        }
    }
    for (updateStoryModeSelectionIndex = 0; updateStoryModeSelectionIndex < 4; updateStoryModeSelectionIndex++) {
        GWPlayer[updateStoryModeSelectionIndex].character = GWPlayerCfg[updateStoryModeSelectionIndex].character = recordPlayerWork[updateStoryModeSelectionIndex].character;
        GWPlayer[updateStoryModeSelectionIndex].port = GWPlayerCfg[updateStoryModeSelectionIndex].pad_idx = recordPlayerWork[updateStoryModeSelectionIndex].controllerPort;
        GWPlayer[updateStoryModeSelectionIndex].diff = GWPlayerCfg[updateStoryModeSelectionIndex].diff = recordPlayerWork[updateStoryModeSelectionIndex].difficulty;
        GWPlayerCfg[updateStoryModeSelectionIndex].group = 0;
        GWPlayer[updateStoryModeSelectionIndex].com = GWPlayerCfg[updateStoryModeSelectionIndex].iscom = recordPlayerWork[updateStoryModeSelectionIndex].isComputerPlayer;
    }
    BoardSaveInit(menuLaunchParameters[2]);
    MentUpdateMenuConfirmation();
    MentLaunchSelectedMode(0);
}

void MentUpdateBoardPlayerSettings(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork)
{
    _ClearFlag(FLAG_ID_MAKE(0, 9));
    playerSelectionWork[0].selectionFlags[0] = GWSystem.diff_story;
    MentStartStoryModeSequence();
    MentAnimateStoryModeCharacter();
    MentUpdateMenuSequence();
    MentRunMenuConfirmation();
    MentConfigureBoardPlayers();
    MentUpdateMenuConfirmation();
    WipeColorSet(0xFF, 0xFF, 0xFF);
    MentLaunchSelectedBoard();
}

void MentUpdateMinigamePlayerSettings(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork)
{
    _ClearFlag(9);
    playerSelectionWork[0].selectionFlags[0] = GWSystem.diff_story;
    MentStartMinigameModeSequence();
    MentRunModeTransition();
    WipeColorSet(0, 0, 0);
    MentLaunchSelectedBoard();
}

void MentCreateBoardSelectionScene(void)
{
    s32 createBoardSelectionSceneSelectionState;

    Vec createBoardSelectionSceneTargetPosition = { 0.0f, 0.0f, 560.0f };
    Vec createBoardSelectionSceneTargetPosition2 = { -120.0f, 0.0f, 670.0f };
    MentSetMenuCameraCallback(MentZoomMenuCameraIn);
    MentSleepTicks(0x5A);
    Hu3DModelAttrReset(menuItemWork[4].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    MentSetModelMotion(playerSelectionWork[0].ownerObject, 2, 3, 0, 0);
    MentSleepTicks(0x96);
    menuObjects[0]->work[0] = 1;
    MentSetMenuCameraCallback(MentZoomAlternateMenuCameraIn);
    MentSleepTicks(0x5A);
    MentSetModelMotionIfChanged(menuItemWork[4].ownerObject, 1, 2, 0xF, 1);
    MentMoveAndTurnModelToTarget(menuItemWork[4].ownerObject, 1, createBoardSelectionSceneTargetPosition, 0.0f, 3.0f, 0.0f, 1, 0);
    MentSetModelMotionIfChanged(menuItemWork[4].ownerObject, 1, 1, 0xF, 1);
    MentSleepTicks(0x1E);
    HuAudSeqPlay(0x2D);
    createBoardSelectionSceneSelectionState = MentCreateStandardMessageWindow(0, 0, 0);
    MentSetModelMotionIfChanged(menuItemWork[4].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(createBoardSelectionSceneSelectionState, 0x2F0000, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[4].ownerObject, 1, 1, 0xF, 1);
    MentCloseMessageWindow(createBoardSelectionSceneSelectionState);
    MentSetModelMotion(menuItemWork[4].ownerObject, 1, 3, 0xF, 1);
    MentMoveAndTurnModelToTarget(menuItemWork[4].ownerObject, 1, createBoardSelectionSceneTargetPosition2, 5.0f, 6.0f, 10.0f, 1, 1);
    MentSetModelMotion(menuItemWork[4].ownerObject, 1, 1, 0xF, 1);
    MentSleepTicks(0x1E);
    MentSetModelMotion(menuItemWork[4].ownerObject, 1, 5, 0xF, 1);
    MentSetMenuCameraCallback(MentAnimateMenuCameraToSelectionView);
    MentSetModelMotion(playerSelectionWork[0].ownerObject, 1, 0, 0, 0);
}

void MentFinishBoardSelectionScene(void)
{
    MentSleepTicks(0x6E);
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        MentSleepFrame();
    }
    CharModelKill(-1);
    MGSeqKillAll();
    HuAudFXAllStop();
    HuAudSStreamAllStop();
    omOvlGotoEx(DLL_option, 1, 0, 0);
    while (1) {
        MentSleepFrame();
    }
}

void MentUpdateBoardSelectionScene(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork)
{
    MentStartMenuMusicAndWipe();
    MentCreateBoardSelectionScene();
    MentFinishBoardSelectionScene();
}

void MentCreateCharacterSelectionScene(void)
{
    s32 createCharacterSelectionSceneSelectionState;

    Vec createCharacterSelectionSceneTargetPosition = { 0.0f, 0.0f, 560.0f };
    Vec createCharacterSelectionSceneTargetPosition2 = { -120.0f, 0.0f, 670.0f };
    MentSetMenuCameraCallback(MentZoomMenuCameraIn);
    MentSleepTicks(0x5A);
    Hu3DModelAttrReset(menuItemWork[2].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    MentSetModelMotion(playerSelectionWork[0].ownerObject, 2, 3, 0, 0);
    MentSleepTicks(0x96);
    menuObjects[0]->work[0] = 1;
    MentSetMenuCameraCallback(MentZoomAlternateMenuCameraIn);
    MentSleepTicks(0x5A);
    MentSetModelMotionIfChanged(menuItemWork[2].ownerObject, 1, 2, 0xF, 1);
    MentMoveAndTurnModelToTarget(menuItemWork[2].ownerObject, 1, createCharacterSelectionSceneTargetPosition, 0.0f, 3.0f, 0.0f, 1, 0);
    MentSetModelMotionIfChanged(menuItemWork[2].ownerObject, 1, 1, 0xF, 1);
    MentSleepTicks(0x1E);
    HuAudSeqPlay(0x2F);
    createCharacterSelectionSceneSelectionState = MentCreateStandardMessageWindow(0, 0, 0);
    MentSetModelMotionIfChanged(menuItemWork[2].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(createCharacterSelectionSceneSelectionState, 0x320000, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[2].ownerObject, 1, 1, 0xF, 1);
    MentCloseMessageWindow(createCharacterSelectionSceneSelectionState);
    MentSetModelMotion(menuItemWork[2].ownerObject, 1, 3, 0xF, 1);
    MentMoveAndTurnModelToTarget(menuItemWork[2].ownerObject, 1, createCharacterSelectionSceneTargetPosition2, 5.0f, 6.0f, 10.0f, 1, 1);
    MentSetModelMotion(menuItemWork[2].ownerObject, 1, 1, 0xF, 1);
    MentSleepTicks(0x1E);
    MentSetModelMotion(menuItemWork[2].ownerObject, 1, 5, 0xF, 1);
    MentSetMenuCameraCallback(MentAnimateMenuCameraToSelectionView);
    MentSetModelMotion(playerSelectionWork[0].ownerObject, 1, 0, 0, 0);
}

void MentFinishCharacterSelectionScene(void)
{
    MentSleepTicks(0x6E);
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        MentSleepFrame();
    }
    CharModelKill(-1);
    MGSeqKillAll();
    HuAudFXAllStop();
    HuAudSStreamAllStop();
    omOvlGotoEx(DLL_present, 1, 0, 0);
    while (1) {
        MentSleepFrame();
    }
}

void MentUpdateCharacterSelectionScene(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork)
{
    MentStartMenuMusicAndWipe();
    MentCreateCharacterSelectionScene();
    MentFinishCharacterSelectionScene();
}

void MentCreateOptionSelectionScene(void)
{
    s32 createOptionSelectionSceneSelectionState;
    s32 createOptionSelectionSceneSelectionState2;

    Vec createOptionSelectionSceneTargetPosition = { 0.0f, 0.0f, 560.0f };
    MentSetMenuCameraCallback(MentZoomMenuCameraIn);
    MentSleepTicks(0x5A);
    Hu3DModelAttrReset(menuItemWork[3].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    MentSetModelMotion(playerSelectionWork[0].ownerObject, 2, 3, 0, 0);
    MentSleepTicks(0x96);
    menuObjects[0]->work[0] = 1;
    MentSetMenuCameraCallback(MentZoomAlternateMenuCameraIn);
    MentSleepTicks(0x5A);
    MentSetModelMotionIfChanged(menuItemWork[3].ownerObject, 1, 2, 0xF, 1);
    MentMoveAndTurnModelToTarget(menuItemWork[3].ownerObject, 1, createOptionSelectionSceneTargetPosition, 0.0f, 3.0f, 0.0f, 1, 0);
    MentSetModelMotionIfChanged(menuItemWork[3].ownerObject, 1, 1, 0xF, 1);
    MentSleepTicks(0x1E);
    createOptionSelectionSceneSelectionState = MentCreateStandardMessageWindow(0, 0, 0);
    MentSetModelMotionIfChanged(menuItemWork[3].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(createOptionSelectionSceneSelectionState, 0x330000, -1, -1);
    MentSetModelMotionIfChanged(menuItemWork[3].ownerObject, 1, 1, 0xF, 1);
    MentCloseMessageWindow(createOptionSelectionSceneSelectionState);
    createOptionSelectionSceneSelectionState = MentCreateStandardMessageWindow(0, 0, 0);
    MentSetModelMotionIfChanged(menuItemWork[3].ownerObject, 1, 4, 0xF, 1);
    MentSetWindowMessage(createOptionSelectionSceneSelectionState, 0x330001, -1, 5);
    MentSetModelMotionIfChanged(menuItemWork[3].ownerObject, 1, 1, 0xF, 1);
    createOptionSelectionSceneSelectionState2 = MentShowYesNoPrompt(0x1E0035, 3, 0);
    MentCloseMessageWindow(createOptionSelectionSceneSelectionState);
    if (createOptionSelectionSceneSelectionState2 == 0) {
        createOptionSelectionSceneSelectionState = MentCreateStandardMessageWindow(0, 0, 0);
        MentSetModelMotionIfChanged(menuItemWork[3].ownerObject, 1, 4, 0xF, 1);
        MentSetWindowMessage(createOptionSelectionSceneSelectionState, 0x330002, -1, -1);
        MentSetModelMotionIfChanged(menuItemWork[3].ownerObject, 1, 1, 0xF, 1);
        MentCloseMessageWindow(createOptionSelectionSceneSelectionState);
    }
}

void MentCreatePlayerSelectionScene(void)
{

    Vec createPlayerSelectionSceneTargetPosition = { -120.0f, 0.0f, 670.0f };
    MentSetModelMotion(menuItemWork[3].ownerObject, 1, 3, 0xF, 1);
    MentMoveAndTurnModelToTarget(menuItemWork[3].ownerObject, 1, createPlayerSelectionSceneTargetPosition, 5.0f, 6.0f, 10.0f, 1, 1);
    MentSetModelMotion(menuItemWork[3].ownerObject, 1, 1, 0xF, 1);
    MentSleepTicks(0x1E);
    MentSetModelMotion(menuItemWork[3].ownerObject, 1, 5, 0xF, 1);
    MentSetMenuCameraCallback(MentAnimateMenuCameraToSelectionView);
    MentSetModelMotion(playerSelectionWork[0].ownerObject, 1, 0, 0, 0);
}

void MentFinishPlayerSelectionScene(void)
{
    MentSleepTicks(0x6E);
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        MentSleepFrame();
    }
    CharModelKill(-1);
    MGSeqKillAll();
    HuAudFadeOut(1);
    omOvlReturnEx(1, 1);
    while (1) {
        MentSleepFrame();
    }
}

void MentUpdatePlayerSelectionScene(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork)
{
    s32 updatePlayerSelectionSceneSelectionValue = 0xA;
    s32 updatePlayerSelectionSceneSelectionValue2 = 0;
    MentStartMenuMusicAndWipe();
    MentCreateOptionSelectionScene();
    MentCreatePlayerSelectionScene();
    MentFinishPlayerSelectionScene();
}

void MentCreateMenuCameraObject(void)
{
    OMOBJ *createMenuCameraObjectObject;

    createMenuCameraObjectObject = omAddObjEx(menuObjectManagerProcess, 0x100, 0x10, 0x10, -1, NULL);
    selectedPlayerWork.ownerObject = createMenuCameraObjectObject;
    switch (menuLaunchParameters[0]) {
        case 0:
        case 20:
            selectedPlayerWork.updateCallback = (MentDllUpdateCallback)MentUpdatePlayerSelectionObject;
            break;
        case 1:
            selectedPlayerWork.updateCallback = (MentDllUpdateCallback)MentUpdateStoryModeSelection;
            break;
        case 3:
            selectedPlayerWork.updateCallback = (MentDllUpdateCallback)MentUpdateBoardSelectionScene;
            break;
        case 4:
            selectedPlayerWork.updateCallback = (MentDllUpdateCallback)MentUpdateCharacterSelectionScene;
            break;
        case 5:
            selectedPlayerWork.updateCallback = (MentDllUpdateCallback)MentUpdatePlayerSelectionScene;
            break;
        case 10:
            selectedPlayerWork.updateCallback = (MentDllUpdateCallback)MentUpdateBoardPlayerSettings;
            break;
        case 11:
            selectedPlayerWork.updateCallback = (MentDllUpdateCallback)MentUpdateMinigamePlayerSettings;
            break;
    }
    while (1) {
        MentSleepFrame();
        if (selectedPlayerWork.updateCallback == NULL) {
            continue;
        }
        selectedPlayerWork.updateCallback(createMenuCameraObjectObject, &selectedPlayerWork);
    }
}

void MentUpdateMenuItemAnimation(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork)
{
    Hu3DData[object->mdlId[4]].pos.y = MentInterpolateSineWave(80.0f, 90.0f, selectionWork->selectionFlags[0]++, 360.0f);
    if (selectionWork->selectionFlags[0] >= 0x168) {
        selectionWork->selectionFlags[0] = 0;
    }
}

void MentCreateMenuItemModel(OMOBJ *object, s32 itemIndex)
{
    object->mdlId[4] = Hu3DModelCreateFile(itemIndex + DATA_MAKE_NUM(DATADIR_MENT, 0xD));
    if (itemIndex != 5) {
        object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x13));
    }
    else {
        object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x14));
    }
    object->mtnId[4] = Hu3DMotionIDGet(object->mdlId[4]);
    object->mtnId[5] = Hu3DMotionIDGet(object->mdlId[5]);
    MentSetModelMotionIfChanged(object, 4, 4, 0, 2);
    MentSetModelMotionIfChanged(object, 5, 5, 0, 2);
    if (itemIndex != 5) {
        Hu3DModelPosSet(object->mdlId[4], 0.0f, 80.0f, 460.0f);
        Hu3DModelHookSet(object->mdlId[4], "partybox_fix2-effect_fook1", object->mdlId[5]);
    }
    else {
        Hu3DModelPosSet(object->mdlId[4], 1200.0f, 80.0f, 460.0f);
        Hu3DModelHookSet(object->mdlId[4], "koopabox_fix-effect_fook2", object->mdlId[5]);
    }
    Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
    Hu3DModelShadowSet(object->mdlId[4]);
}

void MentCreateBoardMenuObjects(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x61));
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x69));
    if (menuLaunchParameters[0] < 0xA) {
        object->mdlId[3] = Hu3DModelCreateFile(menuLaunchParameters[0] + DATA_MAKE_NUM(DATADIR_MENT, 0x63));
    }
    else {
        object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x64));
    }
    if ((menuLaunchParameters[4] == 1) && ((menuLaunchParameters[0] == 0) || (menuLaunchParameters[0] == 0x14))) {
        object->mdlId[6] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x6C));
        Hu3DModelLayerSet(object->mdlId[6], 2);
        Hu3DModelPosSet(object->mdlId[6], 50.0f, 0.0f, -100.0f);
        object->mdlId[7] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x6D));
        Hu3DModelLayerSet(object->mdlId[7], 2);
        Hu3DModelPosSet(object->mdlId[7], 1150.0f, 0.0f, -100.0f);
    }
    object->mtnId[0] = Hu3DMotionIDGet(object->mdlId[1]);
    object->mtnId[1] = Hu3DMotionIDGet(object->mdlId[3]);
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MENT, 0x6A));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MENT, 0x6B));
    if (menuLaunchParameters[0] == 0xA) {
        MentSetModelMotionIfChanged(object, 2, 3, 0, 2);
    }
    else {
        MentSetModelMotionIfChanged(object, 2, 3, 0, 2);
    }
    MentSetModelMotionIfChanged(object, 3, 1, 0, 1);
    MentSetModelMotionIfChanged(object, 1, 0, 0, 2);
    Hu3DModelShadowMapObjSet(object->mdlId[1], "base_fix9-base");
}

void MentCreateCharacterMenuObjects(OMOBJ *object, s32 unusedArg1, s32 unusedArg2, s32 unusedArg3)
{

    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x62));
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x69));
    object->mtnId[1] = Hu3DMotionIDGet(object->mdlId[1]);
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MENT, 0x6A));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[2], DATA_MAKE_NUM(DATADIR_MENT, 0x6B));
    if (menuLaunchParameters[0] == 0xB) {
        MentSetModelMotionIfChanged(object, 2, 3, 0, 2);
    }
    else {
        MentSetModelMotionIfChanged(object, 2, 2, 0, 2);
        Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    }
    MentSetModelMotionIfChanged(object, 1, 1, 0, 1);
    Hu3DModelPosSet(object->mdlId[1], 1200.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(object->mdlId[2], 1200.0f, 0.0f, 0.0f);
    if (menuLaunchParameters[0] == 3) {
        object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x12));
        object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x14));
        object->mtnId[4] = Hu3DMotionIDGet(object->mdlId[4]);
        object->mtnId[5] = Hu3DMotionIDGet(object->mdlId[5]);
        MentSetModelMotionIfChanged(object, 4, 4, 0, 2);
        MentSetModelMotionIfChanged(object, 5, 5, 0, 2);
        Hu3DModelPosSet(object->mdlId[4], 1200.0f, 80.0f, 460.0f);
        Hu3DModelHookSet(object->mdlId[4], "koopabox_fix-effect_fook2", object->mdlId[5]);
        Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
        Hu3DModelShadowSet(object->mdlId[4]);
        Hu3DModelAttrReset(object->mdlId[4], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(object->mdlId[5], HU3D_ATTR_DISPOFF);
        playerSelectionWork[1].updateCallback = (MentDllUpdateCallback)MentUpdateMenuItemAnimation;
    }
    Hu3DModelShadowMapObjSet(object->mdlId[1], "base_story-base");
    if (menuLaunchParameters[0] == 0xB) {
        object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x12));
        object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x14));
        object->mtnId[4] = Hu3DMotionIDGet(object->mdlId[4]);
        object->mtnId[5] = Hu3DMotionIDGet(object->mdlId[5]);
        MentSetModelMotionIfChanged(object, 4, 4, 0, 2);
        MentSetModelMotionIfChanged(object, 5, 5, 0, 2);
        Hu3DModelPosSet(object->mdlId[4], 1200.0f, 80.0f, 460.0f);
        Hu3DModelHookSet(object->mdlId[4], "koopabox_fix-effect_fook2", object->mdlId[5]);
        Hu3DModelAttrSet(object->mdlId[4], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(object->mdlId[5], HU3D_ATTR_DISPOFF);
        Hu3DModelShadowSet(object->mdlId[4]);
    }
}

void MentCreateMenuBackgroundObjects(void)
{
    OMOBJ *createMenuBackgroundObjectsObject;
    s32 createMenuBackgroundObjectsStateValue;
    s32 createMenuBackgroundObjectsStateValue2;

    createMenuBackgroundObjectsStateValue = specialMenuMode;
    specialMenuMode = createMenuBackgroundObjectsStateValue + 1;
    createMenuBackgroundObjectsObject = omAddObjEx(menuObjectManagerProcess, 0x100, 0x10, 0x10, -1, NULL);
    playerSelectionWork[createMenuBackgroundObjectsStateValue].ownerObject = createMenuBackgroundObjectsObject;
    playerSelectionWork[createMenuBackgroundObjectsStateValue].updateCallback = NULL;
    if (createMenuBackgroundObjectsStateValue == 0) {
        MentCreateBoardMenuObjects(createMenuBackgroundObjectsObject);
        switch (menuLaunchParameters[0]) {
            case 0:
                MentAnimatePlayerCountSelection(&playerSelectionWork[createMenuBackgroundObjectsStateValue], 0, 0, 0);
                MentAnimatePlayerSelection(&playerSelectionWork[createMenuBackgroundObjectsStateValue], 0, 0, 0);
                MentAnimatePlayerSelectionCursor(&playerSelectionWork[createMenuBackgroundObjectsStateValue], 0, 0, 0);
                MentAnimatePlayerSelectionTransition(&playerSelectionWork[createMenuBackgroundObjectsStateValue], 0, 0, 0);
                MentAnimatePlayerMenuTransition(&playerSelectionWork[createMenuBackgroundObjectsStateValue], 0, 0, 0);
                break;
            case 1:
                MentAnimateCharacterSelectionTransition(&playerSelectionWork[createMenuBackgroundObjectsStateValue], 0, 0, 0);
                MentAnimateMenuOptionTransition(&playerSelectionWork[createMenuBackgroundObjectsStateValue], 0, 0, 0);
                MentAnimatePlayerSelectionTransition(&playerSelectionWork[createMenuBackgroundObjectsStateValue], 0, 0, 0);
                break;
            case 10:
                if (GWSystem.mg_list == 3) {
                    GWSystem.mg_list = 0;
                }
                createMenuBackgroundObjectsStateValue2 = GWSystem.mg_list;
                playerSelectionWork[0].selectionFlags[1] = createMenuBackgroundObjectsStateValue2;
                playerSelectionWork[0].selectionFlags[0] = GWPlayerCfg[0].diff;
                MentAnimateMenuOptionTransition(&playerSelectionWork[createMenuBackgroundObjectsStateValue], 0, 0, 0);
                MentAnimatePlayerSelectionTransition(&playerSelectionWork[createMenuBackgroundObjectsStateValue], 0, 0, 0);
                break;
            case 5:
                MentAnimatePlayerCountSelection(&playerSelectionWork[createMenuBackgroundObjectsStateValue], 0, 0, 0);
                MentAnimatePlayerSelection(&playerSelectionWork[createMenuBackgroundObjectsStateValue], 0, 0, 0);
                break;
        }
    }
    else {
        MentCreateCharacterMenuObjects(createMenuBackgroundObjectsObject, 0, 0, 0);
        if (menuLaunchParameters[0] == 0xB) {
            MentAnimateMenuOptionTransition(playerSelectionWork, 0, 0, 0);
            MentAnimatePlayerSelectionTransition(playerSelectionWork, 0, 0, 0);
        }
    }
    while (1) {
        MentSleepFrame();
        if (playerSelectionWork[createMenuBackgroundObjectsStateValue].updateCallback == NULL) {
            continue;
        }
        playerSelectionWork[createMenuBackgroundObjectsStateValue].updateCallback(createMenuBackgroundObjectsObject, &playerSelectionWork[createMenuBackgroundObjectsStateValue]);
    }
}

void MentUpdateMenuItemObject(OMOBJ *object, MentDllMenuItemWork *menuItemWork)
{
    float updateMenuItemObjectInterpolationValue;
    float updateMenuItemObjectInterpolationValue2;

    updateMenuItemObjectInterpolationValue = 0.0f;
    updateMenuItemObjectInterpolationValue2 = 0.0f;
    updateMenuItemObjectInterpolationValue = Hu3DData[object->mdlId[1]].pos.z;
    updateMenuItemObjectInterpolationValue2 = Hu3DData[object->mdlId[1]].rot.y;
    if (menuItemWork->isVisible == 1) {
        if (updateMenuItemObjectInterpolationValue > 560.0f) {
            menuItemWork->isVisible = 2;
            MentSetModelMotionIfChanged(object, 1, 1, 0xF, 1);
        }
        else {
            updateMenuItemObjectInterpolationValue += 5.0f;
            MentSetModelMotionIfChanged(object, 1, 2, 0xF, 1);
            updateMenuItemObjectInterpolationValue2 = MentApproachValue(updateMenuItemObjectInterpolationValue2, 0.0f, 10.0f);
        }
    }
    else if (menuItemWork->isVisible == 0) {
        if (updateMenuItemObjectInterpolationValue <= 460.0f) {
            menuItemWork->isVisible = 2;
            MentSetModelMotionIfChanged(object, 1, 1, 0xF, 1);
        }
        else {
            updateMenuItemObjectInterpolationValue -= 5.0f;
            MentSetModelMotionIfChanged(object, 1, 2, 0xF, 1);
            updateMenuItemObjectInterpolationValue2 = MentApproachValue(updateMenuItemObjectInterpolationValue2, menuCameraYawAngles[menuItemWork->selectionIndex], 10.0f);
        }
    }
    Hu3DData[object->mdlId[1]].pos.z = updateMenuItemObjectInterpolationValue;
    Hu3DData[object->mdlId[1]].rot.y = updateMenuItemObjectInterpolationValue2;
}

void MentCreateMenuCharacterModels(OMOBJ *object, s32 modelIndex)
{
    s32 createMenuCharacterModelsMenuValue[6] = {
        DATA_MAKE_NUM(DATADIR_MENT, 0x6E),
        DATA_MAKE_NUM(DATADIR_MENT, 0x75),
        DATA_MAKE_NUM(DATADIR_MENT, 0x7C),
        DATA_MAKE_NUM(DATADIR_MENT, 0x83),
        DATA_MAKE_NUM(DATADIR_MENT, 0x89),
        DATA_MAKE_NUM(DATADIR_MENT, 0x90),
    };
    s32 modelAnimationFileIndices[6][6] = {
        DATA_MAKE_NUM(DATADIR_MENT, 0x6F),
        DATA_MAKE_NUM(DATADIR_MENT, 0x70),
        DATA_MAKE_NUM(DATADIR_MENT, 0x71),
        DATA_MAKE_NUM(DATADIR_MENT, 0x72),
        DATA_MAKE_NUM(DATADIR_MENT, 0x73),
        DATA_MAKE_NUM(DATADIR_MENT, 0x74),
        DATA_MAKE_NUM(DATADIR_MENT, 0x76),
        DATA_MAKE_NUM(DATADIR_MENT, 0x77),
        DATA_MAKE_NUM(DATADIR_MENT, 0x78),
        DATA_MAKE_NUM(DATADIR_MENT, 0x79),
        DATA_MAKE_NUM(DATADIR_MENT, 0x7A),
        DATA_MAKE_NUM(DATADIR_MENT, 0x7B),
        DATA_MAKE_NUM(DATADIR_MENT, 0x7D),
        DATA_MAKE_NUM(DATADIR_MENT, 0x7E),
        DATA_MAKE_NUM(DATADIR_MENT, 0x7F),
        DATA_MAKE_NUM(DATADIR_MENT, 0x80),
        DATA_MAKE_NUM(DATADIR_MENT, 0x81),
        DATA_MAKE_NUM(DATADIR_MENT, 0x82),
        DATA_MAKE_NUM(DATADIR_MENT, 0x84),
        DATA_MAKE_NUM(DATADIR_MENT, 0x85),
        DATA_MAKE_NUM(DATADIR_MENT, 0x85),
        DATA_MAKE_NUM(DATADIR_MENT, 0x86),
        DATA_MAKE_NUM(DATADIR_MENT, 0x87),
        DATA_MAKE_NUM(DATADIR_MENT, 0x88),
        DATA_MAKE_NUM(DATADIR_MENT, 0x8A),
        DATA_MAKE_NUM(DATADIR_MENT, 0x8B),
        DATA_MAKE_NUM(DATADIR_MENT, 0x8C),
        DATA_MAKE_NUM(DATADIR_MENT, 0x8D),
        DATA_MAKE_NUM(DATADIR_MENT, 0x8E),
        DATA_MAKE_NUM(DATADIR_MENT, 0x8F),
        DATA_MAKE_NUM(DATADIR_MENT, 0x91),
        DATA_MAKE_NUM(DATADIR_MENT, 0x92),
        DATA_MAKE_NUM(DATADIR_MENT, 0x93),
        DATA_MAKE_NUM(DATADIR_MENT, 0x94),
        DATA_MAKE_NUM(DATADIR_MENT, 0x95),
        DATA_MAKE_NUM(DATADIR_MENT, 0x91),
    };
    object->mdlId[1] = Hu3DModelCreateFile(createMenuCharacterModelsMenuValue[modelIndex]);
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], modelAnimationFileIndices[modelIndex][0]);
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], modelAnimationFileIndices[modelIndex][1]);
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], modelAnimationFileIndices[modelIndex][2]);
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], modelAnimationFileIndices[modelIndex][3]);
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], modelAnimationFileIndices[modelIndex][4]);
    object->mtnId[6] = Hu3DJointMotionFile(object->mdlId[1], modelAnimationFileIndices[modelIndex][5]);
    {
        s32 createMenuCharacterModelsMenuValue2[6] = { 9, 14, 10, 11, 12, 13 };
        if (modelIndex != 3) {
            CharNpcDustSet(object->mdlId[1], object->mtnId[2], 0, createMenuCharacterModelsMenuValue2[modelIndex]);
            CharNpcDustSet(object->mdlId[1], object->mtnId[3], 1, createMenuCharacterModelsMenuValue2[modelIndex]);
        }
    }
    MentSetModelMotionIfChanged(object, 1, 1, 0, 1);
    Hu3DModelPosSet(object->mdlId[1], menuCameraPositions[modelIndex].x, menuCameraPositions[modelIndex].y, menuCameraPositions[modelIndex].z);
    Hu3DModelRotSet(object->mdlId[1], 0.0f, menuCameraYawAngles[modelIndex], 0.0f);
    if (menuLaunchParameters[0] >= 2 && menuLaunchParameters[0] < 0xA) {
        Hu3DModelPosSet(object->mdlId[1], menuCameraPositions[0].x, menuCameraPositions[0].y, menuCameraPositions[0].z);
        Hu3DModelRotSet(object->mdlId[1], 0.0f, 0.0f, 0.0f);
        if (menuLaunchParameters[0] == 5) {
            Hu3DModelPosSet(object->mdlId[1], menuCameraPositions[0].x, 50.0f + menuCameraPositions[0].y, menuCameraPositions[0].z);
        }
    }
    if (menuLaunchParameters[0] < 0xA) {
        Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    }
    Hu3DModelShadowSet(object->mdlId[1]);
    Hu3DModelLayerSet(object->mdlId[1], 2);
}

void MentCreateMenuItemObjects(void)
{
    OMOBJ *createMenuItemObjectsObject2;
    s32 createMenuItemObjectsStateValue;

    createMenuItemObjectsStateValue = characterSelectionCount;
    characterSelectionCount = createMenuItemObjectsStateValue + 1;
    createMenuItemObjectsObject2 = omAddObjEx(menuObjectManagerProcess, 0x100, 0x10, 0x10, -1, NULL);
    menuItemWork[createMenuItemObjectsStateValue].ownerObject = createMenuItemObjectsObject2;
    menuItemWork[createMenuItemObjectsStateValue].updateCallback = NULL;
    menuItemWork[createMenuItemObjectsStateValue].selectionIndex = createMenuItemObjectsStateValue;
    MentCreateMenuCharacterModels(createMenuItemObjectsObject2, createMenuItemObjectsStateValue);
    while (1) {
        MentSleepFrame();
        if (menuItemWork[createMenuItemObjectsStateValue].updateCallback != NULL) {
            menuItemWork[createMenuItemObjectsStateValue].updateCallback(createMenuItemObjectsObject2, &menuItemWork[createMenuItemObjectsStateValue]);
        }
        if (createMenuItemObjectsObject2->work[0] != 1) {
            continue;
        }
        MentUpdateModelMotionTimer(createMenuItemObjectsObject2);
    }
}

void MentCreatePlayerPortraitModel(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x96));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MENT, 0x97));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MENT, 0x98));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MENT, 0x99));
    MentSetModelMotionIfChanged(object, 1, 3, 0, 1);
    Hu3DModelPosSet(object->mdlId[1], 1020.0f, 0.0f, 800.0f);
    Hu3DModelRotSet(object->mdlId[1], 0.0f, 60.0f, 0.0f);
    Hu3DModelShadowSet(object->mdlId[1]);
    Hu3DModelLayerSet(object->mdlId[1], 2);
}

void MentCreateMenuEffectObject(void)
{
    OMOBJ *createMenuEffectObjectObject2;

    createMenuEffectObjectObject2 = omAddObjEx(menuObjectManagerProcess, 0x100, 0x10, 0x10, -1, NULL);
    selectedMenuItemWork.ownerObject = createMenuEffectObjectObject2;
    selectedMenuItemWork.updateCallback = NULL;
    MentCreatePlayerPortraitModel(createMenuEffectObjectObject2);
    while (1) {
        MentSleepFrame();
        if (selectedMenuItemWork.updateCallback != NULL) {
            selectedMenuItemWork.updateCallback(createMenuEffectObjectObject2, &selectedMenuItemWork);
        }
        if (createMenuEffectObjectObject2->work[0] != 1) {
            continue;
        }
        MentUpdateModelMotionTimer(createMenuEffectObjectObject2);
    }
}

void MentUpdateRecordPlayerDisplay(OMOBJ *object, s32 playerIndex)
{
    MentDllRecordPlayerWork *updateRecordPlayerDisplayRecordPlayer;

    updateRecordPlayerDisplayRecordPlayer = &recordPlayerWork[playerIndex];
    updateRecordPlayerDisplayRecordPlayer->recordMinutes = playerIndex;
    updateRecordPlayerDisplayRecordPlayer->recordSeconds = 0;
    updateRecordPlayerDisplayRecordPlayer->isComputerPlayer = updateRecordPlayerDisplayRecordPlayer->recordMinutes;
    if (updateRecordPlayerDisplayRecordPlayer->isComputerPlayer > 0) {
        updateRecordPlayerDisplayRecordPlayer->isComputerPlayer = 1;
    }
    updateRecordPlayerDisplayRecordPlayer->difficulty = 0;
    updateRecordPlayerDisplayRecordPlayer->character = updateRecordPlayerDisplayRecordPlayer->recordMinutes;
    updateRecordPlayerDisplayRecordPlayer->controllerPort = updateRecordPlayerDisplayRecordPlayer->recordMinutes;
}

void MentUpdateRecordPlayerCharacter(OMOBJ *object, s32 playerIndex)
{
    MentDllRecordPlayerWork *updateRecordPlayerCharacterRecordPlayer;

    updateRecordPlayerCharacterRecordPlayer = &recordPlayerWork[playerIndex];
    updateRecordPlayerCharacterRecordPlayer->recordMinutes = playerIndex;
    updateRecordPlayerCharacterRecordPlayer->recordSeconds = GWPlayerCfg[updateRecordPlayerCharacterRecordPlayer->recordMinutes].group;
    updateRecordPlayerCharacterRecordPlayer->isComputerPlayer = GWPlayerCfg[updateRecordPlayerCharacterRecordPlayer->recordMinutes].iscom;
    updateRecordPlayerCharacterRecordPlayer->difficulty = GWPlayerCfg[updateRecordPlayerCharacterRecordPlayer->recordMinutes].diff;
    updateRecordPlayerCharacterRecordPlayer->character = GWPlayerCfg[updateRecordPlayerCharacterRecordPlayer->recordMinutes].character;
    updateRecordPlayerCharacterRecordPlayer->controllerPort = GWPlayerCfg[updateRecordPlayerCharacterRecordPlayer->recordMinutes].pad_idx;
}

void MentUpdateRecordPlayerDifficulty(OMOBJ *object, s32 playerIndex)
{
    MentDllRecordPlayerWork *updateRecordPlayerDifficultyRecordPlayer;

    updateRecordPlayerDifficultyRecordPlayer = &recordPlayerWork[playerIndex];
    updateRecordPlayerDifficultyRecordPlayer->recordMinutes = playerIndex;
    updateRecordPlayerDifficultyRecordPlayer->recordSeconds = GWPlayerCfg[updateRecordPlayerDifficultyRecordPlayer->recordMinutes].group;
    updateRecordPlayerDifficultyRecordPlayer->isComputerPlayer = GWPlayerCfg[updateRecordPlayerDifficultyRecordPlayer->recordMinutes].iscom;
    updateRecordPlayerDifficultyRecordPlayer->difficulty = GWPlayerCfg[updateRecordPlayerDifficultyRecordPlayer->recordMinutes].diff;
    updateRecordPlayerDifficultyRecordPlayer->character = GWPlayerCfg[updateRecordPlayerDifficultyRecordPlayer->recordMinutes].character;
    updateRecordPlayerDifficultyRecordPlayer->controllerPort = GWPlayerCfg[updateRecordPlayerDifficultyRecordPlayer->recordMinutes].pad_idx;
    object->mdlId[1] = CharModelCreate(updateRecordPlayerDifficultyRecordPlayer->character, 1);
    object->mtnId[1] = CharMotionCreate(updateRecordPlayerDifficultyRecordPlayer->character, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], updateRecordPlayerDifficultyRecordPlayer->character + DATA_MAKE_NUM(DATADIR_MENT, 0x00));
    MentSetModelMotionIfChanged(object, 1, 2, 0, 1);
    Hu3DModelPosSet(object->mdlId[1], 1320.0f, 0.0f, 800.0f);
    Hu3DModelRotSet(object->mdlId[1], 0.0f, -60.0f, 0.0f);
    Hu3DModelShadowSet(object->mdlId[1]);
    Hu3DModelLayerSet(object->mdlId[1], 2);
}

void MentCreateRecordPlayerObjects(void)
{
    MentDllRecordPlayerWork *createRecordPlayerObjectsRecordPlayerWork;
    OMOBJ *createRecordPlayerObjectsObject;
    s32 createRecordPlayerObjectsStateValue;

    createRecordPlayerObjectsStateValue = recordPlayerIndex;
    recordPlayerIndex = createRecordPlayerObjectsStateValue + 1;
    createRecordPlayerObjectsObject = omAddObjEx(menuObjectManagerProcess, 0x100, 0x10, 0x10, -1, NULL);
    recordPlayerWork[createRecordPlayerObjectsStateValue].ownerObject = createRecordPlayerObjectsObject;
    recordPlayerWork[createRecordPlayerObjectsStateValue].updateCallback = NULL;
    if (menuLaunchParameters[0] <= 1) {
        MentUpdateRecordPlayerDisplay(createRecordPlayerObjectsObject, createRecordPlayerObjectsStateValue);
    }
    else if (menuLaunchParameters[0] == 0xA) {
        MentUpdateRecordPlayerCharacter(createRecordPlayerObjectsObject, 0);
    }
    else if (menuLaunchParameters[0] == 0xB) {
        MentUpdateRecordPlayerDifficulty(createRecordPlayerObjectsObject, 0);
    }
    else {
        MentUpdateRecordPlayerDisplay(createRecordPlayerObjectsObject, createRecordPlayerObjectsStateValue);
    }
    while (1) {
        MentSleepFrame();
        if (recordPlayerWork[createRecordPlayerObjectsStateValue].updateCallback == NULL) {
            continue;
        }
        recordPlayerWork[createRecordPlayerObjectsStateValue].updateCallback(createRecordPlayerObjectsObject, &recordPlayerWork[createRecordPlayerObjectsStateValue]);
    }
}

void MentSetDefaultMenuCamera(void)
{
    MentDllCameraWork *setDefaultMenuCameraCamera;

    setDefaultMenuCameraCamera = &menuCameraWork;
    setDefaultMenuCameraCamera->center.x = 0.0f;
    setDefaultMenuCameraCamera->center.y = 0.0f;
    setDefaultMenuCameraCamera->center.z = 0.0f;
    setDefaultMenuCameraCamera->rot.x = -10.0f;
    setDefaultMenuCameraCamera->rot.y = 0.0f;
    setDefaultMenuCameraCamera->rot.z = 0.0f;
    setDefaultMenuCameraCamera->zoom = 2900.0f;
}

void MentZoomMenuCameraIn(void)
{
    MentDllCameraWork zoomMenuCameraInCameraTargetState;
    MentDllCameraWork *zoomMenuCameraInCamera;

    zoomMenuCameraInCamera = &menuCameraWork;
    zoomMenuCameraInCameraTargetState.center.x = 0.0f;
    zoomMenuCameraInCameraTargetState.center.y = 0.0f;
    zoomMenuCameraInCameraTargetState.center.z = 0.0f;
    zoomMenuCameraInCameraTargetState.rot.x = -10.0f;
    zoomMenuCameraInCameraTargetState.rot.y = 0.0f;
    zoomMenuCameraInCameraTargetState.rot.z = 0.0f;
    zoomMenuCameraInCameraTargetState.zoom = 1150.0f;
    MentAnimateCameraState(zoomMenuCameraInCamera, &zoomMenuCameraInCameraTargetState, zoomMenuCameraInCamera->transitionFrame++, 180.0f, 15.0f);
}

void MentSetAlternateMenuCamera(void)
{
    MentDllCameraWork *setAlternateMenuCameraCamera;

    setAlternateMenuCameraCamera = &menuCameraWork;
    setAlternateMenuCameraCamera->center.x = 0.0f;
    setAlternateMenuCameraCamera->center.y = 215.0f;
    setAlternateMenuCameraCamera->center.z = 0.0f;
    setAlternateMenuCameraCamera->rot.x = 0.0f;
    setAlternateMenuCameraCamera->rot.y = 0.0f;
    setAlternateMenuCameraCamera->rot.z = 0.0f;
    setAlternateMenuCameraCamera->zoom = 1600.0f;
}

void MentZoomAlternateMenuCameraIn(void)
{
    MentDllCameraWork zoomAlternateMenuCameraInCameraTargetState;
    MentDllCameraWork *zoomAlternateMenuCameraInCamera;

    zoomAlternateMenuCameraInCamera = &menuCameraWork;
    zoomAlternateMenuCameraInCameraTargetState.center.x = 0.0f;
    zoomAlternateMenuCameraInCameraTargetState.center.y = 215.0f;
    zoomAlternateMenuCameraInCameraTargetState.center.z = 0.0f;
    zoomAlternateMenuCameraInCameraTargetState.rot.x = 0.0f;
    zoomAlternateMenuCameraInCameraTargetState.rot.y = 0.0f;
    zoomAlternateMenuCameraInCameraTargetState.rot.z = 0.0f;
    zoomAlternateMenuCameraInCameraTargetState.zoom = 1600.0f;
    MentAnimateCameraState(zoomAlternateMenuCameraInCamera, &zoomAlternateMenuCameraInCameraTargetState, zoomAlternateMenuCameraInCamera->transitionFrame++, 60.0f, 10.0f);
}

void MentAnimateMenuCameraToFront(void)
{
    MentDllCameraWork animateMenuCameraToFrontCameraTargetState;
    MentDllCameraWork *animateMenuCameraToFrontCamera;

    animateMenuCameraToFrontCamera = &menuCameraWork;
    animateMenuCameraToFrontCameraTargetState.center.x = 0.0f;
    animateMenuCameraToFrontCameraTargetState.center.y = 215.0f;
    animateMenuCameraToFrontCameraTargetState.center.z = 0.0f;
    animateMenuCameraToFrontCameraTargetState.rot.x = 0.0f;
    animateMenuCameraToFrontCameraTargetState.rot.y = 0.0f;
    animateMenuCameraToFrontCameraTargetState.rot.z = 0.0f;
    animateMenuCameraToFrontCameraTargetState.zoom = 1600.0f;
    MentAnimateCameraState(animateMenuCameraToFrontCamera, &animateMenuCameraToFrontCameraTargetState, animateMenuCameraToFrontCamera->transitionFrame++, 10.0f, 5.0f);
}

void MentAnimateMenuCameraToBoard(void)
{
    MentDllCameraWork animateMenuCameraToBoardCameraTargetState;
    MentDllCameraWork *animateMenuCameraToBoardCamera;

    animateMenuCameraToBoardCamera = &menuCameraWork;
    animateMenuCameraToBoardCameraTargetState.center.x = 1200.0f;
    animateMenuCameraToBoardCameraTargetState.center.y = 215.0f;
    animateMenuCameraToBoardCameraTargetState.center.z = 0.0f;
    animateMenuCameraToBoardCameraTargetState.rot.x = 0.0f;
    animateMenuCameraToBoardCameraTargetState.rot.y = 0.0f;
    animateMenuCameraToBoardCameraTargetState.rot.z = 0.0f;
    animateMenuCameraToBoardCameraTargetState.zoom = 1600.0f;
    MentAnimateCameraState(animateMenuCameraToBoardCamera, &animateMenuCameraToBoardCameraTargetState, animateMenuCameraToBoardCamera->transitionFrame++, 10.0f, 5.0f);
}

void MentAnimateMenuCameraToDefaultView(void)
{
    MentDllCameraWork animateMenuCameraToDefaultViewCameraTargetState;
    MentDllCameraWork *animateMenuCameraToDefaultViewCamera;

    animateMenuCameraToDefaultViewCamera = &menuCameraWork;
    animateMenuCameraToDefaultViewCameraTargetState.center.x = 0.0f;
    animateMenuCameraToDefaultViewCameraTargetState.center.y = 125.0f;
    animateMenuCameraToDefaultViewCameraTargetState.center.z = 0.0f;
    animateMenuCameraToDefaultViewCameraTargetState.rot.x = -5.0f;
    animateMenuCameraToDefaultViewCameraTargetState.rot.y = 0.0f;
    animateMenuCameraToDefaultViewCameraTargetState.rot.z = 0.0f;
    animateMenuCameraToDefaultViewCameraTargetState.zoom = 1750.0f;
    MentAnimateCameraState(animateMenuCameraToDefaultViewCamera, &animateMenuCameraToDefaultViewCameraTargetState, animateMenuCameraToDefaultViewCamera->transitionFrame++, 30.0f, 10.0f);
}

void MentAnimateMenuCameraAcrossBoard(void)
{
    MentDllCameraWork animateMenuCameraAcrossBoardCameraTargetState;
    MentDllCameraWork *animateMenuCameraAcrossBoardCamera;

    animateMenuCameraAcrossBoardCamera = &menuCameraWork;
    animateMenuCameraAcrossBoardCameraTargetState.center.x = 1200.0f;
    animateMenuCameraAcrossBoardCameraTargetState.center.y = 125.0f;
    animateMenuCameraAcrossBoardCameraTargetState.center.z = 0.0f;
    animateMenuCameraAcrossBoardCameraTargetState.rot.x = -5.0f;
    animateMenuCameraAcrossBoardCameraTargetState.rot.y = 0.0f;
    animateMenuCameraAcrossBoardCameraTargetState.rot.z = 0.0f;
    animateMenuCameraAcrossBoardCameraTargetState.zoom = 1750.0f;
    MentAnimateCameraState(animateMenuCameraAcrossBoardCamera, &animateMenuCameraAcrossBoardCameraTargetState, animateMenuCameraAcrossBoardCamera->transitionFrame++, 30.0f, 10.0f);
}

void MentAnimateMenuCameraAroundPlayer(void)
{
    MentDllCameraWork animateMenuCameraAroundPlayerCameraTargetState;
    MentDllCameraWork *animateMenuCameraAroundPlayerCamera;
    s32 animateMenuCameraAroundPlayerAnimationFrame;

    animateMenuCameraAroundPlayerAnimationFrame = menuLaunchParameters[2] / 5;
    animateMenuCameraAroundPlayerCamera = &menuCameraWork;
    if (animateMenuCameraAroundPlayerCamera->transitionFrame == 0x32) {
        MentSetModelMotion(playerSelectionWork[animateMenuCameraAroundPlayerAnimationFrame].ownerObject, 4, 4, 0, 0);
        MentSetModelMotion(playerSelectionWork[animateMenuCameraAroundPlayerAnimationFrame].ownerObject, 5, 5, 0, 0);
        HuAudFXPlay(0x7D);
    }
    animateMenuCameraAroundPlayerCameraTargetState.center.x = 0.0f;
    if (animateMenuCameraAroundPlayerAnimationFrame == 1) {
        animateMenuCameraAroundPlayerCameraTargetState.center.x = 1200.0f;
    }
    animateMenuCameraAroundPlayerCameraTargetState.center.y = 100.0f;
    animateMenuCameraAroundPlayerCameraTargetState.center.z = 460.0f;
    animateMenuCameraAroundPlayerCameraTargetState.rot.x = 0.0f;
    animateMenuCameraAroundPlayerCameraTargetState.rot.y = 0.0f;
    animateMenuCameraAroundPlayerCameraTargetState.rot.z = 0.0f;
    animateMenuCameraAroundPlayerCameraTargetState.zoom = 125.0f;
    MentAnimateCameraState(animateMenuCameraAroundPlayerCamera, &animateMenuCameraAroundPlayerCameraTargetState, animateMenuCameraAroundPlayerCamera->transitionFrame, 180.0f, 10.0f);
    if (animateMenuCameraAroundPlayerCamera->transitionFrame++ >= 90) {
        if (animateMenuCameraAroundPlayerCamera->transitionFrame == 91) {
            if (animateMenuCameraAroundPlayerAnimationFrame == 0) {
                menuSoundEffectHandles[10] = HuAudFXPlay(0x22);
            }
            else {
                menuSoundEffectHandles[10] = HuAudFXPlay(0x23);
            }
        }
        animateMenuCameraAroundPlayerCamera->rot.x = MentInterpolateLinear(animateMenuCameraAroundPlayerCamera->rot.x, -45.0f, animateMenuCameraAroundPlayerCamera->transitionFrame - 0x5A, 90.0f);
        Hu3DData[playerSelectionWork[animateMenuCameraAroundPlayerAnimationFrame].ownerObject->mdlId[4]].rot.x = MentInterpolateLinear(0.0f, 45.0f, animateMenuCameraAroundPlayerCamera->playerCount - 0x5A, 90.0f);
    }
}

void MentFocusMenuCameraOnPlayer(void)
{
    MentDllCameraWork focusMenuCameraOnPlayerCameraTargetState;
    MentDllCameraWork *focusMenuCameraOnPlayerCamera;

    focusMenuCameraOnPlayerCamera = &menuCameraWork;
    focusMenuCameraOnPlayerCameraTargetState.center.x = 0.0f;
    focusMenuCameraOnPlayerCameraTargetState.center.y = 215.0f;
    focusMenuCameraOnPlayerCameraTargetState.center.z = 0.0f;
    focusMenuCameraOnPlayerCameraTargetState.rot.x = 0.0f;
    focusMenuCameraOnPlayerCameraTargetState.rot.y = 0.0f;
    focusMenuCameraOnPlayerCameraTargetState.rot.z = 0.0f;
    focusMenuCameraOnPlayerCameraTargetState.zoom = 1600.0f;
    MentAnimateCameraZoom(focusMenuCameraOnPlayerCamera, &focusMenuCameraOnPlayerCameraTargetState, focusMenuCameraOnPlayerCamera->transitionFrame++, 30.0f, 10.0f);
}

void MentAnimateMenuCameraReset(void)
{
    MentDllCameraWork animateMenuCameraResetCameraTargetState;
    MentDllCameraWork *animateMenuCameraResetCamera;

    animateMenuCameraResetCamera = &menuCameraWork;
    animateMenuCameraResetCameraTargetState.center.x = 1200.0f;
    animateMenuCameraResetCameraTargetState.center.y = 215.0f;
    animateMenuCameraResetCameraTargetState.center.z = 0.0f;
    animateMenuCameraResetCameraTargetState.rot.x = 0.0f;
    animateMenuCameraResetCameraTargetState.rot.y = 0.0f;
    animateMenuCameraResetCameraTargetState.rot.z = 0.0f;
    animateMenuCameraResetCameraTargetState.zoom = 1600.0f;
    MentAnimateCameraZoom(animateMenuCameraResetCamera, &animateMenuCameraResetCameraTargetState, animateMenuCameraResetCamera->transitionFrame++, 30.0f, 10.0f);
}

void MentAnimateMenuCameraToCloseView(void)
{
    MentDllCameraWork animateMenuCameraToCloseViewCameraTargetState;
    MentDllCameraWork *animateMenuCameraToCloseViewCamera;

    animateMenuCameraToCloseViewCamera = &menuCameraWork;
    animateMenuCameraToCloseViewCameraTargetState.center.x = 0.0f;
    animateMenuCameraToCloseViewCameraTargetState.center.y = 30.0f;
    animateMenuCameraToCloseViewCameraTargetState.center.z = 0.0f;
    animateMenuCameraToCloseViewCameraTargetState.rot.x = -5.0f;
    animateMenuCameraToCloseViewCameraTargetState.rot.y = 0.0f;
    animateMenuCameraToCloseViewCameraTargetState.rot.z = 0.0f;
    animateMenuCameraToCloseViewCameraTargetState.zoom = 1100.0f;
    MentAnimateCameraState(animateMenuCameraToCloseViewCamera, &animateMenuCameraToCloseViewCameraTargetState, animateMenuCameraToCloseViewCamera->transitionFrame++, 10.0f, 5.0f);
}

void MentResetMenuCamera(void)
{
    MentDllCameraWork *resetMenuCameraCamera;

    resetMenuCameraCamera = &menuCameraWork;
    resetMenuCameraCamera->center.x = 0.0f;
    resetMenuCameraCamera->center.y = 125.0f;
    resetMenuCameraCamera->center.z = 0.0f;
    resetMenuCameraCamera->rot.x = -5.0f;
    resetMenuCameraCamera->rot.y = 0.0f;
    resetMenuCameraCamera->rot.z = 0.0f;
    resetMenuCameraCamera->zoom = 1750.0f;
}

void MentResetAlternateMenuCamera(void)
{
    MentDllCameraWork *resetAlternateMenuCameraCamera;

    resetAlternateMenuCameraCamera = &menuCameraWork;
    resetAlternateMenuCameraCamera->center.x = 1200.0f;
    resetAlternateMenuCameraCamera->center.y = 125.0f;
    resetAlternateMenuCameraCamera->center.z = 0.0f;
    resetAlternateMenuCameraCamera->rot.x = -5.0f;
    resetAlternateMenuCameraCamera->rot.y = 0.0f;
    resetAlternateMenuCameraCamera->rot.z = 0.0f;
    resetAlternateMenuCameraCamera->zoom = 1750.0f;
}

void MentAnimateMenuCameraToSelectionView(void)
{
    MentDllCameraWork animateMenuCameraToSelectionViewCameraTargetState;
    MentDllCameraWork *animateMenuCameraToSelectionViewCamera;

    animateMenuCameraToSelectionViewCamera = &menuCameraWork;
    animateMenuCameraToSelectionViewCameraTargetState.center.x = 0.0f;
    animateMenuCameraToSelectionViewCameraTargetState.center.y = 125.0f;
    animateMenuCameraToSelectionViewCameraTargetState.center.z = 460.0f;
    animateMenuCameraToSelectionViewCameraTargetState.rot.x = 0.0f;
    animateMenuCameraToSelectionViewCameraTargetState.rot.y = 0.0f;
    animateMenuCameraToSelectionViewCameraTargetState.rot.z = 0.0f;
    animateMenuCameraToSelectionViewCameraTargetState.zoom = 125.0f;
    MentAnimateCameraZoom(animateMenuCameraToSelectionViewCamera, &animateMenuCameraToSelectionViewCameraTargetState, animateMenuCameraToSelectionViewCamera->transitionFrame++, 120.0f, 10.0f);
}

void MentInitializePlayerSelectionSpriteGroup(void)
{
    s32 initializePlayerSelectionSpriteGroupPlayerIndex;
    s32 initializePlayerSelectionSpriteGroupSelectionState;
    initializePlayerSelectionSpriteGroupSelectionState = playerSelectionWork[0].selectionState;
    for (initializePlayerSelectionSpriteGroupPlayerIndex = 0; initializePlayerSelectionSpriteGroupPlayerIndex < 0x35; initializePlayerSelectionSpriteGroupPlayerIndex++) {
        HuSprAttrSet(initializePlayerSelectionSpriteGroupSelectionState, initializePlayerSelectionSpriteGroupPlayerIndex, HUSPR_ATTR_DISPOFF);
    }
    for (initializePlayerSelectionSpriteGroupPlayerIndex = 0; initializePlayerSelectionSpriteGroupPlayerIndex < 8; initializePlayerSelectionSpriteGroupPlayerIndex++) {
        HuSprBankSet(initializePlayerSelectionSpriteGroupSelectionState, initializePlayerSelectionSpriteGroupPlayerIndex, 0);
        HuSprAttrReset(initializePlayerSelectionSpriteGroupSelectionState, initializePlayerSelectionSpriteGroupPlayerIndex, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(initializePlayerSelectionSpriteGroupSelectionState, initializePlayerSelectionSpriteGroupPlayerIndex + 8, HUSPR_ATTR_DISPOFF);
    }
    HuSprAttrReset(initializePlayerSelectionSpriteGroupSelectionState, 0x28, HUSPR_ATTR_DISPOFF);
    for (initializePlayerSelectionSpriteGroupPlayerIndex = 0; initializePlayerSelectionSpriteGroupPlayerIndex < 4; initializePlayerSelectionSpriteGroupPlayerIndex++) {
        HuSprBankSet(initializePlayerSelectionSpriteGroupSelectionState, initializePlayerSelectionSpriteGroupPlayerIndex + 0x10, 0);
        HuSprBankSet(initializePlayerSelectionSpriteGroupSelectionState, initializePlayerSelectionSpriteGroupPlayerIndex + 0x14, 0);
        HuSprAttrReset(initializePlayerSelectionSpriteGroupSelectionState, initializePlayerSelectionSpriteGroupPlayerIndex + 0x29, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(initializePlayerSelectionSpriteGroupSelectionState, initializePlayerSelectionSpriteGroupPlayerIndex + 0x2D, HUSPR_ATTR_DISPOFF);
    }
    HuSprGrpPosSet(initializePlayerSelectionSpriteGroupSelectionState, 0.0f, -500.0f);
}

s32 characterRecordSpritePositions[4][4] = {
    { 0xA8, 0x5A, 0x78, 0x33 },
    { 0xF8, 0xBE, 0x1CA, 0x33 },
    { 0x148, 0x5A, 0x78, 0xDE },
    { 0x198, 0xBE, 0x1CA, 0xDE },
};
s32 characterRecordSpriteOffsets[4][2] = {
    { 0x1B, 0x18 },
    { 0x49, 0x18 },
    { 0x77, 0x18 },
    { 0xA5, 0x18 },
};

#if VERSION_NTSC
float playerSelectionSpritePositions[5][5] = {
    { 262.0f, 119.0f, 1.3f, 227.0f, 455.0f },
    { 314.0f, 168.0f, 0.75f, 265.0f, 416.0f },
    { 366.0f, 204.0f, 0.75f, 240.0f, 440.0f },
    { 418.0f, 240.0f, 0.75f, 300.0f, 381.0f },
    { -1.0f, 289.0f, 1.3f, 220.0f, 460.0f },
};
#else
float playerSelectionSpritePositions[5][5] = {
    { 262.0f, 119.0f, 1.3f, 227.0f, 455.0f },
    { 314.0f, 168.0f, 0.75f, 300.0f, 381.0f },
    { 366.0f, 204.0f, 0.75f, 240.0f, 440.0f },
    { 418.0f, 240.0f, 0.75f, 300.0f, 381.0f },
    { -1.0f, 289.0f, 1.3f, 220.0f, 460.0f },
};
#endif

s32 characterSelectSpritePositions[4][5] = {
    { 0xB4, 0x60, 0x87, 0, 0 },
    { 0xFC, 0xB8, 0xF6, 0xD4, 0x1D4 },
    { 0x144, 0x60, 0x11A, 0xE6, 0x1C2 },
    { 0x18C, 0xB8, 0, 0, 0 },
};
s32 minigameRecordSpritePositions[4][4] = {
    { 0xB1, 0x68, 0x78, 0x37 },
    { 0xFB, 0x68, 0x1CA, 0x37 },
    { 0x145, 0x68, 0x78, 0x8E },
    { 0x18F, 0x68, 0x1CA, 0x8E },
};

void MentAnimateCharacterRecordSpritesIn(void)
{
    float animateCharacterRecordSpritesInAnimationProgress;
    s32 animateCharacterRecordSpritesInPlayerIndex;
    s32 animateCharacterRecordSpritesInAnimationFrame;

    animateCharacterRecordSpritesInAnimationFrame = playerSelectionWork[0].selectionState;
    for (animateCharacterRecordSpritesInPlayerIndex = 0; animateCharacterRecordSpritesInPlayerIndex < 4; animateCharacterRecordSpritesInPlayerIndex++) {
        if (recordPlayerWork[animateCharacterRecordSpritesInPlayerIndex].isComputerPlayer != 0) {
            HuSprAttrSet(animateCharacterRecordSpritesInAnimationFrame, recordPlayerWork[animateCharacterRecordSpritesInPlayerIndex].recordMinutes + 0x10, HUSPR_ATTR_DISPOFF);
            HuSprAttrSet(animateCharacterRecordSpritesInAnimationFrame, recordPlayerWork[animateCharacterRecordSpritesInPlayerIndex].recordMinutes + 0x14, HUSPR_ATTR_DISPOFF);
        }
        else {
            HuSprAttrReset(animateCharacterRecordSpritesInAnimationFrame, recordPlayerWork[animateCharacterRecordSpritesInPlayerIndex].recordMinutes + 0x10, HUSPR_ATTR_DISPOFF);
            HuSprAttrReset(animateCharacterRecordSpritesInAnimationFrame, recordPlayerWork[animateCharacterRecordSpritesInPlayerIndex].recordMinutes + 0x14, HUSPR_ATTR_DISPOFF);
        }
        HuSprPosSet(animateCharacterRecordSpritesInAnimationFrame, recordPlayerWork[animateCharacterRecordSpritesInPlayerIndex].recordMinutes + 0x10, characterRecordSpritePositions[recordPlayerWork[animateCharacterRecordSpritesInPlayerIndex].character % 4][0],
            characterRecordSpritePositions[recordPlayerWork[animateCharacterRecordSpritesInPlayerIndex].character / 4][1]);
        HuSprPosSet(animateCharacterRecordSpritesInAnimationFrame, recordPlayerWork[animateCharacterRecordSpritesInPlayerIndex].recordMinutes + 0x14, characterRecordSpritePositions[recordPlayerWork[animateCharacterRecordSpritesInPlayerIndex].character % 4][0],
            characterRecordSpritePositions[recordPlayerWork[animateCharacterRecordSpritesInPlayerIndex].character / 4][1]);
    }
    for (animateCharacterRecordSpritesInPlayerIndex = 0; animateCharacterRecordSpritesInPlayerIndex <= 60; animateCharacterRecordSpritesInPlayerIndex++) {
        MentSleepFrame();
        if (animateCharacterRecordSpritesInPlayerIndex <= 0x32) {
            animateCharacterRecordSpritesInAnimationProgress = MentInterpolateSineEaseIn(-500.0f, 10.0f, animateCharacterRecordSpritesInPlayerIndex, 50.0f);
        }
        else {
            animateCharacterRecordSpritesInAnimationProgress = MentInterpolateCosineEaseOut(10.0f, 2.06f, animateCharacterRecordSpritesInPlayerIndex - 0x32, 10.0f);
        }
        HuSprGrpPosSet(playerSelectionWork[0].selectionState, 0.0f, animateCharacterRecordSpritesInAnimationProgress);
    }
}

void MentAnimateCharacterRecordSpritesOut(void)
{
    float animateCharacterRecordSpritesOutAnimationProgress;
    s32 animateCharacterRecordSpritesOutPlayerIndex;

    for (animateCharacterRecordSpritesOutPlayerIndex = 0; animateCharacterRecordSpritesOutPlayerIndex <= 0x3C; animateCharacterRecordSpritesOutPlayerIndex++) {
        MentSleepFrame();
        if (animateCharacterRecordSpritesOutPlayerIndex <= 0xA) {
            animateCharacterRecordSpritesOutAnimationProgress = MentInterpolateSineEaseIn(2.06f, 10.0f, animateCharacterRecordSpritesOutPlayerIndex, 10.0f);
        }
        else {
            animateCharacterRecordSpritesOutAnimationProgress = MentInterpolateCosineEaseOut(10.0f, -500.0f, animateCharacterRecordSpritesOutPlayerIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(playerSelectionWork[0].selectionState, 0.0f, animateCharacterRecordSpritesOutAnimationProgress);
    }
}

void MentAnimatePlayerSelection(MentDllPlayerSelectionWork *selectionWork, s32 selectionIndex, s32 playerCount, s32 displayMode)
{
    s32 animatePlayerSelectionPlayerIndex;
    s32 animatePlayerSelectionSpriteGroupId;
    s32 animatePlayerSelectionSpriteId;
    s32 animatePlayerSelectionPlayerIndex2;
    s32 animatePlayerSelectionSelectionState;
    ANIMDATA *animatePlayerSelectionSpriteAnimation1;
    ANIMDATA *animatePlayerSelectionSpriteAnimation2;
    ANIMDATA *animatePlayerSelectionSpriteAnimation3;
    ANIMDATA *animatePlayerSelectionSpriteAnimation4;
    ANIMDATA *animatePlayerSelectionSpriteAnimation5;

    animatePlayerSelectionSpriteGroupId = HuSprGrpCreate(0x35);
    selectionWork->selectionState = animatePlayerSelectionSpriteGroupId;
    animatePlayerSelectionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x22));
    for (animatePlayerSelectionPlayerIndex = 0; animatePlayerSelectionPlayerIndex < 8; animatePlayerSelectionPlayerIndex++) {
        animatePlayerSelectionSpriteId = HuSprCreate(HuSprAnimReadFile(animatePlayerSelectionPlayerIndex + DATA_MAKE_NUM(DATADIR_MENT, 0x1A)), 0x3E8, 0);
        HuSprGrpMemberSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex, animatePlayerSelectionSpriteId);
        HuSprPosSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex, characterRecordSpritePositions[animatePlayerSelectionPlayerIndex % 4][0], characterRecordSpritePositions[animatePlayerSelectionPlayerIndex / 4][1]);
        animatePlayerSelectionSpriteId = HuSprCreate(animatePlayerSelectionSpriteAnimation1, 0x3F2, 0);
        HuSprGrpMemberSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 8, animatePlayerSelectionSpriteId);
        HuSprTPLvlSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 8, 0.5f);
        HuSprPosSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 8, characterRecordSpritePositions[animatePlayerSelectionPlayerIndex % 4][0] + 1, characterRecordSpritePositions[animatePlayerSelectionPlayerIndex / 4][1] + 2);
    }
    for (animatePlayerSelectionPlayerIndex = 0; animatePlayerSelectionPlayerIndex < 4; animatePlayerSelectionPlayerIndex++) {
        animatePlayerSelectionSpriteId = HuSprCreate(HuSprAnimReadFile(animatePlayerSelectionPlayerIndex + DATA_MAKE_NUM(DATADIR_MENT, 0x23)), 0, 0);
        HuSprGrpMemberSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x10, animatePlayerSelectionSpriteId);
        animatePlayerSelectionSpriteId = HuSprCreate(HuSprAnimReadFile(animatePlayerSelectionPlayerIndex + DATA_MAKE_NUM(DATADIR_MENT, 0x28)), 0xA, 0);
        HuSprGrpMemberSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x14, animatePlayerSelectionSpriteId);
    }
    animatePlayerSelectionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x27));
    animatePlayerSelectionSpriteAnimation2 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x2C));
    animatePlayerSelectionSpriteAnimation3 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x2D));
    animatePlayerSelectionSpriteAnimation4 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x2F));
    animatePlayerSelectionSpriteAnimation5 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x2E));
    for (animatePlayerSelectionPlayerIndex = 0; animatePlayerSelectionPlayerIndex < 4; animatePlayerSelectionPlayerIndex++) {
        animatePlayerSelectionSpriteId = HuSprCreate(animatePlayerSelectionSpriteAnimation1, 0, 0);
        HuSprGrpMemberSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x18, animatePlayerSelectionSpriteId);
        animatePlayerSelectionSpriteId = HuSprCreate(animatePlayerSelectionSpriteAnimation2, 0x1E, 0);
        HuSprGrpMemberSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x1C, animatePlayerSelectionSpriteId);
        animatePlayerSelectionSpriteId = HuSprCreate(animatePlayerSelectionSpriteAnimation3, 0xA, 0);
        HuSprGrpMemberSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x20, animatePlayerSelectionSpriteId);
        animatePlayerSelectionSpriteId = HuSprCreate(animatePlayerSelectionSpriteAnimation4, 0x14, 0);
        HuSprGrpMemberSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x24, animatePlayerSelectionSpriteId);
        HuSprTPLvlSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x24, 0.5f);
        animatePlayerSelectionSpriteId = HuSprCreate(animatePlayerSelectionSpriteAnimation5, 0x14, 0);
        HuSprGrpMemberSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x31, animatePlayerSelectionSpriteId);
    }
    animatePlayerSelectionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x17)), 0x7D0, 0);
    HuSprGrpMemberSet(animatePlayerSelectionSpriteGroupId, 0x28, animatePlayerSelectionSpriteId);
    HuSprTPLvlSet(animatePlayerSelectionSpriteGroupId, 0x28, 0.8f);
    MentSetSpritePosition(animatePlayerSelectionSpriteGroupId, 0x28, 288.0f, 240.0f);
    animatePlayerSelectionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x18));
    animatePlayerSelectionSpriteAnimation2 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x19));
    for (animatePlayerSelectionPlayerIndex = 0; animatePlayerSelectionPlayerIndex < 4; animatePlayerSelectionPlayerIndex++) {
        animatePlayerSelectionSpriteId = HuSprCreate(animatePlayerSelectionSpriteAnimation1, 0x7BC, 0);
        HuSprGrpMemberSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x29, animatePlayerSelectionSpriteId);
        HuSprBankSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x29, animatePlayerSelectionPlayerIndex);
        MentSetSpritePosition(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x29, characterRecordSpritePositions[animatePlayerSelectionPlayerIndex][2], characterRecordSpritePositions[animatePlayerSelectionPlayerIndex][3]);
        animatePlayerSelectionSpriteId = HuSprCreate(animatePlayerSelectionSpriteAnimation2, 0x7C6, 0);
        HuSprGrpMemberSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x2D, animatePlayerSelectionSpriteId);
        HuSprBankSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x2D, animatePlayerSelectionPlayerIndex);
        HuSprTPLvlSet(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x2D, 0.5f);
        MentSetSpritePosition(animatePlayerSelectionSpriteGroupId, animatePlayerSelectionPlayerIndex + 0x2D, characterRecordSpritePositions[animatePlayerSelectionPlayerIndex][2] + 1, characterRecordSpritePositions[animatePlayerSelectionPlayerIndex][3] + 6);
    }
    animatePlayerSelectionSelectionState = playerSelectionWork[0].selectionState;
    for (animatePlayerSelectionPlayerIndex2 = 0; animatePlayerSelectionPlayerIndex2 < 0x35; animatePlayerSelectionPlayerIndex2++) {
        HuSprAttrSet(animatePlayerSelectionSelectionState, animatePlayerSelectionPlayerIndex2, HUSPR_ATTR_DISPOFF);
    }
    for (animatePlayerSelectionPlayerIndex2 = 0; animatePlayerSelectionPlayerIndex2 < 8; animatePlayerSelectionPlayerIndex2++) {
        HuSprBankSet(animatePlayerSelectionSelectionState, animatePlayerSelectionPlayerIndex2, 0);
        HuSprAttrReset(animatePlayerSelectionSelectionState, animatePlayerSelectionPlayerIndex2, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(animatePlayerSelectionSelectionState, animatePlayerSelectionPlayerIndex2 + 8, HUSPR_ATTR_DISPOFF);
    }
    HuSprAttrReset(animatePlayerSelectionSelectionState, 0x28, HUSPR_ATTR_DISPOFF);
    for (animatePlayerSelectionPlayerIndex2 = 0; animatePlayerSelectionPlayerIndex2 < 4; animatePlayerSelectionPlayerIndex2++) {
        HuSprBankSet(animatePlayerSelectionSelectionState, animatePlayerSelectionPlayerIndex2 + 0x10, 0);
        HuSprBankSet(animatePlayerSelectionSelectionState, animatePlayerSelectionPlayerIndex2 + 0x14, 0);
        HuSprAttrReset(animatePlayerSelectionSelectionState, animatePlayerSelectionPlayerIndex2 + 0x29, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(animatePlayerSelectionSelectionState, animatePlayerSelectionPlayerIndex2 + 0x2D, HUSPR_ATTR_DISPOFF);
    }
    HuSprGrpPosSet(animatePlayerSelectionSelectionState, 0.0f, -500.0f);
}

void MentCreateRecordSprites(void)
{
    float createRecordSpritesInterpolationValue;
    float createRecordSpritesInterpolationValue2;
    s32 createRecordSpritesPlayerIndex;
    MentDllRecordPlayerWork *createRecordSpritesRecordPlayer;
    s32 createRecordSpritesStateValue;
    s32 createRecordSpritesPlayerIndex2;

    createRecordSpritesStateValue = playerSelectionWork[0].selectionState;
    for (createRecordSpritesPlayerIndex = 0; createRecordSpritesPlayerIndex < 8; createRecordSpritesPlayerIndex++) {
        HuSprBankSet(createRecordSpritesStateValue, createRecordSpritesPlayerIndex, 0);
    }
    for (createRecordSpritesPlayerIndex = 0; createRecordSpritesPlayerIndex < 4; createRecordSpritesPlayerIndex++) {
        HuSprBankSet(createRecordSpritesStateValue, createRecordSpritesPlayerIndex + 0x10, 0);
        HuSprBankSet(createRecordSpritesStateValue, createRecordSpritesPlayerIndex + 0x14, 0);
        HuSprBankSet(createRecordSpritesStateValue, createRecordSpritesPlayerIndex + 0x18, 0);
        HuSprBankSet(createRecordSpritesStateValue, createRecordSpritesPlayerIndex + 0x1C, 0);
        HuSprBankSet(createRecordSpritesStateValue, createRecordSpritesPlayerIndex + 0x20, 0);
    }
    for (createRecordSpritesPlayerIndex = 0; createRecordSpritesPlayerIndex <= 0xA; createRecordSpritesPlayerIndex++) {
        MentSleepFrame();
        for (createRecordSpritesPlayerIndex2 = 0; createRecordSpritesPlayerIndex2 < 4; createRecordSpritesPlayerIndex2++) {
            createRecordSpritesRecordPlayer = &recordPlayerWork[createRecordSpritesPlayerIndex2];
            createRecordSpritesInterpolationValue = MentInterpolateSineEaseIn(characterRecordSpritePositions[createRecordSpritesRecordPlayer->character % 4][0], characterRecordSpritePositions[createRecordSpritesRecordPlayer->recordMinutes % 4][0], createRecordSpritesPlayerIndex, 10.0f);
            createRecordSpritesInterpolationValue2 = MentInterpolateSineEaseIn(characterRecordSpritePositions[createRecordSpritesRecordPlayer->character / 4][1], characterRecordSpritePositions[createRecordSpritesRecordPlayer->recordMinutes / 4][1], createRecordSpritesPlayerIndex, 10.0f);
            HuSprPosSet(createRecordSpritesStateValue, createRecordSpritesRecordPlayer->recordMinutes + 0x10, createRecordSpritesInterpolationValue, createRecordSpritesInterpolationValue2);
            HuSprPosSet(createRecordSpritesStateValue, createRecordSpritesRecordPlayer->recordMinutes + 0x14, createRecordSpritesInterpolationValue, createRecordSpritesInterpolationValue2);
        }
    }
    for (createRecordSpritesPlayerIndex = 0; createRecordSpritesPlayerIndex < 4; createRecordSpritesPlayerIndex++) {
        recordPlayerWork[createRecordSpritesPlayerIndex].character = recordPlayerWork[createRecordSpritesPlayerIndex].recordMinutes;
        recordPlayerWork[createRecordSpritesPlayerIndex].recordFields[0] = 0;
    }
}

void MentUpdateRecordDisplay(OMOBJ *object, MentDllRecordPlayerWork *recordWork)
{
    float updateRecordDisplayInterpolationValue;
    float updateRecordDisplayInterpolationValue2;
    s32 updateRecordDisplayStateValue;
    s32 updateRecordDisplayStateValue2;
    s32 updateRecordDisplayPlayerIndex;
    s32 updateRecordDisplayStateValue3;
    s32 updateRecordDisplayStateValue4;
    s32 updateRecordDisplayStateValue5;
    s32 updateRecordDisplayStateValue6;
    s32 updateRecordDisplayStateValue7;

    updateRecordDisplayStateValue5 = playerSelectionWork[0].selectionState;
    updateRecordDisplayStateValue3 = 0;
    updateRecordDisplayStateValue4 = 0;
    updateRecordDisplayStateValue = recordWork->character % 4;
    updateRecordDisplayStateValue6 = updateRecordDisplayStateValue;
    updateRecordDisplayStateValue2 = recordWork->character / 4;
    updateRecordDisplayStateValue7 = updateRecordDisplayStateValue2;
    if (recordWork->recordFields[0] == 0) {
        if ((HuPadBtnDown[recordWork->controllerPort] & PAD_BUTTON_A) != 0) {
            recordWork->recordFields[0] = 1;
            HuSprBankSet(updateRecordDisplayStateValue5, recordWork->character, 1);
            HuSprBankSet(updateRecordDisplayStateValue5, recordWork->recordMinutes + 0x10, 1);
            HuSprBankSet(updateRecordDisplayStateValue5, recordWork->recordMinutes + 0x14, 1);
            HuAudFXPlay(recordWork->character + 0x6D);
        }
        else {
            if ((HuPadStkX[recordWork->controllerPort] >= 5) || ((HuPadBtn[recordWork->controllerPort] & 2) != 0)) {
                updateRecordDisplayStateValue++;
                updateRecordDisplayStateValue3 = 1;
                if (updateRecordDisplayStateValue > 3) {
                    updateRecordDisplayStateValue = 3;
                }
            }
            else if ((HuPadStkX[recordWork->controllerPort] <= -5) || ((HuPadBtn[recordWork->controllerPort] & 1) != 0)) {
                updateRecordDisplayStateValue--;
                updateRecordDisplayStateValue3 = -1;
                if (updateRecordDisplayStateValue < 0) {
                    updateRecordDisplayStateValue = 0;
                }
            }
            if ((HuPadStkY[recordWork->controllerPort] <= -5) || ((HuPadBtn[recordWork->controllerPort] & 4) != 0)) {
                updateRecordDisplayStateValue2++;
                updateRecordDisplayStateValue4 = 1;
                if (updateRecordDisplayStateValue2 > 1) {
                    updateRecordDisplayStateValue2 = 1;
                }
            }
            else if ((HuPadStkY[recordWork->controllerPort] >= 5) || ((HuPadBtn[recordWork->controllerPort] & 8) != 0)) {
                updateRecordDisplayStateValue2--;
                updateRecordDisplayStateValue4 = -1;
                if (updateRecordDisplayStateValue2 < 0) {
                    updateRecordDisplayStateValue2 = 0;
                }
            }
        }
    }
    else if ((HuPadBtnDown[recordWork->controllerPort] & PAD_BUTTON_B) != 0) {
        recordWork->recordFields[0] = 0;
        HuSprBankSet(updateRecordDisplayStateValue5, recordWork->character, 0);
        HuSprBankSet(updateRecordDisplayStateValue5, recordWork->recordMinutes + 0x10, 0);
        HuSprBankSet(updateRecordDisplayStateValue5, recordWork->recordMinutes + 0x14, 0);
        HuAudFXPlay(3);
    }
    if (recordWork->character != (updateRecordDisplayStateValue + (updateRecordDisplayStateValue2 * 4))) {
        do {
            for (updateRecordDisplayPlayerIndex = 0; updateRecordDisplayPlayerIndex < 4; updateRecordDisplayPlayerIndex++) {
                if ((updateRecordDisplayPlayerIndex != recordWork->recordMinutes) && (recordPlayerWork[updateRecordDisplayPlayerIndex].isComputerPlayer == 0)
                    && ((updateRecordDisplayStateValue + (updateRecordDisplayStateValue2 * 4)) == recordPlayerWork[updateRecordDisplayPlayerIndex].character)) {
                    if ((updateRecordDisplayStateValue3 == 0) || (updateRecordDisplayStateValue4 == 0)) {
                        if (((updateRecordDisplayStateValue + updateRecordDisplayStateValue3) > 3) || ((updateRecordDisplayStateValue + updateRecordDisplayStateValue3) < 0)) {
                            updateRecordDisplayStateValue = updateRecordDisplayStateValue6;
                        }
                        else {
                            updateRecordDisplayStateValue += updateRecordDisplayStateValue3;
                        }
                        if (((updateRecordDisplayStateValue2 + updateRecordDisplayStateValue4) > 1) || ((updateRecordDisplayStateValue2 + updateRecordDisplayStateValue4) < 0)) {
                            updateRecordDisplayStateValue2 = updateRecordDisplayStateValue7;
                        }
                        else {
                            updateRecordDisplayStateValue2 += updateRecordDisplayStateValue4;
                        }
                    }
                    else if (((updateRecordDisplayStateValue + updateRecordDisplayStateValue3) <= 3) && ((updateRecordDisplayStateValue + updateRecordDisplayStateValue3) >= 0)) {
                        updateRecordDisplayStateValue += updateRecordDisplayStateValue3;
                    }
                    else if (((updateRecordDisplayStateValue2 + updateRecordDisplayStateValue4) <= 1) && ((updateRecordDisplayStateValue2 + updateRecordDisplayStateValue4) >= 0)) {
                        updateRecordDisplayStateValue2 += updateRecordDisplayStateValue4;
                    }
                    else {
                        updateRecordDisplayStateValue = updateRecordDisplayStateValue6;
                        updateRecordDisplayStateValue2 = updateRecordDisplayStateValue7;
                    }
                    break;
                }
            }
        } while (updateRecordDisplayPlayerIndex != 4);
        if (recordWork->character != (updateRecordDisplayStateValue + (updateRecordDisplayStateValue2 * 4))) {
            recordWork->character = updateRecordDisplayStateValue + (updateRecordDisplayStateValue2 * 4);
            HuAudFXPlay(0);
            for (updateRecordDisplayPlayerIndex = 0; updateRecordDisplayPlayerIndex <= 0xA; updateRecordDisplayPlayerIndex++) {
                MentSleepFrame();
                updateRecordDisplayInterpolationValue = MentInterpolateSineEaseIn(characterRecordSpritePositions[updateRecordDisplayStateValue6][0], characterRecordSpritePositions[updateRecordDisplayStateValue][0], updateRecordDisplayPlayerIndex, 10.0f);
                updateRecordDisplayInterpolationValue2 = MentInterpolateSineEaseIn(characterRecordSpritePositions[updateRecordDisplayStateValue7][1], characterRecordSpritePositions[updateRecordDisplayStateValue2][1], updateRecordDisplayPlayerIndex, 10.0f);
                HuSprPosSet(updateRecordDisplayStateValue5, recordWork->recordMinutes + 0x10, updateRecordDisplayInterpolationValue, updateRecordDisplayInterpolationValue2);
                HuSprPosSet(updateRecordDisplayStateValue5, recordWork->recordMinutes + 0x14, updateRecordDisplayInterpolationValue, updateRecordDisplayInterpolationValue2);
            }
        }
    }
}

void MentGetRecordSelectionState(MentDllRecordPlayerWork *recordWork)
{
    s32 getRecordSelectionStateSelectionState;

    getRecordSelectionStateSelectionState = playerSelectionWork[0].selectionState;
    recordWork->difficulty = 0;
    HuSprBankSet(getRecordSelectionStateSelectionState, recordWork->recordMinutes + 0x20, recordWork->difficulty);
    HuSprPosSet(getRecordSelectionStateSelectionState, recordWork->recordMinutes + 0x20, characterRecordSpritePositions[recordWork->character % 4][0], characterRecordSpritePositions[recordWork->character / 4][1] + 0x23);
    HuSprPosSet(getRecordSelectionStateSelectionState, recordWork->recordMinutes + 0x24, characterRecordSpritePositions[recordWork->character % 4][0] + 1, characterRecordSpritePositions[recordWork->character / 4][1] + 0x25);
    HuSprAttrReset(getRecordSelectionStateSelectionState, recordWork->recordMinutes + 0x20, HUSPR_ATTR_DISPOFF);
    HuSprAttrReset(getRecordSelectionStateSelectionState, recordWork->recordMinutes + 0x24, HUSPR_ATTR_DISPOFF);
}

s32 MentGetRecordSpriteIndex(MentDllRecordPlayerWork *recordWork)
{
    float getRecordSpriteIndexInterpolationValue;
    s32 getRecordSpriteIndexPlayerIndex;
    s32 getRecordSpriteIndexStateValue;
    s32 getRecordSpriteIndexStateValue2;
    s32 getRecordSpriteIndexStateValue3;
    s32 getRecordSpriteIndexStateValue4;

    getRecordSpriteIndexStateValue2 = 0;
    getRecordSpriteIndexStateValue = playerSelectionWork[0].selectionState;
    while (1) {
        MentSleepFrame();
        getRecordSpriteIndexStateValue2 = 0;
        getRecordSpriteIndexStateValue4 = recordWork->difficulty;
        if (HuPadStkX[recordPlayerWork->controllerPort] >= 5 || (HuPadBtn[recordPlayerWork->controllerPort] & PAD_BUTTON_RIGHT)) {
            if (menuLaunchParameters[5] == 1) {
                recordWork->difficulty++;
                getRecordSpriteIndexStateValue2 = 1;
                if (recordWork->difficulty > 3) {
                    recordWork->difficulty = 0;
                }
            }
            else {
                recordWork->difficulty++;
                getRecordSpriteIndexStateValue2 = 1;
                if (recordWork->difficulty > 2) {
                    recordWork->difficulty = 0;
                }
            }
        }
        else if (HuPadStkX[recordPlayerWork->controllerPort] <= -5 || (HuPadBtn[recordPlayerWork->controllerPort] & PAD_BUTTON_LEFT)) {
            if (menuLaunchParameters[5] == 1) {
                recordWork->difficulty--;
                getRecordSpriteIndexStateValue2 = -1;
                if (recordWork->difficulty < 0) {
                    recordWork->difficulty = 3;
                }
            }
            else {
                recordWork->difficulty--;
                getRecordSpriteIndexStateValue2 = -1;
                if (recordWork->difficulty < 0) {
                    recordWork->difficulty = 2;
                }
            }
        }
        else if (HuPadBtnDown[0] & PAD_BUTTON_A) {
            getRecordSpriteIndexStateValue3 = 1;
            HuAudFXPlay(2);
            break;
        }
        else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            getRecordSpriteIndexStateValue3 = 0;
            HuAudFXPlay(3);
            break;
        }
        if (recordWork->difficulty != getRecordSpriteIndexStateValue4) {
            HuAudFXPlay(0);
            for (getRecordSpriteIndexPlayerIndex = 0; getRecordSpriteIndexPlayerIndex <= 5; getRecordSpriteIndexPlayerIndex++) {
                MentSleepFrame();
                getRecordSpriteIndexInterpolationValue = MentInterpolateSineEaseIn(characterRecordSpritePositions[recordWork->character % 4][0], characterRecordSpritePositions[recordWork->character % 4][0] + (getRecordSpriteIndexStateValue2 * 10), getRecordSpriteIndexPlayerIndex, 5);
                HuSprPosSet(getRecordSpriteIndexStateValue, recordWork->recordMinutes + 32, getRecordSpriteIndexInterpolationValue, characterRecordSpritePositions[recordWork->character / 4][1] + 35);
                HuSprPosSet(getRecordSpriteIndexStateValue, recordWork->recordMinutes + 36, 1 + getRecordSpriteIndexInterpolationValue, characterRecordSpritePositions[recordWork->character / 4][1] + 37);
            }
            HuSprBankSet(getRecordSpriteIndexStateValue, recordWork->recordMinutes + 32, recordWork->difficulty);
            for (getRecordSpriteIndexPlayerIndex = 0; getRecordSpriteIndexPlayerIndex <= 5; getRecordSpriteIndexPlayerIndex++) {
                MentSleepFrame();
                getRecordSpriteIndexInterpolationValue = MentInterpolateSineEaseIn(characterRecordSpritePositions[recordWork->character % 4][0] + (getRecordSpriteIndexStateValue2 * 10), characterRecordSpritePositions[recordWork->character % 4][0], getRecordSpriteIndexPlayerIndex, 5);
                HuSprPosSet(getRecordSpriteIndexStateValue, recordWork->recordMinutes + 32, getRecordSpriteIndexInterpolationValue, characterRecordSpritePositions[recordWork->character / 4][1] + 35);
                HuSprPosSet(getRecordSpriteIndexStateValue, recordWork->recordMinutes + 36, 1 + getRecordSpriteIndexInterpolationValue, characterRecordSpritePositions[recordWork->character / 4][1] + 37);
            }
        }
    }
    if (getRecordSpriteIndexStateValue3 == 1) {
        HuSprPosSet(getRecordSpriteIndexStateValue, recordWork->recordMinutes + 49, characterRecordSpritePositions[recordWork->character % 4][0], characterRecordSpritePositions[recordWork->character / 4][1] + 35);
        HuSprBankSet(getRecordSpriteIndexStateValue, recordWork->recordMinutes + 49, recordWork->difficulty);
        HuSprAttrReset(getRecordSpriteIndexStateValue, recordWork->recordMinutes + 49, HUSPR_ATTR_DISPOFF);
        HuSprAttrSet(getRecordSpriteIndexStateValue, recordWork->recordMinutes + 32, HUSPR_ATTR_DISPOFF);
    }

    return getRecordSpriteIndexStateValue3;
}

void MentUpdateRecordSpriteAnimation(OMOBJ *object, MentDllRecordPlayerWork *recordWork)
{
    s32 updateRecordSpriteAnimationStateValue;
    float updateRecordSpriteAnimationAnimationProgress;
    float updateRecordSpriteAnimationAnimationProgress2;
    s32 updateRecordSpriteAnimationAnimationFrame;
    s32 updateRecordSpriteAnimationAnimationFrame2;
    s32 updateRecordSpriteAnimationPlayerIndex;
    s32 updateRecordSpriteAnimationAnimationFrame3;
    s32 updateRecordSpriteAnimationAnimationFrame4;
    s32 updateRecordSpriteAnimationAnimationFrame5;
    s32 updateRecordSpriteAnimationAnimationFrame6;
    s32 updateRecordSpriteAnimationAnimationFrame7;

    updateRecordSpriteAnimationAnimationFrame3 = playerSelectionWork[0].selectionState;
    updateRecordSpriteAnimationAnimationFrame4 = 0;
    updateRecordSpriteAnimationAnimationFrame5 = 0;
    updateRecordSpriteAnimationStateValue = recordWork->difficulty;
    updateRecordSpriteAnimationAnimationFrame6 = updateRecordSpriteAnimationAnimationFrame = recordWork->character % 4;
    updateRecordSpriteAnimationAnimationFrame2 = recordWork->character / 4;
    updateRecordSpriteAnimationAnimationFrame7 = updateRecordSpriteAnimationAnimationFrame2;
    if (recordWork->recordFields[0] == 0) {
        if ((HuPadBtnDown[recordPlayerWork->controllerPort] & PAD_BUTTON_A) != 0) {
            recordWork->recordFields[0] = 1;
            HuSprBankSet(updateRecordSpriteAnimationAnimationFrame3, recordWork->character, 1);
            HuSprBankSet(updateRecordSpriteAnimationAnimationFrame3, recordWork->recordMinutes + 0x18, 1);
            HuSprBankSet(updateRecordSpriteAnimationAnimationFrame3, recordWork->recordMinutes + 0x1C, 1);
            HuAudFXPlay(recordWork->character + 0x6D);
            MentGetRecordSelectionState(recordWork);
        }
        else {
            if ((HuPadStkX[recordPlayerWork->controllerPort] >= 5) || ((HuPadBtn[recordPlayerWork->controllerPort] & 2) != 0)) {
                updateRecordSpriteAnimationAnimationFrame++;
                updateRecordSpriteAnimationAnimationFrame4 = 1;
                if (updateRecordSpriteAnimationAnimationFrame > 3) {
                    updateRecordSpriteAnimationAnimationFrame = 3;
                }
            }
            else if ((HuPadStkX[recordPlayerWork->controllerPort] <= -5) || ((HuPadBtn[recordPlayerWork->controllerPort] & 1) != 0)) {
                updateRecordSpriteAnimationAnimationFrame--;
                updateRecordSpriteAnimationAnimationFrame4 = -1;
                if (updateRecordSpriteAnimationAnimationFrame < 0) {
                    updateRecordSpriteAnimationAnimationFrame = 0;
                }
            }
            if ((HuPadStkY[recordPlayerWork->controllerPort] <= -5) || ((HuPadBtn[recordPlayerWork->controllerPort] & 4) != 0)) {
                updateRecordSpriteAnimationAnimationFrame2++;
                updateRecordSpriteAnimationAnimationFrame5 = 1;
                if (updateRecordSpriteAnimationAnimationFrame2 > 1) {
                    updateRecordSpriteAnimationAnimationFrame2 = 1;
                }
            }
            else if ((HuPadStkY[recordPlayerWork->controllerPort] >= 5) || ((HuPadBtn[recordPlayerWork->controllerPort] & 8) != 0)) {
                updateRecordSpriteAnimationAnimationFrame2--;
                updateRecordSpriteAnimationAnimationFrame5 = -1;
                if (updateRecordSpriteAnimationAnimationFrame2 < 0) {
                    updateRecordSpriteAnimationAnimationFrame2 = 0;
                }
            }
        }
    }
    else if ((HuPadBtnDown[recordPlayerWork->controllerPort] & PAD_BUTTON_B) != 0) {
        recordWork->recordFields[0] = recordWork->recordFields[2] = 0;
        HuSprBankSet(updateRecordSpriteAnimationAnimationFrame3, recordWork->character, 0);
        HuSprBankSet(updateRecordSpriteAnimationAnimationFrame3, recordWork->recordMinutes + 0x18, 0);
        HuSprBankSet(updateRecordSpriteAnimationAnimationFrame3, recordWork->recordMinutes + 0x1C, 0);
        HuSprAttrSet(updateRecordSpriteAnimationAnimationFrame3, recordWork->recordMinutes + 0x20, 4);
        HuSprAttrSet(updateRecordSpriteAnimationAnimationFrame3, recordWork->recordMinutes + 0x24, 4);
        HuAudFXPlay(3);
    }
    if (recordWork->character != (updateRecordSpriteAnimationAnimationFrame + (updateRecordSpriteAnimationAnimationFrame2 * 4))) {
        do {
            for (updateRecordSpriteAnimationPlayerIndex = 0; updateRecordSpriteAnimationPlayerIndex < 4; updateRecordSpriteAnimationPlayerIndex++) {
                if ((updateRecordSpriteAnimationPlayerIndex != recordWork->recordMinutes) && (recordPlayerWork[updateRecordSpriteAnimationPlayerIndex].recordFields[0] == 1)
                    && ((updateRecordSpriteAnimationAnimationFrame + (updateRecordSpriteAnimationAnimationFrame2 * 4)) == recordPlayerWork[updateRecordSpriteAnimationPlayerIndex].character)) {
                    if ((updateRecordSpriteAnimationAnimationFrame4 == 0) || (updateRecordSpriteAnimationAnimationFrame5 == 0)) {
                        if (((updateRecordSpriteAnimationAnimationFrame + updateRecordSpriteAnimationAnimationFrame4) > 3) || ((updateRecordSpriteAnimationAnimationFrame + updateRecordSpriteAnimationAnimationFrame4) < 0)) {
                            updateRecordSpriteAnimationAnimationFrame = updateRecordSpriteAnimationAnimationFrame6;
                        }
                        else {
                            updateRecordSpriteAnimationAnimationFrame += updateRecordSpriteAnimationAnimationFrame4;
                        }
                        if (((updateRecordSpriteAnimationAnimationFrame2 + updateRecordSpriteAnimationAnimationFrame5) > 1) || ((updateRecordSpriteAnimationAnimationFrame2 + updateRecordSpriteAnimationAnimationFrame5) < 0)) {
                            updateRecordSpriteAnimationAnimationFrame2 = updateRecordSpriteAnimationAnimationFrame7;
                        }
                        else {
                            updateRecordSpriteAnimationAnimationFrame2 += updateRecordSpriteAnimationAnimationFrame5;
                        }
                    }
                    else if (((updateRecordSpriteAnimationAnimationFrame + updateRecordSpriteAnimationAnimationFrame4) <= 3) && ((updateRecordSpriteAnimationAnimationFrame + updateRecordSpriteAnimationAnimationFrame4) >= 0)) {
                        updateRecordSpriteAnimationAnimationFrame += updateRecordSpriteAnimationAnimationFrame4;
                    }
                    else if (((updateRecordSpriteAnimationAnimationFrame2 + updateRecordSpriteAnimationAnimationFrame5) <= 1) && ((updateRecordSpriteAnimationAnimationFrame2 + updateRecordSpriteAnimationAnimationFrame5) >= 0)) {
                        updateRecordSpriteAnimationAnimationFrame2 += updateRecordSpriteAnimationAnimationFrame5;
                    }
                    else {
                        updateRecordSpriteAnimationAnimationFrame = updateRecordSpriteAnimationAnimationFrame6;
                        updateRecordSpriteAnimationAnimationFrame2 = updateRecordSpriteAnimationAnimationFrame7;
                    }
                    break;
                }
            }
        } while (updateRecordSpriteAnimationPlayerIndex != 4);
        if (recordWork->character != (updateRecordSpriteAnimationAnimationFrame + (updateRecordSpriteAnimationAnimationFrame2 * 4))) {
            recordWork->character = updateRecordSpriteAnimationAnimationFrame + (updateRecordSpriteAnimationAnimationFrame2 * 4);
            HuAudFXPlay(0);
            for (updateRecordSpriteAnimationPlayerIndex = 0; updateRecordSpriteAnimationPlayerIndex <= 0xA; updateRecordSpriteAnimationPlayerIndex++) {
                MentSleepFrame();
                updateRecordSpriteAnimationAnimationProgress = MentInterpolateSineEaseIn(characterRecordSpritePositions[updateRecordSpriteAnimationAnimationFrame6][0], characterRecordSpritePositions[updateRecordSpriteAnimationAnimationFrame][0], updateRecordSpriteAnimationPlayerIndex, 10.0f);
                updateRecordSpriteAnimationAnimationProgress2 = MentInterpolateSineEaseIn(characterRecordSpritePositions[updateRecordSpriteAnimationAnimationFrame7][1], characterRecordSpritePositions[updateRecordSpriteAnimationAnimationFrame2][1], updateRecordSpriteAnimationPlayerIndex, 10.0f);
                HuSprPosSet(updateRecordSpriteAnimationAnimationFrame3, recordWork->recordMinutes + 0x18, updateRecordSpriteAnimationAnimationProgress, updateRecordSpriteAnimationAnimationProgress2);
                HuSprPosSet(updateRecordSpriteAnimationAnimationFrame3, recordWork->recordMinutes + 0x1C, updateRecordSpriteAnimationAnimationProgress, updateRecordSpriteAnimationAnimationProgress2);
                HuSprPosSet(updateRecordSpriteAnimationAnimationFrame3, recordWork->recordMinutes + 0x20, updateRecordSpriteAnimationAnimationProgress, 35.0f + updateRecordSpriteAnimationAnimationProgress2);
                HuSprPosSet(updateRecordSpriteAnimationAnimationFrame3, recordWork->recordMinutes + 0x24, 1.0f + updateRecordSpriteAnimationAnimationProgress, 37.0f + updateRecordSpriteAnimationAnimationProgress2);
            }
        }
    }
    (void)updateRecordSpriteAnimationAnimationFrame;
}

void MentUpdateRecordPlayerSelection(MentDllRecordPlayerWork *recordWork)
{
    s32 updateRecordPlayerSelectionSelectionState;
    s32 updateRecordPlayerSelectionPlayerIndex;

    updateRecordPlayerSelectionSelectionState = playerSelectionWork[0].selectionState;
    do {
        for (updateRecordPlayerSelectionPlayerIndex = 0; updateRecordPlayerSelectionPlayerIndex < 4; updateRecordPlayerSelectionPlayerIndex++) {
            if ((updateRecordPlayerSelectionPlayerIndex != recordWork->recordMinutes) && (recordPlayerWork[updateRecordPlayerSelectionPlayerIndex].recordFields[0] == 1) && (recordWork->character == recordPlayerWork[updateRecordPlayerSelectionPlayerIndex].character)) {
                recordWork->character++;
                break;
            }
        }
    } while (updateRecordPlayerSelectionPlayerIndex != 4);
    recordWork->difficulty = 0;
    HuSprBankSet(updateRecordPlayerSelectionSelectionState, recordWork->character, 0);
    HuSprBankSet(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x18, 0);
    HuSprBankSet(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x1C, 0);
    HuSprBankSet(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x20, recordWork->difficulty);
    HuSprBankSet(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x31, recordWork->difficulty);
    HuSprPosSet(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x18, characterRecordSpritePositions[recordWork->character % 4][0], characterRecordSpritePositions[recordWork->character / 4][1]);
    HuSprPosSet(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x1C, characterRecordSpritePositions[recordWork->character % 4][0], characterRecordSpritePositions[recordWork->character / 4][1]);
    HuSprPosSet(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x20, characterRecordSpritePositions[recordWork->character % 4][0], characterRecordSpritePositions[recordWork->character / 4][1] + 0x23);
    HuSprPosSet(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x24, characterRecordSpritePositions[recordWork->character % 4][0] + 1, characterRecordSpritePositions[recordWork->character / 4][1] + 0x25);
    HuSprPosSet(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x31, characterRecordSpritePositions[recordWork->character % 4][0] + 1, characterRecordSpritePositions[recordWork->character / 4][1] + 0x25);
    recordWork->recordFields[2] = 0;
    HuSprAttrReset(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x18, HUSPR_ATTR_DISPOFF);
    HuSprAttrReset(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x1C, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x20, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x24, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(updateRecordPlayerSelectionSelectionState, recordWork->recordMinutes + 0x31, HUSPR_ATTR_DISPOFF);
}

void MentGetPlayerSelectionState(MentDllRecordPlayerWork *recordWork)
{
    s32 getPlayerSelectionStateSelectionState;

    getPlayerSelectionStateSelectionState = playerSelectionWork[0].selectionState;
    HuSprBankSet(getPlayerSelectionStateSelectionState, recordWork->character, 0);
    HuSprAttrSet(getPlayerSelectionStateSelectionState, recordWork->recordMinutes + 0x18, 4);
    HuSprAttrSet(getPlayerSelectionStateSelectionState, recordWork->recordMinutes + 0x1C, 4);
    HuSprAttrSet(getPlayerSelectionStateSelectionState, recordWork->recordMinutes + 0x20, 4);
    HuSprAttrSet(getPlayerSelectionStateSelectionState, recordWork->recordMinutes + 0x24, 4);
    HuSprAttrSet(getPlayerSelectionStateSelectionState, recordWork->recordMinutes + 0x31, 4);
}

void MentUpdateRecordSelection(void)
{
    MentDllRecordPlayerWork *updateRecordSelectionRecordPlayer;
    s32 updateRecordSelectionPlayerIndex;
    s32 updateRecordSelectionSelectionState;

    updateRecordSelectionSelectionState = playerSelectionWork[0].selectionTimer;
    for (updateRecordSelectionPlayerIndex = 0; updateRecordSelectionPlayerIndex < 0x11; updateRecordSelectionPlayerIndex++) {
        HuSprAttrSet(updateRecordSelectionSelectionState, updateRecordSelectionPlayerIndex, HUSPR_ATTR_DISPOFF);
    }
    for (updateRecordSelectionPlayerIndex = 0; updateRecordSelectionPlayerIndex < 4; updateRecordSelectionPlayerIndex++) {
        updateRecordSelectionRecordPlayer = &recordPlayerWork[updateRecordSelectionPlayerIndex];
        HuSprPosSet(updateRecordSelectionSelectionState, updateRecordSelectionRecordPlayer->character, characterRecordSpriteOffsets[updateRecordSelectionRecordPlayer->recordMinutes][0], characterRecordSpriteOffsets[updateRecordSelectionRecordPlayer->recordMinutes][1]);
        HuSprAttrReset(updateRecordSelectionSelectionState, updateRecordSelectionRecordPlayer->character, HUSPR_ATTR_DISPOFF);
        if (updateRecordSelectionRecordPlayer->isComputerPlayer == 0) {
            HuSprBankSet(updateRecordSelectionSelectionState, updateRecordSelectionRecordPlayer->recordMinutes + 8, updateRecordSelectionRecordPlayer->recordMinutes);
            HuSprPosSet(updateRecordSelectionSelectionState, updateRecordSelectionRecordPlayer->recordMinutes + 8, characterRecordSpriteOffsets[updateRecordSelectionRecordPlayer->recordMinutes][0], characterRecordSpriteOffsets[updateRecordSelectionRecordPlayer->recordMinutes][1] + 0x19);
            HuSprAttrReset(updateRecordSelectionSelectionState, updateRecordSelectionRecordPlayer->recordMinutes + 8, HUSPR_ATTR_DISPOFF);
        }
        else {
            HuSprBankSet(updateRecordSelectionSelectionState, updateRecordSelectionRecordPlayer->recordMinutes + 0xC, updateRecordSelectionRecordPlayer->difficulty);
            HuSprPosSet(updateRecordSelectionSelectionState, updateRecordSelectionRecordPlayer->recordMinutes + 0xC, characterRecordSpriteOffsets[updateRecordSelectionRecordPlayer->recordMinutes][0], characterRecordSpriteOffsets[updateRecordSelectionRecordPlayer->recordMinutes][1] + 0x19);
            HuSprAttrReset(updateRecordSelectionSelectionState, updateRecordSelectionRecordPlayer->recordMinutes + 0xC, HUSPR_ATTR_DISPOFF);
        }
    }
    HuSprAttrReset(updateRecordSelectionSelectionState, 0x10, HUSPR_ATTR_DISPOFF);
    HuSprGrpPosSet(updateRecordSelectionSelectionState, 16.0f, -500.0f);
}

void MentAnimatePlayerSelectionCursor(MentDllPlayerSelectionWork *selectionWork, s32 selectionIndex, s32 playerIndex, s32 animationFrames)
{
    MentDllRecordPlayerWork *animatePlayerSelectionCursorRecordPlayer;
    s32 animatePlayerSelectionCursorPlayerIndex;
    s32 animatePlayerSelectionCursorSelectionState;
    s32 animatePlayerSelectionCursorPlayerIndex2;
    s32 animatePlayerSelectionCursorSpriteId;
    s32 animatePlayerSelectionCursorSpriteGroupId;
    ANIMDATA *animatePlayerSelectionCursorSpriteAnimation1;
    ANIMDATA *animatePlayerSelectionCursorSpriteAnimation2;

    animatePlayerSelectionCursorSpriteGroupId = HuSprGrpCreate(0x11);
    selectionWork->selectionTimer = animatePlayerSelectionCursorSpriteGroupId;
    for (animatePlayerSelectionCursorPlayerIndex = 0; animatePlayerSelectionCursorPlayerIndex < 8; animatePlayerSelectionCursorPlayerIndex++) {
        animatePlayerSelectionCursorSpriteId = HuSprCreate(HuSprAnimReadFile(animatePlayerSelectionCursorPlayerIndex + DATA_MAKE_NUM(DATADIR_MENT, 0x31)), 0x3E8, 0);
        HuSprGrpMemberSet(animatePlayerSelectionCursorSpriteGroupId, animatePlayerSelectionCursorPlayerIndex, animatePlayerSelectionCursorSpriteId);
    }
    animatePlayerSelectionCursorSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x3A));
    animatePlayerSelectionCursorSpriteAnimation2 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x3B));
    for (animatePlayerSelectionCursorPlayerIndex = 0; animatePlayerSelectionCursorPlayerIndex < 4; animatePlayerSelectionCursorPlayerIndex++) {
        animatePlayerSelectionCursorSpriteId = HuSprCreate(animatePlayerSelectionCursorSpriteAnimation1, 0x3DE, 0);
        HuSprGrpMemberSet(animatePlayerSelectionCursorSpriteGroupId, animatePlayerSelectionCursorPlayerIndex + 8, animatePlayerSelectionCursorSpriteId);
        animatePlayerSelectionCursorSpriteId = HuSprCreate(animatePlayerSelectionCursorSpriteAnimation2, 0x3DE, 0);
        HuSprGrpMemberSet(animatePlayerSelectionCursorSpriteGroupId, animatePlayerSelectionCursorPlayerIndex + 0xC, animatePlayerSelectionCursorSpriteId);
    }
    animatePlayerSelectionCursorSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x30)), 0x7D0, 0);
    HuSprGrpMemberSet(animatePlayerSelectionCursorSpriteGroupId, 0x10, animatePlayerSelectionCursorSpriteId);
    HuSprTPLvlSet(animatePlayerSelectionCursorSpriteGroupId, 0x10, 0.8f);
    HuSprPosSet(animatePlayerSelectionCursorSpriteGroupId, 0x10, 96.0f, 32.0f);
    animatePlayerSelectionCursorSelectionState = playerSelectionWork[0].selectionTimer;
    for (animatePlayerSelectionCursorPlayerIndex2 = 0; animatePlayerSelectionCursorPlayerIndex2 < 0x11; animatePlayerSelectionCursorPlayerIndex2++) {
        HuSprAttrSet(animatePlayerSelectionCursorSelectionState, animatePlayerSelectionCursorPlayerIndex2, HUSPR_ATTR_DISPOFF);
    }
    for (animatePlayerSelectionCursorPlayerIndex2 = 0; animatePlayerSelectionCursorPlayerIndex2 < 4; animatePlayerSelectionCursorPlayerIndex2++) {
        animatePlayerSelectionCursorRecordPlayer = &recordPlayerWork[animatePlayerSelectionCursorPlayerIndex2];
        HuSprPosSet(animatePlayerSelectionCursorSelectionState, animatePlayerSelectionCursorRecordPlayer->character, characterRecordSpriteOffsets[animatePlayerSelectionCursorRecordPlayer->recordMinutes][0], characterRecordSpriteOffsets[animatePlayerSelectionCursorRecordPlayer->recordMinutes][1]);
        HuSprAttrReset(animatePlayerSelectionCursorSelectionState, animatePlayerSelectionCursorRecordPlayer->character, HUSPR_ATTR_DISPOFF);
        if (animatePlayerSelectionCursorRecordPlayer->isComputerPlayer == 0) {
            HuSprBankSet(animatePlayerSelectionCursorSelectionState, animatePlayerSelectionCursorRecordPlayer->recordMinutes + 8, animatePlayerSelectionCursorRecordPlayer->recordMinutes);
            HuSprPosSet(animatePlayerSelectionCursorSelectionState, animatePlayerSelectionCursorRecordPlayer->recordMinutes + 8, characterRecordSpriteOffsets[animatePlayerSelectionCursorRecordPlayer->recordMinutes][0], characterRecordSpriteOffsets[animatePlayerSelectionCursorRecordPlayer->recordMinutes][1] + 0x19);
            HuSprAttrReset(animatePlayerSelectionCursorSelectionState, animatePlayerSelectionCursorRecordPlayer->recordMinutes + 8, HUSPR_ATTR_DISPOFF);
        }
        else {
            HuSprBankSet(animatePlayerSelectionCursorSelectionState, animatePlayerSelectionCursorRecordPlayer->recordMinutes + 0xC, animatePlayerSelectionCursorRecordPlayer->difficulty);
            HuSprPosSet(animatePlayerSelectionCursorSelectionState, animatePlayerSelectionCursorRecordPlayer->recordMinutes + 0xC, characterRecordSpriteOffsets[animatePlayerSelectionCursorRecordPlayer->recordMinutes][0], characterRecordSpriteOffsets[animatePlayerSelectionCursorRecordPlayer->recordMinutes][1] + 0x19);
            HuSprAttrReset(animatePlayerSelectionCursorSelectionState, animatePlayerSelectionCursorRecordPlayer->recordMinutes + 0xC, HUSPR_ATTR_DISPOFF);
        }
    }
    HuSprAttrReset(animatePlayerSelectionCursorSelectionState, 0x10, HUSPR_ATTR_DISPOFF);
    HuSprGrpPosSet(animatePlayerSelectionCursorSelectionState, 16.0f, -500.0f);
}

void MentCreateRecordSelectionSprites(void)
{
    float createRecordSelectionSpritesInterpolationValue;
    MentDllRecordPlayerWork *createRecordSelectionSpritesRecordPlayer;
    s32 createRecordSelectionSpritesSelectionState;
    s32 createRecordSelectionSpritesPlayerIndex;
    s32 createRecordSelectionSpritesPlayerIndex2;

    createRecordSelectionSpritesSelectionState = playerSelectionWork[0].selectionTimer;
    for (createRecordSelectionSpritesPlayerIndex = 0; createRecordSelectionSpritesPlayerIndex < 0x11; createRecordSelectionSpritesPlayerIndex++) {
        HuSprAttrSet(createRecordSelectionSpritesSelectionState, createRecordSelectionSpritesPlayerIndex, HUSPR_ATTR_DISPOFF);
    }
    for (createRecordSelectionSpritesPlayerIndex = 0; createRecordSelectionSpritesPlayerIndex < 4; createRecordSelectionSpritesPlayerIndex++) {
        createRecordSelectionSpritesRecordPlayer = &recordPlayerWork[createRecordSelectionSpritesPlayerIndex];
        HuSprPosSet(createRecordSelectionSpritesSelectionState, createRecordSelectionSpritesRecordPlayer->character, characterRecordSpriteOffsets[createRecordSelectionSpritesRecordPlayer->recordMinutes][0], characterRecordSpriteOffsets[createRecordSelectionSpritesRecordPlayer->recordMinutes][1]);
        HuSprAttrReset(createRecordSelectionSpritesSelectionState, createRecordSelectionSpritesRecordPlayer->character, HUSPR_ATTR_DISPOFF);
        if (createRecordSelectionSpritesRecordPlayer->isComputerPlayer == 0) {
            HuSprBankSet(createRecordSelectionSpritesSelectionState, createRecordSelectionSpritesRecordPlayer->recordMinutes + 8, createRecordSelectionSpritesRecordPlayer->recordMinutes);
            HuSprPosSet(createRecordSelectionSpritesSelectionState, createRecordSelectionSpritesRecordPlayer->recordMinutes + 8, characterRecordSpriteOffsets[createRecordSelectionSpritesRecordPlayer->recordMinutes][0], characterRecordSpriteOffsets[createRecordSelectionSpritesRecordPlayer->recordMinutes][1] + 0x19);
            HuSprAttrReset(createRecordSelectionSpritesSelectionState, createRecordSelectionSpritesRecordPlayer->recordMinutes + 8, HUSPR_ATTR_DISPOFF);
        }
        else {
            HuSprBankSet(createRecordSelectionSpritesSelectionState, createRecordSelectionSpritesRecordPlayer->recordMinutes + 0xC, createRecordSelectionSpritesRecordPlayer->difficulty);
            HuSprPosSet(createRecordSelectionSpritesSelectionState, createRecordSelectionSpritesRecordPlayer->recordMinutes + 0xC, characterRecordSpriteOffsets[createRecordSelectionSpritesRecordPlayer->recordMinutes][0], characterRecordSpriteOffsets[createRecordSelectionSpritesRecordPlayer->recordMinutes][1] + 0x19);
            HuSprAttrReset(createRecordSelectionSpritesSelectionState, createRecordSelectionSpritesRecordPlayer->recordMinutes + 0xC, HUSPR_ATTR_DISPOFF);
        }
    }
    HuSprAttrReset(createRecordSelectionSpritesSelectionState, 0x10, HUSPR_ATTR_DISPOFF);
    HuSprGrpPosSet(createRecordSelectionSpritesSelectionState, 16.0f, -500.0f);
    for (createRecordSelectionSpritesPlayerIndex2 = 0; createRecordSelectionSpritesPlayerIndex2 <= 0x3C; createRecordSelectionSpritesPlayerIndex2++) {
        MentSleepFrame();
        if (createRecordSelectionSpritesPlayerIndex2 <= 0x32) {
            createRecordSelectionSpritesInterpolationValue = MentInterpolateSineEaseIn(-500.0f, 50.0f, createRecordSelectionSpritesPlayerIndex2, 50.0f);
        }
        else {
            createRecordSelectionSpritesInterpolationValue = MentInterpolateCosineEaseOut(50.0f, 40.0f, createRecordSelectionSpritesPlayerIndex2 - 0x32, 10.0f);
        }
        HuSprGrpPosSet(playerSelectionWork[0].selectionTimer, 16.0f, createRecordSelectionSpritesInterpolationValue);
    }
}

void MentUpdateRecordSelectionSprites(void)
{
    float updateRecordSelectionSpritesInterpolationValue;
    s32 updateRecordSelectionSpritesPlayerIndex;

    playerSelectionWork[0].playerSelections[0] = -1;
    playerSelectionWork[0].updateCallback = NULL;
    for (updateRecordSelectionSpritesPlayerIndex = 0; updateRecordSelectionSpritesPlayerIndex <= 0x4B; updateRecordSelectionSpritesPlayerIndex++) {
        MentSleepFrame();
        if (updateRecordSelectionSpritesPlayerIndex <= 0xA) {
            updateRecordSelectionSpritesInterpolationValue = MentInterpolateSineEaseIn(40.0f, 50.0f, updateRecordSelectionSpritesPlayerIndex, 10.0f);
        }
        else {
            updateRecordSelectionSpritesInterpolationValue = MentInterpolateCosineEaseOut(50.0f, -500.0f, updateRecordSelectionSpritesPlayerIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(playerSelectionWork[0].selectionTimer, 16.0f, updateRecordSelectionSpritesInterpolationValue);
        if (updateRecordSelectionSpritesPlayerIndex >= 0xF) {
            updateRecordSelectionSpritesInterpolationValue = MentInterpolateCosineEaseOut(210.0f, -500.0f, updateRecordSelectionSpritesPlayerIndex - 0xF, 60.0f);
            HuSprGrpData[playerSelectionWork[0].selectionIndex].pos.y = (s32)MentApproachValue(HuSprGrpData[playerSelectionWork[0].selectionIndex].pos.y, updateRecordSelectionSpritesInterpolationValue, 10.0f);
        }
    }
}

void MentCreatePlayerSelectionSprites(void)
{
    s32 createPlayerSelectionSpritesPlayerIndex;
    s32 createPlayerSelectionSpritesSelectionState;

    createPlayerSelectionSpritesSelectionState = playerSelectionWork[0].selectionIndex;
    for (createPlayerSelectionSpritesPlayerIndex = 0; createPlayerSelectionSpritesPlayerIndex < 7; createPlayerSelectionSpritesPlayerIndex++) {
        HuSprAttrSet(createPlayerSelectionSpritesSelectionState, createPlayerSelectionSpritesPlayerIndex, 4);
    }
}

void MentAnimatePlayerSelectionTransition(MentDllPlayerSelectionWork *selectionWork, s32 selectionIndex, s32 targetIndex, s32 animationFrames)
{
    s32 animatePlayerSelectionTransitionSpriteGroupId;
    s32 animatePlayerSelectionTransitionSpriteId;
    s32 animatePlayerSelectionTransitionPlayerIndex;
    s32 animatePlayerSelectionTransitionSelectionState;

    animatePlayerSelectionTransitionSpriteGroupId = HuSprGrpCreate(7);
    selectionWork->selectionIndex = animatePlayerSelectionTransitionSpriteGroupId;
    animatePlayerSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x3C)), 0x7DA, 0);
    HuSprGrpMemberSet(animatePlayerSelectionTransitionSpriteGroupId, 0, animatePlayerSelectionTransitionSpriteId);
    animatePlayerSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x3D)), 0x7C6, 0);
    HuSprGrpMemberSet(animatePlayerSelectionTransitionSpriteGroupId, 1, animatePlayerSelectionTransitionSpriteId);
    HuSprPosSet(animatePlayerSelectionTransitionSpriteGroupId, 1, 0.0f, -80.0f);
    animatePlayerSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x3E)), 0x7D0, 0);
    HuSprGrpMemberSet(animatePlayerSelectionTransitionSpriteGroupId, 2, animatePlayerSelectionTransitionSpriteId);
    animatePlayerSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x3F)), 0x7D0, 0);
    HuSprGrpMemberSet(animatePlayerSelectionTransitionSpriteGroupId, 3, animatePlayerSelectionTransitionSpriteId);
    HuSprPosSet(animatePlayerSelectionTransitionSpriteGroupId, 3, 0.0f, -80.0f);
    if (menuLaunchParameters[0] != 0) {
        animatePlayerSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x5A)), 0x7BC, 0);
        HuSprGrpMemberSet(animatePlayerSelectionTransitionSpriteGroupId, 4, animatePlayerSelectionTransitionSpriteId);
        HuSprPosSet(animatePlayerSelectionTransitionSpriteGroupId, 4, -140.0f, -86.0f);
        animatePlayerSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x5B)), 0x7BC, 0);
        HuSprGrpMemberSet(animatePlayerSelectionTransitionSpriteGroupId, 5, animatePlayerSelectionTransitionSpriteId);
        animatePlayerSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x5C)), 0x7D0, 0);
        HuSprGrpMemberSet(animatePlayerSelectionTransitionSpriteGroupId, 6, animatePlayerSelectionTransitionSpriteId);
        HuSprScaleSet(animatePlayerSelectionTransitionSpriteGroupId, 6, 1.0f, 0.875f);
        HuSprTPLvlSet(animatePlayerSelectionTransitionSpriteGroupId, 6, 0.5f);
    }
    HuSprExecLayerSet(0x40, 1);
    HuSprGrpDrawNoSet(animatePlayerSelectionTransitionSpriteGroupId, 0x40);
    animatePlayerSelectionTransitionSelectionState = playerSelectionWork[0].selectionIndex;
    for (animatePlayerSelectionTransitionPlayerIndex = 0; animatePlayerSelectionTransitionPlayerIndex < 7; animatePlayerSelectionTransitionPlayerIndex++) {
        HuSprAttrSet(animatePlayerSelectionTransitionSelectionState, animatePlayerSelectionTransitionPlayerIndex, 4);
    }
}

void MentAnimateSelectionWindow(s32 animationMode)
{
    float animateSelectionWindowAnimationProgress;
    float animateSelectionWindowAnimationProgress2;
    s32 animateSelectionWindowSelectionState;

    animateSelectionWindowSelectionState = playerSelectionWork[0].selectionIndex;
    if (animationMode != 5) {
        HuSprBankSet(animateSelectionWindowSelectionState, 0, animationMode);
        HuSprBankSet(animateSelectionWindowSelectionState, 1, animationMode);
        HuSprAttrSet(animateSelectionWindowSelectionState, 2, HUSPR_ATTR_DISPOFF);
        HuSprAttrSet(animateSelectionWindowSelectionState, 3, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(animateSelectionWindowSelectionState, 0, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(animateSelectionWindowSelectionState, 1, HUSPR_ATTR_DISPOFF);
        if (menuLaunchParameters[0] != 0) {
            if ((animationMode == 0 && _CheckFlag(FLAG_ID_MAKE(0, 2)) != 0) || (animationMode == 1 && _CheckFlag(FLAG_ID_MAKE(0, 3)) != 0)
                || (animationMode == 2 && _CheckFlag(FLAG_ID_MAKE(0, 4)) != 0) || (animationMode == 3 && (_CheckFlag(FLAG_ID_MAKE(0, 5)) != 0))
                || (animationMode == 4 && (_CheckFlag(FLAG_ID_MAKE(0, 6)) != 0))) {
                HuSprAttrReset(animateSelectionWindowSelectionState, 4, HUSPR_ATTR_DISPOFF);
                HuSprAttrReset(animateSelectionWindowSelectionState, 5, HUSPR_ATTR_DISPOFF);
                HuSprTPLvlSet(animateSelectionWindowSelectionState, 5, 0.8f);
                HuSprAttrReset(animateSelectionWindowSelectionState, 6, HUSPR_ATTR_DISPOFF);
                HuSprTPLvlSet(animateSelectionWindowSelectionState, 6, 0.5f);
            }
            else {
                HuSprAttrSet(animateSelectionWindowSelectionState, 4, 4);
                HuSprAttrSet(animateSelectionWindowSelectionState, 5, 4);
                HuSprAttrSet(animateSelectionWindowSelectionState, 6, 4);
            }
        }
    }
    else {
        HuSprAttrSet(animateSelectionWindowSelectionState, 0, HUSPR_ATTR_DISPOFF);
        HuSprAttrSet(animateSelectionWindowSelectionState, 1, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(animateSelectionWindowSelectionState, 2, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(animateSelectionWindowSelectionState, 3, HUSPR_ATTR_DISPOFF);
    }
    playerSelectionWork[0].playerSelections[0] = animationMode;
    playerSelectionWork[0].selectionFlags[1] = (rand8() % 10) * 0x24;
    HuSprGrpScaleSet(animateSelectionWindowSelectionState, 0.0f, 0.0f);
    for (animationMode = 0; animationMode <= 0xF; animationMode++) {
        MentSleepFrame();
        if (animationMode <= 0xA) {
            animateSelectionWindowAnimationProgress = MentInterpolateSineEaseIn(0.0f, 1.0f, animationMode, 10.0f);
            animateSelectionWindowAnimationProgress2 = 0.01f;
        }
        else {
            animateSelectionWindowAnimationProgress = 1.0f;
            animateSelectionWindowAnimationProgress2 = MentInterpolateSineEaseIn(0.01f, 1.0f, animationMode - 0xA, 5.0f);
        }
        HuSprGrpScaleSet(animateSelectionWindowSelectionState, animateSelectionWindowAnimationProgress, animateSelectionWindowAnimationProgress2);
    }
}

void MentUpdateSelectionWindow(void)
{
    float updateSelectionWindowInterpolationValue;
    float updateSelectionWindowInterpolationValue2;
    s32 updateSelectionWindowIndex;
    s32 updateSelectionWindowIndex2;
    s32 updateSelectionWindowSelectionState;
    s32 updateSelectionWindowSelectionState2;

    updateSelectionWindowSelectionState = playerSelectionWork[0].selectionIndex;
    for (updateSelectionWindowIndex = 0; updateSelectionWindowIndex <= 0xF; updateSelectionWindowIndex++) {
        MentSleepFrame();
        if (updateSelectionWindowIndex <= 0xA) {
            updateSelectionWindowInterpolationValue = 1.0f;
            updateSelectionWindowInterpolationValue2 = MentInterpolateSineEaseIn(1.0f, 0.01f, updateSelectionWindowIndex, 10.0f);
        }
        else {
            updateSelectionWindowInterpolationValue = MentInterpolateSineEaseIn(1.0f, 0.0f, updateSelectionWindowIndex - 0xA, 5.0f);
            updateSelectionWindowInterpolationValue2 = 0.01f;
        }
        HuSprGrpScaleSet(updateSelectionWindowSelectionState, updateSelectionWindowInterpolationValue, updateSelectionWindowInterpolationValue2);
    }
    updateSelectionWindowSelectionState2 = playerSelectionWork[0].selectionIndex;
    for (updateSelectionWindowIndex2 = 0; updateSelectionWindowIndex2 < 7; updateSelectionWindowIndex2++) {
        HuSprAttrSet(updateSelectionWindowSelectionState2, updateSelectionWindowIndex2, 4);
    }
    playerSelectionWork[0].playerSelections[0] = -1;
    playerSelectionWork[0].updateCallback = NULL;
}

void MentUpdatePlayerSelectionDisplay(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork)
{
    Vec updatePlayerSelectionDisplayTargetPosition;
    Vec updatePlayerSelectionDisplayTargetPosition2;
    s32 updatePlayerSelectionDisplaySelectionState;

    updatePlayerSelectionDisplaySelectionState = playerSelectionWork[0].selectionIndex;
    if (playerSelectionWork[0].playerSelections[0] != -1) {
        updatePlayerSelectionDisplayTargetPosition.x = menuCameraPositions[playerSelectionWork[0].playerSelections[0]].x;
        updatePlayerSelectionDisplayTargetPosition.y = 0.0f;
        updatePlayerSelectionDisplayTargetPosition.z = menuCameraPositions[playerSelectionWork[0].playerSelections[0]].z;
        Hu3D3Dto2D(&updatePlayerSelectionDisplayTargetPosition, 1, &updatePlayerSelectionDisplayTargetPosition2);
        updatePlayerSelectionDisplayTargetPosition2.y = MentInterpolateSineWave(210.0f, 220.0f, playerSelectionWork[0].selectionFlags[1]++, 360.0f);
        if (playerSelectionWork[0].selectionFlags[1] >= 0x168) {
            playerSelectionWork[0].selectionFlags[1] = 0;
        }
        HuSprGrpPosSet(updatePlayerSelectionDisplaySelectionState, updatePlayerSelectionDisplayTargetPosition2.x, (s32)updatePlayerSelectionDisplayTargetPosition2.y);
    }
}

#if VERSION_PAL

#define POSX1 353
#define POSX2 325

#else

#define POSX1 321
#define POSX2 295

#endif

void MentCreatePlayerSelectionWindows(void)
{
    s32 createPlayerSelectionWindowsPlayerIndex;
    s32 createPlayerSelectionWindowsSelectionState;

    createPlayerSelectionWindowsSelectionState = playerSelectionWork[0].selectionMode;
    for (createPlayerSelectionWindowsPlayerIndex = 0; createPlayerSelectionWindowsPlayerIndex < 0x5B; createPlayerSelectionWindowsPlayerIndex++) {
        HuSprAttrSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex, HUSPR_ATTR_DISPOFF);
    }
    if (menuLaunchParameters[2] != 5) {
        HuSprBankSet(createPlayerSelectionWindowsSelectionState, 0, menuLaunchParameters[2]);
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, 0, HUSPR_ATTR_DISPOFF);
    }
    else {
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, 1, HUSPR_ATTR_DISPOFF);
    }
    HuSprAttrReset(createPlayerSelectionWindowsSelectionState, 2, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(createPlayerSelectionWindowsSelectionState, 2, 288.0f, playerSelectionSpritePositions[0][1]);
    HuSprScaleSet(createPlayerSelectionWindowsSelectionState, 2, 1.0f, playerSelectionSpritePositions[0][2]);
    for (createPlayerSelectionWindowsPlayerIndex = 0; createPlayerSelectionWindowsPlayerIndex < 2; createPlayerSelectionWindowsPlayerIndex++) {
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 3, HUSPR_ATTR_DISPOFF);
        HuSprPosSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 3, playerSelectionSpritePositions[0][createPlayerSelectionWindowsPlayerIndex + 3], playerSelectionSpritePositions[0][1]);
    }
    for (createPlayerSelectionWindowsPlayerIndex = 0; createPlayerSelectionWindowsPlayerIndex < 4; createPlayerSelectionWindowsPlayerIndex++) {
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].character + 7, HUSPR_ATTR_DISPOFF);
        HuSprPosSet(createPlayerSelectionWindowsSelectionState, recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].character + 7, playerSelectionSpritePositions[recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes][0], playerSelectionSpritePositions[0][1]);
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes + 0xF, HUSPR_ATTR_DISPOFF);
        HuSprPosSet(
            createPlayerSelectionWindowsSelectionState, recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes + 0xF, 1.0f + playerSelectionSpritePositions[recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes][0], 2.0f + playerSelectionSpritePositions[0][1]);
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].character + 0x1F, HUSPR_ATTR_DISPOFF);
        HuSprPosSet(createPlayerSelectionWindowsSelectionState, recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].character + 0x1F, playerSelectionSpritePositions[recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes][0], playerSelectionSpritePositions[4][1] - 9.0f);
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes + 0x27, HUSPR_ATTR_DISPOFF);
        HuSprPosSet(
            createPlayerSelectionWindowsSelectionState, recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes + 0x27, 1.0f + playerSelectionSpritePositions[recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes][0], playerSelectionSpritePositions[4][1] - 7.0f);
        if (recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].isComputerPlayer != 0) {
            HuSprAttrReset(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x1B, HUSPR_ATTR_DISPOFF);
            HuSprBankSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x1B, recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].difficulty);
            HuSprPosSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x1B, playerSelectionSpritePositions[recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes][0], 21.0f + playerSelectionSpritePositions[0][1]);
        }
        else {
            HuSprAttrReset(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x17, HUSPR_ATTR_DISPOFF);
            HuSprBankSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x17, recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes);
            HuSprPosSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x17, playerSelectionSpritePositions[recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes][0] - 8.0f, playerSelectionSpritePositions[0][1] - 21.0f);
        }
        HuSprPosSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x2F, playerSelectionSpritePositions[recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes][0], 20.0f + playerSelectionSpritePositions[4][1]);
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x2F, HUSPR_ATTR_DISPOFF);
        HuSprPosSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x33, 10.0f + playerSelectionSpritePositions[recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes][0], 21.0f + playerSelectionSpritePositions[4][1]);
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x33, HUSPR_ATTR_DISPOFF);
        HuSprBankSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x33, 0);
        HuSprPosSet(createPlayerSelectionWindowsSelectionState, recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].character + 0x37, playerSelectionSpritePositions[recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes][0], playerSelectionSpritePositions[4][1] - 7.0f);
        HuSprPosSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x3F, playerSelectionSpritePositions[recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes][0], 23.0f + playerSelectionSpritePositions[4][1]);
        HuSprPosSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x43, 14.0f + playerSelectionSpritePositions[recordPlayerWork[createPlayerSelectionWindowsPlayerIndex].recordMinutes][0], 24.0f + playerSelectionSpritePositions[4][1]);
        HuSprBankSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x43, 0);
    }
    HuSprPosSet(createPlayerSelectionWindowsSelectionState, 0x47, 340.0f, playerSelectionSpritePositions[0][1]);
    HuSprScaleSet(createPlayerSelectionWindowsSelectionState, 0x47, 0.0f, 0.0f);
    HuSprAttrReset(createPlayerSelectionWindowsSelectionState, 0x47, HUSPR_ATTR_DISPOFF);
    #if VERSION_NTSC
    HuSprAttrReset(createPlayerSelectionWindowsSelectionState, 0x48, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(createPlayerSelectionWindowsSelectionState, 0x48, 366.0f, playerSelectionSpritePositions[1][1]);
    HuSprAttrReset(createPlayerSelectionWindowsSelectionState, 0x49, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(createPlayerSelectionWindowsSelectionState, 0x49, 367.0f, 2.0f + playerSelectionSpritePositions[1][1]);
    #endif
    for (createPlayerSelectionWindowsPlayerIndex = 0; createPlayerSelectionWindowsPlayerIndex < 2; createPlayerSelectionWindowsPlayerIndex++) {
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x4A, HUSPR_ATTR_DISPOFF);
        HuSprPosSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x4A, POSX1 - (createPlayerSelectionWindowsPlayerIndex * (POSX1-POSX2)), playerSelectionSpritePositions[1][1]);
        HuSprBankSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x4A, createPlayerSelectionWindowsPlayerIndex);
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x4C, HUSPR_ATTR_DISPOFF);
        HuSprPosSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x4C, POSX1+1 - (createPlayerSelectionWindowsPlayerIndex * (POSX1-POSX2)), 2.0f + playerSelectionSpritePositions[1][1]);
        HuSprBankSet(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x4C, createPlayerSelectionWindowsPlayerIndex);
    }
    HuSprAttrReset(createPlayerSelectionWindowsSelectionState, 0x4E, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(createPlayerSelectionWindowsSelectionState, 0x4E, 340.0f, playerSelectionSpritePositions[2][1]);
    HuSprBankSet(createPlayerSelectionWindowsSelectionState, 0x4E, 0);
    HuSprAttrReset(createPlayerSelectionWindowsSelectionState, 0x4F, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(createPlayerSelectionWindowsSelectionState, 0x4F, 341.0f, 2.0f + playerSelectionSpritePositions[2][1]);
    HuSprBankSet(createPlayerSelectionWindowsSelectionState, 0x4F, 0);
    HuSprAttrReset(createPlayerSelectionWindowsSelectionState, 0x50, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(createPlayerSelectionWindowsSelectionState, 0x50, 340.0f, playerSelectionSpritePositions[3][1]);
    HuSprBankSet(createPlayerSelectionWindowsSelectionState, 0x50, 0);
    HuSprAttrReset(createPlayerSelectionWindowsSelectionState, 0x51, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(createPlayerSelectionWindowsSelectionState, 0x51, 341.0f, 2.0f + playerSelectionSpritePositions[3][1]);
    HuSprBankSet(createPlayerSelectionWindowsSelectionState, 0x51, 0);
    HuSprAttrReset(createPlayerSelectionWindowsSelectionState, 0x52, HUSPR_ATTR_DISPOFF);
    for (createPlayerSelectionWindowsPlayerIndex = 0; createPlayerSelectionWindowsPlayerIndex < 4; createPlayerSelectionWindowsPlayerIndex++) {
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x53, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(createPlayerSelectionWindowsSelectionState, createPlayerSelectionWindowsPlayerIndex + 0x57, HUSPR_ATTR_DISPOFF);
    }
    for (createPlayerSelectionWindowsPlayerIndex = 0; createPlayerSelectionWindowsPlayerIndex < 5; createPlayerSelectionWindowsPlayerIndex++) {
        HuWinPosSet(playerSelectionWork[0].selectionHistory[createPlayerSelectionWindowsPlayerIndex], 100.0f, -500.0f + (playerSelectionSpritePositions[createPlayerSelectionWindowsPlayerIndex][1] - 21.0f));
    }
    for (createPlayerSelectionWindowsPlayerIndex = 0; createPlayerSelectionWindowsPlayerIndex < 5; createPlayerSelectionWindowsPlayerIndex++) {
        playerSelectionWork[1].playerSelections[createPlayerSelectionWindowsPlayerIndex] = playerSelectionWork[1].selectionFlags[createPlayerSelectionWindowsPlayerIndex] = 0;
    }
    playerSelectionWork[1].selectionFlags[1] = 0x14;
    HuSprBankSet(createPlayerSelectionWindowsSelectionState, 0x4A, 0);
    HuSprBankSet(createPlayerSelectionWindowsSelectionState, 0x4C, 0);
    HuSprBankSet(createPlayerSelectionWindowsSelectionState, 0x4B, 2);
    HuSprBankSet(createPlayerSelectionWindowsSelectionState, 0x4D, 2);
    HuSprGrpPosSet(createPlayerSelectionWindowsSelectionState, 0.0f, -500.0f);
}

void MentUpdatePlayerSelectionCursor(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork)
{
    float updatePlayerSelectionCursorCoordinateValue[2];
    float updatePlayerSelectionCursorInterpolationValue;
    float updatePlayerSelectionCursorInterpolationValue2;
    float updatePlayerSelectionCursorInterpolationValue3;
    s32 updatePlayerSelectionCursorSelectionState;
    s32 updatePlayerSelectionCursorPlayerIndex;
    s32 updatePlayerSelectionCursorPlayerIndex2;
    s32 updatePlayerSelectionCursorSelectionState2;
    s32 updatePlayerSelectionCursorSelectionState3;

    updatePlayerSelectionCursorSelectionState3 = 0;
    updatePlayerSelectionCursorSelectionState = playerSelectionWork[0].selectionMode;
    {
        s32 cursorPositionOffsets[4][4] = {
            { 0x106, 0x106, 0x106, 0x106 },
            { 0x13A, 0x133, 0x175, 0x175 },
            { 0x16E, 0x175, 0x133, 0x1A2 },
            { 0x1A2, 0x1A2, 0x1A2, 0x133 },
        };
        playerSelectionWork[0].playerSelections[3] = 0;
        updatePlayerSelectionCursorSelectionState2 = selectionWork->playerSelections[0];
        updatePlayerSelectionCursorCoordinateValue[0] = HuSprData[HuSprGrpData[updatePlayerSelectionCursorSelectionState].members[3]].pos.x;
        updatePlayerSelectionCursorCoordinateValue[1] = HuSprData[HuSprGrpData[updatePlayerSelectionCursorSelectionState].members[4]].pos.x;
        if ((HuPadBtnDown[recordPlayerWork->controllerPort] & PAD_BUTTON_A) != 0) {
            selectionWork->playerSelections[0]++;
            HuAudFXPlay(2);
            if (selectionWork->playerSelections[0] > 4) {
                playerSelectionWork[0].playerSelections[1] = 1;
                selectionWork->playerSelections[0] = 4;
                MentUpdatePlayerMenuWindows();
            }
            playerSelectionWork[0].playerSelections[3] = 1;
        }
        else if ((HuPadBtnDown[recordPlayerWork->controllerPort] & PAD_BUTTON_B) != 0) {
            selectionWork->playerSelections[0]--;
            HuAudFXPlay(3);
            if (selectionWork->playerSelections[0] < 0) {
                playerSelectionWork[0].playerSelections[1] = -1;
                selectionWork->playerSelections[0] = 0;
                MentUpdatePlayerMenuWindows();
            }
            playerSelectionWork[0].playerSelections[3] = 1;
        }
        else if ((HuPadStkX[recordPlayerWork->controllerPort] >= 5) || ((HuPadBtn[recordPlayerWork->controllerPort] & PAD_BUTTON_RIGHT) != 0)) {
            updatePlayerSelectionCursorSelectionState3 = 1;
            playerSelectionWork[0].playerSelections[3] = 1;
            HuAudFXPlay(0);
            switch (selectionWork->playerSelections[0]) {
                case 0:
                    selectionWork->selectionFlags[0] += 1;
                    if (selectionWork->selectionFlags[0] > 3) {
                        selectionWork->selectionFlags[0] = 0;
                    }
                    break;
                case 1:
                    selectionWork->selectionFlags[1] += 5;
                    if (selectionWork->selectionFlags[1] > 0x32) {
                        selectionWork->selectionFlags[1] = 0xA;
                    }
                    break;
                case 2:
                    if (menuLaunchParameters[6] == 1) {
                        selectionWork->selectionFlags[2]++;
                        if (selectionWork->selectionFlags[2] > 2) {
                            selectionWork->selectionFlags[2] = 0;
                        }
                    }
                    else {
                        selectionWork->selectionFlags[2]++;
                        if (selectionWork->selectionFlags[2] > 1) {
                            selectionWork->selectionFlags[2] = 0;
                        }
                    }
                    break;
                case 3:
                    selectionWork->selectionFlags[3]++;
                    if (selectionWork->selectionFlags[3] > 1) {
                        selectionWork->selectionFlags[3] = 0;
                    }
                    break;
                case 4:
                    selectionWork->selectionFlags[4] += 1;
                    if (selectionWork->selectionFlags[4] > 3) {
                        selectionWork->selectionFlags[4] = 0;
                    }
                    break;
            }
        }
        else if ((HuPadStkX[recordPlayerWork->controllerPort] <= -5) || ((HuPadBtn[recordPlayerWork->controllerPort] & PAD_BUTTON_LEFT) != 0)) {
            updatePlayerSelectionCursorSelectionState3 = -1;
            playerSelectionWork[0].playerSelections[3] = 1;
            HuAudFXPlay(0);
            switch (selectionWork->playerSelections[0]) {
                case 0:
                    selectionWork->selectionFlags[0] -= 1;
                    if (selectionWork->selectionFlags[0] < 0) {
                        selectionWork->selectionFlags[0] = 3;
                    }
                    break;
                case 1:
                    selectionWork->selectionFlags[1] -= 5;
                    if (selectionWork->selectionFlags[1] < 0xA) {
                        selectionWork->selectionFlags[1] = 0x32;
                    }
                    break;
                case 2:
                    if (menuLaunchParameters[6] == 1) {
                        selectionWork->selectionFlags[2]--;
                        if (selectionWork->selectionFlags[2] < 0) {
                            selectionWork->selectionFlags[2] = 2;
                        }
                    }
                    else {
                        selectionWork->selectionFlags[2]--;
                        if (selectionWork->selectionFlags[2] < 0) {
                            selectionWork->selectionFlags[2] = 1;
                        }
                    }
                    break;
                case 3:
                    selectionWork->selectionFlags[3]--;
                    if (selectionWork->selectionFlags[3] < 0) {
                        selectionWork->selectionFlags[3] = 1;
                    }
                    break;
                case 4:
                    selectionWork->selectionFlags[4] -= 1;
                    if (selectionWork->selectionFlags[4] < 0) {
                        selectionWork->selectionFlags[4] = 3;
                    }
                    break;
            }
        }
        else if (selectionWork->playerSelections[0] == 4) {
            if ((HuPadStkY[recordPlayerWork->controllerPort] >= 0x1E) || ((HuPadBtn[recordPlayerWork->controllerPort] & PAD_BUTTON_UP) != 0)) {
                playerSelectionWork[0].playerSelections[3] = 1;
                HuAudFXPlay(0);
                for (updatePlayerSelectionCursorPlayerIndex = 0; updatePlayerSelectionCursorPlayerIndex <= 0xA; updatePlayerSelectionCursorPlayerIndex++) {
                    MentSleepFrame();
                    if (updatePlayerSelectionCursorPlayerIndex <= 5) {
                        updatePlayerSelectionCursorInterpolationValue3 = MentInterpolateSineEaseIn(0.0f, 10.0f, updatePlayerSelectionCursorPlayerIndex, 5.0f);
                    }
                    else {
                        updatePlayerSelectionCursorInterpolationValue3 = MentInterpolateSineEaseIn(10.0f, 0.0f, updatePlayerSelectionCursorPlayerIndex - 5, 5.0f);
                    }
                    HuSprPosSet(updatePlayerSelectionCursorSelectionState, 5, playerSelectionSpritePositions[selectionWork->selectionFlags[4]][0], (playerSelectionSpritePositions[4][1] - 48.0f) - updatePlayerSelectionCursorInterpolationValue3);
                    if (updatePlayerSelectionCursorPlayerIndex == 3) {
                        selectionWork->playerSelections[selectionWork->selectionFlags[4] + 1]--;
                        if (selectionWork->playerSelections[selectionWork->selectionFlags[4] + 1] < 0) {
                            selectionWork->playerSelections[selectionWork->selectionFlags[4] + 1] = 9;
                        }
                        HuSprBankSet(updatePlayerSelectionCursorSelectionState, selectionWork->selectionFlags[4] + 0x33, selectionWork->playerSelections[selectionWork->selectionFlags[4] + 1]);
                        HuSprBankSet(updatePlayerSelectionCursorSelectionState, selectionWork->selectionFlags[4] + 0x43, selectionWork->playerSelections[selectionWork->selectionFlags[4] + 1]);
                    }
                }
            }
            else if ((HuPadStkY[recordPlayerWork->controllerPort] <= -0x1E) || ((HuPadBtn[recordPlayerWork->controllerPort] & 4) != 0)) {
                playerSelectionWork[0].playerSelections[3] = 1;
                HuAudFXPlay(0);
                for (updatePlayerSelectionCursorPlayerIndex = 0; updatePlayerSelectionCursorPlayerIndex <= 0xA; updatePlayerSelectionCursorPlayerIndex++) {
                    MentSleepFrame();
                    if (updatePlayerSelectionCursorPlayerIndex <= 5) {
                        updatePlayerSelectionCursorInterpolationValue3 = MentInterpolateSineEaseIn(0.0f, 10.0f, updatePlayerSelectionCursorPlayerIndex, 5.0f);
                    }
                    else {
                        updatePlayerSelectionCursorInterpolationValue3 = MentInterpolateSineEaseIn(10.0f, 0.0f, updatePlayerSelectionCursorPlayerIndex - 5, 5.0f);
                    }
                    HuSprPosSet(updatePlayerSelectionCursorSelectionState, 6, playerSelectionSpritePositions[selectionWork->selectionFlags[4]][0], 49.0f + playerSelectionSpritePositions[4][1] + updatePlayerSelectionCursorInterpolationValue3);
                    if (updatePlayerSelectionCursorPlayerIndex == 3) {
                        selectionWork->playerSelections[selectionWork->selectionFlags[4] + 1]++;
                        if (selectionWork->playerSelections[selectionWork->selectionFlags[4] + 1] > 9) {
                            selectionWork->playerSelections[selectionWork->selectionFlags[4] + 1] = 0;
                        }
                        HuSprBankSet(updatePlayerSelectionCursorSelectionState, selectionWork->selectionFlags[4] + 0x33, selectionWork->playerSelections[selectionWork->selectionFlags[4] + 1]);
                        HuSprBankSet(updatePlayerSelectionCursorSelectionState, selectionWork->selectionFlags[4] + 0x43, selectionWork->playerSelections[selectionWork->selectionFlags[4] + 1]);
                    }
                }
            }
        }
        if (updatePlayerSelectionCursorSelectionState3 != 0) {
            for (updatePlayerSelectionCursorPlayerIndex = 0; updatePlayerSelectionCursorPlayerIndex <= 0xA; updatePlayerSelectionCursorPlayerIndex++) {
                MentSleepFrame();
                if (updatePlayerSelectionCursorPlayerIndex <= 5) {
                    updatePlayerSelectionCursorInterpolationValue = MentInterpolateSineEaseIn(0.0f, 10.0f, updatePlayerSelectionCursorPlayerIndex, 5.0f);
                    updatePlayerSelectionCursorInterpolationValue2 = MentInterpolateSineEaseIn(1.0f, 0.0f, updatePlayerSelectionCursorPlayerIndex, 5.0f);
                }
                else {
                    updatePlayerSelectionCursorInterpolationValue = MentInterpolateSineEaseIn(10.0f, 0.0f, updatePlayerSelectionCursorPlayerIndex - 5, 5.0f);
                    updatePlayerSelectionCursorInterpolationValue2 = MentInterpolateSineEaseIn(0.0f, 1.0f, updatePlayerSelectionCursorPlayerIndex - 5, 5.0f);
                }
                if (selectionWork->playerSelections[0] != 4) {
                    if (updatePlayerSelectionCursorSelectionState3 == 1) {
                        HuSprPosSet(updatePlayerSelectionCursorSelectionState, 4, updatePlayerSelectionCursorInterpolationValue + playerSelectionSpritePositions[selectionWork->playerSelections[0]][4], playerSelectionSpritePositions[selectionWork->playerSelections[0]][1]);
                    }
                    else {
                        HuSprPosSet(updatePlayerSelectionCursorSelectionState, 3, playerSelectionSpritePositions[selectionWork->playerSelections[0]][3] - updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[selectionWork->playerSelections[0]][1]);
                    }
                }
                switch (selectionWork->playerSelections[0]) {
                    case 0:
                        updatePlayerSelectionCursorSelectionState2 = selectionWork->selectionFlags[0] - updatePlayerSelectionCursorSelectionState3;
                        if (updatePlayerSelectionCursorSelectionState2 > 3) {
                            updatePlayerSelectionCursorSelectionState2 = 0;
                        }
                        else if (updatePlayerSelectionCursorSelectionState2 < 0) {
                            updatePlayerSelectionCursorSelectionState2 = 3;
                        }
                        if (selectionWork->selectionFlags[0] != 0) {
                            updatePlayerSelectionCursorInterpolationValue2 = MentInterpolateSineEaseIn(0.0f, 1.0f, updatePlayerSelectionCursorPlayerIndex, 10.0f);
                        }
                        else {
                            updatePlayerSelectionCursorInterpolationValue2 = MentInterpolateSineEaseIn(1.0f, 0.0f, updatePlayerSelectionCursorPlayerIndex, 10.0f);
                        }
                        HuSprScaleSet(updatePlayerSelectionCursorSelectionState, 0x47, updatePlayerSelectionCursorInterpolationValue2, updatePlayerSelectionCursorInterpolationValue2);
                        for (updatePlayerSelectionCursorPlayerIndex2 = 0; updatePlayerSelectionCursorPlayerIndex2 < 4; updatePlayerSelectionCursorPlayerIndex2++) {
                            updatePlayerSelectionCursorInterpolationValue = MentInterpolateSineEaseIn(
                                cursorPositionOffsets[recordPlayerWork[updatePlayerSelectionCursorPlayerIndex2].recordMinutes][updatePlayerSelectionCursorSelectionState2], cursorPositionOffsets[recordPlayerWork[updatePlayerSelectionCursorPlayerIndex2].recordMinutes][selectionWork->selectionFlags[0]], updatePlayerSelectionCursorPlayerIndex, 10.0f);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, recordPlayerWork[updatePlayerSelectionCursorPlayerIndex2].character + 7, updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[0][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, recordPlayerWork[updatePlayerSelectionCursorPlayerIndex2].character + 0xF, 1.0f + updatePlayerSelectionCursorInterpolationValue, 2.0f + playerSelectionSpritePositions[0][1]);
                            if (recordPlayerWork[updatePlayerSelectionCursorPlayerIndex2].isComputerPlayer != 0) {
                                HuSprPosSet(updatePlayerSelectionCursorSelectionState, updatePlayerSelectionCursorPlayerIndex2 + 0x1B, updatePlayerSelectionCursorInterpolationValue, 21.0f + playerSelectionSpritePositions[0][1]);
                            }
                            else {
                                HuSprPosSet(updatePlayerSelectionCursorSelectionState, updatePlayerSelectionCursorPlayerIndex2 + 0x17, updatePlayerSelectionCursorInterpolationValue - 8.0f, playerSelectionSpritePositions[0][1] - 21.0f);
                            }
                        }
                        break;
                    case 1:
                        if (updatePlayerSelectionCursorSelectionState3 == 1) {
                            #if VERSION_NTSC
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x48, 366.0f + updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[1][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x49, 367.0f + updatePlayerSelectionCursorInterpolationValue, 2.0f + playerSelectionSpritePositions[1][1]);
                            #endif
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x4A, POSX1 + updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[1][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x4C, POSX1+1 + updatePlayerSelectionCursorInterpolationValue, 2.0f + playerSelectionSpritePositions[1][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x4B, POSX2 + updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[1][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x4D, POSX2+1 + updatePlayerSelectionCursorInterpolationValue, 2.0f + playerSelectionSpritePositions[1][1]);
                        }
                        else {
                            #if VERSION_NTSC
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x48, 366.0f - updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[1][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x49, 367.0f - updatePlayerSelectionCursorInterpolationValue, 2.0f + playerSelectionSpritePositions[1][1]);
                            #endif
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x4A, POSX1 - updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[1][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x4C, POSX1+1 - updatePlayerSelectionCursorInterpolationValue, 2.0f + playerSelectionSpritePositions[1][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x4B, POSX2 - updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[1][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x4D, POSX2+1 - updatePlayerSelectionCursorInterpolationValue, 2.0f + playerSelectionSpritePositions[1][1]);
                        }
                        #if VERSION_NTSC
                        HuSprScaleSet(updatePlayerSelectionCursorSelectionState, 0x48, 1.0f, updatePlayerSelectionCursorInterpolationValue2);
                        HuSprScaleSet(updatePlayerSelectionCursorSelectionState, 0x49, 1.0f, updatePlayerSelectionCursorInterpolationValue2);
                        #endif
                        HuSprScaleSet(updatePlayerSelectionCursorSelectionState, 0x4A, 1.0f, updatePlayerSelectionCursorInterpolationValue2);
                        HuSprScaleSet(updatePlayerSelectionCursorSelectionState, 0x4C, 1.0f, updatePlayerSelectionCursorInterpolationValue2);
                        HuSprScaleSet(updatePlayerSelectionCursorSelectionState, 0x4B, 1.0f, updatePlayerSelectionCursorInterpolationValue2);
                        HuSprScaleSet(updatePlayerSelectionCursorSelectionState, 0x4D, 1.0f, updatePlayerSelectionCursorInterpolationValue2);
                        break;
                    case 2:
                        if (updatePlayerSelectionCursorSelectionState3 == 1) {
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x4E, 340.0f + updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[2][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x4F, 341.0f + updatePlayerSelectionCursorInterpolationValue, 2.0f + playerSelectionSpritePositions[2][1]);
                        }
                        else {
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x4E, 340.0f - updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[2][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x4F, 341.0f - updatePlayerSelectionCursorInterpolationValue, 2.0f + playerSelectionSpritePositions[2][1]);
                        }
                        HuSprScaleSet(updatePlayerSelectionCursorSelectionState, 0x4E, 1.0f, updatePlayerSelectionCursorInterpolationValue2);
                        HuSprScaleSet(updatePlayerSelectionCursorSelectionState, 0x4F, 1.0f, updatePlayerSelectionCursorInterpolationValue2);
                        break;
                    case 3:
                        if (updatePlayerSelectionCursorSelectionState3 == 1) {
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x50, 340.0f + updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[3][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x51, 341.0f + updatePlayerSelectionCursorInterpolationValue, 2.0f + playerSelectionSpritePositions[3][1]);
                        }
                        else {
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x50, 340.0f - updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[3][1]);
                            HuSprPosSet(updatePlayerSelectionCursorSelectionState, 0x51, 341.0f - updatePlayerSelectionCursorInterpolationValue, 2.0f + playerSelectionSpritePositions[3][1]);
                        }
                        HuSprScaleSet(updatePlayerSelectionCursorSelectionState, 0x50, 1.0f, updatePlayerSelectionCursorInterpolationValue2);
                        HuSprScaleSet(updatePlayerSelectionCursorSelectionState, 0x51, 1.0f, updatePlayerSelectionCursorInterpolationValue2);
                        break;
                    case 4:
                        updatePlayerSelectionCursorInterpolationValue = MentInterpolateSineEaseIn(updatePlayerSelectionCursorCoordinateValue[0], playerSelectionSpritePositions[selectionWork->selectionFlags[4]][0] - 32.0f, updatePlayerSelectionCursorPlayerIndex, 10.0f);
                        HuSprPosSet(updatePlayerSelectionCursorSelectionState, 3, updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[4][1]);
                        updatePlayerSelectionCursorInterpolationValue = MentInterpolateSineEaseIn(updatePlayerSelectionCursorCoordinateValue[1], 33.0f + playerSelectionSpritePositions[selectionWork->selectionFlags[4]][0], updatePlayerSelectionCursorPlayerIndex, 10.0f);
                        HuSprPosSet(updatePlayerSelectionCursorSelectionState, 4, updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[4][1]);
                        updatePlayerSelectionCursorInterpolationValue = MentInterpolateSineEaseIn(32.0f + updatePlayerSelectionCursorCoordinateValue[0], playerSelectionSpritePositions[selectionWork->selectionFlags[4]][0], updatePlayerSelectionCursorPlayerIndex, 10.0f);
                        HuSprPosSet(updatePlayerSelectionCursorSelectionState, 5, updatePlayerSelectionCursorInterpolationValue, playerSelectionSpritePositions[4][1] - 48.0f);
                        HuSprPosSet(updatePlayerSelectionCursorSelectionState, 6, updatePlayerSelectionCursorInterpolationValue, 49.0f + playerSelectionSpritePositions[4][1]);
                        if (updatePlayerSelectionCursorPlayerIndex == 3) {
                            for (updatePlayerSelectionCursorPlayerIndex2 = 0; updatePlayerSelectionCursorPlayerIndex2 < 4; updatePlayerSelectionCursorPlayerIndex2++) {
                                HuSprAttrSet(updatePlayerSelectionCursorSelectionState, recordPlayerWork[updatePlayerSelectionCursorPlayerIndex2].character + 0x37, HUSPR_ATTR_DISPOFF);
                                HuSprAttrSet(updatePlayerSelectionCursorSelectionState, updatePlayerSelectionCursorPlayerIndex2 + 0x3F, HUSPR_ATTR_DISPOFF);
                                HuSprAttrSet(updatePlayerSelectionCursorSelectionState, updatePlayerSelectionCursorPlayerIndex2 + 0x43, HUSPR_ATTR_DISPOFF);
                            }
                            HuSprAttrReset(updatePlayerSelectionCursorSelectionState, recordPlayerWork[selectionWork->selectionFlags[4]].character + 0x37, HUSPR_ATTR_DISPOFF);
                            HuSprAttrReset(updatePlayerSelectionCursorSelectionState, selectionWork->selectionFlags[4] + 0x3F, HUSPR_ATTR_DISPOFF);
                            HuSprAttrReset(updatePlayerSelectionCursorSelectionState, selectionWork->selectionFlags[4] + 0x43, HUSPR_ATTR_DISPOFF);
                        }
                        break;
                }
                if (updatePlayerSelectionCursorPlayerIndex == 6) {
                    switch (selectionWork->playerSelections[0]) {
                        case 1:
                            HuSprBankSet(updatePlayerSelectionCursorSelectionState, 0x4A, selectionWork->selectionFlags[1] % 10);
                            HuSprBankSet(updatePlayerSelectionCursorSelectionState, 0x4C, selectionWork->selectionFlags[1] % 10);
                            HuSprBankSet(updatePlayerSelectionCursorSelectionState, 0x4B, selectionWork->selectionFlags[1] / 10);
                            HuSprBankSet(updatePlayerSelectionCursorSelectionState, 0x4D, selectionWork->selectionFlags[1] / 10);
                            break;
                        case 2:
                            HuSprBankSet(updatePlayerSelectionCursorSelectionState, 0x4E, selectionWork->selectionFlags[2]);
                            HuSprBankSet(updatePlayerSelectionCursorSelectionState, 0x4F, selectionWork->selectionFlags[2]);
                            break;
                        case 3:
                            HuSprBankSet(updatePlayerSelectionCursorSelectionState, 0x50, selectionWork->selectionFlags[3]);
                            HuSprBankSet(updatePlayerSelectionCursorSelectionState, 0x51, selectionWork->selectionFlags[3]);
                            break;
                    }
                }
            }
        }
        else if (updatePlayerSelectionCursorSelectionState2 != selectionWork->playerSelections[0]) {
            for (updatePlayerSelectionCursorPlayerIndex = 0; updatePlayerSelectionCursorPlayerIndex <= 0xA; updatePlayerSelectionCursorPlayerIndex++) {
                MentSleepFrame();
                updatePlayerSelectionCursorInterpolationValue3 = MentInterpolateSineEaseIn(playerSelectionSpritePositions[updatePlayerSelectionCursorSelectionState2][1], playerSelectionSpritePositions[selectionWork->playerSelections[0]][1], updatePlayerSelectionCursorPlayerIndex, 10.0f);
                updatePlayerSelectionCursorInterpolationValue2 = MentInterpolateSineEaseIn(playerSelectionSpritePositions[updatePlayerSelectionCursorSelectionState2][2], playerSelectionSpritePositions[selectionWork->playerSelections[0]][2], updatePlayerSelectionCursorPlayerIndex, 10.0f);
                HuSprPosSet(updatePlayerSelectionCursorSelectionState, 2, 288.0f, updatePlayerSelectionCursorInterpolationValue3);
                HuSprScaleSet(updatePlayerSelectionCursorSelectionState, 2, 1.0f, updatePlayerSelectionCursorInterpolationValue2);
                if (selectionWork->playerSelections[0] != 4) {
                    updatePlayerSelectionCursorInterpolationValue = MentInterpolateSineEaseIn(updatePlayerSelectionCursorCoordinateValue[0], playerSelectionSpritePositions[selectionWork->playerSelections[0]][3], updatePlayerSelectionCursorPlayerIndex, 10.0f);
                    HuSprPosSet(updatePlayerSelectionCursorSelectionState, 3, updatePlayerSelectionCursorInterpolationValue, updatePlayerSelectionCursorInterpolationValue3);
                    updatePlayerSelectionCursorInterpolationValue = MentInterpolateSineEaseIn(updatePlayerSelectionCursorCoordinateValue[1], playerSelectionSpritePositions[selectionWork->playerSelections[0]][4], updatePlayerSelectionCursorPlayerIndex, 10.0f);
                    HuSprPosSet(updatePlayerSelectionCursorSelectionState, 4, updatePlayerSelectionCursorInterpolationValue, updatePlayerSelectionCursorInterpolationValue3);
                    updatePlayerSelectionCursorInterpolationValue2 = MentInterpolateSineEaseIn(1.0f, 0.0f, updatePlayerSelectionCursorPlayerIndex, 10.0f);
                    HuSprTPLvlSet(updatePlayerSelectionCursorSelectionState, 5, updatePlayerSelectionCursorInterpolationValue2);
                    HuSprTPLvlSet(updatePlayerSelectionCursorSelectionState, 6, updatePlayerSelectionCursorInterpolationValue2);
                    updatePlayerSelectionCursorInterpolationValue3 = MentInterpolateSineEaseIn(0.0f, 20.0f, updatePlayerSelectionCursorPlayerIndex, 10.0f);
                    HuSprPosSet(updatePlayerSelectionCursorSelectionState, 5, playerSelectionSpritePositions[selectionWork->selectionFlags[4]][0], (playerSelectionSpritePositions[4][1] - updatePlayerSelectionCursorInterpolationValue3) - 48.0f);
                    HuSprPosSet(updatePlayerSelectionCursorSelectionState, 6, playerSelectionSpritePositions[selectionWork->selectionFlags[4]][0], 49.0f + (playerSelectionSpritePositions[4][1] + updatePlayerSelectionCursorInterpolationValue3));
                    for (updatePlayerSelectionCursorPlayerIndex2 = 0; updatePlayerSelectionCursorPlayerIndex2 < 4; updatePlayerSelectionCursorPlayerIndex2++) {
                        HuSprAttrSet(updatePlayerSelectionCursorSelectionState, recordPlayerWork[updatePlayerSelectionCursorPlayerIndex2].character + 0x37, HUSPR_ATTR_DISPOFF);
                        HuSprAttrSet(updatePlayerSelectionCursorSelectionState, updatePlayerSelectionCursorPlayerIndex2 + 0x3F, HUSPR_ATTR_DISPOFF);
                        HuSprAttrSet(updatePlayerSelectionCursorSelectionState, updatePlayerSelectionCursorPlayerIndex2 + 0x43, HUSPR_ATTR_DISPOFF);
                    }
                }
                else {
                    updatePlayerSelectionCursorInterpolationValue = MentInterpolateSineEaseIn(updatePlayerSelectionCursorCoordinateValue[0], playerSelectionSpritePositions[selectionWork->selectionFlags[4]][0] - 32.0f, updatePlayerSelectionCursorPlayerIndex, 10.0f);
                    HuSprPosSet(updatePlayerSelectionCursorSelectionState, 3, updatePlayerSelectionCursorInterpolationValue, updatePlayerSelectionCursorInterpolationValue3);
                    updatePlayerSelectionCursorInterpolationValue = MentInterpolateSineEaseIn(updatePlayerSelectionCursorCoordinateValue[1], 33.0f + playerSelectionSpritePositions[selectionWork->selectionFlags[4]][0], updatePlayerSelectionCursorPlayerIndex, 10.0f);
                    HuSprPosSet(updatePlayerSelectionCursorSelectionState, 4, updatePlayerSelectionCursorInterpolationValue, updatePlayerSelectionCursorInterpolationValue3);
                    HuSprAttrReset(updatePlayerSelectionCursorSelectionState, 5, HUSPR_ATTR_DISPOFF);
                    HuSprAttrReset(updatePlayerSelectionCursorSelectionState, 6, HUSPR_ATTR_DISPOFF);
                    updatePlayerSelectionCursorInterpolationValue2 = MentInterpolateSineEaseIn(0.0f, 1.0f, updatePlayerSelectionCursorPlayerIndex, 10.0f);
                    HuSprTPLvlSet(updatePlayerSelectionCursorSelectionState, 5, updatePlayerSelectionCursorInterpolationValue2);
                    HuSprTPLvlSet(updatePlayerSelectionCursorSelectionState, 6, updatePlayerSelectionCursorInterpolationValue2);
                    updatePlayerSelectionCursorInterpolationValue3 = MentInterpolateSineEaseIn(20.0f, 0.0f, updatePlayerSelectionCursorPlayerIndex, 10.0f);
                    HuSprPosSet(updatePlayerSelectionCursorSelectionState, 5, playerSelectionSpritePositions[selectionWork->selectionFlags[4]][0], (playerSelectionSpritePositions[4][1] - updatePlayerSelectionCursorInterpolationValue3) - 48.0f);
                    HuSprPosSet(updatePlayerSelectionCursorSelectionState, 6, playerSelectionSpritePositions[selectionWork->selectionFlags[4]][0], 49.0f + (playerSelectionSpritePositions[4][1] + updatePlayerSelectionCursorInterpolationValue3));
                    if (updatePlayerSelectionCursorPlayerIndex == 3) {
                        HuSprAttrReset(updatePlayerSelectionCursorSelectionState, recordPlayerWork[selectionWork->selectionFlags[4]].character + 0x37, HUSPR_ATTR_DISPOFF);
                        HuSprAttrReset(updatePlayerSelectionCursorSelectionState, selectionWork->selectionFlags[4] + 0x3F, HUSPR_ATTR_DISPOFF);
                        HuSprAttrReset(updatePlayerSelectionCursorSelectionState, selectionWork->selectionFlags[4] + 0x43, HUSPR_ATTR_DISPOFF);
                    }
                }
            }
            if (selectionWork->playerSelections[0] != 4) {
                HuSprAttrSet(updatePlayerSelectionCursorSelectionState, 5, HUSPR_ATTR_DISPOFF);
                HuSprAttrSet(updatePlayerSelectionCursorSelectionState, 6, HUSPR_ATTR_DISPOFF);
            }
        }
        playerSelectionWork[0].playerSelections[3] = 0;
    }
}

#undef POSX2
#undef POSX1

void MentAnimatePlayerMenuTransition(MentDllPlayerSelectionWork *selectionWork, s32 selectionIndex, s32 targetIndex, s32 animationFrames)
{
    s32 animatePlayerMenuTransitionPlayerIndex;
    s32 animatePlayerMenuTransitionSpriteId;
    s32 animatePlayerMenuTransitionSpriteGroupId;
    ANIMDATA *animatePlayerMenuTransitionSpriteAnimation1;
    ANIMDATA *animatePlayerMenuTransitionSpriteAnimation2;
    ANIMDATA *animatePlayerMenuTransitionSpriteAnimation3;

    s32 menuTransitionSpritePositions[4][2] = {
        { 0x00000070, 0x00000060 },
        { 0x000001D2, 0x00000060 },
        { 0x00000070, 0x0000012D },
        { 0x000001D2, 0x0000012D },
    };
    animatePlayerMenuTransitionSpriteGroupId = HuSprGrpCreate(0x5B);
    selectionWork->selectionMode = animatePlayerMenuTransitionSpriteGroupId;
    animatePlayerMenuTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x3D)), 0x7C6, 0);
    HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, 0, animatePlayerMenuTransitionSpriteId);
    HuSprPosSet(animatePlayerMenuTransitionSpriteGroupId, 0, 288.0f, 54.0f);
    animatePlayerMenuTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x3F)), 0x7C6, 0);
    HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, 1, animatePlayerMenuTransitionSpriteId);
    HuSprPosSet(animatePlayerMenuTransitionSpriteGroupId, 1, 288.0f, 54.0f);
    animatePlayerMenuTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x41)), 0x7BC, 0);
    HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, 2, animatePlayerMenuTransitionSpriteId);
    animatePlayerMenuTransitionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x42));
    for (animatePlayerMenuTransitionPlayerIndex = 0; animatePlayerMenuTransitionPlayerIndex < 4; animatePlayerMenuTransitionPlayerIndex++) {
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation1, 0, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 3, animatePlayerMenuTransitionSpriteId);
        HuSprBankSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 3, animatePlayerMenuTransitionPlayerIndex);
    }
    animatePlayerMenuTransitionSpriteAnimation2 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x39));
    for (animatePlayerMenuTransitionPlayerIndex = 0; animatePlayerMenuTransitionPlayerIndex < 8; animatePlayerMenuTransitionPlayerIndex++) {
        animatePlayerMenuTransitionSpriteAnimation1 = HuSprAnimReadFile(animatePlayerMenuTransitionPlayerIndex + DATA_MAKE_NUM(DATADIR_MENT, 0x31));
        animatePlayerMenuTransitionSpriteAnimation3 = HuSprAnimReadFile(animatePlayerMenuTransitionPlayerIndex + DATA_MAKE_NUM(DATADIR_MENT, 0x4E));
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation1, 0x3E8, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 7, animatePlayerMenuTransitionSpriteId);
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation2, 0x3F2, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0xF, animatePlayerMenuTransitionSpriteId);
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation1, 0x3E8, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x1F, animatePlayerMenuTransitionSpriteId);
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation2, 0x3F2, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x27, animatePlayerMenuTransitionSpriteId);
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation3, 0x3DE, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x37, animatePlayerMenuTransitionSpriteId);
    }
    animatePlayerMenuTransitionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x3A));
    animatePlayerMenuTransitionSpriteAnimation3 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x3B));
    for (animatePlayerMenuTransitionPlayerIndex = 0; animatePlayerMenuTransitionPlayerIndex < 4; animatePlayerMenuTransitionPlayerIndex++) {
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation1, 0x3DE, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x17, animatePlayerMenuTransitionSpriteId);
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation3, 0x3DE, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x1B, animatePlayerMenuTransitionSpriteId);
    }
    animatePlayerMenuTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x43)), 0x3DE, 0);
    HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, 0x47, animatePlayerMenuTransitionSpriteId);
    animatePlayerMenuTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x44)), 0x3E8, 0);
    HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, 0x48, animatePlayerMenuTransitionSpriteId);
    animatePlayerMenuTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x46)), 0x3F2, 0);
    HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, 0x49, animatePlayerMenuTransitionSpriteId);
    animatePlayerMenuTransitionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x45));
    animatePlayerMenuTransitionSpriteAnimation3 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x47));
    for (animatePlayerMenuTransitionPlayerIndex = 0; animatePlayerMenuTransitionPlayerIndex < 2; animatePlayerMenuTransitionPlayerIndex++) {
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation1, 0x3E8, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x4A, animatePlayerMenuTransitionSpriteId);
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation3, 0x3F2, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x4C, animatePlayerMenuTransitionSpriteId);
    }
    animatePlayerMenuTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x48)), 0x3E8, 0);
    HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, 0x4E, animatePlayerMenuTransitionSpriteId);
    animatePlayerMenuTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x49)), 0x3F2, 0);
    HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, 0x4F, animatePlayerMenuTransitionSpriteId);
    animatePlayerMenuTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x4A)), 0x3E8, 0);
    HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, 0x50, animatePlayerMenuTransitionSpriteId);
    animatePlayerMenuTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x4B)), 0x3F2, 0);
    HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, 0x51, animatePlayerMenuTransitionSpriteId);
    animatePlayerMenuTransitionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x4C));
    animatePlayerMenuTransitionSpriteAnimation3 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x4D));
    for (animatePlayerMenuTransitionPlayerIndex = 0; animatePlayerMenuTransitionPlayerIndex < 4; animatePlayerMenuTransitionPlayerIndex++) {
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation1, 0x3DE, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x2F, animatePlayerMenuTransitionSpriteId);
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation3, 0x3D4, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x33, animatePlayerMenuTransitionSpriteId);
    }
    animatePlayerMenuTransitionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x56));
    animatePlayerMenuTransitionSpriteAnimation3 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x57));
    for (animatePlayerMenuTransitionPlayerIndex = 0; animatePlayerMenuTransitionPlayerIndex < 4; animatePlayerMenuTransitionPlayerIndex++) {
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation1, 0x3CA, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x3F, animatePlayerMenuTransitionSpriteId);
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation3, 0x3C0, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x43, animatePlayerMenuTransitionSpriteId);
    }
    animatePlayerMenuTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x40)), 0x7D0, 0);
    HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, 0x52, animatePlayerMenuTransitionSpriteId);
    HuSprTPLvlSet(animatePlayerMenuTransitionSpriteGroupId, 0x52, 0.8f);
    MentSetSpritePosition(animatePlayerMenuTransitionSpriteGroupId, 0x52, 288.0f, 240.0f);
    animatePlayerMenuTransitionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x18));
    animatePlayerMenuTransitionSpriteAnimation3 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x19));
    for (animatePlayerMenuTransitionPlayerIndex = 0; animatePlayerMenuTransitionPlayerIndex < 4; animatePlayerMenuTransitionPlayerIndex++) {
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation1, 0x7BC, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x53, animatePlayerMenuTransitionSpriteId);
        HuSprBankSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x53, animatePlayerMenuTransitionPlayerIndex);
        MentSetSpritePosition(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x53, menuTransitionSpritePositions[animatePlayerMenuTransitionPlayerIndex][0], menuTransitionSpritePositions[animatePlayerMenuTransitionPlayerIndex][1]);
        animatePlayerMenuTransitionSpriteId = HuSprCreate(animatePlayerMenuTransitionSpriteAnimation3, 0x7C6, 0);
        HuSprGrpMemberSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x57, animatePlayerMenuTransitionSpriteId);
        HuSprBankSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x57, animatePlayerMenuTransitionPlayerIndex);
        HuSprTPLvlSet(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x57, 0.5f);
        MentSetSpritePosition(animatePlayerMenuTransitionSpriteGroupId, animatePlayerMenuTransitionPlayerIndex + 0x57, menuTransitionSpritePositions[animatePlayerMenuTransitionPlayerIndex][0] + 1, menuTransitionSpritePositions[animatePlayerMenuTransitionPlayerIndex][1] + 6);
    }
    for (animatePlayerMenuTransitionPlayerIndex = 0; animatePlayerMenuTransitionPlayerIndex < 5; animatePlayerMenuTransitionPlayerIndex++) {
        selectionWork->selectionHistory[animatePlayerMenuTransitionPlayerIndex] = HuWinExCreateStyled(0.0f, 0.0f, 0xC8, 0xC8, -1, 1);
        HuWinBGTPLvlSet(selectionWork->selectionHistory[animatePlayerMenuTransitionPlayerIndex], 0.0f);
        HuWinMesSet(selectionWork->selectionHistory[animatePlayerMenuTransitionPlayerIndex], animatePlayerMenuTransitionPlayerIndex + 0x1B0010);
        HuWinMesSpeedSet(selectionWork->selectionHistory[animatePlayerMenuTransitionPlayerIndex], 0);
        HuWinDispOn(selectionWork->selectionHistory[animatePlayerMenuTransitionPlayerIndex]);
        winData[selectionWork->selectionHistory[animatePlayerMenuTransitionPlayerIndex]].mess_shadow_color = 0;
    }
    MentCreatePlayerSelectionWindows();
}

void MentCreatePlayerMenuWindows(void)
{
    s32 createPlayerMenuWindowsPlayerIndex;
    s32 createPlayerMenuWindowsStateValue;
    MentDllPlayerSelectionWork *createPlayerMenuWindowsPlayerSelection;

    createPlayerMenuWindowsStateValue = playerSelectionWork[0].selectionMode;
    createPlayerMenuWindowsPlayerSelection = &playerSelectionWork[1];
    HuSprAttrReset(createPlayerMenuWindowsStateValue, 2, HUSPR_ATTR_DISPOFF);
    if (playerSelectionWork[1].playerSelections[0] == 4) {
        for (createPlayerMenuWindowsPlayerIndex = 0; createPlayerMenuWindowsPlayerIndex < 4; createPlayerMenuWindowsPlayerIndex++) {
            HuSprAttrReset(createPlayerMenuWindowsStateValue, createPlayerMenuWindowsPlayerIndex + 3, HUSPR_ATTR_DISPOFF);
        }
        HuSprAttrReset(createPlayerMenuWindowsStateValue, recordPlayerWork[createPlayerMenuWindowsPlayerSelection->selectionFlags[4]].character + 0x37, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(createPlayerMenuWindowsStateValue, createPlayerMenuWindowsPlayerSelection->selectionFlags[4] + 0x3F, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(createPlayerMenuWindowsStateValue, createPlayerMenuWindowsPlayerSelection->selectionFlags[4] + 0x43, HUSPR_ATTR_DISPOFF);
        return;
    }
    for (createPlayerMenuWindowsPlayerIndex = 0; createPlayerMenuWindowsPlayerIndex < 2; createPlayerMenuWindowsPlayerIndex++) {
        HuSprAttrReset(createPlayerMenuWindowsStateValue, createPlayerMenuWindowsPlayerIndex + 3, HUSPR_ATTR_DISPOFF);
    }
}

void MentUpdatePlayerMenuWindows(void)
{
    MentDllPlayerSelectionWork *updatePlayerMenuWindowsPlayerSelection;
    s32 updatePlayerMenuWindowsStateValue;
    s32 updatePlayerMenuWindowsPlayerIndex;

    updatePlayerMenuWindowsStateValue = playerSelectionWork[0].selectionMode;
    updatePlayerMenuWindowsPlayerSelection = &playerSelectionWork[1];
    HuSprAttrSet(updatePlayerMenuWindowsStateValue, 2, HUSPR_ATTR_DISPOFF);
    for (updatePlayerMenuWindowsPlayerIndex = 0; updatePlayerMenuWindowsPlayerIndex < 4; updatePlayerMenuWindowsPlayerIndex++) {
        HuSprAttrSet(updatePlayerMenuWindowsStateValue, updatePlayerMenuWindowsPlayerIndex + 3, HUSPR_ATTR_DISPOFF);
    }
    HuSprAttrSet(updatePlayerMenuWindowsStateValue, recordPlayerWork[updatePlayerMenuWindowsPlayerSelection->selectionFlags[4]].character + 0x37, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(updatePlayerMenuWindowsStateValue, updatePlayerMenuWindowsPlayerSelection->selectionFlags[4] + 0x3F, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(updatePlayerMenuWindowsStateValue, updatePlayerMenuWindowsPlayerSelection->selectionFlags[4] + 0x43, HUSPR_ATTR_DISPOFF);
}

void MentCreateCharacterSelectionSprites(void)
{
    float createCharacterSelectionSpritesInterpolationValue;
    s32 createCharacterSelectionSpritesIndex;
    MentDllPlayerSelectionWork *createCharacterSelectionSpritesPlayerSelection;
    s32 createCharacterSelectionSpritesIndex2;
    s32 createCharacterSelectionSpritesSelectionState;
    s32 createCharacterSelectionSpritesIndex3;

    MentCreatePlayerSelectionWindows();
    createCharacterSelectionSpritesSelectionState = playerSelectionWork[0].selectionMode;
    createCharacterSelectionSpritesPlayerSelection = &playerSelectionWork[1];
    HuSprAttrSet(createCharacterSelectionSpritesSelectionState, 2, 4);
    for (createCharacterSelectionSpritesIndex3 = 0; createCharacterSelectionSpritesIndex3 < 4; createCharacterSelectionSpritesIndex3++) {
        HuSprAttrSet(createCharacterSelectionSpritesSelectionState, createCharacterSelectionSpritesIndex3 + 3, 4);
    }
    HuSprAttrSet(createCharacterSelectionSpritesSelectionState, recordPlayerWork[createCharacterSelectionSpritesPlayerSelection->selectionFlags[4]].character + 0x37, 4);
    HuSprAttrSet(createCharacterSelectionSpritesSelectionState, createCharacterSelectionSpritesPlayerSelection->selectionFlags[4] + 0x3F, 4);
    HuSprAttrSet(createCharacterSelectionSpritesSelectionState, createCharacterSelectionSpritesPlayerSelection->selectionFlags[4] + 0x43, 4);
    for (createCharacterSelectionSpritesIndex = 0; createCharacterSelectionSpritesIndex <= 60; createCharacterSelectionSpritesIndex++) {
        MentSleepFrame();
        if (createCharacterSelectionSpritesIndex <= 0x32) {
            createCharacterSelectionSpritesInterpolationValue = MentInterpolateSineEaseIn(-500.0f, 10.0f, createCharacterSelectionSpritesIndex, 50.0f);
        }
        else {
            createCharacterSelectionSpritesInterpolationValue = MentInterpolateCosineEaseOut(10.0f, 0.0f, createCharacterSelectionSpritesIndex - 0x32, 10.0f);
        }
        HuSprGrpPosSet(playerSelectionWork[0].selectionMode, 0.0f, createCharacterSelectionSpritesInterpolationValue);
        for (createCharacterSelectionSpritesIndex2 = 0; createCharacterSelectionSpritesIndex2 < 5; createCharacterSelectionSpritesIndex2++) {
            HuWinPosSet(playerSelectionWork->selectionWindows[createCharacterSelectionSpritesIndex2], 100.0f, playerSelectionSpritePositions[createCharacterSelectionSpritesIndex2][1] - 21.0f + createCharacterSelectionSpritesInterpolationValue);
        }
    }
}

void MentUpdateCharacterSelectionSprites(void)
{
    float updateCharacterSelectionSpritesInterpolationValue;
    s32 updateCharacterSelectionSpritesIndex;
    MentDllPlayerSelectionWork *updateCharacterSelectionSpritesPlayerSelection;
    s32 updateCharacterSelectionSpritesIndex2;
    s32 updateCharacterSelectionSpritesSelectionState;
    s32 updateCharacterSelectionSpritesIndex3;

    updateCharacterSelectionSpritesSelectionState = playerSelectionWork[0].selectionMode;
    updateCharacterSelectionSpritesPlayerSelection = &playerSelectionWork[1];
    HuSprAttrSet(updateCharacterSelectionSpritesSelectionState, 2, 4);
    for (updateCharacterSelectionSpritesIndex3 = 0; updateCharacterSelectionSpritesIndex3 < 4; updateCharacterSelectionSpritesIndex3++) {
        HuSprAttrSet(updateCharacterSelectionSpritesSelectionState, updateCharacterSelectionSpritesIndex3 + 3, 4);
    }
    HuSprAttrSet(updateCharacterSelectionSpritesSelectionState, recordPlayerWork[updateCharacterSelectionSpritesPlayerSelection->selectionFlags[4]].character + 0x37, 4);
    HuSprAttrSet(updateCharacterSelectionSpritesSelectionState, updateCharacterSelectionSpritesPlayerSelection->selectionFlags[4] + 0x3F, 4);
    HuSprAttrSet(updateCharacterSelectionSpritesSelectionState, updateCharacterSelectionSpritesPlayerSelection->selectionFlags[4] + 0x43, 4);
    for (updateCharacterSelectionSpritesIndex = 0; updateCharacterSelectionSpritesIndex <= 0x3C; updateCharacterSelectionSpritesIndex++) {
        MentSleepFrame();
        if (updateCharacterSelectionSpritesIndex <= 0xA) {
            updateCharacterSelectionSpritesInterpolationValue = MentInterpolateSineEaseIn(0.0f, 10.0f, updateCharacterSelectionSpritesIndex, 10.0f);
        }
        else {
            updateCharacterSelectionSpritesInterpolationValue = MentInterpolateCosineEaseOut(10.0f, -500.0f, updateCharacterSelectionSpritesIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(playerSelectionWork[0].selectionMode, 0.0f, updateCharacterSelectionSpritesInterpolationValue);
        for (updateCharacterSelectionSpritesIndex2 = 0; updateCharacterSelectionSpritesIndex2 < 5; updateCharacterSelectionSpritesIndex2++) {
            HuWinPosSet(playerSelectionWork->selectionWindows[updateCharacterSelectionSpritesIndex2], 100.0f, playerSelectionSpritePositions[updateCharacterSelectionSpritesIndex2][1] - 21.0f + updateCharacterSelectionSpritesInterpolationValue);
        }
    }
}

void MentCreateCharacterSelectionWindows(void)
{
    s32 createCharacterSelectionWindowsIndex;
    s32 createCharacterSelectionWindowsSelectionState;

    createCharacterSelectionWindowsSelectionState = playerSelectionWork[0].selectionState;
    for (createCharacterSelectionWindowsIndex = 0; createCharacterSelectionWindowsIndex < 0x22; createCharacterSelectionWindowsIndex++) {
        HuSprAttrSet(createCharacterSelectionWindowsSelectionState, createCharacterSelectionWindowsIndex, HUSPR_ATTR_DISPOFF);
    }
    HuSprAttrReset(createCharacterSelectionWindowsSelectionState, 0, HUSPR_ATTR_DISPOFF);
    for (createCharacterSelectionWindowsIndex = 0; createCharacterSelectionWindowsIndex < 4; createCharacterSelectionWindowsIndex++) {
        HuSprAttrReset(createCharacterSelectionWindowsSelectionState, createCharacterSelectionWindowsIndex + 1, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(createCharacterSelectionWindowsSelectionState, createCharacterSelectionWindowsIndex + 5, HUSPR_ATTR_DISPOFF);
    }
    HuSprPosSet(createCharacterSelectionWindowsSelectionState, 0x1B, 288.0f, characterSelectSpritePositions[1][2]);
    HuSprScaleSet(createCharacterSelectionWindowsSelectionState, 0x1B, 1.0f, 0.75f);
    HuSprPosSet(createCharacterSelectionWindowsSelectionState, 0x1C, characterSelectSpritePositions[1][3], characterSelectSpritePositions[1][2]);
    HuSprPosSet(createCharacterSelectionWindowsSelectionState, 0x1D, characterSelectSpritePositions[1][4], characterSelectSpritePositions[1][2]);
    for (createCharacterSelectionWindowsIndex = 0; createCharacterSelectionWindowsIndex < 8; createCharacterSelectionWindowsIndex++) {
        HuSprBankSet(createCharacterSelectionWindowsSelectionState, createCharacterSelectionWindowsIndex + 9, 0);
        HuSprAttrReset(createCharacterSelectionWindowsSelectionState, createCharacterSelectionWindowsIndex + 9, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(createCharacterSelectionWindowsSelectionState, createCharacterSelectionWindowsIndex + 0x11, HUSPR_ATTR_DISPOFF);
    }
    HuSprAttrReset(createCharacterSelectionWindowsSelectionState, 0x19, HUSPR_ATTR_DISPOFF);
    HuSprAttrReset(createCharacterSelectionWindowsSelectionState, 0x1A, HUSPR_ATTR_DISPOFF);
    HuSprBankSet(createCharacterSelectionWindowsSelectionState, 0x1E, 0);
    HuSprAttrReset(createCharacterSelectionWindowsSelectionState, 0x1E, HUSPR_ATTR_DISPOFF);
    HuSprBankSet(createCharacterSelectionWindowsSelectionState, 0x1F, 0);
    HuSprAttrReset(createCharacterSelectionWindowsSelectionState, 0x1F, HUSPR_ATTR_DISPOFF);
    HuSprBankSet(createCharacterSelectionWindowsSelectionState, 0x20, 0);
    HuSprAttrReset(createCharacterSelectionWindowsSelectionState, 0x20, HUSPR_ATTR_DISPOFF);
    HuSprBankSet(createCharacterSelectionWindowsSelectionState, 0x21, 0);
    HuSprAttrReset(createCharacterSelectionWindowsSelectionState, 0x21, HUSPR_ATTR_DISPOFF);
    for (createCharacterSelectionWindowsIndex = 0; createCharacterSelectionWindowsIndex < 3; createCharacterSelectionWindowsIndex++) {
        HuWinPosSet(playerSelectionWork->selectionWindows[createCharacterSelectionWindowsIndex], 100.0f, characterSelectSpritePositions[createCharacterSelectionWindowsIndex][2] - 0x209);
    }
    playerSelectionWork[0].playerSelections[0] = 1;
    playerSelectionWork[0].selectionFlags[0] = playerSelectionWork[0].selectionFlags[0] = 0;
    HuSprGrpPosSet(createCharacterSelectionWindowsSelectionState, 0.0f, -500.0f);
}

void MentUpdateCharacterSelectionWindows(void)
{
    float updateCharacterSelectionWindowsInterpolationValue;
    float updateCharacterSelectionWindowsInterpolationValue2;
    float updateCharacterSelectionWindowsInterpolationValue3;
    s32 updateCharacterSelectionWindowsSelectionState;
    s32 updateCharacterSelectionWindowsIndex;

    updateCharacterSelectionWindowsSelectionState = playerSelectionWork[0].selectionState;
    for (updateCharacterSelectionWindowsIndex = 0; updateCharacterSelectionWindowsIndex < 8; updateCharacterSelectionWindowsIndex++) {
        HuSprBankSet(updateCharacterSelectionWindowsSelectionState, updateCharacterSelectionWindowsIndex + 9, 0);
    }
    HuSprBankSet(updateCharacterSelectionWindowsSelectionState, 0x19, 0);
    HuSprBankSet(updateCharacterSelectionWindowsSelectionState, 0x1A, 0);
    HuSprPosSet(updateCharacterSelectionWindowsSelectionState, 0x1B, 288.0f, characterSelectSpritePositions[1][2]);
    HuSprScaleSet(updateCharacterSelectionWindowsSelectionState, 0x1B, 1.0f, 0.75f);
    HuSprPosSet(updateCharacterSelectionWindowsSelectionState, 0x1C, characterSelectSpritePositions[1][3], characterSelectSpritePositions[1][2]);
    HuSprPosSet(updateCharacterSelectionWindowsSelectionState, 0x1D, characterSelectSpritePositions[1][4], characterSelectSpritePositions[1][2]);
    for (updateCharacterSelectionWindowsIndex = 0; updateCharacterSelectionWindowsIndex <= 0xA; updateCharacterSelectionWindowsIndex++) {
        MentSleepFrame();
        if (updateCharacterSelectionWindowsIndex <= 5) {
            updateCharacterSelectionWindowsInterpolationValue = MentInterpolateSineEaseIn(1.0f, 0.0f, updateCharacterSelectionWindowsIndex, 5.0f);
        }
        else {
            updateCharacterSelectionWindowsInterpolationValue = MentInterpolateSineEaseIn(0.0f, 1.0f, updateCharacterSelectionWindowsIndex - 5, 5.0f);
        }
        if (recordPlayerWork->character != 0) {
            updateCharacterSelectionWindowsInterpolationValue2 = MentInterpolateSineEaseIn(characterSelectSpritePositions[recordPlayerWork->character % 4][0], characterSelectSpritePositions[0][0], updateCharacterSelectionWindowsIndex, 10.0f);
            updateCharacterSelectionWindowsInterpolationValue3 = MentInterpolateSineEaseIn(characterSelectSpritePositions[recordPlayerWork->character / 4][1], characterSelectSpritePositions[0][1], updateCharacterSelectionWindowsIndex, 10.0f);
            HuSprPosSet(updateCharacterSelectionWindowsSelectionState, 0x19, updateCharacterSelectionWindowsInterpolationValue2, updateCharacterSelectionWindowsInterpolationValue3);
            HuSprPosSet(updateCharacterSelectionWindowsSelectionState, 0x1A, updateCharacterSelectionWindowsInterpolationValue2, updateCharacterSelectionWindowsInterpolationValue3);
        }
        if (playerSelectionWork[0].selectionFlags[0] != 0) {
            HuSprScaleSet(updateCharacterSelectionWindowsSelectionState, 0x1E, 1.0f, updateCharacterSelectionWindowsInterpolationValue);
            HuSprScaleSet(updateCharacterSelectionWindowsSelectionState, 0x1F, 1.0f, updateCharacterSelectionWindowsInterpolationValue);
        }
        if (playerSelectionWork[0].selectionFlags[1] != 0) {
            HuSprScaleSet(updateCharacterSelectionWindowsSelectionState, 0x20, 1.0f, updateCharacterSelectionWindowsInterpolationValue);
            HuSprScaleSet(updateCharacterSelectionWindowsSelectionState, 0x21, 1.0f, updateCharacterSelectionWindowsInterpolationValue);
        }
        if (updateCharacterSelectionWindowsIndex == 6) {
            HuSprBankSet(updateCharacterSelectionWindowsSelectionState, 0x1E, 0);
            HuSprBankSet(updateCharacterSelectionWindowsSelectionState, 0x1F, 0);
            HuSprBankSet(updateCharacterSelectionWindowsSelectionState, 0x20, 0);
            HuSprBankSet(updateCharacterSelectionWindowsSelectionState, 0x21, 0);
        }
    }
    recordPlayerWork[0].recordFields[0] = recordPlayerWork[0].character = 0;
    playerSelectionWork[0].playerSelections[0] = 1;
    playerSelectionWork[0].selectionFlags[0] = playerSelectionWork[0].selectionFlags[1] = 0;
}

void MentAnimateCharacterSelectionTransition(MentDllPlayerSelectionWork *selectionWork, s32 selectionIndex, s32 targetIndex, s32 animationFrames)
{
    s32 animateCharacterSelectionTransitionIndex;
    s32 animateCharacterSelectionTransitionSelectionState;
    s32 animateCharacterSelectionTransitionSpriteId;
    ANIMDATA *animateCharacterSelectionTransitionSpriteAnimation1;
    ANIMDATA *animateCharacterSelectionTransitionSpriteAnimation2;

    s32 characterTransitionSpritePositions[4][2] = {
        { 0x00000070, 0x00000037 },
        { 0x000001D2, 0x00000037 },
        { 0x00000070, 0x0000011A },
        { 0x000001D2, 0x0000011A },
    };
    selectionWork->selectionState = animateCharacterSelectionTransitionSelectionState = (s16)HuSprGrpCreate(0x22);
    animateCharacterSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x58)), 0x7D0, 0);
    HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, 0, animateCharacterSelectionTransitionSpriteId);
    HuSprTPLvlSet(animateCharacterSelectionTransitionSelectionState, 0, 0.8f);
    MentSetSpritePosition(animateCharacterSelectionTransitionSelectionState, 0, 288.0f, 240.0f);
    animateCharacterSelectionTransitionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x18));
    animateCharacterSelectionTransitionSpriteAnimation2 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x19));
    for (animateCharacterSelectionTransitionIndex = 0; animateCharacterSelectionTransitionIndex < 4; animateCharacterSelectionTransitionIndex++) {
        animateCharacterSelectionTransitionSpriteId = HuSprCreate(animateCharacterSelectionTransitionSpriteAnimation1, 0x7BC, 0);
        HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 1, animateCharacterSelectionTransitionSpriteId);
        HuSprBankSet(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 1, animateCharacterSelectionTransitionIndex);
        MentSetSpritePosition(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 1, characterTransitionSpritePositions[animateCharacterSelectionTransitionIndex][0], characterTransitionSpritePositions[animateCharacterSelectionTransitionIndex][1]);
        animateCharacterSelectionTransitionSpriteId = HuSprCreate(animateCharacterSelectionTransitionSpriteAnimation2, 0x7C6, 0);
        HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 5, animateCharacterSelectionTransitionSpriteId);
        HuSprBankSet(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 5, animateCharacterSelectionTransitionIndex);
        HuSprTPLvlSet(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 5, 0.5f);
        MentSetSpritePosition(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 5, characterTransitionSpritePositions[animateCharacterSelectionTransitionIndex][0] + 1, characterTransitionSpritePositions[animateCharacterSelectionTransitionIndex][1] + 6);
    }
    animateCharacterSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x41)), 0x7BC, 0);
    HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, 0x1B, animateCharacterSelectionTransitionSpriteId);
    animateCharacterSelectionTransitionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x42));
    for (animateCharacterSelectionTransitionIndex = 0; animateCharacterSelectionTransitionIndex < 2; animateCharacterSelectionTransitionIndex++) {
        animateCharacterSelectionTransitionSpriteId = HuSprCreate(animateCharacterSelectionTransitionSpriteAnimation1, 0, 0);
        HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 0x1C, animateCharacterSelectionTransitionSpriteId);
        HuSprBankSet(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 0x1C, animateCharacterSelectionTransitionIndex);
    }
    animateCharacterSelectionTransitionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x22));
    for (animateCharacterSelectionTransitionIndex = 0; animateCharacterSelectionTransitionIndex < 8; animateCharacterSelectionTransitionIndex++) {
        animateCharacterSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(animateCharacterSelectionTransitionIndex + DATA_MAKE_NUM(DATADIR_MENT, 0x1A)), 0x3E8, 0);
        HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 9, animateCharacterSelectionTransitionSpriteId);
        HuSprPosSet(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 9, characterSelectSpritePositions[animateCharacterSelectionTransitionIndex % 4][0], characterSelectSpritePositions[animateCharacterSelectionTransitionIndex / 4][1]);
        animateCharacterSelectionTransitionSpriteId = HuSprCreate(animateCharacterSelectionTransitionSpriteAnimation1, 0x3F2, 0);
        HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 0x11, animateCharacterSelectionTransitionSpriteId);
        HuSprTPLvlSet(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 0x11, 0.5f);
        HuSprPosSet(animateCharacterSelectionTransitionSelectionState, animateCharacterSelectionTransitionIndex + 0x11, characterSelectSpritePositions[animateCharacterSelectionTransitionIndex % 4][0] + 1, characterSelectSpritePositions[animateCharacterSelectionTransitionIndex / 4][1] + 2);
    }
    animateCharacterSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x23)), 0, 0);
    HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, 0x19, animateCharacterSelectionTransitionSpriteId);
    HuSprPosSet(animateCharacterSelectionTransitionSelectionState, 0x19, characterSelectSpritePositions[0][0], characterSelectSpritePositions[0][1]);
    animateCharacterSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x28)), 0xA, 0);
    HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, 0x1A, animateCharacterSelectionTransitionSpriteId);
    HuSprPosSet(animateCharacterSelectionTransitionSelectionState, 0x1A, characterSelectSpritePositions[0][0], characterSelectSpritePositions[0][1]);
    animateCharacterSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x5D)), 0x3E8, 0);
    HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, 0x1E, animateCharacterSelectionTransitionSpriteId);
    HuSprPosSet(animateCharacterSelectionTransitionSelectionState, 0x1E, 340.0f, characterSelectSpritePositions[1][2]);
    animateCharacterSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x5E)), 0x3F2, 0);
    HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, 0x1F, animateCharacterSelectionTransitionSpriteId);
    HuSprTPLvlSet(animateCharacterSelectionTransitionSelectionState, 0x1F, 0.5f);
    HuSprPosSet(animateCharacterSelectionTransitionSelectionState, 0x1F, 341.0f, characterSelectSpritePositions[1][2] + 2);
    animateCharacterSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x48)), 0x3E8, 0);
    HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, 0x20, animateCharacterSelectionTransitionSpriteId);
    HuSprPosSet(animateCharacterSelectionTransitionSelectionState, 0x20, 340.0f, characterSelectSpritePositions[2][2]);
    animateCharacterSelectionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x49)), 0x3F2, 0);
    HuSprGrpMemberSet(animateCharacterSelectionTransitionSelectionState, 0x21, animateCharacterSelectionTransitionSpriteId);
    HuSprTPLvlSet(animateCharacterSelectionTransitionSelectionState, 0x21, 0.5f);
    HuSprPosSet(animateCharacterSelectionTransitionSelectionState, 0x21, 341.0f, characterSelectSpritePositions[2][2] + 2);
    for (animateCharacterSelectionTransitionIndex = 0; animateCharacterSelectionTransitionIndex < 3; animateCharacterSelectionTransitionIndex++) {
        selectionWork->selectionHistory[animateCharacterSelectionTransitionIndex] = HuWinExCreateStyled(0.0f, 0.0f, 0xC8, 0xC8, -1, 1);
        HuWinBGTPLvlSet(selectionWork->selectionHistory[animateCharacterSelectionTransitionIndex], 0.0f);
        HuWinMesSet(selectionWork->selectionHistory[animateCharacterSelectionTransitionIndex], animateCharacterSelectionTransitionIndex + 0x1B002D);
        HuWinMesSpeedSet(selectionWork->selectionHistory[animateCharacterSelectionTransitionIndex], 0);
        HuWinDispOn(selectionWork->selectionHistory[animateCharacterSelectionTransitionIndex]);
        if (animateCharacterSelectionTransitionIndex == 0) {
            HuWinDispOff(selectionWork->selectionHistory[animateCharacterSelectionTransitionIndex]);
        }
        winData[selectionWork->selectionHistory[animateCharacterSelectionTransitionIndex]].mess_shadow_color = 0;
    }
    MentCreateCharacterSelectionWindows();
}

void MentCreateBoardSelectionSprites()
{
    s32 createBoardSelectionSpritesSelectionState;
    s32 createBoardSelectionSpritesBoardIndex;

    createBoardSelectionSpritesSelectionState = playerSelectionWork[0].selectionState;
    HuSprAttrReset(createBoardSelectionSpritesSelectionState, 0x1B, HUSPR_ATTR_DISPOFF);
    for (createBoardSelectionSpritesBoardIndex = 0; createBoardSelectionSpritesBoardIndex < 2; createBoardSelectionSpritesBoardIndex++) {
        HuSprAttrReset(createBoardSelectionSpritesSelectionState, createBoardSelectionSpritesBoardIndex + 0x1C, HUSPR_ATTR_DISPOFF);
    }
}

void MentUpdateBoardSelectionSprites(void)
{
    s32 updateBoardSelectionSpritesSelectionState;
    s32 updateBoardSelectionSpritesBoardIndex;

    updateBoardSelectionSpritesSelectionState = playerSelectionWork[0].selectionState;
    HuSprAttrSet(updateBoardSelectionSpritesSelectionState, 0x1B, 4);
    for (updateBoardSelectionSpritesBoardIndex = 0; updateBoardSelectionSpritesBoardIndex < 2; updateBoardSelectionSpritesBoardIndex++) {
        HuSprAttrSet(updateBoardSelectionSpritesSelectionState, updateBoardSelectionSpritesBoardIndex + 0x1C, 4);
    }
}

void MentCreateBoardSelectionWindows(void)
{
    float createBoardSelectionWindowsInterpolationValue;
    s32 createBoardSelectionWindowsBoardIndex;
    s32 createBoardSelectionWindowsBoardIndex2;
    s32 createBoardSelectionWindowsBoardIndex3;
    s32 createBoardSelectionWindowsSelectionState;

    MentCreateCharacterSelectionWindows();
    createBoardSelectionWindowsSelectionState = playerSelectionWork[0].selectionState;
    HuSprAttrSet(createBoardSelectionWindowsSelectionState, 0x1B, 4);
    for (createBoardSelectionWindowsBoardIndex3 = 0; createBoardSelectionWindowsBoardIndex3 < 2; createBoardSelectionWindowsBoardIndex3++) {
        HuSprAttrSet(createBoardSelectionWindowsSelectionState, createBoardSelectionWindowsBoardIndex3 + 0x1C, 4);
    }
    for (createBoardSelectionWindowsBoardIndex = 0; createBoardSelectionWindowsBoardIndex <= 0x3C; createBoardSelectionWindowsBoardIndex++) {
        MentSleepFrame();
        if (createBoardSelectionWindowsBoardIndex <= 0x32) {
            createBoardSelectionWindowsInterpolationValue = MentInterpolateSineEaseIn(-500.0f, 10.0f, createBoardSelectionWindowsBoardIndex, 50.0f);
        }
        else {
            createBoardSelectionWindowsInterpolationValue = MentInterpolateCosineEaseOut(10.0f, 0.0f, createBoardSelectionWindowsBoardIndex - 0x32, 10.0f);
        }
        HuSprGrpPosSet(playerSelectionWork[0].selectionState, 0.0f, createBoardSelectionWindowsInterpolationValue);
        for (createBoardSelectionWindowsBoardIndex2 = 0; createBoardSelectionWindowsBoardIndex2 < 3; createBoardSelectionWindowsBoardIndex2++) {
            HuWinPosSet(playerSelectionWork[0].selectionHistory[createBoardSelectionWindowsBoardIndex2], 100.0f, characterSelectSpritePositions[createBoardSelectionWindowsBoardIndex2][2] - 0x15 + createBoardSelectionWindowsInterpolationValue);
        }
    }
}

void MentUpdateBoardSelectionWindows(void)
{
    float updateBoardSelectionWindowsInterpolationValue;
    s32 updateBoardSelectionWindowsBoardIndex;
    MentDllPlayerSelectionWork *updateBoardSelectionWindowsPlayerSelection;
    s32 updateBoardSelectionWindowsBoardIndex2;
    s32 updateBoardSelectionWindowsSelectionState;
    s32 updateBoardSelectionWindowsBoardIndex3;

    updateBoardSelectionWindowsSelectionState = playerSelectionWork[0].selectionMode;
    updateBoardSelectionWindowsPlayerSelection = &playerSelectionWork[1];
    HuSprAttrSet(updateBoardSelectionWindowsSelectionState, 2, 4);
    for (updateBoardSelectionWindowsBoardIndex3 = 0; updateBoardSelectionWindowsBoardIndex3 < 4; updateBoardSelectionWindowsBoardIndex3++) {
        HuSprAttrSet(updateBoardSelectionWindowsSelectionState, updateBoardSelectionWindowsBoardIndex3 + 3, 4);
    }
    HuSprAttrSet(updateBoardSelectionWindowsSelectionState, recordPlayerWork[updateBoardSelectionWindowsPlayerSelection->selectionFlags[4]].character + 0x37, 4);
    HuSprAttrSet(updateBoardSelectionWindowsSelectionState, updateBoardSelectionWindowsPlayerSelection->selectionFlags[4] + 0x3F, 4);
    HuSprAttrSet(updateBoardSelectionWindowsSelectionState, updateBoardSelectionWindowsPlayerSelection->selectionFlags[4] + 0x43, 4);
    for (updateBoardSelectionWindowsBoardIndex = 0; updateBoardSelectionWindowsBoardIndex <= 0x3C; updateBoardSelectionWindowsBoardIndex++) {
        MentSleepFrame();
        if (updateBoardSelectionWindowsBoardIndex <= 0xA) {
            updateBoardSelectionWindowsInterpolationValue = MentInterpolateSineEaseIn(0.0f, 10.0f, updateBoardSelectionWindowsBoardIndex, 10.0f);
        }
        else {
            updateBoardSelectionWindowsInterpolationValue = MentInterpolateCosineEaseOut(10.0f, -500.0f, updateBoardSelectionWindowsBoardIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(playerSelectionWork[0].selectionState, 0.0f, updateBoardSelectionWindowsInterpolationValue);
        for (updateBoardSelectionWindowsBoardIndex2 = 0; updateBoardSelectionWindowsBoardIndex2 < 3; updateBoardSelectionWindowsBoardIndex2++) {
            HuWinPosSet(playerSelectionWork->selectionWindows[updateBoardSelectionWindowsBoardIndex2], 100.0f, characterSelectSpritePositions[updateBoardSelectionWindowsBoardIndex2][2] - 0x15 + updateBoardSelectionWindowsInterpolationValue);
        }
    }
}

void MentUpdateBoardRecordDisplay(OMOBJ *object, MentDllRecordPlayerWork *recordWork)
{
    float updateBoardRecordDisplayInterpolationValue;
    float updateBoardRecordDisplayInterpolationValue2;
    s32 updateBoardRecordDisplayStateValue;
    s32 updateBoardRecordDisplayStateValue2;
    s32 updateBoardRecordDisplayStateValue3;
    s32 updateBoardRecordDisplayPlayerIndex;
    s32 updateBoardRecordDisplayStateValue4;
    s32 updateBoardRecordDisplayStateValue5;
    s32 updateBoardRecordDisplayStateValue6;
    s32 updateBoardRecordDisplayStateValue7;

    updateBoardRecordDisplayStateValue3 = playerSelectionWork[0].selectionState;
    updateBoardRecordDisplayStateValue4 = 0;
    updateBoardRecordDisplayStateValue5 = 0;
    updateBoardRecordDisplayStateValue = recordWork->character % 4;
    updateBoardRecordDisplayStateValue6 = updateBoardRecordDisplayStateValue;
    updateBoardRecordDisplayStateValue2 = recordWork->character / 4;
    updateBoardRecordDisplayStateValue7 = updateBoardRecordDisplayStateValue2;
    if (recordWork->recordFields[0] == 0) {
        if (HuPadBtnDown[recordWork->controllerPort] & PAD_BUTTON_A) {
            recordWork->recordFields[0] = 1;
            HuSprBankSet(updateBoardRecordDisplayStateValue3, recordWork->character + 9, 1);
            HuSprBankSet(updateBoardRecordDisplayStateValue3, 0x19, 1);
            HuSprBankSet(updateBoardRecordDisplayStateValue3, 0x1A, 1);
            HuAudFXPlay(recordWork->character + 0x6D);
        }
        else {
            if ((HuPadStkX[recordWork->controllerPort] >= 5) || ((HuPadBtn[recordWork->controllerPort] & 2) != 0)) {
                updateBoardRecordDisplayStateValue++;
                updateBoardRecordDisplayStateValue4 = 1;
                if (updateBoardRecordDisplayStateValue > 3) {
                    updateBoardRecordDisplayStateValue = 3;
                }
            }
            else if ((HuPadStkX[recordWork->controllerPort] <= -5) || ((HuPadBtn[recordWork->controllerPort] & 1) != 0)) {
                updateBoardRecordDisplayStateValue--;
                updateBoardRecordDisplayStateValue4 = -1;
                if (updateBoardRecordDisplayStateValue < 0) {
                    updateBoardRecordDisplayStateValue = 0;
                }
            }
            if ((HuPadStkY[recordWork->controllerPort] <= -5) || ((HuPadBtn[recordWork->controllerPort] & 4) != 0)) {
                updateBoardRecordDisplayStateValue2++;
                updateBoardRecordDisplayStateValue5 = 1;
                if (updateBoardRecordDisplayStateValue2 > 1) {
                    updateBoardRecordDisplayStateValue2 = 1;
                }
            }
            else if ((HuPadStkY[recordWork->controllerPort] >= 5) || ((HuPadBtn[recordWork->controllerPort] & 8) != 0)) {
                updateBoardRecordDisplayStateValue2--;
                updateBoardRecordDisplayStateValue5 = -1;
                if (updateBoardRecordDisplayStateValue2 < 0) {
                    updateBoardRecordDisplayStateValue2 = 0;
                }
            }
        }
    }
    else if ((HuPadBtnDown[recordWork->controllerPort] & 0x200) != 0) {
        recordWork->recordFields[0] = 0;
        HuSprBankSet(updateBoardRecordDisplayStateValue3, recordWork->character + 9, 0);
        HuSprBankSet(updateBoardRecordDisplayStateValue3, 0x19, 0);
        HuSprBankSet(updateBoardRecordDisplayStateValue3, 0x1A, 0);
        HuAudFXPlay(3);
    }
    if (recordWork->character != (updateBoardRecordDisplayStateValue + (updateBoardRecordDisplayStateValue2 * 4))) {
        recordWork->character = updateBoardRecordDisplayStateValue + (updateBoardRecordDisplayStateValue2 * 4);
        HuAudFXPlay(0);
        for (updateBoardRecordDisplayPlayerIndex = 0; updateBoardRecordDisplayPlayerIndex <= 0xA; updateBoardRecordDisplayPlayerIndex++) {
            MentSleepFrame();
            updateBoardRecordDisplayInterpolationValue = MentInterpolateSineEaseIn(characterSelectSpritePositions[updateBoardRecordDisplayStateValue6][0], characterSelectSpritePositions[updateBoardRecordDisplayStateValue][0], updateBoardRecordDisplayPlayerIndex, 10.0f);
            updateBoardRecordDisplayInterpolationValue2 = MentInterpolateSineEaseIn(characterSelectSpritePositions[updateBoardRecordDisplayStateValue7][1], characterSelectSpritePositions[updateBoardRecordDisplayStateValue2][1], updateBoardRecordDisplayPlayerIndex, 10.0f);
            HuSprPosSet(updateBoardRecordDisplayStateValue3, 0x19, updateBoardRecordDisplayInterpolationValue, updateBoardRecordDisplayInterpolationValue2);
            HuSprPosSet(updateBoardRecordDisplayStateValue3, 0x1A, updateBoardRecordDisplayInterpolationValue, updateBoardRecordDisplayInterpolationValue2);
        }
    }
}

void MentUpdateBoardPlayerSelection(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork)
{
    float updateBoardPlayerSelectionInterpolationValue;
    float updateBoardPlayerSelectionInterpolationValue2;
    float updateBoardPlayerSelectionInterpolationValue3;
    s32 updateBoardPlayerSelectionSelectionState;
    s32 updateBoardPlayerSelectionPlayerIndex;
    s32 updateBoardPlayerSelectionSelectionState2;
    s32 updateBoardPlayerSelectionSelectionState3;
    s32 updateBoardPlayerSelectionPlayerIndex2;
    s32 updateBoardPlayerSelectionPlayerIndex3;
    s32 updateBoardPlayerSelectionSelectionState4;
    s32 updateBoardPlayerSelectionSelectionState5;

    updateBoardPlayerSelectionSelectionState = selectionWork->selectionState;
    updateBoardPlayerSelectionSelectionState2 = 0;
    updateBoardPlayerSelectionSelectionState3 = selectionWork->playerSelections[0];
    playerSelectionWork[0].playerSelections[3] = 0;
    if (HuPadBtnDown[recordPlayerWork->controllerPort] & PAD_BUTTON_A) {
        playerSelectionWork[0].playerSelections[3] = 1;
        selectionWork->playerSelections[0]++;
        if (selectionWork->playerSelections[0] > 2) {
            selectionWork->playerSelections[0] = 2;
            playerSelectionWork[1].playerSelections[1] = 1;
            updateBoardPlayerSelectionSelectionState4 = playerSelectionWork[0].selectionState;
            HuSprAttrSet(updateBoardPlayerSelectionSelectionState4, 0x1B, 4);
            for (updateBoardPlayerSelectionPlayerIndex2 = 0; updateBoardPlayerSelectionPlayerIndex2 < 2; updateBoardPlayerSelectionPlayerIndex2++) {
                HuSprAttrSet(updateBoardPlayerSelectionSelectionState4, updateBoardPlayerSelectionPlayerIndex2 + 0x1C, 4);
            }
        }
        HuAudFXPlay(2);
    }
    else if (HuPadBtnDown[recordPlayerWork->controllerPort] & PAD_BUTTON_B) {
        playerSelectionWork[0].playerSelections[3] = 1;
        selectionWork->playerSelections[0]--;
        if (selectionWork->playerSelections[0] < 1) {
            selectionWork->playerSelections[0] = 1;
            playerSelectionWork[1].playerSelections[1] = -1;
            updateBoardPlayerSelectionSelectionState5 = playerSelectionWork[0].selectionState;
            HuSprAttrSet(updateBoardPlayerSelectionSelectionState5, 0x1B, 4);
            for (updateBoardPlayerSelectionPlayerIndex3 = 0; updateBoardPlayerSelectionPlayerIndex3 < 2; updateBoardPlayerSelectionPlayerIndex3++) {
                HuSprAttrSet(updateBoardPlayerSelectionSelectionState5, updateBoardPlayerSelectionPlayerIndex3 + 0x1C, 4);
            }
        }
        HuAudFXPlay(3);
    }
    else if ((HuPadStkX[recordPlayerWork->controllerPort] >= 5) || (HuPadBtn[recordPlayerWork->controllerPort] & PAD_BUTTON_RIGHT)) {
        updateBoardPlayerSelectionSelectionState2 = 1;
        playerSelectionWork[0].playerSelections[3] = 1;
        switch (selectionWork->playerSelections[0]) {
            case 1:
                if (menuLaunchParameters[5] == 1) {
                    selectionWork->selectionFlags[0] += 1;
                    if (selectionWork->selectionFlags[0] > 4) {
                        selectionWork->selectionFlags[0] = 0;
                    }
                }
                else {
                    selectionWork->selectionFlags[0] += 1;
                    if (selectionWork->selectionFlags[0] > 4) {
                        selectionWork->selectionFlags[0] = 0;
                    }
                    if (selectionWork->selectionFlags[0] == 3) {
                        selectionWork->selectionFlags[0] = 4;
                    }
                }
                break;
            case 2:
                if (menuLaunchParameters[6] == 1) {
                    selectionWork->selectionFlags[1]++;
                    if (selectionWork->selectionFlags[1] > 2) {
                        selectionWork->selectionFlags[1] = 0;
                    }
                }
                else {
                    selectionWork->selectionFlags[1]++;
                    if (selectionWork->selectionFlags[1] > 1) {
                        selectionWork->selectionFlags[1] = 0;
                    }
                }
                break;
        }
        HuAudFXPlay(0);
    }
    else if ((HuPadStkX[recordPlayerWork->controllerPort] <= -5) || ((HuPadBtn[recordPlayerWork->controllerPort] & 1) != 0)) {
        updateBoardPlayerSelectionSelectionState2 = -1;
        playerSelectionWork[0].playerSelections[3] = 1;
        switch (selectionWork->playerSelections[0]) {
            case 1:
                if (menuLaunchParameters[5] == 1) {
                    selectionWork->selectionFlags[0] -= 1;
                    if (selectionWork->selectionFlags[0] < 0) {
                        selectionWork->selectionFlags[0] = 4;
                    }
                }
                else {
                    selectionWork->selectionFlags[0] -= 1;
                    if (selectionWork->selectionFlags[0] < 0) {
                        selectionWork->selectionFlags[0] = 4;
                    }
                    if (selectionWork->selectionFlags[0] == 3) {
                        selectionWork->selectionFlags[0] = 2;
                    }
                }
                break;
            case 2:
                if (menuLaunchParameters[6] == 1) {
                    selectionWork->selectionFlags[1]--;
                    if (selectionWork->selectionFlags[1] < 0) {
                        selectionWork->selectionFlags[1] = 2;
                    }
                }
                else {
                    selectionWork->selectionFlags[1]--;
                    if (selectionWork->selectionFlags[1] < 0) {
                        selectionWork->selectionFlags[1] = 1;
                    }
                }
                break;
        }
        HuAudFXPlay(0);
    }
    if (updateBoardPlayerSelectionSelectionState3 != selectionWork->playerSelections[0]) {
        for (updateBoardPlayerSelectionPlayerIndex = 0; updateBoardPlayerSelectionPlayerIndex < 0xB; updateBoardPlayerSelectionPlayerIndex++) {
            MentSleepFrame();
            updateBoardPlayerSelectionInterpolationValue3 = MentInterpolateSineEaseIn(characterSelectSpritePositions[updateBoardPlayerSelectionSelectionState3][2], characterSelectSpritePositions[selectionWork->playerSelections[0]][2], updateBoardPlayerSelectionPlayerIndex, 10.0f);
            HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x1B, 288.0f, updateBoardPlayerSelectionInterpolationValue3);
            updateBoardPlayerSelectionInterpolationValue = MentInterpolateSineEaseIn(characterSelectSpritePositions[updateBoardPlayerSelectionSelectionState3][3], characterSelectSpritePositions[selectionWork->playerSelections[0]][3], updateBoardPlayerSelectionPlayerIndex, 10.0f);
            HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x1C, updateBoardPlayerSelectionInterpolationValue, updateBoardPlayerSelectionInterpolationValue3);
            updateBoardPlayerSelectionInterpolationValue = MentInterpolateSineEaseIn(characterSelectSpritePositions[updateBoardPlayerSelectionSelectionState3][4], characterSelectSpritePositions[selectionWork->playerSelections[0]][4], updateBoardPlayerSelectionPlayerIndex, 10.0f);
            HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x1D, updateBoardPlayerSelectionInterpolationValue, updateBoardPlayerSelectionInterpolationValue3);
        }
    }
    else if (updateBoardPlayerSelectionSelectionState2 != 0) {
        for (updateBoardPlayerSelectionPlayerIndex = 0; updateBoardPlayerSelectionPlayerIndex <= 0xA; updateBoardPlayerSelectionPlayerIndex++) {
            MentSleepFrame();
            if (updateBoardPlayerSelectionPlayerIndex <= 5) {
                updateBoardPlayerSelectionInterpolationValue = MentInterpolateSineEaseIn(0.0f, 10.0f, updateBoardPlayerSelectionPlayerIndex, 5.0f);
                updateBoardPlayerSelectionInterpolationValue2 = MentInterpolateSineEaseIn(1.0f, 0.0f, updateBoardPlayerSelectionPlayerIndex, 5.0f);
            }
            else {
                updateBoardPlayerSelectionInterpolationValue = MentInterpolateSineEaseIn(10.0f, 0.0f, updateBoardPlayerSelectionPlayerIndex - 5, 5.0f);
                updateBoardPlayerSelectionInterpolationValue2 = MentInterpolateSineEaseIn(0.0f, 1.0f, updateBoardPlayerSelectionPlayerIndex - 5, 5.0f);
            }
            if (updateBoardPlayerSelectionSelectionState2 == 1) {
                HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x1D, updateBoardPlayerSelectionInterpolationValue + characterSelectSpritePositions[selectionWork->playerSelections[0]][4], characterSelectSpritePositions[selectionWork->playerSelections[0]][2]);
            }
            else {
                HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x1C, characterSelectSpritePositions[selectionWork->playerSelections[0]][3] - updateBoardPlayerSelectionInterpolationValue, characterSelectSpritePositions[selectionWork->playerSelections[0]][2]);
            }
            switch (selectionWork->playerSelections[0]) {
                case 1:
                    if (updateBoardPlayerSelectionSelectionState2 == 1) {
                        HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x1E, 340.0f + updateBoardPlayerSelectionInterpolationValue, characterSelectSpritePositions[1][2]);
                        HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x1F, 340.0f + updateBoardPlayerSelectionInterpolationValue, characterSelectSpritePositions[1][2] + 2);
                    }
                    else {
                        HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x1E, 341.0f - updateBoardPlayerSelectionInterpolationValue, characterSelectSpritePositions[1][2]);
                        HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x1F, 341.0f - updateBoardPlayerSelectionInterpolationValue, characterSelectSpritePositions[1][2] + 2);
                    }
                    HuSprScaleSet(updateBoardPlayerSelectionSelectionState, 0x1E, 1.0f, updateBoardPlayerSelectionInterpolationValue2);
                    HuSprScaleSet(updateBoardPlayerSelectionSelectionState, 0x1F, 1.0f, updateBoardPlayerSelectionInterpolationValue2);
                    break;
                case 2:
                    if (updateBoardPlayerSelectionSelectionState2 == 1) {
                        HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x20, 340.0f + updateBoardPlayerSelectionInterpolationValue, characterSelectSpritePositions[2][2]);
                        HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x21, 340.0f + updateBoardPlayerSelectionInterpolationValue, characterSelectSpritePositions[2][2] + 2);
                    }
                    else {
                        HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x20, 341.0f - updateBoardPlayerSelectionInterpolationValue, characterSelectSpritePositions[2][2]);
                        HuSprPosSet(updateBoardPlayerSelectionSelectionState, 0x21, 341.0f - updateBoardPlayerSelectionInterpolationValue, characterSelectSpritePositions[2][2] + 2);
                    }
                    HuSprScaleSet(updateBoardPlayerSelectionSelectionState, 0x20, 1.0f, updateBoardPlayerSelectionInterpolationValue2);
                    HuSprScaleSet(updateBoardPlayerSelectionSelectionState, 0x21, 1.0f, updateBoardPlayerSelectionInterpolationValue2);
                    break;
            }
            if (updateBoardPlayerSelectionPlayerIndex == 6) {
                switch (selectionWork->playerSelections[0]) {
                    case 1:
                        HuSprBankSet(updateBoardPlayerSelectionSelectionState, 0x1E, selectionWork->selectionFlags[0]);
                        HuSprBankSet(updateBoardPlayerSelectionSelectionState, 0x1F, selectionWork->selectionFlags[0]);
                        break;
                    case 2:
                        HuSprBankSet(updateBoardPlayerSelectionSelectionState, 0x20, selectionWork->selectionFlags[1]);
                        HuSprBankSet(updateBoardPlayerSelectionSelectionState, 0x21, selectionWork->selectionFlags[1]);
                        break;
                }
            }
        }
    }
    playerSelectionWork[0].playerSelections[3] = 0;
}

void MentCreateMenuOptionSprites(void)
{
    s32 createMenuOptionSpritesStateValue;
    s32 createMenuOptionSpritesMenuItemIndex;

    createMenuOptionSpritesStateValue = playerSelectionWork[0].selectionTimer;
    for (createMenuOptionSpritesMenuItemIndex = 0; createMenuOptionSpritesMenuItemIndex < 9; createMenuOptionSpritesMenuItemIndex++) {
        HuSprAttrSet(createMenuOptionSpritesStateValue, createMenuOptionSpritesMenuItemIndex, HUSPR_ATTR_DISPOFF);
    }
    HuSprAttrReset(createMenuOptionSpritesStateValue, 0, HUSPR_ATTR_DISPOFF);
    HuSprBankSet(createMenuOptionSpritesStateValue, 0, playerSelectionWork[0].selectionFlags[0]);
    HuSprAttrReset(createMenuOptionSpritesStateValue, recordPlayerWork->character + 1, HUSPR_ATTR_DISPOFF);
    HuSprGrpPosSet(createMenuOptionSpritesStateValue, 16.0f, -500.0f);
}

void MentAnimateMenuOptionTransition(MentDllPlayerSelectionWork *selectionWork, s32 selectionIndex, s32 targetIndex, s32 animationFrames)
{
    s32 animateMenuOptionTransitionMenuItemIndex;
    s32 animateMenuOptionTransitionAnimationFrame;
    s32 animateMenuOptionTransitionSpriteGroupId;
    s32 animateMenuOptionTransitionMenuItemIndex2;
    s32 animateMenuOptionTransitionSpriteId;

    animateMenuOptionTransitionSpriteGroupId = HuSprGrpCreate(9);
    selectionWork->selectionTimer = animateMenuOptionTransitionSpriteGroupId;
    animateMenuOptionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x59)), 0x7D0, 0);
    HuSprGrpMemberSet(animateMenuOptionTransitionSpriteGroupId, 0, animateMenuOptionTransitionSpriteId);
    MentSetSpritePosition(animateMenuOptionTransitionSpriteGroupId, 0, 75.0f, 25.0f);
    for (animateMenuOptionTransitionMenuItemIndex = 0; animateMenuOptionTransitionMenuItemIndex < 8; animateMenuOptionTransitionMenuItemIndex++) {
        animateMenuOptionTransitionSpriteId = HuSprCreate(HuSprAnimReadFile(animateMenuOptionTransitionMenuItemIndex + DATA_MAKE_NUM(DATADIR_MENT, 0x31)), 0x3E8, 0);
        HuSprGrpMemberSet(animateMenuOptionTransitionSpriteGroupId, animateMenuOptionTransitionMenuItemIndex + 1, animateMenuOptionTransitionSpriteId);
        HuSprPosSet(animateMenuOptionTransitionSpriteGroupId, animateMenuOptionTransitionMenuItemIndex + 1, 24.0f, 25.0f);
    }
    animateMenuOptionTransitionAnimationFrame = playerSelectionWork[0].selectionTimer;
    for (animateMenuOptionTransitionMenuItemIndex2 = 0; animateMenuOptionTransitionMenuItemIndex2 < 9; animateMenuOptionTransitionMenuItemIndex2++) {
        HuSprAttrSet(animateMenuOptionTransitionAnimationFrame, animateMenuOptionTransitionMenuItemIndex2, 4);
    }
    HuSprAttrReset(animateMenuOptionTransitionAnimationFrame, 0, HUSPR_ATTR_DISPOFF);
    HuSprBankSet(animateMenuOptionTransitionAnimationFrame, 0, playerSelectionWork[0].selectionFlags[0]);
    HuSprAttrReset(animateMenuOptionTransitionAnimationFrame, recordPlayerWork->character + 1, HUSPR_ATTR_DISPOFF);
    HuSprGrpPosSet(animateMenuOptionTransitionAnimationFrame, 16.0f, -500.0f);
}

void MentCreateMenuOptionWindows(void)
{
    float createMenuOptionWindowsInterpolationValue;
    s32 createMenuOptionWindowsMenuItemIndex;
    s32 createMenuOptionWindowsStateValue;
    s32 createMenuOptionWindowsMenuItemIndex2;

    createMenuOptionWindowsStateValue = playerSelectionWork[0].selectionTimer;
    for (createMenuOptionWindowsMenuItemIndex2 = 0; createMenuOptionWindowsMenuItemIndex2 < 9; createMenuOptionWindowsMenuItemIndex2++) {
        HuSprAttrSet(createMenuOptionWindowsStateValue, createMenuOptionWindowsMenuItemIndex2, HUSPR_ATTR_DISPOFF);
    }
    HuSprAttrReset(createMenuOptionWindowsStateValue, 0, HUSPR_ATTR_DISPOFF);
    HuSprBankSet(createMenuOptionWindowsStateValue, 0, playerSelectionWork[0].selectionFlags[0]);
    HuSprAttrReset(createMenuOptionWindowsStateValue, recordPlayerWork->character + 1, HUSPR_ATTR_DISPOFF);
    HuSprGrpPosSet(createMenuOptionWindowsStateValue, 16.0f, -500.0f);
    for (createMenuOptionWindowsMenuItemIndex = 0; createMenuOptionWindowsMenuItemIndex <= 0x3C; createMenuOptionWindowsMenuItemIndex++) {
        MentSleepFrame();
        if (createMenuOptionWindowsMenuItemIndex <= 0x32) {
            createMenuOptionWindowsInterpolationValue = MentInterpolateSineEaseIn(-500.0f, 50.0f, createMenuOptionWindowsMenuItemIndex, 50.0f);
        }
        else {
            createMenuOptionWindowsInterpolationValue = MentInterpolateCosineEaseOut(50.0f, 40.0f, createMenuOptionWindowsMenuItemIndex - 0x32, 10.0f);
        }
        HuSprGrpPosSet(playerSelectionWork[0].selectionTimer, 16.0f, createMenuOptionWindowsInterpolationValue);
    }
}

void MentUpdateMenuOptionWindows(void)
{
    float updateMenuOptionWindowsInterpolationValue;
    s32 updateMenuOptionWindowsMenuItemIndex;

    playerSelectionWork[0].playerSelections[0] = -1;
    playerSelectionWork[0].updateCallback = NULL;
    for (updateMenuOptionWindowsMenuItemIndex = 0; updateMenuOptionWindowsMenuItemIndex <= 0x4B; updateMenuOptionWindowsMenuItemIndex++) {
        MentSleepFrame();
        if (updateMenuOptionWindowsMenuItemIndex <= 0xA) {
            updateMenuOptionWindowsInterpolationValue = MentInterpolateSineEaseIn(40.0f, 50.0f, updateMenuOptionWindowsMenuItemIndex, 10.0f);
        }
        else {
            updateMenuOptionWindowsInterpolationValue = MentInterpolateCosineEaseOut(50.0f, -500.0f, updateMenuOptionWindowsMenuItemIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(playerSelectionWork[0].selectionTimer, 16.0f, updateMenuOptionWindowsInterpolationValue);
        if (updateMenuOptionWindowsMenuItemIndex >= 0xF) {
            updateMenuOptionWindowsInterpolationValue = MentInterpolateCosineEaseOut(210.0f, -500.0f, updateMenuOptionWindowsMenuItemIndex - 0xF, 60.0f);
            HuSprGrpData[playerSelectionWork[0].selectionIndex].pos.y = (s32)MentApproachValue(HuSprGrpData[playerSelectionWork[0].selectionIndex].pos.y, updateMenuOptionWindowsInterpolationValue, 10.0f);
        }
    }
}

void MentUpdateMenuOptionHighlights(void)
{
    float updateMenuOptionHighlightsInterpolationValue;
    s32 updateMenuOptionHighlightsMenuItemIndex;

    playerSelectionWork[0].playerSelections[0] = -1;
    playerSelectionWork[0].updateCallback = NULL;
    for (updateMenuOptionHighlightsMenuItemIndex = 0; updateMenuOptionHighlightsMenuItemIndex <= 0x28; updateMenuOptionHighlightsMenuItemIndex++) {
        MentSleepFrame();
        updateMenuOptionHighlightsInterpolationValue = MentInterpolateCosineEaseOut(40.0f, -500.0f, updateMenuOptionHighlightsMenuItemIndex, 40.0f);
        HuSprGrpPosSet(playerSelectionWork[0].selectionTimer, 16.0f, updateMenuOptionHighlightsInterpolationValue);
        updateMenuOptionHighlightsInterpolationValue = MentInterpolateCosineEaseOut(210.0f, -500.0f, updateMenuOptionHighlightsMenuItemIndex, 40.0f);
        HuSprGrpData[playerSelectionWork[0].selectionIndex].pos.y = (s32)MentApproachValue(HuSprGrpData[playerSelectionWork[0].selectionIndex].pos.y, updateMenuOptionHighlightsInterpolationValue, 10.0f);
    }
}

void MentCreateMenuOptionCursor(void)
{
    s32 createMenuOptionCursorMenuValue;
    s32 createMenuOptionCursorMenuItemIndex;
    s32 createMenuOptionCursorStateValue;

    createMenuOptionCursorMenuValue = 0;
    createMenuOptionCursorStateValue = playerSelectionWork[0].playerCount;
    for (createMenuOptionCursorMenuItemIndex = 0; createMenuOptionCursorMenuItemIndex < 0x13; createMenuOptionCursorMenuItemIndex++) {
        HuSprAttrSet(createMenuOptionCursorStateValue, createMenuOptionCursorMenuItemIndex, 4);
    }
    for (createMenuOptionCursorMenuItemIndex = 0; createMenuOptionCursorMenuItemIndex < 4; createMenuOptionCursorMenuItemIndex++) {
        HuSprBankSet(createMenuOptionCursorStateValue, createMenuOptionCursorMenuItemIndex, 0);
        recordPlayerWork[createMenuOptionCursorMenuItemIndex].recordFields[3] = recordPlayerWork[createMenuOptionCursorMenuItemIndex].isComputerPlayer = 0;
        HuSprAttrReset(createMenuOptionCursorStateValue, createMenuOptionCursorMenuItemIndex, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(createMenuOptionCursorStateValue, createMenuOptionCursorMenuItemIndex + 4, HUSPR_ATTR_DISPOFF);
    }
    HuSprAttrReset(createMenuOptionCursorStateValue, 0xA, HUSPR_ATTR_DISPOFF);
    for (createMenuOptionCursorMenuItemIndex = 0; createMenuOptionCursorMenuItemIndex < 4; createMenuOptionCursorMenuItemIndex++) {
        HuSprAttrReset(createMenuOptionCursorStateValue, createMenuOptionCursorMenuItemIndex + 0xB, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(createMenuOptionCursorStateValue, createMenuOptionCursorMenuItemIndex + 0xF, HUSPR_ATTR_DISPOFF);
    }
    HuSprGrpPosSet(createMenuOptionCursorStateValue, 0.0f, -500.0f);
}

void MentUpdateMenuPlayerSelection(OMOBJ *object, MentDllPlayerSelectionWork *selectionWork)
{
    s32 updateMenuPlayerSelectionSelectionValue;
    float updateMenuPlayerSelectionInterpolationValue;
    float updateMenuPlayerSelectionInterpolationValue2;
    s32 updateMenuPlayerSelectionPlayerIndex;
    s32 updateMenuPlayerSelectionPlayerIndex2;
    s32 updateMenuPlayerSelectionSelectionState;
    s32 updateMenuPlayerSelectionSelectionState2;
    s32 updateMenuPlayerSelectionSelectionState3;
    s32 updateMenuPlayerSelectionSelectionState4;
    s32 updateMenuPlayerSelectionSelectionState5;

    updateMenuPlayerSelectionSelectionValue = 0;
    updateMenuPlayerSelectionSelectionState2 = playerSelectionWork[0].playerCount;
    updateMenuPlayerSelectionSelectionState = updateMenuPlayerSelectionSelectionState3 = updateMenuPlayerSelectionSelectionState4 = updateMenuPlayerSelectionSelectionState5 = 0;
    playerSelectionWork[0].selectionFlags[0] = 0;
    updateMenuPlayerSelectionSelectionState = 1;
    for (updateMenuPlayerSelectionPlayerIndex2 = 1; updateMenuPlayerSelectionPlayerIndex2 < 4; updateMenuPlayerSelectionPlayerIndex2++) {
        if (recordPlayerWork[updateMenuPlayerSelectionPlayerIndex2].isComputerPlayer == 0) {
            updateMenuPlayerSelectionSelectionState++;
        }
    }
    updateMenuPlayerSelectionSelectionState3 = 1;
    for (updateMenuPlayerSelectionPlayerIndex2 = 1; updateMenuPlayerSelectionPlayerIndex2 < 4; updateMenuPlayerSelectionPlayerIndex2++) {
        if (HuPadStatGet(updateMenuPlayerSelectionPlayerIndex2) != -1) {
            updateMenuPlayerSelectionSelectionState3++;
        }
    }
    if (updateMenuPlayerSelectionSelectionState3 >= 2) {
        MentShowMinigamePlayerCountIndicators();
        if ((HuPadStkX[recordPlayerWork->controllerPort] >= 5) || ((HuPadBtn[recordPlayerWork->controllerPort] & 2) != 0)) {
            updateMenuPlayerSelectionSelectionState++;
            updateMenuPlayerSelectionSelectionState4 = 1;
        }
        else if ((HuPadStkX[recordPlayerWork->controllerPort] <= -5) || ((HuPadBtn[recordPlayerWork->controllerPort] & 1) != 0)) {
            updateMenuPlayerSelectionSelectionState--;
            updateMenuPlayerSelectionSelectionState4 = -1;
        }
    }
    else {
        MentHideMinigamePlayerCountIndicators();
    }
    if (updateMenuPlayerSelectionSelectionState < 1) {
        updateMenuPlayerSelectionSelectionState = updateMenuPlayerSelectionSelectionState3;
    }
    else if (updateMenuPlayerSelectionSelectionState > updateMenuPlayerSelectionSelectionState3) {
        if (updateMenuPlayerSelectionSelectionState4 == 0) {
            updateMenuPlayerSelectionSelectionState = updateMenuPlayerSelectionSelectionState3;
            updateMenuPlayerSelectionSelectionState5 = 1;
        }
        else {
            updateMenuPlayerSelectionSelectionState = 1;
        }
    }
    if ((updateMenuPlayerSelectionSelectionState4 != 0) || (updateMenuPlayerSelectionSelectionState5 != 0)) {
        playerSelectionWork[0].selectionFlags[0] = 1;
        HuAudFXPlay(0);
        for (updateMenuPlayerSelectionPlayerIndex2 = 0; updateMenuPlayerSelectionPlayerIndex2 <= 0xA; updateMenuPlayerSelectionPlayerIndex2++) {
            if (updateMenuPlayerSelectionPlayerIndex2 <= 5) {
                updateMenuPlayerSelectionInterpolationValue2 = MentInterpolateSineEaseIn(0.0f, 10.0f, updateMenuPlayerSelectionPlayerIndex2, 5.0f);
                updateMenuPlayerSelectionInterpolationValue = MentInterpolateSineEaseIn(1.0f, 0.0f, updateMenuPlayerSelectionPlayerIndex2, 5.0f);
            }
            else {
                updateMenuPlayerSelectionInterpolationValue2 = MentInterpolateSineEaseIn(10.0f, 0.0f, updateMenuPlayerSelectionPlayerIndex2 - 5, 5.0f);
                updateMenuPlayerSelectionInterpolationValue = MentInterpolateSineEaseIn(0.0f, 1.0f, updateMenuPlayerSelectionPlayerIndex2 - 5, 5.0f);
            }
            if (updateMenuPlayerSelectionSelectionState4 == 1) {
                HuSprPosSet(updateMenuPlayerSelectionSelectionState2, 9, (minigameRecordSpritePositions[3][0] + 0x32) + updateMenuPlayerSelectionInterpolationValue2, minigameRecordSpritePositions[0][1]);
            }
            else if (updateMenuPlayerSelectionSelectionState4 == -1) {
                HuSprPosSet(updateMenuPlayerSelectionSelectionState2, 8, (minigameRecordSpritePositions[0][0] - 0x32) - updateMenuPlayerSelectionInterpolationValue2, minigameRecordSpritePositions[0][1]);
            }
            for (updateMenuPlayerSelectionPlayerIndex = 0; updateMenuPlayerSelectionPlayerIndex < 4; updateMenuPlayerSelectionPlayerIndex++) {
                if ((updateMenuPlayerSelectionPlayerIndex < updateMenuPlayerSelectionSelectionState) && (recordPlayerWork[updateMenuPlayerSelectionPlayerIndex].isComputerPlayer == 1)) {
                    recordPlayerWork[updateMenuPlayerSelectionPlayerIndex].recordFields[3] = 0;
                    HuSprScaleSet(updateMenuPlayerSelectionSelectionState2, updateMenuPlayerSelectionPlayerIndex, updateMenuPlayerSelectionInterpolationValue, 1.0f);
                    HuSprScaleSet(updateMenuPlayerSelectionSelectionState2, updateMenuPlayerSelectionPlayerIndex + 4, updateMenuPlayerSelectionInterpolationValue, 1.0f);
                    if (updateMenuPlayerSelectionPlayerIndex2 == 5) {
                        HuSprBankSet(updateMenuPlayerSelectionSelectionState2, updateMenuPlayerSelectionPlayerIndex, 0);
                    }
                }
                else if ((updateMenuPlayerSelectionPlayerIndex >= updateMenuPlayerSelectionSelectionState) && (recordPlayerWork[updateMenuPlayerSelectionPlayerIndex].isComputerPlayer == 0)) {
                    recordPlayerWork[updateMenuPlayerSelectionPlayerIndex].recordFields[3] = 1;
                    HuSprScaleSet(updateMenuPlayerSelectionSelectionState2, updateMenuPlayerSelectionPlayerIndex, updateMenuPlayerSelectionInterpolationValue, 1.0f);
                    HuSprScaleSet(updateMenuPlayerSelectionSelectionState2, updateMenuPlayerSelectionPlayerIndex + 4, updateMenuPlayerSelectionInterpolationValue, 1.0f);
                    if (updateMenuPlayerSelectionPlayerIndex2 == 5) {
                        HuSprBankSet(updateMenuPlayerSelectionSelectionState2, updateMenuPlayerSelectionPlayerIndex, 1);
                    }
                }
            }
            MentSleepFrame();
        }
        for (updateMenuPlayerSelectionPlayerIndex = 0; updateMenuPlayerSelectionPlayerIndex < 4; updateMenuPlayerSelectionPlayerIndex++) {
            if (updateMenuPlayerSelectionPlayerIndex < updateMenuPlayerSelectionSelectionState) {
                recordPlayerWork[updateMenuPlayerSelectionPlayerIndex].isComputerPlayer = 0;
            }
            else {
                recordPlayerWork[updateMenuPlayerSelectionPlayerIndex].isComputerPlayer = 1;
            }
        }
    }
    playerSelectionWork[0].selectionFlags[0] = 0;
}

void MentSetMenuCursorPosition(s32 cursorIndex)
{
    s32 setMenuCursorPositionMenuValue;
    float setMenuCursorPositionInterpolationValue;
    s32 setMenuCursorPositionMenuItemIndex;
    s32 setMenuCursorPositionStateValue;
    s32 setMenuCursorPositionMenuItemIndex2;
    s32 setMenuCursorPositionStateValue2;

    setMenuCursorPositionStateValue2 = playerSelectionWork[0].playerCount;
    if (cursorIndex == 0) {
        setMenuCursorPositionMenuValue = 0;
        setMenuCursorPositionStateValue = playerSelectionWork[0].playerCount;
        for (setMenuCursorPositionMenuItemIndex = 0; setMenuCursorPositionMenuItemIndex < 0x13; setMenuCursorPositionMenuItemIndex++) {
            HuSprAttrSet(setMenuCursorPositionStateValue, setMenuCursorPositionMenuItemIndex, HUSPR_ATTR_DISPOFF);
        }
        for (setMenuCursorPositionMenuItemIndex = 0; setMenuCursorPositionMenuItemIndex < 4; setMenuCursorPositionMenuItemIndex++) {
            HuSprBankSet(setMenuCursorPositionStateValue, setMenuCursorPositionMenuItemIndex, 0);
            recordPlayerWork[setMenuCursorPositionMenuItemIndex].recordFields[3] = recordPlayerWork[setMenuCursorPositionMenuItemIndex].isComputerPlayer = 0;
            HuSprAttrReset(setMenuCursorPositionStateValue, setMenuCursorPositionMenuItemIndex, HUSPR_ATTR_DISPOFF);
            HuSprAttrReset(setMenuCursorPositionStateValue, setMenuCursorPositionMenuItemIndex + 4, HUSPR_ATTR_DISPOFF);
        }
        HuSprAttrReset(setMenuCursorPositionStateValue, 0xA, HUSPR_ATTR_DISPOFF);
        for (setMenuCursorPositionMenuItemIndex = 0; setMenuCursorPositionMenuItemIndex < 4; setMenuCursorPositionMenuItemIndex++) {
            HuSprAttrReset(setMenuCursorPositionStateValue, setMenuCursorPositionMenuItemIndex + 0xB, HUSPR_ATTR_DISPOFF);
            HuSprAttrReset(setMenuCursorPositionStateValue, setMenuCursorPositionMenuItemIndex + 0xF, HUSPR_ATTR_DISPOFF);
        }
        HuSprGrpPosSet(setMenuCursorPositionStateValue, 0.0f, -500.0f);
    }
    for (setMenuCursorPositionMenuItemIndex2 = 0; setMenuCursorPositionMenuItemIndex2 <= 0x3C; setMenuCursorPositionMenuItemIndex2++) {
        MentSleepFrame();
        if (setMenuCursorPositionMenuItemIndex2 <= 0x32) {
            setMenuCursorPositionInterpolationValue = MentInterpolateSineEaseIn(-500.0f, 10.0f, setMenuCursorPositionMenuItemIndex2, 50.0f);
        }
        else {
            setMenuCursorPositionInterpolationValue = MentInterpolateCosineEaseOut(10.0f, 2.06f, setMenuCursorPositionMenuItemIndex2 - 0x32, 10.0f);
        }
        HuSprGrpPosSet(setMenuCursorPositionStateValue2, 0.0f, setMenuCursorPositionInterpolationValue);
    }
}

void MentUpdateMenuCursor(void)
{
    s32 updateMenuCursorMenuValue;
    s32 updateMenuCursorMenuValue2;
    float updateMenuCursorInterpolationValue;
    s32 updateMenuCursorMenuItemIndex;
    s32 updateMenuCursorStateValue;

    updateMenuCursorMenuValue = 0;
    updateMenuCursorMenuValue2 = 0;
    updateMenuCursorStateValue = playerSelectionWork[0].playerCount;
    for (updateMenuCursorMenuItemIndex = 0; updateMenuCursorMenuItemIndex <= 0x3C; updateMenuCursorMenuItemIndex++) {
        MentSleepFrame();
        if (updateMenuCursorMenuItemIndex <= 0xA) {
            updateMenuCursorInterpolationValue = MentInterpolateSineEaseIn(2.06f, 10.0f, updateMenuCursorMenuItemIndex, 10.0f);
        }
        else {
            updateMenuCursorInterpolationValue = MentInterpolateCosineEaseOut(10.0f, -500.0f, updateMenuCursorMenuItemIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(updateMenuCursorStateValue, 0.0f, updateMenuCursorInterpolationValue);
    }
}

void MentShowMinigamePlayerCountIndicators(void)
{
    s32 showMinigamePlayerCountIndicatorsStateValue;

    showMinigamePlayerCountIndicatorsStateValue = playerSelectionWork[0].playerCount;
    HuSprAttrReset(showMinigamePlayerCountIndicatorsStateValue, 8, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(showMinigamePlayerCountIndicatorsStateValue, 8, minigameRecordSpritePositions[0][0] - 0x32, minigameRecordSpritePositions[0][1]);
    HuSprAttrReset(showMinigamePlayerCountIndicatorsStateValue, 9, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(showMinigamePlayerCountIndicatorsStateValue, 9, minigameRecordSpritePositions[3][0] + 0x32, minigameRecordSpritePositions[0][1]);
}

void MentHideMinigamePlayerCountIndicators(void)
{
    s32 hideMinigamePlayerCountIndicatorsStateValue;

    hideMinigamePlayerCountIndicatorsStateValue = playerSelectionWork[0].playerCount;
    HuSprAttrSet(hideMinigamePlayerCountIndicatorsStateValue, 8, 4);
    HuSprPosSet(hideMinigamePlayerCountIndicatorsStateValue, 8, minigameRecordSpritePositions[0][0] - 0x32, minigameRecordSpritePositions[0][1]);
    HuSprAttrSet(hideMinigamePlayerCountIndicatorsStateValue, 9, 4);
    HuSprPosSet(hideMinigamePlayerCountIndicatorsStateValue, 9, minigameRecordSpritePositions[3][0] + 0x32, minigameRecordSpritePositions[0][1]);
}

void MentAnimatePlayerCountSelection(MentDllPlayerSelectionWork *selectionWork, s32 currentPlayerCount, s32 targetPlayerCount, s32 animationFrames)
{
    s32 animatePlayerCountSelectionSelectionValue;
    s32 animatePlayerCountSelectionPlayerIndex;
    s32 animatePlayerCountSelectionPlayerIndex2;
    s32 animatePlayerCountSelectionSpriteGroupId;
    s32 animatePlayerCountSelectionSpriteId;
    s32 animatePlayerCountSelectionSelectionState;
    ANIMDATA *animatePlayerCountSelectionSpriteAnimation1;
    ANIMDATA *animatePlayerCountSelectionSpriteAnimation2;

    animatePlayerCountSelectionSpriteGroupId = HuSprGrpCreate(0x13);
    selectionWork->playerCount = animatePlayerCountSelectionSpriteGroupId;
    animatePlayerCountSelectionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x22));
    for (animatePlayerCountSelectionPlayerIndex = 0; animatePlayerCountSelectionPlayerIndex < 4; animatePlayerCountSelectionPlayerIndex++) {
        animatePlayerCountSelectionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x16)), 0x3E8, 0);
        HuSprGrpMemberSet(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex, animatePlayerCountSelectionSpriteId);
        HuSprPosSet(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex, minigameRecordSpritePositions[animatePlayerCountSelectionPlayerIndex % 4][0], minigameRecordSpritePositions[0][1]);
        animatePlayerCountSelectionSpriteId = HuSprCreate(animatePlayerCountSelectionSpriteAnimation1, 0x3F2, 0);
        HuSprGrpMemberSet(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex + 4, animatePlayerCountSelectionSpriteId);
        HuSprTPLvlSet(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex + 4, 0.5f);
        HuSprPosSet(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex + 4, minigameRecordSpritePositions[animatePlayerCountSelectionPlayerIndex % 4][0] + 1, minigameRecordSpritePositions[0][1] + 2);
    }
    animatePlayerCountSelectionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x42));
    for (animatePlayerCountSelectionPlayerIndex = 0; animatePlayerCountSelectionPlayerIndex < 2; animatePlayerCountSelectionPlayerIndex++) {
        animatePlayerCountSelectionSpriteId = HuSprCreate(animatePlayerCountSelectionSpriteAnimation1, 0, 0);
        HuSprGrpMemberSet(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex + 8, animatePlayerCountSelectionSpriteId);
        HuSprBankSet(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex + 8, animatePlayerCountSelectionPlayerIndex);
    }
    animatePlayerCountSelectionSpriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x15)), 0x7D0, 0);
    HuSprGrpMemberSet(animatePlayerCountSelectionSpriteGroupId, 0xA, animatePlayerCountSelectionSpriteId);
    HuSprTPLvlSet(animatePlayerCountSelectionSpriteGroupId, 0xA, 0.8f);
    MentSetSpritePosition(animatePlayerCountSelectionSpriteGroupId, 0xA, 288.0f, 240.0f);
    animatePlayerCountSelectionSpriteAnimation1 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x18));
    animatePlayerCountSelectionSpriteAnimation2 = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MENT, 0x19));
    for (animatePlayerCountSelectionPlayerIndex = 0; animatePlayerCountSelectionPlayerIndex < 4; animatePlayerCountSelectionPlayerIndex++) {
        animatePlayerCountSelectionSpriteId = HuSprCreate(animatePlayerCountSelectionSpriteAnimation1, 0x7BC, 0);
        HuSprGrpMemberSet(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex + 0xB, animatePlayerCountSelectionSpriteId);
        HuSprBankSet(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex + 0xB, animatePlayerCountSelectionPlayerIndex);
        MentSetSpritePosition(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex + 0xB, minigameRecordSpritePositions[animatePlayerCountSelectionPlayerIndex][2], minigameRecordSpritePositions[animatePlayerCountSelectionPlayerIndex][3]);
        animatePlayerCountSelectionSpriteId = HuSprCreate(animatePlayerCountSelectionSpriteAnimation2, 0x7C6, 0);
        HuSprGrpMemberSet(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex + 0xF, animatePlayerCountSelectionSpriteId);
        HuSprBankSet(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex + 0xF, animatePlayerCountSelectionPlayerIndex);
        HuSprTPLvlSet(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex + 0xF, 0.5f);
        MentSetSpritePosition(animatePlayerCountSelectionSpriteGroupId, animatePlayerCountSelectionPlayerIndex + 0xF, minigameRecordSpritePositions[animatePlayerCountSelectionPlayerIndex][2] + 1, minigameRecordSpritePositions[animatePlayerCountSelectionPlayerIndex][3] + 6);
    }
    animatePlayerCountSelectionSelectionValue = 0;
    animatePlayerCountSelectionSelectionState = playerSelectionWork[0].playerCount;
    for (animatePlayerCountSelectionPlayerIndex2 = 0; animatePlayerCountSelectionPlayerIndex2 < 0x13; animatePlayerCountSelectionPlayerIndex2++) {
        HuSprAttrSet(animatePlayerCountSelectionSelectionState, animatePlayerCountSelectionPlayerIndex2, HUSPR_ATTR_DISPOFF);
    }
    for (animatePlayerCountSelectionPlayerIndex2 = 0; animatePlayerCountSelectionPlayerIndex2 < 4; animatePlayerCountSelectionPlayerIndex2++) {
        HuSprBankSet(animatePlayerCountSelectionSelectionState, animatePlayerCountSelectionPlayerIndex2, 0);
        recordPlayerWork[animatePlayerCountSelectionPlayerIndex2].recordFields[3] = recordPlayerWork[animatePlayerCountSelectionPlayerIndex2].isComputerPlayer = 0;
        HuSprAttrReset(animatePlayerCountSelectionSelectionState, animatePlayerCountSelectionPlayerIndex2, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(animatePlayerCountSelectionSelectionState, animatePlayerCountSelectionPlayerIndex2 + 4, HUSPR_ATTR_DISPOFF);
    }
    HuSprAttrReset(animatePlayerCountSelectionSelectionState, 0xA, HUSPR_ATTR_DISPOFF);
    for (animatePlayerCountSelectionPlayerIndex2 = 0; animatePlayerCountSelectionPlayerIndex2 < 4; animatePlayerCountSelectionPlayerIndex2++) {
        HuSprAttrReset(animatePlayerCountSelectionSelectionState, animatePlayerCountSelectionPlayerIndex2 + 0xB, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(animatePlayerCountSelectionSelectionState, animatePlayerCountSelectionPlayerIndex2 + 0xF, HUSPR_ATTR_DISPOFF);
    }
    HuSprGrpPosSet(animatePlayerCountSelectionSelectionState, 0.0f, -500.0f);
}

void MentUpdateParticleEffect(OMOBJ *object)
{
    float updateParticleEffectInterpolationValue;
    s32 updateParticleEffectParticleIndex;
    MentDllCameraWork *updateParticleEffectCamera;

    updateParticleEffectCamera = &menuCameraWork;
    for (updateParticleEffectParticleIndex = 1; updateParticleEffectParticleIndex < 0xA; updateParticleEffectParticleIndex++) {
        if ((updateParticleEffectCamera->zoom <= 1200.0f) || (animatedModelWork[updateParticleEffectParticleIndex].startY == -200.0f)) {
            if (animatedModelWork[updateParticleEffectParticleIndex].startY >= -100.0f) {
                animatedModelWork[updateParticleEffectParticleIndex].animationTimer = 0;
                animatedModelWork[updateParticleEffectParticleIndex].startY = -200.0f;
                animatedModelWork[updateParticleEffectParticleIndex].velocityX = -99.0f;
                animatedModelWork[updateParticleEffectParticleIndex].animationDuration = rand8() + 0xB4;
            }
            updateParticleEffectInterpolationValue = MentInterpolateCosineEaseOut(animatedModelWork[updateParticleEffectParticleIndex].positionY, 1000.0f, animatedModelWork[updateParticleEffectParticleIndex].animationTimer++, animatedModelWork[updateParticleEffectParticleIndex].animationDuration);
            Hu3DData[object->mdlId[updateParticleEffectParticleIndex]].pos.y = MentApproachValue(Hu3DData[object->mdlId[updateParticleEffectParticleIndex]].pos.y, updateParticleEffectInterpolationValue, 30.0f);
            Hu3DData[object->mdlId[updateParticleEffectParticleIndex]].pos.x
                = MentApproachValue(Hu3DData[object->mdlId[updateParticleEffectParticleIndex]].pos.x, animatedModelWork[updateParticleEffectParticleIndex].startX + Hu3DData[object->mdlId[updateParticleEffectParticleIndex]].pos.x, 5.0f);
        }
    }
    for (updateParticleEffectParticleIndex = 0xA; updateParticleEffectParticleIndex < 0x8C; updateParticleEffectParticleIndex++) {
        Hu3DData[object->mdlId[updateParticleEffectParticleIndex]].pos.y = MentInterpolateSineWave(
            animatedModelWork[updateParticleEffectParticleIndex].positionY, animatedModelWork[updateParticleEffectParticleIndex].startY, animatedModelWork[updateParticleEffectParticleIndex].animationTimer++, animatedModelWork[updateParticleEffectParticleIndex].animationDuration);
        if (animatedModelWork[updateParticleEffectParticleIndex].animationTimer >= animatedModelWork[updateParticleEffectParticleIndex].animationDuration) {
            animatedModelWork[updateParticleEffectParticleIndex].animationTimer = 0;
        }
    }
    for (updateParticleEffectParticleIndex = 0xA; updateParticleEffectParticleIndex < 0x8C; updateParticleEffectParticleIndex++) {
        if ((animatedModelWork[updateParticleEffectParticleIndex].velocityX == 0.0f) && (animatedModelWork[updateParticleEffectParticleIndex].positionZ > (updateParticleEffectCamera->zoom - 400.0f))
            && (animatedModelWork[updateParticleEffectParticleIndex].positionX < (200.0f + updateParticleEffectCamera->center.x)) && (animatedModelWork[updateParticleEffectParticleIndex].positionX > (updateParticleEffectCamera->center.x - 200.0f))) {
            if (updateParticleEffectCamera->center.x >= animatedModelWork[updateParticleEffectParticleIndex].positionX) {
                animatedModelWork[updateParticleEffectParticleIndex].velocityX = -300.0f;
            }
            else {
                animatedModelWork[updateParticleEffectParticleIndex].velocityX = 300.0f;
            }
        }
        if (animatedModelWork[updateParticleEffectParticleIndex].velocityX != -99.0f) {
            Hu3DData[object->mdlId[updateParticleEffectParticleIndex]].pos.x
                = MentApproachValue(Hu3DData[object->mdlId[updateParticleEffectParticleIndex]].pos.x, animatedModelWork[updateParticleEffectParticleIndex].positionX + animatedModelWork[updateParticleEffectParticleIndex].velocityX, 30.0f);
        }
    }
    if (object->work[0] == 1) {
        for (updateParticleEffectParticleIndex = 0xA; updateParticleEffectParticleIndex < 0x8C; updateParticleEffectParticleIndex++) {
            Hu3DModelAttrSet(object->mdlId[updateParticleEffectParticleIndex], HU3D_ATTR_DISPOFF);
        }
    }
}

void MentCreateParticleEffectObject(OMOBJ *object)
{
    float createParticleEffectObjectInterpolationValue;
    float createParticleEffectObjectInterpolationValue2;
    float createParticleEffectObjectInterpolationValue3;
    s32 createParticleEffectObjectParticleIndex;
    s32 createParticleEffectObjectParticleIndex2;
    s32 createParticleEffectObjectStateValue;

    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x08));
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x09));
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x0A));
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 0x0B));
    Hu3DModelLayerSet(object->mdlId[1], 3);
    Hu3DModelLayerSet(object->mdlId[2], 3);
    Hu3DModelLayerSet(object->mdlId[3], 3);
    Hu3DModelLayerSet(object->mdlId[4], 3);
    for (createParticleEffectObjectParticleIndex = 5; createParticleEffectObjectParticleIndex < 0x8C; createParticleEffectObjectParticleIndex++) {
        object->mdlId[createParticleEffectObjectParticleIndex] = Hu3DModelLink(object->mdlId[rand8() % 4 + 1]);
    }
    for (createParticleEffectObjectParticleIndex = 1; createParticleEffectObjectParticleIndex < 0xA; createParticleEffectObjectParticleIndex++) {
        createParticleEffectObjectStateValue = 0;
        do {
            Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.x = createParticleEffectObjectInterpolationValue = 50.0f + rand8();
            if ((createParticleEffectObjectParticleIndex % 2) != 0) {
                animatedModelWork[createParticleEffectObjectParticleIndex].positionX = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.x *= -1.0f;
            }
            Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.y = createParticleEffectObjectInterpolationValue2 = 50.0f + rand8();
            Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.z = createParticleEffectObjectInterpolationValue3 = 750.0f + (createParticleEffectObjectParticleIndex * 5);
            for (createParticleEffectObjectParticleIndex2 = 1; createParticleEffectObjectParticleIndex2 < 0xA; createParticleEffectObjectParticleIndex2++) {
                if ((createParticleEffectObjectParticleIndex != createParticleEffectObjectParticleIndex2) && (animatedModelWork[createParticleEffectObjectParticleIndex2].isActive == 1)) {
                    createParticleEffectObjectInterpolationValue = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.x - Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex2]].pos.x;
                    createParticleEffectObjectInterpolationValue2 = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.y - Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex2]].pos.y;
                    createParticleEffectObjectInterpolationValue3 = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.z - Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex2]].pos.z;
                    if (sqrtf((createParticleEffectObjectInterpolationValue * createParticleEffectObjectInterpolationValue) + (createParticleEffectObjectInterpolationValue2 * createParticleEffectObjectInterpolationValue2)) <= 100.0f) {
                        break;
                    }
                }
            }
            createParticleEffectObjectStateValue++;
        } while (createParticleEffectObjectParticleIndex2 != 0xA && createParticleEffectObjectStateValue < 0x32);
        animatedModelWork[createParticleEffectObjectParticleIndex].isActive = 1;
        animatedModelWork[createParticleEffectObjectParticleIndex].positionX = animatedModelWork[createParticleEffectObjectParticleIndex].startX = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.x;
        animatedModelWork[createParticleEffectObjectParticleIndex].positionY = animatedModelWork[createParticleEffectObjectParticleIndex].startY = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.y;
        animatedModelWork[createParticleEffectObjectParticleIndex].positionZ = animatedModelWork[createParticleEffectObjectParticleIndex].startZ = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.z;
        animatedModelWork[createParticleEffectObjectParticleIndex].startX = 0.025f * animatedModelWork[createParticleEffectObjectParticleIndex].positionX;
        Hu3DModelScaleSet(object->mdlId[createParticleEffectObjectParticleIndex], 0.5f, 0.5f, 0.5f);
        Hu3DModelLayerSet(object->mdlId[createParticleEffectObjectParticleIndex], 3);
    }
    for (createParticleEffectObjectParticleIndex = 0xA; createParticleEffectObjectParticleIndex < 0x8C; createParticleEffectObjectParticleIndex++) {
        createParticleEffectObjectStateValue = 0;
        do {
            Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.x = createParticleEffectObjectInterpolationValue = rand8() * 2;
            if ((createParticleEffectObjectParticleIndex % 2) != 0) {
                Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.x *= -1.0f;
            }

            Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.y = createParticleEffectObjectInterpolationValue2 = (rand8() * 2) + 0x7D;
            Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.z = createParticleEffectObjectInterpolationValue3 = 800.0f + (createParticleEffectObjectParticleIndex * 0xA);
            for (createParticleEffectObjectParticleIndex2 = 0xA; createParticleEffectObjectParticleIndex2 < 0x8C; createParticleEffectObjectParticleIndex2++) {
                if ((createParticleEffectObjectParticleIndex != createParticleEffectObjectParticleIndex2) && (animatedModelWork[createParticleEffectObjectParticleIndex2].isActive == 1)) {
                    createParticleEffectObjectInterpolationValue = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.x - Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex2]].pos.x;
                    createParticleEffectObjectInterpolationValue2 = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.y - Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex2]].pos.y;
                    createParticleEffectObjectInterpolationValue3 = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.z - Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex2]].pos.z;
                    if (sqrtf((createParticleEffectObjectInterpolationValue3 * createParticleEffectObjectInterpolationValue3) + ((createParticleEffectObjectInterpolationValue * createParticleEffectObjectInterpolationValue) + (createParticleEffectObjectInterpolationValue2 * createParticleEffectObjectInterpolationValue2))) <= 100.0f) {
                        break;
                    }
                }
            }
            createParticleEffectObjectStateValue++;
        } while (createParticleEffectObjectParticleIndex2 != 0x8C && createParticleEffectObjectStateValue < 0x32);

        animatedModelWork[createParticleEffectObjectParticleIndex].isActive = 1;
        Hu3DModelLayerSet(object->mdlId[createParticleEffectObjectParticleIndex], 3);
        animatedModelWork[createParticleEffectObjectParticleIndex].positionX = animatedModelWork[createParticleEffectObjectParticleIndex].startX = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.x;
        animatedModelWork[createParticleEffectObjectParticleIndex].positionY = animatedModelWork[createParticleEffectObjectParticleIndex].startY = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.y;
        animatedModelWork[createParticleEffectObjectParticleIndex].positionZ = animatedModelWork[createParticleEffectObjectParticleIndex].startZ = Hu3DData[object->mdlId[createParticleEffectObjectParticleIndex]].pos.z;
        if ((rand8() % 2) == 0) {
            animatedModelWork[createParticleEffectObjectParticleIndex].startX += rand8() % 30;
        }
        else {
            animatedModelWork[createParticleEffectObjectParticleIndex].startX -= rand8() % 30;
        }
        animatedModelWork[createParticleEffectObjectParticleIndex].startY += (rand8() % 30) + 0xA;
        if ((rand8() % 2) == 0) {
            animatedModelWork[createParticleEffectObjectParticleIndex].startZ += rand8() % 30;
        }
        else {
            animatedModelWork[createParticleEffectObjectParticleIndex].startZ -= rand8() % 30;
        }
        animatedModelWork[createParticleEffectObjectParticleIndex].velocityX = animatedModelWork[createParticleEffectObjectParticleIndex].velocityY = animatedModelWork[createParticleEffectObjectParticleIndex].velocityZ = 0.0f;
        animatedModelWork[createParticleEffectObjectParticleIndex].animationDuration = ((rand8() * 2) % 360) + 0x168;
        animatedModelWork[createParticleEffectObjectParticleIndex].animationTimer = rand8();
    }
    object->objFunc = MentUpdateParticleEffect;
}

void MentUpdateMenuParticles(OMOBJ *object)
{
    s32 updateMenuParticlesMenuItemIndex;

    if (object->work[0] == 0) {
        return;
    }
    for (updateMenuParticlesMenuItemIndex = 0; updateMenuParticlesMenuItemIndex < 0xC8; updateMenuParticlesMenuItemIndex++) {
        switch (particleEffectWork[updateMenuParticlesMenuItemIndex].particleState) {
            case 0:
                Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].pos.x = rand8() + rand8() % 50;
                if ((updateMenuParticlesMenuItemIndex % 2) == 0) {
                    Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].pos.x *= -1.0f;
                }
                Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].pos.y = 500.0f + rand8() + rand8();
                Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].pos.z = 700.0f + rand8() % 128;
                particleEffectWork[updateMenuParticlesMenuItemIndex].particleAlpha = 1.0f;
                particleEffectWork[updateMenuParticlesMenuItemIndex].particleLifetime = (rand8() % 30) + 0x1E;
                particleEffectWork[updateMenuParticlesMenuItemIndex].horizontalVelocity = 0.01f * (25.0f + (rand8() % 50));
                if ((rand8() % 2) == 0) {
                    particleEffectWork[updateMenuParticlesMenuItemIndex].horizontalVelocity *= -1.0f;
                }
                particleEffectWork[updateMenuParticlesMenuItemIndex].verticalVelocity = rand8() % 2;
                particleEffectWork[updateMenuParticlesMenuItemIndex].depthVelocity = 0.01f * ((rand8() % 100) - 0x32);
                particleEffectWork[updateMenuParticlesMenuItemIndex].rotationSpeed = (rand8() % 5) + 5;
                if ((rand8() % 2) == 0) {
                    particleEffectWork[updateMenuParticlesMenuItemIndex].rotationSpeed *= -1;
                }
                particleEffectWork[updateMenuParticlesMenuItemIndex].particleState = 1;
                break;
            case 1:
                if (Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].pos.y <= 0.5f) {
                    Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].pos.y = 0.5f;
                    if (particleEffectWork[updateMenuParticlesMenuItemIndex].verticalVelocity > 0.0f) {
                        Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.x = MentApproachValue(Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.x, 180.0f, 5.0f);
                    }
                    else {
                        Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.x = MentApproachValue(Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.x, 0.0f, 5.0f);
                    }
                    Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.z = MentApproachValue(Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.z, 0.0f, 5.0f);
                    particleEffectWork[updateMenuParticlesMenuItemIndex].particleAlpha -= 0.025f;
                    if (particleEffectWork[updateMenuParticlesMenuItemIndex].particleAlpha <= 0.0f) {
                        particleEffectWork[updateMenuParticlesMenuItemIndex].particleAlpha = 0.0f;
                        particleEffectWork[updateMenuParticlesMenuItemIndex].particleState = 0;
                        if (object->work[0] == 3) {
                            particleEffectWork[updateMenuParticlesMenuItemIndex].particleState = 2;
                        }
                    }
                }
                else {
                    if (particleEffectWork[updateMenuParticlesMenuItemIndex].particleLifetime-- <= 0) {
                        particleEffectWork[updateMenuParticlesMenuItemIndex].particleLifetime = (rand8() % 30) + 0x1E;
                        particleEffectWork[updateMenuParticlesMenuItemIndex].horizontalVelocity = 0.01f * (25.0f + (rand8() % 50));
                        if ((rand8() % 2) == 0) {
                            particleEffectWork[updateMenuParticlesMenuItemIndex].horizontalVelocity *= -1.0f;
                        }
                        particleEffectWork[updateMenuParticlesMenuItemIndex].verticalVelocity = rand8() % 2;
                        particleEffectWork[updateMenuParticlesMenuItemIndex].depthVelocity = 0.01f * ((rand8() % 100) - 0x32);
                        particleEffectWork[updateMenuParticlesMenuItemIndex].rotationSpeed = (rand8() % 5) + 5;
                        if ((rand8() % 2) == 0) {
                            particleEffectWork[updateMenuParticlesMenuItemIndex].rotationSpeed *= -1;
                        }
                    }
                    Hu3DModelAttrReset(object->mdlId[updateMenuParticlesMenuItemIndex], HU3D_ATTR_DISPOFF);
                    Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].pos.x += particleEffectWork[updateMenuParticlesMenuItemIndex].horizontalVelocity;
                    Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].pos.y -= (updateMenuParticlesMenuItemIndex % 3) + 2;
                    Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].pos.z += particleEffectWork[updateMenuParticlesMenuItemIndex].depthVelocity;
                    Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.x += particleEffectWork[updateMenuParticlesMenuItemIndex].rotationSpeed;
                    Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.y += particleEffectWork[updateMenuParticlesMenuItemIndex].rotationSpeed;
                    Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.z += particleEffectWork[updateMenuParticlesMenuItemIndex].rotationSpeed;
                    if (Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.x >= 360.0f) {
                        Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.x -= 360.0f;
                    }
                    else if (Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.x <= 0.0f) {
                        Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.x += 360.0f;
                    }
                    if (Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.y >= 360.0f) {
                        Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.y -= 360.0f;
                    }
                    else if (Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.y <= 0.0f) {
                        Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.y += 360.0f;
                    }
                    if (Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.z >= 360.0f) {
                        Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.z -= 360.0f;
                    }
                    else if (Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.z <= 0.0f) {
                        Hu3DData[object->mdlId[updateMenuParticlesMenuItemIndex]].rot.z += 360.0f;
                    }
                }
                break;
            case 2:
                Hu3DModelAttrSet(object->mdlId[updateMenuParticlesMenuItemIndex], HU3D_ATTR_DISPOFF);
                break;
        }
    }
}

void MentCreateMenuParticles(OMOBJ *object)
{
    float createMenuParticlesInterpolationValue;
    float createMenuParticlesInterpolationValue2;
    s32 createMenuParticlesMenuItemIndex;
    s32 createMenuParticlesStateValue;
    s32 createMenuParticlesStateValue2;

    object->mdlId[0] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MENT, 12));
    for (createMenuParticlesMenuItemIndex = 0; createMenuParticlesMenuItemIndex < 0xC8; createMenuParticlesMenuItemIndex++) {
        object->mdlId[createMenuParticlesMenuItemIndex] = Hu3DModelLink(object->mdlId[0]);
    }
    for (createMenuParticlesMenuItemIndex = 0; createMenuParticlesMenuItemIndex < 0xC8; createMenuParticlesMenuItemIndex++) {
        Hu3DModelAttrSet(object->mdlId[createMenuParticlesMenuItemIndex], HU3D_ATTR_DISPOFF);
        Hu3DData[object->mdlId[createMenuParticlesMenuItemIndex]].pos.x = rand8() + rand8();
        if (rand8() == 0) {
            Hu3DData[object->mdlId[createMenuParticlesMenuItemIndex]].pos.x *= -1.0f;
        }
        Hu3DData[object->mdlId[createMenuParticlesMenuItemIndex]].pos.y = 500.0f + rand8() + rand8();
        Hu3DData[object->mdlId[createMenuParticlesMenuItemIndex]].pos.z = 700.0f + (rand8() % 128);
        createMenuParticlesInterpolationValue2 = 1.0f + (0.1f * (rand8() % 5));
        Hu3DModelScaleSet(object->mdlId[createMenuParticlesMenuItemIndex], createMenuParticlesInterpolationValue2, createMenuParticlesInterpolationValue2, createMenuParticlesInterpolationValue2);
        particleEffectWork[createMenuParticlesMenuItemIndex].horizontalVelocity = 0.01f * (25.0f + (rand8() % 50));
        if ((rand8() % 2) == 0) {
            particleEffectWork[createMenuParticlesMenuItemIndex].horizontalVelocity *= -1.0f;
        }
        particleEffectWork[createMenuParticlesMenuItemIndex].verticalVelocity = rand8() % 2;
        particleEffectWork[createMenuParticlesMenuItemIndex].depthVelocity = 0.01f * ((rand8() % 100) - 0x32);
        particleEffectWork[createMenuParticlesMenuItemIndex].rotationSpeed = (rand8() % 5) + 5;
        if ((rand8() % 2) == 0) {
            particleEffectWork[createMenuParticlesMenuItemIndex].rotationSpeed *= -1;
        }
        createMenuParticlesStateValue = rand8() % 4;
        switch (createMenuParticlesStateValue) {
            case 0:
                Hu3DModelAmbSet(object->mdlId[createMenuParticlesMenuItemIndex], 1.0f, 1.0f, 1.0f);
                break;
            case 1:
                Hu3DModelAmbSet(object->mdlId[createMenuParticlesMenuItemIndex], 1.0f, 0.0f, 0.0f);
                break;
            case 2:
                Hu3DModelAmbSet(object->mdlId[createMenuParticlesMenuItemIndex], 0.0f, 1.0f, 0.0f);
                break;
            case 3:
                Hu3DModelAmbSet(object->mdlId[createMenuParticlesMenuItemIndex], 0.0f, 0.0f, 1.0f);
                break;
        }
        particleEffectWork[createMenuParticlesMenuItemIndex].particleState = 0;
    }
    object->objFunc = MentUpdateMenuParticles;
}

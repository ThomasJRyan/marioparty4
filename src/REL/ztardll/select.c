#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#include "REL/ztardll.h"

typedef void (*ZtarPlayerSelectionCallback)(OMOBJ *, ...);

typedef struct ZtarPlayerSelectionWork {
    /* 0x00 */ OMOBJ *object;
    /* 0x04 */ ZtarPlayerSelectionCallback updateCallback;
    /* 0x08 */ s32 selectionState;
    /* 0x0C */ s32 selectedPlayerIndex;
    /* 0x10 */ s32 selectionChanged;
    /* 0x14 */ s32 playerActive;
    /* 0x18 */ char pad18[0x10]; /* maybe part of playerActive[5]? */
    /* 0x28 */ s32 teamCountTransitionActive;
    /* 0x2C */ s32 unreferencedSelectionValue2C;
    /* 0x30 */ s32 characterSelectionBusy;
    /* 0x34 */ s32 unreferencedSelectionValue34;
    /* 0x38 */ char pad38[0x10]; /* maybe part of unreferencedSelectionValue34[5]? */
    /* 0x48 */ s32 playerIndex;
    /* 0x4C */ s32 team;
    /* 0x50 */ s32 isComputer;
    /* 0x54 */ s32 difficulty;
    /* 0x58 */ s32 controllerPort;
    /* 0x5C */ s32 character;
} ZtarPlayerSelectionWork; /* size = 0x60 */

void HideCharacterDifficultySprites(s32 difficultyLayout);
void ShowCharacterDifficultySprites(s32 difficultyLayout);
void HideCharacterDifficultySpritesAnimated(s32);
void RunSharedSelectionObject(void);
void RunPlayerSelectionObject(void);
s32 MapCompactCharacterIndex(s32 characterIndex);
s32 MapCharacterIndexToGrid(s32 characterIndex);
float InterpolateEaseOut(float startValue, float endValue, float elapsedFrames, float durationFrames);
float InterpolateEaseIn(float startValue, float endValue, float elapsedFrames, float durationFrames);
void SleepSelectionProcess(s32 sleepFrames);
void SetCharacterGridLayout(void);
void SetCharacterGridLayoutAlternate(void);
void SetCharacterGridLayoutCompact(void);
void PlayCharacterSelectionSound(s32 characterIndex);
void CreateTeamSelectionSprites(void);
void HideTeamSelectionSprites(void);
void ShowTeamSelectionSprites(void);
void AnimateTeamSelectionIn(void);
void AnimateTwoPlayerSelectionIn(void);
void AnimateTeamSelectionOut(void);
void HideTeamSelectionPlayers(void);
void UpdateTeamCountSelection(OMOBJ *selectionObject, ZtarPlayerSelectionWork *selectionWork);
void UpdateTwoPlayerTeamCountSelection(OMOBJ *selectionObject, ZtarPlayerSelectionWork *selectionWork);
void CreateCharacterSelectionSprites(void);
void PrepareCharacterGridSprites(s32 gridStyle);
void HideCharacterGridSprites(s32 gridStyle);
void AnimateCharacterGridIn(s32 gridStyle, s32 difficultyLayout);
void AnimateCharacterGridInAlternate(s32 gridStyle);
void AnimateCharacterGridOut(s32 gridStyle);
void ResetCharacterGridPositions(void);
void UpdatePlayerCharacterCursor(OMOBJ *selectionObject, ZtarPlayerSelectionWork *playerSelection);
void ShowPlayerCharacterSelection(ZtarPlayerSelectionWork *playerSelection);
void HidePlayerCharacterSelection(ZtarPlayerSelectionWork *playerSelection);
void UpdateComputerCharacterCursor(OMOBJ *selectionObject, ZtarPlayerSelectionWork *computerSelection);
s32 SelectComputerDifficulty(ZtarPlayerSelectionWork *computerSelection);
void CreateCharacterDifficultySprites(void);
void SetCharacterDifficultySprites(s32 difficultyLayout);

extern s16 characterAssetLoadActive;

s32 characterMotionDirectories[8] = {
    DATADIR_MARIOMOT,
    DATADIR_LUIGIMOT,
    DATADIR_PEACHMOT,
    DATADIR_YOSHIMOT,
    DATADIR_WARIOMOT,
    DATADIR_DONKEYMOT,
    DATADIR_DAISYMOT,
    DATADIR_WALUIGIMOT,
};
s32 characterModelDirectories[8] = {
    DATADIR_MARIOMDL1,
    DATADIR_LUIGIMDL1,
    DATADIR_PEACHMDL1,
    DATADIR_YOSHIMDL1,
    DATADIR_WARIOMDL1,
    DATADIR_DONKEYMDL1,
    DATADIR_DAISYMDL1,
    DATADIR_WALUIGIMDL1,
};

s32 selectionSpriteGroups[0x1B];
ZtarPlayerSelectionWork teamSelectionWork;
ZtarPlayerSelectionWork playerSelectionWork[4];
s32 nextPlayerSelectionSlot[5];
s32 wideCharacterGridEnabled;
HUPROCESS *selectionProcess;

void LoadSelectedCharacterAssets(void)
{
    s16 playerIndex;
    s32 dataReadHandle;

    CharDataClose(-1);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (GWPlayerCfg[playerIndex].character < 8) {
            dataReadHandle = HuDataDirReadAsync(characterMotionDirectories[GWPlayerCfg[playerIndex].character]);
            if (dataReadHandle != -1) {
                while (HuDataGetAsyncStat(dataReadHandle) == 0) {
                    HuPrcVSleep();
                }
            }
            CharMotionInit(GWPlayerCfg[playerIndex].character);
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (GWPlayerCfg[playerIndex].character < 8) {
            dataReadHandle = HuDataDirReadAsync(characterModelDirectories[GWPlayerCfg[playerIndex].character]);
            if (dataReadHandle != -1) {
                while (HuDataGetAsyncStat(dataReadHandle) == 0) {
                    HuPrcVSleep();
                }
            }
        }
    }
    characterAssetLoadActive = 0;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

void StartPlayerSelectionTasks(HUPROCESS *parentProcess)
{
    s32 loopIndex;

    selectionProcess = parentProcess;
    CreateTeamSelectionSprites();
    CreateCharacterSelectionSprites();
    CreateCharacterDifficultySprites();
    HuPrcChildCreate(RunSharedSelectionObject, 0x3E8, 0x2000, 0, selectionProcess);
    for (loopIndex = 0; loopIndex < 4; loopIndex++) {
        HuPrcChildCreate(RunPlayerSelectionObject, 0x3E8, 0x2000, 0, selectionProcess);
    }
}

s32 RunPlayerSelectionMenu(s32 selectionMode)
{
    s32 playerIndex;
    s32 inactivePlayerCount;
    s32 previousInactivePlayerCount;
    s32 confirmed;
    s32 humanPlayerCount;
    s32 connectedHumanCount;

    previousInactivePlayerCount = -1;
    AnimateTeamSelectionIn();
    OpenSelectionWindow();
    SetSelectionWindowMessage(MAKE_MESSID(0x1A, 0x05), -1, -1);
    SleepSelectionProcess(1);
    OpenMessageWindow(MAKE_MESSID(0x33, 0x91));
    teamSelectionWork.updateCallback = (ZtarPlayerSelectionCallback)UpdateTeamCountSelection;
    previousInactivePlayerCount = -1;
    while (TRUE) {
        SleepSelectionProcess(1);
        inactivePlayerCount = 0;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerSelectionWork[playerIndex].playerActive == 0) {
                inactivePlayerCount++;
            }
        }
        if (previousInactivePlayerCount != inactivePlayerCount) {
            previousInactivePlayerCount = inactivePlayerCount;
            SetSelectionWindowMessage(inactivePlayerCount + MAKE_MESSID(0x1A, 0x1B), -1, 1);
        }
        if (teamSelectionWork.teamCountTransitionActive == 0) {
            if (HuPadBtnDown[0] & PAD_BUTTON_A) {
                HuAudFXPlay(2);
                confirmed = 1;
                break;
            }
            else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
                HuAudFXPlay(3);
                confirmed = 0;
                break;
            }
        }
    }
    HideTeamSelectionPlayers();
    teamSelectionWork.updateCallback = NULL;
    CloseMessageWindow();
    if (confirmed == 1) {
        humanPlayerCount = 0;
        connectedHumanCount = 0;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerSelectionWork[playerIndex].isComputer == 0) {
                humanPlayerCount++;
            }
        }
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (connectedHumanCount < humanPlayerCount) {
                if (HuPadStatGet(playerIndex) != -1) {
                    playerSelectionWork[playerIndex].isComputer = 0;
                    connectedHumanCount++;
                }
                else {
                    playerSelectionWork[playerIndex].isComputer = 1;
                }
            }
            else {
                playerSelectionWork[playerIndex].isComputer = 1;
            }
        }
    }
    CloseSelectionWindow();
    AnimateTeamSelectionOut();
    return confirmed;
}

s32 HandleCharacterSelectionInput(s32 selectionVariant, s32 playerCountMode)
{
    s32 unusedSelectionFlag;
    s32 unusedSelectionResult;
    s32 playerIndex;
    s32 playerLimit;
    s32 computerPlayerCount;
    s32 selectionResult;

    unusedSelectionFlag = 0;
    unusedSelectionResult = 0;
    playerLimit = 0;
    if (playerCountMode == 1) {
        playerLimit = 1;
        for (playerIndex = 1; playerIndex < 4; playerIndex++) {
            playerSelectionWork[playerIndex].isComputer = 1;
        }
    }
    else {
        playerLimit = 4;
    }
    ResetCharacterGridPositions();
    if (selectionVariant == 0) {
        AnimateCharacterGridInAlternate(0);
    }
    OpenSelectionWindow();
    SleepSelectionProcess(1);
    if (playerCountMode == 1) {
        SetSelectionWindowMessage(MAKE_MESSID(0x1E, 0x5E), -1, -1);
    }
    else {
        SetSelectionWindowMessage(MAKE_MESSID(0x1A, 0x09), -1, -1);
    }
    OpenMessageWindow(MAKE_MESSID(0x33, 0x92));
    for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
        if (playerSelectionWork[playerIndex].isComputer == 0) {
            playerSelectionWork[playerIndex].updateCallback = (ZtarPlayerSelectionCallback)UpdatePlayerCharacterCursor;
        }
    }
    while (TRUE) {
        SleepSelectionProcess(1);
        if ((playerSelectionWork[0].selectionState == 0) && (HuPadBtnDown[0] & PAD_BUTTON_B) != 0) {
            HuAudFXPlay(3);
            if (teamSelectionWork.selectionChanged == 1) {
                teamSelectionWork.selectionChanged = 0;
                continue;
            }
            selectionResult = 0;
            break;
        }
        else {
            for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
                if ((playerSelectionWork[playerIndex].isComputer == 0) && (playerSelectionWork[playerIndex].selectionState != 1)) {
                    break;
                }
            }
            if (playerIndex == playerLimit) {
                selectionResult = 2;
                break;
            }
        }
    }
    for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
        playerSelectionWork[playerIndex].updateCallback = NULL;
    }
    CloseMessageWindow();
    CloseSelectionWindow();
    if (selectionResult == 0) {
        AnimateCharacterGridOut(0);
        return 0;
    }
    computerPlayerCount = 0;
    for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
        if (playerSelectionWork[playerIndex].isComputer != 0) {
            playerSelectionWork[playerIndex].character = 0;
            computerPlayerCount++;
        }
    }
    if ((computerPlayerCount == 0) || ((playerCountMode == 2) && (computerPlayerCount == 2))) {
        AnimateCharacterGridOut(0);
        return 2;
    }
    if (playerCountMode == 2) {
        playerSelectionWork[1].team = 0;
    }
    return 1;
}

void UpdateCharacterSelectionSprites(s32 selectionVariant)
{
    HandleCharacterSelectionInput(selectionVariant, 4);
}

s32 GetCharacterSelectionResult(s32 animateGrid, s32 playerCount)
{
    s32 unusedSelectionResult;
    s32 playerIndex;
    s32 computerPlayerIndex;
    s32 selectionResult;
    s32 previousComputerPlayerIndex;
    s32 messageState;
    s32 unselectedComputerCount;
    s32 selectionTransitionState;

    computerPlayerIndex = -1;
    previousComputerPlayerIndex = -1;
    messageState = -1;
    selectionTransitionState = -1;
    unusedSelectionResult = 0;
    if (animateGrid != 0) {
        if (playerCount == 2) {
            AnimateCharacterGridIn(0, 2);
        }
        else {
            AnimateCharacterGridIn(0, 1);
        }
    }
    OpenSelectionWindow();
    while (TRUE) {
        SleepSelectionProcess(1);
        unselectedComputerCount = 0;
        for (playerIndex = 0; playerIndex < playerCount; playerIndex++) {
            if ((playerSelectionWork[playerIndex].isComputer != 0) && (playerSelectionWork[playerIndex].selectionState == 0)) {
                unselectedComputerCount++;
            }
        }
        if (messageState == -1) {
            InsertSelectionWindowMessage(unselectedComputerCount + MAKE_MESSID(0x1A, 0x23), 1);
            SetSelectionWindowMessage(MAKE_MESSID(0x1A, 0x28), -1, messageState);
        }
        else {
            InsertSelectionWindowMessage(unselectedComputerCount + MAKE_MESSID(0x1A, 0x23), 1);
            SetSelectionWindowMessage(MAKE_MESSID(0x1A, 0x28), -1, messageState);
        }
        messageState = 1;
        OpenMessageWindow(MAKE_MESSID(0x33, 0x92));
        for (playerIndex = 0; playerIndex < playerCount; playerIndex++) {
            if ((playerSelectionWork[playerIndex].isComputer != 0) && (playerSelectionWork[playerIndex].selectionState == 0)) {
                computerPlayerIndex = playerIndex;
                playerSelectionWork[playerIndex].selectedPlayerIndex = previousComputerPlayerIndex;
                ShowPlayerCharacterSelection(&playerSelectionWork[playerIndex]);
                playerSelectionWork[playerIndex].updateCallback = (ZtarPlayerSelectionCallback)UpdateComputerCharacterCursor;
                break;
            }
        }
        while (TRUE) {
            SleepSelectionProcess(1);
            if (((HuPadBtnDown[playerSelectionWork[0].controllerPort] & 0x200) != 0) && (playerSelectionWork[computerPlayerIndex].selectionState == 0) && (teamSelectionWork.characterSelectionBusy == 0)) {
                HuAudFXPlay(3);
                if (previousComputerPlayerIndex == -1) {
                    selectionResult = 0;
                }
                else {
                    selectionResult = 1;
                }
                break;
            }
            else if (playerSelectionWork[computerPlayerIndex].selectionState == 1) {
                for (playerIndex = 0; playerIndex < playerCount; playerIndex++) {
                    if (playerSelectionWork[playerIndex].selectionState == 0) {
                        playerSelectionWork[playerIndex].character = 0;
                    }
                }
                playerSelectionWork[computerPlayerIndex].updateCallback = NULL;
                SetSelectionWindowMessage(MAKE_MESSID(0x1A, 0x22), -1, 1);
                SleepSelectionProcess(1);
                OpenMessageWindow(MAKE_MESSID(0x33, 0x93));
                selectionResult = SelectComputerDifficulty(&playerSelectionWork[computerPlayerIndex]);
                if (selectionResult == 1) {
                    HuAudFXPlay(2);
                    playerSelectionWork[computerPlayerIndex].selectionChanged = 1;
                    selectionResult = 4;
                    previousComputerPlayerIndex = computerPlayerIndex;
                }
                else {
                    HuAudFXPlay(3);
                    playerSelectionWork[computerPlayerIndex].selectionChanged = 0;
                    selectionResult = 4;
                    playerSelectionWork[computerPlayerIndex].selectionState = 0;
                    HidePlayerCharacterSelection(&playerSelectionWork[computerPlayerIndex]);
                    ShowPlayerCharacterSelection(&playerSelectionWork[computerPlayerIndex]);
                }
                break;
            }
        }
        for (playerIndex = 0; playerIndex < playerCount; playerIndex++) {
            playerSelectionWork[playerIndex].updateCallback = NULL;
        }
        for (playerIndex = 0; playerIndex < playerCount; playerIndex++) {
            if (playerSelectionWork[playerIndex].selectionState == 0) {
                break;
            }
        }
        if (playerIndex == playerCount) {
            selectionResult = 3;
            break;
        }
        else {
            if (selectionResult == 1) {
                ShowPlayerCharacterSelection(&playerSelectionWork[computerPlayerIndex]);
                HidePlayerCharacterSelection(&playerSelectionWork[computerPlayerIndex]);
                ShowPlayerCharacterSelection(&playerSelectionWork[playerSelectionWork[computerPlayerIndex].selectedPlayerIndex]);
                previousComputerPlayerIndex = playerSelectionWork[playerSelectionWork[computerPlayerIndex].selectedPlayerIndex].selectedPlayerIndex;
                playerSelectionWork[playerSelectionWork[computerPlayerIndex].selectedPlayerIndex].selectionState = 0;
            }
            else if (selectionResult != 4) {
                if (selectionResult == 0) {
                    HidePlayerCharacterSelection(&playerSelectionWork[computerPlayerIndex]);
                }
                break;
            }
            selectionTransitionState = -1;
        }
    }

    CloseMessageWindow();
    CloseSelectionWindow();
    if (selectionResult != 0) {
        AnimateCharacterGridOut(0);
    }
    return selectionResult;
}

void SetPlayerSelectionState(s32 animateGrid)
{
    GetCharacterSelectionResult(animateGrid, 4);
}

void ResetPlayerSelectionState(void)
{
    HandleCharacterSelectionInput(0, 1);
}

s32 GetActivePlayerCount(void)
{
    s32 playerIndex;
    s32 inactivePlayerCount;
    s32 humanPlayerCount;
    s32 confirmed;
    s32 connectedHumanCount;
    s32 previousInactivePlayerCount;

    previousInactivePlayerCount = -1;
    AnimateTwoPlayerSelectionIn();
    OpenSelectionWindow();
    SetSelectionWindowMessage(MAKE_MESSID(0x1A, 0x05), -1, -1);
    SleepSelectionProcess(1);
    OpenMessageWindow(MAKE_MESSID(0x33, 0x91));
    teamSelectionWork.updateCallback = (ZtarPlayerSelectionCallback)UpdateTwoPlayerTeamCountSelection;
    while (TRUE) {
        SleepSelectionProcess(1);
        inactivePlayerCount = 0;
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            if (playerSelectionWork[playerIndex].playerActive == 0) {
                inactivePlayerCount++;
            }
        }
        if (previousInactivePlayerCount != inactivePlayerCount) {
            previousInactivePlayerCount = inactivePlayerCount;
            SetSelectionWindowMessage(inactivePlayerCount + MAKE_MESSID(0x33, 0x93), -1, 1);
        }
        if (teamSelectionWork.teamCountTransitionActive != 0) {
            continue;
        }
        if ((*HuPadBtnDown & PAD_BUTTON_A) != 0) {
            HuAudFXPlay(2);
            confirmed = 1;
            break;
        }
        else if ((*HuPadBtnDown & PAD_BUTTON_B) != 0) {
            HuAudFXPlay(3);
            confirmed = 0;
            break;
        }
    }
    HideTeamSelectionPlayers();
    teamSelectionWork.updateCallback = NULL;
    CloseMessageWindow();
    if (confirmed == 1) {
        humanPlayerCount = 0;
        connectedHumanCount = 0;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerSelectionWork[playerIndex].isComputer == 0) {
                humanPlayerCount++;
            }
        }
        if (humanPlayerCount >= 2) {
            humanPlayerCount = 2;
        }
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (connectedHumanCount < humanPlayerCount) {
                if (HuPadStatGet(playerIndex) != -1) {
                    playerSelectionWork[playerIndex].isComputer = 0;
                    playerSelectionWork[playerIndex].team = 0;
                    connectedHumanCount++;
                }
                else {
                    playerSelectionWork[playerIndex].isComputer = 1;
                    playerSelectionWork[playerIndex].team = 1;
                }
            }
            else {
                playerSelectionWork[playerIndex].isComputer = 1;
                playerSelectionWork[playerIndex].team = 1;
            }
        }
    }
    CloseSelectionWindow();
    AnimateTeamSelectionOut();
    return confirmed;
}

void SetCharacterSelectionState(s32 selectionVariant)
{
    HandleCharacterSelectionInput(selectionVariant, 2);
}

void SetCharacterSelectionStateAlternate(s32 animateGrid)
{
    GetCharacterSelectionResult(animateGrid, 2);
}

void RefreshCharacterSelection(void)
{
    RunPlayerSelectionMenu(0);
}

s32 GetCharacterSelectionIndex(s32 selectionVariant)
{
    s32 unusedSelectionFlag;
    s32 unusedSelectionResult;
    s32 playerIndex;
    s32 playerLimit;
    s32 computerPlayerCount;
    s32 selectionMode;
    s32 selectionResult;

    unusedSelectionFlag = 0;
    unusedSelectionResult = 0;
    playerLimit = 4;
    selectionMode = 4;
    if (selectionMode == 1) {
        playerLimit = 1;
        for (playerIndex = 1; playerIndex < 4; playerIndex++) {
            playerSelectionWork[playerIndex].isComputer = 1;
        }
    }
    else {
        playerLimit = 4;
    }
    ResetCharacterGridPositions();
    if (selectionVariant == 0) {
        AnimateCharacterGridInAlternate(3);
    }
    OpenSelectionWindow();
    SleepSelectionProcess(1);
    SetSelectionWindowMessage(MAKE_MESSID(0x1A, 0x09), -1, -1);
    OpenMessageWindow(MAKE_MESSID(0x33, 0x92));
    for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
        if (playerSelectionWork[playerIndex].isComputer == 0) {
            playerSelectionWork[playerIndex].updateCallback = (ZtarPlayerSelectionCallback)UpdatePlayerCharacterCursor;
        }
    }
    while (TRUE) {
        SleepSelectionProcess(1);
        if ((playerSelectionWork[0].selectionState == 0) && ((*HuPadBtnDown & 0x200) != 0)) {
            HuAudFXPlay(3);
            if (teamSelectionWork.selectionChanged == 1) {
                teamSelectionWork.selectionChanged = 0;
                continue;
            }
            selectionResult = 0;
            break;
        }
        else {
            for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
                if ((playerSelectionWork[playerIndex].isComputer == 0) && (playerSelectionWork[playerIndex].selectionState != 1)) {
                    break;
                }
            }
            if (playerIndex == playerLimit) {
                selectionResult = 2;
                break;
            }
        }
    }
    for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
        playerSelectionWork[playerIndex].updateCallback = NULL;
    }
    CloseMessageWindow();
    CloseSelectionWindow();
    if (selectionResult == 0) {
        AnimateCharacterGridOut(0);
        return 0;
    }
    computerPlayerCount = 0;
    for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
        if (playerSelectionWork[playerIndex].isComputer != 0) {
            playerSelectionWork[playerIndex].character = 1;
            computerPlayerCount++;
        }
    }
    if ((computerPlayerCount == 0) || ((selectionMode == 2) && (computerPlayerCount == 2))) {
        AnimateCharacterGridOut(3);
        return 2;
    }
    if (selectionMode == 2) {
        playerSelectionWork[1].team = 0;
    }
    return 1;
}

s32 GetPlayerCharacterSelection(s32 animateGrid)
{
    s32 unusedSelectionResult;
    s32 playerIndex;
    s32 computerPlayerIndex;
    s32 selectionResult;
    s32 playerLimit;
    s32 previousComputerPlayerIndex;
    s32 messageState;
    s32 unselectedComputerCount;
    s32 selectionTransitionState;

    computerPlayerIndex = -1;
    previousComputerPlayerIndex = -1;
    messageState = -1;
    playerLimit = 4;
    selectionTransitionState = -1;
    unusedSelectionResult = 0;
    if (animateGrid != 0) {
        AnimateCharacterGridIn(3, 3);
    }
    OpenSelectionWindow();
    while (TRUE) {
        SleepSelectionProcess(1);
        unselectedComputerCount = 0;
        for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
            if ((playerSelectionWork[playerIndex].isComputer != 0) && (playerSelectionWork[playerIndex].selectionState == 0)) {
                unselectedComputerCount++;
            }
        }
        if (messageState == -1) {
            InsertSelectionWindowMessage(unselectedComputerCount + MAKE_MESSID(0x1A, 0x23), 1);
            SetSelectionWindowMessage(MAKE_MESSID(0x1A, 0x28), -1, messageState);
        }
        else {
            InsertSelectionWindowMessage(unselectedComputerCount + MAKE_MESSID(0x1A, 0x23), 1);
            SetSelectionWindowMessage(MAKE_MESSID(0x1A, 0x28), -1, messageState);
        }
        messageState = 1;
        OpenMessageWindow(MAKE_MESSID(0x33, 0x92));
        for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
            if ((playerSelectionWork[playerIndex].isComputer != 0) && (playerSelectionWork[playerIndex].selectionState == 0)) {
                computerPlayerIndex = playerIndex;
                playerSelectionWork[playerIndex].selectedPlayerIndex = previousComputerPlayerIndex;
                ShowPlayerCharacterSelection(&playerSelectionWork[playerIndex]);
                playerSelectionWork[playerIndex].updateCallback = (ZtarPlayerSelectionCallback)UpdateComputerCharacterCursor;
                break;
            }
        }
        while (TRUE) {
            SleepSelectionProcess(1);
            if ((HuPadBtnDown[playerSelectionWork[0].controllerPort] & PAD_BUTTON_B) && (playerSelectionWork[computerPlayerIndex].selectionState == 0) && (teamSelectionWork.characterSelectionBusy == 0)) {
                HuAudFXPlay(3);
                if (previousComputerPlayerIndex == -1) {
                    selectionResult = 0;
                }
                else {
                    selectionResult = 1;
                }
                break;
            }
            else if (playerSelectionWork[computerPlayerIndex].selectionState == 1) {
                for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
                    if (playerSelectionWork[playerIndex].selectionState == 0) {
                        playerSelectionWork[playerIndex].character = 1;
                    }
                }
                playerSelectionWork[computerPlayerIndex].updateCallback = NULL;
                SetSelectionWindowMessage(MAKE_MESSID(0x1A, 0x22), -1, 1);
                SleepSelectionProcess(1);
                OpenMessageWindow(MAKE_MESSID(0x33, 0x93));
                selectionResult = SelectComputerDifficulty(&playerSelectionWork[computerPlayerIndex]);
                if (selectionResult == 1) {
                    HuAudFXPlay(2);
                    playerSelectionWork[computerPlayerIndex].selectionChanged = 1;
                    selectionResult = 4;
                    previousComputerPlayerIndex = computerPlayerIndex;
                }
                else {
                    HuAudFXPlay(3);
                    playerSelectionWork[computerPlayerIndex].selectionChanged = 0;
                    selectionResult = 4;
                    playerSelectionWork[computerPlayerIndex].selectionState = 0;
                    HidePlayerCharacterSelection(&playerSelectionWork[computerPlayerIndex]);
                    ShowPlayerCharacterSelection(&playerSelectionWork[computerPlayerIndex]);
                }
                break;
            }
        }
        for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
            playerSelectionWork[playerIndex].updateCallback = NULL;
        }
        for (playerIndex = 0; playerIndex < playerLimit; playerIndex++) {
            if (playerSelectionWork[playerIndex].selectionState == 0) {
                break;
            }
        }
        if (playerIndex == playerLimit) {
            selectionResult = 3;
            break;
        }
        else {
            if (selectionResult == 1) {
                ShowPlayerCharacterSelection(&playerSelectionWork[computerPlayerIndex]);
                HidePlayerCharacterSelection(&playerSelectionWork[computerPlayerIndex]);
                ShowPlayerCharacterSelection(&playerSelectionWork[playerSelectionWork[computerPlayerIndex].selectedPlayerIndex]);
                previousComputerPlayerIndex = playerSelectionWork[playerSelectionWork[computerPlayerIndex].selectedPlayerIndex].selectedPlayerIndex;
                playerSelectionWork[playerSelectionWork[computerPlayerIndex].selectedPlayerIndex].selectionState = 0;
            }
            else if (selectionResult == 4) {
            }
            else if (selectionResult == 0) {
                HidePlayerCharacterSelection(&playerSelectionWork[computerPlayerIndex]);
                break;
            }
            else {
                break;
            }
            selectionTransitionState = -1;
        }
    }
    CloseMessageWindow();
    CloseSelectionWindow();
    if (selectionResult != 0) {
        AnimateCharacterGridOut(3);
    }
    return selectionResult;
}

void UpdateCharacterSelection(s32 rotatePlayerCharacters)
{
    s32 playerIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        GWPlayer[playerIndex].team = GWPlayerCfg[playerIndex].group = playerSelectionWork[playerIndex].team;
        GWPlayer[playerIndex].com = GWPlayerCfg[playerIndex].iscom = playerSelectionWork[playerIndex].isComputer;
        GWPlayer[playerIndex].diff = GWPlayerCfg[playerIndex].diff = playerSelectionWork[playerIndex].difficulty;
        if (wideCharacterGridEnabled != 0) {
            if (playerSelectionWork[playerIndex].character <= 5) {
                GWPlayer[playerIndex].character = GWPlayerCfg[playerIndex].character = playerSelectionWork[playerIndex].character - 1;
            }
            else if (playerSelectionWork[playerIndex].character <= 0xB) {
                GWPlayer[playerIndex].character = GWPlayerCfg[playerIndex].character = playerSelectionWork[playerIndex].character - 3;
            }
            else {
                GWPlayer[playerIndex].character = GWPlayerCfg[playerIndex].character = MapCharacterIndexToGrid(playerSelectionWork[playerIndex].character - 4);
            }
        }
        else if (rotatePlayerCharacters == 1) {
            if (playerIndex != 0) {
                playerSelectionWork[playerIndex].character = playerSelectionWork[playerIndex - 1].character + 1;
                if (playerSelectionWork[playerIndex].character >= 8) {
                    playerSelectionWork[playerIndex].character = 0;
                }
            }
            GWPlayer[playerIndex].character = GWPlayerCfg[playerIndex].character = playerSelectionWork[playerIndex].character;
        }
        else {
            GWPlayer[playerIndex].character = GWPlayerCfg[playerIndex].character = playerSelectionWork[playerIndex].character;
        }
        GWPlayer[playerIndex].port = GWPlayerCfg[playerIndex].pad_idx = playerSelectionWork[playerIndex].controllerPort;
    }
    OSReport("\n\n#################### SET PLAYERDATA ####################\n");
    OSReport("ID) GRP:COM:DIF:CHR:PAD\n");
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        OSReport("%2d) %3d:%3d:%3d:%3d:%3d\n", playerIndex, GWPlayerCfg[playerIndex].group, GWPlayerCfg[playerIndex].iscom, GWPlayerCfg[playerIndex].diff,
            GWPlayerCfg[playerIndex].character, GWPlayerCfg[playerIndex].pad_idx);
    }
    OSReport("########################################################\n");
}

s32 playerCountIconPositions[4][2] = {
    { -0x69, 0 },
    { -0x23, 0 },
    { 0x23, 0 },
    { 0x69, 0 },
};
s32 characterGridPositions[8][2] = {
    { -0x7B, -0x34 },
    { -0x29, -0x34 },
    { 0x29, -0x34 },
    { 0x7B, -0x34 },
    { -0x7B, 0x34 },
    { -0x29, 0x34 },
    { 0x29, 0x34 },
    { 0x7B, 0x34 },
};
s32 wideCharacterGridPositions[0xE][2] = {
    { -0x7B, -0x66 },
    { -0x29, -0x66 },
    { 0x29, -0x66 },
    { 0x7B, -0x66 },
    { -0x7B, 0 },
    { -0x29, 0 },
    { 0x29, 0 },
    { 0x7B, 0 },
    { -0xCD, 0x66 },
    { -0x7B, 0x66 },
    { -0x29, 0x66 },
    { 0x29, 0x66 },
    { 0x7B, 0x66 },
    { 0xCD, 0x66 },
};
s32 alternateCharacterGridPositions[0x12][2] = {
    { 0, 0 },
    { -0x7B, -0x66 },
    { -0x29, -0x66 },
    { 0x29, -0x66 },
    { 0x7B, -0x66 },
    { 0, 0 },
    { 0, 0 },
    { -0x7B, 0 },
    { -0x29, 0 },
    { 0x29, 0 },
    { 0x7B, 0 },
    { 0, 0 },
    { -0xCD, 0x66 },
    { -0x7B, 0x66 },
    { -0x29, 0x66 },
    { 0x29, 0x66 },
    { 0x7B, 0x66 },
    { 0xCD, 0x66 },
};
s32 difficultyIconPositions[4][2] = {
    { -0x45, -8 },
    { -0x17, -8 },
    { 0x17, -8 },
    { 0x45, -8 },
};
s32 difficultyIconVerticalOffsets[2] = { 0, 0xFFFFFFF8 };
s32 characterDifficultyPositions[2][2] = {
    { -0x17, -8 },
    { 0x17, -8 },
};
s32 computerDifficultyPositions[4][2] = {
    { -0x59, -8 },
    { -0x2B, -8 },
    { 0x2B, -8 },
    { 0x59, -8 },
};
s32 selectionWindowId = -1;
s32 currentMessageWindowId = -1;

s32 GetPlayerSelectionValue(s32 selectionMode, s32 restorePlayerSettings)
{
    s32 playerIndex;
    s32 stateAction;
    s32 menuState;
    s32 lastSelectionResult;
    s32 hasComputerPlayers;
    s32 humanPlayerCount;
    s32 selectionResult;
    s32 fourPlayerSelectionResult;
    s32 fourPlayerCharacterResult;
    s32 onePlayerSelectionResult;
    s32 twoPlayerSelectionResult;
    s32 twoPlayerCharacterResult;
    s32 fourPlayerCharacterSelectionResult;

    stateAction = 0;
    hasComputerPlayers = 0;
    humanPlayerCount = 0;
    menuState = selectionMode;
    if (selectionMode == 3) {
        wideCharacterGridEnabled = 1;
    }
    else {
        wideCharacterGridEnabled = 0;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSelectionWork[playerIndex].playerIndex = playerIndex;
        playerSelectionWork[playerIndex].team = 0;
        playerSelectionWork[playerIndex].isComputer = 0;
        playerSelectionWork[playerIndex].difficulty = 0;
        if (wideCharacterGridEnabled != 0) {
            playerSelectionWork[playerIndex].character = playerIndex + 1;
        }
        else {
            playerSelectionWork[playerIndex].character = playerIndex;
        }
        playerSelectionWork[playerIndex].controllerPort = playerIndex;
    }
    if (restorePlayerSettings == 1) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            playerSelectionWork[playerIndex].playerIndex = playerIndex;
            playerSelectionWork[playerIndex].team = GWPlayerCfg[playerIndex].group;
            playerSelectionWork[playerIndex].isComputer = GWPlayerCfg[playerIndex].iscom;
            playerSelectionWork[playerIndex].difficulty = GWPlayerCfg[playerIndex].diff;
            playerSelectionWork[playerIndex].character = GWPlayerCfg[playerIndex].character;
            playerSelectionWork[playerIndex].controllerPort = GWPlayerCfg[playerIndex].pad_idx;
            playerSelectionWork[playerIndex].selectionState = playerSelectionWork[playerIndex].selectedPlayerIndex = playerSelectionWork[playerIndex].selectionChanged = playerSelectionWork[playerIndex].playerActive = 0;
            playerSelectionWork[playerIndex].teamCountTransitionActive = playerSelectionWork[playerIndex].unreferencedSelectionValue2C = playerSelectionWork[playerIndex].characterSelectionBusy = playerSelectionWork[playerIndex].unreferencedSelectionValue34 = 0;
            switch (selectionMode) {
                case 0:
                    if (playerSelectionWork[playerIndex].isComputer == 0) {
                        playerSelectionWork[playerIndex].selectionState = 1;
                    }
                    else {
                        hasComputerPlayers = 1;
                    }
                    break;
                case 2:
                    if (playerSelectionWork[playerIndex].isComputer == 0) {
                        playerSelectionWork[playerIndex].selectionState = 1;
                        humanPlayerCount++;
                    }
                    else {
                        hasComputerPlayers = 1;
                    }
                    break;
                case 3:
                    if (playerSelectionWork[playerIndex].isComputer == 0) {
                        playerSelectionWork[playerIndex].selectionState = 1;
                        humanPlayerCount++;
                    }
                    else {
                        hasComputerPlayers = 1;
                    }
                    if (playerSelectionWork[playerIndex].character <= 3) {
                        playerSelectionWork[playerIndex].character++;
                    }
                    else if (playerSelectionWork[playerIndex].character <= 7) {
                        playerSelectionWork[playerIndex].character += 3;
                    }
                    else {
                        playerSelectionWork[playerIndex].character = MapCompactCharacterIndex(playerSelectionWork[playerIndex].character) + 4;
                    }
                    break;
            }
        }
        switch (selectionMode) {
            case 0:
                if (hasComputerPlayers == 1) {
                    menuState = 0x66;
                    stateAction = 1;
                }
                else {
                    menuState = 0x65;
                    stateAction = 0;
                }
                break;
            case 2:
                    if (playerSelectionWork[playerIndex].isComputer == 0) {
                        playerSelectionWork[playerIndex].selectionState = 1;
                        humanPlayerCount++;
                    }
                    else {
                        hasComputerPlayers = 1;
                    }
                    break;
                case 3:
                    if (playerSelectionWork[playerIndex].isComputer == 0) {
                        playerSelectionWork[playerIndex].selectionState = 1;
                        humanPlayerCount++;
                    }
                    else {
                        hasComputerPlayers = 1;
                    }
                    if (playerSelectionWork[playerIndex].character <= 3) {
                        playerSelectionWork[playerIndex].character++;
                    }
                    else if (playerSelectionWork[playerIndex].character <= 7) {
                        playerSelectionWork[playerIndex].character += 3;
                    }
                    else {
                        playerSelectionWork[playerIndex].character = MapCompactCharacterIndex(playerSelectionWork[playerIndex].character) + 4;
                    }
                    break;
            }
        }
        switch (selectionMode) {
            case 0:
                if (hasComputerPlayers == 1) {
                    menuState = 0x66;
                    stateAction = 1;
                }
                else {
                    menuState = 0x65;
                    stateAction = 0;
                }
                break;
            case 1:
                menuState = 0xC8;
                break;
            case 2:
                if ((hasComputerPlayers == 1) && (humanPlayerCount == 1)) {
                    menuState = 0x12E;
                    stateAction = 1;
                }
                else {
                    menuState = 0x12D;
                    stateAction = 0;
                }
                break;
            case 3:
                if (hasComputerPlayers == 1) {
                    menuState = 0x192;
                    stateAction = 1;
                }
                else {
                    menuState = 0x191;
                    stateAction = 0;
                }
                break;
        }
    }
    while (TRUE) {
        SleepSelectionProcess(1);
        if (lastSelectionResult == 0x3E7) {
            break;
        }
        switch (menuState) {
            case 0x0:
                menuState = 0x64;
                break;
            case 0x1:
                menuState = 0xC8;
                break;
            case 0x2:
                menuState = 0x12C;
                break;
            case 0x3:
                menuState = 0x190;
                break;
            case 0x64:
                confirmed = RunPlayerSelectionMenu(inactivePlayerCount);
                if (lastSelectionResult != 0) {
                    menuState = 0x65;
                    stateAction = 0;
                }
                else {
                    menuState = 0x385;
                    stateAction = 1;
                }
                break;
            case 0x65:
                fourPlayerSelectionResult = HandleCharacterSelectionInput(playerLimit, 4);
                lastSelectionResult = fourPlayerSelectionResult;
                if (lastSelectionResult == 1) {
                    menuState = 0x66;
                    stateAction = 0;
                }
                else if (lastSelectionResult == 2) {
                    menuState = 0x384;
                    stateAction = 0;
                }
                else {
                    menuState = 0x64;
                    stateAction = 1;
                }
                break;
            case 0x66:
                fourPlayerCharacterResult = GetCharacterSelectionResult(computerPlayerIndex, 4);
                lastSelectionResult = fourPlayerCharacterResult;
                lastSelectionResult = lastSelectionResult;
                if (lastSelectionResult != 0) {
                    menuState = 0x384;
                    stateAction = 0;
                }
                else {
                    menuState = 0x65;
                    stateAction = 1;
                }
                break;
            case 0xC8:
                onePlayerSelectionResult = HandleCharacterSelectionInput(0, 1);
                lastSelectionResult = onePlayerSelectionResult;
                if (lastSelectionResult != 0) {
                    menuState = 0x384;
                    stateAction = 0;
                }
                else {
                    menuState = 0x385;
                    stateAction = 1;
                }
                break;
            case 0x12C:
                lastSelectionResult = GetActivePlayerCount();
                if (lastSelectionResult != 0) {
                    menuState = 0x12D;
                    stateAction = 0;
                }
                else {
                    menuState = 0x385;
                    stateAction = 1;
                }
                break;
            case 0x12D:
                twoPlayerSelectionResult = HandleCharacterSelectionInput(playerLimit, 2);
                lastSelectionResult = twoPlayerSelectionResult;
                if (lastSelectionResult == 1) {
                    menuState = 0x12E;
                    stateAction = 0;
                }
                else if (lastSelectionResult == 2) {
                    menuState = 0x384;
                    stateAction = 0;
                }
                else {
                    menuState = 0x12C;
                    stateAction = 1;
                }
                break;
            case 0x12E:
                twoPlayerCharacterResult = GetCharacterSelectionResult(computerPlayerIndex, 2);
                lastSelectionResult = twoPlayerCharacterResult;
                if (lastSelectionResult != 0) {
                    menuState = 0x384;
                    stateAction = 0;
                }
                else {
                    menuState = 0x12D;
                    stateAction = 1;
                }
                break;
            case 0x190:
                fourPlayerCharacterSelectionResult = RunPlayerSelectionMenu(0);
                lastSelectionResult = fourPlayerCharacterSelectionResult;
                if (lastSelectionResult != 0) {
                    menuState = 0x191;
                    stateAction = 0;
                }
                else {
                    menuState = 0x385;
                    stateAction = 1;
                }
                break;
            case 0x191:
                lastSelectionResult = GetCharacterSelectionIndex(stateAction);
                if (lastSelectionResult == 1) {
                    menuState = 0x192;
                    stateAction = 0;
                }
                else if (lastSelectionResult == 2) {
                    menuState = 0x384;
                    stateAction = 0;
                }
                else {
                    menuState = 0x190;
                    stateAction = 1;
                }
                break;
            case 0x192:
                playerLimit = GetPlayerCharacterSelection(computerPlayerIndex);
                if (lastSelectionResult != 0) {
                    menuState = 0x384;
                    stateAction = 0;
                }
                else {
                    menuState = 0x191;
                    stateAction = 1;
                }
                break;
            case 0x384:
                UpdateCharacterSelection(selectionMode);
                selectionResult = 1;
                lastSelectionResult = 0x3E7;
                break;
            case 0x385:
                selectionResult = 0;
                lastSelectionResult = 0x3E7;
                break;
        }
    }
    return selectionResult;
}

void AnimateCharacterSelectionOut(s32 selectionMode)
{
    ShowCharacterDifficultySprites(selectionMode);
}

void AnimateCharacterSelectionIn(s32 selectionMode)
{
    HideCharacterDifficultySpritesAnimated(selectionMode);
}

void CreateTeamSelectionSprites(void)
{
    s32 spriteIndex;
    s32 spriteGroupId;
    s32 spriteId;
    ANIMDATA *spriteAnimation;

    spriteGroupId = HuSprGrpCreate(0xB);
    selectionSpriteGroups[0] = spriteGroupId;
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x46)), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    HuSprPosSet(spriteGroupId, 0, 0.0f, 0.0f);
    spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x47));
    for (spriteIndex = 0; spriteIndex < 2; spriteIndex++) {
        spriteId = HuSprCreate(spriteAnimation, 0x3E8, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 1, spriteId);
        HuSprBankSet(spriteGroupId, spriteIndex + 1, spriteIndex);
    }
    spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x48));
    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        spriteId = HuSprCreate(spriteAnimation, 0x76C, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 3, spriteId);
        HuSprPosSet(spriteGroupId, spriteIndex + 3, playerCountIconPositions[spriteIndex % 4][0], playerCountIconPositions[0][1]);
        HuSprBankSet(spriteGroupId, spriteIndex + 3, 0);
    }
    HideTeamSelectionSprites();
    ShowTeamSelectionSprites();
}

void HideTeamSelectionSprites(void)
{
    s32 playerIndex;
    s32 teamSpriteGroup;

    teamSpriteGroup = selectionSpriteGroups[0];
    for (playerIndex = 0; playerIndex < 0xB; playerIndex++) {
        HuSprAttrSet(teamSpriteGroup, playerIndex, 4);
    }
    HuSprAttrReset(teamSpriteGroup, 0, 4);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        HuSprAttrReset(teamSpriteGroup, playerIndex + 3, 4);
        HuSprPosSet(teamSpriteGroup, playerIndex + 3, playerCountIconPositions[playerIndex % 4][0], playerCountIconPositions[0][1]);
        HuSprBankSet(teamSpriteGroup, playerIndex + 3, 0);
    }
    HuSprGrpPosSet(teamSpriteGroup, 288.0f, -390.0f);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSelectionWork[playerIndex].playerActive = playerSelectionWork[playerIndex].isComputer = 0;
    }
}

void HideTwoPlayerSelectionSprites(void)
{
    s32 playerIndex;
    s32 teamSpriteGroup;

    teamSpriteGroup = selectionSpriteGroups[0];
    for (playerIndex = 0; playerIndex < 0xB; playerIndex++) {
        HuSprAttrSet(teamSpriteGroup, playerIndex, 4);
    }
    HuSprAttrReset(teamSpriteGroup, 0, 4);
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        HuSprAttrReset(teamSpriteGroup, playerIndex + 3, 4);
        HuSprPosSet(teamSpriteGroup, playerIndex + 3, playerCountIconPositions[playerIndex % 4 + 1][0], playerCountIconPositions[0][1]);
        HuSprBankSet(teamSpriteGroup, playerIndex + 3, 0);
    }
    HuSprGrpPosSet(teamSpriteGroup, 288.0f, -390.0f);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSelectionWork[playerIndex].playerActive = playerSelectionWork[playerIndex].isComputer = 0;
    }
}

void ShowTeamSelectionSprites(void)
{
    s32 loopIndex;
    s32 teamSpriteGroup;

    teamSpriteGroup = selectionSpriteGroups[0];
    for (loopIndex = 0; loopIndex < 0xB; loopIndex++) {
        HuSprAttrSet(teamSpriteGroup, loopIndex, 4);
    }
    HuSprGrpPosSet(teamSpriteGroup, 288.0f, 110.0f);
    HuSprGrpScaleSet(teamSpriteGroup, 1.0f, 1.0f);
}

void AnimateTeamSelectionIn(void)
{
    float animationProgress;
    s32 playerIndex;
    s32 animationFrame;
    s32 teamSpriteGroup;
    s32 positionedTeamSpriteGroup;

    positionedTeamSpriteGroup = selectionSpriteGroups[0];
    teamSpriteGroup = selectionSpriteGroups[0];
    for (playerIndex = 0; playerIndex < 0xB; playerIndex++) {
        HuSprAttrSet(teamSpriteGroup, playerIndex, 4);
    }
    HuSprAttrReset(teamSpriteGroup, 0, 4);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        HuSprAttrReset(teamSpriteGroup, playerIndex + 3, 4);
        HuSprPosSet(teamSpriteGroup, playerIndex + 3, playerCountIconPositions[playerIndex % 4][0], playerCountIconPositions[0][1]);
        HuSprBankSet(teamSpriteGroup, playerIndex + 3, 0);
    }
    HuSprGrpPosSet(teamSpriteGroup, 288.0f, -390.0f);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSelectionWork[playerIndex].playerActive = playerSelectionWork[playerIndex].isComputer = 0;
    }
    for (animationFrame = 0; animationFrame <= 0x3C; animationFrame++) {
        SleepSelectionProcess(1);
        if (animationFrame <= 0x32) {
            animationProgress = InterpolateEaseIn(-500.0f, 10.0f, animationFrame, 50.0f);
        }
        else {
            animationProgress = InterpolateEaseOut(10.0f, 0.0f, animationFrame - 0x32, 10.0f);
        }
        HuSprGrpPosSet(positionedTeamSpriteGroup, 288.0f, 110.0f + animationProgress);
    }
    HuSprGrpPosSet(positionedTeamSpriteGroup, 288.0f, 110.0f);
    SleepSelectionProcess(0xA);
}

void AnimateTwoPlayerSelectionIn(void)
{
    float animationProgress;
    s32 playerIndex;
    s32 animationFrame;
    s32 teamSpriteGroup;
    s32 positionedTeamSpriteGroup;

    positionedTeamSpriteGroup = selectionSpriteGroups[0];
    teamSpriteGroup = selectionSpriteGroups[0];
    for (playerIndex = 0; playerIndex < 0xB; playerIndex++) {
        HuSprAttrSet(teamSpriteGroup, playerIndex, 4);
    }
    HuSprAttrReset(teamSpriteGroup, 0, 4);
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        HuSprAttrReset(teamSpriteGroup, playerIndex + 3, 4);
        HuSprPosSet(teamSpriteGroup, playerIndex + 3, playerCountIconPositions[playerIndex % 4 + 1][0], playerCountIconPositions[0][1]);
        HuSprBankSet(teamSpriteGroup, playerIndex + 3, 0);
    }
    HuSprGrpPosSet(teamSpriteGroup, 288.0f, -390.0f);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSelectionWork[playerIndex].playerActive = playerSelectionWork[playerIndex].isComputer = 0;
    }
    for (animationFrame = 0; animationFrame <= 0x3C; animationFrame++) {
        SleepSelectionProcess(1);
        if (animationFrame <= 0x32) {
            animationProgress = InterpolateEaseIn(-500.0f, 10.0f, animationFrame, 50.0f);
        }
        else {
            animationProgress = InterpolateEaseOut(10.0f, 0.0f, animationFrame - 0x32, 10.0f);
        }
        HuSprGrpPosSet(positionedTeamSpriteGroup, 288.0f, 110.0f + animationProgress);
    }
    HuSprGrpPosSet(positionedTeamSpriteGroup, 288.0f, 110.0f);
    SleepSelectionProcess(0xA);
}
void AnimateTeamSelectionOut(void)
{
    float animationProgress;
    s32 loopIndex;
    s32 spriteIndex;
    s32 teamSpriteGroup;
    s32 positionedTeamSpriteGroup;

    positionedTeamSpriteGroup = selectionSpriteGroups[0];
    SleepSelectionProcess(0xA);
    for (loopIndex = 0; loopIndex <= 0x3C; loopIndex++) {
        SleepSelectionProcess(1);
        if (loopIndex <= 0xA) {
            animationProgress = InterpolateEaseIn(0.0f, 10.0f, loopIndex, 10.0f);
        }
        else {
            animationProgress = InterpolateEaseOut(10.0f, -500.0f, loopIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(positionedTeamSpriteGroup, 288.0f, 110.0f + animationProgress);
    }
    HuSprGrpPosSet(positionedTeamSpriteGroup, 288.0f, -390.0f);
    teamSpriteGroup = selectionSpriteGroups[0];
    for (spriteIndex = 0; spriteIndex < 0xB; spriteIndex++) {
        HuSprAttrSet(teamSpriteGroup, spriteIndex, 4);
    }
    HuSprGrpPosSet(teamSpriteGroup, 288.0f, 110.0f);
    HuSprGrpScaleSet(teamSpriteGroup, 1.0f, 1.0f);
}

void PositionTeamSelectionSprites(s32 playerCount)
{
    s32 teamSpriteGroup = selectionSpriteGroups[0];
    HuSprAttrReset(teamSpriteGroup, 1, 4);
    HuSprAttrReset(teamSpriteGroup, 2, 4);
    HuSprPosSet(teamSpriteGroup, 1, -155.0f, 0.0f);
    HuSprPosSet(teamSpriteGroup, 2, 155.0f, 0.0f);
    if (playerCount == 2) {
        HuSprPosSet(teamSpriteGroup, 1, -80.0f, 0.0f);
        HuSprPosSet(teamSpriteGroup, 2, 80.0f, 0.0f);
    }
}

void HideTeamSelectionPlayers(void)
{
    s32 teamSpriteGroup = selectionSpriteGroups[0];
    HuSprAttrSet(teamSpriteGroup, 1, 4);
    HuSprAttrSet(teamSpriteGroup, 2, 4);
}

void UpdateTeamCountSelection(OMOBJ *selectionObject, ZtarPlayerSelectionWork *selectionWork)
{
    float slideOffset;
    float playerSpriteScale;
    s32 playerIndex;
    s32 playerIndex;
    s32 targetHumanCount;
    s32 connectedControllerCount;
    s32 teamSpriteGroup;
    s32 teamCountDirection;
    s32 teamArrowSpriteGroup;
    s32 teamCountBoundaryReached;
    s32 teamArrowSpriteGroup;

    teamSpriteGroup = selectionSpriteGroups[0];
    targetHumanCount = connectedControllerCount = teamCountDirection = teamCountBoundaryReached = 0;
    teamSelectionWork.teamCountTransitionActive = 0;
    targetHumanCount = 1;
    for (playerIndex = 1; playerIndex < 4; playerIndex++) {
        if (playerSelectionWork[playerIndex].isComputer == 0) {
            targetHumanCount++;
        }
    }
    connectedControllerCount = 1;
    for (playerIndex = 1; playerIndex < 4; playerIndex++) {
        if (HuPadStatGet(playerIndex) != -1) {
            connectedControllerCount++;
        }
    }
    if (connectedControllerCount >= 2) {
        teamArrowSpriteGroup = selectionSpriteGroups[0];
        HuSprAttrReset(teamArrowSpriteGroup, 1, 4);
        HuSprAttrReset(teamArrowSpriteGroup, 2, 4);
        HuSprPosSet(teamArrowSpriteGroup, 1, -155.0f, 0.0f);
        HuSprPosSet(teamArrowSpriteGroup, 2, 155.0f, 0.0f);
        if (HuPadStkX[0] >= 5) {
            targetHumanCount++;
            teamCountDirection = 1;
        }
        else if (HuPadStkX[0] <= -5) {
            targetHumanCount--;
            teamCountDirection = -1;
        }
    }
    else {
        teamArrowSpriteGroup = selectionSpriteGroups[0];
        HuSprAttrSet(teamArrowSpriteGroup, 1, 4);
        HuSprAttrSet(teamArrowSpriteGroup, 2, 4);
    }
    if (targetHumanCount < 1) {
        targetHumanCount = connectedControllerCount;
    }
    else if (targetHumanCount > connectedControllerCount) {
        if (teamCountDirection == 0) {
            targetHumanCount = connectedControllerCount;
            teamCountBoundaryReached = 1;
        }
        else {
            targetHumanCount = 1;
        }
    }
    if ((teamCountDirection != 0) || (teamCountBoundaryReached != 0)) {
        teamSelectionWork.teamCountTransitionActive = 1;
        HuAudFXPlay(0);
        for (playerIndex = 0; playerIndex <= 0xA; playerIndex++) {
            if (playerIndex <= 5) {
                slideOffset = InterpolateEaseIn(0.0f, 10.0f, playerIndex, 5.0f);
                playerSpriteScale = InterpolateEaseIn(1.0f, 0.0f, playerIndex, 5.0f);
            }
            else {
                slideOffset = InterpolateEaseIn(10.0f, 0.0f, playerIndex - 5, 5.0f);
                playerSpriteScale = InterpolateEaseIn(0.0f, 1.0f, playerIndex - 5, 5.0f);
            }
            if (teamCountDirection == 1) {
                HuSprPosSet(teamSpriteGroup, 2, 3.0f + (155.0f + slideOffset), 0.0f);
            }
            else if (teamCountDirection == -1) {
                HuSprPosSet(teamSpriteGroup, 1, (-155.0f - slideOffset) - 3.0f, 0.0f);
            }
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                if ((playerIndex < targetHumanCount) && (playerSelectionWork[playerIndex].isComputer == 1)) {
                    playerSelectionWork[playerIndex].playerActive = 0;
                    HuSprScaleSet(teamSpriteGroup, playerIndex + 3, playerSpriteScale, 1.0f);
                    if (playerIndex == 5) {
                        HuSprBankSet(teamSpriteGroup, playerIndex + 3, 0);
                    }
                }
                else if ((playerIndex >= targetHumanCount) && (playerSelectionWork[playerIndex].isComputer == 0)) {
                    playerSelectionWork[playerIndex].playerActive = 1;
                    HuSprScaleSet(teamSpriteGroup, playerIndex + 3, playerSpriteScale, 1.0f);
                    if (playerIndex == 5) {
                        HuSprBankSet(teamSpriteGroup, playerIndex + 3, 1);
                    }
                }
            }
            SleepSelectionProcess(1);
        }
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerIndex < targetHumanCount) {
                playerSelectionWork[playerIndex].isComputer = 0;
            }
            else {
                playerSelectionWork[playerIndex].isComputer = 1;
            }
        }
    }
    teamSelectionWork.teamCountTransitionActive = 0;
}

void UpdateTwoPlayerTeamCountSelection(OMOBJ *selectionObject, ZtarPlayerSelectionWork *selectionWork)
{
    float slideOffset;
    float playerSpriteScale;
    s32 playerIndex;
    s32 playerIndex;
    s32 targetHumanCount;
    s32 connectedControllerCount;
    s32 teamSpriteGroup;
    s32 teamCountDirection;
    s32 teamArrowSpriteGroup;
    s32 teamCountBoundaryReached;
    s32 teamArrowSpriteGroup;

    teamSpriteGroup = selectionSpriteGroups[0];
    targetHumanCount = connectedControllerCount = teamCountDirection = teamCountBoundaryReached = 0;
    teamSelectionWork.teamCountTransitionActive = 0;
    targetHumanCount = 1;
    for (playerIndex = 1; playerIndex < 2; playerIndex++) {
        if (playerSelectionWork[playerIndex].isComputer == 0) {
            targetHumanCount++;
        }
    }
    connectedControllerCount = 1;
    for (playerIndex = 1; playerIndex < 4; playerIndex++) {
        if (HuPadStatGet(playerIndex) != -1) {
            connectedControllerCount++;
        }
    }
    if (connectedControllerCount > 2) {
        connectedControllerCount = 2;
    }
    if (connectedControllerCount >= 2) {
        teamArrowSpriteGroup = selectionSpriteGroups[0];
        HuSprAttrReset(teamArrowSpriteGroup, 1, 4);
        HuSprAttrReset(teamArrowSpriteGroup, 2, 4);
        HuSprPosSet(teamArrowSpriteGroup, 1, -155.0f, 0.0f);
        HuSprPosSet(teamArrowSpriteGroup, 2, 155.0f, 0.0f);
        HuSprPosSet(teamArrowSpriteGroup, 1, -80.0f, 0.0f);
        HuSprPosSet(teamArrowSpriteGroup, 2, 80.0f, 0.0f);
        if (HuPadStkX[0] >= 5) {
            targetHumanCount++;
            teamCountDirection = 1;
        }
        else if (HuPadStkX[0] <= -5) {
            targetHumanCount--;
            teamCountDirection = -1;
        }
    }
    else {
        teamArrowSpriteGroup = selectionSpriteGroups[0];
        HuSprAttrSet(teamArrowSpriteGroup, 1, 4);
        HuSprAttrSet(teamArrowSpriteGroup, 2, 4);
    }
    if (targetHumanCount < 1) {
        targetHumanCount = connectedControllerCount;
    }
    else if (targetHumanCount > connectedControllerCount) {
        if (teamCountDirection == 0) {
            targetHumanCount = connectedControllerCount;
            teamCountBoundaryReached = 1;
        }
        else {
            targetHumanCount = 1;
        }
    }
    if ((teamCountDirection != 0) || (teamCountBoundaryReached != 0)) {
        teamSelectionWork.teamCountTransitionActive = 1;
        HuAudFXPlay(0);
        for (playerIndex = 0; playerIndex <= 0xA; playerIndex++) {
            if (playerIndex <= 5) {
                slideOffset = InterpolateEaseIn(0.0f, 10.0f, playerIndex, 5.0f);
                playerSpriteScale = InterpolateEaseIn(1.0f, 0.0f, playerIndex, 5.0f);
            }
            else {
                slideOffset = InterpolateEaseIn(10.0f, 0.0f, playerIndex - 5, 5.0f);
                playerSpriteScale = InterpolateEaseIn(0.0f, 1.0f, playerIndex - 5, 5.0f);
            }
            if (teamCountDirection == 1) {
                HuSprPosSet(teamSpriteGroup, 2, 3.0f + (77.0f + slideOffset), 0.0f);
            }
            else if (teamCountDirection == -1) {
                HuSprPosSet(teamSpriteGroup, 1, (-77.0f - slideOffset) - 3.0f, 0.0f);
            }
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                if ((playerIndex < targetHumanCount) && (playerSelectionWork[playerIndex].isComputer == 1)) {
                    playerSelectionWork[playerIndex].playerActive = 0;
                    HuSprScaleSet(teamSpriteGroup, playerIndex + 3, playerSpriteScale, 1.0f);
                    if (playerIndex == 5) {
                        HuSprBankSet(teamSpriteGroup, playerIndex + 3, 0);
                    }
                }
                else if ((playerIndex >= targetHumanCount) && (playerSelectionWork[playerIndex].isComputer == 0)) {
                    playerSelectionWork[playerIndex].playerActive = 1;
                    HuSprScaleSet(teamSpriteGroup, playerIndex + 3, playerSpriteScale, 1.0f);
                    if (playerIndex == 5) {
                        HuSprBankSet(teamSpriteGroup, playerIndex + 3, 1);
                    }
                }
            }
            SleepSelectionProcess(1);
        }
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerIndex < targetHumanCount) {
                playerSelectionWork[playerIndex].isComputer = 0;
            }
            else {
                playerSelectionWork[playerIndex].isComputer = 1;
            }
        }
    }
    teamSelectionWork.teamCountTransitionActive = 0;
}

void CreateCharacterSelectionSprites(void)
{
    s32 characterIndex;
    s32 spriteId;
    s32 spriteGroupId;
    ANIMDATA *spriteAnimation;
    ANIMDATA *spriteIndex;

    spriteGroupId = HuSprGrpCreate(0x36);
    selectionSpriteGroups[1] = spriteGroupId;
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x4A)), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    HuSprPosSet(spriteGroupId, 0, 0.0f, 0.0f);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x4B)), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId, 1, spriteId);
    HuSprPosSet(spriteGroupId, 1, 0.0f, 0.0f);
    spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x5A));
    for (characterIndex = 0; characterIndex < 0xE; characterIndex++) {
        spriteId = HuSprCreate(HuSprAnimReadFile(characterIndex + DATA_MAKE_NUM(DATADIR_MPEX, 0x4C)), 0x76C, 0);
        HuSprGrpMemberSet(spriteGroupId, characterIndex + 2, spriteId);
    }
    for (characterIndex = 0; characterIndex < 4; characterIndex++) {
        spriteId = HuSprCreate(HuSprAnimReadFile(characterIndex + DATA_MAKE_NUM(DATADIR_MPEX, 0x5B)), 0x384, 0);
        HuSprGrpMemberSet(spriteGroupId, characterIndex + 0x1E, spriteId);
        spriteId = HuSprCreate(HuSprAnimReadFile(characterIndex + DATA_MAKE_NUM(DATADIR_MPEX, 0x60)), 0x3E8, 0);
        HuSprGrpMemberSet(spriteGroupId, characterIndex + 0x22, spriteId);
    }
    spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x5F));
    spriteIndex = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x64));
    for (characterIndex = 0; characterIndex < 4; characterIndex++) {
        spriteId = HuSprCreate(spriteAnimation, 0x384, 0);
        HuSprGrpMemberSet(spriteGroupId, characterIndex + 0x26, spriteId);
        spriteId = HuSprCreate(spriteIndex, 0x3E8, 0);
        HuSprGrpMemberSet(spriteGroupId, characterIndex + 0x2A, spriteId);
    }
    spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x65));
    spriteIndex = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x66));
    for (characterIndex = 0; characterIndex < 4; characterIndex++) {
        spriteId = HuSprCreate(spriteAnimation, 0x384, 0);
        HuSprGrpMemberSet(spriteGroupId, characterIndex + 0x2E, spriteId);
        spriteId = HuSprCreate(spriteIndex, 0x320, 0);
        HuSprGrpMemberSet(spriteGroupId, characterIndex + 0x32, spriteId);
    }
    PrepareCharacterGridSprites(0);
    HideCharacterGridSprites(0);
}

void PrepareCharacterGridSprites(s32 gridStyle)
{
    s32 characterIndex;
    s32 characterSpriteGroup;

    characterSpriteGroup = selectionSpriteGroups[1];
    for (characterIndex = 0; characterIndex < 0x36; characterIndex++) {
        HuSprAttrSet(characterSpriteGroup, characterIndex, 4);
    }
    if (gridStyle != 3) {
        HuSprAttrReset(characterSpriteGroup, 0, 4);
        for (characterIndex = 0; characterIndex < 8; characterIndex++) {
            HuSprBankSet(characterSpriteGroup, characterIndex + 2, 0);
            HuSprAttrReset(characterSpriteGroup, characterIndex + 2, 4);
            HuSprPosSet(characterSpriteGroup, characterIndex + 2, characterGridPositions[characterIndex][0], characterGridPositions[characterIndex][1]);
            HuSprBankSet(characterSpriteGroup, characterIndex + 2, 0);
        }
    }
    else {
        HuSprAttrReset(characterSpriteGroup, 1, 4);
        HuSprPosSet(characterSpriteGroup, 1, 2.0f, 0.0f);
        for (characterIndex = 0; characterIndex < 0xE; characterIndex++) {
            HuSprBankSet(characterSpriteGroup, characterIndex + 2, 0);
            HuSprAttrReset(characterSpriteGroup, characterIndex + 2, 4);
            HuSprPosSet(characterSpriteGroup, characterIndex + 2, wideCharacterGridPositions[characterIndex][0], wideCharacterGridPositions[characterIndex][1]);
            HuSprBankSet(characterSpriteGroup, characterIndex + 2, 0);
        }
    }
    HuSprGrpPosSet(characterSpriteGroup, 288.0f, -331.0f);
}

void HideCharacterGridSprites(s32 gridStyle)
{
    s32 characterIndex;
    s32 characterSpriteGroup = selectionSpriteGroups[1];
    for (characterIndex = 0; characterIndex < 0x36; characterIndex++) {
        HuSprAttrSet(characterSpriteGroup, characterIndex, 4);
    }
    HuSprGrpPosSet(characterSpriteGroup, 288.0f, -331.0f);
}
void AnimateCharacterGridIn(s32 gridStyle, s32 difficultyLayout)
{
    float animationProgress;
    s32 characterIndex;
    s32 characterSpriteGroup;
    s32 animationFrame;
    s32 positionedCharacterSpriteGroup;

    positionedCharacterSpriteGroup = selectionSpriteGroups[1];
    characterSpriteGroup = selectionSpriteGroups[1];
    for (characterIndex = 0; characterIndex < 0x36; characterIndex++) {
        HuSprAttrSet(characterSpriteGroup, characterIndex, 4);
    }
    if (gridStyle != 3) {
        HuSprAttrReset(characterSpriteGroup, 0, 4);
        for (characterIndex = 0; characterIndex < 8; characterIndex++) {
            HuSprBankSet(characterSpriteGroup, characterIndex + 2, 0);
            HuSprAttrReset(characterSpriteGroup, characterIndex + 2, 4);
            HuSprPosSet(characterSpriteGroup, characterIndex + 2, characterGridPositions[characterIndex][0], characterGridPositions[characterIndex][1]);
            HuSprBankSet(characterSpriteGroup, characterIndex + 2, 0);
        }
    }
    else {
        HuSprAttrReset(characterSpriteGroup, 1, 4);
        HuSprPosSet(characterSpriteGroup, 1, 2.0f, 0.0f);
        for (characterIndex = 0; characterIndex < 0xE; characterIndex++) {
            HuSprBankSet(characterSpriteGroup, characterIndex + 2, 0);
            HuSprAttrReset(characterSpriteGroup, characterIndex + 2, 4);
            HuSprPosSet(characterSpriteGroup, characterIndex + 2, wideCharacterGridPositions[characterIndex][0], wideCharacterGridPositions[characterIndex][1]);
            HuSprBankSet(characterSpriteGroup, characterIndex + 2, 0);
        }
    }
    HuSprGrpPosSet(characterSpriteGroup, 288.0f, -331.0f);
    if (difficultyLayout == 1) {
        SetCharacterGridLayout();
    }
    if (difficultyLayout == 2) {
        SetCharacterGridLayoutAlternate();
    }
    if (difficultyLayout == 3) {
        SetCharacterGridLayoutCompact();
    }
    for (animationFrame = 0; animationFrame <= 0x3C; animationFrame++) {
        SleepSelectionProcess(1);
        if (animationFrame <= 0x32) {
            animationProgress = InterpolateEaseIn(-500.0f, 10.0f, animationFrame, 50.0f);
        }
        else {
            animationProgress = InterpolateEaseOut(10.0f, 0.0f, animationFrame - 0x32, 10.0f);
        }
        if (wideCharacterGridEnabled != 0) {
            HuSprGrpPosSet(positionedCharacterSpriteGroup, 288.0f, 20.0f + (169.0f + animationProgress));
        }
        else {
            HuSprGrpPosSet(positionedCharacterSpriteGroup, 288.0f, 169.0f + animationProgress);
        }
    }
    if (wideCharacterGridEnabled != 0) {
        HuSprGrpPosSet(positionedCharacterSpriteGroup, 288.0f, 189.0f);
        return;
    }
    HuSprGrpPosSet(positionedCharacterSpriteGroup, 288.0f, 169.0f);
}

void AnimateCharacterGridInAlternate(s32 gridStyle)
{
    float animationProgress;
    s32 characterIndex;
    s32 characterSpriteGroup;
    s32 animationFrame;
    s32 positionedCharacterSpriteGroup;

    positionedCharacterSpriteGroup = selectionSpriteGroups[1];
    characterSpriteGroup = selectionSpriteGroups[1];
    for (characterIndex = 0; characterIndex < 0x36; characterIndex++) {
        HuSprAttrSet(characterSpriteGroup, characterIndex, 4);
    }
    if (gridStyle != 3) {
        HuSprAttrReset(characterSpriteGroup, 0, 4);
        for (characterIndex = 0; characterIndex < 8; characterIndex++) {
            HuSprBankSet(characterSpriteGroup, characterIndex + 2, 0);
            HuSprAttrReset(characterSpriteGroup, characterIndex + 2, 4);
            HuSprPosSet(characterSpriteGroup, characterIndex + 2, characterGridPositions[characterIndex][0], characterGridPositions[characterIndex][1]);
            HuSprBankSet(characterSpriteGroup, characterIndex + 2, 0);
        }
    }
    else {
        HuSprAttrReset(characterSpriteGroup, 1, 4);
        HuSprPosSet(characterSpriteGroup, 1, 2.0f, 0.0f);
        for (characterIndex = 0; characterIndex < 0xE; characterIndex++) {
            HuSprBankSet(characterSpriteGroup, characterIndex + 2, 0);
            HuSprAttrReset(characterSpriteGroup, characterIndex + 2, 4);
            HuSprPosSet(characterSpriteGroup, characterIndex + 2, wideCharacterGridPositions[characterIndex][0], wideCharacterGridPositions[characterIndex][1]);
            HuSprBankSet(characterSpriteGroup, characterIndex + 2, 0);
        }
    }
    HuSprGrpPosSet(characterSpriteGroup, 288.0f, -331.0f);
    for (animationFrame = 0; animationFrame <= 0x3C; animationFrame++) {
        SleepSelectionProcess(1);
        if (animationFrame <= 0x32) {
            animationProgress = InterpolateEaseIn(-500.0f, 10.0f, animationFrame, 50.0f);
        }
        else {
            animationProgress = InterpolateEaseOut(10.0f, 0.0f, animationFrame - 0x32, 10.0f);
        }
        if (wideCharacterGridEnabled != 0) {
            HuSprGrpPosSet(positionedCharacterSpriteGroup, 288.0f, 20.0f + (169.0f + animationProgress));
        }
        else {
            HuSprGrpPosSet(positionedCharacterSpriteGroup, 288.0f, 169.0f + animationProgress);
        }
    }
    if (wideCharacterGridEnabled != 0) {
        HuSprGrpPosSet(positionedCharacterSpriteGroup, 288.0f, 189.0f);
        return;
    }
    HuSprGrpPosSet(positionedCharacterSpriteGroup, 288.0f, 169.0f);
}

void AnimateCharacterGridOut(s32 gridStyle)
{
    float animationProgress;
    s32 characterIndex;
    s32 spriteIndex;
    s32 characterSpriteGroup;
    s32 positionedCharacterSpriteGroup;

    characterSpriteGroup = selectionSpriteGroups[1];
    for (characterIndex = 0; characterIndex <= 0x3C; characterIndex++) {
        SleepSelectionProcess(1);
        if (characterIndex <= 0xA) {
            animationProgress = InterpolateEaseIn(0.0f, 10.0f, characterIndex, 10.0f);
        }
        else {
            animationProgress = InterpolateEaseOut(10.0f, -500.0f, characterIndex - 0xA, 50.0f);
        }
        if (wideCharacterGridEnabled != 0) {
            HuSprGrpPosSet(characterSpriteGroup, 288.0f, 20.0f + (169.0f + animationProgress));
        }
        else {
            HuSprGrpPosSet(characterSpriteGroup, 288.0f, 169.0f + animationProgress);
        }
    }
    HuSprGrpPosSet(characterSpriteGroup, 288.0f, -331.0f);
    positionedCharacterSpriteGroup = selectionSpriteGroups[1];
    for (spriteIndex = 0; spriteIndex < 0x36; spriteIndex++) {
        HuSprAttrSet(positionedCharacterSpriteGroup, spriteIndex, 4);
    }
    HuSprGrpPosSet(positionedCharacterSpriteGroup, 288.0f, -331.0f);
}

void ResetCharacterGridPositions(void)
{
    float characterX;
    float characterY;
    s32 secondaryPlayerIndex;
    s32 characterSpriteGroup;
    s32 playerIndex;

    characterSpriteGroup = selectionSpriteGroups[1];
    for (secondaryPlayerIndex = 0; secondaryPlayerIndex < 4; secondaryPlayerIndex++) {
        HuSprBankSet(characterSpriteGroup, secondaryPlayerIndex + 0x1E, 0);
        HuSprBankSet(characterSpriteGroup, secondaryPlayerIndex + 0x22, 0);
        HuSprBankSet(characterSpriteGroup, secondaryPlayerIndex + 0x26, 0);
        HuSprBankSet(characterSpriteGroup, secondaryPlayerIndex + 0x2A, 0);
        HuSprBankSet(characterSpriteGroup, secondaryPlayerIndex + 0x2E, 0);
        HuSprBankSet(characterSpriteGroup, secondaryPlayerIndex + 0x32, 0);
    }
    if (wideCharacterGridEnabled != 0) {
        for (secondaryPlayerIndex = 0; secondaryPlayerIndex < 0xE; secondaryPlayerIndex++) {
            HuSprBankSet(characterSpriteGroup, secondaryPlayerIndex + 2, 0);
        }
    }
    else {
        for (secondaryPlayerIndex = 0; secondaryPlayerIndex < 8; secondaryPlayerIndex++) {
            HuSprBankSet(characterSpriteGroup, secondaryPlayerIndex + 2, 0);
        }
    }
    for (secondaryPlayerIndex = 0; secondaryPlayerIndex <= 0xA; secondaryPlayerIndex++) {
        SleepSelectionProcess(1);
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (wideCharacterGridEnabled != 0) {
                characterX = InterpolateEaseIn(alternateCharacterGridPositions[playerSelectionWork[playerIndex].character][0], alternateCharacterGridPositions[playerIndex + 1][0], secondaryPlayerIndex, 10.0f);
                characterY = InterpolateEaseIn(alternateCharacterGridPositions[playerSelectionWork[playerIndex].character][1], alternateCharacterGridPositions[1][1], secondaryPlayerIndex, 10.0f);
            }
            else {
                characterX = InterpolateEaseIn(characterGridPositions[playerSelectionWork[playerIndex].character % 4][0], characterGridPositions[playerIndex][0], secondaryPlayerIndex, 10.0f);
                characterY = InterpolateEaseIn(characterGridPositions[(playerSelectionWork[playerIndex].character / 4) * 4][1], characterGridPositions[0][1], secondaryPlayerIndex, 10.0f);
            }
            HuSprPosSet(characterSpriteGroup, playerIndex + 0x1E, characterX, characterY);
            HuSprPosSet(characterSpriteGroup, playerIndex + 0x22, characterX, characterY);
        }
    }
    for (secondaryPlayerIndex = 0; secondaryPlayerIndex < 4; secondaryPlayerIndex++) {
        if (wideCharacterGridEnabled != 0) {
            HuSprPosSet(characterSpriteGroup, secondaryPlayerIndex + 0x1E, *alternateCharacterGridPositions[secondaryPlayerIndex + 1], alternateCharacterGridPositions[secondaryPlayerIndex + 1][1]);
            HuSprPosSet(characterSpriteGroup, secondaryPlayerIndex + 0x22, *alternateCharacterGridPositions[secondaryPlayerIndex + 1], alternateCharacterGridPositions[secondaryPlayerIndex + 1][1]);
            playerSelectionWork[secondaryPlayerIndex].character = secondaryPlayerIndex + 1;
            playerSelectionWork[secondaryPlayerIndex].playerIndex = secondaryPlayerIndex;
            playerSelectionWork[secondaryPlayerIndex].selectionState = 0;
        }
        else {
            HuSprPosSet(characterSpriteGroup, secondaryPlayerIndex + 0x1E, characterGridPositions[secondaryPlayerIndex][0], characterGridPositions[secondaryPlayerIndex][1]);
            HuSprPosSet(characterSpriteGroup, secondaryPlayerIndex + 0x22, characterGridPositions[secondaryPlayerIndex][0], characterGridPositions[secondaryPlayerIndex][1]);
            playerSelectionWork[secondaryPlayerIndex].playerIndex = secondaryPlayerIndex;
            playerSelectionWork[secondaryPlayerIndex].character = secondaryPlayerIndex;
            playerSelectionWork[secondaryPlayerIndex].selectionState = 0;
        }
    }
}

void UpdatePlayerCharacterCursor(OMOBJ *selectionObject, ZtarPlayerSelectionWork *playerSelection)
{
    float cursorX;
    float cursorY;
    s32 columnIndex;
    s32 rowIndex;
    s32 otherPlayerIndex;
    s32 horizontalMove;
    s32 verticalMove;
    s32 candidateCharacterIndex;
    s32 characterSpriteGroup;
    s32 originalColumnIndex;
    s32 originalRowIndex;
    s32 selectedCompactCharacterIndex;
    s32 previousCompactCharacterIndex;
    s32 previousCharacterIndex;

    characterSpriteGroup = selectionSpriteGroups[1];
    horizontalMove = 0;
    verticalMove = 0;
    HuSprAttrReset(characterSpriteGroup, playerSelection->playerIndex + 0x1E, 4);
    HuSprAttrReset(characterSpriteGroup, playerSelection->playerIndex + 0x22, 4);
    if (wideCharacterGridEnabled != 0) {
        columnIndex = playerSelection->character % 6;
        originalColumnIndex = columnIndex;
        rowIndex = playerSelection->character / 6;
        originalRowIndex = rowIndex;
    }
    else {
        columnIndex = playerSelection->character % 4;
        originalColumnIndex = columnIndex;
        rowIndex = playerSelection->character / 4;
        originalRowIndex = rowIndex;
    }
    if (playerSelection->selectionState == 0) {
        if (HuPadBtnDown[playerSelection->playerIndex] & PAD_BUTTON_A) {
            playerSelection->selectionState = 1;
            if (wideCharacterGridEnabled != 0) {
                selectedCompactCharacterIndex = 0;
                if (playerSelection->character <= 5) {
                    selectedCompactCharacterIndex = playerSelection->character - 1;
                }
                else if (playerSelection->character <= 0xB) {
                    selectedCompactCharacterIndex = playerSelection->character - 3;
                }
                else {
                    selectedCompactCharacterIndex = playerSelection->character - 4;
                }
                PlayCharacterSelectionSound(selectedCompactCharacterIndex);
                HuSprBankSet(characterSpriteGroup, selectedCompactCharacterIndex + 2, 1);
            }
            else {
                PlayCharacterSelectionSound(playerSelection->character);
                HuSprBankSet(characterSpriteGroup, playerSelection->character + 2, 1);
            }
            HuSprBankSet(characterSpriteGroup, playerSelection->playerIndex + 0x1E, 1);
            HuSprBankSet(characterSpriteGroup, playerSelection->playerIndex + 0x22, 1);
        }
        else if (wideCharacterGridEnabled != 0) {
            if (HuPadStkX[playerSelection->playerIndex] >= 5) {
                columnIndex++;
                horizontalMove = 1;
                if (columnIndex > 5) {
                    columnIndex = 5;
                }
            }
            else if (HuPadStkX[playerSelection->playerIndex] <= -5) {
                columnIndex--;
                horizontalMove = -1;
                if (columnIndex < 0) {
                    columnIndex = 0;
                }
            }
            if (HuPadStkY[playerSelection->playerIndex] <= -5) {
                rowIndex++;
                verticalMove = 1;
                if (rowIndex > 2) {
                    rowIndex = 2;
                }
            }
            else if (HuPadStkY[playerSelection->playerIndex] >= 5) {
                rowIndex--;
                verticalMove = -1;
                if (rowIndex < 0) {
                    rowIndex = 0;
                }
            }
        }
        else {
            if (HuPadStkX[playerSelection->playerIndex] >= 5) {
                columnIndex++;
                horizontalMove = 1;
                if (columnIndex > 3) {
                    columnIndex = 3;
                }
            }
            else if (HuPadStkX[playerSelection->playerIndex] <= -5) {
                columnIndex--;
                horizontalMove = -1;
                if (columnIndex < 0) {
                    columnIndex = 0;
                }
            }
            if (HuPadStkY[playerSelection->playerIndex] <= -5) {
                rowIndex++;
                verticalMove = 1;
                if (rowIndex > 1) {
                    rowIndex = 1;
                }
            }
            else if (HuPadStkY[playerSelection->playerIndex] >= 5) {
                rowIndex--;
                verticalMove = -1;
                if (rowIndex < 0) {
                    rowIndex = 0;
                }
            }
        }
    }
    else if ((HuPadBtnDown[playerSelection->playerIndex] & PAD_BUTTON_B) != 0) {
        playerSelection->selectionState = 0;
        teamSelectionWork.selectionChanged = 1;
        HuAudFXPlay(3);
        if (wideCharacterGridEnabled != 0) {
            previousCompactCharacterIndex = 0;
            if (playerSelection->character <= 5) {
                previousCompactCharacterIndex = playerSelection->character - 1;
            }
            else if (playerSelection->character <= 0xB) {
                previousCompactCharacterIndex = playerSelection->character - 3;
            }
            else {
                previousCompactCharacterIndex = playerSelection->character - 4;
            }
            HuSprBankSet(characterSpriteGroup, previousCompactCharacterIndex + 2, 0);
        }
        else {
            HuSprBankSet(characterSpriteGroup, playerSelection->character + 2, 0);
        }
        HuSprBankSet(characterSpriteGroup, playerSelection->playerIndex + 0x1E, 0);
        HuSprBankSet(characterSpriteGroup, playerSelection->playerIndex + 0x22, 0);
    }
    if (wideCharacterGridEnabled != 0) {
        candidateCharacterIndex = columnIndex + (rowIndex * 6);
    }
    else {
        candidateCharacterIndex = columnIndex + (rowIndex * 4);
    }
    if (playerSelection->character != candidateCharacterIndex) {
        if (wideCharacterGridEnabled != 0) {
            while (1) {
                candidateCharacterIndex = columnIndex + (rowIndex * 6);
                for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                    if (((otherPlayerIndex != playerSelection->playerIndex) && (playerSelectionWork[otherPlayerIndex].isComputer == 0) && (candidateCharacterIndex == playerSelectionWork[otherPlayerIndex].character))
                        || (candidateCharacterIndex == 0) || (candidateCharacterIndex == 5) || (candidateCharacterIndex == 6) || (candidateCharacterIndex == 0xB)) {
                        if ((horizontalMove == 0) || (verticalMove == 0)) {
                            if (((columnIndex + horizontalMove) > 5) || ((columnIndex + horizontalMove) < 0)) {
                                columnIndex = originalColumnIndex;
                            }
                            else {
                                columnIndex += horizontalMove;
                            }
                            if (((rowIndex + verticalMove) > 2) || ((rowIndex + verticalMove) < 0)) {
                                rowIndex = originalRowIndex;
                            }
                            else {
                                rowIndex += verticalMove;
                            }
                        }
                        else if (((columnIndex + horizontalMove) <= 5) && ((columnIndex + horizontalMove) >= 0)) {
                            columnIndex += horizontalMove;
                        }
                        else if (((rowIndex + verticalMove) <= 2) && ((rowIndex + verticalMove) >= 0)) {
                            rowIndex += verticalMove;
                        }
                        else {
                            columnIndex = originalColumnIndex;
                            rowIndex = originalRowIndex;
                        }
                        break;
                    }
                    else {
                    }
                }
                if (otherPlayerIndex == 4) {
                    break;
                }
            }
        }
        else {
            while (1) {
                candidateCharacterIndex = columnIndex + (rowIndex * 4);
                for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                    if ((otherPlayerIndex != playerSelection->playerIndex) && (playerSelectionWork[otherPlayerIndex].isComputer == 0) && (candidateCharacterIndex == playerSelectionWork[otherPlayerIndex].character)) {
                        if ((horizontalMove == 0) || (verticalMove == 0)) {
                            if (((columnIndex + horizontalMove) > 3) || ((columnIndex + horizontalMove) < 0)) {
                                columnIndex = originalColumnIndex;
                            }
                            else {
                                columnIndex += horizontalMove;
                            }
                            if (((rowIndex + verticalMove) > 1) || ((rowIndex + verticalMove) < 0)) {
                                rowIndex = originalRowIndex;
                            }
                            else {
                                rowIndex += verticalMove;
                            }
                            break;
                        }
                        else if (((columnIndex + horizontalMove) <= 3) && ((columnIndex + horizontalMove) >= 0)) {
                            columnIndex += horizontalMove;
                            break;
                        }
                        else if (((rowIndex + verticalMove) <= 1) && ((rowIndex + verticalMove) >= 0)) {
                            rowIndex += verticalMove;
                            break;
                        }
                        else {
                            columnIndex = originalColumnIndex;
                            rowIndex = originalRowIndex;
                            break;
                        }
                    }
                    else {
                        continue;
                    }
                }
                if (otherPlayerIndex == 4) {
                    break;
                }
            }
        }
        if (wideCharacterGridEnabled != 0) {
            candidateCharacterIndex = columnIndex + (rowIndex * 6);
        }
        else {
            candidateCharacterIndex = columnIndex + (rowIndex * 4);
        }
        if (playerSelection->character != candidateCharacterIndex) {
            HuAudFXPlay(0);
            previousCharacterIndex = playerSelection->character;
            playerSelection->character = candidateCharacterIndex;
            if (wideCharacterGridEnabled != 0) {
                for (otherPlayerIndex = 0; otherPlayerIndex <= 0xA; otherPlayerIndex++) {
                    SleepSelectionProcess(1);
                    cursorX = InterpolateEaseIn(alternateCharacterGridPositions[previousCharacterIndex][0], alternateCharacterGridPositions[candidateCharacterIndex][0], otherPlayerIndex, 10.0f);
                    cursorY = InterpolateEaseIn(alternateCharacterGridPositions[previousCharacterIndex][1], alternateCharacterGridPositions[candidateCharacterIndex][1], otherPlayerIndex, 10.0f);
                    HuSprPosSet(characterSpriteGroup, playerSelection->playerIndex + 0x1E, cursorX, cursorY);
                    HuSprPosSet(characterSpriteGroup, playerSelection->playerIndex + 0x22, cursorX, cursorY);
                }
                return;
            }
            for (otherPlayerIndex = 0; otherPlayerIndex <= 0xA; otherPlayerIndex++) {
                SleepSelectionProcess(1);
                cursorX = InterpolateEaseIn(*characterGridPositions[originalColumnIndex], *characterGridPositions[columnIndex], otherPlayerIndex, 10.0f);
                cursorY = InterpolateEaseIn(characterGridPositions[originalRowIndex * 4][1], characterGridPositions[rowIndex * 4][1], otherPlayerIndex, 10.0f);
                HuSprPosSet(characterSpriteGroup, playerSelection->playerIndex + 0x1E, cursorX, cursorY);
                HuSprPosSet(characterSpriteGroup, playerSelection->playerIndex + 0x22, cursorX, cursorY);
            }
        }
    }
}

void ShowPlayerCharacterSelection(ZtarPlayerSelectionWork *playerSelection)
{
    s32 characterSpriteGroup;
    s32 otherPlayerIndex;
    s32 compactCharacterIndex;

    characterSpriteGroup = selectionSpriteGroups[1];
    do {
        for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
            if ((otherPlayerIndex != playerSelection->playerIndex) && (playerSelectionWork[otherPlayerIndex].selectionState == 1) && (playerSelection->character == playerSelectionWork[otherPlayerIndex].character)) {
                playerSelection->character += 1;
                break;
            }
        }
    } while (otherPlayerIndex != 4);
    playerSelection->difficulty = 0;
    if (wideCharacterGridEnabled != 0) {
        compactCharacterIndex = 0;
        if (playerSelection->character <= 5) {
            compactCharacterIndex = playerSelection->character - 1;
        }
        else if (playerSelection->character <= 0xB) {
            compactCharacterIndex = playerSelection->character - 3;
        }
        else {
            compactCharacterIndex = playerSelection->character - 4;
        }
        HuSprBankSet(characterSpriteGroup, compactCharacterIndex + 2, 0);
    }
    else {
        HuSprBankSet(characterSpriteGroup, playerSelection->character + 2, 0);
    }
    HuSprBankSet(characterSpriteGroup, playerSelection->playerIndex + 0x26, 0);
    HuSprBankSet(characterSpriteGroup, playerSelection->playerIndex + 0x2A, 0);
    HuSprBankSet(characterSpriteGroup, playerSelection->playerIndex + 0x2E, playerSelection->difficulty);
    HuSprBankSet(characterSpriteGroup, playerSelection->playerIndex + 0x32, playerSelection->difficulty);
    if (wideCharacterGridEnabled != 0) {
        HuSprPosSet(characterSpriteGroup, playerSelection->playerIndex + 0x26, *alternateCharacterGridPositions[playerSelection->character], alternateCharacterGridPositions[playerSelection->character][1]);
        HuSprPosSet(characterSpriteGroup, playerSelection->playerIndex + 0x2A, *alternateCharacterGridPositions[playerSelection->character], alternateCharacterGridPositions[playerSelection->character][1]);
        HuSprPosSet(characterSpriteGroup, playerSelection->playerIndex + 0x2E, *alternateCharacterGridPositions[playerSelection->character], alternateCharacterGridPositions[playerSelection->character][1] + 0x23);
        HuSprPosSet(characterSpriteGroup, playerSelection->playerIndex + 0x32, *alternateCharacterGridPositions[playerSelection->character], alternateCharacterGridPositions[playerSelection->character][1] + 0x23);
    }
    else {
        HuSprPosSet(characterSpriteGroup, playerSelection->playerIndex + 0x26, *characterGridPositions[playerSelection->character % 4], characterGridPositions[(playerSelection->character / 4) * 4][1]);
        HuSprPosSet(characterSpriteGroup, playerSelection->playerIndex + 0x2A, *characterGridPositions[playerSelection->character % 4], characterGridPositions[(playerSelection->character / 4) * 4][1]);
        HuSprPosSet(characterSpriteGroup, playerSelection->playerIndex + 0x2E, *characterGridPositions[playerSelection->character % 4], characterGridPositions[(playerSelection->character / 4) * 4][1] + 0x23);
        HuSprPosSet(characterSpriteGroup, playerSelection->playerIndex + 0x32, *characterGridPositions[playerSelection->character % 4], characterGridPositions[(playerSelection->character / 4) * 4][1] + 0x23);
    }
    playerSelection->selectionChanged = 0;
    HuSprAttrReset(characterSpriteGroup, playerSelection->playerIndex + 0x26, 4);
    HuSprAttrReset(characterSpriteGroup, playerSelection->playerIndex + 0x2A, 4);
    HuSprAttrSet(characterSpriteGroup, playerSelection->playerIndex + 0x2E, 4);
    HuSprAttrSet(characterSpriteGroup, playerSelection->playerIndex + 0x32, 4);
}

void HidePlayerCharacterSelection(ZtarPlayerSelectionWork *playerSelection)
{
    s32 characterSpriteGroup = selectionSpriteGroups[1];
    HuSprAttrSet(characterSpriteGroup, playerSelection->playerIndex + 0x26, 4);
    HuSprAttrSet(characterSpriteGroup, playerSelection->playerIndex + 0x2A, 4);
    HuSprAttrSet(characterSpriteGroup, playerSelection->playerIndex + 0x2E, 4);
    HuSprAttrSet(characterSpriteGroup, playerSelection->playerIndex + 0x32, 4);
}

void UpdateComputerCharacterCursor(OMOBJ *selectionObject, ZtarPlayerSelectionWork *computerSelection)
{
    s32 previousDifficulty;
    float cursorX;
    float cursorY;
    s32 columnIndex;
    s32 rowIndex;
    s32 otherPlayerIndex;
    s32 characterSpriteGroup;
    s32 horizontalMove;
    s32 verticalMove;
    s32 candidateCharacterIndex;
    s32 originalColumnIndex;
    u32 originalRowIndex;
    s32 selectedCompactCharacterIndex;
    s32 previousCompactCharacterIndex;
    s32 previousCharacterIndex;

    characterSpriteGroup = selectionSpriteGroups[1];
    horizontalMove = 0;
    verticalMove = 0;
    previousDifficulty = computerSelection->difficulty;
    teamSelectionWork.characterSelectionBusy = 0;
    if (wideCharacterGridEnabled != 0) {
        columnIndex = computerSelection->character % 6;
        originalColumnIndex = columnIndex;
        rowIndex = computerSelection->character / 6;
        originalRowIndex = rowIndex;
    }
    else {
        columnIndex = computerSelection->character % 4;
        originalColumnIndex = columnIndex;
        rowIndex = computerSelection->character / 4;
        originalRowIndex = rowIndex;
    }
    if (computerSelection->selectionState == 0) {
        if (HuPadBtnDown[playerSelectionWork[0].controllerPort] & PAD_BUTTON_A) {
            computerSelection->selectionState = 1;
            HuSprBankSet(characterSpriteGroup, computerSelection->playerIndex + 0x26, 1);
            HuSprBankSet(characterSpriteGroup, computerSelection->playerIndex + 0x2A, 1);
            if (wideCharacterGridEnabled != 0) {
                selectedCompactCharacterIndex = 0;
                if (computerSelection->character <= 5) {
                    selectedCompactCharacterIndex = computerSelection->character - 1;
                }
                else if (computerSelection->character <= 0xB) {
                    selectedCompactCharacterIndex = computerSelection->character - 3;
                }
                else {
                    selectedCompactCharacterIndex = computerSelection->character - 4;
                }
                HuSprBankSet(characterSpriteGroup, selectedCompactCharacterIndex + 2, 1);
                PlayCharacterSelectionSound(selectedCompactCharacterIndex);
            }
            else {
                HuSprBankSet(characterSpriteGroup, computerSelection->character + 2, 1);
                PlayCharacterSelectionSound(computerSelection->character);
            }
            computerSelection->difficulty = 0;
            HuSprBankSet(characterSpriteGroup, computerSelection->playerIndex + 0x2E, computerSelection->difficulty);
            if (wideCharacterGridEnabled != 0) {
                HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 0x2E, alternateCharacterGridPositions[computerSelection->character][0], alternateCharacterGridPositions[computerSelection->character][1] + 0x23);
            }
            else {
                HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 0x2E, characterGridPositions[computerSelection->character % 4][0], characterGridPositions[(computerSelection->character / 4) * 4][1] + 0x23);
            }
            HuSprAttrReset(characterSpriteGroup, computerSelection->playerIndex + 0x2E, 4);
        }
        else if (wideCharacterGridEnabled != 0) {
            if (HuPadStkX[0] >= 5) {
                columnIndex++;
                horizontalMove = 1;
                if (columnIndex > 5) {
                    columnIndex = 5;
                }
            }
            else if (HuPadStkX[0] <= -5) {
                columnIndex--;
                horizontalMove = -1;
                if (columnIndex < 0) {
                    columnIndex = 0;
                }
            }
            if (*HuPadStkY <= -5) {
                rowIndex++;
                verticalMove = 1;
                if (rowIndex > 2) {
                    rowIndex = 2;
                }
            }
            else if (*HuPadStkY >= 5) {
                rowIndex--;
                verticalMove = -1;
                if (rowIndex < 0) {
                    rowIndex = 0;
                }
            }
        }
        else {
            if (HuPadStkX[0] >= 5) {
                columnIndex++;
                horizontalMove = 1;
                if (columnIndex > 3) {
                    columnIndex = 3;
                }
            }
            else if (HuPadStkX[0] <= -5) {
                columnIndex--;
                horizontalMove = -1;
                if (columnIndex < 0) {
                    columnIndex = 0;
                }
            }
            if (*HuPadStkY <= -5) {
                rowIndex++;
                verticalMove = 1;
                if (rowIndex > 1) {
                    rowIndex = 1;
                }
            }
            else if (*HuPadStkY >= 5) {
                rowIndex--;
                verticalMove = -1;
                if (rowIndex < 0) {
                    rowIndex = 0;
                }
            }
        }
    }
    else if (HuPadBtnDown[playerSelectionWork[0].controllerPort] & PAD_BUTTON_B) {
        computerSelection->selectionState = computerSelection->selectionChanged = 0;
        if (wideCharacterGridEnabled != 0) {
            previousCompactCharacterIndex = 0;
            if (computerSelection->character <= 5) {
                previousCompactCharacterIndex = computerSelection->character - 1;
            }
            else if (computerSelection->character <= 0xB) {
                previousCompactCharacterIndex = computerSelection->character - 3;
            }
            else {
                previousCompactCharacterIndex = computerSelection->character - 4;
            }
            HuSprBankSet(characterSpriteGroup, previousCompactCharacterIndex + 2, 0);
        }
        else {
            HuSprBankSet(characterSpriteGroup, computerSelection->character + 2, 0);
        }
        HuSprBankSet(characterSpriteGroup, computerSelection->playerIndex + 0x26, 0);
        HuSprBankSet(characterSpriteGroup, computerSelection->playerIndex + 0x2A, 0);
        HuSprAttrSet(characterSpriteGroup, computerSelection->playerIndex + 0x2E, 4);
        HuAudFXPlay(3);
    }
    if (wideCharacterGridEnabled != 0) {
        candidateCharacterIndex = columnIndex + (rowIndex * 6);
    }
    else {
        candidateCharacterIndex = columnIndex + (rowIndex * 4);
    }
    if (computerSelection->character != candidateCharacterIndex) {
        if (wideCharacterGridEnabled != 0) {
            while (1) {
                SleepSelectionProcess(1);
                candidateCharacterIndex = columnIndex + (rowIndex * 6);
                for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                    if (((otherPlayerIndex != computerSelection->playerIndex) && (playerSelectionWork[otherPlayerIndex].selectionState == 1) && (candidateCharacterIndex == playerSelectionWork[otherPlayerIndex].character))
                        || (candidateCharacterIndex == 0) || (candidateCharacterIndex == 5) || (candidateCharacterIndex == 6) || (candidateCharacterIndex == 0xB)) {
                        if ((horizontalMove == 0) || (verticalMove == 0)) {
                            if (((columnIndex + horizontalMove) > 5) || ((columnIndex + horizontalMove) < 0)) {
                                columnIndex = originalColumnIndex;
                            }
                            else {
                                columnIndex += horizontalMove;
                            }
                            if (((rowIndex + verticalMove) > 2) || ((rowIndex + verticalMove) < 0)) {
                                rowIndex = originalRowIndex;
                            }
                            else {
                                rowIndex += verticalMove;
                            }
                        }
                        else if (((columnIndex + horizontalMove) <= 5) && ((columnIndex + horizontalMove) >= 0)) {
                            columnIndex += horizontalMove;
                        }
                        else if (((rowIndex + verticalMove) <= 2) && ((rowIndex + verticalMove) >= 0)) {
                            rowIndex += verticalMove;
                        }
                        else {
                            columnIndex = originalColumnIndex;
                            rowIndex = originalRowIndex;
                        }
                        break;
                    }
                }
                if (otherPlayerIndex == 4) {
                    break;
                }
            }
        }
        else {
            do {
                candidateCharacterIndex = columnIndex + (rowIndex * 4);
                for (otherPlayerIndex = 0; otherPlayerIndex < 4; otherPlayerIndex++) {
                    if ((otherPlayerIndex != computerSelection->playerIndex) && (playerSelectionWork[otherPlayerIndex].selectionState == 1) && (candidateCharacterIndex == playerSelectionWork[otherPlayerIndex].character)) {
                        if ((horizontalMove == 0) || (verticalMove == 0)) {
                            if (((columnIndex + horizontalMove) > 3) || ((columnIndex + horizontalMove) < 0)) {
                                columnIndex = originalColumnIndex;
                            }
                            else {
                                columnIndex += horizontalMove;
                            }
                            if (((rowIndex + verticalMove) > 1) || ((rowIndex + verticalMove) < 0)) {
                                rowIndex = originalRowIndex;
                            }
                            else {
                                rowIndex += verticalMove;
                            }
                        }
                        else if (((columnIndex + horizontalMove) <= 3) && ((columnIndex + horizontalMove) >= 0)) {
                            columnIndex += horizontalMove;
                        }
                        else if (((rowIndex + verticalMove) <= 1) && ((rowIndex + verticalMove) >= 0)) {
                            rowIndex += verticalMove;
                        }
                        else {
                            columnIndex = originalColumnIndex;
                            rowIndex = originalRowIndex;
                        }
                        break;
                    }
                }
            } while (otherPlayerIndex != 4);
        }
        if (wideCharacterGridEnabled != 0) {
            candidateCharacterIndex = columnIndex + (rowIndex * 6);
        }
        else {
            candidateCharacterIndex = columnIndex + (rowIndex * 4);
        }
        if (computerSelection->character != candidateCharacterIndex) {
            previousCharacterIndex = computerSelection->character;
            computerSelection->character = candidateCharacterIndex;
            HuAudFXPlay(0);
            if (wideCharacterGridEnabled != 0) {
                for (otherPlayerIndex = 0; otherPlayerIndex <= 0xA; otherPlayerIndex++) {
                    SleepSelectionProcess(1);
                    cursorX = InterpolateEaseIn(alternateCharacterGridPositions[previousCharacterIndex][0], alternateCharacterGridPositions[candidateCharacterIndex][0], otherPlayerIndex, 10.0f);
                    cursorY = InterpolateEaseIn(alternateCharacterGridPositions[previousCharacterIndex][1], alternateCharacterGridPositions[candidateCharacterIndex][1], otherPlayerIndex, 10.0f);
                    HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 0x26, cursorX, cursorY);
                    HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 0x2A, cursorX, cursorY);
                    HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 0x2E, cursorX, 35.0f + cursorY);
                }
            }
            else {
                for (otherPlayerIndex = 0; otherPlayerIndex <= 0xA; otherPlayerIndex++) {
                    SleepSelectionProcess(1);
                    cursorX = InterpolateEaseIn(characterGridPositions[originalColumnIndex][0], characterGridPositions[columnIndex][0], otherPlayerIndex, 10.0f);
                    cursorY = InterpolateEaseIn(characterGridPositions[originalRowIndex * 4][1], characterGridPositions[rowIndex * 4][1], otherPlayerIndex, 10.0f);
                    HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 0x26, cursorX, cursorY);
                    HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 0x2A, cursorX, cursorY);
                    HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 0x2E, cursorX, 35.0f + cursorY);
                }
            }
        }
    }
    teamSelectionWork.characterSelectionBusy = 0;
}

s32 SelectComputerDifficulty(ZtarPlayerSelectionWork *computerSelection)
{
    float difficultyIconX;
    s32 animationFrame;
    s32 characterSpriteGroup;
    s32 difficultyDirection;
    s32 difficultySelectionResult;
    s32 previousDifficulty;

    difficultyDirection = 0;
    characterSpriteGroup = selectionSpriteGroups[1];
    while (1) {
        SleepSelectionProcess(1);
        difficultyDirection = 0;
        previousDifficulty = computerSelection->difficulty;
        if (HuPadStkX[playerSelectionWork[0].controllerPort] >= 5 || (HuPadBtn[playerSelectionWork[0].controllerPort] & PAD_BUTTON_RIGHT)) {
            if (GWGameStat.veryHardUnlock == 1) {
                computerSelection->difficulty++;
                difficultyDirection = 1;
                if (computerSelection->difficulty > 3) {
                    computerSelection->difficulty = 0;
                }
            }
            else {
                computerSelection->difficulty++;
                difficultyDirection = 1;
                if (computerSelection->difficulty > 2) {
                    computerSelection->difficulty = 0;
                }
            }
        }
        else if (HuPadStkX[playerSelectionWork[0].controllerPort] <= -5 || (HuPadBtn[playerSelectionWork[0].controllerPort] & PAD_BUTTON_LEFT)) {
            if (GWGameStat.veryHardUnlock == 1) {
                computerSelection->difficulty--;
                difficultyDirection = -1;
                if (computerSelection->difficulty < 0) {
                    computerSelection->difficulty = 3;
                }
            }
            else {
                computerSelection->difficulty--;
                difficultyDirection = -1;
                if (computerSelection->difficulty < 0) {
                    computerSelection->difficulty = 2;
                }
            }
        }
        else if (HuPadBtnDown[0] & PAD_BUTTON_A) {
            difficultySelectionResult = 1;
            break;
        }
        else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            difficultySelectionResult = 0;
            HuAudFXPlay(3);
            break;
        }
        if (computerSelection->difficulty != previousDifficulty) {
            HuAudFXPlay(0);
            for (animationFrame = 0; animationFrame <= 5; animationFrame++) {
                SleepSelectionProcess(1);
                if (wideCharacterGridEnabled) {
                    difficultyIconX = InterpolateEaseIn(alternateCharacterGridPositions[computerSelection->character][0], alternateCharacterGridPositions[computerSelection->character][0] + (difficultyDirection * 10), animationFrame, 5.0f);
                    HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 46, difficultyIconX, alternateCharacterGridPositions[computerSelection->character][1] + 35);
                }
                else {
                    difficultyIconX = InterpolateEaseIn(characterGridPositions[computerSelection->character % 4][0], characterGridPositions[computerSelection->character % 4][0] + (difficultyDirection * 10), animationFrame, 5.0f);
                    HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 46, difficultyIconX, characterGridPositions[(computerSelection->character / 4) * 4][1] + 35);
                }
            }
            HuSprBankSet(characterSpriteGroup, computerSelection->playerIndex + 46, computerSelection->difficulty);
            for (animationFrame = 0; animationFrame <= 5; animationFrame++) {
                SleepSelectionProcess(1);
                if (wideCharacterGridEnabled) {
                    difficultyIconX = InterpolateEaseIn(alternateCharacterGridPositions[computerSelection->character][0] + (difficultyDirection * 10), alternateCharacterGridPositions[computerSelection->character][0], animationFrame, 5.0f);
                    HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 46, difficultyIconX, alternateCharacterGridPositions[computerSelection->character][1] + 35);
                }
                else {
                    difficultyIconX = InterpolateEaseIn(characterGridPositions[computerSelection->character % 4][0] + (difficultyDirection * 10), characterGridPositions[computerSelection->character % 4][0], animationFrame, 5.0f);
                    HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 46, difficultyIconX, characterGridPositions[(computerSelection->character / 4) * 4][1] + 35);
                }
            }
        }
    }
    if (difficultySelectionResult == 1) {
        if (wideCharacterGridEnabled) {
            HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 50, alternateCharacterGridPositions[computerSelection->character][0], alternateCharacterGridPositions[computerSelection->character][1] + 35);
        }
        else {
            HuSprPosSet(characterSpriteGroup, computerSelection->playerIndex + 50, characterGridPositions[computerSelection->character % 4][0], characterGridPositions[(computerSelection->character / 4) * 4][1] + 35);
        }
        HuSprBankSet(characterSpriteGroup, computerSelection->playerIndex + 50, computerSelection->difficulty);
        HuSprAttrReset(characterSpriteGroup, computerSelection->playerIndex + 50, HUSPR_ATTR_DISPOFF);
        HuSprAttrSet(characterSpriteGroup, computerSelection->playerIndex + 46, HUSPR_ATTR_DISPOFF);
    }
    return difficultySelectionResult;
}

void CreateCharacterDifficultySprites(void)
{
    s32 spriteId;
    s32 spriteIndex;
    s32 spriteGroupId;
    ANIMDATA *computerDifficultyAnimation;
    ANIMDATA *spriteAnimation;

    spriteGroupId = HuSprGrpCreate(0x1B);
    selectionSpriteGroups[2] = spriteGroupId;
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x67)), 0x64, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x68)), 0x64, 0);
    HuSprGrpMemberSet(spriteGroupId, 1, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x69)), 0x64, 0);
    HuSprGrpMemberSet(spriteGroupId, 2, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x6A)), 0x64, 0);
    HuSprGrpMemberSet(spriteGroupId, 3, spriteId);
    for (spriteIndex = 0; spriteIndex < 0xE; spriteIndex++) {
        spriteId = HuSprCreate(HuSprAnimReadFile(spriteIndex + DATA_MAKE_NUM(DATADIR_MPEX, 0x6B)), 0x5A, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 4, spriteId);
    }
    computerDifficultyAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x79));
    spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x7A));
    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        spriteId = HuSprCreate(computerDifficultyAnimation, 0x50, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0x12, spriteId);
        spriteId = HuSprCreate(spriteAnimation, 0x50, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0x16, spriteId);
    }
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x7B)), 0x46, 0);
    HuSprGrpMemberSet(spriteGroupId, 0x1A, spriteId);
    SetCharacterDifficultySprites(0);
    HideCharacterDifficultySprites(0);
}

void SetCharacterDifficultySprites(s32 difficultyLayout)
{
    s32 playerIndex;
    s32 difficultySpriteGroup;
    s32 characterIndex;
    s16 characterId;
    s16 firstPlayerCharacterId;
    s16 playerCharacterId;
    s32 firstPlayerDifficulty;
    s32 singlePlayerDifficulty;
    s32 playerDifficulty;
    s32 computerDifficulty;

    difficultySpriteGroup = selectionSpriteGroups[2];
    for (playerIndex = 0; playerIndex < 0x1B; playerIndex++) {
        HuSprAttrSet(difficultySpriteGroup, playerIndex, 4);
    }
    switch (difficultyLayout) {
        case 0:
            HuSprAttrReset(difficultySpriteGroup, 2, 4);
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                characterId = GWPlayerCfg[playerIndex].character;
                firstPlayerDifficulty = GWPlayerCfg[playerIndex].diff;
                HuSprAttrReset(difficultySpriteGroup, characterId + 4, 4);
                HuSprPosSet(difficultySpriteGroup, characterId + 4, difficultyIconPositions[playerIndex][0], difficultyIconPositions[playerIndex][1]);
                if (GWPlayerCfg[playerIndex].iscom != 0) {
                    HuSprAttrReset(difficultySpriteGroup, playerIndex + 0x16, 4);
                    HuSprBankSet(difficultySpriteGroup, playerIndex + 0x16, firstPlayerDifficulty);
                    HuSprPosSet(difficultySpriteGroup, playerIndex + 0x16, difficultyIconPositions[playerIndex][0], difficultyIconPositions[playerIndex][1] + 0x14);
                }
                else {
                    HuSprAttrReset(difficultySpriteGroup, playerIndex + 0x12, 4);
                    HuSprBankSet(difficultySpriteGroup, playerIndex + 0x12, playerIndex);
                    HuSprPosSet(difficultySpriteGroup, playerIndex + 0x12, difficultyIconPositions[playerIndex][0], difficultyIconPositions[playerIndex][1] + 0x14);
                }
            }
            break;
        case 1:
            HuSprAttrReset(difficultySpriteGroup, 0, 4);
            firstPlayerCharacterId = GWPlayerCfg->character;
            singlePlayerDifficulty = GWPlayerCfg->diff;
            HuSprAttrReset(difficultySpriteGroup, firstPlayerCharacterId + 4, 4);
            HuSprPosSet(difficultySpriteGroup, firstPlayerCharacterId + 4, 0.0f, difficultyIconVerticalOffsets[1]);
            if (GWPlayerCfg->iscom != 0) {
                HuSprAttrReset(difficultySpriteGroup, 0x16, 4);
                HuSprBankSet(difficultySpriteGroup, 0x16, singlePlayerDifficulty);
                HuSprPosSet(difficultySpriteGroup, playerIndex + 0x16, 0.0f, difficultyIconVerticalOffsets[1] + 0x14);
            }
            else {
                HuSprAttrReset(difficultySpriteGroup, 0x12, 4);
                HuSprBankSet(difficultySpriteGroup, 0x12, 0);
                HuSprPosSet(difficultySpriteGroup, 0x12, 0.0f, difficultyIconVerticalOffsets[1] + 0x14);
            }
            break;
        case 2:
            HuSprAttrReset(difficultySpriteGroup, 1, 4);
            for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                playerCharacterId = GWPlayerCfg[playerIndex].character;
                playerDifficulty = GWPlayerCfg[playerIndex].diff;
                HuSprAttrReset(difficultySpriteGroup, playerCharacterId + 4, 4);
                HuSprPosSet(difficultySpriteGroup, playerCharacterId + 4, characterDifficultyPositions[playerIndex][0], characterDifficultyPositions[playerIndex][1]);
                if (GWPlayerCfg[playerIndex].iscom != 0) {
                    HuSprAttrReset(difficultySpriteGroup, playerIndex + 0x16, 4);
                    HuSprBankSet(difficultySpriteGroup, playerIndex + 0x16, playerDifficulty);
                    HuSprPosSet(difficultySpriteGroup, playerIndex + 0x16, characterDifficultyPositions[playerIndex][0], characterDifficultyPositions[playerIndex][1] + 0x19);
                }
                else {
                    HuSprAttrReset(difficultySpriteGroup, playerIndex + 0x12, 4);
                    HuSprBankSet(difficultySpriteGroup, playerIndex + 0x12, playerIndex);
                    HuSprPosSet(difficultySpriteGroup, playerIndex + 0x12, characterDifficultyPositions[playerIndex][0], characterDifficultyPositions[playerIndex][1] + 0x19);
                }
            }
            break;
        case 3:
            HuSprAttrReset(difficultySpriteGroup, 1, 4);
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                characterIndex = GWPlayerCfg[playerIndex].character;
                computerDifficulty = GWPlayerCfg[playerIndex].diff;
                characterIndex = MapCompactCharacterIndex(characterIndex);
                HuSprAttrReset(difficultySpriteGroup, characterIndex + 4, 4);
                HuSprPosSet(difficultySpriteGroup, characterIndex + 4, computerDifficultyPositions[playerIndex][0], computerDifficultyPositions[playerIndex][1]);
                if (GWPlayerCfg[playerIndex].iscom != 0) {
                    HuSprAttrReset(difficultySpriteGroup, playerIndex + 0x16, 4);
                    HuSprBankSet(difficultySpriteGroup, playerIndex + 0x16, computerDifficulty);
                    HuSprPosSet(difficultySpriteGroup, playerIndex + 0x16, computerDifficultyPositions[playerIndex][0], computerDifficultyPositions[playerIndex][1] + 0x19);
                }
                else {
                    HuSprAttrReset(difficultySpriteGroup, playerIndex + 0x12, 4);
                    HuSprBankSet(difficultySpriteGroup, playerIndex + 0x12, playerIndex);
                    HuSprPosSet(difficultySpriteGroup, playerIndex + 0x12, computerDifficultyPositions[playerIndex][0], computerDifficultyPositions[playerIndex][1] + 0x19);
                }
            }
            HuSprAttrReset(difficultySpriteGroup, 0x1A, 4);
            break;
    }
    HuSprGrpPosSet(difficultySpriteGroup, 112.0f, -428.0f);
}

void HideCharacterDifficultySprites(s32 difficultyLayout)
{
    s32 characterIndex;
    s32 difficultySpriteGroup;

    difficultySpriteGroup = selectionSpriteGroups[2];
    for (characterIndex = 0; characterIndex < 0x1B; characterIndex++) {
        HuSprAttrSet(difficultySpriteGroup, characterIndex, 4);
    }
    HuSprGrpPosSet(difficultySpriteGroup, 112.0f, -428.0f);
}

void ShowCharacterDifficultySprites(s32 difficultyLayout)
{
    float verticalOffset;
    s32 difficultySpriteGroup;
    s32 characterIndex;

    difficultySpriteGroup = selectionSpriteGroups[2];
    SetCharacterDifficultySprites(difficultyLayout);
    for (characterIndex = 0; characterIndex <= 0x3C; characterIndex++) {
        SleepSelectionProcess(1);
        if (characterIndex <= 0x32) {
            verticalOffset = InterpolateEaseIn(-500.0f, 10.0f, characterIndex, 50.0f);
        }
        else {
            verticalOffset = InterpolateEaseOut(10.0f, 0.0f, characterIndex - 0x32, 10.0f);
        }
        HuSprGrpPosSet(difficultySpriteGroup, 112.0f, 72.0f + verticalOffset);
    }
    HuSprGrpPosSet(difficultySpriteGroup, 112.0f, 72.0f);
}

void HideCharacterDifficultySpritesAnimated(s32 difficultyLayout)
{
    float verticalOffset;
    s32 characterIndex;
    s32 spriteIndex;
    s32 difficultySpriteGroup;
    s32 hiddenDifficultySpriteGroup;

    difficultySpriteGroup = selectionSpriteGroups[2];
    for (characterIndex = 0; characterIndex <= 0x3C; characterIndex++) {
        SleepSelectionProcess(1);
        if (characterIndex <= 0xA) {
            verticalOffset = InterpolateEaseIn(0.0f, 10.0f, characterIndex, 10.0f);
        }
        else {
            verticalOffset = InterpolateEaseOut(10.0f, -500.0f, characterIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(difficultySpriteGroup, 112.0f, 72.0f + verticalOffset);
    }
    HuSprGrpPosSet(difficultySpriteGroup, 112.0f, -428.0f);
    hiddenDifficultySpriteGroup = selectionSpriteGroups[2];
    for (spriteIndex = 0; spriteIndex < 0x1B; spriteIndex++) {
        HuSprAttrSet(hiddenDifficultySpriteGroup, spriteIndex, 4);
    }
    HuSprGrpPosSet(hiddenDifficultySpriteGroup, 112.0f, -428.0f);
}

void RunSharedSelectionObject(void)
{
    OMOBJ *selectionObject;

    selectionObject = NULL;
    selectionObject = omAddObjEx(selectionProcess, 0x1000, 8, 8, -1, NULL);
    teamSelectionWork.object = selectionObject;
    teamSelectionWork.updateCallback = NULL;
    while (TRUE) {
        HuPrcVSleep();
        if (teamSelectionWork.updateCallback != NULL) {
            teamSelectionWork.updateCallback(teamSelectionWork.object, &teamSelectionWork);
        }
    }
}

void RunPlayerSelectionObject(void)
{
    s32 playerSlotIndex;
    OMOBJ *selectionObject;

    playerSlotIndex = nextPlayerSelectionSlot[0];
    nextPlayerSelectionSlot[0] = playerSlotIndex + 1;
    selectionObject = NULL;
    selectionObject = omAddObjEx(selectionProcess, 0x1000, 8, 8, -1, NULL);
    playerSelectionWork[playerSlotIndex].object = selectionObject;
    playerSelectionWork[playerSlotIndex].updateCallback = NULL;

    while (TRUE) {
        HuPrcVSleep();
        if (playerSelectionWork[playerSlotIndex].updateCallback != NULL) {
            playerSelectionWork[playerSlotIndex].updateCallback(playerSelectionWork[playerSlotIndex].object, &playerSelectionWork[playerSlotIndex]);
        }
    }
}

s32 MapCompactCharacterIndex(s32 characterIndex)
{
    s32 mappedCharacterIndex;

    switch (characterIndex) {
        case 9:
            mappedCharacterIndex = 8;
            break;
        case 10:
            mappedCharacterIndex = 9;
            break;
        case 11:
            mappedCharacterIndex = 0xA;
            break;
        case 12:
            mappedCharacterIndex = 0xB;
            break;
        case 13:
            mappedCharacterIndex = 0xC;
            break;
        case 8:
            mappedCharacterIndex = 0xD;
            break;
        default:
            mappedCharacterIndex = -1;
            break;
    }
    return mappedCharacterIndex;
}

s32 MapCharacterIndexToGrid(s32 characterIndex)
{
    s32 gridCharacterIndex;

    switch (characterIndex) {
        case 8:
            gridCharacterIndex = 9;
            break;
        case 9:
            gridCharacterIndex = 0xA;
            break;
        case 10:
            gridCharacterIndex = 0xB;
            break;
        case 11:
            gridCharacterIndex = 0xC;
            break;
        case 12:
            gridCharacterIndex = 0xD;
            break;
        case 13:
            gridCharacterIndex = 8;
            break;
        default:
            gridCharacterIndex = -1;
            break;
    }
    return gridCharacterIndex;
}

float InterpolateLinear(float startValue, float endValue, float elapsedFrames, float durationFrames)
{
    if (durationFrames <= elapsedFrames) {
        return endValue;
    }
    return startValue + ((elapsedFrames / durationFrames) * (endValue - startValue));
}

float GetAverageValue(float sampleValue, float newValue, float sampleCount)
{
    return (newValue + (sampleValue * (sampleCount - 1.0f))) / sampleCount;
}

float InterpolateEaseOut(float startValue, float endValue, float elapsedFrames, float durationFrames)
{
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return startValue + ((endValue - startValue) * (1.0 - cosd(((90.0f / durationFrames) * elapsedFrames))));
}

float InterpolateEaseIn(float startValue, float endValue, float elapsedFrames, float durationFrames)
{
    if (elapsedFrames >= durationFrames) {
        return endValue;
    }
    return startValue + ((endValue - startValue) * sind(((90.0f / durationFrames) * elapsedFrames)));
}

float InterpolateSineCycle(float startValue, float endValue, float elapsedFrames, float durationFrames)
{
    if (elapsedFrames >= durationFrames) {
        return startValue;
    }
    return startValue + ((endValue - startValue) * sind(((360.0f / durationFrames) * elapsedFrames)));
}

void SleepSelectionProcess(s32 sleepFrames)
{
    if (sleepFrames <= 1) {
        HuPrcVSleep();
        return;
    }
    HuPrcSleep(sleepFrames);
}

void InitializeSelectionWindows(void)
{
    HuWinInit(1);
}

void OpenSelectionWindow(void)
{
    s32 windowWidth;
    s32 windowHeight;
    s32 windowLeft;
    s32 windowTop;

    if (selectionWindowId == -1) {
        windowWidth = 0x1C9;
        windowHeight = 0x44;
        windowLeft = 1.0f + ((576.0f - windowWidth) / 2.0f);
        windowTop = 445.0f - windowHeight;
        selectionWindowId = HuWinExCreateStyled(windowLeft, windowTop, windowWidth, windowHeight, -1, 1);
        winData[selectionWindowId].active_pad = 1;
        HuWinMesPalSet(selectionWindowId, 7, 0, 0, 0);
        HuWinExAnimIn(selectionWindowId);
    }
}

void CloseSelectionWindow(void)
{
    if (selectionWindowId != -1) {
        HuWinMesSet(selectionWindowId, MAKE_MESSID(0x25, 0x04));
        HuWinExAnimOut(selectionWindowId);
        HuWinExCleanup(selectionWindowId);
        selectionWindowId = -1;
    }
}

void SetSelectionWindowMessage(s32 messageId, s32 waitFrames, s32 instantText)
{
    if (selectionWindowId != -1) {
        if (instantText == 1) {
            HuWinMesSpeedSet(selectionWindowId, 0);
        }
        else {
            HuWinMesSpeedSet(selectionWindowId, 1);
        }
        HuWinMesSet(selectionWindowId, messageId);
        if (waitFrames <= 0) {
            HuWinMesWait(selectionWindowId);
            return;
        }
        if (waitFrames <= 1) {
            HuPrcVSleep();
            return;
        }
        HuPrcSleep(waitFrames);
    }
}

void InsertSelectionWindowMessage(u32 messageId, s32 insertPosition)
{
    if (selectionWindowId != -1) {
        HuWinInsertMesSet(selectionWindowId, messageId, insertPosition);
    }
}

s32 ShowSelectionConfirmation(s32 choiceCount)
{
    float messageSize[2];
    s32 windowId;
    s32 windowWidth;
    s32 windowHeight;
    s32 selectedChoice;
    s32 windowLeft;
    s32 windowTop;

    HuWinMesMaxSizeGet(1, messageSize, MAKE_MESSID(0x1E, 0x35));
    windowWidth = messageSize[0];
    windowHeight = messageSize[1];
    windowLeft = 0x22C - windowWidth;
    windowTop = 0x1BD - windowHeight;
    windowId = HuWinExCreateStyled(windowLeft, windowTop, windowWidth, windowHeight, -1, 1);
    winData[windowId].active_pad = 1;
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    HuWinExAnimIn(windowId);
    HuWinMesSpeedSet(windowId, 0);
    HuWinMesSet(windowId, MAKE_MESSID(0x1E, 0x35));
    selectedChoice = HuWinChoiceGet(windowId, choiceCount);
    HuWinMesSet(windowId, MAKE_MESSID(0x25, 0x04));
    HuWinExAnimOut(windowId);
    HuWinExCleanup(windowId);

    return selectedChoice;
}

void OpenMessageWindowCentered(u32 messageId, s32 windowPositionMode)
{
    float messageSize[2];
    s32 messageHeightPixels;
    s32 messageHeight;
    s32 messageWidth;
    s32 windowLeft;

#if VERSION_JP
    if (currentMessageWindowId != -1) {
        HuWinMesSet(currentMessageWindowId, messageId);
        return;
    }
#endif
    HuWinMesMaxSizeGet(1, messageSize, messageId);
    messageWidth = messageSize[0];
    messageHeightPixels = messageSize[1];
    windowLeft = (0x240 - messageWidth) / 2.0f;
    if (windowPositionMode == 1) {
        messageHeight = 0x1BD - messageHeightPixels;
    }
    else {
        messageHeight = 0x181 - messageHeightPixels;
    }
#if VERSION_JP
    currentMessageWindowId = HuWinExCreateStyled(windowLeft, messageHeight, messageWidth, messageHeightPixels, -1, 1);
#else
    if (currentMessageWindowId != -1) {
        CloseMessageWindow();
        currentMessageWindowId = HuWinExCreateStyled(windowLeft, messageHeight, messageWidth, messageHeightPixels, -1, 1);
    }
    else {
        currentMessageWindowId = HuWinExCreateStyled(windowLeft, messageHeight, messageWidth, messageHeightPixels, -1, 1);
    }
#endif
    HuWinBGTPLvlSet(currentMessageWindowId, 0.0f);
    HuWinDispOn(currentMessageWindowId);
    HuWinMesSet(currentMessageWindowId, messageId);
    HuWinMesSpeedSet(currentMessageWindowId, 0);
}

void OpenMessageWindow(u32 messageId)
{
    float messageSize[2];
    s32 messageHeightPixels;
    s32 messageWidth;
    s32 windowHeight;
    s32 windowLeft;

#if VERSION_JP
    if (currentMessageWindowId != -1) {
        HuWinMesSet(currentMessageWindowId, messageId);
        return;
    }
#endif
    HuWinMesMaxSizeGet(1, messageSize, messageId);
    messageWidth = messageSize[0];
    messageHeightPixels = messageSize[1];
    windowLeft = (0x240 - messageWidth) / 2.0f;
    windowHeight = 0x181 - messageHeightPixels;
#if VERSION_JP
    currentMessageWindowId = HuWinExCreateStyled(windowLeft, windowHeight, messageWidth, messageHeightPixels, -1, 1);
    (void)windowHeight;
#else
    if (currentMessageWindowId != -1) {
        CloseMessageWindow();
        currentMessageWindowId = HuWinExCreateStyled(windowLeft, windowHeight, messageWidth, messageHeightPixels, -1, 1);
    }
    else {
        currentMessageWindowId = HuWinExCreateStyled(windowLeft, windowHeight, messageWidth, messageHeightPixels, -1, 1);
    }
#endif
    HuWinBGTPLvlSet(currentMessageWindowId, 0.0f);
    HuWinDispOn(currentMessageWindowId);
    HuWinMesSet(currentMessageWindowId, messageId);
    HuWinMesSpeedSet(currentMessageWindowId, 0);
}

void CloseMessageWindow(void)
{
    if (currentMessageWindowId != -1) {
        HuWinExCleanup(currentMessageWindowId);
        currentMessageWindowId = -1;
    }
}

void SetCharacterGridLayout(void)
{
    s32 secondaryPlayerIndex;
    s32 characterSpriteGroup;

    characterSpriteGroup = selectionSpriteGroups[1];
    for (secondaryPlayerIndex = 0; secondaryPlayerIndex < 4; secondaryPlayerIndex++) {
        if (playerSelectionWork[secondaryPlayerIndex].selectionState == 1) {
            HuSprBankSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].character + 2, 1);
            HuSprBankSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x1E, 1);
            HuSprBankSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x22, 1);
            HuSprAttrReset(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x1E, 4);
            HuSprAttrReset(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x22, 4);
            HuSprPosSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x1E, *characterGridPositions[playerSelectionWork[secondaryPlayerIndex].character % 4],
                characterGridPositions[(playerSelectionWork[secondaryPlayerIndex].character / 4) * 4][1]);
            HuSprPosSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x22, *characterGridPositions[playerSelectionWork[secondaryPlayerIndex].character % 4],
                characterGridPositions[(playerSelectionWork[secondaryPlayerIndex].character / 4) * 4][1]);
        }
        else {
            playerSelectionWork[secondaryPlayerIndex].character = 0;
        }
    }
}

void SetCharacterGridLayoutAlternate(void)
{
    s32 secondaryPlayerIndex;
    s32 characterSpriteGroup;

    characterSpriteGroup = selectionSpriteGroups[1];
    for (secondaryPlayerIndex = 0; secondaryPlayerIndex < 4; secondaryPlayerIndex++) {
        if (playerSelectionWork[secondaryPlayerIndex].selectionState == 1) {
            HuSprBankSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].character + 2, 1);
            HuSprBankSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x1E, 1);
            HuSprBankSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x22, 1);
            HuSprAttrReset(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x1E, 4);
            HuSprAttrReset(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x22, 4);
            HuSprPosSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x1E, *characterGridPositions[playerSelectionWork[secondaryPlayerIndex].character % 4],
                characterGridPositions[(playerSelectionWork[secondaryPlayerIndex].character / 4) * 4][1]);
            HuSprPosSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x22, *characterGridPositions[playerSelectionWork[secondaryPlayerIndex].character % 4],
                characterGridPositions[(playerSelectionWork[secondaryPlayerIndex].character / 4) * 4][1]);
        }
        else {
            playerSelectionWork[secondaryPlayerIndex].character = 0;
        }
    }
}

void SetCharacterGridLayoutCompact(void)
{
    s32 compactCharacterIndex;
    s32 characterSpriteGroup;
    s32 secondaryPlayerIndex;

    characterSpriteGroup = selectionSpriteGroups[1];
    for (secondaryPlayerIndex = 0; secondaryPlayerIndex < 4; secondaryPlayerIndex++) {
        if (playerSelectionWork[secondaryPlayerIndex].selectionState == 1) {
            if (playerSelectionWork[secondaryPlayerIndex].character <= 5) {
                compactCharacterIndex = playerSelectionWork[secondaryPlayerIndex].character - 1;
            }
            else if (playerSelectionWork[secondaryPlayerIndex].character <= 0xB) {
                compactCharacterIndex = playerSelectionWork[secondaryPlayerIndex].character - 3;
            }
            else {
                compactCharacterIndex = playerSelectionWork[secondaryPlayerIndex].character - 4;
            }
            HuSprBankSet(characterSpriteGroup, compactCharacterIndex + 2, 1);
            HuSprBankSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x1E, 1);
            HuSprBankSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x22, 1);
            HuSprAttrReset(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x1E, 4);
            HuSprAttrReset(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x22, 4);
            HuSprPosSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x1E, *alternateCharacterGridPositions[playerSelectionWork[secondaryPlayerIndex].character],
                alternateCharacterGridPositions[playerSelectionWork[secondaryPlayerIndex].character][1]);
            HuSprPosSet(characterSpriteGroup, playerSelectionWork[secondaryPlayerIndex].playerIndex + 0x22, *alternateCharacterGridPositions[playerSelectionWork[secondaryPlayerIndex].character],
                alternateCharacterGridPositions[playerSelectionWork[secondaryPlayerIndex].character][1]);
        }
        else {
            playerSelectionWork[secondaryPlayerIndex].character = 1;
        }
    }
}

void PlayCharacterSelectionSound(s32 characterIndex)
{
    s32 characterSoundIds[14] = {
        0x0000006D,
        0x0000006E,
        0x0000006F,
        0x00000070,
        0x00000071,
        0x00000072,
        0x00000073,
        0x00000074,
        0x00000039,
        0x00000040,
        0x0000004B,
        0x00000046,
        0x0000003E,
        0x0000003A,
    };

    if ((characterIndex < 0) || (characterIndex > 0xD)) {
        return;
    }
    HuAudFXPlay(characterSoundIds[characterIndex]);
}

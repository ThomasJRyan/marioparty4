#include "ext_math.h"
#include "game/board/main.h"
#include "game/chrman.h"
#include "game/esprite.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/minigame_seq.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/saveload.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"
#include "string.h"

#ifndef __MWERKS__
#include "game/audio.h"
#include "game/hsfex.h"
#include <stdio.h>
#endif

#include "REL/mpexDll.h"

typedef void (*MpexObjectUpdateCallback)(OMOBJ *, ...);

typedef struct MinigamePlayerStateWork {
    /* 0x00 */ s32 state;
    /* 0x04 */ s32 stateTimer;
    /* 0x08 */ s32 playerFlags;
    /* 0x0C */ s32 selectionIndex;
    /* 0x10 */ s32 selectionCount;
} MinigamePlayerStateWork; /* size = 0x14 */

typedef struct MpexMenuObjectWork {
    /* 0x00 */ OMOBJ *ownerObject;
    /* 0x04 */ MpexObjectUpdateCallback updateCallback;
    /* 0x08 */ s32 state;
    /* 0x0C */ s32 stateTimer;
    /* 0x10 */ s32 playerIndex;
    /* 0x14 */ char reserved14[0x14];
    /* 0x28 */ s32 characterIndex;
    /* 0x2C */ char reserved2C[0x1C];
} MpexMenuObjectWork; /* size = 0x48 */

s32 rand8(void);
float InterpolateLinear(float startValue, float endValue, float currentFrame, float duration);
float InterpolateCosine(float startValue, float endValue, float currentFrame, float duration);
float InterpolateSine(float startValue, float endValue, float currentFrame, float duration);

void ShowPlayerNameText(void);
void HidePlayerNameText(void);
void AnimatePlayerNameText(s32 messageId);
void CreatePlayerScoreSprites(void);
void DestroyPlayerScoreDisplay(void);
void ShowPlayerScoreDisplay(void);
void HidePlayerScoreDisplay(void);
void ShowPlayerScoreDisplayAlternate(void);
void AnimatePlayerScoreDisplayOut(void);
void ShowPlayerScoreText(void);
void HidePlayerScoreText(void);
void AnimatePlayerScoreText(s32 messageId);
void CreatePlayerStatusDisplay(void);
void DestroyPlayerStatusDisplay(void);
void ShowPlayerStatusIcon(void);
void HidePlayerStatusIcon(void);
void AnimatePlayerStatusDisplay(s32 playerIndex);
void ScalePlayerStatusDisplay(void);
void PositionPlayerStatusSprite(OMOBJ *object, MpexMenuObjectWork *statusWork);
void ReportMinigameHighScore(void);
void FitMessageWindowToText(float *windowSize, s32 messageId);
void RunMinigamePracticeMenuProcess(void);
void UpdateMinigameMenuObject(OMOBJ *object, MpexMenuObjectWork *menuWork);
void CreateMenuCharactersProcess(void);
void AnimateMenuCharacterEntry(OMOBJ *object, MpexMenuObjectWork *menuWork);
void CreatePracticePlayerModelsProcess(void);
void CreateMenuMinigameModelsProcess(void);
void MoveMenuModelWithSineEasing(OMOBJ *object, MpexMenuObjectWork *menuWork);
void MoveMenuModelWithCosineEasing(OMOBJ *object, MpexMenuObjectWork *menuWork);
void InitializePracticeCamera(void);
void RunPracticeMenuState(void);
void RunPracticeMenuResultsSequence(void);
void CreateCoinScoreDisplay(void);
void DestroyCoinScoreDisplay(void);
s32 GetPlayerCoinScore(s32 rank, s32 playerIndex);
void UpdateCoinScoreDisplay(s32 playerCount);
void HideCoinScoreDisplay(s32 playerCount);
void AnimateCoinScoreDisplay(s32 playerCount);
void AnimateCoinScoreDisplayAlternate(s32);
void CreateMinigameScoreDisplay(void);
void DestroyMinigameScoreDisplay(void);
u32 GetMinigameScoreMessageId(s32 playerCount, s32 scoreType);
void UpdateMinigameScoreDisplay(s32 playerCount, s32 scoreType);
void AnimateMinigameScoreDisplay(s32 playerCount, s32);
void HideMinigameScoreWindow(s32);
void CreateHighScoreDisplay(void);
void DestroyHighScoreDisplay(void);
s32 GetFormattedHighScore(s32 minigameId, s32 playerIndex, s32 recordType, s32 score);
void CreateMinigameNameDisplay(s32 messageId);
void DestroyMinigameNameDisplay(s32 displayMode);
void PositionMinigameNameDisplay(s32 displayMode);
void AnimateMinigameNameDisplay(s32 displayMode);
void HideMinigameNameDisplay(s32 displayMode);
void ShowPlayerScoreIcon(void);
void HidePlayerScoreIcon(void);
void SetPlayerScoreIconBank(void);
void HidePlayerScoreIconAlternate(void);
void CreatePlayerNameDisplay(s32 playerCount, s32 displayMode);
void CreatePlayerNameSprites(void);
void DestroyPlayerNameDisplay(void);
void ShowPlayerNameDisplay(void);
void HidePlayerNameDisplay(void);
void ShowPlayerNameDisplayAlternate(void);
void AnimatePlayerNameDisplayOut(void);

float menuModelPathOffsets[21] = {
    0.0f,
    0.0f,
    0.0f,
    -250.0f,
    0.0f,
    0.0f,
    250.0f,
    0.0f,
    0.0f,
    0.0f,
    500.0f,
    0.0f,
    -1000.0f,
    0.0f,
    0.0f,
    1000.0f,
    0.0f,
    0.0f,
    0.0f,
    1000.0f,
    0.0f,
};

MpexMenuObjectWork minigameNameWork;
MpexMenuObjectWork practiceMenuStateWork;
MpexMenuObjectWork menuModelObjects[3];
MpexMenuObjectWork playerCharacterObjects[4];
s32 practiceMenuState;
s32 practiceMenuSelection;
s32 currentMenuPlayer;
s32 availableMinigameFlags[0xB];
MinigamePlayerStateWork practicePlayerSettings[4];
char formattedPlayerRank[16];
char formattedPlayerCoins[8];
char formattedCoinDigits[8];
s32 coinScoreSpriteGroup;
s32 minigameScoreSpriteGroup;
s32 scoreSummaryWindow;
char formattedScoreRank[8];
char formattedScoreValue[8];
char formattedScoreDigits[8];
char formattedScoreRecord[8];
s32 playerNameWindow;
s32 playerNameWindows[6];
s32 playerScoreSpriteGroup;
s32 playerStatusSpriteGroup;
s32 playerStatusWindow;
float menuModelAnimationPhase;
s32 activePlayerStatusCount;
s32 playerStatusAnimationIndex;
u32 menuObjectCreateIndex;
HUPROCESS *minigamePracticeProcess;

HUPROCESS *InitializeMinigamePracticeOverlay(HUPROCESS *parentProcess)
{
    s32 loopIndex;

    minigamePracticeProcess = parentProcess;
    if (omovlevtno == 0) {
        mgGameStatBackup = GWGameStat;
    }
    CreateDirectionalLight();
    CreateOverlayShadow();
    InitializeMessageWindows();
    CreateOverlayCamera(InitializePracticeCamera);
    availableMinigameFlags[0] = 1;
    availableMinigameFlags[1] = 1;
    availableMinigameFlags[2] = GWMGAvailGet(omMgIndexGet(0x40) + 0x191);
    availableMinigameFlags[3] = GWMGAvailGet(omMgIndexGet(0xB) + 0x191);
    availableMinigameFlags[4] = GWMGAvailGet(omMgIndexGet(0x13) + 0x191);
    availableMinigameFlags[5] = 1;
    availableMinigameFlags[6] = 1;
    availableMinigameFlags[7] = 1;
    availableMinigameFlags[8] = 1;
    availableMinigameFlags[9] = GWMGAvailGet(omMgIndexGet(0x1B) + 0x191);
    ReportMinigameHighScore();
    for (loopIndex = 0; loopIndex < 0xA; loopIndex++) {
        if (availableMinigameFlags[loopIndex] == 0) {
            break;
        }
    }
    if (loopIndex == 0xA) {
        availableMinigameFlags[0xA] = 1;
    }
    else {
        availableMinigameFlags[0xA] = 0;
    }
    if (omovlevtno == 1) {
        for (loopIndex = 0; loopIndex < 4; loopIndex++) {
            HuPrcChildCreate(CreatePracticePlayerModelsProcess, 0x7D0, 0x2000, 0, HuPrcCurrentGet());
        }
    }
    HuPrcChildCreate(CreateMenuCharactersProcess, 0x7D0, 0x2000, 0, HuPrcCurrentGet());
    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        HuPrcChildCreate(CreateMenuMinigameModelsProcess, 0x7D0, 0x2000, 0, HuPrcCurrentGet());
    }
    CreateCharacterSelectionProcesses(minigamePracticeProcess);

    return HuPrcChildCreate(RunMinigamePracticeMenuProcess, 0x3E8, 0x2000, 0, HuPrcCurrentGet());
}

void RunMinigamePracticeMenuProcess(void)
{
    OMOBJ *object;

    object = NULL;
    object = omAddObjEx(minigamePracticeProcess, 0x2000, 0x10, 0x10, -1, NULL);
    minigameNameWork.ownerObject = object;
    minigameNameWork.updateCallback = (MpexObjectUpdateCallback)UpdateMinigameMenuObject;
    while (TRUE) {
        WaitFrames(1);
        if (minigameNameWork.updateCallback == NULL) {
            continue;
        }
        minigameNameWork.updateCallback(minigameNameWork.ownerObject, &minigameNameWork);
    }
}

void ReturnToOverlayCaller(void)
{
    WaitFrames(0x1E);
    HuAudFadeOut(0x3E8);
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);

    while (WipeStatGet() != 0) {
        WaitFrames(1);
    }
    HuAudAllStop();
    omOvlReturnEx(1, 1);

    while (TRUE) {
        WaitFrames(1);
    }
}

void SaveSettingsAndExitMinigame(void)
{
    s16 minigameValue;

    if (SLSaveFlagGet() != 0) {
        Hu3DAllKill();
        HuSprClose();
        HuSprInit();
        espInit();
        HuPrcVSleep();
        minigameValue = espEntry(0x860020, 0x1388, 0);
        espPosSet(minigameValue, 288.0f, 240.0f);
        espAttrReset(minigameValue, HUSPR_ATTR_DISPOFF);
        Hu3DBGColorSet(0, 0, 0);
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 10);

        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        SLCommonSet();
        if (SLSave() == 0) {
            GWSystem.save_mode = 1;
        }
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 20);

        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
    }
}

void ReturnToMainMenu(void)
{
    s16 minigameValue;

    WaitFrames(0x1E);
    HuAudFadeOut(0x3E8);
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);

    while (WipeStatGet() != 0) {
        WaitFrames(1);
    }

    HuAudAllStop();
    CharModelKill(-1);
    MGSeqKillAll();
    if (omovlevtno != 0) {
        WaitFrames(0x3C);
        SaveSettingsAndExitMinigame();
    }
    mgQuitExtraF = 0;
    omOvlReturnEx(1, 1);

    while (TRUE) {
        WaitFrames(1);
    }
}

void ReturnToMinigameMenu(void)
{
    OMOVLHIS *minigameValue;

    WaitFrames(0x1E);
    HuAudSStreamAllFadeOut(0x3E8);
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        WaitFrames(1);
    }
    CharModelKill(-1);
    MGSeqKillAll();
    minigameValue = omOvlHisGet(0);
    omOvlHisChg(0, DLL_mpexdll, 2, 0);
    mgQuitExtraF = 0;
    omOvlCallEx(DLL_ztardll, 1, 0, 0);
    while (TRUE) {
        WaitFrames(1);
    }
}

void InitializePracticePlayers(s32 playerCount, s32 saveMode)
{
    s32 overlayHistoryTable[10][3] = {
        { 0x60, 0x01, 0x00 },
        { 0x61, 0x01, 0x01 },
        { 0x45, 0x01, 0x02 },
        { 0x3C, 0x01, 0x03 },
        { 0x3B, 0x01, 0x04 },
        { 0x32, 0x01, 0x05 },
        { 0x41, 0x01, 0x06 },
        { 0x42, 0x01, 0x07 },
        { 0x43, 0x01, 0x08 },
        { 0x44, 0x01, 0x09 },
    };
    OMOVLHIS *historyEntry;
    s32 partyConfigId;

    WaitFrames(0x1E);
    HuAudSeqAllFadeOut(0x3E8);
    HuAudSStreamAllFadeOut(0x3E8);
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        WaitFrames(1);
    }
    if (playerCount == 0 || playerCount == 1) {
        partyConfigId = (saveMode * 0xA) + 0xA;
        BoardPartyConfigSet(0, 0, 0, partyConfigId, 0, 0, 0, 0);
        BoardSaveInit(playerCount + 7);
        overlayHistoryTable[playerCount][2] += (saveMode * 0xA);
    }
    else if (playerCount == 4) {
        GWSystem.mg_type = saveMode;
        overlayHistoryTable[playerCount][2] += (saveMode * 0xA);
    }

    historyEntry = omOvlHisGet(0);
    omOvlHisChg(0, DLL_mpexdll, overlayHistoryTable[playerCount][1], overlayHistoryTable[playerCount][2]);
    HuDataDirClose(DATA_MAKE_NUM(DATADIR_MPEX, 0x00));
    HuDataDirClose(0x150000);
    CharModelKill(-1);
    CharDataClose(-1);
    CharMotionInit(GWPlayerCfg[0].character);
    CharMotionInit(GWPlayerCfg[1].character);
    CharMotionInit(GWPlayerCfg[2].character);
    CharMotionInit(GWPlayerCfg[3].character);
    HuAudAllStop();
    if (playerCount > 1) {
        mgPracticeEnableF = 1;
    }
    mgQuitExtraF = 0;
    omOvlCallEx(overlayHistoryTable[playerCount][0], 1, 0, 0);
    while (TRUE) {
        WaitFrames(1);
    }
}

s32 activeMessageId = -1;
s32 practiceMenuSequenceId = -1;

void StartMinigameMenuSequence(void)
{
    if (omovlevtno != 2) {
        practiceMenuSequenceId = HuAudSeqPlay(0x2E);
    }
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    while (WipeStatGet() != 0) {
        WaitFrames(1);
    }
    WaitFrames(0x3C);
}

void RunPrimaryGameModeMenu(void)
{
    CreateCharacterSelectMessageWindow();
    SetModelMotionWithTransition(menuModelObjects[0].ownerObject, 1, 3, 0xF, 1);
    ShowCharacterSelectMessage(0x330000, -1, -1);
    SetModelMotionWithTransition(menuModelObjects[0].ownerObject, 1, 1, 0xF, 1);
    DestroyCharacterSelectMessageWindow();
    WaitFrames(0x3C);
    CreateCharacterSelectMessageWindow();
    ShowCharacterSelectMessage(0x330001, 5, -1);
    if (ShowCharacterSelectChoice(1) == 0) {
        SetModelMotionWithTransition(menuModelObjects[0].ownerObject, 1, 3, 0xF, 1);
        ShowCharacterSelectMessage(0x330002, -1, -1);
        SetModelMotionWithTransition(menuModelObjects[0].ownerObject, 1, 1, 0xF, 1);
    }
    DestroyCharacterSelectMessageWindow();
    WaitFrames(0x3C);
}

void RunPracticeMenuSelection(void)
{
    s16 minigameValue;
    s32 minigameValue2;
    s32 minigameValue3;

    minigameValue3 = -1;
    minigameValue2 = 0;
    CreateCharacterSelectMessageWindow();
    while (TRUE) {
        WaitFrames(1);
        SetModelMotionWithTransition(menuModelObjects[0].ownerObject, 1, 3, 0xF, 1);
        ShowCharacterSelectMessage(0x330003, -1, -1);
        SetModelMotionWithTransition(menuModelObjects[0].ownerObject, 1, 1, 0xF, 1);
        ShowCharacterSelectTopMessage(0x33004C);
        minigameValue3 = -1;
        practiceMenuStateWork.state = 0;
        practiceMenuStateWork.updateCallback = (MpexObjectUpdateCallback)PositionPlayerStatusSprite;
        while (TRUE) {
            WaitFrames(1);
            if (minigameValue3 != practiceMenuStateWork.state) {
                HuAudFXPlay(0);
                AnimatePlayerStatusDisplay(minigameValue3);
                minigameValue3 = practiceMenuStateWork.state;
                switch (practiceMenuStateWork.state) {
                    case 0:
                        ShowCharacterSelectMessage(0x330005, -1, 1);
                        break;
                    case 1:
                        ShowCharacterSelectMessage(0x330012, -1, 1);
                        break;
                    case 2:
                        ShowCharacterSelectMessage(0x330023, -1, 1);
                        break;
                }
            }
            if (HuPadBtnDown[0] & PAD_BUTTON_A) {
                HuAudFXPlay(2);
                minigameValue2 = 1;
                break;
            }
            else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
                HuAudFXPlay(3);
                minigameValue2 = 0;
                break;
            }
            else {
                if (availableMinigameFlags[0xA] == 1) {
                    if (HuPadStkX[0] <= -0x1E) {
                        practiceMenuStateWork.state = 0;
                    }
                    else if (HuPadStkX[0] >= 0x1E) {
                        practiceMenuStateWork.state = 1;
                    }
                    else if (HuPadStkY[0] >= 0x1E) {
                        practiceMenuStateWork.state = 2;
                    }
                    else if ((HuPadStkY[0] <= -0x1E) && (practiceMenuStateWork.state == 2)) {
                        if (HuPadStkX[0] >= 5) {
                            practiceMenuStateWork.state = 1;
                        }
                        else {
                            practiceMenuStateWork.state = 0;
                        }
                    }
                }
                else if (HuPadStkX[0] <= -0x1E) {
                    practiceMenuStateWork.state = 0;
                }
                else if (HuPadStkX[0] >= 0x1E) {
                    practiceMenuStateWork.state = 1;
                }
            }
        }
        practiceMenuStateWork.updateCallback = NULL;
        DestroyCharacterSelectTextWindow();
        if (minigameValue2 == 0) {
            ScalePlayerStatusDisplay();
            ShowCharacterSelectMessage(0x330004, 5, -1);
            if (ShowCharacterSelectChoice(1) == 0) {
                DestroyCharacterSelectMessageWindow();
                ReturnToMainMenu();
            }
            continue;
        }
        break;
    }
    ScalePlayerStatusDisplay();
    DestroyCharacterSelectMessageWindow();
    practiceMenuState = practiceMenuStateWork.state;
}

void SetMenuObjectsToEnterCallbacks(void)
{
    s32 frameIndex;

    for (frameIndex = 0; frameIndex < 3; frameIndex++) {
        menuModelObjects[frameIndex].updateCallback = (MpexObjectUpdateCallback)MoveMenuModelWithSineEasing;
    }
    do {
        WaitFrames(1);
        for (frameIndex = 0; frameIndex < 3; frameIndex++) {
            if (menuModelObjects[frameIndex].updateCallback != NULL) {
                break;
            }
        }
    } while (frameIndex != 3);
}

void SetMenuObjectsToExitCallbacks(void)
{
    s32 frameIndex;

    for (frameIndex = 0; frameIndex < 3; frameIndex++) {
        menuModelObjects[frameIndex].updateCallback = (MpexObjectUpdateCallback)MoveMenuModelWithCosineEasing;
    }
    do {
        WaitFrames(1);
        for (frameIndex = 0; frameIndex < 3; frameIndex++) {
            if (menuModelObjects[frameIndex].updateCallback != NULL) {
                break;
            }
        }
    } while (frameIndex != 3);
}

s32 RunCharacterSelectForPractice(s32 playerCount)
{
    s32 characterSelectionValue;

    characterSelectionValue = 1;
    characterSelectionValue = InitializeCharacterSelection(0, playerCount);
    return characterSelectionValue;
}

s32 RunPracticeSelectionFlow(s32 selectionMode)
{
    s32 minigameValue = 0;
    s32 minigameValue2 = -1;
    if (selectionMode == 0) {
        SelectCharacterMode(practiceMenuState);
    }
    ShowPlayerNameDisplayAlternate();
    CreateCharacterSelectMessageWindow();
    if (activeMessageId == -1) {
        SetModelMotionWithTransition(menuModelObjects[0].ownerObject, 1, 3, 0xF, 1);
        ShowCharacterSelectMessage(0x330007, -1, -1);
        SetModelMotionWithTransition(menuModelObjects[0].ownerObject, 1, 1, 0xF, 1);
    }
    else {
        practiceMenuStateWork.stateTimer = activeMessageId;
        activeMessageId = -1;
    }
    WaitFrames(1);
    ShowCharacterSelectTopMessage(0x33004D);
    ShowPlayerNameText();
    while (TRUE) {
        WaitFrames(1);
        if (minigameValue2 != practiceMenuStateWork.stateTimer) {
            minigameValue2 = practiceMenuStateWork.stateTimer;
            HuAudFXPlay(0);
            if (availableMinigameFlags[practiceMenuStateWork.stateTimer] == 1) {
                ShowCharacterSelectMessage(practiceMenuStateWork.stateTimer + MAKE_MESSID(0x33, 0x08), -1, 1);
            }
            else {
                ShowCharacterSelectMessage(0x33002E, -1, 1);
            }
            WaitFrames(1);
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_A) {
            if (availableMinigameFlags[practiceMenuStateWork.stateTimer] == 1) {
                HuAudFXPlay(2);
                activeMessageId = practiceMenuStateWork.stateTimer;
                minigameValue = 1;
                break;
            }
            else {
                HuAudFXPlay(4);
            }
        }
        else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            HuAudFXPlay(3);
            minigameValue = 0;
            break;
        }
        else {
            if (HuPadStkY[0] >= 0x1E) {
                AnimatePlayerNameText(-1);
            }
            else if (HuPadStkY[0] <= -0x1E) {
                AnimatePlayerNameText(1);
            }
        }
    }
    HidePlayerNameText();
    DestroyCharacterSelectTextWindow();
    DestroyCharacterSelectMessageWindow();
    AnimatePlayerNameDisplayOut();
    if (minigameValue == 0) {
        SelectDifficultyMode(practiceMenuState);
    }
    practiceMenuSelection = practiceMenuStateWork.stateTimer;
    return minigameValue;
}

s32 RunMinigameDescriptionPrompt(s32 playerCount)
{
    s32 minigameValue;

    minigameValue = 1;
    if (playerCount == 0) {
        practiceMenuStateWork.playerIndex = 0;
        AnimateMinigameNameDisplay(practiceMenuSelection);
    }
    while (TRUE) {
        WaitFrames(1);
        if (HuPadBtnDown[0] & PAD_BUTTON_START) {
            HuAudFXPlay(2);
            minigameValue = 1;
            break;
        }
        else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            HuAudFXPlay(3);
            minigameValue = 0;
            break;
        }
    }
    if (minigameValue == 0) {
        HideMinigameNameDisplay(practiceMenuSelection);
    }
    return minigameValue;
}

s32 RunPracticeOptionPrompt(s32 playerCount)
{
    s32 scoreValue;
    s32 scoreValue2;

    scoreValue = 1;
    scoreValue2 = practiceMenuStateWork.playerIndex;
    if ((practiceMenuSelection == 0) || (practiceMenuSelection == 1) || (practiceMenuSelection == 4)) {
        if (practiceMenuSelection == 4) {
            SetPlayerScoreIconBank();
        }
        else {
            ShowPlayerScoreIcon();
        }
        while (TRUE) {
            WaitFrames(1);
            if (practiceMenuStateWork.playerIndex != scoreValue2) {
                HuAudFXPlay(0);
                CreatePlayerNameDisplay(practiceMenuStateWork.playerIndex, scoreValue2);
                scoreValue2 = practiceMenuStateWork.playerIndex;
            }
            if (HuPadStkY[0] <= -0x1E) {
                practiceMenuStateWork.playerIndex++;
                if (practiceMenuStateWork.playerIndex > 2) {
                    practiceMenuStateWork.playerIndex = 0;
                }
                continue;
            }
            if (HuPadStkY[0] >= 0x1E) {
                practiceMenuStateWork.playerIndex--;
                if (practiceMenuStateWork.playerIndex < 0) {
                    practiceMenuStateWork.playerIndex = 2;
                }
                continue;
            }
            if (HuPadBtnDown[0] & PAD_BUTTON_START) {
                HuAudFXPlay(2);
                scoreValue = 1;
                break;
            }
            else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
                HuAudFXPlay(3);
                scoreValue = 0;
                break;
            }
        }
        currentMenuPlayer = practiceMenuStateWork.playerIndex;
    }
    else {
        scoreValue = 1;
    }
    if (scoreValue == 0) {
        if (practiceMenuSelection == 4) {
            HidePlayerScoreIconAlternate();
        }
        else {
            HidePlayerScoreIcon();
        }
    }
    return scoreValue;
}

s32 RunCharacterSelectForMainGame(s32 playerCount)
{
    s32 characterSelectionValue;

    characterSelectionValue = 1;
    characterSelectionValue = InitializeCharacterSelection(1, playerCount);
    return characterSelectionValue;
}

s32 RunMainGameSelectionFlow(s32 selectionMode)
{
    s32 minigameValue;
    s32 minigameValue2;

    minigameValue2 = -1;
    if (selectionMode == 0) {
        SelectCharacterMode(practiceMenuState);
    }
    ShowPlayerScoreDisplayAlternate();
    CreateCharacterSelectMessageWindow();
    if (activeMessageId == -1) {
        HuAudFXPlay(0x4A);
        SetModelMotionWithTransition(menuModelObjects[1].ownerObject, 1, 3, 0xF, 1);
        ShowCharacterSelectMessage(0x330014, -1, -1);
        SetModelMotionWithTransition(menuModelObjects[1].ownerObject, 1, 1, 0xF, 1);
    }
    else {
        practiceMenuStateWork.stateTimer = activeMessageId;
        activeMessageId = -1;
    }
    WaitFrames(1);
    ShowCharacterSelectTopMessage(0x33004D);
    ShowPlayerScoreText();
    while (TRUE) {
        WaitFrames(1);
        if (minigameValue2 != practiceMenuStateWork.stateTimer) {
            minigameValue2 = practiceMenuStateWork.stateTimer;
            HuAudFXPlay(0);
            if (availableMinigameFlags[practiceMenuStateWork.stateTimer + 4] == 1) {
                ShowCharacterSelectMessage(practiceMenuStateWork.stateTimer + 0x330015, -1, 1);
            }
            else {
                ShowCharacterSelectMessage(0x33002E, -1, 1);
            }
            WaitFrames(1);
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_A) {
            if (availableMinigameFlags[practiceMenuStateWork.stateTimer + 4] == 1) {
                HuAudFXPlay(2);
                activeMessageId = practiceMenuStateWork.stateTimer;
                minigameValue = 1;
                break;
            }
            else {
                HuAudFXPlay(4);
            }
        }
        else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            HuAudFXPlay(3);
            minigameValue = 0;
            break;
        }
        else {
            if (HuPadStkY[0] >= 0x1E) {
                AnimatePlayerScoreText(-1);
            }
            else if (HuPadStkY[0] <= -0x1E) {
                AnimatePlayerScoreText(1);
            }
        }
    }
    HidePlayerScoreText();
    DestroyCharacterSelectTextWindow();
    DestroyCharacterSelectMessageWindow();
    AnimatePlayerScoreDisplayOut();
    if (minigameValue == 0) {
        SelectDifficultyMode(practiceMenuState);
    }
    practiceMenuSelection = practiceMenuStateWork.stateTimer + (practiceMenuState * 4);
    return minigameValue;
}

s32 WaitForMinigameDescriptionPrompt(s32 playerCount)
{
    s32 minigameValue = 1;
    minigameValue = RunMinigameDescriptionPrompt(playerCount);
    return minigameValue;
}

s32 WaitForPracticeOptionPrompt(s32 playerCount)
{
    s32 playerStateValue = 1;
    playerStateValue = RunPracticeOptionPrompt(playerCount);
    return playerStateValue;
}

s32 SetPracticeMenuMode(s32 selectionMode)
{
    s32 minigameValue;

    minigameValue = 1;
    practiceMenuSelection = 0xA;
    minigameValue = RunMinigameDescriptionPrompt(selectionMode);
    return minigameValue;
}

void MoveMenuModelsOnscreen(void)
{
    s32 modelIndex;

    Hu3DModelPosSet(menuModelObjects[0].ownerObject->mdlId[1], -400.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(menuModelObjects[1].ownerObject->mdlId[1], 1000.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(menuModelObjects[2].ownerObject->mdlId[1], 0.0f, 1000.0f, 0.0f);
    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
        Hu3DModelPosSet(playerCharacterObjects[modelIndex].ownerObject->mdlId[1], -225.0f + (150.0f * modelIndex), 0.0f, 0.0f);
        Hu3DModelAttrReset(playerCharacterObjects[modelIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    }
}

void HideMenuModels(void)
{
    s32 modelIndex;

    Hu3DModelPosSet(menuModelObjects[0].ownerObject->mdlId[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(menuModelObjects[1].ownerObject->mdlId[1], 1000.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(menuModelObjects[2].ownerObject->mdlId[1], 0.0f, 1000.0f, 0.0f);
    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
        Hu3DModelPosSet(playerCharacterObjects[modelIndex].ownerObject->mdlId[1], -225.0f + (150.0f * modelIndex), 0.0f, 0.0f);
        Hu3DModelAttrSet(playerCharacterObjects[modelIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    }
}

void InitializePracticeMenuModels(void)
{
    s32 playerIndex;
    s32 playerIndex2;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        practicePlayerSettings[playerIndex].playerGroup = GWPlayerCfg[playerIndex].group;
        practicePlayerSettings[playerIndex].isComputerPlayer = GWPlayerCfg[playerIndex].iscom;
        practicePlayerSettings[playerIndex].difficulty = GWPlayerCfg[playerIndex].diff;
        practicePlayerSettings[playerIndex].characterId = GWPlayerCfg[playerIndex].character;
        practicePlayerSettings[playerIndex].controllerPort = GWPlayerCfg[playerIndex].pad_idx;
    }
    for (playerIndex2 = 0; playerIndex2 < 4; playerIndex2++) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (playerIndex2 == practicePlayerSettings[playerIndex].controllerPort) {
                GWPlayer[playerIndex2].team = GWPlayerCfg[playerIndex2].group = practicePlayerSettings[playerIndex].playerGroup;
                GWPlayer[playerIndex2].com = GWPlayerCfg[playerIndex2].iscom = practicePlayerSettings[playerIndex].isComputerPlayer;
                GWPlayer[playerIndex2].diff = GWPlayerCfg[playerIndex2].diff = practicePlayerSettings[playerIndex].difficulty;
                GWPlayer[playerIndex2].character = GWPlayerCfg[playerIndex2].character = practicePlayerSettings[playerIndex].characterId;
                GWPlayer[playerIndex2].port = GWPlayerCfg[playerIndex2].pad_idx = practicePlayerSettings[playerIndex].controllerPort;
                break;
            }
        }
    }
}

s32 RunMinigameMenuIntro(void)
{
    s32 minigameValue = 0;
    if (practiceMenuSelection != 2) {
        RunPracticeMenuState();
    }
    InitializePracticeMenuModels();
    WaitFrames(0x3C);
    minigameValue = 0;
    return minigameValue;
}

void AnimateMenuModelEntry(void)
{
    Vec position = { 0.0f, 0.0f, 0.0f };
    s32 frameIndex;

    for (frameIndex = 0; frameIndex < 4; frameIndex++) {
        playerCharacterObjects[frameIndex].updateCallback = (MpexObjectUpdateCallback)AnimateMenuCharacterEntry;
    }
    do {
        WaitFrames(1);
        for (frameIndex = 0; frameIndex < 4; frameIndex++) {
            if (playerCharacterObjects[frameIndex].updateCallback != NULL) {
                break;
            }
        }
    } while (frameIndex != 4);
    Hu3DModelAttrReset(practiceMenuStateWork.ownerObject->mdlId[5], HU3D_MOTATTR_PAUSE);
    SetModelMotionWithTransition(menuModelObjects[0].ownerObject, 1, 2, 0xF, 1);
    MoveModelTowardPosition(menuModelObjects[0].ownerObject, 1, position, 0.0f, 9.0f, 0.0f, 1, 0);
    SetModelMotionWithTransition(menuModelObjects[0].ownerObject, 1, 1, 0xF, 1);
    WaitFrames(0x3C);
}

void PositionMenuModelsForPractice(void)
{
    Hu3DModelPosSet(menuModelObjects[0].ownerObject->mdlId[1], -1000.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(menuModelObjects[1].ownerObject->mdlId[1], -400.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(menuModelObjects[2].ownerObject->mdlId[1], 0.0f, 1000.0f, 0.0f);
    Hu3DModelPosSet(playerCharacterObjects[0].ownerObject->mdlId[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelAttrReset(playerCharacterObjects[0].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
}

s32 RunMenuResultsSequence(void)
{
    s32 currentValue = 0;
    RunPracticeMenuResultsSequence();
    WaitFrames(0x3C);
    currentValue = 0;
    return currentValue;
}

// similar to AnimateMenuModelEntry
void AnimateMenuModelExit(void)
{
    Vec position = { 0.0f, 0.0f, 0.0f };

    playerCharacterObjects[0].updateCallback = (MpexObjectUpdateCallback)AnimateMenuCharacterEntry;

    do {
        WaitFrames(1);
    } while (playerCharacterObjects[0].updateCallback != NULL);

    Hu3DModelAttrReset(practiceMenuStateWork.ownerObject->mdlId[5], HU3D_MOTATTR_PAUSE);
    SetModelMotionWithTransition(menuModelObjects[1].ownerObject, 1, 2, 0xF, 1);
    MoveModelTowardPosition(menuModelObjects[1].ownerObject, 1, position, 0.0f, 9.0f, 0.0f, 1, 0);
    SetModelMotionWithTransition(menuModelObjects[1].ownerObject, 1, 1, 0xF, 1);
    WaitFrames(0x3C);
}

void ExitToMainMenu(void)
{
    ReturnToMainMenu();
}

void ShowSingleMenuModel(void)
{
    s32 modelIndex;

    Hu3DModelPosSet(menuModelObjects[0].ownerObject->mdlId[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(menuModelObjects[1].ownerObject->mdlId[1], 1000.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(menuModelObjects[2].ownerObject->mdlId[1], 0.0f, 1000.0f, 0.0f);
    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
        Hu3DModelAttrSet(playerCharacterObjects[modelIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    }
    CreateHighScoreDisplay();
    PositionMinigameNameDisplay(practiceMenuSelection);
}

void ShowAlternateMenuModel(void)
{
    s32 modelIndex;

    Hu3DModelPosSet(menuModelObjects[0].ownerObject->mdlId[1], -1000.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(menuModelObjects[1].ownerObject->mdlId[1], 0.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(menuModelObjects[2].ownerObject->mdlId[1], 0.0f, 1000.0f, 0.0f);
    for (modelIndex = 0; modelIndex < 4; modelIndex++) {
        Hu3DModelAttrSet(playerCharacterObjects[modelIndex].ownerObject->mdlId[1], HU3D_ATTR_DISPOFF);
    }
    CreateHighScoreDisplay();
    PositionMinigameNameDisplay(practiceMenuSelection);
}

void UpdateMinigameMenuObject(OMOBJ *object, MpexMenuObjectWork *menuWork)
{
    s32 minigameValue = 0;
    s32 minigameValue2 = 0;
    s32 minigameValue3 = 0;

    if (omovlevtno == 1) {
        practiceMenuSelection = omovlstat % 10;
        if (practiceMenuSelection <= 3) {
            practiceMenuState = 0;
        }
        else {
            practiceMenuState = 1;
        }
        currentMenuPlayer = omovlstat / 10;
        if (mgQuitExtraF == 1) {
            mgQuitExtraF = 0;
            Hu3DMotionTimeSet(practiceMenuStateWork.ownerObject->mdlId[5], Hu3DMotionMotionMaxTimeGet(practiceMenuStateWork.ownerObject->mdlId[5]) - 5.0f);
            if (practiceMenuSelection <= 3) {
                InitializePracticeMenuModels();
                activeMessageId = practiceMenuSelection;
                ShowSingleMenuModel();
                minigameValue = 0x320;
            }
            else {
                activeMessageId = practiceMenuSelection - 4;
                ShowAlternateMenuModel();
                minigameValue = 0x384;
            }
        }
        else if (practiceMenuSelection <= 3) {
            activeMessageId = practiceMenuSelection;
            if (practiceMenuSelection == 2) {
                Hu3DMotionTimeSet(practiceMenuStateWork.ownerObject->mdlId[5], Hu3DMotionMotionMaxTimeGet(practiceMenuStateWork.ownerObject->mdlId[5]) - 5.0f);
                HideMenuModels();
                minigameValue = 0x1FE;
            }
            else {
                Hu3DMotionTimeSet(practiceMenuStateWork.ownerObject->mdlId[5], 33.0f);
                MoveMenuModelsOnscreen();
                minigameValue = 0x1F4;
            }
        }
        else {
            activeMessageId = practiceMenuSelection - 4;
            Hu3DMotionTimeSet(practiceMenuStateWork.ownerObject->mdlId[5], 33.0f);
            PositionMenuModelsForPractice();
            minigameValue = 0x258;
        }
    }
    else if (omovlevtno == 2) {
        practiceMenuSelection = 0;
        practiceMenuState = 0;
        minigameValue = 1;
    }
    else {
        minigameValue = 0;
    }
    while (TRUE) {
        WaitFrames(1);
        switch (minigameValue) {
            case 0x0:
                StartMinigameMenuSequence();
                SetModelMotionWithTransition(practiceMenuStateWork.ownerObject, 5, 5, 0, 0);
                WaitFrames(0x3C);
                RunPrimaryGameModeMenu();
                minigameValue = 0x64;
                break;
            case 0x1:
                Hu3DMotionTimeSet(practiceMenuStateWork.ownerObject->mdlId[5], Hu3DMotionMotionMaxTimeGet(practiceMenuStateWork.ownerObject->mdlId[5]) - 5.0f);
                StartMinigameMenuSequence();
                minigameValue = 0x64;
                break;
            case 0x64:
                CreatePlayerStatusDisplay();
                RunPracticeMenuSelection();
                DestroyPlayerStatusDisplay();
                minigameValue3 = 0;
                minigameValue = 0x65;
                break;
            case 0x65:
                SetMenuObjectsToEnterCallbacks();

                if (practiceMenuState == 2) {
                    minigameValue = (practiceMenuState * 0x64) + 0xC9;
                }
                else {
                    minigameValue = (practiceMenuState * 0x64) + 0xC8;
                }
                break;
            case 0x66:
                SetMenuObjectsToExitCallbacks();
                minigameValue = 0x64;
                break;
            case 0xC8:
                minigameValue2 = RunCharacterSelectForPractice(minigameValue3);
                if (minigameValue2 != 0) {
                    minigameValue3 = 0;
                    minigameValue = 0xC9;
                }
                else {
                    minigameValue3 = 1;
                    minigameValue = 0x66;
                }
                break;
            case 0xC9:
                CreatePlayerNameSprites();
                minigameValue2 = RunPracticeSelectionFlow(minigameValue3);
                DestroyPlayerNameDisplay();
                if (minigameValue2 != 0) {
                    CreateHighScoreDisplay();
                    minigameValue3 = 0;
                    minigameValue = 0xCA;
                }
                else {
                    minigameValue3 = 1;
                    minigameValue = 0x66;
                }
                break;
            case 0xCA:
                minigameValue2 = RunMinigameDescriptionPrompt(minigameValue3);
                if (minigameValue2 != 0) {
                    minigameValue3 = 0;
                    minigameValue = 0xCB;
                }
                else {
                    DestroyHighScoreDisplay();
                    minigameValue3 = 1;
                    minigameValue = 0xC9;
                }
                break;
            case 0xCB:
                minigameValue2 = RunPracticeOptionPrompt(minigameValue3);
                if (minigameValue2 != 0) {
                    minigameValue3 = 0;
                    minigameValue = 0xCC;
                }
                else {
                    minigameValue3 = 1;
                    minigameValue = 0xCA;
                }
                break;
            case 0xCC:
                InitializePracticePlayers(practiceMenuSelection, currentMenuPlayer);
                minigameValue = -1;
                break;
            case 0x12C:
                minigameValue2 = RunCharacterSelectForMainGame(minigameValue3);
                if (minigameValue2 != 0) {
                    minigameValue3 = 0;
                    minigameValue = 0x12D;
                }
                else {
                    minigameValue3 = 1;
                    minigameValue = 0x66;
                }
                break;
            case 0x12D:
                CreatePlayerScoreSprites();
                minigameValue2 = RunMainGameSelectionFlow(minigameValue3);
                DestroyPlayerScoreDisplay();
                if (minigameValue2 != 0) {
                    CreateHighScoreDisplay();
                    minigameValue3 = 0;
                    minigameValue = 0x12E;
                }
                else {
                    minigameValue3 = 1;
                    minigameValue = 0x66;
                }
                break;
            case 0x12E:
                minigameValue2 = WaitForMinigameDescriptionPrompt(minigameValue3);
                if (minigameValue2 != 0) {
                    minigameValue3 = 0;
                    minigameValue = 0x12F;
                }
                else {
                    DestroyHighScoreDisplay();
                    minigameValue3 = 1;
                    minigameValue = 0x12D;
                }
                break;
            case 0x12F:
                minigameValue2 = WaitForPracticeOptionPrompt(minigameValue3);
                if (minigameValue2 != 0) {
                    minigameValue3 = 0;
                    minigameValue = 0x130;
                }
                else {
                    minigameValue3 = 1;
                    minigameValue = 0x12E;
                }
                break;
            case 0x130:
                InitializePracticePlayers(practiceMenuSelection, currentMenuPlayer);
                minigameValue = -1;
                continue;
            case 0x191:
                CreateHighScoreDisplay();
                minigameValue2 = SetPracticeMenuMode(minigameValue3);
                if (minigameValue2 != 0) {
                    minigameValue3 = 0;
                    minigameValue = 0x192;
                }
                else {
                    DestroyHighScoreDisplay();
                    minigameValue3 = 1;
                    minigameValue = 0x66;
                }
                break;
            case 0x192:
                ReturnToMinigameMenu();
            case 0x1F4:
                StartMinigameMenuSequence();
                minigameValue2 = RunMinigameMenuIntro();
                if (minigameValue2 == 1) {
                    minigameValue = 0xCC;
                }
                else if (minigameValue2 == 0) {
                    minigameValue = 0x1F5;
                }
                else {
                    minigameValue = 0x2BC;
                }
                break;
            case 0x1F5:
                AnimateMenuModelEntry();
                minigameValue = 0xC9;
                break;
            case 0x1FE:
                StartMinigameMenuSequence();
                minigameValue = 0xC9;
                break;
            case 0x258:
                StartMinigameMenuSequence();
                minigameValue2 = RunMenuResultsSequence();
                if (minigameValue2 == 1) {
                    minigameValue = 0x130;
                }
                else if (minigameValue2 == 0) {
                    minigameValue = 0x259;
                }
                else {
                    minigameValue = 0x2BC;
                }
                break;
            case 0x259:
                AnimateMenuModelExit();
                minigameValue = 0x12D;
                break;
            case 0x2BC:
                ReturnToMainMenu();
            case 0x320:
                StartMinigameMenuSequence();
                minigameValue = 0xCA;
                minigameValue3 = 1;
                break;
            case 0x384:
                StartMinigameMenuSequence();
                minigameValue = 0x12E;
                minigameValue3 = 1;
                break;
            default:
                break;
        }
    }
}

void CreateMinigameMenuCharacter(OMOBJ *object)
{
    s32 modelIndex;

    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x00));
    object->mtnId[1] = Hu3DMotionIDGet(object->mdlId[1]);
    Hu3DModelShadowMapObjSet(object->mdlId[1], "extra_ent_fix0-obj41");
    SetModelMotionWithTransition(object, 1, 1, 0, 1);
    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
        object->mdlId[modelIndex + 1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x02));
        Hu3DModelAttrSet(object->mdlId[modelIndex + 1], HU3D_ATTR_DISPOFF);
    }
    object->mdlId[5] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x01));
    object->mtnId[5] = Hu3DMotionIDGet(object->mdlId[5]);
    Hu3DModelPosSet(object->mdlId[5], 0.0f, 0.0f, 270.0f);
    if (omovlevtno == 2) {
        SetModelMotionWithTransition(object, 5, 5, 0, 0);
        return;
    }
    if (omovlevtno == 1) {
        Hu3DModelAttrSet(object->mdlId[5], HU3D_MOTATTR_PAUSE);
        return;
    }
    SetModelMotionWithTransition(object, 5, 5, 0, 2);
}

void CreateMenuCharactersProcess(void)
{
    OMOBJ *object;
    OMOBJ *object2;
    s32 minigameValue;

    object = NULL;
    object = omAddObjEx(minigamePracticeProcess, 0x2000, 0x10, 0x10, -1, NULL);
    practiceMenuStateWork.ownerObject = object;
    practiceMenuStateWork.updateCallback = NULL;
    object2 = practiceMenuStateWork.ownerObject;
    CreateMinigameMenuCharacter(object2);
    while (TRUE) {
        WaitFrames(1);
        if (practiceMenuStateWork.updateCallback == NULL) {
            continue;
        }
        practiceMenuStateWork.updateCallback(practiceMenuStateWork.ownerObject, &practiceMenuStateWork);
    }
}

void AnimateMenuCharacterEntry(OMOBJ *object, MpexMenuObjectWork *menuWork)
{
    Vec modelRotation = { 700.0f, 0.0f, 0.0f };
    WaitFrames((4 - object->work[0]) * 5);
    SetModelMotionWithTransition(object, 1, 2, 0xA, 1);
    object->work[1] = 0;
    while (object->work[1] <= 0xA) {
        WaitFrames(1);
        Hu3DData[object->mdlId[1]].rot.y = InterpolateLinear(0.0f, 90.0f, object->work[1], 10.0f);
        object->work[1]++;
    }

    SetModelMotionWithTransition(object, 1, 3, 0xA, 1);
    MoveModelTowardPosition(object, 1, modelRotation, 0.0f, 9.0f, 0.0f, 1, 0);
    SetModelMotionWithTransition(object, 1, 1, 0xA, 1);
    menuWork->updateCallback = NULL;
}

void CreatePlayerCharacterModel(OMOBJ *object, s32 playerIndex)
{
    s32 playerIndex = GWPlayerCfg[playerIndex].character;
    object->mdlId[1] = CharModelCreate(playerIndex, 4);
    object->mtnId[1] = CharMotionCreate(playerIndex, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00));
    object->mtnId[2] = CharMotionCreate(playerIndex, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02));
    object->mtnId[3] = CharMotionCreate(playerIndex, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03));
    object->mtnId[4] = CharMotionCreate(playerIndex, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x48));
    object->mtnId[5] = CharMotionCreate(playerIndex, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x49));
    Hu3DModelPosSet(object->mdlId[1], -150.0f + (100.0f * playerIndex), 0.0f, 0.0f);
    Hu3DModelShadowSet(object->mdlId[1]);
    Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    CharModelLayerSetAll2(2);
    SetModelMotionWithTransition(object, 1, 1, 0, 1);
    CharDataClose(playerIndex);
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x02));
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
}

void CreatePracticePlayerModelsProcess(void)
{
    MpexMenuObjectWork *menuObjectWork;
    OMOBJ *object;
    OMOBJ *object2;
    s16 minigameValue;
    u32 minigameValue2;

    minigameValue2 = menuObjectCreateIndex;
    menuObjectCreateIndex = minigameValue2 + 1;
    object = NULL;
    object = omAddObjEx(minigamePracticeProcess, 0x2000, 0x10, 0x10, -1, NULL);
    playerCharacterObjects[minigameValue2].ownerObject = object;
    playerCharacterObjects[minigameValue2].updateCallback = NULL;
    playerCharacterObjects[minigameValue2].ownerObject->work[0] = minigameValue2;
    object2 = playerCharacterObjects[minigameValue2].ownerObject;
    CreatePlayerCharacterModel(object2, minigameValue2);
    while (TRUE) {
        WaitFrames(1);
        if (playerCharacterObjects[minigameValue2].updateCallback == NULL) {
            continue;
        }
        playerCharacterObjects[minigameValue2].updateCallback(playerCharacterObjects[minigameValue2].ownerObject, &playerCharacterObjects[minigameValue2]);
    }
}

void CreateMenuMinigameModelA(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x03));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MPEX, 0x04));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MPEX, 0x06));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MPEX, 0x06));
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MPEX, 0x07));
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MPEX, 0x08));
    Hu3DModelPosSet(object->mdlId[1], -200.0f, 0.0f, 0.0f);
    Hu3DModelShadowSet(object->mdlId[1]);
    CharNpcDustSet(object->mdlId[1], object->mtnId[2], 0, 0xF);
    SetModelMotionWithTransition(object, 1, 1, 0, 1);
}

void CreateMenuMinigameModelB(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x09));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MPEX, 0x0A));
    object->mtnId[2] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MPEX, 0x0B));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MPEX, 0x0C));
    object->mtnId[4] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MPEX, 0x0D));
    object->mtnId[5] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MPEX, 0x0E));
    Hu3DModelPosSet(object->mdlId[1], 200.0f, 0.0f, 0.0f);
    Hu3DModelShadowSet(object->mdlId[1]);
    CharNpcDustSet(object->mdlId[1], object->mtnId[2], 0, 0x10);
    SetModelMotionWithTransition(object, 1, 1, 0, 1);
}

void UpdateMenuModelSineMotion(OMOBJ *object)
{
    float interpolatedValue;

    interpolatedValue = 0.0f;
    interpolatedValue = CalculateSineWaveValue(0.0f, 1.0f, object->work[3], 180.0f);
    Hu3DData[object->mdlId[1]].pos.y += interpolatedValue;
    if (object->work[3]++ >= 0xB4) {
        object->work[3] -= 0xB4;
    }
}

void CreateMenuMinigameModelC(OMOBJ *object)
{
    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x0F));
    object->mtnId[1] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MPEX, 0x10));
    object->mtnId[3] = Hu3DJointMotionFile(object->mdlId[1], DATA_MAKE_NUM(DATADIR_MPEX, 0x11));
    Hu3DModelPosSet(object->mdlId[1], 0.0f, 400.0f, 0.0f);
    if (availableMinigameFlags[0xA] == 0) {
        Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
    }
    SetModelMotionWithTransition(object, 1, 1, 0, 1);
    object->objFunc = UpdateMenuModelSineMotion;
}

void CreateMenuMinigameModelsProcess(void)
{
    OMOBJ *object;
    OMOBJ *object2;
    OMOBJ *object3;
    s32 minigameValue;
    OMOBJ *object4;

    minigameValue = playerStatusAnimationIndex;
    playerStatusAnimationIndex = minigameValue + 1;
    object4 = NULL;
    object4 = omAddObjEx(minigamePracticeProcess, 0x2000, 0x10, 0x10, -1, NULL);
    menuModelObjects[minigameValue].ownerObject = object4;
    menuModelObjects[minigameValue].updateCallback = 0;
    menuModelObjects[minigameValue].ownerObject->work[0] = minigameValue;
    switch (minigameValue) {
        case 0:
            object = menuModelObjects[minigameValue].ownerObject;
            CreateMenuMinigameModelA(object);
            break;
        case 1:
            object2 = menuModelObjects[minigameValue].ownerObject;
            CreateMenuMinigameModelB(object2);
            break;
        case 2:
            object3 = menuModelObjects[minigameValue].ownerObject;
            CreateMenuMinigameModelC(object3);
            break;
    }
    while (TRUE) {
        WaitFrames(1);
        if (menuModelObjects[minigameValue].updateCallback == NULL) {
            continue;
        }
        menuModelObjects[minigameValue].updateCallback(menuModelObjects[minigameValue].ownerObject, &menuModelObjects[minigameValue]);
    }
}

void MoveMenuModelWithSineEasing(OMOBJ *object, MpexMenuObjectWork *menuWork)
{
    float interpolatedValue;

    s32 modelIndex;
    s32 *unusedMotionStatePointer;
    s32 *unusedModelScalePointer;
    s32 *unusedModelRotationPointer;
    s32 *unusedModelPositionPointer;
    void *unusedCallbackContext;

    Vec position = { 0.0f, 0.0f, 0.0f };
    Vec position2 = { -1000.0f, 0.0f, 0.0f };
    Vec position3 = { 1000.0f, 0.0f, 0.0f };
    switch (object->work[0]) {
        case 0:
            SetModelMotionWithTransition(object, 1, 3, 0xF, 1);
            if (practiceMenuState == 0) {
                MoveModelTowardPosition(object, 1, position, 0.0f, 9.0f, 0.0f, 1, 0);
            }
            else {
                MoveModelTowardPosition(object, 1, position2, 0.0f, 9.0f, 0.0f, 1, 0);
            }
            SetModelMotionWithTransition(object, 1, 1, 0xF, 1);
            break;
        case 1:
            SetModelMotionWithTransition(object, 1, 2, 0xF, 1);
            if (practiceMenuState == 1) {
                MoveModelTowardPosition(object, 1, position, 0.0f, 9.0f, 0.0f, 1, 0);
            }
            else {
                MoveModelTowardPosition(object, 1, position3, 0.0f, 9.0f, 0.0f, 1, 0);
            }
            SetModelMotionWithTransition(object, 1, 1, 0xF, 1);
            break;
        case 2:
            if (practiceMenuState == 2) {
                interpolatedValue = Hu3DData[object->mdlId[1]].pos.y;
                for (modelIndex = 0; modelIndex <= 0x78; modelIndex++) {
                    WaitFrames(1);
                    Hu3DData[object->mdlId[1]].pos.y = InterpolateSine(interpolatedValue, 250.0f, modelIndex, 120.0f);
                }
            }
            else {
                interpolatedValue = Hu3DData[object->mdlId[1]].pos.y;
                for (modelIndex = 0; modelIndex <= 0x78; modelIndex++) {
                    WaitFrames(1);
                    Hu3DData[object->mdlId[1]].pos.y = InterpolateSine(interpolatedValue, 1000.0f, modelIndex, 120.0f);
                }
            }
            break;
    }
    menuWork->updateCallback = 0;
}

void MoveMenuModelWithCosineEasing(OMOBJ *object, MpexMenuObjectWork *menuWork)
{
    float interpolatedValue;
    s32 modelIndex;
    s32 *unusedMotionStatePointer;
    s32 *unusedModelPositionPointer;

    Vec position = { 0.0f, 0.0f, 0.0f };
    Vec position2 = { -200.0f, 0.0f, 0.0f };
    Vec position3 = { 200.0f, 0.0f, 0.0f };
    switch (object->work[0]) {
        case 0:
            SetModelMotionWithTransition(object, 1, 2, 0xF, 1);
            MoveModelTowardPosition(object, 1, position2, 0.0f, 9.0f, 0.0f, 1, 0);
            SetModelMotionWithTransition(object, 1, 1, 0xF, 1);
            break;
        case 1:
            SetModelMotionWithTransition(object, 1, 2, 0xF, 1);
            MoveModelTowardPosition(object, 1, position3, 0.0f, 9.0f, 0.0f, 1, 0);
            SetModelMotionWithTransition(object, 1, 1, 0xF, 1);
            break;
        case 2:
            interpolatedValue = Hu3DData[object->mdlId[1]].pos.y;
            for (modelIndex = 0; modelIndex <= 0x78; modelIndex++) {
                WaitFrames(1);
                Hu3DData[object->mdlId[1]].pos.y = InterpolateCosine(interpolatedValue, 400.0f, modelIndex, 120.0f);
            }
            break;
    }
    menuWork->updateCallback = 0;
}

void InitializePracticeCamera(void)
{
    MpexCameraWork *cameraState = &cameraWork;
    cameraState->currentCenter.x = 0.0f;
    cameraState->currentCenter.y = 165.0f;
    cameraState->currentCenter.z = 0.0f;
    cameraState->currentRotation.x = 0.0f;
    cameraState->currentRotation.y = 0.0f;
    cameraState->currentRotation.z = 0.0f;
    cameraState->currentZoom = 1200.0f;
}

s32 playerOrderByRank[4] = { -1, -1, -1, -1 };
s32 selectedPlayerCount = -1;

void SetPracticeMenuModeAndPlayerCount(s32 selectionMode)
{
    s32 scoreValues[4];
    s32 loopIndex;
    s32 scoreValue;
    s32 scoreValue2;

    scoreValue2 = 0;
    scoreValue = 0;
    activePlayerStatusCount = 0;
    playerOrderByRank[0] = playerOrderByRank[1] = playerOrderByRank[2] = playerOrderByRank[3] = -1;
    for (loopIndex = 0; loopIndex < 4; loopIndex++) {
        scoreValues[loopIndex] = GetPlayerCoinScore(selectionMode, loopIndex);
    }
    for (loopIndex = 0; loopIndex < 4; loopIndex++) {
        if (scoreValues[loopIndex] >= scoreValue) {
            scoreValue = scoreValues[loopIndex];
        }
    }
    for (loopIndex = 0; loopIndex < 4; loopIndex++) {
        if (scoreValue == scoreValues[loopIndex]) {
            activePlayerStatusCount++;
            playerOrderByRank[scoreValue2++] = loopIndex;
        }
    }
    selectedPlayerCount = scoreValue;
}

void ShowMinigameRecordMessage(void)
{
    CreateCharacterSelectMessageWindow();
    SetModelMotionWithTransition(menuModelObjects[0].ownerObject, 1, 3, 0xF, 1);
    ShowCharacterSelectMessage(0x33000C, -1, -1);
    SetModelMotionWithTransition(menuModelObjects[0].ownerObject, 1, 1, 0xF, 1);
    DestroyCharacterSelectMessageWindow();
}

void ShowMinigameResultMessage(void)
{
    CreateCharacterSelectMessageWindow();
    HuAudFXPlay(0x4A);
    SetModelMotionWithTransition(menuModelObjects[1].ownerObject, 1, 3, 0xF, 1);
    ShowCharacterSelectMessage(0x33001B, -1, -1);
    SetModelMotionWithTransition(menuModelObjects[1].ownerObject, 1, 1, 0xF, 1);
    DestroyCharacterSelectMessageWindow();
}

void ShowMinigameRecordDetails(s32 recordIndex)
{
    s32 minigameValue;
    u32 minigameValue2;
    u32 minigameValue3;

    CreateCharacterSelectMessageWindow();
    if (((mgRecordExtra == 0) && (recordIndex != 4)) || ((mgRecordExtra >= 600*REFRESH_RATE) && (recordIndex == 4))) {
        HuAudFXPlay(0x4A);
        SetModelMotionWithTransition(menuModelObjects[1].ownerObject, 1, 3, 0xF, 1);
        ShowCharacterSelectMessage(recordIndex + 0x330096, -1, -1);
        SetModelMotionWithTransition(menuModelObjects[1].ownerObject, 1, 1, 0xF, 1);
    }
    else {
        if ((recordIndex == 4) || (recordIndex == 8)) {
            minigameValue = mgRecordExtra;
            minigameValue2 = minigameValue / (600*REFRESH_RATE);
            minigameValue -= minigameValue2 * (600*REFRESH_RATE);
            minigameValue3 = minigameValue / (60*REFRESH_RATE);
            minigameValue -= minigameValue3 * (60*REFRESH_RATE);
            sprintf(formattedPlayerRank, "%d", minigameValue3);
            InsertCharacterSelectMessage((u32)&formattedPlayerRank, 0);
            minigameValue2 = minigameValue / (10*REFRESH_RATE);
            minigameValue -= minigameValue2 * (10*REFRESH_RATE);
            minigameValue3 = minigameValue / REFRESH_RATE;
            minigameValue -= minigameValue3 * REFRESH_RATE;
            sprintf(formattedPlayerCoins, "%d%d", minigameValue2, minigameValue3);
            InsertCharacterSelectMessage((u32)formattedPlayerCoins, 1);
            if (recordIndex == 8) {
                minigameValue = (101.5/REFRESH_RATE) * (float)minigameValue;
            }
            else {
                minigameValue = (100.0/REFRESH_RATE) * (float)minigameValue;
            }
            minigameValue2 = minigameValue / 10;
            minigameValue -= minigameValue2 * 0xA;
            minigameValue3 = minigameValue;
            sprintf(formattedCoinDigits, "%d%d", minigameValue2, minigameValue3);
            InsertCharacterSelectMessage((u32)formattedCoinDigits, 2);
        }
        else {
            sprintf(formattedCoinDigits, "%d", mgRecordExtra);
            InsertCharacterSelectMessage((u32)formattedCoinDigits, 0);
        }
        HuAudFXPlay(0x4A);
        SetModelMotionWithTransition(menuModelObjects[1].ownerObject, 1, 3, 0xF, 1);
        ShowCharacterSelectMessage(recordIndex + 0x330018, -1, -1);
        SetModelMotionWithTransition(menuModelObjects[1].ownerObject, 1, 1, 0xF, 1);
    }
    DestroyCharacterSelectMessageWindow();
}

void ShowAdditionalMinigameRecord(s32 recordIndex)
{
    CreateCharacterSelectMessageWindow();
    InsertCharacterSelectMessage(GetMinigameScoreMessageId(recordIndex, mgRecordExtra), 3);
    SetModelMotionWithTransition(menuModelObjects[1].ownerObject, 1, 3, 0xF, 1);
    ShowCharacterSelectMessage(0x330090, -1, -1);
    SetModelMotionWithTransition(menuModelObjects[1].ownerObject, 1, 1, 0xF, 1);
    DestroyCharacterSelectMessageWindow();
}

void ShowPracticeMenuResults(void)
{
    CreateCharacterSelectMessageWindow();
    switch (activePlayerStatusCount) {
        case 1:
            InsertCharacterSelectMessage(GWPlayerCfg[playerOrderByRank[0]].character, 0);
            ShowCharacterSelectMessage(0x33000D, -1, -1);
            break;
        case 2:
            InsertCharacterSelectMessage(GWPlayerCfg[playerOrderByRank[0]].character, 0);
            InsertCharacterSelectMessage(GWPlayerCfg[playerOrderByRank[1]].character, 1);
            ShowCharacterSelectMessage(0x33000E, -1, -1);
            break;
        case 3:
            InsertCharacterSelectMessage(GWPlayerCfg[playerOrderByRank[0]].character, 0);
            InsertCharacterSelectMessage(GWPlayerCfg[playerOrderByRank[1]].character, 1);
            InsertCharacterSelectMessage(GWPlayerCfg[playerOrderByRank[2]].character, 2);
            ShowCharacterSelectMessage(0x33000F, -1, -1);
            break;
        case 4:
            ShowCharacterSelectMessage(0x330010, -1, -1);
            break;
    }
    DestroyCharacterSelectMessageWindow();
}

void UpdateMenuModelTransforms(OMOBJ *object, MpexMenuObjectWork *menuWork)
{
    float angle;
    s32 modelIndex;

    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
        angle = 30.0 * sind(8.0f * (menuModelAnimationPhase + (modelIndex * 0x78)));
        Hu3DModelPosSet(object->mdlId[modelIndex + 1], -200.0f + (200.0f * modelIndex), 850.0f, 0.0f);
        Hu3DModelRotSet(object->mdlId[modelIndex + 1], 0.0f, 0.0f, angle);
        Hu3DModelScaleSet(object->mdlId[modelIndex + 1], 1.5f, 2.0f, 1.5f);
    }
    menuModelAnimationPhase += 0.5f;
}

void AnimateMenuModelFadeIn(OMOBJ *object, MpexMenuObjectWork *menuWork)
{
    float interpolatedValue;
    float interpolatedValue2;
    s32 frameIndex;

    Hu3DModelAttrReset(object->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelTPLvlSet(object->mdlId[2], 0.0f);
    for (frameIndex = 0; frameIndex <= 0xF; frameIndex++) {
        WaitFrames(1);
        interpolatedValue2 = InterpolateLinear(0.0f, 0.5f, frameIndex, 15.0f);
        interpolatedValue = InterpolateLinear(0.0f, 1.5f, frameIndex, 15.0f);
        Hu3DModelPosSet(object->mdlId[2], Hu3DData[object->mdlId[1]].pos.x, 850.0f, Hu3DData[object->mdlId[1]].pos.z);
        Hu3DModelScaleSet(object->mdlId[2], interpolatedValue, 2.0f, interpolatedValue);
        Hu3DModelTPLvlSet(object->mdlId[2], interpolatedValue2);
    }
    menuWork->updateCallback = NULL;
}

void AnimateMenuModelFadeOut(OMOBJ *object, MpexMenuObjectWork *menuWork)
{
    float interpolatedValue;
    s32 frameIndex;

    for (frameIndex = 0; frameIndex <= 0xF; frameIndex++) {
        WaitFrames(1);
        interpolatedValue = InterpolateLinear(0.5f, 0.0f, frameIndex, 15.0f);
        Hu3DModelTPLvlSet(object->mdlId[2], interpolatedValue);
    }
    Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
    Hu3DModelTPLvlSet(object->mdlId[2], 0.0f);
    menuWork->updateCallback = NULL;
}

void StopMenuModelFadeProcess(void)
{
    float interpolatedValue;
    s32 modelIndex;
    s32 modelIndex2;

    practiceMenuStateWork.updateCallback = (MpexObjectUpdateCallback)UpdateMenuModelTransforms;
    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
        Hu3DModelAttrReset(practiceMenuStateWork.ownerObject->mdlId[modelIndex + 1], HU3D_ATTR_DISPOFF);
        Hu3DModelTPLvlSet(practiceMenuStateWork.ownerObject->mdlId[modelIndex + 1], 0.0f);
    }
    for (modelIndex = 0; modelIndex <= 0x3C; modelIndex++) {
        WaitFrames(1);
        interpolatedValue = InterpolateLinear(0.0f, 0.5f, modelIndex, 60.0f);
        for (modelIndex2 = 0; modelIndex2 < 3; modelIndex2++) {
            Hu3DModelTPLvlSet(practiceMenuStateWork.ownerObject->mdlId[modelIndex2 + 1], interpolatedValue);
        }
    }
    WaitFrames(0x5A);
    for (modelIndex = 0; modelIndex <= 0x3C; modelIndex++) {
        WaitFrames(1);
        interpolatedValue = InterpolateLinear(0.5f, 0.0f, modelIndex, 60.0f);
        for (modelIndex2 = 0; modelIndex2 < 3; modelIndex2++) {
            Hu3DModelTPLvlSet(practiceMenuStateWork.ownerObject->mdlId[modelIndex2 + 1], interpolatedValue);
        }
    }
    for (modelIndex = 0; modelIndex < 3; modelIndex++) {
        Hu3DModelAttrSet(practiceMenuStateWork.ownerObject->mdlId[modelIndex + 1], HU3D_ATTR_DISPOFF);
        Hu3DModelTPLvlSet(practiceMenuStateWork.ownerObject->mdlId[modelIndex + 1], 0.0f);
    }
    practiceMenuStateWork.updateCallback = NULL;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

void RunPracticeMenuState(void)
{
    s32 frameIndex;
    s32 frameIndex2;

    SetPracticeMenuModeAndPlayerCount(practiceMenuSelection);
    OSReport("CNT:%d, WIN:%d %d %d %d\n", activePlayerStatusCount, playerOrderByRank[0], playerOrderByRank[1], playerOrderByRank[2], playerOrderByRank[3]);
    ShowMinigameRecordMessage();
    if (practiceMenuSequenceId != -1) {
        HuAudSeqPause(practiceMenuSequenceId, 1, 0x1F4);
    }
    HuPrcChildCreate(StopMenuModelFadeProcess, 0x3E8, 0x1000, 0, HuPrcCurrentGet());
    WaitFrames(0xF);
    HuAudFXPlay(0x9E);
    WaitFrames(0xA5);
    HuAudFXPlay(0x9F);
    HuAudFXPlay(0xA8);
    HuAudFXPlay(0x9A);
    if (practiceMenuSequenceId != -1) {
        HuAudSeqPause(practiceMenuSequenceId, 0, 0x7D0);
    }
    if ((activePlayerStatusCount >= 1) && (selectedPlayerCount != 0)) {
        for (frameIndex = 0; frameIndex < activePlayerStatusCount; frameIndex++) {
            playerCharacterObjects[playerOrderByRank[frameIndex]].updateCallback = (MpexObjectUpdateCallback)AnimateMenuModelFadeIn;
            OSReport("CNT:%d, WIN:%d %d %d %d\n", activePlayerStatusCount, playerOrderByRank[0], playerOrderByRank[1], playerOrderByRank[2], playerOrderByRank[3]);
        }
        ShowPracticeMenuResults();
        for (frameIndex = 0; frameIndex < 4; frameIndex++) {
            for (frameIndex2 = 0; frameIndex2 < 4; frameIndex2++) {
                if (frameIndex == playerOrderByRank[frameIndex2]) {
                    SetModelMotionWithTransition(playerCharacterObjects[frameIndex].ownerObject, 1, 4, 0xF, 0);
                    break;
                }
            }
            if (frameIndex2 == 4) {
                SetModelMotionWithTransition(playerCharacterObjects[frameIndex].ownerObject, 1, 5, 0xF, 0);
            }
        }
    }
    else {
        CreateCharacterSelectMessageWindow();
        SetModelMotionWithTransition(menuModelObjects->ownerObject, 1, 3, 0xF, 1);
        ShowCharacterSelectMessage(practiceMenuSelection + MAKE_MESSID(0x33, 0x96), -1, -1);
        SetModelMotionWithTransition(menuModelObjects->ownerObject, 1, 1, 0xF, 1);
        DestroyCharacterSelectMessageWindow();
        for (frameIndex = 0; frameIndex < 4; frameIndex++) {
            SetModelMotionWithTransition(playerCharacterObjects[frameIndex].ownerObject, 1, 5, 0xF, 0);
        }
    }
    WaitFrames(0x3C);
    for (frameIndex = 0; frameIndex < 4; frameIndex++) {
        SetModelMotionWithTransition(playerCharacterObjects[frameIndex].ownerObject, 1, 1, 0xF, 1);
    }
    CreateCoinScoreDisplay();
    WaitFrames(1);
    AnimateCoinScoreDisplay(practiceMenuSelection);
    WaitFrames(0x3C);
    WaitFrames(0x3C);
    ShowCharacterSelectWindowMessage(0x240007, 1);
    do {
        WaitFrames(1);
    } while ((HuPadBtnDown[0] & 0x100) == 0);
    HuAudFXPlay(2);
    for (frameIndex = 0; frameIndex < 4; frameIndex++) {
        playerCharacterObjects[frameIndex].updateCallback = (MpexObjectUpdateCallback)AnimateMenuModelFadeOut;
    }
    DestroyCharacterSelectTextWindow();
    AnimateCoinScoreDisplayAlternate(practiceMenuSelection);
    WaitFrames(1);
    DestroyCoinScoreDisplay();
}

void RunPracticeMenuResultsSequence(void)
{
    ShowMinigameResultMessage();
    WaitFrames(0x3C);
    ShowMinigameRecordDetails(practiceMenuSelection);
    if (practiceMenuSequenceId != -1) {
        HuAudSeqPause(practiceMenuSequenceId, 1, 0x1F4);
    }
    WaitFrames(0xF);
    HuAudFXPlay(0x9E);
    WaitFrames(0xA5);
    playerCharacterObjects[0].updateCallback = (MpexObjectUpdateCallback)AnimateMenuModelFadeIn;
    #if VERSION_NTSC
    if (((mgRecordExtra == 0) && (practiceMenuSelection != 4)) || ((mgRecordExtra >= 600*REFRESH_RATE) && (practiceMenuSelection == 4))) {
        SetModelMotionWithTransition(playerCharacterObjects[0].ownerObject, 1, 5, 0xF, 0);
    }
    else {
        SetModelMotionWithTransition(playerCharacterObjects[0].ownerObject, 1, 4, 0xF, 0);
    }
    #else
    if (((mgRecordExtra == 0) && (practiceMenuSelection != 4)) || ((mgRecordExtra >= 500*REFRESH_RATE) && (practiceMenuSelection == 4))) {
        SetModelMotionWithTransition(playerCharacterObjects[0].ownerObject, 1, 5, 0xF, 0);
    }
    else {
        SetModelMotionWithTransition(playerCharacterObjects[0].ownerObject, 1, 4, 0xF, 0);
    } 
    #endif
    CreateMinigameScoreDisplay();
    WaitFrames(1);
    HuAudFXPlay(0x9F);
    HuAudFXPlay(0xA8);
    HuAudFXPlay(0x9A);
    if (practiceMenuSequenceId != -1) {
        HuAudSeqPause(practiceMenuSequenceId, 0, 0x7D0);
    }
    AnimateMinigameScoreDisplay(practiceMenuSelection, rand8());
    WaitFrames(0x3C);
    SetModelMotionWithTransition(playerCharacterObjects[0].ownerObject, 1, 1, 0xF, 1);
    ShowAdditionalMinigameRecord(practiceMenuSelection);
    playerCharacterObjects[0].updateCallback = (MpexObjectUpdateCallback)AnimateMenuModelFadeOut;
    HideMinigameScoreWindow(practiceMenuSelection);
    WaitFrames(1);
    DestroyMinigameScoreDisplay();
}

void CreateCoinScoreDisplay(void)
{
    s32 spriteIndex;
    s32 spriteId;
    s32 spriteGroupId;
    ANIMDATA *animationData;

    spriteGroupId = HuSprGrpCreate(0x1B);
    coinScoreSpriteGroup = spriteGroupId;
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x37)), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x38)), 0x7D0, 0);
    HuSprGrpMemberSet(spriteGroupId, 1, spriteId);
    for (spriteIndex = 0; spriteIndex < 8; spriteIndex++) {
        spriteId = HuSprCreate(HuSprAnimReadFile(spriteIndex + DATA_MAKE_NUM(DATADIR_MPEX, 0x3E)), 0x7D0, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 2, spriteId);
    }
    animationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x3B));
    for (spriteIndex = 0; spriteIndex < 0xC; spriteIndex++) {
        spriteId = HuSprCreate(animationData, 0x384, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0xA, spriteId);
    }
    animationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x3C));
    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        spriteId = HuSprCreate(animationData, 0x384, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0x16, spriteId);
    }
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x3D)), 0x7D0, 0);
    HuSprGrpMemberSet(spriteGroupId, 0x1A, spriteId);
    UpdateCoinScoreDisplay(0);
    HideCoinScoreDisplay(0);
}

void DestroyCoinScoreDisplay(void)
{
    s32 spriteGroupId;

    spriteGroupId = coinScoreSpriteGroup;
    HuSprGrpKill(spriteGroupId);
}

s32 GetPlayerCoinScore(s32 rank, s32 playerIndex)
{
    s32 playerIndex = 0;
    if (practiceMenuSelection <= 1) {
        playerIndex = GWPlayer[playerIndex].coins;
    }
    else {
        playerIndex = GWPlayerCoinWinGet(playerIndex);
        OSReport("########## %d:%d\n", playerIndex, playerIndex);
    }
    return playerIndex;
}

void UpdateCoinScoreDisplay(s32 playerCount)
{
    s32 playerIndex;
    s32 spriteGroupId;
    s32 spriteIndex;
    s32 characterSelectionValue;
    s32 characterSelectionValue2;

    spriteGroupId = coinScoreSpriteGroup;
    for (playerIndex = 0; playerIndex < 0x1B; playerIndex++) {
        HuSprAttrSet(spriteGroupId, playerIndex, 4);
    }
    if (playerCount != -1) {
        HuSprAttrReset(spriteGroupId, 0, 4);
        HuSprPosSet(spriteGroupId, 0, 288.0f, 148.0f);
        HuSprAttrReset(spriteGroupId, 1, 4);
        HuSprPosSet(spriteGroupId, 1, 288.0f, 156.0f);
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            HuSprAttrReset(spriteGroupId, GWPlayerCfg[playerIndex].character + 2, 4);
            HuSprPosSet(spriteGroupId, GWPlayerCfg[playerIndex].character + 2, 201.0f, (playerIndex * 0x2A) + 0x60);
        }
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
                HuSprAttrReset(spriteGroupId, 0xA + spriteIndex + (playerIndex * 3), 4);
                HuSprPosSet(spriteGroupId, 0xA + spriteIndex + (playerIndex * 3), (spriteIndex * 0x1C) + 0xF7, (playerIndex * 0x2A) + 0x60);
            }
            characterSelectionValue2 = GetPlayerCoinScore(0, playerIndex);
            characterSelectionValue = characterSelectionValue2 / 100;
            if (characterSelectionValue == 0) {
                HuSprAttrSet(spriteGroupId, (playerIndex * 3) + 0xA, 4);
            }
            HuSprBankSet(spriteGroupId, (playerIndex * 3) + 0xA, characterSelectionValue);
            characterSelectionValue = (characterSelectionValue2 % 100) / 10;
            if ((characterSelectionValue == 0) && ((characterSelectionValue2 / 100) == 0)) {
                HuSprAttrSet(spriteGroupId, (playerIndex * 3) + 0xB, 4);
            }
            HuSprBankSet(spriteGroupId, (playerIndex * 3) + 0xB, characterSelectionValue);
            characterSelectionValue = characterSelectionValue2 % 10;
            HuSprBankSet(spriteGroupId, (playerIndex * 3) + 0xC, characterSelectionValue);
        }
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            HuSprAttrReset(spriteGroupId, playerIndex + 0x16, 4);
            HuSprPosSet(spriteGroupId, playerIndex + 0x16, 358.0f, (playerIndex * 0x2A) + 0x60);
            switch (playerCount) {
                case 0:
                case 1:
                    HuSprBankSet(spriteGroupId, playerIndex + 0x16, 0);
                    break;
                case 3:
                    HuSprBankSet(spriteGroupId, playerIndex + 0x16, 1);
                    break;
                case 2:
                    break;
            }
        }
        HuSprAttrReset(spriteGroupId, 0x1A, 4);
        HuSprPosSet(spriteGroupId, 0x1A, 288.0f, 55.0f);
        switch (playerCount) {
            case 2:
                break;
            case 0:
                HuSprBankSet(spriteGroupId, 0x1A, 0);
                break;
            case 1:
                HuSprBankSet(spriteGroupId, 0x1A, 1);
                break;
            case 3:
                HuSprBankSet(spriteGroupId, 0x1A, 2);
                break;
        }
        HuSprGrpPosSet(spriteGroupId, 0.0f, -500.0f);
    }
    (void)spriteIndex;
}

void HideCoinScoreDisplay(s32 playerCount)
{
    s32 spriteIndex;
    s32 spriteGroupId;

    spriteGroupId = coinScoreSpriteGroup;
    for (spriteIndex = 0; spriteIndex < 0x1B; spriteIndex++) {
        HuSprAttrSet(spriteGroupId, spriteIndex, 4);
    }
    HuSprGrpPosSet(spriteGroupId, 0.0f, -500.0f);
}

void AnimateCoinScoreDisplay(s32 playerCount)
{
    float verticalOffset;
    s32 frameIndex;
    s32 coinScoreGroup = coinScoreSpriteGroup;

    UpdateCoinScoreDisplay(playerCount);
    for (frameIndex = 0; frameIndex < 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0x32) {
            verticalOffset = InterpolateSine(-500.0f, 10.0f, frameIndex, 50.0f);
        }
        else {
            verticalOffset = InterpolateCosine(10.0f, 0.0f, frameIndex - 0x32, 10.0f);
        }
        HuSprGrpPosSet(coinScoreGroup, 0.0f, verticalOffset);
    }
}

void AnimateCoinScoreDisplayAlternate(s32 playerCount)
{
    float spriteGroupId;
    s32 frameIndex;
    s32 spriteIndex;
    s32 spriteGroupId2;
    s32 spriteGroupId3;

    spriteGroupId3 = coinScoreSpriteGroup;
    for (frameIndex = 0; frameIndex < 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0xA) {
            spriteGroupId = InterpolateSine(0.0f, 10.0f, frameIndex, 10.0f);
        }
        else {
            spriteGroupId = InterpolateCosine(10.0f, -500.0f, frameIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(spriteGroupId3, 0.0f, spriteGroupId);
    }
    spriteGroupId2 = coinScoreSpriteGroup;
    for (spriteIndex = 0; spriteIndex < 0x1B; spriteIndex++) {
        HuSprAttrSet(spriteGroupId2, spriteIndex, 4);
    }
    HuSprGrpPosSet(spriteGroupId2, 0.0f, -500.0f);
}

void GetScoreWindowSize(s32 messageId, s32 windowIndex)
{
    float messageDimensions[2];
    s32 loopIndex;
    HUSPRGRP *spriteGroup;
    HUSPRITE *sprite;

    spriteGroup = &HuSprGrpData[winData[messageId].group];
    HuWinMesMaxSizeGet(1, messageDimensions, windowIndex);
    for (loopIndex = 0; loopIndex < spriteGroup->capacity; loopIndex++) {
        sprite = &HuSprData[spriteGroup->members[loopIndex]];
        sprite->pos.x -= messageDimensions[0] / 2;
        sprite->pos.y -= 30.0f;
    }
}

void CreateMinigameScoreDisplay(void)
{
    s32 spriteId;
    s32 spriteGroupId = HuSprGrpCreate(2);

    minigameScoreSpriteGroup = spriteGroupId;
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x39)), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x3A)), 0x7D0, 0);
    HuSprGrpMemberSet(spriteGroupId, 1, spriteId);
    UpdateMinigameScoreDisplay(-1, 0);
}

void DestroyMinigameScoreDisplay(void)
{
    s32 spriteGroupId = minigameScoreSpriteGroup;
    HuSprGrpKill(spriteGroupId);
}

u32 GetMinigameScoreMessageId(s32 playerCount, s32 scoreType)
{
    s32 minigameValue = GetFormattedHighScore(scoreType, playerCount, currentMenuPlayer, 0);
    return ((playerCount - 4) * 0xA) + MAKE_MESSID(0x33, 0x00) + minigameValue + 0x51;
}

void UpdateMinigameScoreDisplay(s32 playerCount, s32 scoreType)
{
    float messageDimensions[2];
    s32 spriteGroupId;
    s32 spriteIndex;
    u32 windowId;

    spriteGroupId = minigameScoreSpriteGroup;
    for (spriteIndex = 0; spriteIndex < 2; spriteIndex++) {
        HuSprAttrSet(spriteGroupId, spriteIndex, 4);
    }
    if (playerCount != -1) {
        HuSprAttrReset(spriteGroupId, 0, 4);
        HuSprAttrReset(spriteGroupId, 1, 4);
        HuSprBankSet(spriteGroupId, 1, playerCount - 4);
        HuSprGrpPosSet(spriteGroupId, 288.0f, 100.0f);
        HuSprGrpScaleSet(spriteGroupId, 0.0f, 0.0f);
        OSReport("#################### SCORE(%d-%d) : %d\n", playerCount, currentMenuPlayer, mgRecordExtra);
        windowId = GetMinigameScoreMessageId(playerCount, mgRecordExtra);
        HuWinMesMaxSizeGet(1, messageDimensions, windowId);
        scoreSummaryWindow = HuWinCreate(0.0f, 0.0f, messageDimensions[0], messageDimensions[1], 0);
        HuWinBGTPLvlSet(scoreSummaryWindow, 0.0f);
        HuWinMesSpeedSet(scoreSummaryWindow, 0);
        HuWinDispOn(scoreSummaryWindow);
        GetScoreWindowSize(scoreSummaryWindow, windowId);
        HuWinMesSet(scoreSummaryWindow, windowId);
        HuWinPosSet(scoreSummaryWindow, 296.0f, 100.0f);
    }
}

void HideMinigameScoreDisplay(void)
{
    s32 spriteIndex;
    s32 minigameValue = minigameScoreSpriteGroup;
    for (spriteIndex = 0; spriteIndex < 2; spriteIndex++) {
        HuSprAttrSet(minigameValue, spriteIndex, 4);
    }
    HuWinKill(scoreSummaryWindow);
}

void AnimateMinigameScoreDisplay(s32 playerCount, s32 scoreType)
{
    float spriteGroupId;
    float interpolatedValue;
    s32 minigameValue;
    s32 frameIndex;
    s32 spriteGroupId2;

    minigameValue = 0;
    spriteGroupId2 = minigameScoreSpriteGroup;
    interpolatedValue = 1.5f;
    UpdateMinigameScoreDisplay(playerCount, scoreType);
    for (frameIndex = 0; frameIndex < 0xA; frameIndex++) {
        WaitFrames(1);
        spriteGroupId = InterpolateSine(interpolatedValue, 1.0f, minigameValue, 5.0f);
        if (++minigameValue == 5) {
            if (interpolatedValue > 1.0f) {
                interpolatedValue -= 0.1f;
            }
            minigameValue = 0;
        }
        HuSprGrpScaleSet(spriteGroupId2, spriteGroupId, spriteGroupId);
        HuWinScaleSet(scoreSummaryWindow, spriteGroupId, spriteGroupId);
    }
}

void HideMinigameScoreWindow(s32 playerCount)
{
    HideMinigameScoreDisplay();
}

s32 selectedMinigameId = 0xFFFFFFFF;
s32 minigameNameSpriteGroup = -1;
s32 minigameNameWindows[3] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

void CreateHighScoreDisplay(void)
{
    s32 spriteId;
    s32 spriteIndex;
    s32 spriteGroupId;
    ANIMDATA *animationData;

    spriteGroupId = HuSprGrpCreate(0x25);
    selectedMinigameId = spriteGroupId;
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x1B)), 0x1388, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x1C)), 0x1388, 0);
    HuSprGrpMemberSet(spriteGroupId, 0x23, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x1D)), 0x7D0, 0);
    HuSprGrpMemberSet(spriteGroupId, 1, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x1E)), 0x7D0, 0);
    HuSprGrpMemberSet(spriteGroupId, 2, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x1F)), 0x7D0, 0);
    HuSprGrpMemberSet(spriteGroupId, 3, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x20)), 0x76C, 0);
    HuSprGrpMemberSet(spriteGroupId, 4, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x21)), 0x834, 0);
    HuSprGrpMemberSet(spriteGroupId, 6, spriteId);
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        spriteId = HuSprCreate(HuSprAnimReadFile(spriteIndex + DATA_MAKE_NUM(DATADIR_MPEX, 0x2F)), 0xA, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 7, spriteId);
    }
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        spriteId = HuSprCreate(HuSprAnimReadFile(spriteIndex + DATA_MAKE_NUM(DATADIR_MPEX, 0x32)), 0xA, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0xA, spriteId);
    }
    animationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x35));
    for (spriteIndex = 0; spriteIndex < 0x15; spriteIndex++) {
        spriteId = HuSprCreate(animationData, 9, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0xD, spriteId);
    }
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x15)), 8, 0);
    HuSprGrpMemberSet(spriteGroupId, 0x22, spriteId);
    CreateMinigameNameDisplay(-1);
    DestroyMinigameNameDisplay(-1);
}

void DestroyHighScoreDisplay(void)
{
    s32 spriteGroupId = selectedMinigameId;
    HuSprGrpKill(spriteGroupId);
}

s32 GetFormattedHighScore(s32 minigameId, s32 playerIndex, s32 recordType, s32 score)
{
    #if VERSION_NTSC
    s32 thresholdsByPlayerCount[5][9] = {
        { 0x64, 0xC8, 0x12C, 0x136, 0x140, 0x14A, 0x154, 0x15E, 0x168 },
        { 0x05, 0x0A, 0x0F, 0x14, 0x19, 0x1E, 0x23, 0x28, 0x32 },
        { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 },
        { 0xE10, 0x1C20, 0x2A30, 0x3840, 0x4650, 0x5460, 0x6270, 0x7080, 0x7E90 },
        { 0x0A, 0x14, 0x1E, 0x23, 0x28, 0x2D, 0x32, 0x35, 0x37 },
    };
    s32 thresholdsByRecordType[3][9] = {
        { 0x4650, 0x3840, 0x2A30, 0x1C20, 0x0E10, 0, 0, 0, 0 },
        { 0x4650, 0x3840, 0x2A30, 0x1C20, 0x1770, 0x12C0, 0x0E10, 0, 0 },
        { 0x4650, 0x3840, 0x2A30, 0x2328, 0x1C20, 0x19C8, 0x1770, 0x1644, 0x1518 },
    };
    #else
    s32 thresholdsByPlayerCount[5][9] = {
        { 0x32, 0x64, 0xC8, 0x12C, 0x136, 0x140, 0x14A, 0x154, 0x15E },
        { 0x05, 0x0A, 0x0F, 0x14, 0x19, 0x1E, 0x23, 0x28, 0x32 },
        { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 },
        { 60*REFRESH_RATE, 120*REFRESH_RATE, 180*REFRESH_RATE, 240*REFRESH_RATE, 300*REFRESH_RATE,
            360*REFRESH_RATE, 420*REFRESH_RATE, 480*REFRESH_RATE, 540*REFRESH_RATE },
        { 0x0A, 0xF, 0x14, 0x19, 0x1E, 0x23, 0x28, 0x2B, 0x2D },
    };
    s32 thresholdsByRecordType[3][9] = {
        { 300*REFRESH_RATE, 240*REFRESH_RATE, 180*REFRESH_RATE, 120*REFRESH_RATE, 60*REFRESH_RATE, 0, 0, 0, 0 },
        { 300*REFRESH_RATE, 240*REFRESH_RATE, 180*REFRESH_RATE, 120*REFRESH_RATE, 110*REFRESH_RATE, 100*REFRESH_RATE, 90*REFRESH_RATE, 0, 0 },
        { 300*REFRESH_RATE, 240*REFRESH_RATE, 180*REFRESH_RATE, 170*REFRESH_RATE, 124*REFRESH_RATE, 150*REFRESH_RATE, 140*REFRESH_RATE, 130*REFRESH_RATE, 120*REFRESH_RATE },
    };
    #endif
    s32 rankByPlayer[3];
    s32 loopIndex;
    s32 loopIndex2;

    if (playerIndex >= 5) {
        for (loopIndex = 0; loopIndex < 9; loopIndex++) {
            if (minigameId < thresholdsByPlayerCount[playerIndex - 5][loopIndex]) {
                break;
            }
        }
    }
    else if (score != 0) {
        for (loopIndex2 = 0; loopIndex2 < 3; loopIndex2++) {
            OSReport("#################### HI-SCORE(%d-%d) : %d\n", playerIndex, loopIndex2, GWGameStat.mg_record[loopIndex2 + 6]);
            minigameId = GWGameStat.mg_record[loopIndex2 + 6];
            for (loopIndex = 0; loopIndex < 9; loopIndex++) {
                if (minigameId >= thresholdsByRecordType[loopIndex2][loopIndex]) {
                    break;
                }
            }
            rankByPlayer[loopIndex2] = loopIndex;
        }
        if (rankByPlayer[0] <= rankByPlayer[1]) {
            loopIndex = rankByPlayer[1];
        }
        else {
            loopIndex = rankByPlayer[0];
        }
        if (loopIndex <= rankByPlayer[2]) {
            loopIndex = rankByPlayer[2];
        }
    }
    else {
        for (loopIndex = 0; loopIndex < 9; loopIndex++) {
            if (minigameId >= thresholdsByRecordType[recordType][loopIndex]) {
                break;
            }
        }
    }
    return loopIndex;
}

void InsertFormattedHighScore(s32 messageId, u32 highScore)
{
    s32 recordIndexByScoreType[11] = {
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0x00000009,
        0xFFFFFFFF,
        0x00000004,
        0x0000000B,
        0x0000000C,
        0x0000000D,
        0x0000000E,
        0xFFFFFFFF,
    };
    s32 windowId;
    u32 scoreValue;
    u32 scoreValue2;

    switch (highScore) {
        case 0:
        case 1:
        case 2:
        case 10:
            break;
        case 4:
            windowId = GetFormattedHighScore(-1, highScore, 0, 1);
            HuWinInsertMesSet(messageId, ((highScore - 4) * 0xA) + 0x330000 + windowId + 0x51, 3);
            break;
        case 5:
        case 6:
        case 7:
        case 9:
            windowId = GetFormattedHighScore(GWGameStat.mg_record[recordIndexByScoreType[highScore]], highScore, 0, 0);
            HuWinInsertMesSet(messageId, ((highScore - 4) * 0xA) + 0x330000 + windowId + 0x51, 3);
        case 3:
            OSReport("#################### HI-SCORE(%d) : %d\n", highScore, GWGameStat.mg_record[recordIndexByScoreType[highScore]]);
            windowId = GWGameStat.mg_record[recordIndexByScoreType[highScore]];
            #if VERSION_NTSC
            if (windowId < 0xA) {
                sprintf(formattedScoreRank, "  %d", windowId);
            }
            else if (windowId < 0x64) {
                sprintf(formattedScoreRank, " %d", windowId);
            }
            else {
                sprintf(formattedScoreRank, "%d", windowId);
            }
            #else
            sprintf(formattedScoreRank, " %d", windowId);
            #endif
            HuWinInsertMesSet(messageId, (u32)formattedScoreRank, 0);
            break;
        case 8:
            OSReport("#################### HI-SCORE(%d) : %d\n", highScore, GWGameStat.mg_record[recordIndexByScoreType[highScore]]);
            windowId = GWGameStat.mg_record[recordIndexByScoreType[highScore]];
            scoreValue = windowId / (600 *REFRESH_RATE);
            windowId -= scoreValue * (600 *REFRESH_RATE);
            scoreValue2 = windowId / (60 *REFRESH_RATE);
            windowId -= scoreValue2 * (60 *REFRESH_RATE);
            sprintf(formattedScoreValue, " %d", scoreValue2);
            HuWinInsertMesSet(messageId, (u32)formattedScoreValue, 0);
            scoreValue = windowId / (10 *REFRESH_RATE);
            windowId -= scoreValue * (10 *REFRESH_RATE);
            scoreValue2 = windowId / REFRESH_RATE;
            windowId -= scoreValue2 * REFRESH_RATE;
            sprintf(formattedScoreDigits, "%d%d", scoreValue, scoreValue2);
            HuWinInsertMesSet(messageId, (u32)formattedScoreDigits, 1);
            windowId = (101.5/REFRESH_RATE) * (float)windowId;
            scoreValue = windowId / 10;
            windowId -= scoreValue * 0xA;
            scoreValue2 = windowId;
            if (GWGameStat.mg_record[recordIndexByScoreType[highScore]] >= ((600*REFRESH_RATE)-1)) {
                sprintf(formattedScoreRecord, "99");
            }
            else {
                sprintf(formattedScoreRecord, "%d%d", scoreValue, scoreValue2);
            }
            HuWinInsertMesSet(messageId, (u32)formattedScoreRecord, 2);
            windowId = GetFormattedHighScore(GWGameStat.mg_record[recordIndexByScoreType[highScore]], highScore, 0, 0);
            HuWinInsertMesSet(messageId, ((highScore - 4) * 0xA) + 0x330000 + windowId + 0x51, 3);
            break;
        default:
            break;
    }
}

void CreateMinigameNameDisplay(s32 messageId)
{
    s32 spriteGroupId;
    s32 spriteIndex;
    s32 spriteId;

    spriteGroupId = selectedMinigameId;
    for (spriteIndex = 0; spriteIndex < 0x25; spriteIndex++) {
        HuSprAttrSet(spriteGroupId, spriteIndex, 4);
    }
    if (messageId != -1) {
        s32 windowId[11][4] = {
            { 0x00370000, 0x00370001, 0x00370002, 0x0017003E },
            { 0x00370003, 0x00370004, 0x00370005, 0x0017003F },
            { 0x00370006, 0x00370007, 0x00370008, 0x0017003D },
            { 0x00370009, 0x0037000A, 0x0037000B, 0x00170034 },
            { 0x0037000C, 0x0037000D, 0x0037000E, 0x00170033 },
            { 0x0037000F, 0x00370010, 0x00370011, 0x0017002A },
            { 0x00370012, 0x00370013, 0x00370014, 0x00170039 },
            { 0x00370015, 0x00370016, 0x00370017, 0x0017003A },
            { 0x00370018, 0x00370019, 0x0037001A, 0x0017003B },
            { 0x0037001B, 0x0037001C, 0x0037001D, 0x0017003C },
            { 0x0037001E, 0x0037001F, 0x00370020, 0x00170021 },
        };
        spriteId = HuSprCreate(HuSprAnimReadFile(messageId + DATA_MAKE_NUM(DATADIR_MPEX, 0x22)), 0x7D0, 0);
        HuSprGrpMemberSet(spriteGroupId, 5, spriteId);
        minigameNameSpriteGroup = CreateMinigameNameSprites(windowId[messageId][3]);
        HuSprGrpPosSet(minigameNameSpriteGroup, 288.0f, -437.0f);
        for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
            minigameNameWindows[spriteIndex] = HuWinCreate(0.0f, 0.0f, 0x136, 0x78, 0);
            HuWinBGTPLvlSet(minigameNameWindows[spriteIndex], 0.0f);
            HuWinMesSpeedSet(minigameNameWindows[spriteIndex], 0);
            HuWinDispOn(minigameNameWindows[spriteIndex]);
            if (windowId[messageId][spriteIndex] != -1) {
                if (spriteIndex == 2) {
                    InsertFormattedHighScore(minigameNameWindows[2], messageId);
                }
                HuWinMesSet(minigameNameWindows[spriteIndex], windowId[messageId][spriteIndex]);
            }
            HuWinPosSet(minigameNameWindows[spriteIndex], -485.0f, (spriteIndex << 7) + 0x70);
        }
        if (messageId != 0xA) {
            HuSprAttrReset(spriteGroupId, 0, 4);
            HuSprPosSet(spriteGroupId, 0, 288.0f, 240.0f);
            HuSprTPLvlSet(spriteGroupId, 0, 0.0f);
        }
        else {
            HuSprAttrReset(spriteGroupId, 0x23, 4);
            HuSprPosSet(spriteGroupId, 0x23, 288.0f, 240.0f);
            HuSprTPLvlSet(spriteGroupId, 0x23, 0.0f);
        }
        HuSprAttrReset(spriteGroupId, 1, 4);
        HuSprPosSet(spriteGroupId, 1, -331.0f, 272.0f);
        HuSprAttrReset(spriteGroupId, 2, 4);
        HuSprPosSet(spriteGroupId, 2, 452.0f, 911.0f);
        HuSprAttrReset(spriteGroupId, 3, 4);
        HuSprPosSet(spriteGroupId, 3, 288.0f, -437.0f);
        HuSprAttrReset(spriteGroupId, 4, 4);
        HuSprPosSet(spriteGroupId, 4, 952.0f, 212.0f);
        HuSprAttrReset(spriteGroupId, 5, 4);
        HuSprPosSet(spriteGroupId, 5, 952.0f, 212.0f);
        HuSprScaleSet(spriteGroupId, 5, 0.5f, 0.5f);
        HuSprAttrReset(spriteGroupId, 6, 4);
        HuSprPosSet(spriteGroupId, 6, 952.0f, 212.0f);
    }
}

void DestroyMinigameNameDisplay(s32 displayMode)
{
    s32 spriteIndex;
    s32 spriteGroupId;

    spriteGroupId = selectedMinigameId;
    for (spriteIndex = 0; spriteIndex < 0x25; spriteIndex++) {
        HuSprAttrSet(spriteGroupId, spriteIndex, 4);
    }
    if (displayMode != -1) {
        HuSprGrpMemberKill(spriteGroupId, 5);
        HuSprGrpKill(minigameNameSpriteGroup);
        for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
            HuWinKill(minigameNameWindows[spriteIndex]);
        }
    }
}

void PositionMinigameNameDisplay(s32 displayMode)
{
    s32 spriteGroupId;
    s32 windowIndex;
    s32 positionValue;

    positionValue = selectedMinigameId;
    CreateMinigameNameDisplay(displayMode);
    if (displayMode != 0xA) {
        HuSprTPLvlSet(positionValue, 0, 1.0f);
    }
    else {
        HuSprTPLvlSet(positionValue, 0x23, 1.0f);
    }
    HuSprPosSet(positionValue, 3, 288.0f, 63.0f);
    HuSprGrpPosSet(minigameNameSpriteGroup, 288.0f, 63.0f);
    HuSprPosSet(positionValue, 1, 169.0f, 272.0f);
    for (windowIndex = 0; windowIndex < 3; windowIndex++) {
        HuWinPosSet(minigameNameWindows[windowIndex], 14.0f, (windowIndex << 7) + 0x70);
    }
    HuSprPosSet(positionValue, 4, 452.0f, 212.0f);
    HuSprPosSet(positionValue, 5, 452.0f, 212.0f);
    HuSprPosSet(positionValue, 6, 452.0f, 212.0f);
    HuSprPosSet(positionValue, 2, 452.0f, 411.0f);
    spriteGroupId = characterSelectSpriteGroups[2];
    if (displayMode <= 3) {
        UpdateCharacterSelectionDisplay(0);
    }
    else {
        UpdateCharacterSelectionDisplay(1);
    }
    HuSprGrpPosSet(spriteGroupId, 452.0f, 355.0f);
}

void AnimateMinigameNameDisplay(s32 displayMode)
{
    float interpolatedValue;
    float spriteGroupId;
    float spriteGroupId2;
    s32 frameIndex;
    s32 positionValue;
    s32 spriteGroupId3;
    s32 windowIndex;
    s32 spriteGroupId4;

    positionValue = selectedMinigameId;
    CreateMinigameNameDisplay(displayMode);
    for (frameIndex = 0; frameIndex <= 0x1E; frameIndex++) {
        WaitFrames(1);
        interpolatedValue = InterpolateSine(0.0f, 1.0f, frameIndex, 30.0f);
        if (displayMode != 0xA) {
            HuSprTPLvlSet(positionValue, 0, interpolatedValue);
        }
        else {
            HuSprTPLvlSet(positionValue, 0x23, interpolatedValue);
        }
    }
    if (displayMode != 0xA) {
        HuSprTPLvlSet(positionValue, 0, 1.0f);
    }
    else {
        HuSprTPLvlSet(positionValue, 0x23, 1.0f);
    }
    for (frameIndex = 0; frameIndex <= 0x1E; frameIndex++) {
        WaitFrames(1);
        spriteGroupId3 = InterpolateSine(-500.0f, 0.0f, frameIndex, 30.0f);
        HuSprPosSet(positionValue, 3, 288.0f, spriteGroupId3 + 0x3F);
        HuSprGrpPosSet(minigameNameSpriteGroup, 288.0f, spriteGroupId3 + 0x3F);
        HuSprPosSet(positionValue, 1, spriteGroupId3 + 0xA9, 272.0f);
        for (windowIndex = 0; windowIndex < 3; windowIndex++) {
            HuWinPosSet(minigameNameWindows[windowIndex], spriteGroupId3 + 0xE, (windowIndex << 7) + 0x70);
        }
        spriteGroupId3 = InterpolateSine(500.0f, 0.0f, frameIndex, 30.0f);
        HuSprPosSet(positionValue, 4, spriteGroupId3 + 0x1C4, 212.0f);
        HuSprPosSet(positionValue, 5, spriteGroupId3 + 0x1C4, 212.0f);
        HuSprPosSet(positionValue, 6, spriteGroupId3 + 0x1C4, 212.0f);
        HuSprPosSet(positionValue, 2, 452.0f, spriteGroupId3 + 0x19B);
        if (displayMode != 0xA) {
            spriteGroupId4 = characterSelectSpriteGroups[2];
            spriteGroupId = InterpolateSine(112.0f, 452.0f, frameIndex, 30.0f);
            spriteGroupId2 = InterpolateSine(72.0f, 355.0f, frameIndex, 30.0f);
            HuSprGrpPosSet(spriteGroupId4, spriteGroupId, spriteGroupId2);
        }
    }
}

void HideMinigameNameDisplay(s32 displayMode)
{
    float interpolatedValue;
    float spriteGroupId;
    float spriteGroupId2;

    s32 frameIndex;
    s32 positionValue;
    s32 spriteGroupId3;
    s32 spriteIndex;
    s32 windowIndex;
    s32 spriteGroupId4;
    s32 spriteGroupId5;

    positionValue = selectedMinigameId;
    for (frameIndex = 0; frameIndex <= 0x1E; frameIndex++) {
        WaitFrames(1);
        spriteGroupId3 = InterpolateCosine(0.0f, -500.0f, frameIndex, 30.0f);
        HuSprPosSet(positionValue, 3, 288.0f, spriteGroupId3 + 0x3F);
        HuSprGrpPosSet(minigameNameSpriteGroup, 288.0f, spriteGroupId3 + 0x3F);
        HuSprPosSet(positionValue, 1, spriteGroupId3 + 0xA9, 272.0f);
        for (windowIndex = 0; windowIndex < 3; windowIndex++) {
            HuWinPosSet(minigameNameWindows[windowIndex], spriteGroupId3 + 0xE, (windowIndex << 7) + 0x70);
        }
        spriteGroupId3 = InterpolateCosine(0.0f, 500.0f, frameIndex, 30.0f);
        HuSprPosSet(positionValue, 4, spriteGroupId3 + 0x1C4, 212.0f);
        HuSprPosSet(positionValue, 5, spriteGroupId3 + 0x1C4, 212.0f);
        HuSprPosSet(positionValue, 6, spriteGroupId3 + 0x1C4, 212.0f);
        HuSprPosSet(positionValue, 2, 452.0f, spriteGroupId3 + 0x19B);
        if (displayMode != 0xA) {
            spriteGroupId5 = characterSelectSpriteGroups[2];
            spriteGroupId = InterpolateCosine(452.0f, 112.0f, frameIndex, 30.0f);
            spriteGroupId2 = InterpolateCosine(355.0f, 72.0f, frameIndex, 30.0f);
            HuSprGrpPosSet(spriteGroupId5, spriteGroupId, spriteGroupId2);
        }
    }
    for (frameIndex = 0; frameIndex <= 0x1E; frameIndex++) {
        WaitFrames(1);
        interpolatedValue = InterpolateCosine(1.0f, 0.0f, frameIndex, 30.0f);
        if (displayMode != 0xA) {
            HuSprTPLvlSet(positionValue, 0, interpolatedValue);
        }
        else {
            HuSprTPLvlSet(positionValue, 0x23, interpolatedValue);
        }
    }
    if (displayMode != 0xA) {
        HuSprTPLvlSet(positionValue, 0, 0.0f);
    }
    else {
        HuSprTPLvlSet(positionValue, 0x23, 0.0f);
    }
    spriteGroupId4 = selectedMinigameId;
    for (spriteIndex = 0; spriteIndex < 0x25; spriteIndex++) {
        HuSprAttrSet(spriteGroupId4, spriteIndex, 4);
    }
    if (displayMode != -1) {
        HuSprGrpMemberKill(spriteGroupId4, 5);
        HuSprGrpKill(minigameNameSpriteGroup);
        for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
            HuWinKill(minigameNameWindows[spriteIndex]);
        }
    }
}

void ShowPlayerScoreIcon(void)
{
    float positionValue;
    s32 spriteIndex;
    s32 positionValue2;
    s32 frameIndex;

    positionValue2 = selectedMinigameId;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrReset(positionValue2, spriteIndex + 7, 4);
        HuSprPosSet(positionValue2, spriteIndex + 7, 788.0f, (spriteIndex * 0x38) + 0xB8);
    }
    for (frameIndex = 0; frameIndex <= 0x3C; frameIndex++) {
        WaitFrames(1);
        for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
            positionValue = InterpolateSine(1000.0f, 0.0f, frameIndex, 0x3C - ((2 - spriteIndex) * 5));
            HuSprPosSet(positionValue2, spriteIndex + 7, 288.0f + positionValue, (spriteIndex * 0x38) + 0xB8);
        }
    }
    HuSprAttrReset(positionValue2, 0x22, 4);
    HuSprPosSet(positionValue2, 0x22, 185.0f, (practiceMenuStateWork.playerIndex * 0x38) + 0xA0);
}

void HidePlayerScoreIcon(void)
{
    float positionValue;
    s32 spriteIndex;
    s32 positionValue2;
    s32 frameIndex;

    positionValue2 = selectedMinigameId;
    HuSprAttrSet(positionValue2, 0x22, 4);
    HuSprPosSet(positionValue2, 0x22, 125.0f, 190.0f);
    for (frameIndex = 0; frameIndex <= 0x3C; frameIndex++) {
        WaitFrames(1);
        for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
            positionValue = InterpolateCosine(0.0f, -1000.0f, frameIndex, 0x3C - ((2 - spriteIndex) * 5));
            HuSprPosSet(positionValue2, spriteIndex + 7, 288.0f + positionValue, (spriteIndex * 0x38) + 0xB8);
        }
    }
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrSet(positionValue2, spriteIndex + 7, 4);
        HuSprPosSet(positionValue2, spriteIndex + 7, 288.0f, (spriteIndex * 0x38) + 0xB8);
    }
}

void SetPlayerScoreIconBank(void)
{
    s32 minigameValue;
    s32 minigameValue2;
    s32 minigameValue3;
    float positionValue;

    s32 spriteIndex;
    s32 minigameValue4;
    s32 positionValue2;
    s32 spriteIndex2;
    u32 minigameValue5;
    u32 minigameValue6;
    s32 frameIndex;

    positionValue2 = selectedMinigameId;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        minigameValue = rand8() % 10;
        minigameValue2 = rand8() % 60;
        minigameValue3 = rand8() % 100;
        HuSprAttrReset(positionValue2, spriteIndex + 0xA, 4);
        HuSprPosSet(positionValue2, spriteIndex + 0xA, 1288.0f, (spriteIndex * 0x38) + 0xB8);
        for (spriteIndex2 = 0; spriteIndex2 < 7; spriteIndex2++) {
            HuSprAttrReset(positionValue2, 0xD + spriteIndex2 + (spriteIndex * 7), 4);
            HuSprPosSet(positionValue2, 0x0D + spriteIndex2 + (spriteIndex * 7), (spriteIndex2 * 0x14) + 0x512, (spriteIndex * 0x38) + 0xBA);
        }
        minigameValue4 = GWGameStat.mg_record[spriteIndex + 6];
        minigameValue5 = minigameValue4 / (600*REFRESH_RATE);
        minigameValue4 -= minigameValue5 * (600*REFRESH_RATE);
        minigameValue6 = minigameValue4 / (60*REFRESH_RATE);
        minigameValue4 -= minigameValue6 * (60*REFRESH_RATE);
        HuSprBankSet(positionValue2, (spriteIndex * 7) + 0xD, minigameValue6);
        HuSprBankSet(positionValue2, (spriteIndex * 7) + 0xE, 0xA);
        minigameValue5 = minigameValue4 / (10*REFRESH_RATE);
        minigameValue4 -= minigameValue5 * (10*REFRESH_RATE);
        minigameValue6 = minigameValue4 / REFRESH_RATE;
        minigameValue4 -= minigameValue6 * REFRESH_RATE;
        HuSprBankSet(positionValue2, (spriteIndex * 7) + 0xF, minigameValue5);
        HuSprBankSet(positionValue2, (spriteIndex * 7) + 0x10, minigameValue6);
        HuSprBankSet(positionValue2, (spriteIndex * 7) + 0x11, 0xB);
        minigameValue4 = (100.0/REFRESH_RATE) * (float)minigameValue4;
        minigameValue5 = minigameValue4 / 10;
        minigameValue4 -= minigameValue5 * 0xA;
        minigameValue6 = minigameValue4;
        if (GWGameStat.mg_record[spriteIndex + 6] == ((600*REFRESH_RATE)-1)) {
            HuSprBankSet(positionValue2, (spriteIndex * 7) + 0x12, 9);
            HuSprBankSet(positionValue2, (spriteIndex * 7) + 0x13, 9);
        }
        else {
            HuSprBankSet(positionValue2, (spriteIndex * 7) + 0x12, minigameValue5);
            HuSprBankSet(positionValue2, (spriteIndex * 7) + 0x13, minigameValue6);
        }
    }
    for (frameIndex = 0; frameIndex <= 0x3C; frameIndex++) {
        WaitFrames(1);
        for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
            positionValue = InterpolateSine(1000.0f, 0.0f, frameIndex, 0x3C - ((2 - spriteIndex) * 5));
            HuSprPosSet(positionValue2, spriteIndex + 0xA, 288.0f + positionValue, (spriteIndex * 0x38) + 0xB8);
            for (spriteIndex2 = 0; spriteIndex2 < 7; spriteIndex2++) {
                HuSprPosSet(positionValue2, 0xD + spriteIndex2 + (spriteIndex * 7), positionValue + ((spriteIndex2 * 0x14) + 0x12A), (spriteIndex * 0x38) + 0xBA);
            }
        }
    }
    HuSprAttrReset(positionValue2, 0x22, 4);
    HuSprPosSet(positionValue2, 0x22, 110.0f, (practiceMenuStateWork.playerIndex * 0x38) + 0xA0);
}

void HidePlayerScoreIconAlternate(void)
{
    float positionValue;
    s32 frameIndex;
    s32 positionValue2;
    s32 spriteIndex;
    s32 spriteIndex2;

    positionValue2 = selectedMinigameId;
    HuSprAttrSet(positionValue2, 0x22, 4);
    HuSprPosSet(positionValue2, 0x22, 125.0f, 184.0f);
    for (frameIndex = 0; frameIndex <= 0x3C; frameIndex++) {
        WaitFrames(1);
        for (spriteIndex2 = 0; spriteIndex2 < 3; spriteIndex2++) {
            positionValue = InterpolateCosine(0.0f, -1000.0f, frameIndex, 0x3C - ((2 - spriteIndex2) * 5));
            HuSprPosSet(positionValue2, spriteIndex2 + 0xA, 288.0f + positionValue, (spriteIndex2 * 0x38) + 0xB8);
            for (spriteIndex = 0; spriteIndex < 7; spriteIndex++) {
                HuSprPosSet(positionValue2, 0xD + spriteIndex + (spriteIndex2 * 7), positionValue + ((spriteIndex * 0x14) + 0x12A), (spriteIndex2 * 0x38) + 0xBA);
            }
        }
    }
    for (spriteIndex2 = 0; spriteIndex2 < 3; spriteIndex2++) {
        HuSprAttrSet(positionValue2, spriteIndex2 + 0xA, 4);
        for (spriteIndex = 0; spriteIndex < 7; spriteIndex++) {
            HuSprAttrSet(positionValue2, 0xD + spriteIndex + (spriteIndex2 * 7), 4);
        }
    }
}

void CreatePlayerNameDisplay(s32 playerCount, s32 displayMode)
{
    float positionValue;
    float interpolatedValue;
    s32 frameIndex;
    s32 spriteGroupId;
    s32 spriteId;

    spriteGroupId = selectedMinigameId;
    {
        s32 minigameValue[3] = { 4, 11, 12 };
        for (frameIndex = 0; frameIndex <= 0xA; frameIndex++) {
            WaitFrames(1);
            if (frameIndex <= 5) {
                interpolatedValue = InterpolateSine(1.0f, 0.0f, frameIndex, 5.0f);
            }
            else {
                interpolatedValue = InterpolateSine(0.0f, 1.0f, frameIndex, frameIndex - 5);
            }
            if (practiceMenuSelection == 4) {
                HuSprTPLvlSet(spriteGroupId, 5, interpolatedValue);
            }
            positionValue = InterpolateSine((displayMode * 0x38) + 0xA0, (playerCount * 0x38) + 0xA0, frameIndex, 10.0f);
            if (practiceMenuSelection == 4) {
                HuSprPosSet(spriteGroupId, 0x22, 110.0f, positionValue);
            }
            else {
                HuSprPosSet(spriteGroupId, 0x22, 185.0f, positionValue);
            }
            if ((practiceMenuSelection == 4) && (frameIndex == 5)) {
                HuSprGrpMemberKill(spriteGroupId, 5);
                spriteId = HuSprCreate(HuSprAnimReadFile(minigameValue[playerCount] + DATA_MAKE_NUM(DATADIR_MPEX, 0x22)), 0x7D0, 0);
                HuSprGrpMemberSet(spriteGroupId, 5, spriteId);
                HuSprPosSet(spriteGroupId, 5, 452.0f, 212.0f);
                HuSprScaleSet(spriteGroupId, 5, 0.5f, 0.5f);
                HuSprTPLvlSet(spriteGroupId, 5, 0.0f);
            }
        }
    }
}

void CreatePlayerNameSprites(void)
{
    float coordinateTable[2];
    s32 windowIndex;
    s32 spriteGroupId;
    s32 spriteId;

    spriteGroupId = HuSprGrpCreate(3);
    playerNameWindow = spriteGroupId;
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x16)), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    HuSprPosSet(spriteGroupId, 0, 0.0f, 88.0f);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x18)), 0x7D0, 0);
    HuSprGrpMemberSet(spriteGroupId, 1, spriteId);
    HuSprPosSet(spriteGroupId, 1, 0.0f, 25.0f);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x1A)), 0x3E8, 0);
    HuSprGrpMemberSet(spriteGroupId, 2, spriteId);
    HuSprPosSet(spriteGroupId, 2, 0.0f, 0.0f);
    HuSprScaleSet(spriteGroupId, 2, 0.9f, 0.65f);
    HuSprTPLvlSet(spriteGroupId, 2, 0.8f);
    for (windowIndex = 0; windowIndex < 6; windowIndex++) {
        FitMessageWindowToText(coordinateTable, MAKE_MESSID(0x33, 0x2F));
        FitMessageWindowToText(coordinateTable, MAKE_MESSID(0x33, 0x30));
        FitMessageWindowToText(coordinateTable, MAKE_MESSID(0x33, 0x31));
        FitMessageWindowToText(coordinateTable, MAKE_MESSID(0x33, 0x32));
        playerNameWindows[windowIndex] = HuWinCreate(0.0f, 0.0f, coordinateTable[0], coordinateTable[1], 0);
        HuWinBGTPLvlSet(playerNameWindows[windowIndex], 0.0f);
        HuWinMesSpeedSet(playerNameWindows[windowIndex], 0);
        HuWinDispOff(playerNameWindows[windowIndex]);
        HuWinPosSet(playerNameWindows[windowIndex], 304.0f, 40.0f + (((60.0f - (coordinateTable[1] / 2)) + (windowIndex * 0x1F)) - 500.0f));
    }
    ShowPlayerNameDisplay();
    HidePlayerNameDisplay();
}

void DestroyPlayerNameDisplay(void)
{
    s32 windowIndex;
    s32 spriteGroupId = playerNameWindow;
    for (windowIndex = 0; windowIndex < 6; windowIndex++) {
        HuWinKill(playerNameWindows[windowIndex]);
    }
    HuSprGrpKill(spriteGroupId);
}

void ShowPlayerNameDisplay(void)
{
    float messageDimensions[2];
    s32 spriteIndex;

    s32 spriteGroupId = playerNameWindow;
    practiceMenuStateWork.stateTimer = 0;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrSet(spriteGroupId, spriteIndex, 4);
    }
    HuSprAttrReset(spriteGroupId, 0, 4);
    HuSprPosSet(spriteGroupId, 0, 0.0f, 92.0f);
    HuSprAttrReset(spriteGroupId, 1, 4);
    HuSprPosSet(spriteGroupId, 1, 0.0f, 25.0f);
    practiceMenuStateWork.stateTimer = 0;
    HuSprPosSet(spriteGroupId, 2, -3.0f, (practiceMenuStateWork.stateTimer * 0x19) + 0x3C);
    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        HuWinDispOn(playerNameWindows[spriteIndex]);
        if (availableMinigameFlags[spriteIndex] == 1) {
            HuWinMesSet(playerNameWindows[spriteIndex], spriteIndex + MAKE_MESSID(0x33, 0x2F));
        }
        else {
            HuWinMesSet(playerNameWindows[spriteIndex], MAKE_MESSID(0x33, 0x2E));
        }
        HuWinMesMaxSizeGet(1, messageDimensions, MAKE_MESSID(0x33, 0x30));
        HuWinPosSet(playerNameWindows[spriteIndex], 288.0f - (messageDimensions[0] / 2), 40.0f + (((60.0f - (messageDimensions[1] / 2)) + (spriteIndex * 0x1F)) - 500.0f));
    }
    HuSprGrpPosSet(spriteGroupId, 288.0f, -460.0f);
}

void HidePlayerNameDisplay(void)
{
    float messageDimensions[2];
    s32 spriteIndex;

    s32 spriteGroupId = playerNameWindow;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrSet(spriteGroupId, spriteIndex, 4);
    }
    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        HuWinDispOff(playerNameWindows[spriteIndex]);
        HuWinMesMaxSizeGet(1, messageDimensions, MAKE_MESSID(0x33, 0x30));
        HuWinPosSet(playerNameWindows[spriteIndex], 288.0f - (messageDimensions[0] / 2), 40.0f + (((60.0f - (messageDimensions[1] / 2)) + (spriteIndex * 0x1F)) - 500.0f));
    }
    HuSprGrpPosSet(spriteGroupId, 288.0f, -460.0f);
}

void ShowPlayerNameDisplayAlternate(void)
{
    float messageDimensions[2];
    float messageDimensions2[2];
    float verticalOffset;

    s32 spriteIndex;
    s32 playerNameGroup2;
    s32 frameIndex;
    s32 windowIndex;

    s32 playerNameGroup = playerNameWindow;
    messageDimensions[0] = messageDimensions[1] = 0.0f;
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x2E));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x2F));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x30));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x31));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x32));
    playerNameGroup2 = playerNameWindow;
    practiceMenuStateWork.stateTimer = 0;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrSet(playerNameGroup2, spriteIndex, 4);
    }
    HuSprAttrReset(playerNameGroup2, 0, 4);
    HuSprPosSet(playerNameGroup2, 0, 0.0f, 92.0f);
    HuSprAttrReset(playerNameGroup2, 1, 4);
    HuSprPosSet(playerNameGroup2, 1, 0.0f, 25.0f);
    practiceMenuStateWork.stateTimer = 0;
    HuSprPosSet(playerNameGroup2, 2, -3.0f, (practiceMenuStateWork.stateTimer * 0x19) + 0x3C);
    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        HuWinDispOn(playerNameWindows[spriteIndex]);
        if (availableMinigameFlags[spriteIndex] == 1) {
            HuWinMesSet(playerNameWindows[spriteIndex], spriteIndex + MAKE_MESSID(0x33, 0x2F));
        }
        else {
            HuWinMesSet(playerNameWindows[spriteIndex], MAKE_MESSID(0x33, 0x2E));
        }
        HuWinMesMaxSizeGet(1, messageDimensions2, 0x330030);
        HuWinPosSet(playerNameWindows[spriteIndex], 288.0f - (messageDimensions2[0] / 2), 40.0f + (((60.0f - (messageDimensions2[1] / 2)) + (spriteIndex * 0x1F)) - 500.0f));
    }
    HuSprGrpPosSet(playerNameGroup2, 288.0f, -460.0f);
    for (frameIndex = 0; frameIndex < 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0x32) {
            verticalOffset = InterpolateSine(-500.0f, 10.0f, frameIndex, 50.0f);
        }
        else {
            verticalOffset = InterpolateCosine(10.0f, 0.0f, frameIndex - 0x32, 10.0f);
        }
        HuSprGrpPosSet(playerNameGroup, 288.0f, 64.0f + (40.0f + verticalOffset));
        for (windowIndex = 0; windowIndex < 4; windowIndex++) {
            HuWinPosSet(playerNameWindows[windowIndex], 288.0f - (messageDimensions[0] / 2), 64.0f + (40.0f + (verticalOffset + ((60.0f - (messageDimensions[1] / 2)) + (windowIndex * 0x1F)))));
        }
    }
}

void AnimatePlayerNameDisplayOut(void)
{
    float messageDimensions[2];
    float messageDimensions2[2];
    float verticalOffset;

    s32 spriteIndex;
    s32 frameIndex;
    s32 windowIndex;
    s32 playerNameGroup2;

    s32 playerNameGroup = playerNameWindow;
    messageDimensions[0] = messageDimensions[1] = 0.0f;

    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x2E));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x2F));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x30));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x31));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x32));
    for (frameIndex = 0; frameIndex < 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0xA) {
            verticalOffset = InterpolateSine(0.0f, 10.0f, frameIndex, 10.0f);
        }
        else {
            verticalOffset = InterpolateCosine(10.0f, -500.0f, frameIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(playerNameGroup, 288.0f, 64.0f + (40.0f + verticalOffset));
        for (windowIndex = 0; windowIndex < 4; windowIndex++) {
            HuWinPosSet(playerNameWindows[windowIndex], 288.0f - (messageDimensions[0] / 2), 64.0f + (40.0f + (verticalOffset + ((60.0f - (messageDimensions[1] / 2)) + (windowIndex * 0x1F)))));
        }
    }
    playerNameGroup2 = playerNameWindow;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrSet(playerNameGroup2, spriteIndex, 4);
    }
    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        HuWinDispOff(playerNameWindows[spriteIndex]);
        HuWinMesMaxSizeGet(1, messageDimensions2, MAKE_MESSID(0x33, 0x30));
        HuWinPosSet(playerNameWindows[spriteIndex], 288.0f - (messageDimensions2[0] / 2), 40.0f + (((60.0f - (messageDimensions2[1] / 2)) + (spriteIndex * 0x1F)) - 500.0f));
    }
    HuSprGrpPosSet(playerNameGroup2, 288.0f, -460.0f);
}

void ShowPlayerNameText(void)
{
    float messageDimensions[2];
    s32 positionValue = playerNameWindow;
    HuWinMesMaxSizeGet(1, messageDimensions, 0x330030);
    HuSprAttrReset(positionValue, 2, 4);
    HuSprPosSet(positionValue, 2, 0.0f, 40.0f + ((60.0f - messageDimensions[1]) + (practiceMenuStateWork.stateTimer * 0x1F)));
}

void HidePlayerNameText(void)
{
    s32 playerStateValue = playerNameWindow;
    HuSprAttrSet(playerStateValue, 2, 4);
}

void AnimatePlayerNameText(s32 messageId)
{
    float messageDimensions[2];
    float interpolatedValue;
    float interpolatedValue2;
    float verticalPosition;
    s32 playerStateValue;
    s32 frameIndex;
    s32 playerStateValue2;

    s32 playerNameGroup = playerNameWindow;
    HuWinMesMaxSizeGet(1, messageDimensions, MAKE_MESSID(0x33, 0x30));
    playerStateValue2 = practiceMenuStateWork.stateTimer;
    playerStateValue = practiceMenuStateWork.stateTimer + messageId;
    if (playerStateValue > 3) {
        playerStateValue = 0;
    }
    else if (playerStateValue < 0) {
        playerStateValue = 3;
    }
    for (frameIndex = 0; frameIndex <= 5; frameIndex++) {
        WaitFrames(1);
        interpolatedValue = 40.0f + ((60.0f - messageDimensions[1]) + (playerStateValue2 * 0x1F));
        interpolatedValue2 = 40.0f + ((60.0f - messageDimensions[1]) + (playerStateValue * 0x1F));
        verticalPosition = InterpolateSine(interpolatedValue, interpolatedValue2, frameIndex, 5.0f);
        HuSprPosSet(playerNameGroup, 2, -4.0f, verticalPosition);
    }
    practiceMenuStateWork.stateTimer = playerStateValue;
}

void CreatePlayerScoreSprites(void)
{
    float scoreValues[2];
    s32 windowIndex;
    s32 spriteId;

    s32 spriteGroupId = HuSprGrpCreate(3);
    playerScoreSpriteGroup = spriteGroupId;
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x17)), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    HuSprPosSet(spriteGroupId, 0, 0.0f, 118.0f);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x19)), 0x7D0, 0);
    HuSprGrpMemberSet(spriteGroupId, 1, spriteId);
    HuSprPosSet(spriteGroupId, 1, 0.0f, 25.0f);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x1A)), 0x3E8, 0);
    HuSprGrpMemberSet(spriteGroupId, 2, spriteId);
    HuSprPosSet(spriteGroupId, 2, 0.0f, 0.0f);
    HuSprPosSet(spriteGroupId, 2, 0.0f, 0.0f);
    HuSprScaleSet(spriteGroupId, 2, 0.9f, 0.65f);
    HuSprTPLvlSet(spriteGroupId, 2, 0.8f);
    for (windowIndex = 0; windowIndex < 6; windowIndex++) {
        FitMessageWindowToText(scoreValues, MAKE_MESSID(0x33, 0x33));
        FitMessageWindowToText(scoreValues, MAKE_MESSID(0x33, 0x34));
        FitMessageWindowToText(scoreValues, MAKE_MESSID(0x33, 0x35));
        FitMessageWindowToText(scoreValues, MAKE_MESSID(0x33, 0x36));
        FitMessageWindowToText(scoreValues, MAKE_MESSID(0x33, 0x37));
        FitMessageWindowToText(scoreValues, MAKE_MESSID(0x33, 0x38));
        playerNameWindows[windowIndex] = HuWinCreate(0.0f, 0.0f, scoreValues[0], scoreValues[1], 0);
        HuWinBGTPLvlSet(playerNameWindows[windowIndex], 0.0f);
        HuWinMesSpeedSet(playerNameWindows[windowIndex], 0);
        HuWinDispOff(playerNameWindows[windowIndex]);
        HuWinPosSet(playerNameWindows[windowIndex], 304.0f, 40.0f + (((60.0f - (scoreValues[1] / 2)) + (windowIndex * 0x1D)) - 500.0f));
    }
    ShowPlayerScoreDisplay();
    HidePlayerScoreDisplay();
    (void)windowIndex;
}

void DestroyPlayerScoreDisplay(void)
{
    s32 spriteGroupId;
    s32 windowIndex;

    spriteGroupId = playerScoreSpriteGroup;
    for (windowIndex = 0; windowIndex < 6; windowIndex++) {
        HuWinKill(playerNameWindows[windowIndex]);
    }
    HuSprGrpKill(spriteGroupId);
}

void ShowPlayerScoreDisplay(void)
{
    float messageDimensions[2];
    s32 spriteIndex;

    s32 spriteGroupId = playerScoreSpriteGroup;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrSet(spriteGroupId, spriteIndex, 4);
    }
    HuSprAttrReset(spriteGroupId, 0, 4);
    HuSprAttrReset(spriteGroupId, 1, 4);
    practiceMenuStateWork.stateTimer = 0;
    HuSprPosSet(spriteGroupId, 2, -3.0f, (practiceMenuStateWork.stateTimer * 0x19) + 0x3C);
    for (spriteIndex = 0; spriteIndex < 6; spriteIndex++) {
        HuWinDispOn(playerNameWindows[spriteIndex]);
        if (availableMinigameFlags[spriteIndex + 4] == 1) {
            HuWinMesSet(playerNameWindows[spriteIndex], spriteIndex + MAKE_MESSID(0x33, 0x33));
        }
        else {
            HuWinMesSet(playerNameWindows[spriteIndex], MAKE_MESSID(0x33, 0x2E));
        }
        HuWinMesMaxSizeGet(1, messageDimensions, MAKE_MESSID(0x33, 0x33));
        HuWinPosSet(playerNameWindows[spriteIndex], 288.0f - (messageDimensions[0] / 2), 40.0f + (((60.0f - (messageDimensions[1] / 2)) + (spriteIndex * 0x1D)) - 500.0f));
    }
    HuSprGrpPosSet(spriteGroupId, 288.0f, -460.0f);
}

void HidePlayerScoreDisplay(void)
{
    float messageDimensions[2];
    s32 spriteIndex;

    s32 spriteGroupId = playerScoreSpriteGroup;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrSet(spriteGroupId, spriteIndex, 4);
    }
    for (spriteIndex = 0; spriteIndex < 6; spriteIndex++) {
        HuWinDispOff(playerNameWindows[spriteIndex]);
        HuWinMesMaxSizeGet(1, messageDimensions, MAKE_MESSID(0x33, 0x33));
        HuWinPosSet(playerNameWindows[spriteIndex], 288.0f - (messageDimensions[0] / 2), 40.0f + (((60.0f - (messageDimensions[1] / 2)) + (spriteIndex * 0x1D)) - 500.0f));
    }
    HuSprGrpPosSet(spriteGroupId, 288.0f, -460.0f);
}

void ShowPlayerScoreDisplayAlternate(void)
{
    float messageDimensions[2];
    float messageDimensions2[2];
    float spriteGroupId;

    s32 spriteIndex;
    s32 frameIndex;
    s32 windowIndex;
    s32 spriteGroupId2;
    s32 spriteGroupId3;

    spriteGroupId3 = playerScoreSpriteGroup;
    messageDimensions[0] = messageDimensions[1] = 0.0f;
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x2E));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x33));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x34));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x35));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x36));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x37));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x38));
    spriteGroupId2 = playerScoreSpriteGroup;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrSet(spriteGroupId2, spriteIndex, 4);
    }
    HuSprAttrReset(spriteGroupId2, 0, 4);
    HuSprAttrReset(spriteGroupId2, 1, 4);
    practiceMenuStateWork.stateTimer = 0;
    HuSprPosSet(spriteGroupId2, 2, -3.0f, (practiceMenuStateWork.stateTimer * 0x19) + 0x3C);
    for (spriteIndex = 0; spriteIndex < 6; spriteIndex++) {
        HuWinDispOn(playerNameWindows[spriteIndex]);
        if (availableMinigameFlags[spriteIndex + 4] == 1) {
            HuWinMesSet(playerNameWindows[spriteIndex], spriteIndex + MAKE_MESSID(0x33, 0x33));
        }
        else {
            HuWinMesSet(playerNameWindows[spriteIndex], MAKE_MESSID(0x33, 0x2E));
        }
        HuWinMesMaxSizeGet(1, messageDimensions2, MAKE_MESSID(0x33, 0x33));
        HuWinPosSet(playerNameWindows[spriteIndex], 288.0f - (messageDimensions2[0] / 2), 40.0f + (((60.0f - (messageDimensions2[1] / 2)) + (spriteIndex * 0x1D)) - 500.0f));
    }
    HuSprGrpPosSet(spriteGroupId2, 288.0f, -460.0f);
    for (frameIndex = 0; frameIndex < 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0x32) {
            spriteGroupId = InterpolateSine(-500.0f, 10.0f, frameIndex, 50.0f);
        }
        else {
            spriteGroupId = InterpolateCosine(10.0f, 0.0f, frameIndex - 0x32, 10.0f);
        }
        HuSprGrpPosSet(spriteGroupId3, 288.0f, 64.0f + (40.0f + spriteGroupId));
        for (windowIndex = 0; windowIndex < 6; windowIndex++) {
            HuWinPosSet(playerNameWindows[windowIndex], 288.0f - (messageDimensions[0] / 2), 64.0f + (40.0f + (spriteGroupId + ((60.0f - (messageDimensions[1] / 2)) + (windowIndex * 0x1D)))));
        }
    }
}

void AnimatePlayerScoreDisplayOut(void)
{
    float messageDimensions[2];
    float messageDimensions2[2];
    float spriteGroupId;

    s32 spriteIndex;
    s32 frameIndex;
    s32 windowIndex;
    s32 spriteGroupId2;
    s32 spriteGroupId3;

    spriteGroupId3 = playerScoreSpriteGroup;
    messageDimensions[0] = messageDimensions[1] = 0.0f;
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x2E));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x33));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x34));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x35));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x36));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x37));
    FitMessageWindowToText(messageDimensions, MAKE_MESSID(0x33, 0x38));
    for (frameIndex = 0; frameIndex < 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0xA) {
            spriteGroupId = InterpolateSine(0.0f, 10.0f, frameIndex, 10.0f);
        }
        else {
            spriteGroupId = InterpolateCosine(10.0f, -500.0f, frameIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(spriteGroupId3, 288.0f, 64.0f + (40.0f + spriteGroupId));
        for (windowIndex = 0; windowIndex < 6; windowIndex++) {
            HuWinPosSet(playerNameWindows[windowIndex], 288.0f - (messageDimensions[0] / 2), 64.0f + (40.0f + (spriteGroupId + ((60.0f - (messageDimensions[1] / 2)) + (windowIndex * 0x1D)))));
        }
    }
    spriteGroupId2 = playerScoreSpriteGroup;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrSet(spriteGroupId2, spriteIndex, 4);
    }
    for (spriteIndex = 0; spriteIndex < 6; spriteIndex++) {
        HuWinDispOff(playerNameWindows[spriteIndex]);
        HuWinMesMaxSizeGet(1, messageDimensions2, MAKE_MESSID(0x33, 0x33));
        HuWinPosSet(playerNameWindows[spriteIndex], 288.0f - (messageDimensions2[0] / 2), 40.0f + (((60.0f - (messageDimensions2[1] / 2)) + (spriteIndex * 0x1D)) - 500.0f));
    }
    HuSprGrpPosSet(spriteGroupId2, 288.0f, -460.0f);
}

void ShowPlayerScoreText(void)
{
    float messageDimensions[2];
    s32 positionValue = playerScoreSpriteGroup;
    HuWinMesMaxSizeGet(1, messageDimensions, 0x330033);
    HuSprAttrReset(positionValue, 2, 4);
    HuSprPosSet(positionValue, 2, 0.0f, 40.0f + ((60.0f - messageDimensions[1]) + (practiceMenuStateWork.stateTimer * 0x1D)));
}

void HidePlayerScoreText(void)
{
    s32 scoreValue = playerScoreSpriteGroup;
    HuSprAttrSet(scoreValue, 2, 4);
}

void AnimatePlayerScoreText(s32 messageId)
{
    float messageDimensions[2];
    float interpolatedValue;
    float interpolatedValue2;
    float positionValue;
    s32 scoreValue;
    s32 frameIndex;
    s32 scoreValue2;
    s32 positionValue2;

    positionValue2 = playerScoreSpriteGroup;
    HuWinMesMaxSizeGet(1, messageDimensions, MAKE_MESSID(0x33, 0x33));
    scoreValue2 = practiceMenuStateWork.stateTimer;
    scoreValue = practiceMenuStateWork.stateTimer + messageId;
    if (scoreValue > 5) {
        scoreValue = 0;
    }
    else if (scoreValue < 0) {
        scoreValue = 5;
    }
    for (frameIndex = 0; frameIndex <= 5; frameIndex++) {
        WaitFrames(1);
        interpolatedValue = 40.0f + ((60.0f - messageDimensions[1]) + (scoreValue2 * 0x1D));
        interpolatedValue2 = 40.0f + ((60.0f - messageDimensions[1]) + (scoreValue * 0x1D));
        positionValue = InterpolateSine(interpolatedValue, interpolatedValue2, frameIndex, 5.0f);
        HuSprPosSet(positionValue2, 2, -4.0f, positionValue);
    }
    practiceMenuStateWork.stateTimer = scoreValue;
}

void CreatePlayerStatusDisplay(void)
{
    s32 spriteId;
    s32 spriteGroupId;
    s32 spriteGroupId2;

    spriteGroupId = HuSprGrpCreate(3);
    playerStatusSpriteGroup = spriteGroupId;
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x12)), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x13)), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId, 1, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x14)), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId, 2, spriteId);
    spriteGroupId2 = HuSprGrpCreate(1);
    playerStatusWindow = spriteGroupId2;
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MPEX, 0x15)), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId2, 0, spriteId);
    HuSprAttrSet(spriteGroupId2, 0, 4);
    ShowPlayerStatusIcon();
    HidePlayerStatusIcon();
}

void DestroyPlayerStatusDisplay(void)
{
    s32 spriteGroupId = playerStatusSpriteGroup;
    s32 spriteGroupId2 = playerStatusWindow;
    HuSprGrpKill(spriteGroupId);
    HuSprGrpKill(spriteGroupId2);
}

void ShowPlayerStatusIcon(void)
{
    s32 spriteIndex;
    s32 playerStateValue = playerStatusSpriteGroup;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrSet(playerStateValue, spriteIndex, 4);
    }
}

void HidePlayerStatusIcon(void)
{
    s32 spriteIndex;
    s32 playerStateValue = playerStatusSpriteGroup;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrSet(playerStateValue, spriteIndex, 4);
    }
}

void AnimatePlayerStatusDisplay(s32 playerIndex)
{
    float scaleX;
    float scaleY;

    s32 frameIndex;
    s32 spriteIndex;
    s32 spriteGroupId3;
    s32 spriteGroupId4;
    s32 playerStateValue;

    spriteGroupId3 = playerStatusSpriteGroup;
    spriteGroupId4 = playerStatusWindow;
    {
        s32 playerStatusPositions[3][2] = {
            { 0x72, 0xFA },
            { 0x144, 0xE1 },
            { 0xEF, 0x5A },
        };
        playerStateValue = playerStatusSpriteGroup;
        for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
            HuSprAttrSet(playerStateValue, spriteIndex, 4);
        }
        if (practiceMenuStateWork.state != 2) {
            HuSprAttrReset(spriteGroupId3, practiceMenuStateWork.state, 4);
        }
        HuSprGrpScaleSet(spriteGroupId3, 0.0f, 0.0f);
        for (frameIndex = 0; frameIndex <= 0xF; frameIndex++) {
            WaitFrames(1);
            if (frameIndex <= 0xA) {
                scaleX = InterpolateSine(0.0f, 1.0f, frameIndex, 10.0f);
                scaleY = 0.01f;
            }
            else {
                scaleX = 1.0f;
                scaleY = InterpolateSine(0.01f, 1.0f, frameIndex - 0xA, 5.0f);
            }
            HuSprGrpScaleSet(spriteGroupId3, scaleX, scaleY);
            if (playerIndex != -1) {
                scaleX = InterpolateSine(playerStatusPositions[playerIndex][0], playerStatusPositions[practiceMenuStateWork.state][0], frameIndex, 15.0f);
                scaleY = InterpolateSine(playerStatusPositions[playerIndex][1], playerStatusPositions[practiceMenuStateWork.state][1], frameIndex, 15.0f);
                HuSprGrpPosSet(spriteGroupId4, scaleX, scaleY);
            }
        }
        HuSprAttrReset(spriteGroupId4, 0, 4);
        if (playerIndex == -1) {
            HuSprGrpPosSet(spriteGroupId4, playerStatusPositions[0][0], playerStatusPositions[0][1]);
        }
    }
}

void ScalePlayerStatusDisplay(void)
{
    float spriteGroupId;
    float spriteGroupId2;
    s32 frameIndex;
    s32 spriteIndex;
    s32 spriteGroupId3;
    s32 playerStateValue;
    s32 playerStateValue2;

    spriteGroupId3 = playerStatusSpriteGroup;
    playerStateValue = playerStatusWindow;
    HuSprAttrSet(playerStateValue, 0, 4);
    for (frameIndex = 0; frameIndex <= 0xF; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0xA) {
            spriteGroupId2 = InterpolateSine(1.0f, 0.01f, frameIndex, 10.0f);
            spriteGroupId = 1.0f;
        }
        else {
            spriteGroupId2 = 0.01f;
            spriteGroupId = InterpolateSine(1.0f, 0.01f, frameIndex - 0xA, 5.0f);
        }
        HuSprGrpScaleSet(spriteGroupId3, spriteGroupId, spriteGroupId2);
    }
    playerStateValue2 = playerStatusSpriteGroup;
    for (spriteIndex = 0; spriteIndex < 3; spriteIndex++) {
        HuSprAttrSet(playerStateValue2, spriteIndex, 4);
    }
}

void PositionPlayerStatusSprite(OMOBJ *object, MpexMenuObjectWork *statusWork)
{
    Vec position;
    Vec position2;

    s32 spriteGroupId = playerStatusSpriteGroup;
    s32 characterSelectionValue[3] = { 0x64, 0x64, 0xE1 };
    position.x = Hu3DData[menuModelObjects[practiceMenuStateWork.state].ownerObject->mdlId[1]].pos.x;
    position.y = 0.0f;
    position.z = Hu3DData[menuModelObjects[practiceMenuStateWork.state].ownerObject->mdlId[1]].pos.z;
    Hu3D3Dto2D(&position, 1, &position2);
    position2.y = CalculateSineWaveValue(characterSelectionValue[practiceMenuStateWork.state], 10.0f + characterSelectionValue[practiceMenuStateWork.state], practiceMenuStateWork.characterIndex++, 360.0f);
    if (practiceMenuStateWork.characterIndex >= 0x168) {
        practiceMenuStateWork.characterIndex = 0;
    }
    HuSprGrpPosSet(spriteGroupId, position2.x, (s32)position2.y);
}

void ReportMinigameHighScore(void)
{
    OSReport("\n\n####################### HI SCORE #######################\n");
    OSReport("HIScore 453(0-99):%d\n", GWGameStat.mg_record[9]);
    if (GWGameStat.mg_record[9] == 0) {
        GWGameStat.mg_record[9] = 0;
    }
    else if (GWGameStat.mg_record[9] >= 0x63) {
        GWGameStat.mg_record[9] = 0x63;
    }
    #if VERSION_NTSC
    OSReport("HIScore 451A(0-35999):%d\n", GWGameStat.mg_record[6]);
    #else
    OSReport("HIScore 451A(0-29999):%d\n", GWGameStat.mg_record[6]);
    #endif
    if (GWGameStat.mg_record[6] == 0) {
        GWGameStat.mg_record[6] = 0;
    }
    else if (GWGameStat.mg_record[6] >= ((REFRESH_RATE*600)-1)) {
        GWGameStat.mg_record[6] = ((REFRESH_RATE*600)-1);
    }
    #if VERSION_NTSC
    OSReport("HIScore 451B(0-35999):%d\n", GWGameStat.mg_record[7]);
    #else
    OSReport("HIScore 451B(0-29999):%d\n", GWGameStat.mg_record[7]);
    #endif
    if (GWGameStat.mg_record[7] == 0) {
        GWGameStat.mg_record[7] = 0;
    }
    else if (GWGameStat.mg_record[7] >= ((REFRESH_RATE*600)-1)) {
        GWGameStat.mg_record[7] = ((REFRESH_RATE*600)-1);
    }
    #if VERSION_NTSC
    OSReport("HIScore 451C(0-35999):%d\n", GWGameStat.mg_record[8]);
    #else
    OSReport("HIScore 451C(0-29999):%d\n", GWGameStat.mg_record[8]);
    #endif
    if (GWGameStat.mg_record[8] == 0) {
        GWGameStat.mg_record[8] = 0;
    }
    else if (GWGameStat.mg_record[8] >= ((REFRESH_RATE*600)-1)) {
        GWGameStat.mg_record[8] = ((REFRESH_RATE*600)-1);
    }
    OSReport("HIScore 442(0-999):%d\n", GWGameStat.mg_record[4]);
    if (GWGameStat.mg_record[4] == 0) {
        GWGameStat.mg_record[4] = 0;
    }
    else if (GWGameStat.mg_record[4] >= 0x3E7) {
        GWGameStat.mg_record[4] = 0x3E7;
    }
    OSReport("HIScore 459(0-99):%d\n", GWGameStat.mg_record[0xB]);
    if (GWGameStat.mg_record[0xB] == 0) {
        GWGameStat.mg_record[0xB] = 0;
    }
    else if (GWGameStat.mg_record[0xB] >= 0x63) {
        GWGameStat.mg_record[0xB] = 0x63;
    }
    OSReport("HIScore 460(0-99):%d\n", GWGameStat.mg_record[0xC]);
    if (GWGameStat.mg_record[0xC] == 0) {
        GWGameStat.mg_record[0xC] = 0;
    }
    else if (GWGameStat.mg_record[0xC] >= 0x63) {
        GWGameStat.mg_record[0xC] = 0x63;
    }
    #if VERSION_NTSC
    OSReport("HIScore 461(0-35999):%d\n", GWGameStat.mg_record[13]);
    #else
    OSReport("HIScore 461(0-29999):%d\n", GWGameStat.mg_record[13]);
    #endif
    if (GWGameStat.mg_record[0xD] == 0) {
        GWGameStat.mg_record[0xD] = 0;
    }
    else if (GWGameStat.mg_record[0xD] >= ((REFRESH_RATE*600)-1)) {
        GWGameStat.mg_record[0xD] = ((REFRESH_RATE*600)-1);
    }
    OSReport("HIScore 462(0-99):%d\n", GWGameStat.mg_record[0xE]);
    if (GWGameStat.mg_record[0xE] == 0) {
        GWGameStat.mg_record[0xE] = 0;
    }
    else if (GWGameStat.mg_record[0xE] >= 0x63) {
        GWGameStat.mg_record[0xE] = 0x63;
    }
    OSReport("########################################################\n\n");
}

void FitMessageWindowToText(float *windowSize, s32 messageId)
{
    float messageDimensions[2];

    HuWinMesMaxSizeGet(1, messageDimensions, messageId);
    if (messageDimensions[0] > windowSize[0]) {
        windowSize[0] = messageDimensions[0];
    }
    if (messageDimensions[1] > windowSize[1]) {
        windowSize[1] = messageDimensions[1];
    }
}

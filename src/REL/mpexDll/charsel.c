#include "ext_math.h"
#include "game/armem.h"
#include "game/audio.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#include "REL/mpexDll.h"

typedef void (*CharacterSelectUpdateCallback)(OMOBJ *, ...);

typedef struct CharacterSelectPlayerWork {
    /* 0x00 */ OMOBJ *ownerObject;
    /* 0x04 */ CharacterSelectUpdateCallback updateCallback;
    /* 0x08 */ s32 isActive;
    /* 0x0C */ s32 playerIndex;
    /* 0x10 */ s32 playerState;
    /* 0x14 */ s32 characterId;
    /* 0x18 */ char reserved18[0x10]; /* maybe part of characterId[5]? */
    /* 0x28 */ s32 characterSelection;
    /* 0x2C */ s32 characterState;
    /* 0x30 */ s32 selectionTimer;
    /* 0x34 */ s32 selectionFlags;
    /* 0x38 */ char reserved38[0x10]; /* maybe part of selectionFlags[5]? */
    /* 0x48 */ s32 spriteBaseId;
    /* 0x4C */ s32 selectionMode;
    /* 0x50 */ s32 selectedCharacter;
    /* 0x54 */ s32 selectedDifficulty;
    /* 0x58 */ s32 controllerPort;
    /* 0x5C */ s32 characterCursor;
} CharacterSelectPlayerWork; /* size = 0x60 */

void CreateCharacterSelectionSprites(void);
void PositionCharacterSelectionSprites(void);
void ScaleCharacterSelectionSprites(void);
void AnimateCharacterSelectionSpritesIn(void);
void AnimateCharacterSelectionSpritesOut(void);
void ScaleCharacterSelectionSpritesIn(void);
void HideCharacterSelectionCursor(void);
void UpdateCharacterSelectionInput(void);
void UpdateCharacterSelectionInputAlternate(void);
void CreateDifficultySelectionSprites(void);
void PositionDifficultySelectionSprites(s32 playerCount);
void HideDifficultySelectionSprites(s32);
void UpdateDifficultySelectionInput(s32 playerCount, s32 selectionMode);
void UpdateDifficultySelectionInputAlternate(s32 playerCount);
void AnimateDifficultySelectionSprites(s32);
void UpdateSelectedDifficultyDisplay(void);
void UpdateCharacterSelectionPlayer(OMOBJ *object, CharacterSelectPlayerWork *playerWork);
void AdvanceOverlappingCharacterCursor(CharacterSelectPlayerWork *playerWork);
void HidePlayerDifficultyCursor(CharacterSelectPlayerWork *playerWork);
void UpdateDifficultySelectionPlayer(OMOBJ *object, CharacterSelectPlayerWork *playerWork);
s32 ProcessDifficultySelectionInput(CharacterSelectPlayerWork *playerWork);
void CreateCharacterSelectionPanel(void);
void UpdateCharacterSelectionDisplay(s32 displayMode);
void HideCharacterSelectionPanel(s32);
void AnimateCharacterSelectionPanelOut(s32 displayMode);
void AnimateCharacterSelectionPanelOutAlternate(s32);
void RunCharacterSelectionManagerProcess(void);
void RunCharacterSelectionPlayerProcess(void);
s32 GetDifficultySpriteBank(s32 difficulty);
s32 GetCharacterSpriteBank(s32 characterIndex);
float InterpolateLinear(float, float, float, float); // seems implicit in main.c
float InterpolateByStep(float, float, float); // seems implicit in main.c
float InterpolateCosine(float, float, float, float); // seems implicit in main.c
float InterpolateSine(float, float, float, float); // seems implcit in main.c
void CreateCharacterSelectMessageWindow(void);
void DestroyCharacterSelectMessageWindow(void);
void ShowCharacterSelectMessage(u32 messageId, s32 waitFrames, s32 instant);
void InsertCharacterSelectMessage(u32 messageId, s32 insertIndex);
void ShowCharacterSelectTopMessage(u32 messageId);
void DestroyCharacterSelectTextWindow(void);
void SetPlayerCharacterSprite(void);
void SetPlayerDifficultySprite(void);
void SetPlayerOptionSprite(void);
void PlayCharacterSelectionSound(s32 characterIndex);

u8 selectedLanguage;
s32 characterSelectSpriteGroups[0x1C];
CharacterSelectPlayerWork characterSelectSummaryWork;
CharacterSelectPlayerWork characterSelectPlayers[4];
s32 characterSelectPlayerCounters[5];
s32 characterSelectionModeFlag;
HUPROCESS *characterSelectProcess;

void CreateCharacterSelectionProcesses(HUPROCESS *parentProcess)
{
    s32 loopIndex;

    characterSelectProcess = parentProcess;
    CreateCharacterSelectionSprites();
    CreateDifficultySelectionSprites();
    CreateCharacterSelectionPanel();
    HuPrcChildCreate(RunCharacterSelectionManagerProcess, 0x3E8, 0x2000, 0, characterSelectProcess);
    for (loopIndex = 0; loopIndex < 4; loopIndex++) {
        HuPrcChildCreate(RunCharacterSelectionPlayerProcess, 0x3E8, 0x2000, 0, characterSelectProcess);
    }
}

s32 RunCharacterSelectionFlow(s32 gameMode)
{
    s32 playerIndex;
    s32 characterSelectionValue;
    s32 characterSelectionValue2;
    s32 characterSelectionValue3;
    s32 characterSelectionValue4;
    s32 characterSelectionValue5;

    characterSelectionValue2 = -1;
    AnimateCharacterSelectionSpritesIn();
    CreateCharacterSelectMessageWindow();
    ShowCharacterSelectMessage(0x1A0005, -1, -1);
    WaitFrames(1);
    ShowCharacterSelectTopMessage(0x330091);
    characterSelectSummaryWork.updateCallback = (CharacterSelectUpdateCallback)UpdateCharacterSelectionInput;
    characterSelectionValue2 = -1;
    while (TRUE) {
        WaitFrames(1);
        characterSelectionValue = 0;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (characterSelectPlayers[playerIndex].characterId == 0) {
                characterSelectionValue++;
            }
        }
        if (characterSelectionValue2 != characterSelectionValue) {
            characterSelectionValue2 = characterSelectionValue;
            ShowCharacterSelectMessage(characterSelectionValue + 0x1A001B, -1, 1);
        }
        if (characterSelectSummaryWork.characterSelection != 0) {
            continue;
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_A) {
            HuAudFXPlay(2);
            characterSelectionValue3 = 1;
            break;
        }
        else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            HuAudFXPlay(3);
            characterSelectionValue3 = 0;
            break;
        }
    }
    HideCharacterSelectionCursor();
    characterSelectSummaryWork.updateCallback = NULL;
    DestroyCharacterSelectTextWindow();
    if (characterSelectionValue3 == 1) {
        characterSelectionValue4 = 0;
        characterSelectionValue5 = 0;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (characterSelectPlayers[playerIndex].selectedCharacter == 0) {
                characterSelectionValue4++;
            }
        }
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (characterSelectionValue5 < characterSelectionValue4) {
                if (HuPadStatGet(playerIndex) != -1) {
                    characterSelectPlayers[playerIndex].selectedCharacter = 0;
                    characterSelectionValue5++;
                }
                else {
                    characterSelectPlayers[playerIndex].selectedCharacter = 1;
                }
            }
            else {
                characterSelectPlayers[playerIndex].selectedCharacter = 1;
            }
        }
    }
    DestroyCharacterSelectMessageWindow();
    ScaleCharacterSelectionSpritesIn();
    return characterSelectionValue3;
}

s32 RunCharacterSelectionMode(s32 gameMode, s32 playerCount)
{
    s32 characterSelectionValue;
    s32 characterSelectionValue2;

    s32 playerIndex;
    s32 characterSelectionValue3;
    s32 characterSelectionValue4;
    s32 characterSelectionValue5;

    characterSelectionValue = 0;
    characterSelectionValue2 = 0;
    characterSelectionValue3 = 0;
    if (playerCount == 1) {
        characterSelectionValue3 = 1;
        for (playerIndex = 1; playerIndex < 4; playerIndex++) {
            characterSelectPlayers[playerIndex].selectedCharacter = 1;
        }
    }
    else {
        characterSelectionValue3 = 4;
    }
    UpdateSelectedDifficultyDisplay();
    if (gameMode == 0) {
        UpdateDifficultySelectionInputAlternate(0);
    }
    CreateCharacterSelectMessageWindow();
    WaitFrames(1);
    if (playerCount == 1) {
        ShowCharacterSelectMessage(0x1E005E, -1, -1);
    }
    else {
        ShowCharacterSelectMessage(0x1A0009, -1, -1);
    }
    ShowCharacterSelectTopMessage(0x330092);
    for (playerIndex = 0; playerIndex < characterSelectionValue3; playerIndex++) {
        if (characterSelectPlayers[playerIndex].selectedCharacter == 0) {
            characterSelectPlayers[playerIndex].updateCallback = (CharacterSelectUpdateCallback)UpdateCharacterSelectionPlayer;
        }
    }
    while (TRUE) {
        WaitFrames(1);
        if ((characterSelectPlayers->isActive == 0) && ((*HuPadBtnDown & 0x200) != 0)) {
            HuAudFXPlay(3);
            if (characterSelectSummaryWork.playerState == 1) {
                characterSelectSummaryWork.playerState = 0;
                continue;
            }
            characterSelectionValue5 = 0;
            break;
        }
        else {
            for (playerIndex = 0; playerIndex < characterSelectionValue3; playerIndex++) {
                if (characterSelectPlayers[playerIndex].selectedCharacter == 0 && characterSelectPlayers[playerIndex].isActive != 1) {
                    break;
                }
            }
            if (playerIndex == characterSelectionValue3) {
                characterSelectionValue5 = 2;
                break;
            }
        }
    }
    for (playerIndex = 0; playerIndex < characterSelectionValue3; playerIndex++) {
        characterSelectPlayers[playerIndex].updateCallback = NULL;
    }
    DestroyCharacterSelectTextWindow();
    DestroyCharacterSelectMessageWindow();
    if (characterSelectionValue5 == 0) {
        AnimateDifficultySelectionSprites(0);
        return 0;
    }
    characterSelectionValue4 = 0;
    for (playerIndex = 0; playerIndex < characterSelectionValue3; playerIndex++) {
        if (characterSelectPlayers[playerIndex].selectedCharacter != 0) {
            characterSelectPlayers[playerIndex].characterCursor = 0;
            characterSelectionValue4++;
        }
    }
    if ((characterSelectionValue4 == 0) || ((playerCount == 2) && (characterSelectionValue4 == 2))) {
        AnimateDifficultySelectionSprites(0);
        return 2;
    }
    if (playerCount == 2) {
        characterSelectPlayers[1].selectionMode = 0;
    }
    return 1;
}

void RunFourPlayerCharacterSelection(s32 gameMode)
{
    RunCharacterSelectionMode(gameMode, 4);
}

s32 RunDifficultySelectionMode(s32 gameMode, s32 playerCount)
{
    s32 characterSelectionValue;
    s32 playerIndex;
    s32 playerIndex2;
    s32 playerIndex3;
    s32 playerIndex4;
    s32 characterSelectionValue2;
    s32 characterSelectionValue3;
    s32 characterSelectionValue4;

    playerIndex2 = -1;
    playerIndex4 = -1;
    characterSelectionValue2 = -1;
    characterSelectionValue4 = -1;
    characterSelectionValue = 0;
    if (gameMode != 0) {
        if (playerCount == 2) {
            UpdateDifficultySelectionInput(0, 2);
        }
        else {
            UpdateDifficultySelectionInput(0, 1);
        }
    }
    CreateCharacterSelectMessageWindow();
    while (TRUE) {
        WaitFrames(1);
        characterSelectionValue3 = 0;
        for (playerIndex = 0; playerIndex < playerCount; playerIndex++) {
            if ((characterSelectPlayers[playerIndex].selectedCharacter != 0) && (characterSelectPlayers[playerIndex].isActive == 0)) {
                characterSelectionValue3++;
            }
        }
        if (characterSelectionValue2 == -1) {
            InsertCharacterSelectMessage(characterSelectionValue3 + 0x1A0023, 1);
            ShowCharacterSelectMessage(0x1A0028, -1, characterSelectionValue2);
        }
        else {
            InsertCharacterSelectMessage(characterSelectionValue3 + 0x1A0023, 1);
            ShowCharacterSelectMessage(0x1A0028, -1, characterSelectionValue2);
        }
        characterSelectionValue2 = 1;
        ShowCharacterSelectTopMessage(0x330092);
        for (playerIndex = 0; playerIndex < playerCount; playerIndex++) {
            if ((characterSelectPlayers[playerIndex].selectedCharacter != 0) && (characterSelectPlayers[playerIndex].isActive == 0)) {
                playerIndex2 = playerIndex;
                characterSelectPlayers[playerIndex].playerIndex = playerIndex4;
                AdvanceOverlappingCharacterCursor(&characterSelectPlayers[playerIndex]);
                characterSelectPlayers[playerIndex].updateCallback = (CharacterSelectUpdateCallback)UpdateDifficultySelectionPlayer;
                break;
            }
        }
        while (TRUE) {
            WaitFrames(1);
            if (((HuPadBtnDown[characterSelectPlayers->controllerPort] & 0x200) != 0) && (characterSelectPlayers[playerIndex2].isActive == 0) && (characterSelectSummaryWork.selectionTimer == 0)) {
                HuAudFXPlay(3);
                if (playerIndex4 == -1) {
                    playerIndex3 = 0;
                }
                else {
                    playerIndex3 = 1;
                }
                break;
            }
            else if (characterSelectPlayers[playerIndex2].isActive == 1) {
                for (playerIndex = 0; playerIndex < playerCount; playerIndex++) {
                    if (characterSelectPlayers[playerIndex].isActive == 0) {
                        characterSelectPlayers[playerIndex].characterCursor = 0;
                    }
                }
                characterSelectPlayers[playerIndex2].updateCallback = NULL;
                ShowCharacterSelectMessage(0x1A0022, -1, 1);
                WaitFrames(1);
                ShowCharacterSelectTopMessage(0x330093);
                playerIndex3 = ProcessDifficultySelectionInput(&characterSelectPlayers[playerIndex2]);
                if (playerIndex3 == 1) {
                    HuAudFXPlay(2);
                    characterSelectPlayers[playerIndex2].playerState = 1;
                    playerIndex3 = 4;
                    playerIndex4 = playerIndex2;
                }
                else {
                    HuAudFXPlay(3);
                    characterSelectPlayers[playerIndex2].playerState = 0;
                    playerIndex3 = 4;
                    characterSelectPlayers[playerIndex2].isActive = 0;
                    HidePlayerDifficultyCursor(&characterSelectPlayers[playerIndex2]);
                    AdvanceOverlappingCharacterCursor(&characterSelectPlayers[playerIndex2]);
                }
                break;
            }
        }
        for (playerIndex = 0; playerIndex < playerCount; playerIndex++) {
            characterSelectPlayers[playerIndex].updateCallback = NULL;
        }
        for (playerIndex = 0; playerIndex < playerCount; playerIndex++) {
            if (characterSelectPlayers[playerIndex].isActive == 0) {
                break;
            }
        }
        if (playerIndex == playerCount) {
            playerIndex3 = 3;
            break;
        }
        else {
            if (playerIndex3 == 1) {
                AdvanceOverlappingCharacterCursor(&characterSelectPlayers[playerIndex2]);
                HidePlayerDifficultyCursor(&characterSelectPlayers[playerIndex2]);
                AdvanceOverlappingCharacterCursor(&characterSelectPlayers[characterSelectPlayers[playerIndex2].playerIndex]);
                playerIndex4 = characterSelectPlayers[characterSelectPlayers[playerIndex2].playerIndex].playerIndex;
                characterSelectPlayers[characterSelectPlayers[playerIndex2].playerIndex].isActive = 0;
            }
            else if (playerIndex3 != 4) {
                if (playerIndex3 == 0) {
                    HidePlayerDifficultyCursor(&characterSelectPlayers[playerIndex2]);
                }
                break;
            }
        }
        characterSelectionValue4 = -1;
    }
    DestroyCharacterSelectTextWindow();
    DestroyCharacterSelectMessageWindow();
    if (playerIndex3 != 0) {
        AnimateDifficultySelectionSprites(0);
    }
    return playerIndex3;
}

void RunFourPlayerDifficultySelection(s32 gameMode)
{
    RunDifficultySelectionMode(gameMode, 4);
}

void InitializeDefaultCharacterSelection(void)
{
    RunCharacterSelectionMode(0, 1);
}

s32 RunCharacterSelectionAlternate(void)
{
    s32 playerIndex;
    s32 characterSelectionValue;
    s32 characterSelectionValue2;
    s32 characterSelectionValue3;
    s32 characterSelectionValue4;
    s32 characterSelectionValue5;

    characterSelectionValue5 = -1;
    AnimateCharacterSelectionSpritesOut();
    CreateCharacterSelectMessageWindow();
    ShowCharacterSelectMessage(0x1A0005, -1, -1);
    WaitFrames(1);
    ShowCharacterSelectTopMessage(0x330091);
    characterSelectSummaryWork.updateCallback = (CharacterSelectUpdateCallback)UpdateCharacterSelectionInputAlternate;

    while (TRUE) {
        WaitFrames(1);
        characterSelectionValue = 0;
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            if (characterSelectPlayers[playerIndex].characterId == 0) {
                characterSelectionValue++;
            }
        }
        if (characterSelectionValue5 != characterSelectionValue) {
            characterSelectionValue5 = characterSelectionValue;
            ShowCharacterSelectMessage(characterSelectionValue + 0x330093, -1, 1);
        }
        if (characterSelectSummaryWork.characterSelection != 0) {
            continue;
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_A) {
            HuAudFXPlay(2);
            characterSelectionValue3 = 1;
            break;
        }
        else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            HuAudFXPlay(3);
            characterSelectionValue3 = 0;
            break;
        }
    }
    HideCharacterSelectionCursor();
    characterSelectSummaryWork.updateCallback = NULL;
    DestroyCharacterSelectTextWindow();
    if (characterSelectionValue3 == 1) {
        characterSelectionValue2 = 0;
        characterSelectionValue4 = 0;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (characterSelectPlayers[playerIndex].selectedCharacter == 0) {
                characterSelectionValue2++;
            }
        }
        if (characterSelectionValue2 >= 2) {
            characterSelectionValue2 = 2;
        }
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (characterSelectionValue4 < characterSelectionValue2) {
                if (HuPadStatGet(playerIndex) != -1) {
                    characterSelectPlayers[playerIndex].selectedCharacter = 0;
                    characterSelectPlayers[playerIndex].selectionMode = 0;
                    characterSelectionValue4++;
                }
                else {
                    characterSelectPlayers[playerIndex].selectedCharacter = 1;
                    characterSelectPlayers[playerIndex].selectionMode = 1;
                }
            }
            else {
                characterSelectPlayers[playerIndex].selectedCharacter = 1;
                characterSelectPlayers[playerIndex].selectionMode = 1;
            }
        }
    }
    DestroyCharacterSelectMessageWindow();
    ScaleCharacterSelectionSpritesIn();
    return characterSelectionValue3;
}

void RunTwoPlayerCharacterSelection(s32 gameMode)
{
    RunCharacterSelectionMode(gameMode, 2);
}

void RunTwoPlayerDifficultySelection(s32 gameMode)
{
    RunDifficultySelectionMode(gameMode, 2);
}

void RunCharacterSelectionForGame(void)
{
    RunCharacterSelectionFlow(0);
}

s32 RunDifficultySelectionAlternate(s32 gameMode)
{
    s32 soundEffectIds[2];
    s32 playerIndex;
    s32 characterSelectionValue;
    s32 characterSelectionValue2;
    s32 characterSelectionValue3;
    s32 characterSelectionValue4;

    soundEffectIds[1] = 0;
    soundEffectIds[0] = 0;
    characterSelectionValue = 4;
    characterSelectionValue3 = 4;
    if (characterSelectionValue3 == 1) {
        characterSelectionValue = 1;
        for (playerIndex = 1; playerIndex < 4; playerIndex++) {
            characterSelectPlayers[playerIndex].selectedCharacter = 1;
        }
    }
    else {
        characterSelectionValue = 4;
    }
    UpdateSelectedDifficultyDisplay();
    if (gameMode == 0) {
        UpdateDifficultySelectionInputAlternate(3);
    }
    CreateCharacterSelectMessageWindow();
    WaitFrames(1);
    ShowCharacterSelectMessage(0x1A0009, -1, -1);
    ShowCharacterSelectTopMessage(0x330092);
    for (playerIndex = 0; playerIndex < characterSelectionValue; playerIndex++) {
        if (characterSelectPlayers[playerIndex].selectedCharacter == 0) {
            characterSelectPlayers[playerIndex].updateCallback = (CharacterSelectUpdateCallback)UpdateCharacterSelectionPlayer;
        }
    }
    while (TRUE) {
        WaitFrames(1);
        if ((characterSelectPlayers->isActive == 0) && (HuPadBtnDown[0] & PAD_BUTTON_B)) {
            HuAudFXPlay(3);
            if (characterSelectSummaryWork.playerState == 1) {
                characterSelectSummaryWork.playerState = 0;
                continue;
            }
            characterSelectionValue4 = 0;
            break;
        }
        else {
            for (playerIndex = 0; playerIndex < characterSelectionValue; playerIndex++) {
                if (characterSelectPlayers[playerIndex].selectedCharacter == 0 && characterSelectPlayers[playerIndex].isActive != 1) {
                    break;
                }
            }
            if (playerIndex == characterSelectionValue) {
                characterSelectionValue4 = 2;
                break;
            }
        }
    }
    for (playerIndex = 0; playerIndex < characterSelectionValue; playerIndex++) {
        characterSelectPlayers[playerIndex].updateCallback = NULL;
    }
    DestroyCharacterSelectTextWindow();
    DestroyCharacterSelectMessageWindow();
    if (characterSelectionValue4 == 0) {
        AnimateDifficultySelectionSprites(0);
        return 0;
    }
    characterSelectionValue2 = 0;
    for (playerIndex = 0; playerIndex < characterSelectionValue; playerIndex++) {
        if (characterSelectPlayers[playerIndex].selectedCharacter != 0) {
            characterSelectPlayers[playerIndex].characterCursor = 1;
            characterSelectionValue2++;
        }
    }
    if ((characterSelectionValue2 == 0) || ((characterSelectionValue3 == 2) && (characterSelectionValue2 == 2))) {
        AnimateDifficultySelectionSprites(3);
        return 2;
    }
    if (characterSelectionValue3 == 2) {
        characterSelectPlayers[1].selectionMode = 0;
    }
    return 1;
}

s32 RunCharacterSelectionAndDifficulty(s32 gameMode)
{
    s32 characterSelectionValue;
    s32 playerIndex;
    s32 playerIndex2;
    s32 playerIndex3;
    s32 characterSelectionValue2;
    s32 playerIndex4;
    s32 characterSelectionValue3;
    s32 characterSelectionValue4;
    s32 characterSelectionValue5;

    playerIndex2 = -1;
    playerIndex4 = -1;
    characterSelectionValue3 = -1;
    characterSelectionValue2 = 4;
    characterSelectionValue5 = -1;
    characterSelectionValue = 0;
    if (gameMode != 0) {
        UpdateDifficultySelectionInput(3, 3);
    }
    CreateCharacterSelectMessageWindow();
    while (TRUE) {
        WaitFrames(1);
        characterSelectionValue4 = 0;
        for (playerIndex = 0; playerIndex < characterSelectionValue2; playerIndex++) {
            if ((characterSelectPlayers[playerIndex].selectedCharacter != 0) && (characterSelectPlayers[playerIndex].isActive == 0)) {
                characterSelectionValue4++;
            }
        }
        if (characterSelectionValue3 == -1) {
            InsertCharacterSelectMessage(characterSelectionValue4 + 0x1A0023, 1);
            ShowCharacterSelectMessage(0x1A0028, -1, characterSelectionValue3);
        }
        else {
            InsertCharacterSelectMessage(characterSelectionValue4 + 0x1A0023, 1);
            ShowCharacterSelectMessage(0x1A0028, -1, characterSelectionValue3);
        }
        characterSelectionValue3 = 1;
        ShowCharacterSelectTopMessage(0x330092);
        for (playerIndex = 0; playerIndex < characterSelectionValue2; playerIndex++) {
            if ((characterSelectPlayers[playerIndex].selectedCharacter != 0) && (characterSelectPlayers[playerIndex].isActive == 0)) {
                playerIndex2 = playerIndex;
                characterSelectPlayers[playerIndex].playerIndex = playerIndex4;
                AdvanceOverlappingCharacterCursor(&characterSelectPlayers[playerIndex]);
                characterSelectPlayers[playerIndex].updateCallback = (CharacterSelectUpdateCallback)UpdateDifficultySelectionPlayer;
                break;
            }
        }
        while (TRUE) {
            WaitFrames(1);
            if (((HuPadBtnDown[characterSelectPlayers->controllerPort] & PAD_BUTTON_B) != 0) && (characterSelectPlayers[playerIndex2].isActive == 0) && (characterSelectSummaryWork.selectionTimer == 0)) {
                HuAudFXPlay(3);
                if (playerIndex4 == -1) {
                    playerIndex3 = 0;
                }
                else {
                    playerIndex3 = 1;
                }
                break;
            }
            else if (characterSelectPlayers[playerIndex2].isActive == 1) {
                for (playerIndex = 0; playerIndex < characterSelectionValue2; playerIndex++) {
                    if (characterSelectPlayers[playerIndex].isActive == 0) {
                        characterSelectPlayers[playerIndex].characterCursor = 1;
                    }
                }
                characterSelectPlayers[playerIndex2].updateCallback = NULL;
                ShowCharacterSelectMessage(0x1A0022, -1, 1);
                WaitFrames(1);
                ShowCharacterSelectTopMessage(0x330093);
                playerIndex3 = ProcessDifficultySelectionInput(&characterSelectPlayers[playerIndex2]);
                if (playerIndex3 == 1) {
                    HuAudFXPlay(2);
                    characterSelectPlayers[playerIndex2].playerState = 1;
                    playerIndex3 = 4;
                    playerIndex4 = playerIndex2;
                }
                else {
                    HuAudFXPlay(3);
                    characterSelectPlayers[playerIndex2].playerState = 0;
                    playerIndex3 = 4;
                    characterSelectPlayers[playerIndex2].isActive = 0;
                    HidePlayerDifficultyCursor(&characterSelectPlayers[playerIndex2]);
                    AdvanceOverlappingCharacterCursor(&characterSelectPlayers[playerIndex2]);
                }
                break;
            }
        }
        for (playerIndex = 0; playerIndex < characterSelectionValue2; playerIndex++) {
            characterSelectPlayers[playerIndex].updateCallback = NULL;
        }
        for (playerIndex = 0; playerIndex < characterSelectionValue2; playerIndex++) {
            if (characterSelectPlayers[playerIndex].isActive == 0) {
                break;
            }
        }
        if (playerIndex == characterSelectionValue2) {
            playerIndex3 = 3;
            break;
        }
        else {
            if (playerIndex3 == 1) {
                AdvanceOverlappingCharacterCursor(&characterSelectPlayers[playerIndex2]);
                HidePlayerDifficultyCursor(&characterSelectPlayers[playerIndex2]);
                AdvanceOverlappingCharacterCursor(&characterSelectPlayers[characterSelectPlayers[playerIndex2].playerIndex]);
                playerIndex4 = characterSelectPlayers[characterSelectPlayers[playerIndex2].playerIndex].playerIndex;
                characterSelectPlayers[characterSelectPlayers[playerIndex2].playerIndex].isActive = 0;
            }
            else if (playerIndex3 != 4) {
                if (playerIndex3 == 0) {
                    HidePlayerDifficultyCursor(&characterSelectPlayers[playerIndex2]);
                }
                break;
            }
            characterSelectionValue5 = -1;
        }
    }
    DestroyCharacterSelectTextWindow();
    DestroyCharacterSelectMessageWindow();
    if (playerIndex3 != 0) {
        AnimateDifficultySelectionSprites(3);
    }
    return playerIndex3;
}

void ApplySelectedPlayerSettings(s32 gameMode)
{
    s32 playerIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        GWPlayer[playerIndex].team = GWPlayerCfg[playerIndex].group = characterSelectPlayers[playerIndex].selectionMode;
        GWPlayer[playerIndex].com = GWPlayerCfg[playerIndex].iscom = characterSelectPlayers[playerIndex].selectedCharacter;
        GWPlayer[playerIndex].diff = GWPlayerCfg[playerIndex].diff = characterSelectPlayers[playerIndex].selectedDifficulty;
        if (characterSelectionModeFlag != 0) {
            if (characterSelectPlayers[playerIndex].characterCursor <= 5) {
                GWPlayer[playerIndex].character = GWPlayerCfg[playerIndex].character = characterSelectPlayers[playerIndex].characterCursor - 1;
            }
            else if (characterSelectPlayers[playerIndex].characterCursor <= 0xB) {
                GWPlayer[playerIndex].character = GWPlayerCfg[playerIndex].character = characterSelectPlayers[playerIndex].characterCursor - 3;
            }
            else {
                GWPlayer[playerIndex].character = GWPlayerCfg[playerIndex].character = GetCharacterSpriteBank(characterSelectPlayers[playerIndex].characterCursor - 4);
            }
        }
        else if (gameMode == 1) {
            if (playerIndex != 0) {
                characterSelectPlayers[playerIndex].characterCursor = characterSelectPlayers[playerIndex - 1].characterCursor + 1;
                if (characterSelectPlayers[playerIndex].characterCursor >= 8) {
                    characterSelectPlayers[playerIndex].characterCursor = 0;
                }
            }
            GWPlayer[playerIndex].character = GWPlayerCfg[playerIndex].character = characterSelectPlayers[playerIndex].characterCursor;
        }
        else {
            GWPlayer[playerIndex].character = GWPlayerCfg[playerIndex].character = characterSelectPlayers[playerIndex].characterCursor;
        }
        GWPlayer[playerIndex].port = GWPlayerCfg[playerIndex].pad_idx = characterSelectPlayers[playerIndex].controllerPort;
    }
    OSReport("\n\n#################### SET PLAYERDATA ####################\n");
    OSReport("ID) GRP:COM:DIF:CHR:PAD\n");
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        OSReport("%2d) %3d:%3d:%3d:%3d:%3d\n", playerIndex, GWPlayerCfg[playerIndex].group, GWPlayerCfg[playerIndex].iscom, GWPlayerCfg[playerIndex].diff,
            GWPlayerCfg[playerIndex].character, GWPlayerCfg[playerIndex].pad_idx);
    }
    OSReport("########################################################\n");
}

s32 InitializeCharacterSelection(s32 gameMode, s32 loadSavedSettings)
{
    s32 playerIndex;
    s32 characterSelectionValue;
    s32 characterSelectionValue2;
    s32 characterSelectionValue3;
    s32 characterSelectionValue4;
    s32 characterSelectionValue5;
    s32 characterSelectionValue6;
    s32 characterSelectionValue7;
    s32 characterSelectionValue8;
    s32 characterSelectionValue9;
    s32 characterSelectionValue10;
    s32 characterSelectionValue11;
    s32 characterSelectionValue12;

    characterSelectionValue = 0;
    characterSelectionValue4 = 0;
    characterSelectionValue5 = 0;
    characterSelectionValue2 = gameMode;
    if (gameMode == 3) {
        characterSelectionModeFlag = 1;
    }
    else {
        characterSelectionModeFlag = 0;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        characterSelectPlayers[playerIndex].spriteBaseId = playerIndex;
        characterSelectPlayers[playerIndex].selectionMode = 0;
        characterSelectPlayers[playerIndex].selectedCharacter = 0;
        characterSelectPlayers[playerIndex].selectedDifficulty = 0;
        if (characterSelectionModeFlag != 0) {
            characterSelectPlayers[playerIndex].characterCursor = playerIndex + 1;
        }
        else {
            characterSelectPlayers[playerIndex].characterCursor = playerIndex;
        }
        characterSelectPlayers[playerIndex].controllerPort = playerIndex;
    }
    if (loadSavedSettings == 1) {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            characterSelectPlayers[playerIndex].spriteBaseId = playerIndex;
            characterSelectPlayers[playerIndex].selectionMode = GWPlayerCfg[playerIndex].group;
            characterSelectPlayers[playerIndex].selectedCharacter = GWPlayerCfg[playerIndex].iscom;
            characterSelectPlayers[playerIndex].selectedDifficulty = GWPlayerCfg[playerIndex].diff;
            characterSelectPlayers[playerIndex].characterCursor = GWPlayerCfg[playerIndex].character;
            characterSelectPlayers[playerIndex].controllerPort = GWPlayerCfg[playerIndex].pad_idx;
            characterSelectPlayers[playerIndex].isActive = characterSelectPlayers[playerIndex].playerIndex = characterSelectPlayers[playerIndex].playerState = characterSelectPlayers[playerIndex].characterId = 0;
            characterSelectPlayers[playerIndex].characterSelection = characterSelectPlayers[playerIndex].characterState = characterSelectPlayers[playerIndex].selectionTimer = characterSelectPlayers[playerIndex].selectionFlags = 0;

            switch (gameMode) {
                case 0:
                    if (characterSelectPlayers[playerIndex].selectedCharacter == 0) {
                        characterSelectPlayers[playerIndex].isActive = 1;
                    }
                    else {
                        characterSelectionValue4 = 1;
                    }
                    break;
                case 2:
                    if (characterSelectPlayers[playerIndex].selectedCharacter == 0) {
                        characterSelectPlayers[playerIndex].isActive = 1;
                        characterSelectionValue5++;
                    }
                    else {
                        characterSelectionValue4 = 1;
                    }
                    break;
                case 3:
                    if (characterSelectPlayers[playerIndex].selectedCharacter == 0) {
                        characterSelectPlayers[playerIndex].isActive = 1;
                        characterSelectionValue5++;
                    }
                    else {
                        characterSelectionValue4 = 1;
                    }
                    if (characterSelectPlayers[playerIndex].characterCursor <= 3) {
                        characterSelectPlayers[playerIndex].characterCursor += 1;
                    }
                    else if (characterSelectPlayers[playerIndex].characterCursor <= 7) {
                        characterSelectPlayers[playerIndex].characterCursor += 3;
                    }
                    else {
                        characterSelectPlayers[playerIndex].characterCursor = GetDifficultySpriteBank(characterSelectPlayers[playerIndex].characterCursor) + 4;
                    }
                    break;
            }
        }
        switch (gameMode) {
            case 0:
                if (characterSelectionValue4 == 1) {
                    characterSelectionValue2 = 0x66;
                    characterSelectionValue = 1;
                }
                else {
                    characterSelectionValue2 = 0x65;
                    characterSelectionValue = 0;
                }
                break;
            case 1:
                characterSelectionValue2 = 0xC8;
                break;
            case 2:
                if ((characterSelectionValue4 == 1) && (characterSelectionValue5 == 1)) {
                    characterSelectionValue2 = 0x12E;
                    characterSelectionValue = 1;
                }
                else {
                    characterSelectionValue2 = 0x12D;
                    characterSelectionValue = 0;
                }
                break;
            case 3:
                if (characterSelectionValue4 == 1) {
                    characterSelectionValue2 = 0x192;
                    characterSelectionValue = 1;
                }
                else {
                    characterSelectionValue2 = 0x191;
                    characterSelectionValue = 0;
                }
                break;
        }
    }
    while (TRUE) {
        WaitFrames(1);
        if (characterSelectionValue3 == 0x3E7) {
            break;
        }
        switch (characterSelectionValue2) {
            case 0x0:
                characterSelectionValue2 = 0x64;
                break;
            case 0x1:
                characterSelectionValue2 = 0xC8;
                break;
            case 0x2:
                characterSelectionValue2 = 0x12C;
                break;
            case 0x3:
                characterSelectionValue2 = 0x190;
                break;
            case 0x64:
                characterSelectionValue3 = RunCharacterSelectionFlow(characterSelectionValue);
                if (characterSelectionValue3 != 0) {
                    characterSelectionValue2 = 0x65;
                    characterSelectionValue = 0;
                }
                else {
                    characterSelectionValue2 = 0x385;
                    characterSelectionValue = 1;
                }
                break;
            case 0x65:
                characterSelectionValue7 = RunCharacterSelectionMode(characterSelectionValue, 4);
                characterSelectionValue3 = characterSelectionValue7;
                if (characterSelectionValue3 == 1) {
                    characterSelectionValue2 = 0x66;
                    characterSelectionValue = 0;
                }
                else if (characterSelectionValue3 == 2) {
                    characterSelectionValue2 = 0x384;
                    characterSelectionValue = 0;
                }
                else {
                    characterSelectionValue2 = 0x64;
                    characterSelectionValue = 1;
                }
                break;
            case 0x66:
                characterSelectionValue8 = RunDifficultySelectionMode(characterSelectionValue, 4);
                characterSelectionValue3 = characterSelectionValue8;
                if (characterSelectionValue3 != 0) {
                    characterSelectionValue2 = 0x384;
                    characterSelectionValue = 0;
                }
                else {
                    characterSelectionValue2 = 0x65;
                    characterSelectionValue = 1;
                }
                break;
            case 0xC8:
                characterSelectionValue9 = RunCharacterSelectionMode(0, 1);
                characterSelectionValue3 = characterSelectionValue9;
                if (characterSelectionValue3 != 0) {
                    characterSelectionValue2 = 0x384;
                    characterSelectionValue = 0;
                }
                else {
                    characterSelectionValue2 = 0x385;
                    characterSelectionValue = 1;
                }
                break;
            case 0x12C:
                characterSelectionValue3 = RunCharacterSelectionAlternate();
                if (characterSelectionValue3 != 0) {
                    characterSelectionValue2 = 0x12D;
                    characterSelectionValue = 0;
                }
                else {
                    characterSelectionValue2 = 0x385;
                    characterSelectionValue = 1;
                }
                break;
            case 0x12D:
                characterSelectionValue10 = RunCharacterSelectionMode(characterSelectionValue, 2);
                characterSelectionValue3 = characterSelectionValue10;
                if (characterSelectionValue3 == 1) {
                    characterSelectionValue2 = 0x12E;
                    characterSelectionValue = 0;
                }
                else if (characterSelectionValue3 == 2) {
                    characterSelectionValue2 = 0x384;
                    characterSelectionValue = 0;
                }
                else {
                    characterSelectionValue2 = 0x12C;
                    characterSelectionValue = 1;
                }
                break;
            case 0x12E:
                characterSelectionValue11 = RunDifficultySelectionMode(characterSelectionValue, 2);
                characterSelectionValue3 = characterSelectionValue11;
                if (characterSelectionValue3 != 0) {
                    characterSelectionValue2 = 0x384;
                    characterSelectionValue = 0;
                }
                else {
                    characterSelectionValue2 = 0x12D;
                    characterSelectionValue = 1;
                }
                break;
            case 0x190:
                characterSelectionValue12 = RunCharacterSelectionFlow(0);
                characterSelectionValue3 = characterSelectionValue12;
                if (characterSelectionValue3 != 0) {
                    characterSelectionValue2 = 0x191;
                    characterSelectionValue = 0;
                }
                else {
                    characterSelectionValue2 = 0x385;
                    characterSelectionValue = 1;
                }
                break;
            case 0x191:
                characterSelectionValue3 = RunDifficultySelectionAlternate(characterSelectionValue);
                if (characterSelectionValue3 == 1) {
                    characterSelectionValue2 = 0x192;
                    characterSelectionValue = 0;
                }
                else if (characterSelectionValue3 == 2) {
                    characterSelectionValue2 = 0x384;
                    characterSelectionValue = 0;
                }
                else {
                    characterSelectionValue2 = 0x190;
                    characterSelectionValue = 1;
                }
                break;
            case 0x192:
                characterSelectionValue3 = RunCharacterSelectionAndDifficulty(characterSelectionValue);
                if (characterSelectionValue3 != 0) {
                    characterSelectionValue2 = 0x384;
                    characterSelectionValue = 0;
                }
                else {
                    characterSelectionValue2 = 0x191;
                    characterSelectionValue = 1;
                }
                break;
            case 0x384:
                ApplySelectedPlayerSettings(gameMode);
                characterSelectionValue6 = 1;
                characterSelectionValue3 = 0x3E7;
                break;
            case 0x385:
                characterSelectionValue6 = 0;
                characterSelectionValue3 = 0x3E7;
                break;
        }
    }
    return characterSelectionValue6;
}

void SelectCharacterMode(s32 selectionResult)
{
    AnimateCharacterSelectionPanelOut(selectionResult);
}

void SelectDifficultyMode(s32 selectionResult)
{
    AnimateCharacterSelectionPanelOutAlternate(selectionResult);
}

s32 twoPlayerCharacterPositions[4][2] = { { -0x69, 0 }, { -0x23, 0 }, { 0x23, 0 }, { 0x69, 0 } };
s32 fourPlayerCharacterPositions[8][2] = {
    { -0x7B, -0x34 },
    { -0x29, -0x34 },
    { 0x29, -0x34 },
    { 0x7B, -0x34 },
    { -0x7B, 0x34 },
    { -0x29, 0x34 },
    { 0x29, 0x34 },
    { 0x7B, 0x34 },
};
s32 largeRosterCharacterPositions[0xE][2] = {
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
s32 characterGridPositions[0x12][2] = {
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
s32 fourPlayerDifficultyPositions[4][2] = { { -0x45, -8 }, { -0x17, -8 }, { 0x17, -8 }, { 0x45, -8 } };
s32 singlePlayerDifficultyOffset[2] = { 0x00, 0xFFFFFFF8 };
s32 twoPlayerDifficultyPositions[2][2] = { { -0x17, -8 }, { 0x17, -8 } };
s32 fourPlayerDifficultyIconPositions[4][2] = { { -0x59, -8 }, { -0x2B, -8 }, { 0x2B, -8 }, { 0x59, -8 } };
s32 characterSelectMessageWindow = -1;
s32 characterSelectTextWindows[2] = {-1, 0};

void CreateCharacterSelectionSprites(void)
{
    s32 spriteIndex;
    s32 spriteGroupId;
    s32 spriteId;
    ANIMDATA *animationData;

    spriteGroupId = HuSprGrpCreate(0xB);
    *characterSelectSpriteGroups = spriteGroupId;
    spriteId = HuSprCreate(HuSprAnimReadFile(0x640046), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    HuSprPosSet(spriteGroupId, 0, 0.0f, 0.0f);
    animationData = HuSprAnimReadFile(0x640047);
    for (spriteIndex = 0; spriteIndex < 2; spriteIndex++) {
        spriteId = HuSprCreate(animationData, 0x3E8, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 1, spriteId);
        HuSprBankSet(spriteGroupId, spriteIndex + 1, spriteIndex);
    }
    animationData = HuSprAnimReadFile(0x640048);
    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        spriteId = HuSprCreate(animationData, 0x76C, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 3, spriteId);
        HuSprPosSet(spriteGroupId, spriteIndex + 3, twoPlayerCharacterPositions[spriteIndex % 4][0], twoPlayerCharacterPositions[0][1]);
        HuSprBankSet(spriteGroupId, spriteIndex + 3, 0);
    }
    PositionCharacterSelectionSprites();
    ScaleCharacterSelectionSprites();
}

void PositionCharacterSelectionSprites(void)
{
    s32 playerIndex;
    s32 spriteGroupId;

    spriteGroupId = *characterSelectSpriteGroups;
    for (playerIndex = 0; playerIndex < 0xB; playerIndex++) {
        HuSprAttrSet(spriteGroupId, playerIndex, 4);
    }
    HuSprAttrReset(spriteGroupId, 0, 4);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        HuSprAttrReset(spriteGroupId, playerIndex + 3, 4);
        HuSprPosSet(spriteGroupId, playerIndex + 3, twoPlayerCharacterPositions[playerIndex % 4][0], twoPlayerCharacterPositions[0][1]);
        HuSprBankSet(spriteGroupId, playerIndex + 3, 0);
    }
    HuSprGrpPosSet(spriteGroupId, 288.0f, -390.0f);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        characterSelectPlayers[playerIndex].characterId = characterSelectPlayers[playerIndex].selectedCharacter = 0;
    }
}

void PositionCharacterSelectionSpritesAlternate(void)
{
    s32 playerIndex;
    s32 spriteGroupId;

    spriteGroupId = *characterSelectSpriteGroups;
    for (playerIndex = 0; playerIndex < 0xB; playerIndex++) {
        HuSprAttrSet(spriteGroupId, playerIndex, 4);
    }
    HuSprAttrReset(spriteGroupId, 0, 4);
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        HuSprAttrReset(spriteGroupId, playerIndex + 3, 4);
        HuSprPosSet(spriteGroupId, playerIndex + 3, twoPlayerCharacterPositions[playerIndex % 4 + 1][0], twoPlayerCharacterPositions[0][1]);
        HuSprBankSet(spriteGroupId, playerIndex + 3, 0);
    }
    HuSprGrpPosSet(spriteGroupId, 288.0f, -390.0f);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        characterSelectPlayers[playerIndex].characterId = characterSelectPlayers[playerIndex].selectedCharacter = 0;
    }
}

void ScaleCharacterSelectionSprites(void)
{
    s32 spriteIndex;
    s32 spriteGroupId;

    spriteGroupId = *characterSelectSpriteGroups;
    for (spriteIndex = 0; spriteIndex < 0xB; spriteIndex++) {
        HuSprAttrSet(spriteGroupId, spriteIndex, 4);
    }
    HuSprGrpPosSet(spriteGroupId, 288.0f, 110.0f);
    HuSprGrpScaleSet(spriteGroupId, 1.0f, 1.0f);
}

void AnimateCharacterSelectionSpritesIn(void)
{
    float spriteGroupId;
    s32 playerIndex;
    s32 frameIndex;
    s32 spriteGroupId2;
    s32 spriteGroupId3;

    spriteGroupId3 = *characterSelectSpriteGroups;
    spriteGroupId2 = *characterSelectSpriteGroups;
    for (playerIndex = 0; playerIndex < 0xB; playerIndex++) {
        HuSprAttrSet(spriteGroupId2, playerIndex, 4);
    }
    HuSprAttrReset(spriteGroupId2, 0, 4);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        HuSprAttrReset(spriteGroupId2, playerIndex + 3, 4);
        HuSprPosSet(spriteGroupId2, playerIndex + 3, twoPlayerCharacterPositions[playerIndex % 4][0], twoPlayerCharacterPositions[0][1]);
        HuSprBankSet(spriteGroupId2, playerIndex + 3, 0);
    }
    HuSprGrpPosSet(spriteGroupId2, 288.0f, -390.0f);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        characterSelectPlayers[playerIndex].characterId = characterSelectPlayers[playerIndex].selectedCharacter = 0;
    }
    for (frameIndex = 0; frameIndex <= 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0x32) {
            spriteGroupId = InterpolateSine(-500.0f, 10.0f, frameIndex, 50.0f);
        }
        else {
            spriteGroupId = InterpolateCosine(10.0f, 0.0f, frameIndex - 0x32, 10.0f);
        }
        HuSprGrpPosSet(spriteGroupId3, 288.0f, 110.0f + spriteGroupId);
    }
    HuSprGrpPosSet(spriteGroupId3, 288.0f, 110.0f);
    WaitFrames(0xA);
}

void AnimateCharacterSelectionSpritesOut(void)
{
    float spriteGroupId;
    s32 playerIndex;
    s32 frameIndex;
    s32 spriteGroupId2;
    s32 spriteGroupId3;

    spriteGroupId3 = *characterSelectSpriteGroups;
    spriteGroupId2 = *characterSelectSpriteGroups;
    for (playerIndex = 0; playerIndex < 0xB; playerIndex++) {
        HuSprAttrSet(spriteGroupId2, playerIndex, 4);
    }
    HuSprAttrReset(spriteGroupId2, 0, 4);
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        HuSprAttrReset(spriteGroupId2, playerIndex + 3, 4);
        HuSprPosSet(spriteGroupId2, playerIndex + 3, twoPlayerCharacterPositions[playerIndex % 4 + 1][0], twoPlayerCharacterPositions[0][1]);
        HuSprBankSet(spriteGroupId2, playerIndex + 3, 0);
    }
    HuSprGrpPosSet(spriteGroupId2, 288.0f, -390.0f);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        characterSelectPlayers[playerIndex].characterId = characterSelectPlayers[playerIndex].selectedCharacter = 0;
    }
    for (frameIndex = 0; frameIndex <= 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0x32) {
            spriteGroupId = InterpolateSine(-500.0f, 10.0f, frameIndex, 50.0f);
        }
        else {
            spriteGroupId = InterpolateCosine(10.0f, 0.0f, frameIndex - 0x32, 10.0f);
        }
        HuSprGrpPosSet(spriteGroupId3, 288.0f, 110.0f + spriteGroupId);
    }
    HuSprGrpPosSet(spriteGroupId3, 288.0f, 110.0f);
    WaitFrames(0xA);
}

void ScaleCharacterSelectionSpritesIn(void)
{
    float verticalOffset;
    s32 frameIndex;
    s32 spriteIndex;
    s32 spriteGroupId2;
    s32 characterSelectGroup = *characterSelectSpriteGroups;

    WaitFrames(0xA);
    for (frameIndex = 0; frameIndex <= 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0xA) {
            verticalOffset = InterpolateSine(0.0f, 10.0f, frameIndex, 10.0f);
        }
        else {
            verticalOffset = InterpolateCosine(10.0f, -500.0f, frameIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(characterSelectGroup, 288.0f, 110.0f + verticalOffset);
    }
    HuSprGrpPosSet(characterSelectGroup, 288.0f, -390.0f);
    spriteGroupId2 = *characterSelectSpriteGroups;
    for (spriteIndex = 0; spriteIndex < 0xB; spriteIndex++) {
        HuSprAttrSet(spriteGroupId2, spriteIndex, 4);
    }
    HuSprGrpPosSet(spriteGroupId2, 288.0f, 110.0f);
    HuSprGrpScaleSet(spriteGroupId2, 1.0f, 1.0f);
}

void ShowCharacterSelectionCursor(s32 playerCount)
{
    s32 positionValue;

    positionValue = *characterSelectSpriteGroups;
    HuSprAttrReset(positionValue, 1, 4);
    HuSprAttrReset(positionValue, 2, 4);
    HuSprPosSet(positionValue, 1, -155.0f, 0.0f);
    HuSprPosSet(positionValue, 2, 155.0f, 0.0f);
    if (playerCount == 2) {
        HuSprPosSet(positionValue, 1, -80.0f, 0.0f);
        HuSprPosSet(positionValue, 2, 80.0f, 0.0f);
    }
}

void HideCharacterSelectionCursor(void)
{
    s32 characterSelectionValue;

    characterSelectionValue = *characterSelectSpriteGroups;
    HuSprAttrSet(characterSelectionValue, 1, 4);
    HuSprAttrSet(characterSelectionValue, 2, 4);
}

void UpdateCharacterSelectionInput(void)
{
    float positionValue;
    float interpolatedValue;
    s32 playerIndex;
    s32 playerIndex2;
    s32 characterIndex;
    s32 characterSelectionValue;
    s32 positionValue2;
    s32 characterSelectionValue2;
    s32 positionValue3;
    s32 characterSelectionValue3;
    s32 characterSelectionValue4;

    positionValue2 = characterSelectSpriteGroups[0];
    characterIndex = characterSelectionValue = characterSelectionValue2 = characterSelectionValue3 = 0;
    characterSelectSummaryWork.characterSelection = 0;
    characterIndex = 1;
    for (playerIndex = 1; playerIndex < 4; playerIndex++) {
        if (characterSelectPlayers[playerIndex].selectedCharacter == 0) {
            characterIndex++;
        }
    }
    characterSelectionValue = 1;
    for (playerIndex = 1; playerIndex < 4; playerIndex++) {
        if (HuPadStatGet(playerIndex) != -1) {
            characterSelectionValue++;
        }
    }
    if (characterSelectionValue >= 2) {
        positionValue3 = characterSelectSpriteGroups[0];
        HuSprAttrReset(positionValue3, 1, 4);
        HuSprAttrReset(positionValue3, 2, 4);
        HuSprPosSet(positionValue3, 1, -155.0f, 0.0f);
        HuSprPosSet(positionValue3, 2, 155.0f, 0.0f);
        if (*HuPadStkX >= 5) {
            characterIndex++;
            characterSelectionValue2 = 1;
        }
        else if (*HuPadStkX <= -5) {
            characterIndex--;
            characterSelectionValue2 = -1;
        }
    }
    else {
        characterSelectionValue4 = characterSelectSpriteGroups[0];
        HuSprAttrSet(characterSelectionValue4, 1, 4);
        HuSprAttrSet(characterSelectionValue4, 2, 4);
    }
    if (characterIndex < 1) {
        characterIndex = characterSelectionValue;
    }
    else if (characterIndex > characterSelectionValue) {
        if (characterSelectionValue2 == 0) {
            characterIndex = characterSelectionValue;
            characterSelectionValue3 = 1;
        }
        else {
            characterIndex = 1;
        }
    }
    if ((characterSelectionValue2 != 0) || (characterSelectionValue3 != 0)) {
        characterSelectSummaryWork.characterSelection = 1;
        HuAudFXPlay(0);
        for (playerIndex = 0; playerIndex <= 0xA; playerIndex++) {
            if (playerIndex <= 5) {
                positionValue = InterpolateSine(0.0f, 10.0f, playerIndex, 5.0f);
                interpolatedValue = InterpolateSine(1.0f, 0.0f, playerIndex, 5.0f);
            }
            else {
                positionValue = InterpolateSine(10.0f, 0.0f, playerIndex - 5, 5.0f);
                interpolatedValue = InterpolateSine(0.0f, 1.0f, playerIndex - 5, 5.0f);
            }
            if (characterSelectionValue2 == 1) {
                HuSprPosSet(positionValue2, 2, 3.0f + (155.0f + positionValue), 0.0f);
            }
            else if (characterSelectionValue2 == -1) {
                HuSprPosSet(positionValue2, 1, (-155.0f - positionValue) - 3.0f, 0.0f);
            }
            for (playerIndex2 = 0; playerIndex2 < 4; playerIndex2++) {
                if ((playerIndex2 < characterIndex) && (characterSelectPlayers[playerIndex2].selectedCharacter == 1)) {
                    characterSelectPlayers[playerIndex2].characterId = 0;
                    HuSprScaleSet(positionValue2, playerIndex2 + 3, interpolatedValue, 1.0f);
                    if (playerIndex == 5) {
                        HuSprBankSet(positionValue2, playerIndex2 + 3, 0);
                    }
                }
                else if ((playerIndex2 >= characterIndex) && (characterSelectPlayers[playerIndex2].selectedCharacter == 0)) {
                    characterSelectPlayers[playerIndex2].characterId = 1;
                    HuSprScaleSet(positionValue2, playerIndex2 + 3, interpolatedValue, 1.0f);
                    if (playerIndex == 5) {
                        HuSprBankSet(positionValue2, playerIndex2 + 3, 1);
                    }
                }
            }
            WaitFrames(1);
        }
        for (playerIndex2 = 0; playerIndex2 < 4; playerIndex2++) {
            if (playerIndex2 < characterIndex) {
                characterSelectPlayers[playerIndex2].selectedCharacter = 0;
            }
            else {
                characterSelectPlayers[playerIndex2].selectedCharacter = 1;
            }
        }
    }
    characterSelectSummaryWork.characterSelection = 0;
}

void UpdateCharacterSelectionInputAlternate(void)
{
    float positionValue;
    float interpolatedValue;
    s32 playerIndex;
    s32 playerIndex2;
    s32 characterIndex;
    s32 characterSelectionValue;
    s32 positionValue2;
    s32 characterSelectionValue2;
    s32 positionValue3;
    s32 characterSelectionValue3;
    s32 characterSelectionValue4;

    positionValue2 = characterSelectSpriteGroups[0];
    characterIndex = characterSelectionValue = characterSelectionValue2 = characterSelectionValue3 = 0;
    characterSelectSummaryWork.characterSelection = 0;
    characterIndex = 1;
    for (playerIndex = 1; playerIndex < 2; playerIndex++) {
        if (characterSelectPlayers[playerIndex].selectedCharacter == 0) {
            characterIndex++;
        }
    }
    characterSelectionValue = 1;
    for (playerIndex = 1; playerIndex < 4; playerIndex++) {
        if (HuPadStatGet(playerIndex) != -1) {
            characterSelectionValue++;
        }
    }
    if (characterSelectionValue > 2) {
        characterSelectionValue = 2;
    }
    if (characterSelectionValue >= 2) {
        positionValue3 = characterSelectSpriteGroups[0];
        HuSprAttrReset(positionValue3, 1, 4);
        HuSprAttrReset(positionValue3, 2, 4);
        HuSprPosSet(positionValue3, 1, -155.0f, 0.0f);
        HuSprPosSet(positionValue3, 2, 155.0f, 0.0f);
        HuSprPosSet(positionValue3, 1, -80.0f, 0.0f);
        HuSprPosSet(positionValue3, 2, 80.0f, 0.0f);
        if (*HuPadStkX >= 5) {
            characterIndex++;
            characterSelectionValue2 = 1;
        }
        else if (*HuPadStkX <= -5) {
            characterIndex--;
            characterSelectionValue2 = -1;
        }
    }
    else {
        characterSelectionValue4 = characterSelectSpriteGroups[0];
        HuSprAttrSet(characterSelectionValue4, 1, 4);
        HuSprAttrSet(characterSelectionValue4, 2, 4);
    }
    if (characterIndex < 1) {
        characterIndex = characterSelectionValue;
    }
    else if (characterIndex > characterSelectionValue) {
        if (characterSelectionValue2 == 0) {
            characterIndex = characterSelectionValue;
            characterSelectionValue3 = 1;
        }
        else {
            characterIndex = 1;
        }
    }
    if ((characterSelectionValue2 != 0) || (characterSelectionValue3 != 0)) {
        characterSelectSummaryWork.characterSelection = 1;
        HuAudFXPlay(0);
        for (playerIndex = 0; playerIndex <= 0xA; playerIndex++) {
            if (playerIndex <= 5) {
                positionValue = InterpolateSine(0.0f, 10.0f, playerIndex, 5.0f);
                interpolatedValue = InterpolateSine(1.0f, 0.0f, playerIndex, 5.0f);
            }
            else {
                positionValue = InterpolateSine(10.0f, 0.0f, playerIndex - 5, 5.0f);
                interpolatedValue = InterpolateSine(0.0f, 1.0f, playerIndex - 5, 5.0f);
            }
            if (characterSelectionValue2 == 1) {
                HuSprPosSet(positionValue2, 2, 3.0f + (77.0f + positionValue), 0.0f);
            }
            else if (characterSelectionValue2 == -1) {
                HuSprPosSet(positionValue2, 1, (-77.0f - positionValue) - 3.0f, 0.0f);
            }
            for (playerIndex2 = 0; playerIndex2 < 4; playerIndex2++) {
                if ((playerIndex2 < characterIndex) && (characterSelectPlayers[playerIndex2].selectedCharacter == 1)) {
                    characterSelectPlayers[playerIndex2].characterId = 0;
                    HuSprScaleSet(positionValue2, playerIndex2 + 3, interpolatedValue, 1.0f);
                    if (playerIndex == 5) {
                        HuSprBankSet(positionValue2, playerIndex2 + 3, 0);
                    }
                }
                else if ((playerIndex2 >= characterIndex) && (characterSelectPlayers[playerIndex2].selectedCharacter == 0)) {
                    characterSelectPlayers[playerIndex2].characterId = 1;
                    HuSprScaleSet(positionValue2, playerIndex2 + 3, interpolatedValue, 1.0f);
                    if (playerIndex == 5) {
                        HuSprBankSet(positionValue2, playerIndex2 + 3, 1);
                    }
                }
            }
            WaitFrames(1);
        }
        for (playerIndex2 = 0; playerIndex2 < 4; playerIndex2++) {
            if (playerIndex2 < characterIndex) {
                characterSelectPlayers[playerIndex2].selectedCharacter = 0;
            }
            else {
                characterSelectPlayers[playerIndex2].selectedCharacter = 1;
            }
        }
    }
    characterSelectSummaryWork.characterSelection = 0;
}

void CreateDifficultySelectionSprites(void)
{
    s32 spriteIndex;
    s32 spriteId;
    s32 spriteGroupId;
    ANIMDATA *animationData;
    ANIMDATA *animationData2;

    spriteGroupId = HuSprGrpCreate(0x36);
    characterSelectSpriteGroups[1] = spriteGroupId;
    spriteId = HuSprCreate(HuSprAnimReadFile(0x64004A), 0xBB8, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    HuSprPosSet(spriteGroupId, 0, 0.0f, 0.0f);
    animationData = HuSprAnimReadFile(0x64005A);
    for (spriteIndex = 0; spriteIndex < 8; spriteIndex++) {
        spriteId = HuSprCreate(HuSprAnimReadFile(spriteIndex + 0x64004C), 0x76C, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 2, spriteId);
    }
    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        spriteId = HuSprCreate(HuSprAnimReadFile(spriteIndex + 0x64005B), 0x384, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0x1E, spriteId);
        spriteId = HuSprCreate(HuSprAnimReadFile(spriteIndex + 0x640060), 0x3E8, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0x22, spriteId);
    }
    animationData = HuSprAnimReadFile(0x64005F);
    animationData2 = HuSprAnimReadFile(0x640064);
    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        spriteId = HuSprCreate(animationData, 0x384, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0x26, spriteId);
        spriteId = HuSprCreate(animationData2, 0x3E8, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0x2A, spriteId);
    }
    animationData = HuSprAnimReadFile(0x640065);
    animationData2 = HuSprAnimReadFile(0x640066);
    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        spriteId = HuSprCreate(animationData, 0x384, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0x2E, spriteId);
        spriteId = HuSprCreate(animationData2, 0x320, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0x32, spriteId);
    }
    PositionDifficultySelectionSprites(0);
    HideDifficultySelectionSprites(0);
}

void PositionDifficultySelectionSprites(s32 playerCount)
{
    s32 spriteIndex;
    s32 spriteGroupId;

    spriteGroupId = characterSelectSpriteGroups[1];
    for (spriteIndex = 0; spriteIndex < 0x36; spriteIndex++) {
        HuSprAttrSet(spriteGroupId, spriteIndex, 4);
    }
    if (playerCount != 3) {
        HuSprAttrReset(spriteGroupId, 0, 4);
        for (spriteIndex = 0; spriteIndex < 8; spriteIndex++) {
            HuSprBankSet(spriteGroupId, spriteIndex + 2, 0);
            HuSprAttrReset(spriteGroupId, spriteIndex + 2, 4);
            HuSprPosSet(spriteGroupId, spriteIndex + 2, fourPlayerCharacterPositions[spriteIndex][0], fourPlayerCharacterPositions[spriteIndex][1]);
            HuSprBankSet(spriteGroupId, spriteIndex + 2, 0);
        }
    }
    else {
        HuSprAttrReset(spriteGroupId, 1, 4);
        HuSprPosSet(spriteGroupId, 1, 2.0f, 0.0f);
        for (spriteIndex = 0; spriteIndex < 0xE; spriteIndex++) {
            HuSprBankSet(spriteGroupId, spriteIndex + 2, 0);
            HuSprAttrReset(spriteGroupId, spriteIndex + 2, 4);
            HuSprPosSet(spriteGroupId, spriteIndex + 2, largeRosterCharacterPositions[spriteIndex][0], largeRosterCharacterPositions[spriteIndex][1]);
            HuSprBankSet(spriteGroupId, spriteIndex + 2, 0);
        }
    }
    HuSprGrpPosSet(spriteGroupId, 288.0f, -331.0f);
}

void HideDifficultySelectionSprites(s32 playerCount)
{
    s32 spriteIndex;
    s32 spriteGroupId;

    spriteGroupId = characterSelectSpriteGroups[1];
    for (spriteIndex = 0; spriteIndex < 0x36; spriteIndex++) {
        HuSprAttrSet(spriteGroupId, spriteIndex, 4);
    }
    HuSprGrpPosSet(spriteGroupId, 288.0f, -331.0f);
}

void UpdateDifficultySelectionInput(s32 playerCount, s32 selectionMode)
{
    float spriteGroupId;
    s32 spriteIndex;
    s32 spriteGroupId2;
    s32 frameIndex;
    s32 spriteGroupId3;

    spriteGroupId3 = characterSelectSpriteGroups[1];
    spriteGroupId2 = characterSelectSpriteGroups[1];
    for (spriteIndex = 0; spriteIndex < 0x36; spriteIndex++) {
        HuSprAttrSet(spriteGroupId2, spriteIndex, 4);
    }
    if (playerCount != 3) {
        HuSprAttrReset(spriteGroupId2, 0, 4);
        for (spriteIndex = 0; spriteIndex < 8; spriteIndex++) {
            HuSprBankSet(spriteGroupId2, spriteIndex + 2, 0);
            HuSprAttrReset(spriteGroupId2, spriteIndex + 2, 4);
            HuSprPosSet(spriteGroupId2, spriteIndex + 2, fourPlayerCharacterPositions[spriteIndex][0], fourPlayerCharacterPositions[spriteIndex][1]);
            HuSprBankSet(spriteGroupId2, spriteIndex + 2, 0);
        }
    }
    else {
        HuSprAttrReset(spriteGroupId2, 1, 4);
        HuSprPosSet(spriteGroupId2, 1, 2.0f, 0.0f);
        for (spriteIndex = 0; spriteIndex < 0xE; spriteIndex++) {
            HuSprBankSet(spriteGroupId2, spriteIndex + 2, 0);
            HuSprAttrReset(spriteGroupId2, spriteIndex + 2, 4);
            HuSprPosSet(spriteGroupId2, spriteIndex + 2, largeRosterCharacterPositions[spriteIndex][0], largeRosterCharacterPositions[spriteIndex][1]);
            HuSprBankSet(spriteGroupId2, spriteIndex + 2, 0);
        }
    }
    HuSprGrpPosSet(spriteGroupId2, 288.0f, -331.0f);
    if (selectionMode == 1) {
        SetPlayerCharacterSprite();
    }
    if (selectionMode == 2) {
        SetPlayerDifficultySprite();
    }
    if (selectionMode == 3) {
        SetPlayerOptionSprite();
    }
    for (frameIndex = 0; frameIndex <= 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0x32) {
            spriteGroupId = InterpolateSine(-500.0f, 10.0f, frameIndex, 50.0f);
        }
        else {
            spriteGroupId = InterpolateCosine(10.0f, 0.0f, frameIndex - 0x32, 10.0f);
        }
        if (characterSelectionModeFlag != 0) {
            HuSprGrpPosSet(spriteGroupId3, 288.0f, 20.0f + (169.0f + spriteGroupId));
        }
        else {
            HuSprGrpPosSet(spriteGroupId3, 288.0f, 169.0f + spriteGroupId);
        }
    }
    if (characterSelectionModeFlag != 0) {
        HuSprGrpPosSet(spriteGroupId3, 288.0f, 189.0f);
        return;
    }
    HuSprGrpPosSet(spriteGroupId3, 288.0f, 169.0f);
}

void UpdateDifficultySelectionInputAlternate(s32 playerCount)
{
    float spriteGroupId;
    s32 spriteIndex;
    s32 spriteGroupId2;
    s32 frameIndex;
    s32 spriteGroupId3;

    spriteGroupId3 = characterSelectSpriteGroups[1];
    spriteGroupId2 = characterSelectSpriteGroups[1];
    for (spriteIndex = 0; spriteIndex < 0x36; spriteIndex++) {
        HuSprAttrSet(spriteGroupId2, spriteIndex, 4);
    }
    if (playerCount != 3) {
        HuSprAttrReset(spriteGroupId2, 0, 4);
        for (spriteIndex = 0; spriteIndex < 8; spriteIndex++) {
            HuSprBankSet(spriteGroupId2, spriteIndex + 2, 0);
            HuSprAttrReset(spriteGroupId2, spriteIndex + 2, 4);
            HuSprPosSet(spriteGroupId2, spriteIndex + 2, fourPlayerCharacterPositions[spriteIndex][0], fourPlayerCharacterPositions[spriteIndex][1]);
            HuSprBankSet(spriteGroupId2, spriteIndex + 2, 0);
        }
    }
    else {
        HuSprAttrReset(spriteGroupId2, 1, 4);
        HuSprPosSet(spriteGroupId2, 1, 2.0f, 0.0f);
        for (spriteIndex = 0; spriteIndex < 0xE; spriteIndex++) {
            HuSprBankSet(spriteGroupId2, spriteIndex + 2, 0);
            HuSprAttrReset(spriteGroupId2, spriteIndex + 2, 4);
            HuSprPosSet(spriteGroupId2, spriteIndex + 2, largeRosterCharacterPositions[spriteIndex][0], largeRosterCharacterPositions[spriteIndex][1]);
            HuSprBankSet(spriteGroupId2, spriteIndex + 2, 0);
        }
    }
    HuSprGrpPosSet(spriteGroupId2, 288.0f, -331.0f);
    for (frameIndex = 0; frameIndex <= 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0x32) {
            spriteGroupId = InterpolateSine(-500.0f, 10.0f, frameIndex, 50.0f);
        }
        else {
            spriteGroupId = InterpolateCosine(10.0f, 0.0f, frameIndex - 0x32, 10.0f);
        }
        if (characterSelectionModeFlag != 0) {
            HuSprGrpPosSet(spriteGroupId3, 288.0f, 20.0f + (169.0f + spriteGroupId));
        }
        else {
            HuSprGrpPosSet(spriteGroupId3, 288.0f, 169.0f + spriteGroupId);
        }
    }
    if (characterSelectionModeFlag != 0) {
        HuSprGrpPosSet(spriteGroupId3, 288.0f, 189.0f);
        return;
    }
    HuSprGrpPosSet(spriteGroupId3, 288.0f, 169.0f);
}

void AnimateDifficultySelectionSprites(s32 playerCount)
{
    float spriteGroupId;
    s32 frameIndex;
    s32 spriteIndex;
    s32 spriteGroupId2;
    s32 spriteGroupId3;

    spriteGroupId2 = characterSelectSpriteGroups[1];
    for (frameIndex = 0; frameIndex <= 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0xA) {
            spriteGroupId = InterpolateSine(0.0f, 10.0f, frameIndex, 10.0f);
        }
        else {
            spriteGroupId = InterpolateCosine(10.0f, -500.0f, frameIndex - 0xA, 50.0f);
        }
        if (characterSelectionModeFlag != 0) {
            HuSprGrpPosSet(spriteGroupId2, 288.0f, 20.0f + (169.0f + spriteGroupId));
        }
        else {
            HuSprGrpPosSet(spriteGroupId2, 288.0f, 169.0f + spriteGroupId);
        }
    }
    HuSprGrpPosSet(spriteGroupId2, 288.0f, -331.0f);
    spriteGroupId3 = characterSelectSpriteGroups[1];
    for (spriteIndex = 0; spriteIndex < 0x36; spriteIndex++) {
        HuSprAttrSet(spriteGroupId3, spriteIndex, 4);
    }
    HuSprGrpPosSet(spriteGroupId3, 288.0f, -331.0f);
}

void UpdateSelectedDifficultyDisplay(void)
{
    float characterIndex;
    float characterIndex2;
    s32 playerIndex;
    s32 positionValue;
    s32 playerIndex2;

    positionValue = characterSelectSpriteGroups[1];
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        HuSprBankSet(positionValue, playerIndex + 0x1E, 0);
        HuSprBankSet(positionValue, playerIndex + 0x22, 0);
        HuSprBankSet(positionValue, playerIndex + 0x26, 0);
        HuSprBankSet(positionValue, playerIndex + 0x2A, 0);
        HuSprBankSet(positionValue, playerIndex + 0x2E, 0);
        HuSprBankSet(positionValue, playerIndex + 0x32, 0);
    }
    if (characterSelectionModeFlag != 0) {
        for (playerIndex = 0; playerIndex < 0xE; playerIndex++) {
            HuSprBankSet(positionValue, playerIndex + 2, 0);
        }
    }
    else {
        for (playerIndex = 0; playerIndex < 8; playerIndex++) {
            HuSprBankSet(positionValue, playerIndex + 2, 0);
        }
    }
    for (playerIndex = 0; playerIndex <= 0xA; playerIndex++) {
        WaitFrames(1);
        for (playerIndex2 = 0; playerIndex2 < 4; playerIndex2++) {
            if (characterSelectionModeFlag != 0) {
                characterIndex = InterpolateSine(characterGridPositions[characterSelectPlayers[playerIndex2].characterCursor][0], characterGridPositions[playerIndex2 + 1][0], playerIndex, 10.0f);
                characterIndex2 = InterpolateSine(characterGridPositions[characterSelectPlayers[playerIndex2].characterCursor][1], characterGridPositions[1][1], playerIndex, 10.0f);
            }
            else {
                characterIndex = InterpolateSine(fourPlayerCharacterPositions[characterSelectPlayers[playerIndex2].characterCursor % 4][0], fourPlayerCharacterPositions[playerIndex2][0], playerIndex, 10.0f);
                characterIndex2 = InterpolateSine(fourPlayerCharacterPositions[(characterSelectPlayers[playerIndex2].characterCursor / 4) * 4][1], fourPlayerCharacterPositions[0][1], playerIndex, 10.0f);
            }
            HuSprPosSet(positionValue, playerIndex2 + 0x1E, characterIndex, characterIndex2);
            HuSprPosSet(positionValue, playerIndex2 + 0x22, characterIndex, characterIndex2);
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (characterSelectionModeFlag != 0) {
            HuSprPosSet(positionValue, playerIndex + 0x1E, characterGridPositions[playerIndex + 1][0], characterGridPositions[playerIndex + 1][1]);
            HuSprPosSet(positionValue, playerIndex + 0x22, characterGridPositions[playerIndex + 1][0], characterGridPositions[playerIndex + 1][1]);
            characterSelectPlayers[playerIndex].characterCursor = playerIndex + 1;
            characterSelectPlayers[playerIndex].spriteBaseId = playerIndex;
            characterSelectPlayers[playerIndex].isActive = 0;
        }
        else {
            HuSprPosSet(positionValue, playerIndex + 0x1E, fourPlayerCharacterPositions[playerIndex][0], fourPlayerCharacterPositions[playerIndex][1]);
            HuSprPosSet(positionValue, playerIndex + 0x22, fourPlayerCharacterPositions[playerIndex][0], fourPlayerCharacterPositions[playerIndex][1]);
            characterSelectPlayers[playerIndex].spriteBaseId = playerIndex;
            characterSelectPlayers[playerIndex].characterCursor = playerIndex;
            characterSelectPlayers[playerIndex].isActive = 0;
        }
    }
}

void UpdateCharacterSelectionPlayer(OMOBJ *object, CharacterSelectPlayerWork *playerWork)
{
    float positionValue;
    float positionValue2;
    s32 characterIndex;
    s32 characterIndex2;
    s32 playerIndex;
    s32 characterSelectionValue;
    s32 characterSelectionValue2;
    s32 characterIndex3;
    s32 characterIndex4;
    s32 characterSelectionValue3;
    s32 characterSelectionValue4;
    s32 characterIndex5;
    s32 characterIndex6;
    s32 characterIndex7;

    characterIndex4 = characterSelectSpriteGroups[1];
    characterSelectionValue = 0;
    characterSelectionValue2 = 0;
    HuSprAttrReset(characterIndex4, playerWork->spriteBaseId + 0x1E, 4);
    HuSprAttrReset(characterIndex4, playerWork->spriteBaseId + 0x22, 4);
    if (characterSelectionModeFlag != 0) {
        characterIndex = playerWork->characterCursor % 6;
        characterSelectionValue3 = characterIndex;
        characterIndex2 = playerWork->characterCursor / 6;
        characterSelectionValue4 = characterIndex2;
    }
    else {
        characterIndex = playerWork->characterCursor % 4;
        characterSelectionValue3 = characterIndex;
        characterIndex2 = playerWork->characterCursor / 4;
        characterSelectionValue4 = characterIndex2;
    }
    if (playerWork->isActive == 0) {
        if (HuPadBtnDown[playerWork->spriteBaseId] & PAD_BUTTON_A) {
            playerWork->isActive = 1;
            if (characterSelectionModeFlag != 0) {
                characterIndex5 = 0;
                if (playerWork->characterCursor <= 5) {
                    characterIndex5 = playerWork->characterCursor - 1;
                }
                else if (playerWork->characterCursor <= 0xB) {
                    characterIndex5 = playerWork->characterCursor - 3;
                }
                else {
                    characterIndex5 = playerWork->characterCursor - 4;
                }
                PlayCharacterSelectionSound(characterIndex5);
                HuSprBankSet(characterIndex4, characterIndex5 + 2, 1);
            }
            else {
                PlayCharacterSelectionSound(playerWork->characterCursor);
                HuSprBankSet(characterIndex4, playerWork->characterCursor + 2, 1);
            }
            HuSprBankSet(characterIndex4, playerWork->spriteBaseId + 0x1E, 1);
            HuSprBankSet(characterIndex4, playerWork->spriteBaseId + 0x22, 1);
        }
        else if (characterSelectionModeFlag != 0) {
            if (HuPadStkX[playerWork->spriteBaseId] >= 5) {
                characterIndex++;
                characterSelectionValue = 1;
                if (characterIndex > 5) {
                    characterIndex = 5;
                }
            }
            else if (HuPadStkX[playerWork->spriteBaseId] <= -5) {
                characterIndex--;
                characterSelectionValue = -1;
                if (characterIndex < 0) {
                    characterIndex = 0;
                }
            }
            if (HuPadStkY[playerWork->spriteBaseId] <= -5) {
                characterIndex2++;
                characterSelectionValue2 = 1;
                if (characterIndex2 > 2) {
                    characterIndex2 = 2;
                }
            }
            else if (HuPadStkY[playerWork->spriteBaseId] >= 5) {
                characterIndex2--;
                characterSelectionValue2 = -1;
                if (characterIndex2 < 0) {
                    characterIndex2 = 0;
                }
            }
        }
        else {
            if (HuPadStkX[playerWork->spriteBaseId] >= 5) {
                characterIndex++;
                characterSelectionValue = 1;
                if (characterIndex > 3) {
                    characterIndex = 3;
                }
            }
            else if (HuPadStkX[playerWork->spriteBaseId] <= -5) {
                characterIndex--;
                characterSelectionValue = -1;
                if (characterIndex < 0) {
                    characterIndex = 0;
                }
            }
            if (HuPadStkY[playerWork->spriteBaseId] <= -5) {
                characterIndex2++;
                characterSelectionValue2 = 1;
                if (characterIndex2 > 1) {
                    characterIndex2 = 1;
                }
            }
            else if (HuPadStkY[playerWork->spriteBaseId] >= 5) {
                characterIndex2--;
                characterSelectionValue2 = -1;
                if (characterIndex2 < 0) {
                    characterIndex2 = 0;
                }
            }
        }
    }
    else if (HuPadBtnDown[playerWork->spriteBaseId] & PAD_BUTTON_B) {
        playerWork->isActive = 0;
        characterSelectSummaryWork.playerState = 1;
        HuAudFXPlay(3);
        if (characterSelectionModeFlag != 0) {
            characterIndex6 = 0;
            if (playerWork->characterCursor <= 5) {
                characterIndex6 = playerWork->characterCursor - 1;
            }
            else if (playerWork->characterCursor <= 0xB) {
                characterIndex6 = playerWork->characterCursor - 3;
            }
            else {
                characterIndex6 = playerWork->characterCursor - 4;
            }
            HuSprBankSet(characterIndex4, characterIndex6 + 2, 0);
        }
        else {
            HuSprBankSet(characterIndex4, playerWork->characterCursor + 2, 0);
        }
        HuSprBankSet(characterIndex4, playerWork->spriteBaseId + 0x1E, 0);
        HuSprBankSet(characterIndex4, playerWork->spriteBaseId + 0x22, 0);
    }
    if (characterSelectionModeFlag != 0) {
        characterIndex3 = characterIndex + (characterIndex2 * 6);
    }
    else {
        characterIndex3 = characterIndex + (characterIndex2 * 4);
    }
    if (playerWork->characterCursor != characterIndex3) {
        if (characterSelectionModeFlag != 0) {
            while (TRUE) {
                characterIndex3 = characterIndex + (characterIndex2 * 6);
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if (((playerIndex != playerWork->spriteBaseId) && (characterSelectPlayers[playerIndex].selectedCharacter == 0) && (characterIndex3 == characterSelectPlayers[playerIndex].characterCursor))
                        || (characterIndex3 == 0) || (characterIndex3 == 5) || (characterIndex3 == 6) || (characterIndex3 == 0xB)) {
                        if ((characterSelectionValue == 0) || (characterSelectionValue2 == 0)) {
                            if (((characterIndex + characterSelectionValue) > 5) || ((characterIndex + characterSelectionValue) < 0)) {
                                characterIndex = characterSelectionValue3;
                            }
                            else {
                                characterIndex += characterSelectionValue;
                            }
                            if (((characterIndex2 + characterSelectionValue2) > 2) || ((characterIndex2 + characterSelectionValue2) < 0)) {
                                characterIndex2 = characterSelectionValue4;
                            }
                            else {
                                characterIndex2 += characterSelectionValue2;
                            }
                        }
                        else if (((characterIndex + characterSelectionValue) <= 5) && ((characterIndex + characterSelectionValue) >= 0)) {
                            characterIndex += characterSelectionValue;
                        }
                        else if (((characterIndex2 + characterSelectionValue2) <= 2) && ((characterIndex2 + characterSelectionValue2) >= 0)) {
                            characterIndex2 += characterSelectionValue2;
                        }
                        else {
                            characterIndex = characterSelectionValue3;
                            characterIndex2 = characterSelectionValue4;
                        }
                        break;
                    }
                }
                if (playerIndex == 4) {
                    break;
                }
            }
        }
        else {
            do {
                characterIndex3 = characterIndex + (characterIndex2 * 4);
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if ((playerIndex != playerWork->spriteBaseId) && (characterSelectPlayers[playerIndex].selectedCharacter == 0) && (characterIndex3 == characterSelectPlayers[playerIndex].characterCursor)) {
                        if ((characterSelectionValue == 0) || (characterSelectionValue2 == 0)) {
                            if (((characterIndex + characterSelectionValue) > 3) || ((characterIndex + characterSelectionValue) < 0)) {
                                characterIndex = characterSelectionValue3;
                            }
                            else {
                                characterIndex += characterSelectionValue;
                            }
                            if (((characterIndex2 + characterSelectionValue2) > 1) || ((characterIndex2 + characterSelectionValue2) < 0)) {
                                characterIndex2 = characterSelectionValue4;
                            }
                            else {
                                characterIndex2 += characterSelectionValue2;
                            }
                        }
                        else if (((characterIndex + characterSelectionValue) <= 3) && ((characterIndex + characterSelectionValue) >= 0)) {
                            characterIndex += characterSelectionValue;
                        }
                        else if (((characterIndex2 + characterSelectionValue2) <= 1) && ((characterIndex2 + characterSelectionValue2) >= 0)) {
                            characterIndex2 += characterSelectionValue2;
                        }
                        else {
                            characterIndex = characterSelectionValue3;
                            characterIndex2 = characterSelectionValue4;
                        }
                        break;
                    }
                }
            } while (playerIndex != 4);
        }
        if (characterSelectionModeFlag != 0) {
            characterIndex3 = characterIndex + (characterIndex2 * 6);
        }
        else {
            characterIndex3 = characterIndex + (characterIndex2 * 4);
        }
        if (playerWork->characterCursor != characterIndex3) {
            HuAudFXPlay(0);
            characterIndex7 = playerWork->characterCursor;
            playerWork->characterCursor = characterIndex3;
            if (characterSelectionModeFlag != 0) {
                for (playerIndex = 0; playerIndex <= 0xA; playerIndex++) {
                    WaitFrames(1);
                    positionValue = InterpolateSine(characterGridPositions[characterIndex7][0], characterGridPositions[characterIndex3][0], playerIndex, 10.0f);
                    positionValue2 = InterpolateSine(characterGridPositions[characterIndex7][1], characterGridPositions[characterIndex3][1], playerIndex, 10.0f);
                    HuSprPosSet(characterIndex4, playerWork->spriteBaseId + 0x1E, positionValue, positionValue2);
                    HuSprPosSet(characterIndex4, playerWork->spriteBaseId + 0x22, positionValue, positionValue2);
                }
                return;
            }
            for (playerIndex = 0; playerIndex <= 0xA; playerIndex++) {
                WaitFrames(1);
                positionValue = InterpolateSine(*fourPlayerCharacterPositions[characterSelectionValue3], *fourPlayerCharacterPositions[characterIndex], playerIndex, 10.0f);
                positionValue2 = InterpolateSine(fourPlayerCharacterPositions[characterSelectionValue4 * 4][1], fourPlayerCharacterPositions[characterIndex2 * 4][1], playerIndex, 10.0f);
                HuSprPosSet(characterIndex4, playerWork->spriteBaseId + 0x1E, positionValue, positionValue2);
                HuSprPosSet(characterIndex4, playerWork->spriteBaseId + 0x22, positionValue, positionValue2);
            }
        }
    }
}

void AdvanceOverlappingCharacterCursor(CharacterSelectPlayerWork *playerWork)
{
    s32 characterIndex;
    s32 playerIndex;
    s32 characterIndex2;

    characterIndex = characterSelectSpriteGroups[1];
    do {
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if ((playerIndex != playerWork->spriteBaseId) && (characterSelectPlayers[playerIndex].isActive == 1) && (playerWork->characterCursor == characterSelectPlayers[playerIndex].characterCursor)) {
                playerWork->characterCursor += 1;
                break;
            }
        }
    } while (playerIndex != 4);
    playerWork->selectedDifficulty = 0;
    if (characterSelectionModeFlag != 0) {
        characterIndex2 = 0;
        if (playerWork->characterCursor <= 5) {
            characterIndex2 = playerWork->characterCursor - 1;
        }
        else if (playerWork->characterCursor <= 0xB) {
            characterIndex2 = playerWork->characterCursor - 3;
        }
        else {
            characterIndex2 = playerWork->characterCursor - 4;
        }
        HuSprBankSet(characterIndex, characterIndex2 + 2, 0);
    }
    else {
        HuSprBankSet(characterIndex, playerWork->characterCursor + 2, 0);
    }
    HuSprBankSet(characterIndex, playerWork->spriteBaseId + 0x26, 0);
    HuSprBankSet(characterIndex, playerWork->spriteBaseId + 0x2A, 0);
    HuSprBankSet(characterIndex, playerWork->spriteBaseId + 0x2E, playerWork->selectedDifficulty);
    HuSprBankSet(characterIndex, playerWork->spriteBaseId + 0x32, playerWork->selectedDifficulty);
    if (characterSelectionModeFlag != 0) {
        HuSprPosSet(characterIndex, playerWork->spriteBaseId + 0x26, *characterGridPositions[playerWork->characterCursor], characterGridPositions[playerWork->characterCursor][1]);
        HuSprPosSet(characterIndex, playerWork->spriteBaseId + 0x2A, *characterGridPositions[playerWork->characterCursor], characterGridPositions[playerWork->characterCursor][1]);
        HuSprPosSet(characterIndex, playerWork->spriteBaseId + 0x2E, *characterGridPositions[playerWork->characterCursor], characterGridPositions[playerWork->characterCursor][1] + 0x23);
        HuSprPosSet(characterIndex, playerWork->spriteBaseId + 0x32, *characterGridPositions[playerWork->characterCursor], characterGridPositions[playerWork->characterCursor][1] + 0x23);
    }
    else {
        HuSprPosSet(characterIndex, playerWork->spriteBaseId + 0x26, *fourPlayerCharacterPositions[playerWork->characterCursor % 4], fourPlayerCharacterPositions[(playerWork->characterCursor / 4) * 4][1]);
        HuSprPosSet(characterIndex, playerWork->spriteBaseId + 0x2A, *fourPlayerCharacterPositions[playerWork->characterCursor % 4], fourPlayerCharacterPositions[(playerWork->characterCursor / 4) * 4][1]);
        HuSprPosSet(characterIndex, playerWork->spriteBaseId + 0x2E, *fourPlayerCharacterPositions[playerWork->characterCursor % 4], fourPlayerCharacterPositions[(playerWork->characterCursor / 4) * 4][1] + 0x23);
        HuSprPosSet(characterIndex, playerWork->spriteBaseId + 0x32, *fourPlayerCharacterPositions[playerWork->characterCursor % 4], fourPlayerCharacterPositions[(playerWork->characterCursor / 4) * 4][1] + 0x23);
    }
    playerWork->playerState = 0;
    HuSprAttrReset(characterIndex, playerWork->spriteBaseId + 0x26, 4);
    HuSprAttrReset(characterIndex, playerWork->spriteBaseId + 0x2A, 4);
    HuSprAttrSet(characterIndex, playerWork->spriteBaseId + 0x2E, 4);
    HuSprAttrSet(characterIndex, playerWork->spriteBaseId + 0x32, 4);
}

void HidePlayerDifficultyCursor(struct CharacterSelectPlayerWork *playerWork)
{
    s32 characterSelectionValue;

    characterSelectionValue = characterSelectSpriteGroups[1];
    HuSprAttrSet(characterSelectionValue, playerWork->spriteBaseId + 0x26, 4);
    HuSprAttrSet(characterSelectionValue, playerWork->spriteBaseId + 0x2A, 4);
    HuSprAttrSet(characterSelectionValue, playerWork->spriteBaseId + 0x2E, 4);
    HuSprAttrSet(characterSelectionValue, playerWork->spriteBaseId + 0x32, 4);
}

void UpdateDifficultySelectionPlayer(OMOBJ *object, CharacterSelectPlayerWork *playerWork)
{
    s32 difficulty;
    float positionValue;
    float positionValue2;
    s32 characterIndex;
    s32 characterIndex2;
    s32 playerIndex;
    s32 characterIndex3;
    s32 characterSelectionValue;
    s32 characterSelectionValue2;
    s32 characterIndex4;
    s32 characterSelectionValue3;
    s32 characterSelectionValue4;
    s32 characterIndex5;
    s32 characterIndex6;
    s32 characterIndex7;

    characterIndex3 = characterSelectSpriteGroups[1];
    characterSelectionValue = 0;
    characterSelectionValue2 = 0;
    difficulty = playerWork->selectedDifficulty;
    characterSelectSummaryWork.selectionTimer = 0;
    if (characterSelectionModeFlag != 0) {
        characterIndex = playerWork->characterCursor % 6;
        characterSelectionValue3 = characterIndex;
        characterIndex2 = playerWork->characterCursor / 6;
        characterSelectionValue4 = characterIndex2;
    }
    else {
        characterIndex = playerWork->characterCursor % 4;
        characterSelectionValue3 = characterIndex;
        characterIndex2 = playerWork->characterCursor / 4;
        characterSelectionValue4 = characterIndex2;
    }
    if (playerWork->isActive == 0) {
        if (HuPadBtnDown[characterSelectPlayers->controllerPort] & PAD_BUTTON_A) {
            playerWork->isActive = 1;
            HuSprBankSet(characterIndex3, playerWork->spriteBaseId + 0x26, 1);
            HuSprBankSet(characterIndex3, playerWork->spriteBaseId + 0x2A, 1);
            if (characterSelectionModeFlag != 0) {
                characterIndex5 = 0;
                if (playerWork->characterCursor <= 5) {
                    characterIndex5 = playerWork->characterCursor - 1;
                }
                else if (playerWork->characterCursor <= 0xB) {
                    characterIndex5 = playerWork->characterCursor - 3;
                }
                else {
                    characterIndex5 = playerWork->characterCursor - 4;
                }
                HuSprBankSet(characterIndex3, characterIndex5 + 2, 1);
                PlayCharacterSelectionSound(characterIndex5);
            }
            else {
                HuSprBankSet(characterIndex3, playerWork->characterCursor + 2, 1);
                PlayCharacterSelectionSound(playerWork->characterCursor);
            }
            playerWork->selectedDifficulty = 0;
            HuSprBankSet(characterIndex3, playerWork->spriteBaseId + 0x2E, playerWork->selectedDifficulty);
            if (characterSelectionModeFlag != 0) {
                HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x2E, characterGridPositions[playerWork->characterCursor][0], characterGridPositions[playerWork->characterCursor][1] + 0x23);
            }
            else {
                HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x2E, fourPlayerCharacterPositions[playerWork->characterCursor % 4][0], fourPlayerCharacterPositions[(playerWork->characterCursor / 4) * 4][1] + 0x23);
            }
            HuSprAttrReset(characterIndex3, playerWork->spriteBaseId + 0x2E, 4);
        }
        else if (characterSelectionModeFlag != 0) {
            if (HuPadStkX[0] >= 5) {
                characterIndex++;
                characterSelectionValue = 1;
                if (characterIndex > 5) {
                    characterIndex = 5;
                }
            }
            else if (HuPadStkX[0] <= -5) {
                characterIndex--;
                characterSelectionValue = -1;
                if (characterIndex < 0) {
                    characterIndex = 0;
                }
            }
            if (HuPadStkY[0] <= -5) {
                characterIndex2++;
                characterSelectionValue2 = 1;
                if (characterIndex2 > 2) {
                    characterIndex2 = 2;
                }
            }
            else if (HuPadStkY[0] >= 5) {
                characterIndex2--;
                characterSelectionValue2 = -1;
                if (characterIndex2 < 0) {
                    characterIndex2 = 0;
                }
            }
        }
        else {
            if (HuPadStkX[0] >= 5) {
                characterIndex++;
                characterSelectionValue = 1;
                if (characterIndex > 3) {
                    characterIndex = 3;
                }
            }
            else if (HuPadStkX[0] <= -5) {
                characterIndex--;
                characterSelectionValue = -1;
                if (characterIndex < 0) {
                    characterIndex = 0;
                }
            }
            if (HuPadStkY[0] <= -5) {
                characterIndex2++;
                characterSelectionValue2 = 1;
                if (characterIndex2 > 1) {
                    characterIndex2 = 1;
                }
            }
            else if (HuPadStkY[0] >= 5) {
                characterIndex2--;
                characterSelectionValue2 = -1;
                if (characterIndex2 < 0) {
                    characterIndex2 = 0;
                }
            }
        }
    }
    else if (HuPadBtnDown[characterSelectPlayers->controllerPort] & PAD_BUTTON_B) {
        playerWork->isActive = playerWork->playerState = 0;
        if (characterSelectionModeFlag != 0) {
            characterIndex6 = 0;
            if (playerWork->characterCursor <= 5) {
                characterIndex6 = playerWork->characterCursor - 1;
            }
            else if (playerWork->characterCursor <= 0xB) {
                characterIndex6 = playerWork->characterCursor - 3;
            }
            else {
                characterIndex6 = playerWork->characterCursor - 4;
            }
            HuSprBankSet(characterIndex3, characterIndex6 + 2, 0);
        }
        else {
            HuSprBankSet(characterIndex3, playerWork->characterCursor + 2, 0);
        }
        HuSprBankSet(characterIndex3, playerWork->spriteBaseId + 0x26, 0);
        HuSprBankSet(characterIndex3, playerWork->spriteBaseId + 0x2A, 0);
        HuSprAttrSet(characterIndex3, playerWork->spriteBaseId + 0x2E, 4);
        HuAudFXPlay(3);
    }
    if (characterSelectionModeFlag != 0) {
        characterIndex4 = characterIndex + (characterIndex2 * 6);
    }
    else {
        characterIndex4 = characterIndex + (characterIndex2 * 4);
    }
    if (playerWork->characterCursor != characterIndex4) {
        if (characterSelectionModeFlag != 0) {
            while (TRUE) {
                WaitFrames(1);
                characterIndex4 = characterIndex + (characterIndex2 * 6);
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if (((playerIndex != playerWork->spriteBaseId) && (characterSelectPlayers[playerIndex].isActive == 1) && (characterIndex4 == characterSelectPlayers[playerIndex].characterCursor))
                        || (characterIndex4 == 0) || (characterIndex4 == 5) || (characterIndex4 == 6) || (characterIndex4 == 0xB)) {
                        if ((characterSelectionValue == 0) || (characterSelectionValue2 == 0)) {
                            if (((characterIndex + characterSelectionValue) > 5) || ((characterIndex + characterSelectionValue) < 0)) {
                                characterIndex = characterSelectionValue3;
                            }
                            else {
                                characterIndex += characterSelectionValue;
                            }
                            if (((characterIndex2 + characterSelectionValue2) > 2) || ((characterIndex2 + characterSelectionValue2) < 0)) {
                                characterIndex2 = characterSelectionValue4;
                            }
                            else {
                                characterIndex2 += characterSelectionValue2;
                            }
                        }
                        else if (((characterIndex + characterSelectionValue) <= 5) && ((characterIndex + characterSelectionValue) >= 0)) {
                            characterIndex += characterSelectionValue;
                        }
                        else if (((characterIndex2 + characterSelectionValue2) <= 2) && ((characterIndex2 + characterSelectionValue2) >= 0)) {
                            characterIndex2 += characterSelectionValue2;
                        }
                        else {
                            characterIndex = characterSelectionValue3;
                            characterIndex2 = characterSelectionValue4;
                        }
                        break;
                    }
                }
                if (playerIndex == 4) {
                    break;
                }
            }
        }
        else {
            do {
                characterIndex4 = characterIndex + (characterIndex2 * 4);
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if ((playerIndex != playerWork->spriteBaseId) && (characterSelectPlayers[playerIndex].isActive == 1) && (characterIndex4 == characterSelectPlayers[playerIndex].characterCursor)) {
                        if ((characterSelectionValue == 0) || (characterSelectionValue2 == 0)) {
                            if (((characterIndex + characterSelectionValue) > 3) || ((characterIndex + characterSelectionValue) < 0)) {
                                characterIndex = characterSelectionValue3;
                            }
                            else {
                                characterIndex += characterSelectionValue;
                            }
                            if (((characterIndex2 + characterSelectionValue2) > 1) || ((characterIndex2 + characterSelectionValue2) < 0)) {
                                characterIndex2 = characterSelectionValue4;
                            }
                            else {
                                characterIndex2 += characterSelectionValue2;
                            }
                        }
                        else if (((characterIndex + characterSelectionValue) <= 3) && ((characterIndex + characterSelectionValue) >= 0)) {
                            characterIndex += characterSelectionValue;
                        }
                        else if (((characterIndex2 + characterSelectionValue2) <= 1) && ((characterIndex2 + characterSelectionValue2) >= 0)) {
                            characterIndex2 += characterSelectionValue2;
                        }
                        else {
                            characterIndex = characterSelectionValue3;
                            characterIndex2 = characterSelectionValue4;
                        }
                        break;
                    }
                }
            } while (playerIndex != 4);
        }
        if (characterSelectionModeFlag != 0) {
            characterIndex4 = characterIndex + (characterIndex2 * 6);
        }
        else {
            characterIndex4 = characterIndex + (characterIndex2 * 4);
        }
        if (playerWork->characterCursor != characterIndex4) {
            characterIndex7 = playerWork->characterCursor;
            playerWork->characterCursor = characterIndex4;
            HuAudFXPlay(0);
            if (characterSelectionModeFlag != 0) {
                for (playerIndex = 0; playerIndex <= 0xA; playerIndex++) {
                    WaitFrames(1);
                    positionValue = InterpolateSine(characterGridPositions[characterIndex7][0], characterGridPositions[characterIndex4][0], playerIndex, 10.0f);
                    positionValue2 = InterpolateSine(characterGridPositions[characterIndex7][1], characterGridPositions[characterIndex4][1], playerIndex, 10.0f);
                    HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x26, positionValue, positionValue2);
                    HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x2A, positionValue, positionValue2);
                    HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x2E, positionValue, 35.0f + positionValue2);
                }
            }
            else {
                for (playerIndex = 0; playerIndex <= 0xA; playerIndex++) {
                    WaitFrames(1);
                    positionValue = InterpolateSine(fourPlayerCharacterPositions[characterSelectionValue3][0], fourPlayerCharacterPositions[characterIndex][0], playerIndex, 10.0f);
                    positionValue2 = InterpolateSine(fourPlayerCharacterPositions[characterSelectionValue4 * 4][1], fourPlayerCharacterPositions[characterIndex2 * 4][1], playerIndex, 10.0f);
                    HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x26, positionValue, positionValue2);
                    HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x2A, positionValue, positionValue2);
                    HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x2E, positionValue, 35.0f + positionValue2);
                }
            }
        }
    }
    characterSelectSummaryWork.selectionTimer = 0;
}

s32 ProcessDifficultySelectionInput(CharacterSelectPlayerWork *playerWork)
{
    float characterIndex;
    s32 characterIndex2;
    s32 characterIndex3;
    s32 characterIndex4;
    s32 characterSelectionValue;
    s32 difficulty;

    characterIndex4 = 0;
    characterIndex3 = characterSelectSpriteGroups[1];
    while (TRUE) {
        WaitFrames(1);
        characterIndex4 = 0;
        difficulty = playerWork->selectedDifficulty;
        if (HuPadStkX[characterSelectPlayers->controllerPort] < 5) {
            if ((HuPadBtn[characterSelectPlayers->controllerPort] & PAD_BUTTON_RIGHT) == 0) {
                // TODO remove gotos
                goto block2;
            }
        }
        if (GWGameStat.veryHardUnlock == 1) {
            playerWork->selectedDifficulty++;
            characterIndex4 = 1;
            if (playerWork->selectedDifficulty > 3) {
                playerWork->selectedDifficulty = 0;
            }
        }
        else {
            playerWork->selectedDifficulty++;
            characterIndex4 = 1;
            if (playerWork->selectedDifficulty > 2) {
                playerWork->selectedDifficulty = 0;
            }
            goto block1;
        block2:
            if (HuPadStkX[characterSelectPlayers->controllerPort] <= -5 || HuPadBtn[characterSelectPlayers->controllerPort] & PAD_BUTTON_LEFT) {
                if (GWGameStat.veryHardUnlock == 1) {
                    playerWork->selectedDifficulty--;
                    characterIndex4 = -1;
                    if (playerWork->selectedDifficulty < 0) {
                        playerWork->selectedDifficulty = 3;
                    }
                }
                else {
                    playerWork->selectedDifficulty--;
                    characterIndex4 = -1;
                    if (playerWork->selectedDifficulty < 0) {
                        playerWork->selectedDifficulty = 2;
                    }
                }
            }
            else if (HuPadBtnDown[0] & PAD_BUTTON_A) {
                characterSelectionValue = 1;
                break;
            }
            else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
                characterSelectionValue = 0;
                HuAudFXPlay(3);
                break;
            }
        }
    block1:
        if (playerWork->selectedDifficulty != difficulty) {
            HuAudFXPlay(0);
            for (characterIndex2 = 0; characterIndex2 <= 5; characterIndex2++) {
                WaitFrames(1);
                if (characterSelectionModeFlag != 0) {
                    characterIndex = InterpolateSine(characterGridPositions[playerWork->characterCursor][0], characterGridPositions[playerWork->characterCursor][0] + (characterIndex4 * 0xA), characterIndex2, 5.0f);
                    HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x2E, characterIndex, characterGridPositions[playerWork->characterCursor][1] + 0x23);
                }
                else {
                    characterIndex = InterpolateSine(fourPlayerCharacterPositions[playerWork->characterCursor % 4][0], fourPlayerCharacterPositions[playerWork->characterCursor % 4][0] + (characterIndex4 * 0xA), characterIndex2, 5.0f);
                    HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x2E, characterIndex, fourPlayerCharacterPositions[(playerWork->characterCursor / 4) * 4][1] + 0x23);
                }
            }
            HuSprBankSet(characterIndex3, playerWork->spriteBaseId + 0x2E, playerWork->selectedDifficulty);
            for (characterIndex2 = 0; characterIndex2 <= 5; characterIndex2++) {
                WaitFrames(1);
                if (characterSelectionModeFlag != 0) {
                    characterIndex = InterpolateSine(characterGridPositions[playerWork->characterCursor][0] + (characterIndex4 * 0xA), characterGridPositions[playerWork->characterCursor][0], characterIndex2, 5.0f);
                    HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x2E, characterIndex, characterGridPositions[playerWork->characterCursor][1] + 0x23);
                }
                else {
                    characterIndex = InterpolateSine(fourPlayerCharacterPositions[playerWork->characterCursor % 4][0] + (characterIndex4 * 0xA), fourPlayerCharacterPositions[playerWork->characterCursor % 4][0], characterIndex2, 5.0f);
                    HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x2E, characterIndex, fourPlayerCharacterPositions[(playerWork->characterCursor / 4) * 4][1] + 0x23);
                }
            }
        }
    }
    if (characterSelectionValue == 1) {
        if (characterSelectionModeFlag != 0) {
            HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x32, characterGridPositions[playerWork->characterCursor][0], characterGridPositions[playerWork->characterCursor][1] + 0x23);
        }
        else {
            HuSprPosSet(characterIndex3, playerWork->spriteBaseId + 0x32, fourPlayerCharacterPositions[playerWork->characterCursor % 4][0], fourPlayerCharacterPositions[(playerWork->characterCursor / 4) * 4][1] + 0x23);
        }
        HuSprBankSet(characterIndex3, playerWork->spriteBaseId + 0x32, playerWork->selectedDifficulty);
        HuSprAttrReset(characterIndex3, playerWork->spriteBaseId + 0x32, 4);
        HuSprAttrSet(characterIndex3, playerWork->spriteBaseId + 0x2E, 4);
    }
    return characterSelectionValue;
}

void CreateCharacterSelectionPanel(void)
{
    s32 spriteIndex;
    s32 spriteId;
    s32 spriteGroupId;
    ANIMDATA *animationData;
    ANIMDATA *animationData2;

    spriteGroupId = HuSprGrpCreate(0x1B);
    characterSelectSpriteGroups[2] = spriteGroupId;
    spriteId = HuSprCreate(HuSprAnimReadFile(0x640067), 0x64, 0);
    HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
    spriteId = HuSprCreate(HuSprAnimReadFile(0x640069), 0x64, 0);
    HuSprGrpMemberSet(spriteGroupId, 2, spriteId);
    for (spriteIndex = 0; spriteIndex < 8; spriteIndex++) {
        spriteId = HuSprCreate(HuSprAnimReadFile(spriteIndex + 0x64006B), 0x5A, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 4, spriteId);
    }
    animationData = HuSprAnimReadFile(0x640079);
    animationData2 = HuSprAnimReadFile(0x64007A);
    for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
        spriteId = HuSprCreate(animationData, 0x50, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0x12, spriteId);
        spriteId = HuSprCreate(animationData2, 0x50, 0);
        HuSprGrpMemberSet(spriteGroupId, spriteIndex + 0x16, spriteId);
    }
    spriteId = HuSprCreate(HuSprAnimReadFile(0x64007B), 0x46, 0);
    HuSprGrpMemberSet(spriteGroupId, 0x1A, spriteId);
    UpdateCharacterSelectionDisplay(0);
    HideCharacterSelectionPanel(0);
}

void UpdateCharacterSelectionDisplay(s32 displayMode)
{
    s32 playerIndex;
    s32 spriteGroupId;
    s32 playerIndex2;
    s32 playerIndex3;
    s32 playerIndex4;
    s32 playerIndex5;
    s32 playerIndex6;
    s32 playerIndex7;
    s32 playerIndex8;
    s32 playerIndex9;

    spriteGroupId = characterSelectSpriteGroups[2];
    for (playerIndex = 0; playerIndex < 0x1B; playerIndex++) {
        HuSprAttrSet(spriteGroupId, playerIndex, 4);
    }
    switch (displayMode) {
        case 0:
            HuSprAttrReset(spriteGroupId, 2, 4);
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                playerIndex3 = GWPlayerCfg[playerIndex].character;
                playerIndex6 = GWPlayerCfg[playerIndex].diff;
                HuSprAttrReset(spriteGroupId, playerIndex3 + 4, 4);
                HuSprPosSet(spriteGroupId, playerIndex3 + 4, fourPlayerDifficultyPositions[playerIndex][0], fourPlayerDifficultyPositions[playerIndex][1]);
                if (GWPlayerCfg[playerIndex].iscom != 0) {
                    HuSprAttrReset(spriteGroupId, playerIndex + 0x16, 4);
                    HuSprBankSet(spriteGroupId, playerIndex + 0x16, playerIndex6);
                    HuSprPosSet(spriteGroupId, playerIndex + 0x16, fourPlayerDifficultyPositions[playerIndex][0], fourPlayerDifficultyPositions[playerIndex][1] + 0x14);
                }
                else {
                    HuSprAttrReset(spriteGroupId, playerIndex + 0x12, 4);
                    HuSprBankSet(spriteGroupId, playerIndex + 0x12, playerIndex);
                    HuSprPosSet(spriteGroupId, playerIndex + 0x12, fourPlayerDifficultyPositions[playerIndex][0], fourPlayerDifficultyPositions[playerIndex][1] + 0x14);
                }
            }
            break;
        case 1:
            HuSprAttrReset(spriteGroupId, 0, 4);
            playerIndex4 = GWPlayerCfg->character;
            playerIndex7 = GWPlayerCfg->diff;
            HuSprAttrReset(spriteGroupId, playerIndex4 + 4, 4);
            HuSprPosSet(spriteGroupId, playerIndex4 + 4, 0.0f, singlePlayerDifficultyOffset[1]);
            if (GWPlayerCfg->iscom != 0) {
                HuSprAttrReset(spriteGroupId, 0x16, 4);
                HuSprBankSet(spriteGroupId, 0x16, playerIndex7);
                HuSprPosSet(spriteGroupId, playerIndex + 0x16, 0.0f, singlePlayerDifficultyOffset[1] + 0x14);
            }
            else {
                HuSprAttrReset(spriteGroupId, 0x12, 4);
                HuSprBankSet(spriteGroupId, 0x12, 0);
                HuSprPosSet(spriteGroupId, 0x12, 0.0f, singlePlayerDifficultyOffset[1] + 0x14);
            }
            break;
        case 2:
            HuSprAttrReset(spriteGroupId, 1, 4);
            for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                playerIndex5 = GWPlayerCfg[playerIndex].character;
                playerIndex8 = GWPlayerCfg[playerIndex].diff;
                HuSprAttrReset(spriteGroupId, playerIndex5 + 4, 4);
                HuSprPosSet(spriteGroupId, playerIndex5 + 4, twoPlayerDifficultyPositions[playerIndex][0], twoPlayerDifficultyPositions[playerIndex][1]);
                if (GWPlayerCfg[playerIndex].iscom != 0) {
                    HuSprAttrReset(spriteGroupId, playerIndex + 0x16, 4);
                    HuSprBankSet(spriteGroupId, playerIndex + 0x16, playerIndex8);
                    HuSprPosSet(spriteGroupId, playerIndex + 0x16, twoPlayerDifficultyPositions[playerIndex][0], twoPlayerDifficultyPositions[playerIndex][1] + 0x19);
                }
                else {
                    HuSprAttrReset(spriteGroupId, playerIndex + 0x12, 4);
                    HuSprBankSet(spriteGroupId, playerIndex + 0x12, playerIndex);
                    HuSprPosSet(spriteGroupId, playerIndex + 0x12, twoPlayerDifficultyPositions[playerIndex][0], twoPlayerDifficultyPositions[playerIndex][1] + 0x19);
                }
            }
            break;
        case 3:
            HuSprAttrReset(spriteGroupId, 1, 4);
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                playerIndex2 = GWPlayerCfg[playerIndex].character;
                playerIndex9 = GWPlayerCfg[playerIndex].diff;
                playerIndex2 = GetDifficultySpriteBank(playerIndex2);
                HuSprAttrReset(spriteGroupId, playerIndex2 + 4, 4);
                HuSprPosSet(spriteGroupId, playerIndex2 + 4, fourPlayerDifficultyIconPositions[playerIndex][0], fourPlayerDifficultyIconPositions[playerIndex][1]);
                if (GWPlayerCfg[playerIndex].iscom != 0) {
                    HuSprAttrReset(spriteGroupId, playerIndex + 0x16, 4);
                    HuSprBankSet(spriteGroupId, playerIndex + 0x16, playerIndex9);
                    HuSprPosSet(spriteGroupId, playerIndex + 0x16, fourPlayerDifficultyIconPositions[playerIndex][0], fourPlayerDifficultyIconPositions[playerIndex][1] + 0x19);
                }
                else {
                    HuSprAttrReset(spriteGroupId, playerIndex + 0x12, 4);
                    HuSprBankSet(spriteGroupId, playerIndex + 0x12, playerIndex);
                    HuSprPosSet(spriteGroupId, playerIndex + 0x12, fourPlayerDifficultyIconPositions[playerIndex][0], fourPlayerDifficultyIconPositions[playerIndex][1] + 0x19);
                }
            }
            HuSprAttrReset(spriteGroupId, 0x1A, 4);
            break;
    }
    HuSprGrpPosSet(spriteGroupId, 112.0f, -428.0f);
}

void HideCharacterSelectionPanel(s32 displayMode)
{
    s32 spriteIndex;
    s32 spriteGroupId;

    spriteGroupId = characterSelectSpriteGroups[2];
    for (spriteIndex = 0; spriteIndex < 0x1B; spriteIndex++) {
        HuSprAttrSet(spriteGroupId, spriteIndex, 4);
    }
    HuSprGrpPosSet(spriteGroupId, 112.0f, -428.0f);
}

void AnimateCharacterSelectionPanelOut(s32 displayMode)
{
    float spriteGroupId;
    s32 frameIndex;
    s32 spriteGroupId2;

    spriteGroupId2 = characterSelectSpriteGroups[2];
    UpdateCharacterSelectionDisplay(displayMode);
    for (frameIndex = 0; frameIndex <= 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0x32) {
            spriteGroupId = InterpolateSine(-500.0f, 10.0f, frameIndex, 50.0f);
        }
        else {
            spriteGroupId = InterpolateCosine(10.0f, 0.0f, frameIndex - 0x32, 10.0f);
        }
        HuSprGrpPosSet(spriteGroupId2, 112.0f, 72.0f + spriteGroupId);
    }
    HuSprGrpPosSet(spriteGroupId2, 112.0f, 72.0f);
}

void AnimateCharacterSelectionPanelOutAlternate(s32 displayMode)
{
    float spriteGroupId;
    s32 frameIndex;
    s32 spriteIndex;
    s32 spriteGroupId2;
    s32 spriteGroupId3;

    spriteGroupId2 = characterSelectSpriteGroups[2];
    for (frameIndex = 0; frameIndex <= 0x3C; frameIndex++) {
        WaitFrames(1);
        if (frameIndex <= 0xA) {
            spriteGroupId = InterpolateSine(0.0f, 10.0f, frameIndex, 10.0f);
        }
        else {
            spriteGroupId = InterpolateCosine(10.0f, -500.0f, frameIndex - 0xA, 50.0f);
        }
        HuSprGrpPosSet(spriteGroupId2, 112.0f, 72.0f + spriteGroupId);
    }
    HuSprGrpPosSet(spriteGroupId2, 112.0f, -428.0f);
    spriteGroupId3 = characterSelectSpriteGroups[2];
    for (spriteIndex = 0; spriteIndex < 0x1B; spriteIndex++) {
        HuSprAttrSet(spriteGroupId3, spriteIndex, 4);
    }
    HuSprGrpPosSet(spriteGroupId3, 112.0f, -428.0f);
}

void RunCharacterSelectionManagerProcess(void)
{
    OMOBJ *object;

    object = NULL;
    object = omAddObjEx(characterSelectProcess, 0x1000, 8, 8, -1, NULL);
    characterSelectSummaryWork.ownerObject = object;
    characterSelectSummaryWork.updateCallback = NULL;
    while (TRUE) {
        HuPrcVSleep();
        if (characterSelectSummaryWork.updateCallback == NULL) {
            continue;
        }
        characterSelectSummaryWork.updateCallback(characterSelectSummaryWork.ownerObject, &characterSelectSummaryWork);
    }
}

void RunCharacterSelectionPlayerProcess(void)
{
    s32 playerIndex;
    OMOBJ *object;

    playerIndex = characterSelectPlayerCounters[0];
    characterSelectPlayerCounters[0] = playerIndex + 1;
    object = NULL;
    object = omAddObjEx(characterSelectProcess, 0x1000, 8, 8, -1, NULL);
    characterSelectPlayers[playerIndex].ownerObject = object;
    characterSelectPlayers[playerIndex].updateCallback = NULL;

    while (TRUE) {
        HuPrcVSleep();
        if (characterSelectPlayers[playerIndex].updateCallback == NULL) {
            continue;
        }
        characterSelectPlayers[playerIndex].updateCallback(characterSelectPlayers[playerIndex].ownerObject, &characterSelectPlayers[playerIndex]);
    }
}

s32 GetDifficultySpriteBank(s32 difficulty)
{
    s32 difficultyValue;

    switch (difficulty) {
        case 9:
            difficultyValue = 8;
            break;
        case 10:
            difficultyValue = 9;
            break;
        case 11:
            difficultyValue = 0xA;
            break;
        case 12:
            difficultyValue = 0xB;
            break;
        case 13:
            difficultyValue = 0xC;
            break;
        case 8:
            difficultyValue = 0xD;
            break;
        default:
            difficultyValue = -1;
            break;
    }
    return difficultyValue;
}

s32 GetCharacterSpriteBank(s32 characterIndex)
{
    s32 characterSelectionValue;

    switch (characterIndex) {
        case 8:
            characterSelectionValue = 9;
            break;
        case 9:
            characterSelectionValue = 0xA;
            break;
        case 10:
            characterSelectionValue = 0xB;
            break;
        case 11:
            characterSelectionValue = 0xC;
            break;
        case 12:
            characterSelectionValue = 0xD;
            break;
        case 13:
            characterSelectionValue = 8;
            break;
        default:
            characterSelectionValue = -1;
            break;
    }
    return characterSelectionValue;
}

float InterpolateLinear(float startValue, float endValue, float currentFrame, float duration)
{
    if (duration <= currentFrame) {
        return endValue;
    }
    return startValue + ((currentFrame / duration) * (endValue - startValue));
}

float InterpolateByStep(float currentValue, float targetValue, float stepCount)
{
    return (targetValue + (currentValue * (stepCount - 1.0f))) / stepCount;
}

float InterpolateCosine(float startValue, float endValue, float currentFrame, float duration)
{
    if (currentFrame >= duration) {
        return endValue;
    }
    return startValue + ((endValue - startValue) * (1.0 - cosd(((90.0f / duration) * currentFrame))));
}

float InterpolateSine(float startValue, float endValue, float currentFrame, float duration)
{
    if (currentFrame >= duration) {
        return endValue;
    }
    return startValue + ((endValue - startValue) * sind(((90.0f / duration) * currentFrame)));
}

float CalculateSineWaveValue(float startValue, float endValue, float currentFrame, float duration)
{
    if (currentFrame >= duration) {
        return startValue;
    }
    return startValue + ((endValue - startValue) * sind(((360.0f / duration) * currentFrame)));
}

void WaitFrames(s32 frameCount)
{
    if (frameCount <= 1) {
        HuPrcVSleep();
        return;
    }
    HuPrcSleep(frameCount);
}

void InitializeMessageWindows(void)
{
    HuWinInit(1);
}

void CreateCharacterSelectMessageWindow(void)
{
    s32 windowId;
    s32 windowId2;
    s32 windowId3;
    s32 windowId4;

    if (characterSelectMessageWindow == -1) {
        windowId = 0x1C9;
        windowId2 = 0x44;
        windowId3 = 1.0f + ((576.0f - windowId) / 2);
        windowId4 = 445.0f - windowId2;
        characterSelectMessageWindow = HuWinExCreateStyled(windowId3, windowId4, windowId, windowId2, -1, 1);
        winData[characterSelectMessageWindow].active_pad = 1;
        HuWinMesPalSet(characterSelectMessageWindow, 7, 0, 0, 0);
        HuWinExAnimIn(characterSelectMessageWindow);
    }
}

void DestroyCharacterSelectMessageWindow(void)
{
    if (characterSelectMessageWindow != -1) {
        HuWinMesSet(characterSelectMessageWindow, 0x250004);
        HuWinExAnimOut(characterSelectMessageWindow);
        HuWinExCleanup(characterSelectMessageWindow);
        characterSelectMessageWindow = -1;
    }
}

void ShowCharacterSelectMessage(u32 messageId, s32 waitFrames, s32 instant)
{
    if (characterSelectMessageWindow != -1) {
        if (instant == 1) {
            HuWinMesSpeedSet(characterSelectMessageWindow, 0);
        }
        else {
            HuWinMesSpeedSet(characterSelectMessageWindow, 1);
        }
        HuWinMesSet(characterSelectMessageWindow, messageId);
        if (waitFrames <= 0) {
            HuWinMesWait(characterSelectMessageWindow);
            return;
        }
        if (waitFrames <= 1) {
            HuPrcVSleep();
            return;
        }
        HuPrcSleep(waitFrames);
    }
}

void InsertCharacterSelectMessage(u32 messageId, s32 insertIndex)
{
    if (characterSelectMessageWindow != -1) {
        HuWinInsertMesSet(characterSelectMessageWindow, messageId, insertIndex);
    }
}

s32 ShowCharacterSelectChoice(s32 timeoutFrames)
{
    float messageDimensions[2];
    s32 windowId;
    s32 windowId2;
    s32 windowId3;
    s32 windowId4;
    s32 windowId5;
    s32 windowId6;

    HuWinMesMaxSizeGet(1, messageDimensions, 0x1E0035);
    windowId2 = messageDimensions[0];
    windowId3 = messageDimensions[1];
    windowId5 = 0x22C - windowId2;
    windowId6 = 0x1BD - windowId3;
    windowId = HuWinExCreateStyled(windowId5, windowId6, windowId2, windowId3, -1, 1);
    winData[windowId].active_pad = 1;
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    HuWinExAnimIn(windowId);
    HuWinMesSpeedSet(windowId, 0);
    HuWinMesSet(windowId, 0x1E0035);
    windowId4 = HuWinChoiceGet(windowId, timeoutFrames);
    HuWinMesSet(windowId, 0x250004);
    HuWinExAnimOut(windowId);
    HuWinExCleanup(windowId);
    return windowId4;
}

void ShowCharacterSelectWindowMessage(u32 messageId, s32 windowPlacement)
{
    float messageDimensions[2];
    s32 windowId;
    s32 windowId2;
    s32 windowId3;
    s32 windowId4;

    HuWinMesMaxSizeGet(1, messageDimensions, messageId);
    windowId3 = messageDimensions[0];
    windowId = messageDimensions[1];
    windowId4 = (0x240 - windowId3) / 2.0f;
    if (windowPlacement == 1) {
        windowId2 = 0x1BD - windowId;
    }
    else {
        windowId2 = 0x181 - windowId;
    }
    if (characterSelectTextWindows[0] != -1) {
        DestroyCharacterSelectTextWindow();
        characterSelectTextWindows[0] = HuWinExCreateStyled(windowId4, windowId2, windowId3, windowId, -1, 1);
    }
    else {
        characterSelectTextWindows[0] = HuWinExCreateStyled(windowId4, windowId2, windowId3, windowId, -1, 1);
    }
    HuWinBGTPLvlSet(characterSelectTextWindows[0], 0.0f);
    HuWinDispOn(characterSelectTextWindows[0]);
    HuWinMesSet(characterSelectTextWindows[0], messageId);
    HuWinMesSpeedSet(characterSelectTextWindows[0], 0);
}

void ShowCharacterSelectTopMessage(u32 messageId)
{
    float messageDimensions[2];
    s32 windowId;
    s32 windowId2;
    s32 windowId3;
    s32 windowId4;

    HuWinMesMaxSizeGet(1, messageDimensions, messageId);
    windowId2 = messageDimensions[0];
    windowId = messageDimensions[1];
    windowId4 = (0x240 - windowId2) / 2.0f;
    windowId3 = 0x181 - windowId;
    if (characterSelectTextWindows[0] != -1) {
        DestroyCharacterSelectTextWindow();
        characterSelectTextWindows[0] = HuWinExCreateStyled(windowId4, windowId3, windowId2, windowId, -1, 1);
    }
    else {
        characterSelectTextWindows[0] = HuWinExCreateStyled(windowId4, windowId3, windowId2, windowId, -1, 1);
    }
    HuWinBGTPLvlSet(characterSelectTextWindows[0], 0.0f);
    HuWinDispOn(characterSelectTextWindows[0]);
    HuWinMesSet(characterSelectTextWindows[0], messageId);
    HuWinMesSpeedSet(characterSelectTextWindows[0], 0);
}

void DestroyCharacterSelectTextWindow(void)
{
    if (characterSelectTextWindows[0] != -1) {
        HuWinExCleanup(characterSelectTextWindows[0]);
        characterSelectTextWindows[0] = -1;
    }
}

void SetPlayerCharacterSprite(void)
{
    s32 characterIndex;
    s32 playerIndex;

    characterIndex = characterSelectSpriteGroups[1];
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (characterSelectPlayers[playerIndex].isActive == 1) {
            HuSprBankSet(characterIndex, characterSelectPlayers[playerIndex].characterCursor + 2, 1);
            HuSprBankSet(characterIndex, characterSelectPlayers[playerIndex].spriteBaseId + 0x1E, 1);
            HuSprBankSet(characterIndex, characterSelectPlayers[playerIndex].spriteBaseId + 0x22, 1);
            HuSprAttrReset(characterIndex, characterSelectPlayers[playerIndex].spriteBaseId + 0x1E, 4);
            HuSprAttrReset(characterIndex, characterSelectPlayers[playerIndex].spriteBaseId + 0x22, 4);
            HuSprPosSet(characterIndex, characterSelectPlayers[playerIndex].spriteBaseId + 0x1E, fourPlayerCharacterPositions[characterSelectPlayers[playerIndex].characterCursor % 4][0],
                fourPlayerCharacterPositions[(characterSelectPlayers[playerIndex].characterCursor / 4) * 4][1]);
            HuSprPosSet(characterIndex, characterSelectPlayers[playerIndex].spriteBaseId + 0x22, fourPlayerCharacterPositions[characterSelectPlayers[playerIndex].characterCursor % 4][0],
                fourPlayerCharacterPositions[(characterSelectPlayers[playerIndex].characterCursor / 4) * 4][1]);
        }
        else {
            characterSelectPlayers[playerIndex].characterCursor = 0;
        }
    }
}

void SetPlayerDifficultySprite(void)
{
    s32 characterIndex;
    s32 playerIndex;

    characterIndex = characterSelectSpriteGroups[1];
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (characterSelectPlayers[playerIndex].isActive == 1) {
            HuSprBankSet(characterIndex, characterSelectPlayers[playerIndex].characterCursor + 2, 1);
            HuSprBankSet(characterIndex, characterSelectPlayers[playerIndex].spriteBaseId + 0x1E, 1);
            HuSprBankSet(characterIndex, characterSelectPlayers[playerIndex].spriteBaseId + 0x22, 1);
            HuSprAttrReset(characterIndex, characterSelectPlayers[playerIndex].spriteBaseId + 0x1E, 4);
            HuSprAttrReset(characterIndex, characterSelectPlayers[playerIndex].spriteBaseId + 0x22, 4);
            HuSprPosSet(characterIndex, characterSelectPlayers[playerIndex].spriteBaseId + 0x1E, fourPlayerCharacterPositions[characterSelectPlayers[playerIndex].characterCursor % 4][0],
                fourPlayerCharacterPositions[(characterSelectPlayers[playerIndex].characterCursor / 4) * 4][1]);
            HuSprPosSet(characterIndex, characterSelectPlayers[playerIndex].spriteBaseId + 0x22, fourPlayerCharacterPositions[characterSelectPlayers[playerIndex].characterCursor % 4][0],
                fourPlayerCharacterPositions[(characterSelectPlayers[playerIndex].characterCursor / 4) * 4][1]);
        }
        else {
            characterSelectPlayers[playerIndex].characterCursor = 0;
        }
    }
}

void SetPlayerOptionSprite(void)
{
    s32 characterIndex;
    s32 characterIndex2;
    s32 playerIndex;

    characterIndex2 = characterSelectSpriteGroups[1];
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (characterSelectPlayers[playerIndex].isActive == 1) {
            if (characterSelectPlayers[playerIndex].characterCursor <= 5) {
                characterIndex = characterSelectPlayers[playerIndex].characterCursor - 1;
            }
            else if (characterSelectPlayers[playerIndex].characterCursor <= 0xB) {
                characterIndex = characterSelectPlayers[playerIndex].characterCursor - 3;
            }
            else {
                characterIndex = characterSelectPlayers[playerIndex].characterCursor - 4;
            }
            HuSprBankSet(characterIndex2, characterIndex + 2, 1);
            HuSprBankSet(characterIndex2, characterSelectPlayers[playerIndex].spriteBaseId + 0x1E, 1);
            HuSprBankSet(characterIndex2, characterSelectPlayers[playerIndex].spriteBaseId + 0x22, 1);
            HuSprAttrReset(characterIndex2, characterSelectPlayers[playerIndex].spriteBaseId + 0x1E, 4);
            HuSprAttrReset(characterIndex2, characterSelectPlayers[playerIndex].spriteBaseId + 0x22, 4);
            HuSprPosSet(characterIndex2, characterSelectPlayers[playerIndex].spriteBaseId + 0x1E, characterGridPositions[characterSelectPlayers[playerIndex].characterCursor][0],
                characterGridPositions[characterSelectPlayers[playerIndex].characterCursor][1]);
            HuSprPosSet(characterIndex2, characterSelectPlayers[playerIndex].spriteBaseId + 0x22, characterGridPositions[characterSelectPlayers[playerIndex].characterCursor][0],
                characterGridPositions[characterSelectPlayers[playerIndex].characterCursor][1]);
        }
        else {
            characterSelectPlayers[playerIndex].characterCursor = 1;
        }
    }
}

void PlayCharacterSelectionSound(s32 characterIndex)
{
    s32 characterSelectionValue[] = { 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x39, 0x40, 0x4B, 0x46, 0x3E, 0x3A };

    if ((characterIndex < 0) || (characterIndex > 0xD)) {
        return;
    }
    HuAudFXPlay(characterSelectionValue[characterIndex]);
}

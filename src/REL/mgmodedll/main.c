#include "REL/mgmodedll.h"
#include "game/audio.h"
#include "game/wipe.h"

#include "ext_math.h"

extern s32 rand8(void);

typedef struct {
    /* 0x00 */ OMOBJ *object;
    /* 0x04 */ void (*updateCallback)(OMOBJ *, ...);
    /* 0x08 */ s32 unreferencedPlayerStates[4];
    /* 0x18 */ char unreferencedPadding[4];
    /* 0x1C */ s32 selectionState[4];
    /* 0x2C */ char unreferencedControlPadding[4];
    /* 0x30 */ s32 menuSpriteGroupId;
    /* 0x34 */ s32 statusSpriteGroupId;
    /* 0x38 */ char reservedPadding[0xC];
    /* 0x44 */ s32 statusWindowIds[2];
} MinigameMenuWork; // Size unknown

typedef struct {
    /* 0x00 */ char unreferencedFunctionState[8];
    /* 0x08 */ s32 inputLocked;
    /* 0x0C */ char unreferencedInputPadding[0x10];
    /* 0x1C */ s32 selectedOption;
    /* 0x20 */ char unreferencedOptionPadding[0x10];
    /* 0x30 */ s32 optionSpriteGroupId;
} OptionMenuWork; // Size unknown

typedef struct {
    /* 0x00 */ s32 animationFrame;
    /* 0x04 */ s32 animationDuration;
    /* 0x08 */ s32 isActive;
    /* 0x0C */ float startPositionX;
    /* 0x10 */ float startPositionY;
    /* 0x14 */ float startPositionZ;
    /* 0x18 */ float targetPositionX;
    /* 0x1C */ float targetPositionY;
    /* 0x20 */ float targetPositionZ;
    /* 0x24 */ float horizontalOffset;
    /* 0x28 */ float unreferencedMotionScalar;
    /* 0x2C */ float unreferencedMotionScale;
} ModelTransitionWork; // Size 0x30

void PrintMinigameModeDebugInfo(MinigameSharedState *sharedState);
void ResetMinigamePlayerSelection(void);
void RunMinigamePlayerObject(void);
void RunMinigameMenuObject(void);
void UpdateMinigameMenuInput(OMOBJ *object);
void CreateMinigameMenuSprites(void);
void ShowMinigameMenuSprites(void);
void HideMinigameMenuSprites(void);
void ShowPlayerCountArrows(void);
void HidePlayerCountArrows(void);
void CreatePlayerStatusSprites(MinigameMenuWork *menuWork, s32 playerStateIndex, s32 selectionIndex, s32 groupIndex);
void SetMinigameMenuMode(s32 modeType);
void SlideMinigameMenuOnscreen(void);
void SlideMinigameMenuOffscreen(void);
void CreateMinigameSelectionSprites(MinigameMenuWork *menuWork, s32 spriteIndex, s32 x, s32 y);
void ResetMinigameSelectionSprites(void);
void UpdatePlayerSelectionCursor(OMOBJ *object, MinigamePlayerState *playerState);
void UpdatePlayerSelectionDisplay(MinigamePlayerState *playerState);
s32 GetPlayerSelectionResult(MinigamePlayerState *playerState);
void UpdateComputerPlayerSelection(s32 playerIndex, MinigamePlayerState *playerState);
void ApplyPlayerSelectionResult(MinigamePlayerState *playerState);
void UpdateDifficultySelection(MinigamePlayerState *playerState);
void ShowDifficultySelection(void);
void HideDifficultySelection(void);
void UpdateOptionSelection(s32 value, OptionMenuWork *secondaryValue);
void UpdatePlayerStatusDisplay(OMOBJ *object);
void RefreshMinigameMenuDisplay(void);
void SetPlayerStatusDisplayMode(s32 playerIndex);
void CreateModeSelectionWindow(void);
s32 GetModeSelectionChoice(void);
void ShowModeSelectionMessage(s32 windowId);
void ShowPlayerSelectionMessage(s32 playerIndex);
s32 SelectFreePlayPlayers(void);
s32 SelectTeamPlayPlayers(void);
s32 SelectBattlePlayers(void);
s32 SelectPlayersForResume(s32 playerIndex);
void ClosePlayerSelectionWindows(void);
void UpdatePlayerMenuObject(OMOBJ *object);
void UpdateMenuCameraObject(OMOBJ *object);
float InterpolateCubicValue(float startValue, float targetValue, float frame, float duration);
float ApproachValue(float startValue, float targetValue, float frame);
float InterpolateValue(float startValue, float targetValue, float frame, float duration);
float InterpolateValueClamped(float startValue, float targetValue, float frame, float duration);
float InterpolateSmoothValue(float startValue, float targetValue, float frame, float duration);
void WaitForMenuFrames(s32 value);
void UpdateMenuCamera(void);
void ResetMenuCamera(void);
void InitializeMenuCamera(void);
void SetMenuCameraEnabled(s32 value);
void InitializeMenuModels(void);
void AnimateWindowPosition(s32 windowId, float targetValue, float frame, s32 waitFrames);
s32 CreateMessageWindow(s32 windowId, s32 messageId, s32 choiceIndex, s32 waitFrames, float progress, float speed, s32 insertId, s32 insertId8, s32 insertId9);
void CloseMessageWindow(s32 windowId, float targetValue, float frame);
void WaitForMessageWindow(s32 windowId, s32 messageId);
void WaitForMessageChoice(s32 windowId, s32 messageId, s32 choiceIndex, s32 waitFrames);
s32 GetMessageChoice(s32 windowId);
void SetMessageInsert(s32 windowId, s32 messageId, s32 choiceIndex);
void SetModelMotionWithMode(s32 modeType, s32 selection, s32 optionIndex, s32 eventType);
void AnimateModelAlongPath(s32 modeType, Vec targetPosition, float frame, float duration, float progress, s32 state, s32 state7);
void AnimateSpritePosition(s32 spriteGroupId, s32 spriteIndex, float frame, float duration);

char reservedMenuWorkPadding[8]; // Could be part of MinigameMenuWork.
MinigameMenuWork minigameMenuWork;
MinigamePlayerState minigamePlayerStates[4];
OMOBJ *minigameMenuObject;
ModelTransitionWork modelTransitionWork[140];
s32 minigamePlayerObjectCount;
MinigameSharedState *minigameSharedState;
HUPROCESS *minigameObjectManager;

s32 unreferencedInitialValue = 1; // Unused.

void InitializeMinigameMenuObjects(HUPROCESS *processManager, MinigameSharedState *sharedState)
{
    s32 i;

    minigameObjectManager = processManager;
    minigameSharedState = sharedState;
    InitializeMenuModels();
    InitializeMenuCamera();
    SetMenuCameraEnabled(1);
    Center.x = 0.0f;
    Center.y = 0.0f;
    Center.z = 0.0f;
    CRot.x = -10.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    CZoom = 2900.0f;
    Hu3DModelPosSet(minigameSharedState->modeModelId, 0.0f, 0.0f, 460.0f);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0, 1);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0, 2);
    minigameMenuObject = omAddObjEx(minigameObjectManager, 0x1000, 141, 16, -1, UpdateMenuCameraObject);
    HuPrcChildCreate(RunMinigameMenuObject, 100, 0x2000, 0, HuPrcCurrentGet());
    for (i = 0; i < 4; i++) {
        HuPrcChildCreate(RunMinigamePlayerObject, 100, 0x2000, 0, HuPrcCurrentGet());
    }
}

void PlayMinigameModeOpening(void)
{
    Vec cameraTargetPosition = { 0.0f, 0.0f, 640.0f };
    float zoomTarget;
    s32 messageWindowId;
    s32 i;

    UpdateMenuCamera();
    for (i = 1; i < 140; i++) {
        Hu3DModelAttrReset(minigameMenuObject->mdlId[i], HU3D_ATTR_DISPOFF);
    }
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    for (i = 0; i < 240; i++) {
        UpdateMenuCamera();
        zoomTarget = InterpolateValueClamped(2900.0f, 1150.0f, frame, 180.0f);
        CZoom = ApproachValue(startValue, targetValue, 15.0f);
        if (i == 90) {
            SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0, 0);
        }
    }
    minigameMenuObject->work[0] = 1;
    zoomTarget = CZoom;
    for (i = 0; i < 90; i++) {
        UpdateMenuCamera();
        CZoom = ApproachValue(startValue, 1600.0f, frame, 60.0f), 10.0f);
        Center.y = ApproachValue(0.0f, 215.0f, frame, 60.0f), 10.0f);
        CRot.x = ApproachValue(-10.0f, 0.0f, frame, 60.0f), 10.0f);
    }
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
    AnimateModelAlongPath(minigameSharedState->modeType, targetPosition, 0.0f, 3.0f, 0.0f, 1, 0);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
    HuPrcSleep(30);
    HuAudSeqPlay(44);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0, 0);
    WaitForMenuFrames(60);
    messageWindowId = CreateStandardMessageWindow(0, 0, 0);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
    HuAudFXPlay(0x43);
    ShowMessageAndWait(windowId, MAKE_MESSID(40, 0), -1, -1);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
    CloseStandardMessageWindow(windowId);
    WaitForMenuFrames(60);
}

s32 RunMinigameModeSelection(s32 modeType)
{
    s32 selectionStatus;
    s32 selectionAction;
    s32 isResumeSelection;
    s32 i;

    selectionStatus = 10;
    selectionAction = 0;
    isResumeSelection = 0;
    if (modeType == 4) {
        isResumeSelection = 1;
        selectionStatus = 0;
        if (minigameSharedState->modeSelectionType == 0 || minigameSharedState->modeSelectionType == 3) {
            for (i = 0; i < 4; i++) {
                if (minigamePlayerStates[i].isComputer == 1) {
                    break;
                }
            }
            if (i != 4) {
                selectionAction = 2;
            }
            else {
                selectionAction = 1;
            }
        }
        else {
            selectionAction = 3;
        }
    }
    else {
        minigameSharedState->modeSelectionType = modeType;
        ResetMinigamePlayerSelection();
    }
    while (TRUE) {
        UpdateMenuCamera();
        switch (selectionStatus) {
            case 10:
                selectionStatus = 0;
                CreateModeSelectionWindow();
                selectionAction = GetModeSelectionChoice();
                ShowModeSelectionMessage(minigameSharedState->windowId);
                if (selectionAction == -99) {
                    selectionStatus = -99;
                }
                break;
            case 0:
                selectionStatus = 1;
                ShowPlayerSelectionMessage(minigameSharedState->playerIndex);
                while (TRUE) {
                    UpdateMenuCamera();
                    switch (selectionAction) {
                        case 1:
                            selectionAction = SelectFreePlayPlayers();
                            break;
                        case 2:
                            selectionAction = SelectTeamPlayPlayers();
                            break;
                        case 3:
                            if (minigameSharedState->modeSelectionType == 0 || minigameSharedState->modeSelectionType == 3) {
                                selectionAction = 5;
                            }
                            else if (minigameSharedState->modeSelectionType == 2) {
                                selectionAction = SelectBattlePlayers();
                            }
                            else {
                                selectionAction = SelectPlayersForResume(playerIndex);
                                isResumeSelection = 0;
                            }
                            break;
                    }
                    if (selectionAction == 0) {
                        selectionStatus = 10;
                        break;
                    }
                    else if (selectionAction == 5) {
                        selectionStatus = 99;
                        break;
                    }
                }
                ClosePlayerSelectionWindows();
                break;
        }
        if (selectionStatus == 99 || selectionStatus == -99) {
            break;
        }
    }
    if (selectionStatus == -99) {
        selectionStatus = 0;
    }
    else {
        for (i = 0; i < 4; i++) {
            minigameSharedState->playerStates[i].team = minigamePlayerStates[i].team;
            minigameSharedState->playerStates[i].isComputer = minigamePlayerStates[i].isComputer;
            minigameSharedState->playerStates[i].difficulty = minigamePlayerStates[i].difficulty;
            minigameSharedState->playerStates[i].character = minigamePlayerStates[i].character;
            minigameSharedState->playerStates[i].controllerPort = minigamePlayerStates[i].controllerPort;
        }
        if (minigameSharedState->modeSelectionType == 1) {
            switch (minigameMenuWork.selectionState[0]) {
                case 1:
                    minigameSharedState->playerStates[0].team = minigameSharedState->playerStates[1].team = 0;
                    minigameSharedState->playerStates[2].team = minigameSharedState->playerStates[3].team = 1;
                    break;
                case 2:
                    minigameSharedState->playerStates[0].team = minigameSharedState->playerStates[2].team = 0;
                    minigameSharedState->playerStates[1].team = minigameSharedState->playerStates[3].team = 1;
                    break;
                case 3:
                    minigameSharedState->playerStates[0].team = minigameSharedState->playerStates[3].team = 0;
                    minigameSharedState->playerStates[1].team = minigameSharedState->playerStates[2].team = 1;
                    break;
                default:
                    OSReport("########## ERROR : TAGMODE TEAMSET\n");
                    minigameSharedState->playerStates[0].team = minigameSharedState->playerStates[1].team = 0;
                    minigameSharedState->playerStates[2].team = minigameSharedState->playerStates[3].team = 1;
                    break;
            }
        }
        else if (minigameSharedState->modeSelectionType == 2) {
            switch (minigameMenuWork.selectionState[0]) {
                case 0:
                    minigameSharedState->battleStarMaximum = 3;
                    break;
                case 1:
                    minigameSharedState->battleStarMaximum = 5;
                    break;
                case 2:
                    minigameSharedState->battleStarMaximum = 7;
                    break;
                default:
                    OSReport("########## ERROR : KNUKIMODE TURNSET\n");
                    minigameSharedState->battleStarMaximum = 3;
                    break;
            }
        }
        selectionStatus = 1;
    }
    return selectionStatus;
}

void PrintMinigameModeDebugInfo(MinigameSharedState *sharedState)
{
    s32 i;

    OSReport("\n\n>>>>>>>>>>>>>>>>>>>> ENTRANCE HELP\n\n");
    switch (sharedState->modeSelectionType) {
        case 0:
            OSReport("MG_MODE : FREE PLAY MODE!!\n");
            break;
        case 1:
            OSReport("MG_MODE : TAG PLAY MODE!!\n");
            break;
        case 2:
            OSReport("MG_MODE : NUKE PLAY MODE!!\n");
            break;
        case 3:
            OSReport("MG_MODE : BINGO PLAY MODE!!\n");
            break;
    }
    if (sharedState->modeSelectionType == 2) {
        OSReport("\nTURN : %d\n", sharedState->battleStarMaximum);
    }
    OSReport("\nNO) GRP:COM:DIF:CHR:PAD\n");
    for (i = 0; i < 4; i++) {
        OSReport("%2d) %3d:%3d:%3d:%3d:%3d\n", i, sharedState->playerStates[i].team, sharedState->playerStates[i].isComputer, sharedState->playerStates[i].difficulty, sharedState->playerStates[i].character,
            sharedState->playerStates[i].controllerPort);
    }
    OSReport("\n>>>>>>>>>>>>>>>>>>>> ENTRANCE HELP\n\n");
}

void ResetMinigamePlayerSelection(void)
{
    s32 i;
    s32 j;

    for (i = 0; i < 4; i++) {
        minigameMenuWork.selectionState[i] = minigameMenuWork.unreferencedPlayerStates[i] = 0;
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            minigamePlayerStates[i].selectionValues[j] = minigamePlayerStates[i].selectionFlags[j] = 0;
        }
        minigamePlayerStates[i].playerIndex = i;
        minigamePlayerStates[i].team = 0;
        minigamePlayerStates[i].isComputer = i;
        if (minigamePlayerStates[i].isComputer > 0) {
            minigamePlayerStates[i].isComputer = 1;
        }
        minigamePlayerStates[i].difficulty = 0;
        minigamePlayerStates[i].character = i;
        minigamePlayerStates[i].controllerPort = i;
    }
}

void RunMinigamePlayerObject(void)
{
    OMOBJ *playerObject;
    s32 playerIndex;

    playerIndex = minigamePlayerObjectCount++;
    playerObject = omAddObjEx(minigameObjectManager, 0x100, 16, 16, -1, NULL);
    minigamePlayerStates[playerIndex].object = playerObject;
    minigamePlayerStates[playerIndex].updateCallback = NULL;
    minigamePlayerStates[playerIndex].playerIndex = playerIndex;
    minigamePlayerStates[playerIndex].team = 0;
    minigamePlayerStates[playerIndex].isComputer = playerIndex;
    if (minigamePlayerStates[playerIndex].isComputer > 0) {
        minigamePlayerStates[playerIndex].isComputer = 1;
    }
    minigamePlayerStates[playerIndex].difficulty = 0;
    minigamePlayerStates[playerIndex].character = playerIndex;
    minigamePlayerStates[playerIndex].controllerPort = playerIndex;
    while (TRUE) {
        UpdateMenuCamera();
        if (minigamePlayerStates[playerIndex].updateCallback != NULL) {
            minigamePlayerStates[playerIndex].updateCallback(playerObject, &minigamePlayerStates[playerIndex]);
        }
    }
}

void RunMinigameMenuObject(void)
{
    OMOBJ *menuObject;

    menuObject = omAddObjEx(minigameObjectManager, 0x100, 16, 16, -1, NULL);
    minigameMenuWork.object = menuObject;
    minigameMenuWork.updateCallback = NULL;
    CreateMinigameSelectionSprites(&menuWork, 0, 0, 0);
    CreatePlayerStatusSprites(&menuWork, 0, 0, 0);
    while (TRUE) {
        UpdateMenuCamera();
        if (minigameMenuWork.updateCallback != NULL) {
            minigameMenuWork.updateCallback(menuObject, &minigameMenuWork);
        }
    }
}

s32 playerStatusSpritePositions[][4] = { { 177, 104, 120, 55 }, { 251, 104, 458, 55 }, { 325, 104, 120, 142 }, { 399, 104, 458, 142 } };

s32 playerSelectionSpritePositions[][6]
    = { { 168, 90, 120, 51, 112, 55 }, { 248, 190, 458, 51, 466, 55 }, { 328, 90, 120, 222, 112, 282 }, { 408, 190, 458, 222, 466, 282 } };

s32 minigameMenuLayoutValues[] = { 224, 456, 272, 292, 468, 282 };

float characterSelectionLayout[][5] = { { 262.0f, 119.0f, 1.3f, 227.0f, 455.0f }, { 314.0f, 168.0f, 0.75f, 265.0f, 416.0f },
    { 366.0f, 204.0f, 0.75f, 230.0f, 450.0f }, { 418.0f, 240.0f, 0.75f, 300.0f, 381.0f }, { -1.0f, 289.0f, 1.3f, 220.0f, 460.0f } };

s32 characterSelectionColumns[][4] = { { 262, 262, 262, 262 }, { 314, 307, 373, 373 }, { 366, 373, 307, 418 }, { 418, 418, 418, 307 } };

s32 sharedMessageWindowId = -1;

void UpdateMinigameMenuInput(OMOBJ *object)
{
    float arrowSlideOffset;
    float playerIconScale;
    s32 statusSpriteGroupId;
    s32 activePlayerCount;
    s32 connectedControllerCount;
    s32 playerCountDelta;
    s32 wrapsToFirstPlayer;
    s32 i;
    s32 j;

    statusSpriteGroupId = minigameMenuWork.statusSpriteGroupId;
    activePlayerCount = connectedControllerCount = playerCountDelta = wrapsToFirstPlayer = 0;
    minigameMenuWork.selectionState[0] = 0;
    activePlayerCount = 1;
    for (i = 1; i < 4; i++) {
        if (minigamePlayerStates[i].isComputer == 0) {
            activePlayerCount++;
        }
    }
    connectedControllerCount = 1;
    for (i = 1; i < 4; i++) {
        if (HuPadStatGet(i) != -1) {
            connectedControllerCount++;
        }
    }
    if (connectedControllerCount >= 2) {
        ShowPlayerCountArrows();
        if (HuPadStkX[minigamePlayerStates->controllerPort] >= 5 || (HuPadBtn[minigamePlayerStates->controllerPort] & 2)) {
            activePlayerCount++;
            playerCountDelta = 1;
        }
        else if (HuPadStkX[minigamePlayerStates->controllerPort] <= -5 || (HuPadBtn[minigamePlayerStates->controllerPort] & 1)) {
            activePlayerCount--;
            playerCountDelta = -1;
        }
    }
    else {
        HidePlayerCountArrows();
    }
    if (activePlayerCount < 1) {
        activePlayerCount = connectedControllerCount;
    }
    else if (activePlayerCount > connectedControllerCount) {
        if (playerCountDelta == 0) {
            activePlayerCount = connectedControllerCount;
            wrapsToFirstPlayer = 1;
        }
        else {
            activePlayerCount = 1;
        }
    }
    if (playerCountDelta != 0 || wrapsToFirstPlayer != 0) {
        minigameMenuWork.selectionState[0] = 1;
        HuAudFXPlay(0);
        for (i = 0; i <= 10; i++) {
            if (i <= 5) {
                arrowSlideOffset = InterpolateValueClamped(0.0f, 10.0f, frame, 5.0f);
                playerIconScale = InterpolateValueClamped(1.0f, 0.0f, frame, 5.0f);
            }
            else {
                arrowSlideOffset = InterpolateValueClamped(10.0f, 0.0f, i - 5, 5.0f);
                playerIconScale = InterpolateValueClamped(0.0f, 1.0f, i - 5, 5.0f);
            }
            if (playerCountDelta == 1) {
                HuSprPosSet(statusSpriteGroupId, 9, playerStatusSpritePositions[3][0] + 50 + arrowSlideOffset, playerStatusSpritePositions[0][1]);
            }
            else if (playerCountDelta == -1) {
                HuSprPosSet(statusSpriteGroupId, 8, playerStatusSpritePositions[0][0] - 50 - arrowSlideOffset, playerStatusSpritePositions[0][1]);
            }
            for (j = 0; j < 4; j++) {
                if (j < activePlayerCount && minigamePlayerStates[j].isComputer == 1) {
                    minigamePlayerStates[j].selectionFlags[3] = 0;
                    HuSprScaleSet(statusSpriteGroupId, j, playerIconScale, 1.0f);
                    HuSprScaleSet(statusSpriteGroupId, j + 4, playerIconScale, 1.0f);
                    if (i == 5) {
                        HuSprBankSet(statusSpriteGroupId, j, 0);
                    }
                }
                else if (j >= activePlayerCount && minigamePlayerStates[j].isComputer == 0) {
                    minigamePlayerStates[j].selectionFlags[3] = 1;
                    HuSprScaleSet(statusSpriteGroupId, j, playerIconScale, 1.0f);
                    HuSprScaleSet(statusSpriteGroupId, j + 4, playerIconScale, 1.0f);
                    if (i == 5) {
                        HuSprBankSet(statusSpriteGroupId, j, 1);
                    }
                }
            }
            UpdateMenuCamera();
        }
        for (j = 0; j < 4; j++) {
            if (j < activePlayerCount) {
                minigamePlayerStates[j].isComputer = 0;
            }
            else {
                minigamePlayerStates[j].isComputer = 1;
            }
        }
    }
    minigameMenuWork.selectionState[0] = 0;
}

void CreateMinigameMenuSprites(void)
{
    s32 statusSpriteGroupId;
    s32 i;

    statusSpriteGroupId = minigameMenuWork.statusSpriteGroupId;
    for (i = 0; i < 19; i++) {
        HuSprAttrSet(statusSpriteGroupId, i, HUSPR_ATTR_DISPOFF);
    }
    for (i = 0; i < 4; i++) {
        HuSprBankSet(statusSpriteGroupId, i, 0);
        minigamePlayerStates[i].selectionFlags[3] = minigamePlayerStates[i].isComputer = 0;
        HuSprAttrReset(statusSpriteGroupId, i, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(statusSpriteGroupId, 4 + i, HUSPR_ATTR_DISPOFF);
    }
    HuSprAttrReset(statusSpriteGroupId, 10, HUSPR_ATTR_DISPOFF);
    for (i = 0; i < 4; i++) {
        HuSprAttrReset(statusSpriteGroupId, 11 + i, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(statusSpriteGroupId, 15 + i, HUSPR_ATTR_DISPOFF);
    }
    HuSprGrpPosSet(statusSpriteGroupId, 0.0f, -500.0f);
}

void ShowMinigameMenuSprites(void)
{
    float verticalPosition;
    s32 statusSpriteGroupId;
    s32 i;

    statusSpriteGroupId = minigameMenuWork.statusSpriteGroupId;
    CreateMinigameMenuSprites();
    for (i = 0; i <= 60; i++) {
        UpdateMenuCamera();
        if (i <= 50) {
            verticalPosition = InterpolateValueClamped(-500.0f, 10.0f, frame, 50.0f);
        }
        else {
            verticalPosition = InterpolateValue(10.0f, 2.06f, i - 50, 10.0f);
        }
        HuSprGrpPosSet(statusSpriteGroupId, 0.0f, verticalPosition);
    }
}

void HideMinigameMenuSprites(void)
{
    float verticalPosition;
    s32 unreferencedAnimationScratch;
    s32 unreferencedFrameScratch;
    s32 statusSpriteGroupId;
    s32 i;

    unreferencedAnimationScratch = 0;
    unreferencedFrameScratch = 0;
    statusSpriteGroupId = minigameMenuWork.statusSpriteGroupId;
    for (i = 0; i <= 60; i++) {
        UpdateMenuCamera();
        if (i <= 10) {
            verticalPosition = InterpolateValueClamped(2.06f, 10.0f, frame, 10.0f);
        }
        else {
            verticalPosition = InterpolateValue(10.0f, -500.0f, i - 10, 50.0f);
        }
        HuSprGrpPosSet(statusSpriteGroupId, 0.0f, verticalPosition);
    }
    SetMinigameMenuMode(0);
}

void ShowPlayerCountArrows(void)
{
    s32 statusSpriteGroupId;

    statusSpriteGroupId = minigameMenuWork.statusSpriteGroupId;
    HuSprAttrReset(statusSpriteGroupId, 8, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(statusSpriteGroupId, 8, playerStatusSpritePositions[0][0] - 50, playerStatusSpritePositions[0][1]);
    HuSprAttrReset(statusSpriteGroupId, 9, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(statusSpriteGroupId, 9, playerStatusSpritePositions[3][0] + 50, playerStatusSpritePositions[0][1]);
}

void HidePlayerCountArrows(void)
{
    s32 statusSpriteGroupId;

    statusSpriteGroupId = minigameMenuWork.statusSpriteGroupId;
    HuSprAttrSet(statusSpriteGroupId, 8, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(statusSpriteGroupId, 8, playerStatusSpritePositions[0][0] - 50, playerStatusSpritePositions[0][1]);
    HuSprAttrSet(statusSpriteGroupId, 9, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(statusSpriteGroupId, 9, playerStatusSpritePositions[3][0] + 50, playerStatusSpritePositions[0][1]);
}

void CreatePlayerStatusSprites(MinigameMenuWork *menuWork, s32 playerStateIndex, s32 selectionIndex, s32 groupIndex)
{
    ANIMDATA *statusOverlayAnimation;
    ANIMDATA *playerShadowAnimation;
    s32 statusSpriteGroupId;
    s32 spriteId;
    s32 i;

    statusSpriteGroupId = HuSprGrpCreate(19);
    menuWork->statusSpriteGroupId = statusSpriteGroupId;
    statusOverlayAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 111));
    for (i = 0; i < 4; i++) {
        spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 99)), 1000, 0);
        HuSprGrpMemberSet(statusSpriteGroupId, i, spriteId);
        HuSprPosSet(statusSpriteGroupId, i, playerStatusSpritePositions[i % 4][0], playerStatusSpritePositions[0][1]);
        spriteId = HuSprCreate(statusOverlayAnimation, 1010, 0);
        HuSprGrpMemberSet(statusSpriteGroupId, 4 + i, spriteId);
        HuSprTPLvlSet(statusSpriteGroupId, 4 + i, 0.5f);
        HuSprPosSet(statusSpriteGroupId, 4 + i, playerStatusSpritePositions[i % 4][0] + 1, playerStatusSpritePositions[0][1] + 2);
    }
    statusOverlayAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 137));
    for (i = 0; i < 2; i++) {
        spriteId = HuSprCreate(statusOverlayAnimation, 0, 0);
        HuSprGrpMemberSet(statusSpriteGroupId, 8 + i, spriteId);
        HuSprBankSet(statusSpriteGroupId, 8 + i, i);
    }
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 98)), 2000, 0);
    HuSprGrpMemberSet(statusSpriteGroupId, 10, spriteId);
    HuSprTPLvlSet(statusSpriteGroupId, 10, 0.8f);
    AnimateSpritePosition(spriteGroupId, 10, 288.0f, 240.0f);
    statusOverlayAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 101));
    playerShadowAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 102));
    for (i = 0; i < 4; i++) {
        spriteId = HuSprCreate(statusOverlayAnimation, 1980, 0);
        HuSprGrpMemberSet(statusSpriteGroupId, 11 + i, spriteId);
        HuSprBankSet(statusSpriteGroupId, 11 + i, i);
        AnimateSpritePosition(spriteGroupId, 11 + spriteIndex, playerStatusSpritePositions[i][2], playerStatusSpritePositions[i][3]);
        spriteId = HuSprCreate(playerShadowAnimation, 1990, 0);
        HuSprGrpMemberSet(statusSpriteGroupId, 15 + i, spriteId);
        HuSprBankSet(statusSpriteGroupId, 15 + i, i);
        HuSprTPLvlSet(statusSpriteGroupId, 15 + i, 0.5f);
        AnimateSpritePosition(spriteGroupId, 15 + spriteIndex, playerStatusSpritePositions[i][2] + 1, playerStatusSpritePositions[i][3] + 6);
    }
    CreateMinigameMenuSprites();
}

void SetMinigameMenuMode(s32 modeType)
{
    s32 menuSpriteGroupId;
    s32 i;

    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    for (i = 0; i < 86; i++) {
        HuSprAttrSet(menuSpriteGroupId, i, HUSPR_ATTR_DISPOFF);
    }
    for (i = 0; i < 4; i++) {
        if (minigamePlayerStates[i].isComputer != 0) {
            HuSprAttrSet(menuSpriteGroupId, 16 + minigamePlayerStates[i].playerIndex, HUSPR_ATTR_DISPOFF);
            HuSprAttrSet(menuSpriteGroupId, 20 + minigamePlayerStates[i].playerIndex, HUSPR_ATTR_DISPOFF);
        }
        else {
            HuSprAttrReset(menuSpriteGroupId, 16 + minigamePlayerStates[i].playerIndex, HUSPR_ATTR_DISPOFF);
            HuSprAttrReset(menuSpriteGroupId, 20 + minigamePlayerStates[i].playerIndex, HUSPR_ATTR_DISPOFF);
        }
        HuSprPosSet(
            menuSpriteGroupId, 16 + minigamePlayerStates[i].playerIndex, playerSelectionSpritePositions[minigamePlayerStates[i].character % 4][0], playerSelectionSpritePositions[minigamePlayerStates[i].character / 4][1]);
        HuSprPosSet(
            menuSpriteGroupId, 20 + minigamePlayerStates[i].playerIndex, playerSelectionSpritePositions[minigamePlayerStates[i].character % 4][0], playerSelectionSpritePositions[minigamePlayerStates[i].character / 4][1]);
    }
    for (i = 0; i < 8; i++) {
        HuSprBankSet(menuSpriteGroupId, i, 0);
        HuSprAttrReset(menuSpriteGroupId, i, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(menuSpriteGroupId, 8 + i, HUSPR_ATTR_DISPOFF);
    }
    HuSprAttrReset(menuSpriteGroupId, 40, HUSPR_ATTR_DISPOFF);
    for (i = 0; i < 4; i++) {
        HuSprBankSet(menuSpriteGroupId, 16 + i, 0);
        HuSprBankSet(menuSpriteGroupId, 20 + i, 0);
        HuSprAttrReset(menuSpriteGroupId, 41 + i, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(menuSpriteGroupId, 45 + i, HUSPR_ATTR_DISPOFF);
        AnimateSpritePosition(spriteGroupId, 41 + spriteIndex, playerSelectionSpritePositions[i][2], playerSelectionSpritePositions[i][3]);
        AnimateSpritePosition(spriteGroupId, 45 + spriteIndex, playerSelectionSpritePositions[i][2], playerSelectionSpritePositions[i][3] + 6);
    }
    HuWinDispOff(minigameMenuWork.statusWindowIds[0]);
    HuWinDispOff(minigameMenuWork.statusWindowIds[1]);
    if (modeType == 1) {
        HuSprAttrSet(menuSpriteGroupId, 40, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(menuSpriteGroupId, 53, HUSPR_ATTR_DISPOFF);
        for (i = 0; i < 4; i++) {
            AnimateSpritePosition(spriteGroupId, 41 + spriteIndex, playerSelectionSpritePositions[i][4], playerSelectionSpritePositions[i][5]);
            AnimateSpritePosition(spriteGroupId, 45 + spriteIndex, playerSelectionSpritePositions[i][4] + 1, playerSelectionSpritePositions[i][5] + 6);
        }
        for (i = 0; i < 4; i++) {
            HuSprAttrReset(menuSpriteGroupId, 81 + i, HUSPR_ATTR_DISPOFF);
            HuSprBankSet(menuSpriteGroupId, 81 + i, minigamePlayerStates[i].isComputer);
            HuSprPosSet(menuSpriteGroupId, 81 + i, characterSelectionLayout[minigamePlayerStates[i].playerIndex][0], minigameMenuLayoutValues[2]);
            HuSprScaleSet(menuSpriteGroupId, 81 + i, 1.0f, 1.0f);
            HuSprAttrReset(menuSpriteGroupId, 65 + i, HUSPR_ATTR_DISPOFF);
            HuSprPosSet(menuSpriteGroupId, 65 + i, characterSelectionLayout[i][0] + 1.0f, minigameMenuLayoutValues[2] + 2);
        }
        HuWinDispOn(minigameMenuWork.statusWindowIds[0]);
    }
    else if (modeType == 2) {
        HuSprAttrSet(menuSpriteGroupId, 40, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(menuSpriteGroupId, 53, HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(menuSpriteGroupId, 56, HUSPR_ATTR_DISPOFF);
        HuSprBankSet(menuSpriteGroupId, 56, 0);
        for (i = 0; i < 4; i++) {
            AnimateSpritePosition(spriteGroupId, 41 + spriteIndex, playerSelectionSpritePositions[i][4], playerSelectionSpritePositions[i][5]);
            AnimateSpritePosition(spriteGroupId, 45 + spriteIndex, playerSelectionSpritePositions[i][4] + 1, playerSelectionSpritePositions[i][5] + 6);
        }
        HuWinDispOn(minigameMenuWork.statusWindowIds[1]);
    }
    HuWinPosSet(minigameMenuWork.statusWindowIds[0], 140.0f, minigameMenuLayoutValues[2] - 519);
    HuWinPosSet(minigameMenuWork.statusWindowIds[1], 100.0f, minigameMenuLayoutValues[2] - 519);
    HuSprPosSet(menuSpriteGroupId, 54, minigameMenuLayoutValues[3], minigameMenuLayoutValues[2]);
    HuSprPosSet(menuSpriteGroupId, 55, minigameMenuLayoutValues[4], minigameMenuLayoutValues[2]);
    HuSprGrpPosSet(menuSpriteGroupId, 0.0f, -500.0f);
}

void SlideMinigameMenuOnscreen(void)
{
    float slidePositionY;
    s32 menuSpriteGroupId;
    s32 i;

    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    for (i = 0; i <= 60; i++) {
        UpdateMenuCamera();
        if (i <= 50) {
            slidePositionY = InterpolateValueClamped(-500.0f, 10.0f, frame, 50.0f);
        }
        else {
            slidePositionY = InterpolateValue(10.0f, 2.06f, i - 50, 10.0f);
        }
        HuSprGrpPosSet(minigameMenuWork.menuSpriteGroupId, 0.0f, slidePositionY);
        HuWinPosSet(minigameMenuWork.statusWindowIds[0], 120.0f, minigameMenuLayoutValues[2] - 19 + slidePositionY);
        HuWinPosSet(minigameMenuWork.statusWindowIds[1], 100.0f, minigameMenuLayoutValues[2] - 19 + slidePositionY);
    }
}

void SlideMinigameMenuOffscreen(void)
{
    float slidePositionY;
    s32 i;

    for (i = 0; i <= 60; i++) {
        UpdateMenuCamera();
        if (i <= 10) {
            slidePositionY = InterpolateValueClamped(2.06f, 10.0f, frame, 10.0f);
        }
        else {
            slidePositionY = InterpolateValue(10.0f, -500.0f, i - 10, 50.0f);
        }
        HuSprGrpPosSet(minigameMenuWork.menuSpriteGroupId, 0.0f, slidePositionY);
        HuWinPosSet(minigameMenuWork.statusWindowIds[0], 120.0f, minigameMenuLayoutValues[2] - 19 + slidePositionY);
        HuWinPosSet(minigameMenuWork.statusWindowIds[1], 100.0f, minigameMenuLayoutValues[2] - 19 + slidePositionY);
    }
}

void CreateMinigameSelectionSprites(MinigameMenuWork *menuWork, s32 spriteIndex, s32 x, s32 y)
{
    ANIMDATA *statusOverlayAnimation;
    ANIMDATA *playerMarkerAnimation;
    ANIMDATA *inactivePlayerMarkerAnimation;
    ANIMDATA *selectedDifficultyAnimation;
    ANIMDATA *playerReadyAnimation;
    s32 menuSpriteGroupId;
    s32 spriteId;
    s32 i;

    menuSpriteGroupId = menuWork->menuSpriteGroupId = HuSprGrpCreate(86);
    statusOverlayAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 111));
    for (i = 0; i < 8; i++) {
        spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 103 + i)), 1000, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, i, spriteId);
        HuSprPosSet(menuSpriteGroupId, i, playerSelectionSpritePositions[i % 4][0], playerSelectionSpritePositions[i / 4][1]);
        spriteId = HuSprCreate(statusOverlayAnimation, 1010, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 8 + i, spriteId);
        HuSprTPLvlSet(menuSpriteGroupId, 8 + i, 0.5f);
        HuSprPosSet(menuSpriteGroupId, 8 + i, playerSelectionSpritePositions[i % 4][0] + 1, (playerSelectionSpritePositions[i / 4][1] + 2));
    }
    for (i = 0; i < 4; i++) {
        spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 112 + i)), 0, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 16 + i, spriteId);
        spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 117 + i)), 10, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 20 + i, spriteId);
    }
    statusOverlayAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 116));
    playerMarkerAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 121));
    inactivePlayerMarkerAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 122));
    selectedDifficultyAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 124));
    playerReadyAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 123));
    for (i = 0; i < 4; i++) {
        spriteId = HuSprCreate(statusOverlayAnimation, 0, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 24 + i, spriteId);
        spriteId = HuSprCreate(playerMarkerAnimation, 30, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 28 + i, spriteId);
        spriteId = HuSprCreate(inactivePlayerMarkerAnimation, 10, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 32 + i, spriteId);
        spriteId = HuSprCreate(selectedDifficultyAnimation, 20, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 36 + i, spriteId);
        HuSprTPLvlSet(menuSpriteGroupId, 36 + i, 0.5f);
        spriteId = HuSprCreate(playerReadyAnimation, 20, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 49 + i, spriteId);
    }
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 100)), 2000, 0);
    HuSprGrpMemberSet(menuSpriteGroupId, 40, spriteId);
    HuSprTPLvlSet(menuSpriteGroupId, 40, 0.8f);
    AnimateSpritePosition(spriteGroupId, 40, 288.0f, 240.0f);
    statusOverlayAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 101));
    playerMarkerAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 102));
    for (i = 0; i < 4; i++) {
        spriteId = HuSprCreate(statusOverlayAnimation, 1980, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 41 + i, spriteId);
        HuSprBankSet(menuSpriteGroupId, 41 + i, i);
        AnimateSpritePosition(spriteGroupId, 41 + spriteIndex, playerSelectionSpritePositions[i][2], playerSelectionSpritePositions[i][3]);
        spriteId = HuSprCreate(playerMarkerAnimation, 1990, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 45 + i, spriteId);
        HuSprBankSet(menuSpriteGroupId, 45 + i, i);
        HuSprTPLvlSet(menuSpriteGroupId, 45 + i, 0.5f);
        AnimateSpritePosition(spriteGroupId, 45 + spriteIndex, playerSelectionSpritePositions[i][2] + 1, playerSelectionSpritePositions[i][3] + 6);
    }
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 125)), 2000, 0);
    HuSprGrpMemberSet(menuSpriteGroupId, 53, spriteId);
    HuSprTPLvlSet(menuSpriteGroupId, 53, 0.8f);
    AnimateSpritePosition(spriteGroupId, 53, 288.0f, 240.0f);
    statusOverlayAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 137));
    for (i = 0; i < 2; i++) {
        spriteId = HuSprCreate(statusOverlayAnimation, 0, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 54 + i, spriteId);
        HuSprBankSet(menuSpriteGroupId, 54 + i, i);
    }
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 139)), 1000, 0);
    HuSprGrpMemberSet(menuSpriteGroupId, 56, spriteId);
    HuSprPosSet(menuSpriteGroupId, 56, 380.0f, minigameMenuLayoutValues[2]);
    for (i = 0; i < 2; i++) {
        menuWork->statusWindowIds[i] = HuWinExCreateStyled(0.0f, 0.0f, 200, 200, -1, 1);
        HuWinBGTPLvlSet(menuWork->statusWindowIds[i], 0.0f);
        HuWinMesSet(menuWork->statusWindowIds[i], MAKE_MESSID(27, 48 + i));
        HuWinMesSpeedSet(menuWork->statusWindowIds[i], 0);
        HuWinDispOff(menuWork->statusWindowIds[i]);
        winData[menuWork->statusWindowIds[i]].mess_shadow_color = 0;
    }
    spriteId = HuSprCreate(HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 138)), 990, 0);
    HuSprGrpMemberSet(menuSpriteGroupId, 85, spriteId);
    inactivePlayerMarkerAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 134));
    for (i = 0; i < 8; i++) {
        statusOverlayAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 126 + i));
        spriteId = HuSprCreate(statusOverlayAnimation, 1000, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 57 + i, spriteId);
        spriteId = HuSprCreate(inactivePlayerMarkerAnimation, 1010, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 65 + i, spriteId);
    }
    statusOverlayAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 135));
    playerMarkerAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 136));
    for (i = 0; i < 4; i++) {
        spriteId = HuSprCreate(statusOverlayAnimation, 990, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 73 + i, spriteId);
        spriteId = HuSprCreate(playerMarkerAnimation, 990, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 77 + i, spriteId);
    }
    statusOverlayAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 140));
    for (i = 0; i < 4; i++) {
        spriteId = HuSprCreate(statusOverlayAnimation, 990, 0);
        HuSprGrpMemberSet(menuSpriteGroupId, 81 + i, spriteId);
    }
    SetMinigameMenuMode(0);
}

void ResetMinigameSelectionSprites(void)
{
    MinigamePlayerState *playerDisplayInfo;
    float playerStartX;
    float playerStartY;
    s32 menuSpriteGroupId;
    s32 i;
    s32 j;

    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    for (i = 0; i < 8; i++) {
        HuSprBankSet(menuSpriteGroupId, i, 0);
    }
    for (i = 0; i < 4; i++) {
        HuSprBankSet(menuSpriteGroupId, 16 + i, 0);
        HuSprBankSet(menuSpriteGroupId, 20 + i, 0);
        HuSprBankSet(menuSpriteGroupId, 24 + i, 0);
        HuSprBankSet(menuSpriteGroupId, 28 + i, 0);
        HuSprBankSet(menuSpriteGroupId, 32 + i, 0);
    }
    for (i = 0; i <= 10; i++) {
        UpdateMenuCamera();
        for (j = 0; j < 4; j++) {
            playerDisplayInfo = &minigamePlayerStates[j];
            playerStartX = InterpolateValueClamped(playerSelectionSpritePositions[playerDisplayInfo->character % 4][0], playerSelectionSpritePositions[playerDisplayInfo->playerIndex % 4][0], frame, 10.0f);
            playerStartY = InterpolateValueClamped(playerSelectionSpritePositions[playerDisplayInfo->character / 4][1], playerSelectionSpritePositions[playerDisplayInfo->playerIndex / 4][1], frame, 10.0f);
            HuSprPosSet(menuSpriteGroupId, 16 + playerDisplayInfo->playerIndex, playerStartX, playerStartY);
            HuSprPosSet(menuSpriteGroupId, 20 + playerDisplayInfo->playerIndex, playerStartX, playerStartY);
        }
    }
    for (i = 0; i < 4; i++) {
        minigamePlayerStates[i].character = minigamePlayerStates[i].playerIndex;
        minigamePlayerStates[i].selectionFlags[0] = 0;
    }
}

void UpdatePlayerSelectionCursor(OMOBJ *object, MinigamePlayerState *playerState)
{
    float playerTargetX;
    float playerTargetY;
    s32 menuSpriteGroupId;
    s32 horizontalMoveDirection;
    s32 verticalMoveDirection;
    s32 playerStartColumn;
    s32 playerStartRow;
    s32 playerColumn;
    s32 playerRow;
    s32 i;

    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    horizontalMoveDirection = 0;
    verticalMoveDirection = 0;
    playerStartColumn = playerColumn = playerState->character % 4;
    playerStartRow = playerRow = playerState->character / 4;
    if (playerState->selectionFlags[0] == 0) {
        if (HuPadBtnDown[playerState->controllerPort] & 0x100) {
            playerState->selectionFlags[0] = 1;
            HuSprBankSet(menuSpriteGroupId, playerState->character, 1);
            HuSprBankSet(menuSpriteGroupId, 16 + playerState->playerIndex, 1);
            HuSprBankSet(menuSpriteGroupId, 20 + playerState->playerIndex, 1);
            HuAudFXPlay(0x6D + playerState->character);
        }
        else {
            if (HuPadStkX[playerState->controllerPort] >= 5 || (HuPadBtn[playerState->controllerPort] & 2)) {
                playerColumn++;
                horizontalMoveDirection = 1;
                if (playerColumn > 3) {
                    playerColumn = 3;
                }
            }
            else if (HuPadStkX[playerState->controllerPort] <= -5 || (HuPadBtn[playerState->controllerPort] & 1)) {
                playerColumn--;
                horizontalMoveDirection = -1;
                if (playerColumn < 0) {
                    playerColumn = 0;
                }
            }
            if (HuPadStkY[playerState->controllerPort] <= -5 || (HuPadBtn[playerState->controllerPort] & 4)) {
                playerRow++;
                verticalMoveDirection = 1;
                if (playerRow > 1) {
                    playerRow = 1;
                }
            }
            else if (HuPadStkY[playerState->controllerPort] >= 5 || (HuPadBtn[playerState->controllerPort] & 8)) {
                playerRow--;
                verticalMoveDirection = -1;
                if (playerRow < 0) {
                    playerRow = 0;
                }
            }
        }
    }
    else if (HuPadBtnDown[playerState->controllerPort] & 0x200) {
        playerState->selectionFlags[0] = 0;
        HuSprBankSet(menuSpriteGroupId, playerState->character, 0);
        HuSprBankSet(menuSpriteGroupId, 16 + playerState->playerIndex, 0);
        HuSprBankSet(menuSpriteGroupId, 20 + playerState->playerIndex, 0);
        HuAudFXPlay(3);
    }
    if (playerState->character != playerColumn + playerRow * 4) {
        do {
            for (i = 0; i < 4; i++) {
                if (i != playerState->playerIndex && minigamePlayerStates[i].isComputer == 0 && playerColumn + playerRow * 4 == minigamePlayerStates[i].character) {
                    if (horizontalMoveDirection == 0 || verticalMoveDirection == 0) {
                        if (playerColumn + horizontalMoveDirection > 3 || playerColumn + horizontalMoveDirection < 0) {
                            playerColumn = playerStartColumn;
                        }
                        else {
                            playerColumn += horizontalMoveDirection;
                        }
                        if (playerRow + verticalMoveDirection > 1 || playerRow + verticalMoveDirection < 0) {
                            playerRow = playerStartRow;
                        }
                        else {
                            playerRow += verticalMoveDirection;
                        }
                    }
                    else {
                        if (playerColumn + horizontalMoveDirection <= 3 && playerColumn + horizontalMoveDirection >= 0) {
                            playerColumn += horizontalMoveDirection;
                        }
                        else if (playerRow + verticalMoveDirection <= 1 && playerRow + verticalMoveDirection >= 0) {
                            playerRow += verticalMoveDirection;
                        }
                        else {
                            playerColumn = playerStartColumn;
                            playerRow = playerStartRow;
                        }
                    }
                    break;
                }
            }
        } while (i != 4);
        if (playerState->character != playerColumn + playerRow * 4) {
            playerState->character = playerColumn + playerRow * 4;
            HuAudFXPlay(0);
            for (i = 0; i <= 10; i++) {
                UpdateMenuCamera();
                playerTargetX = InterpolateValueClamped(playerSelectionSpritePositions[playerStartColumn][0], playerSelectionSpritePositions[playerColumn][0], frame, 10.0f);
                playerTargetY = InterpolateValueClamped(playerSelectionSpritePositions[playerStartRow][1], playerSelectionSpritePositions[playerRow][1], frame, 10.0f);
                HuSprPosSet(menuSpriteGroupId, 16 + playerState->playerIndex, playerTargetX, playerTargetY);
                HuSprPosSet(menuSpriteGroupId, 20 + playerState->playerIndex, playerTargetX, playerTargetY);
            }
        }
    }
}

void UpdatePlayerSelectionDisplay(MinigamePlayerState *playerState)
{
    s32 menuSpriteGroupId;

    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    playerState->difficulty = 0;
    HuSprBankSet(menuSpriteGroupId, 32 + playerState->playerIndex, playerState->difficulty);
    HuSprPosSet(menuSpriteGroupId, 32 + playerState->playerIndex, playerSelectionSpritePositions[playerState->character % 4][0], playerSelectionSpritePositions[playerState->character / 4][1] + 35);
    HuSprPosSet(menuSpriteGroupId, 36 + playerState->playerIndex, playerSelectionSpritePositions[playerState->character % 4][0] + 1, playerSelectionSpritePositions[playerState->character / 4][1] + 37);
    HuSprAttrReset(menuSpriteGroupId, 32 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
    HuSprAttrReset(menuSpriteGroupId, 36 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
}

s32 GetPlayerSelectionResult(MinigamePlayerState *playerState)
{
    float difficultyPositionX;
    s32 i;
    s32 menuSpriteGroupId;
    s32 difficultyMoveDirection;
    s32 previousDifficulty;
    s32 isDifficultyConfirmed;

    difficultyMoveDirection = 0;
    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    while (TRUE) {
        UpdateMenuCamera();
        difficultyMoveDirection = 0;
        previousDifficulty = playerState->difficulty;
        if (HuPadStkX[minigamePlayerStates->controllerPort] >= 5 || (HuPadBtn[minigamePlayerStates->controllerPort] & 2)) {
            if (GWGameStat.veryHardUnlock == 1) {
                playerState->difficulty++;
                difficultyMoveDirection = 1;
                if (playerState->difficulty > 3) {
                    playerState->difficulty = 0;
                }
            }
            else {
                playerState->difficulty++;
                difficultyMoveDirection = 1;
                if (playerState->difficulty > 2) {
                    playerState->difficulty = 0;
                }
            }
        }
        else if (HuPadStkX[minigamePlayerStates->controllerPort] <= -5 || (HuPadBtn[minigamePlayerStates->controllerPort] & 1)) {
            if (GWGameStat.veryHardUnlock == 1) {
                playerState->difficulty--;
                difficultyMoveDirection = -1;
                if (playerState->difficulty < 0) {
                    playerState->difficulty = 3;
                }
            }
            else {
                playerState->difficulty--;
                difficultyMoveDirection = -1;
                if (playerState->difficulty < 0) {
                    playerState->difficulty = 2;
                }
            }
        }
        else if (HuPadBtnDown[0] & 0x100) {
            isDifficultyConfirmed = 1;
            HuAudFXPlay(2);
            break;
        }
        else if (HuPadBtnDown[0] & 0x200) {
            isDifficultyConfirmed = 0;
            HuAudFXPlay(3);
            break;
        }
        if (playerState->difficulty != previousDifficulty) {
            HuAudFXPlay(0);
            for (i = 0; i <= 5; i++) {
                UpdateMenuCamera();
                difficultyPositionX = InterpolateValueClamped(playerSelectionSpritePositions[playerState->character % 4][0], playerSelectionSpritePositions[playerState->character % 4][0] + difficultyMoveDirection * 10, frame, 5.0f);
                HuSprPosSet(menuSpriteGroupId, 32 + playerState->playerIndex, difficultyPositionX, playerSelectionSpritePositions[playerState->character / 4][1] + 35);
                HuSprPosSet(menuSpriteGroupId, 36 + playerState->playerIndex, difficultyPositionX + 1.0f, playerSelectionSpritePositions[playerState->character / 4][1] + 37);
            }
            HuSprBankSet(menuSpriteGroupId, 32 + playerState->playerIndex, playerState->difficulty);
            for (i = 0; i <= 5; i++) {
                UpdateMenuCamera();
                difficultyPositionX = InterpolateValueClamped(playerSelectionSpritePositions[playerState->character % 4][0] + difficultyMoveDirection * 10, playerSelectionSpritePositions[playerState->character % 4][0], frame, 5.0f);
                HuSprPosSet(menuSpriteGroupId, 32 + playerState->playerIndex, difficultyPositionX, playerSelectionSpritePositions[playerState->character / 4][1] + 35);
                HuSprPosSet(menuSpriteGroupId, 36 + playerState->playerIndex, difficultyPositionX + 1.0f, playerSelectionSpritePositions[playerState->character / 4][1] + 37);
            }
        }
    }
    if (isDifficultyConfirmed == 1) {
        HuSprPosSet(menuSpriteGroupId, 49 + playerState->playerIndex, playerSelectionSpritePositions[playerState->character % 4][0], playerSelectionSpritePositions[playerState->character / 4][1] + 35);
        HuSprBankSet(menuSpriteGroupId, 49 + playerState->playerIndex, playerState->difficulty);
        HuSprAttrReset(menuSpriteGroupId, 49 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
        HuSprAttrSet(menuSpriteGroupId, 32 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
    }
    return isDifficultyConfirmed;
}

void UpdateComputerPlayerSelection(s32 playerIndex, MinigamePlayerState *playerState)
{
    float playerTargetX;
    float playerTargetY;
    s32 menuSpriteGroupId;
    s32 horizontalMoveDirection;
    s32 verticalMoveDirection;
    s32 currentDifficulty;
    s32 playerStartColumn;
    s32 playerStartRow;
    s32 playerColumn;
    s32 playerRow;
    s32 i;

    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    horizontalMoveDirection = 0;
    verticalMoveDirection = 0;
    currentDifficulty = playerState->difficulty;
    playerStartColumn = playerColumn = playerState->character % 4;
    playerStartRow = playerRow = playerState->character / 4;
    if (playerState->selectionFlags[0] == 0) {
        if (HuPadBtnDown[minigamePlayerStates->controllerPort] & 0x100) {
            playerState->selectionFlags[0] = 1;
            HuSprBankSet(menuSpriteGroupId, playerState->character, 1);
            HuSprBankSet(menuSpriteGroupId, 24 + playerState->playerIndex, 1);
            HuSprBankSet(menuSpriteGroupId, 28 + playerState->playerIndex, 1);
            HuAudFXPlay(0x6D + playerState->character);
            UpdatePlayerSelectionDisplay(playerState);
        }
        else {
            if (HuPadStkX[minigamePlayerStates->controllerPort] >= 5 || (HuPadBtn[minigamePlayerStates->controllerPort] & 2)) {
                playerColumn++;
                horizontalMoveDirection = 1;
                if (playerColumn > 3) {
                    playerColumn = 3;
                }
            }
            else if (HuPadStkX[minigamePlayerStates->controllerPort] <= -5 || (HuPadBtn[minigamePlayerStates->controllerPort] & 1)) {
                playerColumn--;
                horizontalMoveDirection = -1;
                if (playerColumn < 0) {
                    playerColumn = 0;
                }
            }
            if (HuPadStkY[minigamePlayerStates->controllerPort] <= -5 || (HuPadBtn[minigamePlayerStates->controllerPort] & 4)) {
                playerRow++;
                verticalMoveDirection = 1;
                if (playerRow > 1) {
                    playerRow = 1;
                }
            }
            else if (HuPadStkY[minigamePlayerStates->controllerPort] >= 5 || (HuPadBtn[minigamePlayerStates->controllerPort] & 8)) {
                playerRow--;
                verticalMoveDirection = -1;
                if (playerRow < 0) {
                    playerRow = 0;
                }
            }
        }
    }
    else if (HuPadBtnDown[minigamePlayerStates->controllerPort] & 0x200) {
        playerState->selectionFlags[0] = playerState->selectionFlags[2] = 0;
        HuSprBankSet(menuSpriteGroupId, playerState->character, 0);
        HuSprBankSet(menuSpriteGroupId, 24 + playerState->playerIndex, 0);
        HuSprBankSet(menuSpriteGroupId, 28 + playerState->playerIndex, 0);
        HuSprAttrSet(menuSpriteGroupId, 32 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
        HuSprAttrSet(menuSpriteGroupId, 36 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
        HuAudFXPlay(3);
    }
    if (playerState->character == playerColumn + playerRow * 4) {
        return;
    }
    do {
        for (i = 0; i < 4; i++) {
            if (i != playerState->playerIndex && minigamePlayerStates[i].selectionFlags[0] == 1 && playerColumn + playerRow * 4 == minigamePlayerStates[i].character) {
                if (horizontalMoveDirection == 0 || verticalMoveDirection == 0) {
                    if (playerColumn + horizontalMoveDirection > 3 || playerColumn + horizontalMoveDirection < 0) {
                        playerColumn = playerStartColumn;
                    }
                    else {
                        playerColumn += horizontalMoveDirection;
                    }
                    if (playerRow + verticalMoveDirection > 1 || playerRow + verticalMoveDirection < 0) {
                        playerRow = playerStartRow;
                    }
                    else {
                        playerRow += verticalMoveDirection;
                    }
                }
                else {
                    if (playerColumn + horizontalMoveDirection <= 3 && playerColumn + horizontalMoveDirection >= 0) {
                        playerColumn += horizontalMoveDirection;
                    }
                    else if (playerRow + verticalMoveDirection <= 1 && playerRow + verticalMoveDirection >= 0) {
                        playerRow += verticalMoveDirection;
                    }
                    else {
                        playerColumn = playerStartColumn;
                        playerRow = playerStartRow;
                    }
                }
                break;
            }
        }
    } while (i != 4);
    if (playerState->character != playerColumn + playerRow * 4) {
        playerState->character = playerColumn + playerRow * 4;
        HuAudFXPlay(0);
        for (i = 0; i <= 10; i++) {
            UpdateMenuCamera();
            playerTargetX = InterpolateValueClamped(playerSelectionSpritePositions[playerStartColumn][0], playerSelectionSpritePositions[playerColumn][0], frame, 10.0f);
            playerTargetY = InterpolateValueClamped(playerSelectionSpritePositions[playerStartRow][1], playerSelectionSpritePositions[playerRow][1], frame, 10.0f);
            HuSprPosSet(menuSpriteGroupId, 24 + playerState->playerIndex, playerTargetX, playerTargetY);
            HuSprPosSet(menuSpriteGroupId, 28 + playerState->playerIndex, playerTargetX, playerTargetY);
            HuSprPosSet(menuSpriteGroupId, 32 + playerState->playerIndex, playerTargetX, 35.0f + playerTargetY);
            HuSprPosSet(menuSpriteGroupId, 36 + playerState->playerIndex, 1.0f + playerTargetX, 37.0f + playerTargetY);
        }
    }
}

void ApplyPlayerSelectionResult(MinigamePlayerState *playerState)
{
    s32 menuSpriteGroupId;
    s32 i;

    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    while (TRUE) {
        for (i = 0; i < 4; i++) {
            if (i != playerState->playerIndex && minigamePlayerStates[i].selectionFlags[0] == 1 && playerState->character == minigamePlayerStates[i].character) {
                playerState->character++;
                break;
            }
        }
        if (i == 4) {
            break;
        }
    };
    playerState->difficulty = 0;
    HuSprBankSet(menuSpriteGroupId, playerState->character, 0);
    HuSprBankSet(menuSpriteGroupId, 24 + playerState->playerIndex, 0);
    HuSprBankSet(menuSpriteGroupId, 28 + playerState->playerIndex, 0);
    HuSprBankSet(menuSpriteGroupId, 32 + playerState->playerIndex, playerState->difficulty);
    HuSprBankSet(menuSpriteGroupId, 49 + playerState->playerIndex, playerState->difficulty);
    HuSprPosSet(menuSpriteGroupId, 24 + playerState->playerIndex, playerSelectionSpritePositions[playerState->character % 4][0], playerSelectionSpritePositions[playerState->character / 4][1]);
    HuSprPosSet(menuSpriteGroupId, 28 + playerState->playerIndex, playerSelectionSpritePositions[playerState->character % 4][0], playerSelectionSpritePositions[playerState->character / 4][1]);
    HuSprPosSet(menuSpriteGroupId, 32 + playerState->playerIndex, playerSelectionSpritePositions[playerState->character % 4][0], playerSelectionSpritePositions[playerState->character / 4][1] + 35);
    HuSprPosSet(menuSpriteGroupId, 36 + playerState->playerIndex, playerSelectionSpritePositions[playerState->character % 4][0] + 1, playerSelectionSpritePositions[playerState->character / 4][1] + 37);
    HuSprPosSet(menuSpriteGroupId, 49 + playerState->playerIndex, playerSelectionSpritePositions[playerState->character % 4][0] + 1, playerSelectionSpritePositions[playerState->character / 4][1] + 37);
    playerState->selectionFlags[2] = 0;
    HuSprAttrReset(menuSpriteGroupId, 24 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
    HuSprAttrReset(menuSpriteGroupId, 28 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(menuSpriteGroupId, 32 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(menuSpriteGroupId, 36 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(menuSpriteGroupId, 49 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
}

void UpdateDifficultySelection(MinigamePlayerState *playerState)
{
    s32 menuSpriteGroupId;

    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    HuSprBankSet(menuSpriteGroupId, playerState->character, 0);
    HuSprAttrSet(menuSpriteGroupId, 24 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(menuSpriteGroupId, 28 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(menuSpriteGroupId, 32 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(menuSpriteGroupId, 36 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(menuSpriteGroupId, 49 + playerState->playerIndex, HUSPR_ATTR_DISPOFF);
    #if VERSION_PAL
    HuSprAttrSet(menuSpriteGroupId, 85, HUSPR_ATTR_DISPOFF);
    #endif
}

void ShowDifficultySelection(void)
{
    s32 menuSpriteGroupId;
    s32 i;

    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    HuSprPosSet(menuSpriteGroupId, 54, minigameMenuLayoutValues[3], minigameMenuLayoutValues[2]);
    HuSprPosSet(menuSpriteGroupId, 55, minigameMenuLayoutValues[4], minigameMenuLayoutValues[2]);
    for (i = 0; i < 2; i++) {
        HuSprAttrReset(menuSpriteGroupId, 54 + i, HUSPR_ATTR_DISPOFF);
    }
}

void HideDifficultySelection(void)
{
    s32 menuSpriteGroupId;
    s32 i;

    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    HuSprPosSet(menuSpriteGroupId, 54, minigameMenuLayoutValues[3], minigameMenuLayoutValues[2]);
    HuSprPosSet(menuSpriteGroupId, 55, minigameMenuLayoutValues[4], minigameMenuLayoutValues[2]);
    for (i = 0; i < 2; i++) {
        HuSprAttrSet(menuSpriteGroupId, 54 + i, HUSPR_ATTR_DISPOFF);
    }
}

void UpdateOptionSelection(s32 value, OptionMenuWork *secondaryValue)
{
    float slideOffset;
    float selectionScale;
    s32 optionSpriteGroupId;
    s32 selectionDirection;
    s32 previousInputLock;
    s32 i;

    optionSpriteGroupId = secondaryValue->optionSpriteGroupId;
    selectionDirection = 0;
    previousInputLock = secondaryValue->inputLocked;
    minigameMenuWork.selectionState[3] = 0;
    secondaryValue->inputLocked = 1;
    if (HuPadStkX[minigamePlayerStates->controllerPort] >= 5 || (HuPadBtn[minigamePlayerStates->controllerPort] & 2)) {
        selectionDirection = 1;
        secondaryValue->selectedOption++;
        if (secondaryValue->selectedOption > 2) {
            secondaryValue->selectedOption = 0;
        }
    }
    else if (HuPadStkX[minigamePlayerStates->controllerPort] <= -5 || (HuPadBtn[minigamePlayerStates->controllerPort] & 1)) {
        selectionDirection = -1;
        secondaryValue->selectedOption--;
        if (secondaryValue->selectedOption < 0) {
            secondaryValue->selectedOption = 2;
        }
    }
    if (selectionDirection != 0) {
        minigameMenuWork.selectionState[3] = 1;
        HuAudFXPlay(0);
        for (i = 0; i <= 10; i++) {
            UpdateMenuCamera();
            if (i <= 5) {
                slideOffset = InterpolateValueClamped(0.0f, 10.0f, frame, 5.0f);
                selectionScale = InterpolateValueClamped(1.0f, 0.0f, frame, 5.0f);
            }
            else {
                slideOffset = InterpolateValueClamped(10.0f, 0.0f, i - 5, 5.0f);
                selectionScale = InterpolateValueClamped(0.0f, 1.0f, i - 5, 5.0f);
            }
            if (selectionDirection == 1) {
                HuSprPosSet(optionSpriteGroupId, 55, minigameMenuLayoutValues[4] + slideOffset, minigameMenuLayoutValues[2]);
                HuSprPosSet(optionSpriteGroupId, 56, 380.0f + slideOffset, minigameMenuLayoutValues[2]);
            }
            else {
                HuSprPosSet(optionSpriteGroupId, 54, minigameMenuLayoutValues[3] - slideOffset, minigameMenuLayoutValues[2]);
                HuSprPosSet(optionSpriteGroupId, 56, 380.0f - slideOffset, minigameMenuLayoutValues[2]);
            }
            HuSprScaleSet(optionSpriteGroupId, 56, 1.0f, selectionScale);
            if (i == 6) {
                HuSprBankSet(optionSpriteGroupId, 56, secondaryValue->selectedOption);
            }
        }
    }
    minigameMenuWork.selectionState[3] = 0;
}

void UpdatePlayerStatusDisplay(OMOBJ *object)
{
    float slideOffset;
    float selectionScale;
    s32 menuSpriteGroupId;
    s32 previousSelection;
    s32 selectionDirection;
    s32 j;
    s32 i;

    selectionDirection = 0;
    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    minigameMenuWork.selectionState[3] = 0;
    previousSelection = minigameMenuWork.selectionState[0] - selectionDirection;
    if (previousSelection > 3) {
        previousSelection = 0;
    }
    else if (previousSelection < 0) {
        previousSelection = 3;
    }
    if (HuPadStkX[minigamePlayerStates->controllerPort] >= 5 || (HuPadBtn[minigamePlayerStates->controllerPort] & 2)) {
        selectionDirection = 1;
        minigameMenuWork.selectionState[3] = 1;
        HuAudFXPlay(0);
        minigameMenuWork.selectionState[0]++;
        if (minigameMenuWork.selectionState[0] > 3) {
            minigameMenuWork.selectionState[0] = 1;
        }
    }
    else if (HuPadStkX[minigamePlayerStates->controllerPort] <= -5 || (HuPadBtn[minigamePlayerStates->controllerPort] & 1)) {
        selectionDirection = -1;
        minigameMenuWork.selectionState[3] = 1;
        HuAudFXPlay(0);
        minigameMenuWork.selectionState[0]--;
        if (minigameMenuWork.selectionState[0] < 1) {
            minigameMenuWork.selectionState[0] = 3;
        }
    }
    if (selectionDirection != 0) {
        for (i = 0; i <= 10; i++) {
            UpdateMenuCamera();
            if (i <= 5) {
                slideOffset = InterpolateValueClamped(0.0f, 10.0f, frame, 5.0f);
                selectionScale = InterpolateValueClamped(1.0f, 0.0f, frame, 5.0f);
            }
            else {
                slideOffset = InterpolateValueClamped(10.0f, 0.0f, i - 5, 5.0f);
                selectionScale = InterpolateValueClamped(0.0f, 1.0f, i - 5, 5.0f);
            }
            if (selectionDirection == 1) {
                HuSprPosSet(menuSpriteGroupId, 55, minigameMenuLayoutValues[1] + slideOffset, minigameMenuLayoutValues[2]);
            }
            else {
                HuSprPosSet(menuSpriteGroupId, 54, minigameMenuLayoutValues[0] - slideOffset, minigameMenuLayoutValues[2]);
            }
            if (minigameMenuWork.selectionState[0] != 0) {
                selectionScale = InterpolateValueClamped(0.0f, 1.0f, frame, 10.0f);
            }
            else {
                selectionScale = InterpolateValueClamped(1.0f, 0.0f, frame, 10.0f);
            }
            HuSprScaleSet(menuSpriteGroupId, 85, selectionScale, selectionScale);
            for (j = 0; j < 4; j++) {
                slideOffset = InterpolateValueClamped(characterSelectionColumns[minigamePlayerStates[j].playerIndex][previousSelection], characterSelectionColumns[minigamePlayerStates[j].playerIndex][minigameMenuWork.selectionState[0]], frame, 10.0f);
                HuSprPosSet(menuSpriteGroupId, 57 + minigamePlayerStates[j].character, slideOffset, minigameMenuLayoutValues[2]);
                HuSprPosSet(menuSpriteGroupId, 65 + j, slideOffset + 1.0f, minigameMenuLayoutValues[2] + 2);
                if (minigamePlayerStates[j].isComputer != 0) {
                    HuSprPosSet(menuSpriteGroupId, 77 + j, slideOffset, minigameMenuLayoutValues[2] + 21);
                }
                else {
                    HuSprPosSet(menuSpriteGroupId, 73 + j, slideOffset - 8.0f, minigameMenuLayoutValues[2] - 21);
                }
            }
        }
    }
    minigameMenuWork.selectionState[3] = 0;
}

void RefreshMinigameMenuDisplay(void)
{
    s32 menuSpriteGroupId;
    s32 i;

    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    for (i = 0; i < 4; i++) {
        HuSprAttrSet(menuSpriteGroupId, 77 + i, HUSPR_ATTR_DISPOFF);
        HuSprAttrSet(menuSpriteGroupId, 73 + i, HUSPR_ATTR_DISPOFF);
        HuSprAttrSet(menuSpriteGroupId, 57 + minigamePlayerStates[i].character, HUSPR_ATTR_DISPOFF);
        HuSprAttrSet(menuSpriteGroupId, 85, HUSPR_ATTR_DISPOFF);
        HuSprAttrSet(menuSpriteGroupId, 65 + minigamePlayerStates[i].playerIndex, HUSPR_ATTR_DISPOFF);
        HuSprScaleSet(menuSpriteGroupId, 77 + i, 0.0f, 0.0f);
        HuSprScaleSet(menuSpriteGroupId, 73 + i, 0.0f, 0.0f);
        HuSprScaleSet(menuSpriteGroupId, 57 + minigamePlayerStates[i].character, 0.0f, 0.0f);
        HuSprScaleSet(menuSpriteGroupId, 81 + i, 1.0f, 1.0f);
    }
}

void SetPlayerStatusDisplayMode(s32 playerIndex)
{
    float spriteScaleY;
    float spriteScaleX;
    s32 menuSpriteGroupId;
    s32 playerIndex;
    s32 i;
    s32 j;

    menuSpriteGroupId = minigameMenuWork.menuSpriteGroupId;
    if (playerIndex == 2) {
        for (i = 0; i < 2; i++) {
            HuSprAttrSet(menuSpriteGroupId, 54 + i, HUSPR_ATTR_DISPOFF);
        }
        return;
    }
    for (i = 0; i < 4; i++) {
        HuSprAttrReset(menuSpriteGroupId, 57 + minigamePlayerStates[i].character, HUSPR_ATTR_DISPOFF);
        HuSprPosSet(menuSpriteGroupId, 57 + minigamePlayerStates[i].character, characterSelectionLayout[minigamePlayerStates[i].playerIndex][0], minigameMenuLayoutValues[2]);
        if (playerIndex == 0) {
            HuSprScaleSet(menuSpriteGroupId, 57 + minigamePlayerStates[i].character, 0.0f, 1.0f);
        }
        else {
            HuSprScaleSet(menuSpriteGroupId, 57 + minigamePlayerStates[i].character, 1.0f, 1.0f);
        }
        if (minigamePlayerStates[i].isComputer != 0) {
            HuSprAttrReset(menuSpriteGroupId, 77 + i, HUSPR_ATTR_DISPOFF);
            HuSprBankSet(menuSpriteGroupId, 77 + i, minigamePlayerStates[i].difficulty);
            HuSprPosSet(menuSpriteGroupId, 77 + i, characterSelectionLayout[minigamePlayerStates[i].playerIndex][0], minigameMenuLayoutValues[2] + 21);
            if (playerIndex == 0) {
                HuSprScaleSet(menuSpriteGroupId, 77 + i, 0.0f, 1.0f);
            }
            else {
                HuSprScaleSet(menuSpriteGroupId, 77 + i, 1.0f, 1.0f);
            }
        }
        else {
            HuSprAttrReset(menuSpriteGroupId, 73 + i, HUSPR_ATTR_DISPOFF);
            HuSprBankSet(menuSpriteGroupId, 73 + i, minigamePlayerStates[i].playerIndex);
            HuSprPosSet(menuSpriteGroupId, 73 + i, characterSelectionLayout[minigamePlayerStates[i].playerIndex][0] - 8.0f, minigameMenuLayoutValues[2] - 21);
            if (playerIndex == 0) {
                HuSprScaleSet(menuSpriteGroupId, 73 + i, 0.0f, 1.0f);
            }
            else {
                HuSprScaleSet(menuSpriteGroupId, 73 + i, 1.0f, 1.0f);
            }
        }
    }
    HuSprAttrReset(menuSpriteGroupId, 85, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(menuSpriteGroupId, 85, 340.0f, minigameMenuLayoutValues[2]);
    HuSprScaleSet(menuSpriteGroupId, 85, 0.0f, 0.0f);
    if (playerIndex == 0) {
        minigameMenuWork.selectionState[0] = 1;
        playerIndex = 0;
    }
    else {
        playerIndex = minigameMenuWork.selectionState[0];
        minigameMenuWork.selectionState[0] = 0;
    }
    for (i = 0; i <= 10; i++) {
        if (i <= 5) {
            spriteScaleY = InterpolateValueClamped(1.0f, 0.0f, frame, 5.0f);
        }
        else {
            spriteScaleY = InterpolateValueClamped(0.0f, 1.0f, i - 5, 5.0f);
        }
        for (j = 0; j < 4; j++) {
            HuSprScaleSet(menuSpriteGroupId, 65 + j, spriteScaleY, 1.0f);
            if ((i > 5 && playerIndex == 0) || (i <= 5 && playerIndex == 1)) {
                HuSprScaleSet(menuSpriteGroupId, 57 + minigamePlayerStates[j].character, spriteScaleY, 1.0f);
                if (minigamePlayerStates[j].isComputer != 0) {
                    HuSprScaleSet(menuSpriteGroupId, 77 + j, spriteScaleY, 1.0f);
                }
                else {
                    HuSprScaleSet(menuSpriteGroupId, 73 + j, spriteScaleY, 1.0f);
                }
            }
            else {
                HuSprScaleSet(menuSpriteGroupId, 81 + j, spriteScaleY, 1.0f);
            }
            spriteScaleX = InterpolateValueClamped(characterSelectionColumns[minigamePlayerStates[j].playerIndex][playerIndex], characterSelectionColumns[minigamePlayerStates[j].playerIndex][minigameMenuWork.selectionState[0]], frame, 10.0f);
            HuSprPosSet(menuSpriteGroupId, 57 + minigamePlayerStates[j].character, spriteScaleX, minigameMenuLayoutValues[2]);
            HuSprPosSet(menuSpriteGroupId, 65 + j, spriteScaleX + 1.0f, minigameMenuLayoutValues[2] + 2);
            if (minigamePlayerStates[j].isComputer != 0) {
                HuSprPosSet(menuSpriteGroupId, 77 + j, spriteScaleX, minigameMenuLayoutValues[2] + 21);
            }
            else {
                HuSprPosSet(menuSpriteGroupId, 73 + j, spriteScaleX - 8.0f, minigameMenuLayoutValues[2] - 21);
            }
        }
        if (minigameMenuWork.selectionState[0] != 0) {
            spriteScaleY = InterpolateValueClamped(0.0f, 1.0f, frame, 10.0f);
        }
        else {
            spriteScaleY = InterpolateValueClamped(1.0f, 0.0f, frame, 10.0f);
        }
        HuSprScaleSet(menuSpriteGroupId, 85, spriteScaleY, spriteScaleY);
        UpdateMenuCamera();
    }
    HuSprPosSet(menuSpriteGroupId, 54, minigameMenuLayoutValues[0], minigameMenuLayoutValues[2]);
    HuSprPosSet(menuSpriteGroupId, 55, minigameMenuLayoutValues[1], minigameMenuLayoutValues[2]);
    if (playerIndex == 0) {
        for (i = 0; i < 2; i++) {
            HuSprAttrReset(menuSpriteGroupId, 54 + i, HUSPR_ATTR_DISPOFF);
        }
    }
    else {
        for (i = 0; i < 2; i++) {
            HuSprAttrSet(menuSpriteGroupId, 54 + i, HUSPR_ATTR_DISPOFF);
        }
    }
}

void CreateModeSelectionWindow(void)
{
    ShowMinigameMenuSprites();
}

s32 GetModeSelectionChoice(void)
{
    s32 startResult;
    s32 messageWindowId;
    s32 statusSpriteGroupId;
    s32 previousPlayerCount;
    s32 selectionResult;
    s32 activePlayerCount;
    s32 humanPlayerCount;
    s32 statusSpriteGroupId;
    s32 i;

    startResult = 0;
    messageWindowId = CreateStandardMessageWindow(0, 0, 0);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
    ShowMessageAndWait(windowId, MAKE_MESSID(26, 5), -1, -1);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
    UpdateMenuCamera();
    CreateCenteredMessageWindow(MAKE_MESSID(26, 33));
    statusSpriteGroupId = minigameMenuWork.statusSpriteGroupId;
    HuSprAttrReset(statusSpriteGroupId, 8, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(statusSpriteGroupId, 8, playerStatusSpritePositions[0][0] - 50, playerStatusSpritePositions[0][1]);
    HuSprAttrReset(statusSpriteGroupId, 9, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(statusSpriteGroupId, 9, playerStatusSpritePositions[3][0] + 50, playerStatusSpritePositions[0][1]);
    minigameMenuWork.updateCallback = (void *)UpdateMinigameMenuInput;
    previousPlayerCount = 99;
    startResult = 0;
    while (TRUE) {
        UpdateMenuCamera();
        activePlayerCount = 0;
        for (i = 0; i < 4; i++) {
            if (minigamePlayerStates[i].selectionFlags[3] == 0) {
                activePlayerCount++;
            }
        }
        if (previousPlayerCount != activePlayerCount) {
            previousPlayerCount = activePlayerCount;
            ShowMessageAndWait(windowId, MAKE_MESSID(26, 27 + activePlayerCount), -1, -999);
        }
        if (minigameMenuWork.selectionState[0] != 0) {
            continue;
        }
        if (HuPadBtnDown[minigamePlayerStates->controllerPort] & 0x100) {
            selectionResult = 1;
            HuAudFXPlay(2);
            minigameMenuWork.updateCallback = NULL;
            WaitForMenuFrames(2);
            activePlayerCount = humanPlayerCount = 0;
            for (i = 0; i < 4; i++) {
                if (minigamePlayerStates[i].isComputer == 0) {
                    humanPlayerCount++;
                }
            }
            for (i = 0; i < 4; i++) {
                if (activePlayerCount < humanPlayerCount) {
                    if (HuPadStatGet(i) != -1) {
                        activePlayerCount++;
                        minigamePlayerStates[i].isComputer = 0;
                    }
                    else {
                        minigamePlayerStates[i].isComputer = 1;
                    }
                }
            }
            break;
        }
        else if (HuPadBtnDown[minigamePlayerStates->controllerPort] & 0x200) {
            selectionResult = -99;
            HuAudFXPlay(3);
            break;
        }
    }
    statusSpriteGroupId = minigameMenuWork.statusSpriteGroupId;
    HuSprAttrSet(statusSpriteGroupId, 8, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(statusSpriteGroupId, 8, playerStatusSpritePositions[0][0] - 50, playerStatusSpritePositions[0][1]);
    HuSprAttrSet(statusSpriteGroupId, 9, HUSPR_ATTR_DISPOFF);
    HuSprPosSet(statusSpriteGroupId, 9, playerStatusSpritePositions[3][0] + 50, playerStatusSpritePositions[0][1]);
    minigameMenuWork.updateCallback = NULL;
    CloseCenteredMessageWindow(0);
    CloseStandardMessageWindow(windowId);
    if (selectionResult != -99) {
        activePlayerCount = 0;
        for (i = 0; i < 4; i++) {
            if (minigamePlayerStates[i].isComputer == 0) {
                activePlayerCount++;
            }
        }
        if (activePlayerCount == 0) {
            selectionResult = 2;
        }
        else {
            selectionResult = 1;
        }
    }
    return selectionResult;
}

void ShowModeSelectionMessage(s32 windowId)
{
    float slidePositionY;
    s32 unreferencedAnimationScratch;
    s32 unreferencedFrameScratch;
    s32 statusSpriteGroupId;
    s32 i;

    unreferencedFrameScratch = 0;
    unreferencedAnimationScratch = 0;
    statusSpriteGroupId = minigameMenuWork.statusSpriteGroupId;
    for (i = 0; i <= 60; i++) {
        UpdateMenuCamera();
        if (i <= 10) {
            slidePositionY = InterpolateValueClamped(2.06f, 10.0f, frame, 10.0f);
        }
        else {
            slidePositionY = InterpolateValue(10.0f, -500.0f, i - 10, 50.0f);
        }
        HuSprGrpPosSet(statusSpriteGroupId, 0.0f, slidePositionY);
    }
    SetMinigameMenuMode(0);
    ResetMinigameSelectionSprites();
    SetMinigameMenuMode(modeType);
}

void ShowPlayerSelectionMessage(s32 playerIndex)
{
    SlideMinigameMenuOnscreen();
}

s32 SelectFreePlayPlayers(void)
{
    s32 anyPlayerConfirmed;
    s32 unreferencedInputScratch;
    s32 inactivePlayerCount;
    s32 messageWindowId;
    s32 selectionResult;
    s32 i;

    anyPlayerConfirmed = 0;
    unreferencedInputScratch = 0;
    ResetMinigameSelectionSprites();
    messageWindowId = CreateStandardMessageWindow(0, 0, 0);
    HuWinPushKeySet(messageWindowId, 0x300);
    UpdateMenuCamera();
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
    ShowMessageAndWait(windowId, MAKE_MESSID(26, 9), -1, -1);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
    CreateCenteredMessageWindow(MAKE_MESSID(26, 32));
    for (i = 0; i < 4; i++) {
        if (minigamePlayerStates[i].isComputer == 0) {
            minigamePlayerStates[i].updateCallback = (void *)UpdatePlayerSelectionCursor;
        }
    }
    while (TRUE) {
        UpdateMenuCamera();
        for (i = 0; i < 4; i++) {
            if (minigamePlayerStates[i].isComputer == 0 && (HuPadBtnDown[minigamePlayerStates[i].controllerPort] & 0x100)) {
                anyPlayerConfirmed = 1;
            }
        }
        if (minigamePlayerStates->selectionFlags[0] == 0 && (HuPadBtnDown[minigamePlayerStates->controllerPort] & 0x200)) {
            HuAudFXPlay(3);
            selectionResult = 0;
            break;
        }
        else {
            for (i = 0; i < 4; i++) {
                if (minigamePlayerStates[i].isComputer == 0 && minigamePlayerStates[i].selectionFlags[0] != 1) {
                    break;
                }
            }
            if (i == 4) {
                selectionResult = 2;
                break;
            }
        }
    }
    for (i = 0; i < 4; i++) {
        minigamePlayerStates[i].updateCallback = NULL;
    }
    CloseCenteredMessageWindow(0);
    CloseStandardMessageWindow(windowId);
    if (selectionResult == 2) {
        inactivePlayerCount = 0;
        for (i = 0; i < 4; i++) {
            if (minigamePlayerStates[i].isComputer != 0) {
                minigamePlayerStates[i].character = 0;
                inactivePlayerCount++;
            }
        }
        if (inactivePlayerCount == 0) {
            selectionResult = 3;
        }
        else {
            selectionResult = 2;
        }
    }
    else {
        selectionResult = 0;
    }
    return selectionResult;
}

s32 SelectTeamPlayPlayers(void)
{
    s32 selectionResult;
    s32 activePlayer;
    s32 selectionState;
    s32 selectedPlayer;
    s32 unreferencedInputScratch;
    s32 messageWindowId;
    s32 inactivePlayerCount;
    s32 confirmedPlayerCount;
    s32 i;

    selectionResult = -1;
    activePlayer = -1;
    selectionState = -1;
    selectedPlayer = -1;
    unreferencedInputScratch = 0;
    messageWindowId = CreateStandardMessageWindow(0, 0, 0);
    while (TRUE) {
        UpdateMenuCamera();
        inactivePlayerCount = 0;
        for (i = 0; i < 4; i++) {
            if (minigamePlayerStates[i].isComputer != 0 && minigamePlayerStates[i].selectionFlags[0] == 0) {
                inactivePlayerCount++;
            }
        }
        HuWinInsertMesSet(messageWindowId, MAKE_MESSID(26, 35 + inactivePlayerCount), 1);
        if (selectionState == -1) {
            SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
        }
        ShowMessageAndWait(windowId, MAKE_MESSID(26, 40), -1, waitFrames);
        if (selectionState == -1) {
            SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
        }
        selectionState = -999;
        CreateCenteredMessageWindow(MAKE_MESSID(26, 32));
        for (i = 0; i < 4; i++) {
            if (minigamePlayerStates[i].isComputer != 0 && minigamePlayerStates[i].selectionFlags[0] == 0) {
                selectionResult = i;
                minigamePlayerStates[i].selectionFlags[1] = activePlayer;
                ApplyPlayerSelectionResult(&minigamePlayerStates[i]);
                minigamePlayerStates[i].updateCallback = (void *)UpdateComputerPlayerSelection;
                break;
            }
        }
        while (TRUE) {
            UpdateMenuCamera();
            if ((HuPadBtnDown[minigamePlayerStates->controllerPort] & 0x200) && minigamePlayerStates[selectionResult].selectionFlags[0] == 0) {
                HuAudFXPlay(3);
                if (activePlayer == -1) {
                    confirmedPlayerCount = 0;
                    break;
                }
                else {
                    confirmedPlayerCount = 1;
                    break;
                }
            }
            else if (minigamePlayerStates[selectionResult].selectionFlags[0] == 1) {
                for (i = 0; i < 4; i++) {
                    if (minigamePlayerStates[i].selectionFlags[0] == 0) {
                        minigamePlayerStates[i].character = 0;
                    }
                }
                minigamePlayerStates[selectionResult].updateCallback = NULL;
                ShowMessageAndWait(windowId, MAKE_MESSID(26, 34), -1, -999);
                CreateCenteredMessageWindow(MAKE_MESSID(26, 35));
                confirmedPlayerCount = GetPlayerSelectionResult(&minigamePlayerStates[selectionResult]);
                if (confirmedPlayerCount == 1) {
                    minigamePlayerStates[selectionResult].selectionFlags[2] = 1;
                    confirmedPlayerCount = 4;
                    activePlayer = selectionResult;
                }
                else {
                    minigamePlayerStates[selectionResult].selectionFlags[2] = 0;
                    confirmedPlayerCount = 4;
                    minigamePlayerStates[selectionResult].selectionFlags[0] = 0;
                    UpdateDifficultySelection(&minigamePlayerStates[selectionResult]);
                    ApplyPlayerSelectionResult(&minigamePlayerStates[selectionResult]);
                }
                break;
            }
        }
        for (i = 0; i < 4; i++) {
            minigamePlayerStates[i].updateCallback = NULL;
        }
        for (i = 0; i < 4; i++) {
            if (minigamePlayerStates[i].selectionFlags[0] == 0) {
                break;
            }
        }
        if (i == 4) {
            confirmedPlayerCount = 3;
            break;
        }
        else {
            if (confirmedPlayerCount == 1) {
                ApplyPlayerSelectionResult(&minigamePlayerStates[selectionResult]);
                UpdateDifficultySelection(&minigamePlayerStates[selectionResult]);
                ApplyPlayerSelectionResult(&minigamePlayerStates[minigamePlayerStates[selectionResult].selectionFlags[1]]);
                activePlayer = minigamePlayerStates[minigamePlayerStates[selectionResult].selectionFlags[1]].selectionFlags[1];
                minigamePlayerStates[minigamePlayerStates[selectionResult].selectionFlags[1]].selectionFlags[0] = 0;
            }
            else if (confirmedPlayerCount != 4) {
                if (confirmedPlayerCount == 0) {
                    UpdateDifficultySelection(&minigamePlayerStates[selectionResult]);
                }
                break;
            }
        }
        selectedPlayer = -1;
    }
    CloseCenteredMessageWindow(0);
    CloseStandardMessageWindow(windowId);
    if (confirmedPlayerCount == 0) {
        inactivePlayerCount = 0;
        for (i = 0; i < 4; i++) {
            if (minigamePlayerStates[i].isComputer == 0) {
                inactivePlayerCount++;
            }
        }
        if (inactivePlayerCount != 0) {
            confirmedPlayerCount = 1;
        }
        else {
            confirmedPlayerCount = 0;
        }
    }
    return confirmedPlayerCount;
}

s32 SelectBattlePlayers(void)
{
    s32 unreferencedInputScratch;
    s32 messageWindowId;
    s32 previousSelection;
    s32 selectionResult;
    s32 i;
    s32 j;

    unreferencedInputScratch = 0;
    messageWindowId = CreateStandardMessageWindow(0, 0, 0);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
    ShowMessageAndWait(windowId, MAKE_MESSID(40, 14), -1, -1);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
    UpdateMenuCamera();
    CreateCenteredMessageWindow(MAKE_MESSID(26, 33));
    ShowDifficultySelection();
    minigameMenuWork.updateCallback = (void *)UpdateOptionSelection;
    previousSelection = 99;
    while (TRUE) {
        UpdateMenuCamera();
        if (previousSelection != minigameMenuWork.selectionState[0]) {
            previousSelection = minigameMenuWork.selectionState[0];
            HuWinInsertMesSet(messageWindowId, MAKE_MESSID(27, 52 + minigameMenuWork.selectionState[0]), 0);
            ShowMessageAndWait(windowId, MAKE_MESSID(27, 51), -1, -999);
        }
        if (minigameMenuWork.selectionState[3] != 0) {
            continue;
        }
        if (HuPadBtnDown[minigamePlayerStates->controllerPort] & 0x100) {
            selectionResult = 5;
            HuAudFXPlay(2);
            break;
        }
        else if (HuPadBtnDown[minigamePlayerStates->controllerPort] & 0x200) {
            selectionResult = -99;
            HuAudFXPlay(3);
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 4; j++) {
                    if (minigamePlayerStates[i].isComputer == 1) {
                        minigamePlayerStates[i].selectionFlags[j] = 0;
                        minigamePlayerStates[i].selectionValues[j] = 0;
                        UpdateDifficultySelection(&minigamePlayerStates[i]);
                        minigamePlayerStates[i].character = 0;
                    }
                }
            }
            break;
        }
    }
    HideDifficultySelection();
    minigameMenuWork.updateCallback = NULL;
    CloseCenteredMessageWindow(0);
    CloseStandardMessageWindow(windowId);
    if (selectionResult == -99) {
        j = 0;
        for (i = 0; i < 4; i++) {
            if (minigamePlayerStates[i].isComputer == 1) {
                j++;
            }
        }
        if (j == 0) {
            selectionResult = 1;
        }
        else {
            selectionResult = 2;
        }
    }
    return selectionResult;
}

s32 SelectPlayersForResume(s32 playerIndex)
{
    s32 unreferencedInputScratch;
    s32 messageWindowId;
    s32 previousSelection;
    s32 selectionResult;
    s32 i;
    s32 j;

    unreferencedInputScratch = 0;
    messageWindowId = CreateStandardMessageWindow(0, 0, 0);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
    ShowMessageAndWait(windowId, MAKE_MESSID(40, 13), -1, -1);
    SetModelMotionWithMode(minigameSharedState->modeType, minigameSharedState->selection, 0xF, 1);
    UpdateMenuCamera();
    CreateCenteredMessageWindow(MAKE_MESSID(26, 33));
    SetPlayerStatusDisplayMode(0);
    minigameMenuWork.updateCallback = (void *)UpdatePlayerStatusDisplay;
    previousSelection = 99;
    while (TRUE) {
        UpdateMenuCamera();
        if (previousSelection != minigameMenuWork.selectionState[0]) {
            previousSelection = minigameMenuWork.selectionState[0];
            switch (minigameMenuWork.selectionState[0]) {
                case 1:
                    HuWinInsertMesSet(messageWindowId, MAKE_MESSID(27, 30), 0);
                    HuWinInsertMesSet(messageWindowId, MAKE_MESSID(27, 31), 1);
                    HuWinInsertMesSet(messageWindowId, MAKE_MESSID(27, 32), 2);
                    break;
                case 2:
                    HuWinInsertMesSet(messageWindowId, MAKE_MESSID(27, 31), 0);
                    HuWinInsertMesSet(messageWindowId, MAKE_MESSID(27, 30), 1);
                    HuWinInsertMesSet(messageWindowId, MAKE_MESSID(27, 32), 2);
                    break;
                case 3:
                    HuWinInsertMesSet(messageWindowId, MAKE_MESSID(27, 32), 0);
                    HuWinInsertMesSet(messageWindowId, MAKE_MESSID(27, 30), 1);
                    HuWinInsertMesSet(messageWindowId, MAKE_MESSID(27, 31), 2);
                    break;
            }
            ShowMessageAndWait(windowId, MAKE_MESSID(27, 50), -1, -999);
        }
        if (minigameMenuWork.selectionState[3] != 0) {
            continue;
        }
        if (HuPadBtnDown[minigamePlayerStates->controllerPort] & 0x100) {
            selectionResult = 5;
            HuAudFXPlay(2);
            break;
        }
        else if (HuPadBtnDown[minigamePlayerStates->controllerPort] & 0x200) {
            selectionResult = -99;
            HuAudFXPlay(3);
            SetPlayerStatusDisplayMode(1);
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 4; j++) {
                    if (minigamePlayerStates[i].isComputer == 1) {
                        minigamePlayerStates[i].selectionFlags[j] = 0;
                        minigamePlayerStates[i].selectionValues[j] = 0;
                        UpdateDifficultySelection(&minigamePlayerStates[i]);
                    }
                }
            }
            break;
        }
    }
    SetPlayerStatusDisplayMode(2);
    minigameMenuWork.updateCallback = NULL;
    CloseCenteredMessageWindow(0);
    CloseStandardMessageWindow(windowId);
    if (selectionResult == -99) {
        j = 0;
        for (i = 0; i < 4; i++) {
            if (minigamePlayerStates[i].isComputer == 1) {
                j++;
            }
        }
        if (j == 0) {
            selectionResult = 1;
        }
        else {
            selectionResult = 2;
        }
    }
    return selectionResult;
}

void ClosePlayerSelectionWindows(void)
{
    s32 i;
    s32 j;

    SlideMinigameMenuOffscreen();
    RefreshMinigameMenuDisplay();
    if (minigameSharedState->modeSelectionType == 0 || minigameSharedState->modeSelectionType == 3) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                if (minigamePlayerStates[i].isComputer == 1) {
                    minigamePlayerStates[i].selectionFlags[j] = 0;
                    minigamePlayerStates[i].selectionValues[j] = 0;
                    UpdateDifficultySelection(&minigamePlayerStates[i]);
                }
            }
        }
    }
}

void UpdatePlayerMenuObject(OMOBJ *object)
{
    float targetPositionY;
    s32 i;

    for (i = 1; i < 10; i++) {
        if (CZoom <= 1200.0f || modelTransitionWork[i].targetPositionY == -200.0f) {
            if (modelTransitionWork[i].targetPositionY >= -100.0f) {
                modelTransitionWork[i].animationFrame = 0;
                modelTransitionWork[i].targetPositionY = -200.0f;
                modelTransitionWork[i].horizontalOffset = -99.0f;
                modelTransitionWork[i].animationDuration = rand8() + 180;
            }
            targetPositionY = InterpolateValue(modelTransitionWork[i].startValue, 1000.0f, modelTransitionWork[i].animationFrame++, modelTransitionWork[i].duration);
            Hu3DData[object->mdlId[i]].pos.y = ApproachValue(Hu3DData[object->mdlId[i]].pos.startValue, targetValue, 30.0f);
            Hu3DData[object->mdlId[i]].pos.x
                = ApproachValue(Hu3DData[object->mdlId[i]].pos.startValue, modelTransitionWork[i].targetPositionX + Hu3DData[object->mdlId[i]].pos.targetValue, 5.0f);
        }
    }
    for (i = 10; i < 140; i++) {
        Hu3DData[object->mdlId[i]].pos.y = InterpolateSmoothValue(modelTransitionWork[i].startValue, modelTransitionWork[i].targetValue, modelTransitionWork[i].animationFrame++, modelTransitionWork[i].duration);
        if (modelTransitionWork[i].animationFrame >= modelTransitionWork[i].animationDuration) {
            modelTransitionWork[i].animationFrame = 0;
        }
    }
    for (i = 10; i < 140; i++) {
        if (0.0f == modelTransitionWork[i].horizontalOffset && modelTransitionWork[i].startPositionZ > CZoom - 400.0f && modelTransitionWork[i].startPositionX < 200.0f
            && modelTransitionWork[i].startPositionX > -200.0f) {
            if (0.0f >= modelTransitionWork[i].startPositionX) {
                modelTransitionWork[i].horizontalOffset = -300.0f;
            }
            else {
                modelTransitionWork[i].horizontalOffset = 300.0f;
            }
        }
        if (-99.0f != modelTransitionWork[i].horizontalOffset) {
            Hu3DData[object->mdlId[i]].pos.x = ApproachValue(Hu3DData[object->mdlId[i]].pos.startValue, modelTransitionWork[i].startPositionX + modelTransitionWork[i].targetValue, 30.0f);
        }
    }
    if (object->work[0] == 1) {
        for (i = 10; i < 140; i++) {
            Hu3DModelAttrSet(object->mdlId[i], HU3D_ATTR_DISPOFF);
        }
    }
}

void UpdateMenuCameraObject(OMOBJ *object)
{
    float candidatePositionX;
    float candidatePositionY;
    float candidatePositionZ;
    s32 placementAttempts;
    s32 i;
    s32 j;

    object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MGMODE, 141));
    object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MGMODE, 142));
    object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MGMODE, 143));
    object->mdlId[4] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MGMODE, 144));
    Hu3DModelLayerSet(object->mdlId[1], 3);
    Hu3DModelLayerSet(object->mdlId[2], 3);
    Hu3DModelLayerSet(object->mdlId[3], 3);
    Hu3DModelLayerSet(object->mdlId[4], 3);
    for (i = 5; i < 140; i++) {
        object->mdlId[i] = Hu3DModelLink(object->mdlId[rand8() % 4 + 1]);
    }
    for (i = 1; i < 10; i++) {
        placementAttempts = 0;
        do {
            candidatePositionX = Hu3DData[object->mdlId[i]].pos.x = 50.0f + rand8();
            if (i % 2 != 0) {
                modelTransitionWork[i].startPositionX = Hu3DData[object->mdlId[i]].pos.x *= -1.0f;
            }
            candidatePositionY = Hu3DData[object->mdlId[i]].pos.y = 50.0f + rand8();
            candidatePositionZ = Hu3DData[object->mdlId[i]].pos.z = 750.0f + i * 5;
            for (j = 1; j < 10; j++) {
                if (i != j && modelTransitionWork[j].isActive == 1) {
                    candidatePositionX = Hu3DData[object->mdlId[i]].pos.x - Hu3DData[object->mdlId[j]].pos.x;
                    candidatePositionY = Hu3DData[object->mdlId[i]].pos.y - Hu3DData[object->mdlId[j]].pos.y;
                    candidatePositionZ = Hu3DData[object->mdlId[i]].pos.z - Hu3DData[object->mdlId[j]].pos.z;
                    if (sqrtf(candidatePositionX * candidatePositionX + candidatePositionY * candidatePositionY) <= 100.0f) {
                        break;
                    }
                }
            }
            placementAttempts++;
        } while (j != 10 && placementAttempts < 50);
        modelTransitionWork[i].isActive = 1;
        modelTransitionWork[i].startPositionX = modelTransitionWork[i].targetPositionX = Hu3DData[object->mdlId[i]].pos.x;
        modelTransitionWork[i].startPositionY = modelTransitionWork[i].targetPositionY = Hu3DData[object->mdlId[i]].pos.y;
        modelTransitionWork[i].startPositionZ = modelTransitionWork[i].targetPositionZ = Hu3DData[object->mdlId[i]].pos.z;
        modelTransitionWork[i].targetPositionX = 0.025f * modelTransitionWork[i].startPositionX;
        Hu3DModelScaleSet(object->mdlId[i], 0.5f, 0.5f, 0.5f);
        Hu3DModelLayerSet(object->mdlId[i], 3);
    }
    for (i = 10; i < 140; i++) {
        placementAttempts = 0;
        do {
            candidatePositionX = Hu3DData[object->mdlId[i]].pos.x = rand8() * 2;
            if (i % 2 != 0) {
                Hu3DData[object->mdlId[i]].pos.x *= -1.0f;
            }
            candidatePositionY = Hu3DData[object->mdlId[i]].pos.y = rand8() * 2 + 125;
            candidatePositionZ = Hu3DData[object->mdlId[i]].pos.z = 800.0f + i * 10;
            for (j = 10; j < 140; j++) {
                if (i != j && modelTransitionWork[j].isActive == 1) {
                    candidatePositionX = Hu3DData[object->mdlId[i]].pos.x - Hu3DData[object->mdlId[j]].pos.x;
                    candidatePositionY = Hu3DData[object->mdlId[i]].pos.y - Hu3DData[object->mdlId[j]].pos.y;
                    candidatePositionZ = Hu3DData[object->mdlId[i]].pos.z - Hu3DData[object->mdlId[j]].pos.z;
                    if (sqrtf(candidatePositionX * candidatePositionX + candidatePositionY * candidatePositionY + candidatePositionZ * candidatePositionZ) <= 100.0f) {
                        break;
                    }
                }
            }
            placementAttempts++;
        } while (j != 140 && placementAttempts < 50);
        modelTransitionWork[i].isActive = 1;
        Hu3DModelLayerSet(object->mdlId[i], 3);
        modelTransitionWork[i].startPositionX = modelTransitionWork[i].targetPositionX = Hu3DData[object->mdlId[i]].pos.x;
        modelTransitionWork[i].startPositionY = modelTransitionWork[i].targetPositionY = Hu3DData[object->mdlId[i]].pos.y;
        modelTransitionWork[i].startPositionZ = modelTransitionWork[i].targetPositionZ = Hu3DData[object->mdlId[i]].pos.z;
        if (rand8() % 2 == 0) {
            modelTransitionWork[i].targetPositionX += rand8() % 30;
        }
        else {
            modelTransitionWork[i].targetPositionX -= rand8() % 30;
        }
        modelTransitionWork[i].targetPositionY += rand8() % 30 + 10;
        if (rand8() % 2 == 0) {
            modelTransitionWork[i].targetPositionZ += rand8() % 30;
        }
        else {
            modelTransitionWork[i].targetPositionZ -= rand8() % 30;
        }
        modelTransitionWork[i].horizontalOffset = modelTransitionWork[i].unreferencedMotionScalar = modelTransitionWork[i].unreferencedMotionScale = 0.0f;
        modelTransitionWork[i].animationDuration = 360 + (rand8() * 2) % 360;
        modelTransitionWork[i].animationFrame = rand8();
    }
    for (i = 1; i < 140; i++) {
        Hu3DModelAttrSet(object->mdlId[i], HU3D_ATTR_DISPOFF);
    }
    object->objFunc = UpdatePlayerMenuObject;
}

float InterpolateCubicValue(float startValue, float targetValue, float frame, float duration)
{
    if (duration <= frame) {
        return targetValue;
    }
    return startValue + (frame / duration) * (targetValue - startValue);
}

float ApproachValue(float startValue, float targetValue, float frame)
{
    return (targetValue + startValue * (frame - 1.0f)) / frame;
}

float InterpolateValue(float startValue, float targetValue, float frame, float duration)
{
    if (frame >= duration) {
        return targetValue;
    }
    return startValue + (targetValue - startValue) * (1.0 - cosd((90.0f / duration) * frame));
}

float InterpolateValueClamped(float startValue, float targetValue, float frame, float duration)
{
    if (frame >= duration) {
        return targetValue;
    }
    return startValue + (targetValue - startValue) * sind((90.0f / duration) * frame);
}

float InterpolateSmoothValue(float startValue, float targetValue, float frame, float duration)
{
    if (frame >= duration) {
        return startValue;
    }
    return startValue + (targetValue - startValue) * sind((360.0f / duration) * frame);
}

void WaitForMenuFrames(s32 value)
{
    HuPrcSleep(value);
}

void UpdateMenuCamera(void)
{
    HuPrcVSleep();
}

void ResetMenuCamera(void)
{
    while (TRUE) {
        HuPrcVSleep();
    }
}

void InitializeMenuCamera(void)
{
    s32 lightId;

    lightId = 0;
    lightId = Hu3DGLightCreate(0.0f, 700.0f, 1200.0f, 0.0f, -1.0f, 0.0f, 0xFF, 0xFF, 0xFF);
    Hu3DGLightPosAimSet(lightId, 0.0f, 700.0f, 1200.0f, 0.0f, 0.0f, 640.0f);
    Hu3DGLightInfinitytSet(lightId);
}

void SetMenuCameraEnabled(s32 value)
{
    Vec shadowCameraPosition = { 0.0f, 3000.0f, 800.0f };
    Vec shadowTargetPosition = { 0.0f, 0.0f, 640.0f };
    Vec shadowUpDirection = { 0.0f, 1.0f, 0.0f };

    if (value == 3) {
        shadowCameraPosition.x = shadowTargetPosition.x = 1200.0f;
    }
    Hu3DShadowCreate(45.0f, 2000.0f, 25000.0f);
    Hu3DShadowTPLvlSet(0.8f);
    Hu3DShadowPosSet(&shadowCameraPosition, &shadowUpDirection, &shadowTargetPosition);
}

void InitializeMenuModels(void)
{
    HuWinInit(1);
}

void AnimateWindowPosition(s32 windowId, float targetValue, float frame, s32 waitFrames)
{
    WindowData *windowData;
    float windowStartX;
    float windowStartY;
    float windowWidth;
    float windowHeight;
    float windowTargetX;
    float windowTargetY;
    float windowTargetScaleX;
    float windowTargetScaleY;
    float windowPositionX;
    float windowPositionY;
    float windowScaleX;
    float windowScaleY;
    s32 i;

    windowData = &winData[windowId];
    windowStartX = windowData->pos_x;
    windowStartY = windowData->pos_y;
    windowWidth = windowData->w;
    windowHeight = windowData->h;
    windowTargetX = windowStartX + windowWidth * targetValue;
    windowTargetScaleX = 4.0f / windowWidth;
    if (windowTargetX + 4.0f - (windowStartX + windowWidth) >= 0.0f) {
        windowTargetX = windowStartX + windowWidth - 4.0f;
    }
    else {
        if (windowStartX - (windowTargetX - 2.0f) >= 0.0f) {
            windowTargetX = windowStartX;
        }
        else {
            windowTargetX = windowTargetX - 2.0f;
        }
    }
    windowTargetY = windowStartY + windowHeight * frame;
    windowTargetScaleY = 4.0f / windowHeight;
    if (windowTargetY + 4.0f - (windowStartY + windowHeight) >= 0.0f) {
        windowTargetY = windowStartY + windowHeight - 4.0f;
    }
    else {
        if (windowStartY - (windowTargetY - 2.0f) >= 0.0f) {
            windowTargetY = windowStartY;
        }
        else {
            windowTargetY = windowTargetY - 2.0f;
        }
    }
    if (waitFrames != 0) {
        HuWinPosSet(windowId, windowTargetX, windowTargetY);
        HuWinScaleSet(windowId, windowTargetScaleX, windowTargetScaleY);
        windowPositionX = windowTargetX;
        windowScaleX = windowTargetScaleX;
        windowPositionY = windowTargetY;
        windowScaleY = windowTargetScaleY;
    }
    else {
        HuWinPosSet(windowId, windowStartX, windowStartY);
        HuWinScaleSet(windowId, 1.0f, 1.0f);
        windowPositionX = windowStartX;
        windowScaleX = 1.0f;
        windowPositionY = windowStartY;
        windowScaleY = 1.0f;
    }
    HuWinDispOn(windowId);
    for (i = 0; i <= 15; i++) {
        HuPrcVSleep();
        if (waitFrames != 0) {
            if (i <= 10) {
                windowPositionX = InterpolateValueClamped(startValue, targetValue, frame, 10.0f);
                windowScaleX = InterpolateValueClamped(startValue, 1.0f, frame, 10.0f);
            }
            else {
                windowPositionY = InterpolateValueClamped(startValue, targetValue, i - 10, 5.0f);
                windowScaleY = InterpolateValueClamped(startValue, 1.0f, i - 10, 5.0f);
            }
        }
        else {
            if (i <= 10) {
                windowPositionY = InterpolateValueClamped(startValue, targetValue, frame, 10.0f);
                windowScaleY = InterpolateValueClamped(1.0f, targetValue, frame, 10.0f);
            }
            else {
                windowPositionX = InterpolateValueClamped(startValue, targetValue, i - 10, 5.0f);
                windowScaleX = InterpolateValueClamped(1.0f, targetValue, i - 10, 5.0f);
            }
        }
        HuWinPosSet(windowId, windowPositionX, windowPositionY);
        HuWinScaleSet(windowId, windowScaleX, windowScaleY);
    }
    if (waitFrames != 0) {
        HuWinPosSet(windowId, windowStartX, windowStartY);
        HuWinScaleSet(windowId, 1.0f, 1.0f);
    }
    else {
        HuWinPosSet(windowId, windowTargetX, windowTargetY);
        HuWinScaleSet(windowId, windowTargetScaleX, windowTargetScaleY);
    }
    HuPrcVSleep();
}

s32 CreateMessageWindow(s32 windowId, s32 messageId, s32 choiceIndex, s32 waitFrames, float progress, float speed, s32 insertId, s32 insertId8, s32 insertId9)
{
    float messageSize[2];
    s32 windowId;

    if (insertId % 2 == 0) {
        choiceIndex = choiceIndex * 21 + 16;
        waitFrames = waitFrames * 26 + 16;
        messageId = 445.0f - waitFrames;
        switch (windowId) {
            case -1:
                windowId = 16;
                break;
            case 0:
                windowId = 1.0f + (576.0f - choiceIndex) / 2;
                break;
            case 1:
                windowId = 556.0f - choiceIndex;
                break;
        }
    }
    else {
        HuWinMesMaxSizeGet(1, messageSize, insertId9);
        choiceIndex = messageSize[0];
        waitFrames = messageSize[1];
        windowId = 556.0f - choiceIndex;
        messageId = 445.0f - waitFrames;
    }
    windowId = HuWinExCreateStyled(windowId, messageId, choiceIndex, waitFrames, -1, 1);
    if (insertId == 1) {
        HuWinAttrSet(windowId, 0x10);
    }
    HuWinMesPalSet(windowId, 7, 0, 0, 0);
    winData[windowId].active_pad = 1;
    AnimateWindowPosition(windowId, targetValue, frame, 1);
    return windowId;
}

s32 CreateStandardMessageWindow(s32 windowId, s32 messageId, s32 choiceIndex)
{
    return CreateMessageWindow(windowId, 0, 21, 2, 0.5f, 0.5f, 0, 0, 0);
}

void CloseMessageWindow(s32 windowId, float targetValue, float frame)
{
    HuWinMesSet(windowId, MAKE_MESSID(37, 4));
    HuPrcVSleep();
    AnimateWindowPosition(windowId, targetValue, frame, 0);
    HuWinExCleanup(windowId);
}

void CloseStandardMessageWindow(s32 windowId)
{
    CloseMessageWindow(windowId, 0.5f, 0.5f);
}

void WaitForMessageWindow(s32 windowId, s32 messageId)
{
    if (messageId == -999 || messageId == 0) {
        return;
    }
    if (messageId == -1000) {
        do {
            HuPrcVSleep();
        } while (HuWinStatGet(windowId) == 1);
        return;
    }
    if (messageId > 0) {
        HuPrcSleep(messageId);
        return;
    }
    HuWinComKeyReset();
    HuWinMesWait(windowId);
    HuWinComKeyReset();
}

void WaitForMessageChoice(s32 windowId, s32 messageId, s32 choiceIndex, s32 waitFrames)
{
    s32 i;
    s32 keyWaitCount;
    s32 keyWaitState;

    keyWaitCount = 0;
    keyWaitState = 0;
    keyWaitCount = HuWinKeyWaitNumGet(messageId);
    if (choiceIndex == -1 || choiceIndex >= keyWaitCount - 1) {
        choiceIndex = keyWaitCount - 1;
    }
    HuWinComKeyReset();
    for (i = 0; i < keyWaitCount; i++) {
        keyWaitState = 0;
        do {
            HuPrcVSleep();
        } while (HuWinStatGet(windowId) != 1);
        if (i == choiceIndex) {
            HuWinAttrSet(windowId, 0x400);
            keyWaitState = 1;
        }
        while (TRUE) {
            HuPrcVSleep();
            if (keyWaitState != 0) {
                if (keyWaitState == 1 && HuWinStatGet(windowId) != 1) {
                    keyWaitState = 2;
                }
                else if (keyWaitState == 2) {
                    HuPrcSleep(waitFrames);
                    HuWinAttrReset(windowId, 0x400);
                    HuWinKeyWaitEntry(windowId);
                    break;
                }
            }
            else if (HuWinStatGet(windowId) != 1) {
                break;
            }
        }
    }
    WaitForMessageWindow(windowId, -1); // The second arg could be any negative value different from -999 or -1000.
    HuWinComKeyReset();
}

s32 GetMessageChoice(s32 windowId)
{
    s32 choiceResult;

    HuWinComKeyReset();
    choiceResult = HuWinChoiceGet(windowId, 0);
    HuWinComKeyReset();
    return choiceResult;
}

void SetMessageInsert(s32 windowId, s32 messageId, s32 choiceIndex)
{
    HuWinInsertMesSet(windowId, messageId, choiceIndex);
}

void ShowMessageAndWait(s32 windowId, s32 messageId, s32 choiceIndex, s32 waitFrames)
{
    if (waitFrames == -999) {
        HuWinMesSpeedSet(windowId, 0);
    }
    else {
        HuWinMesSpeedSet(windowId, 1);
    }
    HuWinMesSet(windowId, messageId);
    if (choiceIndex == -1) {
        WaitForMessageWindow(windowId, messageId);
    }
    else {
        WaitForMessageChoice(windowId, messageId, choiceIndex, waitFrames);
    }
}

s32 ShowMessageChoiceWindow(s32 windowId, s32 messageId, s32 choiceIndex)
{
    s32 choiceResult;
    s32 windowId;

    choiceResult = 0;
    windowId = CreateMessageWindow(0, 0, 0, 0, 0.5f, 0.5f, insertId, 0, insertId9);
    HuWinMesSet(windowId, windowId);
    choiceResult = GetMessageChoice(windowId);
    CloseMessageWindow(windowId, 0.5f, 0.5f);
    return choiceResult;
}

s32 CreateCenteredMessageWindow(s32 windowId)
{
    float messageSize[2];
    float messageWidth;
    float messageHeight;
    float windowX;
    float windowY;
    s32 messageWindowId;

    HuWinMesMaxSizeGet(1, messageSize, windowId);
    messageWidth = messageSize[0];
    messageHeight = messageSize[1];
    windowX = (576.0f - messageWidth) / 2;
    windowY = 385.0f - messageHeight;
    if (sharedMessageWindowId == -1) {
        messageWindowId = sharedMessageWindowId = HuWinExCreateStyled(windowX, windowY, messageWidth, messageHeight, -1, 1);
    }
    else {
        CloseCenteredMessageWindow(windowId);
        messageWindowId = sharedMessageWindowId = HuWinExCreateStyled(windowX, windowY, messageWidth, messageHeight, -1, 1);
    }
    HuWinBGTPLvlSet(sharedMessageWindowId, 0.0f);
    HuWinMesSet(sharedMessageWindowId, windowId);
    HuWinMesSpeedSet(sharedMessageWindowId, 0);
    HuWinDispOn(sharedMessageWindowId);
    return sharedMessageWindowId;
}

void CloseCenteredMessageWindow(s32 windowId)
{
    if (sharedMessageWindowId != -1) {
        HuWinExCleanup(sharedMessageWindowId);
        sharedMessageWindowId = -1;
    }
}

void SetModelMotionWithMode(s32 modeType, s32 selection, s32 optionIndex, s32 eventType)
{
    switch (eventType) {
        case 0:
            Hu3DMotionShiftSet(modeType, selection, 0.0f, optionIndex, HU3D_MOTATTR_NONE);
            break;
        case 1:
            Hu3DMotionShiftSet(modeType, selection, 0.0f, optionIndex, HU3D_MOTATTR_LOOP);
            break;
        case 2:
            Hu3DMotionShiftSet(modeType, selection, 0.0f, optionIndex, HU3D_MOTATTR_PAUSE);
            break;
    }
}

void AnimateModelAlongPath(s32 modeType, Vec targetPosition, float frame, float duration, float progress, s32 state, s32 state7)
{
    Vec modelTargetRotation;
    Vec modelStartPosition;
    Vec modelTargetPosition;
    Vec modelStartRotation;
    Vec modelCurrentRotation;
    Vec modelMoveOffset;
    s32 unreferencedMotionState;
    s32 unreferencedRotationState;
    s32 i;

    unreferencedMotionState = 0;
    unreferencedRotationState = 0;
    modelStartPosition.x = Hu3DData[modeType].pos.x;
    modelStartPosition.y = Hu3DData[modeType].pos.y;
    modelStartPosition.z = Hu3DData[modeType].pos.z;
    modelTargetPosition.x = targetPosition.x;
    modelTargetPosition.y = targetPosition.y;
    modelTargetPosition.z = targetPosition.z;
    modelCurrentRotation.x = Hu3DData[modeType].rot.x;
    modelCurrentRotation.y = Hu3DData[modeType].rot.y;
    modelCurrentRotation.z = Hu3DData[modeType].rot.z;
    modelMoveOffset.x = 0.0f;
    modelMoveOffset.y = frame;
    modelMoveOffset.z = 0.0f;
    if (modelTargetPosition.x - modelStartPosition.x >= 0.0f) {
        unreferencedMotionState = 1;
    }
    else {
        unreferencedMotionState = 0;
    }
    if (modelTargetPosition.z - modelStartPosition.z >= 0.0f) {
        unreferencedRotationState = 1;
    }
    else {
        unreferencedRotationState = 0;
    }
    while (state != 0) {
        HuPrcVSleep();
        modelStartRotation.y = -(atan2d(modelTargetPosition.z - modelStartPosition.z, modelTargetPosition.x - modelStartPosition.x) - 90.0);
        if (modelStartRotation.y < 0.0f) {
            modelStartRotation.y += 360.0f;
        }
        else if (modelStartRotation.y >= 360.0f) {
            modelStartRotation.y -= 360.0f;
        }
        if (state7 != 0) {
            if (modelStartRotation.y >= 180.0f) {
                if (modelCurrentRotation.y - modelStartRotation.y >= 180.0f) {
                    modelCurrentRotation.y -= 360.0f;
                }
                if (modelCurrentRotation.y - modelStartRotation.y < -180.0f) {
                    modelCurrentRotation.y += 360.0f;
                }
            }
            else {
                if (modelCurrentRotation.y - modelStartRotation.y > 180.0f) {
                    modelCurrentRotation.y -= 360.0f;
                }
                if (modelCurrentRotation.y - modelStartRotation.y <= -180.0f) {
                    modelCurrentRotation.y += 360.0f;
                }
            }
        }
        modelTargetRotation.x = modelStartPosition.x += duration * sind(modelStartRotation.y);
        modelTargetRotation.z = modelStartPosition.z += duration * cosd(modelStartRotation.y);
        if (unreferencedMotionState == 1) {
            if (modelStartPosition.x >= modelTargetPosition.x) {
                modelTargetRotation.x = modelStartPosition.x = modelTargetPosition.x;
                unreferencedMotionState = 2;
            }
        }
        else if (unreferencedMotionState == 0) {
            if (modelStartPosition.x <= modelTargetPosition.x) {
                modelTargetRotation.x = modelStartPosition.x = modelTargetPosition.x;
                unreferencedMotionState = 2;
            }
        }
        if (unreferencedRotationState == 1) {
            if (modelStartPosition.z >= modelTargetPosition.z) {
                modelTargetRotation.z = modelStartPosition.z = modelTargetPosition.z;
                unreferencedRotationState = 2;
            }
        }
        else if (unreferencedRotationState == 0) {
            if (modelStartPosition.z <= modelTargetPosition.z) {
                modelTargetRotation.z = modelStartPosition.z = modelTargetPosition.z;
                unreferencedRotationState = 2;
            }
        }
        if (state7 != 0) {
            modelCurrentRotation.y = ApproachValue(modelCurrentRotation.startValue, modelStartRotation.targetValue, frame);
        }
        Hu3DModelPosSet(modeType, modelTargetRotation.x, modelStartPosition.y, modelTargetRotation.z);
        Hu3DModelRotSet(modeType, modelCurrentRotation.x, modelCurrentRotation.y, modelCurrentRotation.z);
        if (unreferencedMotionState == 2 && unreferencedRotationState == 2) {
            break;
        }
    }
    modelCurrentRotation.y = Hu3DData[modeType].rot.y;
    if (state7 != 0) {
        for (i = 0; i <= progress; i++) {
            HuPrcVSleep();
            if (modelMoveOffset.y >= 180.0f) {
                if (modelCurrentRotation.y - modelMoveOffset.y >= 180.0f) {
                    modelCurrentRotation.y -= 360.0f;
                }
                if (modelCurrentRotation.y - modelMoveOffset.y < -180.0f) {
                    modelCurrentRotation.y += 360.0f;
                }
            }
            else {
                if (modelCurrentRotation.y - modelMoveOffset.y > 180.0f) {
                    modelCurrentRotation.y -= 360.0f;
                }
                if (modelCurrentRotation.y - modelMoveOffset.y <= -180.0f) {
                    modelCurrentRotation.y += 360.0f;
                }
            }
            modelMoveOffset.z = InterpolateCubicValue(modelCurrentRotation.startValue, modelMoveOffset.targetValue, frame, duration);
            Hu3DModelRotSet(modeType, modelCurrentRotation.x, modelMoveOffset.z, modelCurrentRotation.z);
        }
    }
}

void AnimateSpritePosition(s32 spriteGroupId, s32 spriteIndex, float frame, float duration)
{
    HuSprPosSet(spriteGroupId, spriteIndex, frame, duration + 0.5f);
}

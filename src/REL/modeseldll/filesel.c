#include "dolphin/card.h"
#include "game/audio.h"
#include "game/card.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/saveload.h"
#include "game/window.h"
#include "game/wipe.h"

#include "ext_math.h"

#include "REL/modeseldll.h"

s32 msmSysGetOutputMode(void);

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/hsfex.h"
#endif

s16 introSoundEffectId = -1;
float fileSlotCameraPositions[] = { -55, 0, 0, 0, 60, -57, 806, -71, 0, 0, 0, 100, -131, 900, -87, 0, 0, 0, 46, -117, 566, -87, 0, 0, 0, 46, -117, 566, -90, 0,
    0, -22, 39, -153, 432, -40, 0, 0, 0, 130, -186, 6530, -23, 0, 0, -9, 1, -70, 1318, -25, 0, 0, 0, 242, 90, 343, -30, 0, 0, 0, 223, -60, 505, -23,
    0, 0, 0, 65, -97, 1318 };

s32 saveSlotStarAnimationData[] = {
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x0D),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x0E),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x0F),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x10),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x11),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x12),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x13),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x14),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x15),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x16),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x17),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x18),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x19),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x1A),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x1B),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x1C),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x1D),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x1E),
};

char *saveFileMessageTokens[] = { "\x0E\x16", "\x0E\x17" };

char *memoryCardSlotNames[] = { "A", "B" };

char *saveSlotNames[] = { "1", "2", "3" };

s32 selectedPlayerMask;
s16 fileSelectionWindowId;
s16 saveSlotSelectionWindowId;
s16 saveSlotIsOccupied[3];
s16 saveSlotPrimaryStarAnimationIds[3];
s16 saveSlotSecondaryStarAnimationIds[3];
s32 fileSlotLabelsVisible[2];
s32 saveSlotModelsVisible[3];
s16 highlightedFileSlot;
s16 saveSlotStarSpriteGroups[3];
s16 saveSlotDateSpriteGroups[3];
float fileSlotRotationSpeeds[2];
float fileSlotRotationAngles[2];
ANIMDATA *saveSlotStarAnimation;
ANIMDATA *saveSlotDigitAnimation;
ANIMDATA *saveSlotOverflowAnimation;
s16 saveSlotStarCounts[3];
s16 saveSlotStoryCharacters[3];
OSTime saveSlotCreationTimes[3];
s32 saveSlotStoryContinueFlags[3];
s32 saveSlotPartyContinueFlags[3];
s32 saveSlotStoryBoardIndices[3];
s16 saveSlotStoryBoardIds[3];
s16 saveSlotPartyBoardIds[3];
s16 saveSlotCurrentTurns[3];
s16 saveSlotMaxTurns[3];
s16 copySourceSaveSlot;
float fileSelectionRotationAngle;
s16 occupiedSaveSlotCount;

s32 SelectMemoryCard(void);
void InitializeMemoryCardSelectionModels(void);
s32 SelectSaveFile(void);
void UpdateSaveSlotDisplay(void);

s32 RunFileSelection(void)
{
    s16 i;
    s16 result;
    WindowData *window;
    s16 fxstat;
    HUPROCESS *process = HuPrcCurrentGet();
    HUPROCESS *child;
    float winSize[2];
    for (i = 0; i < 3; i++) {
        saveSlotPrimaryStarAnimationIds[i] = saveSlotSecondaryStarAnimationIds[i] = -1;
        saveSlotStarSpriteGroups[i] = -1;
        saveSlotDateSpriteGroups[i] = -1;
    }
    saveSlotStarAnimation = saveSlotDigitAnimation = saveSlotOverflowAnimation = NULL;
    curSlotNo = 0;
    SetModeSelectionCamera(0);
    InitializeMemoryCardSelectionModels();
    child = HuPrcChildCreate(UpdateSaveSlotDisplay, 10, 8192, 0, process);
    winSize[0] = 457;
    winSize[1] = 94;
    fileSelectionWindowId = HuWinExCreateStyled(-10000, 448 - winSize[1], winSize[0], winSize[1], -1, 0);
    window = &winData[fileSelectionWindowId];
    window->active_pad = 1;
    HuWinPushKeySet(fileSelectionWindowId, 0x300);
    HuWinMesSpeedSet(fileSelectionWindowId, 0);
    HuWinDispOn(fileSelectionWindowId);
    fxstat = HuAudFXPlay(2098);
    introSoundEffectId = fxstat;
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
    while (WipeStatGet()) {
        HuPrcVSleep();
    }
    HuAudSeqPlay(43);
    while (1) {
        result = SelectMemoryCard();
        if (!result) {
            break;
        }
        if (!SLSaveFlagGet()) {
            GWGameStatReset();
            #if VERSION_PAL
            _ClearFlag(0x1000B);
            #endif
            GWGameStat.sound_mode = msmSysGetOutputMode();
            result = 1;
            break;
        }
        else {
            if (SelectSaveFile()) {
                break;
            }
        }
    }
    if (result == 0) {
        HuAudFXStop(fxstat);
    }
    HuWinExAnimOut(fileSelectionWindowId);
    HuWinExCleanup(fileSelectionWindowId);
    HuPrcKill(child);
    HuCardUnMount(0);
    HuCardUnMount(1);
    return result;
}

#define GET_ZEROSIGN(x) (((x) == 0) ? (-1) : (1))

s32 ValidateMemoryCards(void);
void ConfigureModelTransparency(s16 modelId);
void StartFileSlotSelectionAnimation(s16 fileSlotIndex, s32 selectionActive);
s32 GetAvailableMemoryCardMask(void);
void ShowSaveSlotMessage(u32 messageId);
void CloseSaveSlotMessage(void);

s32 SelectMemoryCard(void)
{
    s16 selectedBox;
    s16 animationFrame;
    s32 errorF;
    s32 cardResult;
    s16 choiceResult;
    s16 previousBox;
    s16 menuChoice;
    s16 windowIndex;
    WindowData *messageWindow;
    float transitionProgress;
    CARDFileInfo cardFileInfo;
    Vec screenPosition;
    Vec worldPosition;
    Vec secondWorldPosition;
    float messageWindowSize[2];

    SetModeSelectionCamera(0);
    messageWindow = &winData[fileSelectionWindowId];
    HuWinMesMaxSizeGet(1, messageWindowSize, MAKE_MESSID(0x10, 0x0C));
    saveSlotSelectionWindowId = HuWinExCreateStyled(-10000.0f, 316.0f, messageWindowSize[0], messageWindowSize[1], -1, 0);
    HuWinBGTPLvlSet(saveSlotSelectionWindowId, 0);
    HuWinMesSpeedSet(saveSlotSelectionWindowId, 0);
repeat:
    screenPosition.x = 188;
    screenPosition.y = 250;
    screenPosition.z = 600;
    Hu3D2Dto3D(&screenPosition, 1, &worldPosition);
    Hu3DModelPosSetV(modeSelectModelIds[2], &worldPosition);
    Hu3DModelPosSetV(modeSelectModelIds[4], &worldPosition);
    Hu3DModelPosSetV(modeSelectModelIds[0], &worldPosition);
    screenPosition.x = 388;
    Hu3D2Dto3D(&screenPosition, 1, &worldPosition);
    Hu3DModelPosSetV(modeSelectModelIds[3], &worldPosition);
    Hu3DModelPosSetV(modeSelectModelIds[5], &worldPosition);
    Hu3DModelPosSetV(modeSelectModelIds[1], &worldPosition);
    Hu3DModelAttrReset(modeSelectModelIds[2], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(modeSelectModelIds[4], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(modeSelectModelIds[3], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(modeSelectModelIds[5], HU3D_ATTR_DISPOFF);
    Hu3DModelRotSet(modeSelectModelIds[2], 0, 0, 0);
    Hu3DModelRotSet(modeSelectModelIds[3], 0, 0, 0);
    ConfigureModelTransparency(modeSelectModelIds[4]);
    ConfigureModelTransparency(modeSelectModelIds[5]);
    fileSlotLabelsVisible[0] = fileSlotLabelsVisible[1] = 1;
    fileSlotRotationSpeeds[0] = fileSlotRotationSpeeds[1] = fileSlotRotationAngles[0] = fileSlotRotationAngles[1] = 1.0f;
    highlightedFileSlot = -1;
    GetAvailableMemoryCardMask();
    for (animationFrame = 0; animationFrame <= 20; animationFrame++) {
        transitionProgress = animationFrame / 20.0;
        transitionProgress = sind(90.0f * transitionProgress);
        Hu3DModelScaleSet(modeSelectModelIds[0], transitionProgress, transitionProgress, transitionProgress);
        Hu3DModelScaleSet(modeSelectModelIds[1], transitionProgress, transitionProgress, transitionProgress);
        Hu3DModelScaleSet(modeSelectModelIds[2], transitionProgress, transitionProgress, transitionProgress);
        Hu3DModelScaleSet(modeSelectModelIds[3], transitionProgress, transitionProgress, transitionProgress);
        Hu3DModelScaleSet(modeSelectModelIds[4], transitionProgress, transitionProgress, transitionProgress);
        Hu3DModelScaleSet(modeSelectModelIds[5], transitionProgress, transitionProgress, transitionProgress);
        HuPrcVSleep();
    }
    while (1) {
    repeatLoop:
        HuWinDispOff(saveSlotSelectionWindowId);
        cardResult = ValidateMemoryCards();
        if (cardResult == 0 && GetAvailableMemoryCardMask() == 0) {
            if (GetAvailableMemoryCardMask() == 0) {
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
                HuWinMesWait(fileSelectionWindowId);
            }
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4F));
            HuWinAttrSet(fileSelectionWindowId, 0x10);
            HuWinMesWait(fileSelectionWindowId);
            choiceResult = HuWinChoiceGet(fileSelectionWindowId, 1);
            if (choiceResult == 0) {
                SLSaveFlagSet(0);
                errorF = 1;
                goto cleanup;
            }
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x52), 0);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4C));
            HuWinMesWait(fileSelectionWindowId);
            while (!(HuPadBtnDown[0] & PAD_BUTTON_A)) {
                HuPrcVSleep();
            }
        }
        while (!GetAvailableMemoryCardMask()) {
            cardResult = ValidateMemoryCards();
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
            HuWinMesWait(fileSelectionWindowId);
            if (messageWindow->key_down & PAD_BUTTON_B) {
                HuAudFXPlay(3);
                return 0;
            }
            if (GetAvailableMemoryCardMask()) {
                break;
            }
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4B));
            HuWinAttrSet(fileSelectionWindowId, 0x10);
            HuWinMesWait(fileSelectionWindowId);
            choiceResult = HuWinChoiceGet(fileSelectionWindowId, 1);
            if (choiceResult == 0) {
                SLSaveFlagSet(0);
                errorF = 1;
                goto cleanup;
            }
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x52), 0);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4C));
            HuWinMesWait(fileSelectionWindowId);
            while (!(HuPadBtnDown[0] & PAD_BUTTON_A)) {
                HuPrcVSleep();
            }
        }
        SLSaveFlagSet(1);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x01));
        HuWinMesWait(fileSelectionWindowId);
        HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x0C));
        HuWinDispOn(saveSlotSelectionWindowId);
        if (curSlotNo == 0) {
            if (selectedPlayerMask & 0x1) {
                selectedBox = 0;
            }
            else {
                selectedBox = 1;
            }
        }
        else {
            if (selectedPlayerMask & 0x2) {
                selectedBox = 1;
            }
            else {
                selectedBox = 0;
            }
        }
        StartFileSlotSelectionAnimation(selectedBox, 1);
        HuPrcSleep(10);
        windowIndex = selectedBox;
        while (1) {
            if (!GetAvailableMemoryCardMask()) {
                StartFileSlotSelectionAnimation(selectedBox, 0);
                HuPrcSleep(10);
                goto repeatLoop;
            }
            else {
                if ((HuPadDStkRep[0] & 0x1) && (selectedPlayerMask & 0x1)) {
                    selectedBox = 0;
                }
                else if ((HuPadDStkRep[0] & 0x2) && (selectedPlayerMask & 0x2)) {
                    selectedBox = 1;
                }
                else if (!(selectedPlayerMask & (1 << selectedBox))) {
                    if (!(selectedPlayerMask & (1 << ((selectedBox + 1) & 0x1)))) {
                        StartFileSlotSelectionAnimation(selectedBox, 0);
                        HuPrcSleep(10);
                        goto repeatLoop;
                    }
                    else {
                        selectedBox = (selectedBox + 1) & 0x1;
                    }
                }
                if (windowIndex != selectedBox) {
                    HuAudFXPlay(0);
                    StartFileSlotSelectionAnimation(selectedBox, 1);
                    StartFileSlotSelectionAnimation((selectedBox + 1) & 0x1, 0);
                    HuPrcSleep(10);
                    windowIndex = selectedBox;
                }
                if (HuPadBtnDown[0] & PAD_BUTTON_A) {
                    HuAudFXPlay(2096);
                    goto loopExit;
                }
                else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
                    HuAudFXPlay(3);
                    return 0;
                }
            }
            HuPrcVSleep();
        }
    }
loopExit:
    HuWinHomeClear(fileSelectionWindowId);
    previousBox = (selectedBox + 1) & 0x1;
    Hu3DMotionSet(modeSelectModelIds[selectedBox + 4], modeSelectMotionIds[7]);
    Hu3DMotionSpeedSet(modeSelectModelIds[selectedBox + 4], 2.0f);
    fileSlotLabelsVisible[0] = fileSlotLabelsVisible[1] = 0;
    highlightedFileSlot = selectedBox;
    SLCurSlotNoSet(selectedBox);
    cardResult = HuCardMount(selectedBox);
    if (cardResult == CARD_RESULT_READY) {
        SLSerialNoGet();
        cardResult = HuCardOpen(selectedBox, SaveFileNameTbl[0], &cardFileInfo);
        if (cardResult != 0 && cardResult != -4) {
            errorF = 1;
        }
        else {
            errorF = 0;
        }
        HuCardClose(&cardFileInfo);
    }
    else {
        if (cardResult == CARD_RESULT_NOCARD) {
            goto repeat;
        }
        errorF = 1;
    }
    choiceResult = HuCardSectorSizeGet(curSlotNo);
    if (choiceResult > 0 && choiceResult != 8192) {
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x39));
        HuWinMesWait(fileSelectionWindowId);
        errorF = 1;
        goto skipCheck;
    }
    else {
        if (choiceResult == -128) {
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x53));
            HuWinMesWait(fileSelectionWindowId);
            errorF = 1;
            goto skipCheck;
        }
        else {
            fileSelectionRotationAngle += 720.0f;
            for (animationFrame = 0; animationFrame <= 40; animationFrame++) {
                if (animationFrame <= 30) {
                    transitionProgress = animationFrame / 30.0f;
                    screenPosition.x = 288.0 + (GET_ZEROSIGN(previousBox) * (100.0 + (250.0 * sind(90.0f * transitionProgress))));
                    screenPosition.y = 250.0 - (100.0 * sind(180.0f * transitionProgress));
                    screenPosition.z = (animationFrame * 100) + 600;
                    Hu3D2Dto3D(&screenPosition, 1, &worldPosition);
                    Hu3DModelPosSetV(modeSelectModelIds[previousBox + 2], &worldPosition);
                    Hu3DModelPosSetV(modeSelectModelIds[previousBox], &worldPosition);
                    Hu3DModelRotSet(modeSelectModelIds[previousBox + 2], animationFrame * 20, animationFrame * 10, 0);
                }

                if (animationFrame > 20) {
                    transitionProgress = (animationFrame - 20) / 20.0f;
                    if (errorF) {
                        screenPosition.x = 288.0f + ((1.0 - transitionProgress) * (100 * GET_ZEROSIGN(selectedBox)));
                        screenPosition.y = 250.0 - (100.0 * sind(180.0f * transitionProgress));
                        screenPosition.z = 600.0f;
                        Hu3D2Dto3D(&screenPosition, 1, &worldPosition);
                        Hu3DModelPosSetV(modeSelectModelIds[selectedBox], &worldPosition);
                        transitionProgress = (0.5 * sind(90.0f * transitionProgress)) + 1.0;
                        Hu3DModelScaleSet(modeSelectModelIds[selectedBox], transitionProgress, transitionProgress, transitionProgress);
                    }
                    else {
                        screenPosition.x = 288.0f + (100 * GET_ZEROSIGN(selectedBox));
                        screenPosition.y = 250;
                        screenPosition.z = 600;
                        Hu3D2Dto3D(&screenPosition, 1, &worldPosition);
                        screenPosition.x = 60;
                        screenPosition.y = 90;
                        screenPosition.z = 600;
                        Hu3D2Dto3D(&screenPosition, 1, &secondWorldPosition);
                        VECSubtract(&secondWorldPosition, &worldPosition, &screenPosition);
                        screenPosition.x *= sind(90.0f * transitionProgress);
                        screenPosition.y *= 1.0 - cosd(90.0f * transitionProgress);
                        screenPosition.z = 0;
                        VECAdd(&screenPosition, &worldPosition, &worldPosition);
                        Hu3DModelPosSetV(modeSelectModelIds[selectedBox], &worldPosition);
                        fileSlotRotationSpeeds[selectedBox] *= cosd(90.0f * transitionProgress);
                        fileSlotRotationAngles[selectedBox] = 1.0 - transitionProgress;
                        transitionProgress = 1.0 + (0.3 * cosd(90.0f * transitionProgress));
                        Hu3DModelScaleSet(modeSelectModelIds[selectedBox], transitionProgress, transitionProgress, transitionProgress);
                    }
                }
                HuPrcVSleep();
            }
            if (errorF == 0) {
                HuWinKill(saveSlotSelectionWindowId);
                return 1;
            }
            HuWinDispOff(saveSlotSelectionWindowId);
            errorF = 0;
            if (cardResult == CARD_RESULT_WRONGDEVICE) {
                HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[selectedBox]), 0);
                #if VERSION_PAL
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x39));
                #else
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x37));
                #endif
                HuWinMesWait(fileSelectionWindowId);
                errorF = 1;
            }
            else if (cardResult == CARD_RESULT_FATAL_ERROR) {
                HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[selectedBox]), 0);
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x53));
                HuWinMesWait(fileSelectionWindowId);
                errorF = 1;
            }
            else if (cardResult == CARD_RESULT_NOCARD) {
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
                HuWinMesWait(fileSelectionWindowId);
                errorF = 1;
            }
            else if (cardResult == CARD_RESULT_BROKEN) {
                UnMountCnt = 0;
                HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[selectedBox]), 0);
                HuWinAttrSet(fileSelectionWindowId, 0x10);
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x04));
                HuWinMesWait(fileSelectionWindowId);
                menuChoice = HuWinChoiceGet(fileSelectionWindowId, 1);
                if (menuChoice == 0) {
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4E));
                    HuWinMesWait(fileSelectionWindowId);
                    menuChoice = HuWinChoiceGet(fileSelectionWindowId, 1);
                    if (menuChoice == 0) {
                        ShowSaveSlotMessage(MAKE_MESSID(16, 0x38));
                        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x0B));
                        HuWinMesWait(fileSelectionWindowId);
                        HuPrcSleep(60);
                        if (UnMountCnt & (1 << selectedBox)) {
                            CloseSaveSlotMessage();
                            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x50));
                            HuWinMesWait(fileSelectionWindowId);
                            errorF = 1;
                            goto cleanup;
                        }
                        else {
                            _SetFlag(0x30000);
                            cardResult = HuCardFormat(selectedBox);
                            _ClearFlag(FLAG_ID_MAKE(3, 0));
                            if (cardResult == CARD_RESULT_READY) {
                                SLSerialNoGet();
                            }
                            CloseSaveSlotMessage();
                            if (cardResult == CARD_RESULT_FATAL_ERROR) {
                                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x36));
                                HuWinMesWait(fileSelectionWindowId);
                                HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
                                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x53));
                                HuWinMesWait(fileSelectionWindowId);
                                errorF = 1;
                            }
                            else if (cardResult == CARD_RESULT_NOCARD) {
                                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
                                HuWinMesWait(fileSelectionWindowId);
                                errorF = 1;
                            }
                        }
                    }
                    else {
                        errorF = 1;
                    }
                }
                else {
                    errorF = 1;
                }
            }
        }
    }
    if (errorF == 0) {
        goto cleanup;
    }
skipCheck:
    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4F));
    HuWinAttrSet(fileSelectionWindowId, 0x10);
    HuWinMesWait(fileSelectionWindowId);
    choiceResult = HuWinChoiceGet(fileSelectionWindowId, 1);
    if (choiceResult == 0) {
        SLSaveFlagSet(0);
        goto cleanup;
    }
    else {
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x52), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4C));
        HuWinMesWait(fileSelectionWindowId);
        while (!(HuPadBtnDown[0] & PAD_BUTTON_A)) {
            HuPrcVSleep();
        }
    }
cleanup:
    HuWinDispOff(saveSlotSelectionWindowId);
    fileSelectionRotationAngle += 720.0f;
    for (animationFrame = 0; animationFrame <= 20; animationFrame++) {
        transitionProgress = animationFrame / 20.0f;
        if (errorF) {
            transitionProgress = 1.5 * cosd(90.0f * transitionProgress);
            Hu3DModelScaleSet(modeSelectModelIds[0], transitionProgress, transitionProgress, transitionProgress);
            Hu3DModelScaleSet(modeSelectModelIds[1], transitionProgress, transitionProgress, transitionProgress);
            Hu3DModelScaleSet(modeSelectModelIds[2], 0.7 * transitionProgress, 0.7 * transitionProgress, 0.7 * transitionProgress);
            Hu3DModelScaleSet(modeSelectModelIds[3], 0.7 * transitionProgress, 0.7 * transitionProgress, 0.7 * transitionProgress);
            if (!SLSaveFlagGet()) {
                espScaleSet(modeSelectSpriteIds[1], transitionProgress, transitionProgress);
            }
        }
        else {
            screenPosition.x = 288;
            screenPosition.y = 250;
            screenPosition.z = 600;
            Hu3D2Dto3D(&screenPosition, 1, &worldPosition);
            screenPosition.x = 60;
            screenPosition.y = 90;
            screenPosition.z = 600;
            Hu3D2Dto3D(&screenPosition, 1, &secondWorldPosition);
            VECSubtract(&secondWorldPosition, &worldPosition, &screenPosition);
            screenPosition.x *= sind(90.0f * transitionProgress);
            screenPosition.y *= 1.0 - cosd(90.0f * transitionProgress);
            screenPosition.z = 0.0f;
            VECAdd(&screenPosition, &worldPosition, &worldPosition);
            Hu3DModelPosSetV(modeSelectModelIds[selectedBox], &worldPosition);
            fileSlotRotationSpeeds[selectedBox] *= cosd(90.0f * transitionProgress);
            fileSlotRotationAngles[selectedBox] = 1.0 - transitionProgress;
            transitionProgress = 1.0 + (0.5 * cosd(90.0f * transitionProgress));
            Hu3DModelScaleSet(modeSelectModelIds[selectedBox], transitionProgress, transitionProgress, transitionProgress);
        }
        HuPrcVSleep();
    }
    if (errorF && SLSaveFlagGet()) {
        goto repeat;
    }
    HuWinKill(saveSlotSelectionWindowId);
    return 1;
}

s32 ValidateMemoryCards(void)
{
    s16 i;
    s16 result;
    s16 ret;
    ret = 1;
    for (i = 0; i < 2; i++) {
        result = HuCardSlotCheck((s32)i);
        if (result == -2) {
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[i]), 0);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x37));
            HuWinMesWait(fileSelectionWindowId);
            ret = 0;
        }
        else if (result > 0 && result != 8192) {
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[i]), 0);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x39));
            HuWinMesWait(fileSelectionWindowId);
            ret = 0;
        }
        else if (result == -128) {
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[i]), 0);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x53));
            HuWinMesWait(fileSelectionWindowId);
            ret = 0;
        }
    }
    return ret;
}

void ConfigureModelTransparency(s16 modelId)
{
    HU3DMODEL *model = &Hu3DData[modelId];
    HSFDATA *hsf = model->hsf;
    HSFMATERIAL *mat = hsf->material;
    s16 i;
    for (i = 0; i < hsf->materialNum; i++, mat++) {
        mat->invAlpha = 0.0f;
        mat->pass &= 0xF0;
    }
}

typedef struct FileSelectionAnimationWork {
    s16 fileSlotIndex;
    s32 selectionActive;
} FileSelectionAnimationWork;

void AnimateFileSlotSelection(void)
{
    s16 index;
    FileSelectionAnimationWork *work;
    s16 i;
    s32 param;
    HUPROCESS *curr;
    curr = HuPrcCurrentGet();
    work = curr->user_data;
    index = work->fileSlotIndex;
    param = work->selectionActive;
    Hu3DModelAttrSet(modeSelectModelIds[index + 2], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(modeSelectModelIds[index + 4], HU3D_ATTR_DISPOFF);
    Hu3DMotionSet(modeSelectModelIds[index + 4], modeSelectMotionIds[6]);
    Hu3DMotionSpeedSet(modeSelectModelIds[index + 4], 2.0f);
    if (param) {
        Hu3DModelAttrReset(modeSelectModelIds[index + 4], HU3D_MOTATTR_REV);
    }
    else {
        Hu3DModelAttrSet(modeSelectModelIds[index + 4], HU3D_MOTATTR_REV);
        Hu3DMotionTimeSet(modeSelectModelIds[index + 4], Hu3DMotionMaxTimeGet(modeSelectModelIds[6]));
    }
    for (i = 0; i < 10; i++) {
        float scale;
        float time = i / 10.0;
        if (param) {
            scale = 1.0 + (0.3 * sind(90.0f * time));
        }
        else {
            scale = 1.0 + (0.3 * cosd(90.0f * time));
        }
        Hu3DModelScaleSet(modeSelectModelIds[index + 2], scale, scale, scale);
        Hu3DModelScaleSet(modeSelectModelIds[index + 4], scale, scale, scale);
        Hu3DModelScaleSet(modeSelectModelIds[index], scale, scale, scale);
        HuPrcVSleep();
    }
    if (!param) {
        Hu3DModelAttrReset(modeSelectModelIds[index + 2], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(modeSelectModelIds[index + 4], HU3D_ATTR_DISPOFF);
    }
    HuMemDirectFree(work);
    HuPrcEnd();
    while (1) {
        HuPrcVSleep();
    }
}

void StartFileSlotSelectionAnimation(s16 fileSlotIndex, s32 selectionActive)
{
    FileSelectionAnimationWork *work;
    HUPROCESS *child;
    HUPROCESS *parent;
    parent = HuPrcCurrentGet();
    child = HuPrcChildCreate(AnimateFileSlotSelection, 50, 8192, 0, parent);
    work = HuMemDirectMallocNum(HEAP_HEAP, sizeof(FileSelectionAnimationWork), HU_MEMNUM_OVL);
    child->user_data = work;
    work->fileSlotIndex = fileSlotIndex;
    work->selectionActive = selectionActive;
}

s32 GetAvailableMemoryCardMask(void)
{
    s32 result;
    selectedPlayerMask = 0;
    result = HuCardSlotCheck(0);
    if (result == 8192) {
        Hu3DModelAttrReset(modeSelectModelIds[0], HU3D_ATTR_DISPOFF);
        selectedPlayerMask |= 1;
    }
    else {
        Hu3DModelAttrSet(modeSelectModelIds[0], HU3D_ATTR_DISPOFF);
    }
    result = HuCardSlotCheck(1);
    if (result == 8192) {
        Hu3DModelAttrReset(modeSelectModelIds[1], HU3D_ATTR_DISPOFF);
        selectedPlayerMask |= 2;
    }
    else {
        Hu3DModelAttrSet(modeSelectModelIds[1], HU3D_ATTR_DISPOFF);
    }
    return selectedPlayerMask;
}

void InitializeMemoryCardSelectionModels(void)
{
    HU3DMODEL *model;
    Vec pos2d;
    Vec pos3d;
    Mtx scale;
    espAttrReset(modeSelectSpriteIds[0], HUSPR_ATTR_DISPOFF);
    espDrawNoSet(modeSelectSpriteIds[0], 127);
    espAttrReset(modeSelectSpriteIds[1], HUSPR_ATTR_DISPOFF);
    Hu3DModelAttrReset(modeSelectModelIds[2], HU3D_ATTR_DISPOFF);
    pos2d.x = 188;
    pos2d.y = 250;
    pos2d.z = 600;
    Hu3D2Dto3D(&pos2d, 1, &pos3d);
    Hu3DModelPosSetV(modeSelectModelIds[2], &pos3d);
    Hu3DModelPosSetV(modeSelectModelIds[4], &pos3d);
    Hu3DModelPosSetV(modeSelectModelIds[0], &pos3d);
    MTXScale(scale, 0.5f, 0.5f, 0.5f);
    model = &Hu3DData[modeSelectModelIds[2]];
    MTXCopy(scale, model->mtx);
    model = &Hu3DData[modeSelectModelIds[4]];
    MTXCopy(scale, model->mtx);
    model = &Hu3DData[modeSelectModelIds[0]];
    MTXCopy(scale, model->mtx);
    Hu3DModelAttrReset(modeSelectModelIds[3], HU3D_ATTR_DISPOFF);
    pos2d.x = 388;
    Hu3D2Dto3D(&pos2d, 1, &pos3d);
    Hu3DModelPosSetV(modeSelectModelIds[3], &pos3d);
    Hu3DModelPosSetV(modeSelectModelIds[5], &pos3d);
    Hu3DModelPosSetV(modeSelectModelIds[1], &pos3d);
    model = &Hu3DData[modeSelectModelIds[3]];
    MTXCopy(scale, model->mtx);
    model = &Hu3DData[modeSelectModelIds[5]];
    MTXCopy(scale, model->mtx);
    model = &Hu3DData[modeSelectModelIds[1]];
    MTXCopy(scale, model->mtx);
    Hu3DModelScaleSet(modeSelectModelIds[0], 0, 0, 0);
    Hu3DModelScaleSet(modeSelectModelIds[1], 0, 0, 0);
    Hu3DModelScaleSet(modeSelectModelIds[2], 0, 0, 0);
    Hu3DModelScaleSet(modeSelectModelIds[3], 0, 0, 0);
    Hu3DModelScaleSet(modeSelectModelIds[4], 0, 0, 0);
    Hu3DModelScaleSet(modeSelectModelIds[5], 0, 0, 0);
    GetAvailableMemoryCardMask();
}

void SetSaveSlotHighlight(s16 boxno, s32 flag);

s32 OpenSaveFile(char *name, s16 slotno);
s32 CreateSaveFile(s16 boxno);
s32 WriteSaveFile(void);
s32 ReadSaveFile(void);
s32 DeleteSaveFile(s16 boxno);
s32 CopySaveFile(s16 boxno);
s32 CloseSaveFile(void);
s32 InitializeSaveSlotDisplay(void);

void LoadSaveSlotMetadata(s16 boxno);
void ShowSaveSlotDetails(s16 boxno);

s32 SelectSaveFile(void)
{
    float scaleProgress;
    s16 selectionResult;
    s16 selectedBox;
    s16 boxIndex;
    s16 modelIndex;
    s32 cardResult;
    s16 boxChange;
    float messageWindowSize[2];
    OSTime saveTimestamp;
    HuWinMesMaxSizeGet(1, messageWindowSize, MAKE_MESSID(0x10, 0x3D));
    saveSlotSelectionWindowId = HuWinExCreateStyled(-10000, 316, messageWindowSize[0], messageWindowSize[1], -1, 0);
    HuWinAttrSet(saveSlotSelectionWindowId, 0x800);
    HuWinBGTPLvlSet(saveSlotSelectionWindowId, 0);
    HuWinMesSpeedSet(saveSlotSelectionWindowId, 0);
    selectedBox = -1;
    for (boxIndex = occupiedSaveSlotCount = 0; boxIndex < 3; boxIndex++) {
        saveSlotStarCounts[boxIndex] = -1;
        saveSlotStoryCharacters[boxIndex] = -1;
        saveSlotIsOccupied[boxIndex] = 0;
        cardResult = OpenSaveFile(SaveFileNameTbl[boxIndex], curSlotNo);
        if (cardResult == 0) {
            cardResult = ReadSaveFile();
            if (cardResult == 0) {
                saveSlotIsOccupied[boxIndex] = 1;
                if (occupiedSaveSlotCount == 0) {
                    selectedBox = boxIndex;
                }
                occupiedSaveSlotCount++;
                if (SLCheckSumCheck()) {
                    LoadSaveSlotMetadata(boxIndex);
                }
            }
            CloseSaveFile();
        }
        if (cardResult != 0 && cardResult != -4) {
            HuWinKill(saveSlotSelectionWindowId);
            return 0;
        }
    }
    InitializeSaveSlotDisplay();
    if (selectedBox == -1) {
        selectedBox = 0;
    }
    saveSlotModelsVisible[0] = saveSlotModelsVisible[1] = saveSlotModelsVisible[2] = 1;
    UnMountCnt = 0;
    for (boxIndex = 0; boxIndex <= 20; boxIndex++) {
        scaleProgress = sind(90.0f * (boxIndex / 20.0f));
        for (modelIndex = 0; modelIndex < 3; modelIndex++) {
            Hu3DModelScaleSet(modeSelectModelIds[modelIndex + 8], scaleProgress, scaleProgress, scaleProgress);
            Hu3DModelScaleSet(modeSelectModelIds[modelIndex + 11], scaleProgress, scaleProgress, scaleProgress);
        }
        HuPrcVSleep();
    }
    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x07));
    HuWinMesWait(fileSelectionWindowId);
    SetSaveSlotHighlight(selectedBox, 1);
    if (occupiedSaveSlotCount == 0) {
        HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x40));
    }
    else {
        if (occupiedSaveSlotCount == 3) {
            HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x3F));
        }
        else {
            HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x3D));
        }
    }
    HuWinDispOn(saveSlotSelectionWindowId);
    ShowSaveSlotDetails(selectedBox);
    if (UnMountCnt & (1 << curSlotNo)) {
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x50));
        HuWinMesWait(fileSelectionWindowId);
        selectionResult = 0;
    }
    else {
        UnMountCnt = 0;
        while (1) {
            HuPrcVSleep();
            if (UnMountCnt & (1 << curSlotNo)) {
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x50));
                HuWinMesWait(fileSelectionWindowId);
                selectionResult = 0;
                break;
            }
            boxChange = 0;
            if (HuPadDStkRep[0] & 0x1) {
                boxChange = -1;
            }
            else if (HuPadDStkRep[0] & 0x2) {
                boxChange = 1;
            }
            if (boxChange) {
                SetSaveSlotHighlight(selectedBox, 0);
                selectedBox += boxChange;
                if (selectedBox < 0) {
                    selectedBox = 2;
                }
                else if (selectedBox >= 3) {
                    selectedBox = 0;
                }
                HuAudFXPlay(0);
                SetSaveSlotHighlight(selectedBox, 1);
                ShowSaveSlotDetails(selectedBox);
                HuPrcSleep(10);
            }
            if ((HuPadBtnDown[0] & PAD_BUTTON_X) && saveSlotIsOccupied[selectedBox]) {
                UnMountCnt = 0;
                selectionResult = OpenSaveFile(SaveFileNameTbl[selectedBox], curSlotNo);
                if (selectionResult < 0) {
                    selectionResult = 0;
                    break;
                }
                if (!saveSlotIsOccupied[selectedBox]) {
                    continue;
                }
                HuWinDispOff(saveSlotSelectionWindowId);
                HuAudFXPlay(2);
                HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(saveSlotNames[selectedBox]), 1);
                HuWinAttrSet(fileSelectionWindowId, 0x10);
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x08));
                HuWinMesWait(fileSelectionWindowId);
                selectionResult = HuWinChoiceGet(fileSelectionWindowId, 1);
                if (selectionResult == 0) {
                    selectionResult = DeleteSaveFile(selectedBox);
                    if (selectionResult == 0) {
                        break;
                    }
                }
                if (UnMountCnt & (1 << curSlotNo)) {
                    continue;
                }
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x07));
                HuWinMesWait(fileSelectionWindowId);
                if (occupiedSaveSlotCount == 0) {
                    HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x40));
                }
                else if (occupiedSaveSlotCount == 3) {
                    HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x3F));
                }
                else {
                    HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x3D));
                }
                HuWinDispOn(saveSlotSelectionWindowId);
                ShowSaveSlotDetails(selectedBox);
                continue;
            }
            else if (HuPadBtnDown[0] & PAD_BUTTON_Y) {
                UnMountCnt = 0;
                if (HuCardSlotCheck(curSlotNo) < 0) {
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
                    HuWinMesWait(fileSelectionWindowId);
                    selectionResult = 0;
                    break;
                }
                if (!saveSlotIsOccupied[selectedBox]) {
                    continue;
                }
                for (boxIndex = 0; boxIndex < 3; boxIndex++) {
                    if (!saveSlotIsOccupied[boxIndex]) {
                        break;
                    }
                }
                if (boxIndex == 3) {
                    continue;
                }
                HuWinDispOff(saveSlotSelectionWindowId);
                HuAudFXPlay(2);
                if (CopySaveFile(selectedBox) == 0) {
                    selectionResult = 0;
                    break;
                }
                if (UnMountCnt & (1 << curSlotNo)) {
                    continue;
                }
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x07));
                HuWinMesWait(fileSelectionWindowId);
                if (occupiedSaveSlotCount == 0) {
                    HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x40));
                }
                else if (occupiedSaveSlotCount == 3) {
                    HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x3F));
                }
                else {
                    HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x3D));
                }
                HuWinDispOn(saveSlotSelectionWindowId);
                ShowSaveSlotDetails(selectedBox);
                continue;
            }
            else if (HuPadBtnDown[0] & PAD_BUTTON_A) {
                UnMountCnt = 0;
                HuWinDispOff(saveSlotSelectionWindowId);
                HuCardMount(curSlotNo);
                if (HuCardSlotCheck(curSlotNo) < 0) {
                    HuAudFXPlay(4);
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
                    HuWinMesWait(fileSelectionWindowId);
                    selectionResult = 0;
                    break;
                }
                if (SLSerialNoCheck() == 0) {
                    HuAudFXPlay(4);
                    HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x45));
                    HuWinMesWait(fileSelectionWindowId);
                    selectionResult = 0;
                    break;
                }
                HuAudFXPlay(2097);
                SLCurBoxNoSet(selectedBox);
                if (!saveSlotIsOccupied[selectedBox]) {
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x0B));
                    HuWinMesWait(fileSelectionWindowId);
                    saveTimestamp = OSGetTime();
                    SLSaveDataMake(1, &saveTimestamp);
                    GWGameStatReset();
                    #if VERSION_PAL
                    _ClearFlag(0x1000B);
                    #endif
                    GWGameStat.sound_mode = msmSysGetOutputMode();
                    SLCommonSet();
                    ShowSaveSlotMessage(MAKE_MESSID(16, 0x3C));
                    HuPrcSleep(20);
                    if (UnMountCnt & (1 << curSlotNo)) {
                        CloseSaveSlotMessage();
                        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x50));
                        HuWinMesWait(fileSelectionWindowId);
                        selectionResult = 0;
                        break;
                    }
                    cardResult = CreateSaveFile(selectedBox);
                    CloseSaveSlotMessage();
                    if (cardResult != 0) {
                        selectionResult = 1;
                        break;
                    }
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4F));
                    HuWinAttrSet(fileSelectionWindowId, 0x10);
                    HuWinMesWait(fileSelectionWindowId);
                    selectionResult = HuWinChoiceGet(fileSelectionWindowId, 1);
                    if (selectionResult == 0) {
                        SLSaveFlagSet(0);
                        selectionResult = 1;
                        break;
                    }
                    HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x52), 0);
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4C));
                    HuWinMesWait(fileSelectionWindowId);
                    while (!(HuPadBtnDown[0] & PAD_BUTTON_A)) {
                        HuPrcVSleep();
                    }
                    selectionResult = 0;
                    break;
                }
                else {
                    if (!OpenSaveFile(SaveFileNameTbl[selectedBox], curSlotNo)) {
                        if (!ReadSaveFile()) {
                            if (SLCheckSumCheck()) {
                                CloseSaveFile();
                                SLLoadGameStat();
                                #if VERSION_PAL
                                if(GwLanguageSave != -1) {
                                    GWGameStat.language = GwLanguageSave;
                                }
                                GwLanguage = GWGameStat.language;
                                _ClearFlag(0x1000B);
                                #endif
                                selectionResult = 1;
                                break;
                            }
                            HuWinHomeClear(fileSelectionWindowId);
                            HuPrcSleep(60);
                            if (UnMountCnt & (1 << curSlotNo)) {
                                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x50));
                                HuWinMesWait(fileSelectionWindowId);
                                selectionResult = 0;
                                break;
                            }
                            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(saveSlotNames[selectedBox]), 0);
                            HuWinAttrSet(fileSelectionWindowId, 0x10);
                            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x3A));
                            HuWinMesWait(fileSelectionWindowId);
                            selectionResult = HuWinChoiceGet(fileSelectionWindowId, 1);
                            if (!selectionResult) {
                                selectionResult = DeleteSaveFile(selectedBox);
                                if (selectionResult == 0) {
                                    break;
                                }
                                if (HuCardMount(curSlotNo) < 0) {
                                    selectionResult = 0;
                                    break;
                                }
                            }
                        }
                        else {
                            selectionResult = 0;
                        }
                    }
                    else {
                        selectionResult = 0;
                    }
                    if (selectionResult == 0) {
                        break;
                    }
                    if (UnMountCnt & (1 << curSlotNo)) {
                        continue;
                    }
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x07));
                    HuWinMesWait(fileSelectionWindowId);
                    if (occupiedSaveSlotCount == 0) {
                        HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x40));
                    }
                    else if (occupiedSaveSlotCount == 3) {
                        HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x3F));
                    }
                    else {
                        HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x3D));
                    }
                    HuWinDispOn(saveSlotSelectionWindowId);
                    ShowSaveSlotDetails(selectedBox);
                    continue;
                }
            }
            else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
                HuAudFXPlay(3);
                selectionResult = 0;
                break;
            }
        }
    }
    HuWinDispOff(saveSlotSelectionWindowId);
    for (boxIndex = 0; boxIndex <= 20; boxIndex++) {
        scaleProgress = cosd(90.0 * (boxIndex / 20.0));
        Hu3DModelScaleSet(modeSelectModelIds[curSlotNo], scaleProgress, scaleProgress, scaleProgress);
        for (modelIndex = 0; modelIndex < 3; modelIndex++) {
            Hu3DModelScaleSet(modeSelectModelIds[modelIndex + 8], scaleProgress, scaleProgress, scaleProgress);
            Hu3DModelScaleSet(modeSelectModelIds[modelIndex + 11], scaleProgress, scaleProgress, scaleProgress);
        }
        if (selectionResult) {
            espScaleSet(modeSelectSpriteIds[1], scaleProgress, scaleProgress);
        }
        HuPrcVSleep();
    }
    HuWinKill(saveSlotSelectionWindowId);
    return selectionResult;
}

void AnimateSaveSlotSelection(void)
{
    s16 index;
    FileSelectionAnimationWork *work;
    s16 i;
    s32 param;
    HUPROCESS *curr;
    curr = HuPrcCurrentGet();
    work = curr->user_data;
    index = work->fileSlotIndex;
    param = work->selectionActive;
    if (param) {
        Hu3DModelAttrReset(modeSelectModelIds[index + 8], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(modeSelectModelIds[index + 11], HU3D_ATTR_DISPOFF);
    }
    else {
        Hu3DModelAttrSet(modeSelectModelIds[index + 8], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(modeSelectModelIds[index + 11], HU3D_ATTR_DISPOFF);
    }
    for (i = 0; i <= 10; i++) {
        float scale;
        Vec pos2d;
        Vec pos3d;
        if (param) {
            scale = sind(90.0f * (i / 10.0f));
        }
        else {
            scale = cosd(90.0f * (i / 10.0f));
        }
        pos2d.x = 158.0f + (index * 130);
        pos2d.y = 230;
        pos2d.z = 800;
        Hu3D2Dto3D(&pos2d, 1, &pos3d);
        pos3d.z = 800 + (100.0f * scale);
        Hu3DModelPosSetV(modeSelectModelIds[index + 8], &pos3d);
        Hu3DModelPosSetV(modeSelectModelIds[index + 11], &pos3d);
        Hu3DModelScaleSet(modeSelectModelIds[index + 8], 1, 1, 1);
        Hu3DModelScaleSet(modeSelectModelIds[index + 11], 1, 1, 1);
        HuPrcVSleep();
    }
    (void)curr;
    HuMemDirectFree(work);
    HuPrcEnd();
    while (1) {
        HuPrcVSleep();
    }
}

void SetSaveSlotHighlight(s16 boxno, s32 flag)
{
    FileSelectionAnimationWork *work;
    HUPROCESS *parent;
    HUPROCESS *child;

    parent = HuPrcCurrentGet();
    child = HuPrcChildCreate(AnimateSaveSlotSelection, 50, 8192, 0, parent);
    work = HuMemDirectMallocNum(HEAP_HEAP, sizeof(FileSelectionAnimationWork), HU_MEMNUM_OVL);
    child->user_data = work;
    work->fileSlotIndex = boxno;
    work->selectionActive = flag;
}

s32 OpenSaveFile(char *name, s16 slotno)
{
    s32 result;
    s16 choice;
    s16 filecnt;
    filecnt = 0;
    while (1) {
        filecnt++;
        result = HuCardMount(slotno);
        if (result == CARD_RESULT_READY) {
            result = HuCardOpen(slotno, name, &curFileInfo);
            if (result == CARD_RESULT_READY || result == CARD_RESULT_NOFILE) {
                if (SLSerialNoCheck()) {
                    return result;
                }
            }
        }
        if (result == CARD_RESULT_WRONGDEVICE) {
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[slotno]), 0);
            #if VERSION_PAL
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x39));
            #else
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x37));
            #endif
            HuWinMesWait(fileSelectionWindowId);
            return result;
        }
        if (result == CARD_RESULT_FATAL_ERROR || filecnt > 3) {
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[slotno]), 0);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x53));
            HuWinMesWait(fileSelectionWindowId);
            return result;
        }
        else if (result == CARD_RESULT_NOCARD) {
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
            HuWinMesWait(fileSelectionWindowId);
            return result;
        }
        else if (result == CARD_RESULT_NOFILE || !SLSerialNoCheck()) {
            OSReport("Card Open Error:No File\n");
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x45));
            HuWinMesWait(fileSelectionWindowId);
            break;
        }
        else if (result == CARD_RESULT_BROKEN) {
            UnMountCnt = 0;
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[slotno]), 0);
            HuWinAttrSet(fileSelectionWindowId, 0x10);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x04));
            HuWinMesWait(fileSelectionWindowId);
            choice = HuWinChoiceGet(fileSelectionWindowId, 1);
            if (choice == 0) {
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4E));
                HuWinMesWait(fileSelectionWindowId);
                choice = HuWinChoiceGet(fileSelectionWindowId, 1);
                if (choice == 0) {
                    ShowSaveSlotMessage(MAKE_MESSID(16, 0x38));
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x0B));
                    HuWinMesWait(fileSelectionWindowId);
                    HuPrcSleep(60);
                    if (UnMountCnt & (1 << curSlotNo)) {
                        CloseSaveSlotMessage();
                        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x50));
                        HuWinMesWait(fileSelectionWindowId);
                    }
                    else {
                        _SetFlag(0x30000);
                        result = HuCardFormat(slotno);
                        _ClearFlag(0x30000);
                        if (result == CARD_RESULT_READY) {
                            SLSerialNoGet();
                        }
                        CloseSaveSlotMessage();
                        if (result == CARD_RESULT_FATAL_ERROR) {
                            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x36));
                            HuWinMesWait(fileSelectionWindowId);
                            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
                            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x53));
                            HuWinMesWait(fileSelectionWindowId);
                            return result;
                        }
                        else if (result == CARD_RESULT_NOCARD) {
                            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
                            HuWinMesWait(fileSelectionWindowId);
                            return result;
                        }
                    }
                }
                else {
                    return result;
                }
                continue;
            }
            return result;
        }
        break;
    }
    return result;
}

s32 CreateSaveFile(s16 boxno)
{
    s32 result;
    s32 i;
    result = HuCardMount(curSlotNo);
    if (result == 0 && !SLSerialNoCheck()) {
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x45));
        HuWinMesWait(fileSelectionWindowId);
    }
    else {
        result = HuCardSectorSizeGet(curSlotNo);
        if (result == -3) {
            CloseSaveSlotMessage();
            OSReport("SectorSize Check Error:No Card %d\n", curSlotNo);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
            HuWinMesWait(fileSelectionWindowId);
        }
        else {
            if (result != 8192) {
                CloseSaveSlotMessage();
                HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x39));
                HuWinMesWait(fileSelectionWindowId);
            }
            else {
                u32 byteNotUsed;
                u32 filesNotUsed;
                result = HuCardFreeSpaceGet(curSlotNo, &byteNotUsed, &filesNotUsed);
                if (filesNotUsed == 0 && byteNotUsed < SAVE_BUF_SIZE) {
                    CloseSaveSlotMessage();
                    HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4A));
                    HuWinMesWait(fileSelectionWindowId);
                }
                else if (filesNotUsed == 0) {
                    CloseSaveSlotMessage();
                    HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4A));
                    HuWinMesWait(fileSelectionWindowId);
                }
                else if (byteNotUsed < SAVE_BUF_SIZE) {
                    CloseSaveSlotMessage();
                    HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4A));
                    HuWinMesWait(fileSelectionWindowId);
                }
                else {
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x0B));
                    HuWinMesWait(fileSelectionWindowId);
                    for (i = 0; i <= 40; i++) {
                        float time = i / 40.0;
                        Hu3DModelTPLvlSet(modeSelectModelIds[boxno + 8], 0.6 + (0.4 * time));
                        Hu3DModelTPLvlSet(modeSelectModelIds[boxno + 11], 0.3 + (0.7 * time));
                        HuPrcVSleep();
                    }
                    _SetFlag(0x30000);
                    result = HuCardCreate(curSlotNo, SaveFileNameTbl[boxno], SAVE_BUF_SIZE, &curFileInfo);
                    if (result == CARD_RESULT_NOCARD) {
                        _ClearFlag(0x30000);
                        CloseSaveSlotMessage();
                        OSReport("File Create Error:No Card\n");
                        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
                        HuWinMesWait(fileSelectionWindowId);
                    }
                    else if (result == CARD_RESULT_WRONGDEVICE) {
                        _ClearFlag(0x30000);
                        CloseSaveSlotMessage();
                        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
                        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x37));
                        HuWinMesWait(fileSelectionWindowId);
                    }
                    else if (result < 0) {
                        _ClearFlag(0x30000);
                        CloseSaveSlotMessage();
                        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
                        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x53));
                        HuWinMesWait(fileSelectionWindowId);
                    }
                    else {
                        result = WriteSaveFile();
                        _ClearFlag(0x30000);
                        if (result != 0) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

s32 WriteSaveFile(void)
{
    s32 result;
    SLCheckSumSet();
    _SetFlag(0x30000);
    result = HuCardWrite(&curFileInfo, &saveBuf.buf[0], SAVE_BUF_SIZE, 0);
    _ClearFlag(0x30000);
    if (result == CARD_RESULT_NOCARD) {
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
        HuWinMesWait(fileSelectionWindowId);
        return 0;
    }
    if (result == CARD_RESULT_WRONGDEVICE) {
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x37));
        HuWinMesWait(fileSelectionWindowId);
        return 0;
    }
    if (result < 0) {
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x53));
        HuWinMesWait(fileSelectionWindowId);
        return 0;
    }
    result = SLStatSet(0);
    if (result == CARD_RESULT_NOCARD) {
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
        HuWinMesWait(fileSelectionWindowId);
        return 0;
    }
    if (result == CARD_RESULT_WRONGDEVICE) {
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x37));
        HuWinMesWait(fileSelectionWindowId);
        return 0;
    }
    if (result < 0) {
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x53));
        HuWinMesWait(fileSelectionWindowId);
        return 0;
    }
    return 1;
}

s32 ReadSaveFile(void)
{
    s32 result;
    result = HuCardRead(&curFileInfo, &saveBuf.buf[0], SAVE_BUF_SIZE, 0);
    if (result == -3) {
        OSReport("Card Read Error:No Card\n");
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
        HuWinMesWait(fileSelectionWindowId);
    }
    else if (result == -4 || !SLSerialNoCheck()) {
        OSReport("Card Read Error:No File\n", result);
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x45));
        HuWinMesWait(fileSelectionWindowId);
    }
    else if (result < 0) {
        OSReport("Card Read Error:Fatal Error %d\n", result);
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x53));
        HuWinMesWait(fileSelectionWindowId);
    }
    return result;
}

s32 DeleteSaveFile(s16 boxno)
{
    float scale;
    s32 result;
    s32 time;
    ANIMDATA *sprite;
    result = HuCardMount(curSlotNo);
    if (result == -3) {
        OSReport("Card Delete Error:No Card\n");
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
        HuWinMesWait(fileSelectionWindowId);
        return 0;
    }
    if (result == -4 || !SLSerialNoCheck()) {
        OSReport("Card Delete Error:No File\n", result);
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x45));
        HuWinMesWait(fileSelectionWindowId);
        return 0;
    }
    if (result < 0) {
        OSReport("Card Delete Error:Fatal Error %d\n", result);
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x53));
        HuWinMesWait(fileSelectionWindowId);
        return 0;
    }

    ShowSaveSlotMessage(MAKE_MESSID(16, 0x3B));
    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x0B));
    HuWinMesWait(fileSelectionWindowId);
    HuPrcSleep(60);
    if (UnMountCnt & (1 << curSlotNo)) {
        CloseSaveSlotMessage();
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x50));
        HuWinMesWait(fileSelectionWindowId);
        return 0;
    }
    else {
        _SetFlag(0x30000);
        result = HuCardDelete(curSlotNo, SaveFileNameTbl[boxno]);
        _ClearFlag(0x30000);
        CloseSaveSlotMessage();
        if (result == -3) {
            OSReport("Card Delete Error:No Card\n");
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x02));
            HuWinMesWait(fileSelectionWindowId);
            return 0;
        }
        if (result == -4 || !SLSerialNoCheck()) {
            OSReport("Card Delete Error:No File\n", result);
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x45));
            HuWinMesWait(fileSelectionWindowId);
            return 0;
        }
        if (result < 0) {
            OSReport("Card Delete Error:Fatal Error %d\n", result);
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x53));
            HuWinMesWait(fileSelectionWindowId);
            return 0;
        }
        for (time = 0; time <= 10; time++) {
            scale = cosd(90.0 * (time / 10.0));
            Hu3DModelScaleSet(modeSelectModelIds[boxno + 8], scale, scale, scale);
            HuPrcVSleep();
        }
        Hu3DAnimKill(saveSlotPrimaryStarAnimationIds[boxno]);
        Hu3DAnimKill(saveSlotSecondaryStarAnimationIds[boxno]);
        sprite = HuSprAnimRead(HuDataReadNum(saveSlotStarAnimationData[8], HU_MEMNUM_OVL));
        saveSlotPrimaryStarAnimationIds[boxno] = Hu3DAnimCreate(sprite, modeSelectModelIds[boxno + 8], "star");
        sprite = HuSprAnimRead(HuDataReadNum(saveSlotStarAnimationData[17], HU_MEMNUM_OVL));
        saveSlotSecondaryStarAnimationIds[boxno] = Hu3DAnimCreate(sprite, modeSelectModelIds[boxno + 11], "star");
        saveSlotStarCounts[boxno] = -1;
        Hu3DModelTPLvlSet(modeSelectModelIds[boxno + 8], 0.6f);
        Hu3DModelTPLvlSet(modeSelectModelIds[boxno + 11], 0.3f);
        for (time = 0; time <= 20; time++) {
            scale = sind(90.0 * (time / 20.0));
            Hu3DModelScaleSet(modeSelectModelIds[boxno + 8], scale, scale, scale);
            HuPrcVSleep();
        }
        saveSlotIsOccupied[boxno] = 0;
        occupiedSaveSlotCount--;
        return 1;
    }
}

s32 CopySaveFile(s16 boxno)
{
    float scaleProgress;
    s16 targetBox;
    s16 boxIndex;
    s32 cardResult;
    s16 boxChange;
    s16 starIndex;
    ANIMDATA *starAnimation;
    Vec sourceScreenPosition;
    Vec sourceWorldPosition;
    Vec targetScreenPosition;
    Vec positionDelta;
    s32 freeBlocks;
    s32 freeBytes;
    HUPROCESS *currentProcess;
    HU3DMODEL *selectedModel;
    currentProcess = HuPrcCurrentGet();
    OSReport("Card Copy Seq.:Open\n");
    cardResult = OpenSaveFile(SaveFileNameTbl[boxno], curSlotNo);
    if (cardResult < 0 || !SLSerialNoCheck()) {
        return 0;
    }
    OSReport("Card Copy Seq.:Read\n");
    cardResult = ReadSaveFile();
    if (cardResult != 0) {
        return 0;
    }
    OSReport("Card Copy Seq.:CheckSum Check\n");
    if (!SLCheckSumCheck()) {
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(saveSlotNames[boxno]), 0);
        HuWinAttrSet(fileSelectionWindowId, 0x10);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x3A));
        HuWinMesWait(fileSelectionWindowId);
        cardResult = HuWinChoiceGet(fileSelectionWindowId, 1);
        if (cardResult != 0) {
            return 1;
        }
        else {
            cardResult = DeleteSaveFile(boxno);
            return cardResult;
        }
    }
    if (!SLSerialNoCheck()) {
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x45));
        HuWinMesWait(fileSelectionWindowId);
        return 0;
    }
    cardResult = HuCardFreeSpaceGet(curSlotNo, (u32 *)&freeBytes, (u32 *)&freeBlocks);
    if (freeBlocks == 0 && freeBytes < SAVE_BUF_SIZE) {
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4A));
        HuWinMesWait(fileSelectionWindowId);
        return 1;
    }
    else if (freeBlocks == 0) {
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4A));
        HuWinMesWait(fileSelectionWindowId);
        return 1;
    }
    else if (freeBytes < SAVE_BUF_SIZE) {
        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x4A));
        HuWinMesWait(fileSelectionWindowId);
        return 1;
    }
    for (boxIndex = 0; boxIndex < 3; boxIndex++) {
        if (!saveSlotIsOccupied[boxIndex]) {
            break;
        }
    }
    if (boxIndex == 3) {
        return 1;
    }
    targetBox = boxIndex;
    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x09));
    HuWinMesWait(fileSelectionWindowId);
    HuWinMesSet(saveSlotSelectionWindowId, MAKE_MESSID(16, 0x35));
    HuWinDispOn(saveSlotSelectionWindowId);
    copySourceSaveSlot = boxno;
    Hu3DModelAttrReset(modeSelectModelIds[23], HU3D_ATTR_DISPOFF);
    selectedModel = &Hu3DData[modeSelectModelIds[boxno + 8]];
    Hu3DModelPosSetV(modeSelectModelIds[23], &selectedModel->pos);
    for (boxIndex = 1; boxIndex <= 10; boxIndex++) {
        scaleProgress = 0.7 + (0.3 * (boxIndex / 10.0));
        Hu3DModelScaleSet(modeSelectModelIds[23], scaleProgress, scaleProgress, scaleProgress);
        HuPrcVSleep();
    }
    SetSaveSlotHighlight(targetBox, 1);
    while (1) {
        HuPrcVSleep();
        if (UnMountCnt & (1 << curSlotNo)) {
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x50));
            HuWinMesWait(fileSelectionWindowId);
            break;
        }
        else {
            boxChange = 0;
            if (HuPadDStkRep[0] & 0x1) {
                boxChange = -1;
            }
            else if (HuPadDStkRep[0] & 0x2) {
                boxChange = 1;
            }
            (void)boxChange;
            if (boxChange) {
                HuAudFXPlay(0);
                SetSaveSlotHighlight(targetBox, 0);
                while (1) {
                    targetBox += boxChange;
                    if (targetBox < 0) {
                        targetBox = 2;
                    }
                    else if (targetBox >= 3) {
                        targetBox = 0;
                    }
                    if (saveSlotIsOccupied[targetBox] == 0) {
                        break;
                    }
                }
                SetSaveSlotHighlight(targetBox, 1);
                HuPrcSleep(10);
            }
            if (HuPadBtnDown[0] & PAD_BUTTON_A) {
                HuAudFXPlay(2096);
            }
            else if (!(HuPadBtnDown[0] & PAD_BUTTON_B)) {
                continue;
            }
            else {
                HuAudFXPlay(3);
                Hu3DModelAttrSet(modeSelectModelIds[23], HU3D_ATTR_DISPOFF);
                Hu3DModelAttrReset(modeSelectModelIds[boxno + 8], HU3D_ATTR_DISPOFF);
                SetSaveSlotHighlight(targetBox, 0);
                HuWinDispOff(saveSlotSelectionWindowId);
                return 1;
            }
            HuWinDispOff(saveSlotSelectionWindowId);
            cardResult = OpenSaveFile(SaveFileNameTbl[boxno], curSlotNo);
            if (cardResult >= 0 && SLSerialNoCheck()) {
                ShowSaveSlotMessage(MAKE_MESSID(16, 0x0A));
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x0B));
                HuWinMesWait(fileSelectionWindowId);
                Hu3DModelAttrReset(modeSelectModelIds[boxno + 8], HU3D_ATTR_DISPOFF);
                OSReport("CheckSum Check\n");
                if (!SLCheckSumCheck()) {
                    HuPrcSleep(60);
                    HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(saveSlotNames[boxno]), 0);
                    HuWinAttrSet(fileSelectionWindowId, 0x10);
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x3A));
                    HuWinMesWait(fileSelectionWindowId);
                    cardResult = HuWinChoiceGet(fileSelectionWindowId, 1);
                    if (cardResult == 0) {
                        cardResult = DeleteSaveFile(boxno);
                    }
                }
                else {
                    CloseSaveFile();
                    for (boxIndex = 0; boxIndex <= 10; boxIndex++) {
                        scaleProgress = cosd(90.0 * (boxIndex / 10.0));
                        Hu3DModelScaleSet(modeSelectModelIds[targetBox + 8], scaleProgress, scaleProgress, scaleProgress);
                        HuPrcVSleep();
                    }
                    Hu3DModelAttrSet(modeSelectModelIds[23], HU3D_ATTR_DISPOFF);
                    Hu3DModelAttrReset(modeSelectModelIds[boxno + 8], HU3D_ATTR_DISPOFF);
                    if (UnMountCnt & (1 << curSlotNo)) {
                        CloseSaveSlotMessage();
                        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x50));
                        HuWinMesWait(fileSelectionWindowId);
                    }
                    else {
                        OSReport("File Create\n");
                        SLCurBoxNoSet(targetBox);
                        SLLoadGameStat();
                        SLSaveDataMake(0, &GWGameStat.create_time);
                        if (CreateSaveFile(targetBox)) {
                            CloseSaveFile();
                            LoadSaveSlotMetadata(targetBox);
                            Hu3DAnimKill(saveSlotPrimaryStarAnimationIds[targetBox]);
                            Hu3DAnimKill(saveSlotSecondaryStarAnimationIds[targetBox]);
                            starIndex = saveSlotStoryCharacters[targetBox];
                            if (starIndex < 0) {
                                starIndex = 8;
                            }
                            (void)starIndex;
                            starAnimation = HuSprAnimRead(HuDataReadNum(saveSlotStarAnimationData[starIndex], HU_MEMNUM_OVL));
                            saveSlotPrimaryStarAnimationIds[targetBox] = Hu3DAnimCreate(starAnimation, modeSelectModelIds[targetBox + 8], "star");
                            starAnimation = HuSprAnimRead(HuDataReadNum(saveSlotStarAnimationData[starIndex], HU_MEMNUM_OVL));
                            saveSlotSecondaryStarAnimationIds[targetBox] = Hu3DAnimCreate(starAnimation, modeSelectModelIds[targetBox + 11], "star");
                            Hu3DModelAttrReset(modeSelectModelIds[targetBox + 11], HU3D_ATTR_DISPOFF);
                            Hu3DModelAttrSet(modeSelectModelIds[targetBox + 8], HU3D_ATTR_DISPOFF);
                            sourceScreenPosition.x = 158.0f + (boxno * 130);
                            sourceScreenPosition.y = 230;
                            sourceScreenPosition.z = 800;
                            Hu3D2Dto3D(&sourceScreenPosition, 1, &sourceWorldPosition);
                            sourceWorldPosition.z = 900;
                            sourceScreenPosition.x = 158.0f + (targetBox * 130);
                            Hu3D2Dto3D(&sourceScreenPosition, 1, &targetScreenPosition);
                            targetScreenPosition.z = 800;
                            VECSubtract(&targetScreenPosition, &sourceWorldPosition, &positionDelta);
                            Hu3DModelTPLvlSet(modeSelectModelIds[targetBox + 8], 1.0f);
                            Hu3DModelTPLvlSet(modeSelectModelIds[targetBox + 11], 1.0f);
                            saveSlotModelsVisible[targetBox] = 0;
                            for (boxIndex = 1; boxIndex <= 30; boxIndex++) {
                                scaleProgress = boxIndex / 30.0;
                                sourceScreenPosition.x = positionDelta.x * scaleProgress;
                                sourceScreenPosition.y = positionDelta.y * scaleProgress;
                                sourceScreenPosition.z = (positionDelta.z * scaleProgress) + (-300 * sind(180.0f * scaleProgress));
                                VECAdd(&sourceScreenPosition, &sourceWorldPosition, &sourceScreenPosition);
                                Hu3DModelPosSetV(modeSelectModelIds[targetBox + 11], &sourceScreenPosition);
                                HuPrcVSleep();
                            }
                            saveSlotModelsVisible[targetBox] = 1;
                            CloseSaveSlotMessage();
                            saveSlotIsOccupied[targetBox] = 1;
                            occupiedSaveSlotCount++;
                            return 1;
                        }
                    }
                }
            }
            break;
        }
    }
    CloseSaveSlotMessage();
    Hu3DModelAttrSet(modeSelectModelIds[23], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrReset(modeSelectModelIds[boxno + 8], HU3D_ATTR_DISPOFF);
    for (boxIndex = 0; boxIndex <= 10; boxIndex++) {
        scaleProgress = sind(90.0 * (boxIndex / 10.0));
        Hu3DModelScaleSet(modeSelectModelIds[targetBox + 8], scaleProgress, scaleProgress, scaleProgress);
        HuPrcVSleep();
    }
    SetSaveSlotHighlight(targetBox, 0);
    return 0;
}

s32 CloseSaveFile(void)
{
    s32 result = HuCardClose(&curFileInfo);
    return result;
}

s16 saveSlotDateDigitWidths[] = { 15, 15, 13, 15, 15, 8, 15, 15, 8, 15, 15 };

s32 InitializeSaveSlotDisplay(void)
{
    s16 boxIndex;
    s16 spriteIndex;
    s16 spriteId;
    HU3DMODEL *model;
    s16 starType;
    s16 dateDigitOffset;
    s16 totalDateWidth;
    ANIMDATA *starAnimation;
    s16 result;
    ANIMDATA *dateAnimation;
    Mtx modelScaleMatrix;
    Vec screenPosition;
    Vec worldPosition;
    WindowData *messageWindow;
    result = -1;
    messageWindow = &winData[fileSelectionWindowId];
    OSReport("Init\n");
    MTXScale(modelScaleMatrix, 0.5f, 0.5f, 0.5f);
    for (boxIndex = 0; boxIndex < 3; boxIndex++) {
        if (saveSlotStarSpriteGroups[boxIndex] != -1) {
            HuSprGrpKill(saveSlotStarSpriteGroups[boxIndex]);
        }
    }
    for (boxIndex = 0; boxIndex < 3; boxIndex++) {
        if (saveSlotDateSpriteGroups[boxIndex] != -1) {
            HuSprGrpKill(saveSlotDateSpriteGroups[boxIndex]);
        }
    }
    saveSlotStarAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATA_MAKE_NUM(DATADIR_MODESEL, 0x00), 0x22), HU_MEMNUM_OVL));
    saveSlotDigitAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATA_MAKE_NUM(DATADIR_MODESEL, 0x00), 0x23), HU_MEMNUM_OVL));
    saveSlotOverflowAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATA_MAKE_NUM(DATADIR_MODESEL, 0x00), 0x25), HU_MEMNUM_OVL));
    dateAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATA_MAKE_NUM(DATADIR_MODESEL, 0x00), 0x24), HU_MEMNUM_OVL));
    for (boxIndex = 0; boxIndex < 3; boxIndex++) {
        Hu3DModelAttrSet(modeSelectModelIds[boxIndex + 8], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrReset(modeSelectModelIds[boxIndex + 11], HU3D_ATTR_DISPOFF);
        starType = saveSlotStoryCharacters[boxIndex];
        if (starType < 0 || starType >= 8) {
            starType = 8;
        }
        if (saveSlotPrimaryStarAnimationIds[boxIndex] != -1) {
            Hu3DAnimKill(saveSlotPrimaryStarAnimationIds[boxIndex]);
        }
        if (saveSlotSecondaryStarAnimationIds[boxIndex] != -1) {
            Hu3DAnimKill(saveSlotSecondaryStarAnimationIds[boxIndex]);
        }
        starAnimation = HuSprAnimRead(HuDataReadNum(saveSlotStarAnimationData[starType], HU_MEMNUM_OVL));
        saveSlotPrimaryStarAnimationIds[boxIndex] = Hu3DAnimCreate(starAnimation, modeSelectModelIds[boxIndex + 8], "star");
        starAnimation = HuSprAnimRead(HuDataReadNum(saveSlotStarAnimationData[starType + 9], HU_MEMNUM_OVL));
        saveSlotSecondaryStarAnimationIds[boxIndex] = Hu3DAnimCreate(starAnimation, modeSelectModelIds[boxIndex + 11], "star");
        if (!saveSlotIsOccupied[boxIndex]) {
            Hu3DModelTPLvlSet(modeSelectModelIds[boxIndex + 8], 0.6f);
            Hu3DModelTPLvlSet(modeSelectModelIds[boxIndex + 11], 0.3f);
        }
        else {
            Hu3DModelTPLvlSet(modeSelectModelIds[boxIndex + 8], 1.0f);
            Hu3DModelTPLvlSet(modeSelectModelIds[boxIndex + 11], 1.0f);
        }
        screenPosition.x = 158.0f + (boxIndex * 130);
        screenPosition.y = 230;
        screenPosition.z = 800;
        Hu3D2Dto3D(&screenPosition, 1, &worldPosition);
        worldPosition.z = 800;
        Hu3DModelPosSetV(modeSelectModelIds[boxIndex + 8], &worldPosition);
        Hu3DModelPosSetV(modeSelectModelIds[boxIndex + 11], &worldPosition);
        Hu3DModelScaleSet(modeSelectModelIds[boxIndex + 8], 0, 0, 0);
        Hu3DModelScaleSet(modeSelectModelIds[boxIndex + 11], 0, 0, 0);
        model = &Hu3DData[modeSelectModelIds[boxIndex + 8]];
        MTXCopy(modelScaleMatrix, model->mtx);
        model = &Hu3DData[modeSelectModelIds[boxIndex + 11]];
        MTXCopy(modelScaleMatrix, model->mtx);
        saveSlotStarSpriteGroups[boxIndex] = HuSprGrpCreate(9);
        spriteId = HuSprCreate(saveSlotStarAnimation, 3000, 0);
        HuSprGrpMemberSet(saveSlotStarSpriteGroups[boxIndex], 0, spriteId);
        HuSprPosSet(saveSlotStarSpriteGroups[boxIndex], 0, -48, 0);
        for (spriteIndex = 0; spriteIndex < 4; spriteIndex++) {
            spriteId = HuSprCreate(saveSlotDigitAnimation, 3000, 0);
            HuSprGrpMemberSet(saveSlotStarSpriteGroups[boxIndex], spriteIndex + 1, spriteId);
            HuSprPosSet(saveSlotStarSpriteGroups[boxIndex], spriteIndex + 1, (spriteIndex * 24) - 24, 0);
        }
        spriteId = HuSprCreate(saveSlotOverflowAnimation, 3000, 0);
        HuSprGrpMemberSet(saveSlotStarSpriteGroups[boxIndex], 5, spriteId);
        HuSprPosSet(saveSlotStarSpriteGroups[boxIndex], 5, 16, 0);
        HuSprGrpScissorSet(saveSlotStarSpriteGroups[boxIndex], 0, 16, 640, 448);
        saveSlotDateSpriteGroups[boxIndex] = HuSprGrpCreate(11);
        for (spriteIndex = totalDateWidth = 0; spriteIndex < 11; spriteIndex++) {
            totalDateWidth += saveSlotDateDigitWidths[spriteIndex];
        }
        for (spriteIndex = dateDigitOffset = 0; spriteIndex < 11; spriteIndex++) {
            spriteId = HuSprCreate(dateAnimation, 3000, 0);
            HuSprGrpMemberSet(saveSlotDateSpriteGroups[boxIndex], spriteIndex, spriteId);
            HuSprPosSet(saveSlotDateSpriteGroups[boxIndex], spriteIndex, -(totalDateWidth / 2) + 8 + dateDigitOffset, 0);
            dateDigitOffset += saveSlotDateDigitWidths[spriteIndex];
        }
        HuSprAttrSet(saveSlotDateSpriteGroups[boxIndex], 5, 4);
        HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 2, 10);
        HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 8, 11);
        HuSprGrpScissorSet(saveSlotDateSpriteGroups[boxIndex], 0, 16, 640, 448);
    }
    MTXScale(modelScaleMatrix, 0.45f, 0.45f, 0.45f);
    model = &Hu3DData[modeSelectModelIds[23]];
    Hu3DModelAttrSet(modeSelectModelIds[23], HU3D_MOTATTR_LOOP);
    MTXCopy(modelScaleMatrix, model->mtx);
    return result;
}

void LoadSaveSlotMetadata(s16 boxno)
{
    SLLoadGameStat();
    SLLoadBoardStory();
    saveSlotStarCounts[boxno] = GWGameStat.total_stars;
    saveSlotCreationTimes[boxno] = GWGameStat.create_time;
    saveSlotStoryCharacters[boxno] = GWSystem.storyChar;
    saveSlotStoryContinueFlags[boxno] = GWGameStat.story_continue;
    saveSlotPartyContinueFlags[boxno] = GWGameStat.party_continue;
    if (GWGameStat.story_continue) {
        SLLoadBoardStory();
        saveSlotStoryBoardIds[boxno] = GWSystem.board;
        saveSlotStoryBoardIndices[boxno] = 0;
    }
    if (GWGameStat.party_continue) {
        SLLoadBoard();
        saveSlotPartyBoardIds[boxno] = GWSystem.board;
        saveSlotCurrentTurns[boxno] = GWSystem.turn;
        saveSlotMaxTurns[boxno] = GWSystem.max_turn;
    }
}

void UpdateSaveSlotDisplay(void)
{
    float spriteScale;
    float horizontalProjection;
    float depthProjection;
    HU3DMODEL *model;
    s16 boxIndex;
    s16 digitValue;
    s16 digitIndex;
    Mtx modelScaleMatrix;
    OSCalendarTime calendarTime;
    Vec screenPosition;
    Vec worldPosition;
    for (boxIndex = 0; boxIndex < 2; boxIndex++) {
        fileSlotLabelsVisible[boxIndex] = 1;
    }
    while (1) {
        for (boxIndex = 0; boxIndex < 2; boxIndex++) {
            MTXScale(modelScaleMatrix, 0.5f, 0.5f, 0.5f);
            mtxRotCat(modelScaleMatrix, 0.0f, 0.0f, 15.0f * fileSlotRotationAngles[boxIndex]);
            model = &Hu3DData[modeSelectModelIds[boxIndex]];
            MTXCopy(modelScaleMatrix, model->mtx);
            Hu3DModelRotSet(modeSelectModelIds[boxIndex], 0, fileSelectionRotationAngle * fileSlotRotationSpeeds[boxIndex] * GET_ZEROSIGN(boxIndex == 0), 0);
            if (!fileSlotLabelsVisible[boxIndex]) {
                espAttrSet(modeSelectSpriteIds[boxIndex + 2], HUSPR_ATTR_DISPOFF);
            }
            else {
                model = &Hu3DData[modeSelectModelIds[boxIndex + 2]];
                if (model->attr & 0x1) {
                    model = &Hu3DData[modeSelectModelIds[boxIndex + 4]];
                }
                if (model->attr & 0x1) {
                    espAttrSet(modeSelectSpriteIds[boxIndex + 2], HUSPR_ATTR_DISPOFF);
                }
                else {
                    espAttrReset(modeSelectSpriteIds[boxIndex + 2], HUSPR_ATTR_DISPOFF);
                }
                worldPosition = model->pos;
                worldPosition.y -= 30.0f * model->scale.x;
                Hu3D3Dto2D(&worldPosition, 1, &screenPosition);
                espPosSet(modeSelectSpriteIds[boxIndex + 2], screenPosition.x, screenPosition.y);
                espScaleSet(modeSelectSpriteIds[boxIndex + 2], model->scale.x, model->scale.x);
            }
        }
        fileSelectionRotationAngle += 3.0f;
        if (fileSelectionRotationAngle > 1080.0f) {
            fileSelectionRotationAngle -= 360;
        }
        if (highlightedFileSlot == -1) {
            espAttrSet(modeSelectSpriteIds[4], HUSPR_ATTR_DISPOFF);
        }
        else {
            model = &Hu3DData[modeSelectModelIds[highlightedFileSlot]];
            if (model->attr & 0x1) {
                espAttrSet(modeSelectSpriteIds[4], HUSPR_ATTR_DISPOFF);
            }
            else {
                espBankSet(modeSelectSpriteIds[4], highlightedFileSlot);
                espAttrReset(modeSelectSpriteIds[4], HUSPR_ATTR_DISPOFF);
                worldPosition = model->pos;
                worldPosition.y += 5.0f * model->scale.x;
                Hu3D3Dto2D(&worldPosition, 1, &screenPosition);
                espPosSet(modeSelectSpriteIds[4], screenPosition.x, screenPosition.y);
                espScaleSet(modeSelectSpriteIds[4], model->scale.x, model->scale.x);
            }
        }
        for (boxIndex = 0; boxIndex < 3; boxIndex++) {
            if (!saveSlotModelsVisible[boxIndex]) {
                espAttrSet(modeSelectSpriteIds[boxIndex + 5], HUSPR_ATTR_DISPOFF);
                continue;
            }
            model = &Hu3DData[modeSelectModelIds[boxIndex + 8]];
            if (model->attr & 0x1) {
                model = &Hu3DData[modeSelectModelIds[boxIndex + 11]];
            }
            horizontalProjection = (900.0 + (50.0 * model->scale.x)) * sin(M_PI / 12);
            depthProjection = (model->pos.z + (50.0 * model->scale.x)) * sin(M_PI / 12);
            spriteScale = model->scale.x * (depthProjection / horizontalProjection);
            if (model->attr & 0x1) {
                espAttrSet(modeSelectSpriteIds[boxIndex + 5], HUSPR_ATTR_DISPOFF);
            }
            else {
                espAttrReset(modeSelectSpriteIds[boxIndex + 5], HUSPR_ATTR_DISPOFF);
            }
            worldPosition = model->pos;
            worldPosition.y += 40.0 * model->scale.x;
            worldPosition.z += 50.0 * model->scale.x;
            Hu3D3Dto2D(&worldPosition, 1, &screenPosition);
            espPosSet(modeSelectSpriteIds[boxIndex + 5], screenPosition.x, screenPosition.y);
            espScaleSet(modeSelectSpriteIds[boxIndex + 5], spriteScale, spriteScale);
            if (saveSlotStarCounts[boxIndex] == -1) {
                HuSprGrpPosSet(saveSlotStarSpriteGroups[boxIndex], 0, -500);
            }
            else {
                worldPosition = model->pos;
                worldPosition.y -= 30.0 * model->scale.x;
                worldPosition.z += 50.0 * model->scale.x;
                Hu3D3Dto2D(&worldPosition, 1, &screenPosition);
                HuSprGrpPosSet(saveSlotStarSpriteGroups[boxIndex], screenPosition.x, screenPosition.y);
                HuSprGrpScaleSet(saveSlotStarSpriteGroups[boxIndex], spriteScale, spriteScale);
                if (saveSlotStarCounts[boxIndex] > 9999) {
                    for (digitIndex = 0; digitIndex < 4; digitIndex++) {
                        HuSprAttrSet(saveSlotStarSpriteGroups[boxIndex], digitIndex + 1, HUSPR_ATTR_DISPOFF);
                    }
                    HuSprAttrReset(saveSlotStarSpriteGroups[boxIndex], 5, HUSPR_ATTR_DISPOFF);
                }
                else {
                    for (digitIndex = 0; digitIndex < 4; digitIndex++) {
                        HuSprAttrReset(saveSlotStarSpriteGroups[boxIndex], digitIndex + 1, HUSPR_ATTR_DISPOFF);
                    }
                    HuSprAttrSet(saveSlotStarSpriteGroups[boxIndex], 5, HUSPR_ATTR_DISPOFF);
                    digitValue = saveSlotStarCounts[boxIndex];
                    HuSprBankSet(saveSlotStarSpriteGroups[boxIndex], 1, digitValue / 1000);
                    digitValue -= (digitValue / 1000) * 1000;
                    HuSprBankSet(saveSlotStarSpriteGroups[boxIndex], 2, digitValue / 100);
                    digitValue -= (digitValue / 100) * 100;
                    HuSprBankSet(saveSlotStarSpriteGroups[boxIndex], 3, digitValue / 10);
                    digitValue -= (digitValue / 10) * 10;
                    HuSprBankSet(saveSlotStarSpriteGroups[boxIndex], 4, digitValue);
                }
                worldPosition = model->pos;
                worldPosition.y -= 40.0 * model->scale.x;
                worldPosition.z += 50.0 * model->scale.x;
                Hu3D3Dto2D(&worldPosition, 1, &screenPosition);
                HuSprGrpPosSet(saveSlotDateSpriteGroups[boxIndex], screenPosition.x, screenPosition.y);
                HuSprGrpScaleSet(saveSlotDateSpriteGroups[boxIndex], spriteScale, spriteScale);
                OSTicksToCalendarTime(saveSlotCreationTimes[boxIndex], &calendarTime);
                #if VERSION_PAL
                digitValue = calendarTime.mday;
                HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 0, digitValue / 10);
                digitValue -= (digitValue / 10) * 10;
                HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 1, digitValue);
                digitValue = calendarTime.mon+1;
                HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 3, digitValue / 10);
                digitValue -= (digitValue / 10) * 10;
                HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 4, digitValue);
                #else
                digitValue = calendarTime.mon + 1;
                HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 0, digitValue / 10);
                digitValue -= (digitValue / 10) * 10;
                HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 1, digitValue);
                digitValue = calendarTime.mday;
                HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 3, digitValue / 10);
                digitValue -= (digitValue / 10) * 10;
                HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 4, digitValue);
                #endif
                digitValue = calendarTime.hour;
                HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 6, digitValue / 10);
                digitValue -= (digitValue / 10) * 10;
                HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 7, digitValue);
                digitValue = calendarTime.min;
                HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 9, digitValue / 10);
                digitValue -= (digitValue / 10) * 10;
                HuSprBankSet(saveSlotDateSpriteGroups[boxIndex], 10, digitValue);
            }
        }
        HuPrcVSleep();
    }
}

s16 fileSelectionMessageWindow = -1;

void ShowSaveSlotMessage(u32 messageId)
{
    float size[2];
    if (fileSelectionMessageWindow != -1) {
        HuWinExCleanup(fileSelectionMessageWindow);
    }
    HuWinInsertMesSizeGet(MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
    HuWinMesMaxSizeGet(1, size, messageId);
    fileSelectionMessageWindow = HuWinExCreateStyled(-10000, 308, size[0], size[1], -1, 2);
    HuWinMesSpeedSet(fileSelectionMessageWindow, 0);
    HuWinExAnimIn(fileSelectionMessageWindow);
    HuWinInsertMesSet(fileSelectionMessageWindow, MAKE_MESSID_PTR(memoryCardSlotNames[curSlotNo]), 0);
    HuWinMesSet(fileSelectionMessageWindow, messageId);
    HuWinMesWait(fileSelectionMessageWindow);
}

void CloseSaveSlotMessage(void)
{
    if (fileSelectionMessageWindow != -1) {
        HuWinExAnimOut(fileSelectionMessageWindow);
        HuWinExCleanup(fileSelectionMessageWindow);
        fileSelectionMessageWindow = -1;
    }
}

void ShowSaveSlotDetails(s16 boxno)
{
    char mesMaxTurn[4];
    char mesTurn[4];
    char mesDummy[1] = { 0 };
    if (saveSlotStarCounts[boxno] < 0) {
        HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x11));
    }
    else {
        if (!saveSlotStoryContinueFlags[boxno] && !saveSlotPartyContinueFlags[boxno]) {
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x11), 0);
            HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(mesDummy), 1);
            HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x43));
        }
        else {
            if (!saveSlotStoryContinueFlags[boxno]) {
                HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x11), 0);
                HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(mesDummy), 1);
                HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x12) + saveSlotPartyBoardIds[boxno], 2);
                sprintf(mesMaxTurn, "%2d", saveSlotCurrentTurns[boxno]);
                HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(mesMaxTurn), 3);
                sprintf(mesTurn, "%2d", saveSlotMaxTurns[boxno]);
                HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(mesTurn), 4);
                HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x42));
            }
            else {
                if (!saveSlotPartyContinueFlags[boxno]) {
                    HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x12) + saveSlotStoryBoardIds[boxno], 0);
                    if (saveSlotStoryBoardIndices[boxno]) {
                        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x18), 1);
                    }
                    else {
                        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(mesDummy), 1);
                    }
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x43));
                }
                else {
                    HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x12) + saveSlotStoryBoardIds[boxno], 0);
                    if (saveSlotStoryBoardIndices[boxno]) {
                        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x18), 1);
                    }
                    else {
                        HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(mesDummy), 1);
                    }
                    HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x12) + saveSlotPartyBoardIds[boxno], 2);
                    sprintf(mesMaxTurn, "%2d", saveSlotCurrentTurns[boxno]);
                    HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(mesMaxTurn), 3);
                    sprintf(mesTurn, "%2d", saveSlotMaxTurns[boxno]);
                    HuWinInsertMesSet(fileSelectionWindowId, MAKE_MESSID_PTR(mesTurn), 4);
                    HuWinMesSet(fileSelectionWindowId, MAKE_MESSID(16, 0x42));
                }
            }
        }
    }
    HuWinMesWait(fileSelectionWindowId);
}

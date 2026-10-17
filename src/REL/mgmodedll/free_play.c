#include "REL/mgmodedll.h"
#include "game/gamework.h"

#include "game/audio.h"
#include "game/esprite.h"
#include "game/gamework_data.h"
#include "game/window.h"
#include "game/wipe.h"

#include "ext_math.h"
#include <string.h>

#ifndef __MWERKS__
void ExitMinigameMode(s16 modeType);
#endif

s16 freePlayWindows[2][7];
s16 freePlayMenuSpriteGroups[4];
s16 freePlayMenuSelection;
s16 freePlayPlayerCharacterIds[4];
s16 freePlayPlayerSelections[10][2];
s16 freePlayVisibleRowCount;
s16 freePlayPlayerRowIndices[4];
s16 freePlaySelectedPlayer;
Vec freePlayMenuPositions[2];
s16 freePlayMenuState;
s16 freePlayPlayerCount;
s16 freePlayMinigameType;
float freePlayAnimationProgress;
s16 freePlayAnimationFrame;
s16 freePlayWindowState;
s16 freePlaySpriteGroup;
static s16 pad05000002E0Bss;

s16 freePlayMessageWindow = -1;

s16 freePlaySelectionOrder[7] = { 0, 1, 2, 4, 3, 6, 9 };

float freePlayMenuColumnPositions[4] = { 370, 412, 488, 530 };

s32 freePlayPlayerGroupRules[4] = {
    DATA_MAKE_NUM(DATADIR_MGMODE, 0x37),
    DATA_MAKE_NUM(DATADIR_MGMODE, 0x3D),
    DATA_MAKE_NUM(DATADIR_MGMODE, 0x3A),
    DATA_MAKE_NUM(DATADIR_MGMODE, 0x40),
};

void ShowFreePlayMenu(void);
void AnimateFreePlaySelection(s16 value, s16 secondaryValue, s16 tertiaryValue);
void MoveFreePlayPlayerMarker(s16 playerIndex, float targetValue, float frame, s16 groupIndex);

void HideFreePlayMenu(void);
void ShowFreePlayMenuSelection(void);
void SetFreePlayCharacter(s16 playerIndex, s16 playerStateIndex);
void SetFreePlayMenuMode(s32 modeType);

void AnimateFreePlayPlayer(s16 playerIndex, s16 playerStateIndex, s16 selectionIndex, s16 groupIndex);

s16 GetFreePlayPlayerCounts(s16 *playerIndex, s16 *playerStateIndex);
void SetFreePlayPlayerGroups(void);
void UpdateFreePlayPlayerSelection(void);
s32 GetFreePlayPlayerSelection(void);
void SetFreePlayGameType(s16 modeType);
s32 CheckFreePlaySelection(s32 value);

s32 RunFreePlaySelection(void)
{
    s16 animationFrame;
    s16 menuItemIndex;
    s16 selectionDelta;
    s16 titleSpriteId;
    s16 unreferencedMenuScratch;
    s32 animationStep;
    s32 entryCount;
    WindowData *winP;
    s32 windowId;
    s16 frameLimit;
    s16 winId;
    
    float winSize[2];
    s16 menuItemVisibility[6];
    s16 menuScrollOffset;
    s16 selectedEntryOffset;
    float animationProgress;
    entryCount = 1;
    BuildMinigameTypeLists(0);
    ShowFreePlayMenuSelection();
    ShowFreePlayMenu();
    menuScrollOffset = selectedEntryOffset = 0;
    mgInstExitEnableF = 1;
    mgTypeCurr = 0;
    _SetFlag(11);
    SetFreePlayPlayerGroups();
    if (freePlayModeType != 2) {
        titleSpriteId = mgModeSpriteIds[1];
    }
    else {
        titleSpriteId = mgModeSpriteIds[2];
    }
    if (mgModeEvtNo == 1 || mgModeEvtNo == 2) {
        mgTypeCurr = GetFreePlayPlayerCounts(&playerIndex, &playerStateIndex);
        espAttrReset(mgModeSpriteIds[3], HUSPR_ATTR_DISPOFF);
        espTPLvlSet(mgModeSpriteIds[3], 1.0f);
        espAttrReset(titleSpriteId, HUSPR_ATTR_DISPOFF);
        espPosSet(titleSpriteId, 288, 70);
        espAttrReset(mgModeSpriteIds[13], HUSPR_ATTR_DISPOFF);
        espPosSet(mgModeSpriteIds[13], 450, 216);
        if (freePlayModeType == 0) {
            espAttrReset(mgModeSpriteIds[4], HUSPR_ATTR_DISPOFF);
            espPosSet(mgModeSpriteIds[4], 450, 404);
            if (mgModeEvtNo == 1) {
                SetFreePlayGameType(minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][menuScrollOffset + selectedEntryOffset]);
                espAttrReset(mgModeSpriteIds[14], HUSPR_ATTR_DISPOFF);
                espPosSet(mgModeSpriteIds[14], 450, 348);
                for (menuItemIndex = 0; menuItemIndex < 4; menuItemIndex++) {
                    HuSprAttrReset(freePlayMenuSpriteGroups[menuItemIndex], 0, HUSPR_ATTR_DISPOFF);
                    HuSprAttrReset(freePlayMenuSpriteGroups[menuItemIndex], 1, HUSPR_ATTR_DISPOFF);
                    HuSprGrpPosSet(freePlayMenuSpriteGroups[menuItemIndex], (menuItemIndex * 50) + 375, 348);
                }
            }
            else {
                espAttrReset(mgModeSpriteIds[15], HUSPR_ATTR_DISPOFF);
                espPosSet(mgModeSpriteIds[15], 450, 348);
                espAttrReset(mgModeSpriteIds[16], HUSPR_ATTR_DISPOFF);
                espPosSet(mgModeSpriteIds[16], 450, 348);
                for (menuItemIndex = 0; menuItemIndex < 4; menuItemIndex++) {
                    HuSprAttrReset(freePlayMenuSpriteGroups[menuItemIndex], 0, HUSPR_ATTR_DISPOFF);
                    HuSprAttrReset(freePlayMenuSpriteGroups[menuItemIndex], 1, HUSPR_ATTR_DISPOFF);
                    HuSprGrpPosSet(freePlayMenuSpriteGroups[menuItemIndex], freePlayMenuColumnPositions[menuItemIndex], 348);
                }
            }
        }
        else if (freePlayModeType == 1) {
            espAttrReset(mgModeSpriteIds[20], HUSPR_ATTR_DISPOFF);
            espPosSet(mgModeSpriteIds[20], 450, 414);
        }
        else if (freePlayModeType == 2) {
            espAttrReset(mgModeSpriteIds[21], HUSPR_ATTR_DISPOFF);
            espPosSet(mgModeSpriteIds[21], 450, 400);
        }
        for (menuItemIndex = 0; menuItemIndex < freePlayVisibleRowCount; menuItemIndex++) {
            espAttrReset(mgModeSpriteIds[freePlayPlayerSelections[menuItemIndex][1]], HUSPR_ATTR_DISPOFF);
            espPosSet(mgModeSpriteIds[freePlayPlayerSelections[menuItemIndex][1]], 180, 270);
            espPriSet(mgModeSpriteIds[freePlayPlayerSelections[menuItemIndex][1]], (menuItemIndex * 10) + 20);
        }
        espPriSet(mgModeSpriteIds[freePlayPlayerSelections[mgTypeCurr][1]], 19);
        AnimateFreePlaySelection(0, freePlayPlayerSelections[mgTypeCurr][0], 0);
        MoveFreePlayPlayerMarker(0, 180, 270, 19);
        AnimateFreePlayPlayer(minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][menuScrollOffset + selectedEntryOffset], 0, 450, 216);
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
    }
    else {
        espAttrReset(mgModeSpriteIds[3], HUSPR_ATTR_DISPOFF);
        espTPLvlSet(mgModeSpriteIds[3], 0.0f);
        for (animationFrame = 0; animationFrame <= 20; animationFrame++) {
            espTPLvlSet(mgModeSpriteIds[3], animationFrame / 20.0);
            HuPrcVSleep();
        }
        for (animationFrame = 0; animationFrame < freePlayVisibleRowCount; animationFrame++) {
            menuItemVisibility[animationFrame] = -1;
            espPriSet(mgModeSpriteIds[freePlayPlayerSelections[animationFrame][1]], (animationFrame * 10) + 20);
        }
        animationStep = 0;
        frameLimit = (freePlayVisibleRowCount * 10) + 10;
        for (animationFrame = 0; animationFrame <= frameLimit; animationFrame++) {
            if (animationFrame <= 20) {
                animationProgress = animationFrame / 20.0;
                espAttrReset(titleSpriteId, HUSPR_ATTR_DISPOFF);
                espPosSet(titleSpriteId, 288, -100 + (170 * sind(animationProgress * 90.0f)));
            }
            if (animationFrame <= 20) {
                animationProgress = animationFrame / 20.0;
                espAttrReset(mgModeSpriteIds[13], HUSPR_ATTR_DISPOFF);
                espPosSet(mgModeSpriteIds[13], (-350 * sind(animationProgress * 90.0f)) + 800, 216);
                if (freePlayModeType == 0) {
                    espAttrReset(mgModeSpriteIds[4], HUSPR_ATTR_DISPOFF);
                    espPosSet(mgModeSpriteIds[4], (-350 * sind(animationProgress * 90.0f)) + 800, 404);
                    if (freePlayModeSelection != 1) {
                        espAttrReset(mgModeSpriteIds[14], HUSPR_ATTR_DISPOFF);
                        espPosSet(mgModeSpriteIds[14], (-350 * sind(animationProgress * 90.0f)) + 800, 348);
                        for (menuItemIndex = 0; menuItemIndex < 4; menuItemIndex++) {
                            HuSprAttrReset(freePlayMenuSpriteGroups[menuItemIndex], 0, HUSPR_ATTR_DISPOFF);
                            HuSprAttrReset(freePlayMenuSpriteGroups[menuItemIndex], 1, HUSPR_ATTR_DISPOFF);
                            HuSprGrpPosSet(freePlayMenuSpriteGroups[menuItemIndex], (menuItemIndex * 50) + (((-350 * sind(animationProgress * 90.0f)) + 800) - 75), 348);
                        }
                    }
                    else {
                        espAttrReset(mgModeSpriteIds[15], HUSPR_ATTR_DISPOFF);
                        espPosSet(mgModeSpriteIds[15], (-350 * sind(animationProgress * 90.0f)) + 800, 348);
                        espAttrReset(mgModeSpriteIds[16], HUSPR_ATTR_DISPOFF);
                        espPosSet(mgModeSpriteIds[16], (-350 * sind(animationProgress * 90.0f)) + 800, 348);
                        for (menuItemIndex = 0; menuItemIndex < 4; menuItemIndex++) {
                            HuSprAttrReset(freePlayMenuSpriteGroups[menuItemIndex], 0, HUSPR_ATTR_DISPOFF);
                            HuSprAttrReset(freePlayMenuSpriteGroups[menuItemIndex], 1, HUSPR_ATTR_DISPOFF);
                            HuSprGrpPosSet(freePlayMenuSpriteGroups[menuItemIndex], (800.0 * (1 - sind(animationProgress * 90.0f))) + freePlayMenuColumnPositions[menuItemIndex], 348);
                        }
                    }
                }
                else {
                    if (freePlayModeType == 1) {
                        espAttrReset(mgModeSpriteIds[20], HUSPR_ATTR_DISPOFF);
                        espPosSet(mgModeSpriteIds[20], (-350 * sind(animationProgress * 90.0f)) + 800, 414);
                    }
                    else if (freePlayModeType == 2) {
                        espAttrReset(mgModeSpriteIds[21], HUSPR_ATTR_DISPOFF);
                        espPosSet(mgModeSpriteIds[21], (-350 * sind(animationProgress * 90.0f)) + 800, 400);
                    }
                }
            }
            for (menuItemIndex = 0; menuItemIndex < freePlayVisibleRowCount; menuItemIndex++) {
                if (animationFrame >= ((freePlayVisibleRowCount - menuItemIndex) - 1) * 10 && animationFrame <= (((freePlayVisibleRowCount - menuItemIndex) - 1) * 10) + 10) {
                    animationProgress = (animationFrame - (((freePlayVisibleRowCount - menuItemIndex) - 1) * 10)) / 10.0;
                    if (0.0 == animationProgress) {
                        animationStep++;
                        AnimateFreePlaySelection(animationStep & 0x1, freePlayPlayerSelections[menuItemIndex][0], 0);
                    }
                    espAttrReset(mgModeSpriteIds[freePlayPlayerSelections[menuItemIndex][1]], HUSPR_ATTR_DISPOFF);
                    espPosSet(mgModeSpriteIds[freePlayPlayerSelections[menuItemIndex][1]], 180, 800 + (-530 * sind(animationProgress * 90.0f)));
                    espPriSet(mgModeSpriteIds[freePlayPlayerSelections[menuItemIndex][1]], (menuItemIndex * 10) + 10);
                    MoveFreePlayPlayerMarker(animationStep & 0x1, 180, 800 + (-530 * sind(animationProgress * 90.0f)), (menuItemIndex * 10) + 8);
                }
            }
            HuPrcVSleep();
        }
        AnimateFreePlayPlayer(minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][0], 10, 450, 216);
    }
    #if VERSION_NTSC
    espAttrReset(mgModeSpriteIds[19], HUSPR_ATTR_DISPOFF);
    espPosSet(mgModeSpriteIds[19], 204, (selectedEntryOffset * 38) + 154);
    espPriSet(mgModeSpriteIds[19], 7);
    espTPLvlSet(mgModeSpriteIds[19], 0.7);
    espAttrSet(mgModeSpriteIds[17], HUSPR_ATTR_DISPOFF);
    espPosSet(mgModeSpriteIds[17], 204, 126);
    espPriSet(mgModeSpriteIds[17], 8);
    espAttrSet(mgModeSpriteIds[18], HUSPR_ATTR_DISPOFF);
    espPosSet(mgModeSpriteIds[18], 204, 410);
    espPriSet(mgModeSpriteIds[18], 8);
    espBankSet(mgModeSpriteIds[18], 1);
    #else
    espAttrSet(mgModeSpriteIds[17], HUSPR_ATTR_DISPOFF);
    espPosSet(mgModeSpriteIds[17], 204, 126);
    espPriSet(mgModeSpriteIds[17], 8);
    espAttrSet(mgModeSpriteIds[18], HUSPR_ATTR_DISPOFF);
    espPosSet(mgModeSpriteIds[18], 204, 410);
    espPriSet(mgModeSpriteIds[18], 8);
    espBankSet(mgModeSpriteIds[18], 1);
    #endif
    MoveFreePlayPlayerMarker(0, 180, 270, 7);
    AnimateFreePlaySelection(0, freePlayPlayerSelections[mgTypeCurr][0], tertiaryValue);
    MoveFreePlayPlayerMarker(1, 180, 270, 100);
    SetFreePlayCharacter(availableMinigameCountByType[freePlayPlayerSelections[mgTypeCurr][0]], minigameCountByType[freePlayPlayerSelections[mgTypeCurr][0]]);
    SetFreePlayMenuMode(1);
    HuPrcSleep(10);
    while (WipeStatGet()) {
        HuPrcVSleep();
    }
    #if VERSION_PAL
    if(freePlayModeType == 2) {
        HuWinMesMaxSizeGet(1, winSize, MAKE_MESSID(57, 0));
        winId = HuWinExCreateStyled(-10000, -10000, winSize[0], winSize[1], -1, 1);
        winP = &winData[winId];
        winP->active_pad = 1;
        HuWinMesPalSet(winId, 7, 0, 0, 0);
        HuWinPriSet(winId, 3);
        HuWinExAnimIn(winId);
        HuWinMesSet(winId, MAKE_MESSID(57, 0));
        HuWinMesWait(winId);
        HuWinExAnimOut(winId);
        HuWinExCleanup(winId);
    }
    espAttrReset(mgModeSpriteIds[19], HUSPR_ATTR_DISPOFF);
    espPosSet(mgModeSpriteIds[19], 204, (selectedEntryOffset*38)+154);
    espPriSet(mgModeSpriteIds[19], 7);
    espTPLvlSet(mgModeSpriteIds[19], 0.7f);
    #endif
    while (1) {
        HuPrcVSleep();
        if (menuScrollOffset != 0) {
            espAttrReset(mgModeSpriteIds[17], HUSPR_ATTR_DISPOFF);
        }
        else {
            espAttrSet(mgModeSpriteIds[17], HUSPR_ATTR_DISPOFF);
        }
        if (menuScrollOffset + 7 < minigameCountByType[freePlayPlayerSelections[mgTypeCurr][0]]) {
            espAttrReset(mgModeSpriteIds[18], HUSPR_ATTR_DISPOFF);
        }
        else {
            espAttrSet(mgModeSpriteIds[18], HUSPR_ATTR_DISPOFF);
        }
        unreferencedMenuScratch = selectionDelta = 0;
        if (HuPadDStkRep[0] & PAD_BUTTON_UP) {
            selectionDelta = -1;
        }
        else if (HuPadDStkRep[0] & PAD_BUTTON_DOWN) {
            selectionDelta = 1;
        }
        else if ((HuPadDStkRep[0] & PAD_BUTTON_LEFT) || (HuPadBtnRep[0] & PAD_BUTTON_TRIGGER_L)) {
            unreferencedMenuScratch = -1;
        }
        else if ((HuPadDStkRep[0] & PAD_BUTTON_RIGHT) || (HuPadBtnRep[0] & PAD_BUTTON_TRIGGER_R)) {
            unreferencedMenuScratch = 1;
        }
        if (selectionDelta != 0) {
            if (selectedEntryOffset + selectionDelta < 0 && menuScrollOffset != 0) {
                menuScrollOffset--;
                AnimateFreePlayPlayer(minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][menuScrollOffset + selectedEntryOffset], 5, 450, 216);
                for (animationFrame = 0; animationFrame <= 4; animationFrame++) {
                    espPosSet(mgModeSpriteIds[17], 204, 126 - (8.0 * sind(animationFrame * 45.0)));
                    if (animationFrame == 2) {
                        HuAudFXPlay(772);
                        AnimateFreePlaySelection(0, freePlayPlayerSelections[mgTypeCurr][0], tertiaryValue);
                    }
                    HuPrcVSleep();
                }
            }
            else if (selectedEntryOffset + selectionDelta >= 7 && menuScrollOffset + 7 < minigameCountByType[freePlayPlayerSelections[mgTypeCurr][0]]) {
                menuScrollOffset++;
                AnimateFreePlayPlayer(minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][menuScrollOffset + selectedEntryOffset], 5, 450, 216);
                for (animationFrame = 0; animationFrame <= 4; animationFrame++) {
                    espPosSet(mgModeSpriteIds[18], 204, 410 + (8.0 * sind(animationFrame * 45.0)));
                    if (animationFrame == 2) {
                        HuAudFXPlay(772);
                        AnimateFreePlaySelection(0, freePlayPlayerSelections[mgTypeCurr][0], tertiaryValue);
                    }
                    HuPrcVSleep();
                }
            }
            else if (selectedEntryOffset + selectionDelta >= 0 && (menuScrollOffset + selectedEntryOffset + selectionDelta) < minigameCountByType[freePlayPlayerSelections[mgTypeCurr][0]]) {
                AnimateFreePlayPlayer(minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][menuScrollOffset + selectedEntryOffset + selectionDelta], 5, 450, 216);
                for (animationFrame = 0; animationFrame <= 4; animationFrame++) {
                    espPosSet(mgModeSpriteIds[19], 204, ((selectedEntryOffset * 38) + 154) + (selectionDelta * (38.0 * sind(22.5 * animationFrame))));
                    HuPrcVSleep();
                    if (animationFrame == 2) {
                        HuAudFXPlay(772);
                    }
                }
                selectedEntryOffset += selectionDelta;
            }
            if (freePlayModeSelection == 0) {
                SetFreePlayGameType(minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][menuScrollOffset + selectedEntryOffset]);
            }
        }
        if (unreferencedMenuScratch && freePlayVisibleRowCount > 1) {
            HuAudFXPlay(2172);
            AnimateFreePlaySelection(0, freePlayPlayerSelections[mgTypeCurr][0], tertiaryValue);
            menuItemIndex = mgTypeCurr;
            if (mgTypeCurr + unreferencedMenuScratch < 0) {
                mgTypeCurr = freePlayVisibleRowCount - 1;
            }
            else {
                if (mgTypeCurr + unreferencedMenuScratch >= freePlayVisibleRowCount) {
                    mgTypeCurr = 0;
                }
                else {
                    mgTypeCurr += unreferencedMenuScratch;
                }
            }
            espAttrSet(mgModeSpriteIds[19], HUSPR_ATTR_DISPOFF);
            espAttrSet(mgModeSpriteIds[17], HUSPR_ATTR_DISPOFF);
            espAttrSet(mgModeSpriteIds[18], HUSPR_ATTR_DISPOFF);
            SetFreePlayMenuMode(0);
            for (animationFrame = 0; animationFrame < freePlayVisibleRowCount; animationFrame++) {
                espPriSet(mgModeSpriteIds[freePlayPlayerSelections[animationFrame][1]], 30);
            }
            espPriSet(mgModeSpriteIds[freePlayPlayerSelections[mgTypeCurr][1]], 25);
            espPriSet(mgModeSpriteIds[freePlayPlayerSelections[menuItemIndex][1]], 15);
            AnimateFreePlaySelection(1, freePlayPlayerSelections[menuItemIndex][0], tertiaryValue);
            MoveFreePlayPlayerMarker(1, 180, 270, 14);
            AnimateFreePlaySelection(0, freePlayPlayerSelections[mgTypeCurr][0], 0);
            AnimateFreePlayPlayer(minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][0], 20, 450, 216);
            for (animationFrame = 0; animationFrame <= 20; animationFrame++) {
                if (animationFrame == 10) {
                    espPriSet(mgModeSpriteIds[freePlayPlayerSelections[mgTypeCurr][1]], 10);
                }
                espPosSet(mgModeSpriteIds[freePlayPlayerSelections[mgTypeCurr][1]], (300.0 * sind(animationFrame * 9.0)) + 180.0, 270);
                if (animationFrame >= 10) {
                    windowId = 7;
                }
                else {
                    windowId = 20;
                }
                MoveFreePlayPlayerMarker(0, (300.0 * sind(animationFrame * 9.0)) + 180.0, 270, groupIndex);
                HuPrcVSleep();
            }
            AnimateFreePlaySelection(0, freePlayPlayerSelections[mgTypeCurr][0], 0);
            espPriSet(mgModeSpriteIds[freePlayPlayerSelections[mgTypeCurr][1]], 10);
            MoveFreePlayPlayerMarker(0, 180.0, 270, 7);
            espAttrReset(mgModeSpriteIds[19], HUSPR_ATTR_DISPOFF);
            espPosSet(mgModeSpriteIds[19], 204, 154);
            SetFreePlayCharacter(availableMinigameCountByType[freePlayPlayerSelections[mgTypeCurr][0]], minigameCountByType[freePlayPlayerSelections[mgTypeCurr][0]]);
            if (freePlayPlayerSelections[mgTypeCurr][0] != 9) {
                SetFreePlayMenuMode(1);
            }
            selectedEntryOffset = menuScrollOffset = 0;
            if (freePlayModeSelection == 0) {
                SetFreePlayGameType(minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][menuScrollOffset + selectedEntryOffset]);
            }
        }
        if ((HuPadBtnDown[0] & PAD_BUTTON_A) && freePlayModeType == 2) {
            animationFrame = freePlayPlayerSelections[mgTypeCurr][0];
            if (!GWMGAvailGet(minigameIdsByType[animationFrame][menuScrollOffset + selectedEntryOffset] + 401)) {
                HuAudFXPlay(4);
                continue;
            }
            else {
                if (freePlayPlayerSelections[mgTypeCurr][0] != 9) {
                    HuAudFXPlay(2);
                    animationFrame = minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][menuScrollOffset + selectedEntryOffset] + 401;
                    if (!GWMGCustomGet(animationFrame)) {
                        GWMGCustomSet(animationFrame);
                    }
                    else {
                        GWMGCustomReset(animationFrame);
                    }
                    AnimateFreePlaySelection(0, freePlayPlayerSelections[mgTypeCurr][0], tertiaryValue);
                    AnimateFreePlayPlayer(minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][menuScrollOffset + selectedEntryOffset], 0, 450, 216);
                    UpdateFreePlayPlayerSelection();
                }
                else {
                    if (minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][menuScrollOffset + selectedEntryOffset] >= 0) {
                        HuAudFXPlay(2);
                        animationFrame = minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][menuScrollOffset + selectedEntryOffset] + 401;
                        GWMGCustomReset(animationFrame);
                        UpdateFreePlayPlayerSelection();
                        if (minigameCountByType[freePlayPlayerSelections[mgTypeCurr][0]] <= menuScrollOffset + selectedEntryOffset) {
                            if (--selectedEntryOffset < 0) {
                                selectedEntryOffset = 0;
                                menuScrollOffset--;
                            }
                            espPosSet(mgModeSpriteIds[19], 204, (selectedEntryOffset * 38) + 154);
                        }
                        AnimateFreePlaySelection(0, freePlayPlayerSelections[mgTypeCurr][0], tertiaryValue);
                        AnimateFreePlayPlayer(minigameIdsByType[freePlayPlayerSelections[mgTypeCurr][0]][menuScrollOffset + selectedEntryOffset], 0, 450, 216);
                    }
                }
            }
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            HuAudFXPlay(3);
            if (freePlayModeType == 2 && !GetFreePlayPlayerSelection()) {
                continue;
            }
            entryCount = 0;
            break;
        }
        if (!(HuPadBtnDown[0] & PAD_BUTTON_A)) {
            continue;
        }
        if (freePlayModeType) {
            continue;
        }
        animationFrame = freePlayPlayerSelections[mgTypeCurr][0];
        if (!GWMGAvailGet(minigameIdsByType[animationFrame][menuScrollOffset + selectedEntryOffset] + 401)) {
            HuAudFXPlay(4);
            continue;
        }
        HuAudFXPlay(773);
        GWSystem.mg_next = minigameIdsByType[animationFrame][menuScrollOffset + selectedEntryOffset];
        if (freePlayModeSelection == 0) {
            GWPlayerCfg[0].group = 0;
            GWPlayerCfg[1].group = 1;
            GWPlayerCfg[2].group = 2;
            GWPlayerCfg[3].group = 3;
            if ((animationFrame == 1 || animationFrame == 2) && !CheckFreePlaySelection(0)) {
                continue;
            }
            ExitMinigameMode(1);
        }
        else if (freePlayModeSelection == 1) {
            ExitMinigameMode(2);
        }
        while (1) {
            HuPrcVSleep();
        }
    }
    espAttrSet(mgModeSpriteIds[19], HUSPR_ATTR_DISPOFF);
    espAttrSet(mgModeSpriteIds[17], HUSPR_ATTR_DISPOFF);
    espAttrSet(mgModeSpriteIds[18], HUSPR_ATTR_DISPOFF);
    if (freePlayModeType == 2) {
        for (animationFrame = 0; animationFrame < 7; animationFrame++) {
            HuSprAttrSet(freePlaySpriteGroup, animationFrame, HUSPR_ATTR_DISPOFF);
        }
    }

    SetFreePlayMenuMode(0);
    AnimateFreePlayPlayer(-1, 5, 450, 216);

    for (animationFrame = 0; animationFrame <= 20; animationFrame++) {
        if (animationFrame <= 20) {
            animationProgress = animationFrame / 20.0;
            espAttrReset(titleSpriteId, HUSPR_ATTR_DISPOFF);
            espPosSet(titleSpriteId, 288, -100.0 + (170 * cosd(animationProgress * 90.0f)));
        }
        if (animationFrame <= 10) {
            animationProgress = animationFrame / 10.0;
            espAttrReset(mgModeSpriteIds[13], HUSPR_ATTR_DISPOFF);
            espPosSet(mgModeSpriteIds[13], (-350 * cosd(animationProgress * 90.0f)) + 800, 216);
            if (freePlayModeType == 0) {
                espPosSet(mgModeSpriteIds[4], (-350 * cosd(animationProgress * 90.0f)) + 800, 404);
                if (freePlayModeSelection != 1) {
                    espPosSet(mgModeSpriteIds[14], (-350 * cosd(animationProgress * 90.0f)) + 800, 348);
                    for (menuItemIndex = 0; menuItemIndex < 4; menuItemIndex++) {

                        HuSprGrpPosSet(freePlayMenuSpriteGroups[menuItemIndex], (menuItemIndex * 50) + (((-350 * cosd(animationProgress * 90.0f)) + 800) - 75), 348);
                    }
                }
                else {
                    espPosSet(mgModeSpriteIds[15], (-350 * cosd(animationProgress * 90.0f)) + 800, 348);
                    espPosSet(mgModeSpriteIds[16], (-350 * cosd(animationProgress * 90.0f)) + 800, 348);
                    for (menuItemIndex = 0; menuItemIndex < 4; menuItemIndex++) {
                        HuSprGrpPosSet(freePlayMenuSpriteGroups[menuItemIndex], (800 * (1 - cosd(animationProgress * 90.0f))) + freePlayMenuColumnPositions[menuItemIndex], 348);
                    }
                }
            }
            else {
                if (freePlayModeType == 1) {
                    espAttrReset(mgModeSpriteIds[20], HUSPR_ATTR_DISPOFF);
                    espPosSet(mgModeSpriteIds[20], (-350 * cosd(animationProgress * 90.0f)) + 800, 414);
                }
                else if (freePlayModeType == 2) {
                    espAttrReset(mgModeSpriteIds[21], HUSPR_ATTR_DISPOFF);
                    espPosSet(mgModeSpriteIds[21], (-350 * cosd(animationProgress * 90.0f)) + 800, 400);
                }
            }
        }
        animationProgress = animationFrame / 20.0;
        for (menuItemIndex = 0; menuItemIndex < freePlayVisibleRowCount; menuItemIndex++) {
            espPosSet(mgModeSpriteIds[freePlayPlayerSelections[menuItemIndex][1]], 180, 800 + (-530 * cosd(animationProgress * 90.0f)));
        }
        MoveFreePlayPlayerMarker(0, 180, (-530.0 * cosd(animationProgress * 90.0f)) + 800.0, 10);
        MoveFreePlayPlayerMarker(1, 180, (-530.0 * cosd(animationProgress * 90.0f)) + 800.0, 100);
        HuPrcVSleep();
    }
    HideFreePlayMenu();
    for (animationFrame = 0; animationFrame <= 10; animationFrame++) {
        espTPLvlSet(mgModeSpriteIds[3], 1.0 - (animationFrame / 10.0));
        HuPrcVSleep();
    }
    mgInstExitEnableF = 0;
    return entryCount;
}

void ShowFreePlayMenu(void)
{
    s16 j;
    s16 i;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 7; j++) {
            s16 window = HuWinCreate(0, j * 24, 268, 32, 0);
            WindowData *windowP;
            freePlayWindows[i][j] = window;
            windowP = &winData[window];
            windowP->spacing_x = 0;
            HuWinBGTPLvlSet(window, 0);
            HuWinMesSpeedSet(window, 0);
            HuWinDispOff(window);
            (void)i;
        }
    }
}

void AnimateFreePlaySelection(s16 value, s16 secondaryValue, s16 tertiaryValue)
{
    s16 i;
    s16 no;
    s16 window;
    if (freePlayModeType == 2 && freePlaySelectedPlayer == value) {
        for (i = 0; i < 4; i++) {
            HuSprAttrSet(freePlaySpriteGroup, i + 14, HUSPR_ATTR_DISPOFF);
        }
    }
    if (secondaryValue == 9) {
        for (i = 0; i < 7; i++) {
            HuSprAttrSet(freePlaySpriteGroup, i + (value * 7), HUSPR_ATTR_DISPOFF);
            window = freePlayWindows[value][i];
            if (i + tertiaryValue >= minigameCountByType[9]) {
                HuWinDispOff(window);
            }
            else {
                no = minigameIdsByType[9][i + tertiaryValue];
                if (no < 0) {
                    freePlaySelectedPlayer = value;
                    no = -no - 1;
                    freePlayPlayerRowIndices[no] = i;
                    HuSprAttrReset(freePlaySpriteGroup, no + 14, HUSPR_ATTR_DISPOFF);
                    HuSprPosSet(freePlaySpriteGroup, no + 14, 24 + freePlayMenuPositions[value].x, (freePlayMenuPositions[value].y + (i * 38)) - 116);
                    HuWinDispOff(window);
                }
                else {
                    HuWinMesSet(window, mgInfoTbl[no].name_mess);
                    HuWinDispOn(window);
                }
            }
        }
    }
    else {
        for (i = 0; i < 7; i++) {
            window = freePlayWindows[value][i];
            if (i + tertiaryValue >= minigameCountByType[secondaryValue]) {
                HuWinDispOff(window);
                if (freePlayModeType == 2) {
                    HuSprAttrSet(freePlaySpriteGroup, i + (value * 7), HUSPR_ATTR_DISPOFF);
                }
            }
            else {
                no = minigameIdsByType[secondaryValue][i + tertiaryValue];
                if (GWMGAvailGet(no + 401)) {
                    HuWinMesSet(window, mgInfoTbl[no].name_mess);
                }
                else {
                    HuWinMesSet(window, MAKE_MESSID(0x23, 0x00));
                }
                HuWinDispOn(window);
                if (freePlayModeType == 2) {
                    if (GWMGCustomGet(no + 401)) {
                        HuSprAttrReset(freePlaySpriteGroup, i + (value * 7), HUSPR_ATTR_DISPOFF);
                    }
                    else {
                        HuSprAttrSet(freePlaySpriteGroup, i + (value * 7), HUSPR_ATTR_DISPOFF);
                    }
                }
            }
        }
    }
}

void MoveFreePlayPlayerMarker(s16 playerIndex, float targetValue, float frame, s16 groupIndex)
{
    s16 i;
    freePlayMenuPositions[playerIndex].x = targetValue;
    freePlayMenuPositions[playerIndex].y = frame;
    for (i = 0; i < 7; i++) {
        s16 window = freePlayWindows[playerIndex][i];
        HuWinPosSet(window, targetValue - 105, (frame - 136) + (i * 38));
        HuWinPriSet(window, groupIndex);
        if (freePlayModeType == 2) {
            HuSprPosSet(freePlaySpriteGroup, i + (playerIndex * 7), targetValue + 24, frame + (i * 38) - 116);
            HuSprPriSet(freePlaySpriteGroup, i + (playerIndex * 7), groupIndex);
        }
    }
    if (freePlayModeType == 2 && freePlaySelectedPlayer == playerIndex) {
        for (i = 0; i < 4; i++) {
            HuSprPosSet(freePlaySpriteGroup, i + 14, targetValue + 24, frame + (freePlayPlayerRowIndices[i] * 38) - 116);
            HuSprPriSet(freePlaySpriteGroup, i + 14, groupIndex + 1);
        }
    }
}

void HideFreePlayMenu(void)
{
    s16 j;
    s16 i;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 7; j++) {
            HuWinKill(freePlayWindows[i][j]);
        }
    }
}

void ShowFreePlayMenuSelection(void)
{
    s16 menuScrollOffset[2];
    s16 i;
    ANIMDATA *anim;
    s16 sprite;
    s16 group;
    s16 player;
    s16 bank;
    s16 iscom;
    if (freePlayModeSelection == 1) {
        menuScrollOffset[0] = 0;
        menuScrollOffset[1] = 2;
        for (i = 0; i < 4; i++) {
            if (GWPlayerCfg[i].group >= 2) {
                freePlayPlayerCharacterIds[i] = 0;
            }
            else {
                freePlayPlayerCharacterIds[menuScrollOffset[GWPlayerCfg[i].group]] = i;
                menuScrollOffset[GWPlayerCfg[i].group]++;
            }
        }
    }
    else {
        for (i = 0; i < 4; i++) {
            freePlayPlayerCharacterIds[i] = i;
        }
    }
    for (i = 0; i < 4; i++) {
        if (freePlayMenuSpriteGroups[i]) {
            HuSprGrpKill(freePlayMenuSpriteGroups[i]);
        }
        player = freePlayPlayerCharacterIds[i];
        group = HuSprGrpCreate(2);
        freePlayMenuSpriteGroups[i] = group;
        anim = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 0x10) + GWPlayerCfg[player].character, HU_MEMNUM_OVL));
        sprite = HuSprCreate(anim, 200, 0);
        HuSprGrpMemberSet(group, 0, sprite);
        if (iscom = GWPlayerCfg[player].iscom) {
            anim = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 0x27), HU_MEMNUM_OVL));
            bank = GWPlayerCfg[player].diff;
        }
        else {
            anim = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 0x26), HU_MEMNUM_OVL));
            bank = freePlayPlayerCharacterIds[i];
        }
        sprite = HuSprCreate(anim, 200, bank);
        HuSprGrpMemberSet(group, 1, sprite);
        HuSprPosSet(group, 1, 0, 20);
        HuSprAttrSet(freePlayMenuSpriteGroups[i], 0, HUSPR_ATTR_DISPOFF);
        HuSprAttrSet(freePlayMenuSpriteGroups[i], 1, HUSPR_ATTR_DISPOFF);
    }
    if (freePlayAnimationFrame) {
        HuSprGrpKill(freePlayAnimationFrame);
    }
    group = HuSprGrpCreate(5);
    freePlayAnimationFrame = group;
    anim = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 0x2D), HU_MEMNUM_OVL));
    for (i = 0; i < 5; i++) {
        sprite = HuSprCreate(anim, 5, 0);
        HuSprGrpMemberSet(group, i, sprite);
        HuSprPosSet(group, i, (i * 16) - 32, 0);
    }
    HuSprBankSet(group, 2, 10);
    HuSprGrpPosSet(group, 284, 410);
    SetFreePlayMenuMode(0);
    if (freePlayModeType == 2 && freePlaySpriteGroup == 0) {
        anim = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 0x2B), HU_MEMNUM_OVL));
        freePlaySpriteGroup = HuSprGrpCreate(18);
        for (i = 0; i < 14; i++) {
            sprite = HuSprCreate(anim, 8, 0);
            HuSprGrpMemberSet(freePlaySpriteGroup, i, sprite);
            HuSprColorSet(freePlaySpriteGroup, i, 255, 0, 0);
            HuSprAttrSet(freePlaySpriteGroup, i, HUSPR_ATTR_ADDCOL | HUSPR_ATTR_DISPOFF);
            HuSprTPLvlSet(freePlaySpriteGroup, i, 0.8f);
        }
        for (i = 0; i < 4; i++) {
            anim = HuSprAnimRead(HuDataReadNum(freePlayPlayerGroupRules[i], HU_MEMNUM_OVL));
            sprite = HuSprCreate(anim, 8, 0);
            HuSprGrpMemberSet(freePlaySpriteGroup, i + 14, sprite);
            HuSprAttrSet(freePlaySpriteGroup, i + 14, HUSPR_ATTR_DISPOFF);
        }
        HuSprGrpPosSet(freePlaySpriteGroup, 0, 0);
    }
    freePlayMessageWindow = -1;
    (void)sprite;
}

void SetFreePlayCharacter(s16 playerIndex, s16 playerStateIndex)
{
    s16 digit;
    digit = playerIndex / 10;
    HuSprBankSet(freePlayAnimationFrame, 0, digit);
    playerIndex -= digit * 10;
    HuSprBankSet(freePlayAnimationFrame, 1, playerIndex);
    digit = playerStateIndex / 10;
    HuSprBankSet(freePlayAnimationFrame, 3, digit);
    playerStateIndex -= digit * 10;
    HuSprBankSet(freePlayAnimationFrame, 4, playerStateIndex);
}

void SetFreePlayMenuMode(s32 modeType)
{
    s16 i;
    for (i = 0; i < 5; i++) {
        if (modeType) {
            HuSprAttrReset(freePlayAnimationFrame, i, HUSPR_ATTR_DISPOFF);
        }
        else {
            HuSprAttrSet(freePlayAnimationFrame, i, HUSPR_ATTR_DISPOFF);
        }
    }
}

void ResetFreePlayMenu(void);

void AnimateFreePlayPlayer(s16 playerIndex, s16 playerStateIndex, s16 selectionIndex, s16 groupIndex)
{
    freePlayMenuState = playerIndex;
    freePlayAnimationProgress = playerStateIndex;
    freePlayPlayerCount = selectionIndex;
    freePlayMinigameType = groupIndex;
    HuPrcChildCreate(ResetFreePlayMenu, 100, 12288, 0, HuPrcCurrentGet());
}

void ResetFreePlayMenu(void)
{
    s16 animationFrame;
    s16 menuItemIndex;
    ANIMDATA *selectionDelta;
    s16 titleSpriteId;
    s16 unreferencedMenuScratch;
    s32 animationStep;
    unreferencedMenuScratch = freePlayMessageWindow;
    if (freePlayMenuState < 0) {
        for (menuItemIndex = 1; menuItemIndex <= freePlayAnimationProgress; menuItemIndex++) {
            HuSprGrpTPLvlSet(unreferencedMenuScratch, 1.0 - (menuItemIndex / freePlayAnimationProgress));
            HuPrcVSleep();
        }
        if (unreferencedMenuScratch != -1) {
            HuSprGrpKill(unreferencedMenuScratch);
        }
        freePlayMessageWindow = -1;
        HuPrcEnd();
        while (1) {
            HuPrcVSleep();
        }
    }
    freePlayMessageWindow = HuSprGrpCreate(4);
    animationStep = mgInfoTbl[freePlayMenuState].mg_pic[0];
    if (GWMGAvailGet(freePlayMenuState + 401)) {
        selectionDelta = HuSprAnimRead(HuDataReadNumHeapShortForce(animationStep, HU_MEMNUM_OVL, HEAP_MODEL));
    }
    else {
        selectionDelta = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 0x2C));
    }
    titleSpriteId = HuSprCreate(selectionDelta, 200, 0);
    HuSprGrpMemberSet(freePlayMessageWindow, 0, titleSpriteId);
    HuSprGrpPosSet(freePlayMessageWindow, freePlayPlayerCount, freePlayMinigameType);
    HuSprGrpScaleSet(freePlayMessageWindow, 0.6f, 0.6f);
    animationFrame = 0;
    if (mgInfoTbl[freePlayMenuState].flag & 0x1) {
        selectionDelta = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 0x24));
        titleSpriteId = HuSprCreate(selectionDelta, 190, 0);
        HuSprGrpMemberSet(freePlayMessageWindow, animationFrame + 1, titleSpriteId);
        HuSprPosSet(freePlayMessageWindow, animationFrame + 1, (355.0 / 3.0) - ((animationFrame * 50) * (100.0 / 60.0)), (355.0f / 3.0f));
        HuSprScaleSet(freePlayMessageWindow, animationFrame + 1, 100.0f / 60.0f, 100.0f / 60.0f);
        animationFrame++;
    }
    if (mgInfoTbl[freePlayMenuState].flag & 0x2) {
        selectionDelta = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 0x23));
        titleSpriteId = HuSprCreate(selectionDelta, 190, 0);
        HuSprGrpMemberSet(freePlayMessageWindow, animationFrame + 1, titleSpriteId);
        HuSprPosSet(freePlayMessageWindow, animationFrame + 1, (355.0 / 3.0) - ((animationFrame * 50) * (100.0 / 60.0)), (355.0f / 3.0f));
        HuSprScaleSet(freePlayMessageWindow, animationFrame + 1, 100.0f / 60.0f, 100.0f / 60.0f);
        animationFrame++;
    }
    if (GWMGCustomGet(freePlayMenuState + 401)) {
        selectionDelta = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 0x22));
        titleSpriteId = HuSprCreate(selectionDelta, 190, 0);
        HuSprGrpMemberSet(freePlayMessageWindow, animationFrame + 1, titleSpriteId);
        HuSprPosSet(freePlayMessageWindow, animationFrame + 1, (355.0 / 3.0) - ((animationFrame * 50) * (100.0 / 60.0)), (355.0f / 3.0f));
        HuSprScaleSet(freePlayMessageWindow, animationFrame + 1, 100.0f / 60.0f, 100.0f / 60.0f);
        animationFrame++;
    }
    for (menuItemIndex = 1; menuItemIndex <= freePlayAnimationProgress; menuItemIndex++) {
        if (unreferencedMenuScratch != -1) {
            HuSprGrpTPLvlSet(unreferencedMenuScratch, 1.0 - (menuItemIndex / freePlayAnimationProgress));
        }
        HuSprGrpTPLvlSet(freePlayMessageWindow, menuItemIndex / freePlayAnimationProgress);
        HuPrcVSleep();
    }
    if (unreferencedMenuScratch != -1) {
        HuSprGrpKill(unreferencedMenuScratch);
    }
    freePlayMenuSelection = freePlayMenuState;
    HuPrcEnd();
    while (1) {
        HuPrcVSleep();
    }
}

s16 GetFreePlayPlayerCounts(s16 *playerIndex, s16 *playerStateIndex)
{
    s16 mgNo = GWSystem.mg_next;
    s16 mgType = mgInfoTbl[mgNo].type;
    s16 ret;
    s16 i;
    for (i = ret = 0; i < freePlayVisibleRowCount; i++) {
        if (mgType == freePlayPlayerSelections[i][0]) {
            ret = i;
            break;
        }
    }
    for (i = 0; i < minigameCountByType[mgType]; i++) {
        if (mgNo == minigameIdsByType[mgType][i]) {
            break;
        }
    }
    *playerStateIndex = i;
    i = i - 3;
    if (i < 0) {
        i = 0;
    }
    if (minigameCountByType[mgType] >= 7 && i + 7 >= minigameCountByType[mgType]) {
        i = minigameCountByType[mgType] - 7;
    }
    *playerIndex = i;
    if (minigameCountByType[mgType] < 7) {
        *playerIndex = 0;
    }
    *playerStateIndex = *playerStateIndex - *playerIndex;
    return ret;
}

void SetFreePlayPlayerGroups(void)
{
    if (freePlayModeType == 2) {
        freePlayVisibleRowCount = 5;
        freePlayPlayerSelections[0][0] = 0;
        freePlayPlayerSelections[0][1] = 5;
        freePlayPlayerSelections[1][0] = 1;
        freePlayPlayerSelections[1][1] = 6;
        freePlayPlayerSelections[2][0] = 2;
        freePlayPlayerSelections[2][1] = 7;
        freePlayPlayerSelections[3][0] = 4;
        freePlayPlayerSelections[3][1] = 8;
        freePlayPlayerSelections[4][0] = 9;
        freePlayPlayerSelections[4][1] = 11;
        UpdateFreePlayPlayerSelection();
    }
    else if (freePlayModeSelection == 0 || freePlayModeType == 1) {
        freePlayVisibleRowCount = 7;
        freePlayPlayerSelections[0][0] = 0;
        freePlayPlayerSelections[0][1] = 5;
        freePlayPlayerSelections[1][0] = 1;
        freePlayPlayerSelections[1][1] = 6;
        freePlayPlayerSelections[2][0] = 2;
        freePlayPlayerSelections[2][1] = 7;
        freePlayPlayerSelections[3][0] = 4;
        freePlayPlayerSelections[3][1] = 8;
        freePlayPlayerSelections[4][0] = 3;
        freePlayPlayerSelections[4][1] = 9;
        freePlayPlayerSelections[5][0] = 6;
        freePlayPlayerSelections[5][1] = 10;
        freePlayPlayerSelections[6][0] = 8;
        freePlayPlayerSelections[6][1] = 12;
    }
    else if (freePlayModeSelection == 1) {
        freePlayVisibleRowCount = 1;
        freePlayPlayerSelections[0][0] = 2;
        freePlayPlayerSelections[0][1] = 7;
    }
}

void UpdateFreePlayPlayerSelection(void)
{
    s16 customNum;
    s16 i;
    for (i = customNum = 0; i < 4; i++) {
        s16 j;
        minigameIdsByType[9][customNum++] = -i - 1;
        for (j = 0; j < minigameCountByType[freePlayPlayerSelections[i][0]]; j++) {
            if (GWMGCustomGet(minigameIdsByType[freePlayPlayerSelections[i][0]][j] + 401)) {
                minigameIdsByType[9][customNum++] = minigameIdsByType[freePlayPlayerSelections[i][0]][j];
            }
        }
    }
    minigameCountByType[9] = customNum;
}

s32 GetFreePlayPlayerSelection(void)
{
    char messInsert[4][6];
    s16 customNum[4];
    float winSize[2];
    static char messInsertBase[6] = { "\x1E\x03"
                                      "0"
                                      "\x1E\x08" };
    static s16 customMinNum[4] = { 4, 3, 3, 2 };
    s16 window;
    s16 j;
    s16 i;
    WindowData *windowP;
    s16 choice;
    for (i = 0; i < 4; i++) {
        customNum[i] = 0;
        for (j = 0; j < availableMinigameCountByType[freePlaySelectionOrder[i]]; j++) {
            if (GWMGCustomGet(availableMinigameIdsByType[freePlaySelectionOrder[i]][j] + 401)) {
                customNum[i]++;
            }
        }
    }
    if (customNum[0] >= 4 && customNum[1] >= 3 && customNum[2] >= 3 && customNum[3] >= 2) {
        HuWinMesMaxSizeGet(1, winSize, MAKE_MESSID(0x28, 0x30));
        window = HuWinExCreateStyled(-10000, -10000, winSize[0], winSize[1], -1, 1);
        windowP = &winData[window];
        windowP->active_pad = 1;
        HuWinMesPalSet(window, 7, 0, 0, 0);
        HuWinPriSet(window, 3);
        HuWinExAnimIn(window);
        HuWinMesSet(window, MAKE_MESSID(0x28, 0x30));
        HuWinMesWait(window);
        HuWinExAnimOut(window);
        HuWinExCleanup(window);
        return 1;
    }
    else {
        HuWinInsertMesSizeGet(MAKE_MESSID_PTR(messInsertBase), 0);
        HuWinInsertMesSizeGet(MAKE_MESSID_PTR(messInsertBase), 1);
        HuWinInsertMesSizeGet(MAKE_MESSID_PTR(messInsertBase), 2);
        HuWinInsertMesSizeGet(MAKE_MESSID_PTR(messInsertBase), 3);
        HuWinMesMaxSizeGet(3, winSize, MAKE_MESSID(0x28, 0x2B), MAKE_MESSID(0x28, 0x2C), MAKE_MESSID(0x28, 0x2D));
        window = HuWinExCreateStyled(-10000, -10000, winSize[0], winSize[1], -1, 1);
        windowP = &winData[window];
        windowP->active_pad = 1;
        HuWinMesPalSet(window, 7, 0, 0, 0);
        HuWinPriSet(window, 3);
        HuWinExAnimIn(window);
        for (j = 0; j < 4; j++) {
            strcpy(&messInsert[j][0], messInsertBase);
        }
        for (j = 0; j < 4; j++) {
            if (customNum[j] >= customMinNum[j]) {
                messInsert[j][1] = 5;
            }
            else {
                messInsert[j][2] = (customMinNum[j] + '0' - customNum[j]);
            }
            HuWinInsertMesSet(window, MAKE_MESSID_PTR(&messInsert[j][0]), (s16)j);
        }
        HuWinMesSet(window, MAKE_MESSID(0x28, 0x2C));
        HuWinMesWait(window);
        HuWinMesSet(window, MAKE_MESSID(0x28, 0x2B));
        HuWinMesWait(window);
        HuWinMesSet(window, MAKE_MESSID(0x28, 0x2D));
        HuWinMesWait(window);
        choice = HuWinChoiceGet(window, 0);
        HuWinExAnimOut(window);
        HuWinExCleanup(window);
        if (choice == 0) {
            return 0;
        }
        else {
            return 1;
        }
    }
}

void SetFreePlayGameType(s16 modeType)
{
    s16 i;
    for (i = 0; i < 4; i++) {
        HuSprGrpTPLvlSet(freePlayMenuSpriteGroups[i], 1.0f);
    }
    if (mgInfoTbl[modeType].ovl == DLL_m450dll) {
        HuSprGrpTPLvlSet(freePlayMenuSpriteGroups[1], 0.0f);
    }
    if (mgInfoTbl[modeType].type == 6 || mgInfoTbl[modeType].type == 8) {
        HuSprGrpTPLvlSet(freePlayMenuSpriteGroups[2], 0.0f);
        HuSprGrpTPLvlSet(freePlayMenuSpriteGroups[3], 0.0f);
        return;
    }
}

float freePlayTeamPositions[2][4] = { -132, -12, 60, 132, -132, -60, 60, 132 };

u8 freePlayPlayerOrder[16] = { 0, 1, 2, 3, 1, 0, 2, 3, 2, 0, 1, 3, 3, 0, 1, 2 };

u8 freePlayPlayerAnimationOrder[12] = {
    0,
    1,
    2,
    3,
    0,
    2,
    1,
    3,
    0,
    3,
    1,
    2,
};

s32 CheckFreePlaySelection(s32 value)
{
    s16 animationFrame;
    s16 playerIndex;
    float *playerPositionX;
    s16 playerSpriteGroup;
    ANIMDATA *playerSpriteAnimation;
    s16 playerSpriteId;
    s16 teamSpriteGroup;
    s16 windowId;
    s16 didConfirmTeams;
    WindowData *windowData;
    ANIMDATA *teamSpriteAnimation;

    float animationProgress;
    float transitionProgress;
    float playerPositionY;
    if (mgTypeCurr == 1) {
        playerPositionX = &freePlayTeamPositions[0][0];
        playerPositionY = 216;
    }
    else {
        playerPositionX = &freePlayTeamPositions[1][0];
        playerPositionY = 288;
    }
    playerSpriteGroup = HuSprGrpCreate(16);
    for (animationFrame = 0; animationFrame < 4; animationFrame++) {
        playerSpriteAnimation = HuSprAnimReadFile(minigameSharedState.playerStates[animationFrame].character + DATA_MAKE_NUM(DATADIR_MGMODE, 0x67));
        playerSpriteId = HuSprCreate(playerSpriteAnimation, 4, 0);
        HuSprGrpMemberSet(playerSpriteGroup, 4 * animationFrame, playerSpriteId);
        HuSprPosSet(playerSpriteGroup, 4 * animationFrame, playerPositionX[animationFrame], 0);
        if (mgTypeCurr == 1) {
            if (animationFrame == 0) {
                playerSpriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 0x75));
            }
            else {
                playerSpriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 0x76));
            }
        }
        else {
            if (animationFrame < 2) {
                playerSpriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 0x75));
            }
            else {
                playerSpriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 0x76));
            }
        }
        playerSpriteId = HuSprCreate(playerSpriteAnimation, 3, 0);
        HuSprGrpMemberSet(playerSpriteGroup, (4 * animationFrame) + 1, playerSpriteId);
        HuSprPosSet(playerSpriteGroup, (4 * animationFrame) + 1, playerPositionX[animationFrame], 0);
        if (GWPlayerCfg[animationFrame].iscom) {
            playerSpriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 0x74));
        }
        else {
            playerSpriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 0x70) + GWPlayerCfg[animationFrame].pad_idx);
        }
        playerSpriteId = HuSprCreate(playerSpriteAnimation, 2, 1);
        HuSprGrpMemberSet(playerSpriteGroup, (4 * animationFrame) + 2, playerSpriteId);
        HuSprPosSet(playerSpriteGroup, (4 * animationFrame) + 2, playerPositionX[animationFrame], 0);
        playerSpriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 0x7B));
        playerSpriteId = HuSprCreate(playerSpriteAnimation, 2, GWPlayerCfg[animationFrame].diff);
        HuSprGrpMemberSet(playerSpriteGroup, (4 * animationFrame) + 3, playerSpriteId);
        HuSprPosSet(playerSpriteGroup, (4 * animationFrame) + 3, playerPositionX[animationFrame], 38);
        if (!GWPlayerCfg[animationFrame].iscom) {
            HuSprAttrSet(playerSpriteGroup, (4 * animationFrame) + 3, HUSPR_ATTR_DISPOFF);
        }
    }
    teamSpriteGroup = HuSprGrpCreate(8);
    playerSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 0x65), HU_MEMNUM_OVL));
    teamSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 0x66), HU_MEMNUM_OVL));
    for (animationFrame = 0; animationFrame < 4; animationFrame++) {
        playerSpriteId = HuSprCreate(playerSpriteAnimation, 3, animationFrame);
        HuSprGrpMemberSet(teamSpriteGroup, (animationFrame * 2), playerSpriteId);
        HuSprPosSet(teamSpriteGroup, (animationFrame * 2), ((animationFrame % 2) * 380) - 190, ((animationFrame / 2) * 156) - 68);
        playerSpriteId = HuSprCreate(teamSpriteAnimation, 4, animationFrame);
        HuSprGrpMemberSet(teamSpriteGroup, (animationFrame * 2) + 1, playerSpriteId);
        HuSprTPLvlSet(teamSpriteGroup, (animationFrame * 2) + 1, 0.5f);
        HuSprPosSet(teamSpriteGroup, (animationFrame * 2) + 1, ((animationFrame % 2) * 380) - 190, ((animationFrame / 2) * 156) - 60);
    }
    espTPLvlSet(mgModeSpriteIds[22], 0.9f);
    espAttrReset(mgModeSpriteIds[22], HUSPR_ATTR_DISPOFF);
    espAttrReset(mgModeSpriteIds[24], HUSPR_ATTR_DISPOFF);
    espAttrReset(mgModeSpriteIds[23], HUSPR_ATTR_DISPOFF);
    espAttrReset(mgModeSpriteIds[25], HUSPR_ATTR_DISPOFF);
    espAttrReset(mgModeSpriteIds[26], HUSPR_ATTR_DISPOFF);
    espBankSet(mgModeSpriteIds[25], 1);
    espPosSet(mgModeSpriteIds[22], 288, 240);
    for (animationFrame = 0; animationFrame <= 30; animationFrame++) {
        if (animationFrame <= 20) {
            transitionProgress = animationFrame / 20.0;
            animationProgress = -300 + (560 * sind(transitionProgress * 90.0f));
            espPosSet(mgModeSpriteIds[22], 288, animationProgress);
            espPosSet(mgModeSpriteIds[24], 288, animationProgress - 50);
            espPosSet(mgModeSpriteIds[23], playerPositionY, animationProgress + 20);
            espPosSet(mgModeSpriteIds[26], 108, animationProgress + 20);
            espPosSet(mgModeSpriteIds[25], 468, animationProgress + 20);
            HuSprGrpPosSet(playerSpriteGroup, 288, animationProgress + 20);
            HuSprGrpPosSet(teamSpriteGroup, 288, animationProgress);
        }
        else {
            transitionProgress = (animationFrame - 20) / 10.0;
            animationProgress = 240 + (20 * cosd(transitionProgress * 90.0f));
            espPosSet(mgModeSpriteIds[22], 288, animationProgress);
            espPosSet(mgModeSpriteIds[24], 288, animationProgress - 50);
            espPosSet(mgModeSpriteIds[23], playerPositionY, animationProgress + 20);
            espPosSet(mgModeSpriteIds[26], 108, animationProgress + 20);
            espPosSet(mgModeSpriteIds[25], 468, animationProgress + 20);
            HuSprGrpPosSet(playerSpriteGroup, 288, animationProgress + 20);
            HuSprGrpPosSet(teamSpriteGroup, 288, animationProgress);
        }
        HuPrcVSleep();
    }
    windowId = CreateCenteredMessageWindow(0x1A0020);
    windowData = &winData[windowId];
    HuWinPriSet(windowId, 5);
    HuWinPosSet(windowId, (576.0f - windowData->w) / 2.0f, 310.0f);
    playerIndex = 0;
    didConfirmTeams = 0;
    while (1) {
        s16 unreferencedInputScratch;
        HuPrcVSleep();
        if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            HuAudFXPlay(3);
            goto loopEnd;
        }
        unreferencedInputScratch = 0;
        if (HuPadDStkRep[0] & PAD_BUTTON_LEFT) {
            HuAudFXPlay(772);
            for (animationFrame = 0; animationFrame <= 5; animationFrame++) {
                transitionProgress = animationFrame / 5.0;
                espPosSet(mgModeSpriteIds[26], 108 - (10 * sind(transitionProgress * 90.0f)), 20 + animationProgress);
                HuPrcVSleep();
            }
            playerIndex--;
            if (mgTypeCurr == 1) {
                if (playerIndex < 0) {
                    playerIndex = 3;
                }
                if (playerIndex >= 4) {
                    playerIndex = 0;
                }
                for (animationFrame = 0; animationFrame < 4; animationFrame++) {
                    HuSprPosSet(playerSpriteGroup, freePlayPlayerOrder[(playerIndex * 4) + animationFrame] * 4, playerPositionX[animationFrame], 0);
                    HuSprPosSet(playerSpriteGroup, (freePlayPlayerOrder[(playerIndex * 4) + animationFrame] * 4) + 2, playerPositionX[animationFrame], 0);
                    HuSprPosSet(playerSpriteGroup, (freePlayPlayerOrder[(playerIndex * 4) + animationFrame] * 4) + 3, playerPositionX[animationFrame], 38);
                }
            }
            else {
                if (playerIndex < 0) {
                    playerIndex = 2;
                }
                if (playerIndex >= 3) {
                    playerIndex = 0;
                }
                for (animationFrame = 0; animationFrame < 4; animationFrame++) {
                    HuSprPosSet(playerSpriteGroup, freePlayPlayerAnimationOrder[(playerIndex * 4) + animationFrame] * 4, playerPositionX[animationFrame], 0);
                    HuSprPosSet(playerSpriteGroup, (freePlayPlayerAnimationOrder[(playerIndex * 4) + animationFrame] * 4) + 2, playerPositionX[animationFrame], 0);
                    HuSprPosSet(playerSpriteGroup, (freePlayPlayerAnimationOrder[(playerIndex * 4) + animationFrame] * 4) + 3, playerPositionX[animationFrame], 38);
                }
            }
            for (animationFrame = 0; animationFrame <= 5; animationFrame++) {
                transitionProgress = animationFrame / 5.0;
                espPosSet(mgModeSpriteIds[26], 108 - (10 * cosd(transitionProgress * 90.0f)), 20 + animationProgress);
                HuPrcVSleep();
            }
        }
        else if (HuPadDStkRep[0] & PAD_BUTTON_RIGHT) {
            HuAudFXPlay(772);
            for (animationFrame = 0; animationFrame <= 5; animationFrame++) {
                transitionProgress = animationFrame / 5.0;
                espPosSet(mgModeSpriteIds[25], 468 + (10 * sind(transitionProgress * 90.0f)), 20 + animationProgress);
                HuPrcVSleep();
            }
            playerIndex++;
            if (mgTypeCurr == 1) {
                if (playerIndex < 0) {
                    playerIndex = 3;
                }
                if (playerIndex >= 4) {
                    playerIndex = 0;
                }
                for (animationFrame = 0; animationFrame < 4; animationFrame++) {
                    HuSprPosSet(playerSpriteGroup, freePlayPlayerOrder[(playerIndex * 4) + animationFrame] * 4, playerPositionX[animationFrame], 0);
                    HuSprPosSet(playerSpriteGroup, (freePlayPlayerOrder[(playerIndex * 4) + animationFrame] * 4) + 2, playerPositionX[animationFrame], 0);
                    HuSprPosSet(playerSpriteGroup, (freePlayPlayerOrder[(playerIndex * 4) + animationFrame] * 4) + 3, playerPositionX[animationFrame], 38);
                }
            }
            else {
                if (playerIndex < 0) {
                    playerIndex = 2;
                }
                if (playerIndex >= 3) {
                    playerIndex = 0;
                }
                for (animationFrame = 0; animationFrame < 4; animationFrame++) {
                    HuSprPosSet(playerSpriteGroup, freePlayPlayerAnimationOrder[(playerIndex * 4) + animationFrame] * 4, playerPositionX[animationFrame], 0);
                    HuSprPosSet(playerSpriteGroup, (freePlayPlayerAnimationOrder[(playerIndex * 4) + animationFrame] * 4) + 2, playerPositionX[animationFrame], 0);
                    HuSprPosSet(playerSpriteGroup, (freePlayPlayerAnimationOrder[(playerIndex * 4) + animationFrame] * 4) + 3, playerPositionX[animationFrame], 38);
                }
            }
            for (animationFrame = 0; animationFrame <= 5; animationFrame++) {
                transitionProgress = animationFrame / 5.0;
                espPosSet(mgModeSpriteIds[25], 468 + (10 * cosd(transitionProgress * 90.0f)), 20 + animationProgress);
                HuPrcVSleep();
            }
        }
        if (HuPadBtnDown[0] & (PAD_BUTTON_A | PAD_BUTTON_START)) {
            break;
        }
    }
    HuAudFXPlay(773);
    if (mgTypeCurr == 1) {
        GWPlayerCfg[freePlayPlayerOrder[playerIndex * 4]].group = 0;
        for (animationFrame = 1; animationFrame < 4; animationFrame++) {
            GWPlayerCfg[freePlayPlayerOrder[(playerIndex * 4) + animationFrame]].group = 1;
        }
    }
    else {
        for (animationFrame = 0; animationFrame < 2; animationFrame++) {
            GWPlayerCfg[freePlayPlayerAnimationOrder[(playerIndex * 4) + animationFrame]].group = 0;
        }
        for (animationFrame = 2; animationFrame < 4; animationFrame++) {
            GWPlayerCfg[freePlayPlayerAnimationOrder[(playerIndex * 4) + animationFrame]].group = 1;
        }
    }
    if (value != 0) {
        didConfirmTeams = 1;
    }
    else {
        return 1;
    }
loopEnd:
    CloseCenteredMessageWindow(windowId);
    for (animationFrame = 0; animationFrame <= 30; animationFrame++) {
        if (animationFrame <= 10) {
            transitionProgress = animationFrame / 10.0;
            animationProgress = 240 + (20 * sind(transitionProgress * 90.0f));
            espPosSet(mgModeSpriteIds[22], 288, animationProgress);
            espPosSet(mgModeSpriteIds[24], 288, animationProgress - 50);
            espPosSet(mgModeSpriteIds[23], playerPositionY, animationProgress + 20);
            espPosSet(mgModeSpriteIds[26], 108, animationProgress + 20);
            espPosSet(mgModeSpriteIds[25], 468, animationProgress + 20);
            HuSprGrpPosSet(playerSpriteGroup, 288, animationProgress + 20);
            HuSprGrpPosSet(teamSpriteGroup, 288, animationProgress);
        }
        else {
            transitionProgress = (animationFrame - 10) / 20.0;
            animationProgress = -300 + (560 * cosd(transitionProgress * 90.0f));
            espPosSet(mgModeSpriteIds[22], 288, animationProgress);
            espPosSet(mgModeSpriteIds[24], 288, animationProgress - 50);
            espPosSet(mgModeSpriteIds[23], playerPositionY, animationProgress + 20);
            espPosSet(mgModeSpriteIds[26], 108, animationProgress + 20);
            espPosSet(mgModeSpriteIds[25], 468, animationProgress + 20);
            HuSprGrpPosSet(playerSpriteGroup, 288, animationProgress + 20);
            HuSprGrpPosSet(teamSpriteGroup, 288, animationProgress);
        }
        HuPrcVSleep();
    }
    HuSprGrpKill(playerSpriteGroup);
    HuSprGrpKill(teamSpriteGroup);
    return didConfirmTeams;
}

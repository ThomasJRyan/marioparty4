#include "REL/mgmodedll.h"

#include "game/audio.h"
#include "game/chrman.h"
#include "game/gamework_data.h"
#include "game/minigame_seq.h"
#include "game/wipe.h"

#include "ext_math.h"

#ifndef __MWERKS__
#include "game/frand.h"
#endif

s16 battleInstructionWindow;
s32 battleSelectionState;
s16 battlePlayerChoices[4];
s16 battleStarSelection;
s16 battlePlayerStarCounts[4];
s16 battleStarSpriteGroup;
s16 battleSelectionSpriteGroup;

s16 battleMessageWindow = -1;

void CreateBattleSelectionSprites(void);
s32 RunBattlePlayerSetup(void);
s32 RunBattleStarSetup(void);
void InitializeBattlePlayerModels(void);
void ExitMinigameMode(s16 modeType);

s32 RunBattleMinigameSelection(void)
{
    s16 animationFrame;
    s16 playerIndex;
    s16 selectedMode;
    s16 selectionDelta;
    s16 choiceResult;
    s32 asyncReadHandle;
    HU3DMODEL *modelWork;
    float animationProgress;
    float rotationAngle;
    Vec modelPosition;
    Vec targetPosition;
    Vec transitionPosition;
    selectedMode = 0;
    BuildMinigameTypeLists(0);
    CreateBattleSelectionSprites();
    if (mgModeEvtNo == 3) {
        mgTypeCurr = mgInfoTbl[GWSystem.mg_next].type;
        if (mgTypeCurr >= 4) {
            mgTypeCurr = 3;
        }
        HuSprGrpPosSet(battleSelectionSpriteGroup, 288, 240);
        HuSprGrpDrawNoSet(battleSelectionSpriteGroup, 64);
        Hu3DModelPosSet(mgModeModelIds[5], -350, 0, 680);
        Hu3DModelRotSet(mgModeModelIds[5], 0, 20, 0);
        CreatePlayerCharacterModels();
        for (animationFrame = 0; animationFrame < 4; animationFrame++) {
            Hu3DModelPosSet(playerModelIdsByGroup[animationFrame], (animationFrame * 130) - 195, 0, 750);
            Hu3DMotionSet(playerModelIdsByGroup[animationFrame], playerMotionIdsByGroup[animationFrame][0]);
            Hu3DModelAttrSet(playerModelIdsByGroup[animationFrame], HU3D_MOTATTR_LOOP);
        }
        for (animationFrame = 0; animationFrame < 4; animationFrame++) {
            HuSprAttrSet(battleSelectionSpriteGroup, (animationFrame * 2) + 2, HUSPR_ATTR_DISPOFF);
            HuSprAttrSet(battleSelectionSpriteGroup, (animationFrame * 2) + 3, HUSPR_ATTR_DISPOFF);
        }
        HuSprAttrReset(battleSelectionSpriteGroup, mgTypeCurr + 19, HUSPR_ATTR_DISPOFF);
        HuSprPosSet(battleSelectionSpriteGroup, mgTypeCurr + 19, 0, -155);
        HuSprPosSet(battleSelectionSpriteGroup, 1, 0, -180);
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
        if (RunBattlePlayerSetup() && !RunBattleStarSetup()) {
            return 1;
        }
    }
    else {
        CharModelDataClose(-1);
        if (HuDataReadChk(DATADIR_INSTPIC) == -1) {
            asyncReadHandle = HuDataDirReadAsync(DATADIR_INSTPIC);
        }
        else {
            asyncReadHandle = -1;
        }
        for (animationFrame = 0; animationFrame <= 60; animationFrame++) {
            if (animationFrame <= 50) {
                animationProgress = animationFrame / 50.0;
                HuSprGrpPosSet(battleSelectionSpriteGroup, 288, -400.0 + (656.0 * sind(animationProgress * 90.0f)));
            }
            else {
                animationProgress = (animationFrame - 50) / 10.0;
                HuSprGrpPosSet(battleSelectionSpriteGroup, 288, 240.0 + (16.0 * cosd(animationProgress * 90.0f)));
            }
            HuPrcVSleep();
        }
        #if VERSION_PAL
        if(battleMessageWindow == -1) {
            battleMessageWindow = CreateStandardMessageWindow(0, 0, 0);
            HuWinMesSpeedSet(battleMessageWindow, 1);
            ShowMessageAndWait(windowId, 0x390001, -1, -1);
        }
        #endif
        for (animationFrame = 0; animationFrame <= 5; animationFrame++) {
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                if (playerIndex != selectedMode) {
                    HuSprTPLvlSet(battleSelectionSpriteGroup, (playerIndex * 2) + 2, 1 - (animationFrame / 5.0));
                }
            }
            HuPrcVSleep();
        }
        if (asyncReadHandle != -1) {
            while (!HuDataGetAsyncStat(asyncReadHandle)) {
                HuPrcVSleep();
            }
        }
    repeatMode:
        if (characterAssetsLoaded == 0) {
            HuPrcChildCreate(LoadMinigameCharacterAssets, 100, 12288, 0, objman);
            characterAssetsLoaded = 1;
        }
        battleInstructionWindow = CreateCenteredMessageWindow(0x1A0020);
        #if VERSION_NTSC
        if (battleMessageWindow == -1) {
            battleMessageWindow = CreateStandardMessageWindow(0, 0, 0);
        }
        HuWinMesSpeedSet(battleMessageWindow, 1);
        ShowMessageAndWait(windowId, 0x28000F, -1, -1);
        #else
        if(battleMessageWindow == -1) {
            battleMessageWindow = CreateStandardMessageWindow(0, 0, 0);
            HuWinMesSpeedSet(battleMessageWindow, 1);
            ShowMessageAndWait(windowId, 0x390001, -1, -1);
        }
        ShowMessageAndWait(windowId, 0x390002+messageId, -1, -999);
        #endif
        while (1) {
            selectionDelta = 0;
            if ((HuPadDStkRep[0] & PAD_BUTTON_UP) && selectedMode > 0) {
                selectionDelta = -1;
            }
            else if ((HuPadDStkRep[0] & PAD_BUTTON_DOWN) && selectedMode < 3) {
                selectionDelta = 1;
            }
            if (selectionDelta != 0) {
                HuSprAttrReset(battleSelectionSpriteGroup, (selectedMode * 2) + 3, HUSPR_ATTR_DISPOFF);
                for (animationFrame = 0; animationFrame <= 10; animationFrame++) {
                    HuSprTPLvlSet(battleSelectionSpriteGroup, (selectedMode * 2) + 2, 1 - (animationFrame / 10.0));
                    if (animationFrame == 5) {
                        HuAudFXPlay(0);
                        HuSprTPLvlSet(battleSelectionSpriteGroup, ((selectedMode + selectionDelta) * 2) + 2, 1);
                        HuSprAttrReset(battleSelectionSpriteGroup, ((selectedMode + selectionDelta) * 2) + 2, HUSPR_ATTR_DISPOFF);
                        #if VERSION_PAL
                        ShowMessageAndWait(windowId, 0x390002+selectedMode+messageId, -1, -999);
                        #endif
                    }
                    HuPrcVSleep();
                }
                selectedMode += selectionDelta;
            }
            if (HuPadBtnDown[0] & PAD_BUTTON_B) {
                HuAudFXPlay(3);
                CloseCenteredMessageWindow(windowId);
                CloseStandardMessageWindow(windowId);
                battleMessageWindow = -1;
                for (animationFrame = 0; animationFrame <= 60; animationFrame++) {
                    if (animationFrame <= 10) {
                        animationProgress = animationFrame / 10.0;
                        HuSprGrpPosSet(battleSelectionSpriteGroup, 288, 240.0 + (16.0 * sind(animationProgress * 90.0f)));
                    }
                    else {
                        animationProgress = (animationFrame - 10) / 50.0;
                        HuSprGrpPosSet(battleSelectionSpriteGroup, 288, -400.0 + (656.0 * cosd(animationProgress * 90.0f)));
                    }
                    HuPrcVSleep();
                }
                HuSprGrpKill(battleSelectionSpriteGroup);
                battleSelectionSpriteGroup = 0;
                return 0;
            }
            if (HuPadBtnDown[0] & PAD_BUTTON_A) {
                HuAudFXPlay(1);
                if (availableMinigameCountByType[freePlaySelectionOrder[selectedMode]]) {
                    break;
                }
                ShowMessageAndWait(windowId, 0x28002E, -1, -1);
                ShowMessageAndWait(windowId, 0x28000F, -1, -1);
                continue;
            }
            HuPrcVSleep();
        }
        CloseCenteredMessageWindow(windowId);
        CloseStandardMessageWindow(windowId);
        battleMessageWindow = battleInstructionWindow = -1;
        mgTypeCurr = selectedMode;
        if (selectedMode == 1 || selectedMode == 2) {
            if (CheckFreePlaySelection(1) == 0) {
                goto repeatMode;
            }
        }
        else {
            for (animationFrame = 0; animationFrame < 4; animationFrame++) {
                GWPlayerCfg[animationFrame].group = animationFrame;
            }
        }
        for (animationFrame = 0; animationFrame <= 10; animationFrame++) {
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                HuSprTPLvlSet(battleSelectionSpriteGroup, (playerIndex * 2) + 3, 1 - (animationFrame / 10.0));
            }
            HuPrcVSleep();
        }
        Hu3DMotionTimeSet(mgModeModelIds[5], 0);
        HuPrcVSleep();
        selectionDelta = (selectedMode * 40) - 110;
        modelWork = &Hu3DData[mgModeModelIds[5]];
        modelPosition = modelWork->pos;
        targetPosition.x = -350;
        targetPosition.y = 0;
        targetPosition.z = 680;
        VECSubtract(&targetPosition, &modelPosition, &targetPosition);
        rotationAngle = (180 + atan2d(targetPosition.z, targetPosition.x)) - 360;
        Hu3DMotionShiftSet(mgModeModelIds[5], mgModeMotionIds[7], 0, 8, HU3D_MOTATTR_LOOP);
        for (animationFrame = 0; animationFrame <= 30; animationFrame++) {
            if (animationFrame <= 20) {
                animationProgress = animationFrame / 20.0;
                HuSprPosSet(battleSelectionSpriteGroup, (selectedMode * 2) + 2, 0, (sind(animationProgress * 90.0f) * (-150 - selectionDelta)) + selectionDelta);
            }
            animationProgress = animationFrame / 30.0;
            VECScale(&targetPosition, &transitionPosition, animationProgress);
            VECAdd(&modelPosition, &transitionPosition, &transitionPosition);
            Hu3DModelPosSetV(mgModeModelIds[5], &transitionPosition);
            if (animationFrame <= 10) {
                animationProgress = animationFrame / 10.0;
                Hu3DModelRotSet(mgModeModelIds[5], 0, rotationAngle * animationProgress, 0);
            }
            HuPrcVSleep();
        }
        HuSprGrpDrawNoSet(battleSelectionSpriteGroup, 64);
        Hu3DMotionShiftSet(mgModeModelIds[5], mgModeMotionIds[6], 0, 8, HU3D_MOTATTR_LOOP);
        for (animationFrame = 0; animationFrame <= 10; animationFrame++) {
            animationProgress = animationFrame / 10.0;
            Hu3DModelRotSet(mgModeModelIds[5], 0, rotationAngle + ((20 - rotationAngle) * animationProgress), 0);
            HuPrcVSleep();
        }
        while (characterAssetsLoaded) {
            HuPrcVSleep();
        }
        Hu3DMotionTimeSet(mgModeModelIds[5], 0);
        HuPrcVSleep();
        CreatePlayerCharacterModels();
        for (animationFrame = 0; animationFrame < 4; animationFrame++) {
            CharMotionSet(playerCharacterIdsByGroup[animationFrame], playerMotionIdsByGroup[animationFrame][2]);
            Hu3DModelAttrSet(playerModelIdsByGroup[animationFrame], HU3D_MOTATTR_LOOP);
            Hu3DModelRotSet(playerModelIdsByGroup[animationFrame], 0, -90, 0);
        }
        for (animationFrame = 0; animationFrame <= 30; animationFrame++) {
            animationProgress = animationFrame / 30.0f;
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                Hu3DModelPosSet(playerModelIdsByGroup[playerIndex],
                    (sind(animationProgress * 90.0f) * (((playerIndex * 130) - 195) - ((playerIndex * 130) + 400))) + ((playerIndex * 130) + 400), 0, 750);
                if ((animationFrame + (playerIndex * 2)) % 5 == 0) {
                    HuAudFXPlay(GWPlayerCfg[playerIndex].character + 213);
                }
            }
            HuPrcVSleep();
        }
        for (animationFrame = 0; animationFrame < 4; animationFrame++) {
            Hu3DMotionShiftSet(playerModelIdsByGroup[animationFrame], playerMotionIdsByGroup[animationFrame][0], 0, 10, HU3D_MOTATTR_LOOP);
        }
        for (animationFrame = 0; animationFrame <= 10; animationFrame++) {
            animationProgress = animationFrame / 10.0f;
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                Hu3DModelRotSet(playerModelIdsByGroup[playerIndex], 0, -90.0 * (1.0 - animationProgress), 0);
            }
            HuPrcVSleep();
        }
        for (animationFrame = 0; animationFrame < 4; animationFrame++) {
            mgBattleStar[animationFrame] = 0;
        }
    }
    if (battleMessageWindow == -1) {
        battleMessageWindow = CreateStandardMessageWindow(0, 0, 0);
    }
    ShowMessageAndWait(windowId, 0x280010, -1, -1);
    HuSprTPLvlSet(battleSelectionSpriteGroup, 10, 1);
    for (playerIndex = 0; playerIndex < 35;) {
        for (animationFrame = 0; animationFrame < availableMinigameCountByType[freePlaySelectionOrder[mgTypeCurr]]; animationFrame++) {
            SetMinigamePreviewSprite(availableMinigameIdsByType[freePlaySelectionOrder[mgTypeCurr]][animationFrame], 288, 210);
            HuAudFXPlay(855);
            HuPrcSleep(3);
            playerIndex++;
            if (playerIndex >= 35) {
                break;
            }
        }
    }
    HuAudFXPlay(870);
    freePlayWindowState = availableMinigameIdsByType[freePlaySelectionOrder[mgTypeCurr]][frandmod(availableMinigameCountByType[freePlaySelectionOrder[mgTypeCurr]])];
    SetMinigamePreviewSprite(spriteGroupId, 288, 210);
    HuWinAttrSet(battleMessageWindow, 0x10);
    HuWinInsertMesSet(battleMessageWindow, mgInfoTbl[freePlayWindowState].name_mess, 0);
    ShowMessageAndWait(windowId, 0x280011, -1, -1);
repeatMess:
    ShowMessageAndWait(windowId, 0x280029, -1, -1);
    choiceResult = HuWinChoiceGet(battleMessageWindow, 0);
    if (choiceResult == 0) {
        _ClearFlag(11);
    }
    else if (choiceResult == 1) {
        HuWinInsertMesSet(battleMessageWindow, MAKE_MESSID(0x28, 39), 0);
        ShowMessageAndWait(windowId, 0x280026, -1, -1);
        choiceResult = HuWinChoiceGet(battleMessageWindow, 1);
        if (choiceResult == 1) {
            goto repeatMess;
        }
        SetMinigamePreviewSprite(-1, 0, 0);
        for (animationFrame = 1; animationFrame <= 10; animationFrame++) {
            HuSprTPLvlSet(battleSelectionSpriteGroup, 10, 1 - (animationFrame / 10.0));
            HuPrcVSleep();
        }
        CloseStandardMessageWindow(windowId);
        battleMessageWindow = battleInstructionWindow = -1;
        InitializeBattlePlayerModels();
        return 1;
    }
    else {
        _SetFlag(11);
    }
    GWSystem.mg_next = freePlayWindowState;
    HuDataDirClose(DATADIR_INSTPIC);
    ExitMinigameMode(3);
    while (1) {
        HuPrcVSleep();
    }
}

s16 battlePlayerDisplayOrder[4] = { 0, 2, 1, 3 };
GXColor battleStarCountOptions[4] = {
    { 255, 64, 64, 255 },
    { 128, 255, 128, 255 },
    { 128, 255, 255, 255 },
    { 255, 255, 128, 255 },
};

void CreateBattleSelectionSprites(void)
{
    s16 playerIndex;
    s16 spriteGroupId;
    s16 spriteId;
    ANIMDATA *spriteAnimation;
    s16 starCountFrame;
    ANIMDATA *starGlowAnimation;
    if (battleSelectionSpriteGroup == 0) {
        spriteGroupId = HuSprGrpCreate(23);
        battleSelectionSpriteGroup = spriteGroupId;
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 125), HU_MEMNUM_OVL));
        spriteId = HuSprCreate(spriteAnimation, 200, 0);
        HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
        HuSprTPLvlSet(spriteGroupId, 0, 0.8f);
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 54), HU_MEMNUM_OVL));
        if (mgBattleStarMax == 3) {
            starCountFrame = 0;
        }
        else if (mgBattleStarMax == 5) {
            starCountFrame = 1;
        }
        else {
            starCountFrame = 2;
        }
        spriteId = HuSprCreate(spriteAnimation, 190, starCountFrame);
        HuSprGrpMemberSet(spriteGroupId, 1, spriteId);
        HuSprPosSet(spriteGroupId, 1, 0, -180);
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 55) + (battlePlayerDisplayOrder[playerIndex] * 3), HU_MEMNUM_OVL));
            spriteId = HuSprCreate(spriteAnimation, 190, 0);
            HuSprGrpMemberSet(spriteGroupId, (2 * playerIndex) + 2, spriteId);
            HuSprPosSet(spriteGroupId, (2 * playerIndex) + 2, 0, (playerIndex * 40) - 110);
            spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 56) + (battlePlayerDisplayOrder[playerIndex] * 3), HU_MEMNUM_OVL));
            spriteId = HuSprCreate(spriteAnimation, 195, 0);
            HuSprGrpMemberSet(spriteGroupId, (2 * playerIndex) + 3, spriteId);
            HuSprPosSet(spriteGroupId, (2 * playerIndex) + 3, 0, (playerIndex * 40) - 110);
            spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 57) + (battlePlayerDisplayOrder[playerIndex] * 3), HU_MEMNUM_OVL));
            spriteId = HuSprCreate(spriteAnimation, 195, 0);
            HuSprGrpMemberSet(spriteGroupId, playerIndex + 19, spriteId);
        }
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 37), HU_MEMNUM_OVL));
        spriteId = HuSprCreate(spriteAnimation, 195, 0);
        HuSprGrpMemberSet(spriteGroupId, 10, spriteId);
        HuSprPosSet(spriteGroupId, 10, 0, -32);
        HuSprScaleSet(spriteGroupId, 10, 0.9f, 0.9f);
        HuSprTPLvlSet(spriteGroupId, 10, 0);
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 101), HU_MEMNUM_OVL));
        starGlowAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 102), HU_MEMNUM_OVL));
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            spriteId = HuSprCreate(spriteAnimation, 180, playerIndex);
            HuSprGrpMemberSet(spriteGroupId, (playerIndex * 2) + 11, spriteId);
            HuSprPosSet(spriteGroupId, (playerIndex * 2) + 11, ((playerIndex % 2) * 350) - 175, ((playerIndex / 2) * 220) - 180);
            spriteId = HuSprCreate(starGlowAnimation, 190, playerIndex);
            HuSprGrpMemberSet(spriteGroupId, (playerIndex * 2) + 12, spriteId);
            HuSprTPLvlSet(spriteGroupId, (playerIndex * 2) + 12, 0.5f);
            HuSprPosSet(spriteGroupId, (playerIndex * 2) + 12, ((playerIndex % 2) * 350) - 175, ((playerIndex / 2) * 220) - 172);
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        HuSprTPLvlSet(battleSelectionSpriteGroup, (playerIndex * 2) + 2, 1);
        HuSprAttrSet(battleSelectionSpriteGroup, playerIndex + 19, HUSPR_ATTR_DISPOFF);
    }
    HuSprGrpDrawNoSet(battleSelectionSpriteGroup, 0);
}

void DestroyBattleSelectionSprites(void)
{
    s16 playerIndex;
    s16 starDisplayGroup;
    s16 starIndex;
    s16 starGridLeft;
    s16 spriteId;
    s16 starGridTop;
    s16 starCountFrame;
    ANIMDATA *playerSpriteAnimation;
    s16 playerSpriteId;
    ANIMDATA *starSpriteAnimation;
    s16 unreferencedPlayerIndexScratch[4];
    if (mgBattleStarMax == 3) {
        starCountFrame = 0;
    }
    else if (mgBattleStarMax == 5) {
        starCountFrame = 1;
    }
    else {
        starCountFrame = 2;
    }
    starDisplayGroup = HuSprGrpCreate(35);
    battleStarSpriteGroup = starDisplayGroup;
    playerSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 70) + starCountFrame, HU_MEMNUM_OVL));
    spriteId = HuSprCreate(playerSpriteAnimation, 195, 0);
    HuSprGrpMemberSet(starDisplayGroup, 33, spriteId);
    HuSprAttrSet(starDisplayGroup, 33, HUSPR_ATTR_DISPOFF);
    spriteId = HuSprCreate(playerSpriteAnimation, 195, 0);
    HuSprGrpMemberSet(starDisplayGroup, 34, spriteId);
    HuSprAttrSet(starDisplayGroup, 34, HUSPR_ATTR_DISPOFF);
    starSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 73), HU_MEMNUM_OVL));
    starGridLeft = -(((mgBattleStarMax + 1) * 42) / 2) + 20;
    starGridTop = -64;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 74) + playerCharacterIdsByGroup[playerIndex], HU_MEMNUM_OVL));
        spriteId = HuSprCreate(playerSpriteAnimation, 190, 0);
        HuSprGrpMemberSet(starDisplayGroup, (playerIndex * 8) + 0, spriteId);
        HuSprPosSet(starDisplayGroup, (playerIndex * 8) + 0, starGridLeft, starGridTop + (playerIndex * 42));
        for (starIndex = 0; starIndex < 7; starIndex++) {
            spriteId = HuSprCreate(starSpriteAnimation, 190, 0);
            HuSprGrpMemberSet(starDisplayGroup, 1 + (playerIndex * 8) + starIndex, spriteId);
            HuSprPosSet(starDisplayGroup, 1 + (playerIndex * 8) + starIndex, 42 + starGridLeft + (starIndex * 42), starGridTop + (playerIndex * 42));
            if (mgBattleStar[playerOrderByGroup[playerIndex]] <= starIndex) {
                HuSprScaleSet(starDisplayGroup, 1 + (playerIndex * 8) + starIndex, 0, 0);
            }
            else {
                HuSprScaleSet(starDisplayGroup, 1 + (playerIndex * 8) + starIndex, 1, 1);
            }
            if (mgTypeCurr == 1) {
                if (playerIndex > 2) {
                    HuSprAttrSet(starDisplayGroup, 1 + (playerIndex * 8) + starIndex, HUSPR_ATTR_DISPOFF);
                }
                if (playerIndex == 1) {
                    HuSprPosSet(starDisplayGroup, 1 + (playerIndex * 8) + starIndex, 42 + starGridLeft + (starIndex * 42), starGridTop + 84);
                }
            }
            else if (mgTypeCurr == 2) {
                if (playerIndex % 2 == 1) {
                    HuSprAttrSet(starDisplayGroup, 1 + (playerIndex * 8) + starIndex, HUSPR_ATTR_DISPOFF);
                }
                if (playerIndex == 0) {
                    HuSprPosSet(starDisplayGroup, 1 + (playerIndex * 8) + starIndex, 42 + starGridLeft + (starIndex * 42), starGridTop + 21);
                }
                if (playerIndex == 2) {
                    HuSprPosSet(starDisplayGroup, 1 + (playerIndex * 8) + starIndex, 42 + starGridLeft + (starIndex * 42), starGridTop + 105);
                }
            }
        }
    }
    starGridLeft = -(((mgBattleStarMax + 1) * 42) / 2) + 22;
    if (starCountFrame == 0) {
        starGridLeft += 63;
    }
    else if (starCountFrame == 1) {
        starGridLeft += 105;
    }
    else {
        starGridLeft += 146;
    }
    if (mgTypeCurr == 1) {
        HuSprPosSet(starDisplayGroup, 33, starGridLeft + 21, starGridTop + 63);
        HuSprAttrReset(starDisplayGroup, 33, HUSPR_ATTR_DISPOFF);
        HuSprPosSet(starDisplayGroup, 34, starGridLeft + 21, starGridTop + 105);
        HuSprAttrReset(starDisplayGroup, 34, HUSPR_ATTR_DISPOFF);
    }
    else if (mgTypeCurr == 2) {
        HuSprPosSet(starDisplayGroup, 33, starGridLeft + 21, starGridTop + 21);
        HuSprAttrReset(starDisplayGroup, 33, HUSPR_ATTR_DISPOFF);
        HuSprPosSet(starDisplayGroup, 34, starGridLeft + 21, starGridTop + 105);
        HuSprAttrReset(starDisplayGroup, 34, HUSPR_ATTR_DISPOFF);
    }
    playerSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 67) + starCountFrame, HU_MEMNUM_OVL));
    spriteId = HuSprCreate(playerSpriteAnimation, 200, 0);
    HuSprGrpMemberSet(starDisplayGroup, 32, spriteId);
    HuSprGrpPosSet(starDisplayGroup, 288, 185);
    HuSprGrpDrawNoSet(starDisplayGroup, 64);
    HuSprGrpScaleSet(starDisplayGroup, 1, 0);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        battlePlayerStarCounts[playerIndex] = 0;
    }
    switch (mgTypeCurr) {
        case 0:
            playerSpriteId = -1;
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                unreferencedPlayerIndexScratch[playerIndex] = GWPlayerCoinWinGet(playerIndex) + GWPlayerCoinCollectGet(playerIndex);
                if (playerSpriteId < unreferencedPlayerIndexScratch[playerIndex]) {
                    playerSpriteId = unreferencedPlayerIndexScratch[playerIndex];
                }
            }
            battleStarSelection = 0;
            if (playerSpriteId != 0) {
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if (playerSpriteId == unreferencedPlayerIndexScratch[playerIndex]) {
                        battlePlayerChoices[battleStarSelection] = playerIndex;
                        battlePlayerStarCounts[playerIndex] = 1;
                        battleStarSelection++;
                    }
                }
            }
            break;

        case 1:
        case 2:
            unreferencedPlayerIndexScratch[0] = unreferencedPlayerIndexScratch[1] = 0;
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                unreferencedPlayerIndexScratch[GWPlayerCfg[playerIndex].group] += GWPlayerCoinWinGet(playerIndex) + GWPlayerCoinCollectGet(playerIndex);
            }
            if (mgInfoTbl[GWSystem.mg_next].ovl == DLL_m422dll) {
                unreferencedPlayerIndexScratch[1] /= 3;
            }
            battleStarSelection = 0;
            if (unreferencedPlayerIndexScratch[0] != 0 || unreferencedPlayerIndexScratch[1] != 0) {
                if (unreferencedPlayerIndexScratch[0] != unreferencedPlayerIndexScratch[1]) {
                    if (unreferencedPlayerIndexScratch[0] > unreferencedPlayerIndexScratch[1]) {
                        starIndex = 0;
                    }
                    else {
                        starIndex = 1;
                    }
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        if (starIndex == GWPlayerCfg[playerOrderByGroup[playerIndex]].group) {
                            battlePlayerChoices[battleStarSelection] = playerIndex;
                            battlePlayerStarCounts[playerIndex] = 1;
                            battleStarSelection++;
                        }
                    }
                }
                else {
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        battlePlayerChoices[battleStarSelection] = playerIndex;
                        battlePlayerStarCounts[playerIndex] = 1;
                        battleStarSelection++;
                    }
                }
            }
            break;

        case 3:
            for (playerIndex = battleStarSelection = 0; playerIndex < 4; playerIndex++) {
                if (!GWPlayerCoinWinGet(playerOrderByGroup[playerIndex])) {
                    battlePlayerChoices[battleStarSelection] = playerIndex;
                    battlePlayerStarCounts[playerIndex] = 1;
                    battleStarSelection++;
                }
            }
            break;

        default:
            break;
    }
}

void ShowBattlePlayerStatus(s16 playerIndex);

s32 RunBattlePlayerSetup(void)
{
    s16 animationFrame;
    s16 playerIndex;
    s16 winningPlayerCount;
    s16 playerIndex;
    float scaleProgress;

    s16 winningPlayerIndices[4];
    s16 reachedStarGoal[4];
    s16 winnerCharacterIds[4];

    HuSprTPLvlSet(battleSelectionSpriteGroup, 10, 0);
    while (WipeStatGet()) {
        HuPrcVSleep();
    }
    DestroyBattleSelectionSprites();
    for (animationFrame = 0; animationFrame <= 30; animationFrame++) {
        if (animationFrame <= 20) {
            scaleProgress = animationFrame / 20.0f;
            HuSprGrpScaleSet(battleStarSpriteGroup, 1, 1.1 * sind(scaleProgress * 90.0f));
        }
        else {
            scaleProgress = (animationFrame - 20) / 10.0f;
            HuSprGrpScaleSet(battleStarSpriteGroup, 1, 1.0 + (0.1 * cosd(scaleProgress * 90.0f)));
        }
        HuPrcVSleep();
    }
    HuPrcSleep(30);
    if (battleMessageWindow == -1) {
        battleMessageWindow = CreateStandardMessageWindow(0, 0, 0);
    }
    if (battleStarSelection) {
        for (animationFrame = 0; animationFrame < battleStarSelection; animationFrame++) {
            HuWinInsertMesSet(battleMessageWindow, playerCharacterIdsByGroup[battlePlayerChoices[animationFrame]], (s32)animationFrame);
        }
        if (battleStarSelection == 1) {
            ShowMessageAndWait(windowId, 0x280012, -1, -1);
        }
        else if (battleStarSelection == 2) {
            ShowMessageAndWait(windowId, 0x28001F, -1, -1);
        }
        else if (battleStarSelection == 3) {
            ShowMessageAndWait(windowId, 0x280020, -1, -1);
        }
        else if (battleStarSelection == 4) {
            ShowMessageAndWait(windowId, 0x280021, -1, -1);
        }
        HuAudFXPlay(825);
        HuAudFXPlay(826);
        for (animationFrame = 0; animationFrame <= 30; animationFrame++) {
            scaleProgress = (1 - sind((animationFrame / 30.0f) * 90.0f)) + 1;
            for (playerIndex = 0; playerIndex < battleStarSelection; playerIndex++) {
                playerIndex = battlePlayerChoices[playerIndex];
                HuSprScaleSet(battleStarSpriteGroup, 1 + (playerIndex * 8) + mgBattleStar[playerOrderByGroup[playerIndex]], scaleProgress, scaleProgress);
            }
            if (animationFrame == 10) {
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    if (battlePlayerStarCounts[playerIndex]) {
                        Hu3DMotionShiftSet(playerModelIdsByGroup[playerIndex], playerMotionIdsByGroup[playerIndex][3], 0, 5, HU3D_MOTATTR_NONE);
                    }
                    else {
                        Hu3DMotionShiftSet(playerModelIdsByGroup[playerIndex], playerMotionIdsByGroup[playerIndex][4], 0, 5, HU3D_MOTATTR_NONE);
                    }
                }
            }
            HuPrcVSleep();
        }
        for (animationFrame = 0; animationFrame < 4; animationFrame++) {
            reachedStarGoal[animationFrame] = 0;
        }
        for (playerIndex = winningPlayerCount = 0; playerIndex < battleStarSelection; playerIndex++) {
            mgBattleStar[playerOrderByGroup[battlePlayerChoices[playerIndex]]]++;
            if (mgBattleStar[playerOrderByGroup[battlePlayerChoices[playerIndex]]] >= mgBattleStarMax) {
                winningPlayerIndices[winningPlayerCount] = battlePlayerChoices[playerIndex];
                reachedStarGoal[battlePlayerChoices[playerIndex]] = 1;
                winningPlayerCount++;
            }
        }
        if (winningPlayerCount != 0) {
            for (animationFrame = 0; animationFrame < winningPlayerCount; animationFrame++) {
                HuWinInsertMesSet(battleMessageWindow, playerCharacterIdsByGroup[winningPlayerIndices[animationFrame]], (s32)animationFrame);
            }
            if (winningPlayerCount == 1) {
                ShowMessageAndWait(windowId, 0x280014, -1, -1);
            }
            else if (winningPlayerCount == 2) {
                ShowMessageAndWait(windowId, 0x280022, -1, -1);
            }
            else if (winningPlayerCount == 3) {
                ShowMessageAndWait(windowId, 0x280023, -1, -1);
            }
            else if (winningPlayerCount == 4) {
                ShowMessageAndWait(windowId, 0x280024, -1, -1);
            }
            for (animationFrame = 0; animationFrame < 4; animationFrame++) {
                if (animationFrame < winningPlayerCount) {
                    winnerCharacterIds[animationFrame] = playerCharacterIdsByGroup[winningPlayerIndices[animationFrame]];
                }
                else {
                    winnerCharacterIds[animationFrame] = -1;
                }
            }
            MGSeqAltWinCreate(winnerCharacterIds[0], winnerCharacterIds[1], winnerCharacterIds[2], winnerCharacterIds[3]);
            HuAudFXPlay(2175);
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                if (reachedStarGoal[playerIndex]) {
                    Hu3DMotionShiftSet(playerModelIdsByGroup[playerIndex], playerMotionIdsByGroup[playerIndex][5], 0, 5, HU3D_MOTATTR_NONE);
                    ShowBattlePlayerStatus(playerIndex);
                }
                else {
                    Hu3DMotionShiftSet(playerModelIdsByGroup[playerIndex], playerMotionIdsByGroup[playerIndex][6], 0, 5, HU3D_MOTATTR_NONE);
                }
            }
            HuPrcSleep(120);
            return 1;
        }
        else {
            HuPrcSleep(30);
            for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                Hu3DMotionShiftSet(playerModelIdsByGroup[playerIndex], playerMotionIdsByGroup[playerIndex][0], 0, 5, HU3D_MOTATTR_LOOP);
            }
            HuPrcSleep(30);
        }
    }
    else {
        ShowMessageAndWait(windowId, 0x280013, -1, -1);
    }
    for (animationFrame = 0; animationFrame <= 30; animationFrame++) {
        if (animationFrame <= 10) {
            scaleProgress = animationFrame / 10.0f;
            HuSprGrpScaleSet(battleStarSpriteGroup, 1, 1.0 + (0.1 * sind(scaleProgress * 90.0f)));
        }
        else {
            scaleProgress = (animationFrame - 10) / 20.0f;
            HuSprGrpScaleSet(battleStarSpriteGroup, 1, 1.1 * cosd(scaleProgress * 90.0f));
        }
        HuPrcVSleep();
    }
    return 0;
}

s32 RunBattleStarSetup(void)
{
    s16 choice;
    ShowMessageAndWait(windowId, 0x28000C, -1, -1);
    HuWinAttrSet(battleMessageWindow, 0x10);
    choice = HuWinChoiceGet(battleMessageWindow, 1);
    battleSelectionState = 0;
    CloseStandardMessageWindow(windowId);
    battleMessageWindow = -1;
    if (choice == 0) {
        s16 i;
        s16 j;
        float weight;
        for (i = 0; i < 4; i++) {
            Hu3DMotionShiftSet(playerModelIdsByGroup[i], playerMotionIdsByGroup[i][0], 0, 10, HU3D_MOTATTR_LOOP);
        }
        for (i = 0; i <= 20; i++) {
            weight = i / 20.0f;
            HuSprGrpScaleSet(battleStarSpriteGroup, 1, cosd(weight * 90.0f));
            HuPrcVSleep();
        }
        for (i = 0; i < 4; i++) {
            mgBattleStar[i] = 0;
            for (j = 0; j < 7; j++) {
                HuSprScaleSet(battleStarSpriteGroup, j + (i * 8) + 1, 0, 0);
            }
        }
        for (i = 0; i <= 20; i++) {
            weight = i / 20.0f;
            HuSprGrpScaleSet(battleStarSpriteGroup, 1, sind(weight * 90.0f));
            HuPrcVSleep();
        }
        for (i = 0; i <= 20; i++) {
            weight = i / 20.0f;
            HuSprGrpScaleSet(battleStarSpriteGroup, 1, cosd(weight * 90.0f));
            HuPrcVSleep();
        }
        return 1;
    }
    else {
        InitializeBattlePlayerModels();
        return 0;
    }
}

void InitializeBattlePlayerModels(void)
{
    s16 i;
    s16 j;
    HU3DMODEL *model;
    float weight;
    float angle;

    Vec pos;
    Vec posEnd;
    Vec posModel;
    for (i = 0; i < 4; i++) {
        Hu3DMotionShiftSet(playerModelIdsByGroup[i], playerMotionIdsByGroup[i][2], 0, 10, HU3D_MOTATTR_LOOP);
    }
    model = &Hu3DData[mgModeModelIds[5]];
    pos = model->pos;
    posEnd.x = 0;
    posEnd.y = 0;
    posEnd.z = 640;
    VECSubtract(&posEnd, &pos, &posEnd);
    angle = atan2d(posEnd.z, posEnd.x) + 180;
    for (i = 0; i <= 30; i++) {
        for (j = 0; j < 4; j++) {
            if (i <= 10) {
                weight = i / 10.0;
                Hu3DModelRotSet(playerModelIdsByGroup[j], 0, weight * 90.0f, 0);
            }
            weight = i / 30.0;
            Hu3DModelPosSet(playerModelIdsByGroup[j], ((j * 130) - 195) + ((1 - cosd(weight * 90.0f)) * 700.0), 0, 750);
            if ((i + (j * 2)) % 5 == 0) {
                HuAudFXPlay(GWPlayerCfg[j].character + 213);
            }
        }
        if (i == 20) {
            Hu3DMotionShiftSet(mgModeModelIds[5], mgModeMotionIds[7], 0, 8, HU3D_MOTATTR_LOOP);
        }
        if (i >= 20) {
            weight = (i - 20) / 10.0;
            Hu3DModelRotSet(mgModeModelIds[5], 0, 20.0f + ((angle - 20.0f) * weight), 0);
        }
        HuPrcVSleep();
    }
    for (i = 0; i < 4; i++) {
        Hu3DMotionShiftSet(playerModelIdsByGroup[i], playerMotionIdsByGroup[i][0], 0, 10, HU3D_MOTATTR_LOOP);
    }
    for (i = 0; i <= 30; i++) {
        weight = i / 30.0;
        VECScale(&posEnd, &posModel, weight);
        VECAdd(&pos, &posModel, &posModel);
        Hu3DModelPosSetV(mgModeModelIds[5], &posModel);
        HuPrcVSleep();
    }
    Hu3DMotionShiftSet(mgModeModelIds[5], mgModeMotionIds[6], 0, 10, HU3D_MOTATTR_LOOP);
    for (i = 0; i <= 10; i++) {
        weight = 1.0 - (i / 10.0);
        Hu3DModelRotSet(mgModeModelIds[5], 0, angle * weight, 0);
        HuPrcVSleep();
    }
    for (i = 0; i <= 60; i++) {
        if (i <= 10) {
            weight = i / 10.0;
            HuSprGrpPosSet(battleSelectionSpriteGroup, 288, 240.0 + (16.0 * sind(weight * 90.0f)));
            if (battleStarSpriteGroup) {
                HuSprGrpPosSet(battleStarSpriteGroup, 288, 185.0 + (16.0 * sind(weight * 90.0f)));
            }
        }
        else {
            weight = (i - 10) / 50.0;
            weight = (1 - cosd(weight * 90.0f)) * 400;
            HuSprGrpPosSet(battleSelectionSpriteGroup, 288, 256 - weight);
            if (battleStarSpriteGroup) {
                HuSprGrpPosSet(battleStarSpriteGroup, 288, 201 - weight);
            }
        }

        HuPrcVSleep();
    }
    HuDataDirClose(DATADIR_INSTPIC);
    if (battleSelectionSpriteGroup) {
        HuSprGrpKill(battleSelectionSpriteGroup);
    }
    if (battleStarSpriteGroup) {
        HuSprGrpKill(battleStarSpriteGroup);
    }
    battleSelectionSpriteGroup = battleStarSpriteGroup = 0;
    for (i = 0; i < 4; i++) {
        CharModelKill(GWPlayerCfg[i].character);
    }
}

void HideBattlePlayerStatus(void);

void ShowBattlePlayerStatus(s16 playerIndex)
{
    HUPROCESS *process;
    s32 *data;
    process = HuPrcChildCreate(HideBattlePlayerStatus, 100, 12288, 0, objman);
    data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(s32), HU_MEMNUM_OVL);
    process->user_data = data;
    *data = playerIndex;
    battleSelectionState = 1;
    (void)process;
}

void HideBattlePlayerStatus(void)
{
    s16 particleIndex;
    HU3DMODEL *particleModel;
    HSFMATERIAL *material;
    s32 *playerIndexData;
    s16 starModelId;
    s16 particleModelId;
    s16 starFadeFrame;
    s16 materialIndex;
    s16 particleHorizontalOffset;
    s16 inactiveParticleCount;
    HUPROCESS *particleProcess;
    float starFadeScale;
    Vec particleVelocity[30];
    s16 particleModelIds[30];

    particleProcess = HuPrcCurrentGet();
    playerIndexData = particleProcess->user_data;
    starModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MGMODE, 82));
    Hu3DModelLayerSet(starModelId, 3);
    Hu3DModelPosSet(starModelId, (*playerIndexData * 130) - 195, 300, 750);
    Hu3DModelScaleSet(starModelId, 0.8f, 0.8f, 0.8f);
    for (particleIndex = 0; particleIndex < 30; particleIndex++) {
        if (particleIndex < 4) {
            particleModelId = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MGMODE, 83));
            particleModelIds[particleIndex] = particleModelId;
            particleModel = &Hu3DData[particleModelId];
            material = particleModel->hsf->material;
            for (materialIndex = 0; materialIndex < particleModel->hsf->materialNum; materialIndex++, material++) {
                material->color[0] = battleStarCountOptions[particleIndex].r;
                material->color[1] = battleStarCountOptions[particleIndex].g;
                material->color[2] = battleStarCountOptions[particleIndex].b;
                material->litColor[0] = battleStarCountOptions[particleIndex].r;
                material->litColor[1] = battleStarCountOptions[particleIndex].g;
                material->litColor[2] = battleStarCountOptions[particleIndex].b;
            }
        }
        else {
            particleModelId = Hu3DModelLink(particleModelIds[particleIndex % 4]);
            particleModelIds[particleIndex] = particleModelId;
        }
        Hu3DModelPosSet(particleModelId, (frandmod(50) - 220 + (*playerIndexData * 130)), (particleIndex * 12) + 350, 750);
        Hu3DModelScaleSet(particleModelId, 2.0f, 2.0f, 2.0f);
        Hu3DModelAttrSet(particleModelId, HU3D_ATTR_DISPOFF);
        Hu3DModelLayerSet(particleModelId, 3);
        particleVelocity[particleIndex].x = (float)(frandmod(50) - 25) * 0.05;
        particleVelocity[particleIndex].z = (float)(frandmod(50) - 25) * 0.05;
    }
    HuPrcSleep(45);
    starFadeFrame = 0;
    while (1) {
        for (particleIndex = inactiveParticleCount = 0; particleIndex < 30; particleIndex++) {
            particleModel = &Hu3DData[particleModelIds[particleIndex]];
            if (particleModel->pos.y > 350.0f) {
                Hu3DModelAttrSet(particleModelIds[particleIndex], HU3D_ATTR_DISPOFF);
            }
            else {
                Hu3DModelAttrReset(particleModelIds[particleIndex], HU3D_ATTR_DISPOFF);
                particleModel->pos.x += particleVelocity[particleIndex].x;
                particleModel->pos.z += particleVelocity[particleIndex].z;
            }
            particleModel->pos.y -= 3.0f;
            particleModel->rot.x += ((particleIndex % 3) + 1) * 5;
            particleModel->rot.y += ((particleIndex % 5) + 1) * 5;
            if (particleModel->pos.y < 0.0f) {
                if (battleSelectionState == 0) {
                    Hu3DModelAttrSet(particleModelIds[particleIndex], HU3D_ATTR_DISPOFF);
                    inactiveParticleCount++;
                }
                else {
                    particleModel->pos.y = 350;
                    particleHorizontalOffset = frandmod(50) - 25;
                    particleModel->pos.x = (-195 + (*playerIndexData * 130)) + particleHorizontalOffset;
                    particleModel->pos.z = 750;
                    particleVelocity[particleIndex].x = ((float)frandmod(50) * 0.02) * ((particleHorizontalOffset < 0) ? -1 : (particleHorizontalOffset > 0) ? 1 : 0);
                    particleVelocity[particleIndex].z = (float)(frandmod(50) - 25) * 0.05;
                }
            }
        }
        if (battleSelectionState || inactiveParticleCount != 30) {
            if (battleSelectionState == 0 && starFadeFrame <= 10) {
                starFadeScale = 1 - sind((starFadeFrame / 10.0) * 90.0);
                if (0.0 == starFadeScale) {
                    Hu3DModelAttrSet(starModelId, HU3D_ATTR_DISPOFF);
                }
                Hu3DModelScaleSet(starModelId, starFadeScale, 1, 1);
                starFadeFrame++;
            }
            HuPrcVSleep();
        }
        else {
            break;
        }
    }
    (void)playerIndexData;
    (void)playerIndexData;
    (void)starModelId;
    (void)starModelId;
    Hu3DModelKill(starModelId);
    HuPrcVSleep();
    HuPrcEnd();
    while (1) {
        HuPrcVSleep();
    }
}

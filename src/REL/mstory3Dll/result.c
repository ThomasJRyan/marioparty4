#include "REL/mstory3Dll.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/data.h"

#ifndef __MWERKS__
#include "game/hsfex.h"
#endif

#include "game/esprite.h"
#include "game/flag.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/printfunc.h"
#include "game/saveload.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

extern s32 rand8(Void);

typedef struct {
    /* 0x00 */ s32 playerIndex;
    /* 0x04 */ s32 team;
    /* 0x08 */ s32 isComputer;
    /* 0x0C */ s32 difficulty;
    /* 0x10 */ s32 characterId;
    /* 0x14 */ s32 controllerPort;
    /* 0x18 */ s32 rank;
    /* 0x1C */ char unreferencedPlayerRecordPadding[0x18];
    /* 0x34 */ s32 minigameResults[4][2];
    /* 0x54 */ s32 blueSpaceCount;
    /* 0x58 */ char unreferencedBlueSpaceCountPadding[4];
    /* 0x5C */ s32 redSpaceCount;
    /* 0x60 */ char unreferencedRedSpaceCountPadding[4];
    /* 0x64 */ s32 questionSpaceCount;
    /* 0x68 */ char unreferencedQuestionSpaceCountPadding[4];
    /* 0x6C */ s32 fortuneSpaceCount;
    /* 0x70 */ char unreferencedFortuneSpaceCountPadding[4];
    /* 0x74 */ s32 bowserSpaceCount;
    /* 0x78 */ char unreferencedBowserSpaceCountPadding[4];
    /* 0x7C */ s32 battleSpaceCount;
    /* 0x80 */ char unreferencedBattleSpaceCountPadding[4];
    /* 0x84 */ s32 mushroomSpaceCount;
    /* 0x88 */ char unreferencedMushroomSpaceCountPadding[4];
    /* 0x8C */ s32 warpSpaceCount;
    /* 0x90 */ char unreferencedWarpSpaceCountPadding[4];
} StoryResultPlayerStats; // Size 0x94

typedef struct {
    /* 0x000 */ s32 boardId;
    /* 0x004 */ s32 currentPlayerIndex;
    /* 0x008 */ s32 turnCount;
    /* 0x00C */ s32 messageWindowId;
    /* 0x010 */ s32 selectionMessageId;
    /* 0x014 */ s32 winnerPlayerIndex;
    /* 0x018 */ s32 playerRecordCount;
    /* 0x01C */ char unreferencedResultStatePadding[8];
    /* 0x024 */ s32 selectionRepeatTimer;
    /* 0x028 */ char unreferencedResultStatePadding28[0x10];
    /* 0x038 */ StoryResultPlayerStats playerResults[4];
    /* 0x288 */ Vec playerModelPositions[5];
    /* 0x2C4 */ s32 recordScrollOffset;
    /* 0x2C8 */ s32 selectedRecordPage;
    /* 0x2CC */ s32 teamMode;
    /* 0x2D0 */ s32 winningTeam;
    /* 0x2D4 */ s32 resultMode;
} StoryResultState; // Size 0x2D8

Void Story3LoadResultRecordSprites(Void);
Void Story3ArrangeResultPlayers(Void);
Void Story3CreateResultTitleObject(Void);
Void Story3ShowPlayerModels(Void);
Void Story3HidePlayerModels(Void);
Void Story3UpdateRecordPageRows(Void);
Void Story3HideRecordRowSprites(Void);
Void Story3CreateRecordPageObject(Void);
Void Story3FindHighestMinigameScore(Void);
Void Story3SortPlayersByRank(s32 playerCount);
Void Story3PositionPlayerModels(Void);
Void Story3PositionPlayerSprites(Void);
Void Story3ShowRecordSprites(Void);
Void Story3HideRecordSprites(Void);
Void Story3AddPlayerRecord(Void);
Void Story3StartCharacterModelMotion(Void);
Void Story3ShowResultPlayerModels(Void);
Void Story3HideResultPlayerModels(Void);
Void Story3ShowBoardFlag(Void);
Void Story3HideBoardFlag(Void);
Void Story3CreateBoardFlagObject(Void);
Void Story3ResetResultCamera(Void);

StoryResultState resultState;
StorySequenceProcessState savePromptProcessState;
StorySequenceProcessState recordPageProcessState;
StorySequenceProcessState playerModelProcessState;
StorySequenceProcessState playerRecordProcessStates[4];
ANIMDATA* recordStarAnimations[2];
ANIMDATA* recordMedalAnimations[2];
ANIMDATA* resultDecorationAnimations[3];
s32 resultSelectionState;
HUPROCESS* resultProcess;

Void Story3StartBoardResult(HUPROCESS* objectManager, s32 count, s32 mode, s32 mode3) {
    s32 playerIndex;

    resultProcess = objectManager;
    resultState.boardId = count;
    resultState.turnCount = GWSystem.max_turn;
    mode3 = resultState.winnerPlayerIndex = story3FindFirstHumanPlayer();
    resultState.resultMode = mode;
    resultState.teamMode = GWSystem.team;
    if (_CheckFlag(FLAG_ID_MAKE(1, 29))) {
        resultState.winningTeam = 0;
    } else {
        resultState.winningTeam = 1;
    }
    Story3InitializeWindows();
    Story3CreateGlobalLight(0xFF);
    Story3InitializeResultCamera(resultProcess, Story3ResetResultCamera);
    Story3LoadResultRecordSprites();
    Story3SortPlayersByRank(mode3);
    Story3FindHighestMinigameScore();
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        HuPrcChildCreate(Story3AddPlayerRecord, 100, 0x2000, 0, HuPrcCurrentGet());
    }
    HuPrcChildCreate(Story3CreateRecordPageObject, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(Story3CreateBoardFlagObject, 100, 0x2000, 0, HuPrcCurrentGet());
    HuPrcChildCreate(Story3CreateResultTitleObject, 90, 0x2000, 0, HuPrcCurrentGet());
}

Void Story3CollectPlayerResults(Void) {
    s32 handicapAdjustedStars;
    s32 handicapAdjustedMaxStars;
    s32 i;

    for (i = 0; i < 4; i++) {
        handicapAdjustedStars = GWPlayer[i].stars - GWPlayerHandicapGet(i);
        if (handicapAdjustedStars <= 0) {
            handicapAdjustedStars = 0;
        }
        GWTotalStarsAdd(handicapAdjustedStars);
    }
    GWBoardPlayCountAdd(resultState.boardId, 1);
    if (resultState.teamMode == 1) {
        if (resultState.playerResults[0].isComputer == 0) {
            GWBoardWinCountInc(resultState.playerResults[0].characterId, resultState.boardId);
        }
        if (resultState.playerResults[1].isComputer == 0) {
            GWBoardWinCountInc(resultState.playerResults[1].characterId, resultState.boardId);
        }
    } else if (resultState.playerResults[0].isComputer == 0) {
        GWBoardWinCountInc(resultState.playerResults[0].characterId, resultState.boardId);
    }
    for (i = 0; i < 4; i++) {
        handicapAdjustedMaxStars = GWPlayer[i].stars_max - GWPlayerHandicapGet(i);
        if (handicapAdjustedMaxStars <= 0) {
            handicapAdjustedMaxStars = 0;
        }
        if (GWBoardMaxStarsGet(resultState.boardId) <= handicapAdjustedMaxStars && !GWPlayer[i].com) {
            GWBoardMaxStarsSet(resultState.boardId, handicapAdjustedMaxStars);
        }
    }
    for (i = 0; i < 4; i++) {
        if (GWBoardMaxCoinsGet(resultState.boardId) <= GWPlayer[i].coins_max && !GWPlayer[i].com) {
            GWBoardMaxCoinsSet(resultState.boardId, GWPlayer[i].coins_max);
        }
    }
}

Void Story3LoadResultRecordSprites(Void) {
    recordStarAnimations[0] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 95), HU_MEMNUM_OVL));
    recordStarAnimations[1] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 96), HU_MEMNUM_OVL));
    resultDecorationAnimations[0] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 93), HU_MEMNUM_OVL));
    resultDecorationAnimations[1] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 94), HU_MEMNUM_OVL));
    resultDecorationAnimations[2] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 97), HU_MEMNUM_OVL));
    recordMedalAnimations[0] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 136), HU_MEMNUM_OVL));
    recordMedalAnimations[1] = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 137), HU_MEMNUM_OVL));
}

Void Story3PositionCenteredSprite(s32 spriteGroupId, s32 spriteIndex, s32 leftX, s32 topY, s32 width, s32 height) {
    HuSprPosSet(spriteGroupId, spriteIndex, leftX + width / 2, topY + height / 2);
}

Void Story3DrawNumberSprites(s32 spriteGroupId, s32 firstSpriteIndex, s32 value, s32 leftX, s32 topY, s32 digitWidth, s32 digitHeight, s32 minimumDigits) {
    s32 digitCount;
    s32 digitValue;

    digitCount = 0;
    if (minimumDigits < 2) {
        if ((digitValue = value / 100) == 0) {
            if (minimumDigits != 0) {
                digitCount++;
            }
            HuSprBankSet(spriteGroupId, firstSpriteIndex, digitValue);
            Story3PositionCenteredSprite(spriteGroupId, firstSpriteIndex, -1000, -1000, digitWidth, digitHeight);
        } else {
            HuSprBankSet(spriteGroupId, firstSpriteIndex, digitValue);
            Story3PositionCenteredSprite(spriteGroupId, firstSpriteIndex, leftX + digitWidth * digitCount, topY, digitWidth, digitHeight);
            digitCount++;
        }
    } else {
        firstSpriteIndex--;
    }
    if ((digitValue = (value % 100) / 10) == 0 && value / 100 == 0) {
        if (minimumDigits != 0) {
            digitCount++;
        }
        HuSprBankSet(spriteGroupId, firstSpriteIndex + 1, digitValue);
        Story3PositionCenteredSprite(spriteGroupId, firstSpriteIndex + 1, -1000, -1000, digitWidth, digitHeight);
    } else {
        HuSprBankSet(spriteGroupId, firstSpriteIndex + 1, digitValue);
        Story3PositionCenteredSprite(spriteGroupId, firstSpriteIndex + 1, leftX + digitWidth * digitCount, topY, digitWidth, digitHeight);
        digitCount++;
    }
    digitValue = value % 10;
    HuSprBankSet(spriteGroupId, firstSpriteIndex + 2, digitValue);
    Story3PositionCenteredSprite(spriteGroupId, firstSpriteIndex + 2, leftX + digitWidth * digitCount, topY, digitWidth, digitHeight);
    digitCount++;
}

Void Story3DrawResultDebugOverlay(Void) {
    s32 debugWindowX = 16;
    s32 debugWindowY = 40;
    s32 debugOverlayEnabled = 0;
    GXColor debugWindowBackgroundColor = { 0x00, 0x00, 0x80, 0x80 };
    s32 i;

    while (TRUE) {
        Story3SleepOneFrame();
        if (HuPadBtnDown[0] & PAD_TRIGGER_R) {
            debugOverlayEnabled++;
            debugOverlayEnabled %= 2;
        }
        if (debugOverlayEnabled == 0) {
            continue;
        }
        printWin(debugWindowX, debugWindowY, 220, 150, &debugWindowBackgroundColor);
        if (savePromptProcessState.callback == NULL) {
            print8(debugWindowX, debugWindowY, 1.0f, "PROC_MAIN    -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY, 1.0f, "PROC_MAIN    -> TRUE");
        }
        if (recordPageProcessState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 10, 1.0f, "PROC_MAP     -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 10, 1.0f, "PROC_MAP     -> TRUE");
        }
        if (playerModelProcessState.callback == NULL) {
            print8(debugWindowX, debugWindowY + 20, 1.0f, "PROC_SUB     -> FALSE");
        } else {
            print8(debugWindowX, debugWindowY + 20, 1.0f, "PROC_SUB     -> TRUE");
        }
        for (i = 0; i < 4; i++) {
            if (playerRecordProcessStates[i].callback == NULL) {
                print8(debugWindowX, debugWindowY + 30 + i * 10, 1.0f, "PROC_PLAYER%d -> FALSE", i);
            } else {
                print8(debugWindowX, debugWindowY + 30 + i * 10, 1.0f, "PROC_PLAYER%d -> TRUE", i);
            }
        }
        print8(debugWindowX, debugWindowY + 80, 1.0f, "M:%d P:%d W:%d", resultState.boardId, resultState.currentPlayerIndex, resultState.winnerPlayerIndex);
        print8(debugWindowX, debugWindowY + 100, 1.0f, "NO) IDX:GRP:COM:DIF:PAD:CHR");
        for (i = 0; i < 4; i++) {
            print8(debugWindowX, debugWindowY + 110 + i * 10, 1.0f, "%2d) %3d:%3d:%3d:%3d:%3d:%3d", i,
                resultState.playerResults[i].playerIndex, resultState.playerResults[i].team, resultState.playerResults[i].isComputer,
                resultState.playerResults[i].difficulty, resultState.playerResults[i].controllerPort, resultState.playerResults[i].characterId);
        }
    }
}

Void Story3BeginResultSequence(Void) {
    Story3SleepFrames(30);
    Story3ArrangeResultPlayers();
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
    do {
        Story3SleepOneFrame();
    } while (WipeStatGet() != 0);
    Story3SleepFrames(60);
}

Void Story3WaitForPlayerArrivals(Void) {
    s32 playerArrivalStates[5];
    s32 i;

    for (i = 0; i < 5; i++) {
        playerArrivalStates[i] = 0;
        Story3SetModelMotion(playerModelProcessState.object, i * 3 + 1, i * 3 + 2, 0, 1);
        Hu3DData[playerModelProcessState.object->mdlId[i * 3 + 1]].rot.y = -90.0f;
    }
    do {
        Story3SleepOneFrame();
        for (i = 0; i < 5; i++) {
            if (playerArrivalStates[i] == 0) {
                resultState.playerModelPositions[i].x -= 5.0f;
                if (i == 0) {
                    if (resultState.playerModelPositions[i].x <= -175.0f) {
                        playerArrivalStates[i] = 1;
                        resultState.playerModelPositions[i].x = -175.0f;
                        Story3SetModelMotion(playerModelProcessState.object, i * 3 + 1, i * 3 + 1, 15, 1);
                    }
                } else {
                    if (resultState.playerModelPositions[i].x <= -240.0f) {
                        playerArrivalStates[i] = 1;
                        resultState.playerModelPositions[i].x = -240.0f;
                        Story3SetModelMotion(playerModelProcessState.object, i * 3 + 1, i * 3 + 1, 15, 1);
                    }
                }
            } else if (playerArrivalStates[i] == 1) {
                Hu3DData[playerModelProcessState.object->mdlId[i * 3 + 1]].rot.y = interpolateSmoothAverage(Hu3DData[playerModelProcessState.object->mdlId[i * 3 + 1]].rot.y, 17.5f, 10.0f);
                if (Hu3DData[playerModelProcessState.object->mdlId[i * 3 + 1]].rot.y >= 14.9f) {
                    playerArrivalStates[i] = 2;
                }
            }
        }
        for (i = 0; i < 5; i++) {
            if (playerArrivalStates[i] != 2) {
                break;
            }
        }
    } while (i != 5);
    resultState.messageWindowId = story3CreateDataWindow(MAKE_MESSID(38, 12), 0);
}

s32 Story3WaitForResultInput(Void) {
    s32 skipResultSequence;
    s32 inputWaitFrames;

    skipResultSequence = 0;
    inputWaitFrames = 0;
    while (TRUE) {
        Story3SleepOneFrame();
        if (resultState.winnerPlayerIndex != -1) {
            if (HuPadBtnDown[resultState.playerResults[resultState.currentPlayerIndex].controllerPort] & PAD_BUTTON_A) {
                HuAudFXPlay(0x1C);
                break;
            } else if (HuPadBtnDown[resultState.playerResults[resultState.currentPlayerIndex].controllerPort] & PAD_BUTTON_MENU) {
                HuAudFXPlay(1);
                skipResultSequence = 1;
                break;
            }
        } else if (inputWaitFrames++ >= 300) {
            skipResultSequence = 1;
            break;
        }
    }
    if (skipResultSequence == 0) {
        WipeColorSet(0xFF, 0xFF, 0xFF);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_CROSS, 1);
        while (TRUE) {
            if (WipeStatGet() == 0) {
                break;
            }
            Story3SleepOneFrame();
        }
        Story3CleanupWindow(resultState.messageWindowId);
        Story3PositionPlayerSprites();
        Story3ShowRecordSprites();
        Story3HidePlayerModels();
        Story3UpdateRecordPageRows();
        Story3HideResultPlayerModels();
        Story3ShowBoardFlag();
        WipeColorSet(0xFF, 0xFF, 0xFF);
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_CROSS, -1);
        while (TRUE) {
            if (WipeStatGet() == 0) {
                break;
            }
            Story3SleepOneFrame();
        }
        resultState.selectionMessageId = story3CreateDataWindow(-1, 1);
    }
}

Void Story3UpdateRecordPageMessage(Void) {
    resultState.selectionRepeatTimer = 1;
    Story3SetWindowMessage(resultState.selectionMessageId, MAKE_MESSID(38, resultState.selectedRecordPage));
    while (TRUE) {
        Story3SleepOneFrame();
        if (resultState.selectionRepeatTimer++ > 5 && (HuPadBtnDown[resultState.playerResults[resultState.currentPlayerIndex].controllerPort] & PAD_BUTTON_B)) {
            HuAudFXPlay(3);
            break;
        }
        if (HuPadStkX[resultState.playerResults[resultState.currentPlayerIndex].controllerPort] >= 5 || (HuPadBtn[resultState.playerResults[resultState.currentPlayerIndex].controllerPort] & PAD_BUTTON_RIGHT)) {
            if (resultState.selectedRecordPage != 11) {
                resultState.selectionRepeatTimer = 0;
            }
            resultState.recordScrollOffset++;
            if (resultState.recordScrollOffset >= 6) {
                resultState.recordScrollOffset = 5;
            }
            resultState.selectedRecordPage++;
            if (resultState.selectedRecordPage >= 12) {
                resultState.selectedRecordPage = 11;
            } else {
                Story3StartCharacterModelMotion();
                Story3SetWindowMessage(resultState.selectionMessageId, MAKE_MESSID(38, resultState.selectedRecordPage));
                HuAudFXPlay(0);
                Story3SleepFrames(10);
            }
        }
        if (HuPadStkX[resultState.playerResults[resultState.currentPlayerIndex].controllerPort] <= -5 || (HuPadBtn[resultState.playerResults[resultState.currentPlayerIndex].controllerPort] & PAD_BUTTON_LEFT)) {
            if (resultState.selectedRecordPage != 0) {
                resultState.selectionRepeatTimer = 0;
            }
            resultState.recordScrollOffset--;
            if (resultState.recordScrollOffset < 0) {
                resultState.recordScrollOffset = 0;
            }
            resultState.selectedRecordPage--;
            if (resultState.selectedRecordPage < 0) {
                resultState.selectedRecordPage = 0;
            } else {
                Story3StartCharacterModelMotion();
                Story3SetWindowMessage(resultState.selectionMessageId, MAKE_MESSID(38, resultState.selectedRecordPage));
                HuAudFXPlay(0);
                Story3SleepFrames(10);
            }
        }
    }
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_CROSS, 1);
    while (TRUE) {
        if (WipeStatGet() == 0) {
            break;
        }
        Story3SleepOneFrame();
    }
    Story3CleanupWindow(resultState.selectionMessageId);
    Story3HideRecordSprites();
    Story3PositionPlayerModels();
    Story3HideRecordRowSprites();
    Story3ShowPlayerModels();
    Story3HideBoardFlag();
    Story3ShowResultPlayerModels();
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_CROSS, -1);
    while (TRUE) {
        if (WipeStatGet() == 0) {
            break;
        }
        Story3SleepOneFrame();
    }
    resultState.messageWindowId = story3CreateDataWindow(MAKE_MESSID(38, 12), 0);
}

Void Story3ArrangeResultPlayers(Void) {
    s32 teamPlayerLayoutIndex;
    s32 i;

    for (i = 0; i < 5; i++) {
        if (i == 0) {
            resultState.playerModelPositions[i].x = 1800.0f;
            resultState.playerModelPositions[i].y = 157.5f;
        } else if (resultState.teamMode == 1) {
            if (i <= 2) {
                teamPlayerLayoutIndex = 0;
            } else {
                teamPlayerLayoutIndex = 2;
            }
            resultState.playerModelPositions[i].x = 900.0f - 100.0f * teamPlayerLayoutIndex;
            resultState.playerModelPositions[i].y = 75.0f - 82.5f * (i - 1);
        } else {
            resultState.playerModelPositions[i].x = 900.0f - 100.0f * i;
            resultState.playerModelPositions[i].y = 75.0f - 82.5f * (i - 1);
        }
    }
    resultState.recordScrollOffset = 0;
    resultState.selectedRecordPage = 0;
    Story3HideRecordSprites();
    Story3PositionPlayerModels();
    Story3HideRecordRowSprites();
    Story3ShowPlayerModels();
    Story3HideBoardFlag();
    Story3ShowResultPlayerModels();
}

Void Story3CleanupBeforeSave(Void) {
    s32 saveNoticeSpriteId;

    Hu3DAllKill();
    HuSprClose();
    HuSprInit();
    if (SLSaveFlagGet() == 0) {
        saveExecF = 0;
        resultSelectionState = 1;
        HuPrcEnd();
        while (TRUE) {
            HuPrcVSleep();
        }
    }
    espInit();
    HuPrcVSleep();
    saveNoticeSpriteId = espEntry(DATA_MAKE_NUM(DATADIR_WIN, 32), 5000, 0);
    espPosSet(saveNoticeSpriteId, 288.0f, 240.0f);
    espAttrReset(saveNoticeSpriteId, 4);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 20);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    Story3CollectPlayerResults();
    if (saveExecF == 1) {
        GWGameStat.party_continue = 0;
    }
    SLCommonSet();
    if (SLSaveFlagGet() == 1) {
        SLSave();
    }
    saveExecF = 0;
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 20);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    resultSelectionState = 1;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

Void Story3ShowSavePrompt(Void) {
    float savePromptDimensions[2];
    s32 saveChoice = 1;
    s32 saveNoticeSpriteId;
    s32 savePromptWindowId;

    Hu3DAllKill();
    HuSprClose();
    HuSprInit();
    if (SLSaveFlagGet() == 0) {
        resultSelectionState = 1;
        HuPrcEnd();
        while (TRUE) {
            HuPrcVSleep();
        }
    }
    espInit();
    HuPrcVSleep();
    espPosSet(saveNoticeSpriteId = espEntry(DATA_MAKE_NUM(DATADIR_WIN, 32), 5000, 0), 288.0f, 240.0f);
    espAttrReset(saveNoticeSpriteId, HUSPR_ATTR_DISPOFF);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 20);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    if (SLSaveFlagGet() == 1) {
        HuWinInit(1);
        HuWinMesMaxSizeGet(1, savePromptDimensions, MAKE_MESSID(30, 22));
        savePromptWindowId = HuWinExCreateStyled(-10000.0f, 150.0f, savePromptDimensions[0], savePromptDimensions[1], -1, 2);
        winData[savePromptWindowId].active_pad = 1;
        HuWinAttrSet(savePromptWindowId, 0x10);
        HuWinExAnimIn(savePromptWindowId);
        HuWinMesSet(savePromptWindowId, MAKE_MESSID(30, 22));
        saveChoice = HuWinChoiceGet(savePromptWindowId, 0);
        HuWinExAnimOut(savePromptWindowId);
        HuWinExCleanup(savePromptWindowId);
        if (SLSaveFlagGet() == 1 && saveChoice == 0) {
#if VERSION_REV1 || VERSION_REV2
            GWGameStat.story_continue = 1;
#endif
            SLCommonSet();
            SLSaveBoardStory();
            SLSave();
        }
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 20);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    resultSelectionState = 1;
    HuPrcEnd();
    while (TRUE) {
        HuPrcVSleep();
    }
}

s32 Story3IsBoardClearUnlocked(s32 boardId) {
    if (boardId == 0 && _CheckFlag(FLAG_ID_MAKE(0, 2))) {
         return 1;
    }
    if (boardId == 1 && _CheckFlag(FLAG_ID_MAKE(0, 3))) {
        return 1;
    }
    if (boardId == 2 && _CheckFlag(FLAG_ID_MAKE(0, 4))) {
        return 1;
    }
    if (boardId == 3 && _CheckFlag(FLAG_ID_MAKE(0, 5))) {
        return 1;
    }
    if (boardId == 4 && _CheckFlag(FLAG_ID_MAKE(0, 6))) {
        return 1;
    }
    if (boardId == 5 && _CheckFlag(FLAG_ID_MAKE(0, 7))) {
        return 1;
    }
    return 0;
}

Void Story3UnlockBoardClearEvents(Void) {
    s32 boardClearEventUnlocked;
    s32 i;

    boardClearEventUnlocked = 0;
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, -1);
    do {
        Story3SleepOneFrame();
    } while (WipeStatGet() != 0);
    CharModelKill(-1);
    HuAudSeqAllFadeOut(1000);
    HuAudSStreamAllFadeOut(1000);
    savePromptProcessState.callback = NULL;
    recordPageProcessState.callback = NULL;
    playerModelProcessState.callback = NULL;
    for (i = 0; i < 4; i++) {
        playerRecordProcessStates[i].callback = NULL;
    }
    if (GWPartyGet() == 1) {
        HuPrcChildCreate(Story3CleanupBeforeSave, 100, 0x2000, 0, HuPrcCurrentGet());
        do {
            Story3SleepOneFrame();
        } while (resultSelectionState != 1);
        Story3SleepFrames(30);
        omOvlReturnEx(1, 1);
    } else {
        Story3CollectPlayerResults();
        for (i = 0; i < 4; i++) {
            if (resultState.playerResults[i].characterId != GWStoryCharGet()) {
                continue;
            }
            _SetFlag(9);
            switch (resultState.boardId) {
                case 0:
                    if (_CheckFlag(2) == 0) {
                        _SetFlag(2);
                        boardClearEventUnlocked = 1;
                    }
                    break;
                case 1:
                    if (_CheckFlag(3) == 0) {
                        _SetFlag(3);
                        boardClearEventUnlocked = 1;
                    }
                    break;
                case 2:
                    if (_CheckFlag(4) == 0) {
                        _SetFlag(4);
                        boardClearEventUnlocked = 1;
                    }
                    break;
                case 3:
                    if (_CheckFlag(5) == 0) {
                        _SetFlag(5);
                        boardClearEventUnlocked = 1;
                    }
                    break;
                case 4:
                    if (_CheckFlag(6) == 0) {
                        _SetFlag(6);
                        boardClearEventUnlocked = 1;
                    }
                    break;
                case 5:
                    if (_CheckFlag(7) == 0) {
                        _SetFlag(7);
                        boardClearEventUnlocked = 1;
                    }
                    break;
            }
            if (boardClearEventUnlocked == 1) {
                if (resultState.playerResults[i].rank == 0) {
                    if (resultState.boardId == 5) {
                        GWGameStat.open_w06 = 1;
                    }
                    _ClearFlag(9);
                    OSReport("########## Next to BoardClear Event 10 -> MapNo:%d, MapFlg:%d, MgFlg:%d\n", GWSystem.board, _CheckFlag(2), _CheckFlag(9));
                    Story3SleepFrames(120);
                    HuPrcChildCreate(Story3ShowSavePrompt, 100, 0x2000, 0, HuPrcCurrentGet());
                    do {
                        Story3SleepOneFrame();
                    } while (resultSelectionState != 1);
                    if (resultState.boardId != 5) {
                        omOvlGotoEx(DLL_mstorydll, 1, 0, 0);
                    } else {
                        omOvlGotoEx(DLL_mstory2dll, 1, 1, 0);
                    }
                } else {
                    switch (resultState.boardId) {
                        case 0:
                            _ClearFlag(2);
                            break;
                        case 1:
                            _ClearFlag(3);
                            break;
                        case 2:
                            _ClearFlag(4);
                            break;
                        case 3:
                            _ClearFlag(5);
                            break;
                        case 4:
                            _ClearFlag(6);
                            break;
                        case 5:
                            _ClearFlag(7);
                            break;
                    }
                    OSReport("########## Next to BoardMiss Event 01 -> MapNo:%d, MapFlg:%d, MgFlg:%d\n", GWSystem.board, Story3IsBoardClearUnlocked(GWSystem.board), _CheckFlag(9));
                    Story3SleepFrames(120);
                    HuPrcChildCreate(Story3ShowSavePrompt, 100, 0x2000, 0, HuPrcCurrentGet());
                    do {
                        Story3SleepOneFrame();
                    } while (resultSelectionState != 1);
                    if (resultState.boardId != 5) {
                        omOvlGotoEx(DLL_mstorydll, 1, 1, 0);
                    } else {
                        omOvlGotoEx(DLL_mstory2dll, 1, 2, 0);
                    }
                }
            } else {
                GWPlayerCfg->group = 0;
                GWPlayerCfg->character = resultState.playerResults[resultState.currentPlayerIndex].characterId;
                GWPlayerCfg->pad_idx = resultState.playerResults[resultState.currentPlayerIndex].controllerPort;
                GWPlayerCfg->iscom = 0;
                for (i = 0; i < 4; i++) {
                    GWPlayer[i].character = GWPlayerCfg[i].character;
                    GWPlayer[i].port = GWPlayerCfg[i].pad_idx;
                    GWPlayer[i].com = GWPlayerCfg[i].iscom;
                    GWPlayer[i].team = GWPlayerCfg[i].group;
                }
                OSReport("########## Next to MapSelect Event 11 -> MapNo:%d, MapFlg:%d, MgFlg:%d\n", GWSystem.board, Story3IsBoardClearUnlocked(GWSystem.board), _CheckFlag(9));
                Story3SleepFrames(120);
                HuPrcChildCreate(Story3ShowSavePrompt, 100, 0x2000, 0, HuPrcCurrentGet());
                do {
                    Story3SleepOneFrame();
                } while (resultSelectionState != 1);
                omOvlGotoEx(DLL_mentdll, 1, 10, 0);
            }
        }
    }
    while (TRUE) {
        Story3SleepOneFrame();
    }
}

Void Story3RunResultScreen(OMOBJ* object, StorySequenceProcessState* processState) {
    Story3BeginResultSequence();
    Story3WaitForPlayerArrivals();
    while (TRUE) {
        Story3SleepOneFrame();
        if (Story3WaitForResultInput() != 0) {
            break;
        }
        Story3UpdateRecordPageMessage();
    }
    Story3UnlockBoardClearEvents();
    processState->callback = 0;
}

Void Story3CreateResultTitleObject(Void) {
    OMOBJ* createdResultObject;

    createdResultObject = omAddObjEx(resultProcess, 0x100, 0x10, 0x10, -1, NULL);
    savePromptProcessState.object = createdResultObject;
    savePromptProcessState.callback = (Void*) Story3RunResultScreen;
    while (TRUE) {
        Story3SleepOneFrame();
        if (savePromptProcessState.callback != NULL) {
            savePromptProcessState.callback(createdResultObject, &savePromptProcessState);
        }
    }
}

Void Story3UpdateResultTitleSprite(OMOBJ* object, s32 count) {
    s32 titleSpriteId;
    s32 i;

    object->work[0] = HuSprGrpCreate(1);
    titleSpriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 75) + count, HU_MEMNUM_OVL)), 1000, 0);
    HuSprGrpMemberSet(object->work[0], 0, titleSpriteId);
    // TODO: figure out the actual args of this story3_position_centered_sprite call.
    // It will match as long as the values (leftX + width / 2) and (topY + height / 2) are preserved.
    Story3PositionCenteredSprite(object->work[0], 0, 288, 240, 0, 0);
    HuSprGrpPosSet(object->work[0], 0.0f, 0.0f);
    HuSprGrpDrawNoSet(object->work[0], 0x7F);
    {
        s32 resultModelDataIds[] = {
            DATA_MAKE_NUM(DATADIR_MSTORY3, 104),
            DATA_MAKE_NUM(DATADIR_MSTORY3, 105),
            DATA_MAKE_NUM(DATADIR_MSTORY3, 106),
            DATA_MAKE_NUM(DATADIR_MSTORY3, 107),
            DATA_MAKE_NUM(DATADIR_MSTORY3, 108),
            DATA_MAKE_NUM(DATADIR_MSTORY3, 109),
            DATA_MAKE_NUM(DATADIR_MSTORY3, 106)
        };

        for (i = 0; i < 7; i++) {
            object->mdlId[i + 1] = Hu3DModelCreateFile(resultModelDataIds[i]);
            Hu3DModelPosSet(object->mdlId[i + 1], -122.5 + 42.5 * i, 185.0f, -10.0f);
            Hu3DModelRotSet(object->mdlId[i + 1], 0.0f, 1980.0f, 0.0f);
        }
    }
}

Void Story3AnimatePlayerRotation(OMOBJ* object, s32* count) {
    s32 playerRotationStage = 0;
    s32 i;

    for (i = 0; i < 7; i++) {
        if (count[2] == 0 && Hu3DData[object->mdlId[i + 1]].pos.x >= resultState.playerModelPositions[0].x) {
            Hu3DData[object->mdlId[i + 1]].rot.y = interpolateSmoothAverage(Hu3DData[object->mdlId[i + 1]].rot.y, 0.0f, 20.0f);
        } else if (count[2] == 1 && Hu3DData[object->mdlId[i + 1]].pos.x <= resultState.playerModelPositions[0].x) {
            Hu3DData[object->mdlId[i + 1]].rot.y = interpolateSmoothAverage(Hu3DData[object->mdlId[i + 1]].rot.y, 1980.0f, 20.0f);
        }
    }
}

Void Story3ShowPlayerModels(Void) {
    s32 i;

    for (i = 0; i < 7; i++) {
        Hu3DModelAttrReset(recordPageProcessState.object->mdlId[i + 1], HU3D_ATTR_DISPOFF);
    }
    recordPageProcessState.callback = (Void*) Story3AnimatePlayerRotation;
}

Void Story3HidePlayerModels(Void) {
    s32 i;

    for (i = 0; i < 7; i++) {
        Hu3DModelAttrSet(recordPageProcessState.object->mdlId[i + 1], HU3D_ATTR_DISPOFF);
    }
    for (i = 0; i < 7; i++) {
        if (recordPageProcessState.workValue0 != 0) {
            Hu3DData[recordPageProcessState.object->mdlId[i + 1]].rot.y = 1980.0f;
        } else {
            Hu3DData[recordPageProcessState.object->mdlId[i + 1]].rot.y = 0.0f;
        }
    }
    recordPageProcessState.callback = NULL;
}

Void Story3CreateResultRecordSprites(OMOBJ* object, s32 count) {
    s32 spriteId;
    s32 i;

    // TODO: figure out the actual args of story3_position_centered_sprite calls.
    // It will match as long as the values (leftX + width / 2) and (topY + height / 2) are preserved.
    object->work[1] = HuSprGrpCreate(8);
    spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 116), HU_MEMNUM_OVL)), 900, 0);
    HuSprGrpMemberSet(object->work[1], 0, spriteId);
    Story3PositionCenteredSprite(object->work[1], 0, 368, 171, 0, 0);
    spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 117), HU_MEMNUM_OVL)), 600, 0);
    HuSprGrpMemberSet(object->work[1], 1, spriteId);
    Story3PositionCenteredSprite(object->work[1], 1, 292, 168, 0, 0);
    spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 118), HU_MEMNUM_OVL)), 500, 0);
    HuSprGrpMemberSet(object->work[1], 2, spriteId);
    Story3PositionCenteredSprite(object->work[1], 2, 292, 168, 0, 0);
    spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 119), HU_MEMNUM_OVL)), 500, 0);
    HuSprGrpMemberSet(object->work[1], 3, spriteId);
    Story3PositionCenteredSprite(object->work[1], 3, 292, 168, 0, 0);
    spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(count + DATA_MAKE_NUM(DATADIR_MSTORY3, 110), HU_MEMNUM_OVL)), 500, 0);
    HuSprGrpMemberSet(object->work[1], 4, spriteId);
    Story3PositionCenteredSprite(object->work[1], 4, 96, 53, 0, 0);
    spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 138), HU_MEMNUM_OVL)), 500, 0);
    HuSprGrpMemberSet(object->work[1], 5, spriteId);
    Story3PositionCenteredSprite(object->work[1], 5, 407, 37, 0, 0);
    for (i = 0; i < 2; i++) {
        spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 139), HU_MEMNUM_OVL)), 500, 0);
        HuSprGrpMemberSet(object->work[1], i + 6, spriteId);
    }
    // TODO: figure out the exact minimum digit count. The code uses this value as a minimum when deciding whether to show leading zeroes.
    Story3DrawNumberSprites(object->work[1], 6, resultState.turnCount, 293, 19, 36, 36, 2);
    object->work[2] = HuSprGrpCreate(2);
    spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 120), HU_MEMNUM_OVL)), 800, 0);
    HuSprGrpMemberSet(object->work[2], 0, spriteId);
    Story3PositionCenteredSprite(object->work[2], 0, 34, 119, 0, 0);
    spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 121), HU_MEMNUM_OVL)), 400, 0);
    HuSprGrpMemberSet(object->work[2], 1, spriteId);
    Story3PositionCenteredSprite(object->work[2], 1, 36, 121, 0, 0);
    object->work[3] = HuSprGrpCreate(12);
    for (i = 0; i < 12; i++) {
        spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(i + DATA_MAKE_NUM(DATADIR_MSTORY3, 124), HU_MEMNUM_OVL)), 700, 0);
        HuSprGrpMemberSet(object->work[3], i, spriteId);
        Story3PositionCenteredSprite(object->work[3], i, i * 66, 16, 58, 0);
    }
    HuSprGrpPosSet(object->work[2], 164 + resultState.recordScrollOffset * 66, 78.0f);
    HuSprGrpDrawNoSet(object->work[1], 0x7F);
    HuSprGrpDrawNoSet(object->work[2], 0x7F);
    HuSprGrpDrawNoSet(object->work[3], 0x7F);
}

Void Story3UpdateRecordPageSelection(OMOBJ* object) {
    float selectionBarX;
    float targetSelectionBarX;
    s32 visibleSelectionIndex;
    s32 visiblePageStart;
    s32 recordRowIndex;

    visiblePageStart = resultState.selectedRecordPage - resultState.recordScrollOffset;
    if (visiblePageStart < 0) {
        visiblePageStart = 0;
    }
    selectionBarX = HuSprGrpData[object->work[2]].pos.x;
    targetSelectionBarX = 164 + resultState.recordScrollOffset * 66;
    selectionBarX = interpolateSmoothAverage(selectionBarX, targetSelectionBarX, 2.0f);
    HuSprGrpPosSet(object->work[1], -5.0f, 25.0f);
    HuSprGrpPosSet(object->work[2], selectionBarX, 78.0f);
    HuSprGrpPosSet(object->work[3], 169 - visiblePageStart * 66, 82.0f);
    for (recordRowIndex = 0; recordRowIndex < 12; recordRowIndex++) {
        if (recordRowIndex < visiblePageStart || recordRowIndex > visiblePageStart + 5) {
            HuSprAttrSet(object->work[3], recordRowIndex, HUSPR_ATTR_DISPOFF);
        } else {
            HuSprAttrReset(object->work[3], recordRowIndex, HUSPR_ATTR_DISPOFF);
        }
    }
    visibleSelectionIndex = resultState.selectedRecordPage - resultState.recordScrollOffset;
    if (visibleSelectionIndex < 0) {
        visibleSelectionIndex = 0;
    }
    if (visibleSelectionIndex == 0) {
        HuSprBankSet(recordPageProcessState.object->work[1], 2, 2);
    } else if (visibleSelectionIndex == 1) {
        HuSprBankSet(recordPageProcessState.object->work[1], 2, 0);
    }
    if (visibleSelectionIndex == 6) {
        HuSprBankSet(recordPageProcessState.object->work[1], 3, 2);
    } else if (visibleSelectionIndex == 5) {
        HuSprBankSet(recordPageProcessState.object->work[1], 3, 0);
    }
}

Void Story3UpdateRecordPageRows(Void) {
    s32 visiblePageStart;
    s32 i;

    for (i = 0; i < 8; i++) {
        HuSprAttrReset(recordPageProcessState.object->work[1], i, HUSPR_ATTR_DISPOFF);
    }
    for (i = 0; i < 2; i++) {
        HuSprAttrReset(recordPageProcessState.object->work[2], i, HUSPR_ATTR_DISPOFF);
    }
    for (i = 0; i < 12; i++) {
        HuSprAttrReset(recordPageProcessState.object->work[3], i, HUSPR_ATTR_DISPOFF);
    }
    visiblePageStart = resultState.selectedRecordPage - resultState.recordScrollOffset;
    if (visiblePageStart < 0) {
        visiblePageStart = 0;
    }
    if (visiblePageStart == 0) {
        HuSprBankSet(recordPageProcessState.object->work[1], 2, 2);
    } else if (visiblePageStart == 1) {
        HuSprBankSet(recordPageProcessState.object->work[1], 2, 0);
    }
    if (visiblePageStart == 6) {
        HuSprBankSet(recordPageProcessState.object->work[1], 3, 2);
    } else if (visiblePageStart == 5) {
        HuSprBankSet(recordPageProcessState.object->work[1], 3, 0);
    }
    recordPageProcessState.callback = (Void*) Story3UpdateRecordPageSelection;
}

Void Story3HideRecordRowSprites(Void) {
    s32 i;

    for (i = 0; i < 8; i++) {
        HuSprAttrSet(recordPageProcessState.object->work[1], i, HUSPR_ATTR_DISPOFF);
    }
    for (i = 0; i < 2; i++) {
        HuSprAttrSet(recordPageProcessState.object->work[2], i, HUSPR_ATTR_DISPOFF);
    }
    for (i = 0; i < 12; i++) {
        HuSprAttrSet(recordPageProcessState.object->work[3], i, HUSPR_ATTR_DISPOFF);
    }
    recordPageProcessState.callback = NULL;
}

Void Story3CreateRecordPageObject(Void) {
    OMOBJ* createdResultObject;

    createdResultObject = omAddObjEx(resultProcess, 0x100, 0x10, 0x10, -1, NULL);
    recordPageProcessState.object = createdResultObject;
    recordPageProcessState.callback = NULL;
    Story3UpdateResultTitleSprite(createdResultObject, resultState.boardId);
    Story3CreateResultRecordSprites(createdResultObject, resultState.boardId);
    while (TRUE) {
        Story3SleepOneFrame();
        if (recordPageProcessState.callback != NULL) {
            recordPageProcessState.callback(createdResultObject, &recordPageProcessState);
        }
    }
}

Void Story3FindHighestMinigameScore(Void) {
    s32 highestMinigameScore;
    s32 i;
    s32 j;

    for (i = 0; i < 12; i++) {
        highestMinigameScore = 0;
        for (j = 0; j < 4; j++) {
            if (highestMinigameScore <= resultState.playerResults[j].minigameResults[i][0]) {
                highestMinigameScore = resultState.playerResults[j].minigameResults[i][0];
            }
        }
        if (highestMinigameScore > 0) {
            for (j = 0; j < 4; j++) {
                if (highestMinigameScore == resultState.playerResults[j].minigameResults[i][0]) {
                    resultState.playerResults[j].minigameResults[i][1] = 1;
                }
            }
        }
    }
}

Void Story3CopyResultPlayerStats(StoryResultPlayerStats* playerResult, s32 count, s32 mode) {
    playerResult->playerIndex = count;
    playerResult->team = GWPlayerCfg[playerResult->playerIndex].group;
    playerResult->isComputer = GWPlayerCfg[playerResult->playerIndex].iscom;
    playerResult->difficulty = GWPlayerCfg[playerResult->playerIndex].diff;
    playerResult->characterId = GWPlayerCfg[playerResult->playerIndex].character;
    playerResult->controllerPort = GWPlayerCfg[playerResult->playerIndex].pad_idx;
    playerResult->rank = GWPlayer[playerResult->playerIndex].rank;
    playerResult->minigameResults[0][0] = GWPlayer[playerResult->playerIndex].stars;
    playerResult->minigameResults[1][0] = GWPlayer[playerResult->playerIndex].coins;
    playerResult->minigameResults[2][0] = GWPlayer[playerResult->playerIndex].coins_mg;
    playerResult->minigameResults[3][0] = GWPlayer[playerResult->playerIndex].coins_max;
    playerResult->blueSpaceCount = GWPlayer[playerResult->playerIndex].blue_count;
    playerResult->redSpaceCount = GWPlayer[playerResult->playerIndex].red_count;
    playerResult->questionSpaceCount = GWPlayer[playerResult->playerIndex].question_count;
    playerResult->fortuneSpaceCount = GWPlayer[playerResult->playerIndex].fortune_count;
    playerResult->bowserSpaceCount = GWPlayer[playerResult->playerIndex].bowser_count;
    playerResult->battleSpaceCount = GWPlayer[playerResult->playerIndex].battle_count;
    playerResult->mushroomSpaceCount = GWPlayer[playerResult->playerIndex].mushroom_count;
    playerResult->warpSpaceCount = GWPlayer[playerResult->playerIndex].warp_count;
}

Void Story3SortPlayersByRank(s32 playerCount) {
    s32 playersSortedByRank[4][2];
    s32 rankedPlayersBeforeTeamSort[4][2];
    s32 playerIndicesByRank[4];
    s32 playerRanks[4];
    s32 playerTeams[4];
    s32 sortedPlayerCount;
    s32 teamSortOrder;
    s32 j;
    s32 i;

    sortedPlayerCount = 0;
    playerIndicesByRank[0] = 0;
    for (i = 1; i < 4; i++) {
        playerIndicesByRank[i] = playerIndicesByRank[i - 1] + 1;
        if (playerIndicesByRank[i] >= 4) {
            playerIndicesByRank[i] = 0;
        }
    }
    for (i = 0; i < 4; i++) {
        playerRanks[i] = GWPlayer[playerIndicesByRank[i]].rank;
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (i == playerRanks[j]) {
                playersSortedByRank[sortedPlayerCount][0] = playerIndicesByRank[j];
                playersSortedByRank[sortedPlayerCount][1] = playerRanks[j];
                sortedPlayerCount++;
            }
        }
    }
    if (resultState.teamMode == 1) {
        teamSortOrder = resultState.winningTeam;
        for (i = 0; i < 4; i++) {
            playerTeams[0] = GWPlayer[playersSortedByRank[0][0]].team;
            playerTeams[1] = GWPlayer[playersSortedByRank[1][0]].team;
            playerTeams[2] = GWPlayer[playersSortedByRank[2][0]].team;
            playerTeams[3] = GWPlayer[playersSortedByRank[3][0]].team;
            rankedPlayersBeforeTeamSort[i][0] = playersSortedByRank[i][0];
            rankedPlayersBeforeTeamSort[i][1] = playersSortedByRank[i][1];
        }
        OSReport("########## Sort Start WIN:%d  IDX (%d,%d,%d), (%d,%d,%d), (%d,%d,%d), (%d,%d,%d)\n", teamSortOrder,
            playerTeams[0], playersSortedByRank[0][0], playersSortedByRank[0][1], playerTeams[1], playersSortedByRank[1][0], playersSortedByRank[1][1],
            playerTeams[2], playersSortedByRank[2][0], playersSortedByRank[2][1], playerTeams[3], playersSortedByRank[3][0], playersSortedByRank[3][1]);
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                if (playerTeams[j] != -1 && teamSortOrder == playerTeams[j]) {
                    playerTeams[j] = -1;
                    playersSortedByRank[i][0] = rankedPlayersBeforeTeamSort[j][0];
                    playersSortedByRank[i][1] = rankedPlayersBeforeTeamSort[j][1];
                    break;
                }
            }
            if (i == 1) {
                teamSortOrder++;
                teamSortOrder %= 2;
            }
        }
        OSReport("########## Sort End WIN:%d  IDX %d, %d, %d, %d\n", teamSortOrder, playersSortedByRank[0][0], playersSortedByRank[1][0], playersSortedByRank[2][0], playersSortedByRank[3][0]);
        OSReport("########## Sort End RANK %d, %d, %d, %d,\n", playersSortedByRank[0][1], playersSortedByRank[1][1], playersSortedByRank[2][1], playersSortedByRank[3][1]);
    }
    for (i = 0; i < 4; i++) {
        Story3CopyResultPlayerStats(&resultState.playerResults[i], playersSortedByRank[i][0], playersSortedByRank[i][1]);
        if (playerCount == playersSortedByRank[i][0]) {
            resultState.currentPlayerIndex = i;
        }
    }
}

Void Story3CalculateTeamMinigameScores(OMOBJ* object, s32 count) {
    s32 teamOneMinigameScore;
    s32 teamTwoMinigameScore;
    s32 modelVariant;
    s32 teamHeaderY;
    s32 teamScoreY;
    s32 teamSecondScoreY;
    StoryResultPlayerStats* playerResult;
    s32 spriteId;
    s32 teamMinigameScore;
    s32 scoreDigitIndex;

    // TODO: figure out the actual args of story3_position_centered_sprite calls.
    // It will match as long as the values (leftX + width / 2) and (topY + height / 2) are preserved.
    playerResult = &resultState.playerResults[count];
    if (resultState.teamMode == 1) {
        modelVariant = resultState.winningTeam;
        if (count > 1) {
            modelVariant++;
            modelVariant %= 2;
        }
        object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 140) + modelVariant);
        object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 142) + modelVariant);
        object->mtnId[2] = Hu3DMotionIDGet(object->mdlId[2]);
        if (count <= 1) {
            modelVariant = 0;
        } else {
            modelVariant = 1;
        }
        object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 100) + modelVariant);
        Hu3DModelHookSet(object->mdlId[1], "results_flag04-hook_cubes", object->mdlId[3]);
    } else {
        object->mdlId[1] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 98));
        object->mdlId[2] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 99));
        object->mtnId[2] = Hu3DMotionIDGet(object->mdlId[2]);
        object->mdlId[3] = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_MSTORY3, 100) + playerResult->rank);
        Hu3DModelHookSet(object->mdlId[1], "results_flag04-hook_cubes", object->mdlId[3]);
    }
    object->work[0] = HuSprGrpCreate(12);
    if (resultState.teamMode == 1) {
        teamHeaderY = 35;
        teamScoreY = 10;
        teamSecondScoreY = 65;
        if (count <= 1) {
            spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 81), HU_MEMNUM_OVL)), 900, 0);
        } else {
            spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 82), HU_MEMNUM_OVL)), 900, 0);
        }
        HuSprGrpMemberSet(object->work[0], 0, spriteId);
        Story3PositionCenteredSprite(object->work[0], 0, 40, teamHeaderY + 36, 0, 0);
        spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 85) + playerResult->characterId, HU_MEMNUM_OVL)), 900, 0);
        HuSprGrpMemberSet(object->work[0], 1, spriteId);
        if (count % 2 == 0) {
            Story3PositionCenteredSprite(object->work[0], 1, 119, teamHeaderY + 34, 0, 0);
        } else {
            Story3PositionCenteredSprite(object->work[0], 1, 189, -teamHeaderY + 24, 0, 0);
        }
        spriteId = HuSprCreate(resultDecorationAnimations[0], 900, 0);
        HuSprGrpMemberSet(object->work[0], 2, spriteId);
        Story3PositionCenteredSprite(object->work[0], 2, 262, teamScoreY + 29, 0, 0);
        spriteId = HuSprCreate(resultDecorationAnimations[2], 900, 0);
        HuSprGrpMemberSet(object->work[0], 3, spriteId);
        Story3PositionCenteredSprite(object->work[0], 3, 312, teamScoreY + 29, 0, 0);
        for (scoreDigitIndex = 0; scoreDigitIndex < 3; scoreDigitIndex++) {
            teamOneMinigameScore = resultState.playerResults[0].minigameResults[0][0] + resultState.playerResults[1].minigameResults[0][0];
            teamTwoMinigameScore = resultState.playerResults[2].minigameResults[0][0] + resultState.playerResults[3].minigameResults[0][0];
            if (count <= 1) {
                if (teamOneMinigameScore >= teamTwoMinigameScore) {
                    spriteId = HuSprCreate(recordStarAnimations[1], 900, 0);
                } else {
                    spriteId = HuSprCreate(recordStarAnimations[0], 900, 0);
                }
            } else {
                if (teamOneMinigameScore <= teamTwoMinigameScore) {
                    spriteId = HuSprCreate(recordStarAnimations[1], 900, 0);
                } else {
                    spriteId = HuSprCreate(recordStarAnimations[0], 900, 0);
                }
            }
            HuSprGrpMemberSet(object->work[0], scoreDigitIndex + 4, spriteId);
        }
        if (count <= 1) {
            teamMinigameScore = resultState.playerResults[0].minigameResults[0][0] + resultState.playerResults[1].minigameResults[0][0];
            if (teamMinigameScore >= 999) {
                teamMinigameScore = 999;
            }
            Story3DrawNumberSprites(object->work[0], 4, teamMinigameScore, 340, teamScoreY + 11, 26, 36, 0);
        } else {
            teamMinigameScore = resultState.playerResults[2].minigameResults[0][0] + resultState.playerResults[3].minigameResults[0][0];
            if (teamMinigameScore >= 999) {
                teamMinigameScore = 999;
            }
            Story3DrawNumberSprites(object->work[0], 4, teamMinigameScore, 340, teamScoreY + 11, 26, 36, 0);
        }
        spriteId = HuSprCreate(resultDecorationAnimations[1], 900, 0);
        HuSprGrpMemberSet(object->work[0], 7, spriteId);
        Story3PositionCenteredSprite(object->work[0], 7, 262, teamSecondScoreY + 29, 0, 0);
        spriteId = HuSprCreate(resultDecorationAnimations[2], 900, 0);
        HuSprGrpMemberSet(object->work[0], 8, spriteId);
        Story3PositionCenteredSprite(object->work[0], 8, 312, teamSecondScoreY + 29, 0, 0);
        for (scoreDigitIndex = 0; scoreDigitIndex < 3; scoreDigitIndex++) {
            teamOneMinigameScore = resultState.playerResults[0].minigameResults[1][0] + resultState.playerResults[1].minigameResults[1][0];
            teamTwoMinigameScore = resultState.playerResults[2].minigameResults[1][0] + resultState.playerResults[3].minigameResults[1][0];
            if (count <= 1) {
                if (teamOneMinigameScore >= teamTwoMinigameScore) {
                    spriteId = HuSprCreate(recordStarAnimations[1], 900, 0);
                } else {
                    spriteId = HuSprCreate(recordStarAnimations[0], 900, 0);
                }
            } else {
                if (teamOneMinigameScore <= teamTwoMinigameScore) {
                    spriteId = HuSprCreate(recordStarAnimations[1], 900, 0);
                } else {
                    spriteId = HuSprCreate(recordStarAnimations[0], 900, 0);
                }
            }
            HuSprGrpMemberSet(object->work[0], scoreDigitIndex + 9, spriteId);
        }
        if (count <= 1) {
            teamMinigameScore = resultState.playerResults[0].minigameResults[1][0] + resultState.playerResults[1].minigameResults[1][0];
            if (teamMinigameScore >= 999) {
                teamMinigameScore = 999;
            }
            Story3DrawNumberSprites(object->work[0], 9, teamMinigameScore, 340, teamSecondScoreY + 11, 26, 36, 0);
        } else {
            teamMinigameScore = resultState.playerResults[2].minigameResults[1][0] + resultState.playerResults[3].minigameResults[1][0];
            if (teamMinigameScore >= 999) {
                teamMinigameScore = 999;
            }
            Story3DrawNumberSprites(object->work[0], 9, teamMinigameScore, 340, teamSecondScoreY + 11, 26, 36, 0);
        }
    } else {
        spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 81) + playerResult->rank, HU_MEMNUM_OVL)), 900, 0);
        HuSprGrpMemberSet(object->work[0], 0, spriteId);
        Story3PositionCenteredSprite(object->work[0], 0, 25, 37, 0, 0);
        spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 85) + playerResult->characterId, HU_MEMNUM_OVL)), 900, 0);
        HuSprGrpMemberSet(object->work[0], 1, spriteId);
        Story3PositionCenteredSprite(object->work[0], 1, 87, 29, 0, 0);
        spriteId = HuSprCreate(resultDecorationAnimations[0], 900, 0);
        HuSprGrpMemberSet(object->work[0], 2, spriteId);
        Story3PositionCenteredSprite(object->work[0], 2, 144, 29, 0, 0);
        spriteId = HuSprCreate(resultDecorationAnimations[2], 900, 0);
        HuSprGrpMemberSet(object->work[0], 3, spriteId);
        Story3PositionCenteredSprite(object->work[0], 3, 178, 29, 0, 0);
        for (scoreDigitIndex = 0; scoreDigitIndex < 3; scoreDigitIndex++) {
            spriteId = HuSprCreate(recordStarAnimations[playerResult->minigameResults[0][1]], 900, 0);
            HuSprGrpMemberSet(object->work[0], (scoreDigitIndex + 4), spriteId);
        }
        Story3DrawNumberSprites(object->work[0], 4, playerResult->minigameResults[0][0], 190, 11, 26, 36, 0);
        spriteId = HuSprCreate(resultDecorationAnimations[1], 900, 0);
        HuSprGrpMemberSet(object->work[0], 7, spriteId);
        Story3PositionCenteredSprite(object->work[0], 7, 304, 29, 0, 0);
        spriteId = HuSprCreate(resultDecorationAnimations[2], 900, 0);
        HuSprGrpMemberSet(object->work[0], 8, spriteId);
        Story3PositionCenteredSprite(object->work[0], 8, 338, 29, 0, 0);
        for (scoreDigitIndex = 0; scoreDigitIndex < 3; scoreDigitIndex++) {
            spriteId = HuSprCreate(recordStarAnimations[playerResult->minigameResults[1][1]], 900, 0);
            HuSprGrpMemberSet(object->work[0], scoreDigitIndex + 9, spriteId);
        }
        Story3DrawNumberSprites(object->work[0], 9, playerResult->minigameResults[1][0], 350, 11, 26, 36, 0);
    }
    playerRecordProcessStates[scoreDigitIndex].spriteId = 1000 + rand8() * 5;
}

Void Story3RotateResultBoardFlag(OMOBJ* object, StorySequenceProcessState* processState) {
    Mtx flagModelMatrix;
    Vec flagScreenPosition;
    Vec flagScreenCoordinates;
    Vec flagWorldPosition;
    s32 spriteIndex;

    if (resultState.teamMode == 1) {
        if (processState->workValue0 % 2 == 1) {
            Hu3DModelAttrSet(object->mdlId[1], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(object->mdlId[2], HU3D_ATTR_DISPOFF);
            Hu3DModelAttrSet(object->mdlId[3], HU3D_ATTR_DISPOFF);
            for (spriteIndex = 0; spriteIndex < 12; spriteIndex++) {
                HuSprAttrSet(object->work[0], spriteIndex, 4);
            }
        }
        HuSprAttrReset(object->work[0], 1, HUSPR_ATTR_DISPOFF);
        Hu3DModelPosSet(object->mdlId[1], resultState.playerModelPositions[processState->workValue0 + 1].x + 15.0f, resultState.playerModelPositions[processState->workValue0 + 1].y + 25.0f - 35.0f, resultState.playerModelPositions[processState->workValue0 + 1].z);
    } else {
        Hu3DModelPosSet(object->mdlId[1], resultState.playerModelPositions[processState->workValue0 + 1].x + 15.0f, resultState.playerModelPositions[processState->workValue0 + 1].y + 25.0f, resultState.playerModelPositions[processState->workValue0 + 1].z);
    }
    if (Hu3DData[object->mdlId[1]].pos.x <= -195.0f) {
        Hu3DData[object->mdlId[1]].pos.x = -195.0f;
    }
    Hu3DModelObjMtxGet(object->mdlId[1], "results_flag04-hook_flag", flagModelMatrix);
    Hu3DModelPosSet(object->mdlId[2], flagModelMatrix[0][3], flagModelMatrix[1][3], flagModelMatrix[2][3]);
    if (processState->processState++ >= processState->spriteId) {
        processState->processState = 0;
        processState->spriteId = rand8() * 5 + 1000;
        Story3SetModelMotion(object, 2, 2, 0, 0);
    }
    Hu3DModelRotSet(object->mdlId[3], 0.0f, processState->workValue2 += 3, 0.0f);
    if (processState->workValue2 >= 360) {
        processState->workValue2 -= 360;
    }
    flagWorldPosition.x = Hu3DData[object->mdlId[1]].pos.x;
    flagWorldPosition.y = resultState.playerModelPositions[processState->workValue0 + 1].y + 25.0f;
    flagWorldPosition.z = Hu3DData[object->mdlId[1]].pos.z;
    flagScreenPosition.x = flagWorldPosition.x + 24.0f;
    flagScreenPosition.y = flagWorldPosition.y + 40.0f;
    flagScreenPosition.z = flagWorldPosition.z;
    Hu3D3Dto2D(&flagScreenPosition, 1, &flagScreenCoordinates);
    HuSprGrpPosSet(object->work[0], flagScreenCoordinates.x, flagScreenCoordinates.y);
}

Void Story3PositionPlayerModels(Void) {
    s32 playerIndex;
    s32 modelIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        for (modelIndex = 0; modelIndex < 3; modelIndex++) {
            Hu3DModelAttrReset(playerRecordProcessStates[playerIndex].object->mdlId[modelIndex + 1], HU3D_ATTR_DISPOFF);
        }
        if (resultState.teamMode == 1) {
            if (playerIndex % 2 == 0) {
                for (modelIndex = 0; modelIndex < 12; modelIndex++) {
                    HuSprAttrReset(playerRecordProcessStates[playerIndex].object->work[0], modelIndex, HUSPR_ATTR_DISPOFF);
                }
            }
            HuSprAttrReset(playerRecordProcessStates[playerIndex].object->work[0], 1, HUSPR_ATTR_DISPOFF);
        } else {
            for (modelIndex = 0; modelIndex < 12; modelIndex++) {
                HuSprAttrReset(playerRecordProcessStates[playerIndex].object->work[0], modelIndex, HUSPR_ATTR_DISPOFF);
            }
        }
        playerRecordProcessStates[playerIndex].callback = (Void*) Story3RotateResultBoardFlag;
    }
}

Void Story3PositionPlayerSprites(Void) {
    s32 playerIndex;
    s32 modelOrSpriteIndex;

    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        for (modelOrSpriteIndex = 0; modelOrSpriteIndex < 3; modelOrSpriteIndex++) {
            Hu3DModelAttrSet(playerRecordProcessStates[playerIndex].object->mdlId[modelOrSpriteIndex + 1], HU3D_ATTR_DISPOFF);
        }
        for (modelOrSpriteIndex = 0; modelOrSpriteIndex < 12; modelOrSpriteIndex++) {
            HuSprAttrSet(playerRecordProcessStates[playerIndex].object->work[0], modelOrSpriteIndex, HUSPR_ATTR_DISPOFF);
        }
        playerRecordProcessStates[playerIndex].callback = NULL;
    }
}

Void Story3CreatePlayerRecordSprites(OMOBJ* object, s32 count) {
    StoryResultPlayerStats* playerResult;
    s32 spriteId;
    s32 i;
    s32 j;

    playerResult = &resultState.playerResults[count];
    object->work[1] = HuSprGrpCreate(1);
    spriteId = HuSprCreate(HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MSTORY3, 122), HU_MEMNUM_OVL)), 700, 0);
    HuSprGrpMemberSet(object->work[1], 0, spriteId);
    HuSprBankSet(object->work[1], 0, playerResult->characterId);
    // TODO: figure out the actual args of this story3_position_centered_sprite call.
    // It will match as long as the values (leftX + width / 2) and (topY + height / 2) are preserved.
    Story3PositionCenteredSprite(object->work[1], 0, 66, 21, 0, 0);
    object->work[2] = HuSprGrpCreate(0x24);
    for (i = 0; i < 12; i++) {
        for (j = 0; j < 3; j++) {
            spriteId = HuSprCreate(recordMedalAnimations[playerResult->minigameResults[i][1]], 700, 0);
            HuSprGrpMemberSet(object->work[2], j + i * 3, spriteId);
        }
        Story3DrawNumberSprites(object->work[2], i * 3, playerResult->minigameResults[i][0], i * 66, 0, 20, 32, 1);
    }
    HuSprGrpDrawNoSet(object->work[1], 0x7F);
    HuSprGrpDrawNoSet(object->work[2], 0x7F);
}

Void Story3UpdateRecordList(OMOBJ* object, StorySequenceProcessState* processState) {
    s32 i;
    s32 j;
    s32 visiblePageStart;

    visiblePageStart = resultState.selectedRecordPage - resultState.recordScrollOffset;
    if (visiblePageStart < 0) {
        visiblePageStart = 0;
    }
    HuSprGrpPosSet(object->work[1], 17.0f, processState->workValue0 * 48 + 118);
    HuSprGrpPosSet(object->work[2], 169 - visiblePageStart * 66, processState->workValue0 * 48 + 123);
    for (i = 0; i < 12; i++) {
        for (j = 0; j < 3; j++) {
            if (i < visiblePageStart || i > visiblePageStart + 5) {
                HuSprAttrSet(object->work[2], j + i * 3, HUSPR_ATTR_DISPOFF);
            } else {
                HuSprAttrReset(object->work[2], j + i * 3, HUSPR_ATTR_DISPOFF);
            }
        }
    }
}

Void Story3ShowRecordSprites(Void) {
    s32 i, j, k;

    for (i = 0; i < 4; i++) {
        HuSprAttrReset(playerRecordProcessStates[i].object->work[1], 0, HUSPR_ATTR_DISPOFF);
        for (j = 0; j < 12; j++) {
            for (k = 0; k < 3; k++) {
                HuSprAttrReset(playerRecordProcessStates[i].object->work[2], k + j * 3, HUSPR_ATTR_DISPOFF);
            }
        }
        playerRecordProcessStates[i].callback = (Void*) Story3UpdateRecordList;
    }
}

Void Story3HideRecordSprites(Void) {
    s32 i, j, k;

    for (i = 0; i < 4; i++) {
        HuSprAttrSet(playerRecordProcessStates[i].object->work[1], 0, HUSPR_ATTR_DISPOFF);
        for (j = 0; j < 12; j++) {
            for (k = 0; k < 3; k++) {
                HuSprAttrSet(playerRecordProcessStates[i].object->work[2], k + j * 3, HUSPR_ATTR_DISPOFF);
            }
        }
        playerRecordProcessStates[i].callback = NULL;
    }
}

Void Story3AddPlayerRecord(Void) {
    OMOBJ* playerObject;
    s32 playerIndex;

    playerIndex = resultState.playerRecordCount++;
    playerObject = omAddObjEx(resultProcess, 0x100, 0x10, 0x10, -1, NULL);
    playerRecordProcessStates[playerIndex].object = playerObject;
    playerRecordProcessStates[playerIndex].callback = NULL;
    playerRecordProcessStates[playerIndex].workValue0 = playerIndex;
    Story3CalculateTeamMinigameScores(playerObject, playerIndex);
    Story3CreatePlayerRecordSprites(playerObject, playerIndex);
    while (TRUE) {
        Story3SleepOneFrame();
        if (playerRecordProcessStates[playerIndex].callback != NULL) {
            playerRecordProcessStates[playerIndex].callback(playerObject, &playerRecordProcessStates[playerIndex]);
        }
    }
}

Void Story3StartCharacterModelMotion(Void) {
    if (playerModelProcessState.object->work[0] != 1) {
        Story3SetModelMotion(playerModelProcessState.object, 4, 6, 15, 0);
        playerModelProcessState.object->work[0] = 1;
        playerModelProcessState.object->work[1] = 4;
        playerModelProcessState.object->work[2] = 16;
    }
}

Void Story3LoadResultCharacterModels(OMOBJ* object) {
    s32 characterModelDataIds[] = {
        DATA_MAKE_NUM(DATADIR_MSTORY3, 46),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 40),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 52),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 58),
        DATA_MAKE_NUM(DATADIR_MSTORY3, 64)
    };
    s32 characterMotionDataIds[][3] = {
        { DATA_MAKE_NUM(DATADIR_MSTORY3, 47), DATA_MAKE_NUM(DATADIR_MSTORY3, 48), DATA_MAKE_NUM(DATADIR_MSTORY3, 49) },
        { DATA_MAKE_NUM(DATADIR_MSTORY3, 41), DATA_MAKE_NUM(DATADIR_MSTORY3, 42), DATA_MAKE_NUM(DATADIR_MSTORY3, 43) },
        { DATA_MAKE_NUM(DATADIR_MSTORY3, 53), DATA_MAKE_NUM(DATADIR_MSTORY3, 54), DATA_MAKE_NUM(DATADIR_MSTORY3, 55) },
        { DATA_MAKE_NUM(DATADIR_MSTORY3, 59), DATA_MAKE_NUM(DATADIR_MSTORY3, 60), DATA_MAKE_NUM(DATADIR_MSTORY3, 61) },
        { DATA_MAKE_NUM(DATADIR_MSTORY3, 65), DATA_MAKE_NUM(DATADIR_MSTORY3, 66), DATA_MAKE_NUM(DATADIR_MSTORY3, 67) }
    };
    s32 i;

    for (i = 0; i < 5; i++) {
        object->mdlId[i * 3 + 1] = Hu3DModelCreateFile(characterModelDataIds[i]);
        object->mtnId[i * 3 + 1] = Hu3DJointMotionFile(object->mdlId[i * 3 + 1], characterMotionDataIds[i][0]);
        object->mtnId[i * 3 + 2] = Hu3DJointMotionFile(object->mdlId[i * 3 + 1], characterMotionDataIds[i][1]);
        object->mtnId[i * 3 + 3] = Hu3DJointMotionFile(object->mdlId[i * 3 + 1], characterMotionDataIds[i][2]);
        Hu3DModelRotSet(object->mdlId[i * 3 + 1], 17.5f, 0.0f, 0.0f);
        Hu3DModelScaleSet(object->mdlId[i * 3 + 1], 0.575f, 0.575f, 0.575f);
        Story3SetModelMotion(object, i * 3 + 1, i * 3 + 1, 0, 1);
        {
            s32 characterDustEffectIds[] = { 9, 14, 10, 11, 12, 13 };

            if (i != 3) {
                CharNpcDustSet(object->mdlId[i * 3 + 1], object->mtnId[i * 3 + 2], 1, characterDustEffectIds[i]);
                CharModelFxFlagSet(characterDustEffectIds[i], 0);
            }
        }
    }
}

Void Story3UpdatePlayerModelPositions(OMOBJ* object, StorySequenceProcessState* processState) {
    s32 i;

    for (i = 0; i < 5; i++) {
        Hu3DModelPosSet(object->mdlId[i * 3 + 1], resultState.playerModelPositions[i].x, resultState.playerModelPositions[i].y, resultState.playerModelPositions[i].z);
        if (i == 3) {
            Hu3DData[object->mdlId[i * 3 + 1]].pos.y += 20.0f;
        }
    }
    if (processState->workValue0++ >= 3000) {
        processState->workValue0 = 0;
        processState->processState = 1;
        recordPageProcessState.workValue0++;
        recordPageProcessState.workValue0 %= 2;
        Story3SetModelMotion(playerModelProcessState.object, 1, 2, 15, 1);
    }
    if (processState->processState == 1) {
        resultState.playerModelPositions[0].x += -5.0f + 10.0f * recordPageProcessState.workValue0;
        if (recordPageProcessState.workValue0 != 0) {
            Hu3DData[playerModelProcessState.object->mdlId[1]].rot.y = interpolateSmoothAverage(Hu3DData[playerModelProcessState.object->mdlId[1]].rot.y, 90.0f, 5.0f);
            if (resultState.playerModelPositions[0].x >= 185.0f) {
                processState->processState = 2;
                resultState.playerModelPositions[0].x = 185.0f;
                Story3SetModelMotion(playerModelProcessState.object, 1, 1, 15, 1);
            }
        } else {
            Hu3DData[playerModelProcessState.object->mdlId[1]].rot.y = interpolateSmoothAverage(Hu3DData[playerModelProcessState.object->mdlId[1]].rot.y, -90.0f, 5.0f);
            if (resultState.playerModelPositions[0].x <= -175.0f) {
                processState->processState = 2;
                resultState.playerModelPositions[0].x = -175.0f;
                Story3SetModelMotion(playerModelProcessState.object, 1, 1, 15, 1);
            }
        }
    } else if (processState->processState == 2) {
        if (recordPageProcessState.workValue0 != 0) {
            Hu3DData[playerModelProcessState.object->mdlId[1]].rot.y = interpolateSmoothAverage(Hu3DData[playerModelProcessState.object->mdlId[1]].rot.y, -15.0f, 15.0f);
            if (Hu3DData[playerModelProcessState.object->mdlId[1]].rot.y <= -14.9f) {
                processState->processState = 0;
            }
        } else {
            Hu3DData[playerModelProcessState.object->mdlId[1]].rot.y = interpolateSmoothAverage(Hu3DData[playerModelProcessState.object->mdlId[1]].rot.y, 15.0f, 15.0f);
            if (Hu3DData[playerModelProcessState.object->mdlId[1]].rot.y >= 14.9f) {
                processState->processState = 0;
            }
        }
    }
}

Void Story3ShowResultPlayerModels(Void) {
    s32 i;

    for (i = 0; i < 5; i++) {
        Hu3DModelAttrReset(playerModelProcessState.object->mdlId[(i * 3) + 1], HU3D_ATTR_DISPOFF);
    }
    Hu3DModelRotSet(playerModelProcessState.object->mdlId[4], 17.5f, 15.0f, 0.0f);
    Hu3DModelScaleSet(playerModelProcessState.object->mdlId[4], 0.575f, 0.575f, 0.575f);
    Story3SetModelMotion(playerModelProcessState.object, 4, 4, 0, 1);
    playerModelProcessState.object->work[0] = playerModelProcessState.object->work[1] = playerModelProcessState.object->work[2] = 0;
    playerModelProcessState.callback = (Void*) Story3UpdatePlayerModelPositions;
}

Void Story3HideResultPlayerModels(Void) {
    s32 i;

    for (i = 0; i < 5; i++) {
        Hu3DModelAttrSet(playerModelProcessState.object->mdlId[i * 3 + 1], HU3D_ATTR_DISPOFF);
    }
    Story3SetModelMotion(playerModelProcessState.object, 1, 1, 0, 1);
    if (recordPageProcessState.workValue0 != 0) {
        resultState.playerModelPositions[0].x = 185.0f;
        Hu3DData[playerModelProcessState.object->mdlId[1]].rot.y = -15.0f;
    } else {
        resultState.playerModelPositions[0].x = -175.0f;
        Hu3DData[playerModelProcessState.object->mdlId[1]].rot.y = 15.0f;
    }
    playerModelProcessState.processState = 0;
    playerModelProcessState.callback = NULL;
}

Void Story3UpdateFlagAnimation(OMOBJ* object, StorySequenceProcessState* processState) {
    if (object->work[0] != 0) {
        if (object->work[2] != 0) {
            object->work[2]--;
            return;
        }
        if (object->work[2] == 0 && Hu3DMotionEndCheck(object->mdlId[4])) {
            Story3SetModelMotion(object, 4, object->work[1], 15, 1);
            object->work[0] = object->work[1] = object->work[2] = 0;
        }
    }
}

Void Story3ShowBoardFlag(Void) {
    Hu3DModelAttrReset(playerModelProcessState.object->mdlId[4], HU3D_ATTR_DISPOFF);
    Hu3DModelPosSet(playerModelProcessState.object->mdlId[4], -185.0f, -225.0f, 0.0f);
    Hu3DModelRotSet(playerModelProcessState.object->mdlId[4], 0.0f, 15.0f, 0.0f);
    Hu3DModelScaleSet(playerModelProcessState.object->mdlId[4], 1.0f, 1.0f, 1.0f);
    playerModelProcessState.callback = (Void*) Story3UpdateFlagAnimation;
}

Void Story3HideBoardFlag(Void) {
    Hu3DModelAttrSet(playerModelProcessState.object->mdlId[4], HU3D_ATTR_DISPOFF);
    playerModelProcessState.callback = NULL;
}

Void Story3CreateBoardFlagObject(Void) {
    OMOBJ* createdResultObject;

    createdResultObject = omAddObjEx(resultProcess, 0x100, 0x10, 0x10, -1, NULL);
    playerModelProcessState.object = createdResultObject;
    playerModelProcessState.callback = NULL;
    Story3LoadResultCharacterModels(createdResultObject);
    while (TRUE) {
        Story3SleepOneFrame();
        if (playerModelProcessState.callback != NULL) {
            playerModelProcessState.callback(createdResultObject, &playerModelProcessState);
        }
    }
}

Void Story3ResetResultCamera(Void) {
    StoryCameraState* resultCameraState;

    resultCameraState = &storyCameraState;
    resultCameraState->cameraCenter.x = 0.0f;
    resultCameraState->cameraCenter.y = 0.0f;
    resultCameraState->cameraCenter.z = 0.0f;
    resultCameraState->cameraRotation.x = 0.0f;
    resultCameraState->cameraRotation.y = 0.0f;
    resultCameraState->cameraRotation.z = 0.0f;
    resultCameraState->cameraZoom = 650.0f;
}

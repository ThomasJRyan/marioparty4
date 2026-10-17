#include "REL/mgmodedll.h"
#ifndef __MWERKS__
#include "game/frand.h"
#endif
#include "game/gamework.h"

#include "game/audio.h"
#include "game/chrman.h"
#include "game/esprite.h"
#include "game/gamework_data.h"
#include "game/minigame_seq.h"
#include "game/saveload.h"
#include "game/wipe.h"

#include "ext_math.h"

s16 ticTacToeWindow;
s16 ticTacToeCursorPositions[2];
s16 ticTacToePreviousCursorPositions[2];
s16 ticTacToeCursorTimers[2];
s16 ticTacToeStatusSpriteGroup;
s16 ticTacToeBoardSpriteGroup;

s16 ticTacToeMessageWindow = -1;
s16 ticTacToeWinningLines[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 3, 6, 1, 4, 7, 2, 5, 8, 0, 4, 8, 2, 4, 6 };

float ticTacToePlayerPositions[4] = { -80, -38, 38, 80 };

s16 ticTacToeCenterCells[8] = { 4, 4, 4, 4, 0, 2, 6, 8 };

void ExitMinigameMode(s16 modeType);

void CreateTicTacToeInterface(void);
s32 ProcessTicTacToeMove(s16 playerIndex, s32 inputEnabled);

void ExitTicTacToeMinigame(void);
s32 ShowTicTacToeInstructions(void);
void StartTicTacToeRound(void);
void AnimateTicTacToeBoard(void);

s32 RunTicTacToeMinigame(void)
{
    s16 animationFrame;
    s16 gridRow;
    s16 choiceResult;
    s16 gridColumn;
    HU3DMODEL *modelWork;
    Vec modelPosition;
    Vec targetPosition;
    Vec transitionPosition;
    s16 playerMoveResult[2];
    float animationProgress;
    float rotationProgress;
    float modelRotation;

    BuildMinigameTypeLists(0);
    if (mgModeEvtNo != 4) {
        for (animationFrame = 0; animationFrame < 3; animationFrame++) {
            for (gridRow = 0; gridRow < 3; gridRow++) {
                mgTicTacToeGrid[animationFrame][gridRow] = 0;
            }
        }
    }
    CreateTicTacToeInterface();
    if (mgModeEvtNo == 4) {
        HuSprGrpPosSet(ticTacToeBoardSpriteGroup, 288, 240);
        HuSprGrpPosSet(ticTacToeStatusSpriteGroup, 450, 65);
        Hu3DModelPosSet(mgModeModelIds[5], -300, 0, 680);
        Hu3DModelRotSet(mgModeModelIds[5], 0, 20, 0);
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
        while (WipeStatGet()) {
            HuPrcVSleep();
        }
        if (ShowTicTacToeInstructions()) {
            if (ticTacToeMessageWindow == -1) {
                ticTacToeMessageWindow = CreateStandardMessageWindow(0, 0, 0);
            }
            HuWinAttrSet(ticTacToeMessageWindow, 16);
            ShowMessageAndWait(windowId, 0x28000C, -1, -1);
            choiceResult = HuWinChoiceGet(ticTacToeMessageWindow, 1);
            if (choiceResult == 0) {
                StartTicTacToeRound();
            }
            else {
                ExitTicTacToeMinigame();
                return 0;
            }
        }
    }
    else {
        modelWork = &Hu3DData[mgModeModelIds[5]];
        modelPosition = modelWork->pos;
        targetPosition.x = -300;
        targetPosition.y = 0;
        targetPosition.z = 680;
        VECSubtract(&targetPosition, &modelPosition, &targetPosition);
        modelRotation = (180 + atan2d(targetPosition.z, targetPosition.x)) - 360;
        for (animationFrame = 0; animationFrame <= 60; animationFrame++) {
            if (animationFrame <= 50) {
                animationProgress = 1.0 - sind((animationFrame / 50.0) * 90.0);
                HuSprGrpPosSet(ticTacToeBoardSpriteGroup, 288, 256 + (-400 * animationProgress));
            }
            else {
                animationProgress = cosd(((animationFrame - 50.0) / 10.0) * 90.0);
                HuSprGrpPosSet(ticTacToeBoardSpriteGroup, 288, 240 + (16 * animationProgress));
            }
            if (animationFrame > 20 && animationFrame <= 50) {
                animationProgress = 1.0 - sind(((animationFrame - 20) / 30.0) * 90.0);
                HuSprGrpPosSet(ticTacToeStatusSpriteGroup, 434 + (400 * animationProgress), 65);
            }
            else if (animationFrame > 50) {
                animationProgress = cosd(((animationFrame - 50.0) / 10.0) * 90.0);
                HuSprGrpPosSet(ticTacToeStatusSpriteGroup, 450 + (-16 * animationProgress), 65);
            }
            if (animationFrame == 30) {
                Hu3DMotionShiftSet(mgModeModelIds[5], mgModeMotionIds[7], 0, 8, HU3D_MOTATTR_LOOP);
            }
            if (animationFrame > 30) {
                animationProgress = (animationFrame - 30.0) / 30.0;
                VECScale(&targetPosition, &transitionPosition, animationProgress);
                VECAdd(&modelPosition, &transitionPosition, &transitionPosition);
                Hu3DModelPosSetV(mgModeModelIds[5], &transitionPosition);
                if (animationFrame <= 40) {
                    animationProgress = (animationFrame - 20) / 10.0;
                    Hu3DModelRotSet(mgModeModelIds[5], 0, modelRotation * animationProgress, 0);
                }
            }
            HuPrcVSleep();
        }
        Hu3DMotionShiftSet(mgModeModelIds[5], mgModeMotionIds[6], 0, 8, HU3D_MOTATTR_LOOP);
        for (animationFrame = 0; animationFrame <= 10; animationFrame++) {
            animationProgress = animationFrame / 10.0;
            Hu3DModelRotSet(mgModeModelIds[5], 0, modelRotation + ((20.0f - modelRotation) * animationProgress), 0);
            HuPrcVSleep();
        }
        for (animationFrame = 0; animationFrame < 3; animationFrame++) {
            for (gridRow = 0; gridRow < 3; gridRow++) {
                mgTicTacToeGrid[animationFrame][gridRow] = 0;
            }
        }
    }
    ticTacToeWindow = CreateCenteredMessageWindow(0x280019);
    if (ticTacToeMessageWindow == -1) {
        ticTacToeMessageWindow = CreateStandardMessageWindow(0, 0, 0);
    }
    ShowMessageAndWait(windowId, 0x280015, -1, -1);
    ticTacToeCursorPositions[0] = 0;
    ticTacToeCursorPositions[1] = 2;
    AnimateTicTacToeBoard();
    ticTacToeCursorTimers[0] = ticTacToeCursorTimers[1] = 30;
    HuSprAttrReset(ticTacToeBoardSpriteGroup, 12, HUSPR_ATTR_DISPOFF);
    HuSprTPLvlSet(ticTacToeBoardSpriteGroup, 12, 0.5f);
    HuSprAttrReset(ticTacToeBoardSpriteGroup, 13, HUSPR_ATTR_DISPOFF);
    HuSprTPLvlSet(ticTacToeBoardSpriteGroup, 13, 0.5f);
    rotationProgress = 0;
    playerMoveResult[0] = playerMoveResult[1] = 0;
    for (animationFrame = 0; animationFrame < 9; animationFrame++) {
        mgTicTacToeGrid[animationFrame % 3][animationFrame / 3] &= 0x3;
    }
    while (1) {
        for (animationFrame = 0; animationFrame < 2; animationFrame++) {
            if (playerMoveResult[animationFrame] == 0) {
                playerMoveResult[animationFrame] = ProcessTicTacToeMove(playerIndex, 1);
                if (playerMoveResult[animationFrame] == -1) {
                    playerMoveResult[animationFrame] = 0;
                    goto elseCase;
                }
                else if (playerMoveResult[animationFrame] == 1) {
                    HuSprAttrSet(ticTacToeBoardSpriteGroup, animationFrame + 12, HUSPR_ATTR_DISPOFF);
                    HuSprAttrReset(ticTacToeBoardSpriteGroup, animationFrame + 14, HUSPR_ATTR_DISPOFF);
                    if (ticTacToeCursorPositions[0] != ticTacToeCursorPositions[1]) {
                        HuSprPosSet(
                            ticTacToeBoardSpriteGroup, animationFrame + 14, ((ticTacToeCursorPositions[animationFrame] % 3) * 56) - 56, ((ticTacToeCursorPositions[animationFrame] / 3) * 56) - 56);
                    }
                    else {
                        HuSprPosSet(ticTacToeBoardSpriteGroup, animationFrame + 14, ((ticTacToeCursorPositions[animationFrame] % 3) * 56) - 60 + (animationFrame * 8),
                            ((ticTacToeCursorPositions[animationFrame] / 3) * 56) - 56);
                    }
                }
                else {
                elseCase:
                    if (ticTacToeCursorPositions[0] != ticTacToeCursorPositions[1]) {
                        HuSprPosSet(
                            ticTacToeBoardSpriteGroup, animationFrame + 12, ((ticTacToeCursorPositions[animationFrame] % 3) * 56) - 56, ((ticTacToeCursorPositions[animationFrame] / 3) * 56) - 56);
                    }
                    else {
                        HuSprPosSet(ticTacToeBoardSpriteGroup, animationFrame + 12, ((ticTacToeCursorPositions[animationFrame] % 3) * 56) - 60 + (animationFrame * 8),
                            ((ticTacToeCursorPositions[animationFrame] / 3) * 56) - 56);
                    }
                    HuSprTPLvlSet(ticTacToeBoardSpriteGroup, animationFrame + 12, 0.7 + (0.2 * sind(rotationProgress + (animationFrame * 180))));
                    HuSprPriSet(ticTacToeBoardSpriteGroup, animationFrame + 12, 150);
                }
            }
            else {
                playerMoveResult[animationFrame] = ProcessTicTacToeMove(playerIndex, 0);
                if (playerMoveResult[animationFrame] == -1) {
                    playerMoveResult[animationFrame] = 0;
                    HuSprAttrReset(ticTacToeBoardSpriteGroup, animationFrame + 12, HUSPR_ATTR_DISPOFF);
                    HuSprAttrSet(ticTacToeBoardSpriteGroup, animationFrame + 14, HUSPR_ATTR_DISPOFF);
                    if (ticTacToeCursorPositions[0] != ticTacToeCursorPositions[1]) {
                        HuSprPosSet(
                            ticTacToeBoardSpriteGroup, animationFrame + 12, ((ticTacToeCursorPositions[animationFrame] % 3) * 56) - 56, ((ticTacToeCursorPositions[animationFrame] / 3) * 56) - 56);
                    }
                    else {
                        HuSprPosSet(ticTacToeBoardSpriteGroup, animationFrame + 12, ((ticTacToeCursorPositions[animationFrame] % 3) * 56) - 60 + (animationFrame * 8),
                            ((ticTacToeCursorPositions[animationFrame] / 3) * 56) - 56);
                    }
                }
                else {
                    HuSprTPLvlSet(ticTacToeBoardSpriteGroup, animationFrame + 12, 1.0f);
                    HuSprPriSet(ticTacToeBoardSpriteGroup, animationFrame + 12, animationFrame + 180);
                    if (ticTacToeCursorPositions[0] != ticTacToeCursorPositions[1]) {
                        HuSprPosSet(
                            ticTacToeBoardSpriteGroup, animationFrame + 14, ((ticTacToeCursorPositions[animationFrame] % 3) * 56) - 56, ((ticTacToeCursorPositions[animationFrame] / 3) * 56) - 56);
                    }
                    else {
                        HuSprPosSet(ticTacToeBoardSpriteGroup, animationFrame + 14, ((ticTacToeCursorPositions[animationFrame] % 3) * 56) - 60 + (animationFrame * 8),
                            ((ticTacToeCursorPositions[animationFrame] / 3) * 56) - 56);
                    }
                }
            }
        }
        if (playerMoveResult[0] == 1 && playerMoveResult[1] == 1) {
            HuSprTPLvlSet(ticTacToeBoardSpriteGroup, 12, 1.0f);
            HuSprTPLvlSet(ticTacToeBoardSpriteGroup, 13, 1.0f);
            break;
        }
        else {
            rotationProgress += 10.0f;
            if (rotationProgress > 360.0f) {
                rotationProgress -= 360.0f;
            }
            HuPrcVSleep();
        }
    }
    CloseCenteredMessageWindow(windowId);
    ticTacToeWindow = -1;
    ShowMessageAndWait(windowId, 0x280010, -1, -1);
    HuSprTPLvlSet(ticTacToeBoardSpriteGroup, 24, 1.0f);
    mgTypeCurr = 2;
    for (gridRow = 0; gridRow < 35;) {
        for (animationFrame = 0; animationFrame < availableMinigameCountByType[freePlaySelectionOrder[mgTypeCurr]]; animationFrame++) {
            gridColumn = SetMinigamePreviewSprite(availableMinigameIdsByType[freePlaySelectionOrder[mgTypeCurr]][animationFrame], 288, 240);
            HuSprGrpDrawNoSet(gridColumn, 0);
            HuAudFXPlay(855);
            HuPrcSleep(3);
            gridRow++;
            if (gridRow >= 35) {
                break;
            }
        }
    }
    HuAudFXPlay(870);
    freePlayWindowState = availableMinigameIdsByType[freePlaySelectionOrder[mgTypeCurr]][frandmod(availableMinigameCountByType[freePlaySelectionOrder[mgTypeCurr]])];
    gridColumn = SetMinigamePreviewSprite(spriteGroupId, 288, 240);
    HuSprGrpDrawNoSet(gridColumn, 0);
    HuWinInsertMesSet(ticTacToeMessageWindow, mgInfoTbl[freePlayWindowState].name_mess, 0);
    HuWinAttrSet(ticTacToeMessageWindow, 16);
    ShowMessageAndWait(windowId, 0x280011, -1, -1);
repeatMess:
    ShowMessageAndWait(windowId, 0x280029, -1, -1);
    choiceResult = HuWinChoiceGet(ticTacToeMessageWindow, 0);
    if (choiceResult == 0) {
        _ClearFlag(11);
    }
    else if (choiceResult == 1) {
        HuWinInsertMesSet(ticTacToeMessageWindow, MAKE_MESSID(0x28, 40), 0);
        ShowMessageAndWait(windowId, 0x280026, -1, -1);
        choiceResult = HuWinChoiceGet(ticTacToeMessageWindow, 1);
        if (choiceResult == 1) {
            goto repeatMess;
        }
        SetMinigamePreviewSprite(-1, 0, 0);
        for (animationFrame = 1; animationFrame <= 10; animationFrame++) {
            HuSprTPLvlSet(ticTacToeBoardSpriteGroup, 24, 1 - (animationFrame / 10.0));
            HuPrcVSleep();
        }

        ExitTicTacToeMinigame();
        return 1;
    }
    else {
        _SetFlag(11);
    }
    GWSystem.mg_next = freePlayWindowState;
    HuDataDirClose(DATADIR_INSTPIC);
    ExitMinigameMode(4);
    while (1) {
        HuPrcVSleep();
    }
}

void CreateTicTacToeInterface(void)
{
    s16 playerIndex;
    s16 spriteGroupId;
    s16 spriteId;
    ANIMDATA *spriteAnimation;
    s16 occupiedCellSpriteCount;
    s16 gridOffsetX;
    s16 gridOffsetY;
    s16 cellFlags;
    ANIMDATA *markAnimation;
    ANIMDATA *characterSpriteAnimation;
    s16 playerStatusAnimation;
    s16 characterId;
    s16 isComputerControlled;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerOrderByGroup[playerIndex] = playerIndex;
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        for (occupiedCellSpriteCount = 0; occupiedCellSpriteCount < 3 - playerIndex; occupiedCellSpriteCount++) {
            if (GWPlayerCfg[playerOrderByGroup[occupiedCellSpriteCount]].group > GWPlayerCfg[playerOrderByGroup[occupiedCellSpriteCount + 1]].group) {
                cellFlags = playerOrderByGroup[occupiedCellSpriteCount];
                playerOrderByGroup[occupiedCellSpriteCount] = playerOrderByGroup[occupiedCellSpriteCount + 1];
                playerOrderByGroup[occupiedCellSpriteCount + 1] = cellFlags;
            }
        }
    }
    if (ticTacToeBoardSpriteGroup == 0) {

        ticTacToeBoardSpriteGroup = spriteGroupId = HuSprGrpCreate(25);
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 84), HU_MEMNUM_OVL));
        spriteId = HuSprCreate(spriteAnimation, 200, 0);
        HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
        HuSprTPLvlSet(spriteGroupId, 0, 0.8f);
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 85), HU_MEMNUM_OVL));
        spriteId = HuSprCreate(spriteAnimation, 190, 0);
        HuSprGrpMemberSet(spriteGroupId, 1, spriteId);
        HuSprPosSet(spriteGroupId, 1, 0, -110);
        gridOffsetX = 0;
        gridOffsetY = 0;
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 86), HU_MEMNUM_OVL));
        spriteId = HuSprCreate(spriteAnimation, 190, 0);
        HuSprGrpMemberSet(spriteGroupId, 2, spriteId);
        HuSprPosSet(spriteGroupId, 2, gridOffsetX, gridOffsetY);
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 87), HU_MEMNUM_OVL));
        characterSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 88), HU_MEMNUM_OVL));
        for (playerIndex = occupiedCellSpriteCount = 0; playerIndex < 9; playerIndex++) {
            if (mgTicTacToeGrid[playerIndex % 3][playerIndex / 3] & 0xF) {
                cellFlags = mgTicTacToeGrid[playerIndex % 3][playerIndex / 3];
                if (cellFlags & 0x4) {
                    markAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 91), HU_MEMNUM_OVL));
                    spriteId = HuSprCreate(markAnimation, 180, 0);
                    HuSprGrpMemberSet(spriteGroupId, 14, spriteId);
                    if (cellFlags & 0x8) {
                        HuSprPosSet(spriteGroupId, 14, gridOffsetX + ((playerIndex % 3) * 56) - 60, gridOffsetY + ((playerIndex / 3) * 56) - 56);
                    }
                    else {
                        HuSprPosSet(spriteGroupId, 14, gridOffsetX + ((playerIndex % 3) * 56) - 56, gridOffsetY + ((playerIndex / 3) * 56) - 56);
                    }
                    occupiedCellSpriteCount++;
                }
                if (cellFlags & 0x8) {
                    markAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 92), HU_MEMNUM_OVL));
                    spriteId = HuSprCreate(markAnimation, 180, 0);
                    HuSprGrpMemberSet(spriteGroupId, 15, spriteId);
                    if (cellFlags & 0x4) {
                        HuSprPosSet(spriteGroupId, 15, gridOffsetX + ((playerIndex % 3) * 56) - 48, gridOffsetY + ((playerIndex / 3) * 56) - 56);
                    }
                    else {
                        HuSprPosSet(spriteGroupId, 15, gridOffsetX + ((playerIndex % 3) * 56) - 56, gridOffsetY + ((playerIndex / 3) * 56) - 56);
                    }
                    occupiedCellSpriteCount++;
                }
                if (cellFlags & 0x3) {
                    if (mgTicTacToeGrid[playerIndex % 3][playerIndex / 3] & 0x1) {
                        spriteId = HuSprCreate(spriteAnimation, 180, 0);
                    }
                    else {
                        spriteId = HuSprCreate(characterSpriteAnimation, 180, 0);
                    }
                    HuSprGrpMemberSet(spriteGroupId, playerIndex + 3, spriteId);
                    HuSprPosSet(spriteGroupId, playerIndex + 3, gridOffsetX + ((playerIndex % 3) * 56) - 56, gridOffsetY + ((playerIndex / 3) * 56) - 56);
                }
            }
        }
        if (occupiedCellSpriteCount == 0) {
            markAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 91), HU_MEMNUM_OVL));
            spriteId = HuSprCreate(markAnimation, 150, 0);
            HuSprGrpMemberSet(spriteGroupId, 14, spriteId);
            HuSprPosSet(spriteGroupId, 14, gridOffsetX + ((playerIndex % 3) * 56) - 56, gridOffsetY + ((playerIndex / 3) * 56) - 56);
            HuSprAttrSet(spriteGroupId, 14, HUSPR_ATTR_DISPOFF);
            markAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 92), HU_MEMNUM_OVL));
            spriteId = HuSprCreate(markAnimation, 150, 0);
            HuSprGrpMemberSet(spriteGroupId, 15, spriteId);
            HuSprPosSet(spriteGroupId, 15, gridOffsetX + ((playerIndex % 3) * 56) - 56, gridOffsetY + ((playerIndex / 3) * 56) - 56);
            HuSprAttrSet(spriteGroupId, 15, HUSPR_ATTR_DISPOFF);
        }
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 89), HU_MEMNUM_OVL));
        spriteId = HuSprCreate(spriteAnimation, 150, 0);
        HuSprGrpMemberSet(spriteGroupId, 12, spriteId);
        HuSprAttrSet(spriteGroupId, 12, HUSPR_ATTR_DISPOFF);
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 90), HU_MEMNUM_OVL));
        spriteId = HuSprCreate(spriteAnimation, 150, 0);
        HuSprGrpMemberSet(spriteGroupId, 13, spriteId);
        HuSprAttrSet(spriteGroupId, 13, HUSPR_ATTR_DISPOFF);
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 101), HU_MEMNUM_OVL));
        characterSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 102), HU_MEMNUM_OVL));
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            spriteId = HuSprCreate(spriteAnimation, 130, playerIndex);
            HuSprGrpMemberSet(spriteGroupId, (playerIndex * 2) + 16, spriteId);
            HuSprPosSet(spriteGroupId, (playerIndex * 2) + 16, ((playerIndex % 2) * 314) - 155, ((playerIndex / 2) * 185) - 110);
            spriteId = HuSprCreate(characterSpriteAnimation, 135, playerIndex);
            HuSprGrpMemberSet(spriteGroupId, (playerIndex * 2) + 17, spriteId);
            HuSprTPLvlSet(spriteGroupId, (playerIndex * 2) + 17, 0.5f);
            HuSprPosSet(spriteGroupId, (playerIndex * 2) + 17, ((playerIndex % 2) * 314) - 155, ((playerIndex / 2) * 185) - 102);
        }
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 37), HU_MEMNUM_OVL));
        spriteId = HuSprCreate(spriteAnimation, 150, 0);
        HuSprGrpMemberSet(spriteGroupId, 24, spriteId);
        HuSprPosSet(spriteGroupId, 24, 0, 0);
        HuSprScaleSet(spriteGroupId, 24, 0.9f, 0.9f);
        HuSprTPLvlSet(spriteGroupId, 24, 0);
    }
    if (ticTacToeStatusSpriteGroup == 0) {

        ticTacToeStatusSpriteGroup = spriteGroupId = HuSprGrpCreate(10);
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 15), HU_MEMNUM_OVL));
        spriteId = HuSprCreate(spriteAnimation, 200, 0);
        HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
        HuSprTPLvlSet(spriteGroupId, 0, 0.8f);
        spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 25), HU_MEMNUM_OVL));
        spriteId = HuSprCreate(spriteAnimation, 190, 0);
        HuSprGrpMemberSet(spriteGroupId, 1, spriteId);
        HuSprPosSet(spriteGroupId, 1, 0, 0);
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            characterId = playerCharacterIdsByGroup[playerIndex] = GWPlayerCfg[playerOrderByGroup[playerIndex]].character;
            spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 16) + characterId, HU_MEMNUM_OVL));
            spriteId = HuSprCreate(spriteAnimation, 190, 0);
            HuSprGrpMemberSet(spriteGroupId, (playerIndex * 2) + 2, spriteId);
            HuSprPosSet(spriteGroupId, (playerIndex * 2) + 2, ticTacToePlayerPositions[playerIndex], 0);
            if (isComputerControlled = GWPlayerCfg[playerOrderByGroup[playerIndex]].iscom) {
                spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 39), HU_MEMNUM_OVL));
                playerStatusAnimation = GWPlayerCfg[playerOrderByGroup[playerIndex]].diff;
            }
            else {
                spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 38), HU_MEMNUM_OVL));
                playerStatusAnimation = GWPlayerCfg[playerOrderByGroup[playerIndex]].pad_idx;
            }
            spriteId = HuSprCreate(spriteAnimation, 180, playerStatusAnimation);
            HuSprGrpMemberSet(spriteGroupId, (playerIndex * 2) + 3, spriteId);
            HuSprPosSet(spriteGroupId, (playerIndex * 2) + 3, ticTacToePlayerPositions[playerIndex], 24);
        }
        HuSprGrpPosSet(spriteGroupId, 800, 240);
    }
}

s32 ProcessTicTacToeMove(s16 playerIndex, s32 inputEnabled)
{
    s16 cursorColumn;
    s16 cursorRow;
    s16 controllerPort;
    s16 playerIndex;
    s16 moveColumnDelta;
    s16 moveRowDelta;
    s16 currentColumn;
    s16 currentRow;
    controllerPort = 4;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (playerIndex == GWPlayerCfg[playerIndex].group && GWPlayerCfg[playerIndex].iscom == 0 && controllerPort > GWPlayerCfg[playerIndex].pad_idx) {
            controllerPort = GWPlayerCfg[playerIndex].pad_idx;
        }
    }
    if (controllerPort == 4) {
        if (inputEnabled == 0) {
            return 1;
        }
        cursorColumn = ticTacToeCursorPositions[playerIndex] % 3;
        cursorRow = ticTacToeCursorPositions[playerIndex] / 3;
        currentColumn = ticTacToePreviousCursorPositions[playerIndex] % 3;
        currentRow = ticTacToePreviousCursorPositions[playerIndex] / 3;
        if (--ticTacToeCursorTimers[playerIndex] != 0) {
            return 0;
        }
        if (cursorColumn == currentColumn && cursorRow == currentRow) {
            HuAudFXPlay(1);
            mgTicTacToeGrid[currentColumn][currentRow] |= 4 << playerIndex;
            return 1;
        }
        else {
            ticTacToeCursorTimers[playerIndex] = 30;
            moveColumnDelta = (currentColumn - cursorColumn < 0) ? -1 : (currentColumn - cursorColumn > 0) ? 1 : 0;
            moveRowDelta = (currentRow - cursorRow < 0) ? -1 : (currentRow - cursorRow > 0) ? 1 : 0;
            ticTacToeCursorPositions[playerIndex] = cursorColumn + moveColumnDelta + ((cursorRow + moveRowDelta) * 3);
            HuAudFXPlay(0);
            return 0;
        }
    }
    else {
        cursorColumn = ticTacToeCursorPositions[playerIndex] % 3;
        cursorRow = ticTacToeCursorPositions[playerIndex] / 3;
        if (inputEnabled == 0) {
            return 1;
        }
        if (HuPadBtnDown[controllerPort] & PAD_BUTTON_A) {
            if ((mgTicTacToeGrid[cursorColumn][cursorRow] & 0x3) == 0) {
                mgTicTacToeGrid[cursorColumn][cursorRow] |= 4 << playerIndex;
                HuAudFXPlay(1);
                return 1;
            }
            else {
                HuAudFXPlay(4);
            }
        }
        if (HuPadDStkRep[controllerPort] == 0) {
            return 0;
        }
        moveColumnDelta = moveRowDelta = 0;
        if ((HuPadDStkRep[controllerPort] & PAD_BUTTON_LEFT) && cursorColumn > 0) {
            moveColumnDelta = -1;
        }
        else if ((HuPadDStkRep[controllerPort] & PAD_BUTTON_RIGHT) && cursorColumn < 2) {
            moveColumnDelta = 1;
        }
        if ((HuPadDStkRep[controllerPort] & PAD_BUTTON_UP) && cursorRow > 0) {
            moveRowDelta = -1;
        }
        else if ((HuPadDStkRep[controllerPort] & PAD_BUTTON_DOWN) && cursorRow < 2) {
            moveRowDelta = 1;
        }
        if (moveColumnDelta || moveRowDelta) {
            HuAudFXPlay(0);
        }
        ticTacToeCursorPositions[playerIndex] = cursorColumn + moveColumnDelta + ((cursorRow + moveRowDelta) * 3);
        return 0;
    }
}

void ExitTicTacToeMinigame(void)
{
    s16 animationFrame;
    HU3DMODEL *modelWork;
    Vec modelPosition;
    Vec targetPosition;
    Vec transitionPosition;
    float animationProgress;
    float rotationProgress;

    CloseCenteredMessageWindow(windowId);
    CloseStandardMessageWindow(windowId);
    ticTacToeMessageWindow = ticTacToeWindow = -1;
    modelWork = &Hu3DData[mgModeModelIds[5]];
    modelPosition = modelWork->pos;
    targetPosition.x = 0;
    targetPosition.y = 0;
    targetPosition.z = 640;
    VECSubtract(&targetPosition, &modelPosition, &targetPosition);
    rotationProgress = 180 + atan2d(targetPosition.z, targetPosition.x);
    for (animationFrame = 0; animationFrame <= 60; animationFrame++) {
        if (animationFrame <= 10) {
            animationProgress = sind((animationFrame / 10.0) * 90.0);
            HuSprGrpPosSet(ticTacToeBoardSpriteGroup, 288, 240 + (16 * animationProgress));
        }
        else {
            animationProgress = 1.0 - cosd(((animationFrame - 10.0) / 50.0) * 90.0);
            HuSprGrpPosSet(ticTacToeBoardSpriteGroup, 288, 256 + (-400 * animationProgress));
        }
        if (animationFrame <= 10) {
            animationProgress = sind((animationFrame / 10.0) * 90.0);
            HuSprGrpPosSet(ticTacToeStatusSpriteGroup, 450 + (-16 * animationProgress), 65);
        }
        else if (animationFrame < 30) {
            animationProgress = 1 - cosd(((animationFrame - 10.0) / 20.0) * 90.0);
            HuSprGrpPosSet(ticTacToeStatusSpriteGroup, 434 + (400 * animationProgress), 65);
        }
        if (animationFrame == 29) {
            Hu3DMotionTimeSet(mgModeModelIds[5], 0);
        }
        if (animationFrame == 30) {
            Hu3DMotionShiftSet(mgModeModelIds[5], mgModeMotionIds[7], 0, 8, HU3D_MOTATTR_LOOP);
        }
        if (animationFrame > 30) {
            animationProgress = (animationFrame - 30.0) / 30.0;
            VECScale(&targetPosition, &transitionPosition, animationProgress);
            VECAdd(&modelPosition, &transitionPosition, &transitionPosition);
            Hu3DModelPosSetV(mgModeModelIds[5], &transitionPosition);
            if (animationFrame <= 40) {
                animationProgress = (animationFrame - 30) / 10.0;
                Hu3DModelRotSet(mgModeModelIds[5], 0, rotationProgress * animationProgress, 0);
            }
        }
        HuPrcVSleep();
    }
    Hu3DMotionShiftSet(mgModeModelIds[5], mgModeMotionIds[6], 0, 8, HU3D_MOTATTR_LOOP);
    for (animationFrame = 0; animationFrame <= 10; animationFrame++) {
        animationProgress = 1.0 - (animationFrame / 10.0);
        Hu3DModelRotSet(mgModeModelIds[5], 0, animationProgress * rotationProgress, 0);
        HuPrcVSleep();
    }
    HuSprGrpKill(ticTacToeBoardSpriteGroup);
    ticTacToeBoardSpriteGroup = 0;
    HuSprGrpKill(ticTacToeStatusSpriteGroup);
    ticTacToeStatusSpriteGroup = 0;
}

s32 ShowTicTacToeInstructions(void)
{
    s16 animationFrame;
    s16 playerIndex;
    s16 winningCell;
    s16 winningPlayer;
    s16 winningLineCount;
    s16 effectSpriteId;
    s16 winningLineGroup;
    s16 spriteId;
    ANIMDATA *winningLineAnimation;
    s16 playerIndex;
    s16 row;
    s16 cellFlags;
    float fadeProgress;

    s16 winningCells[9];
    s16 winningCellByPlayer[2];
    s16 playerMoveCounts[2];
    s16 groupCoinTotals[2];
    s16 groupHasCoinLead[2];
    char scoreStrings[2][2];

    for (animationFrame = 0; animationFrame < 9; animationFrame++) {
        if (mgTicTacToeGrid[animationFrame % 3][animationFrame / 3] & 0x4) {
            winningCellByPlayer[0] = animationFrame;
        }
        if (mgTicTacToeGrid[animationFrame % 3][animationFrame / 3] & 0x8) {
            winningCellByPlayer[1] = animationFrame;
        }
    }
    HuPrcSleep(60);
    groupCoinTotals[0] = groupCoinTotals[1] = 0;
    for (animationFrame = 0; animationFrame < 4; animationFrame++) {
        groupCoinTotals[animationFrame / 2] += GWPlayerCoinWinGet(playerOrderByGroup[animationFrame]) + GWPlayerCoinCollectGet(playerOrderByGroup[animationFrame]);
    }
    groupHasCoinLead[0] = groupHasCoinLead[1] = 0;
    if (groupCoinTotals[0] != groupCoinTotals[1]) {
        if (groupCoinTotals[0] > groupCoinTotals[1]) {
            groupHasCoinLead[0] = 1;
        }
        else {
            groupHasCoinLead[1] = 1;
        }
    }
    for (playerIndex = 0, winningPlayer = -1; playerIndex < 2; playerIndex++) {
        playerIndex = winningCellByPlayer[playerIndex] % 3;
        row = winningCellByPlayer[playerIndex] / 3;
        if (groupHasCoinLead[playerIndex] != 0) {
            mgTicTacToeGrid[playerIndex][row] = (1 << playerIndex);
            winningPlayer = playerIndex;
        }
        else {
            mgTicTacToeGrid[playerIndex][row] &= ~(4 << playerIndex);
        }
    }
    if (ticTacToeMessageWindow == -1) {
        ticTacToeMessageWindow = CreateStandardMessageWindow(0, 0, 0);
    }
    if (winningPlayer != -1) {
        HuWinInsertMesSet(ticTacToeMessageWindow, playerCharacterIdsByGroup[winningPlayer * 2], 0);
        HuWinInsertMesSet(ticTacToeMessageWindow, playerCharacterIdsByGroup[(winningPlayer * 2) + 1], 1);
        ShowMessageAndWait(windowId, 0x280016, -1, -1);
    }
    else {
        ShowMessageAndWait(windowId, 0x280013, -1, -1);
    }
    effectSpriteId = espEntry(0x62005D, 100, 0);
    espColorSet(effectSpriteId, 255, 255, 192);
    espAttrSet(effectSpriteId, HUSPR_ATTR_ADDCOL | HUSPR_ATTR_DISPOFF | HUSPR_ATTR_LOOP);
    for (animationFrame = 0; animationFrame <= 30; animationFrame++) {
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            if (groupHasCoinLead[playerIndex]) {
                if (animationFrame == 0) {
                    winningLineAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 87) + playerIndex, HU_MEMNUM_OVL));
                    spriteId = HuSprCreate(winningLineAnimation, 170, 0);
                    HuSprGrpMemberSet(ticTacToeBoardSpriteGroup, winningCellByPlayer[playerIndex] + 3, spriteId);
                    HuSprPosSet(ticTacToeBoardSpriteGroup, winningCellByPlayer[playerIndex] + 3, ((winningCellByPlayer[playerIndex] % 3) * 56) - 56, ((winningCellByPlayer[playerIndex] / 3) * 56) - 56);
                    HuSprAttrSet(ticTacToeBoardSpriteGroup, winningCellByPlayer[playerIndex] + 3, HUSPR_ATTR_DISPOFF);
                    espAttrReset(effectSpriteId, HUSPR_ATTR_DISPOFF);
                    espPosSet(effectSpriteId, (((winningCellByPlayer[playerIndex] % 3) * 56) + 288.0f) - 56, ((winningCellByPlayer[playerIndex] / 3) * 56) + 184);
                    HuAudFXPlay(825);
                    HuAudFXPlay(826);
                }
                if (animationFrame == 8) {
                    HuSprAttrReset(ticTacToeBoardSpriteGroup, winningCellByPlayer[playerIndex] + 3, HUSPR_ATTR_DISPOFF);
                }
                if (animationFrame == 14) {
                    espKill(effectSpriteId);
                }
            }
            else {
                fadeProgress = 1 - (animationFrame / 30.0);
                HuSprTPLvlSet(ticTacToeBoardSpriteGroup, playerIndex + 14, fadeProgress);
            }
        }
        HuPrcVSleep();
    }
    HuSprAttrSet(ticTacToeBoardSpriteGroup, 14, HUSPR_ATTR_DISPOFF);
    HuSprTPLvlSet(ticTacToeBoardSpriteGroup, 14, 1.0f);
    HuSprAttrSet(ticTacToeBoardSpriteGroup, 15, HUSPR_ATTR_DISPOFF);
    HuSprTPLvlSet(ticTacToeBoardSpriteGroup, 15, 1.0f);
    for (animationFrame = 0; animationFrame < 9; animationFrame++) {
        winningCells[animationFrame] = 0;
    }
    winningLineCount = 0;
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        for (animationFrame = 0; animationFrame < 8; animationFrame++) {
            winningCell = ticTacToeWinningLines[animationFrame * 3];
            if ((1 << playerIndex) & mgTicTacToeGrid[winningCell % 3][winningCell / 3]) {
                winningCell = ticTacToeWinningLines[(animationFrame * 3) + 1];
                if ((1 << playerIndex) & mgTicTacToeGrid[winningCell % 3][winningCell / 3]) {
                    winningCell = ticTacToeWinningLines[(animationFrame * 3) + 2];
                    if ((1 << playerIndex) & mgTicTacToeGrid[winningCell % 3][winningCell / 3]) {
                        winningCell = ticTacToeWinningLines[animationFrame * 3];
                        if (winningCells[winningCell] == 0) {
                            winningCells[winningCell]++;
                            winningLineCount++;
                        }
                        winningCell = ticTacToeWinningLines[(animationFrame * 3) + 1];
                        if (winningCells[winningCell] == 0) {
                            winningCells[winningCell]++;
                            winningLineCount++;
                        }
                        winningCell = ticTacToeWinningLines[(animationFrame * 3) + 2];
                        if (winningCells[winningCell] == 0) {
                            winningCells[winningCell]++;
                            winningLineCount++;
                        }
                    }
                }
            }
        }
    }
    if (winningLineCount == 0) {
        playerMoveCounts[0] = playerMoveCounts[1] = 0;
        for (animationFrame = 0; animationFrame < 3; animationFrame++) {
            for (playerIndex = 0; playerIndex < 3; playerIndex++) {
                cellFlags = mgTicTacToeGrid[animationFrame][playerIndex];
                if (cellFlags & 0x1) {
                    playerMoveCounts[0]++;
                }
                if (cellFlags & 0x2) {
                    playerMoveCounts[1]++;
                }
            }
        }
        if (playerMoveCounts[0] + playerMoveCounts[1] >= 9) {
            for (animationFrame = 0; animationFrame < 2; animationFrame++) {
                scoreStrings[animationFrame][0] = playerMoveCounts[animationFrame] + '0';
                scoreStrings[animationFrame][1] = 0;
                HuWinInsertMesSet(ticTacToeMessageWindow, MAKE_MESSID_PTR(&scoreStrings[animationFrame][0]), (s32)animationFrame);
            }
            if (playerMoveCounts[0] > playerMoveCounts[1]) {
                winningPlayer = 0;
            }
            else {
                winningPlayer = 1;
            }
            HuPrcSleep(30);
            HuAudFXPlay(2175);
            HuWinInsertMesSet(ticTacToeMessageWindow, playerCharacterIdsByGroup[(winningPlayer * 2) + 0], 2);
            HuWinInsertMesSet(ticTacToeMessageWindow, playerCharacterIdsByGroup[(winningPlayer * 2) + 1], 3);
            ShowMessageAndWait(windowId, 0x280025, -1, -1);
            CloseStandardMessageWindow(windowId);
            ticTacToeMessageWindow = -1;
            return 1;
        }
        else {
            return 0;
        }
    }
    else {
        winningLineAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 93), HU_MEMNUM_OVL));
        winningLineGroup = HuSprGrpCreate(winningLineCount);
        for (playerIndex = animationFrame = 0; playerIndex < 9; playerIndex++) {
            if (winningCells[playerIndex] == 0) {
                continue;
            }
            spriteId = HuSprCreate(winningLineAnimation, 100, 0);
            HuSprGrpMemberSet(winningLineGroup, animationFrame, spriteId);
            HuSprPosSet(winningLineGroup, animationFrame, (((playerIndex % 3) * 56) + 288.0f) - 56, ((playerIndex / 3) * 56) + 184);
            HuSprAttrSet(winningLineGroup, animationFrame, HUSPR_ATTR_LOOP);
            animationFrame++;
        }
        HuPrcSleep(14);
        HuSprGrpKill(winningLineGroup);
        HuAudFXPlay(2175);
        HuWinInsertMesSet(ticTacToeMessageWindow, playerCharacterIdsByGroup[(winningPlayer * 2) + 0], 0);
        HuWinInsertMesSet(ticTacToeMessageWindow, playerCharacterIdsByGroup[(winningPlayer * 2) + 1], 1);
        ShowMessageAndWait(windowId, 0x280017, -1, -1);
        CloseStandardMessageWindow(windowId);
        ticTacToeMessageWindow = -1;
        return 1;
    }
}

void StartTicTacToeRound(void)
{
    HUSPRGRP *gp = &HuSprGrpData[ticTacToeBoardSpriteGroup];
    s16 j;
    s16 i;
    for (j = 0; j <= 30; j++) {
        float tplvl = 1.0 - (j / 30.0);
        for (i = 3; i < 12; i++) {
            if (gp->members[i] != -1) {
                HuSprData[gp->members[i]].a = 255.0f * tplvl;
            }
        }
        HuPrcVSleep();
    }
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            mgTicTacToeGrid[i][j] = 0;
        }
    }
}

void AnimateTicTacToeBoard(void)
{
    s16 lineIndex;
    s16 candidateCell;
    s16 comparisonPlayer;
    s16 matchingMarkCount;
    s16 adjacentCellCount;
    s16 playerIndex;
    s16 emptyCell;
    s16 ownedCellCount;
    s16 moveSearchCount;
    s16 nearbyCellSearch;
    s16 emptyCellCount;

    s16 ownedCells[9];
    s16 emptyCells[9];
    s16 adjacentEmptyCells[8];
    for (playerIndex = 0; playerIndex < 2; playerIndex++) {
        for (lineIndex = 0; lineIndex < 8; lineIndex++) {
            matchingMarkCount = 0;
            emptyCell = -1;
            candidateCell = ticTacToeWinningLines[(lineIndex * 3) + 0];
            if ((1 << playerIndex) & mgTicTacToeGrid[candidateCell % 3][candidateCell / 3]) {
                matchingMarkCount++;
            }
            else if ((mgTicTacToeGrid[candidateCell % 3][candidateCell / 3] & 0x3) == 0) {
                emptyCell = candidateCell;
            }
            candidateCell = ticTacToeWinningLines[(lineIndex * 3) + 1];
            if ((1 << playerIndex) & mgTicTacToeGrid[candidateCell % 3][candidateCell / 3]) {
                matchingMarkCount++;
            }
            else if ((mgTicTacToeGrid[candidateCell % 3][candidateCell / 3] & 0x3) == 0) {
                emptyCell = candidateCell;
            }
            candidateCell = ticTacToeWinningLines[(lineIndex * 3) + 2];
            if ((1 << playerIndex) & mgTicTacToeGrid[candidateCell % 3][candidateCell / 3]) {
                matchingMarkCount++;
            }
            else if ((mgTicTacToeGrid[candidateCell % 3][candidateCell / 3] & 0x3) == 0) {
                emptyCell = candidateCell;
            }
            if (matchingMarkCount == 2 && emptyCell != -1) {
                OSReport("%d:LEACH\n", playerIndex);
                ticTacToePreviousCursorPositions[playerIndex] = emptyCell;
                break;
            }
        }
        if (lineIndex != 8) {
            continue;
        }
        if (frandmod(5) != 0) {
            comparisonPlayer = playerIndex ^ 1;
            for (lineIndex = 0; lineIndex < 8; lineIndex++) {
                matchingMarkCount = 0;
                emptyCell = -1;
                candidateCell = ticTacToeWinningLines[(lineIndex * 3) + 0];
                if ((1 << comparisonPlayer) & mgTicTacToeGrid[candidateCell % 3][candidateCell / 3]) {
                    matchingMarkCount++;
                }
                else if ((mgTicTacToeGrid[candidateCell % 3][candidateCell / 3] & 0x3) == 0) {
                    emptyCell = candidateCell;
                }
                candidateCell = ticTacToeWinningLines[(lineIndex * 3) + 1];
                if ((1 << comparisonPlayer) & mgTicTacToeGrid[candidateCell % 3][candidateCell / 3]) {
                    matchingMarkCount++;
                }
                else if ((mgTicTacToeGrid[candidateCell % 3][candidateCell / 3] & 0x3) == 0) {
                    emptyCell = candidateCell;
                }
                candidateCell = ticTacToeWinningLines[(lineIndex * 3) + 2];
                if ((1 << comparisonPlayer) & mgTicTacToeGrid[candidateCell % 3][candidateCell / 3]) {
                    matchingMarkCount++;
                }
                else if ((mgTicTacToeGrid[candidateCell % 3][candidateCell / 3] & 0x3) == 0) {
                    emptyCell = candidateCell;
                }
                if (matchingMarkCount == 2 && emptyCell != -1) {
                    ticTacToePreviousCursorPositions[playerIndex] = emptyCell;
                    break;
                }
            }
            if (lineIndex != 8) {
                continue;
            }
        }
        for (lineIndex = ownedCellCount = emptyCellCount = 0; lineIndex < 9; lineIndex++) {
            if ((1 << playerIndex) & mgTicTacToeGrid[lineIndex % 3][lineIndex / 3]) {
                ownedCells[ownedCellCount] = lineIndex;
                ownedCellCount++;
            }
            else if ((mgTicTacToeGrid[lineIndex % 3][lineIndex / 3] & 0x3) == 0) {
                emptyCells[emptyCellCount++] = lineIndex;
            }
        }
        if (ownedCellCount == 0) {
            OSReport("%d:KOMA None\n", playerIndex);
            for (lineIndex = 0; lineIndex < 50; lineIndex++) {
                candidateCell = ticTacToeCenterCells[frandmod(8)];
                if ((mgTicTacToeGrid[candidateCell % 3][candidateCell / 3] & 0x3) == 0) {
                    ticTacToePreviousCursorPositions[playerIndex] = candidateCell;
                    break;
                }
            }
            if (lineIndex == 50) {
                ticTacToePreviousCursorPositions[playerIndex] = emptyCells[frandmod(emptyCellCount)];
            }
        }
        else {
            for (nearbyCellSearch = 0; nearbyCellSearch < 50; nearbyCellSearch++) {
                comparisonPlayer = ownedCells[frandmod(ownedCellCount)];
                adjacentCellCount = 0;
                if ((comparisonPlayer % 3) != 0 && (mgTicTacToeGrid[(comparisonPlayer - 1) % 3][comparisonPlayer / 3] & 0x3) == 0) {
                    adjacentEmptyCells[adjacentCellCount++] = comparisonPlayer - 1;
                }
                if ((comparisonPlayer % 3) != 2 && (mgTicTacToeGrid[(comparisonPlayer + 1) % 3][comparisonPlayer / 3] & 0x3) == 0) {
                    adjacentEmptyCells[adjacentCellCount++] = comparisonPlayer + 1;
                }
                if ((comparisonPlayer / 3) != 0 && (mgTicTacToeGrid[comparisonPlayer % 3][(comparisonPlayer - 3) / 3] & 0x3) == 0) {
                    adjacentEmptyCells[adjacentCellCount++] = comparisonPlayer - 3;
                }
                if ((comparisonPlayer / 3) != 2 && (mgTicTacToeGrid[comparisonPlayer % 3][(comparisonPlayer + 3) / 3] & 0x3) == 0) {
                    adjacentEmptyCells[adjacentCellCount++] = comparisonPlayer + 3;
                }
                if ((comparisonPlayer % 3) != 0 && (comparisonPlayer / 3) != 0 && (mgTicTacToeGrid[(comparisonPlayer - 4) % 3][(comparisonPlayer - 4) / 3] & 0x3) == 0) {
                    adjacentEmptyCells[adjacentCellCount++] = comparisonPlayer - 4;
                }
                if ((comparisonPlayer % 3) != 2 && (comparisonPlayer / 3) != 0 && (mgTicTacToeGrid[(comparisonPlayer - 2) % 3][(comparisonPlayer - 2) / 3] & 0x3) == 0) {
                    adjacentEmptyCells[adjacentCellCount++] = comparisonPlayer - 2;
                }
                if ((comparisonPlayer % 3) != 0 && (comparisonPlayer / 3) != 2 && (mgTicTacToeGrid[(comparisonPlayer + 2) % 3][(comparisonPlayer + 2) / 3] & 0x3) == 0) {
                    adjacentEmptyCells[adjacentCellCount++] = comparisonPlayer + 2;
                }
                if ((comparisonPlayer % 3) != 2 && (comparisonPlayer / 3) != 2 && (mgTicTacToeGrid[(comparisonPlayer + 4) % 3][(comparisonPlayer + 4) / 3] & 0x3) == 0) {
                    adjacentEmptyCells[adjacentCellCount++] = comparisonPlayer + 4;
                }
                if (adjacentCellCount != 0) {
                    moveSearchCount = adjacentEmptyCells[frandmod(adjacentCellCount)];
                    for (lineIndex = 0; lineIndex < 8; lineIndex++) {
                        matchingMarkCount = 0;
                        if (comparisonPlayer == ticTacToeWinningLines[(lineIndex * 3) + 0] || comparisonPlayer == ticTacToeWinningLines[(lineIndex * 3) + 1]
                            || comparisonPlayer == ticTacToeWinningLines[(lineIndex * 3) + 2]) {
                            matchingMarkCount++;
                        }
                        if (moveSearchCount == ticTacToeWinningLines[(lineIndex * 3) + 0] || moveSearchCount == ticTacToeWinningLines[(lineIndex * 3) + 1]
                            || moveSearchCount == ticTacToeWinningLines[(lineIndex * 3) + 2]) {
                            matchingMarkCount++;
                        }
                        if (matchingMarkCount == 2) {
                            for (matchingMarkCount = 0; matchingMarkCount < 3; matchingMarkCount++) {
                                if (comparisonPlayer != ticTacToeWinningLines[(lineIndex * 3) + matchingMarkCount] && moveSearchCount != ticTacToeWinningLines[(lineIndex * 3) + matchingMarkCount]) {
                                    break;
                                }
                            }
                            candidateCell = ticTacToeWinningLines[(lineIndex * 3) + matchingMarkCount];
                            if (mgTicTacToeGrid[candidateCell % 3][candidateCell / 3] & 0x3) {
                                candidateCell = -1;
                            }
                            break;
                        }
                    }
                    if (lineIndex == 8 || candidateCell < 0) {
                        continue;
                    }
                    ticTacToePreviousCursorPositions[playerIndex] = moveSearchCount;
                    break;
                }
            }
            if (nearbyCellSearch == 50) {
                ticTacToePreviousCursorPositions[playerIndex] = emptyCells[frandmod(emptyCellCount)];
            }
        }
    }
    OSReport("com %d,%d\n", ticTacToePreviousCursorPositions[0], ticTacToePreviousCursorPositions[1]);
}

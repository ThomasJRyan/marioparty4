#include "ext_math.h"
#include "game/audio.h"
#include "game/chrman.h"
#include "game/disp.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#include "REL/ztardll.h"
#include "game/frand.h"

#ifndef __MWERKS__
#include "game/hsfex.h"
#endif

extern s32 charVoiceGroupStat[8];

void CreateMainSelectionSprites(void);
void CreateCharacterSelectionSprites(void);
void DestroyMainSelectionSprites(void);
void RunCharacterSelectionAnimation(void);
void RunPlayerCountSelection(void);
void RunSelectionConfirmation(void);
void RunMainSelectionSequence(void);
void CleanupMainSelection(void);
void UpdateSelectionCharacterModel(void);
void SetSelectionCharacterAnimation(s16 overlayEntryPoint);
s32 GetSelectionCharacterAnimation(s32 allowRepeatedTeamSelection);
void UpdateSelectionObject(void);
void SetCharacterModelMatrix(HU3DMODEL *model, Mtx matrix);

OMOBJ *selectionObject;
HUPROCESS *selectionObjectManager;
s32 unusedSelectionScratch;
s32 selectionResultReady;
s16 currentPlayerSelection;
s16 selectionInputState;

s16 selectionMode;
s16 selectionEntryMode;
s32 bonusStarAvailable;
s16 characterNameSpriteGroup;
s16 bonusStarSpriteGroup;
s16 playerCharacterSpriteGroups[2];
s16 selectionCursorSpriteGroup;
s16 selectionTitleSpriteGroup;
s16 selectionPromptSpriteGroup;
s16 bonusStarCountSpriteGroup;
s16 selectionCharacterModel;
s16 selectionCharacterMotions[2];
s16 playerCharacterModels[4];
s16 playerCharacterMotions[4][5];
s16 characterModelMatrixHook;
HUPROCESS *characterAssetLoaderProcess;
s16 characterAssetLoadActive;
s32 selectionResultCode;

void ObjectSetup(void)
{
    s32 globalLightId;
    s32 minigameIndex;

    OSReport("******* ZP ObjectSetup *********\n");
    selectionObjectManager = omInitObjMan(0x32, 0x2000);
    selectionResultReady = 0;
    selectionEntryMode = omovlevtno;
    _ClearFlag(0x10000);
    _ClearFlag(0x10008);
    GWSystem.mg_type = -1;
    GWSystem.player_curr = 0;
    currentPlayerSelection = -1;
    mgBoardHostEnableF = 1;
    minigameIndex = omMgIndexGet(0x29);
    bonusStarAvailable = GWMGAvailGet(minigameIndex + 0x191);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 30.0f, 20.0f, 15000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    CRot.x = 0.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    Center.x = 0.0f;
    Center.y = 90.0f;
    Center.z = 30.0f;
    CZoom = 1220.0f;
    globalLightId = Hu3DGLightCreate(0.0f, 100.0f, 1000.0f, 0.0f, -0.5f, -1.0f, 0xFF, 0xFF, 0xFF);
    Hu3DGLightInfinitytSet(globalLightId);
    HuPrcChildCreate(RunMainSelectionSequence, 0x64, 0x3000, 0, selectionObjectManager);
    HuPrcChildCreate(UpdateSelectionObject, 0xC8, 0x1000, 0, selectionObjectManager);
    selectionObject = omAddObjEx(selectionObjectManager, 0x7FDA, 0, 0, -1, omOutView);
    Hu3DBGColorSet(0, 0, 0);
    HuWinInit(1);
    if (selectionEntryMode == 1) {
        HuAudSndCharGrpSet(-1);
    }
    if (selectionEntryMode != 0) {
        HuAudSndGrpSetSet(4);
        HuAudSeqPlay(0x2E);
    }
    GWSystem.mg_type = -1;
    mgPracticeEnableF = 1;
}

s16 selectionModeCursorPositions[4] = { 0x3A, 0xC8, 0x166, 0xC8 };

void RunMainSelectionSequence(void)
{
    Vec modelScreenPosition;
    Vec selectionTargetPosition;
    Vec selectionMovementVector;
    Vec selectionWorldPosition;
    float animationProgress;
    float interpolatedCursorX;
    s16 loopIndex;
    s16 selectionResult;
    s16 otherPlayerGroupIndex;
    s16 promptSlideDirection;
    s16 selectionRetry;
    s16 skipInitialTransition;

    skipInitialTransition = 0;
    selectionRetry = 0;
    StartPlayerSelectionTasks(selectionObjectManager);
    HuPrcVSleep();
    HuDataDirClose(DATADIR_MPEX);
    selectionMode = 0;
    if (mgQuitExtraF != 0) {
        mgQuitExtraF = 0;
        skipInitialTransition = 1;
        selectionEntryMode = 0;
    }
    else {
        if (selectionEntryMode == 1) {
            selectionMode = 0;
            goto block92;
        }
        if (selectionEntryMode == 2) {
            selectionMode = 1;
            goto block93;
        }
        GWPlayerCfg[0].group = GWPlayerCfg[1].group = 0;
        GWPlayerCfg[2].group = GWPlayerCfg[3].group = 1;
    }
block7:
    CreateMainSelectionSprites();
    if (bonusStarAvailable == 0) {
        HuSprTPLvlSet(playerCharacterSpriteGroups[1], 0, 0.5f);
        HuSprTPLvlSet(playerCharacterSpriteGroups[1], 2, 0.5f);
    }
    if (skipInitialTransition == 0) {
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
        HuPrcSleep(0xA);
        modelScreenPosition.x = 510.0f;
        modelScreenPosition.y = 80.0f;
        modelScreenPosition.z = 1500.0f;
        Hu3D2Dto3D(&modelScreenPosition, 1, &selectionWorldPosition);
        modelScreenPosition.x = 200.0f;
        modelScreenPosition.y = 400.0f;
        modelScreenPosition.z = 100.0f;
        Hu3D2Dto3D(&modelScreenPosition, 1, &selectionTargetPosition);
        VECSubtract(&selectionWorldPosition, &selectionTargetPosition, &selectionMovementVector);
        for (loopIndex = 0; loopIndex <= 0x1E; loopIndex++) {
            if (loopIndex <= 0x14) {
                animationProgress = loopIndex / 20.0;
                HuSprGrpPosSet(characterNameSpriteGroup, 288.0f, 80.0 - (180.0 * (1.0 - sind((90.0f * animationProgress)))));
                HuSprGrpPosSet(bonusStarSpriteGroup, 288.0f, 80.0 - (180.0 * (1.0 - sind((90.0f * animationProgress)))));
                HuSprGrpPosSet(playerCharacterSpriteGroups[0], 138.0 - (300.0 * (1.0 - sind((90.0f * animationProgress)))), 240.0f);
                HuSprGrpPosSet(playerCharacterSpriteGroups[1], 438.0 + (300.0 * (1.0 - sind((90.0f * animationProgress)))), 240.0f);
            }
            animationProgress = loopIndex / 30.0;
            VECScale(&selectionMovementVector, &modelScreenPosition, animationProgress);
            VECAdd(&modelScreenPosition, &selectionTargetPosition, &modelScreenPosition);
            Hu3DModelRotSet(selectionCharacterModel, 0.0f, -10.0f, -15.0f);
            Hu3DModelPosSetV(selectionCharacterModel, &modelScreenPosition);
            HuPrcVSleep();
        }
    }
    else {
        modelScreenPosition.x = 510.0f;
        modelScreenPosition.y = 80.0f;
        modelScreenPosition.z = 1500.0f;
        Hu3D2Dto3D(&modelScreenPosition, 1, &selectionTargetPosition);
        Hu3DModelRotSet(selectionCharacterModel, 0.0f, -10.0f, -15.0f);
        Hu3DModelPosSetV(selectionCharacterModel, &selectionTargetPosition);
        animationProgress = loopIndex / 20.0;
        HuSprGrpPosSet(characterNameSpriteGroup, 288.0f, 80.0f);
        HuSprGrpPosSet(bonusStarSpriteGroup, 288.0f, 80.0f);
        HuSprGrpPosSet(playerCharacterSpriteGroups[0], 138.0f, 240.0f);
        HuSprGrpPosSet(playerCharacterSpriteGroups[1], 438.0f, 240.0f);
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 20);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
    }
loop19:
    OpenSelectionWindow();
    SetSelectionWindowMessage(MAKE_MESSID(0x33, 0x24), 0, 0);
    if (selectionMode == 0) {
        SetSelectionWindowMessage(MAKE_MESSID(0x33, 0x25), 0, 1);
    }
    else {
        SetSelectionWindowMessage(MAKE_MESSID(0x33, 0x2A), 0, 1);
    }
    OpenMessageWindow(MAKE_MESSID(0x33, 0x8D));
    for (loopIndex = 0; loopIndex <= 0x0A; loopIndex++) {
        animationProgress = loopIndex / 10.0;

        HuSprTPLvlSet(selectionCursorSpriteGroup, 0, animationProgress);
        HuSprGrpPosSet(selectionCursorSpriteGroup, selectionModeCursorPositions[selectionMode * 2], selectionModeCursorPositions[(selectionMode * 2) + 1]);
        HuPrcVSleep();
    }
    while (TRUE) {
        HuPrcVSleep();
        if ((HuPadStkX[0] / 10) != 0) {
            otherPlayerGroupIndex = selectionMode;
            if (HuPadStkX[0] < 0 && selectionMode > 0) {
                selectionMode--;
            }
            if (HuPadStkX[0] > 0 && selectionMode < 1) {
                selectionMode++;
            }
            if (selectionMode != otherPlayerGroupIndex) {
                HuAudFXPlay(0);
                for (loopIndex = 0; loopIndex <= 0xA; loopIndex++) {
                    animationProgress = sind((90.0 * (loopIndex / 10.0)));
                    interpolatedCursorX = selectionModeCursorPositions[otherPlayerGroupIndex * 2] + (animationProgress * (selectionModeCursorPositions[selectionMode * 2] - selectionModeCursorPositions[otherPlayerGroupIndex * 2]));
                    HuSprGrpPosSet(selectionCursorSpriteGroup, interpolatedCursorX, selectionModeCursorPositions[(selectionMode * 2) + 1]);
                    HuPrcVSleep();
                }
                if (selectionMode == 0) {
                    SetSelectionWindowMessage(MAKE_MESSID(0x33, 0x25), 0, 1);
                }
                else {
                    SetSelectionWindowMessage(MAKE_MESSID(0x33, 0x2A), 0, 1);
                }
            }
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            HuAudFXPlay(3);
            CloseSelectionWindow();
            CloseMessageWindow();
            selectionResultReady = 1;
            while (TRUE) {
                HuPrcVSleep();
            }
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_A) {
            if (selectionMode != 1 || bonusStarAvailable != 0) {
                break;
            }
            HuAudFXPlay(4);
        }
    }
    HuAudFXPlay(2);
    CloseSelectionWindow();
    CloseMessageWindow();
    currentPlayerSelection = -1;
    otherPlayerGroupIndex = selectionMode ^ 1;
    for (loopIndex = 0; loopIndex <= 0x14; loopIndex++) {
        if (loopIndex <= 0xA) {
            animationProgress = loopIndex / 10.0;
            HuSprTPLvlSet(selectionCursorSpriteGroup, 0, 1.0 - animationProgress);
        }
        animationProgress = 1.0 - (loopIndex / 20.0);

        HuSprGrpScaleSet(playerCharacterSpriteGroups[otherPlayerGroupIndex], animationProgress, animationProgress);
        if (selectionMode == 0) {
            animationProgress = -animationProgress;
        }
        HuSprGrpPosSet(playerCharacterSpriteGroups[selectionMode], 288.0f + (150.0f * animationProgress), 240.0f);
        HuPrcVSleep();
    }
    for (selectionRetry = 0;;) {
        if (selectionMode == 0) {
            selectionResult = GetPlayerSelectionValue(2, selectionRetry);
        }
        else {
            selectionResult = GetPlayerSelectionValue(3, selectionRetry);
            if (selectionResult != 0) {
                mgTypeCurr = 2;
                selectionResult = GetSelectionCharacterAnimation(1);
                if (selectionResult == 0) {
                    selectionRetry = 1;
                    continue;
                }
            }
        }
        if (selectionResult == 0) {
            for (loopIndex = 0; loopIndex <= 0x14; loopIndex++) {
                animationProgress = loopIndex / 20.0;
                HuSprGrpScaleSet(playerCharacterSpriteGroups[otherPlayerGroupIndex], animationProgress, animationProgress);
                if (selectionMode != 0) {
                    animationProgress = -animationProgress;
                }
                HuSprGrpPosSet(playerCharacterSpriteGroups[selectionMode], 288.0f - (150.0f * animationProgress), 240.0f);
                animationProgress = loopIndex / 20.0;
                if (selectionMode == 0) {
                    promptSlideDirection = -1;
                }
                else {
                    promptSlideDirection = 1;
                }
                HuSprGrpPosSet(selectionPromptSpriteGroup, 288.0 + (promptSlideDirection * (400.0 * (1.0 - cosd((90.0f * animationProgress))))), 340.0f);
                HuPrcVSleep();
            }
            goto loop19;
        }
        selectionRetry = 1;
        DestroyMainSelectionSprites();
        for (loopIndex = 0; loopIndex <= 0x14; loopIndex++) {
            animationProgress = loopIndex / 20.0;
            if (selectionMode == 0) {
                promptSlideDirection = -1;
            }
            else {
                promptSlideDirection = 1;
            }
            HuSprGrpPosSet(selectionPromptSpriteGroup, 288.0 + (promptSlideDirection * (400.0 * (1.0 - sind((90.0f * animationProgress))))), 340.0f);
            HuPrcVSleep();
        }
        if (selectionMode == 0) {
            RunSelectionConfirmation();
        }
        if (characterAssetLoadActive == 0) {
            characterAssetLoadActive = 1;
            HuPrcChildCreate(LoadSelectedCharacterAssets, 0x64, 0x2000, 0, selectionObjectManager);
        }
        OpenSelectionWindow();
        SetSelectionWindowMessage(MAKE_MESSID(0x33, 0x2C), 0, 0);
        selectionResult = ShowSelectionConfirmation(0);
        if (selectionResult != -1) {
            if (selectionResult != 1) {
                break;
            }
        }
        CloseSelectionWindow();
        for (loopIndex = 0; loopIndex <= 0xA; loopIndex++) {
            animationProgress = cosd((90.0 * (loopIndex / 10.0)));
            HuSprGrpScaleSet(selectionPromptSpriteGroup, 1.0f, animationProgress);
            HuPrcVSleep();
        }
        HuSprGrpKill(selectionPromptSpriteGroup);
    }
    CloseSelectionWindow();
    CloseMessageWindow();
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 20);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    mgBattleStarMax = 0;
    HuSprGrpKill(selectionTitleSpriteGroup);
    HuSprGrpKill(characterNameSpriteGroup);
    HuSprGrpKill(bonusStarSpriteGroup);
    HuSprGrpKill(playerCharacterSpriteGroups[0]);
    HuSprGrpKill(playerCharacterSpriteGroups[1]);
    HuSprGrpKill(selectionCursorSpriteGroup);
    HuSprGrpKill(selectionPromptSpriteGroup);
    Hu3DModelKill(selectionCharacterModel);
    HuPrcKill(characterAssetLoaderProcess);
    while (characterAssetLoadActive != 0) {
        HuPrcVSleep();
    }
    if (selectionMode == 0) {
    block92:
        RunCharacterSelectionAnimation();
    }
    else {
    block93:
        CleanupMainSelection();
    }
    skipInitialTransition = 1;
    goto block7;
}

void CreateMainSelectionSprites(void)
{
    SeqWork sequenceWork;
    s16 spriteId;
    s16 spriteIndex;
    char *messageTextEnd;
    s16 playerGroupSpriteGroup;
    ANIMDATA *characterNameAnimation;
    char *messageTextStart;
    ANIMDATA *playerIconAnimation;
    ANIMDATA *starCountAnimation;
    ANIMDATA *alternateStarAnimation;

    characterNameAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 3));
    selectionTitleSpriteGroup = HuSprGrpCreate(1);
    spriteId = HuSprCreate(characterNameAnimation, 0xC8, 0);
    HuSprGrpMemberSet(selectionTitleSpriteGroup, 0, spriteId);
    HuSprGrpDrawNoSet(selectionTitleSpriteGroup, 0x7F);
    HuSprGrpPosSet(selectionTitleSpriteGroup, 288.0f, 240.0f);
    for (spriteIndex = 0; spriteIndex < 0x10; spriteIndex++) {
        sequenceWork.sprite[spriteIndex] = sequenceWork.spr_grp[spriteIndex] = -1;
    }
    messageTextEnd = MessData_MesPtrGet(messDataPtr, MAKE_MESSID(0x17, 0x21));
    for (messageTextStart = messageTextEnd; *messageTextEnd != 0; messageTextEnd++) {
        if (*messageTextEnd == 0xA) {
            *messageTextEnd = 0x10;
        }
    }
    characterNameSpriteGroup = sequenceWork.spr_grp[CreateCharacterNameSprites(&sequenceWork, messageTextStart)];
    HuSprGrpPosSet(characterNameSpriteGroup, 288.0f, -300.0f);
    HuSprGrpDrawNoSet(characterNameSpriteGroup, 0x7F);
    characterNameAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 4));
    bonusStarSpriteGroup = HuSprGrpCreate(1);
    spriteId = HuSprCreate(characterNameAnimation, 0x64, 0);
    HuSprGrpMemberSet(bonusStarSpriteGroup, 0, spriteId);
    HuSprGrpPosSet(bonusStarSpriteGroup, 288.0f, -300.0f);
    HuSprGrpDrawNoSet(bonusStarSpriteGroup, 0x7F);
    characterNameAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 5));
    playerIconAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 14));
    starCountAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 64));
    alternateStarAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 15));
    for (spriteIndex = 0; spriteIndex < 2; spriteIndex++) {
        playerGroupSpriteGroup = HuSprGrpCreate(4);
        playerCharacterSpriteGroups[spriteIndex] = playerGroupSpriteGroup;
        spriteId = HuSprCreate(characterNameAnimation, 0x64, spriteIndex);
        HuSprGrpMemberSet(playerGroupSpriteGroup, 0, spriteId);
        spriteId = HuSprCreate(playerIconAnimation, 0x6E, 0);
        HuSprGrpMemberSet(playerGroupSpriteGroup, 1, spriteId);
        spriteId = HuSprCreate(starCountAnimation, 0x78, 0);
        HuSprGrpMemberSet(playerGroupSpriteGroup, 2, spriteId);
        HuSprScaleSet(playerGroupSpriteGroup, 2, 0.5f, 0.5f);
        spriteId = HuSprCreate(alternateStarAnimation, 0x82, 0);
        HuSprGrpMemberSet(playerGroupSpriteGroup, 3, spriteId);
        HuSprGrpPosSet(playerGroupSpriteGroup, 288.0f, -300.0f);
        HuSprGrpDrawNoSet(playerGroupSpriteGroup, 0x7F);
    }
    characterNameAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 6));
    selectionCursorSpriteGroup = HuSprGrpCreate(1);
    spriteId = HuSprCreate(characterNameAnimation, 0x32, 0);
    HuSprGrpMemberSet(selectionCursorSpriteGroup, 0, spriteId);
    HuSprGrpPosSet(selectionCursorSpriteGroup, 288.0f, -300.0f);
    selectionCharacterModel = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_ZTAR, 0));
    selectionCharacterMotions[0] = Hu3DJointMotionFile(selectionCharacterModel, DATA_MAKE_NUM(DATADIR_ZTAR, 1));
    selectionCharacterMotions[1] = Hu3DJointMotionFile(selectionCharacterModel, DATA_MAKE_NUM(DATADIR_ZTAR, 2));
    Hu3DMotionSet(selectionCharacterModel, selectionCharacterMotions[0]);
    Hu3DModelAttrSet(selectionCharacterModel, HU3D_MOTATTR_LOOP);
    Hu3DModelPosSet(selectionCharacterModel, 0.0f, 1000.0f, 0.0f);
    characterAssetLoaderProcess = HuPrcChildCreate(CreateCharacterSelectionSprites, 0x64, 0x2000, 0, selectionObjectManager);
}

void CreateCharacterSelectionSprites(void)
{
    float modelBobPhase = 0.0f;
    HU3DMODEL *selectionModel = &Hu3DData[selectionCharacterModel];

    while (TRUE) {
        selectionModel->pos.y += sind(modelBobPhase);
        modelBobPhase += 3.0f;
        if (modelBobPhase > 360.0f) {
            modelBobPhase -= 360.0f;
        }
        HuPrcVSleep();
    }
}

void DestroyMainSelectionSprites(void)
{
    s16 playerIndicesByTeam[4];
    s16 teamOffsets[2];
    s16 playerIndex;
    ANIMDATA *spriteAnimation;
    s16 spriteId;
    s16 spriteGroupId;
    s16 characterIconX;

    teamOffsets[0] = 0;
    teamOffsets[1] = 2;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerIndicesByTeam[teamOffsets[GWPlayerCfg[playerIndex].group]++] = playerIndex;
    }
    if (selectionMode == 0) {
        spriteGroupId = HuSprGrpCreate(5);
        selectionPromptSpriteGroup = spriteGroupId;
        spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 17));
        spriteId = HuSprCreate(spriteAnimation, 0x78, 0);
        HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
        for (playerIndex = 0; playerIndex < 2; playerIndex++) {
            spriteAnimation = HuSprAnimReadFile(GWPlayerCfg[playerIndicesByTeam[playerIndex]].character + DATA_MAKE_NUM(DATADIR_ZTAR, 18));
            spriteId = HuSprCreate(spriteAnimation, 0x6E, 0);
            HuSprGrpMemberSet(spriteGroupId, (playerIndex * 2) + 1, spriteId);
            HuSprPosSet(spriteGroupId, (playerIndex * 2) + 1, (playerIndex * 0x30) - 0x18, -4.0f);
            if (GWPlayerCfg[playerIndicesByTeam[playerIndex]].iscom == 0) {
                spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 32));
                spriteId = HuSprCreate(spriteAnimation, 0x64, playerIndicesByTeam[playerIndex]);
            }
            else {
                spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 33));
                spriteId = HuSprCreate(spriteAnimation, 0x64, GWPlayerCfg[playerIndicesByTeam[playerIndex]].diff);
            }
            HuSprGrpMemberSet(spriteGroupId, (playerIndex * 2) + 2, spriteId);
            HuSprPosSet(spriteGroupId, (playerIndex * 2) + 2, (playerIndex * 0x30) - 0x18, 20.0f);
        }
    }
    else {
        spriteGroupId = HuSprGrpCreate(0xA);
        selectionPromptSpriteGroup = spriteGroupId;
        spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 16));
        spriteId = HuSprCreate(spriteAnimation, 0x78, 0);
        HuSprGrpMemberSet(spriteGroupId, 0, spriteId);
        characterIconX = -0x4B;
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            spriteAnimation = HuSprAnimReadFile(GWPlayerCfg[playerIndicesByTeam[playerIndex]].character + DATA_MAKE_NUM(DATADIR_ZTAR, 18));
            spriteId = HuSprCreate(spriteAnimation, 0x6E, 0);
            HuSprGrpMemberSet(spriteGroupId, (playerIndex * 2) + 1, spriteId);
            HuSprPosSet(spriteGroupId, (playerIndex * 2) + 1, characterIconX, 0.0f);
            if (GWPlayerCfg[playerIndicesByTeam[playerIndex]].iscom == 0) {
                spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 32));
                spriteId = HuSprCreate(spriteAnimation, 0x64, playerIndicesByTeam[playerIndex]);
            }
            else {
                spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 33));
                spriteId = HuSprCreate(spriteAnimation, 0x64, GWPlayerCfg[playerIndicesByTeam[playerIndex]].diff);
            }
            HuSprGrpMemberSet(spriteGroupId, (playerIndex * 2) + 2, spriteId);
            HuSprPosSet(spriteGroupId, (playerIndex * 2) + 2, characterIconX, 22.0f);
            characterIconX += 0x2A;
            if (playerIndex == 1) {
                characterIconX += 0x18;
            }
        }
        spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 10));
        spriteId = HuSprCreate(spriteAnimation, 0x5A, 0);
        HuSprGrpMemberSet(spriteGroupId, 9, spriteId);
    }
    HuSprGrpPosSet(spriteGroupId, 0.0f, -100.0f);
}

void RunCharacterSelectionAnimation(void)
{
    s16 playerIndicesByTeam[4];
    s16 teamOffsets[2];
    float animationProgress;
    s16 playerIndex;

    teamOffsets[0] = 0;
    teamOffsets[1] = 2;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerIndicesByTeam[teamOffsets[GWPlayerCfg[playerIndex].group]++] = playerIndex;
    }
    if (selectionEntryMode != 0) {
        if (GWPlayerCoinWinGet(playerIndicesByTeam[0]) > 0) {
            mgBattleStarMax++;
        }
    }
    RunPlayerCountSelection();
    Hu3DModelPosSet(playerCharacterModels[2], 500.0f, 0.0f, 0.0f);
    Hu3DModelPosSet(playerCharacterModels[3], 500.0f, 0.0f, 0.0f);
    HuSprScaleSet(bonusStarSpriteGroup, 2, 0.0f, 0.0f);
    if (selectionEntryMode != 0) {
        if (GWPlayerCoinWinGet(playerIndicesByTeam[0]) == 0) {
            mgBattleStarMax++;
        }
        HuSprBankSet(bonusStarSpriteGroup, 1, mgBattleStarMax);
        HuSprScaleSet(bonusStarSpriteGroup, 1, 1.0f, 1.0f);
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 20);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        HuPrcSleep(0x1E);
        if (GWPlayerCoinWinGet(playerIndicesByTeam[0]) > 0) {
            HuSprAttrReset(bonusStarCountSpriteGroup, 6, 4);
            HuSprPosSet(bonusStarCountSpriteGroup, 6, ((mgBattleStarMax - 1) << 5) + 0x46, 340.0f);
            for (playerIndex = 0; playerIndex <= 0x3C; playerIndex++) {
                animationProgress = playerIndex / 60.0;
                HuSprTPLvlSet(bonusStarCountSpriteGroup, 6, animationProgress);
                animationProgress = 1.0 + (5.0 * cosd((90.0f * animationProgress)));
                HuSprScaleSet(bonusStarCountSpriteGroup, 6, animationProgress, animationProgress);
                if (playerIndex == 0x14) {
                    HuAudFXPlay(0x9A);
                    if (mgBattleStarMax < 6) {
                        Hu3DMotionShiftSet(playerCharacterModels[0], playerCharacterMotions[0][2], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
                        Hu3DMotionShiftSet(playerCharacterModels[1], playerCharacterMotions[1][2], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
                    }
                    else {
                        Hu3DMotionShiftSet(playerCharacterModels[0], playerCharacterMotions[0][3], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
                        Hu3DMotionShiftSet(playerCharacterModels[1], playerCharacterMotions[1][3], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
                    }
                }
                HuPrcVSleep();
            }
            HuAudFXPlay(8);
            if (mgBattleStarMax >= 6) {
                HuPrcSleep(0x3C);
                playerIndex = omMgIndexGet(0x29);
                bonusStarAvailable = 1;
                OpenSelectionWindow();
                InsertSelectionWindowMessage(GWPlayerCfg[playerIndicesByTeam[0]].character, 0);
                InsertSelectionWindowMessage(GWPlayerCfg[playerIndicesByTeam[1]].character, 1);
                SetSelectionWindowMessage(MAKE_MESSID(0x33, 0xA0), 0, 0);
                if (GWMGAvailGet(playerIndex + 0x191) == 0) {
                    GWMGAvailSet(playerIndex + 0x191);
                    GWGameStat.present[0x37] = 1;
                    SetSelectionWindowMessage(MAKE_MESSID(0x33, 0x29), 0, 0);
                }
                CloseSelectionWindow();
                WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 30);
                while (WipeStatGet() != 0) {
                    HuPrcVSleep();
                }
            }
            else {
                Hu3DMotionShiftSet(playerCharacterModels[0], playerCharacterMotions[0][0], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
                Hu3DMotionShiftSet(playerCharacterModels[1], playerCharacterMotions[1][0], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
                for (playerIndex = 0; playerIndex <= 0xA; playerIndex++) {
                    animationProgress = 1.0 - (playerIndex / 10.0);
                    HuSprScaleSet(bonusStarSpriteGroup, 1, animationProgress, animationProgress);
                    HuPrcVSleep();
                }
                HuPrcSleep(0x3C);
                goto block39;
            }
        }
        else {
            Hu3DMotionShiftSet(playerCharacterModels[0], playerCharacterMotions[0][4], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            Hu3DMotionShiftSet(playerCharacterModels[1], playerCharacterMotions[1][4], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            HuPrcSleep(0xB4);
            WipeColorSet(0, 0, 0);
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 30);
            while (WipeStatGet() != 0) {
                HuPrcVSleep();
            }
        }
    }
    else {
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 20);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        HuPrcSleep(0x1E);
    block39:
        Hu3DMotionSet(playerCharacterModels[2], playerCharacterMotions[2][1]);
        Hu3DMotionSet(playerCharacterModels[3], playerCharacterMotions[3][1]);
        Hu3DModelRotSet(playerCharacterModels[2], 0.0f, -90.0f, 0.0f);
        Hu3DModelRotSet(playerCharacterModels[3], 0.0f, -90.0f, 0.0f);
        HuSprBankSet(bonusStarSpriteGroup, 1, mgBattleStarMax + 1);
        for (playerIndex = 0; playerIndex <= 0x1E; playerIndex++) {
            animationProgress = 1.0 - sind((90.0 * (playerIndex / 30.0)));
            Hu3DModelPosSet(playerCharacterModels[2], 125.0f + (400.0f * animationProgress), 0.0f, 0.0f);
            Hu3DModelPosSet(playerCharacterModels[3], 275.0f + (400.0f * animationProgress), 0.0f, 0.0f);
            if (playerIndex == 0x19) {
                Hu3DMotionShiftSet(playerCharacterModels[2], playerCharacterMotions[2][0], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
                Hu3DMotionShiftSet(playerCharacterModels[3], playerCharacterMotions[3][0], 0.0f, 10.0f, HU3D_MOTATTR_LOOP);
            }
            if ((GWPlayerCfg[2].character < 8) && ((playerIndex % 5) == 0)) {
                HuAudFXPlay(GWPlayerCfg[2].character + 0xD5);
            }
            if ((GWPlayerCfg[3].character < 8) && (((playerIndex + 3) % 5) == 0)) {
                HuAudFXPlay(GWPlayerCfg[3].character + 0xD5);
            }
            if (playerIndex > 0x14) {
                animationProgress = 1.0 - ((playerIndex - 0x14) / 10.0);
                Hu3DModelRotSet(playerCharacterModels[2], 0.0f, 90.0f * -animationProgress, 0.0f);
                Hu3DModelRotSet(playerCharacterModels[3], 0.0f, 90.0f * -animationProgress, 0.0f);
            }
            if (playerIndex <= 0x14) {
                animationProgress = 1.2 * sind((90.0 * (playerIndex / 20.0)));
                HuSprScaleSet(bonusStarSpriteGroup, 1, animationProgress, animationProgress);
            }
            else if (playerIndex > 0x14) {
                animationProgress = 1.0 + (0.2 * cosd((90.0 * ((playerIndex - 0x14) / 10.0))));
                HuSprScaleSet(bonusStarSpriteGroup, 1, animationProgress, animationProgress);
            }
            HuPrcVSleep();
        }
        HuAudFXPlay(0x355);
        for (playerIndex = 0; playerIndex <= 0x14; playerIndex++) {
            animationProgress = 1.0 + (5.0 * cosd((90.0 * (playerIndex / 20.0))));
            HuSprScaleSet(bonusStarSpriteGroup, 2, animationProgress, animationProgress);
            HuSprTPLvlSet(bonusStarSpriteGroup, 2, playerIndex / 20.0);
            HuPrcVSleep();
        }
        OpenMessageWindowCentered(MAKE_MESSID(0x33, 0x4F), 1);
        while (TRUE) {
            if ((*HuPadBtnDown & 0x100) != 0) {
                HuAudFXPlay(2);
                if (mgBattleStarMax >= 5) {
                    GWSystem.mg_type = 0;
                }
                SetSelectionCharacterAnimation(1);
            }
            if ((*HuPadBtnDown & 0x200) != 0) {
                break;
            }
            HuPrcVSleep();
        }
        HuAudFXPlay(3);
        CloseMessageWindow();
        WipeColorSet(0, 0, 0);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 20);
        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
    }
    HuSprGrpKill(selectionTitleSpriteGroup);
    HuSprGrpKill(bonusStarSpriteGroup);
    HuSprGrpKill(bonusStarCountSpriteGroup);
    Hu3DModelKill(selectionCharacterModel);
    HuPrcKill(characterAssetLoaderProcess);
    Hu3DModelKill(characterModelMatrixHook);
    CharModelKill(-1);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (GWPlayerCfg[playerIndicesByTeam[playerIndex]].character >= 8) {
            Hu3DModelKill(playerCharacterModels[playerIndex]);
            Hu3DMotionKill(playerCharacterMotions[playerIndex][0]);
            Hu3DMotionKill(playerCharacterMotions[playerIndex][1]);
        }
    }
    selectionEntryMode = 0;
    selectionMode = 0;
}

s32 characterModelDataFiles[6] = {
    DATA_MAKE_NUM(DATADIR_ZTAR, 59),
    DATA_MAKE_NUM(DATADIR_ZTAR, 34),
    DATA_MAKE_NUM(DATADIR_ZTAR, 39),
    DATA_MAKE_NUM(DATADIR_ZTAR, 44),
    DATA_MAKE_NUM(DATADIR_ZTAR, 49),
    DATA_MAKE_NUM(DATADIR_ZTAR, 54),
};

u8 characterOrderOffsets[6] = { 0, 1, 1, 2, 2, 3 };

void RunPlayerCountSelection(void)
{
    Vec screenStartPosition;
    Vec modelPosition;
    Vec shadowOrigin;
    Vec shadowDirection;
    s16 playerIndicesByTeam[4];
    s16 teamOffsets[2];
    s16 playerIndex;
    s16 spriteMemberIndex;
    ANIMDATA *starAnimation;
    s16 characterId;
    s16 starCountSpriteGroup;
    s16 modelPositionX;
    s16 characterModelIndex;
    s16 remainingBonusStars;
    ANIMDATA *emptyStarAnimation;

    teamOffsets[0] = 0;
    teamOffsets[1] = 2;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerIndicesByTeam[teamOffsets[GWPlayerCfg[playerIndex].group]++] = playerIndex;
    }
    starAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 3));
    selectionTitleSpriteGroup = HuSprGrpCreate(1);
    spriteMemberIndex = HuSprCreate(starAnimation, 0xC8, 0);
    HuSprGrpMemberSet(selectionTitleSpriteGroup, 0, spriteMemberIndex);
    HuSprGrpDrawNoSet(selectionTitleSpriteGroup, 0x7F);
    HuSprGrpPosSet(selectionTitleSpriteGroup, 288.0f, 240.0f);
    bonusStarSpriteGroup = HuSprGrpCreate(3);
    starAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 12));
    spriteMemberIndex = HuSprCreate(starAnimation, 0x78, 0);
    HuSprGrpMemberSet(bonusStarSpriteGroup, 0, spriteMemberIndex);
    HuSprPosSet(bonusStarSpriteGroup, 0, 288.0f, 80.0f);
    starAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 11));
    if ((mgBattleStarMax + 1) >= 7) {
        spriteMemberIndex = HuSprCreate(starAnimation, 0x64, 6);
    }
    else {
        spriteMemberIndex = HuSprCreate(starAnimation, 0x64, mgBattleStarMax + 1);
    }
    HuSprGrpMemberSet(bonusStarSpriteGroup, 1, spriteMemberIndex);
    HuSprPosSet(bonusStarSpriteGroup, 1, VERSION_JP ? 234.0f : 342.0f, 80.0f);
    HuSprScaleSet(bonusStarSpriteGroup, 1, 0.0f, 0.0f);
    HuSprGrpDrawNoSet(bonusStarSpriteGroup, 0x7F);
    starAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 9));
    spriteMemberIndex = HuSprCreate(starAnimation, 0x64, 0);
    HuSprGrpMemberSet(bonusStarSpriteGroup, 2, spriteMemberIndex);
    HuSprPosSet(bonusStarSpriteGroup, 2, 288.0f, 240.0f);
    HuSprDrawNoSet(bonusStarSpriteGroup, 2, 0);
    HuSprGrpPosSet(bonusStarSpriteGroup, 0.0f, 0.0f);
    starCountSpriteGroup = HuSprGrpCreate(7);
    starCountSpriteGroup = starCountSpriteGroup;
    starAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 8));
    emptyStarAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 7));
    remainingBonusStars = mgBattleStarMax;
    if (GWPlayerCoinWinGet(playerIndicesByTeam[0]) > 0) {
        remainingBonusStars--;
    }
    for (playerIndex = 0; playerIndex < 6; playerIndex++) {
        if (playerIndex >= remainingBonusStars) {
            spriteMemberIndex = HuSprCreate(emptyStarAnimation, 0x64, 0);
        }
        else {
            spriteMemberIndex = HuSprCreate(starAnimation, 0x64, 0);
        }
        HuSprGrpMemberSet(starCountSpriteGroup, playerIndex, spriteMemberIndex);
        HuSprPosSet(starCountSpriteGroup, playerIndex, (playerIndex << 5) + 0x46, 340.0f);
    }
    spriteMemberIndex = HuSprCreate(starAnimation, 0x5A, 0);
    HuSprGrpMemberSet(starCountSpriteGroup, 6, spriteMemberIndex);
    HuSprAttrSet(starCountSpriteGroup, 6, 4);
    HuSprGrpPosSet(starCountSpriteGroup, 0.0f, 0.0f);
    screenStartPosition.x = 510.0f;
    screenStartPosition.y = 80.0f;
    screenStartPosition.z = 1500.0f;
    Hu3D2Dto3D(&screenStartPosition, 1, &modelPosition);
    selectionCharacterModel = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_ZTAR, 0));
    selectionCharacterMotions[0] = Hu3DJointMotionFile(selectionCharacterModel, DATA_MAKE_NUM(DATADIR_ZTAR, 1));
    selectionCharacterMotions[1] = Hu3DJointMotionFile(selectionCharacterModel, DATA_MAKE_NUM(DATADIR_ZTAR, 2));
    Hu3DModelAttrSet(selectionCharacterModel, HU3D_MOTATTR_LOOP);
    Hu3DModelPosSetV(selectionCharacterModel, &modelPosition);
    Hu3DModelRotSet(selectionCharacterModel, 0.0f, -10.0f, -15.0f);
    Hu3DMotionSet(selectionCharacterModel, selectionCharacterMotions[0]);
    characterAssetLoaderProcess = HuPrcChildCreate(CreateCharacterSelectionSprites, 0x64, 0x2000, 0, selectionObjectManager);
    GWPlayerCfg[playerIndicesByTeam[2]].character = mgIndexList[mgBattleStarMax * 2];
    GWPlayerCfg[playerIndicesByTeam[3]].character = mgIndexList[mgBattleStarMax * 2 + 1];
    GWPlayerCfg[playerIndicesByTeam[0]].group = GWPlayerCfg[playerIndicesByTeam[1]].group = 0;
    GWPlayerCfg[playerIndicesByTeam[2]].group = GWPlayerCfg[playerIndicesByTeam[3]].group = 1;
    GWPlayerCfg[playerIndicesByTeam[2]].iscom = GWPlayerCfg[playerIndicesByTeam[3]].iscom = 1;
    GWPlayerCfg[playerIndicesByTeam[2]].diff = GWPlayerCfg[playerIndicesByTeam[3]].diff = characterOrderOffsets[mgBattleStarMax];
    modelPositionX = -0x113;
    if (selectionEntryMode != 0) {
        CharDataClose(-1);
        for (playerIndex = 0; playerIndex < 4; playerIndex++) {
            if (GWPlayerCfg[playerIndicesByTeam[playerIndex]].character < 8) {
                CharMotionInit(GWPlayerCfg[playerIndicesByTeam[playerIndex]].character);
            }
        }
    }
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        characterId = GWPlayerCfg[playerIndicesByTeam[playerIndex]].character;
        if (characterId != 0xFF) {
            if (characterId < 8) {
                playerCharacterModels[playerIndex] = CharModelCreate(characterId, 2);
                playerCharacterMotions[playerIndex][0] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00));
                playerCharacterMotions[playerIndex][1] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03));
                if (playerIndex < 2) {
                    playerCharacterMotions[playerIndex][2] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x48));
                    playerCharacterMotions[playerIndex][3] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17));
                    playerCharacterMotions[playerIndex][4] = CharMotionCreate(characterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18));
                }
                CharMotionVoiceOnSet(GWPlayerCfg[playerIndicesByTeam[playerIndex]].character, playerCharacterMotions[playerIndex][1], 0);
                Hu3DModelScaleSet(playerCharacterModels[playerIndex], 0.8f, 0.8f, 0.8f);
            }
            else {
                characterModelIndex = characterId - 8;
                playerCharacterModels[playerIndex] = Hu3DModelCreateFile(characterModelDataFiles[characterModelIndex]);
                playerCharacterMotions[playerIndex][0] = Hu3DJointMotionFile(playerCharacterModels[playerIndex], characterModelDataFiles[characterModelIndex] + 1);
                playerCharacterMotions[playerIndex][1] = Hu3DJointMotionFile(playerCharacterModels[playerIndex], characterModelDataFiles[characterModelIndex] + 2);
                Hu3DModelScaleSet(playerCharacterModels[playerIndex], 0.8f, 0.8f, 0.8f);
            }
            Hu3DMotionSet(playerCharacterModels[playerIndex], playerCharacterMotions[playerIndex][0]);
            Hu3DModelAttrSet(playerCharacterModels[playerIndex], HU3D_MOTATTR_LOOP);
            Hu3DModelPosSet(playerCharacterModels[playerIndex], modelPositionX, 0.0f, 0.0f);
            Hu3DModelShadowSet(playerCharacterModels[playerIndex]);
            modelPositionX += 0x96;
            if (playerIndex == 1) {
                modelPositionX += 0x64;
            }
        }
    }
    CharModelDataClose(-1);
    Hu3DShadowCreate(20.0f, 500.0f, 8000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowSizeSet(0xC0);
    shadowOrigin.x = shadowOrigin.y = shadowOrigin.z = 0.0f;
    screenStartPosition.x = screenStartPosition.z = 100.0f;
    screenStartPosition.y = 2000.0f;
    shadowDirection.x = shadowDirection.y = 0.0f;
    shadowDirection.z = 1.0f;
    Hu3DShadowPosSet(&screenStartPosition, &shadowDirection, &shadowOrigin);
    characterModelMatrixHook = Hu3DHookFuncCreate(SetCharacterModelMatrix);
    Hu3DModelLayerSet(characterModelMatrixHook, 1);
}

void RunSelectionConfirmation(void)
{
    s16 availableCharacters[4];
    s16 playerIndicesByTeam[4];
    s16 teamOffsets[2];
    s16 candidateCharacterId;
    s16 availableCharacterCount;
    s16 swapCharacterIndex;
    s16 temporaryCharacterId;

    teamOffsets[0] = 0;
    teamOffsets[1] = 2;
    for (candidateCharacterId = 0; candidateCharacterId < 4; candidateCharacterId++) {
        playerIndicesByTeam[teamOffsets[GWPlayerCfg[candidateCharacterId].group]++] = candidateCharacterId;
    }
    availableCharacterCount = 0;
    for (candidateCharacterId = availableCharacterCount; candidateCharacterId < 8; candidateCharacterId++) {
        if ((candidateCharacterId != GWPlayerCfg[playerIndicesByTeam[0]].character) && (candidateCharacterId != GWPlayerCfg[playerIndicesByTeam[1]].character)) {
            availableCharacters[availableCharacterCount++] = candidateCharacterId;
        }
    }
    for (candidateCharacterId = 0; candidateCharacterId < 0x1E; candidateCharacterId++) {
        availableCharacterCount = frandmod(6);
        swapCharacterIndex = frandmod(6);
        temporaryCharacterId = availableCharacters[availableCharacterCount];
        availableCharacters[availableCharacterCount] = availableCharacters[swapCharacterIndex];
        availableCharacters[swapCharacterIndex] = temporaryCharacterId;
    }
    for (candidateCharacterId = 0; candidateCharacterId < 6; candidateCharacterId++) {
        mgIndexList[candidateCharacterId] = availableCharacters[candidateCharacterId];
    }
    mgIndexList[6] = 0xC;
    mgIndexList[7] = 9;
    mgIndexList[8] = 0xB;
    mgIndexList[9] = 0xA;
    mgIndexList[0xA] = 8;
    mgIndexList[0xB] = 0xD;
    mgIndexList[0xC] = mgIndexList[0xD] = 0xFF;
    GWPlayerCfg[playerIndicesByTeam[2]].character = mgIndexList[0];
    GWPlayerCfg[playerIndicesByTeam[3]].character = mgIndexList[1];
}

s16 characterSelectionSoundIds[6] = { 0x3A, 0x36, 0x40, 0x4B, 0x46, 0x3E };
s16 alternateCharacterSelectionSoundIds[7] = { 0x3B, 0x38, 0x42, 0x4D, 0x48, 0x3F, 0 };

void CleanupMainSelection(void)
{
    s16 playerIndicesByTeam[4];
    s16 teamOffsets[2];
    float animationProgress;
    s16 playerIndex;
    s16 winningPlayerCount;

    teamOffsets[0] = 0;
    teamOffsets[1] = 2;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerIndicesByTeam[teamOffsets[GWPlayerCfg[playerIndex].group]++] = playerIndex;
    }
    UpdateSelectionCharacterModel();
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 20);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    HuPrcSleep(0x1E);
    if (selectionEntryMode != 0) {
        winningPlayerCount = 0;
        for (playerIndex = winningPlayerCount; playerIndex < 4; playerIndex++) {
            if (GWPlayerCoinWinGet(playerIndicesByTeam[playerIndex]) > 0) {
                if (winningPlayerCount == 0) {
                    HuAudFXPlay(0x9A);
                }
                if (GWPlayerCfg[playerIndicesByTeam[playerIndex]].character >= 8) {
                    HuAudFXPlay(characterSelectionSoundIds[GWPlayerCfg[playerIndicesByTeam[playerIndex]].character - 8]);
                }
                Hu3DMotionShiftSet(playerCharacterModels[playerIndex], playerCharacterMotions[playerIndex][3], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
                winningPlayerCount++;
            }
            else {
                if (GWPlayerCfg[playerIndicesByTeam[playerIndex]].character >= 8) {
                    HuAudFXPlay(alternateCharacterSelectionSoundIds[GWPlayerCfg[playerIndicesByTeam[playerIndex]].character - 8]);
                }
                Hu3DMotionShiftSet(playerCharacterModels[playerIndex], playerCharacterMotions[playerIndex][4], 0.0f, 10.0f, HU3D_MOTATTR_NONE);
            }
        }
        HuSprAttrReset(bonusStarSpriteGroup, 1, 4);
        HuPrcSleep(0x78);
    }
    else {
        HuAudFXPlay(0x355);
        HuSprAttrReset(bonusStarSpriteGroup, 1, 4);
        for (playerIndex = 0; playerIndex <= 0x14; playerIndex++) {
            animationProgress = 1.0 + (5.0 * cosd((90.0 * (playerIndex / 20.0))));

            HuSprScaleSet(bonusStarSpriteGroup, 1, animationProgress, animationProgress);
            HuSprTPLvlSet(bonusStarSpriteGroup, 1, playerIndex / 20.0);
            HuPrcVSleep();
        }
    }
    OpenMessageWindowCentered(MAKE_MESSID(0x33, 0x4F), 1);
    while (TRUE) {
        if (HuPadBtnDown[0] & PAD_BUTTON_A) {
            HuAudFXPlay(2);
            SetSelectionCharacterAnimation(2);
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            break;
        }
        HuPrcVSleep();
    }
    HuAudFXPlay(3);
    CloseMessageWindow();
    WipeColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 20);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    HuSprGrpKill(selectionTitleSpriteGroup);
    HuSprGrpKill(bonusStarSpriteGroup);
    Hu3DModelKill(selectionCharacterModel);
    HuPrcKill(characterAssetLoaderProcess);
    Hu3DModelKill(characterModelMatrixHook);
    CharModelKill(-1);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        if (GWPlayerCfg[playerIndicesByTeam[playerIndex]].character >= 8) {
            Hu3DModelKill(playerCharacterModels[playerIndex]);
            Hu3DMotionKill(playerCharacterMotions[playerIndex][0]);
            Hu3DMotionKill(playerCharacterMotions[playerIndex][1]);
        }
    }
    selectionEntryMode = 0;
    selectionMode = 0;
}

void UpdateSelectionCharacterModel(void)
{
    Vec positionVector;
    Vec projectedCharacterPosition;
    Vec shadowTargetPosition;
    Vec shadowUpVector;
    s16 playerIndexByGroup[4];
    s16 playerCountByGroup[2];
    s16 playerIndex;
    s16 selectedCharacterId;
    s16 spriteId;
    s16 characterPositionX;
    ANIMDATA *spriteAnimation;
    s16 extraCharacterModelIndex;
    s16 characterModelId;

    playerCountByGroup[0] = 0;
    playerCountByGroup[1] = 2;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        playerIndexByGroup[playerCountByGroup[GWPlayerCfg[playerIndex].group]++] = playerIndex;
    }
    spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 3));
    selectionTitleSpriteGroup = HuSprGrpCreate(1);
    spriteId = HuSprCreate(spriteAnimation, 0xC8, 0);
    HuSprGrpMemberSet(selectionTitleSpriteGroup, 0, spriteId);
    HuSprGrpDrawNoSet(selectionTitleSpriteGroup, 0x7F);
    HuSprGrpPosSet(selectionTitleSpriteGroup, 288.0f, 240.0f);
    bonusStarSpriteGroup = HuSprGrpCreate(2);
    spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 13));
    spriteId = HuSprCreate(spriteAnimation, 0x78, 0);
    HuSprGrpMemberSet(bonusStarSpriteGroup, 0, spriteId);
    HuSprPosSet(bonusStarSpriteGroup, 0, 288.0f, 80.0f);
    HuSprGrpPosSet(bonusStarSpriteGroup, 0.0f, 0.0f);
    HuSprGrpDrawNoSet(bonusStarSpriteGroup, 0x7F);
    spriteAnimation = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 9));
    spriteId = HuSprCreate(spriteAnimation, 0x64, 0);
    HuSprGrpMemberSet(bonusStarSpriteGroup, 1, spriteId);
    HuSprPosSet(bonusStarSpriteGroup, 1, 288.0f, 240.0f);
    HuSprAttrSet(bonusStarSpriteGroup, 1, 4);
    HuSprDrawNoSet(bonusStarSpriteGroup, 1, 0);
    positionVector.x = 510.0f;
    positionVector.y = 80.0f;
    positionVector.z = 1500.0f;
    Hu3D2Dto3D(&positionVector, 1, &projectedCharacterPosition);
    selectionCharacterModel = Hu3DModelCreateFile(DATA_MAKE_NUM(DATADIR_ZTAR, 0));
    selectionCharacterMotions[0] = Hu3DJointMotionFile(selectionCharacterModel, DATA_MAKE_NUM(DATADIR_ZTAR, 1));
    selectionCharacterMotions[1] = Hu3DJointMotionFile(selectionCharacterModel, DATA_MAKE_NUM(DATADIR_ZTAR, 2));
    Hu3DModelAttrSet(selectionCharacterModel, HU3D_MOTATTR_LOOP);
    Hu3DModelPosSetV(selectionCharacterModel, &projectedCharacterPosition);
    Hu3DModelRotSet(selectionCharacterModel, 0.0f, -10.0f, -15.0f);
    Hu3DMotionSet(selectionCharacterModel, selectionCharacterMotions[0]);
    characterAssetLoaderProcess = HuPrcChildCreate(CreateCharacterSelectionSprites, 0x64, 0x2000, 0, selectionObjectManager);
    characterPositionX = -0x113;
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        selectedCharacterId = GWPlayerCfg[playerIndexByGroup[playerIndex]].character;
        if (selectedCharacterId < 8) {
            playerCharacterModels[playerIndex] = CharModelCreate(selectedCharacterId, 2);
            playerCharacterMotions[playerIndex][0] = CharMotionCreate(selectedCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00));
            playerCharacterMotions[playerIndex][3] = CharMotionCreate(selectedCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17));
            playerCharacterMotions[playerIndex][4] = CharMotionCreate(selectedCharacterId, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18));
            Hu3DModelScaleSet(playerCharacterModels[playerIndex], 0.8f, 0.8f, 0.8f);
        }
        else {
            extraCharacterModelIndex = selectedCharacterId - 8;
            characterModelId = Hu3DModelCreateFile(characterModelDataFiles[extraCharacterModelIndex]);
            playerCharacterModels[playerIndex] = characterModelId;
            playerCharacterMotions[playerIndex][0] = Hu3DJointMotionFile(characterModelId, characterModelDataFiles[extraCharacterModelIndex] + 1);
            playerCharacterMotions[playerIndex][3] = Hu3DJointMotionFile(characterModelId, characterModelDataFiles[extraCharacterModelIndex] + 3);
            playerCharacterMotions[playerIndex][4] = Hu3DJointMotionFile(characterModelId, characterModelDataFiles[extraCharacterModelIndex] + 4);
            Hu3DModelScaleSet(playerCharacterModels[playerIndex], 0.8f, 0.8f, 0.8f);
        }
        Hu3DMotionSet(playerCharacterModels[playerIndex], playerCharacterMotions[playerIndex][0]);
        Hu3DModelAttrSet(playerCharacterModels[playerIndex], HU3D_MOTATTR_LOOP);
        Hu3DModelPosSet(playerCharacterModels[playerIndex], characterPositionX, 0.0f, 0.0f);
        Hu3DModelShadowSet(playerCharacterModels[playerIndex]);
        characterPositionX += 0x96;
        if (playerIndex == 1) {
            characterPositionX += 0x64;
        }
    }
    CharModelDataClose(-1);
    Hu3DShadowCreate(20.0f, 500.0f, 8000.0f);
    Hu3DShadowTPLvlSet(0.5f);
    Hu3DShadowSizeSet(0xC0);
    shadowTargetPosition.x = shadowTargetPosition.y = shadowTargetPosition.z = 0.0f;
    positionVector.x = positionVector.z = 100.0f;
    positionVector.y = 2000.0f;
    shadowUpVector.x = shadowUpVector.y = 0.0f;
    shadowUpVector.z = 1.0f;
    Hu3DShadowPosSet(&positionVector, &shadowUpVector, &shadowTargetPosition);
    characterModelMatrixHook = Hu3DHookFuncCreate(SetCharacterModelMatrix);
    Hu3DModelLayerSet(characterModelMatrixHook, 1);
}

void SetSelectionCharacterAnimation(s16 overlayEntryPoint)
{
    s16 characterIndex;
    OMOVLHIS *overlayHistory;
    u32 audioDrainStartTick;

    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 0x3C);
    HuAudSeqAllFadeOut(0x3E8);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
#if !VERSION_ENG
    if ((GWPlayerCfg->character >= 8) && (GWPlayerCfg->character >= 8) && (GWPlayerCfg->character >= 8) && (GWPlayerCfg->character >= 8)) {
        msmMusStopAll(1, 0);
        msmSeStopAll(1, 0);
        audioDrainStartTick = OSGetTick();
        while (TRUE) {
            if ((msmMusGetNumPlay(1) != 0) || (msmSeGetNumPlay(1) != 0)) {
                if (((OSGetTick() - audioDrainStartTick) / (*((u32 *)0x800000F8) / 4 / 1000)) >= 0x1F4) {
                    break;
                }
            }
            else {
                break;
            }
        }
        msmSysDelGroupBase(0);
#if VERSION_PAL
        for (characterIndex = 0; characterIndex < 8; characterIndex++) {
            charVoiceGroupStat[characterIndex] = 0;
        }
#endif
    }
#endif
    overlayHistory = omOvlHisGet(0);
    omOvlHisChg(0, DLL_ztardll, overlayEntryPoint, overlayHistory->stat);
    omOvlCallEx(DLL_m433dll, 1, 0, 0);
    while (TRUE) {
        HuPrcVSleep();
    }
}

float selectionCharacterPositionOffsets[8] = {
    -132.0f,
    -12.0f,
    60.0f,
    132.0f,
    -132.0f,
    -60.0f,
    60.0f,
    132.0f,
};

u8 fourPlayerTeamOrders[16] = { 0, 1, 2, 3, 1, 0, 2, 3, 2, 0, 1, 3, 3, 0, 1, 2 };
u8 twoPlayerTeamOrders[12] = { 0, 1, 2, 3, 0, 2, 1, 3, 0, 3, 1, 2 };

s32 GetSelectionCharacterAnimation(s32 allowRepeatedTeamSelection)
{
    s16 teamSelectionRepeatRequested;
    float animationProgress;
    float transitionY;
    float screenWidth;
    s16 playerIndex;
    s16 teamOrderIndex;
    float *characterXPositions;
    s16 playerSpriteGroup;
    s16 teamOrderSpriteGroup;
    ANIMDATA *spriteAnimationData;
    s16 spriteId;
    s16 teamOrderWindowId;
    s16 teamSelectionRepeatRequested;
    WindowData *windowData;

    if (mgTypeCurr == 1) {
        characterXPositions = selectionCharacterPositionOffsets;
        screenWidth = 216.0f;
    }
    else {
        characterXPositions = &selectionCharacterPositionOffsets[4];
        screenWidth = 288.0f;
    }
    playerSpriteGroup = HuSprGrpCreate(0x10);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        spriteAnimationData = HuSprAnimReadFile(GWPlayerCfg[playerIndex].character + DATA_MAKE_NUM(DATADIR_ZTAR, 69));
        spriteId = HuSprCreate(spriteAnimationData, 4, 0);
        HuSprGrpMemberSet(playerSpriteGroup, playerIndex * 4, spriteId);
        HuSprPosSet(playerSpriteGroup, playerIndex * 4, characterXPositions[playerIndex], 0.0f);
        if (mgTypeCurr == 1) {
            if (playerIndex == 0) {
                spriteAnimationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 89));
            }
            else {
                spriteAnimationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 90));
            }
        }
        else if (playerIndex < 2) {
            spriteAnimationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 89));
        }
        else {
            spriteAnimationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 90));
        }
        spriteId = HuSprCreate(spriteAnimationData, 3, 0);
        HuSprGrpMemberSet(playerSpriteGroup, (playerIndex * 4) + 1, spriteId);
        HuSprPosSet(playerSpriteGroup, (playerIndex * 4) + 1, characterXPositions[playerIndex], 0.0f);
        if (GWPlayerCfg[playerIndex].iscom != 0) {
            spriteAnimationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 87));
        }
        else {
            spriteAnimationData = HuSprAnimReadFile(GWPlayerCfg[playerIndex].pad_idx + DATA_MAKE_NUM(DATADIR_ZTAR, 83));
        }
        spriteId = HuSprCreate(spriteAnimationData, 2, 1);
        HuSprGrpMemberSet(playerSpriteGroup, (playerIndex * 4) + 2, spriteId);
        HuSprPosSet(playerSpriteGroup, (playerIndex * 4) + 2, characterXPositions[playerIndex], 0.0f);
        spriteAnimationData = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_ZTAR, 88));
        spriteId = HuSprCreate(spriteAnimationData, 2, GWPlayerCfg[playerIndex].diff);
        HuSprGrpMemberSet(playerSpriteGroup, (playerIndex * 4) + 3, spriteId);
        HuSprPosSet(playerSpriteGroup, (playerIndex * 4) + 3, characterXPositions[playerIndex], 38.0f);
        if (GWPlayerCfg[playerIndex].iscom == 0) {
            HuSprAttrSet(playerSpriteGroup, (playerIndex * 4) + 3, 4);
        }
    }
    teamOrderSpriteGroup = HuSprGrpCreate(5);
    spriteAnimationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_ZTAR, 67), HU_MEMNUM_OVL));
    spriteId = HuSprCreate(spriteAnimationData, 0x32, 0);
    HuSprGrpMemberSet(teamOrderSpriteGroup, 0, spriteId);
    HuSprTPLvlSet(teamOrderSpriteGroup, 0, 0.9f);
    HuSprPosSet(teamOrderSpriteGroup, 0, 0.0f, 0.0f);
    HuSprScaleSet(teamOrderSpriteGroup, 0, 1.1f, 1.0f);
    spriteAnimationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_ZTAR, 65), HU_MEMNUM_OVL));
    spriteId = HuSprCreate(spriteAnimationData, 0x28, 0);
    HuSprGrpMemberSet(teamOrderSpriteGroup, 1, spriteId);
    HuSprPosSet(teamOrderSpriteGroup, 1, 0.0f, -90.0f);
    spriteAnimationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_ZTAR, 66), HU_MEMNUM_OVL));
    spriteId = HuSprCreate(spriteAnimationData, 0x28, 0);
    HuSprGrpMemberSet(teamOrderSpriteGroup, 2, spriteId);
    HuSprPosSet(teamOrderSpriteGroup, 2, 0.0f, 0.0f);
    spriteAnimationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_ZTAR, 68), HU_MEMNUM_OVL));
    spriteId = HuSprCreate(spriteAnimationData, 0x28, 1);
    HuSprGrpMemberSet(teamOrderSpriteGroup, 3, spriteId);
    HuSprPosSet(teamOrderSpriteGroup, 3, 180.0f, 0.0f);
    spriteAnimationData = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_ZTAR, 68), HU_MEMNUM_OVL));
    spriteId = HuSprCreate(spriteAnimationData, 0x28, 0);
    HuSprGrpMemberSet(teamOrderSpriteGroup, 4, spriteId);
    HuSprPosSet(teamOrderSpriteGroup, 4, -180.0f, 0.0f);
    HuSprGrpPosSet(teamOrderSpriteGroup, 288.0f, 240.0f);
    for (playerIndex = 0; playerIndex <= 0x3C; playerIndex++) {
        if (playerIndex <= 0x32) {
            animationProgress = playerIndex / 50.0;
            transitionY = -300.0 + (560.0 * sind((90.0f * animationProgress)));
            HuSprGrpPosSet(teamOrderSpriteGroup, 288.0f, transitionY);
            HuSprGrpPosSet(playerSpriteGroup, 288.0f, transitionY);
        }
        else {
            animationProgress = (playerIndex - 0x32) / 10.0;
            transitionY = 240.0 + (20.0 * cosd((90.0f * animationProgress)));
            transitionY = transitionY;
            HuSprGrpPosSet(teamOrderSpriteGroup, 288.0f, transitionY);
            HuSprGrpPosSet(playerSpriteGroup, 288.0f, transitionY);
        }
        HuPrcVSleep();
    }
    teamOrderWindowId = HuWinCreate(0.0f, 0.0f, 0x1E0, 0x28, 0);
    HuWinMesSpeedSet(teamOrderWindowId, 0);
    HuWinBGTPLvlSet(teamOrderWindowId, 0.0f);
    HuWinMesSet(teamOrderWindowId, MAKE_MESSID(0x1A, 0x20));
    windowData = &winData[teamOrderWindowId];
    HuWinPriSet(teamOrderWindowId, 5);
    HuWinPosSet(teamOrderWindowId, (576.0f - windowData->w) / 2, 300.0f);
#if VERSION_PAL
    HuWinScaleSet(teamOrderWindowId, 0.95f, 1.0f);
#endif
    HuWinAttrSet(teamOrderWindowId, 0x800);
    teamOrderIndex = 0;
    teamSelectionRepeatRequested = 0;
    while (TRUE) {
        HuPrcVSleep();
        if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            HuAudFXPlay(3);
            goto block89;
        }
        teamSelectionRepeatRequested = 0;
        if (HuPadDStkRep[0] & PAD_BUTTON_LEFT) {
            HuAudFXPlay(0x304);
            for (playerIndex = 0; playerIndex <= 5; playerIndex++) {
                animationProgress = playerIndex / 5.0;
                HuSprPosSet(teamOrderSpriteGroup, 4, -180.0 - (10.0 * sind((90.0f * animationProgress))), 0.0f);
                HuPrcVSleep();
            }
            teamOrderIndex--;
            if (mgTypeCurr == 1) {
                if (teamOrderIndex < 0) {
                    teamOrderIndex = 3;
                }
                if (teamOrderIndex >= 4) {
                    teamOrderIndex = 0;
                }
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    HuSprPosSet(playerSpriteGroup, fourPlayerTeamOrders[teamOrderIndex * 4 + playerIndex] * 4, characterXPositions[playerIndex], 0.0f);
                    HuSprPosSet(playerSpriteGroup, fourPlayerTeamOrders[teamOrderIndex * 4 + playerIndex] * 4 + 2, characterXPositions[playerIndex], 0.0f);
                    HuSprPosSet(playerSpriteGroup, fourPlayerTeamOrders[teamOrderIndex * 4 + playerIndex] * 4 + 3, characterXPositions[playerIndex], 38.0f);
                }
            }
            else {
                if (teamOrderIndex < 0) {
                    teamOrderIndex = 2;
                }
                if (teamOrderIndex >= 3) {
                    teamOrderIndex = 0;
                }
                for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                    HuSprPosSet(playerSpriteGroup, twoPlayerTeamOrders[teamOrderIndex * 4 + playerIndex] * 4, characterXPositions[playerIndex], 0.0f);
                    HuSprPosSet(playerSpriteGroup, twoPlayerTeamOrders[teamOrderIndex * 4 + playerIndex] * 4 + 2, characterXPositions[playerIndex], 0.0f);
                    HuSprPosSet(playerSpriteGroup, twoPlayerTeamOrders[teamOrderIndex * 4 + playerIndex] * 4 + 3, characterXPositions[playerIndex], 38.0f);
                }
            }
            for (playerIndex = 0; playerIndex <= 5; playerIndex++) {
                animationProgress = playerIndex / 5.0;
                HuSprPosSet(teamOrderSpriteGroup, 4, -180.0 - (10.0 * cosd((90.0f * animationProgress))), 0.0f);
                HuPrcVSleep();
            }
        }
        else {
            if (HuPadDStkRep[0] & PAD_BUTTON_RIGHT) {
                HuAudFXPlay(0x304);
                for (playerIndex = 0; playerIndex <= 5; playerIndex++) {
                    animationProgress = playerIndex / 5.0;
                    HuSprPosSet(teamOrderSpriteGroup, 3, 180.0 - (10.0 * sind((90.0f * animationProgress))), 0.0f);
                    HuPrcVSleep();
                }
                teamOrderIndex++;
                if (mgTypeCurr == 1) {
                    if (teamOrderIndex < 0) {
                        teamOrderIndex = 3;
                    }
                    if (teamOrderIndex >= 4) {
                        teamOrderIndex = 0;
                    }
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        HuSprPosSet(playerSpriteGroup, fourPlayerTeamOrders[teamOrderIndex * 4 + playerIndex] * 4, characterXPositions[playerIndex], 0.0f);
                        HuSprPosSet(playerSpriteGroup, (fourPlayerTeamOrders[teamOrderIndex * 4 + playerIndex] * 4) + 2, characterXPositions[playerIndex], 0.0f);
                        HuSprPosSet(playerSpriteGroup, (fourPlayerTeamOrders[teamOrderIndex * 4 + playerIndex] * 4) + 3, characterXPositions[playerIndex], 38.0f);
                    }
                }
                else {
                    if (teamOrderIndex < 0) {
                        teamOrderIndex = 2;
                    }
                    if (teamOrderIndex >= 3) {
                        teamOrderIndex = 0;
                    }
                    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
                        HuSprPosSet(playerSpriteGroup, twoPlayerTeamOrders[teamOrderIndex * 4 + playerIndex] * 4, characterXPositions[playerIndex], 0.0f);
                        HuSprPosSet(playerSpriteGroup, (twoPlayerTeamOrders[teamOrderIndex * 4 + playerIndex] * 4) + 2, characterXPositions[playerIndex], 0.0f);
                        HuSprPosSet(playerSpriteGroup, (twoPlayerTeamOrders[teamOrderIndex * 4 + playerIndex] * 4) + 3, characterXPositions[playerIndex], 38.0f);
                    }
                }
                for (playerIndex = 0; playerIndex <= 5; playerIndex++) {
                    animationProgress = playerIndex / 5.0;
                    HuSprPosSet(teamOrderSpriteGroup, 3, 180.0 + (10.0 * cosd((90.0f * animationProgress))), 0.0f);
                    HuPrcVSleep();
                }
            }
        }
        if (HuPadBtnDown[0] & (PAD_BUTTON_START | PAD_BUTTON_A)) {
            HuAudFXPlay(0x305);
            if (mgTypeCurr == 1) {
                GWPlayerCfg[fourPlayerTeamOrders[teamOrderIndex * 4]].group = 0;
                for (playerIndex = 1; playerIndex < 4; playerIndex++) {
                    GWPlayerCfg[fourPlayerTeamOrders[teamOrderIndex * 4 + playerIndex]].group = 1;
                }
            }
            else {
                for (playerIndex = 0; playerIndex < 2; playerIndex++) {
                    GWPlayerCfg[twoPlayerTeamOrders[teamOrderIndex * 4 + playerIndex]].group = 0;
                }
                for (playerIndex = 2; playerIndex < 4; playerIndex++) {
                    GWPlayerCfg[twoPlayerTeamOrders[teamOrderIndex * 4 + playerIndex]].group = 1;
                }
            }
            if (allowRepeatedTeamSelection != 0) {
                teamSelectionRepeatRequested = 1;
            }
            else {
                return 1;
            }
        block89:
            HuWinKill(teamOrderWindowId);
            for (playerIndex = 0; playerIndex <= 0x3C; playerIndex++) {
                if (playerIndex <= 0xA) {
                    animationProgress = playerIndex / 10.0;
                    transitionY = 240.0 + (20.0 * sind((90.0f * animationProgress)));
                    HuSprGrpPosSet(teamOrderSpriteGroup, 288.0f, transitionY);
                    HuSprGrpPosSet(playerSpriteGroup, 288.0f, transitionY);
                }
                else {
                    animationProgress = (playerIndex - 0xA) / 50.0;
                    transitionY = -300.0 + (560.0 * cosd(90.0f * animationProgress));
                    HuSprGrpPosSet(teamOrderSpriteGroup, 288.0f, transitionY);
                    HuSprGrpPosSet(playerSpriteGroup, 288.0f, transitionY);
                }
                HuPrcVSleep();
            }
            HuSprGrpKill(playerSpriteGroup);
            HuSprGrpKill(teamOrderSpriteGroup);
            return teamSelectionRepeatRequested;
        }
    }
}

void UpdateSelectionObject(void)
{
    s16 playerIndex;

    while (TRUE) {
        if (selectionResultReady != 0 || omSysExitReq) {
            break;
        }
        HuPrcVSleep();
    }
    WipeColorSet(0xFF, 0xFF, 0xFF);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    while (WipeStatGet() != 0) {
        HuPrcVSleep();
    }
    HuDataDirClose(DATADIR_INSTPIC);
    while (characterAssetLoadActive != 0) {
        HuPrcVSleep();
    }
    CharModelDataClose(-1);
    for (playerIndex = 0; playerIndex < 4; playerIndex++) {
        GWPlayerCfg[playerIndex].character = -1;
    }
#if VERSION_PAL
    for (playerIndex = 0; playerIndex < 8; playerIndex++) {
        charVoiceGroupStat[playerIndex] = 0;
    }
#endif
    mgPracticeEnableF = 0;
    omOvlReturnEx(1, 1);
    HuPrcEnd();
}

void CreateCharacterSelectionPrompt(void)
{
    Vec cameraPosition;
    Vec cameraToCenter;
    Vec cameraUpVector;
    Vec rotatedCameraUpVector;
    float cameraRollAngle;
    float cameraVectorComponent;
    float cameraZoom;
    s8 subStickPanInput;

    CRot.y += 0.1f * HuPadStkX[0];
    CRot.x += 0.1f * HuPadStkY[0];
    CZoom += HuPadTrigL[0] / 2;
    CZoom -= HuPadTrigR[0] / 2;
    if (HuPadBtnDown[0] & PAD_BUTTON_B) {
        OSReport("%f,%f,%f\n", CRot.x, CRot.y, CRot.z);
        OSReport("%f,%f,%f\n", Center.x, Center.y, Center.z);
        OSReport("%f\n", CZoom);
    }
    if (CZoom < 100.0f) {
        CZoom = 100.0f;
    }
    cameraPosition.x = Center.x + (CZoom * (sind(CRot.y) * cosd(CRot.x)));
    cameraPosition.y = Center.y + (CZoom * -sind(CRot.x));
    cameraPosition.z = Center.z + (CZoom * (cosd(CRot.y) * cosd(CRot.x)));
    cameraToCenter.x = Center.x - cameraPosition.x;
    cameraToCenter.y = Center.y - cameraPosition.y;
    cameraToCenter.z = Center.z - cameraPosition.z;
    cameraUpVector.x = sind(CRot.y) * sind(CRot.x);
    cameraUpVector.y = cosd(CRot.x);
    cameraUpVector.z = cosd(CRot.y) * sind(CRot.x);
    cameraRollAngle = CRot.z;
    rotatedCameraUpVector.x = cameraUpVector.x * ((cameraToCenter.x * cameraToCenter.x) + ((1.0f - (cameraToCenter.x * cameraToCenter.x)) * cosd(cameraRollAngle)))
        + cameraUpVector.y * (((cameraToCenter.x * cameraToCenter.y) * (1.0 - cosd(cameraRollAngle))) - cameraToCenter.z * sind(cameraRollAngle))
        + cameraUpVector.z * (((cameraToCenter.x * cameraToCenter.z) * (1.0 - cosd(cameraRollAngle))) + cameraToCenter.y * sind(cameraRollAngle));

    rotatedCameraUpVector.y = cameraUpVector.y * ((cameraToCenter.y * cameraToCenter.y) + ((1.0f - (cameraToCenter.y * cameraToCenter.y)) * cosd(cameraRollAngle)))
        + cameraUpVector.x * (((cameraToCenter.x * cameraToCenter.y) * (1.0 - cosd(cameraRollAngle))) + cameraToCenter.z * sind(cameraRollAngle))
        + cameraUpVector.z * (((cameraToCenter.y * cameraToCenter.z) * (1.0 - cosd(cameraRollAngle))) - cameraToCenter.x * sind(cameraRollAngle));

    rotatedCameraUpVector.z = cameraUpVector.z * ((cameraToCenter.z * cameraToCenter.z) + ((1.0f - (cameraToCenter.z * cameraToCenter.z)) * cosd(cameraRollAngle)))
        + (cameraUpVector.x * (((cameraToCenter.x * cameraToCenter.z) * (1.0 - cosd(cameraRollAngle))) - cameraToCenter.y * sind(cameraRollAngle))
            + cameraUpVector.y * (((cameraToCenter.y * cameraToCenter.z) * (1.0 - cosd(cameraRollAngle))) + cameraToCenter.x * sind(cameraRollAngle)));
    VECCrossProduct(&cameraUpVector, &cameraToCenter, &cameraToCenter);
    VECNormalize(&cameraToCenter, &cameraToCenter);
    subStickPanInput = HuPadSubStkX[0] & 0xF8;
    if (subStickPanInput != 0) {
        Center.x += 0.05f * (cameraToCenter.x * subStickPanInput);
        Center.y += 0.05f * (cameraToCenter.y * subStickPanInput);
        Center.z += 0.05f * (cameraToCenter.z * subStickPanInput);
    }
    VECNormalize(&rotatedCameraUpVector, &cameraToCenter);
    subStickPanInput = -(HuPadSubStkY[0] & 0xF8);
    if (subStickPanInput != 0) {
        Center.x += 0.05f * (cameraToCenter.x * subStickPanInput);
        Center.y += 0.05f * (cameraToCenter.y * subStickPanInput);
        Center.z += 0.05f * (cameraToCenter.z * subStickPanInput);
    }
}

void SetCharacterModelMatrix(HU3DMODEL *model, Mtx matrix)
{
    Mtx cameraRelativeShadowMatrix;
    Mtx inverseCameraMatrix;
    Mtx shadowProjectionMatrix;
    GXTexObj shadowTextureObject;
    s32 shadowTextureMatrixIndex;

    shadowTextureMatrixIndex = 0;
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXInitTexObj(&shadowTextureObject, Hu3DShadowData.buf, Hu3DShadowData.size, Hu3DShadowData.size, 1, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXInitTexObjLOD(&shadowTextureObject, GX_LINEAR, GX_LINEAR, 0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXLoadTexObj(&shadowTextureObject, GX_TEXMAP0);
    MTXInverse(Hu3DCameraMtx, inverseCameraMatrix);
    MTXConcat(inverseCameraMatrix, matrix, cameraRelativeShadowMatrix);
    MTXConcat(Hu3DShadowData.projMtx, Hu3DShadowData.lookAtMtx, shadowProjectionMatrix);
    MTXConcat(shadowProjectionMatrix, cameraRelativeShadowMatrix, cameraRelativeShadowMatrix);
    GXLoadTexMtxImm(cameraRelativeShadowMatrix, 0x39, GX_MTX3x4);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, 0x39, GX_FALSE, 0x7D);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_FALSE, GX_TEVPREV);
    GXSetNumTexGens(1);
    GXSetNumTevStages(1);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0, GX_TRUE, GX_SRC_REG, GX_SRC_VTX, 1, GX_DF_CLAMP, GX_AF_NONE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ZERO, GX_BL_INVDSTCLR, GX_LO_NOOP);
    GXSetZMode(1, GX_LEQUAL, GX_FALSE);
    GXLoadPosMtxImm(matrix, 0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-500.0f, 0.0f, -500.0f);
    GXPosition3f32(500.0f, 0.0f, -500.0f);
    GXPosition3f32(500.0f, 0.0f, 500.0f);
    GXPosition3f32(-500.0f, 0.0f, 500.0f);
}

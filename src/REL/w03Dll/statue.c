#include "REL/executor.h"
#include "REL/w03Dll.h"
#include "dolphin.h"
#include "game/audio.h"
#include "game/board/main.h"
#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/space.h"
#include "game/board/ui.h"

#include "game/board/window.h"

#ifndef __MWERKS__
#include "game/board/audio.h"
#include "game/board/com.h"
#endif
#include "game/data.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/objsub.h"
#include "game/process.h"

#include "ext_math.h"

// forward declarations from main.c
void W03SetBridgeMotion(s32 value);
void W03UpdateJunctionLinks(void);

void StatueEventRunProcess(void);
void StatueEventProcessCleanup(void);
s32 StatuePostTurnCheck(void);
s32 StatuePlayerApproach(s32 playerIndex);
void StatuePlayPlayerReaction(s32 playerIndex, s32 playerReactionType);
s32 StatueRewardSelect(void);
void StatuePlaySmallRewardEffect(s32 playerIndex);
void StatuePlayLargeRewardEffect(s32 playerIndex);
void StatueReturnPlayer(s32 playerIndex);
void StatueOverlayDestroy(void);
void StatueWaitForRewardAnimation(void);
void StatueOverlayCreate(void);
void StatueOverlayDraw(void);

// data
s16 playerReturnMotionId = -1;
s16 playerRewardMotionId = -1;
s16 rewardEffectModelId = -1;
s16 overlayHookId = -1;
s32 rewardSoundHandle = -1;

// BSS Extern

// BSS
s8 rewardChoice;
s8 statueSide;
s16 statueModelId;
s16 playerReactionVoiceId;
s16 playerReturnMotionId;
GXTexObj overlayTexture;
char rewardChoiceText[4];
char rewardAmountText[12];
void *overlayTextureBuffer;
f32 overlayOpacity;
s8 rewardAmount;
HUPROCESS *statueEventProcess;

void StatueEventStart(void)
{
    BoardRollDispSet(0);
    statueEventProcess = HuPrcChildCreate(&StatueEventRunProcess, 0x2003, 0x2000, 0, boardMainProc);
    HuPrcDestructorSet2(statueEventProcess, &StatueEventProcessCleanup);

    while (statueEventProcess) {
        HuPrcVSleep();
    }

    BoardRollDispSet(1);
}

void StatueEventRunProcess(void)
{
    s32 curPlayerIndex;
    s32 currentSpace;

    curPlayerIndex = GWSystem.player_curr;
    currentSpace = GWPlayer[curPlayerIndex].space_curr;
    rewardAmount = 0;

    if ((BoardSpaceFlagGet(0, GWPlayer[curPlayerIndex].space_curr) & 0x2000)) {
        statueSide = 0;
        statueModelId = boardModelIds[3];
    }
    else {
        statueSide = 1;
        statueModelId = boardModelIds[4];
    }

    if (StatuePlayerApproach(curPlayerIndex) != 0) {
        if (StatueRewardSelect() != 0) {
            StatuePlaySmallRewardEffect(curPlayerIndex);
        }
        else {
            StatuePlayLargeRewardEffect(curPlayerIndex);
        }
        if (rewardAmount) {
            sprintf(rewardChoiceText, "%d", rewardAmount);
            BoardWinCreate(0, MAKE_MESSID(22, 6), -1);
            BoardWinInsertMesSet((s32)rewardChoiceText, 0);
            BoardWinWait();
            BoardWinKill();
            boardState->rewardPlayerMask = 1 << curPlayerIndex;
            boardState->statueRewardCount = rewardAmount;
            W03UpdateJunctionLinks();
        }
        StatueReturnPlayer(curPlayerIndex);
        BoardStatusShowSetAll(1);
    }

    BoardPlayerPostTurnHookSet(curPlayerIndex, &StatuePostTurnCheck);
    BoardWinKill();
    HuPrcEnd();
}

void StatueEventProcessCleanup(void)
{
    BoardConfettiKill();

    if (playerRewardMotionId != -1) {
        BoardPlayerMotionKill(GWSystem.player_curr, playerRewardMotionId);
        playerRewardMotionId = -1;
    }

    if (playerReturnMotionId != -1) {
        BoardPlayerMotionKill(GWSystem.player_curr, playerReturnMotionId);
        playerReturnMotionId = -1;
    }

    StatueOverlayDestroy();
    statueEventProcess = NULL;
}

s32 StatuePostTurnCheck(void)
{
    BoardModelVisibilitySet(statueModelId, 1);

    if (rewardEffectModelId != -1) {
        BoardModelKill(rewardEffectModelId);
        rewardEffectModelId = -1;
    }

    return 1;
}

s32 StatuePlayerApproach(s32 playerIndex)
{
    Vec spacePosition;
    Vec playerPosition;
    Vec positionDelta;
    Vec cameraRotation;
    Vec cameraRotation2;
    f32 headingAngle;
    s32 linkedSpaceIndex;

    BoardPlayerIdleSet(playerIndex);

    if (boardState->rewardPlayerMask) {
        BoardWinCreate(0, MAKE_MESSID(22, 0), -1);
        BoardWinWait();
        BoardWinKill();
        return 0;
    }

    linkedSpaceIndex = BoardSpaceLinkFlagSearch(0, GWPlayer[playerIndex].space_curr, 0x02000000);
    BoardSpacePosGet(0, linkedSpaceIndex, &spacePosition);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    VECSubtract(&spacePosition, &playerPosition, &positionDelta);
    headingAngle = atan2d(-positionDelta.x, -positionDelta.z);
    BoardPlayerRotYSet(playerIndex, headingAngle);
    cameraRotation.x = 0.0f;
    cameraRotation.y = 100.0f;
    cameraRotation.z = 0.0f;
    cameraRotation2.x = -20.0f;
    cameraRotation2.y = 0.0f;
    cameraRotation2.z = 0.0f;
    BoardCameraMotionStartEx(statueModelId, &cameraRotation2, &cameraRotation, 1360.0f, -1.0f, 21);
    BoardPlayerPosLerpStart(playerIndex, &playerPosition, &spacePosition, 0x28);

    while (GWPlayer[playerIndex].moving) {
        HuPrcVSleep();
    }

    BoardStatusShowSetAll(0);
    BoardPlayerMotBlendSet(playerIndex, 0xB4, 0xF);

    while (BoardPlayerMotBlendCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }

    BoardPlayerIdleSet(playerIndex);
    BoardWinCreate(2, MAKE_MESSID(22, 1), 2);
    BoardWinWait();
    BoardWinCreateChoice(0, MAKE_MESSID(22, 2), -1, 0);
    BoardWinAttrSet(0x10);

    if (GWPlayer[playerIndex].com) {
        if (BoardRandMod(0x64) > 0x32U) {
            BoardComKeySetLeft();
        }
        else {
            BoardComKeySetRight();
        }
    }

    BoardWinWait();
    rewardChoice = BoardWinChoiceGet();
    BoardWinKill();
    StatuePlayPlayerReaction(playerIndex, rewardChoice);
    BoardFilterFadeInit(0x1E, 0xA0);
    BoardMusStart(1, 0x19, 0x7F, 0);
    HuAudFXPlay(0x435);
    cameraRotation2.y = BoardModelRotYGet(statueModelId);

    while (BoardDAngleCalcRange(&cameraRotation2.y, 180.0f, 2.0f) == 0) {
        BoardModelRotYSet(statueModelId, cameraRotation2.y);
        HuPrcVSleep();
    }

    HuAudFXPlay(0x445);
    BoardModelRotYSet(statueModelId, 180.0f);

    while (BoardMusStatusGet(1) != 0) {
        HuPrcVSleep();
    }
    return 1;
}

s32 playerReturnMotionData[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 98), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 98), DATA_MAKE_NUM(DATADIR_PEACHMOT, 98),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 98), DATA_MAKE_NUM(DATADIR_WARIOMOT, 98), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 98),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 98), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 98) };

s32 playerRewardMotionData[] = { DATA_MAKE_NUM(DATADIR_MARIOMOT, 51), DATA_MAKE_NUM(DATADIR_LUIGIMOT, 51), DATA_MAKE_NUM(DATADIR_PEACHMOT, 51),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 51), DATA_MAKE_NUM(DATADIR_WARIOMOT, 51), DATA_MAKE_NUM(DATADIR_DONKEYMOT, 51),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 51), DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 51) };

s32 normalRewardSoundIds[] = { 0x00000125, 0x00000165, 0x000001A5, 0x000001E5, 0x00000225, 0x00000265, 0x000002A5, 0x000002E5 };

s32 specialRewardSoundIds[] = { 0x0000012E, 0x0000016E, 0x000001AE, 0x000001EE, 0x0000022E, 0x0000026E, 0x000002AE, 0x000002EE };

void StatuePlayPlayerReaction(s32 playerIndex, s32 playerIndexValue)
{
    s32 characterIndex;
    s32 integerValue;
    s32 integerValue2;
    s32 reactionSoundId;
    f32 value;

    characterIndex = GWPlayer[playerIndex].character;

    if (playerIndexValue != 0) {
        playerRewardMotionId = BoardPlayerMotionCreate(playerIndex, playerRewardMotionData[characterIndex]);
        playerReactionVoiceId = 8;
        playerReturnMotionId = playerRewardMotionId;
        reactionSoundId = specialRewardSoundIds[GWPlayer[playerIndex].character];
    }
    else {
        playerReactionVoiceId = 7;
        playerReturnMotionId = 1;
        reactionSoundId = normalRewardSoundIds[GWPlayer[playerIndex].character];
    }

    playerReturnMotionId = BoardPlayerMotionCreate(playerIndex, playerReturnMotionData[characterIndex]);
    value = (180.0f + BoardPlayerRotYGet(playerIndex));
    BoardPlayerMotBlendSet(playerIndex, value, 15);

    while (BoardPlayerMotBlendCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }

    HuAudFXPlay(reactionSoundId);
    BoardPlayerVoiceEnableSet(playerIndex, playerReactionVoiceId, 0);
    BoardPlayerMotionShiftSet(playerIndex, playerReactionVoiceId, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(9);

    while (BoardPlayerMotionEndCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }

    HuPrcSleep(60);

    if (playerIndexValue != 0) {
        BoardPlayerMotionShiftSet(playerIndex, playerReturnMotionId, 0.0f, 8.0f, HU3D_MOTATTR_NONE);
        HuPrcSleep(9);
        while (BoardPlayerMotionEndCheck(playerIndex) == 0) {
            HuPrcVSleep();
        }
    }

    BoardPlayerIdleSet(playerIndex);
    HuPrcSleep(0x14);
    value = (180.0f + BoardPlayerRotYGet(playerIndex));
    BoardPlayerMotBlendSet(playerIndex, value, 15);

    while (BoardPlayerMotBlendCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }

    BoardAudSeqPause(0, 1, 1000);
    BoardPlayerMotionShiftSet(playerIndex, playerReturnMotionId, 0.0f, 10.0f, HU3D_MOTATTR_NONE);
    HuPrcSleep(11);

    while (BoardPlayerMotionEndCheck(playerIndex) == 0) {
        HuPrcVSleep();
    }
}

s32 StatueRewardSelect(void)
{
    s32 integerValue;
    s32 integerValue2;
    s32 integerValue3;
    s32 integerValue4;
    s32 integerValue5;
    s32 integerValue6;
    s32 modRand2;
    s32 stateValue;
    s32 value;
    s32 modRand;
    s32 prevTurn;
    s32 maxTurn;

    s32 integerValue7[3] = { 70, 50, 45 };
    s32 integerValue8[3][3] = { { 3, 5, 22 }, { 5, 10, 35 }, { 5, 15, 35 } };

    while (BoardMusStatusGet(1) != 0) {
        HuPrcVSleep();
    }
    BoardWinCreate(2, MAKE_MESSID(22, 3), 2);
    BoardWinWait();
    BoardWinKill();

    prevTurn = GWSystem.turn - 1;
    maxTurn = GWSystem.max_turn;
    stateValue = prevTurn / (maxTurn / 3);

    if (stateValue >= 3) {
        stateValue = 2;
    }

    modRand = BoardRandMod(100);
    if (modRand < integerValue7[stateValue]) {
        return 1;
    }

    modRand2 = BoardRandMod(100 - integerValue7[stateValue]);
    if (modRand2 < integerValue8[stateValue][0]) {
        rewardAmount = 3;
    }
    else if (modRand2 < integerValue8[stateValue][1]) {
        rewardAmount = 2;
    }
    else {
        rewardAmount = 1;
    }

    return 0;
}

void StatuePlaySmallRewardEffect(s32 playerIndex)
{
    Vec playerPosition;
    f32 floatValue;

    rewardEffectModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x0C), NULL, 0);
    BoardModelVisibilitySet(rewardEffectModelId, 0);
    BoardModelVisibilitySet(statueModelId, 0);
    BoardModelPosGet(statueModelId, &playerPosition);
    BoardModelVisibilitySet(rewardEffectModelId, 1);
    BoardModelPosSetV(rewardEffectModelId, &playerPosition);
    BoardFilterFadeOut(0x1E);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    playerPosition.y += 700.0f;
    BoardConfettiCreate(&playerPosition, 0x64, 300.0f);
    floatValue = BoardModelRotYGet(statueModelId);

    while (BoardDAngleCalcRange(&floatValue, 0.0f, 30.0f) == 0) {
        BoardModelRotYSet(rewardEffectModelId, floatValue);
        HuPrcVSleep();
    }

    omVibrate(playerIndex, 0xC, 4, 2);
    BoardMusStart(1, 0x1A, 0x7F, 0);
    BoardModelRotYSet(rewardEffectModelId, 0.0f);
    BoardModelRotYSet(statueModelId, 0.0f);

    while (BoardFilterFadePauseCheck() == 0) {
        HuPrcVSleep();
    }

    BoardPlayerIdleSet(playerIndex);

    while (BoardMusStatusGet(1) != 0) {
        HuPrcVSleep();
    }

    BoardAudSeqPause(0, 0, 1000);
    BoardWinCreate(2, MAKE_MESSID(22, 4), 2);
    BoardWinWait();
    BoardWinKill();
    BoardConfettiStop();
}

void StatuePlayLargeRewardEffect(s32 playerIndex)
{
    Vec cameraRotation;
    Vec cameraRotation2;
    Vec modelPosition;
    f32 floatValue;
    f32 floatValue2;
    s32 i;

    rewardEffectModelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W03, 0x0D), NULL, 0);
    BoardModelVisibilitySet(rewardEffectModelId, 0);
    BoardModelVisibilitySet(statueModelId, 0);
    BoardModelPosGet(statueModelId, &modelPosition);
    BoardModelVisibilitySet(rewardEffectModelId, 1);
    BoardModelPosSetV(rewardEffectModelId, &modelPosition);
    BoardFilterFadeOut(0x1E);
    floatValue = BoardModelRotYGet(statueModelId);

    while (BoardDAngleCalcRange(&floatValue, 0.0f, 30.0f) == 0) {
        BoardModelRotYSet(rewardEffectModelId, floatValue);
        HuPrcVSleep();
    }

    omVibrate(playerIndex, 0xC, 4, 2);
    BoardModelRotYSet(rewardEffectModelId, 0.0f);
    BoardModelRotYSet(statueModelId, 0.0f);

    while (BoardFilterFadePauseCheck() == 0) {
        HuPrcVSleep();
    }

    BoardMusStart(1, 0x1B, 0x7F, 0);
    HuAudFXPlay(0x44A);
    StatueOverlayCreate();
    overlayOpacity = 0.0f;

    while (overlayOpacity < 128.0f) {
        HuPrcVSleep();
        overlayOpacity += 12.0f;
    }

    if (overlayOpacity > 128.0f) {
        overlayOpacity = 128.0f;
    }

    HuPrcSleep(40);
    overlayOpacity = 128.0f;

    while (overlayOpacity > 0.0f) {
        HuPrcVSleep();
        overlayOpacity -= 12.0f;
    }

    if (overlayOpacity < 0.0f) {
        overlayOpacity = 0.0f;
    }

    StatueOverlayDestroy();

    for (i = 0; i < 3; i++) {
        BoardCameraMotionStartEx(-1, NULL, NULL, 680.0f, -1.0f, 13);
        BoardCameraMotionWait();
        BoardCameraMotionStartEx(-1, NULL, NULL, 1360.0f, -1.0f, 13);
        BoardCameraMotionWait();
    }

    BoardPlayerIdleSet(playerIndex);

    while (BoardMusStatusGet(1) != 0) {
        HuPrcVSleep();
    }

    BoardAudSeqPause(0, 0, 1000);
    BoardWinCreate(2, MAKE_MESSID(22, 5), 2);
    BoardWinWait();
    BoardCameraQuakeSet(0x8CA0, 100.0f);
    omVibrate(playerIndex, 0xE10, 4, 2);
    rewardSoundHandle = HuAudFXPlay(0x436);
    BoardCameraViewSet(1);
    HuPrcSleep(0x3C);
    BoardCameraMotionWait();
    cameraRotation.x = 0.0f;
    cameraRotation.y = 100.0f;
    cameraRotation.z = 0.0f;
    cameraRotation2.x = -20.0f;
    cameraRotation2.y = 0.0f;
    cameraRotation2.z = 0.0f;
    BoardCameraMotionStartEx(boardModelIds[1], &cameraRotation2, &cameraRotation, 3000.0f, -1.0f, 60);
    BoardCameraMotionWait();
    omVibrate(playerIndex, 0x1E, 6, 6);
    HuPrcSleep(0x1E);

    for (floatValue2 = 100.0f; floatValue2 > 0.0f; floatValue2 -= 2.0f) {
        BoardCameraQuakeSet(0xA, floatValue2);
        HuPrcVSleep();
    }

    BoardCameraQuakeReset();

    if (rewardSoundHandle != 0) {
        HuAudFXStop(rewardSoundHandle);
        rewardSoundHandle = -1;
    }

    StatueWaitForRewardAnimation();
    omVibrate(playerIndex, 0xC, 0xC, 0);
    HuAudFXPlay(0x438);
    W03SetBridgeMotion(1);
    HuPrcSleep(0x3C);
    cameraRotation.x = 0.0f;
    cameraRotation.y = 100.0f;
    cameraRotation.z = 0.0f;
    cameraRotation2.x = -20.0f;
    cameraRotation2.y = 0.0f;
    cameraRotation2.z = 0.0f;
    BoardCameraMotionStartEx(statueModelId, &cameraRotation2, &cameraRotation, 1360.0f, -1.0f, 60);
    BoardCameraMotionWait();
    HuPrcSleep(0x3C);
}

void StatueReturnPlayer(s32 playerIndex)
{
    Vec spacePosition;
    Vec playerPosition;
    Vec positionDelta;
    f32 headingAngle;

    BoardSpacePosGet(0, GWPlayer[playerIndex].space_curr, &spacePosition);
    BoardPlayerPosGet(playerIndex, &playerPosition);
    VECSubtract(&spacePosition, &playerPosition, &positionDelta);
    VECNormalize(&positionDelta, &positionDelta);
    headingAngle = atan2d(-positionDelta.x, -positionDelta.z);
    BoardPlayerRotYSet(playerIndex, headingAngle);
    BoardCameraViewSet(1);
    BoardCameraTargetPlayerSet(playerIndex);
    BoardPlayerPosLerpStart(playerIndex, &playerPosition, &spacePosition, 0x28);

    while (GWPlayer[playerIndex].moving != 0) {
        HuPrcVSleep();
    }

    BoardCameraMotionWait();
    BoardModelMotionStart(boardModelIds[1], 0, 0);
    BoardPlayerIdleSet(playerIndex);
}

void StatueWaitForRewardAnimation(void)
{
    s16 integerValue;
    s16 animationFrame;
    s16 j;
    s16 integerValue2[3] = { 0x0002, 0x003E, 0x007A };

    integerValue = (4 - rewardAmount) * 60;
    BoardModelMotionStart(boardModelIds[1], 0, 0);

    while (1) {
        animationFrame = BoardModelMotionTimeGet(boardModelIds[1]);
        if (integerValue > animationFrame) {
            for (j = 0; j < ARRAY_COUNT(integerValue2); j++) {
                if (animationFrame == integerValue2[j]) {
                    HuAudFXPlay(0x446);
                }
            }
            HuPrcVSleep();
        }
        else {
            break;
        }
    }

    BoardModelAttrSet(boardModelIds[1], 0x40000002);
    HuPrcSleep(0x3C);
}

s32 StatueRewardTimerUpdate(s32 state)
{
    boardState->statueRewardCount--;

    if (boardState->statueRewardCount <= 0) {
        boardState->rewardPlayerMask = 0;
        boardState->statueRewardCount = 0;
        W03SetBridgeMotion(0);
        BoardWinCreate(0, MAKE_MESSID(22, 8), -1);
        BoardWinWait();
        BoardWinKill();
        W03UpdateJunctionLinks();
        return 1;
    }

    sprintf(rewardAmountText, "%d", boardState->statueRewardCount);
    BoardWinCreate(0, MAKE_MESSID(22, 7), -1);
    BoardWinInsertMesSet((s32)rewardAmountText, 1);
    BoardWinWait();
    BoardWinKill();
    W03UpdateJunctionLinks();
    return 0;
}

void StatueOverlayCreate(void)
{
    overlayOpacity = 0.0f;
    overlayTextureBuffer = HuMemDirectMallocNum(HEAP_HEAP, 0x96000, HU_MEMNUM_OVL);
    overlayHookId = Hu3DHookFuncCreate((void *)&StatueOverlayDraw);
    Hu3DModelLayerSet(overlayHookId, 3);
}

void StatueOverlayDestroy(void)
{
    if (overlayTextureBuffer) {
        HuMemDirectFree(overlayTextureBuffer);
        overlayTextureBuffer = NULL;
    }

    if (overlayHookId != -1) {
        Hu3DModelKill(overlayHookId);
        overlayHookId = -1;
    }
}

void StatueOverlayDraw(void)
{
    Mtx44 modelMatrix;
    Mtx modelMatrix2;
    GXColor color;
    GXColor color2;
    GXColor color3;
    u8 integerValue;
    GXColor color4 = { 0xFF, 0xFF, 0xFF, 0xFF };
    GXColor color5 = { 0xFF, 0xFF, 0xFF, 0xFF };

    f32 floatValue = 0.0f;
    f32 floatValue2 = 640.0f;
    f32 floatValue3 = 0.0f;
    f32 floatValue4 = 480.0f;

    OSf32tou8(&overlayOpacity, &color5.a);
    GXInvalidateTexAll();
    GXLoadTexObj(&overlayTexture, GX_TEXMAP0);
    C_MTXOrtho(modelMatrix, floatValue3, floatValue4, floatValue, floatValue2, 0.0f, 10.0f);
    GXSetProjection(modelMatrix, GX_ORTHOGRAPHIC);
    MTXIdentity(modelMatrix2);
    GXLoadPosMtxImm(modelMatrix2, 0);
    GXSetCurrentMtx(0);
    GXSetViewport(0.0f, 0.0f, floatValue2, 1.0f + floatValue4, 0.0f, 1.0f);
    GXSetScissor(0, 0, floatValue2, (1.0f + floatValue4));
    GXClearVtxDesc();
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetNumChans(1);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetChanCtrl(GX_COLOR0, 1, GX_SRC_REG, GX_SRC_REG, 1, GX_DF_CLAMP, GX_AF_NONE);
    GXSetChanCtrl(GX_ALPHA0, 0, GX_SRC_REG, GX_SRC_REG, 1, GX_DF_NONE, GX_AF_NONE);
    GXSetChanAmbColor(GX_COLOR0A0, color5);
    GXSetChanMatColor(GX_COLOR0A0, color5);
    GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_INVDSTCLR, GX_BL_SRCALPHA, GX_LO_NOOP);
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_RGBX8, 0);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_RGBA6, 0);
    GXSetZMode(1, GX_LEQUAL, 0);
    GXSetAlphaUpdate(1);
    GXSetColorUpdate(1);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition2u16(floatValue, floatValue3);
    GXTexCoord2f32(0.0f, 0.0f);
    GXPosition2u16(floatValue2, floatValue3);
    GXTexCoord2f32(1.0f, 0.0f);
    GXPosition2u16(floatValue2, floatValue4);
    GXTexCoord2f32(1.0f, 1.0f);
    GXPosition2u16(floatValue, floatValue4);
    GXTexCoord2f32(0.0f, 1.0f);
    GXEnd();
    GXSetChanMatColor(GX_COLOR0A0, color4);
}

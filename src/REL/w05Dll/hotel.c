#include "REL/w05Dll.h"

#include "game/hu3d.h"
#include "game/audio.h"
#include "game/pad.h"
#include "game/objsub.h"
#include "game/wipe.h"

#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/window.h"

#include "ext_math.h"

// data
s32 hotelLevelModels[5] = {
    DATA_MAKE_NUM(DATADIR_W05, 0x21), DATA_MAKE_NUM(DATADIR_W05, 0x22), DATA_MAKE_NUM(DATADIR_W05, 0x23),
    DATA_MAKE_NUM(DATADIR_W05, 0x24), DATA_MAKE_NUM(DATADIR_W05, 0x25),
};
s16 hotelModel = 0xFFFF;

// bss
s16 hotelPlayerIndex;

void W05HotelCreateLevelModel(void) {
    Vec position;
    HU3DMODEL* modelData;

    hotelModel = BoardModelCreate(hotelLevelModels[hotelBoardState->hotelLevel], NULL, 0);
    BoardModelAttrSet(hotelModel, 0x40000001);
    modelData = &Hu3DData[BoardModelIDGet(mainBoardModel)];
    Hu3DMotionExec(BoardModelIDGet(mainBoardModel), modelData->motId, 0.0f, 0);
    Hu3DModelObjPosGet(BoardModelIDGet(mainBoardModel), "hotel1", &position);
    BoardModelPosSetV(hotelModel, &position);
}

void W05HotelDestroyLevelModel(void) {
    BoardModelKill(hotelModel);
}

u32 hotelCoinThresholds[4] = { 0, 10, 25, 40 };
s32 hotelMotion = DATA_MAKE_NUM(DATADIR_W05, 0x0C);

s16 W05HotelGetLevelForCoinCount(void) {
    s16 loopIndex;
    
    for (loopIndex = 0; loopIndex < 4;loopIndex++) {
        if (hotelBoardState->coinsInvested <= hotelCoinThresholds[loopIndex]) {
            break;
        }
    }
    return loopIndex;
}

void W05HotelCollectToll(void) {
    Vec vector;
    Vec position;
    Vec position2;
    Vec movementVector;
    Vec movementVector2;
    Vec movementVector3;
    char investedCoinsText[16];
    Vec vector2;
    char contributionCoinsText[4];
    s32 messageId;
    HU3DMODEL* modelData;
    F32 playerFacingAngle;
    F32 hotelFacingAngle;
    s16 updatedHotelLevel;
    s16 modelId;
    s16 coinCount;
    u32 loopIndex;

    hotelPlayerIndex = GWSystem.player_curr;
    BoardRollDispSet(0);
    modelId = BoardModelMotionCreate(boardMapModelIds[4], hotelMotion);
    playerFacingAngle = BoardPlayerRotYGet(hotelPlayerIndex);
    BoardPlayerPosGet(hotelPlayerIndex, &position);
    Hu3DModelObjPosGet(BoardModelIDGet(mainBoardModel), "hotel1", &vector);
    hotelFacingAngle = atan2d(vector.x - position.x, vector.z - position.z);
    BoardPlayerMotBlendSet(hotelPlayerIndex, hotelFacingAngle, 0xF);
    BoardCameraViewSet(2);

    while (BoardPlayerMotBlendCheck(hotelPlayerIndex) == 0) {
        HuPrcVSleep();
    }
    BoardCameraMotionWait();
    BoardModelPosGet(boardMapModelIds[4], &position2);

    movementVector = movementVector3 = position2;
    if (position.x > movementVector3.x) {
        movementVector3.x += 650.0f;
        BoardModelRotYSet(boardMapModelIds[4], 90.0f);
    } else {
        movementVector3.x -= 650.0f;
        BoardModelRotYSet(boardMapModelIds[4], -90.0f);
    }
    BoardModelMotionStart(boardMapModelIds[4], modelId, 0x40000001);
    VECSubtract(&movementVector3, &movementVector, &movementVector2);
    VECScale(&movementVector2, &movementVector2, 0.018518519f);
    
    for (loopIndex = 0; loopIndex < 0x36; loopIndex++) {
        VECAdd(&movementVector, &movementVector2, &movementVector);
        BoardModelPosSetV(boardMapModelIds[4], &movementVector);
        HuPrcVSleep();
    }
    BoardModelPosSetV(boardMapModelIds[4], &movementVector3);
    hotelFacingAngle = atan2d(position.x - movementVector3.x, position.z - movementVector3.z);

    while (W05TurnBoardModelTowardAngle(boardMapModelIds[4], hotelFacingAngle, 10.0f) == 0) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(boardMapModelIds[4], 0, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    coinCount = BoardPlayerCoinsGet(hotelPlayerIndex);
    sprintf(investedCoinsText, "%d", hotelBoardState->coinsInvested);
    if (coinCount > 0) {
        if (coinCount >= 5) {
            messageId = MAKE_MESSID(0x27, 0x01);
            coinCount = 5;
        } else {
            messageId = MAKE_MESSID(0x27, 0x02);
        }
        sprintf(contributionCoinsText, "%d", coinCount);
        BoardWinCreate(2, messageId, 7);
        BoardWinInsertMesSet(MAKE_MESSID_PTR(contributionCoinsText), 1);
    } else {
        BoardWinCreate(2, MAKE_MESSID(39, 3), 7);
        BoardWinInsertMesSet(MAKE_MESSID_PTR(investedCoinsText), 0);
    }
    BoardWinWait();
    BoardWinKill();
    if (coinCount > 0) {
        for (loopIndex = 0; (s32) loopIndex < coinCount; loopIndex++) {
            BoardPlayerCoinsAdd(hotelPlayerIndex, -1);
            HuAudFXPlay(0xE);
            HuPrcSleep(6);
        }
        HuAudFXPlay(0xF);
        hotelBoardState->coinsInvested += coinCount;
        sprintf(investedCoinsText, "%d", hotelBoardState->coinsInvested);
        BoardWinCreate(2, MAKE_MESSID(39, 22), 7);
        BoardWinInsertMesSet(MAKE_MESSID_PTR(investedCoinsText), 0);
        BoardWinWait();
        BoardWinKill();
    }

    updatedHotelLevel = W05HotelGetLevelForCoinCount();
    if (updatedHotelLevel != hotelBoardState->hotelLevel) {
        hotelBoardState->hotelLevel = updatedHotelLevel;
        BoardCameraTargetModelSet(hotelModel);
        BoardCameraOffsetSet(0.0f, 200.0f, 0.0f);
        BoardCameraMotionWait();
        HuPrcSleep(60);
        WipeColorSet(0, 0, 0);
        WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 48);

        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        W05HotelDestroyLevelModel();
        W05HotelCreateLevelModel();
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 48);

        while (WipeStatGet() != 0) {
            HuPrcVSleep();
        }
        HuPrcSleep(60);
        BoardCameraTargetPlayerSet(hotelPlayerIndex);
        BoardCameraMotionWait();
    }
    BoardPlayerMotBlendSet(hotelPlayerIndex, playerFacingAngle, 0xF);
    BoardModelPosGet(boardMapModelIds[4], &movementVector);
    movementVector3 = position2;
    if (position.x > movementVector3.x) {
        BoardModelRotYSet(boardMapModelIds[4], -90.0f);
    } else {
        BoardModelRotYSet(boardMapModelIds[4], 90.0f);
    }
    BoardModelMotionStart(boardMapModelIds[4], modelId, 0x40000001);
    VECSubtract(&movementVector3, &movementVector, &movementVector2);
    VECScale(&movementVector2, &movementVector2, 0.033333335f);
    
    for (loopIndex = 0; loopIndex < 0x1E; loopIndex++) {
        VECAdd(&movementVector, &movementVector2, &movementVector);
        BoardModelPosSetV(boardMapModelIds[4], &movementVector);
        HuPrcVSleep();
    }
    BoardModelPosSetV(boardMapModelIds[4], &movementVector3);

    while (W05TurnBoardModelTowardAngle(boardMapModelIds[4], 0.0f, 15.0f) == 0) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(boardMapModelIds[4], 0, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    BoardCameraViewSet(1);

    while (BoardPlayerMotBlendCheck(hotelPlayerIndex) == 0) {
        HuPrcVSleep();
    }
    BoardCameraMotionWait();
    BoardModelMotionKill(boardMapModelIds[4], modelId);
    BoardRollDispSet(1);
}

s32 hotelUpgradeMotions[4] = {
    DATA_MAKE_NUM(DATADIR_W05, 0x0C), DATA_MAKE_NUM(DATADIR_W05, 0x0E),
    DATA_MAKE_NUM(DATADIR_W05, 0x0F),DATA_MAKE_NUM(DATADIR_W05, 0x10)
};
s32 hotelGuestMotions[2] = { 28, 25 };

void W05HotelUpgrade(void) {
    char investedCoinsText[0x10];
    char coinCountText[0x10];
    Vec position;
    F32 floatValue;
    F32 floatValue2;
    Vec movementVector;
    Vec position2;
    Vec movementVector2;
    Vec movementVector3;
    F32 floatValue3;
    F32 floatValue4;
    Vec movementVector4;
    s16 guestMotionIds[4][2];
    Vec position3;
    s16 hotelMotionIds[4];
    HU3DMODEL* modelData;
    F32 facingAngle;
    F32 floatValue6;
    s16 modelId;
    s16 paymentStepDelay;
    s16 integerValue6;
    s32 coinCount;
    s32 loopIndex;
    s32 soundHandle;
    s32 loopIndex2;

    hotelPlayerIndex = GWSystem.player_curr;
    BoardRollDispSet(0);
    
    for (loopIndex = 0; loopIndex < 4; loopIndex++) {
        hotelMotionIds[loopIndex] = BoardModelMotionCreate(boardMapModelIds[4], hotelUpgradeMotions[loopIndex]);
    }

    for (loopIndex = 0; loopIndex < 4; loopIndex++) {
        for (loopIndex2 = 0; loopIndex2 < 2; loopIndex2++) {
            guestMotionIds[loopIndex][loopIndex2] = BoardPlayerMotionCreate(loopIndex, hotelGuestMotions[loopIndex2] | characterIdleMotions[GWPlayer[loopIndex].character]);
        }
    }
    BoardCameraViewSet(2);
    BoardCameraMotionWait();
    BoardPlayerPosGet(hotelPlayerIndex, &position2);
    BoardModelPosGet(boardMapModelIds[4], &movementVector3);
    movementVector = movementVector4 = movementVector3;
    if (position2.x > movementVector4.x) {
        movementVector4.x += 520.0f;
        BoardModelRotYSet(boardMapModelIds[4], 90.0f);
    } else {
        movementVector4.x -= 520.0f;
        BoardModelRotYSet(boardMapModelIds[4], -90.0f);
    }
    BoardModelMotionStart(boardMapModelIds[4], hotelMotionIds[0], 0x40000001);
    VECSubtract(&movementVector4, &movementVector, &movementVector2);
    VECScale(&movementVector2, &movementVector2, 0.018518519f);

    for (loopIndex = 0; loopIndex < 0x36U; loopIndex++) {
        VECAdd(&movementVector, &movementVector2, &movementVector);
        BoardModelPosSetV(boardMapModelIds[4], &movementVector);
        HuPrcVSleep();
    }
    BoardModelPosSetV(boardMapModelIds[4], &movementVector4);
    facingAngle = atan2d(position2.x - movementVector4.x, position2.z - movementVector4.z);

    while (W05TurnBoardModelTowardAngle(boardMapModelIds[4], facingAngle, 10.0f) == 0) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(boardMapModelIds[4], 0, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    facingAngle = atan2d(movementVector4.x - position2.x, movementVector4.z - position2.z);
    BoardPlayerMotBlendSet(hotelPlayerIndex, facingAngle, 0xF);

    while (BoardPlayerMotBlendCheck(hotelPlayerIndex) == 0) {
        HuPrcVSleep();
    }
    if (hotelBoardState->coinsInvested == 0) {
        BoardWinCreate(2, MAKE_MESSID(39, 5), 7);
        BoardWinWait();
        BoardWinKill();
    } else {
        modelId = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x04), NULL, 0);
        BoardModelVisibilitySet(modelId, 0);
        BoardModelLayerSet(modelId, 2U);
        if (hotelBoardState->coinsInvested > BoardPlayerCoinsGet(hotelPlayerIndex)) {
            coinCount = BoardPlayerCoinsGet(hotelPlayerIndex);
        } else {
            coinCount = hotelBoardState->coinsInvested;
        }
        if (coinCount < 0x14) {
            paymentStepDelay = 6;
        } else if (coinCount < 0x32) {
            paymentStepDelay = 3;
        } else if (coinCount < 0x64) {
            paymentStepDelay = 2;
        } else {
            paymentStepDelay = 0;
        }
        sprintf(investedCoinsText, "%d", hotelBoardState->coinsInvested);
        sprintf(coinCountText, "%d", coinCount);
        if (coinCount == hotelBoardState->coinsInvested) {
            BoardWinCreate(2, MAKE_MESSID(39, 4), 7);
        } else {
            BoardWinCreate(2, MAKE_MESSID(39, 23), 7);
        }
        BoardWinInsertMesSet(MAKE_MESSID_PTR(investedCoinsText), 0);
        BoardWinInsertMesSet(MAKE_MESSID_PTR(coinCountText), 1);
        BoardWinWait();
        BoardWinKill();
        
        for (loopIndex = 0; loopIndex < coinCount; loopIndex++) {
            BoardPlayerCoinsAdd(hotelPlayerIndex, -1);
            HuAudFXPlay(0xE);
            HuPrcSleep(paymentStepDelay);
        }
        HuAudFXPlay(0xF);
        HuPrcSleep(0x1E);
        BoardCameraViewSet(1);
        BoardCameraTargetModelSet(hotelModel);
        BoardCameraMotionWait();
        HuPrcSleep(0x1E);
        omVibrate(hotelPlayerIndex, 0x7FFF, 0xC, 0);
        BoardCameraQuakeSet(0x8C, 250.0f);
        soundHandle = HuAudFXPlay(0x482);

        for (loopIndex = 0; loopIndex < 4; loopIndex++) {
            BoardPlayerMotBlendSet(loopIndex, 0, 0xF);
        }

        while (W05TurnBoardModelTowardAngle(boardMapModelIds[4], 0.0f, 15.0f) == 0) {
            HuPrcVSleep();
        }
        BoardModelMotionStart(boardMapModelIds[4], hotelMotionIds[1], 0U);

        for (loopIndex = 0; loopIndex < 4; loopIndex++) {
            BoardPlayerVoiceEnableSet(loopIndex, guestMotionIds[loopIndex][0], 0);
            BoardPlayerMotionStart(loopIndex, guestMotionIds[loopIndex][0], 0);
        }
        PlayerFXPlay(hotelPlayerIndex, 0x12E);
        HuPrcSleep(0x5A);
        HuAudFXStop(soundHandle);
        HuAudFXPlay(0x483);
        Hu3DModelObjPosGet(BoardModelIDGet(mainBoardModel), "tunami1", &position);
        BoardModelPosSetV(modelId, &position);
        BoardModelVisibilitySet(modelId, 1);
        BoardModelMotionTimeSet(modelId, 0.0f);
        HuPrcSleep(0x1E);
        hotelBoardState->coinsInvested = 0;
        
        hotelBoardState->hotelLevel = W05HotelGetLevelForCoinCount();
        BoardModelKill(hotelModel);
        hotelModel = BoardModelCreate(hotelLevelModels[hotelBoardState->hotelLevel], NULL, 0);
        BoardModelAttrSet(hotelModel, 0x40000001);
        modelData = &Hu3DData[BoardModelIDGet(mainBoardModel)];
        Hu3DMotionExec(BoardModelIDGet(mainBoardModel), modelData->motId, 0.0f, 0);
        Hu3DModelObjPosGet(BoardModelIDGet(mainBoardModel), "hotel1", &position3);
        BoardModelPosSetV(hotelModel, &position3);
        BoardModelMotionStart(boardMapModelIds[4], hotelMotionIds[2], 0x40000001U);
        
        for (loopIndex = 0; loopIndex < 4; loopIndex++) {
            BoardPlayerVoiceEnableSet(loopIndex, guestMotionIds[loopIndex][0], 1);
            if (loopIndex != hotelPlayerIndex) {
                BoardPlayerVoiceEnableSet(loopIndex, 6, 0);
            }
            BoardPlayerMotionStart(loopIndex, 6, 0x40000001);
        }
        while (BoardModelMotionTimeGet(modelId) < BoardModelMotionMaxTimeGet(modelId)) {
            HuPrcVSleep();
        }
        BoardModelKill(modelId);
        HuPadRumbleStop(GWPlayer[hotelPlayerIndex].port);
        BoardWinCreate(0, MAKE_MESSID(39, 6), -1);
        BoardWinWait();
        BoardWinKill();
        HuPrcSleep(0x1E);
        BoardModelMotionShiftSet(boardMapModelIds[4], hotelMotionIds[3], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
        
        for (loopIndex = 0; loopIndex < 4; loopIndex++) {
            BoardPlayerMotionShiftSet(loopIndex, guestMotionIds[loopIndex][1], 0.0f, 5.0f, HU3D_MOTATTR_NONE);
            BoardPlayerVoiceEnableSet(loopIndex, 6, 1);
        }
        HuPrcSleep(0x23);
        BoardModelMotionShiftSet(boardMapModelIds[4], 0, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        
        for (loopIndex = 0; loopIndex < 4; loopIndex++) {
            BoardPlayerMotionShiftSet(loopIndex, 1, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
        }
    }
    BoardCameraViewSet(1);
    BoardPlayerPosGet(hotelPlayerIndex, &position2);
    BoardModelPosGet(boardMapModelIds[4], &movementVector);
    movementVector4 = movementVector3;
    if (position2.x > movementVector4.x) {
        BoardModelRotYSet(boardMapModelIds[4], -90.0f);
    } else {
        BoardModelRotYSet(boardMapModelIds[4], 90.0f);
    }
    BoardModelMotionStart(boardMapModelIds[4], hotelMotionIds[0], 0x40000001U);
    VECSubtract(&movementVector3, &movementVector, &movementVector2);
    VECScale(&movementVector2, &movementVector2, 0.055555556);

    for (loopIndex = 0; loopIndex < 0x12U; loopIndex++) {
        VECAdd(&movementVector, &movementVector2, &movementVector);
        BoardModelPosSetV(boardMapModelIds[4], &movementVector);
        HuPrcVSleep();
    }
    BoardModelPosSetV(boardMapModelIds[4], &movementVector3);
    
    while (W05TurnBoardModelTowardAngle(boardMapModelIds[4], 0.0f, 15.0f) == 0) {
        HuPrcVSleep();
    }
    BoardModelMotionShiftSet(boardMapModelIds[4], 0, 0.0f, 5.0f, HU3D_MOTATTR_LOOP);
    
    for (loopIndex = 0; loopIndex < 4; loopIndex++) {
        BoardModelMotionKill(boardMapModelIds[4], hotelMotionIds[loopIndex]);
    }
    
    for (loopIndex = 0; loopIndex < 4; loopIndex++) {
        for (loopIndex2 = 0; loopIndex2 < 2; loopIndex2++) {
            BoardPlayerMotionKill(loopIndex, guestMotionIds[loopIndex][loopIndex2]);
        }
    }
    BoardCameraMotionWait();
    BoardPlayerMotBlendSet(hotelPlayerIndex, 0, 0xF);

    while (BoardPlayerMotBlendCheck(hotelPlayerIndex) == 0) {
        HuPrcVSleep();
    }
}

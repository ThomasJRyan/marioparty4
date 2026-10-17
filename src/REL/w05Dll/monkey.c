#include "REL/w05Dll.h"

#include "game/audio.h"
#include "game/hu3d.h"
#include "game/frand.h"
#include "game/objsub.h"

#include "game/board/model.h"
#include "game/board/player.h"
#include "game/board/window.h"

// data
s32 monkeyModelList[3] = { DATA_MAKE_NUM(DATADIR_W05, 0x1F), DATA_MAKE_NUM(DATADIR_W05, 0x20), DATA_NUM_LISTEND };
char *monkeyHookNames[2] = { "monkyA", "monkyB"};
s32 monkeyPlayerMotions[3] = { 0x78, 0x6F, 0x70 };

// bss
s16 monkeyModels[2];
s16 monkeyPlayerIndex;
s16 monkeyBoardModel;
s16 monkeySide;
s16 monkeyEffectModels[2];
Vec monkeyLandingPosition;
s16 monkeyPlayerMotionIds[4];

void W05CreateMonkeyModels(void) {
        s32 loopIndex;

    BoardModelAttrSet(boardMapModelIds[2], 0x40000002);
    BoardModelAttrSet(boardMapModelIds[3], 0x40000002);
    BoardModelMotionTimeSet(boardMapModelIds[2], 0.0f);
    BoardModelMotionTimeSet(boardMapModelIds[3], 0.0f);
    
    for (loopIndex = 0; loopIndex < 2; loopIndex++) {
        monkeyModels[loopIndex] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x1E), monkeyModelList, 0);
        BoardModelAttrSet(monkeyModels[loopIndex], 0x40000001);
    }
    BoardModelHookSet(boardMapModelIds[2], monkeyHookNames[0], monkeyModels[0]);
    BoardModelHookSet(boardMapModelIds[3], monkeyHookNames[1], monkeyModels[1]);
}

void W05RunMonkeyBoardEvent(s32 monkeySide) {
    Vec vector;
    s32 loopIndex;

    monkeyPlayerIndex = GWSystem.player_curr;
    monkeySide = monkeySide;
    if (monkeySide == 0) {
        monkeyBoardModel = boardMapModelIds[2];
        vector.x = 150.0f;
        BoardModelPosGet(monkeyBoardModel, &monkeyLandingPosition);
        monkeyLandingPosition.x += 140.0f;
    } else {
        monkeyBoardModel = boardMapModelIds[3];
        vector.x = -150.0f;
        BoardModelPosGet(monkeyBoardModel, &monkeyLandingPosition);
        monkeyLandingPosition.x -= 140.0f;
    }
    monkeyEffectModels[0] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x1C), NULL, 0);
    monkeyEffectModels[1] = BoardModelCreate(DATA_MAKE_NUM(DATADIR_W05, 0x1D), NULL, 0);
    BoardModelVisibilitySet(monkeyEffectModels[0], 0);
    BoardModelVisibilitySet(monkeyEffectModels[1], 0);
    BoardModelAttrSet(monkeyEffectModels[0], 0x40000001);
    
    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        monkeyPlayerMotionIds[loopIndex] = BoardPlayerMotionCreate(monkeyPlayerIndex, monkeyPlayerMotions[loopIndex] | characterIdleMotions[GWPlayer[monkeyPlayerIndex].character]);
    }
    BoardRollDispSet(0);
    BoardPlayerIdleSet(monkeyPlayerIndex);
    vector.y = 100.0f;
    vector.z = 0.0f;
    BoardCameraMotionStartEx(monkeyBoardModel, NULL, &vector, 2300.0f, -1.0f, 0x1E);
    BoardCameraRotSet(-33.0f, 0.0f);
    BoardCameraMotionWait();
    HuAudFXPlay(0x48A);
    BoardWinCreate(0, MAKE_MESSID(39, 24), -1);
    BoardWinWait();
    BoardWinKill();
    W05AnimateMonkeyDrop();
    W05SetMonkeyPlayerAnimation();
    if (GWPlayer[monkeyPlayerIndex].bowser_suit != 0) {
        W05MoveBowserPlayerToNextSpace();
    } else {
        W05AnimateMonkeyPlayerDrop();
    }
    BoardPlayerIdleSet(monkeyPlayerIndex);
    BoardCameraViewSet(1);
    BoardCameraMotionWait();
    BoardRollDispSet(1);
    
    for (loopIndex = 0; loopIndex < 2; loopIndex++) {
        BoardModelKill(monkeyEffectModels[loopIndex]);
    }
    
    for (loopIndex = 0; loopIndex < 3; loopIndex++) {
        BoardPlayerMotionKill(monkeyPlayerIndex, monkeyPlayerMotionIds[loopIndex]);
    }
}

void W05AnimateMonkeyDrop(void) {
    Vec movementVector;
    Vec movementVector2;
    Vec movementVector3;
    F32 floatValue;
    F32 floatValue2;
    F32 floatValue3;
    F32 frameCount;

    HuPrcSleep(0x1E);
    BoardModelMotionStart(monkeyModels[monkeySide], 1, 0);
    BoardModelAttrReset(monkeyBoardModel, 0x40000002);
    BoardModelMotionTimeSet(monkeyBoardModel, 0.0f);
    HuPrcSleep(0x88);
    Hu3DModelObjPosGet(BoardModelIDGet(monkeyBoardModel), monkeyHookNames[monkeySide], &movementVector);
    Hu3DModelObjPosGet(BoardModelIDGet(monkeyModels[monkeySide]), "bakawa1", &movementVector2);
    VECAdd(&movementVector, &movementVector2, &movementVector2);
    BoardModelPosSetV(monkeyEffectModels[0], &movementVector2);
    BoardModelVisibilitySet(monkeyEffectModels[0], 1);
    BoardModelMotionTimeSet(monkeyEffectModels[0], 0.0f);
    HuAudFXPlay(0x48C);
    VECSubtract(&monkeyLandingPosition, &movementVector2, &movementVector3);
    movementVector3.y = 0.0f;
    VECNormalize(&movementVector3, &movementVector3);
    VECScale(&movementVector3, &movementVector3, 8.0f);
    floatValue = 11.0f;
    frameCount = 0.0f;
    
    while (1) {
        movementVector3.y = (floatValue - (0.075 * (0.25f * (frameCount * frameCount))));
        frameCount += 1.0f;
        VECAdd(&movementVector2, &movementVector3, &movementVector2);
        VECSubtract(&monkeyLandingPosition, &movementVector2, &movementVector);
        movementVector.y = 0.0f;
        floatValue2 = VECMag(&movementVector);
        if (floatValue2 <= 8.0f) {
            movementVector2.x = monkeyLandingPosition.x;
            movementVector2.z = monkeyLandingPosition.z;
            movementVector3.x = movementVector3.z = 0.0f;
        }
        if (movementVector2.y <= monkeyLandingPosition.y) {
            movementVector2.y = monkeyLandingPosition.y;
            BoardModelVisibilitySet(monkeyEffectModels[0], 0);
            BoardModelVisibilitySet(monkeyEffectModels[1], 1);
            BoardModelPosSetV(monkeyEffectModels[1], &movementVector2);
            BoardModelMotionTimeSet(monkeyEffectModels[1], 0.0f);
            break;
        } else {
            BoardModelPosSetV(monkeyEffectModels[0], &movementVector2);
            if (BoardModelMotionTimeGet(monkeyModels[monkeySide]) >= BoardModelMotionMaxTimeGet(monkeyModels[monkeySide])) {
                BoardModelMotionStart(monkeyModels[monkeySide], 0, 0x40000001);
            }
            HuPrcVSleep();
        }
    }
}

void W05SetMonkeyPlayerAnimation(void) {
    if (GWPlayer[monkeyPlayerIndex].bowser_suit == 0) {
        BoardPlayerMotionShiftSet(monkeyPlayerIndex, monkeyPlayerMotionIds[2], 0.0f, 8.0f, HU3D_MOTATTR_NONE);
        HuPrcSleep(8);

        while (BoardPlayerMotionEndCheck(monkeyPlayerIndex) == 0) {
            HuPrcVSleep();
        }
        BoardPlayerMotionStart(monkeyPlayerIndex, monkeyPlayerMotionIds[1], 0x40000001);
        HuPrcSleep(0x5A);
        return;
    }
    HuPrcSleep(0x2D);
}

void W05AnimateMonkeyPlayerDrop(void) {
    Vec movementVector;
    Vec movementVector2;
    Vec movementVector3;
    Vec movementVector4;
    F32 floatValue;
    F32 floatValue2;
    F32 frameCount;
    s32 spaceIndex;
    s32 playerIndex;

    BoardPlayerPosGet(monkeyPlayerIndex, &movementVector);
    BoardPlayerMotionShiftSet(monkeyPlayerIndex, 4, 0.0f, 5.0f, HU3D_MOTATTR_NONE);
    VECSubtract(&monkeyLandingPosition, &movementVector, &movementVector2);
    movementVector2.y = 0.0f;
    VECNormalize(&movementVector2, &movementVector2);
    VECScale(&movementVector2, &movementVector2, 7.2f);
    floatValue2 = 10.0f;
    frameCount = 0.0f;
    while (1) {
        movementVector2.y = (floatValue2 - (0.075f * (0.25f * (frameCount * frameCount))));
        frameCount += 1.0f;
        VECAdd(&movementVector, &movementVector2, &movementVector);
        VECSubtract(&monkeyLandingPosition, &movementVector, &movementVector3);
        movementVector3.y = 0.0f;
        floatValue = VECMag(&movementVector3);
        if (floatValue <= 7.2f) {
            movementVector.x = monkeyLandingPosition.x;
            movementVector.z = monkeyLandingPosition.z;
            movementVector2.x = movementVector2.z = 0.0f;
        }
        if (movementVector.y <= monkeyLandingPosition.y) {
            movementVector.y = monkeyLandingPosition.y;
            BoardPlayerMotionStart(monkeyPlayerIndex, monkeyPlayerMotionIds[0], 0);
            break;
        } else {
            BoardPlayerPosSetV(monkeyPlayerIndex, &movementVector);
            HuPrcVSleep();
        }
    }
    HuAudFXPlay(0x48B);
    PlayerFXPlay(monkeyPlayerIndex, 0x123);
    HuPrcChildCreate(W05UpdateMonkeyEffects, 0x2001, 0x1000, 0, boardMainProc);
    HuAudFXPlay(0x48A);
    BoardModelMotionStart(monkeyModels[monkeySide], 2, 0);
    playerIndex = BoardSpaceLinkFlagSearch(0, GWPlayer[monkeyPlayerIndex].space_curr, 0x100);
    if (frandBool() != 0) {
        playerIndex = BoardSpaceLinkFlagSearch(0, playerIndex, 0x200);
    } else {
        playerIndex = BoardSpaceLinkFlagSearch(0, playerIndex, 0x400);
    }
    BoardSpacePosGet(0, playerIndex, &movementVector4);
    VECSubtract(&movementVector4, &movementVector, &movementVector2);
    movementVector2.y = 0.0f;
    VECNormalize(&movementVector2, &movementVector2);
    VECScale(&movementVector2, &movementVector2, 10.0f);
    floatValue2 = 9.0f;
    frameCount = 0.0f;
    while (1) {
        movementVector2.y = floatValue2 - (0.075f * (0.25f * (frameCount * frameCount)));
        frameCount += 1.0f;
        VECAdd(&movementVector, &movementVector2, &movementVector);
        VECSubtract(&movementVector4, &movementVector, &movementVector3);
        movementVector3.y = 0.0f;
        floatValue = VECMag(&movementVector3);
        if (floatValue <= 10.0f) {
            movementVector.x = movementVector4.x;
            movementVector.z = movementVector4.z;
            movementVector2.x = movementVector2.z = 0.0f;
        }
        if (movementVector.y <= movementVector4.y) {
            movementVector.y = movementVector4.y;
            omVibrate(monkeyPlayerIndex, 0xC, 4, 2);
            break;
        }
        BoardPlayerPosSetV(monkeyPlayerIndex, &movementVector);
        if (BoardModelMotionTimeGet(monkeyModels[monkeySide]) >= BoardModelMotionMaxTimeGet(monkeyModels[monkeySide])) {
            BoardModelMotionStart(monkeyModels[monkeySide], 0, 0x40000001);
        }
        HuPrcVSleep();
    }
    floatValue2 = 10.0f;
    frameCount = 0.0f;
    if (BoardPlayerSizeGet(monkeyPlayerIndex) == 2) {
        HuAudFXPlay(0x490);
    } else if (BoardPlayerSizeGet(monkeyPlayerIndex) == 1) {
        HuAudFXPlay(0x48F);
    } else {
        PlayerFXPlay(monkeyPlayerIndex, 0x119);
    }
    while (1) {
        movementVector2.y = floatValue2 - (0.18333334f * (0.25f * (frameCount * frameCount)));
        frameCount += 1.0f;
        movementVector.y += movementVector2.y;
        if (movementVector.y <= movementVector4.y) {
            floatValue2 = 0.25f * -movementVector2.y;
            frameCount = 0.0f;
            movementVector.y = movementVector4.y;
            if (BoardPlayerSizeGet(monkeyPlayerIndex) == 2) {
                OSReport("big se\n");
                HuAudFXPlay(0x490);
            } else {
                if (BoardPlayerSizeGet(monkeyPlayerIndex) == 1) {
                    OSReport("small se\n");
                    HuAudFXPlay(0x48F);
                } else {
                    OSReport("normal se\n");
                    PlayerFXPlay(monkeyPlayerIndex, 0x119);
                }
            }
        }
        BoardPlayerPosSetV(monkeyPlayerIndex, &movementVector);
        if (!(fabs(floatValue2) < 5.0)) {
            if (BoardModelMotionTimeGet(monkeyModels[monkeySide]) >= BoardModelMotionMaxTimeGet(monkeyModels[monkeySide])) {
                BoardModelMotionStart(monkeyModels[monkeySide], 0, 0x40000001U);
            }
            HuPrcVSleep();
        } else {
            BoardPlayerMotionShiftSet(monkeyPlayerIndex, 6, 0.0f, 8.0f, HU3D_MOTATTR_LOOP);
            break;
        }
    }

    spaceIndex = 0x44;
    while (spaceIndex--, spaceIndex != 0) {
        if (BoardModelMotionTimeGet(monkeyModels[monkeySide]) >= BoardModelMotionMaxTimeGet(monkeyModels[monkeySide])) {
            BoardModelMotionStart(monkeyModels[monkeySide], 0, 0x40000001U);
        }
        HuPrcVSleep();
    }
    GWPlayer[monkeyPlayerIndex].space_curr = playerIndex;
}

void W05MoveBowserPlayerToNextSpace(void) {
    s32 playerIndex;

    playerIndex = BoardSpaceLinkFlagSearch(0, GWPlayer[monkeyPlayerIndex].space_curr, 0x100U);
    BoardPlayerMoveBetween(monkeyPlayerIndex, GWPlayer[monkeyPlayerIndex].space_curr, playerIndex);
    GWPlayer[monkeyPlayerIndex].space_curr = playerIndex;
    HuAudFXPlay(0x48B);
    HuPrcChildCreate(W05UpdateMonkeyEffects, 0x2001U, 0x1000U, 0, boardMainProc);
    BoardPlayerIdleSet(monkeyPlayerIndex);
    BoardWinCreate(0, MAKE_MESSID(39, 25), -1);
    BoardWinWait();
    BoardWinKill();
}

void W05UpdateMonkeyEffects(void) {
    Vec position;
    Vec vector;
    s32 i;

    BoardModelPosGet(monkeyEffectModels[1], &position);
    vector.z = 0.0f;
    vector.y = 10.0f;
    if (monkeySide == 0) {
        vector.x = -25.0f;
    } else {
        vector.x = 25.0f;
    }
    for (i = 0; i < 60; i++) {
        VECAdd(&position, &vector, &position);
        vector.y -= 0.625f;
        BoardModelPosSetV(monkeyEffectModels[1], &position);
        HuPrcVSleep();        
    }
    HuPrcEnd();
}

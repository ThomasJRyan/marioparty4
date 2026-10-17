#include "game/audio.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/window.h"
#include "game/wipe.h"

#include "REL/modeseldll.h"
#include "ext_math.h"

#ifndef __MWERKS__
#include "game/esprite.h"
#include "game/thpmain.h"
#endif

s16 specialModeModelIndices[] = { 16, 17, 18, 19, 20, 21 };

s32 specialModeSelectedAnimations[] = { DATA_MAKE_NUM(DATADIR_MODESEL, 0x30), DATA_MAKE_NUM(DATADIR_MODESEL, 0x32), DATA_MAKE_NUM(DATADIR_MODESEL, 0x34),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x36), DATA_MAKE_NUM(DATADIR_MODESEL, 0x38), DATA_MAKE_NUM(DATADIR_MODESEL, 0x3A) };

s32 specialModeUnselectedAnimations[] = { DATA_MAKE_NUM(DATADIR_MODESEL, 0x31), DATA_MAKE_NUM(DATADIR_MODESEL, 0x33), DATA_MAKE_NUM(DATADIR_MODESEL, 0x35),
    DATA_MAKE_NUM(DATADIR_MODESEL, 0x37), DATA_MAKE_NUM(DATADIR_MODESEL, 0x39), DATA_MAKE_NUM(DATADIR_MODESEL, 0x3B) };

u32 specialModeDescriptionMessages[] = { MAKE_MESSID(17, 1), MAKE_MESSID(17, 2), MAKE_MESSID(17, 3), MAKE_MESSID(17, 4), MAKE_MESSID(17, 5), MAKE_MESSID(17, 6) };

s16 modeDescriptionWindowId;
s16 selectedSpecialMode;

void InitializeModeSelectionShadow(void);

// Scratch is at https://decomp.me/scratch/iirXp
// Register allocation issues at line 180 with load of 1.0 double constant
s32 SelectSpecialMode(void)
{
    float descriptionWindowDimensions[2];
    float specialModeWindowDimensions[2];
    float entranceProgress;
    float movieScrollSpeed;
    s16 animationFrame;
    s16 selectionChange;
    s16 specialModeWindowId;
    s16 modeIndex;
    s16 movieSpriteGroup;
    s16 previousModeIndex;
    s32 introSoundHandle;
    WindowData *descriptionWindowData;
    ANIMDATA *selectedModeAnimation;
    ANIMDATA *unselectedModeAnimation;
    s16 movieSpriteId;
    InitializeModeSelectionShadow();
    SetModeSelectionCamera(1);
    introSoundHandle = HuAudFXPlay(2094);
    if (omovlevtno) {
        WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 30);
    }
    for (animationFrame = 0; animationFrame <= 20; animationFrame++) {
        entranceProgress = 1.0 - sind(90.0 * (animationFrame / 20.0));
        Hu3DModelPosSet(modeSelectModelIds[14], 0, -50.0f + (200.0f * entranceProgress), 0);
        Hu3DModelPosSet(modeSelectModelIds[15], 0, -50.0f + (200.0f * entranceProgress), 0);
        HuPrcVSleep();
    }
    HuPrcSleep(20);
    Hu3DModelAttrReset(modeSelectModelIds[14], HU3D_MOTATTR_PAUSE);
    Hu3DModelAttrReset(modeSelectModelIds[15], HU3D_MOTATTR_PAUSE);
    HuAudFXPlay(2093);
    HuPrcSleep(120);
    HuAudFXPlay(2095);
    for (animationFrame = 0; animationFrame <= 30; animationFrame++) {
        entranceProgress = animationFrame / 30.0;
        Hu3DModelPosSet(modeSelectModelIds[14], 0, -50.0f - (20.0f * entranceProgress), 0);
        HuPrcVSleep();
    }
    HuWinMesMaxSizeBetGet(descriptionWindowDimensions, MAKE_MESSID(0x11, 0x00), MAKE_MESSID(0x11, 0x06));
    modeDescriptionWindowId = HuWinExCreateStyled(-10000, 432 - descriptionWindowDimensions[1], descriptionWindowDimensions[0], descriptionWindowDimensions[1], -1, 0);
    HuWinMesSpeedSet(modeDescriptionWindowId, 0);
    descriptionWindowData = &winData[modeDescriptionWindowId];
    descriptionWindowData->active_pad = 1;
    HuWinExAnimIn(modeDescriptionWindowId);
    HuWinMesSet(modeDescriptionWindowId, MAKE_MESSID(0x11, 0x00));
    HuWinMesMaxSizeGet(1, specialModeWindowDimensions, MAKE_MESSID(0x1A, 0x20));
    specialModeWindowId = HuWinCreate(-10000, 8 + (432 - descriptionWindowDimensions[1] - specialModeWindowDimensions[1]), specialModeWindowDimensions[0], specialModeWindowDimensions[1], 0);
    HuWinMesSpeedSet(specialModeWindowId, 0);
    HuWinBGTPLvlSet(specialModeWindowId, 0);
    HuWinMesSet(specialModeWindowId, MAKE_MESSID(0x1A, 0x20));
    HuWinDispOff(specialModeWindowId);
    while (!Hu3DMotionEndCheck(modeSelectModelIds[15])) {
        HuPrcVSleep();
    }
    Hu3DModelAttrSet(modeSelectModelIds[15], HU3D_ATTR_DISPOFF);
    for (animationFrame = 0; animationFrame < 6; animationFrame++) {
        Hu3DModelAttrReset(modeSelectModelIds[specialModeModelIndices[animationFrame]], HU3D_ATTR_DISPOFF);
        Hu3DModelAttrSet(modeSelectModelIds[specialModeModelIndices[animationFrame]], HU3D_MOTATTR_PAUSE);
        Hu3DMotionTimeSet(modeSelectModelIds[specialModeModelIndices[animationFrame]], 0);
        Hu3DModelPosSet(modeSelectModelIds[specialModeModelIndices[animationFrame]], 0, -50, 0);
    }
    HuWinMesWait(modeDescriptionWindowId);

    previousModeIndex = selectedSpecialMode = 0;
    espAttrReset(modeSelectSpriteIds[10], HUSPR_ATTR_DISPOFF);
    espBankSet(modeSelectSpriteIds[10], 0);
    for (animationFrame = 0; animationFrame <= 10; animationFrame++) {
        entranceProgress = animationFrame / 10.0f;
        Hu3DModelPosSet(modeSelectModelIds[specialModeModelIndices[selectedSpecialMode]], 0, -50, 50.0 * sind(90.0f * entranceProgress));
        espPosSet(modeSelectSpriteIds[10], 288.0, -50.0 + (106.0 * sind(90.0f * entranceProgress)));
        HuPrcVSleep();
    }
    HuWinMesSet(modeDescriptionWindowId, specialModeDescriptionMessages[selectedSpecialMode]);
    HuWinDispOn(specialModeWindowId);
    while (1) {
        selectionChange = 0;
        if (HuPadDStkRep[0] & PAD_BUTTON_LEFT) {
            selectionChange = -1;
        }
        else if (HuPadDStkRep[0] & PAD_BUTTON_RIGHT) {
            selectionChange = 1;
        }
        if (selectionChange) {
            if (selectedSpecialMode + selectionChange < 0) {
                selectedSpecialMode = 0;
            }
            else if (selectedSpecialMode + selectionChange >= 6) {
                selectedSpecialMode = 5;
            }
            else {
                selectedSpecialMode += selectionChange;
            }
            if (previousModeIndex != selectedSpecialMode) {
                HuAudFXPlay(0);
                for (animationFrame = 0; animationFrame <= 10; animationFrame++) {
                    entranceProgress = animationFrame / 10.0f;
                    Hu3DModelPosSet(modeSelectModelIds[specialModeModelIndices[previousModeIndex]], 0, -50, 50.0 * cosd(90.0f * entranceProgress));
                    Hu3DModelPosSet(modeSelectModelIds[specialModeModelIndices[selectedSpecialMode]], 0, -50, 50.0 * sind(90.0f * entranceProgress));
                    espPosSet(modeSelectSpriteIds[10], 288.0, -50.0 + (106.0 * (1.0 - sind(180.0f * entranceProgress))));
                    if (animationFrame == 5) {
                        espBankSet(modeSelectSpriteIds[10], selectedSpecialMode);
                    }
                    HuPrcVSleep();
                }
                previousModeIndex = selectedSpecialMode;
                HuWinMesSet(modeDescriptionWindowId, specialModeDescriptionMessages[selectedSpecialMode]);
            }
        }
        if (HuPadBtnDown[0] & PAD_BUTTON_A) {
            HuAudFXPlay(2);
            break;
        }
        else if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            HuAudFXPlay(3);
            HuAudFXStop(introSoundHandle);
            return -1;
        }
        else {
            HuPrcVSleep();
            continue;
        }
    }
    Hu3DModelAttrReset(modeSelectModelIds[specialModeModelIndices[selectedSpecialMode]], HU3D_MOTATTR_PAUSE);
    HuAudSeqAllFadeOut(1000);
    for (animationFrame = 0; animationFrame <= 20; animationFrame++) {
        entranceProgress = animationFrame / 20.0f;
        for (modeIndex = 0; modeIndex < 6; modeIndex++) {
            if (modeIndex != selectedSpecialMode) {
                Hu3DModelPosSet(modeSelectModelIds[specialModeModelIndices[modeIndex]], 0, -50 - (300.0 * (1.0 - cosd(90.0f * entranceProgress))), 0);
                espPosSet(modeSelectSpriteIds[10], 288.0, -50.0 + (106.0 * (cosd(90.0f * entranceProgress))));
            }
        }
        HuPrcVSleep();
    }
    HuWinKill(specialModeWindowId);
    HuWinExAnimOut(modeDescriptionWindowId);
    HuAudFXStop(introSoundHandle);
    movieSpriteGroup = HuSprGrpCreate(1);
    movieSpriteId = HuTHPSprCreateVol("movie/opmov_c00.thp", 0, 3000, 90.0);
    HuSprGrpMemberSet(movieSpriteGroup, 0, movieSpriteId);
    HuSprPosSet(movieSpriteGroup, 0, 288.0f, 240.0f);
    HuSprDrawNoSet(movieSpriteGroup, 0, 127);
    HuPrcSleep(2);
    espAttrSet(modeSelectSpriteIds[9], HUSPR_ATTR_DISPOFF);
    for (animationFrame = 0; animationFrame <= 10; animationFrame++) {
        entranceProgress = animationFrame / 10.0;
        Hu3DModelPosSet(modeSelectModelIds[14], 0, -70.0f - (100.0f * entranceProgress), 0);
        HuPrcVSleep();
    }
    selectedModeAnimation = HuSprAnimRead(HuDataReadNum(specialModeSelectedAnimations[selectedSpecialMode], HU_MEMNUM_OVL));
    unselectedModeAnimation = HuSprAnimRead(HuDataReadNum(specialModeUnselectedAnimations[selectedSpecialMode], HU_MEMNUM_OVL));
    Hu3DAnimCreate(selectedModeAnimation, modeSelectModelIds[22], "ys002_a1");
    Hu3DAnimCreate(unselectedModeAnimation, modeSelectModelIds[22], "ys002_a2");
    Hu3DModelAttrSet(modeSelectModelIds[specialModeModelIndices[selectedSpecialMode]], HU3D_ATTR_DISPOFF);
    Hu3DMotionTimeSet(modeSelectModelIds[22], 0);
    Hu3DModelAttrReset(modeSelectModelIds[22], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(modeSelectModelIds[22], HU3D_MOTATTR_PAUSE);
    Hu3DModelPosSet(modeSelectModelIds[22], 0, 27, 50);
    Hu3DModelScaleSet(modeSelectModelIds[22], 1.0f, 1.063f, 1.0f);
    for (animationFrame = 1; animationFrame <= 20; animationFrame++) {
        entranceProgress = animationFrame / 20.0;
        Hu3DModelPosSet(modeSelectModelIds[22], 0, 27.0 - (60.0 * sind(90.0f * entranceProgress)), 50);
        HuPrcVSleep();
    }
    HuPrcSleep(90);
    Hu3DFogSet(3000, 8000, 255, 255, 255);
    Hu3DModelAttrReset(modeSelectModelIds[22], HU3D_MOTATTR_PAUSE);
    HuPrcSleep(60);
    entranceProgress = 0;
    movieScrollSpeed = 5;
    while (!HuTHPEndCheck()) {
        Hu3DModelPosSet(modeSelectModelIds[22], 0, -33.0f + (entranceProgress / 30.0f), 50.0f - entranceProgress);
        entranceProgress += movieScrollSpeed;
        movieScrollSpeed += 5.0f;
        HuPrcVSleep();
    }
    _ClearFlag(FLAG_ID_MAKE(1, 11));
    WipeColorSet(255, 255, 255);
    #if VERSION_PAL
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 30);
    #else
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 10);
    #endif
    while (WipeStatGet()) {
        HuPrcVSleep();
    }
    HuTHPClose();
    HuSprGrpKill(movieSpriteGroup);
    return selectedSpecialMode;
}

void InitializeModeSelectionShadow(void)
{
    Vec shadowPosition;
    Vec shadowTarget;
    Vec shadowUp;
    Hu3DShadowCreate(20, 100, 5000);
    Hu3DShadowTPLvlSet(0.3f);
    Hu3DShadowSizeSet(192);
    shadowTarget.x = shadowTarget.y = shadowTarget.z = 0;
    shadowPosition.x = 0;
    shadowPosition.z = 300;
    shadowPosition.y = 400;
    shadowUp.x = 0;
    shadowUp.y = 1;
    shadowUp.z = 0;
    Hu3DShadowPosSet(&shadowPosition, &shadowUp, &shadowTarget);
    Hu3DModelAttrReset(modeSelectModelIds[15], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(modeSelectModelIds[15], HU3D_MOTATTR_PAUSE);
    Hu3DMotionTimeSet(modeSelectModelIds[15], 0);
    Hu3DModelAttrReset(modeSelectModelIds[14], HU3D_ATTR_DISPOFF);
    Hu3DModelAttrSet(modeSelectModelIds[14], HU3D_MOTATTR_PAUSE);
    Hu3DMotionTimeSet(modeSelectModelIds[14], 0);
}

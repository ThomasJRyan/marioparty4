#include "REL/mgmodedll.h"
#include "game/audio.h"
#include "game/disp.h"
#ifndef __MWERKS__
#include "game/board/ui.h"
#endif
#include "game/esprite.h"
#ifndef __MWERKS__
#include "game/gamework.h"
#endif
#include "game/gamework_data.h"
#include "game/saveload.h"
#include "game/window.h"
#include "game/wipe.h"

#include "ext_math.h"
#include <string.h>

#ifndef __MWERKS__
#include "game/hsfex.h"
#endif

typedef struct CameraViewParams {
    Vec rot;
    Vec pos;
    float zoom;
} CameraViewParams;

CameraViewParams modeCameraPresets[1] = { { { -5, 0, 0 }, { 0, 125, 0 }, 1750 } };

OMOBJ *outViewObj;
HUPROCESS *objman;
s32 modeSelectionState;
s32 modeSelectionActive;
s16 modeSelectionMessageWindow;
s16 modeSelectionInstructionWindow;
Vec modeCameraRotation;
Vec modeCameraPosition;
float modeCameraZoom;
s16 modeSelectionResults[4];
s16 freePlayModeType;
s16 freePlayModeSelection;
Vec modeCameraRotationTargets[2];
Vec modeCameraPositionTargets[2];
float modeCameraZoomTargets[2];
MinigameSharedState minigameSharedState;
s16 mgModeEvtNo;
s32 modeSelectionReservedState;

void RunMinigameModeTask(void);
void UpdateModeSelectionCamera(void);

void ObjectSetup(void)
{
    s32 glight;

    OSReport("******* MG ObjectSetup *********\n");
    objman = omInitObjMan(50, 8192);
    modeCameraRotationTargets[0].x = -70;
    modeCameraRotationTargets[0].y = 0;
    modeCameraRotationTargets[0].z = 0;
    modeCameraPositionTargets[0].x = 0;
    modeCameraPositionTargets[0].y = 130;
    modeCameraPositionTargets[0].z = -186;
    modeCameraZoomTargets[0] = 1360;
    modeCameraRotation = modeCameraRotationTargets[0];
    modeCameraPosition = modeCameraPositionTargets[0];
    modeCameraZoom = modeCameraZoomTargets[0];
    modeSelectionActive = 0;
    _ClearFlag(0x10000);
    _ClearFlag(0x10008);
    GWSystem.mg_type = -1;
    GWSystem.player_curr = 0;
    mgModeEvtNo = omovlevtno;
    if (mgModeEvtNo == 0) {
        mgGameStatBackup = GWGameStat;
    }
    modeSelectionMessageWindow = -1;
    mgBoardHostEnableF = 1;
    mgPracticeEnableF = 0;
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 42, 20, 15000, 1.2f);
    Hu3DCameraViewportSet(1, 0, 0, 640, 480, 0, 1);
    glight = Hu3DGLightCreate(0, 100, 1000, 0, -0.5, -1, 255, 255, 255);
    Hu3DGLightInfinitytSet(glight);
    HuPrcChildCreate(RunMinigameModeTask, 100, 12288, 0, objman);
    HuPrcChildCreate(UpdateModeSelectionCamera, 200, 4096, 0, objman);
    outViewObj = omAddObjEx(objman, 32730, 0, 0, -1, omOutView);
    Hu3DBGColorSet(0, 0, 0);
    HuWinInit(1);
    BuildMinigameTypeLists(0);
}

void InitializeModeSelectionModels(void);

void RunModeSelectionMenu(void);

void RunMinigameModeTask(void)
{
    s16 i;
    InitializeModeSelectionModels();
    if (mgModeEvtNo == 0) {
        HuAudSStreamPlay(12);
        PlayMinigameModeOpening();
    }
    else {
        HuAudSeqPlay(44);
        for (i = 0; i < 4; i++) {
            minigameSharedState.playerStates[i].character = GWPlayerCfg[i].character;
            minigameSharedState.playerStates[i].isComputer = GWPlayerCfg[i].iscom;
            minigameSharedState.playerStates[i].controllerPort = GWPlayerCfg[i].pad_idx;
            minigameSharedState.playerStates[i].difficulty = GWPlayerCfg[i].diff;
        }
        CZoom = 1600;
        Center.x = Center.z = 0;
        Center.y = 215;
        CRot.x = 0;
        CRot.y = CRot.z = 0;
        if (mgModeEvtNo == 1 || mgModeEvtNo == 2 || mgModeEvtNo == 3 || mgModeEvtNo == 4) {
            Hu3DModelPosSet(mgModeModelIds[5], 0, 0, 640);
            Hu3DMotionSet(mgModeModelIds[5], mgModeMotionIds[6]);
            Hu3DModelAttrSet(mgModeModelIds[5], HU3D_MOTATTR_LOOP);
            HuPrcVSleep();
        }
    }
    RunModeSelectionMenu();
    ShowMessageChoiceWindow(0x1E0035, 3, 0);
    CloseStandardMessageWindow(windowId);
    while (1) {
        HuPrcVSleep();
    }
}

Vec modeCameraOffsetPresets[4] = {
    { 108, 190, 0 },
    { 228, 190, 0 },
    { 348, 190, 0 },
    { 468, 190, 0 },
};

void ShowModeSelectionSubmenu(void);
void ShowModeSelectionHelp(void);
void HideModeSelectionHelp(void);

s32 GetSelectedMode(void);
void ApplySelectedMode(void);

void RunModeSelectionMenu(void)
{
    s16 i;
    s16 time;
    if (mgModeEvtNo == 1 || mgModeEvtNo == 2 || mgModeEvtNo == 3 || mgModeEvtNo == 4) {
        freePlayModeType = 0;
        goto freeplay;
    }
    Hu3DModelAttrSet(mgModeModelIds[9], HU3D_MOTATTR_PAUSE);
    Hu3DMotionTimeSet(mgModeModelIds[9], 0);
    for (time = 0; time <= 30; time++) {
        for (i = 0; i < 4; i++) {
            if (time > i * 2 && time < (i * 2) + 20) {
                float weight = (time - (i * 2)) / 20.0;
                Vec pos2D = modeCameraOffsetPresets[i];
                Vec pos3D;
                pos2D.z = 800;
                pos2D.y -= (1.0 - sind(weight * 90.0f)) * 300.0;
                Hu3D2Dto3D(&pos2D, 1, &pos3D);
                pos3D.z -= 50.0f;
                Hu3DModelPosSetV(mgModeModelIds[i + 9], &pos3D);
                Hu3DModelAttrReset(mgModeModelIds[i + 9], HU3D_ATTR_DISPOFF);
            }
        }
        HuPrcVSleep();
    }
    while (1) {
        if (modeSelectionMessageWindow == -1) {
            modeSelectionMessageWindow = CreateStandardMessageWindow(0, 0, 0);
        }
        HuWinPushKeySet(modeSelectionMessageWindow, PAD_BUTTON_A | PAD_BUTTON_B);
        HuWinMesSpeedSet(modeSelectionMessageWindow, 1);
        HuAudFXPlay(68);
        ShowMessageAndWait(windowId, 0x280001, -1, -1);
        modeSelectionInstructionWindow = CreateCenteredMessageWindow(0x280018);
        freePlayModeType = 0;
        espPosSet(mgModeSpriteIds[0], -20 + modeCameraOffsetPresets[freePlayModeType].x, -10 + modeCameraOffsetPresets[freePlayModeType].y);
        espAttrReset(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
        HuWinMesSpeedSet(modeSelectionMessageWindow, 0);
        ShowMessageAndWait(windowId, freePlayModeType + 0x280002, -1, -999);
        while (1) {
            s16 delta = 0;
            if ((HuPadDStkRep[0] & PAD_BUTTON_LEFT) && freePlayModeType > 0) {
                delta = -1;
            }
            else if ((HuPadDStkRep[0] & PAD_BUTTON_RIGHT) && freePlayModeType < 3) {
                delta = 1;
            }
            if (delta != 0) {
                HuAudFXPlay(0);
                freePlayModeType += delta;
                espPosSet(mgModeSpriteIds[0], -20 + modeCameraOffsetPresets[freePlayModeType].x, -10 + modeCameraOffsetPresets[freePlayModeType].y);
                ShowMessageAndWait(windowId, freePlayModeType + 0x280002, -1, -999);
            }
            if (HuPadBtnDown[0] & PAD_BUTTON_A) {
                HuAudFXPlay(2);
                if (freePlayModeType != 2) {
                    break;
                }
                if (GetSelectedMode() == 0) {
                    continue;
                }
                break;
            }
            if (HuPadBtnDown[0] & PAD_BUTTON_B) {
                s16 choice;
                espAttrSet(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
                ShowMessageAndWait(windowId, 0x280006, -1, -1);
                choice = HuWinChoiceGet(modeSelectionMessageWindow, 1);
                if (choice != 0) {
                    ShowMessageAndWait(windowId, freePlayModeType + 0x280002, -1, -999);
                    espAttrReset(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
                    continue;
                }
                else {
                    CloseCenteredMessageWindow(windowId);
                    CloseStandardMessageWindow(windowId);
                    modeSelectionActive = 1;
                    while (1) {
                        HuPrcVSleep();
                    }
                }
            }
            HuPrcVSleep();
        }
        CloseCenteredMessageWindow(windowId);
        mgModeEvtNo = 0;
        espAttrSet(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
        if (freePlayModeType == 0) {
        freeplay:
            ShowModeSelectionSubmenu();
        }
        else if (freePlayModeType == 1) {
            ShowModeSelectionHelp();
        }
        else if (freePlayModeType == 2) {
            HideModeSelectionHelp();
        }
        else if (freePlayModeType == 3) {
            ApplySelectedMode();
        }
    }
}

s32 SelectFreePlayMode(void);
s32 SelectTeamPlayMode(void);
s32 SelectBattleMode(void);
s32 SelectBingoMode(void);

void ShowModeSelectionSubmenu(void)
{
    s16 exitF = 0;
    s16 time;
    s16 i;
    HU3DMODEL *modelP;
    Vec pos2D;
    Vec pos3D;

    Vec posModel;
    float weight;

    if (mgModeEvtNo == 1) {
        freePlayModeSelection = 0;
        goto freeplay;
    }
    if (mgModeEvtNo == 2) {
        freePlayModeSelection = 1;
        goto teamplay;
    }
    if (mgModeEvtNo == 3) {
        freePlayModeSelection = 2;
        goto battle;
    }
    if (mgModeEvtNo == 4) {
        freePlayModeSelection = 3;
        goto tictactoe;
    }
    pos2D.x = 168;
    pos2D.y = 190;
    pos2D.z = 800;
    Hu3D2Dto3D(&pos2D, 1, &pos3D);
    pos3D.z -= 50;
    modelP = &Hu3DData[mgModeModelIds[9]];
    posModel = modelP->pos;
    VECSubtract(&pos3D, &posModel, &pos2D);
    for (time = 0; time <= 30; time++) {
        for (i = 1; i < 4; i++) {
            if (time > i * 2 && time < (i * 2) + 10) {
                Vec pos;
                weight = (time - (i * 2)) / 10.0;
                pos = modeCameraOffsetPresets[i];
                pos.z = 800;
                pos.x += (1.0 - cosd(weight * 90.0f)) * 500.0;
                Hu3D2Dto3D(&pos, 1, &pos3D);
                pos3D.z -= 50.0f;
                Hu3DModelPosSetV(mgModeModelIds[i + 9], &pos3D);
                Hu3DModelAttrReset(mgModeModelIds[i + 9], HU3D_ATTR_DISPOFF);
            }
        }
        if (time > 10) {
            weight = (time - 10) / 20.0;
            VECScale(&pos2D, &pos3D, sind(weight * 90.0f));
            VECAdd(&posModel, &pos3D, &pos3D);
            Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
        }
        if (time == 20) {
            HuAudFXPlay(2173);
            Hu3DModelAttrReset(mgModeModelIds[9], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
        }
        HuPrcVSleep();
    }
    while (Hu3DMotionEndCheck(mgModeModelIds[9]) == 0) {
        HuPrcVSleep();
    }
    while (1) {
        HuWinMesSpeedSet(modeSelectionMessageWindow, 1);
        HuAudFXPlay(68);
        ShowMessageAndWait(windowId, 0x280007, -1, -1);
        modeSelectionInstructionWindow = CreateCenteredMessageWindow(0x1A0020);
        ShowMessageAndWait(windowId, 0x280008, -1, -999);
        espAttrReset(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
        freePlayModeSelection = 0;
        espPosSet(mgModeSpriteIds[0], -20 + (168.0f + (freePlayModeSelection * 80)), 180);
        while (1) {
            s16 delta = 0;
            if ((HuPadDStkRep[0] & PAD_BUTTON_LEFT) && freePlayModeSelection > 0) {
                delta = -1;
            }
            else if ((HuPadDStkRep[0] & PAD_BUTTON_RIGHT) && freePlayModeSelection < 3) {
                delta = 1;
            }
            if (delta != 0) {
                HuAudFXPlay(0);
                freePlayModeSelection += delta;
                espPosSet(mgModeSpriteIds[0], -20 + (168.0f + (freePlayModeSelection * 80)), 180);
                ShowMessageAndWait(windowId, freePlayModeSelection + 0x280008, -1, -999);
            }
            if (HuPadBtnDown[0] & PAD_BUTTON_A) {
                HuAudFXPlay(2);
                if (freePlayModeSelection == 0 && availableMinigameCountByType[0] == 0 && availableMinigameCountByType[1] == 0 && availableMinigameCountByType[2] == 0 && availableMinigameCountByType[3] == 0
                    && availableMinigameCountByType[4] == 0) {
                    ShowMessageAndWait(windowId, 0x280031, -1, -1);
                    ShowMessageAndWait(windowId, freePlayModeSelection + 0x280008, -1, -999);
                    continue;
                }
                if (freePlayModeSelection == 2 && availableMinigameCountByType[0] == 0 && availableMinigameCountByType[1] == 0 && availableMinigameCountByType[2] == 0 && availableMinigameCountByType[4] == 0) {
                    ShowMessageAndWait(windowId, 0x280033, -1, -1);
                    ShowMessageAndWait(windowId, freePlayModeSelection + 0x280008, -1, -999);
                    continue;
                }
                if ((freePlayModeSelection == 1 || freePlayModeSelection == 3) && availableMinigameCountByType[2] == 0) {
                    if (freePlayModeSelection == 1) {
                        ShowMessageAndWait(windowId, 0x280032, -1, -1);
                    }
                    else {
                        ShowMessageAndWait(windowId, 0x28002F, -1, -1);
                    }
                    ShowMessageAndWait(windowId, freePlayModeSelection + 0x280008, -1, -999);
                    continue;
                }
                break;
            }
            if (HuPadBtnDown[0] & PAD_BUTTON_B) {
                HuAudFXPlay(3);
                exitF = 1;
                break;
            }
            HuPrcVSleep();
        }
        CloseCenteredMessageWindow(windowId);
        if (exitF != 0) {
            break;
        }
        if (freePlayModeSelection == 0) {
        freeplay:
            SelectFreePlayMode();
        }
        else if (freePlayModeSelection == 1) {
        teamplay:
            SelectTeamPlayMode();
        }
        else if (freePlayModeSelection == 2) {
        battle:
            SelectBattleMode();
        }
        else if (freePlayModeSelection == 3) {
        tictactoe:
            SelectBingoMode();
        }
        if (modeSelectionMessageWindow == -1) {
            modeSelectionMessageWindow = CreateStandardMessageWindow(0, 0, 0);
        }
        HuWinPushKeySet(modeSelectionMessageWindow, PAD_BUTTON_A | PAD_BUTTON_B);
    }
    mgModeEvtNo = 0;
    pos2D = modeCameraOffsetPresets[0];
    pos2D.z = 800;
    Hu3D2Dto3D(&pos2D, 1, &pos3D);
    pos3D.z -= 50;
    modelP = &Hu3DData[mgModeModelIds[9]];
    posModel = modelP->pos;
    VECSubtract(&pos3D, &posModel, &pos2D);
    Hu3DModelAttrSet(mgModeModelIds[9], HU3D_MOTATTR_REV);
    espAttrSet(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
    HuAudFXPlay(2174);
    for (time = 0; time <= 20; time++) {
        VECScale(&pos2D, &pos3D, sind(time * 4.5));
        VECAdd(&posModel, &pos3D, &pos3D);
        Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
        HuPrcVSleep();
    }
    for (time = 0; time <= 20; time++) {
        for (i = 1; i < 4; i++) {
            if (time > i * 2 && time < (i * 2) + 10) {
                weight = (time - (i * 2)) / 10.0;
                pos2D = modeCameraOffsetPresets[i];
                pos2D.z = 800;
                pos2D.x += (1.0 - sind(weight * 90.0f)) * 500.0;
                Hu3D2Dto3D(&pos2D, 1, &pos3D);
                pos3D.z -= 50.0f;
                Hu3DModelPosSetV(mgModeModelIds[i + 9], &pos3D);
                Hu3DModelAttrReset(mgModeModelIds[i + 9], HU3D_ATTR_DISPOFF);
            }
        }
        HuPrcVSleep();
    }
}

s32 SelectFreePlayMode(void)
{
    Vec pos2D;
    Vec posModel;
    Vec pos3D;
    s16 i;
    HU3DMODEL *modelP;
    s16 characterSelectMode = 0;

    if (mgModeEvtNo == 1) {
        pos2D.x = 60;
        pos2D.y = 70;
        pos2D.z = 1000;
        Hu3D2Dto3D(&pos2D, 1, &pos3D);
        pos3D.z -= 50.0f;
        Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
        Hu3DModelRotSet(mgModeModelIds[9], -90, 0, 0);
        Hu3DModelAttrSet(mgModeModelIds[9], HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(mgModeModelIds[9], HU3D_ATTR_DISPOFF);
    }
    else {
        Hu3DModelAttrSet(mgModeModelIds[9], HU3D_MOTATTR_REV);
        Hu3DModelAttrReset(mgModeModelIds[9], HU3D_MOTATTR_PAUSE);
        espAttrSet(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
        CloseCenteredMessageWindow(windowId);
        CloseStandardMessageWindow(windowId);
        modeSelectionMessageWindow = modeSelectionInstructionWindow = -1;
        HuAudFXPlay(2174);
        HuPrcSleep(40);
        pos2D.x = 60;
        pos2D.y = 70;
        pos2D.z = 1000;
        Hu3D2Dto3D(&pos2D, 1, &pos3D);
        pos3D.z -= 50.0f;
        modelP = &Hu3DData[mgModeModelIds[9]];
        posModel = modelP->pos;
        VECSubtract(&pos3D, &posModel, &pos2D);
        for (i = 0; i <= 20; i++) {
            VECScale(&pos2D, &pos3D, sind(i * 4.5));
            VECAdd(&posModel, &pos3D, &pos3D);
            Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
            Hu3DModelRotSet(mgModeModelIds[9], -90.0 * (i / 20.0), 0, 0);
            HuPrcVSleep();
        }
        while (!Hu3DMotionEndCheck(mgModeModelIds[9])) {
            HuPrcVSleep();
        }
    charsel:
        if (RunMinigameModeSelection(characterSelectMode) == 0) {
            goto exit;
        }
        characterSelectMode = 4;
        for (i = 0; i < 4; i++) {
            GWPlayerCfg[i].character = minigameSharedState.playerStates[i].character;
            GWPlayerCfg[i].iscom = minigameSharedState.playerStates[i].isComputer;
            GWPlayerCfg[i].diff = minigameSharedState.playerStates[i].difficulty;
        }
    }
    mgPracticeEnableF = 1;
    if (RunFreePlaySelection()) {
        mgModeEvtNo = 0;
        goto charsel;
    }
exit:
    mgPracticeEnableF = 0;
    mgModeEvtNo = 0;
    pos2D.x = 168;
    pos2D.y = 190;
    pos2D.z = 800;
    Hu3D2Dto3D(&pos2D, 1, &pos3D);
    pos3D.z -= 50;
    modelP = &Hu3DData[mgModeModelIds[9]];
    posModel = modelP->pos;
    VECSubtract(&pos3D, &posModel, &pos2D);
    Hu3DModelAttrReset(mgModeModelIds[9], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
    for (i = 0; i <= 20; i++) {
        if (i == 10) {
            HuAudFXPlay(2173);
        }
        VECScale(&pos2D, &pos3D, sind(i * 4.5));
        VECAdd(&posModel, &pos3D, &pos3D);
        Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
        Hu3DModelRotSet(mgModeModelIds[9], -90.0 * (1 - (i / 20.0)), 0, 0);
        HuPrcVSleep();
    }
    while (!Hu3DMotionEndCheck(mgModeModelIds[9])) {
        HuPrcVSleep();
    }
}

s32 SelectTeamPlayMode(void)
{
    Vec pos2D;
    Vec posModel;
    Vec pos3D;
    s16 i;
    HU3DMODEL *modelP;
    s16 characterSelectMode = 1;
    if (mgModeEvtNo == 2) {
        pos2D.x = 60;
        pos2D.y = 70;
        pos2D.z = 1000;
        Hu3D2Dto3D(&pos2D, 1, &pos3D);
        pos3D.z -= 50.0f;
        Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
        Hu3DModelRotSet(mgModeModelIds[9], 0, -90, 90);
        Hu3DModelAttrSet(mgModeModelIds[9], HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(mgModeModelIds[9], HU3D_ATTR_DISPOFF);
    }
    else {
        Hu3DModelAttrSet(mgModeModelIds[9], HU3D_MOTATTR_REV);
        Hu3DModelAttrReset(mgModeModelIds[9], HU3D_MOTATTR_PAUSE);
        espAttrSet(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
        CloseCenteredMessageWindow(windowId);
        CloseStandardMessageWindow(windowId);
        modeSelectionMessageWindow = modeSelectionInstructionWindow = -1;
        HuAudFXPlay(2174);
        HuPrcSleep(40);
        pos2D.x = 60;
        pos2D.y = 70;
        pos2D.z = 1000;
        Hu3D2Dto3D(&pos2D, 1, &pos3D);
        pos3D.z -= 50.0f;
        modelP = &Hu3DData[mgModeModelIds[9]];
        posModel = modelP->pos;
        VECSubtract(&pos3D, &posModel, &pos2D);
        for (i = 0; i <= 20; i++) {
            VECScale(&pos2D, &pos3D, sind(i * 4.5));
            VECAdd(&posModel, &pos3D, &pos3D);
            Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
            Hu3DModelRotSet(mgModeModelIds[9], 0, -90.0 * (i / 20.0), 90.0 * (i / 20.0));
            HuPrcVSleep();
        }
        while (!Hu3DMotionEndCheck(mgModeModelIds[9])) {
            HuPrcVSleep();
        }
    charsel:
        if (RunMinigameModeSelection(characterSelectMode) == 0) {
            goto exit;
        }
        characterSelectMode = 4;
        for (i = 0; i < 4; i++) {
            GWPlayerCfg[i].character = minigameSharedState.playerStates[i].character;
            GWPlayerCfg[i].group = minigameSharedState.playerStates[i].team;
            GWPlayerCfg[i].diff = minigameSharedState.playerStates[i].difficulty;
            GWPlayerCfg[i].iscom = minigameSharedState.playerStates[i].isComputer;
        }
    }
    mgPracticeEnableF = 1;
    if (RunFreePlaySelection()) {
        mgModeEvtNo = 0;
        goto charsel;
    }
exit:
    mgPracticeEnableF = 0;
    mgModeEvtNo = 0;
    pos2D.x = 168;
    pos2D.y = 190;
    pos2D.z = 800;
    Hu3D2Dto3D(&pos2D, 1, &pos3D);
    pos3D.z -= 50;
    modelP = &Hu3DData[mgModeModelIds[9]];
    posModel = modelP->pos;
    VECSubtract(&pos3D, &posModel, &pos2D);
    Hu3DModelAttrReset(mgModeModelIds[9], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
    for (i = 0; i <= 20; i++) {
        if (i == 10) {
            HuAudFXPlay(2173);
        }
        VECScale(&pos2D, &pos3D, sind(i * 4.5));
        VECAdd(&posModel, &pos3D, &pos3D);
        Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
        Hu3DModelRotSet(mgModeModelIds[9], 0, -90.0 * (1.0 - (i / 20.0)), 90.0 * (1.0 - (i / 20.0)));
        HuPrcVSleep();
    }
    while (!Hu3DMotionEndCheck(mgModeModelIds[9])) {
        HuPrcVSleep();
    }
}

s32 SelectBattleMode(void)
{
    Vec pos2D;
    Vec posModel;
    Vec pos3D;
    s16 characterSelectMode = 2;
    s16 i;
    HU3DMODEL *modelP;
    for (i = 0; i < 4; i++) {
        Hu3DModelLayerSet(mgModeModelIds[i + 9], 2);
    }
    if (mgModeEvtNo == 3) {
        OSReport("Eliminate Play STart\n");
        pos2D.x = 50;
        pos2D.y = 70;
        pos2D.z = 1000;
        Hu3D2Dto3D(&pos2D, 1, &pos3D);
        pos3D.z -= 50.0f;
        Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
        Hu3DModelRotSet(mgModeModelIds[9], -90, 180, 0);
        Hu3DModelAttrSet(mgModeModelIds[9], HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(mgModeModelIds[9], HU3D_ATTR_DISPOFF);
    }
    else {
        Hu3DModelAttrSet(mgModeModelIds[9], HU3D_MOTATTR_REV);
        Hu3DModelAttrReset(mgModeModelIds[9], HU3D_MOTATTR_PAUSE);
        espAttrSet(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
        CloseCenteredMessageWindow(windowId);
        CloseStandardMessageWindow(windowId);
        modeSelectionMessageWindow = modeSelectionInstructionWindow = -1;
        HuAudFXPlay(2174);
        HuPrcSleep(40);
        pos2D.x = 50;
        pos2D.y = 70;
        pos2D.z = 1000;
        Hu3D2Dto3D(&pos2D, 1, &pos3D);
        pos3D.z -= 50.0f;
        modelP = &Hu3DData[mgModeModelIds[9]];
        posModel = modelP->pos;
        VECSubtract(&pos3D, &posModel, &pos2D);
        for (i = 0; i <= 20; i++) {
            VECScale(&pos2D, &pos3D, sind(i * 4.5));
            VECAdd(&posModel, &pos3D, &pos3D);
            Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
            Hu3DModelRotSet(mgModeModelIds[9], 90.0 * -(i / 20.0), 180.0 * (i / 20.0), 0);
            HuPrcVSleep();
        }
        while (!Hu3DMotionEndCheck(mgModeModelIds[9])) {
            HuPrcVSleep();
        }
    charsel:
        if (RunMinigameModeSelection(characterSelectMode) == 0) {
            goto exit;
        }
        characterSelectMode = 4;
        for (i = 0; i < 4; i++) {
            GWPlayerCfg[i].character = minigameSharedState.playerStates[i].character;
            GWPlayerCfg[i].group = minigameSharedState.playerStates[i].team;
            GWPlayerCfg[i].iscom = minigameSharedState.playerStates[i].isComputer;
            GWPlayerCfg[i].diff = minigameSharedState.playerStates[i].difficulty;
        }
        mgBattleStarMax = minigameSharedState.battleStarMaximum;
    }

    if (RunBattleMinigameSelection() == 0) {
        mgModeEvtNo = 0;
        goto charsel;
    }
exit:
    HuDataDirClose(DATADIR_INSTPIC);
    for (i = 0; i < 4; i++) {
        Hu3DModelLayerSet(mgModeModelIds[i + 9], 3);
    }
    mgModeEvtNo = 0;
    pos2D.x = 168;
    pos2D.y = 190;
    pos2D.z = 800;
    Hu3D2Dto3D(&pos2D, 1, &pos3D);
    pos3D.z -= 50;
    modelP = &Hu3DData[mgModeModelIds[9]];
    posModel = modelP->pos;
    VECSubtract(&pos3D, &posModel, &pos2D);
    Hu3DModelAttrReset(mgModeModelIds[9], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
    for (i = 0; i <= 20; i++) {
        if (i == 10) {
            HuAudFXPlay(2173);
        }
        VECScale(&pos2D, &pos3D, sind(i * 4.5));
        VECAdd(&posModel, &pos3D, &pos3D);
        Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
        Hu3DModelRotSet(mgModeModelIds[9], 90.0 * -(1 - (i / 20.0)), 180.0 * (1 - (i / 20.0)), 0);
        HuPrcVSleep();
    }
    while (!Hu3DMotionEndCheck(mgModeModelIds[9])) {
        HuPrcVSleep();
    }
}

s32 SelectBingoMode(void)
{
    Vec pos2D;
    Vec posModel;
    Vec pos3D;
    s16 i;
    HU3DMODEL *modelP;
    s16 characterSelectMode = 1;
    if (mgModeEvtNo == 4) {
        pos2D.x = 60;
        pos2D.y = 70;
        pos2D.z = 1000;
        Hu3D2Dto3D(&pos2D, 1, &pos3D);
        pos3D.z -= 50.0f;
        Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
        Hu3DModelRotSet(mgModeModelIds[9], -90, 90, 0);
        Hu3DModelAttrSet(mgModeModelIds[9], HU3D_MOTATTR_PAUSE);
        Hu3DModelAttrReset(mgModeModelIds[9], HU3D_ATTR_DISPOFF);
    }
    else {
        Hu3DModelAttrSet(mgModeModelIds[9], HU3D_MOTATTR_REV);
        Hu3DModelAttrReset(mgModeModelIds[9], HU3D_MOTATTR_PAUSE);
        espAttrSet(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
        CloseCenteredMessageWindow(windowId);
        CloseStandardMessageWindow(windowId);
        modeSelectionMessageWindow = modeSelectionInstructionWindow = -1;
        HuAudFXPlay(2174);
        HuPrcSleep(40);
        pos2D.x = 60;
        pos2D.y = 70;
        pos2D.z = 1000;
        Hu3D2Dto3D(&pos2D, 1, &pos3D);
        pos3D.z -= 50.0f;
        modelP = &Hu3DData[mgModeModelIds[9]];
        posModel = modelP->pos;
        VECSubtract(&pos3D, &posModel, &pos2D);
        for (i = 0; i <= 20; i++) {
            VECScale(&pos2D, &pos3D, sind(i * 4.5));
            VECAdd(&posModel, &pos3D, &pos3D);
            Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
            Hu3DModelRotSet(mgModeModelIds[9], 90.0 * -(i / 20.0), 90.0 * (i / 20.0), 0);
            HuPrcVSleep();
        }
        while (!Hu3DMotionEndCheck(mgModeModelIds[9])) {
            HuPrcVSleep();
        }
    charsel:
        if (RunMinigameModeSelection(characterSelectMode) == 0) {
            goto exit;
        }
        characterSelectMode = 4;
        for (i = 0; i < 4; i++) {
            GWPlayerCfg[i].character = minigameSharedState.playerStates[i].character;
            GWPlayerCfg[i].group = minigameSharedState.playerStates[i].team;
            GWPlayerCfg[i].iscom = minigameSharedState.playerStates[i].isComputer;
            GWPlayerCfg[i].diff = minigameSharedState.playerStates[i].difficulty;
        }
        mgBattleStarMax = minigameSharedState.battleStarMaximum;
    }

    if (RunTicTacToeMinigame() == 0) {
        mgModeEvtNo = 0;
        goto charsel;
    }
exit:
    HuDataDirClose(DATADIR_INSTPIC);
    mgModeEvtNo = 0;
    pos2D.x = 168;
    pos2D.y = 190;
    pos2D.z = 800;
    Hu3D2Dto3D(&pos2D, 1, &pos3D);
    pos3D.z -= 50;
    modelP = &Hu3DData[mgModeModelIds[9]];
    posModel = modelP->pos;
    VECSubtract(&pos3D, &posModel, &pos2D);
    Hu3DModelAttrReset(mgModeModelIds[9], HU3D_MOTATTR_PAUSE | HU3D_MOTATTR_REV);
    for (i = 0; i <= 20; i++) {
        if (i == 10) {
            HuAudFXPlay(2173);
        }
        VECScale(&pos2D, &pos3D, sind(i * 4.5));
        VECAdd(&posModel, &pos3D, &pos3D);
        Hu3DModelPosSetV(mgModeModelIds[9], &pos3D);
        Hu3DModelRotSet(mgModeModelIds[9], 90.0 * -(1 - (i / 20.0)), 90.0 * (1 - (i / 20.0)), 0);
        HuPrcVSleep();
    }
    while (!Hu3DMotionEndCheck(mgModeModelIds[9])) {
        HuPrcVSleep();
    }
}

void ShowModeSelectionHelp(void)
{
    Vec pos2D;
    Vec pos3D;
    Vec posModel;

    s16 unreferencedCountScratch = 0;
    s16 time;
    s16 i;
    float weight;
    HU3DMODEL *modelP;

    CloseCenteredMessageWindow(windowId);
    CloseStandardMessageWindow(windowId);
    modeSelectionMessageWindow = modeSelectionInstructionWindow = -1;
    pos2D.x = 60;
    pos2D.y = 70;
    pos2D.z = 1000;
    Hu3D2Dto3D(&pos2D, 1, &pos3D);
    pos3D.z -= 50.0f;
    modelP = &Hu3DData[mgModeModelIds[10]];
    posModel = modelP->pos;
    VECSubtract(&pos3D, &posModel, &pos2D);
    for (time = 0; time <= 30; time++) {
        for (i = 0; i < 4; i++) {
            if (i == 1) {
                continue;
            }
            if (time > i * 2 && time < (i * 2) + 10) {
                Vec posBox;
                weight = (time - (i * 2)) / 10.0;
                posBox = modeCameraOffsetPresets[i];
                posBox.z = 800;
                posBox.x += ((i < 2) ? -1 : 1) * (500.0 * (1.0 - cosd(90.0f * weight)));
                Hu3D2Dto3D(&posBox, 1, &pos3D);
                pos3D.z -= 50.0f;
                Hu3DModelPosSetV(mgModeModelIds[i + 9], &pos3D);
                Hu3DModelAttrReset(mgModeModelIds[i + 9], HU3D_ATTR_DISPOFF);
            }
        }
        if (time > 10) {
            weight = (time - 10) / 20.0;
            VECScale(&pos2D, &pos3D, sind(weight * 90.0f));
            VECAdd(&posModel, &pos3D, &pos3D);
            Hu3DModelPosSetV(mgModeModelIds[10], &pos3D);
        }
        HuPrcVSleep();
    }
    RunFreePlaySelection();
    pos2D = modeCameraOffsetPresets[1];
    pos2D.z = 800;
    Hu3D2Dto3D(&pos2D, 1, &pos3D);
    pos3D.z -= 50.0f;
    modelP = &Hu3DData[mgModeModelIds[10]];
    posModel = modelP->pos;
    VECSubtract(&pos3D, &posModel, &pos2D);
    espAttrSet(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
    for (time = 0; time <= 20; time++) {
        VECScale(&pos2D, &pos3D, sind(time * 4.5));
        VECAdd(&posModel, &pos3D, &pos3D);
        Hu3DModelPosSetV(mgModeModelIds[10], &pos3D);
        HuPrcVSleep();
    }
    for (time = 0; time <= 20; time++) {
        for (i = 0; i < 4; i++) {
            if (i == 1) {
                continue;
            }
            if (time > i * 2 && time < (i * 2) + 10) {
                weight = (time - (i * 2)) / 10.0;
                pos2D = modeCameraOffsetPresets[i];
                pos2D.z = 800;
                pos2D.x += ((i < 2) ? -1 : 1) * ((1.0 - sind(weight * 90.0f)) * 500.0);
                Hu3D2Dto3D(&pos2D, 1, &pos3D);
                pos3D.z -= 50.0f;
                Hu3DModelPosSetV(mgModeModelIds[i + 9], &pos3D);
                Hu3DModelAttrReset(mgModeModelIds[i + 9], HU3D_ATTR_DISPOFF);
            }
        }
        HuPrcVSleep();
    }
}

void HideModeSelectionHelp(void)
{
    Vec pos2D;
    Vec pos3D;
    Vec posModel;

    s16 customBoardEntryCounts[4];
    s16 unreferencedCountScratch = 0;
    s16 time;
    s16 i;
    float weight;
    HU3DMODEL *modelP;
    CloseCenteredMessageWindow(windowId);
    CloseStandardMessageWindow(windowId);
    modeSelectionMessageWindow = modeSelectionInstructionWindow = -1;
    pos2D.x = 60;
    pos2D.y = 70;
    pos2D.z = 1000;
    Hu3D2Dto3D(&pos2D, 1, &pos3D);
    pos3D.z -= 50.0f;
    modelP = &Hu3DData[mgModeModelIds[11]];
    posModel = modelP->pos;
    VECSubtract(&pos3D, &posModel, &pos2D);
    for (time = 0; time <= 30; time++) {
        for (i = 0; i < 4; i++) {
            if (i == 2) {
                continue;
            }
            if (time > i * 2 && time < (i * 2) + 10) {
                Vec posBox;
                weight = (time - (i * 2)) / 10.0;
                posBox = modeCameraOffsetPresets[i];
                posBox.z = 800;
                posBox.x += ((i < 3) ? -1 : 1) * (500.0 * (1.0 - cosd(90.0f * weight)));
                Hu3D2Dto3D(&posBox, 1, &pos3D);
                pos3D.z -= 50.0f;
                Hu3DModelPosSetV(mgModeModelIds[i + 9], &pos3D);
                Hu3DModelAttrReset(mgModeModelIds[i + 9], HU3D_ATTR_DISPOFF);
            }
        }
        if (time > 10) {
            weight = (time - 10) / 20.0;
            VECScale(&pos2D, &pos3D, sind(weight * 90.0f));
            VECAdd(&posModel, &pos3D, &pos3D);
            Hu3DModelPosSetV(mgModeModelIds[11], &pos3D);
        }
        HuPrcVSleep();
    }
    RunFreePlaySelection();
    for (i = 0; i < 4; i++) {
        customBoardEntryCounts[i] = 0;
        for (time = 0; time < availableMinigameCountByType[freePlaySelectionOrder[i]]; time++) {
            if (GWMGCustomGet(availableMinigameIdsByType[freePlaySelectionOrder[i]][time] + 401)) {
                customBoardEntryCounts[i]++;
            }
        }
    }
    if (customBoardEntryCounts[0] >= 4 && customBoardEntryCounts[1] >= 3 && customBoardEntryCounts[2] >= 3 && customBoardEntryCounts[3] >= 2) {
        GWGameStat.customPackEnable = 1;
    }
    else {
        GWGameStat.customPackEnable = 0;
    }
    pos2D = modeCameraOffsetPresets[2];
    pos2D.z = 800;
    Hu3D2Dto3D(&pos2D, 1, &pos3D);
    pos3D.z -= 50.0f;
    modelP = &Hu3DData[mgModeModelIds[11]];
    posModel = modelP->pos;
    VECSubtract(&pos3D, &posModel, &pos2D);
    espAttrSet(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
    for (time = 0; time <= 20; time++) {
        VECScale(&pos2D, &pos3D, sind(time * 4.5));
        VECAdd(&posModel, &pos3D, &pos3D);
        Hu3DModelPosSetV(mgModeModelIds[11], &pos3D);
        HuPrcVSleep();
    }
    for (time = 0; time <= 20; time++) {
        for (i = 0; i < 4; i++) {
            if (i == 2) {
                continue;
            }
            if (time > i * 2 && time < (i * 2) + 10) {
                weight = (time - (i * 2)) / 10.0;
                pos2D = modeCameraOffsetPresets[i];
                pos2D.z = 800;
                pos2D.x += ((i < 3) ? -1 : 1) * ((1.0 - sind(weight * 90.0f)) * 500.0);
                Hu3D2Dto3D(&pos2D, 1, &pos3D);
                pos3D.z -= 50.0f;
                Hu3DModelPosSetV(mgModeModelIds[i + 9], &pos3D);
                Hu3DModelAttrReset(mgModeModelIds[i + 9], HU3D_ATTR_DISPOFF);
            }
        }
        HuPrcVSleep();
    }
}

s32 GetSelectedMode(void)
{
    s16 i;
    char str[4][6];

    for (i = 0; i < 4; i++) {
        strcpy(&str[i][0],
            "\x1E\x03"
            "0"
            "\x1E\x08");
    }
    if (availableMinigameCountByType[0] >= 4 && availableMinigameCountByType[1] >= 3 && availableMinigameCountByType[2] >= 3 && availableMinigameCountByType[4] >= 2) {
        return 1;
    }
    if (availableMinigameCountByType[0] >= 4) {
        str[0][1] = 5;
    }
    else {
        str[0][2] = '4' - availableMinigameCountByType[0];
    }
    HuWinInsertMesSet(modeSelectionMessageWindow, MAKE_MESSID_PTR(&str[0][0]), 0);
    if (availableMinigameCountByType[1] >= 3) {
        str[1][1] = 5;
    }
    else {
        str[1][2] = '3' - availableMinigameCountByType[1];
    }
    HuWinInsertMesSet(modeSelectionMessageWindow, MAKE_MESSID_PTR(&str[1][0]), 1);
    if (availableMinigameCountByType[2] >= 3) {
        str[2][1] = 5;
    }
    else {
        str[2][2] = '3' - availableMinigameCountByType[2];
    }
    HuWinInsertMesSet(modeSelectionMessageWindow, MAKE_MESSID_PTR(&str[2][0]), 2);
    if (availableMinigameCountByType[4] >= 2) {
        str[3][1] = 5;
    }
    else {
        str[3][2] = '2' - availableMinigameCountByType[4];
    }
    HuWinInsertMesSet(modeSelectionMessageWindow, MAKE_MESSID_PTR(&str[3][0]), 3);
    ShowMessageAndWait(windowId, 0x28002A, -1, -1);
    ShowMessageAndWait(windowId, 0x28002B, -1, -1);
    ShowMessageAndWait(windowId, 0x280002 + messageId, -1, -999);
    return 0;
}

void ApplySelectedMode(void)
{
    Vec pos2D;
    Vec pos3D;
    Vec posModel;

    s16 customBoardEntryCounts[4];
    s16 unreferencedCountScratch = 0;
    s16 time;
    s16 i;
    float weight;
    HU3DMODEL *modelP;
    CloseCenteredMessageWindow(windowId);
    CloseStandardMessageWindow(windowId);
    modeSelectionMessageWindow = modeSelectionInstructionWindow = -1;
    pos2D.x = 50;
    pos2D.y = 64;
    pos2D.z = 1000;
    Hu3D2Dto3D(&pos2D, 1, &pos3D);
    pos3D.z -= 50.0f;
    modelP = &Hu3DData[mgModeModelIds[11]];
    posModel = modelP->pos;
    VECSubtract(&pos3D, &posModel, &pos2D);
    for (time = 0; time <= 30; time++) {
        for (i = 0; i < 4; i++) {
            if (i == 3) {
                continue;
            }
            if (time > i * 2 && time < (i * 2) + 10) {
                Vec posBox;
                weight = (time - (i * 2)) / 10.0;
                posBox = modeCameraOffsetPresets[i];
                posBox.z = 800;
                posBox.x += ((i < 3) ? -1 : 1) * (500.0 * (1.0 - cosd(90.0f * weight)));
                Hu3D2Dto3D(&posBox, 1, &pos3D);
                pos3D.z -= 50.0f;
                Hu3DModelPosSetV(mgModeModelIds[i + 9], &pos3D);
                Hu3DModelAttrReset(mgModeModelIds[i + 9], HU3D_ATTR_DISPOFF);
            }
        }
        if (time > 10) {
            weight = (time - 10) / 20.0;
            VECScale(&pos2D, &pos3D, sind(weight * 90.0f));
            VECAdd(&posModel, &pos3D, &pos3D);
            Hu3DModelPosSetV(mgModeModelIds[12], &pos3D);
            weight = (1.0 - (weight * 0.1));
            Hu3DModelScaleSet(mgModeModelIds[12], weight, weight, weight);
        }
        HuPrcVSleep();
    }
    ShowMinigameRecords();
    pos2D = modeCameraOffsetPresets[3];
    pos2D.z = 800;
    Hu3D2Dto3D(&pos2D, 1, &pos3D);
    pos3D.z -= 50.0f;
    modelP = &Hu3DData[mgModeModelIds[12]];
    posModel = modelP->pos;
    VECSubtract(&pos3D, &posModel, &pos2D);
    espAttrSet(mgModeSpriteIds[0], HUSPR_ATTR_DISPOFF);
    for (time = 0; time <= 20; time++) {
        VECScale(&pos2D, &pos3D, sind(time * 4.5));
        VECAdd(&posModel, &pos3D, &pos3D);
        Hu3DModelPosSetV(mgModeModelIds[12], &pos3D);
        weight = 1.0 - (0.1 * (1.0 - (time / 20.0)));
        Hu3DModelScaleSet(mgModeModelIds[12], weight, weight, weight);
        HuPrcVSleep();
    }
    for (time = 0; time <= 20; time++) {
        for (i = 0; i < 4; i++) {
            if (i == 3) {
                continue;
            }
            if (time > i * 2 && time < (i * 2) + 10) {
                weight = (time - (i * 2)) / 10.0;
                pos2D = modeCameraOffsetPresets[i];
                pos2D.z = 800;
                pos2D.x += ((i < 3) ? -1 : 1) * ((1.0 - sind(weight * 90.0f)) * 500.0);
                Hu3D2Dto3D(&pos2D, 1, &pos3D);
                pos3D.z -= 50.0f;
                Hu3DModelPosSetV(mgModeModelIds[i + 9], &pos3D);
                Hu3DModelAttrReset(mgModeModelIds[i + 9], HU3D_ATTR_DISPOFF);
            }
        }
        HuPrcVSleep();
    }
}

void SetModeSelectionCamera(void);

void InitializeModeSelectionModels(void)
{
    s16 i;
    SetModeSelectionCamera();
    CreateDataListModels(modelList);
    CreateDataListSprites(spriteList);
    Hu3DMotionSet(mgModeModelIds[2], mgModeMotionIds[4]);
    Hu3DMotionTimeSet(mgModeModelIds[0], 0);
    Hu3DModelAttrSet(mgModeModelIds[0], HU3D_MOTATTR_PAUSE);
    HuSprExecLayerSet(64, 3);
    espDrawNoSet(mgModeSpriteIds[3], 64);
    for (i = 0; i < 4; i++) {
        Hu3DModelLayerSet(mgModeModelIds[i + 9], 3);
    }
    InstallMinigameBackgroundDrawHook();
    espDrawNoSet(mgModeSpriteIds[28], 64);
    Hu3DModelPosSet(mgModeModelIds[5], 0, 0, 500);
    minigameSharedState.boardModelId = mgModeModelIds[2];
    minigameSharedState.selectionModelMotionId = mgModeMotionIds[4];
    minigameSharedState.boardMotionId = mgModeMotionIds[3];
    minigameSharedState.modeModelId = mgModeModelIds[5];
    minigameSharedState.modeModelMotionId = mgModeMotionIds[6];
    minigameSharedState.victoryMotionId = mgModeMotionIds[7];
    minigameSharedState.alternateModeMotionId = mgModeMotionIds[6];
    InitializeMinigameMenuObjects(processManager, &sharedState);
    Hu3DModelShadowSet(mgModeModelIds[5]);
    #if VERSION_NTSC
    Hu3DModelShadowMapSet(mgModeModelIds[0]);
    #else
    Hu3DModelShadowMapObjSet(mgModeModelIds[0], "base_fix9-base");
    #endif
}

void UpdateModeSelectionCameraPosition(void);

void UpdateModeSelectionCamera(void)
{
    while (1) {
        if (modeSelectionActive) {
            break;
        }
        if (omSysExitReq) {
            break;
        }
        HuPrcVSleep();
    }
    WipeColorSet(255, 255, 255);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    HuAudSeqAllFadeOut(1000);
    while (WipeStatGet()) {
        HuPrcVSleep();
    }
    HuDataDirClose(DATADIR_INSTPIC);
    if (memcmp(&mgGameStatBackup, &GWGameStat, sizeof(GWGameStat))) {
        UpdateModeSelectionCameraPosition();
    }
    BoardStatusKill();
    omOvlReturnEx(1, 1);
    HuPrcEnd();
}

static u16 modeCameraIds[2] = { 1, 2 };

void UpdateModeSelectionObject(OMOBJ *object)
{
    s16 i;
    for (i = 0; i < 1; i++) {
        Vec pos, target, up;
        float x, y, z;

        x = modeCameraRotationTargets[i].x;
        y = modeCameraRotationTargets[i].y;
        z = modeCameraRotationTargets[i].z;

        pos.x = (((sind(y) * cosd(x)) * modeCameraZoomTargets[i]) + modeCameraPositionTargets[i].x);
        pos.y = (-sind(x) * modeCameraZoomTargets[i]) + modeCameraPositionTargets[i].y;
        pos.z = ((cosd(y) * cosd(x)) * modeCameraZoomTargets[i]) + modeCameraPositionTargets[i].z;
        target.x = modeCameraPositionTargets[i].x;
        target.y = modeCameraPositionTargets[i].y;
        target.z = modeCameraPositionTargets[i].z;
        up.x = sind(y) * sind(x);
        up.y = cosd(x);
        up.z = cosd(y) * sind(x);
        Hu3DCameraPosSet(modeCameraIds[i], pos.x, pos.y, pos.z, up.x, up.y, up.z, target.x, target.y, target.z);
    }
}

void ResetModeSelectionCamera(void);

void AnimateModeSelectionObject(OMOBJ *object)
{
    Vec pos;
    Vec offset;
    Vec dir;
    Vec yOffset;

    f32 zRot;
    s8 stickPos;
    if (HuPadBtnDown[0] & PAD_BUTTON_Y) {
        modeSelectionReservedState = (modeSelectionReservedState) ? 0 : 1;
    }
    if (modeSelectionReservedState == 0) {
        ResetModeSelectionCamera();
        return;
    }
    modeCameraRotationTargets[0].y += 0.1f * HuPadStkX[0];
    modeCameraRotationTargets[0].x += 0.1f * HuPadStkY[0];
    modeCameraZoomTargets[0] += HuPadTrigL[0] / 2;
    modeCameraZoomTargets[0] -= HuPadTrigR[0] / 2;
    if (HuPadBtnDown[0] & PAD_BUTTON_B) {
        OSReport("%f,%f,%f\n", modeCameraRotationTargets[0].x, modeCameraRotationTargets[0].y, modeCameraRotationTargets[0].z);
        OSReport("%f,%f,%f\n", modeCameraPositionTargets[0].x, modeCameraPositionTargets[0].y, modeCameraPositionTargets[0].z);
        OSReport("%f\n", modeCameraZoomTargets[0]);
    }
    if (modeCameraZoomTargets[0] < 100.0f) {
        modeCameraZoomTargets[0] = 100.0f;
    }
    pos.x = modeCameraPositionTargets[0].x + (modeCameraZoomTargets[0] * (sind(modeCameraRotationTargets[0].y) * cosd(modeCameraRotationTargets[0].x)));
    pos.y = (modeCameraPositionTargets[0].y + (modeCameraZoomTargets[0] * -sind(modeCameraRotationTargets[0].x)));
    pos.z = (modeCameraPositionTargets[0].z + (modeCameraZoomTargets[0] * (cosd(modeCameraRotationTargets[0].y) * cosd(modeCameraRotationTargets[0].x))));
    offset.x = modeCameraPositionTargets[0].x - pos.x;
    offset.y = modeCameraPositionTargets[0].y - pos.y;
    offset.z = modeCameraPositionTargets[0].z - pos.z;
    dir.x = (sind(modeCameraRotationTargets[0].y) * sind(modeCameraRotationTargets[0].x));
    dir.y = cosd(modeCameraRotationTargets[0].x);
    dir.z = (cosd(modeCameraRotationTargets[0].y) * sind(modeCameraRotationTargets[0].x));
    zRot = modeCameraRotationTargets[0].z;
    yOffset.x = dir.x * (offset.x * offset.x + (1.0f - offset.x * offset.x) * cosd(zRot))
        + dir.y * (offset.x * offset.y * (1.0f - cosd(zRot)) - offset.z * sind(zRot))
        + dir.z * (offset.x * offset.z * (1.0f - cosd(zRot)) + offset.y * sind(zRot));

    yOffset.y = dir.y * (offset.y * offset.y + (1.0f - offset.y * offset.y) * cosd(zRot))
        + dir.x * (offset.x * offset.y * (1.0f - cosd(zRot)) + offset.z * sind(zRot))
        + dir.z * (offset.y * offset.z * (1.0f - cosd(zRot)) - offset.x * sind(zRot));

    yOffset.z = dir.z * (offset.z * offset.z + (1.0f - offset.z * offset.z) * cosd(zRot))
        + (dir.x * (offset.x * offset.z * (1.0 - cosd(zRot)) - offset.y * sind(zRot))
            + dir.y * (offset.y * offset.z * (1.0 - cosd(zRot)) + offset.x * sind(zRot)));

    VECCrossProduct(&dir, &offset, &offset);
    VECNormalize(&offset, &offset);
    stickPos = (HuPadSubStkX[0] & 0xF8);
    if (stickPos != 0) {
        modeCameraPositionTargets[0].x += 0.05f * (offset.x * stickPos);
        modeCameraPositionTargets[0].y += 0.05f * (offset.y * stickPos);
        modeCameraPositionTargets[0].z += 0.05f * (offset.z * stickPos);
    }
    VECNormalize(&yOffset, &offset);
    stickPos = -(HuPadSubStkY[0] & 0xF8);
    if (stickPos != 0) {
        modeCameraPositionTargets[0].x += 0.05f * (offset.x * stickPos);
        modeCameraPositionTargets[0].y += 0.05f * (offset.y * stickPos);
        modeCameraPositionTargets[0].z += 0.05f * (offset.z * stickPos);
    }
}

void SetModeSelectionCamera(void)
{
    Vec pos = { 0, 3000, 800 };
    Vec target = { 0, 0, 640 };
    Vec up = { 0, 1, 0 };
    Hu3DShadowCreate(45, 2000, 25000);
    Hu3DShadowTPLvlSet(0.8f);
    Hu3DShadowPosSet(&pos, &up, &target);
}

void ResetModeSelectionCamera(void)
{
    Vec deltaVec;
    float delta;

    if (modeSelectionActive) {
        return;
    }
    VECSubtract(&modeCameraRotation, &modeCameraRotationTargets[0], &deltaVec);
    if (VECMag(&deltaVec) > 0.2) {
        VECScale(&deltaVec, &deltaVec, 0.05f);
        VECAdd(&modeCameraRotationTargets[0], &deltaVec, &modeCameraRotationTargets[0]);
    }
    else {
        modeCameraRotationTargets[0] = modeCameraRotation;
    }
    VECSubtract(&modeCameraPosition, &modeCameraPositionTargets[0], &deltaVec);
    if (VECMag(&deltaVec) > 0.2) {
        VECScale(&deltaVec, &deltaVec, 0.05f);
        VECAdd(&modeCameraPositionTargets[0], &deltaVec, &modeCameraPositionTargets[0]);
    }
    else {
        modeCameraPositionTargets[0] = modeCameraPosition;
    }
    delta = modeCameraZoom - modeCameraZoomTargets[0];
    if (sqrtf(delta * delta) > 0.2) {
        delta *= 0.05f;
        modeCameraZoomTargets[0] += delta;
    }
    else {
        modeCameraZoomTargets[0] = modeCameraZoom;
    }
}

void SetModeSelectionCursor(s16 modeType)
{
    modeCameraRotation = modeCameraPresets[modeType].rot;
    modeCameraPosition = modeCameraPresets[modeType].pos;
    modeCameraZoom = modeCameraPresets[modeType].zoom;
}

void SetModeSelectionAnimation(s16 modeType)
{
    modeCameraRotation = modeCameraRotationTargets[0] = modeCameraPresets[modeType].rot;
    modeCameraPosition = modeCameraPositionTargets[0] = modeCameraPresets[modeType].pos;
    modeCameraZoom = modeCameraZoomTargets[0] = modeCameraPresets[modeType].zoom;
}

float GetModeSelectionCameraZoom(void)
{
    float delta = modeCameraZoom - modeCameraZoomTargets[0];
    return sqrtf(delta * delta);
}

void UpdateModeSelectionCameraPosition(void)
{
    s16 sprite;
    if (!SLSaveFlagGet()) {
        return;
    }
    Hu3DAllKill();
    HuSprClose();
    HuSprInit();
    espInit();
    HuPrcVSleep();
    sprite = espEntry(DATA_MAKE_NUM(DATADIR_WIN, 0x20), 5000, 0);
    espPosSet(sprite, 288.0, 240.0);
    espAttrReset(sprite, HUSPR_ATTR_DISPOFF);
    Hu3DBGColorSet(0, 0, 0);
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 10);
    while (WipeStatGet()) {
        HuPrcVSleep();
    }
    SLCommonSet();
    if (!SLSave()) {
        GWSaveModeSet(1);
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 20);
    while (WipeStatGet()) {
        HuPrcVSleep();
    }
}

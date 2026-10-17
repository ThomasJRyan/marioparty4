#include "REL/modeseldll.h"
#include "ext_math.h"
#include "game/audio.h"
#include "game/board/ui.h"
#include "game/disp.h"
#include "game/esprite.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/window.h"
#include "game/wipe.h"

#include <msm/msmsys.h>

typedef struct CameraViewParams {
    Vec rot;
    Vec pos;
    float zoom;
} CameraViewParams;

CameraViewParams modeSelectionCameraViews[] = { { { 0, 0, 0 }, { 0, 0, 0 }, 1500 }, { { 0, 0, 0 }, { 0, 0, 0 }, 524 } };

OMOBJ *mainControllerObject;
HUPROCESS *modeSelectionProcessManager;
s32 modeSelectionExitFlags[2];
Vec cameraTargetRotation;
Vec cameraTargetPosition;
float cameraTargetZoom;
Vec cameraRotation[4];
Vec cameraPosition[2];
float cameraZoom[2];
s32 cameraControlEnabled;

void ModeSelectionProcess(void);
void InitializeModeSelectionScene(void);
void WaitForModeSelectionExit(void);
void UpdateModeSelectionCamera(OMOBJ *object);
void UpdateModeSelectionCameraInput(OMOBJ *object);
void RestoreModeSelectionCamera(void);

void ObjectSetup(void)
{
    s32 light;
    OSReport("******* MS ObjectSetup *********\n");
    modeSelectionProcessManager = omInitObjMan(50, 8192);
    omGameSysInit(modeSelectionProcessManager);
    cameraRotation[0].x = -70;
    cameraRotation[0].y = 0;
    cameraRotation[0].z = 0;
    cameraPosition[0].x = 0;
    cameraPosition[0].y = 130;
    cameraPosition[0].z = -186;
    cameraZoom[0] = 1360;
    cameraTargetRotation = cameraRotation[0];
    cameraTargetPosition = cameraPosition[0];
    cameraTargetZoom = cameraZoom[0];
    modeSelectionExitFlags[0] = 0;
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 30, 20, 8000, 1.2f);
    Hu3DCameraViewportSet(1, 0, 0, 640, 480, 0, 1);
    Hu3DCameraScissorSet(1, 0, 16, 640, 448);
    light = Hu3DGLightCreate(0, 100, 1000, 0, -0.5, -1, 255, 255, 255);
    Hu3DGLightInfinitytSet(light);
    HuPrcChildCreate(ModeSelectionProcess, 100, 16384, 0, modeSelectionProcessManager);
    HuPrcChildCreate(WaitForModeSelectionExit, 200, 4096, 0, modeSelectionProcessManager);
    mainControllerObject = omAddObjEx(modeSelectionProcessManager, 32730, 0, 0, -1, UpdateModeSelectionCamera);
    omAddObjEx(modeSelectionProcessManager, 0, 32, 32, -1, UpdateModeSelectionCameraInput);
    Hu3DBGColorSet(0, 0, 0);
    mgBoardHostEnableF = 0;
    HuWinInit(1);
    mgPracticeEnableF = 0;
    mgQuitExtraF = 0;
}

void ModeSelectionProcess(void)
{
    InitializeModeSelectionScene();
    if (omovlevtno) {
        HuAudSeqPlay(43);
    }
    else {
        if (!RunFileSelection()) {
            WipeColorSet(0, 0, 0);
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 20);
            HuAudSeqAllFadeOut(1000);
            while (WipeStatGet()) {
                HuPrcVSleep();
            }
            #if VERSION_PAL
            if(GwLanguageSave != -1) {
                GWGameStat.language = GwLanguageSave;
            } else if(GwLanguage != -1) {
                GWGameStat.language = GwLanguage;
            }
            #endif
            omOvlReturnEx(1, 1);
            while (1) {
                HuPrcVSleep();
            }
        }
        else {
            s16 grpId;
            s16 sprId;
            #if VERSION_PAL
            HuWinAllKill();
            HuWinInit(1);
            #endif
            grpId = HuSprGrpCreate(1);
            sprId = HuTHPSprCreateVol("movie/opmov_s00.thp", 0, 3000, 70.0);
            HuSprGrpMemberSet(grpId, 0, sprId);
            HuSprPosSet(grpId, 0, 288, 240);
            HuSprDrawNoSet(grpId, 0, 127);
            HuPrcSleep(2);
            espAttrSet(modeSelectSpriteIds[0], HUSPR_ATTR_DISPOFF);
            HuAudFXStop(introSoundEffectId);
            while (!HuTHPEndCheck()) {
                HuPrcVSleep();
            }
            HuTHPClose();
            HuSprGrpKill(grpId);
        }
    }
    espAttrReset(modeSelectSpriteIds[9], HUSPR_ATTR_DISPOFF);
    espDrawNoSet(modeSelectSpriteIds[9], 127);
    {
        s16 result = SelectSpecialMode();
        if (result == -1) {
            HuAudSeqAllFadeOut(1000);
            WipeColorSet(0, 0, 0);
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 20);

            while (WipeStatGet()) {
                HuPrcVSleep();
            }
            omOvlGotoEx(DLL_modeseldll, 1, 0, 0);
            while (1) {
                HuPrcVSleep();
            }
        }
        else {
            s16 i;
            OMOVLHIS *his;
            for (i = 0; i < 4; i++) {
                GWPlayerCfg[i].pad_idx = i;
                if (!HuPadStatGet(i)) {
                    GWPlayerCfg[i].iscom = 0;
                }
                else {
                    GWPlayerCfg[i].iscom = 1;
                }
            }
            his = omOvlHisGet(0);
            omOvlHisChg(0, his->ovl, 1, his->stat);
            _ClearFlag(FLAG_ID_MAKE(1, 0));
            _ClearFlag(FLAG_ID_MAKE(1, 8));
            _ClearFlag(FLAG_ID_MAKE(1, 12));
            _ClearFlag(FLAG_ID_MAKE(1, 11));
            GWGameStat.sound_mode = msmSysGetOutputMode();
            switch (result) {
                case 0:
                    GWMGExplainSet(GWGameStat.party_pause.explain_mg);
                    GWMGShowComSet(GWGameStat.party_pause.show_com_mg);
                    GWMGListSet(GWGameStat.party_pause.mg_list);
                    GWMessSpeedSet(GWGameStat.party_pause.mess_speed);
                    GWSaveModeSet(GWGameStat.party_pause.save_mode);
                    omOvlCallEx(DLL_mentdll, 1, 0, 0);
                    break;

                case 1:
                    GWMGExplainSet(GWGameStat.story_pause.explain_mg);
                    GWMGShowComSet(GWGameStat.story_pause.show_com_mg);
                    GWMGListSet(GWGameStat.story_pause.mg_list);
                    GWMessSpeedSet(GWGameStat.story_pause.mess_speed);
                    GWSaveModeSet(GWGameStat.story_pause.save_mode);
                    omOvlCallEx(DLL_mentdll, 1, 1, 0);
                    break;

                case 2:
                    omOvlCallEx(DLL_mgmodedll, 1, 0, 0);
                    break;

                case 3:
                    omOvlCallEx(DLL_mentdll, 1, 3, 0);
                    break;

                case 4:
                    omOvlCallEx(DLL_mentdll, 1, 4, 0);
                    break;

                case 5:
                    omOvlCallEx(DLL_mpexdll, 1, 0, 0);
                    break;
            }
            while (1) {
                HuPrcVSleep();
            }
        }
    }
}

void InitializeModeSelectionScene(void)
{
    Vec pos;
    Vec target;
    Vec up;
    Hu3DShadowCreate(20, 100, 5000);
    Hu3DShadowTPLvlSet(0.3f);
    Hu3DShadowSizeSet(192);
    target.x = target.y = target.z = 0;
    pos.x = 500;
    pos.z = 1000;
    pos.y = 2000;
    up.x = 0;
    up.y = 1;
    up.z = 0;
    Hu3DShadowPosSet(&pos, &up, &target);
    LoadModeSelectionModels(modeSelectModelData);
    LoadModeSelectionSprites(modeSelectSpriteData);
}

void WaitForModeSelectionExit(void)
{
    s16 i;
    while (1) {
        if (modeSelectionExitFlags[0] || omSysExitReq) {
            break;
        }
        HuPrcVSleep();
    }
    for (i = 0; i < 4; i++) {
        GWPlayerCfg[i].character = -1;
    }
    BoardStatusKill();
    omOvlReturnEx(1, 1);
    HuPrcEnd();
}

u16 modeSelectionCameraIds[] = { 1, 2 };

void UpdateModeSelectionCamera(OMOBJ *object)
{
    s16 i;
    for (i = 0; i < 1; i++) {
        Vec pos, target, up;
        float x, y, z;

        x = cameraRotation[i].x;
        y = cameraRotation[i].y;
        z = cameraRotation[i].z;

        pos.x = (((sind(y) * cosd(x)) * cameraZoom[i]) + cameraPosition[i].x);
        pos.y = (-sind(x) * cameraZoom[i]) + cameraPosition[i].y;
        pos.z = ((cosd(y) * cosd(x)) * cameraZoom[i]) + cameraPosition[i].z;
        target.x = cameraPosition[i].x;
        target.y = cameraPosition[i].y;
        target.z = cameraPosition[i].z;
        up.x = sind(y) * sind(x);
        up.y = cosd(x);
        up.z = cosd(y) * sind(x);
        Hu3DCameraPosSet(modeSelectionCameraIds[i], pos.x, pos.y, pos.z, up.x, up.y, up.z, target.x, target.y, target.z);
    }
}

void UpdateModeSelectionCameraInput(OMOBJ *object)
{
    Vec pos;
    Vec offset;
    Vec dir;
    Vec yOffset;

    f32 zRot;
    s8 stickPos;

    if (!cameraControlEnabled) {
        RestoreModeSelectionCamera();
        return;
    }
    cameraRotation[0].y += 0.1f * HuPadStkX[0];
    cameraRotation[0].x += 0.1f * HuPadStkY[0];
    cameraZoom[0] += HuPadTrigL[0] / 2;
    cameraZoom[0] -= HuPadTrigR[0] / 2;
    if (HuPadBtnDown[0] & PAD_BUTTON_B) {
        OSReport("%f,%f,%f\n", cameraRotation[0].x, cameraRotation[0].y, cameraRotation[0].z);
        OSReport("%f,%f,%f\n", cameraPosition[0].x, cameraPosition[0].y, cameraPosition[0].z);
        OSReport("%f\n", cameraZoom[0]);
    }
    if (cameraZoom[0] < 100.0f) {
        cameraZoom[0] = 100.0f;
    }
    pos.x = cameraPosition[0].x + (cameraZoom[0] * (sind(cameraRotation[0].y) * cosd(cameraRotation[0].x)));
    pos.y = (cameraPosition[0].y + (cameraZoom[0] * -sind(cameraRotation[0].x)));
    pos.z = (cameraPosition[0].z + (cameraZoom[0] * (cosd(cameraRotation[0].y) * cosd(cameraRotation[0].x))));
    offset.x = cameraPosition[0].x - pos.x;
    offset.y = cameraPosition[0].y - pos.y;
    offset.z = cameraPosition[0].z - pos.z;
    dir.x = (sind(cameraRotation[0].y) * sind(cameraRotation[0].x));
    dir.y = cosd(cameraRotation[0].x);
    dir.z = (cosd(cameraRotation[0].y) * sind(cameraRotation[0].x));
    zRot = cameraRotation[0].z;
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
        cameraPosition[0].x += 0.05f * (offset.x * stickPos);
        cameraPosition[0].y += 0.05f * (offset.y * stickPos);
        cameraPosition[0].z += 0.05f * (offset.z * stickPos);
    }
    VECNormalize(&yOffset, &offset);
    stickPos = -(HuPadSubStkY[0] & 0xF8);
    if (stickPos != 0) {
        cameraPosition[0].x += 0.05f * (offset.x * stickPos);
        cameraPosition[0].y += 0.05f * (offset.y * stickPos);
        cameraPosition[0].z += 0.05f * (offset.z * stickPos);
    }
}

void RestoreModeSelectionCamera(void)
{
    Vec deltaVec;
    float deltaZoom;
    if (modeSelectionExitFlags[0]) {
        return;
    }
    VECSubtract(&cameraTargetRotation, &cameraRotation[0], &deltaVec);
    if (VECMag(&deltaVec) > 0.2) {
        VECScale(&deltaVec, &deltaVec, 0.05f);
        VECAdd(&cameraRotation[0], &deltaVec, &cameraRotation[0]);
    }
    else {
        cameraRotation[0] = cameraTargetRotation;
    }
    VECSubtract(&cameraTargetPosition, &cameraPosition[0], &deltaVec);
    if (VECMag(&deltaVec) > 0.2) {
        VECScale(&deltaVec, &deltaVec, 0.05f);
        VECAdd(&cameraPosition[0], &deltaVec, &cameraPosition[0]);
    }
    else {
        cameraPosition[0] = cameraTargetPosition;
    }
    deltaZoom = cameraTargetZoom - cameraZoom[0];
    if (sqrtf(deltaZoom * deltaZoom) > 0.2) {
        deltaZoom *= 0.05f;
        cameraZoom[0] += deltaZoom;
    }
    else {
        cameraZoom[0] = cameraTargetZoom;
    }
}

void SetModeSelectionCameraTarget(s16 view)
{
    cameraTargetRotation = modeSelectionCameraViews[view].rot;
    cameraTargetPosition = modeSelectionCameraViews[view].pos;
    cameraTargetZoom = modeSelectionCameraViews[view].zoom;
}

void SetModeSelectionCamera(s16 view)
{
    s16 i;
    cameraRotation[0] = modeSelectionCameraViews[view].rot;
    cameraTargetRotation = cameraRotation[0];
    cameraPosition[0] = modeSelectionCameraViews[view].pos;
    cameraTargetPosition = cameraPosition[0];
    cameraTargetZoom = cameraZoom[0] = modeSelectionCameraViews[view].zoom;
    UpdateModeSelectionCamera(NULL);
}

float GetModeSelectionCameraZoomDistance(void)
{
    float deltaZoom = cameraTargetZoom - cameraZoom[0];
    return sqrtf(deltaZoom * deltaZoom);
}

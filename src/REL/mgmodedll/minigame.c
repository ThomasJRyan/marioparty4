#include "REL/mgmodedll.h"
#include "game/audio.h"
#include "game/disp.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/wipe.h"

#include "game/chrman.h"

#include "math.h"

s16 mgModeModelIds[13];
s16 mgModeMotionIds[13];
s16 mgModeSpriteIds[29];
s16 playerCharacterIdsByGroup[4];
s16 playerOrderByGroup[4];
s16 playerModelIdsByGroup[4];
s16 playerMotionIdsByGroup[4][10];
s16 minigameIdsByType[10][70];
s16 minigameCountByType[10];
s16 availableMinigameIdsByType[10][70];
s16 availableMinigameCountByType[10];
s16 characterAssetsLoaded;
s16 minigameAssetLoadStatus;

s32 characterMotionDirectories[8] = {
    DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_LUIGIMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_PEACHMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_YOSHIMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_WARIOMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_DONKEYMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_DAISYMOT, 0x00),
    DATA_MAKE_NUM(DATADIR_WALUIGIMOT, 0x00),
};

s32 characterModelDirectories[8] = {
    DATADIR_MARIOMDL1,
    DATADIR_LUIGIMDL1,
    DATADIR_PEACHMDL1,
    DATADIR_YOSHIMDL1,
    DATADIR_WARIOMDL1,
    DATADIR_DONKEYMDL1,
    DATADIR_DAISYMDL1,
    DATADIR_WALUIGIMDL1,
};

s16 minigamePreviewSpriteGroup = -1;

void DrawMinigameBackground(s16 minigameIndex);

void InstallMinigameBackgroundDrawHook(void)
{
    Hu3DLayerHookSet(2, DrawMinigameBackground);
}

void DrawMinigameBackground(s16 minigameIndex)
{
    Mtx44 proj;
    Mtx modelview;
    Vec pos, target, up;
    GXColor color;
    HU3DCAMERA *camera = &Hu3DCamera[0];
    MTXPerspective(proj, camera->fov, camera->aspect, camera->near, camera->far);
    GXSetProjection(proj, GX_PERSPECTIVE);
    GXSetViewport(0, 0, 640, 480, 0, 1);
    GXSetScissor(0, 0, 640, 480);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    color.a = 255;
    GXSetTevColor(GX_TEVREG0, color);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetNumTexGens(0);
    GXSetNumTevStages(01);
    GXSetCullMode(GX_CULL_NONE);
    GXSetAlphaCompare(GX_GEQUAL, 0, GX_AOP_AND, GX_GEQUAL, 0);
    GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
    pos.x = pos.y = pos.z = 0;
    target.x = target.y = 0;
    target.z = -100;
    up.x = up.z = 0;
    up.y = 1;
    MTXLookAt(modelview, &pos, &up, &target);
    GXLoadPosMtxImm(modelview, GX_PNMTX0);
    pos.x = (10000.0f * (-sin(camera->fov / 2.0f))) * 1.2f;
    pos.y = (10000.0f * (-sin(camera->fov / 2.0f))) * (5.0f / 6.0f);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    GXPosition3f32(-pos.x, -pos.y, -10000.0f);
    GXPosition3f32(pos.x, -pos.y, -10000.0f);
    GXPosition3f32(pos.x, pos.y, -10000.0f);
    GXPosition3f32(-pos.x, pos.y, -10000.0f);
    GXEnd();
}

void LoadMinigameCharacterAssets(void)
{
    s16 i;
    s32 status;
    CharModelDataClose(-1);
    CharDataClose(-1);
    for (i = 0; i < 4; i++) {
        status = HuDataDirReadAsync(characterMotionDirectories[GWPlayerCfg[i].character]);
        if (status != -1) {
            while (!HuDataGetAsyncStat(status)) {
                HuPrcVSleep();
            }
        }
        CharMotionInit(GWPlayerCfg[i].character);
    }
    for (i = 0; i < 2; i++) {
        status = HuDataDirReadAsync(characterModelDirectories[GWPlayerCfg[i].character]);
        if (status != -1) {
            while (!HuDataGetAsyncStat(status)) {
                HuPrcVSleep();
            }
        }
    }
    characterAssetsLoaded = 0;
    HuPrcEnd();
    while (1) {
        HuPrcVSleep();
    }
}

s16 SetMinigamePreviewSprite(s16 spriteGroupId, s16 spriteIndex, s16 x)
{
    s16 time;
    ANIMDATA *anim;
    s16 sprite;
    s32 mgpic;
    if (spriteGroupId == -1) {
        if (minigamePreviewSpriteGroup == -1) {
            return;
        }
        for (time = 0; time <= 10; time++) {
            HuSprGrpTPLvlSet(minigamePreviewSpriteGroup, 1.0 - (time / 10.0));
            HuPrcVSleep();
        }
        HuSprGrpKill(minigamePreviewSpriteGroup);
        minigamePreviewSpriteGroup = -1;
        return;
    }

    if (minigamePreviewSpriteGroup != -1) {
        HuSprGrpKill(minigamePreviewSpriteGroup);
    }
    minigamePreviewSpriteGroup = HuSprGrpCreate(1);
    mgpic = mgInfoTbl[spriteGroupId].mg_pic[0];
    if (GWMGAvailGet(spriteGroupId + 401)) {
        anim = HuSprAnimReadFile(mgpic);
    }
    else {
        anim = HuSprAnimReadFile(DATA_MAKE_NUM(DATADIR_MGMODE, 0x2C));
    }
    sprite = HuSprCreate(anim, 100, 0);
    HuSprGrpMemberSet(minigamePreviewSpriteGroup, 0, sprite);
    HuSprGrpPosSet(minigamePreviewSpriteGroup, spriteIndex, x);
    HuSprGrpScaleSet(minigamePreviewSpriteGroup, 0.55f, 0.55f);
    HuSprGrpDrawNoSet(minigamePreviewSpriteGroup, 64);
    return minigamePreviewSpriteGroup;
}

void CreatePlayerCharacterModels(void)
{
    s16 i;
    s16 j;
    for (i = 0; i < 4; i++) {
        playerOrderByGroup[i] = i;
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 3 - i; j++) {
            if (GWPlayerCfg[playerOrderByGroup[j]].group > GWPlayerCfg[playerOrderByGroup[j + 1]].group) {
                s16 swappedPlayerOrder = playerOrderByGroup[j];
                playerOrderByGroup[j] = playerOrderByGroup[j + 1];
                playerOrderByGroup[j + 1] = swappedPlayerOrder;
            }
        }
    }
    for (i = 0; i < 4; i++) {
        s16 charNo;
        playerCharacterIdsByGroup[i] = charNo = GWPlayerCfg[playerOrderByGroup[i]].character;
        playerModelIdsByGroup[i] = CharModelCreate(charNo, 2);
        playerMotionIdsByGroup[i][0] = CharMotionCreate(charNo, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x00));
        playerMotionIdsByGroup[i][1] = CharMotionCreate(charNo, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x02));
        playerMotionIdsByGroup[i][2] = CharMotionCreate(charNo, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x03));
        playerMotionIdsByGroup[i][3] = CharMotionCreate(charNo, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x48));
        playerMotionIdsByGroup[i][4] = CharMotionCreate(charNo, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x49));
        playerMotionIdsByGroup[i][5] = CharMotionCreate(charNo, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x17));
        playerMotionIdsByGroup[i][6] = CharMotionCreate(charNo, DATA_MAKE_NUM(DATADIR_MARIOMOT, 0x18));
        CharMotionVoiceOnSet(GWPlayerCfg[playerOrderByGroup[i]].character, playerMotionIdsByGroup[i][1], 0);
        CharMotionVoiceOnSet(GWPlayerCfg[playerOrderByGroup[i]].character, playerMotionIdsByGroup[i][2], 0);
        Hu3DModelPosSet(playerModelIdsByGroup[i], 600, 0, 800);
        Hu3DModelLayerSet(playerModelIdsByGroup[i], 3);
        CharModelDataClose(charNo);
        Hu3DModelShadowSet(playerModelIdsByGroup[i]);
    }
}

void ExitMinigameMode(s16 modeType)
{
    s16 i;
    OMOVLHIS *his;
    for (i = 0; i < 4; i++) {
        GWPlayerCfg[i].character = minigameSharedState.playerStates[i].character;
        GWPlayerCfg[i].iscom = minigameSharedState.playerStates[i].isComputer;
        GWPlayerCfg[i].pad_idx = minigameSharedState.playerStates[i].controllerPort;
        GWPlayerCfg[i].diff = minigameSharedState.playerStates[i].difficulty;
    }
    WipeColorSet(255, 255, 255);
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    HuAudSeqAllFadeOut(1000);
    while (WipeStatGet()) {
        HuPrcVSleep();
    }
    CharDataClose(-1);
    for (i = 0; i < 4; i++) {
        CharMotionInit(GWPlayerCfg[i].character);
    }
    HuAudSndCharGrpSet(-1);
    his = omOvlHisGet(0);
    omOvlHisChg(0, DLL_mgmodedll, modeType, his->stat);
    omOvlCallEx(DLL_instdll, 1, 0, 0);
    while (1) {
        HuPrcVSleep();
    }
}

void BuildMinigameTypeLists(s32 modeType)
{
    s16 i;
    s16 mgNum;
    for (mgNum = 0; mgInfoTbl[mgNum].ovl != 65535; mgNum++)
        ;
    for (i = 0; i < 9; i++) {
        minigameCountByType[i] = availableMinigameCountByType[i] = 0;
    }
    for (i = 0; i < mgNum; i++) {
        s16 type = mgInfoTbl[i].type;
        if (type >= 0 && type < 9) {
            if (modeType) {
                s16 ovl = mgInfoTbl[i].ovl;
                if (ovl == DLL_m408dll || ovl == DLL_m422dll || ovl == DLL_m430dll) {
                    continue;
                }
            }
            minigameIdsByType[type][minigameCountByType[type]] = i;
            minigameCountByType[type]++;
            if (GWMGAvailGet(i + 401)) {
                availableMinigameIdsByType[type][availableMinigameCountByType[type]] = i;
                availableMinigameCountByType[type]++;
            }
        }
    }
}

void SelectMinigameType(s16 modeType)
{
    s16 i;
    s16 j;
    for (i = 0; i < 256;) {
        for (j = 0; j < availableMinigameCountByType[modeType]; j++, i++) {
            if (i > 256) {
                break;
            }
            mgIndexList[i] = availableMinigameIdsByType[modeType][j];
        }
    }
    for (i = 0; i < 256; i++) {
        mgIndexList[i];
    }
    lbl801D3E94 = 0;
}

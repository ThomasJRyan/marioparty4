#include "REL/mgmodedll.h"
#include "game/gamework.h"

#include "game/audio.h"
#include "game/esprite.h"
#include "game/gamework_data.h"
#include "game/saveload.h"
#include "game/wipe.h"

#include "ext_math.h"

void CreateRecordDisplayWindows(void);
void DestroyRecordDisplayWindows(void);
void ShowMinigameRecord(float startValue, float targetValue, s16 x);

static s16 recordWindows[6];
static s16 recordSpriteGroups[6];

s16 mgRecordIdxTbl[6][3] = { 0, 2, 13, 1, 0, 15, 2, 1, 35, 3, 2, 40, 5, 1, 51, 10, 1, 62 };

void ShowMinigameRecords(void)
{
    s16 i;
    float weight;
    CreateRecordDisplayWindows();
    espAttrReset(mgModeSpriteIds[3], HUSPR_ATTR_DISPOFF);
    espTPLvlSet(mgModeSpriteIds[3], 0);
    for (i = 0; i <= 20; i++) {
        espTPLvlSet(mgModeSpriteIds[3], i / 20.0);
        HuPrcVSleep();
    }
    for (i = 0; i <= 20; i++) {
        weight = i / 20.0;
        espAttrReset(mgModeSpriteIds[27], HUSPR_ATTR_DISPOFF);
        espPosSet(mgModeSpriteIds[27], 288, -100.0 + (164.0 * sind(weight * 90.0f)));
        espAttrReset(mgModeSpriteIds[28], HUSPR_ATTR_DISPOFF);
        espTPLvlSet(mgModeSpriteIds[28], weight);
        espAttrReset(mgModeSpriteIds[20], HUSPR_ATTR_DISPOFF);
        espPosSet(mgModeSpriteIds[20], ((1 - sind(weight * 90.0f)) * 400) + 288, 424);
        HuPrcVSleep();
    }
    for (i = 0; i < 6; i++) {
        ShowMinigameRecord(288, (i * 42) + 150.0f, x);
        HuPrcVSleep();
    }
    while (1) {
        if (HuPadBtnDown[0] & PAD_BUTTON_B) {
            break;
        }
        HuPrcVSleep();
    }
    HuAudFXPlay(3);
    for (i = 0; i < 6; i++) {
        ShowMinigameRecord(1288, 0, x);
        HuPrcVSleep();
    }
    DestroyRecordDisplayWindows();
    for (i = 0; i <= 20; i++) {
        weight = i / 20.0;
        espAttrReset(mgModeSpriteIds[27], HUSPR_ATTR_DISPOFF);
        espPosSet(mgModeSpriteIds[27], 288, -100.0 + (164.0 * cosd(weight * 90.0f)));
        espAttrReset(mgModeSpriteIds[28], HUSPR_ATTR_DISPOFF);
        espTPLvlSet(mgModeSpriteIds[28], 1.0 - weight);
        espAttrReset(mgModeSpriteIds[20], HUSPR_ATTR_DISPOFF);
        espPosSet(mgModeSpriteIds[20], ((1 - cosd(weight * 90.0f)) * 400) + 288, 424);
        HuPrcVSleep();
    }
    for (i = 0; i <= 10; i++) {
        espTPLvlSet(mgModeSpriteIds[3], 1 - (i / 10.0));
        HuPrcVSleep();
    }
}

void CreateRecordDisplayWindows(void)
{
    ANIMDATA *numAnim = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_MGMODE, 53), HU_MEMNUM_OVL));
    s16 i;
    for (i = 0; i < 6; i++) {
        s16 window = HuWinCreate(0, (i * 24), 268, 32, 0);
        s16 j;
        recordWindows[i] = window;
        HuWinBGTPLvlSet(window, 0);
        HuWinMesSpeedSet(window, 0);
        HuWinDispOff(window);
        recordSpriteGroups[i] = HuSprGrpCreate(8);
        for (j = 0; j < 8; j++) {
            s16 sprite = HuSprCreate(numAnim, 10, 0);
            HuSprGrpMemberSet(recordSpriteGroups[i], j, sprite);
            (void)sprite;
            HuSprAttrSet(recordSpriteGroups[i], j, HUSPR_ATTR_DISPOFF);
        }
    }
}

void DestroyRecordDisplayWindows(void)
{
    s16 i;
    for (i = 0; i < 6; i++) {
        HuWinKill(recordWindows[i]);
        HuSprGrpKill(recordSpriteGroups[i]);
    }
}

void ShowMinigameRecord(float startValue, float targetValue, s16 x)
{
    s32 sprGrpId = recordSpriteGroups[x];
    s32 value = GWMGRecordGet(mgRecordIdxTbl[x][0]);
    s32 i;
    s16 mgNo;
    float xOfs;

    if (mgRecordIdxTbl[x][2] != -1) {
        mgNo = omMgIndexGet(mgRecordIdxTbl[x][2]);
        if (!GWMGAvailGet(mgNo + 401)) {
            mgNo = -1;
        }
        else {
            HuWinMesSet(recordWindows[x], mgInfoTbl[mgNo].name_mess);
        }
    }
    else {
        mgNo = -1;
    }
    HuWinDispOn(recordWindows[x]);
    HuWinPosSet(recordWindows[x], startValue - 220, targetValue - 24);
    if (mgNo == -1) {
        HuWinMesSet(recordWindows[x], MAKE_MESSID(35, 0));
        for (i = 0; i < 8; i++) {
            HuSprBankSet(sprGrpId, i, 0);
            HuSprPosSet(sprGrpId, i, i * 16, 0);
            HuSprAttrReset(sprGrpId, i, HUSPR_ATTR_DISPOFF);
        }
        xOfs = 128;
    }
    else {
        s32 memberNum;
        s32 digitValue;
        if (mgRecordIdxTbl[x][1] == 0) {
            memberNum = 0;
            if (value > 9999) {
                value = 9999;
            }
            digitValue = value / 1000;
            if (digitValue != 0) {
                HuSprBankSet(sprGrpId, memberNum, digitValue);
                value -= digitValue * 1000;
                memberNum++;
            }
            digitValue = value / 100;
            if (digitValue != 0 || memberNum != 0) {
                HuSprBankSet(sprGrpId, memberNum, digitValue);
                value -= digitValue * 100;
                memberNum++;
            }
            digitValue = value / 10;
            if (digitValue != 0 || memberNum != 0) {
                HuSprBankSet(sprGrpId, memberNum, digitValue);
                value -= digitValue * 10;
                memberNum++;
            }
            HuSprBankSet(sprGrpId, memberNum, value);
            memberNum++;
            for (i = 0; i < 8; i++) {
                HuSprAttrSet(sprGrpId, i, HUSPR_ATTR_DISPOFF);
            }
            for (i = 0; i < memberNum; i++) {
                HuSprPosSet(sprGrpId, i, i * 16, 0);
                HuSprAttrReset(sprGrpId, i, HUSPR_ATTR_DISPOFF);
            }
            xOfs = memberNum * 16;
        }
        else {
            s16 yOfs[8];
            for (i = 0; i < 8; i++) {
                yOfs[i] = 0;
            }
            memberNum = 0;
            HuSprAttrSet(sprGrpId, memberNum, HUSPR_ATTR_DISPOFF);
            memberNum++;
            digitValue = value / (REFRESH_RATE*60);
            HuSprBankSet(sprGrpId, memberNum, digitValue);
            value -= digitValue * (REFRESH_RATE*60);
            memberNum++;
            HuSprBankSet(sprGrpId, memberNum, 11);
            yOfs[memberNum] = -2;
            memberNum++;
            digitValue = value / (REFRESH_RATE*10);
            HuSprBankSet(sprGrpId, memberNum, digitValue);
            value -= digitValue * (REFRESH_RATE*10);
            memberNum++;
            digitValue = value / REFRESH_RATE;
            HuSprBankSet(sprGrpId, memberNum, digitValue);
            value -= digitValue * REFRESH_RATE;
            memberNum++;
            HuSprBankSet(sprGrpId, memberNum, 12);
            yOfs[memberNum] = -2;
            memberNum++;
            if (mgRecordIdxTbl[x][1] == 2) {
                value = (float)value * (203.0 / (REFRESH_RATE*2));
            }
            else {
                value = (float)value * (200.0 / (REFRESH_RATE*2));
            }
            digitValue = value / 10;
            HuSprBankSet(sprGrpId, memberNum, digitValue);
            value -= digitValue * 10;
            memberNum++;
            HuSprBankSet(sprGrpId, memberNum, value);
            memberNum++;
            for (i = 1; i < 8; i++) {
                HuSprAttrReset(sprGrpId, i, HUSPR_ATTR_DISPOFF);
            }
            for (i = 0; i < memberNum; i++) {
                HuSprPosSet(sprGrpId, i, i * 16, yOfs[i]);
            }
            xOfs = memberNum * 16;
        }
    }
    HuSprGrpPosSet(sprGrpId, 8 + ((210 + startValue) - xOfs), targetValue);
}

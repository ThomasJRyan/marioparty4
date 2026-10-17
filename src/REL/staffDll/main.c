#include "game/animdata.h"
#include "game/audio.h"
#include "game/disp.h"
#include "game/hu3d.h"
#ifndef __MWERKS__
#include "game/minigame_seq.h"
#endif
#include "game/object.h"
#include "game/objsub.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#ifndef __MWERKS__
#include "game/thpmain.h"
#endif
#include "game/window.h"
#include "game/wipe.h"

#include "math.h"

typedef struct StaffSequenceEntry {
    /* 0x00 */ u32 messageId;
    /* 0x04 */ s32 messageStyle;
    /* 0x08 */ float displayDuration;
    /* 0x0C */ u32 advancesPicture;
    /* 0x10 */ s32 logoSpriteIndex;
    /* 0x14 */ s32 isFinished;
} StaffSequenceEntry;

static s16 staffPictureGroups[26];
static s16 staffLogoGroups[3];
static s16 staffMovieGroup;
static s16 staffMovieCoverGroup;
static s32 staffMessageSide;
static s32 currentStaffPicture;
static OMOBJ *staffCameraObject;

static StaffSequenceEntry staffSequence[] = {
    { 0x00340001, 0, 0.0f, 0, 0, 0 },
    { 0x00350000, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 72.0f, 0, 0, 0 },
    { 0x00340002, 0, 0.0f, 0, 0, 0 },
    { 0x00350001, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340003, 0, 0.0f, 0, 0, 0 },
    { 0x00350002, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340004, 0, 0.0f, 0, 0, 0 },
    { 0x00350003, 2, 0.0f, 0, 0, 0 },
    { 0x00350004, 2, 0.0f, 0, 0, 0 },
    { 0x00350005, 2, 0.0f, 0, 0, 0 },
    { 0x00350006, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x00350007, 2, 0.0f, 0, 0, 0 },
    { 0x00350008, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340005, 0, 0.0f, 0, 0, 0 },
    { 0x00350009, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340006, 0, 0.0f, 0, 0, 0 },
    { 0x0035000A, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340007, 0, 0.0f, 0, 0, 0 },
    { 0x0034001C, 1, 0.0f, 0, 0, 0 },
    { 0x0035000B, 2, 0.0f, 0, 0, 0 },
    { 0x0035000C, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x0034001D, 1, 0.0f, 0, 0, 0 },
    { 0x0035000D, 2, 0.0f, 0, 0, 0 },
    { 0x0035000E, 2, 0.0f, 0, 0, 0 },
    { 0x0035000F, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x00350010, 2, 0.0f, 0, 0, 0 },
    { 0x00350011, 2, 0.0f, 0, 0, 0 },
    { 0x00350012, 2, 0.0f, 0, 0, 0 },
    { 0x00350013, 2, 0.0f, 0, 0, 0 },
    { 0x00350014, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x0034001E, 1, 0.0f, 0, 0, 0 },
    { 0x00350015, 2, 0.0f, 0, 0, 0 },
    { 0x00350016, 2, 0.0f, 0, 0, 0 },
    { 0x00350017, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340008, 0, 0.0f, 0, 0, 0 },
    { 0x00350018, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340009, 0, 0.0f, 0, 0, 0 },
    { 0x00350019, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x0034000A, 0, 0.0f, 0, 0, 0 },
    { 0x0034001C, 1, 0.0f, 0, 0, 0 },
    { 0x0035001A, 2, 0.0f, 0, 0, 0 },
    { 0x0035001B, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x0034001D, 1, 0.0f, 0, 0, 0 },
    { 0x0035001C, 2, 0.0f, 0, 0, 0 },
    { 0x0035001D, 2, 0.0f, 0, 0, 0 },
    { 0x0035001E, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x0034001E, 1, 0.0f, 0, 0, 0 },
    { 0x0035001F, 2, 0.0f, 0, 0, 0 },
    { 0x00350020, 2, 0.0f, 0, 0, 0 },
    { 0x00350021, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x0034000B, 0, 0.0f, 0, 0, 0 },
    { 0x0034001F, 1, 0.0f, 0, 0, 0 },
    { 0x00350022, 2, 0.0f, 0, 0, 0 },
    { 0x00350023, 2, 0.0f, 0, 0, 0 },
    { 0x00350024, 2, 0.0f, 0, 0, 0 },
    { 0x00350025, 2, 0.0f, 0, 0, 0 },
    { 0x00350026, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x00350027, 2, 0.0f, 0, 0, 0 },
    { 0x00350028, 2, 0.0f, 0, 0, 0 },
    { 0x00350029, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x0034001C, 1, 0.0f, 0, 0, 0 },
    { 0x0035002A, 2, 0.0f, 0, 0, 0 },
    { 0x0035002B, 2, 0.0f, 0, 0, 0 },
    { 0x0035002C, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x0034001D, 1, 0.0f, 0, 0, 0 },
    { 0x0035002D, 2, 0.0f, 0, 0, 0 },
    { 0x0035002E, 2, 0.0f, 0, 0, 0 },
    { 0x0035002F, 2, 0.0f, 0, 0, 0 },
    { 0x00350030, 2, 0.0f, 0, 0, 0 },
    { 0x00350031, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x00350032, 2, 0.0f, 0, 0, 0 },
    { 0x00350033, 2, 0.0f, 0, 0, 0 },
    { 0x00350034, 2, 0.0f, 0, 0, 0 },
    { 0x00350035, 2, 0.0f, 0, 0, 0 },
    { 0x00350036, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x0034000C, 0, 0.0f, 0, 0, 0 },
    { 0x00350037, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x0034000D, 0, 0.0f, 0, 0, 0 },
    { 0x00350038, 2, 0.0f, 0, 0, 0 },
    { 0x00350039, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x0034000E, 0, 0.0f, 0, 0, 0 },
    { 0x0035003A, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x0034000F, 0, 0.0f, 0, 0, 0 },
    { 0x0035003B, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340010, 0, 0.0f, 0, 0, 0 },
    { 0x0035003C, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x00340011, 0, 0.0f, 0, 0, 0 },
    { 0x0035003B, 2, 0.0f, 0, 0, 0 },
    { 0x0035003C, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x00340012, 0, 0.0f, 0, 0, 0 },
    { 0x0035003D, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x00340013, 0, 0.0f, 0, 0, 0 },
    { 0x0035003E, 2, 0.0f, 0, 0, 0 },
    { 0x0035003F, 2, 0.0f, 0, 0, 0 },
    { 0x00350040, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340014, 0, 0.0f, 0, 0, 0 },
    { 0x00350041, 2, 0.0f, 0, 0, 0 },
    { 0x00350042, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340015, 0, 0.0f, 0, 0, 0 },
    { 0x00350043, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340022, 0, 0.0f, 0, 0, 0 },
    { 0x00340024, 1, 0.0f, 0, 0, 0 },
    { 0x00350056, 2, 0.0f, 0, 0, 0 },
    { 0x00350057, 2, 0.0f, 0, 0, 0 },
    { 0x00350058, 2, 0.0f, 0, 0, 0 },
    { 0x00350059, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x00340025, 1, 0.0f, 0, 0, 0 },
    { 0x0035005A, 2, 0.0f, 0, 0, 0 },
    { 0x0035005B, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340023, 0, 0.0f, 0, 0, 0 },
    { 0x0035005C, 2, 0.0f, 0, 0, 0 },
    { 0x0035005D, 2, 0.0f, 0, 0, 0 },
    { 0x0035005E, 2, 0.0f, 0, 0, 0 },
    { 0x0035005F, 2, 0.0f, 0, 0, 0 },
    { 0x00350060, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    #if VERSION_PAL
    { 0x00340026, 0, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x00340027, 1, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x00340028, 1, 0.0f, 0, 0, 0 },
    { 0x00350061, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x00340029, 1, 0.0f, 0, 0, 0 },
    { 0x0034002A, 1, 0.0f, 0, 0, 0 },
    { 0x00350062, 2, 0.0f, 0, 0, 0 },
    { 0x0034002B, 1, 0.0f, 0, 0, 0 },
    { 0x00350063, 2, 0.0f, 0, 0, 0 },
    { 0x0034002C, 1, 0.0f, 0, 0, 0 },
    { 0x00350064, 2, 0.0f, 0, 0, 0 },
    { 0x0034002D, 1, 0.0f, 0, 0, 0 },
    { 0x00350065, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x0034002E, 1, 0.0f, 0, 0, 0 },
    { -3, 0, 72.0f, 0, 0, 0 },
    #endif
    { 0x00340016, 0, 0.0f, 0, 0, 0 },
    { 0x00350044, 2, 0.0f, 0, 0, 0 },
    #if VERSION_NTSC
    { 0x00350045, 2, 0.0f, 0, 0, 0 },
    #endif
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x00350046, 2, 0.0f, 0, 0, 0 },
    { 0x00350047, 2, 0.0f, 0, 0, 0 },
    { 0x00350048, 2, 0.0f, 0, 0, 0 },
    { 0x00350049, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 24.0f, 0, 0, 0 },
    { 0x0035004A, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x0035004B, 2, 0.0f, 0, 0, 0 },
    { 0x0035004C, 2, 0.0f, 0, 0, 0 },
    { 0x0035004D, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340017, 0, 0.0f, 0, 0, 0 },
    { 0x0035004E, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 48.0f, 0, 0, 0 },
    { 0x00340018, 0, 0.0f, 0, 0, 0 },
    { 0x0035004F, 2, 0.0f, 0, 0, 0 },
    { 0x00350050, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 72.0f, 0, 0, 0 },
    { 0x00340019, 0, 0.0f, 0, 0, 0 },
    { 0x00350038, 2, 0.0f, 0, 0, 0 },
    { 0x00350051, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 72.0f, 0, 0, 0 },
    { 0x0034001A, 0, 0.0f, 0, 0, 0 },
    { 0x00350052, 2, 0.0f, 0, 0, 0 },
    { 0x00350053, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 72.0f, 0, 0, 0 },
    { 0x0034001B, 0, 0.0f, 0, 0, 0 },
    { 0x00350054, 2, 0.0f, 0, 0, 0 },
    { 0x00350055, 2, 0.0f, 0, 0, 0 },
    { -3, 0, 74.0f, 0, 0, 0 },
    { -2, 0, 0.0f, 0, 0, 0 },
    { -1, 0, 0.0f, 0, 0, 0 },
};

#if VERSION_NTSC
static float staffLogoVerticalPositions[] = { 460.0f, 293.0f, 293.0f };
#else
static float staffLogoVerticalPositions[] = { 460.0f, 290.0f, 293.0f };
#endif

static void RunStaffSequence(void);
static void CreateStaffAssets(void);
static void ShowStaffPictures(void);

void ObjectSetup(void)
{
    HUPROCESS *staffProcess;

    staffProcess = omInitObjMan(0x32, 0x2000);
    omGameSysInit(staffProcess);
    HuWinInit(1);
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, 20.0f, 20.0f, 10000.0f, 1.2f);
    Hu3DCameraViewportSet(1, 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1.0f);
    Hu3DCameraScissorSet(1, 0, 0, 640, 480);
    staffCameraObject = omAddObjEx(staffProcess, 0x7FDA, 0, 0, -1, omOutViewMulti);
    staffCameraObject->work[0] = 1;
    CRotM[0].x = -35.0f;
    CRotM[0].y = 0.0f;
    CRotM[0].z = 0.0f;
    CenterM[0].x = 0.0f;
    CenterM[0].y = 0.0f;
    CenterM[0].z = -250.0f;
    CZoomM[0] = 4820.0f;
    Hu3DBGColorSet(0, 0, 0);
    CreateStaffAssets();
    HuPrcChildCreate(RunStaffSequence, 0x1000, 0x2000, 0, HuPrcCurrentGet());
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, 60);
}

static void StaffExitProcess(void)
{
    while (TRUE) {
        if (omSysExitReq == 1) {
            WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
            HuAudFadeOut(1000);
            HuPrcSleep(60);
            MGSeqKillAll();
            omOvlReturnEx(1, 1);

            while (TRUE) {
                HuPrcVSleep();
            }
        }
        HuPrcVSleep();
    }

    while (!omSysExitReq) {
        HuPrcVSleep();
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    HuAudFadeOut(1000);
    HuPrcSleep(60);
    MGSeqKillAll();
    omOvlReturnEx(1, 1);

    while (TRUE) {
        HuPrcVSleep();
    }
}

static void CreateStaffAssets(void)
{
    void *assetData;
    ANIMDATA *spriteAnimation;
    s16 spriteId;
    s32 assetIndex;

    staffMessageSide = 0;
    currentStaffPicture = 0;

    for (assetIndex = 0; assetIndex < 26; assetIndex++) {
        assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_STAFF, assetIndex), HU_MEMNUM_OVL, HEAP_MODEL);
        spriteAnimation = HuSprAnimRead(assetData);
        spriteId = HuSprCreate(spriteAnimation, 16386, 0);
        staffPictureGroups[assetIndex] = HuSprGrpCreate(1);
        HuSprGrpMemberSet(staffPictureGroups[assetIndex], 0, spriteId);
        if (staffMessageSide != 0) {
            HuSprGrpPosSet(staffPictureGroups[assetIndex], 380.0f, 240.0f);
        }
        else {
            HuSprGrpPosSet(staffPictureGroups[assetIndex], 200.0f, 240.0f);
        }
        HuSprAttrSet(staffPictureGroups[assetIndex], 0, HUSPR_ATTR_DISPOFF);
    }

    for (assetIndex = 0; assetIndex < 3; assetIndex++) {
        int languageAssetOffset;
        #if VERSION_NTSC
        assetData = HuDataSelHeapReadNum(assetIndex + DATA_MAKE_NUM(DATADIR_STAFF, 0x1A), HU_MEMNUM_OVL, HEAP_MODEL);
        #else
        if(assetIndex == 2) {
            switch(GWGameStat.language) {
                case 1:
                    languageAssetOffset = 0;
                    break;
                
                case 2:
                    languageAssetOffset = 1;
                    break;
                
                case 4:
                    languageAssetOffset = 3;
                    break;
                
                default:
                    languageAssetOffset = 2;
                    break;
                    
            }
            
            assetData = HuDataSelHeapReadNum(assetIndex + DATA_MAKE_NUM(DATADIR_STAFF, 0x1A) + languageAssetOffset, HU_MEMNUM_OVL, HEAP_MODEL);
        } else {
            assetData = HuDataSelHeapReadNum(assetIndex + DATA_MAKE_NUM(DATADIR_STAFF, 0x1A), HU_MEMNUM_OVL, HEAP_MODEL);
        }
        #endif
        
        spriteAnimation = HuSprAnimRead(assetData);
        spriteId = HuSprCreate(spriteAnimation, 256, 0);
        staffLogoGroups[assetIndex] = HuSprGrpCreate(1);
        HuSprGrpMemberSet(staffLogoGroups[assetIndex], 0, spriteId);
        HuSprAttrSet(staffLogoGroups[assetIndex], 0, HUSPR_ATTR_DISPOFF);
    }
    staffMovieGroup = HuSprGrpCreate(1);
    spriteId = HuTHPSprCreate("movie/stmov_a00.thp", 0, 16386);
    HuSprGrpMemberSet(staffMovieGroup, 0, spriteId);
    HuSprGrpScaleSet(staffMovieGroup, 0.65f, 0.65f);
    HuSprGrpPosSet(staffMovieGroup, 280.0f, 200.0f);
    HuSprAttrSet(staffMovieGroup, 0, HUSPR_ATTR_DISPOFF);
    HuTHPStop();
    #if VERSION_NTSC
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_STAFF, 0x1D), HU_MEMNUM_OVL, HEAP_MODEL);
    #else
    assetData = HuDataSelHeapReadNum(DATA_MAKE_NUM(DATADIR_STAFF, 0x20), HU_MEMNUM_OVL, HEAP_MODEL);
    #endif
    spriteAnimation = HuSprAnimRead(assetData);
    spriteId = HuSprCreate(spriteAnimation, 16385, 0);
    staffMovieCoverGroup = HuSprGrpCreate(1);
    HuSprGrpMemberSet(staffMovieCoverGroup, 0, spriteId);
    HuSprGrpScaleSet(staffMovieCoverGroup, 40.0f, 40.0f);
    HuSprGrpPosSet(staffMovieCoverGroup, 280.0f, 200.0f);
    HuSprAttrSet(staffMovieCoverGroup, 0, HUSPR_ATTR_DISPOFF);
    HuPrcChildCreate(ShowStaffPictures, 4096, 8192, 0, HuPrcCurrentGet());
    HuPrcChildCreate(StaffExitProcess, 4096, 4096, 0, HuPrcCurrentGet());
}

static void AnimateStaffMessage(void)
{
    float messageDimensions[2];
    float messageY;
    float messageX;
    s16 messageWindowId;
    u32 windowAttributes;
    StaffSequenceEntry *staffMessage;

    staffMessage = HuPrcCurrentGet()->user_data;
    staffMessage->isFinished = 0;
    switch (staffMessage->messageId) {
        case -2:
            HuSprAttrReset(staffLogoGroups[staffMessage->logoSpriteIndex], 0, HUSPR_ATTR_DISPOFF);
            messageX = staffLogoVerticalPositions[staffMessage->logoSpriteIndex];
            messageY = 576.0f;
            break;
        default:
            HuWinMesMaxSizeGet(1, messageDimensions, staffMessage->messageId);
            windowAttributes = 0;
            switch (staffMessage->messageStyle) {
                case 3:
                    messageX = 320.0f - (0.75f * messageDimensions[0]) / 2;
                    break;
                case 0:
                    messageX = 10.0f;
                    if (staffMessageSide == 0) {
                        messageX += 240.0f;
                    }
                    break;
                case 1:
                    messageX = 30.0f;
                    if (staffMessageSide == 0) {
                        messageX += 240.0f;
                    }
                    break;
                case 2:
                    messageX = 550.0f - (0.75f * messageDimensions[0]);
                    if (staffMessageSide != 0) {
                        messageX -= 240.0f;
                    }
                    windowAttributes = 0x1000;
                    break;
            }
            messageY = 476.0f;
            messageWindowId = HuWinCreate(messageX, messageY, messageDimensions[0], messageDimensions[1], 1);
            HuWinAttrSet(messageWindowId, windowAttributes);
            HuWinPriSet(messageWindowId, 256);
            HuWinBGTPLvlSet(messageWindowId, 0.0f);
            HuWinMesSpeedSet(messageWindowId, 0);
            HuWinMesSet(messageWindowId, staffMessage->messageId);
            HuWinScaleSet(messageWindowId, 0.75f, 0.75f);
            switch (staffMessage->messageStyle) {
                case 0:
                case 1:
                    HuWinMesPalSet(messageWindowId, 7, 96, 240, 255);
                    break;
                default:
                    HuWinMesPalSet(messageWindowId, 7, 255, 255, 255);
                    break;
            }
            break;
    }
    while (TRUE) {
        messageY -= 0.4f;
        if (staffMessage->messageId == -2) {
            if (messageY < -114.0f) {
                break;
            }
        }
        else if (messageY < -24.0f) {
            break;
        }
        if (staffMessage->messageId == -2) {
            HuSprGrpPosSet(staffLogoGroups[staffMessage->logoSpriteIndex], messageX, messageY);
        }
        else {
            HuWinPosSet(messageWindowId, messageX, messageY);
        }
        HuPrcVSleep();
    }
    staffMessage->isFinished = 1;
    if (staffMessage->messageId == -2) {
        HuSprAttrSet(staffLogoGroups[staffMessage->logoSpriteIndex], 0, HUSPR_ATTR_DISPOFF);
    }
    else {
        HuWinKill(messageWindowId);
    }
    HuPrcVSleep();
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

static void HideCurrentStaffPicture(void)
{
    s32 previousPictureIndex;

    previousPictureIndex = currentStaffPicture;
    currentStaffPicture++;
    if (staffMessageSide != 0) {
        HuSprGrpPosSet(staffPictureGroups[currentStaffPicture], 380.0f, 240.0f);
    }
    else {
        HuSprGrpPosSet(staffPictureGroups[currentStaffPicture], 200.0f, 240.0f);
    }
    HuSprAttrSet(staffPictureGroups[previousPictureIndex], 0, HUSPR_ATTR_DISPOFF);
    HuPrcSleep(20);
    HuSprAttrReset(staffPictureGroups[currentStaffPicture], 0, HUSPR_ATTR_DISPOFF);
    HuPrcKill(HuPrcCurrentGet());
    while (TRUE) {
        HuPrcVSleep();
    }
}

static void ShowStaffPictures(void)
{
    float pictureOpacity;
    s32 fadeFrame;

    HuPrcSleep(360);
    HuSprAttrReset(staffPictureGroups[currentStaffPicture], 0, HUSPR_ATTR_DISPOFF);

    for (fadeFrame = 0, pictureOpacity = 0.0f; fadeFrame < 30; fadeFrame++, pictureOpacity += 1.0f / 30.0f) {
        HuSprTPLvlSet(staffPictureGroups[currentStaffPicture], 0, pictureOpacity);
        HuPrcVSleep();
    }
    HuSprTPLvlSet(staffPictureGroups[currentStaffPicture], 0, 1.0f);

    while (TRUE) {
        #if VERSION_NTSC
        if (currentStaffPicture >= 24) {
            HuPrcSleep(430);
        } else {
            HuPrcSleep(550);
        }
        #else
        if (currentStaffPicture >= 24) {
            HuPrcSleep(480);
        } else {
            HuPrcSleep(600);
        }
        #endif
        if (currentStaffPicture >= 25) {
            break;
        }
        HuPrcChildCreate(HideCurrentStaffPicture, 0x1000, 0x2000, 0, HuPrcCurrentGet());
    }

    for (fadeFrame = 0, pictureOpacity = 1.0f; fadeFrame < 30; fadeFrame++, pictureOpacity -= 1.0f / 30.0f) {
        HuSprTPLvlSet(staffPictureGroups[25], 0, pictureOpacity);
        HuPrcVSleep();
    }
    HuSprAttrSet(staffPictureGroups[25], 0, HUSPR_ATTR_DISPOFF);
    HuSprAttrSet(staffPictureGroups[25], 0, HUSPR_ATTR_DISPOFF);

    while (TRUE) {
        HuPrcVSleep();
    }
}

static void RunStaffSequence(void)
{
    s32 staffMessageIndex;
    s32 animationFrame;
    s32 controllerIndex;
    StaffSequenceEntry *staffMessage;
    s32 audioFadeStarted;
    HUPROCESS *messageProcess;
    s32 staffMusicStream;
    float logoX;
    float logoY;
    float fadeOpacity;

    HuPrcSleep(60);
    staffMusicStream = HuAudSStreamPlay(22);

    for (staffMessageIndex = 0;; staffMessageIndex++) {
        staffMessage = &staffSequence[staffMessageIndex];
        if (staffMessage->messageId == -1) {
            break;
        }

        switch (staffMessage->messageId) {
            case -3:
                HuPrcSleep(staffMessage->displayDuration / 0.4f);
                break;
            default:
                messageProcess = HuPrcChildCreate(AnimateStaffMessage, 0x1000, 0x2000, 0, HuPrcCurrentGet());
                messageProcess->user_data = staffMessage;
                if (staffMessage->advancesPicture == 1) {
                    staffMessageSide++;
                    staffMessageSide &= 1;
                    HuPrcChildCreate(HideCurrentStaffPicture, 0x1000, 0x2000, 0, HuPrcCurrentGet());
                }
                else {
                    HuPrcSleep(60);
                }
                break;
        }
    }
    staffMessage = &staffSequence[staffMessageIndex - 1];

    while (staffMessage->isFinished == 0) {
        HuPrcVSleep();
    }
    HuPrcSleep(60);
    HuSprAttrReset(staffLogoGroups[1], 0, HUSPR_ATTR_DISPOFF);
    logoX = staffLogoVerticalPositions[1];
    logoY = 526.0f;
    audioFadeStarted = 0;

    while (TRUE) {
        logoY -= 0.4f;
        if (logoY < 400.0f) {
            break;
        }
        if ((audioFadeStarted == 0) && (logoY < 506.0f)) {
            audioFadeStarted = 1;
            HuAudSStreamFadeOut(staffMusicStream, 6000);
        }
        HuSprGrpPosSet(staffLogoGroups[1], logoX, logoY);
        HuPrcVSleep();
    }
    HuSprGrpPosSet(staffLogoGroups[1], logoX, 400.0f);
    #if VERSION_NTSC
    HuPrcSleep(240);
    #else
    HuPrcSleep(60);
    #endif
    HuSprAttrReset(staffMovieGroup, 0, HUSPR_ATTR_DISPOFF);
    HuSprAttrReset(staffMovieCoverGroup, 0, HUSPR_ATTR_DISPOFF);
    HuTHPRestart();

    for (animationFrame = 0, fadeOpacity = 1.0f; animationFrame < 90; animationFrame++, fadeOpacity -= 0.1f / 9.0f) {
        HuSprTPLvlSet(staffMovieCoverGroup, 0, fadeOpacity);
        HuPrcVSleep();
    }
    HuSprAttrSet(staffMovieCoverGroup, 0, HUSPR_ATTR_DISPOFF);

    while (HuTHPFrameGet() < 450) {
        HuPrcVSleep();
    }
    HuSprAttrReset(staffMovieCoverGroup, 0, HUSPR_ATTR_DISPOFF);

    for (animationFrame = 0, fadeOpacity = 0.0f; animationFrame < 120; animationFrame++, fadeOpacity += 0.05f / 6.0f) {
        HuSprTPLvlSet(staffMovieCoverGroup, 0, fadeOpacity);
        HuPrcVSleep();
    }
    HuSprTPLvlSet(staffMovieCoverGroup, 0, 1.0f);
    HuSprAttrSet(staffMovieGroup, 0, HUSPR_ATTR_DISPOFF);
    HuTHPClose();

    for (animationFrame = 0, fadeOpacity = 1.0f; animationFrame < 30; animationFrame++, fadeOpacity -= 1.0f / 30.0f) {
        HuSprTPLvlSet(staffLogoGroups[1], 0, fadeOpacity);
        HuPrcVSleep();
    }
    HuSprAttrSet(staffLogoGroups[1], 0, HUSPR_ATTR_DISPOFF);
    HuPrcSleep(120);
    HuSprAttrReset(staffLogoGroups[2], 0, HUSPR_ATTR_DISPOFF);
    #if VERSION_NTSC
    HuSprGrpPosSet(staffLogoGroups[2], 280.0f, 240.0f);
    #else
    HuSprGrpPosSet(staffLogoGroups[2], 275.0f, 240.0f);
    #endif
    for (animationFrame = 0, fadeOpacity = 0.0f; animationFrame < 30; animationFrame++, fadeOpacity += 1.0f / 30.0f) {
        HuSprTPLvlSet(staffLogoGroups[2], 0, fadeOpacity);
        HuPrcVSleep();
    }
    #if VERSION_NTSC
    HuPrcSleep(600);
    #else
    HuPrcSleep(480);
    #endif
    while (TRUE) {
        for (controllerIndex = 0; controllerIndex < 4; controllerIndex++) {
            if (HuPadBtn[controllerIndex] & PAD_BUTTON_START) {
                break;
            }
        }
        if (controllerIndex < 4) {
            break;
        }
        HuPrcVSleep();
    }
    WipeCreate(WIPE_MODE_OUT, WIPE_TYPE_NORMAL, 60);
    HuAudFadeOut(1000);
    HuPrcSleep(60);
    MGSeqKillAll();
    omOvlReturnEx(1, 1);

    while (TRUE) {
        HuPrcVSleep();
    }
}

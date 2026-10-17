#include "ext_math.h"
#include "game/audio.h"
#include "game/frand.h"
#include "game/gamework.h"
#include "game/gamework_data.h"
#include "game/hu3d.h"
#include "game/init.h"
#include "game/objsub.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/wipe.h"

#include "REL/E3SetupDLL.h"

#ifndef __MWERKS__
void HuSysVWaitSet(s16 vcount);
#endif

static s16 defaultMinigameIndex;
static OMOBJ *mgInterfaceObj;
static OMOBJ *mgSelectObj;
static OMOBJ *mgPicObj;

s32 minigameSelectionState;
s16 minigameSelectionTimer;
s16 minigameSelectionFrame;
s16 minigameSelectionDelay;
s16 minigameSelectionIndex;
static s16 minigameSelectionHistory[5];

float e3CameraPosition[] = { 0, 12000, 0, 0, 0, 0, 0, 0, -1 };

static void StartMGSelect(OMOBJ *object);
static void UpdatePad(OMOBJ *object);
static void CreateMGSelect(OMOBJ *object);
static void CreateMGPic(OMOBJ *object);
static void UpdateMGInterface(OMOBJ *object);
static void CreateMGInterface(OMOBJ *object);

void E3MGSelectInit(void)
{
    s32 controllerPort;
    s32 connectedPlayerCount;
    HUPROCESS *objectManager;

    for (controllerPort = 0, connectedPlayerCount = 0; controllerPort < 4; controllerPort++) {
        if (HuPadStatGet(controllerPort) == 0) {
            connectedPlayerCount++;
        }
    }
    e3ConfigPlayerCnt = connectedPlayerCount;
    if (connectedPlayerCount <= 0) {
        OSReport("No pad detected\n");
        connectedPlayerCount++;
    }
    defaultMinigameIndex = 0;
    HuSysVWaitSet(1);
    objectManager = omInitObjMan(0x50, 0x2000);
    HuWinInit(1);
    CRot.x = -4.0f;
    CRot.y = 0.0f;
    CRot.z = 0.0f;
    Center.x = 0.0f;
    Center.y = 160.0f;
    Center.z = 0.0f;
    CZoom = 1960.0;
    e3CameraFov = 40.0f;
    Hu3DCameraCreate(1);
    Hu3DCameraPerspectiveSet(1, e3CameraFov, 100.0f, 25000.0f, 4.0f / 3.0f);
    e3ViewObj = omAddObjEx(objectManager, 32730, 0, 0, -1, omOutView);
    e3OvlWatchObj = omAddObjEx(objectManager, 200, 0, 0, -1, E3OvlWatchInit);
    e3MenuMainObj = omAddObjEx(objectManager, 10, 0, 0, -1, StartMGSelect);
    e3BGObj = omAddObjEx(objectManager, 0x14, 0, 0, -1, E3BGCreate);
    mgInterfaceObj = omAddObjEx(objectManager, 20, 0, 0, -1, CreateMGInterface);
    mgSelectObj = omAddObjEx(objectManager, 30, 0, 0, -1, CreateMGSelect);
    mgPicObj = omAddObjEx(objectManager, 31, 0, 0, -1, CreateMGPic);
    mgPicObj->work[0] = defaultMinigameIndex;
    E3LightInit();
    WipeCreate(WIPE_MODE_IN, WIPE_TYPE_NORMAL, -1);
}

s16 e3OverlayMessageIds[] = { 0x0023, 0x0026, 0x0029, 0x002C };

static s16 minigameOverlayIds[] = {
    11,
    12,
    15,
    18,
    20,
    23,
    26,
    27,
    33,
    34,
    39,
    44,
    46,
};

static s16 minigameIds[] = { 403, 404, 407, 410, 412, 415, 418, 419, 425, 426, 431, 436, 438, 0 };

static s32 minigameMessageIds[] = {
    MAKE_MESSID(0x17, 0x03),
    MAKE_MESSID(0x17, 0x04),
    MAKE_MESSID(0x17, 0x07),
    MAKE_MESSID(0x17, 0x0A),
    MAKE_MESSID(0x17, 0x0C),
    MAKE_MESSID(0x17, 0x0F),
    MAKE_MESSID(0x17, 0x12),
    MAKE_MESSID(0x17, 0x13),
    MAKE_MESSID(0x17, 0x19),
    MAKE_MESSID(0x17, 0x1A),
    MAKE_MESSID(0x17, 0x1F),
    MAKE_MESSID(0x17, 0x24),
    MAKE_MESSID(0x17, 0x26),
};

char testMinigameName01[] = "\013TEST MINIGAME NAME01\n      test";
char testMinigameName02[] = "\013TEST MINIGAME NAME02";
char testMinigameName03[] = "\013TEST MINIGAME NAME03";
char testMinigameName04[] = "\013TEST MINIGAME NAME04";
char testMinigameName05[] = "\013TEST NAME05";
char testMinigameName06[] = "\013TEST MINIGAME NAME06";
char testMinigameName07[] = "\013TEST NAME07";
char testMinigameName08[] = "\013TEST MINIGAME NAME08";
char testMinigameName09[] = "\013TEST MINIGAME NAME09";
char testMinigameName10[] = "\013TEST MINIGAME NAME10";
char testMinigameName11[] = "\013TEST MINIGAME NAME11";
char testMinigameName12[] = "\013TEST MINIGAME NAME12";
char testMinigameName13[] = "\013TEST MINIGAME NAME13";

static char *testMinigameNames[] = {
    testMinigameName01,
    testMinigameName02,
    testMinigameName03,
    testMinigameName04,
    testMinigameName05,
    testMinigameName06,
    testMinigameName07,
    testMinigameName08,
    testMinigameName09,
    testMinigameName10,
    testMinigameName11,
    testMinigameName12,
    testMinigameName13,
};

static float minigameWindowYOffsets[] = { -100.0f, -50.0f, 0.0f, 50.0f, 100.0f };

static s32 minigamePictureDataIds[] = { 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52 };

static void StartMGSelect(OMOBJ *object)
{
    HuAudSeqPlay(0x2B);
    minigameSelectionState = 0;
    minigameSelectionTimer = -1;
    minigameSelectionDelay = 0x1E;
    minigameSelectionIndex = 0;
    object->objFunc = UpdatePad;
    worstVcount = 0;
    object->work[0] = 0;
    object->work[1] = 0;
}

static void UpdatePad(OMOBJ *object)
{
    s32 i;
    s32 players;

    if (e3ExitEnableF == 0) {
        for (i = 0, players = 0; i < 4; i++) {
            E3PadUpdate(i, &e3PadData[i]);
            if (!e3PadData[i].enable) {
                players++;
            }
        }
        e3ConfigPlayerCnt = players;
    }
}

static void UpdateMGSelect(OMOBJ *object)
{
    MGSelectWork *selectionWork;
    s32 minigameInfoIndex;
    u32 overlayId;
    s16 selectionDelta;
    s16 windowIndex;
    s16 nextWindowPosition;
    s16 previousWindowPosition;
    u16 buttonInput;
    s16 windowId;
    s32 minigameId;
    float messageSize[2];

    selectionWork = object->data;
    buttonInput = e3PadData[e3PadCtrl].btnDown;
    if (object->work[3] != 0) {
        if ((buttonInput & 0x1100) != 0) {
            HuAudFXPlay(1);
            defaultMinigameIndex = selectionWork->selectedMinigameIndex;
            minigameId = minigameIds[selectionWork->selectedMinigameIndex];
            GWMGAvailSet(minigameId);
            GWSystem.mg_next = minigameId - 401;
            e3NextOvl = 3;
            e3NextEvent = 0;
            e3ExitMode = 2;
            e3ExitEnableF = 1;
            overlayId = minigameOverlayIds[selectionWork->selectedMinigameIndex];

            for (minigameInfoIndex = 0; minigameInfoIndex < 50; minigameInfoIndex++) {
                if (overlayId == mgInfoTbl[minigameInfoIndex].ovl) {
                    break;
                }
            }
            if (minigameInfoIndex >= 50) {
                e3ExitMode = 0;
                e3NextOvl = 0;
            };
            switch (mgInfoTbl[minigameInfoIndex].type) {
            case 0:
            case 3:
            case 4:
            case 5:
            case 6:
                for (minigameInfoIndex = 0; minigameInfoIndex < 4; minigameInfoIndex++) {
                    GWPlayerCfg[minigameInfoIndex].group = minigameInfoIndex;
                }
                break;
            case 1:
                overlayId = frand();
                overlayId &= 3;
                for (minigameInfoIndex = 0; minigameInfoIndex < 4; minigameInfoIndex++) {
                    if (overlayId == minigameInfoIndex) {
                        GWPlayerCfg[minigameInfoIndex].group = 0;
                    } else {
                        GWPlayerCfg[minigameInfoIndex].group = 1;
                    }
                }
                break;
            case 2:
                for (minigameInfoIndex = 0; minigameInfoIndex < 2; minigameInfoIndex++) {
                    GWPlayerCfg[minigameInfoIndex].group = 0;
                }
                for (minigameInfoIndex = 2; minigameInfoIndex < 4; minigameInfoIndex++) {
                    GWPlayerCfg[minigameInfoIndex].group = 1;
                }
                break;
            }
            object->work[3] = 0;
            return;
        }
        if ((buttonInput & 0x200) != 0) {
            HuAudFXPlay(3);
            e3ExitMode = 0;
            e3NextOvl = 0;
            e3NextEvent = 0;
            e3ExitEnableF = 1;
            object->work[3] = 0;
            return;
        }
        if (object->work[3] != 0) {
            if ((buttonInput & 8) != 0) {
                HuAudFXPlay(0);
                selectionWork->selectedMinigameIndex--;
                if (selectionWork->selectedMinigameIndex < 0) {
                    selectionWork->selectedMinigameIndex = 0xC;
                }
            } else if ((buttonInput & 4) != 0) {
                HuAudFXPlay(0);
                selectionWork->selectedMinigameIndex++;
                if (selectionWork->selectedMinigameIndex >= 13) {
                    selectionWork->selectedMinigameIndex = 0;
                }
            }
            if (selectionWork->previousMinigameIndex != selectionWork->selectedMinigameIndex) {
                mgPicObj->work[0] = selectionWork->selectedMinigameIndex;
                selectionDelta = selectionWork->selectedMinigameIndex - selectionWork->previousMinigameIndex;
                if ((selectionDelta > 1) || (selectionDelta < -1)) {
                    if (selectionDelta < 0) {
                        selectionDelta = 1;
                    } else {
                        selectionDelta = -1;
                    }
                }
                nextWindowPosition = selectionWork->selectedWindowPosition + selectionDelta;
                if ((nextWindowPosition < 0) || (nextWindowPosition >= 5)) {
                    for (minigameInfoIndex = 0; minigameInfoIndex < 5; minigameInfoIndex++) {
                        windowId = selectionWork->minigameWindows[minigameInfoIndex];
                        windowIndex = selectionWork->minigameWindowPositions[minigameInfoIndex];
                        previousWindowPosition = windowIndex - selectionDelta;
                        if ((previousWindowPosition < 0) || (previousWindowPosition >= 5)) {
                            overlayId = minigameMessageIds[selectionWork->selectedMinigameIndex];
                            HuWinMesSizeCancelCRSet(1);
                            HuWinMesMaxSizeGet(1, messageSize, overlayId);
                            HuWinCenterPosSet(windowId, (296.0f - messageSize[0]) / -2.0f, (68.0f - messageSize[1]) / -2.0f);
                            HuWinMesSet(windowId, overlayId);
                            if (previousWindowPosition < 0) {
                                windowIndex = 4;
                            } else {
                                windowIndex = 0;
                            }
                        } else {
                            windowIndex = previousWindowPosition;
                        }
                        HuWinPosSet(windowId, 36.0f, 237.0f + minigameWindowYOffsets[windowIndex]);
                        selectionWork->minigameWindowPositions[minigameInfoIndex] = windowIndex;
                    }
                } else {
                    selectionWork->selectedWindowPosition = nextWindowPosition;
                    HuSprPosSet(selectionWork->cursorSpriteGroup, 1, 0.0f, minigameWindowYOffsets[nextWindowPosition]);
                }
                selectionWork->previousMinigameIndex = selectionWork->selectedMinigameIndex;
            }
        }
    }
}

static void CreateMGSelect(OMOBJ *object)
{
    MGSelectWork *selectionWork;
    s16 spriteGroup;
    s16 spriteId;
    s32 minigameIndex;
    s16 selectedMinigame;
    s16 index2;
    u32 messageId;
    s32 minigameId;
    ANIMDATA *spriteAnimation;
    ANIMDATA *alternateSpriteAnimation;
    float messageSize[2];

    omSetStatBit(object, OM_STAT_MODELPAUSE);
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(MGSelectWork), HU_MEMNUM_OVL);
    selectionWork = object->data;
    index2 = 0;
    spriteGroup = HuSprGrpCreate(3);
    selectionWork->cursorSpriteGroup = spriteGroup;
    HuSprGrpPosSet(spriteGroup, 174.0f, 271.0f);
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_E3SETUP, 0x21), HU_MEMNUM_OVL));
    selectionWork->animations[index2] = spriteAnimation;
    spriteId = HuSprCreate(spriteAnimation, 0x7540, 0);
    HuSprGrpMemberSet(spriteGroup, index2, spriteId);
    index2++;
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_E3SETUP, 0x27), HU_MEMNUM_OVL));
    selectionWork->animations[index2] = spriteAnimation;
    spriteId = HuSprCreate(spriteAnimation, 0x64, 0);
    HuSprGrpMemberSet(spriteGroup, index2, spriteId);
    HuSprScaleSet(spriteGroup, index2, 1.1f, 1.0f);
    HuSprTPLvlSet(spriteGroup, index2, 0.5f);
    selectionWork->selectedWindowPosition = 0;
    HuSprPosSet(spriteGroup, index2, 0.0f, minigameWindowYOffsets[selectionWork->selectedWindowPosition]);
    index2++;
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_E3SETUP, 0x35), HU_MEMNUM_OVL));
    selectionWork->animations[index2++] = spriteAnimation;
    alternateSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_E3SETUP, 0x36), HU_MEMNUM_OVL));
    selectionWork->animations[index2++] = alternateSpriteAnimation;
    spriteGroup = HuSprGrpCreate(2);
    selectionWork->previousArrowGroup = spriteGroup;
    HuSprGrpPosSet(spriteGroup, 174.0f, 128.0f);
    spriteId = HuSprCreate(spriteAnimation, 4, 0);
    HuSprGrpMemberSet(spriteGroup, 0, spriteId);
    spriteId = HuSprCreate(alternateSpriteAnimation, 6, 0);
    HuSprGrpMemberSet(spriteGroup, 1, spriteId);
    HuSprTPLvlSet(spriteGroup, 1, 0.7f);
    HuSprPosSet(spriteGroup, 1, 0.0f, -4.0f);
    spriteGroup = HuSprGrpCreate(2);
    selectionWork->nextArrowGroup = spriteGroup;
    HuSprGrpPosSet(spriteGroup, 174.0f, 414.0f);
    spriteId = HuSprCreate(spriteAnimation, 4, 1);
    HuSprGrpMemberSet(spriteGroup, 0, spriteId);
    spriteId = HuSprCreate(alternateSpriteAnimation, 6, 1);
    HuSprGrpMemberSet(spriteGroup, 1, spriteId);
    HuSprTPLvlSet(spriteGroup, 1, 0.7f);
    HuSprPosSet(spriteGroup, 1, 0.0f, -4.0f);

    minigameId = GWSystem.mg_next + 401;
    selectedMinigame = minigameId;
    for (minigameIndex = 0; minigameIndex < 13;) {
        if (selectedMinigame != minigameIds[minigameIndex]) {
            minigameIndex++;
        } else {
            break;
        }
    }
    if (minigameIndex >= 13) {
        minigameIndex = 0;
    }
    defaultMinigameIndex = minigameIndex;
    selectedMinigame = defaultMinigameIndex;
    mgPicObj->work[0] = selectedMinigame;
    selectionWork->selectedMinigameIndex = selectedMinigame;
    selectionWork->previousMinigameIndex = selectedMinigame;

    for (minigameIndex = 0; minigameIndex < 5;) {
        if (selectedMinigame >= 13) {
            selectedMinigame = 0;
        }
        spriteId = HuWinCreate(36.0f, 237.0f + minigameWindowYOffsets[minigameIndex], 0x128, 0x44, 0x4E20);
        selectionWork->minigameWindows[minigameIndex] = spriteId;
        selectionWork->minigameWindowPositions[minigameIndex] = minigameIndex;
        HuWinBGTPLvlSet(spriteId, 0.0f);
        HuWinMesSpeedSet(spriteId, 0);
        messageId = minigameMessageIds[selectedMinigame];
        HuWinAttrSet(spriteId, 0x100);
        HuWinMesSizeCancelCRSet(1);
        HuWinMesMaxSizeGet(1, messageSize, messageId);
        HuWinCenterPosSet(spriteId, (296.0f - messageSize[0]) / -2.0f, (68.0f - messageSize[1]) / -2.0f);
        HuWinMesSet(spriteId, messageId);
        minigameIndex++;
        selectedMinigame++;
    }
    object->work[3] = 1;
    object->objFunc = UpdateMGSelect;
}

static void UpdateMGPic(OMOBJ *object)
{
    MGPictureWork *pictureWork = (MGPictureWork *)object->data;
    s16 spriteGroup;

    if (object->work[1] != object->work[0]) {
        spriteGroup = pictureWork->spriteGroup;
        HuSprAttrSet(spriteGroup, object->work[1], HUSPR_ATTR_DISPOFF);
        HuSprAttrReset(spriteGroup, object->work[0], HUSPR_ATTR_DISPOFF);
        object->work[1] = object->work[0];
    }
}

static void CreateMGPic(OMOBJ *object)
{
    MGPictureWork *pictureWork;
    ANIMDATA *spriteAnimation;
    s16 spriteGroup;
    s32 minigameIndex;
    s32 index;
    s16 index2;
    s16 group;

    omSetStatBit(object, OM_STAT_MODELPAUSE);
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(MGPictureWork), HU_MEMNUM_OVL);
    pictureWork = object->data;
    index2 = 0;
    spriteGroup = HuSprGrpCreate(14);
    pictureWork->spriteGroup = spriteGroup;
    HuSprGrpPosSet(spriteGroup, 444.0f, 228.0f);

    for (minigameIndex = 0; minigameIndex < 13; minigameIndex++) {
        spriteAnimation = HuSprAnimRead(HuDataReadNum(minigamePictureDataIds[minigameIndex], HU_MEMNUM_OVL));
        pictureWork->animations[minigameIndex] = spriteAnimation;
        group = HuSprCreate(spriteAnimation, 0x2710, 0);
        HuSprGrpMemberSet(spriteGroup, minigameIndex, group);
        HuSprScaleSet(spriteGroup, minigameIndex, 0.6f, 0.6f);
        HuSprAttrSet(spriteGroup, minigameIndex, 4);
    }
    index2 = minigameIndex;
    spriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_E3SETUP, 0x22), HU_MEMNUM_OVL));
    pictureWork->animations[index2] = spriteAnimation;
    group = HuSprCreate(spriteAnimation, 0x7540, 0);
    HuSprGrpMemberSet(spriteGroup, index2, group);
    HuSprTPLvlSet(spriteGroup, index2, 0.7f);
    index2++;
    object->work[1] = object->work[0];
    HuSprAttrReset(spriteGroup, object->work[1], HUSPR_ATTR_DISPOFF);
    object->objFunc = UpdateMGPic;
}

static void UpdateMGInterface(OMOBJ *object) { void *interfaceData = object->data; }

static void CreateMGInterface(OMOBJ *object)
{
    MGInterfaceWork *interfaceWork;
    s16 index;
    s16 group;
    s16 index2;
    ANIMDATA *interfaceSpriteAnimation;
    float messageSize[2];

    omSetStatBit(object, OM_STAT_MODELPAUSE);
    object->data = HuMemDirectMallocNum(HEAP_HEAP, sizeof(MGInterfaceWork), HU_MEMNUM_OVL);
    interfaceWork = object->data;
    index2 = 0;
    group = HuSprGrpCreate(1);
    interfaceWork->titleSpriteGroup = group;
    HuSprGrpPosSet(group, 288.0f, 64.0f);
    interfaceSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_E3SETUP, 0x24), HU_MEMNUM_OVL));
    interfaceWork->animations[index2] = interfaceSpriteAnimation;
    index = HuSprCreate(interfaceSpriteAnimation, 0x2710, 0);
    HuSprGrpMemberSet(group, 0, index);
    index2++;
    group = HuSprGrpCreate(1);
    interfaceWork->promptSpriteGroup = group;
    HuSprGrpPosSet(group, 444.0f, 384.0f);
    interfaceSpriteAnimation = HuSprAnimRead(HuDataReadNum(DATA_MAKE_NUM(DATADIR_E3SETUP, 0x23), HU_MEMNUM_OVL));
    interfaceWork->animations[index2] = interfaceSpriteAnimation;
    index = HuSprCreate(interfaceSpriteAnimation, 0x271A, 0);
    HuSprGrpMemberSet(group, 0, index);
    index2++;
    HuWinMesMaxSizeGet(1, messageSize, "\013\016\r PRESS START");
    index = HuWinCreate(340.0f, 362.0f, messageSize[0], messageSize[1], 0);
    interfaceWork->pressStartWindow = index;
    HuWinMesColSet(index, 0);
    HuWinBGTPLvlSet(index, 0.0f);
    HuWinMesSpeedSet(index, 0);
    HuWinMesSet(index, MAKE_MESSID_PTR("\013\016\r PRESS START"));
    object->objFunc = UpdateMGInterface;
}

#include "game/gamework_data.h"

#include "game/armem.h"
#include "game/audio.h"
#include "game/data.h"
#include "game/disp.h"
#include "game/dvd.h"
#include "game/memory.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"

#include "ext_math.h"
#include <stdarg.h>
#include <string.h>

#include "data_num/win.h"

typedef struct {
    /* 0x00 */ ANIMDATA **anim;
    /* 0x04 */ s16 bank;
    /* 0x06 */ s16 w;
    /* 0x08 */ s16 h;
    /* 0x0A */ s16 center_x;
    /* 0x0C */ s16 center_y;
} spcFontTblData;

typedef struct {
    /* 0x00 */ s16 time;
    /* 0x04 */ u32 player[4];
} keyBufData; // Size 0x14

static void MesDispFunc(HUSPRITE *sprite);
static u8 winBGMake(ANIMDATA *bg, ANIMDATA *frame);
static void HuWinProc(void);
static void HuWinDrawMes(s16 window);
static s32 HuWinCR(WindowData *window);
static void _HuWinHomeClear(WindowData *window);
static void HuWinKeyWait(s16 window);
static s16 HuWinSpcFontEntry(WindowData *window, s16 entry, s16 x, s16 y);
static void HuWinSpcFontClear(WindowData *window);
static void HuWinChoice(WindowData *window);
static void GetMesMaxSizeSub(u32 mess);
static s32 GetMesMaxSizeSub2(WindowData *window, u8 *mess_data);

void mtxTransCat(Mtx, float, float, float);
WindowData ATTRIBUTE_ALIGN(32) winData[32];
u32 winKey[4];
keyBufData winComKeyBuf[256];

static ANIMDATA *iconAnim;
static ANIMDATA *cursorAnim;
static ANIMDATA *cardAnimA;
static ANIMDATA *cardAnimB;
static HUPROCESS *winProc;
void *messDataPtr;
static s32 messDataNo;
static s16 winMaxWidth;
static s16 winMaxHeight;
#if VERSION_NTSC
static u8 mesWInsert[8];
#else
static u16 mesWInsert[8];
#endif

static u8 winTabSize;
static u8 winInsertF;
static u32 winAMemP;
static u8 *fontWidthP;
static s32 cancelCRF;
static s16 mesCharCnt;
static s16 comKeyIdxNow;
static s16 comKeyIdx;
static u8 LanguageNo;
ANIMDATA *fontAnim;

static spcFontTblData spcFontTbl[] = { { &iconAnim, 0, 20, 24, 10, 12 }, { &iconAnim, 1, 20, 24, 10, 12 }, { &iconAnim, 2, 20, 24, 10, 12 },
    { &iconAnim, 3, 20, 24, 10, 12 }, { &iconAnim, 4, 20, 24, 10, 12 }, { &iconAnim, 5, 20, 24, 10, 12 }, { &iconAnim, 6, 20, 24, 10, 12 },
    { &iconAnim, 7, 20, 24, 10, 12 }, { &iconAnim, 8, 20, 24, 10, 12 }, { &iconAnim, 9, 20, 24, 10, 12 }, { &iconAnim, 10, 20, 24, 10, 12 },
    { &iconAnim, 11, 30, 24, 15, 12 }, { &iconAnim, 12, 20, 24, 10, 12 }, { &iconAnim, 13, 20, 24, 10, 12 }, { &iconAnim, 14, 20, 24, 10, 12 },
    { &iconAnim, 15, 20, 24, 10, 12 }, { &iconAnim, 16, 20, 24, 10, 12 }, { &iconAnim, 17, 20, 24, 10, 12 }, { &iconAnim, 18, 20, 24, 10, 12 },
    { &iconAnim, 19, 24, 24, 12, 12 }, { &cursorAnim, 0, 40, 32, -15, 18 }, { &cardAnimA, 0, 32, 32, 16, 16 }, { &cardAnimB, 0, 32, 32, 16, 16 } };

#if VERSION_ENG
u8 charWETbl[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 18, 20, 12, 12, 11, 14, 8, 13, 12, 12, 12, 12, 12, 12, 12, 9, 11, 12, 11, 15, 12, 13, 12,
    13, 12, 12, 11, 12, 11, 15, 12, 13, 11, 12, 6, 8, 8, 12, 20, 12, 11, 12, 11, 11, 9, 12, 11, 4, 8, 11, 4, 14, 11, 12, 11, 12, 9, 11, 9, 11, 11, 15,
    11, 11, 11, 4, 13, 8, 14, 12, 9, 8, 8, 8, 20, 4, 12, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 12, 12, 12, 12, 12, 12, 12, 12, 8, 8, 12, 12, 12, 12,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 12, 12, 7, 14,
    17, 13, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 16, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20 };
#elif VERSION_JP
u8 charWETbl[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 13, 8, 11, 12, 13, 12, 11, 11, 12, 11, 18, 20, 12, 12, 11, 14, 8, 13, 12, 12, 12, 12, 12, 12, 12, 9, 11, 12, 11, 15, 12, 13, 12,
    13, 12, 12, 11, 12, 11, 15, 12, 13, 11, 12, 6, 8, 8, 12, 20, 12, 11, 12, 11, 11, 9, 12, 11, 4, 8, 11, 4, 14, 11, 12, 11, 12, 9, 11, 9, 11, 11, 15,
    11, 11, 11, 4, 13, 8, 14, 12, 9, 8, 8, 8, 20, 4, 12, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 12, 12, 12, 12, 12, 12, 12, 12, 8, 8, 12, 12, 12, 12,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 12, 12, 7, 14,
    17, 13, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 16, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20 };

#else
u8 charWETbl[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 18, 20, 12, 12, 11, 14, 8, 13, 12, 12, 12, 12, 12, 12, 12, 9, 11, 12, 11, 15, 12, 13, 12,
    13, 12, 12, 11, 12, 11, 15, 12, 13, 11, 12, 6, 8, 8, 12, 20, 12, 11, 12, 11, 11, 9, 12, 11, 4, 8, 11, 4, 14, 11, 12, 11, 12, 9, 11, 9, 11, 11, 15,
    11, 11, 11, 4, 13, 8, 14, 12, 9, 8, 8, 8, 20, 4, 12, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 12, 12, 12, 12, 12, 12, 12, 12, 8, 8, 12, 12, 12, 12,
    12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 13, 13, 6, 12,
    18, 14, 14, 8, 20, 20, 20, 20, 20, 20, 20, 20, 20, 10, 10, 10, 12, 12, 12, 12, 12, 10, 10, 12, 12, 10, 10, 10, 10, 7, 7, 12, 12, 10, 12, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 16, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20 };

#endif

u8 charWJTbl[] = { 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 0, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 0, 0, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20 };

static u8 ATTRIBUTE_ALIGN(32) charColPal[2 * 3 * 10] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
    0x00, 0xFF, 0xFF, 0xFF, 0xA0, 0x00, 0xFF, 0xFF, 0xFF, 0x60, 0x60, 0x60, 0x90, 0x90, 0x90, 0x00, 0x00, 0x00, 0x60, 0xB0, 0xFF, 0xFF, 0x40, 0x80,
    0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x60, 0x60, 0x60, 0x90, 0x90, 0x90 };

static s32 frameFileTbl[] = { WIN_FRAME1_ANM, WIN_FRAME2_ANM, WIN_FRAME3_ANM, WIN_FRAME1_ANM };

#if VERSION_NTSC
static char *mesDataTbl[] = { "mess/mini.dat", "mess/board.dat", "mess/mini_e.dat", "mess/board_e.dat" };
#else
static char *mesDataTbl[] = { "mess/mini.dat", "mess/board.dat", "mess/mini_e.dat", "mess/board_e.dat", "mess/mini_g.dat", "mess/board_g.dat",
    "mess/mini_f.dat", "mess/board_f.dat", "mess/mini_i.dat", "mess/board_i.dat", "mess/mini_s.dat", "mess/board_s.dat" };
#endif

static s32 winVoiceTbl[]
    = { 0x37, 0x36, 0x38, 0x44, 0x43, 0x45, 0x41, 0x40, 0x42, 0x4C, 0x4B, 0x4D, 0x47, 0x46, 0x48, 0x3E, 0x3E, 0x3F, 0x49, 0x49, 0x49 };

static s16 winPrio = 1000;

/**
 * @brief Initializes the message subsystem, sprite manager, fonts, palette, and window slots.
 */
void HuWindowInit(void)
{
    s16 i;

    winAMemP = HuAR_DVDtoARAM(DATADIR_WIN);
    for (i = 0; i < 32; i++) {
        winData[i].group = -1;
    }
    winProc = 0;
    winPrio = 1000;
}

/**
 * @brief Loads a message-data set and resets all window slots.
 * @param mess_data_no Index into mesDataTbl selecting the regional message archive copied into messDataPtr.
 */
void HuWinInit(s32 mess_data_no)
{
    s16 i;
    void *anim_data;

    if (!winProc) {
        HuAR_ARAMtoMRAM(winAMemP);
        winProc = HuPrcCreate(HuWinProc, 0x64, 0x1000, 0);
        HuPrcSetStat(winProc, 0xC);
        LanguageNo = GWLanguageGet();
        messDataNo = mess_data_no;
        fontWidthP = (LanguageNo == 0) ? charWJTbl : charWETbl;
        HuWinMesRead(mess_data_no);
        for (i = 0; i < 32; i++) {
            winData[i].group = -1;
        }
        if (!fontAnim) {
            if (LanguageNo == 0) {
                anim_data = HuDataReadNum(WIN_FONTJ_ANM, HU_MEMNUM_OVL);
            }
            else {
                anim_data = HuDataReadNum(WIN_FONTE_ANM, HU_MEMNUM_OVL);
            }
            fontAnim = HuSprAnimRead(anim_data);
        }
        if (!iconAnim) {
            anim_data = HuDataReadNum(WIN_ICON_ANM, HU_MEMNUM_OVL);
            iconAnim = HuSprAnimRead(anim_data);
            HuSprAnimLock(iconAnim);
        }
        if (!cursorAnim) {
            anim_data = HuDataReadNum(WIN_CURSOR_ANM, HU_MEMNUM_OVL);
            cursorAnim = HuSprAnimRead(anim_data);
            HuSprAnimLock(cursorAnim);
        }
        if (!cardAnimA) {
            anim_data = HuDataReadNum(WIN_CARDA_ANM, HU_MEMNUM_OVL);
            cardAnimA = HuSprAnimRead(anim_data);
            HuSprAnimLock(cardAnimA);
        }
        if (!cardAnimB) {
            anim_data = HuDataReadNum(WIN_CARDB_ANM, HU_MEMNUM_OVL);
            cardAnimB = HuSprAnimRead(anim_data);
            HuSprAnimLock(cardAnimB);
        }
        HuDataDirClose(DATADIR_WIN);
        HuWinComKeyReset();
        winPrio = 1000;
    }
}

/**
 * @brief Allocates a window, its character buffer, background group, and cursor sprite.
 * @param x Left screen coordinate in pixels, or -10000.0 to center horizontally.
 * @param y Top screen coordinate in pixels, or -10000.0 to center vertically.
 * @param w Requested width in pixels, rounded up to a multiple of 16.
 * @param h Requested height in pixels, rounded up to a multiple of 16.
 * @param frame Frame style index from 0 through 3; out-of-range values select style 0.
 * @return Allocated window index, or -1 when no slot is available.
 */
s16 HuWinCreate(float x, float y, s16 w, s16 h, s16 frame)
{
    ANIMDATA *bg_anim;
    WindowData *window;
    HUSPRITE *sprite_ptr;
    s16 group;
    s16 sprite;
    s16 window_id;
    s16 i;
    void *frame_data;

    for (window_id = 0; window_id < 32; window_id++) {
        if (winData[window_id].group == -1) {
            break;
        }
    }
    if (window_id == 32) {
        return -1;
    }
    window = &winData[window_id];
    window->group = group = HuSprGrpCreate(30);
    if (frame < 0 || frame >= 4) {
        frame = 0;
    }
    w = (w + 15) & 0xFFF0;
    h = (h + 15) & 0xFFF0;
    if (x == -10000.0f) {
        window->pos_x = (HU_DISP_WIDTHF - w) / 2;
    }
    else {
        window->pos_x = x;
    }
    if (y == -10000.0f) {
        window->pos_y = (HU_DISP_HEIGHTF - h) / 2;
    }
    else {
        window->pos_y = y;
    }
    HuSprGrpCenterSet(group, w / 2, h / 2);
    HuSprGrpPosSet(group, window->pos_x, window->pos_y);
    frame_data = HuAR_ARAMtoMRAMFileRead(frameFileTbl[frame], HU_MEMNUM_OVL, HEAP_MODEL);
    window->frame = HuSprAnimRead(frame_data);
    sprite = window->sprite_id[0] = HuSprCreate(window->frame, winPrio, 0);
    HuSprGrpMemberSet(group, 0, sprite);
    HuSprTPLvlSet(group, 0, 0.9f);
    bg_anim = HuSprAnimMake(w / 16, h / 16, 6);
    HuSprBGSet(group, 0, bg_anim, 0);
    window->color_key = winBGMake(bg_anim, window->frame);
    sprite = window->sprite_id[1] = HuSprFuncCreate(MesDispFunc, winPrio);
    sprite_ptr = &HuSprData[sprite];
    sprite_ptr->work[0] = window_id;
    HuSprGrpMemberSet(group, 1, sprite);
    window->num_chars = 0;
#if VERSION_JP
    window->max_chars = (w / 20) * (h / 24) * 3;
#else
    window->max_chars = (w / 8) * (h / 24) * 4;
#endif
    window->char_data = HuMemDirectMalloc(HEAP_HEAP, window->max_chars * sizeof(WinChar));
    window->attr = 0;
    window->stat = 0;
    window->unk8C = 0;
    window->mess_time = 0;
    window->mess_x = window->mess_y = 0;
    window->mess_rect_x = 8;
    window->mess_rect_y = 8;
    window->mess_rect_w = w - 8;
    window->mess_rect_h = h - 8;
    window->speed = 1;
    window->mess_color = 7;
    window->mess_shadow_color = 0;
    window->active_pad = 0xF;
    window->player_disable = 0;
    window->mess_stackptr = 0;
    window->mess = 0;
    window->num_choices = 0;
    window->prio = winPrio;
    window->spacing_x = 1;
    window->spacing_y = 2;
    window->w = w;
    window->h = h;
    window->scissor_x = window->scissor_y = 0;
    window->scissor_w = 0x280;
    window->scissor_h = 0x1E0;
    window->tab_w = 24;
    window->push_key = 0x300;
#if !VERSION_JP
    window->key_auto = 0;
#endif
    if (frame == 0 || frame == 2) {
        memcpy(&window->mess_pal, &charColPal[30], 30);
    }
    else {
        memcpy(&window->mess_pal, charColPal, 30);
        window->mess_shadow_color = 9;
    }
    for (i = 0; i < 8; i++) {
        window->insert_mess[i] = NULL;
    }
    for (i = 2; i < 30; i++) {
        window->sprite_id[i] = -1;
    }
    winPrio -= 3;
    if (winPrio < 500) {
        winPrio = 1000;
    }
    for (i = 0; i < 16; i++) {
        window->choice_disable[i] = 0;
        window->choice_data[i].stat = 0;
    }
    return window_id;
}

/**
 * @brief Destroys one window and all sprites and buffers owned by it.
 * @param window Active winData slot from 0 through 31 whose group, sprites, and character buffer are released.
 */
void HuWinKill(s16 window)
{
    WindowData *window_ptr = &winData[window];
    s16 i;

    if (window_ptr->group != -1) {
        HuMemDirectFree(window_ptr->char_data);
        for (i = 2; i < 30; i++) {
            if (window_ptr->sprite_id[i] != -1) {
                HuSprGrpMemberKill(window_ptr->group, i);
            }
        }
        HuSprGrpKill(window_ptr->group);
        window_ptr->group = -1;
    }
}

/**
 * @brief Destroys every active window.
 */
void HuWinAllKill(void)
{
    s16 i;

    for (i = 0; i < 32; i++) {
        if (winData[i].group != -1) {
            HuWinKill(i);
        }
    }
    if (fontAnim != 0) {
        HuSprAnimKill(fontAnim);
        fontAnim = 0;
    }
    if (iconAnim != 0) {
        HuSprAnimKill(iconAnim);
        iconAnim = 0;
    }
    if (cursorAnim != 0) {
        HuSprAnimKill(cursorAnim);
        cursorAnim = 0;
    }
    if (cardAnimA != 0) {
        HuSprAnimKill(cardAnimA);
        cardAnimA = 0;
    }
    if (cardAnimB != 0) {
        HuSprAnimKill(cardAnimB);
        cardAnimB = 0;
    }
    if (winProc != 0) {
        HuPrcKill(winProc);
        winProc = 0;
    }
    if (messDataPtr != 0) {
        HuMemDirectFree(messDataPtr);
        messDataPtr = 0;
    }
    HuDataDirClose(DATADIR_WIN);
}

/**
 * @brief Draws all visible glyph quads for one window message sprite.
 * @param sprite HUSPR_ATTR_FUNC sprite whose work[0] selects the winData slot supplying glyphs and palette data.
 */
static void MesDispFunc(HUSPRITE *sprite)
{
    WindowData *window = &winData[sprite->work[0]];
    HUSPRGRP *group;
    float uv_maxx;
    float uv_maxy;
    float uv_minx;
    float uv_miny;
    float char_w;
    float char_x;
    float char_y;
    float char_uv_h;
    s16 i;
    u16 alpha;
    s16 color;
    Mtx scale;
    Mtx modelview;
    Mtx44 proj;

    if (window->num_chars != 0) {
        group = &HuSprGrpData[window->group];
        GXInvalidateTexAll();
        C_MTXOrtho(proj, 0.0f, HU_DISP_HEIGHTF, 0.0f, HU_DISP_WIDTHF, 0.0f, 10.0f);
        GXSetProjection(proj, GX_ORTHOGRAPHIC);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_RGBA6, 0);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_RGBA6, 0);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGBA8, 0);
        GXSetCullMode(GX_CULL_NONE);
        GXSetNumTexGens(1);
        GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
        GXSetNumTevStages(1);
        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
        GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_RASA, GX_CA_TEXA, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1, GX_TEVPREV);
        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, GX_SRC_VTX, 1, GX_DF_CLAMP, GX_AF_SPOT);
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
        GXSetAlphaCompare(GX_GEQUAL, 1, GX_AOP_AND, GX_GEQUAL, 1);
        GXSetZCompLoc(0);
        MTXRotRad(modelview, 'z', MTXDegToRad(sprite->zRot));
        MTXScale(scale, sprite->scale.x, sprite->scale.y, 1.0f);
        MTXConcat(modelview, scale, modelview);
        mtxTransCat(modelview, sprite->pos.x, sprite->pos.y, 0.0f);
        MTXConcat(*sprite->groupMtx, modelview, modelview);
        GXLoadPosMtxImm(modelview, 0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        HuSprTexLoad(fontAnim, 0, 0, GX_CLAMP, GX_CLAMP, VERSION_JP ? GX_NEAR : GX_LINEAR);
        GXBegin(GX_QUADS, GX_VTXFMT0, window->num_chars * 4);
        char_uv_h = (LanguageNo == 0) ? (24.0f / 408.0f) : (24.0f / 312.0f);
        for (i = 0; i < window->num_chars; i++) {
            char_w = fontWidthP[window->char_data[i].character + 48];
#if VERSION_JP
            uv_minx = 0.003125 + (20.0f / 320.0f) * (window->char_data[i].character % 16);
#else
            uv_minx = (20.0f / 320.0f) * (window->char_data[i].character % 16);
#endif
            uv_miny = char_uv_h * (window->char_data[i].character / 16);
            uv_maxx = uv_minx + (char_w / 320.0);
            uv_maxy = uv_miny + char_uv_h;
            char_x = window->char_data[i].x;
            char_y = window->char_data[i].y;
            color = window->char_data[i].color;
            if (window->speed != 0) {
                alpha = window->char_data[i].fade * 8;
            }
            else {
                alpha = 255;
            }
            alpha = 255;
            if (window->char_data[i].fade < 31) {
                window->char_data[i].fade++;
            }
            GXPosition3f32(char_x + 1.0f, char_y, 0.0f);
            GXColor4u8(window->mess_pal[color][0], window->mess_pal[color][1], window->mess_pal[color][2], alpha);
            GXPosition2f32(uv_minx, uv_miny);
            GXPosition3f32(char_x + char_w, char_y, 0.0f);
            GXColor4u8(window->mess_pal[color][0], window->mess_pal[color][1], window->mess_pal[color][2], alpha);
            GXPosition2f32(uv_maxx, uv_miny);
            GXPosition3f32(char_x + char_w, char_y + 24.0f, 0.0f);
            GXColor4u8(window->mess_pal[color][0], window->mess_pal[color][1], window->mess_pal[color][2], alpha);
            GXPosition2f32(uv_maxx, uv_maxy);
            GXPosition3f32(char_x + 1.0f, char_y + 24.0f, 0.0f);
            GXColor4u8(window->mess_pal[color][0], window->mess_pal[color][1], window->mess_pal[color][2], alpha);
            GXPosition2f32(uv_minx, uv_maxy);
        }
        GXEnd();
        mesCharCnt++;
    }
}

/**
 * @brief Builds a nine-slice window background animation from frame and fill images.
 * @param bg Destination background animation whose indexed pixel buffer is allocated and filled.
 * @param frame Frame animation supplying the palette to patch for the generated background.
 * @return Palette index whose original color matched 0xFC00, or frame->bmp->palNum when no entry matched.
 */
static u8 winBGMake(ANIMDATA *bg, ANIMDATA *frame)
{
    ANIMBMP *bmp;
    s16 block_h;
    s16 h;
    s16 block_w;
    s16 w;
    s16 i;
    s16 j;
    u8 *bmp_data;
    u16 *palette;

    w = bg->bmp->sizeX;
    h = bg->bmp->sizeY;
    block_w = (w + 7) & 0xF8;
    block_h = (h + 3) & 0xFC;
    bmp_data = bg->bmp->data = HuMemDirectMallocNum(HEAP_HEAP, block_w * block_h, HU_MEMNUM_OVL);
    for (i = 0; i < h; i++) {
        if (i == 0) {
            for (j = 0; j < w; j++) {
                if (j == 0) {
                    bmp_data[(j & 7) + ((j >> 3) << 5) + (i >> 2) * (block_w * 4) + (i & 3) * 8] = 0;
                }
                else if (j == w - 1) {
                    bmp_data[(j & 7) + ((j >> 3) << 5) + (i >> 2) * (block_w * 4) + (i & 3) * 8] = 0x10;
                }
                else {
                    bmp_data[(j & 7) + ((j >> 3) << 5) + (i >> 2) * (block_w * 4) + (i & 3) * 8] = 0x70;
                }
            }
        }
        else if (i == h - 1) {
            for (j = 0; j < w; j++) {
                if (j == 0) {
                    bmp_data[(j & 7) + ((j >> 3) << 5) + (i >> 2) * (block_w * 4) + (i & 3) * 8] = 0x20;
                }
                else if (j == w - 1) {
                    bmp_data[(j & 7) + ((j >> 3) << 5) + (i >> 2) * (block_w * 4) + (i & 3) * 8] = 0x30;
                }
                else {
                    bmp_data[(j & 7) + ((j >> 3) << 5) + (i >> 2) * (block_w * 4) + (i & 3) * 8] = 0x60;
                }
            }
        }
        else {
            for (j = 0; j < w; j++) {
                if (j == 0) {
                    bmp_data[(j & 7) + ((j >> 3) << 5) + (i >> 2) * (block_w * 4) + (i & 3) * 8] = 0x40;
                }
                else if (j == w - 1) {
                    bmp_data[(j & 7) + ((j >> 3) << 5) + (i >> 2) * (block_w * 4) + (i & 3) * 8] = 0x50;
                }
                else {
                    bmp_data[(j & 7) + ((j >> 3) << 5) + (i >> 2) * (block_w * 4) + (i & 3) * 8] = 0x80;
                }
            }
        }
    }
    DCStoreRangeNoSync(bg->bmp->data, block_w * block_h);
    bmp = frame->bmp;
    palette = bmp->palData;
    for (w = 0; w < bmp->palNum; w++, palette++) {
        if (*palette == 0xFC00) {
            *palette = 0x6114;
            break;
        }
    }
    DCStoreRangeNoSync(bmp->palData, 0x200);
    return w;
}

/**
 * @brief Advances message printing for every active window.
 */
static void HuWinProc(void)
{
    WindowData *window;
    s16 i;

    while (1) {
        HuPrcVSleep();
        window = winData;
        for (i = 0; i < 32; i++, window++) {
            if (window->group != -1 && !(window->attr & 8)) {
                switch (window->stat) {
                    case 0:
                        break;
                    case 1:
                        HuWinDrawMes(i);
                        break;
                    case 2:
                        HuWinComKeyGet(i, winKey);
                        HuWinKeyWait(i);
                        break;
                    case 3:
                        if (!(window->attr & 0x40) || HuWinActiveKeyGetX(window) == 0) {
                            window->attr &= ~0x40;
                            HuWinComKeyGet(i, winKey);
                            HuWinChoice(window);
                        }
                        break;
                }
            }
        }
    }
}

/**
 * @brief Appends one positioned and colored glyph to a window character buffer.
 * @param window Active winData slot from 0 through 31 whose next char_data entry is filled.
 * @param x Glyph X coordinate in pixels before conversion to window-centered coordinates.
 * @param y Glyph Y coordinate in pixels before conversion to window-centered coordinates.
 * @param char_idx Glyph code written to the character buffer.
 * @param color Message-palette index stored with the emitted glyph.
 */
static inline void charEntry(s16 window, s16 x, s16 y, s16 char_idx, s16 color)
{
    WindowData *window_ptr = &winData[window];
    WinChar *win_char = window_ptr->char_data;

    win_char = &window_ptr->char_data[window_ptr->num_chars];
    win_char->x = x - window_ptr->w / 2;
    win_char->y = y - window_ptr->h / 2;
    win_char->character = char_idx - 48;
    win_char->color = color;
    win_char->fade = 0;
    window_ptr->num_chars++;
    if (window_ptr->num_chars >= window_ptr->max_chars) {
        window_ptr->num_chars = window_ptr->max_chars - 1;
    }
}

/**
 * @brief Interprets message control codes and emits the next printable glyphs for a window.
 * @param window Active winData slot from 0 through 31 whose mess bytecode, cursor, and char_data are advanced.
 */
static void HuWinDrawMes(s16 window)
{
    WindowData *window_ptr = &winData[window];
    HUSPRGRP *group = &HuSprGrpData[window_ptr->group];
    s16 i;
    s16 char_w;
    s16 tab_w;
    s16 insert_mess;
    s16 mess_end;
    s16 c;

    s16 shadow_color;
    s16 color;
    s16 mess_w;

    window_ptr->mess_time += VERSION_JP ? 1 : 3;
    while (window_ptr->mess_time >= window_ptr->speed || (window_ptr->attr & 0x2000)) {
#if VERSION_JP
        window_ptr->mess_time = 0;
#else
        window_ptr->mess_time -= window_ptr->speed;
        if (window_ptr->mess_time < 0) {
            window_ptr->mess_time = 0;
        }
#endif
        char_w = window_ptr->spacing_x + fontWidthP[window_ptr->mess[0]];
        mess_end = 0;
        if (window_ptr->mess[0] != 0 && (window_ptr->attr & 4)) {
            window_ptr->attr &= ~4;
            _HuWinHomeClear(window_ptr);
        }
        while (window_ptr->mess[0] < 48 || window_ptr->mess[0] == 255) {
            switch (window_ptr->mess[0]) {
                case 0:
                    if (window_ptr->mess_stackptr == 0) {
                        for (i = 0; i < 16; i++) {
                            window_ptr->choice_disable[i] = 0;
                        }
                        window_ptr->stat = 0;
                        window_ptr->attr &= ~0x80;
                        return;
                    }
                    window_ptr->mess_stackptr--;
                    window_ptr->mess = window_ptr->mess_stack[window_ptr->mess_stackptr];
#if !VERSION_JP
                    window_ptr->mess_time = 0;
#endif
                    break;

                case 31:
                    window_ptr->mess++;
                    insert_mess = window_ptr->mess[0] - 1;
                    if (window_ptr->insert_mess[insert_mess] != 0) {
                        window_ptr->mess_stack[window_ptr->mess_stackptr] = window_ptr->mess;
                        window_ptr->mess = window_ptr->insert_mess[insert_mess];
                        window_ptr->mess_stackptr++;
                        if (window_ptr->mess[0] != 0xB) {
                            window_ptr->mess--;
                        }
                    }
                    break;

                case 11:
                    window_ptr->attr &= ~0x2200;
                    _HuWinHomeClear(window_ptr);
                    if (window_ptr->attr & 0x800) {
                        mess_w = GetMesMaxSizeSub2(window_ptr, window_ptr->mess + 1);
                        window_ptr->mess_x = (window_ptr->mess_rect_w - mess_w) / 2;
                    }
                    else if (window_ptr->attr & 0x1000) {
                        mess_w = GetMesMaxSizeSub2(window_ptr, window_ptr->mess + 1);
                        window_ptr->mess_x = window_ptr->mess_rect_w - mess_w;
                    }
                    break;

                case 30:
                    window_ptr->mess++;
                    if (!(window_ptr->attr & 0x80)) {
                        window_ptr->mess_color = window_ptr->mess[0] - 1;
                    }
                    break;

                case 29:
                    window_ptr->attr ^= 1;
                    break;

                case 10:
                    window_ptr->attr &= ~0x2020;
                    if (window_ptr->attr & 0x200) {
                        if (!(window_ptr->attr & 0x100)) {
                            if (HuWinCR(window_ptr) != 0) {
                                window_ptr->mess++;
                                HuWinKeyWaitEntry(window);
                                window_ptr->attr |= 2;
                                return;
                            }
                            if (window_ptr->attr & 0x800) {
                                mess_w = GetMesMaxSizeSub2(window_ptr, window_ptr->mess + 1);
                                window_ptr->mess_x = (window_ptr->mess_rect_w - mess_w) / 2;
                            }
                            else if (window_ptr->attr & 0x1000) {
                                mess_w = GetMesMaxSizeSub2(window_ptr, window_ptr->mess + 1);
                                window_ptr->mess_x = window_ptr->mess_rect_w - mess_w;
                            }
                            break;
                        }
                        char_w = fontWidthP[16] + window_ptr->spacing_x;
                            /* fallthrough */
                        case 16:
                        case 32:
                            window_ptr->attr |= 0x200;
                            if (window_ptr->mess_x + char_w > window_ptr->mess_rect_w) {
                                if (HuWinCR(window_ptr) != 0) {
                                    window_ptr->mess++;
                                    HuWinKeyWaitEntry(window);
                                    window_ptr->attr |= 2;
                                    return;
                                }
                                break;
                            }
                            window_ptr->mess_x += char_w;
                    }
                    break;

                case 14:
                    window_ptr->attr |= 0x200;
                    window_ptr->mess++;
                    tab_w = window_ptr->spacing_x + spcFontTbl[window_ptr->mess[0] - 1].w;
                    if (window_ptr->mess_x + tab_w > window_ptr->mess_rect_w && HuWinCR(window_ptr) != 0) {
                        window_ptr->mess--;
                        HuWinKeyWaitEntry(window);
                        window_ptr->attr |= 2;
                        return;
                    }
                    HuWinSpcFontEntry(window_ptr, window_ptr->mess[0] - 1, window_ptr->mess_rect_x + window_ptr->mess_x,
                        window_ptr->mess_rect_y + window_ptr->mess_y);
                    window_ptr->mess_x += tab_w;
                    mess_end = 1;
                    break;

                case 28:
                    window_ptr->mess++;
                    HuAudFXPlay(winVoiceTbl[window_ptr->mess[0] - 1]);
                    break;

                case 255:
                    window_ptr->mess++;
                    HuWinKeyWaitEntry(window);
                    window_ptr->attr |= 4;
                    window_ptr->attr &= ~0x200;
                    return;

                case 15:
                    window_ptr->attr |= 0x2000;
                    if (window_ptr->choice_disable[window_ptr->num_choices] != 0) {
                        window_ptr->attr |= 0x20;
                        window_ptr->choice_data[window_ptr->num_choices].stat |= 1;
                    }
                    else {
                        window_ptr->choice_data[window_ptr->num_choices].stat &= ~1;
                    }

                    window_ptr->choice_data[window_ptr->num_choices].x = window_ptr->mess_x + window_ptr->mess_rect_y;
                    window_ptr->choice_data[window_ptr->num_choices].y = window_ptr->mess_y + window_ptr->mess_rect_x;
                    window_ptr->num_choices++;
                    break;

                case 12:
                    window_ptr->attr |= 0x200;
                    tab_w = window_ptr->tab_w * ((window_ptr->mess_x + window_ptr->tab_w) / window_ptr->tab_w) - window_ptr->mess_x;
                    if (window_ptr->mess_x + tab_w > window_ptr->mess_rect_w) {
                        if (HuWinCR(window_ptr) != 0) {
                            window_ptr->mess++;
                            HuWinKeyWaitEntry(window);
                            window_ptr->attr |= 2;
                            return;
                        }
                    }
                    else {
                        window_ptr->mess_x += tab_w;
                    }
                    break;
            }
            window_ptr->mess++;
            char_w = window_ptr->spacing_x + fontWidthP[window_ptr->mess[0]];
            if (mess_end != 0) {
                break;
            }
        }
        if (mess_end == 0) {
            if (window_ptr->mess_x + char_w > window_ptr->mess_rect_w && HuWinCR(window_ptr) != 0) {
                HuWinKeyWaitEntry(window);
                window_ptr->attr |= 2;
                return;
            }
            c = window_ptr->mess[0];
            window_ptr->attr |= 0x200;
#if VERSION_NTSC
            if (window_ptr->mess[1] == 128) {
                if (c >= 150 && c <= 164) {
                    c = c + 106;
                }
                else if (c >= 170 && c <= 174) {
                    c = c + 101;
                }
                else if (c >= 214 && c <= 228) {
                    c = c + 67;
                }
                else if (c >= 234 && c <= 238) {
                    c = c + 62;
                }
                window_ptr->mess++;
            }
            else if (window_ptr->mess[1] == 129) {
                if (c >= 170 && c <= 174) {
                    c = c + 106;
                }
                else if (c >= 234 && c <= 238) {
                    c = c + 67;
                }
                window_ptr->mess++;
            }
#endif
            color = (window_ptr->attr & 0x20) ? 8 : window_ptr->mess_color;
            if (window_ptr->attr & 1) {
                shadow_color = 0;
                if (window_ptr->mess_color == 0 || window_ptr->mess_color == 1) {
                    shadow_color = 8;
                }
                charEntry(window, window_ptr->mess_rect_x + window_ptr->mess_x + 2, window_ptr->mess_rect_y + window_ptr->mess_y, c, shadow_color);
                charEntry(window, window_ptr->mess_rect_x + window_ptr->mess_x - 2, window_ptr->mess_rect_y + window_ptr->mess_y, c, shadow_color);
                charEntry(window, window_ptr->mess_rect_x + window_ptr->mess_x, window_ptr->mess_rect_y + window_ptr->mess_y + 2, c, shadow_color);
                charEntry(window, window_ptr->mess_rect_x + window_ptr->mess_x, window_ptr->mess_rect_y + window_ptr->mess_y - 2, c, shadow_color);
                charEntry(window, window_ptr->mess_rect_x + window_ptr->mess_x, window_ptr->mess_rect_y + window_ptr->mess_y, c, color);
            }
            else {
                charEntry(window, window_ptr->mess_rect_x + window_ptr->mess_x + 2, window_ptr->mess_rect_y + window_ptr->mess_y + 2, c,
                    window_ptr->mess_shadow_color);
                charEntry(window, window_ptr->mess_rect_x + window_ptr->mess_x, window_ptr->mess_rect_y + window_ptr->mess_y, c, color);
            }
            window_ptr->mess_x += char_w;
            window_ptr->mess++;
        }
    }
}

/**
 * @brief Advances the message cursor to a new line, scrolling existing glyphs when necessary.
 * @param window WindowData whose mess_x/mess_y cursor and existing glyph rows are advanced or scrolled.
 * @return 1 when scrolling is required to make room for the new line; otherwise 0.
 */
static s32 HuWinCR(WindowData *window)
{
    if (window->mess_y + 48 > window->mess_rect_h) {
        window->mess_y = 0;
        window->mess_x = 0;
        return 1;
    }
    else {
        window->mess_y += window->spacing_y + 24;
        window->mess_x = 0;
        return 0;
    }
}

/**
 * @brief Clears rendered characters and resets message cursor, choice, and stack state.
 * @param window WindowData whose glyph count, cursor, choice state, message stack, and special-font sprites are reset.
 */
static void _HuWinHomeClear(WindowData *window)
{
    s16 i;

    window->num_chars = 0;
    window->mess_y = window->mess_x = 0;
    window->num_choices = 0;
    HuWinSpcFontClear(window);
    window->attr &= ~0x2020;
    for (i = 0; i < 16; i++) {
        window->choice_data[i].stat = 0;
    }
}

/**
 * @brief Resets one window to an empty home position.
 * @param window Active winData slot from 0 through 31 passed to _HuWinHomeClear.
 */
void HuWinHomeClear(s16 window)
{
    WindowData *window_ptr = &winData[window];

    _HuWinHomeClear(window_ptr);
}

/**
 * @brief Marks a window as waiting for advance-button input.
 * @param window Active winData slot from 0 through 31 whose advance sprite and wait status are enabled.
 */
void HuWinKeyWaitEntry(s16 window)
{
    WindowData *window_ptr = &winData[window];

    if (window_ptr->attr & 0x400) {
        window_ptr->stat = 0;
    }
    else {
        window_ptr->stat = 2;
        window_ptr->advance_sprite = HuWinSpcFontEntry(window_ptr, 19, window_ptr->mess_rect_w - 20, window_ptr->mess_rect_h - 24);
    }
}

/**
 * @brief Waits until an active player supplies an accepted advance button.
 * @param window Active winData slot from 0 through 31 whose accepted input is polled until advance.
 */
static void HuWinKeyWait(s16 window)
{
    WindowData *window_ptr = &winData[window];

    if (window_ptr->push_key & HuWinActivePadGet(window_ptr)) {
        window_ptr->key_down = HuWinActivePadGet(window_ptr);
        window_ptr->stat = 1;
        HuAudFXPlay(28);
        HuWinSprKill(window, window_ptr->advance_sprite);
        if (window_ptr->attr & 2) {
            _HuWinHomeClear(window_ptr);
        }
        window_ptr->attr &= ~2;
    }
}

/**
 * @brief Adds a special-font icon sprite at a message-relative position.
 * @param window WindowData whose sprite_id table receives the icon sprite.
 * @param entry One-based spcFontTbl icon selector encoded by message control code 0x0E.
 * @param x Icon X coordinate relative to the message origin in pixels.
 * @param y Icon Y coordinate relative to the message origin in pixels.
 * @return Window sprite-entry index used by the new icon, or -1 when full.
 */
static s16 HuWinSpcFontEntry(WindowData *window, s16 entry, s16 x, s16 y)
{
    HUSPRGRP *group = &HuSprGrpData[window->group];
    s16 sprite;
    s16 i;
    ANIMDATA *anim;

    for (i = 10; i < 30; i++) {
        if (window->sprite_id[i] == -1) {
            anim = *spcFontTbl[entry].anim;
            window->sprite_id[i] = sprite = HuSprCreate(anim, window->prio - 1, spcFontTbl[entry].bank);
            HuSprGrpMemberSet(window->group, i, sprite);
            HuSprPosSet(window->group, i, x + spcFontTbl[entry].center_x - window->w / 2, y + spcFontTbl[entry].center_y - window->h / 2);
            break;
        }
    }
    return i;
}

/**
 * @brief Moves one special-font icon relative to the current message cursor.
 * @param window WindowData owning the special-font sprite entry.
 * @param index sprite_id slot 2 through 29 returned by HuWinSpcFontEntry.
 * @param x Replacement icon X coordinate relative to the message origin in pixels.
 * @param y Replacement icon Y coordinate relative to the message origin in pixels.
 */
static void HuWinSpcFontPosSet(WindowData *window, s16 index, s16 x, s16 y)
{
    HUSPRGRP *group = &HuSprGrpData[window->group];

    HuSprPosSet(window->group, index, x - window->w / 2, y - window->h / 2);
}

/**
 * @brief Removes all special-font icon sprites from a window.
 * @param window WindowData whose special-font sprite_id entries 2 through 29 are killed and cleared.
 */
static void HuWinSpcFontClear(WindowData *window)
{
    s16 i;

    for (i = 10; i < 30; i++) {
        if (window->sprite_id[i] != -1) {
            HuSprGrpMemberKill(window->group, i);
            window->sprite_id[i] = -1;
        }
    }
}

/**
 * @brief Runs cursor movement and confirmation for the choices parsed from a message.
 * @param window WindowData whose choice_data positions, disabled flags, cursor sprite, and active input are processed.
 */
static void HuWinChoice(WindowData *window)
{
    WinChoice *choice;
    float choice_dist;
    float min_choice_dist_x;
    s16 choice_dist_y;
    s16 key;
    s16 choice_curr_x;
    s16 choice_curr_y;
    s16 choice_curr;
    s16 choice_max;
    s16 choice_dist_x;
    s16 dir;
    s16 choice_y;
    s16 choice_next;
    s16 i;

    choice_curr = window->choice;
    choice_next = choice_curr;
    dir = -1;
    key = HuWinActivePadGet(window);
    if (key & 1) {
        dir = 0;
    }
    if (key & 2) {
        dir = 2;
    }
    if (key & 8) {
        dir = 1;
    }
    if (key & 4) {
        dir = 3;
    }
    choice_curr_x = window->choice_data[choice_curr].x;
    choice_curr_y = window->choice_data[choice_curr].y;
    min_choice_dist_x = 100000.0f;
    choice_dist = min_choice_dist_x;
    choice_max = window->num_choices;
    switch (dir) {
        case 0:
            for (i = 0, choice = window->choice_data; i < choice_max; i++, choice++) {
                if (i != choice_curr && !(choice->stat & 1) && choice->y == choice_curr_y && choice->x < choice_curr_x) {
                    break;
                }
            }
            if (i != choice_max) {
                for (i = 0, choice = window->choice_data; i < choice_max; i++, choice++) {
                    if (i != choice_curr && !(choice->stat & 1) && choice->y == choice_curr_y && choice->x < choice_curr_x
                        && choice_dist > choice_curr_x - choice->x) {
                        choice_dist = choice_curr_x - choice->x;
                        choice_next = i;
                    }
                }
            }
            break;
        case 1:
            for (i = 0, choice = window->choice_data; i < choice_max; i++, choice++) {
                if (i != choice_curr && !(choice->stat & 1) && choice->y < choice_curr_y) {
                    break;
                }
            }
            if (i != choice_max) {
                for (i = 0, choice = window->choice_data; i < choice_max; i++, choice++) {
                    if (i != choice_curr && !(choice->stat & 1) && choice->y < choice_curr_y) {
                        choice_dist_y = choice_curr_y - choice->y;
                        if (choice_dist_y <= choice_dist) {
                            if (choice_dist_y < choice_dist) {
                                min_choice_dist_x = 100000.0f;
                            }
                            choice_dist_x = choice_curr_x - choice->x;
                            choice_dist_x = choice_dist_x * choice_dist_x;
                            if (choice_dist_x < min_choice_dist_x) {
                                choice_dist = choice_dist_y;
                                min_choice_dist_x = choice_dist_x;
                                choice_next = i;
                            }
                        }
                    }
                }
            }
            else {
                choice_y = -1000;
                for (i = 0, choice = window->choice_data; i < choice_max; i++, choice++) {
                    if (i != choice_curr && !(choice->stat & 1) && choice->y > choice_y) {
                        choice_y = choice->y;
                    }
                }
                if (choice_y != choice_curr_y) {
                    min_choice_dist_x = 100000.0f;
                    for (i = 0, choice = window->choice_data; i < choice_max; i++, choice++) {
                        if (i != choice_curr && !(choice->stat & 1) && choice->y == choice_y) {
                            choice_dist_x = choice_curr_x - choice->x;
                            choice_dist_x = choice_dist_x * choice_dist_x;
                            if (choice_dist_x < min_choice_dist_x) {
                                min_choice_dist_x = choice_dist_x;
                                choice_next = i;
                            }
                        }
                    }
                }
            }
            break;
        case 2:
            for (i = 0, choice = window->choice_data; i < choice_max; i++, choice++) {
                if (i != choice_curr && !(choice->stat & 1) && choice->y == choice_curr_y && choice->x > choice_curr_x) {
                    break;
                }
            }
            if (i != choice_max) {
                for (i = 0, choice = window->choice_data; i < choice_max; i++, choice++) {
                    if (i != choice_curr && !(choice->stat & 1) && choice->y == choice_curr_y && choice->x > choice_curr_x
                        && choice_dist > choice->x - choice_curr_x) {
                        choice_dist = choice->x - choice_curr_x;
                        choice_next = i;
                    }
                }
            }
            break;
        case 3:
            for (i = 0, choice = window->choice_data; i < choice_max; i++, choice++) {
                if (i != choice_curr && !(choice->stat & 1) && choice->y > choice_curr_y) {
                    break;
                }
            }
            if (i != choice_max) {
                for (i = 0, choice = window->choice_data; i < choice_max; i++, choice++) {
                    if (i != choice_curr && !(choice->stat & 1) && choice->y > choice_curr_y) {
                        choice_dist_y = choice->y - choice_curr_y;
                        if (choice_dist_y <= choice_dist) {
                            if (choice_dist_y < choice_dist) {
                                min_choice_dist_x = 100000.0f;
                            }
                            choice_dist_x = choice_curr_x - choice->x;
                            choice_dist_x = choice_dist_x * choice_dist_x;
                            if (choice_dist_x < min_choice_dist_x) {
                                choice_dist = choice_dist_y;
                                min_choice_dist_x = choice_dist_x;
                                choice_next = i;
                            }
                        }
                    }
                }
            }
            else {
                choice_y = 1000;
                for (i = 0, choice = window->choice_data; i < choice_max; i++, choice++) {
                    if (i != choice_curr && !(choice->stat & 1) && choice->y < choice_y) {
                        choice_y = choice->y;
                    }
                }
                if (choice_y != choice_curr_y) {
                    min_choice_dist_x = 100000.0f;
                    for (i = 0, choice = window->choice_data; i < choice_max; i++, choice++) {
                        if (i != choice_curr && !(choice->stat & 1) && choice->y == choice_y) {
                            choice_dist_x = choice_curr_x - choice->x;
                            choice_dist_x = choice_dist_x * choice_dist_x;
                            if (choice_dist_x < min_choice_dist_x) {
                                min_choice_dist_x = choice_dist_x;
                                choice_next = i;
                            }
                        }
                    }
                }
            }
            break;
    }
    if (window->choice != choice_next) {
        window->choice = choice_next;
        HuAudFXPlay(0);
    }
    else if (key & (VERSION_JP ? PAD_BUTTON_A : (window->key_auto | PAD_BUTTON_A))) {
        HuAudFXPlay(2);
        window->key_down = key;
        window->stat = 0;
    }
    else if ((key & PAD_BUTTON_B) && !(window->attr & 0x10)) {
        HuAudFXPlay(3);
#if !VERSION_JP
        window->key_down = key;
#else
        (void)key; // hack to match Japanese version
#endif
        window->choice = -1;
        window->stat = 0;
    }
    HuWinSpcFontPosSet(window, window->cursor_sprite, window->choice_data[choice_next].x + spcFontTbl[20].center_x,
        window->choice_data[choice_next].y + spcFontTbl[20].center_y);
}

/**
 * @brief Builds the controller-port mask currently allowed to advance a window.
 * @param window WindowData whose active_pad and player_disable fields determine the returned controller-port mask.
 * @return Bitmask of controller ports allowed to advance the window.
 */
u32 HuWinActivePadGet(WindowData *window)
{
    s32 win_key;
    u32 i;

    win_key = 0;
    for (i = 0; i < 4; i++) {
        if (window->active_pad & (1 << i)) {
            win_key |= winKey[i];
        }
    }
    return win_key;
}

/**
 * @brief Returns accepted pressed or repeated buttons from the window active-player mask.
 * @param window WindowData whose active_pad/player_disable mask selects HuPadBtnDown and HuPadDStkRep entries.
 * @return Accepted pressed or repeated controller-button mask.
 */
u32 HuWinActiveKeyGetX(WindowData *window)
{
    u32 button;
    u32 i;
    u32 j;
    u8 active_pad;

    button = 0;
    active_pad = (window->active_pad & ~window->player_disable);
    for (i = 0; i < 4; i++) {
        if (active_pad & (1 << i)) {
            for (j = 0; j < 4; j++) {
                if (i == GWPlayerCfg[j].pad_idx) {
                    break;
                }
            }
            if (j != 4 && GWPlayerCfg[j].iscom == 0) {
                button |= HuPadBtn[GWPlayerCfg[j].pad_idx];
            }
        }
    }
    return button;
}

/**
 * @brief Sets a window group screen position.
 * @param window Active winData slot from 0 through 31 whose group screen position and pos_x/pos_y is read or updated.
 * @param x New HuSprGrpData[group].pos.x screen coordinate in pixels.
 * @param y New HuSprGrpData[group].pos.y screen coordinate in pixels.
 */
void HuWinPosSet(s16 window, float x, float y)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->pos_x = x;
    window_ptr->pos_y = y;
    HuSprGrpPosSet(window_ptr->group, x, y);
}

/**
 * @brief Sets a window group scale.
 * @param window Active winData slot from 0 through 31 whose group scale is read or updated.
 * @param x New unitless horizontal group scale.
 * @param y New unitless vertical group scale.
 */
void HuWinScaleSet(s16 window, float x, float y)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->scale_x = x;
    window_ptr->scale_y = y;
    HuSprGrpScaleSet(window_ptr->group, x, y);
}

/**
 * @brief Sets a window group Z rotation.
 * @param window Active winData slot from 0 through 31 whose group zRot is read or updated.
 * @param z_rot New group Z-axis angle in degrees.
 */
void HuWinZRotSet(s16 window, float z_rot)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->z_rot = z_rot;
    HuSprGrpZRotSet(window_ptr->group, z_rot);
}

/**
 * @brief Sets a window group rotation and scale center.
 * @param window Active winData slot from 0 through 31 whose group center is read or updated.
 * @param x New horizontal group transform pivot in pixels.
 * @param y New vertical group transform pivot in pixels.
 */
void HuWinCenterPosSet(s16 window, float x, float y)
{
    WindowData *window_ptr = &winData[window];

    HuSprGrpCenterSet(window_ptr->group, window_ptr->w / 2.0f - x, window_ptr->h / 2.0f - y);
}

/**
 * @brief Assigns a window group to a sprite draw pass.
 * @param window Active winData slot from 0 through 31 whose group member drawNo values is read or updated.
 * @param draw_no HuSprData.drawNo pass assigned to every member of the window group.
 */
void HuWinDrawNoSet(s16 window, s16 draw_no)
{
    WindowData *window_ptr = &winData[window];

    HuSprGrpDrawNoSet(window_ptr->group, draw_no);
}

/**
 * @brief Sets a window scissor rectangle and applies it to the group.
 * @param window Active winData slot from 0 through 31 whose scissor rectangle is read or updated.
 * @param x Left edge in screen pixels.
 * @param y Top edge in screen pixels.
 * @param w Scissor width in pixels.
 * @param h Scissor height in pixels.
 */
void HuWinScissorSet(s16 window, s16 x, s16 y, s16 w, s16 h)
{
    WindowData *window_ptr = &winData[window];

    HuSprGrpScissorSet(window_ptr->group, x, y, w, h);
}

/**
 * @brief Sets the base priority used by a window and its sprites.
 * @param window Active winData slot from 0 through 31 whose base priority and owned sprite priorities is read or updated.
 * @param prio Base priority stored in winData[window].prio; owned sprite priorities are reassigned relative to it.
 */
void HuWinPriSet(s16 window, s16 prio)
{
    WindowData *window_ptr = &winData[window];
    s16 i;

    HuSprPriSet(window_ptr->group, 0, prio);
    HuSprPriSet(window_ptr->group, 1, prio);
    for (i = 2; i < 30; i++) {
        if (window_ptr->sprite_id[i] != -1) {
            HuSprPriSet(window_ptr->group, i, prio - 1);
        }
    }
    window_ptr->prio = prio;
}

/**
 * @brief Sets window behavior attribute bits.
 * @param window Active winData slot from 0 through 31 whose attr bitfield is read or updated.
 * @param attr Window behavior bits ORed into winData[window].attr.
 */
void HuWinAttrSet(s16 window, u32 attr)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->attr |= attr;
}

/**
 * @brief Clears window behavior attribute bits.
 * @param window Active winData slot from 0 through 31 whose attr bitfield is read or updated.
 * @param attr Window behavior bits cleared from winData[window].attr.
 */
void HuWinAttrReset(s16 window, u32 attr)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->attr &= ~attr;
}

/**
 * @brief Reads a window status byte.
 * @param window Active winData slot from 0 through 31 whose stat field is read or updated.
 * @return Current window status byte.
 */
s16 HuWinStatGet(s16 window)
{
    WindowData *window_ptr = &winData[window];

    return window_ptr->stat;
}

/**
 * @brief Selects the palette index for subsequently emitted message glyphs.
 * @param window Active winData slot from 0 through 31 whose mess_color field is read or updated.
 * @param color Message-palette index used for subsequently emitted glyphs.
 */
void HuWinMesColSet(s16 window, u8 color)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->mess_color = color;
    window_ptr->attr |= 0x80;
}

/**
 * @brief Replaces one RGB entry in a window message palette.
 * @param window Active winData slot from 0 through 31 whose mess_pal table is read or updated.
 * @param index Palette row 0 through 9 in winData[window].mess_pal.
 * @param r Red channel from 0 through 255.
 * @param g Green channel from 0 through 255.
 * @param b Blue channel from 0 through 255.
 */
void HuWinMesPalSet(s16 window, u8 index, u8 r, u8 g, u8 b)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->mess_pal[index][0] = r;
    window_ptr->mess_pal[index][1] = g;
    window_ptr->mess_pal[index][2] = b;
}

/**
 * @brief Sets the opacity of the nine background sprites.
 * @param window Active winData slot from 0 through 31 whose background member opacity is read or updated.
 * @param tp_level Background opacity level, clamped by the sprite manager to 0.0 through 1.0.
 */
void HuWinBGTPLvlSet(s16 window, float tp_level)
{
    WindowData *window_ptr = &winData[window];

    HuSprTPLvlSet(window_ptr->group, 0, tp_level);
}

/**
 * @brief Sets the RGB modulation color of the nine background sprites.
 * @param window Active winData slot from 0 through 31 whose background member RGB modulation is read or updated.
 * @param bg_color RGB modulation color applied to the window background.
 */
void HuWinBGColSet(s16 window, GXColor *bg_color)
{
    WindowData *window_ptr = &winData[window];

    HuSprColorSet(window_ptr->group, 0, bg_color->r, bg_color->g, bg_color->b);
}

/**
 * @brief Sets the number of update ticks between printed glyphs.
 * @param window Active winData slot from 0 through 31 whose message speed is read or updated.
 * @param speed Animation frames advanced per sprite update.
 */
void HuWinMesSpeedSet(s16 window, s16 speed)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->speed = speed;
}

/**
 * @brief Loads one regional message-data archive into the message heap.
 * @param mess_data_no Index into mesDataTbl selecting the regional archive loaded into messDataPtr.
 */
void HuWinMesRead(s32 mess_data_no)
{
    void *dvd_mess;
    char *mess_path;

    if (messDataPtr) {
        HuMemDirectFree(messDataPtr);
    }
    messDataNo = mess_data_no;
#if VERSION_NTSC
    if (LanguageNo == 0) {
        mess_path = mesDataTbl[messDataNo];
    }
    else {
        mess_path = mesDataTbl[messDataNo + 2];
    }
#else
    mess_path = mesDataTbl[messDataNo + (LanguageNo * 2)];
#endif
    dvd_mess = HuDvdDataRead(mess_path);
    messDataPtr = HuMemDirectMalloc(HEAP_HEAP, DirDataSize);
    memcpy(messDataPtr, dvd_mess, DirDataSize);
    HuMemDirectFree(dvd_mess);
}

/**
 * @brief Clears a window and starts displaying a message ID or direct message pointer.
 * @param window Active winData slot from 0 through 31 whose mess pointer and print status is read or updated.
 * @param mess Message-table ID resolved through messDataPtr, or a direct byte pointer tagged with MAKE_MESSID_PTR.
 */
void HuWinMesSet(s16 window, u32 mess)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->stat = 1;
    if ((mess & 0x80000000) == 0) {
        if (messDataPtr == 0) {
            OSReport("Error: No Message\n");
            return;
        }
        window_ptr->mess = MessData_MesPtrGet(messDataPtr, mess);
        if (window_ptr->mess == 0) {
            OSReport("Error: No Message data\n");
#if !VERSION_JP
            HuWinMesSet(window, 0);
            return;
#endif
        }
    }
    else {
        window_ptr->mess = (u8 *)mess;
    }
    if (!(window_ptr->attr & 0x80)) {
        window_ptr->mess_color = 7;
#if !VERSION_JP
        window_ptr->mess_time = 0;
#endif
    }
}

/**
 * @brief Assigns a message ID or direct pointer to one insertion slot.
 * @param window Active winData slot from 0 through 31 whose insert_mess table is read or updated.
 * @param mess Message-table ID or tagged direct byte pointer stored in one insert_mess slot.
 * @param index Insertion slot 0 through 7 in winData[window].insert_mess.
 */
void HuWinInsertMesSet(s16 window, u32 mess, s16 index)
{
    WindowData *window_ptr = &winData[window];

    if (!(mess & 0x80000000)) {
        if (messDataPtr == 0) {
            OSReport("Error: No Message\n");
            return;
        }
        window_ptr->insert_mess[index] = MessData_MesPtrGet(messDataPtr, mess);
        if (window_ptr->insert_mess[index] == 0) {
            OSReport("Error: No Message data\n");
        }
    }
    else {
        window_ptr->insert_mess[index] = (u8 *)mess;
    }
}

/**
 * @brief Displays choices and blocks until one is selected.
 * @param window Active winData slot from 0 through 31 whose choice state and cursor is read or updated.
 * @param start_choice Preferred choice_data entry; disabled or out-of-range entries are skipped with wraparound.
 * @return Confirmed zero-based choice index.
 */
s16 HuWinChoiceGet(s16 window, s16 start_choice)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->attr |= 0x40;
    while (window_ptr->stat != 0) {
        HuPrcVSleep();
    }
    for (; start_choice < window_ptr->num_choices; start_choice++) {
        if (!(window_ptr->choice_data[start_choice].stat & 1)) {
            break;
        }
    }
    if (start_choice == window_ptr->num_choices) {
        for (start_choice = 0; start_choice < window_ptr->num_choices; start_choice++) {
            if (!(window_ptr->choice_data[start_choice].stat & 1)) {
                break;
            }
        }
        if (start_choice == window_ptr->num_choices) {
            return -1;
        }
    }
    window_ptr->cursor_sprite = HuWinSpcFontEntry(window_ptr, 0x14, window_ptr->choice_data[start_choice].x, window_ptr->choice_data[start_choice].y);
    window_ptr->stat = 3;
    window_ptr->choice = start_choice;
    while (window_ptr->stat != 0) {
        HuPrcVSleep();
    }
    return window_ptr->choice;
}

/**
 * @brief Reads the number of choices parsed for a window.
 * @param window Active winData slot from 0 through 31 whose num_choices field is read or updated.
 * @return Number of choices parsed from the current message.
 */
s16 HuWinChoiceNumGet(s16 window)
{
    WindowData *window_ptr = &winData[window];

    return window_ptr->num_choices;
}

/**
 * @brief Marks one parsed choice unavailable for cursor selection.
 * @param window Active winData slot from 0 through 31 whose choice_disable table is read or updated.
 * @param choice choice_disable entry later copied to the matching parsed choice_data status.
 */
void HuWinChoiceDisable(s16 window, s16 choice)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->choice_disable[choice] = 1;
}

/**
 * @brief Reads the currently highlighted choice.
 * @param window Active winData slot from 0 through 31 whose choice field when stat is 3 is read or updated.
 * @return Currently highlighted zero-based choice index.
 */
s16 HuWinChoiceNowGet(s16 window)
{
    WindowData *window_ptr = &winData[window];

    if (window_ptr->stat == 3) {
        return window_ptr->choice;
    }
    else {
        return -1;
    }
}

/**
 * @brief Blocks until a window finishes printing or is killed.
 * @param window Active winData slot from 0 through 31 whose stat field is read or updated.
 */
void HuWinMesWait(s16 window)
{
    WindowData *window_ptr = &winData[window];

    while (window_ptr->stat != 0) {
        HuPrcVSleep();
    }
}

/**
 * @brief Creates an animation sprite owned by a window.
 * @param window Active winData slot from 0 through 31 whose sprite_id table is read or updated.
 * @param anim Animation retained by the new sprite stored in the first free sprite_id entry 2 through 29.
 * @param bank Initial anim->bank entry selected for the new sprite.
 * @param x Sprite X coordinate relative to the window group in pixels.
 * @param y Sprite Y coordinate relative to the window group in pixels.
 * @return Window sprite-entry index, or -1 when the window sprite table is full.
 */
s16 HuWinAnimSet(s16 window, ANIMDATA *anim, s16 bank, float x, float y)
{
    WindowData *window_ptr = &winData[window];
    s16 sprite;

    sprite = HuSprCreate(anim, window_ptr->prio - 1, bank);
    return HuWinSprSet(window, sprite, x, y);
}

/**
 * @brief Adds an existing sprite slot to a window.
 * @param window Active winData slot from 0 through 31 whose sprite_id table is read or updated.
 * @param sprite Existing HuSprData slot inserted into the first free window sprite_id entry 2 through 29.
 * @param x Sprite X coordinate relative to the window group in pixels.
 * @param y Sprite Y coordinate relative to the window group in pixels.
 * @return Window sprite-entry index, or -1 when the window sprite table is full.
 */
s16 HuWinSprSet(s16 window, s16 sprite, float x, float y)
{
    WindowData *window_ptr = &winData[window];
    HUSPRGRP *group = &HuSprGrpData[window_ptr->group];
    s16 i;

    for (i = 2; i <= 9; i++) {
        if (window_ptr->sprite_id[i] == -1) {
            window_ptr->sprite_id[i] = sprite;
            HuSprGrpMemberSet(window_ptr->group, i, sprite);
            HuSprPosSet(window_ptr->group, i, x - group->center.x, y - group->center.y);
            break;
        }
    }
    return i;
}

/**
 * @brief Moves one window-owned sprite.
 * @param window Active winData slot from 0 through 31 whose owned sprite position is read or updated.
 * @param index Window sprite_id entry 0 through 29 selecting the owned HuSprData slot.
 * @param x Replacement member X coordinate relative to the window group in pixels.
 * @param y Replacement member Y coordinate relative to the window group in pixels.
 */
void HuWinSprPosSet(s16 window, s16 index, float x, float y)
{
    WindowData *window_ptr = &winData[window];
    HUSPRGRP *group = &HuSprGrpData[window_ptr->group];

    HuSprPosSet(window_ptr->group, index, x - group->center.x, y - group->center.y);
}

/**
 * @brief Sets one window-owned sprite priority relative to the window base priority.
 * @param window Active winData slot from 0 through 31 whose owned sprite priority is read or updated.
 * @param index Window sprite_id entry 0 through 29 selecting the owned HuSprData slot.
 * @param prio Priority written to the selected owned sprite after adding winData[window].prio.
 */
void HuWinSprPriSet(s16 window, s16 index, s16 prio)
{
    WindowData *window_ptr = &winData[window];
    HUSPRGRP *group = &HuSprGrpData[window_ptr->group];

    HuSprPriSet(window_ptr->group, index, prio);
}

/**
 * @brief Reads the sprite slot stored in one window sprite entry.
 * @param window Active winData slot from 0 through 31 whose sprite_id table is read or updated.
 * @param index Window sprite_id entry 0 through 29 selecting the owned HuSprData slot.
 * @return Underlying sprite slot index stored at the requested entry.
 */
s16 HuWinSprIDGet(s16 window, s16 index)
{
    WindowData *window_ptr = &winData[window];

    return window_ptr->sprite_id[index];
}

/**
 * @brief Destroys and clears one window-owned sprite entry.
 * @param window Active winData slot from 0 through 31 whose sprite_id table is read or updated.
 * @param index Window sprite_id entry 0 through 29 selecting the owned HuSprData slot.
 */
void HuWinSprKill(s16 window, s16 index)
{
    WindowData *window_ptr = &winData[window];

    HuSprGrpMemberKill(window_ptr->group, index);
    window_ptr->sprite_id[index] = -1;
}

/**
 * @brief Hides a window group and suspends message processing.
 * @param window Active winData slot from 0 through 31 whose group visibility and attr bit 8 is read or updated.
 */
void HuWinDispOff(s16 window)
{
    WindowData *window_ptr = &winData[window];
    s16 i;

    for (i = 0; i < 30; i++) {
        if (window_ptr->sprite_id[i] != -1) {
            HuSprAttrSet(window_ptr->group, i, HUSPR_ATTR_DISPOFF);
        }
    }
    window_ptr->attr |= 8;
}

/**
 * @brief Shows a window group and resumes message processing.
 * @param window Active winData slot from 0 through 31 whose group visibility and attr bit 8 is read or updated.
 */
void HuWinDispOn(s16 window)
{
    WindowData *window_ptr = &winData[window];
    s16 i;

    for (i = 0; i < 30; i++) {
        if (window_ptr->sprite_id[i] != -1) {
            HuSprAttrReset(window_ptr->group, i, HUSPR_ATTR_DISPOFF);
        }
    }
    window_ptr->attr = window_ptr->attr & ~8;
}

/**
 * @brief Queues simulated button presses for four players after a frame delay.
 * @param player_1 Simulated button mask for player 1.
 * @param player_2 Simulated button mask for player 2.
 * @param player_3 Simulated button mask for player 3.
 * @param player_4 Simulated button mask for player 4.
 * @param delay Retraces before simulated input becomes available.
 */
void HuWinComKeyWait(s32 player_1, s32 player_2, s32 player_3, s32 player_4, s16 delay)
{
    s32 wait_input_p4;
    s32 wait_input_p3;
    s32 wait_input_p2;
    s32 wait_input_p1;

    if (player_4 == -1) {
        wait_input_p4 = -1;
    }
    else {
        wait_input_p4 = 0;
    }
    if (player_3 == -1) {
        wait_input_p3 = -1;
    }
    else {
        wait_input_p3 = 0;
    }
    if (player_2 == -1) {
        wait_input_p2 = -1;
    }
    else {
        wait_input_p2 = 0;
    }
    if (player_1 == -1) {
        wait_input_p1 = -1;
    }
    else {
        wait_input_p1 = 0;
    }
    _HuWinComKeySet(wait_input_p1, wait_input_p2, wait_input_p3, wait_input_p4, delay);
    _HuWinComKeySet(player_1, player_2, player_3, player_4, 1);
}

/**
 * @brief Queues immediate simulated button presses for four players.
 * @param player_1 Simulated button mask for player 1.
 * @param player_2 Simulated button mask for player 2.
 * @param player_3 Simulated button mask for player 3.
 * @param player_4 Simulated button mask for player 4.
 */
void HuWinComKeySet(s32 player_1, s32 player_2, s32 player_3, s32 player_4)
{
    _HuWinComKeySet(player_1, player_2, player_3, player_4, 1);
}

/**
 * @brief Stores per-player simulated button masks and a countdown delay.
 * @param player_1 Simulated button mask for player 1.
 * @param player_2 Simulated button mask for player 2.
 * @param player_3 Simulated button mask for player 3.
 * @param player_4 Simulated button mask for player 4.
 * @param delay Retraces before simulated input becomes available.
 */
void _HuWinComKeySet(s32 player_1, s32 player_2, s32 player_3, s32 player_4, s16 delay)
{
    winComKeyBuf[comKeyIdx].player[0] = player_1;
    winComKeyBuf[comKeyIdx].player[1] = player_2;
    winComKeyBuf[comKeyIdx].player[2] = player_3;
    winComKeyBuf[comKeyIdx].player[3] = player_4;
    winComKeyBuf[comKeyIdx].time = delay;
    comKeyIdx++;
    comKeyIdx &= 0xFF;
}

/**
 * @brief Combines due simulated input for the players active in a window.
 * @param window Active winData slot from 0 through 31 whose active-player input is read or updated.
 * @param data Caller-owned four-u32 output array receiving one button mask per player.
 */
void HuWinComKeyGet(s16 window, u32 *data)
{
    WindowData *window_ptr = &winData[window];
    s16 i;

    if (comKeyIdx == comKeyIdxNow) {
        for (i = 0; i < 4; i++) {
            if (!(window_ptr->player_disable & (1 << i))) {
                data[i] = HuPadDStkRep[i] | HuPadBtnDown[i];
            }
            else {
                data[i] = 0;
            }
        }
    }
    else {
        for (i = 0; i < 4; i++) {
            data[i] = winComKeyBuf[comKeyIdxNow].player[i];
            if (data[i] == 0xFFFFFFFF) {
                if (!(window_ptr->player_disable & (1 << i))) {
                    data[i] = HuPadDStkRep[i] | HuPadBtnDown[i];
                }
                else {
                    data[i] = 0;
                }
            }
        }
        winComKeyBuf[comKeyIdxNow].time--;
        if (winComKeyBuf[comKeyIdxNow].time <= 0) {
            comKeyIdxNow++;
            comKeyIdxNow &= 0xFF;
        }
    }
}

/**
 * @brief Clears simulated button masks and delay state.
 */
void HuWinComKeyReset(void)
{
    comKeyIdx = comKeyIdxNow = 0;
}

/**
 * @brief Measures the maximum width and height needed by a variadic list of messages.
 * @param mess_num Number of u32 message IDs or tagged pointers consumed from the variadic list.
 * @param size Caller-owned two-float output: rounded window width in size[0] and height in size[1], in pixels.
 */
void HuWinMesMaxSizeGet(s16 mess_num, float *size, ...)
{
    s16 i;
    u32 mess;
    va_list list;

    winInsertF = 0;
    va_start(list, size);
    winMaxWidth = winMaxHeight = 0;
    for (i = 0; i < mess_num; i++) {
        mess = va_arg(list, u32);
        GetMesMaxSizeSub(mess);
    }
    for (i = 0; i < 8; i++) {
        mesWInsert[i] = 0;
    }
    winTabSize = 24;
    cancelCRF = 0;
    size[0] = (winMaxWidth + 0x1F) & 0xFFF0;
    size[1] = winMaxHeight + 0x10;
    va_end(list);
}

/**
 * @brief Registers an insertion message used by subsequent size measurement.
 * @param mess Message-table ID or tagged direct pointer measured for insertion width.
 * @param index mesWInsert slot 0 through 7 receiving the measured width.
 */
void HuWinInsertMesSizeGet(u32 mess, s16 index)
{
    winInsertF = 1;
    winMaxWidth = winMaxHeight = 0;
    GetMesMaxSizeSub(mess);
    mesWInsert[index] = winMaxWidth;
}

/**
 * @brief Controls whether line breaks terminate message-size measurement.
 * @param cancel_cr Nonzero to continue size measurement across line-break control codes.
 */
void HuWinMesSizeCancelCRSet(s32 cancel_cr)
{
    cancelCRF = cancel_cr;
}

/**
 * @brief Measures the largest message in an inclusive ID range.
 * @param size Caller-owned two-float output receiving the maximum rounded width and height in pixels.
 * @param start First message-table ID measured inclusively.
 * @param end Last message-table ID measured inclusively; values below start produce 100 by 100.
 */
void HuWinMesMaxSizeBetGet(float *size, u32 start, u32 end)
{
    u32 i;

    if (end < start) {
        size[0] = size[1] = 100.0f;
    }
    else {
        winInsertF = 0;
        winMaxWidth = winMaxHeight = 0;
        for (i = start; i <= end; i++) {
            GetMesMaxSizeSub(i);
        }
        for (i = 0; i < 8; i++) {
            mesWInsert[i] = 0;
        }
        winTabSize = 24;
        cancelCRF = 0;
        size[0] = (winMaxWidth + 0x1F) & 0xFFF0;
        size[1] = winMaxHeight + 0x10;
    }
}

/**
 * @brief Parses one message and accumulates global maximum line width and height.
 * @param mess Message-table ID resolved through messDataPtr, or tagged direct byte pointer parsed in place.
 */
static void GetMesMaxSizeSub(u32 mess)
{
    s16 line_h;
    s16 char_w;
    s16 line_w;
    u8 *mess_start;
    s16 from_messdata;
    s16 char_h;
    s32 cr_flag;
    u8 *mess_data;

    mess_start = NULL;
    cr_flag = 0;
    if (mess > 0x80000000) {
        from_messdata = 0;
        mess_data = (u8 *)mess;
    }
    else {
        from_messdata = 1;
        mess_data = mess_start = MessData_MesPtrGet(messDataPtr, mess);
    }
    line_h = 26;
    line_w = 0;
    while (*mess_data != 0) {
        char_h = 0;
        char_w = fontWidthP[*mess_data] + 1;
        switch (*mess_data) {
            case 16:
            case 32:
                break;
            case 30:
                mess_data++;
                char_w = 0;
                break;
            case 10:
                if (cr_flag != 0) {
                    if (cancelCRF == 0) {
                        if (winMaxWidth < line_w) {
                            winMaxWidth = line_w;
                        }
                        line_w = char_w = 0;
                        if (mess_data[1] != 0) {
                            char_h = 26;
                        }
                    }
                    else {
                        char_w = fontWidthP[16] + 1;
                    }
                }
                break;
            case 255:
                line_w += char_w;
                /* fallthrough */
            case 11:
                cr_flag = 0;
                if (winInsertF == 0) {
                    if (winMaxWidth < line_w) {
                        winMaxWidth = line_w;
                    }
                    if (winMaxHeight < line_h) {
                        winMaxHeight = line_h;
                    }
                    line_h = 26;
                    char_h = 0;
                    line_w = char_w = 0;
                }
                break;
            case 28:
                mess_data++;
                /* fallthrough */
            case 15:
            case 29:
                char_w = 0;
                break;
            case 12:
                char_w = winTabSize * ((line_w + winTabSize) / winTabSize) - line_w;
                break;
            case 14:
                mess_data++;
                char_w = spcFontTbl[*mess_data - 1].w + 1;
                break;
            case 31:
                mess_data++;
                char_w = mesWInsert[*mess_data - 1];
                break;
        }
#if VERSION_NTSC
        if ((*mess_data != 255 && *mess_data >= 32) || *mess_data == 16) {
            cr_flag = 1;
        }
#else
        if ((*mess_data != 255 && *mess_data >= 32) || *mess_data == 16 || mess_data[-1] == 31) {
            cr_flag = 1;
        }
#endif
        line_w += char_w;
        line_h += char_h;
        mess_data++;
    }
    if (winMaxWidth < line_w) {
        winMaxWidth = line_w;
    }
    if (winMaxHeight < line_h) {
        winMaxHeight = line_h;
    }
}

/**
 * @brief Measures the first printable line of message data with window-specific formatting.
 * @param window WindowData supplying formatting attributes while the first printable line is measured.
 * @param mess_data NUL-terminated encoded message bytes parsed without taking ownership.
 * @return Width in pixels of the measured printable line.
 */
static s32 GetMesMaxSizeSub2(WindowData *window, u8 *mess_data)
{
    s32 current_line_width;
    u8 mess_finish;
    s16 mess_h;
    s16 char_w;
    s16 mess_w;
    s32 mess_end;
    s16 char_h;
    u8 *mess;

    current_line_width = 0;
    mess_end = 0;
    mess = mess_data;
    mess_h = 0x1A;
    mess_w = 0;
    mess_finish = 0;
    while (*mess != 0 && mess_finish == 0) {
        char_h = 0;
        char_w = fontWidthP[*mess] + 1;
        switch (*mess) {
            case 16:
            case 32:
                break;

            case 10:
                if (mess_end != 0 && !(window->attr & 0x100)) {
                    char_w = 0;
                    mess_finish = 1;
                }
                break;

            case 255:
                mess_w += char_w;
                /* fallthrough */
            case 11:
                mess_end = 0;
                mess_h = 0x1A;
                char_h = 0;
                mess_w = char_w = 0;
                break;

            case 28:
            case 30:
                mess++;
                /* fallthrough */
            case 15:
            case 29:
                char_w = 0;
                break;

            case 12:
                char_w = winTabSize * ((mess_w + winTabSize) / winTabSize) - mess_w;
                break;

            case 14:
                mess++;
                char_w = spcFontTbl[*mess - 1].w + 1;
                break;

            case 31:
                mess++;
                break;
        }
        if ((*mess != 0xFF && *mess >= 0x20) || *mess == 0x10) {
            mess_end = 1;
        }
        mess_w += char_w;
        mess_h += char_h;
        mess++;
    }
    return mess_w;
}

/**
 * @brief Counts advance-button wait markers in a message.
 * @param mess Message-table ID or tagged direct byte pointer scanned for 0xFF wait markers.
 * @return Number of advance-button wait control codes in the message.
 */
s16 HuWinKeyWaitNumGet(u32 mess)
{
    s16 wait_num;
    u8 *mess_data;

    if (mess > 0x80000000) {
        mess_data = (u8 *)mess;
    }
    else {
        mess_data = MessData_MesPtrGet(messDataPtr, mess);
    }
    for (wait_num = 0; *mess_data != 0; mess_data++) {
        if (*mess_data == 0xFF) {
            wait_num++;
        }
    }
    return wait_num;
}

/**
 * @brief Sets the button mask accepted for message advancement.
 * @param window Active winData slot from 0 through 31 whose push_key field is read or updated.
 * @param push_key Button bitmask stored in winData[window].push_key and accepted by HuWinKeyWait.
 */
void HuWinPushKeySet(s16 window, s16 push_key)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->push_key = push_key;
}

/**
 * @brief Prevents one player index from advancing a window.
 * @param window Active winData slot from 0 through 31 whose player_disable bitmask is read or updated.
 * @param player Controller bitmask ORed into player_disable; despite the name, this is not converted from a player index.
 */
void HuWinDisablePlayerSet(s16 window, u8 player)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->player_disable |= player;
}

/**
 * @brief Allows one player index to advance a window again.
 * @param window Active winData slot from 0 through 31 whose player_disable bitmask is read or updated.
 * @param player Controller bitmask cleared from player_disable; despite the name, this is not converted from a player index.
 */
void HuWinDisablePlayerReset(s16 window, u8 player)
{
    WindowData *window_ptr = &winData[window];

    window_ptr->player_disable &= ~(int)player;
}

static s32 winPortraitTbl[]
    = { WIN_TOAD_TALK_ANM, WIN_BOBOMB_TALK_ANM, WIN_SHYGUY_TALK_ANM, WIN_BOO_TALK_ANM, WIN_GOOMBA_TALK_ANM, WIN_BOWSER_TALK_ANM, WIN_KKID_TALK_ANM,
          WIN_KOOPA_TALK_ANM, WIN_CONDOR_TALK_ANM, WIN_BOO_BLUE_TALK_ANM, WIN_DOLPHIN_TALK_ANM, WIN_BOO_RED_TALK_ANM, WIN_THWOMP_TALK_ANM,
          WIN_W01_HOST_TALK_ANM, WIN_W02_HOST_TALK_ANM, WIN_W03_HOST_TALK_ANM, WIN_W04_HOST_TALK_ANM, WIN_W05_HOST_TALK_ANM, WIN_W06_HOST_TALK_ANM };

/**
 * @brief Creates a framed dialogue window with an optional portrait.
 * @param x Left screen coordinate in pixels, or -10000.0 to center horizontally.
 * @param y Top screen coordinate in pixels, or -10000.0 to center vertically.
 * @param w Requested dialogue width in pixels, rounded by HuWinCreate.
 * @param h Requested dialogue height in pixels; portrait windows raise values below 84 to 84.
 * @param portrait Index into winPortraitTbl, or a negative value to omit portrait sprites.
 * @return Allocated dialogue-window index, or -1 when allocation fails.
 */
s16 HuWinExCreate(float x, float y, s16 w, s16 h, s16 portrait)
{
    return HuWinExCreateStyled(x, y, w, h, portrait, 0);
}

/**
 * @brief Creates and positions the portrait and portrait-frame sprites for a dialogue window.
 * @param window Active winData slot from 0 through 31 whose portrait sprite entry is read or updated.
 * @param portrait Index into winPortraitTbl selecting the ARAM animation loaded for the portrait.
 * @param x Portrait X coordinate relative to the window group in pixels.
 * @param y Portrait Y coordinate relative to the window group in pixels.
 */
static void HuWinExCreatePortrait(s16 window, s16 portrait, float x, float y)
{
    s16 sprite;
    ANIMDATA *anim;
    WindowData *window_ptr;
    void *data;

    data = HuAR_ARAMtoMRAMFileRead(winPortraitTbl[portrait], HU_MEMNUM_OVL, 2);
    anim = HuSprAnimRead(data);
    window_ptr = &winData[window];
    sprite = HuSprCreate(anim, window_ptr->prio - 1, 0);
    HuWinSprSet(window, sprite, x, y);
}

/**
 * @brief Creates a dialogue window with explicit frame style and optional portrait.
 * @param x Left screen coordinate in pixels, or -10000.0 to center horizontally.
 * @param y Top screen coordinate in pixels, or -10000.0 to center vertically.
 * @param w Requested dialogue width in pixels, rounded by HuWinCreate.
 * @param h Requested dialogue height in pixels; portrait windows raise values below 84 to 84.
 * @param portrait Index into winPortraitTbl, or a negative value to omit portrait sprites.
 * @param frame Frame style 0 through 3 forwarded to HuWinCreate; out-of-range values become style 0.
 * @return Allocated dialogue-window index, or -1 when allocation fails.
 */
s16 HuWinExCreateStyled(float x, float y, s16 w, s16 h, s16 portrait, s16 frame)
{
    WindowData *window_ptr;
    s16 window;

    if (portrait >= 0) {
        h = (h < 84) ? 84 : h;
    }
    window = HuWinCreate(x, y, w, h, frame);
    window_ptr = &winData[window];
    if (frame == 1) {
        window_ptr->mess_shadow_color = 9;
    }
    if (portrait >= 0) {
        HuWinExCreatePortrait(window, portrait, 48.0f, 48.0f);
    }
    HuWinDispOff(window);
    return window;
}

/**
 * @brief Animates a dialogue window from zero scale to full size.
 * @param window Active winData slot from 0 through 31 whose dialogue group scale is read or updated.
 */
void HuWinExAnimIn(s16 window)
{
    WindowData *window_ptr = &winData[window];
    s16 i;

    _HuWinHomeClear(window_ptr);
    if (window_ptr->sprite_id[2] == -1) {
        HuSprScaleSet(window_ptr->group, 0, 1.0f, 0.0f);
        HuWinDispOn(window);
        for (i = 0; i <= 10; i++) {
            HuSprScaleSet(window_ptr->group, 0, 1.0f, 1.0 - 1.1 * cosd(9.0f * i));
            HuPrcVSleep();
        }
        HuSprScaleSet(window_ptr->group, 0, 1.0f, 1.0f);
    }
    else {
        HuSprScaleSet(window_ptr->group, 0, 1.0f, 0.0f);
        HuSprScaleSet(window_ptr->group, 2, 1.0f, 0.0f);
        HuWinDispOn(window);
        for (i = 0; i <= 20; i++) {
            if (i <= 10) {
                HuSprScaleSet(window_ptr->group, 0, 1.0f, 1.1 * (1.0 - cosd(9.0f * i)));
            }
            else {
                HuSprScaleSet(window_ptr->group, 0, 1.0f, 1.0f);
            }
            if (i > 10) {
                s16 time = i - 10;
                HuSprScaleSet(window_ptr->group, 2, 1.0f, 1.0 - 1.1 * cosd(9.0f * time));
            }
            HuPrcVSleep();
        }
        HuSprScaleSet(window_ptr->group, 2, 1.0f, 1.0f);
    }
}

/**
 * @brief Animates a dialogue window from full size to zero scale.
 * @param window Active winData slot from 0 through 31 whose dialogue group scale is read or updated.
 */
void HuWinExAnimOut(s16 window)
{
    WindowData *window_ptr = &winData[window];
    s16 i;

    _HuWinHomeClear(window_ptr);
    if (window_ptr->sprite_id[2] == -1) {
        for (i = 0; i <= 10; i++) {
            HuSprScaleSet(window_ptr->group, 0, 1.0f, cosd(9.0f * i));
            HuPrcVSleep();
        }
        HuWinDispOff(window);
    }
    else {
        for (i = 0; i <= 15; i++) {
            if (i <= 10) {
                HuSprScaleSet(window_ptr->group, 2, 1.0f, cosd(9.0f * i));
            }
            if (i > 5) {
                s16 time = i - 5;
                HuSprScaleSet(window_ptr->group, 0, 1.0f, cosd(9.0f * time));
            }
            HuPrcVSleep();
        }
        HuWinDispOff(window);
    }
}

/**
 * @brief Destroys a dialogue window after removing its portrait sprites.
 * @param window Active winData slot from 0 through 31 whose owned group and buffers is read or updated.
 */
void HuWinExCleanup(s16 window)
{
    HuWinKill(window);
}

/**
 * @brief Animates a dialogue window in with a brief overshoot and optional portrait motion.
 * @param window Active winData slot from 0 through 31 whose portrait sprite entry is read or updated.
 * @param portrait Index into winPortraitTbl replacing sprite_id entry 2 during the pop animation.
 */
void HuWinExAnimPopIn(s16 window, s16 portrait)
{
    WindowData *window_ptr = &winData[window];
    s16 i;

    for (i = 0; i <= 10; i++) {
        HuSprScaleSet(window_ptr->group, 2, 1.0f, cosd(9.0f * i));
        HuPrcVSleep();
    }
    HuWinSprKill(window, 2);
    HuWinExCreatePortrait(window, portrait, 48.0f, 48.0f);
    for (i = 0; i <= 10; i++) {
        HuSprScaleSet(window_ptr->group, 2, 1.0f, sind(9.0f * i));
        HuPrcVSleep();
    }
    HuSprScaleSet(window_ptr->group, 2, 1.0f, 1.0f);
}

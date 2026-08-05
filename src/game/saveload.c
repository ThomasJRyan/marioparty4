#include "game/saveload.h"
#include "data_num/win.h"
#include "game/card.h"
#include "game/data.h"
#include "game/flag.h"
#include "game/gamework_data.h"
#include "game/pad.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"

#include <stddef.h>
#include <string.h>

#if VERSION_ENG
#define SAVE_WRITE_BEGIN _SetFlag(FLAG_ID_MAKE(3, 0));
#define SAVE_WRITE_END _ClearFlag(FLAG_ID_MAKE(3, 0));
#else
#define SAVE_WRITE_BEGIN
#define SAVE_WRITE_END
#endif

#define SAVE_GET_PLAYER(player_idx) &saveBuf.buf[((player_idx) * sizeof(PlayerState)) + offsetof(SaveBufData, player)]
#define SAVE_GET_PLAYER_STORY(player_idx) &saveBuf.buf[((player_idx) * sizeof(PlayerState)) + offsetof(SaveBufData, playerStory)]

static s16 SLCreateSaveWin(void);
static void SLKillSaveWin(void);

extern u8 UnMountCnt;

CARDFileInfo curFileInfo;
SaveBufAll saveBuf;

u64 SLSerialNo;
s32 saveExecF;
u8 curBoxNo;
s16 curSlotNo;

#if VERSION_JP
static u8 commentTbl[2][32] = { "マリオパーティ４", "？？月？？日　データ？です。" };
static u8 sjisNumTbl[20] = "０１２３４５６７８９";
#else
static u8 commentTbl[2][32] = { "Mario Party 4", "File 0  00/00/0000" };
#endif

char *SaveFileNameTbl[] = { "MARIPA4BOX0", "MARIPA4BOX1", "MARIPA4BOX2" };

s32 SaveEnableF = 1;

static char *SlotNameTbl[] = { "A", "B" };

#if VERSION_JP
static char *slotIconMesTbl[] = { "\x0E\x16", "\x0E\x17" };
static u8 sjisSpace[] = "　";
#endif

static s32 saveMessWin = -1;

/**
 * @brief Mounts the selected card and opens an existing save file for the current box.
 * @param fileName NUL-terminated filename passed to HuCardOpen in the globally selected curSlotNo.
 * @return CARD result code from mount, open, or validation.
 */
s32 SLFileOpen(char *fileName)
{
    s32 result;

    if (SaveEnableF == 0) {
        return 0;
    }
    while (1) {
        result = SLCardMount(curSlotNo);
        if (result < 0) {
            return result;
        }
        result = HuCardOpen(curSlotNo, fileName, &curFileInfo);
        if (result == CARD_RESULT_NOFILE) {
            return CARD_RESULT_NOFILE;
        }
        if (result == CARD_RESULT_WRONGDEVICE) {
            SLMessOut(7);
            return CARD_RESULT_FATAL_ERROR;
        }
        if (result == CARD_RESULT_FATAL_ERROR) {
            SLMessOut(1);
            return CARD_RESULT_FATAL_ERROR;
        }
        if (result == CARD_RESULT_NOCARD) {
            SLMessOut(0);
            return CARD_RESULT_NOCARD;
        }
        if (result == CARD_RESULT_BROKEN) {
            result = HuCardSectorSizeGet(curSlotNo);
            if (result > 0 && result != 8192) {
                SLMessOut(8);
                return CARD_RESULT_WRONGDEVICE;
            }
            UnMountCnt = 0;
            result = SLMessOut(5);
            if (result == 0) {
                result = SLFormat(curSlotNo);
                if (result != 0) {
                    return result;
                }
            }
            else {
                return CARD_RESULT_NOFILE;
            }
        }
        else {
            return CARD_RESULT_READY;
        }
    }
}

/**
 * @brief Creates a save file, writes its initial buffer, and configures comment, banner, and icon metadata.
 * @param fileName NUL-terminated filename passed to HuCardCreate in the globally selected curSlotNo.
 * @param size File allocation size and number of initial bytes read from addr and written at offset zero.
 * @param addr Caller-owned source buffer containing at least size bytes; read-only for the duration of HuCardWrite.
 * @return CARD result code from mount, create, initial write, or metadata update.
 */
s32 SLFileCreate(char *fileName, u32 size, void *addr)
{
    float winSize[2];
    u32 byteNotUsed;
    u32 filesNotUsed;
    s32 window;
    s32 result;

    if (SaveEnableF == 0) {
        return 0;
    }
    result = SLCardMount(curSlotNo);
    if (result < 0) {
        return result;
    }
    result = HuCardSectorSizeGet(curSlotNo);
    if (result < 0 && result != 8192) {
        SLMessOut(8);
        return CARD_RESULT_FATAL_ERROR;
    }
    result = HuCardFreeSpaceGet(curSlotNo, &byteNotUsed, &filesNotUsed);
    if (filesNotUsed == 0 && size > byteNotUsed) {
        SLMessOut(4);
        return CARD_RESULT_INSSPACE;
    }
    if (filesNotUsed == 0) {
        SLMessOut(2);
        return CARD_RESULT_INSSPACE;
    }
    if (size > byteNotUsed) {
        SLMessOut(3);
        return CARD_RESULT_INSSPACE;
    }
    HuWinInit(1);
    HuWinMesMaxSizeGet(1, winSize, MAKE_MESSID(16, 11));
    window = HuWinExCreateStyled(-10000.0f, 200.0f, winSize[0], winSize[1], -1, 2);
    HuWinExAnimIn(window);
    HuWinMesSet(window, MAKE_MESSID(16, 11));
    HuWinMesWait(window);
    SAVE_WRITE_BEGIN
    result = HuCardCreate(curSlotNo, fileName, size, &curFileInfo);
    SAVE_WRITE_END
    if (result < 0) {
        HuWinExAnimOut(window);
        HuWinExCleanup(window);
    }
    if (result == CARD_RESULT_NOCARD) {
        SLMessOut(0);
        return CARD_RESULT_NOCARD;
    }
    if (result < 0) {
        SLMessOut(1);
        return CARD_RESULT_FATAL_ERROR;
    }
    SAVE_WRITE_BEGIN
    result = HuCardWrite(&curFileInfo, addr, size, 0);
    SAVE_WRITE_END
    if (result < 0) {
        HuWinExAnimOut(window);
        HuWinExCleanup(window);
    }
    if (result == CARD_RESULT_NOCARD) {
        SLMessOut(0);
        return CARD_RESULT_NOCARD;
    }
    if (result < 0) {
        SLMessOut(1);
        return CARD_RESULT_FATAL_ERROR;
    }
    SAVE_WRITE_BEGIN
    result = SLStatSet(1);
    SAVE_WRITE_END
    HuWinExAnimOut(window);
    HuWinExCleanup(window);
    if (result < 0) {
        return result;
    }
    return 0;
}

/**
 * @brief Writes bytes from the caller buffer at offset zero of the open save file.
 * @param length Byte count read from addr and written at offset zero of curFileInfo.
 * @param addr Caller-owned source buffer containing at least length readable bytes.
 * @return CARD result code; CARD_RESULT_READY indicates the write completed.
 */
s32 SLFileWrite(s32 length, void *addr)
{
    float winSize[2];
    s32 window;
    s32 result;

    if (SaveEnableF == 0) {
        return 0;
    }
    HuWinInit(1);
    HuWinMesMaxSizeGet(1, winSize, MAKE_MESSID(16, 11));
    window = HuWinExCreateStyled(-10000.0f, 200.0f, winSize[0], winSize[1], -1, 2);
    HuWinExAnimIn(window);
    HuWinMesSet(window, MAKE_MESSID(16, 11));
    HuWinMesWait(window);
    HuPrcSleep(60);
    SAVE_WRITE_BEGIN
    result = HuCardWrite(&curFileInfo, addr, length, 0);
    if (result == 0) {
        result = SLStatSet(1);
    }
    SAVE_WRITE_END
    HuWinExAnimOut(window);
    HuWinExCleanup(window);
    return result;
}

/**
 * @brief Reads bytes at offset zero of the open save file into the caller buffer.
 * @param length Byte count read at offset zero of curFileInfo into addr.
 * @param addr Caller-owned destination buffer with capacity for at least length writable bytes.
 * @return CARD result code; CARD_RESULT_READY indicates the read completed.
 */
s32 SLFileRead(s32 length, void *addr)
{
    s32 result;

    if (SaveEnableF == 0) {
        return 0;
    }
    result = HuCardRead(&curFileInfo, addr, length, 0);
    if (result == CARD_RESULT_NOCARD) {
        SLMessOut(0);
    }
    else if (result < 0) {
        SLMessOut(1);
    }
    return result;
}

/**
 * @brief Closes the current memory-card file and unmounts the selected card.
 * @return CARD result code from close or unmount.
 */
s32 SLFileClose(void)
{
    s32 result;

    if (SaveEnableF == 0) {
        return 0;
    }
    result = HuCardClose(&curFileInfo);
    return result;
}

/**
 * @brief Selects memory-card slot A or B for subsequent operations.
 * @param slotno CARD slot selector stored in curSlotNo for subsequent mount, open, read, write, and format calls.
 */
void SLCurSlotNoSet(s16 slotno)
{
    curSlotNo = slotno;
}

/**
 * @brief Selects the save-box filename used by subsequent operations.
 * @param boxno Entry 0 through 2 stored in curBoxNo to select SaveFileNameTbl[curBoxNo].
 */
void SLCurBoxNoSet(s16 boxno)
{
    curBoxNo = boxno;
}

/**
 * @brief Sets the internal save-request flag.
 * @param flag Pending-save state stored in SaveEnableF; zero also clears party_continue and story_continue.
 */
void SLSaveFlagSet(s32 flag)
{
    if (flag == 0) {
        GWGameStat.party_continue = 0;
        GWGameStat.story_continue = 0;
    }
    SaveEnableF = flag;
}

/**
 * @brief Reads the internal save-request flag.
 * @return Current internal save-request flag.
 */
s32 SLSaveFlagGet(void)
{
    return SaveEnableF;
}

/**
 * @brief Builds save metadata and optionally restores gameplay data from defaults before serialization.
 * @param erase Nonzero to restore gameplay portions of the save buffer from defaults.
 * @param time OS timestamp encoded into the localized file comment.
 */
void SLSaveDataMake(s32 erase, OSTime *time)
{
    ANIMDATA *anim_data;
    u8 *buf;
    s32 i;

    buf = saveBuf.buf;
    if (erase != 0) {
        for (i = 0; i < SAVE_BUF_SIZE; i++) {
            buf[i] = 0xFF;
        }
    }
    for (i = 0; i < 0x20; i++) {
        buf[i] = (&commentTbl[0][0])[i];
    }
    for (i = 0; i < 0x20; i++) {
        (&buf[0x20])[i] = (&commentTbl[0][0])[i + 32];
    }
    anim_data = HuSprAnimReadFile(WIN_CARD_BANNER_ANM);
    memcpy(buf + offsetof(SaveBufData, banner), anim_data->bmp->data, CARD_BANNER_WIDTH * CARD_BANNER_HEIGHT);
    memcpy(buf + offsetof(SaveBufData, bannerTlut), anim_data->bmp->palData, 512);
    anim_data = HuSprAnimReadFile(curBoxNo + WIN_CARD_BOX1_ICON_ANM);
    memcpy(buf + offsetof(SaveBufData, icon), anim_data->bmp->data, CARD_ICON_WIDTH * CARD_ICON_HEIGHT * 4);
    memcpy(buf + offsetof(SaveBufData, iconTlut), anim_data->bmp->palData, 512);
    SLSaveDataInfoSet(time);
}

/**
 * @brief Writes localized comments, timestamp text, banner, icon, palette, and status flags into the save buffer.
 * @param time OS timestamp encoded into the localized file comment.
 */
void SLSaveDataInfoSet(OSTime *time)
{
    s16 year;
    s16 digit;
    OSCalendarTime calendar_time;

    OSTicksToCalendarTime(*time, &calendar_time);
#if VERSION_ENG
    saveBuf.data.comment[37] = curBoxNo + '1';
    digit = (calendar_time.mon + 1) / 10;
    saveBuf.data.comment[40] = digit + '0';
    digit = (calendar_time.mon + 1) % 10;
    saveBuf.data.comment[41] = digit + '0';
    digit = calendar_time.mday / 10;
    saveBuf.data.comment[43] = digit + '0';
    digit = calendar_time.mday % 10;
    saveBuf.data.comment[44] = digit + '0';
    year = calendar_time.year;
    digit = year / 1000;
    saveBuf.data.comment[46] = digit + '0';
    year -= digit * 1000;
    digit = year / 100;
    saveBuf.data.comment[47] = digit + '0';
    year -= digit * 100;
    digit = year / 10;
    saveBuf.data.comment[48] = digit + '0';
    year -= digit * 10;
    saveBuf.data.comment[49] = year + '0';
#elif VERSION_PAL
    saveBuf.data.comment[37] = curBoxNo + '1';
    digit = calendar_time.mday / 10;
    saveBuf.data.comment[40] = digit + '0';
    digit = calendar_time.mday % 10;
    saveBuf.data.comment[41] = digit + '0';
    digit = (calendar_time.mon + 1) / 10;
    saveBuf.data.comment[43] = digit + '0';
    digit = (calendar_time.mon + 1) % 10;
    saveBuf.data.comment[44] = digit + '0';
    year = calendar_time.year;
    digit = year / 1000;
    saveBuf.data.comment[46] = digit + '0';
    year -= digit * 1000;
    digit = year / 100;
    saveBuf.data.comment[47] = digit + '0';
    year -= digit * 100;
    digit = year / 10;
    saveBuf.data.comment[48] = digit + '0';
    year -= digit * 10;
    saveBuf.data.comment[49] = year + '0';
#else
    saveBuf.data.comment[52] = sjisNumTbl[(curBoxNo + 1) * 2];
    saveBuf.data.comment[53] = sjisNumTbl[(curBoxNo + 1) * 2 + 1];
    digit = (calendar_time.mon + 1) / 10;
    if (digit == 0) {
        saveBuf.data.comment[32] = sjisSpace[0];
        saveBuf.data.comment[33] = sjisSpace[1];
    }
    else {
        saveBuf.data.comment[32] = sjisNumTbl[digit * 2];
        saveBuf.data.comment[33] = sjisNumTbl[digit * 2 + 1];
    }
    digit = (calendar_time.mon + 1) % 10;
    saveBuf.data.comment[34] = sjisNumTbl[digit * 2];
    saveBuf.data.comment[35] = sjisNumTbl[digit * 2 + 1];
    digit = calendar_time.mday / 10;
    if (digit == 0) {
        saveBuf.data.comment[38] = sjisSpace[0];
        saveBuf.data.comment[39] = sjisSpace[1];
    }
    else {
        saveBuf.data.comment[38] = sjisNumTbl[digit * 2];
        saveBuf.data.comment[39] = sjisNumTbl[digit * 2 + 1];
    }
    digit = calendar_time.mday % 10;
    saveBuf.data.comment[40] = sjisNumTbl[digit * 2];
    saveBuf.data.comment[41] = sjisNumTbl[digit * 2 + 1];
#endif
}

/**
 * @brief Copies persistent game statistics and current language into the save buffer.
 */
void SLCommonSet(void)
{
    OSTime create_time;

    create_time = OSGetTime();
    GWGameStat.create_time = create_time;
    memcpy(&saveBuf.data.stat, &GWGameStat, sizeof(GameStat));
    SLSaveDataInfoSet(&create_time);
}

/**
 * @brief Copies current party-mode system and player state into the save buffer.
 */
void SLSaveBoard(void)
{
    s16 i;

    memcpy(&saveBuf.data.system, &GWSystem, sizeof(SystemState));
    for (i = 0; i < 4; i++) {
        memcpy(SAVE_GET_PLAYER(i), &GWPlayer[i], sizeof(PlayerState));
    }
}

/**
 * @brief Copies current story-mode system and player state into the save buffer.
 */
void SLSaveBoardStory(void)
{
    s16 i;

    memcpy(&saveBuf.data.systemStory, &GWSystem, sizeof(SystemState));
    for (i = 0; i < 4; i++) {
        memcpy(SAVE_GET_PLAYER_STORY(i), &GWPlayer[i], sizeof(PlayerState));
    }
}

/**
 * @brief Builds, checksums, and writes the complete save buffer to the selected card.
 * @return Save/load status code mapped from the CARD operation.
 */
s32 SLSave(void)
{
    s32 result;

    while (1) {
        SLCheckSumSet();
        result = SLFileOpen(SaveFileNameTbl[curBoxNo]);
        if (result == CARD_RESULT_NOFILE) {
            if (!SLSerialNoCheck()) {
                SLMessOut(9);
            }
            else {
                SLCreateSaveWin();
                result = SLFileCreate(SaveFileNameTbl[curBoxNo], 16384, &saveBuf);
                SLKillSaveWin();
                if (result >= 0) {
                    SLSerialNoGet();
                    goto block_32;
                }
            }
        }
        else {
            if (result == CARD_RESULT_NOCARD) {
                result = SLMessOut(10);
                if (result != 0) {
                    SLMessOut(11);
                    continue;
                }
                SLSaveFlagSet(0);
                break;
            }
            if (result >= 0) {
                if (!SLSerialNoCheck()) {
                    SLMessOut(9);
                }
                else {
                    SLCreateSaveWin();
                    result = SLFileWrite(16384, &saveBuf);
                    SLKillSaveWin();
                    if (result == CARD_RESULT_NOCARD) {
                        SLMessOut(0);
                    }
                    else if (result == CARD_RESULT_WRONGDEVICE) {
                        SLMessOut(7);
                    }
                    else if (result == CARD_RESULT_BROKEN) {
                        result = HuCardSectorSizeGet(curSlotNo);
                        if (result > 0 && result != 8192) {
                            SLMessOut(8);
                            goto block_36;
                        }
                        UnMountCnt = 0;
                        result = SLMessOut(5);
                        if (result == 0) {
                            result = SLFormat(curSlotNo);
                            if (result) {
                                return result;
                            }
                            continue;
                        }
                        else {
                            result = CARD_RESULT_BROKEN;
                        }
                    }
                    else if (result < 0) {
                        SLMessOut(1);
                    }
                block_32:
                    SLFileClose();
                    if (result >= 0) {
                        HuCardUnMount(curSlotNo);
                        return 1;
                    }
                }
            }
        }
    block_36:
        result = SLMessOut(10);
        if (result != 0) {
            SLMessOut(11);
        }
        else {
            SLSaveFlagSet(0);
            break;
        }
    }
    HuCardUnMount(curSlotNo);
    return 0;
}

#if VERSION_NTSC
#define SAVEWIN_POS 150
#else
#define SAVEWIN_POS 120
#endif

#if VERSION_JP
#define SAVEWIN_MESS slotIconMesTbl
#else
#define SAVEWIN_MESS SlotNameTbl
#endif

/**
 * @brief Creates and animates the small save-in-progress window.
 * @return Index of the save-in-progress window.
 */
static s16 SLCreateSaveWin(void)
{
    float size[2];
    s16 window;

    HuWinInit(1);
    HuWinInsertMesSizeGet(MAKE_MESSID_PTR(SAVEWIN_MESS[curSlotNo]), 0);
    HuWinMesMaxSizeGet(1, size, MAKE_MESSID(16, 68));
    window = HuWinExCreateStyled(-10000.0f, SAVEWIN_POS, size[0], size[1], -1, 2);
    saveMessWin = window;
    HuWinExAnimIn(window);
    HuWinInsertMesSet(window, MAKE_MESSID_PTR(SAVEWIN_MESS[curSlotNo]), 0);
    HuWinMesSet(window, MAKE_MESSID(16, 68));
    HuWinMesWait(window);
    return window;
}

/**
 * @brief Animates out and destroys the save-in-progress window when present.
 */
static void SLKillSaveWin(void)
{
    if (saveMessWin != -1) {
        HuWinExAnimOut(saveMessWin);
        HuWinExCleanup(saveMessWin);
        saveMessWin = -1;
    }
}

/**
 * @brief Attempts to read the selected save file into the global save buffer, then unmounts the card.
 * @details The current implementation calculates but does not act on the checksum comparison expression.
 * @return Always 0 after the open/read attempt.
 */
s32 SLLoad(void)
{
    s32 result;
    u16 *save_checksum;
    u16 checksum;

    result = SLFileOpen(SaveFileNameTbl[curBoxNo]);
    if (result >= 0) {
        result = SLFileRead(16384, &saveBuf);
        SLFileClose();
        if (result >= 0) {
            save_checksum = (u16 *)&saveBuf.buf[sizeof(SaveBufData)];
            checksum = SLCheckSumGet();
            *save_checksum == checksum;
        }
    }
    HuCardUnMount(curSlotNo);
    return 0;
}

/**
 * @brief Copies persistent statistics and language from the loaded save buffer.
 */
void SLLoadGameStat(void)
{
    memcpy(&GWGameStat, &saveBuf.data.stat, sizeof(GameStat));
}

/**
 * @brief Restores party-mode system and player state from the loaded save buffer.
 */
void SLLoadBoard(void)
{
    s16 i;

    memcpy(&GWSystem, &saveBuf.data.system, sizeof(SystemState));
    for (i = 0; i < 4; i++) {
        memcpy(&GWPlayer[i], SAVE_GET_PLAYER(i), sizeof(PlayerState));
        GWPlayerCfg[i].character = GWPlayer[i].character;
        GWPlayerCfg[i].pad_idx = GWPlayer[i].port;
        GWPlayerCfg[i].diff = GWPlayer[i].diff;
        GWPlayerCfg[i].iscom = GWPlayer[i].com;
        GWPlayerCfg[i].group = GWPlayer[i].team;
    }
}

/**
 * @brief Restores story-mode system and player state from the loaded save buffer.
 */
void SLLoadBoardStory(void)
{
    s16 i;

    memcpy(&GWSystem, &saveBuf.data.systemStory, 0xDC);
    for (i = 0; i < 4; i++) {
        memcpy(&GWPlayer[i], SAVE_GET_PLAYER_STORY(i), sizeof(PlayerState));
        GWPlayerCfg[i].character = GWPlayer[i].character;
        GWPlayerCfg[i].pad_idx = GWPlayer[i].port;
        GWPlayerCfg[i].diff = GWPlayer[i].diff;
        GWPlayerCfg[i].iscom = GWPlayer[i].com;
        GWPlayerCfg[i].group = 0;
    }
}

/**
 * @brief Reads the serial number of the currently selected memory card.
 * @return CARD result code from CARDGetSerialNo.
 */
s32 SLSerialNoGet(void)
{
    return CARDGetSerialNo(curSlotNo, &SLSerialNo);
}

/**
 * @brief Rejects a successfully read card serial only when it differs from the retained nonzero serial.
 * @return FALSE only for a successful read of a mismatching serial; zero retained serials and CARD read errors return TRUE.
 */
BOOL SLSerialNoCheck(void)
{
    s32 result;
    u64 serialNo;

    if (SLSerialNo == 0) {
        return TRUE;
    }
    result = CARDGetSerialNo(curSlotNo, &serialNo);
    if (result < 0) {
        return TRUE;
    }
    if (serialNo != SLSerialNo) {
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Compares the stored save checksum with a checksum calculated over the data region.
 * @return TRUE when the stored and calculated checksums match; otherwise FALSE.
 */
BOOL SLCheckSumCheck(void)
{
    u16 *save_checksum = (u16 *)&saveBuf.buf[sizeof(SaveBufData)];
    u16 checksum = SLCheckSumGet();

    if (*save_checksum == checksum) {
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief Calculates the 16-bit additive checksum of the save buffer data region.
 * @return 16-bit additive checksum of save bytes after the checksum field.
 */
u16 SLCheckSumGet(void)
{
    u32 i;
    u32 checksum;

    for (i = checksum = 0; i < sizeof(SaveBufData); i++) {
        checksum += saveBuf.buf[i];
    }
    checksum = ~checksum;
    return (u16)checksum & 0xFFFF;
}

/**
 * @brief Stores a freshly calculated checksum in the save buffer.
 */
void SLCheckSumSet(void)
{
    u16 checksum = SLCheckSumGet();

    saveBuf.buf[sizeof(SaveBufData)] = (checksum >> 8) & 0xFF;
    saveBuf.buf[sizeof(SaveBufData) + 1] = checksum;
}

/**
 * @brief Configures comment, icon, banner, and animation metadata for the open card file.
 * @param reportF Nonzero to show a card-status message for mapped errors.
 * @return CARDSetStatus result, with missing-card and other error cases mapped after optional UI reporting.
 */
s32 SLStatSet(s32 reportF)
{
    CARDStat stat;
    s32 fileNo;
    s32 result;

    fileNo = curFileInfo.fileNo;
    result = CARDGetStatus(curSlotNo, fileNo, &stat);
    if (result == CARD_RESULT_NOCARD) {
        if (reportF != 0) {
            SLMessOut(0);
        }
        return -3;
    }
    if (result < 0) {
        if (reportF != 0) {
            SLMessOut(1);
        }
        return CARD_RESULT_FATAL_ERROR;
    }
    CARDSetCommentAddress(&stat, 0);
    CARDSetIconAddress(&stat, 64);
    CARDSetBannerFormat(&stat, CARD_STAT_BANNER_C8);
    CARDSetIconFormat(&stat, 0, CARD_STAT_ICON_C8);
    CARDSetIconFormat(&stat, 1, CARD_STAT_ICON_C8);
    CARDSetIconFormat(&stat, 2, CARD_STAT_ICON_C8);
    CARDSetIconFormat(&stat, 3, CARD_STAT_ICON_C8);
    CARDSetIconSpeed(&stat, 0, CARD_STAT_SPEED_MIDDLE);
    CARDSetIconSpeed(&stat, 1, CARD_STAT_SPEED_MIDDLE);
    CARDSetIconSpeed(&stat, 2, CARD_STAT_SPEED_MIDDLE);
    CARDSetIconSpeed(&stat, 3, CARD_STAT_SPEED_MIDDLE);
    CARDSetIconSpeed(&stat, 4, CARD_STAT_SPEED_END);
    CARDSetIconAnim(&stat, CARD_STAT_ANIM_LOOP);

    result = CARDSetStatus(curSlotNo, fileNo, &stat);
    if (result == CARD_RESULT_NOCARD) {
        if (reportF != 0) {
            SLMessOut(0);
        }
        return CARD_RESULT_NOCARD;
    }
    if (result < 0) {
        if (reportF != 0) {
            SLMessOut(1);
        }
        return CARD_RESULT_FATAL_ERROR;
    }
    return result;
}

/**
 * @brief Mounts a card slot, retrying transient busy results and reporting card errors when requested.
 * @param slotNo Unused; the operation uses the globally selected curSlotNo.
 * @return CARD result code from the final mount or card check.
 */
s32 SLCardMount(s16 slotNo)
{
    s32 result;

    while (1) {
        result = HuCardMount(curSlotNo);
        if (result == CARD_RESULT_WRONGDEVICE) {
            SLMessOut(7);
            return result;
        }
        if (result == CARD_RESULT_FATAL_ERROR) {
            SLMessOut(1);
            return CARD_RESULT_FATAL_ERROR;
        }
        if (result == CARD_RESULT_NOCARD) {
            SLMessOut(0);
            return CARD_RESULT_NOCARD;
        }
        if (result == CARD_RESULT_BROKEN) {
            result = HuCardSectorSizeGet(curSlotNo);
            if (result > 0 && result != 8192) {
                SLMessOut(8);
                return CARD_RESULT_WRONGDEVICE;
            }
            UnMountCnt = 0;
            result = SLMessOut(5);
            if (result == 0) {
                result = SLFormat(curSlotNo);
                if (result != 0) {
                    return result;
                }
            }
            else {
                return CARD_RESULT_FATAL_ERROR;
            }
        }
        else {
            break;
        }
    }
    result = HuCardSectorSizeGet(curSlotNo);
    if (result < 0) {
        SLMessOut(1);
        return result;
    }
    if (result != 8192) {
        SLMessOut(8);
        return CARD_RESULT_WRONGDEVICE;
    }
    return 0;
}

/**
 * @brief Formats a memory card after confirmation and reports the CARD result.
 * @param slotNo Unused; the operation uses the globally selected curSlotNo.
 * @return CARD result code from format or unmount.
 */
s32 SLFormat(s16 slotNo)
{
    float winSize[2];
    s16 result;
    s16 window1;
    s16 window2;
    OSTime time;

    HuWinInit(1);
    HuWinInsertMesSizeGet(MAKE_MESSID_PTR(SAVEWIN_MESS[curSlotNo]), 0);
    HuWinMesMaxSizeGet(1, winSize, MAKE_MESSID(16, 56));
    window1 = HuWinExCreateStyled(-10000.0f, SAVEWIN_POS, winSize[0], winSize[1], -1, 2);
    HuWinExAnimIn(window1);
    HuWinInsertMesSet(window1, MAKE_MESSID_PTR(SAVEWIN_MESS[curSlotNo]), 0);
    HuWinMesSet(window1, MAKE_MESSID(16, 56));
    HuWinMesMaxSizeGet(1, winSize, MAKE_MESSID(16, 11));
    window2 = HuWinExCreateStyled(-10000.0f, 200.0f, winSize[0], winSize[1], -1, 2);
    HuWinExAnimIn(window2);
    HuWinMesSet(window2, MAKE_MESSID(16, 11));
    HuWinMesWait(window2);
    HuPrcSleep(30);
    if (UnMountCnt & (1 << curSlotNo)) {
        HuWinExAnimOut(window1);
        HuWinExCleanup(window1);
        HuWinExAnimOut(window2);
        HuWinExCleanup(window2);
        SLMessOut(12);
        return 0;
    }
    SAVE_WRITE_BEGIN
    result = HuCardFormat(curSlotNo);
    SAVE_WRITE_END
    if (result < 0) {
        HuWinExAnimOut(window1);
        HuWinExCleanup(window1);
        HuWinExAnimOut(window2);
        HuWinExCleanup(window2);
    }
    if (result == CARD_RESULT_FATAL_ERROR) {
        SLMessOut(6);
        SLMessOut(1);
        return CARD_RESULT_FATAL_ERROR;
    }
    if (result == CARD_RESULT_NOCARD) {
        SLMessOut(0);
        return CARD_RESULT_NOCARD;
    }
    if (result == CARD_RESULT_WRONGDEVICE) {
        SLMessOut(7);
        return result;
    }
    HuWinExAnimOut(window1);
    HuWinExCleanup(window1);
    HuWinExAnimOut(window2);
    HuWinExCleanup(window2);
    CARDGetSerialNo(curSlotNo, &SLSerialNo);
    curBoxNo = 0;
    time = OSGetTime();
    SLSaveDataMake(0, &time);
    SLCheckSumSet();
    return result;
}

/**
 * @brief Displays the localized save/load message, optionally obtains a yes/no choice, and returns that choice.
 * @param mess Case 0 through 12 selecting the localized card-error or confirmation message and whether a choice is requested.
 * @return Selected choice index for confirmation messages, or -1 for informational messages.
 */
s16 SLMessOut(s16 mess)
{
    WindowData *window_data;
    float size[2];
    u32 slot_mess;
    s32 save_mess;
    s32 has_choice;
    s16 window;
    s16 choice;

    choice = -1;
    slot_mess = 0;
    has_choice = 0;
    HuWinInit(1);
    SLKillSaveWin();
    switch (mess) {
        case 0:
            save_mess = MAKE_MESSID(16, 2);
            break;

        case 1:
            save_mess = MAKE_MESSID(16, 83);
            HuWinInsertMesSizeGet(MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]), 0);
            slot_mess = MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]);
            break;

        case 2:
            HuWinInsertMesSizeGet(MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]), 0);
            slot_mess = MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]);
#if VERSION_ENG
            save_mess = MAKE_MESSID(16, 74);
#elif VERSION_PAL
            save_mess = MAKE_MESSID(16, 72);
#else
            save_mess = MAKE_MESSID(16, 5);
#endif
            break;

        case 3:
#if VERSION_ENG
            save_mess = MAKE_MESSID(16, 74);
#elif VERSION_PAL
            save_mess = MAKE_MESSID(16, 72);
#else
            save_mess = MAKE_MESSID(16, 6);
#endif
            HuWinInsertMesSizeGet(MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]), 0);
            slot_mess = MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]);
            break;

        case 4:
            save_mess = MAKE_MESSID(16, 74);
            HuWinInsertMesSizeGet(MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]), 0);
            slot_mess = MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]);
            break;

        case 5:
            HuWinInsertMesSizeGet(MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]), 0);
            slot_mess = MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]);
            save_mess = MAKE_MESSID(16, 4);
            has_choice = 1;
            break;

        case 6:
            save_mess = MAKE_MESSID(16, 54);
            break;
        case 7:
            HuWinInsertMesSizeGet(MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]), 0);
            slot_mess = MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]);
            save_mess = MAKE_MESSID(16, 55);
            break;

        case 8:
            HuWinInsertMesSizeGet(MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]), 0);
            slot_mess = MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]);
            save_mess = MAKE_MESSID(16, 57);
            break;

        case 9:
            HuWinInsertMesSizeGet(MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]), 0);
            slot_mess = MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]);
            save_mess = MAKE_MESSID(16, 69);
            break;

        case 10:
            save_mess = MAKE_MESSID(16, 70);
            has_choice = 1;
            break;

        case 11:
            HuWinInsertMesSizeGet(MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]), 0);
            slot_mess = MAKE_MESSID_PTR(SlotNameTbl[curSlotNo]);
#if VERSION_ENG
            save_mess = MAKE_MESSID(16, 72);
#else
            save_mess = MAKE_MESSID(16, 76);
#endif
            break;

        case 12:
            save_mess = MAKE_MESSID(16, 80);
            break;
    }
    if (save_mess == MAKE_MESSID(16, 4)) {
        HuWinMesMaxSizeGet(1, size, MAKE_MESSID(16, 78));
    }
    else {
        HuWinMesMaxSizeGet(1, size, save_mess);
    }
    window = HuWinExCreateStyled(-10000.0f, 200.0f, size[0], size[1], -1, 2);
    window_data = &winData[window];
    window_data->active_pad = 1;
    if (slot_mess != 0) {
        HuWinInsertMesSet(window, slot_mess, 0);
    }
    HuWinAttrSet(window, 0x10);
    HuWinExAnimIn(window);
    HuWinMesSet(window, save_mess);
    HuWinMesWait(window);
    if (has_choice) {
        choice = HuWinChoiceGet(window, 1);
        if (mess == 5 && choice == 0) {
            HuWinMesSet(window, MAKE_MESSID(16, 78));
            HuWinMesWait(window);
            choice = HuWinChoiceGet(window, 1);
        }
    }
    if (mess == 11) {
        while (!(HuPadBtnDown[0] & PAD_BUTTON_A)) {
            HuPrcVSleep();
        }
    }
    HuWinExAnimOut(window);
    HuWinExCleanup(window);
    return choice;
}

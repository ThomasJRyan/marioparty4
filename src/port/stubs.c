#include <dolphin.h>
#include <stdarg.h>
#include <stdio.h>
#include <types.h>

// Credits: Super Monkey Ball

void OSReport(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
}

u32 OSGetConsoleType()
{
    return OS_CONSOLE_RETAIL1;
}

s32 CARDCancel(CARDFileInfo *fileInfo)
{
    puts("CARDCancel is a stub");
    return 0;
}

s32 CARDCheckAsync(s32 chan, CARDCallback callback)
{
    puts("CARDCheckAsync is a stub");
    return 0;
}

s32 CARDClose(CARDFileInfo *fileInfo)
{
    puts("CARDClose is a stub");
    return 0;
}

s32 CARDCreateAsync(s32 chan, const char *fileName, u32 size, CARDFileInfo *fileInfo, CARDCallback callback)
{
    puts("CARDCreateAsync is a stub");
    return 0;
}

s32 CARDDelete(s32 chan, const char *fileName)
{
    puts("CARDDelete is a stub");
    return 0;
}

s32 CARDDeleteAsync(s32 chan, const char *fileName, CARDCallback callback)
{
    puts("CARDDeleteAsync is a stub");
    return 0;
}

s32 CARDFastDeleteAsync(s32 chan, s32 fileNo, CARDCallback callback)
{
    puts("CARDFastDeleteAsync is a stub");
    return 0;
}

s32 CARDFastOpen(s32 chan, s32 fileNo, CARDFileInfo *fileInfo)
{
    puts("CARDFastOpen is a stub");
    return 0;
}

s32 CARDFreeBlocks(s32 chan, s32 *byteNotUsed, s32 *filesNotUsed)
{
    puts("CARDFreeBlocks is a stub");
    return 0;
}

s32 CARDGetResultCode(s32 chan)
{
    puts("CARDGetResultCode is a stub");
    return 0;
}

s32 CARDGetStatus(s32 chan, s32 fileNo, CARDStat *stat)
{
    puts("CARDGetStatus is a stub");
    return 0;
}

void CARDInit()
{
    puts("CARDInit is a stub");
}

s32 CARDMount(s32 chan, void *workArea, CARDCallback detachCallback)
{
    puts("CARDMount is a stub");
    return 0;
}

s32 CARDMountAsync(s32 chan, void *workArea, CARDCallback detachCallback, CARDCallback attachCallback)
{
    puts("CARDMountAsync is a stub");
    return 0;
}

s32 CARDOpen(s32 chan, const char *fileName, CARDFileInfo *fileInfo)
{
    puts("CARDOpen is a stub");
    return 0;
}

s32 CARDProbeEx(s32 chan, s32 *memSize, s32 *sectorSize)
{
    puts("CARDProbeEx is a stub");
    return 0;
}

s32 CARDReadAsync(CARDFileInfo *fileInfo, void *addr, s32 length, s32 offset, CARDCallback callback)
{
    puts("CARDReadAsync is a stub");
    return 0;
}

s32 CARDRename(s32 chan, const char *oldName, const char *newName)
{
    puts("CARDRename is a stub");
    return 0;
}

s32 CARDRenameAsync(s32 chan, const char *oldName, const char *newName, CARDCallback callback)
{
    puts("CARDRenameAsync is a stub");
    return 0;
}

s32 CARDSetStatusAsync(s32 chan, s32 fileNo, CARDStat *stat, CARDCallback callback)
{
    puts("CARDSetStatusAsync is a stub");
    return 0;
}

s32 CARDUnmount(s32 chan)
{
    puts("CARDUnmount is a stub");
    return 0;
}

s32 CARDWriteAsync(CARDFileInfo *fileInfo, const void *addr, s32 length, s32 offset, CARDCallback callback)
{
    puts("CARDWriteAsync is a stub");
    return 0;
}

void C_MTXScale(Mtx m, f32 xS, f32 yS, f32 zS)
{
    *(volatile int *)0 = 0;
    puts("C_MTXScale is a stub");
}

void DCFlushRange(void *addr, u32 nBytes)
{
    puts("DCFlushRange is a stub");
}

void DCFlushRangeNoSync(void *addr, u32 nBytes)
{
    puts("DCFlushRangeNoSync is a stub");
}

void DCInvalidateRange(void *addr, u32 nBytes)
{
    puts("DCInvalidateRange is a stub");
}

void DCStoreRange(void *addr, u32 nBytes)
{
    puts("DCStoreRange is a stub");
}

s32 DVDCancel(DVDCommandBlock *block)
{
    puts("DVDCancel is a stub");
    return 0;
}

void DVDReadAbsAsyncForBS(void *a, struct bb2struct *b, int c, int d, void (*e)())
{
    puts("DVDReadAbsAsyncForBS is a stub");
}

void DVDReadDiskID(void *a, DVDDiskID *b, void (*c)())
{
    puts("DVDReadDiskID is a stub");
}

void DVDReset()
{
    puts("DVDReset is a stub");
}

BOOL EXIDeselect(int chan)
{
    puts("EXIDeselect is a stub");
    return FALSE;
}

BOOL EXIDma(int chan, void *buffer, s32 size, int d, int e)
{
    puts("EXIDma is a stub");
    return FALSE;
}

BOOL EXIImm(int chan, u32 *b, int c, int d, int e)
{
    puts("EXIImm is a stub");
    return FALSE;
}

BOOL EXILock(int chan, int b, int c)
{
    puts("EXILock is a stub");
    return FALSE;
}

BOOL EXISelect(int chan, int b, int c)
{
    puts("EXISelect is a stub");
    return FALSE;
}

BOOL EXISync(int chan)
{
    puts("EXISync is a stub");
    return FALSE;
}

BOOL EXIUnlock(int chan)
{
    puts("EXIUnlock is a stub");
    return FALSE;
}

void LCEnable()
{
    puts("LCEnable is a stub");
}

void OSClearContext(OSContext *context)
{
    puts("OSClearContext is a stub");
}

BOOL OSDisableInterrupts()
{
    puts("OSDisableInterrupts is a stub");
    return FALSE;
}

void OSDumpContext(OSContext *context)
{
    puts("OSDumpContext is a stub");
}

OSThread *OSGetCurrentThread()
{
    puts("OSGetCurrentThread is a stub");
    return 0;
}

u16 OSGetFontEncode()
{
    puts("OSGetFontEncode is a stub");
    return 0;
}

char *OSGetFontTexture(char *string, void **image, s32 *x, s32 *y, s32 *width)
{
    puts("OSGetFontTexture is a stub");
    return 0;
}

char *OSGetFontWidth(char *string, s32 *width)
{
    puts("OSGetFontWidth is a stub");
    return 0;
}

BOOL OSGetResetButtonState()
{
    puts("OSGetResetButtonState is a stub");
    return FALSE;
}

u32 OSGetStackPointer()
{
    puts("OSGetStackPointer is a stub");
    return 0;
}

BOOL OSInitFont(OSFontHeader *fontData)
{
    puts("OSInitFont is a stub");
    return FALSE;
}

BOOL OSLink(OSModuleInfo *newModule, void *bss)
{
    puts("OSLink is a stub");
    return FALSE;
}

void OSLoadContext(OSContext *context)
{
    puts("OSLoadContext is a stub");
}

void OSResetSystem(int reset, u32 resetCode, BOOL forceMenu)
{
    puts("OSResetSystem is a stub");
}

BOOL OSRestoreInterrupts(BOOL level)
{
    puts("OSRestoreInterrupts is a stub");
    return FALSE;
}

s32 OSResumeThread(OSThread *thread)
{
    puts("OSResumeThread is a stub");
    return 0;
}

void OSSetCurrentContext(OSContext *context)
{
    puts("OSSetCurrentContext is a stub");
}

void OSSetStringTable(const void *stringTable)
{
    puts("OSSetStringTable is a stub");
}

s32 OSSuspendThread(OSThread *thread)
{
    puts("OSSuspendThread is a stub");
    return 0;
}

void OSTicksToCalendarTime(OSTime ticks, OSCalendarTime *td)
{
    puts("OSTicksToCalendarTime is a stub");
}

BOOL OSUnlink(OSModuleInfo *oldModule)
{
    puts("OSUnlink is a stub");
    return FALSE;
}

void OSWakeupThread(OSThreadQueue *queue)
{
    puts("OSWakeupThread is a stub");
}

void PPCHalt()
{
    puts("PPCHalt is a stub");
}

void PSMTXIdentity(Mtx m)
{
    puts("PSMTXIdentity is a stub");
}

void SoundChoID(int a, int b)
{
    puts("SoundChoID is a stub");
}

void SoundPan(int a, int b, int c)
{
    puts("SoundPan is a stub");
}

void SoundPitch(u16 a, int b)
{
    puts("SoundPitch is a stub");
}

void SoundRevID(int a, int b)
{
    puts("SoundRevID is a stub");
}

void VIConfigure(const GXRenderModeObj *rm)
{
    puts("VIConfigure is a stub");
}

void VIConfigurePan(u16 xOrg, u16 yOrg, u16 width, u16 height)
{
    puts("VIConfigurePan is a stub");
}

u32 VIGetNextField()
{
    puts("VIGetNextField is a stub");
    return 0;
}

void VISetBlack(BOOL black)
{
    puts("VISetBlack is a stub");
}

void VISetNextFrameBuffer(void *fb)
{
    puts("VISetNextFrameBuffer is a stub");
}

void VIWaitForRetrace()
{
    puts("VIWaitForRetrace is a stub");
}

s32 __CARDFormatRegionAsync(int a, int b)
{
    puts("__CARDFormatRegionAsync is a stub");
    return 0;
}

void __GXSetSUTexSize()
{
    puts("__GXSetSUTexSize is a stub");
}

void __GXSetVAT()
{
    puts("__GXSetVAT is a stub");
}

void __GXSetVCD()
{
    puts("__GXSetVCD is a stub");
}

void __GXUpdateBPMask()
{
    puts("__GXUpdateBPMask is a stub");
}

u32 __OSGetDIConfig()
{
    puts("__OSGetDIConfig is a stub");
    return 0;
}

__OSInterruptHandler __OSSetInterruptHandler(__OSInterrupt interrupt, __OSInterruptHandler handler)
{
    puts("__OSSetInterruptHandler is a stub");
    return 0;
}

OSInterruptMask __OSUnmaskInterrupts(OSInterruptMask mask)
{
    puts("__OSUnmaskInterrupts is a stub");
    return 0;
}

// Hudson
void HuDvdErrDispInit(GXRenderModeObj *rmode, void *xfb1, void *xfb2) { }

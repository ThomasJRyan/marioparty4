#include "game/thpmain.h"
#include "game/THPSimple.h"
#include "game/hu3d.h"
#include "game/init.h"
#include "game/process.h"
#include "game/sprite.h"

#include <string.h>

#define THP_DECODE_STACK_SIZE 0x2000

static void THPTestProc(void);
static void THPViewFunc(HU3DMODEL *arg0, Mtx arg1);
static void THPViewSprFunc(HUSPRITE *arg0);
static void THPDecodeFunc(void *param);

static char THPFileName[64];

static UnkThpStruct04 audioTrack;
static s32 decodeRate;
static u8 *decodeStackP;
static OSThread *decodeThread;
static s16 THPLoopF;
static s16 THPStat;
static u32 THPFrame;
static s16 THPStart;
HUPROCESS *THPProc;

/**
 * @brief Creates sprite-based THP playback and its worker process at an explicit volume.
 * @param path Path of the THP movie to open.
 * @param loop Nonzero to restart the THP stream after its final frame.
 * @param prio Lower numeric sprite draw priority.
 * @param volume Initial THP audio volume.
 * @return Sprite index used to display the movie, or -1 when playback cannot be created.
 */
s16 HuTHPSprCreateVol(char *path, s16 loop, s16 prio, float volume)
{
    s16 sprite_id;

    if (THPProc) {
        return -1;
    }
    THPStart = 0;
    THPProc = HuPrcCreate(THPTestProc, 0x64, 0x3000, 0);
    if (THPProc == 0) {
        return -1;
    }
    sprite_id = HuSprFuncCreate(THPViewSprFunc, prio);
    if (sprite_id == -1) {
        return -1;
    }
    THPSimpleInit(2);
    SimpleControl.unk19C = 0;
    SimpleControl.unk19E = sprite_id;
    SimpleControl.unkBC = volume;
    strcpy(THPFileName, path);
    THPLoopF = loop;
    THPStat = 0;
    THPFrame = 0;
    return sprite_id;
}

/**
 * @brief Creates sprite-based THP playback at the default volume of 110.
 * @param path Path of the THP movie to open.
 * @param loop Nonzero to restart the THP stream after its final frame.
 * @param prio Lower numeric sprite draw priority.
 * @return Sprite index used to display the movie, or -1 when playback cannot be created.
 */
s16 HuTHPSprCreate(char *path, s16 loop, s16 prio)
{
    return HuTHPSprCreateVol(path, loop, prio, 110.0f);
}

/**
 * @brief Creates model-hook THP playback and its worker process at an explicit volume.
 * @param path Path of the THP movie to open.
 * @param loop Nonzero to restart the THP stream after its final frame.
 * @param volume Initial THP audio volume.
 * @return Model-hook index used to display the movie, or -1 when its process cannot be created.
 */
s16 HuTHP3DCreateVol(char *path, s16 loop, float volume)
{
    s16 model_id;

    if (THPProc) {
        return;
    }
    THPStart = 0;
    THPProc = HuPrcCreate(THPTestProc, 0x64, 0x3000, 0);
    if (THPProc == 0) {
        return -1;
    }
    THPSimpleInit(2);
    SimpleControl.unk19C = 1;
    model_id = Hu3DHookFuncCreate(&THPViewFunc);
    SimpleControl.unk19E = model_id;
    SimpleControl.unkBC = volume;
    strcpy(THPFileName, path);
    THPLoopF = loop;
    THPStat = 0;
    THPFrame = 0;
    Hu3DModelAttrSet(SimpleControl.unk19E, HU3D_ATTR_DISPOFF);
    return model_id;
}

/**
 * @brief Creates model-hook THP playback at the default volume of 110.
 * @param path Path of the THP movie to open.
 * @param loop Nonzero to restart the THP stream after its final frame.
 * @return Model-hook index used to display the movie, or -1 when playback cannot be created.
 */
s16 HuTHP3DCreate(char *path, s16 loop)
{
    return HuTHP3DCreateVol(path, loop, 110.0f);
}

/**
 * @brief Requests that playback audio and loading stop without destroying the playback object.
 */
void HuTHPStop(void)
{
    THPStat = 1;
}

/**
 * @brief Requests complete playback shutdown and resource release.
 */
void HuTHPClose(void)
{
    THPStat = 2;
}

/**
 * @brief Requests that playback stop, preload again, and restart from the beginning.
 */
void HuTHPRestart(void)
{
    THPStat = 3;
}

/**
 * @brief Tests whether the displayed frame has reached the final decoded frame.
 * @return TRUE after a valid stream reaches its last frame; otherwise FALSE.
 */
BOOL HuTHPEndCheck(void)
{
    s32 last_frame;

    last_frame = THPSimpleGetTotalFrame() - 1;
    if (last_frame <= 0) {
        return FALSE;
    }
    return (last_frame <= THPFrame);
}

/**
 * @brief Reads the most recently drawn THP frame number.
 * @return Most recently drawn zero-based frame number.
 */
s32 HuTHPFrameGet(void)
{
    return THPFrame;
}

/**
 * @brief Reads the total frame count reported by the open THP stream.
 * @return Total number of frames reported by the THP stream.
 */
s32 HuTHPTotalFrameGet(void)
{
    return THPSimpleGetTotalFrame();
}

/**
 * @brief Sets the left and right THP audio-channel volumes.
 * @param left Left-channel THP volume.
 * @param right Right-channel THP volume.
 */
void HuTHPSetVolume(s32 left, s32 right)
{
    THPSimpleSetVolume(left, right);
}

/**
 * @brief Owns THP open, preload, playback-state transitions, and final resource cleanup.
 */
static void THPTestProc(void)
{
    s32 audio_buffer_size;
    u32 restart_audio;
    void *decoder_memory;

    while (THPSimpleOpen(THPFileName) == 0) {
        OSReport("THPSimpleOpen fail");
        HuPrcVSleep();
    }
    THPSimpleGetVideoInfo(&audioTrack);
    audio_buffer_size = THPSimpleCalcNeedMemory();
    OSReport("Size %x\n", audio_buffer_size);
    decoder_memory = HuMemDirectMalloc(HEAP_MODEL, audio_buffer_size);
    memset(decoder_memory, 0, audio_buffer_size);
    DCFlushRange(decoder_memory, audio_buffer_size);
    if (decoder_memory == 0) {
        OSReport("Can't allocate the memory\n");
    }
    THPSimpleSetBuffer(decoder_memory);
    while (THPSimplePreLoad(THPLoopF) == 0) {
        OSReport("THPSimplePreLoad fail");
        HuPrcVSleep();
    }
    HuPrcVSleep();
    decodeStackP = HuMemDirectMalloc(HEAP_HEAP, THP_DECODE_STACK_SIZE);
    decodeThread = OSSetIdleFunction(THPDecodeFunc, NULL, (decodeStackP + THP_DECODE_STACK_SIZE), THP_DECODE_STACK_SIZE);
    decodeRate = 0;
    THPStart = 1;
    if (SimpleControl.unk19C == 1) {
        Hu3DModelAttrReset(SimpleControl.unk19E, HU3D_ATTR_DISPOFF);
    }
    else {
        HuSprAttrReset(SimpleControl.unk19E, 0, HUSPR_ATTR_DISPOFF);
    }
    while (1) {
        if (THPStat == 2) {
            break;
        }
        switch (THPStat) {
            case 0:
                restart_audio = 1;
                THPStat = 4;
                break;
            case 1:
                THPSimpleAudioStop();
                THPSimpleLoadStop();
                break;
            case 3:
                THPSimpleAudioStop();
                THPSimpleLoadStop();
                while (THPSimplePreLoad(THPLoopF) == 0) {
                    OSReport("THPSimplePreLoad fail");
                    HuPrcVSleep();
                }
                THPStat = 0;
                restart_audio = 1;
                break;
        }
        if (restart_audio != 0) {
            THPSimpleAudioStart();
            restart_audio = 0;
        }
        HuPrcVSleep();
    }
    if (SimpleControl.unk19C == 1) {
        Hu3DModelKill(SimpleControl.unk19E);
    }
    else {
        HuSprKill(SimpleControl.unk19E);
    }
    OSCancelThread(decodeThread);
    HuMemDirectFree(decodeStackP);
    THPSimpleAudioStop();
    THPSimpleLoadStop();
    THPSimpleClose();
    HuMemDirectFree(decoder_memory);
    THPSimpleQuit();
    THPProc = NULL;
    HuPrcKill(HuPrcCurrentGet());
    while (1) {
        HuPrcVSleep();
    }
}

/**
 * @brief Draws the current THP frame through a 3D model hook and starts synchronized audio.
 * @param arg0 Model state associated with the hook; its fields are not read here.
 * @param arg1 Model-view matrix supplied to the 3D draw hook.
 */
static void THPViewFunc(HU3DMODEL *arg0, Mtx arg1)
{
    GXColor spC = { 0xFF, 0xFF, 0xFF, 0xFF };

    if (THPStart != 0) {
        GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GXSetCullMode(GX_CULL_NONE);
        THPFrame
            = THPSimpleDrawCurrentFrame(RenderMode, &spC, arg1, -audioTrack.unk00 / 2, audioTrack.unk04 / 2, audioTrack.unk00, -audioTrack.unk04);
        THPSimpleAudioStart();
    }
}

/**
 * @brief Draws the current THP frame using a sprite transform and modulation color.
 * @param arg0 Sprite transform and RGBA modulation used to draw the movie frame.
 */
static void THPViewSprFunc(HUSPRITE *arg0)
{
    Vec spC = { 0.0f, 0.0f, 1.0f };
    GXColor sprite_color;
    s32 draw_x;
    s32 draw_y;
    Mtx draw_matrix;
    Mtx rotation_matrix;

    if (THPStart != 0) {
        sprite_color.r = arg0->r;
        sprite_color.g = arg0->g;
        sprite_color.b = arg0->b;
        sprite_color.a = arg0->a;
        if (arg0->zRot != 0.0f) {
            PSMTXRotAxisRad(rotation_matrix, &spC, MTXDegToRad(arg0->zRot));
            PSMTXScale(draw_matrix, arg0->scale.x, arg0->scale.y, 1.0f);
            PSMTXConcat(rotation_matrix, draw_matrix, draw_matrix);
        }
        else {
            PSMTXScale(draw_matrix, arg0->scale.x, arg0->scale.y, 1.0f);
        }
        mtxTransCat(draw_matrix, arg0->pos.x, arg0->pos.y, 0.0f);
        PSMTXConcat(*arg0->groupMtx, draw_matrix, draw_matrix);
        draw_x = -((s32)audioTrack.unk00 / 2);
        draw_y = -((s32)audioTrack.unk04 / 2);
        GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
        THPFrame = THPSimpleDrawCurrentFrame(RenderMode, &sprite_color, draw_matrix, draw_x, draw_y, audioTrack.unk00, audioTrack.unk04);
    }
}

/**
 * @brief Continuously decodes THP data until close is requested.
 * @param param Unused decode-thread argument.
 */
static void THPDecodeFunc(void *param)
{
    while (1) {
        if (THPStat == 2) {
            break;
        }
        if (THPSimpleDecode() == 1) {
            OSReport("Fail to decode video data");
        }
        VIWaitForRetrace();
    }
}

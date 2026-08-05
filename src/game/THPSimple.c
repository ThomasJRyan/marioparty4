#include "game/THPSimple.h"
#include "game/THPDraw.h"
#include <string.h>

#define ALIGN32(val) (((val) + 0x1F) & ~0x1F)

static void __THPSimpleDVDCallback(s32 result, DVDFileInfo *file_info);
static s32 VideoDecode(void *video_data);
static void THPAudioMixCallback(void);

UnkThpStruct10 SimpleControl;
static u8 SoundBuffer[2][0x280];
static u8 WorkBuffer[0x40] ATTRIBUTE_ALIGN(32);

static s32 SoundBufferIndex;
static void *LastAudioBuffer;
static void *CurAudioBuffer;
static s32 bufIdx;
static s32 AudioSystem;
static void (*OldAIDCallback)();
static s32 Initialized;

/**
 * @brief Initializes THP decoding and installs the audio DMA callback.
 *
 * @param audio_mode Audio integration mode: zero drives DMA directly; one or two
 * chain the previous callback and select which DMA buffer address is mixed.
 * @return 1 on success, or 0 if THP initialization or callback integration fails.
 */
s32 THPSimpleInit(s32 audio_mode) {
    s32 interrupt_state;

    memset(&SimpleControl, 0, sizeof(SimpleControl));
    LCEnable();
    if (THPInit() == 0) {
        return 0;
    }
    interrupt_state = OSDisableInterrupts();
    AudioSystem = audio_mode;
    SoundBufferIndex = 0;
    LastAudioBuffer = NULL;
    CurAudioBuffer = NULL;
    OldAIDCallback = AIRegisterDMACallback(THPAudioMixCallback);
    if (OldAIDCallback == NULL && AudioSystem != 0) {
        AIRegisterDMACallback(NULL);
        OSRestoreInterrupts(interrupt_state);
        return 0;
    }
    OSRestoreInterrupts(interrupt_state);
    if (AudioSystem == 0) {
        memset(SoundBuffer, 0, sizeof(SoundBuffer));
        DCFlushRange(SoundBuffer, sizeof(SoundBuffer));
        AIInitDMA((u32) &SoundBuffer[SoundBufferIndex], sizeof(SoundBuffer[0]));
        AIStartDMA();
    }
    Initialized = 1;
    return 1;
}

/**
 * @brief Shuts down simple THP playback and restores the previous DMA callback.
 */
void THPSimpleQuit(void) {
    s32 interrupt_state;

    LCDisable();
    interrupt_state = OSDisableInterrupts();
    if (OldAIDCallback) {
        AIRegisterDMACallback(OldAIDCallback);
    }
    OSRestoreInterrupts(interrupt_state);
    Initialized = 0;
}

/**
 * @brief Opens a THP file and reads its stream component metadata.
 *
 * @param path Null-terminated DVD path to the THP file opened by DVDOpen.
 * @return 1 when a version 1.0 THP with supported video/audio components is
 * ready, or 0 on invalid state, I/O failure, or unsupported metadata.
 */
s32 THPSimpleOpen(char *path) {
    s32 component_info_offset;
    s32 i;

    if (Initialized == 0) {
        return 0;
    }
    if (SimpleControl.unk98 != 0) {
        return 0;
    }
    memset(&SimpleControl.unk80, 0, sizeof(SimpleControl.unk80));
    memset(SimpleControl.unk88, 0, sizeof(SimpleControl.unk88));
    if (DVDOpen(path, &SimpleControl.unk00) == 0) {
        return 0;
    }
    if (DVDReadPrio(&SimpleControl.unk00, &WorkBuffer, 0x40, 0, 2) < 0) {
        DVDClose(&SimpleControl.unk00);
        return 0;
    }
    memcpy(&SimpleControl.unk3C, &WorkBuffer, sizeof(SimpleControl.unk3C));
    if (strcmp(SimpleControl.unk3C.mMagic, "THP") != 0) {
        DVDClose(&SimpleControl.unk00);
        return 0;
    }
    if (SimpleControl.unk3C.mVersion != 0x10000) {
        DVDClose(&SimpleControl.unk00);
        return 0;
    }
    component_info_offset = SimpleControl.unk3C.mCompInfoDataOffsets;
    if (DVDReadPrio(&SimpleControl.unk00, &WorkBuffer, 0x20, component_info_offset, 2) < 0) {
        DVDClose(&SimpleControl.unk00);
        return 0;
    }
    memcpy(&SimpleControl.unk6C, &WorkBuffer, sizeof(SimpleControl.unk6C));
    component_info_offset += sizeof(SimpleControl.unk6C);
    SimpleControl.unk9F = 0;
    for (i = 0; i < SimpleControl.unk6C.mNumComponents; i++) {
        switch (SimpleControl.unk6C.mFrameComp[i]) {
            case 0:
                if (DVDReadPrio(&SimpleControl.unk00, &WorkBuffer, 0x20, component_info_offset, 2) < 0) {
                    DVDClose(&SimpleControl.unk00);
                    return 0;
                }
                memcpy(&SimpleControl.unk80, &WorkBuffer, sizeof(SimpleControl.unk80));
                component_info_offset += sizeof(SimpleControl.unk80);
                break;
            case 1:
                if (DVDReadPrio(&SimpleControl.unk00, &WorkBuffer, 0x20, component_info_offset, 2) < 0) {
                    DVDClose(&SimpleControl.unk00);
                    return 0;
                }
                memcpy(SimpleControl.unk88, &WorkBuffer, sizeof(SimpleControl.unk88));
                SimpleControl.unk9F = 1;
                component_info_offset += sizeof(SimpleControl.unk88);
                break;
            default:
                return 0;
        }
    }
    SimpleControl.unkA0 = SimpleControl.unk3C.mMovieDataOffsets;
    SimpleControl.unkB4 = SimpleControl.unk3C.mFirstFrameSize;
    SimpleControl.unkB0 = 0;
    SimpleControl.unkB8 = 0;
    SimpleControl.unkA4 = 0;
    SimpleControl.unk144[0].unk0C = -1;
    SimpleControl.unkAC = 0;
    SimpleControl.unk194 = 0;
    SimpleControl.unk198 = 0;
    SimpleControl.unk9C = 0;
    SimpleControl.unk9D = 0;
    SimpleControl.unk9E = 0;
    SimpleControl.unk98 = 1;
    SimpleControl.unkC0 = SimpleControl.unkBC;
    SimpleControl.unkC8 = 0;
    return 1;
}

/**
 * @brief Closes an idle THP stream.
 *
 * @return 1 when the DVD file is closed, or 0 while playback, audio, or an
 * asynchronous read still prevents closure.
 */
s32 THPSimpleClose(void) {
    if (SimpleControl.unk98 != 0 && SimpleControl.unk9C == 0) {
        if (SimpleControl.unk9F != 0) {
            if (SimpleControl.unk9D == 1) {
                return 0;
            }
        } else {
            SimpleControl.unk9D = 0;
        }
        if (SimpleControl.unkA8 == 0) {
            SimpleControl.unk98 = 0;
            DVDClose(&SimpleControl.unk00);
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Calculates contiguous work memory required by the open THP stream.
 *
 * @return Required byte count for frame, decode, and optional audio buffers,
 * including 32-byte alignment and 0x1000 bytes of decoder work space; zero if
 * no stream is open.
 */
s32 THPSimpleCalcNeedMemory(void) {
    s32 required_size;

    if (SimpleControl.unk98 != 0) {
        required_size = ALIGN32(SimpleControl.unk3C.mBufferSize) * 10;
        required_size += ALIGN32(SimpleControl.unk80.unk00 * SimpleControl.unk80.unk04);
        required_size += ALIGN32(SimpleControl.unk80.unk00 * SimpleControl.unk80.unk04 / 4);
        required_size += ALIGN32(SimpleControl.unk80.unk00 * SimpleControl.unk80.unk04 / 4);
        required_size += ALIGN32(SimpleControl.unk80.unk00 * SimpleControl.unk80.unk04);
        required_size += ALIGN32(SimpleControl.unk80.unk00 * SimpleControl.unk80.unk04 / 4);
        required_size += ALIGN32(SimpleControl.unk80.unk00 * SimpleControl.unk80.unk04 / 4);
        if (SimpleControl.unk9F != 0) {
            required_size += ALIGN32(SimpleControl.unk3C.mAudioMaxSamples * 4) * 4;
        }
        required_size += 0x1000;
        return required_size;
    }
    return 0;
}

/**
 * @brief Partitions caller-owned memory into THP video, read, audio, and work buffers.
 *
 * @param buffer Start of a writable region at least THPSimpleCalcNeedMemory bytes
 * long; ownership remains with the caller for the stream lifetime.
 * @return 0 only when audio playback is already active; otherwise 1. A call in
 * an unopened or already-loading state performs no partitioning but still returns 1.
 */
s32 THPSimpleSetBuffer(void *buffer) {
    u8 *buffer_cursor;
    u32 luma_buffer_size;
    u32 chroma_buffer_size;
    u32 i;

    if (SimpleControl.unk98 != 0 && SimpleControl.unk9C == 0) {
        if (SimpleControl.unk9D == 1) {
            return 0;
        }
        luma_buffer_size = ALIGN32(SimpleControl.unk80.unk00 * SimpleControl.unk80.unk04);
        chroma_buffer_size = ALIGN32(SimpleControl.unk80.unk00 * SimpleControl.unk80.unk04 / 4);
        buffer_cursor = buffer;
        for (i = 0; i < 2; i++) {
            SimpleControl.unk144[i].unk00 = buffer_cursor;
            DCInvalidateRange(buffer_cursor, luma_buffer_size);
            buffer_cursor += luma_buffer_size;
            SimpleControl.unk144[i].unk04 = buffer_cursor;
            DCInvalidateRange(buffer_cursor, chroma_buffer_size);
            buffer_cursor += chroma_buffer_size;
            SimpleControl.unk144[i].unk08 = buffer_cursor;
            DCInvalidateRange(buffer_cursor, chroma_buffer_size);
            buffer_cursor += chroma_buffer_size;
        }
        for (i = 0; i < 10; i++) {
            SimpleControl.unkCC[i].unk00 = (s32*) buffer_cursor;
            buffer_cursor += ALIGN32(SimpleControl.unk3C.mBufferSize);
            SimpleControl.unkCC[i].unk08 = 0;
        }
        if (SimpleControl.unk9F != 0) {
            for (i = 0; i < 4; i++) {
                SimpleControl.unk164[i].unk00 = buffer_cursor;
                SimpleControl.unk164[i].unk04 = buffer_cursor;
                SimpleControl.unk164[i].unk08 = 0;
                buffer_cursor += ALIGN32(SimpleControl.unk3C.mAudioMaxSamples * 4);
            }
        }
        SimpleControl.unk94 = buffer_cursor;
    }
    return 1;
}

/**
 * @brief Starts the next asynchronous frame read into the ten-slot read ring.
 *
 * @details At end of stream, looping mode rewinds to the first frame; otherwise
 * no read is submitted. Submission failure sets the persistent read-error flag.
 */
static inline void THPSimpleInlineFunc(void) {
    if (SimpleControl.unkB8 > SimpleControl.unk3C.mNumFrames - 1) {
        if (SimpleControl.unk9E != 1) {
            return;
        }
        SimpleControl.unkB8 = 0;
        SimpleControl.unkA0 = SimpleControl.unk3C.mMovieDataOffsets;
        SimpleControl.unkB4 = SimpleControl.unk3C.mFirstFrameSize;
    }
    SimpleControl.unkA8 = 1;
    if (DVDReadAsyncPrio(&SimpleControl.unk00, SimpleControl.unkCC[SimpleControl.unkB0].unk00, SimpleControl.unkB4, SimpleControl.unkA0, __THPSimpleDVDCallback, 2) != 1) {
        SimpleControl.unkA8 = 0;
        SimpleControl.unkA4 = 1;
    }
}

/**
 * @brief Advances the frame-read ring after an asynchronous DVD read completes.
 *
 * @param result DVD result: -1 records an error, -3 is ignored as cancellation,
 * and other values publish the filled frame slot.
 * @param file_info DVD file descriptor associated with the completed request.
 */
static void __THPSimpleDVDCallback(s32 result, DVDFileInfo *file_info) {
    s32 next_read_index;

    if (result == -1) {
        SimpleControl.unkA4 = 1;
        return;
    }
    if (result != -3) {
        SimpleControl.unkA8 = 0;
        SimpleControl.unkCC[SimpleControl.unkB0].unk04 = SimpleControl.unkB8;
        SimpleControl.unkB8++;
        SimpleControl.unkCC[SimpleControl.unkB0].unk08 = 1;
        SimpleControl.unkA0 += SimpleControl.unkB4;
        SimpleControl.unkB4 = *SimpleControl.unkCC[SimpleControl.unkB0].unk00;
        if (SimpleControl.unkB0 + 1 >= 10) {
            next_read_index = 0;
        } else {
            next_read_index = SimpleControl.unkB0 + 1;
        }
        SimpleControl.unkB0 = next_read_index;
        if (SimpleControl.unkCC[SimpleControl.unkB0].unk08 == 0 && SimpleControl.unkA4 == 0 && SimpleControl.unk9C == 1) {
            THPSimpleInlineFunc();
        }
    }
}

/**
 * @brief Synchronously preloads the initial THP frames into the read ring.
 *
 * @param loop Nonzero enables looping and fills all ten read slots; zero loads
 * at most the movie's frame count.
 * @return 1 when preloading starts playback state, or 0 on invalid state or a
 * DVD read error.
 */
s32 THPSimplePreLoad(s32 loop) {
    u32 preload_count;
    s32 next_read_index;
    s32 i;

    if (SimpleControl.unk98 != 0 && SimpleControl.unk9C == 0) {
        preload_count = 10;
        if (loop == 0 && SimpleControl.unk3C.mNumFrames < 10) {
            preload_count = SimpleControl.unk3C.mNumFrames;
        }
        for (i = 0; i < preload_count; i++) {
            if (DVDReadPrio(&SimpleControl.unk00, SimpleControl.unkCC[SimpleControl.unkB0].unk00, SimpleControl.unkB4, SimpleControl.unkA0, 2) < 0) {
                SimpleControl.unkA4 = 1;
                return 0;
            }
            SimpleControl.unkA0 += SimpleControl.unkB4;
            SimpleControl.unkB4 = *SimpleControl.unkCC[SimpleControl.unkB0].unk00;
            SimpleControl.unkCC[SimpleControl.unkB0].unk08 = 1;
            SimpleControl.unkCC[SimpleControl.unkB0].unk04 = SimpleControl.unkB8;
            if (SimpleControl.unkB0 + 1 >= 10) {
                next_read_index = 0;
            } else {
                next_read_index = SimpleControl.unkB0 + 1;
            }
            SimpleControl.unkB0 = next_read_index;
            SimpleControl.unkB8++;
            if (SimpleControl.unkB8 > SimpleControl.unk3C.mNumFrames - 1 && SimpleControl.unk9E == 1) {
                SimpleControl.unkB8 = 0;
                SimpleControl.unkA0 = SimpleControl.unk3C.mMovieDataOffsets;
                SimpleControl.unkB4 = SimpleControl.unk3C.mFirstFrameSize;
            }
        }
        SimpleControl.unk9E = loop;
        SimpleControl.unk9C = 1;
        return 1;
    }
    return 0;
}

/**
 * @brief Enables decoded THP audio mixing during DMA callbacks.
 */
void THPSimpleAudioStart(void) {
    SimpleControl.unk9D = 1;
}

/**
 * @brief Disables decoded THP audio mixing during DMA callbacks.
 */
void THPSimpleAudioStop(void) {
    SimpleControl.unk9D = 0;
}

/**
 * @brief Stops frame loading and resets the stream's read/decode rings.
 *
 * @return 1 after resetting an open stream with audio stopped, or 0 if the
 * stream is closed or audio mixing remains active.
 */
s32 THPSimpleLoadStop(void) {
    s32 i;

    if (SimpleControl.unk98 != 0 && SimpleControl.unk9D == 0) {
        SimpleControl.unk9C = 0;
        if (SimpleControl.unkA8 != 0) {
            DVDCancel(&SimpleControl.unk00.cb);
            SimpleControl.unkA8 = 0;
        }
        for (i = 0; i < 10; i++) {
            SimpleControl.unkCC[i].unk08 = 0;
        }
        for (i = 0; i < 4; i++) {
            SimpleControl.unk164[i].unk08 = 0;
        }
        SimpleControl.unk144[0].unk0C = -1;
        SimpleControl.unkA0 = SimpleControl.unk3C.mMovieDataOffsets;
        SimpleControl.unkB4 = SimpleControl.unk3C.mFirstFrameSize;
        SimpleControl.unkB0 = 0;
        SimpleControl.unkB8 = 0;
        SimpleControl.unkA4 = 0;
        SimpleControl.unkAC = 0;
        SimpleControl.unk194 = 0;
        SimpleControl.unk198 = 0;
        SimpleControl.unkBC = SimpleControl.unkC0;
        SimpleControl.unkC8 = 0;
        return 1;
    }
    return 0;
}

/**
 * @brief Decodes the next loaded THP frame's video and optional audio components.
 *
 * @return 0 after consuming a frame, 1 when video decoding fails, 2 when no
 * loaded frame is ready, or 3 when the next audio decode buffer is still occupied.
 */
s32 THPSimpleDecode(void) {
    u8 *component_data;
    s32 *component_sizes;
    s32 interrupt_state;
    s32 decoded_sample_count;
    s32 load_interrupt_state;
    s32 next_decode_index;
    s32 i;

    if (SimpleControl.unkCC[SimpleControl.unkAC].unk08 != 0) {
        component_sizes = SimpleControl.unkCC[SimpleControl.unkAC].unk00 + 2;
        component_data = (u8*) (&SimpleControl.unkCC[SimpleControl.unkAC].unk00[SimpleControl.unk6C.mNumComponents] + 2);
        if (SimpleControl.unk9F != 0) {
            if (SimpleControl.unk164[SimpleControl.unk194].unk08 == 0) {
                for (i = 0; i < SimpleControl.unk6C.mNumComponents; component_data += *component_sizes, component_sizes++, i++) {
                    switch (SimpleControl.unk6C.mFrameComp[i]) {
                        case 0:
                            if (VideoDecode(component_data) == 0) {
                                return 1;
                            }
                            break;
                        case 1:
                            decoded_sample_count = THPAudioDecode(SimpleControl.unk164[SimpleControl.unk194].unk00, component_data, 0);
                            interrupt_state = OSDisableInterrupts();
                            SimpleControl.unk164[SimpleControl.unk194].unk08 = decoded_sample_count;
                            SimpleControl.unk164[SimpleControl.unk194].unk04 = SimpleControl.unk164[SimpleControl.unk194].unk00;
                            OSRestoreInterrupts(interrupt_state);
                            SimpleControl.unk194++;
                            if (SimpleControl.unk194 >= 4) {
                                SimpleControl.unk194 = 0;
                            }
                            break;
                    }
                }
            } else {
                return 3;
            }
        } else {
            for (i = 0; i < SimpleControl.unk6C.mNumComponents; component_data += *component_sizes, component_sizes++, i++) {
                if ((s32) SimpleControl.unk6C.mFrameComp[i] != 0) {
                    (void) i; // required to match. Optimized out unreachable code?
                } else if (VideoDecode(component_data) == 0) {
                    return 1;
                }
            }
        }
        SimpleControl.unkCC[SimpleControl.unkAC].unk08 = 0;
        if (SimpleControl.unkAC + 1 >= 10) {
            next_decode_index = 0;
        } else {
            next_decode_index = SimpleControl.unkAC + 1;
        }
        SimpleControl.unkAC = next_decode_index;
        load_interrupt_state = OSDisableInterrupts();
        if (SimpleControl.unkCC[SimpleControl.unkB0].unk08 == 0 && SimpleControl.unkA8 == 0 && SimpleControl.unkA4 == 0 && SimpleControl.unk9C == 1) {
            THPSimpleInlineFunc();
        }
        OSRestoreInterrupts(load_interrupt_state);
        return 0;
    } else {
        return 2;
    }
}

/**
 * @brief Decodes one THP video component into the alternate YUV frame buffer.
 *
 * @param video_data Compressed video component payload from the current frame.
 * @return 1 when decoding succeeds and a frame number is published, otherwise 0.
 */
static s32 VideoDecode(void *video_data) {
    s32 decode_result;

    bufIdx = (bufIdx + 1) & 1;
    decode_result = THPVideoDecode(video_data, SimpleControl.unk144[bufIdx].unk00, SimpleControl.unk144[bufIdx].unk04, SimpleControl.unk144[bufIdx].unk08, SimpleControl.unk94);
    if (decode_result == 0) {
        SimpleControl.unk144[0].unk0C = SimpleControl.unkCC[SimpleControl.unkAC].unk04;
        return 1;
    }
    return 0;
}

/**
 * @brief Converts and draws the most recently decoded THP frame.
 *
 * @param render_mode Render-mode state used while configuring GX.
 * @param output_color Final TEV color, including the video's output alpha.
 * @param position_matrix Position matrix for the destination quad.
 * @param x Destination left coordinate in screen space.
 * @param y Destination top coordinate in screen space.
 * @param width Destination quad width.
 * @param height Destination quad height; negative values vertically flip the quad.
 * @return Zero-based decoded frame number, or -1 before any frame is available.
 */
s32 THPSimpleDrawCurrentFrame(GXRenderModeObj *render_mode, GXColor *output_color, Mtx position_matrix, s32 x, s32 y, s32 width, s32 height) {
    if (SimpleControl.unk144[0].unk0C >= 0) {
        THPGXYuv2RgbSetup(render_mode, output_color, position_matrix);
        THPGXYuv2RgbDraw(SimpleControl.unk144[bufIdx].unk00, SimpleControl.unk144[bufIdx].unk04, SimpleControl.unk144[bufIdx].unk08, x, y, SimpleControl.unk80.unk00, SimpleControl.unk80.unk04, width, height);
        THPGXRestore();
        return SimpleControl.unk144[0].unk0C;
    }
    return -1;
}

static u16 VolumeTable[] = {
    0x0000, 0x0002, 0x0008, 0x0012, 0x0020, 0x0032, 0x0049, 0x0063,
    0x0082, 0x00A4, 0x00CB, 0x00F5, 0x0124, 0x0157, 0x018E, 0x01C9,
    0x0208, 0x024B, 0x0292, 0x02DD, 0x032C, 0x037F, 0x03D7, 0x0432,
    0x0492, 0x04F5, 0x055D, 0x05C9, 0x0638, 0x06AC, 0x0724, 0x07A0,
    0x0820, 0x08A4, 0x092C, 0x09B8, 0x0A48, 0x0ADD, 0x0B75, 0x0C12,
    0x0CB2, 0x0D57, 0x0DFF, 0x0EAC, 0x0F5D, 0x1012, 0x10CA, 0x1187,
    0x1248, 0x130D, 0x13D7, 0x14A4, 0x1575, 0x164A, 0x1724, 0x1801,
    0x18E3, 0x19C8, 0x1AB2, 0x1BA0, 0x1C91, 0x1D87, 0x1E81, 0x1F7F,
    0x2081, 0x2187, 0x2291, 0x239F, 0x24B2, 0x25C8, 0x26E2, 0x2801,
    0x2923, 0x2A4A, 0x2B75, 0x2CA3, 0x2DD6, 0x2F0D, 0x3048, 0x3187,
    0x32CA, 0x3411, 0x355C, 0x36AB, 0x37FF, 0x3956, 0x3AB1, 0x3C11,
    0x3D74, 0x3EDC, 0x4048, 0x41B7, 0x432B, 0x44A3, 0x461F, 0x479F,
    0x4923, 0x4AAB, 0x4C37, 0x4DC7, 0x4F5C, 0x50F4, 0x5290, 0x5431,
    0x55D6, 0x577E, 0x592B, 0x5ADC, 0x5C90, 0x5E49, 0x6006, 0x61C7,
    0x638C, 0x6555, 0x6722, 0x68F4, 0x6AC9, 0x6CA2, 0x6E80, 0x7061,
    0x7247, 0x7430, 0x761E, 0x7810, 0x7A06, 0x7C00, 0x7DFE, 0x8000
};

/**
 * @brief Mixes queued THP stereo samples into an AI DMA buffer with saturation.
 *
 * @details When no movie samples are ready, the remaining destination is copied
 * from the source mix or cleared. Volume changes use the configured sample-count ramp.
 *
 * @param destination Destination interleaved signed 16-bit stereo buffer.
 * @param source Optional existing interleaved stereo mix; NULL means silence.
 * @param sample_frames Number of stereo sample frames to produce.
 */
static void MixAudio(void *destination, void *source, u32 sample_frames) {
    s32 mixed_sample;
    u32 samples_to_mix;
    u32 samples_remaining;
    s32 i;
    u16 volume_scale;
    s16 *source_samples;
    s16 *destination_samples;
    s16 *movie_samples;

    if (source) {
        if (SimpleControl.unk98 != 0 && SimpleControl.unk9D == 1 && SimpleControl.unk9F != 0) {
            samples_remaining = sample_frames;
            destination_samples = destination;
            source_samples = source;
            while (1) {
                if (SimpleControl.unk164[SimpleControl.unk198].unk08 == 0) {
                    break;
                }
                if (SimpleControl.unk164[SimpleControl.unk198].unk08 >= samples_remaining) {
                    samples_to_mix = samples_remaining;
                } else {
                    samples_to_mix = SimpleControl.unk164[SimpleControl.unk198].unk08;
                }
                movie_samples = SimpleControl.unk164[SimpleControl.unk198].unk04;
                for (i = 0; i < samples_to_mix; i++) {
                    if (SimpleControl.unkC8 != 0) {
                        SimpleControl.unkC8--;
                        SimpleControl.unkBC += SimpleControl.unkC4;
                    } else {
                        SimpleControl.unkBC = SimpleControl.unkC0;
                    }
                    volume_scale = VolumeTable[(s32) SimpleControl.unkBC];
                    mixed_sample = *source_samples + ((volume_scale * *movie_samples) >> 0xF);
                    if (mixed_sample < -0x8000) {
                        mixed_sample = -0x8000;
                    }
                    if (mixed_sample > 0x7FFF) {
                        mixed_sample = 0x7FFF;
                    }
                    *destination_samples = mixed_sample;
                    destination_samples++;
                    source_samples++;
                    movie_samples++;
                    mixed_sample = *source_samples + ((volume_scale * *movie_samples) >> 0xF);
                    if (mixed_sample < -0x8000) {
                        mixed_sample = -0x8000;
                    }
                    if (mixed_sample > 0x7FFF) {
                        mixed_sample = 0x7FFF;
                    }
                    *destination_samples = mixed_sample;
                    destination_samples++;
                    source_samples++;
                    movie_samples++;
                }
                samples_remaining -= samples_to_mix;
                SimpleControl.unk164[SimpleControl.unk198].unk08 -= samples_to_mix;
                SimpleControl.unk164[SimpleControl.unk198].unk04 = movie_samples;
                if (SimpleControl.unk164[SimpleControl.unk198].unk08 == 0) {
                    SimpleControl.unk198++;
                    if (SimpleControl.unk198 >= 4) {
                        SimpleControl.unk198 = 0;
                    }
                }
                if (samples_remaining == 0) {
                    return;
                }
            }
            memcpy(destination_samples, source_samples, samples_remaining * 4);
        } else {
            memcpy(destination, source, sample_frames * 4);
        }
    } else if (SimpleControl.unk98 != 0 && SimpleControl.unk9D == 1 && SimpleControl.unk9F != 0) {
        samples_remaining = sample_frames;
        destination_samples = destination;
        while (1) {
            if (SimpleControl.unk164[SimpleControl.unk198].unk08 == 0) {
                break;
            }
            if (SimpleControl.unk164[SimpleControl.unk198].unk08 >= samples_remaining) {
                samples_to_mix = samples_remaining;
            } else {
                samples_to_mix = SimpleControl.unk164[SimpleControl.unk198].unk08;
            }
            movie_samples = SimpleControl.unk164[SimpleControl.unk198].unk04;
            for (i = 0; i < samples_to_mix; i++) {
                if (SimpleControl.unkC8 != 0) {
                    SimpleControl.unkC8--;
                    SimpleControl.unkBC += SimpleControl.unkC4;
                } else {
                    SimpleControl.unkBC = SimpleControl.unkC0;
                }
                volume_scale = VolumeTable[(s32) SimpleControl.unkBC];
                mixed_sample = (volume_scale * *movie_samples) >> 0xF;
                if (mixed_sample < -0x8000) {
                    mixed_sample = -0x8000;
                }
                if (mixed_sample > 0x7FFF) {
                    mixed_sample = 0x7FFF;
                }
                *destination_samples = mixed_sample;
                destination_samples++;
                movie_samples++;
                mixed_sample = (volume_scale * *movie_samples) >> 0xF;
                if (mixed_sample < -0x8000) {
                    mixed_sample = -0x8000;
                }
                if (mixed_sample > 0x7FFF) {
                    mixed_sample = 0x7FFF;
                }
                *destination_samples = mixed_sample;
                destination_samples++;
                movie_samples++;
            }
            samples_remaining -= samples_to_mix;
            SimpleControl.unk164[SimpleControl.unk198].unk08 -= samples_to_mix;
            SimpleControl.unk164[SimpleControl.unk198].unk04 = movie_samples;
            if (SimpleControl.unk164[SimpleControl.unk198].unk08 == 0) {
                SimpleControl.unk198++;
                if (SimpleControl.unk198 >= 4) {
                    SimpleControl.unk198 = 0;
                }
            }
            if (samples_remaining == 0) {
                return;
            }
        }
        memset(destination_samples, 0, samples_remaining * 4);
    } else {
        memset(destination, 0, sample_frames * 4);
    }
}

/**
 * @brief Copies the open movie's video dimensions.
 *
 * @param video_info Receives width and height as two 32-bit values.
 * @return 1 when a stream is open and the fields were copied, otherwise 0.
 */
s32 THPSimpleGetVideoInfo(UnkThpStruct04 *video_info) {
    if (SimpleControl.unk98 != 0) {
        memcpy(video_info, &SimpleControl.unk80, 8);
        return 1;
    }
    return 0;
}

/**
 * @brief Returns the open movie's frame count.
 *
 * @return Total number of THP frames, or zero when no stream is open.
 */
s32 THPSimpleGetTotalFrame(void) {
    if (SimpleControl.unk98 != 0) {
        return SimpleControl.unk3C.mNumFrames;
    }
    return 0;
}

/**
 * @brief Produces the next 0x280-byte audio DMA buffer and chains host audio.
 */
static void THPAudioMixCallback(void) {
    s32 interrupt_state;

    if (AudioSystem == 0) {
        SoundBufferIndex ^= 1;
        AIInitDMA((u32) SoundBuffer[SoundBufferIndex], sizeof(SoundBuffer[0]));
        interrupt_state = OSEnableInterrupts();
        MixAudio(SoundBuffer[SoundBufferIndex], 0, 0xA0);
        DCFlushRange(SoundBuffer[SoundBufferIndex], sizeof(SoundBuffer[0]));
        OSRestoreInterrupts(interrupt_state);
    } else {
        if (AudioSystem == 1) {
            if (LastAudioBuffer) {
                CurAudioBuffer = LastAudioBuffer;
            }
            OldAIDCallback();
            LastAudioBuffer = (u8*) AIGetDMAStartAddr() + 0x80000000;
        } else {
            OldAIDCallback();
            CurAudioBuffer = (u8*) AIGetDMAStartAddr() + 0x80000000;
        }
        SoundBufferIndex ^= 1;
        AIInitDMA((u32) SoundBuffer[SoundBufferIndex], sizeof(SoundBuffer[0]));
        interrupt_state = OSEnableInterrupts();
        if (CurAudioBuffer) {
            DCInvalidateRange(CurAudioBuffer, sizeof(SoundBuffer[0]));
        }
        MixAudio(SoundBuffer[SoundBufferIndex], CurAudioBuffer, 0xA0);
        DCFlushRange(SoundBuffer[SoundBufferIndex], sizeof(SoundBuffer[0]));
        OSRestoreInterrupts(interrupt_state);
    }
}

/**
 * @brief Sets THP audio volume immediately or over a sample-count ramp.
 *
 * @param volume Target volume index, clamped to the inclusive range 0..127.
 * @param fade_duration Ramp duration control, clamped to 0..60000 and multiplied by
 * 32 stereo sample frames; zero applies the target immediately.
 * @return 1 when an open stream contains audio and the setting is accepted,
 * otherwise 0.
 */
s32 THPSimpleSetVolume(s32 volume, s32 fade_duration) {
    s32 interrupt_state;

    if (SimpleControl.unk98 != 0 && SimpleControl.unk9F != 0) {
        if (volume > 127) {
            volume = 127;
        }
        if (volume < 0) {
            volume = 0;
        }
        if (fade_duration > 60000) {
            fade_duration = 60000;
        }
        if (fade_duration < 0) {
            fade_duration = 0;
        }
        interrupt_state = OSDisableInterrupts();
        SimpleControl.unkC0 = volume;
        if (fade_duration != 0) {
            SimpleControl.unkC8 = (fade_duration << 5);
            SimpleControl.unkC4 = (SimpleControl.unkC0 - SimpleControl.unkBC) / SimpleControl.unkC8;
        } else {
            SimpleControl.unkC8 = 0;
            SimpleControl.unkBC = SimpleControl.unkC0;
        }
        OSRestoreInterrupts(interrupt_state);
        return 1;
    }
    return 0;
}

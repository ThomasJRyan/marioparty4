#include "game/data.h"
#include "dolphin/os.h"

typedef struct Decode_s
{
    u8 *src;
    u8 *dst;
    u32 size;
} DECODE;

static u8 textBuffer[1024];

/**
 * @brief Copy an uncompressed data stream to its destination.
 *
 * @param [in, out] decode Source/destination cursors and remaining output bytes.
 */
static void HuDecodeNone(DECODE *decode)
{
    while(decode->size) {
        *decode->dst++ = *decode->src++;
        decode->size--;
    }
}

/**
 * @brief Decode the game's LZ-compressed data format.
 *
 * @param [in, out] decode Source/destination cursors and remaining output bytes;
 *                         decoding uses a cleared 1 KiB sliding window.
 */
static void HuDecodeLz(DECODE *decode)
{
    u16 flag, pos;
    s32 i, j, copyLen;
    flag = 0;
    pos = 958;

    for(i=0; i<1024; i++) {
        textBuffer[i] = 0;
    }
    while(decode->size) {
        flag >>= 1;
        if(!(flag & 0x100)) {
            flag = (*decode->src++)|0xFF00;
        }
        if(flag & 0x1) {
            textBuffer[pos++] = *decode->dst++ = *decode->src++;
            pos = pos & 0x3FF;
            decode->size--;
        } else {
            i = *decode->src++;
            copyLen = *decode->src++;
            i |= ((copyLen & ~0x3F) << 2);
            copyLen = (copyLen & 0x3F)+3;
            for(j=0; j<copyLen; j++) {
                textBuffer[pos++] = *decode->dst++ = textBuffer[(i+j) & 0x3FF];
                pos &= 0x3FF;
            }
            decode->size -= j;
        }
    }
}

/**
 * @brief Consume the four-byte header of a slide-compressed stream.
 *
 * @param [in, out] decode Decoder whose source cursor advances by four bytes.
 */
static inline void SlideReadHeader(DECODE *decode)
{
    s32 size;
    size = (*decode->src++) << 24;
    size += (*decode->src++) << 16;
    size += (*decode->src++) << 8;
    size += *decode->src++;
}

/**
 * @brief Decode slide-compressed data with zero-filled back references.
 *
 * @param [in, out] decode Source/destination cursors and remaining output bytes;
 *                         back references before the output start produce zeroes.
 */
static void HuDecodeSlide(DECODE *decode)
{
    u8 *dstPOrig;
    u32 flagLen, flag;
    SlideReadHeader(decode);
    flagLen = 0;
    flag = 0;
    dstPOrig = decode->dst;
    while(decode->size) {
        if(flagLen == 0) {
            flag = (*decode->src++) << 24;
            flag += (*decode->src++) << 16;
            flag += (*decode->src++) << 8;
            flag += *decode->src++;
            flagLen = 32;
        }
        if(flag >> 31) {
            *decode->dst++ = (s32)*decode->src++;
            decode->size--;
        } else {
            u8 *src;
            u32 dist, len;
            dist = *decode->src++ << 8;
            dist += *decode->src++;
            len = (dist >> 12) & 0xF;
            dist &= 0xFFF;
            src = decode->dst-dist;
            if(len == 0) {
                len = (*decode->src++)+18;
            } else {
                len += 2;
            }
            decode->size -= len;
            while(len) {
                if(src-1 < dstPOrig) {
                    *decode->dst++ = 0;
                } else {
                    *decode->dst++ = src[-1];
                }
                len--;
                src++;
            }
        }

        flag <<= 1;
        flagLen--;
    }
}

/**
 * @brief Decode fast slide-compressed data.
 *
 * @param [in, out] decode Source/destination cursors and remaining output bytes;
 *                         all back references must address prior output.
 */
static void HuDecodeFslide(DECODE *decode)
{
    u32 flagLen, flag;
    SlideReadHeader(decode);
    flagLen = 0;
    flag = 0;
    while(decode->size) {
        if(flagLen == 0) {
            flag = (*decode->src++) << 24;
            flag += (*decode->src++) << 16;
            flag += (*decode->src++) << 8;
            flag += *decode->src++;
            flagLen = 32;
        }
        if(flag >> 31) {
            *decode->dst++ = (s32)*decode->src++;
            decode->size--;
        } else {
            u8 *src;
            u32 dist, len;
            dist = *decode->src++ << 8;
            dist += *decode->src++;
            len = (dist >> 12) & 0xF;
            dist &= 0xFFF;
            src = decode->dst-dist;
            if(len == 0) {
                len = (*decode->src++)+18;
            } else {
                len += 2;
            }
            decode->size -= len;
            while(len) {
                *decode->dst++ = src[-1];
                len--;
                src++;
            }
        }

        flag <<= 1;
        flagLen--;
    }
}

/**
 * @brief Decode the game's run-length encoded data format.
 *
 * @param [in, out] decode Source/destination cursors and remaining output bytes.
 */
static void HuDecodeRle(DECODE *decode)
{
    s32 i;
    while(decode->size) {
        s32 size = *decode->src++;
        if(size < 128) {
            s32 fill = *decode->src++;
            for(i=0; i<size; i++) {
                *decode->dst++ = fill;
            }
        } else {
            size -= 128;
            for(i=0; i<size; i++) {
                *decode->dst++ = *decode->src++;
            }
        }
        decode->size -= size;
    }
}

/**
 * @brief Decode a data buffer using the requested compression format.
 *
 * @param [in] src Encoded input buffer.
 * @param [out] dst Destination buffer with room for `size` decoded bytes.
 * @param [in] size Expected decoded size in bytes; also flushed from cache.
 * @param [in] decodeType One of the DATA_DECODE_* compression identifiers.
 */
void HuDecodeData(void *src, void *dst, u32 size, s32 decodeType)
{
    struct Decode_s decode;
    struct Decode_s *decodeP = &decode;
    decodeP->src = src;
    decodeP->dst = dst;
    decodeP->size = size;
    switch(decodeType) {
        case DATA_DECODE_NONE:
            HuDecodeNone(decodeP);
            break;

        case DATA_DECODE_LZ:
            HuDecodeLz(decodeP);
            break;

        case DATA_DECODE_SLIDE:
            HuDecodeSlide(decodeP);
            break;

        case DATA_DECODE_FSLIDE_ALT:
            HuDecodeFslide(decodeP);
            break;

        case DATA_DECODE_FSLIDE:
            HuDecodeFslide(decodeP);
            break;

        case DATA_DECODE_RLE:
            HuDecodeRle(decodeP);
            break;
            
        default:
            OSReport("decode tyep unknown.(%x)\n", decodeType);
            break;
    }
    DCFlushRange(dst, size);
}

#include "game/fault.h"
#include <stdarg.h>
#include <stdio.h>

typedef struct Xfb_Color_s {
    u8 y;
    u8 cb;
    u8 cr;
} XFB_COLOR;

typedef struct Xfb_Geometry_s {
    void* fb[4];
    u16 width;
    u16 height;
    u16 mode;
} XFB_GEOMETRY;

static XFB_COLOR XFB_Colors[5] = {
    { 0x00, 0x80, 0x80 },
    { 0xFF, 0x80, 0x80 },
    { 0xC0, 0x80, 0x80 },
    { 0x80, 0x80, 0x80 },
    { 0x40, 0x80, 0x80 }
};

#include "Ascii8x8_1bpp.inc"

static XFB_GEOMETRY XFB_Geometry;

static s32 (*XFB_putc)(u8 character, s32 cursor_x, s32 cursor_y);

static XFB_COLOR Draw_Color;

static s32 x_start;
static s32 y_start;

static s32 XFB_putcProgressive(u8 character, s32 cursor_x, s32 cursor_y);
static s32 XFB_putcInterlace(u8 character, s32 cursor_x, s32 cursor_y);
static s32 XFB_puts(s8* message, s32 cursor_x, s32 cursor_y);
static s32 XFB_putcS(u8 character, s32 cursor_x, s32 cursor_y);
static void XFB_WriteBackCache(void);
static void XFB_CR(s32 line_advance, s32* cursor_x, s32* cursor_y);

/**
 * @brief Display a fatal error directly in the framebuffers and halt the CPU.
 *
 * @param [in] file Source file reporting the panic.
 * @param [in] line Source line reporting the panic.
 * @param [in] msg Formatted panic message.
 */
void OSPanic(const char* file, int line, const char* msg, ...) {
    static char* titleMes = "OSPanic encounterd:";
    
    va_list format_args;
    s32 cursor_x;
    s32 cursor_y;
    char message_buf[1024];
    s32 puts;

    cursor_x = x_start = 0x10;
    cursor_y = y_start = 0x20;
    puts = XFB_puts((s8*)titleMes, cursor_x, cursor_y);
    XFB_CR(puts + 1, &cursor_x, &cursor_y);
    sprintf(message_buf, "%s:%d", file, line);
    puts = XFB_puts((s8*)message_buf, cursor_x, cursor_y);
    XFB_CR(puts, &cursor_x, &cursor_y);
    va_start(format_args, msg);
    vsnprintf(message_buf, 0x400U, msg, &format_args[0]);
    puts = XFB_puts((s8*)message_buf, cursor_x, cursor_y);
    XFB_CR(puts, &cursor_x, &cursor_y);
    XFB_WriteBackCache();
    PPCHalt();
    va_end(format_args);
}

/**
 * @brief Initialize direct framebuffer drawing for fatal errors.
 *
 * @param [in] mode Render mode supplying framebuffer dimensions and XFB mode;
 *                  `NULL` leaves geometry zeroed and selects progressive draw.
 */
void HuFaultInitXfbDirectDraw(GXRenderModeObj *mode) {
    s32 i;
    
    for (i = 0; i < 4; i++) {
        XFB_Geometry.fb[i] = 0;
    }
    
    XFB_Geometry.width = 0;
    XFB_Geometry.height = 0;
    XFB_Geometry.mode = 0;
    
    XFB_putc = XFB_putcProgressive;
    Draw_Color = XFB_Colors[1];
    
    if (mode) {
        XFB_Geometry.width = ((u16)mode->fbWidth + 0xF) & 0xFFFFFFF0;
        XFB_Geometry.height = mode->xfbHeight;
        XFB_Geometry.mode = mode->xFBmode;
        
        if (XFB_Geometry.mode == 0) {
            XFB_putc = XFB_putcInterlace;
        } else {
            XFB_putc = XFB_putcProgressive;
        }
    }
}

/**
 * @brief Register a framebuffer for fatal-error drawing.
 *
 * @param [in] id Framebuffer slot in the inclusive range `0` through `3`.
 * @param [in] addr External framebuffer base address, or `NULL` to clear it.
 */
void HuFaultSetXfbAddress(s16 id, void* addr) {
    if (id >= 0 && id < 4) {
        XFB_Geometry.fb[id] = addr;
    }
}

/**
 * @brief Write modified framebuffer ranges back to memory.
 */
static void XFB_WriteBackCache(void) {
    s32 i;
    void* fb;
    u32 size;

    size = XFB_Geometry.width * 2 * XFB_Geometry.height;

    if (size != 0) {
        for (i = 0; i < 4; i += 1) {
            fb = XFB_Geometry.fb[i];

            if (fb) {
                DCStoreRange(fb, size);
            }
        }
    }
}

/**
 * @brief Advance a framebuffer text cursor to the next requested line.
 *
 * @param [in] line_advance Encoded line count; its low three bits add extra
 *                   18-pixel line steps after the mandatory step.
 * @param [in, out] cursor_x Cursor X coordinate, reset to the configured margin.
 * @param [in, out] cursor_y Cursor Y coordinate, advanced in 18-pixel rows.
 */
static void XFB_CR(s32 line_advance, s32* cursor_x, s32* cursor_y) {
    s32 numLines;
    s32 y;
    s32 x;

    x = *cursor_x;
    y = *cursor_y;

    x = x_start;
    y += 0x12;

    numLines = line_advance & 7;
    if (numLines != 0) {
        y += numLines * 0x12;
    }

    *cursor_x = x;
    *cursor_y = y;
}

/**
 * @brief Draw a null-terminated string into the configured framebuffers.
 *
 * @param [in] message Null-terminated text to draw.
 * @param [in] cursor_x Initial horizontal framebuffer coordinate.
 * @param [in] cursor_y Initial vertical framebuffer coordinate.
 *
 * @return Number of newline and automatic-wrap advances encountered before
 *         the terminating null character.
 */
static s32 XFB_puts(s8* message, s32 cursor_x, s32 cursor_y) {
    s32 i;
    s32 wrap_count;
    s8 current_char;
    
    i = 0;
    
    do {
        current_char = *message++;
        
        if (current_char == '\n') {
            XFB_CR(0, &cursor_x, &cursor_y);
            
            i += 1;
        } else {
            wrap_count = XFB_putcS(current_char, cursor_x, cursor_y);
            
            if (wrap_count >= 0) {
                if (wrap_count != 0) {
                    wrap_count -= 1;

                    XFB_CR(wrap_count, &cursor_x, &cursor_y);
                    
                    i += wrap_count + 1;
                }
                cursor_x += 0x10;
            } else {
                break;
            }
        }
    } while(current_char != 0);
    
    return i;
}

/**
 * @brief Draw an outlined character with automatic line wrapping.
 *
 * @param [in] character Character code from the direct-draw font table.
 * @param [in] cursor_x Horizontal framebuffer coordinate, wrapped if needed.
 * @param [in] cursor_y Vertical framebuffer coordinate, advanced on wrapping.
 *
 * @return `0` when drawn without wrapping, `1` when the cursor first wrapped,
 *         or `-1` when `character` is the terminating null character.
 */
static s32 XFB_putcS(u8 character, s32 cursor_x, s32 cursor_y) {
    XFB_COLOR colorOld;
    s32 crNum;

    crNum = 0;
    colorOld = Draw_Color;

    if (cursor_x + 0x11 >= XFB_Geometry.width) {
        XFB_CR(0, &cursor_x, &cursor_y);
        crNum++;
    }

    Draw_Color = XFB_Colors[0];
    XFB_putc(character, cursor_x, cursor_y - 2);
    XFB_putc(character, cursor_x, cursor_y + 2);
    XFB_putc(character, cursor_x - 1, cursor_y);
    XFB_putc(character, cursor_x + 1, cursor_y);

    Draw_Color = colorOld;
    XFB_putc(character, cursor_x, cursor_y);

    return crNum;
}

/**
 * @brief Draw one character into progressive-scan framebuffers.
 *
 * @param [in] character Character code indexing the 8-by-8 font table.
 * @param [in] cursor_x Horizontal framebuffer coordinate.
 * @param [in] cursor_y Vertical framebuffer coordinate.
 *
 * @return `0` after drawing a glyph, `1` after horizontal wrapping, or `-1`
 *         when `character` is zero or the glyph exceeds framebuffer height.
 */
static s32 XFB_putcProgressive(u8 character, s32 cursor_x, s32 cursor_y) {
    s32 result;
    s32 pitch;
    u8 colorY;
    u8 colorCr;
    u8 colorCb;
    s32 i;
    s32 j;
    s32 xfbOfs;
    u8* src;
    s32 k;
    s32 width;
    s32 writeMaskIdx;
    u32 writeMask;
    u32 bits;
    u8* fb;

    result = 0;

    if (character == 0) {
        return -1;
    }

    if (cursor_x + 0x10 >= XFB_Geometry.width) {
        cursor_y += 0x12;
        cursor_x = x_start;
        result = 1;
    }

    if (cursor_y + 0x10 >= XFB_Geometry.height) {
        return -1;
    }

    colorY = Draw_Color.y;
    colorCb = Draw_Color.cb;
    colorCr = Draw_Color.cr;

    pitch = XFB_Geometry.width * 2;
    xfbOfs = (cursor_x & 0xFFFE) * 2 + cursor_y * pitch;
    src = Ascii8x8_1bpp + (character * 8);

    i = 8;

    while (i != 0) {
        j = 2;

        while (j != 0) {
            for (k = 0; k < 4; k ++) {
                fb = XFB_Geometry.fb[k];

                if (fb != 0) {
                    fb += xfbOfs;

                    bits = *src;
                    writeMask = 0;
                    writeMaskIdx = 0;
                    while (writeMaskIdx < 0x10) {
                        if (bits & 0xF != 0) {
                            writeMask |= 3 << writeMaskIdx;
                        }
                        writeMaskIdx += 2;
                        bits >>= 1;
                    }
                    width = 8;
                    if ((s32) (cursor_x & 1) != 0) {
                        writeMask *= 2;
                        width = 0xA;
                    }

                    while (width != 0) {
                        if ((u32) (writeMask & 3) != 0) {
                            fb[1] = colorCr;
                            fb[3] = colorCb;

                            if ((u32) (writeMask & 1) != 0) {
                                fb[0] = colorY;
                            }
                            if ((u32) (writeMask & 2) != 0) {
                                fb[2] = colorY;
                            }
                        }
                        width -= 1;
                        fb += 4;
                        writeMask = writeMask >> 2;
                    }
                }
            }

            j -= 1;
            xfbOfs += pitch;
        }
        i -= 1;
        src += 1;
    }
    
    return result;
}

/**
 * @brief Draw one character into interlaced framebuffers.
 *
 * @param [in] character Character code indexing the 8-by-8 font table.
 * @param [in] cursor_x Horizontal framebuffer coordinate.
 * @param [in] cursor_y Vertical framebuffer coordinate and field selector.
 *
 * @return `0` after drawing a glyph, or `-1` when `character` is zero or the
 *         glyph lies outside framebuffer bounds.
 */
static s32 XFB_putcInterlace(u8 character, s32 cursor_x, s32 cursor_y) {
    u8 colorY;
    u8 colorCr;
    u8 colorCb;
    s32 pitch;
    s32 i;
    s32 xfbOfs;
    u8* src;
    s32 j;
    s16 bits;
    s32 width;
    u8* fb;

    if (character == 0) {
        return -1;
    }

    if (cursor_x + 8 >= XFB_Geometry.width || cursor_y + 8 >= XFB_Geometry.height) {
        return -1;
    }

    colorY = Draw_Color.y;
    colorCb = Draw_Color.cb;
    colorCr = Draw_Color.cr;

    pitch = XFB_Geometry.width * 2;
    xfbOfs = ((cursor_x & 0xFFFE) * 2) + ((cursor_y >> 1) * pitch);
    src = Ascii8x8_1bpp + character * 8;

    i = 8;

    while (i != 0) {
        for (j = 0; j < 4; j += 2) {
            width = j;

            if ((s32) (cursor_y & 1) != 0) {
                width += 1;
            }

            fb = XFB_Geometry.fb[width];

            if (fb) {
                fb = fb + xfbOfs;
                bits = *src;
                width = 4;

                if (cursor_x & 1) {
                    bits = (s16)bits * 2;
                    width = 5;
                }

                while (width) {
                    if (bits & 3) {
                        fb[1] = colorCr;
                        fb[3] = colorCb;

                        if (bits & 1) {
                            fb[0] = colorY;
                        }
                        if (bits & 2) {
                            fb[2] = colorY;
                        }
                    }

                    width -= 1;
                    fb += 4;
                    bits >>= 2;
                }
            }
        }

        i -= 1;
        cursor_y += 1;
        src += 1;
        xfbOfs += pitch;
    }
    
    return 0;
}

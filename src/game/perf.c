#include "game/perf.h"

typedef struct {
    /* 0x00 */ u8 red;
    /* 0x01 */ u8 green;
    /* 0x02 */ u8 blue;
    /* 0x03 */ u8 alpha;
    /* 0x04 */ char padding04[4];
    /* 0x08 */ s64 elapsed_ticks;
    /* 0x10 */ s64 start_ticks;
    /* 0x18 */ OSStopwatch stopwatch;
    /* 0x50 */ s16 active;
    /* 0x52 */ char padding52[6];
} UnknownPerfStruct; // Size 0x58

static void DSCallbackFunc(u16 draw_sync_token);

static OSStopwatch Ssw;
static UnknownPerfStruct perf[10];

static u32 met0;
static u32 met1;
static u32 vcheck;
static u32 vmiss;
static u32 vstall;
static u32 cp_req;
static u32 tc_req;
static u32 cpu_rd_req;
static u32 cpu_wr_req;
static u32 dsp_req;
static u32 io_req;
static u32 vi_req;
static u32 pe_req;
static u32 rf_req;
static u32 fi_req;
static u32 top_pixels_in;
static u32 top_pixels_out;
static u32 bot_pixels_in;
static u32 bot_pixels_out;
static u32 clr_pixels_in;
static u32 total_copy_clks;
static s16 tokenEndF;
static u8 metf;

/**
 * @brief Initializes the performance timers and GPU draw-sync measurement.
 */
void HuPerfInit(void) {
    s32 i;

    for (i = 0; i < 10; i++) {
        perf[i].active = 0;
    }
    HuPerfCreate("CPU", 0, 0xFF, 0, 0xFF);
    HuPerfCreate("DRAW", 0xFF, 0, 0, 0xFF);
    GXSetDrawSyncCallback(DSCallbackFunc);
    total_copy_clks = 0;
}

/**
 * @brief Creates a named performance timer with an associated display color.
 *
 * @param name Null-terminated timer label passed to OSInitStopwatch.
 * @param red Red display-color component in the inclusive range 0..255.
 * @param green Green display-color component in the inclusive range 0..255.
 * @param blue Blue display-color component in the inclusive range 0..255.
 * @param alpha Alpha display-color component in the inclusive range 0..255.
 * @return The timer slot index, or -1 if all slots are occupied.
 */
s32 HuPerfCreate(char *name, u8 red, u8 green, u8 blue, u8 alpha) {
    UnknownPerfStruct *perf_entry;
    s32 i;

    for (i = 0; i < 10; i++) {
        if (perf[i].active == 0) {
            break;
        }
    }
    if (i == 10) {
        return -1;
    }
    OSInitStopwatch(&perf[i].stopwatch, name);
    perf[i].elapsed_ticks = 0;
    perf[i].active = 1;
    perf[i].red = red;
    perf[i].green = green;
    perf[i].blue = blue;
    perf[i].alpha = alpha;
    return i;
}

/**
 * @brief Resets and starts the shared reference stopwatch.
 */
void HuPerfZero(void) {
    OSStopStopwatch(&Ssw);
    OSResetStopwatch(&Ssw);
    OSStartStopwatch(&Ssw);
}

/**
 * @brief Starts a performance timer.
 *
 * @details Slot 1 is measured through GX draw-sync tokens so it brackets GPU
 * work rather than the calling CPU interval.
 *
 * @param timer_slot Index returned by HuPerfCreate; slot 1 emits the DRAW start token.
 */
void HuPerfBegin(s32 timer_slot) {
    UnknownPerfStruct *perf_entry;

    if (timer_slot == 1) {
        GXSetDrawSync(0xFF00);
        return;
    }
    OSStartStopwatch(&perf[timer_slot].stopwatch);
    perf[timer_slot].start_ticks = OSCheckStopwatch(&Ssw);
}

/**
 * @brief Stops a performance timer and records its elapsed tick count.
 *
 * @param timer_slot Index returned by HuPerfCreate; slot 1 emits the DRAW end token.
 */
void HuPerfEnd(s32 timer_slot) {
    UnknownPerfStruct *perf_entry;

    if (timer_slot == 1) {
        GXSetDrawSync(0xFF01);
        return;
    }
    perf[timer_slot].elapsed_ticks = OSCheckStopwatch(&perf[timer_slot].stopwatch);
    OSStopStopwatch(&perf[timer_slot].stopwatch);
    OSResetStopwatch(&perf[timer_slot].stopwatch);
}

/**
 * @brief Starts or ends the DRAW stopwatch when its GX token reaches the FIFO.
 *
 * @param draw_sync_token GX token: 0xFF00 starts DRAW timing and 0xFF01 stops it.
 */
static void DSCallbackFunc(u16 draw_sync_token) {
    switch (draw_sync_token) {
        case 0xFF00:
            OSStartStopwatch(&perf[1].stopwatch);
            perf[1].start_ticks = OSCheckStopwatch(&Ssw);
            tokenEndF = 0;
            if (metf == 1) {
                GXClearGPMetric();
                GXClearVCacheMetric();
                GXClearMemMetric();
                GXClearPixMetric();
            }
            break;
        case 0xFF01:
            if (tokenEndF == 0) {
                tokenEndF = 1;
                perf[1].elapsed_ticks = OSCheckStopwatch(&perf[1].stopwatch);
                OSStopStopwatch(&perf[1].stopwatch);
                OSResetStopwatch(&perf[1].stopwatch);
                if (metf == 1) {
                    GXReadGPMetric(&met0, &met1);
                    GXReadVCacheMetric(&vcheck, &vmiss, &vstall);
                    GXReadMemMetric(&cp_req, &tc_req, &cpu_rd_req, &cpu_wr_req, &dsp_req, &io_req, &vi_req, &pe_req, &rf_req, &fi_req);
                    GXReadPixMetric(&top_pixels_in, &top_pixels_out, &bot_pixels_in, &bot_pixels_out, &clr_pixels_in, &total_copy_clks);
                }
            }
            break;
    }
}

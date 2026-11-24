#include "bgm.h"

#include <3ds.h>
#include <string.h>

extern const u8 bgm_bin[];
extern const u8 bgm_bin_end[];

#define BGM_RATE 16000

static u8 *g_pcm;
static u32 g_play;
static u32 g_rate = BGM_RATE;
static bool g_on;
static bool g_csnd;
static int g_chn = -1;
static aptHookCookie g_hook;
static bool g_hooked;
static u32 g_pos;
static u64 g_tick;
static u32 g_tick_pos;
static bool g_tail;
static int g_tail_wait;
static volatile int g_wake;

static int owned_channel(void) {

    for (int ch = 0; ch < 32; ch++) {
        if (csndChannels & BIT(ch)) return ch;
    }
    return -1;
}

static void drop_csnd(void) {
    if (!g_csnd) return;
    csndExit();
    g_csnd = false;
    g_chn = -1;
}

static u32 estimate_pos(void) {
    u64 samples;
    if (!g_play || !g_rate) return 0;
    samples = ((svcGetSystemTick() - g_tick) * (u64)g_rate) / (u64)SYSCLOCK_ARM11;
    return (u32)(((u64)g_tick_pos + samples) % g_play) & ~3u;
}

static u32 hardware_pos(void) {
    u32 base;
    u32 cur;
    CSND_ChnInfo info;
    if (!g_csnd || g_chn < 0 || !g_pcm || g_play < 64) return 0xFFFFFFFFu;
    base = osConvertVirtToPhys(g_pcm);
    if (!base) return 0xFFFFFFFFu;
    memset(&info, 0, sizeof(info));
    if (R_FAILED(csndGetState((u32)g_chn, &info))) return 0xFFFFFFFFu;
    cur = info.value[2];
    if (cur < base || cur >= base + g_play) return 0xFFFFFFFFu;
    return (cur - base) & ~3u;
}

static void mark_pos(u32 pos) {
    if (pos >= g_play) pos = 0;
    g_pos = pos & ~3u;
    g_tick = svcGetSystemTick();
    g_tick_pos = g_pos;
}

static void remember(void) {
    u32 hw = hardware_pos();
    u32 pos = hw == 0xFFFFFFFFu ? estimate_pos() : hw;
    if (pos >= g_play) pos = 0;
    g_pos = pos & ~3u;
}

static bool play_from(u32 pos) {
    int ch;
    Result rc;
    u32 flags;
    u32 bytes;
    void *start;
    bool tail;
    if (!g_on || !g_pcm || g_play < 64) return false;
    if (pos >= g_play || g_play - pos < 64) pos = 0;
    if (!g_csnd) {
        if (R_FAILED(csndInit())) return false;
        g_csnd = true;
    }
    ch = owned_channel();
    if (ch < 0) {
        drop_csnd();
        return false;
    }
    g_chn = ch;
    tail = pos != 0;
    bytes = g_play - pos;
    start = g_pcm + pos;
    flags = SOUND_FORMAT_8BIT | (tail ? SOUND_ONE_SHOT : SOUND_REPEAT);
    GSPGPU_FlushDataCache(g_pcm, g_play);
    rc = csndPlaySound(ch, flags, g_rate, 1.0f, 0.0f, start, tail ? start : g_pcm, bytes);
    if (rc != 0) {
        drop_csnd();
        return false;
    }
    CSND_SetVol(ch, 0x80008000u, 0);
    csndExecCmds(true);
    g_tail = tail;
    g_tail_wait = tail ? 8 : 0;
    mark_pos(pos);
    return true;
}

static void apt_event(APT_HookType hook, void *param) {
    (void)param;
    if (hook == APTHOOK_ONSUSPEND || hook == APTHOOK_ONSLEEP || hook == APTHOOK_ONEXIT) {
        remember();
        g_wake = 45;
    } else if (hook == APTHOOK_ONRESTORE || hook == APTHOOK_ONWAKEUP) {
        g_wake = 45;
    }
}

void bgm_start(void) {
    u32 size = (u32)(bgm_bin_end - bgm_bin);
    if (size < 64) return;
    if (R_FAILED(csndInit())) return;
    g_csnd = true;

    int ch = owned_channel();
    if (ch < 0) {
        drop_csnd();
        return;
    }

    u32 step = 1;
    u32 freeb = linearSpaceFree();
    while (step < 8 && size / step + 0x2000 > freeb) step++;
    u32 play = size / step;
    play &= ~3u;
    if (play < 64) {
        drop_csnd();
        return;
    }

    g_pcm = linearAlloc(play);
    if (!g_pcm) {
        drop_csnd();
        return;
    }
    if (step == 1) {
        memcpy(g_pcm, bgm_bin, play);
    } else {
        for (u32 i = 0; i < play; i++) g_pcm[i] = bgm_bin[i * step];
    }
    GSPGPU_FlushDataCache(g_pcm, play);

    g_rate = BGM_RATE / step;
    g_play = play;
    g_chn = ch;
    g_on = true;
    if (!play_from(0)) {
        linearFree(g_pcm);
        g_pcm = NULL;
        g_play = 0;
        g_on = false;
        drop_csnd();
        return;
    }
    if (!g_hooked) {
        aptHook(&g_hook, apt_event, NULL);
        g_hooked = true;
    }
    g_on = true;
}

void bgm_poll(void) {
    if (!g_on) return;
    if (g_wake > 0) {
        g_wake--;
        drop_csnd();
        if (play_from(g_pos)) g_wake = 0;
        return;
    }
    if (!g_csnd) {
        play_from(g_pos);
        return;
    }
    if (g_tail) {
        u8 playing = 1;
        if (g_tail_wait > 0) {
            g_tail_wait--;
            return;
        }
        if (R_SUCCEEDED(csndIsPlaying((u32)g_chn, &playing)) && !playing) play_from(0);
    }
}

void bgm_stop(void) {
    if (!g_on && !g_pcm && g_chn < 0) return;
    if (g_hooked) {
        aptUnhook(&g_hook);
        g_hooked = false;
    }

    if (g_csnd) {
        for (int ch = 0; ch < 32; ch++) {
            if (csndChannels & BIT(ch)) {
                CSND_SetVol(ch, 0, 0);
                CSND_SetPlayState(ch, 0);
            }
        }
        csndExecCmds(true);
        if (g_pcm && g_play) {
            memset(g_pcm, 0, g_play);
            GSPGPU_FlushDataCache(g_pcm, g_play);
        }
        svcSleepThread(20000000);
        drop_csnd();
    }
    if (g_pcm) linearFree(g_pcm);
    g_pcm = NULL;
    g_play = 0;
    g_chn = -1;
    g_on = false;
}

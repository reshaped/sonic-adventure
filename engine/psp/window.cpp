/* the PSP in place of the SDL3 window (engine/pc/window.h): its screen, which
 * raster.cpp draws to, its buttons as the game's pad, and its sound, which a
 * thread of its own asks the backend for a piece at a time. the PSP has one
 * stick, no triggers' travel and nothing that shakes.
 *
 * where a run has made the folder sound beside the program, the first
 * KEPT_PIECES pieces of its sound stay there in sound.raw, as the output
 * takes them: 44100 frames a second of two int16_t */
#include <pspaudio.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include <pspkernel.h>
#include <stdio.h>

#include "draw.h"
#include "sa_pc.h"
#include "window.h"

namespace pc {

struct Window {
    SoundRender render = nullptr;
    void *user = nullptr;
    SceUID thread = -1;
    int channel = -1;
    volatile bool playing = false;
    uint32_t held = 0;
    /* the pieces rendered, and the microseconds their render took */
    uint32_t pieces = 0;
    uint64_t render_us = 0;
};

namespace {

/* Shinobi's PDD_DGT bits, as the game reads a pad */
enum {
    PAD_B = 1 << 1,
    PAD_A = 1 << 2,
    PAD_START = 1 << 3,
    PAD_UP = 1 << 4,
    PAD_DOWN = 1 << 5,
    PAD_LEFT = 1 << 6,
    PAD_RIGHT = 1 << 7,
    PAD_Y = 1 << 9,
    PAD_X = 1 << 10,
    PAD_TR = 1 << 16,
    PAD_TL = 1 << 17
};
/* the PSP's buttons by the place the Dreamcast's pad has each: cross is A
 * and circle B, as each pad's lower and right buttons are */
const struct {
    uint32_t button, bit;
} BUTTONS[] = {
    {PSP_CTRL_CROSS, PAD_A},     {PSP_CTRL_CIRCLE, PAD_B},
    {PSP_CTRL_SQUARE, PAD_X},    {PSP_CTRL_TRIANGLE, PAD_Y},
    {PSP_CTRL_START, PAD_START}, {PSP_CTRL_UP, PAD_UP},
    {PSP_CTRL_DOWN, PAD_DOWN},   {PSP_CTRL_LEFT, PAD_LEFT},
    {PSP_CTRL_RIGHT, PAD_RIGHT}, {PSP_CTRL_LTRIGGER, PAD_TL},
    {PSP_CTRL_RTRIGGER, PAD_TR},
};
/* a stick this near its middle rests: the PSP's never comes back to it
 * exactly */
constexpr int DEAD = 24;
/* the frames a piece of sound holds, which the PSP takes 64 at a time */
constexpr int PIECE = 1024;
/* a minute of pieces */
constexpr uint32_t KEPT_PIECES = 60 * 44100 / PIECE;

Window the_window;

int play(SceSize, void *)
{
    static int16_t piece[PIECE * 2] __attribute__((aligned(64)));

    /* the FPU as the engine's C++ has it elsewhere: to nearest, no
     * exception enabled, and a result too small for a normal number
     * flushed to zero. the word is engine/native/native.h's
     * NATIVE_FCR31_HOST */
    uint32_t host = 1U << 24;

    __asm__ volatile("ctc1 %0, $31" : : "r"(host));
    FILE *kept = fopen("sound/sound.raw", "wb");

    while (the_window.playing) {
        uint32_t began = sceKernelGetSystemTimeLow();

        the_window.render(the_window.user, piece, PIECE);
        the_window.render_us += sceKernelGetSystemTimeLow() - began;
        if (kept != nullptr && the_window.pieces < KEPT_PIECES) {
            fwrite(piece, 1, sizeof piece, kept);
        }
        the_window.pieces++;
        sceAudioOutputBlocking(the_window.channel, PSP_AUDIO_VOLUME_MAX, piece);
    }
    if (kept != nullptr) {
        fclose(kept);
    }
    return 0;
}

} // namespace

Window *window_open(const char *, uint32_t, uint32_t, uint32_t, const char *,
                    const Image *, std::string &)
{
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    return &the_window;
}

void window_close(Window *window)
{
    window_sound_close(window);
}

void window_alert(const char *title, const char *words)
{
    pspDebugScreenInit();
    pspDebugScreenPrintf("%s\n\n%s\n", title, words);
}

/* the PSP has no Vulkan, so no surface of it */
const char *const *window_extensions(uint32_t &count)
{
    count = 0;
    return nullptr;
}

bool window_surface(Window *, void *, uint64_t &)
{
    return false;
}

void window_size(Window *, uint32_t &width, uint32_t &height)
{
    width = 480;
    height = 272;
}

/* the PSP's own menu ends the program (main.c) */
bool window_pump(Window *)
{
    return false;
}

sa_result window_pad(Window *window, uint32_t port, sa_pad &pad)
{
    SceCtrlData now;
    uint32_t held = 0;
    int x, y;

    if (window == nullptr || port != 0) {
        return SA_EMPTY;
    }
    sceCtrlPeekBufferPositive(&now, 1);
    for (const auto &b : BUTTONS) {
        held |= now.Buttons & b.button ? b.bit : 0;
    }
    pad = sa_pad{};
    pad.head.kind = SA_REC_PAD;
    pad.head.size = sizeof pad;
    pad.present = 1;
    pad.on = held;
    pad.press = held & ~window->held;
    pad.release = window->held & ~held;
    window->held = held;
    x = static_cast<int>(now.Lx) - 128;
    y = static_cast<int>(now.Ly) - 128;
    pad.x1 = static_cast<int16_t>(x > -DEAD && x < DEAD ? 0 : x);
    pad.y1 = static_cast<int16_t>(y > -DEAD && y < DEAD ? 0 : y);
    /* the directions push the stick whole where it rests, as a pad's do */
    if (pad.x1 == 0 && (held & (PAD_LEFT | PAD_RIGHT)) != 0) {
        pad.x1 = held & PAD_LEFT ? -128 : 127;
    }
    if (pad.y1 == 0 && (held & (PAD_UP | PAD_DOWN)) != 0) {
        pad.y1 = held & PAD_UP ? -128 : 127;
    }
    pad.l = held & PAD_TL ? 255 : 0;
    pad.r = held & PAD_TR ? 255 : 0;
    return SA_OK;
}

sa_result window_rumble(Window *, uint32_t, uint16_t, uint16_t, uint32_t,
                        uint32_t)
{
    return SA_EMPTY;
}

bool window_sound_open(Window *window, SoundRender render, void *user,
                       std::string &why)
{
    if (window == nullptr || window->playing) {
        why = "the PSP's sound is open already, or it has no window";
        return false;
    }
    window->channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, PIECE,
                                        PSP_AUDIO_FORMAT_STEREO);
    if (window->channel < 0) {
        why = "sceAudioChReserve gave no channel";
        return false;
    }
    window->render = render;
    window->user = user;
    window->playing = true;
    /* above the game's thread, which starts at 0x20: the render takes a
     * small part of the time it fills, and waits out the rest in the
     * output's call, so the sound has each piece in time and the game the
     * processor between */
    window->thread = sceKernelCreateThread("sound", play, 0x1c, 0x8000,
                                           PSP_THREAD_ATTR_USER, nullptr);
    if (window->thread < 0 ||
        sceKernelStartThread(window->thread, 0, nullptr) < 0) {
        window->playing = false;
        sceAudioChRelease(window->channel);
        why = "the PSP started no thread for the sound";
        return false;
    }
    return true;
}

void window_sound_close(Window *window)
{
    if (window == nullptr || !window->playing) {
        return;
    }
    window->playing = false;
    sceKernelWaitThreadEnd(window->thread, nullptr);
    sceKernelDeleteThread(window->thread);
    sceAudioChRelease(window->channel);
}

} // namespace pc

extern "C" void psp_sound_report(void)
{
    const pc::Window &w = pc::the_window;

    if (w.pieces != 0) {
        fprintf(stderr,
                "sa1: the sound's render took %u ms for %u ms of sound\n",
                static_cast<unsigned>(w.render_us / 1000),
                static_cast<unsigned>(static_cast<uint64_t>(w.pieces) *
                                      pc::PIECE * 1000 / 44100));
    }
}

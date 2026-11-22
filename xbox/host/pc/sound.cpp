/* the PC host's sound: the console's AICA, by Flycast's emulation of it
 * (xbox/third_party/flycast, GPL-2.0), behind host_api's aica calls.
 *
 * the game's sound library, Shinobi's sd and G2 sources, runs as the
 * console's code. it writes the AICA's registers through host_api, and its
 * sound RAM, 2 MB, as memory this host shares with the AICA at 0xa0800000.
 * there it loads the disc's driver, masaru.drv, which the AICA's ARM7 runs
 * once the library releases the ARM's reset.
 *
 * a thread of the host's runs the AICA: at each vertical blank, the samples
 * of 44.1 kHz the blank's 16.683 ms take, about 735.7, each a step of the
 * ARM7's 22.58 MHz and of the AICA's 64 channels, its DSP and its timers. a
 * lock keeps the game's register accesses between the thread's runs. the
 * samples go to SDL's audio device, and under HOST_WAV=path to a WAV file as
 * well */
#include <SDL2/SDL.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <ctime>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "hw/aica/aica_if.h"
#include "hw/arm7/arm7.h"
#include "cfg/option.h"

#include "sound.h"

/* Flycast's options and settings, as the port has them */
namespace config {
/* the DSP, the effects, is off, as on the Xbox, whose APU has no reverb
 * yet: on, Red Mountain's reverb ran to full scale within 20 ms and stayed
 * there, with its ring, program and coefficients as the driver set them, by
 * the recompiler and the interpreter alike */
Value<bool> DSPEnabled{false};
Value<bool> GGPOEnable{false};
Value<bool> VmuSound{false};
}
flycast_settings settings;

/* Flycast's log, which the port leaves silent */
void flycast_log(const char *kind, const char *format, ...)
{
    (void)kind;
    (void)format;
}

void flycast_die(const char *file, int line, const char *what)
{
    fprintf(stderr, "aica: %s:%d: %s\n", file, line, what);
    abort();
}

/* the AICA's own timestamp counts, which arm7.cpp and sgc_if.cpp keep */
namespace aica {
namespace sgc {
unsigned long long profile_dsp, profile_channels;
}
extern unsigned long long profile_timers, profile_sample, profile_interrupts;
namespace arm {
extern unsigned long long profile_arm, profile_step;
/* the ARM7's PC at each sample, by its word in the first 128 KB */
unsigned profile_pcs[0x8000];
}
}

/* the RTC, which the port's sg_syrtc takes from the host's date, and which
 * Flycast's aica::init starts */
namespace aica {
void initRtc() {}
void resetRtc(bool) {}
void termRtc() {}
}

/* the disc's masaru.drv's idle loop, which the ARM7 spends most of its time
 * in: at 0x194 it polls the queue at 0xb000, whose write index is at
 * 0x13000 and read index at the address 0x1818 holds, then two flags its
 * interrupt handlers set, at 0x22e and 0x22c, and when all are empty
 * branches back to 0x194. idle_wait says whether they are, for arm7.cpp to
 * pass the rest of the sample. the driver's code at those places is checked
 * first, so that another driver runs as it is */
static const u8 *sound_ram;

static u32 ram32(u32 at)
{
    return *(const u32 *)(sound_ram + (at & (ARAM_SIZE - 4)));
}

static bool masaru_idle(void)
{
    return sound_ram[0x22e] == 0 && sound_ram[0x22c] == 0 && ram32(0x13000) == ram32(ram32(0x1818));
}

static bool is_masaru(void)
{
    return ram32(0x194) == 0xeb000202u && ram32(0x1b4) == 0xeafffff6u && ram32(0x1c0) == 0x0afffff3u &&
           ram32(0x1568) == 0xe3a03a13u && ram32(0x1570) == 0xe59f32a0u && ram32(0x198) == 0xe5df908eu &&
           ram32(0x1b8) == 0xe5df906cu;
}

/* the hook, when the driver is masaru.drv, as it is once the ARM starts */
static bool idle_wait_checked(void)
{
    static bool checked, known;

    if (!checked) {
        checked = true;
        known = is_masaru();
        fprintf(stderr, "host: the sound driver %s masaru.drv's idle loop\n", known ? "has" : "hasn't");
    }
    return known && masaru_idle();
}

namespace {

std::mutex lock;
sem_t blanks;
std::atomic<bool> quitting;
SDL_AudioDeviceID device;
FILE *wav;
unsigned wav_samples;
std::vector<int16_t> mixed;

/* HOST_SOUND_RECORD=path: what the game does to the AICA, for
 * xbox/host/pc/sndreplay.cpp to run again without the game, against any
 * build of the sound's core: a file of events, each a letter and 32-bit
 * words. W offset length, and the bytes: the game's writes to sound RAM
 * since the AICA last ran, found against a copy taken after that run, which
 * holds the ARM7's own. S offset size value: a register write. R offset
 * size value: a register read, and what it gave. N count: the AICA runs
 * count samples */
FILE *record;
std::vector<u8> record_ram;

void record_words(char kind, unsigned a, unsigned b, unsigned c)
{
    unsigned words[3] = {a, b, c};

    fputc(kind, record);
    fwrite(words, 4, kind == 'N' ? 1 : kind == 'W' ? 2 : 3, record);
}

void record_ram_writes(void)
{
    const u8 *ram = &aica::aica_ram[0];
    u32 at = 0;

    while (at < ARAM_SIZE) {
        u32 end;
        u32 same = 0;

        if ((at & 255) == 0 && memcmp(ram + at, record_ram.data() + at, 256) == 0) {
            at += 256;
            continue;
        }
        if (ram[at] == record_ram[at]) {
            at++;
            continue;
        }
        /* to the last changed byte before 64 that match */
        for (end = at; end < ARAM_SIZE && same < 64; end++) {
            same = ram[end] == record_ram[end] ? same + 1 : 0;
        }
        end -= same;
        record_words('W', at, end - at, 0);
        fwrite(ram + at, 1, end - at, record);
        at = end;
    }
}

/* 44100 samples a second, of the blank's 16683333 ns */
constexpr long long BLANK_NS = 16683333;
long long owed_ns_44100;

void wav_header(void)
{
    unsigned bytes = wav_samples * 4;
    unsigned char h[44];
    auto put32 = [&](int at, unsigned v) { for (int k = 0; k < 4; k++) h[at + k] = (unsigned char)(v >> (8 * k)); };
    auto put16 = [&](int at, unsigned v) { h[at] = (unsigned char)v; h[at + 1] = (unsigned char)(v >> 8); };

    memcpy(h, "RIFF", 4);
    put32(4, 36 + bytes);
    memcpy(h + 8, "WAVEfmt ", 8);
    put32(16, 16);
    put16(20, 1);
    put16(22, 2);
    put32(24, 44100);
    put32(28, 44100 * 4);
    put16(32, 4);
    put16(34, 16);
    memcpy(h + 36, "data", 4);
    put32(40, bytes);
    fseek(wav, 0, SEEK_SET);
    fwrite(h, 1, sizeof h, wav);
    fseek(wav, 0, SEEK_END);
    fflush(wav);
}

void run(void)
{
    for (;;) {
        sem_wait(&blanks);
        if (quitting) {
            if (record != nullptr) {
                fclose(record);
            }
            if (wav != nullptr) {
                wav_header();
                fclose(wav);
            }
            fprintf(stderr, "host: quit with %u samples of sound\n", wav_samples);
            _exit(0);
        }
        owed_ns_44100 += BLANK_NS * 44100;
        unsigned count = (unsigned)(owed_ns_44100 / 1000000000);

        owed_ns_44100 -= (long long)count * 1000000000;
        {
            std::lock_guard<std::mutex> hold(lock);

            mixed.clear();
            if (record != nullptr) {
                record_ram_writes();
                record_words('N', count, 0, 0);
            }
            aica::arm::run(count);
            if (record != nullptr) {
                memcpy(record_ram.data(), &aica::aica_ram[0], ARAM_SIZE);
            }
        }
        if (device != 0) {
            /* a quarter second queued at most: a slow PC drops, not lags */
            if (SDL_GetQueuedAudioSize(device) < 44100 / 4 * 4) {
                SDL_QueueAudio(device, mixed.data(), (Uint32)(mixed.size() * sizeof(int16_t)));
            }
        }
        if (wav != nullptr) {
            fwrite(mixed.data(), sizeof(int16_t), mixed.size(), wav);
            wav_samples += (unsigned)(mixed.size() / 2);
            if (wav_samples % 44100 < count) {
                wav_header();
            }
        }
    }
}

}

/* each sample the AICA mixes */
void WriteSample(s16 right, s16 left)
{
    mixed.push_back(left);
    mixed.push_back(right);
}

extern "C" int sound_open(void *ram)
{
    aica::aica_ram.setRegion((u8 *)ram, ARAM_SIZE);
    sound_ram = (const u8 *)ram;
    aica::arm::idle_pc = 0x194;
    aica::arm::idle_wait = idle_wait_checked;
    aica::init();
    aica::reset(true);
    sem_init(&blanks, 0, 0);
    if (getenv("HOST_SOUND_RECORD") != nullptr) {
        record = fopen(getenv("HOST_SOUND_RECORD"), "wb");
        record_ram.assign((const u8 *)ram, (const u8 *)ram + ARAM_SIZE);
    }
    if (getenv("HOST_WAV") != nullptr) {
        wav = fopen(getenv("HOST_WAV"), "wb+");
        if (wav != nullptr) {
            wav_header();
        }
    }
    if (getenv("HOST_HEADLESS") == nullptr && SDL_InitSubSystem(SDL_INIT_AUDIO) == 0) {
        SDL_AudioSpec want = {};

        want.freq = 44100;
        want.format = AUDIO_S16LSB;
        want.channels = 2;
        want.samples = 1024;
        device = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);
        if (device != 0) {
            SDL_PauseAudioDevice(device, 0);
        } else {
            fprintf(stderr, "host: no audio device: %s\n", SDL_GetError());
        }
    }
    std::thread(run).detach();
    return 0;
}

/* from the blank's signal: sem_post is safe there */
extern "C" void sound_blank(void)
{
    sem_post(&blanks);
}

extern "C" void sound_quit(void)
{
    quitting = true;
    sem_post(&blanks);
}

extern "C" unsigned sound_reg_read(unsigned offset, int size)
{
    std::lock_guard<std::mutex> hold(lock);

    unsigned value;

    if (offset >= 0x8000) {
        return 0;
    }
    switch (size) {
    case 1:
        value = aica::readAicaReg<u8>(offset);
        break;
    case 2:
        value = aica::readAicaReg<u16>(offset);
        break;
    default:
        value = aica::readAicaReg<u32>(offset);
        break;
    }
    if (record != nullptr) {
        record_words('R', offset, (unsigned)size, value);
    }
    return value;
}

extern "C" void sound_reg_write(unsigned offset, unsigned value, int size)
{
    std::lock_guard<std::mutex> hold(lock);

    if (offset >= 0x8000) {
        return;
    }
    if (record != nullptr) {
        record_words('S', offset, (unsigned)size, value);
    }
    switch (size) {
    case 1:
        aica::writeAicaReg<u8>(offset, (u8)value);
        break;
    case 2:
        aica::writeAicaReg<u16>(offset, (u16)value);
        break;
    default:
        aica::writeAicaReg<u32>(offset, value);
        break;
    }
}

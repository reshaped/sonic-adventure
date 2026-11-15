/* the Xbox host's AICA: Flycast's ARM7, registers, timers and channel model
 * (xbox/third_party/flycast, GPL-2.0, built with FLYCAST_APU), behind
 * host_api's aica calls, as xbox/host/pc/sound.cpp has them on a PC, but
 * without its mixer: the channels play on the APU's voices (aica_apu.c), and
 * the model goes on without samples, for what the sound driver reads back.
 * no thread: the host runs the blank's samples at each blank, on the game's
 * thread, and the ARM7 skips masaru.drv's idle loop as on the PC. the DSP
 * isn't run (the APU has no reverb here yet) */
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "hw/aica/aica_if.h"
#include "hw/arm7/arm7.h"
#include "cfg/option.h"

#include "aica_apu.h"

extern "C" void host_log_line(const char *format, ...);
extern "C" unsigned long long host_ticks(void);

/* what the sound takes, in host_ticks, for host.c's profile */
extern "C" unsigned long long xbox_sound_ticks_arm, xbox_sound_ticks_apu;
unsigned long long xbox_sound_ticks_arm, xbox_sound_ticks_apu;

namespace config {
Value<bool> DSPEnabled{false};
Value<bool> GGPOEnable{false};
Value<bool> VmuSound{false};
}
flycast_settings settings;

void flycast_log(const char *kind, const char *format, ...)
{
    (void)kind;
    (void)format;
}

void flycast_die(const char *file, int line, const char *what)
{
    host_log_line("aica: %s:%d: %s\n", file, line, what);
    for (;;) {
    }
}

namespace aica {
void initRtc() {}
void resetRtc(bool) {}
void termRtc() {}
}

/* Flycast's functions at load, which xbox_sound_open runs (stdclass.h) */
static void (*onloads[8])();
static int onload_count;

void flycast_defer_onload(void (*f)())
{
    if (onload_count < 8) {
        onloads[onload_count++] = f;
    }
}

void flycast_run_onloads()
{
    for (int k = 0; k < onload_count; k++) {
        onloads[k]();
    }
    onload_count = 0;
}

/* Flycast's own counts of the sound's time, in the CPU's timestamp counts
 * (FLYCAST_PROFILE): the ARM7's, and the sample step's, of the timers, the
 * channels and the interrupts, which xbox_sound_report gives as shares */
namespace aica {
namespace sgc {
unsigned long long profile_dsp, profile_channels;
}
extern unsigned long long profile_timers, profile_sample, profile_interrupts;
namespace arm {
extern unsigned long long profile_arm, profile_step;
extern unsigned staticMissed;
unsigned profile_pcs[0x8000];
}
}

#include "aica_channel.h"

extern "C" void xbox_sound_report(void)
{
    unsigned long long arm = aica::arm::profile_arm, timers = aica::profile_timers;
    unsigned long long channels = aica::profile_sample, interrupts = aica::profile_interrupts;
    unsigned long long all = arm + timers + channels + interrupts;

    if (all != 0) {
        host_log_line("host: sound: of it, the ARM7 %u%%, the channels %u%%, the timers %u%%, the interrupts %u%%\n",
                      (unsigned)(arm * 100 / all), (unsigned)(channels * 100 / all), (unsigned)(timers * 100 / all),
                      (unsigned)(interrupts * 100 / all));
    }
    {
        unsigned counts[3];
        static unsigned last[3], last_missed;

        aica_quiet_counts(counts);
        host_log_line("host: sound: the channels' models passed %u samples in %u runs and %u one at a time; the ARM7 "
                      "interpreted %u instructions the translation lacks\n",
                      counts[2] - last[2], counts[1] - last[1], counts[0] - last[0],
                      aica::arm::staticMissed - last_missed);
        for (int k = 0; k < 3; k++) {
            last[k] = counts[k];
        }
        last_missed = aica::arm::staticMissed;
    }
    aica::arm::profile_arm = aica::arm::profile_step = 0;
    aica::profile_timers = aica::profile_sample = aica::profile_interrupts = 0;
    aica::sgc::profile_channels = aica::sgc::profile_dsp = 0;
}

/* the mixer's samples, which the APU makes instead */
void WriteSample(s16 right, s16 left)
{
    (void)right;
    (void)left;
}

/* masaru.drv's idle loop, as xbox/host/pc/sound.cpp's head says */
static const u8 *sound_ram;

static u32 ram32(u32 at)
{
    return *(const u32 *)(sound_ram + (at & (ARAM_SIZE - 4)));
}

static bool idle_wait(void)
{
    static bool known;

    /* checked until found: the ARM7 may reach 0x194 before the driver is
     * all there */
    if (!known) {
        known = ram32(0x194) == 0xeb000202u && ram32(0x1b4) == 0xeafffff6u && ram32(0x1c0) == 0x0afffff3u &&
                ram32(0x1568) == 0xe3a03a13u && ram32(0x1570) == 0xe59f32a0u && ram32(0x198) == 0xe5df908eu &&
                ram32(0x1b8) == 0xe5df906cu;
        if (known) {
            host_log_line("host: the sound driver has masaru.drv's idle loop\n");
        }
    }
    return known && sound_ram[0x22e] == 0 && sound_ram[0x22c] == 0 && ram32(0x13000) == ram32(ram32(0x1818));
}

/* the samples owed, of the blanks' 16683333 ns at 44.1 kHz. (running them
 * by the host's time at every register access as well didn't end the ADX
 * hang, and the driver's idle loop then went unfound: XBOX.md) */
static long long owed_ns_44100;

extern "C" int xbox_sound_open(void *ram)
{
    host_log_line("host: sound: Flycast's tables\n");
    flycast_run_onloads();
    sound_ram = (const u8 *)ram;
    aica::aica_ram.setRegion((u8 *)ram, ARAM_SIZE);
    host_log_line("host: sound: the AICA\n");
    aica::init();
    aica::reset(true);
    host_log_line("host: sound: the APU\n");
    aica::arm::idle_pc = 0x194;
    aica::arm::idle_wait = idle_wait;
    return aica_apu_open(ram);
}

/* the samples of blanks blanks, which may be more than one: the host's
 * chores, which call this, don't run at a blank the mask holds */
extern "C" void xbox_sound_blanks(unsigned blanks)
{
    unsigned count;

    if (blanks > 30) {
        blanks = 30; /* a long stall's time is lost, not caught up */
    }
    owed_ns_44100 += 16683333LL * 44100 * blanks;
    count = (unsigned)(owed_ns_44100 / 1000000000);
    owed_ns_44100 -= (long long)count * 1000000000;
    unsigned long long t0 = host_ticks();
    aica::arm::run(count);
    unsigned long long t1 = host_ticks();
    aica_apu_blank();
    xbox_sound_ticks_arm += t1 - t0;
    xbox_sound_ticks_apu += host_ticks() - t1;
}

extern "C" unsigned xbox_sound_read(unsigned offset, int size)
{
    if (offset >= 0x8000) {
        return 0;
    }
    return size == 1 ? aica::readAicaReg<u8>(offset)
           : size == 2 ? aica::readAicaReg<u16>(offset)
                       : aica::readAicaReg<u32>(offset);
}

extern "C" void xbox_sound_write(unsigned offset, unsigned value, int size)
{
    if (offset >= 0x8000) {
        return;
    }
    if (size == 1) {
        aica::writeAicaReg<u8>(offset, (u8)value);
    } else if (size == 2) {
        aica::writeAicaReg<u16>(offset, (u16)value);
    } else {
        aica::writeAicaReg<u32>(offset, value);
    }
}

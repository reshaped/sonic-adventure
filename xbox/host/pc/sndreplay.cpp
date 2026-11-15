/* sndreplay RECORD [TRACE]: a recording of the game's side of the AICA
 * (HOST_SOUND_RECORD, sound.cpp) run again against the sound's core as the
 * Xbox host builds it (FLYCAST_APU: the ARM7, the registers, the timers and
 * the channel model without samples), with no game and no sound device. it
 * prints a sum of what the core did, and under TRACE writes it a line an
 * event: each register write of the ARM7's with the sample it came at, each
 * key on, key off and stop of a channel, each register read of the game's
 * and its value, and a sum of sound RAM every 60 runs. two builds of the
 * core that give the same trace for a recording are the same to the driver
 * and to the game: the test for a change to the core (pc.mk's sndreplay) */
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

#include "hw/aica/aica_if.h"
#include "hw/aica/sgc_if.h"
#include "hw/arm7/arm7.h"
#include "cfg/option.h"

#include "../xbox/aica_channel.h"

namespace config {
Value<bool> DSPEnabled{false};
Value<bool> GGPOEnable{false};
Value<bool> VmuSound{false};
}
flycast_settings settings;

void flycast_log(const char *, const char *, ...) {}

void flycast_die(const char *file, int line, const char *what)
{
    fprintf(stderr, "aica: %s:%d: %s\n", file, line, what);
    abort();
}

namespace aica {
void initRtc() {}
void resetRtc(bool) {}
void termRtc() {}
namespace sgc {
unsigned long long profile_dsp, profile_channels;
}
extern unsigned long long profile_timers, profile_sample, profile_interrupts;
namespace arm {
extern unsigned long long profile_arm, profile_step;
unsigned profile_pcs[0x8000];
}
}

void WriteSample(s16, s16) {}

static const unsigned char *sound_ram_bytes;
static FILE *trace;
static unsigned sum = 2166136261u;
static unsigned long long samples;
static unsigned events;

/* an event: into the sum, and the trace */
extern "C" void flycast_trace(const char *kind, unsigned a, unsigned b)
{
    unsigned words[4] = {(unsigned)kind[0], a, b, (unsigned)samples};

    for (unsigned w : words) {
        sum = (sum ^ w) * 16777619u;
    }
    events++;
    if (trace != nullptr) {
        fprintf(trace, "%llu %s %x %x\n", samples, kind, a, b);
    }
}

/* the addresses the translated driver lacked (arm7.cpp's runStatic), each
 * once: roots for tools/arm2c.py */
static std::vector<unsigned> missed;

extern "C" void flycast_static_missed(unsigned pc)
{
    for (unsigned m : missed) {
        if (m == pc) {
            return;
        }
    }
    missed.push_back(pc);
}

#ifdef FLYCAST_TRACE_PC
/* the first million instructions, for a translation that goes astray */
extern "C" void flycast_pc(unsigned pc, unsigned r0, unsigned r1, unsigned flags)
{
    static FILE *out;
    static unsigned count;

    if (out == nullptr) {
        out = fopen(getenv("SOUND_PCS") != nullptr ? getenv("SOUND_PCS") : "/dev/null", "w");
    }
    if (count++ < 1000000) {
        fprintf(out, "%x %x %x %x\n", pc, r0, r1, flags >> 28);
    }
    if (count == 1) {
        atexit([] { fprintf(stderr, "sndreplay: %u instructions\n", count); });
    }
}
#endif

/* the sample the core is at, for the trace: sound_now counts within a run */
extern "C" unsigned sound_now;
unsigned sound_now;

extern "C" void aica_apu_key_on(int channel)
{
    struct aica_channel c;

    aica_channel_get(channel, &c);
    flycast_trace("on", (unsigned)channel, c.sa ^ c.lea << 8 ^ c.update_rate << 16 ^ (unsigned)c.pcms << 28);
    if (getenv("SOUND_KEYS") != nullptr) {
        /* each key on's sample, for a look at the banks: SOUND_KEYS=file, and
         * sound RAM as it was then in file.N for the first of each sample */
        static FILE *keys;
        static std::vector<unsigned> seen;
        bool is_new = true;

        if (keys == nullptr) {
            keys = fopen(getenv("SOUND_KEYS"), "w");
        }
        for (unsigned sa : seen) {
            is_new = is_new && sa != (c.sa ^ c.lea << 24);
        }
        fprintf(keys, "%x %x %x %d %d %d\n", c.sa, c.lsa, c.lea, c.pcms, c.lpctl, (int)is_new);
        if (is_new) {
            char name[256];

            seen.push_back(c.sa ^ c.lea << 24);
            snprintf(name, sizeof name, "%s.%x", getenv("SOUND_KEYS"), c.sa);
            if (FILE *ram = fopen(name, "wb")) {
                unsigned from = c.sa & ~0xfffffu;

                fwrite(sound_ram_bytes + from, 1, 0x100000, ram);
                fclose(ram);
            }
        }
        fflush(keys);
    }
}

extern "C" void aica_apu_key_off(int channel)
{
    flycast_trace("off", (unsigned)channel, 0);
}

extern "C" void aica_apu_stop(int channel)
{
    flycast_trace("stop", (unsigned)channel, 0);
}

/* no voices here: every channel goes by its model */
extern "C" int aica_apu_position(int)
{
    return -2;
}

static const u8 *sound_ram;

static u32 ram32(u32 at)
{
    return *(const u32 *)(sound_ram + (at & (ARAM_SIZE - 4)));
}

/* masaru.drv's idle loop, as sound.cpp and sound_xbox.cpp have it */
static bool idle_wait(void)
{
    static bool known;

    if (!known) {
        known = ram32(0x194) == 0xeb000202u && ram32(0x1b4) == 0xeafffff6u && ram32(0x1c0) == 0x0afffff3u &&
                ram32(0x1568) == 0xe3a03a13u && ram32(0x1570) == 0xe59f32a0u && ram32(0x198) == 0xe5df908eu &&
                ram32(0x1b8) == 0xe5df906cu;
    }
    return known && sound_ram[0x22e] == 0 && sound_ram[0x22c] == 0 && ram32(0x13000) == ram32(ram32(0x1818));
}

/* the core's run of a blank's samples, as the hosts call it: sound_run.cpp's
 * where the build has it, else Flycast's a sample at a time */
#ifdef SOUND_RUN
extern "C" void sound_run(unsigned count);
#else
static void sound_run(unsigned count)
{
    aica::arm::run(count);
}
#endif

int main(int argc, char **argv)
{
    static std::vector<u8> ram(ARAM_SIZE);
    FILE *record = argc > 1 ? fopen(argv[1], "rb") : nullptr;
    unsigned runs = 0, reads = 0, read_differences = 0;
    struct timespec t0, t1;
    int kind;

    if (record == nullptr) {
        fprintf(stderr, "usage: sndreplay RECORD [TRACE]\n");
        return 2;
    }
    if (argc > 2) {
        trace = fopen(argv[2], "w");
    }
    sound_ram = ram.data();
    sound_ram_bytes = ram.data();
    aica::aica_ram.setRegion(ram.data(), ARAM_SIZE);
    if (getenv("SOUND_NO_IDLE") == nullptr) {
        aica::arm::idle_pc = 0x194;
        aica::arm::idle_wait = idle_wait;
    }
    aica::init();
    aica::reset(true);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    while ((kind = fgetc(record)) != EOF) {
        unsigned w[3] = {0, 0, 0};

        if (fread(w, 4, kind == 'N' ? 1 : kind == 'W' ? 2 : 3, record) == 0) {
            break;
        }
        if (kind == 'W') {
            if (w[0] + w[1] > ARAM_SIZE || fread(ram.data() + w[0], 1, w[1], record) != w[1]) {
                break;
            }
        } else if (kind == 'S') {
            if (w[1] == 1) {
                aica::writeAicaReg<u8>(w[0], (u8)w[2]);
            } else if (w[1] == 2) {
                aica::writeAicaReg<u16>(w[0], (u16)w[2]);
            } else {
                aica::writeAicaReg<u32>(w[0], w[2]);
            }
        } else if (kind == 'R') {
            unsigned value = w[1] == 1   ? aica::readAicaReg<u8>(w[0])
                             : w[1] == 2 ? aica::readAicaReg<u16>(w[0])
                                         : aica::readAicaReg<u32>(w[0]);

            flycast_trace("read", w[0], value);
            reads++;
            read_differences += value != w[2];
        } else if (kind == 'N') {
            sound_run(w[0]);
            /* as the APU's bridge looks at every channel at each blank */
            aica::sgc::SyncAllChannels();
            samples += w[0];
            if (++runs % 60 == 0) {
                unsigned ram_sum = 2166136261u;

                for (unsigned k = 0; k < ARAM_SIZE; k += 4) {
                    ram_sum = (ram_sum ^ *(const u32 *)(ram.data() + k)) * 16777619u;
                }
                flycast_trace("ram", runs, ram_sum);
            }
        } else {
            fprintf(stderr, "sndreplay: an event of no kind at %ld\n", ftell(record));
            return 1;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("sndreplay: %u runs, %llu samples, %u events, sum %08x; %u reads of the game's, %u unlike the recording's; "
           "%.0f ms, %.2f ms a second of sound\n",
           runs, samples, events, sum, reads, read_differences,
           ((double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9) * 1000,
           ((double)(t1.tv_sec - t0.tv_sec) + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9) * 1000 / ((double)samples / 44100));
    {
        unsigned counts[3];

        aica_quiet_counts(counts);
        printf("sndreplay: the channels' steps: %u samples passed in %u runs at once, %u a sample at a time\n", counts[2],
               counts[1], counts[0]);
    }
    if (!missed.empty()) {
        printf("sndreplay: interpreted, for the translation lacks them:");
        for (unsigned m : missed) {
            printf(" --root 0x%x", m);
        }
        printf("\n");
    }
    {
        unsigned long long arm = aica::arm::profile_arm, timers = aica::profile_timers, channels = aica::profile_sample,
                           interrupts = aica::profile_interrupts, all = arm + timers + channels + interrupts;

        if (all != 0) {
            printf("sndreplay: of the timed parts, the ARM7 %llu%%, the channels %llu%%, the timers %llu%%, the "
                   "interrupts %llu%%\n",
                   arm * 100 / all, channels * 100 / all, timers * 100 / all, interrupts * 100 / all);
        }
    }
    return 0;
}

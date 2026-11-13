/* the PC backend's sound, as sound.h says. a program bank is SEGA's SMPB as
 * the driver reads it. it holds a header, programs of up to four layers, and
 * in each layer splits of 48 bytes. a split holds the AICA's channel
 * registers, then its key range, its own note and its fine tune. the backend
 * plays a split from those registers as the AICA plays a channel. a split's
 * sample address counts from the bank's start. the DSP runs an fx program.
 * the mix adds its outputs at the levels and pans of an fx output set.
 * qsound places a channel by rewriting the program's coefficients as the
 * driver does */
#include "sound.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace pc {
namespace {

enum { ENTRY_COMMAND = 1, ENTRY_BANK = 2, ENTRY_STREAM = 3 };
enum { PHASE_ATTACK, PHASE_DECAY1, PHASE_DECAY2, PHASE_RELEASE };
enum { FORMAT_PCM16 = 0, FORMAT_PCM8 = 1, FORMAT_ADPCM = 2 };

constexpr uint32_t QUEUE = SA_PC_SOUND_QUEUE;
constexpr double RATE = SA_PC_SOUND_RATE;
constexpr double LEVELS = 1024.0;
constexpr double TL_DB = 0.375; /* a total level's step, in dB */
/* the mix's scale: a voice or a stream at its loudest takes full scale, and
 * the sum clamps */
constexpr float MASTER = 1.0F;

/* the time in ms an envelope's rate of 0 to 63 takes over the whole range.
 * ATTACK_MS serves attack, and DECAY_MS decay and release. a time of 0
 * holds, and -1 is at once. the times are MAME's aica.cpp's ARTimes and
 * DRTimes, under its BSD-3-Clause licence:
 *
 * Copyright (c) ElSemi, Deunan Knute, R. Belmont
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
constexpr double ATTACK_MS[64] = {
    0,    0,    8100, 6900, 6000, 4800, 4000, 3400, 3000, 2400, 2000,
    1700, 1500, 1200, 1000, 860,  760,  600,  500,  430,  380,  300,
    250,  220,  190,  150,  130,  110,  95,   76,   63,   55,   47,
    38,   31,   27,   24,   19,   15,   13,   12,   9.4,  7.9,  6.8,
    6.0,  4.7,  3.8,  3.4,  3.0,  2.4,  2.0,  1.8,  1.6,  1.3,  1.1,
    0.93, 0.85, 0.65, 0.53, 0.44, 0.40, 0.35, -1,   -1};
constexpr double DECAY_MS[64] = {
    0,     0,     118200, 101300, 88600, 70900, 59100, 50700, 44300, 35500,
    29600, 25300, 22200,  17700,  14800, 12700, 11100, 8900,  7400,  6300,
    5500,  4400,  3700,   3200,   2800,  2200,  1800,  1600,  1400,  1100,
    920,   790,   690,    550,    460,   390,   340,   270,   230,   200,
    170,   140,   110,    98,     85,    68,    57,    49,    43,    34,
    28,    25,    22,     18,     14,    12,    11,    8.5,   7.1,   6.1,
    5.4,   4.3,   3.6,    3.1};

/* LFO_HZ is the LFO's frequency by its LFOF of 0 to 31. PITCH_CENTS and
 * LEVEL_DB are its depths by PLFOS and ALFOS. MAME's aica.cpp gives them,
 * under the licence above */
constexpr double LFO_HZ[32] = {0.17, 0.19, 0.23, 0.27, 0.34, 0.39, 0.45, 0.55,
                               0.68, 0.78, 0.92, 1.10, 1.39, 1.60, 1.87, 2.27,
                               2.87, 3.31, 3.92, 4.79, 6.15, 7.18, 8.60, 10.8,
                               14.4, 17.2, 21.5, 28.7, 43.1, 57.4, 86.1, 172.3};
constexpr double PITCH_CENTS[8] = {0,    7.0,   13.5,  27.0,
                                   55.0, 112.0, 230.0, 494.0};
constexpr double LEVEL_DB[8] = {0, 0.4, 0.8, 1.5, 3.0, 6.0, 12.0, 24.0};
enum { WAVE_SAW, WAVE_SQUARE, WAVE_TRIANGLE, WAVE_NOISE };
/* where the game's driver keeps qsound's table in sound memory. qsound
 * places QSOUND_CHANNELS channels. each has a send to each of the program's
 * QSOUND_MIXES mixes. the sends are the program's coefficients from
 * QSOUND_SENDS on, a mix's channels together */
constexpr uint32_t QSOUND_TABLE = 0x3bf0;
constexpr uint32_t QSOUND_CHANNELS = 4;
constexpr uint32_t QSOUND_MIXES = 4;
constexpr uint32_t QSOUND_SENDS = 84;
/* a program bank's split, in bytes */
constexpr uint32_t SPLIT = 0x30;
/* a sequence's events that are neither a note nor midi: one whose next byte
 * goes unread, the sequence's end, and a tempo */
constexpr uint32_t EVENT_UNREAD = 0x82, EVENT_END = 0x83, EVENT_TEMPO = 0x84;

/* the AICA's ADPCM: a code is a sign bit over a 3-bit size. ADPCM_SIZE
 * gives the change by the size, in eighths of the step. ADPCM_SCALE scales
 * the step for the next code, in 256ths. the step stays from ADPCM_LEAST to
 * ADPCM_MOST */
constexpr int ADPCM_SIZE[8] = {1, 3, 5, 7, 9, 11, 13, 15};
constexpr int ADPCM_SCALE[8] = {230, 230, 230, 230, 307, 409, 512, 614};
constexpr int ADPCM_LEAST = 0x7f, ADPCM_MOST = 0x6000;

/* an ADX block holds a 2-byte scale, then 4-bit codes, a byte's high code
 * first. ADX_STEP gives a low code's signed value */
constexpr int ADX_STEP[16] = {0,  1,  2,  3,  4,  5,  6,  7,
                              -8, -7, -6, -5, -4, -3, -2, -1};
constexpr uint32_t ADX_BLOCK = 18;
/* an ADX header's loop, as sa_audio_stream says a file holds one: the count
 * of loops, the loop's first sample and the sample it ends before. a header
 * that holds them has ADX_LOOP_HEADER bytes or more */
constexpr uint32_t ADX_LOOPS = 0x16;
constexpr uint32_t ADX_LOOP_START = 0x1c;
constexpr uint32_t ADX_LOOP_END = 0x24;
constexpr uint32_t ADX_LOOP_HEADER = 0x30;

uint32_t read16(const uint8_t *at)
{
    return static_cast<uint32_t>(at[0] | at[1] << 8);
}

uint32_t read32(const uint8_t *at)
{
    return read16(at) | read16(at + 2) << 16;
}

uint32_t big16(const uint8_t *at)
{
    return static_cast<uint32_t>(at[0] << 8 | at[1]);
}

uint32_t big32(const uint8_t *at)
{
    return big16(at) << 16 | big16(at + 2);
}

int32_t clamp16(int32_t x)
{
    return x > 32767 ? 32767 : x < -32768 ? -32768 : x;
}

/* it returns bank + offset where count records of each bytes fit in size,
 * and null otherwise. it divides, so no sum wraps */
const uint8_t *within(const uint8_t *bank, uint32_t size, uint32_t offset,
                      uint32_t count, uint32_t each)
{
    return offset <= size && count <= (size - offset) / each ? bank + offset
                                                             : nullptr;
}

/* the offset of a program bank's table of levels, or 0 where it has none */
uint32_t levels_at(const uint8_t *bank)
{
    return read32(bank + 0x1c) ? read32(bank + 0x18) : 0;
}

/* whether bank is a program bank whose tables all lie within size. its
 * header holds the programs' table's offset at 0x10 and their count at
 * 0x14. a layer holds its count of splits, then the first's offset. an
 * offset of 0 is a program or a layer the bank goes without */
bool programs_whole(const uint8_t *bank, uint32_t size)
{
    if (size < 0x30 || memcmp(bank, "SMPB", 4) != 0) {
        return false;
    }
    uint32_t count = read32(bank + 0x14), levels = levels_at(bank);
    const uint8_t *table = within(bank, size, read32(bank + 0x10), count, 4);

    if (!table || (levels && !within(bank, size, levels, 128, 1))) {
        return false;
    }
    for (uint32_t p = 0; p < count; p++) {
        uint32_t program = read32(table + 4 * p);
        const uint8_t *layers = within(bank, size, program, 4, 4);

        if (!program) {
            continue;
        }
        if (!layers) {
            return false;
        }
        for (uint32_t layer = 0; layer < 4; layer++) {
            uint32_t at = read32(layers + 4 * layer);
            const uint8_t *head = within(bank, size, at, 1, 16);

            if (at && (!head || !within(bank, size, read32(head + 4),
                                        read32(head), SPLIT))) {
                return false;
            }
        }
    }
    return true;
}

/* a rate's change of level a frame, from its time over the whole range */
double per_frame(double ms)
{
    if (ms == 0) {
        return 0;
    }
    if (ms < 0) {
        return LEVELS;
    }
    return LEVELS / (ms * RATE / 1000.0);
}

double decibels(double db)
{
    return pow(10.0, db / 20.0);
}

/* the filter's damping by its resonance Q. the AICA documents a resonance of
 * -3 dB at 0, rising 0.75 dB a step. this curve gives the same at 4 and
 * 1.9 dB more at 31 */
double damping(uint32_t q)
{
    return q < 4    ? 1.5 - q / 8.0
           : q < 12 ? 1 - (q - 4) / 16.0
           : q < 20 ? 0.5 - (q - 12) / 32.0
                    : 0.25 - (q - 20) / 64.0;
}

/* a stream's volume, in tenths of a dB, as the library carries it to the
 * driver. sofdec's mw_rna.c makes it an attenuation of 0 to 255. mw_sdcs.c
 * makes that a port's volume of -127 to 127. the driver gives the stream's
 * channels a total level of twice the port's fall below 0. the factor of two
 * is inferred, not read */
float stream_gain(int32_t vol)
{
    int32_t att = vol >= 0 ? 0 : vol <= -999 ? 255 : (vol * 255 - 128) / -999;
    int32_t fall = att * 10 / 18;
    int32_t port = att <= 0 ? 127 : fall > 127 ? -127 : -fall;
    int32_t tl = port < 0 ? -2 * port : 0;

    return static_cast<float>(decibels(-TL_DB * tl));
}

/* the gain of a send level as DISDL, IMXL and EFSDL take it. it falls 3 dB
 * a step below 15, and 0 is silent */
double send_gain(int level)
{
    return level > 0 ? decibels(-3.0 * (15 - (level > 15 ? 15 : level))) : 0;
}

/* the gains of a pan as DIPAN and EFPAN take it. its low 4 bits lower the
 * far side 3 dB a step, to silence at 15. with 0x10 set the far side is
 * the right */
void pan_gains(int pan, double &left, double &right)
{
    left = right = 1;
    if (pan & 15) {
        double away = (pan & 15) == 15 ? 0 : decibels(-3.0 * (pan & 15));

        (pan & 16 ? right : left) *= away;
    }
}

/* level once the driver adds add to it. add counts in 255ths of the room
 * between level and 255 */
int total_level(int level, int add)
{
    return level + add * (255 - level) / 255;
}

/* a tick's length in frames at a sequence's tempo. a tick lasts
 * tempo / 800 ms, and the 800 is inferred, not read */
double tick_frames(uint32_t tempo)
{
    return RATE * tempo / 800.0 / 1000.0;
}

} // namespace

bool Sound::create()
{
    memory_ = static_cast<uint8_t *>(calloc(SOUND_MEMORY, 1));
    bytes_ = static_cast<uint8_t *>(malloc(SOUND_BYTES));
    queue_ = static_cast<Entry *>(calloc(QUEUE, sizeof(Entry)));
    for (Stream &s : streams_) {
        s.data = static_cast<uint8_t *>(malloc(STREAM_BYTES));
        s.gain = 1;
    }
    for (float &gain : port_gain_) {
        gain = 1;
    }
    for (auto &port : channels_) {
        for (Channel &c : port) {
            c = Channel{};
        }
    }
    bool made = memory_ && bytes_ && queue_;

    for (const Stream &s : streams_) {
        made = made && s.data;
    }
    if (!made) {
        destroy();
    }
    return made;
}

void Sound::destroy()
{
    free(memory_);
    free(bytes_);
    free(queue_);
    memory_ = bytes_ = nullptr;
    queue_ = nullptr;
    for (Stream &s : streams_) {
        free(s.data);
        s.data = nullptr;
    }
}

bool Sound::push(const Entry &entry, const void *data, uint32_t bytes)
{
    uint32_t tail = tail_.load(std::memory_order_relaxed);
    uint32_t queued = tail - head_.load(std::memory_order_acquire);
    uint64_t at = bytes_tail_;

    if (queued >= QUEUE || bytes > SOUND_BYTES) {
        refused_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    if (at % SOUND_BYTES + bytes > SOUND_BYTES) {
        at += SOUND_BYTES - at % SOUND_BYTES;
    }
    if (at + bytes - bytes_head_.load(std::memory_order_acquire) >
        SOUND_BYTES) {
        refused_.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    Entry &slot = queue_[tail % QUEUE];

    slot = entry;
    slot.at = at;
    slot.end = bytes ? at + bytes : 0;
    if (bytes) {
        memcpy(bytes_ + at % SOUND_BYTES, data, bytes);
        bytes_tail_ = at + bytes;
    }
    tail_.store(tail + 1, std::memory_order_release);
    if (queued + 1 > most_queued_.load(std::memory_order_relaxed)) {
        most_queued_.store(queued + 1, std::memory_order_relaxed);
    }
    return true;
}

/* the commands go in whole or not at all. the places past the tail are the
 * game's thread's to write. the render reads none of them until the tail
 * moves, and it moves once, past them all */
sa_result Sound::submit(const sa_audio_command *commands, uint32_t count)
{
    uint32_t tail = tail_.load(std::memory_order_relaxed);
    uint32_t queued = tail - head_.load(std::memory_order_acquire);

    if (count > QUEUE - queued) {
        refused_.fetch_add(count, std::memory_order_relaxed);
        return SA_ERR_FULL;
    }
    for (uint32_t i = 0; i < count; i++) {
        Entry &slot = queue_[(tail + i) % QUEUE];

        slot = Entry{};
        slot.what = ENTRY_COMMAND;
        slot.command = commands[i];
    }
    tail_.store(tail + count, std::memory_order_release);
    if (queued + count > most_queued_.load(std::memory_order_relaxed)) {
        most_queued_.store(queued + count, std::memory_order_relaxed);
    }
    return SA_OK;
}

sa_result Sound::load(const sa_audio_bank &bank)
{
    Entry entry{};

    if (bank.address > SOUND_MEMORY ||
        bank.bytes > SOUND_MEMORY - bank.address ||
        (bank.data == nullptr && bank.bytes != 0)) {
        return SA_ERR_INVALID;
    }
    if (bank.kind == SA_BANK_PROGRAMS &&
        !programs_whole(static_cast<const uint8_t *>(bank.data), bank.bytes)) {
        return SA_ERR_INVALID;
    }
    entry.what = ENTRY_BANK;
    entry.bank = bank;
    entry.bank.data = nullptr;
    return push(entry, bank.data, bank.bytes) ? SA_OK : SA_ERR_FULL;
}

sa_result Sound::stream(uint32_t index, const void *data, uint32_t bytes)
{
    Entry entry{};

    if (index >= STREAMS || bytes > STREAM_BYTES || data == nullptr) {
        return SA_ERR_INVALID;
    }
    entry.what = ENTRY_STREAM;
    entry.stream = index;
    return push(entry, data, bytes) ? SA_OK : SA_ERR_FULL;
}

sa_pc_sound_stats Sound::stats() const
{
    sa_pc_sound_stats s{};

    s.refused = refused_.load(std::memory_order_relaxed);
    s.most_queued = most_queued_.load(std::memory_order_relaxed);
    s.most_voices = most_voices_.load(std::memory_order_relaxed);
    s.unplayed = unplayed_.load(std::memory_order_relaxed);
    return s;
}

void Sound::drain()
{
    uint32_t head = head_.load(std::memory_order_relaxed);
    uint32_t tail = tail_.load(std::memory_order_acquire);

    for (; head != tail; head++) {
        const Entry &e = queue_[head % QUEUE];
        const uint8_t *data = bytes_ + e.at % SOUND_BYTES;
        uint32_t bytes = e.end ? static_cast<uint32_t>(e.end - e.at) : 0;

        if (e.what == ENTRY_BANK) {
            if (e.end) {
                memcpy(memory_ + e.bank.address, data, bytes);
            }
            if (e.bank.kind == SA_BANK_PROGRAMS && e.bank.number < BANKS) {
                programs_[e.bank.number] = Place{e.bank.address, bytes};
            }
            if (e.bank.kind == SA_BANK_SEQUENCES && e.bank.number < BANKS) {
                sequence_banks_[e.bank.number] = Place{e.bank.address, bytes};
            }
            /* the driver lies at sound memory's start. an fx bank is known
             * by its mark. SFPB holds the DSP's programs, and SFOB holds its
             * outputs' levels and pans */
            if (e.bank.kind == SA_BANK_OTHER && e.bank.address == 0) {
                driver_bytes_ = bytes;
            }
            if (e.bank.kind == SA_BANK_OTHER && bytes >= 16 &&
                !memcmp(data, "SFPB", 4)) {
                fx_programs_ = Place{e.bank.address, bytes};
            }
            if (e.bank.kind == SA_BANK_OTHER && bytes >= 16 &&
                !memcmp(data, "SFOB", 4)) {
                fx_outputs_ = Place{e.bank.address, bytes};
            }
        } else if (e.what == ENTRY_STREAM) {
            start_stream(e.stream, e.end ? data : nullptr, e.end ? bytes : 0);
        } else {
            const sa_audio_command &c = e.command;
            uint32_t id = c.key.id;

            switch (c.kind) {
            case SA_AUDIO_MIDI:
                midi(id, static_cast<uint32_t>(c.value));
                break;
            case SA_AUDIO_SEQUENCE:
                sequence_start(id, static_cast<uint32_t>(c.sound) >> 8 & 0xff,
                               static_cast<uint32_t>(c.sound) & 0xff);
                break;
            case SA_AUDIO_SEQUENCE_STOP:
                sequence_stop(id);
                break;
            case SA_AUDIO_SEQUENCE_VOLUME:
                if (id < PORTS) {
                    double share = (127 + (c.value < 0 ? c.value : 0)) / 127.0;

                    port_gain_[id] =
                        static_cast<float>(share < 0 ? 0 : share * share);
                    for (Voice &v : voices_) {
                        if (v.on && v.port == id) {
                            gains(v);
                        }
                    }
                }
                break;
            case SA_AUDIO_STREAM_STOP:
                if (id < STREAMS) {
                    streams_[id].playing = false;
                }
                break;
            case SA_AUDIO_STREAM_VOLUME:
                if (id < STREAMS) {
                    streams_[id].gain = stream_gain(c.value);
                }
                break;
            case SA_AUDIO_STREAM_WAIT:
                if (id < STREAMS) {
                    wait_[id] = c.value;
                }
                break;
            case SA_AUDIO_FX_PROGRAM:
                fx_program(static_cast<uint32_t>(c.sound),
                           static_cast<uint32_t>(c.value));
                break;
            case SA_AUDIO_FX_OUTPUT:
                if (id < FX_OUTPUTS) {
                    fx_[id].level_moved = c.value;
                    fx_output_gains(id);
                }
                break;
            case SA_AUDIO_QSOUND:
                qsound_place(id, static_cast<uint32_t>(c.value));
                break;
            case SA_AUDIO_SPACE:
                mono_ = c.value != 0;
                for (Voice &v : voices_) {
                    if (v.on) {
                        gains(v);
                    }
                }
                for (uint32_t i = 0; i < FX_OUTPUTS; i++) {
                    fx_output_gains(i);
                }
                break;
            default:
                break;
            }
        }
        if (e.end) {
            bytes_head_.store(e.end, std::memory_order_release);
        }
        head_.store(head + 1, std::memory_order_release);
    }
}

void Sound::midi(uint32_t port, uint32_t message)
{
    uint32_t status = message & 0xf0, channel = message & 15;
    uint32_t d1 = message >> 8 & 0x7f, d2 = message >> 16 & 0x7f;

    if (port >= PORTS) {
        return;
    }
    Channel &c = channels_[port][channel];

    switch (status) {
    case 0x80:
        note_off(port, channel, d1);
        return;
    case 0x90:
        if (d2 == 0) {
            note_off(port, channel, d1);
        } else {
            note_on(port, channel, d1, d2);
        }
        return;
    case 0xb0:
        switch (d1) {
        case 0:
        case 32:
            c.bank = static_cast<uint8_t>(d2);
            break;
        case 7:
            c.volume = static_cast<uint8_t>(d2);
            break;
        case 10:
            c.pan = static_cast<uint8_t>(d2);
            break;
        case 11:
            c.expression = static_cast<uint8_t>(d2);
            break;
        case 120:
        case 123:
            for (Voice &v : voices_) {
                if (v.on && v.port == port && v.channel == channel) {
                    v.released = true;
                    v.phase = PHASE_RELEASE;
                    v.filter_phase = PHASE_RELEASE;
                }
            }
            break;
        case 121:
            c.expression = Channel{}.expression;
            c.bend = Channel{}.bend;
            break;
        default:
            break;
        }
        break;
    case 0xc0:
        c.program = static_cast<uint8_t>(d1);
        return;
    case 0xe0:
        c.bend = static_cast<uint16_t>(d1 | d2 << 7);
        break;
    default:
        return;
    }
    for (Voice &v : voices_) {
        if (v.on && v.port == port && v.channel == channel) {
            tune(v);
            gains(v);
        }
    }
}

/* a split's sample plays at the output's rate at its own note. keys and the
 * channel's bend move it in semitones, and the fine tune in cents */
void Sound::tune(Voice &v) const
{
    const Channel &c = channels_[v.port][v.channel];
    double bend = (c.bend - 8192.0) / 8192.0 * v.bend_range;
    double semitones = v.key - v.base + v.fine / 100.0 + bend;

    v.step = pow(2.0, semitones / 12.0);
}

/* what the driver adds to a voice's total level for x of 0 to 127. x is a
 * velocity, volume or expression. it is twice the fall below 127 of x's
 * entry in the bank's table of levels. with no table it is 46 for each
 * halving below 127. that velocity and expression take the table as volume
 * does is inferred */
int Sound::added_level(const Voice &v, int x) const
{
    if (v.levels) {
        /* an entry past 127 counts as 127: a rise would take the gain past
         * full scale */
        int level = memory_[v.levels + (x < 0 ? 0 : x > 127 ? 127 : x)];

        return 2 * (127 - (level > 127 ? 127 : level));
    }
    if (x >= 127) {
        return 0;
    }
    /* below 3 the steps pass 255, the most an addition takes */
    int add = x <= 0 ? 255 : static_cast<int>(lround(46.0 * log2(127.0 / x)));

    return add > 255 ? 255 : add;
}

void Sound::gains(Voice &v) const
{
    const Channel &c = channels_[v.port][v.channel];
    int tl = total_level(v.total_level, added_level(v, v.velocity));
    double left, right;

    /* velocity, volume and expression lower the send to the DSP as they
     * lower the direct output */
    tl = total_level(tl, added_level(v, c.volume));
    tl = total_level(tl, added_level(v, c.expression));
    double gain = decibels(-TL_DB * tl) * port_gain_[v.port];

    /* the driver pans a voice by its channel, in place of the split's pan.
     * the far side falls linearly to nothing at 0 or 127, on the direct
     * output alone. in mono every pan is heard centred, as on the AICA */
    left = right = 1;
    if (!mono_ && c.pan < 64) {
        right *= c.pan / 64.0;
    } else if (!mono_ && c.pan > 64) {
        left *= (127 - c.pan) / 63.0;
    }
    v.gain = static_cast<float>(gain);
    v.direct_left = static_cast<float>(send_gain(v.direct) * left);
    v.direct_right = static_cast<float>(send_gain(v.direct) * right);
    v.send = static_cast<float>(send_gain(v.send_level));
}

/* a voice that is free, or else the oldest released, or else the oldest */
Voice *Sound::take_voice()
{
    Voice *best = &voices_[0];

    for (Voice &v : voices_) {
        if (!v.on) {
            return &v;
        }
        if (v.released > best->released ||
            (v.released == best->released && v.age < best->age)) {
            best = &v;
        }
    }
    return best;
}

void Sound::note_on(uint32_t port, uint32_t channel, uint32_t key,
                    uint32_t velocity)
{
    const Channel &c = channels_[port][channel];
    Place place = programs_[c.bank];
    const uint8_t *bank = memory_ + place.address;
    uint32_t size = place.bytes;
    bool played = false;

    if (size < 0x30 || memcmp(bank, "SMPB", 4) != 0) {
        unplayed_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    /* load checks a bank whole, but bytes loaded later may lie over its
     * tables. each read here holds to the bank's bytes */
    uint32_t count = read32(bank + 0x14), levels = levels_at(bank);
    const uint8_t *table =
        c.program < count
            ? within(bank, size, read32(bank + 0x10), c.program + 1U, 4)
            : nullptr;
    uint32_t program = table ? read32(table + 4 * c.program) : 0;
    const uint8_t *layers =
        program ? within(bank, size, program, 4, 4) : nullptr;

    levels = levels && within(bank, size, levels, 128, 1)
                 ? place.address + levels
                 : 0;
    for (uint32_t layer = 0; layers && layer < 4; layer++) {
        uint32_t at = read32(layers + 4 * layer);
        const uint8_t *head = at ? within(bank, size, at, 1, 16) : nullptr;

        if (!head) {
            continue;
        }
        /* no more splits than the bank's bytes hold */
        uint32_t splits = read32(head), first = read32(head + 4);
        uint32_t held = first <= size ? (size - first) / SPLIT : 0;

        for (uint32_t s = 0; s < splits && s < held; s++) {
            const uint8_t *split = bank + first + SPLIT * s;

            if (key < split[36] || key > split[37]) {
                continue;
            }
            Voice &v = *take_voice();
            /* the registers are 16-bit words, rNN at split + NN / 2 */
            uint32_t r00 = read16(split), r04 = read16(split + 2);
            uint32_t r10 = read16(split + 8), r14 = read16(split + 10);
            uint32_t r1c = read16(split + 14), r20 = read16(split + 16);
            uint32_t r24 = read16(split + 18), r28 = read16(split + 20);
            uint32_t r40 = read16(split + 32), r44 = read16(split + 34);

            v = Voice{};
            v.on = true;
            v.port = static_cast<uint8_t>(port);
            v.channel = static_cast<uint8_t>(channel);
            v.key = static_cast<uint8_t>(key);
            v.velocity = static_cast<uint8_t>(velocity > 127 ? 127 : velocity);
            v.levels = levels;
            v.age = ++age_;
            v.start = place.address + ((r00 & 0x7f) << 16 | r04);
            v.format = r00 >> 7 & 3;
            v.loops = (r00 >> 9 & 1) != 0;
            v.loop_start = read16(split + 4);
            v.loop_end = read16(split + 6);
            v.base = split[38];
            v.fine = static_cast<int8_t>(split[39]);
            v.bend_range = head[12] ? head[12] : 2;
            v.quant = ADPCM_LEAST;
            v.level = LEVELS - 1;
            tune(v);
            /* as on the AICA, a rate is twice its register's. a KRS below
             * 15 adds twice the sum of KRS and octave. octave counts the
             * pitch's octaves above the split's own note */
            int krs = static_cast<int>(r14 >> 10 & 15);
            int octave = static_cast<int>(floor(log2(v.step)));
            int scale = krs == 15 ? 0 : 2 * (octave + krs);
            auto rate = [scale](uint32_t r) {
                int x = r ? static_cast<int>(2 * r) + scale : 0;

                return x < 0 ? 0 : x > 63 ? 63 : x;
            };
            v.attack = per_frame(ATTACK_MS[rate(r10 & 31)]);
            v.decay1 = per_frame(DECAY_MS[rate(r10 >> 6 & 31)]);
            v.decay2 = per_frame(DECAY_MS[rate(r10 >> 11 & 31)]);
            v.release = per_frame(DECAY_MS[rate(r14 & 31)]);
            v.decay_level = (r14 >> 5 & 31) * 32.0;
            v.direct = static_cast<int>(r24 >> 8 & 15);
            v.total_level = static_cast<int>(r28 >> 8);
            v.send_level = static_cast<int>(r20 >> 4 & 15);
            v.input = r20 & 15;
            /* the LFO, from its period's start */
            v.lfo_step = LFO_HZ[r1c >> 10 & 31] / RATE;
            v.pitch_wave = static_cast<int>(r1c >> 8 & 3);
            v.pitch_depth = PITCH_CENTS[r1c >> 5 & 7];
            v.level_wave = static_cast<int>(r1c >> 3 & 3);
            v.level_depth = LEVEL_DB[r1c & 7];
            v.noise = 0x12345U + static_cast<uint32_t>(v.age);
            v.noise_step = ~0U;
            /* the filter, unless LPOFF. its envelope takes the amplitude's
             * decay times across 8192 levels, eight times the amplitude's
             * 1024 */
            v.filtered = !(r28 & 0x20);
            for (uint32_t k = 0; k < FILTER_LEVELS; k++) {
                v.cutoffs[k] = read16(split + 22 + 2 * k) & 0x1fff;
            }
            v.cutoff = v.cutoffs[0];
            v.filter_rates[0] = 8 * per_frame(DECAY_MS[rate(r40 >> 8 & 31)]);
            v.filter_rates[1] = 8 * per_frame(DECAY_MS[rate(r40 & 31)]);
            v.filter_rates[2] = 8 * per_frame(DECAY_MS[rate(r44 >> 8 & 31)]);
            v.filter_rates[3] = 8 * per_frame(DECAY_MS[rate(r44 & 31)]);
            v.damping = damping(r28 & 31);
            v.filter_at = -1;
            gains(v);
            played = true;
            break;
        }
    }
    if (!played) {
        unplayed_.fetch_add(1, std::memory_order_relaxed);
    }
    uint32_t on = 0;

    for (const Voice &v : voices_) {
        on += v.on;
    }
    if (on > most_voices_.load(std::memory_order_relaxed)) {
        most_voices_.store(on, std::memory_order_relaxed);
    }
}

void Sound::note_off(uint32_t port, uint32_t channel, uint32_t key)
{
    for (Voice &v : voices_) {
        if (v.on && !v.released && v.port == port && v.channel == channel &&
            v.key == key) {
            v.released = true;
            v.phase = PHASE_RELEASE;
            v.filter_phase = PHASE_RELEASE;
        }
    }
}

/* a voice's sample at, in 16 bits. ADPCM decodes in order. it saves its
 * state on first reaching the loop's start. each pass of the loop starts
 * from that state */
int32_t Sound::sample(Voice &v, uint32_t at)
{
    switch (v.format) {
    case FORMAT_PCM16:
        if (v.start + 2 * at + 2 > SOUND_MEMORY) {
            return 0;
        }
        return static_cast<int16_t>(read16(memory_ + v.start + 2 * at));
    case FORMAT_PCM8:
        if (v.start + at >= SOUND_MEMORY) {
            return 0;
        }
        return static_cast<int8_t>(memory_[v.start + at]) * 256;
    default:
        break;
    }
    while (v.decoded <= at) {
        uint32_t n = v.decoded;

        if (v.loops && n == v.loop_start && !v.loop_saved) {
            v.loop_value = v.value;
            v.loop_quant = v.quant;
            v.loop_saved = true;
        }
        if (v.start + n / 2 >= SOUND_MEMORY) {
            return 0;
        }
        uint32_t code = memory_[v.start + n / 2] >> (n & 1 ? 4 : 0) & 15;

        int32_t change = v.quant * ADPCM_SIZE[code & 7] / 8;

        /* the change holds at 0x7fff, as on the AICA */
        change = change > 0x7fff ? 0x7fff : change;
        v.previous = v.value;
        v.value = clamp16(v.value + (code & 8 ? -change : change));
        v.quant = v.quant * ADPCM_SCALE[code & 7] >> 8;
        v.quant = v.quant < ADPCM_LEAST  ? ADPCM_LEAST
                  : v.quant > ADPCM_MOST ? ADPCM_MOST
                                         : v.quant;
        v.decoded++;
    }
    return at + 1 == v.decoded ? v.value : v.previous;
}

/* the LFO's frame: pitch, a factor of the step, and gain. a wave is a saw,
 * square or triangle, or noise drawn anew each 256th of its period. pitch
 * swings both ways, and gain only falls, as far as its depth */
void Sound::lfo_frame(Voice &v, double &pitch, double &gain)
{
    double p = v.lfo, swing, fall;
    uint32_t step = static_cast<uint32_t>(p * 256);

    if (step != v.noise_step) {
        v.noise = v.noise * 1103515245U + 12345U;
        v.noise_value = (v.noise >> 8 & 0xffff) / 32768.0 - 1.0;
        v.noise_step = step;
    }
    auto wave = [&](int kind, bool both) {
        switch (kind) {
        case WAVE_SAW:
            return both ? 2 * p - 1 : 1 - p;
        case WAVE_SQUARE:
            return p < 0.5 ? 1.0 : both ? -1.0 : 0.0;
        case WAVE_TRIANGLE:
            if (!both) {
                return p < 0.5 ? 1 - 2 * p : 2 * p - 1;
            }
            return p < 0.25 ? 4 * p : p < 0.75 ? 2 - 4 * p : 4 * p - 4;
        default:
            return both ? v.noise_value : (v.noise_value + 1) / 2;
        }
    };
    swing = v.pitch_depth ? wave(v.pitch_wave, true) : 0;
    fall = v.level_depth ? wave(v.level_wave, false) : 0;
    pitch = swing ? pow(2.0, swing * v.pitch_depth / 1200.0) : 1;
    gain = fall ? decibels(-fall * v.level_depth) : 1;
    v.lfo += v.lfo_step;
    if (v.lfo >= 1) {
        v.lfo -= 1;
    }
}

/* the filter's envelope runs from FLV0 through FLV1 and FLV2 to FLV3, and to
 * FLV4 once released. the low-pass has two poles. f is the level read as a
 * float, 4 bits of exponent over 9 of mantissa. it is 0.99 at 0x1ff8 and
 * halves every 512 levels down. at 0x1ff8 and Q 4, the filter passes all but
 * 0.4 dB at half the output's rate */
float Sound::filter_frame(Voice &v, float x)
{
    int phase = v.filter_phase > PHASE_RELEASE ? PHASE_RELEASE : v.filter_phase;
    double target = v.cutoffs[phase + 1];
    double rate = v.filter_rates[phase];

    if (v.cutoff < target) {
        v.cutoff = v.cutoff + rate < target ? v.cutoff + rate : target;
    } else if (v.cutoff > target) {
        v.cutoff = v.cutoff - rate > target ? v.cutoff - rate : target;
    }
    if (v.cutoff == target && phase < PHASE_DECAY2) {
        v.filter_phase++;
    }
    int at = static_cast<int>(v.cutoff);

    if (at != v.filter_at) {
        double f = ldexp(512 + (at & 511), (at >> 9) - 25);

        v.filter_gain = f * f;
        v.filter_b1 = 2 - f * v.damping - f * f;
        v.filter_b2 = 1 - f * v.damping;
        v.filter_at = at;
    }
    double y = v.filter_gain * x + v.filter_b1 * v.filter_y1 -
               v.filter_b2 * v.filter_y2;

    y = y > 1 ? 1 : y < -1 ? -1 : y;
    v.filter_y2 = v.filter_y1;
    v.filter_y1 = y;
    return static_cast<float>(y);
}

void Sound::voice_frame(Voice &v, float &left, float &right)
{
    /* a voice whose step outruns its loop passes every sample sound memory
     * can hold, at two a byte. ending it there keeps its position, and each
     * address plus a sample's index, far within 32 bits */
    if (v.position >= 2.0 * SOUND_MEMORY) {
        v.on = false;
        return;
    }
    uint32_t at = static_cast<uint32_t>(v.position);

    if (at >= v.loop_end) {
        if (!v.loops || v.loop_end <= v.loop_start) {
            v.on = false;
            return;
        }
        v.position -= v.loop_end - v.loop_start;
        at = static_cast<uint32_t>(v.position);
        if (v.format == FORMAT_ADPCM && v.loop_saved) {
            v.decoded = v.loop_start;
            v.value = v.loop_value;
            v.quant = v.loop_quant;
        }
    }
    /* the AICA interpolates between the samples around the position. ADPCM
     * decodes the next first, which leaves this one in previous */
    double frac = v.position - at;
    int32_t a, b;

    if (at + 1 < v.loop_end) {
        b = sample(v, at + 1);
        a = v.format == FORMAT_ADPCM ? v.previous : sample(v, at);
    } else {
        a = b = sample(v, at);
    }
    float x = static_cast<float>((a + (b - a) * frac) / 32768.0);

    switch (v.phase) {
    case PHASE_ATTACK:
        v.level -= v.attack;
        if (v.level <= 0) {
            v.level = 0;
            v.phase = PHASE_DECAY1;
        }
        break;
    case PHASE_DECAY1:
        v.level += v.decay1;
        if (v.level >= v.decay_level) {
            v.phase = PHASE_DECAY2;
        }
        break;
    case PHASE_DECAY2:
        v.level += v.decay2;
        break;
    default:
        v.level += v.release;
        break;
    }
    if (v.level >= LEVELS - 1 && v.phase != PHASE_ATTACK) {
        v.on = false;
        return;
    }
    double pitch = 1, lfo_gain = 1;

    if (v.pitch_depth || v.level_depth) {
        lfo_frame(v, pitch, lfo_gain);
    }
    if (v.filtered) {
        x = filter_frame(v, x);
    }
    float out = x * v.gain *
                static_cast<float>(decibels(-v.level * 3.0 / 32.0) * lfo_gain);

    left += out * v.direct_left;
    right += out * v.direct_right;
    if (v.send > 0) {
        dsp_.send(v.input, static_cast<int32_t>(out * v.send * 32768.0F));
    }
    v.position += v.step * pitch;
}

/* an ADX file has a big-endian header, as adx_dcd.c reads one. its blocks
 * start 4 past the offset at 2. they take the channels in turn. the stream
 * loops where sa_audio_stream says its file holds a loop. the loop's words
 * lie at one place whatever version the header gives, as adx_dcd.c's
 * ADX_DecodeInfoExLoop reads them */
void Sound::start_stream(uint32_t index, const uint8_t *data, uint32_t bytes)
{
    Stream &s = streams_[index];

    s.lead = wait_[index] > 0 ? wait_[index] * RATE / 1000 : 0;
    wait_[index] = 0;
    s.playing = false;
    if (!data || bytes < 0x2c || big16(data) != 0x8000 || data[4] != 3 ||
        data[5] != ADX_BLOCK || data[7] < 1 || data[7] > STREAM_CHANNELS) {
        return;
    }
    memcpy(s.data, data, bytes);
    s.bytes = bytes;
    s.offset = big16(data + 2) + 4;
    s.channels = data[7];
    s.rate = big32(data + 8);
    s.samples = big32(data + 12);
    uint32_t cutoff = big16(data + 16);

    /* the loop's words lie in the header, and in the bytes given */
    s.loops = s.offset >= ADX_LOOP_HEADER && bytes >= ADX_LOOP_HEADER &&
              big16(data + ADX_LOOPS) == 1;
    s.loop_start = s.loops ? big32(data + ADX_LOOP_START) : 0;
    s.loop_end = s.loops ? big32(data + ADX_LOOP_END) : s.samples;
    if (s.rate == 0 || s.loop_end <= s.loop_start || s.loop_end > s.samples) {
        s.loops = false;
    }
    /* as ADX_GetCoefficient, in the game's single precision */
    float a = sqrtf(2.0F) - cosf(6.283185307F * static_cast<float>(cutoff) /
                                 static_cast<float>(s.rate));
    float b = sqrtf(2.0F) - 1.0F;
    float c = (a - sqrtf((a + b) * (a - b))) / b;

    s.k0 = static_cast<int16_t>(c * 8192.0F);
    s.k1 = static_cast<int16_t>(-(c * c * 4096.0F));
    memset(s.history, 0, sizeof s.history);
    s.loop_saved = false;
    s.next = 0;
    s.block_at = ~0U;
    s.position = 0;
    s.ending = false;
    s.playing = s.rate != 0 && stream_take(s, s.current);
    if (s.playing && !stream_take(s, s.following)) {
        s.ending = true;
        s.following[0] = s.current[0];
        s.following[1] = s.current[1];
    }
}

bool Sound::stream_decode(Stream &s)
{
    uint32_t block = s.next / ADX_CODES;

    s.block_at = block * ADX_CODES;
    if (s.loops && block == s.loop_start / ADX_CODES) {
        if (!s.loop_saved) {
            memcpy(s.loop_history, s.history, sizeof s.history);
            s.loop_saved = true;
        } else {
            memcpy(s.history, s.loop_history, sizeof s.history);
        }
    }
    for (uint32_t ch = 0; ch < s.channels; ch++) {
        uint32_t at = s.offset + (block * s.channels + ch) * ADX_BLOCK;
        int32_t newer = s.history[ch][0], older = s.history[ch][1];

        /* a scale with its top bit set ends the data, as adx_dcda.c's
         * SCALE_END does */
        if (at + ADX_BLOCK > s.bytes || (s.data[at] & 0x80)) {
            return false;
        }
        int32_t gain = static_cast<int32_t>(big16(s.data + at) & 0x1fff) + 1;

        for (uint32_t i = 0; i < ADX_CODES; i++) {
            int8_t byte = static_cast<int8_t>(s.data[at + 2 + i / 2]);
            int32_t code = i & 1 ? ADX_STEP[byte & 15] : byte >> 4;
            int32_t x =
                clamp16(gain * code + ((s.k0 * newer + s.k1 * older) >> 12));

            s.block[ch][i] = static_cast<int16_t>(x);
            older = newer;
            newer = x;
        }
        s.history[ch][0] = newer;
        s.history[ch][1] = older;
    }
    return true;
}

/* the next sample of each channel, wrapping from the loop's end to its
 * start. it returns false at the stream's end */
bool Sound::stream_take(Stream &s, int16_t *out)
{
    if (s.loops && s.next >= s.loop_end) {
        s.next = s.loop_start;
        s.block_at = ~0U;
    }
    if (!s.loops && s.next >= s.samples) {
        return false;
    }
    if (s.block_at == ~0U || s.next < s.block_at ||
        s.next >= s.block_at + ADX_CODES) {
        if (!stream_decode(s)) {
            return false;
        }
    }
    for (uint32_t ch = 0; ch < s.channels; ch++) {
        out[ch] = s.block[ch][s.next - s.block_at];
    }
    s.next++;
    return true;
}

/* a frame between the two samples around the stream's position, once its
 * lead has passed. a file at the output's rate plays its own samples */
void Sound::stream_frame(Stream &s, float &left, float &right)
{
    if (s.lead >= 1) {
        s.lead--;
        return;
    }
    uint32_t right_channel = s.channels > 1 ? 1 : 0;
    float frac = static_cast<float>(s.position);
    float left_sample = s.current[0] + (s.following[0] - s.current[0]) * frac;
    float right_sample =
        s.current[right_channel] +
        (s.following[right_channel] - s.current[right_channel]) * frac;

    /* a stereo stream plays each channel on its own side. the driver pans
     * the right 0x0e, 42 dB down on the left, and the mix leaves that out.
     * mono plays both on both sides */
    if (mono_ && right_channel) {
        left_sample = right_sample = left_sample + right_sample;
    }
    left += left_sample / 32768.0F * s.gain;
    right += right_sample / 32768.0F * s.gain;
    s.position += s.rate / RATE;
    while (s.position >= 1.0) {
        s.position -= 1.0;
        s.current[0] = s.following[0];
        s.current[1] = s.following[1];
        /* past the last sample, it plays once more, held */
        if (s.ending) {
            s.playing = false;
            return;
        }
        s.ending = !stream_take(s, s.following);
        if (s.ending) {
            s.following[0] = s.current[0];
            s.following[1] = s.current[1];
        }
    }
}

/* a sequence bank, SMSB, holds its count of sequences at 12 and each one's
 * offset from 16. a sequence, SMSD, holds its tempo at 8 and its events
 * from 12 */
void Sound::sequence_start(uint32_t port, uint32_t bank, uint32_t number)
{
    if (port >= PORTS || bank >= BANKS) {
        return;
    }
    sequence_stop(port);
    Place place = sequence_banks_[bank];
    const uint8_t *at = memory_ + place.address;
    Sequence &q = sequences_[port];

    if (place.bytes < 16 || memcmp(at, "SMSB", 4) != 0 ||
        number >= read32(at + 12) || 16 + 4 * number + 4 > place.bytes) {
        unplayed_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    uint32_t offset = read32(at + 16 + 4 * number);

    if (!within(at, place.bytes, offset, 1, 12) ||
        memcmp(at + offset, "SMSD", 4) != 0) {
        unplayed_.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    q = Sequence{};
    q.playing = true;
    q.at = place.address + offset + 12;
    q.end = place.address + place.bytes;
    q.frames = tick_frames(read32(at + offset + 8));
}

void Sound::sequence_stop(uint32_t port)
{
    if (port >= PORTS) {
        return;
    }
    Sequence &q = sequences_[port];

    for (uint32_t i = 0; i < q.holding; i++) {
        note_off(port, q.held[i].channel, q.held[i].key);
    }
    q.holding = 0;
    q.playing = false;
}

/* one event, then its step, the ticks to the next. a note's first byte,
 * below 0x80, holds its channel. its bits 0x20 and 0x10 make its gate and
 * its step two bytes. EVENT_TEMPO sets the tempo, 0xbn is a controller and
 * 0xcn a program. EVENT_END ends the sequence, and the byte after
 * EVENT_UNREAD goes unread */
void Sound::sequence_event(uint32_t port, Sequence &q)
{
    const uint8_t *m = memory_ + q.at;
    uint32_t room = q.end - q.at, step = 0, used = 0;
    uint32_t c = room ? m[0] : EVENT_END;

    if (c < 0x80 && room >= 5) {
        uint32_t gate = c & 0x20 ? m[3] << 8 | m[4] : m[3];
        uint32_t at = c & 0x20 ? 5 : 4;

        if (at + (c & 0x10 ? 2 : 1) <= room) {
            step = c & 0x10 ? m[at] << 8 | m[at + 1] : m[at];
            used = at + (c & 0x10 ? 2 : 1);
            if (m[2] != 0 && q.holding < HELD) {
                note_on(port, c & 15, m[1], m[2]);
                q.held[q.holding++] = {static_cast<uint8_t>(c & 15), m[1],
                                       q.next + gate};
            }
        }
    } else if (c == EVENT_TEMPO && room >= 4) {
        q.frames = tick_frames(m[1] << 8 | m[2]);
        step = m[3];
        used = 4;
    } else if ((c & 0xf0) == 0xb0 && room >= 4) {
        midi(port, c | m[1] << 8 | m[2] << 16);
        step = m[3];
        used = 4;
    } else if ((c & 0xf0) == 0xc0 && room >= 3) {
        midi(port, c | m[1] << 8);
        step = m[2];
        used = 3;
    } else if (c == EVENT_UNREAD && room >= 3) {
        step = m[2];
        used = 3;
    }
    if (used == 0) {
        q.playing = false;
        if (c != EVENT_END) {
            unplayed_.fetch_add(1, std::memory_order_relaxed);
        }
        return;
    }
    q.at += used;
    q.next += step;
}

/* a frame of a port's sequence: the gates that end, then the events due.
 * where a gate ends on the tick a note of its key starts, the gate ends
 * first. so it releases the old note, not the new */
void Sound::sequence_frame(uint32_t port)
{
    Sequence &q = sequences_[port];

    for (uint32_t i = 0; i < q.holding;) {
        if (q.tick >= q.held[i].off) {
            note_off(port, q.held[i].channel, q.held[i].key);
            q.held[i] = q.held[--q.holding];
        } else {
            i++;
        }
    }
    while (q.playing && q.tick >= q.next) {
        sequence_event(port, q);
    }
    if (q.frames > 0) {
        q.tick += 1.0 / q.frames;
    }
}

/* an fx programs bank, SFPB, holds its count at 12 and each program's
 * offset from 16. a program's ring holds 8K, 16K, 32K or 64K words by the
 * low two bits at 0x20. its registers' image lies at the offset at 0x24. an fx
 * outputs bank, SFOB, holds its count at 12 and each output set's offset
 * from 16 */
void Sound::fx_program(uint32_t program, uint32_t output_set)
{
    const uint8_t *fpb = memory_ + fx_programs_.address;
    const uint8_t *fob = memory_ + fx_outputs_.address;
    uint32_t size = fx_programs_.bytes;
    const uint8_t *table = size >= 20 && program < read32(fpb + 12)
                               ? within(fpb, size, 0x10, program + 1, 4)
                               : nullptr;

    if (table) {
        const uint8_t *entry =
            within(fpb, size, read32(table + 4 * program), 1, 0x28);
        uint32_t image = entry ? read32(entry + 0x24) : 0;
        const uint8_t *registers =
            image ? within(fpb, size, image, 1, Dsp::IMAGE) : nullptr;

        if (registers) {
            dsp_.load(registers, Dsp::RING_LEAST << (entry[0x20] & 3));
        }
    }
    size = fx_outputs_.bytes;
    table = size >= 20 && output_set < read32(fob + 12)
                ? within(fob, size, 0x10, output_set + 1, 4)
                : nullptr;
    if (table) {
        uint32_t set = read32(table + 4 * output_set);

        for (uint32_t i = 0; i < FX_OUTPUTS && within(fob, size, set, i + 1, 2);
             i++) {
            fx_[i] =
                FxOutput{fob[set + 2 * i] & 15, fob[set + 2 * i + 1] & 31, 0};
            fx_output_gains(i);
        }
    }
}

/* the game moves an output's level from the set's in eighths of a step.
 * the game moves no pan, so the unit of a pan's move is unknown. the set's
 * pan stays */
void Sound::fx_output_gains(uint32_t i)
{
    double gain = send_gain(fx_[i].level + fx_[i].level_moved / 8);
    double left, right;

    pan_gains(mono_ ? 0 : fx_[i].pan, left, right);
    fx_left_[i] = static_cast<float>(gain * left);
    fx_right_[i] = static_cast<float>(gain * right);
}

/* 0x80 marks a qsound place changed. a marked place sets its channel's send
 * to each of the program's four mixes. the driver's table holds a row of
 * four sends for each four places. the table is the game's driver's,
 * version 1.01 build 0x38 as it says at 0x20. under a driver of another
 * build, no place moves a send */
void Sound::qsound_place(uint32_t channel, uint32_t place)
{
    const uint8_t *table = memory_ + QSOUND_TABLE;

    if (!(place & 0x80) || channel >= QSOUND_CHANNELS ||
        driver_bytes_ < QSOUND_TABLE + 512 ||
        memcmp(memory_ + 0x20, "\x01\x01\x38\x00 GM ", 8) != 0) {
        return;
    }
    for (uint32_t mix = 0; mix < QSOUND_MIXES; mix++) {
        uint32_t at = 16 * ((place & 0x7f) >> 2) + 4 * mix;

        dsp_.set_coefficient(QSOUND_SENDS + QSOUND_CHANNELS * mix + channel,
                             static_cast<int16_t>(read16(table + at)));
    }
}

void Sound::render(int16_t *out, uint32_t frames)
{
    drain();
    for (uint32_t f = 0; f < frames; f++) {
        float left = 0, right = 0;

        for (uint32_t port = 0; port < PORTS; port++) {
            if (sequences_[port].playing || sequences_[port].holding) {
                sequence_frame(port);
            }
        }
        for (Voice &v : voices_) {
            if (v.on) {
                voice_frame(v, left, right);
            }
        }
        for (Stream &s : streams_) {
            if (s.playing) {
                stream_frame(s, left, right);
            }
        }
        if (dsp_.running()) {
            int32_t fx[FX_OUTPUTS];

            dsp_.run(fx);
            for (uint32_t i = 0; i < FX_OUTPUTS; i++) {
                left += fx[i] / 32768.0F * fx_left_[i];
                right += fx[i] / 32768.0F * fx_right_[i];
            }
        }
        out[2 * f] = static_cast<int16_t>(
            clamp16(static_cast<int32_t>(left * MASTER * 32768.0F)));
        out[2 * f + 1] = static_cast<int16_t>(
            clamp16(static_cast<int32_t>(right * MASTER * 32768.0F)));
    }
}

} // namespace pc

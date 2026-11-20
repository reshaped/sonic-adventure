/* the PC backend's sound. the sound library's banks and midi play as its
 * driver plays them on the AICA and its DSP. ADX streams mix with them on
 * the host. the game's thread queues what it asks for. one other thread
 * carries out the queue, then renders. it is the audio device's thread or
 * the caller of sa_pc_audio_render. the game's thread never waits for the
 * render: a full queue refuses. what it refuses is lost, and the queue then
 * refuses all until the render has carried out what it holds and silenced
 * what plays */
#ifndef PC_SOUND_H
#define PC_SOUND_H
#include <stdint.h>
#ifdef __PSP__
#include <stdio.h>
#endif

#include <atomic>

#include "dsp.h"
#include "sa_engine.h"
#include "sa_pc.h"

namespace pc {

constexpr uint32_t SOUND_MEMORY = 2U << 20; /* the AICA's sound memory */
constexpr uint32_t VOICES = 64;
constexpr uint32_t PORTS = 8;
constexpr uint32_t CHANNELS = 16;
constexpr uint32_t BANKS = 128;
constexpr uint32_t STREAMS = 2;
/* the DSP's outputs */
constexpr uint32_t FX_OUTPUTS = 16;
/* the byte ring's size. the ring carries banks and streams to the render.
 * in any two vsyncs, the game gives at most a stage's banks, two music
 * files and two voices. the banks fit sound memory. the disc's longest
 * music file is 16.7 MB, and its longest voice 0.5 MB. a bank or stream
 * that would pass the ring's end starts at its start, which can leave as
 * many bytes unused. so the ring holds all the game gives between renders
 * no more than two vsyncs apart. it holds a lone stream of STREAM_BYTES
 * wherever the last one ended.
 * STREAM_BYTES is the most a stream's file holds: over eight minutes of
 * stereo ADX at 44.1 kHz. the PSP has memory for neither. a bank's bytes go
 * straight to sound memory there, and a stream plays from its file through
 * a window of STREAM_BYTES, a third of a second of stereo ADX: the ring
 * carries a stream's head alone */
#ifdef __PSP__
constexpr uint32_t SOUND_BYTES = 64U << 10;
constexpr uint32_t STREAM_BYTES = 16U << 10;
#else
constexpr uint32_t SOUND_BYTES = 64U << 20;
constexpr uint32_t STREAM_BYTES = 24U << 20;
#endif
/* a stream's channels at the most, and the samples an ADX block gives each */
constexpr uint32_t STREAM_CHANNELS = 2;
constexpr uint32_t ADX_CODES = 32;
/* the notes a sequence holds at once. a note past them isn't played */
constexpr uint32_t HELD = 32;
/* a filter's levels, FLV0 to FLV4 */
constexpr uint32_t FILTER_LEVELS = 5;

/* a number the render reads for each frame of a voice: a double, but a
 * float on the PSP, whose processor has no double */
#ifdef __PSP__
using Real = float;
#else
using Real = double;
#endif

/* one layer of a note, played as the AICA plays a channel */
struct Voice {
    bool on;
    bool released;
    uint8_t port, channel, key, velocity;
    uint64_t age;
    /* its split's sample: where it starts in sound memory, its format and
     * its loop, in samples */
    uint32_t start, format, loop_start, loop_end;
    bool loops;
    double position; /* in samples, from the start */
#ifdef __PSP__
    /* the PSP's render keeps the position as a sample and a 65536th of one,
     * and the gains its last block ended at, each side */
    uint32_t at, part;
    int32_t gain_left, gain_right;
    bool ramped;
#endif
    Real step; /* samples a frame */
    int base, fine; /* the split's own note and its cents */
    int bend_range;
    /* the ADPCM decoder: how many samples it has decoded, the last two, and
     * its step. loop_value and loop_quant keep the last and the step from
     * the loop's start */
    uint32_t decoded;
    int32_t value, quant, previous;
    int32_t loop_value, loop_quant;
    bool loop_saved;
    /* the envelope. its level is an attenuation from 0, the loudest, to
     * 1023, in steps of 3/32 dB. its rates are levels a frame */
    int phase;
    Real level;
    Real attack, decay1, decay2, release;
    Real decay_level;
    /* the split's levels: total level and direct send, and its send to
     * one of the DSP's inputs */
    int total_level, direct, send_level;
    uint32_t input;
    uint32_t levels; /* the bank's table of levels in sound memory, or 0 */
    float gain; /* from total level, velocity, volume, expression and port */
    float direct_left, direct_right, send;
    /* the LFO: its place in its period, its move a frame, and its waves and
     * depths. its pitch depth is in cents, and its level depth in dB */
    Real lfo, lfo_step;
    int pitch_wave, level_wave;
    Real pitch_depth, level_depth;
    uint32_t noise;
    Real noise_value;
    uint32_t noise_step; /* the period's 256th that drew noise_value, or ~0 */
    /* the filter: its levels FLV0 to FLV4, and its envelope's level, phase
     * and rates a frame. each rate is for the move to one level past FLV0 */
    bool filtered;
    Real cutoffs[FILTER_LEVELS];
    Real cutoff;
    int filter_phase;
    Real filter_rates[FILTER_LEVELS - 1];
    Real damping;
    int filter_at; /* the cutoff the coefficients are for, or -1 */
    Real filter_gain, filter_b1, filter_b2;
    Real filter_y1, filter_y2;
};

/* a midi channel. its pan and its bend start centred */
struct Channel {
    uint8_t bank = 0, program = 0, volume = 100, pan = 64, expression = 127;
    uint16_t bend = 0x2000;
};

/* a port playing a sequence: its events in sound memory, and its clock and
 * next event in ticks. it holds notes until their gates end */
struct Sequence {
    bool playing;
    uint32_t at, end;
    double tick, next;
    double frames; /* a tick's length in frames, by the tempo */
    struct Held {
        uint8_t channel, key;
        double off;
    } held[HELD];
    uint32_t holding;
};

/* an ADX stream: its file's bytes and where its decoder stands */
struct Stream {
    uint8_t *data;
    uint32_t bytes;
#ifdef __PSP__
    /* the stream's file, where it plays from one: data holds window_bytes
     * of it from window_at, the file's bytes from base on, and whole is the
     * stream's length, which bytes takes as the stream starts. part is the
     * position's 65536ths */
    FILE *file;
    uint32_t base, whole, window_at, window_bytes;
    uint32_t part;
#endif
    bool playing;
    bool ending; /* the file has run out: following repeats current */
    uint32_t channels, rate, samples, offset;
    int16_t k0, k1;
    bool loops;
    uint32_t loop_start, loop_end;
    uint32_t next; /* each channel's next sample to take */
    /* each channel's last two samples decoded, the newer first, and as they
     * stood entering the loop's block */
    int32_t history[STREAM_CHANNELS][2];
    int32_t loop_history[STREAM_CHANNELS][2];
    bool loop_saved;
    int16_t block[STREAM_CHANNELS][ADX_CODES];
    uint32_t block_at; /* the sample block[] begins at, or ~0 */
    double position; /* past current, in the file's samples */
    /* the samples around position */
    int16_t current[STREAM_CHANNELS], following[STREAM_CHANNELS];
    float gain;
    double lead; /* the frames it waits before it's heard */
};

/* a DSP output's level and pan from the fx outputs bank. level_moved is
 * the game's move of that level */
struct FxOutput {
    int level, pan;
    int level_moved;
};

/* what the queue carries */
struct Entry {
    uint32_t what; /* ENTRY_* */
    sa_audio_command command;
    sa_audio_bank bank; /* its bytes lie in the byte ring, not at data */
    uint32_t stream;
    uint64_t at, end; /* its bytes in the ring, unwrapped. end is 0 for none */
#ifdef __PSP__
    /* a stream's file, where its bytes lie in it and how many they are */
    FILE *file;
    uint32_t base, size;
#endif
};

class Sound {
public:
    bool create();
    void destroy();
    /* the game's thread */
    sa_result submit(const sa_audio_command *commands, uint32_t count);
    sa_result load(const sa_audio_bank &bank);
    sa_result stream(uint32_t index, const void *data, uint32_t bytes);
#ifdef __PSP__
    /* a stream of the size bytes at base in file, which the render reads a
     * window at a time and closes once another takes its place. head is
     * the stream's first bytes, its header's fields among them. a refusal
     * closes the file */
    sa_result stream_file(uint32_t index, FILE *file, uint32_t base,
                          uint32_t size, const void *head, uint32_t bytes);
#endif
    /* the render's thread: carry out the queue, then render frames of two
     * int16_t */
    void render(int16_t *out, uint32_t frames);
    /* any thread */
    sa_pc_sound_stats stats() const;

private:
    bool push(const Entry &entry, const void *data, uint32_t bytes);
    void lose(const sa_audio_bank *bank, uint32_t count);
    void drain();
    void silence();
    void midi(uint32_t port, uint32_t message);
    void sequence_start(uint32_t port, uint32_t bank, uint32_t number);
    void sequence_stop(uint32_t port);
    void sequence_frame(uint32_t port);
    void sequence_event(uint32_t port, Sequence &sequence);
    void note_on(uint32_t port, uint32_t channel, uint32_t key,
                 uint32_t velocity);
    void note_off(uint32_t port, uint32_t channel, uint32_t key);
    Voice *take_voice();
    void tune(Voice &voice) const;
    void gains(Voice &voice) const;
    int added_level(const Voice &voice, int x) const;
    void start_stream(uint32_t index, const uint8_t *data, uint32_t bytes);
    int32_t sample(Voice &voice, uint32_t at);
    int32_t loop_first(Voice &voice);
    void voice_frame(Voice &voice, float &left, float &right);
    void stream_frame(Stream &stream, float &left, float &right);
    bool stream_take(Stream &stream, int16_t *out);
    bool stream_decode(Stream &stream);
    void lfo_frame(Voice &voice, double &pitch, double &gain);
    float filter_frame(Voice &voice, float x);
    void fx_program(uint32_t program, uint32_t output_set);
    void fx_output_gains(uint32_t output);
    void qsound_place(uint32_t channel, uint32_t place);
#ifdef __PSP__
    /* the PSP's render (mixer.h), a block of frames at a time */
    void sequence_block(uint32_t port, uint32_t frames);
    uint32_t reach_ = 0; /* the most a bank's end lay into sound memory */
    void voice_block(Voice &voice, int32_t *mix, uint32_t frames);
    void stream_block(Stream &stream, int32_t *mix, uint32_t frames);
#endif

    uint8_t *memory_ = nullptr;
    uint8_t *bytes_ = nullptr;
    Entry *queue_ = nullptr;
    /* submit and push move the tails, and drain moves the heads */
    std::atomic<uint32_t> head_{0}, tail_{0};
    std::atomic<uint64_t> bytes_head_{0};
    uint64_t bytes_tail_ = 0;
    struct Place {
        uint32_t address, bytes;
    };
    Place programs_[BANKS] = {};
    Place sequence_banks_[BANKS] = {};
    Sequence sequences_[PORTS] = {};
    float port_gain_[PORTS] = {}; /* 1 from create() */
    Voice voices_[VOICES] = {};
    Channel channels_[PORTS][CHANNELS] = {};
    Stream streams_[STREAMS] = {};
    /* the DSP, the fx programs and outputs banks, and each output's level,
     * pan and gain each side */
    Dsp dsp_;
    Place fx_programs_ = {}, fx_outputs_ = {};
    uint32_t driver_bytes_ = 0; /* the driver's size, at address 0 */
    FxOutput fx_[FX_OUTPUTS] = {};
    float fx_left_[FX_OUTPUTS] = {}, fx_right_[FX_OUTPUTS] = {};
    uint64_t age_ = 0;
    bool mono_ = false; /* from SA_AUDIO_SPACE: each pan plays centred */
    int32_t wait_[STREAMS] = {}; /* each stream's next lead, in ms */
    /* the refusals the render has yet to silence, and the program and
     * sequence banks whose loads they refused, a bit each */
    std::atomic<uint32_t> lost_{0};
    std::atomic<uint64_t> lost_banks_[2][BANKS / 64] = {};
    std::atomic<uint32_t> refused_{0}, most_queued_{0};
    std::atomic<uint32_t> most_voices_{0}, unplayed_{0};
};

} // namespace pc
#endif

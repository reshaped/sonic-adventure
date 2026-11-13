/* the PC backend's sound. the sound library's banks and midi play as its
 * driver plays them on the AICA and its DSP. ADX streams mix with them on
 * the host. the game's thread queues what it asks for. one other thread
 * carries out the queue, then renders. it is the audio device's thread or
 * the caller of sa_pc_audio_render. the game's thread never waits for the
 * render: a full queue refuses */
#ifndef PC_SOUND_H
#define PC_SOUND_H
#include <stdint.h>

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
/* the byte ring's size. the ring carries banks and streams to the render */
constexpr uint32_t SOUND_BYTES = 32U << 20;
/* the most a stream's file holds: over eight minutes of stereo ADX at
 * 44.1 kHz */
constexpr uint32_t STREAM_BYTES = 24U << 20;
/* a stream's channels at the most, and the samples an ADX block gives each */
constexpr uint32_t STREAM_CHANNELS = 2;
constexpr uint32_t ADX_CODES = 32;
/* the notes a sequence holds at once. a note past them isn't played */
constexpr uint32_t HELD = 32;
/* a filter's levels, FLV0 to FLV4 */
constexpr uint32_t FILTER_LEVELS = 5;

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
    double step; /* samples a frame */
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
    double level;
    double attack, decay1, decay2, release;
    double decay_level;
    /* the split's levels: total level and direct send, and its send to
     * one of the DSP's inputs */
    int total_level, direct, send_level;
    uint32_t input;
    uint32_t levels; /* the bank's table of levels in sound memory, or 0 */
    float gain; /* from total level, velocity, volume, expression and port */
    float direct_left, direct_right, send;
    /* the LFO: its place in its period, its move a frame, and its waves and
     * depths. its pitch depth is in cents, and its level depth in dB */
    double lfo, lfo_step;
    int pitch_wave, level_wave;
    double pitch_depth, level_depth;
    uint32_t noise;
    double noise_value;
    uint32_t noise_step; /* the period's 256th that drew noise_value, or ~0 */
    /* the filter: its levels FLV0 to FLV4, and its envelope's level, phase
     * and rates a frame. each rate is for the move to one level past FLV0 */
    bool filtered;
    double cutoffs[FILTER_LEVELS];
    double cutoff;
    int filter_phase;
    double filter_rates[FILTER_LEVELS - 1];
    double damping;
    int filter_at; /* the cutoff the coefficients are for, or -1 */
    double filter_gain, filter_b1, filter_b2;
    double filter_y1, filter_y2;
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
};

class Sound {
public:
    bool create();
    void destroy();
    /* the game's thread */
    sa_result submit(const sa_audio_command *commands, uint32_t count);
    sa_result load(const sa_audio_bank &bank);
    sa_result stream(uint32_t index, const void *data, uint32_t bytes);
    /* the render's thread: carry out the queue, then render frames of two
     * int16_t */
    void render(int16_t *out, uint32_t frames);
    /* any thread */
    sa_pc_sound_stats stats() const;

private:
    bool push(const Entry &entry, const void *data, uint32_t bytes);
    void drain();
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
    void voice_frame(Voice &voice, float &left, float &right);
    void stream_frame(Stream &stream, float &left, float &right);
    bool stream_take(Stream &stream, int16_t *out);
    bool stream_decode(Stream &stream);
    void lfo_frame(Voice &voice, double &pitch, double &gain);
    float filter_frame(Voice &voice, float x);
    void fx_program(uint32_t program, uint32_t output_set);
    void fx_output_gains(uint32_t output);
    void qsound_place(uint32_t channel, uint32_t place);

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
    std::atomic<uint32_t> refused_{0}, most_queued_{0};
    std::atomic<uint32_t> most_voices_{0}, unplayed_{0};
};

} // namespace pc
#endif

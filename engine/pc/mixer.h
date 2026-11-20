/* pc::Sound's render for the PSP, which sound.cpp includes in the place of
 * its own. that one makes each frame of each voice in doubles, with
 * a power of ten for its level, and the PSP's processor has no double and
 * a third of a second of work in each second of such sound. this one takes
 * a block of BLOCK frames at a time. a voice's envelope, its LFO and its
 * filter's cutoff move once a block, in floats, and its gains run from the
 * last block's to this one's across it. the frames between are integers: a
 * sample between two, by the position's 65536ths, times a gain each side.
 *
 * it leaves out the DSP, so no voice sends to it and no effect is heard,
 * and a sequence's clock moves a block at a time, so a note may start a
 * block's length late, 1.5 ms */
#include <math.h>

namespace pc {

namespace {

constexpr uint32_t BLOCK = 64;
/* a gain as an integer, and as one that runs across a block */
constexpr int GAIN_BITS = 14;
constexpr int RAMP_BITS = 8;

/* an envelope's level as a gain: 3/32 dB a level */
float level_gains[1024];
bool level_gains_made;

inline float level_gain(float level)
{
    int at = static_cast<int>(level);

    return level_gains[at < 0 ? 0 : at > 1023 ? 1023 : at];
}

inline int32_t gain_word(float gain)
{
    float scaled = gain * MASTER * static_cast<float>(1 << GAIN_BITS);

    return scaled > 32767.0F ? 32767 << RAMP_BITS
                             : static_cast<int32_t>(scaled) << RAMP_BITS;
}

} // namespace

/* sequence_frame, for frames frames at once */
void Sound::sequence_block(uint32_t port, uint32_t frames)
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
        q.tick += frames / q.frames;
    }
}

void Sound::voice_block(Voice &v, int32_t *mix, uint32_t frames)
{
    float span = static_cast<float>(frames);
    float pitch = 1, lfo_gain = 1;

    switch (v.phase) {
    case PHASE_ATTACK:
        v.level -= v.attack * span;
        if (v.level <= 0) {
            v.level = 0;
            v.phase = PHASE_DECAY1;
        }
        break;
    case PHASE_DECAY1:
        v.level += v.decay1 * span;
        if (v.level >= v.decay_level) {
            v.phase = PHASE_DECAY2;
        }
        break;
    case PHASE_DECAY2:
        v.level += v.decay2 * span;
        break;
    default:
        v.level += v.release * span;
        break;
    }
    if (v.level >= static_cast<float>(LEVELS) - 1 && v.phase != PHASE_ATTACK) {
        v.on = false;
        return;
    }
    /* the LFO, as lfo_frame has it, at the block's start */
    if (v.pitch_depth != 0 || v.level_depth != 0) {
        float p = v.lfo, swing = 0, fall = 0;
        uint32_t step = static_cast<uint32_t>(p * 256);

        if (step != v.noise_step) {
            v.noise = v.noise * 1103515245U + 12345U;
            v.noise_value =
                static_cast<float>(v.noise >> 8 & 0xffff) / 32768.0F - 1.0F;
            v.noise_step = step;
        }
        auto wave = [&](int kind, bool both) -> float {
            switch (kind) {
            case WAVE_SAW:
                return both ? 2 * p - 1 : 1 - p;
            case WAVE_SQUARE:
                return p < 0.5F ? 1.0F : both ? -1.0F : 0.0F;
            case WAVE_TRIANGLE:
                if (!both) {
                    return p < 0.5F ? 1 - 2 * p : 2 * p - 1;
                }
                return p < 0.25F ? 4 * p : p < 0.75F ? 2 - 4 * p : 4 * p - 4;
            default:
                return both ? v.noise_value : (v.noise_value + 1) / 2;
            }
        };

        if (v.pitch_depth != 0) {
            swing = wave(v.pitch_wave, true);
        }
        if (v.level_depth != 0) {
            fall = wave(v.level_wave, false);
        }
        if (swing != 0) {
            pitch = powf(2.0F, swing * v.pitch_depth / 1200.0F);
        }
        if (fall != 0) {
            lfo_gain = powf(10.0F, -fall * v.level_depth / 20.0F);
        }
        v.lfo += v.lfo_step * span;
        while (v.lfo >= 1) {
            v.lfo -= 1;
        }
    }
    /* the filter's envelope, as filter_frame has it */
    if (v.filtered) {
        int phase =
            v.filter_phase > PHASE_RELEASE ? PHASE_RELEASE : v.filter_phase;
        float target = v.cutoffs[phase + 1];
        float rate = v.filter_rates[phase] * span;

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
            float f =
                ldexpf(static_cast<float>(512 + (at & 511)), (at >> 9) - 25);

            v.filter_gain = f * f;
            v.filter_b1 = 2 - f * v.damping - f * f;
            v.filter_b2 = 1 - f * v.damping;
            v.filter_at = at;
        }
    }
    float gain = v.gain * level_gain(v.level) * lfo_gain;
    int32_t left_to = gain_word(gain * v.direct_left);
    int32_t right_to = gain_word(gain * v.direct_right);

    if (!v.ramped) {
        v.gain_left = left_to;
        v.gain_right = right_to;
        v.ramped = true;
    }
    int32_t left = v.gain_left, right = v.gain_right;
    int32_t left_step = (left_to - left) / static_cast<int32_t>(frames);
    int32_t right_step = (right_to - right) / static_cast<int32_t>(frames);
    float stepped = v.step * pitch * 65536.0F;
    /* a step past 16 octaves up is no pitch a voice has */
    uint32_t step =
        stepped < 4294967040.0F ? static_cast<uint32_t>(stepped) : 0xffffff00U;
    uint32_t at = v.at, part = v.part;
    bool adpcm = v.format == FORMAT_ADPCM;

    for (uint32_t i = 0; i < frames; i++) {
        /* a voice whose step outruns its loop passes every sample sound
         * memory can hold, as voice_frame says */
        if (at >= 2 * SOUND_MEMORY) {
            v.on = false;
            break;
        }
        if (at >= v.loop_end) {
            if (!v.loops || v.loop_end <= v.loop_start) {
                v.on = false;
                break;
            }
            at -= v.loop_end - v.loop_start;
            if (adpcm && v.loop_saved) {
                v.decoded = v.loop_start;
                v.value = v.loop_value;
                v.quant = v.loop_quant;
            }
        }
        int32_t a, b;

        if (at + 1 < v.loop_end) {
            b = sample(v, at + 1);
            a = adpcm ? v.previous : sample(v, at);
        } else {
            a = b = sample(v, at);
        }
        int32_t x = a + (((b - a) * static_cast<int32_t>(part >> 1)) >> 15);

        if (v.filtered) {
            float y = v.filter_gain * (static_cast<float>(x) / 32768.0F) +
                      v.filter_b1 * v.filter_y1 - v.filter_b2 * v.filter_y2;

            y = y > 1 ? 1 : y < -1 ? -1 : y;
            v.filter_y2 = v.filter_y1;
            v.filter_y1 = y;
            x = static_cast<int32_t>(y * 32767.0F);
        }
        mix[2 * i] += (x * (left >> RAMP_BITS)) >> GAIN_BITS;
        mix[2 * i + 1] += (x * (right >> RAMP_BITS)) >> GAIN_BITS;
        left += left_step;
        right += right_step;
        part += step;
        at += part >> 16;
        part &= 0xffff;
    }
    v.at = at;
    v.part = part;
    v.gain_left = left_to;
    v.gain_right = right_to;
}

/* stream_frame, for frames frames: the stream's samples by its rate over
 * the output's, each side by the stream's gain */
void Sound::stream_block(Stream &s, int32_t *mix, uint32_t frames)
{
    if (s.lead >= 1) {
        uint32_t held =
            s.lead >= frames ? frames : static_cast<uint32_t>(s.lead);

        s.lead -= held;
        mix += 2 * held;
        frames -= held;
    }
    uint32_t right_channel = s.channels > 1 ? 1 : 0;
    int32_t gain = gain_word(s.gain) >> RAMP_BITS;
    uint32_t step = static_cast<uint32_t>(
        (static_cast<uint64_t>(s.rate) << 16) / SA_PC_SOUND_RATE);

    for (uint32_t i = 0; i < frames; i++) {
        int32_t part = static_cast<int32_t>(s.part >> 1);
        int32_t left =
            s.current[0] + (((s.following[0] - s.current[0]) * part) >> 15);
        int32_t right =
            s.current[right_channel] +
            (((s.following[right_channel] - s.current[right_channel]) * part) >>
             15);

        /* a stereo stream plays each channel on its own side, and mono
         * both on both, as stream_frame says */
        if (mono_ && right_channel) {
            left = right = left + right;
        }
        mix[2 * i] += (left * gain) >> GAIN_BITS;
        mix[2 * i + 1] += (right * gain) >> GAIN_BITS;
        s.part += step;
        while (s.part >= 65536) {
            s.part -= 65536;
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
}

void Sound::render(int16_t *out, uint32_t frames)
{
    static int32_t mix[2 * BLOCK];

    if (!level_gains_made) {
        for (int i = 0; i < 1024; i++) {
            level_gains[i] =
                powf(10.0F, -static_cast<float>(i) * 3.0F / 32.0F / 20.0F);
        }
        level_gains_made = true;
    }
    drain();
    for (uint32_t done = 0; done < frames;) {
        uint32_t count = frames - done < BLOCK ? frames - done : BLOCK;

        memset(mix, 0, sizeof mix);
        for (uint32_t port = 0; port < PORTS; port++) {
            if (sequences_[port].playing || sequences_[port].holding) {
                sequence_block(port, count);
            }
        }
        for (Voice &v : voices_) {
            if (v.on) {
                voice_block(v, mix, count);
            }
        }
        for (Stream &s : streams_) {
            if (s.playing) {
                stream_block(s, mix, count);
            }
        }
        for (uint32_t i = 0; i < 2 * count; i++) {
            out[2 * done + i] = static_cast<int16_t>(clamp16(mix[i]));
        }
        done += count;
    }
}

} // namespace pc

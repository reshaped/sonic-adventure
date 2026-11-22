/* the Xbox host's bridge from the Dreamcast's AICA to the MCPX APU: each of
 * the AICA's 64 channels plays on a hardware voice of nxdk-audio's
 * (github.com/Ryzee119/nxdk-audio), while Flycast's channel model
 * (sgc_if.cpp under FLYCAST_APU) goes on without making samples, for the
 * envelope levels, positions and loop flags the sound driver reads back.
 *
 * the model calls aica_apu_key_on, _key_off and _stop as its channels key
 * on, release and end; the host calls aica_apu_blank at each blank, after the
 * ARM7 has run the blank's samples, which starts the voices keyed on since,
 * stops those that ended, and sets every playing voice's pitch and gains
 * from the model:
 *
 * - the sample: PCM16 is read where it is, in sound RAM, so that a stream
 *   the game keeps writing, as the ADX library's, plays as it's written.
 *   PCM8, signed on the AICA and unsigned on the APU, and ADPCM, the AICA's
 *   own, which the APU's isn't, are converted at the key on into a pool of
 *   copies. noise plays nothing yet.
 * - the loop: a looping channel plays from its start, then from LSA to LEA;
 *   a one-shot plays to LEA.
 * - the pitch: update_rate / 1024 of the 44.1 kHz voice.
 * - the gains: 2^(-att/16) by side, from the model's attenuation, its TL,
 *   pan and send level with its AEG's level, set again at each blank: the
 *   AICA's envelopes play as the model steps them, at 60 steps a second,
 *   which the APU's volume ramps smooth.
 *
 * not yet: the DSP's effects, the FEG's filter, the PLFO's vibrato, noise,
 * and the ADPCM streams' rings, which the game rewrites as they play */
#include <math.h>
#include <nxaudio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <xboxkrnl/xboxkrnl.h>

#include "aica_apu.h"
#include "aica_channel.h"
#include "sndpack.h"

void host_log_line(const char *format, ...);

#define CHANNELS 64
/* the converted samples' room, the sound pack's and those made here: 1 MB,
 * because the push buffer and the textures' pool hitch and fault where they
 * run out, and take the rest. what doesn't fit is converted at its key on.
 * the pack's reads keep 512 KB of it for those */
#define POOL_SIZE 0x100000

struct voice {
    nxAudioVoice voice;
    /* two, by turns: nxdk-audio keeps a pointer to the buffer it was given,
     * and unlocks its pages through it when the next is submitted, so the
     * old one has to stay as it was until then. one, rewritten before the
     * submit, had it unlock the new buffer's pages and never the old's, which
     * corrupted the kernel's page counts and, in time, memory */
    nxAudioBuffer buffers[2];
    int which;
    int created;
    int keyed; /* a key on to start at the blank */
    int playing;
    int stopping; /* a stop sent, the voice not yet stopped */
    uint32_t update_rate;
    uint32_t att_left, att_right;
    int bytes_per_sample;
    /* the part of the pool its sample plays from, at to end, or 0 to 0 where
     * it plays from sound RAM */
    uint32_t pool_at, pool_end;
};

static void pack_open(void);

static struct voice voices[CHANNELS];
static const uint8_t *sound_ram;
static int16_t *pool;
static uint32_t pool_used;
static unsigned pool_wraps; /* the pool's turns: each overwrites what it held */
static int ready;
static unsigned starts, stops, failures;
/* the pool's turns that stepped past a playing voice's sample, and the
 * voices stopped because their channel's model had ended without a stop */
static unsigned pool_skips, orphans;

int aica_apu_open(const void *ram)
{
    nxAudioInitParams params;
    nxAudioFormat format;
    int k;

    sound_ram = ram;
    memset(&params, 0, sizeof params);
    if (!nxAudioInit(&params)) {
        host_log_line("host: audio: nxAudioInit failed: %d\n", (int)nxAudioGetLastError());
        return -1;
    }
    pool = MmAllocateContiguousMemoryEx(POOL_SIZE, 0, 0x03ffffff, 0x1000, PAGE_READWRITE | PAGE_WRITECOMBINE);
    memset(&format, 0, sizeof format);
    format.sample_rate = 44100;
    format.channels = 1;
    format.bytes_per_sample = 2;
    format.codec = NX_AUDIO_CODEC_PCM;
    format.type = NX_VOICE_TYPE_2D_STATIC;
    for (k = 0; k < CHANNELS; k++) {
        if (!nxAudioVoiceCreate(&voices[k].voice, &format)) {
            host_log_line("host: audio: no voice %d: %d\n", k, (int)nxAudioGetLastError());
            return -1;
        }
        voices[k].created = 1;
        voices[k].bytes_per_sample = 2;
    }
    if (pool == NULL) {
        host_log_line("host: audio: no room for converted samples\n");
        return -1;
    }
    ready = 1;
    pack_open();
    host_log_line("host: audio: the APU plays the AICA's %d channels\n", CHANNELS);
    return 0;
}

void aica_apu_key_on(int channel)
{
    if (ready && channel >= 0 && channel < CHANNELS) {
        voices[channel].keyed = 1;
    }
}

void aica_apu_key_off(int channel)
{
    /* the model releases: its AEG's level falls, and the gains with it */
    (void)channel;
}

void aica_apu_stop(int channel)
{
    struct voice *v;

    if (!ready || channel < 0 || channel >= CHANNELS) {
        return;
    }
    v = &voices[channel];
    v->keyed = 0;
    if (v->playing) {
        nxAudioVoiceStop(&v->voice);
        v->playing = 0;
        v->stopping = 1;
        stops++;
    }
}

/* where a playing voice is in its sample, for the channel's model, which
 * goes by it where the sample is a ring the game keeps writing (sgc_if.cpp's
 * Sync): the voice's own offset, in 16-bit samples */
int aica_apu_position(int channel)
{
    const struct voice *v;

    if (!ready || channel < 0 || channel >= CHANNELS) {
        return -2;
    }
    v = &voices[channel];
    if (!v->playing || v->bytes_per_sample != 2) {
        return -1;
    }
    return (int)(nxAudioVoiceGetPlaybackOffset(&v->voice) / 2);
}

static float gain(uint32_t att)
{
    return att >= 255 ? 0.0F : powf(2.0F, -(float)att / 16.0F);
}

/* the converted copies kept for the next key on of the same sample: the
 * game keys the same few effects on again and again, and the conversion of
 * each, ADPCM's above all, was most of the voices' time, up to 38 ms a
 * second on a real Xbox. a copy is found by its sample's address, length
 * and format, and kept while a checksum of the sample's bytes holds, as the
 * driver may load another sample there. the pool's ring drops a copy it
 * writes over */
#define COPIES 1024

struct copy {
    uint32_t sa, count, pcms, sum;
    uint32_t at, bytes; /* in the pool */
    int used;
    /* one read from the sound pack (aica_apu_preload) rather than converted
     * here: found by its place and its sample's first bytes, with no sum of
     * the whole sample */
    int packed;
    uint8_t head[8];
};

static struct copy copies[COPIES];
static unsigned copy_next;
static unsigned packed_hits, packed_loaded, converted;

/* FNV-1a of the sample's bytes, by words */
static uint32_t sample_sum(uint32_t sa, uint32_t bytes)
{
    uint32_t sum = 2166136261u;
    uint32_t k;

    for (k = 0; k < bytes; k += 4) {
        sum = (sum ^ *(const uint32_t *)(sound_ram + ((sa + k) & 0x1ffffc))) * 16777619u;
    }
    return sum;
}

static const void *copy_find(uint32_t sa, uint32_t count, uint32_t pcms, uint32_t sum)
{
    int k;

    for (k = 0; k < COPIES; k++) {
        const struct copy *c = &copies[k];

        if (c->used && c->sa == sa && c->count == count && c->pcms == pcms && c->sum == sum) {
            return (const uint8_t *)pool + c->at;
        }
    }
    return NULL;
}

static void copy_keep(uint32_t sa, uint32_t count, uint32_t pcms, uint32_t sum, const void *data, uint32_t bytes)
{
    struct copy *c = &copies[copy_next++ % COPIES];

    c->sa = sa;
    c->count = count;
    c->pcms = pcms;
    c->sum = sum;
    c->at = (uint32_t)((const uint8_t *)data - (const uint8_t *)pool);
    c->bytes = bytes;
    c->used = 1;
    c->packed = 0;
}

static const void *packed_find(uint32_t sa, uint32_t count, uint32_t pcms)
{
    int k;

    for (k = 0; k < COPIES; k++) {
        const struct copy *c = &copies[k];

        if (c->used && c->packed && c->sa == sa && c->count == count && c->pcms == pcms && sa + 8 <= 0x200000 &&
            memcmp(c->head, sound_ram + sa, 8) == 0) {
            packed_hits++;
            return (const uint8_t *)pool + c->at;
        }
    }
    return NULL;
}

/* the sound pack, D:\\sounds.pak (xboxsnd.c): each bank file's samples
 * converted ahead as start converts them. the host calls aica_apu_preload
 * as the game opens a bank file, a place where a read costs nothing that
 * shows: the file's samples go into the pool, the first of them that fit in
 * its room less what the conversions here keep, and a key on finds them by
 * packed_find. those that didn't fit, and any sample the pack lacks, are
 * converted at their key on as before */
static HANDLE pack_file = INVALID_HANDLE_VALUE;
static struct sndpack_file *pack_files;
static uint32_t pack_file_count;

static int16_t *pool_take(uint32_t count);

static void pack_open(void)
{
    struct sndpack_header header;
    DWORD got;

    pack_file = CreateFileA("D:\\sounds.pak", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                            NULL);
    if (pack_file == INVALID_HANDLE_VALUE) {
        host_log_line("host: audio: no D:\\sounds.pak: every sample converted as it's keyed on\n");
        return;
    }
    if (!ReadFile(pack_file, &header, sizeof header, &got, NULL) || got != sizeof header ||
        header.magic != SNDPACK_MAGIC || header.count > 4096 ||
        (pack_files = malloc(header.count * sizeof *pack_files)) == NULL ||
        !ReadFile(pack_file, pack_files, header.count * sizeof *pack_files, &got, NULL) ||
        got != header.count * sizeof *pack_files) {
        host_log_line("host: audio: D:\\sounds.pak isn't a sound pack\n");
        return;
    }
    pack_file_count = header.count;
    host_log_line("host: audio: %u bank files in the sound pack\n", (unsigned)pack_file_count);
}

void aica_apu_preload(const char *name)
{
    const struct sndpack_file *f = NULL;
    struct sndpack_sample *samples;
    uint32_t k, taken = 0, loaded = 0;
    DWORD got;

    for (k = 0; k < pack_file_count; k++) {
        if (strcmp(pack_files[k].name, name) == 0) {
            f = &pack_files[k];
        }
    }
    if (!ready || f == NULL || f->samples == 0) {
        return;
    }
    samples = malloc(f->samples * sizeof *samples);
    if (samples == NULL || SetFilePointer(pack_file, (LONG)f->offset, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
        !ReadFile(pack_file, samples, f->samples * sizeof *samples, &got, NULL) || got != f->samples * sizeof *samples) {
        free(samples);
        return;
    }
    for (k = 0; k < f->samples; k++) {
        const struct sndpack_sample *s = &samples[k];
        uint32_t room = (s->bytes + 4095) & ~4095u;
        struct copy *c;
        void *to;

        /* a part of the pool stays the conversions' */
        if (taken + room > POOL_SIZE - 0x80000) {
            break;
        }
        to = pool_take((s->bytes + 1) / 2);
        if (to == NULL || SetFilePointer(pack_file, (LONG)(f->offset + f->samples * sizeof *samples + s->data), NULL,
                                         FILE_BEGIN) == INVALID_SET_FILE_POINTER ||
            !ReadFile(pack_file, to, s->bytes, &got, NULL) || got != s->bytes) {
            break;
        }
        taken += room;
        c = &copies[copy_next++ % COPIES];
        c->sa = s->sa;
        c->count = s->count;
        c->pcms = s->pcms;
        c->sum = 0;
        c->at = (uint32_t)((const uint8_t *)to - (const uint8_t *)pool);
        c->bytes = room;
        c->used = 1;
        c->packed = 1;
        memcpy(c->head, s->head, sizeof c->head);
        loaded++;
    }
    free(samples);
    packed_loaded += loaded;
    host_log_line("host: audio: %s: %u of its %u samples read from the sound pack, %u KB\n", name, (unsigned)loaded,
                  (unsigned)f->samples, (unsigned)(taken >> 10));
}

/* a copy of count samples in the pool, or null: the pool is a ring, whose
 * oldest copies a new one may overwrite while their voices still play */
static int16_t *pool_take(uint32_t count)
{
    uint32_t bytes = (count * 2 + 4095) & ~4095u;
    uint32_t at;
    int k;

    if (bytes > POOL_SIZE) {
        return NULL;
    }
    /* the pool is a ring: the next room past what it gave last, but never a
     * sample a voice still plays from. writing over one had a looping voice
     * play the new sample's bytes, as the 1-up's, at its loop points */
    for (k = 0;; k++) {
        uint32_t busy_end = 0;
        int n;

        if (pool_used + bytes > POOL_SIZE) {
            pool_used = 0;
            pool_wraps++;
        }
        at = pool_used;
        for (n = 0; n < CHANNELS; n++) {
            const struct voice *v = &voices[n];

            if ((v->playing || v->stopping) && v->pool_end > at && v->pool_at < at + bytes && v->pool_end > busy_end) {
                busy_end = v->pool_end;
            }
        }
        if (busy_end == 0) {
            break;
        }
        if (k == 2 * CHANNELS) {
            return NULL;
        }
        if (pool_skips++ < 8) {
            host_log_line("host: audio: the pool's room at %lu KB holds a playing voice's sample: past it, to %lu KB\n",
                          (unsigned long)(at >> 10), (unsigned long)(((busy_end + 4095) & ~4095u) >> 10));
        }
        pool_used = (busy_end + 4095) & ~4095u;
    }
    pool_used += bytes;
    for (k = 0; k < COPIES; k++) {
        if (copies[k].used && copies[k].at < at + bytes && at < copies[k].at + copies[k].bytes) {
            copies[k].used = 0;
        }
    }
    return (int16_t *)((uint8_t *)pool + at);
}

static void start(int k)
{
    struct voice *v = &voices[k];
    struct aica_channel c;
    uint32_t count;
    const void *data;
    nxAudioFormat format;

    aica_channel_get(k, &c);
    if (!c.enabled) {
        return;
    }
    /* a one-shot plays to LEA; a loop's end is LEA too */
    count = c.lea + 1;
    memset(&format, 0, sizeof format);
    format.sample_rate = 44100;
    format.channels = 1;
    format.codec = NX_AUDIO_CODEC_PCM;
    format.type = NX_VOICE_TYPE_2D_STATIC;
    if (c.pcms == 0) {
        data = sound_ram + (c.sa & 0x1ffffe);
        format.bytes_per_sample = 2;
    } else if (c.pcms == 1) {
        /* signed to unsigned, 8 bits */
        uint32_t sum = 0;

        data = packed_find(c.sa, count, 1);
        if (data == NULL) {
            sum = sample_sum(c.sa, count);
            data = copy_find(c.sa, count, 1, sum);
        }
        if (data == NULL) {
            uint8_t *to = (uint8_t *)pool_take((count + 1) / 2);
            uint32_t n;

            if (to == NULL) {
                failures++;
                return;
            }
            for (n = 0; n < count; n++) {
                to[n] = (uint8_t)(sound_ram[(c.sa + n) & 0x1fffff] ^ 0x80);
            }
            copy_keep(c.sa, count, 1, sum, to, ((count + 1) / 2 * 2 + 4095) & ~4095u);
            data = to;
        }
        format.bytes_per_sample = 1;
    } else if (c.pcms == 2 || c.pcms == 3) {
        /* ADPCM; a stream's (3) isn't kept, as the game rewrites it */
        uint32_t sum = 0;

        data = c.pcms == 2 ? packed_find(c.sa, count, 2) : NULL;
        if (data == NULL && c.pcms == 2) {
            sum = sample_sum(c.sa, (count + 1) / 2);
            data = copy_find(c.sa, count, 2, sum);
        }
        if (data == NULL) {
            int16_t *to = pool_take(count);

            if (to == NULL) {
                failures++;
                return;
            }
            aica_adpcm_decode(sound_ram, c.sa, count, to);
            converted++;
            if (c.pcms == 2) {
                copy_keep(c.sa, count, 2, sum, to, (count * 2 + 4095) & ~4095u);
            }
            data = to;
        }
        format.bytes_per_sample = 2;
    } else {
        return;
    }
    if (format.bytes_per_sample != v->bytes_per_sample) {
        nxAudioVoiceSetFormat(&v->voice, &format);
        v->bytes_per_sample = format.bytes_per_sample;
    }
    if ((const uint8_t *)data >= (const uint8_t *)pool && (const uint8_t *)data < (const uint8_t *)pool + POOL_SIZE) {
        v->pool_at = (uint32_t)((const uint8_t *)data - (const uint8_t *)pool);
        v->pool_end = v->pool_at + count * (uint32_t)format.bytes_per_sample;
    } else {
        v->pool_at = v->pool_end = 0;
    }
    v->which ^= 1;
    if (!nxAudioBufferInitialize(&v->buffers[v->which], data, count * (uint32_t)format.bytes_per_sample) ||
        !nxAudioBufferSubmit(&v->voice, &v->buffers[v->which])) {
        failures++;
        return;
    }
    nxAudioVoiceSetLooping(&v->voice, c.lpctl != 0);
    if (c.lpctl) {
        nxAudioVoiceSetLoopStart(&v->voice, c.lsa);
    }
    v->update_rate = c.update_rate;
    nxAudioVoiceSetPitch(&v->voice, (float)c.update_rate / 1024.0F);
    v->att_left = c.att_left;
    v->att_right = c.att_right;
    nxAudioVoiceSetChannelGain(&v->voice, gain(c.att_left), gain(c.att_right), 0.0F, 0.0F, 0.0F, 0.0F);
    if (!nxAudioVoiceStart(&v->voice)) {
        failures++;
        return;
    }
    v->playing = 1;
    starts++;
}

void aica_apu_blank(void)
{
    int k;

    if (!ready) {
        return;
    }
    /* the channels' models up to now: those that have run out stop their
     * voices here */
    aica_channels_sync();
    for (k = 0; k < CHANNELS; k++) {
        struct voice *v = &voices[k];

        if (v->stopping && nxAudioVoiceGetState(&v->voice) == NX_STOPPED) {
            v->stopping = 0;
        }
        if (v->keyed) {
            if (v->playing) {
                /* keyed on again while it plays: stop it, and start it once
                 * the voice has stopped */
                nxAudioVoiceStop(&v->voice);
                v->playing = 0;
                v->stopping = 1;
            }
            if (!v->stopping) {
                v->keyed = 0;
                start(k);
            }
            continue;
        }
        if (v->playing) {
            struct aica_channel c;

            aica_channel_get(k, &c);
            if (!c.enabled) {
                /* the model ended the channel without a stop: a looping
                 * voice would play on at its last gains */
                if (orphans++ < 8) {
                    host_log_line("host: audio: channel %d's voice played on past its channel's end (sa 0x%06lx, "
                                  "pcms %d, loop %d): stopped\n",
                                  k, (unsigned long)c.sa, c.pcms, c.lpctl);
                }
                aica_apu_stop(k);
                continue;
            }
            if (c.update_rate != v->update_rate) {
                v->update_rate = c.update_rate;
                nxAudioVoiceSetPitch(&v->voice, (float)c.update_rate / 1024.0F);
            }
            if (c.att_left != v->att_left || c.att_right != v->att_right) {
                v->att_left = c.att_left;
                v->att_right = c.att_right;
                nxAudioVoiceSetChannelGain(&v->voice, gain(c.att_left), gain(c.att_right), 0.0F, 0.0F, 0.0F, 0.0F);
            }
        }
    }
}

void aica_apu_report(void)
{
    int playing = 0;
    int k;

    for (k = 0; k < CHANNELS; k++) {
        playing += voices[k].playing;
    }
    host_log_line("host: audio: %d voices playing; %u starts, %u stops, %u failures since\n", playing, starts, stops,
                  failures);
    host_log_line("host: audio: %u samples from the sound pack, %u converted here since; %u read from the pack so far\n",
                  packed_hits, converted, packed_loaded);
    starts = stops = failures = 0;
    packed_hits = converted = 0;
}

/* the report's line (host.c): the pool's and the voices' use */
void aica_apu_report_memory(void)
{
    int playing = 0, created = 0, cached = 0;
    int k;

    for (k = 0; k < CHANNELS; k++) {
        playing += voices[k].playing;
        created += voices[k].created;
    }
    for (k = 0; k < COPIES; k++) {
        cached += copies[k].used;
    }
    host_log_line("report: sound pool: at %u KB of %u, %u turns so far, %u steps past a playing sample; %d of %d "
                  "conversions cached; %d voices made, %d playing, of %d; %u orphans stopped\n",
                  pool_used >> 10, (unsigned)(POOL_SIZE >> 10), pool_wraps, pool_skips, cached, COPIES, created,
                  playing, CHANNELS, orphans);
}

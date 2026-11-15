/* the Xbox host's sound, on the MCPX APU through nxdk-audio
 * (github.com/Ryzee119/nxdk-audio, MIT, which run.sh fetches).
 *
 * so far: HOST_AUDIO_TEST, a tone on one hardware voice, to bring the APU
 * up. nxdk-audio drives the voice processor and runs a passthrough program
 * on the GP DSP, which carries the voices' mix to the AC97 output: a real
 * Xbox has no sound without it, where xemu, by default, plays the voice
 * processor's mix itself */
#include <math.h>
#include <nxaudio.h>
#include <string.h>
#include <windows.h>

#include "audio.h"

void host_log_line(const char *format, ...);

int audio_open(void)
{
    nxAudioInitParams params;

    memset(&params, 0, sizeof params);
    if (!nxAudioInit(&params)) {
        host_log_line("host: audio: nxAudioInit failed: %d\n", (int)nxAudioGetLastError());
        return -1;
    }
    host_log_line("host: audio: the APU is up\n");
    return 0;
}

/* the test: a second of a 440 Hz sine, mono, 16 bits at 44.1 kHz, looping
 * on one voice, in four parts of three seconds: in the middle, a fifth up
 * (660 Hz, by the voice's pitch), on the left alone, on the right alone */
void audio_test(void)
{
    static short tone[44100];
    static nxAudioVoice voice;
    static nxAudioBuffer buffer;
    static const struct {
        const char *what;
        float pitch, left, right;
    } parts[4] = {
        {"440 Hz in the middle", 1.0F, 0.7F, 0.7F},
        {"660 Hz in the middle", 1.5F, 0.7F, 0.7F},
        {"440 Hz on the left", 1.0F, 1.0F, 0.0F},
        {"440 Hz on the right", 1.0F, 0.0F, 1.0F},
    };
    nxAudioFormat format;
    int k;

    for (k = 0; k < 44100; k++) {
        tone[k] = (short)(8000.0 * sin(2.0 * 3.14159265358979 * 440.0 * k / 44100.0));
    }
    memset(&format, 0, sizeof format);
    format.sample_rate = 44100;
    format.channels = 1;
    format.bytes_per_sample = 2;
    format.codec = NX_AUDIO_CODEC_PCM;
    format.type = NX_VOICE_TYPE_2D_STATIC;
    if (!nxAudioVoiceCreate(&voice, &format) || !nxAudioBufferInitialize(&buffer, tone, sizeof tone) ||
        !nxAudioBufferSubmit(&voice, &buffer) || !nxAudioVoiceSetLooping(&voice, true)) {
        host_log_line("host: audio: the test voice failed: %d\n", (int)nxAudioGetLastError());
        return;
    }
    for (k = 0; k < 4; k++) {
        nxAudioVoiceSetPitch(&voice, parts[k].pitch);
        nxAudioVoiceSetChannelGain(&voice, parts[k].left, parts[k].right, 0.0F, 0.0F, 0.0F, 0.0F);
        if (k == 0 && !nxAudioVoiceStart(&voice)) {
            host_log_line("host: audio: the test voice won't start: %d\n", (int)nxAudioGetLastError());
            return;
        }
        host_log_line("host: audio: %s\n", parts[k].what);
        Sleep(3000);
    }
    host_log_line("host: audio: voice state %d, offset %lu\n", (int)nxAudioVoiceGetState(&voice),
                  (unsigned long)nxAudioVoiceGetPlaybackOffset(&voice));
    nxAudioVoiceStop(&voice);
}

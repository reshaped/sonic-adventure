/* an AICA channel as the Xbox host's APU bridge (aica_apu.c) takes it from
 * Flycast's channel model, sgc_if.cpp under FLYCAST_APU */
#ifndef XBOX_HOST_XBOX_AICA_CHANNEL_H
#define XBOX_HOST_XBOX_AICA_CHANNEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct aica_channel {
    int enabled;
    uint32_t sa; /* the sample's byte address in sound RAM */
    int pcms; /* 0 PCM16, 1 PCM8, 2 ADPCM, 3 ADPCM stream, 4 noise */
    int lpctl; /* loops from lsa to lea */
    uint32_t lsa, lea; /* in samples from sa */
    uint32_t update_rate; /* the pitch: 1024 plays at 44.1 kHz */
    uint32_t att_left, att_right; /* 1/16 of a doubling a step, 255 silent */
    int aeg_state; /* 0 attack, 1 decay 1, 2 decay 2, 3 release */
    uint32_t attack_rate;
    uint32_t ca; /* the model's current sample */
};

void aica_channel_get(int k, struct aica_channel *out);
void aica_adpcm_decode(const uint8_t *ram, uint32_t sa, uint32_t count, int16_t *out);

/* the bridge's side, which sgc_if.cpp calls */
void aica_apu_key_on(int channel);
void aica_apu_key_off(int channel);
void aica_apu_stop(int channel);
/* where channel's voice plays, in samples from its sample's start: -1 while
 * it hasn't started, -2 where no voice tells (the channel's model then goes
 * by its own steps) */
int aica_apu_position(int channel);
/* every channel's model brought up to now (sgc_if.cpp), which ends those
 * that have run out: aica_apu_stop */
void aica_channels_sync(void);
/* the model's counts, for the profile: the samples stepped one at a time,
 * the runs passed at once, the samples in them */
void aica_quiet_counts(unsigned out[3]);

#ifdef __cplusplus
}
#endif

#endif

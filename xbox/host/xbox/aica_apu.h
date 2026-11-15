/* the Xbox host's AICA-to-APU bridge, aica_apu.c */
#ifndef XBOX_HOST_XBOX_AICA_APU_H
#define XBOX_HOST_XBOX_AICA_APU_H

#ifdef __cplusplus
extern "C" {
#endif

int aica_apu_open(const void *sound_ram);
void aica_apu_blank(void);
void aica_apu_report(void);
void aica_apu_report_memory(void);
void aica_apu_silence(void);
/* the game opens the bank file name, in capitals without its folder: its
 * samples from the sound pack, where there is one. from a host call, where
 * a file may be read */
void aica_apu_preload(const char *name);

#ifdef __cplusplus
}
#endif

#endif

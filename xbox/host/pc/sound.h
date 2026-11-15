/* the PC host's sound, sound.cpp: the AICA over the sound RAM at ram, 2 MB.
 * sound_blank, from the blank's signal, runs the blank's samples */
#ifndef XBOX_HOST_PC_SOUND_H
#define XBOX_HOST_PC_SOUND_H

#ifdef __cplusplus
extern "C" {
#endif

int sound_open(void *ram);
void sound_blank(void);
/* from the blank's signal: the sound's thread ends the WAV file and the
 * program, before it runs another blank's samples */
void sound_quit(void);
unsigned sound_reg_read(unsigned offset, int size);
void sound_reg_write(unsigned offset, unsigned value, int size);

#ifdef __cplusplus
}
#endif

#endif

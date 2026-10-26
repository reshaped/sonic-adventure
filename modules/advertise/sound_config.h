/* the console's sound mode, stereo or mono, which the options screen reads and
 * sets */
#ifndef ADVERTISE_SOUND_CONFIG_H
#define ADVERTISE_SOUND_CONFIG_H

#include <sg_xpt.h>

/* sound_config_open starts syCfg, the console's settings, in a work area of
 * 0x4000 bytes from the heap. it calls syCfgInit again after each failure, up
 * to the count CFG_TRIES gives. sound_config_close ends syCfg if it started,
 * and frees the work area. it then sets the sound driver to the console's mode,
 * as sound_set_space does */
void sound_config_open(void);
void sound_config_close(void);
/* sound_config_set sets the console's sound mode to mode, SYD_CFG_STEREO or
 * SYD_CFG_MONO, retrying up to the count CFG_TRIES gives. it returns 0, whether
 * or not the mode took. sound_config_get returns the mode, or SYD_CFG_STEREO
 * when it can't read it. both go through syCfg, which sound_config_open
 * starts */
int sound_config_set(Sint32 mode);
Sint32 sound_config_get(void);

#endif

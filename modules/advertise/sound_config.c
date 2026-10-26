/* the console's sound mode, stereo or mono, which the options screen reads and
 * sets */
#include <sg_xpt.h>
#include <shinobi.h>

#include "advertise/sound_config.h"
#include "sound/sound.h"
#include "system/task.h"

#define CONFIG_WORK_SIZE 0x4000 /* the bytes of syCfg's work area */

static struct {
    Bool open; /* TRUE once syCfgInit has succeeded */
    void *work; /* syCfg's work area, or NULL */
} sound_config;
/* the failures of the syCfg call last retried, which each retry loop counts
 * from 0 */
static int sound_config_tries;

void sound_config_open(void)
{
    sound_config.open = FALSE;
    /* @hack spelling: the work area assigned in the test. SHC then tests it in
     * R0 (TST R0,R0 at 0x8c900d5a). assigned first, it is copied to R2 and
     * tested there, the function runs 2 bytes longer, and 31 of 44 words
     * differ */
    if ((sound_config.work = heap_calloc(1, CONFIG_WORK_SIZE)) != NULL) {
        sound_config_tries = 0;
        sound_config.open = TRUE;
        while (syCfgInit(sound_config.work) != SYD_CFG_OK) {
            if (sound_config_tries++ > CFG_TRIES) {
                sound_config.open = FALSE;
                break;
            }
        }
    }
}

void sound_config_close(void)
{
    if (sound_config.open) {
        syCfgExit();
    }
    if (sound_config.work != NULL) {
        heap_free(sound_config.work);
        sound_config.work = NULL;
    }
    sound_set_space();
}

int sound_config_set(Sint32 mode)
{
    sound_config_tries = 0;
    while (syCfgSetSoundMode(mode) != SYD_CFG_OK) {
        if (sound_config_tries++ > CFG_TRIES) {
            break;
        }
    }
    return 0;
}

Sint32 sound_config_get(void)
{
    Sint32 mode;

    sound_config_tries = 0;
    while (syCfgGetSoundMode(&mode) != SYD_CFG_OK) {
        if (sound_config_tries++ > CFG_TRIES) {
            mode = SYD_CFG_STEREO;
            break;
        }
    }
    return mode;
}

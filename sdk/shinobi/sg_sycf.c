/* console sound setting, stereo or mono */
#include <sg_xpt.h>
#include <shinobi.h>

/* the sound mode's byte, SM, in the copy of the console settings that
 * syBtCfgSet reads and writes in low memory */
#define CFG_SOUND_MODE (*(char *)0x8c00007e)

void *sygCfgBufAddr = NULL;
char *sygCfgBuild = "\nsyCfg Ver 1.00 Build:Sep 28 1998 14:59:49\n";

Sint32 syCfgInit(void *work)
{
    if (work == NULL) {
        return SYD_CFG_NG;
    }
    sygCfgBufAddr = work;
    return SYD_CFG_OK;
}

Sint32 syCfgExit(void)
{
    sygCfgBufAddr = NULL;
    return SYD_CFG_OK;
}

Sint32 syCfgSetSoundMode(Sint32 mode)
{
    Sint32 result;

    if (sygCfgBufAddr == NULL) {
        return SYD_CFG_NG;
    }
    if (mode != SYD_CFG_STEREO && mode != SYD_CFG_MONO) {
        return SYD_CFG_NG;
    }
    CFG_SOUND_MODE = mode;
    result = syBtCfgSet(SYD_BT_CFGWRT_SM, sygCfgBufAddr);
    if (result == SYD_BT_CFG_ERR) {
        return SYD_CFG_NG;
    }
    return SYD_CFG_OK;
}

Sint32 syCfgGetSoundMode(Sint32 *mode)
{
    Sint32 result;
    char setting;

    if (mode == NULL) {
        return SYD_CFG_NG;
    }
    /* a failure past the NULL test leaves *mode SYD_CFG_STEREO */
    *mode = SYD_CFG_STEREO;
    if (sygCfgBufAddr == NULL) {
        return SYD_CFG_NG;
    }
    result = syBtCfgSet(SYD_BT_CFGINI, sygCfgBufAddr);
    if (result == SYD_BT_CFG_ERR) {
        return SYD_CFG_NG;
    }
    setting = CFG_SOUND_MODE;
    if (setting == SYD_CFG_STEREO) {
        *mode = SYD_CFG_STEREO;
    } else if (setting == SYD_CFG_MONO) {
        *mode = SYD_CFG_MONO;
    } else {
        return SYD_CFG_NG;
    }
    return SYD_CFG_OK;
}

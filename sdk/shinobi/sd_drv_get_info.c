/* the sound driver's version and table offsets, read into the work */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

/* the source file name the library prints */
#define SOURCE "sd_drv_get_info_.c"

/* offset of the driver's information from the start of the driver */
#define INFO_OFFSET 0x20

/* the driver's information, in sound memory. driver 1.0 keeps its debug
 * version one byte later than other versions */
struct drv_info {
    Sint8 major_ver;
    Sint8 minor_ver;
    Sint8 debug_ver;
    Sint8 debug_ver_1_0;
    Uint8 unk_04[0x9c];
    Sint32 tool_interface;
    Sint32 sound_status;
    Sint32 status;
    Sint32 data_map;
    Sint32 host_command;
    Sint32 command_history;
};

/* the sound memory offset of field, in the driver's information at info */
#define INFO(info, field) ((Sint32)(&((struct drv_info *)(info))->field))

SDE_ERR sdDrvGetInfo(void)
{
    SDE_ERR err = SDE_ERR_NOTHING;
    Sint32 info;

    sdDebugPrintf("Enter:%s\n", SOURCE);
    if (sdgWrkPtr == NULL) {
        err = SDE_ERR_NO_INIT;
        goto error;
    }
    info = sdgWrkPtr->driver + INFO_OFFSET;
    sdgWrkPtr->system_info = info;
    sdSndMemReadSint8(INFO(info, major_ver), &sdgWrkPtr->drv_ver.m_MajorVer);
    sdSndMemReadSint8(INFO(info, minor_ver), &sdgWrkPtr->drv_ver.m_MinorVer);
    if (sdgWrkPtr->drv_ver.m_MajorVer == 1 &&
        sdgWrkPtr->drv_ver.m_MinorVer == 0) {
        sdSndMemReadSint8(INFO(info, debug_ver_1_0),
                          &sdgWrkPtr->drv_ver.m_DebugVer);
    } else {
        sdSndMemReadSint8(INFO(info, debug_ver),
                          &sdgWrkPtr->drv_ver.m_DebugVer);
    }
    sdgWrkPtr->drv_ver.rsv = 0;
    sdSndMemReadSint32(INFO(info, tool_interface), &sdgWrkPtr->tool_interface);
    sdSndMemReadSint32(INFO(info, sound_status), &sdgWrkPtr->sound_status);
    sdSndMemReadSint32(INFO(info, status), &sdgWrkPtr->status);
    sdSndMemReadSint32(INFO(info, data_map), &sdgWrkPtr->data_map);
    sdSndMemReadSint32(INFO(info, host_command), &sdgWrkPtr->host_command);
    sdSndMemReadSint32(INFO(info, command_history),
                       &sdgWrkPtr->command_history);
    sdDebugPrintf("    System Information     : %08X\n",
                  sdgWrkPtr->system_info);
    sdDebugPrintf("    System Tool Interface  : %08X\n",
                  sdgWrkPtr->tool_interface);
    sdDebugPrintf("    System Sound Status    : %08X\n",
                  sdgWrkPtr->sound_status);
    sdDebugPrintf("    System Status          : %08X\n", sdgWrkPtr->status);
    sdDebugPrintf("    System Data Map        : %08X\n", sdgWrkPtr->data_map);
    sdDebugPrintf("    System Host Command    : %08X\n",
                  sdgWrkPtr->host_command);
    sdDebugPrintf("    System Command History : %08X\n",
                  sdgWrkPtr->command_history);

error:
    sdDebugPrintf("Exit :%s \n", SOURCE);
    return err;
}

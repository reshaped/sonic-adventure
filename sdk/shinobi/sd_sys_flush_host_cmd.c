#include <shinobi.h>

SDE_ERR sdSysFlushHostCmd(void)
{
    sdDebugPrintf("Enter:%s\n", "sd_sys_flush_host_cmd_.c");
    sd2ndHostCmdSendList();
    sdDebugPrintf("Exit :%s(%08)\n", "sd_sys_flush_host_cmd_.c",
                  SDE_ERR_NOTHING);
    return SDE_ERR_NOTHING;
}

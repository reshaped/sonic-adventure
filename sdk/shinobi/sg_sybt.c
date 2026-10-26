/* boot ROM menu exit and disc check */
#include <sg_xpt.h>
#include <shinobi.h>

/* the boot ROM's system call vector, in low memory, and its function
 * numbers */
#define BOOT_SYSTEM (*(Sint32(**)(Sint32 func))0x8c0000e0)
#define BOOT_EXIT 1
#define BOOT_CHECK_DISC 2

const char *const sygBtBuild = "\nsyBt Ver 1.01 Build:Oct 09 1998 14:31:23\n";

void syBtExit(void)
{
    BOOT_SYSTEM(BOOT_EXIT);
}

Sint32 syBtCheckDisc(void)
{
    syCachePurgeAll();
    return BOOT_SYSTEM(BOOT_CHECK_DISC);
}

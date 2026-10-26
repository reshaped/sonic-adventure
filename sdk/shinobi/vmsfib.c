/* VMS file system fibers: one for each drive, run by one fiber server */
#include <rfiber.h>
#include <sg_xpt.h>
#include <vmsfs.h>

static struct rf_server *vmsfs_server;

struct rf_server *vmsfb_init(void)
{
    return vmsfs_server = rfCreateServer(VMSFS_DRIVES);
}

Sint32 vmsfb_create(void (*func)(void), void *stack, Sint32 size)
{
    return rfCreateFiber(vmsfs_server, func, stack, size);
}

void vmsfb_exec(void)
{
    rfExecFiberServer(vmsfs_server);
}

void vmsfb_exit(void)
{
    rfDestroyServer(vmsfs_server);
}

void vmsfb_sleep(void)
{
    rfSleepVFiber();
}

void vmsfb_end(void)
{
    rfEndFiber();
}

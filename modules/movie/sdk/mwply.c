/* the middleware player's system calls, each passed straight on to the
 * manager */
#include <sg_mwmng.h>
#include <sg_mwply.h>

char *mwg_ply_build = "\nmwPly Ver.1.04 Build:Nov 11 1998 21:18:40\n";

void mwPlyStartFrame(void)
{
    mwMngStartFrame();
}

void mwPlyEntryErrFunc(MW_PLY_ERRFN errfn, void *obj)
{
    mwMngEntryErrFn(errfn, obj);
}

void mwPlyExecServer(void)
{
    mwMngExecServer();
}

void mwPlyInitSystem(void)
{
    mwMngInit();
}

void mwPlyExitSystem(void)
{
    mwMngFinish();
}

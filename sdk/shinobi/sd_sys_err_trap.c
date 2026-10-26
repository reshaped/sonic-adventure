#include <shinobi.h>

void *sdgErrApi = NULL;
SDE_ERR sdgErrResult = SDE_ERR_NOTHING;

void sdSysErrTrap(void *func, SDE_ERR err)
{
    sdgErrApi = func;
    sdgErrResult = err;
}

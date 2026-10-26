#include <sg_syg2.h>
#include <sg_xpt.h>

struct g2_work *sygG2WrkPtr = NULL;
const char *const sygG2BuildTime =
    "\nsyG2 Ver 1.01.00 Build:Oct 12 1998 05:48:41\n";
Uint8 sygG2WrkImg[sizeof(struct g2_work) + 32];
Uint32 sygG2DmaSuspendAll;

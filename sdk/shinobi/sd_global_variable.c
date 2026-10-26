/* sound library globals and version banner */
#include "shinobi/sd_work.h"
#include <sg_xpt.h>
#include <shinobi.h>

struct sd_work *sdgWrkPtr = NULL;
Sint32 sdgExistSndDrv = false;
Bool sdgDebugMode = false;
void (*sdgPrintFunc)(const char *str) = NULL;
const char *const sdgBuild = "\nsd Ver 1.00.11 Build:Nov 08 1998 02:28:01\n";

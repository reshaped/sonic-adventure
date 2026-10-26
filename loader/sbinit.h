/* LOADER.BIN's start and end of the system */
#ifndef LOADER_SBINIT_H
#define LOADER_SBINIT_H
#include <sg_xpt.h>

/* start the hardware, the heap, Ninja, the pads, the clock and the GD file
 * system. mode, frame and count are njInitSystem's. where the file system
 * can't start on the disc, end the system and exit to the boot ROM */
void sbInitSystem(Int mode, Int frame, Int count);
/* end what sbInitSystem started, in reverse order, and mask every
 * interrupt */
void sbExitSystem(void);

#endif

/* hardware, memory and SDK subsystem initialization */
#ifndef SYSTEM_SBINIT_H
#define SYSTEM_SBINIT_H
#include <sg_xpt.h>

/* start the hardware, the heap and Ninja, which takes mode, frame and
 * count. then start the pads, the clock and the GD file system, and enter
 * the disc's SONICADV directory. an open tray, a changed disc or eight
 * failed starts of the file system exit to the boot ROM */
void sbInitSystem(Int mode, Int frame, Int count);
/* stop what sbInitSystem started, last first, and mask every interrupt */
void sbExitSystem(void);

#endif

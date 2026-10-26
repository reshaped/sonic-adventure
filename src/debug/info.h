/* the debug info menu: one item, which turns the debug info on and off */
#ifndef DEBUG_INFO_H
#define DEBUG_INFO_H
#include <sg_xpt.h>

/* TRUE while the debug info is on, as it is from the start. nothing calls
 * this, and nothing creates the info menu, whose item turns it off and on */
Bool debug_info_on(void);

#endif

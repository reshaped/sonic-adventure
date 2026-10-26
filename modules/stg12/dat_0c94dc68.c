/* the action that the last moving part of act 0's land plays, and the action's
 * motion. the part is the struct land_motion at 0x0c91a594 */
#include <ninja.h>

#include "stg12/dat_0c94dc68.h"

extern NJS_OBJECT dat_0c94da44;
extern NJS_MDATA2 dat_0c94dc48[];

static NJS_MOTION dat_0c94dc68 = {
#include "stg12/dat_0c94dc68/dat_0c94dc68.inc"
};
NJS_ACTION dat_0c94dc74 = {
#include "stg12/dat_0c94dc68/dat_0c94dc74.inc"
};

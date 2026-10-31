/* the position keys that move sonic_object, the root node of Sonic's model,
 * in the action at 0x0cb8d63c. no word in EV004D or 1ST_READ.BIN points at
 * that action */
#include <ninja.h>

#include "ev004d/dat_0cb8ba10.h"

NJS_MKEY_F dat_0cb8ba10[15] = {
#include "ev004d/dat_0cb8ba10/dat_0cb8ba10.inc"
};

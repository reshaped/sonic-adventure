/* the PSP slice's prelude, which the slice includes ahead of each of the
 * game's sources: the native slice's own, then the PSP's place for the
 * console's RAM. the PSP's pointers hold 4 bytes, as the SH-4's do, so the
 * game's records lie as the console lays them, and nothing is marked */
#ifndef PSP_PRELUDE_H
#define PSP_PRELUDE_H

#include "../native/prelude.h"
#include "psp.h"

#endif

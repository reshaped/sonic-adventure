/* the port's stand-in for Flycast's gdrom_if.h: CD audio, which the game
 * doesn't play, comes in silent */
#pragma once
#include "types.h"
static inline void libCore_CDDA_Sector(s16 *sector) { std::memset(sector, 0, 2352); }

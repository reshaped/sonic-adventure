/* the Dreamcast backend's part of the engine's C ABI, beside sa_engine.h's
 * core. the backend reads a draw's borrowed bytes during the call alone, as
 * SA_BORROW_CALL declares. it has written what it draws of them before the
 * call returns: the opaque list to the tile accelerator, and the other lists
 * to its own buffers, which it sends as the picture ends. so it takes a
 * game's own geometry by sa_present_live. the backend's headers define that
 * inline, each for some of a game's formats, as sa1/live.h does. the backend
 * keeps its state in the machine, and make dc links no runtime core. so its
 * calls read no engine, and a client passes SA_DC_ENGINE, a null engine */
#ifndef SA_DC_H
#define SA_DC_H

#include "sa_engine.h"

#define SA_BORROW_CALL
#define SA_DC_ENGINE ((sa_engine *)0)

#endif

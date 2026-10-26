/* the texture list of the garden's objects */
#ifndef AL_GARDEN00_TEXTURES_H
#define AL_GARDEN00_TEXTURES_H

#include <ninja.h>

/* the list the elevator, the fountain and the door draw with.
 * 1ST_READ.BIN's alife_ss_pvms loads GARDEN00_OBJECT into it, reaching it by
 * its address, 0x0cb904d8 */
extern NJS_TEXLIST garden_objects_texlist;

#endif

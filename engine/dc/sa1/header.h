/* Ninja's polygon header, which SA1's model draws on the Dreamcast write,
 * and the words beside it. src/draw/model.c lends them by name to live.h's
 * walk, and sdk/ninja/njModelE.c to easy.h's draw. each name is Ninja's own
 * symbol where a layer declares it. so a draw reads each word at its address
 * and leaves it there for the next draw */
#ifndef SA1_HEADER_H
#define SA1_HEADER_H

#include <kamui.h>
#include <ninja.h>

#include "sdk/ninja/njkamui.h"

/* Ninja's 3D control flags and constant attributes */
extern Uint32 sa1_live_control;
extern Uint32 sa1_live_attr_and;
extern Uint32 sa1_live_attr_or;
/* the polygon header with the lists' write pointers, the current texture's
 * number and the texture list */
extern struct nj_parameterkm sa1_live_header;
extern Uint32 sa1_live_texture;
extern NJS_TEXLIST *sa1_live_texlist;
/* the list the last meshset opened, and its write pointer's top bits, for
 * njEndVertex */
extern Uint32 sa1_live_sq_base;
extern Uint32 **sa1_live_list;

#endif

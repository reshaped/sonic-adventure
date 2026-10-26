/* the textures and texlists of the CHAOS4_COMMON, CHAOS4_KAMA (sickle) and
 * CHAOS4_OBJECT pvms, and the first strips of a model after them */
#ifndef B_CHAOS4_TEXTURES_H
#define B_CHAOS4_TEXTURES_H

#include <ninja.h>
#include <sg_xpt.h>

extern NJS_TEXLIST chaos4_common_texlist;
extern NJS_TEXLIST chaos4_kama_texlist;
/* the CHAOS4_OBJECT pvm's texlist, which the water, the dome, the pads
 * and their stalks draw with */
extern NJS_TEXLIST chaos4_object_texlist;
/* the first strips of a model that runs on past this file. its meshsets,
 * in the generated data that follows, name them */
extern Sint16 dat_0c90888c[];
extern Sint16 dat_0c9088ac[];
extern Sint16 dat_0c9088cc[];
extern Sint16 dat_0c9088e4[];
extern Sint16 dat_0c9088fc[];
extern Sint16 dat_0c908914[];
extern Sint16 dat_0c90892c[];

#endif

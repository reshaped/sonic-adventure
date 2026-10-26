/* the race's object texlist, and the model data that follows it */
#ifndef AL_RACE_TEXLISTS_H
#define AL_RACE_TEXLISTS_H
#include <ninja.h>
#include <sg_xpt.h>

/* the texlist of the textures race_pvms loads from OBJ_AL_RACE. it lies in
 * AL_RACE, so it is valid only while AL_RACE is loaded */
extern NJS_TEXLIST dat_0cbc089c;
/* a model's two materials, and the strips of its two meshsets. the model
 * itself lies in the generated data that follows texlists.c's data */
extern NJS_MATERIAL dat_0cbc08a4[];
extern Sint16 dat_0cbc08cc[];
extern Sint16 dat_0cbc0924[];

#endif

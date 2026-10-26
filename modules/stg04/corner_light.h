/* O CLIGHT solid lamp with a pulsing glow that faces the camera */
#ifndef STG04_CORNER_LIGHT_H
#define STG04_CORNER_LIGHT_H
#include <ninja.h>

#include "system/task.h"

/* the glow's texture list, of st_cornerlight alone */
extern NJS_TEXLIST corner_light_texlist;
/* the glow's one frame */
extern NJS_TEXANIM corner_light_texanim[1];
/* the glow, which O Antena draws too */
extern NJS_SPRITE corner_light_sprite;

/* task step for O CLIGHT, named in the object list */
void corner_light_step(struct task *task);

#endif

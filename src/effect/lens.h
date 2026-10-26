/* lens flares: a light's glare, and the ghosts strung from the light through
 * the middle of the screen */
#ifndef EFFECT_LENS_H
#define EFFECT_LENS_H
#include <ninja.h>

/* lens01 to lens07, and panlight */
extern NJS_TEXLIST lens_texlist;

/* draw the flare of the light at pos on the screen while the light lies less
 * than 90 degrees off the camera's view. the light itself draws as lens01.
 * the glare draws at the light's place while the light lies within about 8
 * degrees of the view. it grows to a scale of 20 and an alpha of 0.9 head
 * on. the ghosts, strung from the light through the screen's middle, show
 * while the light lies within about 53 degrees of the view. they brighten
 * as the light nears the view. the flare draws whatever frame_step is. the
 * call leaves fog on, the blend modes at SRCALPHA and INVSRCALPHA, and the
 * constant material zeroed */
void lens_draw(NJS_POINT3 *pos);
/* a task at level 4 that draws the flare of the light at pos, which it
 * copies. after a first step that sets it up, it calls lens_draw on every
 * step, not only a frame's first */
void lens_create(NJS_POINT3 *pos);
/* draw the glare and ghosts of the light at pos, which shines along the
 * unit vector dir. they draw as lens_draw draws them, but the light itself
 * doesn't. lens_draw takes the cosine of the light's angle off the view.
 * this takes that cosine times the cosine of dir's angle off the line from
 * the light to the camera. so a light behind the camera that shines away
 * from it flares too. it leaves the blend modes and material as lens_draw
 * does, and fog as it was */
void lens_draw_facing(NJS_POINT3 *pos, NJS_VECTOR *dir);

#endif

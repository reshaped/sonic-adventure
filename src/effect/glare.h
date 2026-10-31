/* glare: the glow of a light that shines toward the camera */
#ifndef EFFECT_GLARE_H
#define EFFECT_GLARE_H
#include <ninja.h>

/* the glow's texlist, of one texture, EFF_REGULAR's panlight. STG05's lamps
 * draw their own glow sprite from it */
extern NJS_TEXLIST glare_texlist;

/* draw the glow of the light at pos, which shines along the unit vector
 * dir, in color. let back be the level direction out of the camera's back,
 * and d dir's part along back. the glow's alpha is 0 unless the light lies
 * in front of the camera and d is 0 or more. it is then
 * color->a * (d * d / (1.4 - d * d) - 0.4). only the camera's heading, not
 * its pitch, decides what lies in front. the alpha is 2.1 * color->a with
 * dir along back, and below 0 past about 51 degrees off it. the glow draws
 * once, at a scale of 1.5 * size * alpha. it stands size * (that scale + 10)
 * from pos toward the camera. the call leaves fog on, the blend modes at
 * SRCALPHA and INVSRCALPHA, and the constant material zeroed */
void glare_draw(NJS_POINT3 *pos, NJS_VECTOR *dir, Float size, NJS_ARGB *color);

#endif

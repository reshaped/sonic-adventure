/* quads: a part of a texture drawn on the screen, turned, flipped and
 * colored */
#ifndef DRAW_QUAD_H
#define DRAW_QUAD_H
#include <ninja.h>
#include <sg_xpt.h>

struct quad {
    NJS_TEXLIST *texlist;
    int texture; /* an index in texlist */
    int flags; /* NJD_SPRITE_ANGLE, _COLOR, _HFLIP, _VFLIP and _ALPHA */
    /* the part's top left and bottom right corners in the texture, each
     * clamped to 0 to 1 */
    NJS_POINT2 uv[2];
    /* the point it turns round, which pos places, as a share of size from
     * its top left */
    Float center[2];
    Float size[2];
    Float pos[2]; /* of the center, on the screen */
    Float depth; /* clamped between -1, the nearest, and -65535 */
    Angle ang;
    /* with NJD_SPRITE_COLOR, the colors of the top left, bottom left, top
     * right and bottom right corners. without it, every corner is white */
    Uint32 colors[4];
};

/* draw the quad, unless it's null. its texture must be loaded */
void quad_draw(struct quad *quad);

#endif

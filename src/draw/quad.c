/* quads: a part of a texture drawn on the screen, turned, flipped and
 * colored */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/quad.h"

static void quad_set_depth(NJS_TEXTURE_VTX *corners, Float depth);
static void quad_set_colors(NJS_TEXTURE_VTX *corners, Uint32 *colors,
                            int flags);
static void quad_set_uv(NJS_TEXTURE_VTX *corners, NJS_POINT2 *uv, int flags);
static void quad_set_corners(NJS_TEXTURE_VTX *corners, struct quad *quad);

/* the corners: top left, bottom left, top right and bottom right */
static NJS_TEXTURE_VTX quad_corners[4];
/* the corners' offsets from the center */
static NJS_POINT2 quad_offsets[4];

void quad_draw(struct quad *quad)
{
    int flags;
    Int texture;
    Bool alpha;

    if (quad != NULL) {
        flags = quad->flags;
        quad_set_depth(quad_corners, quad->depth);
        quad_set_colors(quad_corners, quad->colors, flags);
        quad_set_uv(quad_corners, quad->uv, flags);
        quad_set_corners(quad_corners, quad);
        texture =
            ((NJS_TEXMEMLIST *)quad->texlist->textures[quad->texture].texaddr)
                ->globalIndex;
        alpha = (flags & NJD_SPRITE_ALPHA) ? TRUE : FALSE;
        njDrawTexture(quad_corners, 4, texture, alpha);
    }
}

static void quad_set_depth(NJS_TEXTURE_VTX *corners, Float depth)
{
    NJS_TEXTURE_VTX *corner;
    int i;

    if (depth < -65535.0F) {
        depth = -65535.0F;
    } else if (depth > -1.0F) {
        depth = -1.0F;
    }
    depth = -1.0F / depth;
    for (i = 0, corner = corners; i < 4; i++, corner++) {
        corner->z = depth;
    }
}

static void quad_set_colors(NJS_TEXTURE_VTX *corners, Uint32 *colors, int flags)
{
    int i = 0;
    Uint32 *color;

    if (flags & NJD_SPRITE_COLOR) {
        for (color = colors; i < 4; i++, color++, corners++) {
            corners->col = *color;
        }
    } else {
        for (; i < 4; i++, corners++) {
            corners->col = 0xffffffff;
        }
    }
}

static void quad_set_uv(NJS_TEXTURE_VTX *corners, NJS_POINT2 *uv, int flags)
{
    Float u1 = uv[0].x;
    Float v1 = uv[0].y;
    Float u2 = uv[1].x;
    Float v2 = uv[1].y;
    Float swap;

    if (u1 < 0.0F) {
        u1 = 0.0F;
    } else if (u1 > 1.0F) {
        u1 = 1.0F;
    }
    if (v1 < 0.0F) {
        v1 = 0.0F;
    } else if (v1 > 1.0F) {
        v1 = 1.0F;
    }
    if (u2 < 0.0F) {
        u2 = 0.0F;
    } else if (u2 > 1.0F) {
        u2 = 1.0F;
    }
    if (v2 < 0.0F) {
        v2 = 0.0F;
    } else if (v2 > 1.0F) {
        v2 = 1.0F;
    }
    if (flags & NJD_SPRITE_HFLIP) {
        swap = u1;
        u1 = u2;
        u2 = swap;
    }
    if (flags & NJD_SPRITE_VFLIP) {
        swap = v1;
        v1 = v2;
        v2 = swap;
    }
    corners[0].u = u1;
    corners[0].v = v1;
    corners[1].u = u1;
    corners[1].v = v2;
    corners[2].u = u2;
    corners[2].v = v1;
    corners[3].u = u2;
    corners[3].v = v2;
}

static void quad_set_corners(NJS_TEXTURE_VTX *corners, struct quad *quad)
{
    Float width = quad->size[0];
    Float height = quad->size[1];
    Float cx = quad->center[0];
    Float cy = quad->center[1];
    Float left = -(width * cx);
    Float right = (1.0F - cx) * width;
    Float top = -(height * cy);
    Float bottom = (1.0F - cy) * height;
    Float sine;
    Float cosine;
    Float x;
    Float y;
    NJS_POINT2 *offset;
    NJS_TEXTURE_VTX *corner;
    int i;
    Angle ang;

    quad_offsets[0].x = left;
    quad_offsets[0].y = top;
    quad_offsets[1].x = left;
    quad_offsets[1].y = bottom;
    quad_offsets[2].x = right;
    quad_offsets[2].y = top;
    quad_offsets[3].x = right;
    quad_offsets[3].y = bottom;
    if (quad->flags & NJD_SPRITE_ANGLE) {
        ang = quad->ang;
        sine = njSin(ang);
        cosine = njCos(ang);
        for (i = 0, offset = quad_offsets; i < 4; i++, offset++) {
            x = offset->x;
            y = offset->y;
            offset->x = (cosine * x) - (sine * y);
            offset->y = (sine * x) + (cosine * y);
        }
    }
    x = quad->pos[0];
    y = quad->pos[1];
    /* @hack redundant: offset set to quad_offsets, which the loop indexes
     * instead. the store starts the loop's pointer with MOV R13,R4 at
     * 0x8c0a7aaa; without it, SHC adds R13 to a 0 there and 33 of 136 words
     * differ */
    for (i = 0, offset = quad_offsets, corner = corners; i < 4; i++) {
        corner->x = x + quad_offsets[i].x;
        corner->y = y + quad_offsets[i].y;
        corner++;
    }
}

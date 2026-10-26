/* the blast dome: a ball of bands that can scroll its texture, thin its
 * bands and fade. the saru's bomb, the gacha robot and the events' dome draw
 * it */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/model.h"
#include "effect/blast_dome.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */

/* the ball's bands, from the top down, and the sides round each, with the
 * angle each spans */
#define BANDS 8
#define SIDES 8
#define BAND_ANGLE 0x1000
#define SIDE_ANGLE 0x2000

/* the points of a band's strip: a top and a bottom one for each side, and
 * the first two again to close it */
#define STRIP_LENGTH ((SIDES + 1) * 2)

/* the cos and sin of each side's angle */
static Float dome_sides[SIDES * 2];
/* each band's points, a top and a bottom one for each side */
/* @hack type: the points walked a float at a time through coord, which
 * stores each through the walk and steps it (FMOV.S FR3,@R14 and ADD #4,R14
 * at 0x8c0a4e48, 0x8c0a4e4a in blast_dome_blend); through NJS_POINT3, SHC
 * stores y and z at @(R0,Rn), blast_dome_init runs 96 bytes long and 140 of
 * the 274 words of blast_dome_init and blast_dome_blend differ */
static union {
    NJS_POINT3 point[BANDS * SIDES * 2];
    Float coord[BANDS * SIDES * 2 * 3]; /* x, y and z of each in turn */
} dome_points;
/* the points' normals, which stay zero */
static NJS_VECTOR dome_normals[BANDS * SIDES * 2];
/* each band's strip, its length and then its points */
static Sint16 dome_strips[BANDS * (1 + STRIP_LENGTH)];
/* where each point of a strip sits in the texture */
/* @hack type: the scroll walks a Sint16 at a time through coord, which loads
 * the walk's start ahead of its first push (0x8c0a4d5a); through NJS_TEX, SHC
 * stores v at @(2,Rn), the function runs 8 bytes short and 49 of 51 words
 * differ */
static union {
    NJS_COLOR uv[BANDS * STRIP_LENGTH];
    Sint16 coord[BANDS * STRIP_LENGTH * 2]; /* u and v of each in turn */
} dome_uvs;

/* EFF_REGULAR's p_exp */
NJS_TEXLIST blast_dome_texlist = {
#include "effect/blast_dome/blast_dome_texlist.inc"
};

static NJS_MATERIAL dome_material = {
#include "effect/blast_dome/dome_material.inc"
};
static NJS_MESHSET dome_meshset = {
#include "effect/blast_dome/dome_meshset.inc"
};
static NJS_MODEL dome_model = {
#include "effect/blast_dome/dome_model.inc"
};

static Bool dome_made = FALSE;

void blast_dome_init(void)
{
    Float *side;
    Float *point;
    Sint16 *strip;
    Angle ang;
    Angle around;
    int band;
    int i;
    int first;
    int next;

    if (dome_made) {
        return;
    }
    dome_made = TRUE;
    side = dome_sides;
    for (ang = 0; ang < SIDES * SIDE_ANGLE; ang += SIDE_ANGLE) {
        *side++ = njCos(ang);
        *side++ = njSin(ang);
    }
    point = dome_points.coord;
    for (ang = 0; ang < BANDS * BAND_ANGLE; ang += BAND_ANGLE) {
        Float top_sin;
        Float top_cos;
        Float bottom_sin;
        Float bottom_cos;

        side = dome_sides;
        top_sin = njSin(ang);
        top_cos = njCos(ang);
        bottom_sin = njSin(ang + BAND_ANGLE);
        bottom_cos = njCos(ang + BAND_ANGLE);
        for (around = 0; around < SIDES * SIDE_ANGLE; around += SIDE_ANGLE) {
            Float x = *side++;
            Float z = *side++;

            *point++ = top_sin * x;
            *point++ = top_cos;
            *point++ = top_sin * z;
            *point++ = bottom_sin * x;
            *point++ = bottom_cos;
            *point++ = bottom_sin * z;
        }
    }
    strip = dome_strips;
    next = 0;
    for (band = 0; band < BANDS; band++) {
        first = next;
        *strip++ = STRIP_LENGTH;
        for (i = 0; i < SIDES; i++) {
            *strip++ = next++;
            *strip++ = next++;
        }
        *strip++ = first++;
        *strip++ = first;
    }
    blast_dome_scroll(0);
}

void blast_dome_scroll(int scroll)
{
    Sint16 *uv = dome_uvs.coord;
    int band;
    int i;
    int left;
    int top = scroll;

    for (band = 0; band < BANDS; band++) {
        left = 0;
        for (i = 0; i < SIDES + 1; i++) {
            *uv++ = left;
            *uv++ = top;
            *uv++ = left;
            *uv++ = top + 63;
            left += 64;
            top += 64;
        }
    }
}

void blast_dome_blend(Float blend)
{
    Float *side;
    Float *point;
    Angle ang;
    Angle around;

    point = dome_points.coord;
    for (ang = 0; ang < BANDS * BAND_ANGLE; ang += BAND_ANGLE) {
        Float top_sin;
        Float top_cos;
        Float bottom_sin;
        Float bottom_cos;

        side = dome_sides;
        top_sin = njSin(ang);
        top_cos = njCos(ang);
        bottom_sin = njSin(ang + BAND_ANGLE);
        bottom_cos = njCos(ang + BAND_ANGLE);
        top_sin = bottom_sin + (blend * (top_sin - bottom_sin));
        top_cos = bottom_cos + (blend * (top_cos - bottom_cos));
        for (around = 0; around < SIDES * SIDE_ANGLE; around += SIDE_ANGLE) {
            Float x = *side++;
            Float z = *side++;

            *point++ = top_sin * x;
            *point++ = top_cos;
            *point++ = top_sin * z;
            point += 3;
        }
    }
}

void blast_dome_set_alpha(Float alpha)
{
    if (alpha < 0.0F) {
        alpha = 0.0F;
    } else if (alpha > 1.0F) {
        alpha = 1.0F;
    }
    dome_model.mats->diffuse.argb.a = alpha * 255.0F;
}

void blast_dome_draw(void)
{
    draw_model(&dome_model);
}

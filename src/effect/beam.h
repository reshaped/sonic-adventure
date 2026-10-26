/* beams: rays of light that swing about a point, and the draws of a ray or
 * a line as a quad on the screen */
#ifndef EFFECT_BEAM_H
#define EFFECT_BEAM_H
#include <ninja.h>
#include <sg_xpt.h>

struct angles;

struct beam_params {
    Sint32 time; /* steps before it fades */
    Float width[2]; /* at its foot, then its tip */
    Uint32 color[2]; /* at its foot, then its tip */
    Angle max_speed; /* the most it swings a step, about each axis */
    Float length;
    /* what its length is multiplied by each step, from a tenth until full */
    Float growth;
};

struct beam_texture {
    Uint32 texture; /* Ninja's global texture index */
    Uint32 color[2]; /* at its start, then its end */
    /* the quad's corners: two at its start, then two at its end */
    NJS_POINT2 uv[4];
};

/* a beam from pos, swinging at random. params are copied, but pos is kept
 * and must outlive the beam. when no task can be made, it stores through a
 * null task */
void beam_create(NJS_POINT3 *pos, struct beam_params *params);
/* draw a quad on the screen along a ray or a line. the ray runs from pos to
 * a tip length along -z, turned about x by ang->x and then about y by
 * ang->y. the line runs from from to to. the quad is width[0] wide at its
 * start and width[1] at its end. its colors are color[0] and color[1], or
 * texture's for the _textured draws. each draws only on a frame's first
 * step, and nothing when both ends lie behind the near clip */
void beam_draw_ray(NJS_POINT3 *pos, struct angles *ang, Float *width,
                   Uint32 *color, Float length);
void beam_draw_line(NJS_POINT3 *from, NJS_POINT3 *to, Float *width,
                    Uint32 *color);
void beam_draw_ray_textured(NJS_POINT3 *pos, struct angles *ang, Float *width,
                            struct beam_texture *texture, Float length);
void beam_draw_line_textured(NJS_POINT3 *from, NJS_POINT3 *to, Float *width,
                             struct beam_texture *texture);

#endif

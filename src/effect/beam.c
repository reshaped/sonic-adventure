/* beams: rays of light that swing about a point, and the draws of a ray or
 * a line as a quad on the screen */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/beam.h"
#include "main.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/task.h"
#include "system/util.h"

/* how far a beam fades in or out a step, and the share of its length it
 * starts at */
#define FADE_RATE 0.1F
#define START_SCALE 0.1F

struct beam {
    Sint32 unk_00;
    Sint32 unk_04;
    Sint32 timer; /* steps left before it fades */
    Float fade; /* how far it has faded in, 0 to 1. its widths scale by it */
    Float scale; /* the share of its length it has grown to, up to 1 */
    Float growth; /* what scale is multiplied by each step */
    NJS_POINT3 *pos; /* its foot, which the caller keeps */
    struct angles ang;
    struct angles speed; /* added to ang a step, within max_speed */
    struct angles accel;
    Angle max_speed;
    Float width[2];
    /* the colors at its foot and its tip, which beam_draw passes as one
     * array */
    /* @hack type: two members, not an array, which beam_create copies
     * through @(R0,R14) (0x8c047858 to 0x8c04785c); as an array, it stores
     * the first through its address in R3, and 26 of the 162 words of
     * beam_create and beam_draw differ */
    Uint32 color_foot;
    Uint32 color_tip;
    Float length;
};

#define BEAM(task) ((struct beam *)(task)->extra)

/* a segment's two ends on the screen, and its widths there */
struct beam_screen {
    NJS_POINT3 pos[2];
    Float width[2];
};

static void beam_grow(struct task *task);
static void beam_fade(struct task *task);
static void beam_draw(struct task *task);
static void beam_draw_polygon(NJS_POINT3 *pos, const Float *width,
                              const Uint32 *color);

void beam_create(NJS_POINT3 *pos, struct beam_params *params)
{
    struct task *task;
    struct beam *beam;

    if ((task = task_create(0, 4, beam_grow)) != NULL) {
        if ((beam = heap_alloc(sizeof(struct beam))) == NULL) {
            task_destroy(task);
            return;
        }
        task->extra = beam;
        beam->unk_00 = 0;
        beam->unk_04 = 0;
        beam->timer = params->time;
        beam->fade = 0.0F;
        beam->scale = START_SCALE;
        beam->growth = params->growth;
        beam->pos = pos;
        beam->ang.x = rand() * 2;
        beam->ang.y = rand() * 2;
        beam->speed.x = 0;
        beam->speed.y = 0;
        beam->accel.x = (Angle)RANDOM(-9.0F, 9.0F);
        beam->accel.y = (Angle)RANDOM(-9.0F, 9.0F);
        beam->max_speed = params->max_speed;
        beam->width[0] = params->width[0];
        beam->width[1] = params->width[1];
        beam->color_foot = params->color[0];
        beam->color_tip = params->color[1];
        beam->length = params->length;
    }
    /* @bug memory: with no task, BT/S at 0x8c0477be goes on to these
     * stores, which write through task_create's 0 at 0x8c047876 and
     * 0x8c047878 */
    task->step = beam_grow;
    task->draw = beam_draw;
}

static void beam_swing(struct beam *beam)
{
    beam->speed.x =
        CLAMP(beam->speed.x + beam->accel.x, -beam->max_speed, beam->max_speed);
    beam->speed.y =
        CLAMP(beam->speed.y + beam->accel.y, -beam->max_speed, beam->max_speed);
    beam->ang.x += beam->speed.x;
    beam->ang.y += beam->speed.y;
}

static void beam_grow(struct task *task)
{
    struct beam *beam = BEAM(task);

    if (--beam->timer <= 0) {
        task->step = beam_fade;
    }
    beam_swing(beam);
    if (beam->scale < 1.0F) {
        beam->scale *= beam->growth;
        if (beam->scale >= 1.0F) {
            beam->scale = 1.0F;
        }
    }
    if (beam->fade < 1.0F) {
        beam->fade += FADE_RATE;
        if (beam->fade >= 1.0F) {
            beam->fade = 1.0F;
        }
    }
    beam_draw(task);
}

static void beam_fade(struct task *task)
{
    struct beam *beam = BEAM(task);

    beam_swing(beam);
    beam->fade -= FADE_RATE;
    if (beam->fade <= 0.0F) {
        beam->fade = 0.0F;
        task_kill(task);
    }
    beam_draw(task);
}

static void beam_draw(struct task *task)
{
    struct beam *beam = BEAM(task);
    Float width[2];

    width[0] = beam->fade * beam->width[0];
    width[1] = beam->fade * beam->width[1];
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    beam_draw_ray(beam->pos, &beam->ang, width, &beam->color_foot,
                  beam->length * beam->scale);
}

/* move near to where the segment from near to far meets the near clip,
 * and return the width there */
static Float beam_clip_near(NJS_POINT3 *near, NJS_POINT3 *far, Float width,
                            Float far_width)
{
    Float cut;

    if (near->z == far->z) {
        return width;
    }
    cut = (_nj_near_clip_ - near->z) / (far->z - near->z);
    near->z = _nj_near_clip_;
    near->x += (far->x - near->x) * cut;
    near->y += (far->y - near->y) * cut;
    return width + ((far_width - width) * cut);
}

/* project the segment from view[0] to view[1], in view space, onto the
 * screen into out, cutting it at the near clip first. the cut moves view's
 * points. returns FALSE, leaving out unset, when both points lie behind the
 * clip, and TRUE otherwise */
static Bool beam_project(struct beam_screen *out, NJS_POINT3 *view,
                         Float *width)
{
    if (view[0].z < _nj_near_clip_ && view[1].z < _nj_near_clip_) {
        return FALSE;
    }
    if (view[0].z < _nj_near_clip_) {
        out->width[0] = beam_clip_near(&view[0], &view[1], width[0], width[1]);
        out->width[1] = width[1];
    } else if (view[1].z < _nj_near_clip_) {
        out->width[0] = width[0];
        out->width[1] = beam_clip_near(&view[1], &view[0], width[1], width[0]);
    } else {
        out->width[0] = width[0];
        out->width[1] = width[1];
    }
    point_project_centered(&view[0], &out->pos[0]);
    point_project_centered(&view[1], &out->pos[1]);
    return TRUE;
}

/* project the ray from pos as beam_project does. its tip is length along
 * -z, turned about x by ang->x and then about y by ang->y */
static Bool beam_project_ray(struct beam_screen *out, NJS_POINT3 *pos,
                             struct angles *ang, Float *width, Float length)
{
    NJS_POINT3 view[2];
    NJS_VECTOR along;
    NJS_POINT3 tip;

    njPushMatrix(&_unit_matrix_);
    along.x = along.y = 0.0F;
    along.z = -length;
    TURN(njRotateY, ang->y);
    TURN(njRotateX, ang->x);
    njCalcPoint(NULL, &along, &tip);
    njPopMatrix(1);
    VECTOR_SUM(&tip, pos, &tip);
    njCalcPoint(NULL, pos, &view[0]);
    njCalcPoint(NULL, &tip, &view[1]);
    return beam_project(out, view, width);
}

static Bool beam_project_line(struct beam_screen *out, NJS_POINT3 *from,
                              NJS_POINT3 *to, Float *width)
{
    NJS_POINT3 view[2];

    njCalcPoint(NULL, from, &view[0]);
    njCalcPoint(NULL, to, &view[1]);
    return beam_project(out, view, width);
}

/* draw a quad on the screen about the segment from pos[0] to pos[1], on a
 * frame's first step only. at pos[i] it is width[i] wide, in color[i] */
static void beam_draw_polygon(NJS_POINT3 *pos, const Float *width,
                              const Uint32 *color)
{
    NJS_POLYGON_VTX quad[4];
    NJS_POINT3 across;
    NJS_POINT3 side;
    Angle ang;
    int i;

    if (!frame_step) {
        ang = angle_arc_tan2(pos[1].y - pos[0].y, pos[1].x - pos[0].x);
        _builtin_fsca(ang, &across.x, &across.y);
        for (i = 0; i < 2; i++) {
            across.z = pos[i].z;
            point_project_centered(&across, &side);
            side.x *= width[i];
            side.y *= width[i];
            quad[i * 2].x = pos[i].x - side.x + SCREEN_MIDDLE_X;
            quad[i * 2].y = pos[i].y + side.y + SCREEN_MIDDLE_Y;
            quad[i * 2].z = 1.0F / pos[i].z;
            quad[i * 2].col = color[i];
            quad[(i * 2) + 1].x = pos[i].x + side.x + SCREEN_MIDDLE_X;
            quad[(i * 2) + 1].y = pos[i].y - side.y + SCREEN_MIDDLE_Y;
            quad[(i * 2) + 1].z = quad[i * 2].z;
            quad[(i * 2) + 1].col = color[i];
        }
        njDrawPolygon(quad, 4, TRUE);
    }
}

void beam_draw_ray(NJS_POINT3 *pos, struct angles *ang, Float *width,
                   Uint32 *color, Float length)
{
    struct beam_screen screen;

    /* @hack spelling: the projection compared with TRUE, CMP/EQ #1
     * (0x8c047d92). beam_project_ray returns beam_project's result, only
     * TRUE or FALSE; as a plain test, TST, and 2 of 24 words differ */
    if (beam_project_ray(&screen, pos, ang, width, length) == TRUE) {
        beam_draw_polygon(screen.pos, screen.width, color);
    }
}

void beam_draw_line(NJS_POINT3 *from, NJS_POINT3 *to, Float *width,
                    Uint32 *color)
{
    struct beam_screen screen;

    /* @hack spelling: the projection compared with TRUE, CMP/EQ #1
     * (0x8c047dc2). beam_project_line returns beam_project's result, only
     * TRUE or FALSE; as a plain test, TST, and 2 of 24 words differ */
    if (beam_project_line(&screen, from, to, width) == TRUE) {
        beam_draw_polygon(screen.pos, screen.width, color);
    }
}

/* draw beam_draw_polygon's quad textured, with its angle from atan2f rather
 * than angle_arc_tan2 */
static void beam_draw_texture(NJS_POINT3 *pos, const Float *width,
                              struct beam_texture *texture)
{
    NJS_TEXTURE_VTX quad[4];
    NJS_POINT3 across;
    NJS_POINT3 side;
    Angle ang;
    int i;

    if (!frame_step) {
        ang = (Angle)(atan2f(pos[1].y - pos[0].y, pos[1].x - pos[0].x) *
                      ANGLE_PER_RADIAN);
        _builtin_fsca(ang, &across.x, &across.y);
        for (i = 0; i < 2; i++) {
            across.z = pos[i].z;
            point_project_centered(&across, &side);
            side.x *= width[i];
            side.y *= width[i];
            quad[i * 2].x = pos[i].x - side.x + SCREEN_MIDDLE_X;
            quad[i * 2].y = pos[i].y + side.y + SCREEN_MIDDLE_Y;
            quad[i * 2].z = 1.0F / pos[i].z;
            quad[i * 2].col = texture->color[i];
            quad[(i * 2) + 1].x = pos[i].x + side.x + SCREEN_MIDDLE_X;
            quad[(i * 2) + 1].y = pos[i].y - side.y + SCREEN_MIDDLE_Y;
            quad[(i * 2) + 1].z = quad[i * 2].z;
            quad[(i * 2) + 1].col = texture->color[i];
        }
        for (i = 0; i < 4; i++) {
            quad[i].u = texture->uv[i].x;
            quad[i].v = texture->uv[i].y;
        }
        njDrawTexture(quad, 4, texture->texture, TRUE);
    }
}

void beam_draw_ray_textured(NJS_POINT3 *pos, struct angles *ang, Float *width,
                            struct beam_texture *texture, Float length)
{
    struct beam_screen screen;

    /* @hack spelling: the projection compared with TRUE, CMP/EQ #1
     * (0x8c047ff8). beam_project_ray returns beam_project's result, only
     * TRUE or FALSE; as a plain test, TST, and 2 of 24 words differ */
    if (beam_project_ray(&screen, pos, ang, width, length) == TRUE) {
        beam_draw_texture(screen.pos, screen.width, texture);
    }
}

void beam_draw_line_textured(NJS_POINT3 *from, NJS_POINT3 *to, Float *width,
                             struct beam_texture *texture)
{
    struct beam_screen screen;

    /* @hack spelling: the projection compared with TRUE, CMP/EQ #1
     * (0x8c048028). beam_project_line returns beam_project's result, only
     * TRUE or FALSE; as a plain test, TST, and 2 of 24 words differ */
    if (beam_project_line(&screen, from, to, width) == TRUE) {
        beam_draw_texture(screen.pos, screen.width, texture);
    }
}

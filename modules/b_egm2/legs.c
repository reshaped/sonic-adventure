/* the glows on the Egg Walker's legs, and its stomps */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm2/egg_walker.h"
#include "b_egm2/legs.h"
#include "b_egm2/parts.h"
#include "b_egm2/texlists.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "effect/hit.h"
#include "main.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 12
#include "b_egm2/legs/text.inc"

/* Amy's hammer shockwave, in 1ST_READ.BIN */
extern NJS_OBJECT amy_shock_object;

/* a color component from 0 to 1 as a byte */
#define COLOR_BYTE(component) ((Uint32)(int)((component) * 255.0F))

/* each glow sits GLOW_SIDE units off its leg's point, along a direction
 * that turns with the boss. legs 0 and 2 take it one way, and legs 1 and 3
 * the other. a glow is GLOW_SIZE units across at full size. a glow in
 * LEG_FADE loses GLOW_FADE of that size each frame. it also gives off
 * lightning up to LIGHTNING_SCATTER units away on each axis */
#define GLOW_SIDE 5.7F
#define GLOW_SIZE 40.0F
#define GLOW_FADE 0.01F
#define LIGHTNING_SCATTER 10.0F

/* the glows cycle through GLOW_TEXTURES textures, each shown for 4 frames.
 * njDrawTexture takes them by global index from GLOW_TEXTURE. a glow's
 * square lies GLOW_NEARER units nearer the camera than its point */
#define GLOW_TEXTURE 0x631fd
#define GLOW_TEXTURES 3
#define GLOW_NEARER 10.0F

/* the glows' alpha pulses as a sine, whose angle turns PULSE_TURN a frame.
 * it makes a whole turn in about 72 frames */
#define PULSE_TURN 910

/* a stomp lasts 31 steps, its timer counting down from STOMP_TIME past 0.
 * its shape is a cylinder at STAND_Y, whose radius grows by STOMP_RADIUS /
 * STOMP_TIME a step. the radius is STOMP_RADIUS as the timer reaches 0, and
 * 31/30 of it on the last step. its record reaches STOMP_REACH from the
 * stomp's position */
#define STOMP_TIME 30
#define STOMP_RADIUS 74.0F
#define STOMP_REACH 100.0F

/* a stomp's waves are its ripples and walls. each fades from 1 to 0 by
 * FADE_STEP a step, in about 30 steps. a wave's height is WAVE_HEIGHT *
 * njSin(fade * WAVE_ARC), and WAVE_ARC is half a turn. so the height rises
 * from 0 and falls back */
#define FADE_STEP (-1.0F / 30.0F)
#define WAVE_HEIGHT 3.0F
#define WAVE_ARC 32768.0F

/* a wall's color is its base color times its fade and WALL_BRIGHTNESS: at
 * most 7 of 255 */
#define WALL_BRIGHTNESS 0.031F

/* work->vars of a stomp: its timer */
struct stomp_vars {
    Uint8 unk_00[4];
    Sint32 timer;
};

/* work->vars of a wave: its fade, from 1 down to 0 */
struct wave_vars {
    Float fade;
};

enum stomp_mode { STOMP_START, STOMP_SPREAD };
/* a wave's work->mode */
enum wave_mode { WAVE_START, WAVE_FADE };

#define STOMP_VARS(work) ((struct stomp_vars *)&(work)->vars)
#define WAVE_VARS(work) ((struct wave_vars *)&(work)->vars)

static void leg_glow_draw(NJS_POINT3 *pos, NJS_ARGB *color, Float size,
                          int frame);
static void stomp_step(struct task *task);
static void stomp_wall_step(struct task *task);
static void stomp_ripple_step(struct task *task);

/* the names of the glows' textures, in EGM2_EFFECT */
static NJS_TEXNAME egm2_effect_textures[GLOW_TEXTURES] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
NJS_TEXLIST egm2_effect_texlist = {
#include "b_egm2/legs/egm2_effect_texlist.inc"
};

/* leg_glow_sprite's frames, each a whole texture */
static NJS_TEXANIM leg_glow_texanims[GLOW_TEXTURES] = {
#include "b_egm2/legs/leg_glow_texanims.inc"
};

/* a stomp's shape: a cylinder 2 units high, whose radius stomp_step sets.
 * its attack and its guard are full */
static struct collision_shape stomp_shape[1] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     SHAPE_HURTS_OBJECTS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_PLAYERS |
         SHAPE_GUARD | SHAPE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     0.0F,
     1.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* one of a wall's twelve panels: a twelfth of a ring of radius 1 and height
 * 2 */
static NJS_POINT3 wall_points[4] = {
#include "b_egm2/legs/wall_points.inc"
};

/* the panel's corner colors before its fade */
static NJS_COLOR wall_base_colors[4] = {
#include "b_egm2/legs/wall_base_colors.inc"
};

/* the frames since egg_walker_glows_init, which pick the glows' texture */
static Float glow_frames;
/* the glows' alpha this frame */
static Float pulse_alpha;
/* the frames the pulse has run, which egg_walker_glows_init leaves as they
 * are */
static Uint16 pulse_frames;
/* a sprite of the glows' textures, which nothing draws */
static NJS_SPRITE leg_glow_sprite;
/* each leg's glow size, from 1, full size, down to 0 */
static Float leg_glow_sizes[LEGS];
/* the panel's corner colors as the last wall step faded them */
static NJS_COLOR wall_colors[4];

/* @hack type: an array of one, whose address SHC keeps in R9 across the
 * wall's loop (0x0c904014). as one NJS_POINT3COL, 75 of stomp_wall_step's
 * 294 words differ */
static NJS_POINT3COL wall_polygon[1] = {
#include "b_egm2/legs/wall_polygon.inc"
};

void egg_walker_glows_init(void)
{
    sprite_init(&leg_glow_sprite, &egm2_effect_texlist, leg_glow_texanims);
    glow_frames = 0.0F;
    pulse_alpha = 0.0F;
}

void egg_walker_glows_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    int i;
    NJS_POINT3 pos;

    if (frame_step != 0) {
        return;
    }
    pulse_frames++;
    glow_frames += 1.0F;
    /* @bug arithmetic: SHC's code squares 0.5F + 0.25F * njSin(...) here
     * instead of halving it, so the alpha runs from 0.0625 to 0.5625, not from
     * 0.125 to 0.375. SHC loads one 0.5 into FR2 for both uses. its FMAC at
     * 0x0c9038b4 writes the sum over it. the FMUL at 0x0c9038be then
     * multiplies the sum by FR2 */
    pulse_alpha = (0.5F + (0.25F * njSin(pulse_frames * PULSE_TURN))) * 0.5F;
    /* @hack redundant: an empty never-run test of work, which keeps the image's
     * R10 and R11, and FR12 and FR13 (0x0c9039d8, 0x0c903a76); without it, each
     * pair trades and 12 of 259 words differ */
    if (0) {
        if (work) {
        }
    }
    for (i = 0; i < LEGS; i++) {
        switch (boss->legs[i]) {
        case LEG_GLOW:
        case LEG_FADE:
            pos = *egg_walker_part_pos(i + PART_LEGS);
            switch (i) {
            case 0:
            case 2:
                pos.x += GLOW_SIDE * njSin(-work->ang.y + 0x4000);
                pos.z += GLOW_SIDE * njCos(-work->ang.y + 0x4000);
                break;
            case 1:
            case 3:
                pos.x -= GLOW_SIDE * njSin(-work->ang.y + 0x4000);
                pos.z -= GLOW_SIDE * njCos(-work->ang.y + 0x4000);
                break;
            }
            if (boss->legs[i] == LEG_GLOW) {
                leg_glow_sizes[i] = 1.0F;
            } else {
                if ((leg_glow_sizes[i] -= GLOW_FADE) <= 0.0F) {
                    leg_glow_sizes[i] = 0.0F;
                }
            }
            /* @hack redundant: a bare scope for color, size and alpha, which
             * gives the step the image's 28-byte stack frame (ADD #-28,R15 at
             * 0x0c903874). declared with the step's other locals, they
             * change it and 107 of 259 words differ */
            {
                NJS_ARGB color;
                Float size;
                Float alpha;

                alpha = pulse_alpha;
                size = leg_glow_sizes[i] * GLOW_SIZE;
                color.a = alpha;
                color.r = 1.0F;
                color.g = 1.0F;
                color.b = 1.0F;
                leg_glow_draw(&pos, &color, size,
                              ((int)glow_frames >> 2) % GLOW_TEXTURES);
            }
            if (boss->legs[i] == LEG_FADE) {
                NJS_POINT3 at;

                at = pos;
                at.x += RANDOM(-LIGHTNING_SCATTER, LIGHTNING_SCATTER);
                at.y += RANDOM(-LIGHTNING_SCATTER, LIGHTNING_SCATTER);
                at.z += RANDOM(-LIGHTNING_SCATTER, LIGHTNING_SCATTER);
                lightning_create(&at, 1.0F);
            }
            break;
        }
    }
}

/* draw texture frame as a square facing the screen, size units across at
 * pos in the current matrix. the square blends additively, by color's
 * alpha. nothing draws while pos is less than GLOW_NEARER beyond the near
 * clip plane */
static void leg_glow_draw(NJS_POINT3 *pos, NJS_ARGB *color, Float size,
                          int frame)
{
    NJS_POINT3 view;
    NJS_POINT3 screen;
    NJS_POINT3 extent;
    NJS_POINT3 center;
    NJS_TEXTURE_VTX quad[4];
    NJS_POINT3 screen_extent;
    NJS_POINT3 screen_center;
    Uint32 argb;

    njCalcPoint(NULL, pos, &view);
    if (_nj_near_clip_ > view.z - GLOW_NEARER) {
        return;
    }
    point_project_centered(&view, &screen);
    extent.x = size * 1.0F;
    extent.y = size * 1.0F;
    center.x = size * 0.5F;
    center.y = size * 0.5F;
    argb = COLOR_BYTE(color->a) << 24 | COLOR_BYTE(color->r) << 16 |
           COLOR_BYTE(color->g) << 8 | COLOR_BYTE(color->b);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    /* the square's size and its center's offset, projected at the glow's
     * depth */
    extent.z = screen.z;
    center.z = screen.z;
    point_project_centered(&extent, &screen_extent);
    point_project_centered(&center, &screen_center);
    quad[0].u = quad[1].u = 0.0F;
    quad[2].u = quad[3].u = 1.0F;
    quad[0].v = quad[2].v = 0.0F;
    quad[1].v = quad[3].v = 1.0F;
    quad[0].col = quad[1].col = quad[2].col = quad[3].col = argb;
    quad[0].z = quad[1].z = quad[2].z = quad[3].z =
        1.0F / (screen.z - GLOW_NEARER);
    quad[0].x = quad[1].x = screen.x - screen_center.x + SCREEN_MIDDLE_X;
    quad[2].x = quad[3].x =
        screen.x - screen_center.x + screen_extent.x + SCREEN_MIDDLE_X;
    quad[0].y = quad[2].y = screen.y - screen_center.y + SCREEN_MIDDLE_Y;
    quad[1].y = quad[3].y =
        screen.y - screen_center.y + screen_extent.y + SCREEN_MIDDLE_Y;
    njDrawTexture(quad, 4, GLOW_TEXTURE + frame, TRUE);
}

void egg_walker_stomp_create(int foot)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 6, stomp_step)) != NULL) {
        if (foot < LEGS) {
            task->work->pos = *egg_walker_part_pos(foot + PART_FEET);
        } else {
            task->work->pos = player_work[0]->pos;
        }
        task->work->pos.y = STAND_Y;
        collision_init(task, stomp_shape, COLLISION_ITEM, 1, 1);
        task->work->collision->radius = STOMP_REACH;
    }
}

/* the stomp's step. it starts a ripple and a wall at the stomp's x and z, at
 * y 0. then it registers the widening shape */
static void stomp_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *made;

    switch (work->mode) {
    case STOMP_START:
        STOMP_VARS(work)->timer = STOMP_TIME;
        work->mode = STOMP_SPREAD;
        /* fall through */
    case STOMP_SPREAD:
        if ((made = task_create(TASK_WORK, 6, stomp_ripple_step)) != NULL) {
            made->work->pos = work->pos;
            made->work->pos.y = 0.0F;
        }
        if ((made = task_create(TASK_WORK, 6, stomp_wall_step)) != NULL) {
            made->work->pos = work->pos;
            made->work->pos.y = 0.0F;
        }
        if (--STOMP_VARS(work)->timer < 0) {
            task_kill(task);
        }
        break;
    }
    work->collision->shapes->radius =
        ((Float)STOMP_TIME - STOMP_VARS(work)->timer) *
        (STOMP_RADIUS / STOMP_TIME);
    collision_register(work);
}

/* a wall: a ring of twelve faint panels, blended additively. it widens from
 * radius 3 by 1.5 a step as it fades. placement_try_leave can end it
 * early, once the camera and player 0 are farther than 410 from it */
static void stomp_wall_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle rotation;
    Angle angle;
    Uint16 i;

    if (work->mode == WAVE_START) {
        work->scl.x = 3.0F;
        WAVE_VARS(work)->fade = 1.0F;
        work->mode = WAVE_FADE;
    }
    if ((WAVE_VARS(work)->fade += FADE_STEP) < 0.0F) {
        task_kill(task);
        return;
    }
    work->scl.x += 1.5F;
    work->scl.y =
        njSin((Angle)(WAVE_VARS(work)->fade * WAVE_ARC)) * WAVE_HEIGHT;
    work->scl.z = work->scl.x;
    /* @hack redundant: a loop of one pass around the colors' loop, which has
     * SHC load wall_colors' address before the loop's zeros (0x0c903f08).
     * without it, the zeros come first, the step runs 2 bytes long, and 228
     * of 294 words differ */
    do {
        for (i = 0; i < 4; i++) {
            wall_colors[i].argb.r = WAVE_VARS(work)->fade *
                                    wall_base_colors[i].argb.r *
                                    WALL_BRIGHTNESS;
            wall_colors[i].argb.g = WAVE_VARS(work)->fade *
                                    wall_base_colors[i].argb.g *
                                    WALL_BRIGHTNESS;
            wall_colors[i].argb.b = WAVE_VARS(work)->fade *
                                    wall_base_colors[i].argb.b *
                                    WALL_BRIGHTNESS;
        }
    } while (0);
    if (frame_step == 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, rotation);
        TURN_VIA(njRotateX, work->ang.x, rotation);
        for (angle = 0; angle < 0xfffc; angle += 0x1555) {
            njPushMatrix(NULL);
            TURN(njRotateY, angle);
            njScaleV(NULL, &work->scl);
            njDrawPolygon3D(wall_polygon, 4, NJD_FILL | NJD_TRANSPARENT);
            njPopMatrix(1);
        }
        njPopMatrix(1);
        placement_try_leave(task);
    }
}

/* a ripple: Amy's shockwave, drawn with EGM2_HAMON's textures. it widens
 * from 20 by 2.32 a step as it fades */
static void stomp_ripple_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle rotation;

    if (work->mode == WAVE_START) {
        work->scl.x = 20.0F;
        WAVE_VARS(work)->fade = 1.0F;
        work->mode = WAVE_FADE;
    }
    if ((WAVE_VARS(work)->fade += FADE_STEP) < 0.0F) {
        task_kill(task);
        return;
    }
    work->scl.x += 2.32F;
    work->scl.y =
        njSin((Angle)(WAVE_VARS(work)->fade * WAVE_ARC)) * WAVE_HEIGHT;
    work->scl.z = work->scl.x;
    if (frame_step == 0) {
        njSetTexture(&egm2_hamon_texlist);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, rotation);
        TURN_VIA(njRotateX, work->ang.x, rotation);
        njScaleV(NULL, &work->scl);
        njDrawObject(&amy_shock_object);
        njPopMatrix(1);
    }
}

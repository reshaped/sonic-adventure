/* EV0050.PRS's Egg Walker, with Eggman in its seat. it falls to the ground
 * and lands */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/ground.h"
#include "collision/polygon.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "ev0050/eggman_body_textures.h"
#include "ev0050/walker.h"
#include "system/task.h"
#include "system/util.h"

#define WALKER_SEAT 3 /* the node of ev0050_walker_object Eggman sits at */
#define WALKER_REACH 1000.0F /* how far below it the walker seeks ground */
#define WALKER_STILL_FRAME 40.0F /* the landing's frame it stands still at */
#define WALKER_GRAVITY 0.09F /* added to the walker's fall speed each step */

#define WALKER_PUFFS 15 /* how many puffs of dust ring each foot */
#define PUFF_TURN 0x1000 /* the turn between puffs, a sixteenth */
#define PUFF_INNER 10.0F /* how far from the foot a puff starts */
#define PUFF_OUTER 12.0F /* how far from the foot outer lies */

/* work->tick holds one of these. the step draws the shadow while it holds
 * WALKER_SHADOW */
#define WALKER_NO_SHADOW 1
#define WALKER_SHADOW 2

struct walker_vars {
    Float frame; /* the frame of ev0050_walker_landing it is drawn at */
    Float fall; /* the speed it falls at */
    Float floor; /* the y of the ground under it, as its first step found */
};

/* the shadow's size and height. the walker keeps them in task->motion,
 * the record TASK_MOTION allocates, in place of a struct task_motion */
struct walker_shadow {
    Float size;
    Uint8 unk_04[48];
    Float height; /* above floor, where the shadow is drawn */
};

#define VARS(work) ((struct walker_vars *)&(work)->vars)
#define SHADOW(task) ((struct walker_shadow *)(task)->motion)

/* @hack spelling: the landing's frame count read through the action's address,
 * at each of the step's three reads. SHC then loads the action's address
 * from the pool again for each read (0x0cb81fe2, 0x0cb82018, 0x0cb82038), as
 * the image does. as ev0050_walker_landing.motion->nbFrame, SHC keeps the
 * address in R5 after the first, ev0050_walker_step runs 8 bytes short, and
 * 95 of 136 words differ */
#define LANDING_FRAMES ((&ev0050_walker_landing)->motion->nbFrame)

extern NJS_OBJECT ev0050_walker_object;
extern NJS_MDATA2 ev0050_walker_landing_keys[];
extern NJS_OBJECT ev0050_eggman_body_object;

static NJS_MOTION ev0050_walker_landing_motion = {
#include "ev0050/walker/ev0050_walker_landing_motion.inc"
};
static NJS_ACTION ev0050_walker_landing = {
#include "ev0050/walker/ev0050_walker_landing.inc"
};

/* the walker's feet, as points in its own frame */
static NJS_POINT3 ev0050_walker_feet[4] = {
    {-34.7F, 0.0F, 51.4F},
    {34.7F, 0.0F, 51.4F},
    {-34.7F, 0.0F, -51.4F},
    {34.7F, 0.0F, -51.4F},
};

static struct dust_params ev0050_walker_dust = {
    1.0F,
    0.03F,
    0.09F,
    0.09F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.3F, 0.8F, 0.8F, 1.0F},
};

/* the last walker created, for the calls in walker.h. a walker that finds
 * no ground kills itself and leaves this dangling */
static struct task *ev0050_walker = NULL;

static void ev0050_walker_destroy(struct task *task)
{
}

static void ev0050_walker_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MATRIX seat;
    Angle ang;

    draw_set_palette(DRAW_PLAYER_PALETTE);
    njSetTexture(&ev0050_walker_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(&ev0050_walker_landing, VARS(work)->frame);
    njSetTexture(&ev0050_eggman_body_texlist);
    pose_action(&_unit_matrix_, &ev0050_walker_landing, VARS(work)->frame,
                WALKER_SEAT, &seat);
    njMultiMatrix(NULL, &seat);
    draw_tree(&ev0050_eggman_body_object);
    njPopMatrix(1);
    draw_set_palette(0);
}

/* the walker's first step: find the ground up to WALKER_REACH below it, or
 * kill it. a walker that finds the ground stands still at
 * WALKER_STILL_FRAME */
static void ev0050_walker_start(struct task *task, struct task_work *work)
{
    struct ray_query query;

    VARS(work)->fall = 0.0F;
    VARS(work)->frame = WALKER_STILL_FRAME;
    VARS(work)->floor = 0.0F;
    work->mode = WALKER_STILL;
    work->submode = 0;
    query.length = WALKER_REACH;
    query.origin.x = work->pos.x;
    query.origin.y = work->pos.y;
    query.origin.z = work->pos.z;
    query.dir.x = 0.0F;
    query.dir.y = -1.0F;
    query.dir.z = 0.0F;
    if (polygon_cast_ray(&query) == FALSE) {
        task_kill(task);
    } else {
        VARS(work)->floor = query.hit.y;
        task->draw = ev0050_walker_draw;
        task->destructor = ev0050_walker_destroy;
    }
}

static void ev0050_walker_fall(struct task_work *work)
{
    VARS(work)->fall += WALKER_GRAVITY;
    work->pos.y -= VARS(work)->fall;
    if (work->pos.y < VARS(work)->floor) {
        work->pos.y = VARS(work)->floor;
        work->mode = WALKER_LAND;
    }
}

/* raise a ring of dust around foot, a point in the walker's frame. its
 * WALKER_PUFFS puffs stand PUFF_TURN apart, so one place of 16 stays empty.
 * each moves away from the foot at PUFF_OUTER less PUFF_INNER a step,
 * before drag */
static void ev0050_walker_dust_ring(struct task_work *work, NJS_POINT3 *foot)
{
    NJS_POINT3 center;
    NJS_POINT3 outer;
    Angle ang;
    Angle turn;
    Sint8 i;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, turn);
    TURN_VIA(njRotateX, work->ang.x, turn);
    TURN_VIA(njRotateY, work->ang.y, turn);
    njCalcPoint(NULL, foot, &center);
    njPopMatrix(1);
    ang = 0;
    ev0050_walker_dust.scale = 3.0F;
    for (i = 0; i < WALKER_PUFFS; i++, ang += PUFF_TURN) {
        /* @hack spelling: the puff's pos written a float at a time through to
         * and from. SHC keeps the two in stack slots and steps them there, as
         * the image does (from 0x0cb81a8c). as three assignments to
         * ev0050_walker_dust.pos, ev0050_walker_dust_ring runs 208 bytes
         * short, and 594 of 663 words differ */
        {
            Float *to = &ev0050_walker_dust.pos.x;
            Float *from = &center.x;

            *to++ = *from++ + (PUFF_INNER * njCos(ang));
            /* @hack redundant: + 0.0F, which SHC adds as FADD FR15,FR3 at
             * 0x0cb81aca, FR15 holding 0.0 from 0x0cb81a82, as the image
             * does. without it, ev0050_walker_dust_ring runs 8 bytes short,
             * and 560 of 663 words differ */
            *to++ = *from++ + 0.0F;
            *to = *from + (PUFF_INNER * njSin(ang));
        }
        /* @hack spelling: outer written a float at a time through to and from.
         * SHC keeps the two in stack slots and steps them there, as the image
         * does (from 0x0cb81ae2). as three assignments to outer,
         * ev0050_walker_dust_ring runs 212 bytes short, and 537 of 663 words
         * differ */
        {
            Float *to = &outer.x;
            Float *from = &center.x;

            *to++ = *from++ + (PUFF_OUTER * njCos(ang));
            /* @hack redundant: + 0.0F, which SHC adds as FADD FR15,FR3 at
             * 0x0cb81b22. without it, ev0050_walker_dust_ring runs 8 bytes
             * short, and 522 of 663 words differ */
            *to++ = *from++ + 0.0F;
            *to = *from + (PUFF_OUTER * njSin(ang));
        }
        /* @hack spelling: the speed written a float at a time through to, from
         * and less, which SHC keeps in R4, R5 and R6, as the image does (from
         * 0x0cb81b34). as three assignments to ev0050_walker_dust.speed,
         * ev0050_walker_dust_ring runs 12 bytes short, and 590 of 663 words
         * differ */
        {
            Float *to;
            Float *from;
            Float *less;

            /* @hack type: from as (Float *)&outer, not &outer.x. as
             * &outer.x, SHC holds outer's address in R10 through the loop,
             * ev0050_walker_dust_ring runs 8 bytes short, and 549 of 663
             * words differ */
            *(to = &ev0050_walker_dust.speed.x) =
                *(from = (Float *)&outer) - *(less = &ev0050_walker_dust.pos.x);
            *++to = *++from - *++less;
            to++;
            from++;
            less++;
            *to = *from - *less;
        }
        dust_create_from(&ev0050_walker_dust);
    }
}

static void ev0050_walker_raise_dust(struct task_work *work)
{
    NJS_POINT3 *foot = ev0050_walker_feet;
    Sint8 i;

    for (i = 0; i < 4; i++, foot++) {
        ev0050_walker_dust_ring(work, foot);
    }
}

static void ev0050_walker_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    struct angles ang;

    switch (work->mode) {
    case WALKER_START:
        ev0050_walker_start(task, work);
        break;
    case WALKER_FALL:
        ev0050_walker_fall(work);
        break;
    case WALKER_LAND:
        VARS(work)->frame += 0.2F;
        if (VARS(work)->frame > LANDING_FRAMES - 1) {
            VARS(work)->frame = 0.0F;
            work->mode = WALKER_LANDED;
        }
        /* raise the dust once, 15 frames before the landing's end, where
         * the feet meet the ground */
        if (work->submode == 0) {
            if (VARS(work)->frame > LANDING_FRAMES - 15) {
                if (VARS(work)->frame < LANDING_FRAMES - 14) {
                    ev0050_walker_raise_dust(work);
                    work->submode = 1;
                }
            }
        }
        break;
    case WALKER_LANDED:
        work->mode = WALKER_STILL;
        break;
    case WALKER_STILL:
    case WALKER_MODE_5:
        break;
    }
    if (work->tick & WALKER_SHADOW) {
        pos.x = work->pos.x;
        pos.y = VARS(work)->floor + SHADOW(task)->height;
        pos.z = work->pos.z;
        ang.x = 0;
        ang.y = 0;
        ang.z = 0;
        shadow_draw_at(&ang, &pos, SHADOW(task)->size);
    }
    ev0050_walker_draw(task);
}

void ev0050_walker_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                          Angle ang_z)
{
    struct task *task =
        task_create(TASK_MOTION | TASK_WORK, 5, ev0050_walker_step);
    struct task_work *work;

    if (task != NULL) {
        work = task->work;
        work->mode = WALKER_START;
        work->tick = WALKER_NO_SHADOW;
        work->pos.x = x;
        work->pos.y = y;
        work->pos.z = z;
        work->ang.x = ang_x;
        work->ang.y = ang_y;
        work->ang.z = ang_z;
        SHADOW(task)->size = 0.0F;
        SHADOW(task)->height = 0.0F;
        ev0050_walker = task;
    }
}

void ev0050_walker_set_mode(Sint8 mode)
{
    ev0050_walker->work->mode = mode;
}

void ev0050_walker_kill(void)
{
    if (ev0050_walker != NULL) {
        task_kill(ev0050_walker);
        ev0050_walker = NULL;
    }
}

void ev0050_walker_set_shadow(Float size, Float height)
{
    if (ev0050_walker != NULL) {
        if (size > 0.0F) {
            ev0050_walker->work->tick = WALKER_SHADOW;
            SHADOW(ev0050_walker)->size = size;
            SHADOW(ev0050_walker)->height = height;
        } else {
            ev0050_walker->work->tick = WALKER_NO_SHADOW;
        }
    }
}

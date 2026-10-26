/* Loop, a ring of animated sprites that shrinks when touched */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "object/common.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "stg09/loop.h"
#include "stg09/obj_casino_textures.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* in work->flags: LOOP_LATCHED while a touch lasts, so that each counts
 * once, and LOOP_SHRINKING while the ring shrinks */
#define LOOP_LATCHED 0x8000
#define LOOP_SHRINKING 0x100

/* destroyed beyond 1500 units of the camera and player 0 */
#define RANGE (1500.0F * 1500.0F)

/* the ring's radius at rest, as a touch sets it, and at its least.
 * RADIUS_SPEED is how far it shrinks or grows each step */
#define REST_RADIUS 40.0F
#define TOUCHED_RADIUS 38.0F
#define LEAST_RADIUS 10.0F
#define RADIUS_SPEED 2.0F

/* the sprites in the ring */
#define SPRITES 16
/* the frames work->mode steps through, one for each of loop_texlist's
 * textures. each shows for FRAME_WAIT + 1 steps */
#define FRAMES 8
#define FRAME_WAIT 2

#define SE_LOOP 239

struct loop_vars {
    Float radius;
};

#define VARS(work) ((struct loop_vars *)&(work)->vars)

static struct collision_shape loop_shape = {
    SHAPE_SPHERE,
    SHAPE_ATTACK,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    10.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};
/* the sprite's frames. the last 2, past FRAMES, name textures loop_texlist
 * doesn't hold, and no code reads them */
static NJS_TEXANIM loop_frames[10] = {
#include "stg09/loop/loop_frames.inc"
};

static NJS_SPRITE loop_sprite;

static void loop_destroy(struct task *task)
{
}

static void loop_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle angle;
    int i;
    Angle phase;
    Float x;
    Float y;

    if (frame_step) {
        return;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&loop_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, angle);
    TURN_VIA(njRotateX, work->ang.x, angle);
    TURN_VIA(njRotateY, work->ang.y, angle);
    for (i = 0, phase = 0; i < SPRITES; i++, phase += 0x10000 / SPRITES) {
        x = njSin(phase) * VARS(work)->radius;
        y = njCos(phase) * VARS(work)->radius;
        loop_sprite.p.x = x;
        loop_sprite.p.y = y;
        loop_sprite.p.z = 0.0F;
        njDrawSprite3D(&loop_sprite, work->mode,
                       NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    }
    njPopMatrix(1);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
}

/* a new touch sets the ring's radius to TOUCHED_RADIUS and shrinks it to
 * LEAST_RADIUS. the ring then grows back to REST_RADIUS */
static void loop_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (work->collision->flags & COLLISION_HIT) {
        if (!(work->flags & LOOP_LATCHED)) {
            work->flags |= LOOP_LATCHED;
            VARS(work)->radius = TOUCHED_RADIUS;
            work->flags |= LOOP_SHRINKING;
            sound_play(SE_LOOP, 0, 0, 0);
        }
    } else {
        work->flags &= ~LOOP_LATCHED;
    }
    if (VARS(work)->radius != REST_RADIUS) {
        if (work->flags & LOOP_SHRINKING) {
            VARS(work)->radius -= RADIUS_SPEED;
            if (VARS(work)->radius <= LEAST_RADIUS) {
                VARS(work)->radius = LEAST_RADIUS;
                work->flags &= ~LOOP_SHRINKING;
            }
        } else {
            VARS(work)->radius += RADIUS_SPEED;
            if (VARS(work)->radius >= REST_RADIUS) {
                VARS(work)->radius = REST_RADIUS;
            }
        }
    }
    if (!work->tick--) {
        work->tick = FRAME_WAIT;
        work->mode = (work->mode + 1) % FRAMES;
    }
    loop_draw(task);
    collision_register(work);
    object_clear_touch(work, NULL);
}

void loop_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        collision_init(task, &loop_shape, COLLISION_OBJECT, 1, 1);
        work->tick = 0;
        work->mode = 0;
        VARS(work)->radius = REST_RADIUS;
        loop_sprite.tlist = &loop_texlist;
        loop_sprite.tanim = loop_frames;
        loop_sprite.sx = 0.2F;
        loop_sprite.sy = 0.2F;
        loop_sprite.p.x = 0.0F;
        loop_sprite.p.y = 0.0F;
        loop_sprite.p.z = 0.0F;
        loop_sprite.ang = 0;
        task->step = loop_run;
        task->draw = loop_draw;
        task->destructor = loop_destroy;
    }
}

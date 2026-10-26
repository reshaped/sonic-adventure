/* O HW BELL hanging bell that rings and scatters rings when struck */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "object/ring.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg04/bell.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL bell_model;
extern NJS_MODEL bell_clapper_model;

/* the bell's first strike scatters BELL_RINGS rings, and each later strike one
 * fewer. the rings scatter from BELL_RINGS_DROP below the bell's position */
#define BELL_RINGS 5
#define BELL_RINGS_DROP 70.0F

/* player 0 swings the clapper only at a ground speed of SWING_SPEED_LEAST or
 * more. a speed past SWING_SPEED_MOST counts as that. a hit on the clapper
 * sets the swing's phase to a turn for each whole unit of speed */
#define SWING_SPEED_LEAST 1.0F
#define SWING_SPEED_MOST 3.0F
/* the swing's phase falls by SWING_STEP a step, a turn in 64 steps. the
 * clapper tilts at most SWING_FARTHEST angle units, about 17 degrees */
#define SWING_STEP 0x400
#define SWING_FARTHEST 3094.0F
/* a swing's ends lie where the phase within its turn is QUARTER_TURN or
 * THREE_QUARTER_TURN */
#define QUARTER_TURN NJM_DEG_ANG(90)
#define THREE_QUARTER_TURN NJM_DEG_ANG(270)

#define SE_BELL 93

/* BELL_STRUCK and BELL_SWINGING are flags in the bell's work->flags.
 * BELL_STRUCK blocks another strike until the bell and the clapper are
 * untouched and still. BELL_SWINGING is set while the clapper swings */
#define BELL_STRUCK 0x100
#define BELL_SWINGING 0x200

/* the bell's work->mode counts the rings its next strike scatters. its
 * vars.l[0] holds the swing's phase, which falls to 0 */
#define RINGS_LEFT(work) ((work)->mode)
#define SWING_PHASE(work) ((work)->vars.l[0])

/* the shares of the clapper's swing that tilt it about x and about z. a hit
 * that swings the clapper sets them to the sine and negated cosine of player
 * 0's heading. it reads player 0's heading whatever made the hit */
struct clapper_vars {
    Uint8 unk_00[4];
    Float x;
    Float z;
};

#define CLAPPER(work) ((struct clapper_vars *)&(work)->vars)

static struct collision_shape bell_shapes[2] = {
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -30.0F, 0.0F},
     17.0F,
     4.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -48.0F, 0.0F},
     19.0F,
     5.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct collision_shape bell_clapper_shape[1] = {
    {SHAPE_CAPSULE,
     SHAPE_GUARD,
     PUSH_FIRM(PUSH_PLAYERS),
     {0.0F, -60.0F, 0.0F},
     8.0F,
     5.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static void bell_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* the clapper's step, which draws it in the bell's frame. a hit while the bell
 * isn't struck swings the clapper along player 0's heading. the bell rings at
 * each end of the swing. the clapper collides only while it doesn't swing and
 * the set editor doesn't hold it */
static void bell_clapper_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *bell = task->parent->work;
    struct task_work *player = player_work[0];
    Float speed;

    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_model(&bell_clapper_model);
    if ((work->collision->flags & COLLISION_HIT) &&
        !(bell->flags & BELL_STRUCK)) {
        Angle heading;

        /* @bug uninitialized: player_ground_speed leaves speed unset for
         * player 0 off the ground, and its FALSE goes unread. the test below
         * then reads speed's frame slot unwritten (FMOV.S @R15,FR2 at
         * 0x0c9040d4) */
        player_ground_speed(0, &speed);
        if (speed < SWING_SPEED_LEAST) {
            return;
        }
        heading = (Uint16)player->ang.y;
        CLAPPER(work)->x = njSin(heading);
        CLAPPER(work)->z = -njCos(heading);
        if (speed > SWING_SPEED_MOST) {
            speed = SWING_SPEED_MOST;
        }
        SWING_PHASE(bell) = (int)speed << 16;
        bell->flags |= BELL_STRUCK | BELL_SWINGING;
    } else if (bell->flags & BELL_SWINGING) {
        Angle phase = SWING_PHASE(bell) & 0x3ff00;

        if (phase) {
            phase &= 0xff00;
            if (phase == THREE_QUARTER_TURN || phase == QUARTER_TURN) {
                sound_play_at(SE_BELL, 0, 0, 64, bell->pos.x, bell->pos.y,
                              bell->pos.z);
            }
            SWING_PHASE(bell) -= SWING_STEP;
            work->ang.x = (Angle)(CLAPPER(work)->x * njSin(SWING_PHASE(bell)) *
                                  SWING_FARTHEST);
            work->ang.z = (Angle)(CLAPPER(work)->z * njSin(SWING_PHASE(bell)) *
                                  SWING_FARTHEST);
        } else {
            bell->flags &= ~BELL_SWINGING;
        }
    } else if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
}

/* the bell's step and draw, which steps the clapper in the bell's frame */
static void bell_run(struct task *task)
{
    struct task_work *work = task->work;
    /* @bug memory: a bell whose first step ran while the set editor held it has
     * no clapper, and the clapper's work is read through the null child,
     * MOV.L @(32,R3),R2 at 0x0c904250 */
    struct task_work *clapper = task->child->work;

    if (placement_try_leave(task)) {
        return;
    }
    draw_set_material(0.0F, work->scl.x, work->scl.y, work->scl.z);
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njPushMatrix(NULL);
    TURN(njRotateY, work->ang.y);
    draw_model(&bell_model);
    njPopMatrix(1);
    task_step_children(task);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    /* a hit while the bell isn't struck rings it, and scatters RINGS_LEFT
     * rings while it has any. otherwise BELL_STRUCK clears once neither the
     * bell nor the clapper is hit and the swing is over */
    if ((work->collision->flags & COLLISION_HIT) &&
        !(work->flags & BELL_STRUCK)) {
        if (RINGS_LEFT(work)) {
            ring_scatter(work->pos.x, work->pos.y - BELL_RINGS_DROP,
                         work->pos.z, RINGS_LEFT(work)--);
        }
        sound_play(SE_BELL, 0, 0, 0);
        work->flags |= BELL_STRUCK;
    } else if ((work->flags & (BELL_STRUCK | BELL_SWINGING)) == BELL_STRUCK &&
               !(work->collision->flags & COLLISION_HIT) &&
               !(clapper->collision->flags & COLLISION_HIT) &&
               !SWING_PHASE(work)) {
        work->flags &= ~BELL_STRUCK;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
}

/* the bell hangs upright, and its clapper takes its heading. a bell that
 * starts while the set editor holds it has no clapper */
void bell_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *clapper;
    struct task_work *clapper_work;

    work->ang.x = work->ang.z = 0;
    collision_init(task, bell_shapes, COLLISION_OBJECT, 1, 2);
    if (!placement_editing(task)) {
        clapper = task_create_child(TASK_MOTION | TASK_WORK | TASK_FORCE,
                                    bell_clapper_step, task);
        clapper_work = clapper->work;
        clapper_work->pos = work->pos;
        clapper_work->ang.x = clapper_work->ang.z = 0;
        clapper_work->ang.y = work->ang.y;
        collision_init(clapper, bell_clapper_shape, COLLISION_OBJECT, 1, 1);
    }
    work->flags &= ~(BELL_STRUCK | BELL_SWINGING);
    RINGS_LEFT(work) = BELL_RINGS;
    task->step = bell_run;
    task->draw = bell_run;
    task->destructor = bell_destroy;
}

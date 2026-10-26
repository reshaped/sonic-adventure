/* O JDOOR, a gate that rises once its flag is 1, and O JSWITCH, the floor
 * switch that sets the flag to 1 */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv0130/jdoor.h"
#include "collision/collision.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* jswitch_base_object is the switch's fixed base, not its plate.
 * jswitch_plate_object is the plate, which goes down */
extern NJS_OBJECT jdoor_object;
extern NJS_OBJECT jswitch_base_object;
extern NJS_OBJECT jswitch_plate_object;

/* a shaking gate's phase moves on by SHAKE_STEP, a fifth of a turn, a step.
 * the gate rises once the phase passes SHAKE_END, six turns */
#define SHAKE_STEP 0x3333
#define SHAKE_END 0x60000

/* a gate rises JDOOR_SPEED a step, to JDOOR_TOP. a switch's plate goes down
 * JSWITCH_SPEED a step, to JSWITCH_BOTTOM */
#define JDOOR_TOP 37.0F
#define JDOOR_SPEED 0.5F
#define JSWITCH_SPEED 0.4F
#define JSWITCH_BOTTOM (-3.0F)

struct jdoor_vars {
    Uint8 unk_00[4];
    Angle shake; /* a gate's shaking phase */
    /* the height of the gate, or of the switch's plate, above its place. the
     * plate's goes below 0 */
    Float height;
};

#define VARS(work) ((struct jdoor_vars *)&(work)->vars)
#define FLAG(work) ((work)->tick)

/* the gate's modes, which the switch borrows: its plate goes down in
 * JDOOR_SHAKE, and it never reaches JDOOR_RISE */
enum jdoor_mode { JDOOR_INIT, JDOOR_WAIT, JDOOR_DONE, JDOOR_SHAKE, JDOOR_RISE };

static struct collision_shape jdoor_shapes[] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL,
     {0.0F, 25.0F, 0.0F},
     55.0F,
     25.0F,
     2.0F,
     {0, 0, 0},
     0,
     NULL},
};
static struct collision_shape jswitch_shape = {SHAPE_CYLINDER | SHAPE_HURTABLE,
                                               0,
                                               PUSH_ALL,
                                               {0.0F, 5.0F, 15.0F},
                                               5.0F,
                                               5.0F,
                                               0.0F,
                                               {0, 0, 0},
                                               0,
                                               NULL};

/* the dust a shaking gate raises. nothing reads jdoor_soft_dust */
static struct dust_params jdoor_dust = {
    1.0F,
    0.03F,
    0.19F,
    0.05F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.5F, 0.8F, 0.8F, 1.0F},
};
static struct dust_params jdoor_soft_dust = {
    1.0F,
    0.05F,
    0.5F,
    0.05F,
    0.001F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.5F, 0.8F, 0.8F, 1.0F},
};

static void jdoor_draw(struct task *task);
static void jdoor_draw_work(struct task_work *work);
static void jswitch_draw(struct task *task);
static void jswitch_draw_work(struct task_work *work);

/* once its flag is 1, the gate shakes, raising dust, until its phase passes
 * SHAKE_END. it then rises to JDOOR_TOP */
void egg_carrier_jdoor_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case JDOOR_INIT:
        FLAG(work) = work->ang.x + JDOOR_FLAGS;
        work->ang.x = 0;
        collision_init(task, jdoor_shapes, COLLISION_OBJECT, 1, 1);
        task->draw = jdoor_draw;
        work->mode = JDOOR_WAIT;
        break;
    case JDOOR_WAIT:
        if (event_flags[FLAG(work)] == 1) {
            work->mode = JDOOR_SHAKE;
            sound_play(SE_ICE_BREAK, 0, 0, 0);
        }
        break;
    case JDOOR_DONE:
        break;
    case JDOOR_SHAKE:
        VARS(work)->shake += SHAKE_STEP;
        if (VARS(work)->shake > SHAKE_END) {
            VARS(work)->shake = 0;
            work->mode = JDOOR_RISE;
        } else {
            for (i = 0; i < 3; i++) {
                jdoor_dust.scale = 1.0F;
                jdoor_dust.pos.x = work->pos.x + RANDOM(-50.0F, 50.0F);
                jdoor_dust.pos.y = work->pos.y + 1.0F;
                jdoor_dust.pos.z = work->pos.z + 2.0F;
                dust_create_from(&jdoor_dust);
            }
        }
        break;
    case JDOOR_RISE:
        sound_play_at_for(SE_PUSH, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 2);
        VARS(work)->height += JDOOR_SPEED;
        if (VARS(work)->height > JDOOR_TOP) {
            VARS(work)->height = JDOOR_TOP;
            work->mode = JDOOR_DONE;
        }
        work->collision->shapes->center.y = VARS(work)->height + 25.0F;
    }
    jdoor_draw_work(work);
    collision_register(work);
}

static void jdoor_draw(struct task *task)
{
    struct task_work *work = task->work;

    jdoor_draw_work(work);
}

static void jdoor_draw_work(struct task_work *work)
{
    Angle ang;
    Float shake = njSin(VARS(work)->shake) * 0.5F;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, 0.0F, VARS(work)->height + shake, 0.0F);
    draw_tree(&jdoor_object);
    njPopMatrix(1);
}

/* a player standing on the switch, or JSWITCH_PRESS in its flag, sends its
 * plate down. once the plate is down, the switch sets its flag to 1 */
void egg_carrier_jswitch_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case JDOOR_INIT:
        FLAG(work) = work->ang.x + JDOOR_FLAGS;
        work->ang.x = 0;
        collision_init(task, &jswitch_shape, COLLISION_OBJECT, 1, 1);
        task->draw = jswitch_draw;
        work->mode = JDOOR_WAIT;
        break;
    case JDOOR_WAIT:
        if (work->flags & COLLISION_UNDER) {
            /* @hack spelling: collision, a copy of work->collision, which
             * keeps the record in R5 and the hit in R2 (0x0c922f88 to
             * 0x0c922f8e); read in place, the loads take R2, R3 and R1, and
             * 4 of 110 words differ */
            struct collision_work *collision = work->collision;

            if (collision->hit->kind == COLLISION_PLAYER) {
                work->mode = JDOOR_SHAKE;
            }
        }
        if (event_flags[FLAG(work)] & JSWITCH_PRESS) {
            event_flags[FLAG(work)] &= ~JSWITCH_PRESS;
            work->mode = JDOOR_SHAKE;
        }
        break;
    case JDOOR_DONE:
        break;
    case JDOOR_SHAKE:
        /* @hack spelling: the speed negated and added, -0.4 in the pool at
         * 0x0c923024 and an FADD at 0x0c922fbc; subtracted, 1 of 110 words
         * differ */
        VARS(work)->height += -JSWITCH_SPEED;
        if (VARS(work)->height < JSWITCH_BOTTOM) {
            VARS(work)->height = JSWITCH_BOTTOM;
            work->mode = JDOOR_DONE;
            /* @hack redundant: the 1 as a select of a constant condition,
             * which SHC folds to a MOV #1,R3 beside the store (0x0c922fd2);
             * a plain 1 lives in R12 through the step, and 65 of 110 words
             * differ */
            /* NOLINTNEXTLINE(*-branch-clone,*-redundant-expression) */
            event_flags[FLAG(work)] = 1.0F ? 1 : 1;
        }
    }
    work->collision->shapes->center.y = VARS(work)->height + 5.0F;
    jswitch_draw_work(work);
    collision_register(work);
}

static void jswitch_draw(struct task *task)
{
    struct task_work *work = task->work;

    jswitch_draw_work(work);
}

static void jswitch_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&jswitch_base_object);
    njTranslate(NULL, 0.0F, VARS(work)->height, 0.0F);
    draw_tree(&jswitch_plate_object);
    njPopMatrix(1);
}

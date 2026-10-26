/* O REALEGG and O FAKEEGG eggs that players carry and set on the egg stand */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/egg_stand.h"
#include "adv00/eggs.h"
#include "adv00/station_square.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/target.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT real_egg_object;
extern NJS_OBJECT fake_egg_object;

/* player 0's held kind while carrying the fake egg */
#define HELD_FAKE_EGG 6

/* a dropped egg that lies off the stand for more than EGG_WAIT steps ends */
#define EGG_WAIT 1800

/* the stand's x and z in the office act */
#define STAND_X 428.5F
#define STAND_Z 677.5F
/* an egg lying within STAND_REACH of the stand in x and z, and less than
 * STAND_HEIGHT above STAND_Y, settles on it */
#define STAND_REACH 5.0F
#define STAND_Y 7.0F
#define STAND_HEIGHT 10.0F

/* sound played when an egg first lands after it is placed or released */
#define SE_EGG_LAND 533

enum egg_mode {
    EGG_PLACED = 0, /* lying where it was placed */
    EGG_DROPPED = 1, /* lying where it was dropped */
    EGG_HELD = 2,
    EGG_ALREADY_HELD = 3, /* player 0 held the egg when the task started */
    EGG_GONE = 6
};

#define EGG_ON_STAND 1 /* in work->submode, while the egg is on the stand */

struct egg_vars {
    Bool landed; /* TRUE once it lands after it is placed or released */
    NJS_OBJECT *object;
    int held; /* HELD_GOLD_EGG or HELD_FAKE_EGG */
};

#define VARS(work) ((struct egg_vars *)&(work)->vars)

static void egg_draw(struct task *task);
static void egg_step(struct task *task);
static void egg_destroy(struct task *task);

/* clang-format off */
static struct collision_shape egg_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 2.0F, 0.0F}, 5.5F, 4.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL, {0.0F, 2.0F, 0.0F},
     2.0F, 4.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* set when a dropped real egg ends after EGG_WAIT steps. a lying fake egg
 * then ends too. real_egg_init clears it */
static Bool real_egg_gone = FALSE;

static void egg_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(VARS(work)->object);
    njPopMatrix(1);
}

static void egg_step(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float z;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        egg_draw(task);
        return;
    }
    egg_draw(task);
    switch (work->mode) {
    /* @hack redundant: default: break; ahead of the cases, which has SHC place
     * a literal pool after the tests' last BRA (0x0c9090e0); without it, the
     * first case follows that BRA, the object's code runs 32 bytes long, and
     * 174 of 214 words differ */
    default:
        break;
    case EGG_PLACED:
    case EGG_DROPPED:
        if (VARS(work)->held == HELD_FAKE_EGG && real_egg_gone == TRUE) {
            task_kill(task);
            return;
        }
        carry_step_small(task);
        if (!VARS(work)->landed && (work->flags & CARRY_GROUND)) {
            sound_play(SE_EGG_LAND, 0, 0, 0);
            VARS(work)->landed = TRUE;
        }
        if (work->submode != EGG_ON_STAND && work->mode == EGG_DROPPED) {
            ++TASK_TIMER(work);
            if (TASK_TIMER(work) > EGG_WAIT) {
                if (VARS(work)->held == HELD_GOLD_EGG) {
                    real_egg_gone = TRUE;
                }
                task_kill(task);
                return;
            }
        }
        /* in the office act, an egg lying near enough to the empty stand
         * settles on it */
        if (!event_flags[FLAG_EGG_ON_STAND] &&
            (Uint8)stage_get_stage_act() == STATION_SQUARE_OFFICE) {
            x = work->pos.x - STAND_X;
            z = work->pos.z - STAND_Z;
            x = (x * x) + (z * z);
            if (x < STAND_REACH * STAND_REACH) {
                if (work->pos.y - STAND_Y < STAND_HEIGHT) {
                    event_flags[FLAG_EGG_ON_STAND] = TRUE;
                    work->pos.x = STAND_X;
                    work->pos.z = STAND_Z;
                    work->submode = EGG_ON_STAND;
                }
            }
        }
        if (work->flags & CARRY_HELD) {
            player_set_held_kind(0, VARS(work)->held);
            work->mode = EGG_HELD;
            if (work->submode == EGG_ON_STAND) {
                work->submode = 0;
                event_flags[FLAG_EGG_ON_STAND] = FALSE;
            }
            VARS(work)->landed = FALSE;
        }
        break;
    case EGG_HELD:
        work->ang.y = -player_work[0]->ang.y;
        carry_step_small(task);
        if (!(work->flags & CARRY_HELD)) {
            player_set_held_kind(0, 0);
            TASK_TIMER(work) = 0;
            work->mode = EGG_DROPPED;
            task->placement->flags &= ~PLACEMENT_HELD;
        } else {
            placement_hold(task);
        }
        break;
    case EGG_ALREADY_HELD:
        break;
    case EGG_GONE:
        placement_kill(task);
        break;
    }
}

static void egg_destroy(struct task *task)
{
    task->placement->flags &= ~PLACEMENT_HELD;
}

/* TRUE once the real egg is taken into a garden */
/* @hack spelling: the flag read in a helper, which SHC expands in egg_start,
 * zero-extending the flag there (EXTU.B R4,R4 at 0x0c90932c) and writing no
 * code of its own. its definition still ends egg_destroy's code on the
 * literal pool that egg_step and egg_destroy load from (0x0c9092e0), as in
 * the image. read in egg_start, SHC writes that pool inside egg_start, which
 * starts 40 bytes sooner, and 78 of its 78 words differ */
static Bool eggs_swapped(void)
{
    return event_flags[FLAG_GOLD_EGG_IN];
}

/* start an egg, or kill it once the real egg is taken into a garden */
static void egg_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    work->ang.x = 0;
    work->ang.z = 0;
    TASK_TIMER(work) = 0;
    work->submode = 0;
    if (!placement_editing(task)) {
        if (eggs_swapped()) {
            placement_kill(task);
            return;
        }
    }
    if (player_get_held_kind(0) == VARS(work)->held) {
        work->mode = EGG_ALREADY_HELD;
    } else {
        work->mode = EGG_PLACED;
    }
    if (!placement_editing(task)) {
        collision_init(task, egg_shapes, COLLISION_OBJECT, 1, 2);
    }
    carry_set_kind(work, motion, CARRY_KIND_SMALL);
    work->flags |= CARRY_WALLS;
    task->step = egg_step;
    task->draw = egg_draw;
    task->destructor = egg_destroy;
}

void real_egg_init(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->object = &real_egg_object;
    VARS(work)->held = HELD_GOLD_EGG;
    real_egg_gone = FALSE;
    egg_start(task);
}

/* the fake egg starts landed, so it plays no sound as it first settles */
void fake_egg_init(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->object = &fake_egg_object;
    VARS(work)->held = HELD_FAKE_EGG;
    VARS(work)->landed = TRUE;
    egg_start(task);
}

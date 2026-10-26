/* O SONZ Sonic statue, which shows one of five levels by the rings
 * deposited */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg09/burst_statue.h"
#include "stg09/casino.h"
#include "stg09/sonic_statue.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT sonic_statue_object_0;
extern NJS_OBJECT sonic_statue_object_1;
extern NJS_OBJECT sonic_statue_object_2;
extern NJS_OBJECT sonic_statue_object_3;
extern NJS_OBJECT sonic_statue_object_4;
extern NJS_OBJECT sonic_statue_object_5;
extern NJS_OBJECT sonic_statue_object_6;

/* the statue is destroyed beyond about 600 units of the camera and player 0.
 * RANGE is that distance squared */
#define RANGE 360010.0F

#define SONIC_STATUE_LEVELS 5
#define LEVEL_TREES 2

#define LEVEL(work) ((work)->submode)

/* the kept statue's work, whose level sonic_statue_set_level sets, or NULL
 * for none. a statue that starts outside the set editor while none is kept
 * becomes the kept one. its destructor sets this back to NULL */
static struct task_work *sonic_statue_keeper = NULL;
/* each level's two trees, NULL for none. level 0 draws nothing */
static NJS_OBJECT *sonic_statue_trees[SONIC_STATUE_LEVELS * LEVEL_TREES] = {
    NULL,
    NULL,
    &sonic_statue_object_0,
    &sonic_statue_object_1,
    &sonic_statue_object_2,
    &sonic_statue_object_3,
    &sonic_statue_object_4,
    &sonic_statue_object_5,
    &sonic_statue_object_6,
    NULL,
};
static struct collision_shape sonic_statue_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {120.0F, 0.0F, 220.0F},
    50.0F,
    2.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void sonic_statue_init(struct task *task);
static void sonic_statue_destroy(struct task *task);
static void sonic_statue_run(struct task *task);
static void sonic_statue_draw(struct task *task);

/* @hack spelling: sonic_statue_set_level's store through an inline function,
 * which keeps work in R14 and saves it (MOV.L R14,@-R15 at 0x0c915ae2). stored
 * in the setter, work takes R5, the setter runs 2 bytes short and 11 of its 12
 * words differ */
static void sonic_statue_show(struct task_work *work, int level)
{
    LEVEL(work) = level;
}

void sonic_statue_set_level(int level)
{
    struct task_work *work = sonic_statue_keeper;

    if (work) {
        if (level <= SONIC_STATUE_LEVELS - 1) {
            sonic_statue_show(work, level);
        }
    }
}

void sonic_statue_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (player_get_character(0) == CHARACTER_KNUCKLES) {
            sonic_statue_knuckles_step(task);
            return;
        }
        sonic_statue_init(task);
        if (!placement_editing(task)) {
            if (!sonic_statue_keeper) {
                sonic_statue_keeper = work;
            }
        }
    }
}

static void sonic_statue_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_init(task, &sonic_statue_shape, COLLISION_OBJECT, 1, 1);
    }
    task->draw = sonic_statue_draw;
    task->step = sonic_statue_run;
    task->destructor = sonic_statue_destroy;
    work->timer = 0;
    work->mode = 0;
    LEVEL(work) = 0;
}

static void sonic_statue_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (sonic_statue_keeper == work) {
        sonic_statue_keeper = NULL;
    }
}

static void sonic_statue_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        sonic_statue_draw(task);
        if (!placement_editing(task)) {
            /* @hack spelling: the test for the wrap to 0, written <= 0 for
             * the image's CMP/PL (0x0c915bc8). TASK_TIMER is a Uint16, so <= 0
             * holds only at 0. the decrement undoes the wrap, which holds the
             * timer at 65535. as == 0, the test is TST, and 2 of 31 words
             * differ */
            if (++TASK_TIMER(work) <= 0) {
                --TASK_TIMER(work);
            }
        }
    }
}

static void sonic_statue_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT **trees;
    NJS_OBJECT *first;
    NJS_OBJECT *second;
    Angle ang;

    if (!placement_try_leave_by(task, RANGE)) {
        trees = &sonic_statue_trees[LEVEL(work) * LEVEL_TREES];
        first = trees[0];
        second = trees[1];
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        if (first) {
            draw_tree(first);
        }
        if (second) {
            draw_tree(second);
        }
        njPopMatrix(1);
    }
}

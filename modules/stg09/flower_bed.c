/* O KDANL and O KDANM: a large and a medium flower bed, whose soil Knuckles
 * can dig into */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "stg09/casino.h"
#include "stg09/flower_bed.h"
#include "stg09/helpers.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT flower_bed_large_object;
extern NJS_OBJECT flower_bed_medium_object;

/* a bed starts only within about 600 of the camera or player 0, and
 * destroys itself beyond it. placement_try_leave_by takes the range
 * squared */
#define RANGE ((600.0F * 600.0F) + 10.0F)

/* the shapes of each bed's walls, one on each side of its soil */
#define WALLS 4
/* the radius casino_world_add and casino_world_test_near take for the
 * soil's copy of each bed */
#define LARGE_COPY_RADIUS 40.0F
#define MEDIUM_COPY_RADIUS 20.0F

struct flower_bed_vars {
    NJS_OBJECT *object; /* the soil's copy in the collision world */
    NJS_MODEL *model;
    /* the radius casino_world_add and casino_world_test_near take for the
     * copy */
    Float copy_radius;
};

#define VARS(work) ((struct flower_bed_vars *)&(work)->vars)

static struct collision_shape flower_bed_medium_walls[WALLS] = {
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {12.5F, 2.5F, 0.0F},
        2.5F,
        2.5F,
        20.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {-12.5F, 2.5F, 0.0F},
        2.5F,
        2.5F,
        20.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 2.5F, 17.0F},
        10.0F,
        2.5F,
        3.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 2.5F, -17.0F},
        10.0F,
        2.5F,
        3.0F,
        {0},
        0,
        NULL,
    },
};
static struct collision_shape flower_bed_large_walls[WALLS] = {
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {16.0F, 2.5F, 0.0F},
        4.0F,
        2.5F,
        40.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {-16.0F, 2.5F, 0.0F},
        4.0F,
        2.5F,
        40.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 2.5F, 36.0F},
        12.0F,
        2.5F,
        4.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 2.5F, -36.0F},
        12.0F,
        2.5F,
        4.0F,
        {0},
        0,
        NULL,
    },
};

static void flower_bed_init(struct task *task, NJS_OBJECT *object,
                            struct collision_shape *walls, int count,
                            Float copy_radius);
static void flower_bed_destroy(struct task *task);
static void flower_bed_draw(struct task *task);
static void flower_bed_run(struct task *task);

void flower_bed_large_step(struct task *task)
{
    flower_bed_init(task, &flower_bed_large_object, flower_bed_large_walls,
                    WALLS, LARGE_COPY_RADIUS);
}

void flower_bed_medium_step(struct task *task)
{
    flower_bed_init(task, &flower_bed_medium_object, flower_bed_medium_walls,
                    WALLS, MEDIUM_COPY_RADIUS);
}

/* start a bed that draws object's model. outside the set editor, object's
 * child goes in the collision world as ground Knuckles can dig into. the
 * work's collision record then takes count shapes from walls */
static void flower_bed_init(struct task *task, NJS_OBJECT *object,
                            struct collision_shape *walls, int count,
                            Float copy_radius)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        VARS(work)->model = object->model;
        VARS(work)->copy_radius = copy_radius;
        task->draw = flower_bed_draw;
        task->step = flower_bed_run;
        task->destructor = flower_bed_destroy;
        if (!placement_editing(task)) {
            casino_world_add(task, object->child, copy_radius, WORLD_DIGGABLE);
            collision_init(task, walls, COLLISION_OBJECT, 1, count);
        }
    }
}

static void flower_bed_destroy(struct task *task)
{
    /* @hack spelling: a copy of task, which SHC keeps in R14 across
     * placement_editing's call (MOV R4,R14 at 0x0c91ceac). with task alone,
     * SHC keeps it in a frame slot, and 10 of 15 words differ */
    struct task *self = task;

    if (!placement_editing(self)) {
        casino_world_remove(self);
    }
}

static void flower_bed_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (!placement_try_leave_by(task, RANGE)) {
        casino_set_texture(CASINO_TEXTURES_ACTS_12);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(VARS(work)->model);
        njPopMatrix(1);
    }
}

static void flower_bed_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        flower_bed_draw(task);
        if (!placement_editing(task)) {
            collision_register(work);
            casino_world_test_near(task, VARS(work)->copy_radius);
        }
    }
}

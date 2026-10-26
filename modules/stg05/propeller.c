/* O PROPELLER, a fan that turns and bobs, carrying players who stand on it */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/mesh.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg05/propeller.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_MODEL mountain_propeller_base_model;
extern NJS_MODEL mountain_propeller_blades_model;
extern NJS_MODEL mountain_propeller_base_far_model;
extern NJS_MODEL mountain_propeller_blades_far_model;
/* the object each fan adds a copy of to the world */
/* @hack type: an array of one, whose address mountain_propeller_run loads
 * into R3 (0x0c90e68c); as a single object, the pool holds its model member's
 * address and 5 of 269 words differ */
extern NJS_OBJECT mountain_propeller_world_object[1];

enum mountain_propeller_mode { PROPELLER_INIT, PROPELLER_RUN };

struct mountain_propeller_vars {
    /* the fan's copy of mountain_propeller_world_object in the world. while the
     * set editor holds the fan, it is that object itself */
    NJS_OBJECT *world;
    Sint16 bob_phase;
    Sint16 turn_step; /* the fan's turn each step, a multiple of TURN_GRID */
    Float base_y; /* the height the fan bobs about */
};

#define VARS(work) ((struct mountain_propeller_vars *)&(work)->vars)
#define FORCE(task) ((struct task_force *)(task)->force)
/* the fan's turn so far */
#define FAN_ANGLE(work) TASK_TIMER(work)
/* the set entry's scales. x is the fan's turn each step before rounding. y
 * is how high it bobs, and z the step of the bob's phase */
#define SPIN(work) ((work)->scl.x)
#define BOB_HEIGHT(work) ((work)->scl.y)
#define BOB_SPEED(work) ((work)->scl.z)

/* the fan's turn each step is a multiple of this many angle units. so is
 * the angle about its axis that it carries a player from */
#define TURN_GRID 20.0F

/* the world tests the fan's object only while a player is within its
 * model's radius plus this margin */
#define NEAR_MARGIN 16.0F

#define DRAW_RADIUS 150.0F
/* from 250 away from the camera across x and z, the fan draws its plainer
 * models */
#define FAR (250.0F * 250.0F)

static void mountain_propeller_init(struct task *task);
static void mountain_propeller_run(struct task *task);
static void mountain_propeller_draw(struct task *task);
static void mountain_propeller_destroy(struct task *task);
static void mountain_propeller_add_to_world(struct task *task);
static Float mountain_propeller_height(struct task_work *work);

/* the fan and the pole it turns on */
static struct collision_shape mountain_propeller_shapes[2] = {
    {
        SHAPE_CYLINDER,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 0.0F},
        23.0F,
        4.0F,
        0.0F,
        {0, 0, 0},
        0,
        NULL,
    },
    {
        SHAPE_CYLINDER,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 4.3F, 0.0F},
        2.8F,
        18.3F,
        0.0F,
        {0, 0, 0},
        0,
        NULL,
    },
};

void mountain_propeller_step(struct task *task)
{
    if (!placement_try_leave(task)) {
        switch (task->work->mode) {
        case PROPELLER_INIT:
            mountain_propeller_init(task);
            break;
        case PROPELLER_RUN:
            mountain_propeller_run(task);
            break;
        }
    }
}

static void mountain_propeller_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = PROPELLER_RUN;
    FAN_ANGLE(work) = 0;
    VARS(work)->bob_phase = 0;
    VARS(work)->base_y = work->pos.y;
    work->ang.x = 0;
    work->ang.z = 0;
    VARS(work)->turn_step = floorf(SPIN(work) * (1.0F / TURN_GRID)) * TURN_GRID;
    FORCE(task)->speed.x = 0.0F;
    FORCE(task)->speed.z = 0.0F;
    FORCE(task)->speed.y = 0.0F;
    FORCE(task)->turn.x = 0;
    FORCE(task)->turn.z = 0;
    FORCE(task)->turn.y = 0;
    task->draw = mountain_propeller_draw;
    task->destructor = mountain_propeller_destroy;
    if (!placement_editing(task)) {
        mountain_propeller_add_to_world(task);
    }
}

/* turn and bob the fan. while a player is near, the world tests the fan's
 * object. the force then carries player 0 with the fan's turn and rise */
static void mountain_propeller_run(struct task *task)
{
    struct task_work *work = task->work;
    Float last_y = work->pos.y;
    NJS_POINT3 player;
    NJS_POINT3 carried;
    Float reach;
    Angle ang;
    NJS_OBJECT *world;

    mountain_propeller_draw(task);
    if (!placement_editing(task)) {
        if (!player_in_range(&work->pos,
                             mountain_propeller_world_object[0].model->r +
                                 NEAR_MARGIN)) {
            work->flags &= ~WORLD_TESTED;
            FAN_ANGLE(work) += VARS(work)->turn_step;
            VARS(work)->bob_phase += (int)BOB_SPEED(work);
            work->pos.y = mountain_propeller_height(work);
            goto done;
        } else {
            /* @hack redundant: an else after the goto, which keeps the height's
             * difference in R3 (0x0c90e6da, 0x0c90e6e0); without it, R2 takes
             * it and 5 of 269 words differ */
            work->flags |= WORLD_TESTED;
        }
        world = VARS(work)->world;
        world->ang[1] = work->ang.y - FAN_ANGLE(work);
        world->pos[1] = work->pos.y;
    } else {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> FAN SPEED (40)");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> Y SWING   (1.2)");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> Y SPEED   (0x400)");
        /* while the set editor holds the fan, it has no copy, so it points
         * at the shared object */
        VARS(work)->world = mountain_propeller_world_object;
    }
    FAN_ANGLE(work) += VARS(work)->turn_step;
    VARS(work)->bob_phase += (int)BOB_SPEED(work);
    work->pos.y = mountain_propeller_height(work);
    if (!placement_editing(task)) {
        if (!player_get_history(0, 0, &player, NULL)) {
            goto done;
        }
        njSubVector(&player, &work->pos);
        reach = njSqrt((player.x * player.x) + (player.z * player.z));
        if (reach >= 1.0F) {
            /* carry player 0 by turn_step about the axis. the carry rounds
             * player 0's angle to TURN_GRID, and its distance to a multiple
             * of 0.8. the speed takes player 0 there from where it stands */
            ang = (Angle)(atan2f(player.z, player.x) * ANGLE_PER_RADIAN);
            ang = floorf((ang + (TURN_GRID / 2)) * (1.0F / TURN_GRID)) *
                  TURN_GRID;
            reach = (int)((reach * 1.25F) + 0.5F);
            reach *= 0.8F;
            carried.x = reach * njCos(ang + VARS(work)->turn_step);
            carried.z = reach * njSin(ang + VARS(work)->turn_step);
            FORCE(task)->speed.x = carried.x - player.x;
            FORCE(task)->speed.z = carried.z - player.z;
            FORCE(task)->speed.y = work->pos.y - last_y;
            FORCE(task)->turn.x = 0;
            FORCE(task)->turn.z = 0;
            FORCE(task)->turn.y = VARS(work)->turn_step;
        } else {
            FORCE(task)->speed.x = 0.0F;
            FORCE(task)->speed.z = 0.0F;
            FORCE(task)->speed.y = 0.0F;
            FORCE(task)->turn.x = 0;
            FORCE(task)->turn.z = 0;
            FORCE(task)->turn.y = 0;
        }
    }
done:
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void mountain_propeller_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MODEL *base;
    NJS_MODEL *blades;
    Angle turn;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        if (mesh_point_distance2_across(&camera_work->pos, &work->pos) < FAR) {
            base = &mountain_propeller_base_model;
            blades = &mountain_propeller_blades_model;
        } else {
            base = &mountain_propeller_base_far_model;
            blades = &mountain_propeller_blades_far_model;
        }
        turn = -FAN_ANGLE(work);
        njPushMatrix(NULL);
        njSetTexture(&obj_mountain_texlist);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(base);
        njPushMatrix(NULL);
        njSetTexture(&obj_mountain_texlist);
        TURN(njRotateY, turn);
        draw_model(blades);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

static void mountain_propeller_destroy(struct task *task)
{
    if (!placement_editing(task)) {
        world_remove(task, VARS(task->work)->world);
        world_object_free(VARS(task->work)->world);
    }
}

/* give the fan its shapes. then add to the world a copy of
 * mountain_propeller_world_object, at the fan's position and y angle. the
 * destructor removes and frees the copy */
static void mountain_propeller_add_to_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_OBJECT *from;

    collision_init(task, mountain_propeller_shapes, COLLISION_OBJECT, 1, 2);
    from = mountain_propeller_world_object;
    VARS(work)->world = object = world_object_alloc();
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = from->model;
    object->child = from->child;
    object->sibling = from->sibling;
    world_add(WORLD_MOVES, task, object);
}

static Float mountain_propeller_height(struct task_work *work)
{
    return VARS(work)->base_y +
           (BOB_HEIGHT(work) * njSin(VARS(work)->bob_phase));
}

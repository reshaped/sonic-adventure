/* the go-round, which the set editor names O GO ROUND. its carousel and
 * floor turn and rest by turns. the turning floor carries the players on
 * it */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg03/go_round.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: go_round_object as an array, so that go_round_horses_step
 * keeps its address in R4 and reads its model @(4,R4) (0x0c90592e,
 * 0x0c905936). declared as a single object and named by its address, summed
 * over the two functions that use it, 70 of 312 words differ */
extern NJS_OBJECT go_round_object[];
extern NJS_OBJECT go_round_floor_object;
extern NJS_OBJECT go_round_horse_object;

/* the go-round destroys itself once neither the camera nor player 0 is
 * within 710 units */
#define GO_ROUND_RANGE (710.0F * 710.0F)

#define SE_GO_ROUND 67

/* the floor's collision record reaches FLOOR_REACH. the collision world
 * tests the floor only while a player is within FLOOR_NEAR */
#define FLOOR_REACH 180.0F
#define FLOOR_NEAR 200.0F

/* the go-round rests for more than GO_ROUND_REST steps. it then speeds up
 * by 1 a step to GO_ROUND_TOP_SPEED, an angle a step. it turns at that
 * speed for more than GO_ROUND_TURN steps, then slows by 1 a step to rest */
#define GO_ROUND_REST 600
#define GO_ROUND_TOP_SPEED 64
#define GO_ROUND_TURN 12000

/* the horses' height bobs between -HORSES_BOB and HORSES_BOB, moving
 * HORSES_BOB_SPEED a step */
#define HORSES_BOB 5.0F
#define HORSES_BOB_SPEED 0.1F

/* the lamps' blink counts from 0 to LAMPS_BLINK + 1, one a step. the lamps
 * take LAMPS_LIT_TEXTURE up to LAMPS_LIT, and LAMPS_DARK_TEXTURE past it */
#define LAMPS_BLINK 60
#define LAMPS_LIT 30
#define LAMPS_DARK_TEXTURE 28
#define LAMPS_LIT_TEXTURE 95

/* the go-round's turn stage, in work->submode. GO_ROUND_SPEED is its speed,
 * an angle a step. the floor and the horses read both */
enum go_round_turn {
    GO_ROUND_STILL,
    GO_ROUND_SPEEDING,
    GO_ROUND_TURNING,
    GO_ROUND_SLOWING
};
#define GO_ROUND_SPEED(work) TASK_TIMER(work)

struct go_round_vars {
    Uint8 unk_00[8];
    Sint32 steps; /* the steps it has rested, or turned at top speed */
};

#define VARS(work) ((struct go_round_vars *)&(work)->vars)

struct floor_vars {
    NJS_OBJECT *world; /* the floor's copy in the collision world */
    NJS_OBJECT *object;
};

#define FLOOR(work) ((struct floor_vars *)&(work)->vars)
/* the horses' task keeps the horse object in work->vars, and the horses'
 * height in work->scl.x. work->submode holds the way they bob, and
 * work->tick the lamps' blink */
#define HORSE_OBJECT(work) (*(NJS_OBJECT **)&(work)->vars)
enum horses_bob { HORSES_RISING, HORSES_FALLING };
#define HORSES_HEIGHT(work) ((work)->scl.x)

/* the modes of the go-round and of its parts. no code sets GO_ROUND_GONE */
enum go_round_mode { GO_ROUND_INIT, GO_ROUND_RUN, GO_ROUND_GONE };

static struct collision_shape go_round_floor_shape[1] = {
    {SHAPE_CYLINDER | SHAPE_NO_PUSH | SHAPE_IGNORE_ENEMIES |
         SHAPE_IGNORE_OBJECTS | SHAPE_IGNORE_OTHERS,
     0,
     0,
     {0.0F, 4.0F, 0.0F},
     170.0F,
     2.5F,
     0.0F,
     {0},
     0,
     NULL},
};

static void go_round_floor_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        draw_tree(FLOOR(work)->object);
        njPopMatrix(1);
    }
}

/* a part's destructor. outside the set editor, it takes FLOOR(work)->world,
 * the floor's copy, out of the collision world and frees it. the horses'
 * task holds its horse object in that field, which world_remove and
 * world_object_free leave as it is */
static void go_round_part_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && FLOOR(work)->world != NULL) {
        world_remove(task, FLOOR(work)->world);
        world_object_free(FLOOR(work)->world);
    }
    task_kill(task);
}

static void go_round_destroy(struct task *task)
{
    task_destroy_children(task);
    sound_stop(SE_GO_ROUND);
    go_round_part_destroy(task);
}

/* while the go-round turns, the floor turns at its speed. a player that
 * touches the turning floor with PLAYER_ON_LAND set turns with it about its
 * center */
static void go_round_floor_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    Angle ang;
    NJS_OBJECT *world;
    Sint8 player;

    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njEasyDrawObject(&go_round_floor_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case GO_ROUND_INIT:
            work->mode = GO_ROUND_RUN;
            FLOOR(work)->object = &go_round_floor_object;
            collision_init(task, go_round_floor_shape, COLLISION_OBJECT, 1, 1);
            task->work->collision->radius = FLOOR_REACH;
            task->destructor = go_round_part_destroy;
            task->draw = go_round_floor_draw;
            FLOOR(work)->world = world = world_object_alloc();
            world->evalflags = FLOOR(work)->object->evalflags &
                               ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
            world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
            world->ang[0] = work->ang.x;
            world->ang[1] = work->ang.y;
            world->ang[2] = work->ang.z;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->model = FLOOR(work)->object->model;
            world->child = FLOOR(work)->object->child;
            world->sibling = FLOOR(work)->object->sibling;
            world_add(WORLD_LARGE, task, world);
            break;
        case GO_ROUND_RUN:
            if (player_in_range(&work->pos, FLOOR_NEAR)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            if (task->parent->work->submode != GO_ROUND_STILL) {
                ang = GO_ROUND_SPEED(task->parent->work);
                work->ang.y += ang;
                if ((work->collision->flags & COLLISION_HIT) &&
                    work->collision->hit->kind == COLLISION_PLAYER) {
                    player =
                        PLAYER_VARS(work->collision->hit->owner->work)->player;
                    if (player_work[player]->flags & PLAYER_ON_LAND) {
                        NJS_VECTOR from;
                        NJS_VECTOR to;

                        from.x = player_work[player]->pos.x - work->pos.x;
                        from.y = 0.0F;
                        from.z = player_work[player]->pos.z - work->pos.z;
                        njPushMatrix(&_unit_matrix_);
                        TURN(njRotateY, ang);
                        njCalcVector(NULL, &from, &to);
                        njPopMatrix(1);
                        player_work[player]->pos.x = to.x + work->pos.x;
                        player_work[player]->pos.z = to.z + work->pos.z;
                        force->turn.y = -ang;
                    }
                }
                collision_register(work);
            }
            go_round_floor_draw(task);
            break;
        case GO_ROUND_GONE:
            break;
        }
    }
}

/* on a frame's first step, draw the carousel 60 units below the task, and
 * eight horses. four stand a quarter turn apart, raised by the horses'
 * height. the four between them are lowered by it */
static void go_round_horses_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, -60.0F, 0.0F);
        draw_tree(go_round_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, HORSES_HEIGHT(work), 0.0F);
        draw_tree(HORSE_OBJECT(work));
        njRotateY(NULL, 0x4000);
        draw_tree(HORSE_OBJECT(work));
        njRotateY(NULL, 0x4000);
        draw_tree(HORSE_OBJECT(work));
        njRotateY(NULL, 0x4000);
        draw_tree(HORSE_OBJECT(work));
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, -HORSES_HEIGHT(work), 0.0F);
        njRotateY(NULL, 0x2000);
        draw_tree(HORSE_OBJECT(work));
        njRotateY(NULL, 0x4000);
        draw_tree(HORSE_OBJECT(work));
        njRotateY(NULL, 0x4000);
        draw_tree(HORSE_OBJECT(work));
        njRotateY(NULL, 0x4000);
        draw_tree(HORSE_OBJECT(work));
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

/* while the go-round turns, bob the horses and turn them with it. blink the
 * lamps, and draw the carousel while player 0 is near or in front of it. the
 * lamps' texture goes back to dark after the draw */
static void go_round_horses_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float dx;
    Float dz;
    int turn;

    if (edit_mode != 0) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njEasyDrawObject(&go_round_horse_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case GO_ROUND_INIT:
            work->mode = GO_ROUND_RUN;
            HORSE_OBJECT(work) = &go_round_horse_object;
            task->destructor = go_round_part_destroy;
            task->draw = go_round_horses_draw;
            break;
        case GO_ROUND_RUN:
            if (task->parent->work->submode != GO_ROUND_STILL) {
                switch (work->submode) {
                case HORSES_RISING:
                    if ((HORSES_HEIGHT(work) += HORSES_BOB_SPEED) >=
                        HORSES_BOB) {
                        work->submode = HORSES_FALLING;
                    }
                    break;
                case HORSES_FALLING:
                    if ((HORSES_HEIGHT(work) -= HORSES_BOB_SPEED) <=
                        -HORSES_BOB) {
                        work->submode = HORSES_RISING;
                    }
                }
                work->ang.y += GO_ROUND_SPEED(task->parent->work);
            }
            if (work->tick++ > LAMPS_BLINK) {
                work->tick = 0;
            }
            if (work->tick > LAMPS_LIT) {
                go_round_object->model->mats->attr_texId = LAMPS_DARK_TEXTURE;
            } else {
                go_round_object->model->mats->attr_texId = LAMPS_LIT_TEXTURE;
            }
            dx = work->pos.x - player_work[0]->pos.x;
            dz = work->pos.z - player_work[0]->pos.z;
            turn = angle_distance(NJM_DEG_ANG(270),
                                  (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN));
            /* draw while dx lies between -180 and 180 and dz above -180.
             * turn is how far the way from player 0 to the go-round lies
             * from the way along -x. draw too while turn is more than 0x3600
             * and less than 0x6900 */
            /* @bug logic: the box test reads dx where it means dz's upper
             * bound (FCMP/GT FR15,FR3 at 0x0c9059ea, FR15 dx), so any dz
             * above -180 passes, however large */
            /* NOLINTNEXTLINE(misc-redundant-expression) */
            if ((dx > -180.0F && dx < 180.0F && dz > -180.0F && dx < 180.0F) ||
                (turn > 0x3600 && turn < 0x6900)) {
                go_round_horses_draw(task);
            }
            go_round_object->model->mats->attr_texId = LAMPS_DARK_TEXTURE;
            break;
        case GO_ROUND_GONE:
            break;
        }
    }
}

/* the go-round's children: the carousel with its horses, and the floor 85 units
 * below */
static struct task_spec go_round_parts[] = {
    {go_round_horses_step,
     TASK_WORK,
     0,
     {0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&go_round_horse_object},
    {go_round_floor_step,
     TASK_WORK | TASK_FORCE,
     0,
     {0},
     {0.0F, -85.0F, 0.0F},
     (Sint32)&go_round_floor_object},
    {NULL, 0, 0, {0}, {0.0F, 0.0F, 0.0F}, 0},
};

/* step the go-round through its turn stages, playing its sound, and step
 * its parts */
void go_round_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, GO_ROUND_RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        draw_tree(&go_round_horse_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case GO_ROUND_INIT:
            work->mode = GO_ROUND_RUN;
            work->submode = GO_ROUND_SPEEDING;
            work->ang.x = work->ang.z = 0;
            work->scl.x = work->scl.y = 0.0F;
            task->destructor = go_round_destroy;
            task_create_children(go_round_parts, task);
            break;
        case GO_ROUND_RUN:
            if (edit_mode == 0) {
                switch (work->submode) {
                case GO_ROUND_STILL:
                    if (++VARS(work)->steps > GO_ROUND_REST) {
                        work->submode = GO_ROUND_SPEEDING;
                        VARS(work)->steps = 0;
                    }
                    break;
                case GO_ROUND_SPEEDING:
                    if (++GO_ROUND_SPEED(work) >= GO_ROUND_TOP_SPEED) {
                        work->submode = GO_ROUND_TURNING;
                    }
                    break;
                case GO_ROUND_TURNING:
                    if (++VARS(work)->steps > GO_ROUND_TURN) {
                        work->submode = GO_ROUND_SLOWING;
                        VARS(work)->steps = 0;
                    }
                    break;
                case GO_ROUND_SLOWING:
                    if (--GO_ROUND_SPEED(work) <= 0) {
                        work->submode = GO_ROUND_STILL;
                    }
                }
                sound_play_at_for(SE_GO_ROUND, (int)work, 1, 32, work->pos.x,
                                  work->pos.y, work->pos.z, 2);
            }
            task_step_children(task);
            break;
        case GO_ROUND_GONE:
            break;
        }
    }
}

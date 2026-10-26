/* the crane the object list names O_CRANE. its arm turns without stopping,
 * and a handle hangs from each end. a touch by a player, an attack or an
 * enemy makes a handle catch player 0. player 0 then hangs there until it
 * jumps or leaves PLAYER_MODE_HANG_ON */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/world.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg06/crane.h"
#include "stg06/models.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT crane_object;
extern NJS_OBJECT crane_world_object;

/* how far the arm turns each step */
#define CRANE_TURN 136
#define CRANE_FLAGS (WORLD_HUGE | WORLD_TURNED | WORLD_MOVES | WORLD_SOLID)
/* the crane is drawn while a sphere of radius CRANE_VIEW round its position
 * is in view */
#define CRANE_VIEW 400.0F
/* the world tests the crane while a player is within CRANE_NEAR of its
 * model's sphere */
#define CRANE_NEAR 20.0F

/* a handle hangs HANDLE_REACH from the crane's position along the arm, and
 * HANDLE_DROP below it */
#define HANDLE_REACH 210.0F
#define HANDLE_DROP 212.0F
/* a caught player hangs this far below its handle */
#define PLAYER_DROP 9.5F
/* the steps a handle waits after it lets go before it can catch again */
#define HANDLE_REST 120
/* a player on the right handle faces this far round from one on the left */
#define HALF_TURN 0x8000

/* the sound of a handle's catch, and the voice that goes with it: Sonic's,
 * or every other character's */
#define SE_CATCH 180
#define VOICE_SONIC_CATCH 0x45f
#define VOICE_CATCH 0x488

enum crane_mode { CRANE_INIT, CRANE_RUN };

/* a handle's work->mode */
enum handle_mode {
    HANDLE_INIT,
    HANDLE_EMPTY,
    HANDLE_HOLDING, /* player 0 hangs from it */
    HANDLE_RESTING
};

/* a handle's work->submode: the end of the arm it hangs from. HANDLE_LEFT
 * hangs at -HANDLE_REACH along the crane's x, and HANDLE_RIGHT at
 * HANDLE_REACH */
enum handle_side { HANDLE_LEFT, HANDLE_RIGHT };

struct crane_vars {
    /* its shape in the world, which crane_update_world adds on the first
     * step */
    NJS_OBJECT *world;
};

#define VARS(work) ((struct crane_vars *)&(work)->vars)
/* the height of the floor below a handle, which its shadow falls on */
#define FLOOR(work) ((work)->scl.z)

static struct collision_shape handle_shape = {SHAPE_RECTANGLE,
                                              0,
                                              PUSH_FIRM(PUSH_PLAYERS),
                                              {0.0F, 0.0F, 0.0F},
                                              8.0F,
                                              5.0F,
                                              8.0F,
                                              {0, 0, 0},
                                              0,
                                              NULL};

static void crane_draw(struct task *task);
static void crane_destroy(struct task *task);
static void crane_update_world(struct task *task);
static void handle_step(struct task *task);
static void handle_shadow(struct task *task);

void crane_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *handle;

    if (!placement_try_leave(task)) {
        switch (work->mode) {
        case CRANE_INIT:
            work->mode = CRANE_RUN;
            task->draw = crane_draw;
            task->destructor = crane_destroy;
            handle = task_create_child(TASK_WORK, handle_step, task);
            handle->work->submode = HANDLE_LEFT;
            handle = task_create_child(TASK_WORK, handle_step, task);
            handle->work->submode = HANDLE_RIGHT;
            break;
        case CRANE_RUN:
            work->ang.y += CRANE_TURN;
            break;
        }
        crane_draw(task);
        crane_update_world(task);
        task_step_children(task);
    }
}

static void crane_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, CRANE_VIEW)) {
        njSetTexture(&obj_skydeck_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&crane_object);
        njPopMatrix(1);
    }
}

static void crane_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->world) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
        VARS(work)->world = NULL;
    }
}

/* put the crane's shape in the world on the first call. each later call
 * turns the shape with the crane. the world then tests the shape while a
 * player is within CRANE_NEAR of its turned sphere */
static void crane_update_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *shape = &crane_world_object;
    NJS_OBJECT *world;
    NJS_VECTOR center;
    Angle ang;

    /* @hack redundant: shape tested, though it holds crane_world_object's
     * address, which is never null. SHC keeps the test (TST R13,R13 at
     * 0x0c905554); without it, 46 of 160 words differ */
    if (shape) {
        if (!VARS(work)->world) {
            VARS(work)->world = world = world_object_alloc();
            world->evalflags = NJD_EVAL_BREAK;
            world->scl[0] = 1.0F;
            world->scl[1] = 1.0F;
            world->scl[2] = 1.0F;
            world->ang[0] = 0;
            world->ang[1] = work->ang.y;
            world->ang[2] = 0;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->model = shape->model;
            world->child = NULL;
            world->sibling = NULL;
            world_add(CRANE_FLAGS, task, world);
        } else {
            center.x = shape->model->center.x;
            center.y = shape->model->center.y;
            center.z = shape->model->center.z;
            njPushMatrix(NULL);
            njUnitMatrix(NULL);
            TURN_VIA(njRotateZ, work->ang.z, ang);
            TURN_VIA(njRotateX, work->ang.x, ang);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njCalcVector(NULL, &center, &center);
            njPopMatrix(1);
            center.x += work->pos.x;
            center.y += work->pos.y;
            center.z += work->pos.z;
            if (player_in_range(&center, shape->model->r + CRANE_NEAR)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            world = VARS(work)->world;
            world->ang[1] = work->ang.y;
        }
    }
}

/* a handle's step. the handle swings round with the arm. it catches player 0
 * at a touch by any player, attack or enemy. it holds the player until pad
 * 0's jump is pressed or the player leaves PLAYER_MODE_HANG_ON. it then rests
 * HANDLE_REST steps before it can catch again */
static void handle_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *crane = task->parent->work;
    struct task_work *player = player_work[0];
    Float reach;
    Angle yaw;
    Float x;
    Float y;
    Float z;

    reach = work->submode ? HANDLE_REACH : -HANDLE_REACH;
    FLOOR(work) =
        ground_below(work->pos.x, work->pos.y, work->pos.z, &work->ang);
    handle_shadow(task);
    yaw = crane->ang.y + CRANE_TURN; /* the arm's yaw after its next turn */
    /* @hack spelling: the crane's position read before the calls, as the image
     * reads it (0x0c905770 to 0x0c905784); read in each sum, 197 of 227
     * words differ and the object ends 32 bytes early */
    x = crane->pos.x;
    y = crane->pos.y;
    z = crane->pos.z;
    work->pos.x = x + (reach * njCos(-yaw));
    work->pos.y = y - HANDLE_DROP;
    work->pos.z = z + (reach * njSin(-yaw));
    switch (work->mode) {
    case HANDLE_INIT:
        work->mode = HANDLE_EMPTY;
        task->draw = handle_shadow;
        /* @hack redundant: the count written 1.0F && 1, which loads its 1 into
         * R2 at 0x0c905802, apart from the mode's 1 in R4 (0x0c9057c2); as a
         * plain 1, 143 of 227 words differ */
        collision_init(task, &handle_shape, COLLISION_OBJECT, 1, 1.0F && 1);
        break;
    case HANDLE_EMPTY:
        if (work->collision->flags & COLLISION_HIT) {
            player_set_mode(0, PLAYER_MODE_HANG_ON);
            work->mode = HANDLE_HOLDING;
            sound_play(SE_CATCH, 0, 0, 0);
            if (get_character() == CHARACTER_SONIC) {
                sound_play(VOICE_SONIC_CATCH, 0, 0, 0);
            } else {
                sound_play(VOICE_CATCH, 0, 0, 0);
            }
        }
        break;
    case HANDLE_HOLDING:
        player->pos.x = work->pos.x;
        player->pos.y = work->pos.y - PLAYER_DROP;
        player->pos.z = work->pos.z;
        player_set_angles(0, 0, crane->ang.y + (work->submode * HALF_TURN), 0);
        if (player->submode != PLAYER_MODE_HANG_ON) {
            work->mode = HANDLE_RESTING;
            work->timer = HANDLE_REST;
        }
        if (PAD_BUTTONS(pad_records[0].press, pad_jump_buttons)) {
            /* @hack redundant: speed, set from its initializer, and ang, a
             * copy of the player's angles, which the step never reads. the
             * image copies speed's 12-byte template from the constants
             * (0x0cae9c90) and the player's angles onto the frame (0x0c9058ee
             * to 0x0c905912). without them, the constants lose the template,
             * 120 of 227 words differ and the object ends 32 bytes early */
            NJS_VECTOR speed = {0.0F, 2.0F, 0.0F};
            struct angles ang;

            ang.x = player->ang.x;
            ang.y = player->ang.y;
            ang.z = player->ang.z;
            player_set_mode(0, PLAYER_MODE_RESET);
            work->mode = HANDLE_RESTING;
            work->timer = HANDLE_REST;
        }
        break;
    case HANDLE_RESTING:
        --work->timer;
        if (!work->timer) {
            work->mode = HANDLE_EMPTY;
        }
        break;
    }
    collision_register(work);
}

/* the handle's shadow on the floor below it, tilted by the floor's slope,
 * which handle_step keeps in work->ang */
static void handle_shadow(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;

    pos.x = work->pos.x;
    pos.y = FLOOR(work);
    pos.z = work->pos.z;
    shadow_draw_at(&work->ang, &pos, 1.0F);
}

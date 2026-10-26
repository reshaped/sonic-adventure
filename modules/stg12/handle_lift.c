/* O KAITENASHIBA: a platform that its own handle raises and lowers */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg12/handle.h"
#include "stg12/handle_lift.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT handle_lift_object;
extern NJS_OBJECT handle_lift_world_object;
extern NJS_OBJECT handle_object;

#define BODY_WORLD (WORLD_LARGE | WORLD_MOVES)

/* destroyed beyond 1000 units of the camera and player 0 */
#define RANGE (1000.0F * 1000.0F)

/* the collision world tests the platform only while player 0 or 1 is within
 * NEAR of it */
#define NEAR 60.0F

/* the handle's offset from the platform */
#define HANDLE_BACK 15.0F
#define HANDLE_UP 5.0F

/* the most turns each way the handle counts before it starts again from 0 */
#define MOST_TURNS 100.0F

/* each whole turn of the handle moves the platform's aim TURN_RISE. a turn
 * left raises it, and a turn right lowers it. the platform moves toward its
 * aim RISE_SPEED a step up, and SINK_SPEED a step down */
#define TURN_RISE 10.0F
#define RISE_SPEED 0.5F
#define SINK_SPEED 0.75F

/* the platform follows its handle in LIFT_RUN. nothing sets LIFT_STOP */
enum handle_lift_mode { LIFT_RUN = 0, LIFT_STOP = 3 };

/* right and left count the handle's turns the platform has answered */
struct handle_lift_vars {
    NJS_OBJECT *body; /* its collision object, outside the set editor */
    int right;
    int left;
};

struct handle_lift_motion {
    int key; /* the handle's key */
    Uint8 unk_04[4];
    Float bottom; /* the starting height, and the lowest aim */
    Uint8 unk_0c[40];
    Float top; /* the highest aim */
    Float rise; /* how far each turn moves aim */
    Float aim; /* the height the platform moves toward */
};

#define VARS(work) ((struct handle_lift_vars *)&(work)->vars)

static void handle_lift_run(struct task *task);
static void handle_lift_draw(struct task *task);
static void handle_lift_destroy(struct task *task);

void handle_lift_step(struct task *task)
{
    struct task_work *work = task->work;
    struct handle_lift_motion *motion =
        (struct handle_lift_motion *)task->motion;
    struct task_force *force = task->force;
    struct task *child;
    struct task_work *handle;
    NJS_OBJECT *body;

    work->mode = LIFT_RUN;
    motion->key = (int)njAbs(work->scl.x) % HANDLE_KEYS;
    motion->rise = TURN_RISE;
    motion->top = work->pos.y + work->scl.y;
    motion->aim = work->pos.y;
    motion->bottom = work->pos.y;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (!placement_editing(task)) {
        VARS(work)->body = body = world_object_alloc();
        body->evalflags = handle_lift_world_object.evalflags;
        body->scl[0] = 1.0F;
        body->scl[1] = 1.0F;
        body->scl[2] = 1.0F;
        body->ang[0] = 0;
        body->ang[1] = work->ang.y;
        body->ang[2] = 0;
        body->pos[0] = work->pos.x;
        body->pos[1] = work->pos.y;
        body->pos[2] = work->pos.z;
        body->model = handle_lift_world_object.model;
        body->child = NULL;
        body->sibling = NULL;
        world_add(BODY_WORLD, task, body);
        work->flags |= WORLD_TESTED;
        child = task_create_child(TASK_WORK, handle_step, task);
        handle = child->work;
        /* @hack spelling: the x offset negated and added, FNEG and FADD at
         * 0x0c913c0c and 0x0c913c0e; with -=, SHC subtracts and 51 of 168
         * words differ */
        handle->pos.x += -(njSin(work->ang.y) * HANDLE_BACK);
        handle->pos.y += HANDLE_UP;
        /* @hack spelling: the z offset negated and added, FNEG and FADD at
         * 0x0c913c2e and 0x0c913c30; with -=, 34 of 168 words differ */
        handle->pos.z += -(njCos(work->ang.y) * HANDLE_BACK);
        /* the handle reads its key and its most turns from its scale */
        handle->scl.x = work->scl.x;
        handle->scl.y = MOST_TURNS;
        handle->scl.z = MOST_TURNS;
        task_step_children(task);
        handle_get_turns(motion->key, &VARS(work)->right, &VARS(work)->left);
    }
    task->step = handle_lift_run;
    task->draw = handle_lift_draw;
    task->destructor = handle_lift_destroy;
}

static void handle_lift_run(struct task *task)
{
    struct task_work *work = task->work;
    struct handle_lift_motion *motion =
        (struct handle_lift_motion *)task->motion;
    int right;
    int left;
    Float height;
    Float aim;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    handle_lift_draw(task);
    if (placement_editing(task)) {
        Float x;
        Float y;
        Float z;

        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> Key ID");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> Height");
        texture_set_stage();
        njPushMatrix(NULL);
        x = work->pos.x - (njSin(work->ang.y) * HANDLE_BACK);
        y = work->pos.y + HANDLE_UP;
        z = work->pos.z - (njCos(work->ang.y) * HANDLE_BACK);
        njTranslate(NULL, x, y, z);
        TURN(njRotateY, work->ang.y);
        draw_tree(&handle_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njTranslate(NULL, 0.0F, work->scl.y * 0.5F, 0.0F);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, 1.0F, work->scl.y * 0.1F, 1.0F);
        draw_tree(marker_cube_object);
        njPopMatrix(1);
    } else {
        task_step_children(task);
        switch (work->mode) {
        case LIFT_RUN:
            height = work->pos.y;
            aim = motion->aim;
            handle_get_turns(motion->key, &right, &left);
            if (left > VARS(work)->left) {
                VARS(work)->left++;
                aim += motion->rise;
                if (aim >= motion->top) {
                    aim = motion->top;
                }
                motion->aim = aim;
            }
            if (right > VARS(work)->right) {
                VARS(work)->right++;
                aim -= motion->rise;
                if (aim <= motion->bottom) {
                    aim = motion->bottom;
                }
                motion->aim = aim;
            }
            /* @hack redundant: the else's test of height >= aim, which always
             * holds once height < aim fails. SHC tests it with a BT at
             * 0x0c913f00 on the T bit that failure leaves clear, so it never
             * branches; without it, 78 of 272 words differ */
            if (height < aim) {
                height += RISE_SPEED;
                if (height > aim) {
                    height = aim;
                }
            } else if (height >= aim) {
                height -= SINK_SPEED;
                if (height < aim) {
                    height = aim;
                }
            }
            /* player 0 rises and sinks with the platform, wherever player 0
             * is */
            if (height != work->pos.y) {
                NJS_POINT3 player;

                VARS(work)->body->pos[1] = height;
                task->child->work->pos.y = height + HANDLE_UP;
                player_get_history(0, 0, &player, NULL);
                player.y += height - work->pos.y;
                player_set_position(0, player.x, player.y, player.z);
            }
            work->pos.y = height;
            break;
        /* @hack redundant: a case nothing needs, whose CMP/EQ #3,R0 is at
         * 0x0c913dfa; without it, 59 of 272 words differ */
        case LIFT_STOP:
            break;
        }
        if (player_in_range(&work->pos, NEAR)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}

static void handle_lift_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_tree(&handle_lift_object);
        njPopMatrix(1);
    }
}

static void handle_lift_destroy(struct task *task)
{
    struct task_work *work = task->work;

    task_destroy_children(task);
    if (VARS(work)->body != NULL) {
        world_remove(task, VARS(work)->body);
        world_object_free(VARS(work)->body);
    }
}

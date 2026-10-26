/* 0 BLUE LIGHT lamp that turns to shine at the nearest player */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg10/blue_light.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL blue_light_mount_model;
extern NJS_OBJECT blue_light_lamp_object;

/* the lamp is destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

#define TURN_STEP 200 /* the most the lamp turns a step about each axis */
#define TILT_MAX 0x1800 /* the most the lamp tilts from level, up or down */
/* distance between the pivot the lamp tilts about and the line it shines
 * along */
#define LAMP_OFFSET 3.35F

enum light_turn { TURN_Y, TURN_X };

struct light_vars {
    Uint8 unk_00[4];
    /* @hack type: the two turns as one array, whose address SHC takes to read
     * both (ADD #12,R5 at 0x0c90701c). as two fields, SHC reads each through
     * work, FR13 and FR14 trade, and 25 of blue_light_aim's 145 words
     * differ */
    Sint16 turn[2];
};

#define VARS(work) ((struct light_vars *)&(work)->vars)

enum light_mode { LIGHT_INIT, LIGHT_RUN };

static void blue_light_init(struct task *task);
static void blue_light_reset(struct task *task);
static void blue_light_aim(struct task *task);
static void blue_light_draw(struct task *task);
static void blue_light_destroy(struct task *task);

void blue_light_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case LIGHT_INIT:
            blue_light_init(task);
            break;
        case LIGHT_RUN:
            blue_light_draw(task);
            blue_light_aim(task);
            break;
        }
    } else {
        switch (work->mode) {
        case LIGHT_INIT:
            blue_light_init(task);
            break;
        case LIGHT_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            blue_light_draw(task);
            blue_light_aim(task);
            break;
        }
    }
}

static void blue_light_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = blue_light_draw;
    task->destructor = blue_light_destroy;
    blue_light_reset(task);
    work->mode = LIGHT_RUN;
}

static void blue_light_reset(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->turn[TURN_Y] = 0;
    VARS(work)->turn[TURN_X] = 0;
}

/* turn toward the nearest player's eyes, at most TURN_STEP a step about each
 * axis. the lamp has no collision record, so the collision_register at the
 * end registers nothing */
static void blue_light_aim(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 to;
    int player = player_nearest(&work->pos);
    Float x;
    Float y;
    Float z;
    Float distance;
    int turn_y;
    int turn_x;
    int by_y;
    int by_x;

    to = player_work[player]->pos;
    to.y += player_get(player)->consts.eye_height;
    njSubVector(&to, &work->pos);
    x = to.x;
    y = to.y;
    z = to.z;
    turn_y = (int)(atan2f(x, z) * ANGLE_PER_RADIAN);
    distance = njSqrt((x * x) + (y * y) + (z * z));
    turn_x = (int)(acosf(LAMP_OFFSET / distance) * ANGLE_PER_RADIAN) -
             (int)(acosf(-y / distance) * ANGLE_PER_RADIAN);
    if (turn_x > TILT_MAX) {
        turn_x = TILT_MAX;
    } else if (turn_x < -TILT_MAX) {
        turn_x = -TILT_MAX;
    }
    by_y = (Sint16)(turn_y - VARS(work)->turn[TURN_Y]);
    by_x = (Sint16)(turn_x - VARS(work)->turn[TURN_X]);
    if (by_y > TURN_STEP) {
        VARS(work)->turn[TURN_Y] += TURN_STEP;
    } else if (by_y < -TURN_STEP) {
        VARS(work)->turn[TURN_Y] -= TURN_STEP;
    } else {
        VARS(work)->turn[TURN_Y] = turn_y;
    }
    if (by_x > TURN_STEP) {
        VARS(work)->turn[TURN_X] += TURN_STEP;
    } else if (by_x < -TURN_STEP) {
        VARS(work)->turn[TURN_X] -= TURN_STEP;
    } else {
        VARS(work)->turn[TURN_X] = turn_x;
    }
    collision_register(work);
}

static void blue_light_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_model(&blue_light_mount_model);
        TURN_VIA(njRotateY, VARS(work)->turn[TURN_Y], ang);
        TURN_VIA(njRotateX, VARS(work)->turn[TURN_X], ang);
        draw_tree(&blue_light_lamp_object);
        njPopMatrix(1);
    }
}

static void blue_light_destroy(struct task *task)
{
}

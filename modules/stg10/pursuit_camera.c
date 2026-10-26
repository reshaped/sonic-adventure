/* O PURS_CAMERA camera that turns to watch the nearest player */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg10/pursuit_camera.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL pursuit_camera_mount_model;
/* @hack type: pursuit_camera_body_object as an array. SHC then pools its
 * address and adds 8 to reach pos (ADD #8,R12 at 0x0c906a78). as one
 * NJS_OBJECT, SHC pools pos's address, pursuit_camera_draw runs 2 bytes short
 * and 54 of its 81 words differ */
extern NJS_OBJECT pursuit_camera_body_object[];
extern NJS_MODEL pursuit_camera_body_model;
/* @hack type: pursuit_camera_lens_object as an array, as the body is (ADD
 * #8,R14 at 0x0c906ab2). as one NJS_OBJECT, pursuit_camera_draw runs 2 bytes
 * short and 25 of its 81 words differ */
extern NJS_OBJECT pursuit_camera_lens_object[];
extern NJS_MODEL pursuit_camera_lens_model;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

#define TURN_STEP 200 /* the most the camera turns a step about each axis */
#define TILT_MAX 0x1800 /* the most the camera tilts from level, up or down */
/* distance between the pivot the camera tilts about and the line the lens
 * looks along */
#define LENS_OFFSET 3.35F

enum camera_turn { TURN_Y, TURN_X };

struct camera_vars {
    Uint8 unk_00[4];
    /* @hack type: the two turns as one array, whose address SHC takes to read
     * both (ADD #12,R5 at 0x0c90695c). as two fields, SHC reads each through
     * work, FR13 and FR14 trade, and 25 of pursuit_camera_aim's 145 words
     * differ */
    Sint16 turn[2];
};

#define VARS(work) ((struct camera_vars *)&(work)->vars)

enum camera_mode { CAMERA_INIT, CAMERA_RUN };

static void pursuit_camera_init(struct task *task);
static void pursuit_camera_reset(struct task *task);
static void pursuit_camera_aim(struct task *task);
static void pursuit_camera_draw(struct task *task);
static void pursuit_camera_destroy(struct task *task);

void pursuit_camera_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case CAMERA_INIT:
            pursuit_camera_init(task);
            break;
        case CAMERA_RUN:
            pursuit_camera_draw(task);
            pursuit_camera_aim(task);
            break;
        }
    } else {
        switch (work->mode) {
        case CAMERA_INIT:
            pursuit_camera_init(task);
            break;
        case CAMERA_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            pursuit_camera_draw(task);
            pursuit_camera_aim(task);
            break;
        }
    }
}

static void pursuit_camera_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = pursuit_camera_draw;
    task->destructor = pursuit_camera_destroy;
    pursuit_camera_reset(task);
    work->mode = CAMERA_RUN;
}

static void pursuit_camera_reset(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->turn[TURN_Y] = 0;
    VARS(work)->turn[TURN_X] = 0;
}

/* turn toward the nearest player's eyes, at most TURN_STEP a step about each
 * axis. the camera has no collision record, so the collision_register at the
 * end registers nothing */
static void pursuit_camera_aim(struct task *task)
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
    turn_x = (int)(acosf(LENS_OFFSET / distance) * ANGLE_PER_RADIAN) -
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

static void pursuit_camera_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_model(&pursuit_camera_mount_model);
        njTranslate(NULL, pursuit_camera_body_object->pos[0],
                    pursuit_camera_body_object->pos[1],
                    pursuit_camera_body_object->pos[2]);
        TURN_VIA(njRotateY, VARS(work)->turn[TURN_Y], ang);
        TURN_VIA(njRotateX, VARS(work)->turn[TURN_X], ang);
        draw_model(&pursuit_camera_body_model);
        njTranslate(NULL, pursuit_camera_lens_object->pos[0],
                    pursuit_camera_lens_object->pos[1],
                    pursuit_camera_lens_object->pos[2]);
        draw_model(&pursuit_camera_lens_model);
        njPopMatrix(1);
    }
}

static void pursuit_camera_destroy(struct task *task)
{
}

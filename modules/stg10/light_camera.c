/* 0 LIGHT CAMERA camera that turns to watch the nearest player, with a lamp
 * on its lens */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg10/light_camera.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL light_camera_mount_model;
/* @hack type: the body declared as an array, and read through ->. SHC then
 * loads the body's address and reads its pos and model through it (ADD #8,R13
 * at 0x0c906dd6, MOV.L @(4,R4),R4 at 0x0c906e0a). as one NJS_OBJECT read
 * through ., 44 of light_camera_draw's 70 words differ */
extern NJS_OBJECT light_camera_body_object[];
/* the lens, whose tree holds the lamp too */
extern NJS_OBJECT light_camera_lens_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* the most the camera turns each step, about each axis */
#define TURN_STEP 200
#define TILT_MAX 0x1800 /* the most the camera tilts, up or down */
/* distance between the pivot the camera tilts about and the line the lens
 * looks along */
#define LENS_OFFSET 3.35F

/* the indexes of camera_vars' turn: its pan about y, and its tilt about x */
enum camera_turn { PAN, TILT };

struct camera_vars {
    Uint8 unk_00[4];
    /* @hack type: the turns declared as an array. SHC then takes turn's
     * address once and reads both turns through it (ADD #12,R5 at
     * 0x0c906cdc). as two Sint16 fields, 29 of light_camera_aim's 142 words
     * differ */
    Sint16 turn[2];
};

#define VARS(work) ((struct camera_vars *)&(work)->vars)

enum camera_mode { CAMERA_INIT, CAMERA_RUN };

static void light_camera_init(struct task *task);
static void light_camera_reset(struct task *task);
static void light_camera_aim(struct task *task);
static void light_camera_draw(struct task *task);
static void light_camera_destroy(struct task *task);

void light_camera_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case CAMERA_INIT:
            light_camera_init(task);
            break;
        case CAMERA_RUN:
            light_camera_draw(task);
            light_camera_aim(task);
            break;
        }
    } else {
        switch (work->mode) {
        case CAMERA_INIT:
            light_camera_init(task);
            break;
        case CAMERA_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            light_camera_draw(task);
            light_camera_aim(task);
            break;
        }
    }
}

static void light_camera_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = light_camera_draw;
    task->destructor = light_camera_destroy;
    light_camera_reset(task);
    work->mode = CAMERA_RUN;
}

static void light_camera_reset(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->turn[PAN] = 0;
    VARS(work)->turn[TILT] = 0;
}

static void light_camera_aim(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 to;
    int player = player_nearest(&work->pos);
    Float x;
    Float y;
    Float z;
    Float distance;
    int aim_y;
    int aim_x;
    int by_y;
    int by_x;

    to = player_work[player]->pos;
    to.y += player_get(player)->consts.eye_height;
    njSubVector(&to, &work->pos);
    y = to.y;
    z = to.z;
    x = to.x;
    aim_y = (int)(atan2f(x, z) * ANGLE_PER_RADIAN);
    distance = njSqrt((x * x) + (y * y) + (z * z));
    aim_x = (int)(acosf(LENS_OFFSET / distance) * ANGLE_PER_RADIAN) -
            (int)(acosf(-y / distance) * ANGLE_PER_RADIAN);
    if (aim_x > TILT_MAX) {
        aim_x = TILT_MAX;
    } else if (aim_x < -TILT_MAX) {
        aim_x = -TILT_MAX;
    }
    by_y = (Sint16)(aim_y - VARS(work)->turn[PAN]);
    by_x = (Sint16)(aim_x - VARS(work)->turn[TILT]);
    if (by_y > TURN_STEP) {
        VARS(work)->turn[PAN] += TURN_STEP;
    } else if (by_y < -TURN_STEP) {
        VARS(work)->turn[PAN] -= TURN_STEP;
    } else {
        VARS(work)->turn[PAN] = aim_y;
    }
    if (by_x > TURN_STEP) {
        VARS(work)->turn[TILT] += TURN_STEP;
    } else if (by_x < -TURN_STEP) {
        VARS(work)->turn[TILT] -= TURN_STEP;
    } else {
        VARS(work)->turn[TILT] = aim_x;
    }
}

static void light_camera_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_model(&light_camera_mount_model);
        njTranslate(NULL, light_camera_body_object->pos[0],
                    light_camera_body_object->pos[1],
                    light_camera_body_object->pos[2]);
        TURN_VIA(njRotateY, VARS(work)->turn[PAN], ang);
        TURN_VIA(njRotateX, VARS(work)->turn[TILT], ang);
        draw_model(light_camera_body_object->model);
        draw_tree(&light_camera_lens_object);
        njPopMatrix(1);
    }
}

static void light_camera_destroy(struct task *task)
{
}

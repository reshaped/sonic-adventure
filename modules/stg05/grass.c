/* O MT KUSA, a tuft of grass that sways as player 0 runs through it */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/mesh.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg05/grass.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_ACTION mountain_grass_action;
/* the tuft standing still */
/* @hack type: an array of one, whose address mountain_grass_draw loads into
 * R4 (0x0c91012e); as a single object, the pool holds its model member's
 * address and 6 of 68 words differ */
extern NJS_OBJECT mountain_grass_object[1];

/* the only act in which the grass runs: act 2, the third */
#define GRASS_ACT 2

/* the step does nothing in GRASS_GONE, which nothing in this file sets */
enum mountain_grass_mode { GRASS_INIT, GRASS_SWAY, GRASS_GONE };

struct mountain_grass_vars {
    Float frame;
    /* player 0's speed ahead, as of the last step it was on the ground */
    Float speed;
};

#define VARS(work) ((struct mountain_grass_vars *)&(work)->vars)

/* the sway plays up to this frame, then starts again from 0 */
#define SWAY_LAST_FRAME 14.0F
/* the frames the sway advances each step, by player 0's speed */
#define SWAY_STILL 0.1F /* player 0 stands still, at any distance */
#define SWAY_SLOW 0.5F /* below its sprint speed */
#define SWAY_FAST 1.0F /* at its sprint speed or faster */

/* player 0 sways the grass while it moves this near */
#define NEAR_SLOW 15.0F /* across, below its sprint speed */
#define NEAR_FAST 30.0F /* across, at its sprint speed or faster */
#define NEAR_HEIGHT 10.0F /* in height, at either speed */

#define DRAW_RADIUS 5.0F
/* the sway draws while the camera is nearer than 20 across, and the still
 * tuft beyond. mesh_point_distance2_across measures it squared */
#define SWAY_SEEN (20.0F * 20.0F)

static void mountain_grass_init(struct task *task);
static void mountain_grass_sway(struct task *task);
static void mountain_grass_draw(struct task *task);
static void mountain_grass_destroy(struct task *task);

void mountain_grass_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task) &&
        (Uint8)stage_get_stage_act() == GRASS_ACT) {
        switch (work->mode) {
        case GRASS_INIT:
            mountain_grass_init(task);
            break;
        case GRASS_SWAY:
            mountain_grass_sway(task);
            mountain_grass_draw(task);
            break;
        case GRASS_GONE:
            return;
        default:
            work->mode = GRASS_INIT;
            break;
        }
    }
}

static void mountain_grass_init(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame = 0.0F;
    task->destructor = mountain_grass_destroy;
    task->draw = mountain_grass_draw;
    work->mode = GRASS_SWAY;
}

/* advance the sway by player 0's speed. the grass sways slowly while
 * player 0 stands still anywhere, and faster while player 0 moves near it */
static void mountain_grass_sway(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct player *record;
    Float across;
    Float height;

    across =
        njSqrt(((player->pos.x - work->pos.x) * (player->pos.x - work->pos.x)) +
               ((player->pos.z - work->pos.z) * (player->pos.z - work->pos.z)));
    height = _builtin_fabsf(player->pos.y - work->pos.y);
    record = player_get(0);
    player_ground_speed(0, &VARS(work)->speed);
    if (VARS(work)->speed == 0.0F) {
        if (VARS(work)->frame == SWAY_LAST_FRAME) {
            VARS(work)->frame = 0.0F;
        } else {
            VARS(work)->frame += SWAY_STILL;
        }
        if (VARS(work)->frame > SWAY_LAST_FRAME) {
            VARS(work)->frame = SWAY_LAST_FRAME;
        }
    } else if (VARS(work)->speed >= 0.0F &&
               VARS(work)->speed < record->consts.sprint_speed) {
        if (across <= NEAR_SLOW && height <= NEAR_HEIGHT) {
            if (VARS(work)->frame == SWAY_LAST_FRAME) {
                VARS(work)->frame = 0.0F;
            } else {
                VARS(work)->frame += SWAY_SLOW;
            }
            if (VARS(work)->frame > SWAY_LAST_FRAME) {
                VARS(work)->frame = SWAY_LAST_FRAME;
            }
        }
    } else if (VARS(work)->speed >= record->consts.sprint_speed) {
        if (across <= NEAR_FAST && height <= NEAR_HEIGHT) {
            if (VARS(work)->frame == SWAY_LAST_FRAME) {
                VARS(work)->frame = 0.0F;
            } else {
                VARS(work)->frame += SWAY_FAST;
            }
            if (VARS(work)->frame > SWAY_LAST_FRAME) {
                VARS(work)->frame = SWAY_LAST_FRAME;
            }
        }
    }
}

static void mountain_grass_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        if (mesh_point_distance2_across(&camera_work->pos, &work->pos) <
            SWAY_SEEN) {
            draw_action(&mountain_grass_action, VARS(work)->frame);
        } else {
            draw_model(mountain_grass_object[0].model);
        }
        njPopMatrix(1);
    }
}

static void mountain_grass_destroy(struct task *task)
{
}

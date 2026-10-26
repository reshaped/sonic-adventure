/* C SCENECHANGER: a box that changes the stage or act once touched */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/debug.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/stream.h"
#include "stage/scene_changer.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* half the box's size along each axis at scale 0. a placement's scale s
 * makes it (s + 1) * BOX_HALF_SIZE */
#define BOX_HALF_SIZE 5.0F

/* the box at scale 0. collision_init copies it for each task, which scales
 * its copy */
static struct collision_shape scene_changer_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 0.0F, 0.0F},
    BOX_HALF_SIZE,
    BOX_HALF_SIZE,
    BOX_HALF_SIZE,
    {0},
    0,
    NULL};

static void scene_changer_step(struct task *task);
static void scene_changer_destroy(struct task *task);

void scene_changer_go(int destination)
{
    Sint16 stage;
    int act;
    Sint16 current_stage;
    int current_act;
    int by;

    stage_set_entrance((destination >> 4) & 0xf);
    stage = (destination >> 8) & 0xff;
    act = destination & 0xf;
    current_stage = stage_get_stage_act() >> 8;
    current_act = stage_get_stage_act() & 0xff;
    if (stage == current_stage) {
        by = act - current_act;
        stream_destroy();
        world_change_act(by);
        placement_change_act(by);
        camera_change_act(by);
        stage_change_act(by);
        stage_place_player(player_work[0]);
    } else {
        stage_end(STAGE_END_CLEAR);
        stage_set_next_stage_act(stage, act);
    }
}

void scene_changer_init(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float y;
    Float z;

    if (!placement_editing(task)) {
        collision_init(task, &scene_changer_shape, COLLISION_PICKUP, 1, 1);
        /* @hack spelling: each scale's sum inside its product, which SHC
         * multiplies from the register that holds the sum (FMOV FR3,FR4 at
         * 0x8c035af8). as statements, SHC loads the x and z scales again
         * after their stores, and 51 of 74 words differ */
        x = (work->scl.x += 1.0F) * BOX_HALF_SIZE;
        y = (work->scl.y += 1.0F) * BOX_HALF_SIZE;
        z = (work->scl.z += 1.0F) * BOX_HALF_SIZE;
        work->collision->shapes->radius = x;
        /* @hack spelling: the height's store inside the sum, which keeps x's
         * product in FR4 and y's in FR5 (0x8c035afa, 0x8c035b06). as two
         * statements, the two trade, and 52 of 74 words differ */
        work->pos.y += (work->collision->shapes->size_y = y);
        work->collision->shapes->size_z = z;
        /* radius takes the sum of the half sizes' squares, where
         * collision_set_radius would take its square root. the radius then
         * reaches past the box's corners while they lie more than 1 from its
         * center */
        work->collision->radius = (x * x) + (y * y) + (z * z);
    }
    task->destructor = scene_changer_destroy;
    task->step = scene_changer_step;
}

static void scene_changer_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        /* @hack spelling: pad_is_enabled compared with TRUE, CMP/EQ #1
         * (0x8c035b88). it returns pads_on, which only ever holds 1 or 0. as a
         * plain test, SHC writes TST and 2 of 144 words differ */
        if ((work->collision->flags & COLLISION_HIT) &&
            pad_is_enabled() == TRUE) {
            scene_changer_go(work->ang.z);
            task_kill(task);
        } else {
            collision_register(work);
        }
    } else {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATION Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> STAGE & ENTRANCE & ACT");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> SCALE X");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> SCALE Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> SCALE Z");
        debug_print_stage_name(NJM_LOCATION(17, 22), work->ang.z);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x,
                    work->pos.y + ((work->scl.y + 1.0F) * BOX_HALF_SIZE),
                    work->pos.z);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F,
                work->scl.z + 1.0F);
        draw_set_material(0.1F, 1.0F, 0.8F, 0.2F);
        draw_tree(&marker_box_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

static void scene_changer_destroy(struct task *task)
{
}

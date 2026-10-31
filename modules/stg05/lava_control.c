/* MagmaCnt, a box that sets the lava rising the first time player 0 touches
 * it. its set entry gives the level and the speed */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/helpers.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg05/lava_control.h"
#include "stg05/mountain.h"
#include "system/task.h"
#include "system/util.h"

enum lava_control_mode {
    LAVA_CONTROL_INIT,
    LAVA_CONTROL_WAIT,
    LAVA_CONTROL_DONE
};

/* the box is a cube whose half size is scale x plus this margin */
#define LAVA_CONTROL_MARGIN 10.0F
#define ARROW_SCALE 5.0F

/* from the set entry: the level the lava rises to, and its rise each step */
#define RISE_TO(work) ((work)->scl.y)
#define RISE_SPEED(work) ((work)->scl.z)

/* init sizes the record's copy from scale x and turns it as the task faces */
static struct collision_shape lava_control_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    40.0F,
    30.0F,
    10.0F,
    {0, 0, 0},
    0,
    NULL,
};

/* while the set editor is open, the box only draws its marker cube and an
 * arrow. the arrow stands work->ang.x above the box. while the editor holds
 * the box, the step labels scale y and z as the level and speed */
void lava_control_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *player;
    Float size;
    Float scale;
    Angle ang;
    /* @hack redundant: an array whose address the step takes and never uses,
     * which reserves 8 bytes of the frame and keeps the calls that end the
     * step as calls; without it, the function is 22 bytes longer and 121 of
     * 207 words differ */
    Float unused[2];

    (void)&unused;
    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- MAX YPOS");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<- UP SPEED");
    }
    if (placement_editing(task) || edit_mode == 1) {
        size = work->scl.x + LAVA_CONTROL_MARGIN;
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        scale = size * CUBE_FIT;
        njScale(NULL, scale, scale, scale);
        draw_tree(marker_cube_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njTranslate(NULL, 0.0F, work->ang.x, 0.0F);
        njScale(NULL, ARROW_SCALE, ARROW_SCALE, ARROW_SCALE);
        draw_tree(&arrow_y);
        njPopMatrix(1);
        return;
    }
    switch (work->mode) {
    case LAVA_CONTROL_INIT:
        collision_init(task, &lava_control_shape, COLLISION_OBJECT, 1, 1);
        work->collision->shapes->radius = work->scl.x + LAVA_CONTROL_MARGIN;
        work->collision->shapes->size_y = work->scl.x + LAVA_CONTROL_MARGIN;
        work->collision->shapes->size_z = work->scl.x + LAVA_CONTROL_MARGIN;
        work->collision->shapes->ang.y = work->ang.y;
        work->mode = LAVA_CONTROL_WAIT;
        break;
    case LAVA_CONTROL_WAIT:
        if (work->collision->flags & COLLISION_HIT) {
            player = player_get_task(0);
            if (work->collision->hit->owner == player) {
                if (mountain_work) {
                    mountain_work->scl.x = RISE_TO(work);
                    mountain_work->scl.y = RISE_SPEED(work);
                }
                work->mode = LAVA_CONTROL_DONE;
            }
        }
        collision_register(work);
        break;
    case LAVA_CONTROL_DONE:
        placement_kill(task);
        break;
    }
}

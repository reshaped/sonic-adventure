/* O BIRIBIRI, an electric spark. a sprite of lightning over its model
 * flickers through eight textures */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/spark.h"
#include "stg12/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT shelter_spark_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* the sprite's frames, one a step. each of the eight textures shows for
 * two */
#define SPARK_FRAMES 16

enum shelter_spark_mode { SPARK_INIT, SPARK_RUN };

struct shelter_spark_vars {
    Uint8 frame; /* the frame of shelter_spark_frames the sprite shows */
};

#define VARS(work) ((struct shelter_spark_vars *)&(work)->vars)

static void shelter_spark_init(struct task *task);
static void shelter_spark_draw(struct task *task);
static void shelter_spark_destroy(struct task *task);
static void shelter_spark_advance(struct task_work *work);

static struct collision_shape shelter_spark_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {2.0F, 2.0F, 0.0F},
    3.0F,
    2.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static NJS_TEXANIM shelter_spark_frames[SPARK_FRAMES] = {
#include "stg12/spark/shelter_spark_frames.inc"
};

static NJS_SPRITE shelter_spark_sprite = {
#include "stg12/spark/shelter_spark_sprite.inc"
};

/* @hack spelling: collision_register through an inline function, which reads
 * task->work again at each call (MOV.L @(32,R4),R4 at 0x0c901a86 and
 * 0x0c901aca). called with shelter_spark_step's own work, the step saves R12
 * and jumps to collision_register. it then runs 16 bytes longer, and 69 of
 * its 74 words differ */
static void shelter_spark_register(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void shelter_spark_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            switch (work->mode) {
            case SPARK_INIT:
                shelter_spark_init(task);
                break;
            case SPARK_RUN:
                shelter_spark_advance(work);
                shelter_spark_draw(task);
                shelter_spark_register(task);
                break;
            }
        } else {
            switch (work->mode) {
            case SPARK_INIT:
                shelter_spark_init(task);
                break;
            case SPARK_RUN:
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
                shelter_spark_draw(task);
                shelter_spark_register(task);
                break;
            }
        }
    }
}

static void shelter_spark_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = shelter_spark_draw;
    task->destructor = shelter_spark_destroy;
    collision_init(task, &shelter_spark_shape, COLLISION_OBJECT, 1, 1);
    VARS(work)->frame = 0;
    work->mode = SPARK_RUN;
}

static void shelter_spark_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&shelter_spark_object);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njDrawSprite3D(&shelter_spark_sprite, VARS(work)->frame,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_ANGLE);
        njPopMatrix(1);
    }
}

static void shelter_spark_destroy(struct task *task)
{
}

static void shelter_spark_advance(struct task_work *work)
{
    VARS(work)->frame++;
    /* @hack spelling: the frame compared with 15 by CMP/GT (0x0c901bb4,
     * 0x0c901bbe); as >= SPARK_FRAMES, it is compared with 16 by CMP/GE, and
     * 2 of 12 words differ */
    if (VARS(work)->frame > SPARK_FRAMES - 1) {
        VARS(work)->frame = 0;
    }
}

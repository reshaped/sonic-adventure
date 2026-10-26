/* O SETIFF: a source of debris that swirls up about the world's y axis */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg02/swirl.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL windy_breakable_model;
extern NJS_MODEL windy_breakable_piece_1_model;
extern NJS_MODEL windy_breakable_piece_2_model;
extern NJS_MODEL windy_breakable_piece_3_model;
extern NJS_MODEL windy_breakable_piece_4_model;
extern NJS_MODEL windy_breakable_piece_5_model;
extern NJS_MODEL windy_breakable_piece_6_model;
extern NJS_MODEL windy_breakable_piece_7_model;
extern NJS_MODEL windy_breakable_piece_8_model;
extern NJS_MODEL windy_bridge_piece_6_model;

/* a piece's work->vars. its work->scl.y is random, from 0 up to 1. it speeds
 * up the piece's swirl and climb, and sets how fast it tumbles */
struct swirl_piece_vars {
    Float angle; /* about the world's y axis, in degrees */
    Uint8 unk_04[4];
    NJS_MODEL *model;
};

#define PIECE(work) ((struct swirl_piece_vars *)&(work)->vars)

/* the modes of the source, a burst and a piece. in SWIRL_END each step does
 * nothing, and no step here sets it */
enum swirl_mode { SWIRL_START, SWIRL_RUN, SWIRL_END };

/* the squared range. the source destroys itself once the camera and player 0
 * are farther than 1200 from it */
#define SWIRL_RANGE (1200.0F * 1200.0F)

/* a burst starts once the source's timer passes scl.y + BURST_WAIT */
#define BURST_WAIT 40.0F
#define SWIRL_PIECES 10
#define BURST_STEPS 150 /* the steps a burst's pieces run */

/* a piece circles SWIRL_RADIUS from the world's y axis. it moves only on a
 * frame's first step, while it is in view. it must also be below the camera,
 * and higher than PIECE_BELOW under player 0 */
#define SWIRL_RADIUS 100.0F
#define PIECE_BELOW 70.0F

static NJS_MODEL *windy_swirl_models[SWIRL_PIECES] = {
    &windy_breakable_model,         &windy_breakable_piece_1_model,
    &windy_breakable_piece_2_model, &windy_breakable_piece_3_model,
    &windy_breakable_piece_4_model, &windy_breakable_piece_5_model,
    &windy_breakable_piece_6_model, &windy_breakable_piece_7_model,
    &windy_breakable_piece_8_model, &windy_bridge_piece_6_model,
};

static void windy_swirl_piece_step(struct task *task);
static void windy_swirl_piece_draw(struct task *task);

static void windy_swirl_burst_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    int i;

    switch (work->mode) {
    case SWIRL_START:
        work->mode = SWIRL_RUN;
        for (i = 0; i < SWIRL_PIECES; i++) {
            child = task_create_child(TASK_WORK, windy_swirl_piece_step, task);
            PIECE(child->work)->model = windy_swirl_models[i];
            child->work->pos.y = work->pos.y;
        }
        break;
    case SWIRL_RUN:
        work->timer++;
        if (TASK_TIMER(work) < BURST_STEPS) {
            task_step_children(task);
        } else {
            task_destroy_children(task);
            task_kill(task);
        }
        break;
    }
}

static void windy_swirl_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    Float random;
    Angle ang;

    switch (work->mode) {
    case SWIRL_START:
        work->mode = SWIRL_RUN;
        random = rand() * RAND_SCALE;
        work->scl.y = _builtin_fabsf(random);
        work->pos.x = 0.0F;
        work->pos.z = 0.0F;
        PIECE(work)->angle = (int)(random * 360.0F);
        break;
    case SWIRL_RUN:
        if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 3.0F)) {
            if (player_work[0]->pos.y - PIECE_BELOW < work->pos.y &&
                work->pos.y < camera_work->pos.y) {
                PIECE(work)->angle += (int)((work->scl.y * 1.5F) + 8.0F);
                ang = (int)(-(PIECE(work)->angle * ANGLE_PER_DEGREE));
                work->pos.x = njCos(ang) * SWIRL_RADIUS;
                work->pos.z = njSin(ang) * SWIRL_RADIUS;
                work->pos.y += (_builtin_fabsf(rand() * RAND_SCALE) * 3.4F) +
                               (work->scl.y * 1.3F);
                work->ang.x += (int)(work->scl.y * (ANGLE_PER_DEGREE * 2.0F));
                work->ang.y += (int)(work->scl.y * (ANGLE_PER_DEGREE * 5.0F));
                work->ang.z += (int)(work->scl.y * (ANGLE_PER_DEGREE * 2.0F));
                windy_swirl_piece_draw(task);
                return;
            }
        }
        break;
    case SWIRL_END:
        break;
    }
}

static void windy_swirl_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MODEL *model = PIECE(work)->model;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        draw_model(model);
        njPopMatrix(1);
    }
}

/* the source starts a burst at its height once its timer passes scl.y +
 * BURST_WAIT. it starts none while the set editor holds it. it draws as a
 * marker then, and while edit_mode is 1 */
void windy_swirl_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *burst;

    if (placement_try_leave_by(task, SWIRL_RANGE)) {
        return;
    }
    switch (work->mode) {
    case SWIRL_START:
        work->mode = SWIRL_RUN;
        break;
    case SWIRL_RUN:
        if (!placement_editing(task)) {
            if (TASK_TIMER(work)++ > work->scl.y + BURST_WAIT) {
                burst = task_create(TASK_WORK, 3, windy_swirl_burst_step);
                burst->work->pos.y = work->pos.y;
                work->timer = 0;
            }
        }
        if (placement_editing(task) || edit_mode == 1) {
            njPushMatrix(NULL);
            njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
            njScale(NULL, 1.0F, 1.0F, 1.0F);
            draw_tree(&marker_object);
            njPopMatrix(1);
        }
        break;
    case SWIRL_END:
        break;
    }
}

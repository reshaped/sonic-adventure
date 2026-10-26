/* O MB mirror ball that spins and turns the act's light with it */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "stg09/casino.h"
#include "stg09/mirror_ball.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT mirror_ball_object;

/* the act light's turn about y. save/card.c defines it static. this image
 * links to it through 1ST_READ.BIN's symbols, which name its address */
extern Angle light_ry;

/* destroyed beyond about 700 units of the camera and player 0 */
#define RANGE ((700.0F * 700.0F) + 10.0F)
/* the radius of the sphere about the ball that draw_in_view tests */
#define VIEW_RADIUS 40.0F
/* the ball's turn each step, in degrees */
#define TURN_DEGREES(work) ((work)->scl.x + 2.0F)

struct mirror_ball_vars {
    Uint8 unk_00[8];
    Sint16 stage_act; /* the stage and act the ball started in */
};

#define VARS(work) ((struct mirror_ball_vars *)&(work)->vars)
/* light_ry's low byte as the ball started */
#define LIGHT_KEPT(work) ((work)->submode)

static struct collision_shape mirror_ball_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    33.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

static void mirror_ball_destroy(struct task *task);
static void mirror_ball_draw(struct task *task);
static void mirror_ball_run(struct task *task);

void mirror_ball_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        task->draw = mirror_ball_draw;
        task->step = mirror_ball_run;
        task->destructor = mirror_ball_destroy;
        if (!placement_editing(task)) {
            collision_init(task, &mirror_ball_shape, COLLISION_OBJECT, 1, 1);
        }
        VARS(work)->stage_act = stage_get_stage_act();
        /* @bug arithmetic: submode, a Sint8, keeps only light_ry's low byte
         * (MOV.B @R3,R0 at 0x0c917bc8). mirror_ball_destroy gives that byte
         * back sign-extended (MOV.B @(1,R14),R0 at 0x0c917bf4), so a turn
         * outside -128 to 127 comes back as another */
        LIGHT_KEPT(work) = light_ry;
    }
}

/* outside the set editor, in the act the ball started in, give light_ry
 * back LIGHT_KEPT */
static void mirror_ball_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        if (VARS(work)->stage_act == stage_get_stage_act()) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            light_ry = LIGHT_KEPT(work);
        }
    }
}

static void mirror_ball_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
            casino_set_texture(CASINO_TEXTURES_ACTS_12);
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, work->ang.y);
            draw_tree(&mirror_ball_object);
            njPopMatrix(1);
        }
    }
}

static void mirror_ball_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        mirror_ball_draw(task);
        work->ang.y += (Angle)(TURN_DEGREES(work) * ANGLE_PER_DEGREE);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> RotSpd ");
            return;
        }
        collision_register(work);
        if (VARS(work)->stage_act == stage_get_stage_act()) {
            light_ry = work->ang.y;
        }
    }
}

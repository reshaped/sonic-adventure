/* the turn cube: TurnCube, which turns about its x axis. it rests at each
 * quarter turn for the stop time its set entry gives */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/placement.h"
#include "stg07/turn_cube.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT turn_cube_object;

struct turn_cube_vars {
    Uint16 stop_time; /* the steps it rests at each quarter turn, 0 for none */
    Uint8 unk_02[6];
    Angle speed; /* its turn each step */
};

#define VARS(work) ((struct turn_cube_vars *)&(work)->vars)

enum turn_cube_mode { CUBE_TURN, CUBE_REST };

/* which quarter of a turn an angle is in, and how far into it */
#define QUARTER(ang) ((ang) & 0xc000)
#define INTO_QUARTER(ang) ((ang) & 0x3fff)
/* the quarter turn nearest an angle */
#define NEAREST_QUARTER(ang) QUARTER((ang) + 0x2000)

/* clang-format off */
static struct collision_shape turn_cube_shape = {
    SHAPE_RECTANGLE2, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F},
    25.0F, 25.0F, 25.0F, {0, 0x4000, 0}, 0, NULL,
};
/* clang-format on */

/* in the set editor, the work keeps the set entry's angles and scale. they hold
 * the speed and stop time. each step saves the angles here while the cube
 * turns, and puts them back after */
static struct angles turn_cube_edit_ang;
/* the set entry's scale, saved the same way */
static NJS_POINT3 turn_cube_edit_scl;
/* the x angle the cube turned to on the set editor's last step */
static Angle turn_cube_edit_turn;

/* take the stop time from the set entry's x scale, and the speed from its
 * x angle. then clear the scale, and the x and z angles */
static void turn_cube_read_set(struct task *task)
{
    struct task_work *work = task->work;

    if (work->scl.x > 0.0F) {
        VARS(work)->stop_time = (int)work->scl.x;
    } else {
        VARS(work)->stop_time = 0;
    }
    VARS(work)->speed = work->ang.x;
    work->ang.x = work->ang.z = 0;
    work->scl.x = work->scl.y = work->scl.z = 0.0F;
}

static void turn_cube_destroy(struct task *task)
{
}

static void turn_cube_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    texture_set_stage();
    njSimpleDrawObject(&turn_cube_object);
    njPopMatrix(1);
}

static void turn_cube_stop(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->stop_time > 0) {
        work->mode = CUBE_REST;
        TASK_TIMER(work) = VARS(work)->stop_time;
    }
}

static void turn_cube_go(struct task *task)
{
    task->work->mode = CUBE_TURN;
}

/* turn the cube by its speed. a turn that passes a quarter turn ends on the
 * nearest one. the cube rests there if it has a stop time. a turn that
 * starts on a quarter turn goes on */
static void turn_cube_turn(struct task *task)
{
    struct task_work *work = task->work;
    Angle from;
    Angle to;

    /* @hack spelling: from copied into to before the speed is added, which
     * loads the mask into R5 and from into R6, then copies from into R14
     * (0x0c903744 to 0x0c90374e). as to = from + speed, the two trade, the
     * function is a word shorter, and 24 of turn_cube_turn's 28 words
     * differ */
    to = from = work->ang.x;
    to += VARS(work)->speed;
    if (QUARTER(to) != QUARTER(from) && INTO_QUARTER(from)) {
        to = NEAREST_QUARTER(to);
        turn_cube_stop(task);
    }
    work->ang.x = to;
}

static void turn_cube_rest(struct task *task)
{
    struct task_work *work = task->work;

    if (--TASK_TIMER(work) == 0) {
        turn_cube_go(task);
    }
}

static void turn_cube_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case CUBE_TURN:
        turn_cube_turn(task);
        break;
    case CUBE_REST:
        turn_cube_rest(task);
    }
    turn_cube_draw(task);
    if (!placement_editing(task)) {
        work->collision->shapes->ang.z = work->ang.x;
        collision_register(work);
    }
}

/* the cube's step in the set editor. it turns from the angle it last
 * turned to. the set entry's angles and scale come back after */
static void turn_cube_edit(struct task *task)
{
    turn_cube_edit_ang = task->work->ang;
    turn_cube_edit_scl = task->work->scl;
    turn_cube_read_set(task);
    task->work->ang.x = turn_cube_edit_turn;
    turn_cube_run(task);
    turn_cube_edit_turn = task->work->ang.x;
    task->work->ang = turn_cube_edit_ang;
    task->work->scl = turn_cube_edit_scl;
    njPrintSize(16);
    njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "<SPEED>");
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<STOP TIME (FRAMES)>");
}

void turn_cube_step(struct task *task)
{
    task->destructor = turn_cube_destroy;
    task->step = turn_cube_run;
    task->draw = turn_cube_draw;
    if (placement_editing(task)) {
        task->step = turn_cube_edit;
        turn_cube_edit_ang = task->work->ang;
        turn_cube_edit_scl = task->work->scl;
        turn_cube_read_set(task);
        turn_cube_go(task);
        turn_cube_edit_turn = task->work->ang.x;
        task->work->ang = turn_cube_edit_ang;
        task->work->scl = turn_cube_edit_scl;
    } else {
        turn_cube_read_set(task);
        turn_cube_go(task);
        collision_init(task, &turn_cube_shape, COLLISION_OBJECT, 1, 1);
    }
}

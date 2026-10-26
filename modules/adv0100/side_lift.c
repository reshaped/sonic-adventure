/* O SIDELIFT, a lift that carries player 0 between its placement and a far
 * end its scale sets */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/side_lift.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/models.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* side_lift_object is the lift. its child is the wheel, which turns as the
 * lift moves, and dat_0caa1d90 and its sibling are the wheel's next two
 * siblings. side_lift_solid_object is the surface the lift puts in the
 * collision world */
extern NJS_OBJECT side_lift_object;
extern NJS_OBJECT side_lift_wheel_object;
extern NJS_OBJECT dat_0caa1d90;
extern NJS_OBJECT side_lift_solid_object;

#define SE_LIFT_MOVE 560
#define SE_LIFT_STOP 559

/* the lift sets off once player 0 has stood on it more than BOARD_STEPS
 * steps in a row. it stands more than STOP_STEPS steps once it arrives */
#define BOARD_STEPS 5
#define STOP_STEPS 5

/* player 0 calls the lift to an end from within CALL_RADIUS of it across and
 * CALL_HEIGHT in height. the high end's call is measured from CALL_ABOVE above
 * it */
#define CALL_RADIUS 40.0F
#define CALL_HEIGHT 5.0F
#define CALL_ABOVE 10.0F

/* the placement's scale x is the lift's speed a step, DEFAULT_SPEED when 0.
 * its scale y and z are the far end's offset along y and z, turned by its y
 * angle. when both are 0, the far end is DEFAULT_RISE straight up */
#define DEFAULT_SPEED 1.0F
#define DEFAULT_RISE 100.0F
/* the wheel's turn a step for each unit of speed */
#define WHEEL_TURN 1000.0F

/* the collision world tests the lift while a player is within the radius
 * of the surface's model and NEAR_MARGIN more */
#define NEAR_MARGIN 20.0F

struct side_lift_vars {
    NJS_OBJECT *solid; /* the surface's copy in the collision world */
    /* the object whose model, evalflags, scale and offset the copy takes */
    NJS_OBJECT *from;
};

#define VARS(work) ((struct side_lift_vars *)&(work)->vars)

/* task->motion as the lift lays it out */
struct side_lift_motion {
    Uint8 unk_00[4];
    NJS_POINT3 low; /* the end at its placement */
    NJS_VECTOR step; /* its move a step toward high */
    Angle wheel; /* the wheel's turn about x */
    Angle wheel_step; /* the wheel's turn each step the lift moves */
    Uint8 unk_24[16];
    NJS_POINT3 high; /* the far end */
};

/* in work->mode */
enum side_lift_mode {
    LIFT_IDLE = 0, /* waiting for player 0 to board it or call it */
    LIFT_STEP_OFF = 1, /* waiting for player 0 to step off */
    LIFT_STOP = 2, /* standing still once it arrives */
    LIFT_RISE = 5, /* moving to the high end */
    LIFT_SINK = 6, /* moving to the low end */
    LIFT_AT_HIGH = 7, /* the lift arrives: one step sets it at the high end */
    LIFT_AT_LOW = 8 /* the lift arrives: one step sets it at the low end */
};

/* in work->submode: the end the lift stands at */
enum side_lift_end { END_LOW = 3, END_HIGH = 4 };

/* in work->tick, within a step: the end player 0 calls the lift to */
enum side_lift_call { CALL_NONE, CALL_LOW, CALL_HIGH };

static void side_lift_step(struct task *task);
static void side_lift_draw(struct task *task);
static void side_lift_destroy(struct task *task);

/* set the lift's ends and speed from its placement. outside the set editor,
 * put its surface in the collision world */
void egg_carrier_side_lift_step(struct task *task)
{
    struct task_work *work = task->work;
    struct side_lift_motion *motion = (struct side_lift_motion *)task->motion;
    struct task_force *force = task->force;
    NJS_VECTOR offset;
    NJS_OBJECT *solid;
    Angle ang;

    VARS(work)->from = &side_lift_solid_object;
    work->mode = LIFT_IDLE;
    work->submode = END_LOW;
    work->tick = CALL_NONE;
    work->timer = 0;
    work->ang.x = 0;
    work->ang.z = 0;
    motion->wheel = 0;
    if (work->scl.x == 0.0F) {
        work->scl.x = DEFAULT_SPEED;
    }
    motion->wheel_step = work->scl.x * WHEEL_TURN;
    motion->low.x = work->pos.x;
    motion->low.y = work->pos.y;
    motion->low.z = work->pos.z;
    njPushMatrix(NULL);
    offset.x = 0.0F;
    offset.y = work->scl.y;
    offset.z = work->scl.z;
    if (njScalor2(&offset) == 0.0F) {
        offset.x = 0.0F;
        offset.y = DEFAULT_RISE;
        offset.z = 0.0F;
    }
    njUnitMatrix(NULL);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcVector(NULL, &offset, &motion->step);
    motion->high.x = motion->step.x + work->pos.x;
    motion->high.y = motion->step.y + work->pos.y;
    motion->high.z = motion->step.z + work->pos.z;
    njUnitVector(&motion->step);
    motion->step.x *= work->scl.x;
    motion->step.y *= work->scl.x;
    motion->step.z *= work->scl.x;
    njPopMatrix(1);
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (!placement_editing(task)) {
        solid = world_object_alloc();
        VARS(work)->solid = solid;
        solid->evalflags = VARS(work)->from->evalflags;
        solid->scl[0] = VARS(work)->from->scl[0];
        solid->scl[1] = VARS(work)->from->scl[1];
        solid->scl[2] = VARS(work)->from->scl[2];
        solid->ang[0] = 0;
        solid->ang[1] = work->ang.y;
        solid->ang[2] = 0;
        solid->pos[0] = work->pos.x + VARS(work)->from->pos[0];
        solid->pos[1] = work->pos.y + VARS(work)->from->pos[1];
        solid->pos[2] = work->pos.z + VARS(work)->from->pos[2];
        solid->model = VARS(work)->from->model;
        solid->child = NULL;
        solid->sibling = NULL;
        world_add(WORLD_MOVES, task, solid);
        work->flags |= WORLD_TESTED;
    }
    task->step = side_lift_step;
    task->draw = side_lift_draw;
    task->destructor = side_lift_destroy;
}

/* an idle lift goes to the end player 0 calls it to. once player 0 boards it,
 * it goes to its other end, and its force carries player 0 along. in the set
 * editor, label the editor's rows and mark both ends with a shadow */
static void side_lift_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    struct side_lift_motion *motion = (struct side_lift_motion *)task->motion;
    NJS_OBJECT *solid;
    NJS_POINT3 call;
    NJS_POINT3 high;
    NJS_POINT3 offset;
    Angle ang;

    solid = VARS(work)->solid;
    if (!placement_editing(task)) {
        if (player_in_range(&work->pos, solid->model->r + NEAR_MARGIN)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
    if (work->mode == LIFT_IDLE) {
        call.x = motion->high.x;
        call.y = motion->high.y + CALL_ABOVE;
        call.z = motion->high.z;
        if (player_in_cylinder(&call, CALL_RADIUS, CALL_HEIGHT)) {
            work->tick = CALL_HIGH;
        } else if (player_in_cylinder(&motion->low, CALL_RADIUS, CALL_HEIGHT)) {
            work->tick = CALL_LOW;
        }
    }
    if (work->tick == CALL_LOW) {
        if (work->pos.y > motion->low.y) {
            work->mode = LIFT_SINK;
            work->tick = CALL_NONE;
        }
    } else if (work->tick == CALL_HIGH) {
        if (work->pos.y < motion->high.y) {
            work->mode = LIFT_RISE;
            work->tick = CALL_NONE;
        }
    }
    work->tick = CALL_NONE;
    switch (work->mode) {
    case LIFT_IDLE:
        if (!placement_editing(task)) {
            if (player_stands_on(0, task)) {
                ++TASK_TIMER(work);
                if (TASK_TIMER(work) > BOARD_STEPS) {
                    work->timer = 0;
                    if (work->submode == END_LOW) {
                        work->mode = LIFT_RISE;
                        force->speed.x = motion->step.x;
                        force->speed.y = motion->step.y;
                        force->speed.z = motion->step.z;
                    } else {
                        work->mode = LIFT_SINK;
                        force->speed.x = -motion->step.x;
                        force->speed.y = -motion->step.y;
                        force->speed.z = -motion->step.z;
                    }
                }
            } else {
                work->timer = 0;
            }
        }
        break;
    case LIFT_RISE:
        njAddVector(&work->pos, &motion->step);
        solid->pos[0] += motion->step.x;
        solid->pos[1] += motion->step.y;
        solid->pos[2] += motion->step.z;
        motion->wheel += motion->wheel_step;
        if (work->pos.y + motion->step.y >= motion->high.y) {
            work->mode = LIFT_AT_HIGH;
            force->speed.x = motion->high.x - work->pos.x;
            force->speed.y = motion->high.y - work->pos.y;
            force->speed.z = motion->high.z - work->pos.z;
        }
        sound_play_for(SE_LIFT_MOVE, (int)task, 1, 0, 2);
        break;
    case LIFT_AT_HIGH:
        force->speed.x = 0.0F;
        force->speed.y = 0.0F;
        force->speed.z = 0.0F;
        work->pos.x = motion->high.x;
        work->pos.y = motion->high.y;
        work->pos.z = motion->high.z;
        solid->pos[0] = motion->high.x;
        solid->pos[1] = motion->high.y;
        solid->pos[2] = motion->high.z;
        work->mode = LIFT_STOP;
        work->submode = END_HIGH;
        sound_play(SE_LIFT_STOP, 0, 0, 0);
        break;
    case LIFT_SINK:
        njSubVector(&work->pos, &motion->step);
        solid->pos[0] -= motion->step.x;
        solid->pos[1] -= motion->step.y;
        solid->pos[2] -= motion->step.z;
        motion->wheel -= motion->wheel_step;
        if (work->pos.y - motion->step.y <= motion->low.y) {
            work->mode = LIFT_AT_LOW;
            force->speed.x = motion->low.x - work->pos.x;
            force->speed.y = motion->low.y - work->pos.y;
            force->speed.z = motion->low.z - work->pos.z;
        }
        sound_play_for(SE_LIFT_MOVE, (int)task, 1, 0, 2);
        break;
    case LIFT_AT_LOW:
        force->speed.x = 0.0F;
        force->speed.y = 0.0F;
        force->speed.z = 0.0F;
        work->pos.x = motion->low.x;
        work->pos.y = motion->low.y;
        work->pos.z = motion->low.z;
        solid->pos[0] = motion->low.x;
        solid->pos[1] = motion->low.y;
        solid->pos[2] = motion->low.z;
        work->mode = LIFT_STOP;
        work->submode = END_LOW;
        sound_play(SE_LIFT_STOP, 0, 0, 0);
        break;
    case LIFT_STOP:
        ++TASK_TIMER(work);
        if (TASK_TIMER(work) > STOP_STEPS) {
            work->timer = 0;
            work->mode = LIFT_STEP_OFF;
        }
        break;
    case LIFT_STEP_OFF:
        if (!placement_editing(task)) {
            if (!player_stands_on(0, task)) {
                work->timer = 0;
                work->mode = LIFT_IDLE;
            }
        }
        break;
    }
    /* @hack redundant: an empty test, which keeps the first of the image's two
     * calls of placement_editing, the JSR at 0x0c928978, and the TST R0,R0
     * that drops its result; without it, 129 of 573 words differ */
    if (placement_editing(task)) {
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "              default");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> Rise Speed (  1.f)");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> Upstairs Y (100.f)");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> Upstairs Z (  0.f)");
        texture_set_regular();
        draw_set_material(0.0F, 1.0F, -1.0F, -1.0F);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + 1.0F, work->pos.z);
        njScale(NULL, 5.0F, 5.0F, 5.0F);
        njSimpleDrawObject(shadow_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        offset.x = 0.0F;
        offset.y = work->scl.y;
        offset.z = work->scl.z;
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njCalcPoint(NULL, &offset, &high);
        njPopMatrix(1);
        njTranslateV(NULL, &high);
        njScale(NULL, 5.0F, 5.0F, 5.0F);
        njSimpleDrawObject(shadow_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    side_lift_draw(task);
}

static void side_lift_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct side_lift_motion *motion = (struct side_lift_motion *)task->motion;
    NJS_OBJECT *object;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    object = &side_lift_object;
    draw_model(object->model);
    object = &side_lift_wheel_object;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN_VIA(njRotateX, -motion->wheel, ang);
    draw_model(object->model);
    njPopMatrix(1);
    object = &dat_0caa1d90;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    njScale(NULL, object->scl[0], object->scl[1], object->scl[2]);
    draw_model(object->model);
    njPopMatrix(1);
    object = object->sibling;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void side_lift_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}

/* O S LIGHT, a searchlight. its neck turns and its head swings, and a hit on
 * the head puts its beam out or back on */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv0100/search_light.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* search_light_object is the stand, whose model is search_light_base_model.
 * the neck, the head and the beam are each a child of the one before */
extern NJS_MODEL search_light_base_model;
extern NJS_OBJECT search_light_object;
extern NJS_OBJECT search_light_neck_object;
extern NJS_OBJECT search_light_head_object;
extern NJS_OBJECT search_light_beam_object;

/* the head's pitch swings between -SWING_BIAS and -(SWING_BIAS + 2 * scale
 * z). at pitch 0 the beam points straight up */
#define SWING_BIAS 0x444
/* the head's task sits BEAM_REACH along the beam from the head's origin */
#define BEAM_REACH 9.0F
/* a quarter turn about x. the head model's beam runs along its -z, so the
 * draw adds the turn. the head task's reach runs along +z, so the swing
 * takes the turn away */
#define QUARTER_TURN 0x4000

/* the beam's scale halves each step for TOGGLE_STEPS steps as it goes out,
 * and doubles from BEAM_FAINT as it comes on. the next step sets it to 0 or
 * 1 */
#define TOGGLE_STEPS 5
#define BEAM_FAINT 0.03125F

/* how fast a hit on the head pushes player 0 away from it */
#define PUSH_SPEED 2.0F

struct search_light_vars {
    Float beam; /* the beam's scale across, 1 while it is on */
    /* the red, green and blue of the beam's constant material: 0, -0.1 or
     * -0.3 */
    Float flicker;
};

#define VARS(work) ((struct search_light_vars *)&(work)->vars)

/* in work->mode: whether the beam is on. as the beam goes out or comes on,
 * the mode is the state it goes to */
enum search_light_mode { LIGHT_ON, LIGHT_OFF };
/* in work->submode: whether the beam is going out or coming on */
enum search_light_toggle { TOGGLE_NONE, TOGGLE_RUNNING };

/* a bit of the lamp's work->tick. the head's task sets it when a player hits
 * the head, and the lamp clears it as it starts the toggle */
#define HEAD_HIT 0x4

/* task->motion as the lamp lays it out */
struct search_light_motion {
    Uint8 unk_00[4];
    NJS_POINT3 pivot; /* where the neck turns, in the world */
    NJS_VECTOR reach; /* the head's task's place, before the head's turns */
    Angle pitch; /* the head's turn about x */
    Angle yaw; /* the neck's turn about y */
    Angle bias; /* taken from the swing: scale z + SWING_BIAS */
    Angle phase; /* the swing's angle, which scale x advances each step */
    Uint8 unk_2c[20];
};

static void search_light_swing(struct task *task);
static void search_light_still(struct task *task);
static void search_light_draw(struct task *task);
static void search_light_destroy(struct task *task);
static void search_light_head_init(struct task *task);
static void search_light_head_step(struct task *task);

/* the lamp's shapes: a box for the stand and a capsule for its pole. the
 * head's task takes search_light_head_shapes, a sphere that a player's attack
 * can hurt */
static struct collision_shape search_light_shapes[] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 4.2F, 0.0F},
     9.0F,
     1.0F,
     6.0F,
     {0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 11.0F, 0.0F},
     3.0F,
     4.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct collision_shape search_light_head_shapes[] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURT,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     7.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* outside the set editor, set up the lamp's collision and create the head's
 * task. the beam starts on, but once the Egg Carrier has sunk, the lamp
 * stands dark and still */
void egg_carrier_search_light_step(struct task *task)
{
    struct task_work *work = task->work;
    struct search_light_motion *motion =
        (struct search_light_motion *)task->motion;
    struct task *head;
    struct task_work *head_work;

    if (!placement_editing(task)) {
        collision_init(task, search_light_shapes, COLLISION_OBJECT, 1, 2);
        head = task_create_child(TASK_WORK, search_light_head_init, task);
        head_work = head->work;
        motion->pivot.x = work->pos.x + search_light_object.pos[0] +
                          search_light_neck_object.pos[0];
        motion->pivot.y = work->pos.y + search_light_object.pos[1] +
                          search_light_neck_object.pos[1];
        motion->pivot.z = work->pos.z + search_light_object.pos[2] +
                          search_light_neck_object.pos[2];
        head_work->pos.y =
            motion->pivot.y + search_light_head_object.pos[1] + BEAM_REACH;
        motion->reach.x = 0.0F;
        motion->reach.y = 0.0F;
        motion->reach.z = BEAM_REACH;
        task_step_children(task);
    }
    work->tick = 0;
    work->timer = 0;
    VARS(work)->beam = 1.0F;
    work->mode = LIGHT_ON;
    work->submode = TOGGLE_NONE;
    motion->pitch = 0;
    motion->yaw = 0;
    motion->bias = (int)work->scl.z + SWING_BIAS;
    motion->phase = 0;
    if (egg_carrier_has_sunk()) {
        VARS(work)->beam = 0.0F;
        task->step = search_light_still;
    } else {
        task->step = search_light_swing;
    }
    task->draw = search_light_draw;
    task->destructor = search_light_destroy;
}

/* while the beam is on or coming on, the head swings and the neck turns by
 * scale y a step. the head's task moves with the head, and the beam flickers
 * at random. while the beam is out or going out, the lamp holds still. a hit
 * on the head puts the beam out or brings it back on. in the set editor,
 * label the editor's scale rows */
static void search_light_swing(struct task *task)
{
    struct task_work *work = task->work;
    struct search_light_motion *motion =
        (struct search_light_motion *)task->motion;
    struct task_work *head_work = task->child->work;
    Float roll;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (work->mode == LIGHT_ON) {
        motion->pitch =
            (int)(njSin(motion->phase) * work->scl.z) - motion->bias;
        motion->yaw += (int)work->scl.y;
        motion->phase += (int)work->scl.x;
        roll = rand() * RAND_SCALE;
        if (roll >= 0.4F) {
            VARS(work)->flicker = 0.0F;
        } else if (roll >= 0.025F) {
            VARS(work)->flicker = -0.1F;
        } else {
            VARS(work)->flicker = -0.3F;
        }
        if (placement_editing(task)) {
            motion->bias = (int)work->scl.z + SWING_BIAS;
        } else {
            njPushMatrix(NULL);
            njUnitMatrix(NULL);
            njTranslateV(NULL, &motion->pivot);
            TURN_VIA(njRotateY, motion->yaw + work->ang.y, ang);
            njTranslate(NULL, search_light_head_object.pos[0],
                        search_light_head_object.pos[1],
                        search_light_head_object.pos[2]);
            TURN_VIA(njRotateX, motion->pitch - QUARTER_TURN, ang);
            njCalcPoint(NULL, &motion->reach, &head_work->pos);
            njPopMatrix(1);
        }
    }
    if (!placement_editing(task)) {
        if (work->submode == TOGGLE_NONE) {
            task_step_children(task);
            if (work->tick & HEAD_HIT) {
                work->tick &= ~HEAD_HIT;
                work->submode = TOGGLE_RUNNING;
                work->timer = 0;
                if (work->mode == LIGHT_ON) {
                    work->mode = LIGHT_OFF;
                    VARS(work)->beam = 1.0F;
                } else {
                    work->mode = LIGHT_ON;
                    VARS(work)->beam = BEAM_FAINT;
                }
            }
        } else {
            work->timer++;
            if (TASK_TIMER(work) > TOGGLE_STEPS) {
                work->submode = TOGGLE_NONE;
                VARS(work)->beam = work->mode == LIGHT_ON ? 1.0F : 0.0F;
            } else if (work->mode == LIGHT_ON) {
                VARS(work)->beam *= 2.0F;
            } else {
                VARS(work)->beam *= 0.5F;
            }
        }
        collision_register(work);
    }
    search_light_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> HEAD(X) ROT SPEED");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> NECK(Y) ROT SPEED");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> HEAD ROT RANGE");
    }
}

/* the step once the Egg Carrier has sunk: draw the lamp, still and dark, and
 * register its collision */
static void search_light_still(struct task *task)
{
    struct task_work *work = task->work;

    search_light_draw(task);
    if (!placement_editing(task)) {
        collision_register(work);
        task_step_children(task);
    }
}

/* draw the stand, the neck turned and the head pitched. draw the beam, dimmed
 * by its flicker, while its scale isn't 0 */
static void search_light_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct search_light_motion *motion =
        (struct search_light_motion *)task->motion;
    NJS_OBJECT *object;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&search_light_base_model);
    object = &search_light_neck_object;
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN_VIA(njRotateY, motion->yaw, ang);
    draw_model(object->model);
    object = &search_light_head_object;
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN_VIA(njRotateX, motion->pitch + QUARTER_TURN, ang);
    draw_model(object->model);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_DOUBLE_SIDE);
    if (VARS(work)->beam != 0.0F) {
        object = &search_light_beam_object;
        njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
        njScale(NULL, VARS(work)->beam, VARS(work)->beam, 1.0F);
        draw_set_material(0.0F, VARS(work)->flicker, VARS(work)->flicker,
                          VARS(work)->flicker);
        draw_model(object->model);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    draw_attr_restore();
    njPopMatrix(1);
}

static void search_light_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* the head's task's first step: set up its sphere, which the lamp carries
 * along with the head */
static void search_light_head_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_init(task, search_light_head_shapes, COLLISION_OBJECT, 1, 1);
    }
    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    work->scl.x = 0.0F;
    work->scl.y = 0.0F;
    work->scl.z = 0.0F;
    task->step = search_light_head_step;
}

/* when a player hits the head, push player 0 away from it at PUSH_SPEED and
 * set HEAD_HIT in the lamp's work->tick. the step it is hit, the head
 * registers no collision */
static void search_light_head_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR away;

    if (!placement_editing(task)) {
        if (work->flags & COLLISION_DAMAGED) {
            work->flags &= ~COLLISION_DAMAGED;
            if (work->collision->hit->kind == COLLISION_PLAYER) {
                struct task_work *player = player_work[0];

                away.x = player->pos.x - work->pos.x;
                away.y = player->pos.y - work->pos.y;
                away.z = player->pos.z - work->pos.z;
                njUnitVector(&away);
                player_set_speed(0, away.x * PUSH_SPEED, away.y * PUSH_SPEED,
                                 away.z * PUSH_SPEED);
                task->parent->work->tick |= HEAD_HIT;
            }
        } else {
            collision_register(task->work);
        }
    }
}

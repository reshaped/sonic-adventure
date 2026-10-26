/* O COLOR DOOR and O COLOR DOOR2 doors, and the C FAKE AMY boxes that warp
 * player 0 past them */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg10/color_door.h"
#include "stg10/puff.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the six looks. each is the lower panel, and its child the upper one */
extern NJS_OBJECT color_door_object_0;
extern NJS_OBJECT color_door_object_1;
extern NJS_OBJECT color_door_object_2;
extern NJS_OBJECT color_door_object_3;
extern NJS_OBJECT color_door_object_4;
extern NJS_OBJECT color_door_object_5;

#define SE_DOOR_CLUNK 283
#define SE_DOOR_RUMBLE 284

/* a door starts to open once player 0 comes within NEAR */
#define NEAR 40.0F
#define WIDTH 40.0F /* the door's, across which its puffs start */
/* a door is open once its upper panel has risen this far */
#define RISE 40.0F
#define RISE_SPEED 0.2F /* units a step, for each panel */
/* the upper panel starts to rise once the lower one has risen LOWER_LEAD */
#define LOWER_LEAD 10.0F
#define SHAKE_STEP 0x3333 /* a fifth of a turn a step */
#define SHAKE_END 0x80000 /* eight turns */
/* how far the shake lifts and drops the lower panel */
#define SHAKE_LIFT 0.5F
/* the doors' looks. O COLOR DOOR takes its look from scale x */
#define LOOKS 6
/* the C FAKE AMY units. O COLOR DOOR2 shows look color_door_unit + 1 */
#define UNITS 5

struct color_door_vars {
    Uint8 unk_00[4];
    Angle shake; /* the shake's phase */
    NJS_OBJECT *object;
};

#define VARS(work) ((struct color_door_vars *)&(work)->vars)
#define LOWER(work) ((work)->scl.x) /* height the lower panel has risen */
#define UPPER(work) ((work)->scl.y) /* height the upper panel has risen */
/* a C FAKE AMY's unit, from angle x */
#define UNIT(work) ((work)->tick)

enum color_door_mode { DOOR_INIT, DOOR_SHUT, DOOR_OPEN, DOOR_RISE, DOOR_SHAKE };
enum fake_amy_mode { FAKE_AMY_INIT, FAKE_AMY_RUN };

static void color_door_destroy(struct task *task);
static void color_door_draw(struct task *task);
static void color_door_draw_at(struct task_work *work);
static void fake_amy_destroy(struct task *task);
static void fake_amy_warp(void);

/* the unit whose C FAKE AMY warps player 0, which color_door_pick sets */
static int color_door_unit = 0;
/* the rest of the data, defined after the code */
extern NJS_OBJECT *color_door_objects[];
static struct collision_shape color_door_shape;
static struct dust_params color_door_puff;
static struct collision_shape fake_amy_shape;

void color_door_step(struct task *task)
{
    struct task_work *work = task->work;
    int look;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case DOOR_INIT:
        look = (int)work->scl.x;
        if (look < 0 || look > LOOKS - 1) {
            look = 0;
        }
        VARS(work)->object = color_door_objects[look];
        work->ang.x = 0;
        work->ang.z = 0;
        LOWER(work) = 0.0F;
        UPPER(work) = 0.0F;
        collision_init(task, &color_door_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = color_door_destroy;
        task->draw = color_door_draw;
        if (placement_used(task)) {
            LOWER(work) = UPPER(work) = RISE;
            work->collision->shapes->flags |= SHAPE_OFF;
            work->mode = DOOR_OPEN;
        } else {
            work->mode = DOOR_SHUT;
        }
        break;
    case DOOR_SHUT:
        if (njDistanceP2P(&player_work[0]->pos, &work->pos) < NEAR) {
            work->mode = DOOR_SHAKE;
            sound_play(SE_DOOR_CLUNK, 0, 0, 0);
        }
        break;
    case DOOR_OPEN:
        break;
    case DOOR_SHAKE:
        VARS(work)->shake += SHAKE_STEP;
        if (VARS(work)->shake > SHAKE_END) {
            VARS(work)->shake = 0;
            work->mode = DOOR_RISE;
            sound_play_held(SE_DOOR_RUMBLE, (int)work, 1, 0);
        }
        LOWER(work) = njSin(VARS(work)->shake) * SHAKE_LIFT;
        njPushMatrix(&_unit_matrix_);
        color_door_puff.pos.x = RANDOM(-0.5F, 0.5F) * WIDTH;
        color_door_puff.pos.y = 0.0F;
        color_door_puff.pos.z = 0.0F;
        color_door_puff.scale = RANDOM(1.0F, 2.0F);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njCalcPoint(NULL, &color_door_puff.pos, &color_door_puff.pos);
        particle_set_depth(-20.0F);
        final_puff_create(&color_door_puff);
        particle_set_depth(0.0F);
        njPopMatrix(1);
        break;
    case DOOR_RISE:
        LOWER(work) += RISE_SPEED;
        if (LOWER(work) > LOWER_LEAD) {
            UPPER(work) += RISE_SPEED;
        }
        if (UPPER(work) > RISE) {
            UPPER(work) = RISE;
            work->mode = DOOR_OPEN;
            work->collision->shapes->flags |= SHAPE_OFF;
            placement_set_used(task);
            sound_stop_id((int)work);
            sound_play(SE_DOOR_CLUNK, 0, 0, 0);
        } else {
            work->collision->shapes->center.y = UPPER(work) + 30.0F;
        }
        /* no break: with one, placement_editing's and collision_register's
         * addresses load into R3, not R2 (0x0c90f3ca, 0x0c90f3da) */
    }
    color_door_draw_at(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void color_door_destroy(struct task *task)
{
    struct task_work *work = task->work;

    sound_stop_id((int)work);
}

static void color_door_draw(struct task *task)
{
    color_door_draw_at(task->work);
}

static void color_door_draw_at(struct task_work *work)
{
    NJS_OBJECT *object;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        object = VARS(work)->object;
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njTranslate(NULL, 0.0F, LOWER(work), 0.0F);
        draw_model(object->model);
        object = object->child;
        njTranslate(NULL, 0.0F, UPPER(work) - LOWER(work), 8.0F);
        draw_model(object->model);
        njPopMatrix(1);
    }
}

void color_door2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case DOOR_INIT:
        VARS(work)->object = color_door_objects[color_door_unit + 1];
        collision_init(task, &color_door_shape, COLLISION_OBJECT, 1, 1);
        task->draw = color_door_draw;
        work->mode = DOOR_SHUT;
        break;
    case DOOR_SHUT:
        break;
    }
    color_door_draw_at(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

/* the set editor's label asks for scale x and units 1 to 5. the step reads
 * the unit from angle x, and compares it with 0 to 4 */
void fake_amy_step(struct task *task)
{
    struct task_work *work = task->work;

    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + 30.0F, work->pos.z);
        TURN(njRotateY, work->ang.y);
        /* the box's marker, the size of fake_amy_shape */
        njScale(NULL, 30.0F * CUBE_FIT, 30.0F * CUBE_FIT, 10.0F * CUBE_FIT);
        draw_set_material(0.05F, 1.0F, 0.0F, 0.0F);
        draw_tree(marker_cube_object);
        njPopMatrix(1);
        njPrintC(NJM_LOCATION(10, 31), "SCL X -> UNIT ID(1 - 5)");
        return;
    }
    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case FAKE_AMY_INIT:
        UNIT(work) = work->ang.x;
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, &fake_amy_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = fake_amy_destroy;
        work->mode = FAKE_AMY_RUN;
        break;
    case FAKE_AMY_RUN:
        if (color_door_unit == UNIT(work) &&
            (work->collision->flags & COLLISION_HIT)) {
            fake_amy_warp();
            placement_kill(task);
        }
        break;
    }
    collision_register(work);
}

static void fake_amy_destroy(struct task *task)
{
}

static void fake_amy_warp(void)
{
    player_set_position(0, 2033.0F, 5239.0F, -2330.0F);
    player_work[0]->ang.y = 0x4000;
}

void color_door_pick(void)
{
    color_door_unit = rand() * (UNITS * RAND_SCALE);
}

/* @hack type: color_door_objects and the data after it defined after the
 * code. color_door2_step then indexes the table through the declaration
 * above, which has no bound, and adds 1 to the unit (ADD #1,R3 at
 * 0x0c90f512). with the bound in scope, SHC adds 4 to the table's address
 * instead, and 38 of the step's 53 words differ */
NJS_OBJECT *color_door_objects[LOOKS] = {
    &color_door_object_0, &color_door_object_1, &color_door_object_2,
    &color_door_object_3, &color_door_object_4, &color_door_object_5,
};

static struct collision_shape color_door_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 30.0F, 0.0F},
    20.0F,
    30.0F,
    5.0F,
    {0},
    0,
    NULL,
};

static struct dust_params color_door_puff = {
    1.0F,
    0.038F,
    0.3F,
    0.03F,
    0.005F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.5F, 1.0F, 1.0F, 1.0F},
};

static struct collision_shape fake_amy_shape = {
    SHAPE_RECTANGLE | SHAPE_IGNORE_ENEMIES,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 30.0F, 0.0F},
    30.0F,
    30.0F,
    10.0F,
    {0},
    0,
    NULL,
};

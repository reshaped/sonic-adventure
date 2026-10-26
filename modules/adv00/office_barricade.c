/* O OFFICE BARRICADE barricade before the office, which Knuckles knocks down
 * the road */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/office_barricade.h"
#include "adv00/station_square.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/bomb.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* one barrier's top, with its foot as the child */
extern NJS_OBJECT barricade_barrier_object;

/* the sound of a blow on the barricade */
#define SE_BLOW 0x21a
/* the sound of the thrown barricade landing */
#define SE_THROWN 0x21b

/* the space between the three barriers' centers */
#define BARRIER_SPACING 26.0F

/* the blows the barricade takes before it's thrown */
#define BARRICADE_BLOWS 1

/* the rocking barricade's first rise, and the rise it loses each step. the
 * barriers' tops tilt ROCK_TURN further each step, from ROCK_TILT one way to
 * ROCK_TILT the other */
#define ROCK_RISE 1.2F
#define ROCK_FALL 0.1F
#define ROCK_TURN 0xe0
#define ROCK_TILT 0x600

/* the thrown barricade moves THROW_SPEED down the road each step, along -z,
 * and rises THROW_RISE. the barriers' tops turn THROW_SPIN about z each step.
 * it lands at a world z of BARRICADE_LANDS */
#define THROW_SPEED 13.0F
#define THROW_RISE 0.1F
#define THROW_SPIN 0x80
#define BARRICADE_LANDS 260.0F

/* the alpha the landed barricade's fade loses each step */
#define FADE_SPEED 0.01F

enum office_barricade_mode {
    BARRICADE_STANDING,
    BARRICADE_ROCKING,
    BARRICADE_THROWN,
    BARRICADE_FADING,
    BARRICADE_GONE
};

/* work->timer counts the blows left before the barricade is thrown. it
 * starts at BARRICADE_BLOWS, so the first blow throws the barricade and
 * BARRICADE_ROCKING never runs. work->tick is 0 while the rocking tilts the
 * barriers' tops toward +ROCK_TILT, and 1 while it tilts them back */
struct office_barricade_vars {
    Float rest; /* the barricade's height at rest */
    Float rise; /* its rise this step as it rocks */
    Float fade; /* the constant material's alpha as it fades, 0 down to -1 */
};

#define VARS(work) ((struct office_barricade_vars *)&(work)->vars)

static void office_barricade_step(struct task *task);
static void office_barricade_draw(struct task *task);
static void office_barricade_destroy(struct task *task);
static void office_barricade_wall_init(struct task *task);
static void office_barricade_wall_step(struct task *task);

/* clang-format off */
/* the three barriers, as one box an attack can hurt */
static struct collision_shape office_barricade_shape[] = {
    {SHAPE_RECTANGLE | SHAPE_HURTABLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 15.0F, 0.0F}, 39.0F, 15.0F, 0.5F, {0}, 0, NULL},
};
/* a wall 600 wide and 200 high in the barriers' plane. a child task registers
 * it each step until the thrown barricade lands */
static struct collision_shape office_barricade_wall_shape[] = {
    {SHAPE_RECTANGLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 100.0F, 0.0F}, 300.0F, 100.0F, 0.5F, {0}, 0, NULL},
};
/* clang-format on */

/* TRUE for every character but Knuckles, and for him once
 * FLAG_KNUCKLES_SS_BARRICADE is set */
static Bool office_barricade_gone(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
    case CHARACTER_TAILS:
        break;
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_BARRICADE];
    case CHARACTER_AMY:
    case CHARACTER_E102:
    case CHARACTER_BIG:
        break;
    }
    return TRUE;
}

void office_barricade_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        if (office_barricade_gone()) {
            placement_kill(task);
            return;
        }
        task_create_child(TASK_WORK, office_barricade_wall_init, task);
        collision_init(task, office_barricade_shape, COLLISION_OBJECT, 1, 1);
        event_flags[FLAG_OFFICE_BARRICADE] = 0;
    }
    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = BARRICADE_STANDING;
    work->timer = BARRICADE_BLOWS;
    VARS(work)->rest = work->pos.y;
    task->step = office_barricade_step;
    task->draw = office_barricade_draw;
    task->destructor = office_barricade_destroy;
}

static void office_barricade_step(struct task *task)
{
    struct task_work *work = task->work;
    Float y;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case BARRICADE_STANDING:
            if (work->flags & COLLISION_DAMAGED) {
                sound_play(SE_BLOW, 0, 0, 0);
                work->timer--;
                if (work->timer == 0) {
                    work->mode = BARRICADE_THROWN;
                    work->tick = 0;
                } else {
                    work->mode = BARRICADE_ROCKING;
                    work->tick = 0;
                    VARS(work)->rise = ROCK_RISE;
                }
            }
            task_step_children(task);
            collision_register(work);
            break;
        case BARRICADE_ROCKING:
            y = work->pos.y;
            y += VARS(work)->rise;
            VARS(work)->rise -= ROCK_FALL;
            if (y < VARS(work)->rest) {
                y = VARS(work)->rest;
                work->ang.z = 0;
                work->mode = BARRICADE_STANDING;
            } else if (work->tick == 0) {
                work->ang.z += ROCK_TURN;
                if (work->ang.z > ROCK_TILT) {
                    work->ang.z = ROCK_TILT;
                    work->tick = 1;
                }
            } else {
                work->ang.z -= ROCK_TURN;
                if (work->ang.z < -ROCK_TILT) {
                    work->ang.z = -ROCK_TILT;
                    work->tick = 0;
                }
            }
            work->pos.y = y;
            task_step_children(task);
            collision_register(work);
            break;
        case BARRICADE_THROWN:
            work->pos.z -= THROW_SPEED;
            if (work->pos.z <= BARRICADE_LANDS) {
                work->pos.z = BARRICADE_LANDS;
                work->mode = BARRICADE_FADING;
                VARS(work)->fade = 0.0F;
                bomb_create(&work->pos, 1.5F);
                sound_play(SE_THROWN, 0, 0, 0);
                event_flags[FLAG_KNUCKLES_SS_BARRICADE] = 1;
                event_flags[FLAG_OFFICE_BARRICADE] = 1;
            }
            work->pos.y += THROW_RISE;
            work->ang.z += THROW_SPIN;
            task_step_children(task);
            collision_register(work);
            break;
        case BARRICADE_FADING:
            VARS(work)->fade -= FADE_SPEED;
            if (VARS(work)->fade <= -1.0F) {
                VARS(work)->fade = -1.0F;
                work->mode = BARRICADE_GONE;
            }
            break;
        case BARRICADE_GONE:
            break;
        }
    }
    office_barricade_draw(task);
}

/* draw the three barriers, faded by fade: their feet at the rest height, and
 * their tops at the barricade's height, turned by work->ang.z */
static void office_barricade_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_OBJECT *child;
    Float lift;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(VARS(work)->fade, 0.0F, 0.0F, 0.0F);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    lift = VARS(work)->rest - work->pos.y;
    object = &barricade_barrier_object;
    child = object->child;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    njTranslate(NULL, child->pos[0], child->pos[1] + lift, child->pos[2]);
    njTranslate(NULL, -BARRIER_SPACING, 0.0F, 0.0F);
    draw_model(child->model);
    njTranslate(NULL, BARRIER_SPACING, 0.0F, 0.0F);
    draw_model(child->model);
    njTranslate(NULL, BARRIER_SPACING, 0.0F, 0.0F);
    draw_model(child->model);
    njPopMatrix(1);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    njTranslate(NULL, object->pos[0] - BARRIER_SPACING, object->pos[1],
                object->pos[2]);
    draw_model(object->model);
    njTranslate(NULL, BARRIER_SPACING, 0.0F, 0.0F);
    draw_model(object->model);
    njTranslate(NULL, BARRIER_SPACING, 0.0F, 0.0F);
    draw_model(object->model);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
    njPopMatrix(1);
}

static void office_barricade_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void office_barricade_wall_init(struct task *task)
{
    /* @hack spelling: wall, a copy of task, which keeps the task in R14
     * across collision_init's call (MOV R4,R14 at 0x0c90e7a6); used
     * directly, task waits on the stack, the function runs 2 bytes longer
     * and 16 of 17 words differ */
    struct task *wall = task;

    collision_init(wall, office_barricade_wall_shape, COLLISION_OBJECT, 1, 1);
    wall->step = office_barricade_wall_step;
}

static void office_barricade_wall_step(struct task *task)
{
    collision_register(task->work);
}

/* spring h: a spring board, as long as its scl.x says. a player who touches
 * its side is thrown up from THROW_FRONT in front of the board */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/models.h"
#include "object/placement.h"
#include "object/spring_h.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the steps the board swings after a throw */
#define SWING_STEPS 120
/* the fewest steps between two throws' sounds */
#define SOUND_STEPS 10

/* a throw sets the player THROW_FRONT in front of the board, and sends it
 * up at THROW_SPEED plus add_yspd. the player can't steer for THROW_STEPS
 * steps */
#define THROW_FRONT 8.0F
#define THROW_SPEED 5.0F
#define THROW_STEPS 30

#define BOARD_LENGTH 20.0F /* the board's length at an x_scale of 1 */
/* the collision record's radius past the shape's length */
#define SHAPE_REACH 16.0F

/* the board's slant, a unit vector in y and z about 36.6 degrees from
 * upright, along which lift moves it */
#define SLANT_UP 0.803207F
#define SLANT_OUT 0.595699F

/* the board's own layout of task_work. x_scale and add_yspd are the placed
 * scl's x and y */
struct spring_h_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 tick;
    Sint16 flags;
    Sint16 timer; /* steps the board has still to swing */
    Float swing; /* the swing's size: -3 at a throw, rising 0.025 a step */
    Angle phase; /* where the swing is, moving 12 degrees a step */
    Sint32 sound_wait; /* steps until a throw sounds again */
    struct angles ang;
    NJS_POINT3 pos;
    Float x_scale; /* the board's length, in lengths of the model */
    Float add_yspd; /* added to THROW_SPEED for a throw */
    Float lift; /* how far the board stands out along its slant */
    struct collision_work *collision;
    struct event_work *event;
};

enum spring_h_mode { SPRING_H_WAIT, SPRING_H_SWING };

static void spring_h_step(struct task *task);
static void spring_h_throw(struct task *task, int player);
static void spring_h_draw(struct task *task);
static void spring_h_destroy(struct task *task);

/* as long as the board, turned a quarter about z to lie along it */
/* clang-format off */
static struct collision_shape spring_h_shape = {
    SHAPE_CAPSULE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 5.0F, 5.0F}, 6.0F,
    BOARD_LENGTH, 0.0F, {0, 0, NJM_DEG_ANG(90)}, 0, NULL};
/* clang-format on */

/* a positive x_scale stretches the shape with the board. the board starts
 * as a target, which players can aim at */
void spring_h_start(struct task *task)
{
    struct spring_h_work *work = (struct spring_h_work *)task->work;

    /* the board starts only while spring_h_work keeps task_work's fields
     * where task_work does */
    /* @hack redundant: pos tested twice, as the image tests it (ADD #32 at
     * 0x8c06b64e and 0x8c06b680); tested once, 64 of 93 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct spring_h_work, pos) ||
        !WORK_SAME_PLACE(struct spring_h_work, id) ||
        !WORK_SAME_PLACE(struct spring_h_work, collision) ||
        !WORK_SAME_PLACE(struct spring_h_work, event) ||
        !WORK_SAME_PLACE(struct spring_h_work, pos) ||
        !WORK_SAME_PLACE(struct spring_h_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    work->mode = SPRING_H_WAIT;
    work->lift = 0.0F;
    collision_init(task, &spring_h_shape, COLLISION_OBJECT, 1, 1);
    work->collision->flags |= COLLISION_TARGET;
    if (work->x_scale > 0.0F) {
        work->collision->shapes->size_y = work->x_scale * BOARD_LENGTH;
        work->collision->radius = work->collision->shapes->size_y + SHAPE_REACH;
    }
    task->destructor = spring_h_destroy;
    task->step = spring_h_step;
    task->draw = spring_h_draw;
}

/* a player's touch at the board's side throws that player. the board then
 * swings, less and less, and is a target again once it stops */
static void spring_h_step(struct task *task)
{
    struct spring_h_work *work = (struct spring_h_work *)task->work;
    struct task *owner;
    Float sine;
    Float cosine;
    int i;

    if (placement_try_leave(task)) {
        return;
    }
    if ((work->collision->flags & COLLISION_HIT) &&
        work->collision->hit->kind == COLLISION_PLAYER &&
        (work->collision->flags & COLLISION_HIT_SIDE)) {
        work->collision->flags &= ~COLLISION_TARGET;
        owner = work->collision->hit->owner;
        for (i = 0; i < 2; i++) {
            if (player_get_task(i) == owner) {
                spring_h_throw(task, i);
            }
        }
    }
    spring_h_draw(task);
    collision_register((struct task_work *)work);
    if (work->mode == SPRING_H_SWING) {
        if (--work->timer < 0) {
            work->mode = SPRING_H_WAIT;
            work->lift = 0.0F;
            work->collision->flags |= COLLISION_TARGET;
        } else {
            work->phase += NJM_DEG_ANG(12);
            /* @hack spelling: the swing grows by subtracting a negative,
             * FSUB at 0x8c06b7f8 of the pool's -0.025F; with +=, SHC adds,
             * and 1 of 150 words differ */
            work->swing -= -0.025F;
            _builtin_fsca(work->phase, &sine, &cosine);
            work->lift = work->swing * sine;
        }
    }
    if (work->sound_wait) {
        work->sound_wait--;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- X SCALE");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- ADD YSPD");
        work->collision->shapes->size_y = work->x_scale * BOARD_LENGTH;
        work->collision->radius = work->collision->shapes->size_y + SHAPE_REACH;
    }
}

/* the player keeps its place along the board but moves THROW_FRONT in
 * front of it. it is thrown up, turned as the board is but facing back.
 * the board starts to swing */
static void spring_h_throw(struct task *task, int player)
{
    struct spring_h_work *work = (struct spring_h_work *)task->work;
    NJS_VECTOR speed = {0.0F, THROW_SPEED, 0.0F};
    Angle ang[3];
    NJS_POINT3 pos;
    Float dx;
    Float dz;
    Float along;
    Float sine;
    Float cosine;

    work->mode = SPRING_H_SWING;
    work->timer = SWING_STEPS;
    work->swing = -3.0F;
    work->phase = 0;
    work->lift = 0.0F;
    _builtin_fsca(work->ang.y, &sine, &cosine);
    sine = -sine;
    player_get_history(player, 0, &pos, NULL);
    dx = pos.x - work->pos.x;
    dz = pos.z - work->pos.z;
    along = (dx * cosine) + (dz * sine);
    pos.x = ((along * cosine) - (sine * THROW_FRONT)) + work->pos.x;
    pos.z = ((sine * along) + (cosine * THROW_FRONT)) + work->pos.z;
    player_set_position(player, pos.x, pos.y, pos.z);
    speed.y += work->add_yspd;
    ang[0] = work->ang.x;
    ang[1] = work->ang.y + NJM_DEG_ANG(180);
    ang[2] = work->ang.z;
    player_launch_speed(player, &speed, ang, THROW_STEPS);
    if (work->sound_wait == 0) {
        sound_play(SE_SPRING, 0, 0, 0);
        work->sound_wait = SOUND_STEPS;
    }
}

/* draw the base, stretched to its length, then the board, out along its
 * slant */
static void spring_h_draw(struct task *task)
{
    struct spring_h_work *work = (struct spring_h_work *)task->work;

    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if (work->x_scale > 0.0F) {
        njScale(NULL, work->x_scale, 1.0F, 1.0F);
    }
    draw_model(&spring_h_base_model);
    if (work->mode == SPRING_H_SWING) {
        Float up = work->lift * SLANT_UP;
        Float out = work->lift * SLANT_OUT;

        njTranslate(NULL, 0.0F, up, out);
    }
    draw_model(&spring_h_board_model);
    njPopMatrix(1);
}

static void spring_h_destroy(struct task *task)
{
}

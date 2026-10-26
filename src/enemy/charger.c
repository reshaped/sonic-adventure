/* the charger, a hovering can on a disc, drawn from marker models. it
 * turns to face player 0, then rams the spot where player 0 stood */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "animal/kinds.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "enemy/charger.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

enum charger_mode {
    CHARGER_WANDER = 1, /* it wanders near home until it sees a player */
    CHARGER_FACE, /* it turns in place to face player 0, then rams */
    CHARGER_RAM, /* it rams the spot where player 0 stood as the ram began */
    CHARGER_STOP, /* it brakes, then wanders again */
    /* it frees an animal and goes. charger.c never enters it */
    CHARGER_GONE
};

/* the charger's bits in work->submode. charger_turn turns it toward home
 * while CHARGER_FAR or CHARGER_OFF_LEVEL is set */
#define CHARGER_FAR 0x1 /* it is farther than MOVE_RANGE from home */
#define CHARGER_SEES 0x2 /* enemy_sees_player found a player in sight */
/* the ground under it lay more than LEVEL_RANGE above or below home's
 * height this step, and it went back to where it was */
#define CHARGER_OFF_LEVEL 0x10

#define MOVE_RANGE 300.0F
#define LEVEL_RANGE 0.5F

#define WANDER_SPEED 0.3F
#define RAM_SPEED 2.4F
#define STOP_SPEED 0.04F /* braking slows it to this, and no further */
#define BRAKE (-0.03F) /* the change in its speed each step it brakes */

/* the range placement_try_leave_by takes, squared. the charger stays
 * while the camera or player 0 is within 500 of it, or player 1 is Sonic and
 * within 40 */
#define CHARGER_LEAVE_RANGE (500.0F * 500.0F)

static void charger_destroy(struct task *task);
static void charger_step(struct task *task);

/* 4 bytes this file never reads. their type is unknown */
static Uint32 dat_8c5c1808 = 0;

/* the base, then the top: charger_take_touch tells them apart by order */
/* clang-format off */
static struct collision_shape charger_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE, GUARD(2) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL, {0.0F, -4.0F, 0.0F}, 13.5F, 4.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE, GUARD(2) | SHAPE_HURTS_PLAYERS, 0,
     {0.0F, 4.0F, 0.0F}, 8.0F, 2.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

void charger_start(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record;

    collision_init(task, charger_shapes, COLLISION_ENEMY, 1, 2);
    record = enemy_alloc_record(task, work);
    work->mode = CHARGER_WANDER;
    work->submode = 0;
    work->tick = 0;
    work->timer = 0;
    record->sight_distance = 300.0F * 300.0F;
    record->near_distance = 80.0F * 80.0F;
    record->motion.speed.x = WANDER_SPEED;
    record->turn_speed = 0x200;
    work->pos.y = shadow_draw(work, 1.0F);
    enemy_set_home(work, record);
    task->destructor = charger_destroy;
    task->step = charger_step;
}

static void charger_wander_speed(struct task_work *work,
                                 struct lockon_target *record)
{
    record->motion.speed.x += 0.02F;
    if (record->motion.speed.x > WANDER_SPEED) {
        record->motion.speed.x = WANDER_SPEED;
    }
}

static void charger_ram_speed(struct task_work *work,
                              struct lockon_target *record)
{
    record->motion.speed.x += 0.06F;
    if (record->motion.speed.x > RAM_SPEED) {
        record->motion.speed.x = RAM_SPEED;
    }
}

static void charger_brake(struct task_work *work, struct lockon_target *record)
{
    record->motion.speed.x += BRAKE;
    if (record->motion.speed.x < STOP_SPEED) {
        record->motion.speed.x = STOP_SPEED;
    }
}

static void charger_turn(struct task_work *work, struct lockon_target *record)
{
    if (work->submode & (CHARGER_FAR | CHARGER_OFF_LEVEL)) {
        enemy_turn_home(work, record);
    }
    switch (work->mode) {
    case CHARGER_RAM:
        enemy_turn_to_target(work, record);
        break;
    case CHARGER_WANDER:
    case CHARGER_FACE:
        if (work->submode & CHARGER_SEES) {
            enemy_turn_to_player(work, record, 0);
        }
        break;
    }
}

static void charger_mark_far(struct task_work *work,
                             struct lockon_target *record)
{
    Float distance2 = enemy_home_distance2(work, record);

    if (distance2 > MOVE_RANGE * MOVE_RANGE) {
        work->submode |= CHARGER_FAR;
    } else {
        work->submode &= ~CHARGER_FAR;
    }
}

static void charger_knock_away(struct task_work *work,
                               struct lockon_target *record)
{
    struct task_work *player = player_work[0];
    NJS_VECTOR speed;
    NJS_VECTOR away;
    Angle ang[3];
    /* @hack spelling: 0 through a local, which keeps a zero in FR15 across
     * the calls (FLDI0 FR15 at 0x8c10137a); as the literal, the function
     * runs 2 bytes short and 78 of 92 words differ */
    Float zero = 0.0F;

    away.x = player->pos.x - work->pos.x;
    away.y = zero;
    away.z = player->pos.z - work->pos.z;
    ang[0] = 0;
    ang[1] = (Angle)(atan2f(away.x, away.z) * ANGLE_PER_RADIAN);
    ang[2] = 0;
    if (angle_distance(ang[1], player->ang.y) > 0x4000) {
        ang[1] += 0x8000;
        speed.x = -2.5F;
    } else {
        speed.x = 2.5F;
    }
    speed.y = 1.2F;
    speed.z = zero;
    player_send_flying(0, &speed, ang, 60);
    player->pos.y += 1.0F;
}

/* a player's collision record touching the base sends player 0 flying.
 * one touching the top instead bounces player 0 up, whichever player it
 * was */
static void charger_take_touch(struct task_work *work,
                               struct lockon_target *record)
{
    struct collision_work *collision = work->collision;
    struct collision_shape *shape;
    struct task_work *player = player_work[0];

    /* @hack redundant: stores through player that never run, which SHC
     * deletes. with the read of player->mode in shape's assignment, they
     * keep player_work's address in R6 and collision in a frame slot
     * (0x8c101460 to 0x8c101466). without them, the function runs 4 bytes
     * short and 33 of 33 words differ */
    if (0) {
        player->ang.x = 0;
        player->ang.z = 0;
    }
    /* @hack redundant: a discarded read of player 0's mode, which SHC drops.
     * with the stores above, it keeps the frame slot (MOV.L R3,@R15 at
     * 0x8c101466). without it, the function runs 6 bytes short and 33 of 33
     * words differ */
    /* @hack spelling: the read and the shapes' load as one expression, which
     * loads the shapes straight from R3 (MOV.L @(12,R3),R6 at 0x8c10146a);
     * as a statement of its own, the read adds a MOV R3,R6, and 28 of 33
     * words differ */
    shape = ((void)player->mode, collision->shapes);
    if (work->flags & (COLLISION_TOUCHING | COLLISION_UNDER)) {
        if (shape->hit != NULL && shape->hit->kind == COLLISION_PLAYER) {
            charger_knock_away(work, record);
        } else {
            shape++;
            if (shape->hit != NULL && shape->hit->kind == COLLISION_PLAYER) {
                player_bounce(0, 0.0F, 5.0F, 0.0F);
            }
        }
    }
}

static void charger_face_when_seen(struct task_work *work,
                                   struct lockon_target *record)
{
    if (enemy_sees_player(work, record)) {
        work->mode = CHARGER_FACE;
        work->timer = 0;
        work->submode |= CHARGER_SEES;
    } else {
        work->submode &= ~CHARGER_SEES;
    }
}

/* move by speed.x along the heading and by speed.z across it */
static void charger_move(struct task_work *work, struct lockon_target *record)
{
    work->pos.x += (record->motion.speed.x * njCos(work->ang.y)) +
                   (record->motion.speed.z * njSin(work->ang.y));
    work->pos.z += -(record->motion.speed.x * njSin(work->ang.y)) +
                   (record->motion.speed.z * njCos(work->ang.y));
}

static void charger_draw(struct task_work *work, struct lockon_target *record)
{
    /* @hack spelling: each marker drawn through object, which keeps its address
     * in R8 across the calls (MOV.L @(0x8c10175c),R8 at 0x8c1015c6); passed
     * straight, 200 of 231 words differ */
    NJS_OBJECT *object;
    /* @hack spelling: draw_set_material called through set_material, which is
     * set where it's first called. this keeps draw_set_material in R12 and
     * njScale in R11 (0x8c1015a4, 0x8c1015b8); called directly, the two
     * trade and 17 of 231 words differ */
    void (*set_material)(Float alpha, Float red, Float green, Float blue);

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);

    njPushMatrix(NULL);
    njScale(NULL, 0.7F, 0.5F, 0.7F);
    set_material = draw_set_material;
    set_material(1.0F, 0.0F, 1.0F, 0.0F);
    object = &marker_cylinder_object;
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, -2.0F, 0.0F);
    njScale(NULL, 1.2F, 0.15F, 1.2F);
    set_material(1.0F, 0.5F, 0.5F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 3.1F, 0.0F);
    njScale(NULL, 1.0F, 0.2F, 1.0F);
    set_material(1.0F, 1.0F, 0.0F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njTranslate(NULL, 10.0F, 0.0F, 4.0F);
    njPushMatrix(NULL);
    njScale(NULL, 0.25F, 0.25F, 0.25F);
    set_material(1.0F, 1.0F, 1.0F, 1.0F);
    object = &marker_object;
    draw_tree(object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 2.55F, 0.0F, 0.0F);
    njScale(NULL, 0.01F, 0.05F, 0.05F);
    set_material(1.0F, 0.0F, 0.0F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njTranslate(NULL, 10.0F, 0.0F, -4.0F);
    njPushMatrix(NULL);
    njScale(NULL, 0.25F, 0.25F, 0.25F);
    set_material(1.0F, 1.0F, 1.0F, 1.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 2.55F, 0.0F, 0.0F);
    njScale(NULL, 0.01F, 0.05F, 0.05F);
    set_material(1.0F, 0.0F, 0.0F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPopMatrix(1);

    set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njPopMatrix(1);
}

static void charger_hover(struct task_work *work, struct lockon_target *record)
{
    Float ground = shadow_draw(work, 1.2F);

    record->phase += 0x400;
    work->pos.y = ground + (njSin(record->phase) * 0.4F) + 8.0F;
    if (ground > record->home.y + LEVEL_RANGE ||
        ground < record->home.y - LEVEL_RANGE) {
        enemy_restore_pos(work, record);
        work->submode |= CHARGER_OFF_LEVEL;
    } else {
        work->submode &= ~CHARGER_OFF_LEVEL;
    }
}

static void charger_edit_draw(struct task_work *work)
{
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y + 0x8000);
    njScale(NULL, 1.25F, 0.5F, 1.25F);
    draw_set_material(1.0F, 0.0F, 1.0F, 0.0F);
    draw_tree(&marker_cylinder_object);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njPopMatrix(1);
}

static void charger_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

static void charger_die(struct task *task, struct task_work *work,
                        struct lockon_target *record)
{
    animal_create_random(10, work->pos.x, work->pos.y, work->pos.z);
    enemy_end(task, work, record);
    placement_kill(task);
}

static void charger_wander(struct task_work *work, struct lockon_target *record)
{
    /* @hack redundant: a discarded read of work->mode, which SHC drops but
     * which puts work in R14 and record in R13 (MOV R4,R14 at 0x8c101916);
     * without it, the two trade and 17 of 30 words differ */
    (void)work->mode;
    charger_take_touch(work, record);
    charger_face_when_seen(work, record);
    charger_mark_far(work, record);
    charger_wander_speed(work, record);
    charger_turn(work, record);
    charger_move(work, record);
    charger_hover(work, record);
    charger_draw(work, record);
}

static void charger_face(struct task_work *work, struct lockon_target *record)
{
    struct task_work *player = player_work[0];

    charger_take_touch(work, record);
    enemy_turn_to_player(work, record, 0);
    charger_hover(work, record);
    charger_draw(work, record);
    if (TASK_TIMER(work)++ > 100) {
        work->mode = CHARGER_RAM;
        VECTOR_COPY(&record->target, &player->pos);
    }
    /* @hack redundant: two discarded reads through work, which SHC drops but
     * which put work in R14 and record in R13 (MOV R4,R14 at 0x8c101952);
     * without one of them or both, the two trade and 14 of 47 words
     * differ */
    (void)work->mode;
    (void)work->submode;
}

static void charger_ram(struct task_work *work, struct lockon_target *record)
{
    /* the distance in which BRAKE would bring this speed to 0 */
    Float stopping = record->motion.speed.x * record->motion.speed.x *
                     (-1.0F / (2.0F * BRAKE));
    Float distance2 = enemy_target_distance2(work, record);

    if (distance2 < 10.0F * 10.0F || TASK_TIMER(work)++ > 300) {
        work->mode = CHARGER_STOP;
        work->timer = 0;
    } else if (stopping * stopping > distance2) {
        charger_brake(work, record);
    } else {
        charger_ram_speed(work, record);
    }
    charger_take_touch(work, record);
    charger_turn(work, record);
    charger_move(work, record);
    charger_hover(work, record);
    charger_draw(work, record);
}

static void charger_stop(struct task_work *work, struct lockon_target *record)
{
    charger_take_touch(work, record);
    charger_brake(work, record);
    charger_move(work, record);
    charger_hover(work, record);
    charger_draw(work, record);
    if (TASK_TIMER(work)++ > 60) {
        work->mode = CHARGER_WANDER;
    }
}

static void charger_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode) {
        charger_edit_draw(work);
        return;
    }
    /* @hack redundant: a discarded read of player 0's mode, which SHC drops
     * but which keeps the image's load of player_work[0] (MOV.L @R4,R4 at
     * 0x8c101b12); without it, the object runs 54 bytes short and 82 of 83
     * words differ */
    (void)player->mode;
    switch (work->mode) {
    case CHARGER_WANDER:
        charger_wander(work, record);
        break;
    case CHARGER_FACE:
        charger_face(work, record);
        break;
    case CHARGER_RAM:
        charger_ram(work, record);
        break;
    case CHARGER_STOP:
        charger_stop(work, record);
        break;
    case CHARGER_GONE:
        charger_die(task, work, record);
        return;
    /* @hack redundant: an empty default, which puts enemy_save_pos in R2, not
     * R3 (0x8c101ba0); without it, 2 of 83 words differ */
    default:
        break;
    }
    enemy_save_pos(work, record);
    if (placement_try_leave_by(task, CHARGER_LEAVE_RANGE)) {
        return;
    } else {
        /* @hack redundant: this else, after an arm that returns, puts
         * collision_register in R2, not R3 (0x8c101bb8); without it, 2 of 83
         * words differ */
        collision_register(work);
    }
}

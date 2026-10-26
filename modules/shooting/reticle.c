/* the tornado's reticle, a sprite that player 0's stick moves, and the
 * locks it puts on targets. a lock is a marker over its target,
 * which lasts LOCK_STEPS at most. it goes early when its target goes, falls
 * behind the camera or changes its step */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "player/player.h"
#include "shooting/reticle.h"
#include "shooting/shooting.h"
#include "shooting/tornado.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 13
#include "shooting/reticle/text.inc"

struct lock_vars {
    /* the target's own destructor, which the lock's replaces while it holds */
    void (*destructor)(struct task *task);
    Uint8 unk_04[4];
    void (*step)(struct task *task); /* the target's step as the lock took it */
};

#define VARS(work) ((struct lock_vars *)&(work)->vars)
/* a lock marker's growth and height. each step adds the growth's square to
 * the height, then takes the growth down to 0.85 of itself */
#define LOCK_GROWTH(work) ((work)->scl.x)
#define LOCK_HEIGHT(work) ((work)->scl.y)

enum reticle_mode { RETICLE_INIT = 0, RETICLE_AIM = 1, RETICLE_GONE = 4 };
enum lock_mode { LOCK_INIT = 0, LOCK_HOLD = 1, LOCK_GROW = 2, LOCK_GONE = 4 };

/* the most steps a lock lasts: ten seconds, at 60 steps a second */
#define LOCK_STEPS 600

/* how near the tornado a target must be for a lock, and for a bullet's
 * aim, squared */
#define LOCK_REACH (5000.0F * 5000.0F)
#define BULLET_REACH (3000.0F * 3000.0F)
/* how near the reticle a target must show on the screen, in pixels, squared */
#define RETICLE_REACH (32.0F * 32.0F)

/* lock_sprite's texanims in lock_frames: the reticle, the reticle once
 * every lock is taken, and a lock's marker */
enum lock_texanim { TEXANIM_RETICLE, TEXANIM_FULL, TEXANIM_LOCK };

/* the screen's center, where the reticle starts */
#define CENTER_X 320.0F
#define CENTER_Y 240.0F
/* the reticle's bounds, 80 pixels in from each edge of the 640 by 480
 * screen */
#define RETICLE_LEFT 80.0F
#define RETICLE_RIGHT 560.0F
#define RETICLE_TOP 80.0F
#define RETICLE_BOTTOM 400.0F

#define SE_LOCK 683

static void reticle_set_sprite(struct task *task);
static void reticle_draw_sprite(struct task *task);
static void reticle_move(struct task *task);
static void reticle_destroy(struct task *task);
static Bool reticle_over(struct collision_work *collision,
                         struct task_work *reticle);
static int lock_take(struct task_work *reticle);
static Bool locks_full(void);
static struct task *reticle_nearest(struct task_work *reticle);
static void reticle_point_step(struct task *task);
static void lock_step(struct task *task);
static void lock_draw(struct task *task);
static void lock_destroy(struct task *task);
static Bool lock_expired(struct task *task);
static void lock_target_destroy(struct task *task);
static void lock_let_go(struct task *task);

/* the point 300 along z, which nothing reads */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const NJS_POINT3 dat_0c909b28 = {0.0F, 0.0F, 300.0F};

struct task *reticle_target = NULL;
static NJS_TEXNAME lock_textures[3] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
static NJS_TEXLIST lock_texlist = {
#include "shooting/reticle/lock_texlist.inc"
};
static NJS_TEXANIM lock_frames[3] = {
#include "shooting/reticle/lock_frames.inc"
};
static NJS_SPRITE lock_sprite = {
#include "shooting/reticle/lock_sprite.inc"
};
/* a closed square 200 across, which nothing reads */
static NJS_POINT3 dat_0c955d9c[5] = {
    {-100.0F, -100.0F, 0.0F}, {-100.0F, 100.0F, 0.0F},  {100.0F, 100.0F, 0.0F},
    {100.0F, -100.0F, 0.0F},  {-100.0F, -100.0F, 0.0F},
};
/* the pixels the reticle moves a step for each unit of a stick reading.
 * reticle_move reads x1 and y1 times 256. a full tilt of 127 then moves the
 * reticle about 10 pixels a step */
static Float reticle_speed = 0.0003F;

struct collision_work *lock_found[LOCKS];
struct task *lock_targets[LOCKS];

void reticle_step(struct task *task)
{
    struct task_work *work = task->work;

    if (shooting_event_playing()) {
        return;
    }
    switch (work->mode) {
    case RETICLE_INIT:
        work->pos.x = CENTER_X;
        work->pos.y = CENTER_Y;
        work->pos.z = -1000.0F;
        task->destructor = reticle_destroy;
        task->draw = reticle_set_sprite;
        work->mode = RETICLE_AIM;
        task_create_child(TASK_WORK, reticle_point_step, task);
        break;
    case RETICLE_AIM:
        reticle_set_sprite(task);
        reticle_draw_sprite(task);
        reticle_move(task);
        work->ang.z += 0x400;
        if (tornado_count_charge()) {
            lock_take(work);
        }
        reticle_target = reticle_nearest(work);
        if (reticle_target == NULL) {
            reticle_target = task->child;
        }
        break;
    case RETICLE_GONE:
        task_destroy(task);
        return;
    default:
        break;
    }
    task_step_children(task);
}

/* at a frame's first step, set the blending, the material and the texture
 * for lock_sprite. put the sprite where the reticle is, at full size */
static void reticle_set_sprite(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
        njSetTexture(&lock_texlist);
        lock_sprite.p.x = work->pos.x;
        lock_sprite.p.y = work->pos.y;
        lock_sprite.sx = 1.0F;
        lock_sprite.sy = 1.0F;
    }
}

/* draw the reticle, turning until every lock is taken, then still at
 * TEXANIM_FULL */
static void reticle_draw_sprite(struct task *task)
{
    if (locks_full()) {
        lock_sprite.ang = 0x2000;
        njDrawSprite2D(&lock_sprite, TEXANIM_FULL, -150.0F,
                       NJD_SPRITE_ANGLE | NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
    } else {
        lock_sprite.ang += 0x200;
        njDrawSprite2D(&lock_sprite, TEXANIM_RETICLE, -150.0F,
                       NJD_SPRITE_ANGLE | NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
    }
}

/* move the reticle by player 0's stick, within its bounds */
static void reticle_move(struct task *task)
{
    struct task_work *work = task->work;

    work->pos.x += (pad_records[0].x1 << 8) * reticle_speed;
    if (work->pos.x > RETICLE_RIGHT) {
        work->pos.x = RETICLE_RIGHT;
    }
    if (work->pos.x < RETICLE_LEFT) {
        work->pos.x = RETICLE_LEFT;
    }
    work->pos.y += (pad_records[0].y1 << 8) * reticle_speed;
    if (work->pos.y > RETICLE_BOTTOM) {
        work->pos.y = RETICLE_BOTTOM;
    }
    if (work->pos.y < RETICLE_TOP) {
        work->pos.y = RETICLE_TOP;
    }
}

static void reticle_destroy(struct task *task)
{
}

void locks_clear(void)
{
    int i;

    for (i = 0; i < LOCKS; i++) {
        lock_targets[i] = NULL;
    }
}

/* TRUE if collision's owner, without a lock, is ahead of the camera and
 * near the reticle, else FALSE. near is within RETICLE_REACH on the screen */
static Bool reticle_over(struct collision_work *collision,
                         struct task_work *reticle)
{
    NJS_POINT3 point = {0.0F, 0.0F, 0.0F};
    NJS_POINT2 screen;
    struct task_work *work = collision->owner->work;
    Float dx;
    Float dy;

    if (collision->owner->extra != NULL) {
        return FALSE;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njCalcPoint(NULL, &point, &point);
    njPopMatrix(1);
    if (point.z < 0.0F) {
        return FALSE;
    }
    njPushMatrix(NULL);
    njProjectScreen(NULL, &work->pos, &screen);
    njPopMatrix(1);
    dx = screen.x - reticle->pos.x;
    dy = screen.y - reticle->pos.y;
    if ((dx * dx) + (dy * dy) < RETICLE_REACH) {
        return TRUE;
    }
    return FALSE;
}

/* @hack spelling: both points walked through pointers, which reads them with
 * FMOV.S @R4+ and @R5+ (0x0c9031b8); as reads of their fields, 16 of 17
 * words differ */
static Float distance_squared(NJS_POINT3 *from, NJS_POINT3 *to)
{
    Float *one = &from->x;
    Float *other = &to->x;
    Float dx;
    Float dy;
    Float dz;

    dx = *one - *other;
    one++;
    other++;
    dy = *one - *other;
    one++;
    other++;
    dz = *one - *other;
    return (dx * dx) + (dy * dy) + (dz * dz);
}

/* lock the first target found under the reticle that has no lock, if a
 * lock is free. the search takes items, then enemies, within LOCK_REACH of
 * the tornado, LOCKS at most. it always returns -1, which reticle_step
 * ignores */
static int lock_take(struct task_work *reticle)
{
    struct task_work *player = player_work[0];
    int count = 0;
    struct task *found = NULL;
    struct collision_work **list;
    struct collision_work *collision;
    struct task *target;
    struct task *lock;
    Bool fresh;
    int i;
    int j;

    for (i = 0, list = collision_items; i < collision_item_count; i++) {
        collision = *list++;
        if (distance_squared(&collision->owner->work->pos, &player->pos) <
                LOCK_REACH &&
            reticle_over(collision, reticle)) {
            if (count >= LOCKS) {
                break;
            }
            lock_found[count++] = collision;
        }
    }
    for (i = 0, list = collision_enemies; i < collision_enemy_count; i++) {
        collision = *list++;
        if (distance_squared(&collision->owner->work->pos, &player->pos) <
                LOCK_REACH &&
            reticle_over(collision, reticle)) {
            if (count >= LOCKS) {
                break;
            }
            lock_found[count++] = collision;
        }
    }
    /* @hack redundant: list set to lock_found, which the loop never reads,
     * keeps lock_found's address in R11 (MOV R9,R11 at 0x0c9032ec); without
     * it, 77 of 179 words differ */
    for (i = 0, list = lock_found; i < count; i++) {
        fresh = TRUE;
        target = lock_found[i]->owner;
        for (j = 0; j < LOCKS; j++) {
            if (lock_targets[j] == NULL) {
                continue;
            }
            if (lock_targets[j] == target) {
                fresh = FALSE;
                break;
            }
        }
        if (fresh) {
            found = target;
            break;
        }
    }
    if (found != NULL) {
        for (i = 0; i < LOCKS; i++) {
            if (lock_targets[i] == NULL) {
                break;
            }
        }
        if (i < LOCKS) {
            lock = task_create(TASK_WORK, 3, lock_step);
            if (lock != NULL) {
                lock_targets[i] = found;
                lock->parent = found;
                sound_play(SE_LOCK, 0, 0, 0);
            }
        }
    }
    return -1;
}

static Bool locks_full(void)
{
    int free;
    int i;

    /* @hack spelling: i and free set in one expression, which keeps the loop a
     * loop (BF/S at 0x0c9033de); as two statements, SHC unrolls it and 21
     * of 21 words differ */
    for (i = free = 0; i < LOCKS; i++) {
        if (lock_targets[i] == NULL) {
            free++;
        }
    }
    if (free == 0) {
        return TRUE;
    }
    return FALSE;
}

/* the task nearest the tornado of the first LOCKS under the reticle within
 * BULLET_REACH, or NULL for none. items are searched before
 * enemies */
static struct task *reticle_nearest(struct task_work *reticle)
{
    struct collision_work *found[LOCKS];
    Float distances[LOCKS];
    struct task_work *player = player_work[0];
    int count = 0;
    struct collision_work **list;
    struct collision_work *collision;
    struct task *nearest;
    Float distance;
    Float least;
    int i;

    for (i = 0, list = collision_items; i < collision_item_count; i++) {
        collision = *list++;
        distance = distance_squared(&collision->owner->work->pos, &player->pos);
        if (distance < BULLET_REACH && reticle_over(collision, reticle)) {
            if (count >= LOCKS) {
                break;
            }
            found[count] = collision;
            distances[count++] = distance;
        }
    }
    for (i = 0, list = collision_enemies; i < collision_enemy_count; i++) {
        collision = *list++;
        distance = distance_squared(&collision->owner->work->pos, &player->pos);
        if (distance < BULLET_REACH && reticle_over(collision, reticle)) {
            if (count >= LOCKS) {
                break;
            }
            found[count] = collision;
            distances[count++] = distance;
        }
    }
    if (count > 0) {
        nearest = found[0]->owner;
        least = distances[0];
        for (i = 1; i < count; i++) {
            if (distances[i] < least) {
                nearest = found[i]->owner;
                least = distances[i];
            }
        }
        return nearest;
    }
    return NULL;
}

/* put the task 1000 ahead of the camera. the reticle's offset from the
 * screen's center moves it across the view, a unit for each pixel. a bullet
 * flies for it when reticle_nearest finds no target */
static void reticle_point_step(struct task *task)
{
    /* @hack redundant: 12 bytes of stack no instruction reads or writes, ADD
     * #-12,R15 at 0x0c903542; without them, 39 of 41 words differ */
    NJS_POINT3 unused;
    struct task_work *work = task->work;
    struct task_work *reticle = task->parent->work;

    work->pos.x = reticle->pos.x - CENTER_X;
    work->pos.y = reticle->pos.y - CENTER_Y;
    work->pos.z = 1000.0F;
    njPushMatrix(NULL);
    njInvertMatrix(NULL);
    njCalcPoint(NULL, &work->pos, &work->pos);
    njPopMatrix(1);
}

/* a lock's step. its first step hooks the target's destructor and makes
 * the lock the target's extra. the marker then grows in and follows the
 * target until lock_expired returns TRUE */
static void lock_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *target = task->parent->work;

    if (shooting_event_playing()) {
        return;
    }
    switch (work->mode) {
    case LOCK_INIT:
        work->pos = target->pos;
        VARS(work)->step = task->parent->step;
        VARS(work)->destructor = task->parent->destructor;
        task->parent->destructor = lock_target_destroy;
        task->parent->extra = task;
        task->destructor = lock_destroy;
        task->draw = lock_draw;
        TASK_TIMER(work) = LOCK_STEPS;
        LOCK_GROWTH(work) = 0.5F;
        LOCK_HEIGHT(work) = 0.0F;
        work->mode = LOCK_GROW;
        break;
    case LOCK_GROW:
        if (lock_expired(task)) {
            work->mode = LOCK_GONE;
        } else {
            LOCK_HEIGHT(work) += LOCK_GROWTH(work) * LOCK_GROWTH(work);
            LOCK_GROWTH(work) *= 0.85F;
            /* @bug logic: from a growth of 0.5, the height gains 0.25,
             * then 0.85 squared times as much each step. it nears 0.25 / (1 -
             * 0.7225), about 0.901, so the test (FCMP/GT at 0x0c903692) never
             * holds. the marker never stands full height, and the lock never
             * enters LOCK_HOLD */
            if (LOCK_HEIGHT(work) >= 1.0F) {
                LOCK_HEIGHT(work) = 1.0F;
                work->mode = LOCK_HOLD;
            }
            work->pos = target->pos;
            work->ang.z = 0;
            lock_draw(task);
        }
        break;
    case LOCK_HOLD:
        if (lock_expired(task)) {
            work->mode = LOCK_GONE;
        } else {
            work->pos = target->pos;
            work->ang.z = 0;
            lock_draw(task);
        }
        break;
    case LOCK_GONE:
        task_destroy(task);
        break;
    }
}

/* the marker over its target, as tall as it has grown. it draws at a
 * frame's first step only, and not while an event or the score plays */
static void lock_draw(struct task *task)
{
    NJS_POINT2 screen;
    struct task_work *work = task->work;

    if (frame_step == 0 && !shooting_event_playing()) {
        njPushMatrix(NULL);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
        njSetTexture(&lock_texlist);
        njProjectScreen(NULL, &work->pos, &screen);
        lock_sprite.p.x = screen.x;
        lock_sprite.p.y = screen.y;
        lock_sprite.sy = LOCK_HEIGHT(work);
        njDrawSprite2D(&lock_sprite, TEXANIM_LOCK, 1000.0F,
                       NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    }
}

/* free the lock's place among the locks, and give the target its own
 * destructor back */
static void lock_destroy(struct task *task)
{
    int i;

    for (i = 0; i < LOCKS; i++) {
        if (lock_targets[i] == NULL) {
            continue;
        }
        if (lock_targets[i] == task->parent) {
            lock_targets[i] = NULL;
            break;
        }
    }
    lock_let_go(task);
    task->parent = NULL;
}

/* TRUE once the target is behind the camera, the lock's steps are up or the
 * target's step changes. otherwise FALSE. a call with the target ahead of the
 * camera counts one of the lock's steps */
static Bool lock_expired(struct task *task)
{
    NJS_POINT3 point = {0.0F, 0.0F, 0.0F};
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    /* @hack spelling: the pos read through task->work again, not work, which
     * reloads it from task (MOV.L @(32,R14),R5 at 0x0c903864); through work,
     * 52 of 61 words differ */
    njTranslateV(NULL, &task->work->pos);
    njCalcPoint(NULL, &point, &point);
    njPopMatrix(1);
    /* @hack spelling: the step read through task->work again too (MOV.L
     * @(32,R14),R3 at 0x0c90389a); through work, 52 of 61 words differ */
    if (point.z < 0.0F || --TASK_TIMER(work) <= 0 ||
        task->parent->step != VARS(task->work)->step) {
        return TRUE;
    }
    return FALSE;
}

/* the destructor a lock gives its target: destroy the lock, then run the
 * target's own */
static void lock_target_destroy(struct task *task)
{
    struct task *lock = task->extra;

    task->destructor = VARS(lock->work)->destructor;
    task->extra = NULL;
    task_destroy(lock);
    task->destructor(task);
}

/* give the target its own destructor back, if the lock still holds it,
 * and forget the target */
static void lock_let_go(struct task *task)
{
    struct task *target = task->parent;

    if (target->extra != NULL) {
        target->destructor = VARS(task->work)->destructor;
        target->extra = NULL;
    }
    task->parent = NULL;
}

void lock_release(struct task *task)
{
    struct task *lock;

    if (task->extra != NULL) {
        lock = task->extra;
        task->destructor = VARS(lock->work)->destructor;
        task->extra = NULL;
        task_destroy(lock);
    }
}

Bool lock_holds(struct task *target)
{
    int i;

    for (i = 0; i < LOCKS; i++) {
        if (lock_targets[i] == target) {
            return TRUE;
        }
    }
    return FALSE;
}

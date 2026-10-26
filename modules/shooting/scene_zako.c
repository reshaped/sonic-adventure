/* a scene's zako: a zako that flies its scene's path while the tornado's
 * point is in the scene. a gun on its back fires at player 0. a shot's hit
 * sends the zako tumbling away */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "enemy/enemy.h"
#include "player/lockon.h"
#include "player/player.h"
#include "shooting/hud.h"
#include "shooting/reticle.h"
#include "shooting/scene_zako.h"
#include "shooting/shooting.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* a gun shot's work->vars. its work->scl is its velocity */
struct scene_shot_vars {
    Uint8 unk_00[4];
    Sint16 timer; /* steps left to fly */
    Uint8 unk_06[2];
    Float unk_08; /* set to 0, never read */
};

#define SCENE_SHOT_VARS(work) ((struct scene_shot_vars *)&(work)->vars)

/* a gun's modes, and its shots' */
enum gun_mode { GUN_INIT = 0, GUN_RUN = 1, GUN_GONE = 5 };

#define ZAKO_POINTS 30 /* a hit's points */
#define ZAKO_FALL_STEPS 120 /* the steps it falls once hit */
#define ZAKO_FRAMES 15 /* the frames its action loops through */
/* how near the camera it draws zako_near_action rather than
 * zako_far_action */
#define ZAKO_NEAR 1000.0F
/* how near player 0 it plays SE_ZAKO_NEAR, squared */
#define ZAKO_SOUND_RANGE 1080000.0F

#define GUN_INTERVAL 30 /* steps on the screen between a gun's shots */
/* a gun shot's speed, and the steps it flies */
#define SCENE_SHOT_SPEED 40.0F
#define SCENE_SHOT_STEPS 120
/* how many path points ahead of its own an actor faces */
#define FACE_AHEAD 3

extern NJS_TEXLIST shooting0_texlist;
extern NJS_ACTION zako_near_action;
extern NJS_ACTION zako_far_action;
extern NJS_OBJECT zako_gun_object;
extern NJS_OBJECT gun_shot_object;

static void scene_zako_draw(struct task *task);
static int scene_zako_track(struct task_work *work);
static int scene_zako_follow(struct task_work *work);
static void scene_zako_take_hit(struct task *task);
static void scene_zako_fall(struct task *task);
static void scene_zako_destroy(struct task *task);
static void scene_zako_face(struct task_work *work, int point, int unused);
static void scene_zako_animate(struct task_work *work);
static void zako_gun_step(struct task *task);
static void zako_gun_draw(struct task *task);
static void zako_gun_destroy(struct task *task);
static void scene_shot_draw(struct task_work *work);
static void scene_zako_sound(struct task_work *work);

static struct collision_shape scene_zako_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL,
     {0.0F, 0.0F, 0.0F},
     60.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

void scene_zako_step(struct task *task)
{
    struct task_work *work = task->work;

    if (shooting_event_playing()) {
        return;
    }
    switch (work->mode) {
    case SCENE_INIT:
        work->mode = SCENE_FLY;
        task->destructor = scene_zako_destroy;
        task->draw = scene_zako_draw;
        enemy_alloc_record(task, work);
        collision_init(task, scene_zako_shapes, COLLISION_ENEMY, 1, 1);
        task_create_child(TASK_WORK, zako_gun_step, task);
        SCENE_VARS(work)->frame = 0;
        SCENE_VARS(work)->sounded = 0;
        break;
    case SCENE_FLY:
        scene_zako_take_hit(task);
        if (!placement_editing(task)) {
            if (scene_zako_track(work)) {
                scene_zako_draw(task);
                scene_zako_animate(work);
                scene_zako_follow(work);
                scene_zako_face(work, shooting_path_index, 0);
                collision_register(work);
                scene_zako_sound(work);
            } else {
                work->mode = SCENE_WAIT;
            }
        }
        break;
    case SCENE_WAIT:
        scene_zako_take_hit(task);
        if (scene_zako_track(work)) {
            scene_zako_follow(work);
            work->mode = SCENE_FLY;
        }
        break;
    case SCENE_FALL:
        scene_zako_draw(task);
        scene_zako_animate(work);
        shooting_actors[SCENE_VARS(work)->scene].active = 0;
        scene_zako_fall(task);
        break;
    case SCENE_GONE:
        shooting_actors[SCENE_VARS(work)->scene].active = 0;
        task_destroy(task);
        return;
    /* @hack redundant: a default that only breaks, which loads
     * task_step_children's address into R3 for the tail jump (0x0c906fc4);
     * without it, R2, and 2 of 139 words differ */
    default:
        break;
    }
    task_step_children(task);
}

static void scene_zako_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *action;
    Float distance;

    if (frame_step || shooting_event_playing()) {
        return;
    }
    if (!scene_zako_track(work)) {
        return;
    }
    distance = njDistanceP2P(&camera_work->pos, &work->pos);
    if (distance < ZAKO_NEAR) {
        action = &zako_near_action;
    } else {
        action = &zako_far_action;
    }
    njFogDisable();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njPushMatrix(NULL);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njScale(NULL, 2.0F, 2.0F, 2.0F);
    njSetTexture(&shooting0_texlist);
    draw_action(action, SCENE_VARS(work)->frame);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* 1 while the tornado's point is in the scene, else 0. it marks the
 * scene's entry in shooting_actors active or not to match. once the point
 * passes *end, the actor goes */
static int scene_zako_track(struct task_work *work)
{
    Sint8 scene = SCENE_VARS(work)->scene;
    int begin = *shooting_scenes[scene].begin;
    int end = *shooting_scenes[scene].end;

    if (shooting_path_index >= begin && shooting_path_index < end) {
        shooting_actors[scene].active = 1;
        return 1;
    } else {
        shooting_actors[scene].active = 0;
        if (shooting_path_index > end) {
            work->mode = SCENE_GONE;
        }
        return 0;
    }
}

/* move the actor to its path's point for the tornado's point. that point
 * must be from *begin up to *end. it returns 1, which no caller reads */
static int scene_zako_follow(struct task_work *work)
{
    /* scene holds an index, not a character */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int scene = SCENE_VARS(work)->scene;
    NJS_POINT3 *path = shooting_scenes[scene].path;
    int begin = *shooting_scenes[scene].begin;
    /* @hack redundant: end, set and never read, which keeps begin in R6
     * and path in R5 (MOV.L @(8,R7),R6 at 0x0c907170); without it, the two
     * trade and 9 of 45 words differ */
    int *end = shooting_scenes[scene].end;

    work->pos.x = path[shooting_path_index - begin].x;
    work->pos.y = path[shooting_path_index - begin].y;
    work->pos.z = path[shooting_path_index - begin].z;
    return 1;
}

/* a hit releases the zako's lock, scores ZAKO_POINTS and counts as a target
 * destroyed. it sends the zako tumbling with a burst, at a random velocity
 * that starts upward or level */
static void scene_zako_take_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & COLLISION_DAMAGED) {
        lock_release(task);
        shooting_score_hit(ZAKO_POINTS, work);
        shooting_count_hit();
        work->mode = SCENE_FALL;
        SCENE_VARS(work)->timer = ZAKO_FALL_STEPS;
        bomb_create(&work->pos, 3.0F);
        work->scl.x = RANDOM(-5.0F, 5.0F);
        work->scl.y = rand() * (10.0F * RAND_SCALE);
        work->scl.z = RANDOM(-5.0F, 5.0F);
        SCENE_VARS(work)->spin_x = (int)RANDOM(-1024.0F, 1024.0F);
        SCENE_VARS(work)->spin_y = (int)RANDOM(-1024.0F, 1024.0F);
        SCENE_VARS(work)->spin_z = (int)RANDOM(-1024.0F, 1024.0F);
        shooting_actors[SCENE_VARS(work)->scene].active = 0;
        sound_play(SE_HIT, 0, 0, 0);
    }
}

/* the zako falls and spins for ZAKO_FALL_STEPS, then goes. its y
 * velocity drops by 0.3 a step, and it bursts every 8 steps */
static void scene_zako_fall(struct task *task)
{
    struct task_work *work = task->work;

    if (SCENE_VARS(work)->timer-- > 0) {
        /* @hack spelling: the sums spelled out, which read each scl field
         * with FMOV.S @(R0,R14) (0x0c907314); with +=, SHC forms each one's
         * address in R1 first, the function runs 6 bytes long and 52 of 62
         * words differ */
        work->pos.x = work->pos.x + work->scl.x;
        work->pos.y = work->pos.y + work->scl.y;
        work->pos.z = work->pos.z + work->scl.z;
        work->ang.x += SCENE_VARS(work)->spin_x;
        work->ang.y += SCENE_VARS(work)->spin_y;
        work->ang.z += SCENE_VARS(work)->spin_z;
        work->scl.y -= 0.3F;
        if (!(SCENE_VARS(work)->timer & 7)) {
            bomb_create(&work->pos, 3.0F);
        }
    } else {
        work->mode = SCENE_GONE;
    }
}

static void scene_zako_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}

/* turn the actor along its path, from the path's point for point toward
 * the one FACE_AHEAD on. a point ahead at *end or past it wraps to *begin.
 * the actor keeps its turn while point is outside *begin to *end. nothing
 * reads unused, which scene_zako_step passes as 0 */
static void scene_zako_face(struct task_work *work, int point, int unused)
{
    /* scene holds an index, not a character */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int index = SCENE_VARS(work)->scene;
    struct shooting_scene *scene = &shooting_scenes[index];
    int begin = *scene->begin;
    NJS_POINT3 *path = scene->path;
    int end = *scene->end;
    int ahead;
    Float x;
    Float y;
    Float z;
    Float across;
    Angle ang_y;
    Angle ang_x;

    if (point >= begin) {
        if (point > end) {
            return;
        }
        ahead = point + FACE_AHEAD;
        if (ahead >= end) {
            ahead = begin;
        }
        point -= begin;
        ahead -= begin;
        x = path[ahead].x - path[point].x;
        y = path[ahead].y - path[point].y;
        z = path[ahead].z - path[point].z;
        across = njSqrt((x * x) + (z * z));
        ang_y = -(Angle)(atan2f(z, x) * ANGLE_PER_RADIAN) + 0xc000;
        ang_x = (Angle)(atan2f(y, across) * ANGLE_PER_RADIAN);
        work->ang.x = ang_x;
        work->ang.y = ang_y;
    }
}

void scene_gun_aim(struct task_work *work)
{
    NJS_VECTOR way;
    Float dx;
    Float dy;
    Float dz;
    Float flat;
    Angle ang_y;
    Angle ang_x;

    way.x = player_work[0]->pos.x - work->pos.x;
    way.y = player_work[0]->pos.y - work->pos.y;
    way.z = player_work[0]->pos.z - work->pos.z;
    njUnitVector(&way);
    work->scl.x = way.x * SCENE_SHOT_SPEED;
    work->scl.y = way.y * SCENE_SHOT_SPEED;
    work->scl.z = way.z * SCENE_SHOT_SPEED;
    dx = player_work[0]->pos.x - work->pos.x;
    dy = player_work[0]->pos.y - work->pos.y;
    dz = player_work[0]->pos.z - work->pos.z;
    flat = (dx * dx) + (dz * dz);
    flat = njSqrt(flat);
    ang_y = 0x4000 - (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
    ang_x = -(Angle)(atan2f(dy, flat) * ANGLE_PER_RADIAN);
    work->ang.x = ang_x;
    work->ang.y = ang_y;
}

static void scene_zako_animate(struct task_work *work)
{
    SCENE_VARS(work)->frame++;
    if (SCENE_VARS(work)->frame >= ZAKO_FRAMES) {
        SCENE_VARS(work)->frame = 0;
    }
}

/* while the zako flies, the gun rides it, aims at player 0 and draws.
 * while the zako is also on the screen, the gun fires. it then waits
 * GUN_INTERVAL steps on the screen to fire again */
static void zako_gun_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *zako = task->parent->work;

    switch (work->mode) {
    case GUN_INIT:
        work->ang.x = 0;
        work->ang.y = 0;
        work->ang.z = 0;
        SCENE_VARS(work)->timer = 0;
        task->destructor = zako_gun_destroy;
        task->draw = zako_gun_draw;
        work->mode = GUN_RUN;
        break;
    case GUN_RUN:
        if (zako->mode == SCENE_FLY) {
            work->pos = zako->pos;
            scene_gun_aim(work);
            zako_gun_draw(task);
            if (draw_on_screen(zako)) {
                if (SCENE_VARS(work)->timer == 0) {
                    task_create_child(TASK_WORK, scene_shot_step, task);
                    SCENE_VARS(work)->timer = GUN_INTERVAL;
                } else {
                    SCENE_VARS(work)->timer--;
                }
            }
        }
        break;
    case GUN_GONE:
        task_kill(task);
        break;
    }
    task_step_children(task);
}

/* the gun at its place on the zako's back, turned about y by its aim. it
 * draws at a frame's first step only */
static void zako_gun_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *zako = task->parent->work;
    Angle ang;

    if (!frame_step) {
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, zako->pos.x, zako->pos.y, zako->pos.z);
        njRotateXYZ(NULL, zako->ang.x, zako->ang.y, zako->ang.z);
        njScale(NULL, 2.0F, 2.0F, 2.0F);
        njTranslate(NULL, 0.0F, -5.014568F, -1.932182F);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njSetTexture(&shooting0_texlist);
        draw_tree(&zako_gun_object);
        njPopMatrix(1);
    }
}

/* the task's work is freed with it, so nothing reads the mode this sets */
static void zako_gun_destroy(struct task *task)
{
    task->work->mode = GUN_GONE;
}

static void scene_shot_draw(struct task_work *work)
{
    if (!frame_step) {
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
        njScale(NULL, 1.0F, 1.0F, -1.0F);
        njSetTexture(&shooting0_texlist);
        draw_tree(&gun_shot_object);
        njPopMatrix(1);
    }
}

void scene_shot_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *gun = task->parent->work;

    switch (work->mode) {
    case GUN_INIT:
        work->scl = gun->scl;
        work->ang = gun->ang;
        work->pos = gun->pos;
        SCENE_SHOT_VARS(work)->unk_08 = 0.0F;
        SCENE_SHOT_VARS(work)->timer = SCENE_SHOT_STEPS;
        work->mode = GUN_RUN;
        break;
    case GUN_RUN:
        scene_shot_draw(work);
        work->pos.x += work->scl.x;
        work->pos.y += work->scl.y;
        work->pos.z += work->scl.z;
        if (--SCENE_SHOT_VARS(work)->timer <= 0) {
            work->mode = GUN_GONE;
        }
        break;
    case GUN_GONE:
        task_kill(task);
        return;
    }
}

/* play SE_ZAKO_NEAR the first time the zako comes within ZAKO_SOUND_RANGE
 * of player 0 */
static void scene_zako_sound(struct task_work *work)
{
    Float dx;
    Float dy;
    Float dz;
    Float distance;

    dx = player_work[0]->pos.x - work->pos.x;
    dy = player_work[0]->pos.y - work->pos.y;
    dz = player_work[0]->pos.z - work->pos.z;
    distance = (dx * dx) + (dy * dy) + (dz * dz);
    if (distance <= ZAKO_SOUND_RANGE) {
        if (SCENE_VARS(work)->sounded == 0) {
            sound_play(SE_ZAKO_NEAR, 0, 0, 0);
            SCENE_VARS(work)->sounded = 1;
        }
    }
}

/* a scene's renzako: a renzako that flies its scene's path while the
 * tornado's point is in the scene. its gun draws nothing, and fires the
 * scene zako's shots at player 0. a shot's hit sends the renzako tumbling
 * away */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "effect/bomb.h"
#include "enemy/enemy.h"
#include "player/lockon.h"
#include "player/player.h"
#include "shooting/hud.h"
#include "shooting/reticle.h"
#include "shooting/scene_renzako.h"
#include "shooting/scene_zako.h"
#include "shooting/shooting.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

enum gun_mode { GUN_INIT = 0, GUN_RUN = 1, GUN_GONE = 5 };

#define RENZAKO_POINTS 50 /* a hit's points */
#define RENZAKO_FALL_STEPS 120 /* the steps it falls once hit */
#define RENZAKO_FRAMES 15 /* the frames its action loops through */
/* how near the camera it draws renzako_action rather than
 * renzako_far_action */
#define RENZAKO_NEAR 1000.0F
/* how near player 0 it plays SE_ZAKO_NEAR, squared */
#define RENZAKO_SOUND_RANGE 1080000.0F

#define GUN_INTERVAL 30 /* steps on the screen between the gun's shots */
/* how many path points ahead of its own an actor faces */
#define FACE_AHEAD 3

extern NJS_TEXLIST shooting0_texlist;
extern NJS_ACTION renzako_action;
extern NJS_ACTION renzako_far_action;

static void scene_renzako_draw(struct task *task);
static int scene_renzako_track(struct task_work *work);
static int scene_renzako_follow(struct task_work *work);
static void scene_renzako_take_hit(struct task *task);
static void scene_renzako_fall(struct task *task);
static void scene_renzako_destroy(struct task *task);
static void scene_renzako_face(struct task_work *work, int point, int unused);
static void scene_renzako_animate(struct task_work *work);
static void renzako_gun_step(struct task *task);
static void renzako_gun_destroy(struct task *task);
static void scene_renzako_sound(struct task_work *work);

static struct collision_shape scene_renzako_shapes[1] = {
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

void scene_renzako_step(struct task *task)
{
    struct task_work *work = task->work;

    if (shooting_event_playing()) {
        return;
    }
    switch (work->mode) {
    case SCENE_INIT:
        work->mode = SCENE_FLY;
        task->destructor = scene_renzako_destroy;
        task->draw = scene_renzako_draw;
        enemy_alloc_record(task, work);
        collision_init(task, scene_renzako_shapes, COLLISION_ENEMY, 1, 1);
        task_create_child(TASK_WORK, renzako_gun_step, task);
        SCENE_VARS(work)->frame = 0;
        break;
    case SCENE_FLY:
        scene_renzako_take_hit(task);
        if (!placement_editing(task)) {
            if (scene_renzako_track(work)) {
                scene_renzako_draw(task);
                scene_renzako_animate(work);
                scene_renzako_follow(work);
                scene_renzako_face(work, shooting_path_index, 0);
                collision_register(work);
                scene_renzako_sound(work);
            } else {
                work->mode = SCENE_WAIT;
            }
        }
        break;
    case SCENE_WAIT:
        scene_renzako_take_hit(task);
        if (scene_renzako_track(work)) {
            scene_renzako_follow(work);
            work->mode = SCENE_FLY;
        }
        break;
    case SCENE_FALL:
        scene_renzako_draw(task);
        scene_renzako_animate(work);
        shooting_actors[SCENE_VARS(work)->scene].active = 0;
        scene_renzako_fall(task);
        break;
    case SCENE_GONE:
        shooting_actors[SCENE_VARS(work)->scene].active = 0;
        task_destroy(task);
        return;
    /* @hack redundant: a default that only breaks, which loads
     * task_step_children's address into R3 for the tail jump (0x0c907a84);
     * without it, R2, and 2 of 137 words differ */
    default:
        break;
    }
    task_step_children(task);
}

static void scene_renzako_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ACTION *action;
    Float distance;

    if (frame_step || shooting_event_playing()) {
        return;
    }
    if (!scene_renzako_track(work)) {
        return;
    }
    distance = njDistanceP2P(&camera_work->pos, &work->pos);
    if (distance < RENZAKO_NEAR) {
        action = &renzako_action;
    } else {
        action = &renzako_far_action;
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
static int scene_renzako_track(struct task_work *work)
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
static int scene_renzako_follow(struct task_work *work)
{
    /* scene holds an index, not a character */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int scene = SCENE_VARS(work)->scene;
    NJS_POINT3 *path = shooting_scenes[scene].path;
    int begin = *shooting_scenes[scene].begin;
    /* @hack redundant: end, set and never read, as in scene_zako_follow
     * (MOV.L @(8,R7),R6 at 0x0c907c30); without it, begin and path trade
     * R6 and R5, and 9 of 45 words differ */
    int *end = shooting_scenes[scene].end;

    work->pos.x = path[shooting_path_index - begin].x;
    work->pos.y = path[shooting_path_index - begin].y;
    work->pos.z = path[shooting_path_index - begin].z;
    return 1;
}

/* a hit releases the renzako's lock, scores RENZAKO_POINTS and counts as a
 * target destroyed. it sends the renzako tumbling with a burst, at a random
 * velocity that starts upward or level */
static void scene_renzako_take_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & COLLISION_DAMAGED) {
        lock_release(task);
        shooting_score_hit(RENZAKO_POINTS, work);
        shooting_count_hit();
        work->mode = SCENE_FALL;
        SCENE_VARS(work)->timer = RENZAKO_FALL_STEPS;
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

/* the renzako falls and spins for RENZAKO_FALL_STEPS, then goes. its y
 * velocity drops by 0.3 a step, and it bursts every 8 steps */
static void scene_renzako_fall(struct task *task)
{
    struct task_work *work = task->work;

    if (SCENE_VARS(work)->timer-- > 0) {
        /* @hack spelling: the sums spelled out, as in scene_zako_fall
         * (0x0c907dd4); with +=, the function runs 6 bytes long and 52 of 62
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

static void scene_renzako_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}

/* turn the actor along its path, from the path's point for point toward
 * the one FACE_AHEAD on. a point ahead at *end or past it wraps to *begin.
 * the actor keeps its turn while point is outside *begin to *end. nothing
 * reads unused, which scene_renzako_step passes as 0 */
static void scene_renzako_face(struct task_work *work, int point, int unused)
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

static void scene_renzako_animate(struct task_work *work)
{
    /* @hack spelling: frame stepped as frame = frame + 1, which loads it
     * into R2 (MOV.B @(R0,R4),R2 at 0x0c907f0c); as ++, R2 and R3 trade,
     * and 6 of 12 words differ */
    SCENE_VARS(work)->frame = SCENE_VARS(work)->frame + 1;
    if (SCENE_VARS(work)->frame >= RENZAKO_FRAMES) {
        SCENE_VARS(work)->frame = 0;
    }
}

/* while the renzako flies, the gun rides it unseen and aims at player 0.
 * while the renzako is also on the screen, the gun fires. it then waits
 * GUN_INTERVAL steps on the screen to fire again */
static void renzako_gun_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *renzako = task->parent->work;

    switch (work->mode) {
    case GUN_INIT:
        work->ang.x = 0;
        work->ang.y = 0;
        work->ang.z = 0;
        work->pos.x = 0.0F;
        work->pos.y = 0.0F;
        work->pos.z = 0.0F;
        SCENE_VARS(work)->timer = 0;
        task->destructor = renzako_gun_destroy;
        work->mode = GUN_RUN;
        break;
    case GUN_RUN:
        if (renzako->mode == SCENE_FLY) {
            work->pos = renzako->pos;
            scene_gun_aim(work);
            if (draw_on_screen(renzako)) {
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
        task_destroy(task);
        return;
    /* @hack redundant: a default that only breaks, which loads
     * task_step_children's address into R2 for the tail jump (0x0c908012);
     * without it, R3, and 2 of 79 words differ */
    default:
        break;
    }
    task_step_children(task);
}

static void renzako_gun_destroy(struct task *task)
{
}

/* play SE_ZAKO_NEAR the first time the renzako comes within
 * RENZAKO_SOUND_RANGE of player 0 */
static void scene_renzako_sound(struct task_work *work)
{
    Float dx;
    Float dy;
    Float dz;
    Float distance;

    dx = player_work[0]->pos.x - work->pos.x;
    dy = player_work[0]->pos.y - work->pos.y;
    dz = player_work[0]->pos.z - work->pos.z;
    distance = (dx * dx) + (dy * dy) + (dz * dz);
    if (distance <= RENZAKO_SOUND_RANGE) {
        if (SCENE_VARS(work)->sounded == 0) {
            sound_play(SE_ZAKO_NEAR, 0, 0, 0);
            SCENE_VARS(work)->sounded = 1;
        }
    }
}

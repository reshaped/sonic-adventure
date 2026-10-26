/* the boss: BOSS, which flies the course BOSS_BEHIND points behind the
 * tornado. its body and the three parts that hang from it each play their
 * own action. the first part can breathe flame */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "effect/snow.h"
#include "enemy/enemy.h"
#include "player/lockon.h"
#include "shooting/act1_textures.h"
#include "shooting/boss.h"
#include "shooting/course_span.h"
#include "shooting/reticle.h"
#include "shooting/shooting.h"
#include "system/task.h"
#include "system/util.h"

/* the boss's modes. each after BOSS_INIT picks the row of boss_actions its
 * parts play. only BOSS_FLY and BOSS_BREATHE take hits */
enum boss_mode {
    BOSS_INIT,
    BOSS_FLY,
    BOSS_POSE_1,
    BOSS_BREATHE,
    BOSS_POSE_2,
    BOSS_POSE_3
};

#define BOSS_BEHIND 150 /* path points behind the tornado */
#define BOSS_AHEAD 3 /* path points ahead that the boss faces */
#define COURSE_POINTS 9000 /* the length of act 1's path, in points */

/* the boss's parts: its body, and three that hang from the body's nodes */
#define BOSS_PARTS 4
#define NODE_PART_1 32
#define NODE_PART_2 31
#define NODE_PART_3 30
/* nodes of the parts: part 1's mouth, and parts 2's and 3's tips */
#define NODE_MOUTH 8
#define NODE_TIP_2 2
#define NODE_TIP_3 8

#define BOSS_POINTS 100 /* per hit */
#define BREATH_SPEED 60.0F /* along the mouth's -z */

/* the actions of the boss's parts */
extern NJS_ACTION dat_0c98013c;
extern NJS_ACTION dat_0c981a80;
extern NJS_ACTION dat_0c981e64;
extern NJS_ACTION dat_0c985fc8;
extern NJS_ACTION dat_0c98665c;
extern NJS_ACTION dat_0c987500;
extern NJS_ACTION dat_0c987da4;
extern NJS_ACTION dat_0c988c68;
extern NJS_ACTION dat_0c988e8c;
extern NJS_ACTION dat_0c989e10;
extern NJS_ACTION dat_0c98ac34;
extern NJS_ACTION dat_0c98b818;
extern NJS_ACTION dat_0c98ba3c;
extern NJS_ACTION dat_0c98c620;
extern NJS_ACTION dat_0c98ebe4;
/* a path over another course's points, from course_span_start to
 * course_span_end */
extern NJS_POINT3 dat_0caa414c[];

static struct collision_shape boss_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL,
     {0.0F, 0.0F, 0.0F},
     500.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct dust_params boss_flame = {20.0F,
                                        0.07F,
                                        0.4F,
                                        0.06F,
                                        0.025F,
                                        {0.0F, 0.0F, 0.0F},
                                        {0.0F, 0.0F, 0.0F},
                                        {1.0F, 1.0F, 1.0F, 0.0F}};
/* each row gives an action for each part, body first. boss_draw plays
 * only rows 0, 5, 6 and 7 */
static NJS_ACTION *boss_actions[11][BOSS_PARTS] = {
    {&dat_0c985fc8, &dat_0c98013c, &dat_0c981e64, &dat_0c981a80},
    {&dat_0c98ebe4, &dat_0c98b818, &dat_0c98ba3c, &dat_0c98c620},
    {&dat_0c985fc8, &dat_0c98013c, &dat_0c988e8c, &dat_0c981a80},
    {&dat_0c985fc8, &dat_0c98013c, &dat_0c988e8c, &dat_0c981a80},
    {&dat_0c985fc8, &dat_0c98013c, &dat_0c988e8c, &dat_0c981a80},
    {&dat_0c985fc8, &dat_0c989e10, &dat_0c981e64, &dat_0c981a80},
    {&dat_0c985fc8, &dat_0c988c68, &dat_0c981e64, &dat_0c981a80},
    {&dat_0c985fc8, &dat_0c987da4, &dat_0c981e64, &dat_0c981a80},
    {&dat_0c985fc8, &dat_0c98013c, &dat_0c981e64, &dat_0c98ac34},
    {&dat_0c985fc8, &dat_0c98013c, &dat_0c981e64, &dat_0c987500},
    {&dat_0c985fc8, &dat_0c98013c, &dat_0c981e64, &dat_0c98665c},
};
static struct dust_params boss_frost = {1.0F,
                                        0.038F,
                                        0.6F,
                                        0.06F,
                                        0.025F,
                                        {0.0F, 0.0F, 0.0F},
                                        {0.0F, 0.0F, 0.0F},
                                        {1.0F, 1.0F, 1.0F, 1.0F}};

/* the mode boss_step puts back in work->mode at the end of each step.
 * BOSS_INIT alone sets it, to BOSS_FLY */
static int boss_kept_mode;
/* nothing reads or writes it */
static Sint32 dat_0cadd36c;
/* where parts 2, 1 and 3 hang under the view boss_draw draws them in */
static NJS_MATRIX part_2_matrix;
static NJS_MATRIX part_1_matrix;
static NJS_MATRIX part_3_matrix;
/* where the same three hang in the world */
static NJS_MATRIX part_2_world;
static NJS_MATRIX part_1_world;
static NJS_MATRIX part_3_world;
/* part 2's tip, part 1's mouth and part 3's tip, in the world. the breath
 * starts at the mouth, and nothing reads the tips */
static NJS_MATRIX part_2_tip;
static NJS_MATRIX mouth;
static NJS_MATRIX part_3_tip;
static Float part_frames[BOSS_PARTS];

static void boss_draw(struct task *task);
static void boss_flame_breathe(struct task *task);
static void boss_destroy(struct task *task);
static Bool boss_follow(struct task *task);
static void boss_face(struct task_work *work, int point, int unused);
static void boss_take_hit(struct task *task);
static void boss_animate(void);

void boss_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BOSS_INIT: {
        /* @hack redundant: BOSS_FLY as 1.0F && BOSS_FLY, a logical AND of
         * two constants, which SHC makes in R4 (MOV #1,R4 at 0x0c908102). as
         * a plain BOSS_FLY in each store, it takes R3, and 82 of 148 words
         * differ */
        int fly = 1.0F && BOSS_FLY;

        work->mode = fly;
        boss_kept_mode = fly;
        task->destructor = boss_destroy;
        task->draw = NULL;
        part_frames[0] = 0.0F;
        part_frames[1] = 0.0F;
        part_frames[2] = 0.0F;
        part_frames[3] = 0.0F;
        enemy_alloc_record(task, work);
        collision_init(task, boss_shapes, COLLISION_ENEMY, 1, 1);
        break;
    }
    case BOSS_FLY:
        boss_take_hit(task);
        if (boss_follow(task)) {
            task->draw = boss_draw;
            boss_draw(task);
            boss_animate();
            collision_register(work);
        } else {
            task->draw = NULL;
        }
        break;
    case BOSS_POSE_1:
        if (boss_follow(task)) {
            task->draw = boss_draw;
            boss_draw(task);
            boss_animate();
            collision_register(work);
        } else {
            task->draw = NULL;
        }
        break;
    case BOSS_BREATHE:
        boss_take_hit(task);
        if (boss_follow(task)) {
            task->draw = boss_draw;
            part_frames[1] = 29.0F;
            boss_flame_breathe(task);
            boss_draw(task);
            boss_animate();
            collision_register(work);
        } else {
            task->draw = NULL;
        }
        break;
    case BOSS_POSE_2:
        if (boss_follow(task)) {
            task->draw = boss_draw;
            boss_draw(task);
            boss_animate();
            collision_register(work);
        } else {
            task->draw = NULL;
        }
        break;
    case BOSS_POSE_3:
        if (boss_follow(task)) {
            task->draw = boss_draw;
            boss_draw(task);
            boss_animate();
            collision_register(work);
        } else {
            task->draw = NULL;
        }
        break;
    default:
        break;
    }
    work->mode = boss_kept_mode;
}

/* draw the body, then each part where it hangs. a part whose frame has
 * reached its action's frame count starts the action again. then pose the
 * parts in the world, where the breath starts from part 1's mouth */
static void boss_draw(struct task *task)
{
    struct task_work *work = task->work;
    int pose;
    int i;

    switch (work->mode) {
    case BOSS_FLY:
        pose = 0;
        break;
    case BOSS_POSE_1:
        pose = 5;
        break;
    case BOSS_BREATHE:
        pose = 6;
        break;
    case BOSS_POSE_2:
        pose = 7;
        break;
    case BOSS_POSE_3:
        pose = 5;
        break;
    }
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    for (i = 0; i < BOSS_PARTS; ++i) {
        Float end = boss_actions[pose][i]->motion->nbFrame;

        if (part_frames[i] >= end) {
            part_frames[i] = 0.0F;
        }
    }
    njSetTexture(&shooting1_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    draw_action(boss_actions[pose][0], part_frames[0]);
    pose_action(NULL, boss_actions[pose][0], part_frames[0], NODE_PART_1,
                &part_1_matrix);
    njPushMatrix(&part_1_matrix);
    draw_action(boss_actions[pose][1], part_frames[1]);
    njPopMatrix(1);
    pose_get_matrix(NODE_PART_2, &part_2_matrix);
    njPushMatrix(&part_2_matrix);
    draw_action(boss_actions[pose][2], part_frames[2]);
    njPopMatrix(1);
    pose_get_matrix(NODE_PART_3, &part_3_matrix);
    njPushMatrix(&part_3_matrix);
    draw_action(boss_actions[pose][3], part_frames[3]);
    njPopMatrix(1);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    pose_action(NULL, boss_actions[pose][0], part_frames[0], NODE_PART_1,
                &part_1_world);
    pose_get_matrix(NODE_PART_2, &part_2_world);
    pose_get_matrix(NODE_PART_3, &part_3_world);
    pose_action(&part_1_world, boss_actions[pose][1], part_frames[1],
                NODE_MOUTH, &mouth);
    pose_action(&part_2_world, boss_actions[pose][2], part_frames[2],
                NODE_TIP_2, &part_2_tip);
    pose_action(&part_3_world, boss_actions[pose][3], part_frames[3],
                NODE_TIP_3, &part_3_tip);
    njPopMatrix(1);
}

/* flame from part 1's mouth, at BREATH_SPEED */
static void boss_flame_breathe(struct task *task)
{
    NJS_VECTOR way = {0.0F, 0.0F, -BREATH_SPEED};
    NJS_VECTOR speed;
    NJS_POINT3 pos;
    struct task_work *work;

    work = task->work;
    /* @bug logic: the breath count, vars.b[0], is cleared right before
     * its test (MOV.B R0,@(8,R14) at 0x0c90853e). the test always holds, so
     * every call breathes */
    work->vars.b[0] = 0;
    if (work->vars.b[0] == 0) {
        pos.x = mouth[12];
        pos.y = mouth[13];
        pos.z = mouth[14];
        njCalcVector(&mouth, &way, &speed);
        boss_flame.pos = pos;
        boss_flame.speed = speed;
        boss_flame.scale = 20.0F;
        flame_create_from(&boss_flame);
    }
    work->vars.b[0]++;
}

void boss_frost_breathe(struct task *task)
{
    NJS_VECTOR way = {0.0F, 0.0F, -BREATH_SPEED};
    NJS_VECTOR speed;
    NJS_POINT3 pos;
    struct task_work *work;

    work = task->work;
    /* @bug logic: the breath count, vars.b[0], is cleared right before
     * its test (MOV.B R0,@(8,R14) at 0x0c9085c2). the test always holds, so
     * every call breathes */
    work->vars.b[0] = 0;
    if (work->vars.b[0] == 0) {
        pos.x = mouth[12];
        pos.y = mouth[13];
        pos.z = mouth[14];
        njCalcVector(&mouth, &way, &speed);
        boss_frost.pos = pos;
        boss_frost.speed = speed;
        boss_frost.scale = 20.0F;
        snow_create_from(&boss_frost);
    }
    work->vars.b[0]++;
}

static void boss_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}

/* put the boss BOSS_BEHIND points behind the tornado on the course, facing
 * along it, and return TRUE. once the tornado's point reaches
 * COURSE_POINTS, return FALSE and leave the boss where it is */
static Bool boss_follow(struct task *task)
{
    struct task_work *work = task->work;
    int point = shooting_path_index - BOSS_BEHIND;
    Bool on;

    /* @bug memory: the test bounds the tornado's point (CMP/PZ at
     * 0x0c90868a), not the boss's, which is BOSS_BEHIND less. over the
     * tornado's first BOSS_BEHIND points, the read (FMOV.S @R2 at
     * 0x0c9086a6) falls up to BOSS_BEHIND points before shooting_path */
    if (shooting_path_index >= 0 && shooting_path_index < COURSE_POINTS) {
        work->pos.x = shooting_path[point].x;
        work->pos.y = shooting_path[point].y;
        work->pos.z = shooting_path[point].z;
        boss_face(work, point, 0);
        on = TRUE;
    } else {
        on = FALSE;
    }
    return on;
}

void boss_face_other(struct task_work *work, int point)
{
    int ahead;
    Float x;
    Float y;
    Float z;
    Float across;
    Angle ang_y;
    Angle ang_x;

    if (point >= course_span_start && point <= course_span_end) {
        ahead = point + BOSS_AHEAD;
        if (ahead >= course_span_end) {
            ahead = course_span_start;
        }
        point -= course_span_start;
        ahead -= course_span_start;
        x = dat_0caa414c[ahead].x - dat_0caa414c[point].x;
        y = dat_0caa414c[ahead].y - dat_0caa414c[point].y;
        z = dat_0caa414c[ahead].z - dat_0caa414c[point].z;
        across = njSqrt((x * x) + (z * z));
        ang_y = -(Angle)(atan2f(z, x) * ANGLE_PER_RADIAN) + 0xc000;
        ang_x = (Angle)(atan2f(y, across) * ANGLE_PER_RADIAN);
        work->ang.x = ang_x;
        work->ang.y = ang_y;
    }
}

/* turn work to face along the course, from point to BOSS_AHEAD points
 * ahead. the point ahead wraps to 0 at COURSE_POINTS. a point below 0 or
 * past COURSE_POINTS leaves work as it is */
static void boss_face(struct task_work *work, int point, int unused)
{
    int ahead;
    Float x;
    Float y;
    Float z;
    Float across;
    Angle ang_y;
    Angle ang_x;

    if (point >= 0 && point <= COURSE_POINTS) {
        ahead = point + BOSS_AHEAD;
        if (ahead >= COURSE_POINTS) {
            ahead = 0;
        }
        x = shooting_path[ahead].x - shooting_path[point].x;
        y = shooting_path[ahead].y - shooting_path[point].y;
        z = shooting_path[ahead].z - shooting_path[point].z;
        across = njSqrt((x * x) + (z * z));
        ang_y = -(Angle)(atan2f(z, x) * ANGLE_PER_RADIAN) + 0xc000;
        ang_x = (Angle)(atan2f(y, across) * ANGLE_PER_RADIAN);
        work->ang.x = ang_x;
        work->ang.y = ang_y;
    }
}

/* a hit scores BOSS_POINTS and bursts a bomb on the boss */
static void boss_take_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & COLLISION_DAMAGED) {
        lock_release(task);
        shooting_score += BOSS_POINTS;
        bomb_create(&work->pos, 3.0F);
    }
}

static void boss_animate(void)
{
    part_frames[0] += 1.0F;
    part_frames[1] += 1.0F;
    part_frames[2] += 1.0F;
    part_frames[3] += 1.0F;
}

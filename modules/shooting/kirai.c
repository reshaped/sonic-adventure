/* the kirai and the hodai. KIRAI is a mine that bursts at a touch. HODAI is
 * a battery that fires shells at the path ahead of the tornado. it fires
 * while the tornado is in its stretch of the course. one hit downs it */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "shooting/hud.h"
#include "shooting/kirai.h"
#include "shooting/reticle.h"
#include "shooting/shooting.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* shared by a kirai, a hodai and a hodai's shells */
struct kirai_vars {
    Float frame; /* how far the action has played */
    /* steps until a kirai or a shell goes, or until a hodai fires again */
    Sint16 wait;
    Uint8 unk_06[2];
    /* @hack type: the hits and the stretch as one array, whose element SHC
     * reaches through its address, the work plus 16 (ADD #16 at 0x0c905baa
     * and 0x0c905cfc). as two fields, SHC indexes the work through R0, and
     * 58 of hodai_step's 104 words and 40 of hodai_take_hit's 46 differ */
    Sint8 counts[2];
};

#define VARS(work) ((struct kirai_vars *)&(work)->vars)
/* a hodai's counts: the hits it has left, and the stretch of the course its
 * scl.x picks */
#define HITS(work) (VARS(work)->counts[0])
#define STRETCH(work) (VARS(work)->counts[1])

/* a stretch of the course a hodai fires in: from start up to end, in path
 * points. each call of hodai_in_stretch moves start 0 to 119 points
 * earlier, at random. nothing reads number, unk_0c or unk_10 */
struct stretch {
    int number;
    int start;
    int end;
    int unk_0c;
    int unk_10;
};

#define STRETCHES 20

/* the work mode of a kirai, a hodai and a shell */
enum kirai_mode { KIRAI_INIT, KIRAI_RUN, KIRAI_DOWN, KIRAI_GONE, KIRAI_KILL };

#define KIRAI_POINTS 20
#define HODAI_POINTS 50
#define SHELL_POINTS 5

/* how near the camera a hodai takes hits, and how near it fires, both
 * squared */
#define HODAI_REACH (3000.0F * 3000.0F)
#define HODAI_RANGE (5000.0F * 5000.0F)

#define KIRAI_BURST_STEPS 10 /* a burst kirai's steps before it goes */
#define KIRAI_FRAMES 30.0F /* the frames kirai_action loops through */

#define SHELL_SPEED 30.0F /* per step */
#define SHELL_STEPS 120

extern NJS_TEXLIST shooting0_texlist;
extern NJS_ACTION kirai_action;
extern NJS_OBJECT hodai_object;
extern NJS_ACTION hodai_shell_action;

static struct collision_shape kirai_shape = {SHAPE_SPHERE | SHAPE_HURTABLE,
                                             ATTACK(1) | SHAPE_HURTS_PLAYERS,
                                             PUSH_ALL,
                                             {0.0F, 7.0F, 0.0F},
                                             60.0F,
                                             0.0F,
                                             0.0F,
                                             {0},
                                             0,
                                             NULL};
/* nothing reads it */
static int dat_0c9561c4 = 0;
static struct stretch stretches[STRETCHES] = {
#include "shooting/kirai/stretches.inc"
};
static struct collision_shape hodai_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     20.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct collision_shape hodai_shell_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE,
    ATTACK(1) | SHAPE_HURTS_PLAYERS,
    PUSH_ALL,
    {0.0F, 0.0F, 0.0F},
    30.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL};

static void kirai_draw(struct task *task);
static void kirai_destroy(struct task *task);
static void kirai_take_touch(struct task *task);
static void kirai_animate(struct task_work *work);
static void hodai_draw(struct task *task);
static void hodai_destroy(struct task *task);
static void hodai_take_hit(struct task *task);
static void hodai_fire(struct task *task);
static Bool hodai_in_stretch(struct task_work *work);
static void hodai_shell_draw(struct task *task);
static void hodai_shell_step(struct task *task);
static void hodai_shell_aim(struct task_work *work);
static void hodai_shell_take_hit(struct task *task);
static void hodai_shell_destroy(struct task *task);

/* the kirai stops and hides while an event or the score plays. it goes once
 * the course is flown */
void kirai_step(struct task *task)
{
    struct task_work *work = task->work;

    if (shooting_event_playing()) {
        return;
    }
    if (shooting_stage_mode == SHOOTING_CLEAR) {
        work->mode = KIRAI_KILL;
    }
    switch (work->mode) {
    case KIRAI_INIT:
        work->mode = KIRAI_RUN;
        VARS(work)->frame = 0.0F;
        task->destructor = kirai_destroy;
        task->draw = kirai_draw;
        enemy_alloc_record(task, work);
        collision_init(task, &kirai_shape, COLLISION_ITEM, 1, 1);
        break;
    case KIRAI_RUN:
        kirai_draw(task);
        kirai_take_touch(task);
        kirai_animate(work);
        collision_register(work);
        break;
    case KIRAI_GONE:
        if (--VARS(work)->wait == 0) {
            work->mode = KIRAI_KILL;
        }
        break;
    case KIRAI_KILL:
        placement_kill(task);
        break;
    }
}

/* the kirai at twice its size. it draws at a frame's first step only, and
 * not while an event or the score plays */
static void kirai_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step != 0) {
        return;
    }
    if (shooting_event_playing()) {
        return;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njSetTexture(&shooting0_texlist);
    njScale(NULL, 2.0F, 2.0F, 2.0F);
    draw_action(&kirai_action, VARS(work)->frame);
    njPopMatrix(1);
}

static void kirai_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}

/* anything that touches the kirai bursts it, and counts as a target
 * destroyed. a shot's touch also scores KIRAI_POINTS */
static void kirai_take_touch(struct task *task)
{
    struct task_work *work = task->work;

    if (work->collision->hit != NULL) {
        work->mode = KIRAI_GONE;
        VARS(work)->wait = KIRAI_BURST_STEPS;
        shooting_score_hit(KIRAI_POINTS, work);
        shooting_count_hit();
        bomb_create(&work->pos, 5.0F);
        sound_play(SE_SHOT_DOWN, 0, 0, 0);
    }
}

static void kirai_animate(struct task_work *work)
{
    VARS(work)->frame += 1.0F;
    if (VARS(work)->frame >= KIRAI_FRAMES) {
        VARS(work)->frame = 0.0F;
    }
}

/* the hodai stops and hides while an event or the score plays. it goes once
 * the course is flown. it fires in its stretch of the course, and takes hits
 * within HODAI_REACH of the camera */
void hodai_step(struct task *task)
{
    struct task_work *work = task->work;

    if (shooting_event_playing()) {
        return;
    }
    /* @hack spelling: a switch of one case, which makes KIRAI_GONE in R2 (MOV
     * #3,R2 at 0x0c905b5a); as an if, it takes R1, and 2 of 104 words
     * differ */
    switch (shooting_stage_mode) {
    case SHOOTING_CLEAR:
        work->mode = KIRAI_GONE;
        break;
    }
    switch (work->mode) {
    case KIRAI_INIT:
        task->destructor = hodai_destroy;
        task->draw = hodai_draw;
        enemy_alloc_record(task, work);
        collision_init(task, hodai_shapes, COLLISION_ITEM, 1, 1);
        STRETCH(work) = (int)work->scl.x;
        HITS(work) = 1;
        VARS(work)->wait = 0;
        VARS(work)->frame = 0.0F;
        work->mode = KIRAI_RUN;
        break;
    case KIRAI_RUN:
        hodai_draw(task);
        hodai_take_hit(task);
        if (hodai_in_stretch(work)) {
            hodai_fire(task);
            if (VARS(work)->frame <= 30.0F) {
                VARS(work)->frame += 1.0F;
            }
        }
        if (hodai_near(task, HODAI_REACH)) {
            collision_register(work);
        }
        break;
    case KIRAI_DOWN:
        break;
    case KIRAI_GONE:
        placement_kill(task);
    }
    task_step_children(task);
}

/* at a frame's first step only, and not while an event or the score
 * plays */
static void hodai_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step != 0) {
        return;
    }
    if (shooting_event_playing()) {
        return;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njSetTexture(&shooting0_texlist);
    draw_tree(&hodai_object);
    njPopMatrix(1);
}

static void hodai_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}

/* a hit scores HODAI_POINTS and counts as a target destroyed. the last
 * hit sends the hodai down */
static void hodai_take_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & COLLISION_DAMAGED) {
        HITS(work)--;
        lock_release(task);
        shooting_score_hit(HODAI_POINTS, work);
        shooting_count_hit();
        if (HITS(work) <= 0) {
            bomb_create(&work->pos, 6.0F);
            work->mode = KIRAI_DOWN;
        } else {
            bomb_create(&work->pos, 3.0F);
        }
    }
}

Bool hodai_near(struct task *task, Float reach)
{
    /* @hack spelling: both points walked through pointers, which reads them
     * with FMOV.S @R5+ and @R4+ (0x0c905d54); as reads of their fields, 27
     * of 27 words differ */
    const Float *pos = &task->work->pos.x;
    const Float *camera = &camera_work->pos.x;
    Float x;
    Float y;
    Float z;

    x = *pos - *camera;
    pos++;
    camera++;
    y = *pos - *camera;
    pos++;
    camera++;
    z = *pos - *camera;
    if ((x * x) + (y * y) + (z * z) > reach) {
        return FALSE;
    }
    return TRUE;
}

/* fire a shell once the wait runs out, then wait 120 to 179 steps. the
 * hodai waits and fires only while it is on the screen. it fires only
 * within HODAI_RANGE of the camera */
static void hodai_fire(struct task *task)
{
    struct task_work *work = task->work;

    if (draw_on_screen(work)) {
        if (VARS(work)->wait == 0) {
            if (hodai_near(task, HODAI_RANGE)) {
                sound_play(SE_HODAI, 0, 0, 0);
                task_create_child(TASK_WORK, hodai_shell_step, task);
                VARS(work)->wait = (int)(rand() * (60.0F * RAND_SCALE)) + 120;
            }
        } else {
            VARS(work)->wait--;
        }
    }
}

/* TRUE while the tornado's point is in the hodai's stretch, else FALSE. a
 * stretch past the table is never in */
static Bool hodai_in_stretch(struct task_work *work)
{
    if (STRETCH(work) < STRETCHES) {
        int start = stretches[STRETCH(work)].start;
        int end;

        start -= (int)(rand() * (120.0F * RAND_SCALE));
        end = stretches[STRETCH(work)].end;
        if (shooting_path_index >= start && shooting_path_index < end) {
            return TRUE;
        }
    }
    return FALSE;
}

/* the shell, mirrored in z, at a frame's first step only */
static void hodai_shell_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step != 0) {
        return;
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njScale(NULL, 1.0F, 1.0F, -1.0F);
    njSetTexture(&shooting0_texlist);
    draw_action(&hodai_shell_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* a shell the hodai fires at the path ahead of the tornado, which flies
 * for SHELL_STEPS. its scl holds its speed */
static void hodai_shell_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *hodai = task->parent->work;

    switch (work->mode) {
    case KIRAI_INIT:
        enemy_alloc_record(task, work);
        collision_init(task, &hodai_shell_shape, COLLISION_ENEMY, 1, 1);
        task->destructor = hodai_shell_destroy;
        task->draw = hodai_shell_draw;
        work->ang.x = work->ang.y = work->ang.z = 0;
        work->pos = hodai->pos;
        hodai_shell_aim(work);
        VARS(work)->wait = SHELL_STEPS;
        VARS(work)->frame = 0.0F;
        work->mode = KIRAI_RUN;
        break;
    case KIRAI_RUN:
        hodai_shell_draw(task);
        work->pos.x += work->scl.x;
        work->pos.y += work->scl.y;
        work->pos.z += work->scl.z;
        if (--VARS(work)->wait <= 0) {
            work->mode = KIRAI_GONE;
        }
        if (VARS(work)->frame >= 60.0F) {
            VARS(work)->frame = 0.0F;
        } else {
            VARS(work)->frame += 1.0F;
        }
        hodai_shell_take_hit(task);
        collision_register(work);
        break;
    case KIRAI_GONE:
        task_destroy(task);
        break;
    }
}

/* send the shell at SHELL_SPEED toward a path point up to 99 ahead of the
 * tornado's, facing it. nothing keeps the point within the path */
static void hodai_shell_aim(struct task_work *work)
{
    NJS_VECTOR way;
    Float x;
    Float y;
    Float z;
    Float across;
    Angle ang_y;
    Angle ang_x;
    int ahead = (int)(rand() * (100.0F * RAND_SCALE));

    x = shooting_path[shooting_path_index + ahead].x - work->pos.x;
    way.x = x;
    way.y = y = shooting_path[shooting_path_index + ahead].y - work->pos.y;
    way.z = z = shooting_path[shooting_path_index + ahead].z - work->pos.z;
    njUnitVector(&way);
    work->scl.x = way.x * SHELL_SPEED;
    work->scl.y = way.y * SHELL_SPEED;
    work->scl.z = way.z * SHELL_SPEED;
    across = njSqrt((x * x) + (z * z));
    ang_y = 0x4000 - (Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
    ang_x = -(Angle)(atan2f(y, across) * ANGLE_PER_RADIAN);
    work->ang.x = ang_x;
    work->ang.y = ang_y;
}

/* a hit bursts the shell, for SHELL_POINTS */
static void hodai_shell_take_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & COLLISION_DAMAGED) {
        lock_release(task);
        shooting_score_hit(SHELL_POINTS, work);
        work->mode = KIRAI_GONE;
        bomb_create(&work->pos, 3.0F);
        sound_play(SE_HIT, 0, 0, 0);
    }
}

static void hodai_shell_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}

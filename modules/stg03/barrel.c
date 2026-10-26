/* O BARREL powder keg whose fuse player 0 lights */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "effect/spark.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg03/barrel.h"
#include "stg03/mirror.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT barrel_object;
extern NJS_OBJECT barrel_piece_1_object[];
extern NJS_OBJECT barrel_piece_2_object[];
extern NJS_OBJECT barrel_piece_3_object[];
extern NJS_OBJECT barrel_piece_4_object[];
extern NJS_OBJECT barrel_piece_5_object[];

#define SE_FUSE 939
#define SE_EXPLOSION 25

#define BARREL_SCALE 3.0F

/* player 0 lights the fuse within this distance of the barrel */
#define BARREL_NEAR 30.0F
/* the fuse burns this many steps plus the set entry's scale x, rounded down.
 * the barrel blows on the second step after that */
#define BARREL_FUSE 600
/* the blast's radius grows 1 a step from its shape's 5, this much in all */
#define BARREL_BLAST 30.0F

/* a piece lives while the camera or player 0 is within 310 of it, as
 * placement_try_leave_by measures. the range is squared */
#define PIECE_RANGE (310.0F * 310.0F)

struct barrel_vars {
    NJS_OBJECT *object; /* the barrel's or a piece's model */
    int fuse; /* the fuse's length, in steps */
};

#define VARS(work) ((struct barrel_vars *)&(work)->vars)
/* work->scl.x holds the set entry's steps to add to the fuse, until the first
 * step adds them. it then holds how far the blast has grown */
#define BARREL_GROWN(work) ((work)->scl.x)
#define PIECE_SPEED(work) ((work)->scl)

/* the modes, as the image numbers them */
enum barrel_mode {
    BARREL_INIT = 0,
    BARREL_WAITING = 1,
    BARREL_LIT = 2,
    BARREL_BLOWN = 4,
    BARREL_GONE = 5
};

enum piece_mode { PIECE_INIT, PIECE_FLYING };

/* the barrel's cylinder, then the blast's sphere, which the first step turns
 * off until the barrel blows */
static struct collision_shape barrel_shapes[2] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 9.0F, 0.0F},
     7.5F,
     8.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS |
         SHAPE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     5.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static void barrel_piece_step(struct task *task);

static struct task_spec barrel_pieces[6] = {
    {barrel_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)barrel_piece_1_object},
    {barrel_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)barrel_piece_2_object},
    {barrel_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)barrel_piece_3_object},
    {barrel_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)barrel_piece_4_object},
    {barrel_piece_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)barrel_piece_5_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

static void barrel_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, BARREL_SCALE, BARREL_SCALE, BARREL_SCALE);
        draw_tree(VARS(work)->object);
        njPopMatrix(1);
    }
}

static void barrel_blast(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *blast = &work->collision->shapes[1];

    if (BARREL_GROWN(work) < BARREL_BLAST) {
        BARREL_GROWN(work) += 1.0F;
        blast->radius += 1.0F;
    } else {
        work->mode = BARREL_GONE;
    }
}

static void barrel_destroy(struct task *task)
{
    task_kill(task);
}

/* unlike barrel_draw, a piece's draw also turns about x. it draws on every
 * step, not only on a frame's first */
static void barrel_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, BARREL_SCALE, BARREL_SCALE, BARREL_SCALE);
    draw_tree(VARS(work)->object);
    njPopMatrix(1);
}

static void barrel_piece_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, PIECE_RANGE)) {
        return;
    }
    switch (work->mode) {
    case PIECE_INIT:
        work->mode = PIECE_FLYING;
        task->destructor = barrel_destroy;
        task->draw = barrel_piece_draw;
        PIECE_SPEED(work).x = (rand() * RAND_SCALE) - 0.5F;
        PIECE_SPEED(work).y = (rand() * RAND_SCALE) + 0.5F;
        PIECE_SPEED(work).z = (rand() * RAND_SCALE) - 0.5F;
        twinkle_reflect(task);
        break;
    case PIECE_FLYING:
        work->pos.x += PIECE_SPEED(work).x;
        work->pos.y += PIECE_SPEED(work).y;
        work->pos.z += PIECE_SPEED(work).z;
        work->ang.x += 0x80;
        work->ang.y += 0x100;
        PIECE_SPEED(work).y -= 0.05F;
        barrel_piece_draw(task);
        task_step_children(task);
        break;
    }
}

static void barrel_fuse(struct task *task)
{
    struct task_work *work = task->work;

    if (TASK_TIMER(work)++ > VARS(work)->fuse) {
        work->mode = BARREL_BLOWN;
        work->collision->shapes[0].flags |= SHAPE_OFF;
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        work->collision->radius = 30.0F;
        bomb_create(&work->pos, 1.0F);
        /* the reflection twinkle_reflect made, the barrel's one child until
         * the pieces */
        task_kill(task->child);
        task_create_children(barrel_pieces, task);
        sound_stop(SE_FUSE);
        sound_play_at(SE_EXPLOSION, 0, 0, 0, work->pos.x, work->pos.y,
                      work->pos.z);
    }
}

static void barrel_spark(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_VECTOR speed;

    pos = work->pos;
    pos.y += 20.0F;
    speed.x = ((rand() * RAND_SCALE) - 0.5F) * 1.2F;
    speed.y = ((rand() * RAND_SCALE) + 0.2F) * 0.9F;
    speed.z = ((rand() * RAND_SCALE) - 0.5F) * 1.2F;
    spark_create(&pos, &speed, 1.0F);
}

void barrel_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, BARREL_SCALE, BARREL_SCALE, BARREL_SCALE);
        draw_tree(&barrel_object);
        njPopMatrix(1);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> FUSE LENGTH +");
            njPrintD(NJM_LOCATION(34, EDITOR_ROW_SCL_X), BARREL_FUSE, 3);
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), ">");
        }
    } else {
        switch (work->mode) {
        case BARREL_INIT:
            work->mode = BARREL_WAITING;
            VARS(work)->fuse = (int)floorf(BARREL_GROWN(work)) + BARREL_FUSE;
            BARREL_GROWN(work) = 0.0F;
            VARS(work)->object = &barrel_object;
            collision_init(task, barrel_shapes, COLLISION_OBJECT, 1, 2);
            work->collision->shapes[1].flags |= SHAPE_OFF;
            work->collision->radius = 15.0F;
            task->destructor = barrel_destroy;
            task->draw = barrel_draw;
            twinkle_reflect(task);
            break;
        case BARREL_WAITING:
            if (njDistanceP2P(&work->pos, &player_work[0]->pos) < BARREL_NEAR) {
                work->mode = BARREL_LIT;
                sound_play_held(SE_FUSE, -1, 8, 0);
            }
            collision_register(work);
            barrel_draw(task);
            task_step_children(task);
            break;
        case BARREL_LIT:
            barrel_fuse(task);
            barrel_spark(task);
            barrel_draw(task);
            task_step_children(task);
            collision_register(work);
            break;
        case BARREL_BLOWN:
            task_step_children(task);
            collision_register(work);
            barrel_blast(task);
            break;
        case BARREL_GONE:
            placement_kill(task);
            break;
        }
    }
}

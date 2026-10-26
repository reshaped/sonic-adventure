/* the Egg Walker's bombs: five lobbed from its body as it walks, or ten
 * scattered as it jumps. each bounces until it comes to rest or something
 * hurts it, and blows up */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm2/blast.h"
#include "b_egm2/bombs.h"
#include "b_egm2/egg_walker.h"
#include "b_egm2/parts.h"
#include "b_egm2/texlists.h"
#include "collision/collision.h"
#include "draw/motion.h"
#include "draw/state.h"
#include "draw/tree.h"
#include "enemy/enemy.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the bomb's record, which its task's extra points to */
struct bomb {
    NJS_VECTOR speed;
    Sint32 kind; /* BOMB_LOBBED or BOMB_SCATTER */
};

/* work->vars of a bomb: the steps it has left to rest */
struct bomb_vars {
    Uint8 unk_00[4];
    Sint32 rest;
};

#define BOMB_VARS(work) ((struct bomb_vars *)&(work)->vars)

/* the boss's submodes in EGG_WALKER_BOMBS */
enum bombs_mode { BOMBS_START, BOMBS_LOB, BOMBS_END };
enum bomb_mode { BOMB_START, BOMB_BOUNCE, BOMB_REST, BOMB_GONE };

/* the boss lobs BOMBS bombs from its body. their heading starts at
 * BOMBS_HEADING and loses BOMBS_TURN with each bomb. BOMBS_SPEED is 0, so
 * each drops straight down whatever its heading */
#define BOMBS 5
#define BOMBS_HEADING 0x1c70
#define BOMBS_TURN 0xb6
#define BOMBS_SPEED 0.0F

/* a bomb drops as the boss's timer passes below 0. the timer is set to
 * BOMBS_DELAY before the first bomb. after each, it is set to BOMBS_GAP
 * over the boss's pace */
#define BOMBS_DELAY 120
#define BOMBS_GAP (60.0F * 0.6F)

/* each step adds BOMB_FALL to a bomb's upward speed. each bounce keeps
 * BOMB_BOUNCE_LOBBED or BOMB_BOUNCE_SCATTER of the bomb's speed, by its
 * kind. once a bounce leaves the bomb rising slower than BOMB_STILL, the
 * bomb marks its own shape hurt. bomb_blast_when_hurt then blows it up on that
 * step, so the BOMB_REST_TIME steps of rest it sets never run */
#define BOMB_FALL (-9.8F / (60.0F * 6.0F))
#define BOMB_BOUNCE_LOBBED 0.6F
#define BOMB_BOUNCE_SCATTER 0.2F
#define BOMB_STILL 0.5F
#define BOMB_REST_TIME 600

#define SE_BOMB_RELEASE 0x1e4 /* as each bomb is made, lobbed or scattered */
#define SE_BOMB_BLAST 0x1e5

/* a bomb's blast: its grow_end, fade_start, end and size. pad 0 shakes at
 * BLAST_POWER and BLAST_FREQ as it blows up */
#define BLAST_GROWN 30
#define BLAST_FADE 60
#define BLAST_END 70
#define BLAST_SIZE 35.0F
#define BLAST_POWER 3
#define BLAST_FREQ 30

static void bomb_run(struct task *task);
static void bomb_blast_when_hurt(struct task *task);
static void bomb_draw(struct task *task);
static void bomb_destroy(struct task *task);

static NJS_MATERIAL bomb_fuse_materials[1] = {
#include "b_egm2/bombs/bomb_fuse_materials.inc"
};

static Sint16 bomb_fuse_meshes0[4] = {
#include "b_egm2/bombs/bomb_fuse_meshes0.inc"
};

static Sint16 bomb_fuse_meshes1[24] = {
#include "b_egm2/bombs/bomb_fuse_meshes1.inc"
};

static NJS_COLOR bomb_fuse_uvs0[3] = {
#include "b_egm2/bombs/bomb_fuse_uvs0.inc"
};

static NJS_COLOR bomb_fuse_uvs1[24] = {
#include "b_egm2/bombs/bomb_fuse_uvs1.inc"
};

static NJS_MESHSET bomb_fuse_meshsets[2] = {
#include "b_egm2/bombs/bomb_fuse_meshsets.inc"
};

static NJS_POINT3 bomb_fuse_points[9] = {
#include "b_egm2/bombs/bomb_fuse_points.inc"
};

static NJS_VECTOR bomb_fuse_normals[9] = {
#include "b_egm2/bombs/bomb_fuse_normals.inc"
};

static NJS_MODEL bomb_fuse_model = {
#include "b_egm2/bombs/bomb_fuse_model.inc"
};

static NJS_OBJECT bomb_fuse_object = {
#include "b_egm2/bombs/bomb_fuse_object.inc"
};

static NJS_MATERIAL bomb_materials[1] = {
#include "b_egm2/bombs/bomb_materials.inc"
};

static Sint16 bomb_meshes0[34] = {
#include "b_egm2/bombs/bomb_meshes0.inc"
};

static Sint16 bomb_meshes1[64] = {
#include "b_egm2/bombs/bomb_meshes1.inc"
};

static NJS_COLOR bomb_uvs0[28] = {
#include "b_egm2/bombs/bomb_uvs0.inc"
};

static NJS_COLOR bomb_uvs1[64] = {
#include "b_egm2/bombs/bomb_uvs1.inc"
};

static NJS_MESHSET bomb_meshsets[2] = {
#include "b_egm2/bombs/bomb_meshsets.inc"
};

static NJS_POINT3 bomb_points[29] = {
#include "b_egm2/bombs/bomb_points.inc"
};

static NJS_VECTOR bomb_normals[29] = {
#include "b_egm2/bombs/bomb_normals.inc"
};

static NJS_MODEL bomb_model = {
#include "b_egm2/bombs/bomb_model.inc"
};

static NJS_OBJECT bomb_object = {
#include "b_egm2/bombs/bomb_object.inc"
};

static struct collision_shape bomb_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     3.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static Angle bombs_heading;

void egg_walker_bombs(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_walker *boss = task->extra;
    NJS_VECTOR speed;

    switch (work->submode) {
    case BOMBS_START:
        EGG_WALKER_VARS(work)->timer = BOMBS_DELAY;
        EGG_WALKER_VARS(work)->count = BOMBS;
        bombs_heading = BOMBS_HEADING;
        work->submode = BOMBS_LOB;
    case BOMBS_LOB:
        if (--EGG_WALKER_VARS(work)->timer < 0) {
            --EGG_WALKER_VARS(work)->count;
            speed.y = 0.0F;
            speed.x = njSin(bombs_heading) * BOMBS_SPEED;
            speed.z = njCos(bombs_heading) * BOMBS_SPEED;
            egg_walker_bomb_create(egg_walker_part_pos(0), &speed, BOMB_LOBBED);
            bombs_heading -= BOMBS_TURN;
            EGG_WALKER_VARS(work)->timer = (int)(BOMBS_GAP / boss->pace);
            if (EGG_WALKER_VARS(work)->count == 0 ||
                boss->walk == WALK_STOPPED) {
                work->submode = BOMBS_END;
            }
        }
        break;
    case BOMBS_END:
        work->submode = MODE_DONE;
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != EGG_WALKER_WALK) {
            boss_next_mode = EGG_WALKER_WALK;
            boss_last_mode = work->mode;
        }
        work->submode = BOMBS_START;
    }
}

void egg_walker_bomb_create(NJS_POINT3 *pos, NJS_VECTOR *speed, int kind)
{
    struct task *task;
    struct task_work *work;
    struct bomb *bomb;

    /* @hack spelling: the task stored as it's tested, which tests the result
     * in R0 and copies it to R14 in the branch's slot (TST R0,R0 at
     * 0x0c901de8, MOV R0,R14 at 0x0c901dec); as two statements, SHC tests
     * R14 and 3 of 84 words differ */
    if ((task = task_create(TASK_WORK, 3, bomb_run)) != NULL) {
        work = task->work;
        bomb = heap_calloc(1, sizeof(struct bomb));
        if (bomb == NULL) {
            task_kill(task);
            return;
        }
        task->extra = bomb;
        enemy_alloc_record(task, work);
        work->pos = *pos;
        bomb->speed = *speed;
        bomb->kind = kind;
        task->step = bomb_run;
        task->draw = bomb_draw;
        task->destructor = bomb_destroy;
        collision_init(task, bomb_shape, COLLISION_OBJECT, 1, 1);
        sound_play(SE_BOMB_RELEASE, 0, 0, 0);
    }
}

static void bomb_run(struct task *task)
{
    struct task_work *work = task->work;
    struct bomb *bomb = task->extra;
    struct collision_shape *shape;

    switch (work->mode) {
    case BOMB_START:
        work->mode = BOMB_BOUNCE;
    case BOMB_BOUNCE:
        VECTOR_SUM(&work->pos, &work->pos, &bomb->speed);
        bomb->speed.y += BOMB_FALL;
        if (work->pos.y < 0.0F) {
            work->pos.y = 0.0F;
            if (bomb->kind != BOMB_LOBBED) {
                VECTOR_SCALE(&bomb->speed, &bomb->speed, BOMB_BOUNCE_SCATTER);
            } else {
                VECTOR_SCALE(&bomb->speed, &bomb->speed, BOMB_BOUNCE_LOBBED);
            }
            bomb->speed.y = njAbs(bomb->speed.y);
            if (bomb->speed.y < BOMB_STILL) {
                BOMB_VARS(work)->rest = BOMB_REST_TIME;
                shape = work->collision->shapes;
                shape->levels |= SHAPE_HURT;
                work->mode = BOMB_REST;
            }
        }
        break;
    case BOMB_REST:
        if (--BOMB_VARS(work)->rest < 0) {
            work->mode = BOMB_GONE;
        }
        break;
    case BOMB_GONE:
        task_kill(task);
        break;
    }
    bomb_blast_when_hurt(task);
    collision_register(work);
    bomb_draw(task);
}

/* once the bomb's shape is hurt, blow the bomb up and end its task */
static void bomb_blast_when_hurt(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_shape *shape = work->collision->shapes;
    struct blast_params params;

    if (shape->levels & SHAPE_HURT) {
        params.pos = &work->pos;
        params.flags = BLAST_COPY_POS | BLAST_HURTS;
        params.beam_start = 0;
        params.beam_end = 0;
        params.beam_length = 0.0F;
        params.beam_speed = 0;
        params.grow_start = 0;
        params.grow_end = BLAST_GROWN;
        params.fade_start = BLAST_FADE;
        params.end = BLAST_END;
        params.size = BLAST_SIZE;
        egg_walker_blast_create(&params);
        sound_play(SE_BOMB_BLAST, 0, 0, 0);
        vibration_start(0, BLAST_POWER, BLAST_FREQ, 1);
        task_kill(task);
    }
}

static void bomb_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njSetTexture(&egm2_common_texlist);
        draw_state_save();
        draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_tree(&bomb_object);
        njPopMatrix(1);
        draw_state_restore();
    }
}

static void bomb_destroy(struct task *task)
{
}

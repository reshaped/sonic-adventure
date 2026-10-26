/* the shock wave Zero makes as it lands from a leap */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_robo/banks.h"
#include "b_robo/shock_wave.h"
#include "collision/collision.h"
#include "draw/tree.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the ring's object, as an offset into the model bank */
#define RING_OBJECT 0x13f0
/* the first of the ring's two textures in erobo_textures, which take turns
 * every two steps */
#define RING_TEXTURE 34
/* the ring's scale in x and z for each unit of its radius */
#define RING_SCALE 0.2F

/* the ring's center lies AHEAD along the negated cosine and sine of its
 * maker's ang.y. Zero faces that way. the center's height is RING_Y, 0.09
 * below DECK_FLOOR_Y */
#define AHEAD 15.0F
#define RING_Y 752.0F
/* the ring's turn about y each step */
#define SPIN 0x800

/* the ring's radius starts at 1, and grows by GROWTH on each step before
 * the shock wave's STEPS-th. that step kills its task */
#define GROWTH 2.82857144F
#define STEPS 35

/* the shock wave's work->vars. work->scl.x is the ring's radius */
struct shock_wave_vars {
    Uint32 steps; /* the steps it has run, this one included */
};

#define VARS(work) ((struct shock_wave_vars *)&(work)->vars)

/* a sphere of radius GROWTH, which shock_wave_step moves to the ring's
 * edge. it hurts players. its guard is the highest, so no shape can hurt
 * it. PUSH_FIRM keeps any shape from pushing it */
static struct collision_shape shock_wave_shape[1] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_GUARD | SHAPE_ATTACK,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     GROWTH,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static void shock_wave_step(struct task *task);
static void shock_wave_draw(struct task *task);

void erobo_shock_wave_create(struct task_work *work)
{
    struct task *task;
    struct task_work *wave;
    Float sine;
    Float cosine;

    if ((task = task_create(TASK_WORK, 6, shock_wave_step)) != NULL) {
        wave = task->work;
        _builtin_fsca(work->ang.y, &sine, &cosine);
        wave->pos.x = work->pos.x - (cosine * AHEAD);
        wave->pos.z = work->pos.z - (sine * AHEAD);
        wave->pos.y = RING_Y;
        wave->scl.x = 1.0F;
        wave->scl.y = GROWTH;
        collision_init(task, shock_wave_shape, COLLISION_ITEM, 1, 1);
        /* @hack spelling: the record through task->work, which the image loads
         * again after collision_init (MOV.L @(32,R14),R3 at 0x0c908cd2);
         * through wave, 65 of 70 words differ */
        task->work->collision->radius = 1000.0F;
        task->draw = shock_wave_draw;
    }
}

static void shock_wave_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *player;
    NJS_VECTOR center;

    if (++VARS(work)->steps >= STEPS) {
        task_kill(task);
        return;
    }
    work->scl.x += GROWTH;
    work->ang.y += SPIN;
    player = &player_work[0]->pos;
    center.x = player->x - work->pos.x;
    center.y = 0.0F;
    center.z = player->z - work->pos.z;
    njUnitVector(&center);
    center.x *= work->scl.x;
    center.z *= work->scl.x;
    center.x += work->pos.x;
    center.z += work->pos.z;
    center.y = work->pos.y;
    work->collision->shapes->center = center;
    collision_register(work);
    shock_wave_draw(task);
}

static void shock_wave_draw(struct task *task)
{
    struct task_work *work;
    NJS_VECTOR scale;

    if (frame_step) {
        return;
    }
    work = task->work;
    ((NJS_OBJECT *)(erobo_models + RING_OBJECT))->model->mats->attr_texId =
        ((VARS(work)->steps >> 1) & 1) + RING_TEXTURE;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    scale.x = work->scl.x * RING_SCALE;
    scale.y = 1.0F;
    scale.z = scale.x;
    njScaleV(NULL, &scale);
    njSetTexture(erobo_textures);
    draw_tree((NJS_OBJECT *)(erobo_models + RING_OBJECT));
    njPopMatrix(1);
}

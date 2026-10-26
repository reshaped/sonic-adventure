/* the furniture: TV1, ISU1, ISU2, TANSU, KABIN, KAIGA, TV2, REIZOU, SOFA,
 * TANA and YAKAN, things from a house that hang where they are placed. a
 * hit burns one up in a beam of light */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/tree.h"
#include "effect/beam.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "shooting/furniture.h"
#include "shooting/hud.h"
#include "system/task.h"

struct furniture_model {
    NJS_OBJECT *object;
    NJS_TEXLIST *texlist; /* never read: the draw sets shooting0_texlist */
};

/* the model a thing draws, which FURNITURE keeps in the work's timer */
enum furniture {
    FURNITURE_TV1,
    FURNITURE_ISU1,
    FURNITURE_ISU2,
    FURNITURE_TANSU,
    FURNITURE_KABIN,
    FURNITURE_KAIGA,
    FURNITURE_TV2,
    FURNITURE_REIZOU,
    FURNITURE_SOFA,
    FURNITURE_TANA,
    FURNITURE_YAKAN,
    FURNITURE_COUNT
};

#define FURNITURE(work) TASK_TIMER(work)

struct furniture_vars {
    Uint8 unk_00[8];
    int burning; /* steps left to burn */
};

#define VARS(work) ((struct furniture_vars *)&(work)->vars)

enum furniture_mode {
    FURNITURE_INIT = 0,
    FURNITURE_RUN = 1,
    FURNITURE_BURN = 3,
    FURNITURE_GONE = 4
};

#define FURNITURE_POINTS 5
#define FURNITURE_BURN_STEPS 20

extern NJS_TEXLIST shooting0_texlist;
extern NJS_OBJECT furniture_tv1_object;
extern NJS_OBJECT furniture_isu1_object;
extern NJS_OBJECT furniture_isu2_object;
extern NJS_OBJECT furniture_tansu_object;
extern NJS_OBJECT furniture_kabin_object;
extern NJS_OBJECT furniture_kaiga_object;
extern NJS_OBJECT furniture_tv2_object;
extern NJS_OBJECT furniture_reizou_object;
extern NJS_OBJECT furniture_sofa_object;
extern NJS_OBJECT furniture_tana_object;
extern NJS_OBJECT furniture_yakan_object;

static void furniture_step(struct task *task);
static void furniture_take_hit(struct task *task);
static void furniture_burn(struct task *task);
static void furniture_draw(struct task *task);
static void furniture_destroy(struct task *task);

static struct furniture_model furniture_models[FURNITURE_COUNT] = {
    {&furniture_tv1_object, &shooting0_texlist},
    {&furniture_isu1_object, &shooting0_texlist},
    {&furniture_isu2_object, &shooting0_texlist},
    {&furniture_tansu_object, &shooting0_texlist},
    {&furniture_kabin_object, &shooting0_texlist},
    {&furniture_kaiga_object, &shooting0_texlist},
    {&furniture_tv2_object, &shooting0_texlist},
    {&furniture_reizou_object, &shooting0_texlist},
    {&furniture_sofa_object, &shooting0_texlist},
    {&furniture_tana_object, &shooting0_texlist},
    {&furniture_yakan_object, &shooting0_texlist},
};
static struct collision_shape furniture_shapes[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1) | GUARD(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL,
     {0.0F, 7.0F, 0.0F},
     15.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static struct beam_params furniture_beam = {
    180, {1.0F, 3.0F}, {0xffffff00, 0xffffffff}, 0x800, 20.0F, 1.0F};

/* the object list's steps, one for each model. each keeps its model in the
 * work, and hands the task to furniture_step */
void furniture_tv1_step(struct task *task)
{
    FURNITURE(task->work) = FURNITURE_TV1;
    task->step = furniture_step;
    furniture_step(task);
}

void furniture_isu1_step(struct task *task)
{
    FURNITURE(task->work) = FURNITURE_ISU1;
    task->step = furniture_step;
    furniture_step(task);
}

void furniture_isu2_step(struct task *task)
{
    FURNITURE(task->work) = FURNITURE_ISU2;
    task->step = furniture_step;
    furniture_step(task);
}

void furniture_tansu_step(struct task *task)
{
    FURNITURE(task->work) = FURNITURE_TANSU;
    task->step = furniture_step;
    furniture_step(task);
}

void furniture_kabin_step(struct task *task)
{
    FURNITURE(task->work) = FURNITURE_KABIN;
    task->step = furniture_step;
    furniture_step(task);
}

void furniture_kaiga_step(struct task *task)
{
    FURNITURE(task->work) = FURNITURE_KAIGA;
    task->step = furniture_step;
    furniture_step(task);
}

void furniture_tv2_step(struct task *task)
{
    FURNITURE(task->work) = FURNITURE_TV2;
    task->step = furniture_step;
    furniture_step(task);
}

void furniture_reizou_step(struct task *task)
{
    FURNITURE(task->work) = FURNITURE_REIZOU;
    task->step = furniture_step;
    furniture_step(task);
}

void furniture_sofa_step(struct task *task)
{
    FURNITURE(task->work) = FURNITURE_SOFA;
    task->step = furniture_step;
    furniture_step(task);
}

void furniture_tana_step(struct task *task)
{
    FURNITURE(task->work) = FURNITURE_TANA;
    task->step = furniture_step;
    furniture_step(task);
}

void furniture_yakan_step(struct task *task)
{
    FURNITURE(task->work) = FURNITURE_YAKAN;
    task->step = furniture_step;
    furniture_step(task);
}

static void furniture_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case FURNITURE_INIT:
        work->mode = FURNITURE_RUN;
        task->destructor = furniture_destroy;
        task->draw = furniture_draw;
        enemy_alloc_record(task, work);
        collision_init(task, furniture_shapes, COLLISION_ENEMY, 1, 1);
        break;
    case FURNITURE_RUN:
        furniture_draw(task);
        furniture_take_hit(task);
        collision_register(work);
        break;
    case FURNITURE_BURN:
        furniture_draw(task);
        furniture_burn(task);
        break;
    case FURNITURE_GONE:
        placement_kill(task);
        break;
    }
}

/* a hit scores FURNITURE_POINTS, counts as a target destroyed and burns
 * the thing up for FURNITURE_BURN_STEPS */
static void furniture_take_hit(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & COLLISION_DAMAGED) {
        shooting_score_hit(FURNITURE_POINTS, work);
        shooting_count_hit();
        work->mode = FURNITURE_BURN;
        VARS(work)->burning = FURNITURE_BURN_STEPS;
        beam_create(&work->pos, &furniture_beam);
    }
}

static void furniture_burn(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack spelling: an empty branch for a count above 0, which makes
     * FURNITURE_GONE in R2 (MOV #4,R2 at 0x0c905820); as an if of 0 or
     * less, it takes R3, and 2 of 10 words differ */
    if (VARS(work)->burning-- > 0) {
    } else {
        work->mode = FURNITURE_GONE;
    }
}

static void furniture_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njSetTexture(&shooting0_texlist);
    draw_tree(furniture_models[FURNITURE(work)].object);
    njPopMatrix(1);
}

static void furniture_destroy(struct task *task)
{
    enemy_end(task, task->work, (struct lockon_target *)task->motion);
}

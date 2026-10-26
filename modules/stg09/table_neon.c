/* Neon, whose three models trade places, and Neonkabe, a wall of six lamps
 * that blink and shine */
#include <ninja.h>

#include "draw/model.h"
#include "stg09/casino.h"
#include "stg09/table_neon.h"
#include "system/task.h"

/* set in Neonkabe's work->flags by its step. its draw, which runs while the
 * game is paused, clears it. the lamps' ticks advance only while it is set */
#define NEON_ACTIVE 0x100

/* Neon's models trade places every TRADE_STEPS steps */
#define TRADE_STEPS 6

/* a lamp's tick counts its steps round 0 to 255. while the tick is below
 * BLINK_STEPS, the lamp blinks. it shows its first model while the tick mod
 * BLINK_CYCLE is above BLINK_SPLIT, and its second otherwise. after that it
 * shines with its third */
#define BLINK_STEPS 100
#define BLINK_CYCLE 25
#define BLINK_SPLIT 10

/* every fourth step, the lamps' lit models move their UVs this far along v */
#define SCROLL_V 32

/* Neon's three models in their places, or a lamp's two blinking models and
 * its lit one */
/* @hack type: three fields, not an array of three, which table_neon_run
 * loads by their offsets, as MOV.L @(8,R5),R6 at 0x0c9067ee. as models[3],
 * it forms the array's address with ADD #8 twice, runs 8 bytes long and 11
 * of 17 words differ */
struct table_neon_vars {
    NJS_MODEL *first;
    NJS_MODEL *second;
    NJS_MODEL *third;
};
#define VARS(work) ((struct table_neon_vars *)&(work)->vars)

extern NJS_MODEL table_neon_model_0;
extern NJS_MODEL table_neon_model_1;
extern NJS_MODEL table_neon_model_2;
extern NJS_MODEL table_neon_lamp_0_model_0;
extern NJS_MODEL table_neon_lamp_0_model_1;
extern NJS_MODEL table_neon_lamp_0_model_2;
extern NJS_MODEL table_neon_lamp_1_model_0;
extern NJS_MODEL table_neon_lamp_1_model_1;
extern NJS_MODEL table_neon_lamp_1_model_2;
extern NJS_MODEL table_neon_lamp_2_model_0;
extern NJS_MODEL table_neon_lamp_2_model_1;
extern NJS_MODEL table_neon_lamp_2_model_2;
extern NJS_MODEL table_neon_lamp_3_model_0;
extern NJS_MODEL table_neon_lamp_3_model_1;
extern NJS_MODEL table_neon_lamp_3_model_2;
extern NJS_MODEL table_neon_lamp_4_model_0;
extern NJS_MODEL table_neon_lamp_4_model_1;
extern NJS_MODEL table_neon_lamp_4_model_2;
extern NJS_MODEL table_neon_lamp_5_model_0;
extern NJS_MODEL table_neon_lamp_5_model_1;
extern NJS_MODEL table_neon_lamp_5_model_2;

static void table_neon_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void table_neon_draw(struct task *task)
{
    struct task_work *work = task->work;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njPushMatrix(NULL);
    njTranslate(NULL, -9.8377F, 0.0F, -2.053F);
    njRotateY(NULL, 0x06c1);
    draw_model(VARS(work)->first);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 9.2365F, 0.0F, 3.961F);
    njRotateY(NULL, 0xe05b);
    draw_model(VARS(work)->second);
    njPopMatrix(1);
    njRotateY(NULL, -0x0c71);
    draw_model(VARS(work)->third);
    njPopMatrix(1);
}

static void table_neon_wall_draw(struct task *task)
{
    struct task_work *work = task->work;

    work->flags &= ~NEON_ACTIVE;
    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    task_step_children(task);
    /* @hack redundant: the return, which makes SHC open the frame in the
     * prologue with ADD #-4,R15 and store task with MOV.L R4,@R15 (0x0c906772,
     * 0x0c906774). without it, SHC pushes task with MOV.L R4,@-R15, the
     * function runs 2 bytes short and 14 of 16 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

static void table_neon_run(struct task *task)
{
    struct task_work *work = task->work;

    if (++work->tick == TRADE_STEPS) {
        NJS_MODEL *first = VARS(work)->first;
        work->tick = 0;
        VARS(work)->first = VARS(work)->second;
        VARS(work)->second = VARS(work)->third;
        VARS(work)->third = first;
    }
    table_neon_draw(task);
}

static void table_neon_lamp_run(struct task *task)
{
    struct task_work *work = task->work;

    if (task->parent->work->flags & NEON_ACTIVE) {
        work->tick++;
    }
    if (work->tick < BLINK_STEPS) {
        if (work->tick % BLINK_CYCLE > BLINK_SPLIT) {
            draw_model(VARS(work)->first);
        } else {
            draw_model(VARS(work)->second);
        }
    } else {
        draw_model(VARS(work)->third);
    }
}

static void table_neon_wall_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_COLOR *uv;
    int i;

    work->flags |= NEON_ACTIVE;
    if ((++work->tick & 3) == 0) {
        /* @hack spelling: each lit model's meshsets read through &model, which
         * puts the model's own address in the pool (0x0c906984) and loads
         * meshsets with MOV.L @(12,R4),R4 (0x0c9068a4). as model.meshsets,
         * the pool holds the field's address, and 12 of 115 words and 6 pool
         * words differ */
        uv = (&table_neon_lamp_0_model_2)->meshsets[0].vertuv;
        for (i = 0; i < 8; i++, uv++) {
            uv->tex.v += SCROLL_V;
        }
        uv = (&table_neon_lamp_1_model_2)->meshsets[0].vertuv;
        for (i = 0; i < 8; i++, uv++) {
            uv->tex.v += SCROLL_V;
        }
        uv = (&table_neon_lamp_2_model_2)->meshsets[0].vertuv;
        for (i = 0; i < 4; i++, uv++) {
            uv->tex.v += SCROLL_V;
        }
        uv = (&table_neon_lamp_3_model_2)->meshsets[0].vertuv;
        for (i = 0; i < 4; i++, uv++) {
            uv->tex.v += SCROLL_V;
        }
        uv = (&table_neon_lamp_4_model_2)->meshsets[0].vertuv;
        for (i = 0; i < 4; i++, uv++) {
            uv->tex.v += SCROLL_V;
        }
        uv = (&table_neon_lamp_5_model_2)->meshsets[0].vertuv;
        for (i = 0; i < 4; i++, uv++) {
            uv->tex.v += SCROLL_V;
        }
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    task_step_children(task);
    /* @hack redundant: the return, as in table_neon_wall_draw (0x0c906882,
     * 0x0c906884). without it, the function runs 2 bytes short and 113 of 115
     * words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

static NJS_MODEL *table_neon_lamp_models[6][3] = {
    {&table_neon_lamp_0_model_0, &table_neon_lamp_0_model_1,
     &table_neon_lamp_0_model_2},
    {&table_neon_lamp_1_model_0, &table_neon_lamp_1_model_1,
     &table_neon_lamp_1_model_2},
    {&table_neon_lamp_2_model_0, &table_neon_lamp_2_model_1,
     &table_neon_lamp_2_model_2},
    {&table_neon_lamp_3_model_0, &table_neon_lamp_3_model_1,
     &table_neon_lamp_3_model_2},
    {&table_neon_lamp_4_model_0, &table_neon_lamp_4_model_1,
     &table_neon_lamp_4_model_2},
    {&table_neon_lamp_5_model_0, &table_neon_lamp_5_model_1,
     &table_neon_lamp_5_model_2}};

static void table_neon_lamp_init(struct task_work *work, int index)
{
    NJS_MODEL **models = table_neon_lamp_models[index];
    VARS(work)->first = models[0];
    VARS(work)->second = models[1];
    VARS(work)->third = models[2];
    work->mode = index;
}

void table_neon_step(struct task *task)
{
    struct task_work *work = task->work;
    VARS(work)->first = &table_neon_model_0;
    VARS(work)->second = &table_neon_model_1;
    VARS(work)->third = &table_neon_model_2;
    task->step = table_neon_run;
    task->draw = table_neon_draw;
    task->destructor = table_neon_destroy;
}

void table_neon_wall_step(struct task *task)
{
    table_neon_lamp_init(
        task_create_child(TASK_WORK, table_neon_lamp_run, task)->work, 0);
    table_neon_lamp_init(
        task_create_child(TASK_WORK, table_neon_lamp_run, task)->work, 1);
    table_neon_lamp_init(
        task_create_child(TASK_WORK, table_neon_lamp_run, task)->work, 2);
    table_neon_lamp_init(
        task_create_child(TASK_WORK, table_neon_lamp_run, task)->work, 3);
    table_neon_lamp_init(
        task_create_child(TASK_WORK, table_neon_lamp_run, task)->work, 4);
    table_neon_lamp_init(
        task_create_child(TASK_WORK, table_neon_lamp_run, task)->work, 5);
    task->step = table_neon_wall_run;
    task->draw = table_neon_wall_draw;
    task->destructor = table_neon_destroy;
}

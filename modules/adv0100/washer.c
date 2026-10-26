/* O WASHER, a washing machine that rocks, swells and splashes. a touch
 * settles a rocking washer upright, and sets any other rocking again */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv0100/washer.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "effect/splash.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT washer_object;

/* the washer swings by work->timer's turn, which moves ROCK_RATE a step as
 * it rocks. as it settles, the turn moves SETTLE_START a step at first, and
 * SETTLE_SLOWING less each step down to SETTLE_LEAST */
#define ROCK_RATE 2048
#define SETTLE_START 2048
#define SETTLE_LEAST 512
#define SETTLE_SLOWING 32

/* the washer stands upright at each half turn of work->timer. a settling
 * washer stops once its turn is within UPRIGHT past a half turn, or within
 * UPRIGHT + 1 short of the next */
#define UPRIGHT 0x200

struct washer_vars {
    Uint8 unk_00[4];
    Sint16 swell; /* the phase of the washer's swell along y */
    Uint8 unk_06[2];
    Sint16 rate; /* the turn a step as the washer settles */
};

#define VARS(work) ((struct washer_vars *)&(work)->vars)
/* whether the washer's collision was hit on the last step */
#define TOUCHED(work) ((work)->tick)

/* the washer's tip about z, at most 4096 either way, by work->timer's turn */
#define TIP(work) (njSin(TASK_TIMER(work)) * 4096.0F)

enum washer_mode { WASHER_INIT, WASHER_RUN };
/* how the washer swings, in work->submode */
enum washer_swing { WASHER_ROCK, WASHER_SETTLE, WASHER_STILL };

static void washer_init(struct task *task);
static void washer_collision_init(struct task *task);
static void washer_run(struct task *task);
static void washer_draw(struct task *task);
static void washer_destroy(struct task *task);

static struct collision_shape washer_shapes[] = {
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 19.0F, 0.0F},
     11.0F,
     21.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* draw the washer and swing it. while the set editor holds the task, label
 * the editor's angle rows too */
void egg_carrier_washer_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case WASHER_INIT:
            washer_init(task);
            break;
        case WASHER_RUN:
            washer_draw(task);
            washer_run(task);
            break;
        }
    } else {
        switch (work->mode) {
        case WASHER_INIT:
            washer_init(task);
            break;
        case WASHER_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            washer_draw(task);
            washer_run(task);
            break;
        }
    }
}

static void washer_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = washer_draw;
    task->destructor = washer_destroy;
    washer_collision_init(task);
    work->mode = WASHER_RUN;
}

static void washer_collision_init(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, washer_shapes, COLLISION_OBJECT, 1, 1);
    TOUCHED(work) = 0;
    work->timer = 0;
    VARS(work)->swell = 0;
    work->submode = WASHER_ROCK;
}

/* swing the washer as work->submode says, and splash water from its mouth
 * while it swings */
static void washer_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 mouth;
    NJS_POINT3 pos;
    NJS_VECTOR splash;
    NJS_VECTOR speed;
    Uint16 turn;
    Angle ang;

    /* a touch counts on the first step of a hit */
    if (work->collision->flags & COLLISION_HIT) {
        if (!TOUCHED(work)) {
            if (work->submode == WASHER_ROCK) {
                work->submode = WASHER_SETTLE;
                VARS(work)->rate = SETTLE_START;
            } else {
                work->submode = WASHER_ROCK;
            }
        }
        TOUCHED(work) = 1;
    } else {
        TOUCHED(work) = 0;
    }
    switch (work->submode) {
    case WASHER_ROCK:
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN(njRotateZ, TIP(work));
        splash.x = -(njCos(TASK_TIMER(work) - 0x4000) * 1.4650186F);
        splash.y = rand() * (0.4F * RAND_SCALE);
        splash.z = (rand() * RAND_SCALE) - 0.5F;
        njCalcVector(NULL, &splash, &speed);
        njScale(NULL, 1.0F, 1.0F + (0.1F * njSin(VARS(work)->swell)), 1.0F);
        mouth.x = -(njSin(TASK_TIMER(work)) * 10.0F);
        mouth.y = (rand() * RAND_SCALE) + 17.5F;
        mouth.z = (rand() * (0.5F * RAND_SCALE)) - 1.0F;
        njCalcPoint(NULL, &mouth, &pos);
        njPopMatrix(1);
        splash_create(&pos, &speed, (rand() * (0.9F * RAND_SCALE)) + 0.1F);
        work->timer += ROCK_RATE;
        VARS(work)->swell += (int)((rand() * 0.25F) + 4096.0F);
        break;
    case WASHER_SETTLE:
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN(njRotateZ, TIP(work));
        splash.x = -((Float)VARS(work)->rate *
                     njCos(TASK_TIMER(work) - 0x4000) * 0.0007153411F);
        splash.y = rand() * (0.4F * RAND_SCALE);
        splash.z = (rand() * RAND_SCALE) - 0.5F;
        njCalcVector(NULL, &splash, &speed);
        njScale(NULL, 1.0F, 1.0F + (0.1F * njSin(VARS(work)->swell)), 1.0F);
        mouth.x = -(njSin(TASK_TIMER(work)) * 10.0F);
        mouth.y = (rand() * RAND_SCALE) + 17.5F;
        mouth.z = (rand() * (0.5F * RAND_SCALE)) - 1.0F;
        njCalcPoint(NULL, &mouth, &pos);
        njPopMatrix(1);
        splash_create(&pos, &speed, (rand() * (0.9F * RAND_SCALE)) + 0.1F);
        work->timer += VARS(work)->rate;
        VARS(work)->swell += (int)((Float)VARS(work)->rate * (Float)rand() *
                                   (4.0F * RAND_SCALE));
        if (VARS(work)->rate > SETTLE_LEAST) {
            VARS(work)->rate -= SETTLE_SLOWING;
        }
        turn = TASK_TIMER(work) & 0x7fff;
        if (turn >= 0x7fff - UPRIGHT || turn <= UPRIGHT) {
            work->submode = WASHER_STILL;
            work->timer = 0;
        }
        break;
    case WASHER_STILL:
        break;
    }
    collision_register(work);
}

static void washer_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN(njRotateZ, TIP(work));
    njScale(NULL, 1.0F, 1.0F + (0.1F * njSin(VARS(work)->swell)), 1.0F);
    draw_tree(&washer_object);
    njPopMatrix(1);
}

static void washer_destroy(struct task *task)
{
}

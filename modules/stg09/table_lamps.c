/* AkaAoMido lamps on the tables, lit always or by a stopper, a Shoogai or a
 * drop target piece */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"
#include "stg09/casino.h"
#include "stg09/pinball.h"
#include "stg09/table_lamps.h"
#include "system/task.h"
#include "system/util.h"

/* the models of kinds 0 to 2, 3 to 9 and 10 to 18 */
extern NJS_MODEL table_lamp_model_0;
extern NJS_MODEL table_lamp_model_1;
extern NJS_MODEL table_lamp_model_2;

#define TABLE_LAMPS 19

/* set in AkaAoMido's work->flags by its step. its draw, which runs while the
 * game is paused, clears it. the lamps pulse only while it is set */
#define TABLE_LAMPS_PULSING 0x100

/* a stopper's lamp sets this in its work->flags once it sees its stopper
 * up. it clears it once it sees the stopper down */
#define TABLE_LAMP_SEEN_UP 0x8000

/* the steps a stopper's lamp shows once its stopper comes up, counted down
 * in the lamp's work->timer */
#define TABLE_LAMP_FLASH 128

/* a lamp's alpha is the sine of its phase. each step a lamp pulses, its phase
 * rises by TABLE_LAMP_PULSE. a step that finds it past PULSE_END sets it back
 * to PULSE_START instead. PULSE_START is a quarter turn and PULSE_END three
 * quarters. a pass from PULSE_START thus takes the alpha from 1 down to -1. a
 * lamp's phase starts at the phase its place gives. a stopper's lamp's phase
 * starts again at PULSE_START each time its stopper comes up */
#define TABLE_LAMP_PULSE 0x200
#define PULSE_START 0x4000
#define PULSE_END 0xc000

struct table_lamp_place {
    NJS_POINT3 pos;
    Float scale_x;
    Float scale_z;
    Angle ang_y;
    Angle phase; /* the pulse's phase as the lamp starts */
};

/* each lamp's place, indexed by its kind */
static struct table_lamp_place table_lamp_places[TABLE_LAMPS] = {
#include "stg09/table_lamps/table_lamp_places.inc"
};

struct table_lamp_vars {
    Uint8 unk_00[8];
    Angle phase;
};

#define VARS(work) ((struct table_lamp_vars *)&(work)->vars)

static void table_lamps_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void table_lamp_pulse(struct task *task)
{
    struct task_work *work = task->work;

    if (task->parent->work->flags & TABLE_LAMPS_PULSING) {
        if (VARS(work)->phase > PULSE_END) {
            VARS(work)->phase = PULSE_START;
            /* @hack redundant: the return, which ends this path at an RTS of
             * its own with the store in its slot (0x0c9061de); without it, the
             * path joins the RTS below, and 1 of 20 words differs */
            return;
        } else {
            VARS(work)->phase += TABLE_LAMP_PULSE;
        }
    }
}

/* the lamp's kind is its work->mode. the kind picks what lights the lamp,
 * and which of the three models it draws */
static void table_lamp_run(struct task *task)
{
    struct task_work *work = task->work;
    int lit = 0;
    Angle ang;

    switch (work->mode) {
    case 0:
    case 1:
    case 2:
    case 10:
    case 11:
        table_lamp_pulse(task);
        lit = 99;
        break;
    case 3:
        if (casino_flags & CASINO_STOPPER_C_UP) {
            if (!(work->flags & TABLE_LAMP_SEEN_UP)) {
                work->timer = TABLE_LAMP_FLASH;
                VARS(work)->phase = PULSE_START;
                work->flags |= TABLE_LAMP_SEEN_UP;
            } else if (work->timer) {
                table_lamp_pulse(task);
                lit = 3;
                work->timer--;
            }
        } else {
            work->flags &= ~TABLE_LAMP_SEEN_UP;
        }
        break;
    case 4:
        if (casino_flags & CASINO_STOPPER_L_UP) {
            if (!(work->flags & TABLE_LAMP_SEEN_UP)) {
                work->timer = TABLE_LAMP_FLASH;
                VARS(work)->phase = PULSE_START;
                work->flags |= TABLE_LAMP_SEEN_UP;
            } else if (work->timer) {
                table_lamp_pulse(task);
                lit = 4;
                work->timer--;
            }
        } else {
            work->flags &= ~TABLE_LAMP_SEEN_UP;
        }
        break;
    case 5:
        if (casino_flags & CASINO_STOPPER_R_UP) {
            if (!(work->flags & TABLE_LAMP_SEEN_UP)) {
                work->timer = TABLE_LAMP_FLASH;
                VARS(work)->phase = PULSE_START;
                work->flags |= TABLE_LAMP_SEEN_UP;
            } else if (work->timer) {
                table_lamp_pulse(task);
                lit = 5;
                work->timer--;
            }
        } else {
            work->flags &= ~TABLE_LAMP_SEEN_UP;
        }
        break;
    case 6:
        if (casino_flags & CASINO_SHOOGAI_L_DOWN) {
            table_lamp_pulse(task);
            lit = 6;
        }
        break;
    case 7:
        if (casino_flags & CASINO_SHOOGAI_R_DOWN) {
            table_lamp_pulse(task);
            lit = 7;
        }
        break;
    case 8:
        if (!(casino_flags & CASINO_SHOOGAI_L_DOWN)) {
            table_lamp_pulse(task);
            lit = 8;
        }
        break;
    case 9:
        if (!(casino_flags & CASINO_SHOOGAI_R_DOWN)) {
            table_lamp_pulse(task);
            lit = 9;
        }
        break;
    case 12:
        if (casino_flags & CASINO_DROP_TARGET_2_UP) {
            table_lamp_pulse(task);
            lit = 12;
        }
        break;
    case 13:
        if (casino_flags & CASINO_DROP_TARGET_1_UP) {
            table_lamp_pulse(task);
            lit = 13;
        }
        break;
    case 14:
        if (casino_flags & CASINO_DROP_TARGET_0_UP) {
            table_lamp_pulse(task);
            lit = 14;
        }
        break;
    case 15:
        if (casino_flags & CASINO_SHOOGAI_R_DOWN) {
            table_lamp_pulse(task);
            lit = 15;
        }
        break;
    case 16:
        if (casino_flags & CASINO_SHOOGAI_L_DOWN) {
            table_lamp_pulse(task);
            lit = 16;
        }
        break;
    case 17:
        if (!(casino_flags & CASINO_SHOOGAI_L_DOWN)) {
            table_lamp_pulse(task);
            lit = 17;
        }
        break;
    case 18:
        if (!(casino_flags & CASINO_SHOOGAI_R_DOWN)) {
            table_lamp_pulse(task);
            lit = 18;
        }
        break;
    }
    if (lit) {
        draw_set_material(njSin(VARS(work)->phase), 1.0F, 1.0F, 1.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScaleV(NULL, &work->scl);
        if (work->mode < 3) {
            draw_model(&table_lamp_model_0);
        } else if (work->mode < 10) {
            draw_model(&table_lamp_model_1);
        } else {
            draw_model(&table_lamp_model_2);
        }
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

static void table_lamps_draw(struct task *task)
{
    task->work->flags &= ~TABLE_LAMPS_PULSING;
    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    task_step_children(task);
    /* @hack redundant: the return, which makes SHC open the frame in the
     * prologue with ADD #-4,R15 and store task with MOV.L R4,@R15 (0x0c9064c2,
     * 0x0c9064c8). without it, SHC pushes task with MOV.L R4,@-R15, the
     * function runs 2 bytes short and 17 of 17 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void table_lamps_run(struct task *task)
{
    task->work->flags |= TABLE_LAMPS_PULSING;
    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    task_step_children(task);
    /* @hack redundant: the return, as in table_lamps_draw (0x0c9064e4,
     * 0x0c9064ea). without it, the function runs 2 bytes short and 17 of 17
     * words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void table_lamp_init(struct task_work *work, int kind)
{
    struct table_lamp_place *place = &table_lamp_places[kind];

    work->mode = kind;
    work->pos.x = place->pos.x;
    work->pos.y = place->pos.y;
    work->pos.z = place->pos.z;
    work->scl.x = place->scale_x;
    work->scl.y = 1.0F;
    work->scl.z = place->scale_z;
    work->ang.y = place->ang_y;
    VARS(work)->phase = place->phase;
}

void table_lamps_step(struct task *task)
{
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    0);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    1);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    2);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    3);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    4);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    5);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    6);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    7);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    8);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    9);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    10);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    11);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    12);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    13);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    14);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    15);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    16);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    17);
    table_lamp_init(task_create_child(TASK_WORK, table_lamp_run, task)->work,
                    18);
    task->step = table_lamps_run;
    task->draw = table_lamps_draw;
    task->destructor = table_lamps_destroy;
}

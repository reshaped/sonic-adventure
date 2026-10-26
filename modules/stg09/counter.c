/* O HYOUJIKI: a four-digit counter. its digits roll, then lock one at a time
 * from the ones up */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/counter.h"
#include "stg09/gold.h"
#include "stg09/helpers.h"
#include "system/task.h"
#include "system/util.h"

/* the board, whose model holds the lamp's and the digits' materials */
extern NJS_OBJECT counter_object;
extern NJS_MESHSET counter_lamp_meshset;

/* the task starts only within about 600 of the camera or player 0, and
 * destroys itself otherwise. placement_try_leave_by takes the range
 * squared */
#define RANGE ((600.0F * 600.0F) + 10.0F)
/* the counter runs and draws only while player 0 or 1 is this near */
#define NEAR 300.0F

#define DIGITS 4
#define VALUE_MAX 9999
/* the lamp flashes while the digits rest on a value whose last four digits
 * pass this */
#define FLASH_OVER 1000

/* the board's material for the lamp, and the lamp's textures.
 * LAMP_FLASH_TEXTURE shows as each flash ends */
#define LAMP_MATERIAL 4
#define LAMP_TEXTURE 99
#define LAMP_FLASH_TEXTURE 98
/* the board's materials for the digits */
#define ONES_MATERIAL 7
#define TENS_MATERIAL 9
#define HUNDREDS_MATERIAL 8
#define THOUSANDS_MATERIAL 6

#define SE_LOCK 273

struct counter_vars {
    Uint16 rolling; /* the number whose digits show while they roll */
    Uint16 value; /* the number the digits lock on */
};

#define VARS(work) ((struct counter_vars *)&(work)->vars)
/* the lamp's pulse, counted in steps, in vars' byte 8 */
/* @hack type: the pulse reached through a byte cursor 16 bytes into work,
 * which SHC forms again for each read and write (ADD #16 at 0x0c910362,
 * 0x0c91037a and 0x0c910386). as vars.b[8], summed over counter_run and
 * counter_draw, 120 of 420 words differ */
#define PULSE(work) (((Sint8 *)((Sint8 *)(work) + 16))[0])
/* TRUE from the first step a player is near */
#define STARTED(work) ((work)->mode)
/* the digits locked so far, from the ones up. the others roll */
#define LOCKED(work) ((work)->submode)
/* the lamp's flash, counted in steps, or 0 while it doesn't flash */
#define FLASH(work) ((work)->tick)
#define INDEX(work) ((Sint8)(work)->scl.x)

static int counter_roll = 100; /* the steps before the ones digit locks */
/* each later digit locks on the first reroll more than this many steps after
 * the digit before it */
static int counter_lock = 60;
static int counter_reroll = 2; /* the steps between rerolls of the digits */
static int counter_flash = 25; /* the flash's count starts over past this */
/* the flash shows LAMP_TEXTURE up to this count, and LAMP_FLASH_TEXTURE past
 * it */
static int counter_flash_lit = 17;
/* the pulse shows the shifted lamp past this count, and starts over past
 * twice it */
static int counter_pulse = 5;
/* the lamp as the pulse shows it, with its UVs half the texture along in u */
static struct uv_shift counter_lamp_shift[1] = {
#include "stg09/counter/counter_lamp_shift.inc"
};
/* the texture of each digit, 0 to 9 */
static Uint32 counter_digit_textures[10] = {64, 63, 62, 61, 60,
                                            59, 58, 57, 56, 55};
/* the board, its two legs and their feet */
static struct collision_shape counter_shapes[5] = {
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 44.5F, 0.0F},
        50.0F,
        28.5F,
        2.5F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {37.0F, 8.0F, 0.0F},
        5.0F,
        8.0F,
        3.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {-37.0F, 8.0F, 0.0F},
        5.0F,
        8.0F,
        3.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_SPHERE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {37.0F, 0.0F, 3.0F},
        9.0F,
        0.0F,
        0.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_SPHERE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {-37.0F, 0.0F, 3.0F},
        9.0F,
        0.0F,
        0.0F,
        {0},
        0,
        NULL,
    },
};
/* the counter that holds each index, or NULL. the first counter to start
 * with an index holds it until it is destroyed */
static struct task_work *counters[COUNTERS] = {NULL, NULL, NULL, NULL, NULL};

static void counter_init(struct task *task);
static void counter_destroy(struct task *task);
static void counter_run(struct task *task);
static void counter_draw(struct task *task);

Bool counter_set(int index, int value)
{
    struct task_work *work;

    if (index >= 0 && index < COUNTERS && counters[index]) {
        work = counters[index];
        work->timer = 0;
        if (value >= VALUE_MAX) {
            VARS(work)->value = VALUE_MAX;
        } else {
            VARS(work)->value = value;
        }
        VARS(work)->rolling = (int)(rand() * (20000 * RAND_SCALE));
        LOCKED(work) = 0;
        return TRUE;
    }
    return FALSE;
}

int counter_get(int index)
{
    struct task_work *work;

    if (index >= 0 && index < COUNTERS && counters[index]) {
        work = counters[index];
        return VARS(work)->value;
    }
    return -1;
}

void counter_step(struct task *task)
{
    /* @hack redundant: work, read and never used, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c90ffee). without it, 12 of
     * 16 words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        counter_init(task);
    }
}

static void counter_init(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: 52 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint8[52]. the image's ADD #-52,R15 reserves them
     * (0x0c91000e); without them, 99 of 101 words differ */
    Uint8 unused[52];

    if (!placement_editing(task)) {
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, counter_shapes, COLLISION_OBJECT, 1, 5);
    }
    task->draw = counter_draw;
    task->step = counter_run;
    task->destructor = counter_destroy;
    work->timer = 0;
    VARS(work)->value =
        INDEX(work) == 0 ? (Sint16)(rand() * (20000 * RAND_SCALE)) : 0;
    VARS(work)->rolling = (int)(rand() * (20000 * RAND_SCALE));
    LOCKED(work) = DIGITS;
    STARTED(work) = FALSE;
    FLASH(work) = 0;
    uv_shift_make(counter_lamp_shift, 1);
    if (INDEX(work) >= 0 && INDEX(work) < COUNTERS && !counters[INDEX(work)]) {
        counters[INDEX(work)] = work;
    }
}

static void counter_destroy(struct task *task)
{
    struct task_work *work = task->work;

    uv_shift_free(counter_lamp_shift, 1);
    if (INDEX(work) >= 0 && INDEX(work) < COUNTERS &&
        counters[INDEX(work)] == work) {
        counters[INDEX(work)] = NULL;
    }
}

static void counter_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!player_in_range(&work->pos, NEAR)) {
        return;
    }
    /* @hack spelling: the test written == FALSE, under which SHC gives the
     * index read after it R2 (STS FPUL,R2 at 0x0c91018e). as !, the index
     * takes R3, and 3 of 228 words differ */
    if (STARTED(work) == FALSE) {
        if (INDEX(work) != 0) {
            VARS(work)->value = gold_heap_get_rings();
        }
        STARTED(work) = TRUE;
        work->timer = 0;
    }
    counter_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "ID:");
        njPrintD(EDITOR_NUMBER(EDITOR_ROW_SCL_X), INDEX(work), 3);
        return;
    }
    if (!INDEX(work) && (work->collision->flags & COLLISION_HIT)) {
        work->timer = 0;
        VARS(work)->value = (int)(rand() * (20000 * RAND_SCALE));
        {
            /* @hack spelling: vars through a pointer, which SHC forms as work
             * plus 8 before it reads value (ADD #8,R4 at 0x0c910200).
             * through VARS(work) at each use, 48 of 228 words differ */
            struct counter_vars *vars = VARS(work);

            vars->rolling = vars->value;
        }
        LOCKED(work) = 0;
    }
    if (LOCKED(work) == 0) {
        if (TASK_TIMER(work) < counter_roll) {
            if (!(TASK_TIMER(work) % counter_reroll)) {
                VARS(work)->rolling = (int)(rand() * (10000 * RAND_SCALE));
            }
        } else {
            ++LOCKED(work);
            work->timer = 0;
        }
    } else if (LOCKED(work) < DIGITS) {
        if (!(TASK_TIMER(work) % counter_reroll)) {
            VARS(work)->rolling = (int)(rand() * (10000 * RAND_SCALE));
            if (TASK_TIMER(work) > counter_lock) {
                sound_play(SE_LOCK, 0, 0, 0);
                ++LOCKED(work);
                work->timer = 0;
            }
        }
    }
    /* a flash, once started, runs to its count's end. otherwise the lamp
     * pulses */
    if (FLASH(work) ||
        (LOCKED(work) >= DIGITS && VARS(work)->value % 10000 > FLASH_OVER)) {
        ++FLASH(work);
        if (FLASH(work) > counter_flash) {
            FLASH(work) = 0;
        }
    } else {
        if (PULSE(work)++ > counter_pulse) {
            if (PULSE(work) > 2 * counter_pulse) {
                PULSE(work) = 0;
            }
        }
    }
    collision_register(work);
    /* @hack spelling: the test for the wrap to 0, written <= 0 for the
     * image's CMP/PL (0x0c910398). the timer is unsigned, so the test holds
     * at 0 alone, and the decrement holds the timer at 65535. as == 0, the
     * test is TST, and 2 of 228 words differ */
    if (++TASK_TIMER(work) <= 0) {
        --TASK_TIMER(work);
    }
}

static void counter_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &counter_object;
    Uint32 *lamp = &object->model->mats[LAMP_MATERIAL].attr_texId;
    Uint32 *digits[DIGITS];
    int rolling;
    int value;
    int i;
    Angle ang;

    if (!player_in_range(&work->pos, NEAR)) {
        return;
    }
    digits[0] = &object->model->mats[ONES_MATERIAL].attr_texId;
    digits[1] = &object->model->mats[TENS_MATERIAL].attr_texId;
    digits[2] = &object->model->mats[HUNDREDS_MATERIAL].attr_texId;
    digits[3] = &object->model->mats[THOUSANDS_MATERIAL].attr_texId;
    {
        /* @hack spelling: vars through a pointer, as in counter_run (ADD #8,R4
         * at 0x0c9103ea). through VARS(work) at each use, 167 of 192 words
         * differ */
        struct counter_vars *vars = VARS(work);

        rolling = vars->rolling;
        value = vars->value;
    }
    for (i = 0; i < DIGITS; ++i) {
        *digits[i] = counter_digit_textures[i < LOCKED(work) ? value % 10
                                                             : rolling % 10];
        value /= 10;
        rolling /= 10;
    }
    if (PULSE(work) > counter_pulse) {
        uv_shift_use(counter_lamp_shift, 1);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    *lamp =
        FLASH(work) <= counter_flash_lit ? LAMP_TEXTURE : LAMP_FLASH_TEXTURE;
    draw_model(object->model);
    njPopMatrix(1);
    if (PULSE(work) > counter_pulse) {
        uv_shift_restore(counter_lamp_shift, 1);
    }
    /* @hack redundant: mode read and never used, which keeps work live past
     * the pulse's last read, so SHC copies it before adding 16 (MOV R14,R3
     * at 0x0c910562). without it, 20 of 192 words differ */
    (void)STARTED(work);
}

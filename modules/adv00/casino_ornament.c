/* the casino act's ornament, whose textures scroll and whose light blinks
 * by night, with a row of ten lamps */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/casino_ornament.h"
#include "adv00/texlists.h"
#include "b_egm2/ornament_scroll.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT casino_ornament_object;
/* the row's base, whose child and that child's siblings are the lamps */
extern NJS_OBJECT casino_lamps_object;
extern NJS_OBJECT dat_0caa7778;
extern NJS_OBJECT dat_0caab730;
extern NJS_OBJECT dat_0caadbac;
extern NJS_COLOR dat_0caaa014[];
extern NJS_COLOR dat_0caab7f0[];
extern NJS_COLOR dat_0caabbe0[];
extern NJS_COLOR dat_0caabc20[];
extern NJS_COLOR dat_0caac3ec[];

#define LAMPS 10
#define PARTS 3

/* the ornament's work->mode. a mode gives way to the other once MODE_STEPS
 * passes MODE_END, so each lasts MODE_END + 1 steps */
enum ornament_mode { ORNAMENT_BLINK, ORNAMENT_SCROLL };
#define MODE_END 360

/* the ornament's work->submode. init clears NJD_FLAG_IGNORE_LIGHT from
 * casino_ornament_object's materials. in ORNAMENT_GLOW the draw sets the flag
 * on them, and in ORNAMENT_SHADED it leaves them as they are */
enum ornament_light { ORNAMENT_GLOW, ORNAMENT_SHADED };

/* in ORNAMENT_BLINK the light glows while MODE_STEPS mod BLINK_PERIOD is
 * below BLINK_GLOW. it is shaded for the rest of each period */
#define BLINK_PERIOD 60
#define BLINK_GLOW 30

/* the steps the ornament has spent in its mode */
#define MODE_STEPS(work) ((work)->vars.l[1])

/* the stage and act the ornament started in, as stage_get_stage_act gives
 * them */
#define ORNAMENT_STAGE_ACT(work) TASK_TIMER(work)

/* the lamps' work->mode. in LAMPS_SHIFT every third lamp is lit. every
 * SHIFT_EVERY steps, the lit lamps move one place toward the first. once
 * work->timer passes SHIFT_END, the lamps sweep */
enum lamps_mode { LAMPS_SHIFT, LAMPS_SWEEP };
#define SHIFT_EVERY 10
#define SHIFT_END 800

/* the lamps' work->submode in LAMPS_SWEEP: four sweeps, each of which
 * changes a lamp every SWEEP_EVERY steps. the lamps light from the first to
 * the last and go dark the same way. then they light and go dark from the
 * last back to the first, and LAMPS_SHIFT follows */
enum lamps_sweep { SWEEP_ON, SWEEP_OFF, SWEEP_BACK_ON, SWEEP_BACK_OFF };
#define SWEEP_EVERY 6

/* the ornament's other models, drawn as their materials have them:
 * ignoring the light by night, and taking it by day and in the evening */
static NJS_OBJECT *casino_ornament_parts[PARTS] = {&dat_0caa7778, &dat_0caab730,
                                                   &dat_0caadbac};
/* scroll_0 and scroll_1 move UVs of casino_ornament_object's tree, in
 * ORNAMENT_SCROLL alone. scroll_2 moves UVs of casino_ornament_parts[1]'s
 * tree in both modes */
static struct uv_group scroll_0_groups[1] = {{dat_0caabc20, 70}};
static struct ornament_scroll scroll_0 = {
#include "adv00/casino_ornament/scroll_0.inc"
};
static struct uv_group scroll_1_groups[3] = {
    {dat_0caab7f0, 12}, {dat_0caabbe0, 16}, {dat_0caac3ec, 16}};
static struct ornament_scroll scroll_1 = {
#include "adv00/casino_ornament/scroll_1.inc"
};
static struct uv_group scroll_2_groups[1] = {{dat_0caaa014, 96}};
static struct ornament_scroll scroll_2 = {
#include "adv00/casino_ornament/scroll_2.inc"
};

/* which lamps are lit */
static Sint8 casino_lamps_lit[LAMPS];

static void casino_ornament_step(struct task *task);
static void casino_ornament_draw(struct task *task);
static void casino_ornament_destroy(struct task *task);
static void casino_lamps_init(struct task *task);
static void casino_lamps_step(struct task *task);
static void casino_lamps_draw(struct task *task);
static void casino_ornament_draw_still(struct task *task);
/* @hack spelling: casino_ornament_draw_still under a second name, at the same
 * address in the symbol table. SHC pools a name once, so the second name
 * gives the step's store and the draw's store a pool word each, 0x0c90ad98
 * and 0x0c90ad9c. without it, the stores share 0x0c90ad98, 3 of the 76
 * words of casino_ornament_init differ, and the code from 0x0c90adc0 on
 * lies 32 bytes early */
extern void casino_ornament_step_still(struct task *task);

/* or bits into the attrflags of every material in the tree */
static void tree_attr_or(NJS_OBJECT *object, Uint32 bits)
{
    NJS_MATERIAL *material;
    Uint16 i;
    Uint16 count;

    count = object->model->nbMat;
    for (i = 0, material = object->model->mats; i < count; i++, material++) {
        /* @hack spelling: the or written out, which keeps material in R4 and
         * i in R5 (0x0c90ab0e to 0x0c90ab28). as |=, the two trade and 7 of
         * 37 words differ */
        material->attrflags = material->attrflags | bits;
    }
    if (object->child) {
        tree_attr_or(object->child, bits);
    }
    if (object->sibling) {
        tree_attr_or(object->sibling, bits);
    }
}

/* keep only mask's bits in the attrflags of every material in the tree */
static void tree_attr_and(NJS_OBJECT *object, Uint32 mask)
{
    NJS_MATERIAL *material;
    Uint16 i;
    Uint16 count;

    count = object->model->nbMat;
    for (i = 0, material = object->model->mats; i < count; i++, material++) {
        /* @hack spelling: the and written out, which keeps material in R4 and
         * i in R5 (0x0c90ab58 to 0x0c90ab72). as &=, the two trade and 7 of
         * 37 words differ */
        material->attrflags = material->attrflags & mask;
    }
    if (object->child) {
        tree_attr_and(object->child, mask);
    }
    if (object->sibling) {
        tree_attr_and(object->sibling, mask);
    }
}

static void ornament_scroll_step(struct ornament_scroll *scroll)
{
    Sint16 timer;
    Sint16 phase;
    int offset;
    NJS_COLOR *uv;
    int i;
    int j;
    int speed;
    int count;

    timer = scroll->timer + 1;
    if (timer > scroll->delay) {
        phase = scroll->phase;
        /* @hack redundant: a never-run copy of phase into j, which with the
         * never-run tests of j below keeps speed in R14 (0x0c90abae,
         * 0x0c90abb0, 0x0c90abc0, 0x0c90abc2); without it, speed takes R7
         * and 4 of 96 words differ */
        if (0) {
            j = phase;
        }
        speed = scroll->speed;
        phase += speed;
        if (phase > 255) {
            phase -= 256;
            offset = speed - 256;
        } else {
            offset = speed;
        }
        /* @hack redundant: never-run tests of j, which the copy above needs
         * to keep speed in R14; without them, 4 of 96 words differ */
        if (0) {
            if (j) {
            }
            if (j) {
            }
            if (j) {
            }
            if (j) {
            }
        }
        scroll->phase = phase;
        if (!(scroll->flags & SCROLL_FORWARD)) {
            offset = -offset;
        }
        /* @hack spelling: i = i + 1 and uv = uv + 1, which keep i in R14
         * and uv in R7 (0x0c90abde to 0x0c90ac74). with i++, 36 of 96 words
         * differ, and with uv++, 23 */
        if (scroll->flags & SCROLL_U) {
            for (i = 0; i < scroll->count; i = i + 1) {
                uv = scroll->groups[i].uvs;
                count = scroll->groups[i].count;
                /* @hack type: count narrowed to a Sint16 in each test, which
                 * keeps the image's EXTS.W here at 0x0c90abee and in the v
                 * loop at 0x0c90ac5a; without it, 50 of 96 words differ */
                for (j = 0; j < (Sint16)count; j++) {
                    uv->tex.u += offset;
                    uv = uv + 1;
                }
            }
        } else if (scroll->flags & SCROLL_V) {
            for (i = 0; i < scroll->count; i = i + 1) {
                uv = scroll->groups[i].uvs;
                count = scroll->groups[i].count;
                for (j = 0; j < (Sint16)count; j++) {
                    uv->tex.v += offset;
                    uv = uv + 1;
                }
            }
        }
        timer = 0;
    }
    scroll->timer = timer;
}

void casino_ornament_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    work->mode = ORNAMENT_SCROLL;
    ORNAMENT_STAGE_ACT(work) = stage_get_stage_act();
    work->submode = ORNAMENT_GLOW;
    MODE_STEPS(work) = 0;
    if (story_get_time() == TIME_NIGHT) {
        task_create_child(TASK_WORK, casino_lamps_init, task);
        tree_attr_and(&casino_ornament_object, ~NJD_FLAG_IGNORE_LIGHT);
        tree_attr_or(&dat_0caa7778, NJD_FLAG_IGNORE_LIGHT);
        tree_attr_or(&casino_lamps_object, NJD_FLAG_IGNORE_LIGHT);
        tree_attr_or(&dat_0caab730, NJD_FLAG_IGNORE_LIGHT);
        tree_attr_or(&dat_0caadbac, NJD_FLAG_IGNORE_LIGHT);
        task->step = casino_ornament_step;
        task->draw = casino_ornament_draw;
        task->destructor = casino_ornament_destroy;
    } else {
        tree_attr_and(&dat_0caa7778, ~NJD_FLAG_IGNORE_LIGHT);
        tree_attr_and(&casino_lamps_object, ~NJD_FLAG_IGNORE_LIGHT);
        tree_attr_and(&dat_0caab730, ~NJD_FLAG_IGNORE_LIGHT);
        tree_attr_and(&casino_ornament_object, ~NJD_FLAG_IGNORE_LIGHT);
        tree_attr_and(&dat_0caadbac, ~NJD_FLAG_IGNORE_LIGHT);
        task->step = casino_ornament_step_still;
        task->draw = casino_ornament_draw_still;
    }
}

static void casino_ornament_step(struct task *task)
{
    struct task_work *work = task->work;
    int beat;

    /* @hack type: stage_get_stage_act called as returning an int, not
     * stage.h's Uint16, so the step compares R0 whole (CMP/EQ R0,R8 at
     * 0x0c90ad30). the callee leaves stage << 8 | act there unnarrowed
     * (0x8c052ac8, 0x8c052acc). as a Uint16, an EXTU.W R0,R0 comes first
     * and 9 of 78 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-cast-function-type-mismatch) */
    if (ORNAMENT_STAGE_ACT(work) != ((int (*)(void))stage_get_stage_act)()) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case ORNAMENT_SCROLL:
        MODE_STEPS(work)++;
        if (MODE_STEPS(work) > MODE_END) {
            MODE_STEPS(work) = 0;
            work->mode = ORNAMENT_BLINK;
        }
        ornament_scroll_step(&scroll_0);
        ornament_scroll_step(&scroll_1);
        break;
    case ORNAMENT_BLINK:
        MODE_STEPS(work)++;
        if (MODE_STEPS(work) > MODE_END) {
            MODE_STEPS(work) = 0;
            work->mode = ORNAMENT_SCROLL;
            work->submode = ORNAMENT_GLOW;
        } else {
            beat = MODE_STEPS(work) % BLINK_PERIOD;
            if (beat < BLINK_GLOW) {
                work->submode = ORNAMENT_GLOW;
            } else {
                work->submode = ORNAMENT_SHADED;
            }
        }
        break;
    }
    ornament_scroll_step(&scroll_2);
    casino_ornament_draw(task);
    task_step_children(task);
}

static void casino_ornament_draw(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    njSetTexture(&ss_casino_texlist);
    draw_control_save();
    draw_attr_save();
    draw_control_set(NJD_CONTROL_3D_CONSTANT_ATTR);
    if (work->submode == ORNAMENT_GLOW) {
        draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT);
        draw_attr_clear(NJD_FLAG_IGNORE_LIGHT, 0);
    } else {
        draw_attr_set(NJD_FLAG_IGNORE_LIGHT, 0);
        draw_attr_clear(0, NJD_FLAG_IGNORE_LIGHT);
    }
    draw_tree(&casino_ornament_object);
    draw_attr_restore();
    draw_control_restore();
    for (i = 0; i < PARTS; i++) {
        draw_tree(casino_ornament_parts[i]);
    }
}

static void casino_ornament_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void casino_lamps_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = LAMPS_SHIFT;
    work->timer = 0;
    work->tick = 0;
    tree_attr_and(&casino_lamps_object, ~NJD_FLAG_IGNORE_LIGHT);
    task->step = casino_lamps_step;
    task->draw = casino_lamps_draw;
}

/* work->timer counts the steps of each mode. work->tick counts the shifts
 * in LAMPS_SHIFT, and is the lamp the sweep is at in LAMPS_SWEEP */
static void casino_lamps_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;
    int k;

    switch (work->mode) {
    case LAMPS_SHIFT:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > SHIFT_END) {
            work->submode = SWEEP_ON;
            work->timer = 0;
            work->tick = 0;
            work->mode = LAMPS_SWEEP;
            for (i = 0; i < LAMPS; i++) {
                casino_lamps_lit[i] = FALSE;
            }
        } else if (TASK_TIMER(work) % SHIFT_EVERY == 0) {
            work->tick++;
            k = work->tick % 3;
            for (i = 0; i < LAMPS; i++) {
                if (k % 3 == 0) {
                    casino_lamps_lit[i] = TRUE;
                } else {
                    casino_lamps_lit[i] = FALSE;
                }
                k++;
            }
        }
        break;
    case LAMPS_SWEEP:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) % SWEEP_EVERY != 0) {
            break;
        }
        switch (work->submode) {
        case SWEEP_ON:
            casino_lamps_lit[work->tick] = TRUE;
            if (work->tick == LAMPS - 1) {
                work->submode = SWEEP_OFF;
                work->tick = 0;
            } else {
                work->tick++;
            }
            break;
        case SWEEP_OFF:
            casino_lamps_lit[work->tick] = FALSE;
            if (work->tick == LAMPS - 1) {
                work->submode = SWEEP_BACK_ON;
                work->tick = LAMPS - 1;
            } else {
                work->tick++;
            }
            break;
        case SWEEP_BACK_ON:
            casino_lamps_lit[work->tick] = TRUE;
            if (work->tick == 0) {
                work->submode = SWEEP_BACK_OFF;
                work->tick = LAMPS - 1;
            } else {
                work->tick--;
            }
            break;
        case SWEEP_BACK_OFF:
            casino_lamps_lit[work->tick] = FALSE;
            if (work->tick == 0) {
                work->timer = 0;
                work->submode = SWEEP_ON;
                work->tick = 0;
                work->mode = LAMPS_SHIFT;
            } else {
                work->tick--;
            }
            break;
        }
        break;
    }
    casino_lamps_draw(task);
}

/* draw the row's base, then each lamp, which ignores the light while it is
 * lit */
static void casino_lamps_draw(struct task *task)
{
    NJS_OBJECT *object;
    NJS_MODEL *model;
    int i;

    njSetTexture(&ss_casino_texlist);
    draw_control_save();
    draw_attr_save();
    draw_control_set(NJD_CONTROL_3D_CONSTANT_ATTR);
    object = &casino_lamps_object;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    /* @hack spelling: the base's model read through model, which loads
     * draw_model's address into R8, where it stays for the lamps
     * (0x0c90b214), and njPushMatrix's into R3 (0x0c90b1f8). as
     * object->model, the two trade and 173 of 315 words differ */
    model = object->model;
    draw_model(model);
    object = object->child;
    for (i = 0; i < LAMPS; i++) {
        if (casino_lamps_lit[i]) {
            draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT);
            draw_attr_clear(NJD_FLAG_IGNORE_LIGHT, 0);
        } else {
            draw_attr_set(NJD_FLAG_IGNORE_LIGHT, 0);
            draw_attr_clear(0, NJD_FLAG_IGNORE_LIGHT);
        }
        njPushMatrix(NULL);
        njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
        TURN(njRotateY, object->ang[1]);
        draw_model(object->model);
        njPopMatrix(1);
        object = object->sibling;
    }
    njPopMatrix(1);
    draw_attr_restore();
    draw_control_restore();
}

/* the step and draw by day and in the evening: the parts, the ornament and
 * the lamps, as their materials have them */
static void casino_ornament_draw_still(struct task *task)
{
    int i;

    njSetTexture(&ss_casino_texlist);
    for (i = 0; i < PARTS; i++) {
        draw_tree(casino_ornament_parts[i]);
    }
    draw_tree(&casino_ornament_object);
    draw_tree(&casino_lamps_object);
}

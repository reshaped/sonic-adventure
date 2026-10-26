/* O KAZARI: an ornament whose textures scroll and whose light blinks, with
 * a row of ten lamps */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm2/ornament.h"
#include "b_egm2/ornament_scroll.h"
#include "b_egm2/ornament_textures.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT ornament_object;
/* the row's base, whose child and that child's siblings are the lamps */
extern NJS_OBJECT lamps_object;
extern NJS_OBJECT dat_0c98b490;
extern NJS_OBJECT dat_0c98f448;
extern NJS_OBJECT dat_0c9918c4;
extern NJS_COLOR dat_0c98dd2c[];
extern NJS_COLOR dat_0c98f508[];
extern NJS_COLOR dat_0c98f8f8[];
extern NJS_COLOR dat_0c98f938[];
extern NJS_COLOR dat_0c990104[];

/* the ornament ends once the camera and player 0 are both farther than
 * 1000 from it. placement_try_leave_by takes the range squared */
#define RANGE (1000.0F * 1000.0F)

#define LAMPS 10
#define PARTS 3

/* the ornament's work->mode. a mode gives way to the other once MODE_STEPS
 * passes MODE_END, so each lasts MODE_END + 1 steps */
enum ornament_mode { ORNAMENT_BLINK, ORNAMENT_SCROLL };
#define MODE_END 360

/* the ornament's work->submode. ornament_step clears NJD_FLAG_IGNORE_LIGHT
 * from ornament_object's materials. in ORNAMENT_GLOW the draw sets the flag
 * on them, and in ORNAMENT_SHADED it leaves them as they are */
enum ornament_light { ORNAMENT_GLOW, ORNAMENT_SHADED };

/* in ORNAMENT_BLINK the light glows while MODE_STEPS mod BLINK_PERIOD is
 * below BLINK_GLOW. it is shaded for the rest of each period */
#define BLINK_PERIOD 60
#define BLINK_GLOW 30

/* the steps the ornament has spent in its mode */
#define MODE_STEPS(work) ((work)->vars.l[1])

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

/* the ornament's other models, drawn in its frame as their materials have
 * them */
static NJS_OBJECT *ornament_parts[PARTS] = {&dat_0c98b490, &dat_0c98f448,
                                            &dat_0c9918c4};
/* scroll_0 and scroll_1 move UVs of ornament_object's tree, in
 * ORNAMENT_SCROLL alone. scroll_2 moves UVs of ornament_parts[1]'s tree in
 * both modes */
static struct uv_group scroll_0_groups[1] = {{dat_0c98f938, 70}};
static struct ornament_scroll scroll_0 = {
#include "b_egm2/ornament/scroll_0.inc"
};
static struct uv_group scroll_1_groups[3] = {
    {dat_0c98f508, 12}, {dat_0c98f8f8, 16}, {dat_0c990104, 16}};
static struct ornament_scroll scroll_1 = {
#include "b_egm2/ornament/scroll_1.inc"
};
static struct uv_group scroll_2_groups[1] = {{dat_0c98dd2c, 96}};
static struct ornament_scroll scroll_2 = {
#include "b_egm2/ornament/scroll_2.inc"
};

/* which lamps are lit */
static Sint8 lamps_lit[LAMPS];

static void ornament_run(struct task *task);
static void ornament_draw(struct task *task);
static void ornament_destroy(struct task *task);
static void lamps_start(struct task *task);
static void lamps_step(struct task *task);
static void lamps_draw(struct task *task);

void tree_attr_or(NJS_OBJECT *object, Uint32 bits)
{
    NJS_MATERIAL *material;
    Uint16 i;
    Uint16 count;

    count = object->model->nbMat;
    for (i = 0, material = object->model->mats; i < count; i++, material++) {
        /* @hack spelling: the or written out, which keeps material in R4 and
         * i in R5 (0x0c9078ae to 0x0c9078c8). as |=, the two trade and 7 of
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
         * i in R5 (0x0c9078f8 to 0x0c907912). as &=, the two trade and 7 of
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
         * never-run tests of j below keeps speed in R14 (0x0c90794e,
         * 0x0c907950, 0x0c907960, 0x0c907962); without it, speed takes R7
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
         * and uv in R7 (0x0c90797e to 0x0c907a14). with i++, 36 of 96 words
         * differ, and with uv++, 23 */
        if (scroll->flags & SCROLL_U) {
            for (i = 0; i < scroll->count; i = i + 1) {
                uv = scroll->groups[i].uvs;
                count = scroll->groups[i].count;
                /* @hack type: count narrowed to a Sint16 in each test, which
                 * keeps the image's EXTS.W here at 0x0c90798e and in the v
                 * loop at 0x0c9079fa; without it, 50 of 96 words differ */
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

void ornament_step(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    work->mode = ORNAMENT_SCROLL;
    /* the stage and act it started in, which none of its steps reads */
    work->timer = stage_get_stage_act();
    work->submode = ORNAMENT_GLOW;
    MODE_STEPS(work) = 0;
    task_create_child(TASK_WORK, lamps_start, task);
    tree_attr_and(&ornament_object, ~NJD_FLAG_IGNORE_LIGHT);
    task->step = ornament_run;
    task->draw = ornament_draw;
    task->destructor = ornament_destroy;
}

static void ornament_run(struct task *task)
{
    struct task_work *work = task->work;
    int beat;

    if (placement_try_leave_by(task, RANGE)) {
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
    ornament_draw(task);
    task_step_children(task);
}

/* draw ornament_object with its root at work->pos, since draw_tree adds the
 * root's own pos back. the parts follow in the same frame */
static void ornament_draw(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    njSetTexture(&ornament_texlist);
    draw_attr_save();
    if (work->submode == ORNAMENT_GLOW) {
        draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT);
        draw_attr_clear(NJD_FLAG_IGNORE_LIGHT, 0);
    } else {
        draw_attr_set(NJD_FLAG_IGNORE_LIGHT, 0);
        draw_attr_clear(0, NJD_FLAG_IGNORE_LIGHT);
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x - ornament_object.pos[0],
                work->pos.y - ornament_object.pos[1],
                work->pos.z - ornament_object.pos[2]);
    draw_tree(&ornament_object);
    draw_attr_restore();
    for (i = 0; i < PARTS; i++) {
        draw_tree(ornament_parts[i]);
    }
    njPopMatrix(1);
}

static void ornament_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void lamps_start(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = LAMPS_SHIFT;
    work->timer = 0;
    work->tick = 0;
    tree_attr_and(&lamps_object, ~NJD_FLAG_IGNORE_LIGHT);
    task->step = lamps_step;
    task->draw = lamps_draw;
}

/* work->timer counts the steps of each mode. work->tick counts the shifts
 * in LAMPS_SHIFT, and is the lamp the sweep is at in LAMPS_SWEEP */
static void lamps_step(struct task *task)
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
                lamps_lit[i] = FALSE;
            }
        } else if (TASK_TIMER(work) % SHIFT_EVERY == 0) {
            work->tick++;
            k = work->tick % 3;
            for (i = 0; i < LAMPS; i++) {
                if (k % 3 == 0) {
                    lamps_lit[i] = TRUE;
                } else {
                    lamps_lit[i] = FALSE;
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
            lamps_lit[work->tick] = TRUE;
            if (work->tick == LAMPS - 1) {
                work->submode = SWEEP_OFF;
                work->tick = 0;
            } else {
                work->tick++;
            }
            break;
        case SWEEP_OFF:
            lamps_lit[work->tick] = FALSE;
            if (work->tick == LAMPS - 1) {
                work->submode = SWEEP_BACK_ON;
                work->tick = LAMPS - 1;
            } else {
                work->tick++;
            }
            break;
        case SWEEP_BACK_ON:
            lamps_lit[work->tick] = TRUE;
            if (work->tick == 0) {
                work->submode = SWEEP_BACK_OFF;
                work->tick = LAMPS - 1;
            } else {
                work->tick--;
            }
            break;
        case SWEEP_BACK_OFF:
            lamps_lit[work->tick] = FALSE;
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
    lamps_draw(task);
}

/* draw the row's base in the ornament's frame, then each lamp. a lamp
 * ignores the light while it is lit */
static void lamps_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    int i;

    njSetTexture(&ornament_texlist);
    draw_control_save();
    draw_attr_save();
    object = &lamps_object;
    /* @hack redundant: a never-run block of tests of object, which gives object
     * the image's register, R14 (0x0c907efe, 0x0c907f16); without it, R12, R13
     * and R14 rotate and 16 of 330 words differ */
    if (0) {
        if (object) {
        }
        if (object) {
        }
        if (object) {
        }
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x - ornament_object.pos[0],
                work->pos.y - ornament_object.pos[1],
                work->pos.z - ornament_object.pos[2]);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    draw_model(object->model);
    for (object = object->child, i = 0; i < LAMPS; i++) {
        if (lamps_lit[i]) {
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

/* the arches O ARCH 1 and O ARCH 2, the lights O LIGHT 1 and O LIGHT 2, and
 * the pole O POLE, whose beam swings */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "minicart/arch.h"
#include "minicart/shareobj_textures.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* a prop's work vars. the props are the arches, the lights, the spotlights
 * and the pole. pole_step makes prop_draw the pole's draw, which reads
 * object here, so pole_vars starts with object too */
struct prop_vars {
    NJS_OBJECT *object;
    /* how far a spotlight rocks either way. SPOTLIGHT_MINUS_X's limit is
     * negated, so it rocks the mirror way. the negated limit is cut to 16
     * bits, from 0 to 0xffff */
    Angle limit;
    Angle turn; /* a spotlight's turn so far, or O LIGHT 2's turn each step */
};

struct pole_vars {
    NJS_OBJECT *object;
    Uint8 unk_04[4];
    /* the beam's swing limit and its swing so far, in angle units, by
     * pole_beam */
    /* @hack type: an array, not two members. pole_step forms its address with
     * ADD #16 (0x0c9159ee) and reads both through it. as two members, SHC
     * reads each at its offset in the work, the step runs 12 bytes shorter,
     * and 58 of its 155 words differ */
    Sint16 beam[2];
};

enum pole_beam { BEAM_LIMIT, BEAM_SWING };

#define VARS(work) ((struct prop_vars *)&(work)->vars)
#define POLE_VARS(work) ((struct pole_vars *)&(work)->vars)

/* SCROLL is how far an arch's or the pole's texture has scrolled in v. it
 * moves SCROLL_SPEED each step for an arch, and each frame for the pole.
 * the set entry's scale x, rounded down, gives SCROLL_SPEED */
#define SCROLL(work) TASK_TIMER(work)
#define SCROLL_SPEED(work) ((work)->tick)
/* the scroll goes back to 0 once it reaches SCROLL_WRAP, a texture's span in
 * v */
#define SCROLL_WRAP 256

/* a spotlight's work flags: alone on its arch, or one of a pair. a pair's
 * spotlights stand at the arch's plus x and minus x */
enum spotlight_place { SPOTLIGHT_ALONE, SPOTLIGHT_PLUS_X, SPOTLIGHT_MINUS_X };

enum prop_mode { PROP_INIT, PROP_RUN };
/* which way a spotlight or the pole's beam swings, in work->submode.
 * SWING_OUT heads for the limit, and SWING_BACK for the limit negated */
enum swing_way { SWING_OUT, SWING_BACK };

/* the ranges the steps give placement_try_leave_by, squared. they are
 * 3010 for an arch or a light, and 3510 for the pole */
#define PROP_RANGE (3010.0F * 3010.0F)
#define POLE_RANGE (3510.0F * 3510.0F)

/* how far a spotlight turns each step as it rocks, in angle units */
#define SPOTLIGHT_SPEED 64

/* the meshset of each model whose uvs scroll, and how many uvs it holds */
#define ARCH_1_MESHSET 3
#define ARCH_1_UVS 56
#define ARCH_2_MESHSET 8
#define ARCH_2_UVS 16
#define POLE_MESHSET 1
#define POLE_UVS 12

/* spotlight_object is a sub arch with its lamp as its child. pole_object is
 * a pole with its beam as its child */
extern NJS_OBJECT arch_1_object;
extern NJS_OBJECT arch_2_object;
extern NJS_OBJECT spotlight_object[];
extern NJS_OBJECT light_1_object[];
extern NJS_OBJECT light_2_object[];
extern NJS_OBJECT pole_object;

static void prop_destroy(struct task *task);
static void prop_draw(struct task *task);
static void arch_1_draw(struct task *task);
static void arch_2_draw(struct task *task);
static void spotlight_draw(struct task *task);
static void spotlight_step(struct task *task);
static void pole_draw(struct task *task);

/* O ARCH 2's spotlights, one or a pair, as task_create_children takes
 * them */
static struct task_spec arch_one_spotlight[2] = {
    {spotlight_step,
     TASK_WORK,
     SPOTLIGHT_ALONE,
     {0, 0, 0},
     {0.0F, 17.5F, 0.0F},
     (Sint32)spotlight_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};
static struct task_spec arch_two_spotlights[3] = {
    {spotlight_step,
     TASK_WORK,
     SPOTLIGHT_PLUS_X,
     {0, 0, 0},
     {18.0F, 17.5F, 0.0F},
     (Sint32)spotlight_object},
    {spotlight_step,
     TASK_WORK,
     SPOTLIGHT_MINUS_X,
     {0, 0, 0},
     {-18.0F, 17.5F, 0.0F},
     (Sint32)spotlight_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

static void prop_destroy(struct task *task)
{
    task_kill(task);
}

/* draw VARS(work)->object's tree at the task's pos and angles, only on a
 * frame's first step */
static void prop_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&obj_shareobj_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(VARS(work)->object);
        njPopMatrix(1);
    }
}

/* advance the scroll, and draw the arch with ARCH_1_MESHSET's uvs scrolled
 * by it in v. the uvs go back after the draw, since every O ARCH 1 shares
 * the model */
static void arch_1_draw(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 i;

    /* @hack spelling: the sum stored inside the test, which tests R0 as it
     * stores it (MOV.W R0,@(6,R14) at 0x0c914bf4, EXTU.W R0,R0 at
     * 0x0c914bf6); stored before the test and read back, 74 of 85 words and
     * 1 pool word differ */
    if ((SCROLL(work) += SCROLL_SPEED(work)) >= SCROLL_WRAP) {
        SCROLL(work) = 0;
    }
    for (i = 0; i < ARCH_1_UVS; i++) {
        VARS(work)->object->model->meshsets[ARCH_1_MESHSET].vertuv[i].tex.v -=
            SCROLL(work);
    }
    prop_draw(task);
    for (i = 0; i < ARCH_1_UVS; i++) {
        VARS(work)->object->model->meshsets[ARCH_1_MESHSET].vertuv[i].tex.v +=
            SCROLL(work);
    }
}

void arch_1_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, PROP_RANGE)) {
        return;
    }
    if (stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 0) ||
        IN_STAGE(STAGE_CART)) {
        if (edit_mode) {
            VARS(work)->object = &arch_1_object;
            SCROLL_SPEED(work) = (Uint8)floorf(work->scl.x);
            arch_1_draw(task);
            if (placement_editing(task)) {
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> TEX SCROLL");
            }
        } else {
            switch (work->mode) {
            case PROP_INIT:
                work->mode = PROP_RUN;
                SCROLL_SPEED(work) = (Uint8)floorf(work->scl.x);
                VARS(work)->object = &arch_1_object;
                task->draw = prop_draw;
                task->destructor = prop_destroy;
                break;
            case PROP_RUN:
                arch_1_draw(task);
                break;
            }
        }
    }
}

/* as arch_1_draw, for O ARCH 2 and ARCH_2_MESHSET's uvs */
static void arch_2_draw(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 i;

    /* @hack spelling: the sum stored inside the test, which tests R0 as it
     * stores it (MOV.W R0,@(6,R14) at 0x0c914df6, EXTU.W R0,R0 at
     * 0x0c914df8); stored before the test and read back, 76 of 88 words and
     * 1 pool word differ */
    if ((SCROLL(work) += SCROLL_SPEED(work)) >= SCROLL_WRAP) {
        SCROLL(work) = 0;
    }
    for (i = 0; i < ARCH_2_UVS; i++) {
        VARS(work)->object->model->meshsets[ARCH_2_MESHSET].vertuv[i].tex.v -=
            SCROLL(work);
    }
    prop_draw(task);
    for (i = 0; i < ARCH_2_UVS; i++) {
        VARS(work)->object->model->meshsets[ARCH_2_MESHSET].vertuv[i].tex.v +=
            SCROLL(work);
    }
}

/* draw a spotlight's sub arch, and its lamp turned by VARS(work)->turn. the
 * lamp turns about z on a lone spotlight, and about x on one of a pair */
static void spotlight_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;
    Angle ang;

    njSetTexture(&obj_shareobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(object->model);
    object = object->child;
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    if (work->flags == SPOTLIGHT_ALONE) {
        TURN_VIA(njRotateZ, VARS(work)->turn, ang);
    } else {
        TURN_VIA(njRotateX, VARS(work)->turn, ang);
    }
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* rock the spotlight SPOTLIGHT_SPEED a step, as far as its arch's scale z in
 * degrees either way. then draw it. SPOTLIGHT_MINUS_X rocks the mirror
 * way */
static void spotlight_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case PROP_INIT:
        work->mode = PROP_RUN;
        VARS(work)->limit =
            (Angle)(task->parent->work->scl.z * ANGLE_PER_DEGREE);
        if (work->flags == SPOTLIGHT_MINUS_X) {
            VARS(work)->limit = (Uint16)-VARS(work)->limit;
        }
        VARS(work)->object = spotlight_object;
        task->draw = prop_draw;
        task->destructor = prop_destroy;
        break;
    case PROP_RUN:
        switch (work->submode) {
        case SWING_OUT:
            VARS(work)->turn = angle_approach(
                VARS(work)->turn, VARS(work)->limit, SPOTLIGHT_SPEED);
            if (angle_distance(VARS(work)->turn, VARS(work)->limit) <
                SPOTLIGHT_SPEED) {
                work->submode = SWING_BACK;
            }
            break;
        case SWING_BACK:
            VARS(work)->turn = angle_approach(
                VARS(work)->turn, -VARS(work)->limit, SPOTLIGHT_SPEED);
            if (angle_distance(VARS(work)->turn, -VARS(work)->limit) <
                SPOTLIGHT_SPEED) {
                work->submode = SWING_OUT;
            }
            break;
        }
        spotlight_draw(task);
        break;
    }
}

void arch_2_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Angle ang;

    if (placement_try_leave_by(task, PROP_RANGE)) {
        return;
    }
    if (stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 0) ||
        IN_STAGE(STAGE_CART)) {
        if (edit_mode) {
            VARS(work)->object = &arch_2_object;
            SCROLL_SPEED(work) = (Uint8)floorf(work->scl.x);
            arch_2_draw(task);
            if (work->scl.y > 10.0F) {
                njSetTexture(&obj_shareobj_texlist);
                njPushMatrix(NULL);
                njTranslateV(NULL, &work->pos);
                TURN_VIA(njRotateZ, work->ang.z, ang);
                TURN_VIA(njRotateX, work->ang.x, ang);
                TURN_VIA(njRotateY, work->ang.y, ang);
                njTranslate(NULL, 18.0F, 17.5F, 0.0F);
                object = spotlight_object;
                draw_model(object->model);
                object = object->child;
                njPushMatrix(NULL);
                njTranslateV(NULL, (NJS_VECTOR *)object->pos);
                TURN_VIA(njRotateX, -(work->scl.z * ANGLE_PER_DEGREE), ang);
                draw_model(object->model);
                njPopMatrix(1);
                njTranslate(NULL, -36.0F, 0.0F, 0.0F);
                object = spotlight_object;
                draw_model(object->model);
                object = object->child;
                njPushMatrix(NULL);
                njTranslateV(NULL, (NJS_VECTOR *)object->pos);
                TURN_VIA(njRotateX, work->scl.z * ANGLE_PER_DEGREE, ang);
                draw_model(object->model);
                njPopMatrix(1);
                njPopMatrix(1);
            } else if (work->scl.y > 0.0F) {
                njSetTexture(&obj_shareobj_texlist);
                njPushMatrix(NULL);
                njTranslateV(NULL, &work->pos);
                TURN_VIA(njRotateZ, work->ang.z, ang);
                TURN_VIA(njRotateX, work->ang.x, ang);
                TURN_VIA(njRotateY, work->ang.y, ang);
                njTranslate(NULL, 0.0F, 17.5F, 0.0F);
                object = spotlight_object;
                draw_model(object->model);
                object = object->child;
                njPushMatrix(NULL);
                njTranslateV(NULL, (NJS_VECTOR *)object->pos);
                TURN_VIA(njRotateZ, work->scl.z * ANGLE_PER_DEGREE, ang);
                draw_model(object->model);
                njPopMatrix(1);
                njPopMatrix(1);
            }
            if (placement_editing(task)) {
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> TEX SCROLL");
                if (work->scl.y > 10.0F) {
                    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y),
                             "> W/ 2 SUB ARCHES");
                } else if (work->scl.y > 0.0F) {
                    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "> W/ 1 SUB ARCH");
                } else {
                    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "> W/O SUB ARCH");
                }
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "> SPOT LITE ROT");
            }
        } else {
            switch (work->mode) {
            case PROP_INIT:
                work->mode = PROP_RUN;
                SCROLL_SPEED(work) = (Uint8)floorf(work->scl.x);
                VARS(work)->object = &arch_2_object;
                task->draw = prop_draw;
                task->destructor = prop_destroy;
                if (work->scl.y > 10.0F) {
                    task_create_children(arch_two_spotlights, task);
                } else if (work->scl.y > 0.0F) {
                    task_create_children(arch_one_spotlight, task);
                }
                break;
            case PROP_RUN:
                task_step_children(task);
                arch_2_draw(task);
                break;
            }
        }
    }
}

void light_1_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, PROP_RANGE)) {
        return;
    }
    if (stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 0) ||
        IN_STAGE(STAGE_CART)) {
        if (edit_mode) {
            VARS(work)->object = light_1_object;
            prop_draw(task);
        } else {
            switch (work->mode) {
            case PROP_INIT:
                work->mode = PROP_RUN;
                VARS(work)->object = light_1_object;
                task->draw = prop_draw;
                task->destructor = prop_destroy;
                break;
            case PROP_RUN:
                prop_draw(task);
                break;
            }
        }
    }
}

void light_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, PROP_RANGE)) {
        return;
    }
    if (stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 0) ||
        IN_STAGE(STAGE_CART)) {
        if (edit_mode) {
            VARS(work)->object = light_2_object;
            prop_draw(task);
            if (placement_editing(task)) {
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "> Y ROT RATE");
                VARS(work)->turn = (Angle)(work->scl.y * ANGLE_PER_DEGREE);
                work->ang.y += VARS(work)->turn;
            }
        } else {
            switch (work->mode) {
            case PROP_INIT:
                work->mode = PROP_RUN;
                VARS(work)->turn = (Angle)(work->scl.y * ANGLE_PER_DEGREE);
                VARS(work)->object = light_2_object;
                task->draw = prop_draw;
                task->destructor = prop_destroy;
                break;
            case PROP_RUN:
                work->ang.y += VARS(work)->turn;
                prop_draw(task);
                break;
            }
        }
    }
}

/* on a frame's first step, advance the scroll and draw the pole. its
 * POLE_MESHSET uvs scroll in v the other way from an arch's, and go back
 * after the draw. its beam turns about x by its swing */
static void pole_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Uint16 i;
    Angle ang;

    if (frame_step != 0) {
        return;
    }
    /* @hack spelling: the sum stored inside the test, which tests R0 as it
     * stores it (MOV.W R0,@(6,R14) at 0x0c915706, EXTU.W R0,R0 at
     * 0x0c915708); stored before the test and read back, 169 of 190 words
     * differ */
    if ((SCROLL(work) += SCROLL_SPEED(work)) >= SCROLL_WRAP) {
        SCROLL(work) = 0;
    }
    for (i = 0; i < POLE_UVS; i++) {
        POLE_VARS(work)
            ->object->model->meshsets[POLE_MESHSET]
            .vertuv[i]
            .tex.v += SCROLL(work);
    }
    njSetTexture(&obj_shareobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    object = POLE_VARS(work)->object;
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    draw_model(object->model);
    njPushMatrix(NULL);
    object = object->child;
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    TURN_VIA(njRotateX, POLE_VARS(work)->beam[BEAM_SWING], ang);
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
    for (i = 0; i < POLE_UVS; i++) {
        POLE_VARS(work)
            ->object->model->meshsets[POLE_MESHSET]
            .vertuv[i]
            .tex.v -= SCROLL(work);
    }
}

void pole_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, POLE_RANGE)) {
        return;
    }
    if (stage_get_stage_act() == STAGE_ACT(STAGE_TWINKLE, 0) ||
        IN_STAGE(STAGE_CART)) {
        if (edit_mode) {
            POLE_VARS(work)->object = &pole_object;
            pole_draw(task);
            if (placement_editing(task)) {
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> TEX SCROLL");
                SCROLL_SPEED(work) = (Uint8)floorf(work->scl.x);
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "> BEAM X ROT");
                POLE_VARS(work)->beam[BEAM_SWING] = (Sint16)floorf(work->scl.y);
            }
        } else {
            switch (work->mode) {
            case PROP_INIT:
                work->mode = PROP_RUN;
                SCROLL_SPEED(work) = (Uint8)floorf(work->scl.x);
                POLE_VARS(work)->beam[BEAM_LIMIT] =
                    (Sint16)(work->scl.y * ANGLE_PER_DEGREE);
                POLE_VARS(work)->object = &pole_object;
                task->draw = prop_draw;
                task->destructor = prop_destroy;
                break;
            case PROP_RUN:
                switch (work->submode) {
                case SWING_OUT:
                    POLE_VARS(work)->beam[BEAM_SWING] += SCROLL_SPEED(work);
                    if (POLE_VARS(work)->beam[BEAM_SWING] >
                        POLE_VARS(work)->beam[BEAM_LIMIT]) {
                        POLE_VARS(work)->beam[BEAM_SWING] =
                            POLE_VARS(work)->beam[BEAM_LIMIT];
                        work->submode = SWING_BACK;
                    }
                    break;
                case SWING_BACK:
                    POLE_VARS(work)->beam[BEAM_SWING] -= SCROLL_SPEED(work);
                    if (POLE_VARS(work)->beam[BEAM_SWING] <
                        -POLE_VARS(work)->beam[BEAM_LIMIT]) {
                        POLE_VARS(work)->beam[BEAM_SWING] =
                            -POLE_VARS(work)->beam[BEAM_LIMIT];
                        work->submode = SWING_OUT;
                    }
                    break;
                }
                pole_draw(task);
                break;
            }
        }
    }
}

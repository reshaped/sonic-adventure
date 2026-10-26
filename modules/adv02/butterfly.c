/* the butterfly the object list names O BUTTERFLY: one of four kinds, which
 * flutters about the point where it was placed */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv02/butterfly.h"
#include "adv02/texlists.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MOTION butterfly_motion;
extern NJS_OBJECT butterfly_1_object;
extern NJS_OBJECT butterfly_2_object;
extern NJS_OBJECT butterfly_3_object;
extern NJS_OBJECT butterfly_4_object;

/* the square of the distance from the camera or player 0 within which the
 * butterfly stays */
#define BUTTERFLY_RANGE (500.0F * 500.0F)

#define KINDS 4

/* the butterfly's target height swings SWING above and below home's y. its
 * y stays within RISE of that height, and its x and z within ROAM of
 * home's */
#define ROAM 80.0F
#define RISE 12.0F
#define SWING 3.2F

#define GRAVITY (98.0F / 3600.0F)

/* the angle about x that the model is tipped by as it draws */
#define TIP 0x2f00

enum butterfly_mode { BUTTERFLY_INIT, BUTTERFLY_RUN };

/* the wing stroke, in work->submode */
enum butterfly_flap { FLAP_DOWN, FLAP_UP };

/* how the butterfly moves along x and z: it hovers, flies ahead, or turns as
 * it flies */
enum butterfly_move { MOVE_HOVER, MOVE_AHEAD, MOVE_TURN };

struct butterfly_flight {
    NJS_POINT3 home;
    NJS_VECTOR speed;
    Float flap_speed; /* the motion frames the wings play a step */
    /* the stroke's pull toward the target height, which speeds the flap
     * too */
    Float lift;
    Uint16 steps; /* left in the current move */
    Uint8 move;
    Uint8 turn_right;
};

/* work->timer holds the phase of the target height's swing, which
 * TASK_TIMER reads as an angle */
struct butterfly_vars {
    Uint8 unk_00[4];
    Float frame; /* the frame butterfly_motion draws at */
    /* from the heap: butterfly_init allocates it, and butterfly_destroy frees
     * it */
    struct butterfly_flight *flight;
};

#define VARS(work) ((struct butterfly_vars *)&(work)->vars)
#define KIND(work) ((work)->tick)

struct butterfly_kind {
    void *unk_00; /* null in every kind */
    NJS_OBJECT *object;
};

static void butterfly_init(struct task *task);
static void butterfly_reset(struct task *task);
static void butterfly_update(struct task *task);
static void butterfly_draw(struct task *task);
static void butterfly_destroy(struct task *task);

static struct butterfly_kind butterfly_kinds[KINDS] = {
    {NULL, &butterfly_1_object},
    {NULL, &butterfly_2_object},
    {NULL, &butterfly_3_object},
    {NULL, &butterfly_4_object},
};

/* the butterflies placed and not yet destroyed */
static int butterfly_count = 0;

static Float butterfly_up_drag = 0.5F;
static Float butterfly_down_push = 1.1F;
static Float butterfly_lift_most = 4.0F;
static Float butterfly_lift_least = 0.1F;
static Float butterfly_down_gain = 5.0F;
static Float butterfly_up_gain = 10.0F;
static Float butterfly_flap_lift = 0.2F;
static Float butterfly_flap_base = 0.7F;

/* while the set editor edits a butterfly, the butterfly starts over each
 * step, from the kind and home the editor leaves in its work */
void butterfly_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, BUTTERFLY_RANGE)) {
        if (!placement_editing(task)) {
            switch (work->mode) {
            case BUTTERFLY_INIT:
                butterfly_init(task);
                break;
            case BUTTERFLY_RUN:
                butterfly_draw(task);
                butterfly_update(task);
                break;
            }
        } else {
            switch (work->mode) {
            case BUTTERFLY_INIT:
                butterfly_init(task);
                break;
            case BUTTERFLY_RUN:
                butterfly_reset(task);
                butterfly_draw(task);
                butterfly_update(task);
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
                debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> KIND (0-%d)",
                            KINDS - 1);
                break;
            }
        }
    }
}

static void butterfly_init(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    /* the first butterfly placed runs a loop over the kinds that does nothing
     * (0x0c90cf78 to 0x0c90cf7e) */
    if (butterfly_count == 0) {
        for (i = 0; i < KINDS; i++) {
        }
    }
    butterfly_count++;
    task->draw = butterfly_draw;
    task->destructor = butterfly_destroy;
    VARS(work)->flight = heap_alloc(sizeof(struct butterfly_flight));
    VARS(work)->frame = 0.0F;
    work->submode = FLAP_DOWN;
    butterfly_reset(task);
    work->mode = BUTTERFLY_RUN;
}

/* scl.x picks the kind, and a kind past the last becomes the last. the
 * butterfly then hovers from rest, with pos its home */
static void butterfly_reset(struct task *task)
{
    struct task_work *work = task->work;
    struct butterfly_flight *flight = VARS(work)->flight;

    KIND(work) = (int)work->scl.x;
    if (KIND(work) >= KINDS) {
        KIND(work) = KINDS - 1;
    }
    flight->speed.x = 0.0F;
    flight->speed.y = 0.0F;
    flight->speed.z = 0.0F;
    flight->flap_speed = 1.0F;
    flight->lift = 0.0F;
    flight->home = work->pos;
    flight->move = MOVE_HOVER;
    flight->steps = 30;
    work->timer = 0;
}

static void butterfly_update(struct task *task)
{
    struct task_work *work = task->work;
    struct butterfly_flight *flight = VARS(work)->flight;
    NJS_MOTION *motion;
    Float height;
    Bool strayed = FALSE;
    NJS_VECTOR push;
    NJS_VECTOR turned;
    Angle ang;

    height = flight->home.y + (njSin(TASK_TIMER(work)) * SWING);
    if (work->submode == FLAP_DOWN) {
        flight->lift = butterfly_down_gain * (height - work->pos.y);
        if (flight->lift < butterfly_lift_least) {
            flight->lift = butterfly_lift_least;
        } else if (flight->lift > butterfly_lift_most) {
            flight->lift = butterfly_lift_most;
        }
        flight->flap_speed =
            butterfly_flap_base + (flight->lift * butterfly_flap_lift);
        flight->speed.y += flight->lift * butterfly_down_push * GRAVITY;
        flight->speed.y -= GRAVITY;
        VARS(work)->frame += flight->flap_speed;
        motion = &butterfly_motion;
        if (VARS(work)->frame >= motion->nbFrame - 1.0F) {
            VARS(work)->frame = motion->nbFrame - 1.0F;
            work->submode = FLAP_UP;
        }
    } else {
        flight->lift = -(butterfly_up_gain * (height - work->pos.y));
        if (flight->lift < butterfly_lift_least) {
            flight->lift = butterfly_lift_least;
        } else if (flight->lift > butterfly_lift_most) {
            flight->lift = butterfly_lift_most;
        }
        flight->speed.y -= flight->lift * butterfly_up_drag * GRAVITY;
        flight->speed.y -= GRAVITY;
        flight->flap_speed =
            butterfly_flap_base + (flight->lift * butterfly_flap_lift);
        VARS(work)->frame -= flight->flap_speed;
        if (VARS(work)->frame < 0.0F) {
            VARS(work)->frame = 0.0F;
            work->submode = FLAP_DOWN;
        }
    }
    switch (flight->move) {
    case MOVE_HOVER:
        flight->speed.x *= 0.7F;
        flight->speed.z *= 0.7F;
        break;
    case MOVE_AHEAD:
        if (work->submode == FLAP_DOWN) {
            push.z = (rand() * RAND_SCALE * 0.025F) + 0.05F;
            push.x = ((rand() * RAND_SCALE * 2.0F) - 1.0F) * 0.025F;
            push.y = 0.0F;
            njPushMatrix(&_unit_matrix_);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njCalcVector(NULL, &push, &turned);
            njPopMatrix(1);
            flight->speed.x += turned.x;
            flight->speed.z += turned.z;
            flight->speed.x *= 0.9F;
            flight->speed.z *= 0.9F;
        } else {
            flight->speed.x *= 0.75F;
            flight->speed.z *= 0.78F;
        }
        break;
    case MOVE_TURN:
        if (flight->turn_right) {
            work->ang.y += (rand() * RAND_SCALE * 200.0F) + 200.0F;
        } else {
            work->ang.y -= (rand() * RAND_SCALE * 200.0F) + 200.0F;
        }
        if (work->submode == FLAP_DOWN) {
            push.z = (rand() * RAND_SCALE * 0.025F) + 0.05F;
            push.x = ((rand() * RAND_SCALE * 2.0F) - 1.0F) * 0.025F;
            push.y = 0.0F;
            njPushMatrix(&_unit_matrix_);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njCalcVector(NULL, &push, &turned);
            njPopMatrix(1);
            flight->speed.x += turned.x;
            flight->speed.z += turned.z;
            flight->speed.x *= 0.85F;
            flight->speed.z *= 0.85F;
        } else {
            flight->speed.x *= 0.75F;
            flight->speed.z *= 0.75F;
        }
        break;
    }
    njAddVector(&work->pos, &flight->speed);
    flight->speed.y *= 0.9F;
    if (work->pos.y > height + RISE) {
        work->pos.y = height + RISE;
    } else if (work->pos.y < height - RISE) {
        work->pos.y = height - RISE;
    }
    if (work->pos.x > flight->home.x + ROAM) {
        work->pos.x = flight->home.x + ROAM;
        strayed = TRUE;
    } else if (work->pos.x < flight->home.x - ROAM) {
        work->pos.x = flight->home.x - ROAM;
        strayed = TRUE;
    }
    if (work->pos.z > flight->home.z + ROAM) {
        work->pos.z = flight->home.z + ROAM;
        strayed = TRUE;
    } else if (work->pos.z < flight->home.z - ROAM) {
        work->pos.z = flight->home.z - ROAM;
        strayed = TRUE;
    }
    if (strayed && flight->move != MOVE_TURN) {
        flight->move = MOVE_TURN;
        flight->steps = (int)((rand() * RAND_SCALE * 20.0F) + 50.0F);
        if (rand() * RAND_SCALE < 0.5F) {
            flight->turn_right = FALSE;
        } else {
            flight->turn_right = TRUE;
        }
    }
    if (flight->steps) {
        flight->steps--;
    } else {
        switch (flight->move) {
        case MOVE_HOVER:
            flight->move = MOVE_AHEAD;
            flight->steps = (int)((rand() * RAND_SCALE * 20.0F) + 20.0F);
            break;
        case MOVE_AHEAD:
            if (rand() * RAND_SCALE < 0.5F) {
                flight->move = MOVE_TURN;
                flight->steps = (int)((rand() * RAND_SCALE * 50.0F) + 50.0F);
                if (rand() * RAND_SCALE < 0.5F) {
                    flight->turn_right = FALSE;
                } else {
                    flight->turn_right = TRUE;
                }
            } else {
                flight->move = MOVE_HOVER;
                flight->steps = (int)((rand() * RAND_SCALE * 30.0F) + 30.0F);
            }
            break;
        case MOVE_TURN:
            if (rand() * RAND_SCALE < 0.4F) {
                flight->move = MOVE_TURN;
                flight->steps = (int)((rand() * RAND_SCALE * 50.0F) + 50.0F);
                if (rand() * RAND_SCALE < 0.5F) {
                    flight->turn_right = FALSE;
                } else {
                    flight->turn_right = TRUE;
                }
            } else {
                flight->move = MOVE_AHEAD;
                flight->steps = (int)((rand() * RAND_SCALE * 30.0F) + 30.0F);
            }
            break;
        }
    }
    work->timer += (int)((rand() * RAND_SCALE * 40.0F) + 160.0F);
}

static void butterfly_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njRotateX(NULL, TIP);
    draw_motion(butterfly_kinds[KIND(work)].object, &butterfly_motion,
                VARS(work)->frame);
    njPopMatrix(1);
}

static void butterfly_destroy(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    /* the last butterfly destroyed runs the same loop (0x0c90d74e to
     * 0x0c90d754) */
    butterfly_count--;
    if (butterfly_count == 0) {
        for (i = 0; i < KINDS; i++) {
        }
    }
    heap_free(VARS(work)->flight);
}

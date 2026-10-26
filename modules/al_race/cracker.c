/* CRACKER, a party cracker that fires confetti, and streamers too, while the
 * crackers are on */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_race/confetti.h"
#include "al_race/cracker.h"
#include "al_race/game.h"
#include "al_race/streamer.h"
#include "al_race/texlists.h"
#include "chaos/chaos.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the cracker's object */
extern NJS_OBJECT dat_0cbc6f40;

/* what the crackers fire, by race_cracker_kind's result: nothing, confetti, or
 * streamers and confetti */
enum cracker_kind { CRACKER_NONE, CRACKER_CONFETTI, CRACKER_STREAMERS };

/* a cracker's modes. CRACKER_START waits a step, and CRACKER_OFF waits for
 * the crackers to turn on. CRACKER_WAIT counts the timer down to the next
 * shot, CRACKER_FIRE fires it, and CRACKER_RELOAD sets the timer again */
enum cracker_mode {
    CRACKER_START,
    CRACKER_OFF,
    CRACKER_WAIT,
    CRACKER_FIRE,
    CRACKER_RELOAD
};

/* a cracker's set entry gives the speed it fires at in scale x, POWER, and
 * the confetti pieces it fires in scale y, COUNT */
#define POWER(work) ((work)->scl.x)
#define COUNT(work) ((work)->scl.y)

/* steps between bursts in the set editor */
#define EDITOR_WAIT 60

static void cracker_destroy(struct task *task);
static void cracker_draw_aim(struct task *task);
static void cracker_draw(struct task *task);
static void cracker_fire_confetti(struct task *task);
static void cracker_fire_streamers(struct task *task);
static void cracker_step(struct task *task);

static enum cracker_kind cracker_kind;

/* TRUE while the crackers are on */
static Bool cracker_flag = FALSE;
/* the direction a cracker fires and its mouth, in its unturned frame */
static NJS_VECTOR cracker_way = {1.0F, 0.0F, 0.0F};
static NJS_VECTOR cracker_mouth = {3.21F, 0.0F, 0.0F};

static void cracker_reset(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = CRACKER_START;
}

void cracker_init(struct task *task)
{
    if (placement_editing(task)) {
        if (frame_step == 0) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "Power");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "Num");
        }
        if (++TASK_TIMER(task->work) >= EDITOR_WAIT) {
            task->work->timer = 0;
            cracker_fire_confetti(task);
        }
        cracker_reset(task);
        cracker_draw(task);
    } else {
        cracker_reset(task);
        task->step = cracker_step;
        task->draw = cracker_draw;
        task->destructor = cracker_destroy;
    }
}

void cracker_stop(void)
{
    cracker_flag = FALSE;
}

void cracker_start(void)
{
    cracker_flag = TRUE;
    cracker_kind = race_cracker_kind();
}

Bool cracker_on(void)
{
    return cracker_flag;
}

static void cracker_destroy(struct task *task)
{
}

static void cracker_draw_aim(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3COL line;
    NJS_POINT3 points[2];
    NJS_COLOR colors[2];
    NJS_VECTOR mouth;
    NJS_VECTOR way;
    Angle ang;
    Float scale;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    chaos_draw_axes(2.0F, NULL);
    njPopMatrix(1);
    line.p = points;
    line.col = colors;
    line.tex = NULL;
    line.num = 2;
    points[0].x = work->pos.x;
    points[0].y = work->pos.y;
    points[0].z = work->pos.z;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    njCalcVector(NULL, &cracker_mouth, &mouth);
    scale = POWER(work) * 5.0F;
    njScale(NULL, scale, scale, scale);
    njCalcVector(NULL, &cracker_way, &way);
    njPopMatrix(1);
    /* @hack spelling: mouth's and way's members through their addresses, which
     * SHC computes again for each use, R15 plus 16 and plus 4 (0x0cb968dc to
     * 0x0cb96946), as the image does. as plain members, the function runs 28
     * bytes short and 87 of 167 words differ */
    (&mouth)->x = (&mouth)->x + work->pos.x;
    (&mouth)->y = (&mouth)->y + work->pos.y;
    (&mouth)->z = (&mouth)->z + work->pos.z;
    points[1].x = (&way)->x + (&mouth)->x;
    points[1].y = (&way)->y + (&mouth)->y;
    points[1].z = (&way)->z + (&mouth)->z;
    colors[0].color = colors[1].color = 0xffff0000;
    njDrawLine3D(&line, 1, NJD_DRAW_NORMAL);
}

static void cracker_draw(struct task *task)
{
    struct task_work *work;
    Angle ang;

    if (frame_step == 0) {
        work = task->work;
        if (placement_editing(task)) {
            cracker_draw_aim(task);
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        njSetTexture(&dat_0cbc089c);
        draw_tree(&dat_0cbc6f40);
        njPopMatrix(1);
    }
}

static void cracker_fire_confetti(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR mouth;
    NJS_VECTOR way;
    Angle ang;
    Float scale;

    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    njCalcVector(NULL, &cracker_mouth, &mouth);
    scale = POWER(work);
    njScale(NULL, scale, scale, scale);
    njCalcVector(NULL, &cracker_way, &way);
    njPopMatrix(1);
    /* @hack spelling: mouth's members through &mouth, whose address SHC
     * computes again for each use, R15 plus 16 (0x0cb96ac0 to 0x0cb96af4), as
     * the image does. as plain members, 43 of 92 words differ */
    (&mouth)->x = (&mouth)->x + work->pos.x;
    (&mouth)->y = (&mouth)->y + work->pos.y;
    (&mouth)->z = (&mouth)->z + work->pos.z;
    confetti_create(&mouth, &way, (int)COUNT(work));
}

static void cracker_fire_streamers(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR mouth;
    NJS_VECTOR way;
    Angle ang;
    Float scale;

    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    njCalcVector(NULL, &cracker_mouth, &mouth);
    scale = POWER(work);
    njScale(NULL, scale, scale, scale);
    njCalcVector(NULL, &cracker_way, &way);
    njPopMatrix(1);
    /* @hack spelling: mouth's members through &mouth, whose address SHC
     * computes again for each use, R15 plus 16 (0x0cb96b78 to 0x0cb96bb2), as
     * the image does. as plain members, 39 of 88 words differ */
    (&mouth)->x = (&mouth)->x + work->pos.x;
    (&mouth)->y = (&mouth)->y + work->pos.y;
    (&mouth)->z = (&mouth)->z + work->pos.z;
    streamer_create(&mouth, &way);
}

/* a cracker fires 2 to 61 steps after the step that finds the crackers on,
 * then every 92 to 121 steps while they stay on. once they turn off, it goes
 * back to CRACKER_START */
static void cracker_step(struct task *task)
{
    struct task_work *work = task->work;
    Bool *on = &cracker_flag;

    if (work->mode >= CRACKER_WAIT && !*on) {
        work->mode = CRACKER_START;
    }
    switch (work->mode) {
    case CRACKER_START:
        ++work->mode;
        break;
    case CRACKER_OFF:
        if (*on) {
            TASK_TIMER(work) = (rand() % 60) + 1;
            ++work->mode;
        }
        break;
    case CRACKER_WAIT:
        if (--TASK_TIMER(work) <= 0) {
            ++work->mode;
        }
        break;
    case CRACKER_FIRE:
        switch (cracker_kind) {
        case CRACKER_NONE:
            break;
        case CRACKER_STREAMERS:
            cracker_fire_streamers(task);
            /* fallthrough */
        case CRACKER_CONFETTI:
            cracker_fire_confetti(task);
        }
        ++work->mode;
        break;
    case CRACKER_RELOAD:
        TASK_TIMER(work) = (rand() % 30) + 90;
        work->mode = CRACKER_WAIT;
        break;
    }
    cracker_draw(task);
}

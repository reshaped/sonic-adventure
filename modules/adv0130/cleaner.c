/* O OSOUJI, a cleaning robot that drives round a rectangle with rounded
 * corners, splashing water as it goes. it stops now and then to scrub */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv0130/cleaner.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "effect/dust.h"
#include "effect/splash.h"
#include "event/play.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION cleaner_action;

/* the round the cleaner drives, from its set entry. it starts at x and z,
 * and runs length along z, then width along -x. it then runs back along z
 * and along x. each corner is a quarter circle of radius, and ang turns the
 * whole round about y */
struct cleaner_round {
    Float x;
    Float z;
    Float width;
    Float length;
    Float radius;
    Angle ang;
};

struct cleaner_vars {
    Float frame; /* the action's frame, which only a scrub moves */
    Float along; /* how far the cleaner is along its part of the round */
    struct cleaner_round *round; /* from heap_alloc; cleaner_destroy frees it */
};

#define VARS(work) ((struct cleaner_vars *)&(work)->vars)
/* the part of the round the cleaner is on, 0 to 7, in the order it drives
 * them. the even parts are sides, and the odd parts corners */
#define PART(work) ((work)->submode)
/* the splashes so far. cleaner_run calls cleaner_dust at every third */
#define SPLASHES(work) ((work)->tick)

/* how far the cleaner drives along its round a step, and the action's frames
 * a step as it scrubs */
#define CLEANER_SPEED 0.4F
#define SCRUB_SPEED 0.6F

/* the steps the cleaner drives between stops, and the times it plays the
 * action through at each */
#define GO_STEPS 30
#define CLEANER_SCRUBS 3

/* a corner is CORNER_ARC times the radius long, a quarter circle's arc, and
 * at least CORNER_LEAST */
#define CORNER_ARC 1.57F
#define CORNER_LEAST 16.0F

struct cleaner_dust {
    Sint32 unk_00;
    struct dust_params params;
};

enum cleaner_mode {
    CLEANER_EDIT, /* while the set editor holds the task */
    CLEANER_GO_ROUND,
    CLEANER_SCRUB
};

static void cleaner_destroy(struct task *task);
static void cleaner_draw(struct task *task);
static void cleaner_take_round(struct task *task);
static void cleaner_draw_round(struct task *task);
static Float cleaner_part_length(struct task *task);
static void cleaner_go(struct task *task);
static void cleaner_scrub(struct task *task);
static void cleaner_splash(struct task *task);
static void cleaner_dust(struct task *task);
static void cleaner_run(struct task *task);
static void cleaner_step(struct task *task);

/* the body, and the arm reaching out ahead */
static struct collision_shape cleaner_shapes[] = {
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 12.0F, 0.0F},
     8.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {4.0F, 6.0F, 12.0F},
     4.0F,
     6.0F,
     0.0F,
     {0x4000, 0, 0},
     0,
     NULL},
};

static void cleaner_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(VARS(work)->round);
}

static void cleaner_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_action(&cleaner_action, VARS(work)->frame);
    njPopMatrix(1);
}

static void cleaner_take_round(struct task *task)
{
    struct task_work *work = task->work;
    struct cleaner_round *round = VARS(work)->round;

    round->x = work->pos.x;
    round->z = work->pos.z;
    round->width = work->scl.x;
    round->length = work->scl.z;
    round->radius = work->scl.y;
    round->ang = work->ang.y;
    if (round->width < 0.0F) {
        round->width = 0.0F;
    }
    if (round->length < 0.0F) {
        round->length = 0.0F;
    }
    if (round->radius < 0.0F) {
        round->radius = 0.0F;
    }
}

/* draw the round the set entry gives, as the set editor changes it. label
 * the editor's rows for the round's heading, width, radius and length */
static void cleaner_draw_round(struct task *task)
{
    struct task_work *work = task->work;
    struct cleaner_round *round = VARS(work)->round;
    NJS_POINT3 points[36];
    NJS_POINT3COL line;
    NJS_POINT3 ends[2];
    NJS_COLOR colors[2];
    int i;
    Angle ang;

    cleaner_take_round(task);
    for (i = 0; i < 9; i++) {
        points[i].x = round->radius * njCos(i * 2048);
        points[i].z = (round->radius * njSin(i * 2048)) + round->length;
    }
    for (i = 9; i < 18; i++) {
        points[i].x = (round->radius * njCos((i - 1) * 2048)) - round->width;
        points[i].z = (round->radius * njSin((i - 1) * 2048)) + round->length;
    }
    for (i = 18; i < 27; i++) {
        points[i].x = (round->radius * njCos((i - 2) * 2048)) - round->width;
        points[i].z = round->radius * njSin((i - 2) * 2048);
    }
    for (i = 27; i < 36; i++) {
        points[i].x = round->radius * njCos((i - 3) * 2048);
        points[i].z = round->radius * njSin((i - 3) * 2048);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, -round->radius, 0.0F, 0.0F);
    colors[0].color = colors[1].color = 0x00ff0000;
    line.p = ends;
    line.col = colors;
    for (i = 0; i < 36; i++) {
        ends[0] = points[i];
        ends[1] = points[i == 35 ? 0 : i + 1];
        ends[0].y = ends[1].y = 0.1F;
        njDrawLine3D(&line, 1, 0);
    }
    njPopMatrix(1);
    njPrintSize(16);
    njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "<PATH MUKI>");
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<PATH YOKO>");
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<PATH HANKEI>");
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<PATH TATE>");
}

/* the length of the cleaner's part of the round. sides 0 and 4 are length
 * long, and sides 2 and 6 width long */
static Float cleaner_part_length(struct task *task)
{
    struct task_work *work = task->work;
    struct cleaner_round *round = VARS(work)->round;
    Float arc = round->radius * CORNER_ARC;

    /* @hack spelling: the length as one return of nested selects in place
     * of ifs, which copies the part into R6 and tests it with TST R6,R3
     * (0x0c926bf0, 0x0c926bf2); as ifs, R0 holds it for TST #1,R0, and 26
     * of 29 words differ */
    /* NOLINTBEGIN(readability-avoid-nested-conditional-operator) */
    return (PART(work) & 1) ? (arc < CORNER_LEAST ? CORNER_LEAST : arc)
                            : ((PART(work) & 2) ? round->width : round->length);
    /* NOLINTEND(readability-avoid-nested-conditional-operator) */
}

/* @hack spelling: cleaner_set_mode expanded at each call, which reads
 * task->work again there, as at 0x0c926eb4; called, summed over cleaner_go,
 * cleaner_scrub and egg_carrier_cleaner_step, 199 of 345 words differ */
#pragma inline(cleaner_set_mode) /* NOLINT(clang-diagnostic-unknown-pragmas)   \
                                  */
/* set the mode, and in work->timer the count it runs down */
static void cleaner_set_mode(struct task *task, int mode, int count)
{
    struct task_work *work = task->work;

    work->mode = mode;
    TASK_TIMER(work) = count;
}

/* drive the cleaner CLEANER_SPEED along its round, facing the way it goes.
 * it stops to scrub once work->timer runs down */
static void cleaner_go(struct task *task)
{
    struct task_work *work = task->work;
    struct cleaner_round *round = VARS(work)->round;
    Float length = cleaner_part_length(task);
    Float x;
    Float z;
    Float cos;
    Float sin;
    int ang;

    VARS(work)->along += CLEANER_SPEED;
    while (VARS(work)->along >= length) {
        VARS(work)->along -= length;
        PART(work) = (PART(work) + 1) & 7;
        length = cleaner_part_length(task);
    }
    switch (PART(work)) {
    case 0:
        ang = 0;
        x = 0.0F;
        z = VARS(work)->along;
        break;
    case 1:
        ang = (Angle)(VARS(work)->along * 16384.0F / length);
        x = round->radius * (njCos(ang) - 1.0F);
        z = round->length + (round->radius * njSin(ang));
        break;
    case 2:
        x = -round->radius - VARS(work)->along;
        z = round->length + round->radius;
        ang = 0x4000;
        break;
    case 3:
        ang = (Angle)(((VARS(work)->along / length) + 1.0F) * 16384.0F);
        x = -round->width + (round->radius * (njCos(ang) - 1.0F));
        z = round->length + (round->radius * njSin(ang));
        break;
    case 4:
        x = -round->width - (2.0F * round->radius);
        z = round->length - VARS(work)->along;
        ang = 0x8000;
        break;
    case 5:
        ang = (Angle)(((VARS(work)->along / length) + 2.0F) * 16384.0F);
        x = -round->width + (round->radius * (njCos(ang) - 1.0F));
        z = round->radius * njSin(ang);
        break;
    case 6:
        x = -round->width - round->radius + VARS(work)->along;
        z = -round->radius;
        ang = 0xc000;
        break;
    case 7:
        ang = (Angle)(((VARS(work)->along / length) + 3.0F) * 16384.0F);
        /* @bug logic: of the four corners, part 7's alone truncates x
         * toward 0, to a whole number (FTRC and FLOAT at 0x0c926dfa and
         * 0x0c926e00). on it, the cleaner's place across the round moves a
         * whole unit at a time */
        x = (int)(round->radius * (njCos(ang) - 1.0F));
        z = round->radius * njSin(ang);
        break;
    default:
        ang = 0;
        x = z = 0.0F;
        break;
    }
    cos = njCos(round->ang);
    sin = njSin(round->ang);
    work->pos.z = round->z + (z * cos) - (x * sin);
    work->pos.x = round->x + (x * cos) + (z * sin);
    work->ang.y = round->ang - ang;
    if (--TASK_TIMER(work) == 0) {
        cleaner_set_mode(task, CLEANER_SCRUB, CLEANER_SCRUBS);
    }
}

static void cleaner_scrub(struct task *task)
{
    struct task_work *work = task->work;
    Float last = cleaner_action.motion->nbFrame - 1;

    VARS(work)->frame += SCRUB_SPEED;
    if (VARS(work)->frame > last) {
        VARS(work)->frame = 0.0F;
        if (--TASK_TIMER(work) == 0) {
            cleaner_set_mode(task, CLEANER_GO_ROUND, GO_STEPS);
        }
    }
}

/* splash water from ahead of the cleaner. the water flies forward until
 * the action passes its middle, and back after */
static void cleaner_splash(struct task *task)
{
    struct task_work *work = task->work;
    Float last = cleaner_action.motion->nbFrame - 1;
    Float cos = njCos(work->ang.y);
    Float sin = njSin(work->ang.y);
    /* @hack redundant: ahead's spread scaled by 1, an FMAC by the 1 FLDI1
     * loads (0x0c926f1a, 0x0c926f1e); without it, 85 of 122 words differ */
    Float ahead = 18.0F + (1.0F * ((rand() * (2.0F * RAND_SCALE)) - 1.0F));
    Float aside = 4.0F + (4.0F * ((rand() * (2.0F * RAND_SCALE)) - 1.0F));
    Float speed;
    NJS_POINT3 pos;
    NJS_VECTOR velocity;

    if (VARS(work)->frame * 2.0F > last) {
        ahead += 1.0F;
    } else {
        ahead -= 1.0F;
    }
    pos.z = work->pos.z + (ahead * cos) - (aside * sin);
    pos.x = work->pos.x + (aside * cos) + (ahead * sin);
    speed = 0.8F;
    if (VARS(work)->frame * 2.0F > last) {
        speed = -0.8F;
    }
    velocity.z = cos * speed;
    velocity.x = sin * speed;
    velocity.y = 0.0F;
    pos.y = work->pos.y;
    splash_create(&pos, &velocity, 0.1F);
}

/* fill in a puff of dust at the cleaner's feet. nothing creates it, so no
 * dust shows */
static void cleaner_dust(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: dust, filled in and never read, which the image
     * copies to the stack frame and moves to the cleaner (0x0c927016 to
     * 0x0c92707c). without it, count unmeasured: the object's P ends 0x80
     * bytes short of its row, and the link fails */
    struct cleaner_dust dust = {
        0,
        {0.2F,
         0.038F,
         0.19F,
         0.06F,
         0.025F,
         {0.0F, 2.0F, 0.0F},
         {0.0F, -0.1F, 0.0F},
         {1.0F, 1.0F, 1.0F, 1.0F}},
    };

    dust.params.pos.x += work->pos.x;
    dust.params.pos.y += work->pos.y;
    dust.params.pos.z += work->pos.z;
}

static void cleaner_run(struct task *task)
{
    struct task_work *work = task->work;

    cleaner_splash(task);
    if (SPLASHES(work) % 3 == 0) {
        cleaner_dust(task);
    }
    SPLASHES(work)++;
    collision_register(work);
}

static void cleaner_step(struct task *task)
{
    struct task_work *work = task->work;

    if (event_in_control()) {
        return;
    }
    if (placement_try_leave(task)) {
        return;
    }
    cleaner_draw(task);
    switch (work->mode) {
    case CLEANER_EDIT:
        cleaner_draw_round(task);
        break;
    case CLEANER_GO_ROUND:
        cleaner_go(task);
        cleaner_run(task);
        break;
    case CLEANER_SCRUB:
        cleaner_scrub(task);
        cleaner_run(task);
        break;
    }
}

void egg_carrier_cleaner_step(struct task *task)
{
    struct task_work *work = task->work;

    task->destructor = cleaner_destroy;
    task->step = cleaner_step;
    task->draw = cleaner_draw;
    VARS(work)->frame = 0.0F;
    VARS(work)->round = heap_alloc(sizeof(struct cleaner_round));
    SPLASHES(work) = 0;
    if (placement_editing(task)) {
        work->mode = CLEANER_EDIT;
    } else {
        cleaner_take_round(task);
        cleaner_set_mode(task, CLEANER_GO_ROUND, GO_STEPS);
        collision_init(task, cleaner_shapes, COLLISION_OBJECT, 1, 2);
    }
}

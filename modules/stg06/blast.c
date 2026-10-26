/* the blasts and bursts the cannons' shells land with. each is a ball that
 * hurts players, growing and then shrinking by its timer. a burst's ball is
 * half a blast's, and nothing draws it */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "effect/explosion.h"
#include "object/placement.h"
#include "stg06/blast.h"
#include "stg06/models.h"
#include "stg06/sparks.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT blast_ring_object;
extern NJS_OBJECT blast_pillar_object;
extern NJS_OBJECT blast_fireball_object;
extern NJS_OBJECT blast_cloud_object;
/* a quad, which nothing draws. sky_deck_blast_init blends it additively,
 * as it does the four models the blast draws */
extern NJS_OBJECT dat_0ca8bcd0;

/* the range placement_try_leave_by takes, squared: about 1500 */
#define BLAST_RANGE 2250100.0F
/* the ball by its timer: it starts to grow at BLAST_START, and is biggest
 * at BLAST_TOP. its shrink would end at BLAST_GONE, but the step its timer
 * passes BLAST_END kills it */
#define BLAST_START 3
#define BLAST_TOP 36
#define BLAST_END 53
#define BLAST_GONE 56
#define BLAST_VIEW 100.0F /* the radius draw_in_view tests, times the scale */
/* the ring grows to RING_SIZE times the blast and fades over RING_STEPS.
 * the pillar stands up to PILLAR_HEIGHT times its model's height */
#define RING_STEPS 20
#define RING_SIZE 4.0F
#define PILLAR_HEIGHT 1.2F
/* how fast each model turns about y, in degrees a step */
#define RING_SPIN 40.0F
#define FIREBALL_SPIN 10.0F
#define PILLAR_SPIN 35.0F
#define CLOUD_SPIN 8.0F

/* a blast's own layout of task_work */
struct blast_work {
    Sint8 mode;
    Sint8 unk_01;
    Uint8 unk_02;
    Uint8 unk_03;
    Sint16 flags;
    Uint16 timer;
    /* how far the ball is through its growth and shrink. it is below 0 before
     * BLAST_START, 0.5 at BLAST_TOP and 1 at BLAST_GONE */
    Float phase;
    Float scale;
    Float size; /* from 0 up to 1 at BLAST_TOP, and back down */
    struct angles ang; /* x and z tilt the blast to lie on its floor */
    NJS_POINT3 pos;
    NJS_POINT3 scl;
    struct collision_work *collision;
    struct event_work *event;
};

enum blast_mode { BLAST_INIT, BLAST_RUN };

/* the blast's and the burst's balls at full size. each hurts players */
static struct collision_shape sky_deck_blast_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_UNTURNED,
    ATTACK(2) | GUARD(3) | SHAPE_HURTS_PLAYERS,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    20.0F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL};
static struct collision_shape sky_deck_burst_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_UNTURNED,
    ATTACK(2) | GUARD(3) | SHAPE_HURTS_PLAYERS,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    10.0F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL};

#define ADDITIVE(material)                                                     \
    ((material)->attrflags =                                                   \
         ((material)->attrflags & ~(NJD_SA_MASK | NJD_DA_MASK)) | NJD_SA_SRC | \
         NJD_DA_ONE | NJD_FLAG_USE_ALPHA)

static void sky_deck_blast_init(struct task *task);
static void sky_deck_blast_destroy(struct task *task);
static void sky_deck_blast_step(struct task *task);
static void sky_deck_blast_draw(struct task *task);
static void sky_deck_burst_init(struct task *task);
static void sky_deck_burst_destroy(struct task *task);
static void sky_deck_burst_step(struct task *task);

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (0x0c914c68); without it, task is kept at sp+0, 12 of 16 words
 * differ and sky_deck_blast_init starts 2 bytes late */
static void sky_deck_blast_start(register struct task *task)
{
    if (!placement_try_leave_by(task, BLAST_RANGE)) {
        sky_deck_blast_init(task);
    }
}

static void sky_deck_blast_init(struct task *task)
{
    struct blast_work *work = (struct blast_work *)task->work;

    if (!placement_editing(task)) {
        collision_init(task, &sky_deck_blast_shape, COLLISION_OBJECT, 1, 1);
    }
    task->draw = sky_deck_blast_draw;
    task->step = sky_deck_blast_step;
    task->destructor = sky_deck_blast_destroy;
    work->timer = 0;
    work->mode = BLAST_RUN;
    work->size = 1.0F;
    work->phase = 0.0F;
    ADDITIVE(blast_cloud_object.model->mats);
    ADDITIVE(blast_ring_object.model->mats);
    ADDITIVE(blast_pillar_object.model->mats);
    ADDITIVE(dat_0ca8bcd0.model->mats);
    ADDITIVE(blast_fireball_object.model->mats);
}

static void sky_deck_blast_destroy(struct task *task)
{
}

/* the shape that hurts players is a ball whose radius follows size. its center
 * sits half that radius above pos */
static void sky_deck_blast_step(struct task *task)
{
    struct blast_work *work = (struct blast_work *)task->work;
    struct collision_shape *shape;

    if (placement_try_leave_by(task, BLAST_RANGE)) {
        return;
    }
    sky_deck_blast_draw(task);
    if (placement_editing(task)) {
        return;
    }
    shape = work->collision->shapes;
    if (work->timer > BLAST_END) {
        task_kill(task);
    }
    /* @bug logic: the shrink takes over from the growth at a timer of
     * 33, BLAST_TOP - BLAST_START, though each gives 0.5 at BLAST_TOP. so the
     * phase drops from 0.439 to 0.425 at 33, and climbs again to 0.5 at
     * BLAST_TOP (MOV #33,R3 and CMP/GE at 0x0c914d5e and 0x0c914d62) */
    if (work->timer < BLAST_TOP - BLAST_START) {
        work->phase =
            (work->timer - BLAST_START) * (0.5F / (BLAST_TOP - BLAST_START));
    } else {
        work->phase =
            ((work->timer - BLAST_TOP) * (0.5F / (BLAST_GONE - BLAST_TOP))) +
            0.5F;
    }
    if (work->phase >= 0.0F) {
        work->size = njSin((Angle)(work->phase * 180.0F * ANGLE_PER_DEGREE));
    } else {
        work->size = 0.0F;
    }
    /* @hack spelling: the radius read through the shape's address, which the
     * pool holds, 20 bytes on (MOV #20,R0 and FMOV.S @(R0,R3),FR2 at
     * 0x0c914e2a and 0x0c914e2c); read as sky_deck_blast_shape.radius, the
     * pool holds the radius's own address and 3 of 119 words differ */
    shape->radius = (&sky_deck_blast_shape)->radius * work->size;
    shape->center.y = shape->radius * 0.5F;
    collision_register((struct task_work *)work);
    if (work->timer == 0) {
        explosion_create_at(work->pos.x, work->pos.y, work->pos.z,
                            work->scale * 7.0F);
    }
    ++work->timer;
}

/* the ring grows and fades over the first RING_STEPS. past BLAST_START,
 * the fireball takes the ball's size, and the pillar rises and falls while
 * the ball grows. as the ball shrinks, the fireball and the cloud fade, and
 * the cloud flattens */
static void sky_deck_blast_draw(struct task *task)
{
    struct blast_work *work = (struct blast_work *)task->work;
    Float scale;
    Float time;
    Float fade;
    Float part;
    Float height;
    Float width;

    if (placement_try_leave_by(task, BLAST_RANGE)) {
        return;
    }
    scale = work->scale;
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                      scale * BLAST_VIEW)) {
        return;
    }
    if (work->mode != BLAST_RUN) {
        return;
    }
    time = work->timer;
    fade = 0.0F;
    if (work->phase > 0.5F) {
        fade = (0.5F - work->phase) * 2.0F;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    njScale(NULL, scale, scale, scale);
    if (time < RING_STEPS) {
        njSetTexture(&obj_skydeck_texlist);
        njPushMatrix(NULL);
        part = time * (1.0F / RING_STEPS);
        scale = part * RING_SIZE;
        draw_set_material(-part, 0.0F, 0.0F, 0.0F);
        njScale(NULL, scale, scale, scale);
        TURN(njRotateY, (Angle)(time * RING_SPIN * ANGLE_PER_DEGREE));
        draw_tree(&blast_ring_object);
        njPopMatrix(1);
    }
    draw_set_material(fade, 0.0F, 0.0F, 0.0F);
    if (time > BLAST_START) {
        njSetTexture(&obj_skydeck_texlist);
        njPushMatrix(NULL);
        scale = work->size;
        njScale(NULL, scale, scale, scale);
        TURN(njRotateY, (Angle)(-(time * FIREBALL_SPIN) * ANGLE_PER_DEGREE));
        draw_tree(&blast_fireball_object);
        njPopMatrix(1);
    }
    if (time > BLAST_START && work->phase < 0.5F) {
        height = njSin((Angle)(work->phase * 360.0F * ANGLE_PER_DEGREE)) *
                 PILLAR_HEIGHT;
        njSetTexture(&obj_skydeck_texlist);
        njPushMatrix(NULL);
        /* @hack redundant: the widths times 1, an FLDI1 and FMUL at
         * 0x0c915058 and 0x0c91505e; as work->size alone, 143 of 318 words
         * differ */
        njScale(NULL, work->size * 1.0F, height, work->size * 1.0F);
        TURN(njRotateY, (Angle)(-(time * PILLAR_SPIN) * ANGLE_PER_DEGREE));
        draw_tree(&blast_pillar_object);
        njPopMatrix(1);
    }
    if (time > BLAST_START) {
        njSetTexture(&obj_skydeck_texlist);
        njPushMatrix(NULL);
        width = work->size;
        height = width;
        if (work->phase >= 0.5F) {
            width = 1.0F;
            height = (1.0F - work->phase) * 2.0F;
        }
        njScale(NULL, width, height, width);
        TURN(njRotateY, (Angle)(-(time * CLOUD_SPIN) * ANGLE_PER_DEGREE));
        draw_tree(&blast_cloud_object);
        njPopMatrix(1);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}

struct task *sky_deck_blast_create_level(NJS_POINT3 *pos, Float scale)
{
    return sky_deck_blast_create(pos, scale, 0, 0);
}

struct task *sky_deck_blast_create(NJS_POINT3 *pos, Float scale, Angle ang_x,
                                   Angle ang_z)
{
    struct task *task;
    struct blast_work *work;
    NJS_POINT3 from;
    NJS_VECTOR speed;

    task = task_create(TASK_WORK, 3, sky_deck_blast_start);
    if (task) {
        work = (struct blast_work *)task->work;
        work->pos = *pos;
        work->ang.x = ang_x;
        work->ang.z = ang_z;
        work->scale = scale;
        from = *pos;
        from.y += scale + 2.0F;
        speed.x = 0.0F;
        speed.y = (scale * 3.0F) + 3.0F;
        speed.z = 0.0F;
        sky_deck_sparks_create(&from, &speed, scale * 2.5F, pos->y,
                               (int)(scale * 20.0F));
        return task;
    }
    return NULL;
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (0x0c91525a); without it, task is kept at sp+0, 12 of 16 words
 * differ and sky_deck_burst_init starts 2 bytes late */
static void sky_deck_burst_start(register struct task *task)
{
    if (!placement_try_leave_by(task, BLAST_RANGE)) {
        sky_deck_burst_init(task);
    }
}

static void sky_deck_burst_init(struct task *task)
{
    struct blast_work *work = (struct blast_work *)task->work;

    if (!placement_editing(task)) {
        collision_init(task, &sky_deck_burst_shape, COLLISION_OBJECT, 1, 1);
    }
    task->step = sky_deck_burst_step;
    task->destructor = sky_deck_burst_destroy;
    work->timer = 0;
    work->mode = BLAST_RUN;
    work->size = 1.0F;
    work->phase = 0.0F;
}

static void sky_deck_burst_destroy(struct task *task)
{
}

/* sky_deck_blast_step without the draw. the first step sets off a puff
 * and a smaller explosion */
static void sky_deck_burst_step(struct task *task)
{
    struct blast_work *work = (struct blast_work *)task->work;
    struct collision_shape *shape;

    if (placement_try_leave_by(task, BLAST_RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        return;
    }
    shape = work->collision->shapes;
    if (work->timer > BLAST_END) {
        task_kill(task);
    }
    /* @bug logic: the shrink takes over from the growth at a timer of
     * 33, as the blast's does (MOV #33,R3 and CMP/GE at 0x0c915338 and
     * 0x0c91533c) */
    if (work->timer < BLAST_TOP - BLAST_START) {
        work->phase =
            (work->timer - BLAST_START) * (0.5F / (BLAST_TOP - BLAST_START));
    } else {
        work->phase =
            ((work->timer - BLAST_TOP) * (0.5F / (BLAST_GONE - BLAST_TOP))) +
            0.5F;
    }
    if (work->phase >= 0.0F) {
        work->size = njSin((Angle)(work->phase * 180.0F * ANGLE_PER_DEGREE));
    } else {
        work->size = 0.0F;
    }
    /* @hack spelling: the radius read through the shape's address, as the
     * blast's is (0x0c915398); read as sky_deck_burst_shape.radius, 3 of 119
     * words differ */
    shape->radius = (&sky_deck_burst_shape)->radius * work->size;
    shape->center.y = shape->radius * 0.5F;
    collision_register((struct task_work *)work);
    if (work->timer == 0) {
        bomb_create(&work->pos, work->scale);
        explosion_create_at(work->pos.x, work->pos.y, work->pos.z,
                            work->scale * 4.0F);
    }
    ++work->timer;
}

struct task *sky_deck_burst_create(NJS_POINT3 *pos, Float scale)
{
    struct task *task;
    struct blast_work *work;
    /* @hack redundant: 24 bytes of frame that no code reads, of a type unknown,
     * declared as sky_deck_blast_create's from and speed. the image's ADD
     * #-32,R15 reserves them (0x0c9153f0); without them, 3 of 32 words
     * differ */
    NJS_POINT3 from;
    NJS_VECTOR speed;

    task = task_create(TASK_WORK, 3, sky_deck_burst_start);
    if (task) {
        work = (struct blast_work *)task->work;
        work->pos = *pos;
        work->scale = scale;
        return task;
    }
    return NULL;
}

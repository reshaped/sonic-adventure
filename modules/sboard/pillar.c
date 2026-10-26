/* PILLAR S, M and L, stones that slide along their own y as a player comes
 * near. a pillar that slides far raises smoke and rumbles the pad */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "object/placement.h"
#include "object/simple.h"
#include "player/player.h"
#include "sboard/pillar.h"
#include "sboard/rumble.h"
#include "sboard/sand_smoke.h"
#include "sound/cue.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/util.h"

/* a pillar's record, in task->motion */
struct pillar_record {
    Sint8 mode;
    NJS_POINT3 home; /* where it was placed, before START and the slide */
    Float offset; /* how far it has slid past START */
    Float travel; /* how far it slides in all */
    Float speed;
    Float reach; /* the world test's radius at size 1: twice the model's */
    NJS_OBJECT *world; /* the pillar's world object */
    NJS_OBJECT *object;
    Sint8 done; /* TRUE once the slide is over, which fades the smoke */
    const struct sand_smoke_shape *smoke;
};

/* a kind of pillar, which init takes as arg */
struct pillar_kind {
    NJS_OBJECT *object;
    struct sand_smoke_shape smoke;
};

enum pillar_mode { PILLAR_WAIT, PILLAR_SLIDE, PILLAR_DONE };

/* the parts of a pillar's scale. START is how far along its y the pillar
 * starts, and TRAVEL how far it slides from there. SIZE is its size times
 * -10, and a SIZE of 0 or more means size 1 */
#define START(scl) ((scl)->x)
#define TRAVEL(scl) ((scl)->y)
#define SIZE(scl) ((scl)->z)

/* a player within WAKE_REACH wakes the pillar, which then slides SLIDE_SPEED
 * a step. a slide SHAKE_TRAVEL long or longer, either way along y, raises
 * smoke and plays the slide's sounds. such a slide also rumbles the pad
 * while the pillar is within SHAKE_REACH of the camera */
#define WAKE_REACH 800.0F
#define SLIDE_SPEED (120.0F * (1.0F / 60.0F))
#define SHAKE_TRAVEL 10.0F
#define SHAKE_REACH 500.0F

#define SE_PILLAR_SLIDE 693
#define SE_PILLAR_STOP 694

extern NJS_OBJECT pillar_s_object;
extern NJS_OBJECT pillar_m_object;
extern NJS_OBJECT pillar_l_object;

static void pillar_init(struct task *task, void *arg);
static void pillar_step(struct task *task);
static void pillar_draw(struct task *task);
static void pillar_destroy(struct task *task);
static void pillar_move(struct task_work *work, NJS_POINT3 *home, Float offset);

static const struct task_desc pillar_desc = {sizeof(struct pillar_record),
                                             TASK_WORK | TASK_FORCE,
                                             pillar_init,
                                             pillar_step,
                                             pillar_draw,
                                             pillar_destroy};
static const struct sound_falloff slide_falloff = {63, -128, 100.0F, 2400.0F};
static const struct sound_falloff stop_falloff = {63, -128, 100.0F, 2400.0F};

/* where each kind's smoke rises: six points round the pillar's home */
static const NJS_POINT3 pillar_s_smoke[6] = {
    {0.0F, 8.0F, 100.0F},          {0.0F, 8.0F, -100.0F},
    {50.0F, 8.0F, 100.0F / 3.0F},  {50.0F, 8.0F, -100.0F / 3.0F},
    {-50.0F, 8.0F, 100.0F / 3.0F}, {-50.0F, 8.0F, -100.0F / 3.0F},
};
static const struct pillar_kind pillar_s = {&pillar_s_object,
                                            {pillar_s_smoke, 6, 4.0F}};
static const NJS_POINT3 pillar_m_smoke[6] = {
    {0.0F, 8.0F, 100.0F},          {0.0F, 8.0F, -100.0F},
    {50.0F, 8.0F, 100.0F / 3.0F},  {50.0F, 8.0F, -100.0F / 3.0F},
    {-50.0F, 8.0F, 100.0F / 3.0F}, {-50.0F, 8.0F, -100.0F / 3.0F},
};
static const struct pillar_kind pillar_m = {&pillar_m_object,
                                            {pillar_m_smoke, 6, 4.0F}};
static const NJS_POINT3 pillar_l_smoke[6] = {
    {0.0F, 8.0F, 100.0F},          {0.0F, 8.0F, -100.0F},
    {50.0F, 8.0F, 100.0F / 3.0F},  {50.0F, 8.0F, -100.0F / 3.0F},
    {-50.0F, 8.0F, 100.0F / 3.0F}, {-50.0F, 8.0F, -100.0F / 3.0F},
};
static const struct pillar_kind pillar_l = {&pillar_l_object,
                                            {pillar_l_smoke, 6, 4.0F}};

/* the offset pillar_move turns by the pillar's angles: 0 on x and z, and
 * the slide on y */
static NJS_POINT3 slide = {0.0F, 0.0F, 0.0F};

static void pillar_turn_x(struct angles *ang)
{
    Angle x;

    TURN_VIA(njRotateX, ang->x, x);
}

static void pillar_turn_y(struct angles *ang)
{
    Angle y;

    TURN_VIA(njRotateY, ang->y, y);
}

static void pillar_turn_z(struct angles *ang)
{
    Angle z;

    TURN_VIA(njRotateZ, ang->z, z);
}

void pillar_s_step(struct task *task)
{
    task_apply_desc(task, &pillar_desc, (void *)&pillar_s);
}

void pillar_m_step(struct task *task)
{
    task_apply_desc(task, &pillar_desc, (void *)&pillar_m);
}

void pillar_l_step(struct task *task)
{
    task_apply_desc(task, &pillar_desc, (void *)&pillar_l);
}

/* @hack spelling: the pragma, which has SHC expand pillar_size at each call
 * under -noinline; without it, SHC writes pillar_size out of line, and 260
 * of the 376 words of pillar_init, pillar_step and pillar_draw differ */
#pragma inline(pillar_size)
/* a pillar's size: z times scale, or 1 for a z of 0 or more. each call
 * passes SIZE as z */
/* @hack spelling: the scale through a parameter, which each call passes as
 * -0.1F and SHC loads into FR5 ahead of the test (FMOV.S @R0,FR5 at
 * 0x0c9035c0); as a constant in pillar_size, the object's code runs 32
 * bytes longer and 332 of those 376 words differ */
static Float pillar_size(Float z, Float scale)
{
    /* @hack spelling: the size through a local, which keeps the task in R12
     * in pillar_init (MOV R4,R12 at 0x0c903576); returned directly, the task
     * and its work trade R11 and R12, and 12 of those 376 words differ */
    Float size = z >= 0.0F ? 1.0F : z * scale;

    return size;
}

static void pillar_init(struct task *task, void *arg)
{
    NJS_OBJECT *object = ((const struct pillar_kind *)arg)->object;
    NJS_OBJECT *world = object_world_copy(object);
    struct pillar_record *record = (struct pillar_record *)task->motion;
    struct task_work *work;
    NJS_POINT3 *scl;
    Float size;

    record->world = world;
    if (world == NULL) {
        task_kill(task);
    } else {
        work = task->work;
        scl = &work->scl;
        record->home = work->pos;
        pillar_move(work, &record->home, START(scl));
        size = pillar_size(SIZE(scl), -0.1F);
        object_world_place(world, work, size);
        object_world_add_sized(task, world, WORLD_MOVES | WORLD_SOLID, size);
        record->mode = PILLAR_WAIT;
        record->offset = 0.0F;
        record->reach = world->model->r * 2.0F;
        record->object = object;
        record->smoke = &((const struct pillar_kind *)arg)->smoke;
        object_force_clear(task->force);
    }
}

/* while the set editor holds the pillar, show it slid all the way.
 * otherwise it waits for a player within WAKE_REACH, then slides TRAVEL
 * from START. its force's speed.y is how far it slid this step */
static void pillar_step(struct task *task)
{
    struct task_work *work;
    struct pillar_record *record;
    struct task_force *force;
    NJS_POINT3 *home;
    NJS_POINT3 *scl;
    Float start;
    Float travel;
    Float size;
    Float far;
    int shakes;
    struct sand_smoke_arg smoke;

    if (placement_try_leave(task)) {
        return;
    }
    work = task->work;
    scl = &work->scl;
    travel = TRAVEL(scl);
    record = (struct pillar_record *)task->motion;
    force = task->force;
    home = &record->home;
    start = START(scl);
    size = pillar_size(SIZE(scl), -0.1F);
    far = travel;
    if (far < 0.0F) {
        far = -far;
    }
    shakes = far >= SHAKE_TRAVEL;
    if (placement_editing(task)) {
        *home = work->pos;
        pillar_move(work, home, travel + start);
        pillar_draw(task);
        work->pos = *home;
    } else {
        pillar_draw(task);
        object_world_place(record->world, work, size);
        object_world_test_radius(work, record->reach * size);
        switch (record->mode) {
        case PILLAR_WAIT:
            if (player_in_range(&work->pos, WAKE_REACH) == 0) {
                return;
            }
            record->travel = travel;
            record->speed = travel >= 0.0F ? SLIDE_SPEED : -SLIDE_SPEED;
            record->done = FALSE;
            record->mode = PILLAR_SLIDE;
            if (shakes) {
                smoke.pos = home;
                smoke.ang = &work->ang;
                smoke.shape = record->smoke;
                smoke.done = &record->done;
                task_create_desc_child(task, &sand_smoke_desc, &smoke);
                sound_keep_near(SE_PILLAR_SLIDE, task, &work->pos,
                                &slide_falloff);
            }
            break;
        case PILLAR_SLIDE: {
            Float speed = record->speed;
            Float end = record->travel;

            travel = record->offset + speed;
            if ((speed >= 0.0F && travel >= end) ||
                (speed < 0.0F && travel <= end)) {
                travel = end;
                record->done = TRUE;
                record->mode = PILLAR_DONE;
                if (shakes) {
                    sound_stop_near(task);
                    sound_play_near(SE_PILLAR_STOP, &work->pos, &stop_falloff);
                }
            } else if (shakes) {
                sound_keep_near(SE_PILLAR_SLIDE, task, &work->pos,
                                &slide_falloff);
            }
            if (shakes) {
                rumble_near_camera(&work->pos, SHAKE_REACH);
            }
            force->speed.y = travel - record->offset;
            record->offset = travel;
            pillar_move(work, home, travel + start);
            task_step_children(task);
        } break;
        case PILLAR_DONE:
        default:
            force->speed.y = 0.0F;
            if (task->child != NULL) {
                task_step_children(task);
            }
            break;
        }
    }
}

/* draw the pillar at its size on a frame's first step */
static void pillar_draw(struct task *task)
{
    struct task_work *work;
    struct pillar_record *record;
    Float size;

    if (frame_step == 0) {
        work = task->work;
        record = (struct pillar_record *)task->motion;
        size = pillar_size(SIZE(&work->scl), -0.1F);
        object_draw_tree_zyx(record->object, work, size, NULL);
    }
}

static void pillar_destroy(struct task *task)
{
    struct pillar_record *record = (struct pillar_record *)task->motion;

    object_world_free(task, record->world);
}

/* set work->pos to home moved offset along the pillar's own y, as work->ang
 * turns it */
static void pillar_move(struct task_work *work, NJS_POINT3 *home, Float offset)
{
    struct angles *ang;

    slide.y = offset;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, home);
    ang = &work->ang;
    pillar_turn_z(ang);
    pillar_turn_y(ang);
    pillar_turn_x(ang);
    njCalcPoint(NULL, &slide, &work->pos);
    njPopMatrix(1);
}

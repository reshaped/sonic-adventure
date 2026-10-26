/* O FOUNT fountain whose jet throws player 0 up and away from inside it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg04/fountain.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the jet, whose child and the child's sibling are two pieces at its top. the
 * spout and the sprays stand at its foot. both sprays draw the one spray
 * model */
extern NJS_OBJECT fountain_object;
extern NJS_OBJECT fountain_spout_object;
extern NJS_OBJECT fountain_spray_object;
/* the water's four frames */
extern NJS_TEXNAME fountain_texnames[4];

/* the fountain's range for placement_try_leave_by, squared */
#define FOUNTAIN_RANGE (1010.0F * 1010.0F)

/* reach of the fountain's collision record from its position */
#define FOUNTAIN_REACH 5.0F

/* the water's frame, 0 to 3, which advances each step */
#define WATER_FRAMES 4
#define WATER_FRAME(work) ((work)->tick)

/* the jet's height is a scale of its models. it wavers within WAVER_BAND above
 * LOW_HEIGHT for PULSE_STEPS steps. it then rises RISE_STEP a step to
 * HIGH_HEIGHT, and wavers within WAVER_BAND below it for PULSE_STEPS steps. it
 * then falls FALL_STEP a step back to LOW_HEIGHT. a waver moves the height by
 * up to WAVER_STEP a step */
#define LOW_HEIGHT 0.4F
#define HIGH_HEIGHT 1.0F
#define PULSE_STEPS 100
#define WAVER_BAND 0.1F
#define WAVER_STEP 0.005F
#define RISE_STEP 0.05F
#define FALL_STEP 0.01F

/* the jet, the spout and each spray sway their scale in x and z at random.
 * each step moves it by up to WOBBLE_STEP, within WOBBLE_LEAST and
 * WOBBLE_MOST */
#define WOBBLE_STEP 0.01F
#define WOBBLE_LEAST 0.99F
#define WOBBLE_MOST 1.01F

/* player 0 is in the jet within THROW_REACH of its axis, above its foot. the
 * jet reaches THROW_HEIGHT times its height above the foot */
#define THROW_REACH 15.0F
#define THROW_HEIGHT 100.0F

/* the throw's speed in the fountain's frame, unless the set entry gives one in
 * its scale */
#define THROW_X 0.0F
#define THROW_Y 6.0F
#define THROW_Z (-6.0F)

/* a thrown player starts at least this far above the fountain */
#define THROW_LIFT 20.0F

/* a spray's height, as a scale of its model, starts at SPRAY_START. it swings
 * between SPRAY_LOW and SPRAY_HIGH by SPRAY_STEP a step, while the spray turns
 * SPRAY_TURN a step */
#define SPRAY_START 0.75F
#define SPRAY_LOW 0.65F
#define SPRAY_HIGH 0.85F
#define SPRAY_STEP 0.002F
#define SPRAY_TURN 32

#define SE_THROW 106
#define SE_WATER 105

#define HALF_TURN NJM_DEG_ANG(180)

enum fountain_mode {
    FOUNTAIN_INIT,
    FOUNTAIN_READY,
    FOUNTAIN_THROWN, /* until the thrown player leaves the jet */
    FOUNTAIN_GONE /* this file never sets it */
};

/* where the jet's height is in its pulse, in work->submode */
enum fountain_pulse {
    PULSE_LOW,
    PULSE_RISE,
    PULSE_HIGH,
    PULSE_FALL,
    PULSE_STILL /* this file never sets it */
};

/* SPRAY_INIT moves on to the mode in the spray's work->flags, which its spec
 * sets to SPRAY_RISE or SPRAY_SINK */
enum spray_mode {
    SPRAY_INIT,
    SPRAY_RISE,
    SPRAY_SINK,
    SPRAY_GONE /* this file never sets it */
};
enum spout_mode {
    SPOUT_INIT = 0,
    SPOUT_RUN = 1,
    SPOUT_GONE = 3 /* this file never sets it */
};

/* the fountain's extra record: the throw's speed in the world's frame */
struct fountain_extra {
    /* @hack type: the speed as an array, which has fountain_throw keep the
     * extra's address in a frame word across sound_play, MOV.L R3,@R15 at
     * 0x0c9023ba. as an NJS_VECTOR, 8 of fountain_aim's 82 words differ where
     * they read the pool the two share, fountain_throw runs 8 bytes shorter,
     * and 53 of 56 words differ */
    Float throw_speed[3];
};

static void fountain_draw(struct task *task);
static void fountain_release(struct task *task);
static void fountain_destroy(struct task *task);
static void fountain_aim(struct task *task);
static void fountain_throw(struct task *task);
static void fountain_wobble(struct task_work *work);
static void fountain_pulse(struct task_work *work);
static void fountain_spout_draw(struct task *task);
static void fountain_spout_step(struct task *task);
static void fountain_spray_draw(struct task *task);
static void fountain_spray_step(struct task *task);

static NJS_TEXLIST fountain_texlists[WATER_FRAMES] = {
#include "stg04/fountain/fountain_texlists.inc"
};

static struct collision_shape fountain_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 30.0F, 0.0F},
    5.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

/* the two sprays, the second turned half round and starting the other way */
static struct task_spec fountain_sprays[3] = {
    {fountain_spray_step,
     TASK_WORK,
     SPRAY_RISE,
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&fountain_spray_object},
    {fountain_spray_step,
     TASK_WORK,
     SPRAY_SINK,
     {0, HALF_TURN, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&fountain_spray_object},
    {0},
};

/* draw the jet stretched to its height, on a frame's first step only. the
 * pieces at its top keep their own height, and widen as the jet rises */
static void fountain_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = MODEL(work)->object;
    NJS_OBJECT *child;
    Float width;
    Float height;
    Angle ang;

    if (!frame_step) {
        MODEL(work)->texlist = &fountain_texlists[WATER_FRAME(work)];
        njSetTexture(MODEL(work)->texlist);
        njPushMatrix(NULL);
        height = 1.0F / work->scl.y;
        width = 0.4F + (0.6F * work->scl.y);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScaleV(NULL, &work->scl);
        draw_model(object->model);
        njPushMatrix(NULL);
        child = MODEL(work)->object->child;
        njTranslate(NULL, child->pos[0], child->pos[1], child->pos[2]);
        njScale(NULL, width, height, width);
        draw_model(child->model);
        njPopMatrix(1);
        njPushMatrix(NULL);
        child = child->sibling;
        njTranslate(NULL, child->pos[0], child->pos[1], child->pos[2]);
        njScale(NULL, width, height, width);
        draw_model(child->model);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

/* take the task's object out of the collision world, and give it back to the
 * world's pool. no task of the fountain's puts an object in the world, so
 * world_remove changes nothing. the object is a model of the stage's, which
 * world_object_free ignores */
static void fountain_release(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        if (MODEL(work)->object) {
            world_remove(task, MODEL(work)->object);
            world_object_free(MODEL(work)->object);
        }
    }
}

static void fountain_destroy(struct task *task)
{
    task_destroy_children(task);
    fountain_release(task);
}

/* set the throw's speed in the world's frame from the set entry's scale,
 * turned by the fountain's angles. a scale of 0 on every axis gives THROW_X,
 * THROW_Y and THROW_Z instead */
static void fountain_aim(struct task *task)
{
    struct fountain_extra *extra = task->extra;
    struct task_work *work = task->work;
    NJS_VECTOR speed = {THROW_X, THROW_Y, THROW_Z};
    Angle ang;

    if (work->scl.x != 0.0F || work->scl.y != 0.0F || work->scl.z != 0.0F) {
        speed.x = work->scl.x;
        speed.y = work->scl.y;
        speed.z = work->scl.z;
    }
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &speed, (NJS_VECTOR *)extra->throw_speed);
    njPopMatrix(1);
}

/* throw player 0 from the jet's axis at the throw's speed. turn player 0 to
 * face half round from the fountain's heading */
static void fountain_throw(struct task *task)
{
    struct task_work *work = task->work;
    struct fountain_extra *extra = task->extra;
    struct task_work *player = player_work[0];

    sound_play(SE_THROW, 0, 0, 0);
    player_bounce(0, extra->throw_speed[0], extra->throw_speed[1],
                  extra->throw_speed[2]);
    player_set_angles(0, 0, work->ang.y + HALF_TURN, 0);
    player->pos.x = work->pos.x;
    player->pos.z = work->pos.z;
    if (player->pos.y - work->pos.y < THROW_LIFT) {
        player->pos.y = work->pos.y + THROW_LIFT;
    }
}

static void fountain_wobble(struct task_work *work)
{
    work->scl.x += RANDOM(-WOBBLE_STEP, WOBBLE_STEP);
    work->scl.z += RANDOM(-WOBBLE_STEP, WOBBLE_STEP);
    if (work->scl.x > WOBBLE_MOST) {
        work->scl.x = WOBBLE_MOST;
    } else if (work->scl.x < WOBBLE_LEAST) {
        work->scl.x = WOBBLE_LEAST;
    }
    if (work->scl.z > WOBBLE_MOST) {
        work->scl.z = WOBBLE_MOST;
    } else if (work->scl.z < WOBBLE_LEAST) {
        work->scl.z = WOBBLE_LEAST;
    }
}

static void fountain_pulse(struct task_work *work)
{
    switch (work->submode) {
    case PULSE_LOW:
        if (TASK_TIMER(work)++ >= PULSE_STEPS) {
            work->timer = 0;
            work->submode = PULSE_RISE;
        } else {
            work->scl.y += RANDOM(-WAVER_STEP, WAVER_STEP);
            if (work->scl.y < LOW_HEIGHT) {
                work->scl.y = LOW_HEIGHT;
            } else if (work->scl.y > LOW_HEIGHT + WAVER_BAND) {
                work->scl.y = LOW_HEIGHT + WAVER_BAND;
            }
        }
        break;
    case PULSE_RISE:
        if (work->scl.y > HIGH_HEIGHT) {
            work->submode = PULSE_HIGH;
        } else {
            work->scl.y += RISE_STEP;
        }
        break;
    case PULSE_HIGH:
        if (TASK_TIMER(work)++ >= PULSE_STEPS) {
            work->timer = 0;
            work->submode = PULSE_FALL;
        } else {
            work->scl.y += RANDOM(-WAVER_STEP, WAVER_STEP);
            if (work->scl.y > HIGH_HEIGHT) {
                work->scl.y = HIGH_HEIGHT;
            } else if (work->scl.y < HIGH_HEIGHT - WAVER_BAND) {
                work->scl.y = HIGH_HEIGHT - WAVER_BAND;
            }
        }
        break;
    case PULSE_FALL:
        if (work->scl.y < LOW_HEIGHT) {
            work->submode = PULSE_LOW;
        } else {
            work->scl.y -= FALL_STEP;
        }
        break;
    case PULSE_STILL:
    default:
        work->scl.y = HIGH_HEIGHT;
        break;
    }
}

/* while the set editor holds the fountain, it draws its models. it labels its
 * scale as the throw's speed. a thrown player still in the jet at the next
 * step is thrown again. each such throw sets the jet rising to its full
 * height */
void fountain_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    /* @hack spelling: ready, a copy of FOUNTAIN_READY that also sets the pulse,
     * as PULSE_RISE. SHC keeps it in R10, MOV #1,R10 at 0x0c9026fc, and passes
     * collision_init its 1s from R3, MOV #1,R3 at 0x0c90279a; without it,
     * collision_init's 1s come from R10, and 35 of 360 words differ */
    Sint8 ready;

    if (placement_try_leave_by(task, FOUNTAIN_RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        WATER_FRAME(work)++;
        if (WATER_FRAME(work) > WATER_FRAMES - 1) {
            WATER_FRAME(work) = 0;
        }
        njSetTexture(&fountain_texlists[WATER_FRAME(work)]);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&fountain_object);
        draw_tree(&fountain_spout_object);
        draw_tree(&fountain_spray_object);
        njPopMatrix(1);
        njPrintC(NJM_LOCATION(20, 9), "SCL IS FOR SPEED.");
        njPrintC(NJM_LOCATION(20, 10), "THE DEFAULT IS");
        njPrintC(NJM_LOCATION(20, 11), "0.0F, 6.0F, -6.0F.");
        /* @hack redundant: the else after the return, which loads
         * fountain_object's address into R3 at 0x0c902780; without it, R2 takes
         * the address, and 2 of 360 words differ */
        return;
    } else {
        ready = FOUNTAIN_READY;
        switch (work->mode) {
        case FOUNTAIN_INIT:
            MODEL(work)->object = &fountain_object;
            fountain_aim(task);
            work->scl.x = work->scl.z = 1.0F;
            work->scl.y = LOW_HEIGHT;
            work->mode = ready;
            work->submode = PULSE_LOW;
            collision_init(task, &fountain_shape, COLLISION_OBJECT, 1, 1);
            work->collision->radius = FOUNTAIN_REACH;
            work->collision->flags |= COLLISION_TARGET;
            task->destructor = fountain_destroy;
            task->draw = fountain_draw;
            task_create_children(fountain_sprays, task);
            task_create_child(TASK_WORK, fountain_spout_step, task);
            break;
        case FOUNTAIN_READY: {
            NJS_VECTOR delta;
            struct task_work *player;

            fountain_draw(task);
            sound_play_at_for(SE_WATER, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 30);
            if (placement_editing(task)) {
                break;
            }
            player = player_work[0];
            delta.x = player->pos.x - work->pos.x;
            delta.y = player->pos.y - work->pos.y;
            delta.z = player->pos.z - work->pos.z;
            /* @hack redundant: a never-run read of delta.x and delta.z, which
             * gives the deltas the image's spill slots (0x0c902898 to
             * 0x0c90295c); without it, 6 of 360 words differ */
            if (0) {
                (void)(delta.x + delta.z);
            }
            if (njSqrt((delta.x * delta.x) + (delta.z * delta.z)) <=
                    THROW_REACH &&
                delta.y > 0.0F && delta.y <= work->scl.y * THROW_HEIGHT) {
                fountain_throw(task);
                work->mode = FOUNTAIN_THROWN;
                work->timer = 0;
            }
            fountain_pulse(work);
            fountain_wobble(work);
            task_step_children(task);
            collision_register(work);
            break;
        }
        case FOUNTAIN_THROWN: {
            NJS_VECTOR delta;
            struct task_work *player;

            fountain_draw(task);
            sound_play_at_for(SE_WATER, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 30);
            if (placement_editing(task)) {
                break;
            }
            /* @hack spelling: player assigned inside the first difference,
             * which gives placement_editing's address R11 and ready R10, at
             * 0x0c90266e and 0x0c9026fc; assigned ahead of it, the two swap
             * registers, and 18 of 360 words differ */
            delta.x = (player = player_work[0])->pos.x - work->pos.x;
            delta.y = player->pos.y - work->pos.y;
            delta.z = player->pos.z - work->pos.z;
            /* @hack redundant: the same for player's position, which gives the
             * deltas the image's spill slots (0x0c902952 to 0x0c902964);
             * without it, 3 of 360 words differ */
            if (0) {
                (void)(player->pos.x + player->pos.y + player->pos.z);
            }
            if (njSqrt((delta.x * delta.x) + (delta.z * delta.z)) <=
                    THROW_REACH &&
                delta.y > 0.0F && delta.y <= work->scl.y * THROW_HEIGHT) {
                fountain_throw(task);
                work->submode = ready;
            } else {
                work->mode = ready;
                work->timer = 0;
            }
            fountain_pulse(work);
            fountain_wobble(work);
            task_step_children(task);
            break;
        }
        case FOUNTAIN_GONE:
        default:
            fountain_release(task);
            break;
        }
    }
    WATER_FRAME(work)++;
    if (WATER_FRAME(work) > WATER_FRAMES - 1) {
        WATER_FRAME(work) = 0;
    }
}

static void fountain_spout_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;

    njSetTexture(&fountain_texlists[WATER_FRAME(parent)]);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScaleV(NULL, &work->scl);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}

static void fountain_spout_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SPOUT_INIT:
        work->mode = SPOUT_RUN;
        MODEL(work)->object = &fountain_spout_object;
        task->destructor = fountain_release;
        task->draw = fountain_spout_draw;
        work->scl.x = work->scl.y = work->scl.z = 1.0F;
        break;
    case SPOUT_RUN:
        fountain_wobble(work);
        fountain_spout_draw(task);
        break;
    case SPOUT_GONE:
    default:
        fountain_release(task);
        break;
    }
}

static void fountain_spray_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    Angle ang;

    njSetTexture(&fountain_texlists[WATER_FRAME(parent)]);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScaleV(NULL, &work->scl);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}

/* a spray's spec gives it its model, and SPRAY_INIT sets the model again from
 * the first spray's spec. both specs name fountain_spray_object, so the second
 * spray's model stays as it was */
static void fountain_spray_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SPRAY_INIT:
        MODEL(work)->object = (NJS_OBJECT *)fountain_sprays[0].var;
        task->destructor = fountain_release;
        task->draw = fountain_spray_draw;
        work->scl.x = work->scl.y = work->scl.z = 1.0F;
        work->scl.y = SPRAY_START;
        work->mode = work->flags;
        break;
    case SPRAY_RISE:
        if (work->scl.y >= SPRAY_HIGH) {
            work->mode = SPRAY_SINK;
        } else {
            work->scl.y += SPRAY_STEP;
            work->ang.y += SPRAY_TURN;
        }
        fountain_wobble(work);
        fountain_spray_draw(task);
        break;
    case SPRAY_SINK:
        if (work->scl.y <= SPRAY_LOW) {
            work->mode = SPRAY_RISE;
        } else {
            work->scl.y -= SPRAY_STEP;
            work->ang.y += SPRAY_TURN;
        }
        fountain_wobble(work);
        fountain_spray_draw(task);
        break;
    case SPRAY_GONE:
    default:
        fountain_release(task);
        break;
    }
}

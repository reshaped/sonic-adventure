/* Froggy in the events. he plays the motions his event queues, or draws his
 * still model. shape scripts blend the models of three of his nodes toward
 * other shapes, and back */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "event/event.h"
#include "shared/frog.h"
#include "system/task.h"
#include "system/util.h"

/* the shapes the scripts blend toward. 0 to 2 blend node 1's model, 3 and
 * 4 node 12's, and 5 and 6 node 13's */
#define FROG_SHAPES 7

/* how far above the ground EVENT_GROUND stands him */
#define FROG_LIFT 1.8F

/* what his shape record does each step, by its mode */
enum frog_shape_mode {
    FROG_STILL, /* nothing */
    FROG_START, /* start the script's next key, or stop at its end */
    FROG_BLEND, /* blend toward the key's shape, then FROG_START */
    /* FROG_START and FROG_BLEND for a pair: the script and the one after it.
     * each step blends both scripts' keys at one index. the first script's keys
     * set the pace and end the pair */
    FROG_START_PAIR,
    FROG_BLEND_PAIR,
    FROG_RESTORE /* put the three nodes' models back as made, then stop */
};

/* a key of a shape script. it blends the shape's node from its model as made
 * toward the shape. each step blends 1 / steps of the way. a key of 0 steps
 * ends the script */
struct frog_key {
    Sint8 steps;
    Sint8 shape;
};

/* his shape record, which his first step allocates and his destructor
 * frees */
struct frog_shape {
    Sint8 mode; /* a frog_shape_mode */
    Sint8 script; /* the one running, or of a pair the first */
    Sint8 key; /* the one blending, by its index in the script */
    Uint8 unk_03;
    Float blend; /* from 0, the model as made, to 1, the shape */
    Float speed; /* added to blend each step */
    Uint8 unk_0c[16];
};

struct frog_vars {
    struct frog_shape *shape;
};

#define VARS(work) ((struct frog_vars *)&(work)->vars)

/* each event that links frog.c defines these. he draws event_frog_object,
 * whose nodes 1, 12 and 13 hold the _model models that the blends change.
 * the _made models are those nodes' in a second copy of his model, kept as
 * made */
extern NJS_OBJECT event_frog_object;
extern NJS_MODEL event_frog_node1_model;
extern NJS_MODEL event_frog_node12_model;
extern NJS_MODEL event_frog_node13_model;
extern NJS_MODEL event_frog_node1_made;
extern NJS_MODEL event_frog_node12_made;
extern NJS_MODEL event_frog_node13_made;
extern NJS_POINT3 event_frog_shape_0[];
extern NJS_POINT3 event_frog_shape_1[];
extern NJS_POINT3 event_frog_shape_2[];
extern NJS_POINT3 event_frog_shape_3[];
extern NJS_POINT3 event_frog_shape_4[];
extern NJS_POINT3 event_frog_shape_5[];
extern NJS_POINT3 event_frog_shape_6[];

/* the shape scripts, which event_frog_set_shape numbers 0 to 6 */
static struct frog_key event_frog_script_0[2] = {
#include "shared/frog/event_frog_script_0.inc"
};
static struct frog_key event_frog_script_1[9] = {
#include "shared/frog/event_frog_script_1.inc"
};
static struct frog_key event_frog_script_2[3] = {
#include "shared/frog/event_frog_script_2.inc"
};
static struct frog_key event_frog_script_3[3] = {
#include "shared/frog/event_frog_script_3.inc"
};
static struct frog_key event_frog_script_4[3] = {
#include "shared/frog/event_frog_script_4.inc"
};
static struct frog_key event_frog_script_5[3] = {
#include "shared/frog/event_frog_script_5.inc"
};
static struct frog_key event_frog_script_6[2] = {
#include "shared/frog/event_frog_script_6.inc"
};
static struct frog_key *event_frog_scripts[7] = {
    event_frog_script_0, event_frog_script_1, event_frog_script_2,
    event_frog_script_3, event_frog_script_4, event_frog_script_5,
    event_frog_script_6,
};

/* the points each shape blends toward */
static NJS_POINT3 *event_frog_shape_points[FROG_SHAPES] = {
    event_frog_shape_0, event_frog_shape_1, event_frog_shape_2,
    event_frog_shape_3, event_frog_shape_4, event_frog_shape_5,
    event_frog_shape_6,
};
/* each shape's node's model as made, which its blends start from */
static NJS_MODEL *event_frog_made[FROG_SHAPES] = {
    &event_frog_node1_made,  &event_frog_node1_made,  &event_frog_node1_made,
    &event_frog_node12_made, &event_frog_node12_made, &event_frog_node13_made,
    &event_frog_node13_made,
};
/* each shape's node's model as drawn, which its blends write */
static NJS_MODEL *event_frog_models[FROG_SHAPES] = {
    &event_frog_node1_model,  &event_frog_node1_model,
    &event_frog_node1_model,  &event_frog_node12_model,
    &event_frog_node12_model, &event_frog_node13_model,
    &event_frog_node13_model,
};

/* the weld model_weld makes as he plays a motion. it moves four points of
 * node 12's model onto their partners in node 1's. a pair is a point's
 * index in node 1's model, then in node 12's */
static Sint16 event_frog_pairs[8] = {
#include "shared/frog/event_frog_pairs.inc"
};
static struct model_weld event_frog_welds[2] = {
    {4, 1, 12, event_frog_pairs},
    {0, 0, 0, NULL},
};

/* his collision: a sphere at his origin, of radius 0 until event_set_radius
 * gives it one. no group of shapes can push it, and it pushes the players. the
 * step registers it only while its radius is above 0 */
/* clang-format off */
static struct collision_shape event_frog_collision[1] = {
    {SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_PLAYERS, {0.0F, 0.0F, 0.0F},
     0.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
};
/* clang-format on */

/* the last Froggy created, or null before the first create and after
 * event_frog_kill */
static struct task *event_frog = NULL;

static void event_frog_restore(void);

static void event_frog_destroy(struct task *task)
{
    struct task_work *work = task->work;

    event_frog_restore();
    if (VARS(work)->shape != NULL) {
        heap_free(VARS(work)->shape);
        VARS(work)->shape = NULL;
    }
}

static void event_frog_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct event_work *event = work->event;
    struct event_motion *motion = event->motions;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    /* half a turn less ang.y, as event_actor_step turns an actor */
    TURN_VIA(njRotateY, NJM_DEG_ANG(180) - work->ang.y, ang);
    /* @hack spelling: work->event read again for the test (MOV.L @(60,R14),R2
     * at 0x0cb8325a in EV001C), though R12 holds it as event; without it, the
     * draw runs 2 bytes short and 41 of 85 words differ */
    if (work->event->motions != NULL) {
        /* the motion plays on event_frog_object, whatever model the event
         * queued it with, so that the blends show */
        motion->action.object = &event_frog_object;
        model_weld(motion->action.object, motion->action.motion,
                   event->motion_frame, event_frog_welds);
        event_motions_draw(work);
    } else {
        njSetTexture(&event_frog_texlist);
        draw_set_palette(DRAW_PLAYER_PALETTE);
        draw_tree(&event_frog_object);
        draw_set_palette(0);
    }
    njPopMatrix(1);
}

/* set the key's node's points shape->blend of the way from as made to the
 * key's shape */
static void event_frog_blend(struct frog_key *script, struct task *task)
{
    struct task_work *work = task->work;
    struct frog_shape *shape = VARS(work)->shape;
    Sint8 index;
    NJS_POINT3 *from;
    NJS_POINT3 *to;
    NJS_POINT3 *out;
    int count;

    script += shape->key;
    index = script->shape;
    from = event_frog_made[index]->points;
    out = event_frog_models[index]->points;
    to = event_frog_shape_points[index];
    count = event_frog_made[index]->nbPoint;
    points_blend(from, to, out, shape->blend, count);
}

static void event_frog_step(struct task *task)
{
    struct task_work *work = task->work;
    struct frog_shape *shape = VARS(work)->shape;
    struct event_work *event = work->event;
    struct frog_key *script;
    NJS_POINT3 under;
    struct angles slope;
    Float x;
    Float y;
    Float z;

    switch (shape->mode) {
    case FROG_STILL:
        break;
    case FROG_START:
        script = event_frog_scripts[shape->script];
        if (script[shape->key].steps == 0) {
            shape->mode = FROG_STILL;
            break;
        }
        shape->speed = 1.0F / script[shape->key].steps;
        shape->blend = 0.0F;
        shape->mode = FROG_BLEND;
        break;
    case FROG_BLEND:
        script = event_frog_scripts[shape->script];
        shape->blend += shape->speed;
        if (shape->blend >= 1.0F) {
            shape->blend = 1.0F;
        }
        event_frog_blend(script, task);
        if (shape->blend >= 1.0F) {
            shape->mode = FROG_START;
            shape->key++;
        }
        break;
    case FROG_START_PAIR:
        script = event_frog_scripts[shape->script];
        if (script[shape->key].steps == 0) {
            shape->mode = FROG_STILL;
            break;
        }
        shape->speed = 1.0F / script[shape->key].steps;
        shape->blend = 0.0F;
        shape->mode = FROG_BLEND_PAIR;
        break;
    case FROG_BLEND_PAIR:
        script = event_frog_scripts[shape->script];
        shape->blend += shape->speed;
        if (shape->blend >= 1.0F) {
            shape->blend = 1.0F;
        }
        event_frog_blend(script, task);
        script = event_frog_scripts[shape->script + 1];
        event_frog_blend(script, task);
        if (shape->blend >= 1.0F) {
            shape->mode = FROG_START_PAIR;
            shape->key++;
        }
        break;
    case FROG_RESTORE:
        event_frog_restore();
        shape->mode = FROG_STILL;
        break;
    }
    /* @hack spelling: work->event read again for the test (MOV.L @(60,R13),R2
     * at 0x0cb83456 in EV001C), though R11 holds it as event; without it, the
     * step runs 2 bytes short and 92 of 230 words differ */
    if (work->event->paths != NULL) {
        event_paths_step(work);
    }
    x = work->pos.x;
    y = work->pos.y;
    z = work->pos.z;
    under.y = ground_below(x, y, z, &slope);
    if (event->flags & EVENT_GROUND) {
        work->pos.y = under.y + FROG_LIFT;
    }
    if (event->flags & EVENT_SLOPE) {
        work->ang.x = slope.x;
        work->ang.z = slope.z;
    }
    if (event->shadow > 0.0F) {
        under.x = x;
        under.y += SHADOW_LIFT;
        under.z = z;
        shadow_draw_at(&slope, &under, event->shadow);
    }
    event_frog_draw(task);
    if (work->collision->shapes->radius > 0.0F) {
        collision_register(work);
    }
}

static void event_frog_init(struct task *task)
{
    struct task_work *work = task->work;
    struct frog_shape *shape = heap_alloc(sizeof(struct frog_shape));

    shape->mode = FROG_STILL;
    shape->script = 0;
    shape->key = 0;
    shape->blend = 0.0F;
    shape->speed = 0.0F;
    VARS(work)->shape = shape;
    event_alloc(work);
    collision_init(task, event_frog_collision, COLLISION_OBJECT, 1, 1);
    task->step = event_frog_step;
    task->draw = event_frog_draw;
    task->destructor = event_frog_destroy;
}

struct task *event_frog_create(void)
{
    struct task *task;

    task = task_create(TASK_WORK, 5, event_frog_init);
    if (task != NULL) {
        event_frog = task;
    }
    /* @hack implicit: no return, so R0 still holds task_create's result
     * (MOV R0,R4 at 0x0cb80ef0 in EV00DF), which the callers take; returned
     * explicitly, the function runs 2 bytes long and 8 of 14 words differ */
#ifdef GCC
    /* keeps the value the tag names: task_create's result */
    return task;
#endif
} /* NOLINT(clang-diagnostic-return-type) */

void event_frog_kill(void)
{
    if (event_frog != NULL) {
        task_kill(event_frog);
        event_frog = NULL;
    }
}

void event_frog_set_shape(Sint8 script)
{
    struct frog_shape *shape;

    if (event_frog != NULL) {
        shape = VARS(event_frog->work)->shape;
        if (script == 2 || script == 4) {
            shape->mode = FROG_START_PAIR;
        } else if (script == FROG_SCRIPT_RESTORE) {
            shape->mode = FROG_RESTORE;
        } else {
            shape->mode = FROG_START;
        }
        shape->script = script;
        shape->key = 0;
        shape->blend = 0.0F;
        shape->speed = 0.0F;
    }
}

/* put the points of each shape's model back as made */
static void event_frog_restore(void)
{
    Sint8 index;
    Sint8 point;

    /* @hack redundant: point set with index, though the inner loop sets it
     * before any read. SHC then copies one 0 to three registers (MOV #0,R12,
     * then MOV R12,R11 and MOV R12,R7, at 0x0cb835d6 to 0x0cb835de in
     * EV001C); without it, the restore runs 4 bytes long and 73 of 74 words
     * differ */
    for (index = point = 0; index < FROG_SHAPES; index++) {
        /* NOLINTNEXTLINE(bugprone-too-small-loop-variable) */
        for (point = 0; point < event_frog_made[index]->nbPoint; point++) {
            event_frog_models[index]->points[point].x =
                event_frog_made[index]->points[point].x;
            event_frog_models[index]->points[point].y =
                event_frog_made[index]->points[point].y;
            event_frog_models[index]->points[point].z =
                event_frog_made[index]->points[point].z;
        }
    }
}

/* Chao race hall gates, eight in an arc, one for each racing Chao */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/gate.h"
#include "al_race/scenery.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "object/simple.h"
#include "sound/cue.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"

/* the gates' actions */
extern NJS_ACTION dat_0cbd2f28;
extern NJS_ACTION dat_0cbd2f6c;
extern NJS_ACTION dat_0cbd2fb0;
extern NJS_ACTION dat_0cbd2ff4;
extern NJS_ACTION dat_0cbd3038;
extern NJS_ACTION dat_0cbd307c;
extern NJS_ACTION dat_0cbd30c0;
extern NJS_ACTION dat_0cbd3104;

/* a gate's starting values */
struct gate_kind {
    NJS_ACTION *action;
    Angle ang; /* about y */
};

/* a gate's state, in its task's motion */
struct gate {
    Sint8 mode;
    Uint8 unk_01[3];
    Float frame;
    Float last; /* the action's last frame */
    Float speed; /* added to frame each step, negative to shut */
    NJS_ACTION *action;
};

#define GATE(task) ((struct gate *)(task)->motion)

/* a Chao in front of a gate opens it, and the gate shuts once none is. a
 * Chao in the gateway sends it through the GATE_PASS modes. it then opens
 * all the way and shuts, counts the pass once, and stays shut */
enum gate_mode {
    GATE_SHUT,
    GATE_OPENING,
    GATE_OPEN,
    GATE_SHUTTING,
    GATE_PASS_OPENING,
    GATE_PASS_SHUTTING,
    GATE_PASSED,
    GATE_DONE
};

/* what a Chao touches: nothing, the box in front of the gate, or the box in the
 * gateway */
enum gate_touch { TOUCH_NONE, TOUCH_NEAR, TOUCH_IN };

/* how far a step moves the gate's action, in frames. the action's last
 * frame is 1. SHC folds the quotient to 0x3d088888 (0x0cb958b8), one below
 * 1/30's 0x3d088889, so the gate opens in 31 steps. it shuts in 30, as the
 * game's FPSCR rounds each sum toward zero */
#define GATE_SPEED (1.0F / 30.0F)

#define SE_GATE 0x273 /* a shut gate starts to open */

/* how far beyond its gate the point gate_get_far_side gives lies */
#define GATE_FAR_SIDE 10.0F

static void gate_start(struct task *task, void *arg);
static void gate_step(struct task *task);
static void gate_draw(struct task *task);
static enum gate_touch gate_touched(struct task_work *work);

static struct collision_shape gate_shapes[2];

/* the count of Chao that have gone through their gates */
static Sint8 gates_passed;

static const struct gate_kind gate_kinds[GATE_COUNT] = {
    {&dat_0cbd2f28, 0x382f},  {&dat_0cbd2f6c, 0x255d},
    {&dat_0cbd2fb0, 0x16af},  {&dat_0cbd2ff4, 0x80e},
    {&dat_0cbd3038, -0x810},  {&dat_0cbd307c, -0x1891},
    {&dat_0cbd30c0, -0x2852}, {&dat_0cbd3104, -0x34ab},
};
static const struct task_desc gate_desc = {
    sizeof(struct gate), TASK_WORK, gate_start, gate_step, gate_draw, NULL};
static const NJS_POINT3 gate_spots[GATE_COUNT] = {
#include "al_race/gate/gate_spots.inc"
};
static const struct collision_list gate_list = {gate_shapes, 2, 40.0F};
static const struct collision_kind gate_collision = {COLLISION_OBJECT, 1};

/* a box in front of the gate, and one in the gateway that pushes every group
 * but items and enemies. a Chao's record is an enemy's, so a Chao goes
 * through and a player doesn't */
static struct collision_shape gate_shapes[2] = {
    {SHAPE_RECTANGLE | SHAPE_IGNORE_OBJECTS | SHAPE_IGNORE_OTHERS,
     0,
     PUSH_FIRM(PUSH_ALL),
     {10.0F, 15.0F, 0.0F},
     10.0F,
     15.0F,
     8.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE | SHAPE_IGNORE_OBJECTS | SHAPE_IGNORE_OTHERS,
     0,
     PUSH_PLAYERS | PUSH_OBJECTS | PUSH_OTHERS | PUSH_FIRM(PUSH_ALL),
     {-10.0F, 15.0F, 0.0F},
     10.0F,
     15.0F,
     8.0F,
     {0, 0, 0},
     0,
     NULL},
};

void gates_create(void)
{
    const struct gate_kind *kind;
    /* @hack spelling: the count through a local, which SHC keeps in R12 for
     * the loop's test (0x0cb95728, 0x0cb9573a); as GATE_COUNT, SHC unrolls
     * the loop by two, and 18 of 25 words differ */
    int count = GATE_COUNT;
    int i;

    for (i = 0, kind = gate_kinds; i < count; i++, kind++) {
        scenery_add(&gate_desc, (void *)kind);
    }
    gates_passed = 0;
}

Bool gate_get_spot(NJS_POINT3 *pos, int index)
{
    /* @hack spelling: one return, of found, which joins both arms at one
     * RTS and copies R4 into R0 in its slot (0x0cb9577a to 0x0cb95782);
     * with a return in each arm, 4 of 25 words differ */
    Bool found;

    if (pos != NULL && index >= 0 && index < GATE_COUNT) {
        *pos = gate_spots[index];
        found = TRUE;
    } else {
        found = FALSE;
    }
    return found;
}

Bool gate_get_far_side(NJS_POINT3 *pos, int index)
{
    /* @hack spelling: one return, of found, which joins both arms at one
     * exit (0x0cb957e6 to 0x0cb957ec); with a return in each arm, 8 of 58
     * words differ */
    Bool found;

    if (pos != NULL && index >= 0 && index < GATE_COUNT) {
        const struct gate_kind *kind = &gate_kinds[index];
        Float *from = kind->action->object->pos;
        /* @hack type: the angle kept as a Float, which SHC converts there and
         * back (FLOAT and FTRC at 0x0cb957b2, 0x0cb957b6). every kind's ang is
         * within 0x382f of 0, which a Float holds exactly, so the cast gives it
         * back; as kind->ang, 52 of 58 words differ */
        Float ang = kind->ang;

        pos->x = from[0] - (njCos((Angle)ang) * GATE_FAR_SIDE);
        pos->y = from[1];
        pos->z = from[2] + (njSin((Angle)ang) * GATE_FAR_SIDE);
        found = TRUE;
    } else {
        found = FALSE;
    }
    return found;
}

Bool gates_all_passed(void)
{
    /* @hack spelling: one return, of all, which SHC sets with MOVT R4 and
     * copies into R0 in the RTS's slot (0x0cb95800, 0x0cb95804); with a
     * return in each arm, 2 of 7 words differ */
    Bool all;
    /* @hack spelling: the count through a local, which SHC loads into R4
     * (MOV.B @R3,R4 at 0x0cb957fc); read in the test, it takes R3, and 4 of
     * 7 words differ */
    int passed = gates_passed; /* NOLINT(bugprone-signed-char-misuse) */

    if (passed >= GATE_COUNT) {
        all = TRUE;
    } else {
        all = FALSE;
    }
    return all;
}

static void gate_start(struct task *task, void *arg)
{
    struct task_work *work = task->work;
    struct gate *gate = GATE(task);
    NJS_ACTION *action = ((struct gate_kind *)arg)->action;

    scenery_place(work, action->object);
    work->ang.y = ((struct gate_kind *)arg)->ang;
    object_collision_init(task, (struct collision_list *)&gate_list,
                          (struct collision_kind *)&gate_collision);
    gate->mode = GATE_SHUT;
    gate->frame = 0.0F;
    gate->last = action->motion->nbFrame - 1.0F;
    gate->speed = GATE_SPEED;
    gate->action = action;
}

static void gate_step(struct task *task)
{
    struct gate *gate;
    struct task_work *work;
    enum gate_touch touch;
    Bool done;

    gate_draw(task);
    gate = GATE(task);
    work = task->work;
    touch = gate_touched(work);
    switch (gate->mode) {
    case GATE_SHUT:
        switch (touch) {
        case TOUCH_NONE:
            break;
        case TOUCH_NEAR:
            gate->speed = GATE_SPEED;
            gate->mode = GATE_OPENING;
            sound_play_near(SE_GATE, &work->pos, NULL);
            break;
        case TOUCH_IN:
            gate->speed = GATE_SPEED;
            gate->mode = GATE_PASS_OPENING;
            sound_play_near(SE_GATE, &work->pos, NULL);
            break;
        }
        break;
    case GATE_OPENING:
        done = FALSE;
        gate->frame += gate->speed;
        if (gate->frame >= gate->last) {
            gate->frame = gate->last;
            done = TRUE;
        }
        switch (touch) {
        case TOUCH_NONE:
            if (done) {
                gate->speed = -GATE_SPEED;
                gate->mode = GATE_SHUTTING;
            }
            break;
        case TOUCH_NEAR:
        default:
            if (done) {
                gate->mode = GATE_OPEN;
            }
            break;
        case TOUCH_IN:
            if (done) {
                gate->speed = -GATE_SPEED;
                gate->mode = GATE_PASS_SHUTTING;
            } else {
                gate->speed = GATE_SPEED;
                gate->mode = GATE_PASS_OPENING;
            }
            break;
        }
        break;
    case GATE_OPEN:
        switch (touch) {
        case TOUCH_NONE:
            gate->speed = -GATE_SPEED;
            gate->mode = GATE_SHUTTING;
            break;
        case TOUCH_NEAR:
            break;
        case TOUCH_IN:
            gate->speed = -GATE_SPEED;
            gate->mode = GATE_PASS_SHUTTING;
            break;
        }
        break;
    case GATE_SHUTTING:
        done = FALSE;
        gate->frame += gate->speed;
        if (gate->frame <= 0.0F) {
            gate->frame = 0.0F;
            done = TRUE;
        }
        switch (touch) {
        case TOUCH_NONE:
            if (done) {
                gate->mode = GATE_SHUT;
            }
            break;
        case TOUCH_NEAR:
        default:
            gate->speed = GATE_SPEED;
            gate->mode = GATE_OPENING;
            break;
        case TOUCH_IN:
            gate->speed = GATE_SPEED;
            gate->mode = GATE_PASS_OPENING;
            break;
        }
        break;
    case GATE_PASS_OPENING:
        gate->frame += gate->speed;
        if (gate->frame >= gate->last) {
            gate->frame = gate->last;
            gate->speed = -GATE_SPEED;
            gate->mode = GATE_PASS_SHUTTING;
        }
        break;
    case GATE_PASS_SHUTTING:
        gate->frame += gate->speed;
        if (gate->frame <= 0.0F) {
            gate->frame = 0.0F;
            gate->mode = GATE_PASSED;
        }
        break;
    case GATE_PASSED:
        ++gates_passed;
        gate->mode = GATE_DONE;
        break;
    case GATE_DONE:
        break;
    }
    collision_register(work);
}

static void gate_draw(struct task *task)
{
    if (frame_step == 0 && !placement_far_from_camera(task)) {
        struct gate *gate = GATE(task);

        texture_set_stage();
        draw_action(gate->action, gate->frame);
    }
}

/* which of the gate's boxes a Chao touched this step, TOUCH_NEAR where it
 * touched both. only a COLLISION_ENEMY record counts, as a Chao's is */
static enum gate_touch gate_touched(struct task_work *work)
{
    struct collision_work *collision = work->collision;
    enum gate_touch touch = TOUCH_NONE;

    if (collision->flags & COLLISION_HIT) {
        struct collision_work *hit = collision->hit;

        if (hit != NULL) {
            /* @hack spelling: a switch of one case, which loads the first box's
             * hit into R2 (0x0cb95b58); as an if, R3 takes it, and 2 of 27
             * words differ */
            switch (hit->kind) {
            case COLLISION_ENEMY: {
                struct collision_shape *shapes = collision->shapes;

                if (shapes[0].hit != NULL) {
                    touch = TOUCH_NEAR;
                } else if (shapes[1].hit != NULL) {
                    touch = TOUCH_IN;
                }
                break;
            }
            }
        }
    }
    return touch;
}

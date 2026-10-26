/* O CRANE, a claw that drops when its switch turns on. it catches player 0,
 * carries it along a rail and lets it go */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/models.h"
#include "object/switch.h"
#include "player/player.h"
#include "stg12/crane.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the root has no model. its child is the body, which stays put as the arm
 * drops. the body's sibling is the arm, whose child and that child's sibling
 * are the claw's halves */
extern NJS_OBJECT shelter_crane_object;

/* the hook is where the claw catches. it lies HOOK_DROP below the arm, and
 * HOOK_REACH along -x, turned with the crane about y */
#define HOOK_DROP 113.0F
#define HOOK_REACH 6.5F

/* the claw catches player 0 if it is within this distance of the hook as the
 * claw shuts */
#define CATCH_RANGE 10.0F

/* the claw's angle when shut, and how far it turns each step as it shuts and
 * opens */
#define CLAW_SHUT 0x1600
#define CLAW_SPEED 224

/* each claw half's shape sits at its tip, TIP_REACH from the halves' pivot.
 * the pivot lies CLAW_PIVOT below the arm. with the claw open, the tip lies
 * TIP_ANG below level */
#define CLAW_PIVOT 73.0F
#define TIP_REACH 30.0F
#define TIP_ANG 0x1e0f
/* the center of the arm's shape lies this far above the arm */
#define ARM_LIFT 5.0F

/* how far work->collision reaches from the task. the first step sets it in
 * place of the reach collision_init measures from the shapes */
#define CRANE_REACH 300.0F

/* the steps the crane pauses before it runs a mode, counted down in
 * work->timer */
#define FIRST_PAUSE 100
#define SHORT_PAUSE 20
#define LONG_PAUSE 40

/* each step the arm drops or rises DROP_SPEED, and the crane runs
 * RUN_SPEED */
#define DROP_SPEED 1.0F
#define RUN_SPEED 2.0F

/* ground_below looks down from PROBE_DROP below the arm at its lowest, 4
 * units above the hook. the shadow lies CRANE_SHADOW_LIFT above the ground it
 * finds */
#define PROBE_DROP 109.0F
#define CRANE_SHADOW_LIFT 0.1F
/* the shadow's scale with the hook at the ground. with the hook h above the
 * ground, the scale is SHADOW_SCALE over 1 + h * SHADOW_FALLOFF */
#define SHADOW_SCALE 4.0F
#define SHADOW_FALLOFF 0.03F

/* the crane waits for its switch, drops the arm and shuts the claw. with
 * player 0 caught, it rises, runs out to the rail's far end and lets go. then
 * it opens, runs home and rests. with nothing caught, it rises, opens and
 * waits again */
enum shelter_crane_mode {
    CRANE_WAIT,
    CRANE_DROP,
    CRANE_CLOSE,
    CRANE_RISE,
    CRANE_RISE_EMPTY,
    CRANE_OPEN_EMPTY,
    CRANE_OUT,
    CRANE_LET_GO,
    CRANE_OPEN,
    CRANE_HOME,
    CRANE_REST
};

/* in work->submode while the crane carries player 0, and 0 otherwise */
#define CRANE_CARRYING 1

struct shelter_crane_vars {
    Float drop; /* the arm's y from its top: from 0 down to scale y */
    Float run; /* how far the crane has run out along the rail */
};

#define VARS(work) ((struct shelter_crane_vars *)&(work)->vars)

struct shelter_crane_motion {
    Sint16 switch_id;
    /* the hook's offset from the crane's pos in x and z. once player 0 is
     * caught, y holds player 0's height relative to the arm */
    NJS_POINT3 hook;
    /* speed_x and speed_z move the crane out along the rail each step, and
     * back home */
    Float speed_x;
    /* where the shadow lies: CRANE_SHADOW_LIFT above the ground below the hook
     * at home */
    Float ground;
    Float speed_z;
    Uint8 unk_1c[12];
    Angle claw;
    Uint8 unk_2c[8];
    Float rail; /* the rail's length: scale x without its sign */
    Float home_x;
    Float home_z;
};

#define CRANE(task) ((struct shelter_crane_motion *)(task)->motion)

static void shelter_crane_run(struct task *task);
static void shelter_crane_draw(struct task *task);
static void shelter_crane_destroy(struct task *task);

/* shapes of the arm and of the claw's two halves */
static struct collision_shape shelter_crane_shapes[3] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, ARM_LIFT, 0.0F},
     3.0F,
     80.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-6.0F, 0.0F, 0.0F},
     3.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-6.0F, 0.0F, 0.0F},
     3.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* the switches a crane can wait for. bit 0 of its x angle picks one */
static Sint16 shelter_crane_switches[2] = {7, 8};

/* raise *value by step, stopping at target. TRUE once it reaches target */
static Bool shelter_crane_raise(Float *value, Float target, Float step)
{
    Float next = *value;

    next += step;
    if (next >= target) {
        *value = target;
        return TRUE;
    }
    *value = next;
    return FALSE;
}

/* lower *value by step, stopping at target. TRUE once it reaches target */
static Bool shelter_crane_lower(Float *value, Float target, Float step)
{
    Float next = *value;

    next -= step;
    if (next <= target) {
        *value = target;
        return TRUE;
    }
    *value = next;
    return FALSE;
}

static void shelter_crane_place_shapes(struct task *task)
{
    struct task_work *work = task->work;
    struct shelter_crane_motion *motion = CRANE(task);
    struct collision_shape *shape;
    Angle ang;
    Float y;
    Float z;

    shape = work->collision->shapes;
    shape->center.y = VARS(work)->drop + ARM_LIFT;
    ang = motion->claw + TIP_ANG;
    y = (-njSin(ang) * TIP_REACH) + VARS(work)->drop - CLAW_PIVOT;
    z = njCos(ang) * TIP_REACH;
    shape = &work->collision->shapes[1];
    shape->center.y = y;
    shape->center.z = z;
    shape = &work->collision->shapes[2];
    shape->center.y = y;
    shape->center.z = -z;
}

/* the claw halves' shapes push things away. they are off while the crane
 * carries player 0 */
static void shelter_crane_claw_on(struct task_work *work)
{
    work->collision->shapes[1].flags &= ~SHAPE_OFF;
    work->collision->shapes[2].flags &= ~SHAPE_OFF;
}

static void shelter_crane_claw_off(struct task_work *work)
{
    work->collision->shapes[1].flags |= SHAPE_OFF;
    work->collision->shapes[2].flags |= SHAPE_OFF;
}

/* the crane's first step, which sets it up from its set entry. bit 0 of its x
 * angle picks the switch. scale x gives the rail's length, and by its sign the
 * direction along the crane's z. scale y gives how far the arm drops */
void shelter_crane_step(struct task *task)
{
    struct task_work *work = task->work;
    struct shelter_crane_motion *motion = CRANE(task);
    struct angles slope;
    Float cos_y;
    Float sin_y;
    Float speed;
    Uint16 which;
    Float x;
    Float y;
    Float z;

    work->timer = FIRST_PAUSE;
    work->mode = CRANE_WAIT;
    work->submode = 0;
    VARS(work)->drop = 0.0F;
    VARS(work)->run = 0.0F;
    motion->home_x = work->pos.x;
    motion->home_z = work->pos.z;
    motion->claw = 0;
    /* @hack spelling: the angle's low byte, then its mask in a statement of its
     * own, which loads the mask's 1 into R3 at 0x0c911720 for AND R3,R4 at
     * 0x0c911744. as one statement, 23 of 141 words differ */
    which = (Uint8)work->ang.x;
    which &= 1;
    motion->switch_id = shelter_crane_switches[which];
    cos_y = njCos(work->ang.y);
    sin_y = njSin(work->ang.y);
    motion->hook.x = -cos_y * HOOK_REACH;
    motion->hook.y = 0.0F;
    motion->hook.z = sin_y * HOOK_REACH;
    if (work->scl.x >= 0.0F) {
        speed = RUN_SPEED;
    } else {
        speed = -RUN_SPEED;
    }
    motion->speed_x = sin_y * speed;
    motion->speed_z = cos_y * speed;
    motion->rail = njAbs(work->scl.x);
    x = work->pos.x + motion->hook.x;
    y = work->pos.y + work->scl.y - PROBE_DROP;
    z = work->pos.z + motion->hook.z;
    motion->ground = ground_below(x, y, z, &slope) + CRANE_SHADOW_LIFT;
    if (!placement_editing(task)) {
        collision_init(task, shelter_crane_shapes, COLLISION_OBJECT, 1, 3);
        work->collision->radius = CRANE_REACH;
    }
    task->step = shelter_crane_run;
    task->draw = shelter_crane_draw;
    task->destructor = shelter_crane_destroy;
}

static void shelter_crane_run(struct task *task)
{
    struct task_work *work = task->work;
    struct shelter_crane_motion *motion = CRANE(task);
    Angle ang;
    Uint16 which;
    Float half;
    /* @hack spelling: caught declared before hook, which puts hook 4 bytes up
     * the stack, as ADD #4,R4 at 0x0c911aae shows. in the other order, the two
     * trade places and 6 of 441 words differ */
    NJS_POINT3 caught;
    NJS_POINT3 hook;

    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> SWITCH ID:");
        /* @hack spelling: the angle's low byte, then its mask in a statement of
         * its own, which keeps which in R13 for AND R3,R13 at 0x0c91185a. as
         * one statement, SHC masks R0 with AND #1 and keeps which on the stack,
         * and 18 of 441 words differ */
        which = (Uint8)work->ang.x;
        which &= 1;
        njPrintD(NJM_LOCATION(30, EDITOR_ROW_ANG_X),
                 shelter_crane_switches[which], 4);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njPushMatrix(NULL);
        njTranslate(NULL, -HOOK_REACH, work->scl.y - HOOK_DROP, 0.0F);
        draw_tree(&marker_object);
        njPopMatrix(1);
        /* draw the rail as marker_cube_object, ten across, stretched along z to
         * the rail's length */
        half = work->scl.x * 0.5F;
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, half);
        njScale(NULL, 1.0F, 1.0F, njAbs(half) * 0.2F);
        draw_tree(marker_cube_object);
        njPopMatrix(1);
        njPopMatrix(1);
        shelter_crane_draw(task);
        return;
    }
    shelter_crane_draw(task);
    if (work->timer) {
        work->timer--;
    } else {
        switch (work->mode) {
        case CRANE_WAIT:
            if (switch_is_on(motion->switch_id)) {
                work->timer = SHORT_PAUSE;
                work->mode = CRANE_DROP;
                shelter_crane_claw_on(work);
                shelter_crane_place_shapes(task);
            }
            break;
        case CRANE_DROP:
            if (shelter_crane_lower(&VARS(work)->drop, work->scl.y,
                                    DROP_SPEED)) {
                work->timer = LONG_PAUSE;
                work->mode = CRANE_CLOSE;
            }
            shelter_crane_place_shapes(task);
            break;
        case CRANE_CLOSE:
            motion->claw += CLAW_SPEED;
            if (motion->claw >= CLAW_SHUT) {
                motion->claw = CLAW_SHUT;
                hook.x = work->pos.x + motion->hook.x;
                hook.y = work->pos.y + VARS(work)->drop - HOOK_DROP;
                hook.z = work->pos.z + motion->hook.z;
                if (player_in_range(&hook, CATCH_RANGE) == 1) {
                    player_set_mode(0, PLAYER_MODE_STILL);
                    player_get_history(0, 0, &caught, NULL);
                    motion->hook.y = caught.y - work->pos.y - VARS(work)->drop;
                    work->mode = CRANE_RISE;
                    work->submode = CRANE_CARRYING;
                    shelter_crane_claw_off(work);
                } else {
                    work->mode = CRANE_RISE_EMPTY;
                }
                work->timer = SHORT_PAUSE;
            }
            shelter_crane_place_shapes(task);
            break;
        case CRANE_RISE:
            if (shelter_crane_raise(&VARS(work)->drop, 0.0F, DROP_SPEED)) {
                work->timer = LONG_PAUSE;
                VARS(work)->run = 0.0F;
                work->mode = CRANE_OUT;
            }
            shelter_crane_place_shapes(task);
            break;
        case CRANE_RISE_EMPTY:
            if (shelter_crane_raise(&VARS(work)->drop, 0.0F, DROP_SPEED)) {
                work->mode = CRANE_OPEN_EMPTY;
            }
            shelter_crane_place_shapes(task);
            break;
        case CRANE_OPEN_EMPTY:
            motion->claw -= CLAW_SPEED;
            if (motion->claw <= 0) {
                motion->claw = 0;
                work->mode = CRANE_WAIT;
                work->timer = SHORT_PAUSE;
            }
            shelter_crane_place_shapes(task);
            break;
        case CRANE_OUT:
            if (shelter_crane_raise(&VARS(work)->run, motion->rail,
                                    RUN_SPEED)) {
                Float rail = work->scl.x;
                Angle ang_y = work->ang.y;

                work->pos.x = motion->home_x + (rail * njSin(ang_y));
                work->pos.z = motion->home_z + (rail * njCos(ang_y));
                work->mode = CRANE_LET_GO;
                work->timer = LONG_PAUSE;
            } else {
                work->pos.x += motion->speed_x;
                work->pos.z += motion->speed_z;
            }
            break;
        case CRANE_LET_GO:
            work->submode = 0;
            player_set_mode(0, PLAYER_MODE_RESET);
            work->mode = CRANE_OPEN;
            shelter_crane_claw_on(work);
            break;
        case CRANE_OPEN:
            motion->claw -= CLAW_SPEED;
            if (motion->claw <= 0) {
                motion->claw = 0;
                work->mode = CRANE_HOME;
                work->timer = SHORT_PAUSE;
            }
            shelter_crane_place_shapes(task);
            break;
        case CRANE_HOME:
            if (shelter_crane_lower(&VARS(work)->run, 0.0F, RUN_SPEED)) {
                work->pos.x = motion->home_x;
                work->pos.z = motion->home_z;
                work->mode = CRANE_REST;
            } else {
                work->pos.x -= motion->speed_x;
                work->pos.z -= motion->speed_z;
            }
            break;
        case CRANE_REST:
            work->timer = LONG_PAUSE;
            work->mode = CRANE_WAIT;
            break;
        }
        if (work->submode == CRANE_CARRYING) {
            player_set_position(0, work->pos.x + motion->hook.x,
                                work->pos.y + motion->hook.y + VARS(work)->drop,
                                work->pos.z + motion->hook.z);
        }
    }
    collision_register(work);
}

static void shelter_crane_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct shelter_crane_motion *motion = CRANE(task);
    NJS_OBJECT *body;
    NJS_OBJECT *arm;
    NJS_OBJECT *claw;
    NJS_OBJECT *other;
    Angle ang;
    Float x;
    Float y;
    Float z;
    Float ground;
    Float scale;

    if (frame_step != 0) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    body = shelter_crane_object.child;
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(body->model);
    arm = body->sibling;
    njTranslate(NULL, 0.0F, VARS(work)->drop, 0.0F);
    draw_model(arm->model);
    claw = arm->child;
    njPushMatrix(NULL);
    njTranslate(NULL, claw->pos[0], claw->pos[1], claw->pos[2]);
    TURN_VIA(njRotateX, -motion->claw, ang);
    draw_model(claw->model);
    njPopMatrix(1);
    other = claw->sibling;
    njTranslate(NULL, other->pos[0], other->pos[1], other->pos[2]);
    TURN_VIA(njRotateX, motion->claw, ang);
    draw_model(other->model);
    njPopMatrix(1);
    x = work->pos.x + motion->hook.x;
    y = work->pos.y + VARS(work)->drop - HOOK_DROP;
    z = work->pos.z + motion->hook.z;
    ground = motion->ground;
    scale = SHADOW_SCALE / (1.0F + ((y - ground) * SHADOW_FALLOFF));
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslate(NULL, x, ground, z);
    njScale(NULL, scale, 1.0F, scale);
    draw_tree(shadow_object);
    njPopMatrix(1);
}

static void shelter_crane_destroy(struct task *task)
{
}

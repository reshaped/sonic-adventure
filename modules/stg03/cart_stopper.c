/* the cart stopper, which the set editor names O CART STOPPER. once a player
 * touches it, it pulls in the cart player 0 rides. it carries the cart
 * through its action, then lets it go */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "object/simple.h"
#include "sound/cue.h"
#include "sound/sound.h"
#include "stg03/cart.h"
#include "stg03/cart_stopper.h"
#include "stg03/shareobj_textures.h"
#include "stg03/stopper_object.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/util.h"

extern NJS_ACTION stopper_action;
extern NJS_OBJECT stopper_frame_object;

/* the most the pull turns the cart about each axis in a step */
#define STOPPER_TURN NJM_DEG_ANG(10)
/* the pull moves the cart at most PULL_STEP a step, toward a point
 * PULL_HEIGHT above the stopper's position */
#define PULL_STEP 1.0F
#define PULL_HEIGHT 1.0F

/* the nodes of the stopper's object, which the action moves */
#define STOPPER_NODES 3

/* stopper_init's argument, as cart_stopper_start passes it */
struct stopper_arg {
    NJS_ACTION *action;
    NJS_TEXLIST *texlist;
    struct collision_list shapes;
    NJS_OBJECT *object; /* the object the collision world takes a copy of */
};

/* the argument as stopper_init reads it. its radius and frame are
 * stopper_radius and stopper_frame, which lie right after stopper_arg */
struct stopper_init_arg {
    struct stopper_arg arg;
    /* the radius within which object_world_test_radius looks for players */
    Float radius;
    NJS_OBJECT *frame; /* a still object stopper_draw draws with the action */
};

#define INIT_ARG(arg) ((struct stopper_init_arg *)(arg))

/* the stopper's record, which task_apply_desc puts in task->motion */
struct stopper {
    Sint8 mode; /* a stopper_mode */
    Float frame; /* the action's frame stopper_draw draws */
    Float next_frame; /* the frame the next step draws */
    Float speed; /* the frames the action moves on each step it carries */
    Float last_frame;
    NJS_ACTION *action;
    NJS_TEXLIST *texlist;
    struct collision_list *shapes;
    NJS_OBJECT *world; /* the stopper's object in the collision world */
    /* the action's root in the world at next_frame. the world object stands
     * there from the next step on, and a carried cart moves as it moves */
    NJS_POINT3 hold;
    Float radius;
    NJS_OBJECT *frame_object;
    struct task *cart; /* the cart the stopper holds */
};

#define STOPPER(task) ((struct stopper *)(task)->motion)

enum stopper_mode {
    STOPPER_WAIT,
    STOPPER_PULL,
    STOPPER_CARRY,
    STOPPER_RELEASE,
    STOPPER_DONE
};

static void stopper_init(struct task *task, void *arg);
static void stopper_step(struct task *task);
static void stopper_draw(struct task *task);
static void stopper_destroy(struct task *task);
static void stopper_hold_point(NJS_POINT3 *out, struct task_work *work,
                               NJS_ACTION *action, Float frame);
static void stopper_place_world(NJS_OBJECT *object, const Float *pos,
                                const Angle *ang);
static struct task *stopper_find_cart(struct task_work *work);
static Bool stopper_pull_cart(struct task *task);
static Bool stopper_carry_cart(struct task *task);
static Bool stopper_turn_toward(Angle *ang, Angle target);

static struct collision_shape stopper_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 25.0F, 0.0F},
    25.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

/* @hack type: stopper_arg, stopper_radius and stopper_frame defined apart,
 * where stopper_init reads them as one stopper_init_arg. cart_stopper_start
 * then loads the last two from pool words of their own (0x0c90c380,
 * 0x0c90c38e). defined as one stopper_init_arg, the function loads its
 * address once, is 2 bytes longer, and 10 of 14 words differ */
#ifdef GCC
/* keeps the read the tag names: GCC's three are the members of one
 * stopper_init_arg */
static struct stopper_init_arg stopper_args = {
    {
        &stopper_action,
        &obj_shareobj_texlist,
        {&stopper_shape, 1, 40.0F},
        &stopper_object,
    },
    0.0F,
    &stopper_frame_object,
};
#define stopper_arg stopper_args.arg
#define stopper_radius stopper_args.radius
#define stopper_frame stopper_args.frame
#else
static struct stopper_arg stopper_arg = {
    &stopper_action,
    &obj_shareobj_texlist,
    {&stopper_shape, 1, 40.0F},
    &stopper_object,
};
static Float stopper_radius = 0.0F;
static NJS_OBJECT *stopper_frame = &stopper_frame_object;
#endif

static const struct task_desc stopper_desc = {
    sizeof(struct stopper), TASK_WORK | TASK_FORCE, stopper_init,
    stopper_step,           stopper_draw,           stopper_destroy,
};
static const struct collision_kind stopper_kind = {COLLISION_OBJECT, 1};

/* njGetAction's pose of each node: its position, angles and scale */
static NJS_POINT3 stopper_pos[STOPPER_NODES];
static Angle stopper_ang[STOPPER_NODES][3];
static NJS_POINT3 stopper_scl[STOPPER_NODES];
static const NJS_MOTION_DATA stopper_motion = {
    &stopper_pos[0].x, stopper_ang[0], &stopper_scl[0].x};

static void stopper_turn_x(struct angles *ang)
{
    Angle turn;

    TURN_VIA(njRotateX, ang->x, turn);
}

static void stopper_turn_y(struct angles *ang)
{
    Angle turn;

    TURN_VIA(njRotateY, ang->y, turn);
}

static void stopper_turn_z(struct angles *ang)
{
    Angle turn;

    TURN_VIA(njRotateZ, ang->z, turn);
}

void cart_stopper_start(struct task *task)
{
    stopper_radius = stopper_frame->model->r * 2.0F;
    task_apply_desc(task, &stopper_desc, &stopper_arg);
}

/* the descriptor's init. it puts a copy of arg's object in the collision
 * world, at the action's root for frame 0. the copy takes the stopper's
 * angles. init gives the task a collision record unless the set editor
 * holds the stopper. with no world object to lend, it kills the task */
static void stopper_init(struct task *task, void *arg)
{
    NJS_OBJECT *world = object_world_copy(INIT_ARG(arg)->arg.object);
    struct stopper *stopper = STOPPER(task);

    stopper->world = world;
    if (world == NULL) {
        task_kill(task);
    } else {
        struct task_work *work;
        NJS_ACTION *action;
        struct collision_list *shapes;
        /* @hack type: the hold point through a Float pointer, cast back for
         * stopper_hold_point, which forms its address once in R8 (ADD #36,R8
         * at 0x0c90c406); as &stopper->hold, 114 of 117 words differ */
        Float *hold;
        /* @hack spelling: the editor's test through a local, which writes
         * the image's MOVT R4 (0x0c90c3f4); tested in place, 82 of 117 words
         * differ */
        Bool live;
        /* @hack spelling: 0 through a local, which keeps a zero in FR15
         * across the calls (FLDI0 FR15 at 0x0c90c3ea); as the literal, 110 of
         * 117 words differ */
        Float frame;

        action = INIT_ARG(arg)->arg.action;
        work = task->work;
        shapes = &INIT_ARG(arg)->arg.shapes;
        frame = 0.0F;
        live = !placement_editing(task);
        if (live) {
            object_collision_init(task, shapes,
                                  (struct collision_kind *)&stopper_kind);
        }
        hold = &stopper->hold.x;
        stopper_hold_point((NJS_POINT3 *)hold, work, action, frame);
        stopper_place_world(world, hold, &work->ang.x);
        object_world_add_sized(task, world, WORLD_MOVES | WORLD_SOLID, 1.0F);
        object_force_clear(task->force);
        stopper->mode = STOPPER_WAIT;
        stopper->next_frame = stopper->frame = frame;
        stopper->speed = 1.0F;
        stopper->last_frame = action->motion->nbFrame - 1.0F;
        stopper->action = action;
        stopper->texlist = INIT_ARG(arg)->arg.texlist;
        stopper->shapes = shapes;
        stopper->radius = INIT_ARG(arg)->radius;
        stopper->frame_object = INIT_ARG(arg)->frame;
        stopper->cart = NULL;
    }
}

static void stopper_step(struct task *task)
{
    struct task_work *work = task->work;
    struct stopper *stopper = STOPPER(task);
    struct task *cart;
    Float *pos;
    NJS_POINT3 *hold;

    if (!placement_editing(task)) {
        pos = stopper->world->pos;
        stopper->frame = stopper->next_frame;
        hold = &stopper->hold;
        pos[0] = hold->x;
        pos[1] = hold->y;
        pos[2] = hold->z;
        object_world_test_radius(work, stopper->radius);
        switch (stopper->mode) {
        case STOPPER_WAIT:
        default:
            cart = stopper_find_cart(work);
            if (cart != NULL) {
                stopper->cart = cart;
                stopper->mode = STOPPER_PULL;
            }
            collision_register(work);
            break;
        case STOPPER_PULL:
            if (stopper_pull_cart(task)) {
                sound_play_near(SE_CLUNK, &work->pos, NULL);
                stopper->mode = STOPPER_CARRY;
            }
            break;
        case STOPPER_CARRY:
            if (stopper_carry_cart(task)) {
                stopper->mode = STOPPER_RELEASE;
            }
            break;
        case STOPPER_RELEASE:
            cart_release(stopper->cart);
            sound_play_near(SE_SPRING, &work->pos, NULL);
            stopper->mode = STOPPER_DONE;
            break;
        case STOPPER_DONE:
            break;
        }
    }
    stopper_draw(task);
}

static void stopper_draw(struct task *task)
{
    struct task_work *work;
    struct stopper *stopper;
    struct angles *ang;

    if (frame_step == 0 && !placement_far_from_camera(task)) {
        work = task->work;
        stopper = STOPPER(task);
        ang = &work->ang;
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        stopper_turn_z(ang);
        stopper_turn_y(ang);
        stopper_turn_x(ang);
        njSetTexture(stopper->texlist);
        draw_action(stopper->action, stopper->frame);
        draw_tree(stopper->frame_object);
        njPopMatrix(1);
    }
}

static void stopper_destroy(struct task *task)
{
    struct stopper *stopper = STOPPER(task);

    object_world_free(task, stopper->world);
}

/* store in out the world position of the action's root at frame */
static void stopper_hold_point(NJS_POINT3 *out, struct task_work *work,
                               NJS_ACTION *action, Float frame)
{
    NJS_POINT3 root;
    NJS_POINT3 at;
    struct angles *ang;

    njGetAction(action, (NJS_MOTION_DATA *)&stopper_motion, frame);
    root.x = stopper_pos[0].x;
    root.y = stopper_pos[0].y;
    root.z = stopper_pos[0].z;
    ang = &work->ang;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    stopper_turn_z(ang);
    stopper_turn_y(ang);
    stopper_turn_x(ang);
    njCalcPoint(NULL, &root, &at);
    njPopMatrix(1);
    out->x = at.x;
    out->y = at.y;
    out->z = at.z;
}

static void stopper_place_world(NJS_OBJECT *object, const Float *pos,
                                const Angle *ang)
{
    Float *to = object->pos;
    Angle *ang_to;

    to[0] = pos[0];
    to[1] = pos[1];
    to[2] = pos[2];
    ang_to = object->ang;
    ang_to[0] = ang[0];
    ang_to[1] = ang[1];
    ang_to[2] = ang[2];
    to = object->scl;
    to[0] = 1.0F;
    to[1] = 1.0F;
    to[2] = 1.0F;
}

/* if a player's record touched the stopper's this step, return what
 * cart_hold returns. otherwise return null */
static struct task *stopper_find_cart(struct task_work *work)
{
    struct task *cart = NULL;
    struct collision_work *collision = work->collision;
    struct collision_work *hit;

    /* @bug memory: a stopper that started while the set editor held it has no
     * collision record. this then reads the record's flags through null
     * (0x0c90c740) */
    if (collision->flags & COLLISION_HIT) {
        hit = collision->hit;
        if (hit != NULL) {
            switch (hit->kind) {
            case COLLISION_PLAYER:
                cart = cart_hold();
                break;
            }
        }
    }
    return cart;
}

/* move the cart PULL_STEP at most toward the point PULL_HEIGHT above the
 * stopper. turn it, by STOPPER_TURN at most about each axis, to the
 * stopper's angles turned half round in y. return TRUE once it is at the
 * point and the angles, else FALSE */
static Bool stopper_pull_cart(struct task *task)
{
    struct task_work *work = task->work;
    struct stopper *stopper = STOPPER(task);
    struct task *cart = stopper->cart;
    struct task_work *cart_work = cart->work;
    NJS_POINT3 pos;
    struct angles ang;
    NJS_VECTOR to;
    NJS_POINT3 *at;
    struct angles *facing;
    Float distance;
    Bool there = TRUE;

    pos = cart_work->pos;
    ang = cart_work->ang;
    at = &work->pos;
    to.x = at->x - pos.x;
    to.y = at->y - pos.y + PULL_HEIGHT;
    to.z = at->z - pos.z;
    distance = njScalor2(&to);
    if (distance <= PULL_STEP * PULL_STEP) {
        njAddVector(&pos, &to);
    } else {
        there = FALSE;
        distance = njInvertSqrt(distance) * PULL_STEP;
        pos.x += to.x * distance;
        pos.y += to.y * distance;
        pos.z += to.z * distance;
    }
    facing = &work->ang;
    if (!stopper_turn_toward(&ang.x, facing->x)) {
        there = FALSE;
    }
    if (!stopper_turn_toward(&ang.y, facing->y + 0x8000)) {
        there = FALSE;
    }
    if (!stopper_turn_toward(&ang.z, facing->z)) {
        there = FALSE;
    }
    cart_place(cart, &pos, &ang);
    return there;
}

/* move the action on by speed, and the cart by as much as the hold point
 * moves. return TRUE once the action reaches its last frame, else FALSE */
static Bool stopper_carry_cart(struct task *task)
{
    struct task_work *work = task->work;
    struct stopper *stopper = STOPPER(task);
    struct task *cart;
    struct task_work *cart_work;
    NJS_POINT3 pos;
    struct angles ang;
    Float *was;
    NJS_POINT3 *hold;
    Bool done = FALSE;

    stopper->next_frame = stopper->frame + stopper->speed;
    if (stopper->next_frame >= stopper->last_frame) {
        stopper->next_frame = stopper->last_frame;
        done = TRUE;
    }
    cart = stopper->cart;
    cart_work = cart->work;
    was = stopper->world->pos;
    hold = &stopper->hold;
    pos = cart_work->pos;
    ang = cart_work->ang;
    stopper_hold_point(hold, work, stopper->action, stopper->next_frame);
    pos.x += hold->x - was[0];
    pos.y += hold->y - was[1];
    pos.z += hold->z - was[2];
    cart_place(cart, &pos, &ang);
    return done;
}

/* turn *ang toward target the short way round, by STOPPER_TURN at most.
 * return TRUE once it is there, else FALSE */
static Bool stopper_turn_toward(Angle *ang, Angle target)
{
    Angle from;
    Sint16 turn;
    /* @hack redundant: distance declared register, which keeps turn's
     * sign-extended copy in R0 and distance in R6 (0x0c90c98c, 0x0c90c996);
     * without it, the two trade and 5 of 23 words differ */
    register int distance;
    int step;
    Bool there;

    from = *ang;
    turn = target - from;
    /* @hack redundant: a never-run store of from, which keeps the image's
     * register allocation: turn in R7 and its sign-extended copy in R0
     * (0x0c90c988 to 0x0c90c98e); without it, 15 of 23 words differ */
    if (0) {
        *ang = from;
    }
    if (turn >= 0) {
        step = STOPPER_TURN;
        distance = turn;
    } else {
        step = -STOPPER_TURN;
        distance = -turn;
    }
    /* @hack redundant: a goto to the next line, which loads STOPPER_TURN into
     * R2 for the test (0x0c90c9c4) and the sum into R3 (0x0c90c9d0); without
     * it, the two trade and 5 of 23 words differ */
    goto compare;
compare:
    if (distance <= STOPPER_TURN) {
        *ang = target;
        there = TRUE;
    } else {
        *ang += step;
        there = FALSE;
    }
    return there;
}

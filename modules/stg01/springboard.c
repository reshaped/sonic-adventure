/* O GOMBAN springboard, a board that bends under a player and springs
 * back */
#include "stg01/springboard.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <string.h>

/* the board's state, in work->vars. work->submode holds the number of the
 * player it pushes, and work->tick its chain id. scl.z holds the speed of its
 * deflection. work->timer counts the steps since a player last touched it, up
 * to 0x7fff */
struct springboard_vars {
    /* the heading the player came in at, in the board's frame, mirrored
     * across its x axis. a throw sends the player the opposite way, which
     * reflects it off the board */
    Uint16 aim;
    Uint16 flags;
    /* how far the board's middle is pushed along its x axis, to meet the
     * player's ball */
    Float deflection;
    /* where along its z axis, its span, the board's bend peaks */
    Float apex;
};

/* vars' flags. scl.x picks SPRING_REFLECT, both reflect flags, or
 * SPRING_RUBBER. scl.y picks STRETCH_RUBBER, STRETCH_CORD or neither.
 * nothing reads either rubber flag. SPRING_FEEDBACK and SPRING_BOUNCED hold
 * for one push: the board clears them each step it pushes no player */
#define SPRING_REFLECT 0x8 /* the board throws a player */
/* the throw takes one of four headings 45 degrees apart */
#define SPRING_REFLECT_45 0x10
#define SPRING_RUBBER 0x4
#define STRETCH_RUBBER 0x1
#define STRETCH_CORD 0x2 /* the push grows as the stretch squared */
#define SPRING_FEEDBACK 0x40 /* the push has played its sound and vibration */
#define SPRING_BOUNCED 0x80 /* the push has thrown its player */

/* a push sounds as the board turns back from a deflection past
 * FEEDBACK_DEFLECTION. it throws its player as the board turns back from
 * past THROW_DEFLECTION. a throw needs the player to move faster than
 * THROW_SPEED */
#define FEEDBACK_DEFLECTION 0.8F
#define THROW_DEFLECTION 1.0F
#define THROW_SPEED 0.2F

/* the sound a push plays as it sets SPRING_FEEDBACK */
#define SE_SPRINGBOARD 46
/* the push's vibration grows with the deflection, up to POWER_MOST */
#define POWER_MOST 6

/* SPRING_REFLECT_45 masks the aim with AIM_45_MASK. the mask leaves one of four
 * headings, 0 to 135 degrees in steps of 45 */
#define AIM_45_MASK 0x6000
/* a throw turns toward the chain's next board only within NEXT_REACH of the
 * throw's heading */
#define NEXT_REACH NJM_DEG_ANG(67.5)
/* a thrown player is beyond its control for LAUNCH_STEPS steps */
#define LAUNCH_STEPS 30

/* the board's half length, from its middle to either end, along z */
#define HALF_SPAN 34.0F

/* the points of the board's strip, bent or not */
#define BOARD_POINTS 15
/* at a deflection of -BENT_DEFLECTION, the board takes its bent shape */
#define BENT_DEFLECTION 4.95F
/* the board's points within this of its middle along z move with the apex */
#define APEX_REACH 3.5F

/* at most this many boards with a chain id are listed */
#define CHAIN_BOARDS 20

/* the record's radius takes in its shapes: the cylinder's distance from the
 * task, plus SHAPES_REACH */
#define SHAPES_REACH 39.0F

#define SPRING(work) ((struct springboard_vars *)&(work)->vars)

static const char reflect_bane45[20] = "ReflectMode: BANE45";
static const char reflect_bane[20] = "ReflectMode: BANE";
static const char reflect_gomu[20] = "ReflectMode: GOMU";
static const char stretch_himo[20] = "Stretchmode: HIMO";
static const char stretch_gomu[20] = "Stretchmode: GOMU";
static const char chain_label[4] = "ID:";
/* the board's middle at rest, where shape 2 sits, in the springboard's frame.
 * the board's frame has its origin here */
static const NJS_POINT3 board_middle = {4.12F, 8.4F, 0.0F};

/* the board pushing a player, or null. only one board pushes at a time */
static struct task *active_springboard = NULL;
/* the boards started and not yet destroyed */
static int springboard_count = 0;
/* capsules at the board's two ends, and a cylinder over the board, which
 * moves as the board bends */
static struct collision_shape springboard_shapes[3] = {
    {SHAPE_CAPSULE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {1.25F, 6.0F, 35.25F},
     3.0F,
     4.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CAPSULE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {1.25F, 6.0F, -35.25F},
     3.0F,
     4.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL),
     {4.12F, 8.4F, 0.0F},
     34.0F,
     3.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL}};

/* the started boards with a chain id, null where free. a board that starts
 * while no other runs clears it */
static struct task_work *springboards[CHAIN_BOARDS];

static void springboard_init(struct task *task);
static void springboard_destroy(struct task *task);
static void springboard_run(struct task *task);
static void springboard_draw(struct task *task);

/* whether a player's ball, at position with radius and moving at velocity,
 * touches the board. a board untouched on the previous step picks the side
 * from the ball's x against its x speed. it also sets *aim from the ball's
 * heading. a board touched then keeps the side of that touch. a touch sets
 * *deflection to put the board's face against the ball. it sets *apex to the
 * ball's z in the board's frame */
static int contact(NJS_POINT3 *position, Float radius, NJS_VECTOR *velocity,
                   NJS_POINT3 *unused, Float *deflection, Float *apex,
                   struct task_work *work, int *aim)
{
    int hit = 0;
    NJS_POINT3 local_position;
    NJS_VECTOR local_velocity;
    Angle angle;
    njPushMatrix(&_unit_matrix_);
    njTranslate(0, -4.12F, -8.4F, -0.0F);
    TURN_VIA(njRotateY, -work->ang.y, angle);
    njTranslate(0, -work->pos.x, -work->pos.y, -work->pos.z);
    njCalcVector(0, velocity, &local_velocity);
    njCalcPoint(0, position, &local_position);
    njPopMatrix(1);
    if (TASK_TIMER(work) > 1) {
        if (local_position.x <= radius && local_position.x > local_velocity.x) {
            *deflection = local_position.x - radius;
            hit = 1;
            *aim = (int)(atan2f(local_velocity.z, local_velocity.x) *
                         ANGLE_PER_RADIAN);
        } else if (local_position.x >= -radius &&
                   local_position.x < local_velocity.x) {
            *deflection = local_position.x + radius;
            hit = 1;
            *aim = (int)(atan2f(local_velocity.z, local_velocity.x) *
                         ANGLE_PER_RADIAN);
        }
    } else if (SPRING(work)->deflection < 0.0F) {
        if (local_position.x <= radius) {
            *deflection = local_position.x - radius;
            hit = 1;
        }
    } else {
        if (local_position.x >= -radius) {
            *deflection = local_position.x + radius;
            hit = 1;
        }
    }
    if (!hit) {
        return 0;
    }
    *apex = local_position.z;
    return 1;
}

static void read_settings(Float reflect, Float stretch, struct task_work *work)
{
    SPRING(work)->flags = 0;
    switch ((Sint8)reflect % 3) {
    case 0:
        SPRING(work)->flags |= SPRING_REFLECT;
        break;
    case 1:
        SPRING(work)->flags |= SPRING_REFLECT;
        SPRING(work)->flags |= SPRING_REFLECT_45;
        break;
    case 2:
        SPRING(work)->flags |= SPRING_RUBBER;
        break;
    }
    switch ((Sint8)stretch % 3) {
    case 0:
        SPRING(work)->flags |= STRETCH_RUBBER;
        break;
    case 1:
        SPRING(work)->flags |= STRETCH_CORD;
        break;
    }
}

void beach_springboard_step(struct task *task)
{
    /* @hack spelling: current, a copy of task, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c90536a). without it, 10 of 14
     * words differ and springboard_init moves */
    struct task *current = task;
    if (!placement_try_leave(current)) {
        springboard_init(current);
    }
}

static void springboard_init(struct task *task)
{
    struct task_work *work = task->work;
    int i;
    if (!placement_editing(task)) {
        collision_init(task, springboard_shapes, COLLISION_OBJECT, 1, 3);
    }
    task->draw = springboard_draw;
    task->step = springboard_run;
    task->destructor = springboard_destroy;
    read_settings(work->scl.x, work->scl.y, work);
    work->tick = (Sint8)work->scl.z;
    work->timer = 2;
    SPRING(work)->deflection = 0.0F;
    SPRING(work)->apex = 0.0F;
    work->scl.z = 0.0F;
    if (springboard_count++ == 0) {
        memset((void *)springboards, 0, sizeof(springboards));
    }
    if (work->tick) {
        /* @hack redundant: a goto to the next statement, which loads each slot
         * into R2 rather than R3 (MOV.L @R5,R2 at 0x0c9053fc). without it, 2 of
         * 77 words differ */
        goto entries;
    entries:
        for (i = 0; i < CHAIN_BOARDS; ++i) {
            if (springboards[i] == 0) {
                springboards[i] = work;
                return;
            }
        }
    }
}

static void springboard_destroy(struct task *task)
{
    struct task_work *work = task->work;
    int i;
    --springboard_count;
    for (i = 0; i < CHAIN_BOARDS; ++i) {
        if (springboards[i] == work) {
            springboards[i] = 0;
        }
    }
    if (active_springboard == task) {
        active_springboard = 0;
    }
}

/* push a player that touches the board. the board bends to meet the player's
 * ball. it pushes the player toward its middle, harder as it stretches more.
 * untouched, the board springs back to rest */
static void springboard_run(struct task *task)
{
    struct task_work *work = task->work;
    Float deflection = SPRING(work)->deflection;
    Float apex = SPRING(work)->apex;
    Float velocity = work->scl.z;
    Sint8 number = work->submode;
    Sint16 timer = work->timer;
    struct task_work *rider = player_work[number];
    struct player *player = player_get(number);
    NJS_POINT3 point;
    NJS_POINT3 difference;
    Angle angle;
    int untouched;
    int aim;
    Angle launch_aim;
    Float x;
    Float z;
    Float length;
    Float factor;
    Float radius;
    Sint16 power;
    int i;
    struct task_work *next;

    /* @hack redundant: a never-run test of timer, which gives timer R12, as in
     * the image; without it, 48 of 836 words differ */
    if (0) {
        if (timer) {
        }
    }
    /* @hack redundant: the same for player, which gives player R11; without it,
     * 39 of 836 words differ */
    if (0) {
        if (player) {
        }
    }
    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        read_settings(work->scl.x, work->scl.y, work);
        work->tick = (Sint8)work->scl.z;
        work->timer = 0;
        SPRING(work)->deflection = 0.0F;
        SPRING(work)->apex = 0.0F;
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X),
                 (SPRING(work)->flags & SPRING_REFLECT)
                     ? (SPRING(work)->flags & SPRING_REFLECT_45 ? reflect_bane45
                                                                : reflect_bane)
                     : reflect_gomu);
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y),
                 SPRING(work)->flags & STRETCH_CORD ? stretch_himo
                                                    : stretch_gomu);
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), chain_label);
        njPrintD(NJM_LOCATION(22, EDITOR_ROW_SCL_Z), work->tick, 2);
        springboard_draw(task);
        return;
    }
    springboard_draw(task);
    if ((work->collision->flags & COLLISION_HIT) &&
        work->collision->own_shape == 2 &&
        (timer > 1 || work->collision->hit->owner->work == rider)) {
        aim = SPRING(work)->aim;
        if (timer > 1) {
            number = player_nearest(&work->pos);
            rider = player_work[number];
            player = player_get(number);
        }
        if (contact(&rider->pos, player->consts.radius * 0.9F,
                    &player_motion[number]->speed, &point, &deflection, &apex,
                    work, &aim)) {
            timer = 0;
        }
        SPRING(work)->aim = aim;
    }
    /* @hack spelling: timer copied to untouched, an int, which the image
     * keeps at sp+8 for the second test (MOV.L R3,@(8,R15) at 0x0c9056b0);
     * tested as timer, 147 of 836 words differ */
    untouched = timer;
    if (!untouched || deflection > 0.1F || apex > 0.1F) {
        work->collision->shapes[2].center.x =
            springboard_shapes[2].center.x + deflection;
        work->collision->shapes[2].center.z =
            springboard_shapes[2].center.z + apex;
    }
    x = work->collision->shapes[2].center.x;
    z = work->collision->shapes[2].center.z;
    radius = njSqrt((x * x) + (z * z));
    radius += SHAPES_REACH;
    work->collision->radius = radius;
    collision_register(work);
    if (untouched > 0 ||
        (active_springboard != 0 && active_springboard != task)) {
        if (active_springboard == task) {
            active_springboard = 0;
        }
        if (njAbs(apex) < 0.1F) {
            apex = 0.0F;
        }
        if (njAbs(velocity) < 0.1F) {
            velocity = 0.0F;
        }
        if (apex == 0.0F && velocity == 0.0F && njAbs(deflection) < 0.1F) {
            deflection = 0.0F;
        } else {
            apex *= 0.92F;
            velocity += -deflection * 0.5F;
            velocity *= 0.95F;
            deflection += velocity;
        }
        SPRING(work)->flags &= ~SPRING_BOUNCED;
        SPRING(work)->flags &= ~SPRING_FEEDBACK;
    } else {
        /* @hack redundant: the second test of deflection, which repeats the
         * first and so never decides; the image tests it twice (0x0c905840,
         * 0x0c905848). without it, the function shrinks, springboard_draw
         * moves, and 514 of 836 words differ */
        /* NOLINTNEXTLINE(misc-redundant-expression) */
        if (njAbs(deflection) > 0.1F || njAbs(deflection) > 0.1F) {
            /* the point the player is pushed toward. it lies 0.9 of the
             * player's radius out from the board's middle, on the player's
             * side. along z it lies 0.8 of the way to the apex. board_middle's
             * z is 0, so both arms below set the same z */
            point = board_middle;
            if (deflection < 0.0F) {
                point.x += player->consts.radius * 0.9F;
            } else {
                point.x -= player->consts.radius * 0.9F;
            }
            if (velocity < 0.0F) {
                point.z += apex * 0.8F;
            } else {
                point.z = apex * 0.8F;
            }
            /* the stretch: the two halves' lengths, from the apex to either
             * end, less the board's length at rest */
            x = HALF_SPAN - apex;
            z = -HALF_SPAN - apex;
            length = njSqrt((deflection * deflection) + (x * x)) +
                     njSqrt((deflection * deflection) + (z * z)) -
                     (2.0F * HALF_SPAN);

            /* @hack redundant: a never-run test of x, which puts x in FR15
             * ahead of z in FR13; without it, 13 of 836 words differ */
            if (0) {
                if (x) {
                }
            }
            /* velocity, the deflection's speed, takes the push's speed here.
             * the step sets it back to the deflection's speed before it
             * stores it in scl.z */
            if (SPRING(work)->flags & STRETCH_CORD) {
                velocity = length * length * 0.02F;
            } else {
                velocity = length * 0.02F;
            }
            njPushMatrix(&_unit_matrix_);
            njTranslateV(0, &work->pos);
            TURN_VIA(njRotateY, work->ang.y, angle);
            njCalcPoint(0, &point, &point);
            njPopMatrix(1);
            {
                /* @hack spelling: out, from and other, byte cursors over
                 * difference, point and the rider's pos, which keep task in R10
                 * (MOV R4,R10 at 0x0c9054e0). without them, 371 of 836 words
                 * differ */
                unsigned char *out = (unsigned char *)&difference;
                unsigned char *from = (unsigned char *)&point;
                unsigned char *other = (unsigned char *)&rider->pos;

                /* @hack redundant: a never-run test of other, which puts other
                 * in R4, from in R5 and out in R6; without it, 11 of 836 words
                 * differ */
                if (0) {
                    if (other) {
                    }
                }
                *(Float *)out = *(Float *)from - *(Float *)other;
                from += sizeof(Float);
                other += sizeof(Float);
                *(Float *)(out += sizeof(Float)) =
                    *(Float *)from - *(Float *)other;
                out += sizeof(Float);
                from += sizeof(Float);
                other += sizeof(Float);
                *(Float *)out = *(Float *)from - *(Float *)other;
            }
            difference.y = 0.0F;
            length = njScalor(&difference);
            factor = velocity / length;
            {
                /* @hack spelling: out, a byte cursor over difference, which SHC
                 * forms on the stack (ADD #20,R4 at 0x0c9059d4). without it,
                 * 193 of 836 words differ */
                unsigned char *out = (unsigned char *)&difference;

                /* @hack redundant: an empty never-run branch, which puts the
                 * first product in FR3; without it, 3 of 836 words differ */
                if (0) {
                }
                *(Float *)out *= factor;
                out += sizeof(Float);
                /* @hack redundant: the same, which orders the next two
                 * products' scratch registers; without it, 6 of 836 words
                 * differ */
                if (0) {
                }
                *(Float *)out *= factor;
                out += sizeof(Float);
                *(Float *)out *= factor;
            }
            active_springboard = task;
            player_push(number, difference.x, difference.y, difference.z);
            if (velocity > player->consts.gravity) {
                player_motion[number]->accel.y += player->consts.gravity;
            }
        }
        if (njAbs(deflection) > FEEDBACK_DEFLECTION &&
            !(SPRING(work)->flags & SPRING_FEEDBACK) &&
            ((work->scl.z < 0.0F && SPRING(work)->deflection <= deflection) ||
             (work->scl.z > 0.0F && SPRING(work)->deflection >= deflection))) {
            power = (Sint16)(njAbs(deflection) * 0.25F) + 1;
            if (power > POWER_MOST) {
                power = POWER_MOST;
            }
            sound_play(SE_SPRINGBOARD, 0, 0, 0);
            vibration_start(0, power, (power * 2) + 10, 0);
            SPRING(work)->flags |= SPRING_FEEDBACK;
        }
        if (njAbs(deflection) > THROW_DEFLECTION &&
            !(SPRING(work)->flags & SPRING_BOUNCED) &&
            (SPRING(work)->flags & SPRING_REFLECT) &&
            ((work->scl.z < 0.0F && SPRING(work)->deflection <= deflection) ||
             (work->scl.z > 0.0F && SPRING(work)->deflection >= deflection))) {
            point = player_motion[number]->speed;
            length = njScalor(&point);
            if (length > THROW_SPEED) {
                factor = -3.5F - (length * 1.2F);
                if (factor < -7.0F) {
                    factor = -7.0F;
                }
                if (SPRING(work)->flags & SPRING_REFLECT_45) {
                    /* @hack spelling: aim through a pointer, which forms its
                     * address in R4 (ADD #8,R4 at 0x0c905b84). without it, 17
                     * of 836 words differ */
                    Uint16 *turn = &SPRING(work)->aim;
                    *turn &= AIM_45_MASK;
                    launch_aim = (Sint16)*turn;
                } else {
                    launch_aim = SPRING(work)->aim;
                }
                if (work->tick) {
                    struct task_work **entry;
                    struct task_work **entries;
                    for (entries = springboards, entry = springboards, i = 0;
                         i < CHAIN_BOARDS; ++i, ++entry) {
                        if ((next = *entries++) &&
                            next->tick == work->tick + 1) {
                            {
                                /* @hack spelling: out, from and other, byte
                                 * cursors over point, the listed board's pos
                                 * and the rider's, which keep task in R10 (MOV
                                 * R4,R10 at 0x0c9054e0). without them, 304 of
                                 * 836 words differ */
                                unsigned char *out = (unsigned char *)&point;
                                unsigned char *from =
                                    (unsigned char *)&(*entry)->pos;
                                unsigned char *other =
                                    (unsigned char *)&rider->pos;

                                /* @hack redundant: a never-run test of from,
                                 * which puts from in R4, out in R5 and other in
                                 * R6; without it, 125 of 836 words differ */
                                if (0) {
                                    if (from) {
                                    }
                                }
                                *(Float *)out =
                                    *(Float *)from - *(Float *)other;
                                out += sizeof(Float);
                                from += sizeof(Float);
                                *(Float *)out =
                                    *(Float *)from -
                                    *(Float *)(other += sizeof(Float));
                                out += sizeof(Float);
                                from += sizeof(Float);
                                other += sizeof(Float);
                                *(Float *)out =
                                    *(Float *)from - *(Float *)other;
                            }
                            angle = -(int)(atan2f(point.z, point.x) *
                                           ANGLE_PER_RADIAN) -
                                    work->ang.y + NJM_DEG_ANG(180);
                            {
                                Sint16 delta = angle - launch_aim;
                                if (delta < NEXT_REACH && delta > -NEXT_REACH) {
                                    launch_aim = angle;
                                }
                            }
                            break;
                        }
                    }
                }
                point.x = factor;
                point.y = 0.0F;
                point.z = 0.0F;
                njPushMatrix(&_unit_matrix_);
                TURN_VIA(njRotateY, work->ang.y + launch_aim, angle);
                njCalcVector(0, &point, &point);
                njPopMatrix(1);
                angle = -(int)(atan2f(point.z, point.x) * ANGLE_PER_RADIAN);
                player_set_angles(number, 0, angle + NJM_DEG_ANG(90), 0);
                player_bounce(number, point.x, point.y, point.z);

                player->launch_steps = LAUNCH_STEPS;
                SPRING(work)->flags |= SPRING_BOUNCED;
            }
        }
        velocity = deflection - SPRING(work)->deflection;
    }
    if (++timer <= 0) {
        --timer;
    }
    SPRING(work)->deflection = deflection;
    work->scl.z = velocity;
    SPRING(work)->apex = apex;
    work->submode = number;
    work->timer = timer;
}

/* draw the frame, and the board blended between its rest shape and its bent
 * one by its deflection. the board's points near its middle move along z by
 * the apex */
static void springboard_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &beach_springboard_object;
    NJS_POINT3 points[BOARD_POINTS];
    /* @hack spelling: from, to and out, byte cursors over the rest shape, the
     * bent one and points, which read each Float with a post-increment (FMOV.S
     * @R6+,FR8 at 0x0c905e90). as NJS_POINT3 pointers, 67 of 136 words
     * differ */
    const unsigned char *from;
    const unsigned char *to;
    unsigned char *out;
    NJS_POINT3 *original;
    NJS_POINT3 *mesh;
    NJS_MODEL *model;
    Angle angle;
    Float ratio;
    Float apex;
    NJS_OBJECT *sibling;
    Float x;
    Float y;
    Float z;
    Float from_x;
    Float from_y;
    Float from_z;
    Float to_x;
    Float to_y;
    Float to_z;
    int i;
    if (placement_try_leave(task)) {
        return;
    }
    texture_set_stage();
    njPushMatrix(0);
    njTranslateV(0, &work->pos);

    TURN_VIA(njRotateY, work->ang.y, angle);
    draw_model(object->model);
    object = object->child;
    if (SPRING(work)->deflection == 0.0F && SPRING(work)->apex == 0.0F) {
        njTranslate(0, 4.12F, 8.4F, 0.0F);
        draw_model(object->model);
    } else {
        sibling = object->sibling;
        mesh = points;
        out = (unsigned char *)mesh;
        from = (const unsigned char *)(model = object->model)->points;
        original = model->points;
        to = (const unsigned char *)sibling->model->points;
        /* the model points at the local copy only until draw_model below has
         * drawn it */
        model->points = mesh;
        ratio = -SPRING(work)->deflection * (1.0F / BENT_DEFLECTION);
        apex = SPRING(work)->apex;
        for (i = BOARD_POINTS; i != 0; --i) {
            from_x = *(const Float *)from;
            from += sizeof(Float);
            from_y = *(const Float *)from;
            from += sizeof(Float);
            from_z = *(const Float *)from;
            from += sizeof(Float);
            to_x = *(const Float *)to;
            to += sizeof(Float);
            to_y = *(const Float *)to;
            to += sizeof(Float);
            to_z = *(const Float *)to;
            to += sizeof(Float);
            x = from_x + ((to_x - from_x) * ratio);
            y = from_y + ((to_y - from_y) * ratio);
            z = from_z + ((to_z - from_z) * ratio);
            if (njAbs(z) < APEX_REACH) {
                z += apex;
            }
            *(Float *)out = x;
            out += sizeof(Float);
            *(Float *)out = y;
            out += sizeof(Float);
            *(Float *)out = z;
            out += sizeof(Float);
        }

        /* @hack redundant: three never-run reads of y, which keep the cursors
         * from, to and out in R6, R5 and R4 (0x0c905e90, 0x0c905e92,
         * 0x0c905ebe) and APEX_REACH in FR11 (0x0c905e8a); without them, the
         * cursors take R5, R4 and R6, APEX_REACH takes FR10 and 25 of 136
         * words differ */
        if (0) {
            if (y) {
            }
            if (y) {
            }
            if (y) {
            }
        }
        njTranslate(0, 4.12F, 8.4F, 0.0F);
        draw_model(object->model);
        object->model->points = original;
    }
    njPopMatrix(1);
}

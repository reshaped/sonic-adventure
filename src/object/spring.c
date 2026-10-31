/* springs: a touch launches a player along the spring's y axis, and the
 * spring bobs after. spring b also takes the player's control away for the
 * steps its scl.x gives */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/models.h"
#include "object/placement.h"
#include "object/spring.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/cache.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the coil's stretch at a launch, as its scale less 1 */
#define LAUNCH_STRETCH 0.1F
/* a launch's speed, before ADD_YSPD adds to it */
#define LAUNCH_SPEED 5.0F

/* a player on the ground running ahead at RUN_SPEED or more is set on the
 * spring's top before its launch. a slower one between PULL and PULL_RANGE
 * across from the top is pulled PULL toward it */
#define RUN_SPEED 1.0F
#define PULL 2.0F
#define PULL_RANGE 14.0F

/* a spring takes a touch once work->tick passes TOUCH_WAIT. a launch sets
 * the plain spring's tick to 0, and spring b's to B_LAUNCH_TICK. so the
 * plain spring takes another touch 11 steps after a launch, and spring b 6
 * steps after one */
#define TOUCH_WAIT 10
#define B_LAUNCH_TICK 5
/* work->tick counts up to TICK_LIMIT and stays there */
#define TICK_LIMIT 50

/* a spring's bytes in work->vars. work->submode holds the player it
 * launches, and work->tick counts the steps since its last launch */
struct spring_vars {
    /* the coil's scale less 1: LAUNCH_STRETCH at a launch, then between
     * -0.6 and 0 as it bobs */
    Float stretch;
    Float phase_b; /* spring b's bob phase, in degrees */
    Float bob; /* the bob's size, which shrinks as the phase runs on */
};

#define VARS(work) ((struct spring_vars *)&(work)->vars)

/* the plain spring's bob phase, in degrees. spring b keeps its own in
 * phase_b, since its scl.x is NO_TIMER */
#define PHASE(work) ((work)->scl.x)
/* spring b's steps out of the player's control after a launch, labeled NO
 * TIMER in the set editor */
#define NO_TIMER(work) ((work)->scl.x)
/* the speed a launch adds to LAUNCH_SPEED, labeled ADD YSPD in the set
 * editor */
#define ADD_YSPD(work) ((work)->scl.y)
#define PHASE_SPEED(work) ((work)->scl.z) /* the bob's degrees a step */

/* a spring's modes, in work->mode */
enum spring_mode { SPRING_INIT, SPRING_WAIT, SPRING_LAUNCH, SPRING_KILL };

static void spring_launch(struct task *task, Uint8 along_angles);
static void spring_destroy(struct task *task);
static void spring_draw(struct task *task);
static void spring_b_draw(struct task *task);
static void spring_b_destroy(struct task *task);

/* clang-format off */
static struct collision_shape spring_shape[1] = {
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 6.0F, 10.0F,
     0.0F, {0}, 0, NULL},
};
static struct collision_shape spring_b_shape[1] = {
    {SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 14.0F, 0.0F,
     0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* while the set editor is open, a spring draws but neither bobs nor takes
 * a touch */
void spring_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 i;
    struct task *player;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SPRING_INIT:
        task->destructor = spring_destroy;
        task->draw = spring_draw;
        collision_init(task, spring_shape, COLLISION_OBJECT, 1, 1);
        work->collision->flags |= COLLISION_TARGET;
        PHASE(work) = 180.0F;
        PHASE_SPEED(work) = 0.0F;
        VARS(work)->bob = 0.5F;
        work->mode = SPRING_WAIT;
        break;
    case SPRING_WAIT:
        VARS(work)->stretch = -_builtin_fabsf(
            VARS(work)->bob * njSin(PHASE(work) * ANGLE_PER_DEGREE));
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- ADD YSPD");
        }
        if (edit_mode == 0) {
            PHASE(work) += PHASE_SPEED(work);
            if (PHASE(work) >= 290.0F &&
                PHASE(work) < 290.0F + PHASE_SPEED(work)) {
                PHASE_SPEED(work) = 18.0F;
            }
            VARS(work)->bob =
                njCos((PHASE(work) - 180.0F) * (0.1F * ANGLE_PER_DEGREE)) *
                0.6F;
            if (VARS(work)->bob < 0.0F) {
                PHASE(work) = 180.0F;
                PHASE_SPEED(work) = 0.0F;
            }
            work->tick++;
            if (work->tick > TOUCH_WAIT) {
                if (work->collision->flags & COLLISION_HIT) {
                    work->collision->flags &= ~COLLISION_HIT;
                    for (i = 0; i < 2; i++) {
                        player = player_get_task(i);
                        if (work->collision->hit->owner == player) {
                            VARS(work)->stretch = LAUNCH_STRETCH;
                            work->submode = i;
                            PHASE_SPEED(work) = 20.0F;
                            PHASE(work) = 270.0F;
                            work->mode = SPRING_LAUNCH;
                            break;
                        }
                    }
                }
                if (work->mode != SPRING_LAUNCH) {
                    collision_register(work);
                }
            }
            if (work->tick > TICK_LIMIT) {
                work->tick = TICK_LIMIT;
            }
        }
        spring_draw(task);
        break;
    case SPRING_LAUNCH:
        sound_play(SE_SPRING, 0, 0, 0);
        work->tick = 0;
        spring_launch(task, 0);
        spring_draw(task);
        break;
    case SPRING_KILL:
        placement_kill(task);
        break;
    }
}

/* launch the player that touched the spring from the spring's top. without
 * along_angles, player_bounce sends it along the spring's y axis. with it,
 * player_launch sends it through the spring's angles, out of its control
 * for NO_TIMER steps */
static void spring_launch(struct task *task, Uint8 along_angles)
{
    struct task_work *work = task->work;
    struct task_work *player;
    NJS_POINT3 offset = {0.0F, LAUNCH_SPEED, 0.0F};
    NJS_POINT3 top;
    NJS_POINT3 up;
    Float speed;
    Float distance;
    Float dx;
    Float dy;
    Float dz;
    Uint8 number;

    player = player_work[work->submode];
    offset.y = ADD_YSPD(work) + LAUNCH_SPEED;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njCalcPoint(NULL, &offset, &up);
    njPopMatrix(1);
    top.x = work->pos.x + up.x;
    top.y = work->pos.y + up.y;
    top.z = work->pos.z + up.z;
    if (player == NULL) {
        return;
    }
    number = work->submode;
    dx = top.x - player->pos.x;
    /* @hack redundant: dy, which nothing reads, worked out in FR4 (FSUB
     * and FABS at 0x8c0692de and 0x8c0692ea); without it, 262 of 358 words
     * differ */
    /* @hack spelling: dy's fabs a statement of its own, which keeps dy in FR4;
     * as one statement, 23 of 358 words differ */
    dy = top.y - player->pos.y;
    dy = _builtin_fabsf(dy);
    dz = top.z - player->pos.z;
    distance = njSqrt((dx * dx) + (dz * dz));
    /* @bug uninitialized: player_ground_speed leaves speed unset for a
     * player off the ground, and its FALSE goes unread. the test below then
     * reads speed's frame slot unwritten (FMOV.S @(R0,R15),FR3 at
     * 0x8c069306, R0 4) */
#ifdef GCC
    /* repairs the unset speed the tag names: GCC starts it at 0, a still
     * player's speed, so the test below takes a player off the ground as
     * slow */
    speed = 0.0F;
#endif
    player_ground_speed(number, &speed);
    /* @hack spelling: RUN_SPEED > speed, which loads RUN_SPEED into FR2 and
     * speed into FR3 (FLDI1 FR2 at 0x8c069304); as speed < RUN_SPEED, the
     * two trade and 5 of 358 words differ */
    /* @hack spelling: each arm stores the mode itself, which the image does
     * twice (MOV.B R9,@R14 at 0x8c0694b2 and 0x8c069518); with one store
     * after the arms, SHC holds player_launch's address in R8 and 208 of
     * 358 words differ */
    if (RUN_SPEED > speed || VARS(work)->stretch != LAUNCH_STRETCH) {
        if (distance < PULL_RANGE && distance >= PULL) {
            player_set_mode(number, PLAYER_MODE_STILL);
            player->pos.x += (dx / distance) * PULL;
            player->pos.z += (dz / distance) * PULL;
            player_set_mode(number, PLAYER_MODE_RESET);
            if (!along_angles) {
                player_bounce(number, up.x, up.y, up.z);
            } else {
                /* @hack spelling: angles declared in each arm, which forms
                 * the array's address there (MOV R15,R10 and ADD #8,R10 at
                 * 0x8c069376); declared once, 254 of 358 words differ */
                Angle angles[3];

                angles[0] = work->ang.x;
                angles[1] = work->ang.y;
                angles[2] = work->ang.z;
                player->pos.x = top.x;
                player->pos.y = work->pos.y + (up.y * 0.8F);
                player->pos.z = top.z;
                if (NO_TIMER(work) <= 0.0F) {
                    NO_TIMER(work) = 0.0F;
                }
                player_launch(number, angles, ADD_YSPD(work) + LAUNCH_SPEED,
                              NO_TIMER(work));
            }
            work->mode = SPRING_WAIT;
        } else {
            if (!along_angles) {
                player_bounce(number, up.x, up.y, up.z);
            } else {
                Angle angles[3];

                angles[0] = work->ang.x;
                angles[1] = work->ang.y;
                angles[2] = work->ang.z;
                player->pos.x = top.x;
                player->pos.y = work->pos.y + (up.y * 0.8F);
                player->pos.z = top.z;
                if (NO_TIMER(work) <= 0.0F) {
                    NO_TIMER(work) = 0.0F;
                }
                player_launch(number, angles, ADD_YSPD(work) + LAUNCH_SPEED,
                              NO_TIMER(work));
            }
            work->mode = SPRING_WAIT;
        }
    } else {
        player_set_mode(number, PLAYER_MODE_STILL);
        player->pos = top;
        player_set_mode(number, PLAYER_MODE_RESET);
        if (!along_angles) {
            player_bounce(number, up.x, up.y, up.z);
        } else {
            Angle angles[3];

            angles[0] = work->ang.x;
            angles[1] = work->ang.y;
            angles[2] = work->ang.z;
            if (NO_TIMER(work) <= 0.0F) {
                NO_TIMER(work) = 0.0F;
            }
            player_launch(number, angles, ADD_YSPD(work) + LAUNCH_SPEED,
                          NO_TIMER(work));
        }
        work->mode = SPRING_WAIT;
    }
    VARS(work)->stretch = LAUNCH_STRETCH;
    vibration_start(number, 4, 15, 0);
}

static void spring_destroy(struct task *task)
{
}

/* draw the base, the coil at its stretch, and the top on the coil */
static void spring_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_MODEL *model;

    texture_set_regular();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    /* @hack spelling: the base's model through a local, which loads
     * draw_model's address ahead of the model's (0x8c0695ac, 0x8c0695ae);
     * passed in place, 2 of 113 words differ */
    model = &spring_base_model;
    draw_model(model);
    njPushMatrix(NULL);
    object = &spring_coil_object;
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    njScale(NULL, 1.0F, VARS(work)->stretch + 1.0F, 1.0F);
    draw_model(object->model);
    matrix_load_parent();
    object = &spring_top_object;
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    njTranslate(NULL, 0.0F, VARS(work)->stretch * 4.0F, 0.0F);
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

void spring_b_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 i;
    struct task *player;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SPRING_INIT:
        task->destructor = spring_b_destroy;
        task->draw = spring_b_draw;
        collision_init(task, spring_b_shape, COLLISION_OBJECT, 1, 1);
        work->collision->flags |= COLLISION_TARGET;
        VARS(work)->phase_b = 180.0F;
        PHASE_SPEED(work) = 0.0F;
        VARS(work)->bob = 0.5F;
        work->tick = TOUCH_WAIT;
        work->mode = SPRING_WAIT;
        break;
    case SPRING_WAIT:
        VARS(work)->stretch = -_builtin_fabsf(
            VARS(work)->bob * njSin(VARS(work)->phase_b * ANGLE_PER_DEGREE));
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- NO TIMER");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- ADD YSPD");
        }
        if (edit_mode == 0) {
            VARS(work)->phase_b += PHASE_SPEED(work);
            if (VARS(work)->phase_b >= 290.0F &&
                VARS(work)->phase_b < 290.0F + PHASE_SPEED(work)) {
                PHASE_SPEED(work) = 15.0F;
            }
            VARS(work)->bob = njCos((VARS(work)->phase_b - 180.0F) *
                                    (0.1F * ANGLE_PER_DEGREE)) *
                              0.6F;
            if (VARS(work)->bob < 0.0F) {
                VARS(work)->phase_b = 180.0F;
                PHASE_SPEED(work) = 0.0F;
            }
            work->tick++;
            if (work->tick > TOUCH_WAIT) {
                if (work->collision->flags & COLLISION_HIT) {
                    work->collision->flags &= ~COLLISION_HIT;
                    for (i = 0; i < 2; i++) {
                        player = player_get_task(i);
                        if (work->collision->hit->owner == player) {
                            VARS(work)->stretch = LAUNCH_STRETCH;
                            work->submode = i;
                            PHASE_SPEED(work) = 20.0F;
                            VARS(work)->phase_b = 270.0F;
                            work->mode = SPRING_LAUNCH;
                            break;
                        }
                    }
                }
                if (work->mode != SPRING_LAUNCH) {
                    collision_register(work);
                }
            }
            if (work->tick > TICK_LIMIT) {
                work->tick = TICK_LIMIT;
            }
        }
        spring_b_draw(task);
        break;
    case SPRING_LAUNCH:
        work->tick = B_LAUNCH_TICK;
        spring_launch(task, 1);
        spring_b_draw(task);
        sound_play(SE_SPRING, 0, 0, 0);
        break;
    case SPRING_KILL:
        placement_kill(task);
        break;
    }
}

/* draw the base, the top on the coil, and the coil at its stretch */
static void spring_b_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *coil;
    NJS_OBJECT *object;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a point. the image's ADD #-16,R15 reserves them in its
     * 44-byte frame (0x8c06996e); without them, the frame is 32 bytes and 2 of
     * 117 words differ */
    NJS_POINT3 unused;
    /* @hack spelling: draw_model called through draw, which keeps its address
     * in R12 and njPushMatrix's in R11 (0x8c0699c0, 0x8c069976); called by
     * name, the two trade and 11 of 117 words differ */
    void (*draw)(NJS_MODEL *model);

    texture_set_regular();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw = draw_model;
    draw(&spring_b_base_model);
    njPushMatrix(NULL);
    coil = &spring_b_coil_object;
    njTranslate(NULL, coil->pos[0], coil->pos[1], coil->pos[2]);
    njPushMatrix(NULL);
    object = &spring_b_top_object;
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    njTranslate(NULL, 0.0F, VARS(work)->stretch * 4.0F, 0.0F);
    draw(object->model);
    njPopMatrix(1);
    /* @hack redundant: coil set again, which loads its address from the pool
     * again and keeps it at @R15 (MOV.L R3,@R15 at 0x8c069a1a); held from
     * above, coil stays in R10, R9 is saved too, and 86 of 117 words
     * differ */
    coil = &spring_b_coil_object;
    njScale(NULL, 1.0F, VARS(work)->stretch + 1.0F, 1.0F);
    draw(coil->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void spring_b_destroy(struct task *task)
{
}

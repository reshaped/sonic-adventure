/* O Cone1 and O Cone2 traffic cones that a running player kicks away */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg04/cone.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL cone_1_model;
extern NJS_MODEL cone_2_model;

/* in work->mode, which cone it is */
enum cone_kind { CONE_1, CONE_2 };

/* a flying cone's speed along its heading and its speed up, a step. the cone
 * lies still while both are 0 */
#define SPEED_ACROSS(work) ((work)->scl.x)
#define SPEED_UP(work) ((work)->scl.y)

struct cone_vars {
    Angle heading; /* the way the kick sends it, from player 0 */
};

#define VARS(work) ((struct cone_vars *)&(work)->vars)

/* in the air, the cone's speed up loses GRAVITY a step. on the ground, its
 * speed along its heading loses FRICTION a step. a landing faster than
 * BOUNCE_LEAST bounces it back up at BOUNCE_SHARE of that speed */
#define GRAVITY 0.1F
#define FRICTION 0.5F
#define BOUNCE_LEAST 1.0F
#define BOUNCE_SHARE 0.5F

/* a kick's speed along the ground is KICK_ACROSS times player 0's ground
 * speed. its speed up is KICK_UP times it */
#define KICK_ACROSS 1.5F
#define KICK_UP (2.0F / 3.0F)

#define QUARTER_TURN NJM_DEG_ANG(90)

#define SE_KICK 118

static void cone_destroy(struct task *task);
static void cone_draw(struct task *task);
static void cone_run(struct task *task);
static void cone_start(struct task *task);

static struct collision_shape cone_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 2.0F, 0.0F},
    3.0F,
    1.5F,
    0.0F,
    {0},
    0,
    NULL,
};

static void cone_destroy(struct task *task)
{
}

static void cone_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (work->mode == CONE_1) {
        draw_model(&cone_1_model);
    } else {
        draw_model(&cone_2_model);
    }
    njPopMatrix(1);
}

/* a hit sends a still cone flying from player 0. a flying cone that leaves
 * range kills its placement */
static void cone_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos = &work->pos;
    NJS_POINT3 *player = &player_work[0]->pos;
    Angle heading;
    Float ground;
    Float speed;

    if (placement_try_leave(task)) {
        if (SPEED_UP(work) != 0.0F || SPEED_ACROSS(work) != 0.0F) {
            placement_kill(task);
        }
        return;
    }
    if (SPEED_UP(work) != 0.0F || SPEED_ACROSS(work) != 0.0F) {
        heading = VARS(work)->heading;
        /* @bug memory: ground_below writes the slope's x and z through
         * &speed, a lone Float. the z lands 8 bytes past it, on the FR15 the
         * frame saved, and the return hands the caller that z in FR15 (JSR at
         * 0x0c9006d8, FMOV.S @R15+,FR15 at 0x0c900864) */
        ground = ground_below(pos->x, pos->y, pos->z, (struct angles *)&speed);
        pos->x -= njSin(heading) * SPEED_ACROSS(work);
        pos->y += SPEED_UP(work);
        pos->z += njCos(heading) * SPEED_ACROSS(work);
        if (pos->y > ground || ground == -GROUND_FAR) {
            SPEED_UP(work) -= GRAVITY;
        } else {
            SPEED_ACROSS(work) -= FRICTION;
            if (SPEED_ACROSS(work) < 0.0F) {
                SPEED_ACROSS(work) = 0.0F;
            }
            if (SPEED_UP(work) < -BOUNCE_LEAST) {
                SPEED_UP(work) = -SPEED_UP(work) * BOUNCE_SHARE;
            } else {
                SPEED_UP(work) = 0.0F;
            }
            pos->y = ground;
        }
    } else if (work->collision->flags & COLLISION_HIT) {
        /* @bug uninitialized: player_ground_speed leaves speed unset for
         * player 0 off the ground, and its FALSE goes unread. the kick then
         * reads speed's frame slot unwritten (FMOV.S @R15,FR2 at
         * 0x0c9007fc) */
        player_ground_speed(0, &speed);
        SPEED_ACROSS(work) = speed * KICK_ACROSS;
        SPEED_UP(work) = speed * KICK_UP;
        VARS(work)->heading =
            ((Angle)(atan2f(pos->z - player->z, pos->x - player->x) *
                     ANGLE_PER_RADIAN) -
             QUARTER_TURN) &
            0xffff;
        sound_play(SE_KICK, 0, 0, 0);
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    cone_draw(task);
}

static void cone_start(struct task *task)
{
    struct task_work *work = task->work;

    SPEED_UP(work) = SPEED_ACROSS(work) = 0.0F;
    collision_init(task, &cone_shape, COLLISION_OBJECT, 1, 1);
    task->destructor = cone_destroy;
    task->step = cone_run;
    task->draw = cone_draw;
}

void cone_1_step(struct task *task)
{
    task->work->mode = CONE_1;
    cone_start(task);
}

void cone_2_step(struct task *task)
{
    task->work->mode = CONE_2;
    cone_start(task);
}

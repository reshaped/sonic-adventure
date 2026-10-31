/* O Bat, a bat that hangs in Red Mountain's caves. when player 0 comes near,
 * it flies about, then goes back to hang where it started */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/helpers.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg05/bat.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_ACTION bat_action;
#ifdef GCC
/* the wings' UVs, as bat_draw takes them: the 3 their strips share, then
 * the 4th the image's draw reads past them */
static NJS_COLOR bat_wing_uvs[4] = {
    UV(0, 0),
    UV(255, 195),
    UV(0, 255),
    UV(-16384, 1),
};
#endif

/* a bat's record, which bat_alloc puts on the heap for BAT(task) and
 * bat_destroy frees */
struct bat {
    int flags;
    int timer; /* the steps the bat has run */
    Float frame; /* the frame of bat_action it draws */
    /* how many frames the action advances each step, negative to play it
     * backward */
    Float frame_rate;
    /* the distance the bat moves along ang each step. it only grows, up to
     * BAT_TOP_SPEED */
    Float speed;
    /* the heading the bat moves along at speed. its x and z turns drift at
     * random, and its y turn stays 0. the bat faces its target, not ang */
    struct angles ang;
    /* where the last step left the bat, which nothing reads */
    NJS_POINT3 last_pos;
    NJS_POINT3 home; /* where the bat hangs */
    struct angles home_ang;
    NJS_POINT3 waypoint; /* the point the bat flies to on its way home */
    int target; /* what a chasing bat flies at, by enum bat_target */
};

#define BAT(task) (*(struct bat **)(task)->extra)

struct bat_vars {
    NJS_ACTION *action; /* bat_action, which bat_draw stores before it draws */
};

#define VARS(work) ((struct bat_vars *)&(work)->vars)

/* in bat->flags: the bat has reached its waypoint and flies on home */
#define BAT_WAYPOINT_REACHED 0x1

enum bat_mode { BAT_INIT, BAT_HANG, BAT_CHASE, BAT_RETURN, BAT_GONE };

/* what a chasing bat flies at. BAT_HALFWAY is the point halfway between
 * player 0 and home. BAT_HOME sends the bat home instead, and BAT_TARGETS
 * counts the targets */
enum bat_target { BAT_HALFWAY, BAT_PLAYER, BAT_CAMERA, BAT_HOME, BAT_TARGETS };

/* the steps from one of a bat's checks to the next. a hanging bat checks for
 * player 0, and a chasing bat its target and its distance from home */
#define BAT_CHECK_STEPS 10
/* player 0 within this distance starts a chase */
#define BAT_SPOT_RANGE 50.0F
/* a chasing bat this far from home or farther heads home */
#define BAT_HOME_RANGE 100.0F
/* the bat has reached a point within this distance of it */
#define BAT_ARRIVAL_RANGE 0.2F
#define BAT_TOP_SPEED 0.3F
/* how far the bat moves toward its target each step, before it moves along
 * its heading */
#define BAT_APPROACH 0.5F
/* the most bat->ang's x and z turns drift in a step, either way: 30 degrees */
#define BAT_DRIFT 5461.0F
/* a hanging bat swings its wings between frames BAT_HANG_FROM and BAT_HANG_TO
 * of bat_action, BAT_HANG_RATE frames a step. a flying bat beats them between
 * frames 0 and BAT_FLY_TO, BAT_FLY_RATE frames a step */
#define BAT_HANG_FROM 25.0F
#define BAT_HANG_TO 30.0F
#define BAT_HANG_RATE 0.8F
#define BAT_FLY_TO 15.0F
#define BAT_FLY_RATE 3.2F
/* a hanging bat's chance each step of flying off to a waypoint and back */
#define BAT_WANDER_CHANCE 0.002F
/* the waypoint lies within a box this wide across x and z, centered on
 * home. it lies up to this far below home */
#define BAT_WAYPOINT_SPREAD 10.0F

/* the record of the bat bat_step is running, or ran last. bat_draw takes the
 * frame from here, not from its own task. so a bat that task_draw_all draws
 * outside its step shows the frame of the bat stepped last */
static struct bat *bat;

static void bat_destroy(struct task *task);
static void bat_draw(struct task *task);
static void bat_flap(struct task *task, struct task_work *work);
static struct bat *bat_alloc(void);
static void bat_save_pos(struct task *task, struct task_work *work);
static void bat_watch(struct task *task, struct task_work *work);
static void bat_chase(struct task *task, struct task_work *work);
static void bat_wander(struct task *task, struct task_work *work);
static void bat_head_home(struct task *task, struct task_work *work);
static void bat_return(struct task *task, struct task_work *work);
static void bat_fly(struct task *task, struct task_work *work,
                    NJS_POINT3 *target);
static Bool bat_near(struct task *task, struct task_work *work,
                     NJS_POINT3 *target, Float distance);

/* the bat hangs until player 0 comes within BAT_SPOT_RANGE. then it chases
 * a target, and flies home to hang again. in the set editor, it only draws
 * itself, turned as it hangs, at frame 0 of bat_action */
void bat_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    bat = BAT(task);
    if (placement_editing(task)) {
        if (work->mode == BAT_INIT) {
            work->mode++;
            BAT(task) = bat_alloc();
            task->destructor = bat_destroy;
            task->draw = bat_draw;
        }
        bat = BAT(task);
        work->ang.x = -0x4000;
        bat_draw(task);
    } else {
        switch (work->mode) {
        case BAT_INIT:
            work->mode = BAT_HANG;
            BAT(task) = bat_alloc();
            bat = BAT(task);
            task->destructor = bat_destroy;
            task->draw = bat_draw;
            work->ang.x = -0x4000;
            VECTOR_COPY(&bat->last_pos, &work->pos);
            VECTOR_COPY(&bat->home, &work->pos);
            bat->home_ang.x = work->ang.x;
            bat->home_ang.y = work->ang.y;
            bat->home_ang.z = work->ang.z;
            break;
        case BAT_HANG:
            bat_draw(task);
            bat_watch(task, work);
            bat_wander(task, work);
            bat_flap(task, work);
            break;
        case BAT_CHASE:
            bat_draw(task);
            bat_chase(task, work);
            bat_flap(task, work);
            break;
        case BAT_RETURN:
            bat_draw(task);
            bat_return(task, work);
            bat_flap(task, work);
            break;
        case BAT_GONE:
        default:
            placement_kill(task);
            break;
        }
    }
    bat->timer++;
    bat_save_pos(task, work);
}

static void bat_destroy(struct task *task)
{
    if (BAT(task) != NULL) {
        heap_free(BAT(task));
        BAT(task) = NULL;
    }
}

static void bat_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 5.0F)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x - 0x4000, ang);
        TURN_VIA(njRotateY, work->ang.y + 0x8000, ang);
        VARS(work)->action = &bat_action;
#ifdef GCC
        /* keeps the image's UVs with no read past. each wing is a strip of 4
         * points, whose meshset takes the 3 UVs at 0x0ca95538 in STG05.BIN.
         * the image's draw reads a 4th past them, the first word of the
         * meshset at 0x0ca95544, as u -16384 and v 1. GCC's wings take
         * bat_wing_uvs, which holds all 4 */
        bat_action.object->child->child->model->meshsets->vertuv = bat_wing_uvs;
        bat_action.object->child->sibling->child->model->meshsets->vertuv =
            bat_wing_uvs;
#endif
        draw_action(VARS(work)->action, bat->frame);
        njPopMatrix(1);
    }
}

/* advance the action, swinging a hanging bat's wings and beating a flying
 * bat's */
static void bat_flap(struct task *task, struct task_work *work)
{
    bat->frame += bat->frame_rate;
    switch (work->mode) {
    case BAT_HANG:
        if (bat->frame < BAT_HANG_FROM) {
            bat->frame = BAT_HANG_FROM;
            bat->frame_rate = BAT_HANG_RATE;
        } else if (bat->frame >= BAT_HANG_TO) {
            bat->frame = BAT_HANG_TO - 0.1F;
            bat->frame_rate = -BAT_HANG_RATE;
        }
        break;
    case BAT_CHASE:
    case BAT_RETURN:
        if (bat->frame > BAT_FLY_TO) {
            if (bat->frame_rate > 0.0F) {
                bat->frame_rate = -BAT_FLY_RATE;
                bat->frame = BAT_FLY_TO;
            }
        } else if (bat->frame < 0.0F) {
            bat->frame = 0.0F;
            bat->frame_rate = BAT_FLY_RATE;
        }
        break;
    }
}

static struct bat *bat_alloc(void)
{
    struct bat *record;
    int i;

    /* @hack redundant: an empty test of the record, which writes TST R0,R0 at
     * 0x0c90431c with no branch after it; without it, 19 of 25 words differ */
    if (!(record = heap_alloc(sizeof(struct bat)))) {
    }
    /* NOLINTNEXTLINE(clang-diagnostic-sign-compare) */
    for (i = 0; i < sizeof(struct bat); i++) {
        ((char *)record)[i] = 0;
    }
    return record;
}

static void bat_save_pos(struct task *task, struct task_work *work)
{
    VECTOR_COPY(&bat->last_pos, &work->pos);
}

/* every BAT_CHECK_STEPS steps, chase a random target if player 0 is within
 * BAT_SPOT_RANGE */
static void bat_watch(struct task *task, struct task_work *work)
{
    if (bat->timer % BAT_CHECK_STEPS == 0) {
        if (bat_near(task, work, &player_work[0]->pos, BAT_SPOT_RANGE)) {
            work->mode = BAT_CHASE;
            bat->frame_rate = -BAT_HANG_RATE;
            bat->target = rand() * (BAT_TARGETS * RAND_SCALE);
        }
    }
}

/* fly at the target. every BAT_CHECK_STEPS steps, pick another target unless
 * the bat has reached this one. at the same steps, head home if the bat is
 * BAT_HOME_RANGE or more from home */
static void bat_chase(struct task *task, struct task_work *work)
{
    NJS_POINT3 target;

    switch (bat->target) {
    case BAT_HALFWAY:
        target.x = (player_work[0]->pos.x + bat->home.x) * 0.5F;
        target.y = (player_work[0]->pos.y + bat->home.y) * 0.5F;
        target.z = (player_work[0]->pos.z + bat->home.z) * 0.5F;
        bat_fly(task, work, &target);
        if (bat->timer % BAT_CHECK_STEPS == 0) {
            if (bat_near(task, work, &target, BAT_ARRIVAL_RANGE) == FALSE) {
                bat->target = rand() * (BAT_TARGETS * RAND_SCALE);
            }
        }
        break;
    case BAT_PLAYER:
        bat_fly(task, work, &player_work[0]->pos);
        if (bat->timer % BAT_CHECK_STEPS == 0) {
            if (bat_near(task, work, &player_work[0]->pos, BAT_ARRIVAL_RANGE) ==
                FALSE) {
                bat->target = rand() * (BAT_TARGETS * RAND_SCALE);
            }
        }
        break;
    case BAT_CAMERA:
        bat_fly(task, work, &camera_work->pos);
        if (bat->timer % BAT_CHECK_STEPS == 0) {
            if (bat_near(task, work, &camera_work->pos, BAT_ARRIVAL_RANGE) ==
                FALSE) {
                bat->target = rand() * (BAT_TARGETS * RAND_SCALE);
            }
        }
        break;
    case BAT_HOME:
    default:
        bat_head_home(task, work);
        break;
    }
    if (bat->timer % BAT_CHECK_STEPS == 0) {
        if (bat_near(task, work, &bat->home, BAT_HOME_RANGE) == FALSE) {
            bat_head_home(task, work);
        }
    }
}

/* with a chance of BAT_WANDER_CHANCE, start the hanging bat for home by way
 * of a waypoint */
static void bat_wander(struct task *task, struct task_work *work)
{
    if (rand() * RAND_SCALE < BAT_WANDER_CHANCE) {
        bat_head_home(task, work);
    }
}

/* start for home by way of a random waypoint, within BAT_WAYPOINT_SPREAD */
static void bat_head_home(struct task *task, struct task_work *work)
{
    work->mode = BAT_RETURN;
    bat->waypoint.x = bat->home.x +
                      (rand() * (BAT_WAYPOINT_SPREAD * RAND_SCALE)) -
                      (BAT_WAYPOINT_SPREAD / 2);
    bat->waypoint.y =
        bat->home.y - (rand() * (BAT_WAYPOINT_SPREAD * RAND_SCALE));
    bat->waypoint.z = bat->home.z +
                      (rand() * (BAT_WAYPOINT_SPREAD * RAND_SCALE)) -
                      (BAT_WAYPOINT_SPREAD / 2);
    bat->flags &= ~BAT_WAYPOINT_REACHED;
}

/* fly to the waypoint, then home, and hang again there at the home angles */
static void bat_return(struct task *task, struct task_work *work)
{
    if (bat->flags & BAT_WAYPOINT_REACHED) {
        bat_fly(task, work, &bat->home);
        if (bat_near(task, work, &bat->home, BAT_ARRIVAL_RANGE)) {
            work->mode = BAT_HANG;
            VECTOR_COPY(&work->pos, &bat->home);
            work->ang.x = bat->home_ang.x;
            work->ang.y = bat->home_ang.y;
            work->ang.z = bat->home_ang.z;
        }
    } else {
        bat_fly(task, work, &bat->waypoint);
        if (bat_near(task, work, &bat->waypoint, BAT_ARRIVAL_RANGE)) {
            bat->flags |= BAT_WAYPOINT_REACHED;
        }
    }
}

/* move the bat BAT_APPROACH toward target, then by its speed along its
 * heading. the speed grows by up to a fifth of BAT_TOP_SPEED, and the
 * heading drifts by up to BAT_DRIFT. then the bat turns to face target
 * across x and z */
static void bat_fly(struct task *task, struct task_work *work,
                    NJS_POINT3 *target)
{
    NJS_VECTOR move;

    /* one product of the constants, 0x35f5c28d, as SHC folds them */
    bat->speed += rand() * (RAND_SCALE * BAT_TOP_SPEED * 0.2F);
    if (bat->speed < -BAT_TOP_SPEED) {
        bat->speed = -BAT_TOP_SPEED;
    } else if (bat->speed > BAT_TOP_SPEED) {
        bat->speed = BAT_TOP_SPEED;
    }
    bat->ang.x += (Angle)RANDOM(-BAT_DRIFT, BAT_DRIFT);
    bat->ang.z += (Angle)RANDOM(-BAT_DRIFT, BAT_DRIFT);
    {
        NJS_MATRIX matrix;
        NJS_VECTOR ahead;

        ahead.x = 0.0F;
        ahead.y = 0.0F;
        ahead.z = bat->speed;
        njUnitMatrix(&matrix);
        njRotateXYZ(&matrix, bat->ang.x, bat->ang.y, bat->ang.z);
        njCalcVector(&matrix, &ahead, &move);
        point_toward(&work->pos, target, BAT_APPROACH, &work->pos);
        work->pos.x += move.x;
        work->pos.y += move.y;
        work->pos.z += move.z;
    }
    /* @hack redundant: the bare scope around away, which lets away take the
     * stack slot that matrix held in the scope above. declared with the
     * function's locals, away makes the frame 100 bytes, not 88 (ADD #-88,R15
     * at 0x0c90476c), and 7 of 167 words differ */
    {
        NJS_VECTOR away;

        away.x = target->x - work->pos.x;
        away.z = target->z - work->pos.z;
        work->ang.x =
            (Angle)(atan2f(away.z, away.x) * ANGLE_PER_RADIAN) + 0x4000;
        work->ang.y = -0x4000;
        work->ang.z = -0x4000;
    }
}

static Bool bat_near(struct task *task, struct task_work *work,
                     NJS_POINT3 *target, Float distance)
{
    NJS_VECTOR away;

    away.x = target->x - work->pos.x;
    away.y = target->y - work->pos.y;
    away.z = target->z - work->pos.z;
    if (njScalor(&away) < distance) {
        return TRUE;
    }
    return FALSE;
}

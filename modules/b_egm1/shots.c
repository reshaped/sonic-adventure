/* the Egg Hornet's shots: volleys of missiles that curve toward a target.
 * the launchers at the boss's two sides fire by turns. each missile sets out
 * its own way and curves round to the target. it blows up where its curve
 * ends */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm1/hornet_textures.h"
#include "b_egm1/rumble.h"
#include "b_egm1/shots.h"
#include "b_egm1/texlists.h"
#include "b_egm1/util.h"
#include "collision/collision.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

struct missile {
    struct curve path;
    NJS_POINT3 *target;
    Float time; /* along path, from 0 to 1 */
    Float time_step; /* what each step adds to time */
    Uint16 trail_timer; /* the steps since the last puff of its trail */
    Uint16 follow_steps; /* the steps left in which path's end follows target */
};

struct volley_record {
    struct volley volley;
    Uint16 timer; /* the steps since the volley began */
    Uint16 fired; /* the shots fired so far */
};

#define SE_SHOT 455
#define SE_SHOT_BLAST 456

/* the most a missile is drawn tipped up or down */
#define SHOT_PITCH NJM_DEG_ANG(25)

/* each launcher's offset from the boss, before the boss's turn. the
 * launcher on side 1 is at +LAUNCHER_X, and on side 0 at -LAUNCHER_X */
#define LAUNCHER_X 28.36F
#define LAUNCHER_Y 3.9F
#define LAUNCHER_Z 3.0F

/* a missile leaves a puff of dust every TRAIL_EVERY steps. the puff, at a
 * scale of TRAIL_SCALE, drifts back along the missile's last move */
#define TRAIL_EVERY 2
#define TRAIL_SCALE 0.5F

/* each shot's turn of the line from its launcher to the target, by y and
 * then x. half the turned line ends at the curve's control point, so the
 * shot sets out that way */
static const struct angles egg_hornet_volley_angles[MOST_SHOTS] = {
    {NJM_DEG_ANG(15), NJM_DEG_ANG(-30), 0},
    {NJM_DEG_ANG(45), NJM_DEG_ANG(25), 0},
    {NJM_DEG_ANG(35), NJM_DEG_ANG(-35), 0},
    {NJM_DEG_ANG(30), NJM_DEG_ANG(30), 0},
    {NJM_DEG_ANG(25), NJM_DEG_ANG(-20), 0},
    {NJM_DEG_ANG(10), NJM_DEG_ANG(35), 0},
};

static Float egg_hornet_missile_speed = 1.5F;
static Sint32 egg_hornet_missile_volume = 20;
/* clang-format off */
static struct collision_shape egg_hornet_missile_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE,
    ATTACK(2) | GUARD(1) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
        SHAPE_HURTS_OBJECTS,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F}, 3.0F, 8.0F, 0.65F, {0x2000, 0x4000, 0x6000}, 0, NULL};
/* clang-format on */

static void egg_hornet_missile_draw(struct task *task);

/* *pos gets the launcher on side 0 or side 1. its offset from the boss is
 * turned by work->ang.y and moved to work->pos */
static void egg_hornet_missile_launcher(struct task_work *work, NJS_POINT3 *pos,
                                        int side)
{
    NJS_MATRIX matrix;
    NJS_VECTOR offset;

    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, work->ang.y);
    offset.x = side ? LAUNCHER_X : -LAUNCHER_X;
    offset.y = LAUNCHER_Y;
    offset.z = LAUNCHER_Z;
    njCalcVector(&matrix, &offset, pos);
    pos->x += work->pos.x;
    pos->y += work->pos.y;
    pos->z += work->pos.z;
}

static void egg_hornet_missile_step(struct task *task)
{
    struct missile *missile = task->extra;
    struct task_work *work = task->work;
    NJS_VECTOR moved;

    missile->time += missile->time_step;
    if (missile->time >= 1.0F) {
        sound_play_at(SE_SHOT_BLAST, 0, 0, egg_hornet_missile_volume,
                      work->pos.x, work->pos.y, work->pos.z);
        bomb_create(&work->pos, 1.0F);
        rumble_weak();
        task_kill(task);
    } else {
        if (missile->follow_steps > 0) {
            --missile->follow_steps;
            missile->path.end.x = missile->target->x;
            missile->path.end.y = missile->target->y;
            missile->path.end.z = missile->target->z;
        }
        VECTOR_COPY(&moved, &work->pos);
        curve_point(&work->pos, &missile->path, missile->time);
        VECTOR_DIFF(&moved, &work->pos, &moved);
        work->ang.y = vector_yaw(&moved);
        work->ang.x = vector_pitch(&moved);
        if (work->ang.x < -SHOT_PITCH) {
            work->ang.x = -SHOT_PITCH;
        }
        if (work->ang.x > SHOT_PITCH) {
            work->ang.x = SHOT_PITCH;
        }
        if (++missile->trail_timer >= TRAIL_EVERY) {
            moved.x = -moved.x;
            moved.y = -moved.y;
            moved.z = -moved.z;
            dust_create(&work->pos, &moved, TRAIL_SCALE);
            missile->trail_timer = 0;
        }
        collision_register(work);
        egg_hornet_missile_draw(task);
    }
}

static void egg_hornet_missile_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&egm1mis_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, work->ang.x);
    draw_tree(&egg_hornet_missile_object);
    njPopMatrix(1);
}

/* fire a missile from pos along a curve to *target. the curve's control
 * point is pos plus half the line to the target, turned by ang. the curve's
 * end follows *target for the missile's first follow_steps steps */
static void egg_hornet_missile_create(NJS_POINT3 *pos, NJS_POINT3 *target,
                                      const struct angles *ang,
                                      Uint16 follow_steps)
{
    struct task *task;
    struct task_work *work;
    struct missile *missile;
    NJS_MATRIX matrix;
    NJS_VECTOR half;
    NJS_VECTOR out;
    Float length;

    if ((task = task_create(TASK_WORK, 3, egg_hornet_missile_step)) == NULL) {
        return;
    }
    work = task->work;
    collision_init(task, &egg_hornet_missile_shape, COLLISION_ITEM, 1, 1);
    work->collision->flags &= ~COLLISION_TARGET;
    task->extra = missile = heap_calloc(1, sizeof(struct missile));
    missile->target = target;
    missile->path.start.x = pos->x;
    missile->path.start.y = pos->y;
    missile->path.start.z = pos->z;
    missile->path.end.x = missile->target->x;
    missile->path.end.y = missile->target->y;
    missile->path.end.z = missile->target->z;
    missile->time = 0.0F;
    half.x = (target->x - pos->x) * 0.5F;
    half.y = (target->y - pos->y) * 0.5F;
    half.z = (target->z - pos->z) * 0.5F;
    if ((length = njScalor(&half)) > 0.0F) {
        missile->time_step = egg_hornet_missile_speed / length;
    } else {
        missile->time_step = 1.0F;
    }
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, ang->y);
    TURN_MATRIX(njRotateX, &matrix, ang->x);
    njCalcVector(&matrix, &half, &out);
    VECTOR_SUM(&missile->path.control, &out, pos);
    missile->follow_steps = follow_steps;
    work->pos.x = pos->x;
    work->pos.y = pos->y;
    work->pos.z = pos->z;
    work->ang.y = vector_yaw(&half);
    work->ang.x = work->ang.z = 0;
    task->step = egg_hornet_missile_step;
    task->draw = egg_hornet_missile_draw;
}

/* fire shot n once the volley has run more than n * interval steps. side
 * n & 1 fires it, and the volley ends after its last shot */
static void egg_hornet_volley_step(struct task *task)
{
    struct volley_record *record = task->extra;
    Uint16 delay = record->fired * record->volley.interval;
    NJS_POINT3 pos;
    struct task_work *owner;

    if (++record->timer > delay) {
        owner = record->volley.owner;
        egg_hornet_missile_launcher(owner, &pos, record->fired & 1);
        egg_hornet_missile_create(
            &pos, record->volley.target,
            &egg_hornet_volley_angles[record->fired],
            (record->fired * record->volley.follow_interval) - delay);
        sound_play(SE_SHOT, 0, 0, 0);
        if (++record->fired >= record->volley.count) {
            task_kill(task);
        }
    }
}

void egg_hornet_volley_create(struct volley *volley)
{
    struct task *task;
    struct volley_record *record;

    if ((task = task_create(0, 3, egg_hornet_volley_step)) != NULL) {
        task->extra = record = heap_calloc(1, sizeof(struct volley_record));
        record->volley = *volley;
        record->timer = 0;
        record->fired = 0;
        task->step = egg_hornet_volley_step;
    }
}

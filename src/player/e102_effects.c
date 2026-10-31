/* E-102's booster, sparks and dust, and helpers his step and other files
 * call */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/mesh.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "effect/dust.h"
#include "player/e102.h"
#include "player/e102_effects.h"
#include "player/player.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* how far his booster bobs above and below its place */
#define BOOSTER_BOB 1.0F

#define SPARK_STEPS 60 /* the steps a spark flies */

/* the puffs in his landing's ring, and their speed for each unit of the
 * speed he fell at */
#define LANDING_PUFFS 12
#define LANDING_SPEED 1.0F

enum spark_mode { SPARK_START, SPARK_FLY };

struct spark_vars {
    Uint8 unk_00[4];
    Sint32 steps; /* steps left to fly */
};

#define SPARK(work) ((struct spark_vars *)&(work)->vars)

/* his dust: pale blue puffs that glow. before each puff's create, his
 * dust functions set its scale, pos and speed here */
static struct dust_params e102_dust = {
    1.0F,
    0.03F,
    0.19F,
    0.05F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.3F, 0.8F, 0.8F, 1.0F},
};

/* a spark: a line 0.8 long that spins as it flies, in colors drawn anew
 * each step */
static NJS_POINT3 e102_spark_points[2] = {
#include "player/e102_effects/e102_spark_points.inc"
};
static NJS_COLOR e102_spark_colors[2] = {
#include "player/e102_effects/e102_spark_colors.inc"
};
static NJS_POINT3COL e102_spark_line = {
#include "player/e102_effects/e102_spark_line.inc"
};
/* a spark's launch speed before it's turned: 1 to 3 along x */
static NJS_VECTOR e102_spark_speed = {0.0F, 0.0F, 0.0F};

void e102_bob_booster(struct task_work *work, struct player *record,
                      struct e102 *e102)
{
    Float speed;
    int tilt;

    speed = CLAMP(_builtin_fabsf(record->speed.x + record->speed.z) - 1.0F,
                  0.0F, 1.0F);
    e102->booster_phase +=
        (Angle)(speed * (12.5F * ANGLE_PER_DEGREE)) + NJM_DEG_ANG(11.25);
    e102->booster_lift = njSin(e102->booster_phase) * BOOSTER_BOB;
    tilt = record->speed.x * (10.0F * ANGLE_PER_DEGREE);
    e102->booster_ang_z = CLAMP(tilt, -NJM_DEG_ANG(25), NJM_DEG_ANG(25));
}

Uint32 e102_floor_flags(struct task_work *work)
{
    struct floor_query query;
    NJS_VECTOR away;
    struct world_object *objects;
    NJS_OBJECT *object;
    Uint32 flags;
    /* @hack type: i a Uint16, which the image zero-extends to index with it and
     * to test it (EXTU.W at 0x8c0db820 and 0x8c0db8d6). as an int, the two
     * EXTU.W go, the function runs 2 bytes short and 22 of 141 words differ */
    Uint16 i;
    Float best;
    Float was;
    Float now;

    flags = 0;
    query.pos = work->pos;
    objects = world_objects;
    best = -100000.0F;
    for (i = 0; i < world_count; i++) {
        object = objects[i].object;
        /* an object with no owner, the land, is always tested. a task's reach
         * is its model's radius at its largest scale */
        if (objects[i].owner != NULL) {
            Float distance;
            Float size;
            Float reach;

            away.x = object->pos[0] - work->pos.x;
            away.y = 0.0F;
            away.z = object->pos[2] - work->pos.z;
            distance = njScalor(&away);
            size = object->scl[0];
            if (size < object->scl[1]) {
                size = object->scl[1];
            }
            if (size < object->scl[2]) {
                size = object->scl[2];
            }
            reach = size * object->model->r;
            if (distance > reach) {
                continue;
            }
        }
        if (polygon_find_floor(&query, objects[i].object, objects[i].flags)) {
            was = njAbs(work->pos.y - best);
            now = njAbs(work->pos.y - query.floor.level);
            if (was > now) {
                best = query.floor.level;
                /* a task's floor counts where its force moves it this step */
                if (objects[i].owner != NULL) {
                    best +=
                        ((struct task_force *)objects[i].owner->force)->speed.y;
                }
                flags = objects[i].flags;
            }
        }
    }
    return flags;
}

/* the turns that send a spark off. its yaw is back from the way it faces,
 * up to 20 degrees to either side. its pitch is 30 to 60 degrees up, less
 * its ang.z. each read draws a rand() of its own: e102_spark_step reads the
 * yaw up to three times and the pitch twice */
#define SPARK_YAW(work)                                                        \
    (0x8000 - (work)->ang.y +                                                  \
     (Angle)((20.0F - (rand() * RAND_SCALE * 40.0F)) * ANGLE_PER_DEGREE))
#define SPARK_PITCH(work)                                                      \
    ((Angle)((30.0F + (rand() * RAND_SCALE * 30.0F)) * ANGLE_PER_DEGREE) -     \
     (work)->ang.z)

/* a spark. it leaves with 1 to 3 added to its starting speed, back from
 * the way it faces and upward, then falls and spins for SPARK_STEPS */
static void e102_spark_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_VECTOR speed;
    int i;

    switch (work->mode) {
    case SPARK_START:
        njPushMatrix(&_unit_matrix_);
        e102_spark_speed.x = 1.0F + (rand() * RAND_SCALE * 2.0F);
        SPARK_YAW(work) ? TURN(njRotateY, SPARK_YAW(work)) : (void)0;
        TURN(njRotateZ, SPARK_PITCH(work));
        njCalcVector(NULL, &e102_spark_speed, &speed);
        njAddVector(&motion->speed, &speed);
        njPopMatrix(1);
        work->ang.x = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
        work->ang.y = (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE);
        SPARK(work)->steps = SPARK_STEPS;
        work->mode = SPARK_FLY;
        break;
    case SPARK_FLY:
        SPARK(work)->steps--;
        if (SPARK(work)->steps <= 0) {
            task_destroy(task);
            return;
        }
        work->ang.x += 0x2000;
        work->ang.y += 0x2000;
        motion->speed.y -= 9.8F / 60.0F;
        njAddVector(&work->pos, &motion->speed);
        break;
    }
    njPushMatrix(NULL);
    for (i = 0; i < 2; i++) {
        e102_spark_colors[i].argb.r = 96.0F + (rand() * RAND_SCALE * 160.0F);
        e102_spark_colors[i].argb.g = 96.0F + (rand() * RAND_SCALE * 160.0F);
        e102_spark_colors[i].argb.b = rand() * RAND_SCALE * 128.0F;
    }
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njDrawLine3D(&e102_spark_line, 1, NJD_TRANSPARENT);
    njPopMatrix(1);
}

/* each spark starts by a foot's weld point, with his speed and angles */
void e102_skate_sparks(struct task_work *work, struct task_motion *motion,
                       struct player *record)
{
    NJS_POINT3 skates[2];
    struct collision_shape *shape;
    struct task *task;
    int i;
    int j;

    if (work->flags & PLAYER_ON_GROUND) {
        njPushMatrix(&_unit_matrix_);
        shape = work->collision->shapes;
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, 0x8000 - work->ang.y);
        njTranslate(NULL, -4.0F, -3.0F, 0.0F);
        njCalcPoint(NULL, &record->weld_point3, &skates[0]);
        njAddVector(&skates[0], &shape->center);
        njCalcPoint(NULL, &record->weld_point2, &skates[1]);
        njAddVector(&skates[1], &shape->center);
        njPopMatrix(1);
        /* @hack redundant: a loop of one pass round the pair, which counts j
         * down in R9 (MOV #1,R9 and DT R9 at 0x8c0dbd14, 0x8c0dbd66). without
         * it, 138 of 143 words differ */
        for (j = 1; j != 0; j--) {
            for (i = 0; i < 2; i++) {
                task = task_create(TASK_MOTION | TASK_WORK, 5, e102_spark_step);
                if (task != NULL) {
                    task->work->pos = skates[i];
                    task->motion->speed = motion->speed;
                    task->work->ang = work->ang;
                }
            }
        }
    }
}

/* one puff, at a random point 7 from a spot 10 behind him. it's bigger the
 * lower he is */
void e102_hover_dust(struct task_work *work, Float ground)
{
    Float height = work->pos.y - ground;
    Angle heading = work->ang.y;
    Float back_x = -njCos(heading) * 10.0F;
    Float back_z = -njSin(heading) * 10.0F;

    if (height > 0.0F && height < 30.0F) {
        Angle ang =
            (Angle)(rand() * (RAND_SCALE * (2 * NJD_PI) * ANGLE_PER_RADIAN));
        Float speed = (21.0F - height) * 0.25F;

        e102_dust.scale = 0.5F + (speed * 0.1F);
        e102_dust.pos.x = work->pos.x + back_x + (njCos(ang) * 7.0F);
        e102_dust.pos.y = ground + 1.5F;
        e102_dust.pos.z = work->pos.z + back_z + (njSin(ang) * 7.0F);
        e102_dust.speed.x = speed * ((rand() * RAND_SCALE) - 0.5F);
        e102_dust.speed.y = 0.0F;
        e102_dust.speed.z = speed * ((rand() * RAND_SCALE) - 0.5F);
        dust_create_glow(&e102_dust);
    }
}

/* LANDING_PUFFS puffs, evenly round him 7 out */
void e102_landing_dust(struct task_work *work, struct task_motion *motion)
{
    int i;
    Float speed;

    for (i = 0; i < LANDING_PUFFS; i++) {
        Angle ang =
            (Angle)(i * ((2 * NJD_PI / LANDING_PUFFS) * ANGLE_PER_RADIAN));

        speed = _builtin_fabsf(motion->speed.y) * LANDING_SPEED;
        e102_dust.scale = 0.5F + (speed * 0.1F);
        e102_dust.pos.x = work->pos.x + (njCos(ang) * 7.0F);
        e102_dust.pos.y = work->pos.y + 1.5F;
        e102_dust.pos.z = work->pos.z + (njSin(ang) * 7.0F);
        e102_dust.speed.x = speed * ((rand() * RAND_SCALE) - 0.5F);
        e102_dust.speed.y = 0.0F;
        e102_dust.speed.z = speed * ((rand() * RAND_SCALE) - 0.5F);
        dust_create_glow(&e102_dust);
    }
}

void e102_record_stick(struct e102 *e102, int player, Bool lag)
{
    struct pad_stick *stick = e102->sticks;
    int i;

    for (i = 0; i < E102_STICK_STEPS - 1; i++) {
        *stick = stick[1];
        stick++;
    }
    *stick = pad_sticks[player];
    if (lag) {
        pad_sticks[player].ang = e102->sticks[0].ang;
        pad_sticks[player].stroke = e102->sticks[0].stroke;
    }
}

void e102_restore_stick(struct e102 *e102, int player)
{
    pad_sticks[player].ang = e102->sticks[E102_STICK_STEPS - 1].ang;
    pad_sticks[player].stroke = e102->sticks[E102_STICK_STEPS - 1].stroke;
}

Bool e102_is_aiming(struct task *task)
{
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;
    struct e102 *e102 = record->own.e102.record[0];

    return e102->aim_mode == E102_AIM_START ||
           e102->aim_mode == E102_AIM_LOCKING;
}

void e102_set_skating(int player)
{
    struct task_work *work = player_work[player];

    work->mode = E102_SKATE_START;
}

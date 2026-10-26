/* O_TANPOPO, a dandelion that sheds its seeds. O POPO, a flower with two
 * heads. O HANAA, O HANAB, O GRFLOWERA and O GRFLOWERB, flowers that are
 * only drawn */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/wind.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg02/dandelion.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* windy_dandelion_stem_model and windy_popo_stem_model are the stems */
extern NJS_MODEL windy_dandelion_stem_model;
extern NJS_OBJECT windy_dandelion_head_object;
extern NJS_OBJECT windy_seed_object;
extern NJS_MODEL windy_popo_stem_model;
extern NJS_OBJECT windy_popo_head_object;
extern NJS_OBJECT windy_popo_head_2_object;
extern NJS_OBJECT windy_flower_a_object;
extern NJS_OBJECT windy_flower_b_object;

/* a dandelion starts with SEEDS seeds on its head */
#define SEEDS 13

/* each step, a new seed may drift off the head. its chance is DRIFT_CHANCE
 * for each seed still on the head. the seeds on the head stay */
#define DRIFT_CHANCE 0.0009F

/* a player moving at BLOW_SPEED or more can blow off a seed on the head
 * within BLOW_REACH. the seed goes when a random chance comes up BLOW_CHANCE
 * or more, 1 in 10 a step */
#define BLOW_REACH 5.0F
#define BLOW_SPEED 0.23F
#define BLOW_CHANCE 0.9F

/* a seed blown off takes up to BLOW_ACROSS of the player's x and z speeds.
 * it takes 0.9 up to 1 of the player's y speed, by the same chance. BLOW_LIFT
 * of the player's speed adds to its y speed */
#define BLOW_ACROSS 0.7F
#define BLOW_LIFT 0.2F

/* a player within PUSH_REACH of a flying seed pushes it. the seed takes
 * PUSH_ACROSS of the player's x and z speeds. it takes the player's y speed,
 * plus PUSH_LIFT of the player's speed */
#define PUSH_REACH 2.5F
#define PUSH_ACROSS 0.5F
#define PUSH_LIFT 0.1F

/* a seed in flight ends once it has flown more than SEED_STEPS steps */
#define SEED_STEPS 300

/* a windborne seed's speed moves WIND_SHARE of the way to the wind's push
 * each step. DRAG scales a seed's x and z speeds each step while its speed
 * squared is DRAG_SPEED_SQUARED or more */
#define WIND_SHARE 0.1F
#define DRAG_SPEED_SQUARED 0.4F
#define DRAG 0.98F

/* a random chance scales a drifting seed's x and z speeds by XZ_JITTER plus up
 * to XZ_JITTER_SPAN. that is 5% either way. the same chance scales its y speed
 * by Y_JITTER plus up to Y_JITTER_SPAN, 2% either way */
#define XZ_JITTER 0.95F
#define XZ_JITTER_SPAN 0.1F
#define Y_JITTER 0.98F
#define Y_JITTER_SPAN 0.04F

/* a blown seed's y speed falls SINK a step, until it is below SINK_LIMIT */
#define SINK 0.003F
#define SINK_LIMIT (-0.01F)

/* without a wind, a drifting seed starts at CALM_SPEED up to twice that
 * along x and along z. it rises at CALM_RISE up to twice that */
#define CALM_SPEED 0.05F
#define CALM_RISE 0.0125F

/* a drifting seed slower than STILL_SPEED along both x and z sinks
 * STILL_SINK a step */
#define STILL_SPEED 0.01F
#define STILL_SINK 0.01F

/* the dandelion's and O POPO's modes. no code sets FLOWER_END */
enum flower_mode { FLOWER_START = 0, FLOWER_RUN = 1, FLOWER_END = 3 };

/* modes of a seed that starts on the head */
enum seed_mode { SEED_START, SEED_HELD, SEED_FLYING };
/* modes of a seed that drifts off the head */
enum drift_mode { DRIFT_START, DRIFT_FLYING, DRIFT_GONE };

/* a seed's vars. work->scl holds its speed */
union seed_vars {
    NJS_OBJECT *object; /* the seed's model, never read. flown overwrites it */
    Uint16 flown; /* the steps it has flown */
};

#define SEED(work) ((union seed_vars *)&(work)->vars)
/* TRUE when a wind blew as the seed started. such a seed rides the wind */
#define WINDBORNE(work) ((work)->tick)
#define SEEDS_LEFT(work) TASK_TIMER(work)
/* the head's offset from the dandelion's position, turned with it */
#define HEAD(work) ((work)->scl)

static void windy_dandelion_seed_step(struct task *task);

/* the seeds on the head. a seed's number goes in its work's flags. its tilt
 * adds to the dandelion's angles. no seed's step or draw reads the number or
 * the tilt */
/* clang-format off */
#define SEED_SPEC(number, tilt_x, tilt_z)                                      \
    {windy_dandelion_seed_step, TASK_WORK, (number), {(tilt_x), 0, (tilt_z)}, \
     {0.0F, 0.0F, 0.0F}, (Sint32)&windy_seed_object}
/* clang-format on */
static struct task_spec windy_dandelion_seeds[SEEDS + 1] = {
    SEED_SPEC(0, 0x0000, 0x0000),
    SEED_SPEC(1, 0x2000, 0x2000),
    SEED_SPEC(2, 0x2000, 0x2000),
    SEED_SPEC(3, 0xe000, 0x2000),
    SEED_SPEC(4, 0xe000, 0x2000),
    SEED_SPEC(5, 0x4000, 0x0000),
    SEED_SPEC(6, 0xc000, 0x0000),
    SEED_SPEC(7, 0x0000, 0x4000),
    SEED_SPEC(8, 0x0000, 0xc000),
    SEED_SPEC(9, 0x6000, 0x2000),
    SEED_SPEC(10, 0x6000, 0x2000),
    SEED_SPEC(11, 0xa000, 0x2000),
    SEED_SPEC(12, 0xa000, 0x2000),
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

static void windy_dandelion_find_head(struct task_work *work);
static void windy_dandelion_destroy(struct task *task);
static void windy_dandelion_draw(struct task *task);
static void windy_dandelion_seed_draw(struct task *task);
static void windy_dandelion_drift_step(struct task *task);
static void windy_popo_draw(struct task *task);
static void windy_popo_destroy(struct task *task);

/* the first step makes the seeds on the head, outside the set editor. each
 * later step runs them, and may send a new seed drifting */
void windy_dandelion_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case FLOWER_START:
        task->draw = windy_dandelion_draw;
        SEEDS_LEFT(work) = SEEDS;
        /* flown is a seed's field. this clears it in the dandelion's own
         * vars, where nothing reads it */
        SEED(work)->flown = 0;
        if (!placement_editing(task)) {
            task_create_children(windy_dandelion_seeds, task);
        }
        task->destructor = windy_dandelion_destroy;
        windy_dandelion_draw(task);
        work->mode = FLOWER_RUN;
        break;
    case FLOWER_RUN:
        windy_dandelion_draw(task);
        windy_dandelion_find_head(work);
        if (!placement_editing(task)) {
            task_step_children(task);
        }
        if (!placement_editing(task)) {
            if (rand() * RAND_SCALE < SEEDS_LEFT(work) * DRIFT_CHANCE) {
                /* task_create_child has made child the first child, at the
                 * dandelion's position. these stores repeat it */
                if ((child = task_create_child(TASK_WORK,
                                               windy_dandelion_drift_step,
                                               task)) != NULL) {
                    task->child = child;
                    child->work->pos = work->pos;
                }
            }
        }
        break;
    case FLOWER_END:
        windy_dandelion_destroy(task);
        break;
    }
}

static void windy_dandelion_find_head(struct task_work *work)
{
    NJS_VECTOR up;
    NJS_VECTOR head;
    Angle ang;

    up.x = 0.0F;
    up.y = 3.5F;
    up.z = 0.28F;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcVector(NULL, &up, &head);
    njPopMatrix(1);
    HEAD(work) = head;
}

static void windy_dandelion_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* the head on the stem turns to face the camera */
static void windy_dandelion_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *head = &windy_dandelion_head_object;
    Float *at;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&windy_dandelion_stem_model);
    njPushMatrix(NULL);
    at = head->pos;
    njTranslate(NULL, at[0], at[1], at[2]);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    draw_model(head->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* a seed stays on the head until a player blows it off. in flight, a
 * player's push sets its speed. without a push, a windborne seed rides the
 * wind. DRAG slows a fast seed, and the seed sinks */
static void windy_dandelion_seed_step(struct task *task)
{
    static const NJS_VECTOR windy_seed_calm = {0.0F, 0.0F, 0.0F};
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    NJS_POINT3 at;
    Float chance;
    int player;
    Float speed;
    struct task_motion *motion;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SEED_START:
        SEED(work)->flown = 0;
        work->mode = SEED_HELD;
        WINDBORNE(work) = wind_get_force(&work->pos, NULL);
        break;
    case SEED_HELD:
        windy_dandelion_seed_draw(task);
        at = work->pos;
        njAddVector(&at, &HEAD(parent));
        if ((player = player_in_range(&at, BLOW_REACH)) != 0) {
            if ((chance = rand() * RAND_SCALE) >= BLOW_CHANCE) {
                motion = player_motion[player - 1];
                if ((speed = njScalor(&motion->speed)) >= BLOW_SPEED) {
                    work->scl.x = _builtin_fabsf(rand() * RAND_SCALE) *
                                  motion->speed.x * BLOW_ACROSS;
                    /* @hack redundant: the y speed times 1, for the image's
                     * FLDI1 and FMAC at 0x0c90672c. without it, 38 of 308
                     * words differ */
                    work->scl.y =
                        (_builtin_fabsf(chance) * motion->speed.y * 1.0F) +
                        (speed * BLOW_LIFT);
                    work->scl.z = _builtin_fabsf(rand() * RAND_SCALE) *
                                  motion->speed.z * BLOW_ACROSS;
                    work->mode = SEED_FLYING;
                    if (SEEDS_LEFT(parent) > 0) {
                        SEEDS_LEFT(parent)--;
                    }
                }
            }
        }
        break;
    case SEED_FLYING:
        if (SEED(work)->flown++ > SEED_STEPS) {
            task_kill(task);
            break;
        }
        njAddVector(&work->pos, &work->scl);
        if ((player = player_in_range(&work->pos, PUSH_REACH)) != 0) {
            motion = player_motion[player - 1];
            speed = njScalor(&motion->speed);
            work->scl.x = motion->speed.x * PUSH_ACROSS;
            work->scl.y = motion->speed.y + (speed * PUSH_LIFT);
            work->scl.z = motion->speed.z * PUSH_ACROSS;
        } else {
            if (WINDBORNE(work)) {
                at = windy_seed_calm;
                wind_get_force(&work->pos, &at);
                work->scl.x += (at.x - work->scl.x) * WIND_SHARE;
                work->scl.y += (at.y - work->scl.y) * WIND_SHARE;
                work->scl.z += (at.z - work->scl.z) * WIND_SHARE;
            }
            if (njScalor2(&work->scl) >= DRAG_SPEED_SQUARED) {
                work->scl.x *= DRAG;
                work->scl.z *= DRAG;
            }
            if (work->scl.y >= SINK_LIMIT) {
                work->scl.y -= SINK;
            }
        }
        windy_dandelion_seed_draw(task);
        break;
    }
}

static void windy_dandelion_seed_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *parent = task->parent->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x + HEAD(parent).x,
                work->pos.y + HEAD(parent).y, work->pos.z + HEAD(parent).z);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    draw_tree(&windy_seed_object);
    njPopMatrix(1);
}

/* a drifting seed starts at the wind's push, or at a calm drift where no
 * wind blows. in flight, a player's push sets its speed. without a push, a
 * windborne seed rides the wind, and a random jitter then scales its speed.
 * DRAG slows a fast seed. a still seed sinks */
static void windy_dandelion_drift_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR wind;
    Float chance;
    Float jitter;
    Float speed;
    int player;
    struct task_motion *motion;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case DRIFT_START:
        SEED(work)->object = &windy_seed_object;
        if ((WINDBORNE(work) = wind_get_force(&work->pos, &work->scl)) ==
            FALSE) {
            chance = rand() * RAND_SCALE;
            work->scl.x = CALM_SPEED + (rand() * (CALM_SPEED * RAND_SCALE));
            work->scl.y = CALM_RISE + (chance * CALM_RISE);
            work->scl.z = CALM_SPEED + (rand() * (CALM_SPEED * RAND_SCALE));
        }
        windy_dandelion_seed_draw(task);
        work->mode = DRIFT_FLYING;
        SEED(work)->flown = 0;
        break;
    case DRIFT_FLYING:
        SEED(work)->flown++;
        if (SEED(work)->flown > SEED_STEPS) {
            work->mode = DRIFT_GONE;
            break;
        }
        njAddVector(&work->pos, &work->scl);
        windy_dandelion_seed_draw(task);
        if ((player = player_in_range(&work->pos, PUSH_REACH)) != 0) {
            motion = player_motion[player - 1];
            speed = njScalor(&motion->speed);
            work->scl.x = motion->speed.x * PUSH_ACROSS;
            work->scl.y = motion->speed.y + (speed * PUSH_LIFT);
            work->scl.z = motion->speed.z * PUSH_ACROSS;
            break;
        }
        if (WINDBORNE(work)) {
            wind_get_force(&work->pos, &wind);
            work->scl.x += (wind.x - work->scl.x) * WIND_SHARE;
            work->scl.y += (wind.y - work->scl.y) * WIND_SHARE;
            work->scl.z += (wind.z - work->scl.z) * WIND_SHARE;
        }
        chance = rand() * RAND_SCALE;
        work->scl.x *= (jitter = XZ_JITTER + (chance * XZ_JITTER_SPAN));
        work->scl.y *= Y_JITTER + (chance * Y_JITTER_SPAN);
        work->scl.z *= jitter;
        if (njScalor2(&work->scl) >= DRAG_SPEED_SQUARED) {
            work->scl.x *= DRAG;
            work->scl.z *= DRAG;
        }
        if (_builtin_fabsf(work->scl.x) < STILL_SPEED &&
            _builtin_fabsf(work->scl.z) < STILL_SPEED) {
            work->pos.y -= STILL_SINK;
        }
        break;
    case DRIFT_GONE:
        task_kill(task);
        break;
    }
}

void windy_popo_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case FLOWER_START:
        task->draw = windy_popo_draw;
        task->destructor = windy_popo_destroy;
        work->mode = FLOWER_RUN;
        break;
    case FLOWER_RUN:
        windy_popo_draw(task);
        break;
    case FLOWER_END:
        break;
    }
}

/* the two heads on the stem turn to face the camera */
static void windy_popo_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *head;
    NJS_OBJECT *head_2;
    Float *at;
    Angle ang;

    texture_set_stage();
    head = &windy_popo_head_object;
    head_2 = &windy_popo_head_2_object;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&windy_popo_stem_model);
    njPushMatrix(NULL);
    at = head->pos;
    njTranslate(NULL, at[0], at[1], at[2]);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    draw_model(head->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, head_2->pos[0], head_2->pos[1], head_2->pos[2]);
    TURN_VIA(njRotateY, -work->ang.y, ang);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    draw_model(head_2->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void windy_popo_destroy(struct task *task)
{
}

void windy_flower_a_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_flower_a_object;
    model_start(task);
}

void windy_flower_b_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_flower_b_object;
    model_start(task);
}

void windy_grass_flower_b_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &windy_flower_b_object;
    model_start(task);
}

void windy_grass_flower_a_step(struct task *task)
{
    struct task_work *work = task->work;

    /* O GRFLOWERA draws flower B, as O GRFLOWERB does */
    MODEL(work)->object = &windy_flower_b_object;
    model_start(task);
}

Bool windy_no_child_killed(struct task *task)
{
    struct task *child;

    for (child = task->child; child != NULL; child = child->next) {
        if (child->step == task_destroy) {
            return FALSE;
        }
    }
    return TRUE;
}

/* nothing calls this. SHC leaves out its code, but keeps its locals'
 * initializers and its strings in the constants, from 0x0ca15604. group.c in
 * STG04 prints the same labels for a group's count and spacing */
/* NOLINTNEXTLINE(clang-diagnostic-unused-function) */
static void windy_dandelion_label_scales(void)
{
    struct angles ang = {0, 0, 0};
    NJS_VECTOR offset = {0.0F, 0.0F, 0.0F};

    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- NUM");
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- DISTANCE");
}
